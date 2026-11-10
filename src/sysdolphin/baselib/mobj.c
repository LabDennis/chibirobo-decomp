#include <sysdolphin/mobj.h>
#include <sysdolphin/class_new.h>

void* hsdAllocMemPiece();
void OSReport(const char* msg, ...);

void MObjInfoInit();
HSD_MObjInfo hsdMObj = { MObjInfoInit };

// declared backwards, mwcc lays these out last one first
HSD_TObj* tobj_shadows;
HSD_TObj* tobj_toon;
static HSD_MObj* current_mobj;
static HSD_MObjInfo* default_class;

/**
 * @note Address: 0x801B8E54
 * @note Size: 0x8
 */
void HSD_MObjSetCurrent(HSD_MObj* mobj)
{
	current_mobj = mobj;
}

/**
 * @note Address: 0x801B8E5C
 * @note Size: 0x18
 */
u32 GetMaterialRenderModeFlags(HSD_MObj* mobj)
{
	// the flags getter, no name evidence for this revision
	if (mobj != NULL) {
		return mobj->rendermode;
	}
	return 0;
}

/**
 * @note Address: 0x801B8E74
 * @note Size: 0x18
 */
void HSD_MObjSetFlags(HSD_MObj* mobj, u32 flags)
{
	if (mobj != NULL) {
		mobj->rendermode |= flags;
	}
}

/**
 * @note Address: 0x801B8E8C
 * @note Size: 0x18
 */
void HSD_MObjClearFlags(HSD_MObj* mobj, u32 flags)
{
	if (mobj != NULL) {
		mobj->rendermode &= ~flags;
	}
}

/**
 * @note Address: 0x801B8EA4
 * @note Size: 0x60
 */
void HSD_MObjRemoveAnimByFlags(HSD_MObj* mobj, u32 flags)
{
	if (mobj == NULL)
		return;

	if (flags & MOBJ_ANIM) {
		HSD_AObjRemove(mobj->aobj);
		mobj->aobj = NULL;
	}
	if (flags & TOBJ_ANIM) {
		HSD_TObjRemoveAnimAll(mobj->tobj);
	}
}

/**
 * @note Address: 0x801B8F04
 * @note Size: 0x68
 */
void HSD_MObjAddAnim(HSD_MObj* mobj, HSD_MatAnim* matanim)
{
	if (mobj == NULL)
		return;

	if (matanim != NULL) {
		if (mobj->aobj != NULL) {
			HSD_AObjRemove(mobj->aobj);
		}
		mobj->aobj = HSD_AObjLoadDesc(matanim->aobjdesc);
		HSD_TObjAddAnimAll(mobj->tobj, matanim->texanim);
	}
}

/**
 * @note Address: 0x801B8F6C
 * @note Size: 0x64
 */
void HSD_MObjReqAnimByFlags(HSD_MObj* mobj, f32 startframe, u32 flags)
{
	if (mobj == NULL)
		return;
	if (flags & MOBJ_ANIM) {
		HSD_AObjReqAnim(mobj->aobj, startframe);
	}
	HSD_TObjReqAnimAllByFlags(mobj->tobj, startframe, flags);
}

/**
 * @note Address: N/A
 * @note Size: ?
 */
inline f32 HSD_ClampFloat(f32 val, f32 min, f32 max)
{
	if (val <= min)
		return min;
	if (val >= max)
		return max;
	return val;
}

/**
 * @note Address: 0x801B8FD0
 * @note Size: 0x49C
 */
void MObjUpdateFunc(void* obj, u32 type, FObjData* val)
{
	HSD_MObj* mobj = obj;

	if (mobj == NULL)
		return;

	switch (type) {
	case HSD_A_M_AMBIENT_R:
		if (mobj->mat)
			mobj->mat->ambient.r = 255 * HSD_ClampFloat(val->fv, 0, 1);
		break;
	case HSD_A_M_AMBIENT_G:
		if (mobj->mat)
			mobj->mat->ambient.g = 255 * HSD_ClampFloat(val->fv, 0, 1);
		break;
	case HSD_A_M_AMBIENT_B:
		if (mobj->mat)
			mobj->mat->ambient.b = 255 * HSD_ClampFloat(val->fv, 0, 1);
		break;
	case HSD_A_M_DIFFUSE_R:
		if (mobj->mat)
			mobj->mat->diffuse.r = 255 * HSD_ClampFloat(val->fv, 0, 1);
		break;
	case HSD_A_M_DIFFUSE_G:
		if (mobj->mat)
			mobj->mat->diffuse.g = 255 * HSD_ClampFloat(val->fv, 0, 1);
		break;
	case HSD_A_M_DIFFUSE_B:
		if (mobj->mat)
			mobj->mat->diffuse.b = 255 * HSD_ClampFloat(val->fv, 0, 1);
		break;
	case HSD_A_M_ALPHA:
		if (mobj->mat)
			mobj->mat->alpha = HSD_ClampFloat(1 - val->fv, 0, 1);
		break;
	case HSD_A_M_SPECULAR_R:
		if (mobj->mat)
			mobj->mat->specular.r = 255 * HSD_ClampFloat(val->fv, 0, 1);
		break;
	case HSD_A_M_SPECULAR_G:
		if (mobj->mat)
			mobj->mat->specular.g = 255 * HSD_ClampFloat(val->fv, 0, 1);
		break;
	case HSD_A_M_SPECULAR_B:
		if (mobj->mat)
			mobj->mat->specular.b = 255 * HSD_ClampFloat(val->fv, 0, 1);
		break;
	case HSD_A_M_PE_REF0:
		if (mobj->pe) {
			mobj->pe->ref0 = 255 * HSD_ClampFloat(val->fv, 0, 1);
		}
		break;
	case HSD_A_M_PE_REF1:
		if (mobj->pe) {
			mobj->pe->ref1 = 255 * HSD_ClampFloat(val->fv, 0, 1);
		}
		break;
	case HSD_A_M_PE_DSTALPHA:
		if (mobj->pe) {
			mobj->pe->dst_alpha = 255 * HSD_ClampFloat(val->fv, 0, 1);
		}
		break;
	}
}

/**
 * @note Address: 0x801B946C
 * @note Size: 0x48
 */
void HSD_MObjAnim(HSD_MObj* mobj)
{
	if (mobj == NULL) {
		return;
	}

	HSD_AObjInterpretAnim(mobj->aobj, mobj, HSD_MOBJ_METHOD(mobj)->update);
	HSD_TObjAnimAll(mobj->tobj);
}

/**
 * @note Address: 0x801B94B4
 * @note Size: 0xA0
 */
int MObjLoad(HSD_MObj* mobj, HSD_MObjDesc* desc)
{
	mobj->rendermode = desc->rendermode;
	mobj->tobj       = HSD_TObjLoadDesc(desc->texdesc);
	mobj->mat        = HSD_MaterialAlloc();
	memcpy(mobj->mat, desc->mat, sizeof(HSD_Material));
	mobj->rendermode |= RENDER_TOON;
	if (desc->pedesc) {
		mobj->pe = hsdAllocMemPiece(sizeof(HSD_PEDesc));
		memcpy(mobj->pe, desc->pedesc, sizeof(HSD_PEDesc));
	}
	mobj->aobj = NULL;
	return 0;
}

/**
 * @note Address: 0x801B9554
 * @note Size: 0x58
 */
void HSD_MObjSetDefaultClass(HSD_MObjInfo* info)
{
	if (info != NULL) {
		if (!hsdIsDescendantOf(info, &hsdMObj))
			__assert(__FILE__, 0x14E, "hsdIsDescendantOf(info, &hsdMObj)");
	}
	default_class = info;
}

/**
 * @note Address: 0x801B95AC
 * @note Size: 0x18
 */
HSD_MObjInfo* HSD_MObjGetDefaultClass(void)
{
	return default_class ? default_class : &hsdMObj;
}

/**
 * @note Address: 0x801B95C4
 * @note Size: 0xA0
 */
HSD_MObj* HSD_MObjLoadDesc(HSD_MObjDesc* mobjdesc)
{
	if (mobjdesc) {
		HSD_MObj* mobj;
		HSD_ClassInfo* info;

		if (!mobjdesc->class_name || !(info = hsdSearchClassInfo(mobjdesc->class_name))) {
			mobj = HSD_MObjAlloc();
		} else {
			mobj = hsdNew(info);
			if (mobj == NULL) {
				__assert(__FILE__, 0x175, "mobj");
			}
		}

		HSD_MOBJ_METHOD(mobj)->load(mobj, mobjdesc);
		HSD_MObjCompileTev(mobj);

		return mobj;
	} else {
		return NULL;
	}
}

/**
 * @note Address: 0x801B9664
 * @note Size: 0x9CC
 */
HSD_TExp* MObjMakeTExp(HSD_MObj* mobj, HSD_TObj* tobj_top, HSD_TExp** list)
{
	HSD_TExp *diff, *spec, *ext, *alpha;
	HSD_TExp* cnst;
	HSD_TExp *exp, *exp_2, *exp_3;
	HSD_TObj *tobj, *tobj_2, *tobj_3, *tobj_4, *toon = NULL;
	u32 done = 0;
	u32 diffuse_mode;
	int alpha_mode;
	static u8 one = 255;

	assert_line(0x18B, list);
	*list = NULL;
	for (tobj = tobj_top; tobj; tobj = tobj->next) {
		if (tobj_coord(tobj) == TEX_COORD_TOON) {
			toon = tobj;
		}
	}
	diffuse_mode = mobj->rendermode & RENDER_DIFFUSE_BITS;
	if (diffuse_mode == 0) {
		diffuse_mode = RENDER_DIFFUSE_MAT;
	}
	alpha_mode = mobj->rendermode & RENDER_ALPHA_BITS;
	if (alpha_mode == 0) {
		alpha_mode = diffuse_mode << RENDER_ALPHA_SHIFT;
	}
	exp = HSD_TExpTev(list);
	if (mobj->rendermode & RENDER_DIFFUSE) {
		switch (diffuse_mode) {
		case RENDER_DIFFUSE_VTX:
			HSD_TExpColorOp(exp, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE);
			HSD_TExpColorIn(exp, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_1, HSD_TEXP_ZERO);
			break;
		default:
			cnst = HSD_TExpCnst(&mobj->mat->diffuse, HSD_TE_RGB, HSD_TE_U8, list);
			HSD_TExpColorOp(exp, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE);
			HSD_TExpColorIn(exp, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_RGB, cnst);
			break;
		}
		switch (alpha_mode) {
		case RENDER_ALPHA_VTX:
			cnst = HSD_TExpCnst(&one, HSD_TE_X, HSD_TE_U8, list);
			HSD_TExpAlphaOp(exp, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE);
			HSD_TExpAlphaIn(exp, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_X, cnst);
			break;
		default:
			cnst = HSD_TExpCnst(&mobj->mat->alpha, HSD_TE_X, HSD_TE_F32, list);
			HSD_TExpAlphaOp(exp, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE);
			HSD_TExpAlphaIn(exp, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_X, cnst);
			break;
		}
	} else {
		switch (diffuse_mode) {
		case RENDER_DIFFUSE_MAT:
			cnst = HSD_TExpCnst(&mobj->mat->diffuse, HSD_TE_RGB, HSD_TE_U8, list);
			HSD_TExpColorOp(exp, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE);
			HSD_TExpColorIn(exp, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_RGB, cnst);
			break;
		case RENDER_DIFFUSE_VTX:
			HSD_TExpOrder(exp, toon, GX_COLOR0A0);
			HSD_TExpColorOp(exp, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE);
			HSD_TExpColorIn(exp, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_RGB,
			                toon ? HSD_TEXP_TEX : HSD_TEXP_RAS);
			break;
		default:
			cnst = HSD_TExpCnst(&mobj->mat->diffuse, HSD_TE_RGB, HSD_TE_U8, list);
			HSD_TExpOrder(exp, toon, GX_COLOR0A0);
			HSD_TExpColorOp(exp, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE);
			HSD_TExpColorIn(exp, HSD_TE_RGB, toon ? HSD_TEXP_TEX : HSD_TEXP_RAS, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_0, HSD_TEXP_ZERO,
			                HSD_TE_RGB, cnst);
			break;
		}
		switch (alpha_mode) {
		case RENDER_ALPHA_MAT:
			cnst = HSD_TExpCnst(&mobj->mat->alpha, HSD_TE_X, HSD_TE_F32, list);
			HSD_TExpAlphaOp(exp, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE);
			HSD_TExpAlphaIn(exp, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_X, cnst);
			break;
		case RENDER_ALPHA_VTX:
			HSD_TExpOrder(exp, toon, GX_COLOR0A0);
			HSD_TExpAlphaOp(exp, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE);
			HSD_TExpAlphaIn(exp, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_A, HSD_TEXP_RAS);
			break;
		default:
			cnst = HSD_TExpCnst(&mobj->mat->alpha, HSD_TE_X, HSD_TE_F32, list);
			HSD_TExpOrder(exp, toon, GX_COLOR0A0);
			HSD_TExpAlphaOp(exp, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE);
			HSD_TExpAlphaIn(exp, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_A, HSD_TEXP_RAS, HSD_TE_X, cnst, HSD_TE_0, HSD_TEXP_ZERO);
			break;
		}
	}
	diff  = exp;
	alpha = exp;
	for (tobj_2 = tobj_top; tobj_2 != NULL; tobj_2 = tobj_2->next) {
		if ((tobj_2->flags & (TEX_LIGHTMAP_DIFFUSE | TEX_LIGHTMAP_AMBIENT)) && tobj_2->map_id != GX_TEXMAP_NULL) {
			HSD_TOBJ_METHOD(tobj_2)->make_texp(tobj_2, (TEX_LIGHTMAP_DIFFUSE | TEX_LIGHTMAP_AMBIENT), done, &diff, &alpha, list);
		}
	}
	done |= (TEX_LIGHTMAP_DIFFUSE | TEX_LIGHTMAP_AMBIENT);

	if (mobj->rendermode & RENDER_DIFFUSE) {
		diffuse_mode = alpha_mode & RENDER_ALPHA_VTX;
		if (diffuse_mode) {
			exp_2 = HSD_TExpTev(list);
			HSD_TExpOrder(exp_2, NULL, GX_COLOR1A1);
			HSD_TExpColorOp(exp_2, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE);
			HSD_TExpColorIn(exp_2, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_RGB, diff);
			HSD_TExpAlphaOp(exp_2, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE);
			HSD_TExpAlphaIn(exp_2, HSD_TE_A, alpha, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_A, HSD_TEXP_RAS, HSD_TE_0, HSD_TEXP_ZERO);
			diff  = exp_2;
			alpha = exp_2;
		}
		exp_2 = HSD_TExpTev(list);
		if (toon != NULL) {
			HSD_TExpOrder(exp_2, toon, GX_COLOR0A0);
			HSD_TExpColorOp(exp_2, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE);
			HSD_TExpColorIn(exp_2, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_RGB, diff, HSD_TE_RGB, HSD_TEXP_TEX, HSD_TE_0, HSD_TEXP_ZERO);
		} else {
			HSD_TExpOrder(exp_2, NULL, GX_COLOR0A0);
			HSD_TExpColorOp(exp_2, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE);
			HSD_TExpColorIn(exp_2, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_RGB, diff, HSD_TE_RGB, HSD_TEXP_RAS, HSD_TE_0, HSD_TEXP_ZERO);
		}
		diff = exp_2;
		if (diffuse_mode) {
			HSD_TExpAlphaOp(exp_2, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE);
			HSD_TExpAlphaIn(exp_2, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_A, alpha, HSD_TE_A, HSD_TEXP_RAS, HSD_TE_0, HSD_TEXP_ZERO);
		} else {
			HSD_TExpAlphaOp(exp_2, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE);
			HSD_TExpAlphaIn(exp_2, HSD_TE_A, alpha, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_A, HSD_TEXP_RAS, HSD_TE_0, HSD_TEXP_ZERO);
		}
		alpha = exp_2;
	}

	if (mobj->rendermode & RENDER_SPECULAR) {
		cnst  = HSD_TExpCnst(&mobj->mat->specular, HSD_TE_RGB, HSD_TE_U8, list);
		exp_3 = HSD_TExpTev(list);
		HSD_TExpColorOp(exp_3, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE);
		HSD_TExpColorIn(exp_3, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_RGB, cnst);
		spec = exp_3;

		for (tobj_3 = tobj_top; tobj_3 != NULL; tobj_3 = tobj_3->next) {
			if ((tobj_3->flags & TEX_LIGHTMAP_SPECULAR) && tobj_3->map_id != GX_TEXMAP_NULL) {
				HSD_TOBJ_METHOD(tobj_3)->make_texp(tobj_3, TEX_LIGHTMAP_SPECULAR, done, &spec, &alpha, list);
			}
		}
		done |= TEX_LIGHTMAP_SPECULAR;

		exp_3 = HSD_TExpTev(list);
		HSD_TExpOrder(exp_3, NULL, GX_COLOR1A1);
		HSD_TExpColorOp(exp_3, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE);
		HSD_TExpColorIn(exp_3, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_RGB, spec, HSD_TE_RGB, HSD_TEXP_RAS, HSD_TE_0, HSD_TEXP_ZERO);
		spec = exp_3;

		exp_3 = HSD_TExpTev(list);
		HSD_TExpColorOp(exp_3, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE);
		HSD_TExpColorIn(exp_3, HSD_TE_RGB, spec, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_RGB, diff);
		diff = exp_3;
	}

	ext = diff;

	for (tobj_4 = tobj_top; tobj_4 != NULL; tobj_4 = tobj_4->next) {
		if ((tobj_4->flags & TEX_LIGHTMAP_EXT) && tobj_4->map_id != GX_TEXMAP_NULL) {
			HSD_TOBJ_METHOD(tobj_4)->make_texp(tobj_4, TEX_LIGHTMAP_EXT, done, &ext, &alpha, list);
		}
	}

	if (ext != alpha || HSD_TExpGetType(ext) != HSD_TE_TEV || HSD_TExpGetType(alpha) != HSD_TE_TEV) {
		exp_2 = HSD_TExpTev(list);
		HSD_TExpColorOp(exp_2, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE);
		HSD_TExpColorIn(exp_2, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_RGB, ext);
		HSD_TExpAlphaOp(exp_2, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_ENABLE);
		HSD_TExpAlphaIn(exp_2, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_0, HSD_TEXP_ZERO, HSD_TE_A, alpha);
		return exp_2;
	}

	return ext;
}

/**
 * @note Address: 0x801BA030
 * @note Size: 0x120
 */
void HSD_MObjCompileTev(HSD_MObj* mobj)
{
	HSD_TObj* sp8;
	HSD_TObj** var_r31 = NULL;
	HSD_TExp* texp;

	if (mobj == NULL) {
		return;
	}
	if (mobj->tevdesc != NULL) {
		HSD_TExpFreeTevDesc(mobj->tevdesc);
		mobj->tevdesc = NULL;
	}
	if (mobj->texp != NULL) {
		HSD_TExpFreeList(mobj->texp, HSD_TE_ALL, 1);
		mobj->texp = NULL;
	}
	sp8 = mobj->tobj;
	if (mobj->rendermode & RENDER_SHADOW && tobj_shadows != NULL) {
		var_r31 = &sp8;
		while (*var_r31 != NULL) {
			var_r31 = &(*var_r31)->next;
		}
		*var_r31 = tobj_shadows;
	}
	if (mobj->rendermode & RENDER_TOON && tobj_toon != NULL && tobj_toon->imagedesc != NULL) {
		tobj_toon->next = sp8;
		sp8             = tobj_toon;
	}
	HSD_TObjAssignResources(sp8);
	texp = HSD_MOBJ_METHOD(mobj)->make_texp(mobj, sp8, &mobj->texp);
	HSD_TExpCompile(texp, &mobj->tevdesc, &mobj->texp);
	if (var_r31 != NULL) {
		*var_r31 = NULL;
	}
}

/**
 * @note Address: 0x801BA150
 * @note Size: 0x78
 */
void MObjSetupTev(HSD_MObj* mobj, HSD_TObj* tobj, u32 rendermode)
{
	if (mobj->tevdesc == NULL) {
		__assert(__FILE__, 0x31E, "mobj->tevdesc");
	}
	HSD_TExpSetupTev(mobj->tevdesc, mobj->texp);
	HSD_TObjSetupVolatileTev(tobj, rendermode);
}

/**
 * @note Address: 0x801BA1C8
 * @note Size: 0x144
 */
void HSD_MObjSetup(HSD_MObj* mobj, u32 rendermode)
{
	HSD_TObj* tobj;
	GXColor sp10;
	GXColor spC;
	GXColor sp8;
	HSD_TObj** tail;

	HSD_StateInitTev();
	rendermode = mobj->rendermode;
	sp8        = mobj->mat->specular;
	spC        = mobj->mat->diffuse;
	sp10       = mobj->mat->ambient;
	HSD_SetMaterialColor(&sp10, &spC, &sp8, mobj->mat, mobj->mat->alpha);
	if (rendermode & RENDER_SPECULAR) {
		HSD_SetMaterialShininess(mobj->mat, mobj->mat->shininess);
	}
	tail = NULL;
	tobj = mobj->tobj;
	if (rendermode & RENDER_SHADOW && tobj_shadows != NULL) {
		tail = &tobj;
		while (*tail != NULL) {
			tail = &(*tail)->next;
		}
		*tail = tobj_shadows;
	}
	if (rendermode & RENDER_TOON && tobj_toon != NULL && tobj_toon->imagedesc != NULL) {
		tobj_toon->next = tobj;
		tobj            = tobj_toon;
	}
	HSD_TObjSetup(tobj);
	HSD_TObjSetupTextureCoordGen(tobj);
	HSD_MOBJ_METHOD(mobj)->setup_tev(mobj, tobj, rendermode);
	HSD_SetupRenderModeWithCustomPE(rendermode, mobj->pe);
	if (tail != NULL) {
		*tail = NULL;
	}
}

/**
 * @note Address: 0x801BA30C
 * @note Size: 0x24
 */
void HSD_MObjUnset(HSD_MObj* mobj, u32 rendermode)
{
	HSD_TObjSetup(NULL);
}

static HSD_TObjDesc tobj_toon_desc = {
	NULL,
	NULL,
	GX_TEXMAP7,
	GX_TG_COLOR0,
	{ 0.0F, 0.0F, 0.0F },
	{ 0.0F, 0.0F, 0.0F },
	{ 0.0F, 0.0F, 0.0F },
	GX_CLAMP,
	GX_CLAMP,
	0,
	0,
	TEX_COORD_TOON,
	1.0F,
	GX_LINEAR,
	NULL,
	NULL,
	NULL,
	NULL,
};

/**
 * @note Address: 0x801BA330
 * @note Size: 0x7C
 */
void SetToonTObj(HSD_ImageDesc* imagedesc)
{
	if (tobj_toon == NULL) {
		tobj_toon = HSD_TObjLoadDesc(&tobj_toon_desc);
		if (tobj_toon == NULL) {
			OSReport("cannot allocate tobj for toon.");
			__assert(__FILE__, 0x396, "tobj_toon");
		}
	}
	tobj_toon->imagedesc = imagedesc;
}

/**
 * @note Address: 0x801BA3AC
 * @note Size: 0x30
 */
void SetMaterialAmbientColor(HSD_MObj* mobj, u8 r, u8 g, u8 b)
{
	// sets the ambient color, no name evidence
	if (mobj == NULL || mobj->mat == NULL) {
		return;
	}
	mobj->mat->ambient.r = r;
	mobj->mat->ambient.g = g;
	mobj->mat->ambient.b = b;
}

/**
 * @note Address: 0x801BA3DC
 * @note Size: 0x30
 */
void HSD_MObjSetDiffuseColor(HSD_MObj* mobj, u8 r, u8 g, u8 b)
{
	if (mobj == NULL || mobj->mat == NULL) {
		return;
	}
	mobj->mat->diffuse.r = r;
	mobj->mat->diffuse.g = g;
	mobj->mat->diffuse.b = b;
}

/**
 * @note Address: 0x801BA40C
 * @note Size: 0x20
 */
void HSD_MObjSetAlpha(HSD_MObj* mobj, f32 alpha)
{
	if (mobj == NULL || mobj->mat == NULL) {
		return;
	}
	mobj->mat->alpha = alpha;
}

/**
 * @note Address: 0x801BA42C
 * @note Size: 0x18
 */
HSD_TObj* HSD_MObjGetTObj(HSD_MObj* mobj)
{
	if (mobj == NULL) {
		return NULL;
	}
	return mobj->tobj;
}

/**
 * @note Address: 0x801BA444
 * @note Size: 0x50
 */
void HSD_MObjRemove(HSD_MObj* mobj)
{
	if (mobj != NULL) {
		HSD_CLASS_METHOD(mobj)->release((HSD_Class*)mobj);
		HSD_CLASS_METHOD(mobj)->destroy((HSD_Class*)mobj);
	}
}

/**
 * @note Address: 0x801BA494
 * @note Size: 0x5C
 */
HSD_MObj* HSD_MObjAlloc(void)
{
	HSD_MObj* temp_r3;

	temp_r3 = hsdNew((HSD_ClassInfo*)HSD_MObjGetDefaultClass());
	if (temp_r3 == NULL) {
		__assert(__FILE__, 0x44A, "mobj");
	}
	return temp_r3;
}

/**
 * @note Address: 0x801BA4F0
 * @note Size: 0x60
 */
HSD_Material* HSD_MaterialAlloc(void)
{
	HSD_Material* mat = hsdAllocMemPiece(sizeof(HSD_Material));
	if (mat == NULL)
		__assert(__FILE__, 0x466, "mat");
	memset(mat, 0, sizeof(HSD_Material));
	mat->alpha = 1.0F;
	return mat;
}

/**
 * @note Address: 0x801BA550
 * @note Size: 0x64
 */
void HSD_MObjAddShadowTexture(HSD_TObj* tobj)
{
	HSD_TObj* tp;
	if (tobj == NULL)
		__assert(__FILE__, 0x495, "tobj");
	for (tp = tobj_shadows; tp != NULL; tp = tp->next) {
		if (tp == tobj)
			return;
	}
	tobj->next   = tobj_shadows;
	tobj_shadows = tobj;
}

/**
 * @note Address: 0x801BA5B4
 * @note Size: 0x64
 */
void HSD_MObjDeleteShadowTexture(HSD_TObj* tobj)
{
	if (tobj != NULL) {
		HSD_TObj** tp;
		for (tp = &tobj_shadows; *tp != NULL; tp = &(*tp)->next) {
			if (*tp == tobj) {
				*tp        = tobj->next;
				tobj->next = NULL;
				return;
			}
		}
	} else {
		while (tobj_shadows != NULL) {
			HSD_TObj* next     = tobj_shadows->next;
			tobj_shadows->next = NULL;
			tobj_shadows       = next;
		}
	}
}

/**
 * @note Address: 0x801BA618
 * @note Size: 0x9C
 */
void MObjRelease(HSD_Class* o)
{
	HSD_MObj* mobj = HSD_MOBJ(o);

	HSD_AObjRemove(mobj->aobj);
	hsdFreeMemPiece(mobj->mat, sizeof(HSD_Material));
	HSD_TObjRemoveAll(mobj->tobj);

	if (mobj->tevdesc != NULL)
		HSD_TExpFreeTevDesc(mobj->tevdesc);
	if (mobj->texp != NULL)
		HSD_TExpFreeList(mobj->texp, HSD_TE_ALL, 1);
	if (mobj->pe != NULL)
		hsdFreeMemPiece(mobj->pe, sizeof(HSD_PEDesc));
	HSD_PARENT_INFO(&hsdMObj)->release(o);
}

/**
 * @note Address: 0x801BA6B4
 * @note Size: 0x64
 */
void MObjAmnesia(HSD_ClassInfo* info)
{
	if (info == HSD_CLASS_INFO(default_class)) {
		default_class = NULL;
	}
	if (info == HSD_CLASS_INFO(&hsdMObj)) {
		tobj_toon    = NULL;
		tobj_shadows = NULL;
	}
	HSD_PARENT_INFO(&hsdMObj)->amnesia(info);
}

/**
 * @note Address: 0x801BA718
 * @note Size: 0xB0
 */
void MObjInfoInit(void)
{
	hsdInitClassInfo(HSD_CLASS_INFO(&hsdMObj), HSD_CLASS_INFO(&hsdClass), "sysdolphin_base_library", "hsd_mobj", sizeof(HSD_MObjInfo),
	                 sizeof(HSD_MObj));

	HSD_CLASS_INFO(&hsdMObj)->release  = MObjRelease;
	HSD_CLASS_INFO(&hsdMObj)->amnesia  = MObjAmnesia;
	HSD_MOBJ_INFO(&hsdMObj)->setup     = HSD_MObjSetup;
	HSD_MOBJ_INFO(&hsdMObj)->unset     = HSD_MObjUnset;
	HSD_MOBJ_INFO(&hsdMObj)->load      = MObjLoad;
	HSD_MOBJ_INFO(&hsdMObj)->make_texp = MObjMakeTExp;
	HSD_MOBJ_INFO(&hsdMObj)->setup_tev = MObjSetupTev;
	HSD_MOBJ_INFO(&hsdMObj)->update    = MObjUpdateFunc;
}
