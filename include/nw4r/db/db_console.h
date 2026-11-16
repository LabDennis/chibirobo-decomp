#ifndef NW4R_DB_CONSOLE_H
#define NW4R_DB_CONSOLE_H

#include <nw4r/types_nw4r.h>

#include <cstdarg>

namespace nw4r {
namespace ut {

// Forward declarations
template <typename T> class TextWriterBase;

} // namespace ut

namespace db {
namespace detail {

struct ConsoleHead {
    u8* textBuf;                      // at 0x0
    u16 width;                        // at 0x4
    u16 height;                       // at 0x6
    u16 priority;                     // at 0x8
    u16 attr;                         // at 0xA
    u16 printTop;                     // at 0xC
    u16 printXPos;                    // at 0xE
    u16 ringTop;                      // at 0x10
    s32 ringTopLineCnt;               // at 0x14
    s32 viewTopLine;                  // at 0x18
    s16 viewPosX;                     // at 0x1C
    s16 viewPosY;                     // at 0x1E
    u16 viewLines;                    // at 0x20
    bool isVisible;                   // at 0x22
    ut::TextWriterBase<char>* writer; // at 0x24
    ConsoleHead* next;                // at 0x28
};

} // namespace detail

typedef detail::ConsoleHead* ConsoleHandle;

enum ConsoleOutputType {
    CONSOLE_OUTPUT_NONE = 0,
    CONSOLE_OUTPUT_DISPLAY = 1,
    CONSOLE_OUTPUT_TERMINAL = 2,
    CONSOLE_OUTPUT_ALL = CONSOLE_OUTPUT_DISPLAY | CONSOLE_OUTPUT_TERMINAL
};

void Console_VFPrintf(ConsoleOutputType type, ConsoleHandle console,
                      const char* format, std::va_list vlist);
void Console_Printf(ConsoleHandle console, const char* format, ...);
s32 Console_GetTotalLines(ConsoleHandle console);

inline bool Console_SetVisible(ConsoleHandle console, bool isVisible) {
    bool before = console->isVisible;
    console->isVisible = isVisible;
    return before;
}

inline s32 Console_SetViewBaseLine(ConsoleHandle console, s32 line) {
    s32 before = console->viewTopLine;
    console->viewTopLine = line;
    return before;
}

inline u16 Console_GetViewHeight(ConsoleHandle console) {
    return console->viewLines;
}

inline s32 Console_ShowLatestLine(ConsoleHandle console) {
    s32 baseLine =
        Console_GetTotalLines(console) - Console_GetViewHeight(console);

    if (baseLine < 0) {
        baseLine = 0;
    }

    Console_SetViewBaseLine(console, baseLine);
    return baseLine;
}

} // namespace db
} // namespace nw4r

#endif
