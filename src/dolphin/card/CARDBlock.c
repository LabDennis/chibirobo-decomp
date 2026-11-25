#include <dolphin/card.h>

#include "__card.h"

enum {
    CARD_FAT_BLOCK = 3,
    CARD_FAT_BACKUP_BLOCK = 4,
    CARD_FAT_END = 0xFFFF
};

// prototypes
static void WriteCallback(s32 chan, s32 result);
static void EraseCallback(s32 chan, s32 result);

/**
 * @note Address: 0x80180380
 * @note Size: 0x8
 */
void* __CARDGetFatBlock(CARDControl* card) {
    ASSERTLINE(57, card->currentFat);
    return card->currentFat;
}

/**
 * @note Address: 0x80180388
 * @note Size: 0xD4
 */
static void WriteCallback(s32 chan, s32 result) {
    CARDControl* card;
    CARDCallback callback;
    u16* fat0;
    u16* fat1;

    card = &__CARDBlock[chan];

    if (result >= 0) {
        fat0 = (u16*)((u8*)card->workArea + CARD_FAT_BLOCK * CARD_SYSTEM_BLOCK_SIZE);
        fat1 = (u16*)((u8*)card->workArea + CARD_FAT_BACKUP_BLOCK * CARD_SYSTEM_BLOCK_SIZE);

        ASSERTLINE(82, card->currentFat);
        if (card->currentFat == fat0) {
            card->currentFat = fat1;
            memcpy(fat1, fat0, CARD_SYSTEM_BLOCK_SIZE);
        } else {
            ASSERTLINE(90, card->currentFat == fat1);
            card->currentFat = fat0;
            memcpy(fat0, fat1, CARD_SYSTEM_BLOCK_SIZE);
        }
    }

    if (!card->apiCallback)
        __CARDPutControlBlock(card, result);

    callback = card->eraseCallback;
    if (callback) {
        card->eraseCallback = NULL;
        callback(chan, result);
    }
}

/**
 * @note Address: 0x8018045C
 * @note Size: 0xC8
 */
static void EraseCallback(s32 chan, s32 result) {
    CARDControl* card = &__CARDBlock[chan];
    CARDCallback callback;
    u16* fat;
    u32 addr;

    if (result < 0)
        goto error;

    fat = __CARDGetFatBlock(card);
    addr = ((u32)fat - (u32)card->workArea) / CARD_SYSTEM_BLOCK_SIZE * card->sectorSize;
    result = __CARDWrite(chan, addr, CARD_SYSTEM_BLOCK_SIZE, fat, WriteCallback);
    if (result < 0)
        goto error;

    return;

error:
    if (!card->apiCallback)
        __CARDPutControlBlock(card, result);

    callback = card->eraseCallback;
    if (callback) {
        card->eraseCallback = NULL;
        callback(chan, result);
    }
}

/**
 * @note Address: 0x80180524
 * @note Size: 0x118
 */
s32 __CARDAllocBlock(s32 chan, u32 cBlock, CARDCallback callback) {
    CARDControl* card;
    u16* fat;
    u16 iBlock;
    u16 startBlock;
    u16 prevBlock;
    u16 count;

    ASSERTLINE(182, 0 < cBlock);
    ASSERTLINE(183, 0 <= chan && chan < 2);

    card = &__CARDBlock[chan];
    if (!card->attached)
        return CARD_RESULT_NOCARD;

    fat = __CARDGetFatBlock(card);
    if (fat[CARD_FAT_FREEBLOCKS] < cBlock)
        return CARD_RESULT_INSSPACE;

    fat[CARD_FAT_FREEBLOCKS] -= cBlock;
    startBlock = CARD_FAT_END;
    iBlock = fat[CARD_FAT_LASTSLOT];
    count = 0;
    while (0 < cBlock) {
        if (card->cBlock - CARD_NUM_SYSTEM_BLOCK < ++count)
            return CARD_RESULT_BROKEN;

        iBlock++;
        if (!CARDIsValidBlockNo(card, iBlock))
            iBlock = CARD_NUM_SYSTEM_BLOCK;

        if (fat[iBlock] == CARD_FAT_AVAIL) {
            if (startBlock == CARD_FAT_END)
                startBlock = iBlock;
            else
                fat[prevBlock] = iBlock;
            prevBlock = iBlock;
            fat[iBlock] = CARD_FAT_END;
            --cBlock;
        }
    }

    fat[CARD_FAT_LASTSLOT] = iBlock;
    card->startBlock = startBlock;
    return __CARDUpdateFatBlock(chan, fat, callback);
}

/**
 * @note Address: 0x8018063C
 * @note Size: 0x9C
 */
s32 __CARDFreeBlock(s32 chan, u16 nBlock, CARDCallback callback) {
    CARDControl* card;
    u16* fat;
    u16 nextBlock;

    ASSERTLINE(253, 0 <= chan && chan < 2);

    card = &__CARDBlock[chan];
    if (!card->attached)
        return CARD_RESULT_NOCARD;

    fat = __CARDGetFatBlock(card);
    while (nBlock != CARD_FAT_END) {
        if (!CARDIsValidBlockNo(card, nBlock))
            return CARD_RESULT_BROKEN;

        nextBlock = fat[nBlock];
        fat[nBlock] = CARD_FAT_AVAIL;
        nBlock = nextBlock;
        ++fat[CARD_FAT_FREEBLOCKS];
    }

    return __CARDUpdateFatBlock(chan, fat, callback);
}

/**
 * @note Address: 0x801806D8
 * @note Size: 0xAC
 */
s32 __CARDUpdateFatBlock(s32 chan, u16* fat, CARDCallback callback) {
    CARDControl* card;
    u32 addr;

    ASSERTLINE(295, 0 <= chan && chan < 2);

    card = &__CARDBlock[chan];
    ++fat[CARD_FAT_CHECKCODE];
    __CARDCheckSum(fat + CARD_FAT_CHECKCODE,
                  CARD_SYSTEM_BLOCK_SIZE - CARD_FAT_CHECKCODE * sizeof(*fat),
                  &fat[CARD_FAT_CHECKSUM], &fat[CARD_FAT_CHECKSUMINV]);
    DCStoreRange(fat, CARD_SYSTEM_BLOCK_SIZE);
    card->eraseCallback = callback;
    addr = (((u8*)fat - (u8*)card->workArea) / CARD_SYSTEM_BLOCK_SIZE) * card->sectorSize;
    return __CARDEraseSector(chan, addr, EraseCallback);
}
