#include "game/stg.h"

// the destructors of the menus and of the status menu's value table

/**
 * @note Address: 0x8006331C
 * @note Size: 0x94
 */
MenuStatusTable::~MenuStatusTable()
{
}

/**
 * @note Address: 0x800633B0
 * @note Size: 0x54
 */
MenuStatusEntry::~MenuStatusEntry()
{
}

/**
 * @note Address: 0x80063404
 * @note Size: 0x18
 */
MenuStatusEntry::MenuStatusEntry()
{
}

// The Japanese menu list emits its destructors in a different order.
#if defined(VERSION_JP)
MenuSave::~MenuSave()
{
}
#endif

/**
 * @note Address: 0x8006341C
 * @note Size: 0x90
 */
MenuMCTestSub::~MenuMCTestSub()
{
}

/**
 * @note Address: 0x800634AC
 * @note Size: 0x90
 */
MenuMCTest::~MenuMCTest()
{
}

#if defined(VERSION_JP)
MenuRequestClear::~MenuRequestClear()
{
}
#else
/**
 * @note Address: 0x8006353C
 * @note Size: 0x90
 */
MenuStageSelect::~MenuStageSelect()
{
}

/**
 * @note Address: 0x800635CC
 * @note Size: 0x90
 */
MenuSave::~MenuSave()
{
}
#endif

/**
 * @note Address: 0x8006365C
 * @note Size: 0x90
 */
MenuPC::~MenuPC()
{
}

#if defined(VERSION_JP)
MenuStageSelect::~MenuStageSelect()
{
}

MenuSound::~MenuSound()
{
}

MenuPadControl::~MenuPadControl()
{
}

MenuSettings::~MenuSettings()
{
}

MenuRequest::~MenuRequest()
{
}
#endif

/**
 * @note Address: 0x800636EC
 * @note Size: 0x90
 */
MenuItem::~MenuItem()
{
}

/**
 * @note Address: 0x8006377C
 * @note Size: 0x90
 */
MenuStatus::~MenuStatus()
{
}

/**
 * @note Address: 0x8006380C
 * @note Size: 0x90
 */
MenuMain::~MenuMain()
{
}
