#include "game/cparticle.h"

/**
 * @note Address: 0x8001364C
 * @note Size: 0x34
 */
// the particle jobj link callback, kind 0x48 on link 0 goes to link 1
extern "C" HSD_Generator* LinkJointParticleGenerator(int linkNo, int bank, int kind, HSD_JObj* jobj)
{
	if (linkNo == 0 && kind == 0x48)
		linkNo = 1;
	return psCreateJointGenerator(linkNo, bank, kind, jobj);
}

/**
 * @note Address: 0x80013680
 * @note Size: 0x34
 */
// the same for the sound link
extern "C" HSD_Generator* LinkJointSoundGenerator(int linkNo, int bank, int kind, HSD_JObj* jobj)
{
	if (linkNo == 0 && kind == 0x48)
		linkNo = 1;
	return psCreateQueuedJointGenerator(linkNo, bank, kind, jobj);
}
