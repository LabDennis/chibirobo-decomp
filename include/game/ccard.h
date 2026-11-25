#ifndef _GAME_CCARD_H
#define _GAME_CCARD_H

#include "Dolphin/card.h"

// a memory card slot and one open file on it. every call retries while the card says busy
// and keeps the last result. class name from RTTI. Memcard holds one at +0x20, the vtable
// and dtor get emitted with CGame

class CCard {
public:
	s32 mChan;                        // _00
	CARDFileInfo mFileInfo;           // _04
	s32 mResult;                      // _18
	u8 _1C[0x4];                      // _1C
	u8 mWorkArea[CARD_WORKAREA_SIZE]; // _20, for CARDMount
	// _A020 vtable

	virtual ~CCard() { } // _08

	const char* GetResultString(s32 result);
	s32 SetStatus(CARDStat* stat);
	s32 GetStatus(CARDStat* stat);
	u32 GetFreeBlocks(s32* bytesNotUsed, s32* filesNotUsed);
	s32 Write(void* buf, s32 length, s32 offset);
	s32 Read(void* buf, s32 length, s32 offset);
	s32 Delete(const char* name);
	void Close();
	s32 CloseEx();
	s32 Open(char* name);
	s32 Create(char* name, u32 size);
	s32 Format();
	s32 Check();
	void Unmount();
	s32 UnmountEx();
	s32 Mount(s32 chan, CARDCallback detachCallback);
	s32 Probe(s32 chan, s32* memSize, s32* sectorSize);
	s32 ProbeEx(s32 chan, s32* memSize, s32* sectorSize);
	void Init();
};
typedef char CCard_size_check[sizeof(CCard) == 0xA024 ? 1 : -1];

#endif
