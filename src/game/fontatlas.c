#include "types.h"

// The bytes are extracted from the disc in orig/ at build time (extract in config.yml).
u8 gSingleByteFontAtlas[0x20000] ATTRIBUTE_ALIGN(32) = {
#include "assets/fontatlas.inc"
};
