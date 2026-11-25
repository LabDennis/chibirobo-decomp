#include "Dolphin/dsp.h"

/**
 * @note Address: 0x8017D010
 * @note Size: 0x50
 */
void __DSP_debug_printf(const char* format, ...)
{
	// DSP tracing is disabled in the release library.
}
