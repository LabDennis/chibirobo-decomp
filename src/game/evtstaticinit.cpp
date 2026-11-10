#include "game/hsd_jobj.h"
#include "game/cmotioncontrolcamera.h"
#include "game/evt.h"
#include "Version.h"
#include "PowerPC_EABI_Support/Runtime/MWCPlusLib.h"
#include "PowerPC_EABI_Support/Runtime/global_destructor_chain.h"

typedef s32 (CEvt::*EventHandler)(u8, s32);

struct EventCommandData {
	u8 prefix[0x26c];
	EventHandler handlers[49];
};

extern "C" {
extern void* __vt__8CEvtVars[];
extern void* __vt__9CEvtQueue[];
extern void* __vt__11CEvtQueueEx[];
extern void* __vt__9CEvtTimer[];
extern EventCommandData lbl_80224748;
#if defined(VERSION_JP)
extern u8 gGlobalScriptResources[];
#else
extern u8 lbl_803676D0[];
#endif
XObj* __ct__4XObjFv(XObj*);
XObj* __dt__4XObjFv(XObj*, s32);
CCameraMemento* __ct__14CCameraMementoFv(CCameraMemento*);
CCameraMemento* __dt__14CCameraMementoFv(CCameraMemento*, s32);
void DestroyCCameraMementoArray();
CEvtFlags* __dt__9CEvtFlagsFv(CEvtFlags*, s32);
CEvtVars* __dt__8CEvtVarsFv(CEvtVars*, s32);
CEvtQueueEx* __dt__11CEvtQueueExFv(CEvtQueueEx*, s32);
CEvtTimer* __dt__9CEvtTimerFv(CEvtTimer*, s32);
}

inline void* operator new(size_t, void* storage)
{
	return storage;
}

static inline void InstallEventHandlers(EvtCmdEntry* __restrict commands, const EventHandler* handlers)
{
	commands[0].handler = handlers[0];
	commands[1].handler = handlers[1];
	commands[2].handler = handlers[2];
	commands[3].handler = handlers[3];
	commands[4].handler = handlers[4];
	commands[5].handler = handlers[5];
}
// These globals and their destructor-chain nodes were pooled in the original event unit.
extern "C" void __sinit_evt_cpp()
{
#if defined(VERSION_JP)
	u8* storage      = gGlobalScriptResources + 0x100;
	const int prefix = -0xc;
#else
	u8* storage      = lbl_803676D0;
	const int prefix = 0;
#endif
	u8* flags = storage + prefix + 0x14c;
	new (flags) CEvtFlags;
	__register_global_object(flags, (void*)__dt__9CEvtFlagsFv, storage + prefix + 0x140);

	u8* vars                  = storage + prefix + 0x35c;
	*(void***)(vars + 0x2000) = __vt__8CEvtVars;

	__register_global_object(vars, (void*)__dt__8CEvtVarsFv, storage + prefix + 0x350);

	u8* queue                 = storage + prefix + 0x236c;
	*(void***)(queue + 0x204) = __vt__9CEvtQueue;
	*(void***)(queue + 0x204) = __vt__11CEvtQueueEx;

	__register_global_object(queue, (void*)__dt__11CEvtQueueExFv, storage + prefix + 0x2360);

	u8* timer                = storage + prefix + 0x2980;
	*(void***)(timer + 0x50) = __vt__9CEvtTimer;

	__register_global_object(timer, (void*)__dt__9CEvtTimerFv, storage + prefix + 0x2974);

	gEventBeatLatch.mPending = 0;
	evtCmdTable[0].handler   = lbl_80224748.handlers[0];
	InstallEventHandlers(evtCmdTable + 1, lbl_80224748.handlers + 1);
	InstallEventHandlers(evtCmdTable + 7, lbl_80224748.handlers + 7);
	InstallEventHandlers(evtCmdTable + 13, lbl_80224748.handlers + 13);
	InstallEventHandlers(evtCmdTable + 19, lbl_80224748.handlers + 19);
	InstallEventHandlers(evtCmdTable + 25, lbl_80224748.handlers + 25);
	InstallEventHandlers(evtCmdTable + 31, lbl_80224748.handlers + 31);
	InstallEventHandlers(evtCmdTable + 37, lbl_80224748.handlers + 37);
	InstallEventHandlers(evtCmdTable + 43, lbl_80224748.handlers + 43);

#if defined(VERSION_JP)
	u8* object  = storage + 0x29d8;
	u8* cameras = storage + 0x3354;
#else
	u8* object  = storage + 0x29e0;
	u8* cameras = storage + 0x336c;
#endif
	__register_global_object(__ct__4XObjFv((XObj*)object), (void*)__dt__4XObjFv, storage + prefix + 0x29d4);
	__construct_array(cameras, (ConstructorDestructor)__ct__14CCameraMementoFv, (ConstructorDestructor)__dt__14CCameraMementoFv,
	                  sizeof(CCameraMemento), 4);
	__register_global_object(NULL, (void*)DestroyCCameraMementoArray, cameras - 0xc);
}
