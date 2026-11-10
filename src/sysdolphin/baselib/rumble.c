#include <global.h>

#include <dolphin/os.h>
#include <sysdolphin/rumble.h>

extern void PADControlMotor(int chan, int command);

struct RumbleInfo {
	u16 max_list;                     // _00
	u8 unk2;                          // _02
	HSD_PadRumbleListData* listdatap; // _04
};

// the pad library's state lives in controller.c, rumble only touches the tail
struct PadLibData {
	u8 _00[0x2C];           // _00
	RumbleInfo rumble_info; // _2C
};

extern PadLibData HSD_PadLibData;

HSD_RumbleData gPadRumbleStates[4];

static HSD_RumbleData default_rumble = { 0 };

/**
 * @note Address: 0x801CD9CC
 * @note Size: 0x4C
 */
void HSD_PadRumbleOn(u8 no)
{
	bool intrEnabled  = OSDisableInterrupts();
	HSD_RumbleData* r = &gPadRumbleStates[no];

	r->direct_status = 2;
	OSRestoreInterrupts(intrEnabled);
}

/**
 * @note Address: 0x801CDA18
 * @note Size: 0x4C
 */
void HSD_PadRumbleOffN(u8 no)
{
	bool intrEnabled  = OSDisableInterrupts();
	HSD_RumbleData* r = &gPadRumbleStates[no];

	r->direct_status = 0;
	OSRestoreInterrupts(intrEnabled);
}

static inline void HSD_PadRumbleFree(HSD_RumbleData* a, HSD_PadRumbleListData* b)
{
	RumbleInfo* info             = &HSD_PadLibData.rumble_info;
	HSD_PadRumbleListData** prev = &a->listdatap;

	while ((*prev) != b) {
		prev = &(*prev)->next;
	}
	*prev = b->next;
	a->nb_list--;
	b->next         = info->listdatap;
	info->listdatap = b;
}

/**
 * @note Address: 0x801CDA64
 * @note Size: 0x94
 */
void HSD_PadRumbleRemove(u8 no)
{
	HSD_RumbleData* r          = &gPadRumbleStates[no];
	bool intrEnabled           = OSDisableInterrupts();
	HSD_PadRumbleListData* cur = r->listdatap;

	while (cur != NULL) {
		HSD_PadRumbleListData* next = cur->next;
		HSD_PadRumbleFree(r, cur);
		cur = next;
	}
	OSRestoreInterrupts(intrEnabled);
}

/**
 * @note Address: 0x801CDAF8
 * @note Size: 0xBC
 */
void HSD_PadRumbleRemoveAll(void)
{
	int i;

	for (i = 0; i < 4; i++) {
		HSD_PadRumbleRemove(i);
	}
}

static inline void HSD_PadRumblePause(u8 no, int status)
{
	bool intrEnabled           = OSDisableInterrupts();
	HSD_PadRumbleListData* cur = gPadRumbleStates[no].listdatap;

	while (cur != NULL) {
		HSD_PadRumbleListData* next = cur->next;

		cur->pause = status;
		cur        = next;
	}
	OSRestoreInterrupts(intrEnabled);
}

/**
 * @note Address: 0x801CDBB4
 * @note Size: 0x80
 */
void HSD_PadRumblePauseAll(void)
{
	int i;

	for (i = 0; i < 4; i++) {
		HSD_PadRumblePause(i, 1);
	}
}

/**
 * @note Address: 0x801CDC34
 * @note Size: 0x80
 */
void HSD_PadRumbleActiveAll(void)
{
	int i;

	for (i = 0; i < 4; i++) {
		HSD_PadRumblePause(i, 0);
	}
}

static inline void HSD_PadRumbleInsert(HSD_PadRumbleListData** prev, HSD_PadRumbleListData* data)
{
	HSD_PadRumbleListData* cur;

	while ((cur = *prev) != NULL && cur->pri <= data->pri) {
		prev = &cur->next;
	}
	data->next = cur;
	*prev      = data;
}

/**
 * @note Address: 0x801CDCB4
 * @note Size: 0xF8
 */
int HSD_PadRumbleAdd(u8 no, int id, int frame, int pri, void* listp)
{
	RumbleInfo* info            = &HSD_PadLibData.rumble_info;
	HSD_RumbleData* r           = &gPadRumbleStates[no];
	int result                  = 0;
	bool intrEnabled            = OSDisableInterrupts();
	HSD_PadRumbleListData* data = info->listdatap;

	if (data != NULL && r->nb_list < info->max_list) {
		info->listdatap  = data->next;
		data->id         = id;
		data->pause      = 0;
		data->pri        = pri;
		data->status     = 0;
		data->loop_count = 0;
		data->wait       = 0;
		data->frame      = frame;
		data->stack      = NULL;
		data->headp      = listp;
		data->listp      = listp;
		HSD_PadRumbleInsert(&r->listdatap, data);
		r->nb_list++;
		result = 1;
	}
	OSRestoreInterrupts(intrEnabled);
	return result;
}

/**
 * @note Address: 0x801CDDAC
 * @note Size: 0x190
 */
int HSD_PadRumbleInterpret1(HSD_PadRumbleListData* a, u8* b)
{
	if (a->pause == 1) {
		return 0;
	}
	while (a->wait == 0) {
		switch ((*(u8*)a->listp >> 5) & 7) {
		case 0:
			if (a->frame == -2) {
				return 1;
			}
			a->listp = a->headp;
			break;
		case 1:
			a->status = 2;
			a->wait   = *a->listp & 0x1FFF;
			a->listp++;
			break;
		case 2:
			a->status = 1;
			a->wait   = *a->listp & 0x1FFF;
			a->listp++;
			break;
		case 3:
			a->status = 0;
			a->wait   = *a->listp & 0x1FFF;
			a->listp++;
			break;
		case 4:
			a->loop_count = *a->listp & 0x1FFF;
			a->listp++;
			a->stack = a->listp;
			break;
		case 5:
			if (--a->loop_count != 0) {
				a->listp = a->stack;
			} else {
				a->listp++;
			}
			break;
		}
	}
	*b = a->status;
	a->wait--;
	if (a->frame != -1 && a->frame != -2) {
		if (--a->frame == 0) {
			return 1;
		}
	}
	return 0;
}

/**
 * @note Address: 0x801CDF3C
 * @note Size: 0x128
 */
void HSD_PadRumbleInterpret(void)
{
	HSD_RumbleData* r30 = gPadRumbleStates;
	HSD_PadRumbleListData* r29;
	HSD_PadRumbleListData* r28;
	int i;

	for (i = 0; i < 4; i++, r30++) {
		r30->status = r30->direct_status;
		r29         = r30->listdatap;
		while (r29 != NULL) {
			r28 = r29->next;

			if (HSD_PadRumbleInterpret1(r29, &r30->status) != 0) {
				HSD_PadRumbleFree(r30, r29);
			}
			r29 = r28;
		}
		if (r30->status != r30->last_status) {
			switch (r30->status) {
			case 0:
				PADControlMotor(i, 2);
				break;
			case 1:
				PADControlMotor(i, 0);
				break;
			case 2:
				PADControlMotor(i, 1);
				break;
			}
			r30->last_status = r30->status;
		}
	}
}

/**
 * @note Address: 0x801CE064
 * @note Size: 0x1D4
 */
void HSD_PadRumbleInit(u16 a, HSD_PadRumbleListData* b)
{
	RumbleInfo* info = &HSD_PadLibData.rumble_info;
	int i;

	info->max_list  = a;
	info->listdatap = b;
	if (a != 0) {
		for (i = 0; i < a - 1; i++) {
			info->listdatap[i].next = &info->listdatap[i + 1];
		}
		info->listdatap[i].next = NULL;
	}
	for (i = 0; i < 4; i++) {
		gPadRumbleStates[i] = default_rumble;
	}
}
