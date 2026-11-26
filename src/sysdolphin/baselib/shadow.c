#include <sysdolphin/shadow.h>

#include <dolphin/gx/gx.h>
#include <dolphin/gx/gxfifo.h>
#include <dolphin/mtx/mtx.h>
#include <dolphin/mtx/vec.h>
#include <sysdolphin/cobj.h>
#include <sysdolphin/constants.h>
#include <sysdolphin/identity_mtx.h>
#include <sysdolphin/list.h>
#include <sysdolphin/memory.h>
#include <sysdolphin/mobj.h>
#include <sysdolphin/perf.h>
#include <sysdolphin/pobj.h>
#include <sysdolphin/tev.h>
#include <sysdolphin/tobj.h>
#include <sysdolphin/vec_orthogonal.h>
#include <float.h>
#include <string.h>

/**
 * @note Address: N/A
 */
extern inline f64 fabs(f64 x)
{
	return __fabs(x);
}

static void makeMatrix(HSD_Shadow* shadow);

static const char shadow_assert_file[] = "shadow.c";
static const char viewing_rect_position_assert[] = "position";

#define ASSERT_NAMED_MSG(line, cond, msg) ((cond) ? ((void)0) : __assert(shadow_assert_file, line, msg))
#define ASSERT_NAMED(line, cond) ASSERT_NAMED_MSG(line, cond, #cond)

enum { SHADOW_DEFAULT_SIZE = 256 };

HSD_ObjAllocData shadow_alloc_data;

static inline HSD_ObjAllocData* HSD_ShadowGetAllocData(void)
{
	return &shadow_alloc_data;
}

static inline HSD_TObj* makeShadowTObj(void)
{
	HSD_TObj* tobj;

	tobj            = HSD_TObjAlloc();
	tobj->src       = GX_TG_POS;
	tobj->wrap_s    = GX_CLAMP;
	tobj->wrap_t    = GX_CLAMP;
	tobj->flags     = TEX_COORD_SHADOW | TEX_LIGHTMAP_SHADOW | TEX_COLORMAP_MODULATE | TEX_ALPHAMAP_PASS;
	tobj->imagedesc = HSD_ImageDescAlloc();
	return tobj;
}

/**
 * @note Address: 0x801ABD58
 * @note Size: 0x30
 */
void HSD_ShadowInitAllocData(void)
{
	HSD_ObjAllocInit(&shadow_alloc_data, sizeof(HSD_Shadow), 4);
}

/**
 * @note Address: 0x801ABC4C
 * @note Size: 0x10C
 */
HSD_Shadow* HSD_ShadowAlloc(void)
{
	HSD_Shadow* shadow;

	shadow = HSD_ObjAlloc(HSD_ShadowGetAllocData());
	memset(shadow, 0, sizeof(HSD_Shadow));
	shadow->camera  = HSD_CObjAlloc();
	shadow->texture = makeShadowTObj();

	shadow->scaleS    = 0.5F;
	shadow->scaleT    = -0.5F;
	shadow->transS    = 0.5F;
	shadow->transT    = 0.5F;
	shadow->intensity = 0;

	shadow->texture->imagedesc->format = GX_TF_I8;
	shadow->texture->imagedesc->format = GX_TF_I8;
	shadow->texture->imagedesc->width  = SHADOW_DEFAULT_SIZE;
	shadow->texture->imagedesc->height = SHADOW_DEFAULT_SIZE;

	HSD_CObjSetViewportfx4(shadow->camera, 0.0F, SHADOW_DEFAULT_SIZE, 0.0F, SHADOW_DEFAULT_SIZE);
	HSD_CObjSetScissorx4(shadow->camera, 0, SHADOW_DEFAULT_SIZE, 0, SHADOW_DEFAULT_SIZE);

	return shadow;
}

/**
 * @note Address: 0x801ABB24
 * @note Size: 0x128
 */
void HSD_ShadowRemove(HSD_Shadow* shadow)
{
	HSD_CObj* cobj;
	HSD_TObj* tobj;

	if (shadow == NULL) {
		return;
	}
	if ((cobj = shadow->camera) != NULL) {
		if (ref_DEC(cobj)) {
			hsdDelete(cobj);
		}
	}
	HSD_ShadowDeleteObject(shadow, NULL);
	if (shadow->active) {
		HSD_MObjDeleteShadowTexture(shadow->texture);
	}
	if (shadow->texture->imagedesc->img_ptr != NULL) {
		HSD_Free(shadow->texture->imagedesc->img_ptr);
	}
	tobj = shadow->texture;
	HSD_ImageDescRemove(tobj->imagedesc);
	HSD_TObjRemove(tobj);
	HSD_ObjFree(HSD_ShadowGetAllocData(), shadow);
}

/**
 * @note Address: 0x801ABAB4
 * @note Size: 0x70
 */
void HSD_ShadowInit(HSD_Shadow* shadow)
{
	HSD_ImageDesc* imagedesc;

	ASSERT_NAMED(0x104, shadow);
	imagedesc = shadow->texture->imagedesc;
	GXSetTexCopySrc(0, 0, imagedesc->width, imagedesc->height);
	GXSetTexCopyDst(imagedesc->width, imagedesc->height, GX_CTF_R8, GX_FALSE);
}

/**
 * @note Address: 0x801AB95C
 * @note Size: 0x158
 */
void HSD_ShadowSetSize(HSD_Shadow* shadow, u16 width, u16 height)
{
	u32 size;
	HSD_ImageDesc* imgdesc;

	ASSERT_NAMED(0x12F, shadow);
	ASSERT_NAMED(0x130, width > 0);
	ASSERT_NAMED(0x131, height > 0);

	imgdesc = shadow->texture->imagedesc;
	if (imgdesc->img_ptr == NULL || imgdesc->width != width || imgdesc->height != height) {
		if (imgdesc->img_ptr != NULL) {
			HSD_Free(imgdesc->img_ptr);
		}
		size = GXGetTexBufferSize(width, height, GX_TF_I8, GX_FALSE, 0);
		ASSERT_NAMED(0x140, size > 0);

		imgdesc->img_ptr = HSD_Alloc(size);
		imgdesc->width   = width;
		imgdesc->height  = height;
		HSD_CObjSetViewportfx4(shadow->camera, 0.0F, width, 0.0F, height);
		HSD_CObjSetScissorx4(shadow->camera, 0, width, 0, height);
	}
}

HSD_Chan channel = {
	NULL,             // next
	GX_COLOR0A0,      // chan
	0,                // flags
	{ 0 },            // amb_color
	{ 0, 0, 0, 255 }, // mat_color
	FALSE,            // enable
	GX_SRC_REG,       // amb_src
	GX_SRC_REG,       // mat_src
	GX_LIGHT_NULL,    // light_mask
	GX_DF_CLAMP,      // diff_fn
	GX_AF_NONE,       // attn_fn
};

HSD_TevDesc tevstage = {
	NULL, 1, 0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0,
	{ {
		GX_TEV_ADD, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_RASC,
		GX_CS_SCALE_1, GX_TB_ZERO, GX_ENABLE, GX_TEVPREV,
		GX_TEV_ADD, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_RASA,
		GX_CS_SCALE_1, GX_TB_ZERO, GX_ENABLE, GX_TEVPREV,
	} },
};

HSD_PEDesc pedesc = {
	HSD_PE_COLOR_UPDATE | HSD_PE_Z_COMP_BEFORE_TEX, 0, 0, 0, GX_BM_NONE,
	GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET, GX_ALWAYS, GX_ALWAYS, GX_AOP_AND, GX_ALWAYS,
};

/**
 * @note Address: 0x801AB6BC
 * @note Size: 0x2A0
 */
void HSD_ShadowStartRender(HSD_Shadow* shadow)
{
	Mtx44 projection;
	HSD_CObj* camera;
	u8 intensity;
	HSD_ImageDesc* imgdesc;
	HSD_SList* objects;

	ASSERT_NAMED(0x189, shadow);
	ASSERT_NAMED(0x18A, shadow->camera);
	ASSERT_NAMED(0x18B, shadow->texture);
	ASSERT_NAMED(0x18C, shadow->texture->imagedesc);

	camera  = shadow->camera;
	imgdesc = shadow->texture->imagedesc;
	if (shadow->active) {
		HSD_StateInitTev();
		HSD_SetupTevStageAll(&tevstage);
		HSD_StateSetNumTexGens();
		HSD_SetupPEMode(0, &pedesc);
		channel.mat_color.r = 0xFF;
		channel.mat_color.g = 0xFF;
		channel.mat_color.b = 0xFF;
		HSD_SetupChannelAll(&channel);
		C_MTXOrtho(projection, 1.0F, 0.0F, 0.0F, 1.0F, -1.0F, 1.0F);
		GXSetProjection(projection, GX_ORTHOGRAPHIC);
		GXSetViewport(0.0F, 0.0F, imgdesc->width, imgdesc->height, 0.0F, 1.0F);
		GXSetScissor(0, 0, imgdesc->width, imgdesc->height);
		GXLoadPosMtxImm(HSD_identityMtx, GX_PNMTX0);
		HSD_PerfCurrentStat.nb_mtx_load++;
		GXSetCurrentMtx(GX_PNMTX0);
		HSD_ClearVtxDesc();
		GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
		GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_U8, 0);
		HSD_StateSetCullMode(GX_CULL_BACK);
		GXBegin(GX_QUADS, GX_VTXFMT0, 4);
		GXPosition2u8(0, 1);
		GXPosition2u8(1, 1);
		GXPosition2u8(1, 0);
		GXPosition2u8(0, 0);
		GXEnd();

		intensity           = shadow->intensity;
		channel.mat_color.r = intensity;
		channel.mat_color.g = intensity;
		channel.mat_color.b = intensity;
		HSD_SetupChannelAll(&channel);
		if (HSD_CObjGetEyeDistance(camera) >= FLT_EPSILON && HSD_CObjSetCurrent(camera)) {
			GXSetScissor(2, 2, imgdesc->width - 4, imgdesc->height - 4);
			for (objects = shadow->objects; objects != NULL; objects = objects->next) {
				HSD_JObjDispAll(objects->data, NULL, 5, RENDER_SHADOW);
			}
			HSD_CObjEndCurrent();
		}
	}
}

/**
 * @note Address: 0x801AB420
 * @note Size: 0x29C
 */
void HSD_ShadowEndRender(HSD_Shadow* shadow)
{
	HSD_ImageDesc* imgdesc;
	ASSERT_NAMED(0x244, shadow);
	imgdesc = shadow->texture->imagedesc;
	if (imgdesc->img_ptr == NULL) {
		HSD_ShadowSetSize(shadow, imgdesc->width, imgdesc->height);
	}
	GXCopyTex(imgdesc->img_ptr, GX_TRUE);
	GXPixModeSync();
	GXInvalidateTexAll();
	makeMatrix(shadow);
}

/**
 * @note Address: 0x801AB260
 * @note Size: 0x1C0
 */
void HSD_ShadowSetActive(HSD_Shadow* shadow, s32 active)
{
	ASSERT_NAMED(0x297, shadow);
	// skip if already in requested state
	if (shadow->active && active || !shadow->active && !active) {
		return;
	}

	shadow->active = active;
	if (active) {
		HSD_ImageDesc* imgdesc = shadow->texture->imagedesc;
		if (imgdesc->img_ptr == NULL) {
			HSD_ShadowSetSize(shadow, imgdesc->width, imgdesc->height);
		}
		HSD_MObjAddShadowTexture(shadow->texture);
	} else {
		HSD_MObjDeleteShadowTexture(shadow->texture);
	}
}

/**
 * @note Address: 0x801AB1B4
 * @note Size: 0xAC
 */
void HSD_ShadowAddObject(HSD_Shadow* shadow, HSD_Obj* obj)
{
	HSD_SList* cur;

	if (shadow == NULL || obj == NULL) {
		return;
	}
	for (cur = shadow->objects; cur != NULL; cur = cur->next) {
		if (cur->data == obj) {
			return;
		}
	}
	shadow->objects = HSD_SListPrepend(shadow->objects, obj);
	ref_INC(obj);
}

/**
 * @note Address: 0x801AB120
 * @note Size: 0x94
 */
void HSD_ShadowDeleteObject(HSD_Shadow* shadow, struct _HSD_JObj* jobj)
{
	if (shadow == NULL) {
		return;
	}
	if (jobj != NULL) {
		HSD_SList** link = &shadow->objects;
		while (*link != NULL) {
			if ((*link)->data == jobj) {
				HSD_JObjUnref(jobj);
				*link = HSD_SListRemove(*link);
				break;
			}
			link = &(*link)->next;
		}
	} else {
		while (shadow->objects != NULL) {
			HSD_JObjUnref(shadow->objects->data);
			shadow->objects = HSD_SListRemove(shadow->objects);
		}
	}
}

Mtx ShadowProjectionFallbackMtx = {
	{ 0.0F, 0.0F, 0.0F, 0.0F },
	{ 0.0F, 0.0F, 0.0F, 0.0F },
	{ 0.0F, 0.0F, 0.0F, 1.0F },
};

/**
 * @note Address: N/A
 * @note Size: 0x140
 */
static void makeMatrix(HSD_Shadow* shadow)
{
	Mtx Mprj;

	if (HSD_CObjGetEyeDistance(shadow->camera) < FLT_EPSILON) {
		PSMTXCopy(ShadowProjectionFallbackMtx, shadow->texture->mtx);
		return;
	}

	switch (HSD_CObjGetProjectionType(shadow->camera)) {
	case PROJ_PERSPECTIVE:
		C_MTXLightPerspective(Mprj, shadow->camera->projection_param.perspective.fov, shadow->camera->projection_param.perspective.aspect,
		                      shadow->scaleS, shadow->scaleT, shadow->transS, shadow->transT);
		break;

	case PROJ_FRUSTRUM:
		C_MTXLightFrustum(Mprj, shadow->camera->projection_param.frustrum.top, shadow->camera->projection_param.frustrum.bottom,
		                  shadow->camera->projection_param.frustrum.left, shadow->camera->projection_param.frustrum.right,
		                  shadow->camera->near, shadow->scaleS, shadow->scaleT, shadow->transS, shadow->transT);
		break;

	case PROJ_ORTHO:
		C_MTXLightOrtho(Mprj, shadow->camera->projection_param.ortho.top, shadow->camera->projection_param.ortho.bottom,
		                shadow->camera->projection_param.ortho.left, shadow->camera->projection_param.ortho.right, shadow->scaleS,
		                shadow->scaleT, shadow->transS, shadow->transT);
		break;

	default:
		ASSERT_NAMED(0x357, 0);
	}

	PSMTXConcat(Mprj, shadow->camera->view_mtx, shadow->texture->mtx);
}

f64 atan2(f64, f64);

/**
 * @note Address: 0x801AAF54
 * @note Size: 0x1CC
 */
void HSD_ShadowSetViewingRect(HSD_Shadow* shadow, f32 top, f32 bottom, f32 left, f32 right)
{
	HSD_CObj* camera;
	f32 distance;

	ASSERT_NAMED(0x371, shadow);

	camera   = shadow->camera;
	distance = HSD_CObjGetEyeDistance(camera);
	if (distance < FLT_EPSILON) {
		return;
	}

	switch (HSD_CObjGetProjectionType(camera)) {
	case PROJ_PERSPECTIVE: {
		f32 width, height;

		if (fabs(top) > fabs(bottom)) {
			width = fabs(top);
		} else {
			width = fabs(bottom);
		}
		if (fabs(left) > fabs(right)) {
			height = fabs(left);
		} else {
			height = fabs(right);
		}
		HSD_CObjSetAspect(camera, height / (width + FLT_EPSILON));
		HSD_CObjSetFov(camera, (f32)atan2(width, distance) * 57.29578F);
	} break;

	case PROJ_ORTHO:
		HSD_CObjSetOrtho(camera, top, bottom, left, right);
		break;

	case PROJ_FRUSTRUM: {
		f32 scale = HSD_CObjGetNear(camera) / distance;
		if (scale < FLT_EPSILON) {
			scale = FLT_EPSILON;
		}
		HSD_CObjSetFrustum(camera, scale * top, scale * bottom, scale * left, scale * right);
	} break;

	default:
		ASSERT_NAMED(0x39E, 0);
	}
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline BOOL TryNormalizeChecked(Vec* src, Vec* dst)
{
	if (src == NULL || dst == NULL) {
		return ~0;
	}
	if (__fabs(src->x) <= MSL_FLT_MIN && __fabs(src->y) <= MSL_FLT_MIN && __fabs(src->z) <= MSL_FLT_MIN) {
		return ~0;
	}
	PSVECNormalize(src, dst);
	return 0;
}

/**
 * @note Address: 0x801AAD48
 * @note Size: 0x20C
 */
void HSD_ViewingRectInit(HSD_ViewingRect* rect, Vec* position, Vec* interest, Vec* upvector, s32 perspective)
{
	ASSERT_NAMED(0x3BD, rect);

	rect->invalid = 0;
	rect->origin  = *position;
	PSVECSubtract(interest, position, &rect->eye_v);
	if (TryNormalizeChecked(&rect->eye_v, &rect->eye_vn) < 0) {
		rect->invalid = 1;
		return;
	}

	PSVECCrossProduct(&rect->eye_vn, upvector, &rect->right_v);
	if (TryNormalizeChecked(&rect->right_v, &rect->right_v) < 0) {
		HSD_VecGetOrthogonal(&rect->eye_vn, &rect->right_v);
		PSVECNormalize(&rect->right_v, &rect->right_v);
	}
	PSVECCrossProduct(&rect->right_v, &rect->eye_vn, &rect->up_v);
	rect->distance = PSVECMag(&rect->eye_v);

	rect->top = rect->right = -3.4028235E38F;
	rect->bottom = rect->left = 3.4028235E38F;
	rect->perspective         = perspective != 0;
}

/**
 * @note Address: 0x801AACCC
 * @note Size: 0x7C
 */
s32 HSD_ViewingRectCheck(HSD_ViewingRect* rect)
{
	ASSERT_NAMED(0x3DB, rect);
	return !rect->invalid && rect->top > rect->bottom && rect->right > rect->left;
}

/**
 * @note Address: 0x801AAABC
 * @note Size: 0x210
 */
void HSD_ViewingRectAddRect(HSD_ViewingRect* rect, Vec* position, f32 top, f32 bottom, f32 left, f32 right)
{
	f32 x, y, dot;
	Vec o2p, e2p;

	ASSERT_NAMED(0x401, rect);
	ASSERT_NAMED_MSG(0x402, position, viewing_rect_position_assert);

	if (rect->invalid) {
		return;
	}

	PSVECSubtract(position, &rect->origin, &o2p);
	dot = PSVECDotProduct(&o2p, &rect->eye_vn);
	if (rect->perspective) {
		f32 scale;

		if (dot <= 0.0F) {
			return;
		}
		scale = rect->distance / dot;
		PSVECScale(&o2p, &o2p, scale);
		PSVECSubtract(&o2p, &rect->eye_v, &e2p);
		x = PSVECDotProduct(&rect->right_v, &e2p);
		y = PSVECDotProduct(&rect->up_v, &e2p);

		top *= scale;
		bottom *= scale;
		left *= scale;
		right *= scale;
	} else {
		Vec tmp;
		PSVECScale(&rect->eye_vn, &tmp, dot);
		PSVECSubtract(&o2p, &tmp, &e2p);
		x = PSVECDotProduct(&rect->right_v, &e2p);
		y = PSVECDotProduct(&rect->up_v, &e2p);
	}

	if (x + right > rect->right) {
		rect->right = x + right;
	}
	if (x + left < rect->left) {
		rect->left = x + left;
	}
	if (y + top > rect->top) {
		rect->top = y + top;
	}
	if (y + bottom < rect->bottom) {
		rect->bottom = y + bottom;
	}
}
