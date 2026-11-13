#ifndef _GAME_CPARTICLE_H
#define _GAME_CPARTICLE_H

extern "C" {
#include <sysdolphin/particle.h>
}
#include "game/cfile.h"

// HSD particle wrapper. Class names from RTTI, Read from a halt string. CGame owns the
// CParticle at +0x9114 and emits both classes' vtables and RTTI.

extern "C" {
void psKillGenerator(HSD_Generator* gen);
void psKillAllGenerator(void);
// hsd generator.c, the jobj particle and sound link callbacks
HSD_Generator* psCreateJointGenerator(int linkNo, int bank, int kind, HSD_JObj* jobj);
HSD_Generator* psCreateQueuedJointGenerator(int linkNo, int bank, int kind, HSD_JObj* jobj);
}

// one generator the game keeps track of, by the id it was made from
class CGenerator {
public:
	CGenerator()
	{
		mId  = -1;
		mGen = NULL;
	}

	s32 mId;             // _00
	HSD_Generator* mGen; // _04
	// _08 vtable

	virtual ~CGenerator() { } // _08

	void Kill()
	{
		if (mGen != NULL) {
			psKillGenerator(mGen);
			mId  = -1;
			mGen = NULL;
		}
	}
};
typedef char CGenerator_size_check[sizeof(CGenerator) == 0xC ? 1 : -1];

#define CPARTICLE_FILE_NUM 4
#define CPARTICLE_GEN_NUM  32

// reads one particle bank, ptl txg ref txf, and keeps the generators made from it
class CParticle {
public:
	s32 mReadStep;                             // _00, how many of the four files are in
	CFile mFile[CPARTICLE_FILE_NUM];           // _04
	s32* mCmdBank;                             // _3B4, the ptl file
	s32* mTexBank;                             // _3B8, txg
	void* mRef;                                // _3BC, ref
	s32* mFormBank;                            // _3C0, txf
	CGenerator mGenerators[CPARTICLE_GEN_NUM]; // _3C4
	// _544 vtable

	virtual ~CParticle() { } // _08

	void KillAll();
	void Kill(HSD_Generator* gen);
	void Kill(s32 id);
	HSD_Generator* Create(u32 code, u8 type, HSD_JObj* jobj, BOOL keep);
	HSD_Generator* CreateSRT(u32 code, s32 status, const Vec& translate, const Vec& scale, const Vec& rotate, BOOL keep);
	void Draw(s32 mask);
	void Update(u32 frames);
	BOOL Read(const char* name);
	void Init();

	// the ptl file starts with its version and bank number, then the first id in it
	s32 GetVersion() { return mCmdBank != NULL ? ((u16*)mCmdBank)[0] : 0; }
	s32 GetBank() { return mCmdBank != NULL ? ((u16*)mCmdBank)[1] : 0; }
	s32 GetIdBase() { return mCmdBank != NULL ? mCmdBank[1] : 0; }
};
typedef char CParticle_size_check[sizeof(CParticle) == 0x548 ? 1 : -1];

#endif
