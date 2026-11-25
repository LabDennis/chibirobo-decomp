#include <global.h>
#include <dolphin/gx/gx.h>
#include <dolphin/gx/gxfifo.h>
#include <dolphin/mtx/mtx.h>
#include <float.h>
#include <math.h>
#include <sysdolphin/cobj.h>
#include <sysdolphin/jobj.h>
#include <sysdolphin/memory.h>
#include <sysdolphin/lobj.h>
#include <sysdolphin/util.h>
#include <sysdolphin/mtx.h>
#include <sysdolphin/particle_internal.h>
#include <sysdolphin/psdisp.h>
#include <sysdolphin/pslist.h>
#include <sysdolphin/fog.h>
#include <sysdolphin/tev_state.h>

// Each form primitive starts with one header word, followed by its vertices.
typedef struct HSD_PSFormPrimitiveHeader {
	u8 type;
	u8 vertexCount;
	u8 reserved[2];
} HSD_PSFormPrimitiveHeader;

typedef struct HSD_PSFormVertex {
	f32 s;
	f32 t;
} HSD_PSFormVertex;

STATIC_ASSERT(sizeof(HSD_PSFormPrimitiveHeader) == sizeof(u32));
STATIC_ASSERT(sizeof(HSD_PSFormVertex) == 2 * sizeof(u32));

/* The flip bits select one of four groups of quad texture coordinates. */
enum {
	PS_QUAD_TEXCOORD_SHIFT = 16,
	PS_QUAD_TEXCOORD_MASK = (PS_PARTICLE_FLIP_S | PS_PARTICLE_FLIP_T) >> PS_QUAD_TEXCOORD_SHIFT,
};

// billboard texcoords, two bytes each, picked by GX_INDEX8
u8 sParticleBillboardTexCoords[32] __attribute__((aligned(32)))
= { 0, 1, 0, 0, 1, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 0 };

static Mtx psdisp_view;
static Mtx psdisp_cam;
static f32 psdisp_proj[7];
static Mtx psdisp_pv;
static Mtx rvmtxRot;

u8 psFrameNum = 123;
// .sbss comes out last declaration first
s32 sParticlePointSize;
s32 sParticleLineWidth;
f32 sBillboardDiagonalRightX;
f32 sBillboardDiagonalUpX;
f32 sBillboardDiagonalRightY;
f32 sBillboardDiagonalUpY;
f32 sBillboardDiagonalRightZ;
f32 sBillboardDiagonalUpZ;
s32 sParticleBlendMode;
u32 sParticleLightingMode;
GXColor sParticleMaterialColor;
GXColor sParticleAmbientColor;
GXColor psdispTevReg0Color;
GXColor psdispTevReg1Color;
GXColor psdispTevReg2Color;
s32 sParticleCurrentPosMtx;
HSD_Fog* particleFog;

static const f64 sParticleRotationThreshold = 0.01;

static HSD_Particle* psDispSubPointTrail(HSD_Particle* pp);
static void psDispSubAPPSRTPoint(HSD_Particle* pp);
static void psDispSub(HSD_Particle* pp, u32* cmdList);
static void psDispSubMakePolygon(HSD_Particle* pp, u32* cmdList, f32 x, f32 y, f32 z, f32 prevX, f32 prevY, f32 prevZ,
                                 f32 rightX, f32 rightY, f32 rightZ, f32 upX, f32 upY, f32 upZ);
static void psDispSubAppSRT(HSD_Particle* pp, u32* cmdList);
static void setupTevReg(HSD_Particle* pp);
static void setupChanReg(HSD_Particle* pp);

static inline void psdispUseViewMatrix(void)
{
	if (sParticleCurrentPosMtx != GX_PNMTX0) {
		sParticleCurrentPosMtx = GX_PNMTX0;
		GXSetCurrentMtx(GX_PNMTX0);
	}
}

static inline f32 psdisp_sinf(f32 x)
{
	return sin(x);
}

static inline f32 psdisp_cosf(f32 x)
{
	return cos(x);
}

static inline f32 psdisp_atan2f(f32 y, f32 x)
{
	return atan2(y, x);
}

static inline f32 psdisp_tanf(f32 x)
{
	return tan(x);
}

enum { PS_COLOR_FRACTION_BITS = 16 };

// Particle scripts encode transition times in at most 15 bits.
// The remaining fraction weights the current color.
static inline s32 psColorWeight(u16 remaining, u16 duration)
{
	return ((s32)remaining << PS_COLOR_FRACTION_BITS) / duration;
}

static inline u8 psLerpColorChannel(u8 current, u8 target, s32 weight)
{
	return ((target << PS_COLOR_FRACTION_BITS) + weight * (current - target)) >> PS_COLOR_FRACTION_BITS;
}

// Trails and points can use the interpolated primary color or opaque white.
static inline void psdisp_getTrailColor(HSD_Particle* pp, GXColor* color)
{
	switch (pp->kind & (PS_PARTICLE_ENV_COLOR | PS_PARTICLE_LIGHTING)) {
	case 0:
	case PS_PARTICLE_LIGHTING:
		if (pp->primColCount != 0) {
			s32 weight = psColorWeight(pp->primColRemain, pp->primColCount);

			color->r = psLerpColorChannel(pp->primCol.r, pp->primColTarget.r, weight);
			color->g = psLerpColorChannel(pp->primCol.g, pp->primColTarget.g, weight);
			color->b = psLerpColorChannel(pp->primCol.b, pp->primColTarget.b, weight);
			color->a = psLerpColorChannel(pp->primCol.a, pp->primColTarget.a, weight);
		} else {
			*color = pp->primCol;
		}
		break;
	case PS_PARTICLE_ENV_COLOR:
	case PS_PARTICLE_ENV_COLOR | PS_PARTICLE_LIGHTING:
		color->r = color->g = color->b = color->a = 0xFF;
		break;
	}
}

static inline void psdisp_getColorPrimEnv(HSD_Particle* pp, GXColor* prim, GXColor* env)
{
	if (pp->primColCount != 0) {
		s32 weight = psColorWeight(pp->primColRemain, pp->primColCount);

		prim->r = psLerpColorChannel(pp->primCol.r, pp->primColTarget.r, weight);
		prim->g = psLerpColorChannel(pp->primCol.g, pp->primColTarget.g, weight);
		prim->b = psLerpColorChannel(pp->primCol.b, pp->primColTarget.b, weight);
		prim->a = psLerpColorChannel(pp->primCol.a, pp->primColTarget.a, weight);
	} else {
		*prim = pp->primCol;
	}
	if (pp->envColCount != 0) {
		s32 weight = psColorWeight(pp->envColRemain, pp->envColCount);

		env->r = psLerpColorChannel(pp->envCol.r, pp->envColTarget.r, weight);
		env->g = psLerpColorChannel(pp->envCol.g, pp->envColTarget.g, weight);
		env->b = psLerpColorChannel(pp->envCol.b, pp->envColTarget.b, weight);
		env->a = psLerpColorChannel(pp->envCol.a, pp->envColTarget.a, weight);
	} else {
		*env = pp->envCol;
	}
}

static inline void psdisp_getColorMatAmb(HSD_Particle* pp, GXColor* mat, GXColor* amb)
{
	if (pp->matColCount != 0) {
		s32 weight = psColorWeight(pp->matColRemain, pp->matColCount);
		u8 rgb = psLerpColorChannel(pp->matRGB, pp->matRGBTarget, weight);

		mat->r = mat->g = mat->b = rgb;
		mat->a = psLerpColorChannel(pp->matA, pp->matATarget, weight);
	} else {
		mat->r = mat->g = mat->b = pp->matRGB;
		mat->a = pp->matA;
	}
	if (pp->ambColCount != 0) {
		s32 weight = psColorWeight(pp->ambColRemain, pp->ambColCount);
		u8 rgb = psLerpColorChannel(pp->ambRGB, pp->ambRGBTarget, weight);

		amb->r = amb->g = amb->b = rgb;
		amb->a = psLerpColorChannel(pp->ambA, pp->ambATarget, weight);
	} else {
		amb->r = amb->g = amb->b = pp->ambRGB;
		amb->a = pp->ambA;
	}
}

// where a tornado particle sits, it orbits its generator
static inline void psdisp_tornadoPos(HSD_Particle* pp, f32* x, f32* y, f32* z)
{
	f32 radius;
	f32 px, py, pz;
	f32 sina, sinb, cosa, cosb;
	f32 vx0, vz0;
	HSD_Generator* gp;

	gp = pp->gen;
	if (gp == NULL) {
		*x = pp->pos.x;
		*y = pp->pos.y;
		*z = pp->pos.z;
		return;
	}

	sina = psdisp_sinf(pp->grav);
	sinb = psdisp_sinf(pp->fric);
	cosa = psdisp_cosf(pp->grav);
	cosb = psdisp_cosf(pp->fric);

	vz0 = pp->vel.z - gp->aux.tornado.vel;
	vx0 = pp->vel.x - gp->grav;

	radius = (gp->radius < 0.0F) ? -gp->radius : gp->radius;
	radius += vz0 * psdisp_tanf((gp->angle < 0.0F) ? -gp->angle : gp->angle);
	radius *= pp->vel.y;
	px = radius * psdisp_cosf(vx0);
	py = radius * psdisp_sinf(vx0);
	pz = vz0;

	*x = gp->pos.x + (px * cosb + pz * sinb);
	*y = gp->pos.y + (pz * sina * cosb + (-px * sina * sinb + py * cosa));
	*z = gp->pos.z + (pz * cosa * cosb + (-px * cosa * sinb - py * sina));
}

// same, then pushed through the particle's appsrt
static inline void psdisp_tornadoPosSRT(HSD_Particle* pp, f32* x, f32* y, f32* z)
{
	HSD_psAppSRT* srt;
	Mtx mtx;
	Vec v;

	psdisp_tornadoPos(pp, x, y, z);
	srt = pp->appsrt;
	HSD_MtxSRT(mtx, &srt->scale, &srt->rot, &srt->translate, NULL);
	v.x = *x;
	v.y = *y;
	v.z = *z;
	PSMTXMultVec(mtx, &v, &v);
	*x = v.x;
	*y = v.y;
	*z = v.z;
}

/**
 * @note Address: 0x801B58A8
 * @note Size: 0x274
 */
static void setupChanReg(HSD_Particle* pp)
{
	GXColor prim;
	GXColor amb;
	GXColor mat;
	HSD_LObj* lobj;

	if (pp->kind & PS_PARTICLE_LIGHTING) {
		psdisp_getColorMatAmb(pp, &mat, &amb);
		if (pp->kind & PS_PARTICLE_ENV_COLOR) {
			prim.r = prim.g = prim.b = 0xFF;
		} else {
			psdisp_getColorPrimEnv(pp, &prim, &mat);
			amb.r = (amb.r * prim.r) >> 8;
			amb.g = (amb.g * prim.g) >> 8;
			amb.b = (amb.b * prim.b) >> 8;
		}
		if (prim.r != sParticleMaterialColor.r || prim.g != sParticleMaterialColor.g
		    || prim.b != sParticleMaterialColor.b) {
			sParticleMaterialColor = prim;
			GXSetChanMatColor(GX_COLOR0, sParticleMaterialColor);
		}
		lobj = HSD_LObjGetActiveByID(HSD_LIGHT_AMBIENT);
		if (lobj != NULL) {
			HSD_MulColor(&amb, &lobj->color, &amb);
		} else {
			amb.r = amb.g = amb.b = 0;
		}
		if (amb.r != sParticleAmbientColor.r || amb.g != sParticleAmbientColor.g
		    || amb.b != sParticleAmbientColor.b) {
			sParticleAmbientColor = amb;
			GXSetChanAmbColor(GX_COLOR0, sParticleAmbientColor);
		}
	}
}
/**
 * @note Address: 0x801B548C
 * @note Size: 0x41C
 */
static void setupTevReg(HSD_Particle* pp)
{
	GXColor prim;
	GXColor env;
	GXColor mat;
	GXColor amb;

	psdisp_getColorPrimEnv(pp, &prim, &env);
	if ((pp->kind & PS_PARTICLE_ENV_COLOR) || (!(pp->kind & PS_PARTICLE_LIGHTING) && !(pp->kind & PS_PARTICLE_TRAIL))) {
		if (psdispTevReg0Color.r != prim.r || psdispTevReg0Color.g != prim.g || psdispTevReg0Color.b != prim.b || psdispTevReg0Color.a != prim.a) {
			psdispTevReg0Color = prim;
			GXSetTevColor(GX_TEVREG0, psdispTevReg0Color);
		}
		if (pp->kind & PS_PARTICLE_ENV_COLOR) {
			if (psdispTevReg1Color.r != env.r || psdispTevReg1Color.g != env.g || psdispTevReg1Color.b != env.b || psdispTevReg1Color.a != env.a) {
				psdispTevReg1Color = env;
				GXSetTevColor(GX_TEVREG1, psdispTevReg1Color);
			}
		} else if (psdispTevReg1Color.r != 0 || psdispTevReg1Color.g != 0 || psdispTevReg1Color.b != 0 || psdispTevReg1Color.a != 0) {
			psdispTevReg1Color.r = psdispTevReg1Color.g = psdispTevReg1Color.b = psdispTevReg1Color.a = 0;
			GXSetTevColor(GX_TEVREG1, psdispTevReg1Color);
		}
	}
	if (pp->kind & PS_PARTICLE_LIGHTING) {
		psdisp_getColorMatAmb(pp, &mat, &amb);
		if (pp->kind & PS_PARTICLE_ENV_COLOR) {
			if (psdispTevReg2Color.r != mat.r || psdispTevReg2Color.g != mat.g || psdispTevReg2Color.b != mat.b || psdispTevReg2Color.a != mat.a) {
				psdispTevReg2Color = mat;
				GXSetTevColor(GX_TEVREG2, psdispTevReg2Color);
			}
		} else {
			mat.a = (mat.a * prim.a) >> 8;
			if (psdispTevReg2Color.r != mat.r || psdispTevReg2Color.g != mat.g || psdispTevReg2Color.b != mat.b || psdispTevReg2Color.a != mat.a) {
				psdispTevReg2Color = mat;
				GXSetTevColor(GX_TEVREG2, psdispTevReg2Color);
			}
		}
	}
}

enum {
	PS_APP_SRT_UPDATE_ONCE = 1,
	PS_APP_SRT_FROZEN = 2,
};

static inline void psdispPrepareSRTMatrix(HSD_Particle* pp, Mtx localMtx)
{
	if (pp->appsrt->frameNum != psFrameNum) {
		if (pp->appsrt->status != PS_APP_SRT_FROZEN) {
			HSD_MtxSRT(pp->appsrt->mmtx, &pp->appsrt->scale, &pp->appsrt->rot, &pp->appsrt->translate, NULL);
		}
		if (pp->appsrt->status == PS_APP_SRT_UPDATE_ONCE) {
			pp->appsrt->status = PS_APP_SRT_FROZEN;
		}
	}
	pp->appsrt->frameNum = psFrameNum;

	// Subtract the translation so it can be applied after camera rotation.
	PSMTXCopy(pp->appsrt->mmtx, localMtx);
	localMtx[0][3] -= pp->appsrt->translate.x;
	localMtx[1][3] -= pp->appsrt->translate.y;
	localMtx[2][3] -= pp->appsrt->translate.z;
}

static inline void psdispTransformPosition(HSD_Particle* pp, Mtx localMtx, Vec* position)
{
	position->x = pp->pos.x;
	position->y = pp->pos.y;
	position->z = pp->pos.z;
	if (pp->appsrt->useCameraRotation != 0) {
		PSMTXMultVec(localMtx, position, position);
		PSMTXMultVec(rvmtxRot, position, position);
		position->x += pp->appsrt->translate.x;
		position->y += pp->appsrt->translate.y;
		position->z += pp->appsrt->translate.z;
	} else {
		PSMTXMultVec(pp->appsrt->mmtx, position, position);
	}
}

/**
 * @note Address: 0x801B4D98
 * @note Size: 0x6F4
 */
static void psDispSubAPPSRTPoint(HSD_Particle* pp)
{
	f32 velX;
	f32 velY;
	f32 velZ;
	f32 x;
	f32 y;
	f32 z;
	f32 prevX;
	f32 prevY;
	f32 prevZ;
	u8 w;
	Mtx mtx;
	Vec v;

	psdispUseViewMatrix();

	psdispPrepareSRTMatrix(pp, mtx);

	v.x = pp->vel.x;
	v.y = pp->vel.y;
	v.z = pp->vel.z;
	PSMTXMultVec(mtx, &v, &v);
	if (pp->appsrt->useCameraRotation != 0) {
		PSMTXMultVec(rvmtxRot, &v, &v);
	}
	velX = v.x;
	velY = v.y;
	velZ = v.z;

	psdispTransformPosition(pp, mtx, &v);
	x = v.x;
	y = v.y;
	z = v.z;

	if (pp->kind & PS_PARTICLE_TORNADO) {
		psdisp_tornadoPosSRT(pp, &prevX, &prevY, &prevZ);
	} else {
		prevX = x - velX;
		prevY = y - velY;
		prevZ = z - velZ;
	}

	w = (pp->size > 42.5) ? 255.0F : 6.0F * pp->size;
	if (pp->kind & PS_PARTICLE_TRAIL) {
		GXColor color;

		if (sParticleLineWidth != (s32)w) {
			sParticleLineWidth = w;
			GXSetLineWidth(w, 5);
		}
		psdisp_getTrailColor(pp, &color);
		if (pp->kind & PS_PARTICLE_TEXTURED) {
			GXClearVtxDesc();
			GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
			GXSetVtxDesc(GX_VA_CLR0, GX_DIRECT);
			GXSetVtxDesc(GX_VA_TEX0, GX_INDEX8);
			GXBegin(GX_LINES, GX_VTXFMT2, 2);
		} else {
			GXClearVtxDesc();
			GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
			GXSetVtxDesc(GX_VA_CLR0, GX_DIRECT);
			GXBegin(GX_LINES, GX_VTXFMT3, 2);
		}
		GXPosition3f32(prevX, prevY, prevZ);
		GXColor4u8(color.r, color.g, color.b, (f32)color.a * pp->trail);
		if (pp->kind & PS_PARTICLE_TEXTURED) {
			GXTexCoord1x8(0);
		}
		GXPosition3f32(x, y, z);
		GXColor4u8(color.r, color.g, color.b, color.a);
		if (pp->kind & PS_PARTICLE_TEXTURED) {
			GXTexCoord1x8(1);
		}
	} else {
		if (sParticlePointSize != (s32)w) {
			sParticlePointSize = w;
			GXSetPointSize(w, 5);
		}
		if (pp->kind & PS_PARTICLE_TEXTURED) {
			GXClearVtxDesc();
			GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
			GXSetVtxDesc(GX_VA_TEX0, GX_INDEX8);
			GXBegin(GX_POINTS, GX_VTXFMT0, 1);
		} else {
			GXClearVtxDesc();
			GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
			GXBegin(GX_POINTS, GX_VTXFMT1, 1);
		}
		GXPosition3f32(x, y, z);
		if (pp->kind & PS_PARTICLE_TEXTURED) {
			GXTexCoord1x8(1);
		}
	}
}

/**
 * @note Address: 0x801B4934
 * @note Size: 0x464
 */
static HSD_Particle* psDispSubPointTrail(HSD_Particle* pp)
{
	f32 dx;
	f32 dy;
	f32 dz;
	GXColor col;
	u8 w;

	psdispUseViewMatrix();

	w = (pp->size > 42.5) ? 255.0F : 6.0F * pp->size;
	if (sParticleLineWidth != (s32)w) {
		sParticleLineWidth = w;
		GXSetLineWidth(w, 5);
	}

	if (pp->kind & PS_PARTICLE_TORNADO) {
		psdisp_tornadoPos(pp, &dx, &dy, &dz);
	} else {
		dx = pp->pos.x - pp->vel.x;
		dy = pp->pos.y - pp->vel.y;
		dz = pp->pos.z - pp->vel.z;
	}

	psdisp_getTrailColor(pp, &col);

	if (pp->kind & PS_PARTICLE_TEXTURED) {
		GXClearVtxDesc();
		GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
		GXSetVtxDesc(GX_VA_CLR0, GX_DIRECT);
		GXSetVtxDesc(GX_VA_TEX0, GX_INDEX8);
		GXBegin(GX_LINES, GX_VTXFMT2, 2);
	} else {
		GXClearVtxDesc();
		GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
		GXSetVtxDesc(GX_VA_CLR0, GX_DIRECT);
		GXBegin(GX_LINES, GX_VTXFMT3, 2);
	}

	GXPosition3f32(dx, dy, dz);
	GXColor4u8(col.r, col.g, col.b, (f32)col.a * pp->trail);
	if (pp->kind & PS_PARTICLE_TEXTURED) {
		GXTexCoord1x8(0);
	}
	GXPosition3f32(pp->pos.x, pp->pos.y, pp->pos.z);
	GXColor4u8(col.r, col.g, col.b, col.a);
	if (pp->kind & PS_PARTICLE_TEXTURED) {
		GXTexCoord1x8(1);
	}
	return pp;
}

/**
 * @note Address: 0x801B3EF4
 * @note Size: 0xA40
 */
static void psDispSubAppSRT(HSD_Particle* pp, u32* cmdList)
{
	f32 rightX;
	f32 upX;
	f32 rightY;
	f32 upY;
	f32 rightZ;
	f32 upZ;
	f32 x;
	f32 y;
	f32 z;
	f32 velX;
	f32 velY;
	f32 velZ;
	f32 angle;
	Mtx mtx;
	Vec v;

	psdispPrepareSRTMatrix(pp, mtx);

	v.x = pp->vel.x;
	v.y = pp->vel.y;
	v.z = pp->vel.z;
	PSMTXMultVec(mtx, &v, &v);
	if (pp->appsrt->useCameraRotation != 0 && !(pp->kind & PS_PARTICLE_TORNADO)) {
		PSMTXMultVec(rvmtxRot, &v, &v);
	}
	velX = v.x;
	velY = v.y;
	velZ = v.z;

	psdispTransformPosition(pp, mtx, &v);
	x = v.x;
	y = v.y;
	z = v.z;

	if (cmdList != NULL) {
		Mtx scaleMtx;

		PSMTXScale(scaleMtx, pp->appsrt->scale.x, pp->appsrt->scale.y, 1.0F);
		PSMTXConcat(psdisp_cam, scaleMtx, scaleMtx);
		rightX = scaleMtx[0][0] * pp->size;
		upX    = -scaleMtx[0][1] * pp->size;
		rightY = scaleMtx[1][0] * pp->size;
		upY    = -scaleMtx[1][1] * pp->size;
		rightZ = scaleMtx[2][0] * pp->size;
		upZ    = -scaleMtx[2][1] * pp->size;
	} else {
		Vec right;
		Vec up;

		right.x = pp->appsrt->scale.x + psdisp_view[0][3];
		right.y = pp->appsrt->scale.y + psdisp_view[1][3];
		right.z = psdisp_view[2][3];
		up.x    = pp->appsrt->scale.x + psdisp_view[0][3];
		up.y    = -pp->appsrt->scale.y + psdisp_view[1][3];
		up.z    = psdisp_view[2][3];
		PSMTXMultVec(psdisp_cam, &right, &right);
		rightX = right.x * pp->size;
		rightY = right.y * pp->size;
		rightZ = right.z * pp->size;
		PSMTXMultVec(psdisp_cam, &up, &up);
		upX = up.x * pp->size;
		upY = up.y * pp->size;
		upZ = up.z * pp->size;
	}

	if ((pp->kind & PS_PARTICLE_TRAIL) || (pp->kind & PS_PARTICLE_ROTATE_WITH_MOTION)) {
		f32 sx;
		f32 sy;

		if ((f32)GX_PERSPECTIVE == psdisp_proj[0]) {
			f32 prevX;
			f32 prevY;
			f32 prevZ;
			f32 w0;
			f32 w1;
			f32 depth;

			if (pp->kind & PS_PARTICLE_TORNADO) {
				psdisp_tornadoPos(pp, &prevX, &prevY, &prevZ);
				v.x = prevX;
				v.y = prevY;
				v.z = prevZ;
				if (pp->appsrt->useCameraRotation != 0) {
					PSMTXMultVec(mtx, &v, &v);
					PSMTXMultVec(rvmtxRot, &v, &v);
					v.x += pp->appsrt->translate.x;
					v.y += pp->appsrt->translate.y;
					v.z += pp->appsrt->translate.z;
				} else {
					PSMTXMultVec(pp->appsrt->mmtx, &v, &v);
				}
				prevX = v.x;
				prevY = v.y;
				prevZ = v.z;
			} else {
				prevX = x - velX;
				prevY = y - velY;
				prevZ = z - velZ;
			}
			depth = psdisp_view[2][3] + (psdisp_view[2][2] * z + (psdisp_view[2][0] * x + psdisp_view[2][1] * y));
			if (0.0F == depth) {
				goto no_dir;
			}
			w0 = -1.0F / depth;
			w1 = psdisp_view[2][3] + (psdisp_view[2][2] * prevZ + (psdisp_view[2][0] * prevX + psdisp_view[2][1] * prevY));
			if (0.0F == w1) {
				goto no_dir;
			}
			w1 = -1.0F / w1;
			sx = w0 * (psdisp_pv[0][3] + (psdisp_pv[0][2] * z + (psdisp_pv[0][0] * x + psdisp_pv[0][1] * y)))
			   - w1 * (psdisp_pv[0][3] + (psdisp_pv[0][2] * prevZ + (psdisp_pv[0][0] * prevX + psdisp_pv[0][1] * prevY)));
			sy = w0 * (psdisp_pv[1][3] + (psdisp_pv[1][2] * z + (psdisp_pv[1][0] * x + psdisp_pv[1][1] * y)))
			   - w1 * (psdisp_pv[1][3] + (psdisp_pv[1][2] * prevZ + (psdisp_pv[1][0] * prevX + psdisp_pv[1][1] * prevY)));
		} else if (pp->kind & PS_PARTICLE_TORNADO) {
			f32 dx;
			f32 dy;
			f32 dz;

			psdisp_tornadoPos(pp, &dx, &dy, &dz);
			v.x = dx;
			v.y = dy;
			v.z = dz;
			if (pp->appsrt->useCameraRotation != 0) {
				PSMTXMultVec(mtx, &v, &v);
				PSMTXMultVec(rvmtxRot, &v, &v);
				v.x += pp->appsrt->translate.x;
				v.y += pp->appsrt->translate.y;
				v.z += pp->appsrt->translate.z;
			} else {
				PSMTXMultVec(pp->appsrt->mmtx, &v, &v);
			}
			dx = x - v.x;
			dy = y - v.y;
			dz = z - v.z;
			sx = psdisp_pv[0][2] * dz + (psdisp_pv[0][0] * dx + psdisp_pv[0][1] * dy);
			sy = psdisp_pv[1][2] * dz + (psdisp_pv[1][0] * dx + psdisp_pv[1][1] * dy);
		} else {
			sx = psdisp_pv[0][2] * velZ + (psdisp_pv[0][0] * velX + psdisp_pv[0][1] * velY);
			sy = psdisp_pv[1][2] * velZ + (psdisp_pv[1][0] * velX + psdisp_pv[1][1] * velY);
		}
		if (__fabs(sy) < MSL_FLT_MIN) {
			angle = (sx >= 0.0F) ? 1.5707964F : -1.5707964F;
		} else {
			angle = psdisp_atan2f(sx, sy);
		}
		if (pp->kind & PS_PARTICLE_ROTATE_WITH_MOTION) {
			angle += pp->rotate;
		}
	} else {
	no_dir:
		angle = pp->rotate;
	}

	if (__fabs(angle) > sParticleRotationThreshold) {
		Mtx rot;
		Vec axis;
		f32 rx = rightX;
		f32 ry = rightY;
		f32 rz = rightZ;
		f32 ux = upX;
		f32 uz = upZ;
		f32 uy = upY;

		axis.x = ry * uz - rz * uy;
		axis.y = rz * ux - rx * uz;
		axis.z = rx * uy - ry * ux;
		PSMTXRotAxisRad(rot, &axis, angle);
		rightX = rot[0][2] * rz + (rot[0][0] * rx + rot[0][1] * ry);
		rightY = rot[1][2] * rz + (rot[1][0] * rx + rot[1][1] * ry);
		rightZ = rot[2][2] * rz + (rot[2][0] * rx + rot[2][1] * ry);
		upX    = rot[0][2] * uz + (rot[0][0] * ux + rot[0][1] * uy);
		upY    = rot[1][2] * uz + (rot[1][0] * ux + rot[1][1] * uy);
		upZ    = rot[2][2] * uz + (rot[2][0] * ux + rot[2][1] * uy);
	}

	psDispSubMakePolygon(pp, cmdList, x, y, z, velX, velY, velZ, rightX, rightY, rightZ, upX, upY, upZ);
}

/**
 * @note Address: 0x801B3218
 * @note Size: 0xCDC
 */
// prevX, prevY and prevZ come in as the velocity and leave as the trail's tail
static void psDispSubMakePolygon(HSD_Particle* pp, u32* cmdList, f32 x, f32 y, f32 z, f32 prevX, f32 prevY, f32 prevZ,
                                 f32 rightX, f32 rightY, f32 rightZ, f32 upX, f32 upY, f32 upZ)
{
	// NONMATCHING: retail gives the fourth quad vertex's trail-alpha conversion its own stack temps (frame 0x1A0, ours 0x190);
	// MWCC reuses the first vertex's temps across the texcoord branches. Also dx/dy/dz get f3/f2/f4 in retail, f2/f4/f5 here.
	GXColor color;

	psdispUseViewMatrix();
	if (pp->kind & PS_PARTICLE_TRAIL) {
		if (pp->kind & PS_PARTICLE_TORNADO) {
			if (pp->appsrt != NULL) {
				psdisp_tornadoPosSRT(pp, &prevX, &prevY, &prevZ);
			} else {
				psdisp_tornadoPos(pp, &prevX, &prevY, &prevZ);
			}
		} else {
			prevX = x - prevX;
			prevY = y - prevY;
			prevZ = z - prevZ;
		}
		psdisp_getTrailColor(pp, &color);
		if (cmdList == NULL) {
			if (pp->kind & PS_PARTICLE_TEXTURED) {
				GXClearVtxDesc();
				GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
				GXSetVtxDesc(GX_VA_CLR0, GX_DIRECT);
				GXSetVtxDesc(GX_VA_TEX0, GX_INDEX8);
				GXBegin(GX_QUADS, GX_VTXFMT2, 4);
			} else {
				GXClearVtxDesc();
				GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
				GXSetVtxDesc(GX_VA_CLR0, GX_DIRECT);
				GXBegin(GX_QUADS, GX_VTXFMT3, 4);
			}
			GXPosition3f32(prevX - rightX, prevY - rightY, prevZ - rightZ);
			GXColor4u8(color.r, color.g, color.b, (f32)color.a * pp->trail);
			if (pp->kind & PS_PARTICLE_TEXTURED) {
				GXTexCoord1x8((pp->kind >> PS_QUAD_TEXCOORD_SHIFT) & PS_QUAD_TEXCOORD_MASK);
			}
			GXPosition3f32(x - upX, y - upY, z - upZ);
			GXColor4u8(color.r, color.g, color.b, color.a);
			if (pp->kind & PS_PARTICLE_TEXTURED) {
				GXTexCoord1x8(((pp->kind >> PS_QUAD_TEXCOORD_SHIFT) & PS_QUAD_TEXCOORD_MASK) + 1);
			}
			GXPosition3f32(x + rightX, y + rightY, z + rightZ);
			GXColor4u8(color.r, color.g, color.b, color.a);
			if (pp->kind & PS_PARTICLE_TEXTURED) {
				GXTexCoord1x8(((pp->kind >> PS_QUAD_TEXCOORD_SHIFT) & PS_QUAD_TEXCOORD_MASK) + 2);
			}
			GXPosition3f32(prevX + upX, prevY + upY, prevZ + upZ);
			GXColor4u8(color.r, color.g, color.b, (f32)color.a * pp->trail);
			if (pp->kind & PS_PARTICLE_TEXTURED) {
				GXTexCoord1x8(((pp->kind >> PS_QUAD_TEXCOORD_SHIFT) & PS_QUAD_TEXCOORD_MASK) + 3);
			}
		} else {
			f32 trailAlpha;
			f32 upLen;

			trailAlpha = 255.0F * (1.0F - pp->trail);
			upLen      = msl_sqrtf(upX * upX + upY * upY + upZ * upZ);

			if (!(__fabs(upLen) < MSL_FLT_MIN)) {
				f32 dx    = x - prevX;
				f32 dy    = y - prevY;
				f32 dz    = z - prevZ;
				f32 xl    = dx * dx;
				f32 yl    = dy * dy;
				f32 zl    = dz * dz;
				f32 ratio = msl_sqrtf(zl + (xl + yl)) / upLen;
				u32 primCount;

				if (ratio < 1.0F) {
					ratio = 1.0F;
				}
				upX *= ratio;
				upY *= ratio;
				upZ *= ratio;
				primCount = *cmdList++;
				for (; primCount != 0; primCount--) {
					HSD_PSFormPrimitiveHeader* header = (HSD_PSFormPrimitiveHeader*)cmdList;
					u8 prim = header->type;
					u8 count = header->vertexCount;
					s32 i;

					cmdList += sizeof(HSD_PSFormPrimitiveHeader) / sizeof(u32);
					if (pp->kind & PS_PARTICLE_TEXTURED) {
						GXClearVtxDesc();
						GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
						GXSetVtxDesc(GX_VA_CLR0, GX_DIRECT);
						GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
						GXBegin(prim, GX_VTXFMT5, count);
					} else {
						GXClearVtxDesc();
						GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
						GXSetVtxDesc(GX_VA_CLR0, GX_DIRECT);
						GXBegin(prim, GX_VTXFMT3, count);
					}
					for (i = 0; i < count; i++) {
						HSD_PSFormVertex* vertex = (HSD_PSFormVertex*)cmdList;
						f32 s = vertex->s;
						f32 t;
						f32 sx = 2.0F * (s - 0.5F);
						f32 tx;
						s32 alpha;

						if (pp->kind & PS_PARTICLE_FLIP_S) {
							s = 1.0F - s;
						}
						t = vertex->t;
						cmdList += sizeof(HSD_PSFormVertex) / sizeof(u32);
						alpha = 255.0F - t * trailAlpha;
						if (alpha < 0) {
							alpha = 0;
						}
						if (alpha > 0xFF) {
							alpha = 0xFF;
						}
						tx = 2.0F * (t - 0.5F);
						if (pp->kind & PS_PARTICLE_FLIP_T) {
							t = 1.0F - t;
						}
						GXPosition3f32(upX * tx + (rightX * sx + x), upY * tx + (rightY * sx + y), upZ * tx + (rightZ * sx + z));
						GXColor4u8(color.r, color.g, color.b, alpha);
						if (pp->kind & PS_PARTICLE_TEXTURED) {
							GXTexCoord2f32(s, t);
						}
					}
				}
			}
		}
	} else if (cmdList == NULL) {
		if (pp->kind & PS_PARTICLE_TEXTURED) {
			GXClearVtxDesc();
			GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
			GXSetVtxDesc(GX_VA_TEX0, GX_INDEX8);
			GXBegin(GX_QUADS, GX_VTXFMT0, 4);
		} else {
			GXClearVtxDesc();
			GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
			GXBegin(GX_QUADS, GX_VTXFMT1, 4);
		}
		GXPosition3f32(x - rightX, y - rightY, z - rightZ);
		if (pp->kind & PS_PARTICLE_TEXTURED) {
			GXTexCoord1x8((pp->kind >> PS_QUAD_TEXCOORD_SHIFT) & PS_QUAD_TEXCOORD_MASK);
		}
		GXPosition3f32(x - upX, y - upY, z - upZ);
		if (pp->kind & PS_PARTICLE_TEXTURED) {
			GXTexCoord1x8(((pp->kind >> PS_QUAD_TEXCOORD_SHIFT) & PS_QUAD_TEXCOORD_MASK) + 1);
		}
		GXPosition3f32(x + rightX, y + rightY, z + rightZ);
		if (pp->kind & PS_PARTICLE_TEXTURED) {
			GXTexCoord1x8(((pp->kind >> PS_QUAD_TEXCOORD_SHIFT) & PS_QUAD_TEXCOORD_MASK) + 2);
		}
		GXPosition3f32(x + upX, y + upY, z + upZ);
		if (pp->kind & PS_PARTICLE_TEXTURED) {
			GXTexCoord1x8(((pp->kind >> PS_QUAD_TEXCOORD_SHIFT) & PS_QUAD_TEXCOORD_MASK) + 3);
		}
	} else {
		u32 primCount = *cmdList++;

		for (; primCount != 0; primCount--) {
			HSD_PSFormPrimitiveHeader* header = (HSD_PSFormPrimitiveHeader*)cmdList;
			u8 prim = header->type;
			u8 count = header->vertexCount;
			s32 i;

			cmdList += sizeof(HSD_PSFormPrimitiveHeader) / sizeof(u32);
			if (pp->kind & PS_PARTICLE_TEXTURED) {
				GXClearVtxDesc();
				GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
				GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
				GXBegin(prim, GX_VTXFMT4, count);
			} else {
				GXClearVtxDesc();
				GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
				GXBegin(prim, GX_VTXFMT1, count);
			}
			for (i = 0; i < count; i++) {
				HSD_PSFormVertex* vertex = (HSD_PSFormVertex*)cmdList;
				f32 s = vertex->s;
				f32 t;
				f32 sx = 2.0F * (s - 0.5F);
				f32 tx;

				if (pp->kind & PS_PARTICLE_FLIP_S) {
					s = 1.0F - s;
				}
				t = vertex->t;
				cmdList += sizeof(HSD_PSFormVertex) / sizeof(u32);
				tx = 2.0F * (t - 0.5F);
				if (pp->kind & PS_PARTICLE_FLIP_T) {
					t = 1.0F - t;
				}
				GXPosition3f32(x + rightX * sx + upX * tx, y + rightY * sx + upY * tx, z + rightZ * sx + upZ * tx);
				if (pp->kind & PS_PARTICLE_TEXTURED) {
					GXTexCoord2f32(s, t);
				}
			}
		}
	}
}

/**
 * @note Address: 0x801B2B14
 * @note Size: 0x704
 */
static void psDispSub(HSD_Particle* pp, u32* cmdList)
{
	f32 x;
	f32 y;
	f32 z;
	f32 rightX;
	f32 upX;
	f32 rightY;
	f32 upY;
	f32 rightZ;
	f32 upZ;
	f32 angle;

	x = pp->pos.x;
	y = pp->pos.y;
	z = pp->pos.z;
	if (cmdList != NULL) {
		rightX = psdisp_cam[0][0] * pp->size;
		upX    = -psdisp_cam[0][1] * pp->size;
		rightY = psdisp_cam[1][0] * pp->size;
		upY    = -psdisp_cam[1][1] * pp->size;
		rightZ = psdisp_cam[2][0] * pp->size;
		upZ    = -psdisp_cam[2][1] * pp->size;
	} else {
		rightX = sBillboardDiagonalRightX * pp->size;
		upX    = sBillboardDiagonalUpX * pp->size;
		rightY = sBillboardDiagonalRightY * pp->size;
		upY    = sBillboardDiagonalUpY * pp->size;
		rightZ = sBillboardDiagonalRightZ * pp->size;
		upZ    = sBillboardDiagonalUpZ * pp->size;
	}

	if ((pp->kind & PS_PARTICLE_TRAIL) || (pp->kind & PS_PARTICLE_ROTATE_WITH_MOTION)) {
		f32 sx;
		f32 sy;

		if ((f32)GX_PERSPECTIVE == psdisp_proj[0]) {
			f32 prevX;
			f32 prevY;
			f32 prevZ;
			f32 w1;
			f32 w0;
			f32 depth;

			if (pp->kind & PS_PARTICLE_TORNADO) {
				psdisp_tornadoPos(pp, &prevX, &prevY, &prevZ);
			} else {
				prevX = pp->pos.x - pp->vel.x;
				prevY = pp->pos.y - pp->vel.y;
				prevZ = pp->pos.z - pp->vel.z;
			}
			depth = psdisp_view[2][3] + (psdisp_view[2][2] * pp->pos.z + (psdisp_view[2][0] * pp->pos.x + psdisp_view[2][1] * pp->pos.y));
			if (0.0F == depth) {
				goto no_dir;
			}
			w0    = -1.0F / depth;
			depth = psdisp_view[2][3] + (psdisp_view[2][2] * prevZ + (psdisp_view[2][0] * prevX + psdisp_view[2][1] * prevY));
			if (0.0F == depth) {
				goto no_dir;
			}
			w1 = -1.0F / depth;
			sx = w0 * (psdisp_pv[0][3] + (psdisp_pv[0][2] * pp->pos.z + (psdisp_pv[0][0] * pp->pos.x + psdisp_pv[0][1] * pp->pos.y)))
			   - w1 * (psdisp_pv[0][3] + (psdisp_pv[0][2] * prevZ + (psdisp_pv[0][0] * prevX + psdisp_pv[0][1] * prevY)));
			sy = w0 * (psdisp_pv[1][3] + (psdisp_pv[1][2] * pp->pos.z + (psdisp_pv[1][0] * pp->pos.x + psdisp_pv[1][1] * pp->pos.y)))
			   - w1 * (psdisp_pv[1][3] + (psdisp_pv[1][2] * prevZ + (psdisp_pv[1][0] * prevX + psdisp_pv[1][1] * prevY)));
		} else if (pp->kind & PS_PARTICLE_TORNADO) {
			f32 prevX;
			f32 prevY;
			f32 prevZ;
			f32 dx;
			f32 dy;
			f32 dz;

			psdisp_tornadoPos(pp, &prevX, &prevY, &prevZ);
			dx = pp->pos.x - prevX;
			dy = pp->pos.y - prevY;
			dz = pp->pos.z - prevZ;
			sx = psdisp_pv[0][2] * dz + (psdisp_pv[0][0] * dx + psdisp_pv[0][1] * dy);
			sy = psdisp_pv[1][2] * dz + (psdisp_pv[1][0] * dx + psdisp_pv[1][1] * dy);
		} else {
			sx = psdisp_pv[0][2] * pp->vel.z + (psdisp_pv[0][0] * pp->vel.x + psdisp_pv[0][1] * pp->vel.y);
			sy = psdisp_pv[1][2] * pp->vel.z + (psdisp_pv[1][0] * pp->vel.x + psdisp_pv[1][1] * pp->vel.y);
		}
		if (__fabs(sy) < MSL_FLT_MIN) {
			angle = (sx >= 0.0F) ? 1.5707964F : -1.5707964F;
		} else {
			angle = psdisp_atan2f(sx, sy);
		}
		if (pp->kind & PS_PARTICLE_ROTATE_WITH_MOTION) {
			angle += pp->rotate;
		}
	} else {
	no_dir:
		angle = pp->rotate;
	}

	if (__fabs(angle) > sParticleRotationThreshold) {
		Mtx rot;
		Vec axis;
		f32 rx = rightX;
		f32 ry = rightY;
		f32 rz = rightZ;
		f32 ux = upX;
		f32 uz = upZ;
		f32 uy = upY;

		axis.x = ry * uz - rz * uy;
		axis.y = rz * ux - rx * uz;
		axis.z = rx * uy - ry * ux;
		PSMTXRotAxisRad(rot, &axis, angle);
		rightX = rot[0][2] * rz + (rot[0][0] * rx + rot[0][1] * ry);
		rightY = rot[1][2] * rz + (rot[1][0] * rx + rot[1][1] * ry);
		rightZ = rot[2][2] * rz + (rot[2][0] * rx + rot[2][1] * ry);
		upX    = rot[0][2] * uz + (rot[0][0] * ux + rot[0][1] * uy);
		upY    = rot[1][2] * uz + (rot[1][0] * ux + rot[1][1] * uy);
		upZ    = rot[2][2] * uz + (rot[2][0] * ux + rot[2][1] * uy);
	}

	psDispSubMakePolygon(pp, cmdList, x, y, z, pp->vel.x, pp->vel.y, pp->vel.z, rightX, rightY, rightZ, upX, upY, upZ);
}

/**
 * @note Address: 0x801B1C9C
 * @note Size: 0xE78
 */
void _psDispParticlesSub(s32 bankMask, s32 kind)
{
	s32 i;
	HSD_Particle* opaHead;
	HSD_Particle* xluHead;
	HSD_Particle* pp;
	void* prevTex;
	u32 prevKind;
	s32 alphaCache;
	u8 alphaRefA;
	u8 alphaRefB;
	s32 first;
	u32 prevZMode;
	u32 prevMirror;
	u32 prevNear;
	GXTexObj texObj;
	GXTlutObj tlutObj;

	prevNear   = 0;
	alphaCache = 0;
	alphaRefA  = 0;
	alphaRefB  = 0xFF;
	first      = 1;
	if (kind == 0) {
		u8 n = 1;
		if (psFrameNum < 0xFF) {
			n = (u8)(psFrameNum + 1);
		}
		psFrameNum = n;
		return;
	}

	for (i = 0; i < 16; i++) {
		if (bankMask & (1 << i)) {
			particleSort(i, &opaHead, &xluHead);

			switch (kind) {
			case 1:
				pp = opaHead;
				break;
			case 2:
				pp = xluHead;
				break;
			default:
				continue;
			}

			while (pp != NULL) {
				if ((u32)kind == 1 && !(pp->kind & PS_PARTICLE_OPAQUE)) {
					break;
				}
				{
					if (!(pp->size < FLT_EPSILON)) {
						s32 blendMode;
						u32 lightMode;
						u32* cmdList;
						u8 refA;
						u8 refB;
						HSD_PSFormGroup* formGroup;
						HSD_PSTexGroup* texGroup;
						f32 scaleS;
						f32 scaleT;
						s32 fmt;
						u8** texTable;
						u32 width;
						u32 height;
						void* image;
						u32 wrapS;
						u32 wrapT;

						if (first) {
							sParticlePointSize = -1;
							prevTex = NULL;
							sParticleLineWidth = -1;
							sParticleLightingMode = -1;
							psSetupTevInvalidState();
							prevMirror = -1;
							prevKind = 0;
							prevZMode = -1;
							HSD_FogSet(NULL);

							sParticleMaterialColor.r = sParticleMaterialColor.g = sParticleMaterialColor.b = 0xFF;
							sParticleAmbientColor.r = sParticleAmbientColor.g = sParticleAmbientColor.b = 0xFF;
							sParticleMaterialColor.a = sParticleAmbientColor.a = 0xFF;
							GXSetChanMatColor(GX_COLOR0A0, sParticleMaterialColor);
							GXSetChanAmbColor(GX_COLOR0A0, sParticleAmbientColor);

							psSetupTevInvalidState();
							psSetupTevCommon();

							psdispTevReg0Color.r = 0xFF;
							psdispTevReg0Color.g = 0xFF;
							psdispTevReg0Color.b = 0xFF;
							psdispTevReg0Color.a = 0xFF;
							psdispTevReg1Color.r = 0;
							psdispTevReg1Color.g = 0;
							psdispTevReg1Color.b = 0;
							psdispTevReg1Color.a = 0;
							psdispTevReg2Color.r = 0xFF;
							psdispTevReg2Color.g = 0xFF;
							psdispTevReg2Color.b = 0xFF;
							psdispTevReg2Color.a = 0xFF;
							GXSetTevColor(GX_TEVREG0, psdispTevReg0Color);
							GXSetTevColor(GX_TEVREG1, psdispTevReg1Color);
							GXSetTevColor(GX_TEVREG2, psdispTevReg2Color);

							sParticleBlendMode = -1;
							GXSetZCompLoc(GX_DISABLE);

							{
								HSD_CObj* cobj = HSD_CObjGetCurrent();
								HSD_CObjGetViewingMtx(cobj, psdisp_view);
							}
							PSMTXInverse(psdisp_view, psdisp_cam);
							GXGetProjectionv(psdisp_proj);

							if ((f32)GX_PERSPECTIVE == psdisp_proj[0]) {
								f32* p          = psdisp_proj;
								psdisp_pv[0][0] = p[1] * psdisp_view[0][0] + p[2] * psdisp_view[2][0];
								psdisp_pv[0][1] = p[1] * psdisp_view[0][1] + p[2] * psdisp_view[2][1];
								psdisp_pv[0][2] = p[1] * psdisp_view[0][2] + p[2] * psdisp_view[2][2];
								psdisp_pv[0][3] = p[1] * psdisp_view[0][3] + p[2] * psdisp_view[2][3];
								psdisp_pv[1][0] = p[3] * psdisp_view[1][0] + p[4] * psdisp_view[2][0];
								psdisp_pv[1][1] = p[3] * psdisp_view[1][1] + p[4] * psdisp_view[2][1];
								psdisp_pv[1][2] = p[3] * psdisp_view[1][2] + p[4] * psdisp_view[2][2];
								psdisp_pv[1][3] = p[3] * psdisp_view[1][3] + p[4] * psdisp_view[2][3];
							} else {
								f32* p          = psdisp_proj;
								psdisp_pv[0][0] = p[1] * psdisp_view[0][0] + p[2];
								psdisp_pv[0][1] = p[1] * psdisp_view[0][1] + p[2];
								psdisp_pv[0][2] = p[1] * psdisp_view[0][2] + p[2];
								psdisp_pv[0][3] = p[1] * psdisp_view[0][3] + p[2];
								psdisp_pv[1][0] = p[3] * psdisp_view[1][0] + p[4];
								psdisp_pv[1][1] = p[3] * psdisp_view[1][1] + p[4];
								psdisp_pv[1][2] = p[3] * psdisp_view[1][2] + p[4];
								psdisp_pv[1][3] = p[3] * psdisp_view[1][3] + p[4];
							}
							sBillboardDiagonalRightX = psdisp_cam[0][0] + psdisp_cam[0][1];
							sBillboardDiagonalUpX = psdisp_cam[0][0] - psdisp_cam[0][1];
							sBillboardDiagonalRightY = psdisp_cam[1][0] + psdisp_cam[1][1];
							sBillboardDiagonalUpY = psdisp_cam[1][0] - psdisp_cam[1][1];
							sBillboardDiagonalRightZ = psdisp_cam[2][0] + psdisp_cam[2][1];
							sBillboardDiagonalUpZ = psdisp_cam[2][0] - psdisp_cam[2][1];

							GXLoadPosMtxImm(psdisp_view, GX_PNMTX0);
							{
								Mtx identity = {
									{ 1.0F, 0.0F, 0.0F, 0.0F },
									{ 0.0F, 1.0F, 0.0F, 0.0F },
									{ 0.0F, 0.0F, 1.0F, 0.0F },
								};
								GXLoadPosMtxImm(identity, GX_PNMTX1);
							}

							sParticleCurrentPosMtx = GX_PNMTX1;
							psdispUseViewMatrix();

							GXEnableTexOffsets(GX_TEXCOORD0, GX_ENABLE, GX_ENABLE);
							HSD_MtxGetRotationMtx(psdisp_cam, rvmtxRot, 'z', 'x');
							GXSetCullMode(GX_CULL_BACK);
							GXSetArray(GX_VA_TEX0, sParticleBillboardTexCoords, 2);

							GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
							GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_U8, 0);
							GXSetVtxAttrFmt(GX_VTXFMT1, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
							GXSetVtxAttrFmt(GX_VTXFMT2, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
							GXSetVtxAttrFmt(GX_VTXFMT2, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
							GXSetVtxAttrFmt(GX_VTXFMT2, GX_VA_TEX0, GX_TEX_ST, GX_U8, 0);
							GXSetVtxAttrFmt(GX_VTXFMT3, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
							GXSetVtxAttrFmt(GX_VTXFMT3, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
							GXSetVtxAttrFmt(GX_VTXFMT4, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
							GXSetVtxAttrFmt(GX_VTXFMT4, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
							GXSetVtxAttrFmt(GX_VTXFMT5, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
							GXSetVtxAttrFmt(GX_VTXFMT5, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
							GXSetVtxAttrFmt(GX_VTXFMT5, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);

							first = 0;
						}

						blendMode = (pp->kind >> PS_PARTICLE_BLEND_SHIFT) & PS_PARTICLE_BLEND_MODE_MASK;
						if (sParticleBlendMode != blendMode) {
							sParticleBlendMode = blendMode;
							switch (blendMode) {
							case PS_PARTICLE_BLEND_ALPHA:
								GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
								break;
							case PS_PARTICLE_BLEND_ADD:
								GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_ONE, GX_LO_CLEAR);
								break;
							case PS_PARTICLE_BLEND_SUBTRACT:
								GXSetBlendMode(GX_BM_SUBTRACT, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
								break;
							case PS_PARTICLE_BLEND_MULTIPLY:
								GXSetBlendMode(GX_BM_BLEND, GX_BL_DSTCLR, GX_BL_ZERO, GX_LO_CLEAR);
								break;
							default:
								OSReport("Particle:setBlendMode:Unknown mode\n");
								break;
							}
						}

						if (pp->aCmpCount != 0) {
							s32 t = (pp->aCmpRemain << 16) / pp->aCmpCount;
							refA  = ((pp->aCmpParam1Target << 16) + t * (pp->aCmpParam1 - pp->aCmpParam1Target)) >> 16;
							refB  = ((pp->aCmpParam2Target << 16) + t * (pp->aCmpParam2 - pp->aCmpParam2Target)) >> 16;
						} else {
							refA = pp->aCmpParam1;
							refB = pp->aCmpParam2;
						}
						if (alphaCache != pp->aCmpMode || alphaRefA != refA || alphaRefB != refB) {
							alphaCache = pp->aCmpMode;
							alphaRefA  = refA;
							alphaRefB  = refB;
							GXSetAlphaCompare((alphaCache >> 3) & 7, alphaRefA, (alphaCache >> 6) & 3, alphaCache & 7, alphaRefB);
						}

						psSetupTev(pp);

						lightMode = pp->kind & (PS_PARTICLE_TRAIL | PS_PARTICLE_LIGHTING);
						if (lightMode != sParticleLightingMode) {
							sParticleLightingMode = lightMode;
							GXSetNumChans(1);
							switch ((s32)sParticleLightingMode) {
							case PS_PARTICLE_TRAIL:
								GXSetChanCtrl(GX_COLOR0A0, GX_DISABLE, GX_SRC_VTX, GX_SRC_VTX, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE);
								break;
							case PS_PARTICLE_LIGHTING:
								GXSetChanCtrl(GX_COLOR0, GX_ENABLE, GX_SRC_REG, GX_SRC_REG, HSD_LObjGetLightMaskDiffuse(), GX_DF_NONE,
								              HSD_LObjGetLightMaskAttnFunc() ? GX_AF_SPOT : GX_AF_NONE);
								GXSetChanCtrl(GX_ALPHA0, GX_DISABLE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE);
								break;
							case PS_PARTICLE_TRAIL | PS_PARTICLE_LIGHTING:
								GXSetChanCtrl(GX_COLOR0, GX_ENABLE, GX_SRC_REG, GX_SRC_REG, HSD_LObjGetLightMaskDiffuse(), GX_DF_NONE,
								              HSD_LObjGetLightMaskAttnFunc() ? GX_AF_SPOT : GX_AF_NONE);
								GXSetChanCtrl(GX_ALPHA0, GX_DISABLE, GX_SRC_VTX, GX_SRC_VTX, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE);
								break;
							default:
								GXSetChanCtrl(GX_COLOR0A0, GX_DISABLE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE);
								break;
							}
							GXSetChanCtrl(GX_COLOR1A1, GX_DISABLE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE);
						}

						setupChanReg(pp);
						setupTevReg(pp);

						if ((pp->kind & (PS_PARTICLE_NO_Z_COMPARE | PS_PARTICLE_OPAQUE)) != prevZMode) {
							prevZMode = pp->kind & (PS_PARTICLE_NO_Z_COMPARE | PS_PARTICLE_OPAQUE);
							if (prevZMode & PS_PARTICLE_OPAQUE) {
								GXSetZMode((pp->kind & PS_PARTICLE_NO_Z_COMPARE) == 0, GX_LEQUAL, GX_ENABLE);
							} else {
								GXSetZMode((pp->kind & PS_PARTICLE_NO_Z_COMPARE) == 0, GX_LEQUAL, GX_DISABLE);
							}
						}

						if ((pp->kind ^ prevKind) & PS_PARTICLE_FOG) {
							if (pp->kind & PS_PARTICLE_FOG) {
								HSD_FogSet(particleFog);
							} else {
								HSD_FogSet(NULL);
							}
						}

						if (psFormGroupArray[pp->bank] != NULL && (formGroup = psFormGroupArray[pp->bank][pp->texGroup]) != NULL) {
							cmdList = formGroup->formTable[pp->poseNum];
						} else {
							cmdList = NULL;
						}

						if (pp->kind & PS_PARTICLE_TEXTURED) {
							if (pp->kind & PS_PARTICLE_MIRROR_S) {
								scaleS = 2.0F;
								wrapS  = GX_MIRROR;
							} else {
								scaleS = 1.0F;
								wrapS  = GX_CLAMP;
							}
							if (pp->kind & PS_PARTICLE_MIRROR_T) {
								scaleT = 2.0F;
								wrapT  = GX_MIRROR;
							} else {
								scaleT = 1.0F;
								wrapT  = GX_CLAMP;
							}
							if ((pp->kind & PS_PARTICLE_MIRROR_MASK) != prevMirror) {
								Mtx texMtx;

								prevMirror = pp->kind & PS_PARTICLE_MIRROR_MASK;
								prevTex    = NULL;
								PSMTXScale(texMtx, scaleS, scaleT, 1.0F);
								if (pp->kind & PS_PARTICLE_MIRROR_T) {
									texMtx[1][3] = 1.0F;
								}
								GXLoadTexMtxImm(texMtx, GX_TEXMTX0, GX_MTX2x4);
								GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_TEXMTX0, GX_DISABLE, GX_PTIDENTITY);
							}
							texGroup = psTexGroupArray[pp->bank][pp->texGroup];
							if (texGroup != NULL) {
								fmt      = texGroup->fmt;
								texTable = texGroup->texTable;
								width    = texGroup->width;
								height   = texGroup->height;
							} else {
								fmt      = 0;
								height   = 0;
								width    = 0;
								texTable = NULL;
							}
							if (texTable != NULL) {
								image = texTable[pp->poseNum];
							} else {
								image = NULL;
							}
							if ((fmt == GX_TF_C4 || fmt == GX_TF_C8) && texTable != NULL) {
								// The palette pointers follow the image pointers in the bank.
								u8** palettes = &texTable[texGroup->num];
								void* lut;

								if (pp->palNum != PS_PARTICLE_PALETTE_DEFAULT) {
									lut = palettes[pp->palNum];
								} else if (!(pp->kind & PS_PARTICLE_SHARED_PALETTE)) {
									lut = palettes[pp->poseNum];
								} else {
									lut = palettes[0];
								}
								if (lut != NULL) {
									GXInitTlutObj(&tlutObj, lut, texGroup->tlutfmt, fmt == GX_TF_C4 ? 16 : 256);
									GXLoadTlut(&tlutObj, GX_TLUT0);
								}
								prevTex = NULL;
							}
							if (prevTex != image && image != NULL) {
								prevTex = image;
								switch (fmt) {
								case GX_TF_C4:
								case GX_TF_C8:
									GXInitTexObjCI(&texObj, image, width, height, fmt, wrapS, wrapT, GX_DISABLE, GX_TLUT0);
									break;
								case GX_TF_I4:
								case GX_TF_I8:
								case GX_TF_IA4:
								case GX_TF_IA8:
								case GX_TF_RGB565:
								case GX_TF_RGB5A3:
								case GX_TF_RGBA8:
								case GX_TF_CMPR:
									GXInitTexObj(&texObj, image, width, height, fmt, wrapS, wrapT, GX_DISABLE);
									break;
								default:
									assert_line(2027, 0);
									break;
								}
								prevNear = pp->kind & PS_PARTICLE_NEAREST;
								GXInitTexObjLOD(&texObj, (prevNear != 0) ? GX_NEAR : GX_LINEAR, ((pp->kind >> 9) & 1) ? GX_NEAR : GX_LINEAR,
								                0.0F, 0.0F, 0.0F, GX_DISABLE, GX_DISABLE, GX_ANISO_1);
								GXLoadTexObj(&texObj, GX_TEXMAP0);
							}
							if (prevNear != (pp->kind & PS_PARTICLE_NEAREST)) {
								prevNear = pp->kind & PS_PARTICLE_NEAREST;
								GXInitTexObjLOD(&texObj, (prevNear != 0) ? GX_NEAR : GX_LINEAR,
								                (pp->kind & PS_PARTICLE_NEAREST) != 0 ? GX_NEAR : GX_LINEAR, 0.0F, 0.0F, 0.0F, GX_DISABLE, GX_DISABLE, GX_ANISO_1);
								GXLoadTexObj(&texObj, GX_TEXMAP0);
							}
						}

						if (pp->kind & PS_PARTICLE_POINT) {
							if (pp->appsrt != NULL) {
								psDispSubAPPSRTPoint(pp);
							} else if (pp->kind & PS_PARTICLE_TRAIL) {
								pp = psDispSubPointTrail(pp);
							} else {
								u8 w;

								psdispUseViewMatrix();
								w = (pp->size > 42.5) ? 255.0F : 6.0F * pp->size;
								if (sParticlePointSize != (s32)w) {
									sParticlePointSize = w;
									GXSetPointSize(w, 5);
								}
								if (pp->kind & PS_PARTICLE_TEXTURED) {
									GXClearVtxDesc();
									GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
									GXSetVtxDesc(GX_VA_TEX0, GX_INDEX8);
									GXBegin(GX_POINTS, GX_VTXFMT0, 1);
								} else {
									GXClearVtxDesc();
									GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
									GXBegin(GX_POINTS, GX_VTXFMT1, 1);
								}
								GXPosition3f32(pp->pos.x, pp->pos.y, pp->pos.z);
								if (pp->kind & PS_PARTICLE_TEXTURED) {
									GXTexCoord1x8(1);
								}
							}
						} else if (pp->appsrt != NULL) {
							psDispSubAppSRT(pp, cmdList);
						} else {
							psDispSub(pp, cmdList);
						}
					}
				}
				prevKind = pp->kind;
				pp       = pp->next;
			}
		}
	}

	if (!first) {
		HSD_StateInvalidate(-1);
	}
}
