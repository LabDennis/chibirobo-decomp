#include <global.h>
#include <string.h>
#include <dolphin/gx/gx.h>
#include <dolphin/mtx/mtx.h>
#include <dolphin/mtx/vec.h>
#include <sysdolphin/tobj.h>
#include <sysdolphin/pobj.h>
#include <sysdolphin/class.h>

#include <sysdolphin/aobj.h>
#include <sysdolphin/class_alloc.h>
#include <sysdolphin/class_new.h>
#include <sysdolphin/cobj.h>
#include <sysdolphin/gobjproc.h>
#include <sysdolphin/initialize.h>
#include <sysdolphin/lobj.h>
#include <sysdolphin/memory.h>
#include <sysdolphin/mtx.h>
#include <sysdolphin/tev_state.h>
#include <sysdolphin/tev_stage.h>
#include <sysdolphin/util.h>

#define TEVCONF_MODE 1

#define TOBJ_ASSERT(line, cond, str) ((cond) ? ((void)0) : __assert("tobj.c", line, str))

void TObjInfoInit(void);

HSD_TObjInfo hsdTObj = { TObjInfoInit };

static HSD_TObjInfo* default_class = NULL;

static HSD_TObj* tobj_head;

void TObjAmnesia(HSD_ClassInfo* info);
void TObjRelease(HSD_Class* o);
int TObjInit(HSD_Class* o);
void TObjMakeTExp(HSD_TObj* tobj, u32 lightmap, u32 lightmap_done, HSD_TExp** c, HSD_TExp** a, HSD_TExp** list);
void MakeColorGenTExp(u32 lightmap, HSD_TObj* tobj, HSD_TExp** c, HSD_TExp** a, HSD_TExp** list, int repeat);
void TObjSetupMtx(HSD_TObj* tobj);
void MakeTextureMtx(HSD_TObj* tobj);
void TObjUpdateFunc(void* obj, enum_t type, HSD_ObjData* val);

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void TObjRemoveAnim(HSD_TObj* tobj)
{
	if (tobj == NULL) {
		return;
	}
	HSD_AObjRemove(tobj->aobj);
	tobj->aobj = NULL;
}

/**
 * @note Address: 0x801B1C3C
 * @note Size: 0x60
 */
void HSD_TObjRemoveAnimAll(HSD_TObj* tobj)
{
	HSD_TObj* tp;

	if (tobj != NULL) {
		for (tp = tobj; tp != NULL; tp = tp->next) {
			TObjRemoveAnim(tp);
		}
	}
}

/**
 * @note Address: N/A
 * @note Size: 0x2C
 */
static inline void HSD_TlutRemove(HSD_Tlut* tlut)
{
	if (tlut) {
		hsdFreeMemPiece(tlut, sizeof(HSD_Tlut));
	}
}

/**
 * @note Address: N/A
 * @note Size: 0x4C
 */
static inline HSD_Tlut* HSD_TlutLoadDesc(HSD_TlutDesc* tlutdesc)
{
	if (tlutdesc != NULL) {
		HSD_Tlut* tlut = HSD_TlutAlloc();
		memcpy(tlut, tlutdesc, sizeof(HSD_Tlut));
		return tlut;
	}
	return NULL;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline HSD_TexAnim* lookupTextureAnim(s32 id, HSD_TexAnim* texanim)
{
	HSD_TexAnim* ta;
	for (ta = texanim; ta; ta = ta->next) {
		if (ta->id == (GXTexMapID)id) {
			return ta;
		}
	}
	return NULL;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void TObjAddAnim(HSD_TObj* tobj, HSD_TexAnim* texanim)
{
	s32 i;
	HSD_TexAnim* ta;

	if (tobj != NULL) {
		if ((ta = lookupTextureAnim(tobj->id, texanim)) != NULL) {
			if (tobj->aobj != NULL) {
				HSD_AObjRemove(tobj->aobj);
			}
			tobj->aobj     = HSD_AObjLoadDesc(ta->aobjdesc);
			tobj->imagetbl = ta->imagetbl;

			if (tobj->tluttbl != NULL) {
				for (i = 0; tobj->tluttbl[i]; i++) {
					HSD_TlutRemove(tobj->tluttbl[i]);
				}
				HSD_Free(tobj->tluttbl);
			}

			if (ta->n_tluttbl) {
				tobj->tluttbl = (HSD_Tlut**)HSD_Alloc((s32)sizeof(HSD_Tlut*) * (ta->n_tluttbl + 1));
				for (i = 0; i < ta->n_tluttbl; i++) {
					tobj->tluttbl[i] = HSD_TlutLoadDesc(ta->tluttbl[i]);
				}
				tobj->tluttbl[i] = NULL;
			} else {
				tobj->tluttbl = NULL;
			}
			tobj->tlut_no = (u8)-1;
		}
	}
}

/**
 * @note Address: 0x801B1A98
 * @note Size: 0x1A4
 */
void HSD_TObjAddAnimAll(HSD_TObj* tobj, HSD_TexAnim* texanim)
{
	HSD_TObj* tp;

	if (tobj != NULL) {
		for (tp = tobj; tp; tp = tp->next) {
			TObjAddAnim(tp, texanim);
		}
	}
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void TObjReqAnimByFlags(HSD_TObj* tobj, f32 startframe, u32 flags)
{
	if (tobj == NULL) {
		return;
	}
	if (flags & TOBJ_ANIM) {
		HSD_AObjReqAnim(tobj->aobj, startframe);
	}
}

/**
 * @note Address: 0x801B1A24
 * @note Size: 0x74
 */
void HSD_TObjReqAnimAllByFlags(HSD_TObj* tobj, f32 startframe, u32 flags)
{
	HSD_TObj* tp;

	if (tobj != NULL) {
		for (tp = tobj; tp != NULL; tp = tp->next) {
			TObjReqAnimByFlags(tp, startframe, flags);
		}
	}
}

/**
 * @note Address: 0x801B1458
 * @note Size: 0x5CC
 */
void TObjUpdateFunc(void* obj, enum_t type, HSD_ObjData* val)
{
	HSD_TObj* tobj = obj;

	if (tobj == NULL) {
		return;
	}

	switch (type) {
	case HSD_A_T_TIMG: {
		int n;
		TOBJ_ASSERT(0x169, tobj->imagetbl, "tobj->imagetbl");
		n = (int)val->fv;
		if (tobj->imagetbl[n]) {
			tobj->imagedesc = tobj->imagetbl[n];
		}
	} break;
	case HSD_A_T_TCLT: {
		if (tobj->tluttbl) {
			tobj->tlut_no = (u8)val->fv;
		}
	} break;
	case HSD_A_T_BLEND:
		tobj->blending = HSD_ClampFloat(val->fv, 0.0F, 1.0F);
		break;
	case HSD_A_T_ROTX:
		tobj->rotate.x = val->fv;
		goto mtxdirty;
	case HSD_A_T_ROTY:
		tobj->rotate.y = val->fv;
		goto mtxdirty;
	case HSD_A_T_ROTZ:
		tobj->rotate.z = val->fv;
		goto mtxdirty;
	case HSD_A_T_TRAU:
		tobj->translate.x = val->fv;
		goto mtxdirty;
	case HSD_A_T_TRAV:
		tobj->translate.y = val->fv;
		goto mtxdirty;
	case HSD_A_T_SCAU:
		tobj->scale.x = val->fv;
		goto mtxdirty;
	case HSD_A_T_SCAV:
		tobj->scale.y = val->fv;
		goto mtxdirty;
	mtxdirty:
		tobj->flags |= TEX_MTX_DIRTY;
		break;
	case HSD_A_T_LOD_BIAS:
		if (tobj->lod) {
			tobj->lod->LODBias = val->fv;
		}
		break;
	case HSD_A_T_KONST_R:
		if (tobj->tev) {
			tobj->tev->konst.r = (u8)(255.0F * HSD_ClampFloat(val->fv, 0.0F, 1.0F));
		}
		break;
	case HSD_A_T_KONST_G:
		if (tobj->tev) {
			tobj->tev->konst.g = (u8)(255.0F * HSD_ClampFloat(val->fv, 0.0F, 1.0F));
		}
		break;
	case HSD_A_T_KONST_B:
		if (tobj->tev) {
			tobj->tev->konst.b = (u8)(255.0F * HSD_ClampFloat(val->fv, 0.0F, 1.0F));
		}
		break;
	case HSD_A_T_KONST_A:
		if (tobj->tev) {
			tobj->tev->konst.a = (u8)(255.0F * HSD_ClampFloat(val->fv, 0.0F, 1.0F));
		}
		break;
	case HSD_A_T_TEV0_R:
		if (tobj->tev) {
			tobj->tev->tev0.r = (u8)(255.0F * HSD_ClampFloat(val->fv, 0.0F, 1.0F));
		}
		break;
	case HSD_A_T_TEV0_G:
		if (tobj->tev) {
			tobj->tev->tev0.g = (u8)(255.0F * HSD_ClampFloat(val->fv, 0.0F, 1.0F));
		}
		break;
	case HSD_A_T_TEV0_B:
		if (tobj->tev) {
			tobj->tev->tev0.b = (u8)(255.0F * HSD_ClampFloat(val->fv, 0.0F, 1.0F));
		}
		break;
	case HSD_A_T_TEV0_A:
		if (tobj->tev) {
			tobj->tev->tev0.a = (u8)(255.0F * HSD_ClampFloat(val->fv, 0.0F, 1.0F));
		}
		break;
	case HSD_A_T_TEV1_R:
		if (tobj->tev) {
			tobj->tev->tev1.r = (u8)(255.0F * HSD_ClampFloat(val->fv, 0.0F, 1.0F));
		}
		break;
	case HSD_A_T_TEV1_G:
		if (tobj->tev) {
			tobj->tev->tev1.g = (u8)(255.0F * HSD_ClampFloat(val->fv, 0.0F, 1.0F));
		}
		break;
	case HSD_A_T_TEV1_B:
		if (tobj->tev) {
			tobj->tev->tev1.b = (u8)(255.0F * HSD_ClampFloat(val->fv, 0.0F, 1.0F));
		}
		break;
	case HSD_A_T_TEV1_A:
		if (tobj->tev) {
			tobj->tev->tev1.a = (u8)(255.0F * HSD_ClampFloat(val->fv, 0.0F, 1.0F));
		}
		break;
	case HSD_A_T_TS_BLEND:
		tobj->blending = HSD_ClampFloat(val->fv, 0.0F, 1.0F);
		break;
	}
}

/**
 * @note Address: N/A
 * @note Size: 0x38
 */
static inline void HSD_TObjInterpretAnim(HSD_TObj* tobj)
{
	if (tobj == NULL) {
		return;
	}

	HSD_AObjInterpretAnim(tobj->aobj, tobj, HSD_TOBJ_METHOD(tobj)->update);
}

/**
 * @note Address: 0x801B13FC
 * @note Size: 0x5C
 */
void HSD_TObjAnimAll(HSD_TObj* tobj)
{
	HSD_TObj* i;

	if (tobj == NULL) {
		return;
	}

	for (i = tobj; i != NULL; i = i->next) {
		HSD_TObjInterpretAnim(i);
	}
}

/**
 * @note Address: N/A
 * @note Size: 0x4C
 */
static inline HSD_TObjTev* HSD_TObjTevLoadDesc(HSD_TObjTevDesc* tevdesc)
{
	if (tevdesc != NULL) {
		HSD_TObjTev* new = HSD_TObjTevAlloc();
		memcpy(new, tevdesc, sizeof(HSD_TObjTev));
		return new;
	}
	return NULL;
}

/**
 * @note Address: 0x801B11A4
 * @note Size: 0x258
 */
int TObjLoad(HSD_TObj* tobj, HSD_TObjDesc* td)
{
	tobj->next      = HSD_TObjLoadDesc(td->next);
	tobj->id        = (u16)td->id;
	tobj->src       = td->src;
	tobj->mtxid     = GX_IDENTITY;
	tobj->rotate.x  = td->rotate.x;
	tobj->rotate.y  = td->rotate.y;
	tobj->rotate.z  = td->rotate.z;
	tobj->scale     = td->scale;
	tobj->translate = td->translate;
	tobj->wrap_s    = td->wrap_s;
	tobj->wrap_t    = td->wrap_t;
	tobj->repeat_s  = td->repeat_s;
	tobj->repeat_t  = td->repeat_t;
	tobj->flags     = td->blend_flags;
	tobj->blending  = td->blending;
	tobj->magFilt   = td->magFilt;
	tobj->imagedesc = td->imagedesc;
	tobj->tlut      = HSD_TlutLoadDesc(td->tlutdesc);
	tobj->lod       = td->lod;
	tobj->aobj      = NULL;
	tobj->flags |= TEX_MTX_DIRTY;
	tobj->tlut_no = (u8)-1;
	tobj->tev     = HSD_TObjTevLoadDesc(td->tev);

	_HSD_NeedCacheInvalidate(HSD_INVALIDATE_TEXTURE_CACHE);

	return 0;
}

/**
 * @note Address: 0x801B10E0
 * @note Size: 0xC4
 */
HSD_TObj* HSD_TObjLoadDesc(HSD_TObjDesc* td)
{
	if (td != NULL) {
		HSD_TObj* tobj;
		HSD_ClassInfo* info;

		if (!td->class_name || !(info = hsdSearchClassInfo(td->class_name))) {
			tobj = HSD_TObjAlloc();
		} else {
			tobj = hsdNew(info);
			TOBJ_ASSERT(0x240, tobj, "tobj");
		}
		HSD_TOBJ_METHOD(tobj)->load(tobj, td);
		return tobj;
	} else {
		return NULL;
	}
}

/**
 * @note Address: 0x801B10A4
 * @note Size: 0x3C
 */
HSD_TObj* _HSD_TObjGetCurrentByType(HSD_TObj* from, u32 mapping)
{
	HSD_TObj* tp;

	if (from == NULL) {
		tp = tobj_head;
	} else {
		tp = from->next;
	}

	for (; tp != NULL; tp = tp->next) {
		if (tobj_coord(tp) == mapping) {
			return tp;
		}
	}

	return NULL;
}

/**
 * @note Address: 0x801B0ED0
 * @note Size: 0x1D4
 */
void MakeTextureMtx(HSD_TObj* tobj)
{
	Vec scale;
	Mtx m;
	Vec trans;
	Quaternion rot;

	TOBJ_ASSERT(0x2BA, tobj->repeat_s && tobj->repeat_t, "tobj->repeat_s && tobj->repeat_t");

	scale.x = (f32)__fabs(tobj->scale.x) < 1e-10F ? 0.0F : (f32)tobj->repeat_s / tobj->scale.x;
	scale.y = (f32)__fabs(tobj->scale.y) < 1e-10F ? 0.0F : (f32)tobj->repeat_t / tobj->scale.y;
	scale.z = tobj->scale.z;
	rot.x   = tobj->rotate.x;
	rot.y   = tobj->rotate.y;
	rot.z   = -tobj->rotate.z;
	trans.x = -tobj->translate.x;
	trans.y = -(tobj->translate.y + (tobj->wrap_t == GX_MIRROR ? 1.0F / ((f32)tobj->repeat_t / tobj->scale.y) : 0.0F));
	trans.z = tobj->translate.z;

	PSMTXTrans(tobj->mtx, trans.x, trans.y, trans.z);
	HSD_MkRotationMtx(m, (Vec*)&rot);
	PSMTXConcat(m, tobj->mtx, tobj->mtx);
	PSMTXScale(m, scale.x, scale.y, scale.z);
	PSMTXConcat(m, tobj->mtx, tobj->mtx);
}

/**
 * @note Address: N/A
 * @note Size: 0x2E8
 */
void TObjSetupMtx(HSD_TObj* tobj)
{
	int i;

	if (tobj_coord(tobj) == TEX_COORD_TOON) {
		return;
	}

	if (tobj->flags & TEX_MTX_DIRTY) {
		HSD_TOBJ_METHOD(tobj)->make_mtx(tobj);
		tobj->flags &= ~TEX_MTX_DIRTY;
	}

	switch (tobj_coord(tobj)) {
	case TEX_COORD_REFLECTION: {
		Mtx mtx;

		for (i = 0; i < 3; i++) {
			mtx[i][0] = 0.5F * tobj->mtx[i][0];
			mtx[i][1] = -0.5F * tobj->mtx[i][1];
			mtx[i][2] = 0.0F;
			mtx[i][3] = 0.5F * tobj->mtx[i][0] + 0.5F * tobj->mtx[i][1] + tobj->mtx[i][2] + tobj->mtx[i][3];
		}
		GXLoadTexMtxImm(mtx, tobj->mtxid, GX_MTX3x4);
	} break;

	case TEX_COORD_HILIGHT: {
		HSD_LObj* lobj;

		if ((lobj = HSD_LObjGetCurrentByType(LOBJ_INFINITE)) != NULL) {
			HSD_CObj* cobj;
			Vec ldir, half;
			Mtx mtx;

			cobj = HSD_CObjGetCurrent();
			TOBJ_ASSERT(0x315, cobj, "cobj");
			HSD_LObjGetLightVector(lobj, &ldir);
			PSMTXMultVecSR(cobj->view_mtx, &ldir, &ldir);
			ldir.z += -1.0F;

			PSVECNormalize(&ldir, &half);

			half.x *= -0.5;
			half.y *= -0.5;
			half.z *= -0.5;

			mtx[0][0] = tobj->mtx[0][0] * half.x;
			mtx[0][1] = tobj->mtx[0][0] * half.y;
			mtx[0][2] = tobj->mtx[0][0] * half.z;
			mtx[0][3] = tobj->mtx[0][0] * 0.5F + tobj->mtx[0][3];
			mtx[1][0] = tobj->mtx[1][0] * half.x;
			mtx[1][1] = tobj->mtx[1][0] * half.y;
			mtx[1][2] = tobj->mtx[1][0] * half.z;
			mtx[1][3] = tobj->mtx[1][0] * 0.5F + tobj->mtx[1][3];
			mtx[2][0] = mtx[2][1] = mtx[2][2] = 0.0F;
			mtx[2][3]                         = 1.0F;

			GXLoadTexMtxImm(mtx, tobj->mtxid, GX_MTX3x4);
		} else {
			static Mtx zero = { 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F };

			GXLoadTexMtxImm(zero, tobj->mtxid, GX_MTX3x4);
		}
	} break;

	case TEX_COORD_SHADOW: {
		HSD_CObj* cobj = HSD_CObjGetCurrent();
		Mtx mtx;

		PSMTXConcat(tobj->mtx, HSD_CObjGetInvViewingMtxPtrDirect(cobj), mtx);
		GXLoadTexMtxImm(mtx, tobj->mtxid, GX_MTX3x4);
	} break;

	default:
		if (tobj_bump(tobj)) {
			GXLoadTexMtxImm(tobj->mtx, tobj->mtxid, GX_MTX2x4);
		} else {
			GXLoadTexMtxImm(tobj->mtx, tobj->mtxid, GX_MTX3x4);
		}
		break;
	}
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void setupTextureCoordGen(HSD_TObj* tobj)
{
	switch (tobj_coord(tobj)) {
	case TEX_COORD_SHADOW:
		GXSetTexCoordGen2(tobj->coord, GX_TG_MTX3x4, GX_TG_POS, GX_PNMTX0, GX_DISABLE, tobj->mtxid);
		break;
	case TEX_COORD_REFLECTION:
	case TEX_COORD_HILIGHT:
		GXSetTexCoordGen2(tobj->coord, GX_TG_MTX3x4, GX_TG_NRM, GX_TEXMTX0, GX_ENABLE, tobj->mtxid);
		break;
	default:
		if (tobj_bump(tobj)) {
			GXSetTexCoordGen2(tobj->coord, GX_TG_MTX2x4, tobj->src, tobj->mtxid, GX_DISABLE, GX_PTIDENTITY);
		} else {
			GXSetTexCoordGen2(tobj->coord, GX_TG_MTX2x4, tobj->src, GX_IDENTITY, GX_DISABLE, tobj->mtxid);
		}
	}
}

/**
 * @note Address: N/A
 * @note Size: 0x88
 */
static inline u32 HSD_TexCoordID2TexGenSrc(u32 coord)
{
	switch (coord) {
	case GX_TEXCOORD0:
		return GX_TG_TEXCOORD0;
	case GX_TEXCOORD0 + 1:
		return GX_TG_TEXCOORD0 + 1;
	case GX_TEXCOORD0 + 2:
		return GX_TG_TEXCOORD0 + 2;
	case GX_TEXCOORD0 + 3:
		return GX_TG_TEXCOORD0 + 3;
	case GX_TEXCOORD0 + 4:
		return GX_TG_TEXCOORD0 + 4;
	case GX_TEXCOORD0 + 5:
		return GX_TG_TEXCOORD0 + 5;
	case GX_TEXCOORD0 + 6:
		return GX_TG_TEXCOORD0 + 6;
	case GX_TEXCOORD0 + 7:
	default:
		TOBJ_ASSERT(0x811, 0, "0");
	}
	return GX_TG_TEXCOORD0;
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void setupTextureCoordGenBump(HSD_TObj* bump)
{
	static u32 func[] = {
		GX_TG_BUMP0, GX_TG_BUMP1, GX_TG_BUMP2, GX_TG_BUMP3, GX_TG_BUMP4, GX_TG_BUMP5, GX_TG_BUMP6, GX_TG_BUMP7,
	};

	u32 mask;
	int i;

	mask = HSD_LObjGetLightMaskDiffuse();

	for (i = 0; i < 8; i++) {
		if (mask & (1 << i)) {
			break;
		}
	}
	if (i >= 8) {
		i = 0;
	}

	GXSetTexCoordGen2((u32)(bump->coord + 1), func[i], HSD_TexCoordID2TexGenSrc(bump->coord), GX_IDENTITY, GX_DISABLE, GX_PTIDENTITY);
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void setupTextureCoordGenToon(HSD_TObj* toon)
{
	GXSetTexCoordGen2(toon->coord, GX_TG_SRTG, toon->src, GX_IDENTITY, GX_DISABLE, GX_PTIDENTITY);
}

/**
 * @note Address: 0x801B08F4
 * @note Size: 0x2F4
 */
void HSD_TObjSetupTextureCoordGen(HSD_TObj* tobj)
{
	for (; tobj != NULL; tobj = tobj->next) {
		if (tobj->map_id == GX_TEXMAP_NULL) {
			continue;
		}
		if (tobj_bump(tobj)) {
			setupTextureCoordGen(tobj);
			setupTextureCoordGenBump(tobj);
		} else if (tobj_coord(tobj) == TEX_COORD_TOON) {
			setupTextureCoordGenToon(tobj);
		} else {
			setupTextureCoordGen(tobj);
		}
	}
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void TObjSetupTevModulateShadow(HSD_TObj* shadow)
{
	static HSD_TevDesc tevdesc = {
		NULL,        TEVCONF_MODE, (u32)(-1),  GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL, GX_TEV_ADD, GX_CC_ZERO,
		GX_CC_CPREV, GX_CC_TEXC,   GX_CC_ZERO, GX_CS_SCALE_1,    GX_TB_ZERO,     GX_ENABLE,     GX_TEVPREV, GX_TEV_ADD,
		GX_CA_ZERO,  GX_CA_ZERO,   GX_CA_ZERO, GX_CA_APREV,      GX_CS_SCALE_1,  GX_TB_ZERO,    GX_ENABLE,  GX_TEVPREV,
	};

	for (; shadow && tobj_coord(shadow) == TEX_COORD_SHADOW; shadow = shadow->next) {
		tevdesc.stage = HSD_StateAssignTev();
		tevdesc.coord = shadow->coord;
		tevdesc.map   = shadow->map_id;
		HSD_SetupTevStage(&tevdesc);
	}
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline void SetupEmbossBumpTev(HSD_TObj* bump)
{
	static HSD_TevDesc tevdesc = {
		NULL,       TEVCONF_MODE, (u32)(-1),   GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0, GX_TEV_SUB, GX_CC_ZERO,
		GX_CC_TEXC, GX_CC_RASC,   GX_CC_CPREV, GX_CS_SCALE_1,    GX_TB_ZERO,     GX_ENABLE,   GX_TEVPREV, GX_TEV_SUB,
		GX_CA_ZERO, GX_CA_ZERO,   GX_CA_ZERO,  GX_CA_APREV,      GX_CS_SCALE_1,  GX_TB_ZERO,  GX_ENABLE,  GX_TEVPREV,
	};

	tevdesc.stage               = HSD_StateAssignTev();
	tevdesc.coord               = bump->coord;
	tevdesc.map                 = bump->map_id;
	tevdesc.u.tevconf.clr_op    = GX_TEV_ADD;
	tevdesc.u.tevconf.alpha_op  = GX_TEV_ADD;
	tevdesc.u.tevconf.clr_clamp = GX_DISABLE;
	HSD_SetupTevStage(&tevdesc);
	tevdesc.stage               = HSD_StateAssignTev();
	tevdesc.coord               = (u32)(bump->coord + 1);
	tevdesc.u.tevconf.clr_op    = GX_TEV_SUB;
	tevdesc.u.tevconf.alpha_op  = GX_TEV_SUB;
	tevdesc.u.tevconf.clr_clamp = GX_ENABLE;
	HSD_SetupTevStage(&tevdesc);
}

/**
 * @note Address: 0x801B07D4
 * @note Size: 0x120
 */
void HSD_TObjSetupVolatileTev(HSD_TObj* tobj, u32 rendermode)
{
	for (; tobj; tobj = tobj->next) {
		if (tobj->map_id == GX_TEXMAP_NULL) {
			continue;
		}

		if (tobj_bump(tobj)) {
			SetupEmbossBumpTev(tobj);
		}

		if (tobj_lightmap(tobj) & TEX_LIGHTMAP_SHADOW) {
			TObjSetupTevModulateShadow(tobj);
			break;
		}
	}
}

/**
 * @note Address: 0x801AFF14
 * @note Size: 0x8C0
 */
void MakeColorGenTExp(u32 lightmap, HSD_TObj* tobj, HSD_TExp** c, HSD_TExp** a, HSD_TExp** list, int repeat)
{
	HSD_TObjTev* tev = tobj->tev;
	u8* in;
	HSD_TExp *e0, *tmp;
	int i;

	HSD_TExp* konst_rgb;
	HSD_TExp* konst_r;
	HSD_TExp* konst_g;
	HSD_TExp* konst_b;
	HSD_TExp* konst_a;
	HSD_TExp* reg0_rgb;
	HSD_TExp* reg0_a;
	HSD_TExp* reg1_rgb;
	HSD_TExp* reg1_a;

	int use_k_rgb    = 0;
	int use_k_r      = 0;
	int use_k_g      = 0;
	int use_k_b      = 0;
	int use_k_a      = 0;
	int use_reg0_rgb = 0;
	int use_reg0_a   = 0;
	int use_reg1_rgb = 0;
	int use_reg1_a   = 0;

	in = &tev->color_a;
	for (i = 0; i < 4; i++) {
		switch (in[i]) {
		case TOBJ_TEV_CC_KONST_RGB:
			use_k_rgb = 1;
			break;
		case TOBJ_TEV_CC_KONST_RRR:
			use_k_r = 1;
			break;
		case TOBJ_TEV_CC_KONST_GGG:
			use_k_g = 1;
			break;
		case TOBJ_TEV_CC_KONST_BBB:
			use_k_b = 1;
			break;
		case TOBJ_TEV_CC_KONST_AAA:
			use_k_a = 1;
			break;
		case TOBJ_TEV_CC_TEX0_RGB:
			use_reg0_rgb = 1;
			break;
		case TOBJ_TEV_CC_TEX0_AAA:
			use_reg0_a = 1;
			break;
		case TOBJ_TEV_CC_TEX1_RGB:
			use_reg1_rgb = 1;
			break;
		case TOBJ_TEV_CC_TEX1_AAA:
			use_reg1_a = 1;
			break;
		default:
			break;
		}
	}
	in = &tev->alpha_a;
	for (i = 0; i < 4; i++) {
		switch (in[i]) {
		case TOBJ_TEV_CA_KONST_R:
			use_k_r = 1;
			break;
		case TOBJ_TEV_CA_KONST_G:
			use_k_g = 1;
			break;
		case TOBJ_TEV_CA_KONST_B:
			use_k_b = 1;
			break;
		case TOBJ_TEV_CA_KONST_A:
			use_k_a = 1;
			break;
		case TOBJ_TEV_CA_TEX0_A:
			use_reg0_a = 1;
			break;
		case TOBJ_TEV_CA_TEX1_A:
			use_reg1_a = 1;
			break;
		default:
			break;
		}
	}

	if (use_k_rgb) {
		konst_rgb = HSD_TExpCnst(&tobj->tev->konst, HSD_TE_RGB, HSD_TE_U8, list);
	}
	if (use_k_r) {
		konst_r = HSD_TExpCnst(&tobj->tev->konst.r, HSD_TE_X, HSD_TE_U8, list);
	}
	if (use_k_g) {
		konst_g = HSD_TExpCnst(&tobj->tev->konst.g, HSD_TE_X, HSD_TE_U8, list);
	}
	if (use_k_b) {
		konst_b = HSD_TExpCnst(&tobj->tev->konst.b, HSD_TE_X, HSD_TE_U8, list);
	}
	if (use_k_a) {
		konst_a = HSD_TExpCnst(&tobj->tev->konst.a, HSD_TE_X, HSD_TE_U8, list);
	}
	if (use_reg0_rgb) {
		reg0_rgb = HSD_TExpCnst(&tobj->tev->tev0, HSD_TE_RGB, HSD_TE_U8, list);
	}
	if (use_reg0_a) {
		reg0_a = HSD_TExpCnst(&tobj->tev->tev0.a, HSD_TE_X, HSD_TE_U8, list);
	}
	if (use_reg1_rgb) {
		reg1_rgb = HSD_TExpCnst(&tobj->tev->tev1, HSD_TE_RGB, HSD_TE_U8, list);
	}
	if (use_reg1_a) {
		reg1_a = HSD_TExpCnst(&tobj->tev->tev1.a, HSD_TE_X, HSD_TE_U8, list);
	}

	e0 = HSD_TExpTev(list);
	HSD_TExpOrder(e0, tobj, GX_COLOR_NULL);

	if (tev->active & TOBJ_TEVREG_ACTIVE_COLOR_TEV) {
		HSD_TEInput sel[4];
		HSD_TExp* exp[4];
		int i;

		in = &tev->color_a;
		for (i = 0; i < 4; i++) {
			switch (in[i]) {
			case GX_CC_ZERO:
				sel[i] = HSD_TE_0;
				exp[i] = HSD_TEXP_ZERO;
				break;
			case GX_CC_ONE:
				sel[i] = HSD_TE_1;
				exp[i] = HSD_TEXP_ZERO;
				break;
			case GX_CC_HALF:
				sel[i] = HSD_TE_4_8;
				exp[i] = HSD_TEXP_ZERO;
				break;
			case GX_CC_TEXC:
				sel[i] = HSD_TE_RGB;
				exp[i] = HSD_TEXP_TEX;
				break;
			case GX_CC_TEXA:
				sel[i] = HSD_TE_A;
				exp[i] = HSD_TEXP_TEX;
				break;
			case TOBJ_TEV_CC_KONST_RGB:
				sel[i] = HSD_TE_RGB;
				exp[i] = konst_rgb;
				break;
			case TOBJ_TEV_CC_KONST_RRR:
				sel[i] = HSD_TE_X;
				exp[i] = konst_r;
				break;
			case TOBJ_TEV_CC_KONST_GGG:
				sel[i] = HSD_TE_X;
				exp[i] = konst_g;
				break;
			case TOBJ_TEV_CC_KONST_BBB:
				sel[i] = HSD_TE_X;
				exp[i] = konst_b;
				break;
			case TOBJ_TEV_CC_KONST_AAA:
				sel[i] = HSD_TE_X;
				exp[i] = konst_a;
				break;
			case TOBJ_TEV_CC_TEX0_RGB:
				tmp = HSD_TExpTev(list);
				HSD_TExpOrder(tmp, NULL, GX_COLOR_NULL);
				HSD_TExpColorOp(tmp, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE);
				HSD_TExpColorIn(tmp, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_RGB, reg0_rgb);
				sel[i] = HSD_TE_RGB;
				exp[i] = tmp;
				break;
			case TOBJ_TEV_CC_TEX0_AAA:
				tmp = HSD_TExpTev(list);
				HSD_TExpOrder(tmp, NULL, GX_COLOR_NULL);
				HSD_TExpColorOp(tmp, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE);
				HSD_TExpColorIn(tmp, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_X, reg0_a);
				sel[i] = HSD_TE_RGB;
				exp[i] = tmp;
				break;
			case TOBJ_TEV_CC_TEX1_RGB:
				tmp = HSD_TExpTev(list);
				HSD_TExpOrder(tmp, NULL, GX_COLOR_NULL);
				HSD_TExpColorOp(tmp, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE);
				HSD_TExpColorIn(tmp, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_RGB, reg1_rgb);
				sel[i] = HSD_TE_RGB;
				exp[i] = tmp;
				break;
			case TOBJ_TEV_CC_TEX1_AAA:
				tmp = HSD_TExpTev(list);
				HSD_TExpOrder(tmp, NULL, GX_COLOR_NULL);
				HSD_TExpColorOp(tmp, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE);
				HSD_TExpColorIn(tmp, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_X, reg1_a);
				sel[i] = HSD_TE_RGB;
				exp[i] = tmp;
				break;
			default:
				TOBJ_ASSERT(0x5A9, 0, "0");
				break;
			}
		}

		HSD_TExpColorOp(e0, tev->color_op, tev->color_bias, tev->color_scale, tev->color_clamp);
		HSD_TExpColorIn(e0, sel[0], exp[0], sel[1], exp[1], sel[2], exp[2], sel[3], exp[3]);
		*c = e0;
	}

	if (tev->active & TOBJ_TEVREG_ACTIVE_ALPHA_TEV) {
		HSD_TEInput sel[4];
		HSD_TExp* exp[4];
		int i;

		in = &tev->alpha_a;
		for (i = 0; i < 4; i++) {
			switch (in[i]) {
			case GX_CA_ZERO:
				sel[i] = HSD_TE_0;
				exp[i] = HSD_TEXP_ZERO;
				break;
			case GX_CA_TEXA:
				sel[i] = HSD_TE_A;
				exp[i] = HSD_TEXP_TEX;
				break;
			case TOBJ_TEV_CA_KONST_R:
				sel[i] = HSD_TE_X;
				exp[i] = konst_r;
				break;
			case TOBJ_TEV_CA_KONST_G:
				sel[i] = HSD_TE_X;
				exp[i] = konst_g;
				break;
			case TOBJ_TEV_CA_KONST_B:
				sel[i] = HSD_TE_X;
				exp[i] = konst_b;
				break;
			case TOBJ_TEV_CA_KONST_A:
				sel[i] = HSD_TE_X;
				exp[i] = konst_a;
				break;
			case TOBJ_TEV_CA_TEX0_A:
				tmp = HSD_TExpTev(list);
				HSD_TExpOrder(tmp, NULL, GX_COLOR_NULL);
				HSD_TExpAlphaOp(tmp, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE);
				HSD_TExpAlphaIn(tmp, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_X, reg0_a);
				sel[i] = HSD_TE_A;
				exp[i] = tmp;
				break;
			case TOBJ_TEV_CA_TEX1_A:
				tmp = HSD_TExpTev(list);
				HSD_TExpOrder(tmp, NULL, GX_COLOR_NULL);
				HSD_TExpAlphaOp(tmp, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE);
				HSD_TExpAlphaIn(tmp, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_X, reg1_a);
				sel[i] = HSD_TE_A;
				exp[i] = tmp;
				break;
			default:
				TOBJ_ASSERT(0x5F2, 0, "0");
				break;
			}
		}

		HSD_TExpAlphaOp(e0, tev->alpha_op, tev->alpha_bias, tev->alpha_scale, tev->alpha_clamp);
		HSD_TExpAlphaIn(e0, sel[0], exp[0], sel[1], exp[1], sel[2], exp[2], sel[3], exp[3]);

		*a = e0;
	}
}

/**
 * @note Address: 0x801AF8B4
 * @note Size: 0x660
 */
void TObjMakeTExp(HSD_TObj* tobj, u32 lightmap, u32 lightmap_done, HSD_TExp** c, HSD_TExp** a, HSD_TExp** list)
{
	HSD_TExp *e0, *e1;
	HSD_TExp *c_src, *a_src;
	HSD_TEInput c_sel, a_sel;
	int repeat = (lightmap_done & tobj_lightmap(tobj)) != 0;

	c_src = HSD_TEXP_TEX;
	c_sel = HSD_TE_RGB;

	a_src = HSD_TEXP_TEX;
	a_sel = HSD_TE_A;

	e0 = HSD_TExpTev(list);

	if (tobj->tev && (tobj->tev->active & (TOBJ_TEVREG_ACTIVE_COLOR_TEV | TOBJ_TEVREG_ACTIVE_ALPHA_TEV))) {
		MakeColorGenTExp(lightmap, tobj, &c_src, &a_src, list, repeat);
	}

	HSD_TExpOrder(e0, tobj, GX_COLOR_NULL);

	switch (tobj_colormap(tobj)) {
	case TEX_COLORMAP_ALPHA_MASK:
		HSD_TExpColorOp(e0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE);
		HSD_TExpColorIn(e0, HSD_TE_RGB, *c, c_sel, c_src, a_sel, a_src, HSD_TE_0, HSD_TEXP_ZERO);
		break;
	case TEX_COLORMAP_RGB_MASK:
		HSD_TExpColorOp(e0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE);
		HSD_TExpColorIn(e0, HSD_TE_RGB, *c, c_sel, c_src, c_sel, c_src, HSD_TE_0, HSD_TEXP_ZERO);
		break;
	case TEX_COLORMAP_BLEND:
		e1 = HSD_TExpCnst(&tobj->blending, HSD_TE_X, HSD_TE_F32, list);
		HSD_TExpColorOp(e0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE);
		HSD_TExpColorIn(e0, HSD_TE_RGB, *c, c_sel, c_src, HSD_TE_X, e1, HSD_TE_0, HSD_TEXP_ZERO);
		break;
	case TEX_COLORMAP_MODULATE:
		HSD_TExpColorOp(e0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE);
		HSD_TExpColorIn(e0, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_RGB, *c, c_sel, c_src, HSD_TE_0, HSD_TEXP_ZERO);
		break;
	case TEX_COLORMAP_REPLACE:
		HSD_TExpColorOp(e0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE);
		HSD_TExpColorIn(e0, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_0, HSD_TEXP_ZERO, c_sel, c_src);
		break;
	case TEX_COLORMAP_NONE:
	case TEX_COLORMAP_PASS:
		HSD_TExpColorOp(e0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE);
		HSD_TExpColorIn(e0, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_RGB, *c);
		break;
	case TEX_COLORMAP_ADD:
		HSD_TExpColorOp(e0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE);
		HSD_TExpColorIn(e0, c_sel, c_src, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_RGB, *c);
		break;
	case TEX_COLORMAP_SUB:
		HSD_TExpColorOp(e0, GX_TEV_SUB, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE);
		HSD_TExpColorIn(e0, c_sel, c_src, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_RGB, *c);
		break;
	default:
		TOBJ_ASSERT(0x65E, 0, "0");
	}
	*c = e0;

	if (!repeat) {
		switch (tobj_alphamap(tobj)) {
		case TEX_ALPHAMAP_ALPHA_MASK:
			HSD_TExpAlphaOp(e0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE);
			HSD_TExpAlphaIn(e0, HSD_TE_A, *a, a_sel, a_src, a_sel, a_src, HSD_TE_0, HSD_TEXP_ZERO);
			break;
		case TEX_ALPHAMAP_BLEND:
			e1 = HSD_TExpCnst(&tobj->blending, HSD_TE_X, HSD_TE_F32, list);
			HSD_TExpAlphaOp(e0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE);
			HSD_TExpAlphaIn(e0, HSD_TE_A, *a, a_sel, a_src, HSD_TE_X, e1, HSD_TE_0, HSD_TEXP_ZERO);
			break;
		case TEX_ALPHAMAP_MODULATE:
			HSD_TExpAlphaOp(e0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE);
			HSD_TExpAlphaIn(e0, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_A, *a, a_sel, a_src, HSD_TE_0, HSD_TEXP_ZERO);
			break;
		case TEX_ALPHAMAP_REPLACE:
			HSD_TExpAlphaOp(e0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE);
			HSD_TExpAlphaIn(e0, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_0, HSD_TEXP_ZERO, a_sel, a_src);
			break;
		case TEX_ALPHAMAP_NONE:
		case TEX_ALPHAMAP_PASS:
			HSD_TExpAlphaOp(e0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE);
			HSD_TExpAlphaIn(e0, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_A, *a);
			break;
		case TEX_ALPHAMAP_ADD:
			HSD_TExpAlphaOp(e0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE);
			HSD_TExpAlphaIn(e0, a_sel, a_src, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_A, *a);
			break;
		case TEX_ALPHAMAP_SUB:
			HSD_TExpAlphaOp(e0, GX_TEV_SUB, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE);
			HSD_TExpAlphaIn(e0, a_sel, a_src, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_A, *a);
			break;
		default:
			TOBJ_ASSERT(0x698, 0, "0");
		}
		*a = e0;
	}
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
static inline u32 HSD_Index2TexCoord(u32 index)
{
	switch (index) {
	case 0:
		return GX_TEXCOORD0;
	case 1:
		return GX_TEXCOORD1;
	case 2:
		return GX_TEXCOORD2;
	case 3:
		return GX_TEXCOORD3;
	case 4:
		return GX_TEXCOORD4;
	case 5:
		return GX_TEXCOORD5;
	case 6:
		return GX_TEXCOORD6;
	case 7:
		return GX_TEXCOORD7;
	default:
		TOBJ_ASSERT(0x84F, 0, "0");
	}
	return GX_TEXCOORD0;
}

/**
 * @note Address: N/A
 * @note Size: 0x94
 */
static inline u32 HSD_TexMapID2PTTexMtx(u32 id)
{
	switch (id) {
	case GX_TEXMAP0:
		return GX_PTTEXMTX0;
	case GX_TEXMAP1:
		return GX_PTTEXMTX1;
	case GX_TEXMAP2:
		return GX_PTTEXMTX2;
	case GX_TEXMAP3:
		return GX_PTTEXMTX3;
	case GX_TEXMAP4:
		return GX_PTTEXMTX4;
	case GX_TEXMAP5:
		return GX_PTTEXMTX5;
	case GX_TEXMAP6:
		return GX_PTTEXMTX6;
	case GX_TEXMAP7:
		return GX_PTTEXMTX7;
	default:
		HSD_Panic("tobj.c", 0x2AB, "unexpected texmap id.\n");
	}
	return 0;
}

/**
 * @note Address: 0x801AF354
 * @note Size: 0x560
 */
s32 HSD_TObjAssignResources(HSD_TObj* tobj_top)
{
	HSD_TObj* tobj;
	u32 texmap_no   = 0;
	u32 texcoord_no = 0;
	u32 limit       = 8;
	HSD_TObj* bump  = NULL;
	HSD_TObj* toon  = NULL;

	if (tobj_top == NULL) {
		return 0;
	}

	for (tobj = tobj_top; tobj; tobj = tobj->next) {
		if (tobj_coord(tobj) == TEX_COORD_TOON) {
			toon = tobj;
		} else if (tobj_bump(tobj)) {
			bump = tobj;
		}
	}

	if (toon) {
		limit -= 1;
	}
	if (bump) {
		limit -= 2;
	}

	for (tobj = tobj_top; tobj; tobj = tobj->next) {
		if (tobj_coord(tobj) == TEX_COORD_TOON) {
			if (tobj != toon) {
				tobj->map_id = GX_TEXMAP_NULL;
			}
		} else if (tobj_bump(tobj)) {
			if (tobj != bump) {
				tobj->map_id = GX_TEXMAP_NULL;
			}
		} else if (texmap_no < limit) {
			tobj->map_id = HSD_Index2TexMap(texmap_no++);
			tobj->mtxid  = HSD_TexMapID2PTTexMtx(tobj->map_id);
			switch (tobj_coord(tobj)) {
			case TEX_COORD_REFLECTION:
			case TEX_COORD_HILIGHT:
			case TEX_COORD_SHADOW:
				tobj->coord = HSD_Index2TexCoord(texcoord_no++);
				break;
			default:
				break;
			}
		} else {
			tobj->map_id = GX_TEXMAP_NULL;
		}
	}

	for (tobj = tobj_top; tobj; tobj = tobj->next) {
		if (tobj->map_id != GX_TEXMAP_NULL && !tobj_bump(tobj)) {
			switch (tobj_coord(tobj)) {
			case TEX_COORD_UV:
				tobj->coord = HSD_Index2TexCoord(texcoord_no++);
				break;
			default:
				break;
			}
		}
	}

	if (bump) {
		bump->map_id = HSD_Index2TexMap(texmap_no++);
		bump->mtxid  = GX_TEXMTX9;
		bump->coord  = HSD_Index2TexCoord(texcoord_no);
		texcoord_no += 2;
	}
	if (toon) {
		toon->map_id = HSD_Index2TexMap(texmap_no++);
		toon->coord  = HSD_Index2TexCoord(texcoord_no++);
	}

	return (int)texcoord_no;
}

/**
 * @note Address: 0x801AF00C
 * @note Size: 0x348
 */
void HSD_TObjSetup(HSD_TObj* tobj)
{
	GXTlutObj tlutobj;
	GXTexObj texobj;
	int num;
	HSD_Tlut* tluts[8];
	int nb_tluts      = 0;
	u32 tlut_name     = GX_TLUT0;
	u32 big_tlut_name = GX_BIGTLUT0;
	int i;

	tobj_head = tobj;

	if (tobj == NULL) {
		return;
	}

	num = HSD_TObjAssignResources(tobj);
	if (num > 0) {
		HSD_StateRegisterTexGen(HSD_Index2TexCoord(num - 1));
	}

	for (; tobj; tobj = tobj->next) {
		static HSD_TexLODDesc default_lod = {
			GX_LIN_MIP_LIN, 0.0F, GX_FALSE, GX_FALSE, GX_ANISO_1,
		};

		HSD_TexLODDesc* lod;
		HSD_ImageDesc* imagedesc = tobj->imagedesc;
		s32 min_filter;

		if (tobj->map_id == GX_TEXMAP_NULL) {
			continue;
		}

		TObjSetupMtx(tobj);
		TOBJ_ASSERT(0x759, imagedesc, "imagedesc");
		TOBJ_ASSERT(0x75A, imagedesc->img_ptr, "imagedesc->image_ptr");

		lod        = tobj->lod != NULL ? tobj->lod : &default_lod;
		min_filter = lod->minFilt;

		switch (imagedesc->format) {
		case GX_TF_C4:
		case GX_TF_C8:
		case GX_TF_C14X2: {
			HSD_Tlut* tlut;

			if (tobj->tlut_no != (u8)-1) {
				tlut = tobj->tluttbl[tobj->tlut_no];
			} else {
				tlut = tobj->tlut;
			}

			TOBJ_ASSERT(0x76A, tlut, "tlut");

			// Retail reuses a loaded TLUT when the entry counts match.
			for (i = 0; i < nb_tluts; i++) {
				if (tluts[i]->n_entries == tlut->n_entries) {
					break;
				}
			}
			if (i < nb_tluts) {
				tlut->tlut_name = tluts[i]->tlut_name;
			} else if (nb_tluts < 8) {
				if (tlut->n_entries > 256) {
					tlut->tlut_name = big_tlut_name++;
				} else {
					tlut->tlut_name = tlut_name++;
				}
				GXInitTlutObj(&tlutobj, tlut->lut, tlut->fmt, tlut->n_entries);
				GXLoadTlut(&tlutobj, tlut->tlut_name);
				tluts[nb_tluts++] = tlut;
			} else {
				tlut->tlut_name = GX_TLUT0;
			}

			GXInitTexObjCI(&texobj, imagedesc->img_ptr, imagedesc->width, imagedesc->height, imagedesc->format, tobj->wrap_s, tobj->wrap_t,
			               imagedesc->mipmap ? GX_TRUE : GX_FALSE, tlut->tlut_name);
			if (min_filter == GX_LIN_MIP_LIN) {
				min_filter = GX_LIN_MIP_NEAR;
			}
		} break;

		case GX_TF_I4:
		case GX_TF_I8:
		case GX_TF_IA4:
		case GX_TF_IA8:
		case GX_TF_RGB565:
		case GX_TF_RGB5A3:
		case GX_TF_RGBA8:
		case GX_TF_CMPR:
			GXInitTexObj(&texobj, imagedesc->img_ptr, imagedesc->width, imagedesc->height, imagedesc->format, tobj->wrap_s, tobj->wrap_t,
			             imagedesc->mipmap ? GX_TRUE : GX_FALSE);
			break;

		default:
			TOBJ_ASSERT(0x7A6, 0, "0");
		}

		if (!imagedesc->mipmap) {
			min_filter &= 0x01;
		}
		GXInitTexObjLOD(&texobj, min_filter, tobj->magFilt, imagedesc->minLOD, imagedesc->maxLOD, lod->LODBias, lod->bias_clamp,
		                lod->edgeLODEnable, lod->max_anisotropy);

		GXLoadTexObj(&texobj, tobj->map_id);
	}
}

/**
 * @note Address: 0x801AEF50
 * @note Size: 0xBC
 */
u32 HSD_Index2TexMtx(u32 index)
{
	switch (index) {
	case 0:
		return GX_TEXMTX0;
	case 1:
		return GX_TEXMTX1;
	case 2:
		return GX_TEXMTX2;
	case 3:
		return GX_TEXMTX3;
	case 4:
		return GX_TEXMTX4;
	case 5:
		return GX_TEXMTX5;
	case 6:
		return GX_TEXMTX6;
	case 7:
		return GX_TEXMTX7;
	case 8:
		return GX_TEXMTX8;
	case 9:
		return GX_TEXMTX9;
	case 10:
		return GX_IDENTITY;
	default:
		OSReport("texmtx index exceed hardware limit (%d).\n", index);
		HSD_Panic("tobj.c", 0x89C, "");
	}
	return GX_IDENTITY;
}

/**
 * @note Address: N/A
 * @note Size: 0x90
 */
u32 HSD_Index2TexMap(u32 index)
{
	switch (index) {
	case 0:
		return GX_TEXMAP0;
	case 1:
		return GX_TEXMAP1;
	case 2:
		return GX_TEXMAP2;
	case 3:
		return GX_TEXMAP3;
	case 4:
		return GX_TEXMAP4;
	case 5:
		return GX_TEXMAP5;
	case 6:
		return GX_TEXMAP6;
	case 7:
		return GX_TEXMAP7;
	default:
		TOBJ_ASSERT(0x8C2, 0, "0");
	}
	return GX_TEXMAP0;
}

/**
 * @note Address: 0x801AEEDC
 * @note Size: 0x74
 */
void HSD_TObjRemoveAll(HSD_TObj* tobj)
{
	while (tobj) {
		HSD_TObj* next = tobj->next;
		hsdDelete(tobj);
		tobj = next;
	}
}

/**
 * @note Address: 0x801AEEC4
 * @note Size: 0x18
 */
HSD_TObj* HSD_TObjGetNext(HSD_TObj* tobj)
{
	if (tobj == NULL) {
		return NULL;
	}

	return tobj->next;
}

/**
 * @note Address: 0x801AEE68
 * @note Size: 0x5C
 */
HSD_TObj* HSD_TObjAlloc(void)
{
	HSD_TObj* new = hsdNew((HSD_ClassInfo*)(default_class ? default_class : &hsdTObj));
	TOBJ_ASSERT(0x93F, new, "new");
	return new;
}

/**
 * @note Address: 0x801AEE34
 * @note Size: 0x34
 */
void HSD_TObjRemove(HSD_TObj* tobj)
{
	if (tobj) {
		HSD_CLASS_METHOD(tobj)->destroy((HSD_Class*)tobj);
	}
}

/**
 * @note Address: N/A
 * @note Size: 0x58
 */
HSD_Tlut* HSD_TlutAlloc(void)
{
	HSD_Tlut* tlut = hsdAllocMemPiece(sizeof(HSD_Tlut));
	TOBJ_ASSERT(0x95C, tlut, "tlut");
	memset(tlut, 0, sizeof(HSD_Tlut));
	return tlut;
}

/**
 * @note Address: N/A
 * @note Size: 0x58
 */
HSD_TObjTev* HSD_TObjTevAlloc(void)
{
	HSD_TObjTev* tev = hsdAllocMemPiece(sizeof(HSD_TObjTev));
	TOBJ_ASSERT(0x987, tev, "tev");
	memset(tev, 0, sizeof(HSD_TObjTev));
	return tev;
}

/**
 * @note Address: 0x801AEDDC
 * @note Size: 0x58
 */
HSD_ImageDesc* HSD_ImageDescAlloc(void)
{
	HSD_ImageDesc* idesc = hsdAllocMemPiece(sizeof(HSD_ImageDesc));
	TOBJ_ASSERT(0x9B2, idesc, "idesc");
	memset(idesc, 0, sizeof(HSD_ImageDesc));
	return idesc;
}

/**
 * @note Address: 0x801AEDB8
 * @note Size: 0x24
 */
void HSD_ImageDescRemove(HSD_ImageDesc* idesc)
{
	hsdFreeMemPiece(idesc, sizeof(HSD_ImageDesc));
}

/**
 * @note Address: 0x801AED60
 * @note Size: 0x58
 */
int TObjInit(HSD_Class* o)
{
	int result = HSD_PARENT_INFO(&hsdTObj)->init(o);
	if (result >= 0) {
		result             = 0;
		((HSD_TObj*)o)->id = (u16)-1;
	}
	return result;
}

/**
 * @note Address: 0x801AECA4
 * @note Size: 0xBC
 */
void TObjRelease(HSD_Class* o)
{
	HSD_TObj* tobj = HSD_TOBJ(o);

	HSD_AObjRemove(tobj->aobj);
	if (tobj->tlut) {
		hsdFreeMemPiece(tobj->tlut, sizeof(HSD_Tlut));
	}
	if (tobj->tev) {
		hsdFreeMemPiece(tobj->tev, sizeof(HSD_TObjTev));
	}
	if (tobj->tluttbl) {
		int i;
		for (i = 0; tobj->tluttbl[i]; i++) {
			HSD_TlutRemove(tobj->tluttbl[i]);
		}
		HSD_Free(tobj->tluttbl);
	}

	HSD_PARENT_INFO(&hsdTObj)->release(o);
}

/**
 * @note Address: 0x801AEC44
 * @note Size: 0x60
 */
void TObjAmnesia(HSD_ClassInfo* info)
{
	if (info == HSD_CLASS_INFO(default_class)) {
		default_class = NULL;
	}
	if (info == HSD_CLASS_INFO(&hsdTObj)) {
		tobj_head = NULL;
	}
	HSD_PARENT_INFO(&hsdTObj)->amnesia(info);
}

/**
 * @note Address: 0x801AEBA0
 * @note Size: 0xA4
 */
void TObjInfoInit(void)
{
	hsdInitClassInfo(HSD_CLASS_INFO(&hsdTObj), &hsdObj, "sysdolphin_base_library", "hsd_tobj", sizeof(HSD_TObjInfo), sizeof(HSD_TObj));

	HSD_CLASS_INFO(&hsdTObj)->init     = TObjInit;
	HSD_CLASS_INFO(&hsdTObj)->release  = TObjRelease;
	HSD_CLASS_INFO(&hsdTObj)->amnesia  = TObjAmnesia;
	HSD_TOBJ_INFO(&hsdTObj)->load      = TObjLoad;
	HSD_TOBJ_INFO(&hsdTObj)->make_texp = TObjMakeTExp;

	hsdTObj.make_mtx = MakeTextureMtx;

	HSD_TOBJ_INFO(&hsdTObj)->update = TObjUpdateFunc;
}

// pobj functions, they sit here until pobj's split is extended to 0x801AEBA0
