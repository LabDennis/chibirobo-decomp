#include <global.h>

#include <string.h>

#include <dolphin/os.h>
#include <sysdolphin/jobj.h>
#include <sysdolphin/object.h>
#include <sysdolphin/particle.h>
#include <sysdolphin/particle_internal.h>
#include <sysdolphin/psinterpret.h>
#include <sysdolphin/pslist.h>

void psClearPointJObj(void);

const u32* ptclref[PS_BANK_COUNT];
s32 psBankTextureCounts[PS_BANK_COUNT];
HSD_PSTexGroup** psTexGroupArray[PS_BANK_COUNT];
HSD_PSFormGroup** psFormGroupArray[PS_BANK_COUNT];
HSD_JObj* psPointJObj[8];

u16 psParticleCount;
u16 psGeneratorCount;
u16 psAppSRTCount;
u16 psPeakParticleCount;
u16 psPeakGeneratorCount;
u16 psPeakAppSRTCount;
PSParticleCallback* psParticleCallbackTable;

typedef struct {
	HSD_PSCmdList** entries;
	s32 firstId;
	s32 count;
} PSCommandBank;

static PSCommandBank sCommandBanks[PS_BANK_COUNT];

static BOOL readBankHeader(s32* cmdBank, const s32* texBank, const s32* formBank, PSCommandBank* commands)
{
	u16 version;

	if (formBank != NULL && *formBank != *texBank) {
		OSPanic("particle.c", 95, "illigal form data (strange number of group)\n");
		return FALSE;
	}

	memcpy(&version, cmdBank, sizeof(version));
	switch (version) {
	case 0:
		commands->entries = (HSD_PSCmdList**)(cmdBank + 2);
		commands->firstId = 0;
		commands->count = cmdBank[1];
		break;
	case 0x40:
	case 0x41:
	case 0x42:
	case 0x43:
		commands->entries = (HSD_PSCmdList**)(cmdBank + 3);
		commands->firstId = cmdBank[1];
		commands->count = cmdBank[2];
		break;
	default:
		OSPanic("particle.c", 125, "psInitDataBanks: unknown version\n");
		return FALSE;
	}

	return commands->firstId >= 0 && commands->count >= 0;
}

static void loadBank(s32 bank, const PSCommandBank* commands, s32* texBank, const void* ref, s32* formBank)
{
	sCommandBanks[bank] = *commands;
	ptclref[bank] = ref;
	psBankTextureCounts[bank] = *texBank;
	psTexGroupArray[bank] = (HSD_PSTexGroup**)(texBank + 1);
	psFormGroupArray[bank] = formBank != NULL ? (HSD_PSFormGroup**)(formBank + 1) : NULL;
}

/**
 * @note Address: 0x801DD33C
 * @note Size: 0x140
 */
void psInitDataBankLoad(s32 bank, s32* cmdBank, s32* texBank, void* ref, s32* formBank)
{
	PSCommandBank commands;

	if (bank >= 0 && bank < PS_BANK_COUNT && readBankHeader(cmdBank, texBank, formBank, &commands)) {
		loadBank(bank, &commands, texBank, ref, formBank);
	}
}

static inline u8* bankPointer(void* bank, const void* offset)
{
	return (u8*)bank + (unsigned long)offset;
}

static void relocateBank(const PSCommandBank* commands, s32* cmdBank, s32* texBank, s32* formBank)
{
	s32 i;
	HSD_PSTexGroup** textureGroups = (HSD_PSTexGroup**)(texBank + 1);
	s32 groupCount = texBank[0];

	for (i = 0; i < commands->count; i++) {
		if (commands->entries[i] != NULL) {
			commands->entries[i] = (HSD_PSCmdList*)bankPointer(cmdBank, commands->entries[i]);
		}
	}

	for (i = 0; i < commands->count; i++) {
		HSD_PSCmdList* command = commands->entries[i];
		if (command != NULL) {
			command->kind &= ~PS_PARTICLE_SORT_ORDER_MASK;
			command->kind |= (PS_PARTICLE_SORT_ORDER_COUNT / 2) << PS_PARTICLE_SORT_ORDER_SHIFT;
		}
	}

	for (i = 0; i < groupCount; i++) {
		if (textureGroups[i] != NULL) {
			textureGroups[i] = (HSD_PSTexGroup*)bankPointer(texBank, textureGroups[i]);
		}
	}

	for (i = 0; i < groupCount; i++) {
		HSD_PSTexGroup* group = textureGroups[i];
		u32 entryCount;
		u32 j;

		if (group == NULL) {
			continue;
		}
		entryCount = group->num;
		switch (group->fmt) {
		case GX_TF_C4:
		case GX_TF_C8:
		case GX_TF_C14X2:
			if (group->palflag & 1) {
				entryCount++;
			} else if (group->palnum != 0) {
				entryCount += group->palnum;
			} else {
				entryCount += group->num;
			}
			break;
		}
		for (j = 0; j < entryCount; j++) {
			if (group->texTable[j] != NULL) {
				group->texTable[j] = bankPointer(texBank, group->texTable[j]);
			}
		}
	}

	if (formBank != NULL) {
		HSD_PSFormGroup** formGroups = (HSD_PSFormGroup**)(formBank + 1);

		for (i = 0; i < groupCount; i++) {
			HSD_PSFormGroup* group = formGroups[i];
			if (group != NULL) {
				u32 j;

				group = (HSD_PSFormGroup*)bankPointer(formBank, group);
				formGroups[i] = group;
				for (j = 0; j < group->num; j++) {
					if (group->formTable[j] != NULL) {
						group->formTable[j] = (u32*)bankPointer(formBank, group->formTable[j]);
					}
				}
			}
		}
	}
}

/**
 * @note Address: 0x801DD47C
 * @note Size: 0x374
 */
void psInitDataBankLocate(s32* cmdBank, s32* texBank, s32* formBank)
{
	PSCommandBank commands;

	if (readBankHeader(cmdBank, texBank, formBank, &commands)) {
		relocateBank(&commands, cmdBank, texBank, formBank);
	}
}

/**
 * @note Address: 0x801DD7F0
 * @note Size: 0x15C
 */
void psInitDataBank(s32 bank, s32* cmdBank, s32* texBank, void* ref, s32* formBank)
{
	PSCommandBank commands;

	if (bank >= 0 && bank < PS_BANK_COUNT && readBankHeader(cmdBank, texBank, formBank, &commands)) {
		relocateBank(&commands, cmdBank, texBank, formBank);
		loadBank(bank, &commands, texBank, ref, formBank);
	}
}

HSD_PSCmdList* psGetBankCommand(s32 bank, s32 id)
{
	const PSCommandBank* commands;
	s32 index;

	if (bank < 0 || bank >= PS_BANK_COUNT) {
		return NULL;
	}
	commands = &sCommandBanks[bank];
	if (id < commands->firstId) {
		return NULL;
	}
	index = id - commands->firstId;
	if (index >= commands->count) {
		return NULL;
	}
	return commands->entries[index];
}

static void clearBanks(void)
{
	s32 i;

	memset(sCommandBanks, 0, sizeof(sCommandBanks));
	for (i = 0; i < PS_BANK_COUNT; i++) {
		psBankTextureCounts[i] = 0;
		psTexGroupArray[i]     = NULL;
		psFormGroupArray[i]    = NULL;
		ptclref[i]             = NULL;
	}
}

/**
 * @note Address: 0x801DD94C
 * @note Size: 0x164
 */
void psInitParticle(s32 count)
{
	int i;

	_psLinkInit(count);
	psParticleCount     = 0;
	psPeakParticleCount = 0;
	clearBanks();
	psParticleCallbackTable = NULL;
	for (i = 0; i < 8; i++) {
		psPointJObj[i] = NULL;
	}
}

/**
 * @note Address: 0x801DDAB0
 * @note Size: 0x13C
 */
void psRemoveParticle(void)
{
	psKillAllParticle();
	psKillAllGenerator();
	_psListClear();
	clearBanks();
	psClearPointJObj();
}

/**
 * @note Address: N/A
 * @note Size: 0x2FC
 */
HSD_Particle* psGenerateParticle0(HSD_Particle* parent, int linkNo, int bank, int reserved0, int reserved1, u32 kind, int texGroup,
                                  u8* list, f32 x, f32 y, f32 z, f32 vx, f32 vy, f32 vz, f32 size, f32 grav, int life, f32 fric,
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
	pp->kind       = kind;
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
		pp->kind |= PS_PARTICLE_SHARED_PALETTE;
	}

	pp->cmdWait       = (u16)(list != NULL);
	pp->poseNum       = 0;
	pp->palNum        = PS_PARTICLE_PALETTE_DEFAULT;
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
	if (((pp->kind >> PS_PARTICLE_BLEND_SHIFT) & PS_PARTICLE_BLEND_MODE_MASK) >= PS_PARTICLE_BLEND_SUBTRACT) {
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
 * @note Address: 0x801DDEE4
 * @note Size: 0xF8
 */
HSD_Particle* psGenerateParticleID0(HSD_Particle* head, int linkNo, int bank, int id, int flgInterpret)
{
	HSD_PSCmdList* cl;
	HSD_PSTexGroup* tg;
	s32 palflag;

	if (linkNo < 0 || linkNo >= 8) {
		return NULL;
	}

	cl = psGetBankCommand(bank, id);
	if (cl == NULL) {
		return NULL;
	}

	tg = psTexGroupArray[bank][cl->texGroup];
	if (tg != NULL) {
		palflag = tg->palflag;
	} else {
		palflag = 0;
	}

	return psGenerateParticle0(head, linkNo, bank, 0, 0, cl->kind, cl->texGroup, cl->cmdList, 0.0F, 0.0F, 0.0F, cl->vx, cl->vy,
	                           cl->vz, cl->size, cl->grav, cl->life, cl->fric, palflag, NULL, flgInterpret);
}

/**
 * @note Address: N/A
 * @note Size: 0x64
 */
HSD_Particle* psGenerateParticle(int linkNo, int bank, int reserved, u32 kind, int texGroup, u8* list, int life, f32 x, f32 y, f32 z,
                                 f32 vx, f32 vy, f32 vz, f32 size, f32 grav, f32 fric, int palflag, HSD_Generator* gp)
{
	return psGenerateParticle0(NULL, linkNo, bank, kind, reserved, kind, texGroup, list, x, y, z, vx, vy, vz, size, grav, life, fric,
	                           palflag, gp, 1);
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
 * @note Address: 0x801DE13C
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
		ref_INC(jobj);
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
 * @note Address: 0x801DE234
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
		ref_INC(jobj);
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
 * @note Address: 0x801DE3D4
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
