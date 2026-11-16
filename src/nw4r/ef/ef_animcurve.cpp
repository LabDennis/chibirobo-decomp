#include "nw4r/ef.h"

#include <cmath>



namespace nw4r {
namespace ef {

namespace {

// Key of a float curve with N components
template <int N> struct AnimCurveKeyF32 {
    u16 frame;       // at 0x0
    u16 interp;      // at 0x2, two bits per component
    u16 PADDING_0x4; // at 0x4
    u8 random;       // at 0x6
    u8 PADDING_0x7;  // at 0x7
    union {
        f32 value[N]; // at 0x8
        u16 rangeIdx; // at 0x8
    };
};

// Random range of a float curve with N components
template <int N> struct AnimCurveRangeF32 {
    struct {
        f32 base;  // at 0x0
        f32 width; // at 0x4
    } comp[N];
};

// Random range of a rotation curve
struct AnimCurveRangeRotate {
    struct {
        f32 base;  // at 0x0
        f32 width; // at 0x4
    } comp[3];
    u8 randomSign;     // at 0x18
    u8 PADDING_0x19[3]; // at 0x19
};

// Key of a byte curve with N components
template <int N> struct AnimCurveKeyU8 {
    u16 frame;       // at 0x0
    u16 interp;      // at 0x2, two bits per component
    u16 PADDING_0x4; // at 0x4
    u8 random;       // at 0x6
    u8 PADDING_0x7;  // at 0x7
    union {
        u8 value[N];  // at 0x8
        u16 rangeIdx; // at 0x8
    };
};

struct AnimCurveRangeCompU8 {
    u8 base;  // at 0x0
    u8 width; // at 0x1, scaled by a signed random in [-1, 1)
};

// Random range of a byte curve with N components
template <int N> struct AnimCurveRangeU8 {
    AnimCurveRangeCompU8 comp[N];
};

// Key of a texture curve
struct AnimCurveKeyTexture {
    u16 frame;          // at 0x0
    u8 random;          // at 0x2
    u8 PADDING_0x3[9];  // at 0x3
    union {
        struct {
            u8 wrap;    // at 0xC
            u8 reverse; // at 0xD
        };
        u16 rangeIdx; // at 0xC
    };
    u16 name; // at 0xE
};

struct AnimCurveRangeTexture {
    u8 wrap;          // at 0x0
    u8 reverse;       // at 0x1
    u16 name;         // at 0x2
    u8 randomReverse; // at 0x4
    u8 PADDING_0x5[3]; // at 0x5
};

struct AnimCurveRandomTexture {
    u8 wrap;    // at 0x0
    u8 reverse; // at 0x1
    u16 name;   // at 0x2
};

// Child created by a child curve
struct AnimCurveChild {
    EmitterInheritSetting setting; // at 0x0
    u16 resIdx;                    // at 0x8, index into the name table
};

// Key of a child curve
struct AnimCurveKeyChild {
    u16 frame;          // at 0x0
    u8 random;          // at 0x2
    u8 PADDING_0x3[9];  // at 0x3
    union {
        AnimCurveChild child; // at 0xC
        u16 rangeIdx;         // at 0xC
    };
};

template <typename T> struct AnimCurveTable {
    u16 count;       // at 0x0
    u16 PADDING_0x2; // at 0x2
    T datas[1];      // at 0x4
};

enum InterpType {
    INTERP_LINEAR,
    INTERP_SMOOTH,
    INTERP_STEP,
};

enum KeyRandomFlag {
    KEY_RANDOM_FLAG_TABLE = (1 << 1),
};


// Number of set bits of a byte: the number of components a curve animates
u8 sBitCount[256] = {
    0x00, 0x01, 0x01, 0x02, 0x01, 0x02, 0x02, 0x03, 0x01, 0x02, 0x02, 0x03, 0x02, 0x03, 0x03, 0x04,
    0x01, 0x02, 0x02, 0x03, 0x02, 0x03, 0x03, 0x04, 0x02, 0x03, 0x03, 0x04, 0x03, 0x04, 0x04, 0x05,
    0x01, 0x02, 0x02, 0x03, 0x02, 0x03, 0x03, 0x04, 0x02, 0x03, 0x03, 0x04, 0x03, 0x04, 0x04, 0x05,
    0x02, 0x03, 0x03, 0x04, 0x03, 0x04, 0x04, 0x05, 0x03, 0x04, 0x04, 0x05, 0x04, 0x05, 0x05, 0x06,
    0x01, 0x02, 0x02, 0x03, 0x02, 0x03, 0x03, 0x04, 0x02, 0x03, 0x03, 0x04, 0x03, 0x04, 0x04, 0x05,
    0x02, 0x03, 0x03, 0x04, 0x03, 0x04, 0x04, 0x05, 0x03, 0x04, 0x04, 0x05, 0x04, 0x05, 0x05, 0x06,
    0x02, 0x03, 0x03, 0x04, 0x03, 0x04, 0x04, 0x05, 0x03, 0x04, 0x04, 0x05, 0x04, 0x05, 0x05, 0x06,
    0x03, 0x04, 0x04, 0x05, 0x04, 0x05, 0x05, 0x06, 0x04, 0x05, 0x05, 0x06, 0x05, 0x06, 0x06, 0x07,
    0x01, 0x02, 0x02, 0x03, 0x02, 0x03, 0x03, 0x04, 0x02, 0x03, 0x03, 0x04, 0x03, 0x04, 0x04, 0x05,
    0x02, 0x03, 0x03, 0x04, 0x03, 0x04, 0x04, 0x05, 0x03, 0x04, 0x04, 0x05, 0x04, 0x05, 0x05, 0x06,
    0x02, 0x03, 0x03, 0x04, 0x03, 0x04, 0x04, 0x05, 0x03, 0x04, 0x04, 0x05, 0x04, 0x05, 0x05, 0x06,
    0x03, 0x04, 0x04, 0x05, 0x04, 0x05, 0x05, 0x06, 0x04, 0x05, 0x05, 0x06, 0x05, 0x06, 0x06, 0x07,
    0x02, 0x03, 0x03, 0x04, 0x03, 0x04, 0x04, 0x05, 0x03, 0x04, 0x04, 0x05, 0x04, 0x05, 0x05, 0x06,
    0x03, 0x04, 0x04, 0x05, 0x04, 0x05, 0x05, 0x06, 0x04, 0x05, 0x05, 0x06, 0x05, 0x06, 0x06, 0x07,
    0x03, 0x04, 0x04, 0x05, 0x04, 0x05, 0x05, 0x06, 0x04, 0x05, 0x05, 0x06, 0x05, 0x06, 0x06, 0x07,
    0x04, 0x05, 0x05, 0x06, 0x05, 0x06, 0x06, 0x07, 0x05, 0x06, 0x06, 0x07, 0x06, 0x07, 0x07, 0x08,
};

// Highest set bit of a byte: the mask of the first component
u8 sTopBit[256] = {
    0x00, 0x01, 0x02, 0x02, 0x04, 0x04, 0x04, 0x04, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
    0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
    0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
    0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
    0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
};

// A texture curve names its target by the offset of the texture in ParticleParameter
enum {
    AC_TARGET_TEXTURE1 = offsetof(ParticleParameter, mTexture[TEX_LAYER_1]),
    AC_TARGET_TEXTURE2 = offsetof(ParticleParameter, mTexture[TEX_LAYER_2]),
    AC_TARGET_TEXTUREIND = offsetof(ParticleParameter, mTexture[TEX_LAYER_IND]),
};

} // namespace

// Integer frame of the step curves (texture and child)
// rEnd is set on the last tick of an even number of turns.
static inline void CalcFrameStep(u8* pCmdList, u32 tick, u32 life,
                                 u16& rFrame, u32& rLoop, bool& rEnd) {
    AnimCurveHeader* pHeader = reinterpret_cast<AnimCurveHeader*>(pCmdList);
    u8 flag = pHeader->processFlag;
    rLoop = 0;
    rEnd = false;

    if (!(flag & AnimCurveHeader::PROC_FLAG_INFLOOP) &&
        pHeader->loopCount <= 1) {

        if (!(flag & AnimCurveHeader::PROC_FLAG_FITTING)) {
            if (tick >= pHeader->frameLength) {
                rFrame = pHeader->frameLength;
                return;
            }
            rFrame = tick;
            return;
        }

        rFrame = pHeader->frameLength * tick / life;
        return;
    }

    if (!(flag & AnimCurveHeader::PROC_FLAG_FITTING)) {
        if (pHeader->frameLength <= 1 || !(flag & AnimCurveHeader::PROC_FLAG_TURN)) {
            rLoop = tick / pHeader->frameLength;

            if (!(flag & AnimCurveHeader::PROC_FLAG_INFLOOP) &&
                rLoop >= pHeader->loopCount) {
                rLoop = static_cast<u8>(pHeader->loopCount - 1);
                rFrame = pHeader->frameLength;
                return;
            }

            rFrame = tick - rLoop * pHeader->frameLength;
            return;
        }

        rLoop = tick / (pHeader->frameLength - 1);

        if (!(flag & AnimCurveHeader::PROC_FLAG_INFLOOP) &&
            rLoop >= pHeader->loopCount) {
            if (pHeader->loopCount % 2 == 0) {
                if (rLoop == pHeader->loopCount &&
                    tick - rLoop * (pHeader->frameLength - 1) == 0) {
                    rEnd = true;
                }
                rFrame = 0;
            } else {
                rFrame = pHeader->frameLength - 1;
            }
            rLoop = static_cast<u8>(pHeader->loopCount - 1);
            return;
        }

        if (rLoop % 2 == 0) {
            rFrame = tick - rLoop * (pHeader->frameLength - 1);
            return;
        }
        rFrame = (pHeader->frameLength - 1) - (tick - rLoop * (pHeader->frameLength - 1));
        return;
    }

    if (tick >= life) {
        if (!(flag & AnimCurveHeader::PROC_FLAG_TURN) ||
            pHeader->loopCount % 2 != 0) {
            rFrame = pHeader->frameLength;
        } else {
            rFrame = 0;
        }
        rLoop = static_cast<u8>(pHeader->loopCount - 1);
        return;
    }

    if (pHeader->frameLength <= 1 || !(flag & AnimCurveHeader::PROC_FLAG_TURN)) {
        f32 scale = pHeader->loopCount *
                    (static_cast<f32>(pHeader->frameLength) / static_cast<f32>(life));

        u32 loop = tick * scale / pHeader->frameLength;
        rLoop = loop;
        rFrame = tick * scale - loop * pHeader->frameLength;
        return;
    }

    f32 scale =
        (1.0f + static_cast<f32>(static_cast<int>(pHeader->frameLength - 1)) * pHeader->loopCount) / life;
    u32 pos = tick * scale;
    // The original converts tick here too (dead): the int-to-float scratch slots of the next
    // conversion alternate as if it existed (0x8/0x10).
    f32 fTick = tick;
    rLoop = pos / (pHeader->frameLength - 1);

    if (rLoop >= pHeader->loopCount) {
        if (pHeader->loopCount % 2 == 0) {
            if (tick == life - 1) {
                rEnd = true;
            }
            rFrame = 0;
        } else {
            rFrame = pHeader->frameLength - 1;
        }
        rLoop = static_cast<u8>(pHeader->loopCount - 1);
        return;
    }

    if (rLoop % 2 == 0) {
        rFrame = pos - rLoop * (pHeader->frameLength - 1);
        return;
    }
    rFrame = (pHeader->frameLength - 1) * (rLoop + 1) - pos;
}

// Step curves: binary search for the frame. With lowerBound set this finds
// the first key at or after the frame, otherwise the last key at or before it.
template <typename TKey>
static inline int SearchKeyStep(TKey* pKeys, u16 numKey, int frame,
                                bool lowerBound) {
    int hi = numKey - 1;
    int lo = 0;
    int mid = hi / 2;
    int firstFrame = pKeys[0].frame;
    if (frame < firstFrame || (frame == firstFrame && lowerBound)) {
        return 0;
    }

    if (pKeys[hi].frame < frame ||
        (frame == pKeys[hi].frame && !lowerBound)) {
        return hi;
    }

    int midFrame = pKeys[mid].frame;

    while (lo < mid) {
        if (frame == midFrame) {
            if (lowerBound) {
                hi = mid;
            } else {
                lo = mid;
            }
        } else {
            if (midFrame < frame) {
                lo = mid;
            }
            if (midFrame >= frame) {
                hi = mid;
            }
        }

        mid = (lo + hi) / 2;
        midFrame = pKeys[mid].frame;
    }

    return lo;
}

// Frame of a key in a table with variable sized keys
static inline u16 KeyFrame(u8* pKeys, int idx, int keySize) {
    return *reinterpret_cast<u16*>(pKeys + idx * keySize);
}

// Time of a looping curve stretched over the particle life
static inline f32 CalcFittingLoopTime(AnimCurveHeader* pHeader, u8 flag,
                                      u32 tick, u32 lastTick, u32& rLoop) {
    f32 scale = pHeader->loopCount *
                (static_cast<f32>(static_cast<int>((pHeader->frameLength - 1))) / lastTick);

    u32 loop = tick * scale / static_cast<int>((pHeader->frameLength - 1));
    rLoop = loop;

    f32 time = tick * scale - loop * (pHeader->frameLength - 1);

    if (!(flag & AnimCurveHeader::PROC_FLAG_TURN)) {
        return time;
    }

    if (loop % 2 == 0) {
        return time;
    }

    return static_cast<f32>(static_cast<int>((pHeader->frameLength - 1))) - time;
}

// Converts the particle tick to a curve time (loop, turn and fitting modes)
static inline f32 CalcTime(AnimCurveHeader* pHeader, u32 tick, u32 life,
                           u16& rFrame, u32& rLoop) {
    u16 frameLength = pHeader->frameLength;
    rLoop = 0;
    f32 time;
    // Last tick of the particle life, shared by the fitting-loop test and CalcFittingLoopTime.
    // Declared here and assigned at the test: the original's register colouring shows a
    // front-end value for life - 1 (CalcTime's flag is coloured last only if it exists).
    u32 lastTick;

    if (frameLength <= 1) {
        time = 0.0f;
        rLoop = tick;
        rFrame = 0;
    } else {
        u8 flag = pHeader->processFlag;

        if (!(flag & AnimCurveHeader::PROC_FLAG_INFLOOP) &&
            pHeader->loopCount <= 1) {

            if (!(flag & AnimCurveHeader::PROC_FLAG_FITTING)) {
                rFrame = tick;
                if (tick >= frameLength - 1) {
                    rFrame = frameLength - 1;
                }
                time = rFrame;
            } else {
                if (life == 1) {
                    time = static_cast<f32>(frameLength - 1);
                } else {
                    time = tick * (static_cast<f32>(frameLength - 1) / (life - 1));

                    if (time > static_cast<f32>(frameLength - 1)) {
                        time = static_cast<f32>(frameLength - 1);
                    }
                }

                rFrame = time;
            }
        } else if (!(flag & AnimCurveHeader::PROC_FLAG_FITTING)) {
            rLoop = tick / (frameLength - 1);

            if (!(flag & AnimCurveHeader::PROC_FLAG_TURN)) {
                if (!(flag & AnimCurveHeader::PROC_FLAG_INFLOOP) &&
                    rLoop >= pHeader->loopCount) {
                    rFrame = (frameLength - 1);
                    rLoop = static_cast<u8>(pHeader->loopCount - 1);
                } else {
                    rFrame = tick - rLoop * (frameLength - 1);
                }
            } else {
                if (!(flag & AnimCurveHeader::PROC_FLAG_INFLOOP) &&
                    rLoop >= pHeader->loopCount) {
                    if (pHeader->loopCount % 2 == 0) {
                        rFrame = 0;
                    } else {
                        rFrame = (frameLength - 1);
                    }
                    rLoop = static_cast<u8>(pHeader->loopCount - 1);
                } else if (rLoop % 2 == 0) {
                    rFrame = tick - rLoop * (frameLength - 1);
                } else {
                    rFrame = (frameLength - 1) * (rLoop + 1) - tick;
                }
            }

            time = rFrame;
        } else {
            lastTick = life - 1;
            if (tick >= lastTick) {
                if (!(flag & AnimCurveHeader::PROC_FLAG_TURN) ||
                    pHeader->loopCount % 2 != 0) {
                    rFrame = frameLength - 1;
                } else {
                    rFrame = 0;
                }

                time = rFrame;
                rLoop = static_cast<u8>(pHeader->loopCount - 1);
            } else {
                time = CalcFittingLoopTime(pHeader, flag, tick, lastTick, rLoop);
                rFrame = time;
            }
        }
    }


    return time;
}

// Turning curves play the keys backwards on odd loops, so the random seed of
// the key on the far side belongs to the neighbouring loop.
static inline void AdjustTurnLoop(AnimCurveHeader* pHeader,
                                  AnimCurveKey* pKeyTbl, int idx,
                                  u32 loop, u32& rLoop0, u32& rLoop1) {
    u8 flag = pHeader->processFlag;

    if (flag & AnimCurveHeader::PROC_FLAG_TURN) {
        if ((flag & AnimCurveHeader::PROC_FLAG_INFLOOP) ||
            pHeader->loopCount > 1) {

            if (!(loop & 1) && idx + 1 >= pKeyTbl->count - 1) {
                if ((flag & AnimCurveHeader::PROC_FLAG_INFLOOP) ||
                    loop < pHeader->loopCount - 1) {
                    rLoop1 = loop + 1;
                }
            }

            if ((loop & 1) && idx == 0 && loop != 0) {
                rLoop0 = loop + 1;
            }
        }
    }
}

static inline bool SearchKeyStride(AnimCurveTable<u8>* pKeyTbl, u8* pKeys, int keySize,
                                   int frame,
                             f32 time, int& rIdx, u16& rPrevFrame,
                             u16& rNextFrame) {
    bool exact;
    int lo;
    int hi;
    int mid;

    lo = 0;
    hi = pKeyTbl->count - 1;
    mid = hi / 2;

    // Retail narrows the result of fabs (fabs, frsp): the C library's fabsf, which
    // include/rvl_msl only declares. fabsl is its inline with the same code.
    exact =
        static_cast<f32>(fabsl(static_cast<f32>(frame) - time)) < NW4R_MATH_FLT_EPSILON;

    rPrevFrame = KeyFrame(pKeys, 0, keySize);

    if (frame < KeyFrame(pKeys, 0, keySize)) {
        rIdx = 0;
        return true;
    }

    if (frame == KeyFrame(pKeys, 0, keySize)) {
        rIdx = 0;

        if (hi == 0) {
            return true;
        }

        if (!exact) {
            rNextFrame = KeyFrame(pKeys, 1, keySize);
        }
        return exact;
    }

    rNextFrame = KeyFrame(pKeys, hi, keySize);
    if (rNextFrame <= frame) {
        rIdx = hi;
        return true;
    }

    int midFrame = KeyFrame(pKeys, mid, keySize);

    while (lo < mid) {
        if (frame == midFrame) {
            rIdx = mid;

            if (!exact) {
                rPrevFrame = midFrame;
                rNextFrame = KeyFrame(pKeys, mid + 1, keySize);
            }
            return exact;
        }

        if (midFrame < frame) {
            lo = mid;
            rPrevFrame = midFrame;
        } else {
            hi = mid;
            rNextFrame = midFrame;
        }

        mid = (lo + hi) / 2;
        midFrame = KeyFrame(pKeys, mid, keySize);
    }

    rIdx = lo;
    return false;
}


static inline u32 MixRandom(u32 rnd) {
    return rnd * 0x343FD + 0x269EC3;
}

// Random seed of a key, folded byte by byte
static inline u32 CalcRandomSeed(u16 seed, AnimCurveHeader* pHeader, u32 loop,
                                 u32 rangeIdx) {
    union {
        u32 value;
        u8 bytes[4];
    } rndSeed;

    // A flat sum with a u32 rangeIdx, as in doldecomp/forecast (CC0): the order of the adds
    // in retail depends on it.
    rndSeed.value = seed * 0x3F81F635 + pHeader->randomSeed * 0x30A74193 +
                    rangeIdx * 0x371097E7 + loop * 0x7B929 + 0x4BF53;

    rndSeed.bytes[2] ^= rndSeed.bytes[3];
    rndSeed.bytes[1] ^= rndSeed.bytes[2];
    rndSeed.bytes[0] ^= rndSeed.bytes[1];

    return rndSeed.value;
}

static inline u8* GetRandomRangeU8(AnimCurveHeader* pHeader,
                                   AnimCurveTable<u8>* pRangeTbl,
                                   AnimCurveTable<u8>* pRandomTbl,
                                   int rangeSize, u8 random, int rangeIdx,
                                   u32 loop, u16 seed, u32& rRnd) {
    u32 rnd = CalcRandomSeed(seed, pHeader, loop, rangeIdx);
    u8* pRange;

    if (!(random & KEY_RANDOM_FLAG_TABLE)) {
        pRange = pRangeTbl->datas + rangeIdx * rangeSize;
    } else {
        pRange = pRandomTbl->datas + (rnd >> 16) % pRandomTbl->count * rangeSize;
        rnd = MixRandom(rnd);
    }

    rRnd = rnd;
    return pRange;
}

// Base plus width scaled by a signed random in [-1, 1). The caller clamps the sum and
// advances the range: the original keeps the base load and the clamped sum apart.
static inline int RandomValueU8(const u8* pRange, u32 rnd) {
    return pRange[0] + pRange[1] * static_cast<s16>(rnd >> 16) / 32768;
}

// Next random value, unless the components share one
static inline void UpdateRandomU8(AnimCurveHeader* pHeader, u32& rRnd) {
    if (!(pHeader->processFlag & AnimCurveHeader::PROC_FLAG_SYNC)) {
        rRnd = MixRandom(rRnd);
    }
}

static inline u8 InterpolateU8(u8 interp, u32 t, u8 v0, u8 v1) {
    if (v0 == v1) {
        return v0;
    }

    switch (interp & 3) {
    case INTERP_LINEAR: {
        return v0 + ((t * (v1 - v0)) >> 16);
    }
    case INTERP_SMOOTH: {
        f32 ft = t / 65536.0f;

        if (interp == 1) {
            return v0 + ft * (ft * ((3.0f - 2.0f * ft) * (v1 - v0)));
        }

        f32 easeOut;
        f32 easeIn;
        if ((interp >> 2 & 1) == 0) {
            easeIn = 0.0f;
        } else {
            easeIn = 1.5f;
        }
        if ((interp >> 3 & 1) == 0) {
            easeOut = 0.0f;
        } else {
            easeOut = 1.5f;
        }

        f32 value =
            v0 + ft * (v1 - v0) *
                     (easeOut + (ft * (ft * (easeOut + easeIn - 2.0f)) +
                                 ft * (3.0f + (-2.0f * easeOut - easeIn))));

        if (value < 0.0f) {
            return 0;
        } else if (value > 255.0f) {
            return 255;
        } else {
            return value;
        }
    }
    case INTERP_STEP: {
        return v0;
    }
    default: {
        return 0;
    }
    }
}

// Not referenced; the linker strips them. Compiled first, they give .sdata2
// its constant order.
f32 AnimCurveCalcTime(u8* pCmdList, u32 tick, u32 life) {
    u16 frame;
    u32 loop;
    return CalcTime(reinterpret_cast<AnimCurveHeader*>(pCmdList), tick, life,
                    frame, loop);
}

u16 AnimCurveCalcFrameStep(u8* pCmdList, u32 tick, u32 life) {
    u32 loop;
    bool end;
    u16 frame;
    CalcFrameStep(pCmdList, tick, life, frame, loop, end);
    return frame;
}

// NONMATCHING: retail's code (0x12F8 bytes both) but for registers and the order of a few
// instructions. Retail keeps seed r31, pTarget r30, pHeader r29, rangeSize r28, then enable r25,
// top r24, keySize r23 and the key, range and random tables r22-r20, loop r19, idx r18; ours
// gives the first registers to enable and the tables. The two key products of the interpolation
// setup (retail 0x8023A168) are also scheduled the other way round, as they were in the 2007
// file. Tried: declarations at first use (worse), a search over the order of the declaration
// block (230 -> 216 differing lines of 1214, the order below), the key path as an inline behind
// the baked test (far worse), const key pointers (far worse), rangeSize as const int& (same).
void AnimCurveExecuteU8(u8* pCmdList, u8* pTarget, u32 tick, u16 seed,
                        u32 life) {
    bool exact;
    u8 enable;
    bool isRandom1;
    u16 rangeIdx0;
    u16 rangeIdx1;
    bool isRandom0;
    AnimCurveTable<u8>* pRangeTbl;
    int keySize;
    u8* pKey0;
    f32 time;
    AnimCurveHeader* pHeader;
    AnimCurveTable<u8>* pKeyTbl;
    AnimCurveTable<u8>* pRandomTbl;
    u8* pValue0;
    u32 t;
    int num;
    u8* pKey1;
    u8* pValue1;
    u16 frame;
    u32 loop;
    int idx;
    u8 top;
    u8* pInterp;
    u8 random0;
    u16 prevFrame;
    u16 nextFrame;
    u32 loop1;
    u8* pKeys;
    pHeader = reinterpret_cast<AnimCurveHeader*>(pCmdList);

    if (pHeader->magic == NW4R_EF_MAGIC_ANIMBAKED) {
        AnimCurveExecuteBakedU8(pCmdList, pTarget, tick, life);
        return;
    }
    enable = pHeader->kindEnable;

    num = sBitCount[enable];
    top = sTopBit[enable];

    if (num == 0) {
        return;
    }

    keySize = (num + 13) & ~1;

    pKeyTbl =
        reinterpret_cast<AnimCurveTable<u8>*>(pCmdList + sizeof(AnimCurveHeader));
    pRangeTbl = reinterpret_cast<AnimCurveTable<u8>*>(
        reinterpret_cast<u8*>(pKeyTbl) + pHeader->keyTable);
    pRandomTbl = reinterpret_cast<AnimCurveTable<u8>*>(
        reinterpret_cast<u8*>(pRangeTbl) + pHeader->rangeTable);
    int rangeSize = num * 2;

    time = CalcTime(pHeader, tick, life, frame, loop);

    pKeys = pKeyTbl->datas;
    exact = SearchKeyStride(pKeyTbl, pKeys, keySize, frame, time, idx,
                                 prevFrame, nextFrame);

    if (exact) {
        u8* pKey = pKeys + idx * keySize;
        u8 random = pKey[2];
        u8* pValue = pKey + 12;

        if (random == 0) {
            for (u16 bit = 1; bit <= top; bit <<= 1) {
                if (enable & bit) {
                    *pTarget = *pValue++;
                }
                pTarget++;
            }
        } else {
            u32 rnd;
            u8* pRange = GetRandomRangeU8(pHeader, pRangeTbl, pRandomTbl,
                                          rangeSize, random,
                                          *reinterpret_cast<u16*>(pValue),
                                          loop, seed, rnd);

            for (u16 bit = 1; bit <= top; bit <<= 1) {
                if (enable & bit) {
                    int value = RandomValueU8(pRange, rnd);
                    pRange += 2;
                    if (value < 0) {
                        value = 0;
                    }
                    if (value > 255) {
                        value = 255;
                    }
                    *pTarget = value;
                }
                UpdateRandomU8(pHeader, rnd);
                pTarget++;
            }
        }
        return;
    }

    loop1 = loop;
    AdjustTurnLoop(pHeader, reinterpret_cast<AnimCurveKey*>(pKeyTbl), idx, loop, loop,
                   loop1);

    t = static_cast<u32>(65536.0f * (time - prevFrame)) /
            (nextFrame - prevFrame);

    pKey0 = pKeys + idx * keySize;
    pKey1 = pKeys + (idx + 1) * keySize;

    pValue0 = pKey0 + 12;
    pInterp = pKey0 + 4;
    pValue1 = pKey1 + 12;
    random0 = pKey0[2];
    isRandom0 = random0 != 0;
    isRandom1 = pKey1[2] != 0;

    if (!isRandom0 && !isRandom1) {
        for (u16 bit = 1; bit <= top; bit <<= 1) {
            if (enable & bit) {
                *pTarget = InterpolateU8(*pInterp, t, *pValue0, *pValue1);
                pValue0++;
                pValue1++;
            }
            pInterp++;
            pTarget++;
        }
    } else if (isRandom0 && !isRandom1) {
        u16 rangeIdx = *reinterpret_cast<u16*>(pValue0);
        u32 rnd;
        u8* pRange = GetRandomRangeU8(pHeader, pRangeTbl, pRandomTbl,
                                      rangeSize, random0,
                                      rangeIdx, loop,
                                      seed, rnd);

        for (u16 bit = 1; bit <= top; bit <<= 1) {
            if (enable & bit) {
                int v0 = RandomValueU8(pRange, rnd);
                pRange += 2;
                if (v0 < 0) {
                    v0 = 0;
                }
                if (v0 > 255) {
                    v0 = 255;
                }
                *pTarget = InterpolateU8(*pInterp, t, v0, *pValue1);
                pValue1++;
            }
            UpdateRandomU8(pHeader, rnd);
            pInterp++;
            pTarget++;
        }
    } else if (!isRandom0 && isRandom1) {
        u32 rnd;
        u8* pRange = GetRandomRangeU8(pHeader, pRangeTbl, pRandomTbl,
                                      rangeSize, pKey1[2],
                                      *reinterpret_cast<u16*>(pValue1), loop1,
                                      seed, rnd);

        for (u16 bit = 1; bit <= top; bit <<= 1) {
            if (enable & bit) {
                int v1 = RandomValueU8(pRange, rnd);
                pRange += 2;
                if (v1 < 0) {
                    v1 = 0;
                }
                if (v1 > 255) {
                    v1 = 255;
                }
                *pTarget = InterpolateU8(*pInterp, t, *pValue0, v1);
                pValue0++;
            }
            UpdateRandomU8(pHeader, rnd);
            pInterp++;
            pTarget++;
        }
    } else {
        u32 rnd0;
        rangeIdx0 = *reinterpret_cast<u16*>(pValue0);
        u8* pRange0 = GetRandomRangeU8(pHeader, pRangeTbl, pRandomTbl,
                                       rangeSize, random0,
                                       rangeIdx0, loop,
                                       seed, rnd0);
        u32 rnd1;
        rangeIdx1 = *reinterpret_cast<u16*>(pValue1);
        u8* pRange1 = GetRandomRangeU8(pHeader, pRangeTbl, pRandomTbl,
                                       rangeSize, pKey1[2],
                                       rangeIdx1, loop1,
                                       seed, rnd1);

        for (u16 bit = 1; bit <= top; bit <<= 1) {
            if (enable & bit) {
                int v0 = RandomValueU8(pRange0, rnd0);
                pRange0 += 2;
                if (v0 < 0) {
                    v0 = 0;
                }
                if (v0 > 255) {
                    v0 = 255;
                }
                UpdateRandomU8(pHeader, rnd0);
                int v1 = RandomValueU8(pRange1, rnd1);
                pRange1 += 2;
                if (v1 < 0) {
                    v1 = 0;
                }
                if (v1 > 255) {
                    v1 = 255;
                }
                UpdateRandomU8(pHeader, rnd1);
                *pTarget = InterpolateU8(*pInterp, t, v0, v1);
            } else if (!(pHeader->processFlag & AnimCurveHeader::PROC_FLAG_SYNC)) {
                rnd0 = MixRandom(rnd0);
                rnd1 = MixRandom(rnd1);
            }
            pInterp++;
            pTarget++;
        }
    }
}

// Random value in [base, base + width * 0xFFFF]
static inline f32 CalcRandomValue(f32 base, f32 width, u32 rnd) {
    return base + width * (rnd >> 16);
}

// The random value of one component's range (base, width)
static inline f32 RandomValueF32(const f32* pRange, u32 rnd) {
    return CalcRandomValue(pRange[0], pRange[1], rnd);
}

static inline f32 CalcRandomF32(AnimCurveHeader* pHeader, f32*& rpRange,
                                u32& rRnd) {
    f32 value = CalcRandomValue(rpRange[0], rpRange[1], rRnd);
    rpRange += 2;

    if (!(pHeader->processFlag & AnimCurveHeader::PROC_FLAG_SYNC)) {
        rRnd = MixRandom(rRnd);
    }

    return value;
}

static inline f32 InterpolateF32(u8 interp, f32 t, f32 v0, f32 v1) {
    if (v0 == v1) {
        return v0;
    }

    switch (interp & 3) {
    case INTERP_LINEAR: {
        return v0 + t * (v1 - v0);
    }
    case INTERP_SMOOTH: {
        if (interp == 1) {
            return v0 + t * (t * ((3.0f - 2.0f * t) * (v1 - v0)));
        }

        f32 easeIn;
        f32 easeOut;
        if ((interp >> 3 & 1) == 0) {
            easeOut = 0.0f;
        } else {
            easeOut = 1.5f;
        }
        if ((interp >> 2 & 1) == 0) {
            easeIn = 0.0f;
        } else {
            easeIn = 1.5f;
        }

        return v0 + t * (v1 - v0) *
                        (easeOut + (t * (t * (easeOut + easeIn - 2.0f)) +
                                    t * (3.0f + (-2.0f * easeOut - easeIn))));
    }
    case INTERP_STEP: {
        return v0;
    }
    default: {
        return 0.0f;
    }
    }
}

// NONMATCHING: retail's code (0xFEC bytes both) but for registers and instruction order in 124
// of 1019 lines: `exact` and the search's lower bound are in r22/r28 the wrong way round,
// the addresses of the two bit tables in r3/r4 likewise, and the key products of the
// interpolation setup (retail 0x8023B458) are scheduled as in AnimCurveExecuteU8. The order
// of the declaration block was searched (412 -> 124); nothing else was tried.
void AnimCurveExecuteF32(u8* pCmdList, f32* pTarget, u32 tick, u16 seed,
                         u32 life) {
    u8 random1;
    u8* pInterp;
    u8 enable;
    AnimCurveTable<u8>* pKeyTbl;
    f32 time;
    int num;
    AnimCurveTable<u8>* pRangeTbl;
    bool exact;
    u8* pKey1;
    AnimCurveTable<u8>* pRandomTbl;
    u32 loop;
    u8 random0;
    u16 frame;
    AnimCurveHeader* pHeader;
    u8* pKey0;
    u16 nextFrame;
    u32 loop1;
    bool isRandom1;
    u16 rangeIdx1;
    u8 top;
    f32 v0;
    f32 v1;
    f32 t;
    f32* pValue1;
    f32* pValue0;
    bool isRandom0;
    u8* pKeys;
    int keySize;
    int idx;
    u16 prevFrame;
    u16 rangeIdx;
    u16 rangeIdx0;
    u32 rnd0;
    f32* pRange0;
    u32 rnd1;
    f32* pRange1;
    pHeader = reinterpret_cast<AnimCurveHeader*>(pCmdList);

    if (pHeader->magic == NW4R_EF_MAGIC_ANIMBAKED) {
        AnimCurveExecuteBakedF32(pCmdList, pTarget, tick, life);
        return;
    }

    pKeyTbl =
        reinterpret_cast<AnimCurveTable<u8>*>(pCmdList + sizeof(AnimCurveHeader));
    pRangeTbl = reinterpret_cast<AnimCurveTable<u8>*>(
        reinterpret_cast<u8*>(pKeyTbl) + pHeader->keyTable);
    pRandomTbl = reinterpret_cast<AnimCurveTable<u8>*>(
        reinterpret_cast<u8*>(pRangeTbl) + pHeader->rangeTable);

    enable = pHeader->kindEnable;

    time = CalcTime(pHeader, tick, life, frame, loop);

    num = sBitCount[enable];
    top = sTopBit[enable];

    if (num == 0) {
        return;
    }

    keySize = num * 4 + 12;
    int rangeSize = num * 8;

    pKeys = pKeyTbl->datas;
    exact = SearchKeyStride(pKeyTbl, pKeys, keySize, frame, time, idx,
                                 prevFrame, nextFrame);

    if (exact) {
        u8* pKey = pKeys + idx * keySize;
        u8 random = pKey[2];
        f32* pValue = reinterpret_cast<f32*>(pKey + 12);

        if (random == 0) {
            for (u16 bit = 1; bit <= top; bit <<= 1) {
                if (enable & bit) {
                    *pTarget = *pValue++;
                }
                pTarget++;
            }
        } else {
            u32 rnd;
            f32* pRange = reinterpret_cast<f32*>(GetRandomRangeU8(
                pHeader, pRangeTbl, pRandomTbl, rangeSize, random,
                *reinterpret_cast<u16*>(pValue), loop, seed, rnd));

            for (u16 bit = 1; bit <= top; bit <<= 1) {
                if (enable & bit) {
                    *pTarget = RandomValueF32(pRange, rnd);
                    pRange += 2;
                    if (!(pHeader->processFlag & AnimCurveHeader::PROC_FLAG_SYNC)) {
                        rnd = MixRandom(rnd);
                    }
                }
                pTarget++;
            }
        }
        return;
    }

    loop1 = loop;
    AdjustTurnLoop(pHeader, reinterpret_cast<AnimCurveKey*>(pKeyTbl), idx, loop,
                   loop, loop1);

    t = (time - prevFrame) / (nextFrame - prevFrame);

    pKey0 = pKeys + idx * keySize;
    pKey1 = pKeys + (idx + 1) * keySize;

    pValue0 = reinterpret_cast<f32*>(pKey0 + 12);
    pInterp = pKey0 + 4;
    pValue1 = reinterpret_cast<f32*>(pKey1 + 12);
    random0 = pKey0[2];
    random1 = pKey1[2];
    isRandom0 = random0 != 0;
    isRandom1 = random1 != 0;

    if (!isRandom0 && !isRandom1) {
        for (u16 bit = 1; bit <= top; bit <<= 1) {
            if (enable & bit) {
                *pTarget = InterpolateF32(*pInterp, t, *pValue0, *pValue1);
                pValue0++;
                pValue1++;
            }
            pInterp++;
            pTarget++;
        }
    } else if (isRandom0 && !isRandom1) {
        u32 rnd;
        rangeIdx = *reinterpret_cast<u16*>(pValue0);
        f32* pRange = reinterpret_cast<f32*>(GetRandomRangeU8(
            pHeader, pRangeTbl, pRandomTbl, rangeSize, random0,
            rangeIdx, loop, seed, rnd));

        for (u16 bit = 1; bit <= top; bit <<= 1) {
            if (enable & bit) {
                v0 = CalcRandomF32(pHeader, pRange, rnd);
                *pTarget = InterpolateF32(*pInterp, t, v0, *pValue1);
                pValue1++;
            }
            pInterp++;
            pTarget++;
        }
    } else if (!isRandom0 && isRandom1) {
        u32 rnd;
        rangeIdx = *reinterpret_cast<u16*>(pValue1);
        f32* pRange = reinterpret_cast<f32*>(GetRandomRangeU8(
            pHeader, pRangeTbl, pRandomTbl, rangeSize, random1,
            rangeIdx, loop1, seed, rnd));

        for (u16 bit = 1; bit <= top; bit <<= 1) {
            if (enable & bit) {
                v1 = CalcRandomF32(pHeader, pRange, rnd);
                *pTarget = InterpolateF32(*pInterp, t, *pValue0, v1);
                pValue0++;
            }
            pInterp++;
            pTarget++;
        }
    } else {
        rangeIdx0 = *reinterpret_cast<u16*>(pValue0);
        pRange0 = reinterpret_cast<f32*>(GetRandomRangeU8(
            pHeader, pRangeTbl, pRandomTbl, rangeSize, random0,
            rangeIdx0, loop, seed, rnd0));
        rangeIdx1 = *reinterpret_cast<u16*>(pValue1);
        pRange1 = reinterpret_cast<f32*>(GetRandomRangeU8(
            pHeader, pRangeTbl, pRandomTbl, rangeSize, random1,
            rangeIdx1, loop1, seed, rnd1));

        for (u16 bit = 1; bit <= top; bit <<= 1) {
            if (enable & bit) {
                v0 = CalcRandomF32(pHeader, pRange0, rnd0);
                v1 = CalcRandomF32(pHeader, pRange1, rnd1);
                *pTarget = InterpolateF32(*pInterp, t, v0, v1);
            }
            pInterp++;
            pTarget++;
        }
    }
}

// Random sign of a rotation range: the byte behind the last component
static inline void GetRandomSign(u8* pRange, int signOfs, u32& rRnd,
                                 bool& rNegate) {
    rNegate = false;

    if (pRange[signOfs]) {
        if (!(rRnd & 0x10000)) {
            rNegate = true;
        }
        rRnd = MixRandom(rRnd);
    }
}

// NONMATCHING: retail's code (0x10F8 bytes both) but for registers and instruction order: retail
// saves r21-r31 and keeps tick in r31, ours saves r22-r31. The order of the declaration
// block was searched (627 -> 406 differing lines of 1086); nothing else was tried.
// In 2009 a rotation curve has a key per enabled component like the float curve; the sign
// byte of a range sits behind its last component.
void AnimCurveExecuteRotate(u8* pCmdList, f32* pTarget, u32 tick, u16 seed,
                            u32 life) {
    AnimCurveTable<u8>* pRangeTbl;
    AnimCurveHeader* pHeader;
    AnimCurveTable<u8>* pKeyTbl;
    bool isRandom1;
    u32 loop;
    AnimCurveTable<u8>* pRandomTbl;
    bool exact;
    u16 frame;
    u16 rangeIdx1;
    f32 time;
    u8 top;
    bool isRandom0;
    u8 random1;
    u32 loop1;
    f32 v0;
    f32 v1;
    u16 prevFrame;
    f32 t;
    u8* pKey0;
    u8* pKey1;
    f32* pValue1;
    u16 nextFrame;
    f32* pValue0;
    int idx;
    u8 enable;
    int num;
    int keySize;
    u8 random0;
    u8* pKeys;
    u16 rangeIdx;
    u8* pInterp;
    u16 rangeIdx0;
    pHeader = reinterpret_cast<AnimCurveHeader*>(pCmdList);

    pKeyTbl =
        reinterpret_cast<AnimCurveTable<u8>*>(pCmdList + sizeof(AnimCurveHeader));
    pRangeTbl = reinterpret_cast<AnimCurveTable<u8>*>(
        reinterpret_cast<u8*>(pKeyTbl) + pHeader->keyTable);
    pRandomTbl = reinterpret_cast<AnimCurveTable<u8>*>(
        reinterpret_cast<u8*>(pRangeTbl) + pHeader->rangeTable);

    enable = pHeader->kindEnable;

    time = CalcTime(pHeader, tick, life, frame, loop);

    num = sBitCount[enable];
    top = sTopBit[enable];

    if (num == 0) {
        return;
    }

    keySize = num * 4 + 12;
    int signOfs = num * 8;
    int rangeSize = signOfs + 4;

    pKeys = pKeyTbl->datas;
    exact = SearchKeyStride(pKeyTbl, pKeys, keySize, frame, time, idx,
                            prevFrame, nextFrame);

    if (exact) {
        u8* pKey = pKeys + idx * keySize;
        u8 random = pKey[2];
        f32* pValue = reinterpret_cast<f32*>(pKey + 12);

        if (random == 0) {
            for (u16 bit = 1; bit <= top; bit <<= 1) {
                if (enable & bit) {
                    *pTarget = *pValue++;
                }
                pTarget++;
            }
        } else {
            u32 rnd;
            u8* pRange = GetRandomRangeU8(pHeader, pRangeTbl, pRandomTbl,
                                          rangeSize, random,
                                          *reinterpret_cast<u16*>(pValue),
                                          loop, seed, rnd);
            bool negate;
            GetRandomSign(pRange, signOfs, rnd, negate);
            f32* pRangeValue = reinterpret_cast<f32*>(pRange);

            for (u16 bit = 1; bit <= top; bit <<= 1) {
                if (enable & bit) {
                    f32 value = RandomValueF32(pRangeValue, rnd);
                    pRangeValue += 2;
                    if (negate) {
                        value = -value;
                    }
                    *pTarget = value;
                }
                pTarget++;
                if (!(pHeader->processFlag & AnimCurveHeader::PROC_FLAG_SYNC)) {
                    rnd = MixRandom(rnd);
                }
            }
        }
        return;
    }

    loop1 = loop;
    AdjustTurnLoop(pHeader, reinterpret_cast<AnimCurveKey*>(pKeyTbl), idx, loop,
                   loop, loop1);

    t = (time - prevFrame) / (nextFrame - prevFrame);

    pKey0 = pKeys + idx * keySize;
    pKey1 = pKeys + (idx + 1) * keySize;

    pValue0 = reinterpret_cast<f32*>(pKey0 + 12);
    pInterp = pKey0 + 4;
    pValue1 = reinterpret_cast<f32*>(pKey1 + 12);
    random0 = pKey0[2];
    random1 = pKey1[2];
    isRandom0 = random0 != 0;
    isRandom1 = random1 != 0;

    if (!isRandom0 && !isRandom1) {
        for (u16 bit = 1; bit <= top; bit <<= 1) {
            if (enable & bit) {
                *pTarget = InterpolateF32(*pInterp, t, *pValue0, *pValue1);
                pValue0++;
                pValue1++;
            }
            pInterp++;
            pTarget++;
        }
    } else if (isRandom0 && !isRandom1) {
        u32 rnd;
        rangeIdx = *reinterpret_cast<u16*>(pValue0);
        u8* pRange = GetRandomRangeU8(pHeader, pRangeTbl, pRandomTbl,
                                      rangeSize, random0, rangeIdx, loop,
                                      seed, rnd);
        f32* pRangeValue = reinterpret_cast<f32*>(pRange);
        bool negate;
        GetRandomSign(pRange, signOfs, rnd, negate);

        for (u16 bit = 1; bit <= top; bit <<= 1) {
            if (enable & bit) {
                v0 = RandomValueF32(pRangeValue, rnd);
                pRangeValue += 2;
                if (negate) {
                    v0 = -v0;
                }
                *pTarget = InterpolateF32(*pInterp, t, v0, *pValue1);
                pValue1++;
            }
            pInterp++;
            pTarget++;
            if (!(pHeader->processFlag & AnimCurveHeader::PROC_FLAG_SYNC)) {
                rnd = MixRandom(rnd);
            }
        }
    } else if (!isRandom0 && isRandom1) {
        u32 rnd;
        rangeIdx = *reinterpret_cast<u16*>(pValue1);
        u8* pRange = GetRandomRangeU8(pHeader, pRangeTbl, pRandomTbl,
                                      rangeSize, random1, rangeIdx, loop1,
                                      seed, rnd);
        f32* pRangeValue = reinterpret_cast<f32*>(pRange);
        bool negate;
        GetRandomSign(pRange, signOfs, rnd, negate);

        for (u16 bit = 1; bit <= top; bit <<= 1) {
            if (enable & bit) {
                v1 = RandomValueF32(pRangeValue, rnd);
                pRangeValue += 2;
                if (negate) {
                    v1 = -v1;
                }
                *pTarget = InterpolateF32(*pInterp, t, *pValue0, v1);
                pValue0++;
            }
            pInterp++;
            pTarget++;
            if (!(pHeader->processFlag & AnimCurveHeader::PROC_FLAG_SYNC)) {
                rnd = MixRandom(rnd);
            }
        }
    } else {
        rangeIdx0 = *reinterpret_cast<u16*>(pValue0);
        u32 rnd0;
        u8* pRange0 = GetRandomRangeU8(pHeader, pRangeTbl, pRandomTbl,
                                       rangeSize, random0, rangeIdx0, loop,
                                       seed, rnd0);
        u32 rnd1;
        rangeIdx1 = *reinterpret_cast<u16*>(pValue1);
        u8* pRange1 = GetRandomRangeU8(pHeader, pRangeTbl, pRandomTbl,
                                       rangeSize, random1, rangeIdx1, loop1,
                                       seed, rnd1);
        bool negate0;
        GetRandomSign(pRange0, signOfs, rnd0, negate0);
        bool negate1;
        GetRandomSign(pRange1, signOfs, rnd1, negate1);
        f32* pRangeValue0 = reinterpret_cast<f32*>(pRange0);
        f32* pRangeValue1 = reinterpret_cast<f32*>(pRange1);

        for (u16 bit = 1; bit <= top; bit <<= 1) {
            if (enable & bit) {
                v0 = RandomValueF32(pRangeValue0, rnd0);
                pRangeValue0 += 2;
                if (negate0) {
                    v0 = -v0;
                }
                v1 = RandomValueF32(pRangeValue1, rnd1);
                pRangeValue1 += 2;
                if (negate1) {
                    v1 = -v1;
                }
                *pTarget = InterpolateF32(*pInterp, t, v0, v1);
            }
            pInterp++;
            pTarget++;
            if (!(pHeader->processFlag & AnimCurveHeader::PROC_FLAG_SYNC)) {
                rnd0 = MixRandom(rnd0);
                rnd1 = MixRandom(rnd1);
            }
        }
    }
}


// NONMATCHING: retail's code (0x5D4 bytes both) but for three registers: retail has wrap
// in r8 (the register that held the frame), reverse in r9 and layer * 4 in r10; ours wrap
// in r9, reverse in r10 and layer * 4 in r8. The order of the three assignments is
// retail's; the order of the declarations (every single move was compiled) changes nothing.
void AnimCurveExecuteTexture(u8* pCmdList, Particle* pParticle, u32 tick,
                             u16 seed, u32 life) {
    AnimCurveHeader* pHeader = reinterpret_cast<AnimCurveHeader*>(pCmdList);

    AnimCurveTable<AnimCurveKeyTexture>* pKeyTbl =
        reinterpret_cast<AnimCurveTable<AnimCurveKeyTexture>*>(
            reinterpret_cast<u8*>(pHeader) + sizeof(AnimCurveHeader));
    AnimCurveTable<AnimCurveRangeTexture>* pRangeTbl =
        reinterpret_cast<AnimCurveTable<AnimCurveRangeTexture>*>(
            reinterpret_cast<u8*>(pKeyTbl) + pHeader->keyTable);
    AnimCurveTable<AnimCurveRandomTexture>* pRandomTbl =
        reinterpret_cast<AnimCurveTable<AnimCurveRandomTexture>*>(
            reinterpret_cast<u8*>(pRangeTbl) + pHeader->rangeTable);
    AnimCurveTable<TextureData*>* pNameTbl =
        reinterpret_cast<AnimCurveTable<TextureData*>*>(
            reinterpret_cast<u8*>(pRandomTbl) + pHeader->randomTable);

    u32 loop;
    bool end;
    u16 frame;
    int layer;
    TextureData** ppTexture;
    AnimCurveKeyTexture* pKeys;
    AnimCurveKeyTexture* pKey;
    u16 name;
    u8 wrap;
    u8 reverse;

    CalcFrameStep(reinterpret_cast<u8*>(pHeader), tick, life, frame, loop, end);


    switch (pHeader->kindType) {
    case AC_TARGET_TEXTURE1: {
        ppTexture = &pParticle->mParameter.mTexture[TEX_LAYER_1];
        layer = TEX_LAYER_1;
        break;
    }
    case AC_TARGET_TEXTURE2: {
        ppTexture = &pParticle->mParameter.mTexture[TEX_LAYER_2];
        layer = TEX_LAYER_2;
        break;
    }
    case AC_TARGET_TEXTUREIND: {
        ppTexture = &pParticle->mParameter.mTexture[TEX_LAYER_IND];
        layer = TEX_LAYER_IND;
        break;
    }
    default: {
        return;
    }
    }

    pKeys = pKeyTbl->datas;
    pKey =
        &pKeys[SearchKeyStep(pKeys, pKeyTbl->count, frame, false)];


    if (pKey->random == 0) {
        wrap = pKey->wrap;
        reverse = pKey->reverse;
        name = pKey->name;
    } else {
        u16 rangeIdx = pKey->rangeIdx;
        u32 rnd = CalcRandomSeed(seed, pHeader, loop, rangeIdx);

        if (!(pKey->random & KEY_RANDOM_FLAG_TABLE)) {
            AnimCurveRangeTexture* pRange = &pRangeTbl->datas[rangeIdx];

            wrap = pRange->wrap;
            reverse = pRange->reverse;
            name = pRange->name;

            switch (pRange->randomReverse) {
            case 1: {
                reverse = (reverse & 2) | ((rnd >> 16) & 1);
                break;
            }
            case 2: {
                reverse = (reverse & 1) | ((rnd >> 16) & 2);
                break;
            }
            case 3: {
                reverse = (rnd >> 16) & 3;
                break;
            }
            }
        } else {
            AnimCurveRandomTexture* pRandom =
                &pRandomTbl->datas[(rnd >> 16) % pRandomTbl->count];

            wrap = pRandom->wrap;
            reverse = pRandom->reverse;
            name = pRandom->name;
        }
    }

    *ppTexture = pNameTbl->datas[name];

    pParticle->mParameter.mTextureWrap =
        static_cast<u16>(pParticle->mParameter.mTextureWrap & ~(0xF << layer * 4)) |
        ((wrap & 0xF) << layer * 4);
    pParticle->mParameter.mTextureReverse =
        static_cast<u8>(pParticle->mParameter.mTextureReverse & ~(3 << layer * 2)) |
        ((reverse & 3) << layer * 2);
}

void createChild(u8* pKeyData, u16 seed, AnimCurveHeader* pHeader,
                 AnimCurveNameTable* pNameTbl,
                 AnimCurveRandomTable* pRandomTbl, Particle* pParticle,
                 u32 loop) {
    AnimCurveKeyChild* pKey = reinterpret_cast<AnimCurveKeyChild*>(pKeyData);
    AnimCurveChild* pChild;

    if (pKey->random == 0) {
        pChild = &pKey->child;
    } else {
        union {
            u32 value;
            u8 bytes[4];
        } rndSeed;

        u32 rangeIdx = pKey->rangeIdx;
        rndSeed.value = seed * 0x3F81F635 + pHeader->randomSeed * 0x30A74193 +
                        rangeIdx * 0x371097E7 + loop * 0x7B929 + 0x4BF53;

        rndSeed.bytes[2] ^= rndSeed.bytes[3];
        rndSeed.bytes[1] ^= rndSeed.bytes[2];
        rndSeed.bytes[0] ^= rndSeed.bytes[1];

        // @bug pChild is left unset for keys with a random range
        if (pKey->random & KEY_RANDOM_FLAG_TABLE) {
            u16 count = pRandomTbl->count;
            if (count == 0) {
                return;
            }

            pChild = &reinterpret_cast<AnimCurveChild*>(
                pRandomTbl->datas)[(rndSeed.value >> 16) % count];
        }
    }

    EmitterResource* pResource = reinterpret_cast<EmitterResource*>(
        pNameTbl->datas[pChild->resIdx].work);

    if (pResource == NULL) {
        return;
    }

    if (pChild->setting.type == 0) {
        pParticle->mParticleManager->mManagerEM->mManagerEF->mManagerES
            ->mCreationQueue.AddParticleCreation(&pChild->setting, pParticle,
                                                 pResource,
                                                 pParticle->mCalcRemain, NULL, NULL);
    } else {
        pParticle->mParticleManager->mManagerEM->mManagerEF->mManagerES
            ->mCreationQueue.AddEmitterCreation(&pChild->setting, pParticle,
                                                pResource,
                                                pParticle->mCalcRemain, NULL, NULL);
    }
}

// NONMATCHING: three instructions. Retail narrows frame1 to 16 bits once, before the
// loop0 == loop1 test (0x8023DBBC), and compares frame0 against that; ours narrows both at
// the comparison. Tried with the 2007 file: the comparison turned round, frame1 in a second
// local, the frame test ahead of the loop test, a u16-returning CalcFrameStep; all worse.
void AnimCurveExecuteChild(u8* pCmdList, Particle* pParticle, u32 tick,
                           u16 seed, u32 life) {
    AnimCurveHeader* pHeader;
    AnimCurveTable<AnimCurveKeyChild>* pKeyTbl;
    u8* pRangeTbl;
    AnimCurveRandomTable* pRandomTbl;
    AnimCurveNameTable* pNameTbl;
    u16 frame0;
    u32 loop0;
    u16 frame1;
    u32 loop1;
    bool end1;
    AnimCurveKeyChild* pKeys;
    AnimCurveKeyChild* pKey;
    bool end;
    int i;

    pHeader = reinterpret_cast<AnimCurveHeader*>(pCmdList);

    pKeyTbl =
        reinterpret_cast<AnimCurveTable<AnimCurveKeyChild>*>(
            reinterpret_cast<u8*>(pHeader) + sizeof(AnimCurveHeader));
    pRangeTbl = reinterpret_cast<u8*>(pKeyTbl) + pHeader->keyTable;
    pRandomTbl =
        reinterpret_cast<AnimCurveRandomTable*>(pRangeTbl +
                                                pHeader->rangeTable);
    pNameTbl = reinterpret_cast<AnimCurveNameTable*>(
        reinterpret_cast<u8*>(pRandomTbl) + pHeader->randomTable);

    // Children are created for the keys passed between this tick and the
    // next one
    CalcFrameStep(reinterpret_cast<u8*>(pHeader), tick, life, frame0, loop0, end);
    CalcFrameStep(reinterpret_cast<u8*>(pHeader), tick + 1, life, frame1, loop1, end1);

    pKeys = pKeyTbl->datas;

    if (loop0 == loop1) {
        if (frame0 == frame1) {
            if (!end) {
                return;
            }
            frame1 = 1;
        }

        if (frame0 < frame1) {
            for (i = SearchKeyStep(pKeys, pKeyTbl->count, frame0, true); i < pKeyTbl->count; i++) {
                if (pKeys[i].frame >= frame0) {
                    if (frame1 <= pKeys[i].frame) {
                        return;
                    }

                    createChild(reinterpret_cast<u8*>(&pKeys[i]), seed, pHeader,
                                pNameTbl, pRandomTbl, pParticle, loop0);
                }
            }
        } else {
            for (i = SearchKeyStep(pKeys, pKeyTbl->count, frame0, false); i >= 0; i--) {
                if (frame0 >= pKeys[i].frame) {
                    if (pKeys[i].frame <= frame1) {
                        return;
                    }

                    createChild(reinterpret_cast<u8*>(&pKeys[i]), seed, pHeader,
                                pNameTbl, pRandomTbl, pParticle, loop0);
                }
            }
        }
        return;
    }

    if (pHeader->processFlag & AnimCurveHeader::PROC_FLAG_TURN) {
        // Rest of the current loop
        if (!(loop0 & 1)) {
            i = SearchKeyStep(pKeys, pKeyTbl->count, frame0, true);
            pKey = &pKeys[i];

            for (; i < pKeyTbl->count; pKey++, i++) {
                if (pKey->frame >= frame0) {
                    if (pKey->frame == pHeader->frameLength - 1) {
                        break;
                    }

                    createChild(reinterpret_cast<u8*>(pKey), seed, pHeader,
                                pNameTbl, pRandomTbl, pParticle, loop0);
                }
            }
        } else {
            for (i = SearchKeyStep(pKeys, pKeyTbl->count, frame0, false); i >= 0; i--) {
                if (frame0 >= pKeys[i].frame) {
                    if (pKeys[i].frame == 0) {
                        break;
                    }

                    createChild(reinterpret_cast<u8*>(&pKeys[i]), seed, pHeader,
                                pNameTbl, pRandomTbl, pParticle, loop0);
                }
            }
        }

        // Whole loops in between
        for (u32 loop = loop0 + 1; loop < loop1; loop++) {
            if (!(loop0 & 1)) {
                pKey = pKeys;

                for (i = 0; i < pKeyTbl->count; pKey++, i++) {
                    if (pKey->frame == pHeader->frameLength - 1) {
                        break;
                    }

                    createChild(reinterpret_cast<u8*>(pKey), seed, pHeader,
                                pNameTbl, pRandomTbl, pParticle, loop0);
                }
            } else {
                i = pKeyTbl->count - 1;
                pKey = &pKeys[i];

                for (; i >= 0; pKey--, i--) {
                    if (pKey->frame == 0) {
                        break;
                    }

                    createChild(reinterpret_cast<u8*>(pKey), seed, pHeader,
                                pNameTbl, pRandomTbl, pParticle, loop0);
                }
            }
        }

        // Start of the next loop
        if (!(loop1 & 1)) {
            for (i = 0; i < pKeyTbl->count; i++) {
                if (frame1 <= pKeys[i].frame) {
                    return;
                }

                createChild(reinterpret_cast<u8*>(&pKeys[i]), seed, pHeader,
                            pNameTbl, pRandomTbl, pParticle, loop0);
            }
        } else {
            i = pKeyTbl->count - 1;
            pKey = &pKeys[i];

            for (; i >= 0; pKey--, i--) {
                if (frame1 >= pKey->frame) {
                    return;
                }

                createChild(reinterpret_cast<u8*>(pKey), seed, pHeader,
                            pNameTbl, pRandomTbl, pParticle, loop0);
            }
        }
    } else {
        for (i = SearchKeyStep(pKeys, pKeyTbl->count, frame0, true); i < pKeyTbl->count; i++) {
            if (pKeys[i].frame >= frame0) {
                createChild(reinterpret_cast<u8*>(&pKeys[i]), seed, pHeader,
                            pNameTbl, pRandomTbl, pParticle, loop0);
            }
        }

        for (u32 loop = loop0 + 1; loop < loop1; loop++) {
            pKey = pKeys;

            for (i = 0; i < pKeyTbl->count; pKey++, i++) {
                createChild(reinterpret_cast<u8*>(pKey), seed, pHeader,
                            pNameTbl, pRandomTbl, pParticle, loop0);
            }
        }

        for (i = 0; i < pKeyTbl->count; i++) {
            if (frame1 <= pKeys[i].frame) {
                return;
            }

            createChild(reinterpret_cast<u8*>(&pKeys[i]), seed, pHeader, pNameTbl,
                        pRandomTbl, pParticle, loop0);
        }
    }
}


// Frame of a baked curve: the tick with the loop, turn and fitting modes applied
static inline u16 CalcBakedFrame(AnimCurveHeader* pHeader, u32 tick, u32 life) {
    u16 frame;

    if (pHeader->frameLength <= 1) {
        frame = 0;
    } else {
        u8 flag = pHeader->processFlag;

        if (!(flag & AnimCurveHeader::PROC_FLAG_INFLOOP) &&
            pHeader->loopCount <= 1) {

            if (!(flag & AnimCurveHeader::PROC_FLAG_FITTING)) {
                if (tick >= pHeader->frameLength - 1) {
                    frame = pHeader->frameLength - 1;
                } else {
                    frame = tick;
                }
            } else if (life == 1) {
                frame = pHeader->frameLength - 1;
            } else {
                f32 time =
                    tick * (static_cast<f32>(pHeader->frameLength - 1) / (life - 1));

                if (time > static_cast<f32>(pHeader->frameLength - 1)) {
                    time = static_cast<f32>(pHeader->frameLength - 1);
                }

                frame = time;
            }
        } else if (!(flag & AnimCurveHeader::PROC_FLAG_FITTING)) {
            u32 loop = tick / (pHeader->frameLength - 1);

            if (!(flag & AnimCurveHeader::PROC_FLAG_TURN)) {
                if (!(flag & AnimCurveHeader::PROC_FLAG_INFLOOP) &&
                    loop >= pHeader->loopCount) {
                    frame = pHeader->frameLength - 1;
                } else {
                    frame = tick - loop * (pHeader->frameLength - 1);
                }
            } else {
                if (!(flag & AnimCurveHeader::PROC_FLAG_INFLOOP) &&
                    loop >= pHeader->loopCount) {
                    if (pHeader->loopCount % 2 == 0) {
                        frame = 0;
                    } else {
                        frame = pHeader->frameLength - 1;
                    }
                } else if (loop % 2 == 0) {
                    frame = tick - loop * (pHeader->frameLength - 1);
                } else {
                    frame = (pHeader->frameLength - 1) * (loop + 1) - tick;
                }
            }
        } else {
            u32 lastTick = life - 1;

            if (tick >= lastTick) {
                if (!(flag & AnimCurveHeader::PROC_FLAG_TURN) ||
                    pHeader->loopCount % 2 != 0) {
                    frame = pHeader->frameLength - 1;
                } else {
                    frame = 0;
                }
            } else {
                f32 scale = pHeader->loopCount *
                            (static_cast<f32>(static_cast<int>(pHeader->frameLength - 1)) /
                             lastTick);
                u32 loop = tick * scale / static_cast<int>(pHeader->frameLength - 1);
                f32 time = tick * scale - loop * (pHeader->frameLength - 1);

                if (!(flag & AnimCurveHeader::PROC_FLAG_TURN)) {
                    frame = time;
                } else {
                    if (loop % 2 == 0) {
                        frame = time;
                    } else {
                        frame = static_cast<f32>(static_cast<int>(
                                    pHeader->frameLength - 1)) -
                                time;
                    }
                }
            }
        }
    }

    return frame;
}

// Baked curves hold one value per enabled component and frame
void AnimCurveExecuteBakedU8(u8* pCmdList, u8* pTarget, u32 tick, u32 life) {
    AnimCurveHeader* pHeader = reinterpret_cast<AnimCurveHeader*>(pCmdList);
    u8 enable = pHeader->kindEnable;
    u8 top;
    int num;

    num = sBitCount[enable];
    top = sTopBit[enable];

    if (num == 0) {
        return;
    }

    u8* pFrames = pCmdList + sizeof(AnimCurveHeader);
    u8* pValue = pFrames + num * CalcBakedFrame(pHeader, tick, life);

    for (u16 bit = 1; bit <= top; bit <<= 1) {
        if (enable & bit) {
            *pTarget = *pValue++;
        }
        pTarget++;
    }
}

void AnimCurveExecuteBakedF32(u8* pCmdList, f32* pTarget, u32 tick, u32 life) {
    AnimCurveHeader* pHeader = reinterpret_cast<AnimCurveHeader*>(pCmdList);
    u8 enable = pHeader->kindEnable;
    u8 top;
    int num;

    num = sBitCount[enable];
    top = sTopBit[enable];

    if (num == 0) {
        return;
    }

    f32* pFrames = reinterpret_cast<f32*>(pCmdList + sizeof(AnimCurveHeader));
    f32* pValue = pFrames + num * CalcBakedFrame(pHeader, tick, life);

    for (u16 bit = 1; bit <= top; bit <<= 1) {
        if (enable & bit) {
            *pTarget = *pValue++;
        }
        pTarget++;
    }
}

} // namespace ef
} // namespace nw4r
