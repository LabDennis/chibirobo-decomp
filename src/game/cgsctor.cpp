#include "game/hsd_jobj.h"
#include "game/cgs.h"
#include "PowerPC_EABI_Support/Runtime/MWCPlusLib.h"

extern "C" {
extern void* __vt__3CGs[];
extern void* __vt__9CGsScreen[];
extern void* __vt__13CGsRenderMode[];
extern void* __vt__5CFont[];
extern void* __vt__7CTexObj[];
extern void* __vt__10CPrimitive[];
extern void* __vt__7CSprite[];
extern void* __vt__5CLine[];
extern void* __vt__9CTriangle[];
extern void* __vt__7CCircle[];
extern void* __vt__5CTile[];
extern void* __vt__5CCube[];
extern void* __vt__11COctahedron[];
extern void* __vt__7CDodeca[];
extern void* __vt__12CIcosahedron[];
extern void* __vt__7CSphere[];
extern void* __vt__9CCylinder[];
extern void* __vt__6CTorus[];
// the weak ctors and dtors of CGs's arrays
CGsTevStage* __ct__11CGsTevStageFv(CGsTevStage* stage);
CGsTevStage* __dt__11CGsTevStageFv(CGsTevStage* stage, s32 flags);
CGsChannel* __ct__10CGsChannelFv(CGsChannel* channel);
CGsChannel* __dt__10CGsChannelFv(CGsChannel* channel, s32 flags);
}

/**
 * @note Address: 0x8001B9B8
 * @note Size: 0x1A4
 */
// todo: weak copy
extern "C" CGs* __ct__3CGsFv(CGs* gs)
{
#if defined(VERSION_JP)
	*(void***)((u8*)gs + sizeof(CGs) - 4) = __vt__3CGs;
#else
	*(void***)((u8*)&gs->mTextRot - 4) = __vt__3CGs;
#endif
	*(void***)((u8*)&gs->mScreen + sizeof(CGsScreen) - 4) = __vt__9CGsScreen;
	gs->mScreen.mMode                                     = -1;
	*(void***)((u8*)&gs->mRenderMode + 0x30)              = __vt__13CGsRenderMode;
	__construct_array(gs->mTevStages, (ConstructorDestructor)__ct__11CGsTevStageFv, (ConstructorDestructor)__dt__11CGsTevStageFv,
	                  sizeof(CGsTevStage), 2);
	__construct_array(gs->mChannels, (ConstructorDestructor)__ct__10CGsChannelFv, (ConstructorDestructor)__dt__10CGsChannelFv,
	                  sizeof(CGsChannel), 4);
	*(void***)((u8*)&gs->mFont + 0x13C)        = __vt__5CFont;
	*(void***)((u8*)&gs->mFont.mTex + 0xFC)    = __vt__7CTexObj;
	gs->mFont.mDesc.textureHeader              = NULL;
	*(void***)((u8*)&gs->mTex + 0xFC)          = __vt__7CTexObj;
	*(void***)((u8*)&gs->mSprite + 0x164)      = __vt__10CPrimitive;
	*(void***)((u8*)&gs->mSprite + 0x164)      = __vt__7CSprite;
	*(void***)((u8*)&gs->mLine + 0x164)        = __vt__10CPrimitive;
	*(void***)((u8*)&gs->mLine + 0x164)        = __vt__5CLine;
	*(void***)((u8*)&gs->mTriangle + 0x164)    = __vt__10CPrimitive;
	*(void***)((u8*)&gs->mTriangle + 0x164)    = __vt__9CTriangle;
	*(void***)((u8*)&gs->mCircle + 0x164)      = __vt__10CPrimitive;
	*(void***)((u8*)&gs->mCircle + 0x164)      = __vt__7CCircle;
	*(void***)((u8*)&gs->mTile + 0x164)        = __vt__10CPrimitive;
	*(void***)((u8*)&gs->mTile + 0x164)        = __vt__5CTile;
	*(void***)((u8*)&gs->mSprite2 + 0x164)     = __vt__10CPrimitive;
	*(void***)((u8*)&gs->mSprite2 + 0x164)     = __vt__7CSprite;
	*(void***)((u8*)&gs->mCube + 0x164)        = __vt__10CPrimitive;
	*(void***)((u8*)&gs->mCube + 0x164)        = __vt__5CCube;
	*(void***)((u8*)&gs->mOctahedron + 0x164)  = __vt__10CPrimitive;
	*(void***)((u8*)&gs->mOctahedron + 0x164)  = __vt__11COctahedron;
	*(void***)((u8*)&gs->mDodeca + 0x164)      = __vt__10CPrimitive;
	*(void***)((u8*)&gs->mDodeca + 0x164)      = __vt__7CDodeca;
	*(void***)((u8*)&gs->mIcosahedron + 0x164) = __vt__10CPrimitive;
	*(void***)((u8*)&gs->mIcosahedron + 0x164) = __vt__12CIcosahedron;
	*(void***)((u8*)&gs->mSphere + 0x164)      = __vt__10CPrimitive;
	*(void***)((u8*)&gs->mSphere + 0x164)      = __vt__7CSphere;
	*(void***)((u8*)&gs->mCylinder + 0x164)    = __vt__10CPrimitive;
	*(void***)((u8*)&gs->mCylinder + 0x164)    = __vt__9CCylinder;
	*(void***)((u8*)&gs->mTorus + 0x164)       = __vt__10CPrimitive;
	*(void***)((u8*)&gs->mTorus + 0x164)       = __vt__6CTorus;
	return gs;
}
