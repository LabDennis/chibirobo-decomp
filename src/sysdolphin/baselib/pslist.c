#include <global.h>

#include <dolphin/mtx/mtxtypes.h>
#include <dolphin/os.h>

#include <sysdolphin/cobj.h>
#include <sysdolphin/memory.h>
#include <sysdolphin/particle.h>
#include <sysdolphin/particle_internal.h>
#include <sysdolphin/perf.h>
#include <sysdolphin/pslist.h>
#include <string.h>

#define PS_NUM_LINK 16

HSD_Particle* activeParticle[PS_NUM_LINK];
s32 sortDirty[PS_NUM_LINK];
HSD_Particle* translucentParticleHead[PS_NUM_LINK];
static HSD_Particle* freeParticle;

#define PS_LIST_ASSERT(line, condition) ((condition) ? ((void)0) : __assert("pslist.c", line, #condition))

/**
 * @note Address: 0x801E5A38
 * @note Size: 0x10C
 */
s32 _psLinkInit(s32 count)
{
	int i;
	HSD_Particle* pp;

	for (i = 0; i < PS_NUM_LINK; i++) {
		activeParticle[i]          = NULL;
		sortDirty[i]               = 0;
		translucentParticleHead[i] = NULL;
	}
	freeParticle = NULL;

	for (i = count - 1; i >= 0; i--) {
		pp = HSD_Alloc(sizeof(*pp));
		memset(pp, 0, sizeof(*pp));
		if (pp == NULL) {
			return -1;
		}
		pp->next     = freeParticle;
		freeParticle = pp;
	}
	return i;
}

/**
 * @note Address: 0x801E5B44
 * @note Size: 0xA8
 */
void _psListClear(void)
{
	HSD_Particle* pp;
	HSD_Particle* next;
	s32 i;

	for (pp = freeParticle; pp != NULL; pp = next) {
		next = pp->next;
		HSD_Free(pp);
	}
	freeParticle = NULL;

	for (i = 0; i < PS_NUM_LINK; i++) {
		for (pp = activeParticle[i]; pp != NULL; pp = next) {
			next = pp->next;
			HSD_Free(pp);
		}
		activeParticle[i]          = NULL;
		sortDirty[i]               = 0;
		translucentParticleHead[i] = NULL;
	}
}

/**
 * @note Address: 0x801E5BEC
 * @note Size: 0x8C
 */
HSD_Particle* _psListNew(HSD_Particle* parent, s32 linkNo)
{
	HSD_Particle* pp = freeParticle;

	if (pp == NULL) {
		return NULL;
	}
	psParticleCount++;
	if (psParticleCount > psPeakParticleCount) {
		psPeakParticleCount = psParticleCount;
	}
	freeParticle = pp->next;
	if (parent == NULL) {
		pp->next               = activeParticle[linkNo];
		activeParticle[linkNo] = pp;
	} else {
		pp->next     = parent->next;
		parent->next = pp;
	}
	sortDirty[linkNo] = 1;
	return pp;
}

/**
 * @note Address: 0x801E5C78
 * @note Size: 0xDC
 */
void _psListDelete(HSD_Particle* pp, HSD_Particle* parent)
{
	sortDirty[pp->linkNo] = 1;

	if (parent == NULL) {
		PS_LIST_ASSERT(0x88, activeParticle[pp->linkNo] == pp);
		activeParticle[pp->linkNo] = pp->next;
	} else {
		PS_LIST_ASSERT(0x8B, parent->next == pp);
		parent->next = pp->next;
	}

	pp->next     = freeParticle;
	freeParticle = pp;
	psParticleCount--;
}

/**
 * @note Address: 0x801E5D54
 * @note Size: 0x6C
 */
HSD_Particle* _psListGetFirst(s32 linkNo)
{
	PS_LIST_ASSERT(0x98, linkNo >= 0 && linkNo < PS_NUM_LINK);
	return activeParticle[linkNo];
}

typedef struct PSParticleBucket {
	HSD_Particle* head;
	HSD_Particle* tail;
} PSParticleBucket;

enum { PS_PARTICLE_BUCKET_COUNT = 2 * PS_PARTICLE_SORT_ORDER_COUNT };

/**
 * @note Address: N/A
 * @note Size: 0x338
 */
HSD_Particle* particleSort(s32 linkNo, HSD_Particle** opaqueHead, HSD_Particle** translucentHead)
{
	HSD_Particle* p;
	HSD_Particle* next;
	HSD_Particle* opaqueFirst;
	HSD_Particle* translucentFirst;
	HSD_Particle* opaqueLast;
	HSD_Particle* translucentLast;
	HSD_Particle* result;
	PSParticleBucket buckets[PS_PARTICLE_BUCKET_COUNT];
	s32 bucket;
	s32 i;

	p = activeParticle[linkNo];

	if (!sortDirty[linkNo]) {
		if (p == NULL) {
			*opaqueHead      = NULL;
			*translucentHead = NULL;
			return NULL;
		}
		if (p->kind & PS_PARTICLE_OPAQUE) {
			*opaqueHead = p;
		} else {
			*opaqueHead = NULL;
		}
		*translucentHead = translucentParticleHead[linkNo];
		return p;
	}

	sortDirty[linkNo] = 0;

	if (p == NULL) {
		translucentParticleHead[linkNo] = NULL;
		*opaqueHead                     = NULL;
		*translucentHead                = NULL;
		return NULL;
	}

	memset(buckets, 0, sizeof(buckets));

	bucket = ((p->kind >> PS_PARTICLE_SORT_ORDER_SHIFT) & (PS_PARTICLE_SORT_ORDER_COUNT - 1))
	         + ((p->kind & PS_PARTICLE_OPAQUE) ? 0 : 1) * PS_PARTICLE_SORT_ORDER_COUNT;
	buckets[bucket].head = p;
	next = p->next;

	while (next != NULL) {
		if ((p->kind ^ next->kind) & (PS_PARTICLE_SORT_ORDER_MASK | PS_PARTICLE_OPAQUE)) {
			buckets[bucket].tail = p;
			bucket = ((next->kind >> PS_PARTICLE_SORT_ORDER_SHIFT) & (PS_PARTICLE_SORT_ORDER_COUNT - 1))
			         + ((next->kind & PS_PARTICLE_OPAQUE) ? 0 : 1) * PS_PARTICLE_SORT_ORDER_COUNT;
			if (buckets[bucket].head == NULL) {
				buckets[bucket].head = next;
			} else {
				buckets[bucket].tail->next = next;
			}
		}
		p    = next;
		next = next->next;
	}
	buckets[bucket].tail = p;

	opaqueLast       = NULL;
	opaqueFirst      = NULL;
	translucentLast  = NULL;
	translucentFirst = NULL;
	for (i = 0; i < PS_PARTICLE_SORT_ORDER_COUNT; i++) {
		if (buckets[i].head != NULL) {
			if (opaqueFirst == NULL) {
				opaqueFirst = buckets[i].head;
			} else {
				opaqueLast->next = buckets[i].head;
			}
			opaqueLast = buckets[i].tail;
		}
	}

	for (i = PS_PARTICLE_SORT_ORDER_COUNT; i < PS_PARTICLE_BUCKET_COUNT; i++) {
		if (buckets[i].head != NULL) {
			if (translucentFirst == NULL) {
				translucentFirst = buckets[i].head;
			} else {
				translucentLast->next = buckets[i].head;
			}
			translucentLast = buckets[i].tail;
		}
	}

	result = NULL;
	if (opaqueLast != NULL) {
		result           = opaqueFirst;
		opaqueLast->next = translucentFirst;
	}
	if (translucentLast != NULL) {
		if (result == NULL) {
			result = translucentFirst;
		}
		translucentLast->next = NULL;
	}

	activeParticle[linkNo]         = result;
	translucentParticleHead[linkNo] = translucentFirst;
	*opaqueHead                    = opaqueFirst;
	*translucentHead               = translucentFirst;
	return result;
}
