#include <sysdolphin/cobj.h>

#include <dolphin/gx/gx.h>
#include <dolphin/mtx/mtx.h>
#include <dolphin/mtx/vec.h>
#include <kar/gr/grcoll.h>
#include <kar/math.h>
#include <sysdolphin/class_new.h>
#include <sysdolphin/constants.h>
#include <sysdolphin/displayfunc.h>
#include <sysdolphin/fobj_alloc.h>
#include <sysdolphin/gobjproc.h>
#include <sysdolphin/initialize.h>
#include <sysdolphin/objalloc.h>
#include <sysdolphin/video.h>
#include <sysdolphin/zlist.h>

#define FLT_MIN         1.17549435e-38F
#define GX_PERSPECTIVE  0
#define GX_ORTHOGRAPHIC 1
#define DegToRad(x)     ((x) * 0.017453292F)
#define COBJ_MTX_DIRTY  0x40000000
#define COBJ_INV_DIRTY  0x80000000

// only the current render mode is read here, the full struct lives in video.c
typedef struct _HSD_VICurrent {
	HSD_VIStatus vi;
	u8 chg_flag;
} HSD_VICurrent;

extern struct _HSD_VIInfo {
	HSD_VICurrent current;
} HSD_VIData;

static inline GXRenderModeObj* HSD_VIGetRenderMode(void)
{
	return &HSD_VIData.current.vi.rmode;
}

s32 lbl_8065AE98 = 640; /* reference width */
s32 lbl_8065AE9C = 480; /* reference height */

#define ASSERT_COBJ(line) ((cobj) ? ((void)0) : __assert("cobj.c", line, "cobj"))

extern f64 atan2(f64 y, f64 x);

// the msl math.h fabs, an inline over the intrinsic that calls the library fabs once inlining runs too deep
extern inline f64 fabs(f64 x)
{
	return __fabs(x);
}
extern MtxPtr HSD_MtxAlloc(void);
extern void HSD_MtxFree(MtxPtr mtx);
extern void C_MTXPerspective(Mtx44 mtx, f32 fov, f32 aspect, f32 near, f32 far);
extern void C_MTXFrustum(Mtx44 mtx, f32 top, f32 bottom, f32 left, f32 right, f32 near, f32 far);
extern void C_MTXOrtho(Mtx44 mtx, f32 top, f32 bottom, f32 left, f32 right, f32 near, f32 far);

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline f32 cobj_sqrtf(f32 x)
{
	volatile f32 y;
	if (x > 0.0F) {
		f64 guess = __frsqrte((f64)x);
		guess     = 0.5 * guess * (3.0 - guess * guess * x);
		guess     = 0.5 * guess * (3.0 - guess * guess * x);
		guess     = 0.5 * guess * (3.0 - guess * guess * x);
		y         = (f32)(x * guess);
		return y;
	}
	return x;
}

static f64 CObjGetHalfFov(HSD_CObj* cobj)
{
	return 0.5 * cobj->projection_param.perspective.fov;
}

static HSD_CObj* sCurrentCameraObject;     /* current */
static HSD_CObjInfo* sDefaultCameraClass; /* default_class */

int CObjLoad(HSD_CObj* cobj, HSD_CObjDesc* desc);
int CObjInit(HSD_Class* obj);
void CObjRelease(HSD_Class* obj);
void CObjAmnesia(HSD_ClassInfo* info);
void CObjInfoInit(void);
void CObjUpdateFunc(void* obj, enum_t type, HSD_ObjData* val);
extern Vec lbl_80259EA0; /* default up vector */

HSD_CObjInfo hsdCObj = { CObjInfoInit };

/**
 * @note Address: 0x801BF364
 * @note Size: 0x70
 */
void HSD_CObjSetReferenceSize(s32 ref_width, s32 ref_height)
{
	if (ref_width <= 0) {
		__assert("cobj.c", 0x2F, "ref_width > 0");
	}
	if (ref_height <= 0) {
		__assert("cobj.c", 0x30, "ref_height > 0");
	}
	lbl_8065AE98 = ref_width;
	lbl_8065AE9C = ref_height;
}

/**
 * @note Address: 0x801BF3D4
 * @note Size: 0x7C
 */
void HSD_CObjAddAnim(HSD_CObj* cobj, HSD_CameraAnim* anim)
{
	if (cobj != NULL && anim != NULL) {
		if (cobj->aobj != NULL) {
			HSD_AObjRemove(cobj->aobj);
		}
		cobj->aobj = HSD_AObjLoadDesc(anim->aobjdesc);
		HSD_WObjAddAnim(HSD_CObjGetEyePositionWObj(cobj), anim->eye_anim);
		HSD_WObjAddAnim(HSD_CObjGetInterestWObj(cobj), anim->interest_anim);
	}
}

/**
 * @note Address: 0x801BF450
 * @note Size: 0x144
 */
void CObjUpdateFunc(void* obj, enum_t type, HSD_ObjData* val)
{
	HSD_CObj* cobj = obj;
	Vec vec;

	if (cobj == NULL) {
		return;
	}

	switch (type) {
	case 1:
		HSD_CObjGetEyePosition(cobj, &vec);
		vec.x = val->fv;
		HSD_CObjSetEyePosition(cobj, &vec);
		break;
	case 2:
		HSD_CObjGetEyePosition(cobj, &vec);
		vec.y = val->fv;
		HSD_CObjSetEyePosition(cobj, &vec);
		break;
	case 3:
		HSD_CObjGetEyePosition(cobj, &vec);
		vec.z = val->fv;
		HSD_CObjSetEyePosition(cobj, &vec);
		break;
	case 5:
		HSD_CObjGetInterest(cobj, &vec);
		vec.x = val->fv;
		HSD_CObjSetInterest(cobj, &vec);
		break;
	case 6:
		HSD_CObjGetInterest(cobj, &vec);
		vec.x = val->fv;
		HSD_CObjSetInterest(cobj, &vec);
		break;
	case 7:
		HSD_CObjGetInterest(cobj, &vec);
		vec.x = val->fv;
		HSD_CObjSetInterest(cobj, &vec);
		break;
	case 9:
		HSD_CObjSetRoll(cobj, val->fv);
		break;
	case 10:
		HSD_CObjSetFov(cobj, val->fv);
		break;
	case 11:
		HSD_CObjSetNear(cobj, val->fv);
		break;
	case 12:
		HSD_CObjSetFar(cobj, val->fv);
		break;
	}
}

/**
 * @note Address: 0x801BF594
 * @note Size: 0x50
 */
void HSD_CObjAnim(HSD_CObj* cobj)
{
	if (cobj != NULL) {
		HSD_AObjInterpretAnim(cobj->aobj, cobj, HSD_COBJ_METHOD(cobj)->update);
		HSD_WObjInterpretAnim(cobj->eye_position);
		HSD_WObjInterpretAnim(cobj->interest);
	}
}

/**
 * @note Address: 0x801BF5E4
 * @note Size: 0x5C
 */
void HSD_CObjReqAnim(HSD_CObj* cobj, f32 frame)
{
	if (cobj == NULL) {
		return;
	}
	if (cobj == NULL) {
		return;
	}
	HSD_AObjReqAnim(cobj->aobj, frame);
	HSD_WObjReqAnim(cobj->eye_position, frame);
	HSD_WObjReqAnim(cobj->interest, frame);
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline s32 makeProjectionMtx(HSD_CObj* cobj, Mtx44 mtx)
{
	s32 projection_type;

	switch (cobj->projection_type) {
	case PROJ_PERSPECTIVE:
		projection_type = GX_PERSPECTIVE;
		C_MTXPerspective(mtx, cobj->projection_param.perspective.fov, cobj->projection_param.perspective.aspect, cobj->near, cobj->far);
		break;
	case PROJ_FRUSTRUM:
		projection_type = GX_PERSPECTIVE;
		C_MTXFrustum(mtx, cobj->projection_param.frustrum.top, cobj->projection_param.frustrum.bottom, cobj->projection_param.frustrum.left,
		             cobj->projection_param.frustrum.right, cobj->near, cobj->far);
		break;
	case PROJ_ORTHO:
		projection_type = GX_ORTHOGRAPHIC;
		C_MTXOrtho(mtx, cobj->projection_param.ortho.top, cobj->projection_param.ortho.bottom, cobj->projection_param.ortho.left,
		           cobj->projection_param.ortho.right, cobj->near, cobj->far);
		break;
	}
	return projection_type;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline s32 vec_normalize_check(Vec* src, Vec* dst)
{
	if (!src || !dst) {
		return -1;
	}
	if (fabs(src->x) <= FLT_MIN && fabs(src->y) <= FLT_MIN && fabs(src->z) <= FLT_MIN) {
		return -1;
	}
	PSVECNormalize(src, dst);
	return 0;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static BOOL setupOffscreenCamera(HSD_CObj* cobj)
{
	Mtx44 mtx;

	GXSetViewport(cobj->viewport_left, cobj->viewport_top, cobj->viewport_right - cobj->viewport_left,
	              cobj->viewport_bottom - cobj->viewport_top, 0.0F, 1.0F);
	GXSetScissor(cobj->scissor_left, cobj->scissor_top, cobj->scissor_right - cobj->scissor_left, cobj->scissor_bottom - cobj->scissor_top);
	GXSetProjection(mtx, makeProjectionMtx(cobj, mtx));
	return TRUE;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static BOOL setupNormalCamera(HSD_CObj* cobj)
{
	Mtx44 mtx;
	s32 projection_type;
	f64 x_scale;
	f64 y_scale;
	f32 top;
	f32 bottom;
	f32 left;
	f32 right;
	f32 width;
	f32 height;
	GXRenderModeObj* rmode = HSD_VIGetRenderMode();

	x_scale = rmode->fbWidth / (f64)lbl_8065AE98;
	y_scale = rmode->efbHeight / (f64)lbl_8065AE9C;

	left   = cobj->viewport_left * x_scale;
	right  = cobj->viewport_right * x_scale;
	top    = cobj->viewport_top * y_scale;
	bottom = cobj->viewport_bottom * y_scale;
	width  = right - left;
	height = bottom - top;
	if (rmode->field_rendering) {
		GXSetViewportJitter(left, top, width, height, 0.0F, 1.0F, VIGetNextField());
	} else {
		GXSetViewport(left, top, width, height, 0.0F, 1.0F);
	}

	left   = cobj->scissor_left * x_scale;
	right  = cobj->scissor_right * x_scale;
	top    = cobj->scissor_top * y_scale;
	bottom = cobj->scissor_bottom * y_scale;
	width  = right - left;
	height = bottom - top;
	GXSetScissor(left, top, width, height);

	projection_type = makeProjectionMtx(cobj, mtx);
	GXSetProjection(mtx, projection_type);
	return TRUE;
}

/**
 * @note Address: 0x801BF640
 * @note Size: 0x264
 */
BOOL setupTopHalfCamera(HSD_CObj* cobj)
{
	Mtx44 mtx;
	s32 projection_type;
	f32 h_scale;
	f32 rad;
	f32 t;
	f32 w;
	f32 b;
	f32 top;
	f32 bottom;
	f32 left;
	f32 right;
	f32 width;
	f32 height;
	GXRenderModeObj* rmode = HSD_VIGetRenderMode();

	if (cobj->viewport_top >= rmode->efbHeight) {
		return FALSE;
	}

	left   = cobj->viewport_left;
	right  = cobj->viewport_right;
	top    = cobj->viewport_top;
	bottom = cobj->viewport_bottom < rmode->efbHeight ? cobj->viewport_bottom : rmode->efbHeight;
	width  = right - left;
	height = bottom - top;
	GXSetScissor(left, top, width, height);

	top     = cobj->viewport_top;
	bottom  = cobj->viewport_bottom;
	left    = cobj->viewport_left;
	right   = cobj->viewport_right;
	height  = (bottom < rmode->efbHeight ? bottom : rmode->efbHeight) - top;
	h_scale = height / (bottom - top);
	width   = right - left;

	GXSetViewport(left, top, width, height, 0.0F, 1.0F);

	switch (cobj->projection_type) {
	case PROJ_PERSPECTIVE:
		projection_type = GX_PERSPECTIVE;
		rad             = DegToRad(CObjGetHalfFov(cobj));
		t               = cobj->near * (f32)tan(rad);
		w               = t * cobj->projection_param.perspective.aspect;
		b               = t * (1.0F - 2.0F * h_scale);
		C_MTXFrustum(mtx, t, b, -w, w, cobj->near, cobj->far);
		break;
	case PROJ_FRUSTRUM:
		projection_type = GX_PERSPECTIVE;
		C_MTXFrustum(mtx, cobj->projection_param.frustrum.top,
		             cobj->projection_param.frustrum.top
		                 - h_scale * (cobj->projection_param.frustrum.top - cobj->projection_param.frustrum.bottom),
		             cobj->projection_param.frustrum.left, cobj->projection_param.frustrum.right, cobj->near, cobj->far);
		break;
	case PROJ_ORTHO:
		projection_type = GX_ORTHOGRAPHIC;
		C_MTXOrtho(mtx, cobj->projection_param.ortho.top,
		           cobj->projection_param.ortho.top - h_scale * (cobj->projection_param.ortho.top - cobj->projection_param.ortho.bottom),
		           cobj->projection_param.ortho.left, cobj->projection_param.ortho.right, cobj->near, cobj->far);
		break;
	}
	GXSetProjection(mtx, projection_type);
	return TRUE;
}

/**
 * @note Address: 0x801BF8A4
 * @note Size: 0x2CC
 */
BOOL setupBottomHalfCamera(HSD_CObj* cobj)
{
	s32 projection_type;
	Mtx44 mtx;
	f32 top;
	f32 bottom;
	f32 left;
	f32 right;
	f32 width;
	f32 height;
	f32 h_scale;
	f32 t;
	f32 b;
	f32 w;
	f32 h;
	f32 rad;
	u32 screen_top;
	GXRenderModeObj* rmode = HSD_VIGetRenderMode();

	screen_top = rmode->efbHeight - 8;
	if (cobj->viewport_bottom < screen_top) {
		return FALSE;
	}

	top    = cobj->scissor_top;
	left   = cobj->scissor_left;
	right  = cobj->scissor_right;
	top    = (cobj->scissor_top > screen_top ? cobj->scissor_top : screen_top) - screen_top;
	bottom = cobj->scissor_bottom - screen_top;
	width  = right - left;
	height = bottom - top;
	GXSetScissor(left, top, width, height);

	top    = cobj->viewport_top;
	left   = cobj->viewport_left;
	right  = cobj->viewport_right;
	top    = top > screen_top ? top : screen_top;
	bottom = cobj->viewport_bottom;
	width  = right - left;
	height = bottom - cobj->viewport_top;

	t       = top - screen_top;
	h       = (bottom - screen_top) - t;
	h_scale = h / height;
	GXSetViewport(left, t, width, h, 0.0F, 1.0F);

	switch (cobj->projection_type) {
	case PROJ_PERSPECTIVE:
		projection_type = GX_PERSPECTIVE;
		rad             = DegToRad(CObjGetHalfFov(cobj));
		b               = cobj->near * (f32)tan(rad);
		w               = b * cobj->projection_param.perspective.aspect;
		t               = b * (2.0F * h_scale + -1.0F);
		C_MTXFrustum(mtx, t, -b, -w, w, cobj->near, cobj->far);
		break;
	case PROJ_FRUSTRUM:
		projection_type = GX_PERSPECTIVE;
		C_MTXFrustum(mtx,
		             h_scale * (cobj->projection_param.frustrum.top - cobj->projection_param.frustrum.bottom)
		                 + cobj->projection_param.frustrum.bottom,
		             cobj->projection_param.frustrum.bottom, cobj->projection_param.frustrum.left, cobj->projection_param.frustrum.right,
		             cobj->near, cobj->far);
		break;
	case PROJ_ORTHO:
		projection_type = GX_ORTHOGRAPHIC;
		C_MTXOrtho(mtx,
		           h_scale * (cobj->projection_param.ortho.top - cobj->projection_param.ortho.bottom) + cobj->projection_param.ortho.bottom,
		           cobj->projection_param.ortho.bottom, cobj->projection_param.ortho.left, cobj->projection_param.ortho.right, cobj->near,
		           cobj->far);
		break;
	}
	GXSetProjection(mtx, projection_type);
	return TRUE;
}

/**
 * @note Address: 0x801BFB70
 * @note Size: 0xCC
 */
void HSD_CObjSetupViewingMtx(HSD_CObj* cobj)
{
	Vec eye;
	Vec up;
	Vec interest;

	if ((cobj->flags & 2) == 0 && HSD_CObjMtxIsDirty(cobj)) {
		HSD_CObjGetEyePosition(cobj, &eye);
		if (HSD_CObjGetUpVector(cobj, &up) == 0) {
			up.x = 0.0F;
			up.y = 1.0F;
			up.z = 0.0F;
		}
		HSD_CObjGetInterest(cobj, &interest);
		C_MTXLookAt(cobj->view_mtx, &eye, &up, &interest);
		cobj->eye_position->flags &= ~2;
		cobj->interest->flags &= ~2;
		HSD_CObjClearFlags(cobj, COBJ_MTX_DIRTY);
		HSD_CObjSetFlags(cobj, COBJ_INV_DIRTY);
	}
}

/**
 * @note Address: 0x801BFC3C
 * @note Size: 0x504
 */
BOOL HSD_CObjSetCurrent(HSD_CObj* cobj)
{
	HSD_RenderPass render_pass;
	BOOL result;

	if (cobj == NULL) {
		return FALSE;
	}

	render_pass = HSD_GetCurrentRenderPass();
	_HSD_ZListClear();
	sCurrentCameraObject = cobj;

	switch (render_pass) {
	case HSD_RP_OFFSCREEN:
		result = setupOffscreenCamera(cobj);
		break;
	case HSD_RP_SCREEN:
		result = setupNormalCamera(cobj);
		break;
	case HSD_RP_TOPHALF:
		result = setupTopHalfCamera(cobj);
		break;
	case HSD_RP_BOTTOMHALF:
		result = setupBottomHalfCamera(cobj);
		break;
	default:
		HSD_Panic("cobj.c", 0x297, "unkown type of render pass.\n");
		return FALSE;
	}

	if (!result) {
		return FALSE;
	}
	HSD_CObjSetupViewingMtx(cobj);
	return TRUE;
}

/**
 * @note Address: 0x801C0140
 * @note Size: 0x24
 */
void HSD_CObjEndCurrent(void)
{
	_HSD_ZListSort();
	_HSD_ZListDisp();
}

/**
 * @note Address: 0x801C0164
 * @note Size: 0x40
 */
HSD_WObj* HSD_CObjGetInterestWObj(HSD_CObj* cobj)
{
	ASSERT_COBJ(0x2BC);
	return cobj->interest;
}

/**
 * @note Address: 0x801C01A4
 * @note Size: 0x40
 */
HSD_WObj* HSD_CObjGetEyePositionWObj(HSD_CObj* cobj)
{
	ASSERT_COBJ(0x2D4);
	return cobj->eye_position;
}

/**
 * @note Address: 0x801C01E4
 * @note Size: 0x6C
 */
void HSD_CObjGetInterest(HSD_CObj* cobj, Vec* interest)
{
	ASSERT_COBJ(0x2EC);
	HSD_WObjGetPosition(HSD_CObjGetInterestWObj(cobj), interest);
}

/**
 * @note Address: 0x801C0250
 * @note Size: 0x6C
 */
void HSD_CObjSetInterest(HSD_CObj* cobj, Vec* interest)
{
	ASSERT_COBJ(0x2F8);
	HSD_WObjSetPosition(HSD_CObjGetInterestWObj(cobj), interest);
}

/**
 * @note Address: 0x801C02BC
 * @note Size: 0x6C
 */
void HSD_CObjGetEyePosition(HSD_CObj* cobj, Vec* position)
{
	ASSERT_COBJ(0x304);
	HSD_WObjGetPosition(HSD_CObjGetEyePositionWObj(cobj), position);
}

/**
 * @note Address: 0x801C0328
 * @note Size: 0x6C
 */
void HSD_CObjSetEyePosition(HSD_CObj* cobj, Vec* position)
{
	ASSERT_COBJ(0x310);
	HSD_WObjSetPosition(HSD_CObjGetEyePositionWObj(cobj), position);
}

/**
 * @note Address: N/A
 * @note Size: 0x184
 */
s32 HSD_CObjGetEyeVector(HSD_CObj* cobj, Vec* eye)
{
	Vec pos;
	Vec interest;

	if (cobj == NULL || cobj->eye_position == NULL || cobj->interest == NULL) {
		return FALSE;
	}
	HSD_CObjGetEyePosition(cobj, &pos);
	HSD_CObjGetInterest(cobj, &interest);
	PSVECSubtract(&interest, &pos, eye);
	return vec_normalize_check(eye, eye) == 0;
}

/**
 * @note Address: 0x801C0394
 * @note Size: 0x104
 */
f32 HSD_CObjGetEyeDistance(HSD_CObj* cobj)
{
	Vec pos;
	Vec interest;
	Vec eye;

	if (cobj == NULL) {
		return 0.0F;
	}
	if (cobj->eye_position == NULL) {
		__assert("cobj.c", 0x33F, "cobj->eyepos");
	}
	if (cobj->interest == NULL) {
		__assert("cobj.c", 0x340, "cobj->interest");
	}
	HSD_CObjGetEyePosition(cobj, &pos);
	HSD_CObjGetInterest(cobj, &interest);
	PSVECSubtract(&interest, &pos, &eye);
	return PSVECMag(&eye);
}

static Vec lbl_80259E50 = { 0.0F, 0.0F, 0.0F };
static Vec lbl_80259E5C = { 0.0F, 1.0F, 0.0F };

/**
 * @note Address: 0x801C0498
 * @note Size: 0x1E8
 */
f32 upvec2roll(HSD_CObj* cobj, Vec* up)
{
	Vec v;
	Vec eye;
	Mtx mtx;
	f32 dot;
	f32 x;
	f32 y;

	if (!HSD_CObjGetEyeVector(cobj, &eye)) {
		return 0.0F;
	}
	dot = PSVECDotProduct(up, &eye);
	dot = 1.0F - (f32)__fabs(dot);
	if (dot < FLT_MIN) {
		return 0.0F;
	}
	C_MTXLookAt(mtx, &lbl_80259E50, &lbl_80259E5C, &eye);
	PSMTXMultVecSR(mtx, up, &v);
	x = -v.x;
	y = v.y;
	if (y == 0.0F) {
		return x >= 0.0F ? 1.5707964F : -1.5707964F;
	}
	return atan2(x, y);
}

/**
 * @note Address: 0x801C0680
 * @note Size: 0x2E4
 */
s32 roll2upvec(HSD_CObj* cobj, Vec* up, f32 roll)
{
	Vec eye;
	Vec v0;
	Vec v1;
	Mtx mtx;

	if (!HSD_CObjGetEyeVector(cobj, &eye)) {
		return FALSE;
	}
	if (1.0 - (f32)fabs(eye.y) < 0.0001) {
		v0.x = cobj_sqrtf(eye.y * eye.y + eye.z * eye.z);
		v0.y = eye.y * (-eye.x / v0.x);
		v0.z = eye.z * (-eye.x / v0.x);
	} else {
		v0.y = cobj_sqrtf(eye.x * eye.x + eye.z * eye.z);
		v0.x = eye.x * (-eye.y / v0.y);
		v0.z = eye.z * (-eye.y / v0.y);
	}
	PSMTXRotAxisRad(mtx, &eye, -roll);
	PSMTXMultVecSR(mtx, &v0, &v1);
	PSVECNormalize(&v1, up);
	return TRUE;
}

#pragma push
#pragma dont_inline on
/**
 * @note Address: 0x801C0964
 * @note Size: 0x68
 */
s32 HSD_CObjGetUpVector(HSD_CObj* cobj, Vec* up)
{
	if (cobj == NULL || up == NULL) {
		return FALSE;
	}
	if (cobj->flags & 1) {
		*up = cobj->u.up;
		return TRUE;
	}
	return roll2upvec(cobj, up, cobj->u.roll);
}
#pragma pop

/**
 * @note Address: 0x801C09CC
 * @note Size: 0x2FC
 */
void HSD_CObjSetUpVector(HSD_CObj* cobj, Vec* up)
{
	Vec normalized;

	if (!cobj || !up) {
		return;
	}
	if (cobj->flags & 1) {
		if (vec_normalize_check(up, &normalized) != 0) {
			OSReport("illegal up vector.");
			__assert("cobj.c", 0x3D0, "0");
		}
		if (cobj->u.up.x != normalized.x || cobj->u.up.y != normalized.y || cobj->u.up.z != normalized.z) {
			HSD_CObjSetMtxDirty(cobj);
			cobj->u.up = normalized;
		}
	} else {
		HSD_CObjSetRoll(cobj, upvec2roll(cobj, up));
	}
}

static char lbl_dead_805300XX_hsdIsDescendantOf[] = "hsdIsDescendantOf(info, &hsdCObj)";

/**
 * @note Address: N/A
 * @note Size: 0x23C
 */
s32 HSD_CObjGetLeftVector(HSD_CObj* cobj, Vec* left)
{
	Vec eye;
	Vec up;

	if (cobj == NULL || left == NULL) {
		return FALSE;
	}
	if (HSD_CObjGetEyeVector(cobj, &eye) && HSD_CObjGetUpVector(cobj, &up)) {
		PSVECCrossProduct(&up, &eye, left);
		return vec_normalize_check(left, left) == 0;
	}
	return FALSE;
}

/**
 * @note Address: 0x801C0CC8
 * @note Size: 0x10
 */
void HSD_CObjSetMtxDirty(HSD_CObj* cobj)
{
	cobj->flags |= COBJ_MTX_DIRTY | COBJ_INV_DIRTY;
}

/**
 * @note Address: 0x801C0CD8
 * @note Size: 0x4C
 */
BOOL HSD_CObjMtxIsDirty(HSD_CObj* cobj)
{
	return (cobj->flags & COBJ_MTX_DIRTY) || (cobj->eye_position != NULL && (cobj->eye_position->flags & 2))
	    || (cobj->interest != NULL && (cobj->interest->flags & 2));
}

/**
 * @note Address: 0x801C0D24
 * @note Size: 0x34
 */
void HSD_CObjGetViewingMtx(HSD_CObj* cobj, Mtx mtx)
{
	PSMTXCopy(HSD_CObjGetViewingMtxPtr(cobj), mtx);
}

/**
 * @note Address: 0x801C0D58
 * @note Size: 0x64
 */
MtxPtr HSD_CObjGetInvViewingMtxPtrDirect(HSD_CObj* cobj)
{
	if (cobj->flags & COBJ_INV_DIRTY) {
		if (cobj->proj_mtx == NULL) {
			cobj->proj_mtx = HSD_MtxAlloc();
		}
		PSMTXInverse(cobj->view_mtx, cobj->proj_mtx);
		HSD_CObjClearFlags(cobj, COBJ_INV_DIRTY);
	}
	return cobj->proj_mtx;
}

#pragma push
#pragma dont_inline on
/**
 * @note Address: 0x801C0DBC
 * @note Size: 0x30
 */
MtxPtr HSD_CObjGetViewingMtxPtr(HSD_CObj* cobj)
{
	HSD_CObjSetupViewingMtx(cobj);
	return cobj->view_mtx;
}
#pragma pop

/**
 * @note Address: 0x801C0DEC
 * @note Size: 0x218
 */
f32 GetCameraRoll(HSD_CObj* cobj)
{
	Vec up;

	if (cobj == NULL) {
		return 0.0F;
	}
	if (cobj->flags & 1) {
		if (HSD_CObjGetUpVector(cobj, &up) == 0) {
			return 0.0F;
		}
		return upvec2roll(cobj, &up);
	}
	return cobj->u.roll;
}

/**
 * @note Address: 0x801C1004
 * @note Size: 0x428
 */
void HSD_CObjSetRoll(HSD_CObj* cobj, f32 roll)
{
	Vec up;

	if (cobj == NULL) {
		return;
	}
	if (cobj->flags & 1) {
		roll2upvec(cobj, &up, roll);
		HSD_CObjSetUpVector(cobj, &up);
	} else {
		if (cobj->u.roll != roll) {
			HSD_CObjSetMtxDirty(cobj);
		}
		cobj->u.roll = roll;
	}
}

/**
 * @note Address: 0x801C142C
 * @note Size: 0x24
 */
f32 HSD_CObjGetFov(HSD_CObj* cobj)
{
	if (cobj == NULL || cobj->projection_type != PROJ_PERSPECTIVE) {
		return 0.0F;
	}
	return cobj->projection_param.perspective.fov;
}

/**
 * @note Address: 0x801C1450
 * @note Size: 0x20
 */
void HSD_CObjSetFov(HSD_CObj* cobj, f32 fov)
{
	if (cobj == NULL || cobj->projection_type != PROJ_PERSPECTIVE) {
		return;
	}
	cobj->projection_param.perspective.fov = fov;
}

/**
 * @note Address: N/A
 * @note Size: 0x24
 */
f32 HSD_CObjGetAspect(HSD_CObj* cobj)
{
	if (cobj == NULL || cobj->projection_type != PROJ_PERSPECTIVE) {
		return 0.0F;
	}
	return cobj->projection_param.perspective.aspect;
}

/**
 * @note Address: 0x801C1470
 * @note Size: 0x20
 */
void HSD_CObjSetAspect(HSD_CObj* cobj, f32 aspect)
{
	if (cobj == NULL || cobj->projection_type != PROJ_PERSPECTIVE) {
		return;
	}
	cobj->projection_param.perspective.aspect = aspect;
}

/**
 * @note Address: 0x801C1490
 * @note Size: 0x34
 */
void HSD_CObjSetTop(HSD_CObj* cobj, f32 top)
{
	if (cobj == NULL) {
		return;
	}
	switch (cobj->projection_type) {
	case PROJ_PERSPECTIVE:
		break;
	case PROJ_FRUSTRUM:
		cobj->projection_param.frustrum.top = top;
		break;
	case PROJ_ORTHO:
		cobj->projection_param.ortho.top = top;
		break;
	}
}

/**
 * @note Address: 0x801C14C4
 * @note Size: 0x34
 */
void HSD_CObjSetBottom(HSD_CObj* cobj, f32 bottom)
{
	if (cobj == NULL) {
		return;
	}
	switch (cobj->projection_type) {
	case PROJ_PERSPECTIVE:
		break;
	case PROJ_FRUSTRUM:
		cobj->projection_param.frustrum.bottom = bottom;
		break;
	case PROJ_ORTHO:
		cobj->projection_param.ortho.bottom = bottom;
		break;
	}
}

/**
 * @note Address: 0x801C14F8
 * @note Size: 0x34
 */
void HSD_CObjSetLeft(HSD_CObj* cobj, f32 left)
{
	if (cobj == NULL) {
		return;
	}
	switch (cobj->projection_type) {
	case PROJ_PERSPECTIVE:
		break;
	case PROJ_FRUSTRUM:
		cobj->projection_param.frustrum.left = left;
		break;
	case PROJ_ORTHO:
		cobj->projection_param.ortho.left = left;
		break;
	}
}

/**
 * @note Address: 0x801C152C
 * @note Size: 0x34
 */
void HSD_CObjSetRight(HSD_CObj* cobj, f32 right)
{
	if (cobj == NULL) {
		return;
	}
	switch (cobj->projection_type) {
	case PROJ_PERSPECTIVE:
		break;
	case PROJ_FRUSTRUM:
		cobj->projection_param.frustrum.right = right;
		break;
	case PROJ_ORTHO:
		cobj->projection_param.ortho.right = right;
		break;
	}
}

/**
 * @note Address: 0x801C1560
 * @note Size: 0x18
 */
f32 HSD_CObjGetNear(HSD_CObj* cobj)
{
	if (cobj == NULL) {
		return 0.0F;
	}
	return cobj->near;
}

/**
 * @note Address: 0x801C1578
 * @note Size: 0x10
 */
void HSD_CObjSetNear(HSD_CObj* cobj, f32 near)
{
	if (cobj != NULL) {
		cobj->near = near;
	}
}

/**
 * @note Address: 0x801C1588
 * @note Size: 0x18
 */
f32 HSD_CObjGetFar(HSD_CObj* cobj)
{
	if (cobj == NULL) {
		return 0.0F;
	}
	return cobj->far;
}

/**
 * @note Address: 0x801C15A0
 * @note Size: 0x10
 */
void HSD_CObjSetFar(HSD_CObj* cobj, f32 far)
{
	if (cobj != NULL) {
		cobj->far = far;
	}
}

/**
 * @note Address: N/A
 * @note Size: 0x1C
 */
void HSD_CObjGetScissor(HSD_CObj* cobj, Scissor* scissor)
{
	if (cobj != NULL) {
		*scissor = cobj->scissor;
	}
}

/**
 * @note Address: N/A
 * @note Size: 0x1C
 */
void HSD_CObjSetScissor(HSD_CObj* cobj, Scissor* scissor)
{
	if (cobj != NULL) {
		cobj->scissor = *scissor;
	}
}

/**
 * @note Address: 0x801C15B0
 * @note Size: 0x1C
 */
// fn_801C15B0

/**
 * @note Address: 0x801C15CC
 * @note Size: 0x1C
 */
void HSD_CObjSetScissorx4(HSD_CObj* cobj, s32 left, u16 right, s32 top, u16 bottom)
{
	if (cobj != NULL) {
		cobj->scissor_left   = left;
		cobj->scissor_right  = right;
		cobj->scissor_top    = top;
		cobj->scissor_bottom = bottom;
	}
}

/**
 * @note Address: N/A
 * @note Size: 0x64
 */
void HSD_CObjGetViewport(HSD_CObj* cobj, HSD_RectS16* viewport)
{
	if (cobj != NULL) {
		viewport->xmin = cobj->viewport_left;
		viewport->xmax = cobj->viewport_right;
		viewport->ymin = cobj->viewport_top;
		viewport->ymax = cobj->viewport_bottom;
	}
}

/**
 * @note Address: N/A
 * @note Size: 0x2C
 */
void HSD_CObjGetViewportf(HSD_CObj* cobj, HSD_RectF32* viewport)
{
	if (cobj != NULL) {
		*viewport = cobj->viewport;
	}
}

/**
 * @note Address: 0x801C15E8
 * @note Size: 0x8C
 */
void HSD_CObjSetViewport(HSD_CObj* cobj, HSD_RectS16* viewport)
{
	if (cobj != NULL) {
		cobj->viewport_left   = viewport->xmin;
		cobj->viewport_right  = viewport->xmax;
		cobj->viewport_top    = viewport->ymin;
		cobj->viewport_bottom = viewport->ymax;
	}
}

/**
 * @note Address: 0x801C1674
 * @note Size: 0x2C
 */
void HSD_CObjSetViewportf(HSD_CObj* cobj, HSD_RectF32* viewport)
{
	if (cobj != NULL) {
		cobj->viewport = *viewport;
	}
}

/**
 * @note Address: 0x801C16A0
 * @note Size: 0x1C
 */
void HSD_CObjSetViewportfx4(HSD_CObj* cobj, f32 left, f32 right, f32 top, f32 bottom)
{
	if (cobj != NULL) {
		cobj->viewport_left   = left;
		cobj->viewport_right  = right;
		cobj->viewport_top    = top;
		cobj->viewport_bottom = bottom;
	}
}

/**
 * @note Address: 0x801C16BC
 * @note Size: 0x18
 */
s32 HSD_CObjGetProjectionType(HSD_CObj* cobj)
{
	if (cobj == NULL) {
		return PROJ_PERSPECTIVE;
	}
	return cobj->projection_type;
}

/**
 * @note Address: 0x801C16D4
 * @note Size: 0x10
 */
void HSD_CObjSetProjectionType(HSD_CObj* cobj, u32 projection_type)
{
	if (cobj != NULL) {
		cobj->projection_type = projection_type;
	}
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
void HSD_CObjSetPerspective(HSD_CObj* cobj, f32 fov, f32 aspect)
{
	if (cobj != NULL) {
		cobj->projection_type                     = PROJ_PERSPECTIVE;
		cobj->projection_param.perspective.fov    = fov;
		cobj->projection_param.perspective.aspect = aspect;
	}
}

/**
 * @note Address: 0x801C16E4
 * @note Size: 0x24
 */
void HSD_CObjSetFrustum(HSD_CObj* cobj, f32 top, f32 bottom, f32 left, f32 right)
{
	if (cobj != NULL) {
		cobj->projection_type                  = PROJ_FRUSTRUM;
		cobj->projection_param.frustrum.top    = top;
		cobj->projection_param.frustrum.bottom = bottom;
		cobj->projection_param.frustrum.left   = left;
		cobj->projection_param.frustrum.right  = right;
	}
}

/**
 * @note Address: 0x801C1708
 * @note Size: 0x24
 */
void HSD_CObjSetOrtho(HSD_CObj* cobj, f32 top, f32 bottom, f32 left, f32 right)
{
	if (cobj != NULL) {
		cobj->projection_type               = PROJ_ORTHO;
		cobj->projection_param.ortho.top    = top;
		cobj->projection_param.ortho.bottom = bottom;
		cobj->projection_param.ortho.left   = left;
		cobj->projection_param.ortho.right  = right;
	}
}

/**
 * @note Address: 0x801C172C
 * @note Size: 0x8
 */
u32 HSD_CObjGetFlags(HSD_CObj* cobj)
{
	return cobj->flags;
}

/**
 * @note Address: 0x801C1734
 * @note Size: 0x18
 */
void HSD_CObjSetFlags(HSD_CObj* cobj, u32 flags)
{
	if (cobj != NULL) {
		cobj->flags |= flags;
	}
}

/**
 * @note Address: 0x801C174C
 * @note Size: 0x18
 */
void HSD_CObjClearFlags(HSD_CObj* cobj, u32 flags)
{
	if (cobj != NULL) {
		cobj->flags &= ~flags;
	}
}

/**
 * @note Address: 0x801C1764
 * @note Size: 0x8
 */
HSD_CObj* HSD_CObjGetCurrent(void)
{
	return sCurrentCameraObject;
}

/**
 * @note Address: 0x801C176C
 * @note Size: 0x5C
 */
HSD_CObj* HSD_CObjAlloc(void)
{
	HSD_CObj* cobj;

	cobj = hsdNew((HSD_ClassInfo*)(sDefaultCameraClass != NULL ? sDefaultCameraClass : &hsdCObj));
	ASSERT_COBJ(0x790);
	return cobj;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void CObjResetFlags(HSD_CObj* cobj, u32 flags)
{
	if (cobj == NULL) {
		return;
	}
	cobj->flags = (cobj->flags & (COBJ_MTX_DIRTY | COBJ_INV_DIRTY)) | (flags & ~(COBJ_MTX_DIRTY | COBJ_INV_DIRTY));
}

/**
 * @note Address: 0x801C17C8
 * @note Size: 0x22C
 */
int CObjLoad(HSD_CObj* cobj, HSD_CObjDesc* desc)
{
	cobj->flags = desc->flags;
	CObjResetFlags(cobj, desc->flags);
	HSD_CObjSetViewport(cobj, &desc->viewport);
	HSD_CObjSetScissor(cobj, &desc->scissor);
	HSD_WObjInit(cobj->eye_position, desc->eye_desc);
	HSD_WObjInit(cobj->interest, desc->interest_desc);
	HSD_CObjSetNear(cobj, desc->near);
	HSD_CObjSetFar(cobj, desc->far);
	if (desc->flags & 1) {
		if (desc->vector != NULL) {
			HSD_CObjSetUpVector(cobj, desc->vector);
		} else {
			HSD_CObjSetUpVector(cobj, &lbl_80259EA0);
		}
	} else {
		HSD_CObjSetRoll(cobj, desc->roll);
	}

	switch (desc->projection_type) {
	case PROJ_PERSPECTIVE:
		HSD_CObjSetPerspective(cobj, desc->projection_param.perspective.fov, desc->projection_param.perspective.aspect);
		break;
	case PROJ_ORTHO:
		HSD_CObjSetOrtho(cobj, desc->projection_param.ortho.top, desc->projection_param.ortho.bottom, desc->projection_param.ortho.left,
		                 desc->projection_param.ortho.right);
		break;
	case PROJ_FRUSTRUM:
		HSD_CObjSetFrustum(cobj, desc->projection_param.frustrum.top, desc->projection_param.frustrum.bottom,
		                   desc->projection_param.frustrum.left, desc->projection_param.frustrum.right);
		break;
	default:
		__assert("cobj.c", 0x7BE, "0");
		break;
	}
	return 0;
}

/**
 * @note Address: 0x801C19F4
 * @note Size: 0x34
 */
void HSD_CObjInit(HSD_CObj* cobj, HSD_CObjDesc* desc)
{
	if (cobj == NULL || desc == NULL) {
		return;
	}
	CObjLoad(cobj, desc);
}

/**
 * @note Address: 0x801C1A28
 * @note Size: 0xC4
 */
HSD_CObj* HSD_CObjLoadDesc(HSD_CObjDesc* desc)
{
	HSD_ClassInfo* info;
	HSD_CObj* cobj;

	if (desc != NULL) {
		if (desc->class_name == NULL || (info = hsdSearchClassInfo(desc->class_name)) == NULL) {
			cobj = HSD_CObjAlloc();
		} else {
			cobj = hsdNew(info);
			ASSERT_COBJ(0x7E5);
		}
		HSD_COBJ_METHOD(cobj)->load(cobj, desc);
		return cobj;
	}
	return NULL;
}

/**
 * @note Address: 0x801C1AEC
 * @note Size: 0x74
 */
int CObjInit(HSD_Class* obj)
{
	HSD_CObj* cobj;
	s32 result;

	result = HSD_OBJECT_PARENT_INFO(&hsdCObj)->init(obj);
	if (result < 0) {
		return result;
	}

	cobj = (HSD_CObj*)obj;
	if (cobj != NULL) {
		HSD_CObjSetMtxDirty(cobj);
	}
	cobj->eye_position = HSD_WObjAlloc();
	cobj->interest     = HSD_WObjAlloc();
	return 0;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline bool ref_DEC(void* o)
{
	bool ret;
	if ((ret = (HSD_OBJ(o)->ref_count == HSD_OBJ_NOREF))) {
		return ret;
	}
	return HSD_OBJ(o)->ref_count-- == 0;
}

/**
 * @note Address: 0x801C1B60
 * @note Size: 0x184
 */
void CObjRelease(HSD_Class* obj)
{
	HSD_CObj* cobj;
	HSD_WObj* eye_position;
	HSD_WObj* interest;

	cobj = (HSD_CObj*)obj;
	HSD_AObjRemove(cobj->aobj);
	eye_position = HSD_CObjGetEyePositionWObj(cobj);
	if (eye_position != NULL) {
		if (ref_DEC(eye_position) && eye_position != NULL) {
			HSD_CLASS_METHOD(eye_position)->release((HSD_Class*)eye_position);
			HSD_CLASS_METHOD(eye_position)->destroy((HSD_Class*)eye_position);
		}
	}
	interest = HSD_CObjGetInterestWObj(cobj);
	if (interest != NULL) {
		if (ref_DEC(interest) && interest != NULL) {
			HSD_CLASS_METHOD(interest)->release((HSD_Class*)interest);
			HSD_CLASS_METHOD(interest)->destroy((HSD_Class*)interest);
		}
	}
	if (cobj->proj_mtx != NULL) {
		HSD_MtxFree(cobj->proj_mtx);
	}
	HSD_OBJECT_PARENT_INFO(&hsdCObj)->release(obj);
}

/**
 * @note Address: 0x801C1CE4
 * @note Size: 0x60
 */
void CObjAmnesia(HSD_ClassInfo* info)
{
	if (info == HSD_CLASS_INFO(sDefaultCameraClass)) {
		sDefaultCameraClass = NULL;
	}
	if (info == HSD_CLASS_INFO(&hsdCObj)) {
		sCurrentCameraObject = NULL;
	}
	HSD_OBJECT_PARENT_INFO(&hsdCObj)->amnesia(info);
}

Vec lbl_80259EA0 = { 0.0F, 1.0F, 0.0F };

/**
 * @note Address: 0x801C1D44
 * @note Size: 0x8C
 */
void CObjInfoInit(void)
{
	hsdInitClassInfo(HSD_CLASS_INFO(&hsdCObj), &hsdObj, "sysdolphin_base_library", "hsd_cobj", sizeof(HSD_CObjInfo), sizeof(HSD_CObj));
	HSD_CLASS_INFO(&hsdCObj)->init    = CObjInit;
	HSD_CLASS_INFO(&hsdCObj)->release = CObjRelease;
	HSD_CLASS_INFO(&hsdCObj)->amnesia = CObjAmnesia;
	hsdCObj.load                      = CObjLoad;
	hsdCObj.update                    = CObjUpdateFunc;
}

/**
 * @note Address: N/A
 * @note Size: 0xC
 */
HSD_ObjAllocData* HSD_FObjGetAllocData(void)
{
	return &fobj_alloc_data;
}
