#include <global.h>

#include <float.h>
#include <math.h>
#include <PowerPC_EABI_Support/MSL_C/PPC_EABI/math_ppc.h>

#include <dolphin/gx/gxtypes.h>
#include <dolphin/mtx/mtxtypes.h>
#include <dolphin/os.h>
#include <sysdolphin/forward.h>
#include <sysdolphin/jobj.h>
#include <sysdolphin/mtx.h>
#include <sysdolphin/object.h>
#include <sysdolphin/particle_internal.h>
#include <sysdolphin/psinterpret.h>
#include <sysdolphin/pslist.h>
#include <sysdolphin/random.h>

void SyncParticleGeneratorFromJoint(HSD_Generator* gen);
s32 applyForceJObj(HSD_Particle* pp, HSD_JObj* jobj, f32 force, f32 range);

// the command stream floats get copied through here a byte at a time
static f32 BYTEF32;

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

/**
 * @note Address: 0x801E1FF8
 * @note Size: 0xB0
 */
static inline void HSD_JObjAddTz(HSD_JObj* jobj, f32 delta)
{
	assert_line_jobjh(0x46b, jobj);
	jobj->translate.z += delta;
	if (!(jobj->flags & MTX_INDEP_SRT)) {
		HSD_JObjSetMtxDirty(jobj);
	}
}

/**
 * @note Address: 0x801E20A8
 * @note Size: 0xB0
 */
static inline void HSD_JObjAddTy(HSD_JObj* jobj, f32 delta)
{
	assert_line_jobjh(0x460, jobj);
	jobj->translate.y += delta;
	if (!(jobj->flags & MTX_INDEP_SRT)) {
		HSD_JObjSetMtxDirty(jobj);
	}
}

/**
 * @note Address: 0x801E2158
 * @note Size: 0xB0
 */
static inline void HSD_JObjAddTx(HSD_JObj* jobj, f32 delta)
{
	assert_line_jobjh(0x455, jobj);
	jobj->translate.x += delta;
	if (!(jobj->flags & MTX_INDEP_SRT)) {
		HSD_JObjSetMtxDirty(jobj);
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
static inline void HSD_MTXSRT(Mtx m, Vec* scale, Vec* rot, Vec* trans, Vec* parent_scale)
{
	HSD_MtxSRT(m, scale, rot, trans, parent_scale);
}

/**
 * @note Address: 0x801E22F0
 * @note Size: 0x64
 */
static inline void AttachJointToParticleGenerator(HSD_Generator* gen, HSD_JObj* jobj)
{
	if (gen != NULL) {
		gen->jobj = jobj;
		if (jobj != NULL) {
			ref_INC(jobj);
		}
	}
}

/**
 * @note Address: 0x801E2354
 * @note Size: 0x8
 */
static inline HSD_Particle* _psListGetNext(HSD_Particle* pp)
{
	return pp->next;
}

/**
 * @note Address: 0x801DE4F0
 * @note Size: 0x170
 */
void SyncParticleGeneratorFromJoint(HSD_Generator* gen)
{
	HSD_JObj* jobj;

	if (gen == NULL) {
		return;
	}
	if (gen->kind & PS_PARTICLE_PAUSED) {
		return;
	}
	if (!(gen->type & PS_GEN_USE_JOINT)) {
		return;
	}

	jobj = gen->jobj;
	if (jobj == NULL) {
		return;
	}

	HSD_JObjSetupMatrix(jobj);

	if (gen->type & PS_GEN_COPY_JOINT_POSITION) {
		gen->pos.x = gen->jobj->mtx[0][3];
		gen->pos.y = gen->jobj->mtx[1][3];
		gen->pos.z = gen->jobj->mtx[2][3];
	}

	if ((gen->type & PS_GEN_SRT_JOINT_POSITION) && gen->appsrt != NULL && gen->appsrt->gp == gen) {
		gen->appsrt->translate.x = gen->jobj->mtx[0][3];
		gen->appsrt->translate.y = gen->jobj->mtx[1][3];
		gen->appsrt->translate.z = gen->jobj->mtx[2][3];
	}

	if ((gen->type & PS_GEN_SRT_JOINT_SCALE) && gen->appsrt != NULL && gen->appsrt->gp == gen) {
		HSD_MtxGetScale(gen->jobj->mtx, &gen->appsrt->scale);
	}

	if (gen->type & PS_GEN_SRT_JOINT_ROTATION) {
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

	if (__fabsf(dz) < MSL_FLT_MIN) {
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

	if (__fabsf(dy) < MSL_FLT_MIN) {
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

	speed = msl_sqrtf(pp->vel.x * pp->vel.x + pp->vel.y * pp->vel.y + pp->vel.z * pp->vel.z);

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

	if (__fabsf(vz) < MSL_FLT_MIN) {
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

	if (__fabsf(ny) < MSL_FLT_MIN) {
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

	speed = msl_sqrtf(vx * vx + vy * vy + vz * vz);

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
 * @note Address: 0x801DECDC
 * @note Size: 0x2A8
 */
void setVelToJObj(HSD_Particle* pp, HSD_JObj* jobj)
{
	f32 vmag;
	f32 dx, dy, dz, distsq, dist;

	if (jobj == NULL) {
		return;
	}

	HSD_JObjSetupMatrix(jobj);

	dx     = jobj->mtx[0][3] - pp->pos.x;
	dy     = jobj->mtx[1][3] - pp->pos.y;
	dz     = jobj->mtx[2][3] - pp->pos.z;
	vmag   = msl_sqrtf(pp->vel.x * pp->vel.x + pp->vel.y * pp->vel.y + pp->vel.z * pp->vel.z);
	distsq = dx * dx + dy * dy + dz * dz;
	if (distsq == 0.0F) {
		return;
	}
	dist = msl_sqrtf(distsq);
	{
		f32 scale = vmag / dist;
		pp->vel.x = dx * scale;
		pp->vel.y = dy * scale;
		pp->vel.z = dz * scale;
	}
}

/**
 * @note Address: 0x801DEF84
 * @note Size: 0x144
 */
s32 applyForceJObj(HSD_Particle* pp, HSD_JObj* jobj, f32 force, f32 range)
{
	f32 dx, dy, dz;
	f32 distSq;

	if (jobj == NULL || range < 0.0F) {
		return 0;
	}

	HSD_JObjSetupMatrix(jobj);

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

	if (pp->kind & PS_PARTICLE_PAUSED) {
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
					tg          = psTexGroupArray[pp->bank][pp->texGroup];
					if (tg != NULL && tg->texTable != NULL && tg->texTable[pp->poseNum] != NULL) {
						pp->kind |= PS_PARTICLE_TEXTURED;
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
					pp->kind &= ~PS_PARTICLE_TEXTURED;
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
						if (pp->gen != NULL && (pp->gen->type & PS_GEN_SRT_JOINT_ROTATION)) {
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
					if (ptclref[bank] != NULL) {
						idx = ptclref[bank][idx];
					}

					child = psGenerateParticleID0(pp, pp->linkNo, bank, idx, 0);
					if (child != NULL) {
						child->idnum = pp->idnum;
						child->gen   = pp->gen;
						if (pp->gen != NULL) {
							pp->gen->numChild++;
						}
						if (pp->gen != NULL && (pp->gen->type & PS_GEN_SRT_JOINT_ROTATION)) {
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
							if (pp->gen != NULL && (pp->gen->type & PS_GEN_SRT_JOINT_ROTATION)) {
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
							if (pp->gen != NULL && (pp->gen->type & PS_GEN_SRT_JOINT_ROTATION)) {
								AttachGeneratorAppSRT(gchild, pp->appsrt);
							} else {
								psAttachGeneratorAppSRT(gchild, pp->appsrt);
							}
						}
						gchild->kind &= ~PS_PARTICLE_SORT_ORDER_MASK;
						gchild->kind |= (flags & (PS_PARTICLE_SORT_ORDER_COUNT - 1)) << PS_PARTICLE_SORT_ORDER_SHIFT;
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
					if (ptclref[pp->bank] != NULL) {
						idx = ptclref[pp->bank][idx];
					}
					gchild = psCreateGeneratorID(pp->linkNo, pp->bank, idx);
					if (gchild != NULL) {
						gchild->idnum = pp->idnum;
						if (pp->gen != NULL) {
							AttachJointToParticleGenerator(gchild, pp->gen->jobj);
						}
						if (pp->appsrt != NULL) {
							if (pp->gen != NULL && (pp->gen->type & PS_GEN_SRT_JOINT_ROTATION)) {
								AttachGeneratorAppSRT(gchild, pp->appsrt);
							} else {
								psAttachGeneratorAppSRT(gchild, pp->appsrt);
							}
						}
						gchild->kind &= ~PS_PARTICLE_SORT_ORDER_MASK;
						gchild->kind |= (flags & (PS_PARTICLE_SORT_ORDER_COUNT - 1)) << PS_PARTICLE_SORT_ORDER_SHIFT;
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
						s32 done = pp->gen->type & PS_GEN_SRT_JOINT_ROTATION;
						pp->gen->type |= PS_GEN_SRT_JOINT_ROTATION;
						pp->gen->appsrt->useCameraRotation = 0;
						if (!done) {
							SyncParticleGeneratorFromJoint(pp->gen);
						}
					}
					break;
				}

				case 0xF6: {
					if (pp->gen != NULL && pp->gen->appsrt != NULL) {
						s32 done = pp->gen->type & PS_GEN_SRT_JOINT_SCALE;
						pp->gen->type |= PS_GEN_SRT_JOINT_SCALE;
						if (!done) {
							SyncParticleGeneratorFromJoint(pp->gen);
						}
					}
					break;
				}

				case 0xF7:
					pp->kind |= PS_PARTICLE_NO_Z_COMPARE;
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
					if (ptclref[bank] != NULL) {
						idx = ptclref[bank][idx];
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
							if (pp->gen != NULL && (pp->gen->type & PS_GEN_SRT_JOINT_ROTATION)) {
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
					pp->kind |= PS_PARTICLE_ENV_COLOR;
					break;

				case 0xAE:
					pp->kind &= ~PS_PARTICLE_MIRROR_MASK;
					break;

				case 0xAF:
					pp->kind &= ~PS_PARTICLE_MIRROR_T;
					pp->kind |= PS_PARTICLE_MIRROR_S;
					break;

				case 0xB0:
					pp->kind &= ~PS_PARTICLE_MIRROR_S;
					pp->kind |= PS_PARTICLE_MIRROR_T;
					break;

				case 0xB1:
					pp->kind |= PS_PARTICLE_MIRROR_MASK;
					break;

				case 0xB2: {
					f32 x, y, z;
					f32 nx, ny, nz;

					if (pp->appsrt == NULL) {
						break;
					}
					if (pp->appsrt->useCameraRotation != 0) {
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
					pp->kind |= PS_PARTICLE_NEAREST;
					break;

				case 0xB5:
					pp->kind &= ~PS_PARTICLE_NEAREST;
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
							if (pp->gen != NULL && (pp->gen->type & PS_GEN_SRT_JOINT_ROTATION)) {
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
					if (ptclref[bank] != NULL) {
						idx = ptclref[bank][idx];
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
							if (pp->gen != NULL && (pp->gen->type & PS_GEN_SRT_JOINT_ROTATION)) {
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
					tg          = psTexGroupArray[pp->bank][pp->texGroup];
					if (tg != NULL && tg->texTable != NULL && tg->texTable[pp->poseNum] != NULL) {
						pp->kind |= PS_PARTICLE_TEXTURED;
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
						pp->callback = psParticleCallbackTable[idx - 1];
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
					pp->kind |= PS_PARTICLE_OPAQUE;
					break;

				case 0xE3:
					pp->palNum = *pc++;
					break;

				case 0xE4: {
					u8 mode = *pc++ & 3;
					switch (mode) {
					case 0:
						pp->kind &= ~PS_PARTICLE_FLIP_S;
						break;
					case 1:
						pp->kind |= PS_PARTICLE_FLIP_S;
						break;
					case 2:
						pp->kind ^= PS_PARTICLE_FLIP_S;
						break;
					case 3:
						if (HSD_Randf() < 0.5F) {
							pp->kind &= ~PS_PARTICLE_FLIP_S;
						} else {
							pp->kind |= PS_PARTICLE_FLIP_S;
						}
						break;
					}
					break;
				}

				case 0xE5: {
					u8 mode = *pc++ & 3;
					switch (mode) {
					case 0:
						pp->kind &= ~PS_PARTICLE_FLIP_T;
						break;
					case 1:
						pp->kind |= PS_PARTICLE_FLIP_T;
						break;
					case 2:
						pp->kind ^= PS_PARTICLE_FLIP_T;
						break;
					case 3:
						if (HSD_Randf() < 0.5F) {
							pp->kind &= ~PS_PARTICLE_FLIP_T;
						} else {
							pp->kind |= PS_PARTICLE_FLIP_T;
						}
						break;
					}
					break;
				}

				case 0xE6:
					pp->kind |= PS_PARTICLE_ROTATE_WITH_MOTION;
					break;

				case 0xE7:
					pp->kind &= ~PS_PARTICLE_ROTATE_WITH_MOTION;
					break;

				case 0xE8: {
					pc = getFloat(pc, &v);
					if (v < 0.0F) {
						pp->kind &= ~PS_PARTICLE_TRAIL;
					} else {
						pp->kind |= PS_PARTICLE_TRAIL;
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

	if (pp->kind & PS_PARTICLE_TORNADO) {
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
			HSD_JObjSetupMatrix(jobj);
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
