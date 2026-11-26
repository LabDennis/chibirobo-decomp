#include <global.h>
#include <dolphin/gx/gx.h>
#include <sysdolphin/tev.h>
#include <sysdolphin/lobj.h>
#include <sysdolphin/lobj_setup.h>
#include <sysdolphin/util.h>
/* XD GX TEV stage and color-channel state cache. */
#define HSD_ASSERT(line, cond) ((cond) ? (void)0 : __assert("tev.c", line, #cond))

s32 HSD_TevStage2Num(s32 stage);
s32 HSD_TexCoordID2Num(s32 id);
static struct TevReg_t {
	GXColorS10 color;
	BOOL pending;
} TevReg[4] = { 0 };

static HSD_Chan prev_ch[4] = {
	{ NULL, 0, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, 255, 0, 0, 0, 0, 2, NULL },
	{ NULL, 1, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, 255, 0, 0, 0, 0, 2, NULL },
	{ NULL, 2, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, 255, 0, 0, 0, 0, 2, NULL },
	{ NULL, 3, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, 255, 0, 0, 0, 0, 2, NULL },
};

static HSD_ObjAllocData rd_alloc_data;
static HSD_ObjAllocData reg_alloc_data;
static HSD_ObjAllocData ch_alloc_data;

static s32 current_tev;
static s32 prev_amb_invalid[2];
static s32 prev_mat_invalid[2];
static s32 prev_num_chans;
static s32 num_tex_gens;

void HSD_RenderInitAllocData(void)
{
	HSD_ObjAllocInit(&rd_alloc_data, 28, 4);
	HSD_ObjAllocInit(&reg_alloc_data, 20, 4);
	HSD_ObjAllocInit(&ch_alloc_data, sizeof(HSD_Chan), 4);
}

HSD_ObjAllocData* HSD_RenderGetAllocData(void)
{
	return &rd_alloc_data;
}

HSD_ObjAllocData* HSD_TevRegGetAllocData(void)
{
	return &reg_alloc_data;
}

HSD_ObjAllocData* HSD_ChanGetAllocData(void)
{
	return &ch_alloc_data;
}

static BOOL UpdateChannelColor(GXChannelID chan, GXColor* cached, const GXColor* color)
{
	BOOL rgbOnly = chan == GX_COLOR0 || chan == GX_COLOR1;
	BOOL updateRGB = rgbOnly || chan == GX_COLOR0A0 || chan == GX_COLOR1A1;
	BOOL sameRGB = !updateRGB || (cached->r == color->r && cached->g == color->g && cached->b == color->b);

	if (sameRGB && (rgbOnly || cached->a == color->a)) {
		return FALSE;
	}
	if (updateRGB) {
		cached->r = color->r;
		cached->g = color->g;
		cached->b = color->b;
	}
	if (!rgbOnly) {
		cached->a = color->a;
	}
	return TRUE;
}

void HSD_SetupChannel(HSD_Chan* ch)
{
	s32 idx;
	GXChannelID chan;
	s32 no;

	if (ch == NULL || ch->chan == GX_COLOR_NULL) {
		return;
	}

	chan = ch->chan;
	idx  = chan & 3;
	no   = chan & 1;
	if (ch->enable != GX_DISABLE && ch->amb_src == GX_SRC_REG) {
		if (prev_amb_invalid[no] != 0) {
			prev_amb_invalid[no] = 0;
			GXSetChanAmbColor(no + 4, ch->amb_color);
			prev_ch[no].amb_color = ch->amb_color;
		} else if (UpdateChannelColor(chan, &prev_ch[no].amb_color, &ch->amb_color)) {
			GXSetChanAmbColor(chan, ch->amb_color);
		}
	}

	if (ch->mat_src == GX_SRC_REG) {
		if (prev_mat_invalid[no] != 0) {
			prev_mat_invalid[no] = 0;
			GXSetChanMatColor(no + 4, ch->mat_color);
			prev_ch[no].mat_color = ch->mat_color;
		} else if (UpdateChannelColor(chan, &prev_ch[no].mat_color, &ch->mat_color)) {
			GXSetChanMatColor(chan, ch->mat_color);
		}
	}

	if ((ch->enable != prev_ch[idx].enable) || (ch->amb_src != prev_ch[idx].amb_src) || (ch->mat_src != prev_ch[idx].mat_src)
	    || (ch->light_mask != prev_ch[idx].light_mask) || (ch->diff_fn != prev_ch[idx].diff_fn) || (ch->attn_fn != prev_ch[idx].attn_fn)) {
		GXSetChanCtrl(chan, ch->enable, ch->amb_src, ch->mat_src, ch->light_mask, ch->diff_fn, ch->attn_fn);
		prev_ch[idx].enable     = ch->enable;
		prev_ch[idx].amb_src    = ch->amb_src;
		prev_ch[idx].mat_src    = ch->mat_src;
		prev_ch[idx].light_mask = ch->light_mask;
		prev_ch[idx].diff_fn    = ch->diff_fn;
		prev_ch[idx].attn_fn    = ch->attn_fn;
		if (chan == GX_COLOR0A0 || chan == GX_COLOR1A1) {
			prev_ch[idx + 2].enable     = ch->enable;
			prev_ch[idx + 2].amb_src    = ch->amb_src;
			prev_ch[idx + 2].mat_src    = ch->mat_src;
			prev_ch[idx + 2].light_mask = ch->light_mask;
			prev_ch[idx + 2].diff_fn    = ch->diff_fn;
			prev_ch[idx + 2].attn_fn    = ch->attn_fn;
		}
	}
}

void HSD_DisableChannelLighting(s32 chan)
{
	s32 idx = chan & 3;
	switch (chan) {
	case GX_COLOR0A0:
	case GX_COLOR1A1:
		if (prev_ch[idx].enable || prev_ch[idx + 2].enable) {
			s32 amb_src, mat_src, diff_fn, attn_fn;

			prev_ch[idx].enable = prev_ch[idx + 2].enable = 0;
			prev_ch[idx].light_mask = prev_ch[idx + 2].light_mask = 0;

			amb_src                  = prev_ch[idx].amb_src;
			mat_src                  = prev_ch[idx].mat_src;
			diff_fn                  = prev_ch[idx].diff_fn;
			attn_fn                  = prev_ch[idx].attn_fn;
			prev_ch[idx + 2].amb_src = amb_src;
			prev_ch[idx + 2].mat_src = mat_src;
			prev_ch[idx + 2].diff_fn = diff_fn;
			prev_ch[idx + 2].attn_fn = attn_fn;
			GXSetChanCtrl(chan, 0, amb_src, mat_src, 0, diff_fn, attn_fn);
		}
		break;
	default:
		if (prev_ch[idx].enable) {
			prev_ch[idx].enable     = 0;
			prev_ch[idx].light_mask = 0;
			GXSetChanCtrl(chan, 0, prev_ch[idx].amb_src, prev_ch[idx].mat_src, 0, prev_ch[idx].diff_fn, prev_ch[idx].attn_fn);
		}
		break;
	}
}

void HSD_StateSetNumChans(s32 num)
{
	if (prev_num_chans != num) {
		GXSetNumChans(num);
		prev_num_chans = num;
	}
}

void HSD_SetupChannelAll(HSD_Chan* channel)
{
	s32 num = 0;
	while (channel != NULL) {
		s32 temp = HSD_Channel2Num(channel->chan);
		if (temp > num) {
			num = temp;
		}
		HSD_SetupChannel(channel);
		channel = channel->next;
	}
	HSD_StateSetNumChans((u8)num);
}

void HSD_StateRegisterTexGen(s32 coord)
{
	s32 temp_r3 = HSD_TexCoordID2Num(coord);
	if (temp_r3 > num_tex_gens) {
		num_tex_gens = temp_r3;
	}
}

void HSD_StateSetNumTexGens(void)
{
	GXSetNumTexGens(num_tex_gens);
	num_tex_gens = 0;
}

void HSD_StateInitTev(void)
{
	current_tev = 0;
}

s32 HSD_StateGetNumTevStages(void)
{
	return current_tev;
}

u32 HSD_StateAssignTev(void)
{
	return HSD_Index2TevStage(current_tev++);
}

void HSD_StateSetNumTevStages(void)
{
	GXSetNumTevStages(current_tev);
	current_tev = 0;
}

void HSD_SetupTevStage(HSD_TevDesc* desc)
{
	GXSetTevOrder(desc->stage, desc->coord, desc->map, desc->color);
	if (desc->flags == 0) {
		GXSetTevOp(desc->stage, desc->u.tevconf.clr_op);
		GXSetTevSwapMode(desc->stage, GX_TEV_SWAP0, GX_TEV_SWAP0);
		return;
	}
	GXSetTevColorOp(desc->stage, desc->u.tevconf.clr_op, desc->u.tevconf.clr_bias, desc->u.tevconf.clr_scale, desc->u.tevconf.clr_clamp,
	                desc->u.tevconf.clr_out_reg);
	GXSetTevColorIn(desc->stage, desc->u.tevconf.clr_a, desc->u.tevconf.clr_b, desc->u.tevconf.clr_c, desc->u.tevconf.clr_d);
	GXSetTevAlphaOp(desc->stage, desc->u.tevconf.alpha_op, desc->u.tevconf.alpha_bias, desc->u.tevconf.alpha_scale,
	                desc->u.tevconf.alpha_clamp, desc->u.tevconf.alpha_out_reg);
	GXSetTevAlphaIn(desc->stage, desc->u.tevconf.alpha_a, desc->u.tevconf.alpha_b, desc->u.tevconf.alpha_c, desc->u.tevconf.alpha_d);
	GXSetTevSwapMode(desc->stage, desc->u.tevconf.ras_swap, desc->u.tevconf.tex_swap);
	GXSetTevKColorSel(desc->stage, desc->u.tevconf.kcsel);
	GXSetTevKAlphaSel(desc->stage, desc->u.tevconf.kasel);
}

void HSD_SetupTevStageAll(HSD_TevDesc* desc)
{
	s32 num = 0;

	while (desc != NULL) {
		s32 temp = HSD_TevStage2Num(desc->stage);
		if (temp > num) {
			num = temp;
		}

		HSD_SetupTevStage(desc);
		desc = desc->next;
	}
	current_tev = num;
	HSD_StateSetNumTevStages();
}

s32 HSD_Channel2Num(s32 chan)
{
	switch (chan) {
	case 0:
		return 1;
	case 1:
		return 2;
	case 2:
		return 1;
	case 3:
		return 2;
	case 4:
		return 1;
	case 5:
		return 2;
	case 0xFF:
		return 0;
	default:
		HSD_ASSERT(0x2D2, 0);
		return 0;
	}
}

s32 HSD_Index2TevStage(s32 idx)
{
	switch (idx) {
	case 0:
		return 0;
	case 1:
		return 1;
	case 2:
		return 2;
	case 3:
		return 3;
	case 4:
		return 4;
	case 5:
		return 5;
	case 6:
		return 6;
	case 7:
		return 7;
	case 8:
		return 8;
	case 9:
		return 9;
	case 10:
		return 10;
	case 11:
		return 11;
	case 12:
		return 12;
	case 13:
		return 13;
	case 14:
		return 14;
	case 15:
		return 15;
	default:
		HSD_ASSERT(0x307, 0);
		return 15;
	}
}

s32 HSD_TevStage2Num(s32 stage)
{
	switch (stage) {
	case 0:
		return 1;
	case 1:
		return 2;
	case 2:
		return 3;
	case 3:
		return 4;
	case 4:
		return 5;
	case 5:
		return 6;
	case 6:
		return 7;
	case 7:
		return 8;
	case 8:
		return 9;
	case 9:
		return 10;
	case 10:
		return 11;
	case 11:
		return 12;
	case 12:
		return 13;
	case 13:
		return 14;
	case 14:
		return 15;
	case 15:
		return 16;
	default:
		HSD_ASSERT(0x35B, 0);
		return 0;
	}
}

// NONMATCHING: without the removed single-use inline mapping helper, MWCC schedules the outgoing color copy after the switch instead of before it.
void HSD_SetTevRegAll(void)
{
	static const GXTevRegID registerIds[] = { GX_TEVREG0, GX_TEVREG1, GX_TEVREG2, GX_TEVPREV };
	u32 i;

	for (i = 0; i < ARRAY_SIZE(TevReg); i++) {
		if (TevReg[i].pending) {
			GXSetTevColorS10(registerIds[i], TevReg[i].color);
			TevReg[i].pending = FALSE;
		}
	}
}

s32 HSD_TexCoordID2Num(s32 id)
{
	switch (id) {
	case 0:
		return 1;
	case 1:
		return 2;
	case 2:
		return 3;
	case 3:
		return 4;
	case 4:
		return 5;
	case 5:
		return 6;
	case 6:
		return 7;
	case 7:
		return 8;
	case 0xFF:
		return 0;
	default:
		HSD_ASSERT(0x434, 0);
		return 0;
	}
}

void _HSD_StateInvalidateColorChannel(void)
{
	static HSD_Chan invalid_prev_ch[4] = {
		{ NULL, 0, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, 255, 0, 0, 0, 0, 2, NULL },
		{ NULL, 1, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, 255, 0, 0, 0, 0, 2, NULL },
		{ NULL, 2, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, 255, 0, 0, 0, 0, 2, NULL },
		{ NULL, 3, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, 255, 0, 0, 0, 0, 2, NULL },
	};

	memcpy(&prev_ch, &invalid_prev_ch, sizeof(prev_ch));
	prev_amb_invalid[0] = 1;
	prev_amb_invalid[1] = 1;
	prev_mat_invalid[0] = 1;
	prev_mat_invalid[1] = 1;
	prev_num_chans      = -1;
}

void _HSD_StateInvalidateTevStage(void)
{
	current_tev = 0;
}

void _HSD_StateInvalidateTevRegister(void)
{
	s32 i;
	for (i = 0; i < 4; i++) {
		TevReg[i].pending = FALSE;
	}
}

void _HSD_StateInvalidateTexCoordGen(void)
{
	num_tex_gens = 0;
}
