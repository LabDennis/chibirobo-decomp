#include <global.h>

#include <dolphin/os.h>
#include <sysdolphin/controller.h>
#include <sysdolphin/rumble.h>

enum {
	RUMBLE_STATE_STOP_HARD = 0,
	RUMBLE_STATE_STOP = 1,
	RUMBLE_STATE_ON = 2,
};

enum {
	RUMBLE_OP_END = 0,
	RUMBLE_OP_ON = 1,
	RUMBLE_OP_STOP = 2,
	RUMBLE_OP_STOP_HARD = 3,
	RUMBLE_OP_LOOP_START = 4,
	RUMBLE_OP_LOOP_END = 5,
};

enum {
	RUMBLE_REPEAT = -1,
	RUMBLE_PLAY_ONCE = -2,
};

// A big-endian command word packs a three-bit opcode and a thirteen-bit operand.
enum {
	RUMBLE_OPCODE_BYTE_SHIFT = 5,
	RUMBLE_OPCODE_MASK = 7,
	RUMBLE_OPERAND_MASK = 0x1FFF,
};

HSD_RumbleData gPadRumbleStates[PAD_MAX_CONTROLLERS];

static HSD_RumbleData default_rumble = { 0 };

/**
 * @note Address: 0x801CD9CC
 * @note Size: 0x4C
 */
void HSD_PadRumbleOn(u8 no)
{
	bool intrEnabled  = OSDisableInterrupts();
	HSD_RumbleData* r = &gPadRumbleStates[no];

	r->direct_status = RUMBLE_STATE_ON;
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

	r->direct_status = RUMBLE_STATE_STOP_HARD;
	OSRestoreInterrupts(intrEnabled);
}

static inline void HSD_PadRumbleFree(HSD_RumbleData* port, HSD_PadRumbleListData* entry)
{
	RumbleInfo* info            = &HSD_PadLibData.rumble_info;
	HSD_PadRumbleListData** prev = &port->listdatap;

	while (*prev != entry) {
		prev = &(*prev)->next;
	}
	*prev = entry->next;
	port->nb_list--;
	entry->next     = info->listdatap;
	info->listdatap = entry;
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

	for (i = 0; i < PAD_MAX_CONTROLLERS; i++) {
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

	for (i = 0; i < PAD_MAX_CONTROLLERS; i++) {
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

	for (i = 0; i < PAD_MAX_CONTROLLERS; i++) {
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
		data->status     = RUMBLE_STATE_STOP_HARD;
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

static inline void HSD_PadRumbleSetTimedState(HSD_PadRumbleListData* entry, u8 motor_state)
{
	entry->status = motor_state;
	entry->wait = *entry->listp & RUMBLE_OPERAND_MASK;
	entry->listp++;
}

/**
 * @note Address: 0x801CDDAC
 * @note Size: 0x190
 */
int HSD_PadRumbleInterpret1(HSD_PadRumbleListData* entry, u8* motor_state)
{
	if (entry->pause == 1) {
		return 0;
	}
	while (entry->wait == 0) {
		switch ((*(const u8*)entry->listp >> RUMBLE_OPCODE_BYTE_SHIFT) & RUMBLE_OPCODE_MASK) {
		case RUMBLE_OP_END:
			if (entry->frame == RUMBLE_PLAY_ONCE) {
				return 1;
			}
			entry->listp = entry->headp;
			break;
		case RUMBLE_OP_ON:
			HSD_PadRumbleSetTimedState(entry, RUMBLE_STATE_ON);
			break;
		case RUMBLE_OP_STOP:
			HSD_PadRumbleSetTimedState(entry, RUMBLE_STATE_STOP);
			break;
		case RUMBLE_OP_STOP_HARD:
			HSD_PadRumbleSetTimedState(entry, RUMBLE_STATE_STOP_HARD);
			break;
		case RUMBLE_OP_LOOP_START:
			entry->loop_count = *entry->listp & RUMBLE_OPERAND_MASK;
			entry->listp++;
			entry->stack = entry->listp;
			break;
		case RUMBLE_OP_LOOP_END:
			if (--entry->loop_count != 0) {
				entry->listp = entry->stack;
			} else {
				entry->listp++;
			}
			break;
		}
	}
	*motor_state = entry->status;
	entry->wait--;
	if (entry->frame != RUMBLE_REPEAT && entry->frame != RUMBLE_PLAY_ONCE) {
		if (--entry->frame == 0) {
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
	HSD_RumbleData* port = gPadRumbleStates;
	HSD_PadRumbleListData* entry;
	HSD_PadRumbleListData* next;
	int i;

	for (i = 0; i < PAD_MAX_CONTROLLERS; i++, port++) {
		port->status = port->direct_status;
		entry        = port->listdatap;
		while (entry != NULL) {
			next = entry->next;

			if (HSD_PadRumbleInterpret1(entry, &port->status) != 0) {
				HSD_PadRumbleFree(port, entry);
			}
			entry = next;
		}
		if (port->status != port->last_status) {
			switch (port->status) {
			case RUMBLE_STATE_STOP_HARD:
				PADControlMotor(i, PAD_MOTOR_STOP_HARD);
				break;
			case RUMBLE_STATE_STOP:
				PADControlMotor(i, PAD_MOTOR_STOP);
				break;
			case RUMBLE_STATE_ON:
				PADControlMotor(i, PAD_MOTOR_RUMBLE);
				break;
			}
			port->last_status = port->status;
		}
	}
}

/**
 * @note Address: 0x801CE064
 * @note Size: 0x1D4
 */
void HSD_PadRumbleInit(u16 count, HSD_PadRumbleListData* entries)
{
	RumbleInfo* info = &HSD_PadLibData.rumble_info;
	int i;

	info->max_list  = count;
	info->listdatap = entries;
	if (count != 0) {
		for (i = 0; i < count - 1; i++) {
			info->listdatap[i].next = &info->listdatap[i + 1];
		}
		info->listdatap[i].next = NULL;
	}
	for (i = 0; i < PAD_MAX_CONTROLLERS; i++) {
		gPadRumbleStates[i] = default_rumble;
	}
}
