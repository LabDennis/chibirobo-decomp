#include <HBMCommon.h>

#include <HBMAxSound.h>
#include <HBMBase.h>

#include "revolution/mem.h"
#include "revolution/mem_exp_heap.h"
#include "revolution/os.h"

const char* __HBMVersion =
    "<< RVL_SDK - HBM \trelease build: Sep 24 2008 10:40:24 (0x4199_60831) >>";

enum HBMAllocatorType {
    HBM_ALLOCATOR_APPLI, // the application's allocator
    HBM_ALLOCATOR_LOCAL, // a heap in the memory the application passed
    HBM_ALLOCATOR_NW4R,  // the allocator the layout library already has
};

static MEMAllocator sAllocator;
MEMAllocator* spAllocator = &sAllocator;

void* HBMAllocMem(u32 size) {
    return MEMAllocFromAllocator(spAllocator, size);
}

void HBMFreeMem(void* pBlock) {
    MEMFreeToAllocator(spAllocator, pBlock);
}

static HBMAllocatorType getAllocatorType(const HBMDataInfo* pDataInfo) {
    if (pDataInfo->pAllocator != NULL) {
        return HBM_ALLOCATOR_APPLI;
    }

    if (pDataInfo->mem != NULL) {
        return HBM_ALLOCATOR_LOCAL;
    }

    return HBM_ALLOCATOR_NW4R;
}

void HBMCreate(const HBMDataInfo* pDataInfo) {
    MEMiHeapHead* hExpHeap;

    if (getAllocatorType(pDataInfo) == HBM_ALLOCATOR_LOCAL) {
        hExpHeap = MEMCreateExpHeapEx(pDataInfo->mem, pDataInfo->memSize, 0);
        MEMInitAllocatorForExpHeap(&sAllocator, hExpHeap, 32);
        spAllocator = &sAllocator;
    }

    switch (getAllocatorType(pDataInfo)) {
    case HBM_ALLOCATOR_APPLI: {
        nw4hbm::lyt::Layout::SetAllocator(pDataInfo->pAllocator);
        spAllocator = pDataInfo->pAllocator;
        break;
    }

    case HBM_ALLOCATOR_LOCAL: {
        nw4hbm::lyt::Layout::SetAllocator(spAllocator);
        break;
    }

    case HBM_ALLOCATOR_NW4R: {
        spAllocator = nw4hbm::lyt::Layout::GetAllocator();
        break;
    }
    }

    homebutton::HomeButton::createInstance(pDataInfo);
    homebutton::HomeButton::getInstance()->create();
}

void HBMInit() {
    OSRegisterVersion(__HBMVersion);
    homebutton::HomeButton::getInstance()->init();
}

HBMSelectBtnNum HBMCalc(const HBMControllerData* pController) {
    homebutton::HomeButton::getInstance()->calc(pController);
    return homebutton::HomeButton::getInstance()->getSelectBtnNum();
}

void HBMDraw() {
    homebutton::HomeButton::getInstance()->draw();
}

HBMSelectBtnNum HBMGetSelectBtnNum() {
    return homebutton::HomeButton::getInstance()->getSelectBtnNum();
}

void HBMSetAdjustFlag(BOOL flag) {
    homebutton::HomeButton::getInstance()->setAdjustFlag(flag);
}

void HBMStartBlackOut() {
    homebutton::HomeButton::getInstance()->startBlackOut();
}

void HBMCreateSound(void* pSoundData, void* pMemBuf, u32 memSize) {
    homebutton::HomeButton::getInstance();
    homebutton::InitAxSound(pSoundData, pMemBuf, memSize);
}

void HBMDeleteSound() {
    homebutton::ShutdownAxSound();
}

void HBMUpdateSound() {
    homebutton::HomeButton::getInstance()->update_sound();
}
