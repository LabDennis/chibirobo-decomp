#ifndef NW4HBM_UT_TEXT_WRITER_BASE_H
#define NW4HBM_UT_TEXT_WRITER_BASE_H
#include <nw4hbm/types_nw4hbm.h>

#include <nw4hbm/ut/ut_CharWriter.h>
#include <nw4hbm/ut/ut_TagProcessorBase.h>

#include <nw4hbm/math.h>

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <cwchar>

namespace nw4hbm {
namespace ut {

template <typename T> class TextWriterBase : public CharWriter {
public:
    typedef TagProcessorBase<T> TagProcessorType;

public:
    enum DrawFlag {
        // Align text lines
        DRAWFLAG_ALIGN_TEXT_BASELINE = 0,
        DRAWFLAG_ALIGN_TEXT_CENTER = (1 << 0),
        DRAWFLAG_ALIGN_TEXT_RIGHT = (1 << 1),

        // Align text block (horizontal)
        DRAWFLAG_ALIGN_H_BASELINE = 0,
        DRAWFLAG_ALIGN_H_CENTER = (1 << 4),
        DRAWFLAG_ALIGN_H_RIGHT = (1 << 5),

        // Align text block (vertical)
        DRAWFLAG_ALIGN_V_BASELINE = 0,
        DRAWFLAG_ALIGN_V_CENTER = (1 << 8),
        DRAWFLAG_ALIGN_V_TOP = (1 << 9),

        // Mask constants
        DRAWFLAG_MASK_ALIGN_TEXT = DRAWFLAG_ALIGN_TEXT_BASELINE |
                                   DRAWFLAG_ALIGN_TEXT_CENTER |
                                   DRAWFLAG_ALIGN_TEXT_RIGHT,

        DRAWFLAG_MASK_ALIGN_H = DRAWFLAG_ALIGN_H_BASELINE |
                                DRAWFLAG_ALIGN_H_CENTER |
                                DRAWFLAG_ALIGN_H_RIGHT,

        DRAWFLAG_MASK_ALIGN_V = DRAWFLAG_ALIGN_V_BASELINE |
                                DRAWFLAG_ALIGN_V_CENTER | DRAWFLAG_ALIGN_V_TOP,
    };

public:
    TextWriterBase();
    ~TextWriterBase();

    void SetLineHeight(f32 lineHeight);
    f32 GetLineHeight() const;

    void SetLineSpace(f32 space);
    void SetCharSpace(f32 space);

    f32 GetLineSpace() const;
    f32 GetCharSpace() const;

    void SetTabWidth(int width);
    int GetTabWidth() const;

    void SetDrawFlag(u32 flag);
    u32 GetDrawFlag() const;

    void SetTagProcessor(TagProcessorBase<T>* pProcessor);
    void ResetTagProcessor();
    TagProcessorBase<T>& GetTagProcessor() const;

    f32 CalcFormatStringWidth(const T* pStr, ...) const;
    f32 CalcFormatStringHeight(const T* pStr, ...) const;

    void CalcFormatStringRect(Rect* pRect, const T* pStr, ...) const;
    void CalcVStringRect(Rect* pRect, const T* pStr, std::va_list args) const;

    f32 CalcStringWidth(const T* pStr, int len) const;
    f32 CalcStringWidth(const T* pStr) const;

    f32 CalcStringHeight(const T* pStr, int len) const;
    f32 CalcStringHeight(const T* pStr) const;

    void CalcStringRect(Rect* pRect, const T* pStr, int len) const;
    void CalcStringRect(Rect* pRect, const T* pStr) const;

    f32 Printf(const T* pStr, ...);
    f32 VPrintf(const T* pStr, std::va_list args);
    f32 Print(const T* pStr, int len);
    f32 Print(const T* pStr);

    static T* SetBuffer(T* pBuffer, u32 size);
    static T* SetBuffer(u32 size);

    static u32 GetBufferSize();
    static T* GetBuffer();

    static int VSNPrintf(T* pBuffer, u32 count, const T* pStr,
                         std::va_list args) {
        return sizeof(T) == sizeof(char)
                   ? std::vsnprintf((char*)pBuffer, count, (const char*)pStr,
                                    args)
                   : std::vswprintf((wchar_t*)pBuffer, count,
                                    (const wchar_t*)pStr, args);
    }
    static int StrLen(const T* pStr) {
        return sizeof(T) == sizeof(char) ? std::strlen((const char*)pStr)
                                         : std::wcslen((const wchar_t*)pStr);
    }

private:
    static const int DEFAULT_FORMAT_BUFFER_SIZE = 256;

    static const u32 DRAWFLAG_MASK_ALL = DRAWFLAG_MASK_ALIGN_TEXT |
                                         DRAWFLAG_MASK_ALIGN_H |
                                         DRAWFLAG_MASK_ALIGN_V;

private:
    f32 CalcLineWidth(const T* pStr, int len);
    int CalcLineRectImpl(Rect* pRect, const T* pStr, int len);
    void CalcStringRectImpl(Rect* pRect, const T* pStr, int len);

    f32 PrintImpl(const T* pStr, int len);
    f32 AdjustCursor(f32* pX, f32* pY, const T* pStr, int len);

    bool IsDrawFlagSet(u32 mask, u32 flag) const {
        return (mDrawFlag & mask) == flag;
    }

private:
    f32 mCharSpace;                     // at 0x4C
    f32 mLineSpace;                     // at 0x50
    int mTabWidth;                      // at 0x54
    u32 mDrawFlag;                      // at 0x58
    TagProcessorBase<T>* mTagProcessor; // at 0x5C

    static T* mFormatBuffer;
    static u32 mFormatBufferSize;
    static TagProcessorBase<T> mDefaultTagProcessor;
};

} // namespace ut
} // namespace nw4hbm

#endif
