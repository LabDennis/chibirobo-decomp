#include <global.h>

#include <float.h>
#include <math.h>
#include <string.h>

#include <dolphin/mtx/mtx.h>
#include <dolphin/mtx/vec.h>
#include <sysdolphin/cobj.h>
#include <sysdolphin/jobj.h>
#include <sysdolphin/list.h>
#include <sysdolphin/memory.h>
#include <sysdolphin/mtx.h>
#include <sysdolphin/object.h>
#include <sysdolphin/particle.h>
#include <sysdolphin/particle_internal.h>
#include <sysdolphin/random.h>

#define M_PI   3.141592653589793
#define M_PI_2 1.5707963267948966

// The low four bits of type select the emitter shape.
enum {
	GEN_SHAPE_DISC              = 0,
	GEN_SHAPE_LINE              = 1,
	GEN_SHAPE_TORNADO           = 2,
	GEN_SHAPE_DISC_RADIAL_SPEED = 3,
	GEN_SHAPE_DISC_UNIFORM      = 4,
	GEN_SHAPE_BOX               = 5,
	GEN_SHAPE_CONE              = 6,
	GEN_SHAPE_CYLINDER          = 7,
	GEN_SHAPE_SPHERE            = 8,
	GEN_SHAPE_MASK              = 0xF,
};

enum {
	GEN_KIND_PRIME_COUNT = 0x100,
	GEN_KIND_FACE_CAMERA = 0x10000,
	GEN_KIND_CAMERA_SRT  = 0x20000,
};

// Each bit allows emission from the two faces perpendicular to its axis.
enum {
	GEN_BOX_FACE_X = 1,
	GEN_BOX_FACE_Y = 2,
	GEN_BOX_FACE_Z = 4,
};

#define PS_GENERATOR_LINK_MASK_SHIFT 16
#define PS_EMISSION_ID_MIN 0x100

u16 sParticleGeneratorIdCounter = PS_EMISSION_ID_MIN;

void (*sCustomGeneratorEmit)(HSD_Generator* gen, Mtx mtx);
void (*sCustomGeneratorInit)(HSD_Generator* gen);
HSD_CObj* psCamera;
HSD_Generator* sFreeGenerators;
HSD_Generator* sActiveGenerators;
HSD_Generator* sGeneratorPrev;
HSD_SList* sQueuedJointGenerators;

static char sPathGeneratorC[] = "generator.c";
static char sGeneratorCameraCondition[] = "psCamera";

static void psSetBillboardCamera(HSD_CObj* cobj);

static inline f32 gen_sinf(f32 x)
{
	return sin(x);
}

static inline f32 gen_cosf(f32 x)
{
	return cos(x);
}

static inline f32 gen_atan2f(f32 y, f32 x)
{
	return atan2(y, x);
}

// Compute atan2, using a signed right angle when x is near zero.
#define GEN_ANGLE(dst, y, x)                \
	do {                                    \
		if (__fabsf(x) < MSL_FLT_MIN) {     \
			if ((y) >= 0.0F) {              \
				(dst) = (f32)M_PI_2;        \
			} else {                        \
				(dst) = -(f32)M_PI_2;       \
			}                               \
		} else {                            \
			(dst) = gen_atan2f((y), (x));   \
		}                                   \
	} while (0)

static inline void gen_normalizeColumn(Mtx matrix, int column, Vec* axis)
{
	axis->x = matrix[0][column];
	axis->y = matrix[1][column];
	axis->z = matrix[2][column];
	PSVECNormalize(axis, axis);
}

static inline f32 gen_randRadius(f32 radius)
{
	if (radius < 0.0F) {
		return -radius;
	}
	return radius * HSD_Randf();
}

static inline void gen_cobjUnref(HSD_CObj* cobj)
{
	if (cobj != NULL && ref_DEC(cobj)) {
		hsdDelete(cobj);
	}
}

// unlinks a dead generator, or parks it when children or a shared srt still need it
static inline void gen_killNode(HSD_Generator* gen)
{
	HSD_Generator* prev;
	HSD_psAppSRT* srt;

	prev = sGeneratorPrev;
	if (gen->callback != NULL) {
		gen->callback(gen);
	}
	if (gen->type & PS_GEN_KILL_CHILDREN) {
		psKillGeneratorChild(gen);
	}
	if (gen->numChild != 0) {
		prev         = gen;
		gen->random  = 0.0F;
		gen->genLife = 1;
	} else if ((gen->type & PS_GEN_USE_JOINT) && gen->jobj != NULL
	           && (gen->type & PS_GEN_SRT_JOINT_TRANSFORM)
	           && (srt = gen->appsrt) != NULL && srt->gp == gen && srt->usedCount != 1) {
		prev         = gen;
		gen->random  = 0.0F;
		gen->genLife = 1;
	} else {
		if (prev == NULL) {
			sActiveGenerators = gen->next;
		} else {
			prev->next = gen->next;
		}
		if (gen->appsrt != NULL) {
			psRemoveGeneratorAppSRT(gen);
		}
		gen->next = sFreeGenerators;
		sFreeGenerators = gen;
		psGeneratorCount--;
		if (gen->jobj != NULL) {
			HSD_JObjUnref(gen->jobj);
			gen->jobj = NULL;
		}
	}
	sGeneratorPrev = prev;
}

static inline void gen_kill(HSD_Generator* gen)
{
	HSD_Generator* cur;

	cur = sActiveGenerators;
	sGeneratorPrev = NULL;
	while (cur != NULL) {
		if (cur == gen) {
			gen_killNode(gen);
			if (sGeneratorPrev != NULL) {
				while (sGeneratorPrev->next != NULL) {
					sGeneratorPrev = sGeneratorPrev->next;
				}
			} else if (sActiveGenerators != NULL) {
				sGeneratorPrev = sActiveGenerators;
				while (sGeneratorPrev->next != NULL) {
					sGeneratorPrev = sGeneratorPrev->next;
				}
			}
			return;
		}
		sGeneratorPrev = cur;
		cur = cur->next;
	}
}

static inline u16 gen_nextId(void)
{
	if (++sParticleGeneratorIdCounter < PS_EMISSION_ID_MIN) {
		sParticleGeneratorIdCounter = PS_EMISSION_ID_MIN;
	}
	return sParticleGeneratorIdCounter;
}

/**
 * @note Address: 0x801E2434
 * @note Size: 0x28
 */
u16 psGetNewIDNum(void)
{
	return gen_nextId();
}

/**
 * @note Address: 0x801E245C
 * @note Size: 0x98
 */
void psInitGenerator(s32 num)
{
	HSD_Generator* gp;
	s32 i;

	sActiveGenerators = NULL;
	sFreeGenerators = NULL;
	for (i = 0; i < num; ++i) {
		gp = HSD_Alloc(sizeof(*gp));
		if (gp == NULL) {
			break;
		}
		memset(gp, 0, sizeof(*gp));
		gp->next = sFreeGenerators;
		sFreeGenerators = gp;
	}
	psGeneratorCount = 0;
	psPeakGeneratorCount = 0;
	sQueuedJointGenerators = NULL;
	psCamera = NULL;
	sCustomGeneratorEmit = NULL;
	sCustomGeneratorInit = NULL;
	sGeneratorPrev = NULL;
}

/**
 * @note Address: 0x801E24F4
 * @note Size: 0x58
 */
void psRemoveGenerator(void)
{
	HSD_Generator* gp;
	HSD_Generator* next;

	psKillAllParticle();
	psKillAllGenerator();
	for (gp = sFreeGenerators; gp != NULL; gp = next) {
		next = gp->next;
		HSD_Free(gp);
	}
	sFreeGenerators = NULL;
	psSetBillboardCamera(NULL);
}

/**
 * @note Address: 0x801E254C
 * @note Size: 0x1BC
 */
void psKillGenerator(HSD_Generator* gen)
{
	gen_kill(gen);
}

/**
 * @note Address: 0x801E2708
 * @note Size: 0x1F0
 */
void psKillAllGenerator(void)
{
	HSD_Generator* gen;
	HSD_Generator* next;

	for (gen = sActiveGenerators; gen != NULL; gen = next) {
		next = gen->next;
		gen_kill(gen);
	}
	while (sQueuedJointGenerators != NULL) {
		sQueuedJointGenerators = HSD_SListRemove(sQueuedJointGenerators);
	}
}

/**
 * @note Address: 0x801E28F8
 * @note Size: 0x2DC
 */
static void psApplyGeneratorJObj(HSD_Generator* gen)
{
	HSD_JObj* jobj;
	Vec col;
	f32 c0x, c0y, c0z;
	f32 c1x, c1y, c1z;

	if (!(gen->type & PS_GEN_USE_JOINT)) {
		return;
	}
	if ((jobj = gen->jobj) == NULL) {
		return;
	}
	HSD_JObjSetupMatrix(jobj);
	if (gen->type & PS_GEN_COPY_JOINT_POSITION) {
		gen->pos.x = gen->jobj->mtx[0][3];
		gen->pos.y = gen->jobj->mtx[1][3];
		gen->pos.z = gen->jobj->mtx[2][3];
	}
	if (gen->type & PS_GEN_SRT_JOINT_POSITION) {
		if (gen->appsrt != NULL && gen->appsrt->gp == gen) {
			gen->appsrt->translate.x = gen->jobj->mtx[0][3];
			gen->appsrt->translate.y = gen->jobj->mtx[1][3];
			gen->appsrt->translate.z = gen->jobj->mtx[2][3];
		}
	}
	if (gen->type & PS_GEN_SRT_JOINT_SCALE) {
		if (gen->appsrt != NULL && gen->appsrt->gp == gen) {
			HSD_MtxGetScale(gen->jobj->mtx, &gen->appsrt->scale);
		}
	}

	gen_normalizeColumn(jobj->mtx, 0, &col);
	c0x = col.x;
	c0y = col.y;
	c0z = col.z;

	gen_normalizeColumn(jobj->mtx, 1, &col);
	c1x = col.x;
	c1y = col.y;
	c1z = col.z;

	gen_normalizeColumn(jobj->mtx, 2, &col);

	{
		f32 c2y    = col.y;
		f32 c2z    = col.z;
		f32 vx     = gen->vel.x;
		f32 vy     = gen->vel.y;
		f32 vz     = gen->vel.z;
		gen->vel.x = c0x * vx + c1x * vy + col.x * vz;
		gen->vel.y = c0y * vx + c1y * vy + c2y * vz;
		gen->vel.z = c0z * vx + c1z * vy + c2z * vz;
	}

	if ((gen->type & GEN_SHAPE_MASK) == GEN_SHAPE_LINE) {
		f32 ax           = gen->aux.line.x2;
		f32 ay           = gen->aux.line.y2;
		f32 az           = gen->aux.line.z2;
		gen->aux.line.x2 = jobj->mtx[0][0] * ax + jobj->mtx[0][1] * ay + jobj->mtx[0][2] * az;
		gen->aux.line.y2 = jobj->mtx[1][0] * ax + jobj->mtx[1][1] * ay + jobj->mtx[1][2] * az;
		gen->aux.line.z2 = jobj->mtx[2][0] * ax + jobj->mtx[2][1] * ay + jobj->mtx[2][2] * az;
	}
}

/**
 * @note Address: 0x801E2BD4
 * @note Size: 0x140C
 */
static f32 generateParticle(HSD_Generator* gen)
{
	// NONMATCHING: floating-point registers in the direction and cone calculations.
	Vec vel;
	Vec pos;
	Vec rvec;
	Vec vec;
	Mtx mtx;
	Mtx jmtx;
	Vec look;
	Vec up;
	Vec right;
	f32 speed;
	f32 rx;
	f32 ry;
	f32 azimuth;
	f32 azimuthStep;
	f32 spreadAngle;

	ry = rx = 0.0F;
	if (gen->count < 1.0F) {
		return gen->count;
	}

	vel.x = gen->vel.x;
	vel.y = gen->vel.y;
	vel.z = gen->vel.z;
	speed = msl_sqrtf(vel.z * vel.z + (vel.x * vel.x + vel.y * vel.y));

	PSMTXIdentity(mtx);

	if ((gen->type & PS_GEN_USE_JOINT) && gen->jobj != NULL && (gen->type & PS_GEN_USE_JOINT_ROTATION)
	    && !(gen->kind & (GEN_KIND_FACE_CAMERA | GEN_KIND_CAMERA_SRT))) {
		PSMTXCopy(gen->jobj->mtx, jmtx);

		gen_normalizeColumn(jmtx, 0, &vec);
		mtx[0][0] = vec.x;
		mtx[1][0] = vec.y;
		mtx[2][0] = vec.z;

		gen_normalizeColumn(jmtx, 1, &vec);
		mtx[0][1] = vec.x;
		mtx[1][1] = vec.y;
		mtx[2][1] = vec.z;

		gen_normalizeColumn(jmtx, 2, &vec);
		mtx[0][2] = vec.x;
		mtx[1][2] = vec.y;
		mtx[2][2] = vec.z;

		mtx[0][3] = mtx[1][3] = mtx[2][3] = 0.0F;
	}

	if (gen->kind & GEN_KIND_FACE_CAMERA) {
		if (!psCamera) {
			__assert(sPathGeneratorC, 0x21A, sGeneratorCameraCondition);
		}
		look.x = psCamera->eye_position->pos.x - gen->pos.x;
		look.y = psCamera->eye_position->pos.y - gen->pos.y;
		look.z = psCamera->eye_position->pos.z - gen->pos.z;
		PSVECNormalize(&look, &look);
		HSD_CObjGetUpVector(psCamera, &up);
		PSVECNormalize(&up, &up);
		PSVECCrossProduct(&up, &look, &right);
		PSVECCrossProduct(&look, &right, &up);

		mtx[0][0] = right.x;
		mtx[1][0] = right.y;
		mtx[2][0] = right.z;
		mtx[0][1] = up.x;
		mtx[1][1] = up.y;
		mtx[2][1] = up.z;
		mtx[0][2] = look.x;
		mtx[1][2] = look.y;
		mtx[2][2] = look.z;
	}

	if ((gen->type & GEN_SHAPE_MASK) != GEN_SHAPE_LINE && speed > FLT_EPSILON) {
		Vec dir;
		Mtx rot;
		f32 xAngle, yAngle;
		f32 sinX, cosX, sinY;

		dir.x = gen->vel.x;
		dir.y = gen->vel.y;
		dir.z = gen->vel.z;
		PSVECNormalize(&dir, &dir);

		GEN_ANGLE(xAngle, dir.y, dir.z);
		sinX = gen_sinf(xAngle);
		cosX = gen_cosf(xAngle);
		GEN_ANGLE(yAngle, dir.x, dir.y * sinX + dir.z * cosX);
		sinY = gen_sinf(yAngle);
		{
			f32 cosY    = gen_cosf(yAngle);
			f32 negSinX = -sinX;

			rot[0][0] = cosY;
			rot[0][1] = 0.0F;
			rot[0][2] = sinY;
			rot[0][3] = 0.0F;
			rot[1][0] = negSinX * sinY;
			rot[1][1] = cosX;
			rot[1][2] = sinX * cosY;
			rot[1][3] = 0.0F;
			rot[2][0] = -cosX * sinY;
			rot[2][1] = negSinX;
			rot[2][2] = cosX * cosY;
			rot[2][3] = 0.0F;
		}
		PSMTXConcat(mtx, rot, mtx);
	}

	if ((gen->type & GEN_SHAPE_MASK) == GEN_SHAPE_TORNADO) {
		f32 len;

		GEN_ANGLE(rx, mtx[1][2], mtx[2][2]);
		len = mtx[1][2] * gen_sinf(rx) + mtx[2][2] * gen_cosf(rx);
		GEN_ANGLE(ry, mtx[0][2], len);
	}

	if (gen->angle < 0.0F) {
		switch (gen->type & GEN_SHAPE_MASK) {
		case GEN_SHAPE_DISC:
		case GEN_SHAPE_DISC_RADIAL_SPEED:
		case GEN_SHAPE_DISC_UNIFORM: {
			f32 min     = gen->aux.disc.minAngle;
			azimuth     = (gen->aux.disc.maxAngle - min) / (f32)(s32)gen->count * HSD_Randf() + min;
			azimuthStep = (gen->aux.disc.maxAngle - min) / (f32)(s32)gen->count;
			break;
		}
		case GEN_SHAPE_CONE:
		case GEN_SHAPE_CYLINDER: {
			f32 min     = gen->aux.cone.minAngle;
			azimuth     = (gen->aux.cone.maxAngle - min) / (f32)(s32)gen->count * HSD_Randf() + min;
			azimuthStep = (gen->aux.cone.maxAngle - min) / (f32)(s32)gen->count;
			break;
		}
		default:
			azimuth     = 2.0 * (M_PI * HSD_Randf());
			azimuthStep = (2.0 * M_PI) / (f64)(s32)gen->count;
			break;
		}
	}

	while (gen->count >= 1.0F) {
		switch (gen->type & GEN_SHAPE_MASK) {
		case GEN_SHAPE_DISC:
		case GEN_SHAPE_DISC_RADIAL_SPEED:
		case GEN_SHAPE_DISC_UNIFORM:
		case GEN_SHAPE_CONE:
		case GEN_SHAPE_CYLINDER: {
			f32 radius;
			f32 radialFraction;

			if (gen->radius < 0.0F) {
				radius         = -gen->radius;
				radialFraction = 1.0F;
			} else {
				radialFraction = HSD_Randf();
				if ((gen->type & GEN_SHAPE_MASK) == GEN_SHAPE_DISC_RADIAL_SPEED
				    || (gen->type & GEN_SHAPE_MASK) == GEN_SHAPE_DISC_UNIFORM) {
					// Square root distributes samples evenly over the disc's area.
					radialFraction = msl_sqrtf(radialFraction);
				}
				radius = radialFraction * gen->radius;
			}

			switch (gen->type & GEN_SHAPE_MASK) {
			case GEN_SHAPE_CONE:
				if (gen->angle < 0.0F) {
					azimuth += azimuthStep;
					if (__fabsf(radius) < MSL_FLT_MIN) {
						if (gen->aux.cone.height >= 0.0F) {
							spreadAngle = -gen->angle;
						} else {
							spreadAngle = M_PI - gen->angle;
						}
					} else {
						spreadAngle = M_PI_2 - gen_atan2f(gen->aux.cone.height, radius) - gen->angle;
					}
				} else {
					f32 min = gen->aux.cone.minAngle;
					azimuth = (gen->aux.cone.maxAngle - min) * HSD_Randf() + min;
					if (__fabsf(radius) < MSL_FLT_MIN) {
						if (gen->aux.cone.height >= 0.0F) {
							spreadAngle = gen->angle;
						} else {
							spreadAngle = M_PI + gen->angle;
						}
					} else {
						spreadAngle = gen->angle + (M_PI_2 - gen_atan2f(gen->aux.cone.height, radius));
					}
				}
				break;
			case GEN_SHAPE_CYLINDER:
				if (gen->angle < 0.0F) {
					azimuth += azimuthStep;
					spreadAngle = M_PI_2 - gen->angle;
				} else {
					f32 min     = gen->aux.cone.minAngle;
					azimuth     = (gen->aux.cone.maxAngle - min) * HSD_Randf() + min;
					spreadAngle = M_PI_2 + gen->angle;
				}
				break;
			default:
				if (gen->angle < 0.0F) {
					azimuth += azimuthStep;
					spreadAngle = radialFraction * -gen->angle;
				} else {
					f32 min     = gen->aux.disc.minAngle;
					azimuth     = (gen->aux.disc.maxAngle - min) * HSD_Randf() + min;
					spreadAngle = radialFraction * gen->angle;
				}
				break;
			}

			pos.x = radius * gen_cosf(azimuth);
			pos.y = radius * gen_sinf(azimuth);
			if ((gen->type & GEN_SHAPE_MASK) == GEN_SHAPE_CONE || (gen->type & GEN_SHAPE_MASK) == GEN_SHAPE_CYLINDER) {
				pos.z = HSD_Randf();
				if ((gen->type & GEN_SHAPE_MASK) == GEN_SHAPE_CONE) {
					pos.x *= 1.0F - pos.z;
					pos.y *= 1.0F - pos.z;
				}
				pos.z *= gen->aux.cone.height;
			} else {
				pos.z = 0.0F;
			}

			{
				f32 st = speed * gen_sinf(spreadAngle);
				vec.x  = st * gen_cosf(azimuth);
				vec.y  = st * gen_sinf(azimuth);
				vec.z  = speed * gen_cosf(spreadAngle);
			}
			if ((gen->type & GEN_SHAPE_MASK) == GEN_SHAPE_DISC_RADIAL_SPEED) {
				vec.x *= radialFraction;
				vec.y *= radialFraction;
				vec.z *= radialFraction;
			}

			PSMTXMultVec(mtx, &pos, &pos);
			pos.x += gen->pos.x;
			pos.y += gen->pos.y;
			pos.z += gen->pos.z;
			PSMTXMultVec(mtx, &vec, &vec);
			psGenerateParticle(gen->linkNo, gen->bank, 0, gen->kind, gen->texGroup, gen->cmdList, gen->life, pos.x, pos.y, pos.z, vec.x,
			                   vec.y, vec.z, gen->size, gen->grav, gen->fric, 0, gen);
			break;
		}

		case GEN_SHAPE_LINE: {
			f32 rnd = HSD_Randf();
			pos.x   = rnd * gen->aux.line.x2;
			pos.y   = rnd * gen->aux.line.y2;
			pos.z   = rnd * gen->aux.line.z2;
			PSMTXMultVec(mtx, &pos, &pos);
			pos.x += gen->pos.x;
			pos.y += gen->pos.y;
			pos.z += gen->pos.z;
			PSMTXMultVec(mtx, &vel, &vec);
			psGenerateParticle(gen->linkNo, gen->bank, 0, gen->kind, gen->texGroup, gen->cmdList, gen->life, pos.x, pos.y, pos.z, vec.x,
			                   vec.y, vec.z, gen->size, gen->grav, gen->fric, 0, gen);
			break;
		}

		case GEN_SHAPE_TORNADO: {
			f32 radiusFraction;

			if (gen->radius < 0.0F) {
				radiusFraction = 1.0F;
			} else {
				radiusFraction = HSD_Randf();
			}
			if (gen->angle < 0.0F) {
				azimuth += azimuthStep;
			} else {
				azimuth = 2.0 * (M_PI * HSD_Randf());
			}
			gen->aux.tornado.vel = speed;
			psGenerateParticle(gen->linkNo, gen->bank, 0, gen->kind | PS_PARTICLE_TORNADO, gen->texGroup, gen->cmdList, gen->life, 0.0F,
			                   0.0F, 0.0F, azimuth, radiusFraction, 0.0F, gen->size, rx, ry, 0, gen);
			break;
		}

		case GEN_SHAPE_BOX:
			pos.x = HSD_Randf();
			pos.y = HSD_Randf();
			pos.z = HSD_Randf();
			switch (gen->aux.rect.flag) {
			case 0:
				break;
			case GEN_BOX_FACE_X:
				pos.x = pos.x > 0.5F ? 1.0F : 0.0F;
				break;
			case GEN_BOX_FACE_Y:
				pos.y = pos.y > 0.5F ? 1.0F : 0.0F;
				break;
			case GEN_BOX_FACE_X | GEN_BOX_FACE_Y: {
				f32 rnd = HSD_Randf();
				f32 a   = gen->aux.rect.x;
				if (rnd > a / (a + gen->aux.rect.y)) {
					pos.y = pos.y > 0.5F ? 1.0F : 0.0F;
				} else {
					pos.x = pos.x > 0.5F ? 1.0F : 0.0F;
				}
				break;
			}
			case GEN_BOX_FACE_Z:
				pos.z = pos.z > 0.5F ? 1.0F : 0.0F;
				break;
			case GEN_BOX_FACE_X | GEN_BOX_FACE_Z: {
				f32 rnd = HSD_Randf();
				f32 a   = gen->aux.rect.x;
				if (rnd > a / (a + gen->aux.rect.z)) {
					pos.z = pos.z > 0.5F ? 1.0F : 0.0F;
				} else {
					pos.x = pos.x > 0.5F ? 1.0F : 0.0F;
				}
				break;
			}
			case GEN_BOX_FACE_Y | GEN_BOX_FACE_Z: {
				f32 rnd = HSD_Randf();
				f32 a   = gen->aux.rect.y;
				if (rnd > a / (a + gen->aux.rect.z)) {
					pos.z = pos.z > 0.5F ? 1.0F : 0.0F;
				} else {
					pos.y = pos.y > 0.5F ? 1.0F : 0.0F;
				}
				break;
			}
			case GEN_BOX_FACE_X | GEN_BOX_FACE_Y | GEN_BOX_FACE_Z: {
				f32 rnd = HSD_Randf();
				f32 z   = gen->aux.rect.z;
				f32 y   = gen->aux.rect.y;
				f32 x   = gen->aux.rect.x;
				f32 inv = 1.0F / (x * (y + z) + y * z);
				if (rnd < inv * (x * y)) {
					pos.z = pos.z > 0.5F ? 1.0F : 0.0F;
				} else if (rnd > 1.0F - inv * (x * z)) {
					pos.y = pos.y > 0.5F ? 1.0F : 0.0F;
				} else {
					pos.x = pos.x > 0.5F ? 1.0F : 0.0F;
				}
				break;
			}
			}

			pos.x -= 0.5F;
			pos.y -= 0.5F;
			pos.z -= 0.5F;
			rvec.x = gen->aux.rect.xx * pos.x + gen->aux.rect.yx * pos.y + gen->aux.rect.zx * pos.z;
			rvec.y = gen->aux.rect.xy * pos.x + gen->aux.rect.yy * pos.y + gen->aux.rect.zy * pos.z;
			rvec.z = gen->aux.rect.xz * pos.x + gen->aux.rect.yz * pos.y + gen->aux.rect.zz * pos.z;
			PSMTXMultVec(mtx, &rvec, &pos);
			pos.x += gen->pos.x;
			pos.y += gen->pos.y;
			pos.z += gen->pos.z;
			{
				f32 len   = msl_sqrtf(gen->aux.rect.zx * gen->aux.rect.zx + gen->aux.rect.zy * gen->aux.rect.zy
				                      + gen->aux.rect.zz * gen->aux.rect.zz);
				f32 scale = speed / len;
				vec.x     = gen->aux.rect.zx * scale;
				vec.y     = gen->aux.rect.zy * scale;
				vec.z     = gen->aux.rect.zz * scale;
			}
			PSMTXMultVec(mtx, &vec, &vec);
			psGenerateParticle(gen->linkNo, gen->bank, 0, gen->kind, gen->texGroup, gen->cmdList, gen->life, pos.x, pos.y, pos.z, vec.x,
			                   vec.y, vec.z, gen->size, gen->grav, gen->fric, 0, gen);
			break;

		case GEN_SHAPE_SPHERE: {
			f32 latitude;
			f32 longitude;
			f32 radius;

			if (0.0F == gen->aux.sphere.latRange) {
				latitude = M_PI * HSD_Randf();
			} else {
				latitude = gen->aux.sphere.latRange * HSD_Randf();
			}
			longitude = 2.0 * (M_PI * HSD_Randf());
			radius    = gen_randRadius(gen->radius);
			vec.x     = gen_sinf(latitude) * gen_cosf(longitude);
			vec.y     = gen_sinf(latitude) * gen_sinf(longitude);
			vec.z     = gen_cosf(latitude);
			PSMTXMultVec(mtx, &vec, &pos);
			vec.x = pos.x * gen->aux.sphere.speed;
			vec.y = pos.y * gen->aux.sphere.speed;
			vec.z = pos.z * gen->aux.sphere.speed;
			if (gen->radius >= 0.0F && gen->aux.sphere.speed < 0.0F) {
				f32 scale = radius / gen->radius;
				vec.x *= scale;
				vec.y *= scale;
				vec.z *= scale;
			}
			pos.x = radius * pos.x + gen->pos.x;
			pos.y = radius * pos.y + gen->pos.y;
			pos.z = radius * pos.z + gen->pos.z;
			psGenerateParticle(gen->linkNo, gen->bank, 0, gen->kind, gen->texGroup, gen->cmdList, gen->life, pos.x, pos.y, pos.z, vec.x,
			                   vec.y, vec.z, gen->size, gen->grav, gen->fric, 0, gen);
			break;
		}

		default:
			if (sCustomGeneratorEmit != NULL) {
				sCustomGeneratorEmit(gen, mtx);
			}
			break;
		}
		gen->count -= 1.0F;
	}
	return gen->count;
}

/**
 * @note Address: 0x801E3FE0
 * @note Size: 0x3A0
 */
void psExecGenerator(u32 mask)
{
	HSD_Generator* gen;
	HSD_JObj* jobj;

	while (sQueuedJointGenerators != NULL) {
		gen = sQueuedJointGenerators->data;
		psApplyGeneratorJObj(gen);
		sQueuedJointGenerators = HSD_SListRemove(sQueuedJointGenerators);
		if (gen->jobj != NULL) {
			HSD_JObjUnref(gen->jobj);
			gen->jobj = NULL;
		}
	}

	gen = sActiveGenerators;
	sGeneratorPrev = NULL;
	while (gen != NULL) {
		if (mask & (1U << (gen->linkNo + PS_GENERATOR_LINK_MASK_SHIFT))) {
			sGeneratorPrev = gen;
			gen          = gen->next;
			continue;
		}
		if (gen->kind & PS_PARTICLE_PAUSED) {
			sGeneratorPrev = gen;
			gen          = gen->next;
			continue;
		}

		if ((gen->type & PS_GEN_USE_JOINT) && (jobj = gen->jobj) != NULL) {
			HSD_JObjSetupMatrix(jobj);
			if (gen->type & PS_GEN_COPY_JOINT_POSITION) {
				gen->pos.x = gen->jobj->mtx[0][3];
				gen->pos.y = gen->jobj->mtx[1][3];
				gen->pos.z = gen->jobj->mtx[2][3];
			}
			if (gen->type & PS_GEN_SRT_JOINT_POSITION) {
				if (gen->appsrt != NULL && gen->appsrt->gp == gen) {
					gen->appsrt->translate.x = gen->jobj->mtx[0][3];
					gen->appsrt->translate.y = gen->jobj->mtx[1][3];
					gen->appsrt->translate.z = gen->jobj->mtx[2][3];
				}
			}
			if (gen->type & PS_GEN_SRT_JOINT_SCALE) {
				if (gen->appsrt != NULL && gen->appsrt->gp == gen) {
					HSD_MtxGetScale(gen->jobj->mtx, &gen->appsrt->scale);
				}
			}
			if (gen->type & PS_GEN_SRT_JOINT_ROTATION) {
				HSD_MtxGetRotation(gen->jobj->mtx, &gen->appsrt->rot);
			}
		}

		if (gen->random < 0.0F) {
			gen->count -= gen->random;
		} else {
			gen->count += gen->random * HSD_Randf();
		}
		if (gen->count >= 1.0F) {
			gen->count = generateParticle(gen);
		}

		if (gen->genLife != 0 && --gen->genLife == 0) {
			gen_killNode(gen);
			if (sGeneratorPrev != NULL) {
				gen = sGeneratorPrev->next;
			} else {
				gen = sActiveGenerators;
			}
		} else {
			sGeneratorPrev = gen;
			gen          = gen->next;
		}
	}
}

static inline void gen_attachJoint(HSD_Generator* gen, HSD_JObj* jobj)
{
	gen->jobj = jobj;
	if (jobj != NULL) {
		ref_INC(jobj);
	}
}

static inline void gen_setJointMode(HSD_Generator* gen)
{
	gen->type |= (gen->kind & GEN_KIND_CAMERA_SRT)
	                 ? (PS_GEN_USE_JOINT | PS_GEN_USE_JOINT_ROTATION)
	                 : (PS_GEN_USE_JOINT | PS_GEN_USE_JOINT_ROTATION | PS_GEN_COPY_JOINT_POSITION);
}

/**
 * @note Address: 0x801E4380
 * @note Size: 0xAC
 */
HSD_Generator* psCreateJointGenerator(int linkNo, int bank, int kind, HSD_JObj* jobj)
{
	HSD_Generator* gen = psCreateGeneratorID(linkNo, bank, kind);

	if (gen == NULL) {
		return NULL;
	}
	gen_attachJoint(gen, jobj);
	gen_setJointMode(gen);
	return gen;
}

static inline HSD_Generator* gen_allocGenerator(void)
{
	HSD_Generator* gen;

	gen = sFreeGenerators;
	if (gen == NULL) {
		gen = NULL;
	} else {
		psGeneratorCount++;
		if (psGeneratorCount > psPeakGeneratorCount) {
			psPeakGeneratorCount = psGeneratorCount;
		}
		sFreeGenerators = gen->next;
		if (sGeneratorPrev == NULL || sGeneratorPrev->next == NULL) {
			if (sActiveGenerators == NULL) {
				gen->next = NULL;
				sActiveGenerators = gen;
			} else {
				gen->next = sActiveGenerators->next;
				sActiveGenerators->next = gen;
			}
		} else {
			gen->next = sGeneratorPrev->next->next;
			sGeneratorPrev->next->next = gen;
		}
		gen->idnum    = gen_nextId();
		gen->appsrt   = NULL;
		gen->userfunc = NULL;
	}
	return gen;
}

/**
 * @note Address: 0x801E442C
 * @note Size: 0x82C
 */
HSD_Generator* psCreateGeneratorID(int linkNo, int bank, int kind)
{
	HSD_Generator* gen;
	HSD_PSCmdList* command;
	HSD_PSTexGroup* tex;

	if (linkNo < 0 || linkNo >= 8) {
		return NULL;
	}
	command = psGetBankCommand(bank, kind);
	if (command == NULL) {
		return NULL;
	}

	gen = gen_allocGenerator();
	if (gen != NULL) {
		gen->type     = command->type;
		gen->bank     = bank;
		gen->linkNo   = linkNo;
		gen->kind     = command->kind;
		gen->texGroup = command->texGroup;
		gen->life     = command->life;
		gen->genLife  = command->genLife;
		gen->pos.z    = 0.0F;
		gen->pos.y    = 0.0F;
		gen->pos.x    = 0.0F;
		gen->vel.x    = command->vx;
		gen->vel.y    = command->vy;
		gen->vel.z    = command->vz;
		gen->grav     = command->grav;
		gen->fric     = command->fric;
		gen->size     = command->size;
		gen->cmdList  = command->cmdList;
		gen->radius   = command->radius;
		gen->angle    = command->angle;
		gen->random   = command->random;

		if (gen->kind & GEN_KIND_PRIME_COUNT) {
			if (gen->random < 0.0F) {
				gen->count = (1.0F + gen->random > FLT_EPSILON) ? 1.0F : 0.0F;
			} else {
				gen->count = 1.0F - FLT_EPSILON;
			}
		} else if (gen->random < 0.0F) {
			gen->count = 0.0F;
		} else {
			gen->count = HSD_Randf();
		}

		tex = psTexGroupArray[bank][gen->texGroup];
		if (tex != NULL && tex->palflag != 0) {
			gen->kind |= PS_PARTICLE_SHARED_PALETTE;
		}

		gen->jobj     = NULL;
		gen->numChild = 0;

		switch (gen->type & GEN_SHAPE_MASK) {
		case GEN_SHAPE_DISC:
		case GEN_SHAPE_DISC_RADIAL_SPEED:
		case GEN_SHAPE_DISC_UNIFORM:
			if (command->param1 == 0.0F && command->param2 == 0.0F) {
				gen->aux.disc.minAngle = 0.0F;
				gen->aux.disc.maxAngle = 6.2831855F;
			} else {
				gen->aux.disc.minAngle = command->param1;
				gen->aux.disc.maxAngle = command->param2;
			}
			break;
		case GEN_SHAPE_LINE:
			gen->aux.line.x2 = command->param1;
			gen->aux.line.y2 = command->param2;
			gen->aux.line.z2 = command->param3;
			break;
		case GEN_SHAPE_TORNADO:
			break;
		case GEN_SHAPE_CONE:
		case GEN_SHAPE_CYLINDER:
			if (command->param1 == 0.0F && command->param2 == 0.0F) {
				gen->aux.cone.minAngle = 0.0F;
				gen->aux.cone.maxAngle = 6.2831855F;
			} else {
				gen->aux.cone.minAngle = command->param1;
				gen->aux.cone.maxAngle = command->param2;
			}
			gen->aux.cone.height = command->param3;
			break;
		case GEN_SHAPE_BOX:
			gen->aux.rect.xx = gen->aux.rect.x = command->param1;
			gen->aux.rect.yy = gen->aux.rect.y = command->param2;
			gen->aux.rect.zz = gen->aux.rect.z = command->param3;
			gen->aux.rect.zy                   = 0.0F;
			gen->aux.rect.zx                   = 0.0F;
			gen->aux.rect.yz                   = 0.0F;
			gen->aux.rect.yx                   = 0.0F;
			gen->aux.rect.xz                   = 0.0F;
			gen->aux.rect.xy                   = 0.0F;
			gen->aux.rect.flag                 = 0;
			if (command->param1 < 0.0F) {
				gen->aux.rect.flag |= GEN_BOX_FACE_X;
			}
			if (command->param2 < 0.0F) {
				gen->aux.rect.flag |= GEN_BOX_FACE_Y;
			}
			if (command->param3 < 0.0F) {
				gen->aux.rect.flag |= GEN_BOX_FACE_Z;
			}
			break;
		case GEN_SHAPE_SPHERE: {
			f32 horiz;

			gen->aux.sphere.speed = msl_sqrtf(gen->vel.x * gen->vel.x + gen->vel.y * gen->vel.y + gen->vel.z * gen->vel.z);
			horiz                 = msl_sqrtf(gen->vel.x * gen->vel.x + gen->vel.z * gen->vel.z);
			if (horiz < MSL_FLT_MIN) {
				if (gen->vel.y >= 0.0F) {
					gen->aux.sphere.latMid = 1.5707964F;
				} else {
					gen->aux.sphere.latMid = -1.5707964F;
				}
			} else {
				gen->aux.sphere.latMid = gen_atan2f(gen->vel.y, horiz);
			}
			GEN_ANGLE(gen->aux.sphere.lonMid, gen->vel.z, gen->vel.x);
			gen->aux.sphere.latRange = command->param1;
			if (gen->aux.sphere.latRange < 0.0F) {
				gen->aux.sphere.latRange = -gen->aux.sphere.latRange;
				gen->aux.sphere.speed    = -gen->aux.sphere.speed;
			}
			gen->aux.sphere.lonRange = command->param2;
			break;
		}
		default:
			if (sCustomGeneratorInit != NULL) {
				sCustomGeneratorInit(gen);
			}
			break;
		}

		if (gen->kind & GEN_KIND_CAMERA_SRT) {
			gen->type |= PS_GEN_SRT_JOINT_POSITION;
			CreateGeneratorAppSRT(gen, 0);
			if (gen->appsrt != NULL) {
				gen->appsrt->useCameraRotation = 1;
				gen->appsrt->gp                = gen;
			}
		}
	}
	return gen;
}

/**
 * @note Address: 0x801E4C58
 * @note Size: 0xBC
 */
HSD_Generator* psCreateQueuedJointGenerator(int linkNo, int bank, int kind, HSD_JObj* jobj)
{
	HSD_Generator* gen = psCreateGeneratorID(linkNo, bank, kind);

	if (gen == NULL) {
		return NULL;
	}
	gen_attachJoint(gen, jobj);
	gen_setJointMode(gen);
	sQueuedJointGenerators = HSD_SListAppend(sQueuedJointGenerators, gen);
	return gen;
}

/**
 * @note Address: 0x801E4D14
 * @note Size: 0xEC
 */
static void psSetBillboardCamera(HSD_CObj* cobj)
{
	gen_cobjUnref(psCamera);
	if (cobj != NULL) {
		ref_INC(cobj);
	}
	psCamera = cobj;
}
