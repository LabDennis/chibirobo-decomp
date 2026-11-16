#include <HBMAxSound.h>

#include <sound/mix.h>
#include <sound/seq.h>
#include <sound/syn.h>

#include "Dolphin/ai.h"
#include "revolution/arc.h"
#include "revolution/ax.h"
#include "revolution/os.h"

#include <cmath>

namespace {

enum {
    NUM_SEQ_PLAYER = 4,
    NUM_SE_PLAYER = 3,

    THREAD_STACK_SIZE = 0x4000,
    THREAD_PRIORITY = 4,
    NUM_MESSAGE = 4,
};

enum {
    MESSAGE_AUDIO_FRAME = 1,
    MESSAGE_QUIT = 8,
};

const char WT_FILENAME[] = "wt\\HomeButtonSe.wt";
const char PCM_FILENAME[] = "wt\\HomeButtonSe.pcm";

struct Player {
    HBMSEQSEQUENCE seq; // at 0x0
    bool used;          // at 0x2E1C
    Player* next;       // at 0x2E20
    Player* prev;       // at 0x2E24
    int num;            // at 0x2E28
};

struct PlayerList {
    Player* head; // at 0x0
    Player* tail; // at 0x4
};

struct Work {
    Player seqPlayer[NUM_SEQ_PLAYER];  // at 0x0
    Player sePlayer[NUM_SE_PLAYER];    // at 0xB8B0
    PlayerList seqList;                // at 0x14334
    PlayerList seList;                 // at 0x1433C
    u32 unk_0x14344;                   // at 0x14344, never touched
    AXOutCallback oldCallback;         // at 0x14348
    ARCHandle arc;                     // at 0x1434C
    OSThread thread;                   // at 0x14368
    OSMessageQueue msgQueue;           // at 0x14680
    OSMessage msgBuffer[NUM_MESSAGE];  // at 0x146A0
    u8* stack;                         // at 0x146B0
    u8* wt;                            // at 0x146B4
    u8* pcm;                           // at 0x146B8
    u8 padding[0x44];                  // at 0x146BC
};

Work* sWork;

// The cursor sounds get players of their own, so that they never cut off one
// of the longer sounds.
inline PlayerList* GetList(int num) {
    switch (num) {
    case 4:
    case 23:
    case 25:
        return &sWork->seList;
    default:
        return &sWork->seqList;
    }
}

inline void StopPlayer(Player* pPlayer) {
    PlayerList* pList = GetList(pPlayer->num);

    HBMSEQSetState(&pPlayer->seq, HBM_SEQ_STATE_0);
    HBMSEQRemoveSequence(&pPlayer->seq);
    pPlayer->used = false;

    if (pPlayer->prev == NULL) {
        pList->head = pPlayer->next;
    } else {
        pPlayer->prev->next = pPlayer->next;
    }

    if (pPlayer->next == NULL) {
        pList->tail = pPlayer->prev;
    } else {
        pPlayer->next->prev = pPlayer->prev;
    }

    pPlayer->next = NULL;
    pPlayer->prev = NULL;
}

Player* GetFreePlayer(int num) {
    Player* pPlayers;
    int count;

    switch (num) {
    case 4:
    case 23:
    case 25:
        pPlayers = sWork->sePlayer;
        count = NUM_SE_PLAYER;
        break;
    default:
        pPlayers = sWork->seqPlayer;
        count = NUM_SEQ_PLAYER;
        break;
    }

    Player* pPlayer = NULL;

    for (int i = 0; i < count; i++) {
        if (!pPlayers[i].used) {
            pPlayer = &pPlayers[i];
            pPlayer->used = true;
            break;
        }
    }

    if (pPlayer == NULL) {
        // none free: take over the one that has played longest
        pPlayer = GetList(num)->head;
        StopPlayer(pPlayer);
        pPlayer->used = true;
    }

    return pPlayer;
}

void AudioFrameCallback() {
    if (sWork == NULL) {
        return;
    }

    OSSendMessage(&sWork->msgQueue,
                  reinterpret_cast<OSMessage>(MESSAGE_AUDIO_FRAME),
                  OS_MESSAGE_NOBLOCK);

    if (sWork->oldCallback != NULL) {
        sWork->oldCallback();
    }
}

// A player whose sequence has run out and whose last note has faded is free
// again.
inline void UpdatePlayerList(PlayerList* pList) {
    Player* pPlayer = pList->head;

    while (pPlayer != NULL) {
        Player* pNext = pPlayer->next;

        if (pPlayer->used && HBMSEQGetState(&pPlayer->seq) == HBM_SEQ_STATE_0 &&
            pPlayer->seq.synth.notes == 0) {
            StopPlayer(pPlayer);
        }

        pPlayer = pNext;
    }
}

void* AudioSoundThreadProc(void* /* pArg */) {
    OSMessage msg = NULL;

    while (true) {
        OSReceiveMessage(&sWork->msgQueue, &msg, OS_MESSAGE_BLOCK);

        if (reinterpret_cast<u32>(msg) == MESSAGE_AUDIO_FRAME) {
            HBMSEQRunAudioFrame();
            HBMSYNRunAudioFrame();
            HBMMIXUpdateSettings();

            if (sWork != NULL) {
                UpdatePlayerList(&sWork->seqList);
                UpdatePlayerList(&sWork->seList);
            }
        } else if (reinterpret_cast<u32>(msg) == MESSAGE_QUIT) {
            break;
        }
    }

    return NULL;
}

} // namespace

namespace homebutton {

void PlaySeq(int num) {
    static const char* SOUND_FILENAME[] = {
        "midi\\00_home_button.mid",
        "midi\\01_return_app.mid",
        "midi\\02_goto_menu.mid",
        "midi\\03_reset_app.mid",
        "midi\\04_focus.mid",
        "midi\\05_select.mid",
        "midi\\06_cancel.mid",
        "midi\\07_open_controller.mid",
        "midi\\08_close_controller.mid",
        "midi\\09_volume_plus.mid",
        "midi\\10_volume_minus.mid",
        "midi\\11_volume_plus_limit.mid",
        "midi\\12_volume_minus_limit.mid",
        "midi\\13_nothing_done.mid",
        "midi\\14_vibe_on.mid",
        "midi\\15_vibe_off.mid",
        "midi\\16_start_connect_window.mid",
        "midi\\17_connected1.mid",
        "midi\\18_connected2.mid",
        "midi\\19_connected3.mid",
        "midi\\20_connected4.mid",
        "midi\\21_end_connect_window.mid",
        "midi\\22_manual_open.mid",
        "midi\\23_manual_focus.mid",
        "midi\\24_manual_select.mid",
        "midi\\25_manual_scroll.mid",
        "midi\\26_manual_cancel.mid",
        "midi\\27_manual_return_app.mid",
    };

    ARCFileInfo info;
    u8* pMidi;

    if (ARCOpen(&sWork->arc, SOUND_FILENAME[num], &info)) {
        pMidi = static_cast<u8*>(ARCGetStartAddrInMem(&info));
    } else {
        pMidi = NULL;
    }

    if (pMidi == NULL) {
        return;
    }

    Player* pPlayer = GetFreePlayer(num);

    HBMSEQAddSequence(&pPlayer->seq, pMidi, sWork->wt, sWork->pcm, NULL);
    HBMSEQSetState(&pPlayer->seq, HBM_SEQ_STATE_1);
    pPlayer->num = num;

    PlayerList* pList = GetList(num);

    if (pList->tail == NULL) {
        pList->head = pPlayer;
        pList->tail = pPlayer;
        pPlayer->next = NULL;
        pPlayer->prev = NULL;
    } else {
        pList->tail->next = pPlayer;
        pPlayer->prev = pList->tail;
        pPlayer->next = NULL;
        pList->tail = pPlayer;
    }
}

void InitAxSound(const void* pSoundData, void* pMem, u32 memSize) {
    if (memSize < sizeof(Work) + THREAD_STACK_SIZE) {
        return;
    }

    if (!AICheckInit()) {
        AIInit(NULL);
        AXInit();
    }

    HBMMIXInit();
    HBMSYNInit();
    HBMSEQInit();

    Work* pWork = static_cast<Work*>(pMem);

    for (int i = 0; i < NUM_SEQ_PLAYER; i++) {
        pWork->seqPlayer[i].used = false;
        pWork->seqPlayer[i].next = NULL;
        pWork->seqPlayer[i].prev = NULL;
    }

    for (int i = 0; i < NUM_SE_PLAYER; i++) {
        pWork->sePlayer[i].used = false;
        pWork->sePlayer[i].next = NULL;
        pWork->sePlayer[i].prev = NULL;
    }

    pWork->wt = NULL;
    pWork->pcm = NULL;
    pWork->stack = reinterpret_cast<u8*>(pWork) + sizeof(Work);
    pWork->seqList.head = NULL;
    pWork->seqList.tail = NULL;
    pWork->seList.head = NULL;
    pWork->seList.tail = NULL;

    if (!ARCInitHandle(const_cast<void*>(pSoundData), &pWork->arc)) {
        return;
    }

    {
        ARCFileInfo info;

        if (!ARCOpen(&pWork->arc, WT_FILENAME, &info)) {
            return;
        }

        pWork->wt = static_cast<u8*>(ARCGetStartAddrInMem(&info));
    }

    {
        ARCFileInfo info;

        if (!ARCOpen(&pWork->arc, PCM_FILENAME, &info)) {
            return;
        }

        pWork->pcm = static_cast<u8*>(ARCGetStartAddrInMem(&info));
    }

    OSInitMessageQueue(&pWork->msgQueue, pWork->msgBuffer, NUM_MESSAGE);

    if (!OSCreateThread(&pWork->thread, &AudioSoundThreadProc, NULL,
                        pWork->stack + memSize - sizeof(Work),
                        memSize - sizeof(Work), THREAD_PRIORITY, 0)) {
        return;
    }

    sWork = pWork;
    OSResumeThread(&pWork->thread);

    pWork->oldCallback = AXRegisterCallback(&AudioFrameCallback);
}

void ShutdownAxSound() {
    if (sWork == NULL) {
        return;
    }

    StopAllSeq();
    AXRegisterCallback(sWork->oldCallback);

    OSJamMessage(&sWork->msgQueue, reinterpret_cast<OSMessage>(MESSAGE_QUIT),
                 OS_MESSAGE_BLOCK);
    OSJoinThread(&sWork->thread, NULL);

    HBMSEQQuit();
    HBMSYNQuit();
    HBMMIXQuit();

    sWork = NULL;
}

void AxSoundMain() {}

void StopAllSeq() {
    if (sWork == NULL) {
        return;
    }

    for (int i = 0; i < NUM_SEQ_PLAYER; i++) {
        Player* pPlayer = &sWork->seqPlayer[i];

        if (pPlayer->used) {
            StopPlayer(pPlayer);
        }
    }

    for (int i = 0; i < NUM_SE_PLAYER; i++) {
        Player* pPlayer = &sWork->sePlayer[i];

        if (pPlayer->used) {
            StopPlayer(pPlayer);
        }
    }
}

void SetVolumeAllSeq(f32 gain) {
    if (sWork == NULL) {
        return;
    }

    // in tenths of a decibel
    int dB;

    if (gain <= 0.0f) {
        dB = -904;
    } else {
        dB = static_cast<int>(10.0f * (20.0f * log10f(gain)));
    }

    if (dB > 60) {
        dB = 60;
    }

    if (dB < -904) {
        dB = -904;
    }

    for (int i = 0; i < NUM_SEQ_PLAYER; i++) {
        Player* pPlayer = &sWork->seqPlayer[i];

        if (pPlayer->used) {
            HBMSEQSetVolume(&pPlayer->seq, dB);
        }
    }

    for (int i = 0; i < NUM_SE_PLAYER; i++) {
        Player* pPlayer = &sWork->sePlayer[i];

        if (pPlayer->used) {
            HBMSEQSetVolume(&pPlayer->seq, dB);
        }
    }
}

void SetSoundMode(u32 mode) {
    HBMMIXSetSoundMode(mode);
}

} // namespace homebutton
