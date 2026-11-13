#include <global.h>
#include <dolphin/gx/gxtypes.h>
#include <dolphin/mtx/mtxtypes.h>
#include <sysdolphin/forward.h>
#include <sysdolphin/pslist.h>
#include <sysdolphin/psdisp.h>
#include <sysdolphin/jobj.h>
#include <sysdolphin/memory.h>
#include <sysdolphin/lobj.h>
#include <sysdolphin/util.h>
#include <sysdolphin/mtx.h>
#include <sysdolphin/fog.h>

#define GX_TF_I4   0
#define GX_TF_CI4  8
#define GX_TF_CI8  9
#define GX_TF_CMPR 14
#define GX_MIRROR  2
#define GX_REPEAT  1
typedef enum _GXAttnFn {
	GX_AF_SPEC,
	GX_AF_SPOT,
	GX_AF_NONE,
} GXAttnFn;
#define GX_DF_NONE        0
#define GX_SRC_REG        0
#define GX_SRC_VTX        1
#define GX_LIGHT_NULL     0
#define GX_CULL_BACK      2
#define GX_TEX_ST         1
#define GX_POS_XYZ        1
#define GX_CLR_RGBA       1
#define GX_U8             0
#define GX_F32            4
#define GX_RGBA8          5
#define GX_PNMTX0         0
#define GX_PNMTX1         3
#define GX_TEVREG0        1
#define GX_TEVREG1        2
#define GX_TEVREG2        3
#define GX_BM_BLEND       1
#define GX_BM_SUBTRACT    3
#define GX_BL_SRCALPHA    4
#define GX_BL_INVSRCALPHA 5
#define GX_BL_ONE         1
#define GX_BL_ZERO        0
#define GX_BL_DSTCLR      2
#define GX_LO_CLEAR       0
#define GX_CLAMP          0
#define GX_COLOR0         0
#define GX_ALPHA0         2
#define GX_COLOR0A0       4
#define GX_COLOR1A1       5
#define GX_ENABLE         1
#define GX_DISABLE        0
#define GX_LEQUAL         3
#define GX_TEXCOORD0      0
#define GX_TG_MTX2x4      1
#define GX_TG_TEX0        4
#define GX_POINTS         0xB8

typedef struct {
	u8 pad[0x20];
} GXTexObj;

#define GX_DIRECT  1
#define GX_INDEX8  2
#define GX_LINES   0xA8
#define GX_QUADS   0x80
#define GX_VTXFMT0 0
#define GX_VTXFMT1 1
#define GX_VTXFMT2 2
#define GX_VTXFMT3 3
#define GX_VTXFMT4 4
#define GX_VTXFMT5 5
#define GX_VA_TEX0 13

#define GX_FIFO_U8  (*(volatile u8*)0xCC008000)
#define GX_FIFO_F32 (*(volatile f32*)0xCC008000)

typedef struct {
	u8 kind;
	u8 count;
} TrailStripHdr;

extern void psSetupTevCommon(void);
extern void psSetupTevInvalidState(void);
extern void psSetupTev(HSD_Particle* pp);

extern void HSD_StateInvalidate(s32 mask);
extern void OSReport(const char* fmt, ...);
extern HSD_CObj* HSD_CObjGetCurrent(void);
extern void HSD_CObjGetViewingMtx(HSD_CObj* cobj, Mtx out);
extern u32 PSMTXInverse(Mtx src, Mtx inv);
extern void GXGetProjectionv(f32* ptr);
extern void GXEnableTexOffsets(u8 coord, u8 line_enable, u8 point_enable);
extern void GXSetCullMode(u32 mode);
extern void GXSetArray(u32 attr, void* data, u8 stride);
extern void GXSetVtxAttrFmt(u32 vtxfmt, u32 attr, u32 cnt, u32 type, u8 frac);
extern void GXSetBlendMode(u32 type, u32 src, u32 dst, u32 op);
extern void GXSetChanCtrl(u32 chan, u8 enable, u32 amb_src, u32 mat_src, u32 light_mask, u32 diff_fn, GXAttnFn attn_fn);
extern void GXSetZMode(u32 compare_enable, u32 func, u32 update_enable);
extern void GXInitTlutObj(void* tlutObj, void* lut, u32 fmt, u16 nEntries);
extern void GXLoadTlut(void* tlutObj, u32 tlutName);
extern void GXInitTexObjCI(GXTexObj* obj, void* img, u16 w, u16 h, u32 fmt, u32 wrapS, u32 wrapT, u8 mipmap, u32 tlutName);
extern void GXInitTexObj(GXTexObj* obj, void* img, u16 w, u16 h, u32 fmt, u32 wrapS, u32 wrapT, u8 mipmap);
extern void GXLoadTexObj(GXTexObj* obj, u32 texmap);
extern void GXInitTexObjLOD(GXTexObj* obj, GXTexFilter minFilt, GXTexFilter magFilt, f32 minLod, f32 maxLod, f32 lodBias, u8 biasClamp,
                            u8 edgeLod, u32 maxAniso);
extern void GXLoadTexMtxImm(Mtx mtx, u32 texmtx, u32 type);
extern void GXSetTexCoordGen2(u32 dst, u32 func, u32 src, u32 mtx, u8 normalize, u32 postmtx);
extern void GXLoadPosMtxImm(Mtx mtx, u32 id);
extern void GXSetCurrentMtx(u32 id);
extern void GXSetTevColor(u32 reg, GXColor color);
extern void GXSetZCompLoc(u8 before_tex);
extern void GXSetAlphaCompare(u32 comp0, u8 ref0, u32 op, u32 comp1, u8 ref1);
extern void GXSetNumChans(u8 n);
extern void GXClearVtxDesc(void);
extern void GXSetVtxDesc(u32 attr, u32 type);
extern void GXBegin(u8 prim, u8 vtxfmt, u16 nverts);
extern void GXSetPointSize(u8 size, u32 tex_offsets);
extern void GXSetLineWidth(u8 width, u32 tex_offsets);
extern void GXSetChanAmbColor(u32 chan, GXColor color);
extern void GXSetChanMatColor(u32 chan, GXColor color);
extern HSD_LObj* HSD_LObjGetActiveByID(u32 id);
extern void PSMTXScale(Mtx m, f32 x, f32 y, f32 z);
extern void PSMTXRotAxisRad(Mtx m, Vec* axis, f32 rad);
extern void PSMTXConcat(Mtx a, Mtx b, Mtx ab);
extern void PSMTXCopy(Mtx src, Mtx dst);
extern void PSMTXMultVec(Mtx mtx, Vec* src, Vec* dst);
extern f64 sin(f64 x);
extern f64 cos(f64 x);
extern f64 tan(f64 x);
extern f64 atan2(f64 y, f64 x);
extern f64 __fabs(f64 x);
extern int __float_nan[];
#define NAN (*(f32*)__float_nan)

extern f32 __float_min[];
extern int __float_epsilon[];
#define FLT_EPSILON (*(f32*)__float_epsilon)
typedef struct {
	Mtx m;
} psdisp_Mtx;

static const psdisp_Mtx psdisp_ident = { { { 1.0F, 0.0F, 0.0F, 0.0F }, { 0.0F, 1.0F, 0.0F, 0.0F }, { 0.0F, 0.0F, 1.0F, 0.0F } } };
extern HSD_PSTexGroup** psTexGroupArray[64];
/* size: 0x8 */
typedef struct {
	u8 pad[0xC];
} GXTlutObj;

typedef struct {
	void* lut;     // _00
	u32 fmt;       // _04
	u32 tlut_name; // _08
	u16 n_entries; // _0C
} HSD_TlutDesc;

extern HSD_PSFormGroup** psFormGroupArray[64];

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
s32 lbl_8065B938;
s32 lbl_8065B934;
f32 lbl_8065B930;
f32 lbl_8065B92C;
f32 lbl_8065B928;
f32 lbl_8065B924;
f32 lbl_8065B920;
f32 lbl_8065B91C;
s32 lbl_8065B918;
u32 lbl_8065B914;
GXColor lbl_8065B910;
GXColor lbl_8065B90C;
GXColor psdispTevReg0Color;
GXColor psdispTevReg1Color;
GXColor psdispTevReg2Color;
s32 lbl_8065B8FC;
HSD_Fog* particleFog;

static const f64 lbl_805E6010 = 0.01;

HSD_Particle* psDispSubPointTrail(HSD_Particle* pp);
void psDispSubAPPSRTPoint(HSD_Particle* pp);
void psDispSub(HSD_Particle* pp, u32* cmdList);
void psDispSubMakePolygon(HSD_Particle* pp, u32* cmdList, f32 x, f32 y, f32 z, f32 prevX, f32 prevY, f32 prevZ, f32 rightX, f32 rightY,
                          f32 rightZ, f32 upX, f32 upY, f32 upZ);
void psDispSubAppSRT(HSD_Particle* pp, u32* cmdList);
void setupTevReg(HSD_Particle* pp);
void setupChanReg(HSD_Particle* pp);

static inline void GXPosition3f32(const f32 x, const f32 y, const f32 z)
{
	GX_FIFO_F32 = x;
	GX_FIFO_F32 = y;
	GX_FIFO_F32 = z;
}

static inline void GXColor4u8(const u8 r, const u8 g, const u8 b, const u8 a)
{
	GX_FIFO_U8 = r;
	GX_FIFO_U8 = g;
	GX_FIFO_U8 = b;
	GX_FIFO_U8 = a;
}

static inline void GXTexCoord2f32(const f32 s, const f32 t)
{
	GX_FIFO_F32 = s;
	GX_FIFO_F32 = t;
}

static inline void GXTexCoord1x8(const u8 s)
{
	GX_FIFO_U8 = s;
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

static inline int psdisp_fpclassifyf(f32 x)
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
static inline f32 psdisp_sqrtf(f32 x)
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
	} else if (psdisp_fpclassifyf(x) == 1) {
		return NAN;
	}
	return x;
}

// the color a trail or point gets, interpolated prim color or plain white
static inline void psdisp_getTrailColor(HSD_Particle* pp, GXColor* color)
{
	switch (pp->kind & 0x80000080) {
	case 0x0:
	case 0x80000000:
		if (pp->primColCount != 0) {
			int scale = 0x10000 * pp->primColRemain / pp->primColCount;
			color->r  = ((pp->primColTarget.r << 16) + (pp->primCol.r - pp->primColTarget.r) * scale) >> 16;
			color->g  = ((pp->primColTarget.g << 16) + (pp->primCol.g - pp->primColTarget.g) * scale) >> 16;
			color->b  = ((pp->primColTarget.b << 16) + (pp->primCol.b - pp->primColTarget.b) * scale) >> 16;
			color->a  = ((pp->primColTarget.a << 16) + (pp->primCol.a - pp->primColTarget.a) * scale) >> 16;
		} else {
			*color = pp->primCol;
		}
		break;
	case 0x80:
	case 0x80000080:
		color->r = color->g = color->b = color->a = 0xFF;
		break;
	}
}

static inline void psdisp_getColorPrimEnv(HSD_Particle* pp, GXColor* prim, GXColor* env)
{
	if (pp->primColCount != 0) {
		int scale = 0x10000 * pp->primColRemain / pp->primColCount;
		prim->r   = ((pp->primColTarget.r << 16) + (pp->primCol.r - pp->primColTarget.r) * scale) >> 16;
		prim->g   = ((pp->primColTarget.g << 16) + (pp->primCol.g - pp->primColTarget.g) * scale) >> 16;
		prim->b   = ((pp->primColTarget.b << 16) + (pp->primCol.b - pp->primColTarget.b) * scale) >> 16;
		prim->a   = ((pp->primColTarget.a << 16) + (pp->primCol.a - pp->primColTarget.a) * scale) >> 16;
	} else {
		*prim = pp->primCol;
	}
	if (pp->envColCount != 0) {
		int scale = 0x10000 * pp->envColRemain / pp->envColCount;
		env->r    = ((pp->envColTarget.r << 16) + (pp->envCol.r - pp->envColTarget.r) * scale) >> 16;
		env->g    = ((pp->envColTarget.g << 16) + (pp->envCol.g - pp->envColTarget.g) * scale) >> 16;
		env->b    = ((pp->envColTarget.b << 16) + (pp->envCol.b - pp->envColTarget.b) * scale) >> 16;
		env->a    = ((pp->envColTarget.a << 16) + (pp->envCol.a - pp->envColTarget.a) * scale) >> 16;
	} else {
		*env = pp->envCol;
	}
}

static inline void psdisp_getColorMatAmb(HSD_Particle* pp, GXColor* mat, GXColor* amb)
{
	if (pp->matColCount != 0) {
		int scale = 0x10000 * pp->matColRemain / pp->matColCount;
		u8 rgb    = ((pp->matRGBTarget << 16) + (pp->matRGB - pp->matRGBTarget) * scale) >> 16;
		mat->r = mat->g = mat->b = rgb;
		mat->a                   = ((pp->matATarget << 16) + (pp->matA - pp->matATarget) * scale) >> 16;
	} else {
		mat->r = mat->g = mat->b = pp->matRGB;
		mat->a                   = pp->matA;
	}
	if (pp->ambColCount != 0) {
		int scale = 0x10000 * pp->ambColRemain / pp->ambColCount;
		u8 rgb    = ((pp->ambRGBTarget << 16) + (pp->ambRGB - pp->ambRGBTarget) * scale) >> 16;
		amb->r = amb->g = amb->b = rgb;
		amb->a                   = ((pp->ambATarget << 16) + (pp->ambA - pp->ambATarget) * scale) >> 16;
	} else {
		amb->r = amb->g = amb->b = pp->ambRGB;
		amb->a                   = pp->ambA;
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

	vz0 = pp->vel.z - gp->aux.line.x2;
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
void setupChanReg(HSD_Particle* pp)
{
	GXColor prim;
	GXColor amb;
	GXColor mat;
	HSD_LObj* lobj;

	if (pp->kind & 0x80000000) {
		psdisp_getColorMatAmb(pp, &mat, &amb);
		if (pp->kind & 0x80) {
			prim.r = prim.g = prim.b = 0xFF;
		} else {
			psdisp_getColorPrimEnv(pp, &prim, &mat);
			amb.r = (amb.r * prim.r) >> 8;
			amb.g = (amb.g * prim.g) >> 8;
			amb.b = (amb.b * prim.b) >> 8;
		}
		if (prim.r != lbl_8065B910.r || prim.g != lbl_8065B910.g || prim.b != lbl_8065B910.b) {
			lbl_8065B910 = prim;
			GXSetChanMatColor(GX_COLOR0, lbl_8065B910);
		}
		lobj = HSD_LObjGetActiveByID(0x100);
		if (lobj != NULL) {
			HSD_MulColor(&amb, &lobj->color, &amb);
		} else {
			amb.r = amb.g = amb.b = 0;
		}
		if (amb.r != lbl_8065B90C.r || amb.g != lbl_8065B90C.g || amb.b != lbl_8065B90C.b) {
			lbl_8065B90C = amb;
			GXSetChanAmbColor(GX_COLOR0, lbl_8065B90C);
		}
	}
}
/**
 * @note Address: 0x801B548C
 * @note Size: 0x41C
 */
void setupTevReg(HSD_Particle* pp)
{
	GXColor prim;
	GXColor env;
	GXColor mat;
	GXColor amb;

	psdisp_getColorPrimEnv(pp, &prim, &env);
	if ((pp->kind & 0x80) || (!(pp->kind & 0x80000000) && !(pp->kind & 0x100000))) {
		if (psdispTevReg0Color.r != prim.r || psdispTevReg0Color.g != prim.g || psdispTevReg0Color.b != prim.b || psdispTevReg0Color.a != prim.a) {
			psdispTevReg0Color = prim;
			GXSetTevColor(GX_TEVREG0, psdispTevReg0Color);
		}
		if (pp->kind & 0x80) {
			if (psdispTevReg1Color.r != env.r || psdispTevReg1Color.g != env.g || psdispTevReg1Color.b != env.b || psdispTevReg1Color.a != env.a) {
				psdispTevReg1Color = env;
				GXSetTevColor(GX_TEVREG1, psdispTevReg1Color);
			}
		} else if (psdispTevReg1Color.r != 0 || psdispTevReg1Color.g != 0 || psdispTevReg1Color.b != 0 || psdispTevReg1Color.a != 0) {
			psdispTevReg1Color.r = psdispTevReg1Color.g = psdispTevReg1Color.b = psdispTevReg1Color.a = 0;
			GXSetTevColor(GX_TEVREG1, psdispTevReg1Color);
		}
	}
	if (pp->kind & 0x80000000) {
		psdisp_getColorMatAmb(pp, &mat, &amb);
		if (pp->kind & 0x80) {
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

/**
 * @note Address: 0x801B4D98
 * @note Size: 0x6F4
 */
void psDispSubAPPSRTPoint(HSD_Particle* pp)
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

	if (lbl_8065B8FC != 0) {
		lbl_8065B8FC = 0;
		GXSetCurrentMtx(GX_PNMTX0);
	}

	if (pp->appsrt->frameNum != psFrameNum) {
		if (pp->appsrt->status != 2) {
			HSD_MtxSRT(pp->appsrt->mmtx, &pp->appsrt->scale, &pp->appsrt->rot, &pp->appsrt->translate, NULL);
		}
		if (pp->appsrt->status == 1) {
			pp->appsrt->status = 2;
		}
	}
	pp->appsrt->frameNum = psFrameNum;

	PSMTXCopy(pp->appsrt->mmtx, mtx);
	mtx[0][3] -= pp->appsrt->translate.x;
	mtx[1][3] -= pp->appsrt->translate.y;
	mtx[2][3] -= pp->appsrt->translate.z;

	v.x = pp->vel.x;
	v.y = pp->vel.y;
	v.z = pp->vel.z;
	PSMTXMultVec(mtx, &v, &v);
	if (pp->appsrt->xA2 != 0) {
		PSMTXMultVec(rvmtxRot, &v, &v);
	}
	velX = v.x;
	velY = v.y;
	velZ = v.z;

	v.x = pp->pos.x;
	v.y = pp->pos.y;
	v.z = pp->pos.z;
	if (pp->appsrt->xA2 != 0) {
		PSMTXMultVec(mtx, &v, &v);
		PSMTXMultVec(rvmtxRot, &v, &v);
		v.x += pp->appsrt->translate.x;
		v.y += pp->appsrt->translate.y;
		v.z += pp->appsrt->translate.z;
	} else {
		PSMTXMultVec(pp->appsrt->mmtx, &v, &v);
	}
	x = v.x;
	y = v.y;
	z = v.z;

	if (pp->kind & 4) {
		psdisp_tornadoPosSRT(pp, &prevX, &prevY, &prevZ);
	} else {
		prevX = x - velX;
		prevY = y - velY;
		prevZ = z - velZ;
	}

	w = (pp->size > 42.5) ? 255.0F : 6.0F * pp->size;
	if (pp->kind & 0x100000) {
		GXColor color;

		if (lbl_8065B934 != (s32)w) {
			lbl_8065B934 = w;
			GXSetLineWidth(w, 5);
		}
		psdisp_getTrailColor(pp, &color);
		if (pp->kind & 0x400) {
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
		if (pp->kind & 0x400) {
			GXTexCoord1x8(0);
		}
		GXPosition3f32(x, y, z);
		GXColor4u8(color.r, color.g, color.b, color.a);
		if (pp->kind & 0x400) {
			GXTexCoord1x8(1);
		}
	} else {
		if (lbl_8065B938 != (s32)w) {
			lbl_8065B938 = w;
			GXSetPointSize(w, 5);
		}
		if (pp->kind & 0x400) {
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
		if (pp->kind & 0x400) {
			GXTexCoord1x8(1);
		}
	}
}
/*
    stwu     r1, -0x140(r1)
    mflr     r0
    stw      r0, 0x144(r1)
    stfd     f31, 0x130(r1)
    psq_st   f31, 0x138(r1), 0, qr0
    stfd     f30, 0x120(r1)
    psq_st   f30, 0x128(r1), 0, qr0
    stfd     f29, 0x110(r1)
    psq_st   f29, 0x118(r1), 0, qr0
    stfd     f28, 0x100(r1)
    psq_st   f28, 0x108(r1), 0, qr0
    stfd     f27, 0xf0(r1)
    psq_st   f27, 0xf8(r1), 0, qr0
    stfd     f26, 0xe0(r1)
    psq_st   f26, 0xe8(r1), 0, qr0
    stfd     f25, 0xd0(r1)
    psq_st   f25, 0xd8(r1), 0, qr0
    stfd     f24, 0xc0(r1)
    psq_st   f24, 0xc8(r1), 0, qr0
    stfd     f23, 0xb0(r1)
    psq_st   f23, 0xb8(r1), 0, qr0
    stfd     f22, 0xa0(r1)
    psq_st   f22, 0xa8(r1), 0, qr0
    stw      r31, 0x9c(r1)
    stw      r30, 0x98(r1)
    lwz      r0, lbl_8065B8FC@sda21(r0)
    mr       r31, r3
    cmpwi    r0, 0x0
    beq      .L_801B4E1C
    li       r0, 0x0
    li       r3, 0x0
    stw      r0, lbl_8065B8FC@sda21(r0)
    bl       GXSetCurrentMtx
.L_801B4E1C:
    lwz      r6, 0x90(r31)
    lbz      r0, psFrameNum@sda21(r0)
    lbz      r3, 0x31(r6)
    cmplw    r3, r0
    beq      .L_801B4E6C
    lbz      r0, 0x30(r6)
    cmplwi   r0, 0x2
    beq      .L_801B4E54
    addi     r3, r6, 0x34
    addi     r4, r6, 0x24
    addi     r5, r6, 0x14
    addi     r6, r6, 0x8
    li       r7, 0x0
    bl       HSD_MtxSRT
.L_801B4E54:
    lwz      r3, 0x90(r31)
    lbz      r0, 0x30(r3)
    cmplwi   r0, 0x1
    bne      .L_801B4E6C
    li       r0, 0x2
    stb      r0, 0x30(r3)
.L_801B4E6C:
    lbz      r0, psFrameNum@sda21(r0)
    addi     r4, r1, 0x54
    lwz      r3, 0x90(r31)
    stb      r0, 0x31(r3)
    lwz      r3, 0x90(r31)
    addi     r3, r3, 0x34
    bl       PSMTXCopy
    lwz      r6, 0x90(r31)
    addi     r4, r1, 0x18
    lfs      f1, 0x60(r1)
    mr       r5, r4
    lfs      f0, 0x8(r6)
    addi     r3, r1, 0x54
    lfs      f2, 0x70(r1)
    fsubs    f0, f1, f0
    lfs      f1, 0x80(r1)
    stfs     f0, 0x60(r1)
    lfs      f0, 0xc(r6)
    fsubs    f0, f2, f0
    stfs     f0, 0x70(r1)
    lfs      f0, 0x10(r6)
    fsubs    f0, f1, f0
    stfs     f0, 0x80(r1)
    lfs      f0, 0x2c(r31)
    stfs     f0, 0x18(r1)
    lfs      f0, 0x30(r31)
    stfs     f0, 0x1c(r1)
    lfs      f0, 0x34(r31)
    stfs     f0, 0x20(r1)
    bl       PSMTXMultVec
    lwz      r3, 0x90(r31)
    lbz      r0, 0x72(r3)
    cmplwi   r0, 0x0
    beq      .L_801B4F08
    lis      r3, rvmtxRot@ha
    addi     r4, r1, 0x18
    addi     r3, r3, rvmtxRot@l
    mr       r5, r4
    bl       PSMTXMultVec
.L_801B4F08:
    lfs      f22, 0x18(r1)
    lfs      f0, 0x40(r31)
    lfs      f27, 0x1c(r1)
    stfs     f0, 0x18(r1)
    lfs      f28, 0x20(r1)
    lfs      f0, 0x44(r31)
    stfs     f0, 0x1c(r1)
    lfs      f0, 0x48(r31)
    stfs     f0, 0x20(r1)
    lwz      r3, 0x90(r31)
    lbz      r0, 0x72(r3)
    cmplwi   r0, 0x0
    beq      .L_801B4F98
    addi     r4, r1, 0x18
    addi     r3, r1, 0x54
    mr       r5, r4
    bl       PSMTXMultVec
    lis      r3, rvmtxRot@ha
    addi     r4, r1, 0x18
    addi     r3, r3, rvmtxRot@l
    mr       r5, r4
    bl       PSMTXMultVec
    lwz      r3, 0x90(r31)
    lfs      f1, 0x18(r1)
    lfs      f0, 0x8(r3)
    lfs      f2, 0x1c(r1)
    fadds    f0, f1, f0
    lfs      f1, 0x20(r1)
    stfs     f0, 0x18(r1)
    lfs      f0, 0xc(r3)
    fadds    f0, f2, f0
    stfs     f0, 0x1c(r1)
    lfs      f0, 0x10(r3)
    fadds    f0, f1, f0
    stfs     f0, 0x20(r1)
    b        .L_801B4FA8
.L_801B4F98:
    addi     r4, r1, 0x18
    addi     r3, r3, 0x34
    mr       r5, r4
    bl       PSMTXMultVec
.L_801B4FA8:
    lwz      r0, 0x4(r31)
    lfs      f26, 0x18(r1)
    rlwinm.  r0, r0, 0, 29, 29
    lfs      f25, 0x1c(r1)
    lfs      f24, 0x20(r1)
    beq      .L_801B5114
    lwz      r30, 0x8c(r31)
    cmplwi   r30, 0x0
    bne      .L_801B4FDC
    lfs      f22, 0x40(r31)
    lfs      f23, 0x44(r31)
    lfs      f27, 0x48(r31)
    b        .L_801B50CC
.L_801B4FDC:
    lfs      f1, 0x38(r31)
    bl       sin
    frsp     f30, f1
    lfs      f1, 0x3c(r31)
    bl       sin
    frsp     f29, f1
    lfs      f1, 0x38(r31)
    bl       cos
    frsp     f28, f1
    lfs      f1, 0x3c(r31)
    bl       cos
    lfs      f3, 0x34(r31)
    frsp     f27, f1
    lfs      f0, 0x60(r30)
    lfs      f2, 0x2c(r31)
    lfs      f1, 0x3c(r30)
    fsubs    f31, f3, f0
    lfs      f22, 0x48(r30)
    lfs      f0, lbl_8065F990@sda21(r0)
    fsubs    f23, f2, f1
    fcmpo    cr0, f22, f0
    bge      .L_801B5038
    fneg     f22, f22
.L_801B5038:
    lfs      f1, 0x4c(r30)
    lfs      f0, lbl_8065F990@sda21(r0)
    fcmpo    cr0, f1, f0
    bge      .L_801B504C
    fneg     f1, f1
.L_801B504C:
    bl       tan
    frsp     f2, f1
    lfs      f0, 0x30(r31)
    fmr      f1, f23
    fmadds   f22, f31, f2, f22
    fmuls    f22, f22, f0
    bl       cos
    frsp     f0, f1
    fmr      f1, f23
    fmuls    f23, f22, f0
    bl       sin
    frsp     f1, f1
    lfs      f8, 0x24(r30)
    fneg     f9, f23
    lfs      f6, 0x28(r30)
    fmuls    f0, f31, f29
    lfs      f3, 0x2c(r30)
    fmuls    f5, f22, f1
    fmadds   f7, f23, f27, f0
    fmuls    f4, f9, f30
    fmuls    f2, f5, f28
    fmuls    f1, f9, f28
    fmuls    f0, f5, f30
    fmuls    f5, f31, f30
    fmadds   f4, f29, f4, f2
    fmuls    f2, f31, f28
    fmsubs   f0, f29, f1, f0
    fmadds   f1, f27, f5, f4
    fadds    f22, f8, f7
    fmadds   f0, f27, f2, f0
    fadds    f23, f6, f1
    fadds    f27, f3, f0
.L_801B50CC:
    lwz      r6, 0x90(r31)
    addi     r3, r1, 0x24
    li       r7, 0x0
    addi     r4, r6, 0x24
    addi     r5, r6, 0x14
    addi     r6, r6, 0x8
    bl       HSD_MtxSRT
    addi     r4, r1, 0xc
    stfs     f22, 0xc(r1)
    mr       r5, r4
    addi     r3, r1, 0x24
    stfs     f23, 0x10(r1)
    stfs     f27, 0x14(r1)
    bl       PSMTXMultVec
    lfs      f22, 0xc(r1)
    lfs      f23, 0x10(r1)
    lfs      f27, 0x14(r1)
    b        .L_801B5120
.L_801B5114:
    fsubs    f22, f26, f22
    fsubs    f23, f25, f27
    fsubs    f27, f24, f28
.L_801B5120:
    lfs      f1, 0x4c(r31)
    lfd      f0, lbl_8065F9A8@sda21(r0)
    fcmpo    cr0, f1, f0
    ble      .L_801B5138
    lfs      f0, lbl_8065F9A0@sda21(r0)
    b        .L_801B5140
.L_801B5138:
    lfs      f0, lbl_8065F9A4@sda21(r0)
    fmuls    f0, f0, f1
.L_801B5140:
    fctiwz   f0, f0
    lwz      r0, 0x4(r31)
    rlwinm.  r0, r0, 0, 11, 11
    stfd     f0, 0x88(r1)
    lwz      r3, 0x8c(r1)
    beq      .L_801B5388
    lwz      r0, lbl_8065B934@sda21(r0)
    clrlwi   r4, r3, 24
    cmpw     r0, r4
    beq      .L_801B5174
    stw      r4, lbl_8065B934@sda21(r0)
    li       r4, 0x5
    bl       GXSetLineWidth
.L_801B5174:
    lis      r3, 0x8000
    lwz      r0, 0x4(r31)
    addi     r5, r3, 0x80
    and.     r4, r0, r5
    beq      .L_801B51B4
    bge      .L_801B51A8
    cmpw     r4, r5
    beq      .L_801B525C
    bge      .L_801B5270
    addi     r3, r3, 0x1
    cmpw     r4, r3
    bge      .L_801B5270
    b        .L_801B51B4
.L_801B51A8:
    cmpwi    r4, 0x80
    beq      .L_801B525C
    b        .L_801B5270
.L_801B51B4:
    lhz      r5, 0xe(r31)
    cmplwi   r5, 0x0
    beq      .L_801B5250
    lhz      r3, 0x6c(r31)
    lbz      r6, 0x70(r31)
    slwi     r3, r3, 16
    lbz      r7, 0x71(r31)
    divw     r11, r3, r5
    lbz      r4, 0x12(r31)
    lbz      r3, 0x13(r31)
    slwi     r10, r6, 16
    subf     r5, r6, r4
    lbz      r12, 0x72(r31)
    subf     r6, r7, r3
    slwi     r8, r7, 16
    mullw    r7, r11, r6
    lbz      r4, 0x14(r31)
    lbz      r30, 0x73(r31)
    slwi     r6, r12, 16
    lbz      r3, 0x15(r31)
    subf     r3, r30, r3
    mullw    r9, r11, r5
    subf     r5, r12, r4
    add      r7, r8, r7
    mullw    r5, r11, r5
    add      r4, r10, r9
    srawi    r9, r4, 16
    srawi    r7, r7, 16
    stb      r9, 0x8(r1)
    add      r5, r6, r5
    srawi    r5, r5, 16
    slwi     r4, r30, 16
    mullw    r3, r11, r3
    stb      r7, 0x9(r1)
    stb      r5, 0xa(r1)
    add      r3, r4, r3
    srawi    r3, r3, 16
    stb      r3, 0xb(r1)
    b        .L_801B5270
.L_801B5250:
    lwz      r3, 0x12(r31)
    stw      r3, 0x8(r1)
    b        .L_801B5270
.L_801B525C:
    li       r3, 0xff
    stb      r3, 0xb(r1)
    stb      r3, 0xa(r1)
    stb      r3, 0x9(r1)
    stb      r3, 0x8(r1)
.L_801B5270:
    rlwinm.  r0, r0, 0, 21, 21
    beq      .L_801B52B4
    bl       GXClearVtxDesc
    li       r3, 0x9
    li       r4, 0x1
    bl       GXSetVtxDesc
    li       r3, 0xb
    li       r4, 0x1
    bl       GXSetVtxDesc
    li       r3, 0xd
    li       r4, 0x2
    bl       GXSetVtxDesc
    li       r3, 0xa8
    li       r4, 0x2
    li       r5, 0x2
    bl       GXBegin
    b        .L_801B52E0
.L_801B52B4:
    bl       GXClearVtxDesc
    li       r3, 0x9
    li       r4, 0x1
    bl       GXSetVtxDesc
    li       r3, 0xb
    li       r4, 0x1
    bl       GXSetVtxDesc
    li       r3, 0xa8
    li       r4, 0x3
    li       r5, 0x2
    bl       GXBegin
.L_801B52E0:
    lbz      r7, 0xb(r1)
    lis      r3, 0xcc01
    stfs     f22, -0x8000(r3)
    lis      r0, 0x4330
    lfd      f2, lbl_8065F9E8@sda21(r0)
    stfs     f23, -0x8000(r3)
    lbz      r6, 0x8(r1)
    stfs     f27, -0x8000(r3)
    lbz      r5, 0x9(r1)
    lfs      f0, 0x88(r31)
    stw      r7, 0x8c(r1)
    lbz      r4, 0xa(r1)
    stw      r0, 0x88(r1)
    lfd      f1, 0x88(r1)
    stb      r6, -0x8000(r3)
    fsubs    f1, f1, f2
    stb      r5, -0x8000(r3)
    fmuls    f0, f1, f0
    stb      r4, -0x8000(r3)
    fctiwz   f0, f0
    stfd     f0, 0x90(r1)
    lwz      r0, 0x94(r1)
    stb      r0, -0x8000(r3)
    lwz      r0, 0x4(r31)
    rlwinm.  r0, r0, 0, 21, 21
    beq      .L_801B5350
    li       r0, 0x0
    stb      r0, -0x8000(r3)
.L_801B5350:
    lis      r3, 0xcc01
    stfs     f26, -0x8000(r3)
    stfs     f25, -0x8000(r3)
    stfs     f24, -0x8000(r3)
    stb      r6, -0x8000(r3)
    stb      r5, -0x8000(r3)
    stb      r4, -0x8000(r3)
    stb      r7, -0x8000(r3)
    lwz      r0, 0x4(r31)
    rlwinm.  r0, r0, 0, 21, 21
    beq      .L_801B5424
    li       r0, 0x1
    stb      r0, -0x8000(r3)
    b        .L_801B5424
.L_801B5388:
    lwz      r0, lbl_8065B938@sda21(r0)
    clrlwi   r4, r3, 24
    cmpw     r0, r4
    beq      .L_801B53A4
    stw      r4, lbl_8065B938@sda21(r0)
    li       r4, 0x5
    bl       GXSetPointSize
.L_801B53A4:
    lwz      r0, 0x4(r31)
    rlwinm.  r0, r0, 0, 21, 21
    beq      .L_801B53E0
    bl       GXClearVtxDesc
    li       r3, 0x9
    li       r4, 0x1
    bl       GXSetVtxDesc
    li       r3, 0xd
    li       r4, 0x2
    bl       GXSetVtxDesc
    li       r3, 0xb8
    li       r4, 0x0
    li       r5, 0x1
    bl       GXBegin
    b        .L_801B5400
.L_801B53E0:
    bl       GXClearVtxDesc
    li       r3, 0x9
    li       r4, 0x1
    bl       GXSetVtxDesc
    li       r3, 0xb8
    li       r4, 0x1
    li       r5, 0x1
    bl       GXBegin
.L_801B5400:
    lis      r3, 0xcc01
    stfs     f26, -0x8000(r3)
    stfs     f25, -0x8000(r3)
    stfs     f24, -0x8000(r3)
    lwz      r0, 0x4(r31)
    rlwinm.  r0, r0, 0, 21, 21
    beq      .L_801B5424
    li       r0, 0x1
    stb      r0, -0x8000(r3)
.L_801B5424:
    psq_l    f31, 0x138(r1), 0, qr0
    lfd      f31, 0x130(r1)
    psq_l    f30, 0x128(r1), 0, qr0
    lfd      f30, 0x120(r1)
    psq_l    f29, 0x118(r1), 0, qr0
    lfd      f29, 0x110(r1)
    psq_l    f28, 0x108(r1), 0, qr0
    lfd      f28, 0x100(r1)
    psq_l    f27, 0xf8(r1), 0, qr0
    lfd      f27, 0xf0(r1)
    psq_l    f26, 0xe8(r1), 0, qr0
    lfd      f26, 0xe0(r1)
    psq_l    f25, 0xd8(r1), 0, qr0
    lfd      f25, 0xd0(r1)
    psq_l    f24, 0xc8(r1), 0, qr0
    lfd      f24, 0xc0(r1)
    psq_l    f23, 0xb8(r1), 0, qr0
    lfd      f23, 0xb0(r1)
    psq_l    f22, 0xa8(r1), 0, qr0
    lfd      f22, 0xa0(r1)
    lwz      r31, 0x9c(r1)
    lwz      r0, 0x144(r1)
    lwz      r30, 0x98(r1)
    mtlr     r0
    addi     r1, r1, 0x140
    blr
*/

// these two sit right after the split end, the split probably wants to grow to cover them

/**
 * @note Address: 0x801B4934
 * @note Size: 0x464
 */
HSD_Particle* psDispSubPointTrail(HSD_Particle* pp)
{
	f32 dx;
	f32 dy;
	f32 dz;
	GXColor col;
	u8 w;

	if (lbl_8065B8FC != 0) {
		lbl_8065B8FC = 0;
		GXSetCurrentMtx(GX_PNMTX0);
	}

	w = (pp->size > 42.5) ? 255.0F : 6.0F * pp->size;
	if (lbl_8065B934 != (s32)w) {
		lbl_8065B934 = w;
		GXSetLineWidth(w, 5);
	}

	if (pp->kind & 4) {
		psdisp_tornadoPos(pp, &dx, &dy, &dz);
	} else {
		dx = pp->pos.x - pp->vel.x;
		dy = pp->pos.y - pp->vel.y;
		dz = pp->pos.z - pp->vel.z;
	}

	psdisp_getTrailColor(pp, &col);

	if (pp->kind & 0x400) {
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
	if (pp->kind & 0x400) {
		GXTexCoord1x8(0);
	}
	GXPosition3f32(pp->pos.x, pp->pos.y, pp->pos.z);
	GXColor4u8(col.r, col.g, col.b, col.a);
	if (pp->kind & 0x400) {
		GXTexCoord1x8(1);
	}
	return pp;
}

/**
 * @note Address: 0x801B3EF4
 * @note Size: 0xA40
 */
void psDispSubAppSRT(HSD_Particle* pp, u32* cmdList)
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

	if (pp->appsrt->frameNum != psFrameNum) {
		if (pp->appsrt->status != 2) {
			HSD_MtxSRT(pp->appsrt->mmtx, &pp->appsrt->scale, &pp->appsrt->rot, &pp->appsrt->translate, NULL);
		}
		if (pp->appsrt->status == 1) {
			pp->appsrt->status = 2;
		}
	}
	pp->appsrt->frameNum = psFrameNum;

	PSMTXCopy(pp->appsrt->mmtx, mtx);
	mtx[0][3] -= pp->appsrt->translate.x;
	mtx[1][3] -= pp->appsrt->translate.y;
	mtx[2][3] -= pp->appsrt->translate.z;

	v.x = pp->vel.x;
	v.y = pp->vel.y;
	v.z = pp->vel.z;
	PSMTXMultVec(mtx, &v, &v);
	if (pp->appsrt->xA2 != 0 && !(pp->kind & 4)) {
		PSMTXMultVec(rvmtxRot, &v, &v);
	}
	velX = v.x;
	velY = v.y;
	velZ = v.z;

	v.x = pp->pos.x;
	v.y = pp->pos.y;
	v.z = pp->pos.z;
	if (pp->appsrt->xA2 != 0) {
		PSMTXMultVec(mtx, &v, &v);
		PSMTXMultVec(rvmtxRot, &v, &v);
		v.x += pp->appsrt->translate.x;
		v.y += pp->appsrt->translate.y;
		v.z += pp->appsrt->translate.z;
	} else {
		PSMTXMultVec(pp->appsrt->mmtx, &v, &v);
	}
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

	if ((pp->kind & 0x100000) || (pp->kind & 0x200000)) {
		f32 sx;
		f32 sy;

		if (0.0F == psdisp_proj[0]) {
			f32 prevX;
			f32 prevY;
			f32 prevZ;
			f32 w0;
			f32 w1;
			f32 depth;

			if (pp->kind & 4) {
				psdisp_tornadoPos(pp, &prevX, &prevY, &prevZ);
				v.x = prevX;
				v.y = prevY;
				v.z = prevZ;
				if (pp->appsrt->xA2 != 0) {
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
		} else if (pp->kind & 4) {
			f32 dx;
			f32 dy;
			f32 dz;

			psdisp_tornadoPos(pp, &dx, &dy, &dz);
			v.x = dx;
			v.y = dy;
			v.z = dz;
			if (pp->appsrt->xA2 != 0) {
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
		if (__fabs(sy) < __float_min[0]) {
			angle = (sx >= 0.0F) ? 1.5707964F : -1.5707964F;
		} else {
			angle = psdisp_atan2f(sx, sy);
		}
		if (pp->kind & 0x200000) {
			angle += pp->rotate;
		}
	} else {
	no_dir:
		angle = pp->rotate;
	}

	if (__fabs(angle) > lbl_805E6010) {
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
void psDispSubMakePolygon(HSD_Particle* pp, u32* cmdList, f32 x, f32 y, f32 z, f32 prevX, f32 prevY, f32 prevZ, f32 rightX, f32 rightY,
                          f32 rightZ, f32 upX, f32 upY, f32 upZ)
{
	// NONMATCHING: retail gives the fourth quad vertex's trail-alpha conversion its own stack temps (frame 0x1A0, ours 0x190);
	// MWCC reuses the first vertex's temps across the texcoord branches. Also dx/dy/dz get f3/f2/f4 in retail, f2/f4/f5 here.
	GXColor color;

	if (lbl_8065B8FC != 0) {
		lbl_8065B8FC = 0;
		GXSetCurrentMtx(GX_PNMTX0);
	}
	if (pp->kind & 0x100000) {
		if (pp->kind & 4) {
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
			if (pp->kind & 0x400) {
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
			if (pp->kind & 0x400) {
				GXTexCoord1x8((pp->kind >> 16) & 0xC);
			}
			GXPosition3f32(x - upX, y - upY, z - upZ);
			GXColor4u8(color.r, color.g, color.b, color.a);
			if (pp->kind & 0x400) {
				GXTexCoord1x8(((pp->kind >> 16) & 0xC) + 1);
			}
			GXPosition3f32(x + rightX, y + rightY, z + rightZ);
			GXColor4u8(color.r, color.g, color.b, color.a);
			if (pp->kind & 0x400) {
				GXTexCoord1x8(((pp->kind >> 16) & 0xC) + 2);
			}
			GXPosition3f32(prevX + upX, prevY + upY, prevZ + upZ);
			GXColor4u8(color.r, color.g, color.b, (f32)color.a * pp->trail);
			if (pp->kind & 0x400) {
				GXTexCoord1x8(((pp->kind >> 16) & 0xC) + 3);
			}
		} else {
			f32 trailAlpha;
			f32 upLen;

			trailAlpha = 255.0F * (1.0F - pp->trail);
			upLen      = psdisp_sqrtf(upX * upX + upY * upY + upZ * upZ);

			if (!(__fabs(upLen) < __float_min[0])) {
				f32 dx    = x - prevX;
				f32 dy    = y - prevY;
				f32 dz    = z - prevZ;
				f32 xl    = dx * dx;
				f32 yl    = dy * dy;
				f32 zl    = dz * dz;
				f32 ratio = psdisp_sqrtf(zl + (xl + yl)) / upLen;
				u32 primCount;

				if (ratio < 1.0F) {
					ratio = 1.0F;
				}
				upX *= ratio;
				upY *= ratio;
				upZ *= ratio;
				primCount = *cmdList++;
				for (; primCount != 0; primCount--) {
					u8 prim  = ((u8*)cmdList)[0];
					u8 count = ((u8*)cmdList)[1];
					s32 i;

					cmdList++;
					if (pp->kind & 0x400) {
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
						f32 s = ((f32*)cmdList)[0];
						f32 t;
						f32 sx = 2.0F * (s - 0.5F);
						f32 tx;
						s32 alpha;

						if (pp->kind & 0x40000) {
							s = 1.0F - s;
						}
						t = ((f32*)cmdList)[1];
						cmdList += 2;
						alpha = 255.0F - t * trailAlpha;
						if (alpha < 0) {
							alpha = 0;
						}
						if (alpha > 0xFF) {
							alpha = 0xFF;
						}
						tx = 2.0F * (t - 0.5F);
						if (pp->kind & 0x80000) {
							t = 1.0F - t;
						}
						GXPosition3f32(upX * tx + (rightX * sx + x), upY * tx + (rightY * sx + y), upZ * tx + (rightZ * sx + z));
						GXColor4u8(color.r, color.g, color.b, alpha);
						if (pp->kind & 0x400) {
							GXTexCoord2f32(s, t);
						}
					}
				}
			}
		}
	} else if (cmdList == NULL) {
		if (pp->kind & 0x400) {
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
		if (pp->kind & 0x400) {
			GXTexCoord1x8((pp->kind >> 16) & 0xC);
		}
		GXPosition3f32(x - upX, y - upY, z - upZ);
		if (pp->kind & 0x400) {
			GXTexCoord1x8(((pp->kind >> 16) & 0xC) + 1);
		}
		GXPosition3f32(x + rightX, y + rightY, z + rightZ);
		if (pp->kind & 0x400) {
			GXTexCoord1x8(((pp->kind >> 16) & 0xC) + 2);
		}
		GXPosition3f32(x + upX, y + upY, z + upZ);
		if (pp->kind & 0x400) {
			GXTexCoord1x8(((pp->kind >> 16) & 0xC) + 3);
		}
	} else {
		u32 primCount = *cmdList++;

		for (; primCount != 0; primCount--) {
			u8 prim  = ((u8*)cmdList)[0];
			u8 count = ((u8*)cmdList)[1];
			s32 i;

			cmdList++;
			if (pp->kind & 0x400) {
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
				f32 s = ((f32*)cmdList)[0];
				f32 t;
				f32 sx = 2.0F * (s - 0.5F);
				f32 tx;

				if (pp->kind & 0x40000) {
					s = 1.0F - s;
				}
				t = ((f32*)cmdList)[1];
				cmdList += 2;
				tx = 2.0F * (t - 0.5F);
				if (pp->kind & 0x80000) {
					t = 1.0F - t;
				}
				GXPosition3f32(x + rightX * sx + upX * tx, y + rightY * sx + upY * tx, z + rightZ * sx + upZ * tx);
				if (pp->kind & 0x400) {
					GXTexCoord2f32(s, t);
				}
			}
		}
	}
}
/*
    stwu     r1, -0x1a0(r1)
    mflr     r0
    stw      r0, 0x1a4(r1)
    stfd     f31, 0x190(r1)
    psq_st   f31, 0x198(r1), 0, qr0
    stfd     f30, 0x180(r1)
    psq_st   f30, 0x188(r1), 0, qr0
    stfd     f29, 0x170(r1)
    psq_st   f29, 0x178(r1), 0, qr0
    stfd     f28, 0x160(r1)
    psq_st   f28, 0x168(r1), 0, qr0
    stfd     f27, 0x150(r1)
    psq_st   f27, 0x158(r1), 0, qr0
    stfd     f26, 0x140(r1)
    psq_st   f26, 0x148(r1), 0, qr0
    stfd     f25, 0x130(r1)
    psq_st   f25, 0x138(r1), 0, qr0
    stfd     f24, 0x120(r1)
    psq_st   f24, 0x128(r1), 0, qr0
    stfd     f23, 0x110(r1)
    psq_st   f23, 0x118(r1), 0, qr0
    stfd     f22, 0x100(r1)
    psq_st   f22, 0x108(r1), 0, qr0
    stfd     f21, 0xf0(r1)
    psq_st   f21, 0xf8(r1), 0, qr0
    stfd     f20, 0xe0(r1)
    psq_st   f20, 0xe8(r1), 0, qr0
    stfd     f19, 0xd0(r1)
    psq_st   f19, 0xd8(r1), 0, qr0
    stfd     f18, 0xc0(r1)
    psq_st   f18, 0xc8(r1), 0, qr0
    stfd     f17, 0xb0(r1)
    psq_st   f17, 0xb8(r1), 0, qr0
    stfd     f16, 0xa0(r1)
    psq_st   f16, 0xa8(r1), 0, qr0
    stmw     r23, 0x7c(r1)
    lwz      r0, lbl_8065B8FC@sda21(r0)
    fmr      f23, f1
    fmr      f24, f2
    lfs      f28, 0x1a8(r1)
    fmr      f25, f3
    cmpwi    r0, 0x0
    fmr      f16, f4
    fmr      f17, f5
    fmr      f18, f6
    lfs      f29, 0x1ac(r1)
    fmr      f26, f7
    lfs      f30, 0x1b0(r1)
    fmr      f27, f8
    lfs      f31, 0x1b4(r1)
    mr       r30, r3
    mr       r31, r4
    beq      .L_801B32FC
    li       r0, 0x0
    li       r3, 0x0
    stw      r0, lbl_8065B8FC@sda21(r0)
    bl       GXSetCurrentMtx
.L_801B32FC:
    lwz      r3, 0x4(r30)
    rlwinm.  r0, r3, 0, 11, 11
    beq      .L_801B3C18
    rlwinm.  r0, r3, 0, 29, 29
    beq      .L_801B3580
    lwz      r0, 0x90(r30)
    cmplwi   r0, 0x0
    beq      .L_801B3470
    lwz      r26, 0x8c(r30)
    cmplwi   r26, 0x0
    bne      .L_801B3338
    lfs      f16, 0x40(r30)
    lfs      f17, 0x44(r30)
    lfs      f18, 0x48(r30)
    b        .L_801B3428
.L_801B3338:
    lfs      f1, 0x38(r30)
    bl       sin
    frsp     f22, f1
    lfs      f1, 0x3c(r30)
    bl       sin
    frsp     f21, f1
    lfs      f1, 0x38(r30)
    bl       cos
    frsp     f20, f1
    lfs      f1, 0x3c(r30)
    bl       cos
    lfs      f3, 0x34(r30)
    frsp     f19, f1
    lfs      f0, 0x60(r26)
    lfs      f2, 0x2c(r30)
    lfs      f1, 0x3c(r26)
    fsubs    f16, f3, f0
    lfs      f18, 0x48(r26)
    lfs      f0, lbl_8065F990@sda21(r0)
    fsubs    f17, f2, f1
    fcmpo    cr0, f18, f0
    bge      .L_801B3394
    fneg     f18, f18
.L_801B3394:
    lfs      f1, 0x4c(r26)
    lfs      f0, lbl_8065F990@sda21(r0)
    fcmpo    cr0, f1, f0
    bge      .L_801B33A8
    fneg     f1, f1
.L_801B33A8:
    bl       tan
    frsp     f2, f1
    lfs      f0, 0x30(r30)
    fmr      f1, f17
    fmadds   f18, f16, f2, f18
    fmuls    f18, f18, f0
    bl       cos
    frsp     f0, f1
    fmr      f1, f17
    fmuls    f17, f18, f0
    bl       sin
    frsp     f1, f1
    lfs      f8, 0x24(r26)
    fneg     f9, f17
    lfs      f6, 0x28(r26)
    fmuls    f0, f16, f21
    lfs      f3, 0x2c(r26)
    fmuls    f5, f18, f1
    fmadds   f7, f17, f19, f0
    fmuls    f4, f9, f22
    fmuls    f2, f5, f20
    fmuls    f1, f9, f20
    fmuls    f0, f5, f22
    fmuls    f5, f16, f22
    fmadds   f4, f21, f4, f2
    fmuls    f2, f16, f20
    fmsubs   f0, f21, f1, f0
    fmadds   f1, f19, f5, f4
    fadds    f16, f8, f7
    fmadds   f0, f19, f2, f0
    fadds    f17, f6, f1
    fadds    f18, f3, f0
.L_801B3428:
    lwz      r6, 0x90(r30)
    addi     r3, r1, 0x20
    li       r7, 0x0
    addi     r4, r6, 0x24
    addi     r5, r6, 0x14
    addi     r6, r6, 0x8
    bl       HSD_MtxSRT
    addi     r4, r1, 0x14
    stfs     f16, 0x14(r1)
    mr       r5, r4
    addi     r3, r1, 0x20
    stfs     f17, 0x18(r1)
    stfs     f18, 0x1c(r1)
    bl       PSMTXMultVec
    lfs      f16, 0x14(r1)
    lfs      f17, 0x18(r1)
    lfs      f18, 0x1c(r1)
    b        .L_801B358C
.L_801B3470:
    lwz      r26, 0x8c(r30)
    cmplwi   r26, 0x0
    bne      .L_801B348C
    lfs      f16, 0x40(r30)
    lfs      f17, 0x44(r30)
    lfs      f18, 0x48(r30)
    b        .L_801B358C
.L_801B348C:
    lfs      f1, 0x38(r30)
    bl       sin
    frsp     f19, f1
    lfs      f1, 0x3c(r30)
    bl       sin
    frsp     f20, f1
    lfs      f1, 0x38(r30)
    bl       cos
    frsp     f21, f1
    lfs      f1, 0x3c(r30)
    bl       cos
    lfs      f3, 0x34(r30)
    frsp     f22, f1
    lfs      f0, 0x60(r26)
    lfs      f2, 0x2c(r30)
    lfs      f1, 0x3c(r26)
    fsubs    f18, f3, f0
    lfs      f16, 0x48(r26)
    lfs      f0, lbl_8065F990@sda21(r0)
    fsubs    f17, f2, f1
    fcmpo    cr0, f16, f0
    bge      .L_801B34E8
    fneg     f16, f16
.L_801B34E8:
    lfs      f1, 0x4c(r26)
    lfs      f0, lbl_8065F990@sda21(r0)
    fcmpo    cr0, f1, f0
    bge      .L_801B34FC
    fneg     f1, f1
.L_801B34FC:
    bl       tan
    frsp     f2, f1
    lfs      f0, 0x30(r30)
    fmr      f1, f17
    fmadds   f16, f18, f2, f16
    fmuls    f16, f16, f0
    bl       cos
    frsp     f0, f1
    fmr      f1, f17
    fmuls    f17, f16, f0
    bl       sin
    frsp     f1, f1
    lfs      f8, 0x24(r26)
    fneg     f9, f17
    lfs      f6, 0x28(r26)
    fmuls    f0, f18, f20
    lfs      f3, 0x2c(r26)
    fmuls    f5, f16, f1
    fmadds   f7, f17, f22, f0
    fmuls    f4, f9, f19
    fmuls    f2, f5, f21
    fmuls    f1, f9, f21
    fmuls    f0, f5, f19
    fmuls    f5, f18, f19
    fmadds   f4, f20, f4, f2
    fmuls    f2, f18, f21
    fmsubs   f0, f20, f1, f0
    fmadds   f1, f22, f5, f4
    fadds    f16, f8, f7
    fmadds   f0, f22, f2, f0
    fadds    f17, f6, f1
    fadds    f18, f3, f0
    b        .L_801B358C
.L_801B3580:
    fsubs    f16, f23, f16
    fsubs    f17, f24, f17
    fsubs    f18, f25, f18
.L_801B358C:
    lis      r3, 0x8000
    lwz      r0, 0x4(r30)
    addi     r5, r3, 0x80
    and.     r4, r0, r5
    beq      .L_801B35CC
    bge      .L_801B35C0
    cmpw     r4, r5
    beq      .L_801B3674
    bge      .L_801B3688
    addi     r3, r3, 0x1
    cmpw     r4, r3
    bge      .L_801B3688
    b        .L_801B35CC
.L_801B35C0:
    cmpwi    r4, 0x80
    beq      .L_801B3674
    b        .L_801B3688
.L_801B35CC:
    lhz      r5, 0xe(r30)
    cmplwi   r5, 0x0
    beq      .L_801B3668
    lhz      r3, 0x6c(r30)
    lbz      r6, 0x70(r30)
    slwi     r3, r3, 16
    lbz      r7, 0x71(r30)
    divw     r11, r3, r5
    lbz      r4, 0x12(r30)
    lbz      r3, 0x13(r30)
    slwi     r10, r6, 16
    subf     r5, r6, r4
    lbz      r12, 0x72(r30)
    subf     r6, r7, r3
    slwi     r8, r7, 16
    mullw    r7, r11, r6
    lbz      r4, 0x14(r30)
    lbz      r26, 0x73(r30)
    slwi     r6, r12, 16
    lbz      r3, 0x15(r30)
    subf     r3, r26, r3
    mullw    r9, r11, r5
    subf     r5, r12, r4
    add      r7, r8, r7
    mullw    r5, r11, r5
    add      r4, r10, r9
    srawi    r9, r4, 16
    srawi    r7, r7, 16
    stb      r9, 0x10(r1)
    add      r5, r6, r5
    srawi    r5, r5, 16
    slwi     r4, r26, 16
    mullw    r3, r11, r3
    stb      r7, 0x11(r1)
    stb      r5, 0x12(r1)
    add      r3, r4, r3
    srawi    r3, r3, 16
    stb      r3, 0x13(r1)
    b        .L_801B3688
.L_801B3668:
    lwz      r3, 0x12(r30)
    stw      r3, 0x10(r1)
    b        .L_801B3688
.L_801B3674:
    li       r3, 0xff
    stb      r3, 0x13(r1)
    stb      r3, 0x12(r1)
    stb      r3, 0x11(r1)
    stb      r3, 0x10(r1)
.L_801B3688:
    cmplwi   r31, 0x0
    bne      .L_801B3878
    rlwinm.  r0, r0, 0, 21, 21
    beq      .L_801B36D4
    bl       GXClearVtxDesc
    li       r3, 0x9
    li       r4, 0x1
    bl       GXSetVtxDesc
    li       r3, 0xb
    li       r4, 0x1
    bl       GXSetVtxDesc
    li       r3, 0xd
    li       r4, 0x2
    bl       GXSetVtxDesc
    li       r3, 0x80
    li       r4, 0x2
    li       r5, 0x4
    bl       GXBegin
    b        .L_801B3700
.L_801B36D4:
    bl       GXClearVtxDesc
    li       r3, 0x9
    li       r4, 0x1
    bl       GXSetVtxDesc
    li       r3, 0xb
    li       r4, 0x1
    bl       GXSetVtxDesc
    li       r3, 0x80
    li       r4, 0x3
    li       r5, 0x4
    bl       GXBegin
.L_801B3700:
    fsubs    f1, f16, f26
    lbz      r8, 0x13(r1)
    lis      r3, 0xcc01
    lis      r0, 0x4330
    fsubs    f0, f17, f27
    stw      r8, 0x54(r1)
    stfs     f1, -0x8000(r3)
    fsubs    f2, f18, f28
    lfd      f1, lbl_8065F9E8@sda21(r0)
    stfs     f0, -0x8000(r3)
    lbz      r7, 0x10(r1)
    stw      r0, 0x50(r1)
    lbz      r6, 0x11(r1)
    lfd      f0, 0x50(r1)
    stfs     f2, -0x8000(r3)
    fsubs    f1, f0, f1
    lbz      r5, 0x12(r1)
    lfs      f0, 0x88(r30)
    fmuls    f0, f1, f0
    stb      r7, -0x8000(r3)
    stb      r6, -0x8000(r3)
    fctiwz   f0, f0
    stb      r5, -0x8000(r3)
    stfd     f0, 0x58(r1)
    lwz      r0, 0x5c(r1)
    stb      r0, -0x8000(r3)
    lwz      r4, 0x4(r30)
    rlwinm.  r0, r4, 0, 21, 21
    beq      .L_801B377C
    rlwinm   r0, r4, 16, 28, 29
    stb      r0, -0x8000(r3)
.L_801B377C:
    fsubs    f2, f23, f29
    lis      r4, 0xcc01
    fsubs    f1, f24, f30
    fsubs    f0, f25, f31
    stfs     f2, -0x8000(r4)
    stfs     f1, -0x8000(r4)
    stfs     f0, -0x8000(r4)
    stb      r7, -0x8000(r4)
    stb      r6, -0x8000(r4)
    stb      r5, -0x8000(r4)
    stb      r8, -0x8000(r4)
    lwz      r3, 0x4(r30)
    rlwinm.  r0, r3, 0, 21, 21
    beq      .L_801B37C0
    rlwinm   r3, r3, 16, 28, 29
    addi     r0, r3, 0x1
    stb      r0, -0x8000(r4)
.L_801B37C0:
    fadds    f2, f23, f26
    lis      r4, 0xcc01
    fadds    f1, f24, f27
    fadds    f0, f25, f28
    stfs     f2, -0x8000(r4)
    stfs     f1, -0x8000(r4)
    stfs     f0, -0x8000(r4)
    stb      r7, -0x8000(r4)
    stb      r6, -0x8000(r4)
    stb      r5, -0x8000(r4)
    stb      r8, -0x8000(r4)
    lwz      r3, 0x4(r30)
    rlwinm.  r0, r3, 0, 21, 21
    beq      .L_801B3804
    rlwinm   r3, r3, 16, 28, 29
    addi     r0, r3, 0x2
    stb      r0, -0x8000(r4)
.L_801B3804:
    fadds    f1, f16, f29
    lis      r4, 0xcc01
    lis      r0, 0x4330
    fadds    f0, f17, f30
    stw      r8, 0x64(r1)
    fadds    f2, f18, f31
    stfs     f1, -0x8000(r4)
    lfd      f1, lbl_8065F9E8@sda21(r0)
    stfs     f0, -0x8000(r4)
    stw      r0, 0x60(r1)
    lfd      f0, 0x60(r1)
    stfs     f2, -0x8000(r4)
    fsubs    f1, f0, f1
    lfs      f0, 0x88(r30)
    fmuls    f0, f1, f0
    stb      r7, -0x8000(r4)
    stb      r6, -0x8000(r4)
    fctiwz   f0, f0
    stb      r5, -0x8000(r4)
    stfd     f0, 0x68(r1)
    lwz      r0, 0x6c(r1)
    stb      r0, -0x8000(r4)
    lwz      r3, 0x4(r30)
    rlwinm.  r0, r3, 0, 21, 21
    beq      .L_801B3E60
    rlwinm   r3, r3, 16, 28, 29
    addi     r0, r3, 0x3
    stb      r0, -0x8000(r4)
    b        .L_801B3E60
.L_801B3878:
    fmuls    f0, f30, f30
    lfs      f3, lbl_8065F998@sda21(r0)
    lfs      f2, 0x88(r30)
    lfs      f4, lbl_8065F9A0@sda21(r0)
    fmadds   f0, f29, f29, f0
    lfs      f1, lbl_8065F990@sda21(r0)
    fsubs    f2, f3, f2
    fmadds   f0, f31, f31, f0
    fmuls    f19, f4, f2
    fcmpo    cr0, f0, f1
    ble      .L_801B38EC
    frsqrte  f2, f0
    lfd      f4, lbl_8065F9C8@sda21(r0)
    lfd      f3, lbl_8065F9D0@sda21(r0)
    fmul     f1, f2, f2
    fmul     f2, f4, f2
    fnmsub   f1, f0, f1, f3
    fmul     f2, f2, f1
    fmul     f1, f2, f2
    fmul     f2, f4, f2
    fnmsub   f1, f0, f1, f3
    fmul     f2, f2, f1
    fmul     f1, f2, f2
    fmul     f2, f4, f2
    fnmsub   f1, f0, f1, f3
    fmul     f1, f2, f1
    fmul     f0, f0, f1
    frsp     f0, f0
    b        .L_801B3970
.L_801B38EC:
    lfd      f1, lbl_8065F9D8@sda21(r0)
    fcmpo    cr0, f0, f1
    bge      .L_801B3904
    lis      r3, __float_nan@ha
    lfs      f0, __float_nan@l(r3)
    b        .L_801B3970
.L_801B3904:
    stfs     f0, 0xc(r1)
    lis      r0, 0x7f80
    lwz      r4, 0xc(r1)
    rlwinm   r3, r4, 0, 1, 8
    cmpw     r3, r0
    beq      .L_801B392C
    bge      .L_801B395C
    cmpwi    r3, 0x0
    beq      .L_801B3944
    b        .L_801B395C
.L_801B392C:
    clrlwi.  r0, r4, 9
    beq      .L_801B393C
    li       r0, 0x1
    b        .L_801B3960
.L_801B393C:
    li       r0, 0x2
    b        .L_801B3960
.L_801B3944:
    clrlwi.  r0, r4, 9
    beq      .L_801B3954
    li       r0, 0x5
    b        .L_801B3960
.L_801B3954:
    li       r0, 0x3
    b        .L_801B3960
.L_801B395C:
    li       r0, 0x4
.L_801B3960:
    cmpwi    r0, 0x1
    bne      .L_801B3970
    lis      r3, __float_nan@ha
    lfs      f0, __float_nan@l(r3)
.L_801B3970:
    lis      r3, __float_min@ha
    fabs     f2, f0
    lfs      f1, __float_min@l(r3)
    fcmpo    cr0, f2, f1
    blt      .L_801B3E60
    fsubs    f3, f23, f16
    lfs      f1, lbl_8065F990@sda21(r0)
    fsubs    f2, f24, f17
    fsubs    f4, f25, f18
    fmuls    f3, f3, f3
    fmuls    f2, f2, f2
    fmuls    f4, f4, f4
    fadds    f2, f3, f2
    fadds    f5, f4, f2
    fcmpo    cr0, f5, f1
    ble      .L_801B39F8
    frsqrte  f2, f5
    lfd      f4, lbl_8065F9C8@sda21(r0)
    lfd      f3, lbl_8065F9D0@sda21(r0)
    fmul     f1, f2, f2
    fmul     f2, f4, f2
    fnmsub   f1, f5, f1, f3
    fmul     f2, f2, f1
    fmul     f1, f2, f2
    fmul     f2, f4, f2
    fnmsub   f1, f5, f1, f3
    fmul     f2, f2, f1
    fmul     f1, f2, f2
    fmul     f2, f4, f2
    fnmsub   f1, f5, f1, f3
    fmul     f1, f2, f1
    fmul     f5, f5, f1
    frsp     f5, f5
    b        .L_801B3A7C
.L_801B39F8:
    lfd      f1, lbl_8065F9D8@sda21(r0)
    fcmpo    cr0, f5, f1
    bge      .L_801B3A10
    lis      r3, __float_nan@ha
    lfs      f5, __float_nan@l(r3)
    b        .L_801B3A7C
.L_801B3A10:
    stfs     f5, 0x8(r1)
    lis      r0, 0x7f80
    lwz      r4, 0x8(r1)
    rlwinm   r3, r4, 0, 1, 8
    cmpw     r3, r0
    beq      .L_801B3A38
    bge      .L_801B3A68
    cmpwi    r3, 0x0
    beq      .L_801B3A50
    b        .L_801B3A68
.L_801B3A38:
    clrlwi.  r0, r4, 9
    beq      .L_801B3A48
    li       r0, 0x1
    b        .L_801B3A6C
.L_801B3A48:
    li       r0, 0x2
    b        .L_801B3A6C
.L_801B3A50:
    clrlwi.  r0, r4, 9
    beq      .L_801B3A60
    li       r0, 0x5
    b        .L_801B3A6C
.L_801B3A60:
    li       r0, 0x3
    b        .L_801B3A6C
.L_801B3A68:
    li       r0, 0x4
.L_801B3A6C:
    cmpwi    r0, 0x1
    bne      .L_801B3A7C
    lis      r3, __float_nan@ha
    lfs      f5, __float_nan@l(r3)
.L_801B3A7C:
    fdivs    f1, f5, f0
    lfs      f0, lbl_8065F998@sda21(r0)
    fcmpo    cr0, f1, f0
    bge      .L_801B3A90
    fmr      f1, f0
.L_801B3A90:
    fmuls    f29, f29, f1
    lwz      r25, 0x0(r31)
    fmuls    f30, f30, f1
    lbz      r26, 0x10(r1)
    fmuls    f31, f31, f1
    lbz      r27, 0x11(r1)
    lbz      r28, 0x12(r1)
    lis      r29, 0xcc01
    lfs      f22, lbl_8065F994@sda21(r0)
    addi     r31, r31, 0x4
    lfs      f21, lbl_8065F9E0@sda21(r0)
    lfs      f20, lbl_8065F9A0@sda21(r0)
    b        .L_801B3C0C
.L_801B3AC4:
    lwz      r0, 0x4(r30)
    lbz      r24, 0x0(r31)
    rlwinm.  r0, r0, 0, 21, 21
    lbz      r23, 0x1(r31)
    addi     r31, r31, 0x4
    beq      .L_801B3B18
    bl       GXClearVtxDesc
    li       r3, 0x9
    li       r4, 0x1
    bl       GXSetVtxDesc
    li       r3, 0xb
    li       r4, 0x1
    bl       GXSetVtxDesc
    li       r3, 0xd
    li       r4, 0x1
    bl       GXSetVtxDesc
    mr       r3, r24
    mr       r5, r23
    li       r4, 0x5
    bl       GXBegin
    b        .L_801B3B44
.L_801B3B18:
    bl       GXClearVtxDesc
    li       r3, 0x9
    li       r4, 0x1
    bl       GXSetVtxDesc
    li       r3, 0xb
    li       r4, 0x1
    bl       GXSetVtxDesc
    mr       r3, r24
    mr       r5, r23
    li       r4, 0x3
    bl       GXBegin
.L_801B3B44:
    mtctr    r23
    cmpwi    r23, 0x0
    ble      .L_801B3C08
.L_801B3B50:
    lfs      f3, 0x0(r31)
    lwz      r3, 0x4(r30)
    fsubs    f0, f3, f21
    rlwinm.  r0, r3, 0, 13, 13
    fmuls    f5, f22, f0
    beq      .L_801B3B70
    lfs      f0, lbl_8065F998@sda21(r0)
    fsubs    f3, f0, f3
.L_801B3B70:
    lfs      f4, 0x4(r31)
    addi     r31, r31, 0x8
    fnmsubs  f0, f4, f19, f20
    fctiwz   f0, f0
    stfd     f0, 0x68(r1)
    lwz      r4, 0x6c(r1)
    cmpwi    r4, 0x0
    bge      .L_801B3B94
    li       r4, 0x0
.L_801B3B94:
    cmpwi    r4, 0xff
    ble      .L_801B3BA0
    li       r4, 0xff
.L_801B3BA0:
    fsubs    f0, f4, f21
    rlwinm.  r0, r3, 0, 12, 12
    fmuls    f6, f22, f0
    beq      .L_801B3BB8
    lfs      f0, lbl_8065F998@sda21(r0)
    fsubs    f4, f0, f4
.L_801B3BB8:
    fmadds   f2, f26, f5, f23
    fmadds   f1, f27, f5, f24
    fmadds   f0, f28, f5, f25
    fmadds   f2, f29, f6, f2
    fmadds   f1, f30, f6, f1
    fmadds   f0, f31, f6, f0
    stfs     f2, -0x8000(r29)
    stfs     f1, -0x8000(r29)
    stfs     f0, -0x8000(r29)
    stb      r26, -0x8000(r29)
    stb      r27, -0x8000(r29)
    stb      r28, -0x8000(r29)
    stb      r4, -0x8000(r29)
    lwz      r0, 0x4(r30)
    rlwinm.  r0, r0, 0, 21, 21
    beq      .L_801B3C04
    lis      r3, 0xcc01
    stfs     f3, -0x8000(r3)
    stfs     f4, -0x8000(r3)
.L_801B3C04:
    bdnz     .L_801B3B50
.L_801B3C08:
    subi     r25, r25, 0x1
.L_801B3C0C:
    cmplwi   r25, 0x0
    bne      .L_801B3AC4
    b        .L_801B3E60
.L_801B3C18:
    cmplwi   r31, 0x0
    bne      .L_801B3D48
    rlwinm.  r0, r3, 0, 21, 21
    beq      .L_801B3C58
    bl       GXClearVtxDesc
    li       r3, 0x9
    li       r4, 0x1
    bl       GXSetVtxDesc
    li       r3, 0xd
    li       r4, 0x2
    bl       GXSetVtxDesc
    li       r3, 0x80
    li       r4, 0x0
    li       r5, 0x4
    bl       GXBegin
    b        .L_801B3C78
.L_801B3C58:
    bl       GXClearVtxDesc
    li       r3, 0x9
    li       r4, 0x1
    bl       GXSetVtxDesc
    li       r3, 0x80
    li       r4, 0x1
    li       r5, 0x4
    bl       GXBegin
.L_801B3C78:
    fsubs    f2, f23, f26
    lis      r3, 0xcc01
    fsubs    f1, f24, f27
    fsubs    f0, f25, f28
    stfs     f2, -0x8000(r3)
    stfs     f1, -0x8000(r3)
    stfs     f0, -0x8000(r3)
    lwz      r4, 0x4(r30)
    rlwinm.  r0, r4, 0, 21, 21
    beq      .L_801B3CA8
    rlwinm   r0, r4, 16, 28, 29
    stb      r0, -0x8000(r3)
.L_801B3CA8:
    fsubs    f2, f23, f29
    lis      r4, 0xcc01
    fsubs    f1, f24, f30
    fsubs    f0, f25, f31
    stfs     f2, -0x8000(r4)
    stfs     f1, -0x8000(r4)
    stfs     f0, -0x8000(r4)
    lwz      r3, 0x4(r30)
    rlwinm.  r0, r3, 0, 21, 21
    beq      .L_801B3CDC
    rlwinm   r3, r3, 16, 28, 29
    addi     r0, r3, 0x1
    stb      r0, -0x8000(r4)
.L_801B3CDC:
    fadds    f2, f23, f26
    lis      r4, 0xcc01
    fadds    f1, f24, f27
    fadds    f0, f25, f28
    stfs     f2, -0x8000(r4)
    stfs     f1, -0x8000(r4)
    stfs     f0, -0x8000(r4)
    lwz      r3, 0x4(r30)
    rlwinm.  r0, r3, 0, 21, 21
    beq      .L_801B3D10
    rlwinm   r3, r3, 16, 28, 29
    addi     r0, r3, 0x2
    stb      r0, -0x8000(r4)
.L_801B3D10:
    fadds    f2, f23, f29
    lis      r4, 0xcc01
    fadds    f1, f24, f30
    fadds    f0, f25, f31
    stfs     f2, -0x8000(r4)
    stfs     f1, -0x8000(r4)
    stfs     f0, -0x8000(r4)
    lwz      r3, 0x4(r30)
    rlwinm.  r0, r3, 0, 21, 21
    beq      .L_801B3E60
    rlwinm   r3, r3, 16, 28, 29
    addi     r0, r3, 0x3
    stb      r0, -0x8000(r4)
    b        .L_801B3E60
.L_801B3D48:
    lwz      r23, 0x0(r31)
    lis      r29, 0xcc01
    lfs      f20, lbl_8065F994@sda21(r0)
    addi     r31, r31, 0x4
    lfs      f19, lbl_8065F9E0@sda21(r0)
    b        .L_801B3E58
.L_801B3D60:
    lwz      r0, 0x4(r30)
    lbz      r24, 0x0(r31)
    rlwinm.  r0, r0, 0, 21, 21
    lbz      r25, 0x1(r31)
    addi     r31, r31, 0x4
    beq      .L_801B3DA8
    bl       GXClearVtxDesc
    li       r3, 0x9
    li       r4, 0x1
    bl       GXSetVtxDesc
    li       r3, 0xd
    li       r4, 0x1
    bl       GXSetVtxDesc
    mr       r3, r24
    mr       r5, r25
    li       r4, 0x4
    bl       GXBegin
    b        .L_801B3DC8
.L_801B3DA8:
    bl       GXClearVtxDesc
    li       r3, 0x9
    li       r4, 0x1
    bl       GXSetVtxDesc
    mr       r3, r24
    mr       r5, r25
    li       r4, 0x1
    bl       GXBegin
.L_801B3DC8:
    mtctr    r25
    cmpwi    r25, 0x0
    ble      .L_801B3E54
.L_801B3DD4:
    lfs      f3, 0x0(r31)
    lwz      r3, 0x4(r30)
    fsubs    f0, f3, f19
    rlwinm.  r0, r3, 0, 13, 13
    fmuls    f5, f20, f0
    beq      .L_801B3DF4
    lfs      f0, lbl_8065F998@sda21(r0)
    fsubs    f3, f0, f3
.L_801B3DF4:
    lfs      f4, 0x4(r31)
    rlwinm.  r0, r3, 0, 12, 12
    addi     r31, r31, 0x8
    fsubs    f0, f4, f19
    fmuls    f6, f20, f0
    beq      .L_801B3E14
    lfs      f0, lbl_8065F998@sda21(r0)
    fsubs    f4, f0, f4
.L_801B3E14:
    fmadds   f2, f26, f5, f23
    fmadds   f1, f27, f5, f24
    fmadds   f0, f28, f5, f25
    fmadds   f2, f29, f6, f2
    fmadds   f1, f30, f6, f1
    fmadds   f0, f31, f6, f0
    stfs     f2, -0x8000(r29)
    stfs     f1, -0x8000(r29)
    stfs     f0, -0x8000(r29)
    lwz      r0, 0x4(r30)
    rlwinm.  r0, r0, 0, 21, 21
    beq      .L_801B3E50
    lis      r3, 0xcc01
    stfs     f3, -0x8000(r3)
    stfs     f4, -0x8000(r3)
.L_801B3E50:
    bdnz     .L_801B3DD4
.L_801B3E54:
    subi     r23, r23, 0x1
.L_801B3E58:
    cmplwi   r23, 0x0
    bne      .L_801B3D60
.L_801B3E60:
    psq_l    f31, 0x198(r1), 0, qr0
    lfd      f31, 0x190(r1)
    psq_l    f30, 0x188(r1), 0, qr0
    lfd      f30, 0x180(r1)
    psq_l    f29, 0x178(r1), 0, qr0
    lfd      f29, 0x170(r1)
    psq_l    f28, 0x168(r1), 0, qr0
    lfd      f28, 0x160(r1)
    psq_l    f27, 0x158(r1), 0, qr0
    lfd      f27, 0x150(r1)
    psq_l    f26, 0x148(r1), 0, qr0
    lfd      f26, 0x140(r1)
    psq_l    f25, 0x138(r1), 0, qr0
    lfd      f25, 0x130(r1)
    psq_l    f24, 0x128(r1), 0, qr0
    lfd      f24, 0x120(r1)
    psq_l    f23, 0x118(r1), 0, qr0
    lfd      f23, 0x110(r1)
    psq_l    f22, 0x108(r1), 0, qr0
    lfd      f22, 0x100(r1)
    psq_l    f21, 0xf8(r1), 0, qr0
    lfd      f21, 0xf0(r1)
    psq_l    f20, 0xe8(r1), 0, qr0
    lfd      f20, 0xe0(r1)
    psq_l    f19, 0xd8(r1), 0, qr0
    lfd      f19, 0xd0(r1)
    psq_l    f18, 0xc8(r1), 0, qr0
    lfd      f18, 0xc0(r1)
    psq_l    f17, 0xb8(r1), 0, qr0
    lfd      f17, 0xb0(r1)
    psq_l    f16, 0xa8(r1), 0, qr0
    lfd      f16, 0xa0(r1)
    lmw      r23, 0x7c(r1)
    lwz      r0, 0x1a4(r1)
    mtlr     r0
    addi     r1, r1, 0x1a0
    blr
*/

/**
 * @note Address: 0x801B2B14
 * @note Size: 0x704
 */
void psDispSub(HSD_Particle* pp, u32* cmdList)
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
		rightX = lbl_8065B930 * pp->size;
		upX    = lbl_8065B92C * pp->size;
		rightY = lbl_8065B928 * pp->size;
		upY    = lbl_8065B924 * pp->size;
		rightZ = lbl_8065B920 * pp->size;
		upZ    = lbl_8065B91C * pp->size;
	}

	if ((pp->kind & 0x100000) || (pp->kind & 0x200000)) {
		f32 sx;
		f32 sy;

		if (0.0F == psdisp_proj[0]) {
			f32 prevX;
			f32 prevY;
			f32 prevZ;
			f32 w1;
			f32 w0;
			f32 depth;

			if (pp->kind & 4) {
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
		} else if (pp->kind & 4) {
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
		if (__fabs(sy) < __float_min[0]) {
			angle = (sx >= 0.0F) ? 1.5707964F : -1.5707964F;
		} else {
			angle = psdisp_atan2f(sx, sy);
		}
		if (pp->kind & 0x200000) {
			angle += pp->rotate;
		}
	} else {
	no_dir:
		angle = pp->rotate;
	}

	if (__fabs(angle) > lbl_805E6010) {
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
/*
    stwu     r1, -0x170(r1)
    mflr     r0
    stw      r0, 0x174(r1)
    stfd     f31, 0x160(r1)
    psq_st   f31, 0x168(r1), 0, qr0
    stfd     f30, 0x150(r1)
    psq_st   f30, 0x158(r1), 0, qr0
    stfd     f29, 0x140(r1)
    psq_st   f29, 0x148(r1), 0, qr0
    stfd     f28, 0x130(r1)
    psq_st   f28, 0x138(r1), 0, qr0
    stfd     f27, 0x120(r1)
    psq_st   f27, 0x128(r1), 0, qr0
    stfd     f26, 0x110(r1)
    psq_st   f26, 0x118(r1), 0, qr0
    stfd     f25, 0x100(r1)
    psq_st   f25, 0x108(r1), 0, qr0
    stfd     f24, 0xf0(r1)
    psq_st   f24, 0xf8(r1), 0, qr0
    stfd     f23, 0xe0(r1)
    psq_st   f23, 0xe8(r1), 0, qr0
    stfd     f22, 0xd0(r1)
    psq_st   f22, 0xd8(r1), 0, qr0
    stfd     f21, 0xc0(r1)
    psq_st   f21, 0xc8(r1), 0, qr0
    stfd     f20, 0xb0(r1)
    psq_st   f20, 0xb8(r1), 0, qr0
    stfd     f19, 0xa0(r1)
    psq_st   f19, 0xa8(r1), 0, qr0
    stfd     f18, 0x90(r1)
    psq_st   f18, 0x98(r1), 0, qr0
    stfd     f17, 0x80(r1)
    psq_st   f17, 0x88(r1), 0, qr0
    stfd     f16, 0x70(r1)
    psq_st   f16, 0x78(r1), 0, qr0
    stw      r31, 0x6c(r1)
    stw      r30, 0x68(r1)
    stw      r29, 0x64(r1)
    stw      r28, 0x60(r1)
    mr       r30, r3
    lfs      f0, 0x40(r3)
    lfs      f1, 0x44(r30)
    lis      r3, rvmtxRot@ha
    lfs      f2, 0x48(r30)
    mr.      r31, r4
    fmr      f31, f0
    addi     r29, r3, rvmtxRot@l
    fmr      f30, f1
    fmr      f29, f2
    beq      .L_801B2C24
    addi     r3, r29, 0x7c
    lfs      f8, 0x7c(r29)
    lfs      f5, 0x4(r3)
    lfs      f4, 0x14(r3)
    lfs      f3, 0x24(r3)
    fneg     f7, f5
    lfs      f9, 0x4c(r30)
    fneg     f5, f4
    fneg     f3, f3
    lfs      f6, 0x10(r3)
    lfs      f4, 0x20(r3)
    fmuls    f28, f8, f9
    fmuls    f27, f7, f9
    fmuls    f26, f6, f9
    fmuls    f25, f5, f9
    fmuls    f24, f4, f9
    fmuls    f23, f3, f9
    b        .L_801B2C58
.L_801B2C24:
    lfs      f5, lbl_8065B930@sda21(r0)
    lfs      f6, 0x4c(r30)
    lfs      f4, lbl_8065B92C@sda21(r0)
    lfs      f3, lbl_8065B928@sda21(r0)
    fmuls    f28, f5, f6
    lfs      f5, lbl_8065B924@sda21(r0)
    fmuls    f27, f4, f6
    lfs      f4, lbl_8065B920@sda21(r0)
    fmuls    f26, f3, f6
    lfs      f3, lbl_8065B91C@sda21(r0)
    fmuls    f25, f5, f6
    fmuls    f24, f4, f6
    fmuls    f23, f3, f6
.L_801B2C58:
    lwz      r3, 0x4(r30)
    rlwinm.  r0, r3, 0, 11, 11
    bne      .L_801B2C6C
    rlwinm.  r0, r3, 0, 10, 10
    beq      .L_801B307C
.L_801B2C6C:
    lfs      f4, lbl_8065F990@sda21(r0)
    lfs      f3, 0x60(r29)
    fcmpu    cr0, f4, f3
    bne      .L_801B2E84
    rlwinm.  r0, r3, 0, 29, 29
    beq      .L_801B2D94
    lwz      r28, 0x8c(r30)
    cmplwi   r28, 0x0
    bne      .L_801B2CA0
    fmr      f8, f0
    fmr      f4, f1
    fmr      f9, f2
    b        .L_801B2DAC
.L_801B2CA0:
    lfs      f1, 0x38(r30)
    bl       sin
    frsp     f22, f1
    lfs      f1, 0x3c(r30)
    bl       sin
    frsp     f21, f1
    lfs      f1, 0x38(r30)
    bl       cos
    frsp     f20, f1
    lfs      f1, 0x3c(r30)
    bl       cos
    lfs      f3, 0x34(r30)
    frsp     f19, f1
    lfs      f0, 0x60(r28)
    lfs      f2, 0x2c(r30)
    lfs      f1, 0x3c(r28)
    fsubs    f18, f3, f0
    lfs      f16, 0x48(r28)
    lfs      f0, lbl_8065F990@sda21(r0)
    fsubs    f17, f2, f1
    fcmpo    cr0, f16, f0
    bge      .L_801B2CFC
    fneg     f16, f16
.L_801B2CFC:
    lfs      f1, 0x4c(r28)
    lfs      f0, lbl_8065F990@sda21(r0)
    fcmpo    cr0, f1, f0
    bge      .L_801B2D10
    fneg     f1, f1
.L_801B2D10:
    bl       tan
    frsp     f2, f1
    lfs      f0, 0x30(r30)
    fmr      f1, f17
    fmadds   f16, f18, f2, f16
    fmuls    f16, f16, f0
    bl       cos
    frsp     f0, f1
    fmr      f1, f17
    fmuls    f17, f16, f0
    bl       sin
    frsp     f1, f1
    lfs      f8, 0x24(r28)
    fneg     f9, f17
    lfs      f6, 0x28(r28)
    fmuls    f0, f18, f21
    lfs      f3, 0x2c(r28)
    fmuls    f5, f16, f1
    fmadds   f7, f17, f19, f0
    fmuls    f4, f9, f22
    fmuls    f2, f5, f20
    fmuls    f1, f9, f20
    fmuls    f0, f5, f22
    fmuls    f5, f18, f22
    fmadds   f4, f21, f4, f2
    fmuls    f2, f18, f20
    fmsubs   f0, f21, f1, f0
    fmadds   f1, f19, f5, f4
    fadds    f8, f8, f7
    fmadds   f0, f19, f2, f0
    fadds    f4, f6, f1
    fadds    f9, f3, f0
    b        .L_801B2DAC
.L_801B2D94:
    lfs      f5, 0x2c(r30)
    lfs      f4, 0x30(r30)
    lfs      f3, 0x34(r30)
    fsubs    f8, f0, f5
    fsubs    f4, f1, f4
    fsubs    f9, f2, f3
.L_801B2DAC:
    addi     r3, r29, 0xac
    lfs      f0, 0x44(r30)
    lfs      f7, 0x24(r3)
    lfs      f10, 0x20(r3)
    fmuls    f3, f7, f0
    lfs      f1, 0x40(r30)
    lfs      f12, 0x28(r3)
    lfs      f2, 0x48(r30)
    fmadds   f3, f10, f1, f3
    lfs      f13, 0x2c(r3)
    lfs      f6, lbl_8065F990@sda21(r0)
    fmadds   f3, f12, f2, f3
    fadds    f11, f13, f3
    fcmpu    cr0, f6, f11
    beq      .L_801B307C
    lfs      f5, lbl_8065F9B0@sda21(r0)
    fmuls    f3, f7, f4
    fdivs    f11, f5, f11
    fmadds   f3, f10, f8, f3
    fmadds   f3, f12, f9, f3
    fadds    f3, f13, f3
    fcmpu    cr0, f6, f3
    beq      .L_801B307C
    fdivs    f10, f5, f3
    addi     r3, r29, 0x30
    lfs      f13, 0x4(r3)
    lfs      f19, 0x14(r3)
    lfs      f18, 0x30(r29)
    lfs      f5, 0x10(r3)
    fmuls    f7, f13, f4
    lfs      f3, 0x8(r3)
    fmuls    f12, f19, f4
    lfs      f6, 0x18(r3)
    fmuls    f17, f13, f0
    lfs      f4, 0xc(r3)
    fmadds   f16, f18, f8, f7
    lfs      f7, 0x1c(r3)
    fmuls    f13, f19, f0
    fmadds   f0, f5, f8, f12
    fmadds   f12, f18, f1, f17
    fmadds   f8, f3, f9, f16
    fmadds   f1, f5, f1, f13
    fmadds   f0, f6, f9, f0
    fmadds   f5, f3, f2, f12
    fadds    f3, f4, f8
    fmadds   f1, f6, f2, f1
    fadds    f0, f7, f0
    fadds    f4, f4, f5
    fmuls    f3, f10, f3
    fadds    f2, f7, f1
    fmuls    f0, f10, f0
    fmsubs   f1, f11, f4, f3
    fmsubs   f2, f11, f2, f0
    b        .L_801B3024
.L_801B2E84:
    rlwinm.  r0, r3, 0, 29, 29
    beq      .L_801B2FE4
    lwz      r28, 0x8c(r30)
    cmplwi   r28, 0x0
    bne      .L_801B2EA4
    fmr      f4, f0
    fmr      f5, f2
    b        .L_801B2F94
.L_801B2EA4:
    lfs      f1, 0x38(r30)
    bl       sin
    frsp     f19, f1
    lfs      f1, 0x3c(r30)
    bl       sin
    frsp     f20, f1
    lfs      f1, 0x38(r30)
    bl       cos
    frsp     f21, f1
    lfs      f1, 0x3c(r30)
    bl       cos
    lfs      f3, 0x34(r30)
    frsp     f22, f1
    lfs      f0, 0x60(r28)
    lfs      f2, 0x2c(r30)
    lfs      f1, 0x3c(r28)
    fsubs    f16, f3, f0
    lfs      f18, 0x48(r28)
    lfs      f0, lbl_8065F990@sda21(r0)
    fsubs    f17, f2, f1
    fcmpo    cr0, f18, f0
    bge      .L_801B2F00
    fneg     f18, f18
.L_801B2F00:
    lfs      f1, 0x4c(r28)
    lfs      f0, lbl_8065F990@sda21(r0)
    fcmpo    cr0, f1, f0
    bge      .L_801B2F14
    fneg     f1, f1
.L_801B2F14:
    bl       tan
    frsp     f2, f1
    lfs      f0, 0x30(r30)
    fmr      f1, f17
    fmadds   f18, f16, f2, f18
    fmuls    f18, f18, f0
    bl       cos
    frsp     f0, f1
    fmr      f1, f17
    fmuls    f17, f18, f0
    bl       sin
    frsp     f1, f1
    lfs      f8, 0x24(r28)
    fneg     f9, f17
    lfs      f6, 0x28(r28)
    fmuls    f0, f16, f20
    lfs      f3, 0x2c(r28)
    fmuls    f5, f18, f1
    fmadds   f7, f17, f22, f0
    fmuls    f4, f9, f19
    fmuls    f2, f5, f21
    fmuls    f1, f9, f21
    fmuls    f0, f5, f19
    fmuls    f5, f16, f19
    fmadds   f4, f20, f4, f2
    fmuls    f2, f16, f21
    fmsubs   f0, f20, f1, f0
    fmadds   f1, f22, f5, f4
    fadds    f4, f8, f7
    fmadds   f0, f22, f2, f0
    fadds    f1, f6, f1
    fadds    f5, f3, f0
.L_801B2F94:
    lfs      f0, 0x44(r30)
    addi     r3, r29, 0x30
    lfs      f2, 0x40(r30)
    fsubs    f7, f0, f1
    lfs      f1, 0x4(r3)
    lfs      f0, 0x14(r3)
    fsubs    f6, f2, f4
    lfs      f4, 0x48(r30)
    fmuls    f2, f1, f7
    lfs      f3, 0x30(r29)
    fmuls    f0, f0, f7
    lfs      f1, 0x10(r3)
    fsubs    f5, f4, f5
    fmadds   f3, f3, f6, f2
    lfs      f4, 0x8(r3)
    fmadds   f0, f1, f6, f0
    lfs      f2, 0x18(r3)
    fmadds   f1, f4, f5, f3
    fmadds   f2, f2, f5, f0
    b        .L_801B3024
.L_801B2FE4:
    addi     r3, r29, 0x30
    lfs      f4, 0x30(r30)
    lfs      f1, 0x4(r3)
    lfs      f0, 0x14(r3)
    fmuls    f2, f1, f4
    lfs      f3, 0x30(r29)
    lfs      f5, 0x2c(r30)
    fmuls    f0, f0, f4
    lfs      f1, 0x10(r3)
    fmadds   f3, f3, f5, f2
    fmadds   f0, f1, f5, f0
    lfs      f1, 0x8(r3)
    lfs      f4, 0x34(r30)
    lfs      f2, 0x18(r3)
    fmadds   f1, f1, f4, f3
    fmadds   f2, f2, f4, f0
.L_801B3024:
    lis      r3, __float_min@ha
    fabs     f3, f2
    lfs      f0, __float_min@l(r3)
    fcmpo    cr0, f3, f0
    bge      .L_801B3058
    lfs      f0, lbl_8065F990@sda21(r0)
    fcmpo    cr0, f1, f0
    cror     eq, gt, eq
    bne      .L_801B3050
    lfs      f1, lbl_8065F9B4@sda21(r0)
    b        .L_801B3064
.L_801B3050:
    lfs      f1, lbl_8065F9B8@sda21(r0)
    b        .L_801B3064
.L_801B3058:
    bl       atan2
    frsp     f0, f1
    fmr      f1, f0
.L_801B3064:
    lwz      r0, 0x4(r30)
    rlwinm.  r0, r0, 0, 10, 10
    beq      .L_801B3080
    lfs      f0, 0x50(r30)
    fadds    f1, f1, f0
    b        .L_801B3080
.L_801B307C:
    lfs      f1, 0x50(r30)
.L_801B3080:
    fabs     f2, f1
    lfd      f0, lbl_8065F9C0@sda21(r0)
    fcmpo    cr0, f2, f0
    ble      .L_801B313C
    fmuls    f3, f24, f25
    addi     r3, r1, 0x24
    fmuls    f2, f28, f23
    addi     r4, r1, 0x18
    fmuls    f0, f26, f27
    fmsubs   f3, f26, f23, f3
    fmsubs   f2, f24, f27, f2
    fmsubs   f0, f28, f25, f0
    fmr      f16, f28
    stfs     f3, 0x18(r1)
    fmr      f17, f26
    fmr      f18, f27
    stfs     f2, 0x1c(r1)
    fmr      f19, f25
    stfs     f0, 0x20(r1)
    bl       PSMTXRotAxisRad
    lfs      f0, 0x28(r1)
    lfs      f1, 0x38(r1)
    lfs      f7, 0x48(r1)
    fmuls    f5, f0, f26
    lfs      f6, 0x24(r1)
    fmuls    f2, f0, f25
    fmuls    f4, f1, f26
    lfs      f8, 0x34(r1)
    fmuls    f1, f1, f25
    fmuls    f3, f7, f17
    lfs      f9, 0x44(r1)
    fmuls    f0, f7, f19
    fmadds   f5, f6, f28, f5
    lfs      f7, 0x2c(r1)
    fmadds   f2, f6, f27, f2
    fmadds   f4, f8, f16, f4
    lfs      f6, 0x3c(r1)
    fmadds   f1, f8, f18, f1
    fmadds   f3, f9, f16, f3
    lfs      f8, 0x4c(r1)
    fmadds   f0, f9, f18, f0
    fmadds   f28, f7, f24, f5
    fmadds   f26, f6, f24, f4
    fmadds   f24, f8, f24, f3
    fmadds   f27, f7, f23, f2
    fmadds   f25, f6, f23, f1
    fmadds   f23, f8, f23, f0
.L_801B313C:
    stfs     f24, 0x8(r1)
    fmr      f1, f31
    fmr      f2, f30
    mr       r3, r30
    stfs     f27, 0xc(r1)
    fmr      f3, f29
    fmr      f7, f28
    stfs     f25, 0x10(r1)
    fmr      f8, f26
    mr       r4, r31
    stfs     f23, 0x14(r1)
    lfs      f4, 0x2c(r30)
    lfs      f5, 0x30(r30)
    lfs      f6, 0x34(r30)
    bl       psDispSubMakePolygon
    psq_l    f31, 0x168(r1), 0, qr0
    lfd      f31, 0x160(r1)
    psq_l    f30, 0x158(r1), 0, qr0
    lfd      f30, 0x150(r1)
    psq_l    f29, 0x148(r1), 0, qr0
    lfd      f29, 0x140(r1)
    psq_l    f28, 0x138(r1), 0, qr0
    lfd      f28, 0x130(r1)
    psq_l    f27, 0x128(r1), 0, qr0
    lfd      f27, 0x120(r1)
    psq_l    f26, 0x118(r1), 0, qr0
    lfd      f26, 0x110(r1)
    psq_l    f25, 0x108(r1), 0, qr0
    lfd      f25, 0x100(r1)
    psq_l    f24, 0xf8(r1), 0, qr0
    lfd      f24, 0xf0(r1)
    psq_l    f23, 0xe8(r1), 0, qr0
    lfd      f23, 0xe0(r1)
    psq_l    f22, 0xd8(r1), 0, qr0
    lfd      f22, 0xd0(r1)
    psq_l    f21, 0xc8(r1), 0, qr0
    lfd      f21, 0xc0(r1)
    psq_l    f20, 0xb8(r1), 0, qr0
    lfd      f20, 0xb0(r1)
    psq_l    f19, 0xa8(r1), 0, qr0
    lfd      f19, 0xa0(r1)
    psq_l    f18, 0x98(r1), 0, qr0
    lfd      f18, 0x90(r1)
    psq_l    f17, 0x88(r1), 0, qr0
    lfd      f17, 0x80(r1)
    psq_l    f16, 0x78(r1), 0, qr0
    lfd      f16, 0x70(r1)
    lwz      r31, 0x6c(r1)
    lwz      r30, 0x68(r1)
    lwz      r29, 0x64(r1)
    lwz      r0, 0x174(r1)
    lwz      r28, 0x60(r1)
    mtlr     r0
    addi     r1, r1, 0x170
    blr
*/

/**
 * @note Address: 0x801B1C9C
 * @note Size: 0xE78
 */
void _psDispParticlesSub(s32 bankMask, s32 kind)
{
	// NONMATCHING: retail .rodata starts with a 0x50-byte block of six default Vecs from a header, so the offsets from the
	// rodata base (psdisp_ident, the blend-mode message, "psdisp.c") are 0x50 higher there
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
	void* prevLut;
	u32 prevNear;
	HSD_TlutDesc tlut;
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
				if ((u32)kind == 1 && !(pp->kind & 8)) {
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
							prevLut      = NULL;
							lbl_8065B938 = -1;
							prevTex      = NULL;
							lbl_8065B934 = -1;
							lbl_8065B914 = -1;
							psSetupTevInvalidState();
							prevMirror = -1;
							prevKind &= 0xFEFFFFFF;
							prevZMode = -1;
							HSD_FogSet(NULL);

							lbl_8065B910.r = lbl_8065B910.g = lbl_8065B910.b = 0xFF;
							lbl_8065B90C.r = lbl_8065B90C.g = lbl_8065B90C.b = 0xFF;
							lbl_8065B910.a = lbl_8065B90C.a = 0xFF;
							GXSetChanMatColor(4, lbl_8065B910);
							GXSetChanAmbColor(4, lbl_8065B90C);

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

							lbl_8065B918 = -1;
							GXSetZCompLoc(GX_DISABLE);

							{
								HSD_CObj* cobj = HSD_CObjGetCurrent();
								HSD_CObjGetViewingMtx(cobj, psdisp_view);
							}
							PSMTXInverse(psdisp_view, psdisp_cam);
							GXGetProjectionv(psdisp_proj);

							if (0.0F == psdisp_proj[0]) {
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
							lbl_8065B930 = psdisp_cam[0][0] + psdisp_cam[0][1];
							lbl_8065B92C = psdisp_cam[0][0] - psdisp_cam[0][1];
							lbl_8065B928 = psdisp_cam[1][0] + psdisp_cam[1][1];
							lbl_8065B924 = psdisp_cam[1][0] - psdisp_cam[1][1];
							lbl_8065B920 = psdisp_cam[2][0] + psdisp_cam[2][1];
							lbl_8065B91C = psdisp_cam[2][0] - psdisp_cam[2][1];

							GXLoadPosMtxImm(psdisp_view, GX_PNMTX0);
							{
								psdisp_Mtx m2;
								m2 = psdisp_ident;
								GXLoadPosMtxImm(m2.m, GX_PNMTX1);
							}

							lbl_8065B8FC = 3;
							if (lbl_8065B8FC != 0) {
								lbl_8065B8FC = 0;
								GXSetCurrentMtx(GX_PNMTX0);
							}

							GXEnableTexOffsets(0, 1, 1);
							HSD_MtxGetRotationMtx(psdisp_cam, rvmtxRot, 'z', 'x');
							GXSetCullMode(GX_CULL_BACK);
							GXSetArray(0xD, (void*)sParticleBillboardTexCoords, 2);

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

						blendMode = ((u32)pp->kind >> 0x16) & 3;
						if (lbl_8065B918 != blendMode) {
							lbl_8065B918 = blendMode;
							switch (blendMode) {
							case 0:
								GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
								break;
							case 1:
								GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_ONE, GX_LO_CLEAR);
								break;
							case 2:
								GXSetBlendMode(GX_BM_SUBTRACT, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
								break;
							case 3:
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

						lightMode = pp->kind & 0x80100000;
						if (lightMode != lbl_8065B914) {
							lbl_8065B914 = lightMode;
							GXSetNumChans(1);
							switch ((s32)lbl_8065B914) {
							case 0x100000:
								GXSetChanCtrl(GX_COLOR0A0, GX_DISABLE, GX_SRC_VTX, GX_SRC_VTX, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE);
								break;
							case 0x80000000:
								GXSetChanCtrl(GX_COLOR0, GX_ENABLE, GX_SRC_REG, GX_SRC_REG, HSD_LObjGetLightMaskDiffuse(), GX_DF_NONE,
								              HSD_LObjGetLightMaskAttnFunc() ? GX_AF_SPOT : GX_AF_NONE);
								GXSetChanCtrl(GX_ALPHA0, GX_DISABLE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE);
								break;
							case 0x80100000:
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

						if ((pp->kind & 0x10000008) != prevZMode) {
							prevZMode = pp->kind & 0x10000008;
							if (prevZMode & 8) {
								GXSetZMode(((pp->kind >> 0x1C) & 1) ^ 1, GX_LEQUAL, GX_ENABLE);
							} else {
								GXSetZMode(((pp->kind >> 0x1C) & 1) ^ 1, GX_LEQUAL, GX_DISABLE);
							}
						}

						if ((pp->kind ^ (u32)prevKind) & 0x01000000) {
							if (pp->kind & 0x01000000) {
								HSD_FogSet(particleFog);
							} else {
								HSD_FogSet(NULL);
							}
						}

						if (psFormGroupArray[pp->bank] != NULL && (formGroup = psFormGroupArray[pp->bank][pp->texGroup]) != NULL
						    && formGroup->formTable != NULL) {
							cmdList = formGroup->formTable[pp->poseNum];
						} else {
							cmdList = NULL;
						}

						if (pp->kind & 0x400) {
							if (pp->kind & 0x20) {
								scaleS = 2.0F;
								wrapS  = GX_MIRROR;
							} else {
								scaleS = 1.0F;
								wrapS  = GX_CLAMP;
							}
							if (pp->kind & 0x40) {
								scaleT = 2.0F;
								wrapT  = GX_MIRROR;
							} else {
								scaleT = 1.0F;
								wrapT  = GX_CLAMP;
							}
							if ((pp->kind & 0x60) != prevMirror) {
								Mtx texMtx;

								prevMirror = pp->kind & 0x60;
								prevTex    = NULL;
								PSMTXScale(texMtx, scaleS, scaleT, 1.0F);
								if (pp->kind & 0x40) {
									texMtx[1][3] = 1.0F;
								}
								GXLoadTexMtxImm(texMtx, 0x1E, 1);
								GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, 0x1E, GX_DISABLE, 0x7D);
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
							if (fmt == GX_TF_CI4 || fmt == GX_TF_CI8) {
								if (texTable != NULL) {
									void** palettes = (void**)&texTable[texGroup->num];

									if (palettes != NULL) {
										void* lut;

										if (pp->palNum != 0xFF) {
											lut = palettes[pp->palNum];
										} else if (!(pp->kind & 0x10)) {
											lut = palettes[pp->poseNum];
										} else {
											lut = palettes[0];
										}
										if (lut != prevLut) {
											HSD_TlutDesc* desc = &tlut;
											desc->fmt          = texGroup->tlutfmt;
											desc->tlut_name    = 0;
											desc->n_entries    = (fmt == GX_TF_CI4) ? 16 : 256;
											GXInitTlutObj(&tlutObj, lut, desc->fmt, desc->n_entries);
											GXLoadTlut(&tlutObj, desc->tlut_name);
										}
										prevTex = NULL;
									}
								}
							}
							if (prevTex != image && image != NULL) {
								prevTex = image;
								switch (fmt) {
								case GX_TF_CI4:
								case GX_TF_CI8:
									GXInitTexObjCI(&texObj, image, width, height, fmt, wrapS, wrapT, GX_DISABLE, 0);
									break;
								case 0:
								case 1:
								case 2:
								case 3:
								case 4:
								case 5:
								case 6:
								case GX_TF_CMPR:
									GXInitTexObj(&texObj, image, width, height, fmt, wrapS, wrapT, GX_DISABLE);
									break;
								default:
									assert_line(2027, 0);
									break;
								}
								prevNear = pp->kind & 0x200;
								GXInitTexObjLOD(&texObj, (prevNear != 0) ? GX_NEAR : GX_LINEAR, ((pp->kind >> 9) & 1) ? GX_NEAR : GX_LINEAR,
								                0.0F, 0.0F, 0.0F, GX_DISABLE, GX_DISABLE, 0);
								GXLoadTexObj(&texObj, GX_TEXMAP0);
							}
							if (prevNear != (pp->kind & 0x200)) {
								prevNear = pp->kind & 0x200;
								GXInitTexObjLOD(&texObj, (prevNear != 0) ? GX_NEAR : GX_LINEAR,
								                (pp->kind & 0x200) != 0 ? GX_NEAR : GX_LINEAR, 0.0F, 0.0F, 0.0F, GX_DISABLE, GX_DISABLE, 0);
								GXLoadTexObj(&texObj, GX_TEXMAP0);
							}
						}

						if (pp->kind & 0x40000000) {
							if (pp->appsrt != NULL) {
								psDispSubAPPSRTPoint(pp);
							} else if (pp->kind & 0x100000) {
								pp = psDispSubPointTrail(pp);
							} else {
								u8 w;

								if (lbl_8065B8FC != 0) {
									lbl_8065B8FC = 0;
									GXSetCurrentMtx(GX_PNMTX0);
								}
								w = (pp->size > 42.5) ? 255.0F : 6.0F * pp->size;
								if (lbl_8065B938 != (s32)w) {
									lbl_8065B938 = w;
									GXSetPointSize(w, 5);
								}
								if (pp->kind & 0x400) {
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
								if (pp->kind & 0x400) {
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
/*
    stwu     r1, -0x160(r1)
    mflr     r0
    stw      r0, 0x164(r1)
    stfd     f31, 0x150(r1)
    psq_st   f31, 0x158(r1), 0, qr0
    stfd     f30, 0x140(r1)
    psq_st   f30, 0x148(r1), 0, qr0
    stmw     r14, 0xf8(r1)
    mr.      r0, r4
    stw      r4, 0x14(r1)
    li       r0, 0x0
    lis      r5, lbl_8020F9A0@ha
    lis      r4, rvmtxRot@ha
    stw      r3, 0x10(r1)
    addi     r30, r5, lbl_8020F9A0@l
    li       r22, 0x0
    stw      r0, 0xe0(r1)
    li       r0, 0x0
    addi     r31, r4, rvmtxRot@l
    stb      r0, 0xdd(r1)
    li       r0, 0xff
    stb      r0, 0xdc(r1)
    li       r0, 0x1
    stw      r0, 0xd8(r1)
    bne      .L_801B1D20
    lbz      r3, psFrameNum@sda21(r0)
    li       r0, 0x1
    cmplwi   r3, 0xff
    bge      .L_801B1D18
    addi     r0, r3, 0x1
    clrlwi   r0, r0, 24
.L_801B1D18:
    stb      r0, psFrameNum@sda21(r0)
    b        .L_801B2AF0
.L_801B1D20:
    li       r0, 0x0
    addi     r29, r31, 0xac
    stw      r0, 0xe4(r1)
    addi     r28, r31, 0x60
    addi     r27, r31, 0x30
    addi     r26, r31, 0x7c
.L_801B1D38:
    lwz      r0, 0xe4(r1)
    li       r3, 0x1
    slw      r3, r3, r0
    lwz      r0, 0x10(r1)
    and.     r0, r0, r3
    beq      .L_801B2AC4
    lwz      r3, 0xe4(r1)
    addi     r4, r1, 0x30
    addi     r5, r1, 0x2c
    bl       particleSort
    lwz      r0, 0x14(r1)
    cmpwi    r0, 0x2
    beq      .L_801B1D84
    bge      .L_801B2AC4
    cmpwi    r0, 0x1
    bge      .L_801B1D7C
    b        .L_801B2AC4
.L_801B1D7C:
    lwz      r25, 0x30(r1)
    b        .L_801B1D88
.L_801B1D84:
    lwz      r25, 0x2c(r1)
.L_801B1D88:
    lis      r3, __float_epsilon@ha
    addi     r0, r3, __float_epsilon@l
    stw      r0, 0xe8(r1)
    b        .L_801B2ABC
.L_801B1D98:
    lwz      r0, 0x14(r1)
    cmplwi   r0, 0x1
    bne      .L_801B1DB0
    lwz      r0, 0x4(r25)
    rlwinm.  r0, r0, 0, 28, 28
    beq      .L_801B2AC4
.L_801B1DB0:
    lwz      r3, 0xe8(r1)
    lfs      f1, 0x4c(r25)
    lfs      f0, 0x0(r3)
    fcmpo    cr0, f1, f0
    blt      .L_801B2AB4
    lwz      r0, 0xd8(r1)
    cmpwi    r0, 0x0
    beq      .L_801B22B4
    li       r0, -0x1
    li       r24, 0x0
    stw      r0, lbl_8065B938@sda21(r0)
    stw      r0, lbl_8065B934@sda21(r0)
    stw      r0, lbl_8065B914@sda21(r0)
    bl       psSetupTevInvalidState
    rlwinm   r15, r15, 0, 8, 6
    li       r23, -0x1
    li       r14, -0x1
    li       r3, 0x0
    bl       fn_801D37E0
    li       r6, 0xff
    li       r5, lbl_8065B910@sda21
    li       r3, lbl_8065B910@sda21
    stb      r6, 0x1(r5)
    li       r5, lbl_8065B910@sda21
    addi     r4, r1, 0x28
    stb      r6, 0x2(r3)
    li       r3, 0x4
    stb      r6, 0x3(r5)
    li       r5, lbl_8065B90C@sda21
    stb      r6, lbl_8065B910@sda21(r0)
    lwz      r0, lbl_8065B910@sda21(r0)
    stb      r6, 0x2(r5)
    li       r5, lbl_8065B90C@sda21
    stb      r6, 0x1(r5)
    li       r5, lbl_8065B90C@sda21
    stb      r6, lbl_8065B90C@sda21(r0)
    stb      r6, 0x3(r5)
    stw      r0, 0x28(r1)
    bl       GXSetChanMatColor
    lwz      r0, lbl_8065B90C@sda21(r0)
    addi     r4, r1, 0x24
    li       r3, 0x4
    stw      r0, 0x24(r1)
    bl       GXSetChanAmbColor
    bl       psSetupTevInvalidState
    bl       psSetupTevCommon
    li       r7, 0xff
    li       r5, psdispTevReg0Color@sda21
    stb      r7, 0x1(r5)
    li       r5, psdispTevReg0Color@sda21
    li       r6, 0x0
    addi     r4, r1, 0x20
    stb      r7, 0x2(r5)
    li       r5, psdispTevReg0Color@sda21
    li       r3, 0x1
    stb      r7, 0x3(r5)
    li       r5, psdispTevReg1Color@sda21
    stb      r6, 0x1(r5)
    li       r5, psdispTevReg1Color@sda21
    stb      r7, psdispTevReg0Color@sda21(r0)
    stb      r6, 0x2(r5)
    li       r5, psdispTevReg1Color@sda21
    lwz      r0, psdispTevReg0Color@sda21(r0)
    stb      r6, 0x3(r5)
    li       r5, psdispTevReg2Color@sda21
    stb      r7, 0x1(r5)
    li       r5, psdispTevReg2Color@sda21
    stb      r7, 0x2(r5)
    li       r5, psdispTevReg2Color@sda21
    stb      r6, psdispTevReg1Color@sda21(r0)
    stb      r7, psdispTevReg2Color@sda21(r0)
    stb      r7, 0x3(r5)
    stw      r0, 0x20(r1)
    bl       GXSetTevColor
    lwz      r0, psdispTevReg1Color@sda21(r0)
    addi     r4, r1, 0x1c
    li       r3, 0x2
    stw      r0, 0x1c(r1)
    bl       GXSetTevColor
    lwz      r0, psdispTevReg2Color@sda21(r0)
    addi     r4, r1, 0x18
    li       r3, 0x3
    stw      r0, 0x18(r1)
    bl       GXSetTevColor
    li       r0, -0x1
    li       r3, 0x0
    stw      r0, lbl_8065B918@sda21(r0)
    bl       GXSetZCompLoc
    bl       HSD_CObjGetCurrent
    addi     r4, r31, 0xac
    bl       HSD_CObjGetViewingMtx
    addi     r3, r31, 0xac
    addi     r4, r31, 0x7c
    bl       PSMTXInverse
    addi     r3, r31, 0x60
    bl       fn_8018C3F0
    lfs      f1, lbl_8065F990@sda21(r0)
    lfs      f0, 0x60(r31)
    fcmpu    cr0, f1, f0
    bne      .L_801B1FE4
    lfs      f2, 0x8(r28)
    lfs      f0, 0x20(r29)
    lfs      f3, 0x24(r29)
    lfs      f7, 0x10(r28)
    fmuls    f9, f2, f0
    lfs      f12, 0x28(r29)
    fmuls    f1, f2, f3
    lfs      f10, 0x2c(r29)
    fmuls    f0, f7, f0
    fmuls    f5, f2, f12
    fmuls    f2, f2, f10
    lfs      f30, 0x4(r28)
    lfs      f11, 0xac(r31)
    fmuls    f4, f7, f3
    lfs      f8, 0x4(r29)
    lfs      f6, 0x8(r29)
    lfs      f3, 0xc(r29)
    fmadds   f31, f30, f11, f9
    fmadds   f13, f30, f8, f1
    lfs      f8, 0xc(r28)
    lfs      f1, 0x10(r29)
    fmadds   f11, f30, f6, f5
    lfs      f5, 0x14(r29)
    fmadds   f9, f30, f3, f2
    fmadds   f6, f8, f1, f0
    lfs      f3, 0x18(r29)
    fmuls    f2, f7, f12
    lfs      f1, 0x1c(r29)
    fmuls    f0, f7, f10
    fmadds   f4, f8, f5, f4
    fmadds   f2, f8, f3, f2
    stfs     f31, 0x30(r31)
    fmadds   f0, f8, f1, f0
    stfs     f13, 0x4(r27)
    stfs     f11, 0x8(r27)
    stfs     f9, 0xc(r27)
    stfs     f6, 0x10(r27)
    stfs     f4, 0x14(r27)
    stfs     f2, 0x18(r27)
    stfs     f0, 0x1c(r27)
    b        .L_801B2054
.L_801B1FE4:
    lfs      f4, 0x4(r28)
    lfs      f0, 0xac(r31)
    lfs      f3, 0x8(r28)
    lfs      f2, 0x4(r29)
    lfs      f1, 0x8(r29)
    fmadds   f9, f4, f0, f3
    lfs      f0, 0xc(r29)
    fmadds   f8, f4, f2, f3
    fmadds   f7, f4, f1, f3
    lfs      f5, 0xc(r28)
    fmadds   f6, f4, f0, f3
    lfs      f0, 0x10(r29)
    lfs      f4, 0x10(r28)
    lfs      f2, 0x14(r29)
    lfs      f1, 0x18(r29)
    fmadds   f3, f5, f0, f4
    lfs      f0, 0x1c(r29)
    fmadds   f2, f5, f2, f4
    fmadds   f1, f5, f1, f4
    stfs     f9, 0x30(r31)
    fmadds   f0, f5, f0, f4
    stfs     f8, 0x4(r27)
    stfs     f7, 0x8(r27)
    stfs     f6, 0xc(r27)
    stfs     f3, 0x10(r27)
    stfs     f2, 0x14(r27)
    stfs     f1, 0x18(r27)
    stfs     f0, 0x1c(r27)
.L_801B2054:
    lfs      f2, 0x7c(r31)
    addi     r3, r31, 0xac
    lfs      f0, 0x4(r26)
    li       r4, 0x0
    lfs      f3, 0x10(r26)
    lfs      f1, 0x14(r26)
    fadds    f6, f2, f0
    fsubs    f5, f2, f0
    lfs      f2, 0x20(r26)
    lfs      f0, 0x24(r26)
    fadds    f4, f3, f1
    fsubs    f3, f3, f1
    fadds    f1, f2, f0
    fsubs    f0, f2, f0
    stfs     f6, lbl_8065B930@sda21(r0)
    stfs     f5, lbl_8065B92C@sda21(r0)
    stfs     f4, lbl_8065B928@sda21(r0)
    stfs     f3, lbl_8065B924@sda21(r0)
    stfs     f1, lbl_8065B920@sda21(r0)
    stfs     f0, lbl_8065B91C@sda21(r0)
    bl       GXLoadPosMtxImm
    lwz      r18, 0x50(r30)
    addi     r3, r1, 0xa0
    lwz      r17, 0x54(r30)
    li       r4, 0x3
    lwz      r16, 0x58(r30)
    lwz      r12, 0x5c(r30)
    lwz      r11, 0x60(r30)
    lwz      r10, 0x64(r30)
    lwz      r9, 0x68(r30)
    lwz      r8, 0x6c(r30)
    lwz      r7, 0x70(r30)
    lwz      r6, 0x74(r30)
    lwz      r5, 0x78(r30)
    lwz      r0, 0x7c(r30)
    stw      r18, 0xa0(r1)
    stw      r17, 0xa4(r1)
    stw      r16, 0xa8(r1)
    stw      r12, 0xac(r1)
    stw      r11, 0xb0(r1)
    stw      r10, 0xb4(r1)
    stw      r9, 0xb8(r1)
    stw      r8, 0xbc(r1)
    stw      r7, 0xc0(r1)
    stw      r6, 0xc4(r1)
    stw      r5, 0xc8(r1)
    stw      r0, 0xcc(r1)
    bl       GXLoadPosMtxImm
    li       r0, 0x3
    cmpwi    r0, 0x0
    stw      r0, lbl_8065B8FC@sda21(r0)
    beq      .L_801B2134
    li       r0, 0x0
    li       r3, 0x0
    stw      r0, lbl_8065B8FC@sda21(r0)
    bl       GXSetCurrentMtx
.L_801B2134:
    li       r3, 0x0
    li       r4, 0x1
    li       r5, 0x1
    bl       GXEnableTexOffsets
    addi     r3, r31, 0x7c
    addi     r4, r31, 0x0
    li       r5, 0x7a
    li       r6, 0x78
    bl       fn_801CF3C8
    li       r3, 0x2
    bl       GXSetCullMode
    lis      r4, sParticleBillboardTexCoords@ha
    li       r3, 0xd
    addi     r4, r4, sParticleBillboardTexCoords@l
    li       r5, 0x2
    bl       fn_80187110
    li       r3, 0x0
    li       r4, 0x9
    li       r5, 0x1
    li       r6, 0x4
    li       r7, 0x0
    bl       GXSetVtxAttrFmt
    li       r3, 0x0
    li       r4, 0xd
    li       r5, 0x1
    li       r6, 0x0
    li       r7, 0x0
    bl       GXSetVtxAttrFmt
    li       r3, 0x1
    li       r4, 0x9
    li       r5, 0x1
    li       r6, 0x4
    li       r7, 0x0
    bl       GXSetVtxAttrFmt
    li       r3, 0x2
    li       r4, 0x9
    li       r5, 0x1
    li       r6, 0x4
    li       r7, 0x0
    bl       GXSetVtxAttrFmt
    li       r3, 0x2
    li       r4, 0xb
    li       r5, 0x1
    li       r6, 0x5
    li       r7, 0x0
    bl       GXSetVtxAttrFmt
    li       r3, 0x2
    li       r4, 0xd
    li       r5, 0x1
    li       r6, 0x0
    li       r7, 0x0
    bl       GXSetVtxAttrFmt
    li       r3, 0x3
    li       r4, 0x9
    li       r5, 0x1
    li       r6, 0x4
    li       r7, 0x0
    bl       GXSetVtxAttrFmt
    li       r3, 0x3
    li       r4, 0xb
    li       r5, 0x1
    li       r6, 0x5
    li       r7, 0x0
    bl       GXSetVtxAttrFmt
    li       r3, 0x4
    li       r4, 0x9
    li       r5, 0x1
    li       r6, 0x4
    li       r7, 0x0
    bl       GXSetVtxAttrFmt
    li       r3, 0x4
    li       r4, 0xd
    li       r5, 0x1
    li       r6, 0x4
    li       r7, 0x0
    bl       GXSetVtxAttrFmt
    li       r3, 0x5
    li       r4, 0x9
    li       r5, 0x1
    li       r6, 0x4
    li       r7, 0x0
    bl       GXSetVtxAttrFmt
    li       r3, 0x5
    li       r4, 0xb
    li       r5, 0x1
    li       r6, 0x5
    li       r7, 0x0
    bl       GXSetVtxAttrFmt
    li       r3, 0x5
    li       r4, 0xd
    li       r5, 0x1
    li       r6, 0x4
    li       r7, 0x0
    bl       GXSetVtxAttrFmt
    li       r0, 0x0
    stw      r0, 0xd8(r1)
.L_801B22B4:
    lwz      r3, 0x4(r25)
    lwz      r0, lbl_8065B918@sda21(r0)
    extrwi   r3, r3, 2, 8
    cmpw     r0, r3
    beq      .L_801B2360
    cmpwi    r3, 0x2
    stw      r3, lbl_8065B918@sda21(r0)
    beq      .L_801B2324
    bge      .L_801B22E8
    cmpwi    r3, 0x0
    beq      .L_801B22F4
    bge      .L_801B230C
    b        .L_801B2354
.L_801B22E8:
    cmpwi    r3, 0x4
    bge      .L_801B2354
    b        .L_801B233C
.L_801B22F4:
    li       r3, 0x1
    li       r4, 0x4
    li       r5, 0x5
    li       r6, 0x0
    bl       GXSetBlendMode
    b        .L_801B2360
.L_801B230C:
    li       r3, 0x1
    li       r4, 0x4
    li       r5, 0x1
    li       r6, 0x0
    bl       GXSetBlendMode
    b        .L_801B2360
.L_801B2324:
    li       r3, 0x3
    li       r4, 0x4
    li       r5, 0x5
    li       r6, 0x0
    bl       GXSetBlendMode
    b        .L_801B2360
.L_801B233C:
    li       r3, 0x1
    li       r4, 0x2
    li       r5, 0x0
    li       r6, 0x0
    bl       GXSetBlendMode
    b        .L_801B2360
.L_801B2354:
    addi     r3, r30, 0x80
    crclr    cr1eq
    bl       OSReport
.L_801B2360:
    lhz      r4, 0x54(r25)
    cmplwi   r4, 0x0
    beq      .L_801B23BC
    lhz      r0, 0x7c(r25)
    lbz      r5, 0x7e(r25)
    slwi     r0, r0, 16
    lbz      r3, 0x57(r25)
    divw     r7, r0, r4
    lbz      r6, 0x7f(r25)
    lbz      r0, 0x58(r25)
    subf     r3, r5, r3
    slwi     r5, r5, 16
    subf     r0, r6, r0
    mullw    r4, r7, r3
    slwi     r3, r6, 16
    mullw    r0, r7, r0
    add      r4, r5, r4
    srawi    r4, r4, 16
    add      r0, r3, r0
    clrlwi   r4, r4, 24
    srawi    r0, r0, 16
    clrlwi   r7, r0, 24
    b        .L_801B23C4
.L_801B23BC:
    lbz      r4, 0x57(r25)
    lbz      r7, 0x58(r25)
.L_801B23C4:
    lbz      r6, 0x56(r25)
    lwz      r0, 0xe0(r1)
    cmpw     r0, r6
    bne      .L_801B23F4
    lbz      r3, 0xdd(r1)
    clrlwi   r0, r4, 24
    cmplw    r3, r0
    bne      .L_801B23F4
    lbz      r3, 0xdc(r1)
    clrlwi   r0, r7, 24
    cmplw    r3, r0
    beq      .L_801B2410
.L_801B23F4:
    stw      r6, 0xe0(r1)
    extrwi   r3, r6, 3, 26
    extrwi   r5, r6, 2, 24
    clrlwi   r6, r6, 29
    stb      r4, 0xdd(r1)
    stb      r7, 0xdc(r1)
    bl       GXSetAlphaCompare
.L_801B2410:
    mr       r3, r25
    bl       psSetupTev
    lwz      r3, 0x4(r25)
    lwz      r0, lbl_8065B914@sda21(r0)
    andis.   r3, r3, 0x8010
    cmplw    r3, r0
    beq      .L_801B258C
    stw      r3, lbl_8065B914@sda21(r0)
    li       r3, 0x1
    bl       GXSetNumChans
    lwz      r4, lbl_8065B914@sda21(r0)
    lis      r0, 0x8010
    cmpw     r4, r0
    beq      .L_801B24F0
    bge      .L_801B2460
    lis      r3, 0x8000
    addi     r0, r3, 0x1
    cmpw     r4, r0
    bge      .L_801B254C
    b        .L_801B2494
.L_801B2460:
    lis      r0, 0x10
    cmpw     r4, r0
    beq      .L_801B2470
    b        .L_801B254C
.L_801B2470:
    li       r3, 0x4
    li       r4, 0x0
    li       r5, 0x1
    li       r6, 0x1
    li       r7, 0x0
    li       r8, 0x0
    li       r9, 0x2
    bl       GXSetChanCtrl
    b        .L_801B256C
.L_801B2494:
    bl       HSD_LObjGetLightMaskAttnFunc
    cmpwi    r3, 0x0
    li       r16, 0x2
    beq      .L_801B24A8
    li       r16, 0x1
.L_801B24A8:
    bl       HSD_LObjGetLightMaskDiffuse
    mr       r7, r3
    mr       r9, r16
    li       r3, 0x0
    li       r4, 0x1
    li       r5, 0x0
    li       r6, 0x0
    li       r8, 0x0
    bl       GXSetChanCtrl
    li       r3, 0x2
    li       r4, 0x0
    li       r5, 0x0
    li       r6, 0x0
    li       r7, 0x0
    li       r8, 0x0
    li       r9, 0x2
    bl       GXSetChanCtrl
    b        .L_801B256C
.L_801B24F0:
    bl       HSD_LObjGetLightMaskAttnFunc
    cmpwi    r3, 0x0
    li       r16, 0x2
    beq      .L_801B2504
    li       r16, 0x1
.L_801B2504:
    bl       HSD_LObjGetLightMaskDiffuse
    mr       r7, r3
    mr       r9, r16
    li       r3, 0x0
    li       r4, 0x1
    li       r5, 0x0
    li       r6, 0x0
    li       r8, 0x0
    bl       GXSetChanCtrl
    li       r3, 0x2
    li       r4, 0x0
    li       r5, 0x1
    li       r6, 0x1
    li       r7, 0x0
    li       r8, 0x0
    li       r9, 0x2
    bl       GXSetChanCtrl
    b        .L_801B256C
.L_801B254C:
    li       r3, 0x4
    li       r4, 0x0
    li       r5, 0x0
    li       r6, 0x0
    li       r7, 0x0
    li       r8, 0x0
    li       r9, 0x2
    bl       GXSetChanCtrl
.L_801B256C:
    li       r3, 0x5
    li       r4, 0x0
    li       r5, 0x0
    li       r6, 0x0
    li       r7, 0x0
    li       r8, 0x0
    li       r9, 0x2
    bl       GXSetChanCtrl
.L_801B258C:
    mr       r3, r25
    bl       setupChanReg
    mr       r3, r25
    bl       setupTevReg
    lis      r3, 0x1000
    lwz      r4, 0x4(r25)
    addi     r0, r3, 0x8
    and      r3, r4, r0
    cmplw    r3, r14
    beq      .L_801B25EC
    rlwinm.  r0, r3, 0, 28, 28
    mr       r14, r3
    beq      .L_801B25D8
    extrwi   r0, r4, 1, 3
    li       r4, 0x3
    xori     r3, r0, 0x1
    li       r5, 0x1
    bl       GXSetZMode
    b        .L_801B25EC
.L_801B25D8:
    extrwi   r0, r4, 1, 3
    li       r4, 0x3
    xori     r3, r0, 0x1
    li       r5, 0x0
    bl       GXSetZMode
.L_801B25EC:
    lwz      r3, 0x4(r25)
    xor      r0, r3, r15
    rlwinm.  r0, r0, 0, 7, 7
    beq      .L_801B2618
    rlwinm.  r0, r3, 0, 7, 7
    beq      .L_801B2610
    lwz      r3, particleFog@sda21(r0)
    bl       fn_801D37E0
    b        .L_801B2618
.L_801B2610:
    li       r3, 0x0
    bl       fn_801D37E0
.L_801B2618:
    lbz      r0, 0x8(r25)
    lis      r3, psFormGroupArray@ha
    addi     r3, r3, psFormGroupArray@l
    slwi     r0, r0, 2
    lwzx     r3, r3, r0
    cmplwi   r3, 0x0
    beq      .L_801B2664
    lbz      r0, 0x9(r25)
    slwi     r0, r0, 2
    lwzx     r4, r3, r0
    cmplwi   r4, 0x0
    beq      .L_801B2664
    addic.   r0, r4, 0x4
    beq      .L_801B2664
    lbz      r0, 0xa(r25)
    slwi     r3, r0, 2
    addi     r0, r3, 0x4
    lwzx     r21, r4, r0
    b        .L_801B2668
.L_801B2664:
    li       r21, 0x0
.L_801B2668:
    lwz      r3, 0x4(r25)
    rlwinm.  r0, r3, 0, 21, 21
    beq      .L_801B295C
    rlwinm.  r0, r3, 0, 26, 26
    beq      .L_801B2688
    lfs      f1, lbl_8065F994@sda21(r0)
    li       r16, 0x2
    b        .L_801B2690
.L_801B2688:
    lfs      f1, lbl_8065F998@sda21(r0)
    li       r16, 0x0
.L_801B2690:
    rlwinm.  r0, r3, 0, 25, 25
    beq      .L_801B26A4
    lfs      f2, lbl_8065F994@sda21(r0)
    li       r15, 0x2
    b        .L_801B26AC
.L_801B26A4:
    lfs      f2, lbl_8065F998@sda21(r0)
    li       r15, 0x0
.L_801B26AC:
    rlwinm   r0, r3, 0, 25, 26
    cmplw    r0, r23
    beq      .L_801B270C
    lfs      f3, lbl_8065F998@sda21(r0)
    mr       r23, r0
    addi     r3, r1, 0x70
    li       r24, 0x0
    bl       PSMTXScale
    lwz      r0, 0x4(r25)
    rlwinm.  r0, r0, 0, 25, 25
    beq      .L_801B26E0
    lfs      f0, lbl_8065F998@sda21(r0)
    stfs     f0, 0x8c(r1)
.L_801B26E0:
    addi     r3, r1, 0x70
    li       r4, 0x1e
    li       r5, 0x1
    bl       GXLoadTexMtxImm
    li       r3, 0x0
    li       r4, 0x1
    li       r5, 0x4
    li       r6, 0x1e
    li       r7, 0x0
    li       r8, 0x7d
    bl       GXSetTexCoordGen2
.L_801B270C:
    lbz      r4, 0x8(r25)
    lis      r3, psTexGroupArray@ha
    lbz      r0, 0x9(r25)
    addi     r3, r3, psTexGroupArray@l
    slwi     r4, r4, 2
    lwzx     r3, r3, r4
    slwi     r0, r0, 2
    lwzx     r3, r3, r0
    cmplwi   r3, 0x0
    beq      .L_801B2748
    lwz      r20, 0x4(r3)
    addi     r4, r3, 0x18
    lwz      r19, 0xc(r3)
    lwz      r18, 0x10(r3)
    b        .L_801B2758
.L_801B2748:
    li       r20, 0x0
    li       r18, 0x0
    li       r19, 0x0
    li       r4, 0x0
.L_801B2758:
    cmplwi   r4, 0x0
    beq      .L_801B2770
    lbz      r0, 0xa(r25)
    slwi     r0, r0, 2
    lwzx     r17, r4, r0
    b        .L_801B2774
.L_801B2770:
    li       r17, 0x0
.L_801B2774:
    cmpwi    r20, 0x8
    beq      .L_801B2784
    cmpwi    r20, 0x9
    bne      .L_801B2824
.L_801B2784:
    cmplwi   r4, 0x0
    beq      .L_801B2824
    lwz      r0, 0x0(r3)
    slwi     r0, r0, 2
    add.     r4, r4, r0
    beq      .L_801B2824
    lbz      r0, 0xb(r25)
    cmplwi   r0, 0xff
    beq      .L_801B27B4
    clrlslwi r0, r0, 24, 2
    lwzx     r4, r4, r0
    b        .L_801B27D4
.L_801B27B4:
    lwz      r0, 0x4(r25)
    rlwinm.  r0, r0, 0, 27, 27
    bne      .L_801B27D0
    lbz      r0, 0xa(r25)
    slwi     r0, r0, 2
    lwzx     r4, r4, r0
    b        .L_801B27D4
.L_801B27D0:
    lwz      r4, 0x0(r4)
.L_801B27D4:
    li       r0, 0x0
    cmplw    r4, r0
    beq      .L_801B2820
    lwz      r3, 0x8(r3)
    li       r0, 0x0
    cmpwi    r20, 0x8
    stw      r0, 0x48(r1)
    li       r0, 0x100
    stw      r3, 0x44(r1)
    bne      .L_801B2800
    li       r0, 0x10
.L_801B2800:
    sth      r0, 0x4c(r1)
    clrlwi   r6, r0, 16
    lwz      r5, 0x44(r1)
    addi     r3, r1, 0x34
    bl       GXInitTlutObj
    lwz      r4, 0x48(r1)
    addi     r3, r1, 0x34
    bl       GXLoadTlut
.L_801B2820:
    li       r24, 0x0
.L_801B2824:
    cmplw    r24, r17
    beq      .L_801B2910
    cmplwi   r17, 0x0
    beq      .L_801B2910
    cmpwi    r20, 0xa
    mr       r24, r17
    bge      .L_801B2858
    cmpwi    r20, 0x7
    beq      .L_801B28BC
    bge      .L_801B2864
    cmpwi    r20, 0x0
    bge      .L_801B2894
    b        .L_801B28BC
.L_801B2858:
    cmpwi    r20, 0xe
    beq      .L_801B2894
    b        .L_801B28BC
.L_801B2864:
    li       r0, 0x0
    mr       r4, r17
    stw      r0, 0x8(r1)
    mr       r7, r20
    mr       r8, r16
    mr       r9, r15
    addi     r3, r1, 0x50
    clrlwi   r5, r19, 16
    clrlwi   r6, r18, 16
    li       r10, 0x0
    bl       GXInitTexObjCI
    b        .L_801B28CC
.L_801B2894:
    mr       r4, r17
    mr       r7, r20
    mr       r8, r16
    mr       r9, r15
    addi     r3, r1, 0x50
    clrlwi   r5, r19, 16
    clrlwi   r6, r18, 16
    li       r10, 0x0
    bl       GXInitTexObj
    b        .L_801B28CC
.L_801B28BC:
    addi     r3, r30, 0xa4
    li       r4, 0x7eb
    li       r5, lbl_8065F99C@sda21
    bl       __assert
.L_801B28CC:
    lfs      f1, lbl_8065F990@sda21(r0)
    addi     r3, r1, 0x50
    lwz      r0, 0x4(r25)
    li       r6, 0x0
    fmr      f2, f1
    li       r7, 0x0
    rlwinm   r22, r0, 0, 22, 22
    extrwi   r0, r0, 1, 22
    xori     r5, r0, 0x1
    fmr      f3, f1
    cntlzw   r0, r22
    li       r8, 0x0
    srwi     r4, r0, 5
    bl       GXInitTexObjLOD
    addi     r3, r1, 0x50
    li       r4, 0x0
    bl       GXLoadTexObj
.L_801B2910:
    lwz      r0, 0x4(r25)
    rlwinm   r3, r0, 0, 22, 22
    cmplw    r22, r3
    beq      .L_801B295C
    lfs      f1, lbl_8065F990@sda21(r0)
    cntlzw   r0, r3
    srwi     r4, r0, 5
    mr       r22, r3
    fmr      f2, f1
    mr       r5, r4
    fmr      f3, f1
    addi     r3, r1, 0x50
    li       r6, 0x0
    li       r7, 0x0
    li       r8, 0x0
    bl       GXInitTexObjLOD
    addi     r3, r1, 0x50
    li       r4, 0x0
    bl       GXLoadTexObj
.L_801B295C:
    lwz      r3, 0x4(r25)
    rlwinm.  r0, r3, 0, 1, 1
    beq      .L_801B2A8C
    lwz      r0, 0x90(r25)
    cmplwi   r0, 0x0
    beq      .L_801B2980
    mr       r3, r25
    bl       psDispSubAPPSRTPoint
    b        .L_801B2AB4
.L_801B2980:
    rlwinm.  r0, r3, 0, 11, 11
    beq      .L_801B2998
    mr       r3, r25
    bl       psDispSubPointTrail
    mr       r25, r3
    b        .L_801B2AB4
.L_801B2998:
    lwz      r0, lbl_8065B8FC@sda21(r0)
    cmpwi    r0, 0x0
    beq      .L_801B29B4
    li       r0, 0x0
    li       r3, 0x0
    stw      r0, lbl_8065B8FC@sda21(r0)
    bl       GXSetCurrentMtx
.L_801B29B4:
    lfs      f1, 0x4c(r25)
    lfd      f0, lbl_8065F9A8@sda21(r0)
    fcmpo    cr0, f1, f0
    ble      .L_801B29CC
    lfs      f0, lbl_8065F9A0@sda21(r0)
    b        .L_801B29D4
.L_801B29CC:
    lfs      f0, lbl_8065F9A4@sda21(r0)
    fmuls    f0, f0, f1
.L_801B29D4:
    fctiwz   f0, f0
    lwz      r0, lbl_8065B938@sda21(r0)
    stfd     f0, 0xd0(r1)
    lwz      r3, 0xd4(r1)
    clrlwi   r4, r3, 24
    cmpw     r0, r4
    beq      .L_801B29FC
    stw      r4, lbl_8065B938@sda21(r0)
    li       r4, 0x5
    bl       GXSetPointSize
.L_801B29FC:
    lwz      r0, 0x4(r25)
    rlwinm.  r0, r0, 0, 21, 21
    beq      .L_801B2A38
    bl       GXClearVtxDesc
    li       r3, 0x9
    li       r4, 0x1
    bl       GXSetVtxDesc
    li       r3, 0xd
    li       r4, 0x2
    bl       GXSetVtxDesc
    li       r3, 0xb8
    li       r4, 0x0
    li       r5, 0x1
    bl       GXBegin
    b        .L_801B2A58
.L_801B2A38:
    bl       GXClearVtxDesc
    li       r3, 0x9
    li       r4, 0x1
    bl       GXSetVtxDesc
    li       r3, 0xb8
    li       r4, 0x1
    li       r5, 0x1
    bl       GXBegin
.L_801B2A58:
    lfs      f2, 0x48(r25)
    lis      r3, 0xcc01
    lfs      f1, 0x44(r25)
    lfs      f0, 0x40(r25)
    stfs     f0, -0x8000(r3)
    stfs     f1, -0x8000(r3)
    stfs     f2, -0x8000(r3)
    lwz      r0, 0x4(r25)
    rlwinm.  r0, r0, 0, 21, 21
    beq      .L_801B2AB4
    li       r0, 0x1
    stb      r0, -0x8000(r3)
    b        .L_801B2AB4
.L_801B2A8C:
    lwz      r0, 0x90(r25)
    cmplwi   r0, 0x0
    beq      .L_801B2AA8
    mr       r3, r25
    mr       r4, r21
    bl       psDispSubAppSRT
    b        .L_801B2AB4
.L_801B2AA8:
    mr       r3, r25
    mr       r4, r21
    bl       psDispSub
.L_801B2AB4:
    lwz      r15, 0x4(r25)
    lwz      r25, 0x0(r25)
.L_801B2ABC:
    cmplwi   r25, 0x0
    bne      .L_801B1D98
.L_801B2AC4:
    lwz      r3, 0xe4(r1)
    addi     r3, r3, 0x1
    mr       r0, r3
    stw      r3, 0xe4(r1)
    cmpwi    r0, 0x10
    blt      .L_801B1D38
    lwz      r0, 0xd8(r1)
    cmpwi    r0, 0x0
    bne      .L_801B2AF0
    li       r3, -0x1
    bl       HSD_StateInvalidate
.L_801B2AF0:
    psq_l    f31, 0x158(r1), 0, qr0
    lfd      f31, 0x150(r1)
    psq_l    f30, 0x148(r1), 0, qr0
    lfd      f30, 0x140(r1)
    lmw      r14, 0xf8(r1)
    lwz      r0, 0x164(r1)
    mtlr     r0
    addi     r1, r1, 0x160
    blr
*/
