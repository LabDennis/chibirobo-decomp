#include "game/menuselectionbar.h"
#include "game/drawhelptext.h"
#include "game/hsd_jobj.h"
#include "game/hsd_cobj.h"
#include "game/stg.h"
#include "game/cgame.h"
#include "game/cgs.h"
#include "game/fbtexeff.h"
#include "game/cfont.h"
#include "game/xchr.h"
#include "game/video.h"
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "game/menupanelbackground.h"

extern "C" {
void* GetSpawnData(s32 index);
const char* GetLocalizedPCDialogue(s32 id);
char* GetLocalizedInterfaceText(s32 id);
char* GetStatusOverviewLabel(s32 id);
char* GetStatusIconName(s32 id);
char* GetItemName(s16 id);
s32 GetStatusIconEventFlagId(s32 id);
void DrawMenuSelectionCursor(MenuCursor* cursor, f32 x, f32 y, f32 w, f32 h, f32 alpha, f32 alpha2);
void UpdateObject__6CursorFUl(MenuCursor* cursor, s32 a);
void RebuildStageSelectList(MenuStageSelect* menu);
void DrawBeveledMenuRect(u32 color, f32 x, f32 y, f32 z, f32 w, f32 h, f32 alpha);
s32 ConvertHudTextToFullWidth(char* text);

void DrawStatusMenuTabs(MenuStatus* menu);
void DrawInventoryGrid(s32 id, f32 x, f32 y, f32 alpha, f32 scale, f32 a);
void DrawStatusItemDetails(s32 id, f32 x, f32 y, f32 alpha, f32 a);
u16 GetItemFlagsMasked(s16 id, s32 kind);
void DrawDiscardCountPanel(MenuStatus* menu);
void DrawPlayerStatusOverview(MenuStatus* menu);
void DrawStatusDiagram(MenuStatus* menu);
void DrawStatusIconGrid(MenuStatus* menu);
void DrawStatusHelpText(MenuStatus* menu);
void DrawStatusMenuTitle(MenuStatus* menu);
void GXSetScissor(u32 left, u32 top, u32 width, u32 height);
void LayoutMainMenuRows(MenuMain* menu);
void DrawMenuWindow(MenuWindow* win);
void PSMTXTrans(Mtx m, f32 x, f32 y, f32 z);
void PSMTXScale(Mtx m, f32 x, f32 y, f32 z);
u16 GXGetTexObjWidth(const GXTexObj* obj);
u16 GXGetTexObjHeight(const GXTexObj* obj);
CTexObj* GetFontGlyphTexture(CFont* font, const char* str, s32* width);
}

// japan has no language setting, its text is always japanese
static inline bool IsJapanese()
{
#if defined(VERSION_JP)
	return true;
#else
	return gGame.mLanguage == 6;
#endif
}

// the fixed lines of the pause menu, MemCard's ? becomes the slot letter
#if defined(VERSION_JP)
enum { MainMemCardAction = 10, MainSelectStageAction = 7, MainPCAction = 8, MainSaveAction = 12 };

static MenuMainItem sMainLines[] = {
	{ "\x83\x58\x83\x65\x81\x5B\x83\x5E\x83\x58", 1 },
	{ "\x01", -1 },
	{ "\x83\x81\x83\x82\x83\x4A (?)", MainMemCardAction },
	{ "\x83\x7D\x83\x62\x83\x76\x91\x49\x91\xF0", MainSelectStageAction },
	{ NULL, 0 },
};
#else
enum { MainMemCardAction = 6, MainSelectStageAction = 5, MainPCAction = 3, MainSaveAction = 4 };

static MenuMainItem sMainLines[] = {
	{ "StatusMenu", 1 }, { "\x01", -1 }, { "MemCard(?)", MainMemCardAction }, { "SelectStage", MainSelectStageAction }, { NULL, 0 },
};
#endif

// the stages the stage select offers, in its order
static s32 sStageOrder[] = { 0, 5, 7, 1, 2, 4, 6, 0x12, 3, 9, 0x10, 0xB, 0x16, 0xD, 0xE, 0xA, -1 };

// the card test popup's rows
#if defined(VERSION_JP)
static MenuMCTestAction sMCTestSubLines[]
    = { { "\x83\x8D\x81\x5B\x83\x68", 4 }, { "\x83\x5A\x81\x5B\x83\x75", 2 }, { "\x01", -1 }, { "\x8D\xED\x8F\x9C", 6 } };
#else
static MenuMCTestAction sMCTestSubLines[] = { { "load", 4 }, { "save", 2 }, { "\x01", -1 }, { "delete", 6 } };
#endif

// the file's statics in .bss order, what the first ones hold isn't known yet
u8 lbl_80361F68[0xD4];
Vec lbl_8036203C;
#if defined(VERSION_JP)
// The MenuMain destructor record follows the seventh vector in JP.
Vec lbl_80362048[7];
#else
Vec lbl_80362048[8];
#endif
// MenuPC scrolls two text chunks in US and three in PAL.
#if defined(VERSION_PAL)
char gPCDialogueChunks[3][256];
#elif !defined(VERSION_JP)
char gPCDialogueChunks[2][256];
#endif

static MenuMain sMenuMain;
static MenuStatus sMenuStatus;
static MenuItem sMenuItem;
#if defined(VERSION_JP)
static MenuRequest sMenuRequest;
static MenuSettings sMenuSettings;
static MenuPadControl sMenuPadControl;
static MenuSound sMenuSound;
static MenuStageSelect sMenuStageSelect;
static MenuPC sMenuPC;
static MenuRequestClear sMenuRequestClear;
static MenuMCTest sMenuMCTest;
static MenuMCTestSub sMenuMCTestSub;
static MenuSave sMenuSave;
#else
static MenuPC sMenuPC;
static MenuSave sMenuSave;
static MenuStageSelect sMenuStageSelect;
static MenuMCTest sMenuMCTest;
static MenuMCTestSub sMenuMCTestSub;
#endif

// the items the status and item menus page through
static MenuStatusTable sStatusItemTable;
// the chibi-pc pages and the lists they show
MenuPCPage gShopHistoryPage;
MenuPCPage gShopCatalogPage;
u8 gStatusIconUnlocked[24];
u8 gStatusEntryAvailable[16];
static MenuPCItem sShopHistoryEntries[20];
PCRankingRow gPCRankingRows[10];
static MenuPCItem sShopCatalogEntries[20];

extern "C" CTexObj* CopyMenuTextureObject(CTexObj* dst, const CTexObj* src);
extern "C" s32 CountShopCatalogEntries(const MenuPCPage*);
extern "C" s32 CountShopHistoryEntries(const MenuPCPage*);
extern "C" s32 CountDecimalDigits(s32 value);
extern "C" BOOL Draw__5CMenuFv(CMenu* menu);
extern "C" s32 GetPlayerLevel(Params* params);
extern "C" Params* GetPlayerParamsByIndex(CGame* game, s32 i);
extern "C" void DrawMenuLine(CGs* gs, u32 color, f32 x0, f32 y0, f32 z0, f32 x1, f32 y1, f32 z1);
extern "C" void DrawPCExchangeRateIcon(f32 x, f32 y, f32 alpha);
extern "C" CTexObj* GetSubwindowTexture(CGame* game);
extern "C" s32 GetShopCatalogFirstRow(const MenuPCPage* page);
extern "C" s32 GetShopHistoryFirstRow(const MenuPCPage* page);
extern "C" void SetMenuTextureTranslation(CTexObj* tex, f32 x, f32 y);
extern "C" void SetMenuTextureScale(CTexObj* tex, f32 x, f32 y);
extern "C" void DrawSolidQuad(CGs* gs, Vec* quad, u32 color);
extern "C" void DrawPCShopScrollBar(f32 x, f32 y, f32 start, f32 end, f32 alpha, f32 /*scale*/);
extern "C" void SetMenuLineWidth(CGs* gs, u8 width, s32 texOffs);
extern "C" void SetMenuBlendMode(CGs* gs, s32 type, s32 src, s32 dst, s32 op);
extern "C" u8 GetGameLanguage(CBase* base);
extern "C" void DrawMenuTexturedQuad(CGs* gs, CTexObj* tex, u32 color, f32 x, f32 y, f32 z, f32 w, f32 h, f32 a, f32 b, f32 c);
extern "C" u16 GetMenuTextureHeight(CTexObj* tex);
extern "C" u16 GetMenuTextureWidth(CTexObj* tex);
extern "C" void SetMenuTevMode(CGs* gs, s32 mode);
extern "C" CTexObj* LoadMenuPaletteTexture(CGame* game, s32 i);
extern "C" void DrawMenuTriangleFan(CGs* gs, Vec* pos, u32 color, u16 count);
extern "C" void SetMenuDepthMode(CGs* gs, u8 enable, s32 func, u8 update);
extern "C" CGs* GetGameGraphics(CGame* game);
extern "C" const char* GetPCDialogueText(s32 id);
extern "C" char* GetPCMenuLabelText(s32 id);
extern "C" void DrawPCBlueNumber(bool icon, f32 x, f32 y, f32 alpha, f32 scale, f32 value);
extern "C" void DrawPCGoldNumber(bool icon, f32 x, f32 y, f32 alpha, f32 scale, f32 value);
extern "C" u32 GetPCDialogueNumber(MenuPC* menu);
extern "C" void DrawPCRecyclePanel(MenuPC* menu, s32 choice, f32 x, f32 y, f32 scrap, f32 money, f32 scroll, f32 alpha, f32 scale);
extern "C" void DrawPCScrapPurchasePanel(s32 selected, s32 choice, f32 x, f32 y, f32 scroll, f32 alpha, f32 scale);
extern "C" void DrawPCItemPurchasePanel(s32 selected, s32 choice, f32 x, f32 y, f32 scroll, f32 alpha, f32 scale);
extern "C" void DrawPCRankings(MenuPC* menu, f32 x, f32 y, f32 alpha, f32 scale);
extern "C" void ApproachHudValue(f32*, f32*, f32);
extern "C" f32 gPCDialogueX;
static inline void PCEaseCall(f32& value, f32& target)
{
	f32 speed = target - value;
	f32 rate  = 0.25f;
	rate      = speed * rate;
	if (rate < 0.0f)
		rate = -rate;
	if (rate <= 1.0f)
		rate = 1.0f;
	ApproachHudValue(&value, &target, rate);
}
enum PCShopListKind { PriceList, ItemPriceList };
template <PCShopListKind kind>
#if defined(VERSION_JP)
void DrawPCShopList(s32, f32, f32, f32, f32, f32);
#else
void DrawPCShopList(s32, f32, f32, f32, f32, f32, f32);
#endif

extern "C" {
f32 gItemHelpOutlineX[4]  = { -1.0f, 1.0f, 1.0f, -1.0f };
f32 gItemHelpOutlineY[4]  = { -1.0f, -1.0f, 1.0f, 1.0f };
f32 gItemTitleOutlineX[4] = { -1.0f, 1.0f, 1.0f, -1.0f };
f32 gItemTitleOutlineY[4] = { -1.0f, -1.0f, 1.0f, 1.0f };

f32 gStatusOutlineX[4]      = { -1.0f, 1.0f, 1.0f, -1.0f };
f32 gStatusOutlineY[4]      = { -1.0f, -1.0f, 1.0f, 1.0f };
f32 gStatusTitleOutlineX[4] = { -1.0f, 1.0f, 1.0f, -1.0f };
f32 gStatusTitleOutlineY[4] = { -1.0f, -1.0f, 1.0f, 1.0f };
f32 gStatusHelpOutlineX[4]  = { -1.0f, 1.0f, 1.0f, -1.0f };
f32 gStatusHelpOutlineY[4]  = { -1.0f, -1.0f, 1.0f, 1.0f };

f32 gSaveTitleOutlineX[4] = { -1.0f, 1.0f, 1.0f, -1.0f };
f32 gSaveTitleOutlineY[4] = { -1.0f, -1.0f, 1.0f, 1.0f };
f32 gSaveHelpOutlineX[4]  = { -1.0f, 1.0f, 1.0f, -1.0f };
f32 gSaveHelpOutlineY[4]  = { -1.0f, -1.0f, 1.0f, 1.0f };
#if defined(VERSION_JP)
f32 gGradientTextOutlineX[4] = { -1.0f, 1.0f, 1.0f, -1.0f };
f32 gGradientTextOutlineY[4] = { -1.0f, -1.0f, 1.0f, 1.0f };
#endif

s32 gPCLevelDialogueIds[11] = { 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14 };
}
struct PCScreenAnimation {
	f32 textureU, textureV;
	f32 topPhase, bottomPhase;
};

struct PCChoiceArt {
	s32 icon, label;
};

static const PCChoiceArt pcChoiceArt[] = { { 0, 0 },
	                                       { 0, 0 },
	                                       { 0, 0 },
	                                       { 146 - StatusTextureOffset, 2 },
	                                       { 145 - StatusTextureOffset, 4 },
	                                       { 145 - StatusTextureOffset, 0 },
	                                       { 0, 0 },
	                                       { 141 - StatusTextureOffset, 24 },
	                                       { 0, 0 },
	                                       { 145 - StatusTextureOffset, 6 },
	                                       { 145 - StatusTextureOffset, 28 },
	                                       { 0, 0 } };

static inline bool PCUsesFixedChoiceLabels()
{
#if defined(VERSION_JP)
	return true;
#else
	return GetGameLanguage(&gGame) == 6;
#endif
}

enum PCIconAnchor { PCIconCenter, PCIconCorner, PCIconCursor };

static inline f32 PCDigitDivisor(s32 i)
{
	f32 divisor = 1.0f;
	if (i > 0) {
		for (s32 place = 0; place < i; ++place)
			divisor *= 10.0f;
	}
	return divisor;
}

enum PCDepositField { DepositBalance, DepositEntry, DepositReceipt };

#if defined(VERSION_JP)
static inline void SetPCDialogueGlyph(MenuPC* menu, u16 code)
{
	*(u16*)menu->mCharCopies[menu->mNextGlyph] = code;
	menu->mCharCopies[menu->mNextGlyph][2]     = 0;
}

static inline u16 ReadPCDialogueGlyph(const char* text)
{
	const u8* bytes = reinterpret_cast<const u8*>(text);
	return (bytes[0] << 8) | bytes[1];
}
#endif

// NONMATCHING: same code size but retail schedules the large mode switch differently,
// load/store order and regalloc differ throughout
BOOL MenuPC::Draw()
{
	if (Draw__5CMenuFv(this)) {
		CTexObj tex;
		char labelText[256];
		{
			f32 coord0     = 130;
			f32 coord1     = 40;
			f32 coord2     = 510;
			f32 coord3     = 360;
			Vec vertices[] = { (Vec) { coord0, coord1, -10 }, (Vec) { coord2, coord1, -10 }, (Vec) { coord2, coord3, -10 },
				               (Vec) { coord0, coord3, -10 }, (Vec) { coord0, coord1, -10 } };
			SetMenuDepthMode(GetGameGraphics(&gGame), 1, 7, 1);
			DrawMenuTriangleFan(GetGameGraphics(&gGame), vertices, (u32)(255.0f * this->mAlpha), 5);
		}
		switch (mMode) {
		case 1: {

			f32 coord0  = 260;
			f32 coord1  = 160;
			f32 coord2  = 380;
			f32 coord3  = 240;
			Vec panel[] = { (Vec) { coord0, coord1, -10 }, (Vec) { coord2, coord1, -10 }, (Vec) { coord2, coord3, -10 },
				            (Vec) { coord0, coord3, -10 }, (Vec) { coord0, coord1, -10 } };
			SetMenuDepthMode(GetGameGraphics(&gGame), 1, 7, 1);
			DrawMenuTriangleFan(GetGameGraphics(&gGame), panel, (u32)(255.0f * this->mAlpha) | 0xFFB62800, 5);
			DrawGsLineStrip(GetGameGraphics(&gGame), panel, (u32)(255.0f * this->mAlpha) | 0xC8C8C800, 5);
			{
				CopyMenuTextureObject(&tex, LoadMenuPaletteTexture(&gGame, 142 - StatusTextureOffset));
				SetMenuTevMode(GetGameGraphics(&gGame), 0);
				const f32 scale = 1.4f;
				f32 width       = scale * (1.1f * GetMenuTextureWidth(&tex));
				f32 height      = scale * GetMenuTextureHeight(&tex);
				u32 color       = 0xC8C8C800 | (u32)(255.0f * this->mAlpha);
				DrawMenuTexturedQuad(GetGameGraphics(&gGame), &tex, color, 320, 193, 0, width, height, 0, 0, 0);
				SetMenuTevMode(GetGameGraphics(&gGame), 4);
			}
#if !defined(VERSION_JP)
			if (GetGameLanguage(&gGame) == 6)
#endif
			{
				CopyMenuTextureObject(&tex, LoadMenuPaletteTexture(&gGame, 143 - StatusTextureOffset));
				SetMenuTevMode(GetGameGraphics(&gGame), 0);
				f32 width  = 1.1f * GetMenuTextureWidth(&tex);
				f32 height = GetMenuTextureHeight(&tex);
				u32 color  = 0xC8C8C800 | (u32)(255.0f * this->mAlpha);
				DrawMenuTexturedQuad(GetGameGraphics(&gGame), &tex, color, 320, 227, 0, width, height, 0, 0, 0);
				SetMenuTevMode(GetGameGraphics(&gGame), 4);
			}

			SetGsChannelEnabled(GetGameGraphics(&gGame), 0, 0);
			SetMenuBlendMode(GetGameGraphics(&gGame), 1, 4, 5, 15);
			SetMenuDepthMode(GetGameGraphics(&gGame), 0, 3, 0);
			f32 fill    = 281.0f + 80.0f * this->mProgress;
			Vec bar[12] = {};
			bar[0].x    = 281;
			bar[0].y    = 249;
			bar[1].x    = 361;
			bar[1].y    = 249;
			bar[2].x    = 361;
			bar[2].y    = 253;
			bar[3].x    = 281;
			bar[3].y    = 253;
			bar[4].x    = 281;
			bar[4].y    = 249;
			bar[5].x    = fill;
			bar[5].y    = 249;
			bar[6].x    = fill;
			bar[6].y    = 253;
			bar[7].x    = 281;
			bar[7].y    = 253;
			bar[8].x    = fill;
			bar[8].y    = 249;
			bar[9].x    = 361;
			bar[9].y    = 249;
			bar[10].x   = 361;
			bar[10].y   = 253;
			bar[11].x   = fill;
			bar[11].y   = 253;
			DrawGsLineStrip(GetGameGraphics(&gGame), bar, (u32)(255.0f * this->mAlpha) | 0xC8C8C800, 5);
			SetMenuLineWidth(GetGameGraphics(&gGame), 6, 5);
			DrawSolidQuad(GetGameGraphics(&gGame), bar + 4, (u32)(255.0f * this->mAlpha) | 0xFFB62800);
			DrawSolidQuad(GetGameGraphics(&gGame), bar + 8, (u32)(255.0f * this->mAlpha) | 0xC8000000);
			SetMenuLineWidth(GetGameGraphics(&gGame), 12, 5);
		} break;
		case 2: {
			f32 origin, step;
			switch (this->mChoiceCount) {
			case 1:
				origin = 170.0f;
				step   = 36.0f;
				break;
			case 2:
				origin = 170.0f;
				step   = 36.0f;
				break;
			case 3:
				origin = 180.0f;
				step   = 40.0f;
				break;
			case 4:
				origin = 170.0f;
				step   = 36.0f;
				break;
			default:
				origin = 0;
				step   = 0;
				break;
			}
			const f32 top = origin - 20.0f;

			f32 center, width;
			if (PCUsesFixedChoiceLabels()) {

				sprintf(labelText, "%.255s", GetPCMenuLabelText(pcChoiceArt[this->mChoices[this->mChoice]].label));
				u32 chars = strlen(labelText) / 2;
				center    = 224.0f + 0.5f * ((chars - 1) * 20.0f);
				width     = 0.9f * ((chars + 2) * 20.0f);
			} else {
				const char* text = GetPCMenuLabelText(pcChoiceArt[this->mChoices[this->mChoice]].label);
				f32 textWidth    = MeasureGameTextWidth(GetGameGraphics(&gGame), text, TRUE, 20.0f);
				width            = 6.0f
				      + (1.1f * GetMenuTextureWidth(LoadMenuPaletteTexture(&gGame, pcChoiceArt[this->mChoices[this->mChoice]].icon))
				         + textWidth);
				center = 204.0f + width * 0.5f;
			}
			f32 y = 2.0f + (step * this->mChoice + (8.0f + top));
			{
				u32 color = 0x181CC600 | (u32)(200.0f * this->mAlpha);
				SetMenuTevMode(GetGameGraphics(&gGame), 0);
				CopyMenuTextureObject(&tex, LoadMenuPaletteTexture(&gGame, 117 - StatusTextureOffset));
				SetMenuTextureScale(&tex, 0.25f, 1.0f);
				SetMenuTextureTranslation(&tex, 0.25f, 0);
				DrawMenuTexturedQuad(GetGameGraphics(&gGame), &tex, color, center, y, 0, width, 24, 0, 0, 0);
				SetMenuTextureTranslation(&tex, 0, 0);
				DrawMenuTexturedQuad(GetGameGraphics(&gGame), &tex, color, (center - width * 0.5f) - 4.0f, y, 0, 8, 24, 0, 0, 0);
				SetMenuTextureTranslation(&tex, 0.5f, 0);
				DrawMenuTexturedQuad(GetGameGraphics(&gGame), &tex, color, 4.0f + (center + width * 0.5f), y, 0, 8, 24, 0, 0, 0);
				CTexObj* fill = LoadMenuPaletteTexture(&gGame, 118 - StatusTextureOffset);
				SetMenuTextureTranslation(fill, this->mTexScroll, 0);
				SetMenuTextureScale(fill, 0.5f * (width / GetMenuTextureWidth(fill)), 0.5f);
				color = 0xFFFFFF00 | (u32)(40.0f * this->mAlpha);
				DrawMenuTexturedQuad(GetGameGraphics(&gGame), fill, color, 2.0f + center, y, 0, width, 20, 0, 0, 0);
				SetMenuTevMode(GetGameGraphics(&gGame), 4);
			}
			for (s32 i = 0; i < this->mChoiceCount; ++i) {
				s32 choice = this->mChoices[i];
				if (choice == -1)
					continue;
				const PCChoiceArt& row = pcChoiceArt[choice];
				f32 alpha              = this->mAlpha;
				if (choice == 7 && !this->mCanSelect)
					alpha *= 0.4f;
				f32 rowY = step * i + top;
				CopyMenuTextureObject(&tex, LoadMenuPaletteTexture(&gGame, row.icon));

				SetMenuTevMode(GetGameGraphics(&gGame), 0);
				f32 w     = 1.1f * GetMenuTextureWidth(&tex);
				f32 h     = GetMenuTextureHeight(&tex);
				u32 color = 0xFFFFFF00 | (u32)(255.0f * alpha);
				DrawMenuTexturedQuad(GetGameGraphics(&gGame), &tex, color, 204.0f + 0.5f * w, 1.0f + (rowY + 0.5f * h), 0, w, h, 0, 0, 0);
				SetMenuTevMode(GetGameGraphics(&gGame), 4);
				if (PCUsesFixedChoiceLabels()) {
#if defined(VERSION_JP)
					sprintf(labelText, "%.255s", GetPCMenuLabelText(pcChoiceArt[this->mChoices[i]].label));
					DrawGsProportionalTextFormat(GetGameGraphics(&gGame), color, labelText, 224, rowY, 0, 20, 20);
#else
					DrawGsProportionalTextFormat(GetGameGraphics(&gGame), color, GetPCMenuLabelText(pcChoiceArt[this->mChoices[i]].label),
					                             224, rowY, 0, 20, 20);
#endif
				} else {
					DrawGsProportionalTextFormat(GetGameGraphics(&gGame), color, GetPCMenuLabelText(pcChoiceArt[this->mChoices[i]].label),
					                             6.0f + (204.0f + w), rowY, 0, 20, 20);
				}
			}
		} break;
		case 3: {
#if defined(VERSION_JP)
			const f32 scroll = 0.0f;
#else
			const f32 scroll = 8.0f;
#endif
			MenuPCPage* page = &gShopHistoryPage;
			s32 selected     = this->mPurchaseSelection;
			f32 alpha        = this->mListAlpha;
#if defined(VERSION_JP)
			DrawPCShopList<ItemPriceList>(selected, 320.0f, 215.0f + this->mScroll, this->mTexScroll, alpha, 1.0f);
#else
			DrawPCShopList<ItemPriceList>(selected, 320.0f, 215.0f + this->mScroll, this->mTexScroll, alpha, 1.0f, scroll);
#endif
			DrawPCGoldNumber(true, 339.0f, 316.0f, this->mAlpha, 1.0f, this->mMoneyDisplay.shown);
			s32 positions = CountShopHistoryEntries(page) - 8;
			f32 first, last;
			if (positions <= 1) {
				first = 0.f;
				last  = 1.f;
			} else {
				s32 start = GetShopHistoryFirstRow(page);
				f32 step  = 1.0f / (f32)positions;
				f32 row   = (f32)start;
				first     = step * row;
				last      = step * (1.f + row);
			}
			DrawPCShopScrollBar(460.0f, 76.0f - scroll, first, last, this->mAlpha, 1.0f);
		} break;
		case 4:
			DrawPCRankings(this, -30, -20, mAlpha, 1);
			break;
		case 7: {
#if defined(VERSION_JP)
			const f32 scroll = 0.0f;
#else
			const f32 scroll = 8.0f;
#endif
			MenuPCPage* page = &gShopCatalogPage;
			s32 selected     = this->mShopSelection;
#if defined(VERSION_JP)
			DrawPCShopList<PriceList>(selected, 320.0f, 215.0f + this->mScroll, this->mTexScroll, 1.0f, 1.0f);
#else
			DrawPCShopList<PriceList>(selected, 320.0f, 215.0f + this->mScroll, this->mTexScroll, 1.0f, 1.0f, scroll);
#endif
			DrawPCBlueNumber(true, 339.0f, 316.0f, this->mAlpha, 1.0f, this->mScrapDisplay.shown);
			s32 positions = CountShopCatalogEntries(page) - 8;
			f32 first, last;
			if (positions <= 1) {
				first = 0.f;
				last  = 1.f;
			} else {
				s32 start = GetShopCatalogFirstRow(page);
				f32 step  = 1.0f / (f32)positions;
				f32 row   = (f32)start;
				first     = step * row;
				last      = step * (1.f + row);
			}
			DrawPCShopScrollBar(460.0f, 76.0f - scroll, first, last, this->mAlpha, 1.0f);
		} break;
		case 5: {
			CTexObj* subwindow = GetSubwindowTexture(&gGame);
			f32 w              = 0.6f * (1.1f * GetMenuTextureWidth(subwindow));
			f32 h              = 0.6f * GetMenuTextureHeight(subwindow);

			SetMenuTevMode(GetGameGraphics(&gGame), 0);
			DrawMenuTexturedQuad(GetGameGraphics(&gGame), subwindow, 0xC8C8C800 | (u32)(255.0f * this->mAlpha), 320, 200, 0, w, h, 0, 0, 0);
			SetMenuTevMode(GetGameGraphics(&gGame), 4);
		} break;

		case 9: {
			sprintf(labelText, "%.255s", GetPCMenuLabelText(7));
			DrawGsProportionalTextFormat(GetGameGraphics(&gGame), (u32)(this->mAlpha * 255.0f) | 0xC8C8C800, labelText, 224, 148, 0, 18,
			                             18);
			{
				const f32 scale = 1.2f;

				CopyMenuTextureObject(&tex, LoadMenuPaletteTexture(&gGame, 125 - StatusTextureOffset));
				f32 w = scale * GetMenuTextureWidth(&tex);
				f32 h = scale * (GetMenuTextureHeight(&tex) / 10);

				mDepositDisplay.target = this->mDepositBalance;
				PCEaseCall(mDepositDisplay.shown, mDepositDisplay.target);
				SetMenuTextureScale(&tex, 1, 0.1f);

				f32 step   = w - 2.4f;
				f32 start  = 276;
				f32 offset = 9.6f;
				f32 y      = 184;
				u32 rgb    = 0xFFB62800;

				for (int i = 0; i < 5; i++) {
					f32 divisor = PCDigitDivisor(i);
					s32 digit   = (s32)(mDepositDisplay.shown / divisor) % 10;
					SetMenuTextureTranslation(&tex, 0, 0.1f * digit);
					SetMenuTevMode(GetGameGraphics(&gGame), 0);
					f32 x = (7.0f * w + (start - i * step)) - offset;
					DrawMenuTexturedQuad(GetGameGraphics(&gGame), &tex, rgb | (u32)(255.0f * mAlpha), x, y, 0, w, h, 0, 0, 0);
					SetMenuTevMode(GetGameGraphics(&gGame), 4);
				}
				SetMenuTevMode(GetGameGraphics(&gGame), 4);
			}
			switch (this->mPhase) {
			case 0: {
				sprintf(labelText, "%.255s", GetPCMenuLabelText(8));
				DrawGsProportionalTextFormat(GetGameGraphics(&gGame), (u32)(this->mAlpha * 255.0f) | 0xC8C8C800, labelText, 224, 212, 0, 18,
				                             18);
				{
					const f32 scale = 1.4f;

					CopyMenuTextureObject(&tex, LoadMenuPaletteTexture(&gGame, 125 - StatusTextureOffset));
					f32 w = scale * GetMenuTextureWidth(&tex);
					f32 h = scale * (GetMenuTextureHeight(&tex) / 10);

					PCAmount entry = {};
					entry.shown = entry.target = this->mAmount;
					SetMenuTextureScale(&tex, 1, 0.1f);

					f32 step   = w + 1.4f;
					f32 start  = 256;
					f32 offset = 30.8f;
					f32 y      = 256;
					u32 rgb    = 0xC8C8C800;

					for (int i = 0; i < 5; i++) {
						f32 divisor = PCDigitDivisor(i);
						s32 digit   = (s32)(entry.shown / divisor) % 10;
						SetMenuTextureTranslation(&tex, 0, 0.1f * digit);
						SetMenuTevMode(GetGameGraphics(&gGame), 0);
						f32 x = (7.0f * w + (start - i * step)) - offset;
						DrawMenuTexturedQuad(GetGameGraphics(&gGame), &tex, rgb | (u32)(255.0f * mAlpha), x, y, 0, w, h, 0, 0, 0);
						SetMenuTevMode(GetGameGraphics(&gGame), 4);
					}
					SetMenuTevMode(GetGameGraphics(&gGame), 4);
				}
				CopyMenuTextureObject(&tex, LoadMenuPaletteTexture(&gGame, 125 - StatusTextureOffset));
				f32 w    = 1.4f * GetMenuTextureWidth(&tex);
				f32 h    = 1.4f * (GetMenuTextureHeight(&tex) / 10);
				f32 left = (388.0f - this->mMoneyDigit * (1.0f + w)) - 8.0f;
				{
					f32 coord0     = left;
					f32 coord1     = 242;
					f32 coord2     = left + w;
					f32 coord3     = 242.0f + h;
					Vec vertices[] = { (Vec) { coord0, coord1, -10 }, (Vec) { coord2, coord1, -10 }, (Vec) { coord2, coord3, -10 },
						               (Vec) { coord0, coord3, -10 }, (Vec) { coord0, coord1, -10 } };
					SetMenuDepthMode(GetGameGraphics(&gGame), 1, 7, 1);
					DrawMenuTriangleFan(GetGameGraphics(&gGame), vertices, 0x1F24F800 | (u32)(80.0f * this->mAlpha), 5);
				}
				break;
			}
			case 1: {
				sprintf(labelText, "%.255s", GetPCMenuLabelText(9));
				DrawGsProportionalTextFormat(GetGameGraphics(&gGame), (u32)(this->mAlpha * 255.0f) | 0xC8C8C800, labelText, 224, 212, 0, 18,
				                             18);
				sprintf(labelText, "%.255s", GetPCMenuLabelText(10));
				DrawGsProportionalTextFormat(GetGameGraphics(&gGame), (u32)(this->mAlpha * 255.0f) | 0xC8C8C800, labelText, 260, 260, 0, 18,
				                             18);
				f32 x = this->mYesNo ? 302.0f : 242.0f;
				{
					f32 coord0     = x;
					f32 coord1     = 251;
					f32 coord2     = x + 68.0f;
					f32 coord3     = 283;
					Vec vertices[] = { (Vec) { coord0, coord1, -10 }, (Vec) { coord2, coord1, -10 }, (Vec) { coord2, coord3, -10 },
						               (Vec) { coord0, coord3, -10 }, (Vec) { coord0, coord1, -10 } };
					DrawMenuTriangleFan(GetGameGraphics(&gGame), vertices, 0x1F24F800 | (u32)(80.0f * this->mAlpha), 5);
				}
				break;
			}
			case 2: {
				const f32 scale = 1.2f;

				CopyMenuTextureObject(&tex, LoadMenuPaletteTexture(&gGame, 125 - StatusTextureOffset));
				f32 w = scale * GetMenuTextureWidth(&tex);
				f32 h = scale * (GetMenuTextureHeight(&tex) / 10);

				PCAmount entry = {};
				entry.shown = entry.target = this->mAmount;
				SetMenuTextureScale(&tex, 1, 0.1f);

				f32 step   = w - 2.4f;
				f32 start  = 372;
				f32 offset = 136.8f;
				f32 y      = 240;
				u32 rgb    = 0xFFFFFF00;

				for (int i = 0; i < 5; i++) {
					f32 divisor = PCDigitDivisor(i);
					s32 digit   = (s32)(entry.shown / divisor) % 10;
					SetMenuTextureTranslation(&tex, 0, 0.1f * digit);
					SetMenuTevMode(GetGameGraphics(&gGame), 0);
					f32 x = (7.0f * w + (start - i * step)) - offset;
					DrawMenuTexturedQuad(GetGameGraphics(&gGame), &tex, rgb | (u32)(255.0f * mAlpha), x, y, 0, w, h, 0, 0, 0);
					SetMenuTevMode(GetGameGraphics(&gGame), 4);
				}
				SetMenuTevMode(GetGameGraphics(&gGame), 4);
			}
				sprintf(labelText, "%.255s", GetPCMenuLabelText(11));
				DrawGsProportionalTextFormat(GetGameGraphics(&gGame), (u32)(this->mAlpha * 255.0f) | 0xC8C8C800, labelText, 244, 260, 0, 18,
				                             18);
				break;
			}
			DrawPCGoldNumber(true, 339, 316, this->mAlpha, 1, this->mMoneyDisplay.shown);
		} break;
		case 10: {
			s32 rate = (s32)this->mScrapRate;
			switch (this->mPhase) {
			case 0: {
				{
#if defined(VERSION_JP)
					sprintf(labelText, "%.255s", GetPCMenuLabelText(29));
					u32 chars = strlen(labelText) / 2;
					f32 x     = (320.0f - ((chars - 1) * 20.0f) * 0.5f) - 10.0f;
					DrawGsProportionalTextFormat(GetGameGraphics(&gGame), (u32)(255.0f * this->mAlpha) | 0xFFFFFF00, labelText, x, 148, 0,
					                             20, 20);
#else
					const char* localized = GetLocalizedInterfaceText(29 + 45);
					u32 color             = (u32)(255.0f * this->mAlpha) | 0xFFFFFF00;
					f32 x = 320.0f - 0.5f * MeasureGameTextWidth(GetGameGraphics(&gGame), GetLocalizedInterfaceText(29 + 45), TRUE, 20);
					DrawGsProportionalTextFormat(GetGameGraphics(&gGame), color, localized, x, 148, 0, 20, 20);
#endif
				}
				{
#if defined(VERSION_JP)
					sprintf(labelText, "%.255s", GetPCMenuLabelText(30));
					u32 chars = strlen(labelText) / 2;
					f32 x     = (320.0f - ((chars - 1) * 20.0f) * 0.5f) - 10.0f;
					DrawGsProportionalTextFormat(GetGameGraphics(&gGame), (u32)(255.0f * this->mAlpha) | 0xFFFFFF00, labelText, x, 220, 0,
					                             20, 20);
#else
					const char* localized = GetLocalizedInterfaceText(30 + 45);
					u32 color             = (u32)(255.0f * this->mAlpha) | 0xFFFFFF00;
					f32 x = 320.0f - 0.5f * MeasureGameTextWidth(GetGameGraphics(&gGame), GetLocalizedInterfaceText(30 + 45), TRUE, 20);
					DrawGsProportionalTextFormat(GetGameGraphics(&gGame), color, localized, x, 220, 0, 20, 20);
#endif
				}
				{
					s32 amount = this->mAmount;
					CopyMenuTextureObject(&tex, LoadMenuPaletteTexture(&gGame, 125 - StatusTextureOffset));
					f32 width  = 1.4f * GetMenuTextureWidth(&tex);
					f32 height = 1.4f * (GetMenuTextureHeight(&tex) / 10);
					f32 step   = width + 1.4f;

					PCAmount entry = {};
					entry.shown = entry.target = amount;
					SetMenuTextureScale(&tex, 1, 0.1f);

					for (s32 i = 0; i < 5; ++i) {
						f32 divisor = PCDigitDivisor(i);
						int digit   = (s32)(entry.shown / divisor) % 10;
						SetMenuTextureTranslation(&tex, 0, digit * 0.1f);
						SetMenuTevMode(GetGameGraphics(&gGame), 0);
						f32 x = ((7.0f * width + (256.0f - i * step)) - 26.6f) - (2.0f * step) * 0.5f;
						DrawMenuTexturedQuad(GetGameGraphics(&gGame), &tex, (u32)(255.0f * this->mAlpha) | 0xC8C8C800, x, 256, 0, width,
						                     height, 0, 0, 0);
						SetMenuTevMode(GetGameGraphics(&gGame), 4);
					}
					SetMenuTevMode(GetGameGraphics(&gGame), 4);
					CopyMenuTextureObject(&tex, LoadMenuPaletteTexture(&gGame, 125 - StatusTextureOffset));
					f32 markerW = 1.4f * GetMenuTextureWidth(&tex);
					f32 markerH = 1.4f * (GetMenuTextureHeight(&tex) / 10);
					f32 left    = ((388.0f - this->mScrapDigit * step) - 4.0f) - (2.0f * step) * 0.5f;
					f32 right = left + markerW, bottom = 241.0f + markerH;
					f32 coord0   = left;
					f32 coord1   = 241;
					f32 coord2   = right;
					f32 coord3   = bottom;
					Vec marker[] = { (Vec) { coord0, coord1, -10 }, (Vec) { coord2, coord1, -10 }, (Vec) { coord2, coord3, -10 },
						             (Vec) { coord0, coord3, -10 }, (Vec) { coord0, coord1, -10 } };
					SetMenuDepthMode(GetGameGraphics(&gGame), 1, 7, 1);
					DrawMenuTriangleFan(GetGameGraphics(&gGame), marker, 0x1F24F800 | (u32)(80.0f * this->mAlpha), 5);
				}
#if defined(VERSION_JP)
				DrawPCBlueNumber(false, 164, 178, this->mAlpha, 1, 1);
				sprintf(labelText, "%.255s", GetPCMenuLabelText(34));
				DrawGsProportionalTextFormat(GetGameGraphics(&gGame), (u32)(255.0f * this->mAlpha) | 0xC8C8C800, labelText, 313, 169, 0, 18,
				                             18);
				s32 digits = CountDecimalDigits(rate);
				CopyMenuTextureObject(&tex, LoadMenuPaletteTexture(&gGame, 125 - StatusTextureOffset));
				DrawPCGoldNumber(false, 212.0f + digits * (1.4f * GetMenuTextureWidth(&tex)), 178, this->mAlpha, 1, rate);
#else
				DrawPCBlueNumber(false, 168, 177, this->mAlpha, 1, 1);
				DrawPCExchangeRateIcon(323, 177, this->mAlpha);
				CopyMenuTextureObject(&tex, LoadMenuPaletteTexture(&gGame, 125 - StatusTextureOffset));
				DrawPCGoldNumber(false, 215, 177, this->mAlpha, 1, rate);
#endif
				break;
			}
			case 1:
			case 2:
				DrawPCRecyclePanel(this, this->mYesNo, 320, 215.0f + this->mScroll, this->mAmount, this->mAmount * rate, this->mTexScroll,
				                   this->mAlpha, 1);
				break;
			}
			DrawPCBlueNumber(true, 339, 316, this->mAlpha, 1, this->mScrapDisplay.shown);
		} break;
		case 11:
			break;
		}
		static bool topInitialized, bottomInitialized;
		static f32 textureScrollU, textureScrollV, topScanPhase, bottomScanPhase;
		{
			textureScrollU -= 0.008f;
			textureScrollV -= 0.008f;
			f32 height = this->mFrameHeight;
			{
				f32 coord0     = 130;
				f32 coord1     = 40.0f;
				f32 coord2     = 510;
				f32 coord3     = 40.0f + height;
				Vec vertices[] = { (Vec) { coord0, coord1, -10 }, (Vec) { coord2, coord1, -10 }, (Vec) { coord2, coord3, -10 },
					               (Vec) { coord0, coord3, -10 }, (Vec) { coord0, coord1, -10 } };
				SetMenuDepthMode(GetGameGraphics(&gGame), 1, 7, 1);
				DrawMenuTriangleFan(GetGameGraphics(&gGame), vertices, (u32)(255.0f * this->mAlpha) | 0xFFB62800, 5);
			}

			CopyMenuTextureObject(&tex, LoadMenuPaletteTexture(&gGame, 138 - StatusTextureOffset));
			SetMenuTextureTranslation(&tex, textureScrollU, textureScrollV);
			f32 v = 320.0f / GetMenuTextureHeight(&tex);
			SetMenuTextureScale(&tex, 380.0f / GetMenuTextureWidth(&tex), v);

			SetMenuDepthMode(GetGameGraphics(&gGame), 1, 4, 1);
			SetMenuBlendMode(GetGameGraphics(&gGame), 1, 2, 4, 15);
			SetMenuTevMode(GetGameGraphics(&gGame), 2);
			DrawMenuTexturedQuad(GetGameGraphics(&gGame), &tex, 0xFFFFFF00 | (u32)(16.0f * this->mAlpha), 510, 360, -11, 380, height, 0, 0,
			                     0);
			SetMenuTevMode(GetGameGraphics(&gGame), 4);
			SetMenuBlendMode(GetGameGraphics(&gGame), 1, 4, 5, 15);
			SetMenuDepthMode(GetGameGraphics(&gGame), 0, 3, 0);

			f32 half = height * 0.5f;
			u32 bandColor;
			{
				f32 coord0     = 130;
				f32 coord1     = 360.0f - half;
				f32 coord2     = 510;
				f32 coord3     = 360.0f;
				Vec vertices[] = { (Vec) { coord0, coord1, -10 }, (Vec) { coord2, coord1, -10 }, (Vec) { coord2, coord3, -10 },
					               (Vec) { coord0, coord3, -10 }, (Vec) { coord0, coord1, -10 } };
				SetMenuDepthMode(GetGameGraphics(&gGame), 1, 7, 1);
				bandColor = (u32)(255.0f * this->mAlpha) | 0xFFB62800;
				DrawMenuTriangleFan(GetGameGraphics(&gGame), vertices, bandColor, 5);
			}
			u32 gray = (bandColor & 0xFF) | 0x30303000;
			{
				f32 start = (40.0f + half) - half;
				SetMenuDepthMode(GetGameGraphics(&gGame), 1, 7, 0);
				if (!topInitialized) {
					topScanPhase   = 0.0f;
					topInitialized = true;
				}
				topScanPhase += 1.0f;
				if (topScanPhase >= 4.0f)
					topScanPhase = 0.0f;
				SetMenuBlendMode(GetGameGraphics(&gGame), 3, 4, 5, 15);
				SetMenuLineWidth(GetGameGraphics(&gGame), 12, 5);
				for (s32 row = (s32)start; row < (s32)(start + height); row += 4.0f) {
					f32 y = row + topScanPhase;
					DrawMenuLine(GetGameGraphics(&gGame), gray, 130, y, 0, 510, y, 0);
				}
				SetMenuBlendMode(GetGameGraphics(&gGame), 1, 4, 5, 15);
				SetMenuLineWidth(GetGameGraphics(&gGame), 12, 5);
			}
			{
				f32 start = (360.0f - half * 0.5f) - half * 0.5f;
				SetMenuDepthMode(GetGameGraphics(&gGame), 1, 7, 0);
				if (!bottomInitialized) {
					bottomScanPhase   = 0.0f;
					bottomInitialized = true;
				}
				bottomScanPhase += 1.0f;
				if (bottomScanPhase >= 4.0f)
					bottomScanPhase = 0.0f;
				SetMenuBlendMode(GetGameGraphics(&gGame), 3, 4, 5, 15);
				SetMenuLineWidth(GetGameGraphics(&gGame), 12, 5);
				for (s32 row = (s32)start; row < (s32)(start + half); row += 4.0f) {
					f32 y = row + bottomScanPhase;
					DrawMenuLine(GetGameGraphics(&gGame), gray, 130, y, 0, 510, y, 0);
				}
				SetMenuBlendMode(GetGameGraphics(&gGame), 1, 4, 5, 15);
				SetMenuLineWidth(GetGameGraphics(&gGame), 12, 5);
			}
		}
		{
			if (this->mMode != 0 && this->mMode != 1 && this->mMode != 11) {
				{

					switch (this->mMode) {

					case 2:
						sprintf(labelText, "%.255s", GetPCMenuLabelText(1));
						break;
					case 3:
						if (this->mShopOnly)
							sprintf(labelText, "%.255s", GetPCMenuLabelText(3));
						else
							sprintf(labelText, "%.255s", GetPCMenuLabelText(2));
						break;
					case 4:
						sprintf(labelText, "%.255s", GetPCMenuLabelText(4));
						break;
					case 5:
						sprintf(labelText, "%.255s", GetPCMenuLabelText(5));
						break;
					case 7:
						sprintf(labelText, "%.255s", GetPCMenuLabelText(24));
						break;
					case 9:
						sprintf(labelText, "%.255s", GetPCMenuLabelText(6));
						break;
					case 10:
						sprintf(labelText, "%.255s", GetPCMenuLabelText(28));
						break;
					default:
						sprintf(labelText, "%.255s", "");
						break;
					}
#if defined(VERSION_JP)
					char joined[256];
					sprintf(joined, "%.255s", GetPCMenuLabelText(0));
					strcat(joined, labelText);
					strcpy(labelText, joined);

					const f32 x = 210.0f, y = 120.0f;
#else

					const f32 x = 214.0f;
					f32 y       = 120.0f;
					switch (this->mMode) {
					case 3:
					case 7:
					case 10:
						y -= 8.0f;
						break;
					}
#endif
					DrawGsProportionalTextFormat(GetGameGraphics(&gGame), 0xFFB62800 | (u32)(255.0f * this->mAlpha), labelText, x, y, 0,
					                             18.0f, 18.0f);
				}
				if (!this->mShopOnly) {
					f32 rotation = 0;

					f32 top = 129;
#if !defined(VERSION_JP)
					switch (this->mMode) {
					case 3:
					case 7:
					case 10:
						top = 121;
						break;
					}
#endif
					f32 inset;
					switch (this->mMode) {
					case 3:
					case 7:
					case 10:
						inset = 70;
						break;
					case 4:
						inset = 50;
						break;
					default:
						inset = 30;
						break;
					}
					u32 color = 0xC8C8C800 | (u32)(255.0f * this->mAlpha);
					switch (this->mTextPhase) {
					case 1: {
						f32 coord0 = 210;
						f32 coord1 = top;

						Vec points[] = { (Vec) { coord0, coord1, -10 }, (Vec) { this->mTextX, coord1, -10 } };
						rotation     = 0;
						DrawGsLineStrip(GetGameGraphics(&gGame), points, color, 2);
						break;
					}
					case 2: {
						f32 coord0 = 210;
						f32 coord1 = top;
						f32 coord2 = 210.0f - inset;

						Vec points[]
						    = { (Vec) { coord0, coord1, -10 }, (Vec) { coord2, coord1, -10 }, (Vec) { coord2, this->mTextY, -10 } };
						rotation = 1.57079637;
						DrawGsLineStrip(GetGameGraphics(&gGame), points, color, 3);
						break;
					}
					case 3:
						rotation = -3.14159274;
						{
							f32 startX       = 210.0f;
							const f32 left   = startX - inset;
							const f32 bottom = top + 172.0f;
							Vec start        = { startX, top, -10 };
							Vec upperLeft    = { left, top, -10 };
							Vec lowerLeft    = { left, bottom, -10 };
							Vec tip          = { this->mTextX, bottom, -10 };
							Vec points[]     = { start, upperLeft, lowerLeft, tip };
							DrawGsLineStrip(GetGameGraphics(&gGame), points, color, 4);
						}
						break;
					case 4:
						rotation = -3.14159274;
						{
							f32 startX       = 210.0f;
							const f32 left   = startX - inset;
							const f32 bottom = top + 172.0f;
							Vec start        = { startX, top, -10 };
							Vec upperLeft    = { left, top, -10 };
							Vec lowerLeft    = { left, bottom, -10 };
							Vec tip          = { this->mTextX, bottom, -10 };
							Vec points[]     = { start, upperLeft, lowerLeft, tip };
							DrawGsLineStrip(GetGameGraphics(&gGame), points, color, 4);
						}
						break;
					default:
						break;
					}

					{
						{
							CopyMenuTextureObject(&tex, LoadMenuPaletteTexture(&gGame, 147 - StatusTextureOffset));
							SetMenuTevMode(GetGameGraphics(&gGame), 0);
							f32 width  = 1.1f * GetMenuTextureWidth(&tex);
							f32 height = GetMenuTextureHeight(&tex);
							f32 x      = this->mTextX;
							f32 y      = this->mTextY;
							u32 color  = 0xC8C8C800 | (u32)(255.0f * this->mAlpha);
							DrawMenuTexturedQuad(GetGameGraphics(&gGame), &tex, color, x, y, 0, width, height, 0, 0, rotation);
							SetMenuTevMode(GetGameGraphics(&gGame), 4);
						}
						{
							CopyMenuTextureObject(&tex, LoadMenuPaletteTexture(&gGame, 142 - StatusTextureOffset));
							SetMenuTevMode(GetGameGraphics(&gGame), 0);
							const f32 scale = 0.8f;
							f32 width       = scale * (1.1f * GetMenuTextureWidth(&tex));
							f32 height      = scale * GetMenuTextureHeight(&tex);
							f32 x           = width * 0.5f + 184;
							f32 y           = height * 0.5f + 60;
							u32 color       = 0xC8C8C800 | (u32)(255.0f * this->mAlpha);
							DrawMenuTexturedQuad(GetGameGraphics(&gGame), &tex, color, x, y, 0, width, height, 0, 0, 0);
							SetMenuTevMode(GetGameGraphics(&gGame), 4);
						}
#if !defined(VERSION_JP)
						if (GetGameLanguage(&gGame) == 6)
#endif
						{
							CopyMenuTextureObject(&tex, LoadMenuPaletteTexture(&gGame, 143 - StatusTextureOffset));
							SetMenuTevMode(GetGameGraphics(&gGame), 0);
							f32 width  = 1.1f * GetMenuTextureWidth(&tex);
							f32 height = GetMenuTextureHeight(&tex);
							f32 x      = width * 0.5f + 220;
							f32 y      = height * 0.5f + 66;
							u32 color  = 0xC8C8C800 | (u32)(255.0f * this->mAlpha);
							DrawMenuTexturedQuad(GetGameGraphics(&gGame), &tex, color, x, y, 0, width, height, 0, 0, 0);
							SetMenuTevMode(GetGameGraphics(&gGame), 4);
						}
					}
				}
				{

					{
						CopyMenuTextureObject(&tex, LoadMenuPaletteTexture(&gGame, 140 - StatusTextureOffset));
						SetMenuTevMode(GetGameGraphics(&gGame), 0);
						f32 width  = 1.1f * GetMenuTextureWidth(&tex);
						f32 height = GetMenuTextureHeight(&tex);
						f32 x      = width * 0.5f + 204;
						f32 y      = height * 0.5f + 335;
						u32 color  = 0xC8C8C800 | (u32)(255.0f * this->mAlpha);
						DrawMenuTexturedQuad(GetGameGraphics(&gGame), &tex, color, x, y, 0, width, height, 0, 0, 0);
						SetMenuTevMode(GetGameGraphics(&gGame), 4);
					}
					if (this->mMode == 2)
						sprintf(labelText, "%.255s", GetPCMenuLabelText(12));
					else
						sprintf(labelText, "%.255s", GetPCMenuLabelText(13));
					DrawGsProportionalTextFormat(GetGameGraphics(&gGame), (u32)(255.0f * this->mAlpha), labelText, 224, 334, 0, 20, 20);
					{
						CopyMenuTextureObject(&tex, LoadMenuPaletteTexture(&gGame, 139 - StatusTextureOffset));
						SetMenuTevMode(GetGameGraphics(&gGame), 0);
						f32 width  = 1.1f * GetMenuTextureWidth(&tex);
						f32 height = GetMenuTextureHeight(&tex);
						f32 x      = width * 0.5f + 344;
						f32 y      = height * 0.5f + 335;
						u32 color  = 0xC8C8C800 | (u32)(255.0f * this->mAlpha);
						DrawMenuTexturedQuad(GetGameGraphics(&gGame), &tex, color, x, y, 0, width, height, 0, 0, 0);
						SetMenuTevMode(GetGameGraphics(&gGame), 4);
					}
					{
						switch (this->mMode) {
						case 4:
							sprintf(labelText, "%.255s", GetPCMenuLabelText(14));
							break;
						case 3:
							switch (this->mPhase) {
							case 0:
							case 10:
							case 20:
								sprintf(labelText, "%.255s", GetPCMenuLabelText(16));
								break;
							case 1:
								sprintf(labelText, "%.255s", GetPCMenuLabelText(15));
								break;
							default:
								sprintf(labelText, "%.255s", GetPCMenuLabelText(13));
								break;
							}
							break;
						case 7:
							switch (this->mPhase) {
							case 0:
							case 10:
							case 20:
								sprintf(labelText, "%.255s", GetPCMenuLabelText(16));
								break;
							case 1:
								sprintf(labelText, "%.255s", GetPCMenuLabelText(25));
								break;
							default:
								sprintf(labelText, "%.255s", GetPCMenuLabelText(13));
								break;
							}
							break;
						case 10:
							switch (this->mPhase) {
							case 0:
								sprintf(labelText, "%.255s", GetPCMenuLabelText(16));
								break;
							case 1:
								sprintf(labelText, "%.255s", GetPCMenuLabelText(37));
								break;
							default:
								sprintf(labelText, "%.255s", GetPCMenuLabelText(13));
								break;
							}
							break;
						default:
							sprintf(labelText, "%.255s", GetPCMenuLabelText(16));
							break;
						}
					}
					DrawGsProportionalTextFormat(GetGameGraphics(&gGame), (u32)(255.0f * this->mAlpha), labelText, 364, 334, 0, 20, 20);
				}
			}
		}
		// The dialogue branch only runs for modes at or above 2.
		if (mMode >= 2) {
			const char* text;
			{
				switch (this->mMode) {
				case 2:
					switch (this->mGreetingVariant) {
					case 0:
						text = GetPCDialogueText(20) + this->mCharIndex;
						break;
					case 1:
						text = GetPCDialogueText(0) + this->mCharIndex;
						break;
					case 2:
						text = GetPCDialogueText(1) + this->mCharIndex;
						break;
					default:
						text = GetPCDialogueText(20) + this->mCharIndex;
						break;
					}
					break;
				case 4: {
					s32 level = GetPlayerLevel(GetPlayerParamsByIndex(&gGame, 0));
					text      = GetPCDialogueText(gPCLevelDialogueIds[level]) + this->mCharIndex;
					if (*text == '\t') {
						this->mInsertsNumber = 1;
						++this->mCharIndex;
					}
					break;
				}
				case 5:
					if (GetPCDialogueText(2)[this->mCharIndex] == '\a') {
						++this->mKeeperAnimStep;
						++this->mCharIndex;
						this->mPlayKeeperAnim = 1;
					}
					text = GetPCDialogueText(2) + this->mCharIndex;
					break;
				case 3:
					DrawPCItemPurchasePanel(this->mPurchaseSelection, this->mYesNo, 320.0f, 215.0f + this->mScroll, this->mTexScroll,
					                        this->mAlpha, this->mPanelAlpha);
					text = GetPCDialogueText(15) + this->mCharIndex;
					break;
				case 9:
					text = GetPCDialogueText(16) + this->mCharIndex;
					break;
				case 7:
					DrawPCScrapPurchasePanel(this->mShopSelection, this->mYesNo, 320.0f, 215.0f + this->mScroll, this->mTexScroll,
					                         this->mAlpha, this->mPanelAlpha);
					text = GetPCDialogueText(17) + this->mCharIndex;
					break;
				case 10:
					text = GetPCDialogueText(18) + this->mCharIndex;
					break;
				default:
					text = GetPCDialogueText(19) + this->mCharIndex;
					break;
				}
			}
			{
#if defined(VERSION_JP)
				if ((s32)++ this->mGlyphFrames >= 8) {
					this->mGlyphFrames                 = 0;
					this->mCharFlags[this->mNextGlyph] = 1;

					if (this->mInsertsNumber) {
						char number[32];
						sprintf(number, "%d", (s32)GetPCDialogueNumber(this));
						ConvertHudTextToFullWidth(number);
						SetPCDialogueGlyph(this, ReadPCDialogueGlyph(number + this->mNumberOffset));
						this->mNumberOffset += 2;
						if (!number[this->mNumberOffset]) {
							this->mNumberOffset  = 0;
							this->mInsertsNumber = 0;
						}
					} else if (!*text) {
						SetPCDialogueGlyph(this, 0x8140);
						this->mCharIndex      = 0;
						this->mNumberOffset   = 0;
						this->mKeeperAnimStep = 0;
					} else {
						SetPCDialogueGlyph(this, ReadPCDialogueGlyph(text));
						this->mCharIndex += 2;
					}

					if ((s32)++ this->mNextGlyph >= 40)
						this->mNextGlyph = 0;
				}
				for (int i = 0; i < 40; ++i) {
					if (!this->mCharFlags[i])
						continue;
					this->mCharX[i] -= 3.0f;
					if (this->mCharX[i] <= -22.0f) {
						this->mCharX[i]     = 640.0f;
						this->mCharFlags[i] = 0;
					}
					u32 color = (u32)(255.0f * this->mAlpha) | 0xC8C8C800;
					DrawGsProportionalTextFormat(GetGameGraphics(&gGame), color, this->mCharCopies[i], this->mCharX[i], 370.0f, 0.0f, 22.0f,
					                             22.0f);
				}
#else
				// Buffers are persistent; original strncpy calls leave the rest of each chunk intact.
				char(*chunks)[256] = gPCDialogueChunks;
#if defined(VERSION_PAL)
				const size_t length = strlen(text), span = length / 3;
#else
				const size_t span = strlen(text) / 2;
#endif
				// Keep a terminator in each buffer even for a longer replacement translation.
				const size_t n = span < 255 ? span : 255;
				strncpy(chunks[0], text, n);
				strncpy(chunks[1], text + span, n);
#if defined(VERSION_PAL)
				const size_t tailSize = length - 2 * span + 1;
				memcpy(chunks[2], text + 2 * span, tailSize < sizeof(chunks[2]) ? tailSize : sizeof(chunks[2]) - 1);
				chunks[2][sizeof(chunks[2]) - 1] = '\0';
#endif
				gPCDialogueX -= 3.0f;
				u32 color     = (u32)(255.0f * this->mAlpha) | 0xC8C8C800;
				CGs* graphics = GetGameGraphics(&gGame);
				DrawGsProportionalTextFormat(graphics, color, chunks[0], gPCDialogueX, 370.0f, 0.0f, 22.0f, 22.0f);
				color          = (u32)(255.0f * this->mAlpha) | 0xC8C8C800;
				f32 firstWidth = MeasureGameTextWidth(GetGameGraphics(&gGame), chunks[0], 1, 22.0f);
				f32 x          = gPCDialogueX + firstWidth;
				DrawGsProportionalTextFormat(GetGameGraphics(&gGame), color, chunks[1], x, 370.0f, 0.0f, 22.0f, 22.0f);
#if defined(VERSION_PAL)
				color           = (u32)(255.0f * this->mAlpha) | 0xC8C8C800;
				f32 secondWidth = MeasureGameTextWidth(GetGameGraphics(&gGame), chunks[1], 1, 22.0f);
				firstWidth      = MeasureGameTextWidth(GetGameGraphics(&gGame), chunks[0], 1, 22.0f);
				x               = (gPCDialogueX + firstWidth) + secondWidth;
				DrawGsProportionalTextFormat(GetGameGraphics(&gGame), color, chunks[2], x, 370.0f, 0.0f, 22.0f, 22.0f);
#endif
				f32 width = MeasureGameTextWidth(GetGameGraphics(&gGame), text, 1, 22.0f);
				if (gPCDialogueX + width <= 0)
					gPCDialogueX = 640.0f;
#endif
			}
		}
		return TRUE;
	}
	return FALSE;
}

static void (MenuStatus::*gStatusStepHandlers[])(u32)
    = { &MenuStatus::State0, &MenuStatus::State1, &MenuStatus::State2, &MenuStatus::State3, &MenuStatus::State4,
	    &MenuStatus::State5, &MenuStatus::State6, &MenuStatus::State7, &MenuStatus::State8, &MenuStatus::State9 };
static void (MenuPC::*gPCStepHandlers[])(u32)
    = { &MenuPC::State0, &MenuPC::State1, &MenuPC::State2, &MenuPC::State3, &MenuPC::State4,  &MenuPC::State5,
	    &MenuPC::State6, &MenuPC::State7, &MenuPC::State8, &MenuPC::State9, &MenuPC::State10, &MenuPC::State11 };
static void (MenuSave::*gSaveStepHandlers[])(u32)
    = { &MenuSave::State0,  &MenuSave::State1,  &MenuSave::State2,  &MenuSave::State3,  &MenuSave::State4,  &MenuSave::State5,
	    &MenuSave::State6,  &MenuSave::State7,  &MenuSave::State8,  &MenuSave::State9,  &MenuSave::State10, &MenuSave::State11,
	    &MenuSave::State12, &MenuSave::State13, &MenuSave::State14, &MenuSave::State15, &MenuSave::State16, &MenuSave::State17 };

#if defined(VERSION_JP)
enum {
	StatusTitleText       = 0x50,
	StatusEmptyText       = 0x0C,
	SaveTitleText         = 0x3D,
	MenuWindowTexture     = 136,
	UnavailableStatusIcon = 238,
	MenuTitleBlobTexture  = 106,
	FirstStatusTabTexture = 233
};
#else
enum {
	StatusTitleText       = 0x17,
	StatusEmptyText       = 0x0E,
	SaveTitleText         = 0x62,
	MenuWindowTexture     = 138,
	UnavailableStatusIcon = 240,
	MenuTitleBlobTexture  = 108,
	FirstStatusTabTexture = 235
};
#endif

extern "C" {
s32 gStatusTabTextOffsets[4] = { 1, 3, 2, 4 };
}

#if defined(VERSION_JP)
inline void MenuRequest::Open()
{
	CMenu::Open();
	gGame.mStatus.mFlags |= 2;
	mWindow.SetSize(0.0f, 0.0f);
	mWindow.SetPos(0.0f, 0.0f);
	mPhase = 0;
	_528   = 640.0f;
	_52C   = 0.0f;
	_530   = 0.0f;
	_534   = 0.0f;
	_53C   = 0.0f;
	mDelay = 0;
}

inline void MenuRequestClear::Open()
{
	CMenu::Open();
	gGame.mStatus.mFlags |= 2;
	mWindow.SetSize(0.0f, 0.0f);
	mWindow.SetPos(0.0f, 0.0f);
	mPhase      = 0;
	_170        = 640.0f;
	_174        = 0.0f;
	_178        = 0.0f;
	_17C        = 0.0f;
	_180        = 640.0f;
	_184        = 0.0f;
	_188        = 0.0f;
	_18C        = 0.0f;
	_190        = -10.0f;
	_194        = -200.0f;
	_198        = -10.0f;
	_19C        = 0.0f;
	_1B4        = 0.0f;
	_1B8        = 0.0f;
	mDelay      = 0;
	mTargets[0] = mValues[0];
	mTargets[2] = mValues[2];
	mTargets[1] = mValues[1];
	mTargets[3] = mValues[3];
	_1DC        = TRUE;
	_1E0        = 0.8f;
	_1E4        = 0.8f;
}

inline void MenuPadControl::Open()
{
	CMenu::Open();
	mWindow.mWidth  = 0.0f;
	mWindow.mHeight = 0.0f;
	mWindow.mX      = 0.0f;
	mWindow.mY      = 0.0f;
}

inline void MenuSound::Open()
{
	CMenu::Open();
	PlayGameSound(&gGame.mSound, 0x148, 0x50);
	OSReport("MenuSound::Activate\n");
}

inline void MenuSettings::Open()
{
	CMenu::Open();
	mValues[0] = (gGame.mStatus.mFlags & 0x0000080000000000ULL) ? TRUE : FALSE;
	mValues[1] = TRUE;
	mValues[2] = (gGame.mStatus.mFlags & 0x0000004000000000ULL) ? TRUE : FALSE;
	mValues[3] = (gGame.GetPlayer()->mPlug.mCord.mFlags >> 3) & 1;
	mValues[4] = gGame.GetPlayer()->mMenu.mAtc._280;
}
#endif

inline void MenuMain::Open()
{
	CMenu::Open();
	PlayGameSound(&gGame.mSound, 0x86, 0xFF);
	gGame.mStatus.mFlags |= 2;
}

inline void MenuMain::Close()
{
	CMenu::Close();
	PlayGameSound(&gGame.mSound, 0x86, 0xFF);
	gGame.mStatus.mFlags &= ~2ULL;
}

inline void MenuMain::Resume()
{
	if (mSuspended) {
		gGame.mStatus.mFlags |= 2;
	}
	CMenu::Resume();
}

inline void MenuMCTest::Open()
{
	mState = 1;
	Resume();
	mWindow.mOpen = TRUE;
	for (int i = 0; i < mFileCount; i++) {
		strcpy(mText[i], "");
	}
	sprintf(mText[mFileCount + 1], "SLOT: %c", sMenuMain.mChannel + 'A');
	StartMemoryCardPreviewRead(&gGame.mMemcard.mCard, sMenuMain.mChannel, 1);
	mOperation = 1;
}

inline void MenuStageSelect::Open()
{
	mState = 1;
	Resume();
	mWindow.mOpen = TRUE;
	if (gGame.mStatus.mFlags & 0x0008000000000000ULL) {
		mSelection = -1;
		int i      = 0;
		for (s32* stage = sStageOrder; *stage >= 0; stage++, i++) {
			if (*stage == gGame.mStage) {
				mSelection = i;
				break;
			}
		}
		if (mSelection < 0) {
			mSelection = 0;
		}
	} else {
		mSelection = gGame.mStage;
	}
	RebuildStageSelectList(this);
}

inline void MenuTitle::Set(const char* text, u32 color)
{
	const char* p = text;
	mActive       = TRUE;
	mCount        = 0;
	if (text) {
		mColor = color;
		int i;
		for (i = 0; i < 20 && *p; i++, p++) {
			mChars[i].mText   = p;
			mChars[i].mFade   = -0.2f * i;
			mChars[i].mHold   = FRAME_RATE;
			mChars[i].mAngle0 = 0.017453292f * (rand() % 360);
			mChars[i].mAngle1 = 0.017453292f * (rand() % 360);
			mCount++;
			if (IsJapanese() && (IsSjisLead(*p) & 1)) {
				p++;
			}
		}
		for (i = mCount; i < 20; i++) {
			mChars[i].mText = NULL;
			mChars[i].mFade = 0.0f;
			mChars[i].mHold = 0.0f;
		}
	}
}

/**
 * @note Address: 0x800313D8
 * @note Size: 0x88
 */
extern "C" void DrawOpenMenus(MenuEx* ex)
{
	if (ex->mActive) {
		for (int i = 0; i < (int)ARRAY_SIZE(ex->mMenus); i++) {
#if defined(VERSION_JP)
			if (ex->mMenus[i]->IsOpen()) {
#else
			if (ex->mMenus[i] && ex->mMenus[i]->IsOpen()) {
#endif
				ex->mMenus[i]->Draw();
			}
		}
	}
}

/**
 * @note Address: 0x80031460
 * @note Size: 0xB0
 */
extern "C" void UpdateOpenMenus(MenuEx* ex, u32 frames)
{
	ex->mActive = FALSE;
	for (int i = 0; i < (int)ARRAY_SIZE(ex->mMenus); i++) {
#if defined(VERSION_JP)
		if (ex->mMenus[i]->IsOpen() && ex->mMenus[i]->Update(frames)) {
#else
		if (ex->mMenus[i] && ex->mMenus[i]->IsOpen() && ex->mMenus[i]->Update(frames)) {
#endif
			ex->mActive = TRUE;
		}
	}
	ex->mCursor.UpdateObject(frames);
}

/**
 * @note Address: 0x80031510
 * @note Size: 0x74
 */
extern "C" void ResetMenuGroup(MenuEx* ex)
{
	ex->mCursor.ResetObjectState();
	for (int i = 0; i < (int)ARRAY_SIZE(ex->mMenus); i++) {
#if !defined(VERSION_JP)
		if (ex->mMenus[i])
#endif
		{
			ex->mMenus[i]->Reset();
		}
	}
}

/**
 * @note Address: 0x80031584
 * @note Size: 0xC8
 */
extern "C" BOOL LoadMenuGroupResources(MenuEx* ex)
{
	if (ex->mCursor.LoadFromPlacement(CoordInfo(GetSpawnData(12)))) {
		for (int i = 0; i < (int)ARRAY_SIZE(ex->mMenus); i++) {
#if !defined(VERSION_JP)
			if (ex->mMenus[i])
#endif
			{
				ex->mMenus[i]->LoadTexture();
				ex->mMenus[i]->mCursor = &ex->mCursor;
			}
		}
		return TRUE;
	}
	return FALSE;
}

/**
 * @note Address: 0x8003164C
 * @note Size: 0xF4
 */
extern "C" void InitGameMenus(MenuEx* ex)
{
	ex->mActive = FALSE;
#if !defined(VERSION_JP)
	for (int i = 0; i < (int)ARRAY_SIZE(ex->mMenus); i++) {
		ex->mMenus[i] = NULL;
	}
#endif
	ex->mMenus[0] = &sMenuMain;
	ex->mMenus[1] = &sMenuStatus;
	ex->mMenus[2] = &sMenuItem;
#if defined(VERSION_JP)
	ex->mMenus[3]  = &sMenuRequest;
	ex->mMenus[4]  = &sMenuPadControl;
	ex->mMenus[5]  = &sMenuSettings;
	ex->mMenus[6]  = &sMenuSound;
	ex->mMenus[7]  = &sMenuStageSelect;
	ex->mMenus[8]  = &sMenuPC;
	ex->mMenus[9]  = &sMenuRequestClear;
	ex->mMenus[10] = &sMenuMCTest;
	ex->mMenus[11] = &sMenuMCTestSub;
	ex->mMenus[12] = &sMenuSave;
#else
	ex->mMenus[3] = &sMenuPC;
	ex->mMenus[4] = &sMenuSave;
	ex->mMenus[5] = &sMenuStageSelect;
	ex->mMenus[6] = &sMenuMCTest;
	ex->mMenus[7] = &sMenuMCTestSub;
#endif
	for (int j = 0; j < (int)ARRAY_SIZE(ex->mMenus); j++) {
#if !defined(VERSION_JP)
		if (!ex->mMenus[j])
			continue;
#endif
		ex->mMenus[j]->Init();
	}
	ex->mCursor.InitializeObject(0);
}

/**
 * @note Address: 0x80031740
 * @note Size: 0x398
 */
BOOL MenuMCTestSub::Draw()
{
	if (CMenu::Draw()) {
		f32 x, y, width, step, height, maxW, lines, lineH;
		maxW   = mMaxWidth;
		width  = 30.0f + maxW;
		lineH  = mFontSize;
		step   = 2.0f + lineH;
		lines  = mRowCount;
		height = step * lines;
		x      = mWindow.mX;
		y      = mWindow.mY;
		for (int i = 0; i < lines; i++) {
			u32 color;
			if (i == mSelection) {
				color = (u32)(255.0f * mAlpha) | 0x20202000;
			} else {
				color = (u32)(255.0f * mAlpha) | 0x64646400;
			}
			if (*mText[i] == *"\x01") {
				f32 lineY  = y - height / 2 + step * (0.5f + i);
				f32 left   = x - width / 2;
				u32 shadow = (u32)(255.0f * mAlpha) | 0x64646400;
				DrawGsLineSegment(&gGame.mGs, NULL, left, lineY, 0.0f, shadow, left + width, lineY, 0.0f, shadow);
				u32 light = (u32)(255.0f * mAlpha) | 0xC8C8C800;
				DrawGsLineSegment(&gGame.mGs, NULL, 1.0f + left, 1.0f + lineY, 0.0f, light, 1.0f + (left + width), 1.0f + lineY, 0.0f,
				                  light);
			} else {
				DrawGsFixedWidthTextFormat(&gGame.mGs, color, mText[i], x - mTextWidth[i] / 2, y - height / 2 + step * i, 0.0f, lineH,
				                           lineH);
			}
		}
		DrawMenuSelectionCursor(mCursor, x - maxW / 2, y - height / 2 + step * mSelection, maxW, lineH, mAlpha, mAlpha);
		return TRUE;
	}
	return FALSE;
}

/**
 * @note Address: 0x80031AD8
 * @note Size: 0x3F4
 */
// NONMATCHING: string pool offsets, the strings before these aren't in yet
BOOL MenuMCTestSub::Update(u32 frames)
{
	if (CMenu::Update(frames) == 4) {
		CPad pad(0);
		if (pad.mPad->mRepeat & 0x10008) {
			do {
				if (--mSelection < 0) {
					mSelection = 3;
				}
			} while (*mText[mSelection] == *"\x01");
			PlayGameSound(&gGame.mSound, 0x87, 0xFF);
		} else if (pad.mPad->mRepeat & 0x20004) {
			do {
				if (++mSelection > 3) {
					mSelection = 0;
				}
			} while (*mText[mSelection] == *"\x01");
			PlayGameSound(&gGame.mSound, 0x87, 0xFF);
		} else if (pad.mPad->mTrigger & 0x200) {
			Close();
			sMenuMCTest.mSuspended   = FALSE;
			sMenuMCTest.mSuspendFade = 0;
			PlayGameSound(&gGame.mSound, 0x86, 0xFF);
		} else if (pad.mPad->mTrigger & 0x100) {
			s32 id                  = sMCTestSubLines[mSelection].mAction;
			Memcard* card           = &gGame.mMemcard.mCard;
			SavePreviewRecord* save = &card->mSaves[0][sMenuMCTest.mSelection];
			if ((id == 4 || id == 6) && save->mState == 0) {
				PlayGameSound(&gGame.mSound, 0x85, 0xFF);
			} else if (id == 4 && save->mState != 0 && (save->mStatus != 0 || !IsValidSavePreview(save))) {
				PlayGameSound(&gGame.mSound, 0x85, 0xFF);
			} else {
				Close();
				sMenuMCTest.mSuspended   = FALSE;
				sMenuMCTest.mSuspendFade = 0;
				switch (id) {
				case 0:
					sMenuMCTest.mOperation = 0;
					break;
				case 1:
					StartMemoryCardPreviewRead(card, sMenuMain.mChannel, 1);
					sMenuMCTest.mOperation = 1;
					break;
				case 2:
					sprintf(sMenuMCTest.mText[sMenuMCTest.mSelection], "(%d) saving...", sMenuMCTest.mSelection);
					StartMemoryCardSaveWrite(card, sMenuMain.mChannel, sMenuMCTest.mSelection, 1);
					sMenuMCTest.mOperation = 3;
					break;
				case 4:
					sprintf(sMenuMCTest.mText[sMenuMCTest.mSelection], "(%d) loading...", sMenuMCTest.mSelection);
					StartMemoryCardSaveRead(card, sMenuMain.mChannel, sMenuMCTest.mSelection, 1);
					sMenuMCTest.mOperation = 5;
					break;
				case 6:
					sprintf(sMenuMCTest.mText[sMenuMCTest.mSelection], "(%d) deleting...", sMenuMCTest.mSelection);
					StartMemoryCardSaveDeletion(card, sMenuMain.mChannel, sMenuMCTest.mSelection, 1);
					sMenuMCTest.mOperation = 7;
					break;
				}
				PlayGameSound(&gGame.mSound, 0x88, 0xFF);
			}
		}
	}
	return IsOpen();
}

/**
 * @note Address: 0x80031ECC
 * @note Size: 0x50
 */
void MenuMCTestSub::Open()
{
	mState = 1;
	Resume();
	mWindow.mOpen = TRUE;
	mSelection    = 0;
}

/**
 * @note Address: 0x800341EC
 * @note Size: 0x5C
 */
extern "C" void Init__15MenuStageSelectFv(Menu* menu)
{
	menu->CMenu::Init();
}

/**
 * @note Address: 0x80031F1C
 * @note Size: 0x17C
 */
void MenuMCTestSub::Init()
{
	MenuMCTestSub* menu = this;
	menu->CMenu::Init();
	menu->mText[0]  = sMCTestSubLines[0].mText;
	menu->mText[1]  = sMCTestSubLines[1].mText;
	menu->mText[2]  = sMCTestSubLines[2].mText;
	menu->mText[3]  = sMCTestSubLines[3].mText;
	menu->mRowCount = 4.0f;
	menu->mMaxWidth = 0.0f;
	menu->mFontSize = 16.0f;
	for (int i = 0; i < menu->mRowCount; i++) {
		menu->mTextWidth[i] = MeasureGameTextWidth(&gGame.mGs, menu->mText[i], 1, menu->mFontSize);
		if (menu->mTextWidth[i] > menu->mMaxWidth) {
			menu->mMaxWidth = menu->mTextWidth[i];
		}
	}
	menu->mWindow.SetSize(60.0f + menu->mMaxWidth, menu->mRowCount * (2.0f + menu->mFontSize) + 24.0f);
	menu->mWindow.SetPos(48.0f + (menu->mWindow.mWidth / 2 + 16.0f), 48.0f + (menu->mWindow.mHeight / 2 + 30.0f));
}

/**
 * @note Address: 0x80032098
 * @note Size: 0x4C4
 */
BOOL MenuMCTest::Draw()
{
	if (CMenu::Draw()) {
		f32 x, y, width, step, height, maxW, lines, lineH;
		maxW       = mMaxWidth;
		width      = 30.0f + maxW;
		lineH      = mFontSize;
		step       = 2.0f + lineH;
		lines      = mRowCount;
		height     = step * lines;
		x          = mWindow.mX;
		y          = mWindow.mY;
		s32 result = gGame.mMemcard.mCard.mCard.mResult;
		for (int i = 0; i < lines; i++) {
			u32 color;
			if (i == mSelection) {
				color = (u32)(255.0f * mAlpha) | 0x20202000;
			} else {
				color = (u32)(255.0f * mAlpha) | 0x64646400;
			}
			if (*mText[i] == *"\x01") {
				f32 lineY  = y - height / 2 + step * (0.5f + i);
				f32 left   = x - width / 2;
				u32 shadow = (u32)(255.0f * mAlpha) | 0x64646400;
				DrawGsLineSegment(&gGame.mGs, NULL, left, lineY, 0.0f, shadow, left + width, lineY, 0.0f, shadow);
				u32 light = (u32)(255.0f * mAlpha) | 0xC8C8C800;
				DrawGsLineSegment(&gGame.mGs, NULL, 1.0f + left, 1.0f + lineY, 0.0f, light, 1.0f + (left + width), 1.0f + lineY, 0.0f,
				                  light);
			} else if (result == 0 || result == -1 || result == -4 || result == -7 || result == -128 || i >= mFileCount) {
				DrawGsFixedWidthTextFormat(&gGame.mGs, color, mText[i], x - mMaxWidth / 2, y - height / 2 + step * i, 0.0f, lineH, lineH);
			} else {
				f32 textW = MeasureGameTextWidth(&gGame.mGs, mText[i], 0, mFontSize);
				DrawGsFixedWidthTextFormat(&gGame.mGs, (u32)(255.0f * mAlpha) | 0x20202000, mText[i], x - textW / 2,
				                           y - height / 2 + step * i, 0.0f, lineH, lineH);
			}
		}
		if (result == 0 || result == -1 || result == -4 || result == -7 || result == -128) {
			f32 cx = x - maxW / 2;
			f32 cy = y - height / 2 + step * mSelection;
			f32 alpha;
			if (sMenuMCTestSub.IsOpen()) {
				if (mState == 4) {
					alpha = 1.0f - sMenuMCTestSub.mWindow.mDrawAlpha;
				} else {
					alpha = 0.0f;
				}
			} else {
				alpha = mAlpha;
			}
			DrawMenuSelectionCursor(mCursor, cx, cy, maxW, lineH, mAlpha, alpha);
		}
		return TRUE;
	}
	return FALSE;
}

/**
 * @note Address: 0x80032CAC
 * @note Size: 0x178
 */
// NONMATCHING: string pool offsets, the strings before these aren't in yet
extern "C" void DispatchMemoryCardTestOperation(MenuMCTest* menu, s32 id)
{
	switch (id) {
	case 0:
		menu->mOperation = 0;
		break;
	case 1:
		StartMemoryCardPreviewRead(&gGame.mMemcard.mCard, sMenuMain.mChannel, 1);
		menu->mOperation = 1;
		break;
	case 2:
		sprintf(menu->mText[menu->mSelection], "(%d) saving...", menu->mSelection);
		StartMemoryCardSaveWrite(&gGame.mMemcard.mCard, sMenuMain.mChannel, menu->mSelection, 1);
		menu->mOperation = 3;
		break;
	case 4:
		sprintf(menu->mText[menu->mSelection], "(%d) loading...", menu->mSelection);
		StartMemoryCardSaveRead(&gGame.mMemcard.mCard, sMenuMain.mChannel, menu->mSelection, 1);
		menu->mOperation = 5;
		break;
	case 6:
		sprintf(menu->mText[menu->mSelection], "(%d) deleting...", menu->mSelection);
		StartMemoryCardSaveDeletion(&gGame.mMemcard.mCard, sMenuMain.mChannel, menu->mSelection, 1);
		menu->mOperation = 7;
		break;
	}
}

/**
 * @note Address: 0x80032E24
 * @note Size: 0xD8
 */
extern "C" void Open__10MenuMCTestFv(MenuMCTest* menu)
{
	menu->mState = 1;
	menu->Resume();
	menu->mWindow.mOpen = TRUE;
	for (int i = 0; i < menu->mFileCount; i++) {
		strcpy(menu->mText[i], "");
	}
	sprintf(menu->mText[menu->mFileCount + 1], "SLOT: %c", sMenuMain.mChannel + 'A');
	StartMemoryCardPreviewRead(&gGame.mMemcard.mCard, sMenuMain.mChannel, 1);
	menu->mOperation = 1;
}

/**
 * @note Address: 0x80032EFC
 * @note Size: 0x1E8
 */
void MenuMCTest::Init()
{
	MenuMCTest* menu = this;
	menu->CMenu::Init();
	menu->mFileCount = 10;
	for (int i = 0; i < menu->mFileCount; i++) {
		strcpy(menu->mText[i],
		       "(0) 00/00/00 00:00 [ 0:00] (000)\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40");
	}
	strcpy(menu->mText[menu->mFileCount], "\x01");
	strcpy(menu->mText[menu->mFileCount + 1], "");
	menu->mRowCount = menu->mFileCount + 2;
	menu->mMaxWidth = 0.0f;
	menu->mFontSize = 16.0f;
	for (int i = 0; i < menu->mRowCount; i++) {
		menu->mTextWidth[i] = MeasureGameTextWidth(&gGame.mGs, menu->mText[i], 0, menu->mFontSize);
		if (menu->mTextWidth[i] > menu->mMaxWidth) {
			menu->mMaxWidth = menu->mTextWidth[i];
		}
	}
	menu->mWindow.SetSize(60.0f + menu->mMaxWidth, menu->mRowCount * (2.0f + menu->mFontSize) + 24.0f);
	menu->mWindow.SetPos(24.0f + (menu->mWindow.mWidth / 2 + 16.0f), 24.0f + (menu->mWindow.mHeight / 2 + 30.0f));
	menu->mOperation = 0;
}

/**
 * @note Address: 0x800336DC
 * @note Size: 0x38C
 */
BOOL MenuStageSelect::Update(u32 frames)
{
	if (CMenu::Update(frames) == 4) {
		CPad pad(0);
		if (pad.mPad->mRepeat & 0x10008) {
			do {
				if (--mSelection < 0) {
					mSelection = mCount - 1;
				}
			} while (*mLabels[mSelection] == *"\x01");
			PlayGameSound(&gGame.mSound, 0x87, 0xFF);
		} else if (pad.mPad->mRepeat & 0x20004) {
			do {
				if (++mSelection > mCount - 1) {
					mSelection = 0;
				}
			} while (*mLabels[mSelection] == *"\x01");
			PlayGameSound(&gGame.mSound, 0x87, 0xFF);
		} else if (pad.mPad->mRepeat & 0x40001) {
			if (mSelection - 20 >= 0 && *mLabels[mSelection - 20] != *"\x01") {
				mSelection -= 20;
				PlayGameSound(&gGame.mSound, 0x87, 0xFF);
			}
		} else if (pad.mPad->mRepeat & 0x80002) {
			if (mSelection + 20 < mCount && *mLabels[mSelection + 20] != *"\x01") {
				mSelection += 20;
				PlayGameSound(&gGame.mSound, 0x87, 0xFF);
			}
		} else if (pad.mPad->mTrigger & 0x200) {
			Close();
			sMenuMain.Resume();
			PlayGameSound(&gGame.mSound, 0x86, 0xFF);
		} else if (pad.mPad->mTrigger & 0x100) {
			Close();
			sMenuMain.Close();
			if (gGame.mStatus.mFlags & 0x0008000000000000ULL) {
				ChangeGameStage(&gGame, sStageOrder[mSelection], 1);
			} else {
				ChangeGameStage(&gGame, mSelection, 1);
			}
			PlayGameSound(&gGame.mSound, 0x88, 0xFF);
		}
	}
	return IsOpen();
}

/**
 * @note Address: 0x80034108
 * @note Size: 0xE4
 */
extern "C" void Open__15MenuStageSelectFv(MenuStageSelect* menu)
{
	menu->mState = 1;
	menu->Resume();
	menu->mWindow.mOpen = TRUE;
	if (gGame.mStatus.mFlags & 0x0008000000000000ULL) {
		menu->mSelection = -1;
		s32* stages      = sStageOrder;
		s32 stage;
		s32 current = gGame.mStage;
		for (int i = 0; stages[i] >= 0; i++) {
			stage = stages[i];
			if (stage == current) {
				menu->mSelection = i;
				break;
			}
		}
		if (menu->mSelection < 0) {
			menu->mSelection = 0;
		}
	} else {
		menu->mSelection = gGame.mStage;
	}
	RebuildStageSelectList(menu);
}

/**
 * @note Address: 0x8003D350
 * @note Size: 0x3AC
 */
// NONMATCHING: regalloc
void MenuSave::Open()
{
	mState = 1;
	Resume();
	mWindow.mOpen = TRUE;
	gGame.mStatus.mFlags |= 2;
	mWindow.SetSize(0.0f, 0.0f);
	mWindow.SetPos(0.0f, 0.0f);
	mPanelX       = -10.0f;
	mPanelY       = -200.0f;
	mTargetPanelX = -10.0f;
	mTargetPanelY = 0.0f;
	_168          = 0.0f;
	mBannerSlide  = 0.0f;
	_170          = 0.0f;
	mOffset       = 0.0f;
	mDrawAlpha    = 0.0f;
	mTargetAlpha  = 1.0f;
	mMode         = 0;
	mPrevMode     = mMode;
	mPhase        = 0;
	mTime         = 0.0f;
	mDelay        = 0;
	if (mLastFile == -1) {
		mFile = gGame.mInfoScrn.mSelectedSaveSlot;
	} else {
		mFile = mLastFile;
	}
	mChoice      = 0;
	mResult      = 0;
	mMessage     = 0;
	mReady       = 0;
	mShowMessage = 0;
	mWrongDevice = 0;
	mSaved       = 0;
	mChannel     = 0;
	mTexScroll   = 0.0f;
#if !defined(VERSION_JP)
#if !defined(VERSION_JP)
	mFormatCancelled = 0;
#endif
#endif
	PlayGameSound(&gGame.mSound, 0x14B, 0xFF);
	u32 color = (u32)(255.0f * mTargetAlpha) | 0xFFFFFF00;
	mTitle.Set(GetLocalizedInterfaceText(SaveTitleText), color);
}

/**
 * @note Address: 0x8003D6FC
 * @note Size: 0x34
 */
void MenuSave::Reset()
{
	CMenu::Reset();
	mLastFile = -1;
}

/**
 * @note Address: 0x8003D730
 * @note Size: 0x70
 */
void MenuSave::Init()
{
	MenuSave* menu = this;
	menu->CMenu::Init();
	menu->mLastFile = -1;
}

/**
 * @note Address: 0x8004ED74
 * @note Size: 0x74
 */
void MenuPC::Init()
{
	MenuPC* menu = this;
	menu->CMenu::Init();
	menu->mDepositBalance = 0;
	menu->mDialogueNumber = 0;
}

/**
 * @note Address: 0x8005C728
 * @note Size: 0x70
 */
void MenuStatus::Init()
{
	MenuStatus* menu = this;
	menu->CMenu::Init();
	menu->mSelection = 0;
}

/**
 * @note Address: 0x80060768
 * @note Size: 0x70
 */
void MenuItem::Init()
{
	MenuItem* menu = this;
	menu->CMenu::Init();
	menu->mSelection = 0;
}

/**
 * @note Address: 0x800611D4
 * @note Size: 0x30
 */
extern "C" void Resume__8MenuMainFv(MenuMain* menu)
{
	menu->MenuMain::Resume();
}

/**
 * @note Address: 0x80061204
 * @note Size: 0x8C
 */
extern "C" void Close__8MenuMainFv(MenuMain* menu)
{
	menu->MenuMain::Close();
}

/**
 * @note Address: 0x80061290
 * @note Size: 0x78
 */
extern "C" void Open__8MenuMainFv(MenuMain* menu)
{
	menu->MenuMain::Open();
}

/**
 * @note Address: 0x80061308
 * @note Size: 0x5C
 */
extern "C" void AddMainMenuExtraRow(MenuMain* menu, char* text, s32 id)
{
	for (int i = 0; i < 5; i++) {
		if (menu->mExtraRows[i].mText == NULL) {
			menu->mExtraRows[i].mText   = text;
			menu->mExtraRows[i].mAction = id;
			break;
		}
	}
	LayoutMainMenuRows(menu);
}

/**
 * @note Address: 0x80061364
 * @note Size: 0x4C
 */
extern "C" void ClearMainMenuExtraRows(MenuMain* menu)
{
	for (int i = 0; i < 5; i++) {
		menu->mExtraRows[i].mText   = NULL;
		menu->mExtraRows[i].mAction = 0;
	}
	LayoutMainMenuRows(menu);
}

/**
 * @note Address: 0x800613B0
 * @note Size: 0x74
 */
void MenuMain::Reset()
{
	CMenu::Reset();
	mChannel = 0;
	for (int i = 0; i < 5; i++) {
		mExtraRows[i].mText   = NULL;
		mExtraRows[i].mAction = 0;
	}
	LayoutMainMenuRows(this);
}

/**
 * @note Address: 0x800607D8
 * @note Size: 0x504
 */
BOOL MenuMain::Draw()
{
	if (CMenu::Draw()) {
		f32 x, y, width, step, height, maxW, lines, lineH;
		maxW   = mMaxWidth;
		width  = 30.0f + maxW;
		lineH  = mFontSize;
		step   = 2.0f + lineH;
		lines  = mRowCount;
		height = step * lines;
		x      = mWindow.mX;
		y      = mWindow.mY;
		for (int i = 0; i < lines; i++) {
			u32 color;
			if (i == mSelection) {
				color = (u32)(255.0f * mAlpha) | 0x20202000;
			} else {
				color = (u32)(255.0f * mAlpha) | 0x64646400;
			}
			if (*mText[i] == *"\x01") {
				f32 lineY  = y - height / 2 + step * (0.5f + i);
				f32 left   = x - width / 2;
				u32 shadow = (u32)(255.0f * mAlpha) | 0x64646400;
				DrawGsLineSegment(&gGame.mGs, NULL, left, lineY, 0.0f, shadow, left + width, lineY, 0.0f, shadow);
				u32 light = (u32)(255.0f * mAlpha) | 0xC8C8C800;
				DrawGsLineSegment(&gGame.mGs, NULL, 1.0f + left, 1.0f + lineY, 0.0f, light, 1.0f + (left + width), 1.0f + lineY, 0.0f,
				                  light);
			} else {
				DrawGsFixedWidthTextFormat(&gGame.mGs, color, mText[i], x - mMaxWidth / 2, y - height / 2 + step * i, 0.0f, lineH, lineH);
			}
		}
		f32 cx = x - maxW / 2;
		f32 cy = y - height / 2 + step * mSelection;
		f32 alpha;
		if (sMenuStatus.IsOpen()) {
			alpha = mState == 4 ? 1.0f - sMenuStatus.mWindow.mDrawAlpha : 0.0f;
#if defined(VERSION_JP)
		} else if (sMenuSettings.IsOpen()) {
			alpha = mState == 4 ? 1.0f - sMenuSettings.mWindow.mDrawAlpha : 0.0f;
#endif
		} else if (sMenuMCTest.IsOpen()) {
			alpha = mState == 4 ? 1.0f - sMenuMCTest.mWindow.mDrawAlpha : 0.0f;
		} else if (sMenuStageSelect.IsOpen()) {
			alpha = mState == 4 ? 1.0f - sMenuStageSelect.mWindow.mDrawAlpha : 0.0f;
#if defined(VERSION_JP)
		} else if (sMenuPadControl.IsOpen()) {
			alpha = mState == 4 ? 1.0f - sMenuPadControl.mWindow.mDrawAlpha : 0.0f;
		} else if (sMenuRequest.IsOpen()) {
			alpha = mState == 4 ? 1.0f - sMenuRequest.mWindow.mDrawAlpha : 0.0f;
		} else if (sMenuSound.IsOpen()) {
			alpha = mState == 4 ? 1.0f - sMenuSound.mWindow.mDrawAlpha : 0.0f;
#endif
		} else if (sMenuPC.IsOpen()) {
			alpha = mState == 4 ? 1.0f - sMenuPC.mWindow.mDrawAlpha : 0.0f;
#if defined(VERSION_JP)
		} else if (sMenuRequestClear.IsOpen()) {
			alpha = mState == 4 ? 1.0f - sMenuRequestClear.mWindow.mDrawAlpha : 0.0f;
#endif
		} else if (sMenuSave.IsOpen()) {
			alpha = mState == 4 ? 1.0f - sMenuSave.mWindow.mDrawAlpha : 0.0f;
		} else {
			alpha = mAlpha;
		}
		DrawMenuSelectionCursor(mCursor, cx, cy, maxW, lineH, mAlpha, alpha);
		return TRUE;
	}
	return FALSE;
}

/**
 * @note Address: 0x80060CDC
 * @note Size: 0x4F8
 */
BOOL MenuMain::Update(u32 frames)
{
	if (CMenu::Update(frames) == 4) {
		CPad pad(0);
		s32 id = -1;
		if (pad.mPad->mRepeat & 0x10008) {
			do {
				if (--mSelection < 0) {
					mSelection = mRowCount - 1.0f;
				}
			} while (*mText[mSelection] == *"\x01");
			PlayGameSound(&gGame.mSound, 0x87, 0xFF);
		} else if (pad.mPad->mRepeat & 0x20004) {
			do {
				if (++mSelection > mRowCount - 1.0f) {
					mSelection = 0;
				}
			} while (*mText[mSelection] == *"\x01");
			PlayGameSound(&gGame.mSound, 0x87, 0xFF);
		} else if (pad.mPad->mRepeat & 0xC0003) {
			if (sMainLines[mSelection].mAction == MainMemCardAction) {
				mChannel = (mChannel + 1) & 1;
				PlayGameSound(&gGame.mSound, 0x87, 0xFF);
				mText[mSelection][8] = mChannel + 'A';
			}
		} else if (pad.mPad->mTrigger & 0x1200) {
			Close();
		} else if (pad.mPad->mTrigger & 0x100) {
			if (mSelection < mBaseRows) {
				id = sMainLines[mSelection].mAction;
			} else {
				Close();
				StartOrQueueEventScript(&gGame.mEvt, (u8*)mExtraRows[mSelection - (mBaseRows + 1)].mAction, 0, -1, -1);
			}
		}
		if (id >= 0) {
			BOOL fade = TRUE;
			switch (id) {
			case 1:
				sMenuStatus.MenuStatus::Open();
				break;
			case 2:
				sMenuItem.MenuItem::Open();
				break;
#if defined(VERSION_JP)
			case 3:
				sMenuRequest.Open();
				break;
			case 4:
				sMenuPadControl.Open();
				break;
			case 5:
				fade = FALSE;
				sMenuSettings.Open();
				break;
			case 6:
				fade = FALSE;
				sMenuSound.Open();
				break;
#endif
			case MainMemCardAction:
				fade = FALSE;
				sMenuMCTest.Open();
				break;
			case MainSelectStageAction:
				fade = FALSE;
				sMenuStageSelect.Open();
				break;
			case MainPCAction:
				sMenuPC.MenuPC::Open();
				break;
#if defined(VERSION_JP)
			case 9:
				sMenuRequestClear.Open();
				break;
#endif
			case MainSaveAction:
				sMenuSave.MenuSave::Open();
				break;
			}
			if (id >= 0) {
				PlayGameSound(&gGame.mSound, 0x88, 0xFF);
				Suspend(fade);
			} else {
				PlayGameSound(&gGame.mSound, 0x85, 0xFF);
			}
		}
	}
	return IsOpen();
}

/**
 * @note Address: 0x80061424
 * @note Size: 0x5C
 */
void MenuMain::Init()
{
	MenuMain* menu = this;
	menu->CMenu::Init();
}

/**
 * @note Address: 0x80061480
 * @note Size: 0x2B8
 */
extern "C" void LayoutMainMenuRows(MenuMain* menu)
{
	menu->mFontSize = 16.0f;
	menu->mBaseRows = 4;
	menu->mRowCount = menu->mBaseRows;
	menu->mMaxWidth = 0.0f;
	for (int i = 0; i < menu->mRowCount; i++) {
		menu->mText[i] = sMainLines[i].mText;
		if (sMainLines[i].mAction == MainMemCardAction) {
			menu->mText[i][8] = menu->mChannel + 'A';
		}
		menu->mTextWidth[i] = MeasureGameTextWidth(&gGame.mGs, menu->mText[i], 0, menu->mFontSize);
		if (menu->mTextWidth[i] > menu->mMaxWidth) {
			menu->mMaxWidth = menu->mTextWidth[i];
		}
	}
	int items = 0;
	for (int k = 0; k < 5; k++) {
		if (menu->mExtraRows[k].mText) {
			items++;
		}
	}
	int line = menu->mRowCount;
	if (items > 0) {
		menu->mText[line]      = "\x01";
		menu->mTextWidth[line] = 1.0f;
		line++;
		for (int j = 0; j < items; j++) {
			if (menu->mExtraRows[j].mText) {
				menu->mText[line]      = menu->mExtraRows[j].mText;
				menu->mTextWidth[line] = MeasureGameTextWidth(&gGame.mGs, menu->mText[line], 0, menu->mFontSize);
				if (menu->mTextWidth[line] > menu->mMaxWidth) {
					menu->mMaxWidth = menu->mTextWidth[line];
				}
				line++;
			}
		}
		menu->mRowCount = line;
	}
	if (menu->mSelection >= menu->mRowCount) {
		menu->mSelection = menu->mRowCount - 1.0f;
	}
	menu->mWindow.SetSize(60.0f + menu->mMaxWidth, menu->mRowCount * (2.0f + menu->mFontSize) + 24.0f);
	menu->mWindow.SetPos(menu->mWindow.mWidth / 2 + 16.0f, menu->mWindow.mHeight / 2 + 30.0f);
}

/**
 * @note Address: 0x80061738
 * @note Size: 0xA4
 */
extern "C" BOOL Draw__5CMenuFv(CMenu* menu)
{
	return menu->CMenu::Draw();
}

/**
 * @note Address: 0x800617DC
 * @note Size: 0x2E0
 */
BOOL CMenu::Update(u32 frames)
{
	if (mSuspended) {
		if (mSuspendFade != 0 && mSuspendAlpha > 0.0f) {
			if ((mSuspendAlpha -= (1.0f / (0.2f * FRAME_RATE)) * frames) < 0.0f) {
				mSuspendAlpha = 0.0f;
			}
		}
	} else if (mSuspendAlpha < 1.0f) {
		if ((mSuspendAlpha += (1.0f / (0.2f * FRAME_RATE)) * frames) > 1.0f) {
			mSuspendAlpha = 1.0f;
		}
	}
	if (!IsOpen()) {
		return FALSE;
	}
	if (mSuspended) {
		mAlpha             = mFade * mSuspendAlpha;
		mWindow.mDrawAlpha = mAlpha;
		return FALSE;
	}
	CPad pad(0);
	switch (mState) {
	case 0:
		break;
	case 1:
		mState = 2;
	case 2:
		mState = 3;
		break;
	case 3:
		if (mFade < 1.0f) {
			if ((mFade += (1.0f / (0.2f * FRAME_RATE)) * frames) > 1.0f) {
				mFade = 1.0f;
			}
		}
		if (mFade >= 1.0f) {
			mState = 4;
		}
		break;
	case 4:
		break;
	case 5:
		if (mFade > 0.0f) {
			if ((mFade -= (1.0f / (0.2f * FRAME_RATE)) * frames) < 0.0f) {
				mFade = 0.0f;
			}
		}
		if (0.0f == mFade) {
			mState = 6;
		}
		mWindow.mOpen = FALSE;
		break;
	case 6:
		break;
	}
	mAlpha             = mFade * mSuspendAlpha;
	mWindow.mDrawAlpha = mAlpha;
	if (mWindow.mOpen) {
		mWindow.mTex.mX += 0.002f * frames;
		mWindow.mTex.mY += 0.002f * frames;
		PSMTXTrans(mWindow.mTex.mTex.mMtx[0], mWindow.mTex.mX, mWindow.mTex.mY, 0.0f);
		mWindow.mTex.mTex.mMtxDirty = TRUE;
		PSMTXScale(mWindow.mTex.mTex.mMtx[1], mWindow.mTex.mScaleX, mWindow.mTex.mScaleY, 1.0f);
		mWindow.mTex.mTex.mMtxDirty = TRUE;
		CPad pad2(0);
	}
	return mState;
}

/**
 * @note Address: 0x80061AE8
 * @note Size: 0x18C
 */
BOOL CMenu::LoadTexture()
{
	CMenu* menu       = this;
	MenuWindow* win   = &menu->mWindow;
	win->mTex.mTex    = (&gGame.mTpl)->mTex.Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[MenuWindowTexture], 0);
	CTexObj* tex      = &win->mTex.mTex;
	u16 w             = GXGetTexObjWidth(tex);
	win->mTex.mScaleX = win->mWidth / w;
	u16 h             = GXGetTexObjHeight(tex);
	win->mTex.mScaleY = win->mHeight / h;
	PSMTXTrans(tex->mMtx[0], win->mTex.mX, win->mTex.mY, 0.0f);
	win->mTex.mTex.mMtxDirty = TRUE;
	PSMTXScale(tex->mMtx[1], win->mTex.mScaleX, win->mTex.mScaleY, 1.0f);
	win->mTex.mTex.mMtxDirty = TRUE;
	return TRUE;
}

/**
 * @note Address: 0x80061ABC
 * @note Size: 0x2C
 */
extern "C" void Reset__5CMenuFv(CMenu* menu)
{
	menu->CMenu::Reset();
}

/**
 * @note Address: 0x80061C74
 * @note Size: 0x5C
 */
extern "C" void Init__5CMenuFv(CMenu* menu)
{
	menu->CMenu::Init();
}

/**
 * @note Address: 0x8003AC48
 * @note Size: 0xE0
 */
void MenuSave::State17(u32)
{
	MenuSave* menu = this;
	if (menu->mDrawAlpha <= 0.0f) {
		StopGameSfx(&gGame.mSound, 0x14B);
		PlayGameSound(&gGame.mSound, 0x185, 0xFF);
		gGame.mStatus.mFlags &= ~2ULL;
		menu->Close();
		sMenuMain.Resume();
	}
}

/**
 * @note Address: 0x8003AD28
 * @note Size: 0x80
 */
// NONMATCHING: regalloc
void MenuSave::State16(u32)
{
	MenuSave* menu = this;
	for (int i = 0; i < 20; i++) {
		menu->mTitle.mChars[i].mHold = 0.0f;
	}
	menu->mTargetAlpha  = 0.0f;
	menu->mTargetPanelY = -200.0f;
	menu->mBannerSlide  = 0.0f;
	menu->SetMode(0x11);
}

/**
 * @note Address: 0x8003ADA8
 * @note Size: 0x20
 */
void MenuSave::State15(u32 frames)
{
	State14(frames);
}

/**
 * @note Address: 0x8003BE90
 * @note Size: 0x4
 */
void MenuSave::State7(u32)
{
}

/**
 * @note Address: 0x8003D7A0
 * @note Size: 0x24
 */
extern "C" char* GetTitleAndSaveMenuText(s32 id)
{
#if defined(VERSION_JP)
	return GetLocalizedInterfaceText(id + 13);
#else
	return GetLocalizedInterfaceText(id + 94);
#endif
}

/**
 * @note Address: 0x8003D7C4
 * @note Size: 0x18
 */
extern "C" BOOL IsSaveSlotIntact(MenuSaveSlot* slot)
{
	return slot->mState != 1;
}

/**
 * @note Address: 0x8003D7DC
 * @note Size: 0x44
 */
extern "C" void ResetSaveSlotRecord(MenuSaveSlot* slot)
{
	ClearSaveSlot(slot);
}

/**
 * @note Address: 0x80048EB0
 * @note Size: 0x8
 */
extern "C" s32 GetPlayerLevel(Params* params)
{
	return params->mLevel;
}

/**
 * @note Address: 0x80048EB8
 * @note Size: 0x20
 */
extern "C" Params* GetPlayerParamsByIndex(CGame* game, s32 i)
{
	return &((XObjChr*)game->mXObjs.mPools[2].Get(i))->mParams;
}

/**
 * @note Address: 0x80048ED8
 * @note Size: 0x2C
 */
extern "C" void DrawMenuLine(CGs* gs, u32 color, f32 x0, f32 y0, f32 z0, f32 x1, f32 y1, f32 z1)
{
	DrawGsLineSegment(gs, NULL, x0, y0, z0, color, x1, y1, z1, color);
}

/**
 * @note Address: 0x80048F04
 * @note Size: 0x1E0
 */
extern "C" void DrawPCExchangeRateIcon(f32 x, f32 y, f32 alpha)
{
	CTexObj icon = (&gGame.mTpl)->mTex.Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[119], 0);
	PSMTXScale(icon.mMtx[1], 0.05882353f, 1.0f, 1.0f);
	icon.mMtxDirty = TRUE;
	PSMTXTrans(icon.mMtx[0], 0.11764706f, 0.0f, 0.0f);
	icon.mMtxDirty = TRUE;
	SetGsTevMode(gGame.mGs.mCurTevStage, 0);
	u32 color = (u32)(255.0f * alpha) | 0xFFFFFF00;
	DrawGsTexturedQuad(&gGame.mGs, &icon, color, color, color, color, x, y, 0.0f, 18.0f, 18.0f, 0.0f, 0.0f, 0.0f);
	SetGsTevMode(gGame.mGs.mCurTevStage, 4);
}

/**
 * @note Address: 0x800490E4
 * @note Size: 0x8
 */
extern "C" CTexObj* GetSubwindowTexture(CGame* game)
{
	return &game->mSubwin.mTexture.mTex;
}

/**
 * @note Address: 0x800490EC
 * @note Size: 0x8
 */
extern "C" s32 GetShopCatalogFirstRow(const MenuPCPage* page)
{
	return page->mStart;
}

/**
 * @note Address: 0x800490F4
 * @note Size: 0x8
 */
extern "C" s32 GetShopHistoryFirstRow(const MenuPCPage* page)
{
	return page->mStart;
}

/**
 * @note Address: 0x800490FC
 * @note Size: 0x3C
 */
extern "C" void SetMenuTextureTranslation(CTexObj* tex, f32 x, f32 y)
{
	PSMTXTrans(tex->mMtx[0], x, y, 0.0f);
	tex->mMtxDirty = TRUE;
}

/**
 * @note Address: 0x80049138
 * @note Size: 0x3C
 */
extern "C" void SetMenuTextureScale(CTexObj* tex, f32 x, f32 y)
{
	PSMTXScale(tex->mMtx[1], x, y, 1.0f);
	tex->mMtxDirty = TRUE;
}

/**
 * @note Address: 0x80049174
 * @note Size: 0x6C
 */
// NONMATCHING: scheduling
extern "C" void DrawSolidQuad(CGs* gs, Vec* quad, u32 color)
{
	DrawGsVertexQuad(gs, quad[0].x, quad[0].y, quad[0].z, color, quad[1].x, quad[1].y, quad[1].z, color, quad[2].x, quad[2].y, quad[2].z,
	                 color, quad[3].x, quad[3].y, quad[3].z, color);
}

/**
 * @note Address: 0x8003EBBC
 * @note Size: 0x354
 */
// NONMATCHING: register allocation and instruction scheduling
extern "C" void DrawPCShopScrollBar(f32 x, f32 y, f32 start, f32 end, f32 alpha, f32 /*scale*/)
{
	SetGsChannelEnabled(&gGame.mGs, 0, 0);
	SetGsBlendMode(gGame.mGs.mCurRenderMode, 1, 4, 5, 15);
	SetGsDepthMode(gGame.mGs.mCurRenderMode, 0, 3, 0);
	y               = 48.0f + (1.0f + y);
	x               = 10.0f + (27.0f + (x - 4.0f));
	f32 top         = 4.0f + y;
	f32 bottom      = 172.0f + top;
	f32 first       = 172.0f * start + top;
	f32 last        = 172.0f * end + top;
	Vec quads[4][4] = {};
	quads[0][0].x   = x;
	quads[0][0].y   = top;
	quads[0][1].x   = 4.0f + x;
	quads[0][1].y   = top;
	quads[0][2].x   = 4.0f + x;
	quads[0][2].y   = bottom;
	quads[0][3].x   = x;
	quads[0][3].y   = bottom;
	quads[1][0].x   = x;
	quads[1][0].y   = top;
	quads[1][1].x   = 4.0f + x;
	quads[1][1].y   = top;
	quads[1][2].x   = 4.0f + x;
	quads[1][2].y   = first;
	quads[1][3].x   = x;
	quads[1][3].y   = first;
	quads[2][0].x   = x;
	quads[2][0].y   = first;
	quads[2][1].x   = 4.0f + x;
	quads[2][1].y   = first;
	quads[2][2].x   = 4.0f + x;
	quads[2][2].y   = last;
	quads[2][3].x   = x;
	quads[2][3].y   = last;
	quads[3][0].x   = x;
	quads[3][0].y   = last;
	quads[3][1].x   = 4.0f + x;
	quads[3][1].y   = last;
	quads[3][2].x   = 4.0f + x;
	quads[3][2].y   = bottom;
	quads[3][3].x   = x;
	quads[3][3].y   = bottom;
	u32 color       = (u32)(255.0f * alpha);
	DrawGsLineStrip(&gGame.mGs, quads[0], color | 0xC8C8C800, 5);
	SetGsLineWidth(gGame.mGs.mCurRenderMode, 6, 5);
	DrawSolidQuad(&gGame.mGs, quads[1], color);
	DrawSolidQuad(&gGame.mGs, quads[2], color | 0xFFB62800);
	DrawSolidQuad(&gGame.mGs, quads[3], color);
	DrawGsLineStrip(&gGame.mGs, quads[2], color | 0xFFFFFF00, 5);
	SetGsLineWidth(gGame.mGs.mCurRenderMode, 12, 5);
}

/**
 * @note Address: 0x800491E0
 * @note Size: 0x24
 */
extern "C" void SetMenuLineWidth(CGs* gs, u8 width, s32 texOffs)
{
	SetGsLineWidth(gs->mCurRenderMode, width, texOffs);
}

/**
 * @note Address: 0x80049204
 * @note Size: 0x24
 */
extern "C" void SetMenuBlendMode(CGs* gs, s32 type, s32 src, s32 dst, s32 op)
{
	SetGsBlendMode(gs->mCurRenderMode, type, src, dst, op);
}

#if !defined(VERSION_JP)
/**
 * @note Address: 0x80049228
 * @note Size: 0x8
 */
extern "C" u8 GetGameLanguage(CBase* base)
{
	return base->mLanguage;
}
#endif

/**
 * @note Address: 0x80049230
 * @note Size: 0x2C
 */
extern "C" void DrawMenuTexturedQuad(CGs* gs, CTexObj* tex, u32 color, f32 x, f32 y, f32 z, f32 w, f32 h, f32 a, f32 b, f32 c)
{
	DrawGsTexturedQuad(gs, tex, color, color, color, color, x, y, z, w, h, a, b, c);
}

/**
 * @note Address: 0x8004925C
 * @note Size: 0x20
 */
extern "C" u16 GetMenuTextureHeight(CTexObj* tex)
{
	return GXGetTexObjHeight(tex);
}

/**
 * @note Address: 0x8004927C
 * @note Size: 0x20
 */
extern "C" u16 GetMenuTextureWidth(CTexObj* tex)
{
	return GXGetTexObjWidth(tex);
}

/**
 * @note Address: 0x8004929C
 * @note Size: 0x24
 */
extern "C" void SetMenuTevMode(CGs* gs, s32 mode)
{
	SetGsTevMode(gs->mCurTevStage, mode);
}

/**
 * @note Address: 0x800492C0
 * @note Size: 0xA0
 */
extern "C" CTexObj* CopyMenuTextureObject(CTexObj* dst, const CTexObj* src)
{
	*dst = *src;
	return dst;
}

/**
 * @note Address: 0x80049360
 * @note Size: 0x38
 */
extern "C" CTexObj* LoadMenuPaletteTexture(CGame* game, s32 i)
{
	return &(&game->mTpl)->mTex.Init(&((TPLPalette*)(&game->mTpl)->mBuffer)->descriptorArray[i], 0);
}

/**
 * @note Address: 0x80049398
 * @note Size: 0x38
 */
extern "C" void DrawMenuTriangleFan(CGs* gs, Vec* pos, u32 color, u16 count)
{
	DrawGsTrianglePrimitive(gs, GX_TRIANGLEFAN, gs->mCamera->mViewMtx, pos, color, count);
}

/**
 * @note Address: 0x800493D0
 * @note Size: 0x24
 */
extern "C" void SetMenuDepthMode(CGs* gs, u8 enable, s32 func, u8 update)
{
	SetGsDepthMode(gs->mCurRenderMode, enable, func, update);
}

/**
 * @note Address: 0x800493F4
 * @note Size: 0x4
 */
extern "C" CGs* GetGameGraphics(CGame* game)
{
	return &game->mGs;
}

/**
 * @note Address: 0x800493F8
 * @note Size: 0x20
 */
extern "C" const char* GetPCDialogueText(s32 id)
{
	return GetLocalizedPCDialogue(id);
}

/**
 * @note Address: 0x8004AB3C
 * @note Size: 0x4
 */
void MenuPC::State8(u32)
{
}

/**
 * @note Address: 0x8004B928
 * @note Size: 0x4
 */
void MenuPC::State6(u32)
{
}

/**
 * @note Address: 0x80051C60
 * @note Size: 0x5C
 */
extern "C" BOOL IsUtilibotStatusIconUnlocked()
{
	BOOL found = FALSE;
	u32 id     = GetStatusIconEventFlagId(3);
	if (gEventFlags.mFlags[id / 32] & (1u << (id % 32))) {
		found = TRUE;
	}
	return found;
}

/**
 * @note Address: 0x8005875C
 * @note Size: 0x4
 */
void MenuStatus::State7(u32)
{
}

/**
 * @note Address: 0x80058760
 * @note Size: 0x4
 */
void MenuStatus::State6(u32)
{
}

/**
 * @note Address: 0x80062390
 * @note Size: 0x48
 */
void MenuCursor::UpdateObject(u32 a)
{
	if (mStatus.mFlags & 1) {
		UpdateObject__6CursorFUl(this, a);
	}
}

/**
 * @note Address: 0x800623D8
 * @note Size: 0x20
 */
void MenuCursor::ResetObjectState()
{
	ResetObjectState__4XObjFv(this);
}

/**
 * @note Address: 0x8003CD68
 * @note Size: 0xB0
 */
void MenuSave::State0(u32)
{
	MenuSave* menu = this;
	for (int i = 0; i < 3; i++) {
		ClearSaveSlot(&menu->mSlots[i]);
	}
	StartMemoryCardPreviewRead(&gGame.mMemcard.mCard, menu->mChannel, 1);
	menu->SetMode(1);
#if !defined(VERSION_JP)
#if !defined(VERSION_JP)
	menu->mFormatCancelled = 0;
#endif
#endif
}

/**
 * @note Address: 0x8004EDE8
 * @note Size: 0x24
 */
extern "C" char* GetPCMenuLabelText(s32 id)
{
#if defined(VERSION_JP)
	return GetLocalizedInterfaceText(id + 102);
#else
	return GetLocalizedInterfaceText(id + 45);
#endif
}

/**
 * @note Address: 0x80034E4C
 * @note Size: 0x60
 */
// NONMATCHING: regalloc
extern "C" s16 GetPlayerItemCount(s16 id)
{
	int byteOffset;
	const XObjChr* player = gGame.GetPlayer();
	s16 count             = 0;
	int i                 = 255;
	for (byteOffset = i * sizeof(InventoryItem); i >= 0; byteOffset -= sizeof(InventoryItem), --i) {
		const InventoryItem* entry
		    = reinterpret_cast<const InventoryItem*>(reinterpret_cast<const u8*>(&player->mParams.mInventory.mItems) + byteOffset);
		if (id == entry->mId) {
			count = gGame.GetPlayer()->mParams.mInventory.mItems[i].mCount;
			break;
		}
	}
	return count;
}

/**
 * @note Address: 0x80034248
 * @note Size: 0x360
 */
extern "C" void MarkAvailableStatusEntries(u8* available)
{
	available[0] = 1;
	available[1] = 1;
	available[2] = 1;
	available[3] = 1;
	available[4] = 1;
	if (GetPlayerItemCount(0x35) > 0)
		available[5] = 1;
	if (GetPlayerItemCount(0x36) > 0)
		available[6] = 1;
	if (GetPlayerItemCount(0x3E) > 0)
		available[7] = 1;
	if (GetPlayerItemCount(0x9E) > 0)
		available[8] = 1;
	if (GetPlayerItemCount(8) > 0 || (gEventFlags.mFlags[0] & 0x20000000))
		available[9] = 1;
	if (GetPlayerItemCount(0x30) > 0 || (gEventFlags.mFlags[0] & 0x40000000))
		available[10] = 1;
	if (GetPlayerItemCount(0x6D) > 0 || (gEventFlags.mFlags[0] & 0x10000000))
		available[11] = 1;
	if (gEventFlags.mFlags[0x84 / 4] & 0x01000000) {
		if (GetPlayerItemCount(0x7C) > 0 || (gEventFlags.mFlags[0x54 / 4] & 0x01000000))
			available[12] = 1;
	}
}

/**
 * @note Address: 0x80035184
 * @note Size: 0x70
 */
extern "C" s32 IndexOfLargestOfThree(s64 a, s64 b, s64 c)
{
	if (a >= b) {
		if (a >= c) {
			return 0;
		}
		return 2;
	}
	if (b >= c) {
		return 1;
	}
	return 2;
}

/**
 * @note Address: 0x800345A8
 * @note Size: 0xC4
 */
extern "C" s32 CountShopCatalogEntries(const MenuPCPage*)
{
	s32 count = 0;
	for (int i = 0; i < 20; i++) {
		const MenuPCItem* entry = sShopCatalogEntries + i;
		s16 stock               = entry->mCount;
		if (entry->mItem >= 0 && stock >= 0)
			count++;
	}
	return count;
}

/**
 * @note Address: 0x800349A8
 * @note Size: 0xC4
 */
extern "C" s32 CountShopHistoryEntries(const MenuPCPage*)
{
	s32 count = 0;
	for (int i = 0; i < 20; i++) {
		const MenuPCItem* entry = sShopHistoryEntries + i;
		s16 stock               = entry->mCount;
		if (entry->mItem >= 0 && stock >= 0)
			count++;
	}
	return count;
}

/**
 * @note Address: 0x8003466C
 * @note Size: 0x33C
 */
// the shop's page of 9 from the catalog, and the items just before and after it
// NONMATCHING: regalloc
extern "C" void PopulateShopCatalogPage(MenuPCPage* list)
{
	s32 count;
	s32 n;
	s16 price;
	int i;
	for (i = 0; i < 20; i++) {
		list->mRows[i].mItem  = -1;
		list->mRows[i].mCount = -1;
		list->mRows[i].mPrice = -1;
		list->mIndex[i]       = -1;
	}
	count = 0;
	n     = 0;
	for (i = 0; i < 20; i++) {
		const MenuPCItem* entry = sShopCatalogEntries + i;
		s16 itemId              = entry->mItem;
		s16 stock               = entry->mCount;
		price                   = entry->mPrice;
		if (itemId >= 0 && stock >= 0) {
			if (count >= list->mStart && count < list->mStart + 9 && n < 9) {
				list->mRows[n].mItem  = itemId;
				list->mRows[n].mCount = stock;
				list->mRows[n].mPrice = price;
				list->mIndex[n]       = i;
				n++;
			}
			count++;
		}
	}
	list->mPrev[0].mItem  = -1;
	list->mPrev[0].mCount = -1;
	list->mPrev[0].mPrice = -1;
	list->mNext[0].mItem  = -1;
	list->mNext[0].mCount = -1;
	list->mNext[0].mPrice = -1;
	n                     = 0;
	count                 = 0;
	for (i = 0; i < 20; i++) {
		const MenuPCItem* entry = sShopCatalogEntries + i;
		s16 itemId              = entry->mItem;
		s16 stock               = entry->mCount;
		price                   = entry->mPrice;
		if (itemId >= 0 && stock >= 0) {
			if (count >= list->mStart + 9 && count < list->mStart + 10) {
				list->mNext[n].mItem  = itemId;
				list->mNext[n].mCount = stock;
				list->mNext[n].mPrice = price;
				if (++n >= 1) {
					break;
				}
			}
			count++;
		}
	}
	n     = 0;
	count = 0;
	for (i = 0; i < 20; i++) {
		const MenuPCItem* entry = sShopCatalogEntries + i;
		s16 itemId              = entry->mItem;
		s16 stock               = entry->mCount;
		price                   = entry->mPrice;
		if (itemId >= 0 && stock >= 0) {
			if (count >= list->mStart - 1 && count < list->mStart) {
				list->mPrev[n].mItem  = itemId;
				list->mPrev[n].mCount = stock;
				list->mPrev[n].mPrice = price;
				if (++n >= 1) {
					break;
				}
			}
			count++;
		}
	}
}

static inline void ClearShopHistoryRows(MenuPCPage* list)
{
	for (int i = 0; i < 20; i++) {
		list->mRows[i].mItem         = -1;
		list->mRows[i].mCount        = -1;
		list->mRows[i].mPrice        = -1;
		list->mRows[i].mIsAttachment = 0;
		list->mIndex[i]              = -1;
	}
}

/**
 * @note Address: 0x80034A6C
 * @note Size: 0x3E0
 */
// NONMATCHING: register allocation and instruction scheduling
extern "C" void PopulateShopHistoryPage(MenuPCPage* list)
{
	s32 count;
	s32 n;
	s16 price;
	ClearShopHistoryRows(list);
	count = 0;
	n     = 0;
	for (int i = 0; i < 20; i++) {
		const MenuPCItem* entry = sShopHistoryEntries + i;
		s16 itemId              = entry->mItem;
		s16 stock               = entry->mCount;
		price                   = entry->mPrice;
		if (itemId >= 0 && stock >= 0) {
			if (count >= list->mStart && count < list->mStart + 9 && n < 9) {
				list->mRows[n].mItem  = itemId;
				list->mRows[n].mCount = stock;
				list->mRows[n].mPrice = price;
				list->mIndex[n]       = i;
				if (entry->mIsAttachment)
					list->mRows[n].mIsAttachment = 1;
				n++;
			}
			count++;
		}
	}
	list->mPrev[0].mItem         = -1;
	list->mPrev[0].mCount        = -1;
	list->mPrev[0].mPrice        = -1;
	list->mPrev[0].mIsAttachment = 0;
	list->mNext[0].mItem         = -1;
	list->mNext[0].mCount        = -1;
	list->mNext[0].mPrice        = -1;
	list->mNext[0].mIsAttachment = 0;
	count                        = 0;
	n                            = 0;
	for (int i = 0; i < 20; i++) {
		const MenuPCItem* entry = sShopHistoryEntries + i;
		s16 itemId              = entry->mItem;
		s16 stock               = entry->mCount;
		price                   = entry->mPrice;
		if (itemId >= 0 && stock >= 0) {
			if (count >= list->mStart + 9 && count < list->mStart + 10) {
				list->mNext[n].mItem  = itemId;
				list->mNext[n].mCount = stock;
				list->mNext[n].mPrice = price;
				if (entry->mIsAttachment)
					list->mNext[n].mIsAttachment = 1;
				if (++n >= 1) {
					break;
				}
			}
			count++;
		}
	}
	count = 0;
	n     = 0;
	for (int i = 0; i < 20; i++) {
		const MenuPCItem* entry = sShopHistoryEntries + i;
		s16 itemId              = entry->mItem;
		s16 stock               = entry->mCount;
		price                   = entry->mPrice;
		if (itemId >= 0 && stock >= 0) {
			if (count >= list->mStart - 1 && count < list->mStart) {
				list->mPrev[n].mItem  = itemId;
				list->mPrev[n].mCount = stock;
				list->mPrev[n].mPrice = price;
				if (entry->mIsAttachment)
					list->mPrev[n].mIsAttachment = 1;
				if (++n >= 1) {
					break;
				}
			}
			count++;
		}
	}
}

/**
 * @note Address: 0x80034EAC
 * @note Size: 0x2D8
 */
// the status menu's page of 8 owned items, and the 4 before and after it
// NONMATCHING: regalloc
extern "C" void PopulateOwnedItemPage(MenuStatusTable* table)
{
	s32 count;
	s16 num;
	int i;
	InventoryItem* pairs;
	s16 id;
	s32 n;
	for (i = 0; i < 256; i++) {
		table->mItems[i].mItem          = -1;
		table->mItems[i].mCount         = -1;
		table->mItems[i].mStatus.mFlags = 0;
	}
	n     = 0;
	count = 0;
	for (i = 255; i >= 0; i--) {
		pairs                    = gGame.GetPlayer()->mParams.mInventory.mItems;
		const InventoryItem& row = pairs[i];
		id                       = row.mId;
		num                      = row.mCount;
		if (GetItemFlagsMasked(id, 4) && id >= 0 && num > 0) {
			if (count >= table->mPage * 4 && count < (table->mPage + 2) * 4) {
				table->mItems[n].mItem  = id;
				table->mItems[n].mCount = num;
				if (++n >= 8) {
					break;
				}
			}
			count++;
		}
	}
	n     = 0;
	count = 0;
	for (i = 0; i < 4; i++) {
		table->mPrev[i].mItem          = -1;
		table->mPrev[i].mCount         = -1;
		table->mPrev[i].mStatus.mFlags = 0;
		table->mNext[i].mItem          = -1;
		table->mNext[i].mCount         = -1;
		table->mNext[i].mStatus.mFlags = 0;
	}
	for (i = 255; i >= 0; i--) {
		pairs                    = gGame.GetPlayer()->mParams.mInventory.mItems;
		const InventoryItem& row = pairs[i];
		id                       = row.mId;
		num                      = row.mCount;
		if (GetItemFlagsMasked(id, 4) && id >= 0 && num > 0) {
			if (count >= (table->mPage + 2) * 4 && count < (table->mPage + 4) * 4) {
				table->mNext[n].mItem  = id;
				table->mNext[n].mCount = num;
				if (++n >= 4) {
					break;
				}
			}
			count++;
		}
	}
	n     = 0;
	count = 0;
	for (i = 255; i >= 0; i--) {
		pairs                    = gGame.GetPlayer()->mParams.mInventory.mItems;
		const InventoryItem& row = pairs[i];
		id                       = row.mId;
		num                      = row.mCount;
		if (GetItemFlagsMasked(id, 4) && id >= 0 && num > 0) {
			if (count >= (table->mPage - 1) * 4 && count < (table->mPage + 1) * 4) {
				table->mPrev[n].mItem  = id;
				table->mPrev[n].mCount = num;
				if (++n >= 4) {
					break;
				}
			}
			count++;
		}
	}
}

// how many kinds of things the player has
static inline s32 CountMenuItems()
{
	s16 num;
	s16 id;
	s32 count;
	int i;
	InventoryItem* pairs;
	count = 0;
	for (i = 255; i >= 0; i--) {
		pairs = gGame.GetPlayer()->mParams.mInventory.mItems;
		id    = pairs[i].mId;
		num   = pairs[i].mCount;
		if (GetItemFlagsMasked(id, 4) && id >= 0 && num > 0) {
			count++;
		}
	}
	return count;
}

static inline void SelectDiscardQuantity(MenuStatus* menu, CPad& pad)
{
	s32 num = sStatusItemTable.mItems[menu->mItem].mCount;
	if (num >= 1) {
		if (pad.mPad->mRepeat & 0x40001) {
			if (--menu->mCount < 1) {
				menu->mCount = num;
			}
			PlayGameSound(&gGame.mSound, 0x87, 0xFF);
		} else if (pad.mPad->mRepeat & 0x80002) {
			if (++menu->mCount > num) {
				menu->mCount = 1;
			}
			PlayGameSound(&gGame.mSound, 0x87, 0xFF);
		}
		if (pad.mPad->mTrigger & 0x100) {
			PlayGameSound(&gGame.mSound, 0x88, 0xFF);
			menu->mPhase++;
		} else if (pad.mPad->mTrigger & 0x200) {
			PlayGameSound(&gGame.mSound, 0x86, 0xFF);
			menu->mTargetItemPanelX    = 725.0f;
			menu->mTargetDiscardPanelX = 937.0f;
			menu->mPhase               = 0;
		}
	}
}

static inline void ConfirmDiscardQuantity(MenuStatus* menu, CPad& pad, bool inventoryTab)
{
	if (pad.mPad->mRepeat & 0x40001) {
		if (menu->mChoice > 0) {
			menu->mChoice--;
			PlayGameSound(&gGame.mSound, 0x87, 0xFF);
		}
	} else if (pad.mPad->mRepeat & 0x80002) {
		if (menu->mChoice < 1) {
			menu->mChoice++;
			PlayGameSound(&gGame.mSound, 0x87, 0xFF);
		}
	}
	if (pad.mPad->mTrigger & 0x200) {
		PlayGameSound(&gGame.mSound, 0x86, 0xFF);
		menu->mPhase = 1;
	} else if (pad.mPad->mTrigger & 0x100) {
		if (menu->mChoice == 0) {
			MenuStatusEntry* entry = &sStatusItemTable.mItems[menu->mItem];
			RemoveInventoryItemCount(&gGame.GetPlayer()->mParams.mInventory, entry->mItem, menu->mCount);
			menu->mTargetItemPanelX    = 725.0f;
			menu->mTargetDiscardPanelX = 937.0f;
			menu->mPhase               = 0;
			PlayGameSound(&gGame.mSound, 0x88, 0xFF);
			menu->mItem = 0;
			PopulateOwnedItemPage(&sStatusItemTable);
			menu->mTargetX = 360.0f - (CountMenuItems() - 1) * 54;
			if (CountMenuItems() == 0) {
				menu->mTargetX = 360.0f;
			}
			if (inventoryTab)
				menu->mTargetX = 207.0f;
		} else {
			PlayGameSound(&gGame.mSound, 0x86, 0xFF);
			menu->mPhase = 1;
		}
	}
}

/**
 * @note Address: 0x80058C7C
 * @note Size: 0x6AC
 */
// throwing things away: pick the thing, then how many, then confirm
// NONMATCHING: regalloc
void MenuStatus::State3(u32)
{
	MenuStatus* menu = this;
	CPad pad(0);
	s32 count = CountMenuItems();
	switch (menu->mPhase) {
	case 0:
		if (count > 1) {
			if (pad.mPad->mRepeat & 0x40001) {
				if (--menu->mItem < 0) {
					menu->mItem = count - 1;
				}
				PlayGameSound(&gGame.mSound, 0x87, 0xFF);
			} else if (pad.mPad->mRepeat & 0x80002) {
				if (++menu->mItem > count - 1) {
					menu->mItem = 0;
				}
				PlayGameSound(&gGame.mSound, 0x87, 0xFF);
			}
		}
		if (pad.mPad->mTrigger & 0x200) {
			menu->mPrevMode = menu->mMode;
			menu->mMode     = 4;
			menu->mNextMode = 2;
			menu->mPhase    = 0;
			PlayGameSound(&gGame.mSound, 0x86, 0xFF);
			for (int i = 0; i < 4; i++) {
				menu->mTabs2[i].mX = 0.0f;
				if (menu->mHasFourthTab) {
					menu->mTabs2[i].mTargetX = 198.0f + i * 108 - 8.0f;
				} else {
					menu->mTabs2[i].mTargetX = 252.0f + i * 108;
				}
			}
			menu->mTargetX = 758.0f;
		} else if ((pad.mPad->mTrigger & 0x100) && sStatusItemTable.mItems[menu->mItem].mCount >= 1) {
			menu->mTargetItemPanelX    = 336.0f;
			menu->mTargetDiscardPanelX = 336.0f;
			menu->mCount               = 1;
			menu->mPhase++;
		}
		break;
	case 1:
		SelectDiscardQuantity(menu, pad);
		break;
	case 2:
		ConfirmDiscardQuantity(menu, pad, false);
		break;
	}
}

/**
 * @note Address: 0x80058BBC
 * @note Size: 0xC0
 */
void MenuStatus::State4(u32 frameCount)
{
	MenuStatus* menu = this;
	s32 wait         = (s32)frameCount;
	CPad pad(0);
	switch (menu->mPhase) {
	case 0:
		menu->mTargetPanelY = -200.0f;
		menu->mDelay        = wait * 4;
		menu->mPhase++;
	case 1:
		if (--menu->mDelay < 0) {
			if (menu->mNextMode != 5) {
				menu->mTargetPanelY = 0.0f;
			}
			menu->SetMode(menu->mNextMode);
		}
		break;
	}
}

/**
 * @note Address: 0x8005A290
 * @note Size: 0x110C
 */
// the status menu's tabs: pick one and open its page, or close the menu
// NONMATCHING: regalloc
void MenuStatus::State1(u32 frames)
{
	MenuStatus* menu = this;
	CPad pad(0);
	switch (menu->mPhase) {
	case 0:
		if (pad.mPad->mRepeat & 0x40001) {
			if (--menu->mSelection < 0) {
				if (menu->mHasFourthTab) {
					menu->mSelection = 3;
				} else {
					menu->mSelection = 2;
				}
			}
			char* msg = GetLocalizedInterfaceText((menu->mSelection >= 4 ? 0 : gStatusTabTextOffsets[menu->mSelection]) + StatusTitleText);
			menu->mShowHelp = 1;
			menu->mHelp     = msg;
			PlayGameSound(&gGame.mSound, 0x87, 0xFF);
		} else if (pad.mPad->mRepeat & 0x80002) {
			if (menu->mHasFourthTab) {
				if (++menu->mSelection > 3) {
					menu->mSelection = 0;
				}
			} else if (++menu->mSelection > 2) {
				menu->mSelection = 0;
			}
			char* msg = GetLocalizedInterfaceText((menu->mSelection >= 4 ? 0 : gStatusTabTextOffsets[menu->mSelection]) + StatusTitleText);
			menu->mShowHelp = 1;
			menu->mHelp     = msg;
			PlayGameSound(&gGame.mSound, 0x87, 0xFF);
		}
		if (pad.mPad->mTrigger & 0x1200) {
			menu->mTargetPanelY = -200.0f;
			for (int i = 0; i < 4; i++) {
				menu->mTabs[i].mTargetX = -102.0f;
			}
			PlayGameSound(&gGame.mSound, 0x86, 0xFF);
			menu->mPhase++;
			menu->mDelay = frames * 4;
			for (int i = 0; i < 20; i++) {
				menu->mTitle.mChars[i].mHold = 0.0f;
			}
		} else if (pad.mPad->mTrigger & 0x100) {
			switch (menu->mSelection) {
			case 0: {
				menu->mPrevMode = menu->mMode;
				menu->mMode     = 4;
				menu->mNextMode = 2;
				menu->mPhase    = 0;
				PlayGameSound(&gGame.mSound, 0x88, 0xFF);
				for (int i = 0; i < 4; i++) {
					menu->mTabs[i].mTargetX = -102.0f;
				}
				for (int i = 0; i < 4; i++) {
					menu->mTabs2[i].mX = 758.0f;
					if (menu->mHasFourthTab) {
						menu->mTabs2[i].mTargetX = 198.0f + i * 108 - 8.0f;
					} else {
						menu->mTabs2[i].mTargetX = 252.0f + i * 108;
					}
				}
				s32 count = CountMenuItems();
				for (int i = 0; i < 4; i++) {
					menu->mTabs2[i].mTargetX = 0.0f;
				}
				menu->mTargetX = 360.0f - (count - 1) * 51;
				if (count == 0) {
					menu->mTargetX = 360.0f;
				}
				menu->mTargetX = 207.0f;
				u32 color      = (u32)(255.0f * menu->mAlpha) | 0xFFFFFF00;
				menu->mTitle.Set(GetLocalizedInterfaceText(StatusTitleText + 1), color);
				s16 id              = sStatusItemTable.mItems[menu->mItem].mItem;
				menu->mPreviousItem = menu->mItem;
				if (id >= 0 && CountMenuItems() > 0) {
					char* msg       = GetItemName(id);
					menu->mShowHelp = 1;
					menu->mHelp     = msg;
				} else {
					menu->mShowHelp = 1;
					menu->mHelp     = NULL;
				}
				break;
			}
			case 1: {
				menu->mTargetOverviewX = 360.0f;
				menu->mPrevMode        = menu->mMode;
				menu->mMode            = 4;
				menu->mNextMode        = 9;
				menu->mPhase           = 0;
				PlayGameSound(&gGame.mSound, 0x88, 0xFF);
				for (int i = 0; i < 4; i++) {
					menu->mTabs[i].mTargetX = -102.0f;
				}
				u32 color = (u32)(255.0f * menu->mAlpha) | 0xFFFFFF00;
				menu->mTitle.Set(GetLocalizedInterfaceText(StatusTitleText + 3), color);
				if (gStatusEntryAvailable[menu->mOverviewSelection]) {
					char* msg       = GetStatusOverviewLabel(menu->mOverviewSelection);
					menu->mShowHelp = 1;
					menu->mHelp     = msg;
				} else {
					char* msg       = GetLocalizedInterfaceText(StatusEmptyText);
					menu->mShowHelp = 1;
					menu->mHelp     = msg;
				}
				break;
			}
			case 2: {
				menu->mTargetDiagramX = 360.0f;
				menu->mPrevMode       = menu->mMode;
				menu->mMode           = 4;
				menu->mNextMode       = 5;
				menu->mPhase          = 0;
				PlayGameSound(&gGame.mSound, 0x88, 0xFF);
				for (int i = 0; i < 4; i++) {
					menu->mTabs[i].mTargetX = -102.0f;
				}
				u32 color = (u32)(255.0f * menu->mAlpha) | 0xFFFFFF00;
				menu->mTitle.Set(GetLocalizedInterfaceText(StatusTitleText + 2), color);
				menu->mShowHelp = 1;
				menu->mHelp     = NULL;
				break;
			}
			case 3: {
				menu->mTargetGridX = 360.0f;
				menu->mPrevMode    = menu->mMode;
				menu->mMode        = 4;
				menu->mNextMode    = 8;
				menu->mPhase       = 0;
				PlayGameSound(&gGame.mSound, 0x88, 0xFF);
				for (int i = 0; i < 4; i++) {
					menu->mTabs[i].mTargetX = -102.0f;
				}
				u32 color = (u32)(255.0f * menu->mAlpha) | 0xFFFFFF00;
				menu->mTitle.Set(GetLocalizedInterfaceText(StatusTitleText + 4), color);
				if (gStatusIconUnlocked[menu->mIconSelection]) {
					char* msg       = GetStatusIconName(menu->mIconSelection);
					menu->mShowHelp = 1;
					menu->mHelp     = msg;
				} else {
					char* msg       = GetLocalizedInterfaceText(StatusEmptyText);
					menu->mShowHelp = 1;
					menu->mHelp     = msg;
				}
				break;
			}
			default:
				PlayGameSound(&gGame.mSound, 0x147, 0xFF);
				break;
			}
		}
		break;
	case 2:
		break;
	case 1:
		if (--menu->mDelay < 0) {
			gGame.mStatus.mFlags &= ~2ULL;
			menu->Close();
			sMenuMain.Resume();
			StopGameSfx(&gGame.mSound, 0xFFFF0004);
			if (menu->mSavedBgm) {
				PlayGameSound(&gGame.mSound, menu->mSavedBgm, menu->mSavedBgmVolume);
			}
			menu->mPhase++;
		}
		break;
	}
}

/**
 * @note Address: 0x8005B39C
 * @note Size: 0x9C
 */
void MenuStatus::State0(u32)
{
	MenuStatus* menu      = this;
	menu->mSavedBgm       = gGame.mSound.mMusicFadeOutFrames <= 0 ? gGame.mSound.mCurrentMusicId : 0;
	menu->mSavedBgmVolume = gGame.mSound.mMusicVolume;
	PlayGameSound(&gGame.mSound, 0xFFFF0004, 0xFF);
	menu->mPrevMode = menu->mMode;
	menu->mMode     = 4;
	menu->mNextMode = 1;
	menu->mPhase    = 0;
}

/**
 * @note Address: 0x8003B9B8
 * @note Size: 0x160
 */
void MenuSave::State9(u32 frameCount)
{
	MenuSave* menu = this;
	s32 frames     = (s32)frameCount;
	menu->mDelay -= frames;
	if (menu->mDelay < 0) {
		menu->mDelay = 0;
	}
	switch (gGame.mMemcard.mCard.mState) {
	case 0:
		menu->mFreeBlocks = gGame.mMemcard.mCard.mFreeBlocks;
		menu->mFreeFiles  = gGame.mMemcard.mCard.mFreeFiles;
		menu->SetMode(10);
		break;
	case 1:
	case 2:
	case 3:
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
	case 11:
	case 12:
	case 13:
	case 14:
	case 15:
		s32 result    = gGame.mMemcard.mCard.mCard.mResult;
		menu->mResult = result;
		switch (result) {
		case 0:
		case -1:
		case -2:
		case -3:
		case -4:
		case -5:
		case -6:
		case -7:
		case -8:
		case -9:
		case -10:
		case -11:
		case -12:
		case -13:
		case -14:
		case -128:
			if (result == -6 || result == -13) {
				menu->SetMode(13);
				menu->mShowMessage = TRUE;
			} else {
				menu->SetMode(15);
				menu->mMessage     = 8;
				menu->mShowMessage = TRUE;
			}
			break;
		default:
			menu->SetMode(11);
			menu->mOffset = -32.0f;
			break;
		}
		break;
	default:
		menu->SetMode(11);
		menu->mOffset = -32.0f;
		break;
	}
}

/**
 * @note Address: 0x8003CA2C
 * @note Size: 0x33C
 */
// waits for the card to list its files, then fills in the three file headers
// NONMATCHING: regalloc
void MenuSave::State1(u32 frameCount)
{
	MenuSave* menu = this;
	s32 frames     = (s32)frameCount;
	SavePreviewRecord* save;
	MenuSaveSlot* header;
	s32 state;
	Memcard* card = &gGame.mMemcard.mCard;
	state         = card->mState;
	menu->mDelay -= frames;
	if (menu->mDelay < 0) {
		menu->mDelay = 0;
	}
	switch (state) {
	case 0:
		for (int i = 0; i < 3; i++) {
			save = &card->mSaves[0][i];
			if (save->mState != 0) {
				header = &menu->mSlots[i];
				OSCalendarTime cal;
				OSTicksToCalendarTime((OSTime)save->mStat.time * OS_TIMER_CLOCK, &cal);
				s32 stage           = save->mHeader.mStage;
				s32 level           = save->mHeader.mLevel;
				s32 exp             = save->mHeader.mExp;
				f32 battery         = save->mHeader.mBattery;
				f32 maxBattery      = save->mHeader.mMaxBattery;
				s32 money           = save->mHeader.mMoney;
				s32 scrap           = save->mHeader.mScrap;
				s32 rank            = save->mHeader.mRank;
				s32 hours           = save->mHeader.mFrames / 216000;
				s32 minutes         = (save->mHeader.mFrames / 3600) % 60;
				s32 timeMode        = save->mHeader.mTimeMode;
				header->mUsed       = 1;
				header->mLevel      = level;
				header->mBattery    = battery;
				header->mMaxBattery = maxBattery;
				header->mMoney      = money;
				header->mExp        = exp;
				header->mScrap      = scrap;
				header->mRank       = rank;
				header->mHours      = hours;
				header->mMinutes    = minutes;
				header->mStage      = stage;
				header->mTimeMode   = timeMode;
				if (save->mHeader.mActive > 0) {
					header->mActive = 1;
				} else {
					header->mActive = 0;
				}
				OSCalendarTimeToTicks(&cal);
				if (save->mStatus != 0 || !IsValidSavePreview(save)) {
					header->mState = 1;
				} else {
					header->mState = 0;
				}
			}
		}
		menu->mFreeBlocks = gGame.mMemcard.mCard.mFreeBlocks;
		menu->mFreeFiles  = gGame.mMemcard.mCard.mFreeFiles;
		if (menu->mSaved == TRUE) {
			menu->SetMode(4);
			menu->mTime  = 0.5f * FRAME_RATE;
			menu->mSaved = 0;
		} else {
			menu->SetMode(2);
		}
		break;
	case 1:
	case 2:
	case 3:
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
	case 11:
	case 12:
	case 13:
	case 14:
	case 15:
		s32 result    = gGame.mMemcard.mCard.mCard.mResult;
		menu->mResult = result;
		switch (result) {
		case 0:
		case -1:
		case -2:
		case -3:
		case -4:
		case -5:
		case -6:
		case -7:
		case -8:
		case -9:
		case -10:
		case -11:
		case -12:
		case -13:
		case -14:
		case -128:
			if (result == -6 || result == -13) {
				menu->SetMode(8);
				menu->mChoice = 1;
			} else if (result == -5) {
				menu->SetMode(13);
				menu->mShowMessage = TRUE;
			} else {
				menu->SetMode(11);
				menu->mOffset = -32.0f;
			}
			menu->mTargetAlpha = 1.0f;
			break;
		default:
			menu->SetMode(11);
			menu->mTargetAlpha = 1.0f;
			menu->mOffset      = -32.0f;
			break;
		}
		break;
	}
}

/**
 * @note Address: 0x8003C0A8
 * @note Size: 0x3C0
 */
// waits for the card to save, then reads back the header of the file it wrote
void MenuSave::State5(u32 frameCount)
{
	MenuSave* menu = this;
	s32 frames     = (s32)frameCount;
	SavePreviewRecord* save;
	Memcard* card = &gGame.mMemcard.mCard;
	s32 state     = card->mState;
	menu->mSaved  = 0;
	menu->mDelay -= frames;
	if (menu->mDelay < 0) {
		menu->mDelay = 0;
	}
	switch (state) {
	case 0: {
		save = &card->mSaves[0][menu->mFile];
		if (save->mState != 0) {
			OSCalendarTime cal;
			OSTicksToCalendarTime((OSTime)save->mStat.time * OS_TIMER_CLOCK, &cal);
			s32 stage                             = save->mHeader.mStage;
			s32 exp                               = save->mHeader.mExp;
			s32 level                             = save->mHeader.mLevel;
			f32 battery                           = save->mHeader.mBattery;
			f32 maxBattery                        = save->mHeader.mMaxBattery;
			s32 money                             = save->mHeader.mMoney;
			s32 scrap                             = save->mHeader.mScrap;
			s32 rank                              = save->mHeader.mRank;
			s32 hours                             = save->mHeader.mFrames / 216000;
			s32 minutes                           = (save->mHeader.mFrames / 3600) % 60;
			s32 timeMode                          = save->mHeader.mTimeMode;
			menu->mSlots[menu->mFile].mUsed       = 1;
			menu->mSlots[menu->mFile].mLevel      = level;
			menu->mSlots[menu->mFile].mBattery    = battery;
			menu->mSlots[menu->mFile].mMaxBattery = maxBattery;
			menu->mSlots[menu->mFile].mMoney      = money;
			menu->mSlots[menu->mFile].mExp        = exp;
			menu->mSlots[menu->mFile].mScrap      = scrap;
			menu->mSlots[menu->mFile].mRank       = rank;
			menu->mSlots[menu->mFile].mHours      = hours;
			menu->mSlots[menu->mFile].mMinutes    = minutes;
			menu->mSlots[menu->mFile].mTimeMode   = timeMode;
			menu->mSlots[menu->mFile].mStage      = stage;
			menu->mSlots[menu->mFile].mState      = 0;
			if (save->mHeader.mActive > 0) {
				menu->mSlots[menu->mFile].mActive = 1;
			} else {
				menu->mSlots[menu->mFile].mActive = 0;
			}
		}
		menu->mFreeBlocks = gGame.mMemcard.mCard.mFreeBlocks;
		menu->mFreeFiles  = gGame.mMemcard.mCard.mFreeFiles;
		StartMemoryCardPreviewRead(card, menu->mChannel, 1);
		menu->SetMode(1);
		menu->mLastFile = menu->mFile;
		menu->mSaved    = 1;
		menu->mOffset   = 0.0f;
		break;
	}
	case 1:
	case 2:
	case 3:
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
	case 11:
	case 12:
	case 13:
	case 14:
	case 15:
		s32 result    = gGame.mMemcard.mCard.mCard.mResult;
		menu->mResult = result;
		switch (result) {
		case 0:
		case -1:
		case -2:
		case -3:
		case -4:
		case -5:
		case -6:
		case -7:
		case -8:
		case -9:
		case -10:
		case -11:
		case -12:
		case -13:
		case -14:
		case -128:
			if (state == 2 && (result == -6 || result == -13)) {
				menu->SetMode(8);
				menu->mChoice      = 1;
				menu->mShowMessage = TRUE;
			} else if (result == -3 || result == -5) {
				menu->SetMode(14);
				menu->mMessage     = 3;
				menu->mShowMessage = TRUE;
			} else {
				menu->SetMode(11);
				menu->mOffset = -32.0f;
			}
			menu->mTargetAlpha = 1.0f;
			break;
		default:
			menu->SetMode(11);
			menu->mTargetAlpha = 1.0f;
			menu->mOffset      = -32.0f;
			break;
		}
		break;
	}
}

/**
 * @note Address: 0x8003C468
 * @note Size: 0x130
 */
void MenuSave::State4(u32 frames)
{
	MenuSave* menu = this;
	CPad pad(0);
	switch (menu->mPhase) {
	case 0:
		menu->mDelay -= frames;
		if (menu->mDelay < 0) {
			menu->mDelay = 0;
			menu->mPhase++;
			PlayGameSound(&gGame.mSound, 0x88, 0xFF);
		}
		break;
	case 1:
		if ((pad.mPad->mTrigger & 0x100) || (menu->mTime -= frames) < 0.0f) {
			menu->mTime = 0.0f;
			menu->SetMode(16);
			PlayGameSound(&gGame.mSound, 0x88, 0xFF);
		}
		break;
	}
}

/**
 * @note Address: 0x8003B05C
 * @note Size: 0x1F8
 */
void MenuSave::State13(u32)
{
	MenuSave* menu = this;
	CPad pad(0);
	if (menu->mResult == -3) {
		if (menu->mReady == TRUE) {
			for (int i = 0; i < 3; i++) {
				ClearSaveSlot(&menu->mSlots[i]);
			}
			StartMemoryCardPreviewRead(&gGame.mMemcard.mCard, menu->mChannel, 1);
			menu->SetMode(1);
			PlayGameSound(&gGame.mSound, 0x88, 0xFF);
#if !defined(VERSION_JP)
#if !defined(VERSION_JP)
			menu->mFormatCancelled = 0;
#endif
#endif
			return;
		}
	} else if (!menu->mReady) {
		menu->ResetResults();
#if !defined(VERSION_JP)
#if !defined(VERSION_JP)
		menu->mFormatCancelled = 0;
#endif
#endif
		return;
	}
	if (pad.mPad->mTrigger & 0x100) {
		menu->SetMode(16);
		PlayGameSound(&gGame.mSound, 0x88, 0xFF);
#if !defined(VERSION_JP)
#if !defined(VERSION_JP)
		menu->mFormatCancelled = 0;
#endif
#endif
	}
}

/**
 * @note Address: 0x8003B254
 * @note Size: 0x1B8
 */
void MenuSave::State12(u32)
{
	MenuSave* menu = this;
	CPad pad(0);
	if (menu->mResult == -2) {
		if (!menu->mWrongDevice) {
			menu->ResetResults();
			return;
		}
	} else if (!menu->mReady) {
		menu->ResetResults();
		return;
	}
	if (pad.mPad->mTrigger & 0x100) {
		menu->SetMode(16);
		PlayGameSound(&gGame.mSound, 0x88, 0xFF);
	}
}

/**
 * @note Address: 0x8003B7AC
 * @note Size: 0x20C
 */
void MenuSave::State10(u32 frames)
{
	MenuSave* menu = this;
	CPad pad(0);
	if (!menu->mReady) {
		menu->ResetResults();
		return;
	}
	switch (menu->mPhase) {
	case 0:
		menu->mDelay -= frames;
		if (menu->mDelay < 0) {
			menu->mDelay = 0;
			menu->mPhase++;
			PlayGameSound(&gGame.mSound, 0x88, 0xFF);
		}
		break;
	case 1:
		if (pad.mPad->mTrigger & 0x100) {
			for (int i = 0; i < 3; i++) {
				ClearSaveSlot(&menu->mSlots[i]);
			}
			StartMemoryCardPreviewRead(&gGame.mMemcard.mCard, menu->mChannel, 1);
			menu->SetMode(1);
			PlayGameSound(&gGame.mSound, 0x88, 0xFF);
			return;
		}
		break;
	}
}

/**
 * @note Address: 0x8003C598
 * @note Size: 0x200
 */
void MenuSave::State3(u32)
{
	MenuSave* menu = this;
	if (!menu->mReady) {
		menu->ResetResults();
		return;
	}
	CPad pad(0);
	if (pad.mPad->mRepeat & 0x40001) {
		if (menu->mChoice > 0) {
			menu->mChoice--;
			PlayGameSound(&gGame.mSound, 0x87, 0xFF);
		}
	} else if (pad.mPad->mRepeat & 0x80002) {
		if (menu->mChoice < 1) {
			menu->mChoice++;
			PlayGameSound(&gGame.mSound, 0x87, 0xFF);
		}
	}
	if (pad.mPad->mTrigger & 0x100) {
		switch (menu->mChoice) {
		case 0:
			StartMemoryCardSaveWrite(&gGame.mMemcard.mCard, menu->mChannel, menu->mFile, 1);
			menu->SetMode(5);
			menu->mOffset = -32.0f;
			menu->mDelay  = 1.5f * FRAME_RATE;
			break;
		case 1:
			menu->SetMode(2);
			break;
		}
		PlayGameSound(&gGame.mSound, 0x88, 0xFF);
	}
}

/**
 * @note Address: 0x8003ADC8
 * @note Size: 0x294
 */
void MenuSave::State14(u32)
{
	MenuSave* menu = this;
	CPad pad(0);
	if (menu->mResult == -3) {
		if (menu->mReady == TRUE) {
			for (int i = 0; i < 3; i++) {
				ClearSaveSlot(&menu->mSlots[i]);
			}
			StartMemoryCardPreviewRead(&gGame.mMemcard.mCard, menu->mChannel, 1);
			menu->SetMode(1);
			PlayGameSound(&gGame.mSound, 0x88, 0xFF);
			return;
		}
	} else if (!menu->mReady) {
		menu->ResetResults();
		return;
	}
	if (pad.mPad->mTrigger & 0x100) {
		if (menu->mResult == -3) {
			menu->ResetResults();
		} else if (menu->mResult == -5) {
			menu->SetMode(13);
			menu->mShowMessage = TRUE;
		} else {
			menu->SetMode(13);
		}
		PlayGameSound(&gGame.mSound, 0x88, 0xFF);
	}
}

/**
 * @note Address: 0x8003BE94
 * @note Size: 0x214
 */
void MenuSave::State6(u32)
{
	MenuSave* menu = this;
	CPad pad(0);
	if (menu->mWrongDevice == TRUE) {
		for (int i = 0; i < 3; i++) {
			ClearSaveSlot(&menu->mSlots[i]);
		}
		StartMemoryCardPreviewRead(&gGame.mMemcard.mCard, menu->mChannel, 1);
		menu->SetMode(1);
		PlayGameSound(&gGame.mSound, 0x88, 0xFF);
		return;
	}
	if (menu->mReady == TRUE) {
		for (int i = 0; i < 3; i++) {
			ClearSaveSlot(&menu->mSlots[i]);
		}
		StartMemoryCardPreviewRead(&gGame.mMemcard.mCard, menu->mChannel, 1);
		menu->SetMode(1);
		PlayGameSound(&gGame.mSound, 0x88, 0xFF);
		return;
	}
	if (pad.mPad->mTrigger & 0x100) {
		menu->SetMode(16);
		PlayGameSound(&gGame.mSound, 0x86, 0xFF);
		return;
	}
}

/**
 * @note Address: 0x8003C798
 * @note Size: 0x294
 */
void MenuSave::State2(u32)
{
	MenuSave* menu = this;
	if (!menu->mReady) {
		menu->ResetResults();
		return;
	}
	CPad pad(0);
	if (pad.mPad->mRepeat & 0x40001) {
		menu->mFile--;
		if (menu->mFile < 0) {
			menu->mFile = 2;
		}
		PlayGameSound(&gGame.mSound, 0x87, 0xFF);
	} else if (pad.mPad->mRepeat & 0x80002) {
		menu->mFile++;
		if (menu->mFile > 2) {
			menu->mFile = 0;
		}
		PlayGameSound(&gGame.mSound, 0x87, 0xFF);
	}
	if (pad.mPad->mTrigger & 0x100) {
		if (menu->mSlots[menu->mFile].mUsed == 1) {
			menu->mChoice = 1;
			menu->SetMode(3);
		} else if (menu->mFreeBlocks >= 6 && menu->mFreeFiles > 0) {
			StartMemoryCardSaveWrite(&gGame.mMemcard.mCard, menu->mChannel, menu->mFile, 1);
			menu->SetMode(5);
			menu->mDelay = 1.5f * FRAME_RATE;
		} else {
			menu->mResult = -9;
			menu->SetMode(11);
			menu->mOffset = -32.0f;
		}
		PlayGameSound(&gGame.mSound, 0x88, 0xFF);
	} else if (pad.mPad->mTrigger & 0x200) {
		menu->SetMode(16);
		PlayGameSound(&gGame.mSound, 0x86, 0xFF);
	}
}

/**
 * @note Address: 0x8003BB18
 * @note Size: 0x378
 */
void MenuSave::State8(u32)
{
	MenuSave* menu = this;
	CPad pad(0);
	if (!menu->mReady) {
		menu->ResetResults();
		return;
	}
	switch (menu->mPhase) {
	case 0:
		if (pad.mPad->mRepeat & 0x40001) {
			if (menu->mChoice > 0) {
				menu->mChoice--;
				PlayGameSound(&gGame.mSound, 0x87, 0xFF);
			}
		} else if (pad.mPad->mRepeat & 0x80002) {
			if (menu->mChoice < 1) {
				menu->mChoice++;
				PlayGameSound(&gGame.mSound, 0x87, 0xFF);
			}
		}
		if (pad.mPad->mTrigger & 0x100) {
			if (menu->mChoice == 0) {
				PlayGameSound(&gGame.mSound, 0x88, 0xFF);
				menu->mChoice = 1;
				menu->mPhase++;
			} else {
				menu->SetMode(13);
				PlayGameSound(&gGame.mSound, 0x88, 0xFF);
#if !defined(VERSION_JP)
#if !defined(VERSION_JP)
				menu->mFormatCancelled = 1;
#endif
#endif
			}
			return;
		}
		return;
	case 1:
		if (pad.mPad->mRepeat & 0x40001) {
			if (menu->mChoice > 0) {
				menu->mChoice--;
				PlayGameSound(&gGame.mSound, 0x87, 0xFF);
			}
		} else if (pad.mPad->mRepeat & 0x80002) {
			if (menu->mChoice < 1) {
				menu->mChoice++;
				PlayGameSound(&gGame.mSound, 0x87, 0xFF);
			}
		}
		if (pad.mPad->mTrigger & 0x100) {
			if (menu->mChoice == 0) {
				StartMemoryCardFormatCheck(&gGame.mMemcard.mCard, menu->mChannel, 1);
				menu->SetMode(9);
				menu->mDelay = 1.5f * FRAME_RATE;
				PlayGameSound(&gGame.mSound, 0x88, 0xFF);
			} else {
				menu->SetMode(13);
				PlayGameSound(&gGame.mSound, 0x88, 0xFF);
#if !defined(VERSION_JP)
#if !defined(VERSION_JP)
				menu->mFormatCancelled = 1;
#endif
#endif
			}
			return;
		}
		break;
	}
}

/**
 * @note Address: 0x8003B40C
 * @note Size: 0x3A0
 */
void MenuSave::State11(u32)
{
	MenuSave* menu = this;
	CPad pad(0);
	if (menu->mResult == -3) {
		if (menu->mWrongDevice == TRUE) {
			for (int i = 0; i < 3; i++) {
				ClearSaveSlot(&menu->mSlots[i]);
			}
			StartMemoryCardPreviewRead(&gGame.mMemcard.mCard, menu->mChannel, 1);
			menu->SetMode(1);
			PlayGameSound(&gGame.mSound, 0x88, 0xFF);
			menu->mOffset = 0.0f;
			return;
		}
		if (menu->mReady == TRUE) {
			for (int i = 0; i < 3; i++) {
				ClearSaveSlot(&menu->mSlots[i]);
			}
			StartMemoryCardPreviewRead(&gGame.mMemcard.mCard, menu->mChannel, 1);
			menu->SetMode(1);
			PlayGameSound(&gGame.mSound, 0x88, 0xFF);
			menu->mOffset = 0.0f;
			return;
		}
	} else if (menu->mResult == -2) {
		if (!menu->mWrongDevice) {
			menu->ResetResults();
			return;
		}
	} else if (!menu->mReady) {
		menu->ResetResults();
		return;
	}
	if (pad.mPad->mTrigger & 0x100) {
		if (menu->mResult == -9) {
			menu->SetMode(12);
			menu->mMessage = 2;
			PlayGameSound(&gGame.mSound, 0x88, 0xFF);
			menu->mOffset = 0.0f;
		} else {
			menu->SetMode(16);
			PlayGameSound(&gGame.mSound, 0x88, 0xFF);
		}
	}
}

// the letters of a line flying in, each with a blob behind it and a dark outline
/**
 * @note Address: 0x800373D4
 * @note Size: 0x5A4
 */
// NONMATCHING: regalloc
extern "C" void DrawSaveMenuTitle(MenuSave* menu)
{
	if (menu->mTitle.mActive) {
		SetGsBlendMode(gGame.mGs.mCurRenderMode, 1, 4, 5, 15);
		SetGsChannelEnabled(&gGame.mGs, 0, 0);
		SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 7, 0);
		SetGsTevMode(gGame.mGs.mCurTevStage, 0);
		CTexObj blob = (&gGame.mTpl)->mTex.Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[MenuTitleBlobTexture], 0);
		u32 alpha;
		f32 t;
		f32 s0;
		f32 bob;
		f32 rot;
		f32 s1;
		f32 s2;
		f32 scale;
		f32 x;
		f32 y;
		f32 pad;
		u32 color = menu->mTitle.mColor & 0xFFFFFF00;
		f32 zero  = 0.0f;
		for (int i = 0; i < menu->mTitle.mCount; i++) {
			MenuTitleChar* glyph = &menu->mTitle.mChars[i];
			t                    = glyph->mFade;
			if (!(t <= 0.0f)) {
				CFont font;
				s32 width;
				CTexObj tex = *GetFontGlyphTexture(&font, glyph->mText, &width);
				s0          = sin(glyph->mAngle0);
				bob         = 4.0f * s0;
				rot         = sin(0.7f * glyph->mAngle1);
				s1          = sin(3.1415927f * t);
				s2          = sin(1.5707964f * t);
				scale       = 0.2f * s1 + s2;
				x           = 24.0f + (48.0f * i + (320.0f - 48.0f * menu->mTitle.mCount * 0.5f));
				alpha       = 255.0f * t;
				rot         = -(0.017453292f * (10.0f * rot));
				y           = 64.0f + bob;
				u32 c       = alpha | 0xFFFFFF00;
				DrawGsTexturedQuad(&gGame.mGs, &blob, c, c, c, c, x, y, 0.0f, 20.0f * (2.0f * scale), 20.0f * (2.0f * scale), 0.0f, -zero,
				                   rot);
				scale = 20.0f * (1.7f * scale);
				pad   = (width / 24.0f) * scale;
				pad   = scale - pad;
				for (int j = 0; j < 4; j++) {
					DrawGsTexturedQuad(&gGame.mGs, &tex, alpha, alpha, alpha, alpha, pad / 2 + (x + gSaveTitleOutlineX[j]),
					                   bob + (64.0f + gSaveTitleOutlineY[j]), 0.0f, scale, scale, 0.0f, -zero, rot);
				}
				c = color | alpha;
				DrawGsTexturedQuad(&gGame.mGs, &tex, c, c, c, c, pad / 2 + x, y, 0.0f, scale, scale, 0.0f, -zero, rot);
			}
		}
		SetGsTevMode(gGame.mGs.mCurTevStage, 4);
		SetGsDepthMode(gGame.mGs.mCurRenderMode, 0, 7, 0);
		SetGsChannelEnabled(&gGame.mGs, 0, 0);
	}
}

// the same for the item menu
/**
 * @note Address: 0x8005C798
 * @note Size: 0x5A4
 */
// NONMATCHING: regalloc
extern "C" void DrawItemMenuTitle(MenuItem* menu)
{
	if (menu->mTitle.mActive) {
		SetGsBlendMode(gGame.mGs.mCurRenderMode, 1, 4, 5, 15);
		SetGsChannelEnabled(&gGame.mGs, 0, 0);
		SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 7, 0);
		SetGsTevMode(gGame.mGs.mCurTevStage, 0);
		CTexObj blob = (&gGame.mTpl)->mTex.Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[MenuTitleBlobTexture], 0);
		u32 alpha;
		f32 t;
		f32 s0;
		f32 bob;
		f32 rot;
		f32 s1;
		f32 s2;
		f32 scale;
		f32 x;
		f32 y;
		f32 pad;
		u32 color = menu->mTitle.mColor & 0xFFFFFF00;
		f32 zero  = 0.0f;
		for (int i = 0; i < menu->mTitle.mCount; i++) {
			MenuTitleChar* glyph = &menu->mTitle.mChars[i];
			t                    = glyph->mFade;
			if (!(t <= 0.0f)) {
				CFont font;
				s32 width;
				CTexObj tex = *GetFontGlyphTexture(&font, glyph->mText, &width);
				s0          = sin(glyph->mAngle0);
				bob         = 4.0f * s0;
				rot         = sin(0.7f * glyph->mAngle1);
				s1          = sin(3.1415927f * t);
				s2          = sin(1.5707964f * t);
				scale       = 0.2f * s1 + s2;
				x           = 24.0f + (48.0f * i + (320.0f - 48.0f * menu->mTitle.mCount * 0.5f));
				alpha       = 255.0f * t;
				rot         = -(0.017453292f * (10.0f * rot));
				y           = 64.0f + bob;
				u32 c       = alpha | 0xFFFFFF00;
				DrawGsTexturedQuad(&gGame.mGs, &blob, c, c, c, c, x, y, 0.0f, 20.0f * (2.0f * scale), 20.0f * (2.0f * scale), 0.0f, -zero,
				                   rot);
				scale = 20.0f * (1.7f * scale);
				pad   = (width / 24.0f) * scale;
				pad   = scale - pad;
				for (int j = 0; j < 4; j++) {
					DrawGsTexturedQuad(&gGame.mGs, &tex, alpha, alpha, alpha, alpha, pad / 2 + (x + gItemTitleOutlineX[j]),
					                   bob + (64.0f + gItemTitleOutlineY[j]), 0.0f, scale, scale, 0.0f, -zero, rot);
				}
				c = color | alpha;
				DrawGsTexturedQuad(&gGame.mGs, &tex, c, c, c, c, pad / 2 + x, y, 0.0f, scale, scale, 0.0f, -zero, rot);
			}
		}
		SetGsTevMode(gGame.mGs.mCurTevStage, 4);
		SetGsDepthMode(gGame.mGs.mCurRenderMode, 0, 7, 0);
		SetGsChannelEnabled(&gGame.mGs, 0, 0);
	}
}

#if defined(VERSION_JP)
static const int PCBlueNumberIcon = 151, PCGoldNumberIcon = 150, PCNumberDigits = 123;
static inline void DrawPCNumberSuffix(CTexObj& tex, int index, f32 x, f32 y, f32 scale, f32 opacity)
{
	tex = (&gGame.mTpl)->mTex.Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[index], 0);
	SetGsTevMode(gGame.mGs.mCurTevStage, 0);
	u16 w     = GXGetTexObjWidth(&tex);
	u16 h     = GXGetTexObjHeight(&tex);
	u32 color = (u32)opacity | 0xFFFFFF00;
	DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, 128.0f * scale + x, y, 0.0f, scale * (1.1f * w), scale * h, 0.0f, 0.0f,
	                   0.0f);
	SetGsTevMode(gGame.mGs.mCurTevStage, 4);
}
#else
static const int PCBlueNumberIcon = 153, PCGoldNumberIcon = 152, PCNumberDigits = 125;
#endif

/**
 * @note Address: 0x8003EF10
 * @note Size: 0x56C
 */
// a number of up to 7 digits, right aligned, and the money icon in front
// NONMATCHING: regalloc
extern "C" void DrawPCBlueNumber(bool icon, f32 x, f32 y, f32 alpha, f32 scale, f32 value)
{
	CTexObj tex;
	if (icon) {
		tex = (&gGame.mTpl)->mTex.Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[PCBlueNumberIcon], 0);
		SetGsTevMode(gGame.mGs.mCurTevStage, 0);
		u16 w     = GXGetTexObjWidth(&tex);
		u16 h     = GXGetTexObjHeight(&tex);
		u32 color = (u32)(255.0f * alpha) | 0xC8C8C800;
		DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, x - 16.0f, y, 0.0f, 0.65f * (scale * w), 0.65f * (scale * h), 0.0f,
		                   0.0f, 0.0f);
	}
	tex   = (&gGame.mTpl)->mTex.Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[PCNumberDigits], 0);
	f32 w = scale * GXGetTexObjWidth(&tex);
	f32 h = scale * (GXGetTexObjHeight(&tex) / 10);
	PSMTXScale(tex.mMtx[1], 1.0f, 0.1f, 1.0f);
	tex.mMtxDirty = TRUE;
#if defined(VERSION_JP)
	alpha = 255.0f * alpha;
	u8 a  = alpha;
#else
	u8 a = 255.0f * alpha;
#endif
	for (int i = 0; i < 7; i++) {
		u32 top    = a | 0xCECEFF00;
		u32 bottom = a | 0x5959FF00;
		f32 div    = 1.0f;
		if (i > 0) {
			for (int j = 0; j < i; j++) {
				div *= 10.0f;
			}
		}
		s32 n = value / div;
		if (i == 0 || n != 0) {
			n %= 10;
			PSMTXTrans(tex.mMtx[0], 0.0f, 0.1f * n, 0.0f);
			tex.mMtxDirty = TRUE;
			SetGsTevMode(gGame.mGs.mCurTevStage, 0);
#if defined(VERSION_JP)
			f32 digitX = 7.0f * w + (x - i * w) - 8.0f * scale;
#else
			f32 digitX = 18.0f * scale + (7.0f * w + (x - i * w) - 8.0f * scale);
#endif
			DrawGsTexturedQuad(&gGame.mGs, &tex, top, top, bottom, bottom, digitX, y, 0.0f, w, h, 0.0f, 0.0f, 0.0f);
			SetGsTevMode(gGame.mGs.mCurTevStage, 4);
		}
	}
#if defined(VERSION_JP)
	DrawPCNumberSuffix(tex, 146, x, y, scale, alpha);
#endif
}

/**
 * @note Address: 0x8003F47C
 * @note Size: 0x56C
 */
// the same with the other icon and colors
// NONMATCHING: regalloc
extern "C" void DrawPCGoldNumber(bool icon, f32 x, f32 y, f32 alpha, f32 scale, f32 value)
{
	CTexObj tex;
	if (icon) {
		tex = (&gGame.mTpl)->mTex.Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[PCGoldNumberIcon], 0);
		SetGsTevMode(gGame.mGs.mCurTevStage, 0);
		u16 w     = GXGetTexObjWidth(&tex);
		u16 h     = GXGetTexObjHeight(&tex);
		u32 color = (u32)(255.0f * alpha) | 0xC8C8C800;
		DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, x - 16.0f, y, 0.0f, 0.8f * (scale * w), 0.8f * (scale * h), 0.0f,
		                   0.0f, 0.0f);
	}
	tex   = (&gGame.mTpl)->mTex.Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[PCNumberDigits], 0);
	f32 w = scale * GXGetTexObjWidth(&tex);
	f32 h = scale * (GXGetTexObjHeight(&tex) / 10);
	PSMTXScale(tex.mMtx[1], 1.0f, 0.1f, 1.0f);
	tex.mMtxDirty = TRUE;
#if defined(VERSION_JP)
	alpha = 255.0f * alpha;
	u8 a  = alpha;
#else
	u8 a = 255.0f * alpha;
#endif
	for (int i = 0; i < 7; i++) {
		u32 top    = a | 0xEFEBC500;
		u32 bottom = a | 0xEDA83600;
		f32 div    = 1.0f;
		if (i > 0) {
			for (int j = 0; j < i; j++) {
				div *= 10.0f;
			}
		}
		s32 n = value / div;
		if (i == 0 || n != 0) {
			n %= 10;
			PSMTXTrans(tex.mMtx[0], 0.0f, 0.1f * n, 0.0f);
			tex.mMtxDirty = TRUE;
			SetGsTevMode(gGame.mGs.mCurTevStage, 0);
#if defined(VERSION_JP)
			f32 digitX = 7.0f * w + (x - i * w) - 8.0f * scale;
#else
			f32 digitX = 18.0f * scale + (7.0f * w + (x - i * w) - 8.0f * scale);
#endif
			DrawGsTexturedQuad(&gGame.mGs, &tex, top, top, bottom, bottom, digitX, y, 0.0f, w, h, 0.0f, 0.0f, 0.0f);
			SetGsTevMode(gGame.mGs.mCurTevStage, 4);
		}
	}
#if defined(VERSION_JP)
	DrawPCNumberSuffix(tex, 142, x, y, scale, alpha);
#endif
}

/**
 * @note Address: 0x80049418
 * @note Size: 0xF0
 */
void MenuPC::State11(u32)
{
	MenuPC* menu = this;
	if (menu->mFrameHeight <= 2.0f) {
		gGame.mEvt._10 = menu->mExitState;
		menu->Close();
		sMenuMain.Resume();
		PlayGameSound(&gGame.mSound, 0x86, 0xFF);
		gGame.mStatus.mFlags &= ~2ULL;
		StopGameSfx(&gGame.mSound, 0x30008);
	}
}

// the status screen's item strip, europe scales it to the screen
static inline void SetScissor(u32 top, u32 height)
{
#if defined(VERSION_PAL)
	f32 scale     = gGame.mGs.mScreen._40;
	u32 scaledH   = (u32)((f32)height * scale);
	u32 scaledTop = (u32)((f32)top * scale);
	GXSetScissor(0, scaledTop, 640, scaledH);
#else
	GXSetScissor(0, top, 640, height);
#endif
}

// looks up flag 4 of every carried item, the results go unused
static inline void CheckPlayerInventoryFlags()
{
	for (s32 i = 255; i >= 0; --i) {
		InventoryItem* items = (InventoryItem*)&gGame.GetPlayer()->mParams.mInventory;
		GetItemFlagsMasked(items[i].mId, 4);
	}
}

/**
 * @note Address: 0x8005773C
 * @note Size: 0x1F8
 */
BOOL MenuStatus::Draw()
{
	if (CMenu::Draw()) {
		DrawPanelBackground<ItemStatusPanel>(mAlpha);
		DrawStatusMenuTabs(this);
		if (mMode == 2 || mMode == 4) {
			SetScissor(104, 248);
			f32 x    = (mX - 42.0f) + mOffsetX;
			f32 y    = (180.0f + mY) + mOffsetY;
			s32 item = mPhase == 100 ? -1 : mItem;
			DrawInventoryGrid(item, x, y, mAlpha, 1.0f, mPulsePhase);
			DrawStatusItemDetails(sStatusItemTable.mItems[mItem].mItem, 320.0f, 224.0f, mAlpha, mDetailScale);
			CheckPlayerInventoryFlags();
			DrawDiscardCountPanel(this);
			SetScissor(0, 448);
		}
		DrawPlayerStatusOverview(this);
		DrawStatusDiagram(this);
		DrawStatusIconGrid(this);
		DrawStatusHelpText(this);
		DrawStatusMenuTitle(this);
		return TRUE;
	}
	return FALSE;
}

/**
 * @note Address: 0x80057084
 * @note Size: 0x294
 */
// two byte text a character at a time, digits and narrow letters moved to the middle of their cell
// NONMATCHING: regalloc
extern "C" void DrawDoubleByteMenuText(const char* text, f32 x, f32 y, f32 z, f32 w, f32 h, f32 alpha)
{
	const char* p;
	u32 shadow;
	u32 color;
	u32 count;
	int i;
	p      = text;
	count  = strlen(text) / 2;
	shadow = (u32)(128.0f * alpha) | 0xFFFFFF00;
	color  = (u32)(255.0f * alpha) | 0x20202000;
	for (i = 0; i < count; i++, p += 2) {
		char c[3];
		char buf[4];
		c[0]     = p[0];
		c[1]     = p[1];
		c[2]     = 0;
		f32 left = i * w + x;
		DrawBeveledMenuRect(shadow, left, y, z, w, h, alpha);
		f32 shift = 0.0f;
		for (int digit = 0; digit < 10; digit++) {
			sprintf(buf, "%d", digit);
			ConvertHudTextToFullWidth(buf);
			if (strcmp(c, buf) == 0) {
				if (digit == 1) {
					shift = 6.0f;
				} else {
					shift = 3.0f;
				}
				break;
			}
		}
		for (int letter = 0; letter < 26; letter++) {
			buf[0] = 'A' + letter;
			buf[1] = 0;
			ConvertHudTextToFullWidth(buf);
			if (strcmp(c, buf) == 0) {
				switch (letter) {
				case 'B' - 'A':
				case 'E' - 'A':
				case 'F' - 'A':
				case 'J' - 'A':
				case 'K' - 'A':
				case 'L' - 'A':
				case 'P' - 'A':
				case 'R' - 'A':
				case 'S' - 'A':
				case 'T' - 'A':
					shift = 6.0f;
					break;
				case 'I' - 'A':
					shift = 12.0f;
					break;
				case 'C' - 'A':
				case 'H' - 'A':
				case 'U' - 'A':
				case 'X' - 'A':
					shift = 4.5f;
					break;
				default:
					shift = 3.0f;
					break;
				}
				break;
			}
		}
		DrawGsProportionalTextFormat(&gGame.mGs, color, c, shift + left - w / 2, y - h / 2, 0.0f, w, h);
	}
}

// empties one entry of the status table
static inline void ClearStatusEntry(MenuStatusEntry* e)
{
	e->mItem          = -1;
	e->mCount         = -1;
	e->mStatus.mFlags = 0;
}

static inline void ClearStatusTable(MenuStatusTable* table)
{
	table->mPage = 0;
	for (int i = 0; i < 256; i++) {
		ClearStatusEntry(&table->mItems[i]);
	}
	for (int i = 0; i < 4; i++) {
		ClearStatusEntry(&table->mPrev[i]);
		ClearStatusEntry(&table->mNext[i]);
	}
}

/**
 * @note Address: 0x80060118
 * @note Size: 0x650
 */
// NONMATCHING: regalloc
void MenuItem::Open()
{
	mState = 1;
	Resume();
	mWindow.mOpen = TRUE;
	gGame.mStatus.mFlags |= 2;
	mWindow.SetSize(0.0f, 0.0f);
	mWindow.SetPos(0.0f, 0.0f);
	ClearStatusTable(&sStatusItemTable);
	PopulateOwnedItemPage(&sStatusItemTable);
	s32 count = CountMenuItems();
	mGridX    = 758.0f;
	mGridY    = 36.0f;
	if (count == 0) {
		mTargetGridX = 360.0f;
	} else {
		mTargetGridX = 360.0f - (count - 1) * 51;
	}
	mTargetGridX   = 207.0f;
	mTargetGridY   = 36.0f;
	_170           = -10.0f;
	_174           = -200.0f;
	_178           = -10.0f;
	_17C           = 0.0f;
	mPhase         = 0;
	mDelay         = 0;
	mSelection     = 0;
	mPrevSelection = mSelection;
	_190           = 0.0f;
	_194           = 0.0f;
	u32 color      = (u32)(255.0f * mAlpha) | 0xFFFFFF00;
	mTitle.Set(GetLocalizedInterfaceText(StatusTitleText + 1), color);
	mShowHelp  = 0;
	mNameAlpha = 0.0f;
	s16 id     = sStatusItemTable.mItems[mSelection].mItem;
	if (id >= 0 && CountMenuItems() > 0) {
		mName     = GetItemName(id);
		char* msg = GetItemName(id);
		mShowHelp = 1;
		mHelp     = msg;
	} else {
		mShowHelp = 1;
		mHelp     = NULL;
	}
	mNameCharCount = 0;
	for (int i = 0; i < (int)ARRAY_SIZE(mBg); i++) {
		mBg[i].mAngle0 = 0.017453292f * (rand() % 360);
		mBg[i].mAngle1 = 0.017453292f * (rand() % 360);
	}
}

/**
 * @note Address: 0x80057934
 * @note Size: 0x700
 */
// moving the cursor over the map grid, each cell names its place once it's been visited
// NONMATCHING: regalloc
void MenuStatus::State9(u32)
{
	MenuStatus* menu = this;
	CPad pad(0);
	s32 neighbors[13][4] = {
		{ 4, 2, 9, 5 },  { 0, 2, 10, 6 }, { 0, 3, 10, 6 },  { 2, 4, 11, 7 }, { 3, 0, 12, 8 },  { 8, 6, 0, 9 },  { 5, 7, 2, 10 },
		{ 6, 8, 3, 11 }, { 7, 5, 4, 12 }, { 12, 10, 5, 0 }, { 9, 11, 6, 2 }, { 10, 12, 7, 3 }, { 11, 9, 8, 4 },
	};
	switch (menu->mPhase) {
	case 0:
		if (pad.mPad->mRepeat & 0x40001) {
			menu->mOverviewSelection = neighbors[menu->mOverviewSelection][2];
			if (gStatusEntryAvailable[menu->mOverviewSelection]) {
				char* msg       = GetStatusOverviewLabel(menu->mOverviewSelection);
				menu->mShowHelp = 1;
				menu->mHelp     = msg;
			} else {
				char* msg       = GetLocalizedInterfaceText(StatusEmptyText);
				menu->mShowHelp = 1;
				menu->mHelp     = msg;
			}
			PlayGameSound(&gGame.mSound, 0x87, 0xFF);
		} else if (pad.mPad->mRepeat & 0x80002) {
			menu->mOverviewSelection = neighbors[menu->mOverviewSelection][3];
			if (gStatusEntryAvailable[menu->mOverviewSelection]) {
				char* msg       = GetStatusOverviewLabel(menu->mOverviewSelection);
				menu->mShowHelp = 1;
				menu->mHelp     = msg;
			} else {
				char* msg       = GetLocalizedInterfaceText(StatusEmptyText);
				menu->mShowHelp = 1;
				menu->mHelp     = msg;
			}
			PlayGameSound(&gGame.mSound, 0x87, 0xFF);
		}
		if (pad.mPad->mRepeat & 0x10008) {
			menu->mOverviewSelection = neighbors[menu->mOverviewSelection][0];
			if (gStatusEntryAvailable[menu->mOverviewSelection]) {
				char* msg       = GetStatusOverviewLabel(menu->mOverviewSelection);
				menu->mShowHelp = 1;
				menu->mHelp     = msg;
			} else {
				char* msg       = GetLocalizedInterfaceText(StatusEmptyText);
				menu->mShowHelp = 1;
				menu->mHelp     = msg;
			}
			PlayGameSound(&gGame.mSound, 0x87, 0xFF);
		} else if (pad.mPad->mRepeat & 0x20004) {
			menu->mOverviewSelection = neighbors[menu->mOverviewSelection][1];
			if (gStatusEntryAvailable[menu->mOverviewSelection]) {
				char* msg       = GetStatusOverviewLabel(menu->mOverviewSelection);
				menu->mShowHelp = 1;
				menu->mHelp     = msg;
			} else {
				char* msg       = GetLocalizedInterfaceText(StatusEmptyText);
				menu->mShowHelp = 1;
				menu->mHelp     = msg;
			}
			PlayGameSound(&gGame.mSound, 0x87, 0xFF);
		}
		if (pad.mPad->mTrigger & 0x200) {
			menu->mTargetOverviewX = 1086.0f;
			PlayGameSound(&gGame.mSound, 0x86, 0xFF);
			for (int i = 0; i < 4; i++) {
				menu->mTabs[i].mX = 0.0f;
				if (menu->mHasFourthTab) {
					menu->mTabs[i].mTargetX = 198.0f + i * 108 - 8.0f;
				} else {
					menu->mTabs[i].mTargetX = 252.0f + i * 108;
				}
			}
			menu->_658      = 0.0f;
			menu->mPrevMode = menu->mMode;
			menu->mMode     = 4;
			menu->mNextMode = 1;
			menu->mPhase    = 0;
			u32 color       = (u32)(255.0f * menu->mAlpha) | 0xFFFFFF00;
			menu->mTitle.Set(GetLocalizedInterfaceText(StatusTitleText), color);
			char* msg = GetLocalizedInterfaceText((menu->mSelection >= 4 ? 0 : gStatusTabTextOffsets[menu->mSelection]) + StatusTitleText);
			menu->mShowHelp       = 1;
			menu->mHelp           = msg;
			menu->mAnimationFrame = 0;
		}
		break;
	case 100:
		if (pad.mPad->mTrigger & 0x300) {
			PlayGameSound(&gGame.mSound, 0x88, 0xFF);
			menu->mPhase             = 0;
			menu->mTargetDetailScale = 0.0f;
		}
		break;
	}
}

/**
 * @note Address: 0x80058034
 * @note Size: 0x728
 */
// the same on the 6 by 4 grid, it wraps around
// NONMATCHING: regalloc
void MenuStatus::State8(u32)
{
	MenuStatus* menu = this;
	CPad pad(0);
	switch (menu->mPhase) {
	case 0:
		if (pad.mPad->mRepeat & 0x40001) {
			if (--menu->mIconSelection < 0) {
				menu->mIconSelection = 23;
			}
			if (gStatusIconUnlocked[menu->mIconSelection]) {
				char* msg       = GetStatusIconName(menu->mIconSelection);
				menu->mShowHelp = 1;
				menu->mHelp     = msg;
			} else {
				char* msg       = GetLocalizedInterfaceText(StatusEmptyText);
				menu->mShowHelp = 1;
				menu->mHelp     = msg;
			}
			PlayGameSound(&gGame.mSound, 0x87, 0xFF);
		} else if (pad.mPad->mRepeat & 0x80002) {
			menu->mIconSelection++;
			if (menu->mIconSelection > 23) {
				menu->mIconSelection = 0;
			}
			if (gStatusIconUnlocked[menu->mIconSelection]) {
				char* msg       = GetStatusIconName(menu->mIconSelection);
				menu->mShowHelp = 1;
				menu->mHelp     = msg;
			} else {
				char* msg       = GetLocalizedInterfaceText(StatusEmptyText);
				menu->mShowHelp = 1;
				menu->mHelp     = msg;
			}
			PlayGameSound(&gGame.mSound, 0x87, 0xFF);
		}
		if (pad.mPad->mRepeat & 0x10008) {
			s32 row = menu->mIconSelection / 6;
			s32 col = menu->mIconSelection % 6;
			if (--row < 0) {
				row = 3;
			}
			menu->mIconSelection = col + row * 6;
			if (gStatusIconUnlocked[menu->mIconSelection]) {
				char* msg       = GetStatusIconName(menu->mIconSelection);
				menu->mShowHelp = 1;
				menu->mHelp     = msg;
			} else {
				char* msg       = GetLocalizedInterfaceText(StatusEmptyText);
				menu->mShowHelp = 1;
				menu->mHelp     = msg;
			}
			PlayGameSound(&gGame.mSound, 0x87, 0xFF);
		} else if (pad.mPad->mRepeat & 0x20004) {
			s32 row = menu->mIconSelection / 6;
			s32 col = menu->mIconSelection % 6;
			if (++row > 3) {
				row = 0;
			}
			menu->mIconSelection = col + row * 6;
			if (gStatusIconUnlocked[menu->mIconSelection]) {
				char* msg       = GetStatusIconName(menu->mIconSelection);
				menu->mShowHelp = 1;
				menu->mHelp     = msg;
			} else {
				char* msg       = GetLocalizedInterfaceText(StatusEmptyText);
				menu->mShowHelp = 1;
				menu->mHelp     = msg;
			}
			PlayGameSound(&gGame.mSound, 0x87, 0xFF);
		}
		if (pad.mPad->mTrigger & 0x200) {
			menu->mTargetGridX = 959.0f;
			PlayGameSound(&gGame.mSound, 0x86, 0xFF);
			for (int i = 0; i < 4; i++) {
				menu->mTabs[i].mX = 0.0f;
				if (menu->mHasFourthTab) {
					menu->mTabs[i].mTargetX = 198.0f + i * 108 - 8.0f;
				} else {
					menu->mTabs[i].mTargetX = 252.0f + i * 108;
				}
			}
			menu->mPrevMode = menu->mMode;
			menu->mMode     = 4;
			menu->mNextMode = 1;
			menu->mPhase    = 0;
			u32 color       = (u32)(255.0f * menu->mAlpha) | 0xFFFFFF00;
			menu->mTitle.Set(GetLocalizedInterfaceText(StatusTitleText), color);
			char* msg = GetLocalizedInterfaceText((menu->mSelection >= 4 ? 0 : gStatusTabTextOffsets[menu->mSelection]) + StatusTitleText);
			menu->mShowHelp       = 1;
			menu->mHelp           = msg;
			menu->mAnimationFrame = 0;
		}
		break;
	case 100:
		if (pad.mPad->mTrigger & 0x300) {
			PlayGameSound(&gGame.mSound, 0x88, 0xFF);
			menu->mPhase             = 0;
			menu->mTargetDetailScale = 0.0f;
		}
		break;
	}
}

/**
 * @note Address: 0x80058764
 * @note Size: 0x458
 */
// NONMATCHING: regalloc
void MenuStatus::State5(u32)
{
	MenuStatus* menu = this;
	CPad pad(0);
	if (pad.mPad->mTrigger & 0x200) {
		menu->mTargetDiagramX = 1086.0f;
		PlayGameSound(&gGame.mSound, 0x86, 0xFF);
		for (int i = 0; i < 4; i++) {
			menu->mTabs[i].mX = 0.0f;
			if (menu->mHasFourthTab) {
				menu->mTabs[i].mTargetX = 198.0f + i * 108 - 8.0f;
			} else {
				menu->mTabs[i].mTargetX = 252.0f + i * 108;
			}
		}
		menu->mPrevMode = menu->mMode;
		menu->mMode     = 4;
		menu->mNextMode = 1;
		menu->mPhase    = 0;
		u32 color       = (u32)(255.0f * menu->mAlpha) | 0xFFFFFF00;
		menu->mTitle.Set(GetLocalizedInterfaceText(StatusTitleText), color);
		char* msg = GetLocalizedInterfaceText((menu->mSelection >= 4 ? 0 : gStatusTabTextOffsets[menu->mSelection]) + StatusTitleText);
		menu->mShowHelp       = 1;
		menu->mHelp           = msg;
		menu->mAnimationFrame = 0;
	}
}

extern "C" f32 gPCDialogueX;
extern "C" void ApproachHudValue(f32*, f32*, f32);

static inline void ResetPCCharacters(MenuPC* menu)
{
	menu->mCharIndex     = 0;
	menu->mNumberOffset  = 0;
	menu->mInsertsNumber = 0;
	char space[]         = "\x81\x40"; // a full-width space
	for (int i = 0; i < 40; i++)
		menu->mChars[i] = *(u16*)space;
	menu->mTerminator = 0;
	for (int i = 0; i < 40; i++) {
		menu->mCharX[i]             = 640.0f;
		menu->mCharFlags[i]         = 0;
		*(u16*)menu->mCharCopies[i] = *(u16*)space;
		menu->mCharCopies[i][2]     = 0;
	}
	menu->mGlyphFrames    = 0;
	menu->mNextGlyph      = 0;
	menu->mKeeperAnimStep = 0;
	menu->mKeeperAnimId   = 0;
	menu->mPlayKeeperAnim = 0;
#if !defined(VERSION_JP)
	gPCDialogueX = 640.0f;
	// Open and State0 clear the first two rows in both US and PAL.
	memset(gPCDialogueChunks, 0, 2 * sizeof(gPCDialogueChunks[0]));
#endif
}

static inline void ResetPCText(MenuPC* menu, s32 step)
{
	menu->mPrevMode  = menu->mMode;
	menu->mMode      = step;
	menu->mPhase     = 0;
	menu->mTextPhase = 0;
	menu->mTextX     = 210.0f;
	menu->mTextY     = 129.0f;
	menu->mTargetX   = menu->mTextX;
	menu->mTargetY   = menu->mTextY;
#if !defined(VERSION_JP)
	switch (menu->mMode) {
	case 3:
	case 7:
	case 10:
		menu->mTextY   = 121.0f;
		menu->mTargetY = menu->mTextY;
		break;
	}
#endif
	ResetPCCharacters(menu);
}

/**
 * @note Address: 0x8004BBC0
 * @note Size: 0x288
 */
void MenuPC::State4(u32)
{
	MenuPC* menu = this;
	CPad pad(0);
	if (pad.mPad->mTrigger & 0x200) {
		ResetPCText(menu, 2);
		PlayGameSound(&gGame.mSound, 0x86, 0xFF);
	}
}

/**
 * @note Address: 0x8004B92C
 * @note Size: 0x294
 */
void MenuPC::State5(u32)
{
	MenuPC* menu = this;
	CPad pad(0);
	if (pad.mPad->mTrigger & 0x200) {
		gGame.mSubwin.mEnabled = 0;
		ResetPCText(menu, 2);
		PlayGameSound(&gGame.mSound, 0x86, 0xFF);
	}
}

/**
 * @note Address: 0x8004D584
 * @note Size: 0x2D8
 */
void MenuPC::State1(u32 frames)
{
	MenuPC* menu = this;
	switch (menu->mPhase) {
	case 0:
		PlayGameSound(&gGame.mSound, 0x30008, 0xFF);
		menu->mPhase++;
		break;
	case 1:
		break;
	}
	ApproachHudValue(&menu->mProgress, &menu->mTargetProgress, menu->mSpeed * frames);
	if (menu->mProgress >= 1.0f)
		ResetPCText(menu, 2);
}

/**
 * @note Address: 0x8004D85C
 * @note Size: 0x4C0
 */
void MenuPC::State0(u32)
{
	MenuPC* menu = this;
	if (menu->mShopOnly) {
		ResetPCText(menu, 3);
	} else {
		ResetPCText(menu, 1);
		menu->mProgress       = 0.0f;
		menu->mTargetProgress = 1.0f;
		switch (rand() % 2) {
		case 0:
			menu->mSpeed = 0.04f;
			break;
		case 1:
			menu->mSpeed = 0.08f;
			break;
		}
	}
}

static inline s16 MenuHistoryValue(int i)
{
	return sShopHistoryEntries[i].mCount;
}

static inline void RestorePCHistory(MenuPC* menu)
{
	if (menu->mHistoryCount > 0) {
		for (int i = 0; i < menu->mHistoryCount; i++) {
			gEventVars.mVars[menu->mHistoryStart + i + menu->mHistoryCount] = MenuHistoryValue(i);
		}
		gEventVars.mVars[196] = menu->mDepositBalance;
	}
}

/**
 * @note Address: 0x8004CD38
 * @note Size: 0x84C
 */
void MenuPC::State2(u32)
{
	MenuPC* menu = this;
	CPad pad(0);
	if (pad.mPad->mRepeat & 0x10008) {
		menu->mChoice--;
		if (menu->mChoice < 0)
			menu->mChoice = menu->mChoiceCount - 1;
		PlayGameSound(&gGame.mSound, 0x87, 0xFF);
	} else if (pad.mPad->mRepeat & 0x20004) {
		menu->mChoice++;
		if (menu->mChoice > menu->mChoiceCount - 1)
			menu->mChoice = 0;
		PlayGameSound(&gGame.mSound, 0x87, 0xFF);
	} else if (pad.mPad->mTrigger & 0x100) {
		s32 step = menu->mChoices[menu->mChoice];
		if (step == 7) {
			if (menu->mCanSelect) {
				ResetPCText(menu, 7);
				PlayGameSound(&gGame.mSound, 0x88, 0xFF);
			} else {
				PlayGameSound(&gGame.mSound, 0x85, 0xFF);
			}
		} else if (step >= 0) {
			ResetPCText(menu, step);
			PlayGameSound(&gGame.mSound, 0x88, 0xFF);
		}
	} else if (pad.mPad->mTrigger & 0x200) {
		ResetPCText(menu, 11);
		menu->mTargetFrameHeight = 0.0f;
		RestorePCHistory(menu);
	}
}

static inline s32 MenuPower10(s32 exponent)
{
	s32 result = 1;
	for (s32 i = 0; i < exponent; i++)
		result *= 10;
	return result;
}
static inline s32 MenuDigit(s32 value, s32 digit)
{
	if (digit == 0)
		return value % 10;
	return (value / MenuPower10(digit)) % 10;
}
// chibi robo's money and scrap
static inline s32 GetMoney()
{
	return gGame.GetPlayer()->mParams.mMoney;
}
static inline s32 GetScrap()
{
	return gGame.GetPlayer()->mParams.mScrap;
}

static inline void DepositPCAmount(MenuPC* menu)
{
	s32 deposit = menu->mAmount;
	s32 money   = GetMoney();
	money -= deposit;
	menu->mDepositBalance += deposit;
	Params* params = &gGame.GetPlayer()->mParams;
	params->SetMoney(money);
	menu->mMoneyDisplay.target = money;
}

static inline void RecyclePCAmount(MenuPC* menu)
{
	s32 scrap = GetScrap();
	scrap -= menu->mAmount;
	gGame.GetPlayer()->mParams.SetScrap(scrap);
	s32 money = GetMoney();
	money += menu->mAmount * menu->mScrapRate;
	gGame.GetPlayer()->mParams.SetMoney(money);
	menu->mScrapDisplay.target = scrap;
	menu->mMoneyDisplay.target = money;
}

/**
 * @note Address: 0x8004A188
 * @note Size: 0x9B4
 */
void MenuPC::State9(u32)
{
	MenuPC* menu = this;
	CPad pad(0);
	s32 digits[7] = { 0, 0, 0, 0, 0, 0, 0 };
	switch (menu->mPhase) {
	case 0:
		if (pad.mPad->mRepeat & 0x80002) {
			menu->mMoneyDigit--;
			if (menu->mMoneyDigit < 0)
				menu->mMoneyDigit = 4;
			PlayGameSound(&gGame.mSound, 0x87, 0xFF);
		} else if (pad.mPad->mRepeat & 0x40001) {
			menu->mMoneyDigit++;
			if (menu->mMoneyDigit > 4)
				menu->mMoneyDigit = 0;
			PlayGameSound(&gGame.mSound, 0x87, 0xFF);
		} else if (pad.mPad->mRepeat & 0x20004) {
			s32 value = 0;
			for (s32 i = 0; i < 7; i++) {
				if (i == 0)
					digits[i] = menu->mAmount % 10;
				else {
					s32 power = 1;
					for (s32 j = 0; j < i; j++)
						power *= 10;
					digits[i] = (menu->mAmount / power) % 10;
				}
			}
			digits[menu->mMoneyDigit]--;
			if (digits[menu->mMoneyDigit] < 0)
				digits[menu->mMoneyDigit] = 9;
			for (s32 i = 0; i < 7; i++) {
				s32 power = 1;
				if (i > 0) {
					for (s32 j = 0; j < i; j++)
						power *= 10;
				}
				value += power * digits[i];
			}
			menu->mAmount = value;
			PlayGameSound(&gGame.mSound, 0x87, 0xFF);
		} else if (pad.mPad->mRepeat & 0x10008) {
			s32 value = 0;
			for (s32 i = 0; i < 7; i++) {
				if (i == 0)
					digits[i] = menu->mAmount % 10;
				else {
					s32 power = 1;
					for (s32 j = 0; j < i; j++)
						power *= 10;
					digits[i] = (menu->mAmount / power) % 10;
				}
			}
			digits[menu->mMoneyDigit]++;
			if (digits[menu->mMoneyDigit] > 9)
				digits[menu->mMoneyDigit] = 0;
			for (s32 i = 0; i < 7; i++) {
				s32 power = 1;
				if (i > 0) {
					for (s32 j = 0; j < i; j++)
						power *= 10;
				}
				value += power * digits[i];
			}
			menu->mAmount = value;
			PlayGameSound(&gGame.mSound, 0x87, 0xFF);
		} else if (pad.mPad->mTrigger & 0x200) {
			ResetPCText(menu, 2);
			PlayGameSound(&gGame.mSound, 0x86, 0xFF);
		} else if (pad.mPad->mTrigger & 0x100) {
			if (menu->mAmount > GetMoney()) {
				menu->mAmount = GetMoney();
				PlayGameSound(&gGame.mSound, 0x88, 0xFF);
				return;
			}
			if (menu->mAmount == 0) {
				PlayGameSound(&gGame.mSound, 0x88, 0xFF);
				return;
			}
			menu->mPhase = 1;
			PlayGameSound(&gGame.mSound, 0x88, 0xFF);
		}
		break;
	case 1:
		if (pad.mPad->mRepeat & 0x40001) {
			if (menu->mYesNo > 0) {
				menu->mYesNo--;
				PlayGameSound(&gGame.mSound, 0x87, 0xFF);
			}
		} else if (pad.mPad->mRepeat & 0x80002) {
			if (menu->mYesNo < 1) {
				menu->mYesNo++;
				PlayGameSound(&gGame.mSound, 0x87, 0xFF);
			}
		}
		if (pad.mPad->mTrigger & 0x100) {
			switch (menu->mYesNo) {
			case 0: {
				DepositPCAmount(menu);
				PlayGameSound(&gGame.mSound, 0x88, 0xFF);
				menu->mPhase = 2;
				break;
			}
			case 1:
				menu->mPhase = 0;
				PlayGameSound(&gGame.mSound, 0x86, 0xFF);
				break;
			}
		} else if (pad.mPad->mTrigger & 0x200) {
			menu->mPhase = 0;
			PlayGameSound(&gGame.mSound, 0x86, 0xFF);
		}
		break;
	case 2:
		if (pad.mPad->mTrigger & 0x300) {
			menu->mPhase = 0;
			PlayGameSound(&gGame.mSound, 0x86, 0xFF);
		}
		break;
	}
}

/**
 * @note Address: 0x80049508
 * @note Size: 0xC80
 */
void MenuPC::State10(u32)
{
	MenuPC* menu = this;
	CPad pad(0);
	s32 digits[7] = { 0, 0, 0, 0, 0, 0, 0 };
	switch (menu->mPhase) {
	case 0:
		menu->mYesNo = -1;
		if (pad.mPad->mRepeat & 0x80002) {
			menu->mScrapDigit--;
			if (menu->mScrapDigit < 0)
				menu->mScrapDigit = 4;
			PlayGameSound(&gGame.mSound, 0x87, 0xFF);
		} else if (pad.mPad->mRepeat & 0x40001) {
			menu->mScrapDigit++;
			if (menu->mScrapDigit > 4)
				menu->mScrapDigit = 0;
			PlayGameSound(&gGame.mSound, 0x87, 0xFF);
		} else if (pad.mPad->mRepeat & 0x20004) {
			s32 value = 0;
			for (s32 i = 0; i < 5; i++) {
				if (i == 0)
					digits[i] = menu->mAmount % 10;
				else {
					s32 power = 1;
					for (s32 j = 0; j < i; j++)
						power *= 10;
					digits[i] = (menu->mAmount / power) % 10;
				}
			}
			digits[menu->mScrapDigit]--;
			if (digits[menu->mScrapDigit] < 0)
				digits[menu->mScrapDigit] = 9;
			for (s32 i = 0; i < 5; i++) {
				s32 power = 1;
				if (i > 0) {
					for (s32 j = 0; j < i; j++)
						power *= 10;
				}
				value += power * digits[i];
			}
			menu->mAmount = value;
			PlayGameSound(&gGame.mSound, 0x87, 0xFF);
		} else if (pad.mPad->mRepeat & 0x10008) {
			s32 value = 0;
			for (s32 i = 0; i < 5; i++) {
				if (i == 0)
					digits[i] = menu->mAmount % 10;
				else {
					s32 power = 1;
					for (s32 j = 0; j < i; j++)
						power *= 10;
					digits[i] = (menu->mAmount / power) % 10;
				}
			}
			digits[menu->mScrapDigit]++;
			if (digits[menu->mScrapDigit] > 9)
				digits[menu->mScrapDigit] = 0;
			for (s32 i = 0; i < 5; i++) {
				s32 power = 1;
				if (i > 0) {
					for (s32 j = 0; j < i; j++)
						power *= 10;
				}
				value += power * digits[i];
			}
			menu->mAmount = value;
			PlayGameSound(&gGame.mSound, 0x87, 0xFF);
		} else if (pad.mPad->mTrigger & 0x200) {
			ResetPCText(menu, 2);
			PlayGameSound(&gGame.mSound, 0x86, 0xFF);
		} else if (pad.mPad->mTrigger & 0x100) {
			if (menu->mAmount > GetScrap()) {
				menu->mAmount = GetScrap();
				PlayGameSound(&gGame.mSound, 0x85, 0xFF);
				return;
			}
			if (menu->mAmount == 0) {
				PlayGameSound(&gGame.mSound, 0x85, 0xFF);
				return;
			}
			menu->mPhase = 1;
			menu->mYesNo = 0;
			PlayGameSound(&gGame.mSound, 0x88, 0xFF);
		}
		break;
	case 1:
		if (pad.mPad->mRepeat & 0x10008) {
			if (menu->mYesNo > 0) {
				menu->mYesNo--;
				PlayGameSound(&gGame.mSound, 0x87, 0xFF);
			}
		} else if (pad.mPad->mRepeat & 0x20004) {
			if (menu->mYesNo < 1) {
				menu->mYesNo++;
				PlayGameSound(&gGame.mSound, 0x87, 0xFF);
			}
		}
		if (pad.mPad->mTrigger & 0x100) {
			switch (menu->mYesNo) {
			case 0: {
				RecyclePCAmount(menu);
				menu->mYesNo = 2;
				PlayGameSound(&gGame.mSound, 0x30009, 0xFF);
				menu->mPhase = 2;
				break;
			}
			case 1:
				menu->mPhase = 0;
				PlayGameSound(&gGame.mSound, 0x86, 0xFF);
				break;
			}
		} else if (pad.mPad->mTrigger & 0x200) {
			menu->mPhase = 0;
			PlayGameSound(&gGame.mSound, 0x86, 0xFF);
		}
		break;
	case 2:
		if (pad.mPad->mTrigger & 0x300) {
			menu->mPhase = 0;
			ResetPCText(menu, 2);
			PlayGameSound(&gGame.mSound, 0x86, 0xFF);
		}
		break;
	}
}
// the placement entry's scale, in percent there
static inline Vec EntryScale(XObj* obj)
{
	UnkStageObj* entry = obj->mCoord._00;
	Vec scale          = lbl_80362048[0];
	scale.x            = entry->mScale[0] / 100.0f;
	scale.y            = entry->mScale[1] / 100.0f;
	scale.z            = entry->mScale[2] / 100.0f;
	return scale;
}

/**
 * @note Address: 0x800621E0
 * @note Size: 0xD8
 */
extern "C" CVec GetStagePlacementScale(XObj* obj)
{
	return EntryScale(obj);
}

// and its position, in centimeters
static inline Vec EntryPos(XObj* obj)
{
	UnkStageObj* entry = obj->mCoord._00;
	Vec pos            = lbl_8036203C;
	pos.x              = entry->mPos[0] / 100.0f;
	pos.y              = entry->mPos[1] / 100.0f;
	pos.z              = entry->mPos[2] / 100.0f;
	return pos;
}

/**
 * @note Address: 0x800622B8
 * @note Size: 0xD8
 */
extern "C" CVec GetStagePlacementPosition(XObj* obj)
{
	return EntryPos(obj);
}

extern "C" void PopulateShopCatalogPage(MenuPCPage*);

static inline s32 CountPCShopItems(MenuPCItem* table)
{
	s16 x;
	s16 y;
	int i;
	s32 count;
	count = 0;
	for (i = 0; i < 20; i++) {
		x = table[i].mItem;
		y = table[i].mCount;
		if (x >= 0 && y >= 0)
			count++;
	}
	return count;
}
static inline s32 PCShopMaxPage(MenuPCItem*& table)
{
	s16 x, y;
	int i;
	s32 pages = 0;
	for (i = 0; i < 20; table++, i++) {
		x = table->mItem;
		y = table->mCount;
		if (x >= 0 && y >= 0)
			pages++;
	}
	pages -= 8;
	if (pages < 1)
		pages = 1;
	return pages;
}
static inline s32 ResetPCShopPage(MenuPC* menu, MenuPCItem*& table)
{
	s16 x, y;
	int i;
	s32 pages    = 0;
	i            = 0;
	menu->mPhase = 0;
	for (; i < 20; table++, i++) {
		x = table->mItem;
		y = table->mCount;
		if (x >= 0 && y >= 0)
			pages++;
	}
	pages -= 8;
	if (pages < 1)
		pages = 1;
	return pages;
}
static inline s16 PCShopEntryId(s32 index)
{
	return gShopCatalogPage.mRows[index].mItem;
}

/**
 * @note Address: 0x8004AB40
 * @note Size: 0xDE8
 */
void MenuPC::State7(u32 frames)
{
	MenuPC* menu = this;
	s32 count;
	CPad pad(0);
	MenuPCItem* table = sShopCatalogEntries;
	count             = CountPCShopItems(table);
	ApproachHudValue(&menu->mPanelAlpha, &menu->mPanelTarget, 0.08f * frames);
	switch (menu->mPhase) {
	case 0:
		if (count > 1) {
			if (menu->mShopSelection == 0) {
				if (pad.mPad->mRepeat & 0x20004) {
					s32 next = menu->mShopSelection + 1;
					if (next <= 19) {
						menu->mShopSelection = next;
						PlayGameSound(&gGame.mSound, 0x87, 0xFF);
					}
				}
				if (pad.mPad->mRepeat & 0x10008) {
					CountPCShopItems(table);
					if (gShopCatalogPage.mStart > 0) {
						menu->mScrollTarget = 17.0f;
						menu->mPhase        = 20;
						break;
					}
				}
			} else if (menu->mShopSelection == 8) {
				if (pad.mPad->mRepeat & 0x10008) {
					if (--menu->mShopSelection < 0)
						menu->mShopSelection = 19;
					PlayGameSound(&gGame.mSound, 0x87, 0xFF);
				}
				if (pad.mPad->mRepeat & 0x20004) {
					if (gShopCatalogPage.mStart < PCShopMaxPage(table) - 1) {
						menu->mScrollTarget = -17.0f;
						menu->mPhase        = 10;
						break;
					}
				}
			} else {
				if (pad.mPad->mRepeat & 0x10008) {
					if (--menu->mShopSelection < 0)
						menu->mShopSelection = 19;
					PlayGameSound(&gGame.mSound, 0x87, 0xFF);
				}
				if (pad.mPad->mRepeat & 0x20004) {
					s32 next = menu->mShopSelection + 1;
					if (next <= 19) {
						menu->mShopSelection = next;
						s16 id               = gShopCatalogPage.mRows[menu->mShopSelection].mItem;
						if (id >= 0)
							PlayGameSound(&gGame.mSound, 0x87, 0xFF);
					}
				}
			}
		}
		if (PCShopEntryId(menu->mShopSelection) < 0) {
			do {
				menu->mShopSelection--;
				s16 id = gShopCatalogPage.mRows[menu->mShopSelection].mItem;
				if (id >= 0)
					break;
			} while (menu->mShopSelection != 0);
		}
		if (pad.mPad->mTrigger & 0x100) {
			if (PCShopEntryId(menu->mShopSelection) >= 0 && gShopCatalogPage.mRows[menu->mShopSelection].mCount > 0) {
				PlayGameSound(&gGame.mSound, 0x88, 0xFF);
				menu->mPanelTarget = 1.0f;
				menu->mPhase++;
				menu->mYesNo = 0;
				break;
			}
			PlayGameSound(&gGame.mSound, 0x85, 0xFF);
			break;
		}
		if (pad.mPad->mTrigger & 0x200) {
			ResetPCText(menu, 2);
			PlayGameSound(&gGame.mSound, 0x86, 0xFF);
			menu->mPanelTarget = 0.0f;
		}
		break;
	case 1:
		if (pad.mPad->mRepeat & 0x10008) {
			menu->mYesNo--;
			if (menu->mYesNo < 0)
				menu->mYesNo = 1;
			PlayGameSound(&gGame.mSound, 0x87, 0xFF);
		} else if (pad.mPad->mRepeat & 0x20004) {
			menu->mYesNo++;
			if (menu->mYesNo > 1)
				menu->mYesNo = 0;
			PlayGameSound(&gGame.mSound, 0x87, 0xFF);
		} else if (pad.mPad->mTrigger & 0x100) {
			if (menu->mYesNo == 0) {
				MenuPCItem* entry;
				s32 item  = gShopCatalogPage.mIndex[menu->mShopSelection];
				s32 scrap = GetScrap();
				entry     = &sShopCatalogEntries[item];
				s16 price = entry->mPrice;
				if (price <= scrap) {
					scrap -= price;
					gGame.GetPlayer()->mParams.SetScrap(scrap);
					menu->mScrapDisplay.target = scrap;
					menu->mYesNo               = 2;
					PlayGameSound(&gGame.mSound, 0x30009, 0xFF);
					gEventVars.mVars[menu->mCatalogBase + entry->mItem] = 1;
					menu->mExitState                                    = item;
					s16& quantity                                       = entry->mCount;
					s16 value                                           = quantity;
					s32 remaining                                       = value;
					if (value > 0)
						remaining = value - 1;
					quantity = remaining;
					menu->mPhase++;
					return;
				}
				PlayGameSound(&gGame.mSound, 0x85, 0xFF);
			} else {
				menu->mPhase       = 0;
				menu->mPanelTarget = 0.0f;
				PlayGameSound(&gGame.mSound, 0x88, 0xFF);
				menu->mTargetListAlpha = 1.0f;
			}
		} else if (pad.mPad->mTrigger & 0x200) {
			PlayGameSound(&gGame.mSound, 0x86, 0xFF);
			menu->mPhase       = 0;
			menu->mPanelTarget = 0.0f;
		}
		break;
	case 2:
		if (pad.mPad->mTrigger & 0x300) {
			menu->mPhase       = 0;
			menu->mPanelTarget = 0.0f;
			PlayGameSound(&gGame.mSound, 0x88, 0xFF);
			PopulateShopCatalogPage(&gShopCatalogPage);
			ResetPCText(menu, 11);
			menu->mTargetFrameHeight = 0.0f;
			RestorePCHistory(menu);
		}
		break;
	case 10:
		if (menu->mScroll == menu->mScrollTarget) {
			if (gShopCatalogPage.mStart < ResetPCShopPage(menu, table) - 1) {
				gShopCatalogPage.mStart++;
				PopulateShopCatalogPage(&gShopCatalogPage);
				PlayGameSound(&gGame.mSound, 0x87, 0xFF);
			}
			menu->mScroll       = 0.0f;
			menu->mScrollTarget = 0.0f;
		}
		break;
	case 20:
		if (menu->mScroll == menu->mScrollTarget) {
			menu->mPhase = 0;
			CountPCShopItems(table);
			if (gShopCatalogPage.mStart > 0) {
				gShopCatalogPage.mStart--;
				PopulateShopCatalogPage(&gShopCatalogPage);
				PlayGameSound(&gGame.mSound, 0x87, 0xFF);
			}
			menu->mScroll       = 0.0f;
			menu->mScrollTarget = 0.0f;
		}
		break;
	}
}

static inline void PCSlide(f32& value, f32& target, f32 speed)
{
	if (value > target) {
		if ((value -= speed) < target)
			value = target;
	}
	if (value < target) {
		if ((value += speed) > target)
			value = target;
	}
}
static inline void PCEase(f32& value, f32& target)
{
	f32 speed = target - value;
	f32 rate  = 0.25f;
	speed     = speed * rate;
	if (speed < 0.0f)
		speed = -speed;
	if (speed < 1.0f)
		speed = 1.0f;
	PCSlide(value, target, speed);
}

static inline XObj* PCWorldObject(s32 index)
{
	return gGame.mXObjs.mPools[22].Get(index);
}
// where the object's first anim is and where it ends
static inline f32 PCAnimEnd(XObj* obj)
{
	HSD_AObj* aobj = GetAObj(obj->mJObj);
	return aobj ? HSD_AObjGetEndFrame(aobj) : 0.0f;
}
static inline f32 PCAnimFrame(XObj* obj)
{
	HSD_AObj* aobj = GetAObj(obj->mJObj);
	return aobj ? HSD_AObjGetCurrentFrame(aobj) : 0.0f;
}

/**
 * @note Address: 0x8004DD1C
 * @note Size: 0x830
 */
BOOL MenuPC::Update(u32 frames)
{
	if (CMenu::Update(frames) == 4) {
		PCEase(_198, _1A0);
		PCEase(mFrameHeight, mTargetFrameHeight);
		PCSlide(mTextX, mTargetX, 6.0f * frames);
		PCSlide(mTextY, mTargetY, 6.0f * frames);
		ApproachHudValue(&mScroll, &mScrollTarget, 17.0f);
		PCEaseCall(mMoneyDisplay.shown, mMoneyDisplay.target);
		PCEaseCall(mScrapDisplay.shown, mScrapDisplay.target);
		PCEaseCall(_424, _428);
		mTexScroll -= 0.008f * frames;
		if (mMode > 1) {
			switch (mTextPhase) {
			case 0:
				switch (mMode) {
				case 3:
				case 7:
				case 10:
					mTargetX = 140.0f;
					break;
				case 4:
					mTargetX = 160.0f;
					break;
				default:
					mTargetX = 180.0f;
					break;
				}
				mTextPhase++;
				break;
			case 1:
				if (mTextX == mTargetX) {
					mTextY   = 129.0f;
					mTargetY = 301.0f;
#if !defined(VERSION_JP)
					switch (mMode) {
					case 3:
					case 7:
					case 10:
						mTextY -= 8.0f;
						mTargetY -= 8.0f;
						break;
					}
#endif
					mTextPhase++;
				}
				break;
			case 2:
				if (mTextY == mTargetY) {
					switch (mMode) {
					case 3:
					case 7:
					case 10:
						mTextX   = 140.0f;
						mTargetX = 469.0f;
						break;
					case 4:
						mTextX   = 160.0f;
						mTargetX = 449.0f;
						break;
					default:
						mTextX   = 210.0f;
						mTargetX = 429.0f;
						break;
					}
					mTextPhase++;
				}
				break;
			case 3:
				if (mTextX == mTargetX)
					mTextPhase++;
				break;
			case 4:
				break;
			}
		}
		switch (mKeeperAnimStep) {
		case 0:
			mKeeperAnimId = 0;
			break;
		case 1:
			mKeeperAnimId = 1;
			break;
		case 2:
			mKeeperAnimId = 2;
			break;
		case 3:
			mKeeperAnimId = 6;
			break;
		case 4:
			mKeeperAnimId = 7;
			break;
		case 5:
			mKeeperAnimId = 8;
			break;
		case 6:
			mKeeperAnimId = 9;
			break;
		case 7:
			mKeeperAnimId = 2;
			break;
		case 8:
			mKeeperAnimId = 10;
			break;
		default:
			mKeeperAnimId = 0;
			break;
		}
		if (mPlayKeeperAnim == 1) {
			for (s32 i = 0; i < 288; i++) {
				XObj* obj = PCWorldObject(i);
				switch (obj->GetKind()) {
				case 120:
					obj->SetAnim(mKeeperAnimId, 0, 0, 0, 0, 0.0f, frames, 1.0f, 10.0f);
					break;
				}
			}
			mPlayKeeperAnim = 0;
		}
		switch (mKeeperAnimStep) {
		case 1:
		case 2:
		case 6:
		case 8:
			for (s32 i = 0; i < 288; i++) {
				XObj* obj = PCWorldObject(i);
				switch (obj->GetKind()) {
				case 120: {
					f32 end     = PCAnimEnd(obj);
					f32 current = PCAnimFrame(obj);
					if (end - 4.0f <= current) {
						mKeeperAnimStep++;
						mPlayKeeperAnim = 1;
					}
				} break;
				}
			}
			break;
		}
		(this->*gPCStepHandlers[mMode])(frames);
	}
	return IsOpen();
}

template <class MenuType>
static inline void UpdateMenuTitle(MenuType* menu, u32 frames)
{
	if (menu->mTitle.mActive && menu->mTitle.mActive > 0) {
		for (int i = 0; i < menu->mTitle.mCount; i++) {
			MenuTitleChar* glyph = &menu->mTitle.mChars[i];
			if (glyph->mHold > 0.0f) {
				if (glyph->mFade < 1.0f) {
					if ((glyph->mFade += (1.0f / (0.3f * FRAME_RATE)) * frames) > 1.0f) {
						glyph->mFade = 1.0f;
					}
				}
			} else if (glyph->mFade > 0.0f) {
				if ((glyph->mFade -= (1.0f / (0.3f * FRAME_RATE)) * frames) < 0.0f) {
					glyph->mFade = 0.0f;
				}
			}
		}
		s32 shown = 0;
		f32 spinX = 0.08726646f * frames;
		f32 spinY = 0.05235988f * frames;
		for (int i = 0; i < menu->mTitle.mCount; i++) {
			MenuTitleChar* glyph = &menu->mTitle.mChars[i];
			if (glyph->mFade != 0.0f) {
				shown++;
				glyph->mAngle0 += spinX;
				glyph->mAngle1 += spinY;
			}
		}
		if (shown == 0) {
			menu->mTitle.mActive = FALSE;
		}
	}
}

/**
 * @note Address: 0x8003CE18
 * @note Size: 0x538
 */
// NONMATCHING: regalloc, and retail reaches the card's busy flag in two steps
BOOL MenuSave::Update(u32 frames)
{
	if (CMenu::Update(frames) == 4) {
		PCEaseCall(mPanelY, mTargetPanelY);
		ApproachHudValue(&mDrawAlpha, &mTargetAlpha, 0.08f * frames);
		PCEaseCall(mBannerSlide, mOffset);
		mTexScroll -= 0.008f * frames;
		UpdateMenuTitle(this, frames);
		switch (mShowHelp) {
		case 0:
			break;
		case 1:
			mHelpAlpha = 0.0f;
			mShowHelp++;
			mHelpText = mNextHelpText;
			break;
		case 2:
			mHelpAlpha += 0.04f * frames;
			if (mHelpAlpha >= 1.0f) {
				mHelpAlpha = 1.0f;
				mShowHelp  = 0;
			}
			break;
		}
		if (mAlpha > 0.0f) {
			f32 spinX = 0.08726646f * frames;
			f32 spinY = 0.05235988f * frames;
			for (int i = 0; i < 20; i++) {
				mBg[i].mAngle0 += spinX;
				mBg[i].mAngle1 += spinY;
			}
		}
		if (mMode != 1 && mMode != 5 && mMode != 9) {
			s32 sector;
			s32 size;
			mReady       = FALSE;
			mWrongDevice = FALSE;
			switch (ProbeMemoryCardCapacity(&gGame.mMemcard.mCard, mChannel, &size, &sector)) {
			case CARD_RESULT_WRONGDEVICE:
				mWrongDevice = TRUE;
				break;
			case CARD_RESULT_READY:
				if (sector == 0x2000) {
					mReady = TRUE;
				}
				break;
			case CARD_RESULT_NOCARD:
			case CARD_RESULT_FATAL_ERROR:
				break;
			}
			if (!mReady) {
				mShowMessage = FALSE;
			}
		}
		if (gGame.mMemcard.mCard.mBusy != 0) {
			return IsOpen();
		}
		(this->*gSaveStepHandlers[mMode])(frames);
	}
	return IsOpen();
}

static inline f32 MenuTextCenter(f32 w, f32 factor, f32 center)
{
	return center - w * factor;
}
static inline void DrawMenuHelpLine(s32 id, f32 y, f32 size, u32 color)
{
	char* text = GetLocalizedInterfaceText(id);
	f32 width  = MeasureGameTextWidth(&gGame.mGs, text, 1, size);
	DrawGsProportionalTextFormat(&gGame.mGs, color, text, MenuTextCenter(width, 0.5f, 320.0f), y, 0.0f, size, size);
}

/**
 * @note Address: 0x800351F4
 * @note Size: 0x10D0
 */
extern "C" s32 DrawSaveStatusMessage(s32, s32 id, f32 x, f32 y, f32 size, f32 alpha)
{
	s32 lines = 0;
	u32 color = (u32)(255.0f * alpha);
	switch (id) {
	case 1:
#if defined(VERSION_PAL)
		if (gGame.mLanguage == 2 || gGame.mLanguage == 1) {
			DrawMenuHelpLine(0xad, y, size, color);
			DrawMenuHelpLine(0xae, (y + size), size, color);
			DrawMenuHelpLine(0xaf, (2.0f * size + y), size, color);
			lines = 3;
		} else
#endif
		{
			DrawMenuHelpLine(0xad, y, size, color);
			DrawMenuHelpLine(0xae, (y + size), size, color);
			lines = 2;
		}
		break;
	case 2:
		if (IsJapanese()) {
			DrawMenuHelpLine(0xdf, y, size, color);
			lines = 1;
#if defined(VERSION_PAL)
		} else if (gGame.mLanguage == 2 || gGame.mLanguage == 1) {
			DrawMenuHelpLine(0xdf, y, size, color);
			DrawMenuHelpLine(0xe0, (y + size), size, color);
			DrawMenuHelpLine(0xe1, (2.0f * size + y), size, color);
			lines = 3;
#endif
		} else {
			DrawMenuHelpLine(0xdf, y, size, color);
			DrawMenuHelpLine(0xe0, (y + size), size, color);
			lines = 2;
		}
		break;
	case 3:
#if defined(VERSION_PAL)
		if (gGame.mLanguage == 2 || gGame.mLanguage == 1) {
			DrawMenuHelpLine(0xb7, y, size, color);
			DrawMenuHelpLine(0xb8, (y + size), size, color);
			lines = 2;
		} else
#endif
		{
			DrawMenuHelpLine(0xb7, y, size, color);
			lines = 1;
		}
		break;
	case 8:
		if (IsJapanese()) {
			DrawMenuHelpLine(0x85, y, size, color);
			lines = 1;
		} else {
			DrawMenuHelpLine(0x85, y, size, color);
			DrawMenuHelpLine(0x86, (y + size), size, color);
			lines = 2;
		}
		break;
	case 0xe:
#if defined(VERSION_PAL)
		if (gGame.mLanguage == 1) {
			DrawMenuHelpLine(0xee, y, size, color);
			DrawMenuHelpLine(0xef, (y + size), size, color);
			lines = 2;
		} else
#endif
		{
			DrawMenuHelpLine(0xee, y, size, color);
			lines = 1;
		}
		break;
	case 4:
#if defined(VERSION_PAL)
		if (gGame.mLanguage == 1) {
			DrawMenuHelpLine(0xb2, y, size, color);
			DrawMenuHelpLine(0xb3, (y + size), size, color);
			DrawMenuHelpLine(0xb4, (2.0f * size + y), size, color);
			DrawMenuHelpLine(0xb5, (3.0f * size + y), size, color);
			lines = 4;
		} else
#endif
		{
			DrawMenuHelpLine(0xb2, y, size, color);
			DrawMenuHelpLine(0xb3, (y + size), size, color);
			DrawMenuHelpLine(0xb4, (2.0f * size + y), size, color);
			lines = 3;
		}
		break;
	case 5:
		DrawMenuHelpLine(0x76, y, size, color);
		DrawMenuHelpLine(0x77, (y + size), size, color);
		DrawMenuHelpLine(0x78, (2.0f * size + y), size, color);
		lines = 4;
		break;
	case 6:
#if defined(VERSION_PAL)
		if (gGame.mLanguage == 1) {
			DrawMenuHelpLine(0x7b, y, size, color);
			DrawMenuHelpLine(0x7c, (y + size), size, color);
			DrawMenuHelpLine(0x7d, (2.0f * size + y), size, color);
			DrawMenuHelpLine(0x7e, (3.0f * size + y), size, color);
			lines = 4;
		} else if (gGame.mLanguage == 2 || gGame.mLanguage == 3) {
			DrawMenuHelpLine(0x7b, y, size, color);
			DrawMenuHelpLine(0x7c, (y + size), size, color);
			DrawMenuHelpLine(0x7d, (2.0f * size + y), size, color);
			lines = 3;
		} else {
			DrawMenuHelpLine(0x7b, y, size, color);
			DrawMenuHelpLine(0x7c, (y + size), size, color);
			lines = 2;
		}
		lines = lines + 1;
#else
		DrawMenuHelpLine(0x7b, y, size, color);
		DrawMenuHelpLine(0x7c, (y + size), size, color);
		lines = 3;
#endif
		break;
	case 7:
		DrawMenuHelpLine(0x80, y, size, color);
		DrawMenuHelpLine(0x81, (y + size), size, color);
		DrawMenuHelpLine(0x82, (2.0f * size + y), size, color);
		lines = 3;
		break;
	case 9:
		if (IsJapanese()) {
			DrawMenuHelpLine(0x8a, y, size, color);
			lines = 1;
		} else {
			DrawMenuHelpLine(0x8a, y, size, color);
			DrawMenuHelpLine(0x8b, (y + size), size, color);
			lines = 2;
		}
		break;
	case 10:
		if (IsJapanese()) {
			DrawMenuHelpLine(0x8f, y, size, color);
			DrawMenuHelpLine(0x90, (y + size), size, color);
			lines = 2;
		} else {
			DrawMenuHelpLine(0x8f, y, size, color);
			lines = 1;
		}
		lines = lines + 1;
		break;
	case 0xb:
		DrawMenuHelpLine(0xcb, y, size, color);
		DrawMenuHelpLine(0xcc, (y + size), size, color);
		lines = 3;
		break;
	case 0x10:
		DrawMenuHelpLine(0x71, y, size, color);
		DrawMenuHelpLine(0x72, (y + size), size, color);
		lines = 2;
		break;
	case 0xc:
#if defined(VERSION_PAL)
		if (gGame.mLanguage == 2 || gGame.mLanguage == 1) {
			DrawMenuHelpLine(0xf8, y, size, color);
			DrawMenuHelpLine(0xf9, (y + size), size, color);
			DrawMenuHelpLine(0xfa, (2.0f * size + y), size, color);
			DrawMenuHelpLine(0xfb, (3.0f * size + y), size, color);
			DrawMenuHelpLine(0xfc, (4.0f * size + y), size, color);
			lines = 5;
		} else
#endif
		{
			DrawMenuHelpLine(0xf8, y, size, color);
			DrawMenuHelpLine(0xf9, (y + size), size, color);
			DrawMenuHelpLine(0xfa, (2.0f * size + y), size, color);
			DrawMenuHelpLine(0xfb, (3.0f * size + y), size, color);
			lines = 4;
		}
		break;
	case 0x11:
#if defined(VERSION_PAL)
		if (gGame.mLanguage == 2) {
			DrawMenuHelpLine(0xfd, y, size, color);
			DrawMenuHelpLine(0xfe, (y + size), size, color);
			DrawMenuHelpLine(0xff, (2.0f * size + y), size, color);
			DrawMenuHelpLine(0x100, (3.0f * size + y), size, color);
			DrawMenuHelpLine(0x101, (4.0f * size + y), size, color);
			lines = 5;
		} else
#endif
		{
			DrawMenuHelpLine(0xfd, y, size, color);
			DrawMenuHelpLine(0xfe, (y + size), size, color);
			DrawMenuHelpLine(0xff, (2.0f * size + y), size, color);
			DrawMenuHelpLine(0x100, (3.0f * size + y), size, color);
			lines = 4;
		}
		break;
	case 0xd:
		DrawMenuHelpLine(0xda, y, size, color);
		DrawMenuHelpLine(0xdb, (y + size), size, color);
		DrawMenuHelpLine(0xdc, (2.0f * size + y), size, color);
		DrawMenuHelpLine(0xdd, (3.0f * size + y), size, color);
		DrawMenuHelpLine(0xde, (4.0f * size + y), size, color);
		lines = 5;
	}
	return lines;
}

/**
 * @note Address: 0x800362C4
 * @note Size: 0xAA8
 */
extern "C" s32 DrawCardErrorMessage(s32, s32 id, u8 retry, f32 x, f32 y, f32 size, f32 alpha)
{
	s32 lines = 0;
	u32 color = (u32)(255.0f * alpha);
	switch (id) {
	case -128:
	case -14:
	case -12:
	case -11:
	case -10:
	case -8:
	case -7:
	case -4:
	case -1:
	case 0:
		if (IsJapanese()) {
			DrawMenuHelpLine(0xC6, y, size, color);
			lines = 1;
		} else {
			DrawMenuHelpLine(0xC6, y, size, color);
			DrawMenuHelpLine(0xC7, y + size, size, color);
			DrawMenuHelpLine(0xC8, 2.0f * size + y, size, color);
			lines = 3;
		}
		break;
	case -5:
		DrawMenuHelpLine(0xF3, y, size, color);
		DrawMenuHelpLine(0xF4, y + size, size, color);
		lines = 2;
		break;
	case -3: {
		s32 n;
		if (IsJapanese()) {
			DrawMenuHelpLine(0x67, y, size, color);
			DrawMenuHelpLine(0x68, y + (2.0f + size), size, color);
			DrawMenuHelpLine(0x69, 2.0f * (2.0f + size) + y, size, color);
			n = 3;
		} else {
			DrawMenuHelpLine(0x67, y, size, color);
			DrawMenuHelpLine(0x68, y + (2.0f + size), size, color);
			DrawMenuHelpLine(0x69, 2.0f * (2.0f + size) + y, size, color);
			DrawMenuHelpLine(0x6A, 3.0f * (2.0f + size) + y, size, color);
			n = 4;
		}
		lines = n;
		break;
	}
	case -9:
		DrawMenuHelpLine(0xA3, y, size, color);
		DrawMenuHelpLine(0xA4, y + size, size, color);
		DrawMenuHelpLine(0xA5, 2.0f * size + y, size, color);
		DrawMenuHelpLine(0xA6, 3.0f * size + y, size, color);
		lines = 4;
		break;
	case -2:
		if (retry) {

			if (IsJapanese()) {
				DrawMenuHelpLine(0x6C, y, size, color);
				lines = 1;
#if defined(VERSION_PAL)
			} else if (gGame.mLanguage == 1) {
				DrawMenuHelpLine(0x6C, y, size, color);
				DrawMenuHelpLine(0x6D, y + size, size, color);
				DrawMenuHelpLine(0x6E, 2.0f * size + y, size, color);
				DrawMenuHelpLine(0x6F, 3.0f * size + y, size, color);
				DrawMenuHelpLine(0x70, 4.0f * size + y, size, color);
				lines = 5;
#endif
			} else {
				DrawMenuHelpLine(0x6C, y, size, color);
				DrawMenuHelpLine(0x6D, y + size, size, color);
				DrawMenuHelpLine(0x6E, 2.0f * size + y, size, color);
				DrawMenuHelpLine(0x6F, 3.0f * size + y, size, color);
				lines = 4;
			}
#if defined(VERSION_PAL)
		} else if (gGame.mLanguage == 1) {
			DrawMenuHelpLine(0xD0, y, size, color);
			DrawMenuHelpLine(0xD1, y + size, size, color);
			DrawMenuHelpLine(0xD2, 2.0f * size + y, size, color);
			DrawMenuHelpLine(0xD3, 3.0f * size + y, size, color);
			DrawMenuHelpLine(0xD4, 4.0f * size + y, size, color);
			lines = 5;
#endif
		} else {
			DrawMenuHelpLine(0xD0, y, size, color);
			DrawMenuHelpLine(0xD1, y + size, size, color);
			DrawMenuHelpLine(0xD2, 2.0f * size + y, size, color);
			DrawMenuHelpLine(0xD3, 3.0f * size + y, size, color);
			lines = 4;
		}
		break;

	case -13:
	case -6:
		break;
	}
	return lines;
}

#if defined(VERSION_JP)
// These menu entry points only exist in the Japanese edition.
extern "C" u32 GetPCDialogueNumber(MenuPC* menu)
{
	return menu->mDialogueNumber;
}

void MenuRequestClear::Init()
{
	CMenu::Init();
}

void MenuPadControl::Init()
{
	CMenu::Init();
}

void MenuRequest::Init()
{
	CMenu::Init();
}

BOOL MenuPadControl::Update(u32 frames)
{
	if (CMenu::Update(frames) == 4) {
		CPad pad(0);
		if (pad.mPad->mTrigger & 0x200) {
			Close();
			sMenuMain.Resume();
			PlayGameSound(&gGame.mSound, 0x86, 0xFF);
		}
	}
	return IsOpen();
}

extern "C" s32 CountDecimalDigits(s32 value)
{
	s32 digits = 1;
	while (true) {
		f32 divisor = 1.0f;
		for (s32 i = 0; i < digits; i++)
			divisor *= 10.0f;
		if ((s32)(value / divisor) == 0)
			break;
		digits++;
	}
	return digits;
}

#endif

/**
 * @note Address: 0x80051CBC
 * @note Size: 0x44C
 */
extern "C" void DrawPlayerStatusIcon(s32 id, u8 available, bool selected, f32 x, f32 y, f32 alpha, f32 scale, f32 phase)
{
	if (alpha > 0.0f) {
		SetGsBlendMode(gGame.mGs.mCurRenderMode, 1, 4, 5, 15);
		SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 7, 0);
		f32 width;
		CTexObj tex;
		u32 color = (u32)(255.0f * alpha) | 0xFFFFFF00;
		if (!available)
			id = UnavailableStatusIcon;
		f32 bob = 0.0f;
		if (selected) {
			static s32 frame = 0;
			frame++;
			bob = 4.0f * (f32)sin(0.017453292f * ((frame * 20) % 360));
		}
		tex        = (&gGame.mTpl)->mTex.Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[id], 0);
		width      = scale * GXGetTexObjWidth(&tex);
		f32 height = scale * GXGetTexObjHeight(&tex);
		SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 7, 1);
		SetGsDepthCompareBeforeTexture(gGame.mGs.mCurRenderMode, 0);
		SetGsTevMode(gGame.mGs.mCurTevStage, 0);
		y += bob;
		DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, x, y, 1.0f, width, height, 0.0f, 0.0f, 0.0f);
		SetGsTevMode(gGame.mGs.mCurTevStage, 4);
		SetGsDepthCompareBeforeTexture(gGame.mGs.mCurRenderMode, 1);
		if (selected) {
			SetGsBlendMode(gGame.mGs.mCurRenderMode, 1, 2, 4, 15);
			SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 2, 0);
			f32 pulse = alpha * (0.2f * (f32)sin(3.1415927f * phase) + 0.2f);
			if (pulse < 0.0f)
				pulse = 0.0f;
			u32 highlight = (u32)(255.0f * pulse) | 0xFFFFFF00;
			DrawGsQuad(&gGame.mGs, highlight, highlight, highlight, highlight, x, y, 1.0f, width, height, 0.0f, 0.0f, 0.0f);
			SetGsBlendMode(gGame.mGs.mCurRenderMode, 1, 4, 5, 15);
			SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 7, 0);
		}
	}
}

/**
 * @note Address: 0x80057318
 * @note Size: 0x424
 */
// NONMATCHING: register allocation
extern "C" void DrawBeveledMenuRect(u32 color, f32 x, f32 y, f32 z, f32 width, f32 height, f32 alpha)
{
	f32 left    = x - width / 2.0f;
	f32 top     = y - height / 2.0f;
	f32 right   = width + left;
	f32 bottom  = height + top;
	Vec fill[5] = { (Vec) { left, top, z }, (Vec) { right, top, z }, (Vec) { right, bottom, z }, (Vec) { left, bottom, z },
		            (Vec) { left, top, z } };
	SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 7, 1);
	DrawGsTrianglePrimitive(&gGame.mGs, GX_TRIANGLEFAN, gGame.mGs.mCamera->mViewMtx, fill, color, 5);
	Vec border[6] = { (Vec) { left, top, 0.0f },    (Vec) { right, top, 0.0f }, (Vec) { right, bottom, 0.0f },
		              (Vec) { left, bottom, 0.0f }, (Vec) { left, top, 0.0f },  (Vec) { right, top, 0.0f } };
	u32 opacity   = (u32)(255.0f * alpha);
	DrawGsLineStrip(&gGame.mGs, &border[3], opacity | 0x64646400, 3);
	DrawGsLineStrip(&gGame.mGs, &border[1], opacity | 0xC8C8C800, 3);
}

static inline f32 MenuRectStart(f32 center, f32 size)
{
	return center - size / 2;
}

static inline f32 MenuRectEnd(f32 center, f32 size)
{
	return size + MenuRectStart(center, size);
}

/**
 * @note Address: 0x80056CB8
 * @note Size: 0x3CC
 */
// NONMATCHING: temporary vector lifetimes and register allocation
extern "C" void DrawMenuRectWithShadow(CMenu* menu, u32 color, f32 x, f32 y, f32 width, f32 height)
{
	// The original evaluates both animation curves even though their values are unused.
	sin(4.712389f * (1.0f - menu->mAlpha));
	sin(4.712389f * (1.0f - menu->mAlpha));
	Vec outline[10] = { (Vec) { MenuRectStart(x, width), MenuRectStart(y, height), -10.0f },
		                (Vec) { MenuRectEnd(x, width), MenuRectStart(y, height), -10.0f },
		                (Vec) { MenuRectEnd(x, width), MenuRectEnd(y, height), -10.0f },
		                (Vec) { MenuRectStart(x, width), MenuRectEnd(y, height), -10.0f },
		                (Vec) { MenuRectStart(x, width), MenuRectStart(y, height), -10.0f },
		                (Vec) { 6.0f + MenuRectStart(x, width), 6.0f + MenuRectStart(y, height), -10.0f },
		                (Vec) { 6.0f + MenuRectEnd(x, width), 6.0f + MenuRectStart(y, height), -10.0f },
		                (Vec) { 6.0f + MenuRectEnd(x, width), 6.0f + MenuRectEnd(y, height), -10.0f },
		                (Vec) { 6.0f + MenuRectStart(x, width), 6.0f + MenuRectEnd(y, height), -10.0f },
		                (Vec) { 6.0f + MenuRectStart(x, width), 6.0f + MenuRectStart(y, height), -10.0f } };
	SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 7, 1);
	DrawGsTrianglePrimitive(&gGame.mGs, GX_TRIANGLEFAN, gGame.mGs.mCamera->mViewMtx, outline, color, 5);
	SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 5, 0);
	u32 shadow = (u32)(128.0f * menu->mAlpha);
	DrawGsTrianglePrimitive(&gGame.mGs, GX_TRIANGLEFAN, gGame.mGs.mCamera->mViewMtx, &outline[5], shadow, 5);
	SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 3, 0);
	SetGsLineWidth(gGame.mGs.mCurRenderMode, 12, 5);
	SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 7, 0);
}

extern "C" void DrawStatusCallout(MenuStatus* menu, s32 id, f32 x, f32 y, f32 targetX, f32 targetY);

static inline f32 StatusLabelColumn(f32 center, bool right)
{
	if (right)
		center += 42.0f;
	else
		center -= 42.0f;
	return center;
}

/**
 * @note Address: 0x800554A8
 * @note Size: 0x51C
 */
// NONMATCHING: coordinate lifetimes and register allocation
extern "C" void DrawStatusDiagram(MenuStatus* menu)
{
	CTexObj tex;
	if (menu->mMode == 5 || menu->mMode == 4) {
		f32 y;
		f32 x = menu->mDiagramX + menu->mOffsetX;
		y     = menu->mDiagramY + menu->mOffsetY;
		tex   = (&gGame.mTpl)->mTex.Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[181], 0);
		SetGsTevMode(gGame.mGs.mCurTevStage, 0);
		u16 width  = GXGetTexObjWidth(&tex);
		u16 height = GXGetTexObjHeight(&tex);
		f32 labelX = x - 182.0f;
		f32 labelY = 210.0f + y;
		u32 color  = (u32)(255.0f * menu->mAlpha) | 0xFFFFFF00;
		DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, labelX, labelY, 0.0f, 1.2f * (1.1f * width), 1.2f * height, 0.0f,
		                   0.0f, 0.0f);
		SetGsTevMode(gGame.mGs.mCurTevStage, 4);
		x           = 42.0f + x;
		y           = 84.0f + y;
		labelX      = x - 42.0f;
		labelY      = y - 12.0f;
		f32 targetY = 114.0f + y;
		f32 columnX = StatusLabelColumn(labelX, false);
		DrawStatusCallout(menu, 7, columnX, labelY, 42.0f + ((labelX - 200.0f) - 100.0f), targetY - 7.0f);
		columnX = StatusLabelColumn(labelX, true);
		labelY += 22.0f;
		DrawStatusCallout(menu, 8, columnX, labelY, 42.0f + ((labelX - 200.0f) - 6.0f), targetY - 56.0f);
		columnX = StatusLabelColumn(labelX, false);
		labelY += 22.0f;
		DrawStatusCallout(menu, 6, columnX, labelY, 42.0f + (9.0f + (labelX - 200.0f)), targetY - 46.0f);
		y       = targetY - 27.0f;
		columnX = StatusLabelColumn(labelX, true);
		labelY += 22.0f;
		DrawStatusCallout(menu, 5, columnX, labelY, 42.0f + ((labelX - 200.0f) - 2.0f), y);
		columnX = StatusLabelColumn(labelX, false);
		labelY += 22.0f;
		DrawStatusCallout(menu, 4, columnX, labelY, 42.0f + (27.0f + (labelX - 200.0f)), y);
		columnX = StatusLabelColumn(labelX, true);
		labelY += 22.0f;
		DrawStatusCallout(menu, 1, columnX, labelY, 42.0f + (12.0f + (labelX - 200.0f)), targetY - 15.0f);
		columnX = StatusLabelColumn(labelX, false);
		labelY += 22.0f;
		DrawStatusCallout(menu, 3, columnX, labelY, 42.0f + (6.0f + (labelX - 200.0f)), 19.0f + targetY);
		columnX = StatusLabelColumn(labelX, true);
		labelY += 22.0f;
		DrawStatusCallout(menu, 2, columnX, labelY, 42.0f + (labelX - 200.0f), targetY);
		columnX = StatusLabelColumn(labelX, false);
		labelY += 22.0f;
		DrawStatusCallout(menu, 9, columnX, labelY, 42.0f + (6.0f + ((labelX - 200.0f) - 33.0f)), 6.0f + (1.0f + targetY));
		labelY += 22.0f;
		x       = (labelX - 200.0f) - 75.0f;
		y       = 29.0f + targetY;
		columnX = StatusLabelColumn(labelX, true);
		DrawStatusCallout(menu, 10, columnX, labelY, 42.0f + (36.0f + x), 16.0f + y);
		labelY += 22.0f;
		DrawStatusCallout(menu, 0, 6.0f + (labelX - 126.0f), labelY, 42.0f + x, y);
	}
}

extern "C" u8* GetItemDescription(s32 id);
extern "C" s32 GetItemIconTextureIndex(s32 id);
extern "C" void DrawTexturedWindowFrame(u32 color, u32 textureColor, f32 x, f32 y, f32 width, f32 height, f32 alpha, f32 scaleX,
                                        f32 scaleY);

/**
 * @note Address: 0x8005CD3C
 * @note Size: 0x40C
 */
// NONMATCHING: register allocation and local static initialization
extern "C" void DrawStatusItemDetails(s32 id, f32 x, f32 y, f32 alpha, f32 scale)
{
	CTexObj tex;
	if (scale <= 0.0f)
		return;
	f32 height  = 153.0f * scale;
	height      = 10.0f * scale + height;
	y           = y - height / 2.0f;
	u32 opacity = (u32)(255.0f * alpha);
	u32 color   = opacity | 0xFFFFFF00;
	DrawTexturedWindowFrame((u32)(220.0f * alpha), color, x, y, 254.0f * scale, height, alpha, 1.0f, 1.0f);
	y += scale;
	f32 fontSize   = 17.0f * scale;
	f32 textOffset = 24.0f * scale;
	char text[256];
	sprintf(text, "%s", GetItemDescription(id));
	f32 width   = MeasureGameTextWidth(&gGame.mGs, text, 1, fontSize);
	f32 padding = 6.0f * scale;
	DrawGsProportionalTextFormat(&gGame.mGs, color, text, x - width / 2.0f, (4.0f * scale + (textOffset + (5.0f * fontSize + y))) - padding,
	                             0.0f, fontSize, fontSize);
	sprintf(text, "%s", GetItemName(id));
	f32 helpSize = 2.0f * scale + fontSize;
	width        = MeasureGameTextWidth(&gGame.mGs, text, 1, helpSize);
	DrawGsProportionalTextFormat(&gGame.mGs, opacity | 0xFFAA0000, text, x - width / 2.0f, ((4.0f * fontSize + y) + textOffset) - padding,
	                             0.0f, helpSize, helpSize);
	s32 icon       = GetItemIconTextureIndex(id);
	tex            = (&gGame.mTpl)->mTex.Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[icon], 0);
	f32 iconWidth  = scale * GXGetTexObjWidth(&tex);
	f32 iconHeight = scale * GXGetTexObjHeight(&tex);
	if (icon >= 0) {
		SetGsTevMode(gGame.mGs.mCurTevStage, 0);
		DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, ((56.0f * scale + x) - 30.0f * scale) - 127.0f * scale,
		                   46.0f * scale + y, 0.0f, iconWidth, iconHeight, 0.0f, 0.0f, 0.0f);
		SetGsTevMode(gGame.mGs.mCurTevStage, 4);
	}
}

/**
 * @note Address: 0x80053714
 * @note Size: 0x644
 */
// NONMATCHING: register allocation
extern "C" void DrawStatusHelpText(MenuStatus* menu)
{
	f32 alpha   = menu->mAlpha * menu->mHelpAlpha;
	f32 spacing = 30.0f;
#if !defined(VERSION_JP)
	spacing *= 0.9f;
#endif
	SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 7, 1);
	SetGsTevMode(gGame.mGs.mCurTevStage, 0);
	CTexObj blob     = (&gGame.mTpl)->mTex.Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[109 - StatusTextureOffset], 0);
	menu->mHelpCount = 0;
	const char* text = menu->mHelpText;
	if (text) {
#if defined(VERSION_JP)
		const int limit = 20;
#else
		const int limit = 22;
#endif
		for (int i = 0; i < limit && *text; i++, text++) {
			menu->mHelpChars[i].mText = text;
			menu->mHelpCount++;
			if (IsJapanese() && (IsSjisLead(*text) & 1))
				text++;
		}
	}
	f32 zero    = 0.0f;
	f32 opacity = 255.0f * alpha;
	for (int i = 0; i < menu->mHelpCount; i++) {
		MenuHelpChar* glyph = &menu->mHelpChars[i];
#if !defined(VERSION_JP)
		if (strncmp(glyph->mText, " ", 1) != 0)
#endif
		{
			CFont font;
			s32 width;
			CTexObj tex  = *GetFontGlyphTexture(&font, glyph->mText, &width);
			f32 bob      = 2.0f * (f32)sin(glyph->mAngle0);
			f32 rotation = sin(0.7f * glyph->mAngle1);
			f32 s1       = sin(3.1415927f * alpha);
			f32 s2       = sin(1.5707964f * alpha);
			f32 scale    = 0.2f * s1 + s2;
			f32 x        = 0.5f * spacing + (i * spacing + (320.0f - menu->mHelpCount * spacing * 0.5f));
			f32 y        = 384.0f + bob;
			rotation     = -(0.017453292f * (10.0f * rotation));
			u32 color    = (u32)opacity | 0xFFFFFF00;
			DrawGsTexturedQuad(&gGame.mGs, &blob, color, color, color, color, x, y, 1.0f, 30.0f * scale, 30.0f * scale, 0.0f, -zero,
			                   rotation);
			scale       = 30.0f * (0.8f * scale);
			f32 padding = HelpGlyphPadding(width, scale);
			DrawHelpLetter(tex, x, y, bob, scale, padding, rotation, zero, opacity, gStatusHelpOutlineX, gStatusHelpOutlineY);
		}
	}
	SetGsTevMode(gGame.mGs.mCurTevStage, 4);
}

/**
 * @note Address: 0x80053D58
 * @note Size: 0x5BC
 */
// NONMATCHING: register allocation
extern "C" void DrawStatusMenuTitle(MenuStatus* menu)
{
	if (menu->mTitle.mActive) {
		SetGsBlendMode(gGame.mGs.mCurRenderMode, 1, 4, 5, 15);
		SetGsChannelEnabled(&gGame.mGs, 0, 0);
		SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 7, 0);
		SetGsTevMode(gGame.mGs.mCurTevStage, 0);
		CTexObj blob = (&gGame.mTpl)->mTex.Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[MenuTitleBlobTexture], 0);
		u32 alpha;
		f32 t;
		f32 s0;
		f32 bob;
		f32 rot;
		f32 s1;
		f32 s2;
		f32 scale;
		f32 x;
		f32 y;
		f32 pad;
		u32 color = menu->mTitle.mColor & 0xFFFFFF00;
		f32 zero  = 0.0f;
		for (int i = 0; i < menu->mTitle.mCount; i++) {
			MenuTitleChar* glyph = &menu->mTitle.mChars[i];
			t                    = glyph->mFade;
			if (!(t <= 0.0f)
#if !defined(VERSION_JP)
			    && strncmp(glyph->mText, " ", 1) != 0
#endif
			) {
				CFont font;
				s32 width;
				CTexObj tex = *GetFontGlyphTexture(&font, glyph->mText, &width);
				s0          = sin(glyph->mAngle0);
				bob         = 4.0f * s0;
				rot         = sin(0.7f * glyph->mAngle1);
				s1          = sin(3.1415927f * t);
				s2          = sin(1.5707964f * t);
				scale       = 0.2f * s1 + s2;
				x           = 24.0f + (48.0f * i + (320.0f - 48.0f * menu->mTitle.mCount * 0.5f));
				alpha       = 255.0f * t;
				rot         = -(0.017453292f * (10.0f * rot));
				y           = 64.0f + bob;
				u32 c       = alpha | 0xFFFFFF00;
				DrawGsTexturedQuad(&gGame.mGs, &blob, c, c, c, c, x, y, 0.0f, 20.0f * (2.0f * scale), 20.0f * (2.0f * scale), 0.0f, -zero,
				                   rot);
				scale = 20.0f * (1.7f * scale);
				pad   = (width / 24.0f) * scale;
				pad   = scale - pad;
				for (int j = 0; j < 4; j++) {
					DrawGsTexturedQuad(&gGame.mGs, &tex, alpha, alpha, alpha, alpha, pad / 2 + (x + gStatusTitleOutlineX[j]),
					                   bob + (64.0f + gStatusTitleOutlineY[j]), 0.0f, scale, scale, 0.0f, -zero, rot);
				}
				c = color | alpha;
				DrawGsTexturedQuad(&gGame.mGs, &tex, c, c, c, c, pad / 2 + x, y, 0.0f, scale, scale, 0.0f, -zero, rot);
			}
		}
		SetGsTevMode(gGame.mGs.mCurTevStage, 4);
		SetGsDepthMode(gGame.mGs.mCurRenderMode, 0, 7, 0);
		SetGsChannelEnabled(&gGame.mGs, 0, 0);
	}
}

extern "C" const char* GetStageFileName(s32 stage);
extern "C" const char* GetStageName(s32 stage);
extern "C" char* GetStageDesc(s32 stage);

static inline void FinishStageLabel(char* text)
{
	for (int i = 0; text[i]; i++) {
		if (IsJapanese() && (IsSjisLead(text[i]) & 1)) {
			if (text[++i] == 0) {
				text[i - 1] = '.';
				break;
			}
		}
	}
}

static inline int CopyStageFileName(char* dst, const char* name)
{
	int count = 0;
	for (; *name; name++, count++) {
		char c   = *name;
		u8 lower = c >= 'a' && c <= 'z';
		if (lower)
			c -= 'a' - 'A';
		dst[count] = c;
	}
	return count;
}

static inline void MeasureStageLabels(MenuStageSelect* menu)
{
	menu->mRowCount = menu->mCount;
	menu->mMaxWidth = 0.0f;
	menu->mFontSize = 16.0f;
	for (int i = 0; i < menu->mRowCount; i++) {
		menu->mLabelWidth[i] = MeasureGameTextWidth(&gGame.mGs, menu->mLabels[i], 0, menu->mFontSize);
		if (menu->mLabelWidth[i] > menu->mMaxWidth)
			menu->mMaxWidth = menu->mLabelWidth[i];
	}
}

/**
 * @note Address: 0x80033A68
 * @note Size: 0x6A0
 */
// NONMATCHING: register allocation
extern "C" void RebuildStageSelectList(MenuStageSelect* menu)
{
	menu->mCount = 0;
	for (int i = 0; i < 100; i++) {
		menu->mLabels[i][0]  = 0;
		menu->mLabelWidth[i] = 0.0f;
	}
	if (gGame.mStatus.mFlags & 0x0008000000000000ULL) {
		for (s32* stage = sStageOrder; *stage >= 0; stage++) {
			char* line         = menu->mLabels[menu->mCount];
			int length         = CopyStageFileName(line, GetStageFileName(*stage));
			const char* format = IsJapanese() ? "(%2d) %-.20s" : "(%2d) %-.30s";
			sprintf(line + length, format, *stage, FormatToStaticBuffer("%s%s", GetStageName(*stage), GetStageDesc(*stage)));
			FinishStageLabel(line + length);
			menu->mCount++;
		}
		MeasureStageLabels(menu);
		menu->mWindow.SetSize(60.0f + menu->mMaxWidth, (2.0f + menu->mFontSize) * menu->mRowCount + 24.0f);
		menu->mWindow.SetPos(24.0f + (menu->mWindow.mWidth * 0.5f + 16.0f), 24.0f + (menu->mWindow.mHeight * 0.5f + 30.0f));
	} else {
		menu->mCount = gGame._344C;
		for (int i = 0; i < gGame._344C; i++) {
			char* line = menu->mLabels[i];
			int length = CopyStageFileName(line, GetStageFileName(i));
			sprintf(line + length, "(%2d) %-.18s", i, FormatToStaticBuffer("%s%s", GetStageName(i), GetStageDesc(i)));
			FinishStageLabel(line + length);
		}
		strcpy(menu->mLabels[menu->mCount], "\x01");
		menu->mCount++;
		for (int i = 100; GetStageFileName(i); i++) {
			sprintf(menu->mLabels[menu->mCount], "%-8s(%2d)", GetStageFileName(i), i);
			menu->mCount++;
		}
		MeasureStageLabels(menu);
		menu->mWindow.SetSize(2.0f * (60.0f + menu->mMaxWidth), 20.0f * (2.0f + menu->mFontSize) + 24.0f);
		menu->mWindow.SetPos(menu->mWindow.mWidth * 0.5f + 16.0f, menu->mWindow.mHeight * 0.5f + 30.0f);
	}
}

static inline u32 StageRowBlue(int row, u8 normal, u8 current)
{
	if (gGame.mStatus.mFlags & 0x0008000000000000ULL)
		return (u8)(sStageOrder[row] == gGame.mStage ? current : normal);
	return (u8)(row == gGame.mStage ? current : normal);
}

/**
 * @note Address: 0x80033128
 * @note Size: 0x5F8
 */
// NONMATCHING: register allocation and loop scheduling
BOOL MenuStageSelect::Draw()
{
	if (CMenu::Draw()) {
		f32 cx       = mWindow.mX;
		f32 cy       = mWindow.mY;
		f32 maxWidth = mMaxWidth;
		f32 fontSize = mFontSize;
		f32 step     = 2.0f + fontSize;
		f32 x        = 30.0f + (cx - mWindow.mWidth / 2);
		for (int column = 0; column < 2; column++) {
			f32 y = 12.0f + (cy - mWindow.mHeight / 2);
			for (int row = 0; row < 20; row++) {
				int index = column * 20 + row;
				if (index > mCount)
					break;
				u32 color;
				if (index == mSelection)
					color = (0x20200000 | (StageRowBlue(index, 0x20, 0x80) << 8)) | (u32)(255.0f * mAlpha);
				else
					color = (0x64640000 | (StageRowBlue(index, 0x64, 0xC8) << 8)) | (u32)(255.0f * mAlpha);
				if (mLabels[index][0] == *"\x01") {

					u32 shadow = (u32)(255.0f * mAlpha) | 0x64646400;
					DrawGsLineSegment(&gGame.mGs, NULL, (x - 15.0f), (fontSize / 2 + y), 0.0f, shadow, ((x - 15.0f) + (30.0f + maxWidth)),
					                  (fontSize / 2 + y), 0.0f, shadow);
					u32 light = (u32)(255.0f * mAlpha) | 0xC8C8C800;
					DrawGsLineSegment(&gGame.mGs, NULL, 1.0f + (x - 15.0f), 1.0f + (fontSize / 2 + y), 0.0f, light,
					                  1.0f + ((x - 15.0f) + (30.0f + maxWidth)), 1.0f + (fontSize / 2 + y), 0.0f, light);
				} else {
					DrawGsFixedWidthTextFormat(&gGame.mGs, color, mLabels[index], x, y, 0.0f, fontSize, fontSize);
				}
				y += step;
			}
			x += 60.0f + mMaxWidth;
		}
		if (!(gGame.mStatus.mFlags & 0x0008000000000000ULL)) {
			f32 top    = 12.0f + (cy - mWindow.mHeight / 2);
			u32 shadow = (u32)(255.0f * mAlpha) | 0x64646400;
			f32 bottom = 20.0f * step + top;
			DrawGsLineSegment(&gGame.mGs, NULL, cx, top, 0.0f, shadow, cx, bottom, 0.0f, shadow);
			u32 light = (u32)(255.0f * mAlpha) | 0xC8C8C800;
			DrawGsLineSegment(&gGame.mGs, NULL, 1.0f + cx, 1.0f + top, 0.0f, light, 1.0f + cx, 1.0f + bottom, 0.0f, light);
		}
		DrawMenuSelectionCursor(mCursor, (mSelection / 20) * (60.0f + mMaxWidth) + (30.0f + (cx - mWindow.mWidth / 2)),
		                        (mSelection % 20) * step + (12.0f + (cy - mWindow.mHeight / 2)), maxWidth, fontSize, mAlpha, mAlpha);
		return TRUE;
	}
	return FALSE;
}

/**
 * @note Address: 0x80061CD0
 * @note Size: 0x510
 */
// NONMATCHING: temporary vector lifetimes and register allocation
extern "C" void DrawMenuSelectionCursor(MenuCursor* cursor, f32 x, f32 y, f32 width, f32 height, f32 alpha, f32 scale)
{
	Vec border[6] = {
		(Vec) { x - 8.0f, y - 1.0f, 0.0f },
		(Vec) { 8.0f + (x + width), y - 1.0f, 0.0f },
		(Vec) { 8.0f + (x + width), 1.0f + (y + height), 0.0f },
		(Vec) { x - 8.0f, 1.0f + (y + height), 0.0f },
		(Vec) { x - 8.0f, y - 1.0f, 0.0f },
		(Vec) { 8.0f + (x + width), y - 1.0f, 0.0f },
	};
	u32 opacity = (u32)(255.0f * alpha);
	DrawGsLineStrip(&gGame.mGs, &border[3], opacity | 0x64646400, 3);
	DrawGsLineStrip(&gGame.mGs, &border[1], opacity | 0xC8C8C800, 3);
	if (cursor->mJObj) {
		Vec pos  = GetStagePlacementPosition(cursor) * scale + (Vec) { x, y, 0.0f };
		Vec size = GetStagePlacementScale(cursor) * scale;
		cursor->SetScale(size, 0);
		cursor->SetRotationY(0.2617994f, 0);
		pos.y = 4.0f + (height * scale + pos.y);
		cursor->SetPos((pos.x - 16.0f) - 8.0f, pos.y, pos.z, 0);
		cursor->vfAC();
		cursor->SetRotationY(-0.2617994f, 0);
		cursor->SetPos(8.0f + (16.0f + (pos.x + width)), pos.y, pos.z, 0);
		cursor->vfAC();
		HSD_StateInvalidate(-1);
		if (gGame.mGs.mCurRenderMode)
			gGame.mGs.mCurRenderMode->Invalidate();
		if (gGame.mGs.mCurTevStage)
			gGame.mGs.mCurTevStage->Invalidate();
		if (gGame.mGs.mCurChannel && !gGame.mGs.mCurChannel->mDirty)
			gGame.mGs.mCurChannel->mDirty = TRUE;
	}
}

/**
 * @note Address: 0x800623F8
 * @note Size: 0x774
 */
// NONMATCHING: operand order in the two window-centering instructions
extern "C" void DrawMenuWindow(MenuWindow* window)
{
	if (window->mDrawAlpha > 0.0f) {
		f32 width            = window->mWidth;
		f32 height           = window->mHeight;
		f32 x                = window->mX - width / 2.0f;
		f32 y                = window->mY - height / 2.0f;
		f32 dx               = 2.0f * (f32)sin(4.712389f * (1.0f - window->mDrawAlpha));
		f32 dy               = 2.0f * (f32)sin(4.712389f * (1.0f - window->mDrawAlpha));
		window->mTex.mScaleX = width / GXGetTexObjWidth(&window->mTex.mTex);
		window->mTex.mScaleY = height / GXGetTexObjHeight(&window->mTex.mTex);
		PSMTXScale(window->mTex.mTex.mMtx[1], window->mTex.mScaleX, window->mTex.mScaleY, 1.0f);
		window->mTex.mTex.mMtxDirty = TRUE;
		Vec outline[18]             = { (Vec) { x - dx, 8.0f + (y - dy), -10.0f },
			                            (Vec) { 8.0f + (x - dx), y - dy, -10.0f },
			                            (Vec) { dx + (x + width) - 8.0f, y - dy, -10.0f },
			                            (Vec) { dx + (x + width), 8.0f + (y - dy), -10.0f },
			                            (Vec) { dx + (x + width), dy + (y + height) - 8.0f, -10.0f },
			                            (Vec) { dx + (x + width) - 8.0f, dy + (y + height), -10.0f },
			                            (Vec) { 8.0f + (x - dx), dy + (y + height), -10.0f },
			                            (Vec) { x - dx, dy + (y + height) - 8.0f, -10.0f },
			                            (Vec) { x - dx, 8.0f + (y - dy), -10.0f },
			                            (Vec) { (6.0f + x) - dx, 8.0f + ((6.0f + y) - dy), -10.0f },
			                            (Vec) { 8.0f + ((6.0f + x) - dx), (6.0f + y) - dy, -10.0f },
			                            (Vec) { dx + (width + (6.0f + x)) - 8.0f, (6.0f + y) - dy, -10.0f },
			                            (Vec) { dx + (width + (6.0f + x)), 8.0f + ((6.0f + y) - dy), -10.0f },
			                            (Vec) { dx + (width + (6.0f + x)), dy + (height + (6.0f + y)) - 8.0f, -10.0f },
			                            (Vec) { dx + (width + (6.0f + x)) - 8.0f, dy + (height + (6.0f + y)), -10.0f },
			                            (Vec) { 8.0f + ((6.0f + x) - dx), dy + (height + (6.0f + y)), -10.0f },
			                            (Vec) { (6.0f + x) - dx, dy + (height + (6.0f + y)) - 8.0f, -10.0f },
			                            (Vec) { (6.0f + x) - dx, 8.0f + ((6.0f + y) - dy), -10.0f } };
		SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 7, 1);
		u32 color = (u32)(255.0f * window->mDrawAlpha) | 0xA0A0A000;
		DrawGsTrianglePrimitive(&gGame.mGs, GX_TRIANGLEFAN, gGame.mGs.mCamera->mViewMtx, outline, color, 9);
		SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 2, 1);
		SetGsTevMode(gGame.mGs.mCurTevStage, 0);
		color = (u32)(32.0f * window->mDrawAlpha) | 0xFFFFFF00;
		DrawGsTexturedQuad(&gGame.mGs, &window->mTex.mTex, color, color, color, color, window->mX, window->mY, -10.0f, width, height, 0.0f,
		                   0.0f, 0.0f);
		SetGsTevMode(gGame.mGs.mCurTevStage, 4);
		SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 5, 0);
		color = (u32)(128.0f * window->mDrawAlpha);
		DrawGsTrianglePrimitive(&gGame.mGs, GX_TRIANGLEFAN, gGame.mGs.mCamera->mViewMtx, &outline[9], color, 9);
		SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 3, 0);
		SetGsLineWidth(gGame.mGs.mCurRenderMode, 12, 5);
		color = (u32)(255.0f * window->mDrawAlpha) | 0xFFFFFF00;
		DrawGsLineStrip(&gGame.mGs, outline, color, 4);
		DrawGsLineStrip(&gGame.mGs, &outline[7], color, 2);
		color = (u32)(255.0f * window->mDrawAlpha) | 0x64646400;
		DrawGsLineStrip(&gGame.mGs, &outline[3], color, 5);
		SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 7, 0);
	}
}

/**
 * @note Address: 0x8003255C
 * @note Size: 0x750
 */
// NONMATCHING: register allocation and popup coordinate scheduling
BOOL MenuMCTest::Update(u32 frames)
{
	if (CMenu::Update(frames) == 4) {
		Memcard* card = &gGame.mMemcard.mCard;
		if (card->mBusy)
			return IsOpen();
		s32 result = card->mCard.mResult;
		switch (mOperation) {
		case 1:
			for (int i = 0; i < mFileCount; i++) {
				if (result == 0) {
					SavePreviewRecord* save = &card->mAllSaves[i];
					if (save->mState) {
						if (save->mStatus != 0 || !IsValidSavePreview(save)) {
							sprintf(mText[i], "(%d) broken [%d,%d]", i, (s32)(save->mStatus != 0), (s32)!IsValidSavePreview(save));
						} else {
							OSCalendarTime cal;
							OSTicksToCalendarTime((OSTime)save->mStat.time * OS_TIMER_CLOCK, &cal);
							s32 stage        = save->mHeader.mStage;
							const char* name = GetStageName(stage);
							if (!name)
								name = GetStageFileName(stage);
							sprintf(mText[i], "(%d) %02d/%02d/%02d %2d:%02d [%2d:%02d] (%3d)%s", i, cal.year % 100, cal.mon + 1, cal.mday,
							        cal.hour, cal.min, save->mHeader.mFrames / 216000, (save->mHeader.mFrames / 3600) % 60, stage, name);
						}
					} else
						sprintf(mText[i], "(%d) none", i);
				} else {
					if (i == mFileCount / 2)
						strcpy(mText[i], card->mCard.GetResultString(card->mCard.mResult));
					else
						strcpy(mText[i], "");
				}
			}
			if (result == 0)
				sprintf(mText[mFileCount + 1], "SLOT: %c   FREE BLOCKS: %d   FREE ENTRIES: %d", sMenuMain.mChannel + 'A', card->mFreeBlocks,
				        card->mFreeFiles);
			mOperation = 0;
			break;
		case 3:
		case 7:
			if (result == 0) {
				StartMemoryCardPreviewRead(card, sMenuMain.mChannel, 1);
				mOperation = 1;
			} else {
				mOperation = 0;
				for (int i = 0; i < mFileCount; i++) {
					if (i == mFileCount / 2)
						strcpy(mText[i], card->mCard.GetResultString(card->mCard.mResult));
					else
						strcpy(mText[i], "");
				}
			}
			return IsOpen();
		case 5:
			mOperation = 0;
			if (result == 0) {
				Close();
				sMenuMain.CMenu::Close();
				PlayGameSound(&gGame.mSound, 0x86, 0xFF);
				gGame.mStatus.mFlags &= ~2ULL;
				ChangeGameStage(&gGame, 20000, 1);
			} else {
				for (int i = 0; i < mFileCount; i++) {
					if (i == mFileCount / 2)
						strcpy(mText[i], card->mCard.GetResultString(card->mCard.mResult));
					else
						strcpy(mText[i], "");
				}
			}
			return IsOpen();
		}
		CPad pad(0);
		if (pad.mPad->mRepeat & 0x10008) {
			if (result == 0) {
				do {
					if (--mSelection < 0)
						mSelection = mFileCount - 1;
				} while (*mText[mSelection] == *"\x01");
				PlayGameSound(&gGame.mSound, 0x87, 0xFF);
			}
		} else if (pad.mPad->mRepeat & 0x20004) {
			if (result == 0) {
				do {
					if (++mSelection > mFileCount - 1)
						mSelection = 0;
				} while (*mText[mSelection] == *"\x01");
				PlayGameSound(&gGame.mSound, 0x87, 0xFF);
			}
		} else if (pad.mPad->mTrigger & 0x200) {
			Close();
			if (sMenuMain.mSuspended)
				gGame.mStatus.mFlags |= 2;
			sMenuMain.mSuspended   = FALSE;
			sMenuMain.mSuspendFade = FALSE;
			PlayGameSound(&gGame.mSound, 0x86, 0xFF);
		} else if (pad.mPad->mTrigger & 0x100) {
			if (result == 0) {
				sMenuMCTestSub.mWindow.mY = (2.0f + mFontSize) * (mSelection + 1) + (mWindow.mY - ((2.0f + mFontSize) * mRowCount) * 0.5f);
				sMenuMCTestSub.mWindow.mX = 3.0f * mFontSize + (mTextWidth[mSelection] * 0.5f + mWindow.mX);
				sMenuMCTestSub.CMenu::Open();
				sMenuMCTestSub.mSelection = 0;
				Suspend(FALSE);
				PlayGameSound(&gGame.mSound, 0x88, 0xFF);
			}
		}
	}
	return IsOpen();
}

/**
 * @note Address: 0x8005EC48
 * @note Size: 0x730
 */
// NONMATCHING: register allocation and floating-point scheduling
BOOL MenuItem::Draw()
{
	if (CMenu::Draw()) {
		DrawPanelBackground<ItemStatusPanel>(mAlpha);
		f32 x = mGridX - 42.0f;
		f32 y = 180.0f + mGridY;
		SetScissor(104, 248);
		DrawInventoryGrid(mSelection, x, y, mAlpha, 1.0f, mItemPhase);
		char text[256];
#if defined(VERSION_JP)
		sprintf(text, "%s", GetLocalizedInterfaceText(0x51));
#else
		sprintf(text, "%s", GetLocalizedInterfaceText(0x18));
#endif
		SetScissor(0, 448);
		DrawItemMenuTitle(this);
		if (mName) {
			f32 alpha   = mAlpha * mNameAlpha;
			f32 spacing = 30.0f;
#if defined(VERSION_PAL)
			spacing *= 0.9f;
#endif
			SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 7, 1);
			SetGsTevMode(gGame.mGs.mCurTevStage, 0);
			CTexObj blob = (&gGame.mTpl)->mTex.Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[109 - StatusTextureOffset], 0);
			mNameCharCount   = 0;
			const char* text = mName;
			if (text) {
#if defined(VERSION_PAL)
				const int limit = 22;
#else
				const int limit = 20;
#endif
				for (int i = 0; i < limit && *text; i++, text++) {
					mBg[i].mText = text;
					mNameCharCount++;
					if (IsJapanese() && (IsSjisLead(*text) & 1))
						text++;
				}
			}
			f32 zero    = 0.0f;
			f32 opacity = 255.0f * alpha;
			for (int i = 0; i < mNameCharCount; i++) {
				MenuHelpChar* glyph = &mBg[i];
#if !defined(VERSION_JP)
				if (strncmp(glyph->mText, " ", 1) != 0)
#endif
				{
					CFont font;
					s32 width;
					CTexObj tex  = *GetFontGlyphTexture(&font, glyph->mText, &width);
					f32 bob      = 2.0f * (f32)sin(glyph->mAngle0);
					f32 rotation = sin(0.7f * glyph->mAngle1);
					f32 s1       = sin(3.1415927f * alpha);
					f32 s2       = sin(1.5707964f * alpha);
					f32 scale    = 0.2f * s1 + s2;
					f32 x        = 0.5f * spacing + (i * spacing + (320.0f - mNameCharCount * spacing * 0.5f));
					f32 y        = 384.0f + bob;
					rotation     = -(0.017453292f * (10.0f * rotation));
					u32 color    = (u32)opacity | 0xFFFFFF00;
					DrawGsTexturedQuad(&gGame.mGs, &blob, color, color, color, color, x, y, 1.0f, 30.0f * scale, 30.0f * scale, 0.0f, -zero,
					                   rotation);
					scale       = 30.0f * (0.8f * scale);
					f32 padding = HelpGlyphPadding(width, scale);
					DrawHelpLetter(tex, x, y, bob, scale, padding, rotation, zero, opacity, gItemHelpOutlineX, gItemHelpOutlineY);
				}
			}
			SetGsTevMode(gGame.mGs.mCurTevStage, 4);
		}
		return TRUE;
	}
	return FALSE;
}

/**
 * @note Address: 0x80056608
 * @note Size: 0x6B0
 */
// NONMATCHING: floating-point register allocation
extern "C" void DrawStatusMenuTabs(MenuStatus* menu)
{
	CTexObj tex;
	f32 offset = 0.0f;
	SetGsBlendMode(gGame.mGs.mCurRenderMode, 1, 4, 5, 15);
	SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 7, 0);
	for (int i = 0; i < 4; i++) {
		if (!menu->mHasFourthTab && i == 3)
			continue;
		f32 bob;
		if (menu->mSelection == i && menu->mMode == 1)
			bob = 4.0f * (f32)sin(0.017453292f * ((menu->mAnimationFrame * 20) % 360));
		else
			bob = 0.0f;
		f32 x = 6.0f + (8.0f + ((menu->mTabs[i].mX - 52.0f) + menu->mOffsetX));
		f32 y = bob + ((180.0f + menu->mTabs[i].mY) + menu->mOffsetY);
		switch (i) {
		case 0:
			offset = -12.0f;
			tex    = (&gGame.mTpl)->mTex.Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[0], 0);
			break;
		case 1:
			offset = 0.0f;
			tex    = (&gGame.mTpl)->mTex.Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[FirstStatusTabTexture + 2], 0);
			break;
		case 2:
			offset = 12.0f;
			tex    = (&gGame.mTpl)->mTex.Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[FirstStatusTabTexture], 0);
			break;
		case 3:
			offset = 24.0f;
			tex    = (&gGame.mTpl)->mTex.Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[FirstStatusTabTexture + 1], 0);
			break;
		}
		f32 width  = 1.2f * GXGetTexObjWidth(&tex);
		f32 height = 1.2f * GXGetTexObjHeight(&tex);
		SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 7, 1);
		SetGsDepthCompareBeforeTexture(gGame.mGs.mCurRenderMode, 0);
		SetGsTevMode(gGame.mGs.mCurTevStage, 0);
		u32 color = (u32)(255.0f * menu->mAlpha) | 0xC8C8C800;
		x += offset;
		DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, x, y, 1.0f, width, height, 0.0f, 0.0f, 0.0f);
		SetGsTevMode(gGame.mGs.mCurTevStage, 4);
		SetGsDepthCompareBeforeTexture(gGame.mGs.mCurRenderMode, 1);
		if (menu->mSelection == i && menu->mMode == 1) {
			SetGsBlendMode(gGame.mGs.mCurRenderMode, 1, 2, 4, 15);
			SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 2, 0);
			f32 pulse = menu->mAlpha * (0.2f * (f32)sin(3.1415927f * menu->mPulsePhase) + 0.2f);
			if (pulse < 0.0f)
				pulse = 0.0f;
			color = (u32)(255.0f * pulse) | 0xFFFFFF00;
			DrawGsQuad(&gGame.mGs, color, color, color, color, x, y, 1.0f, width, height, 0.0f, 0.0f, 0.0f);
			SetGsBlendMode(gGame.mGs.mCurRenderMode, 1, 4, 5, 15);
			SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 7, 0);
		}
	}
}

/**
 * @note Address: 0x80054314 (item/status), 0x80037978 (save)
 * @note Size: 0x83C
 */
// NONMATCHING: vertex initialization and register allocation
template <PanelLayout layout>
void DrawPanelBackground(f32 alpha)
{
	f32 t      = sin(1.5707964f * alpha);
	f32 expand = (layout == SavePanel ? 44.4f : 42.0f) * t;
	f32 spread = 20.0f * t + 60.0f;
	f32 x      = 320.0f;
	x += -2.2f * spread;
	u32 opacity       = (u32)(160.0f * alpha);
	f32 lowerAlpha    = 0.0f;
	u32 bottomOpacity = (u32)lowerAlpha;

	// The original also prepares the lower strip, but submits only the upper three quads.
	f32 lowerY = layout == SavePanel ? 248.0f : 252.0f, upperY = layout == SavePanel ? 224.0f : 228.0f;
	u32 clear = 0;
	PrimVertex vertices[24]
	    = { { (Vec) { ((x - expand) - 2.0f * expand), ((upperY - expand) - 2.0f * expand), 0.0f }, clear },
		    { (Vec) { (x - expand), ((upperY - expand) - 2.0f * expand), 0.0f }, opacity },
		    { (Vec) { (x - expand), (2.0f * expand + (upperY + expand)), 0.0f }, opacity },
		    { (Vec) { ((x - expand) - 2.0f * expand), (2.0f * expand + (upperY + expand)), 0.0f }, clear },
		    { (Vec) { (x - expand), ((upperY - expand) - 2.0f * expand), 0.0f }, opacity },
		    { (Vec) { ((2.2f * spread + 320.0f) + expand), ((upperY - expand) - 2.0f * expand), 0.0f }, opacity },
		    { (Vec) { ((2.2f * spread + 320.0f) + expand), (2.0f * expand + (upperY + expand)), 0.0f }, opacity },
		    { (Vec) { (x - expand), (2.0f * expand + (upperY + expand)), 0.0f }, opacity },
		    { (Vec) { ((2.2f * spread + 320.0f) + expand), ((upperY - expand) - 2.0f * expand), 0.0f }, opacity },
		    { (Vec) { (2.0f * expand + ((2.2f * spread + 320.0f) + expand)), ((upperY - expand) - 2.0f * expand), 0.0f }, clear },
		    { (Vec) { (2.0f * expand + ((2.2f * spread + 320.0f) + expand)), (2.0f * expand + (upperY + expand)), 0.0f }, clear },
		    { (Vec) { ((2.2f * spread + 320.0f) + expand), (2.0f * expand + (upperY + expand)), 0.0f }, opacity },
		    { (Vec) { ((x - expand) - 2.0f * expand), lowerY, 0.0f }, clear },
		    { (Vec) { (x - expand), lowerY, 0.0f }, bottomOpacity },
		    { (Vec) { (x - expand), (2.0f * expand + (lowerY + expand)), 0.0f }, bottomOpacity },
		    { (Vec) { ((x - expand) - 2.0f * expand), (2.0f * expand + (lowerY + expand)), 0.0f }, clear },
		    { (Vec) { (x - expand), lowerY, 0.0f }, bottomOpacity },
		    { (Vec) { ((2.2f * spread + 320.0f) + expand), lowerY, 0.0f }, bottomOpacity },
		    { (Vec) { ((2.2f * spread + 320.0f) + expand), (2.0f * expand + (lowerY + expand)), 0.0f }, bottomOpacity },
		    { (Vec) { (x - expand), (2.0f * expand + (lowerY + expand)), 0.0f }, bottomOpacity },
		    { (Vec) { ((2.2f * spread + 320.0f) + expand), lowerY, 0.0f }, bottomOpacity },
		    { (Vec) { (2.0f * expand + ((2.2f * spread + 320.0f) + expand)), lowerY, 0.0f }, clear },
		    { (Vec) { (2.0f * expand + ((2.2f * spread + 320.0f) + expand)), (2.0f * expand + (lowerY + expand)), 0.0f }, clear },
		    { (Vec) { ((2.2f * spread + 320.0f) + expand), (2.0f * expand + (lowerY + expand)), 0.0f }, bottomOpacity } };
	DrawGsTriangleVertices(&gGame.mGs, GX_TRIANGLEFAN, gGame.mGs.mCamera->mViewMtx, vertices, 4);
	DrawGsTriangleVertices(&gGame.mGs, GX_TRIANGLEFAN, gGame.mGs.mCamera->mViewMtx, &vertices[4], 4);
	DrawGsTriangleVertices(&gGame.mGs, GX_TRIANGLEFAN, gGame.mGs.mCamera->mViewMtx, &vertices[8], 4);
	u32 color = (u32)(255.0f * alpha) | 0xFFFFFF00;
	DrawGsLineSegment(&gGame.mGs, NULL, (x - expand), ((upperY - expand) - 2.0f * expand), 0.0f, color, ((2.2f * spread + 320.0f) + expand),
	                  ((upperY - expand) - 2.0f * expand), 0.0f, color);
	DrawGsLineSegment(&gGame.mGs, NULL, ((x - expand) - 2.0f * expand), ((upperY - expand) - 2.0f * expand), 0.0f, 0xFFFFFF00, (x - expand),
	                  ((upperY - expand) - 2.0f * expand), 0.0f, color);
	DrawGsLineSegment(&gGame.mGs, NULL, ((2.2f * spread + 320.0f) + expand), ((upperY - expand) - 2.0f * expand), 0.0f, color,
	                  (2.0f * expand + ((2.2f * spread + 320.0f) + expand)), ((upperY - expand) - 2.0f * expand), 0.0f, 0xFFFFFF00);
	DrawGsLineSegment(&gGame.mGs, NULL, (x - expand), (2.0f * expand + (upperY + expand)), 0.0f, color, ((2.2f * spread + 320.0f) + expand),
	                  (2.0f * expand + (upperY + expand)), 0.0f, color);
	DrawGsLineSegment(&gGame.mGs, NULL, ((x - expand) - 2.0f * expand), (2.0f * expand + (upperY + expand)), 0.0f, 0xFFFFFF00, (x - expand),
	                  (2.0f * expand + (upperY + expand)), 0.0f, color);
	DrawGsLineSegment(&gGame.mGs, NULL, ((2.2f * spread + 320.0f) + expand), (2.0f * expand + (upperY + expand)), 0.0f, color,
	                  (2.0f * expand + ((2.2f * spread + 320.0f) + expand)), (2.0f * expand + (upperY + expand)), 0.0f, 0xFFFFFF00);
}

template void DrawPanelBackground<ItemStatusPanel>(f32);
template void DrawPanelBackground<SavePanel>(f32);

#if defined(VERSION_JP)
BOOL MenuRequestClear::Draw()
{
	if (CMenu::Draw())
		return TRUE;
	return FALSE;
}

BOOL MenuRequest::Draw()
{
	if (CMenu::Draw())
		return TRUE;
	return FALSE;
}

#endif

#if defined(VERSION_JP)
extern "C" const char* gSoundMenuRowLabels[4];
extern "C" const char* gSettingsMenuRowLabels[5];

void MenuSound::Init()
{
	CMenu::Init();
	mFontSize = 18.0f;
	mRowCount = 4.0f;
	mMaxWidth = 0.0f;
	for (int i = 0; i < mRowCount; i++) {
		mText[i]      = gSoundMenuRowLabels[i];
		mTextWidth[i] = MeasureGameTextWidth(&gGame.mGs, mText[i], 1, mFontSize);
		if (mTextWidth[i] > mMaxWidth)
			mMaxWidth = mTextWidth[i];
	}
	mWindow.SetSize(60.0f + mMaxWidth, mRowCount * (2.0f + mFontSize) + 24.0f);
	mWindow.SetPos(24.0f + (mWindow.mWidth * 0.5f + 16.0f), 24.0f + (mWindow.mHeight * 0.5f + 30.0f));
	mSelection = 0;
	if (mSelection < 0 || mRowCount <= mSelection)
		mSelection = 0;
}

void MenuSettings::Init()
{
	CMenu::Init();
	mFontSize = 18.0f;
	mRowCount = 5.0f;
	mMaxWidth = 0.0f;
	for (int i = 0; i < mRowCount; i++) {
		mText[i]      = gSettingsMenuRowLabels[i];
		f32 extra     = MeasureGameTextWidth(&gGame.mGs, "%s%s", 0, mFontSize);
		mTextWidth[i] = MeasureGameTextWidth(&gGame.mGs, mText[i], 0, mFontSize) - extra;
		if (mTextWidth[i] > mMaxWidth)
			mMaxWidth = mTextWidth[i];
	}
	mWindow.SetSize(60.0f + mMaxWidth, mRowCount * (2.0f + mFontSize) + 24.0f);
	mWindow.SetPos(24.0f + (mWindow.mWidth * 0.5f + 16.0f), 24.0f + (mWindow.mHeight * 0.5f + 30.0f));
	mSelection = 0;
}
#endif

#if defined(VERSION_JP)
extern "C" s32 GetSoundMenuSelection(XSound* sound, s32 index);
extern "C" void SetSoundMenuSelection(XSound* sound, s32 index, s32 value);

BOOL MenuSound::Update(u32 frames)
{
	if (CMenu::Update(frames) == 4) {
		CPad pad(0);
		if (pad.mPad->mRepeat & 0x10008) {
			if (--mSelection < 0)
				mSelection = mRowCount - 1.0f;
			PlayGameSound(&gGame.mSound, 0x149, 0xFF);
		} else if (pad.mPad->mRepeat & 0x20004) {
			if (mRowCount <= ++mSelection)
				mSelection = 0;
			PlayGameSound(&gGame.mSound, 0x149, 0xFF);
		} else if ((pad.mPad->mTrigger & 0x100) || (pad.mPad->mTrigger & 0x200)) {
			StopGameSfx(&gGame.mSound, 0x148);
			PlayGameSound(&gGame.mSound, 0x14A, 0x50);
			Close();
			sMenuMain.Resume();
		}
		if (mSelection >= 0 && mSelection <= 2) {
			s32 value = GetSoundMenuSelection(&gGame.mSound, mSelection);
			if (pad.mPad->mTrigger & 0x40001) {
				if (--value < 0)
					value = 9;
				SetSoundMenuSelection(&gGame.mSound, mSelection, value);
			} else if (pad.mPad->mTrigger & 0x80002) {
				if (++value > 9)
					value = 0;
				SetSoundMenuSelection(&gGame.mSound, mSelection, value);
			}
		}
	}
	return IsOpen();
}

BOOL MenuPadControl::Draw()
{
	if (CMenu::Draw()) {
		f32 left = 0.0f, top = 0.0f, right = 640.0f, bottom = 480.0f;
		Vec background[5] = { (Vec) { left, top, -10.0f }, (Vec) { right, top, -10.0f }, (Vec) { right, bottom, -10.0f },
			                  (Vec) { left, bottom, -10.0f }, (Vec) { left, top, -10.0f } };
		SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 7, 1);
		u32 color = (u32)(160.0f * mAlpha) | 0x0A0A0A00;
		DrawGsTrianglePrimitive(&gGame.mGs, GX_TRIANGLEFAN, gGame.mGs.mCamera->mViewMtx, background, color, 5);
		return TRUE;
	}
	return FALSE;
}
#endif

#if defined(VERSION_JP)
BOOL MenuSound::Draw()
{
	if (CMenu::Draw()) {
		f32 fontSize = mFontSize;
		f32 rows     = mRowCount;
		f32 step     = 2.0f + fontSize;
		f32 y        = mWindow.mY;
		f32 x        = mWindow.mX;
		f32 height   = step * rows;
		f32 maxWidth = mMaxWidth;
		for (int i = 0; i < rows; i++) {
			u32 color;
			if (i == mSelection)
				color = (u32)(255.0f * mAlpha) | 0x20202000;
			else
				color = (u32)(255.0f * mAlpha) | 0x64646400;
			f32 textX = x - mMaxWidth / 2.0f;
			char text[64];
			sprintf(text, mText[i], GetSoundMenuSelection(&gGame.mSound, i));
			DrawGsProportionalTextFormat(&gGame.mGs, color, text, textX, step * i + (y - height / 2.0f), 0.0f, fontSize, fontSize);
		}
		DrawMenuSelectionCursor(mCursor, x - maxWidth / 2.0f, step * mSelection + (y - height / 2.0f), maxWidth, fontSize, mAlpha, mAlpha);
		return TRUE;
	}
	return FALSE;
}
#endif

#if defined(VERSION_JP)
extern "C" void SetRingMenuOption(XObjChrMenu* menu, s32 value);

BOOL MenuSettings::Update(u32 frames)
{
	if (CMenu::Update(frames) == 4) {
		CPad pad(0);
		if (pad.mPad->mRepeat & 0x10008) {
			do {
				if (--mSelection < 0)
					mSelection = mRowCount - 1.0f;
			} while (*mText[mSelection] == *"\x01");
			PlayGameSound(&gGame.mSound, 0x87, 0xFF);
		} else if (pad.mPad->mRepeat & 0x20004) {
			do {
				if (++mSelection > mRowCount - 1.0f)
					mSelection = 0;
			} while (*mText[mSelection] == *"\x01");
			PlayGameSound(&gGame.mSound, 0x87, 0xFF);
		} else if (pad.mPad->mRepeat & 0xC0003) {
			s32 extra = mSelection == 4;
			if (pad.mPad->mRepeat & 0x40001) {
				if (--mValues[mSelection] < 0)
					mValues[mSelection] = extra + 1;
			} else if (pad.mPad->mRepeat & 0x80002) {
				if (++mValues[mSelection] > extra + 1)
					mValues[mSelection] = 0;
			}
			PlayGameSound(&gGame.mSound, 0x88, 0xFF);
			if (mSelection == 0) {
				if (mValues[mSelection] == 0)
					gGame.mStatus.mFlags &= ~0x0000080000000000ULL;
				else
					gGame.mStatus.mFlags |= 0x0000080000000000ULL;
			} else if (mSelection == 1) {
			} else if (mSelection == 2) {
				if (mValues[mSelection] == 0)
					gGame.mStatus.mFlags &= ~0x0000004000000000ULL;
				else
					gGame.mStatus.mFlags |= 0x0000004000000000ULL;
			} else if (mSelection == 3) {
				XObjChr* player = gGame.GetPlayer();
				if (mValues[mSelection])
					player->mPlug.mCord.mFlags |= 8;
				else
					player->mPlug.mCord.mFlags &= ~8;
			} else if (mSelection == 4) {
				SetRingMenuOption(&gGame.GetPlayer()->mMenu, mValues[mSelection]);
			}
		} else if (pad.mPad->mTrigger & 0x300) {
			Close();
			sMenuMain.Resume();
			PlayGameSound(&gGame.mSound, 0x86, 0xFF);
		}
	}
	return IsOpen();
}
#endif

#if defined(VERSION_JP)
BOOL MenuRequest::Update(u32 frames)
{
	if (CMenu::Update(frames) == 4) {
		CPad pad(0);
		PCEase(_528, _530);
		PCEase(_52C, _534);
		switch (mPhase) {
		case 0:
			PlayGameSound(&gGame.mSound, 0x88, 0xFF);
			mPhase++;
			break;
		case 1:
			_53C += 0.08f * frames;
			if (_53C >= 1.0f) {
				_53C   = 1.0f;
				mDelay = 10 * frames;
				mPhase++;
			}
			break;
		case 2:
			if (--mDelay < 0)
				mPhase++;
			break;
		case 3:
			if ((pad.mPad->mTrigger & 0x200) | (pad.mPad->mTrigger & 0x100)) {
				_530 = -640.0f;
				PlayGameSound(&gGame.mSound, 0x88, 0xFF);
				mPhase++;
			}
			break;
		case 4:
			_53C -= 0.08f * frames;
			if (_53C < 0.0f) {
				_53C = 0.0f;
				gGame.mStatus.mFlags &= ~2ULL;
				Close();
				sMenuMain.Resume();
				mPhase++;
			}
			break;
		case 5:
			break;
		}
	}
	return IsOpen();
}
#endif

#if defined(VERSION_JP)
BOOL MenuSettings::Draw()
{
	if (CMenu::Draw()) {
		f32 fontSize = mFontSize;
		f32 maxWidth = mMaxWidth;
		f32 step     = 2.0f + fontSize;
		f32 rows     = mRowCount;
		f32 width    = 30.0f + maxWidth;
		f32 height   = step * rows;
		f32 x        = mWindow.mX;
		f32 y        = mWindow.mY;
		for (int i = 0; i < rows; i++) {
			char text[64];
			char prefix[20];
			char suffix[20];
			u32 color;
			if (i == mSelection) {
				color = (u32)(255.0f * mAlpha) | 0x20202000;
				sprintf(prefix, "\\C0000FF%02X", (u8)(255.0f * mAlpha));
				sprintf(suffix, "\\C202020%02X", (u8)(255.0f * mAlpha));
			} else {
				color = (u32)(255.0f * mAlpha) | 0x64646400;
				sprintf(prefix, "\\C404080%02X", (u8)(255.0f * mAlpha));
				sprintf(suffix, "\\C646464%02X", (u8)(255.0f * mAlpha));
			}
			if (*mText[i] == *"\x01") {
				f32 lineY  = y - height / 2 + step * (0.5f + i);
				f32 left   = x - width / 2;
				u32 shadow = (u32)(255.0f * mAlpha) | 0x64646400;
				DrawGsLineSegment(&gGame.mGs, NULL, left, lineY, 0.0f, shadow, left + width, lineY, 0.0f, shadow);
				u32 light = (u32)(255.0f * mAlpha) | 0xC8C8C800;
				DrawGsLineSegment(&gGame.mGs, NULL, 1.0f + left, 1.0f + lineY, 0.0f, light, 1.0f + (left + width), 1.0f + lineY, 0.0f,
				                  light);
			} else {
				if (i == 0)
					sprintf(text, mText[i], prefix, mValues[i] ? "\x90V" : "\x8b\x8c", suffix);
				else if (i == 1)
					sprintf(text, mText[i], prefix, mValues[i] ? "\x97L" : "\x96\xb3", suffix);
				else if (i == 2)
					sprintf(text, mText[i], prefix, mValues[i] ? "\x97L" : "\x96\xb3", suffix);
				else if (i == 3)
					sprintf(text, mText[i], prefix, mValues[i] ? "\x97L" : "\x96\xb3", suffix);
				else if (i == 4)
					sprintf(text, mText[i], prefix, ConvertHalfWidthToFullWidthSJIS(FormatToStaticBuffer("%d", mValues[i]), NULL), suffix);
				DrawGsFixedWidthTextFormat(&gGame.mGs, color, text, x - mTextWidth[i] / 2, y - height / 2 + step * i, 0.0f, fontSize,
				                           fontSize);
			}
		}
		DrawMenuSelectionCursor(mCursor, x - maxWidth / 2, y - height / 2 + step * mSelection, maxWidth, fontSize, mAlpha, mAlpha);
		return TRUE;
	}
	return FALSE;
}
#endif

#if defined(VERSION_JP)

extern "C" void DrawMenuGradientText(const char* text, f32 x, f32 y, f32 size, f32 alpha, f32)
{
	MeasureGameTextWidth(&gGame.mGs, text, 1, size);
	f32 spacing = size - 5.0f;
	f32 startX  = x;
	SetGsTevMode(gGame.mGs.mCurTevStage, 0);
	MenuTitleChar chars[20];
	s32 count = 0;
	if (text) {
		for (int i = 0; i < 20 && *text; i++, text++) {
			chars[i].mText = text;
			count++;
			if (IsSjisLead(*text))
				text++;
		}
	}
	u32 top       = ((u8)(255.0f * alpha)) | 0xEFEBC500;
	u32 bottom    = ((u8)(255.0f * alpha)) | 0xEDA83600;
	u32 opacity   = (u32)(255.0f * alpha);
	f32 glyphSize = size * 0.8f;
	f32 zero      = 0.0f;
	for (int i = 0; i < count; i++) {
		CFont font;
		s32 width;
		CTexObj tex = *GetFontGlyphTexture(&font, chars[i].mText, &width);
		x           = i * spacing + startX;
		for (int j = 0; j < 4; j++) {
			DrawGsTexturedQuad(&gGame.mGs, &tex, opacity, opacity, opacity, opacity, x + gGradientTextOutlineX[j],
			                   zero + (y + gGradientTextOutlineY[j]), 1.0f, glyphSize, glyphSize, zero, zero, zero);
		}
		DrawGsTexturedQuad(&gGame.mGs, &tex, top, top, bottom, bottom, x, y + zero, 1.0f, glyphSize, glyphSize, zero, zero, zero);
	}
	SetGsTevMode(gGame.mGs.mCurTevStage, 4);
}
#endif

/**
 * @note Address: 0x80036D6C
 * @note Size: 0x668
 */
extern "C" void DrawSaveHelpText(MenuSave* menu)
{
	f32 alpha   = menu->mAlpha * menu->mHelpAlpha;
	f32 spacing = 30.0f;
	SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 7, 1);
	SetGsTevMode(gGame.mGs.mCurTevStage, 0);
	CTexObj blob     = (&gGame.mTpl)->mTex.Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[109 - StatusTextureOffset], 0);
	menu->mHelpCount = 0;
	const char* text = menu->mHelpText;
	if (text) {
		const int limit = 20;
		for (int i = 0; i < limit && *text; i++, text++) {
			menu->mBg[i].mText = text;
			menu->mHelpCount++;
			if (HelpTextIsJapanese() && (IsSjisLead(*text) & 1))
				text++;
		}
	}
	f32 zero    = 0.0f;
	f32 opacity = 255.0f * alpha;
	for (int i = 0; i < menu->mHelpCount; i++) {
		MenuHelpChar* glyph = &menu->mBg[i];
		s32 width;
		CFont font;
		CTexObj tex  = *GetFontGlyphTexture(&font, glyph->mText, &width);
		f32 bob      = 2.0f * (f32)sin(glyph->mAngle0);
		f32 rotation = sin(0.7f * glyph->mAngle1);
		f32 s1       = sin(3.1415927f * alpha);
		f32 s2       = sin(1.5707964f * alpha);
		f32 scale    = 0.2f * s1 + s2;
		f32 x        = 0.5f * spacing + (i * spacing + (320.0f - menu->mHelpCount * spacing * 0.5f));
		f32 y        = 384.0f + bob;
		rotation     = -(0.017453292f * (10.0f * rotation));
		u32 color    = (u32)opacity | 0xFFFFFF00;
		DrawGsTexturedQuad(&gGame.mGs, &blob, color, color, color, color, x, y, 1.0f, 30.0f * scale, 30.0f * scale, 0.0f, -zero, rotation);
		scale       = 30.0f * (0.8f * scale);
		f32 padding = HelpGlyphPadding(width, scale);
		DrawHelpLetter(tex, x, y, bob, scale, padding, rotation, zero, opacity, gSaveHelpOutlineX, gSaveHelpOutlineY);
	}
	SetGsTevMode(gGame.mGs.mCurTevStage, 4);
}

extern "C" u8* GetUtilibotName(s32 id);

extern "C" void DrawPCPriceRow(s16 id, s16 count, s16 value, f32 x, f32 y, f32 alpha, f32 scale)
{
#if defined(VERSION_JP)
	const int rowTexture = 139, digitTexture = 123;
#else
	const int rowTexture = 141, digitTexture = 125;
#endif
	CTexObj tex;
	f32 fontSize = 17.0f * scale;
	if (id < 0)
		return;
	tex = (&gGame.mTpl)->mTex.Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[rowTexture], 0);
	SetGsTevMode(gGame.mGs.mCurTevStage, 0);
	f32 w       = 0.7f * (scale * (1.1f * GXGetTexObjWidth(&tex)));
	f32 h       = 0.7f * (scale * GXGetTexObjHeight(&tex));
	alpha       = 255.0f * alpha;
	u32 opacity = (u32)alpha;
	u32 color   = opacity | 0xC8C8C800;
	DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, x - w / 2.0f, 2.0f * scale + (h / 2.0f + y), 0.0f, w, h, 0.0f, 0.0f,
	                   0.0f);
	SetGsTevMode(gGame.mGs.mCurTevStage, 4);
	if (id >= 20)
		id = 0;
	char text[256];
	sprintf(text, "%s", GetUtilibotName(id));
	color = (count <= 0 ? (u32)(0.4f * alpha) : opacity) | 0xFFFFFF00;
	DrawGsProportionalTextFormat(&gGame.mGs, color, text, 3.0f * scale + x, y, 0.0f, fontSize, fontSize);
	tex = (&gGame.mTpl)->mTex.Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[digitTexture], 0);
	w   = scale * GXGetTexObjWidth(&tex);
	h   = scale * (GXGetTexObjHeight(&tex) / 10);
	PSMTXScale(tex.mMtx[1], 1.0f, 0.1f, 1.0f);
	tex.mMtxDirty = TRUE;
	for (int i = 0; i < 4; i++) {
		f32 div = 1.0f;
		if (i > 0) {
			for (int j = 0; j < i; j++)
				div *= 10.0f;
		}
		s32 n = value / div;
		if (i == 0 || n != 0) {
			n %= 10;
			PSMTXTrans(tex.mMtx[0], 0.0f, 0.1f * n, 0.0f);
			tex.mMtxDirty = TRUE;
			SetGsTevMode(gGame.mGs.mCurTevStage, 0);
			u32 top, bottom;
			if (count <= 0) {
				top    = (u8)(0.4f * alpha) | 0xCECEFF00;
				bottom = (u8)(0.4f * alpha) | 0x5959FF00;
			} else {
				top    = (u8)alpha | 0xCECEFF00;
				bottom = (u8)alpha | 0x5959FF00;
			}
			f32 digitX = 80.0f * scale + (12.0f * scale + (67.0f * scale + (7.0f * w + (x - i * w))));
#if !defined(VERSION_JP)
			digitX = 18.0f * scale + digitX;
#endif
			DrawGsTexturedQuad(&gGame.mGs, &tex, top, top, bottom, bottom, digitX, 8.0f * scale + y, 0.0f, w, h, 0.0f, 0.0f, 0.0f);
			SetGsTevMode(gGame.mGs.mCurTevStage, 4);
		}
	}
#if defined(VERSION_JP)
	tex = (&gGame.mTpl)->mTex.Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[146], 0);
	SetGsTevMode(gGame.mGs.mCurTevStage, 0);
	u16 iconW = GXGetTexObjWidth(&tex);
	u16 iconH = GXGetTexObjHeight(&tex);
	DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, 80.0f * scale + (12.0f * scale + (203.0f * scale + x)),
	                   8.5f * scale + y, 0.0f, 0.9f * (scale * (1.1f * iconW)), 0.9f * (scale * iconH), 0.0f, 0.0f, 0.0f);
	SetGsTevMode(gGame.mGs.mCurTevStage, 4);
#endif
}

extern "C" u8* GetAttachmentName(s32 id);

extern "C" void DrawPCItemPriceRow(s16 id, s16 count, s16 value, bool attachment, f32 x, f32 y, f32 alpha, f32 scale)
{
#if defined(VERSION_JP)
	const int rowTexture = 144, digitTexture = 123;
#else
	const int rowTexture = 146, digitTexture = 125;
#endif
	CTexObj tex;
	f32 fontSize = 17.0f * scale;
	if (id < 0)
		return;
	tex = (&gGame.mTpl)->mTex.Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[rowTexture], 0);
	SetGsTevMode(gGame.mGs.mCurTevStage, 0);
	f32 w       = 0.7f * (scale * (1.1f * GXGetTexObjWidth(&tex)));
	f32 h       = 0.7f * (scale * GXGetTexObjHeight(&tex));
	alpha       = 255.0f * alpha;
	u32 opacity = (u32)alpha;
	u32 color   = opacity | 0xC8C8C800;
	DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, x - w / 2.0f, 2.0f * scale + (h / 2.0f + y), 0.0f, w, h, 0.0f, 0.0f,
	                   0.0f);
	SetGsTevMode(gGame.mGs.mCurTevStage, 4);
	char text[256];
	if (attachment)
		sprintf(text, "%s", GetAttachmentName(id));
	else
		sprintf(text, "%s", GetItemName(id));
	color = (count <= 0 ? (u32)(0.4f * alpha) : opacity) | 0xFFFFFF00;
	DrawGsProportionalTextFormat(&gGame.mGs, color, text, 3.0f * scale + x, y, 0.0f, fontSize, fontSize);
	tex = (&gGame.mTpl)->mTex.Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[digitTexture], 0);
	w   = scale * GXGetTexObjWidth(&tex);
	h   = scale * (GXGetTexObjHeight(&tex) / 10);
	PSMTXScale(tex.mMtx[1], 1.0f, 0.1f, 1.0f);
	tex.mMtxDirty = TRUE;
	for (int i = 0; i < 4; i++) {
		f32 div = 1.0f;
		if (i > 0) {
			for (int j = 0; j < i; j++)
				div *= 10.0f;
		}
		s32 n = value / div;
		if (i == 0 || n != 0) {
			n %= 10;
			PSMTXTrans(tex.mMtx[0], 0.0f, 0.1f * n, 0.0f);
			tex.mMtxDirty = TRUE;
			SetGsTevMode(gGame.mGs.mCurTevStage, 0);
			u32 top, bottom;
			if (count <= 0) {
				top    = (u8)(0.4f * alpha) | 0xEFEBC500;
				bottom = (u8)(0.4f * alpha) | 0xEDA83600;
			} else {
				top    = (u8)alpha | 0xEFEBC500;
				bottom = (u8)alpha | 0xEDA83600;
			}
			f32 digitX = 80.0f * scale + (12.0f * scale + (67.0f * scale + (7.0f * w + (x - i * w))));
#if !defined(VERSION_JP)
			digitX = 18.0f * scale + digitX;
#endif
			DrawGsTexturedQuad(&gGame.mGs, &tex, top, top, bottom, bottom, digitX, 8.0f * scale + y, 0.0f, w, h, 0.0f, 0.0f, 0.0f);
			SetGsTevMode(gGame.mGs.mCurTevStage, 4);
		}
	}
#if defined(VERSION_JP)
	tex = (&gGame.mTpl)->mTex.Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[142], 0);
	SetGsTevMode(gGame.mGs.mCurTevStage, 0);
	u16 iconW = GXGetTexObjWidth(&tex);
	u16 iconH = GXGetTexObjHeight(&tex);
	DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, 80.0f * scale + (12.0f * scale + (203.0f * scale + x)),
	                   9.0f * scale + y, 0.0f, scale * (1.1f * iconW), scale * iconH, 0.0f, 0.0f, 0.0f);
	SetGsTevMode(gGame.mGs.mCurTevStage, 4);
#endif
}

#if !defined(VERSION_JP)
f32 gSaveSlotSpacing = 10.0f;
#endif

static inline f32 SaveNumberLabelX(f32 x, f32 offsetScale)
{
#if defined(VERSION_JP)
	return x;
#else
	return gSaveSlotSpacing * offsetScale + x;
#endif
}

extern "C" void DrawSavePlayTime(s32 icon, s32 hours, s32 minutes, u8, f32 x, f32 y, f32 alpha, f32 offsetScale, f32)
{
	if (!(alpha > 0.0f))
		return;
	CTexObj tex;
	s32 value = hours * 100 + minutes;
	if (value >= 9959)
		value = 9959;
	// The original counts the digits before drawing the fixed time field.
	for (int places = 1;; places++) {
		f32 divisor = 1.0f;
		for (int j = 0; j < places; j++)
			divisor *= 10.0f;
		if ((s32)(value / divisor) == 0)
			break;
	}
	tex = (&gGame.mTpl)->mTex.Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[icon], 0);
	SetGsTevMode(gGame.mGs.mCurTevStage, 0);
	f32 w       = 0.6f * GXGetTexObjWidth(&tex);
	f32 h       = 0.6f * GXGetTexObjHeight(&tex);
	f32 opacity = 255.0f * alpha;
	u32 color   = (u32)opacity | 0xC8C8C800;
	DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, x, y, 0.0f, w, h, 0.0f, 0.0f, 0.0f);
	tex = (&gGame.mTpl)->mTex.Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[PCNumberDigits], 0);
	w   = 1.2f * GXGetTexObjWidth(&tex);
	h   = 1.2f * (GXGetTexObjHeight(&tex) / 10);
	PSMTXScale(tex.mMtx[1], 1.0f, 0.1f, 1.0f);
	tex.mMtxDirty = TRUE;
	u8 a          = opacity;
	u32 top = a | 0xEFEBC500, bottom = a | 0xEDA83600;

	f32 offset = 12.0f + ((6.0f * (w - 3.0f * 1.2f) - 98.0f * 1.2f) - 12.0f);
	for (int i = 0; i < 4; i++) {
		f32 div = 1.0f;
		if (i > 0) {
			for (int j = 0; j < i; j++)
				div *= 10.0f;
		}
		s32 digit = value / div;
		if (i != 3 || digit != 0) {
			PSMTXTrans(tex.mMtx[0], 0.0f, 0.1f * (digit % 10), 0.0f);
			tex.mMtxDirty = TRUE;
			SetGsTevMode(gGame.mGs.mCurTevStage, 0);
			f32 gap = 0.0f;
			if (i < 2)
				gap = w - 3.0f * 1.2f;
			f32 digitX = SaveNumberLabelX(gap + (offset + (7.0f * w + (x - i * (w - 3.0f * 1.2f)))), offsetScale);
			DrawGsTexturedQuad(&gGame.mGs, &tex, top, top, bottom, bottom, digitX, y - 2.4f, 0.0f, w, h, 0.0f, 0.0f, 0.0f);
		}
	}
	SetGsTevMode(gGame.mGs.mCurTevStage, 4);
#if defined(VERSION_JP)
	const s32 separator = 127;
#else
	const s32 separator = 129;
#endif
	tex    = (&gGame.mTpl)->mTex.Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[separator], 0);
	u16 sw = GXGetTexObjWidth(&tex), sh = GXGetTexObjHeight(&tex);
	SetGsTevMode(gGame.mGs.mCurTevStage, 0);
	f32 sepX = SaveNumberLabelX(12.0f + (x + 81.0f * 1.2f), offsetScale);
	DrawGsTexturedQuad(&gGame.mGs, &tex, top, top, bottom, bottom, sepX, y - 1.2f, 0.0f, 0.7f * (1.2f * sw), 0.7f * (1.2f * sh), 0.0f, 0.0f,
	                   0.0f);
	SetGsTevMode(gGame.mGs.mCurTevStage, 4);
}

extern "C" void DrawSaveStat(s32 icon, s32 value, u8, f32 x, f32 y, f32 alpha, f32 offsetScale, f32)
{
	if (!(alpha > 0.0f))
		return;
	CTexObj tex;
	bool missing = false;
	// The original counts the digits before drawing the fixed stat field.
	for (int places = 1;; places++) {
		f32 divisor = 1.0f;
		for (int j = 0; j < places; j++)
			divisor *= 10.0f;
		if ((s32)(value / divisor) == 0)
			break;
	}
	f32 iconScale = 0.7f, iconOffset = 0.0f;
	tex = (&gGame.mTpl)->mTex.Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[icon], 0);
	SetGsTevMode(gGame.mGs.mCurTevStage, 0);
#if defined(VERSION_JP)
	const int rankIcon = 178, expIcon = 152, specialIcon = 241, missingTexture = 165;
#else
	const int rankIcon = 180, expIcon = 154, specialIcon = 243, missingTexture = 167;
#endif
	switch (icon) {
	case rankIcon:
		iconScale = 0.6f;
		break;
	case expIcon:
		iconOffset = 4.0f;
		break;
	case specialIcon:
		if (value == -1)
			missing = true;
		break;
	}
	u16 iw = GXGetTexObjWidth(&tex), ih = GXGetTexObjHeight(&tex);
	alpha     = 255.0f * alpha;
	u32 color = (u32)alpha | 0xC8C8C800;
	DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, x, y + iconOffset, 0.0f, iconScale * iw, iconScale * ih, 0.0f, 0.0f,
	                   0.0f);
	u8 a    = alpha;
	u32 top = a | 0xEFEBC500, bottom = a | 0xEDA83600;
	if (missing) {
		tex   = (&gGame.mTpl)->mTex.Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[missingTexture], 0);
		f32 w = 1.2f * GXGetTexObjWidth(&tex);
		f32 h = 1.2f * GXGetTexObjHeight(&tex);
		SetGsTevMode(gGame.mGs.mCurTevStage, 0);
		f32 offset = 12.0f + ((7.0f * (w - 3.0f * 1.2f) - 98.0f * 1.2f) - 12.0f);
		DrawGsTexturedQuad(&gGame.mGs, &tex, top, top, bottom, bottom, SaveNumberLabelX(offset + (7.0f * w + x), offsetScale), y - 2.4f,
		                   0.0f, w, h, 0.0f, 0.0f, 0.0f);
		SetGsTevMode(gGame.mGs.mCurTevStage, 4);
	} else {
		tex   = (&gGame.mTpl)->mTex.Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[PCNumberDigits], 0);
		f32 w = 1.2f * GXGetTexObjWidth(&tex);
		f32 h = 1.2f * (GXGetTexObjHeight(&tex) / 10);
		PSMTXScale(tex.mMtx[1], 1.0f, 0.1f, 1.0f);
		tex.mMtxDirty = TRUE;
		f32 offset    = 12.0f + ((7.0f * (w - 3.0f * 1.2f) - 98.0f * 1.2f) - 12.0f);
		for (int i = 0; i < 7; i++) {
			f32 div = 1.0f;
			if (i > 0) {
				for (int j = 0; j < i; j++)
					div *= 10.0f;
			}
			s32 digit = value / div;
			if (i == 0 || digit != 0) {
				PSMTXTrans(tex.mMtx[0], 0.0f, 0.1f * (digit % 10), 0.0f);
				tex.mMtxDirty = TRUE;
				SetGsTevMode(gGame.mGs.mCurTevStage, 0);
				DrawGsTexturedQuad(&gGame.mGs, &tex, top, top, bottom, bottom,
				                   SaveNumberLabelX(offset + (7.0f * w + (x - i * (w - 3.0f * 1.2f))), offsetScale), y - 2.4f, 0.0f, w, h,
				                   0.0f, 0.0f, 0.0f);
				SetGsTevMode(gGame.mGs.mCurTevStage, 4);
			}
		}
	}
	SetGsTevMode(gGame.mGs.mCurTevStage, 4);
}

template <PCShopListKind kind>
static inline void DrawPCListRow(MenuPCItem& row, f32 x, f32 y, f32 alpha, f32 scale)
{
	if (kind == ItemPriceList)
		DrawPCItemPriceRow(row.mItem, row.mCount, row.mPrice, row.mIsAttachment, x, y, alpha, scale);
	else
		DrawPCPriceRow(row.mItem, row.mCount, row.mPrice, x, y, alpha, scale);
}

static inline void DrawPCListMask(f32 top, f32 height, u32 color)
{
	f32 left = 130.0f, right = 510.0f;
	Vec mask[5] = { (Vec) { left, top, -10.0f }, (Vec) { right, top, -10.0f }, (Vec) { right, top + height, -10.0f },
		            (Vec) { left, top + height, -10.0f }, (Vec) { left, top, -10.0f } };
	SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 7, 1);
	DrawGsTrianglePrimitive(&gGame.mGs, GX_TRIANGLEFAN, gGame.mGs.mCamera->mViewMtx, mask, color, 5);
}

template <PCShopListKind kind>
#if defined(VERSION_JP)
void DrawPCShopList(s32 selected, f32 x, f32 y, f32 textureOffset, f32 alpha, f32 scale)
#else
void DrawPCShopList(s32 selected, f32 x, f32 y, f32 textureOffset, f32 alpha, f32 scale, f32 scroll)
#endif
{
	CTexObj tex;
#if !defined(VERSION_JP)
	y -= scroll;
#endif
	f32 left;
	f32 width = 296.0f * scale, height = 156.0f * scale;
	left        = x - width / 2.0f;
	Vec bg[5]   = { (Vec) { left, y - height * 0.5f, -10.0f }, (Vec) { width + left, y - height * 0.5f, -10.0f },
		            (Vec) { width + left, height + (y - height * 0.5f), -10.0f }, (Vec) { left, height + (y - height * 0.5f), -10.0f },
		            (Vec) { left, y - height * 0.5f, -10.0f } };
	u32 opacity = (u32)(255.0f * alpha);
	DrawGsTrianglePrimitive(&gGame.mGs, GX_TRIANGLEFAN, gGame.mGs.mCamera->mViewMtx, bg, opacity, 5);
	y += scale;
	f32 step = 17.0f * scale;
	f32 rowY = y - height * 0.5f;
	f32 barX, barW;
	if (IsJapanese()) {
		barX = (10.0f * step * 0.5f + (left + step)) - 8.0f * scale;
		barW = 0.9f * (14.0f * step);
	} else {
		CTexObj& label = (&gGame.mTpl)->mTex.Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[146], 0);
		barX           = x - 4.0f;
		barW           = (0.7f * (scale * (1.1f * (2 * GXGetTexObjWidth(&label)))) + width) - 5.0f * scale;
	}
	f32 barH     = step + scale;
	u32 barColor = (u32)(200.0f * alpha) | 0x181CC600;
	InitMenuSelectionBar(tex);
	f32 barY = 2.0f + ((step * 0.5f + (step * selected + rowY)) - 2.0f * scale);
	DrawMenuSelectionBar(tex, barX, barY, barW, barH, alpha, textureOffset, barColor);
	MenuPCPage& page  = kind == ItemPriceList ? gShopHistoryPage : gShopCatalogPage;
	MenuPCItem* items = kind == ItemPriceList ? sShopHistoryEntries : sShopCatalogEntries;
	for (int i = 0; i < 9; i++)
		DrawPCListRow<kind>(page.mRows[i], left, step * i + rowY, alpha, scale);
	if (page.mStart < PCShopMaxPage(items) - 1)
		DrawPCListRow<kind>(page.mNext[0], left, 9.0f * step + rowY, alpha, scale);
	CountPCShopItems(kind == ItemPriceList ? sShopHistoryEntries : sShopCatalogEntries);
	if (page.mStart > 0)
		DrawPCListRow<kind>(page.mPrev[0], left, rowY - step, alpha, scale);
#if defined(VERSION_JP)
	DrawPCListMask(97.0f, 40.0f, opacity | 0x0A0A0A00);
	DrawPCListMask(292.5f, 40.0f, opacity | 0x0A0A0A00);
#else
	DrawPCListMask(97.0f - scroll, 40.0f, opacity | 0x0A0A0A00);
	DrawPCListMask(292.5f - scroll, 40.0f + scroll, opacity | 0x0A0A0A00);
#endif
}

static inline char* StatusTabHelp(s32 selection)
{
	return selection >= 4 ? NULL : GetLocalizedInterfaceText(StatusTitleText + gStatusTabTextOffsets[selection]);
}

void MenuStatus::Open()
{
	CMenu::Open();
	gGame.mStatus.mFlags |= 2;
	mFontSize            = 22;
	mRowCount            = 14;
	mMaxWidth            = 0;
	mOffsetX             = 0;
	mOffsetY             = 0;
	mTargetOffsetX       = 0;
	mTargetOffsetY       = 0;
	mPanelX              = -10;
	mPanelY              = -200;
	mTargetPanelX        = -10;
	mTargetPanelY        = 0;
	mItemPanelX          = 725;
	mItemPanelY          = 100;
	mTargetItemPanelX    = 725;
	mTargetItemPanelY    = 100;
	mDiscardPanelX       = 937;
	mDiscardPanelY       = 100;
	mTargetDiscardPanelX = 937;
	mTargetDiscardPanelY = 100;
	mHasFourthTab        = TRUE;
	for (int i = 0; i < 4; i++) {
		mTabs[i].mX = 758;
		mTabs[i].mY = 36;
		if (mHasFourthTab)
			mTabs[i].mTargetX = (198.0f + i * 108) - 8.0f;
		else
			mTabs[i].mTargetX = 252.0f + i * 108;
		mTabs[i].mTargetY  = 36;
		mTabs2[i].mX       = 758;
		mTabs2[i].mY       = 36;
		mTabs2[i].mTargetX = 758;
		mTabs2[i].mTargetY = 36;
	}
	mX               = 758;
	mY               = 36;
	mTargetX         = 758;
	mTargetY         = 36;
	mOverviewX       = 1086;
	mOverviewY       = 36;
	mTargetOverviewX = 1086;
	mTargetOverviewY = 36;
	mGridX           = 959;
	mGridY           = 36;
	mTargetGridX     = 959;
	mTargetGridY     = 36;
	mDiagramX        = 1086;
	mDiagramY        = 36;
	mTargetDiagramX  = 1086;
	mTargetDiagramY  = 36;
	mWindow.SetSize(0, 0);
	mWindow.SetPos(0, 0);
	mMode              = 0;
	mPrevMode          = 0;
	mNextMode          = 0;
	mAnimationFrame    = 0;
	mPhase             = 0;
	mDelay             = 0;
	mItem              = 0;
	mPreviousItem      = 0;
	mOverviewSelection = 0;
	mIconSelection     = 0;
	mChoice            = 0;
	mCount             = 0;
	ClearStatusTable(&sStatusItemTable);
	PopulateOwnedItemPage(&sStatusItemTable);
	SetMode(0);
	mDetailScale       = 0;
	mTargetDetailScale = 0;
	mSavedBgm          = gGame.mSound.mMusicFadeOutFrames <= 0 ? gGame.mSound.mCurrentMusicId : 0;
	mSavedBgmVolume    = gGame.mSound.mMusicVolume;
	if (!mHasFourthTab && mSelection >= 3)
		mSelection = 0;
	if (gEventFlags.mFlags[0] & 4)
		mIconLit = TRUE;
	else
		mIconLit = FALSE;
	if ((gEventFlags.mFlags[0] & 0x20000000) && (gEventFlags.mFlags[0] & 0x40000000))
		mShowGauge = TRUE;
	else
		mShowGauge = FALSE;
	mGaugeValue = gEventVars.mVars[0x220 / 4] / 10000.0f;
	for (int i = 0; i < 24; i++)
		gStatusIconUnlocked[i] = 0;
	for (int i = 0; i < 24; i++) {
		int flag = GetStatusIconEventFlagId(i);
		if (flag >= 0 && (gEventFlags.mFlags[(u32)flag / 32] & (1u << (flag & 31))))
			gStatusIconUnlocked[i] = 1;
	}
	for (int i = 0; i < 13; i++)
		gStatusEntryAvailable[i] = 0;
	MarkAvailableStatusEntries(gStatusEntryAvailable);
	u32 color = (u32)(255.0f * mAlpha) | 0xFFFFFF00;
	mTitle.Set(GetLocalizedInterfaceText(StatusTitleText), color);
	mShowHelp  = 0;
	mHelpAlpha = 0;
	mHelpText  = StatusTabHelp(mSelection);
	char* help = StatusTabHelp(mSelection);
	mShowHelp  = 1;
	mHelp      = help;
	mHelpCount = 0;
	for (int i = 0; i < (int)ARRAY_SIZE(mHelpChars); i++) {
		mHelpChars[i].mAngle0 = 0.017453292f * (rand() % 360);
		mHelpChars[i].mAngle1 = 0.017453292f * (rand() % 360);
	}
}

extern "C" s16 GetAttachmentShopPrice(s32 index);
extern "C" s16 GetItemShopPrice(s32 index);
extern "C" s16 GetUtilibotPrice(s32 index);

template <PCShopListKind kind>
static inline void ClearPCShopPage()
{
	MenuPCPage* page  = kind == ItemPriceList ? &gShopHistoryPage : &gShopCatalogPage;
	page->mStart      = 0;
	MenuPCItem* entry = page->mRows;
	for (int i = 0; i < 20; i++, entry++) {
		entry->mItem  = -1;
		entry->mCount = -1;
		entry->mPrice = -1;
		if (kind == ItemPriceList)
			entry->mIsAttachment = 0;
		page->mIndex[i] = -1;
	}
	page->mPrev[0].mItem         = -1;
	page->mPrev[0].mCount        = -1;
	page->mPrev[0].mPrice        = -1;
	page->mPrev[0].mIsAttachment = 0;
	page->mNext[0].mItem         = -1;
	page->mNext[0].mCount        = -1;
	page->mNext[0].mPrice        = -1;
	if (kind == ItemPriceList)
		page->mNext[0].mIsAttachment = 0;
}

void MenuPC::Open()
{
	CMenu::Open();
	mWindow.SetSize(0, 0);
	mWindow.SetPos(0, 0);
	mMode              = 0;
	mPrevMode          = mMode;
	mPhase             = 0;
	mTextPhase         = 0;
	mChoice            = 0;
	mPurchaseSelection = 0;
	_178               = 0;
	_17C               = 0;
	mMoneyDigit        = 0;
	mShopSelection     = 0;
	mScrapDigit        = 0;
	mProgress          = 0;
	mTargetProgress    = 0;
	mSpeed             = 1;
	_198               = -10;
	mFrameHeight       = 0;
	_1A0               = -10;
	mTargetFrameHeight = 60;
	mTextX             = 210;
	mTextY             = 129;
	mTargetX           = mTextX;
	mTargetY           = mTextY;
	ResetPCCharacters(this);
	mAmount          = 0;
	mListAlpha       = 1;
	mTargetListAlpha = 1;
	mPanelAlpha      = 0;
	mPanelTarget     = 0;
	for (int i = 0; i < 4; i++)
		mChoices[i] = -1;
	mChoiceCount = 0;
	mTexScroll   = 0;
	s32 stage    = gGame.mStage;
	if (FindStageIndexByName("stage05") == stage)
		mShopOnly = FALSE;
	else
		mShopOnly = TRUE;
	if (gGame._344C + 1 == gGame.mStage)
		mShopOnly = FALSE;
	FindStageIndexByName("shun");
	mHistoryStart = 600;
	mHistoryCount = 20;
	if (mHistoryCount > 0) {
		for (int i = 0; i < mHistoryCount; i++) {
			MenuPCItem* row = &sShopHistoryEntries[i];
			s32 id          = gEventVars.mVars[mHistoryStart + i];
			row->mItem      = id;
			row->mCount     = gEventVars.mVars[mHistoryStart + (mHistoryCount + i)];
			if (id != -1) {
				if (id & 0x10000) {
					id &= ~0x10000;
					row->mItem         = id;
					row->mIsAttachment = TRUE;
					row->mPrice        = GetAttachmentShopPrice(id);
				} else {
					row->mIsAttachment = FALSE;
					row->mPrice        = GetItemShopPrice(id);
				}
			}
		}
	}
	mChoices[mChoiceCount] = 3;
	mChoiceCount++;
	mCanSelect    = TRUE;
	mCatalogBase  = 560;
	mCatalogCount = 20;
	int count     = 0;
	if (mCatalogCount > 0) {
		for (int i = 0; i < mCatalogCount; i++) {
			MenuPCItem* row = &sShopCatalogEntries[i];
			s32 id          = gEventVars.mVars[mCatalogBase + i];
			row->mItem      = id;
			row->mCount     = gEventVars.mVars[mCatalogBase + (mCatalogCount + i)];
			if (id != -1) {
				row->mPrice = GetUtilibotPrice(id);
				count++;
			}
		}
	}
	if (count == 0)
		mCanSelect = FALSE;
	mChoices[mChoiceCount] = 7;
	mChoiceCount++;
	int flag      = GetStatusIconEventFlagId(3);
	bool unlocked = false;
	if (gEventFlags.mFlags[flag >> 5] & (1u << (flag & 31)))
		unlocked = true;
	if (unlocked) {
		mChoices[mChoiceCount] = 10;
		mChoiceCount++;
	}
	for (int i = 0; i < 5; i++) {
		mRankingOffsets[i]        = 0;
		mRankingRows[i].nameIndex = 0;
		mRankingRows[i].score     = 0;
	}
	mRankingEnd      = 0;
	mGreetingVariant = rand() % 8;
	ClearPCShopPage<ItemPriceList>();
	PopulateShopHistoryPage(&gShopHistoryPage);
	ClearPCShopPage<PriceList>();
	PopulateShopCatalogPage(&gShopCatalogPage);
	mScroll             = 0;
	mScrollTarget       = 0;
	mMoneyDisplay.shown = mMoneyDisplay.target = GetMoney();
	mScrapDisplay.shown = mScrapDisplay.target = GetScrap();
	mDepositDisplay.shown                      = mDepositBalance;
	mDepositDisplay.target                     = mDepositBalance;
	_424                                       = 0;
	_428                                       = 0;
	mScrapRate                                 = 1;
	mExitState                                 = -1;
	StopGameSfx(&gGame.mSound, 0x30007);
	gGame.mStatus.mFlags |= 2;
}

#if defined(VERSION_JP)
static const int StatusIconAtlas = 117, StatusSingleIcon = 228, StatusTextBase = 0x56;
#else
static const int StatusIconAtlas = 119, StatusSingleIcon = 230, StatusTextBase = 0x1d;
#endif

static inline void SetStatusIconCell(CTexObj& tex, int cell)
{
	PSMTXScale(tex.mMtx[1], 1.0f / 17, 1.0f, 1.0f);
	tex.mMtxDirty = TRUE;
	PSMTXTrans(tex.mMtx[0], (1.0f / 17) * cell, 0.0f, 0.0f);
	tex.mMtxDirty = TRUE;
}

static inline void DrawCalloutSprite(CTexObj& tex, u32 color, f32 x, f32 y, f32 size)
{
	SetGsTevMode(gGame.mGs.mCurTevStage, 0);
	DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, x, y, 0.0f, size, size, 0.0f, 0.0f, 0.0f);
	SetGsTevMode(gGame.mGs.mCurTevStage, 4);
}

static inline void DrawStatusDot(CTexObj& tex, u32 color, f32 x, f32 y, f32 size)
{
	tex = (&gGame.mTpl)->mTex.Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[StatusIconAtlas], 0);
	SetStatusIconCell(tex, 13);
	DrawCalloutSprite(tex, color, x, y, size);
}

static inline void DrawCalloutIcon(CTexObj& tex, s32 cell, u32 white, f32 x, f32 y)
{
	if (cell == 14) {
		tex = (&gGame.mTpl)->mTex.Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[StatusSingleIcon], 0);
		DrawCalloutSprite(tex, white, x - 10.8f, y + 9.0f, 21.6f);
	} else {
		tex = (&gGame.mTpl)->mTex.Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[StatusIconAtlas], 0);
		SetStatusIconCell(tex, cell);
		DrawCalloutSprite(tex, white, x - 10.8f, y + 9.0f, 21.6f);
	}
}

extern "C" void DrawStatusCallout(MenuStatus*, s32 id, f32 x, f32 y, f32 targetX, f32 targetY)
{
	CTexObj tex = (&gGame.mTpl)->mTex.Init(&((TPLPalette*)(&gGame.mTpl)->mBuffer)->descriptorArray[StatusIconAtlas], 0);
	struct Label {
		s32 cell;
		const char* text;
	};
	Label labels[11] = { { 0, GetLocalizedInterfaceText(StatusTextBase + 10) }, { 5, GetLocalizedInterfaceText(StatusTextBase) },
		                 { 6, GetLocalizedInterfaceText(StatusTextBase + 1) },  { 7, GetLocalizedInterfaceText(StatusTextBase + 2) },
		                 { 8, GetLocalizedInterfaceText(StatusTextBase + 3) },  { 9, GetLocalizedInterfaceText(StatusTextBase + 4) },
		                 { 10, GetLocalizedInterfaceText(StatusTextBase + 5) }, { 11, GetLocalizedInterfaceText(StatusTextBase + 6) },
		                 { 12, GetLocalizedInterfaceText(StatusTextBase + 7) }, { 13, GetLocalizedInterfaceText(StatusTextBase + 8) },
		                 { 14, GetLocalizedInterfaceText(StatusTextBase + 9) } };
	MeasureGameTextWidth(&gGame.mGs, labels[id].text, 1, 18.0f);
	f32 alpha   = 255.0f;
	u32 opacity = (u32)alpha;
	for (int i = 0; i < 4; i++)
		DrawGsProportionalTextFormat(&gGame.mGs, opacity, labels[id].text, x + gStatusOutlineX[i], y + gStatusOutlineY[i], 0.0f, 18.0f,
		                             18.0f);
	u32 white = opacity | 0xFFFFFF00;
	DrawGsProportionalTextFormat(&gGame.mGs, white, labels[id].text, x, y, 0.0f, 18.0f, 18.0f);
	DrawCalloutIcon(tex, labels[id].cell, white, x, y);
	x = (x - 21.6f) - 10.8f;
	y += 10.8f;
	u32 shadow    = opacity | 0x14141400;
	Vec border[3] = { { x, y, 0.0f }, { targetX, y, 0.0f }, { targetX, targetY, 0.0f } };
	SetGsLineWidth(gGame.mGs.mCurRenderMode, 18, 5);
	DrawGsLineStrip(&gGame.mGs, border, shadow, 3);
	SetGsLineWidth(gGame.mGs.mCurRenderMode, 12, 5);
	DrawStatusDot(tex, shadow, targetX, targetY, 11.0f);
	Vec line[3] = { { x, y, 0.0f }, { targetX, y, 0.0f }, { targetX, targetY, 0.0f } };
	SetGsLineWidth(gGame.mGs.mCurRenderMode, 6, 5);
	DrawGsLineStrip(&gGame.mGs, line, white, 3);
	SetGsLineWidth(gGame.mGs.mCurRenderMode, 12, 5);
	DrawStatusDot(tex, white, targetX, targetY, 10.0f);
}

BOOL MenuStatus::Update(u32 frames)
{
	if (CMenu::Update(frames) == 4) {
		mAnimationFrame++;
		mPulsePhase += (1.0f / (0.3f * FRAME_RATE)) * frames;
		if (mPulsePhase >= 2.0f)
			mPulsePhase = fmod(mPulsePhase, 2.0);
		CPad pad(0);
		PCEase(mOffsetX, mTargetOffsetX);
		PCEase(mOffsetY, mTargetOffsetY);
		PCEase(mPanelX, mTargetPanelX);
		PCEase(mPanelY, mTargetPanelY);
		for (int i = 0; i < 4; i++) {
			PCEase(mTabs[i].mX, mTabs[i].mTargetX);
			PCEase(mTabs[i].mY, mTabs[i].mTargetY);
			PCEase(mTabs2[i].mX, mTabs2[i].mTargetX);
			PCEase(mTabs2[i].mY, mTabs2[i].mTargetY);
		}
		PCEase(mX, mTargetX);
		ApproachHudValue(&mY, &mTargetY, 20.0f);
		PCEase(mOverviewX, mTargetOverviewX);
		ApproachHudValue(&mDetailScale, &mTargetDetailScale, 0.08f * frames);
		PCEase(mGridX, mTargetGridX);
		ApproachHudValue(&mGridY, &mTargetGridY, 20.0f);
		PCEase(mDiagramX, mTargetDiagramX);
		ApproachHudValue(&mDiagramY, &mTargetDiagramY, 20.0f);
		UpdateMenuTitle(this, frames);
		switch (mShowHelp) {
		case 1:
			mHelpAlpha = 0;
			mShowHelp++;
			mHelpText = mHelp;
			break;
		case 2:
			mHelpAlpha += 0.04f * frames;
			if (mHelpAlpha >= 1.0f) {
				mHelpAlpha = 1.0f;
				mShowHelp  = 0;
			}
			break;
		}
		if (mAlpha > 0.0f) {
			f32 spinX = 0.08726646f * frames, spinY = 0.05235988f * frames;
			for (int i = 0; i < (int)ARRAY_SIZE(mHelpChars); i++) {
				mHelpChars[i].mAngle0 += spinX;
				mHelpChars[i].mAngle1 += spinY;
			}
		}
		(this->*gStatusStepHandlers[mMode])(frames);
	}
	return IsOpen();
}

extern "C" void ResumeGameSfx(XSound*, s32);
extern "C" void PauseGameMusic(XSound*, s32, s32);

static inline void BeginItemMenuClose(MenuItem* menu, u32 frames)
{
	menu->mTargetGridX = -640.0f;
	menu->_17C         = -200.0f;
	menu->mDelay       = 4 * frames;
	menu->mPhase++;
	for (int i = 0; i < (int)ARRAY_SIZE(menu->mTitle.mChars); i++)
		menu->mTitle.mChars[i].mHold = 0;
}

static inline void FinishItemMenuScroll(MenuItem* menu)
{
	menu->mGridY         = 36.0f;
	menu->mTargetGridY   = 36.0f;
	char* help           = GetItemName(sStatusItemTable.mItems[menu->mSelection].mItem);
	menu->mShowHelp      = 1;
	menu->mHelp          = help;
	menu->mPrevSelection = menu->mSelection;
}

BOOL MenuItem::Update(u32 frames)
{
	static s32 selectedItem = -1;
	if (CMenu::Update(frames) == 4) {
		CPad pad(0);
		if ((mItemPhase += (1.0f / (0.3f * FRAME_RATE)) * frames) >= 2.0f)
			mItemPhase = fmod(mItemPhase, 2.0);
		PCEase(mGridX, mTargetGridX);
		ApproachHudValue(&mGridY, &mTargetGridY, 20.0f);
		PCEase(_170, _178);
		PCEase(_174, _17C);
		ApproachHudValue(&_190, &_194, 0.08f * frames);
		UpdateMenuTitle(this, frames);
		switch (mShowHelp) {
		case 0:
			break;
		case 1:
			mNameAlpha = 0;
			mShowHelp++;
			mName = mHelp;
			break;
		case 2:
			mNameAlpha += 0.04f * frames;
			if (mNameAlpha >= 1.0f) {
				mNameAlpha = 1.0f;
				mShowHelp  = 0;
			}
			break;
		}
		if (mAlpha > 0.0f) {
			f32 spinX = 0.08726646f * frames, spinY = 0.05235988f * frames;
			for (int i = 0; i < (int)ARRAY_SIZE(mBg); i++) {
				mBg[i].mAngle0 += spinX;
				mBg[i].mAngle1 += spinY;
			}
		}
		s32 count = CountMenuItems();
		switch (mPhase) {
		case 0:
			mSavedBgm = gGame.mSound.mMusicFadeOutFrames <= 0 ? gGame.mSound.mCurrentMusicId : 0;
			PauseGameMusic(&gGame.mSound, mSavedBgm, 0);
			PlayGameSound(&gGame.mSound, 0x88, 0xFF);
			mPhase++;
			break;
		case 1:
			if (mGridX == mTargetGridX)
				mPhase++;
			break;
		case 2:
			if (count > 1) {
				if (pad.mPad->mRepeat & 0x40001) {
					if (--mSelection < 0)
						mSelection = count - 1;
					PlayGameSound(&gGame.mSound, 0x87, 0xFF);
				} else if (pad.mPad->mRepeat & 0x80002) {
					s32 next = ++mSelection;
					if (sStatusItemTable.mItems[next].mItem < 0)
						mSelection = 0;
					PlayGameSound(&gGame.mSound, 0x87, 0xFF);
				}
			}
			if (count > 4) {
				if (mSelection >= 4) {
					if (pad.mPad->mRepeat & 0x10008) {
						if ((mSelection -= 4) < 0)
							mSelection = count - 1;
						PlayGameSound(&gGame.mSound, 0x87, 0xFF);
					}
					if ((pad.mPad->mRepeat & 0x20004) && sStatusItemTable.mPage < (CountMenuItems() - 1) / 4 - 1) {
						mTargetGridY = -56.0f;
						mPhase       = 10;
						break;
					}
				} else {
					if ((pad.mPad->mRepeat & 0x20004) && mSelection + 4 <= count - 1) {
						mSelection += 4;
						PlayGameSound(&gGame.mSound, 0x87, 0xFF);
					}
					if (pad.mPad->mRepeat & 0x10008) {
						CountMenuItems();
						if (sStatusItemTable.mPage > 0) {
							mTargetGridY = 128.0f;
							mPhase       = 20;
							break;
						}
					}
				}
			}
			if (sStatusItemTable.mItems[mSelection].mItem < 0) {
				do {
					--mSelection;
				} while (sStatusItemTable.mItems[mSelection].mItem < 0 && mSelection != 0);
			}
			if (mSelection != mPrevSelection) {
				s16 id = sStatusItemTable.mItems[mSelection].mItem;
				if (id >= 0 && CountMenuItems() > 0) {
					char* text = GetItemName(id);
					mShowHelp  = 1;
					mHelp      = text;
				} else {
					mShowHelp = 1;
					mHelp     = NULL;
				}
				mPrevSelection = mSelection;
			}
			if (pad.mPad->mTrigger & 0x100) {
				s16 id = -1, amount = 0;
				if (mSelection >= 0) {
					const MenuStatusEntry& item = sStatusItemTable.mItems[mSelection];
					id                          = item.mItem;
					amount                      = item.mCount;
				}
				if (count > 0 && id >= 0 && amount > 0)
					selectedItem = id;
				else
					selectedItem = -1;
				BeginItemMenuClose(this, frames);
			} else if (pad.mPad->mTrigger & 0x200) {
				selectedItem = -1;
				BeginItemMenuClose(this, frames);
			}
			break;
		case 3:
			if (--mDelay < 0) {
				gGame.mEvt._10 = selectedItem;
				gGame.mStatus.mFlags &= ~2ULL;
				Close();
				PlayGameSound(&gGame.mSound, 0x88, 0xFF);
				if (mSavedBgm)
					ResumeGameSfx(&gGame.mSound, mSavedBgm);
				mPhase++;
			}
			break;
		case 10:
			if (mGridY == mTargetGridY) {
				mPhase = 2;
				if (sStatusItemTable.mPage < (CountMenuItems() - 1) / 4 - 1) {
					++sStatusItemTable.mPage;
					PopulateOwnedItemPage(&sStatusItemTable);
					PlayGameSound(&gGame.mSound, 0x87, 0xff);
				}
				FinishItemMenuScroll(this);
			}
			break;
		case 20:
			if (mGridY == mTargetGridY) {
				mPhase = 2;
				CountMenuItems();
				if (sStatusItemTable.mPage > 0) {
					--sStatusItemTable.mPage;
					PopulateOwnedItemPage(&sStatusItemTable);
					PlayGameSound(&gGame.mSound, 0x87, 0xff);
				}
				FinishItemMenuScroll(this);
			}
			break;
		}
	}
	return IsOpen();
}

#if defined(VERSION_JP)
BOOL MenuRequestClear::Update(u32 frames)
{
	if (CMenu::Update(frames) == 4) {
		CPad pad(0);
		PCEase(_170, _178);
		PCEase(_174, _17C);
		PCEase(_180, _188);
		PCEase(_184, _18C);
		PCEase(_190, _198);
		PCEase(_194, _19C);
		ApproachHudValue(&_1E0, &_1E4, 0.08f);
		for (int i = 0; i < 4; i++) {
			PCEaseCall(mValues[i], mTargets[i]);
			f32 distance = mValues[i] - mTargets[i];
			if (distance < 0)
				distance = -distance;
			if (distance != 0)
				PlayGameSound(&gGame.mSound, 0xB4, 0xFF);
		}
		switch (mPhase) {
		case 0:
			PlayGameSound(&gGame.mSound, 0x88, 0xFF);
			mPhase++;
			_19C = 0;
			break;
		case 1:
			_1B4 += 0.08f * frames;
			if (_1B4 >= 1) {
				_1B4   = 1;
				mDelay = 10 * frames;
				mPhase++;
			}
			break;
		case 2:
			if (--mDelay < 0) {
				PlayGameSound(&gGame.mSound, 0x8F, 0xFF);
				mPhase++;
			}
			break;
		case 3:
			_1B8 += 0.08f * frames;
			if (_1B8 >= 1) {
				_1B8 = 1;
				mPhase++;
			}
			break;
		case 4:
			if (!IsGameSoundPlaying(&gGame.mSound, 0x8F)) {
				mPhase++;
				_1E4 = 0;
			}
			break;
		case 5:
			if (_1E0 <= 0.001f) {
				_1DC = FALSE;
				_1E4 = 0.8f;
				mPhase++;
			}
			break;
		case 6:
			if (_1E0 >= 0.799f) {
				mPhase++;
				mDelay = 10 * frames;
			}
			break;
		case 7:
			if (--mDelay < 0) {
				mTargets[0] = 0;
				mTargets[2] = gGame.GetPlayer()->mParams.mExperience;
				mPhase++;
			}
			break;
		case 8:
			if (mValues[0] == mTargets[0]) {
				PlayGameSound(&gGame.mSound, 0xF5, 0xFF);
				mDelay = 10 * frames;
				mPhase++;
			}
			break;
		case 9:
			if (--mDelay < 0) {
				mTargets[1] = 0;
				mTargets[3] = gGame.GetPlayer()->mParams.mMoney;
				mPhase++;
			}
			break;
		case 10:
			if (mValues[1] == mTargets[1]) {
				PlayGameSound(&gGame.mSound, 0xF5, 0xFF);
				mDelay = 10 * frames;
				mPhase++;
			}
			break;
		case 11:
			if (--mDelay < 0)
				mPhase = 12;
			break;
		case 12:
			if ((pad.mPad->mTrigger & 0x200) | (pad.mPad->mTrigger & 0x100)) {
				_178 = -640;
				_188 = -640;
				_19C = -200;
				PlayGameSound(&gGame.mSound, 0x88, 0xFF);
				mPhase++;
			}
			break;
		case 13:
			_1B4 -= 0.08f * frames;
			if (_1B4 < 0) {
				_1B4 = 0;
				Close();
				gGame.mStatus.mFlags &= ~2ULL;
				sMenuMain.Resume();
				mPhase++;
			}
			break;
		case 14:
			break;
		}
	}
	return IsOpen();
}
#endif

#if defined(VERSION_JP)
static inline void DrawSaveSlotMessage(char (&message)[256], s32 id, f32 row, f32 x, f32 y, f32 alpha, f32 sx, f32 sy)
{
	sprintf(message, "%.255s", GetLocalizedInterfaceText(id));
	u32 chars     = strlen(message) / 2 - 1;
	f32 halfGlyph = 9.0f * sx;
	f32 right     = 10.0f * sx + x;
	f32 top       = (y - 5.0f * (17.0f * sy)) - 6.0f * sy;

	DrawMenuGradientText(message, ((right - halfGlyph * chars) - halfGlyph) - 2.0f * sx, row * sy + top, 23.0f * sx, alpha, sx);
}

#else
extern "C" {
f32 gSaveTextShadowX[4] = { -1.0f, 1.0f, 1.0f, -1.0f };
f32 gSaveTextShadowY[4] = { -1.0f, -1.0f, 1.0f, 1.0f };
}

static inline void DrawCenteredSaveLine(const char* text, f32 x, f32 y, f32 size, u32 color, f32 offsetY)
{
	f32 top   = (y + offsetY) - size / 2.0f;
	f32 width = MeasureGameTextWidth(&gGame.mGs, text, 1, size);
	DrawGsProportionalTextFormat(&gGame.mGs, color, text, x - width / 2.0f, top, 0.0f, size, size);
}
static inline void DrawColoredSlotCaption(const char* text, f32 x, f32 y, f32 size, u32 shadow, u32 color)
{
	const f32 glyphSize = 0.8f * size;
	for (int i = 0; i < 4; i++)
		DrawCenteredSaveLine(text, x + gSaveTextShadowX[i], y + gSaveTextShadowY[i], glyphSize, shadow, 0.0f);
	DrawCenteredSaveLine(text, x, y, glyphSize, color, 0.0f);
}
static inline void DrawSaveSlotCaption(const char* text, f32 x, f32 y, f32 size, f32 alpha)
{
	u32 color  = (u8)(255.0f * alpha) | 0xEDA83600;
	u32 shadow = (u32)(255.0f * alpha);
	DrawColoredSlotCaption(text, x, y, size, shadow, color);
}
static inline void DrawSaveSlotMessage(s32 id, bool twoLines, f32 x, f32 y, f32 alpha, f32 sx, f32 sy)
{
	char* text = GetLocalizedInterfaceText(id);
	sx         = 23.0f * sx;
	u32 color  = (u8)(255.0f * alpha) | 0xEDA83600;
	u32 shadow = (u32)(255.0f * alpha);
	y          = (y - 5.0f * (17.0f * sy)) - 6.0f * sy;
	DrawColoredSlotCaption(text, x, (twoLines ? 76.0f : 86.0f) * sy + y, sx, shadow, color);
	if (twoLines) {
		text = GetLocalizedInterfaceText(id + 1);
		DrawColoredSlotCaption(text, x, 96.0f * sy + y, sx, shadow, color);
	}
}
#endif

static inline f32 SaveSlotStatX(f32 x, f32 scale)
{
#if defined(VERSION_JP)
	return x;
#else
	return x - (gSaveSlotSpacing * 0.5f) * scale;
#endif
}

static inline void DrawSlotFrame(u8 selected, f32 x, f32 y, f32 width, f32 height, f32 alpha)
{
	// The original constructs a separate texture while drawing the slot frame.
	CTexObj frameTexture;
	u32 fill, border;
	if (selected == 1) {
		fill   = (u32)(255.0f * alpha) | 0xFFFFFF00;
		border = (u32)(255.0f * alpha) | 0xFFB62800;
	} else {
		fill   = (u32)(160.0f * alpha) | 0x28282800;
		border = (u32)(255.0f * alpha) | 0xFFFFFF00;
	}
	DrawTexturedWindowFrame(fill, border, x, y - height / 2.0f, width - 56.0f, height, alpha, 1.0f, 1.0f);
}
static inline void DrawSlotStats(int iconOffset, MenuSaveSlot* slot, u8 selected, f32 x, f32 y, f32 alpha, f32 sx, f32 sy, f32 offsetY)
{
	x -= 70.0f * sx;

	DrawSaveStat(0xB2 + iconOffset, slot->mRank, selected, SaveSlotStatX(x, sx), offsetY + (y - 64.0f * sy), alpha, sx, sy);
	s32 battery = slot->mBattery;
	if (slot->mActive)
		battery = -1;
	DrawSaveStat(0xF1 + iconOffset, battery, selected, SaveSlotStatX(x, sx), offsetY + (y - 32.0f * sy), alpha, sx, sy);
	DrawSaveStat(0x98 + iconOffset, slot->mExp, selected, SaveSlotStatX(x, sx), y + offsetY, alpha, sx, sy);
	DrawSaveStat(0x96 + iconOffset, slot->mMoney, selected, SaveSlotStatX(x, sx), offsetY + (y + 32.0f * sy), alpha, sx, sy);
	DrawSaveStat(0x97 + iconOffset, slot->mScrap, selected, SaveSlotStatX(x, sx), offsetY + (y + 64.0f * sy), alpha, sx, sy);
	DrawSavePlayTime(0x9B + iconOffset - (slot->mTimeMode == 0), slot->mHours, slot->mMinutes, selected, SaveSlotStatX(x, sx),
	                 offsetY + (96.0f * sy + y), alpha, sx, sy);
}
extern "C" void DrawSaveSlotDetails(MenuSaveSlot* slot, u8 emptyMessage, s16 index, u8 selected, f32 x, f32 y, f32 width, f32 height,
                                    f32 alpha, f32 sx, f32 sy)
{
#if defined(VERSION_JP)
	char message[256];
	const int iconOffset = 0;
#else
	const int iconOffset = 2;
#endif
	CTexObj tex;
	if (index >= 0) {
		DrawSlotFrame(selected, x, y, width, height, alpha);
	}
	if (slot->mState == 1u) {
#if defined(VERSION_JP)
		DrawSaveSlotMessage(message, 0x39, 76.0f, x, y, alpha, sx, sy);
		DrawSaveSlotMessage(message, 0x3A, 96.0f, x, y, alpha, sx, sy);
#else
		DrawSaveSlotMessage(0x9E, true, x, y, alpha, sx, sy);
#endif
	} else if (slot->mUsed) {
		f32 offsetY = -24.0f * sy;
		tex         = gGame.mTpl.mTex.Init(&((TPLPalette*)gGame.mTpl.mBuffer)->descriptorArray[111 + iconOffset], 0);
		SetGsTevMode(gGame.mGs.mCurTevStage, 0);
		u16 w = GXGetTexObjWidth(&tex), h = GXGetTexObjHeight(&tex);
		SetGsTevMode(gGame.mGs.mCurTevStage, 0);
		f32 iconX = 100.0f * sx + (x - 10.0f * (17.0f * sx));
#if !defined(VERSION_JP)
		iconX -= (gSaveSlotSpacing * 0.5f) * sx;
#endif
		u32 color = (u32)(255.0f * alpha) | 0xC8C8C800;
		f32 top   = y - 5.0f * (17.0f * sy);
		DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, iconX, offsetY + (218.0f * sy + (top - 6.0f * sy)), 0,
		                   0.6f * (sx * w), 0.6f * (sy * h), 0, 0, 0);
		SetGsTevMode(gGame.mGs.mCurTevStage, 4);
		s32 stage        = slot->mStage;
		const char* name = GetStageName(stage);
		if (!name)
			name = GetStageFileName(stage);
		if (stage == 14)
			name = GetStageName(7);
#if defined(VERSION_JP)
		u32 chars = strlen(name) / 2;
		f32 nameX = (88.0f * sx + x) - (18.0f * sx) * chars;
		DrawMenuGradientText(name, nameX, offsetY + (210.0f * sy + (4.0f * sy + top)), 23.0f * sx, alpha, sx);
#else
		const f32 textSize = 23.0f * sx;
		f32 nameWidth      = MeasureGameTextWidth(&gGame.mGs, name, 1, 0.8f * textSize);
		f32 nameX          = (88.0f * sx + x) - nameWidth * 0.5f;
		DrawSaveSlotCaption(name, ((gSaveSlotSpacing * 0.5f) * sx + nameX) - 5.0f * sx, offsetY + (210.0f * sy + (4.0f * sy + top)),
		                    textSize, alpha);
#endif
		DrawSlotStats(iconOffset, slot, selected, x, y, alpha, sx, sy, offsetY);
	} else if (emptyMessage) {
#if defined(VERSION_JP)
		DrawSaveSlotMessage(message, 0x3B, 86.0f, x, y, alpha, sx, sy);
#else
		DrawSaveSlotMessage(0x63, false, x, y, alpha, sx, sy);
#endif
	} else {
#if defined(VERSION_JP)
		DrawSaveSlotMessage(message, 0x3C, 86.0f, x, y, alpha, sx, sy);
#else
		if (IsJapanese())
			DrawSaveSlotMessage(0x99, false, x, y, alpha, sx, sy);
		else
			DrawSaveSlotMessage(0x99, true, x, y, alpha, sx, sy);
#endif
	}
}

static f32 sBannerScrollX, sBannerScrollY;

static inline void DrawMenuBannerStrip(CTexObj& tex, f32 x, f32 y, f32 height, f32 alpha)
{
	Vec vertices[5] = { (Vec) { x, y, -10.0f }, (Vec) { x + 660.0f, y, -10.0f }, (Vec) { x + 660.0f, y + height, -10.0f },
		                (Vec) { x, y + height, -10.0f }, (Vec) { x, y, -10.0f } };
	CGs& gs         = gGame.mGs;
	SetGsDepthMode(gs.mCurRenderMode, 1, 7, 1);
	u32 fill = (u32)(255.0f * alpha) | 0xFFB62800;
	DrawGsTrianglePrimitive(&gs, GX_TRIANGLEFAN, gs.mCamera->mViewMtx, vertices, fill, 5);
#if defined(VERSION_JP)
	const int icon = 136;
#else
	const int icon = 138;
#endif
	tex = gGame.mTpl.mTex.Init(&((TPLPalette*)gGame.mTpl.mBuffer)->descriptorArray[icon], 0);
	PSMTXTrans(tex.mMtx[0], sBannerScrollX, sBannerScrollY, 0.0f);
	tex.mMtxDirty = TRUE;
	u16 h = GXGetTexObjHeight(&tex), w = GXGetTexObjWidth(&tex);
	PSMTXScale(tex.mMtx[1], 660.0f / w, height / h, 1.0f);
	tex.mMtxDirty = TRUE;
	SetGsDepthMode(gs.mCurRenderMode, 1, 4, 1);
	SetGsBlendMode(gs.mCurRenderMode, 1, 2, 4, 15);
	SetGsTevMode(gs.mCurTevStage, 2);
	u32 overlay = (u32)(16.0f * alpha) | 0xFFFFFF00;
	DrawGsTexturedQuad(&gs, &tex, overlay, overlay, overlay, overlay, x + 330.0f, height * 0.5f + y, -11.0f, 660.0f, height, 0, 0, 0);
	SetGsTevMode(gs.mCurTevStage, 4);
	SetGsBlendMode(gs.mCurRenderMode, 1, 4, 5, 15);
	SetGsDepthMode(gs.mCurRenderMode, 0, 3, 0);
}

extern "C" void DrawMenuBannerPanels(const char* title, f32 x, f32 y, f32 alpha, f32 slide)
{
	CTexObj tex;
	sBannerScrollX -= 0.008f;
	sBannerScrollY -= 0.008f;

	DrawMenuBannerStrip(tex, x, y + 10.0f, 88.0f, alpha);
	if (title) {
		char text[256];
		sprintf(text, "%.255s", title);
		u32 count   = strlen(text) / 2;
		f32 left    = (320.0f + x) - ((count - 1) * 30) / 2;
		f32 textTop = 4.0f + (8.0f + (22.0f + y));
		DrawDoubleByteMenuText(text, 8.0f + left, 8.0f + (15.0f + textTop), -10.0f, 30.0f, 30.0f, alpha);
	}
	f32 bottomY = (356.0f - y) - 6.0f + slide;
	f32 height  = 88.0f - slide;
	if (height < 8.0f)
		height = 8.0f;
	DrawMenuBannerStrip(tex, x, bottomY, height, alpha);
}

#if defined(VERSION_JP)
extern "C" s32 DrawJapaneseSaveStatusMessage(s32 channel, s32 id, f32 x, f32 y, f32 size, f32 alpha);
extern "C" s32 DrawJapaneseCardErrorMessage(s32 channel, s32 id, u8 retry, f32 x, f32 y, f32 size, f32 alpha);
#endif

static inline f32 SaveMessageY(MenuSave* menu, f32 panelY)
{
	return ((8.0f + (356.0f - panelY)) - 6.0f) + menu->mBannerSlide;
}
static inline f32 SaveMessageX(f32 panelX)
{
#if defined(VERSION_JP)
	return 320.0f + panelX;
#else
	return 0.0f;
#endif
}
static inline s32 DrawSaveLinesAt(MenuSave* menu, s32 id, f32 x, f32 y)
{
#if defined(VERSION_JP)
	return DrawJapaneseSaveStatusMessage(menu->mChannel, id, x, y, 21.0f, menu->mDrawAlpha);
#else
	return DrawSaveStatusMessage(menu->mChannel, id, x, y, 21.0f, menu->mDrawAlpha);
#endif
}
static inline s32 SaveStatusLines(MenuSave* menu, s32 id, f32 panelX, f32 panelY)
{
	return DrawSaveLinesAt(menu, id, SaveMessageX(panelX), SaveMessageY(menu, panelY));
}
static inline s32 DrawSavePhaseMessage(MenuSave* menu, s32 first, s32 next, f32 panelX, f32 panelY)
{
	f32 x = SaveMessageX(panelX), y = SaveMessageY(menu, panelY);
	if (menu->mPhase == 0)
		return DrawSaveLinesAt(menu, first, x, y);
	return DrawSaveLinesAt(menu, next, x, y);
}

#if defined(VERSION_JP)
static inline u32 SavePromptHalfWidth(u32 cells)
{
	return ((cells - 1) * 21) / 2;
}
#else
static inline void DrawSaveChoiceLabel(MenuSave* menu, s32 id, f32 x, f32 y)
{
	u32 color        = (u32)(255.0f * menu->mDrawAlpha);
	const char* text = GetLocalizedInterfaceText(id);
	f32 width        = MeasureGameTextWidth(&gGame.mGs, text, 1, 21.0f);
	DrawGsProportionalTextFormat(&gGame.mGs, color, text, x - width / 2, y, 0.0f, 21.0f, 21.0f);
}
#endif

BOOL MenuSave::Draw()
{
	if (CMenu::Draw()) {
		DrawPanelBackground<SavePanel>(mDrawAlpha);
		switch (mMode) {
		case 2:
		case 3:
		case 4:
		case 5:
			for (int i = 0; i < 3; i++) {
#if defined(VERSION_JP)
				f32 width = 154.0f;
				f32 x     = 49.0f + (77.0f + i * 194.0f);
#else
				f32 width = 154.0f + gSaveSlotSpacing;
				f32 x     = 49.0f + (width / 2.0f + i * (40.0f + width)) - (3.0f * gSaveSlotSpacing) * 0.5f;
#endif
				u8 selected = 0, emptyMessage = 0;
				if (mFile == i)
					selected = 1;
				if (mFreeBlocks >= 6 && mFreeFiles > 0)
					emptyMessage = 1;
				DrawSaveSlotDetails(&mSlots[i], emptyMessage, i, selected, x, 223.0f, width, 230.0f, mDrawAlpha, 1.0f, 1.0f);
			}
			DrawSaveHelpText(this);
			break;
		}
		f32 panelX = mPanelX;
		f32 panelY = mPanelY - 8.0f;
		DrawMenuBannerPanels(NULL, panelX, panelY, mDrawAlpha, mBannerSlide);
		DrawSaveMenuTitle(this);
#if defined(VERSION_JP)
		enum { selectMessage = 0x23, confirmMessage = 0xE, writeMessage = 0x16 };
#else
		enum { selectMessage = 0xE9, confirmMessage = 0xA8, writeMessage = 0xE4 };
#endif
		s32 lines        = 2;
		bool showMessage = false;
		char text[256];

		switch (mMode) {
		case 1:
			if (mSaved == 1)
				lines = SaveStatusLines(this, 4, panelX, panelY);
			break;
		case 2:
			sprintf(text, "%.255s", GetLocalizedInterfaceText(selectMessage));
			showMessage = true;
			break;
		case 3:
			sprintf(text, "%.255s", GetLocalizedInterfaceText(confirmMessage));
			showMessage = true;
			break;
		case 4:
			if (mPhase == 0)
				lines = SaveStatusLines(this, 4, panelX, panelY);
			else {
				sprintf(text, "%.255s", GetLocalizedInterfaceText(writeMessage));
				showMessage = true;
			}
			break;
		case 5:
			lines = SaveStatusLines(this, 4, panelX, panelY);
			break;
		case 8:
			lines = DrawSavePhaseMessage(this, 5, 6, panelX, panelY);
			break;
		case 9:
			lines = SaveStatusLines(this, 7, panelX, panelY);
			break;
		case 10:
			lines = DrawSavePhaseMessage(this, 7, 9, panelX, panelY);
			break;
		case 6:
		case 11: {
			f32 y = SaveMessageY(this, panelY);
#if defined(VERSION_JP)
			lines = DrawJapaneseCardErrorMessage(mChannel, mResult, 0, 320.0f + panelX, y, 21.0f, mDrawAlpha);
#else
			lines = DrawCardErrorMessage(mChannel, mResult, 0, 0.0f, y, 21.0f, mDrawAlpha);
#endif
			break;
		}
		case 12:
			lines = SaveStatusLines(this, mMessage, panelX, panelY);
			break;
		case 13:
#if defined(VERSION_JP)
			lines = SaveStatusLines(this, 13, panelX, panelY);
#else
			if (mFormatCancelled == 1)
				lines = SaveStatusLines(this, 17, panelX, panelY);
			else
				lines = SaveStatusLines(this, 13, panelX, panelY);
#endif
			break;
		case 14:
			lines = SaveStatusLines(this, mMessage, panelX, panelY);
			break;
		case 15:
			lines = SaveStatusLines(this, mMessage, panelX, panelY);
			break;
		case 16:
		case 17:
			break;
		}
		if (lines < 2)
			lines = 2;
		mOffset = -(lines - 2) * 21;
		if (showMessage) {
#if defined(VERSION_JP)
			u32 cells = strlen(text) / 2;
			u32 color = (u32)(255.0f * mDrawAlpha);
			f32 y     = SaveMessageY(this, panelY);
			f32 x     = 320.0f + panelX - SavePromptHalfWidth(cells);
#else
			f32 y     = SaveMessageY(this, panelY);
			u32 color = (u32)(255.0f * mDrawAlpha);
			f32 x     = 320.0f - MeasureGameTextWidth(&gGame.mGs, text, 1, 21.0f) / 2;
#endif
			DrawGsProportionalTextFormat(&gGame.mGs, color, text, x, y, 0.0f, 21.0f, 21.0f);
		}
		switch (mMode) {
		case 3:
		case 8: {
			f32 lineOffset = 0;
			if (lines > 1)
				lineOffset = (lines - 2) * 21;
#if defined(VERSION_JP)
			sprintf(text, "%.255s", GetLocalizedInterfaceText(0x40));
			u32 cells  = strlen(text) / 2;
			f32 center = 320.0f + panelX;

			f32 barX;
			if (mChoice == 0)
				barX = ((32.0f + center) - SavePromptHalfWidth(cells)) - 2.0f;
			else
				barX = 60.0f + (10.0f + ((32.0f + center) - SavePromptHalfWidth(cells)));
			f32 barWidth = 56.7f;
#else
			f32 barX     = mChoice == 0 ? 281.25f : 358.75f;
			f32 barWidth = 61.5f;
#endif
			f32 baseLabelY = (24.0f + (12.0f + (356.0f - panelY))) - 6.0f;
			f32 labelY     = lineOffset + (baseLabelY + mBannerSlide);

			{
				CTexObj tex;
				f32 opacity = mDrawAlpha, textureOffset = mTexScroll;
				u32 barColor = (u32)(200.0f * opacity) | 0x181CC600;
				InitMenuSelectionBar(tex);
				DrawMenuSelectionBar(tex, barX, 2.0f + (8.0f + labelY), barWidth, 22.0f, opacity, textureOffset, barColor);
			}
			labelY = lineOffset + (baseLabelY + mBannerSlide);
#if defined(VERSION_JP)
			f32 labelX = 320.0f + panelX - SavePromptHalfWidth(cells);
			DrawGsProportionalTextFormat(&gGame.mGs, (u32)(255.0f * mDrawAlpha), text, 10.0f + labelX, labelY, 0.0f, 21.0f, 21.0f);
#else
			if (IsJapanese()) {
				DrawSaveChoiceLabel(this, 0x65, 281.25f, labelY);
				DrawSaveChoiceLabel(this, 0x66, 358.75f, labelY);
			} else {
				DrawSaveChoiceLabel(this, 0x65, 283.25f, labelY - 1.0f);
				DrawSaveChoiceLabel(this, 0x66, 360.75f, labelY - 1.0f);
			}
#endif
			break;
		}
		}
		return TRUE;
	}
	return FALSE;
}

#if defined(VERSION_JP)
static inline void DrawJapaneseCardLine(char* text, f32 x, f32 y, f32 size, u32 color)
{
	u32 gaps = strlen(text) / 2 - 1;
	DrawGsProportionalTextFormat(&gGame.mGs, color, text, x - (gaps * size) / 2.0f, y, 0.0f, size, size);
}

static inline void DrawJapaneseCardLine(char (&text)[256], s32 id, f32 x, f32 y, f32 size, u32 color)
{
	sprintf(text, "%.255s", GetLocalizedInterfaceText(id));
	DrawJapaneseCardLine(text, x, y, size, color);
}

static void AppendJapaneseCardSuffix(char (&text)[256], const char* suffix);

static inline void DrawJapaneseCardLine(char (&text)[256], const char* prefix, s32 id, f32 x, f32 y, f32 size, u32 color)
{
	sprintf(text, "%.255s", prefix);
	AppendJapaneseCardSuffix(text, GetLocalizedInterfaceText(id));
	DrawJapaneseCardLine(text, x, y, size, color);
}

static inline const char* JapaneseCardSlotName(s32 channel)
{
	if (channel == 0)
		return GetLocalizedInterfaceText(0x29);
	if (channel == 1)
		return GetLocalizedInterfaceText(0x2A);
	return "";
}

extern "C" s32 DrawJapaneseCardErrorMessage(s32 channel, s32 error, u8 retry, f32 x, f32 y, f32 size, f32 alpha)
{
	char text[256];
	u32 color = (u32)(255.0f * alpha);
	s32 lines = 0;
	switch (error) {
	case -128:
	case -14:
	case -12:
	case -11:
	case -10:
	case -8:
	case -7:
	case -4:
	case -1:
	case 0:
		DrawJapaneseCardLine(text, 0x36, x, y, size, color);
		lines = 1;
		break;
	case -5:
		DrawJapaneseCardLine(text, 0x17, x, y, size, color);
		DrawJapaneseCardLine(text, 0x18, x, y + size, size, color);
		lines = 2;
		break;
	case -3: {
		char channelText[256];
		if (channel == 0)
			sprintf(channelText, "%.255s", GetLocalizedInterfaceText(0x26));
		else
			sprintf(channelText, "%.255s", GetLocalizedInterfaceText(0x27));
		DrawJapaneseCardLine(channelText, x, y, size, color);
		DrawJapaneseCardLine(channelText, 0x1A, x, y + (2.0f + size), size, color);
		DrawJapaneseCardLine(channelText, 0x1C, x, 2.0f * (2.0f + size) + y, size, color);
		lines = 3;
		break;
	}
	case -9: {
		DrawJapaneseCardLine(text, JapaneseCardSlotName(channel), 0x2B, x, y, size, color);
		DrawJapaneseCardLine(text, 0x2C, x, y + size, size, color);
		DrawJapaneseCardLine(text, 0x2D, x, 2.0f * size + y, size, color);
		char blocks[16];
		sprintf(blocks, "%d", 6);
		DrawJapaneseCardLine(text, blocks, 0x2E, x, 3.0f * size + y, size, color);
		lines = 4;
		break;
	}
	case -2: {
		const char* slot = JapaneseCardSlotName(channel);
		if (retry) {
			DrawJapaneseCardLine(text, slot, 0x28, x, y, size, color);
			lines = 1;
		} else {
			DrawJapaneseCardLine(text, 0x0D, x, y, size, color);
			DrawJapaneseCardLine(text, slot, 0x28, x, y + size, size, color);
			DrawJapaneseCardLine(text, slot, 0x1B, x, 2.0f * size + y, size, color);
			DrawJapaneseCardLine(text, 0x34, x, 3.0f * size + y, size, color);
			lines = 4;
		}
		break;
	}
	}
	return lines;
}

// The save menu reserves extra space below some prompts for the choice labels.
extern "C" s32 DrawJapaneseSaveStatusMessage(s32 channel, s32 status, f32 x, f32 y, f32 size, f32 alpha)
{
	char text[256];
	s32 lines = 0;
	u32 color = (u32)(255.0f * alpha);
	switch (status) {
	case 1:
		DrawJapaneseCardLine(text, 0x2F, x, y, size, color);
		DrawJapaneseCardLine(text, 0x30, x, y + size, size, color);
		lines = 2;
		break;
	case 2:
		DrawJapaneseCardLine(text, 0x31, x, y, size, color);
		lines = 1;
		break;
	case 3:
		DrawJapaneseCardLine(text, 0x19, x, y, size, color);
		lines = 1;
		break;
	case 8:
		DrawJapaneseCardLine(text, 0x35, x, y, size, color);
		lines = 1;
		break;
	case 14:
		DrawJapaneseCardLine(text, 0x46, x, y, size, color);
		lines = 1;
		break;
	case 4:
		DrawJapaneseCardLine(text, 0x11, x, y, size, color);
		DrawJapaneseCardLine(text, 0x0F, x, y + size, size, color);
		DrawJapaneseCardLine(text, 0x10, x, 2.0f * size + y, size, color);
		lines = 3;
		break;
	case 5: {
		const char* slot = JapaneseCardSlotName(channel);
		DrawJapaneseCardLine(text, slot, 0x32, x, y, size, color);
		DrawJapaneseCardLine(text, 0x12, x, y + size, size, color);
		DrawJapaneseCardLine(text, 0x13, x, 2.0f * size + y, size, color);
		lines = 4;
		break;
	}
	case 6:
		DrawJapaneseCardLine(text, 0x44, x, y, size, color);
		DrawJapaneseCardLine(text, 0x45, x, y + size, size, color);
		lines = 3;
		break;
	case 7:
		DrawJapaneseCardLine(text, 0x14, x, y, size, color);
		DrawJapaneseCardLine(text, 0x0F, x, y + size, size, color);
		DrawJapaneseCardLine(text, 0x10, x, 2.0f * size + y, size, color);
		lines = 3;
		break;
	case 9:
		DrawJapaneseCardLine(text, 0x15, x, y, size, color);
		lines = 1;
		break;
	case 10:
		DrawJapaneseCardLine(text, 0x24, x, y, size, color);
		DrawJapaneseCardLine(text, 0x25, x, y + size, size, color);
		lines = 3;
		break;
	case 11:
		DrawJapaneseCardLine(text, 0x37, x, y, size, color);
		DrawJapaneseCardLine(text, 0x38, x, y + size, size, color);
		lines = 3;
		break;
	case 12: {
		const char* slot = JapaneseCardSlotName(channel);
		DrawJapaneseCardLine(text, slot, 0x32, x, y, size, color);
		DrawJapaneseCardLine(text, 0x33, x, y + size, size, color);
		lines = 2;
		break;
	}
	case 13: {
		const char* slot = JapaneseCardSlotName(channel);
		DrawJapaneseCardLine(text, 0x0D, x, y, size, color);
		DrawJapaneseCardLine(text, slot, 0x32, x, y + size, size, color);
		DrawJapaneseCardLine(text, 0x33, x, 2.0f * size + y, size, color);
		DrawJapaneseCardLine(text, slot, 0x1B, x, 3.0f * size + y, size, color);
		DrawJapaneseCardLine(text, 0x34, x, 4.0f * size + y, size, color);
		lines = 5;
		break;
	}
	}
	return lines;
}

static void AppendJapaneseCardSuffix(char (&text)[256], const char* suffix)
{
	size_t length = strlen(text);
	sprintf(text + length, "%.*s", (int)(sizeof(text) - length - 1), suffix);
}
#endif

struct StatusGridTextures {
	s32 locked, unlocked;
};
StatusGridTextures gStatusGridTextures[24] = {
#if defined(VERSION_JP)
	{ 192, 193 }, { 222, 223 }, { 188, 189 }, { 208, 209 }, { 200, 201 }, { 198, 199 }, { 186, 187 }, { 214, 215 },
	{ 202, 203 }, { 184, 185 }, { 182, 183 }, { 206, 207 }, { 218, 219 }, { 180, 181 }, { 226, 227 }, { 224, 225 },
	{ 196, 197 }, { 204, 205 }, { 194, 195 }, { 212, 213 }, { 190, 191 }, { 210, 211 }, { 216, 217 }, { 220, 221 },
#else
	{ 194, 195 }, { 224, 225 }, { 190, 191 }, { 210, 211 }, { 202, 203 }, { 200, 201 }, { 188, 189 }, { 216, 217 },
	{ 204, 205 }, { 186, 187 }, { 184, 185 }, { 208, 209 }, { 220, 221 }, { 182, 183 }, { 228, 229 }, { 226, 227 },
	{ 198, 199 }, { 206, 207 }, { 196, 197 }, { 214, 215 }, { 192, 193 }, { 212, 213 }, { 218, 219 }, { 222, 223 },
#endif
};
extern "C" u8* GetStatusIconDescription(s32 id);

static inline void DrawStatusGridIcon(CTexObj& tex, u32 color, f32 x, f32 y, f32 bob, f32& width, f32& height, bool selected)
{
	width  = GXGetTexObjWidth(&tex);
	width  = 0.8f * width;
	height = GXGetTexObjHeight(&tex);
	height = 0.8f * height;

	SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 7, 1);
	SetGsDepthCompareBeforeTexture(gGame.mGs.mCurRenderMode, 0);
	SetGsTevMode(gGame.mGs.mCurTevStage, 0);
	DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, x - 2.0f, selected ? bob + (y - 2.0f) : y - 2.0f, 1.0f, width, height,
	                   0, 0, 0);
	SetGsTevMode(gGame.mGs.mCurTevStage, 4);
	SetGsDepthCompareBeforeTexture(gGame.mGs.mCurRenderMode, 1);
}

extern "C" u8* GetStatusOverviewDescription(s32);
static inline void DrawStatusGridDescription(MenuStatus* menu, bool iconGrid)
{
	s32 id    = iconGrid ? menu->mIconSelection : menu->mOverviewSelection;
	f32 scale = menu->mDetailScale;
	f32 alpha = menu->mAlpha;
	// The original constructs a texture and advances its panel counter even when closed.
	static u32 frame = 0;
	CTexObj panelTexture;
	++frame;
	if (scale <= 0)
		return;
	f32 height  = 153.0f * scale;
	f32 width   = 254.0f * scale;
	f32 y       = 224.0f - height / 2.0f;
	u32 opacity = (u32)(255.0f * alpha);
	u32 color   = opacity | 0xFFFFFF00;
	DrawTexturedWindowFrame((u32)(220.0f * alpha), color, 320.0f, y, width, height, alpha, 1.0f, 1.0f);
	y += scale;
	f32 size = scale * 17.0f, offset = scale * 24.0f;
	char text[256];
	sprintf(text, "%.255s", iconGrid ? GetStatusIconDescription(id) : GetStatusOverviewDescription(id));
	f32 w       = MeasureGameTextWidth(&gGame.mGs, text, 1, size);
	f32 padding = 6.0f * scale;
	DrawGsProportionalTextFormat(&gGame.mGs, color, text, 320.0f - w / 2.0f, (4.0f * scale + (offset + (5.0f * size + y))) - padding, 0.0f,
	                             size, size);
	sprintf(text, "%.255s", iconGrid ? GetStatusIconName(id) : GetStatusOverviewLabel(id));
	f32 helpSize = 2.0f * scale + size;
	w            = MeasureGameTextWidth(&gGame.mGs, text, 1, helpSize);
	DrawGsProportionalTextFormat(&gGame.mGs, opacity | 0xFFAA0000, text, 320.0f - w / 2.0f, ((4.0f * size + y) + offset) - padding, 0.0f,
	                             helpSize, helpSize);
}

extern "C" void DrawStatusIconGrid(MenuStatus* menu)
{
	if (menu->mMode == 8 || menu->mMode == 4) {
		f32 left     = ((menu->mGridX - 42.0f) + menu->mOffsetX) - 201.0f;
		f32 top      = ((180.0f + menu->mGridY) + menu->mOffsetY) - 81.0f;
		s32 selected = menu->mPhase == 100 ? -1 : menu->mIconSelection;
		CTexObj tex;
		static u32 frame = 0;
		++frame;

		SetGsBlendMode(gGame.mGs.mCurRenderMode, 1, 4, 5, 15);
		SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 7, 0);
		for (int i = 0; i < 24; i++) {
			f32 x     = 2.0f + ((i % 6) * 80.0f + left);
			f32 y     = 2.0f + ((i / 6) * 60.0f + top);
			u32 color = (u32)(255.0f * menu->mAlpha) | 0xFFFFFF00;
			if (gStatusIconUnlocked[i] != 0)
				tex = gGame.mTpl.mTex.Init(&((TPLPalette*)gGame.mTpl.mBuffer)->descriptorArray[gStatusGridTextures[i].unlocked], 0);
			else
				tex = gGame.mTpl.mTex.Init(&((TPLPalette*)gGame.mTpl.mBuffer)->descriptorArray[gStatusGridTextures[i].locked], 0);
			f32 bob = (f32)sin(0.017453292f * (((s32)(frame * 20)) % 360)) * 4.0f;
			SetGsTevMode(gGame.mGs.mCurTevStage, 0);
			f32 width, height;
			if (selected == i)
				DrawStatusGridIcon(tex, color, x, y, bob, width, height, true);
			else
				DrawStatusGridIcon(tex, color, x, y, bob, width, height, false);
			if (selected == i) {
				SetGsBlendMode(gGame.mGs.mCurRenderMode, 1, 2, 4, 15);
				SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 2, 0);
				f32 wave  = (f32)sin(3.1415927f * menu->mPulsePhase);
				f32 pulse = (0.2f * wave + 0.2f) * menu->mAlpha;
				if (pulse < 0)
					pulse = 0;
				color = (u32)(255.0f * pulse) | 0xFFFFFF00;
				DrawGsQuad(&gGame.mGs, color, color, color, color, x, y + bob, 1.0f, width, height, 0, 0, 0);
				SetGsBlendMode(gGame.mGs.mCurRenderMode, 1, 4, 5, 15);
				SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 7, 0);
			}
		}
		DrawStatusGridDescription(menu, true);
	}
}

extern "C" void DrawPlayerStatusValue(s32 icon, s32 value, u8 selected, f32 x, f32 y, f32 alpha, f32 phase);
extern "C" void DrawStatusGauge(f32 value, f32 x, f32 y, f32 alpha);
static inline void LoadOverviewTexture(CTexObj& tex, s32 icon)
{
	tex = gGame.mTpl.mTex.Init(&((TPLPalette*)gGame.mTpl.mBuffer)->descriptorArray[icon - StatusTextureOffset], 0);
}
static inline void BeginOverviewTexture(CTexObj& tex)
{
	PSMTXScale(tex.mMtx[1], 1, 1, 1);
	tex.mMtxDirty = TRUE;
	PSMTXTrans(tex.mMtx[0], 0, 0, 0);
	tex.mMtxDirty = TRUE;
	SetGsTevMode(gGame.mGs.mCurTevStage, 0);
}
static inline void DrawOverviewTexture(CTexObj& tex, u32 color, f32 x, f32 y, f32 width, f32 height)
{
	DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, x, y, 0, width, height, 0, 0, 0);
	SetGsTevMode(gGame.mGs.mCurTevStage, 4);
}
static inline void DrawStatusOverviewEntry(MenuStatus* menu, s32 icon, s32 cell, s32 selected, f32 x, f32 y)
{
	DrawPlayerStatusIcon(icon - StatusTextureOffset, gStatusEntryAvailable[cell], cell == selected, x, y, menu->mAlpha, 0.8f,
	                     menu->mPulsePhase);
}
static inline void DrawOverviewStats(MenuStatus* menu, s32 selected, f32 x, f32 y)
{
	f32 statX = 20.0f + (x - 280.0f);
	DrawPlayerStatusValue(0xB4 - StatusTextureOffset, gGame.GetPlayer()->mParams.mRank, selected == 0, statX, (y - 80.0f) + 8.0f,
	                      menu->mAlpha, menu->mPulsePhase);
	DrawPlayerStatusValue(0x9A - StatusTextureOffset, gGame.GetPlayer()->mParams.mExperience, selected == 2, statX,
	                      40.0f + (8.0f + ((y - 80.0f) + 44.0f)), menu->mAlpha, menu->mPulsePhase);
	DrawPlayerStatusValue(0x98 - StatusTextureOffset, gGame.GetPlayer()->mParams.mMoney, selected == 3, statX,
	                      40.0f + (8.0f + ((y - 80.0f) + 88.0f)), menu->mAlpha, menu->mPulsePhase);
	DrawPlayerStatusValue(0x99 - StatusTextureOffset, gGame.GetPlayer()->mParams.mScrap, selected == 4, statX,
	                      40.0f + (8.0f + ((y - 80.0f) + 132.0f)), menu->mAlpha, menu->mPulsePhase);
}
extern "C" void DrawPlayerStatusOverview(MenuStatus* menu)
{
	if (menu->mMode == 9 || menu->mMode == 4) {
		CTexObj tex;
		f32 x        = (menu->mOverviewX - 42.0f) + menu->mOffsetX;
		f32 y        = (180.0f + menu->mOverviewY) + menu->mOffsetY;
		s32 selected = menu->mPhase == 100 ? -1 : menu->mOverviewSelection;
		DrawOverviewStats(menu, selected, x, y);
		LoadOverviewTexture(tex, 173);
		u16 width = GXGetTexObjWidth(&tex), height = GXGetTexObjHeight(&tex);
		BeginOverviewTexture(tex);
		u32 color = (u32)(255.0f * menu->mAlpha) | 0xC8C8C800;
		DrawOverviewTexture(tex, color, ((x - 24.0f) - 48.0f) - 12.0f, y + 14.0f, 0.6f * (1.1f * width), 0.6f * height);
		f32 left = ((48.0f + x) - 48.0f) - 12.0f;
		f32 row0 = (y - 82.0f) + 14.0f;
		DrawStatusOverviewEntry(menu, 0xAA, 5, selected, left, row0);
		f32 row1 = 14.0f + ((y - 82.0f) + 52.0f);
		DrawStatusOverviewEntry(menu, 0xB3, 6, selected, left, row1);
		f32 row2 = 14.0f + ((y - 82.0f) + 104.0f);
		DrawStatusOverviewEntry(menu, 0xA9, 7, selected, left, row2);
		f32 row3 = 14.0f + ((y - 82.0f) + 156.0f);
		DrawStatusOverviewEntry(menu, 0xAC, 8, selected, left, row3);
		LoadOverviewTexture(tex, 178);
		f32 w = 1.4f * (1.1f * GXGetTexObjWidth(&tex)), h = 1.4f * GXGetTexObjHeight(&tex);
		BeginOverviewTexture(tex);
		if (menu->mIconLit)
			color = (u32)(255.0f * menu->mAlpha) | 0xC8C8C800;
		else
			color = (u32)(255.0f * menu->mAlpha);
		f32 center = ((180.0f + (x - 24.0f)) - 24.0f) - 12.0f;
		f32 middle = (y - 18.0f) + 14.0f;
		DrawOverviewTexture(tex, color, center, middle, w, h);
		if (gEventFlags.mFlags[0] & 0x10000000) {
			LoadOverviewTexture(tex, 245);
			width  = GXGetTexObjWidth(&tex);
			height = GXGetTexObjHeight(&tex);
			BeginOverviewTexture(tex);
			color = (u32)(255.0f * menu->mAlpha) | 0xC8C8C800;
			DrawOverviewTexture(tex, color, 18.0f + center, 56.5f + middle, 1.4f * (1.1f * width), 1.4f * height);
		}
		if (menu->mShowGauge)
			DrawStatusGauge(menu->mGaugeValue, (11.0f + ((140.0f + x) - 120.0f)) - 12.0f, (114.0f + y) - 22.0f, menu->mAlpha);
		f32 right = (192.0f + (72.0f + x)) - 12.0f;
		DrawStatusOverviewEntry(menu, 0xAE, 9, selected, right, row0);
		DrawStatusOverviewEntry(menu, 0xAF, 10, selected, right, row1);
		DrawStatusOverviewEntry(menu, 0xB0, 11, selected, right, row2);
		DrawStatusOverviewEntry(menu, 0xB1, 12, selected, right, row3);
		DrawStatusGridDescription(menu, false);
	}
}

extern "C" void DrawPlayerStatusValue(s32 icon, s32 value, u8 selected, f32 x, f32 y, f32 alpha, f32 phase)
{
	if (!(alpha > 0.0f))
		return;

	CGs& gs = gGame.mGs;
	SetGsBlendMode(gs.mCurRenderMode, 1, 4, 5, 15);
	SetGsDepthMode(gs.mCurRenderMode, 1, 7, 0);
	CTexObj tex;
	f32 bob = 0.0f;
	if (selected) {
		static u32 frame = 0;
		++frame;
		bob = 4.0f * (f32)sin(0.017453292f * (((s32)(frame * 20)) % 360));
	}
	s32 places = 1;
	for (;; places++) {
		f32 divisor = 1.0f;
		for (s32 j = 0; j < places; j++)
			divisor *= 10.0f;
		if ((s32)(value / divisor) == 0)
			break;
	}

	Vec outline[65];
	f32 centerY = y - 7.2f;
	for (int i = 0; i < 65; i++) {
		f32 angle    = 0.09817477f * i - 1.5707964f;
		f32 sine     = (f32)sin(angle);
		f32 cosine   = (f32)cos(angle);
		outline[i].x = sine * 14.4f + x;
		outline[i].y = -cosine * 14.4f + centerY;
		outline[i].z = 0.0f;
	}
	f32 opacity = 255.0f * alpha;
	u32 color   = (u32)opacity;
	SetGsLineWidth(gs.mCurRenderMode, 18, 5);
	DrawGsLineStrip(&gs, outline, color, 65);
	SetGsLineWidth(gs.mCurRenderMode, 12, 5);

	LoadOverviewTexture(tex, 125);
	f32 width    = 1.2f * GXGetTexObjWidth(&tex);
	outline[0].x = x;
	outline[0].y = centerY + 14.4f;
	outline[1].x = x + (places * (width - 3.6f) + 34.0f);
	outline[1].y = outline[0].y;
	SetGsLineWidth(gs.mCurRenderMode, 18, 5);
	DrawGsLineStrip(&gs, outline, color, 2);
	SetGsLineWidth(gs.mCurRenderMode, 12, 5);

	LoadOverviewTexture(tex, 155);
	SetGsTevMode(gs.mCurTevStage, 0);
	u16 markerWidth = GXGetTexObjWidth(&tex), markerHeight = GXGetTexObjHeight(&tex);
	color |= 0xC8C8C800;
	DrawOverviewTexture(tex, color, outline[1].x, outline[1].y, 1.2f * markerWidth, 1.2f * markerHeight);

	f32 scale = icon == 180 - StatusTextureOffset ? 0.7f : 0.8f;
	tex       = gGame.mTpl.mTex.Init(&((TPLPalette*)gGame.mTpl.mBuffer)->descriptorArray[icon], 0);
	SetGsDepthMode(gs.mCurRenderMode, 1, 7, 1);
	SetGsDepthCompareBeforeTexture(gs.mCurRenderMode, 0);
	SetGsTevMode(gs.mCurTevStage, 0);
	width      = scale * GXGetTexObjWidth(&tex);
	f32 height = scale * GXGetTexObjHeight(&tex);
	f32 iconY  = bob + (y - 9.0f);
	if (icon == 154 - StatusTextureOffset)
		iconY = 4.0f + iconY;
	DrawGsTexturedQuad(&gs, &tex, color, color, color, color, x, iconY, 1.0f, width, height, 0, 0, 0);
	SetGsTevMode(gs.mCurTevStage, 4);
	SetGsDepthCompareBeforeTexture(gs.mCurRenderMode, 1);
	if (selected) {
		SetGsBlendMode(gs.mCurRenderMode, 1, 2, 4, 15);
		SetGsDepthMode(gs.mCurRenderMode, 1, 2, 0);
		f32 pulse = alpha * (0.2f * (f32)sin(3.1415927f * phase) + 0.2f);
		if (pulse < 0.0f)
			pulse = 0.0f;
		u32 highlight = (u32)(255.0f * pulse) | 0xFFFFFF00;
		DrawGsQuad(&gs, highlight, highlight, highlight, highlight, x, iconY, 1.0f, width, height, 0, 0, 0);
		SetGsBlendMode(gs.mCurRenderMode, 1, 4, 5, 15);
		SetGsDepthMode(gs.mCurRenderMode, 1, 7, 0);
	}
	LoadOverviewTexture(tex, 125);
	x -= 12.0f;
	y -= 4.0f;
	width  = 1.2f * GXGetTexObjWidth(&tex);
	height = 1.2f * (GXGetTexObjHeight(&tex) / 10);
	PSMTXScale(tex.mMtx[1], 1, 0.1f, 1);
	tex.mMtxDirty = TRUE;
	u8 a          = (u8)opacity;
	f32 offset    = places * (width - 3.6f) - 98.0f * 1.2f;
	for (s32 i = 0; i < 7; i++) {
		f32 divisor = 1.0f;
		for (s32 j = 0; j < i; j++)
			divisor *= 10.0f;
		s32 digit = (s32)(value / divisor);
		if (i != 0 && digit == 0)
			continue;
		PSMTXTrans(tex.mMtx[0], 0, 0.1f * (digit % 10), 0);
		tex.mMtxDirty = TRUE;
		SetGsTevMode(gs.mCurTevStage, 0);
		u32 top, bottom;
		switch (icon) {
		case 154 - StatusTextureOffset:
			top    = 0xCECEFF00 | a;
			bottom = 0xE037EE00 | a;
			break;
		case 152 - StatusTextureOffset:
			top    = 0xEFEBC500 | a;
			bottom = 0xEDA83600 | a;
			break;
		case 153 - StatusTextureOffset:
			top    = 0xCECEFF00 | a;
			bottom = 0x5959FF00 | a;
			break;
		case 180 - StatusTextureOffset:
			top    = 0xBFDDFB00 | a;
			bottom = 0x0E72D600 | a;
			break;
		default:
			top = bottom = 0xFFFFFF00 | a;
		}
		DrawGsTexturedQuad(&gs, &tex, top, top, bottom, bottom, offset + (7.0f * width + (x - i * (width - 3.6f))), y - 2.4f, 0, width,
		                   height, 0, 0, 0);
		SetGsTevMode(gs.mCurTevStage, 4);
	}
	SetGsTevMode(gs.mCurTevStage, 4);
}

static inline void SetGaugeQuad(Vec* vertices, f32 left, f32 right, f32 top, f32 bottom)
{
	vertices[0].x = left;
	vertices[0].y = top;
	vertices[1].x = right;
	vertices[1].y = top;
	vertices[2].x = right;
	vertices[2].y = bottom;
	vertices[3].x = left;
	vertices[3].y = bottom;
}

static inline void DrawGaugeNumber(CTexObj& tex, s32 number, u32 color, f32 x, f32 y, f32 width, f32 height, f32 offset, f32 shift)
{
	for (s32 i = 0; i < 5; i++) {
		f32 divisor = 1.0f;
		for (s32 j = 0; j < i; j++)
			divisor *= 10.0f;
		s32 digit = (s32)(number / divisor);
		if (i != 0 && digit == 0)
			continue;
		PSMTXTrans(tex.mMtx[0], 0, 0.1f * (digit % 10), 0);
		tex.mMtxDirty = TRUE;
		SetGsTevMode(gGame.mGs.mCurTevStage, 0);
		DrawOverviewTexture(tex, color, (offset + (7.0f * width + (x - i * (width - 3.6f)))) - shift, y - 2.4f, width, height);
	}
}

extern "C" void DrawStatusGauge(f32 value, f32 x, f32 y, f32 alpha)
{
	if (!(alpha > 0.0f))
		return;
	CTexObj tex;
	CGs& gs = gGame.mGs;
	SetGsChannelEnabled(&gs, 0, 0);
	SetGsBlendMode(gs.mCurRenderMode, 1, 4, 5, 15);
	SetGsDepthMode(gs.mCurRenderMode, 0, 3, 0);
	SetGsLineWidth(gs.mCurRenderMode, 6, 5);

	f32 left         = 16.0f + x;
	f32 right        = 168.0f + left;
	f32 fill         = 168.0f * value + left;
	f32 bottom       = 16.0f + y;
	Vec vertices[20] = {};
	SetGaugeQuad(vertices, left, right, y, bottom);
	SetGaugeQuad(vertices + 4, left, fill, y, bottom);
	SetGaugeQuad(vertices + 8, fill, right, y, bottom);
	f32 shadowX = 4.0f + left;
	f32 shadowY = 4.0f + y;
	SetGaugeQuad(vertices + 12, shadowX, 168.0f + shadowX, shadowY, 16.0f + shadowY);
	SetGaugeQuad(vertices + 16, left, fill, shadowY, (bottom - 4.0f) - 1.0f);
	u32 shade = (u32)(100.0f * alpha);
	DrawSolidQuad(&gs, vertices + 12, shade | 0x0A0A0A00);
	f32 opacity = 255.0f * alpha;
	u32 a       = (u32)opacity;
	DrawGsLineStrip(&gs, vertices, a | 0xC8C8C800, 5);
	DrawSolidQuad(&gs, vertices, (u32)(40.0f * alpha) | 0xA0FFFF00);
	DrawSolidQuad(&gs, vertices + 16, shade | 0x40FF8000);
	SetGsLineWidth(gs.mCurRenderMode, 12, 5);

	u32 white = a | 0xFFFFFF00;
	LoadOverviewTexture(tex, 242);
	f32 width  = 0.85f * GXGetTexObjWidth(&tex);
	f32 height = 0.85f * GXGetTexObjHeight(&tex);
	SetGsTevMode(gs.mCurTevStage, 0);
	f32 capY = (8.0f + y) - 0.5f;
	DrawGsTexturedQuad(&gs, &tex, white, white, white, white, 2.0f + (left - width / 2.0f), capY, 0, width, height, 0, 0, 3.1415927f);
	SetGsTevMode(gs.mCurTevStage, 4);
	LoadOverviewTexture(tex, 241);
	width  = 0.95f * GXGetTexObjWidth(&tex);
	height = 0.95f * GXGetTexObjHeight(&tex);
	SetGsTevMode(gs.mCurTevStage, 0);
	DrawGsTexturedQuad(&gs, &tex, white, white, white, white, (width * 0.5f + right) - 4.0f, capY, 0, width, height, 0, 0, 3.1415927f);
	SetGsTevMode(gs.mCurTevStage, 4);

	s32 current = (s32)(10000.0f * value);
	x += 121.2f;
	y -= 4.8f;
	LoadOverviewTexture(tex, 125);
	width  = 1.2f * GXGetTexObjWidth(&tex);
	height = 1.2f * (GXGetTexObjHeight(&tex) / 10);
	PSMTXScale(tex.mMtx[1], 1, 0.1f, 1);
	tex.mMtxDirty = TRUE;
	u32 textColor = 0xFFFFFF00 | (u8)opacity;
	f32 offset    = (3.0f * (width - 3.6f) - 98.0f * 1.2f) - 16.0f;
	DrawGaugeNumber(tex, 10000, textColor, x, y, width, height, offset, 0);
	DrawGaugeNumber(tex, current, textColor, x, y, width, height, offset, 6.0f * (width - 3.6f));
	SetGsTevMode(gs.mCurTevStage, 4);
	LoadOverviewTexture(tex, 168);
	u16 slashWidth = GXGetTexObjWidth(&tex), slashHeight = GXGetTexObjHeight(&tex);
	SetGsTevMode(gs.mCurTevStage, 0);
	DrawOverviewTexture(tex, textColor, offset + (7.0f * width + (x - 5.0f * (width - 3.6f))), y - 1.2f, 0.8f * (1.2f * slashWidth),
	                    0.8f * (1.2f * slashHeight));
}

static inline void StrokeStatusOutline(Vec* outline, u32 color, u16 count)
{
	CGs& gs = gGame.mGs;
	SetGsLineWidth(gs.mCurRenderMode, 18, 5);
	DrawGsLineStrip(&gs, outline, color, count);
	SetGsLineWidth(gs.mCurRenderMode, 12, 5);
}

extern "C" void DrawSeparatedStatusValue(s32 icon, s32 value, bool selected, f32 x, f32 y, f32 alpha)
{
	if (!(alpha > 0.0f))
		return;
	CTexObj tex;
	CGs& gs = gGame.mGs;
	f32 bob = 0.0f;
	if (selected) {
		static u32 frame = 0;
		++frame;
		bob = 4.0f * (f32)sin(0.017453292f * (((s32)(frame * 20)) % 360));
	}
	s32 places    = 4 + (value >= 1000);
	bool skipTens = false;
	if ((value % 100) / 10 == 0)
		skipTens = true;
	Vec outline[65];
	f32 centerY = y - 7.2f;
	for (int i = 0; i < 65; i++) {
		f32 angle    = 0.09817477f * i - 1.5707964f;
		f32 sine     = (f32)sin(angle);
		f32 cosine   = (f32)cos(angle);
		outline[i].x = sine * 14.4f + x;
		outline[i].y = -cosine * 14.4f + centerY;
		outline[i].z = 0.0f;
	}
	f32 opacity = 255.0f * alpha;
	u32 color   = (u32)opacity;
	StrokeStatusOutline(outline, color, 65);
	s32 visible = places;
	if (skipTens)
		--visible;
	LoadOverviewTexture(tex, 125);
	f32 width    = 1.2f * GXGetTexObjWidth(&tex);
	outline[0].x = x;
	outline[0].y = centerY + 14.4f;
	outline[1].x = x + (visible * (width - 3.6f) + 34.0f);
	outline[1].y = outline[0].y;
	StrokeStatusOutline(outline, color, 2);

	LoadOverviewTexture(tex, 155);
	SetGsTevMode(gs.mCurTevStage, 0);
	u16 markerWidth = GXGetTexObjWidth(&tex), markerHeight = GXGetTexObjHeight(&tex);
	color |= 0xC8C8C800;
	DrawOverviewTexture(tex, color, outline[1].x, outline[1].y, 1.2f * markerWidth, 1.2f * markerHeight);

	tex = gGame.mTpl.mTex.Init(&((TPLPalette*)gGame.mTpl.mBuffer)->descriptorArray[icon], 0);
	SetGsTevMode(gs.mCurTevStage, 0);
	width      = 0.8f * GXGetTexObjWidth(&tex);
	f32 height = 0.8f * GXGetTexObjHeight(&tex);
	f32 iconY  = bob + (y - 9.0f);
	if (icon == 154 - StatusTextureOffset)
		DrawGsTexturedQuad(&gs, &tex, color, color, color, color, x, 4.0f + iconY, 0, width, height, 0, 0, 0);
	else
		DrawGsTexturedQuad(&gs, &tex, color, color, color, color, x, iconY, 0, width, height, 0, 0, 0);
	LoadOverviewTexture(tex, 125);
	x -= 12.0f;
	y -= 4.0f;
	width  = 1.2f * GXGetTexObjWidth(&tex);
	height = 1.2f * (GXGetTexObjHeight(&tex) / 10);
	PSMTXScale(tex.mMtx[1], 1, 0.1f, 1);
	tex.mMtxDirty = TRUE;
	u32 white     = 0xFFFFFF00 | (u8)opacity;
	f32 offset    = (places - 1) * (width - 3.6f) - 98.0f * 1.2f;
	for (s32 i = 0; i < 4; i++) {
		f32 divisor = 1.0f;
		for (s32 j = 0; j < i; j++)
			divisor *= 10.0f;
		s32 digit = (s32)(value / divisor) % 10;
		if ((i == 3 || i == 1) && digit == 0)
			continue;
		PSMTXTrans(tex.mMtx[0], 0, 0.1f * digit, 0);
		tex.mMtxDirty = TRUE;
		SetGsTevMode(gs.mCurTevStage, 0);
		f32 gap = i < 2 ? width - 3.6f : 0.0f;
		if (skipTens && i == 0)
			gap -= width - 3.6f;
		DrawOverviewTexture(tex, white, gap + (offset + (7.0f * width + (x - i * (width - 3.6f)))), y - 2.4f, width, height);
	}
	SetGsTevMode(gs.mCurTevStage, 4);
	LoadOverviewTexture(tex, 168);
	u16 sw = GXGetTexObjWidth(&tex), sh = GXGetTexObjHeight(&tex);
	SetGsTevMode(gs.mCurTevStage, 0);
	DrawOverviewTexture(tex, white, offset + (7.0f * width + (x - (width - 3.6f))), y - 1.2f, 0.8f * (1.2f * sw), 0.8f * (1.2f * sh));
}

// The atlas is measured twice; the discard dialog uses the enlarged sample.
static inline f32 DiscardDigitExtent(s32 normal, s32 enlarged, f32 mix)
{
	return (1.0f - mix) * normal + (1.4f * mix) * enlarged;
}
static inline void DrawDiscardDigit(MenuStatus* menu, CTexObj& tex, s32 digit, f32 x, f32 y, f32 width, f32 height, f32 alpha)
{
	PSMTXTrans(tex.mMtx[0], 0, 0.1f * digit, 0);
	tex.mMtxDirty = TRUE;
	SetGsTevMode(gGame.mGs.mCurTevStage, 0);
	u32 color = (u32)((255.0f * menu->mAlpha) * alpha) | 0xC8C8C800;
	DrawOverviewTexture(tex, color, x, y, width, height);
}

static inline void DrawDiscardOutline(MenuStatus* menu, f32 x, f32 y)
{
	f32 left = x - 120.0f, top = y - 72.0f;
	f32 right = 240.0f + left, bottom = 144.0f + top;
	f32 depth      = -10.0f;
	Vec outline[5] = { (Vec) { left, top, depth }, (Vec) { right, top, depth }, (Vec) { right, bottom, depth },
		               (Vec) { left, bottom, depth }, (Vec) { left, top, depth } };
	DrawGsLineStrip(&gGame.mGs, outline, (u32)(255.0f * menu->mAlpha) | 0xC8282800, 5);
}
extern "C" void DrawDiscardCountPanel(MenuStatus* menu)
{
	f32 wave = 2.0f * (f32)sin(0.017453292f * (((s32)((u32)menu->mAnimationFrame * 24)) % 360));
	f32 x    = (240.0f + ((menu->mDiscardPanelX - 52.0f) + menu->mOffsetX)) - 204.0f;
	f32 y    = (144.0f + (((180.0f + menu->mDiscardPanelY) + menu->mOffsetY) - 96.0f)) - 36.0f;
	DrawMenuRectWithShadow(menu, (u32)(255.0f * menu->mAlpha) | 0xFFB62800, x, y, 240.0f, 144.0f);
	if (menu->mCount == 0) {
		DrawDiscardOutline(menu, x, y);
	}
	// All three retail objects pass stack buffers to the label renderer without writing any text.
	char label[256];
	label[0]      = '\0';
	u32 halfWidth = (24 * (strlen(label) / 2 - 1)) / 2;
	f32 labelX    = 12.0f + ((12.0f + (4.0f + ((40.0f + ((72.0f + x) - halfWidth)) - 120.0f))) - 20.0f);
	f32 labelY    = (12.0f + ((2.0f + (((22.0f + y) - 12.0f) - 2.0f)) - 22.0f)) - 36.0f;
	DrawDoubleByteMenuText(label, labelX, labelY, -10.0f, 24.0f, 24.0f, menu->mAlpha);

	CTexObj tex;
	f32 radius = 30.0f + wave;
	for (s32 i = 0; i < 2; i++) {
		f32 angle = 3.1415927f * i + 1.5707964f;
		f32 sine = (f32)sin(angle), cosine = (f32)cos(angle);
		SetGsTevMode(gGame.mGs.mCurTevStage, 0);
		LoadOverviewTexture(tex, 128);
		u16 width = GXGetTexObjWidth(&tex), height = GXGetTexObjHeight(&tex);
		u32 color = (u32)(255.0f * menu->mAlpha) | 0xFFEA0000;
		DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, radius * sine + x, 16.0f + (radius * cosine + y), 0,
		                   1.4f * (1.1f * width), 1.4f * height, 0, 0, 3.1415927f + angle);
		SetGsTevMode(gGame.mGs.mCurTevStage, 4);
	}

	switch (menu->mPhase) {
	case 1: {
		LoadOverviewTexture(tex, 125);
		SetGsTevMode(gGame.mGs.mCurTevStage, 0);
		s32 count  = menu->mCount;
		u16 firstW = GXGetTexObjWidth(&tex), secondW = GXGetTexObjWidth(&tex);
		f32 width  = DiscardDigitExtent(firstW, secondW, 1);
		u16 firstH = GXGetTexObjHeight(&tex), secondH = GXGetTexObjHeight(&tex);
		f32 height = DiscardDigitExtent(firstH / 10, secondH / 10, 1);
		PSMTXScale(tex.mMtx[1], 1, 0.1f, 1);
		tex.mMtxDirty = TRUE;
		if (count >= 10) {
			DrawDiscardDigit(menu, tex, count / 10, x - width / 3.0f, 14.0f + y, width, height, 1.0f);
			DrawDiscardDigit(menu, tex, count % 10, x + width / 3.0f, 14.0f + y, width, height, 1.0f);
		} else {
			DrawDiscardDigit(menu, tex, count, x, 14.0f + y, width, height, 1.0f);
		}
		break;
	}
	case 2: {
		char message[256];
		message[0] = '\0';
		// Retail measures the confirmation label even though it draws it at a fixed position.
		(void)strlen(message);
		DrawDoubleByteMenuText(message, x, 14.0f + y, -10.0f, 24.0f, 24.0f, menu->mAlpha);
		break;
	}
	}
}

extern "C" s32 GetAttachmentCountById(AttachmentValues*, s32);
extern "C" void SetAttachmentCountById(AttachmentValues*, s32, s32);

void MenuPC::State3(u32 frames)
{
	MenuPC* menu = this;
	s32 count;
	CPad pad(0);
	MenuPCItem* table = sShopHistoryEntries;
	count             = CountPCShopItems(table);
	ApproachHudValue(&menu->mPanelAlpha, &menu->mPanelTarget, 0.08f * frames);
	switch (menu->mPhase) {
	case 0:
		if (count > 1) {
			if (menu->mPurchaseSelection == 0) {
				if (pad.mPad->mRepeat & 0x20004) {
					s32 next = menu->mPurchaseSelection + 1;
					if (next <= 19) {
						menu->mPurchaseSelection = next;
						PlayGameSound(&gGame.mSound, 0x87, 0xFF);
					}
				}
				if (pad.mPad->mRepeat & 0x10008) {
					CountPCShopItems(table);
					if (gShopHistoryPage.mStart > 0) {
						menu->mScrollTarget = 17.0f;
						menu->mPhase        = 20;
						break;
					}
				}
			} else if (menu->mPurchaseSelection == 8) {
				if (pad.mPad->mRepeat & 0x10008) {
					if (--menu->mPurchaseSelection < 0)
						menu->mPurchaseSelection = 19;
					PlayGameSound(&gGame.mSound, 0x87, 0xFF);
				}
				if (pad.mPad->mRepeat & 0x20004) {
					if (gShopHistoryPage.mStart < PCShopMaxPage(table) - 1) {
						menu->mScrollTarget = -17.0f;
						menu->mPhase        = 10;
						break;
					}
				}
			} else {
				if (pad.mPad->mRepeat & 0x10008) {
					if (--menu->mPurchaseSelection < 0)
						menu->mPurchaseSelection = 19;
					PlayGameSound(&gGame.mSound, 0x87, 0xFF);
				}
				if (pad.mPad->mRepeat & 0x20004) {
					s32 next = menu->mPurchaseSelection + 1;
					if (next <= 19) {
						menu->mPurchaseSelection = next;
						PlayGameSound(&gGame.mSound, 0x87, 0xFF);
					}
				}
			}
		}
		if (pad.mPad->mTrigger & 0x100) {
			if (gShopHistoryPage.mRows[menu->mPurchaseSelection].mItem >= 0
			    && gShopHistoryPage.mRows[menu->mPurchaseSelection].mCount > 0) {
				PlayGameSound(&gGame.mSound, 0x88, 0xFF);
				menu->mPanelTarget = 1.0f;
				menu->mPhase++;
				menu->mYesNo           = 0;
				menu->mTargetListAlpha = 0.5f;
				break;
			}
			PlayGameSound(&gGame.mSound, 0x85, 0xFF);
			break;
		}
		if (pad.mPad->mTrigger & 0x200) {
			if (menu->mShopOnly) {
				ResetPCText(menu, 11);
				menu->mTargetFrameHeight = 0.0f;
				RestorePCHistory(menu);
			} else {
				ResetPCText(menu, 2);
				PlayGameSound(&gGame.mSound, 0x86, 0xFF);
			}
			menu->mPanelTarget = 0.0f;
		}
		break;
	case 1:
		if (pad.mPad->mRepeat & 0x10008) {
			menu->mYesNo--;
			if (menu->mYesNo < 0)
				menu->mYesNo = 1;
			PlayGameSound(&gGame.mSound, 0x87, 0xFF);
		} else if (pad.mPad->mRepeat & 0x20004) {
			menu->mYesNo++;
			if (menu->mYesNo > 1)
				menu->mYesNo = 0;
			PlayGameSound(&gGame.mSound, 0x87, 0xFF);
		} else if (pad.mPad->mTrigger & 0x100) {
			if (menu->mYesNo == 0) {
				MenuPCItem* entry;
				s32 item  = gShopHistoryPage.mIndex[menu->mPurchaseSelection];
				s32 money = GetMoney();
				entry     = &sShopHistoryEntries[item];
				if (entry->mPrice <= money) {
					money -= entry->mPrice;
					gGame.GetPlayer()->mParams.SetMoney(money);
					menu->mMoneyDisplay.target = money;
					menu->mYesNo               = 2;
					PlayGameSound(&gGame.mSound, 0x30009, 0xFF);
					s16 id = entry->mItem;
					if (entry->mIsAttachment) {
						if (id >= 0 && id < 9) {
							s32 level = GetAttachmentCountById(&gGame.GetPlayer()->mAttachments, id) + 1;
							if (level >= 3)
								level = 3;
							SetAttachmentCountById(&gGame.GetPlayer()->mAttachments, id, level);
						}
					} else if (id >= 0 && id < 0x9F) {
						s32 index            = FindOwnedInventoryItemIndex(gGame.GetPlayer()->mParams.mInventory.mItems, id);
						Inventory& inventory = gGame.GetPlayer()->mParams.mInventory;
						s16 quantity         = index < 0 ? 0 : inventory.mItems[index].mCount;
						SetOrAddInventoryItemCount(&inventory, id, quantity + 1, FALSE);
					}
					if (entry->mCount > 0)
						entry->mCount--;
					if (entry->mItem != 0x58 && entry->mItem >= 0x57 && entry->mItem < 0x5B) {
						for (s32 i = 0; i < 20; i++) {
							MenuPCItem& other = table[i];
							if (other.mItem >= 0 && !other.mIsAttachment && other.mItem != 0x58 && other.mItem >= 0x57 && other.mItem < 0x5B
							    && other.mItem != entry->mItem) {
								other.mCount = 0;
								SetOrAddInventoryItemCount(&gGame.GetPlayer()->mParams.mInventory, other.mItem, 0, FALSE);
							}
						}
					}
					menu->mPhase++;
					return;
				}
				PlayGameSound(&gGame.mSound, 0x85, 0xFF);
			} else {
				menu->mPhase       = 0;
				menu->mPanelTarget = 0.0f;
				PlayGameSound(&gGame.mSound, 0x88, 0xFF);
				menu->mTargetListAlpha = 1.0f;
			}
		}
		if (pad.mPad->mTrigger & 0x200) {
			PlayGameSound(&gGame.mSound, 0x86, 0xFF);
			menu->mPhase           = 0;
			menu->mPanelTarget     = 0.0f;
			menu->mTargetListAlpha = 1.0f;
		}
		break;
	case 2:
		if (pad.mPad->mTrigger & 0x300) {
			menu->mPhase       = 0;
			menu->mPanelTarget = 0.0f;
			PlayGameSound(&gGame.mSound, 0x88, 0xFF);
			menu->mTargetListAlpha = 1.0f;
			PopulateShopHistoryPage(&gShopHistoryPage);
		}
		break;
	case 10:
		if (menu->mScroll == menu->mScrollTarget) {
			if (gShopHistoryPage.mStart < ResetPCShopPage(menu, table) - 1) {
				gShopHistoryPage.mStart++;
				PopulateShopHistoryPage(&gShopHistoryPage);
				PlayGameSound(&gGame.mSound, 0x87, 0xFF);
			}
			menu->mScroll       = 0.0f;
			menu->mScrollTarget = 0.0f;
		}
		break;
	case 20:
		if (menu->mScroll == menu->mScrollTarget) {
			menu->mPhase = 0;
			CountPCShopItems(table);
			if (gShopHistoryPage.mStart > 0) {
				gShopHistoryPage.mStart--;
				PopulateShopHistoryPage(&gShopHistoryPage);
				PlayGameSound(&gGame.mSound, 0x87, 0xFF);
			}
			menu->mScroll       = 0.0f;
			menu->mScrollTarget = 0.0f;
		}
		break;
	}
}

static inline void FinishStatusItemScroll(MenuStatus* menu)
{
	menu->mY            = 36.0f;
	menu->mTargetY      = 36.0f;
	char* help          = GetItemName(sStatusItemTable.mItems[menu->mItem].mItem);
	menu->mShowHelp     = 1;
	menu->mHelp         = help;
	menu->mPreviousItem = menu->mItem;
}

static inline void UpdateOwnedItemHelp(MenuStatus* menu)
{
	if (sStatusItemTable.mItems[menu->mItem].mItem < 0) {
		do {
			--menu->mItem;
		} while (sStatusItemTable.mItems[menu->mItem].mItem < 0 && menu->mItem != 0);
	}
	if (menu->mItem != menu->mPreviousItem) {
		char* help = NULL;
		{
			s16 id = sStatusItemTable.mItems[menu->mItem].mItem;
			if (id >= 0 && CountMenuItems() > 0)
				help = GetItemName(id);
		}
		menu->mShowHelp     = 1;
		menu->mHelp         = help;
		menu->mPreviousItem = menu->mItem;
	}
}

void MenuStatus::State2(u32)
{
	MenuStatus* menu = this;
	CPad pad(0);
	s32 count = CountMenuItems();
	switch (menu->mPhase) {
	case 0:
		if (count > 1) {
			if (pad.mPad->mRepeat & 0x40001) {
				if (--menu->mItem < 0)
					menu->mItem = count - 1;
				PlayGameSound(&gGame.mSound, 0x87, 0xFF);
			} else if (pad.mPad->mRepeat & 0x80002) {
				if (sStatusItemTable.mItems[++menu->mItem].mItem < 0)
					menu->mItem = 0;
				PlayGameSound(&gGame.mSound, 0x87, 0xFF);
			}
		}
		if (count > 4) {
			if (menu->mItem >= 4) {
				if (pad.mPad->mRepeat & 0x10008) {
					if ((menu->mItem -= 4) < 0)
						menu->mItem = count - 1;
					PlayGameSound(&gGame.mSound, 0x87, 0xFF);
				}
				if ((pad.mPad->mRepeat & 0x20004) && sStatusItemTable.mPage < (CountMenuItems() - 1) / 4 - 1) {
					menu->mTargetY = -56.0f;
					menu->mPhase   = 10;
					break;
				}
			} else {
				if ((pad.mPad->mRepeat & 0x20004) && menu->mItem + 4 <= count - 1) {
					menu->mItem += 4;
					PlayGameSound(&gGame.mSound, 0x87, 0xFF);
				}
				if (pad.mPad->mRepeat & 0x10008) {
					CountMenuItems();
					if (sStatusItemTable.mPage > 0) {
						menu->mTargetY = 128.0f;
						menu->mPhase   = 20;
						break;
					}
				}
			}
		}
		UpdateOwnedItemHelp(menu);
		if (pad.mPad->mTrigger & 0x200) {
			menu->mPrevMode = menu->mMode;
			menu->mMode     = 4;
			menu->mNextMode = 1;
			menu->mPhase    = 0;
			PlayGameSound(&gGame.mSound, 0x86, 0xFF);
			for (int i = 0; i < 4; ++i) {
				menu->mTabs2[i].mX       = 0.0f;
				menu->mTabs2[i].mTargetX = menu->mHasFourthTab ? 198.0f + i * 108 - 8.0f : 252.0f + i * 108;
			}
			menu->mTargetX = 758.0f;
			for (int i = 0; i < 4; ++i) {
				menu->mTabs[i].mX       = 0.0f;
				menu->mTabs[i].mTargetX = menu->mHasFourthTab ? 198.0f + i * 108 - 8.0f : 252.0f + i * 108;
			}
			for (int i = 0; i < 4; ++i)
				menu->mTabs2[i].mTargetX = 758.0f;

			u32 color = (u32)(255.0f * menu->mAlpha) | 0xFFFFFF00;
			menu->mTitle.Set(GetLocalizedInterfaceText(StatusTitleText), color);
			char* msg = GetLocalizedInterfaceText(StatusTitleText + (menu->mSelection >= 4 ? 0 : gStatusTabTextOffsets[menu->mSelection]));
			menu->mShowHelp       = 1;
			menu->mHelp           = msg;
			menu->mAnimationFrame = 0;
		} else if (pad.mPad->mTrigger & 0x100) {
			if (count > 0) {
				menu->mTargetDetailScale = 1.0f;
				PlayGameSound(&gGame.mSound, 0x88, 0xFF);
				menu->mPhase = 100;
			} else {
				PlayGameSound(&gGame.mSound, 0x86, 0xFF);
			}
		}
		break;
	case 1:
		SelectDiscardQuantity(menu, pad);
		break;
	case 2:
		ConfirmDiscardQuantity(menu, pad, true);
		break;
	case 10:
		if (menu->mY == menu->mTargetY) {
			menu->mPhase = 0;
			if (sStatusItemTable.mPage < (CountMenuItems() - 1) / 4 - 1) {
				++sStatusItemTable.mPage;
				PopulateOwnedItemPage(&sStatusItemTable);
				PlayGameSound(&gGame.mSound, 0x87, 0xff);
			}
			FinishStatusItemScroll(menu);
		}
		break;
	case 20:
		if (menu->mY == menu->mTargetY) {
			menu->mPhase = 0;
			CountMenuItems();
			if (sStatusItemTable.mPage > 0) {
				--sStatusItemTable.mPage;
				PopulateOwnedItemPage(&sStatusItemTable);
				PlayGameSound(&gGame.mSound, 0x87, 0xff);
			}
			FinishStatusItemScroll(menu);
		}
		break;
	case 100:
		if (pad.mPad->mTrigger & 0x300) {
			PlayGameSound(&gGame.mSound, 0x88, 0xFF);
			menu->mPhase             = 0;
			menu->mTargetDetailScale = 0.0f;
		}
		break;
	}
}

static inline void DrawPCChoicePiece(CTexObj& tex, u32 color, f32 u, f32 x, f32 y, f32 width, f32 height)
{
	PSMTXTrans(tex.mMtx[0], u, 0, 0);
	tex.mMtxDirty = TRUE;
	DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, x, y, 0, width, height, 0, 0, 0);
}

static inline CTexObj& InitPCPanelTexture(s32 icon)
{
	return gGame.mTpl.mTex.Init(&((TPLPalette*)gGame.mTpl.mBuffer)->descriptorArray[icon - StatusTextureOffset], 0);
}

static inline f32 PCRecycleChoiceX(f32 x, f32 size, f32 scale, f32 margin, f32 offset)
{
	f32 base = (4.0f * scale + (size * 0.5f + (x + size))) - 40.0f * scale;
	return 40.0f * scale + (margin + (offset + base));
}

static inline f32 PCRecycleTextX(f32 x, f32 size, f32 scale, f32 margin, f32 offset)
{
#if defined(VERSION_JP)
	return x;
#else
	return PCRecycleChoiceX(x, size, scale, margin, offset);
#endif
}

static inline void DrawPCRecycleLabel(s32 id, f32 x, f32 offset, f32 margin, f32 y, f32 size, u32 color)
{
#if defined(VERSION_JP)
	char text[256];
	sprintf(text, "%.255s", GetLocalizedInterfaceText(id));
	x = margin + (offset + (x - (strlen(text) / 2 - 1) * size * 0.5f));
#else
	const char* text = GetLocalizedInterfaceText(id);
	x -= MeasureGameTextWidth(&gGame.mGs, GetLocalizedInterfaceText(id), 1, size) * 0.5f;
#endif
	DrawGsProportionalTextFormat(&gGame.mGs, color, text, x, y, 0, size, size);
}

static inline void DrawPCRecycleUnit(f32 originX, f32 originY, f32 scale, f32 alpha)
{
#if defined(VERSION_JP)
	char units[256];
	sprintf(units, "%.255s", GetLocalizedInterfaceText(0x88));
	DrawGsProportionalTextFormat(&gGame.mGs, (u32)(255.0f * alpha) | 0xC8C8C800, units, 216.0f * scale + originX, 147.0f * scale + originY,
	                             0, 18.0f * scale, 18.0f * scale);
#else
	{
		f32 unitAlpha = alpha;
		CTexObj unit  = InitPCPanelTexture(119);
		PSMTXScale(unit.mMtx[1], 1.0f / 17.0f, 1, 1);
		unit.mMtxDirty = TRUE;
		PSMTXTrans(unit.mMtx[0], 2.0f / 17.0f, 0, 0);
		unit.mMtxDirty = TRUE;
		SetGsTevMode(gGame.mGs.mCurTevStage, 0);
		DrawOverviewTexture(unit, (u32)(255.0f * unitAlpha) | 0xFFFFFF00, 9.0f * scale + (216.0f * scale + originX),
		                    9.0f * scale + (147.0f * scale + originY), 18, 18);
	}
#endif
}

static inline void DrawPCChoiceCaps(CTexObj& tex, f32 cx, f32 y, f32 barWidth, f32 barHeight, f32 scale, f32 alpha)
{
	f32 gap   = 4.0f * scale;
	u32 color = (u32)(200.0f * alpha) | 0x181CC600;
	InitMenuSelectionBar(tex);
	DrawGsTexturedQuad(&gGame.mGs, &tex, color, color, color, color, cx, y, 0, barWidth, barHeight, 0, 0, 0);
	DrawPCChoicePiece(tex, color, 0, (cx - barWidth * 0.5f) - gap, y, 8.0f * scale, barHeight);
	DrawPCChoicePiece(tex, color, 0.5f, gap + (cx + barWidth * 0.5f), y, 8.0f * scale, barHeight);
}

static inline void DrawPCChoiceStripe(f32 x, f32 y, f32 barWidth, f32 barHeight, f32 alpha, f32 scroll)
{
	CTexObj& stripe = InitPCPanelTexture(118);
	PSMTXTrans(stripe.mMtx[0], scroll, 0, 0);
	stripe.mMtxDirty = TRUE;
	PSMTXScale(stripe.mMtx[1], 0.5f * (barWidth / GXGetTexObjWidth(&stripe)), 0.5f, 1);
	stripe.mMtxDirty = TRUE;
	DrawOverviewTexture(stripe, (u32)(40.0f * alpha) | 0xFFFFFF00, x, y, barWidth, barHeight);
}

extern "C" void DrawPCRecyclePanel(MenuPC* menu, s32 choice, f32 x, f32 y, f32 scrap, f32 money, f32 scroll, f32 alpha, f32 scale)
{
	CTexObj tex;
	f32 width = 296.0f * scale, height = 156.0f * scale;
	f32 left = x - width / 2.0f, top = y - height / 2.0f;
	f32 right = width + left, bottom = height + top;
	Vec panel[5] = { (Vec) { left, top, -10.0f }, (Vec) { right, top, -10.0f }, (Vec) { right, bottom, -10.0f },
		             (Vec) { left, bottom, -10.0f }, (Vec) { left, top, -10.0f } };
	u32 opacity  = (u32)(255.0f * alpha);
	DrawGsTrianglePrimitive(&gGame.mGs, GX_TRIANGLEFAN, gGame.mGs.mCamera->mViewMtx, panel, opacity, 5);
	y += scale;
	f32 margin = 20.0f * scale, size = 17.0f * scale;
	f32 labelOffset = 155.0f * scale + margin, rowOffset = 204.0f * scale;
	f32 originX = left - width * 0.25f, originY = (y - height) - height * 0.25f;
	if (choice != 2) {
		f32 inset    = 2.0f * scale;
		f32 cx       = PCRecycleChoiceX(originX, size, scale, margin, labelOffset);
		f32 cy       = (rowOffset + ((size * 0.5f + (size * (choice + 2) + originY)) - inset)) - size;
		f32 barWidth = 0.9f * (4.0f * size), barHeight = size + scale;
		DrawPCChoiceCaps(tex, cx, cy + 2.0f * scale, barWidth, barHeight, scale, alpha);
		DrawPCChoiceStripe(cx + 2.0f * scale, cy + 2.0f * scale, barWidth, barHeight - 4.0f * scale, alpha, scroll);
	}

	DrawPCBlueNumber(false, (130.0f * scale + originX) - 60.0f * scale, (originY + height), alpha, scale, scrap);
	DrawPCRecycleUnit(originX, originY, scale, menu->mAlpha);
	int places = 1;
	for (;; places++) {
		f32 divisor = 1.0f;
		for (int j = 0; j < places; j++)
			divisor *= 10.0f;
		if ((s32)((s32)money / divisor) == 0)
			break;
	}
	tex            = InitPCPanelTexture(125);
	u16 digitWidth = GXGetTexObjWidth(&tex);
#if !defined(VERSION_JP)
	u16 iconWidth = GXGetTexObjWidth(&InitPCPanelTexture(144));
#endif
	f32 numberX = places * (scale * digitWidth) + (120.0f * scale + originX);
#if !defined(VERSION_JP)
	numberX -= scale * (1.1f * iconWidth);
#endif
	DrawPCGoldNumber(false, numberX, (originY + height), alpha, scale, money);
#if defined(VERSION_JP)
	const s32 label1 = 0x86, label2 = 0x87, yes = 0x79, no = 0x7A, done = 0x80;
#else
	const s32 label1 = 0x4D, label2 = 0x4E, yes = 0x40, no = 0x41, done = 0x47;
#endif
	u32 white      = opacity | 0xFFFFFF00;
	f32 textMargin = 40.0f * scale, gap = 4.0f * scale;
	if (choice != 2) {
		f32 textX = PCRecycleTextX(originX, size, scale, margin, labelOffset);
		DrawPCRecycleLabel(label1, textX, labelOffset, textMargin, (rowOffset + (originY - size)) - gap, size, white);
		DrawPCRecycleLabel(label2, textX, labelOffset, textMargin, (originY + rowOffset) - gap, size, white);
		DrawPCRecycleLabel(yes, textX, labelOffset, textMargin, rowOffset + (originY + size), size, white);
		DrawPCRecycleLabel(no, textX, labelOffset, textMargin, rowOffset + (2.0f * size + originY), size, white);
	} else {
		f32 textX = PCRecycleTextX(originX, size, scale, margin, labelOffset);
		DrawPCRecycleLabel(done, textX, labelOffset, textMargin, (originY + rowOffset) - gap, size, white);
	}
}

extern "C" u8* GetUtilibotDescription(s32 id);

static inline void DrawPCShopCenteredText(const char* text, u32 color, f32 x, f32 y, f32 size)
{
	f32 width = MeasureGameTextWidth(&gGame.mGs, text, 1, size);
	DrawGsProportionalTextFormat(&gGame.mGs, color, text, x - width / 2.0f, y, 0, size, size);
}

extern "C" u8* GetAttachmentDescription(s32 id);

#if defined(VERSION_JP)
template <PCShopListKind kind>
static inline void DrawPCPriceUnit(CTexObj& tex, f32 scale, f32 left, f32 digitY, u32 white)
{
	tex = InitPCPanelTexture(kind == ItemPriceList ? 144 : 148);
	SetGsTevMode(gGame.mGs.mCurTevStage, 0);
	u16 unitW = GXGetTexObjWidth(&tex), unitH = GXGetTexObjHeight(&tex);
	DrawOverviewTexture(tex, white, 287.0f * scale + (18.0f * scale + left), digitY, 1.1f * unitW, unitH);
}
#endif

template <PCShopListKind kind>
static inline f32 DrawPCPurchasePrice(CTexObj& tex, const MenuPCItem& row, f32 opacity, f32 left, f32 top, f32 scale, f32 extraW,
                                      f32 extraH)
{
	tex   = InitPCPanelTexture(125);
	f32 w = scale * GXGetTexObjWidth(&tex), h = scale * (GXGetTexObjHeight(&tex) / 10);
	PSMTXScale(tex.mMtx[1], 1, 0.1f, 1);
	tex.mMtxDirty = TRUE;
	f32 digitY    = extraH * scale + (156.0f * scale + top);

#if !defined(VERSION_JP)
	f32 centerOffset = (extraW * 0.5f) * scale;
#endif
	for (int i = 0; i < 4; i++) {
		f32 div = 1.0f;
		if (i > 0)
			for (int j = 0; j < i; j++)
				div *= 10.0f;
		s32 n = (s32)(row.mPrice / div);
		if (i && n == 0)
			continue;
		PSMTXTrans(tex.mMtx[0], 0, 0.1f * (n % 10), 0);
		tex.mMtxDirty = TRUE;
		SetGsTevMode(gGame.mGs.mCurTevStage, 0);
		u32 upper  = (u8)opacity | (kind == ItemPriceList ? 0xEFEBC500 : 0xCECEFF00);
		u32 lower  = (u8)opacity | (kind == ItemPriceList ? 0xEDA83600 : 0x5959FF00);
		f32 digitX = 287.0f * scale + (left - i * w);
#if !defined(VERSION_JP)
		digitX = 18.0f * scale + digitX;
		digitX += centerOffset;
#endif
		DrawGsTexturedQuad(&gGame.mGs, &tex, upper, upper, lower, lower, digitX, digitY, 0, w, h, 0, 0, 0);
		SetGsTevMode(gGame.mGs.mCurTevStage, 4);
	}

	return digitY;
}

static inline void DrawPCPurchaseLabels(s32 question, s32 yes, s32 no, f32 left, f32 top, f32 size, f32 scale, f32 margin, f32 offset,
                                        f32 rowY, f32 extraW, u32 white)
{
	f32 textX = PCRecycleTextX(left, size, scale, margin, offset) + (extraW * 0.5f) * scale;
	DrawPCRecycleLabel(question, textX, offset, 40.0f * scale, (top + rowY) - 4.0f * scale, size, white);
	DrawPCRecycleLabel(yes, textX, offset, 40.0f * scale, rowY + (top + size), size, white);
	DrawPCRecycleLabel(no, textX, offset, 40.0f * scale, rowY + (2.0f * size + top), size, white);
}

static inline void DrawPCPurchaseResult(char (&text)[256], s32 bought, f32 x, f32 y, f32 size, u32 white)
{
#if defined(VERSION_JP)
	sprintf(text, "%.255s", GetLocalizedInterfaceText(bought));
	DrawPCShopCenteredText(text, white, x, y, size);
#else
	f32 labelWidth = MeasureGameTextWidth(&gGame.mGs, GetLocalizedInterfaceText(bought), 1, size);
	DrawGsProportionalTextFormat(&gGame.mGs, white, GetLocalizedInterfaceText(bought), x - labelWidth / 2.0f, y, 0, size, size);
#endif
}

extern "C" void DrawPCScrapPurchasePanel(s32 selected, s32 choice, f32 x, f32 y, f32 scroll, f32 alpha, f32 scale)
{
	static u32 frame = 0;

	++frame;
	CTexObj tex;
	if (scale <= 0)
		return;
	f32 extraH = IsJapanese() ? 0 : 17;
	f32 extraW = IsJapanese() ? 0 : 8;

	f32 width = (246.0f + extraW) * scale, height = (153.0f + extraH) * scale;
	f32 left              = (x - width / 2.0f) - 34.0f * scale;
	f32 top               = ((y - extraH * 0.5f) - height / 2.0f) - 13.0f * scale;
	const MenuPCItem& row = gShopCatalogPage.mRows[selected];
	s16 id                = row.mItem;
	f32 opacity           = 255.0f * alpha;
	u32 color = (u32)opacity, white = color | 0xFFFFFF00;
	DrawTexturedWindowFrame(color, white, x, top + 13.0f * scale, width, height, alpha, 1, 1);
	top += scale;
	f32 margin = 20.0f * scale, size = 17.0f * scale, offset = 130.0f * scale + margin, rowY = 24.0f * scale;

	char text[256];
#if defined(VERSION_JP)
	const s32 bought = 0x8A, question = 0x89, yes = 0x79, no = 0x7A;
#else
	const s32 bought = 0x51, question = 0x50, yes = 0x40, no = 0x41;
#endif
	if (choice == 2) {
		DrawPCPurchaseResult(text, bought, x, 4.0f * scale + (rowY + (5.0f * size + top)), size, white);
	} else {
		f32 cx       = (extraW * 0.5f) * scale + PCRecycleChoiceX(left, size, scale, margin, offset);
		f32 cy       = (rowY + ((size * 0.5f + (size * (choice + 2) + top)) - 2.0f * scale)) - size;
		f32 barWidth = 0.9f * (4.0f * size), barHeight = size + scale;
		DrawPCChoiceCaps(tex, cx, cy + 2.0f * scale, barWidth, barHeight, scale, alpha);
		DrawPCChoiceStripe(cx + 2.0f * scale, cy + 2.0f * scale, barWidth, barHeight - 4.0f * scale, alpha, scroll);
	}
	if (id < 0)
		return;
	sprintf(text, "%.255s", GetUtilibotDescription(id));
	if (choice != 2)
		DrawPCShopCenteredText(text, white, x, (4.0f * scale + (rowY + (5.0f * size + top))) - 3.0f * scale, size);
	sprintf(text, "%.255s", GetUtilibotName(id));
	DrawPCShopCenteredText(text, color | 0xFFAA0000, x, (rowY + (4.0f * size + top)) - 3.0f * scale, 2.0f * scale + size);
	if (choice != 2) {
		f32 digitY = DrawPCPurchasePrice<PriceList>(tex, row, opacity, left, top, scale, extraW, extraH);
#if defined(VERSION_JP)
		DrawPCPriceUnit<PriceList>(tex, scale, left, digitY, white);
#endif

		DrawPCPurchaseLabels(question, yes, no, left, top, size, scale, margin, offset, rowY, extraW, white);
	}
	SetGsTevMode(gGame.mGs.mCurTevStage, 0);
	s32 icon = 238;
	if (id && (gEventFlags.mFlags[0x28 / 4] & 0x8000))
		icon = 239;
	tex   = InitPCPanelTexture(icon);
	u16 w = GXGetTexObjWidth(&tex), h = GXGetTexObjHeight(&tex);
	DrawOverviewTexture(tex, white, 52.0f * scale + left, 48.0f * scale + top, scale * w, scale * h);
}

extern "C" void DrawPCItemPurchasePanel(s32 selected, s32 choice, f32 x, f32 y, f32 scroll, f32 alpha, f32 scale)
{
	static u32 frame = 0;

	++frame;
	CTexObj tex;
	if (scale <= 0)
		return;
	f32 extraH = IsJapanese() ? 0 : 17;
	f32 extraW = IsJapanese() ? 0 : 8;

	f32 width = (246.0f + extraW) * scale, height = (153.0f + extraH) * scale;
	f32 left    = (x - width / 2.0f) - 34.0f * scale;
	f32 top     = ((y - extraH / 2.0f) - height / 2.0f) - 13.0f * scale;
	s16 id      = gShopHistoryPage.mRows[selected].mItem;
	f32 opacity = 255.0f * alpha;
	u32 color = (u32)opacity, white = color | 0xFFFFFF00;
	DrawTexturedWindowFrame(color, white, x, top + 13.0f * scale, width, height, alpha, 1, 1);
	top += scale;
	f32 margin = 20.0f * scale, size = 17.0f * scale, offset = 130.0f * scale + margin, rowY = 24.0f * scale;

	char text[256];
#if defined(VERSION_JP)
	const s32 bought = 0x77, question = 0x78, yes = 0x79, no = 0x7A;
#else
	const s32 bought = 0x3E, question = 0x3F, yes = 0x40, no = 0x41;
#endif
	if (choice == 2) {
		DrawPCPurchaseResult(text, bought, x, 4.0f * scale + (rowY + (5.0f * size + top)), size, white);
	} else {
		f32 cx       = (extraW * 0.5f) * scale + PCRecycleChoiceX(left, size, scale, margin, offset);
		f32 cy       = (rowY + ((size * 0.5f + (size * (choice + 2) + top)) - 2.0f * scale)) - size;
		f32 barWidth = 0.9f * (4.0f * size), barHeight = size + scale;
		DrawPCChoiceCaps(tex, cx, cy + 2.0f * scale, barWidth, barHeight, scale, alpha);
		DrawPCChoiceStripe(cx + 2.0f * scale, cy + 2.0f * scale, barWidth, barHeight - 4.0f * scale, alpha, scroll);
	}
	const MenuPCItem& row = gShopHistoryPage.mRows[selected];
	if (id < 0)
		return;
	if (row.mIsAttachment) {
		sprintf(text, "%.255s", GetAttachmentDescription(id));
		if (choice != 2)
			DrawPCShopCenteredText(text, white, x, (4.0f * scale + (rowY + (5.0f * size + top))) - 3.0f * scale, size);
		sprintf(text, "%.255s", GetAttachmentName(id));
	} else {
		sprintf(text, "%.255s", GetItemDescription(id));
		if (choice != 2)
			DrawPCShopCenteredText(text, white, x, (4.0f * scale + (rowY + (5.0f * size + top))) - 3.0f * scale, size);
		sprintf(text, "%.255s", GetItemName(id));
	}
	DrawPCShopCenteredText(text, color | 0xFFAA0000, x, (rowY + (4.0f * size + top)) - 3.0f * scale, 2.0f * scale + size);
	if (choice != 2) {
		f32 digitY = DrawPCPurchasePrice<ItemPriceList>(tex, row, opacity, left, top, scale, extraW, extraH);
#if defined(VERSION_JP)
		DrawPCPriceUnit<ItemPriceList>(tex, scale, left, digitY, white);
#endif

		DrawPCPurchaseLabels(question, yes, no, left, top, size, scale, margin, offset, rowY, extraW, white);
	}
	s32 icon;
	f32 w, h;
	if (row.mIsAttachment) {
		icon = (id == 2 ? 92 : 93) - StatusTextureOffset;
		tex  = gGame.mTpl.mTex.Init(&((TPLPalette*)gGame.mTpl.mBuffer)->descriptorArray[icon], 0);
		w    = 0.8f * (scale * GXGetTexObjWidth(&tex));
		h    = 0.8f * (scale * GXGetTexObjHeight(&tex));
	} else {
		icon = GetItemIconTextureIndex(id);
		if (icon < 0)
			return;
		tex = gGame.mTpl.mTex.Init(&((TPLPalette*)gGame.mTpl.mBuffer)->descriptorArray[icon], 0);
		w   = scale * GXGetTexObjWidth(&tex);
		h   = scale * GXGetTexObjHeight(&tex);
	}
	SetGsTevMode(gGame.mGs.mCurTevStage, 0);
	DrawOverviewTexture(tex, white, 52.0f * scale + left, 48.0f * scale + top, w, h);
}

extern "C" u8* GetPCRankingName(s32 index);

static inline void DrawPCRankingDigits(CTexObj& tex, const s32& value, f32 x, f32 y, f32 scale, u32 upper, u32 lower, bool rank)
{
	tex   = InitPCPanelTexture(125);
	f32 w = scale * GXGetTexObjWidth(&tex), h = scale * (GXGetTexObjHeight(&tex) / 10);
	PSMTXScale(tex.mMtx[1], 1, 0.1f, 1);
	tex.mMtxDirty = TRUE;
	for (int i = 0; i < (rank ? 7 : 6); i++) {
		f32 div = 1.0f;
		if (i > 0)
			for (int j = 0; j < i; j++)
				div *= 10.0f;
		s32 digit = (s32)(value / div);
		if (i != 0 && digit == 0)
			continue;
		PSMTXTrans(tex.mMtx[0], 0, 0.1f * (digit % 10), 0);
		tex.mMtxDirty = TRUE;
		SetGsTevMode(gGame.mGs.mCurTevStage, 0);
		f32 dx = 7.0f * w + (x - i * (w - 4.0f * scale));
		dx     = rank ? dx - 24.0f * scale : 106.0f * scale + dx;
		DrawGsTexturedQuad(&gGame.mGs, &tex, upper, upper, lower, lower, dx, y + 8.0f * scale, 0, w, h, 0, 0, 0);
		SetGsTevMode(gGame.mGs.mCurTevStage, 4);
	}
}

static inline void ClearRankingRow(PCRankingRow& row)
{
	row.rank      = 0;
	row.nameIndex = -1;
	row.score     = 0;
}

static inline void LoadRankingRow(MenuPC* menu, int slot, int offset)
{
	menu->mRankingRows[slot].rank      = gPCRankingRows[menu->mRankingEnd - offset].rank;
	menu->mRankingRows[slot].nameIndex = gPCRankingRows[menu->mRankingEnd - offset].nameIndex;
	menu->mRankingRows[slot].score     = gPCRankingRows[menu->mRankingEnd - offset].score;
}

static inline void DrawPCRankingMask(f32 left, f32 top, f32 right, f32 bottom, f32 alpha, u32& color, bool first)
{
	Vec vertices[5] = { (Vec) { left, top, -10 }, (Vec) { right, top, -10 }, (Vec) { right, bottom, -10 }, (Vec) { left, bottom, -10 },
		                (Vec) { left, top, -10 } };
	SetGsDepthMode(gGame.mGs.mCurRenderMode, 1, 7, 1);
	if (first)
		color = (u32)(255.0f * alpha) | 0x0A0A0A00;
	DrawGsTrianglePrimitive(&gGame.mGs, GX_TRIANGLEFAN, gGame.mGs.mCamera->mViewMtx, vertices, color, 5);
}

extern "C" void DrawPCRankings(MenuPC* menu, f32 x, f32 y, f32 alpha, f32 scale)
{
	if (alpha > 0) {
		switch (menu->mPhase) {
		case 0:
			for (int i = 0; i < 5; i++) {
				menu->mRankingOffsets[i]        = 0;
				menu->mRankingRows[i].nameIndex = 0;
				menu->mRankingRows[i].score     = 0;
			}
			menu->mRankingEnd = 3;
			if (menu->mRankingEnd <= 3) {
				menu->mPhase          = 2;
				menu->mRankingRows[3] = gPCRankingRows[2];
				menu->mRankingRows[2] = gPCRankingRows[1];
				menu->mRankingRows[1] = gPCRankingRows[0];
			} else {
				ClearRankingRow(menu->mRankingRows[4]);
				LoadRankingRow(menu, 3, 1);
				LoadRankingRow(menu, 2, 2);
				LoadRankingRow(menu, 1, 3);
				LoadRankingRow(menu, 0, 4);
				menu->mPhase++;
			}
			break;
		case 1:
			for (int i = 0; i < 4; i++)
				menu->mRankingOffsets[i] += 0.4f;
			if (menu->mRankingOffsets[0] >= 48.0f) {
				menu->mRankingEnd--;
				if (menu->mRankingEnd <= 3) {
					menu->mPhase++;
					for (int i = 0; i < 4; i++)
						menu->mRankingOffsets[i] = 48.0f;
				} else {
					for (int i = 0; i < 4; i++)
						menu->mRankingOffsets[i] = 0;
					menu->mRankingRows[3] = menu->mRankingRows[2];
					menu->mRankingRows[2] = menu->mRankingRows[1];
					menu->mRankingRows[1] = menu->mRankingRows[0];
					if (menu->mRankingEnd >= 4)
						LoadRankingRow(menu, 0, 4);
					else
						ClearRankingRow(menu->mRankingRows[0]);
				}
			}
			break;
		}
		CTexObj tex;
		f32 left    = 17.0f + (10.0f + ((120.0f + (174.0f + x)) - 70.0f));
		f32 top     = 168.0f + y;
		f32 opacity = 255.0f * alpha;
		u32 light   = (u32)opacity | 0xC8C8C800;
		u8 a        = (u8)opacity;
		for (int i = 0; i < 5; i++) {
			if (gPCRankingRows[i].nameIndex == -1)
				continue;
			f32 rowY = (menu->mRankingOffsets[i] + (top + i * 48)) - 48.0f;
			tex      = InitPCPanelTexture(180);
			SetGsTevMode(gGame.mGs.mCurTevStage, 0);
			f32 w = 1.1f * (1.1f * GXGetTexObjWidth(&tex)), h = 1.1f * GXGetTexObjHeight(&tex);
			DrawOverviewTexture(tex, light, (w / 2.0f + left) - 34.0f, 1.0f + (h / 2.0f + rowY), w, h);
			const PCRankingRow& row = menu->mRankingRows[i];
			s32 rankValue           = row.rank;
#if defined(VERSION_JP)
			const int rankLabel = 0x7C, unrankedLabel = 0x7D;
#else
			const int rankLabel = 0x43, unrankedLabel = 0x44;
#endif
			char text[256];
			if (rankValue > 0 && rankValue <= 1000000) {
				DrawPCRankingDigits(tex, rankValue, left, rowY, scale, a | 0xBFDDFB00, a | 0x0E72D600, true);
				sprintf(text, "%.255s", GetLocalizedInterfaceText(rankLabel));
				DrawGsProportionalTextFormat(&gGame.mGs, a | 0xBFDDFB00, text, 103.0f * scale + left, rowY, 0, 18.0f * scale,
				                             18.0f * scale);
			} else {
				sprintf(text, "%.255s", GetLocalizedInterfaceText(unrankedLabel));
				DrawGsProportionalTextFormat(&gGame.mGs, a | 0xBFDDFB00, text, 48.0f * scale + left, rowY, 0, 18.0f * scale, 18.0f * scale);
			}
			DrawPCRankingDigits(tex, row.score, left, rowY, scale, a | 0xCECEFF00, a | 0xE037EE00, false);
			tex = InitPCPanelTexture(154);
			SetGsTevMode(gGame.mGs.mCurTevStage, 0);
			u16 iw = GXGetTexObjWidth(&tex), ih = GXGetTexObjHeight(&tex);
			DrawOverviewTexture(tex, light, 140.0f * scale + left, rowY + 10.0f * scale, 0.6f * (scale * iw), 0.6f * (scale * ih));
			sprintf(text, "%.255s", GetPCRankingName(row.nameIndex));
			DrawGsProportionalTextFormat(&gGame.mGs, (u32)(255.0f * menu->mAlpha) | 0xC8C8C800, text, 48.0f * scale + left,
			                             rowY + 20.0f * scale, 0, 18.0f * scale, 18.0f * scale);
		}
	}
	u32 mask;
	DrawPCRankingMask(130, 100, 510, 140, alpha, mask, true);
	DrawPCRankingMask(130, 290, 510, 330, alpha, mask, false);
}

static inline void DrawInventoryDigit(CTexObj& tex, s32 digit, u32 color, f32 x, f32 y, f32 width, f32 height)
{
	PSMTXTrans(tex.mMtx[0], 0, 0.1f * digit, 0);
	tex.mMtxDirty = TRUE;
	SetGsTevMode(gGame.mGs.mCurTevStage, 0);
	DrawOverviewTexture(tex, color, x, y, width, height);
}

static inline void PrepareInventoryDigits(CTexObj& tex, f32 scale, f32& width, f32& height)
{
	tex = InitPCPanelTexture(125);
	SetGsTevMode(gGame.mGs.mCurTevStage, 0);
	u16 firstWidth = GXGetTexObjWidth(&tex), secondWidth = GXGetTexObjWidth(&tex);
	width           = scale * (1.2f * secondWidth) + (1.0f - scale) * (scale * firstWidth);
	int firstHeight = GXGetTexObjHeight(&tex) / 10, secondHeight = GXGetTexObjHeight(&tex) / 10;
	height = scale * (1.2f * secondHeight) + 0.0f * (scale * firstHeight);
	PSMTXScale(tex.mMtx[1], 1, 0.1f, 1);
	tex.mMtxDirty = TRUE;
}
static inline void DrawInventoryCount(CTexObj& tex, s16 count, u32 color, f32 x, f32 y, f32 width, f32 height)
{
	if (count >= 10) {
		DrawInventoryDigit(tex, count / 10, color, x - width / 3.0f, y, width, height);
		DrawInventoryDigit(tex, count % 10, color, x + width / 3.0f, y, width, height);
	} else {
		DrawInventoryDigit(tex, count, color, x, y, width, height);
	}
}

static inline void DrawEmptyInventorySlot(CTexObj& tex, u32 color, f32 x, f32 y, f32 scale)
{
	tex = InitPCPanelTexture(234);
	SetGsTevMode(gGame.mGs.mCurTevStage, 0);
	u16 w = GXGetTexObjWidth(&tex), h = GXGetTexObjHeight(&tex);
	DrawOverviewTexture(tex, color, x, y, scale * w, scale * h);
}

static inline void DrawInventoryHighlight(f32 x, f32 y, f32 w, f32 h, f32 alpha, f32 phase)
{
	CGs& gs = gGame.mGs;
	SetGsBlendMode(gs.mCurRenderMode, 1, 2, 4, 15);
	SetGsDepthMode(gs.mCurRenderMode, 1, 2, 0);
	f32 pulse = alpha * (0.2f * (f32)sin(3.1415927f * phase) + 0.2f);
	if (pulse < 0)
		pulse = 0;
	u32 white = (u32)(255.0f * pulse) | 0xFFFFFF00;
	DrawGsQuad(&gs, white, white, white, white, x, y, 1, w, h, 0, 0, 0);
	SetGsBlendMode(gs.mCurRenderMode, 1, 4, 5, 15);
	SetGsDepthMode(gs.mCurRenderMode, 1, 7, 0);
}

static inline void DrawInventoryPreviewIcon(CTexObj& tex, s32 icon, u32 color, f32 x, f32 y, f32 scale, u32 frame)
{
	tex = gGame.mTpl.mTex.Init(&((TPLPalette*)gGame.mTpl.mBuffer)->descriptorArray[icon], 0);
	GXGetTexObjWidth(&tex);
	GXGetTexObjHeight(&tex);
	sin(0.017453292f * (((s32)(frame * 20)) % 360));
	SetGsTevMode(gGame.mGs.mCurTevStage, 0);
	u16 w = GXGetTexObjWidth(&tex), h = GXGetTexObjHeight(&tex);
	DrawOverviewTexture(tex, color, x, y, scale * w, scale * h);
}

static inline void DrawVisibleInventoryIcon(CTexObj& tex, s32 icon, u32 color, f32 xx, f32 yy, f32 bob, f32 scale, f32& iw, f32& ih)
{
	CGs& gs = gGame.mGs;
	tex     = gGame.mTpl.mTex.Init(&((TPLPalette*)gGame.mTpl.mBuffer)->descriptorArray[icon], 0);
	SetGsDepthMode(gs.mCurRenderMode, 1, 7, 1);
	SetGsDepthCompareBeforeTexture(gs.mCurRenderMode, 0);
	SetGsTevMode(gs.mCurTevStage, 0);
	iw = scale * GXGetTexObjWidth(&tex);
	ih = scale * GXGetTexObjHeight(&tex);
	DrawGsTexturedQuad(&gs, &tex, color, color, color, color, xx, yy + bob, 1, iw, ih, 0, 0, 0);
	SetGsTevMode(gs.mCurTevStage, 4);
	SetGsDepthCompareBeforeTexture(gs.mCurRenderMode, 1);
}
extern "C" void DrawInventoryGrid(s32 selected, f32 x, f32 y, f32 alpha, f32 scale, f32 phase)
{
	static u32 frame = 0;
	++frame;
	CGs& gs = gGame.mGs;
	SetGsBlendMode(gs.mCurRenderMode, 1, 4, 5, 15);
	SetGsDepthMode(gs.mCurRenderMode, 1, 7, 0);
	u32 opacity = (u32)(255.0f * alpha), color = opacity | 0xC8C8C800;
	for (int i = 0; i < 8; i++) {
		f32 w = 92.0f * scale, h = 80.0f * scale;
		f32 dx = w * 0.125f + w, dy = h * 0.125f + h;
		f32 xx = (i % 4) * (w * 0.125f + w) + x;
		f32 yy = 12.0f + ((-dy * 0.5f) + ((i / 4) * dy + y));
		CTexObj tex;
		s16 id = sStatusItemTable.mItems[i].mItem, count = sStatusItemTable.mItems[i].mCount;
		scale = 1.0f;
		if (id >= 0 && count > 0) {
			f32 bob = 0;
			if (selected == i)
				bob = 4.0f * (f32)sin(0.017453292f * (((s32)(frame * 20)) % 360));
			s32 icon = GetItemIconTextureIndex(id);
			if (icon >= 0) {
				f32 iw, ih;
				DrawVisibleInventoryIcon(tex, icon, color, xx, yy, bob, scale, iw, ih);
				if (selected == i)
					DrawInventoryHighlight(xx, yy + bob, iw, ih, alpha, phase);
			}
			f32 digitWidth, digitHeight;
			PrepareInventoryDigits(tex, scale, digitWidth, digitHeight);
			DrawInventoryCount(tex, count, color, xx + 22.0f, bob + (yy + 14.0f), digitWidth, digitHeight);
		} else
			DrawEmptyInventorySlot(tex, color, xx, yy, scale);
	}
	f32 w = 92.0f * scale, h = 80.0f * scale;
	f32 dx = w * 0.125f + w, dy = h * 0.125f + h;
	f32 yy = 12.0f + ((-dy * 0.5f) + (2.0f * dy + y));
	for (int i = 0; i < 4; i++) {
		if (sStatusItemTable.mPage >= (CountMenuItems() - 1) / 4 - 1)
			break;
		f32 xx = (i % 4) * (w * 0.125f + w) + x;
		CTexObj tex;
		s16 id = sStatusItemTable.mNext[i].mItem, count = sStatusItemTable.mNext[i].mCount;
		if (id >= 0 && count > 0) {
			s32 icon = GetItemIconTextureIndex(id);
			if (icon >= 0)
				DrawInventoryPreviewIcon(tex, icon, color, xx, yy, scale, frame);
			f32 digitWidth, digitHeight;
			PrepareInventoryDigits(tex, scale, digitWidth, digitHeight);
			DrawInventoryCount(tex, count, color, xx + 22.0f * scale, yy + 14.0f * scale, digitWidth, digitHeight);
		} else
			DrawEmptyInventorySlot(tex, color, xx, yy, scale);
	}
	dx = w * 0.125f + w;
	dy = h * 0.125f + h;
	dx = w * 0.125f + w;
	dy = h * 0.125f + h;
	yy = 12.0f + ((-dy * 0.5f) + (-dy + y));
	for (int i = 0; i < 4; i++) {
		CountMenuItems();
		if (sStatusItemTable.mPage <= 0)
			break;
		f32 xx = (i % 4) * (w * 0.125f + w) + x;
		CTexObj tex;
		s16 id = sStatusItemTable.mPrev[i].mItem, count = sStatusItemTable.mPrev[i].mCount;
		if (id >= 0 && count > 0) {
			s32 icon = GetItemIconTextureIndex(id);
			if (icon >= 0)
				DrawInventoryPreviewIcon(tex, icon, opacity | 0xFFFFFF00, xx, yy, scale, frame);
			f32 digitWidth, digitHeight;
			PrepareInventoryDigits(tex, scale, digitWidth, digitHeight);
			DrawInventoryCount(tex, count, color, xx + 22.0f * scale, yy + 14.0f * scale, digitWidth, digitHeight);
		} else
			DrawEmptyInventorySlot(tex, color, xx, yy, scale);
	}
}

/**
 * @note Address: 0x8003135C
 * @note Size: 0x48
 */
extern "C" void Close__5CMenuFv(CMenu* menu)
{
	menu->mState = 5;
	menu->Resume();
	menu->mWindow.mOpen = FALSE;
}

/**
 * @note Address: 0x800313A4
 * @note Size: 0x24
 */
extern "C" BOOL IsOpen__5CMenuFv(CMenu* menu)
{
	return menu->CMenu::IsOpen();
}

/**
 * @note Address: 0x800313C8
 * @note Size: 0x10
 */
extern "C" void Suspend__5CMenuFi(CMenu* menu, s32 dir)
{
	menu->mSuspended   = TRUE;
	menu->mSuspendFade = dir;
}
