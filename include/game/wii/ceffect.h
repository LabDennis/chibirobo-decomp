#ifndef _GAME_WII_CEFFECT_H
#define _GAME_WII_CEFFECT_H

#include "types.h"
#include "Dolphin/mtx.h"
#include "nw4r/ef/ef_handle.h"

// one running effect (ceffect.cpp makes and steps it), the elements of CEvtPtcl
struct EffectInstance {
	nw4r::ef::HandleBase handle; // _00
	u32 _08;                     // _08
	Vec pos;                     // _0C
	Vec rot;                     // _18
	Vec scale;                   // _24
	u8 _30[0x60 - 0x30];         // _30
	MtxP mtx;                    // _60
	u16 _64;                     // _64
	u8 _66[0x68 - 0x66];         // _66
	u32 id;                      // _68, the emitter index the event started
	u32 group;                   // _6C, the effect group, 3 for the stage's own emitters
	u32 _70;                     // _70

	// closed or closing, the slot is free
	bool IsFinished() const
	{
		nw4r::ef::ReferencedObject::LifeStatus status = handle.IsValid()
		                                                  ? static_cast<nw4r::ef::ReferencedObject*>(handle.GetPtr())->GetLifeStatus()
		                                                  : nw4r::ef::ReferencedObject::NW4R_EF_LS_CLOSED;
		return status == nw4r::ef::ReferencedObject::NW4R_EF_LS_CLOSED || status == nw4r::ef::ReferencedObject::NW4R_EF_LS_CLOSING;
	}
};
STATIC_ASSERT(sizeof(EffectInstance) == 0x74);

BOOL CreateEffectInstance(EffectInstance* inst, u32 emitterIdx, u32 groupID);
void RetireEffect(nw4r::ef::HandleBase* handle, BOOL retireParticles);
void UpdateEffectMtx(EffectInstance* inst);

#endif
