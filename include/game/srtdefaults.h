#ifndef _GAME_SRTDEFAULTS_H
#define _GAME_SRTDEFAULTS_H

// include after the HSD or dolphin headers, which define Vec

// Two sets of scale/translate/rotate defaults from a game header: 1,1,1 / 0,0,0 / 0,0,0, twice. MWCC emits
// unreferenced static consts too, so these 0x48 bytes open the .rodata of every file that includes the original
// header: retail has them at the start of evt, msg, obja_chr, objb_attach, xobjactormama, xobjs,
// ccameraactionmap2 and ccameraactionscroll2, and in the unsplit .rodata ahead of gamemain, gxa, objb_act, scrc,
// the spider, Jenny, Papa, Tao, Deca and toy constants. Code takes particle rotations from them: the toy's
// bursts copy the first rotate (lbl_8020B6C8+0x18), the player's tumble the second (lbl_8020A588+0x3C).
// The original header is not known, so only the files that show the references include this one.

static const Vec sDefaultScale     = { 1.0f, 1.0f, 1.0f };
static const Vec sDefaultTranslate = { 0.0f, 0.0f, 0.0f };
static const Vec sDefaultRotate    = { 0.0f, 0.0f, 0.0f };

static const Vec sDefaultScale2     = { 1.0f, 1.0f, 1.0f };
static const Vec sDefaultTranslate2 = { 0.0f, 0.0f, 0.0f };
static const Vec sDefaultRotate2    = { 0.0f, 0.0f, 0.0f };

#endif
