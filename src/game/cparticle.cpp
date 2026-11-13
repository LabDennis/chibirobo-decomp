#include "game/cparticle.h"
#include <sysdolphin/psdisp.h>
#include <stdio.h>

extern "C" {
#include <dolphin/os.h>
void OSReport(const char* msg, ...);
HSD_Generator* psCreateGeneratorID(int linkNo, int bank, int kind);
HSD_psAppSRT* CreateGeneratorAppSRT(HSD_Generator* gen, s32 status);
void psInterpretParticles(u32 flags);
void psExecGenerator(u32 mask);
void psInitDataBank(s32 bank, s32* cmdBank, s32* texBank, void* ref, s32* formBank);
void psInitDataBankLoad(s32 bank, s32* cmdBank, s32* texBank, void* ref, s32* formBank);
void psInitParticle(s32 count);
void psRemoveParticle(void);
void psRemoveGenerator(void);
void psRemoveAppSRT(void);
void psSetPointJObjNodup(HSD_JObj* jobj, s32 no);
void HSD_JObjSetSPtclCallback(HSD_Generator* (*func)(int, int, int, HSD_JObj*));
void HSD_JObjSetDPtclCallback(HSD_Generator* (*func)(int, int, int, HSD_JObj*));
void HSD_JObjSetPtclTargetCallback(void (*func)(HSD_JObj*, s32));
}

// the ptcl file version this code reads
#define PTCL_VERSION 0x43

/**
 * @note Address: 0x801A8924
 * @note Size: 0x7C
 */
void CParticle::KillAll()
{
	for (int i = 0; i < CPARTICLE_GEN_NUM; i++) {
		if (mGenerators[i].mGen != NULL) {
			mGenerators[i].Kill();
		}
	}
	psKillAllGenerator();
}

/**
 * @note Address: 0x801A89A0
 * @note Size: 0x70
 */
void CParticle::Kill(HSD_Generator* gen)
{
	for (int i = 0; i < CPARTICLE_GEN_NUM; i++) {
		if (mGenerators[i].mGen != NULL && mGenerators[i].mGen == gen) {
			mGenerators[i].Kill();
		}
	}
}

/**
 * @note Address: 0x801A8A10
 * @note Size: 0x74
 */
void CParticle::Kill(s32 id)
{
	for (int i = 0; i < CPARTICLE_GEN_NUM; i++) {
		if (mGenerators[i].mGen != NULL && mGenerators[i].mId == id) {
			mGenerators[i].Kill();
		}
	}
}

/**
 * @note Address: 0x801A8A84
 * @note Size: 0x12C
 */
HSD_Generator* CParticle::Create(u32 code, u8 type, HSD_JObj* jobj, BOOL keep)
{
	HSD_Generator* gen = NULL;
	for (int i = 0; i < CPARTICLE_GEN_NUM; i++) {
		if (mGenerators[i].mGen == NULL) {
			s32 linkNo = (code >> 16) & 0xFF;
#if defined(VERSION_JP)
			s32 no   = code & 0xFFFF;
			s32 bank = GetBank();
			s32 id   = no + GetIdBase();
#else
			s32 id = code & 0xFFFF;
			if (id < GetIdBase()) {
				id += GetIdBase();
			}
			s32 bank = GetBank();
#endif
			mGenerators[i].mId = id;
			if (type == 0) {
				mGenerators[i].mGen = psCreateJointGenerator(linkNo, bank, id, jobj);
			} else {
				mGenerators[i].mGen = psCreateQueuedJointGenerator(linkNo, bank, id, jobj);
			}
			gen = mGenerators[i].mGen;
			if (gen != NULL && !keep && gen->genLife != 0) {
				mGenerators[i].mId  = -1;
				mGenerators[i].mGen = NULL;
			}
			break;
		}
	}
	return gen;
}

/**
 * @note Address: 0x801A8BB0
 * @note Size: 0x16C
 */
HSD_Generator* CParticle::CreateSRT(u32 code, s32 status, const Vec& translate, const Vec& scale, const Vec& rotate, BOOL keep)
{
	HSD_Generator* gen = NULL;
	for (int i = 0; i < CPARTICLE_GEN_NUM; i++) {
		if (mGenerators[i].mGen == NULL) {
			s32 linkNo = (code >> 16) & 0xFF;
#if defined(VERSION_JP)
			s32 no   = code & 0xFFFF;
			s32 bank = GetBank();
			s32 id   = no + GetIdBase();
#else
			s32 id = code & 0xFFFF;
			if (id < GetIdBase()) {
				id += GetIdBase();
			}
			s32 bank = GetBank();
#endif
			mGenerators[i].mId  = id;
			mGenerators[i].mGen = psCreateGeneratorID(linkNo, bank, id);
			if (mGenerators[i].mGen != NULL) {
				HSD_psAppSRT* srt = CreateGeneratorAppSRT(mGenerators[i].mGen, status);
				if (srt != NULL) {
					srt->scale     = scale;
					srt->translate = translate;
					srt->rot       = rotate;
				}
			}
			gen = mGenerators[i].mGen;
			if (gen != NULL && !keep && gen->genLife != 0) {
				mGenerators[i].mId  = -1;
				mGenerators[i].mGen = NULL;
			}
			break;
		}
	}
	return gen;
}

/**
 * @note Address: 0x801A8D1C
 * @note Size: 0x4C
 */
void CParticle::Draw(s32 mask)
{
	for (u32 i = 0; i < 3; i++) {
		_psDispParticlesSub(mask, i);
	}
}

/**
 * @note Address: 0x801A8D68
 * @note Size: 0x54
 */
void CParticle::Update(u32 frames)
{
	for (u32 i = 0; i < frames; i++) {
		psInterpretParticles(0);
		psExecGenerator(0);
	}
}

/**
 * @note Address: 0x801A8DBC
 * @note Size: 0x1C8
 */
BOOL CParticle::Read(const char* name)
{
	const char* exts[CPARTICLE_FILE_NUM] = { "ptl", "txg", "ref", "txf" };
	void** banks[CPARTICLE_FILE_NUM]     = { (void**)&mCmdBank, (void**)&mTexBank, &mRef, (void**)&mFormBank };
	char path[64];

	if (mReadStep < CPARTICLE_FILE_NUM) {
		sprintf(path, "%s.%s", name, exts[mReadStep]);
		if (mFile[mReadStep].ReadEx(path, 0, 0)) {
			CFile* file       = &mFile[mReadStep];
			*banks[mReadStep] = file->mBuffer;
			mReadStep++;
		}
	}
	if (mReadStep >= CPARTICLE_FILE_NUM) {
		if (GetVersion() != PTCL_VERSION) {
			OSReport("\n!!!HALT!!!\n");
			OSPanic(__FILE__, 162, "%s%s", "unmatch versions between system(%d) and ptcl-file(%d)", PTCL_VERSION, GetVersion(),
			        __FUNCTION__);
		}
		psInitDataBank(GetBank(), mCmdBank, mTexBank, mRef, mFormBank);
		return TRUE;
	}
	return FALSE;
}

/**
 * @note Address: 0x801A8F84
 * @note Size: 0xCC
 */
void CParticle::Init()
{
	mReadStep = 0;
	mCmdBank  = NULL;
	mTexBank  = NULL;
	mRef      = NULL;
	mFormBank = NULL;
	if (mReadStep > 0) {
		psRemoveAppSRT();
		psRemoveGenerator();
		psRemoveParticle();
	}
	psInitParticle(800);
	psInitGenerator(100);
	psInitAppSRT(100, sizeof(HSD_psAppSRT));
	HSD_JObjSetSPtclCallback(psCreateQueuedJointGenerator);
	HSD_JObjSetDPtclCallback(psCreateJointGenerator);
	HSD_JObjSetPtclTargetCallback(psSetPointJObjNodup);
	if (mReadStep > 0) {
		psInitDataBankLoad(GetBank(), mCmdBank, mTexBank, mRef, mFormBank);
	}
}
