#ifndef _controller_h_
#define _controller_h_

#include <dolphin/types.h>

#include <dolphin/pad.h>
#include <sysdolphin/forward.h>
#include <sysdolphin/rumble.h>

#define HSD_PAD_DPADLEFT  0x0001
#define HSD_PAD_DPADRIGHT 0x0002
#define HSD_PAD_DPADDOWN  0x0004
#define HSD_PAD_DPADUP    0x0008

// stick directions as button bits
#define HSD_PAD_STICK_UP     0x00010000
#define HSD_PAD_STICK_DOWN   0x00020000
#define HSD_PAD_STICK_LEFT   0x00040000
#define HSD_PAD_STICK_RIGHT  0x00080000
#define HSD_PAD_CSTICK_UP    0x00100000
#define HSD_PAD_CSTICK_DOWN  0x00200000
#define HSD_PAD_CSTICK_LEFT  0x00400000
#define HSD_PAD_CSTICK_RIGHT 0x00800000

struct HSD_PadData {
	PADStatus stat[4]; // _00
	u32 connect_mask;  // _30
};

struct HSD_PadStatus {
	u32 button;        // _00
	u32 last_button;   // _04
	u32 trigger;       // _08
	u32 repeat;        // _0C
	u32 release;       // _10
	s32 repeat_count;  // _14
	s8 stickX;         // _18
	s8 stickY;         // _19
	s8 subStickX;      // _1A
	s8 subStickY;      // _1B
	u8 analogL;        // _1C
	u8 analogR;        // _1D
	u8 analogA;        // _1E
	u8 analogB;        // _1F
	f32 nml_stickX;    // _20
	f32 nml_stickY;    // _24
	f32 nml_subStickX; // _28
	f32 nml_subStickY; // _2C
	f32 nml_analogL;   // _30
	f32 nml_analogR;   // _34
	f32 nml_analogA;   // _38
	f32 nml_analogB;   // _3C
	u8 cross_dir;      // _40
	u8 _41;            // _41, set when PADRead reported the port connected
	s8 err;            // _42
};

typedef enum HSD_FlushType {
	HSD_PAD_FLUSH_QUEUE_MERGE,
	HSD_PAD_FLUSH_QUEUE_THROWAWAY,
	HSD_PAD_FLUSH_QUEUE_LEAVE1,
} HSD_FlushType;

struct PadLibData {
	u8 qnum;                                 // _00
	u8 qread;                                // _01
	u8 qwrite;                               // _02
	u8 qcount;                               // _03
	u8 qtype;                                // _04
	HSD_PadData* queue;                      // _08
	s32 repeat_start;                        // _0C
	s32 repeat_interval;                     // _10
	s8 adc_type;                             // _14
	s8 adc_th;                               // _15
	f32 adc_angle;                           // _18
	u8 clamp_stickType;                      // _1C
	u8 clamp_stickShift;                     // _1D
	s8 clamp_stickMax;                       // _1E
	s8 clamp_stickMin;                       // _1F
	u8 clamp_analogLRShift;                  // _20
	u8 clamp_analogLRMax;                    // _21
	u8 clamp_analogLRMin;                    // _22
	u8 clamp_analogABShift;                  // _23
	u8 clamp_analogABMax;                    // _24
	u8 clamp_analogABMin;                    // _25
	s8 scale_stick;                          // _26
	u8 scale_analogLR;                       // _27
	u8 scale_analogAB;                       // _28
	u8 cross_dir;                            // _29
	u8 reset_switch_status;                  // _2A
	u8 reset_switch;                         // _2B
	u16 rumble_max_list;                     // _2C
	u8 rumble_unk2E;                         // _2E
	HSD_PadRumbleListData* rumble_listdatap; // _30
};

extern PadLibData HSD_PadLibData;
extern HSD_PadStatus HSD_PadMasterStatus[4];
extern HSD_PadStatus HSD_PadCopyStatus[4];

void SetPadQueueType(u8 qtype);
s32 HSD_PadGetResetSwitch(void);
void HSD_PadRenewRawStatus(void);
void HSD_PadRenewMasterStatus(void);
void HSD_PadRenewCopyStatus(void);
void HSD_PadReset(void);
void HSD_PadInit(u8 qnum, HSD_PadData* queue, u16 nb_list, HSD_PadRumbleListData* listdatap);

#endif
