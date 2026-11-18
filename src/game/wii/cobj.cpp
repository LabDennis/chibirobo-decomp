// the Wii model object, cobj.cpp by the file name its nw4r asserts carry: NW_Obj wraps the
// nw4r scene model and its animation objects, CObj is the GameCube CJObj and CXObj in one
// class on top of it. The library classes come from the declarations in xobj.h.
#include "game/wii/xobj.h"
#include "Dolphin/gx.h"
#include "Dolphin/mtx.h"
#include "game/gmath.h"
#include <math.h>

// the two allocators mem.cpp sets up: MEM1's for the scene models, MEM2's for the anims
extern MEMAllocator gWiiMem1Allocator;
extern MEMAllocator gWiiPadAllocator;

// nw4r's db checks (db_assert.h) as the game's sources use them; the lines are the game's own
#define NW4R_ASSERT_LINE(line, exp) ((void)((exp) || (nw4r::db::Panic(__FILE__, line, "NW4R:Failed assertion " #exp), 0)))
#define NW4R_ASSERT_VALID_PTR_LINE(line, exp)                                                                                              \
	((void)((((u32)(exp) & 0xFF000000) == 0x80000000 || ((u32)(exp) & 0xFF800000) == 0x81000000 || ((u32)(exp) & 0xF8000000) == 0x90000000 \
	         || ((u32)(exp) & 0xFF000000) == 0xC0000000 || ((u32)(exp) & 0xFF800000) == 0xC1000000                                         \
	         || ((u32)(exp) & 0xF8000000) == 0xD0000000 || ((u32)(exp) & 0xFFFFC000) == 0xE0000000)                                        \
	        || (nw4r::db::Panic(__FILE__, line, "NW4R:Pointer Error\n" #exp "(=%p) is not valid pointer.", (exp)), 0)))

using namespace nw4r::g3d;

/**
 * @note Address: 0x802B6560
 * @note Size: 0x130
 */
void NW_Obj::CreateScnMdl(int numView, BOOL blend, BOOL noTevColor, const char* name)
{
	// the material buffers the model needs for the anims its file has
	u32 option = 0x1C8;
	if (m_resFile.HasResAnmVis()) {
		option |= 0x40;
	}
	if (m_resFile.HasResAnmClr()) {
		option |= 0x108;
	}
	if (m_resFile.HasResAnmTexPat()) {
		option |= 0x3;
	}
	if (m_resFile.HasResAnmTexSrt()) {
		option |= 0x204;
	}
	if (m_resFile.HasResAnmShp()) {
		option |= 0x7000;
	}
	if (noTevColor) {
		option &= ~0x100;
	}
	m_pScnMdl = ScnMdl::Construct(&gWiiMem1Allocator, NULL, m_resMdl, option, numView);
	if (m_pScnMdl == NULL) {
		nw4r::db::Panic(__FILE__, 0x61, "%s, m_pScnMdl(=%p)\n", name, m_pScnMdl);
	}
	m_blend.Init(&gWiiPadAllocator, NULL, m_resMdl);
	m_flags = 0;
	if (blend) {
		m_flags |= 1;
	}
}

/**
 * @note Address: 0x802B6690
 * @note Size: 0x1C0
 */
// NONMATCHING: the child loop's index and the reloaded blend pointer swap registers (retail r28/r27,
// here r26/r28). the blend pointer's second test is retail's: it reads like an inlined helper of
// m_blend's, which brings the registers one step closer but is not written here without more evidence
void NW_Obj::Release(BOOL freeResource)
{
	if (m_resources.chr != NULL) {
		m_resources.chr->Destroy();
	}
	if (m_resources.vis != NULL) {
		m_resources.vis->Destroy();
	}
	if (m_resources.matclr != NULL) {
		m_resources.matclr->Destroy();
	}
	if (m_resources.texpat != NULL) {
		m_resources.texpat->Destroy();
	}
	if (m_resources.texsrt != NULL) {
		m_resources.texsrt->Destroy();
	}
	if (m_resources.shp != NULL) {
		m_resources.shp->Destroy();
	}
	if (m_resources.scn != NULL) {
		m_resources.scn->Destroy();
	}
	if (m_blend.m_pBlend != NULL) {
		if (m_blend.m_pBlend != NULL) {
			m_blend.m_pBlend->Release();
		}
		for (int i = 0; i < 4; i++) {
			if (m_blend.m_pBlend->GetChild(i) != NULL) {
				m_blend.m_pBlend->Detach(i);
			}
		}
		m_blend.m_pBlend->Destroy();
		m_blend.m_pBlend = NULL;
	}
	if (m_pScnMdl != NULL) {
		m_pScnMdl->Destroy();
		m_pScnMdl = NULL;
	}
	if (freeResource) {
		if (m_resMdl.IsValid()) {
			m_resMdl.Release();
			m_resMdl.Terminate();
		}
		if (m_resFile.IsValid()) {
			m_resFile.Release();
			m_resFile.Terminate();
		}
	}
}

/**
 * @note Address: 0x802B6850
 * @note Size: 0x88
 */
AnmObjChrRes* NW_Obj::GetAnmObjChrRes() const
{
	if (m_flags & 1) {
		return m_blend.m_pBlend->GetChild(0);
	}
	const ScnMdl* mdl = m_pScnMdl;
	return (AnmObjChrRes*)mdl->GetAnmObj(ScnMdlSimple::ANMOBJTYPE_CHR);
}

/**
 * @note Address: 0x802B68D8
 * @note Size: 0xB4
 */
// each kind takes the name the one before it found, so a model whose anims share one index picks
// all of them by the skeletal anim's name
void NW_Obj::SetAnim(const char* name, s32 id, f32 frame, f32 blend, f32 blendFrames)
{
	if (m_pScnMdl != NULL) {
		name = SetAnmChr(name, id, frame, blend, blendFrames);
		name = SetAnmVis(name, id, frame);
		name = SetAnmClr(name, id, frame);
		name = SetAnmTexPat(name, id, frame);
		name = SetAnmTexSrt(name, id, frame);
		SetAnmShp(name, id, frame);
	}
}

/**
 * @note Address: 0x802B698C
 * @note Size: 0x174
 */
const char* NW_Obj::SetAnmChr(const char* name, s32 id, f32 frame, f32 blend, f32 blendFrames)
{
	if (m_resFile.HasResAnmChr()) {
		if (name != NULL || (id >= 0 && id < m_resFile.GetResAnmChrNumEntries())) {
			ResAnmChr chr = name != NULL ? m_resFile.GetResAnmChr(name) : m_resFile.GetResAnmChr(id);
			if (SetResAnmChr(chr, frame, blend, blendFrames) && name == NULL) {
				name = chr.GetName();
			}
		}
	} else if (m_pScnMdl->GetAnmObj(ScnMdlSimple::ANMOBJTYPE_CHR) != NULL) {
		RemoveAnmChr();
	}
	return name;
}

/**
 * @note Address: 0x802B6B00
 * @note Size: 0x84
 */
void NW_Obj::RemoveAnmChr()
{
	if (m_pScnMdl->GetAnmObj(ScnMdlSimple::ANMOBJTYPE_CHR) != NULL) {
		m_pScnMdl->RemoveAnmObj(ScnMdlSimple::ANMOBJTYPE_CHR);
	}
	// a blended anim belongs to m_blend
	if (!(m_flags & 1) && m_resources.chr != NULL) {
		m_resources.chr->Destroy();
		m_resources.chr = NULL;
	}
}

/**
 * @note Address: 0x802B6B84
 * @note Size: 0x154
 */
const char* NW_Obj::SetAnmVis(const char* name, s32 id, f32 frame)
{
	if (m_resFile.HasResAnmVis()) {
		if (name != NULL || (id >= 0 && id < m_resFile.GetResAnmVisNumEntries())) {
			ResAnmVis vis = name != NULL ? m_resFile.GetResAnmVis(name) : m_resFile.GetResAnmVis(id);
			if (SetResAnmVis(vis, frame) && name == NULL) {
				name = vis.GetName();
			}
		}
	} else if (m_pScnMdl->GetAnmObj(ScnMdlSimple::ANMOBJTYPE_VIS) != NULL) {
		RemoveAnmVis();
	}
	return name;
}

/**
 * @note Address: 0x802B6CD8
 * @note Size: 0x78
 */
void NW_Obj::RemoveAnmVis()
{
	if (m_pScnMdl->GetAnmObj(ScnMdlSimple::ANMOBJTYPE_VIS) != NULL) {
		m_pScnMdl->RemoveAnmObj(ScnMdlSimple::ANMOBJTYPE_VIS);
	}
	if (m_resources.vis != NULL) {
		m_resources.vis->Destroy();
		m_resources.vis = NULL;
	}
}

/**
 * @note Address: 0x802B6D50
 * @note Size: 0x154
 */
const char* NW_Obj::SetAnmClr(const char* name, s32 id, f32 frame)
{
	if (m_resFile.HasResAnmClr()) {
		if (name != NULL || (id >= 0 && id < m_resFile.GetResAnmClrNumEntries())) {
			ResAnmClr clr = name != NULL ? m_resFile.GetResAnmClr(name) : m_resFile.GetResAnmClr(id);
			if (SetResAnmClr(clr, frame) && name == NULL) {
				name = clr.GetName();
			}
		}
	} else if (m_pScnMdl->GetAnmObj(ScnMdlSimple::ANMOBJTYPE_MATCLR) != NULL) {
		RemoveAnmClr();
	}
	return name;
}

/**
 * @note Address: 0x802B6EA4
 * @note Size: 0x78
 */
void NW_Obj::RemoveAnmClr()
{
	if (m_pScnMdl->GetAnmObj(ScnMdlSimple::ANMOBJTYPE_MATCLR) != NULL) {
		m_pScnMdl->RemoveAnmObj(ScnMdlSimple::ANMOBJTYPE_MATCLR);
	}
	if (m_resources.matclr != NULL) {
		m_resources.matclr->Destroy();
		m_resources.matclr = NULL;
	}
}

/**
 * @note Address: 0x802B6F1C
 * @note Size: 0x154
 */
const char* NW_Obj::SetAnmTexPat(const char* name, s32 id, f32 frame)
{
	if (m_resFile.HasResAnmTexPat()) {
		if (name != NULL || (id >= 0 && id < m_resFile.GetResAnmTexPatNumEntries())) {
			ResAnmTexPat pat = name != NULL ? m_resFile.GetResAnmTexPat(name) : m_resFile.GetResAnmTexPat(id);
			if (SetResAnmTexPat(pat, frame) && name == NULL) {
				name = pat.GetName();
			}
		}
	} else if (m_pScnMdl->GetAnmObj(ScnMdlSimple::ANMOBJTYPE_TEXPAT) != NULL) {
		RemoveAnmTexPat();
	}
	return name;
}

/**
 * @note Address: 0x802B7070
 * @note Size: 0x78
 */
void NW_Obj::RemoveAnmTexPat()
{
	if (m_pScnMdl->GetAnmObj(ScnMdlSimple::ANMOBJTYPE_TEXPAT) != NULL) {
		m_pScnMdl->RemoveAnmObj(ScnMdlSimple::ANMOBJTYPE_TEXPAT);
	}
	if (m_resources.texpat != NULL) {
		m_resources.texpat->Destroy();
		m_resources.texpat = NULL;
	}
}

/**
 * @note Address: 0x802B70E8
 * @note Size: 0x154
 */
const char* NW_Obj::SetAnmTexSrt(const char* name, s32 id, f32 frame)
{
	if (m_resFile.HasResAnmTexSrt()) {
		if (name != NULL || (id >= 0 && id < m_resFile.GetResAnmTexSrtNumEntries())) {
			ResAnmTexSrt srt = name != NULL ? m_resFile.GetResAnmTexSrt(name) : m_resFile.GetResAnmTexSrt(id);
			if (SetResAnmTexSrt(srt, frame) && name == NULL) {
				name = srt.GetName();
			}
		}
	} else if (m_pScnMdl->GetAnmObj(ScnMdlSimple::ANMOBJTYPE_TEXSRT) != NULL) {
		RemoveAnmTexSrt();
	}
	return name;
}

/**
 * @note Address: 0x802B723C
 * @note Size: 0x78
 */
void NW_Obj::RemoveAnmTexSrt()
{
	if (m_pScnMdl->GetAnmObj(ScnMdlSimple::ANMOBJTYPE_TEXSRT) != NULL) {
		m_pScnMdl->RemoveAnmObj(ScnMdlSimple::ANMOBJTYPE_TEXSRT);
	}
	if (m_resources.texsrt != NULL) {
		m_resources.texsrt->Destroy();
		m_resources.texsrt = NULL;
	}
}

/**
 * @note Address: 0x802B72B4
 * @note Size: 0x154
 */
const char* NW_Obj::SetAnmShp(const char* name, s32 id, f32 frame)
{
	if (m_resFile.HasResAnmShp()) {
		if (name != NULL || (id >= 0 && id < m_resFile.GetResAnmShpNumEntries())) {
			ResAnmShp shp = name != NULL ? m_resFile.GetResAnmShp(name) : m_resFile.GetResAnmShp(id);
			if (SetResAnmShp(shp, frame) && name == NULL) {
				name = shp.GetName();
			}
		}
	} else if (m_pScnMdl->GetAnmObj(ScnMdlSimple::ANMOBJTYPE_SHP) != NULL) {
		RemoveAnmShp();
	}
	return name;
}

/**
 * @note Address: 0x802B7408
 * @note Size: 0x78
 */
void NW_Obj::RemoveAnmShp()
{
	if (m_pScnMdl->GetAnmObj(ScnMdlSimple::ANMOBJTYPE_SHP) != NULL) {
		m_pScnMdl->RemoveAnmObj(ScnMdlSimple::ANMOBJTYPE_SHP);
	}
	if (m_resources.shp != NULL) {
		m_resources.shp->Destroy();
		m_resources.shp = NULL;
	}
}

/**
 * @note Address: 0x802B7480
 * @note Size: 0x6C
 */
BOOL NW_Obj::SetResAnmChr(const ResAnmChr& chr, f32 frame, f32 blend, f32 blendFrames)
{
	if (chr.IsValid()) {
		CreateAnmChr(chr, frame, blend, blendFrames);
		return TRUE;
	}
	if (m_pScnMdl->GetAnmObj(ScnMdlSimple::ANMOBJTYPE_CHR) != NULL) {
		RemoveAnmChr();
	}
	return FALSE;
}

/**
 * @note Address: 0x802B74EC
 * @note Size: 0x2B8
 */
// NONMATCHING: the ResMdl copies the two Bind calls take swap stack slots, retail has the blended
// one at 0x8 and the plain one at 0xC; declaring the results up front or turning the branches round
// moves more than it fixes
void NW_Obj::CreateAnmChr(const ResAnmChr& chr, f32 frame, f32 blend, f32 blendFrames)
{
	RemoveAnmChr();
	m_resources.chr = AnmObjChrRes::Construct(&gWiiPadAllocator, NULL, chr, m_resMdl, false);
	NW4R_ASSERT_VALID_PTR_LINE(0x17A, m_resources.chr);
	if (m_flags & 1) {
		m_blend.Attach(m_resources.chr, blend, blendFrames);
		bool bBindResult = m_blend.m_pBlend->Bind(m_resMdl, 0, AnmObjChr::BIND_PARTIAL);
		NW4R_ASSERT_LINE(0x187, bBindResult);
		bool bSetResult = m_pScnMdl->SetAnmObj(m_blend.m_pBlend, ScnMdlSimple::ANMOBJTYPE_CHR);
		NW4R_ASSERT_LINE(0x18B, bSetResult);
	} else {
		bool bBindResult = m_resources.chr->Bind(m_resMdl, 0, AnmObjChr::BIND_PARTIAL);
		NW4R_ASSERT_LINE(0x193, bBindResult);
		bool bSetResult = m_pScnMdl->SetAnmObj(m_resources.chr, ScnMdlSimple::ANMOBJTYPE_CHR);
		NW4R_ASSERT_LINE(0x197, bSetResult);
	}
	m_resources.chr->SetFrame(frame);
}

/**
 * @note Address: 0x802B77A4
 * @note Size: 0x6C
 */
BOOL NW_Obj::SetResAnmVis(const ResAnmVis& vis, f32 frame)
{
	if (vis.IsValid()) {
		CreateAnmVis(vis, frame);
		return TRUE;
	}
	if (m_pScnMdl->GetAnmObj(ScnMdlSimple::ANMOBJTYPE_VIS) != NULL) {
		RemoveAnmVis();
	}
	return FALSE;
}

/**
 * @note Address: 0x802B7810
 * @note Size: 0x1EC
 */
void NW_Obj::CreateAnmVis(const ResAnmVis& vis, f32 frame)
{
	RemoveAnmVis();
	m_resources.vis = AnmObjVisRes::Construct(&gWiiPadAllocator, NULL, vis, m_resMdl);
	NW4R_ASSERT_VALID_PTR_LINE(0x1B6, m_resources.vis);
	bool bBindResult = m_resources.vis->Bind(m_resMdl);
	NW4R_ASSERT_LINE(0x1BA, bBindResult);
	bool bSetResult = m_pScnMdl->SetAnmObj(m_resources.vis, ScnMdlSimple::ANMOBJTYPE_VIS);
	NW4R_ASSERT_LINE(0x1BE, bSetResult);
	m_resources.vis->SetFrame(frame);
}

/**
 * @note Address: 0x802B79FC
 * @note Size: 0x6C
 */
BOOL NW_Obj::SetResAnmClr(const ResAnmClr& clr, f32 frame)
{
	if (clr.IsValid()) {
		CreateAnmClr(clr, frame);
		return TRUE;
	}
	if (m_pScnMdl->GetAnmObj(ScnMdlSimple::ANMOBJTYPE_MATCLR) != NULL) {
		RemoveAnmClr();
	}
	return FALSE;
}

/**
 * @note Address: 0x802B7A68
 * @note Size: 0x1F0
 */
void NW_Obj::CreateAnmClr(const ResAnmClr& clr, f32 frame)
{
	RemoveAnmClr();
	m_resources.matclr = AnmObjMatClrRes::Construct(&gWiiPadAllocator, NULL, clr, m_resMdl, false);
	NW4R_ASSERT_VALID_PTR_LINE(0x1DD, m_resources.matclr);
	bool bBindResult = m_resources.matclr->Bind(m_resMdl);
	NW4R_ASSERT_LINE(0x1E1, bBindResult);
	bool bSetResult = m_pScnMdl->SetAnmObj(m_resources.matclr, ScnMdlSimple::ANMOBJTYPE_MATCLR);
	NW4R_ASSERT_LINE(0x1E5, bSetResult);
	m_resources.matclr->SetFrame(frame);
}

/**
 * @note Address: 0x802B7C58
 * @note Size: 0x12C
 */
// without a pattern anim of its own the model goes back to its default one
BOOL NW_Obj::SetResAnmTexPat(const ResAnmTexPat& pat, f32 frame)
{
	if (pat.IsValid()) {
		CreateAnmTexPat(pat, frame);
		if (mTexPatName == mDefaultTexPat) {
			mTexPatName = pat.GetName();
		}
		return TRUE;
	}
	if (mDefaultTexPat != NULL) {
		if (mTexPatName != mDefaultTexPat) {
			ResAnmTexPat def = m_resFile.GetResAnmTexPat(mDefaultTexPat);
			if (def.IsValid()) {
				CreateAnmTexPat(def, frame);
			}
			mTexPatName = mDefaultTexPat;
		}
	} else if (m_pScnMdl->GetAnmObj(ScnMdlSimple::ANMOBJTYPE_TEXPAT) != NULL) {
		RemoveAnmTexPat();
	}
	return FALSE;
}

/**
 * @note Address: 0x802B7D84
 * @note Size: 0x1F0
 */
void NW_Obj::CreateAnmTexPat(const ResAnmTexPat& pat, f32 frame)
{
	RemoveAnmTexPat();
	m_resources.texpat = AnmObjTexPatRes::Construct(&gWiiPadAllocator, NULL, pat, m_resMdl, false);
	NW4R_ASSERT_VALID_PTR_LINE(0x20E, m_resources.texpat);
	bool bBindResult = m_resources.texpat->Bind(m_resMdl);
	NW4R_ASSERT_LINE(0x212, bBindResult);
	bool bSetResult = m_pScnMdl->SetAnmObj(m_resources.texpat, ScnMdlSimple::ANMOBJTYPE_TEXPAT);
	NW4R_ASSERT_LINE(0x216, bSetResult);
	m_resources.texpat->SetFrame(frame);
}

/**
 * @note Address: 0x802B7F74
 * @note Size: 0x6C
 */
BOOL NW_Obj::SetResAnmTexSrt(const ResAnmTexSrt& srt, f32 frame)
{
	if (srt.IsValid()) {
		CreateAnmTexSrt(srt, frame);
		return TRUE;
	}
	if (m_pScnMdl->GetAnmObj(ScnMdlSimple::ANMOBJTYPE_TEXSRT) != NULL) {
		RemoveAnmTexSrt();
	}
	return FALSE;
}

/**
 * @note Address: 0x802B7FE0
 * @note Size: 0x1F0
 */
void NW_Obj::CreateAnmTexSrt(const ResAnmTexSrt& srt, f32 frame)
{
	RemoveAnmTexSrt();
	m_resources.texsrt = AnmObjTexSrtRes::Construct(&gWiiPadAllocator, NULL, srt, m_resMdl, false);
	NW4R_ASSERT_VALID_PTR_LINE(0x235, m_resources.texsrt);
	bool bBindResult = m_resources.texsrt->Bind(m_resMdl);
	NW4R_ASSERT_LINE(0x239, bBindResult);
	bool bSetResult = m_pScnMdl->SetAnmObj(m_resources.texsrt, ScnMdlSimple::ANMOBJTYPE_TEXSRT);
	NW4R_ASSERT_LINE(0x23D, bSetResult);
	m_resources.texsrt->SetFrame(frame);
}

/**
 * @note Address: 0x802B81D0
 * @note Size: 0x6C
 */
BOOL NW_Obj::SetResAnmShp(const ResAnmShp& shp, f32 frame)
{
	if (shp.IsValid()) {
		CreateAnmShp(shp, frame);
		return TRUE;
	}
	if (m_pScnMdl->GetAnmObj(ScnMdlSimple::ANMOBJTYPE_SHP) != NULL) {
		RemoveAnmShp();
	}
	return FALSE;
}

/**
 * @note Address: 0x802B823C
 * @note Size: 0x1F0
 */
void NW_Obj::CreateAnmShp(const ResAnmShp& shp, f32 frame)
{
	RemoveAnmShp();
	m_resources.shp = AnmObjShpRes::Construct(&gWiiPadAllocator, NULL, shp, m_resMdl, false);
	NW4R_ASSERT_VALID_PTR_LINE(0x25C, m_resources.shp);
	bool bBindResult = m_resources.shp->Bind(m_resMdl);
	NW4R_ASSERT_LINE(0x260, bBindResult);
	bool bSetResult = m_pScnMdl->SetAnmObj(m_resources.shp, ScnMdlSimple::ANMOBJTYPE_SHP);
	NW4R_ASSERT_LINE(0x264, bSetResult);
	m_resources.shp->SetFrame(frame);
}

/**
 * @note Address: 0x802B842C
 * @note Size: 0x64
 */
void NW_Obj::RemoveAnimAll()
{
	RemoveAnmChrAll();
	RemoveAnmChr();
	RemoveAnmVis();
	RemoveAnmClr();
	RemoveAnmTexPat();
	RemoveAnmTexSrt();
	RemoveAnmShp();
	mTexPatName = NULL;
}

/**
 * @note Address: 0x802B8490
 * @note Size: 0xEC
 */
void NW_Obj::SetUpdateRate(f32 rate)
{
	static ScnMdlSimple::AnmObjType types[] = {
		ScnMdlSimple::ANMOBJTYPE_CHR,    ScnMdlSimple::ANMOBJTYPE_VIS,    ScnMdlSimple::ANMOBJTYPE_MATCLR,
		ScnMdlSimple::ANMOBJTYPE_TEXPAT, ScnMdlSimple::ANMOBJTYPE_TEXSRT, ScnMdlSimple::ANMOBJTYPE_SHP,
	};

	AnmObj* chr = m_pScnMdl->GetAnmObj(ScnMdlSimple::ANMOBJTYPE_CHR);
	if (chr != NULL) {
		if (m_flags & 1) {
			m_blend.SetUpdateRate(chr, rate);
		} else {
			chr->SetUpdateRate(rate);
		}
	}
	for (int i = 1; i < 6; i++) {
		AnmObj* obj = m_pScnMdl->GetAnmObj(types[i]);
		if (obj != NULL) {
			obj->SetUpdateRate(rate);
		}
	}
}

/**
 * @note Address: 0x802B857C
 * @note Size: 0x18
 */
void NW_Obj::UpdateBlend(f32 frames)
{
	if (m_flags & 1) {
		m_blend.Update(frames);
	}
}

/**
 * @note Address: 0x802B8594
 * @note Size: 0x6C
 */
void NW_Obj::RemoveAnmChrAll()
{
	m_blend.DetachAll();
	m_pScnMdl->RemoveAnmObj(ScnMdlSimple::ANMOBJTYPE_CHR);
	if (!(m_flags & 1) && m_resources.chr != NULL) {
		m_resources.chr->Destroy();
	}
	m_resources.chr = NULL;
}

/**
 * @note Address: 0x802B8600
 * @note Size: 0x74
 */
void NW_Obj::SetAnmScn(u32 id, f32 frame)
{
	if (id < m_resFile.GetResAnmScnNumEntries()) {
		ResAnmScn scn = m_resFile.GetResAnmScn(id);
		if (scn.IsValid()) {
			CreateAnmScn(scn, frame);
		}
	}
}

/**
 * @note Address: 0x802B8674
 * @note Size: 0x44
 */
void NW_Obj::ReleaseWork()
{
	if (m_resources.scn != NULL) {
		m_resources.scn->Destroy();
		m_resources.scn = NULL;
	}
}

/**
 * @note Address: 0x802B86B8
 * @note Size: 0x1B4
 */
void NW_Obj::CreateAnmScn(ResAnmScn& scn, f32 frame)
{
	if (m_resources.scn != NULL) {
		m_resources.scn->Destroy();
	}
	scn.Release();
	bool bBindResult = scn.Bind();
	NW4R_ASSERT_LINE(0x2E9, bBindResult);
	m_resources.scn = AnmScnRes::Construct(&gWiiPadAllocator, NULL, scn, false);
	NW4R_ASSERT_VALID_PTR_LINE(0x2F2, m_resources.scn);
	m_resources.scn->SetFrame(frame);
}

/**
 * @note Address: 0x802B886C
 * @note Size: 0x420
 */
// adds a last TEV stage to every material, out = prev * C1 + K1, through which SetColors tints the
// model; the stage's spare byte 0xC3 marks a material that already has it
// NONMATCHING: register allocation only; retail keeps the loop and stage values in r16-r19 and the
// hoisted string addresses above them, this gives the variables the high registers
void NW_Obj::InitColorTev()
{
	if (m_pScnMdl == NULL) {
		return;
	}
	u32 num = m_resMdl.GetResMatNumEntries();
	for (u32 i = 0; i < num; i++) {
		ResMat mat = m_resMdl.GetResMat(i);
		ScnMdl::CopiedMatAccess access(m_pScnMdl, i);
		ResGenMode genMode;
		genMode = access.GetResGenMode(false);
		if (!genMode.IsValid()) {
			genMode = mat.GetResGenMode();
		}
		NW4R_ASSERT_LINE(0x313, genMode.IsValid());
		ResMatTevColor tevColor;
		tevColor = access.GetResMatTevColor(false);
		if (!tevColor.IsValid()) {
			tevColor = mat.GetResMatTevColor();
		}
		NW4R_ASSERT_LINE(0x319, tevColor.IsValid());
		ResTev tev;
		tev = access.GetResTev(false);
		if (!tev.IsValid()) {
			tev = mat.GetResTev();
		}
		NW4R_ASSERT_LINE(0x31F, tev.IsValid());
		// the model's materials share their TEV, so one tag means all of them are done
		if (i == 0 && tev.ptr()->PADDING_0x18[7] == 0xC3) {
			return;
		}
		u8 stage    = genMode.GXGetNumTevStages();
		u8 tevStage = tev.GetNumTevStages();
		if (stage > 15) {
			// retail evaluates the material's name for a report that is compiled out
			mat.GetName();
			continue;
		}
		genMode.GXSetNumTevStages(stage + 1);
		GXColor add = { 0, 0, 0, 0 };
		tevColor.GXSetTevKColor(GX_KCOLOR1, add);
		GXColor mul = { 0xFF, 0xFF, 0xFF, 0 };
		tevColor.GXSetTevColor(GX_TEVREG1, mul);
		tevColor.EndEdit();
		if (tev.ptr()->PADDING_0x18[7] == 0) {
			tev.SetNumTevStages(tevStage + 1);
			tev.GXSetTevKColorSel((GXTevStageID)stage, GX_TEV_KCSEL_K1);
			tev.GXSetTevOrder((GXTevStageID)stage, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
			tev.GXSetTevColorIn((GXTevStageID)stage, GX_CC_ZERO, GX_CC_C1, GX_CC_CPREV, GX_CC_KONST);
			tev.GXSetTevColorOp((GXTevStageID)stage, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
			tev.GXSetTevAlphaIn((GXTevStageID)stage, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_APREV);
			tev.GXSetTevAlphaOp((GXTevStageID)stage, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
			tev.ptr()->PADDING_0x18[7] = 0xC3;
			tev.EndEdit();
		}
	}
}

/**
 * @note Address: 0x802B8C8C
 * @note Size: 0x1F0
 */
void NW_Obj::SetColors(GXColor add, GXColor mul)
{
	if (!m_resMdl.IsValid()) {
		return;
	}
	u32 num = m_resMdl.GetResMatNumEntries();
	for (int i = 0; i < num; i++) {
		ResMat mat = m_resMdl.GetResMat(i);
		ScnMdl::CopiedMatAccess access(m_pScnMdl, i);
		ResTev tev = access.GetResTev(false);
		if (!tev.IsValid()) {
			tev = mat.GetResTev();
		}
		NW4R_ASSERT_LINE(0x360, tev.IsValid());
		if (tev.ptr()->PADDING_0x18[7] == 0xC3) {
			ResMatTevColor tevColor;
			tevColor = access.GetResMatTevColor(false);
			if (!tevColor.IsValid()) {
				tevColor = mat.GetResMatTevColor();
			}
			NW4R_ASSERT_LINE(0x368, tevColor.IsValid());
			tevColor.GXSetTevKColor(GX_KCOLOR1, add);
			tevColor.GXSetTevColor(GX_TEVREG1, mul);
			tevColor.EndEdit();
		}
	}
}

/**
 * @note Address: 0x802B8E7C
 * @note Size: 0x158
 */
void NW_ChrBlend::Init(MEMAllocator* allocator, u32* size, ResMdl mdl)
{
	m_pBlend = AnmObjChrBlend::Construct(allocator, size, mdl, 4);
	NW4R_ASSERT_VALID_PTR_LINE(0x38B, m_pBlend);
	for (int i = 0; i < 4; i++) {
		mNames[i]       = NULL;
		mWeightSteps[i] = 0.0f;
	}
}

/**
 * @note Address: 0x802B8FD4
 * @note Size: 0x264
 */
// the children move one place down; the old ones keep fading out while the new one fades in
// over blendFrames (the next one out over half of it)
void NW_ChrBlend::Attach(AnmObjChrRes* chr, f32 blend, f32 blendFrames)
{
	if (m_pBlend != NULL) {
		m_pBlend->Release();
	}
	for (int i = 3; i > 0; i--) {
		AnmObjChrRes* prev = m_pBlend->GetChild(i - 1);
		if (prev != NULL) {
			m_pBlend->Detach(i - 1);
			AnmObjChrRes* old = m_pBlend->Attach(i, prev);
			if (old != NULL) {
				old->Destroy();
			}
			m_pBlend->SetWeight(i, blend < 1.0f ? m_pBlend->GetWeight(i - 1) : 0.0f);
			mNames[i]       = mNames[i - 1];
			mWeightSteps[i] = mWeightSteps[i - 1];
		}
	}
	mWeightSteps[1] = m_pBlend->GetWeight(1) / (blendFrames / 2);
	mNames[0]       = chr->GetResAnm().GetName();
	mWeightSteps[0] = 1.0f / blendFrames;
	m_pBlend->Attach(0, chr);
	m_pBlend->SetWeight(0, blend);
}

/**
 * @note Address: 0x802B9238
 * @note Size: 0x12C
 */
// the anims fading out hold their frame
void NW_ChrBlend::SetUpdateRate(AnmObj* obj, f32 rate)
{
	obj->SetUpdateRate(rate);
	for (int i = 1; i < 4; i++) {
		AnmObjChrRes* child = m_pBlend->GetChild(i);
		if (child != NULL && m_pBlend->GetWeight(i) > 0.0f && child->GetUpdateRate() > 0.0f) {
			child->SetUpdateRate(0.0f);
		}
	}
}

/**
 * @note Address: 0x802B9364
 * @note Size: 0x194
 */
void NW_ChrBlend::Update(f32 frames)
{
	for (int i = 0; i < 4; i++) {
		AnmObjChrRes* child = m_pBlend->GetChild(i);
		if (child != NULL) {
			f32 weight = m_pBlend->GetWeight(i);
			if (i == 0) {
				if (weight < 1.0f) {
					weight += frames * mWeightSteps[i];
					if (weight > 1.0f) {
						weight = 1.0f;
					}
				}
			} else if (weight > 0.0f) {
				weight -= frames * mWeightSteps[i];
				if (weight < 0.0f) {
					weight = 0.0f;
				}
			}
			if (weight > 0.0f) {
				m_pBlend->SetWeight(i, weight);
			} else {
				m_pBlend->Detach(i);
				child->Destroy();
			}
		}
	}
}

/**
 * @note Address: 0x802B94F8
 * @note Size: 0xC8
 */
void NW_ChrBlend::DetachAll()
{
	for (int i = 0; i < 4; i++) {
		AnmObjChrRes* child = m_pBlend->GetChild(i);
		if (child != NULL) {
			m_pBlend->Detach(i);
			child->Destroy();
		}
	}
}

/**
 * @note Address: 0x802B95C0
 * @note Size: 0x180
 */
void CObj::Init()
{
	mRequest.Reset();
	mLoadState = 0;
	mMoveTranslate.Set(0.0f, 0.0f, 0.0f);
	mMoveTarget0.Set(0.0f, 0.0f, 0.0f);
	mMoveSpeed0.Set(0.0f, 0.0f, 0.0f);
	mMoveRotate.Set(0.0f, 0.0f, 0.0f);
	mMoveTarget1.Set(0.0f, 0.0f, 0.0f);
	mMoveSpeed1.Set(0.0f, 0.0f, 0.0f);
	mMoveScale.Set(1.0f, 1.0f, 1.0f);
	mMoveTarget2.Set(1.0f, 1.0f, 1.0f);
	mMoveSpeed2.Set(0.0f, 0.0f, 0.0f);
	mAnimId        = -1;
	mAnimRate[1]   = 0.0f;
	mAnimRate[0]   = 0.0f;
	mAnimRate[3]   = 0.0f;
	mAnimRate[2]   = 0.0f;
	GXColor add    = { 0, 0, 0, 0 };
	mColor0        = add;
	GXColor mul    = { 0xFF, 0xFF, 0xFF, 0 };
	mColor1        = mul;
	mColorsChanged = TRUE;
}

/**
 * @note Address: 0x802B9740
 * @note Size: 0x160
 */
// NONMATCHING: retail keeps the four components of the first color in r3..r6 in rising order, this
// loads them falling (r6..r3); the second color and everything else agree. Building the first color
// from a u32 0 gets the registers and swaps the two colors' stack slots instead.
BOOL CObj::Load(ResourceFile* file, const char* path, u32 model, u32 flags, u32 offset, u32 size)
{
	switch (mLoadState) {
	case 0:
		ReadAsync(file, path, model, flags, offset, size);
		// no break: the Wii checks the read it has just asked for
	case 1:
		ReadCheck();
		break;
	}
	if (mLoadState == 0) {
		GXColor add    = { 0, 0, 0, 0 };
		mColor0        = add;
		GXColor mul    = { 0xFF, 0xFF, 0xFF, 0 };
		mColor1        = mul;
		mColorsChanged = TRUE;
		return TRUE;
	}
	return FALSE;
}

/**
 * @note Address: 0x802B98A0
 * @note Size: 0x7C
 */
BOOL CObj::ReadAsync(ResourceFile* file, const char* path, u32 model, u32 flags, u32 offset, u32 size)
{
	if (mRequest.Request(file, path, flags, offset, size)) {
		mLoadState++;
		mModel     = model;
		mLoadFlags = flags;
		return TRUE;
	}
	return FALSE;
}

/**
 * @note Address: 0x802B991C
 * @note Size: 0x1C8
 */
// NONMATCHING: the three flag arguments of CreateScnMdl are extracted in another order (retail does
// the 0x40000 bit first); casts, masks or bool parameters do not move it
BOOL CObj::ReadCheck()
{
	if (mLoadState == 1 && mRequest.Update()) {
		if (mRequest.mResult >= 0) {
			// the first owner of the file binds it
			BOOL bind = (mRequest.mFile != NULL ? mRequest.mFile->mRefCount : 0) == 1;
			// CResFile's two calls are NW_Obj's own: they set up m_resFile and m_resMdl and clear the anims
			((CResFile*)&mObject)->Init(mRequest.mFile != NULL ? mRequest.mFile->mBuffer : NULL, bind, bind);
			((CResFile*)&mObject)->BindTop(0, bind);
			mObject.CreateScnMdl(mLoadFlags & 0xFF, (mLoadFlags >> 16) & 1, (mLoadFlags >> 18) & 1,
			                     mRequest.mFile != NULL ? mRequest.mFile->mPath : NULL);
			if (!(mLoadFlags & 0x20000)) {
				mObject.InitColorTev();
				mObject.SetColors(mColor0, mColor1);
			}
			mObject.m_pScnMdl->SetScnMdlCallback(this);
			mObject.m_pScnMdl->EnableScnMdlCallbackTiming(nw4r::g3d::ScnObj::CALLBACK_TIMING_A);
			mObject.m_pScnMdl->EnableScnMdlCallbackTiming(nw4r::g3d::ScnObj::CALLBACK_TIMING_B);
			mObject.m_pScnMdl->EnableScnMdlCallbackTiming(nw4r::g3d::ScnObj::CALLBACK_TIMING_C);
			mObject.m_pScnMdl->SetScnMdlCallbackNodeID(0);
			mObject.m_pScnMdl->SetScnObjCallback(this);
			mObject.m_pScnMdl->EnableScnObjCallbackExecOp(nw4r::g3d::ScnObj::EXECOP_CALC_MAT);
			mObject.m_pScnMdl->EnableScnObjCallbackTiming(nw4r::g3d::ScnObj::CALLBACK_TIMING_C);
			mAnimId = -1;
		}
		mLoadState = 0;
	}
	return mLoadState == 0;
}

/**
 * @note Address: 0x802B9AE4
 * @note Size: 0x80
 */
void CObj::Release()
{
	if (mObject.m_pScnMdl != NULL) {
		ClearAnim();
		mObject.Release((mRequest.mFile != NULL ? mRequest.mFile->mRefCount : 0) == 1);
	}
	mRequest.Release();
	mLoadState = 0;
}

/**
 * @note Address: 0x802B9B64
 * @note Size: 0x88
 */
// with the blend flag the skeletal anims cross-fade as soon as there is more than one to choose from
void CObj::SetAnmSource(NW_Obj* source)
{
	mAnmSource = source;
	if ((mLoadFlags & 0x10000) && !(mObject.m_flags & 1) && mObject.m_resFile.HasResAnmChr()
	    && mObject.m_resFile.GetResAnmChrNumEntries() + mAnmSource->m_resFile.GetResAnmChrNumEntries() > 1) {
		mObject.m_flags |= 1;
	}
}

/**
 * @note Address: 0x802B9BEC
 * @note Size: 0x64
 */
void CObj::ClearAnmSource()
{
	mAnmSource = NULL;
	if ((mLoadFlags & 0x10000) && (mObject.m_flags & 1) && mObject.m_resFile.GetResAnmChrNumEntries() <= 1) {
		mObject.m_flags &= ~1;
	}
}

/**
 * @note Address: 0x802B9C50
 * @note Size: 0x124
 */
// NONMATCHING: the two temporaries swap stack slots, retail has the scaled direction at 0x8 and
// the Vec operator- takes at 0x18. Leaving the product unnamed gets the slots and loses the order
// its components are loaded in.
void CObj::SetMoveDir(const Vec* dir, f32 speed)
{
	mMoveTarget0.Set(dir->x, dir->y, dir->z);
	CVec d = mMoveTranslate - *dir;
	if (PSVECMag(&d) > 0.0f) {
		// the float fabs is nw4r::math::FAbs's, as in fbeffect.cpp
		d.x = __fabsf(d.x);
		d.y = __fabsf(d.y);
		d.z = __fabsf(d.z);
		// each axis gets its share of the distance, so all three arrive on the same frame
		PSVECNormalize(&d, &d);
		CVec step = d * speed;
		mMoveSpeed0.Set(step.x, step.y, step.z);
		if (mMoveSpeed0.x > 0.0f && mMoveSpeed0.x < 0.001f) {
			mMoveSpeed0.x = 0.001f;
		}
		if (mMoveSpeed0.z > 0.0f && mMoveSpeed0.z < 0.001f) {
			mMoveSpeed0.z = 0.001f;
		}
	}
}

/**
 * @note Address: 0x802B9D74
 * @note Size: 0xDC
 */
void CObj::SetRotateY(f32 target, f32 speed)
{
	mMoveTarget1.y = target;
	mMoveSpeed1.y  = speed;
	if (speed == 0.0f) {
		mMoveRotate.y = target;
		return;
	}
	f32 cur        = WrapAngle(mMoveRotate.y);
	f32 tgt        = WrapAngle(mMoveTarget1.y);
	mMoveRotate.y  = cur;
	mMoveTarget1.y = tgt;
	if (tgt == cur) {
		return;
	}
	f32 diff = tgt - cur;
	diff     = __fabsf(diff);
	if (diff >= TWO_PI) {
		diff -= TWO_PI;
	}
	// go the short way round
	if (diff > PI) {
		if (tgt < cur) {
			mMoveTarget1.y = TWO_PI + tgt;
		} else {
			mMoveTarget1.y = tgt - TWO_PI;
		}
	}
}

/**
 * @note Address: 0x802B9E50
 * @note Size: 0x9C
 */
void CObj::SetAnim(s32 id, BOOL force, f32 frame, f32 rate, f32 blend, f32 blendRate)
{
	if (force || id != mAnimId) {
		mAnimId         = id;
		mAnimFromSource = FALSE;
		mObject.SetAnim(NULL, id, frame, blend, blendRate);
		// a new anim always takes the rate too
		force = TRUE;
	}
	if (rate != mAnimRate[0] || force) {
		mAnimRate[0] = rate;
		mAnimRate[1] = 0.01f + rate;
	}
}

/**
 * @note Address: 0x802B9EEC
 * @note Size: 0x224
 */
// the anims are looked up by name in mAnmSource first, then by index in the object's own file
void CObj::SetAnim(const CObjAnim* anim, BOOL force, f32 frame, f32 rate, f32 blend, f32 blendRate)
{
	if (force || anim->mId != mAnimId) {
		mAnimId         = anim->mId;
		mAnimFromSource = FALSE;
		if (mAnmSource != NULL) {
			nw4r::g3d::ResAnmChr chr    = mAnmSource->m_resFile.GetResAnmChr(anim->mName);
			nw4r::g3d::ResAnmVis vis    = mAnmSource->m_resFile.GetResAnmVis(anim->mName);
			nw4r::g3d::ResAnmClr clr    = mAnmSource->m_resFile.GetResAnmClr(anim->mName);
			nw4r::g3d::ResAnmTexPat pat = mAnmSource->m_resFile.GetResAnmTexPat(anim->mName);
			nw4r::g3d::ResAnmTexSrt srt = mAnmSource->m_resFile.GetResAnmTexSrt(anim->mName);
			nw4r::g3d::ResAnmShp shp    = mAnmSource->m_resFile.GetResAnmShp(anim->mName);
			if (chr.IsValid() || vis.IsValid() || clr.IsValid() || pat.IsValid() || srt.IsValid() || shp.IsValid()) {
				mObject.SetResAnmChr(chr, frame, blend, blendRate);
				mObject.SetResAnmVis(vis, frame);
				mObject.SetResAnmClr(clr, frame);
				mObject.SetResAnmTexPat(pat, frame);
				mObject.SetResAnmTexSrt(srt, frame);
				mObject.SetResAnmShp(shp, frame);
				mAnimFromSource = TRUE;
			}
		}
		if (!mAnimFromSource) {
			mObject.SetAnim(anim->mName, anim->mId, frame, blend, blendRate);
		}
		force = TRUE;
	}
	if (rate != mAnimRate[0] || force) {
		SetAnimRate(rate);
	}
}

/**
 * @note Address: 0x802BA110
 * @note Size: 0x10
 */
void CObj::ClearAnim()
{
	mAnimId = -1;
	mObject.RemoveAnimAll();
}

/**
 * @note Address: 0x802BA120
 * @note Size: 0x14
 */
void CObj::SetAnimRate(f32 rate)
{
	mAnimRate[0] = rate;
	// off by a bit so the next update always sets it
	mAnimRate[1] = 0.01f + rate;
}

/**
 * @note Address: 0x802BA134
 * @note Size: 0x164
 */
// the material anims alone, from mAnmSource when it has any of them
void CObj::SetMatAnim(const CObjAnim* anim, f32 frame)
{
	BOOL found = FALSE;
	if (mAnmSource != NULL) {
		nw4r::g3d::ResAnmClr clr    = mAnmSource->m_resFile.GetResAnmClr(anim->mName);
		nw4r::g3d::ResAnmTexPat pat = mAnmSource->m_resFile.GetResAnmTexPat(anim->mName);
		nw4r::g3d::ResAnmTexSrt srt = mAnmSource->m_resFile.GetResAnmTexSrt(anim->mName);
		if (clr.IsValid() || pat.IsValid() || srt.IsValid()) {
			mObject.SetResAnmClr(clr, frame);
			mObject.SetResAnmTexPat(pat, frame);
			mObject.SetResAnmTexSrt(srt, frame);
			found = TRUE;
		}
	}
	if (!found) {
		nw4r::g3d::ResAnmClr clr    = mObject.m_resFile.GetResAnmClr(anim->mName);
		nw4r::g3d::ResAnmTexPat pat = mObject.m_resFile.GetResAnmTexPat(anim->mName);
		nw4r::g3d::ResAnmTexSrt srt = mObject.m_resFile.GetResAnmTexSrt(anim->mName);
		if (clr.IsValid() || pat.IsValid() || srt.IsValid()) {
			mObject.SetResAnmClr(clr, frame);
			mObject.SetResAnmTexPat(pat, frame);
			mObject.SetResAnmTexSrt(srt, frame);
		}
	}
}

/**
 * @note Address: 0x802BA298
 * @note Size: 0xB4
 */
void CObj::UpdateAnim(f32 speed)
{
	if (mAnimRate[1] != mAnimRate[0] * speed) {
		mAnimRate[1] = mAnimRate[0] * speed;
		mObject.SetUpdateRate(mAnimRate[1]);
	}
	if ((mObject.m_resFile.HasResAnmScn() ? mObject.m_resources.scn : NULL) != NULL) {
		mAnimRate[3] = mAnimRate[2] * speed;
		if (mObject.m_resources.scn != NULL) {
			mObject.m_resources.scn->SetUpdateRate(mAnimRate[3]);
		}
	}
	mObject.UpdateBlend(speed);
}

/**
 * @note Address: 0x802BA34C
 * @note Size: 0x48
 */
void CObj::SetAnmScn(u32 id, f32 frame, f32 rate)
{
	mObject.SetAnmScn(id, frame);
	SetAnmScnRate(rate);
}

/**
 * @note Address: 0x802BA394
 * @note Size: 0x14
 */
void CObj::SetAnmScnRate(f32 rate)
{
	mAnimRate[2] = rate;
	mAnimRate[3] = 0.01f + rate;
}

/**
 * @note Address: 0x802BA3A8
 * @note Size: 0x94
 */
// the root node takes the object's own rotation, translation and scale
void CObj::ExecCallbackA(nw4r::g3d::ChrAnmResult* pResult, nw4r::g3d::ResMdl mdl, nw4r::g3d::FuncObjCalcWorld* pFuncObj)
{
	if (pFuncObj->GetCallbackNodeID() == 0) {
		nw4r::math::MTX34 rot;
		nw4r::math::MTX34 rotTrans;
		nw4r::math::MTX34RotXYZRad(&rot, mMoveRotate.x, mMoveRotate.y, mMoveRotate.z);
		PSMTXTransApply(rot.m, rotTrans.m, mMoveTranslate.x, mMoveTranslate.y, mMoveTranslate.z);
		pResult->SetRotTrans(&rotTrans);
		pResult->SetScale((const nw4r::math::VEC3*)&mMoveScale);
	}
}

/**
 * @note Address: 0x802BA43C
 * @note Size: 0xA4
 */
void CObj::ExecCallback_CALC_MAT(int timing, nw4r::g3d::ScnObj* obj, u32 param, void* info)
{
	switch (timing) {
	case 4:
		if (!(mLoadFlags & 0x20000) && mColorsChanged) {
			mObject.SetColors(mColor0, mColor1);
			mColorsChanged = FALSE;
		}
		break;
	}
}

/**
 * @note Address: 0x802BA4E0
 * @note Size: 0x4
 */
void CObj::ExecCallbackB(nw4r::g3d::ChrAnmResult* pResult, nw4r::g3d::ResMdl mdl, nw4r::g3d::FuncObjCalcWorld* pFuncObj)
{
}
