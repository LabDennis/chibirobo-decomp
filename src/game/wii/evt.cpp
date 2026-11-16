// The event script runner, .text 0x8009453C-0x800A18B8: its .rodata names it "evt.cpp" (wii_units.md Nr. 38),
// the GameCube's evt.cpp. The functions come in the reverse of the GameCube's order.
// NONMATCHING: only the small commands have source so far.
#include "game/evt.h"
#include "game/globals.h"

CEvtFlags gEventFlags;
CEvtVars gEventVars;
CEvtQueueEx gEventScriptQueue;
CEvtTimer gEventTimers;

extern "C" s32 CEvt_Evaluate(CEvt* evt);
extern "C" void CEvt_EvalSet(CEvt* evt);

s32 CEvt::_cmd_SETSP(u8, s32)
{
	s32 value                      = CEvt_Evaluate(this);
	mValues.mBuf[mValues.mCount++] = value;
	return 0;
}

s32 CEvt::_cmd_RUN(u8, s32)
{
	mCalls.mBuf[mCalls.mCount++] = (s32)(mData.mScript + 4);
	mData.mScript                = ResolveEventScriptOffset(mData.mScript);
	return 0;
}

s32 CEvt::_cmd_LIB(u8, s32)
{
	// like RUN but the target is a 2 byte index into the library table
	mCalls.mBuf[mCalls.mCount++] = (s32)(mData.mScript + 2);
	u8* p                        = mData.mScript;
	s16 idx                      = p[0] + (p[1] << 8);
	mData.mScript                = p + 2;
	mData.mScript                = GetGlobalScriptByIndex(idx);
	return 0;
}

s32 CEvt::_cmd_GOTO(u8, s32)
{
	mData.mScript = ResolveEventScriptOffset(mData.mScript);
	return 0;
}

s32 CEvt::_cmd_POPBP(u8, s32)
{
	mValues.mCount = mFrames.mBuf[--mFrames.mCount];
	return 0;
}

s32 CEvt::_cmd_PUSHBP(u8, s32)
{
	mFrames.mBuf[mFrames.mCount++] = mValues.mCount;
	return 0;
}

s32 CEvt::_cmd_SET(u8, s32)
{
	CEvt_EvalSet(this);
	return 0;
}
