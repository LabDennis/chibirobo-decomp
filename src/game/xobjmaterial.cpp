#include "game/xobj.h"
#include "game/hsd_dobj.h"

extern "C" {
#include <sysdolphin/class_alloc.h>
}

/**
 * @note Address: 0x800912B4
 * @note Size: 0xB8
 */
extern "C" void SetJointMaterialOpaque(HSD_JObj* joint, void*, s32)
{
	HSD_JObjModifyFlags(joint, 0x40000, 0x1C0000);
	for (HSD_DObj* draw = HSD_JObjGetDObj(joint); draw != NULL; draw = HSD_DObjGetNext(draw)) {
		HSD_DObjModifyFlags(draw, 2, 0xE);
		HSD_MObj* material = HSD_DObjGetMObj(draw);
		if (material != NULL) {
			HSD_MObjClearFlags(material, RENDER_XLU | RENDER_NO_ZUPDATE | RENDER_ZMODE_ALWAYS);
			HSD_MObjSetFlags(material, RENDER_ALPHA_MAT);
			HSD_MObjCompileTev(material);
		}
	}
}

struct TranslucentMaterial {
	CXObj* object;
	f32 alpha;
};

/**
 * @note Address: 0x8009136C
 * @note Size: 0x100
 */
extern "C" void SetJointMaterialTranslucent(HSD_JObj* joint, void* context, s32)
{
	const TranslucentMaterial& materialParams = *static_cast<const TranslucentMaterial*>(context);
	CXObj* object                             = materialParams.object;
	f32 alpha                                 = materialParams.alpha;
	HSD_JObjModifyFlags(joint, 0x80000, 0x1C0000);
	for (HSD_DObj* draw = HSD_JObjGetDObj(joint); draw != NULL; draw = HSD_DObjGetNext(draw)) {
		HSD_DObjModifyFlags(draw, 4, 0xE);
		HSD_MObj* material = HSD_DObjGetMObj(draw);
		if (material != NULL) {
			HSD_MObjSetFlags(material, RENDER_XLU | RENDER_NO_ZUPDATE);
			HSD_MObjSetFlags(material, RENDER_ALPHA_MAT);
			HSD_MObjCompileTev(material);
			if (material->mat != NULL)
				HSD_MObjSetAlpha(material, alpha * object->GetMObjAlpha(material));
		}
	}
}

/**
 * @note Address: 0x8009146C
 * @note Size: 0x60
 */
void XObj::SetOpaque()
{
	if (mJObj != NULL)
		HSD_JObjWalkTree(mJObj, SetJointMaterialOpaque, NULL);
	HSD_JObjModifyFlags(mJObj, 0x10000000, 0x70000000);
}

/**
 * @note Address: 0x800914CC
 * @note Size: 0x74
 */
void XObj::SetTranslucent(f32 alpha)
{
	TranslucentMaterial material = { this, alpha };
	if (mJObj != NULL)
		HSD_JObjWalkTree(mJObj, SetJointMaterialTranslucent, &material);
	HSD_JObjModifyFlags(mJObj, 0x20000000, 0x70000000);
}

/**
 * @note Address: 0x80091540
 * @note Size: 0x1EC
 */
extern "C" void ScaleJointMaterialDiffuse(HSD_JObj* joint, void* context, s32)
{
	f32 factor = *static_cast<const f32*>(context);
	HSD_JObjSetFlags(joint, 0x80);
	for (HSD_DObj* draw = HSD_JObjGetDObj(joint); draw != NULL; draw = HSD_DObjGetNext(draw)) {
		HSD_MObj* material = HSD_DObjGetMObj(draw);
		if (material != NULL) {
			HSD_MObjSetFlags(material, 4);
			if (material->mat != NULL && __fpclassifyf(factor) != FP_NAN) {
				material->mat->diffuse.r = (u8)(material->mat->diffuse.r * factor);
				material->mat->diffuse.g = (u8)(material->mat->diffuse.g * factor);
				material->mat->diffuse.b = (u8)(material->mat->diffuse.b * factor);
			}
			for (HSD_TObj* texture = HSD_MObjGetTObj(material); texture != NULL; texture = HSD_TObjGetNext(texture)) {
				texture->flags = (texture->flags & ~(TEX_LIGHTMAP_MASK | TEX_COLORMAP_MASK)) | TEX_LIGHTMAP_DIFFUSE | TEX_LIGHTMAP_AMBIENT
				               | TEX_COLORMAP_ADD;
			}
			HSD_MObjCompileTev(material);
		}
	}
}

/**
 * @note Address: 0x8009172C
 * @note Size: 0x58
 */
void XObj::EnableLighting(f32 factor)
{
	if (mJObj != NULL)
		HSD_JObjWalkTree(mJObj, ScaleJointMaterialDiffuse, &factor);
}

struct DestinationAlpha {
	BOOL enabled;
	u8 alpha;
};

/**
 * @note Address: 0x80091784
 * @note Size: 0xB0
 */
extern "C" void SetJointMaterialDestAlpha(HSD_JObj* joint, void* context, s32)
{
	const DestinationAlpha& settings = *static_cast<const DestinationAlpha*>(context);
	BOOL enabled                     = settings.enabled;
	u8 alpha                         = settings.alpha;
	for (HSD_DObj* draw = HSD_JObjGetDObj(joint); draw != NULL; draw = HSD_DObjGetNext(draw)) {
		HSD_MObj* material = HSD_DObjGetMObj(draw);
		if (material != NULL && material->pe != NULL) {
			if (enabled)
				material->pe->flags |= HSD_PE_DST_ALPHA_ENABLE | HSD_PE_ALPHA_UPDATE;
			else
				material->pe->flags &= ~(HSD_PE_DST_ALPHA_ENABLE | HSD_PE_ALPHA_UPDATE);
			material->pe->dst_alpha = alpha;
		}
	}
}

/**
 * @note Address: 0x80091834
 * @note Size: 0x54
 */
void XObj::SetDestinationAlpha(BOOL enabled, u8 value)
{
	DestinationAlpha settings = { enabled, value };
	if (mJObj != NULL)
		HSD_JObjWalkTree(mJObj, SetJointMaterialDestAlpha, &settings);
}

static const HSD_PEDesc sDefaultMaterialPE = {
	HSD_PE_COLOR_UPDATE | HSD_PE_ALPHA_UPDATE | HSD_PE_Z_TEST | HSD_PE_Z_UPDATE | HSD_PE_DITHER,
	0,
	255,
	0,
	GX_BM_NONE,
	GX_BL_SRCALPHA,
	GX_BL_INVSRCALPHA,
	GX_LO_SET,
	GX_LEQUAL,
	GX_GREATER,
	GX_AOP_AND,
	GX_LEQUAL,
};

static const HSD_PEDesc sMaterialPEPresets[] = {
	{
	    HSD_PE_ALPHA_UPDATE | HSD_PE_DST_ALPHA_ENABLE | HSD_PE_Z_COMP_BEFORE_TEX | HSD_PE_Z_TEST | HSD_PE_Z_UPDATE,
	    0,
	    0,
	    4,
	    GX_BM_NONE,
	    GX_BL_SRCALPHA,
	    GX_BL_INVSRCALPHA,
	    GX_LO_SET,
	    GX_LEQUAL,
	    GX_ALWAYS,
	    GX_AOP_AND,
	    GX_ALWAYS,
	},
	{
	    HSD_PE_COLOR_UPDATE | HSD_PE_ALPHA_UPDATE | HSD_PE_DST_ALPHA_ENABLE | HSD_PE_Z_COMP_BEFORE_TEX | HSD_PE_Z_TEST | HSD_PE_Z_UPDATE,
	    0,
	    255,
	    4,
	    GX_BM_NONE,
	    GX_BL_ONE,
	    GX_BL_ZERO,
	    GX_LO_SET,
	    GX_LEQUAL,
	    GX_GREATER,
	    GX_AOP_AND,
	    GX_LEQUAL,
	},
	{
	    HSD_PE_COLOR_UPDATE | HSD_PE_ALPHA_UPDATE | HSD_PE_DST_ALPHA_ENABLE | HSD_PE_Z_COMP_BEFORE_TEX | HSD_PE_Z_TEST | HSD_PE_Z_UPDATE,
	    0,
	    255,
	    4,
	    GX_BM_NONE,
	    GX_BL_SRCALPHA,
	    GX_BL_INVSRCALPHA,
	    GX_LO_SET,
	    GX_LEQUAL,
	    GX_GREATER,
	    GX_AOP_AND,
	    GX_LEQUAL,
	},
};

// joint walk callback for XObj::ApplyMaterialPresets, the args are five on/off flags
/**
 * @note Address: 0x80091888
 * @note Size: 0x3BC
 */
extern "C" void ApplyJointMaterialPEPresets(HSD_JObj* joint, void* context, s32)
{
	const u32* flags = static_cast<const u32*>(context);
	HSD_DObj* draw;
	u32 reset        = flags[0];
	u32 firstPreset  = flags[1];
	u32 secondPreset = flags[2];
	u32 thirdPreset  = flags[3];
	u32 toon         = flags[4];
	for (draw = HSD_JObjGetDObj(joint); draw != NULL; draw = HSD_DObjGetNext(draw)) {
		HSD_MObj* material = HSD_DObjGetMObj(draw);
		if (material != NULL) {
			if (reset) {
				if (material->pe == NULL)
					material->pe = (HSD_PEDesc*)hsdAllocMemPiece(sizeof(HSD_PEDesc));
				*material->pe = sDefaultMaterialPE;
				if (material->rendermode & RENDER_XLU)
					material->pe->type = GX_BM_BLEND;
				if (material->rendermode & RENDER_NO_ZUPDATE)
					material->pe->flags &= ~HSD_PE_Z_UPDATE;
				if (material->rendermode & RENDER_ZMODE_ALWAYS)
					material->pe->flags &= ~HSD_PE_Z_TEST;
			}
			if (firstPreset) {
				if (material->pe == NULL)
					material->pe = (HSD_PEDesc*)hsdAllocMemPiece(sizeof(HSD_PEDesc));
				*material->pe = sMaterialPEPresets[0];
			} else if (secondPreset) {
				if (material->pe == NULL)
					material->pe = (HSD_PEDesc*)hsdAllocMemPiece(sizeof(HSD_PEDesc));
				*material->pe = sMaterialPEPresets[1];
			} else if (thirdPreset) {
				if (material->pe == NULL)
					material->pe = (HSD_PEDesc*)hsdAllocMemPiece(sizeof(HSD_PEDesc));
				*material->pe = sMaterialPEPresets[2];
				if (material->rendermode & RENDER_XLU)
					material->pe->type = GX_BM_BLEND;
			}
			if (toon) {
				if (!(GetMaterialRenderModeFlags(material) & RENDER_TOON)) {
					HSD_MObjSetFlags(material, RENDER_TOON);
					HSD_MObjCompileTev(material);
				}
			} else {
				if (GetMaterialRenderModeFlags(material) & RENDER_TOON) {
					HSD_MObjClearFlags(material, RENDER_TOON);
					HSD_MObjCompileTev(material);
				}
			}
		}
	}
}
