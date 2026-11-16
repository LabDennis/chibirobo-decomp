// The Wii save manager, one file from InitializeWiiSaveManager (0x800A9320) to its static initializer
// (0x800AACEC), which constructs gWiiSaveThread. Retail's strings show the banner and slot code are one file:
// "banner.bin" is its own object, and the string pool that starts with "CB_SKIP(%d)" follows it 4-aligned,
// where the next file's pool would start 8-aligned.
// NONMATCHING: all functions have source now; the diffs left are mostly register allocation and
// scheduling. The sprintf sites in ReadWiiSaveSlot and WriteWiiSaveSlot show branch stubs around the
// inlined path helper that MWCC does not reproduce here, and ApplyWiiSaveSlot's min-clamps come out as
// the and-idiom instead of the selects retail has. ScanWiiSaveSlots differs in three optimizer
// decisions: retail recomputes the entry address per status store instead of caching the base in a
// register, materializes the VerifyWiiSaveSlotChecksum result as a 0/1 value rather than folding the
// comparison into the branch, strength-reduces the tail loop to a running byte offset, and allocates
// two fewer callee-saved registers than this version (stmw r16 / -0x90 frame against stmw r14 /
// -0xa0). The tail loop's `(status & 4) || (status & 8) || (status & 0x10)` guard is redundant next
// to the two real tests inside; retail still emits all three rlwinm. records (0x800A9BE0-F8) with no
// branch before testing 8 and 0x10 live, which is the shape this spelling leaves behind. "banner.bin"
// is its own rodata object in retail; as a plain literal here it joins the unit's @stringBase0 pool
// instead, which the remaining referent diffs in ReadWiiSaveSlot/WriteWiiSaveSlot show.
// CheckWiiSaveBanner (0xE4, same as retail) and BuildWiiSaveBanner (0x284, one instruction short of
// retail's 0x288) are reached via bl in retail, as they are under this unit's -inline noauto.
// StartWiiSaveJob and GetWiiSaveSlotChecksum are static inline and inline fully; retail likewise
// has no call to either. ReadWiiSaveSlot lines up except for the stubs and its folded checksum
// select, and the static initializer does not register CThread's destructor as retail's does.
// The file's data is not assigned to the unit until the rest has source.
#include "game/wii/saveslots.h"
#include "game/wii/cgame.h"
#include "game/cthread.h"
#include "game/globals.h"
#include "revolution/mem.h"
#include "Dolphin/OS/OSUtil.h"
#include "stl/stdio.h"
#include "stl/mem.h"

struct WiiSaveJob {
	CSaveFile* mManager;
	s32 mSlot;
	s32 mNewGame;
};

extern "C" {
extern MEMiHeapHead* gWiiGameHeap;
extern s32 GetStageNum();

CThread gWiiSaveThread;
WiiSaveJob gWiiSaveJob;
const char* gWiiSaveBannerPath = "banner.bin";
void* gWiiSaveNandWorkBuffer;
u8* gWiiSaveThreadStack;

void ScanWiiSaveSlots(CSaveFile* manager, int count);
void CaptureWiiSaveSlot(CSaveFile* manager, bool newGame);
s32 GetDefaultStageIndex();
void ResetPlayerHeadOffset(WiiPlayerState* player);
void ResetPlayerPlug(WiiPlayerState* player);
void SetMapCameraKeepPreviousEye(CInterfaceCameraAction* action);

void* RunWiiSaveSlotScan(void* param);
void* RunWiiSaveSlotWrite(void* param);
void* RunWiiSaveSlotRead(void* param);
void* RunWiiSaveSlotDelete(void* param);
}

extern "C" void InitializeWiiSaveManager(CSaveFile* manager)
{
	InitializeNandGameFile(&manager->mFile);
	gWiiSaveNandWorkBuffer = MEMAllocFromExpHeapEx(gWiiGameHeap, 0x4000, 32);
	gWiiSaveThreadStack    = static_cast<u8*>(MEMAllocFromExpHeapEx(gWiiGameHeap, 0x2000, 32));
	GetNandGameHomeDir(&manager->mFile, manager->mHomeDir);
	u32 blocks, inodes;
	GetNandGameFreeSpace(&manager->mFile, &blocks, &inodes);
}

extern "C" void UpdateWiiSaveJob(CSaveFile* manager)
{
	if (manager->mJobStarted && OSIsThreadTerminated(&gWiiSaveThread.mThread))
		manager->mJobStarted = 0;
}

extern "C" int CheckWiiSaveBanner(CSaveFile* manager, bool removeInvalid)
{
	WiiGameSaveFile* file = &manager->mFile;
	u32 flags             = 0;
	if (OpenNandGameFile(file, gWiiSaveBannerPath, 1, gWiiSaveNandWorkBuffer, 0x4000) == 0) {
		u32 length;
		if (GetNandGameFileLength(file, &length) == 0) {
			if (length == sizeof(manager->mBanner)) {
				if (CloseNandGameFile(file) == 0)
					flags |= 1;
				else
					flags |= 4;
			} else
				flags |= 6;
		} else
			flags |= 6;
	}
	if (!(flags & 1)) {
		if (flags & 2)
			CloseNandGameFile(file);
		if ((flags & 4) && removeInvalid)
			DeleteNandGameFile(file, gWiiSaveBannerPath);
		flags = 0;
	}
	return flags;
}

extern "C" void BuildWiiSaveBanner(CSaveFile* manager)
{
	memset(manager->mBanner.comment[0], 0x30, 0x20);
	memset(manager->mBanner.comment[1], 0x30, 0x20);
	const u16* subtitle = reinterpret_cast<const u16*>(GetLocalizedInterfaceText(2));
	const u16* title    = reinterpret_cast<const u16*>(GetLocalizedInterfaceText(1));
	NANDInitBanner(&manager->mBanner, 0, title, subtitle);
	gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[247], 0);
	CTexObj tex = gGame.mTpl.mTex;
	memcpy(manager->mBanner.bannerTexture, tex.mImageData, sizeof(manager->mBanner.bannerTexture));
	for (int i = 0; i < 8; ++i) {
		gGame.mTpl.mTex.Init(&gGame.mTpl.mPalette->descriptorArray[248 + i], 0);
		CTexObj icon = gGame.mTpl.mTex;
		memcpy(manager->mBanner.iconTexture[i], icon.mImageData, sizeof(manager->mBanner.iconTexture[i]));
		manager->mBanner.iconSpeed = (manager->mBanner.iconSpeed & ~(3 << (2 * i))) | (1 << (2 * i));
	}
	manager->mBanner.iconSpeed = (manager->mBanner.iconSpeed & ~(3 << (2 * 8))) | (0 << (2 * 8));
}

extern "C" s32 WriteWiiSaveBanner(CSaveFile* manager)
{
	BuildWiiSaveBanner(manager);
	WiiGameSaveFile* file = &manager->mFile;
	for (int attempt = 0; attempt < 3; ++attempt) {
		s32 result = CreateNandGameFile(file, gWiiSaveBannerPath, 0x30, 0);
		if (result == 0)
			break;
		if (result == NAND_RESULT_EXISTS) {
			result = DeleteNandGameFile(file, gWiiSaveBannerPath);
			if (result == 0)
				continue;
			return result;
		}
		return result;
	}
	s32 result = OpenNandGameFile(file, gWiiSaveBannerPath, 2, gWiiSaveNandWorkBuffer, 0x4000);
	if (result != 0)
		return result;
	result = WriteNandGameFile(file, &manager->mBanner, sizeof(manager->mBanner));
	if (result < 0)
		return result;
	result = CloseNandGameFile(file);
	return result == 0 ? 0 : result;
}

static inline u32 GetWiiSaveSlotChecksum(WiiSaveSlot& slot)
{
	slot.mHeader.mChecksum = 0;
	const u16* p           = reinterpret_cast<const u16*>(&slot);
	u32 sum                = 0;
	u32 i                  = sizeof(slot) / 2;
	while (i-- != 0)
		sum += *p++;
	return sum;
}

static inline bool VerifyWiiSaveSlotChecksum(WiiSaveSlot& slot)
{
	u32 storedChecksum = slot.mHeader.mChecksum ^ 0x50494b53;
	return storedChecksum == GetWiiSaveSlotChecksum(slot);
}

void ScanWiiSaveSlots(CSaveFile* manager, int count)
{
	static WiiSaveSlot sScanSlot;
	char path[0x40];
	int usable = 0;
	int empty  = 0;
	int i;
	s32 result;

	ChangeNandGameDir(&manager->mFile, manager->mHomeDir);
	int hasBanner = CheckWiiSaveBanner(manager, false);

	for (i = 0; i < count; ++i) {
		sprintf(path, "CB_SKIP(%d)", i);
		manager->mScanEntries[i].mStatus = 0;
		memset(manager->mScanEntries[i].mData, 0, sizeof(manager->mScanEntries[i].mData));
		result = OpenNandGameFile(&manager->mFile, path, 1, gWiiSaveNandWorkBuffer, 0x4000);
		GetNandGameResultName(&manager->mFile, result);
		switch (result) {
		case NAND_RESULT_OK: {
			u32 length;
			result = GetNandGameFileLength(&manager->mFile, &length);
			if (result == 0 && length == sizeof(sScanSlot)) {
				result = ReadNandGameFile(&manager->mFile, &sScanSlot, sizeof(sScanSlot));
				if (result >= 0) {
					bool ok = VerifyWiiSaveSlotChecksum(sScanSlot);
					if (ok) {
						// retail copies the whole 0x2D20-byte slot into the 0x80-byte
						// mData field - an overflow in the original, spilling into the
						// entry tail and the following scan entries
						memcpy(manager->mScanEntries[i].mData, &sScanSlot, sizeof(sScanSlot));
						manager->mScanEntries[i].mStatus = 1;
					} else {
						manager->mScanEntries[i].mStatus = 3;
						GetNandGameResultName(&manager->mFile, result);
					}
					CloseNandGameFile(&manager->mFile);
				} else {
					switch (result) {
					case NAND_RESULT_ECC_CRIT:
					case NAND_RESULT_AUTHENTICATION:
					default:
						manager->mScanEntries[i].mStatus = 3;
						CloseNandGameFile(&manager->mFile);
						break;
					case NAND_RESULT_CORRUPT:
						i = count;
						break;
					}
					GetNandGameResultName(&manager->mFile, result);
				}
			} else {
				manager->mScanEntries[i].mStatus = 3;
				CloseNandGameFile(&manager->mFile);
				GetNandGameResultName(&manager->mFile, result);
			}
			++usable;
			break;
		}
		case NAND_RESULT_AUTHENTICATION:
		case NAND_RESULT_ECC_CRIT:
			manager->mScanEntries[i].mStatus = 3;
			++usable;
			break;
		case NAND_RESULT_CORRUPT:
			i = count;
			break;
		case NAND_RESULT_NOEXISTS:
			manager->mScanEntries[i].mStatus = 4;
			++empty;
			manager->mFile.mResult = 0;
			break;
		default:
			manager->mScanEntries[i].mStatus = 4;
			++empty;
			manager->mFile.mResult = 0;
			break;
		}
	}
	GetNandGameResultName(&manager->mFile, manager->mFile.mResult);
	if (manager->mFile.mResult == 0 && empty > 0) {
		u32 answer;
		int needed = 0;
		for (i = 0; i < count; ++i) {
			if (!(manager->mScanEntries[i].mStatus & 4))
				continue;
			++needed;
			u32 blocks = needed;
			u32 files  = needed;
			if (!hasBanner) {
				blocks = needed + 4;
				files  = needed + 1;
			}
			result = CheckNandGameSpace(&manager->mFile, blocks, files, &answer);
			GetNandGameResultName(&manager->mFile, result);
			if (result != 0)
				manager->mFile.mResult = 0;
			else if (answer == 0)
				++usable;
			else if (answer & 5)
				manager->mScanEntries[i].mStatus = 8;
			else if (answer & 0xa)
				manager->mScanEntries[i].mStatus = 0x10;
		}
		if (usable == 0) {
			for (i = 0; i < count; ++i) {
				if ((manager->mScanEntries[i].mStatus & 4) || (manager->mScanEntries[i].mStatus & 8)
				    || (manager->mScanEntries[i].mStatus & 0x10)) {
					if (manager->mScanEntries[i].mStatus & 8) {
						manager->mFile.mResult = NAND_RESULT_MAXBLOCKS;
						return;
					} else if (manager->mScanEntries[i].mStatus & 0x10) {
						manager->mFile.mResult = NAND_RESULT_MAXFILES;
						return;
					}
				}
			}
		}
	}
}

static inline void StartWiiSaveJob(CSaveFile* manager, OSThreadStartFunction entry)
{
	gWiiSaveThread.mStack     = gWiiSaveThreadStack;
	gWiiSaveThread.mStackSize = 0x2000;
	OSCreateThread(&gWiiSaveThread.mThread, entry, &gWiiSaveJob, gWiiSaveThreadStack + 0x2000, 0x2000, 17, OS_THREAD_ATTR_DETACH);
	OSResumeThread(&gWiiSaveThread.mThread);
	manager->mJobStarted = 1;
}

static inline void MakeWiiSaveSlotPath(CSaveFile* manager, int slot)
{
	sprintf(manager->mSlotPath, "CB_SKIP(%d)", slot);
}

extern "C" void StartWiiSaveSlotScan(CSaveFile* manager, int count)
{
	gWiiSaveJob.mManager = manager;
	gWiiSaveJob.mSlot    = count;
	StartWiiSaveJob(manager, RunWiiSaveSlotScan);
}

extern "C" void* RunWiiSaveSlotScan(void* param)
{
	WiiSaveJob* job = static_cast<WiiSaveJob*>(param);
	ScanWiiSaveSlots(job->mManager, job->mSlot);
	return NULL;
}

extern "C" s32 WriteWiiSaveSlot(CSaveFile* manager, int slot, bool newGame)
{
	MakeWiiSaveSlotPath(manager, slot);
	memset(&manager->mSlot, 0, sizeof(manager->mSlot));
	CaptureWiiSaveSlot(manager, newGame);
	ChangeNandGameDir(&manager->mFile, manager->mHomeDir);
	int hasBanner = CheckWiiSaveBanner(manager, true);
	s32 result    = OpenNandGameFile(&manager->mFile, manager->mSlotPath, 2, gWiiSaveNandWorkBuffer, 0x4000);
	if (result != 0) {
		u32 answer;
		u32 blocks = 1;
		u32 files  = 1;
		if (!hasBanner) {
			blocks = 5;
			files  = 2;
		}
		result = CheckNandGameSpace(&manager->mFile, blocks, files, &answer);
		if (result != 0)
			return result;
		if (answer & 5)
			return manager->mFile.mResult = NAND_RESULT_MAXBLOCKS;
		if (answer & 10)
			return manager->mFile.mResult = NAND_RESULT_MAXFILES;
		result = CreateNandGameFile(&manager->mFile, manager->mSlotPath, 0x30, 0);
		if (result != 0)
			return result;
		result = OpenNandGameFile(&manager->mFile, manager->mSlotPath, 2, gWiiSaveNandWorkBuffer, 0x4000);
		if (result != 0)
			return result;
	}
	manager->mSlot.mHeader.mChecksum = GetWiiSaveSlotChecksum(manager->mSlot) ^ 0x50494b53;
	result                           = WriteNandGameFile(&manager->mFile, &manager->mSlot, sizeof(manager->mSlot));
	if (result < 0) {
		CloseNandGameFile(&manager->mFile);
		return result;
	}
	result = CloseNandGameFile(&manager->mFile);
	if (result != 0)
		return result;
	WriteWiiSaveBanner(manager);
	return 0;
}

extern "C" void StartWiiSaveSlotWrite(CSaveFile* manager, int slot, int newGame)
{
	gWiiSaveJob.mManager = manager;
	gWiiSaveJob.mNewGame = newGame;
	gWiiSaveJob.mSlot    = slot;
	StartWiiSaveJob(manager, RunWiiSaveSlotWrite);
}

extern "C" void* RunWiiSaveSlotWrite(void* param)
{
	WiiSaveJob* job = static_cast<WiiSaveJob*>(param);
	WriteWiiSaveSlot(job->mManager, job->mSlot, job->mNewGame != 0);
	return NULL;
}

extern "C" s32 ReadWiiSaveSlot(CSaveFile* manager, int slot)
{
	MakeWiiSaveSlotPath(manager, slot);
	memset(&manager->mSlot, 0, sizeof(manager->mSlot));
	ChangeNandGameDir(&manager->mFile, manager->mHomeDir);
	s32 result = OpenNandGameFile(&manager->mFile, manager->mSlotPath, 1, gWiiSaveNandWorkBuffer, 0x4000);
	if (result != 0)
		return result;
	result = ReadNandGameFile(&manager->mFile, &manager->mSlot, sizeof(manager->mSlot));
	if (result < 0) {
		CloseNandGameFile(&manager->mFile);
		return result;
	}
	result = VerifyWiiSaveSlotChecksum(manager->mSlot);
	if (result == 0)
		return NAND_RESULT_UNKNOWN;
	if (manager->mSlot.mHeader.mStages[0] < 0)
		manager->mSlot.mHeader.mStages[0] = manager->mSlot.mHeader.mStages[1] = GetDefaultStageIndex();
	result = CloseNandGameFile(&manager->mFile);
	if (result != 0)
		return result;
	return 0;
}

extern "C" void StartWiiSaveSlotRead(CSaveFile* manager, int slot)
{
	gWiiSaveJob.mManager = manager;
	gWiiSaveJob.mSlot    = slot;
	StartWiiSaveJob(manager, RunWiiSaveSlotRead);
}

extern "C" void* RunWiiSaveSlotRead(void* param)
{
	WiiSaveJob* job = static_cast<WiiSaveJob*>(param);
	ReadWiiSaveSlot(job->mManager, job->mSlot);
	return NULL;
}

extern "C" void StartWiiSaveSlotDelete(CSaveFile* manager, int slot)
{
	gWiiSaveJob.mManager = manager;
	gWiiSaveJob.mSlot    = slot;
	StartWiiSaveJob(manager, RunWiiSaveSlotDelete);
}

extern "C" void* RunWiiSaveSlotDelete(void* param)
{
	WiiSaveJob* job    = static_cast<WiiSaveJob*>(param);
	CSaveFile* manager = job->mManager;
	MakeWiiSaveSlotPath(manager, job->mSlot);
	DeleteNandGameFile(&manager->mFile, manager->mSlotPath);
	return NULL;
}

static inline void CopyVec(Vec& dst, const Vec& src)
{
	dst.x = src.x;
	dst.y = src.y;
	dst.z = src.z;
}

extern "C" void CaptureWiiSaveSlot(CSaveFile* manager, bool newGame)
{
	manager->mSlot.mHeader.mSignature = 0x53464342;
	manager->mSlot.mHeader.mVersion   = 0x100;
	manager->mSlot.mHeader.mTimestamp = OSGetTime();
	if (newGame) {
		manager->mSlot.mHeader.mStages[0] = manager->mSlot.mHeader.mStages[1] = -1;
		return;
	}
	// gGame.mStage and gGame.mPrevStage are adjacent, read as a pair
	s32* stages = &gGame.mStage;
	for (int i = 0; i < 2; ++i) {
		manager->mSlot.mHeader.mStages[i] = stages[i];
		if (manager->mSlot.mHeader.mStages[i] > GetStageNum())
			manager->mSlot.mHeader.mStages[i] -= GetStageNum() - 99;
	}
	manager->mSlot.mHeader.mStatusFlags  = (u32)gGame.GetStatusFlags(1);
	manager->mSlot.mHeader._18           = 0;
	manager->mSlot.mHeader.mPlayTime     = gGame.mPlayTime;
	manager->mSlot.mHeader.mTimeMode     = gGame.mTimeState.mMode;
	manager->mSlot.mHeader.mTime         = gGame.mTimeState.mTime;
	manager->mSlot.mHeader.mDuration     = gGame.mTimeState.mDuration;
	manager->mSlot.mHeader.mExp          = gGame.GetPlayer()->mParams.mExperience;
	manager->mSlot.mHeader.mLevel        = gGame.GetPlayer()->mParams.mLevel;
	manager->mSlot.mHeader.mRank         = gGame.GetPlayer()->mParams.mRank;
	manager->mSlot.mHeader.mBattery      = gGame.GetPlayer()->mParams.mBattery;
	manager->mSlot.mHeader.mMaxBattery   = gGame.GetPlayer()->mParams.mMaxBattery;
	manager->mSlot.mHeader.mMoney        = gGame.GetPlayer()->mParams.mMoney;
	manager->mSlot.mHeader.mScrap        = gGame.GetPlayer()->mParams.mScrap;
	manager->mSlot.mHeader.mActive       = gGame.GetPlayer()->mParams._1C[0];
	manager->mSlot.mHeader.mMenuControls = gGame.mMenuControls;
	memcpy(manager->mSlot.mData.mPlayer, gGame.GetPlayer()->mParams.mInventory.mItems, sizeof(manager->mSlot.mData.mPlayer));
	memcpy(manager->mSlot.mData.mEvtFlags, gEventFlags.mFlags, sizeof(manager->mSlot.mData.mEvtFlags));
	memcpy(manager->mSlot.mData.mEvtVars, gEventVars.mVars, sizeof(manager->mSlot.mData.mEvtVars));
	memcpy(manager->mSlot.mData.mCoinFlags, ((WiiFlagSource*)gGame.mXObjs.mPools[7].mObjects)->mFlags,
	       sizeof(manager->mSlot.mData.mCoinFlags));
	memcpy(manager->mSlot.mData.mItemFlags, ((WiiFlagSource*)gGame.mXObjs.mPools[8].mObjects)->mFlags,
	       sizeof(manager->mSlot.mData.mItemFlags));
	memcpy(manager->mSlot.mData.mDustFlags, ((WiiFlagSource*)gGame.mXObjs.mPools[20].mObjects)->mFlags,
	       sizeof(manager->mSlot.mData.mDustFlags));
	memcpy(manager->mSlot.mData.mPlayerRecord, gGame.GetPlayer()->mAttachments, sizeof(manager->mSlot.mData.mPlayerRecord));
	manager->mSlot.mData.mSuit = gGame.GetPlayer()->mSuits.mCurrent;
	// the Wii player class is not reconstructed; the +0x244 getter returns its position Vec
	const Vec* pos = gGame.GetPlayer()->mVtable->GetPosition(gGame.GetPlayer());
	CopyVec(manager->mSlot.mData.mPos, *pos);
	manager->mSlot.mData.mRotY        = gGame.GetPlayer()->mRotY;
	manager->mSlot.mData.mSpecialSuit = gGame.GetPlayer()->mSuits.mSpecial;
	if (manager->mSlot.mHeader.mStatusFlags & 1) {
		CMotionControlCamera* camera       = &gGame.mMotionControlCamera;
		manager->mSlot.mData.mCameraAction = camera->GetActionType();
		CopyVec(manager->mSlot.mData.mCameraEye, camera->mView.mEye);
		CopyVec(manager->mSlot.mData.mCameraInterest, camera->mView.mInterest);
		CopyVec(manager->mSlot.mData.mCameraAngle, camera->mView.mAngle);
		manager->mSlot.mData.mCameraDistance = camera->mView.mDistance;
		manager->mSlot.mData.mCameraFov      = camera->mFov;
	}
	manager->mSlot.mData.mSquirterLiquid = gGame.GetPlayer()->mSquirterContent;
	manager->mSlot.mData.mItemTint       = gGame.GetPlayer()->mItemTint;
}

// reads a saved stage back into a live stage index, the inverse of the remap CaptureWiiSaveSlot does
extern "C" s32 GetWiiSaveSlotStage(CSaveFile* manager, int index)
{
	s32 stage = manager->mSlot.mHeader.mStages[index];
	if (stage >= 100)
		stage += GetStageNum() - 99;
	return stage;
}

static inline void SetExperience(WiiPlayerParams* stats, s32 value)
{
	stats->mExperience = value > 0 ? value : 0;
	if (stats->mExperience < 0)
		stats->mExperience = 0;
	if (stats->mExperience > 99999)
		stats->mExperience = 99999;
}

static inline void SetLevel(WiiPlayerParams* stats, s32 value)
{
	stats->mLevel = value > 0 ? value : 0;
	if (stats->mLevel < 0)
		stats->mLevel = 0;
	if (stats->mLevel > 30)
		stats->mLevel = 30;
}

static inline void SetRank(WiiPlayerParams* stats, s32 value)
{
	stats->mRank = value > 0 ? value : 0;
	if (stats->mRank < 1)
		stats->mRank = 1;
	if (stats->mRank > 1000000)
		stats->mRank = 1000000;
}

static inline void SetMaxBattery(WiiPlayerParams* stats, f32 value)
{
	stats->mMaxBattery = value > 0.0f ? value : 0.0f;
	if (stats->mMaxBattery < 0.0f)
		stats->mMaxBattery = 0.0f;
	if (stats->mMaxBattery > 999.9f)
		stats->mMaxBattery = 999.9f;
	stats->SetBattery(stats->mBattery);
}

static inline void RequestSuit(WiiPlayerState* chr, s32 value)
{
	if (chr->mSuits.mChangeStep == FALSE) { // only when no suit change is in progress
		chr->mSuits.mRequested  = value;
		chr->mSuits.mChangeStep = TRUE;
	}
}

// NONMATCHING: retail lowers the integer `value > 0 ? value : 0` selects in the
// Set* helpers as compare/branch diamonds (cmpwi; ble arm; b join; li 0), while
// GC/3.0a5.2 folds them to the neg/andc/srawi/and. arithmetic idiom here. Every
// honest spelling of the clamp (ternary, statement if, if/else store diamonds,
// return-based setters) folds the same way; the tail and every gGame.GetPlayer()
// reload match instruction-for-instruction, so the divergence is the
// select-lowering decision, not the source shape.

// applies the loaded slot to the live game state; the position and the camera go through
// RestoreSavedPlayerAndCamera afterwards
extern "C" void ApplyWiiSaveSlot(CSaveFile* manager)
{
	s32 stage = manager->mSlot.mHeader.mStages[0];
	if (stage >= 100)
		stage = GetStageNum() + stage - 99;
	gGame.mPrevStage           = gGame.mStage;
	gGame.mStage               = stage;
	gGame.mTimeState.mMode     = manager->mSlot.mHeader.mTimeMode;
	gGame.mTimeState.mTime     = manager->mSlot.mHeader.mTime;
	gGame.mTimeState.mRunning  = 0;
	gGame.mTimeState.mDuration = manager->mSlot.mHeader.mDuration;
	SetExperience(&gGame.GetPlayer()->mParams, manager->mSlot.mHeader.mExp);
	SetLevel(&gGame.GetPlayer()->mParams, manager->mSlot.mHeader.mLevel);
	SetRank(&gGame.GetPlayer()->mParams, manager->mSlot.mHeader.mRank);
	SetMaxBattery(&gGame.GetPlayer()->mParams, manager->mSlot.mHeader.mMaxBattery);
	gGame.GetPlayer()->mParams.SetBattery(manager->mSlot.mHeader.mBattery);
	gGame.GetPlayer()->mParams.SetMoney(manager->mSlot.mHeader.mMoney);
	gGame.GetPlayer()->mParams.SetScrap(manager->mSlot.mHeader.mScrap);
	if (manager->mSlot.mHeader.mActive)
		gGame.GetPlayer()->mParams._1C[0] = 1;
	else
		gGame.GetPlayer()->mParams._1C[0] = 0;
	gGame.mPlayTime = manager->mSlot.mHeader.mPlayTime;
	memcpy(gGame.GetPlayer()->mParams.mInventory.mItems, manager->mSlot.mData.mPlayer, sizeof(manager->mSlot.mData.mPlayer));
	memcpy(gEventFlags.mFlags, manager->mSlot.mData.mEvtFlags, sizeof(gEventFlags.mFlags));
	memcpy(gEventVars.mVars, manager->mSlot.mData.mEvtVars, sizeof(gEventVars.mVars));
	memcpy(((WiiFlagSource*)gGame.mXObjs.mPools[7].mObjects)->mFlags, manager->mSlot.mData.mCoinFlags,
	       sizeof(manager->mSlot.mData.mCoinFlags));
	memcpy(((WiiFlagSource*)gGame.mXObjs.mPools[8].mObjects)->mFlags, manager->mSlot.mData.mItemFlags,
	       sizeof(manager->mSlot.mData.mItemFlags));
	memcpy(((WiiFlagSource*)gGame.mXObjs.mPools[20].mObjects)->mFlags, manager->mSlot.mData.mDustFlags,
	       sizeof(manager->mSlot.mData.mDustFlags));
	memcpy(&gGame.GetPlayer()->mAttachments, manager->mSlot.mData.mPlayerRecord, sizeof(manager->mSlot.mData.mPlayerRecord));
	RequestSuit(gGame.GetPlayer(), manager->mSlot.mData.mSuit);
	gGame.GetPlayer()->mSuits.mSpecial  = manager->mSlot.mData.mSpecialSuit;
	gGame.GetPlayer()->mSquirterContent = manager->mSlot.mData.mSquirterLiquid;
	gGame.GetPlayer()->mItemTint        = manager->mSlot.mData.mItemTint;
}

/**
 * @note Address: 0x800AAB18
 * @note Size: 0x1D4
 */
extern "C" void RestoreSavedPlayerAndCamera(CSaveFile* manager)
{
	// the Wii player class is not reconstructed; the +0x90 slot takes the position
	// Vec, +0x70 the rotation
	gGame.GetPlayer()->mVtable->SetPosition(gGame.GetPlayer(), &manager->mSlot.mData.mPos);
	gGame.GetPlayer()->mVtable->SetRotation(gGame.GetPlayer(), manager->mSlot.mData.mRotY);
	ResetPlayerHeadOffset(gGame.GetPlayer());
	ResetPlayerPlug(gGame.GetPlayer());
	CMotionControlCamera* motion = &gGame.mMotionControlCamera;
	if (manager->mSlot.mData.mCameraAction >= 0 && manager->mSlot.mData.mCameraAction < CAMACTION_NUM) {
		motion->ChangeAction(manager->mSlot.mData.mCameraAction);
		motion->SetEye(manager->mSlot.mData.mCameraEye);
		motion->SetInterest(manager->mSlot.mData.mCameraInterest);
		motion->SetAngle(manager->mSlot.mData.mCameraAngle);
		motion->SetDistance(manager->mSlot.mData.mCameraDistance);
		motion->SetFov(manager->mSlot.mData.mCameraFov);
		CCameraMemento memento;
		memento.SetEye(manager->mSlot.mData.mCameraEye);
		memento.SetInterest(manager->mSlot.mData.mCameraInterest);
		memento.SetAngle(manager->mSlot.mData.mCameraAngle);
		memento.mDistance = manager->mSlot.mData.mCameraDistance;
		memento.mFov      = manager->mSlot.mData.mCameraFov;
		memento.mAction   = gGame.mCameraActions.mTable[manager->mSlot.mData.mCameraAction];
		SetMapCameraKeepPreviousEye(gGame.mCameraActions.mTable[CAMACTION_MAP]);
		gGame.mMotionControlCamera.SetMemento(&memento);
	}
}
