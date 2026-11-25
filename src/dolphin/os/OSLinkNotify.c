#include <dolphin/os/OSModule.h>

/* The debugger uses these calls to track loaded modules. */
/**
 * @note Address: 0x801676BC
 * @note Size: 0x4
 */
void OSNotifyLink(OSModuleInfo* module) {}

/**
 * @note Address: 0x801676C0
 * @note Size: 0x4
 */
void OSNotifyUnlink(OSModuleInfo* module) {}
