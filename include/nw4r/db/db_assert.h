#ifndef NW4R_DB_ASSERT_H
#define NW4R_DB_ASSERT_H

#include <nw4r/types_nw4r.h>

#include <cstdarg>

namespace nw4r
{
    namespace db
    {
        // Forward declarations
        namespace detail {
            class ConsoleHead;
        }

        #define NW4R_WARNING(...) nw4r::db::Warning(__FILE__, __LINE__, __VA_ARGS__)
        #define NW4R_PANIC(...) nw4r::db::Panic(__FILE__, __LINE__, __VA_ARGS__)
        #define NW4R_DB_ASSERT(exp, ...) \
            if(!(exp))                \
            nw4r::db::Panic(__FILE__, __LINE__, __VA_ARGS__)

        // Final builds of the libraries carry no warnings: retail nw4r::ef inlines math::FSqrt as
        // `x <= 0 ? 0 : x * FrSqrt(x)` with no call to db::Warning (0x80247824, 0x80250FC8).
        // NW4R_ASSERT is the switch NW4RAssert.hpp uses.
        #if defined(NW4R_ASSERT)
        #define NW4R_DB_ASSERT_WARN(exp, ...) \
            if(!(exp))                \
            nw4r::db::Warning(__FILE__, __LINE__, __VA_ARGS__)
        #define NW4R_ASSERT_PTR_FILELINE(file_, line_, x) \
            ((void)(((((u32)(x) & 0xff000000) == 0x80000000) || \
                (((u32)(x) & 0xff800000) == 0x81000000) || \
                (((u32)(x) & 0xf8000000) == 0x90000000) || \
                (((u32)(x) & 0xff000000) == 0xc0000000) || \
                (((u32)(x) & 0xff800000) == 0xc1000000) || \
                (((u32)(x) & 0xf8000000) == 0xd0000000) || \
                (((u32)(x) & 0xffffc000) == 0xe0000000)) || \
                (nw4r::db::Panic(file_, line_, "NW4R:Pointer Error\n" #x "(=%p) is not valid pointer.", (x)), 0)))
        #define NW4R_ASSERT_PTR(x) NW4R_ASSERT_PTR_FILELINE(__FILE__, __LINE__, x)
        #else
        #define NW4R_DB_ASSERT_WARN(exp, ...) ((void)0)
        #define NW4R_ASSERT_PTR_FILELINE(file_, line_, x) ((void)0)
        #define NW4R_ASSERT_PTR(x) ((void)0)
        #endif

        /* DECL_WEAK */ void VPanic(const char* file, int line, const char* fmt, std::va_list vlist, bool halt);
        /* DECL_WEAK */ void Panic(const char* file, int line, const char* fmt, ...);
        /* DECL_WEAK */ void VWarning(const char* file, int line, const char* fmt, std::va_list vlist);
        /* DECL_WEAK */ void Warning(const char* file, int line, const char* msg, ...);
        
        namespace detail
        {
            void Log(const char* fmt, ...);
        }
        
        detail::ConsoleHead* Assertion_SetConsole(detail::ConsoleHead* console);
        detail::ConsoleHead* Assertion_GetConsole();
        void Assertion_ShowConsole(u32 time);
        void Assertion_HideConsole();
        void Assertion_SetWarningTime(u32 time);
        bool Assertion_SetAutoWarning(bool enable);
    }
}

#endif
