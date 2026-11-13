#include <global.h>

#include <dolphin/os.h>
#include <sysdolphin/jobj.h>
#include <sysdolphin/object.h>
#include <sysdolphin/particle.h>
#include <sysdolphin/pslist.h>

void psClearPointJObj(void);
void psInitDataBankLocate(s32* cmdBank, s32* texBank, s32* formBank);

extern s32 psRemoveParticleAppSRT(HSD_Particle* pp);
extern s32 psAttachParticleAppSRT(HSD_Particle* pp, HSD_psAppSRT* srt);
extern s32 _psLinkInit(s32 count);
extern void _psListClear(void);
extern HSD_Particle* _psListNew(HSD_Particle* parent, s32 linkNo);
extern u16 psGetNewIDNum(void);
extern void psKillAllGenerator(void);

static inline void particle_ref_INC(void* o)
{
	if (o != NULL) {
		HSD_OBJ(o)->ref_count++;
		((HSD_OBJ(o)->ref_count != HSD_OBJ_NOREF)) ? ((void)0) : __assert("object.h", 93, "HSD_OBJ(o)->ref_count != HSD_OBJ_NOREF");
	}
}

// per bank data tables, all addressed off one base in the target
void* ptclref[64];
s32 psBankTextureCounts[64];
HSD_PSTexGroup** psTexGroupArray[64];
HSD_PSFormGroup** psFormGroupArray[64];
s32 psNumCmdList[64];
HSD_PSCmdList** psCmdListArray[64];
HSD_JObj* psPointJObj[8];

#define BANK_REF(b)       (ptclref[b])
#define BANK_TEXCOUNT(b)  (psBankTextureCounts[b])
#define BANK_TEXARRAY(b)  (psTexGroupArray[b])
#define BANK_FORMARRAY(b) (psFormGroupArray[b])
#define BANK_CMDCOUNT(b)  (psNumCmdList[b])
#define BANK_CMDARRAY(b)  (psCmdListArray[b])

// peak and live counts for appsrts, generators and particles, the 2005 game
// keeps all six in particle.c, .sbss comes out last declaration first
u16 lbl_8065BA8E; // live particles
u16 lbl_8065BA8C; // live generators
u16 lbl_8065BA8A; // live appsrts
u16 lbl_8065BA88; // peak particles
u16 lbl_8065BA86; // peak generators
u16 lbl_8065BA84; // peak appsrts
// user callback table psInterpretParticle0 picks pp->callback from
int (**lbl_8065BA80)(HSD_Particle*);

/**
 * @note Address: N/A
 * @note Size: 0x140
 */
void psInitDataBankLoad(s32 bank, s32* cmdBank, s32* texBank, void* ref, s32* formBank)
{
	if (formBank != NULL && *formBank != *texBank) {
		OSPanic("particle.c", 95, "illigal form data (strange number of group)\n");
	}

	ptclref[bank]             = ref;
	psBankTextureCounts[bank] = *texBank;
	psTexGroupArray[bank]     = (HSD_PSTexGroup**)(texBank + 1);
	if (formBank != NULL) {
		psFormGroupArray[bank] = (HSD_PSFormGroup**)(formBank + 1);
	} else {
		psFormGroupArray[bank] = NULL;
	}

	switch (*(u16*)cmdBank) {
	case 0:
		psNumCmdList[bank]   = cmdBank[1];
		psCmdListArray[bank] = (HSD_PSCmdList**)(cmdBank + 2);
		break;
	case 0x40:
	case 0x41:
	case 0x42:
	case 0x43: {
		s32 count            = cmdBank[1];
		psNumCmdList[bank]   = cmdBank[2] + count;
		psCmdListArray[bank] = (HSD_PSCmdList**)(cmdBank + 3 - count);
		break;
	}
	default:
		OSPanic("particle.c", 125, "psInitDataBanks: unknown version\n");
	}
}

/**
 * @note Address: N/A
 * @note Size: 0x374
 */
void psInitDataBankLocate(s32* cmdBank, s32* texBank, s32* formBank)
{
	s32 i;
	s32 num2;
	s32* group;
	s32* ptr;
	s32 num;
	s32* base;
	s32 version;
	s32 j;
	s32* groups;

	version = *(u16*)cmdBank;
	switch (version) {
	case 0:
		num2 = cmdBank[1];
		base = cmdBank + 2;
		num  = 0;
		for (i = 0; i < num2; i++) {
			cmdBank[i + 2] += (s32)cmdBank;
		}
		break;
	case 0x40:
	case 0x41:
	case 0x42:
	case 0x43:
		num  = cmdBank[1];
		num2 = cmdBank[2] + num;
		base = cmdBank + 3 - num;
		ptr  = cmdBank;
		j    = 0;
		while (j < cmdBank[2]) {
			if (ptr[3] != 0) {
				ptr[3] += (s32)cmdBank;
			}
			ptr++;
			j++;
		}
		break;
	}

	ptr = base + num;
	for (i = num; i < num2; i++) {
		s32* cmd = (s32*)ptr[0];
		if (cmd != NULL) {
			cmd[2] = cmd[2] & 0xF1FFFFFF;
			cmd    = (s32*)ptr[0];
			cmd[2] = cmd[2] | 0x08000000;
		}
		ptr++;
	}

	{
		s32 num_groups = texBank[0];
		s32 k;

		group = groups = texBank + 1;
		for (k = 1; k <= num_groups; k++) {
			if (group[0] != 0) {
				group[0] += (s32)texBank;
			}
			group++;
		}

		for (k = 0; k < num_groups; k++) {
			HSD_PSTexGroup* tg = (HSD_PSTexGroup*)groups[k];
			if (tg == NULL) {
				continue;
			}
			{
				s32 ti;
				for (ti = 0; (u32)ti < ((HSD_PSTexGroup*)groups[k])->num; ti++) {
					if (((HSD_PSTexGroup*)groups[k])->texTable[ti] != NULL) {
						((HSD_PSTexGroup*)groups[k])->texTable[ti] += (u32)texBank;
					}
				}
				tg = (HSD_PSTexGroup*)groups[k];
			}
			{
				u32 fmt = tg->fmt;
				if (fmt != 8 && fmt != 9 && fmt != 10) {
					continue;
				}
			}
			if (tg->palflag & 1) {
				i = tg->num;
				if (tg->texTable[i] != NULL) {
					tg->texTable[i] += (u32)texBank;
				}
			} else if (tg->palnum != 0) {
				i = tg->num;
				for (; (u32)i < ((HSD_PSTexGroup*)groups[k])->num + ((HSD_PSTexGroup*)groups[k])->palnum; i++) {
					if (((HSD_PSTexGroup*)groups[k])->texTable[i] != NULL) {
						((HSD_PSTexGroup*)groups[k])->texTable[i] += (u32)texBank;
					}
				}
			} else {
				i = tg->num;
				for (; (u32)i < ((HSD_PSTexGroup*)groups[k])->num * 2; i++) {
					if (((HSD_PSTexGroup*)groups[k])->texTable[i] != NULL) {
						((HSD_PSTexGroup*)groups[k])->texTable[i] += (u32)texBank;
					}
				}
			}
		}

		if (formBank == NULL) {
			return;
		}
		for (i = 1; i <= num_groups; i++) {
			if (formBank[i] != 0) {
				s32 fi;
				u32* fg;
				formBank[i] += (s32)formBank;
				fg = (u32*)formBank[i];
				for (fi = 0; (u32)fi < fg[0]; fi++) {
					if (fg[1 + fi] != 0) {
						fg[1 + fi] += (u32)formBank;
					}
				}
			}
		}
	}
}

/**
 * @note Address: N/A
 * @note Size: 0x15C
 */
void psInitDataBank(s32 bank, s32* cmdBank, s32* texBank, void* ref, s32* formBank)
{
	if (bank < 0x40) {
		psInitDataBankLocate(cmdBank, texBank, formBank);
		psInitDataBankLoad(bank, cmdBank, texBank, ref, formBank);
	}
}

/**
 * @note Address: N/A
 * @note Size: 0x164
 */
void psInitParticle(s32 count)
{
	int i;

	_psLinkInit(count);
	lbl_8065BA8E = 0;
	lbl_8065BA88 = 0;
	for (i = 0; i < 64; i++) {
		psNumCmdList[i]        = 0;
		psBankTextureCounts[i] = 0;
		psCmdListArray[i]      = NULL;
		psTexGroupArray[i]     = NULL;
		psFormGroupArray[i]    = NULL;
		ptclref[i]             = NULL;
	}
	lbl_8065BA80 = NULL;
	for (i = 0; i < 8; i++) {
		psPointJObj[i] = NULL;
	}
}

/**
 * @note Address: N/A
 * @note Size: 0x13C
 */
void psRemoveParticle(void)
{
	int i;

	psKillAllParticle();
	psKillAllGenerator();
	_psListClear();
	for (i = 0; i < 64; i++) {
		psNumCmdList[i]        = 0;
		psBankTextureCounts[i] = 0;
		psCmdListArray[i]      = NULL;
		psTexGroupArray[i]     = NULL;
		psFormGroupArray[i]    = NULL;
		ptclref[i]             = NULL;
	}
	psClearPointJObj();
}

/**
 * @note Address: N/A
 * @note Size: 0x2FC
 */
HSD_Particle* psGenerateParticle0(HSD_Particle* parent, int linkNo, int bank, u64 kind, int texGroup, u8* list, int life,
                                  f32 x, f32 y, f32 z, f32 vx, f32 vy, f32 vz, f32 size, f32 grav, f32 fric,
                                  int palflag, HSD_Generator* gp, int flgInterpret)
{
	HSD_Particle* pp;

	pp = _psListNew(parent, linkNo);
	if (pp == NULL) {
		return NULL;
	}

	if (gp != NULL) {
		pp->idnum = gp->idnum;
	} else {
		pp->idnum = psGetNewIDNum();
	}

	pp->appsrt = NULL;
	if (gp != NULL && gp->appsrt != NULL) {
		psAttachParticleAppSRT(pp, gp->appsrt);
	}

	pp->bank       = (u8)bank;
	pp->linkNo     = (u8)linkNo;
	pp->kind       = (u32)kind;
	pp->texGroup   = (u8)texGroup;
	pp->pos.x      = x;
	pp->pos.y      = y;
	pp->pos.z      = z;
	pp->vel.x      = vx;
	pp->vel.y      = vy;
	pp->vel.z      = vz;
	pp->size       = size;
	pp->grav       = grav;
	pp->fric       = fric;
	pp->life       = (u16)(life + 1);
	pp->cmdList    = list;
	pp->cmdMarkPtr = 0;
	pp->cmdPtr     = 0;

	if (palflag != 0) {
		pp->kind = pp->kind | 0x10;
	}

	pp->cmdWait       = (u16)(list != NULL);
	pp->poseNum       = 0;
	pp->palNum        = 0xFF;
	pp->primCol.a     = 0xFF;
	pp->primCol.b     = 0xFF;
	pp->primCol.g     = 0xFF;
	pp->primCol.r     = 0xFF;
	pp->envCol.a      = 0;
	pp->envCol.b      = 0;
	pp->envCol.g      = 0;
	pp->envCol.r      = 0;
	pp->envColCount   = 0;
	pp->primColCount  = 0;
	pp->sizeCount     = 0;
	pp->envColRemain  = 0;
	pp->primColRemain = 0;
	pp->aCmpMode      = 0x33;
	if (((pp->kind >> 22) & 3) >= 2) {
		pp->aCmpParam1 = 0;
	} else {
		pp->aCmpParam1 = 1;
	}
	pp->aCmpParam2   = 0xFF;
	pp->aCmpRemain   = 0;
	pp->aCmpCount    = 0;
	pp->rotateCount  = 0;
	pp->spreadRate   = 0.0F;
	pp->rotateTarget = 0.0F;
	pp->rotate       = 0.0F;
	pp->gen          = gp;
	if (gp != NULL) {
		gp->numChild++;
	}
	pp->pJObjOfs     = 0;
	pp->matColRemain = 0;
	pp->matColCount  = 0;
	pp->matRGB       = 0xFF;
	pp->matA         = 0xFF;
	pp->ambColRemain = 0;
	pp->ambColCount  = 0;
	pp->ambRGB       = 0xFF;
	pp->ambA         = 0xFF;
	pp->trail        = 1.0F;
	pp->userdata     = NULL;

	if (gp != NULL && gp->userfunc != NULL && gp->userfunc->hookCreate != NULL) {
		gp->userfunc->hookCreate(pp);
	}

	pp->callback = NULL;
	if (flgInterpret != 0) {
		psInterpretParticle0(pp, NULL);
	}
	return pp;
}

/**
 * @note Address: N/A
 * @note Size: 0xF8
 */
HSD_Particle* psGenerateParticleID0(HSD_Particle* head, int linkNo, int bank, int id, int flgInterpret)
{
	HSD_PSCmdList* cl;
	HSD_PSTexGroup* tg;
	s32 palflag;

	if (linkNo >= 8) {
		return NULL;
	}
	if (bank >= 64) {
		return NULL;
	}
	if (id >= (s32)BANK_CMDCOUNT(bank)) {
		return NULL;
	}

	cl = ((HSD_PSCmdList**)BANK_CMDARRAY(bank))[id];
	if (cl == NULL) {
		return NULL;
	}

	tg = ((HSD_PSTexGroup**)BANK_TEXARRAY(bank))[cl->texGroup];
	if (tg != NULL) {
		palflag = tg->palflag;
	} else {
		palflag = 0;
	}

	return psGenerateParticle0(head, linkNo, bank, cl->kind, cl->texGroup, cl->cmdList, cl->life, 0.0F, 0.0F, 0.0F,
	                           cl->vx, cl->vy, cl->vz, cl->size, cl->grav, cl->fric, palflag, NULL, flgInterpret);
}

/**
 * @note Address: N/A
 * @note Size: 0x64
 */
HSD_Particle* psGenerateParticle(int linkNo, int bank, u64 kind, int texGroup, u8* list, int life, f32 x, f32 y, f32 z,
                                 f32 vx, f32 vy, f32 vz, f32 size, f32 grav, f32 fric, int palflag, HSD_Generator* gp)
{
	return psGenerateParticle0(NULL, linkNo, bank, kind, texGroup, list, life, x, y, z, vx, vy, vz, size, grav, fric, palflag, gp, 1);
}

/**
 * @note Address: 0x801DE040
 * @note Size: 0xFC
 */
void psKillAllParticle(void)
{
	HSD_Particle* node;
	HSD_Particle* prev;
	HSD_Particle* next;
	HSD_Particle* cur;
	int i;

	for (i = 0; i < 16; i++) {
		cur = _psListGetFirst(i);
		while (cur != NULL) {
			next = cur->next;
			prev = NULL;
			node = _psListGetFirst(cur->linkNo);
			while (node != NULL) {
				if (node == cur) {
					if (cur->gen != NULL) {
						cur->gen->numChild--;
					}
					if (cur->appsrt != NULL) {
						psRemoveParticleAppSRT(cur);
					}
					psDeletePntJObjwithParticle(node);
					_psListDelete(node, prev);
					if (cur->gen != NULL && cur->gen->userfunc != NULL && cur->gen->userfunc->hookDelete != NULL) {
						cur->gen->userfunc->hookDelete(cur);
					}
					break;
				}
				prev = node;
				node = node->next;
			}
			cur = next;
		}
	}
}

/**
 * @note Address: N/A
 * @note Size: 0xF8
 */
void psSetPointJObj(s32 no, HSD_JObj* jobj)
{
	HSD_JObj* old;

	if (no < 0 || no > 8) {
		return;
	}

	if (no != 0) {
		old = (HSD_JObj*)psPointJObj[no - 1];
		if (old == jobj) {
			return;
		}
		if (old != NULL) {
			HSD_JObjUnref(old);
		}
		psPointJObj[no - 1] = jobj;
		particle_ref_INC(jobj);
	} else {
		int i;
		for (i = 0; i < 8; i++) {
			if ((HSD_JObj*)psPointJObj[i] == jobj) {
				HSD_JObjUnref((HSD_JObj*)psPointJObj[i]);
				psPointJObj[i] = NULL;
			}
		}
	}
}

/**
 * @note Address: N/A
 * @note Size: 0xDC
 */
void psSetPointJObjNodup(HSD_JObj* jobj, s32 no)
{
	int i;
	HSD_JObj* old;

	if (no < 0 || no > 8) {
		return;
	}

	for (i = 0; i < 8; i++) {
		if ((HSD_JObj*)psPointJObj[i] == jobj) {
			HSD_JObjUnref((HSD_JObj*)psPointJObj[i]);
			psPointJObj[i] = NULL;
		}
	}

	if (no != 0) {
		old = (HSD_JObj*)psPointJObj[no - 1];
		if (old != NULL) {
			HSD_JObjUnref(old);
		}
		psPointJObj[no - 1] = jobj;
		particle_ref_INC(jobj);
	}
}

/**
 * @note Address: N/A
 * @note Size: 0x68
 */
void psClearPointJObj(void)
{
	int i;
	for (i = 0; i < 8; i++) {
		if (psPointJObj[i] != NULL) {
			HSD_JObjUnref((HSD_JObj*)psPointJObj[i]);
			psPointJObj[i] = NULL;
		}
	}
}

/**
 * @note Address: 0x801DD33C
 * @note Size: 0x140
 */
// psInitDataBankLoad

/**
 * @note Address: 0x801DD47C
 * @note Size: 0x374
 */
// psInitDataBankLocate
/*
    lhz      r6, 0x0(r3)
    cmpwi    r6, 0x40
    bge      .L_801DD494
    cmpwi    r6, 0x0
    beq      .L_801DD4A0
    b        .L_801DD5C4
.L_801DD494:
    cmpwi    r6, 0x44
    bge      .L_801DD5C4
    b        .L_801DD578
.L_801DD4A0:
    lwz      r7, 0x4(r3)
    addi     r8, r3, 0x8
    li       r0, 0x0
    li       r6, 0x0
    cmpwi    r7, 0x0
    ble      .L_801DD5C4
    cmpwi    r7, 0x8
    subi     r10, r7, 0x8
    ble      .L_801DD548
    addi     r9, r10, 0x7
    mr       r11, r3
    srwi     r9, r9, 3
    mtctr    r9
    cmpwi    r10, 0x0
    ble      .L_801DD548
.L_801DD4DC:
    lwz      r9, 0x8(r11)
    addi     r6, r6, 0x8
    add      r9, r9, r3
    stw      r9, 0x8(r11)
    lwz      r9, 0xc(r11)
    add      r9, r9, r3
    stw      r9, 0xc(r11)
    lwz      r9, 0x10(r11)
    add      r9, r9, r3
    stw      r9, 0x10(r11)
    lwz      r9, 0x14(r11)
    add      r9, r9, r3
    stw      r9, 0x14(r11)
    lwz      r9, 0x18(r11)
    add      r9, r9, r3
    stw      r9, 0x18(r11)
    lwz      r9, 0x1c(r11)
    add      r9, r9, r3
    stw      r9, 0x1c(r11)
    lwz      r9, 0x20(r11)
    add      r9, r9, r3
    stw      r9, 0x20(r11)
    lwz      r9, 0x24(r11)
    add      r9, r9, r3
    stw      r9, 0x24(r11)
    addi     r11, r11, 0x20
    bdnz     .L_801DD4DC
.L_801DD548:
    slwi     r10, r6, 2
    subf     r9, r6, r7
    add      r10, r3, r10
    mtctr    r9
    cmpw     r6, r7
    bge      .L_801DD5C4
.L_801DD560:
    lwz      r6, 0x8(r10)
    add      r6, r6, r3
    stw      r6, 0x8(r10)
    addi     r10, r10, 0x4
    bdnz     .L_801DD560
    b        .L_801DD5C4
.L_801DD578:
    lwz      r0, 0x4(r3)
    addi     r6, r3, 0xc
    lwz      r7, 0x8(r3)
    mr       r9, r3
    slwi     r8, r0, 2
    li       r10, 0x0
    add      r7, r7, r0
    subf     r8, r8, r6
    b        .L_801DD5B8
.L_801DD59C:
    lwz      r6, 0xc(r9)
    cmpwi    r6, 0x0
    beq      .L_801DD5B0
    add      r6, r6, r3
    stw      r6, 0xc(r9)
.L_801DD5B0:
    addi     r9, r9, 0x4
    addi     r10, r10, 0x1
.L_801DD5B8:
    lwz      r6, 0x8(r3)
    cmpw     r10, r6
    blt      .L_801DD59C
.L_801DD5C4:
    slwi     r6, r0, 2
    subf     r3, r0, r7
    add      r6, r8, r6
    mtctr    r3
    cmpw     r0, r7
    bge      .L_801DD60C
.L_801DD5DC:
    lwz      r3, 0x0(r6)
    cmplwi   r3, 0x0
    beq      .L_801DD604
    lwz      r0, 0x8(r3)
    rlwinm   r0, r0, 0, 7, 3
    stw      r0, 0x8(r3)
    lwz      r3, 0x0(r6)
    lwz      r0, 0x8(r3)
    oris     r0, r0, 0x800
    stw      r0, 0x8(r3)
.L_801DD604:
    addi     r6, r6, 0x4
    bdnz     .L_801DD5DC
.L_801DD60C:
    lwz      r0, 0x0(r4)
    addi     r7, r4, 0x4
    mr       r6, r7
    mtctr    r0
    cmpwi    r0, 0x1
    blt      .L_801DD640
.L_801DD624:
    lwz      r3, 0x0(r6)
    cmpwi    r3, 0x0
    beq      .L_801DD638
    add      r3, r3, r4
    stw      r3, 0x0(r6)
.L_801DD638:
    addi     r6, r6, 0x4
    bdnz     .L_801DD624
.L_801DD640:
    mr       r8, r7
    mtctr    r0
    cmpwi    r0, 0x0
    ble      .L_801DD780
.L_801DD650:
    lwz      r3, 0x0(r8)
    cmplwi   r3, 0x0
    beq      .L_801DD778
    li       r10, 0x0
    li       r6, 0x0
    b        .L_801DD688
.L_801DD668:
    addi     r9, r6, 0x18
    lwzx     r3, r7, r9
    cmplwi   r3, 0x0
    beq      .L_801DD680
    add      r3, r3, r4
    stwx     r3, r7, r9
.L_801DD680:
    addi     r6, r6, 0x4
    addi     r10, r10, 0x1
.L_801DD688:
    lwz      r7, 0x0(r8)
    lwz      r3, 0x0(r7)
    cmplw    r10, r3
    blt      .L_801DD668
    lwz      r3, 0x4(r7)
    cmplwi   r3, 0x8
    beq      .L_801DD6B4
    cmplwi   r3, 0x9
    beq      .L_801DD6B4
    cmplwi   r3, 0xa
    bne      .L_801DD778
.L_801DD6B4:
    lhz      r3, 0x16(r7)
    clrlwi.  r3, r3, 31
    beq      .L_801DD6E4
    lwz      r3, 0x0(r7)
    slwi     r3, r3, 2
    addi     r6, r3, 0x18
    lwzx     r3, r7, r6
    cmplwi   r3, 0x0
    beq      .L_801DD778
    add      r3, r3, r4
    stwx     r3, r7, r6
    b        .L_801DD778
.L_801DD6E4:
    lhz      r3, 0x14(r7)
    cmplwi   r3, 0x0
    beq      .L_801DD738
    lwz      r10, 0x0(r7)
    slwi     r7, r10, 2
    b        .L_801DD71C
.L_801DD6FC:
    addi     r6, r7, 0x18
    lwzx     r3, r9, r6
    cmplwi   r3, 0x0
    beq      .L_801DD714
    add      r3, r3, r4
    stwx     r3, r9, r6
.L_801DD714:
    addi     r7, r7, 0x4
    addi     r10, r10, 0x1
.L_801DD71C:
    lwz      r9, 0x0(r8)
    lwz      r6, 0x0(r9)
    lhz      r3, 0x14(r9)
    add      r3, r6, r3
    cmplw    r10, r3
    blt      .L_801DD6FC
    b        .L_801DD778
.L_801DD738:
    lwz      r10, 0x0(r7)
    slwi     r6, r10, 2
    b        .L_801DD764
.L_801DD744:
    addi     r9, r6, 0x18
    lwzx     r3, r7, r9
    cmplwi   r3, 0x0
    beq      .L_801DD75C
    add      r3, r3, r4
    stwx     r3, r7, r9
.L_801DD75C:
    addi     r6, r6, 0x4
    addi     r10, r10, 0x1
.L_801DD764:
    lwz      r7, 0x0(r8)
    lwz      r3, 0x0(r7)
    slwi     r3, r3, 1
    cmplw    r10, r3
    blt      .L_801DD744
.L_801DD778:
    addi     r8, r8, 0x4
    bdnz     .L_801DD650
.L_801DD780:
    cmplwi   r5, 0x0
    beqlr
    addi     r4, r5, 0x4
    mtctr    r0
    cmpwi    r0, 0x1
    bltlr
.L_801DD798:
    lwz      r0, 0x0(r4)
    cmpwi    r0, 0x0
    beq      .L_801DD7E4
    add      r0, r0, r5
    li       r6, 0x0
    stw      r0, 0x0(r4)
    lwz      r7, 0x0(r4)
    mr       r3, r7
    b        .L_801DD7D8
.L_801DD7BC:
    lwz      r0, 0x4(r3)
    cmplwi   r0, 0x0
    beq      .L_801DD7D0
    add      r0, r0, r5
    stw      r0, 0x4(r3)
.L_801DD7D0:
    addi     r3, r3, 0x4
    addi     r6, r6, 0x1
.L_801DD7D8:
    lwz      r0, 0x0(r7)
    cmplw    r6, r0
    blt      .L_801DD7BC
.L_801DD7E4:
    addi     r4, r4, 0x4
    bdnz     .L_801DD798
    blr
*/

/**
 * @note Address: 0x801DD7F0
 * @note Size: 0x15C
 */
// psInitDataBank

/**
 * @note Address: 0x801DD94C
 * @note Size: 0x164
 */
// psInitParticle

/**
 * @note Address: 0x801DDAB0
 * @note Size: 0x13C
 */
// psRemoveParticle

/**
 * @note Address: 0x801DDEE4
 * @note Size: 0xF8
 */
// psGenerateParticleID0

/**
 * @note Address: 0x801DE13C
 * @note Size: 0xF8
 */
// psSetPointJObj

/**
 * @note Address: 0x801DE234
 * @note Size: 0xDC
 */
// psSetPointJObjNodup

/**
 * @note Address: 0x801DE378
 * @note Size: 0x5C
 */
void psDeletePntJObjwithParticle(HSD_Particle* pp)
{
	HSD_JObj* jobj;
	u32 idx;

	if (pp->kind & 0x8000) {
		idx  = (pp->kind >> 10) & 0x1c;
		jobj = *(HSD_JObj**)((u8*)psPointJObj + idx);
		if (jobj != NULL) {
			HSD_JObjUnref(jobj);
			*(HSD_JObj**)((u8*)psPointJObj + idx) = NULL;
		}
	}
}

static inline HSD_Particle* _psListGetNext(HSD_Particle* pp)
{
	return pp->next;
}

/**
 * @note Address: N/A
 * @note Size: 0x114
 */
void psKillGeneratorChild(HSD_Generator* gp)
{
	HSD_Particle* prev = NULL;
	HSD_Particle* pp;
	u16 idnum = gp->idnum;

	pp = _psListGetFirst(gp->linkNo);
	while (pp != NULL) {
		HSD_Particle* next = _psListGetNext(pp);
		if (pp->idnum == idnum && pp->gen != NULL && pp->gen == gp) {
			if (pp->gen != NULL) {
				pp->gen->numChild--;
			}
			if (pp->appsrt != NULL) {
				psRemoveParticleAppSRT(pp);
			}
			psDeletePntJObjwithParticle(pp);
			_psListDelete(pp, prev);
			if (pp->gen != NULL && pp->gen->userfunc != NULL && pp->gen->userfunc->hookDelete != NULL) {
				pp->gen->userfunc->hookDelete(pp);
			}
		} else {
			prev = pp;
		}
		pp = next;
	}
}

/**
 * @note Address: 0x801DE4E8
 * @note Size: 0x8
 */
void psSetUserFunc(HSD_Generator* gp, HSD_PSUserFunc* userfunc)
{
	gp->userfunc = userfunc;
}

/**
 * @note Address: 0x801DE3D4
 * @note Size: 0x114
 */
// psKillGeneratorChild
/*
    stwu     r1, -0x30(r1)
    mflr     r0
    stw      r0, 0x34(r1)
    addi     r11, r1, 0x30
    bl       _savegpr_25
    mr       r28, r3
    li       r31, 0x0
    lhz      r29, 0x1c(r3)
    lbz      r3, 0x19(r3)
    bl       _psListGetFirst
    mr       r30, r3
    b        .L_801DE4C8
.L_801DE404:
    lhz      r0, 0x1e(r30)
    lwz      r25, 0x0(r30)
    cmplw    r0, r29
    bne      .L_801DE4C0
    lwz      r4, 0x8c(r30)
    cmplwi   r4, 0x0
    beq      .L_801DE4C0
    cmplw    r4, r28
    bne      .L_801DE4C0
    cmplwi   r4, 0x0
    beq      .L_801DE43C
    lwz      r3, 0x50(r4)
    subi     r0, r3, 0x1
    stw      r0, 0x50(r4)
.L_801DE43C:
    lwz      r0, 0x90(r30)
    cmplwi   r0, 0x0
    beq      .L_801DE450
    mr       r3, r30
    bl       psRemoveParticleAppSRT
.L_801DE450:
    lwz      r4, 0x4(r30)
    rlwinm.  r0, r4, 0, 16, 16
    beq      .L_801DE480
    lis      r3, psPointJObj@ha
    rlwinm   r26, r4, 22, 27, 29
    addi     r27, r3, psPointJObj@l
    lwzx     r3, r27, r26
    cmplwi   r3, 0x0
    beq      .L_801DE480
    bl       HSD_JObjUnref
    li       r0, 0x0
    stwx     r0, r27, r26
.L_801DE480:
    mr       r3, r30
    mr       r4, r31
    bl       _psListDelete
    lwz      r3, 0x8c(r30)
    cmplwi   r3, 0x0
    beq      .L_801DE4C4
    lwz      r3, 0x58(r3)
    cmplwi   r3, 0x0
    beq      .L_801DE4C4
    lwz      r12, 0x4(r3)
    cmplwi   r12, 0x0
    beq      .L_801DE4C4
    mr       r3, r30
    mtctr    r12
    bctrl
    b        .L_801DE4C4
.L_801DE4C0:
    mr       r31, r30
.L_801DE4C4:
    mr       r30, r25
.L_801DE4C8:
    cmplwi   r30, 0x0
    bne      .L_801DE404
    addi     r11, r1, 0x30
    bl       _restgpr_25
    lwz      r0, 0x34(r1)
    mtlr     r0
    addi     r1, r1, 0x30
    blr
*/
