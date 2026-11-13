#include <global.h>

#include <dolphin/mtx/mtxtypes.h>
#include <dolphin/os.h>

#include <sysdolphin/cobj.h>
#include <sysdolphin/memory.h>
#include <sysdolphin/particle.h>
#include <sysdolphin/perf.h>
#include <sysdolphin/pslist.h>

void memset(void*, int, int);
void memcpy(void*, void*, int);

#define PS_NUM_LINK 16

#define GX_FIFO_U8  (*(volatile u8*)0xCC008000)
#define GX_FIFO_F32 (*(volatile f32*)0xCC008000)

// the asserts call this one activeParticle, the other two arrays sit right behind it
HSD_Particle* activeParticle[PS_NUM_LINK];
s32 sortDirty[PS_NUM_LINK];
HSD_Particle* translucentParticleHead[PS_NUM_LINK];
// free list of particle structs
HSD_Particle* freeParticle;

// live and peak particle counts, owned by particle.c
extern u16 lbl_8065BA8E;
extern u16 lbl_8065BA88;

char lbl_8025B490[] = "pslist.c";

#undef assert_line
#define assert_line(line, cond) ((cond) ? ((void)0) : __assert(lbl_8025B490, line, #cond))

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
		pp = HSD_Alloc(0x9C);
		memset(pp, 0, 0x9C);
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
	lbl_8065BA8E++;
	if (lbl_8065BA8E > lbl_8065BA88) {
		lbl_8065BA88 = lbl_8065BA8E;
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
		assert_line(0x88, activeParticle[pp->linkNo] == pp);
		activeParticle[pp->linkNo] = pp->next;
	} else {
		assert_line(0x8B, parent->next == pp);
		parent->next = pp->next;
	}

	pp->next     = freeParticle;
	freeParticle = pp;
	lbl_8065BA8E--;
}

/**
 * @note Address: 0x801E5D54
 * @note Size: 0x6C
 */
HSD_Particle* _psListGetFirst(s32 linkNo)
{
	assert_line(0x98, linkNo >= 0 && linkNo < PS_NUM_LINK);
	return activeParticle[linkNo];
}

typedef struct PSParticleBucket {
	HSD_Particle* head;
	HSD_Particle* tail;
} PSParticleBucket;

/**
 * @note Address: N/A
 * @note Size: 0x338
 */
HSD_Particle* particleSort(s32 linkNo, HSD_Particle** opaqueHead, HSD_Particle** translucentHead)
{
	HSD_Particle* p;
	HSD_Particle* next;
	HSD_Particle* head0;
	HSD_Particle* head1;
	HSD_Particle* tail0;
	HSD_Particle* tail1;
	HSD_Particle* result;
	PSParticleBucket buckets[16];
	s32 bucket;
	s32 i;

	p = activeParticle[linkNo];

	if (!sortDirty[linkNo]) {
		if (p == NULL) {
			*opaqueHead      = NULL;
			*translucentHead = NULL;
			return NULL;
		}
		if (p->kind & 8) {
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

	bucket               = ((p->kind >> 0x19) & 7) + ((p->kind & 8) == 0) * 8;
	buckets[bucket].head = p;
	next                 = p->next;

	while (next != NULL) {
		if ((p->kind ^ next->kind) & 0x0E000008) {
			buckets[bucket].tail = p;
			bucket               = ((next->kind >> 0x19) & 7) + ((next->kind & 8) == 0) * 8;
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

	tail0 = NULL;
	head0 = NULL;
	tail1 = NULL;
	head1 = NULL;
	for (i = 0; i < 8; i++) {
		if (buckets[i].head != NULL) {
			if (head0 == NULL) {
				head0 = buckets[i].head;
			} else {
				tail0->next = buckets[i].head;
			}
			tail0 = buckets[i].tail;
		}
	}

	for (i = 8; i < 16; i++) {
		if (buckets[i].head != NULL) {
			if (head1 == NULL) {
				head1 = buckets[i].head;
			} else {
				tail1->next = buckets[i].head;
			}
			tail1 = buckets[i].tail;
		}
	}

	result = NULL;
	if (tail0 != NULL) {
		result      = head0;
		tail0->next = head1;
	}
	if (tail1 != NULL) {
		if (result == NULL) {
			result = head1;
		}
		tail1->next = NULL;
	}

	activeParticle[linkNo]          = result;
	translucentParticleHead[linkNo] = head1;
	*opaqueHead                     = head0;
	*translucentHead                = head1;
	return result;
}
