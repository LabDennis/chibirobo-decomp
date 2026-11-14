#include <global.h>

#include <dolphin/gx/gxtypes.h>
#include <dolphin/mtx/mtxtypes.h>
#include <dolphin/os.h>
#include <sysdolphin/forward.h>
#include <sysdolphin/jobj.h>
#include <sysdolphin/mtx.h>
#include <sysdolphin/object.h>
#include <sysdolphin/pslist.h>
#include <sysdolphin/random.h>

#include <sysdolphin/particle.h>


void SyncParticleGeneratorFromJoint(HSD_Generator* gen);
s32 applyForceJObj(HSD_Particle* pp, HSD_JObj* jobj, f32 force, f32 range);

extern f32 sqrtf(f32);
extern f32 tanf(f32);
extern f32 sinf(f32);
extern f32 cosf(f32);
extern f64 atan2(f64, f64);
extern f64 sin(f64);
extern f64 cos(f64);
extern int __float_nan[];
extern int __float_min[];

extern s32 psRemoveParticleAppSRT(HSD_Particle* pp);
extern s32 psAttachParticleAppSRT(HSD_Particle* pp, HSD_psAppSRT* srt);
extern s32 psAttachGeneratorAppSRT(HSD_Generator* gen, HSD_psAppSRT* srt);
extern s32 AttachParticleAppSRT(HSD_Particle* pp, HSD_psAppSRT* srt);
extern s32 AttachGeneratorAppSRT(HSD_Generator* gen, HSD_psAppSRT* srt);
extern HSD_Generator* psCreateGeneratorID(s32 linkNo, s32 bank, s32 idx);
extern void psSetPointJObj(s32 no, HSD_JObj* jobj);

// particle.c's per bank tables
extern void* ptclref[64];
extern HSD_PSTexGroup** psTexGroupArray[64];
extern HSD_JObj* psPointJObj[8];
// the user callback table psInitParticle clears
extern int (**lbl_8065BA80)(HSD_Particle*);

#define BANK_REF(b)      (ptclref[b])
#define BANK_TEXARRAY(b) (psTexGroupArray[b])

// the command stream floats get copied through here a byte at a time
static f32 BYTEF32;

#define JOBJ_FLAG_NO_AUTODIRTY 0x2000000

#define PS_ABS(x) ((x) < 0.0F ? -(x) : (x))

#define assert_line_jobjh(line, cond) ((cond) ? ((void)0) : __assert("jobj.h", line, "jobj"))

// the msl float wrappers, they just call the double versions
static inline f32 particle_sinf(f32 x)
{
	return sin(x);
}

static inline f32 particle_cosf(f32 x)
{
	return cos(x);
}

static inline s32 particle_fpclassifyf(f32 x)
{
	switch (*(s32*)&x & 0x7F800000) {
	case 0x7F800000:
		if (*(s32*)&x & 0x7FFFFF) {
			return 1;
		}
		return 2;
	case 0:
		if (*(s32*)&x & 0x7FFFFF) {
			return 5;
		}
		return 3;
	}
	return 4;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline f32 particle_sqrtf(f32 x)
{
	static const f64 _half  = 0.5;
	static const f64 _three = 3.0;

	if (x > 0.0F) {
		f64 guess = __frsqrte((f64)x);
		guess     = _half * guess * (_three - guess * guess * x);
		guess     = _half * guess * (_three - guess * guess * x);
		guess     = _half * guess * (_three - guess * guess * x);
		return (f32)(x * guess);
	} else if (x < 0.0) {
		return *(f32*)__float_nan;
	} else if (particle_fpclassifyf(x) == 1) {
		return *(f32*)__float_nan;
	}
	return x;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void particle_ref_INC_b(void* o)
{
	if (o != NULL) {
		HSD_OBJ(o)->ref_count++;
		((HSD_OBJ(o)->ref_count != HSD_OBJ_NOREF)) ? ((void)0) : __assert("object.h", 93, "HSD_OBJ(o)->ref_count != HSD_OBJ_NOREF");
	}
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline BOOL particle_JObjMtxIsDirty(HSD_JObj* jobj)
{
	BOOL result;
	assert_line_jobjh(0x25d, jobj);
	result = FALSE;
	if (!(jobj->flags & 0x800000) && (jobj->flags & 0x40)) {
		result = TRUE;
	}
	return result;
}

// SetupDirtyParticleJointMatrix through _psListGetNext are inlines, psInterpretParticle0 is too big to
// take them so the compiler puts copies after it, last used first, same as retail
static inline void SetupDirtyParticleJointMatrix(HSD_JObj* jobj)
{
	if (jobj == NULL || !particle_JObjMtxIsDirty(jobj)) {
		return;
	}
	HSD_JObjSetupMatrixSub(jobj);
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void particle_JObjSetMtxDirty(HSD_JObj* jobj)
{
	if (jobj != NULL && !particle_JObjMtxIsDirty(jobj)) {
		HSD_JObjSetMtxDirtySub(jobj);
	}
}

/**
 * @note Address: N/A
 * @note Size: 0xB0
 */
static inline void HSD_JObjAddTz(HSD_JObj* jobj, f32 delta)
{
	assert_line_jobjh(0x46b, jobj);
	jobj->translate.z += delta;
	if (!(jobj->flags & JOBJ_FLAG_NO_AUTODIRTY)) {
		particle_JObjSetMtxDirty(jobj);
	}
}

/**
 * @note Address: N/A
 * @note Size: 0xB0
 */
static inline void HSD_JObjAddTy(HSD_JObj* jobj, f32 delta)
{
	assert_line_jobjh(0x460, jobj);
	jobj->translate.y += delta;
	if (!(jobj->flags & JOBJ_FLAG_NO_AUTODIRTY)) {
		particle_JObjSetMtxDirty(jobj);
	}
}

/**
 * @note Address: N/A
 * @note Size: 0xB0
 */
static inline void HSD_JObjAddTx(HSD_JObj* jobj, f32 delta)
{
	assert_line_jobjh(0x455, jobj);
	jobj->translate.x += delta;
	if (!(jobj->flags & JOBJ_FLAG_NO_AUTODIRTY)) {
		particle_JObjSetMtxDirty(jobj);
	}
}

/**
 * @note Address: N/A
 * @note Size: 0x58
 */
static inline s32 U8ClampAdd(u8 old, f32 delta)
{
	f32 v = (f32)old + delta;
	if (v < 0.0F) {
		v = 0.0F;
	}
	if (v > 255.0F) {
		v = 255.0F;
	}
	return (s32)v;
}

/**
 * @note Address: 0x801E22D0
 * @note Size: 0x20
 */
static inline void HSD_MTXSRT(Mtx mtx, Vec* scale, Vec* rotate, Vec* translate, Vec* arg4)
{
	HSD_MtxSRT(mtx, scale, rotate, translate, arg4);
}

/**
 * @note Address: N/A
 * @note Size: 0x64
 */
static inline void AttachJointToParticleGenerator(HSD_Generator* gen, HSD_JObj* jobj)
{
	if (gen != NULL) {
		gen->jobj = jobj;
		if (jobj != NULL) {
			particle_ref_INC_b(jobj);
		}
	}
}

/**
 * @note Address: N/A
 * @note Size: 0x8
 */
static inline HSD_Particle* _psListGetNext(HSD_Particle* pp)
{
	return pp->next;
}

/**
 * @note Address: N/A
 * @note Size: 0x170
 */
void SyncParticleGeneratorFromJoint(HSD_Generator* gen)
{
	HSD_JObj* jobj;

	if (gen == NULL) {
		return;
	}
	if (gen->kind & 0x800) {
		return;
	}
	if (!(gen->type & 0x100)) {
		return;
	}

	jobj = gen->jobj;
	if (jobj == NULL) {
		return;
	}

	SetupDirtyParticleJointMatrix(jobj);

	if (gen->type & 0x200) {
		gen->pos.x = gen->jobj->mtx[0][3];
		gen->pos.y = gen->jobj->mtx[1][3];
		gen->pos.z = gen->jobj->mtx[2][3];
	}

	if ((gen->type & 0x800) && gen->appsrt != NULL && gen->appsrt->gp == gen) {
		gen->appsrt->translate.x = gen->jobj->mtx[0][3];
		gen->appsrt->translate.y = gen->jobj->mtx[1][3];
		gen->appsrt->translate.z = gen->jobj->mtx[2][3];
	}

	if ((gen->type & 0x1000) && gen->appsrt != NULL && gen->appsrt->gp == gen) {
		HSD_MtxGetScale(gen->jobj->mtx, &gen->appsrt->scale);
	}

	if (gen->type & 0x2000) {
		HSD_MtxGetRotation(gen->jobj->mtx, &gen->appsrt->rot);
	}
}

/**
 * @note Address: N/A
 * @note Size: 0x38
 */
u8* getFloat(u8* pc, f32* out)
{
	// the command stream is not aligned, so copy it a byte at a time
	u8* dst = (u8*)&BYTEF32;

	*dst++ = *pc++;
	*dst++ = *pc++;
	*dst++ = *pc++;
	*dst++ = *pc++;
	*out   = BYTEF32;
	return pc;
}

/**
 * @note Address: N/A
 * @note Size: 0x30
 */
u8* getTime(u8* pc, u16* out)
{
	*out = *pc++;
	if (*out & 0x80) {
		*out = ((*out & 0x7F) << 8) + *pc++;
	}
	return pc;
}

/**
 * @note Address: 0x801DE6C8
 * @note Size: 0x31C
 */
void modifyDirGenBase(f32 angle, f32 vx, f32 vy, f32 vz, HSD_Particle* pp)
{
	HSD_Generator* gen = pp->gen;
	f32 randAngle;
	f32 dx, dy, dz;
	f32 sin1, cos1, sin2, cos2;
	f32 speed;
	f32 sinR, cosR;
	f32 sinAngle, cosAngle;
	f32 azimuth;
	f32 elevation;

	dz = gen->vel.z + vz;
	dx = gen->vel.x + vx;
	dy = gen->vel.y + vy;

	if (__fabsf(dz) < *(f32*)__float_min) {
		if (dy >= 0.0F) {
			azimuth = 1.5707964F;
		} else {
			azimuth = -1.5707964F;
		}
	} else {
		f32 t   = atan2(dy, dz);
		azimuth = t;
	}

	sin1 = particle_sinf(azimuth);
	cos1 = particle_cosf(azimuth);

	dy = dy * sin1 + dz * cos1;

	if (__fabsf(dy) < *(f32*)__float_min) {
		if (dx >= 0.0F) {
			elevation = 1.5707964F;
		} else {
			elevation = -1.5707964F;
		}
	} else {
		f32 t     = atan2(dx, dy);
		elevation = t;
	}

	sin2 = particle_sinf(elevation);
	cos2 = particle_cosf(elevation);

	speed = particle_sqrtf(pp->vel.x * pp->vel.x + pp->vel.y * pp->vel.y + pp->vel.z * pp->vel.z);

	randAngle = (f32)(3.141592653589793 * HSD_Randf() * 2.0);
	sinAngle  = speed * particle_sinf(angle);
	sinR      = sinAngle * particle_cosf(randAngle);
	cosR      = sinAngle * particle_sinf(randAngle);
	cosAngle  = speed * particle_cosf(angle);

	pp->vel.x = sinR * cos2 + cosAngle * sin2;
	pp->vel.y = cos2 * (cosAngle * sin1) + (sin2 * (-sinR * sin1) + cosR * cos1);
	pp->vel.z = cos2 * (cosAngle * cos1) + (sin2 * (-sinR * cos1) - cosR * sin1);
}

/**
 * @note Address: 0x801DE9E4
 * @note Size: 0x2F8
 */
void modifyDir(f32 angle, HSD_Particle* pp)
{
	f32 cosR;
	f32 sinAngle;
	f32 azimuth;
	f32 sin1, cos1, sin2, cos2;
	f32 speed;
	f32 randAngle;
	f32 sinR;
	f32 vx, vy, vz;
	f32 ny;
	f32 elevation;
	f32 cosAngle;

	vz = pp->vel.z;
	vx = pp->vel.x;
	vy = pp->vel.y;

	if (__fabsf(vz) < *(f32*)__float_min) {
		if (vy >= 0.0F) {
			azimuth = 1.5707964F;
		} else {
			azimuth = -1.5707964F;
		}
	} else {
		// the float result goes through its own temp first
		f32 t   = atan2(vy, vz);
		azimuth = t;
	}

	sin1 = particle_sinf(azimuth);
	cos1 = particle_cosf(azimuth);

	ny = vy * sin1 + vz * cos1;

	if (__fabsf(ny) < *(f32*)__float_min) {
		if (vx >= 0.0F) {
			elevation = 1.5707964F;
		} else {
			elevation = -1.5707964F;
		}
	} else {
		// the float result goes through its own temp first
		f32 t   = atan2(vx, ny);
		elevation = t;
	}

	sin2 = particle_sinf(elevation);
	cos2 = particle_cosf(elevation);

	speed = particle_sqrtf(vx * vx + vy * vy + vz * vz);

	randAngle = (f32)(3.141592653589793 * HSD_Randf() * 2.0);
	sinAngle  = speed * particle_sinf(angle);
	sinR      = sinAngle * particle_cosf(randAngle);
	cosR      = sinAngle * particle_sinf(randAngle);
	cosAngle  = speed * particle_cosf(angle);

	pp->vel.x = sinR * cos2 + cosAngle * sin2;
	pp->vel.y = cos2 * (cosAngle * sin1) + (sin2 * (-sinR * sin1) + cosR * cos1);
	pp->vel.z = cos2 * (cosAngle * cos1) + (sin2 * (-sinR * cos1) - cosR * sin1);
}

/**
 * @note Address: N/A
 * @note Size: 0x1A8
 */
void setVelToJObj(HSD_Particle* pp, HSD_JObj* jobj)
{
	f32 vmag;
	f32 dx, dy, dz, distsq, dist;

	if (jobj == NULL) {
		return;
	}

	SetupDirtyParticleJointMatrix(jobj);

	dx     = jobj->mtx[0][3] - pp->pos.x;
	dy     = jobj->mtx[1][3] - pp->pos.y;
	dz     = jobj->mtx[2][3] - pp->pos.z;
	vmag   = particle_sqrtf(pp->vel.x * pp->vel.x + pp->vel.y * pp->vel.y + pp->vel.z * pp->vel.z);
	distsq = dx * dx + dy * dy + dz * dz;
	if (distsq == 0.0F) {
		return;
	}
	dist = particle_sqrtf(distsq);
	{
		f32 scale = vmag / dist;
		pp->vel.x = dx * scale;
		pp->vel.y = dy * scale;
		pp->vel.z = dz * scale;
	}
}

/**
 * @note Address: N/A
 * @note Size: 0x144
 */
s32 applyForceJObj(HSD_Particle* pp, HSD_JObj* jobj, f32 force, f32 range)
{
	f32 dx, dy, dz;
	f32 distSq;

	if (jobj == NULL || range < 0.0F) {
		return 0;
	}

	SetupDirtyParticleJointMatrix(jobj);

	dx = jobj->mtx[0][3] - pp->pos.x;
	dy = jobj->mtx[1][3] - pp->pos.y;
	dz = jobj->mtx[2][3] - pp->pos.z;

	distSq = dz * dz + (dx * dx + dy * dy);
	if (distSq <= range * range) {
		return 1;
	}
	if (distSq == 0.0F) {
		return 0;
	}
	{
		f32 scale = force / distSq;
		pp->vel.x += scale * dx;
		pp->vel.y += scale * dy;
		pp->vel.z += scale * dz;
	}
	return 0;
}

/**
 * @note Address: 0x801DF0C8
 * @note Size: 0x2F30
 */
HSD_Particle* psInterpretParticle0(HSD_Particle* pp, HSD_Particle* prev)
{
	f32 v;
	f32 v1;
	f32 v2;
	f32 v3;
	u8* pc;
	u16 operand;
	u8 opcode;
	u8 cls;
	HSD_Particle* child;

	if (pp->kind & 0x800) {
		return _psListGetNext(pp);
	}

	if (pp->sizeCount != 0) {
		pp->size += (pp->sizeTarget - pp->size) / (f32)pp->sizeCount;
		pp->sizeCount--;
	}

	if (pp->primColCount != 0) {
		pp->primColRemain--;
		if (pp->primColRemain == 0) {
			pp->primColCount = 0;
			pp->primCol.r    = pp->primColTarget.r;
			pp->primCol.g    = pp->primColTarget.g;
			pp->primCol.b    = pp->primColTarget.b;
			pp->primCol.a    = pp->primColTarget.a;
		}
	}

	if (pp->envColCount != 0) {
		pp->envColRemain--;
		if (pp->envColRemain == 0) {
			pp->envColCount = 0;
			pp->envCol.r    = pp->envColTarget.r;
			pp->envCol.g    = pp->envColTarget.g;
			pp->envCol.b    = pp->envColTarget.b;
			pp->envCol.a    = pp->envColTarget.a;
		}
	}

	if (pp->matColCount != 0) {
		pp->matColRemain--;
		if (pp->matColRemain == 0) {
			pp->matColCount = 0;
			pp->matRGB      = pp->matRGBTarget;
			pp->matA        = pp->matATarget;
		}
	}

	if (pp->ambColCount != 0) {
		pp->ambColRemain--;
		if (pp->ambColRemain == 0) {
			pp->ambColCount = 0;
			pp->ambRGB      = pp->ambRGBTarget;
			pp->ambA        = pp->ambATarget;
		}
	}

	if (pp->aCmpCount != 0) {
		pp->aCmpRemain--;
		if (pp->aCmpRemain == 0) {
			pp->aCmpCount  = 0;
			pp->aCmpParam1 = pp->aCmpParam1Target;
			pp->aCmpParam2 = pp->aCmpParam2Target;
		}
	}

	if (pp->rotateCount != 0) {
		if (pp->spreadRate) {
			pp->rotate += pp->rotateTarget;
			if (pp->rotateTarget >= 0.0F) {
				pp->rotateTarget += pp->spreadRate;
			} else {
				pp->rotateTarget -= pp->spreadRate;
			}
			if (--pp->rotateCount == 0) {
				pp->spreadRate   = 0.0F;
				pp->rotateTarget = 0.0F;
			}
		} else {
			pp->rotate += (pp->rotateTarget - pp->rotate) / (f32)pp->rotateCount;
			pp->rotateCount--;
		}
	}

	if (pp->cmdWait != 0 && --pp->cmdWait == 0) {
		pc = pp->cmdList + pp->cmdPtr;

		do {
			opcode = *pc++;

			if (opcode < 0x80) {
				operand = opcode & 0x1F;
				if (opcode & 0x20) {
					operand = (operand << 8) + *pc++;
				}
				switch (opcode & 0xC0) {
				case 0x00:
					break;
				case 0x40: {
					HSD_PSTexGroup* tg;

					pp->poseNum = *pc++;
					tg          = BANK_TEXARRAY(pp->bank)[pp->texGroup];
					if (tg != NULL && tg->texTable != NULL && tg->texTable[pp->poseNum] != NULL) {
						pp->kind |= 0x400;
					}
					break;
				}
				}
			} else {
				operand = 0;
				cls     = opcode & 0xF8;
				if (cls > 0x98) {
					cls = opcode & 0xF0;
					if (cls != 0xC0 && cls != 0xD0) {
						cls = opcode;
					}
				}

				switch (cls) {
				case 0x80: {
					if (opcode & 1) {
						pc = getFloat(pc, &pp->pos.x);
					}
					if (opcode & 2) {
						pc = getFloat(pc, &pp->pos.y);
					}
					if (opcode & 4) {
						pc = getFloat(pc, &pp->pos.z);
					}
					break;
				}

				case 0x88: {
					if (opcode & 1) {
						pc = getFloat(pc, &v);
						pp->pos.x += v;
					}
					if (opcode & 2) {
						pc = getFloat(pc, &v);
						pp->pos.y += v;
					}
					if (opcode & 4) {
						pc = getFloat(pc, &v);
						pp->pos.z += v;
					}
					break;
				}

				case 0x90: {
					if (opcode & 1) {
						pc = getFloat(pc, &pp->vel.x);
					}
					if (opcode & 2) {
						pc = getFloat(pc, &pp->vel.y);
					}
					if (opcode & 4) {
						pc = getFloat(pc, &pp->vel.z);
					}
					break;
				}

				case 0x98: {
					if (opcode & 1) {
						pc = getFloat(pc, &v);
						pp->vel.x += v;
					}
					if (opcode & 2) {
						pc = getFloat(pc, &v);
						pp->vel.y += v;
					}
					if (opcode & 4) {
						pc = getFloat(pc, &v);
						pp->vel.z += v;
					}
					break;
				}

				case 0xA0: {
					pc = getTime(pc, &pp->sizeCount);
					pc = getFloat(pc, &pp->sizeTarget);
					if (pp->sizeCount == 0) {
						pp->size = pp->sizeTarget;
					}
					break;
				}

				case 0xA1:
					pp->kind &= ~0x400;
					break;

				case 0xA2: {
					pc = getFloat(pc, &pp->grav);
					if (pp->grav == 0.0F) {
						pp->kind &= ~1;
					} else {
						pp->kind |= 1;
					}
					break;
				}

				case 0xA3: {
					pc = getFloat(pc, &pp->fric);
					if (pp->fric == 1.0F) {
						pp->kind &= ~2;
					} else {
						pp->kind |= 2;
					}
					break;
				}

				case 0xA4: {
					s32 idx = *pc++ << 8;
					idx += *pc++;

					child = psGenerateParticleID0(pp, pp->linkNo, pp->bank, idx, 0);
					if (child != NULL) {
						child->idnum = pp->idnum;
						child->gen   = pp->gen;
						if (pp->gen != NULL) {
							pp->gen->numChild++;
						}
						if (pp->gen != NULL && (pp->gen->type & 0x2000)) {
							AttachParticleAppSRT(child, pp->appsrt);
						} else {
							psAttachParticleAppSRT(child, pp->appsrt);
						}
						child->pos.x = pp->pos.x;
						child->pos.y = pp->pos.y;
						child->pos.z = pp->pos.z;
						psInterpretParticle0(child, pp);
					}
					break;
				}

				case 0xF1: {
					u8 bank = pp->bank;
					s32 idx = *pc++ << 8;
					idx += *pc++;
					if (BANK_REF(bank) != NULL) {
						idx = ((u32*)BANK_REF(bank))[idx];
					}

					child = psGenerateParticleID0(pp, pp->linkNo, bank, idx, 0);
					if (child != NULL) {
						child->idnum = pp->idnum;
						child->gen   = pp->gen;
						if (pp->gen != NULL) {
							pp->gen->numChild++;
						}
						if (pp->gen != NULL && (pp->gen->type & 0x2000)) {
							AttachParticleAppSRT(child, pp->appsrt);
						} else {
							psAttachParticleAppSRT(child, pp->appsrt);
						}
						child->pos.x = pp->pos.x;
						child->pos.y = pp->pos.y;
						child->pos.z = pp->pos.z;
						psInterpretParticle0(child, pp);
					}
					break;
				}

				case 0xA5: {
					HSD_Generator* gchild;
					s32 idx = *pc++ << 8;
					idx += *pc++;
					gchild = psCreateGeneratorID(pp->linkNo, pp->bank, idx);
					if (gchild != NULL) {
						gchild->idnum = pp->idnum;
						if (pp->gen != NULL) {
							AttachJointToParticleGenerator(gchild, pp->gen->jobj);
						}
						if (pp->appsrt != NULL) {
							if (pp->gen != NULL && (pp->gen->type & 0x2000)) {
								AttachGeneratorAppSRT(gchild, pp->appsrt);
							} else {
								psAttachGeneratorAppSRT(gchild, pp->appsrt);
							}
						}
						if (pp->appsrt != NULL) {
							if (gchild->appsrt != NULL) {
								gchild->pos.x = pp->pos.x;
								gchild->pos.y = pp->pos.y;
								gchild->pos.z = pp->pos.z;
								if (gchild->appsrt != pp->appsrt) {
									gchild->appsrt->translate = pp->appsrt->translate;
								}
							}
						} else if (gchild->appsrt != NULL) {
							gchild->appsrt->translate.x = pp->pos.x;
							gchild->appsrt->translate.y = pp->pos.y;
							gchild->appsrt->translate.z = pp->pos.z;
							gchild->pos.x               = pp->pos.x;
							gchild->pos.y               = pp->pos.y;
							gchild->pos.z               = pp->pos.z;
						} else {
							gchild->pos.x = pp->pos.x;
							gchild->pos.y = pp->pos.y;
							gchild->pos.z = pp->pos.z;
						}
						if (gchild->appsrt != pp->appsrt) {
							if (pp->appsrt == NULL) {
								if (gchild->appsrt != NULL) {
									SyncParticleGeneratorFromJoint(gchild);
									gchild->pos.x -= gchild->appsrt->translate.x;
									gchild->pos.y -= gchild->appsrt->translate.y;
									gchild->pos.z -= gchild->appsrt->translate.z;
								}
							} else {
								SyncParticleGeneratorFromJoint(pp->appsrt->gp);
								if (gchild->appsrt != NULL) {
									SyncParticleGeneratorFromJoint(gchild);
									gchild->pos.x += pp->appsrt->translate.x - gchild->appsrt->translate.x;
									gchild->pos.y += pp->appsrt->translate.y - gchild->appsrt->translate.y;
									gchild->pos.z += pp->appsrt->translate.z - gchild->appsrt->translate.z;
								} else {
									gchild->pos.x += pp->appsrt->translate.x;
									gchild->pos.y += pp->appsrt->translate.y;
									gchild->pos.z += pp->appsrt->translate.z;
								}
							}
						}
					}
					break;
				}

				case 0xEF: {
					s32 idx;
					u32 flags;
					HSD_Generator* gchild;
					idx = *pc++ << 8;
					idx += *pc++;
					flags  = *pc++;
					gchild = psCreateGeneratorID(pp->linkNo, pp->bank, idx);
					if (gchild != NULL) {
						gchild->idnum = pp->idnum;
						if (pp->gen != NULL) {
							AttachJointToParticleGenerator(gchild, pp->gen->jobj);
						}
						if (pp->appsrt != NULL) {
							if (pp->gen != NULL && (pp->gen->type & 0x2000)) {
								AttachGeneratorAppSRT(gchild, pp->appsrt);
							} else {
								psAttachGeneratorAppSRT(gchild, pp->appsrt);
							}
						}
						gchild->kind &= 0xF1FFFFFF;
						gchild->kind |= (flags & 7) << 25;
						if (pp->appsrt != NULL) {
							if (gchild->appsrt != NULL) {
								gchild->pos.x = pp->pos.x;
								gchild->pos.y = pp->pos.y;
								gchild->pos.z = pp->pos.z;
								if (gchild->appsrt != pp->appsrt) {
									gchild->appsrt->translate = pp->appsrt->translate;
								}
							}
						} else if (gchild->appsrt != NULL) {
							gchild->appsrt->translate.x = pp->pos.x;
							gchild->appsrt->translate.y = pp->pos.y;
							gchild->appsrt->translate.z = pp->pos.z;
							gchild->pos.x               = pp->pos.x;
							gchild->pos.y               = pp->pos.y;
							gchild->pos.z               = pp->pos.z;
						} else {
							gchild->pos.x = pp->pos.x;
							gchild->pos.y = pp->pos.y;
							gchild->pos.z = pp->pos.z;
						}
						if (gchild->appsrt != pp->appsrt) {
							if (pp->appsrt == NULL) {
								if (gchild->appsrt != NULL) {
									SyncParticleGeneratorFromJoint(gchild);
									gchild->pos.x -= gchild->appsrt->translate.x;
									gchild->pos.y -= gchild->appsrt->translate.y;
									gchild->pos.z -= gchild->appsrt->translate.z;
								}
							} else {
								SyncParticleGeneratorFromJoint(pp->appsrt->gp);
								if (gchild->appsrt != NULL) {
									SyncParticleGeneratorFromJoint(gchild);
									gchild->pos.x += pp->appsrt->translate.x - gchild->appsrt->translate.x;
									gchild->pos.y += pp->appsrt->translate.y - gchild->appsrt->translate.y;
									gchild->pos.z += pp->appsrt->translate.z - gchild->appsrt->translate.z;
								} else {
									gchild->pos.x += pp->appsrt->translate.x;
									gchild->pos.y += pp->appsrt->translate.y;
									gchild->pos.z += pp->appsrt->translate.z;
								}
							}
						}
					}
					break;
				}

				case 0xF0: {
					s32 idx;
					u32 flags;
					HSD_Generator* gchild;
					idx = *pc++ << 8;
					idx += *pc++;
					flags = *pc++;
					if (BANK_REF(pp->bank) != NULL) {
						idx = ((u32*)BANK_REF(pp->bank))[idx];
					}
					gchild = psCreateGeneratorID(pp->linkNo, pp->bank, idx);
					if (gchild != NULL) {
						gchild->idnum = pp->idnum;
						if (pp->gen != NULL) {
							AttachJointToParticleGenerator(gchild, pp->gen->jobj);
						}
						if (pp->appsrt != NULL) {
							if (pp->gen != NULL && (pp->gen->type & 0x2000)) {
								AttachGeneratorAppSRT(gchild, pp->appsrt);
							} else {
								psAttachGeneratorAppSRT(gchild, pp->appsrt);
							}
						}
						gchild->kind &= 0xF1FFFFFF;
						gchild->kind |= (flags & 7) << 25;
						if (pp->appsrt != NULL) {
							if (gchild->appsrt != NULL) {
								gchild->pos.x = pp->pos.x;
								gchild->pos.y = pp->pos.y;
								gchild->pos.z = pp->pos.z;
								if (gchild->appsrt != pp->appsrt) {
									gchild->appsrt->translate = pp->appsrt->translate;
								}
							}
						} else if (gchild->appsrt != NULL) {
							gchild->appsrt->translate.x = pp->pos.x;
							gchild->appsrt->translate.y = pp->pos.y;
							gchild->appsrt->translate.z = pp->pos.z;
							gchild->pos.x               = pp->pos.x;
							gchild->pos.y               = pp->pos.y;
							gchild->pos.z               = pp->pos.z;
						} else {
							gchild->pos.x = pp->pos.x;
							gchild->pos.y = pp->pos.y;
							gchild->pos.z = pp->pos.z;
						}
						if (gchild->appsrt != pp->appsrt) {
							if (pp->appsrt == NULL) {
								if (gchild->appsrt != NULL) {
									SyncParticleGeneratorFromJoint(gchild);
									gchild->pos.x -= gchild->appsrt->translate.x;
									gchild->pos.y -= gchild->appsrt->translate.y;
									gchild->pos.z -= gchild->appsrt->translate.z;
								}
							} else {
								SyncParticleGeneratorFromJoint(pp->appsrt->gp);
								if (gchild->appsrt != NULL) {
									SyncParticleGeneratorFromJoint(gchild);
									gchild->pos.x += pp->appsrt->translate.x - gchild->appsrt->translate.x;
									gchild->pos.y += pp->appsrt->translate.y - gchild->appsrt->translate.y;
									gchild->pos.z += pp->appsrt->translate.z - gchild->appsrt->translate.z;
								} else {
									gchild->pos.x += pp->appsrt->translate.x;
									gchild->pos.y += pp->appsrt->translate.y;
									gchild->pos.z += pp->appsrt->translate.z;
								}
							}
						}
					}
					break;
				}

				case 0xA6: {
					s32 baseLife;
					s32 randomRange;
					baseLife = *pc++ << 8;
					baseLife += *pc++;
					randomRange = *pc++ << 8;
					randomRange += *pc++;
					pp->life = (u16)(baseLife + (s32)((f32)randomRange * HSD_Randf()));
					break;
				}

				case 0xA7: {
					u8 threshold = *pc++;
					if (threshold >= (s32)(100.0F * HSD_Randf())) {
						pp->life = 1;
						goto exit_loop;
					}
					break;
				}

				case 0xA8: {
					pc = getFloat(pc, &v);
					pp->pos.x += 2.0F * v * HSD_Randf() - v;
					pc = getFloat(pc, &v);
					pp->pos.y += 2.0F * v * HSD_Randf() - v;
					pc = getFloat(pc, &v);
					pp->pos.z += 2.0F * v * HSD_Randf() - v;
					break;
				}

				case 0xA9: {
					pc = getFloat(pc, &v);
					modifyDir(v, pp);
					break;
				}

				case 0xF4: {
					pc = getFloat(pc, &v1);
					pc = getFloat(pc, &v2);
					pc = getFloat(pc, &v3);
					pc = getFloat(pc, &v);
					if (pp->gen != NULL) {
						modifyDirGenBase(v, v1, v2, v3, pp);
					}
					break;
				}

				case 0xF5: {
					if (pp->gen != NULL && pp->gen->appsrt != NULL) {
						s32 done = pp->gen->type & 0x2000;
						pp->gen->type |= 0x2000;
						pp->gen->appsrt->xA2 = 0;
						if (!done) {
							SyncParticleGeneratorFromJoint(pp->gen);
						}
					}
					break;
				}

				case 0xF6: {
					if (pp->gen != NULL && pp->gen->appsrt != NULL) {
						s32 done = pp->gen->type & 0x1000;
						pp->gen->type |= 0x1000;
						if (!done) {
							SyncParticleGeneratorFromJoint(pp->gen);
						}
					}
					break;
				}

				case 0xF7:
					pp->kind |= 0x10000000;
					break;

				case 0xAA: {
					s32 idx;
					s32 range;
					u8 bank;
					idx = *pc++ << 8;
					idx += *pc++;
					range = *pc++ << 8;
					range += *pc++;

					idx += (s32)(range * HSD_Randf());
					bank = pp->bank;
					if (BANK_REF(bank) != NULL) {
						idx = ((u32*)BANK_REF(bank))[idx];
					}

					child = psGenerateParticleID0(pp, pp->linkNo, bank, idx, 0);
					if (child != NULL) {
						child->pos.x = pp->pos.x;
						child->pos.y = pp->pos.y;
						child->pos.z = pp->pos.z;
						child->idnum = pp->idnum;
						child->gen   = pp->gen;
						if (pp->gen != NULL) {
							pp->gen->numChild++;
						}
						if (pp->appsrt != NULL) {
							if (pp->gen != NULL && (pp->gen->type & 0x2000)) {
								AttachParticleAppSRT(child, pp->appsrt);
							} else {
								psAttachParticleAppSRT(child, pp->appsrt);
							}
						}
						psInterpretParticle0(child, pp);
					}
					break;
				}

				case 0xAB: {
					pc = getFloat(pc, &v);
					pp->vel.x *= v;
					pp->vel.y *= v;
					pp->vel.z *= v;
					break;
				}

				case 0xAC: {
					pc = getTime(pc, &pp->sizeCount);
					pc = getFloat(pc, &pp->sizeTarget);
					pc = getFloat(pc, &v);
					pp->sizeTarget += v * HSD_Randf();
					if (pp->sizeCount == 0) {
						pp->size = pp->sizeTarget;
					}
					break;
				}

				case 0xAD:
					pp->kind |= 0x80;
					break;

				case 0xAE:
					pp->kind &= ~(0x20 | 0x40);
					break;

				case 0xAF:
					pp->kind &= ~0x40;
					pp->kind |= 0x20;
					break;

				case 0xB0:
					pp->kind &= ~0x20;
					pp->kind |= 0x40;
					break;

				case 0xB1:
					pp->kind |= (0x20 | 0x40);
					break;

				case 0xB2: {
					f32 x, y, z;
					f32 nx, ny, nz;

					if (pp->appsrt == NULL) {
						break;
					}
					if (pp->appsrt->xA2 != 0) {
						break;
					}
					SyncParticleGeneratorFromJoint(pp->appsrt->gp);
					HSD_MTXSRT(pp->appsrt->mmtx, &pp->appsrt->scale, &pp->appsrt->rot, &pp->appsrt->translate, NULL);
					x  = pp->pos.x;
					y  = pp->pos.y;
					z  = pp->pos.z;
					nx = pp->appsrt->mmtx[0][0] * x + pp->appsrt->mmtx[0][1] * y + pp->appsrt->mmtx[0][2] * z + pp->appsrt->mmtx[0][3];
					ny = pp->appsrt->mmtx[1][0] * x + pp->appsrt->mmtx[1][1] * y + pp->appsrt->mmtx[1][2] * z + pp->appsrt->mmtx[1][3];
					nz = pp->appsrt->mmtx[2][0] * x + pp->appsrt->mmtx[2][1] * y + pp->appsrt->mmtx[2][2] * z + pp->appsrt->mmtx[2][3];
					pp->pos.x = nx;
					pp->pos.y = ny;
					pp->pos.z = nz;
					psRemoveParticleAppSRT(pp);
					break;
				}

				case 0xB3: {
					s32 step;
					if (pp->aCmpCount != 0) {
						step = ((s32)pp->aCmpRemain << 16) / (s32)pp->aCmpCount;
						pp->aCmpParam1
						    = (u8)((((s32)pp->aCmpParam1Target << 16) + step * ((s32)pp->aCmpParam1 - (s32)pp->aCmpParam1Target)) >> 16);
						pp->aCmpParam2
						    = (u8)((((s32)pp->aCmpParam2Target << 16) + step * ((s32)pp->aCmpParam2 - (s32)pp->aCmpParam2Target)) >> 16);
					}
					pc                   = getTime(pc, &pp->aCmpCount);
					pp->aCmpMode         = *pc++;
					pp->aCmpParam1Target = *pc++;
					pp->aCmpParam2Target = *pc++;
					if (pp->aCmpCount == 0) {
						pp->aCmpParam1 = pp->aCmpParam1Target;
						pp->aCmpParam2 = pp->aCmpParam2Target;
						pp->aCmpRemain = 0;
						pp->aCmpCount  = 0;
					} else {
						pp->aCmpRemain = pp->aCmpCount;
					}
					break;
				}

				case 0xB4:
					pp->kind |= 0x200;
					break;

				case 0xB5:
					pp->kind &= ~0x200;
					break;

				case 0xB6: {
					pc = getTime(pc, &pp->rotateCount);
					pc = getFloat(pc, &v);
					pp->rotateTarget += v;
					if (pp->rotateCount == 0) {
						pp->rotate = pp->rotateTarget;
					}
					break;
				}

				case 0xB7: {
					s32 idx = *pc++;
					setVelToJObj(pp, (HSD_JObj*)psPointJObj[idx + pp->pJObjOfs]);
					break;
				}

				case 0xB8: {
					s32 idx = *pc++ + pp->pJObjOfs;
					pc      = getFloat(pc, &v);
					pc      = getFloat(pc, &v1);
					if (applyForceJObj(pp, psPointJObj[idx], v, v1) != 0) {
						pp->life = 1;
						goto exit_loop;
					}
					break;
				}

				case 0xB9: {
					s32 idx = *pc++ << 8;
					idx += *pc++;

					child = psGenerateParticleID0(pp, pp->linkNo, pp->bank, idx, 0);
					if (child != NULL) {
						child->pos.x = pp->pos.x;
						child->pos.y = pp->pos.y;
						child->pos.z = pp->pos.z;
						child->vel.x = pp->vel.x;
						child->vel.y = pp->vel.y;
						child->vel.z = pp->vel.z;
						child->idnum = pp->idnum;
						child->gen   = pp->gen;
						if (pp->gen != NULL) {
							pp->gen->numChild++;
						}
						if (pp->appsrt != NULL) {
							if (pp->gen != NULL && (pp->gen->type & 0x2000)) {
								AttachParticleAppSRT(child, pp->appsrt);
							} else {
								psAttachParticleAppSRT(child, pp->appsrt);
							}
						}
						psInterpretParticle0(child, pp);
					}
					break;
				}

				case 0xF2: {
					u8 bank = pp->bank;
					s32 idx = *pc++ << 8;
					idx += *pc++;
					if (BANK_REF(bank) != NULL) {
						idx = ((u32*)BANK_REF(bank))[idx];
					}

					child = psGenerateParticleID0(pp, pp->linkNo, bank, idx, 0);
					if (child != NULL) {
						child->pos.x = pp->pos.x;
						child->pos.y = pp->pos.y;
						child->pos.z = pp->pos.z;
						child->vel.x = pp->vel.x;
						child->vel.y = pp->vel.y;
						child->vel.z = pp->vel.z;
						child->idnum = pp->idnum;
						child->gen   = pp->gen;
						if (pp->gen != NULL) {
							pp->gen->numChild++;
						}
						if (pp->appsrt != NULL) {
							if (pp->gen != NULL && (pp->gen->type & 0x2000)) {
								AttachParticleAppSRT(child, pp->appsrt);
							} else {
								psAttachParticleAppSRT(child, pp->appsrt);
							}
						}
						psInterpretParticle0(child, pp);
					}
					break;
				}

				case 0xBA: {
					s32 step;

					if (pp->primColCount != 0) {
						step = ((s32)pp->primColRemain << 16) / (s32)pp->primColCount;
						pp->primCol.r
						    = (u8)((((s32)pp->primColTarget.r << 16) + step * ((s32)pp->primCol.r - (s32)pp->primColTarget.r)) >> 16);
						pp->primCol.g
						    = (u8)((((s32)pp->primColTarget.g << 16) + step * ((s32)pp->primCol.g - (s32)pp->primColTarget.g)) >> 16);
						pp->primCol.b
						    = (u8)((((s32)pp->primColTarget.b << 16) + step * ((s32)pp->primCol.b - (s32)pp->primColTarget.b)) >> 16);
						pp->primCol.a
						    = (u8)((((s32)pp->primColTarget.a << 16) + step * ((s32)pp->primCol.a - (s32)pp->primColTarget.a)) >> 16);
					}

					v                   = (f32)((s8)*pc++ * 2) * HSD_Randf();
					pp->primColTarget.r = U8ClampAdd(pp->primColTarget.r, v);
					v                   = (f32)((s8)*pc++ * 2) * HSD_Randf();
					pp->primColTarget.g = U8ClampAdd(pp->primColTarget.g, v);
					v                   = (f32)((s8)*pc++ * 2) * HSD_Randf();
					pp->primColTarget.b = U8ClampAdd(pp->primColTarget.b, v);
					v                   = (f32)((s8)*pc++ * 2) * HSD_Randf();
					pp->primColTarget.a = U8ClampAdd(pp->primColTarget.a, v);

					if (pp->primColCount == 0) {
						pp->primCol = pp->primColTarget;
					} else {
						pp->primColRemain = pp->primColCount;
					}
					break;
				}

				case 0xBB: {
					s32 step;

					if (pp->envColCount != 0) {
						step         = ((s32)pp->envColRemain << 16) / (s32)pp->envColCount;
						pp->envCol.r = (u8)((((s32)pp->envColTarget.r << 16) + step * ((s32)pp->envCol.r - (s32)pp->envColTarget.r)) >> 16);
						pp->envCol.g = (u8)((((s32)pp->envColTarget.g << 16) + step * ((s32)pp->envCol.g - (s32)pp->envColTarget.g)) >> 16);
						pp->envCol.b = (u8)((((s32)pp->envColTarget.b << 16) + step * ((s32)pp->envCol.b - (s32)pp->envColTarget.b)) >> 16);
						pp->envCol.a = (u8)((((s32)pp->envColTarget.a << 16) + step * ((s32)pp->envCol.a - (s32)pp->envColTarget.a)) >> 16);
					}

					v                  = (f32)((s8)*pc++ * 2) * HSD_Randf();
					pp->envColTarget.r = U8ClampAdd(pp->envColTarget.r, v);
					v                  = (f32)((s8)*pc++ * 2) * HSD_Randf();
					pp->envColTarget.g = U8ClampAdd(pp->envColTarget.g, v);
					v                  = (f32)((s8)*pc++ * 2) * HSD_Randf();
					pp->envColTarget.b = U8ClampAdd(pp->envColTarget.b, v);
					v                  = (f32)((s8)*pc++ * 2) * HSD_Randf();
					pp->envColTarget.a = U8ClampAdd(pp->envColTarget.a, v);

					if (pp->envColCount == 0) {
						pp->envCol = pp->envColTarget;
					} else {
						pp->envColRemain = pp->envColCount;
					}
					break;
				}

				case 0xBC: {
					HSD_PSTexGroup* tg;

					pp->poseNum = *pc++;
					v           = *pc++;
					pp->poseNum = v * HSD_Randf() + pp->poseNum;
					tg          = BANK_TEXARRAY(pp->bank)[pp->texGroup];
					if (tg != NULL && tg->texTable != NULL && tg->texTable[pp->poseNum] != NULL) {
						pp->kind |= 0x400;
					}
					break;
				}

				case 0xBD:
					pc = getFloat(pc, &v);
					pc = getFloat(pc, &v1);
					v += v1 * HSD_Randf();
					v1 = sqrtf(pp->vel.x * pp->vel.x + pp->vel.y * pp->vel.y + pp->vel.z * pp->vel.z);
					if (v1 > 1e-10F) {
						v /= v1;
						pp->vel.x *= v;
						pp->vel.y *= v;
						pp->vel.z *= v;
					}
					break;

				case 0xBE: {
					pc = getFloat(pc, &v);
					pp->vel.x *= v;
					pc = getFloat(pc, &v);
					pp->vel.y *= v;
					pc = getFloat(pc, &v);
					pp->vel.z *= v;
					break;
				}

				case 0xBF: {
					u8 idx = *pc++;
					pp->kind |= (((idx + pp->pJObjOfs) & 7) << 12) | 0x8000;
					break;
				}

				case 0xC0: {
					s32 step;
					if (pp->primColCount != 0) {
						step = ((s32)pp->primColRemain << 16) / (s32)pp->primColCount;
						pp->primCol.r
						    = (u8)((((s32)pp->primColTarget.r << 16) + step * ((s32)pp->primCol.r - (s32)pp->primColTarget.r)) >> 16);
						pp->primCol.g
						    = (u8)((((s32)pp->primColTarget.g << 16) + step * ((s32)pp->primCol.g - (s32)pp->primColTarget.g)) >> 16);
						pp->primCol.b
						    = (u8)((((s32)pp->primColTarget.b << 16) + step * ((s32)pp->primCol.b - (s32)pp->primColTarget.b)) >> 16);
						pp->primCol.a
						    = (u8)((((s32)pp->primColTarget.a << 16) + step * ((s32)pp->primCol.a - (s32)pp->primColTarget.a)) >> 16);
					}
					pc                = getTime(pc, &pp->primColCount);
					pp->primColTarget = pp->primCol;
					if (opcode & 1) {
						pp->primColTarget.r = *pc++;
					}
					if (opcode & 2) {
						pp->primColTarget.g = *pc++;
					}
					if (opcode & 4) {
						pp->primColTarget.b = *pc++;
					}
					if (opcode & 8) {
						pp->primColTarget.a = *pc++;
					}
					if (pp->primColCount == 0) {
						pp->primCol       = pp->primColTarget;
						pp->primColRemain = 0;
					} else {
						pp->primColRemain = pp->primColCount;
					}
					break;
				}

				case 0xD0: {
					s32 step;
					if (pp->envColCount != 0) {
						step         = ((s32)pp->envColRemain << 16) / (s32)pp->envColCount;
						pp->envCol.r = (u8)((((s32)pp->envColTarget.r << 16) + step * ((s32)pp->envCol.r - (s32)pp->envColTarget.r)) >> 16);
						pp->envCol.g = (u8)((((s32)pp->envColTarget.g << 16) + step * ((s32)pp->envCol.g - (s32)pp->envColTarget.g)) >> 16);
						pp->envCol.b = (u8)((((s32)pp->envColTarget.b << 16) + step * ((s32)pp->envCol.b - (s32)pp->envColTarget.b)) >> 16);
						pp->envCol.a = (u8)((((s32)pp->envColTarget.a << 16) + step * ((s32)pp->envCol.a - (s32)pp->envColTarget.a)) >> 16);
					}
					pc               = getTime(pc, &pp->envColCount);
					pp->envColTarget = pp->envCol;
					if (opcode & 1) {
						pp->envColTarget.r = *pc++;
					}
					if (opcode & 2) {
						pp->envColTarget.g = *pc++;
					}
					if (opcode & 4) {
						pp->envColTarget.b = *pc++;
					}
					if (opcode & 8) {
						pp->envColTarget.a = *pc++;
					}
					if (pp->envColCount == 0) {
						pp->envCol       = pp->envColTarget;
						pp->envColRemain = 0;
					} else {
						pp->envColRemain = pp->envColCount;
					}
					break;
				}

				case 0xE0: {
					s32 step;

					if (pp->primColCount != 0) {
						step = ((s32)pp->primColRemain << 16) / (s32)pp->primColCount;
						pp->primCol.r
						    = (u8)((((s32)pp->primColTarget.r << 16) + step * ((s32)pp->primCol.r - (s32)pp->primColTarget.r)) >> 16);
						pp->primCol.g
						    = (u8)((((s32)pp->primColTarget.g << 16) + step * ((s32)pp->primCol.g - (s32)pp->primColTarget.g)) >> 16);
						pp->primCol.b
						    = (u8)((((s32)pp->primColTarget.b << 16) + step * ((s32)pp->primCol.b - (s32)pp->primColTarget.b)) >> 16);
						pp->primCol.a
						    = (u8)((((s32)pp->primColTarget.a << 16) + step * ((s32)pp->primCol.a - (s32)pp->primColTarget.a)) >> 16);
					}
					if (pp->envColCount != 0) {
						step         = ((s32)pp->envColRemain << 16) / (s32)pp->envColCount;
						pp->envCol.r = (u8)((((s32)pp->envColTarget.r << 16) + step * ((s32)pp->envCol.r - (s32)pp->envColTarget.r)) >> 16);
						pp->envCol.g = (u8)((((s32)pp->envColTarget.g << 16) + step * ((s32)pp->envCol.g - (s32)pp->envColTarget.g)) >> 16);
						pp->envCol.b = (u8)((((s32)pp->envColTarget.b << 16) + step * ((s32)pp->envCol.b - (s32)pp->envColTarget.b)) >> 16);
						pp->envCol.a = (u8)((((s32)pp->envColTarget.a << 16) + step * ((s32)pp->envCol.a - (s32)pp->envColTarget.a)) >> 16);
					}

					v                   = (f32)((s8)*pc++ * 2) * HSD_Randf();
					pp->primColTarget.r = U8ClampAdd(pp->primColTarget.r, v);
					pp->envColTarget.r  = U8ClampAdd(pp->envColTarget.r, v);
					v                   = (f32)((s8)*pc++ * 2) * HSD_Randf();
					pp->primColTarget.g = U8ClampAdd(pp->primColTarget.g, v);
					pp->envColTarget.g  = U8ClampAdd(pp->envColTarget.g, v);
					v                   = (f32)((s8)*pc++ * 2) * HSD_Randf();
					pp->primColTarget.b = U8ClampAdd(pp->primColTarget.b, v);
					pp->envColTarget.b  = U8ClampAdd(pp->envColTarget.b, v);
					v                   = (f32)((s8)*pc++ * 2) * HSD_Randf();
					pp->primColTarget.a = U8ClampAdd(pp->primColTarget.a, v);
					pp->envColTarget.a  = U8ClampAdd(pp->envColTarget.a, v);

					if (pp->primColCount == 0) {
						pp->primCol = pp->primColTarget;
					}
					pp->primColRemain = pp->primColCount;

					if (pp->envColCount == 0) {
						pp->envCol = pp->envColTarget;
					}
					pp->envColRemain = pp->envColCount;
					break;
				}

				case 0xE9: {
					s32 step;
					s32 timing;
					u32 flags;

					if (pp->primColCount != 0) {
						step = ((s32)pp->primColRemain << 16) / (s32)pp->primColCount;
						pp->primCol.r
						    = (u8)((((s32)pp->primColTarget.r << 16) + step * ((s32)pp->primCol.r - (s32)pp->primColTarget.r)) >> 16);
						pp->primCol.g
						    = (u8)((((s32)pp->primColTarget.g << 16) + step * ((s32)pp->primCol.g - (s32)pp->primColTarget.g)) >> 16);
						pp->primCol.b
						    = (u8)((((s32)pp->primColTarget.b << 16) + step * ((s32)pp->primCol.b - (s32)pp->primColTarget.b)) >> 16);
						pp->primCol.a
						    = (u8)((((s32)pp->primColTarget.a << 16) + step * ((s32)pp->primCol.a - (s32)pp->primColTarget.a)) >> 16);
					}
					if (pp->envColCount != 0) {
						step         = ((s32)pp->envColRemain << 16) / (s32)pp->envColCount;
						pp->envCol.r = (u8)((((s32)pp->envColTarget.r << 16) + step * ((s32)pp->envCol.r - (s32)pp->envColTarget.r)) >> 16);
						pp->envCol.g = (u8)((((s32)pp->envColTarget.g << 16) + step * ((s32)pp->envCol.g - (s32)pp->envColTarget.g)) >> 16);
						pp->envCol.b = (u8)((((s32)pp->envColTarget.b << 16) + step * ((s32)pp->envCol.b - (s32)pp->envColTarget.b)) >> 16);
						pp->envCol.a = (u8)((((s32)pp->envColTarget.a << 16) + step * ((s32)pp->envCol.a - (s32)pp->envColTarget.a)) >> 16);
					}

					flags  = *pc++;
					timing = *pc++;

					if (timing != 0) {
						v = (s32)((timing + 1) * HSD_Randf()) / (f32)timing;
					} else {
						v = HSD_Randf();
					}
					if (flags & 1) {
						v1 = v * ((s8)*pc++ * 2);
						if (flags & 0x10) {
							pp->primColTarget.r = U8ClampAdd(pp->primColTarget.r, v1);
						}
						if (flags & 0x20) {
							pp->envColTarget.r = U8ClampAdd(pp->envColTarget.r, v1);
						}
					}
					if (flags & 2) {
						v1 = v * ((s8)*pc++ * 2);
						if (flags & 0x10) {
							pp->primColTarget.g = U8ClampAdd(pp->primColTarget.g, v1);
						}
						if (flags & 0x20) {
							pp->envColTarget.g = U8ClampAdd(pp->envColTarget.g, v1);
						}
					}
					if (flags & 4) {
						v1 = v * ((s8)*pc++ * 2);
						if (flags & 0x10) {
							pp->primColTarget.b = U8ClampAdd(pp->primColTarget.b, v1);
						}
						if (flags & 0x20) {
							pp->envColTarget.b = U8ClampAdd(pp->envColTarget.b, v1);
						}
					}
					if (flags & 8) {
						if (timing != 0) {
							v = (f32)((s8)*pc++ * 2) * (s32)((timing + 1) * HSD_Randf()) / timing;
						} else {
							v = (s8)*pc++ * 2 * HSD_Randf();
						}
						if (flags & 0x10) {
							pp->primColTarget.a = U8ClampAdd(pp->primColTarget.a, v);
						}
						if (flags & 0x20) {
							pp->envColTarget.a = U8ClampAdd(pp->envColTarget.a, v);
						}
					}

					if (pp->primColCount == 0) {
						pp->primCol = pp->primColTarget;
					}
					pp->primColRemain = pp->primColCount;

					if (pp->envColCount == 0) {
						pp->envCol = pp->envColTarget;
					}
					pp->envColRemain = pp->envColCount;
					break;
				}

				case 0xE1: {
					s32 idx = *pc++;
					if (idx == 0) {
						pp->callback = NULL;
					} else {
						pp->callback = lbl_8065BA80[idx - 1];
					}
					break;
				}

				case 0xEC: {
					s32 idx = *pc++;
					pc      = getFloat(pc, &v);
					if (pp->gen->userfunc != NULL && pp->gen->userfunc->setUserData != NULL) {
						pp->gen->userfunc->setUserData(pp, (u8)idx, v);
					} else if (pp->userdata != NULL) {
						pp->userdata[idx] = v;
					}
					break;
				}

				case 0xE2:
					pp->kind |= 8;
					break;

				case 0xE3:
					pp->palNum = *pc++;
					break;

				case 0xE4: {
					u8 mode = *pc++ & 3;
					switch (mode) {
					case 0:
						pp->kind &= ~0x40000;
						break;
					case 1:
						pp->kind |= 0x40000;
						break;
					case 2:
						pp->kind ^= 0x40000;
						break;
					case 3:
						if (HSD_Randf() < 0.5F) {
							pp->kind &= ~0x40000;
						} else {
							pp->kind |= 0x40000;
						}
						break;
					}
					break;
				}

				case 0xE5: {
					u8 mode = *pc++ & 3;
					switch (mode) {
					case 0:
						pp->kind &= ~0x80000;
						break;
					case 1:
						pp->kind |= 0x80000;
						break;
					case 2:
						pp->kind ^= 0x80000;
						break;
					case 3:
						if (HSD_Randf() < 0.5F) {
							pp->kind &= ~0x80000;
						} else {
							pp->kind |= 0x80000;
						}
						break;
					}
					break;
				}

				case 0xE6:
					pp->kind |= 0x200000;
					break;

				case 0xE7:
					pp->kind &= ~0x200000;
					break;

				case 0xE8: {
					pc = getFloat(pc, &v);
					if (v < 0.0F) {
						pp->kind &= ~0x100000;
					} else {
						pp->kind |= 0x100000;
						pp->trail = v;
					}
					break;
				}

				case 0xEA: {
					s32 step;
					u32 flags;

					if (pp->matColCount != 0) {
						step       = ((s32)pp->matColRemain << 16) / (s32)pp->matColCount;
						pp->matRGB = (u8)((((s32)pp->matRGBTarget << 16) + step * ((s32)pp->matRGB - (s32)pp->matRGBTarget)) >> 16);
						pp->matA   = (u8)((((s32)pp->matATarget << 16) + step * ((s32)pp->matA - (s32)pp->matATarget)) >> 16);
					}
					pc               = getTime(pc, &pp->matColCount);
					flags            = *pc++;
					pp->matRGBTarget = pp->matRGB;
					if (flags & 1) {
						pp->matRGBTarget = *pc++;
					}
					if (flags & 8) {
						pp->matATarget = *pc++;
					}
					if (pp->matColCount == 0) {
						pp->matRGB       = pp->matRGBTarget;
						pp->matColRemain = 0;
					} else {
						pp->matColRemain = pp->matColCount;
					}
					break;
				}

				case 0xEB: {
					s32 step;
					u32 flags;

					if (pp->ambColCount != 0) {
						step       = ((s32)pp->ambColRemain << 16) / (s32)pp->ambColCount;
						pp->ambRGB = (u8)((((s32)pp->ambRGBTarget << 16) + step * ((s32)pp->ambRGB - (s32)pp->ambRGBTarget)) >> 16);
						pp->ambA   = (u8)((((s32)pp->ambATarget << 16) + step * ((s32)pp->ambA - (s32)pp->ambATarget)) >> 16);
					}
					pc               = getTime(pc, &pp->ambColCount);
					flags            = *pc++;
					pp->ambRGBTarget = pp->ambRGB;
					if (flags & 1) {
						pp->ambRGBTarget = *pc++;
					}
					if (flags & 8) {
						pp->ambATarget = *pc++;
					}
					if (pp->ambColCount == 0) {
						pp->ambRGB       = pp->ambRGBTarget;
						pp->ambColRemain = 0;
					} else {
						pp->ambColRemain = pp->ambColCount;
					}
					break;
				}

				case 0xED: {
					s32 timing;
					pc     = getFloat(pc, &v);
					pc     = getFloat(pc, &v1);
					timing = *pc++;
					if (timing != 0) {
						v = v + v1 * (s32)((timing + 1) * HSD_Randf()) / timing;
					} else {
						v = v + v1 * HSD_Randf();
					}
					pp->rotateTarget += v;
					pp->rotate += v;
					break;
				}

				case 0xF3: {
					s32 mode = *pc++;
					pc       = getFloat(pc, &pp->rotateTarget);
					pc       = getFloat(pc, &pp->spreadRate);
					pc       = getTime(pc, &pp->rotateCount);
					if (pp->rotateCount != 0) {
						if (mode == 0) {
							pp->rotateTarget += pp->spreadRate / 2.0;
						} else {
							pp->rotateTarget *= -1.0F;
							pp->rotateTarget -= pp->spreadRate / 2.0;
						}
					} else {
						pp->rotateTarget = 0.0F;
						pp->spreadRate   = 0.0F;
					}
					break;
				}

				case 0xFA:
					pp->loopCount  = *pc++;
					pp->cmdLoopPtr = (u16)(pc - pp->cmdList);
					break;

				case 0xFB:
					if (--pp->loopCount != 0) {
						pc = pp->cmdList + pp->cmdLoopPtr;
					}
					break;

				case 0xFC:
					pp->cmdMarkPtr = (u16)(pc - pp->cmdList);
					break;

				case 0xFD:
					pc = pp->cmdList + pp->cmdMarkPtr;
					break;

				case 0xFE:
				case 0xFF:
					pp->life = 1;
					goto exit_loop;

				default:
					break;
				}
			}

		} while (operand == 0);

	exit_loop:
		pp->cmdPtr  = (u16)(pc - pp->cmdList);
		pp->cmdWait = (u16)operand;
	}

	if (--pp->life == 0) {
		HSD_Particle* next;

	kill:
		if (pp->gen != NULL && pp->gen->userfunc != NULL && pp->gen->userfunc->hookDelete != NULL) {
			pp->gen->userfunc->hookDelete(pp);
		}
		if (pp->gen != NULL) {
			pp->gen->numChild--;
		}
		next = _psListGetNext(pp);
		if (pp->appsrt != NULL && psRemoveParticleAppSRT(pp) == 0 && prev == NULL && _psListGetFirst(pp->linkNo) != next) {
			next = _psListGetFirst(pp->linkNo);
		}
		psDeletePntJObjwithParticle(pp);
		_psListDelete(pp, prev);
		return next;
	}

	if (pp->kind & 4) {
		HSD_Generator* gp = pp->gen;
		f32 sinA, sinB, cosA, cosB;
		f32 R;
		f32 d, nd, e, vz;

		sinA = sinf(pp->grav);
		sinB = sinf(pp->fric);
		cosA = cosf(pp->grav);
		cosB = cosf(pp->fric);

		pp->vel.z += gp->aux.tornado.vel;

		R = PS_ABS(gp->radius);
		R = pp->vel.z * tanf(PS_ABS(gp->angle)) + R;

		R *= pp->vel.y;
		pp->vel.x += gp->grav;

		d  = R * cosf(pp->vel.x);
		e  = R * sinf(pp->vel.x);
		nd = -d;
		vz = pp->vel.z;

		pp->pos.x = gp->pos.x + (d * cosB + vz * sinB);
		pp->pos.y = gp->pos.y + (cosB * (vz * sinA) + (sinB * (nd * sinA) + e * cosA));
		pp->pos.z = gp->pos.z + (cosB * (vz * cosA) + (sinB * (nd * cosA) - e * sinA));
	} else {
		if (pp->kind & 1) {
			pp->vel.y -= pp->grav;
		}
		if (pp->kind & 2) {
			pp->vel.x *= pp->fric;
			pp->vel.y *= pp->fric;
			pp->vel.z *= pp->fric;
		}
		pp->pos.x += pp->vel.x;
		pp->pos.y += pp->vel.y;
		pp->pos.z += pp->vel.z;
	}

	if (pp->kind & 0x8000) {
		s32 jobjIdx = (pp->kind >> 12) & 7;
		HSD_JObj* jobj;

		if (psPointJObj[jobjIdx] == NULL) {
			HSD_JObj* newJObj = HSD_JObjAlloc();
			if (newJObj != NULL) {
				psSetPointJObj(jobjIdx + 1, newJObj);
				HSD_JObjUnref(newJObj);
			}
		}

		jobj = psPointJObj[jobjIdx];
		if (jobj != NULL) {
			SetupDirtyParticleJointMatrix(jobj);
			jobj = psPointJObj[jobjIdx];
			HSD_JObjAddTx(jobj, pp->pos.x - jobj->mtx[0][3]);
			jobj = psPointJObj[jobjIdx];
			HSD_JObjAddTy(jobj, pp->pos.y - jobj->mtx[1][3]);
			jobj = psPointJObj[jobjIdx];
			HSD_JObjAddTz(jobj, pp->pos.z - jobj->mtx[2][3]);
		}
	}

	if (pp->callback != NULL && pp->callback(pp) == -1) {
		goto kill;
	}
	return _psListGetNext(pp);
}

/**
 * @note Address: 0x801E235C
 * @note Size: 0xD8
 */
void psInterpretParticles(u32 flags)
{
	HSD_Particle* pp;
	HSD_Particle* next;
	HSD_Particle* lastPP;
	s32 linkNo;
	HSD_Particle* ret;

	for (linkNo = 0; linkNo < 16; linkNo++, flags >>= 1) {
		if (!(flags & 0x10000)) {
			lastPP = NULL;
			pp     = _psListGetFirst(linkNo);

			while (pp != NULL) {
				next = pp->next;
				ret  = psInterpretParticle0(pp, lastPP);

				if (next != NULL) {
					if (pp->next == next) {
						lastPP = pp;
					} else if (ret != next) {
						lastPP = ret;
						while (lastPP->next != next) {
							assert_line(0x703, lastPP);
							lastPP = lastPP->next;
						}
					}
				}
				pp = next;
			}
		}
	}
}

/**
 * @note Address: 0x801DE4F0
 * @note Size: 0x170
 */
// SyncParticleGeneratorFromJoint

/**
 * @note Address: 0x801DE9E4
 * @note Size: 0x2F8
 */
// modifyDir
/*
    stwu     r1, -0xa0(r1)
    mflr     r0
    stw      r0, 0xa4(r1)
    stfd     f31, 0x90(r1)
    psq_st   f31, 0x98(r1), 0, qr0
    stfd     f30, 0x80(r1)
    psq_st   f30, 0x88(r1), 0, qr0
    stfd     f29, 0x70(r1)
    psq_st   f29, 0x78(r1), 0, qr0
    stfd     f28, 0x60(r1)
    psq_st   f28, 0x68(r1), 0, qr0
    stfd     f27, 0x50(r1)
    psq_st   f27, 0x58(r1), 0, qr0
    stfd     f26, 0x40(r1)
    psq_st   f26, 0x48(r1), 0, qr0
    stfd     f25, 0x30(r1)
    psq_st   f25, 0x38(r1), 0, qr0
    stfd     f24, 0x20(r1)
    psq_st   f24, 0x28(r1), 0, qr0
    stw      r31, 0x1c(r1)
    mr       r31, r3
    lis      r3, __float_min@ha
    lfs      f26, 0x34(r31)
    fmr      f25, f1
    lfs      f0, __float_min@l(r3)
    fabs     f1, f26
    lfs      f27, 0x2c(r31)
    lfs      f28, 0x30(r31)
    fcmpo    cr0, f1, f0
    bge      .L_801DEA7C
    lfs      f0, lbl_8065FD70@sda21(r0)
    fcmpo    cr0, f28, f0
    cror     eq, gt, eq
    bne      .L_801DEA74
    lfs      f24, lbl_8065FD74@sda21(r0)
    b        .L_801DEA90
.L_801DEA74:
    lfs      f24, lbl_8065FD78@sda21(r0)
    b        .L_801DEA90
.L_801DEA7C:
    fmr      f1, f28
    fmr      f2, f26
    bl       atan2
    frsp     f0, f1
    fmr      f24, f0
.L_801DEA90:
    fmr      f1, f24
    bl       sin
    frsp     f31, f1
    fmr      f1, f24
    bl       cos
    frsp     f30, f1
    lis      r3, __float_min@ha
    lfs      f0, __float_min@l(r3)
    fmuls    f1, f26, f30
    fmadds   f2, f28, f31, f1
    fabs     f1, f2
    fcmpo    cr0, f1, f0
    bge      .L_801DEAE4
    lfs      f0, lbl_8065FD70@sda21(r0)
    fcmpo    cr0, f27, f0
    cror     eq, gt, eq
    bne      .L_801DEADC
    lfs      f24, lbl_8065FD74@sda21(r0)
    b        .L_801DEAF4
.L_801DEADC:
    lfs      f24, lbl_8065FD78@sda21(r0)
    b        .L_801DEAF4
.L_801DEAE4:
    fmr      f1, f27
    bl       atan2
    frsp     f0, f1
    fmr      f24, f0
.L_801DEAF4:
    fmr      f1, f24
    bl       sin
    frsp     f29, f1
    fmr      f1, f24
    bl       cos
    fmuls    f2, f28, f28
    lfs      f0, lbl_8065FD70@sda21(r0)
    frsp     f28, f1
    fmadds   f1, f27, f27, f2
    fmadds   f27, f26, f26, f1
    fcmpo    cr0, f27, f0
    ble      .L_801DEB6C
    frsqrte  f1, f27
    lfd      f3, lbl_8065FD80@sda21(r0)
    lfd      f2, lbl_8065FD88@sda21(r0)
    fmul     f0, f1, f1
    fmul     f1, f3, f1
    fnmsub   f0, f27, f0, f2
    fmul     f1, f1, f0
    fmul     f0, f1, f1
    fmul     f1, f3, f1
    fnmsub   f0, f27, f0, f2
    fmul     f1, f1, f0
    fmul     f0, f1, f1
    fmul     f1, f3, f1
    fnmsub   f0, f27, f0, f2
    fmul     f0, f1, f0
    fmul     f27, f27, f0
    frsp     f27, f27
    b        .L_801DEBF0
.L_801DEB6C:
    lfd      f0, lbl_8065FD90@sda21(r0)
    fcmpo    cr0, f27, f0
    bge      .L_801DEB84
    lis      r3, __float_nan@ha
    lfs      f27, __float_nan@l(r3)
    b        .L_801DEBF0
.L_801DEB84:
    stfs     f27, 0x8(r1)
    lis      r0, 0x7f80
    lwz      r4, 0x8(r1)
    rlwinm   r3, r4, 0, 1, 8
    cmpw     r3, r0
    beq      .L_801DEBAC
    bge      .L_801DEBDC
    cmpwi    r3, 0x0
    beq      .L_801DEBC4
    b        .L_801DEBDC
.L_801DEBAC:
    clrlwi.  r0, r4, 9
    beq      .L_801DEBBC
    li       r0, 0x1
    b        .L_801DEBE0
.L_801DEBBC:
    li       r0, 0x2
    b        .L_801DEBE0
.L_801DEBC4:
    clrlwi.  r0, r4, 9
    beq      .L_801DEBD4
    li       r0, 0x5
    b        .L_801DEBE0
.L_801DEBD4:
    li       r0, 0x3
    b        .L_801DEBE0
.L_801DEBDC:
    li       r0, 0x4
.L_801DEBE0:
    cmpwi    r0, 0x1
    bne      .L_801DEBF0
    lis      r3, __float_nan@ha
    lfs      f27, __float_nan@l(r3)
.L_801DEBF0:
    bl       HSD_Randf
    lfd      f2, lbl_8065FDA0@sda21(r0)
    lfd      f0, lbl_8065FD98@sda21(r0)
    fmul     f2, f2, f1
    fmr      f1, f25
    fmul     f26, f0, f2
    frsp     f26, f26
    bl       sin
    frsp     f0, f1
    fmr      f1, f26
    fmuls    f24, f27, f0
    bl       cos
    frsp     f0, f1
    fmr      f1, f26
    fmuls    f26, f24, f0
    bl       sin
    frsp     f0, f1
    fmr      f1, f25
    fmuls    f25, f24, f0
    bl       cos
    frsp     f0, f1
    fneg     f1, f26
    fmuls    f2, f25, f30
    fmuls    f6, f27, f0
    fmuls    f3, f1, f31
    fmuls    f1, f1, f30
    fmuls    f5, f6, f29
    fmuls    f0, f25, f31
    fmuls    f4, f6, f31
    fmadds   f3, f29, f3, f2
    fmadds   f5, f26, f28, f5
    fmuls    f2, f6, f30
    fmsubs   f0, f29, f1, f0
    fmadds   f1, f28, f4, f3
    stfs     f5, 0x2c(r31)
    fmadds   f0, f28, f2, f0
    stfs     f1, 0x30(r31)
    stfs     f0, 0x34(r31)
    psq_l    f31, 0x98(r1), 0, qr0
    lfd      f31, 0x90(r1)
    psq_l    f30, 0x88(r1), 0, qr0
    lfd      f30, 0x80(r1)
    psq_l    f29, 0x78(r1), 0, qr0
    lfd      f29, 0x70(r1)
    psq_l    f28, 0x68(r1), 0, qr0
    lfd      f28, 0x60(r1)
    psq_l    f27, 0x58(r1), 0, qr0
    lfd      f27, 0x50(r1)
    psq_l    f26, 0x48(r1), 0, qr0
    lfd      f26, 0x40(r1)
    psq_l    f25, 0x38(r1), 0, qr0
    lfd      f25, 0x30(r1)
    psq_l    f24, 0x28(r1), 0, qr0
    lfd      f24, 0x20(r1)
    lwz      r0, 0xa4(r1)
    lwz      r31, 0x1c(r1)
    mtlr     r0
    addi     r1, r1, 0xa0
    blr
*/

/**
 * @note Address: 0x801DECDC
 * @note Size: 0x2A8
 */
// setVelToJObj

/**
 * @note Address: 0x801DEF84
 * @note Size: 0x144
 */
// applyForceJObj

/**
 * @note Address: 0x801DF0C8
 * @note Size: 0x2F30
 */
// psInterpretParticle0
/*
    stwu     r1, -0xc0(r1)
    mflr     r0
    stw      r0, 0xc4(r1)
    stfd     f31, 0xb0(r1)
    psq_st   f31, 0xb8(r1), 0, qr0
    stfd     f30, 0xa0(r1)
    psq_st   f30, 0xa8(r1), 0, qr0
    stfd     f29, 0x90(r1)
    psq_st   f29, 0x98(r1), 0, qr0
    stfd     f28, 0x80(r1)
    psq_st   f28, 0x88(r1), 0, qr0
    stfd     f27, 0x70(r1)
    psq_st   f27, 0x78(r1), 0, qr0
    stfd     f26, 0x60(r1)
    psq_st   f26, 0x68(r1), 0, qr0
    addi     r11, r1, 0x60
    bl       _savegpr_26
    lwz      r0, 0x4(r3)
    mr       r31, r3
    mr       r28, r4
    rlwinm.  r0, r0, 0, 20, 20
    beq      .L_801DF128
    bl       _psListGetNext
    b        .L_801E1FB0
.L_801DF128:
    lhz      r3, 0xc(r31)
    cmplwi   r3, 0x0
    beq      .L_801DF170
    lis      r0, 0x4330
    stw      r3, 0x1c(r1)
    lfs      f3, 0x4c(r31)
    stw      r0, 0x18(r1)
    lfs      f2, 0x60(r31)
    lfd      f1, "@1009"@sda21(r0)
    lfd      f0, 0x18(r1)
    fsubs    f2, f2, f3
    fsubs    f0, f0, f1
    fdivs    f0, f2, f0
    fadds    f0, f3, f0
    stfs     f0, 0x4c(r31)
    lhz      r3, 0xc(r31)
    subi     r0, r3, 0x1
    sth      r0, 0xc(r31)
.L_801DF170:
    lhz      r0, 0xe(r31)
    cmplwi   r0, 0x0
    beq      .L_801DF1BC
    lhz      r3, 0x6c(r31)
    subi     r0, r3, 0x1
    sth      r0, 0x6c(r31)
    lhz      r0, 0x6c(r31)
    cmplwi   r0, 0x0
    bne      .L_801DF1BC
    li       r0, 0x0
    sth      r0, 0xe(r31)
    lbz      r0, 0x70(r31)
    stb      r0, 0x12(r31)
    lbz      r0, 0x71(r31)
    stb      r0, 0x13(r31)
    lbz      r0, 0x72(r31)
    stb      r0, 0x14(r31)
    lbz      r0, 0x73(r31)
    stb      r0, 0x15(r31)
.L_801DF1BC:
    lhz      r0, 0x10(r31)
    cmplwi   r0, 0x0
    beq      .L_801DF208
    lhz      r3, 0x6e(r31)
    subi     r0, r3, 0x1
    sth      r0, 0x6e(r31)
    lhz      r0, 0x6e(r31)
    cmplwi   r0, 0x0
    bne      .L_801DF208
    li       r0, 0x0
    sth      r0, 0x10(r31)
    lbz      r0, 0x74(r31)
    stb      r0, 0x16(r31)
    lbz      r0, 0x75(r31)
    stb      r0, 0x17(r31)
    lbz      r0, 0x76(r31)
    stb      r0, 0x18(r31)
    lbz      r0, 0x77(r31)
    stb      r0, 0x19(r31)
.L_801DF208:
    lhz      r0, 0x5a(r31)
    cmplwi   r0, 0x0
    beq      .L_801DF244
    lhz      r3, 0x78(r31)
    subi     r0, r3, 0x1
    sth      r0, 0x78(r31)
    lhz      r0, 0x78(r31)
    cmplwi   r0, 0x0
    bne      .L_801DF244
    li       r0, 0x0
    sth      r0, 0x5a(r31)
    lbz      r0, 0x84(r31)
    stb      r0, 0x80(r31)
    lbz      r0, 0x85(r31)
    stb      r0, 0x81(r31)
.L_801DF244:
    lhz      r0, 0x5c(r31)
    cmplwi   r0, 0x0
    beq      .L_801DF280
    lhz      r3, 0x7a(r31)
    subi     r0, r3, 0x1
    sth      r0, 0x7a(r31)
    lhz      r0, 0x7a(r31)
    cmplwi   r0, 0x0
    bne      .L_801DF280
    li       r0, 0x0
    sth      r0, 0x5c(r31)
    lbz      r0, 0x86(r31)
    stb      r0, 0x82(r31)
    lbz      r0, 0x87(r31)
    stb      r0, 0x83(r31)
.L_801DF280:
    lhz      r0, 0x54(r31)
    cmplwi   r0, 0x0
    beq      .L_801DF2BC
    lhz      r3, 0x7c(r31)
    subi     r0, r3, 0x1
    sth      r0, 0x7c(r31)
    lhz      r0, 0x7c(r31)
    cmplwi   r0, 0x0
    bne      .L_801DF2BC
    li       r0, 0x0
    sth      r0, 0x54(r31)
    lbz      r0, 0x7e(r31)
    stb      r0, 0x57(r31)
    lbz      r0, 0x7f(r31)
    stb      r0, 0x58(r31)
.L_801DF2BC:
    lhz      r3, 0x5e(r31)
    cmplwi   r3, 0x0
    beq      .L_801DF374
    lfs      f0, 0x68(r31)
    lfs      f2, "@195"@sda21(r0)
    fcmpu    cr0, f0, f2
    beq      .L_801DF338
    lfs      f1, 0x50(r31)
    lfs      f0, 0x64(r31)
    fadds    f0, f1, f0
    stfs     f0, 0x50(r31)
    lfs      f1, 0x64(r31)
    fcmpo    cr0, f1, f2
    cror     eq, gt, eq
    bne      .L_801DF308
    lfs      f0, 0x68(r31)
    fadds    f0, f1, f0
    stfs     f0, 0x64(r31)
    b        .L_801DF314
.L_801DF308:
    lfs      f0, 0x68(r31)
    fsubs    f0, f1, f0
    stfs     f0, 0x64(r31)
.L_801DF314:
    lhz      r3, 0x5e(r31)
    subi     r3, r3, 0x1
    clrlwi.  r0, r3, 16
    sth      r3, 0x5e(r31)
    bne      .L_801DF374
    lfs      f0, "@195"@sda21(r0)
    stfs     f0, 0x68(r31)
    stfs     f0, 0x64(r31)
    b        .L_801DF374
.L_801DF338:
    lis      r0, 0x4330
    stw      r3, 0x1c(r1)
    lfs      f3, 0x50(r31)
    stw      r0, 0x18(r1)
    lfs      f2, 0x64(r31)
    lfd      f1, "@1009"@sda21(r0)
    lfd      f0, 0x18(r1)
    fsubs    f2, f2, f3
    fsubs    f0, f0, f1
    fdivs    f0, f2, f0
    fadds    f0, f3, f0
    stfs     f0, 0x50(r31)
    lhz      r3, 0x5e(r31)
    subi     r0, r3, 0x1
    sth      r0, 0x5e(r31)
.L_801DF374:
    lhz      r3, 0x1a(r31)
    cmplwi   r3, 0x0
    beq      .L_801E1C98
    subi     r3, r3, 0x1
    clrlwi.  r0, r3, 16
    sth      r3, 0x1a(r31)
    bne      .L_801E1C98
    lwz      r3, 0x20(r31)
    lhz      r0, 0x24(r31)
    add      r30, r3, r0
.L_801DF39C:
    lbz      r26, 0x0(r30)
    addi     r30, r30, 0x1
    cmplwi   r26, 0x80
    bge      .L_801DF450
    rlwinm.  r0, r26, 0, 26, 26
    clrlwi   r3, r26, 27
    mr       r29, r3
    beq      .L_801DF3D0
    lbz      r0, 0x0(r30)
    clrlslwi r3, r3, 16, 8
    addi     r30, r30, 0x1
    add      r0, r3, r0
    clrlwi   r29, r0, 16
.L_801DF3D0:
    rlwinm   r0, r26, 0, 24, 25
    cmpwi    r0, 0x40
    beq      .L_801DF3EC
    bge      .L_801E1C80
    cmpwi    r0, 0x0
    beq      .L_801E1C80
    b        .L_801E1C80
.L_801DF3EC:
    lbz      r0, 0x0(r30)
    lis      r3, psTexGroupArray@ha
    addi     r3, r3, psTexGroupArray@l
    addi     r30, r30, 0x1
    stb      r0, 0xa(r31)
    lbz      r4, 0x8(r31)
    lbz      r0, 0x9(r31)
    slwi     r4, r4, 2
    lwzx     r3, r3, r4
    slwi     r0, r0, 2
    lwzx     r4, r3, r0
    cmplwi   r4, 0x0
    beq      .L_801E1C80
    addic.   r0, r4, 0x18
    beq      .L_801E1C80
    lbz      r0, 0xa(r31)
    slwi     r3, r0, 2
    addi     r0, r3, 0x18
    lwzx     r0, r4, r0
    cmplwi   r0, 0x0
    beq      .L_801E1C80
    lwz      r0, 0x4(r31)
    ori      r0, r0, 0x400
    stw      r0, 0x4(r31)
    b        .L_801E1C80
.L_801DF450:
    rlwinm   r0, r26, 0, 24, 28
    li       r29, 0x0
    cmplwi   r0, 0x98
    ble      .L_801DF478
    rlwinm   r0, r26, 0, 24, 27
    cmplwi   r0, 0xc0
    beq      .L_801DF478
    cmplwi   r0, 0xd0
    beq      .L_801DF478
    mr       r0, r26
.L_801DF478:
    clrlwi   r3, r0, 24
    subi     r0, r3, 0x80
    cmplwi   r0, 0x7f
    bgt      .L_801E1C80
    lis      r3, "@1010"@ha
    slwi     r0, r0, 2
    addi     r3, r3, "@1010"@l
    lwzx     r0, r3, r0
    mtctr    r0
    bctr
.L_801DF4A0:
    clrlwi.  r0, r26, 31
    beq      .L_801DF4B8
    mr       r3, r30
    addi     r4, r31, 0x40
    bl       getFloat
    mr       r30, r3
.L_801DF4B8:
    rlwinm.  r0, r26, 0, 30, 30
    beq      .L_801DF4D0
    mr       r3, r30
    addi     r4, r31, 0x44
    bl       getFloat
    mr       r30, r3
.L_801DF4D0:
    rlwinm.  r0, r26, 0, 29, 29
    beq      .L_801E1C80
    mr       r3, r30
    addi     r4, r31, 0x48
    bl       getFloat
    mr       r30, r3
    b        .L_801E1C80
.L_801DF4EC:
    clrlwi.  r0, r26, 31
    beq      .L_801DF514
    mr       r3, r30
    addi     r4, r1, 0x14
    bl       getFloat
    lfs      f1, 0x40(r31)
    mr       r30, r3
    lfs      f0, 0x14(r1)
    fadds    f0, f1, f0
    stfs     f0, 0x40(r31)
.L_801DF514:
    rlwinm.  r0, r26, 0, 30, 30
    beq      .L_801DF53C
    mr       r3, r30
    addi     r4, r1, 0x14
    bl       getFloat
    lfs      f1, 0x44(r31)
    mr       r30, r3
    lfs      f0, 0x14(r1)
    fadds    f0, f1, f0
    stfs     f0, 0x44(r31)
.L_801DF53C:
    rlwinm.  r0, r26, 0, 29, 29
    beq      .L_801E1C80
    mr       r3, r30
    addi     r4, r1, 0x14
    bl       getFloat
    lfs      f1, 0x48(r31)
    mr       r30, r3
    lfs      f0, 0x14(r1)
    fadds    f0, f1, f0
    stfs     f0, 0x48(r31)
    b        .L_801E1C80
.L_801DF568:
    clrlwi.  r0, r26, 31
    beq      .L_801DF580
    mr       r3, r30
    addi     r4, r31, 0x2c
    bl       getFloat
    mr       r30, r3
.L_801DF580:
    rlwinm.  r0, r26, 0, 30, 30
    beq      .L_801DF598
    mr       r3, r30
    addi     r4, r31, 0x30
    bl       getFloat
    mr       r30, r3
.L_801DF598:
    rlwinm.  r0, r26, 0, 29, 29
    beq      .L_801E1C80
    mr       r3, r30
    addi     r4, r31, 0x34
    bl       getFloat
    mr       r30, r3
    b        .L_801E1C80
.L_801DF5B4:
    clrlwi.  r0, r26, 31
    beq      .L_801DF5DC
    mr       r3, r30
    addi     r4, r1, 0x14
    bl       getFloat
    lfs      f1, 0x2c(r31)
    mr       r30, r3
    lfs      f0, 0x14(r1)
    fadds    f0, f1, f0
    stfs     f0, 0x2c(r31)
.L_801DF5DC:
    rlwinm.  r0, r26, 0, 30, 30
    beq      .L_801DF604
    mr       r3, r30
    addi     r4, r1, 0x14
    bl       getFloat
    lfs      f1, 0x30(r31)
    mr       r30, r3
    lfs      f0, 0x14(r1)
    fadds    f0, f1, f0
    stfs     f0, 0x30(r31)
.L_801DF604:
    rlwinm.  r0, r26, 0, 29, 29
    beq      .L_801E1C80
    mr       r3, r30
    addi     r4, r1, 0x14
    bl       getFloat
    lfs      f1, 0x34(r31)
    mr       r30, r3
    lfs      f0, 0x14(r1)
    fadds    f0, f1, f0
    stfs     f0, 0x34(r31)
    b        .L_801E1C80
.L_801DF630:
    mr       r3, r30
    addi     r4, r31, 0xc
    bl       getTime
    addi     r4, r31, 0x60
    bl       getFloat
    lhz      r0, 0xc(r31)
    mr       r30, r3
    cmplwi   r0, 0x0
    bne      .L_801E1C80
    lfs      f0, 0x60(r31)
    stfs     f0, 0x4c(r31)
    b        .L_801E1C80
.L_801DF660:
    lwz      r0, 0x4(r31)
    rlwinm   r0, r0, 0, 22, 20
    stw      r0, 0x4(r31)
    b        .L_801E1C80
.L_801DF670:
    mr       r3, r30
    addi     r4, r31, 0x38
    bl       getFloat
    lfs      f1, "@195"@sda21(r0)
    mr       r30, r3
    lfs      f0, 0x38(r31)
    fcmpu    cr0, f1, f0
    bne      .L_801DF6A0
    lwz      r0, 0x4(r31)
    clrrwi   r0, r0, 1
    stw      r0, 0x4(r31)
    b        .L_801E1C80
.L_801DF6A0:
    lwz      r0, 0x4(r31)
    ori      r0, r0, 0x1
    stw      r0, 0x4(r31)
    b        .L_801E1C80
.L_801DF6B0:
    mr       r3, r30
    addi     r4, r31, 0x3c
    bl       getFloat
    lfs      f1, "@1002"@sda21(r0)
    mr       r30, r3
    lfs      f0, 0x3c(r31)
    fcmpu    cr0, f1, f0
    bne      .L_801DF6E0
    lwz      r0, 0x4(r31)
    rlwinm   r0, r0, 0, 31, 29
    stw      r0, 0x4(r31)
    b        .L_801E1C80
.L_801DF6E0:
    lwz      r0, 0x4(r31)
    ori      r0, r0, 0x2
    stw      r0, 0x4(r31)
    b        .L_801E1C80
.L_801DF6F0:
    lbz      r4, 0x0(r30)
    mr       r3, r31
    lbz      r0, 0x1(r30)
    li       r7, 0x0
    clrlslwi r6, r4, 24, 8
    lbz      r4, 0x1d(r31)
    lbz      r5, 0x8(r31)
    add      r6, r6, r0
    addi     r30, r30, 0x2
    bl       psGenerateParticleID0
    mr.      r26, r3
    beq      .L_801E1C80
    lhz      r0, 0x1e(r31)
    sth      r0, 0x1e(r26)
    lwz      r0, 0x8c(r31)
    stw      r0, 0x8c(r26)
    lwz      r4, 0x8c(r31)
    cmplwi   r4, 0x0
    beq      .L_801DF748
    lwz      r3, 0x50(r4)
    addi     r0, r3, 0x1
    stw      r0, 0x50(r4)
.L_801DF748:
    lwz      r3, 0x8c(r31)
    cmplwi   r3, 0x0
    beq      .L_801DF770
    lhz      r0, 0x16(r3)
    rlwinm.  r0, r0, 0, 18, 18
    beq      .L_801DF770
    lwz      r4, 0x90(r31)
    mr       r3, r26
    bl       AttachParticleAppSRT
    b        .L_801DF77C
.L_801DF770:
    lwz      r4, 0x90(r31)
    mr       r3, r26
    bl       psAttachParticleAppSRT
.L_801DF77C:
    lfs      f0, 0x40(r31)
    mr       r3, r26
    mr       r4, r31
    stfs     f0, 0x40(r26)
    lfs      f0, 0x44(r31)
    stfs     f0, 0x44(r26)
    lfs      f0, 0x48(r31)
    stfs     f0, 0x48(r26)
    bl       psInterpretParticle0
    b        .L_801E1C80
.L_801DF7A4:
    lbz      r5, 0x8(r31)
    lis      r3, ptclref@ha
    lbz      r6, 0x0(r30)
    addi     r3, r3, ptclref@l
    clrlslwi r0, r5, 24, 2
    lbz      r4, 0x1(r30)
    lwzx     r3, r3, r0
    clrlslwi r6, r6, 24, 8
    add      r6, r6, r4
    addi     r30, r30, 0x2
    cmplwi   r3, 0x0
    beq      .L_801DF7DC
    slwi     r0, r6, 2
    lwzx     r6, r3, r0
.L_801DF7DC:
    lbz      r4, 0x1d(r31)
    mr       r3, r31
    li       r7, 0x0
    bl       psGenerateParticleID0
    mr.      r26, r3
    beq      .L_801E1C80
    lhz      r0, 0x1e(r31)
    sth      r0, 0x1e(r26)
    lwz      r0, 0x8c(r31)
    stw      r0, 0x8c(r26)
    lwz      r4, 0x8c(r31)
    cmplwi   r4, 0x0
    beq      .L_801DF81C
    lwz      r3, 0x50(r4)
    addi     r0, r3, 0x1
    stw      r0, 0x50(r4)
.L_801DF81C:
    lwz      r3, 0x8c(r31)
    cmplwi   r3, 0x0
    beq      .L_801DF844
    lhz      r0, 0x16(r3)
    rlwinm.  r0, r0, 0, 18, 18
    beq      .L_801DF844
    lwz      r4, 0x90(r31)
    mr       r3, r26
    bl       AttachParticleAppSRT
    b        .L_801DF850
.L_801DF844:
    lwz      r4, 0x90(r31)
    mr       r3, r26
    bl       psAttachParticleAppSRT
.L_801DF850:
    lfs      f0, 0x40(r31)
    mr       r3, r26
    mr       r4, r31
    stfs     f0, 0x40(r26)
    lfs      f0, 0x44(r31)
    stfs     f0, 0x44(r26)
    lfs      f0, 0x48(r31)
    stfs     f0, 0x48(r26)
    bl       psInterpretParticle0
    b        .L_801E1C80
.L_801DF878:
    lbz      r3, 0x0(r30)
    lbz      r0, 0x1(r30)
    addi     r30, r30, 0x2
    clrlslwi r5, r3, 24, 8
    lbz      r3, 0x1d(r31)
    lbz      r4, 0x8(r31)
    add      r5, r5, r0
    bl       psCreateGeneratorID
    mr.      r26, r3
    beq      .L_801E1C80
    lhz      r0, 0x1e(r31)
    sth      r0, 0x1c(r26)
    lwz      r4, 0x8c(r31)
    cmplwi   r4, 0x0
    beq      .L_801DF8BC
    lwz      r4, 0x10(r4)
    bl       AttachJointToParticleGenerator
.L_801DF8BC:
    lwz      r4, 0x90(r31)
    cmplwi   r4, 0x0
    beq      .L_801DF8F4
    lwz      r3, 0x8c(r31)
    cmplwi   r3, 0x0
    beq      .L_801DF8EC
    lhz      r0, 0x16(r3)
    rlwinm.  r0, r0, 0, 18, 18
    beq      .L_801DF8EC
    mr       r3, r26
    bl       AttachGeneratorAppSRT
    b        .L_801DF8F4
.L_801DF8EC:
    mr       r3, r26
    bl       psAttachGeneratorAppSRT
.L_801DF8F4:
    lwz      r0, 0x90(r31)
    cmplwi   r0, 0x0
    beq      .L_801DF950
    lwz      r0, 0x54(r26)
    cmplwi   r0, 0x0
    beq      .L_801DF9B0
    lfs      f0, 0x40(r31)
    stfs     f0, 0x24(r26)
    lfs      f0, 0x44(r31)
    stfs     f0, 0x28(r26)
    lfs      f0, 0x48(r31)
    stfs     f0, 0x2c(r26)
    lwz      r5, 0x54(r26)
    lwz      r4, 0x90(r31)
    cmplw    r5, r4
    beq      .L_801DF9B0
    lwz      r3, 0x8(r4)
    lwz      r0, 0xc(r4)
    stw      r3, 0x8(r5)
    stw      r0, 0xc(r5)
    lwz      r0, 0x10(r4)
    stw      r0, 0x10(r5)
    b        .L_801DF9B0
.L_801DF950:
    lwz      r3, 0x54(r26)
    cmplwi   r3, 0x0
    beq      .L_801DF998
    lfs      f0, 0x40(r31)
    stfs     f0, 0x8(r3)
    lfs      f0, 0x44(r31)
    lwz      r3, 0x54(r26)
    stfs     f0, 0xc(r3)
    lfs      f0, 0x48(r31)
    lwz      r3, 0x54(r26)
    stfs     f0, 0x10(r3)
    lfs      f0, 0x40(r31)
    stfs     f0, 0x24(r26)
    lfs      f0, 0x44(r31)
    stfs     f0, 0x28(r26)
    lfs      f0, 0x48(r31)
    stfs     f0, 0x2c(r26)
    b        .L_801DF9B0
.L_801DF998:
    lfs      f0, 0x40(r31)
    stfs     f0, 0x24(r26)
    lfs      f0, 0x44(r31)
    stfs     f0, 0x28(r26)
    lfs      f0, 0x48(r31)
    stfs     f0, 0x2c(r26)
.L_801DF9B0:
    lwz      r0, 0x54(r26)
    lwz      r3, 0x90(r31)
    cmplw    r0, r3
    beq      .L_801E1C80
    cmplwi   r3, 0x0
    bne      .L_801DFA18
    cmplwi   r0, 0x0
    beq      .L_801E1C80
    mr       r3, r26
    bl       SyncParticleGeneratorFromJoint
    lwz      r3, 0x54(r26)
    lfs      f1, 0x24(r26)
    lfs      f0, 0x8(r3)
    fsubs    f0, f1, f0
    stfs     f0, 0x24(r26)
    lwz      r3, 0x54(r26)
    lfs      f1, 0x28(r26)
    lfs      f0, 0xc(r3)
    fsubs    f0, f1, f0
    stfs     f0, 0x28(r26)
    lwz      r3, 0x54(r26)
    lfs      f1, 0x2c(r26)
    lfs      f0, 0x10(r3)
    fsubs    f0, f1, f0
    stfs     f0, 0x2c(r26)
    b        .L_801E1C80
.L_801DFA18:
    lwz      r3, 0x4(r3)
    bl       SyncParticleGeneratorFromJoint
    lwz      r0, 0x54(r26)
    cmplwi   r0, 0x0
    beq      .L_801DFA98
    mr       r3, r26
    bl       SyncParticleGeneratorFromJoint
    lwz      r4, 0x90(r31)
    lwz      r3, 0x54(r26)
    lfs      f1, 0x8(r4)
    lfs      f0, 0x8(r3)
    lfs      f2, 0x24(r26)
    fsubs    f0, f1, f0
    fadds    f0, f2, f0
    stfs     f0, 0x24(r26)
    lwz      r4, 0x90(r31)
    lwz      r3, 0x54(r26)
    lfs      f1, 0xc(r4)
    lfs      f0, 0xc(r3)
    lfs      f2, 0x28(r26)
    fsubs    f0, f1, f0
    fadds    f0, f2, f0
    stfs     f0, 0x28(r26)
    lwz      r4, 0x90(r31)
    lwz      r3, 0x54(r26)
    lfs      f1, 0x10(r4)
    lfs      f0, 0x10(r3)
    lfs      f2, 0x2c(r26)
    fsubs    f0, f1, f0
    fadds    f0, f2, f0
    stfs     f0, 0x2c(r26)
    b        .L_801E1C80
.L_801DFA98:
    lwz      r3, 0x90(r31)
    lfs      f1, 0x24(r26)
    lfs      f0, 0x8(r3)
    fadds    f0, f1, f0
    stfs     f0, 0x24(r26)
    lwz      r3, 0x90(r31)
    lfs      f1, 0x28(r26)
    lfs      f0, 0xc(r3)
    fadds    f0, f1, f0
    stfs     f0, 0x28(r26)
    lwz      r3, 0x90(r31)
    lfs      f1, 0x2c(r26)
    lfs      f0, 0x10(r3)
    fadds    f0, f1, f0
    stfs     f0, 0x2c(r26)
    b        .L_801E1C80
.L_801DFAD8:
    lbz      r3, 0x0(r30)
    lbz      r0, 0x1(r30)
    clrlslwi r5, r3, 24, 8
    lbz      r27, 0x2(r30)
    lbz      r3, 0x1d(r31)
    add      r5, r5, r0
    lbz      r4, 0x8(r31)
    addi     r30, r30, 0x3
    bl       psCreateGeneratorID
    mr.      r26, r3
    beq      .L_801E1C80
    lhz      r0, 0x1e(r31)
    sth      r0, 0x1c(r26)
    lwz      r4, 0x8c(r31)
    cmplwi   r4, 0x0
    beq      .L_801DFB20
    lwz      r4, 0x10(r4)
    bl       AttachJointToParticleGenerator
.L_801DFB20:
    lwz      r4, 0x90(r31)
    cmplwi   r4, 0x0
    beq      .L_801DFB58
    lwz      r3, 0x8c(r31)
    cmplwi   r3, 0x0
    beq      .L_801DFB50
    lhz      r0, 0x16(r3)
    rlwinm.  r0, r0, 0, 18, 18
    beq      .L_801DFB50
    mr       r3, r26
    bl       AttachGeneratorAppSRT
    b        .L_801DFB58
.L_801DFB50:
    mr       r3, r26
    bl       psAttachGeneratorAppSRT
.L_801DFB58:
    lwz      r3, 0x4(r26)
    clrlslwi r0, r27, 29, 25
    rlwinm   r3, r3, 0, 7, 3
    stw      r3, 0x4(r26)
    lwz      r3, 0x4(r26)
    or       r0, r3, r0
    stw      r0, 0x4(r26)
    lwz      r0, 0x90(r31)
    cmplwi   r0, 0x0
    beq      .L_801DFBD0
    lwz      r0, 0x54(r26)
    cmplwi   r0, 0x0
    beq      .L_801DFC30
    lfs      f0, 0x40(r31)
    stfs     f0, 0x24(r26)
    lfs      f0, 0x44(r31)
    stfs     f0, 0x28(r26)
    lfs      f0, 0x48(r31)
    stfs     f0, 0x2c(r26)
    lwz      r5, 0x54(r26)
    lwz      r4, 0x90(r31)
    cmplw    r5, r4
    beq      .L_801DFC30
    lwz      r3, 0x8(r4)
    lwz      r0, 0xc(r4)
    stw      r3, 0x8(r5)
    stw      r0, 0xc(r5)
    lwz      r0, 0x10(r4)
    stw      r0, 0x10(r5)
    b        .L_801DFC30
.L_801DFBD0:
    lwz      r3, 0x54(r26)
    cmplwi   r3, 0x0
    beq      .L_801DFC18
    lfs      f0, 0x40(r31)
    stfs     f0, 0x8(r3)
    lfs      f0, 0x44(r31)
    lwz      r3, 0x54(r26)
    stfs     f0, 0xc(r3)
    lfs      f0, 0x48(r31)
    lwz      r3, 0x54(r26)
    stfs     f0, 0x10(r3)
    lfs      f0, 0x40(r31)
    stfs     f0, 0x24(r26)
    lfs      f0, 0x44(r31)
    stfs     f0, 0x28(r26)
    lfs      f0, 0x48(r31)
    stfs     f0, 0x2c(r26)
    b        .L_801DFC30
.L_801DFC18:
    lfs      f0, 0x40(r31)
    stfs     f0, 0x24(r26)
    lfs      f0, 0x44(r31)
    stfs     f0, 0x28(r26)
    lfs      f0, 0x48(r31)
    stfs     f0, 0x2c(r26)
.L_801DFC30:
    lwz      r0, 0x54(r26)
    lwz      r3, 0x90(r31)
    cmplw    r0, r3
    beq      .L_801E1C80
    cmplwi   r3, 0x0
    bne      .L_801DFC98
    cmplwi   r0, 0x0
    beq      .L_801E1C80
    mr       r3, r26
    bl       SyncParticleGeneratorFromJoint
    lwz      r3, 0x54(r26)
    lfs      f1, 0x24(r26)
    lfs      f0, 0x8(r3)
    fsubs    f0, f1, f0
    stfs     f0, 0x24(r26)
    lwz      r3, 0x54(r26)
    lfs      f1, 0x28(r26)
    lfs      f0, 0xc(r3)
    fsubs    f0, f1, f0
    stfs     f0, 0x28(r26)
    lwz      r3, 0x54(r26)
    lfs      f1, 0x2c(r26)
    lfs      f0, 0x10(r3)
    fsubs    f0, f1, f0
    stfs     f0, 0x2c(r26)
    b        .L_801E1C80
.L_801DFC98:
    lwz      r3, 0x4(r3)
    bl       SyncParticleGeneratorFromJoint
    lwz      r0, 0x54(r26)
    cmplwi   r0, 0x0
    beq      .L_801DFD18
    mr       r3, r26
    bl       SyncParticleGeneratorFromJoint
    lwz      r4, 0x90(r31)
    lwz      r3, 0x54(r26)
    lfs      f1, 0x8(r4)
    lfs      f0, 0x8(r3)
    lfs      f2, 0x24(r26)
    fsubs    f0, f1, f0
    fadds    f0, f2, f0
    stfs     f0, 0x24(r26)
    lwz      r4, 0x90(r31)
    lwz      r3, 0x54(r26)
    lfs      f1, 0xc(r4)
    lfs      f0, 0xc(r3)
    lfs      f2, 0x28(r26)
    fsubs    f0, f1, f0
    fadds    f0, f2, f0
    stfs     f0, 0x28(r26)
    lwz      r4, 0x90(r31)
    lwz      r3, 0x54(r26)
    lfs      f1, 0x10(r4)
    lfs      f0, 0x10(r3)
    lfs      f2, 0x2c(r26)
    fsubs    f0, f1, f0
    fadds    f0, f2, f0
    stfs     f0, 0x2c(r26)
    b        .L_801E1C80
.L_801DFD18:
    lwz      r3, 0x90(r31)
    lfs      f1, 0x24(r26)
    lfs      f0, 0x8(r3)
    fadds    f0, f1, f0
    stfs     f0, 0x24(r26)
    lwz      r3, 0x90(r31)
    lfs      f1, 0x28(r26)
    lfs      f0, 0xc(r3)
    fadds    f0, f1, f0
    stfs     f0, 0x28(r26)
    lwz      r3, 0x90(r31)
    lfs      f1, 0x2c(r26)
    lfs      f0, 0x10(r3)
    fadds    f0, f1, f0
    stfs     f0, 0x2c(r26)
    b        .L_801E1C80
.L_801DFD58:
    lbz      r4, 0x8(r31)
    lis      r3, ptclref@ha
    lbz      r5, 0x0(r30)
    addi     r3, r3, ptclref@l
    clrlslwi r0, r4, 24, 2
    lbz      r6, 0x1(r30)
    lwzx     r3, r3, r0
    clrlslwi r5, r5, 24, 8
    lbz      r27, 0x2(r30)
    add      r5, r5, r6
    cmplwi   r3, 0x0
    addi     r30, r30, 0x3
    beq      .L_801DFD94
    slwi     r0, r5, 2
    lwzx     r5, r3, r0
.L_801DFD94:
    lbz      r3, 0x1d(r31)
    bl       psCreateGeneratorID
    mr.      r26, r3
    beq      .L_801E1C80
    lhz      r0, 0x1e(r31)
    sth      r0, 0x1c(r26)
    lwz      r4, 0x8c(r31)
    cmplwi   r4, 0x0
    beq      .L_801DFDC0
    lwz      r4, 0x10(r4)
    bl       AttachJointToParticleGenerator
.L_801DFDC0:
    lwz      r4, 0x90(r31)
    cmplwi   r4, 0x0
    beq      .L_801DFDF8
    lwz      r3, 0x8c(r31)
    cmplwi   r3, 0x0
    beq      .L_801DFDF0
    lhz      r0, 0x16(r3)
    rlwinm.  r0, r0, 0, 18, 18
    beq      .L_801DFDF0
    mr       r3, r26
    bl       AttachGeneratorAppSRT
    b        .L_801DFDF8
.L_801DFDF0:
    mr       r3, r26
    bl       psAttachGeneratorAppSRT
.L_801DFDF8:
    lwz      r3, 0x4(r26)
    clrlslwi r0, r27, 29, 25
    rlwinm   r3, r3, 0, 7, 3
    stw      r3, 0x4(r26)
    lwz      r3, 0x4(r26)
    or       r0, r3, r0
    stw      r0, 0x4(r26)
    lwz      r0, 0x90(r31)
    cmplwi   r0, 0x0
    beq      .L_801DFE70
    lwz      r0, 0x54(r26)
    cmplwi   r0, 0x0
    beq      .L_801DFED0
    lfs      f0, 0x40(r31)
    stfs     f0, 0x24(r26)
    lfs      f0, 0x44(r31)
    stfs     f0, 0x28(r26)
    lfs      f0, 0x48(r31)
    stfs     f0, 0x2c(r26)
    lwz      r5, 0x54(r26)
    lwz      r4, 0x90(r31)
    cmplw    r5, r4
    beq      .L_801DFED0
    lwz      r3, 0x8(r4)
    lwz      r0, 0xc(r4)
    stw      r3, 0x8(r5)
    stw      r0, 0xc(r5)
    lwz      r0, 0x10(r4)
    stw      r0, 0x10(r5)
    b        .L_801DFED0
.L_801DFE70:
    lwz      r3, 0x54(r26)
    cmplwi   r3, 0x0
    beq      .L_801DFEB8
    lfs      f0, 0x40(r31)
    stfs     f0, 0x8(r3)
    lfs      f0, 0x44(r31)
    lwz      r3, 0x54(r26)
    stfs     f0, 0xc(r3)
    lfs      f0, 0x48(r31)
    lwz      r3, 0x54(r26)
    stfs     f0, 0x10(r3)
    lfs      f0, 0x40(r31)
    stfs     f0, 0x24(r26)
    lfs      f0, 0x44(r31)
    stfs     f0, 0x28(r26)
    lfs      f0, 0x48(r31)
    stfs     f0, 0x2c(r26)
    b        .L_801DFED0
.L_801DFEB8:
    lfs      f0, 0x40(r31)
    stfs     f0, 0x24(r26)
    lfs      f0, 0x44(r31)
    stfs     f0, 0x28(r26)
    lfs      f0, 0x48(r31)
    stfs     f0, 0x2c(r26)
.L_801DFED0:
    lwz      r0, 0x54(r26)
    lwz      r3, 0x90(r31)
    cmplw    r0, r3
    beq      .L_801E1C80
    cmplwi   r3, 0x0
    bne      .L_801DFF38
    cmplwi   r0, 0x0
    beq      .L_801E1C80
    mr       r3, r26
    bl       SyncParticleGeneratorFromJoint
    lwz      r3, 0x54(r26)
    lfs      f1, 0x24(r26)
    lfs      f0, 0x8(r3)
    fsubs    f0, f1, f0
    stfs     f0, 0x24(r26)
    lwz      r3, 0x54(r26)
    lfs      f1, 0x28(r26)
    lfs      f0, 0xc(r3)
    fsubs    f0, f1, f0
    stfs     f0, 0x28(r26)
    lwz      r3, 0x54(r26)
    lfs      f1, 0x2c(r26)
    lfs      f0, 0x10(r3)
    fsubs    f0, f1, f0
    stfs     f0, 0x2c(r26)
    b        .L_801E1C80
.L_801DFF38:
    lwz      r3, 0x4(r3)
    bl       SyncParticleGeneratorFromJoint
    lwz      r0, 0x54(r26)
    cmplwi   r0, 0x0
    beq      .L_801DFFB8
    mr       r3, r26
    bl       SyncParticleGeneratorFromJoint
    lwz      r4, 0x90(r31)
    lwz      r3, 0x54(r26)
    lfs      f1, 0x8(r4)
    lfs      f0, 0x8(r3)
    lfs      f2, 0x24(r26)
    fsubs    f0, f1, f0
    fadds    f0, f2, f0
    stfs     f0, 0x24(r26)
    lwz      r4, 0x90(r31)
    lwz      r3, 0x54(r26)
    lfs      f1, 0xc(r4)
    lfs      f0, 0xc(r3)
    lfs      f2, 0x28(r26)
    fsubs    f0, f1, f0
    fadds    f0, f2, f0
    stfs     f0, 0x28(r26)
    lwz      r4, 0x90(r31)
    lwz      r3, 0x54(r26)
    lfs      f1, 0x10(r4)
    lfs      f0, 0x10(r3)
    lfs      f2, 0x2c(r26)
    fsubs    f0, f1, f0
    fadds    f0, f2, f0
    stfs     f0, 0x2c(r26)
    b        .L_801E1C80
.L_801DFFB8:
    lwz      r3, 0x90(r31)
    lfs      f1, 0x24(r26)
    lfs      f0, 0x8(r3)
    fadds    f0, f1, f0
    stfs     f0, 0x24(r26)
    lwz      r3, 0x90(r31)
    lfs      f1, 0x28(r26)
    lfs      f0, 0xc(r3)
    fadds    f0, f1, f0
    stfs     f0, 0x28(r26)
    lwz      r3, 0x90(r31)
    lfs      f1, 0x2c(r26)
    lfs      f0, 0x10(r3)
    fadds    f0, f1, f0
    stfs     f0, 0x2c(r26)
    b        .L_801E1C80
.L_801DFFF8:
    lbz      r0, 0x0(r30)
    lbz      r3, 0x2(r30)
    clrlslwi r26, r0, 24, 8
    lbz      r4, 0x1(r30)
    lbz      r0, 0x3(r30)
    clrlslwi r27, r3, 24, 8
    add      r26, r26, r4
    addi     r30, r30, 0x4
    add      r27, r27, r0
    bl       HSD_Randf
    xoris    r3, r27, 0x8000
    lis      r0, 0x4330
    stw      r3, 0x1c(r1)
    lfd      f2, "@1011"@sda21(r0)
    stw      r0, 0x18(r1)
    lfd      f0, 0x18(r1)
    fsubs    f0, f0, f2
    fmuls    f0, f0, f1
    fctiwz   f0, f0
    stfd     f0, 0x20(r1)
    lwz      r0, 0x24(r1)
    add      r0, r26, r0
    sth      r0, 0x2a(r31)
    b        .L_801E1C80
.L_801E0058:
    lbz      r26, 0x0(r30)
    addi     r30, r30, 0x1
    bl       HSD_Randf
    lfs      f0, "@1003"@sda21(r0)
    fmuls    f0, f0, f1
    fctiwz   f0, f0
    stfd     f0, 0x20(r1)
    lwz      r0, 0x24(r1)
    cmpw     r26, r0
    blt      .L_801E1C80
    li       r0, 0x1
    sth      r0, 0x2a(r31)
    b        .L_801E1C88
.L_801E008C:
    mr       r3, r30
    addi     r4, r1, 0x14
    bl       getFloat
    lfs      f28, 0x14(r1)
    mr       r26, r3
    bl       HSD_Randf
    lfs      f2, "@1004"@sda21(r0)
    mr       r3, r26
    lfs      f0, 0x40(r31)
    addi     r4, r1, 0x14
    fmuls    f2, f2, f28
    fmsubs   f1, f2, f1, f28
    fadds    f0, f0, f1
    stfs     f0, 0x40(r31)
    bl       getFloat
    lfs      f28, 0x14(r1)
    mr       r26, r3
    bl       HSD_Randf
    lfs      f2, "@1004"@sda21(r0)
    mr       r3, r26
    lfs      f0, 0x44(r31)
    addi     r4, r1, 0x14
    fmuls    f2, f2, f28
    fmsubs   f1, f2, f1, f28
    fadds    f0, f0, f1
    stfs     f0, 0x44(r31)
    bl       getFloat
    lfs      f28, 0x14(r1)
    mr       r30, r3
    bl       HSD_Randf
    lfs      f2, "@1004"@sda21(r0)
    lfs      f0, 0x48(r31)
    fmuls    f2, f2, f28
    fmsubs   f1, f2, f1, f28
    fadds    f0, f0, f1
    stfs     f0, 0x48(r31)
    b        .L_801E1C80
.L_801E0120:
    mr       r3, r30
    addi     r4, r1, 0x14
    bl       getFloat
    lfs      f1, 0x14(r1)
    mr       r30, r3
    mr       r3, r31
    bl       modifyDir
    b        .L_801E1C80
.L_801E0140:
    mr       r3, r30
    addi     r4, r1, 0x10
    bl       getFloat
    addi     r4, r1, 0xc
    bl       getFloat
    addi     r4, r1, 0x8
    bl       getFloat
    addi     r4, r1, 0x14
    bl       getFloat
    lwz      r0, 0x8c(r31)
    mr       r30, r3
    cmplwi   r0, 0x0
    beq      .L_801E1C80
    lfs      f1, 0x14(r1)
    mr       r3, r31
    lfs      f2, 0x10(r1)
    lfs      f3, 0xc(r1)
    lfs      f4, 0x8(r1)
    bl       modifyDirGenBase
    b        .L_801E1C80
.L_801E0190:
    lwz      r5, 0x8c(r31)
    cmplwi   r5, 0x0
    beq      .L_801E1C80
    lwz      r0, 0x54(r5)
    cmplwi   r0, 0x0
    beq      .L_801E1C80
    lhz      r4, 0x16(r5)
    li       r0, 0x0
    ori      r3, r4, 0x2000
    rlwinm.  r4, r4, 0, 18, 18
    sth      r3, 0x16(r5)
    lwz      r3, 0x8c(r31)
    lwz      r3, 0x54(r3)
    stb      r0, 0x72(r3)
    bne      .L_801E1C80
    lwz      r3, 0x8c(r31)
    bl       SyncParticleGeneratorFromJoint
    b        .L_801E1C80
.L_801E01D8:
    lwz      r3, 0x8c(r31)
    cmplwi   r3, 0x0
    beq      .L_801E1C80
    lwz      r0, 0x54(r3)
    cmplwi   r0, 0x0
    beq      .L_801E1C80
    lhz      r0, 0x16(r3)
    rlwinm.  r4, r0, 0, 19, 19
    ori      r0, r0, 0x1000
    sth      r0, 0x16(r3)
    bne      .L_801E1C80
    lwz      r3, 0x8c(r31)
    bl       SyncParticleGeneratorFromJoint
    b        .L_801E1C80
.L_801E0210:
    lwz      r0, 0x4(r31)
    oris     r0, r0, 0x1000
    stw      r0, 0x4(r31)
    b        .L_801E1C80
.L_801E0220:
    lbz      r0, 0x0(r30)
    lbz      r3, 0x2(r30)
    clrlslwi r26, r0, 24, 8
    lbz      r4, 0x1(r30)
    lbz      r0, 0x3(r30)
    clrlslwi r27, r3, 24, 8
    add      r26, r26, r4
    addi     r30, r30, 0x4
    add      r27, r27, r0
    bl       HSD_Randf
    xoris    r3, r27, 0x8000
    lis      r0, 0x4330
    stw      r3, 0x24(r1)
    lis      r3, ptclref@ha
    lbz      r5, 0x8(r31)
    addi     r3, r3, ptclref@l
    stw      r0, 0x20(r1)
    lfd      f2, "@1011"@sda21(r0)
    clrlslwi r0, r5, 24, 2
    lfd      f0, 0x20(r1)
    lwzx     r3, r3, r0
    fsubs    f0, f0, f2
    cmplwi   r3, 0x0
    fmuls    f0, f0, f1
    fctiwz   f0, f0
    stfd     f0, 0x18(r1)
    lwz      r0, 0x1c(r1)
    add      r26, r26, r0
    beq      .L_801E029C
    slwi     r0, r26, 2
    lwzx     r26, r3, r0
.L_801E029C:
    lbz      r4, 0x1d(r31)
    mr       r3, r31
    mr       r6, r26
    li       r7, 0x0
    bl       psGenerateParticleID0
    mr.      r26, r3
    beq      .L_801E1C80
    lfs      f0, 0x40(r31)
    stfs     f0, 0x40(r26)
    lfs      f0, 0x44(r31)
    stfs     f0, 0x44(r26)
    lfs      f0, 0x48(r31)
    stfs     f0, 0x48(r26)
    lhz      r0, 0x1e(r31)
    sth      r0, 0x1e(r26)
    lwz      r0, 0x8c(r31)
    stw      r0, 0x8c(r26)
    lwz      r4, 0x8c(r31)
    cmplwi   r4, 0x0
    beq      .L_801E02F8
    lwz      r3, 0x50(r4)
    addi     r0, r3, 0x1
    stw      r0, 0x50(r4)
.L_801E02F8:
    lwz      r4, 0x90(r31)
    cmplwi   r4, 0x0
    beq      .L_801E0330
    lwz      r3, 0x8c(r31)
    cmplwi   r3, 0x0
    beq      .L_801E0328
    lhz      r0, 0x16(r3)
    rlwinm.  r0, r0, 0, 18, 18
    beq      .L_801E0328
    mr       r3, r26
    bl       AttachParticleAppSRT
    b        .L_801E0330
.L_801E0328:
    mr       r3, r26
    bl       psAttachParticleAppSRT
.L_801E0330:
    mr       r3, r26
    mr       r4, r31
    bl       psInterpretParticle0
    b        .L_801E1C80
.L_801E0340:
    mr       r3, r30
    addi     r4, r1, 0x14
    bl       getFloat
    lfs      f1, 0x2c(r31)
    mr       r30, r3
    lfs      f0, 0x14(r1)
    fmuls    f0, f1, f0
    stfs     f0, 0x2c(r31)
    lfs      f1, 0x30(r31)
    lfs      f0, 0x14(r1)
    fmuls    f0, f1, f0
    stfs     f0, 0x30(r31)
    lfs      f1, 0x34(r31)
    lfs      f0, 0x14(r1)
    fmuls    f0, f1, f0
    stfs     f0, 0x34(r31)
    b        .L_801E1C80
.L_801E0384:
    mr       r3, r30
    addi     r4, r31, 0xc
    bl       getTime
    addi     r4, r31, 0x60
    bl       getFloat
    addi     r4, r1, 0x14
    bl       getFloat
    mr       r30, r3
    bl       HSD_Randf
    lfs      f2, 0x14(r1)
    lfs      f0, 0x60(r31)
    fmadds   f0, f2, f1, f0
    stfs     f0, 0x60(r31)
    lhz      r0, 0xc(r31)
    cmplwi   r0, 0x0
    bne      .L_801E1C80
    lfs      f0, 0x60(r31)
    stfs     f0, 0x4c(r31)
    b        .L_801E1C80
.L_801E03D0:
    lwz      r0, 0x4(r31)
    ori      r0, r0, 0x80
    stw      r0, 0x4(r31)
    b        .L_801E1C80
.L_801E03E0:
    lwz      r0, 0x4(r31)
    rlwinm   r0, r0, 0, 27, 24
    stw      r0, 0x4(r31)
    b        .L_801E1C80
.L_801E03F0:
    lwz      r0, 0x4(r31)
    rlwinm   r0, r0, 0, 26, 24
    stw      r0, 0x4(r31)
    lwz      r0, 0x4(r31)
    ori      r0, r0, 0x20
    stw      r0, 0x4(r31)
    b        .L_801E1C80
.L_801E040C:
    lwz      r0, 0x4(r31)
    rlwinm   r0, r0, 0, 27, 25
    stw      r0, 0x4(r31)
    lwz      r0, 0x4(r31)
    ori      r0, r0, 0x40
    stw      r0, 0x4(r31)
    b        .L_801E1C80
.L_801E0428:
    lwz      r0, 0x4(r31)
    ori      r0, r0, 0x60
    stw      r0, 0x4(r31)
    b        .L_801E1C80
.L_801E0438:
    lwz      r3, 0x90(r31)
    cmplwi   r3, 0x0
    beq      .L_801E1C80
    lbz      r0, 0x72(r3)
    cmplwi   r0, 0x0
    bne      .L_801E1C80
    lwz      r3, 0x4(r3)
    bl       SyncParticleGeneratorFromJoint
    lwz      r6, 0x90(r31)
    li       r7, 0x0
    addi     r3, r6, 0x34
    addi     r4, r6, 0x24
    addi     r5, r6, 0x14
    addi     r6, r6, 0x8
    bl       HSD_MTXSRT
    lwz      r4, 0x90(r31)
    mr       r3, r31
    lfs      f3, 0x44(r31)
    lfs      f0, 0x38(r4)
    lfs      f1, 0x48(r4)
    fmuls    f0, f0, f3
    lfs      f2, 0x58(r4)
    lfs      f6, 0x40(r31)
    fmuls    f4, f1, f3
    lfs      f1, 0x34(r4)
    fmuls    f2, f2, f3
    lfs      f5, 0x44(r4)
    fmadds   f0, f1, f6, f0
    lfs      f3, 0x54(r4)
    lfs      f7, 0x48(r31)
    fmadds   f4, f5, f6, f4
    lfs      f1, 0x3c(r4)
    fmadds   f2, f3, f6, f2
    lfs      f5, 0x4c(r4)
    lfs      f3, 0x5c(r4)
    fmadds   f0, f1, f7, f0
    lfs      f1, 0x40(r4)
    fmadds   f4, f5, f7, f4
    lfs      f5, 0x50(r4)
    fmadds   f2, f3, f7, f2
    lfs      f3, 0x60(r4)
    fadds    f0, f1, f0
    fadds    f1, f5, f4
    fadds    f2, f3, f2
    stfs     f0, 0x40(r31)
    stfs     f1, 0x44(r31)
    stfs     f2, 0x48(r31)
    bl       psRemoveParticleAppSRT
    b        .L_801E1C80
.L_801E04FC:
    lhz      r4, 0x54(r31)
    cmplwi   r4, 0x0
    beq      .L_801E0554
    lhz      r0, 0x7c(r31)
    lbz      r5, 0x7e(r31)
    slwi     r3, r0, 16
    lbz      r0, 0x57(r31)
    divw     r6, r3, r4
    subf     r0, r5, r0
    slwi     r3, r5, 16
    mullw    r0, r6, r0
    add      r0, r3, r0
    srawi    r0, r0, 16
    stb      r0, 0x57(r31)
    lbz      r4, 0x7f(r31)
    lbz      r0, 0x58(r31)
    slwi     r3, r4, 16
    subf     r0, r4, r0
    mullw    r0, r6, r0
    add      r0, r3, r0
    srawi    r0, r0, 16
    stb      r0, 0x58(r31)
.L_801E0554:
    mr       r3, r30
    addi     r4, r31, 0x54
    bl       getTime
    lbz      r0, 0x0(r3)
    addi     r30, r3, 0x3
    stb      r0, 0x56(r31)
    lbz      r0, 0x1(r3)
    stb      r0, 0x7e(r31)
    lbz      r0, 0x2(r3)
    stb      r0, 0x7f(r31)
    lhz      r0, 0x54(r31)
    cmplwi   r0, 0x0
    bne      .L_801E05A8
    lbz      r3, 0x7e(r31)
    li       r0, 0x0
    stb      r3, 0x57(r31)
    lbz      r3, 0x7f(r31)
    stb      r3, 0x58(r31)
    sth      r0, 0x7c(r31)
    sth      r0, 0x54(r31)
    b        .L_801E1C80
.L_801E05A8:
    sth      r0, 0x7c(r31)
    b        .L_801E1C80
.L_801E05B0:
    lwz      r0, 0x4(r31)
    ori      r0, r0, 0x200
    stw      r0, 0x4(r31)
    b        .L_801E1C80
.L_801E05C0:
    lwz      r0, 0x4(r31)
    rlwinm   r0, r0, 0, 23, 21
    stw      r0, 0x4(r31)
    b        .L_801E1C80
.L_801E05D0:
    mr       r3, r30
    addi     r4, r31, 0x5e
    bl       getTime
    addi     r4, r1, 0x14
    bl       getFloat
    lfs      f1, 0x64(r31)
    mr       r30, r3
    lfs      f0, 0x14(r1)
    fadds    f0, f1, f0
    stfs     f0, 0x64(r31)
    lhz      r0, 0x5e(r31)
    cmplwi   r0, 0x0
    bne      .L_801E1C80
    lfs      f0, 0x64(r31)
    stfs     f0, 0x50(r31)
    b        .L_801E1C80
.L_801E0610:
    lbz      r5, 0x0(r30)
    lis      r3, psPointJObj@ha
    lbz      r0, 0x59(r31)
    addi     r4, r3, psPointJObj@l
    mr       r3, r31
    addi     r30, r30, 0x1
    add      r0, r5, r0
    slwi     r0, r0, 2
    lwzx     r4, r4, r0
    bl       setVelToJObj
    b        .L_801E1C80
.L_801E063C:
    lbz      r5, 0x0(r30)
    addi     r30, r30, 0x1
    lbz      r0, 0x59(r31)
    mr       r3, r30
    addi     r4, r1, 0x14
    add      r26, r5, r0
    bl       getFloat
    addi     r4, r1, 0x10
    bl       getFloat
    lis      r4, psPointJObj@ha
    mr       r30, r3
    slwi     r0, r26, 2
    lfs      f1, 0x14(r1)
    addi     r4, r4, psPointJObj@l
    lfs      f2, 0x10(r1)
    lwzx     r4, r4, r0
    mr       r3, r31
    bl       applyForceJObj
    cmpwi    r3, 0x0
    beq      .L_801E1C80
    li       r0, 0x1
    sth      r0, 0x2a(r31)
    b        .L_801E1C88
.L_801E0698:
    lbz      r4, 0x0(r30)
    mr       r3, r31
    lbz      r0, 0x1(r30)
    li       r7, 0x0
    clrlslwi r6, r4, 24, 8
    lbz      r4, 0x1d(r31)
    lbz      r5, 0x8(r31)
    add      r6, r6, r0
    addi     r30, r30, 0x2
    bl       psGenerateParticleID0
    mr.      r26, r3
    beq      .L_801E1C80
    lfs      f0, 0x40(r31)
    stfs     f0, 0x40(r26)
    lfs      f0, 0x44(r31)
    stfs     f0, 0x44(r26)
    lfs      f0, 0x48(r31)
    stfs     f0, 0x48(r26)
    lfs      f0, 0x2c(r31)
    stfs     f0, 0x2c(r26)
    lfs      f0, 0x30(r31)
    stfs     f0, 0x30(r26)
    lfs      f0, 0x34(r31)
    stfs     f0, 0x34(r26)
    lhz      r0, 0x1e(r31)
    sth      r0, 0x1e(r26)
    lwz      r0, 0x8c(r31)
    stw      r0, 0x8c(r26)
    lwz      r4, 0x8c(r31)
    cmplwi   r4, 0x0
    beq      .L_801E0720
    lwz      r3, 0x50(r4)
    addi     r0, r3, 0x1
    stw      r0, 0x50(r4)
.L_801E0720:
    lwz      r4, 0x90(r31)
    cmplwi   r4, 0x0
    beq      .L_801E0758
    lwz      r3, 0x8c(r31)
    cmplwi   r3, 0x0
    beq      .L_801E0750
    lhz      r0, 0x16(r3)
    rlwinm.  r0, r0, 0, 18, 18
    beq      .L_801E0750
    mr       r3, r26
    bl       AttachParticleAppSRT
    b        .L_801E0758
.L_801E0750:
    mr       r3, r26
    bl       psAttachParticleAppSRT
.L_801E0758:
    mr       r3, r26
    mr       r4, r31
    bl       psInterpretParticle0
    b        .L_801E1C80
.L_801E0768:
    lbz      r5, 0x8(r31)
    lis      r3, ptclref@ha
    lbz      r6, 0x0(r30)
    addi     r3, r3, ptclref@l
    clrlslwi r0, r5, 24, 2
    lbz      r4, 0x1(r30)
    lwzx     r3, r3, r0
    clrlslwi r6, r6, 24, 8
    add      r6, r6, r4
    addi     r30, r30, 0x2
    cmplwi   r3, 0x0
    beq      .L_801E07A0
    slwi     r0, r6, 2
    lwzx     r6, r3, r0
.L_801E07A0:
    lbz      r4, 0x1d(r31)
    mr       r3, r31
    li       r7, 0x0
    bl       psGenerateParticleID0
    mr.      r26, r3
    beq      .L_801E1C80
    lfs      f0, 0x40(r31)
    stfs     f0, 0x40(r26)
    lfs      f0, 0x44(r31)
    stfs     f0, 0x44(r26)
    lfs      f0, 0x48(r31)
    stfs     f0, 0x48(r26)
    lfs      f0, 0x2c(r31)
    stfs     f0, 0x2c(r26)
    lfs      f0, 0x30(r31)
    stfs     f0, 0x30(r26)
    lfs      f0, 0x34(r31)
    stfs     f0, 0x34(r26)
    lhz      r0, 0x1e(r31)
    sth      r0, 0x1e(r26)
    lwz      r0, 0x8c(r31)
    stw      r0, 0x8c(r26)
    lwz      r4, 0x8c(r31)
    cmplwi   r4, 0x0
    beq      .L_801E0810
    lwz      r3, 0x50(r4)
    addi     r0, r3, 0x1
    stw      r0, 0x50(r4)
.L_801E0810:
    lwz      r4, 0x90(r31)
    cmplwi   r4, 0x0
    beq      .L_801E0848
    lwz      r3, 0x8c(r31)
    cmplwi   r3, 0x0
    beq      .L_801E0840
    lhz      r0, 0x16(r3)
    rlwinm.  r0, r0, 0, 18, 18
    beq      .L_801E0840
    mr       r3, r26
    bl       AttachParticleAppSRT
    b        .L_801E0848
.L_801E0840:
    mr       r3, r26
    bl       psAttachParticleAppSRT
.L_801E0848:
    mr       r3, r26
    mr       r4, r31
    bl       psInterpretParticle0
    b        .L_801E1C80
.L_801E0858:
    lhz      r4, 0xe(r31)
    cmplwi   r4, 0x0
    beq      .L_801E08F0
    lhz      r0, 0x6c(r31)
    lbz      r5, 0x70(r31)
    slwi     r0, r0, 16
    lbz      r3, 0x12(r31)
    divw     r0, r0, r4
    slwi     r4, r5, 16
    subf     r3, r5, r3
    mullw    r3, r0, r3
    add      r3, r4, r3
    srawi    r3, r3, 16
    stb      r3, 0x12(r31)
    lbz      r5, 0x71(r31)
    lbz      r3, 0x13(r31)
    slwi     r4, r5, 16
    subf     r3, r5, r3
    mullw    r3, r0, r3
    add      r3, r4, r3
    srawi    r3, r3, 16
    stb      r3, 0x13(r31)
    lbz      r5, 0x72(r31)
    lbz      r3, 0x14(r31)
    slwi     r4, r5, 16
    subf     r3, r5, r3
    mullw    r3, r0, r3
    add      r3, r4, r3
    srawi    r3, r3, 16
    stb      r3, 0x14(r31)
    lbz      r5, 0x73(r31)
    lbz      r3, 0x15(r31)
    slwi     r4, r5, 16
    subf     r3, r5, r3
    mullw    r0, r0, r3
    add      r0, r4, r0
    srawi    r0, r0, 16
    stb      r0, 0x15(r31)
.L_801E08F0:
    bl       HSD_Randf
    lbz      r3, 0x0(r30)
    lis      r0, 0x4330
    stw      r0, 0x20(r1)
    extsb    r0, r3
    lfd      f2, "@1011"@sda21(r0)
    slwi     r0, r0, 1
    xoris    r0, r0, 0x8000
    stw      r0, 0x24(r1)
    lfd      f0, 0x20(r1)
    fsubs    f0, f0, f2
    fmuls    f1, f0, f1
    stfs     f1, 0x14(r1)
    lbz      r3, 0x70(r31)
    bl       U8ClampAdd
    stb      r3, 0x70(r31)
    bl       HSD_Randf
    lbz      r3, 0x1(r30)
    lis      r0, 0x4330
    stw      r0, 0x18(r1)
    extsb    r0, r3
    lfd      f2, "@1011"@sda21(r0)
    slwi     r0, r0, 1
    xoris    r0, r0, 0x8000
    stw      r0, 0x1c(r1)
    lfd      f0, 0x18(r1)
    fsubs    f0, f0, f2
    fmuls    f1, f0, f1
    stfs     f1, 0x14(r1)
    lbz      r3, 0x71(r31)
    bl       U8ClampAdd
    stb      r3, 0x71(r31)
    bl       HSD_Randf
    lbz      r3, 0x2(r30)
    lis      r0, 0x4330
    stw      r0, 0x28(r1)
    extsb    r0, r3
    lfd      f2, "@1011"@sda21(r0)
    slwi     r0, r0, 1
    xoris    r0, r0, 0x8000
    stw      r0, 0x2c(r1)
    lfd      f0, 0x28(r1)
    fsubs    f0, f0, f2
    fmuls    f1, f0, f1
    stfs     f1, 0x14(r1)
    lbz      r3, 0x72(r31)
    bl       U8ClampAdd
    stb      r3, 0x72(r31)
    bl       HSD_Randf
    lbz      r3, 0x3(r30)
    lis      r0, 0x4330
    stw      r0, 0x30(r1)
    addi     r30, r30, 0x4
    extsb    r0, r3
    lfd      f2, "@1011"@sda21(r0)
    slwi     r0, r0, 1
    xoris    r0, r0, 0x8000
    stw      r0, 0x34(r1)
    lfd      f0, 0x30(r1)
    fsubs    f0, f0, f2
    fmuls    f1, f0, f1
    stfs     f1, 0x14(r1)
    lbz      r3, 0x73(r31)
    bl       U8ClampAdd
    stb      r3, 0x73(r31)
    lhz      r0, 0xe(r31)
    cmplwi   r0, 0x0
    bne      .L_801E0A0C
    lwz      r0, 0x70(r31)
    stw      r0, 0x12(r31)
    b        .L_801E1C80
.L_801E0A0C:
    sth      r0, 0x6c(r31)
    b        .L_801E1C80
.L_801E0A14:
    lhz      r4, 0x10(r31)
    cmplwi   r4, 0x0
    beq      .L_801E0AAC
    lhz      r0, 0x6e(r31)
    lbz      r5, 0x74(r31)
    slwi     r0, r0, 16
    lbz      r3, 0x16(r31)
    divw     r0, r0, r4
    slwi     r4, r5, 16
    subf     r3, r5, r3
    mullw    r3, r0, r3
    add      r3, r4, r3
    srawi    r3, r3, 16
    stb      r3, 0x16(r31)
    lbz      r5, 0x75(r31)
    lbz      r3, 0x17(r31)
    slwi     r4, r5, 16
    subf     r3, r5, r3
    mullw    r3, r0, r3
    add      r3, r4, r3
    srawi    r3, r3, 16
    stb      r3, 0x17(r31)
    lbz      r5, 0x76(r31)
    lbz      r3, 0x18(r31)
    slwi     r4, r5, 16
    subf     r3, r5, r3
    mullw    r3, r0, r3
    add      r3, r4, r3
    srawi    r3, r3, 16
    stb      r3, 0x18(r31)
    lbz      r5, 0x77(r31)
    lbz      r3, 0x19(r31)
    slwi     r4, r5, 16
    subf     r3, r5, r3
    mullw    r0, r0, r3
    add      r0, r4, r0
    srawi    r0, r0, 16
    stb      r0, 0x19(r31)
.L_801E0AAC:
    bl       HSD_Randf
    lbz      r3, 0x0(r30)
    lis      r0, 0x4330
    stw      r0, 0x30(r1)
    extsb    r0, r3
    lfd      f2, "@1011"@sda21(r0)
    slwi     r0, r0, 1
    xoris    r0, r0, 0x8000
    stw      r0, 0x34(r1)
    lfd      f0, 0x30(r1)
    fsubs    f0, f0, f2
    fmuls    f1, f0, f1
    stfs     f1, 0x14(r1)
    lbz      r3, 0x74(r31)
    bl       U8ClampAdd
    stb      r3, 0x74(r31)
    bl       HSD_Randf
    lbz      r3, 0x1(r30)
    lis      r0, 0x4330
    stw      r0, 0x28(r1)
    extsb    r0, r3
    lfd      f2, "@1011"@sda21(r0)
    slwi     r0, r0, 1
    xoris    r0, r0, 0x8000
    stw      r0, 0x2c(r1)
    lfd      f0, 0x28(r1)
    fsubs    f0, f0, f2
    fmuls    f1, f0, f1
    stfs     f1, 0x14(r1)
    lbz      r3, 0x75(r31)
    bl       U8ClampAdd
    stb      r3, 0x75(r31)
    bl       HSD_Randf
    lbz      r3, 0x2(r30)
    lis      r0, 0x4330
    stw      r0, 0x20(r1)
    extsb    r0, r3
    lfd      f2, "@1011"@sda21(r0)
    slwi     r0, r0, 1
    xoris    r0, r0, 0x8000
    stw      r0, 0x24(r1)
    lfd      f0, 0x20(r1)
    fsubs    f0, f0, f2
    fmuls    f1, f0, f1
    stfs     f1, 0x14(r1)
    lbz      r3, 0x76(r31)
    bl       U8ClampAdd
    stb      r3, 0x76(r31)
    bl       HSD_Randf
    lbz      r3, 0x3(r30)
    lis      r0, 0x4330
    stw      r0, 0x18(r1)
    addi     r30, r30, 0x4
    extsb    r0, r3
    lfd      f2, "@1011"@sda21(r0)
    slwi     r0, r0, 1
    xoris    r0, r0, 0x8000
    stw      r0, 0x1c(r1)
    lfd      f0, 0x18(r1)
    fsubs    f0, f0, f2
    fmuls    f1, f0, f1
    stfs     f1, 0x14(r1)
    lbz      r3, 0x77(r31)
    bl       U8ClampAdd
    stb      r3, 0x77(r31)
    lhz      r0, 0x10(r31)
    cmplwi   r0, 0x0
    bne      .L_801E0BC8
    lwz      r0, 0x74(r31)
    stw      r0, 0x16(r31)
    b        .L_801E1C80
.L_801E0BC8:
    sth      r0, 0x6e(r31)
    b        .L_801E1C80
.L_801E0BD0:
    lbz      r3, 0x0(r30)
    lis      r0, 0x4330
    stw      r0, 0x30(r1)
    lfd      f1, "@1009"@sda21(r0)
    stb      r3, 0xa(r31)
    lbz      r0, 0x1(r30)
    addi     r30, r30, 0x2
    stw      r0, 0x34(r1)
    lfd      f0, 0x30(r1)
    fsubs    f0, f0, f1
    stfs     f0, 0x14(r1)
    bl       HSD_Randf
    lbz      r4, 0xa(r31)
    lis      r0, 0x4330
    lis      r3, psTexGroupArray@ha
    stw      r0, 0x28(r1)
    lfd      f2, "@1009"@sda21(r0)
    addi     r3, r3, psTexGroupArray@l
    stw      r4, 0x2c(r1)
    lfs      f3, 0x14(r1)
    lfd      f0, 0x28(r1)
    fsubs    f0, f0, f2
    fmadds   f0, f3, f1, f0
    fctiwz   f0, f0
    stfd     f0, 0x20(r1)
    lwz      r0, 0x24(r1)
    stb      r0, 0xa(r31)
    lbz      r4, 0x8(r31)
    lbz      r0, 0x9(r31)
    slwi     r4, r4, 2
    lwzx     r3, r3, r4
    slwi     r0, r0, 2
    lwzx     r4, r3, r0
    cmplwi   r4, 0x0
    beq      .L_801E1C80
    addic.   r0, r4, 0x18
    beq      .L_801E1C80
    lbz      r0, 0xa(r31)
    slwi     r3, r0, 2
    addi     r0, r3, 0x18
    lwzx     r0, r4, r0
    cmplwi   r0, 0x0
    beq      .L_801E1C80
    lwz      r0, 0x4(r31)
    ori      r0, r0, 0x400
    stw      r0, 0x4(r31)
    b        .L_801E1C80
.L_801E0C8C:
    mr       r3, r30
    addi     r4, r1, 0x14
    bl       getFloat
    addi     r4, r1, 0x10
    bl       getFloat
    mr       r30, r3
    bl       HSD_Randf
    lfs      f2, 0x10(r1)
    lfs      f0, 0x14(r1)
    fmadds   f0, f2, f1, f0
    stfs     f0, 0x14(r1)
    lfs      f1, 0x2c(r31)
    lfs      f0, 0x30(r31)
    lfs      f2, 0x34(r31)
    fmuls    f1, f1, f1
    fmuls    f0, f0, f0
    fmuls    f2, f2, f2
    fadds    f0, f1, f0
    fadds    f1, f2, f0
    bl       sqrtf
    frsp     f2, f1
    lfs      f0, "@1005"@sda21(r0)
    stfs     f1, 0x10(r1)
    fcmpo    cr0, f2, f0
    ble      .L_801E1C80
    lfs      f0, 0x14(r1)
    fdivs    f1, f0, f2
    stfs     f1, 0x14(r1)
    lfs      f0, 0x2c(r31)
    fmuls    f0, f0, f1
    stfs     f0, 0x2c(r31)
    lfs      f1, 0x30(r31)
    lfs      f0, 0x14(r1)
    fmuls    f0, f1, f0
    stfs     f0, 0x30(r31)
    lfs      f1, 0x34(r31)
    lfs      f0, 0x14(r1)
    fmuls    f0, f1, f0
    stfs     f0, 0x34(r31)
    b        .L_801E1C80
.L_801E0D2C:
    mr       r3, r30
    addi     r4, r1, 0x14
    bl       getFloat
    lfs      f1, 0x2c(r31)
    addi     r4, r1, 0x14
    lfs      f0, 0x14(r1)
    fmuls    f0, f1, f0
    stfs     f0, 0x2c(r31)
    bl       getFloat
    lfs      f1, 0x30(r31)
    addi     r4, r1, 0x14
    lfs      f0, 0x14(r1)
    fmuls    f0, f1, f0
    stfs     f0, 0x30(r31)
    bl       getFloat
    lfs      f1, 0x34(r31)
    mr       r30, r3
    lfs      f0, 0x14(r1)
    fmuls    f0, f1, f0
    stfs     f0, 0x34(r31)
    b        .L_801E1C80
.L_801E0D80:
    lbz      r4, 0x0(r30)
    addi     r30, r30, 0x1
    lbz      r0, 0x59(r31)
    lwz      r3, 0x4(r31)
    add      r0, r4, r0
    clrlslwi r0, r0, 29, 12
    ori      r0, r0, 0x8000
    or       r0, r3, r0
    stw      r0, 0x4(r31)
    b        .L_801E1C80
.L_801E0DA8:
    lhz      r4, 0xe(r31)
    cmplwi   r4, 0x0
    beq      .L_801E0E40
    lhz      r0, 0x6c(r31)
    lbz      r5, 0x70(r31)
    slwi     r0, r0, 16
    lbz      r3, 0x12(r31)
    divw     r0, r0, r4
    slwi     r4, r5, 16
    subf     r3, r5, r3
    mullw    r3, r0, r3
    add      r3, r4, r3
    srawi    r3, r3, 16
    stb      r3, 0x12(r31)
    lbz      r5, 0x71(r31)
    lbz      r3, 0x13(r31)
    slwi     r4, r5, 16
    subf     r3, r5, r3
    mullw    r3, r0, r3
    add      r3, r4, r3
    srawi    r3, r3, 16
    stb      r3, 0x13(r31)
    lbz      r5, 0x72(r31)
    lbz      r3, 0x14(r31)
    slwi     r4, r5, 16
    subf     r3, r5, r3
    mullw    r3, r0, r3
    add      r3, r4, r3
    srawi    r3, r3, 16
    stb      r3, 0x14(r31)
    lbz      r5, 0x73(r31)
    lbz      r3, 0x15(r31)
    slwi     r4, r5, 16
    subf     r3, r5, r3
    mullw    r0, r0, r3
    add      r0, r4, r0
    srawi    r0, r0, 16
    stb      r0, 0x15(r31)
.L_801E0E40:
    mr       r3, r30
    addi     r4, r31, 0xe
    bl       getTime
    lwz      r4, 0x12(r31)
    clrlwi.  r0, r26, 31
    mr       r30, r3
    stw      r4, 0x70(r31)
    beq      .L_801E0E6C
    lbz      r0, 0x0(r30)
    addi     r30, r30, 0x1
    stb      r0, 0x70(r31)
.L_801E0E6C:
    rlwinm.  r0, r26, 0, 30, 30
    beq      .L_801E0E80
    lbz      r0, 0x0(r30)
    addi     r30, r30, 0x1
    stb      r0, 0x71(r31)
.L_801E0E80:
    rlwinm.  r0, r26, 0, 29, 29
    beq      .L_801E0E94
    lbz      r0, 0x0(r30)
    addi     r30, r30, 0x1
    stb      r0, 0x72(r31)
.L_801E0E94:
    rlwinm.  r0, r26, 0, 28, 28
    beq      .L_801E0EA8
    lbz      r0, 0x0(r30)
    addi     r30, r30, 0x1
    stb      r0, 0x73(r31)
.L_801E0EA8:
    lhz      r0, 0xe(r31)
    cmplwi   r0, 0x0
    bne      .L_801E0EC8
    lwz      r3, 0x70(r31)
    li       r0, 0x0
    stw      r3, 0x12(r31)
    sth      r0, 0x6c(r31)
    b        .L_801E1C80
.L_801E0EC8:
    sth      r0, 0x6c(r31)
    b        .L_801E1C80
.L_801E0ED0:
    lhz      r4, 0x10(r31)
    cmplwi   r4, 0x0
    beq      .L_801E0F68
    lhz      r0, 0x6e(r31)
    lbz      r5, 0x74(r31)
    slwi     r0, r0, 16
    lbz      r3, 0x16(r31)
    divw     r0, r0, r4
    slwi     r4, r5, 16
    subf     r3, r5, r3
    mullw    r3, r0, r3
    add      r3, r4, r3
    srawi    r3, r3, 16
    stb      r3, 0x16(r31)
    lbz      r5, 0x75(r31)
    lbz      r3, 0x17(r31)
    slwi     r4, r5, 16
    subf     r3, r5, r3
    mullw    r3, r0, r3
    add      r3, r4, r3
    srawi    r3, r3, 16
    stb      r3, 0x17(r31)
    lbz      r5, 0x76(r31)
    lbz      r3, 0x18(r31)
    slwi     r4, r5, 16
    subf     r3, r5, r3
    mullw    r3, r0, r3
    add      r3, r4, r3
    srawi    r3, r3, 16
    stb      r3, 0x18(r31)
    lbz      r5, 0x77(r31)
    lbz      r3, 0x19(r31)
    slwi     r4, r5, 16
    subf     r3, r5, r3
    mullw    r0, r0, r3
    add      r0, r4, r0
    srawi    r0, r0, 16
    stb      r0, 0x19(r31)
.L_801E0F68:
    mr       r3, r30
    addi     r4, r31, 0x10
    bl       getTime
    lwz      r4, 0x16(r31)
    clrlwi.  r0, r26, 31
    mr       r30, r3
    stw      r4, 0x74(r31)
    beq      .L_801E0F94
    lbz      r0, 0x0(r30)
    addi     r30, r30, 0x1
    stb      r0, 0x74(r31)
.L_801E0F94:
    rlwinm.  r0, r26, 0, 30, 30
    beq      .L_801E0FA8
    lbz      r0, 0x0(r30)
    addi     r30, r30, 0x1
    stb      r0, 0x75(r31)
.L_801E0FA8:
    rlwinm.  r0, r26, 0, 29, 29
    beq      .L_801E0FBC
    lbz      r0, 0x0(r30)
    addi     r30, r30, 0x1
    stb      r0, 0x76(r31)
.L_801E0FBC:
    rlwinm.  r0, r26, 0, 28, 28
    beq      .L_801E0FD0
    lbz      r0, 0x0(r30)
    addi     r30, r30, 0x1
    stb      r0, 0x77(r31)
.L_801E0FD0:
    lhz      r0, 0x10(r31)
    cmplwi   r0, 0x0
    bne      .L_801E0FF0
    lwz      r3, 0x74(r31)
    li       r0, 0x0
    stw      r3, 0x16(r31)
    sth      r0, 0x6e(r31)
    b        .L_801E1C80
.L_801E0FF0:
    sth      r0, 0x6e(r31)
    b        .L_801E1C80
.L_801E0FF8:
    lhz      r4, 0xe(r31)
    cmplwi   r4, 0x0
    beq      .L_801E1090
    lhz      r0, 0x6c(r31)
    lbz      r5, 0x70(r31)
    slwi     r0, r0, 16
    lbz      r3, 0x12(r31)
    divw     r0, r0, r4
    slwi     r4, r5, 16
    subf     r3, r5, r3
    mullw    r3, r0, r3
    add      r3, r4, r3
    srawi    r3, r3, 16
    stb      r3, 0x12(r31)
    lbz      r5, 0x71(r31)
    lbz      r3, 0x13(r31)
    slwi     r4, r5, 16
    subf     r3, r5, r3
    mullw    r3, r0, r3
    add      r3, r4, r3
    srawi    r3, r3, 16
    stb      r3, 0x13(r31)
    lbz      r5, 0x72(r31)
    lbz      r3, 0x14(r31)
    slwi     r4, r5, 16
    subf     r3, r5, r3
    mullw    r3, r0, r3
    add      r3, r4, r3
    srawi    r3, r3, 16
    stb      r3, 0x14(r31)
    lbz      r5, 0x73(r31)
    lbz      r3, 0x15(r31)
    slwi     r4, r5, 16
    subf     r3, r5, r3
    mullw    r0, r0, r3
    add      r0, r4, r0
    srawi    r0, r0, 16
    stb      r0, 0x15(r31)
.L_801E1090:
    lhz      r4, 0x10(r31)
    cmplwi   r4, 0x0
    beq      .L_801E1128
    lhz      r0, 0x6e(r31)
    lbz      r5, 0x74(r31)
    slwi     r0, r0, 16
    lbz      r3, 0x16(r31)
    divw     r0, r0, r4
    slwi     r4, r5, 16
    subf     r3, r5, r3
    mullw    r3, r0, r3
    add      r3, r4, r3
    srawi    r3, r3, 16
    stb      r3, 0x16(r31)
    lbz      r5, 0x75(r31)
    lbz      r3, 0x17(r31)
    slwi     r4, r5, 16
    subf     r3, r5, r3
    mullw    r3, r0, r3
    add      r3, r4, r3
    srawi    r3, r3, 16
    stb      r3, 0x17(r31)
    lbz      r5, 0x76(r31)
    lbz      r3, 0x18(r31)
    slwi     r4, r5, 16
    subf     r3, r5, r3
    mullw    r3, r0, r3
    add      r3, r4, r3
    srawi    r3, r3, 16
    stb      r3, 0x18(r31)
    lbz      r5, 0x77(r31)
    lbz      r3, 0x19(r31)
    slwi     r4, r5, 16
    subf     r3, r5, r3
    mullw    r0, r0, r3
    add      r0, r4, r0
    srawi    r0, r0, 16
    stb      r0, 0x19(r31)
.L_801E1128:
    bl       HSD_Randf
    lbz      r3, 0x0(r30)
    lis      r0, 0x4330
    stw      r0, 0x30(r1)
    extsb    r0, r3
    lfd      f2, "@1011"@sda21(r0)
    slwi     r0, r0, 1
    xoris    r0, r0, 0x8000
    stw      r0, 0x34(r1)
    lfd      f0, 0x30(r1)
    fsubs    f0, f0, f2
    fmuls    f1, f0, f1
    stfs     f1, 0x14(r1)
    lbz      r3, 0x70(r31)
    bl       U8ClampAdd
    stb      r3, 0x70(r31)
    lbz      r3, 0x74(r31)
    lfs      f1, 0x14(r1)
    bl       U8ClampAdd
    stb      r3, 0x74(r31)
    bl       HSD_Randf
    lbz      r3, 0x1(r30)
    lis      r0, 0x4330
    stw      r0, 0x28(r1)
    extsb    r0, r3
    lfd      f2, "@1011"@sda21(r0)
    slwi     r0, r0, 1
    xoris    r0, r0, 0x8000
    stw      r0, 0x2c(r1)
    lfd      f0, 0x28(r1)
    fsubs    f0, f0, f2
    fmuls    f1, f0, f1
    stfs     f1, 0x14(r1)
    lbz      r3, 0x71(r31)
    bl       U8ClampAdd
    stb      r3, 0x71(r31)
    lbz      r3, 0x75(r31)
    lfs      f1, 0x14(r1)
    bl       U8ClampAdd
    stb      r3, 0x75(r31)
    bl       HSD_Randf
    lbz      r3, 0x2(r30)
    lis      r0, 0x4330
    stw      r0, 0x20(r1)
    extsb    r0, r3
    lfd      f2, "@1011"@sda21(r0)
    slwi     r0, r0, 1
    xoris    r0, r0, 0x8000
    stw      r0, 0x24(r1)
    lfd      f0, 0x20(r1)
    fsubs    f0, f0, f2
    fmuls    f1, f0, f1
    stfs     f1, 0x14(r1)
    lbz      r3, 0x72(r31)
    bl       U8ClampAdd
    stb      r3, 0x72(r31)
    lbz      r3, 0x76(r31)
    lfs      f1, 0x14(r1)
    bl       U8ClampAdd
    stb      r3, 0x76(r31)
    bl       HSD_Randf
    lbz      r3, 0x3(r30)
    lis      r0, 0x4330
    stw      r0, 0x18(r1)
    addi     r30, r30, 0x4
    extsb    r0, r3
    lfd      f2, "@1011"@sda21(r0)
    slwi     r0, r0, 1
    xoris    r0, r0, 0x8000
    stw      r0, 0x1c(r1)
    lfd      f0, 0x18(r1)
    fsubs    f0, f0, f2
    fmuls    f1, f0, f1
    stfs     f1, 0x14(r1)
    lbz      r3, 0x73(r31)
    bl       U8ClampAdd
    stb      r3, 0x73(r31)
    lbz      r3, 0x77(r31)
    lfs      f1, 0x14(r1)
    bl       U8ClampAdd
    stb      r3, 0x77(r31)
    lhz      r0, 0xe(r31)
    cmplwi   r0, 0x0
    bne      .L_801E1280
    lwz      r0, 0x70(r31)
    stw      r0, 0x12(r31)
.L_801E1280:
    lhz      r0, 0xe(r31)
    sth      r0, 0x6c(r31)
    lhz      r0, 0x10(r31)
    cmplwi   r0, 0x0
    bne      .L_801E129C
    lwz      r0, 0x74(r31)
    stw      r0, 0x16(r31)
.L_801E129C:
    lhz      r0, 0x10(r31)
    sth      r0, 0x6e(r31)
    b        .L_801E1C80
.L_801E12A8:
    lhz      r4, 0xe(r31)
    cmplwi   r4, 0x0
    beq      .L_801E1340
    lhz      r0, 0x6c(r31)
    lbz      r5, 0x70(r31)
    slwi     r0, r0, 16
    lbz      r3, 0x12(r31)
    divw     r0, r0, r4
    slwi     r4, r5, 16
    subf     r3, r5, r3
    mullw    r3, r0, r3
    add      r3, r4, r3
    srawi    r3, r3, 16
    stb      r3, 0x12(r31)
    lbz      r5, 0x71(r31)
    lbz      r3, 0x13(r31)
    slwi     r4, r5, 16
    subf     r3, r5, r3
    mullw    r3, r0, r3
    add      r3, r4, r3
    srawi    r3, r3, 16
    stb      r3, 0x13(r31)
    lbz      r5, 0x72(r31)
    lbz      r3, 0x14(r31)
    slwi     r4, r5, 16
    subf     r3, r5, r3
    mullw    r3, r0, r3
    add      r3, r4, r3
    srawi    r3, r3, 16
    stb      r3, 0x14(r31)
    lbz      r5, 0x73(r31)
    lbz      r3, 0x15(r31)
    slwi     r4, r5, 16
    subf     r3, r5, r3
    mullw    r0, r0, r3
    add      r0, r4, r0
    srawi    r0, r0, 16
    stb      r0, 0x15(r31)
.L_801E1340:
    lhz      r4, 0x10(r31)
    cmplwi   r4, 0x0
    beq      .L_801E13D8
    lhz      r0, 0x6e(r31)
    lbz      r5, 0x74(r31)
    slwi     r0, r0, 16
    lbz      r3, 0x16(r31)
    divw     r0, r0, r4
    slwi     r4, r5, 16
    subf     r3, r5, r3
    mullw    r3, r0, r3
    add      r3, r4, r3
    srawi    r3, r3, 16
    stb      r3, 0x16(r31)
    lbz      r5, 0x75(r31)
    lbz      r3, 0x17(r31)
    slwi     r4, r5, 16
    subf     r3, r5, r3
    mullw    r3, r0, r3
    add      r3, r4, r3
    srawi    r3, r3, 16
    stb      r3, 0x17(r31)
    lbz      r5, 0x76(r31)
    lbz      r3, 0x18(r31)
    slwi     r4, r5, 16
    subf     r3, r5, r3
    mullw    r3, r0, r3
    add      r3, r4, r3
    srawi    r3, r3, 16
    stb      r3, 0x18(r31)
    lbz      r5, 0x77(r31)
    lbz      r3, 0x19(r31)
    slwi     r4, r5, 16
    subf     r3, r5, r3
    mullw    r0, r0, r3
    add      r0, r4, r0
    srawi    r0, r0, 16
    stb      r0, 0x19(r31)
.L_801E13D8:
    lbz      r27, 0x1(r30)
    lbz      r26, 0x0(r30)
    addi     r30, r30, 0x2
    cmpwi    r27, 0x0
    beq      .L_801E1450
    bl       HSD_Randf
    addi     r0, r27, 0x1
    lis      r3, 0x4330
    xoris    r4, r0, 0x8000
    xoris    r0, r27, 0x8000
    stw      r4, 0x34(r1)
    lfd      f2, "@1011"@sda21(r0)
    stw      r3, 0x30(r1)
    lfd      f0, 0x30(r1)
    stw      r3, 0x20(r1)
    fsubs    f0, f0, f2
    stw      r0, 0x1c(r1)
    fmuls    f1, f0, f1
    stw      r3, 0x18(r1)
    lfd      f0, 0x18(r1)
    fctiwz   f1, f1
    fsubs    f0, f0, f2
    stfd     f1, 0x28(r1)
    lwz      r0, 0x2c(r1)
    xoris    r0, r0, 0x8000
    stw      r0, 0x24(r1)
    lfd      f1, 0x20(r1)
    fsubs    f1, f1, f2
    fdivs    f1, f1, f0
    b        .L_801E1454
.L_801E1450:
    bl       HSD_Randf
.L_801E1454:
    clrlwi.  r0, r26, 31
    stfs     f1, 0x14(r1)
    beq      .L_801E14C4
    lbz      r4, 0x0(r30)
    lis      r3, 0x4330
    stw      r3, 0x30(r1)
    frsp     f1, f1
    extsb    r3, r4
    lfd      f2, "@1011"@sda21(r0)
    slwi     r3, r3, 1
    rlwinm.  r0, r26, 0, 27, 27
    xoris    r3, r3, 0x8000
    stw      r3, 0x34(r1)
    addi     r30, r30, 0x1
    lfd      f0, 0x30(r1)
    fsubs    f0, f0, f2
    fmuls    f1, f1, f0
    stfs     f1, 0x10(r1)
    beq      .L_801E14AC
    lbz      r3, 0x70(r31)
    bl       U8ClampAdd
    stb      r3, 0x70(r31)
.L_801E14AC:
    rlwinm.  r0, r26, 0, 26, 26
    beq      .L_801E14C4
    lbz      r3, 0x74(r31)
    lfs      f1, 0x10(r1)
    bl       U8ClampAdd
    stb      r3, 0x74(r31)
.L_801E14C4:
    rlwinm.  r0, r26, 0, 30, 30
    beq      .L_801E1530
    lbz      r4, 0x0(r30)
    lis      r3, 0x4330
    stw      r3, 0x30(r1)
    rlwinm.  r0, r26, 0, 27, 27
    extsb    r3, r4
    lfd      f1, "@1011"@sda21(r0)
    slwi     r3, r3, 1
    lfs      f2, 0x14(r1)
    xoris    r3, r3, 0x8000
    addi     r30, r30, 0x1
    stw      r3, 0x34(r1)
    lfd      f0, 0x30(r1)
    fsubs    f0, f0, f1
    fmuls    f1, f2, f0
    stfs     f1, 0x10(r1)
    beq      .L_801E1518
    lbz      r3, 0x71(r31)
    bl       U8ClampAdd
    stb      r3, 0x71(r31)
.L_801E1518:
    rlwinm.  r0, r26, 0, 26, 26
    beq      .L_801E1530
    lbz      r3, 0x75(r31)
    lfs      f1, 0x10(r1)
    bl       U8ClampAdd
    stb      r3, 0x75(r31)
.L_801E1530:
    rlwinm.  r0, r26, 0, 29, 29
    beq      .L_801E159C
    lbz      r4, 0x0(r30)
    lis      r3, 0x4330
    stw      r3, 0x30(r1)
    rlwinm.  r0, r26, 0, 27, 27
    extsb    r3, r4
    lfd      f1, "@1011"@sda21(r0)
    slwi     r3, r3, 1
    lfs      f2, 0x14(r1)
    xoris    r3, r3, 0x8000
    addi     r30, r30, 0x1
    stw      r3, 0x34(r1)
    lfd      f0, 0x30(r1)
    fsubs    f0, f0, f1
    fmuls    f1, f2, f0
    stfs     f1, 0x10(r1)
    beq      .L_801E1584
    lbz      r3, 0x72(r31)
    bl       U8ClampAdd
    stb      r3, 0x72(r31)
.L_801E1584:
    rlwinm.  r0, r26, 0, 26, 26
    beq      .L_801E159C
    lbz      r3, 0x76(r31)
    lfs      f1, 0x10(r1)
    bl       U8ClampAdd
    stb      r3, 0x76(r31)
.L_801E159C:
    rlwinm.  r0, r26, 0, 28, 28
    beq      .L_801E16A0
    cmpwi    r27, 0x0
    beq      .L_801E1638
    bl       HSD_Randf
    addi     r0, r27, 0x1
    lis      r4, 0x4330
    xoris    r0, r0, 0x8000
    lbz      r3, 0x0(r30)
    stw      r0, 0x34(r1)
    xoris    r0, r27, 0x8000
    extsb    r3, r3
    lfd      f3, "@1011"@sda21(r0)
    stw      r4, 0x30(r1)
    slwi     r3, r3, 1
    xoris    r3, r3, 0x8000
    addi     r30, r30, 0x1
    lfd      f0, 0x30(r1)
    stw      r0, 0x3c(r1)
    fsubs    f0, f0, f3
    stw      r4, 0x20(r1)
    fmuls    f1, f0, f1
    stw      r3, 0x1c(r1)
    stw      r4, 0x18(r1)
    fctiwz   f2, f1
    lfd      f0, 0x18(r1)
    stw      r4, 0x38(r1)
    fsubs    f1, f0, f3
    stfd     f2, 0x28(r1)
    lfd      f0, 0x38(r1)
    lwz      r0, 0x2c(r1)
    fsubs    f0, f0, f3
    xoris    r0, r0, 0x8000
    stw      r0, 0x24(r1)
    lfd      f2, 0x20(r1)
    fsubs    f2, f2, f3
    fmuls    f1, f1, f2
    fdivs    f0, f1, f0
    b        .L_801E166C
.L_801E1638:
    bl       HSD_Randf
    lbz      r3, 0x0(r30)
    lis      r0, 0x4330
    stw      r0, 0x40(r1)
    addi     r30, r30, 0x1
    extsb    r0, r3
    lfd      f2, "@1011"@sda21(r0)
    slwi     r0, r0, 1
    xoris    r0, r0, 0x8000
    stw      r0, 0x44(r1)
    lfd      f0, 0x40(r1)
    fsubs    f0, f0, f2
    fmuls    f0, f0, f1
.L_801E166C:
    rlwinm.  r0, r26, 0, 27, 27
    stfs     f0, 0x14(r1)
    beq      .L_801E1688
    frsp     f1, f0
    lbz      r3, 0x73(r31)
    bl       U8ClampAdd
    stb      r3, 0x73(r31)
.L_801E1688:
    rlwinm.  r0, r26, 0, 26, 26
    beq      .L_801E16A0
    lbz      r3, 0x77(r31)
    lfs      f1, 0x14(r1)
    bl       U8ClampAdd
    stb      r3, 0x77(r31)
.L_801E16A0:
    lhz      r0, 0xe(r31)
    cmplwi   r0, 0x0
    bne      .L_801E16B4
    lwz      r0, 0x70(r31)
    stw      r0, 0x12(r31)
.L_801E16B4:
    lhz      r0, 0xe(r31)
    sth      r0, 0x6c(r31)
    lhz      r0, 0x10(r31)
    cmplwi   r0, 0x0
    bne      .L_801E16D0
    lwz      r0, 0x74(r31)
    stw      r0, 0x16(r31)
.L_801E16D0:
    lhz      r0, 0x10(r31)
    sth      r0, 0x6e(r31)
    b        .L_801E1C80
.L_801E16DC:
    lbz      r0, 0x0(r30)
    addi     r30, r30, 0x1
    cmpwi    r0, 0x0
    bne      .L_801E16F8
    li       r0, 0x0
    stw      r0, 0x98(r31)
    b        .L_801E1C80
.L_801E16F8:
    slwi     r3, r0, 2
    lwz      r4, lbl_8065BA80@sda21(r0)
    subi     r0, r3, 0x4
    lwzx     r0, r4, r0
    stw      r0, 0x98(r31)
    b        .L_801E1C80
.L_801E1710:
    lbz      r26, 0x0(r30)
    addi     r30, r30, 0x1
    mr       r3, r30
    addi     r4, r1, 0x14
    bl       getFloat
    lwz      r4, 0x8c(r31)
    mr       r30, r3
    lwz      r3, 0x58(r4)
    cmplwi   r3, 0x0
    beq      .L_801E175C
    lwz      r12, 0x8(r3)
    cmplwi   r12, 0x0
    beq      .L_801E175C
    mr       r3, r31
    clrlwi   r4, r26, 24
    lfs      f1, 0x14(r1)
    mtctr    r12
    bctrl
    b        .L_801E1C80
.L_801E175C:
    lwz      r3, 0x94(r31)
    cmplwi   r3, 0x0
    beq      .L_801E1C80
    lfs      f0, 0x14(r1)
    slwi     r0, r26, 2
    stfsx    f0, r3, r0
    b        .L_801E1C80
.L_801E1778:
    lwz      r0, 0x4(r31)
    ori      r0, r0, 0x8
    stw      r0, 0x4(r31)
    b        .L_801E1C80
.L_801E1788:
    lbz      r0, 0x0(r30)
    addi     r30, r30, 0x1
    stb      r0, 0xb(r31)
    b        .L_801E1C80
.L_801E1798:
    lbz      r0, 0x0(r30)
    addi     r30, r30, 0x1
    clrlwi   r0, r0, 30
    cmpwi    r0, 0x2
    beq      .L_801E17EC
    bge      .L_801E17C0
    cmpwi    r0, 0x0
    beq      .L_801E17CC
    bge      .L_801E17DC
    b        .L_801E1C80
.L_801E17C0:
    cmpwi    r0, 0x4
    bge      .L_801E1C80
    b        .L_801E17FC
.L_801E17CC:
    lwz      r0, 0x4(r31)
    rlwinm   r0, r0, 0, 14, 12
    stw      r0, 0x4(r31)
    b        .L_801E1C80
.L_801E17DC:
    lwz      r0, 0x4(r31)
    oris     r0, r0, 0x4
    stw      r0, 0x4(r31)
    b        .L_801E1C80
.L_801E17EC:
    lwz      r0, 0x4(r31)
    xoris    r0, r0, 0x4
    stw      r0, 0x4(r31)
    b        .L_801E1C80
.L_801E17FC:
    bl       HSD_Randf
    lfs      f0, "@1006"@sda21(r0)
    fcmpo    cr0, f1, f0
    bge      .L_801E181C
    lwz      r0, 0x4(r31)
    rlwinm   r0, r0, 0, 14, 12
    stw      r0, 0x4(r31)
    b        .L_801E1C80
.L_801E181C:
    lwz      r0, 0x4(r31)
    oris     r0, r0, 0x4
    stw      r0, 0x4(r31)
    b        .L_801E1C80
.L_801E182C:
    lbz      r0, 0x0(r30)
    addi     r30, r30, 0x1
    clrlwi   r0, r0, 30
    cmpwi    r0, 0x2
    beq      .L_801E1880
    bge      .L_801E1854
    cmpwi    r0, 0x0
    beq      .L_801E1860
    bge      .L_801E1870
    b        .L_801E1C80
.L_801E1854:
    cmpwi    r0, 0x4
    bge      .L_801E1C80
    b        .L_801E1890
.L_801E1860:
    lwz      r0, 0x4(r31)
    rlwinm   r0, r0, 0, 13, 11
    stw      r0, 0x4(r31)
    b        .L_801E1C80
.L_801E1870:
    lwz      r0, 0x4(r31)
    oris     r0, r0, 0x8
    stw      r0, 0x4(r31)
    b        .L_801E1C80
.L_801E1880:
    lwz      r0, 0x4(r31)
    xoris    r0, r0, 0x8
    stw      r0, 0x4(r31)
    b        .L_801E1C80
.L_801E1890:
    bl       HSD_Randf
    lfs      f0, "@1006"@sda21(r0)
    fcmpo    cr0, f1, f0
    bge      .L_801E18B0
    lwz      r0, 0x4(r31)
    rlwinm   r0, r0, 0, 13, 11
    stw      r0, 0x4(r31)
    b        .L_801E1C80
.L_801E18B0:
    lwz      r0, 0x4(r31)
    oris     r0, r0, 0x8
    stw      r0, 0x4(r31)
    b        .L_801E1C80
.L_801E18C0:
    lwz      r0, 0x4(r31)
    oris     r0, r0, 0x20
    stw      r0, 0x4(r31)
    b        .L_801E1C80
.L_801E18D0:
    lwz      r0, 0x4(r31)
    rlwinm   r0, r0, 0, 11, 9
    stw      r0, 0x4(r31)
    b        .L_801E1C80
.L_801E18E0:
    mr       r3, r30
    addi     r4, r1, 0x14
    bl       getFloat
    lfs      f1, 0x14(r1)
    mr       r30, r3
    lfs      f0, "@195"@sda21(r0)
    fcmpo    cr0, f1, f0
    bge      .L_801E1910
    lwz      r0, 0x4(r31)
    rlwinm   r0, r0, 0, 12, 10
    stw      r0, 0x4(r31)
    b        .L_801E1C80
.L_801E1910:
    lwz      r0, 0x4(r31)
    oris     r0, r0, 0x10
    stw      r0, 0x4(r31)
    lfs      f0, 0x14(r1)
    stfs     f0, 0x88(r31)
    b        .L_801E1C80
.L_801E1928:
    lhz      r4, 0x5a(r31)
    cmplwi   r4, 0x0
    beq      .L_801E1980
    lhz      r0, 0x78(r31)
    lbz      r5, 0x84(r31)
    slwi     r3, r0, 16
    lbz      r0, 0x80(r31)
    divw     r6, r3, r4
    subf     r0, r5, r0
    slwi     r3, r5, 16
    mullw    r0, r6, r0
    add      r0, r3, r0
    srawi    r0, r0, 16
    stb      r0, 0x80(r31)
    lbz      r4, 0x85(r31)
    lbz      r0, 0x81(r31)
    slwi     r3, r4, 16
    subf     r0, r4, r0
    mullw    r0, r6, r0
    add      r0, r3, r0
    srawi    r0, r0, 16
    stb      r0, 0x81(r31)
.L_801E1980:
    mr       r3, r30
    addi     r4, r31, 0x5a
    bl       getTime
    lbz      r4, 0x0(r3)
    addi     r30, r3, 0x1
    lbz      r3, 0x80(r31)
    clrlwi.  r0, r4, 31
    stb      r3, 0x84(r31)
    beq      .L_801E19B0
    lbz      r0, 0x0(r30)
    addi     r30, r30, 0x1
    stb      r0, 0x84(r31)
.L_801E19B0:
    rlwinm.  r0, r4, 0, 28, 28
    beq      .L_801E19C4
    lbz      r0, 0x0(r30)
    addi     r30, r30, 0x1
    stb      r0, 0x85(r31)
.L_801E19C4:
    lhz      r0, 0x5a(r31)
    cmplwi   r0, 0x0
    bne      .L_801E19E4
    lbz      r3, 0x84(r31)
    li       r0, 0x0
    stb      r3, 0x80(r31)
    sth      r0, 0x78(r31)
    b        .L_801E1C80
.L_801E19E4:
    sth      r0, 0x78(r31)
    b        .L_801E1C80
.L_801E19EC:
    lhz      r4, 0x5c(r31)
    cmplwi   r4, 0x0
    beq      .L_801E1A44
    lhz      r0, 0x7a(r31)
    lbz      r5, 0x86(r31)
    slwi     r3, r0, 16
    lbz      r0, 0x82(r31)
    divw     r6, r3, r4
    subf     r0, r5, r0
    slwi     r3, r5, 16
    mullw    r0, r6, r0
    add      r0, r3, r0
    srawi    r0, r0, 16
    stb      r0, 0x82(r31)
    lbz      r4, 0x87(r31)
    lbz      r0, 0x83(r31)
    slwi     r3, r4, 16
    subf     r0, r4, r0
    mullw    r0, r6, r0
    add      r0, r3, r0
    srawi    r0, r0, 16
    stb      r0, 0x83(r31)
.L_801E1A44:
    mr       r3, r30
    addi     r4, r31, 0x5c
    bl       getTime
    lbz      r4, 0x0(r3)
    addi     r30, r3, 0x1
    lbz      r3, 0x82(r31)
    clrlwi.  r0, r4, 31
    stb      r3, 0x86(r31)
    beq      .L_801E1A74
    lbz      r0, 0x0(r30)
    addi     r30, r30, 0x1
    stb      r0, 0x86(r31)
.L_801E1A74:
    rlwinm.  r0, r4, 0, 28, 28
    beq      .L_801E1A88
    lbz      r0, 0x0(r30)
    addi     r30, r30, 0x1
    stb      r0, 0x87(r31)
.L_801E1A88:
    lhz      r0, 0x5c(r31)
    cmplwi   r0, 0x0
    bne      .L_801E1AA8
    lbz      r3, 0x86(r31)
    li       r0, 0x0
    stb      r3, 0x82(r31)
    sth      r0, 0x7a(r31)
    b        .L_801E1C80
.L_801E1AA8:
    sth      r0, 0x7a(r31)
    b        .L_801E1C80
.L_801E1AB0:
    mr       r3, r30
    addi     r4, r1, 0x14
    bl       getFloat
    addi     r4, r1, 0x10
    bl       getFloat
    lbz      r26, 0x0(r3)
    addi     r30, r3, 0x1
    cmpwi    r26, 0x0
    beq      .L_801E1B48
    bl       HSD_Randf
    addi     r0, r26, 0x1
    lis      r3, 0x4330
    xoris    r4, r0, 0x8000
    xoris    r0, r26, 0x8000
    stw      r4, 0x44(r1)
    lfd      f4, "@1011"@sda21(r0)
    stw      r3, 0x40(r1)
    lfs      f2, 0x10(r1)
    lfd      f0, 0x40(r1)
    stw      r3, 0x30(r1)
    fsubs    f3, f0, f4
    lfs      f0, 0x14(r1)
    stw      r0, 0x2c(r1)
    fmuls    f3, f3, f1
    stw      r3, 0x28(r1)
    lfd      f1, 0x28(r1)
    fctiwz   f3, f3
    fsubs    f1, f1, f4
    stfd     f3, 0x38(r1)
    lwz      r0, 0x3c(r1)
    xoris    r0, r0, 0x8000
    stw      r0, 0x34(r1)
    lfd      f3, 0x30(r1)
    fsubs    f3, f3, f4
    fmuls    f2, f2, f3
    fdivs    f1, f2, f1
    fadds    f0, f0, f1
    b        .L_801E1B58
.L_801E1B48:
    bl       HSD_Randf
    lfs      f2, 0x10(r1)
    lfs      f0, 0x14(r1)
    fmadds   f0, f2, f1, f0
.L_801E1B58:
    stfs     f0, 0x14(r1)
    frsp     f0, f0
    lfs      f1, 0x64(r31)
    fadds    f0, f1, f0
    stfs     f0, 0x64(r31)
    lfs      f1, 0x50(r31)
    lfs      f0, 0x14(r1)
    fadds    f0, f1, f0
    stfs     f0, 0x50(r31)
    b        .L_801E1C80
.L_801E1B80:
    lbz      r26, 0x0(r30)
    addi     r30, r30, 0x1
    mr       r3, r30
    addi     r4, r31, 0x64
    bl       getFloat
    addi     r4, r31, 0x68
    bl       getFloat
    addi     r4, r31, 0x5e
    bl       getTime
    lhz      r0, 0x5e(r31)
    mr       r30, r3
    cmplwi   r0, 0x0
    beq      .L_801E1C04
    cmpwi    r26, 0x0
    bne      .L_801E1BD8
    lfs      f2, 0x68(r31)
    lfd      f1, "@198"@sda21(r0)
    lfs      f0, 0x64(r31)
    fmadd    f0, f2, f1, f0
    frsp     f0, f0
    stfs     f0, 0x64(r31)
    b        .L_801E1C80
.L_801E1BD8:
    lfs      f2, 0x64(r31)
    lfs      f0, "@1007"@sda21(r0)
    lfd      f1, "@198"@sda21(r0)
    fmuls    f0, f2, f0
    stfs     f0, 0x64(r31)
    lfs      f2, 0x68(r31)
    lfs      f0, 0x64(r31)
    fnmsub   f0, f2, f1, f0
    frsp     f0, f0
    stfs     f0, 0x64(r31)
    b        .L_801E1C80
.L_801E1C04:
    lfs      f0, "@195"@sda21(r0)
    stfs     f0, 0x64(r31)
    stfs     f0, 0x68(r31)
    b        .L_801E1C80
.L_801E1C14:
    lbz      r0, 0x0(r30)
    addi     r30, r30, 0x1
    stb      r0, 0x1c(r31)
    lwz      r0, 0x20(r31)
    subf     r0, r0, r30
    sth      r0, 0x28(r31)
    b        .L_801E1C80
.L_801E1C30:
    lbz      r3, 0x1c(r31)
    subi     r3, r3, 0x1
    clrlwi.  r0, r3, 24
    stb      r3, 0x1c(r31)
    beq      .L_801E1C80
    lwz      r3, 0x20(r31)
    lhz      r0, 0x28(r31)
    add      r30, r3, r0
    b        .L_801E1C80
.L_801E1C54:
    lwz      r0, 0x20(r31)
    subf     r0, r0, r30
    sth      r0, 0x26(r31)
    b        .L_801E1C80
.L_801E1C64:
    lwz      r3, 0x20(r31)
    lhz      r0, 0x26(r31)
    add      r30, r3, r0
    b        .L_801E1C80
.L_801E1C74:
    li       r0, 0x1
    sth      r0, 0x2a(r31)
    b        .L_801E1C88
.L_801E1C80:
    clrlwi.  r0, r29, 16
    beq      .L_801DF39C
.L_801E1C88:
    lwz      r0, 0x20(r31)
    subf     r0, r0, r30
    sth      r0, 0x24(r31)
    sth      r29, 0x1a(r31)
.L_801E1C98:
    lhz      r3, 0x2a(r31)
    subi     r3, r3, 0x1
    clrlwi.  r0, r3, 16
    sth      r3, 0x2a(r31)
    bne      .L_801E1D5C
.L_801E1CAC:
    lwz      r3, 0x8c(r31)
    cmplwi   r3, 0x0
    beq      .L_801E1CDC
    lwz      r3, 0x58(r3)
    cmplwi   r3, 0x0
    beq      .L_801E1CDC
    lwz      r12, 0x4(r3)
    cmplwi   r12, 0x0
    beq      .L_801E1CDC
    mr       r3, r31
    mtctr    r12
    bctrl
.L_801E1CDC:
    lwz      r4, 0x8c(r31)
    cmplwi   r4, 0x0
    beq      .L_801E1CF4
    lwz      r3, 0x50(r4)
    subi     r0, r3, 0x1
    stw      r0, 0x50(r4)
.L_801E1CF4:
    mr       r3, r31
    bl       _psListGetNext
    lwz      r0, 0x90(r31)
    mr       r26, r3
    cmplwi   r0, 0x0
    beq      .L_801E1D40
    mr       r3, r31
    bl       psRemoveParticleAppSRT
    cmpwi    r3, 0x0
    bne      .L_801E1D40
    cmplwi   r28, 0x0
    bne      .L_801E1D40
    lbz      r3, 0x1d(r31)
    bl       _psListGetFirst
    cmplw    r3, r26
    beq      .L_801E1D40
    lbz      r3, 0x1d(r31)
    bl       _psListGetFirst
    mr       r26, r3
.L_801E1D40:
    mr       r3, r31
    bl       psDeletePntJObjwithParticle
    mr       r3, r31
    mr       r4, r28
    bl       _psListDelete
    mr       r3, r26
    b        .L_801E1FB0
.L_801E1D5C:
    lwz      r3, 0x4(r31)
    rlwinm.  r0, r3, 0, 29, 29
    beq      .L_801E1E70
    lwz      r26, 0x8c(r31)
    lfs      f1, 0x38(r31)
    bl       sinf
    fmr      f28, f1
    lfs      f1, 0x3c(r31)
    bl       sinf
    fmr      f29, f1
    lfs      f1, 0x38(r31)
    bl       cosf
    fmr      f30, f1
    lfs      f1, 0x3c(r31)
    bl       cosf
    lfs      f2, 0x34(r31)
    fmr      f31, f1
    lfs      f1, 0x60(r26)
    lfs      f0, "@195"@sda21(r0)
    fadds    f1, f2, f1
    stfs     f1, 0x34(r31)
    lfs      f27, 0x48(r26)
    fcmpo    cr0, f27, f0
    bge      .L_801E1DC0
    fneg     f27, f27
.L_801E1DC0:
    lfs      f1, 0x4c(r26)
    lfs      f0, "@195"@sda21(r0)
    fcmpo    cr0, f1, f0
    bge      .L_801E1DD4
    fneg     f1, f1
.L_801E1DD4:
    bl       tanf
    lfs      f2, 0x2c(r31)
    lfs      f0, 0x3c(r26)
    lfs      f3, 0x34(r31)
    fadds    f0, f2, f0
    lfs      f2, 0x30(r31)
    fmadds   f27, f3, f1, f27
    stfs     f0, 0x2c(r31)
    fmuls    f27, f27, f2
    lfs      f1, 0x2c(r31)
    bl       cosf
    fmuls    f26, f27, f1
    lfs      f1, 0x2c(r31)
    bl       sinf
    lfs      f8, 0x34(r31)
    fmuls    f7, f27, f1
    fneg     f6, f26
    lfs      f5, 0x24(r26)
    fmuls    f2, f8, f29
    fmuls    f0, f7, f30
    fmuls    f1, f6, f28
    fmadds   f4, f26, f31, f2
    fmuls    f3, f8, f28
    fmadds   f2, f29, f1, f0
    fadds    f4, f5, f4
    fmuls    f1, f6, f30
    fmuls    f0, f7, f28
    stfs     f4, 0x40(r31)
    fmadds   f3, f31, f3, f2
    fmuls    f2, f8, f30
    lfs      f4, 0x28(r26)
    fmsubs   f0, f29, f1, f0
    fadds    f1, f4, f3
    fmadds   f0, f31, f2, f0
    stfs     f1, 0x44(r31)
    lfs      f1, 0x2c(r26)
    fadds    f0, f1, f0
    stfs     f0, 0x48(r31)
    b        .L_801E1EF4
.L_801E1E70:
    clrlwi.  r0, r3, 31
    beq      .L_801E1E88
    lfs      f1, 0x30(r31)
    lfs      f0, 0x38(r31)
    fsubs    f0, f1, f0
    stfs     f0, 0x30(r31)
.L_801E1E88:
    lwz      r0, 0x4(r31)
    rlwinm.  r0, r0, 0, 30, 30
    beq      .L_801E1EC4
    lfs      f1, 0x2c(r31)
    lfs      f0, 0x3c(r31)
    fmuls    f0, f1, f0
    stfs     f0, 0x2c(r31)
    lfs      f1, 0x30(r31)
    lfs      f0, 0x3c(r31)
    fmuls    f0, f1, f0
    stfs     f0, 0x30(r31)
    lfs      f1, 0x34(r31)
    lfs      f0, 0x3c(r31)
    fmuls    f0, f1, f0
    stfs     f0, 0x34(r31)
.L_801E1EC4:
    lfs      f1, 0x40(r31)
    lfs      f0, 0x2c(r31)
    fadds    f0, f1, f0
    stfs     f0, 0x40(r31)
    lfs      f1, 0x44(r31)
    lfs      f0, 0x30(r31)
    fadds    f0, f1, f0
    stfs     f0, 0x44(r31)
    lfs      f1, 0x48(r31)
    lfs      f0, 0x34(r31)
    fadds    f0, f1, f0
    stfs     f0, 0x48(r31)
.L_801E1EF4:
    lwz      r4, 0x4(r31)
    rlwinm.  r0, r4, 0, 16, 16
    beq      .L_801E1F88
    lis      r3, psPointJObj@ha
    rlwinm   r27, r4, 22, 27, 29
    addi     r26, r3, psPointJObj@l
    extrwi   r29, r4, 3, 17
    lwzx     r0, r26, r27
    cmplwi   r0, 0x0
    bne      .L_801E1F3C
    bl       HSD_JObjAlloc
    mr.      r30, r3
    beq      .L_801E1F3C
    mr       r4, r30
    addi     r3, r29, 0x1
    bl       psSetPointJObj
    mr       r3, r30
    bl       HSD_JObjUnref
.L_801E1F3C:
    lwzx     r3, r26, r27
    cmplwi   r3, 0x0
    beq      .L_801E1F88
    bl       SetupDirtyParticleJointMatrix
    lwzx     r3, r26, r27
    lfs      f1, 0x40(r31)
    lfs      f0, 0x50(r3)
    fsubs    f1, f1, f0
    bl       HSD_JObjAddTx
    lwzx     r3, r26, r27
    lfs      f1, 0x44(r31)
    lfs      f0, 0x60(r3)
    fsubs    f1, f1, f0
    bl       HSD_JObjAddTy
    lwzx     r3, r26, r27
    lfs      f1, 0x48(r31)
    lfs      f0, 0x70(r3)
    fsubs    f1, f1, f0
    bl       HSD_JObjAddTz
.L_801E1F88:
    lwz      r12, 0x98(r31)
    cmplwi   r12, 0x0
    beq      .L_801E1FA8
    mr       r3, r31
    mtctr    r12
    bctrl
    cmpwi    r3, -0x1
    beq      .L_801E1CAC
.L_801E1FA8:
    mr       r3, r31
    bl       _psListGetNext
.L_801E1FB0:
    psq_l    f31, 0xb8(r1), 0, qr0
    lfd      f31, 0xb0(r1)
    psq_l    f30, 0xa8(r1), 0, qr0
    lfd      f30, 0xa0(r1)
    psq_l    f29, 0x98(r1), 0, qr0
    lfd      f29, 0x90(r1)
    psq_l    f28, 0x88(r1), 0, qr0
    lfd      f28, 0x80(r1)
    psq_l    f27, 0x78(r1), 0, qr0
    lfd      f27, 0x70(r1)
    psq_l    f26, 0x68(r1), 0, qr0
    addi     r11, r1, 0x60
    lfd      f26, 0x60(r1)
    bl       _restgpr_26
    lwz      r0, 0xc4(r1)
    mtlr     r0
    addi     r1, r1, 0xc0
    blr
*/

/**
 * @note Address: 0x801E1FF8
 * @note Size: 0xB0
 */
// HSD_JObjAddTz

/**
 * @note Address: 0x801E20A8
 * @note Size: 0xB0
 */
// HSD_JObjAddTy

/**
 * @note Address: 0x801E2158
 * @note Size: 0xB0
 */
// HSD_JObjAddTx

/**
 * @note Address: 0x801E2208
 * @note Size: 0x70
 */
// SetupDirtyParticleJointMatrix

/**
 * @note Address: 0x801E22F0
 * @note Size: 0x64
 */
// AttachJointToParticleGenerator
