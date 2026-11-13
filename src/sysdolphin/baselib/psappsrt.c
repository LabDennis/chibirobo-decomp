#include <global.h>
#include <string.h>
#include <sysdolphin/memory.h>
#include <sysdolphin/particle.h>

extern u16 lbl_8065BA84;
extern u16 lbl_8065BA8A;
HSD_psAppSRT* freeAppSRT;

s32 psRemoveParticleAppSRT(HSD_Particle* pp);
s32 psRemoveGeneratorAppSRT(HSD_Generator* gp);

/**
 * @note Address: 0x801E5614
 * @note Size: 0x9C
 */
s32 psInitAppSRT(s32 num, s32 size)
{
	HSD_psAppSRT* srt;
	s32 i;

	lbl_8065BA8A = 0;
	lbl_8065BA84 = 0;
	freeAppSRT   = NULL;
	for (i = 0; i < num; i++) {
		srt = HSD_Alloc(size);
		if (srt == NULL) {
			return i;
		}
		memset(srt, 0, size);
		srt->next  = freeAppSRT;
		freeAppSRT = srt;
	}
	return i;
}

/**
 * @note Address: 0x801E56B0
 * @note Size: 0x48
 */
void psRemoveAppSRT(void)
{
	HSD_psAppSRT* srt;
	HSD_psAppSRT* next;

	for (srt = freeAppSRT; srt != NULL; srt = next) {
		next = srt->next;
		HSD_Free(srt);
	}
	freeAppSRT = NULL;
}

/**
 * @note Address: 0x801E56F8
 * @note Size: 0x98
 */
static inline HSD_psAppSRT* psNewAppSRT(s8 status, u16 idnum)
{
	HSD_psAppSRT* srt;

	srt = freeAppSRT;
	if (srt != NULL) {
		freeAppSRT       = srt->next;
		srt->next        = NULL;
		srt->usedCount   = 1;
		srt->frameNum    = 0;
		srt->status      = status;
		srt->translate.z = 0.0F;
		srt->translate.y = 0.0F;
		srt->translate.x = 0.0F;
		srt->rot.z       = 0.0F;
		srt->rot.y       = 0.0F;
		srt->rot.x       = 0.0F;
		srt->scale.z     = 1.0F;
		srt->scale.y     = 1.0F;
		srt->scale.x     = 1.0F;
		srt->freefunc    = NULL;
		srt->xA2         = 0;
		srt->gp          = NULL;
		srt->idnum       = idnum;
		lbl_8065BA8A++;
		if (lbl_8065BA8A > lbl_8065BA84) {
			lbl_8065BA84 = lbl_8065BA8A;
		}
	}
	return srt;
}

HSD_psAppSRT* CreateGeneratorAppSRT(HSD_Generator* gp, s32 status)
{
	return gp->appsrt = psNewAppSRT(status, gp->idnum);
}

/**
 * @note Address: 0x801E5790
 * @note Size: 0x3C
 */
s32 psAttachParticleAppSRT(HSD_Particle* pp, HSD_psAppSRT* srt)
{
	if (srt == NULL || pp == NULL || pp->appsrt != NULL) {
		return -1;
	}
	pp->appsrt = srt;
	return ++srt->usedCount;
}

/**
 * @note Address: 0x801E57CC
 * @note Size: 0x3C
 */
s32 psAttachGeneratorAppSRT(HSD_Generator* gp, HSD_psAppSRT* srt)
{
	if (srt == NULL || gp == NULL || gp->appsrt != NULL) {
		return -1;
	}
	gp->appsrt = srt;
	return ++srt->usedCount;
}

/**
 * @note Address: 0x801E5808
 * @note Size: 0x6C
 */
s32 AttachParticleAppSRT(HSD_Particle* pp, HSD_psAppSRT* srt)
{
	if (srt == NULL || pp == NULL) {
		return -1;
	}
	if (pp->appsrt != NULL) {
		psRemoveParticleAppSRT(pp);
	}
	pp->appsrt = srt;
	return ++srt->usedCount;
}

/**
 * @note Address: 0x801E5874
 * @note Size: 0x6C
 */
s32 AttachGeneratorAppSRT(HSD_Generator* gp, HSD_psAppSRT* srt)
{
	if (srt == NULL || gp == NULL) {
		return -1;
	}
	if (gp->appsrt != NULL) {
		psRemoveGeneratorAppSRT(gp);
	}
	gp->appsrt = srt;
	return ++srt->usedCount;
}

/**
 * @note Address: 0x801E58E0
 * @note Size: 0xA0
 */
s32 psRemoveParticleAppSRT(HSD_Particle* pp)
{
	HSD_psAppSRT* srt;
	u16 count;

	srt = pp->appsrt;
	if (srt == NULL) {
		return -1;
	}
	count          = srt->usedCount - 1;
	srt->usedCount = count;
	if (count == 0) {
		srt = pp->appsrt;
		if (srt->freefunc != NULL) {
			srt->freefunc(srt);
		}
		srt->next  = freeAppSRT;
		freeAppSRT = srt;
		lbl_8065BA8A--;
	}
	pp->appsrt = NULL;
	return count;
}

/**
 * @note Address: 0x801E5980
 * @note Size: 0xB8
 */
s32 psRemoveGeneratorAppSRT(HSD_Generator* gp)
{
	HSD_psAppSRT* srt;
	u16 count;

	srt = gp->appsrt;
	if (srt == NULL) {
		return -1;
	}
	if (srt->gp == gp) {
		srt->gp = NULL;
	}
	srt            = gp->appsrt;
	count          = srt->usedCount - 1;
	srt->usedCount = count;
	if (count == 0) {
		srt = gp->appsrt;
		if (srt->freefunc != NULL) {
			srt->freefunc(srt);
		}
		srt->next  = freeAppSRT;
		freeAppSRT = srt;
		lbl_8065BA8A--;
	}
	gp->appsrt = NULL;
	return count;
}
