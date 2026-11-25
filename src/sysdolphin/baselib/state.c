#include <global.h>

#include <dolphin/gx/gx.h>
#include <sysdolphin/tev.h>
#include <sysdolphin/tev_state.h>

#include <sysdolphin/lobj.h>
#include <sysdolphin/lobj_setup.h>
#include <sysdolphin/util.h>

/**
 * @note Address: N/A
 */
static inline u32 StateNormalizeBool(u32 value)
{
	return value ? 1 : 0;
}

static u8 state_line_width;
static s32 state_line_tex_offsets;
static u8 state_point_size;
static s32 state_point_tex_offsets;
static s32 state_cull_mode;
static s32 state_blend_type;
static s32 state_src_factor;
static s32 state_dst_factor;
static s32 state_logic_op;
static u8 state_z_enable;
static s32 state_z_func;
static u8 state_z_update;
static s32 state_alpha_comp0;
static u8 state_alpha_ref0;
static s32 state_alpha_op;
static s32 state_alpha_comp1;
static u8 state_alpha_ref1;
static u8 state_color_update;
static u8 state_alpha_update;
static u8 state_enable_dst_alpha;
static u8 state_dst_alpha;
static u8 state_before_tex;
static u8 state_dither;

static struct {
	GXColor ambient;
	GXColor diffuse;
	GXColor specular;
	u8 alpha;
	f32 shininess;
} matstate;

void HSD_SetupPEMode(u32 rendermode, HSD_PEDesc* pe);
void HSD_DisableChannelLighting(s32 chan);
void HSD_ClearVtxDesc(void);
void HSD_SetupChannelMode(u32 rendermode);

/**
 * @note Address: 0x801AA888
 * @note Size: 0x234
 */
void HSD_SetupChannelMode(u32 rendermode)
{
	s32 diffuse;
	s32 alpha;
	s32 use_specular     = 0;
	s32 use_vertex_alpha = 0;

	diffuse = rendermode & RENDER_DIFFUSE_BITS;
	if (diffuse == 0) {
		diffuse = RENDER_DIFFUSE_MAT;
	}
	alpha = rendermode & RENDER_ALPHA_BITS;
	if (alpha == 0) {
		alpha = diffuse << RENDER_ALPHA_SHIFT;
	}

	if (rendermode & RENDER_SPECULAR) {
		static HSD_Chan ch
		    = { NULL, GX_COLOR1, 0, { 0 }, { 255, 255, 255, 255 }, 1, GX_SRC_REG, GX_SRC_REG, 0, GX_DF_NONE, GX_AF_SPEC, NULL };
		s32 i;
		s32 max;

		ch.light_mask = HSD_LObjGetLightMaskSpecular();
		HSD_SetupChannel(&ch);
		use_specular = 1;
		max          = HSD_LObjGetNbActive();
		for (i = 0; i < max; i++) {
			HSD_LObj* light = HSD_LObjGetActiveByIndex(i);
			if (light != NULL) {
				GXColor color = light->color;
				HSD_LObjSetup(light, &color, matstate.shininess);
			}
		}
	}

	if (rendermode & RENDER_DIFFUSE) {
		static HSD_Chan ch
		    = { NULL, GX_COLOR0, 0, { 0 }, { 255, 255, 255, 255 }, 1, GX_SRC_REG, GX_SRC_REG, 0, GX_DF_CLAMP, GX_AF_SPOT, NULL };
		static HSD_Chan ch_a       = { NULL, GX_ALPHA0, 0, { 0 }, { 0 }, 1, GX_SRC_REG, GX_SRC_REG, 0, GX_DF_CLAMP, GX_AF_SPOT, NULL };
		static HSD_Chan ch_vtx_a   = { NULL, GX_ALPHA0, 0, { 0 }, { 0 }, 0, GX_SRC_REG, GX_SRC_VTX, 0, GX_DF_CLAMP, GX_AF_SPOT, NULL };
		static GXColor dark_matter = { 0, 0, 0, 255 };
		HSD_LObj* ambient          = HSD_LObjGetActiveByID(HSD_LIGHT_AMBIENT);
		u8 amb_alpha;

		if (ambient != NULL && (ambient->flags & LOBJ_DIFFUSE)) {
			HSD_MulColor(&matstate.ambient, &ambient->color, &ch.amb_color);
		} else {
			ch.amb_color = dark_matter;
		}
		ch.mat_src    = (diffuse & RENDER_DIFFUSE_VTX) ? GX_SRC_VTX : GX_SRC_REG;
		ch.light_mask = HSD_LObjGetLightMaskDiffuse();
		HSD_SetupChannel(&ch);

		if (alpha & RENDER_ALPHA_VTX) {
			ch_a.chan        = GX_ALPHA1;
			use_vertex_alpha = 1;
			HSD_SetupChannel(&ch_vtx_a);
		} else {
			ch_a.chan = GX_ALPHA0;
		}
		ch_a.light_mask = HSD_LObjGetLightMaskAlpha();
		if (ambient != NULL && (ambient->flags & LOBJ_ALPHA)) {
			amb_alpha = ambient->color.a;
		} else {
			amb_alpha = 0;
		}
		if (ch_a.light_mask != 0) {
			ch_a.enable      = 1;
			ch_a.mat_color.a = 255;
			ch_a.amb_color.a = amb_alpha;
		} else {
			ch_a.enable      = 0;
			ch_a.mat_color.a = amb_alpha;
		}
		HSD_SetupChannel(&ch_a);
	} else {
		static HSD_Chan ch
		    = { NULL, GX_COLOR0, 0, { 0 }, { 255, 255, 255, 255 }, 0, GX_SRC_REG, GX_SRC_VTX, 0, GX_DF_NONE, GX_AF_NONE, NULL };
		static HSD_Chan ch_a = { NULL, GX_ALPHA0, 0, { 0 }, { 0, 0, 0, 255 }, 0, GX_SRC_REG, GX_SRC_VTX, 0, GX_DF_NONE, GX_AF_NONE, NULL };

		ch.mat_src = (diffuse & RENDER_DIFFUSE_VTX) ? GX_SRC_VTX : GX_SRC_REG;
		HSD_SetupChannel(&ch);
		ch_a.mat_src = (alpha & RENDER_ALPHA_VTX) ? GX_SRC_VTX : GX_SRC_REG;
		HSD_SetupChannel(&ch_a);
	}

	if (use_specular) {
		if (!use_vertex_alpha) {
			HSD_DisableChannelLighting(GX_ALPHA1);
		}
		HSD_StateSetNumChans(2);
	} else if (use_vertex_alpha) {
		HSD_DisableChannelLighting(GX_COLOR1);
		HSD_StateSetNumChans(2);
	} else {
		HSD_DisableChannelLighting(GX_COLOR1A1);
		HSD_StateSetNumChans(1);
	}
}

/**
 * @note Address: 0x801AA3A8
 * @note Size: 0x4E0
 */
void HSD_SetupPEMode(u32 rendermode, HSD_PEDesc* pe)
{
	u8 z_update;

	if (pe != NULL) {
		HSD_StateSetColorUpdate(pe->flags & HSD_PE_COLOR_UPDATE);
		HSD_StateSetAlphaUpdate(pe->flags & HSD_PE_ALPHA_UPDATE);
		HSD_StateSetDstAlpha(pe->flags & HSD_PE_DST_ALPHA_ENABLE, pe->dst_alpha);
		HSD_StateSetBlendMode(pe->type, pe->src_factor, pe->dst_factor, pe->logic_op);
		HSD_StateSetZMode(pe->flags & HSD_PE_Z_TEST, pe->z_comp, pe->flags & HSD_PE_Z_UPDATE);
		HSD_StateSetZCompLoc(pe->flags & HSD_PE_Z_COMP_BEFORE_TEX);
		HSD_StateSetAlphaCompare(pe->alpha_comp0, pe->ref0, pe->alpha_op, pe->alpha_comp1, pe->ref1);
		HSD_StateSetDither(pe->flags & HSD_PE_DITHER);
		return;
	}

	HSD_StateSetColorUpdate(GX_TRUE);
	HSD_StateSetAlphaUpdate(GX_FALSE);
	HSD_StateSetDstAlpha(GX_FALSE, 0);
	HSD_StateSetBlendMode((rendermode & RENDER_XLU) ? GX_BM_BLEND : GX_BM_NONE, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
	z_update = (rendermode & RENDER_NO_ZUPDATE) ? GX_FALSE : GX_TRUE;
	HSD_StateSetZMode(GX_TRUE, (rendermode & RENDER_ZMODE_ALWAYS) ? GX_ALWAYS : GX_LEQUAL, z_update);
	if (!(rendermode & RENDER_NO_ZUPDATE) && (rendermode & RENDER_XLU)) {
		HSD_StateSetZCompLoc(GX_FALSE);
		HSD_StateSetAlphaCompare(GX_GREATER, 0, GX_AOP_AND, GX_GREATER, 0);
	} else {
		HSD_StateSetZCompLoc(GX_TRUE);
		HSD_StateSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 0);
	}
	HSD_StateSetDither(GX_TRUE);
}

/**
 * @note Address: 0x801AA318
 * @note Size: 0x90
 */
void HSD_SetupRenderModeWithCustomPE(u32 rendermode, HSD_PEDesc* pe)
{
	if (HSD_StateGetNumTevStages() == 0) {
		HSD_TevDesc tevdesc;
		tevdesc.flags           = 0;
		tevdesc.stage           = HSD_StateAssignTev();
		tevdesc.coord           = 0xFF;
		tevdesc.map             = 0xFF;
		tevdesc.color           = GX_COLOR0A0;
		tevdesc.u.tevop.tevmode = GX_PASSCLR;
		HSD_SetupTevStage(&tevdesc);
	}

	HSD_SetupPEMode(rendermode, pe);
	HSD_SetTevRegAll();
	HSD_StateSetNumTevStages();
	HSD_StateSetNumTexGens();
	HSD_SetupChannelMode(rendermode);
}

/**
 * @note Address: 0x801AA2A4
 * @note Size: 0x74
 */
void HSD_SetMaterialColor(GXColor ambient, GXColor diffuse, GXColor specular, f32 alpha)
{
	matstate.ambient  = ambient;
	matstate.diffuse  = diffuse;
	matstate.specular = specular;
	matstate.alpha    = 255.0F * HSD_ClampFloat(alpha, 0.0F, 1.0F);
}

/**
 * @note Address: 0x801AA294
 * @note Size: 0x10
 */
void HSD_SetMaterialShininess(f32 shininess)
{
	matstate.shininess = shininess;
}

/**
 * @note Address: 0x801AA230
 * @note Size: 0x64
 */
void HSD_StateSetLineWidth(u8 width, s32 tex_offsets)
{
	if (state_line_width != width || state_line_tex_offsets != tex_offsets) {
		GXSetLineWidth(width, tex_offsets);
		state_line_width       = width;
		state_line_tex_offsets = tex_offsets;
	}
}

/**
 * @note Address: 0x801AA1F4
 * @note Size: 0x3C
 */
void HSD_StateSetCullMode(s32 mode)
{
	if (state_cull_mode != mode) {
		GXSetCullMode(mode);
		state_cull_mode = mode;
	}
}

/**
 * @note Address: N/A
 * @note Size: 0xA0
 */
void HSD_StateSetBlendMode(s32 type, s32 src_factor, s32 dst_factor, s32 op)
{
	if (state_blend_type != type || state_src_factor != src_factor || state_dst_factor != dst_factor || state_logic_op != op) {
		GXSetBlendMode(type, src_factor, dst_factor, op);
		state_blend_type = type;
		state_src_factor = src_factor;
		state_dst_factor = dst_factor;
		state_logic_op   = op;
	}
}

/**
 * @note Address: 0x801AA164
 * @note Size: 0x90
 */
void HSD_StateSetZMode(u32 enable, s32 func, u32 update)
{
	u32 z_enable = StateNormalizeBool(enable);
	u32 z_update = StateNormalizeBool(update);

	if (state_z_enable != z_enable || state_z_func != func || state_z_update != z_update) {
		GXSetZMode(z_enable, func, z_update);
		state_z_enable = z_enable;
		state_z_func   = func;
		state_z_update = z_update;
	}
}

/**
 * @note Address: 0x801AA100
 * @note Size: 0x64
 */
void HSD_StateSetPointSize(u8 size, s32 tex_offsets)
{
	if (state_point_size != size || state_point_tex_offsets != tex_offsets) {
		GXSetPointSize(size, tex_offsets);
		state_point_size        = size;
		state_point_tex_offsets = tex_offsets;
	}
}

/**
 * @note Address: N/A
 * @note Size: 0xB0
 */
void HSD_StateSetAlphaCompare(s32 comp0, u8 ref0, s32 op, s32 comp1, u8 ref1)
{
	if (state_alpha_comp0 != comp0 || state_alpha_ref0 != ref0 || state_alpha_op != op || state_alpha_comp1 != comp1
	    || state_alpha_ref1 != ref1) {
		GXSetAlphaCompare(comp0, ref0, op, comp1, ref1);
		state_alpha_comp0 = comp0;
		state_alpha_ref0  = ref0;
		state_alpha_op    = op;
		state_alpha_comp1 = comp1;
		state_alpha_ref1  = ref1;
	}
}

/**
 * @note Address: 0x801AA0B8
 * @note Size: 0x48
 */
void HSD_StateSetColorUpdate(u32 enable)
{
	enable = enable ? 1 : 0;
	if (state_color_update != enable) {
		GXSetColorUpdate(enable);
		state_color_update = (u8)enable;
	}
}

/**
 * @note Address: 0x801AA070
 * @note Size: 0x48
 */
void HSD_StateSetAlphaUpdate(u32 enable)
{
	enable = StateNormalizeBool(enable);
	if (state_alpha_update != enable) {
		GXSetAlphaUpdate(enable);
		state_alpha_update = (u8)enable;
	}
}

/**
 * @note Address: N/A
 * @note Size: 0x6C
 */
void HSD_StateSetDstAlpha(u32 enable, u8 value)
{
	enable = StateNormalizeBool(enable);
	if (state_enable_dst_alpha != enable || state_dst_alpha != value) {
		GXSetDstAlpha(enable, value);
		state_enable_dst_alpha = (u8)enable;
		state_dst_alpha        = value;
	}
}

/**
 * @note Address: N/A
 * @note Size: 0x48
 */
void HSD_StateSetZCompLoc(u32 before_tex)
{
	before_tex = StateNormalizeBool(before_tex);
	if (state_before_tex != before_tex) {
		GXSetZCompLoc(before_tex);
		state_before_tex = (u8)before_tex;
	}
}

/**
 * @note Address: N/A
 * @note Size: 0x48
 */
void HSD_StateSetDither(u32 enable)
{
	enable = enable ? 1 : 0;
	if (state_dither != enable) {
		GXSetDither(enable);
		state_dither = (u8)enable;
	}
}

/**
 * @note Address: 0x801AA054
 * @note Size: 0x1C
 */
void _HSD_StateInvalidatePrimitive(void)
{
	state_point_size = 0;
	state_cull_mode  = -1;
	state_line_width = 0xFF;
}

/**
 * @note Address: 0x801AA034
 * @note Size: 0x20
 */
void _HSD_StateInvalidateVtxAttr(void)
{
	HSD_ClearVtxDesc();
}

/**
 * @note Address: 0x801A9FDC
 * @note Size: 0x58
 */
void _HSD_StateInvalidateRenderMode(void)
{
	state_blend_type       = -1;
	state_src_factor       = -1;
	state_dst_factor       = -1;
	state_logic_op         = -1;
	state_z_enable         = 0xFF;
	state_z_func           = -1;
	state_z_update         = 0xFF;
	state_alpha_comp0      = -1;
	state_alpha_ref0       = 0;
	state_alpha_op         = -1;
	state_alpha_comp1      = -1;
	state_alpha_ref1       = 0;
	state_color_update     = 0xFF;
	state_alpha_update     = 0xFF;
	state_enable_dst_alpha = 0xFF;
	state_dst_alpha        = 0;
	state_before_tex       = 0xFF;
	state_dither           = 0xFF;
}

// clang-format off
struct invalidate_funcs_t {
	int mask;
	void (*func)(void);
} invalidate_funcs[] = {
	{ HSD_STATE_PRIMITIVE, _HSD_StateInvalidatePrimitive },
	{ HSD_STATE_VTX_ATTR, _HSD_StateInvalidateVtxAttr },
	{ HSD_STATE_COLOR_CHANNEL, _HSD_StateInvalidateColorChannel },
	{ HSD_STATE_TEV_STAGE, _HSD_StateInvalidateTevStage },
	{ HSD_STATE_TEV_REGISTER, _HSD_StateInvalidateTevRegister },
	{ HSD_STATE_TEX_COORD_GEN, _HSD_StateInvalidateTexCoordGen },
	{ HSD_STATE_RENDER_MODE, _HSD_StateInvalidateRenderMode },
	{ HSD_STATE_NONE },
};
// clang-format on

/**
 * @note Address: 0x801A9F7C
 * @note Size: 0x60
 */
void HSD_StateInvalidate(s32 mask)
{
	int i;

	for (i = 0; invalidate_funcs[i].mask != HSD_STATE_NONE; i++) {
		if (mask & invalidate_funcs[i].mask) {
			invalidate_funcs[i].func();
		}
	}
}
