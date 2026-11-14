#include <global.h>
#include <string.h>

#include <dolphin/gx/gxtypes.h>
#include <dolphin/mtx/mtx.h>
#include <dolphin/mtx/vec.h>
#include <sysdolphin/cobj.h>
#include <sysdolphin/jobj.h>
#include <sysdolphin/list.h>
#include <sysdolphin/memory.h>
#include <sysdolphin/mtx.h>
#include <sysdolphin/object.h>
#include <sysdolphin/random.h>

#include <sysdolphin/particle.h>

extern int __float_nan[];
extern int __float_epsilon[];
extern int __float_min[];
#define NAN         (*(f32*)__float_nan)
#define FLT_EPSILON (*(f32*)__float_epsilon)
#define FLT_MIN     (*(f32*)__float_min)

#define M_PI   3.141592653589793
#define M_PI_2 1.5707963267948966

extern f32 __fabsf(f32 x);
extern f64 __fabs(f64 x);
extern f64 sin(f64 x);
extern f64 cos(f64 x);
extern f64 atan2(f64 y, f64 x);

// particle side
extern HSD_PSTexGroup** psTexGroupArray[];

// generator tables, one per bank
extern s32 psNumCmdList[64];
extern HSD_PSCmdList** psCmdListArray[64];

extern u16 lbl_8065BA84;
extern u16 lbl_8065BA86;
extern u16 lbl_8065BA8A;
extern u16 lbl_8065BA8C;
u16 sParticleGeneratorIdCounter = 0x100;
// .sbss comes out last declaration first
extern HSD_psAppSRT* freeAppSRT;
extern u32 lbl_8065BAB8;
void (*lbl_8065BAB0)(HSD_Generator* gen, Mtx mtx);
void (*lbl_8065BAAC)(HSD_Generator* gen);
HSD_CObj* psCamera;
HSD_Generator* lbl_8065BAA4;
HSD_Generator* lbl_8065BAA0;
HSD_Generator* lbl_8065BA9C;
HSD_SList* lbl_8065BA98;

char sPathGeneratorC[] = "generator.c";
char sGeneratorCameraCondition[] = "psCamera";

void psKillAllGenerator(void);
f32 generateParticle(HSD_Generator* gen);
void psApplyGeneratorJObj(HSD_Generator* gen);
HSD_Generator* psCreateGeneratorID(int linkNo, int bank, int kind);
void psSetBillboardCamera(HSD_CObj* cobj);
HSD_psAppSRT* CreateGeneratorAppSRT(HSD_Generator* gp, s32 status);
s32 psRemoveParticleAppSRT(HSD_Particle* pp);
s32 psRemoveGeneratorAppSRT(HSD_Generator* gp);

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

static inline int gen_fpclassifyf(f32 x)
{
	switch (*(int*)&x & 0x7F800000) {
	case 0x7F800000:
		if ((*(int*)&x & 0x7FFFFF) != 0) {
			return 1;
		}
		return 2;
	case 0:
		if ((*(int*)&x & 0x7FFFFF) != 0) {
			return 5;
		}
		return 3;
	}
	return 4;
}

// msl's sqrtf with the nan checks
static inline f32 gen_sqrtf(f32 x)
{
	static const f64 _half  = 0.5;
	static const f64 _three = 3.0;

	if (x > 0.0F) {
		f64 dx    = x;
		f64 guess = __frsqrte(dx);
		guess     = _half * guess * (_three - guess * guess * dx);
		guess     = _half * guess * (_three - guess * guess * dx);
		guess     = _half * guess * (_three - guess * guess * dx);
		return (f32)(dx * guess);
	} else if (x < 0.0) {
		return NAN;
	} else if (gen_fpclassifyf(x) == 1) {
		return NAN;
	}
	return x;
}

// dst = atan2(y, x), with x near zero giving plus or minus a right angle
#define GEN_ANGLE(dst, y, x)          \
	if (__fabsf(x) < FLT_MIN) {       \
		if ((y) >= 0.0F) {            \
			(dst) = 1.5707964F;       \
		} else {                      \
			(dst) = -1.5707964F;      \
		}                             \
	} else {                          \
		(dst) = gen_atan2f((y), (x)); \
	}

static inline f32 gen_randLat(HSD_Generator* gen)
{
	if (0.0F == gen->aux.sphere.latRange) {
		f32 angle = M_PI * HSD_Randf();
		return angle;
	} else {
		f32 angle = gen->aux.sphere.latRange * HSD_Randf();
		return angle;
	}
}

static inline f32 gen_randRadius(f32 radius)
{
	if (radius < 0.0F) {
		return -radius;
	}
	return radius * HSD_Randf();
}

static inline BOOL gen_ref_DEC(void* o)
{
	BOOL ret;
	if ((ret = (HSD_OBJ(o)->ref_count == HSD_OBJ_NOREF))) {
		return ret;
	}
	return HSD_OBJ(o)->ref_count-- == 0;
}

static inline void gen_hsdDelete(void* object)
{
	if (object != NULL) {
		HSD_CLASS_METHOD(object)->release((HSD_Class*)object);
		HSD_CLASS_METHOD(object)->destroy((HSD_Class*)object);
	}
}

static inline void gen_cobjUnref(HSD_CObj* cobj)
{
	if (cobj != NULL) {
		if (cobj != NULL && gen_ref_DEC(cobj)) {
			gen_hsdDelete(cobj);
		}
	}
}

// unlinks a dead generator, or parks it when children or a shared srt still need it
static inline void gen_killNode(HSD_Generator* gen)
{
	HSD_Generator* prev;
	HSD_psAppSRT* srt;

	prev = lbl_8065BA9C;
	if (gen->callback != NULL) {
		gen->callback(gen);
	}
	if (gen->type & 0x80) {
		psKillGeneratorChild(gen);
	}
	if (gen->numChild != 0) {
		prev         = gen;
		gen->random  = 0.0F;
		gen->genLife = 1;
	} else if ((gen->type & 0x100) && gen->jobj != NULL && (gen->type & 0x3800) && (srt = gen->appsrt) != NULL && srt->gp == gen
	           && srt->usedCount != 1) {
		prev         = gen;
		gen->random  = 0.0F;
		gen->genLife = 1;
	} else {
		if (prev == NULL) {
			lbl_8065BAA0 = gen->next;
		} else {
			prev->next = gen->next;
		}
		if (gen->appsrt != NULL) {
			psRemoveGeneratorAppSRT(gen);
		}
		gen->next    = lbl_8065BAA4;
		lbl_8065BAA4 = gen;
		lbl_8065BA8C--;
		if (gen->jobj != NULL) {
			HSD_JObjUnref(gen->jobj);
			gen->jobj = NULL;
		}
	}
	lbl_8065BA9C = prev;
}

static inline void gen_kill(HSD_Generator* gen)
{
	HSD_Generator* cur;

	cur          = lbl_8065BAA0;
	lbl_8065BA9C = NULL;
	while (cur != NULL) {
		if (cur == gen) {
			gen_killNode(gen);
			if (lbl_8065BA9C != NULL) {
				while (lbl_8065BA9C->next != NULL) {
					lbl_8065BA9C = lbl_8065BA9C->next;
				}
			} else if (lbl_8065BAA0 != NULL) {
				lbl_8065BA9C = lbl_8065BAA0;
				while (lbl_8065BA9C->next != NULL) {
					lbl_8065BA9C = lbl_8065BA9C->next;
				}
			}
			return;
		}
		lbl_8065BA9C = cur;
		cur          = cur->next;
	}
}

/**
 * @note Address: 0x801E2434
 * @note Size: 0x28
 */
u16 psGetNewIDNum(void)
{
	if (++sParticleGeneratorIdCounter < 0x100) {
		sParticleGeneratorIdCounter = 0x100;
	}
	return sParticleGeneratorIdCounter;
}

/**
 * @note Address: 0x801E245C
 * @note Size: 0x98
 */
void psInitGenerator(s32 num)
{
	HSD_Generator* gp;
	s32 i;

	lbl_8065BAA0 = NULL;
	lbl_8065BAA4 = NULL;
	for (i = num - 1; i >= 0; i--) {
		gp = HSD_Alloc(sizeof(HSD_Generator));
		memset(gp, 0, sizeof(HSD_Generator));
		if (gp == NULL) {
			return;
		}
		gp->next     = lbl_8065BAA4;
		lbl_8065BAA4 = gp;
	}
	lbl_8065BA8C = 0;
	lbl_8065BA86 = 0;
	lbl_8065BA98 = NULL;
	psCamera     = NULL;
	lbl_8065BAB0 = NULL;
	lbl_8065BAAC = NULL;
	lbl_8065BA9C = NULL;
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
	for (gp = lbl_8065BAA4; gp != NULL; gp = next) {
		next = gp->next;
		HSD_Free(gp);
	}
	lbl_8065BAA4 = NULL;
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

	for (gen = lbl_8065BAA0; gen != NULL; gen = next) {
		next = gen->next;
		gen_kill(gen);
	}
	while (lbl_8065BA98 != NULL) {
		lbl_8065BA98 = HSD_SListRemove(lbl_8065BA98);
	}
}

/**
 * @note Address: 0x801E28F8
 * @note Size: 0x2DC
 */
void psApplyGeneratorJObj(HSD_Generator* gen)
{
	HSD_JObj* jobj;
	Vec col;
	f32 c0x, c0y, c0z;
	f32 c1x, c1y, c1z;

	if (!(gen->type & 0x100)) {
		return;
	}
	if ((jobj = gen->jobj) == NULL) {
		return;
	}
	HSD_JObjSetupMatrix(jobj);
	if (gen->type & 0x200) {
		gen->pos.x = gen->jobj->mtx[0][3];
		gen->pos.y = gen->jobj->mtx[1][3];
		gen->pos.z = gen->jobj->mtx[2][3];
	}
	if (gen->type & 0x800) {
		if (gen->appsrt != NULL && gen->appsrt->gp == gen) {
			gen->appsrt->translate.x = gen->jobj->mtx[0][3];
			gen->appsrt->translate.y = gen->jobj->mtx[1][3];
			gen->appsrt->translate.z = gen->jobj->mtx[2][3];
		}
	}
	if (gen->type & 0x1000) {
		if (gen->appsrt != NULL && gen->appsrt->gp == gen) {
			HSD_MtxGetScale(gen->jobj->mtx, &gen->appsrt->scale);
		}
	}

	col.x = jobj->mtx[0][0];
	col.y = jobj->mtx[1][0];
	col.z = jobj->mtx[2][0];
	PSVECNormalize(&col, &col);
	c0x = col.x;
	c0y = col.y;
	c0z = col.z;

	col.x = jobj->mtx[0][1];
	col.y = jobj->mtx[1][1];
	col.z = jobj->mtx[2][1];
	PSVECNormalize(&col, &col);
	c1x = col.x;
	c1y = col.y;
	c1z = col.z;

	col.x = jobj->mtx[0][2];
	col.y = jobj->mtx[1][2];
	col.z = jobj->mtx[2][2];
	PSVECNormalize(&col, &col);

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

	if ((gen->type & 0xF) == 1) {
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
f32 generateParticle(HSD_Generator* gen)
{
	// NONMATCHING: scalar registers, a latitude-result move and pooled diagnostic strings
	Vec vel;
	Vec pos;
	Vec rvec;
	Vec vec;
	Mtx mtx;
	Mtx jmtx;
	Vec look;
	Vec up;
	Vec right;
	Vec dir;
	Mtx rot;
	f32 speed;
	f32 rx;
	f32 ry;
	f32 sa;
	f32 ca;
	f32 se;
	f32 step;
	f32 cur;
	f32 theta;
	f32 elev;
	f32 r;

	ry = rx = 0.0F;
	if (gen->count < 1.0F) {
		return gen->count;
	}

	vel.x = gen->vel.x;
	vel.y = gen->vel.y;
	vel.z = gen->vel.z;
	speed = gen_sqrtf(vel.z * vel.z + (vel.x * vel.x + vel.y * vel.y));

	PSMTXIdentity(mtx);

	if ((gen->type & 0x100) && gen->jobj != NULL && (gen->type & 0x400) && !(gen->kind & 0x30000)) {
		PSMTXCopy(gen->jobj->mtx, jmtx);

		vec.x = jmtx[0][0];
		vec.y = jmtx[1][0];
		vec.z = jmtx[2][0];
		PSVECNormalize(&vec, &vec);
		mtx[0][0] = vec.x;
		mtx[1][0] = vec.y;
		mtx[2][0] = vec.z;

		vec.x = jmtx[0][1];
		vec.y = jmtx[1][1];
		vec.z = jmtx[2][1];
		PSVECNormalize(&vec, &vec);
		mtx[0][1] = vec.x;
		mtx[1][1] = vec.y;
		mtx[2][1] = vec.z;

		vec.x = jmtx[0][2];
		vec.y = jmtx[1][2];
		vec.z = jmtx[2][2];
		PSVECNormalize(&vec, &vec);
		mtx[0][2] = vec.x;
		mtx[1][2] = vec.y;
		mtx[2][2] = vec.z;

		mtx[2][3] = 0.0F;
		mtx[1][3] = 0.0F;
		mtx[0][3] = 0.0F;
	}

	if (gen->kind & 0x10000) {
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

	if ((gen->type & 0xF) != 1 && speed > FLT_EPSILON) {
		dir.x = gen->vel.x;
		dir.y = gen->vel.y;
		dir.z = gen->vel.z;
		PSVECNormalize(&dir, &dir);

		GEN_ANGLE(sa, dir.y, dir.z);
		ca = gen_sinf(sa);
		sa = gen_cosf(sa);
		GEN_ANGLE(elev, dir.x, dir.y * ca + dir.z * sa);
		se = gen_sinf(elev);
		{
			f32 ce  = gen_cosf(elev);
			f32 nca = -ca;

			rot[0][0] = ce;
			rot[0][1] = 0.0F;
			rot[0][2] = se;
			rot[0][3] = 0.0F;
			rot[1][0] = nca * se;
			rot[1][1] = sa;
			rot[1][2] = ca * ce;
			rot[1][3] = 0.0F;
			rot[2][0] = -sa * se;
			rot[2][1] = nca;
			rot[2][2] = sa * ce;
			rot[2][3] = 0.0F;
		}
		PSMTXConcat(mtx, rot, mtx);
	}
	elev = speed;

	if ((gen->type & 0xF) == 2) {
		f32 len;

		GEN_ANGLE(rx, mtx[1][2], mtx[2][2]);
		len = mtx[1][2] * gen_sinf(rx) + mtx[2][2] * gen_cosf(rx);
		GEN_ANGLE(ry, mtx[0][2], len);
	}

	if (gen->angle < 0.0F) {
		switch (gen->type & 0xF) {
		case 0:
		case 3:
		case 4: {
			f32 min = gen->aux.disc.minAngle;
			cur     = (gen->aux.disc.maxAngle - min) / (f32)(s32)gen->count * HSD_Randf() + min;
			step    = (gen->aux.disc.maxAngle - min) / (f32)(s32)gen->count;
			break;
		}
		case 6:
		case 7: {
			f32 min = gen->aux.cone.minAngle;
			cur     = (gen->aux.cone.maxAngle - min) / (f32)(s32)gen->count * HSD_Randf() + min;
			step    = (gen->aux.cone.maxAngle - min) / (f32)(s32)gen->count;
			break;
		}
		default:
			cur  = 2.0 * (M_PI * HSD_Randf());
			step = (2.0 * M_PI) / (f64)(s32)gen->count;
			break;
		}
	}

	while (gen->count >= 1.0F) {
		switch (gen->type & 0xF) {
		case 0:
		case 3:
		case 4:
		case 6:
		case 7:
			if (gen->radius < 0.0F) {
				sa = -gen->radius;
				r  = 1.0F;
			} else {
				r = HSD_Randf();
				if ((gen->type & 0xF) == 3 || (gen->type & 0xF) == 4) {
					r = gen_sqrtf(r);
				}
				sa = r * gen->radius;
			}

			switch (gen->type & 0xF) {
			case 6:
				if (gen->angle < 0.0F) {
					cur += step;
					if (__fabsf(sa) < FLT_MIN) {
						if (gen->aux.cone.height >= 0.0F) {
							theta = -gen->angle;
						} else {
							theta = M_PI - gen->angle;
						}
					} else {
						theta = M_PI_2 - gen_atan2f(gen->aux.cone.height, sa) - gen->angle;
					}
				} else {
					f32 min = gen->aux.cone.minAngle;
					cur     = (gen->aux.cone.maxAngle - min) * HSD_Randf() + min;
					if (__fabsf(sa) < FLT_MIN) {
						if (gen->aux.cone.height >= 0.0F) {
							theta = gen->angle;
						} else {
							theta = M_PI + gen->angle;
						}
					} else {
						theta = gen->angle + (M_PI_2 - gen_atan2f(gen->aux.cone.height, sa));
					}
				}
				break;
			case 7:
				if (gen->angle < 0.0F) {
					cur += step;
					theta = M_PI_2 - gen->angle;
				} else {
					f32 min = gen->aux.disc.minAngle;
					cur     = (gen->aux.disc.maxAngle - min) * HSD_Randf() + min;
					theta   = M_PI_2 + gen->angle;
				}
				break;
			default:
				if (gen->angle < 0.0F) {
					cur += step;
					theta = r * -gen->angle;
				} else {
					f32 min = gen->aux.disc.minAngle;
					cur     = (gen->aux.disc.maxAngle - min) * HSD_Randf() + min;
					theta   = r * gen->angle;
				}
				break;
			}

			pos.x = sa * gen_cosf(cur);
			pos.y = sa * gen_sinf(cur);
			if ((gen->type & 0xF) == 6 || (gen->type & 0xF) == 7) {
				pos.z = HSD_Randf();
				if ((gen->type & 0xF) == 6) {
					pos.x *= 1.0F - pos.z;
					pos.y *= 1.0F - pos.z;
				}
				pos.z *= gen->aux.cone.height;
			} else {
				pos.z = 0.0F;
			}

			{
				f32 st = elev * gen_sinf(theta);
				vec.x  = st * gen_cosf(cur);
				vec.y  = st * gen_sinf(cur);
				vec.z  = elev * gen_cosf(theta);
			}
			if ((gen->type & 0xF) == 3) {
				vec.x *= r;
				vec.y *= r;
				vec.z *= r;
			}

			PSMTXMultVec(mtx, &pos, &pos);
			pos.x += gen->pos.x;
			pos.y += gen->pos.y;
			pos.z += gen->pos.z;
			PSMTXMultVec(mtx, &vec, &vec);
			psGenerateParticle(gen->linkNo, gen->bank, gen->kind, gen->texGroup, gen->cmdList, gen->life, pos.x, pos.y, pos.z, vec.x,
			                   vec.y, vec.z, gen->size, gen->grav, gen->fric, 0, gen);
			break;

		case 1: {
			f32 rnd = HSD_Randf();
			pos.x   = rnd * gen->aux.line.x2;
			pos.y   = rnd * gen->aux.line.y2;
			pos.z   = rnd * gen->aux.line.z2;
			PSMTXMultVec(mtx, &pos, &pos);
			pos.x += gen->pos.x;
			pos.y += gen->pos.y;
			pos.z += gen->pos.z;
			PSMTXMultVec(mtx, &vel, &vec);
			psGenerateParticle(gen->linkNo, gen->bank, gen->kind, gen->texGroup, gen->cmdList, gen->life, pos.x, pos.y, pos.z, vec.x,
			                   vec.y, vec.z, gen->size, gen->grav, gen->fric, 0, gen);
			break;
		}

		case 2:
			if (gen->radius < 0.0F) {
				sa = 1.0F;
			} else {
				sa = HSD_Randf();
			}
			if (gen->angle < 0.0F) {
				cur += step;
			} else {
				cur = 2.0 * (M_PI * HSD_Randf());
			}
			gen->aux.line.x2 = elev;
			psGenerateParticle(gen->linkNo, gen->bank, gen->kind | 4, gen->texGroup, gen->cmdList, gen->life, 0.0F, 0.0F, 0.0F, cur, sa,
			                   0.0F, gen->size, rx, ry, 0, gen);
			break;

		case 5:
			pos.x = HSD_Randf();
			pos.y = HSD_Randf();
			pos.z = HSD_Randf();
			switch (gen->aux.rect.flag) {
			case 0:
				break;
			case 1:
				pos.x = pos.x > 0.5F ? 1.0F : 0.0F;
				break;
			case 2:
				pos.y = pos.y > 0.5F ? 1.0F : 0.0F;
				break;
			case 3: {
				f32 rnd = HSD_Randf();
				f32 a   = gen->aux.rect.x;
				if (rnd > a / (a + gen->aux.rect.y)) {
					pos.y = pos.y > 0.5F ? 1.0F : 0.0F;
				} else {
					pos.x = pos.x > 0.5F ? 1.0F : 0.0F;
				}
				break;
			}
			case 4:
				pos.z = pos.z > 0.5F ? 1.0F : 0.0F;
				break;
			case 5: {
				f32 rnd = HSD_Randf();
				f32 a   = gen->aux.rect.x;
				if (rnd > a / (a + gen->aux.rect.z)) {
					pos.z = pos.z > 0.5F ? 1.0F : 0.0F;
				} else {
					pos.x = pos.x > 0.5F ? 1.0F : 0.0F;
				}
				break;
			}
			case 6: {
				f32 rnd = HSD_Randf();
				f32 a   = gen->aux.rect.y;
				if (rnd > a / (a + gen->aux.rect.z)) {
					pos.z = pos.z > 0.5F ? 1.0F : 0.0F;
				} else {
					pos.y = pos.y > 0.5F ? 1.0F : 0.0F;
				}
				break;
			}
			case 7: {
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
				f32 len   = gen_sqrtf(gen->aux.rect.zx * gen->aux.rect.zx + gen->aux.rect.zy * gen->aux.rect.zy
				                      + gen->aux.rect.zz * gen->aux.rect.zz);
				f32 scale = elev / len;
				vec.x     = gen->aux.rect.zx * scale;
				vec.y     = gen->aux.rect.zy * scale;
				vec.z     = gen->aux.rect.zz * scale;
			}
			PSMTXMultVec(mtx, &vec, &vec);
			psGenerateParticle(gen->linkNo, gen->bank, gen->kind, gen->texGroup, gen->cmdList, gen->life, pos.x, pos.y, pos.z, vec.x,
			                   vec.y, vec.z, gen->size, gen->grav, gen->fric, 0, gen);
			break;

		case 8:
			r     = gen_randLat(gen);
			theta = 2.0 * (M_PI * HSD_Randf());
			sa    = gen_randRadius(gen->radius);
			vec.x = gen_sinf(r) * gen_cosf(theta);
			vec.y = gen_sinf(r) * gen_sinf(theta);
			vec.z = gen_cosf(r);
			PSMTXMultVec(mtx, &vec, &pos);
			vec.x = pos.x * gen->aux.sphere.speed;
			vec.y = pos.y * gen->aux.sphere.speed;
			vec.z = pos.z * gen->aux.sphere.speed;
			if (gen->radius >= 0.0F && gen->aux.sphere.speed < 0.0F) {
				f32 scale = sa / gen->radius;
				vec.x *= scale;
				vec.y *= scale;
				vec.z *= scale;
			}
			pos.x = sa * pos.x + gen->pos.x;
			pos.y = sa * pos.y + gen->pos.y;
			pos.z = sa * pos.z + gen->pos.z;
			psGenerateParticle(gen->linkNo, gen->bank, gen->kind, gen->texGroup, gen->cmdList, gen->life, pos.x, pos.y, pos.z, vec.x,
			                   vec.y, vec.z, gen->size, gen->grav, gen->fric, 0, gen);
			break;

		default:
			if (lbl_8065BAB0 != NULL) {
				lbl_8065BAB0(gen, mtx);
			}
			break;
		}
		gen->count -= 1.0F;
	}
	return gen->count;
}
/*
    stwu     r1, -0x1d0(r1)
    mflr     r0
    stw      r0, 0x1d4(r1)
    stfd     f31, 0x1c0(r1)
    psq_st   f31, 0x1c8(r1), 0, qr0
    stfd     f30, 0x1b0(r1)
    psq_st   f30, 0x1b8(r1), 0, qr0
    stfd     f29, 0x1a0(r1)
    psq_st   f29, 0x1a8(r1), 0, qr0
    stfd     f28, 0x190(r1)
    psq_st   f28, 0x198(r1), 0, qr0
    stfd     f27, 0x180(r1)
    psq_st   f27, 0x188(r1), 0, qr0
    stfd     f26, 0x170(r1)
    psq_st   f26, 0x178(r1), 0, qr0
    stfd     f25, 0x160(r1)
    psq_st   f25, 0x168(r1), 0, qr0
    stfd     f24, 0x150(r1)
    psq_st   f24, 0x158(r1), 0, qr0
    stfd     f23, 0x140(r1)
    psq_st   f23, 0x148(r1), 0, qr0
    stfd     f22, 0x130(r1)
    psq_st   f22, 0x138(r1), 0, qr0
    stw      r31, 0x12c(r1)
    mr       r31, r3
    lfs      f28, lbl_8065FDD8@sda21(r0)
    lfs      f1, 0xc(r3)
    lfs      f0, lbl_8065FDDC@sda21(r0)
    fmr      f27, f28
    fcmpo    cr0, f1, f0
    bge      .L_801E2C54
    b        .L_801E3F7C
.L_801E2C54:
    lfs      f0, 0x30(r31)
    stfs     f0, 0x70(r1)
    fmuls    f1, f0, f0
    lfs      f0, 0x34(r31)
    stfs     f0, 0x74(r1)
    fmuls    f0, f0, f0
    lfs      f2, 0x38(r31)
    fadds    f0, f1, f0
    fmuls    f1, f2, f2
    stfs     f2, 0x78(r1)
    fadds    f29, f1, f0
    fcmpo    cr0, f29, f28
    ble      .L_801E2CD0
    frsqrte  f1, f29
    lfd      f3, lbl_8065FDE0@sda21(r0)
    lfd      f2, lbl_8065FDE8@sda21(r0)
    fmul     f0, f1, f1
    fmul     f1, f3, f1
    fnmsub   f0, f29, f0, f2
    fmul     f1, f1, f0
    fmul     f0, f1, f1
    fmul     f1, f3, f1
    fnmsub   f0, f29, f0, f2
    fmul     f1, f1, f0
    fmul     f0, f1, f1
    fmul     f1, f3, f1
    fnmsub   f0, f29, f0, f2
    fmul     f0, f1, f0
    fmul     f29, f29, f0
    frsp     f29, f29
    b        .L_801E2D54
.L_801E2CD0:
    lfd      f0, lbl_8065FDF0@sda21(r0)
    fcmpo    cr0, f29, f0
    bge      .L_801E2CE8
    lis      r3, __float_nan@ha
    lfs      f29, __float_nan@l(r3)
    b        .L_801E2D54
.L_801E2CE8:
    stfs     f29, 0x18(r1)
    lis      r0, 0x7f80
    lwz      r4, 0x18(r1)
    rlwinm   r3, r4, 0, 1, 8
    cmpw     r3, r0
    beq      .L_801E2D10
    bge      .L_801E2D40
    cmpwi    r3, 0x0
    beq      .L_801E2D28
    b        .L_801E2D40
.L_801E2D10:
    clrlwi.  r0, r4, 9
    beq      .L_801E2D20
    li       r0, 0x1
    b        .L_801E2D44
.L_801E2D20:
    li       r0, 0x2
    b        .L_801E2D44
.L_801E2D28:
    clrlwi.  r0, r4, 9
    beq      .L_801E2D38
    li       r0, 0x5
    b        .L_801E2D44
.L_801E2D38:
    li       r0, 0x3
    b        .L_801E2D44
.L_801E2D40:
    li       r0, 0x4
.L_801E2D44:
    cmpwi    r0, 0x1
    bne      .L_801E2D54
    lis      r3, __float_nan@ha
    lfs      f29, __float_nan@l(r3)
.L_801E2D54:
    addi     r3, r1, 0xdc
    bl       PSMTXIdentity
    lhz      r3, 0x16(r31)
    rlwinm.  r0, r3, 0, 23, 23
    beq      .L_801E2E58
    lwz      r4, 0x10(r31)
    cmplwi   r4, 0x0
    beq      .L_801E2E58
    rlwinm.  r0, r3, 0, 21, 21
    beq      .L_801E2E58
    lwz      r0, 0x4(r31)
    rlwinm.  r0, r0, 0, 14, 15
    bne      .L_801E2E58
    addi     r3, r4, 0x44
    addi     r4, r1, 0xac
    bl       PSMTXCopy
    lfs      f2, 0xac(r1)
    addi     r3, r1, 0x4c
    lfs      f1, 0xbc(r1)
    mr       r4, r3
    lfs      f0, 0xcc(r1)
    stfs     f2, 0x4c(r1)
    stfs     f1, 0x50(r1)
    stfs     f0, 0x54(r1)
    bl       PSVECNormalize
    lfs      f5, 0x4c(r1)
    addi     r3, r1, 0x4c
    lfs      f4, 0x50(r1)
    mr       r4, r3
    lfs      f3, 0x54(r1)
    lfs      f2, 0xb0(r1)
    lfs      f1, 0xc0(r1)
    lfs      f0, 0xd0(r1)
    stfs     f5, 0xdc(r1)
    stfs     f4, 0xec(r1)
    stfs     f3, 0xfc(r1)
    stfs     f2, 0x4c(r1)
    stfs     f1, 0x50(r1)
    stfs     f0, 0x54(r1)
    bl       PSVECNormalize
    lfs      f5, 0x4c(r1)
    addi     r3, r1, 0x4c
    lfs      f4, 0x50(r1)
    mr       r4, r3
    lfs      f3, 0x54(r1)
    lfs      f2, 0xb4(r1)
    lfs      f1, 0xc4(r1)
    lfs      f0, 0xd4(r1)
    stfs     f5, 0xe0(r1)
    stfs     f4, 0xf0(r1)
    stfs     f3, 0x100(r1)
    stfs     f2, 0x4c(r1)
    stfs     f1, 0x50(r1)
    stfs     f0, 0x54(r1)
    bl       PSVECNormalize
    lfs      f0, lbl_8065FDD8@sda21(r0)
    lfs      f3, 0x4c(r1)
    lfs      f2, 0x50(r1)
    lfs      f1, 0x54(r1)
    stfs     f3, 0xe4(r1)
    stfs     f2, 0xf4(r1)
    stfs     f1, 0x104(r1)
    stfs     f0, 0x108(r1)
    stfs     f0, 0xf8(r1)
    stfs     f0, 0xe8(r1)
.L_801E2E58:
    lwz      r0, 0x4(r31)
    rlwinm.  r0, r0, 0, 15, 15
    beq      .L_801E2F4C
    lwz      r0, psCamera@sda21(r0)
    cmplwi   r0, 0x0
    bne      .L_801E2E88
    lis      r3, sPathGeneratorC@ha
    lis      r5, sGeneratorCameraCondition@ha
    addi     r3, r3, sPathGeneratorC@l
    li       r4, 0x21a
    addi     r5, r5, sGeneratorCameraCondition@l
    bl       __assert
.L_801E2E88:
    lwz      r5, psCamera@sda21(r0)
    addi     r3, r1, 0x40
    lfs      f0, 0x24(r31)
    mr       r4, r3
    lwz      r5, 0x24(r5)
    lfs      f1, 0xc(r5)
    fsubs    f0, f1, f0
    stfs     f0, 0x40(r1)
    lfs      f1, 0x10(r5)
    lfs      f0, 0x28(r31)
    fsubs    f0, f1, f0
    stfs     f0, 0x44(r1)
    lfs      f1, 0x14(r5)
    lfs      f0, 0x2c(r31)
    fsubs    f0, f1, f0
    stfs     f0, 0x48(r1)
    bl       PSVECNormalize
    lwz      r3, psCamera@sda21(r0)
    addi     r4, r1, 0x34
    bl       HSD_CObjGetUpVector
    addi     r3, r1, 0x34
    mr       r4, r3
    bl       PSVECNormalize
    addi     r3, r1, 0x34
    addi     r4, r1, 0x40
    addi     r5, r1, 0x28
    bl       PSVECCrossProduct
    addi     r3, r1, 0x40
    addi     r4, r1, 0x28
    addi     r5, r1, 0x34
    bl       PSVECCrossProduct
    lfs      f8, 0x28(r1)
    lfs      f7, 0x2c(r1)
    lfs      f6, 0x30(r1)
    lfs      f5, 0x34(r1)
    lfs      f4, 0x38(r1)
    lfs      f3, 0x3c(r1)
    lfs      f2, 0x40(r1)
    lfs      f1, 0x44(r1)
    lfs      f0, 0x48(r1)
    stfs     f8, 0xdc(r1)
    stfs     f7, 0xec(r1)
    stfs     f6, 0xfc(r1)
    stfs     f5, 0xe0(r1)
    stfs     f4, 0xf0(r1)
    stfs     f3, 0x100(r1)
    stfs     f2, 0xe4(r1)
    stfs     f1, 0xf4(r1)
    stfs     f0, 0x104(r1)
.L_801E2F4C:
    lhz      r0, 0x16(r31)
    clrlwi   r0, r0, 28
    cmpwi    r0, 0x1
    beq      .L_801E30C0
    lis      r3, __float_epsilon@ha
    lfs      f0, __float_epsilon@l(r3)
    fcmpo    cr0, f29, f0
    ble      .L_801E30C0
    lfs      f0, 0x30(r31)
    addi     r3, r1, 0x1c
    mr       r4, r3
    stfs     f0, 0x1c(r1)
    lfs      f0, 0x34(r31)
    stfs     f0, 0x20(r1)
    lfs      f0, 0x38(r31)
    stfs     f0, 0x24(r1)
    bl       PSVECNormalize
    lfs      f2, 0x24(r1)
    lis      r3, __float_min@ha
    lfs      f0, __float_min@l(r3)
    fabs     f1, f2
    fcmpo    cr0, f1, f0
    bge      .L_801E2FCC
    lfs      f1, 0x20(r1)
    lfs      f0, lbl_8065FDD8@sda21(r0)
    fcmpo    cr0, f1, f0
    cror     eq, gt, eq
    bne      .L_801E2FC4
    lfs      f24, lbl_8065FDF8@sda21(r0)
    b        .L_801E2FDC
.L_801E2FC4:
    lfs      f24, lbl_8065FDFC@sda21(r0)
    b        .L_801E2FDC
.L_801E2FCC:
    lfs      f1, 0x20(r1)
    bl       atan2
    frsp     f0, f1
    fmr      f24, f0
.L_801E2FDC:
    fmr      f1, f24
    bl       sin
    frsp     f23, f1
    fmr      f1, f24
    bl       cos
    frsp     f25, f1
    lfs      f0, 0x24(r1)
    lfs      f2, 0x20(r1)
    lis      r3, __float_min@ha
    fmuls    f1, f0, f25
    lfs      f0, __float_min@l(r3)
    fmadds   f2, f2, f23, f1
    fabs     f1, f2
    fcmpo    cr0, f1, f0
    bge      .L_801E303C
    lfs      f1, 0x1c(r1)
    lfs      f0, lbl_8065FDD8@sda21(r0)
    fcmpo    cr0, f1, f0
    cror     eq, gt, eq
    bne      .L_801E3034
    lfs      f26, lbl_8065FDF8@sda21(r0)
    b        .L_801E304C
.L_801E3034:
    lfs      f26, lbl_8065FDFC@sda21(r0)
    b        .L_801E304C
.L_801E303C:
    lfs      f1, 0x1c(r1)
    bl       atan2
    frsp     f0, f1
    fmr      f26, f0
.L_801E304C:
    fmr      f1, f26
    bl       sin
    frsp     f24, f1
    fmr      f1, f26
    bl       cos
    frsp     f5, f1
    lfs      f4, lbl_8065FDD8@sda21(r0)
    fneg     f6, f23
    addi     r3, r1, 0xdc
    fneg     f0, f25
    stfs     f4, 0x80(r1)
    fmuls    f3, f6, f24
    stfs     f5, 0x7c(r1)
    fmuls    f2, f23, f5
    mr       r5, r3
    fmuls    f1, f0, f24
    stfs     f24, 0x84(r1)
    fmuls    f0, f25, f5
    stfs     f4, 0x88(r1)
    addi     r4, r1, 0x7c
    stfs     f3, 0x8c(r1)
    stfs     f25, 0x90(r1)
    stfs     f2, 0x94(r1)
    stfs     f4, 0x98(r1)
    stfs     f1, 0x9c(r1)
    stfs     f6, 0xa0(r1)
    stfs     f0, 0xa4(r1)
    stfs     f4, 0xa8(r1)
    bl       PSMTXConcat
.L_801E30C0:
    lhz      r0, 0x16(r31)
    clrlwi   r0, r0, 28
    cmpwi    r0, 0x2
    bne      .L_801E318C
    lfs      f2, 0x104(r1)
    lis      r3, __float_min@ha
    lfs      f0, __float_min@l(r3)
    fabs     f1, f2
    fcmpo    cr0, f1, f0
    bge      .L_801E310C
    lfs      f1, 0xf4(r1)
    lfs      f0, lbl_8065FDD8@sda21(r0)
    fcmpo    cr0, f1, f0
    cror     eq, gt, eq
    bne      .L_801E3104
    lfs      f28, lbl_8065FDF8@sda21(r0)
    b        .L_801E311C
.L_801E3104:
    lfs      f28, lbl_8065FDFC@sda21(r0)
    b        .L_801E311C
.L_801E310C:
    lfs      f1, 0xf4(r1)
    bl       atan2
    frsp     f0, f1
    fmr      f28, f0
.L_801E311C:
    fmr      f1, f28
    bl       cos
    frsp     f23, f1
    fmr      f1, f28
    bl       sin
    lfs      f0, 0x104(r1)
    frsp     f3, f1
    lfs      f2, 0xf4(r1)
    lis      r3, __float_min@ha
    fmuls    f1, f0, f23
    lfs      f0, __float_min@l(r3)
    fmadds   f2, f2, f3, f1
    fabs     f1, f2
    fcmpo    cr0, f1, f0
    bge      .L_801E317C
    lfs      f1, 0xe4(r1)
    lfs      f0, lbl_8065FDD8@sda21(r0)
    fcmpo    cr0, f1, f0
    cror     eq, gt, eq
    bne      .L_801E3174
    lfs      f27, lbl_8065FDF8@sda21(r0)
    b        .L_801E318C
.L_801E3174:
    lfs      f27, lbl_8065FDFC@sda21(r0)
    b        .L_801E318C
.L_801E317C:
    lfs      f1, 0xe4(r1)
    bl       atan2
    frsp     f0, f1
    fmr      f27, f0
.L_801E318C:
    lfs      f1, 0x4c(r31)
    lfs      f0, lbl_8065FDD8@sda21(r0)
    fcmpo    cr0, f1, f0
    bge      .L_801E32BC
    lhz      r0, 0x16(r31)
    clrlwi   r0, r0, 28
    cmpwi    r0, 0x5
    beq      .L_801E326C
    bge      .L_801E31C8
    cmpwi    r0, 0x0
    beq      .L_801E31D4
    blt      .L_801E326C
    cmpwi    r0, 0x3
    bge      .L_801E31D4
    b        .L_801E326C
.L_801E31C8:
    cmpwi    r0, 0x8
    bge      .L_801E326C
    b        .L_801E3220
.L_801E31D4:
    lfs      f23, 0x60(r31)
    bl       HSD_Randf
    lfs      f0, 0xc(r31)
    lis      r0, 0x4330
    lfs      f3, 0x64(r31)
    fctiwz   f0, f0
    stw      r0, 0x118(r1)
    lfd      f2, lbl_8065FE28@sda21(r0)
    fsubs    f3, f3, f23
    stfd     f0, 0x110(r1)
    lwz      r0, 0x114(r1)
    xoris    r0, r0, 0x8000
    stw      r0, 0x11c(r1)
    lfd      f0, 0x118(r1)
    fsubs    f0, f0, f2
    fdivs    f0, f3, f0
    fmr      f30, f0
    fmadds   f31, f0, f1, f23
    b        .L_801E32BC
.L_801E3220:
    lfs      f23, 0x60(r31)
    bl       HSD_Randf
    lfs      f0, 0xc(r31)
    lis      r0, 0x4330
    lfs      f3, 0x64(r31)
    fctiwz   f0, f0
    stw      r0, 0x110(r1)
    lfd      f2, lbl_8065FE28@sda21(r0)
    fsubs    f3, f3, f23
    stfd     f0, 0x118(r1)
    lwz      r0, 0x11c(r1)
    xoris    r0, r0, 0x8000
    stw      r0, 0x114(r1)
    lfd      f0, 0x110(r1)
    fsubs    f0, f0, f2
    fdivs    f0, f3, f0
    fmr      f30, f0
    fmadds   f31, f0, f1, f23
    b        .L_801E32BC
.L_801E326C:
    bl       HSD_Randf
    lfs      f0, 0xc(r31)
    lis      r0, 0x4330
    lfd      f3, lbl_8065FE08@sda21(r0)
    fctiwz   f0, f0
    stw      r0, 0x110(r1)
    lfd      f2, lbl_8065FE28@sda21(r0)
    fmul     f4, f3, f1
    lfd      f3, lbl_8065FE00@sda21(r0)
    stfd     f0, 0x118(r1)
    lfd      f1, lbl_8065FE10@sda21(r0)
    lwz      r0, 0x11c(r1)
    fmul     f31, f3, f4
    xoris    r0, r0, 0x8000
    stw      r0, 0x114(r1)
    frsp     f31, f31
    lfd      f0, 0x110(r1)
    fsub     f0, f0, f2
    fdiv     f30, f1, f0
    frsp     f30, f30
.L_801E32BC:
    lfs      f26, lbl_8065FDDC@sda21(r0)
    b        .L_801E3F6C
.L_801E32C4:
    lhz      r0, 0x16(r31)
    clrlwi   r0, r0, 28
    cmplwi   r0, 0x8
    bgt      .L_801E3F44
    lis      r3, jumptable_8025B410@ha
    slwi     r0, r0, 2
    addi     r3, r3, jumptable_8025B410@l
    lwzx     r0, r3, r0
    mtctr    r0
    bctr
.L_801E32EC:
    lfs      f1, 0x48(r31)
    lfs      f0, lbl_8065FDD8@sda21(r0)
    fcmpo    cr0, f1, f0
    bge      .L_801E3308
    fneg     f24, f1
    lfs      f23, lbl_8065FDDC@sda21(r0)
    b        .L_801E3414
.L_801E3308:
    bl       HSD_Randf
    lhz      r0, 0x16(r31)
    fmr      f23, f1
    clrlwi   r0, r0, 28
    cmpwi    r0, 0x3
    beq      .L_801E3328
    cmpwi    r0, 0x4
    bne      .L_801E340C
.L_801E3328:
    lfs      f0, lbl_8065FDD8@sda21(r0)
    fcmpo    cr0, f23, f0
    ble      .L_801E337C
    frsqrte  f1, f23
    lfd      f3, lbl_8065FDE0@sda21(r0)
    lfd      f2, lbl_8065FDE8@sda21(r0)
    fmul     f0, f1, f1
    fmul     f1, f3, f1
    fnmsub   f0, f23, f0, f2
    fmul     f1, f1, f0
    fmul     f0, f1, f1
    fmul     f1, f3, f1
    fnmsub   f0, f23, f0, f2
    fmul     f1, f1, f0
    fmul     f0, f1, f1
    fmul     f1, f3, f1
    fnmsub   f0, f23, f0, f2
    fmul     f0, f1, f0
    fmul     f0, f23, f0
    frsp     f0, f0
    b        .L_801E3408
.L_801E337C:
    lfd      f0, lbl_8065FDF0@sda21(r0)
    fcmpo    cr0, f23, f0
    bge      .L_801E3394
    lis      r3, __float_nan@ha
    lfs      f0, __float_nan@l(r3)
    b        .L_801E3408
.L_801E3394:
    stfs     f23, 0x14(r1)
    lis      r0, 0x7f80
    lwz      r4, 0x14(r1)
    rlwinm   r3, r4, 0, 1, 8
    cmpw     r3, r0
    beq      .L_801E33BC
    bge      .L_801E33EC
    cmpwi    r3, 0x0
    beq      .L_801E33D4
    b        .L_801E33EC
.L_801E33BC:
    clrlwi.  r0, r4, 9
    beq      .L_801E33CC
    li       r0, 0x1
    b        .L_801E33F0
.L_801E33CC:
    li       r0, 0x2
    b        .L_801E33F0
.L_801E33D4:
    clrlwi.  r0, r4, 9
    beq      .L_801E33E4
    li       r0, 0x5
    b        .L_801E33F0
.L_801E33E4:
    li       r0, 0x3
    b        .L_801E33F0
.L_801E33EC:
    li       r0, 0x4
.L_801E33F0:
    cmpwi    r0, 0x1
    bne      .L_801E3404
    lis      r3, __float_nan@ha
    lfs      f0, __float_nan@l(r3)
    b        .L_801E3408
.L_801E3404:
    fmr      f0, f23
.L_801E3408:
    fmr      f23, f0
.L_801E340C:
    lfs      f0, 0x48(r31)
    fmuls    f24, f23, f0
.L_801E3414:
    lhz      r0, 0x16(r31)
    clrlwi   r0, r0, 28
    cmpwi    r0, 0x7
    beq      .L_801E352C
    bge      .L_801E3578
    cmpwi    r0, 0x6
    bge      .L_801E3434
    b        .L_801E3578
.L_801E3434:
    lfs      f2, 0x4c(r31)
    lfs      f1, lbl_8065FDD8@sda21(r0)
    fcmpo    cr0, f2, f1
    bge      .L_801E34AC
    lis      r3, __float_min@ha
    fabs     f3, f24
    lfs      f0, __float_min@l(r3)
    fadds    f31, f31, f30
    fcmpo    cr0, f3, f0
    bge      .L_801E3484
    lfs      f0, 0x68(r31)
    fcmpo    cr0, f0, f1
    cror     eq, gt, eq
    bne      .L_801E3474
    fneg     f25, f2
    b        .L_801E35B4
.L_801E3474:
    lfd      f0, lbl_8065FE08@sda21(r0)
    fsub     f25, f0, f2
    frsp     f25, f25
    b        .L_801E35B4
.L_801E3484:
    fmr      f2, f24
    lfs      f1, 0x68(r31)
    bl       atan2
    frsp     f2, f1
    lfd      f1, lbl_8065FE18@sda21(r0)
    lfs      f0, 0x4c(r31)
    fsub     f1, f1, f2
    fsub     f25, f1, f0
    frsp     f25, f25
    b        .L_801E35B4
.L_801E34AC:
    lfs      f25, 0x60(r31)
    bl       HSD_Randf
    lfs      f0, 0x64(r31)
    lis      r3, __float_min@ha
    fabs     f3, f24
    fsubs    f2, f0, f25
    lfs      f0, __float_min@l(r3)
    fcmpo    cr0, f3, f0
    fmadds   f31, f2, f1, f25
    bge      .L_801E3504
    lfs      f1, 0x68(r31)
    lfs      f0, lbl_8065FDD8@sda21(r0)
    fcmpo    cr0, f1, f0
    cror     eq, gt, eq
    bne      .L_801E34F0
    lfs      f25, 0x4c(r31)
    b        .L_801E35B4
.L_801E34F0:
    lfd      f1, lbl_8065FE08@sda21(r0)
    lfs      f0, 0x4c(r31)
    fadd     f25, f1, f0
    frsp     f25, f25
    b        .L_801E35B4
.L_801E3504:
    fmr      f2, f24
    lfs      f1, 0x68(r31)
    bl       atan2
    frsp     f2, f1
    lfd      f0, lbl_8065FE18@sda21(r0)
    lfs      f1, 0x4c(r31)
    fsub     f0, f0, f2
    fadd     f25, f1, f0
    frsp     f25, f25
    b        .L_801E35B4
.L_801E352C:
    lfs      f1, 0x4c(r31)
    lfs      f0, lbl_8065FDD8@sda21(r0)
    fcmpo    cr0, f1, f0
    bge      .L_801E3550
    lfd      f0, lbl_8065FE18@sda21(r0)
    fadds    f31, f31, f30
    fsub     f25, f0, f1
    frsp     f25, f25
    b        .L_801E35B4
.L_801E3550:
    lfs      f31, 0x60(r31)
    bl       HSD_Randf
    lfs      f3, 0x64(r31)
    lfd      f2, lbl_8065FE18@sda21(r0)
    lfs      f0, 0x4c(r31)
    fsubs    f3, f3, f31
    fadd     f25, f2, f0
    fmadds   f31, f3, f1, f31
    frsp     f25, f25
    b        .L_801E35B4
.L_801E3578:
    lfs      f1, 0x4c(r31)
    lfs      f0, lbl_8065FDD8@sda21(r0)
    fcmpo    cr0, f1, f0
    bge      .L_801E3598
    fneg     f0, f1
    fadds    f31, f31, f30
    fmuls    f25, f23, f0
    b        .L_801E35B4
.L_801E3598:
    lfs      f31, 0x60(r31)
    bl       HSD_Randf
    lfs      f2, 0x64(r31)
    lfs      f0, 0x4c(r31)
    fsubs    f2, f2, f31
    fmuls    f25, f23, f0
    fmadds   f31, f2, f1, f31
.L_801E35B4:
    fmr      f1, f31
    bl       cos
    frsp     f0, f1
    fmr      f1, f31
    fmuls    f0, f24, f0
    stfs     f0, 0x64(r1)
    bl       sin
    frsp     f0, f1
    fmuls    f0, f24, f0
    stfs     f0, 0x68(r1)
    lhz      r0, 0x16(r31)
    clrlwi   r0, r0, 28
    cmpwi    r0, 0x6
    beq      .L_801E35F4
    cmpwi    r0, 0x7
    bne      .L_801E3644
.L_801E35F4:
    bl       HSD_Randf
    stfs     f1, 0x6c(r1)
    lhz      r0, 0x16(r31)
    clrlwi   r0, r0, 28
    cmpwi    r0, 0x6
    bne      .L_801E3630
    frsp     f1, f1
    lfs      f2, lbl_8065FDDC@sda21(r0)
    lfs      f3, 0x64(r1)
    lfs      f0, 0x68(r1)
    fsubs    f2, f2, f1
    fmuls    f1, f3, f2
    fmuls    f0, f0, f2
    stfs     f1, 0x64(r1)
    stfs     f0, 0x68(r1)
.L_801E3630:
    lfs      f1, 0x6c(r1)
    lfs      f0, 0x68(r31)
    fmuls    f0, f1, f0
    stfs     f0, 0x6c(r1)
    b        .L_801E364C
.L_801E3644:
    lfs      f0, lbl_8065FDD8@sda21(r0)
    stfs     f0, 0x6c(r1)
.L_801E364C:
    fmr      f1, f25
    bl       sin
    frsp     f0, f1
    fmr      f1, f31
    fmuls    f24, f29, f0
    bl       cos
    frsp     f0, f1
    fmr      f1, f31
    fmuls    f0, f24, f0
    stfs     f0, 0x4c(r1)
    bl       sin
    frsp     f0, f1
    fmr      f1, f25
    fmuls    f0, f24, f0
    stfs     f0, 0x50(r1)
    bl       cos
    frsp     f0, f1
    fmuls    f0, f29, f0
    stfs     f0, 0x54(r1)
    lhz      r0, 0x16(r31)
    clrlwi   r0, r0, 28
    cmpwi    r0, 0x3
    bne      .L_801E36C8
    lfs      f2, 0x4c(r1)
    fmuls    f0, f0, f23
    lfs      f1, 0x50(r1)
    fmuls    f2, f2, f23
    fmuls    f1, f1, f23
    stfs     f0, 0x54(r1)
    stfs     f2, 0x4c(r1)
    stfs     f1, 0x50(r1)
.L_801E36C8:
    addi     r4, r1, 0x64
    addi     r3, r1, 0xdc
    mr       r5, r4
    bl       PSMTXMultVec
    lfs      f1, 0x64(r1)
    addi     r4, r1, 0x4c
    lfs      f0, 0x24(r31)
    mr       r5, r4
    lfs      f2, 0x68(r1)
    addi     r3, r1, 0xdc
    fadds    f0, f1, f0
    lfs      f1, 0x6c(r1)
    stfs     f0, 0x64(r1)
    lfs      f0, 0x28(r31)
    fadds    f0, f2, f0
    stfs     f0, 0x68(r1)
    lfs      f0, 0x2c(r31)
    fadds    f0, f1, f0
    stfs     f0, 0x6c(r1)
    bl       PSMTXMultVec
    lfs      f0, 0x40(r31)
    li       r5, 0x0
    li       r10, 0x0
    stfs     f0, 0x8(r1)
    stw      r31, 0xc(r1)
    lbz      r3, 0x19(r31)
    lbz      r4, 0x18(r31)
    lwz      r6, 0x4(r31)
    lbz      r7, 0x1a(r31)
    lwz      r8, 0x20(r31)
    lhz      r9, 0x1e(r31)
    lfs      f1, 0x64(r1)
    lfs      f2, 0x68(r1)
    lfs      f3, 0x6c(r1)
    lfs      f4, 0x4c(r1)
    lfs      f5, 0x50(r1)
    lfs      f6, 0x54(r1)
    lfs      f7, 0x44(r31)
    lfs      f8, 0x3c(r31)
    bl       psGenerateParticle
    b        .L_801E3F60
.L_801E376C:
    bl       HSD_Randf
    lfs      f0, 0x60(r31)
    addi     r4, r1, 0x64
    mr       r5, r4
    addi     r3, r1, 0xdc
    fmuls    f0, f1, f0
    stfs     f0, 0x64(r1)
    lfs      f0, 0x64(r31)
    fmuls    f0, f1, f0
    stfs     f0, 0x68(r1)
    lfs      f0, 0x68(r31)
    fmuls    f0, f1, f0
    stfs     f0, 0x6c(r1)
    bl       PSMTXMultVec
    lfs      f1, 0x64(r1)
    addi     r3, r1, 0xdc
    lfs      f0, 0x24(r31)
    addi     r4, r1, 0x70
    lfs      f2, 0x68(r1)
    addi     r5, r1, 0x4c
    fadds    f0, f1, f0
    lfs      f1, 0x6c(r1)
    stfs     f0, 0x64(r1)
    lfs      f0, 0x28(r31)
    fadds    f0, f2, f0
    stfs     f0, 0x68(r1)
    lfs      f0, 0x2c(r31)
    fadds    f0, f1, f0
    stfs     f0, 0x6c(r1)
    bl       PSMTXMultVec
    lfs      f0, 0x40(r31)
    li       r5, 0x0
    li       r10, 0x0
    stfs     f0, 0x8(r1)
    stw      r31, 0xc(r1)
    lbz      r3, 0x19(r31)
    lbz      r4, 0x18(r31)
    lwz      r6, 0x4(r31)
    lbz      r7, 0x1a(r31)
    lwz      r8, 0x20(r31)
    lhz      r9, 0x1e(r31)
    lfs      f1, 0x64(r1)
    lfs      f2, 0x68(r1)
    lfs      f3, 0x6c(r1)
    lfs      f4, 0x4c(r1)
    lfs      f5, 0x50(r1)
    lfs      f6, 0x54(r1)
    lfs      f7, 0x44(r31)
    lfs      f8, 0x3c(r31)
    bl       psGenerateParticle
    b        .L_801E3F60
.L_801E3838:
    lfs      f1, 0x48(r31)
    lfs      f0, lbl_8065FDD8@sda21(r0)
    fcmpo    cr0, f1, f0
    bge      .L_801E3850
    lfs      f1, lbl_8065FDDC@sda21(r0)
    b        .L_801E3854
.L_801E3850:
    bl       HSD_Randf
.L_801E3854:
    lfs      f2, 0x4c(r31)
    fmr      f23, f1
    lfs      f0, lbl_8065FDD8@sda21(r0)
    fcmpo    cr0, f2, f0
    bge      .L_801E3870
    fadds    f4, f31, f30
    b        .L_801E3888
.L_801E3870:
    bl       HSD_Randf
    lfd      f2, lbl_8065FE08@sda21(r0)
    lfd      f0, lbl_8065FE00@sda21(r0)
    fmul     f1, f2, f1
    fmul     f4, f0, f1
    frsp     f4, f4
.L_801E3888:
    stfs     f29, 0x60(r31)
    fmr      f31, f4
    lfs      f1, lbl_8065FDD8@sda21(r0)
    fmr      f5, f23
    stfs     f27, 0x8(r1)
    fmr      f8, f28
    fmr      f2, f1
    stw      r31, 0xc(r1)
    fmr      f3, f1
    fmr      f6, f1
    li       r5, 0x0
    lwz      r0, 0x4(r31)
    lbz      r3, 0x19(r31)
    li       r10, 0x0
    lbz      r4, 0x18(r31)
    lbz      r7, 0x1a(r31)
    ori      r6, r0, 0x4
    lwz      r8, 0x20(r31)
    lhz      r9, 0x1e(r31)
    lfs      f7, 0x44(r31)
    bl       psGenerateParticle
    b        .L_801E3F60
.L_801E38E0:
    bl       HSD_Randf
    stfs     f1, 0x64(r1)
    bl       HSD_Randf
    stfs     f1, 0x68(r1)
    bl       HSD_Randf
    stfs     f1, 0x6c(r1)
    lhz      r0, 0x90(r31)
    cmplwi   r0, 0x7
    bgt      .L_801E3B54
    lis      r3, jumptable_8025B3F0@ha
    slwi     r0, r0, 2
    addi     r3, r3, jumptable_8025B3F0@l
    lwzx     r0, r3, r0
    mtctr    r0
    bctr
.L_801E391C:
    lfs      f1, 0x64(r1)
    lfs      f0, lbl_8065FE20@sda21(r0)
    fcmpo    cr0, f1, f0
    ble      .L_801E3934
    lfs      f0, lbl_8065FDDC@sda21(r0)
    b        .L_801E3938
.L_801E3934:
    lfs      f0, lbl_8065FDD8@sda21(r0)
.L_801E3938:
    stfs     f0, 0x64(r1)
    b        .L_801E3B54
.L_801E3940:
    lfs      f1, 0x68(r1)
    lfs      f0, lbl_8065FE20@sda21(r0)
    fcmpo    cr0, f1, f0
    ble      .L_801E3958
    lfs      f0, lbl_8065FDDC@sda21(r0)
    b        .L_801E395C
.L_801E3958:
    lfs      f0, lbl_8065FDD8@sda21(r0)
.L_801E395C:
    stfs     f0, 0x68(r1)
    b        .L_801E3B54
.L_801E3964:
    bl       HSD_Randf
    lfs      f2, 0x60(r31)
    lfs      f0, 0x64(r31)
    fadds    f0, f2, f0
    fdivs    f0, f2, f0
    fcmpo    cr0, f1, f0
    ble      .L_801E39A4
    lfs      f1, 0x68(r1)
    lfs      f0, lbl_8065FE20@sda21(r0)
    fcmpo    cr0, f1, f0
    ble      .L_801E3998
    lfs      f0, lbl_8065FDDC@sda21(r0)
    b        .L_801E399C
.L_801E3998:
    lfs      f0, lbl_8065FDD8@sda21(r0)
.L_801E399C:
    stfs     f0, 0x68(r1)
    b        .L_801E3B54
.L_801E39A4:
    lfs      f1, 0x64(r1)
    lfs      f0, lbl_8065FE20@sda21(r0)
    fcmpo    cr0, f1, f0
    ble      .L_801E39BC
    lfs      f0, lbl_8065FDDC@sda21(r0)
    b        .L_801E39C0
.L_801E39BC:
    lfs      f0, lbl_8065FDD8@sda21(r0)
.L_801E39C0:
    stfs     f0, 0x64(r1)
    b        .L_801E3B54
.L_801E39C8:
    frsp     f1, f1
    lfs      f0, lbl_8065FE20@sda21(r0)
    fcmpo    cr0, f1, f0
    ble      .L_801E39E0
    lfs      f0, lbl_8065FDDC@sda21(r0)
    b        .L_801E39E4
.L_801E39E0:
    lfs      f0, lbl_8065FDD8@sda21(r0)
.L_801E39E4:
    stfs     f0, 0x6c(r1)
    b        .L_801E3B54
.L_801E39EC:
    bl       HSD_Randf
    lfs      f2, 0x60(r31)
    lfs      f0, 0x68(r31)
    fadds    f0, f2, f0
    fdivs    f0, f2, f0
    fcmpo    cr0, f1, f0
    ble      .L_801E3A2C
    lfs      f1, 0x6c(r1)
    lfs      f0, lbl_8065FE20@sda21(r0)
    fcmpo    cr0, f1, f0
    ble      .L_801E3A20
    lfs      f0, lbl_8065FDDC@sda21(r0)
    b        .L_801E3A24
.L_801E3A20:
    lfs      f0, lbl_8065FDD8@sda21(r0)
.L_801E3A24:
    stfs     f0, 0x6c(r1)
    b        .L_801E3B54
.L_801E3A2C:
    lfs      f1, 0x64(r1)
    lfs      f0, lbl_8065FE20@sda21(r0)
    fcmpo    cr0, f1, f0
    ble      .L_801E3A44
    lfs      f0, lbl_8065FDDC@sda21(r0)
    b        .L_801E3A48
.L_801E3A44:
    lfs      f0, lbl_8065FDD8@sda21(r0)
.L_801E3A48:
    stfs     f0, 0x64(r1)
    b        .L_801E3B54
.L_801E3A50:
    bl       HSD_Randf
    lfs      f2, 0x64(r31)
    lfs      f0, 0x68(r31)
    fadds    f0, f2, f0
    fdivs    f0, f2, f0
    fcmpo    cr0, f1, f0
    ble      .L_801E3A90
    lfs      f1, 0x6c(r1)
    lfs      f0, lbl_8065FE20@sda21(r0)
    fcmpo    cr0, f1, f0
    ble      .L_801E3A84
    lfs      f0, lbl_8065FDDC@sda21(r0)
    b        .L_801E3A88
.L_801E3A84:
    lfs      f0, lbl_8065FDD8@sda21(r0)
.L_801E3A88:
    stfs     f0, 0x6c(r1)
    b        .L_801E3B54
.L_801E3A90:
    lfs      f1, 0x68(r1)
    lfs      f0, lbl_8065FE20@sda21(r0)
    fcmpo    cr0, f1, f0
    ble      .L_801E3AA8
    lfs      f0, lbl_8065FDDC@sda21(r0)
    b        .L_801E3AAC
.L_801E3AA8:
    lfs      f0, lbl_8065FDD8@sda21(r0)
.L_801E3AAC:
    stfs     f0, 0x68(r1)
    b        .L_801E3B54
.L_801E3AB4:
    bl       HSD_Randf
    lfs      f5, 0x68(r31)
    lfs      f0, 0x64(r31)
    lfs      f6, 0x60(r31)
    fadds    f3, f0, f5
    lfs      f4, lbl_8065FDDC@sda21(r0)
    fmuls    f2, f0, f5
    fmuls    f0, f6, f0
    fmadds   f2, f6, f3, f2
    fdivs    f2, f4, f2
    fmuls    f0, f2, f0
    fcmpo    cr0, f1, f0
    bge      .L_801E3B08
    lfs      f1, 0x6c(r1)
    lfs      f0, lbl_8065FE20@sda21(r0)
    fcmpo    cr0, f1, f0
    ble      .L_801E3AFC
    b        .L_801E3B00
.L_801E3AFC:
    lfs      f4, lbl_8065FDD8@sda21(r0)
.L_801E3B00:
    stfs     f4, 0x6c(r1)
    b        .L_801E3B54
.L_801E3B08:
    fmuls    f0, f6, f5
    fnmsubs  f0, f2, f0, f4
    fcmpo    cr0, f1, f0
    ble      .L_801E3B38
    lfs      f1, 0x68(r1)
    lfs      f0, lbl_8065FE20@sda21(r0)
    fcmpo    cr0, f1, f0
    ble      .L_801E3B2C
    b        .L_801E3B30
.L_801E3B2C:
    lfs      f4, lbl_8065FDD8@sda21(r0)
.L_801E3B30:
    stfs     f4, 0x68(r1)
    b        .L_801E3B54
.L_801E3B38:
    lfs      f1, 0x64(r1)
    lfs      f0, lbl_8065FE20@sda21(r0)
    fcmpo    cr0, f1, f0
    ble      .L_801E3B4C
    b        .L_801E3B50
.L_801E3B4C:
    lfs      f4, lbl_8065FDD8@sda21(r0)
.L_801E3B50:
    stfs     f4, 0x64(r1)
.L_801E3B54:
    lfs      f3, 0x64(r1)
    addi     r3, r1, 0xdc
    lfs      f2, lbl_8065FE20@sda21(r0)
    addi     r4, r1, 0x58
    lfs      f1, 0x68(r1)
    addi     r5, r1, 0x64
    lfs      f0, 0x6c(r1)
    fsubs    f5, f3, f2
    fsubs    f4, f1, f2
    fsubs    f3, f0, f2
    stfs     f5, 0x64(r1)
    stfs     f4, 0x68(r1)
    stfs     f3, 0x6c(r1)
    lfs      f0, 0x78(r31)
    lfs      f1, 0x6c(r31)
    fmuls    f0, f0, f4
    lfs      f2, 0x84(r31)
    fmadds   f0, f1, f5, f0
    fmadds   f0, f2, f3, f0
    stfs     f0, 0x58(r1)
    lfs      f0, 0x7c(r31)
    lfs      f1, 0x70(r31)
    fmuls    f0, f0, f4
    lfs      f2, 0x88(r31)
    fmadds   f0, f1, f5, f0
    fmadds   f0, f2, f3, f0
    stfs     f0, 0x5c(r1)
    lfs      f0, 0x80(r31)
    lfs      f1, 0x74(r31)
    fmuls    f0, f0, f4
    lfs      f2, 0x8c(r31)
    fmadds   f0, f1, f5, f0
    fmadds   f0, f2, f3, f0
    stfs     f0, 0x60(r1)
    bl       PSMTXMultVec
    lfs      f1, 0x64(r1)
    lfs      f0, 0x24(r31)
    lfs      f3, 0x68(r1)
    fadds    f1, f1, f0
    lfs      f2, 0x6c(r1)
    lfs      f0, lbl_8065FDD8@sda21(r0)
    stfs     f1, 0x64(r1)
    lfs      f1, 0x28(r31)
    fadds    f1, f3, f1
    stfs     f1, 0x68(r1)
    lfs      f1, 0x2c(r31)
    fadds    f1, f2, f1
    stfs     f1, 0x6c(r1)
    lfs      f2, 0x84(r31)
    lfs      f1, 0x88(r31)
    lfs      f3, 0x8c(r31)
    fmuls    f2, f2, f2
    fmuls    f1, f1, f1
    fmuls    f3, f3, f3
    fadds    f1, f2, f1
    fadds    f4, f3, f1
    fcmpo    cr0, f4, f0
    ble      .L_801E3C84
    frsqrte  f1, f4
    lfd      f3, lbl_8065FDE0@sda21(r0)
    lfd      f2, lbl_8065FDE8@sda21(r0)
    fmul     f0, f1, f1
    fmul     f1, f3, f1
    fnmsub   f0, f4, f0, f2
    fmul     f1, f1, f0
    fmul     f0, f1, f1
    fmul     f1, f3, f1
    fnmsub   f0, f4, f0, f2
    fmul     f1, f1, f0
    fmul     f0, f1, f1
    fmul     f1, f3, f1
    fnmsub   f0, f4, f0, f2
    fmul     f0, f1, f0
    fmul     f4, f4, f0
    frsp     f4, f4
    b        .L_801E3D08
.L_801E3C84:
    lfd      f0, lbl_8065FDF0@sda21(r0)
    fcmpo    cr0, f4, f0
    bge      .L_801E3C9C
    lis      r3, __float_nan@ha
    lfs      f4, __float_nan@l(r3)
    b        .L_801E3D08
.L_801E3C9C:
    stfs     f4, 0x10(r1)
    lis      r0, 0x7f80
    lwz      r4, 0x10(r1)
    rlwinm   r3, r4, 0, 1, 8
    cmpw     r3, r0
    beq      .L_801E3CC4
    bge      .L_801E3CF4
    cmpwi    r3, 0x0
    beq      .L_801E3CDC
    b        .L_801E3CF4
.L_801E3CC4:
    clrlwi.  r0, r4, 9
    beq      .L_801E3CD4
    li       r0, 0x1
    b        .L_801E3CF8
.L_801E3CD4:
    li       r0, 0x2
    b        .L_801E3CF8
.L_801E3CDC:
    clrlwi.  r0, r4, 9
    beq      .L_801E3CEC
    li       r0, 0x5
    b        .L_801E3CF8
.L_801E3CEC:
    li       r0, 0x3
    b        .L_801E3CF8
.L_801E3CF4:
    li       r0, 0x4
.L_801E3CF8:
    cmpwi    r0, 0x1
    bne      .L_801E3D08
    lis      r3, __float_nan@ha
    lfs      f4, __float_nan@l(r3)
.L_801E3D08:
    fdivs    f1, f29, f4
    lfs      f0, 0x84(r31)
    addi     r4, r1, 0x4c
    addi     r3, r1, 0xdc
    mr       r5, r4
    fmuls    f0, f0, f1
    stfs     f0, 0x4c(r1)
    lfs      f0, 0x88(r31)
    fmuls    f0, f0, f1
    stfs     f0, 0x50(r1)
    lfs      f0, 0x8c(r31)
    fmuls    f0, f0, f1
    stfs     f0, 0x54(r1)
    bl       PSMTXMultVec
    lfs      f0, 0x40(r31)
    li       r5, 0x0
    li       r10, 0x0
    stfs     f0, 0x8(r1)
    stw      r31, 0xc(r1)
    lbz      r3, 0x19(r31)
    lbz      r4, 0x18(r31)
    lwz      r6, 0x4(r31)
    lbz      r7, 0x1a(r31)
    lwz      r8, 0x20(r31)
    lhz      r9, 0x1e(r31)
    lfs      f1, 0x64(r1)
    lfs      f2, 0x68(r1)
    lfs      f3, 0x6c(r1)
    lfs      f4, 0x4c(r1)
    lfs      f5, 0x50(r1)
    lfs      f6, 0x54(r1)
    lfs      f7, 0x44(r31)
    lfs      f8, 0x3c(r31)
    bl       psGenerateParticle
    b        .L_801E3F60
.L_801E3D94:
    lfs      f1, lbl_8065FDD8@sda21(r0)
    lfs      f0, 0x68(r31)
    fcmpu    cr0, f1, f0
    bne      .L_801E3DB8
    bl       HSD_Randf
    lfd      f0, lbl_8065FE08@sda21(r0)
    fmul     f0, f0, f1
    frsp     f0, f0
    b        .L_801E3DC4
.L_801E3DB8:
    bl       HSD_Randf
    lfs      f0, 0x68(r31)
    fmuls    f0, f0, f1
.L_801E3DC4:
    fmr      f23, f0
    bl       HSD_Randf
    lfd      f0, lbl_8065FE08@sda21(r0)
    lfd      f2, lbl_8065FE00@sda21(r0)
    fmul     f1, f0, f1
    lfs      f24, 0x48(r31)
    lfs      f0, lbl_8065FDD8@sda21(r0)
    fcmpo    cr0, f24, f0
    fmul     f22, f2, f1
    frsp     f22, f22
    bge      .L_801E3DF8
    fneg     f25, f24
    b        .L_801E3E00
.L_801E3DF8:
    bl       HSD_Randf
    fmuls    f25, f24, f1
.L_801E3E00:
    fmr      f1, f22
    bl       cos
    frsp     f24, f1
    fmr      f1, f23
    bl       sin
    frsp     f0, f1
    fmr      f1, f22
    fmuls    f0, f0, f24
    stfs     f0, 0x4c(r1)
    bl       sin
    frsp     f24, f1
    fmr      f1, f23
    bl       sin
    frsp     f0, f1
    fmr      f1, f23
    fmuls    f0, f0, f24
    stfs     f0, 0x50(r1)
    bl       cos
    frsp     f0, f1
    addi     r3, r1, 0xdc
    addi     r4, r1, 0x4c
    addi     r5, r1, 0x64
    stfs     f0, 0x54(r1)
    bl       PSMTXMultVec
    lfs      f2, 0x64(r1)
    lfs      f5, 0x60(r31)
    lfs      f0, 0x68(r1)
    lfs      f1, 0x6c(r1)
    fmuls    f2, f2, f5
    fmuls    f4, f0, f5
    lfs      f0, lbl_8065FDD8@sda21(r0)
    fmuls    f3, f1, f5
    stfs     f2, 0x4c(r1)
    stfs     f4, 0x50(r1)
    stfs     f3, 0x54(r1)
    lfs      f1, 0x48(r31)
    fcmpo    cr0, f1, f0
    cror     eq, gt, eq
    bne      .L_801E3EC0
    fcmpo    cr0, f5, f0
    bge      .L_801E3EC0
    fdivs    f0, f25, f1
    fmuls    f2, f2, f0
    fmuls    f1, f4, f0
    fmuls    f0, f3, f0
    stfs     f2, 0x4c(r1)
    stfs     f1, 0x50(r1)
    stfs     f0, 0x54(r1)
.L_801E3EC0:
    lfs      f1, 0x64(r1)
    li       r5, 0x0
    lfs      f0, 0x24(r31)
    li       r10, 0x0
    lfs      f2, 0x68(r1)
    fmadds   f0, f25, f1, f0
    lfs      f1, 0x6c(r1)
    stfs     f0, 0x64(r1)
    lfs      f0, 0x28(r31)
    fmadds   f0, f25, f2, f0
    stfs     f0, 0x68(r1)
    lfs      f0, 0x2c(r31)
    fmadds   f0, f25, f1, f0
    stfs     f0, 0x6c(r1)
    lfs      f0, 0x40(r31)
    stfs     f0, 0x8(r1)
    stw      r31, 0xc(r1)
    lbz      r3, 0x19(r31)
    lbz      r4, 0x18(r31)
    lwz      r6, 0x4(r31)
    lbz      r7, 0x1a(r31)
    lwz      r8, 0x20(r31)
    lhz      r9, 0x1e(r31)
    lfs      f1, 0x64(r1)
    lfs      f2, 0x68(r1)
    lfs      f3, 0x6c(r1)
    lfs      f4, 0x4c(r1)
    lfs      f5, 0x50(r1)
    lfs      f6, 0x54(r1)
    lfs      f7, 0x44(r31)
    lfs      f8, 0x3c(r31)
    bl       psGenerateParticle
    b        .L_801E3F60
.L_801E3F44:
    lwz      r12, lbl_8065BAB0@sda21(r0)
    cmplwi   r12, 0x0
    beq      .L_801E3F60
    mr       r3, r31
    addi     r4, r1, 0xdc
    mtctr    r12
    bctrl
.L_801E3F60:
    lfs      f0, 0xc(r31)
    fsubs    f0, f0, f26
    stfs     f0, 0xc(r31)
.L_801E3F6C:
    lfs      f1, 0xc(r31)
    fcmpo    cr0, f1, f26
    cror     eq, gt, eq
    beq      .L_801E32C4
.L_801E3F7C:
    psq_l    f31, 0x1c8(r1), 0, qr0
    lfd      f31, 0x1c0(r1)
    psq_l    f30, 0x1b8(r1), 0, qr0
    lfd      f30, 0x1b0(r1)
    psq_l    f29, 0x1a8(r1), 0, qr0
    lfd      f29, 0x1a0(r1)
    psq_l    f28, 0x198(r1), 0, qr0
    lfd      f28, 0x190(r1)
    psq_l    f27, 0x188(r1), 0, qr0
    lfd      f27, 0x180(r1)
    psq_l    f26, 0x178(r1), 0, qr0
    lfd      f26, 0x170(r1)
    psq_l    f25, 0x168(r1), 0, qr0
    lfd      f25, 0x160(r1)
    psq_l    f24, 0x158(r1), 0, qr0
    lfd      f24, 0x150(r1)
    psq_l    f23, 0x148(r1), 0, qr0
    lfd      f23, 0x140(r1)
    psq_l    f22, 0x138(r1), 0, qr0
    lfd      f22, 0x130(r1)
    lwz      r0, 0x1d4(r1)
    lwz      r31, 0x12c(r1)
    mtlr     r0
    addi     r1, r1, 0x1d0
    blr
*/

/**
 * @note Address: 0x801E3FE0
 * @note Size: 0x3A0
 */
void psExecGenerator(u32 mask)
{
	HSD_Generator* gen;
	HSD_JObj* jobj;

	while (lbl_8065BA98 != NULL) {
		gen = lbl_8065BA98->data;
		psApplyGeneratorJObj(gen);
		lbl_8065BA98 = HSD_SListRemove(lbl_8065BA98);
		if (gen->jobj != NULL) {
			HSD_JObjUnref(gen->jobj);
			gen->jobj = NULL;
		}
	}

	gen          = lbl_8065BAA0;
	lbl_8065BA9C = NULL;
	while (gen != NULL) {
		if (mask & (1 << (gen->linkNo + 0x10))) {
			lbl_8065BA9C = gen;
			gen          = gen->next;
			continue;
		}
		if (gen->kind & 0x800) {
			lbl_8065BA9C = gen;
			gen          = gen->next;
			continue;
		}

		if ((gen->type & 0x100) && (jobj = gen->jobj) != NULL) {
			HSD_JObjSetupMatrix(jobj);
			if (gen->type & 0x200) {
				gen->pos.x = gen->jobj->mtx[0][3];
				gen->pos.y = gen->jobj->mtx[1][3];
				gen->pos.z = gen->jobj->mtx[2][3];
			}
			if (gen->type & 0x800) {
				if (gen->appsrt != NULL && gen->appsrt->gp == gen) {
					gen->appsrt->translate.x = gen->jobj->mtx[0][3];
					gen->appsrt->translate.y = gen->jobj->mtx[1][3];
					gen->appsrt->translate.z = gen->jobj->mtx[2][3];
				}
			}
			if (gen->type & 0x1000) {
				if (gen->appsrt != NULL && gen->appsrt->gp == gen) {
					HSD_MtxGetScale(gen->jobj->mtx, &gen->appsrt->scale);
				}
			}
			if (gen->type & 0x2000) {
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
			if (lbl_8065BA9C != NULL) {
				gen = lbl_8065BA9C->next;
			} else {
				gen = lbl_8065BAA0;
			}
		} else {
			lbl_8065BA9C = gen;
			gen          = gen->next;
		}
	}
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
	if (gen != NULL) {
		gen->jobj = jobj;
		if (jobj != NULL) {
			ref_INC(jobj);
		}
	}
	gen->type |= (gen->kind & 0x20000) ? 0x500 : 0x700;
	return gen;
}

static inline HSD_Generator* gen_allocGenerator(void)
{
	HSD_Generator* gen;

	gen = lbl_8065BAA4;
	if (gen == NULL) {
		gen = NULL;
	} else {
		lbl_8065BA8C++;
		if (lbl_8065BA8C > lbl_8065BA86) {
			lbl_8065BA86 = lbl_8065BA8C;
		}
		lbl_8065BAA4 = gen->next;
		if (lbl_8065BA9C == NULL || lbl_8065BA9C->next == NULL) {
			if (lbl_8065BAA0 == NULL) {
				gen->next    = NULL;
				lbl_8065BAA0 = gen;
			} else {
				gen->next          = lbl_8065BAA0->next;
				lbl_8065BAA0->next = gen;
			}
		} else {
			gen->next                = lbl_8065BA9C->next->next;
			lbl_8065BA9C->next->next = gen;
		}
		if (++sParticleGeneratorIdCounter < 0x100) {
			sParticleGeneratorIdCounter = 0x100;
		}
		gen->idnum    = sParticleGeneratorIdCounter;
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
	HSD_PSTexGroup* tex;

	if (bank >= 0x40) {
		return NULL;
	}
	if (linkNo >= 8) {
		return NULL;
	}
	if (kind >= psNumCmdList[bank]) {
		return NULL;
	}
	if (psCmdListArray[bank][kind] == NULL) {
		return NULL;
	}

	gen = gen_allocGenerator();
	if (gen != NULL) {
		gen->type     = psCmdListArray[bank][kind]->type;
		gen->bank     = bank;
		gen->linkNo   = linkNo;
		gen->kind     = psCmdListArray[bank][kind]->kind;
		gen->texGroup = psCmdListArray[bank][kind]->texGroup;
		gen->life     = psCmdListArray[bank][kind]->life;
		gen->genLife  = psCmdListArray[bank][kind]->genLife;
		gen->pos.z    = 0.0F;
		gen->pos.y    = 0.0F;
		gen->pos.x    = 0.0F;
		gen->vel.x    = psCmdListArray[bank][kind]->vx;
		gen->vel.y    = psCmdListArray[bank][kind]->vy;
		gen->vel.z    = psCmdListArray[bank][kind]->vz;
		gen->grav     = psCmdListArray[bank][kind]->grav;
		gen->fric     = psCmdListArray[bank][kind]->fric;
		gen->size     = psCmdListArray[bank][kind]->size;
		gen->cmdList  = psCmdListArray[bank][kind]->cmdList;
		gen->radius   = psCmdListArray[bank][kind]->radius;
		gen->angle    = psCmdListArray[bank][kind]->angle;
		gen->random   = psCmdListArray[bank][kind]->random;

		if (gen->kind & 0x100) {
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
			gen->kind |= 0x10;
		}

		gen->jobj     = NULL;
		gen->numChild = 0;

		switch (gen->type & 0xF) {
		case 0:
		case 3:
		case 4:
			if (psCmdListArray[bank][kind]->param1 == 0.0F && psCmdListArray[bank][kind]->param2 == 0.0F) {
				gen->aux.disc.minAngle = 0.0F;
				gen->aux.disc.maxAngle = 6.2831855F;
			} else {
				gen->aux.disc.minAngle = psCmdListArray[bank][kind]->param1;
				gen->aux.disc.maxAngle = psCmdListArray[bank][kind]->param2;
			}
			break;
		case 1:
			gen->aux.line.x2 = psCmdListArray[bank][kind]->param1;
			gen->aux.line.y2 = psCmdListArray[bank][kind]->param2;
			gen->aux.line.z2 = psCmdListArray[bank][kind]->param3;
			break;
		case 2:
			break;
		case 6:
		case 7:
			if (psCmdListArray[bank][kind]->param1 == 0.0F && psCmdListArray[bank][kind]->param2 == 0.0F) {
				gen->aux.cone.minAngle = 0.0F;
				gen->aux.cone.maxAngle = 6.2831855F;
			} else {
				gen->aux.cone.minAngle = psCmdListArray[bank][kind]->param1;
				gen->aux.cone.maxAngle = psCmdListArray[bank][kind]->param2;
			}
			gen->aux.cone.height = psCmdListArray[bank][kind]->param3;
			break;
		case 5:
			gen->aux.rect.xx = gen->aux.rect.x = psCmdListArray[bank][kind]->param1;
			gen->aux.rect.yy = gen->aux.rect.y = psCmdListArray[bank][kind]->param2;
			gen->aux.rect.zz = gen->aux.rect.z = psCmdListArray[bank][kind]->param3;
			gen->aux.rect.zy                   = 0.0F;
			gen->aux.rect.zx                   = 0.0F;
			gen->aux.rect.yz                   = 0.0F;
			gen->aux.rect.yx                   = 0.0F;
			gen->aux.rect.xz                   = 0.0F;
			gen->aux.rect.xy                   = 0.0F;
			gen->aux.rect.flag                 = 0;
			if (psCmdListArray[bank][kind]->param1 < 0.0F) {
				gen->aux.rect.flag |= 1;
			}
			if (psCmdListArray[bank][kind]->param2 < 0.0F) {
				gen->aux.rect.flag |= 2;
			}
			if (psCmdListArray[bank][kind]->param3 < 0.0F) {
				gen->aux.rect.flag |= 4;
			}
			break;
		case 8: {
			f32 horiz;

			gen->aux.sphere.speed = gen_sqrtf(gen->vel.x * gen->vel.x + gen->vel.y * gen->vel.y + gen->vel.z * gen->vel.z);
			horiz                 = gen_sqrtf(gen->vel.x * gen->vel.x + gen->vel.z * gen->vel.z);
			if (horiz < FLT_MIN) {
				if (gen->vel.y >= 0.0F) {
					gen->aux.sphere.latMid = 1.5707964F;
				} else {
					gen->aux.sphere.latMid = -1.5707964F;
				}
			} else {
				gen->aux.sphere.latMid = gen_atan2f(gen->vel.y, horiz);
			}
			GEN_ANGLE(gen->aux.sphere.lonMid, gen->vel.z, gen->vel.x);
			gen->aux.sphere.latRange = psCmdListArray[bank][kind]->param1;
			if (gen->aux.sphere.latRange < 0.0F) {
				gen->aux.sphere.latRange = -gen->aux.sphere.latRange;
				gen->aux.sphere.speed    = -gen->aux.sphere.speed;
			}
			gen->aux.sphere.lonRange = psCmdListArray[bank][kind]->param2;
			break;
		}
		default:
			if (lbl_8065BAAC != NULL) {
				lbl_8065BAAC(gen);
			}
			break;
		}

		if (gen->kind & 0x20000) {
			gen->type |= 0x800;
			CreateGeneratorAppSRT(gen, 0);
			if (gen->appsrt != NULL) {
				gen->appsrt->xA2 = 1;
				gen->appsrt->gp  = gen;
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
	if (gen != NULL) {
		gen->jobj = jobj;
		if (jobj != NULL) {
			ref_INC(jobj);
		}
	}
	gen->type |= (gen->kind & 0x20000) ? 0x500 : 0x700;
	lbl_8065BA98 = HSD_SListAppend(lbl_8065BA98, gen);
	return gen;
}

/**
 * @note Address: 0x801E4D14
 * @note Size: 0xEC
 */
void psSetBillboardCamera(HSD_CObj* cobj)
{
	gen_cobjUnref(psCamera);
	if (cobj != NULL) {
		ref_INC(cobj);
	}
	psCamera = cobj;
}
