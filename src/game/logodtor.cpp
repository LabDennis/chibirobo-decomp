#include "game/hsd_jobj.h"
#include "game/logo.h"
#include "game/infoscrn.h"
#include "game/cparticle.h"
#include "game/cscrn.h"
#include "PowerPC_EABI_Support/Runtime/MWCPlusLib.h"

void operator delete(void* object);

extern "C" {
extern void* __vt__4Logo[];
extern void* __vt__9CScrnFade[];
extern void* __vt__10CScrnColor[];
extern void* __vt__8InfoScrn[];
extern void* __vt__9CParticle[];
extern void* __vt__10CScrnQuake[];
void* __dt__10CGeneratorFv(void* gen, s32 flags);
void* __dt__5CFileFv(void* file, s32 flags);
void* __dt__4CVecFv(void* vec, s32 flags);
}

// todo: weak copy
/**
 * @note Address: 0x80018D28
 * @note Size: 0x90
 */
extern "C" Logo* __dt__4LogoFv(Logo* logo, s32 flags)
{
	if (logo != NULL) {
		*(void***)((u8*)logo + 0x200) = __vt__4Logo; // _200 vtable
		logo->mTpl.CTpl::~CTpl();
		if ((s16)flags > 0) {
			::operator delete(logo);
		}
	}
	return logo;
}

// todo: weak copy
/**
 * @note Address: 0x80018DB8
 * @note Size: 0x5C
 */
extern "C" CScrnFade* __dt__9CScrnFadeFv(CScrnFade* fade, s32 flags)
{
	if (fade != NULL) {
		*(void***)((u8*)fade + 0x30) = __vt__9CScrnFade; // _30 vtable
		fade->CScrnColor::~CScrnColor();
		if ((s16)flags > 0) {
			::operator delete(fade);
		}
	}
	return fade;
}

// todo: weak copy
/**
 * @note Address: 0x80018E14
 * @note Size: 0x48
 */
extern "C" CScrnColor* __dt__10CScrnColorFv(CScrnColor* color, s32 flags)
{
	if (color != NULL) {
		*(void***)((u8*)color + 0x30) = __vt__10CScrnColor; // _30 vtable
		if ((s16)flags > 0) {
			::operator delete(color);
		}
	}
	return color;
}

// todo: weak copy
/**
 * @note Address: 0x80018E5C
 * @note Size: 0x98
 */
extern "C" InfoScrn* __dt__8InfoScrnFv(InfoScrn* info, s32 flags)
{
	if (info != NULL) {
		*(void***)((u8*)info + 0x1D8) = __vt__8InfoScrn; // _1D8 vtable
		info->mWipe.CScrnWipe::~CScrnWipe();
		info->mFade.CScrnFade::~CScrnFade();
		if ((s16)flags > 0) {
			::operator delete(info);
		}
	}
	return info;
}

// todo: weak copy
/**
 * @note Address: 0x80018EF4
 * @note Size: 0x88
 */
extern "C" CParticle* __dt__9CParticleFv(CParticle* particle, s32 flags)
{
	if (particle != NULL) {
		*(void***)((u8*)particle + 0x544) = __vt__9CParticle; // _544 vtable
		__destroy_arr(particle->mGenerators, (ConstructorDestructor*)__dt__10CGeneratorFv, sizeof(CGenerator), CPARTICLE_GEN_NUM);
		__destroy_arr(particle->mFile, (ConstructorDestructor*)__dt__5CFileFv, sizeof(CFile), CPARTICLE_FILE_NUM);
		if ((s16)flags > 0) {
			::operator delete(particle);
		}
	}
	return particle;
}

// todo: weak copy
/**
 * @note Address: 0x80018F7C
 * @note Size: 0x6C
 */
extern "C" CScrnQuake* __dt__10CScrnQuakeFv(CScrnQuake* quake, s32 flags)
{
	if (quake != NULL) {
		*(void***)((u8*)quake + 0x34) = __vt__10CScrnQuake; // _34 vtable
		__destroy_arr(quake->mShake, (ConstructorDestructor*)__dt__4CVecFv, sizeof(CVec), 3);
		if ((s16)flags > 0) {
			::operator delete(quake);
		}
	}
	return quake;
}
