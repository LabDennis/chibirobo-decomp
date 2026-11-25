#include <sysdolphin/fog.h>

#include <dolphin/gx/gx.h>
#include <string.h>
#include <sysdolphin/cobj.h>
#include <sysdolphin/gobjproc.h>

/**
 * @note Address: 0x801D37E0
 * @note Size: 0x2BC
 */
void HSD_FogSet(HSD_Fog* fog)
{
	HSD_FogAdj* fog_adj;
	HSD_CObj* cobj;
	u32 flags;
	s32 range;
	s32 width;
	Mtx44Ptr mtx;
	GXFogAdjTable tbl;
	f32 v[6];

	if (fog == NULL) {
		GXColor color = { 0 };
		GXSetFog(GX_FOG_NONE, 0.0F, 0.0F, 0.0F, 0.0F, color);
		return;
	}

	cobj = HSD_CObjGetCurrent();
	if (cobj == NULL) {
		HSD_Panic(__FILE__, 0x58, "You must specify CObj first.\n");
	}

	GXSetFog(fog->type, fog->start, fog->end, HSD_CObjGetNear(cobj), HSD_CObjGetFar(cobj), fog->color);

	fog_adj = (fog != NULL) ? fog->fog_adj : NULL;
	flags   = HSD_FogAdjGetFlags(fog_adj);
	if (fog_adj != NULL && (flags & 7)) {
		Mtx44 local_mtx;

		GXGetViewportv(v);

		if (flags & 1) {
			s32 center = (fog_adj != NULL) ? fog_adj->center : -1;
			range      = v[0] + v[2] * (center + 320) / 640.0F;
		} else {
			range = v[0] + v[2] / 2;
		}

		if (flags & 2) {
			width = (fog_adj != NULL) ? fog_adj->width : -1;
		} else {
			width = v[2];
		}

		if (flags & 4) {
			mtx = (fog_adj != NULL) ? fog_adj->mtx : NULL;
		} else {
			f32 proj[7];

			mtx = local_mtx;
			memset(mtx, 0, sizeof(Mtx44));
			GXGetProjectionv(proj);
			switch ((s32)proj[0]) {
			case GX_PERSPECTIVE:
				mtx[0][0] = proj[1];
				mtx[0][2] = proj[2];
				mtx[1][1] = proj[3];
				mtx[1][2] = proj[4];
				mtx[2][2] = proj[5];
				mtx[2][3] = proj[6];
				mtx[3][2] = -1.0F;
				break;
			default:
				mtx[0][0] = proj[1];
				mtx[0][3] = proj[2];
				mtx[1][1] = proj[3];
				mtx[1][3] = proj[4];
				mtx[2][2] = proj[5];
				mtx[2][3] = proj[6];
				mtx[3][3] = 1.0F;
				break;
			}
		}

		GXInitFogAdjTable(&tbl, width, mtx);
		GXSetFogRangeAdj(GX_TRUE, range, &tbl);
	} else {
		GXSetFogRangeAdj(GX_FALSE, 0, NULL);
	}
}

/**
 * @note Address: 0x801D3A9C
 * @note Size: 0x18
 */
u32 HSD_FogAdjGetFlags(HSD_FogAdj* fog_adj)
{
	return (fog_adj != NULL) ? fog_adj->flags : 0;
}
