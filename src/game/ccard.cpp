#include "game/ccard.h"

/**
 * @note Address: 0x801A93C8
 * @note Size: 0x11C
 */
const char* CCard::GetResultString(s32 result)
{
	switch (result) {
	case CARD_RESULT_UNLOCKED:
		return "CARD_RESULT_UNLOCKED";
	case CARD_RESULT_READY:
		return "CARD_RESULT_READY";
	case CARD_RESULT_BUSY:
		return "CARD_RESULT_BUSY";
	case CARD_RESULT_WRONGDEVICE:
		return "CARD_RESULT_WRONGDEVICE";
	case CARD_RESULT_NOCARD:
		return "CARD_RESULT_NOCARD";
	case CARD_RESULT_NOFILE:
		return "CARD_RESULT_NOFILE";
	case CARD_RESULT_IOERROR:
		return "CARD_RESULT_IOERROR";
	case CARD_RESULT_BROKEN:
		return "CARD_RESULT_BROKEN";
	case CARD_RESULT_EXIST:
		return "CARD_RESULT_EXIST";
	case CARD_RESULT_NOENT:
		return "CARD_RESULT_NOENT";
	case CARD_RESULT_INSSPACE:
		return "CARD_RESULT_INSSPACE";
	case CARD_RESULT_NOPERM:
		return "CARD_RESULT_NOPERM";
	case CARD_RESULT_LIMIT:
		return "CARD_RESULT_LIMIT";
	case CARD_RESULT_NAMETOOLONG:
		return "CARD_RESULT_NAMETOOLONG";
	case CARD_RESULT_ENCODING:
		return "CARD_RESULT_ENCODING";
	case CARD_RESULT_CANCELED:
		return "CARD_RESULT_CANCELED";
	case CARD_RESULT_FATAL_ERROR:
		return "CARD_RESULT_FATAL_ERROR";
	}
	return "Illegal";
}

/**
 * @note Address: 0x801A94E4
 * @note Size: 0x54
 */
s32 CCard::SetStatus(CARDStat* stat)
{
	// every wrapper here retries while the card is busy
	while ((mResult = CARDSetStatus(mChan, mFileInfo.fileNo, stat)) == CARD_RESULT_BUSY) { }
	return mResult;
}

/**
 * @note Address: 0x801A9538
 * @note Size: 0x54
 */
s32 CCard::GetStatus(CARDStat* stat)
{
	while ((mResult = CARDGetStatus(mChan, mFileInfo.fileNo, stat)) == CARD_RESULT_BUSY) { }
	return mResult;
}

/**
 * @note Address: 0x801A958C
 * @note Size: 0xB0
 */
u32 CCard::GetFreeBlocks(s32* bytesNotUsed, s32* filesNotUsed)
{
	u32 sectorSize;

	while ((mResult = CARDFreeBlocks(mChan, bytesNotUsed, filesNotUsed)) == CARD_RESULT_BUSY) { }
	if (mResult != CARD_RESULT_READY) {
		return 0;
	}

	// == where it meant =, so this never actually retries. retail bug
	while ((mResult == CARDGetSectorSize(mChan, &sectorSize)) == CARD_RESULT_BUSY) { }
	if (mResult != CARD_RESULT_READY) {
		return 0;
	}
	return *bytesNotUsed / sectorSize; // free sectors, not bytes
}

/**
 * @note Address: 0x801A963C
 * @note Size: 0x70
 */
s32 CCard::Write(void* buf, s32 length, s32 offset)
{
	while ((mResult = CARDWrite(&mFileInfo, buf, length, offset)) == CARD_RESULT_BUSY) { }
	return mResult;
}

/**
 * @note Address: 0x801A96AC
 * @note Size: 0x7C
 */
s32 CCard::Read(void* buf, s32 length, s32 offset)
{
	while ((mResult = CARDRead(&mFileInfo, buf, length, offset)) == CARD_RESULT_BUSY) { }
	DCInvalidateRange(buf, length); // the card dma wrote past the cache
	return mResult;
}

/**
 * @note Address: 0x801A9728
 * @note Size: 0x50
 */
s32 CCard::Delete(const char* name)
{
	while ((mResult = CARDDelete(mChan, name)) == CARD_RESULT_BUSY) { }
	return mResult;
}

/**
 * @note Address: 0x801A9778
 * @note Size: 0x38
 */
void CCard::Close()
{
	while (CARDClose(&mFileInfo) == CARD_RESULT_BUSY) { }
}

/**
 * @note Address: 0x801A97B0
 * @note Size: 0x40
 */
s32 CCard::CloseEx()
{
	while ((mResult = CARDClose(&mFileInfo)) == CARD_RESULT_BUSY) { }
	return mResult;
}

/**
 * @note Address: 0x801A97F0
 * @note Size: 0x54
 */
s32 CCard::Open(char* name)
{
	while ((mResult = CARDOpen(mChan, name, &mFileInfo)) == CARD_RESULT_BUSY) { }
	return mResult;
}

/**
 * @note Address: 0x801A9844
 * @note Size: 0x64
 */
s32 CCard::Create(char* name, u32 size)
{
	while ((mResult = CARDCreate(mChan, name, size, &mFileInfo)) == CARD_RESULT_BUSY) { }
	return mResult;
}

/**
 * @note Address: 0x801A98A8
 * @note Size: 0x40
 */
s32 CCard::Format()
{
	while ((mResult = CARDFormat(mChan)) == CARD_RESULT_BUSY) { }
	return mResult;
}

/**
 * @note Address: 0x801A98E8
 * @note Size: 0x40
 */
s32 CCard::Check()
{
	while ((mResult = CARDCheck(mChan)) == CARD_RESULT_BUSY) { }
	return mResult;
}

/**
 * @note Address: 0x801A9928
 * @note Size: 0x38
 */
void CCard::Unmount()
{
	while (CARDUnmount(mChan) == CARD_RESULT_BUSY) { }
}

/**
 * @note Address: 0x801A9960
 * @note Size: 0x40
 */
s32 CCard::UnmountEx()
{
	while ((mResult = CARDUnmount(mChan)) == CARD_RESULT_BUSY) { }
	return mResult;
}

/**
 * @note Address: 0x801A99A0
 * @note Size: 0x58
 */
s32 CCard::Mount(s32 chan, CARDCallback detachCallback)
{
	mChan = chan;
	while ((mResult = CARDMount(mChan, (CARDMemoryCard*)mWorkArea, detachCallback)) == CARD_RESULT_BUSY) { }
	return mResult;
}

/**
 * @note Address: 0x801A99F8
 * @note Size: 0x3C
 */
s32 CCard::Probe(s32 chan, s32* memSize, s32* sectorSize)
{
	// a single shot, unlike ProbeEx no busy retry
	s32 result = CARDProbeEx(chan, memSize, sectorSize);
	mResult    = result;
	return result;
}

/**
 * @note Address: 0x801A9A34
 * @note Size: 0x6C
 */
s32 CCard::ProbeEx(s32 chan, s32* memSize, s32* sectorSize)
{
	while ((mResult = CARDProbeEx(chan, memSize, sectorSize)) == CARD_RESULT_BUSY) { }
	return mResult;
}

/**
 * @note Address: 0x801A9AA0
 * @note Size: 0x34
 */
void CCard::Init()
{
	CARDInit();
	mResult = CARD_RESULT_READY;
}
