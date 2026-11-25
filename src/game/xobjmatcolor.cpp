#include "game/hsd_jobj.h"
#include "game/hsd_dobj.h"

// joint walk callback, args are the diffuse and ambient colors and the alpha
/**
 * @note Address: 0x800970E4
 * @note Size: 0xE0
 */
extern "C" void SetJointMaterialColorsAndAlpha(HSD_JObj* joint, void* context, s32)
{
	const JointMaterialColors& colors = *static_cast<const JointMaterialColors*>(context);
	GXColor diffuse                   = *colors.diffuse;
	GXColor ambient                   = *colors.ambient;
	f32 alpha                         = *colors.alpha;
	for (HSD_DObj* draw = HSD_JObjGetDObj(joint); draw != NULL; draw = HSD_DObjGetNext(draw)) {
		HSD_MObj* material = HSD_DObjGetMObj(draw);
		if (material != NULL) {
			HSD_MObjSetDiffuseColor(material, diffuse.r, diffuse.g, diffuse.b);
			SetMaterialAmbientColor(material, ambient.r, ambient.g, ambient.b);
			HSD_MObjSetAlpha(material, alpha);
		}
	}
}

// joint walk callback, makes every material translucent
/**
 * @note Address: 0x800971C4
 * @note Size: 0xC0
 */
extern "C" void EnableJointMaterialTranslucency(HSD_JObj* joint, void*, s32)
{
	HSD_JObjModifyFlags(joint, 0x20080000, 0x701C0000);
	for (HSD_DObj* draw = HSD_JObjGetDObj(joint); draw != NULL; draw = HSD_DObjGetNext(draw)) {
		HSD_DObjModifyFlags(draw, 4, 0xE);
		HSD_MObj* material = HSD_DObjGetMObj(draw);
		if (material != NULL) {
			HSD_MObjSetFlags(material, 0x60000000);
			HSD_MObjClearFlags(material, 8);
		}
		if (draw != NULL)
			HSD_MObjCompileTev(draw->mobj);
	}
}
