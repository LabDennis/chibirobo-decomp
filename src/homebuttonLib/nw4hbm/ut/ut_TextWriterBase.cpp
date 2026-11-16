#include <nw4hbm/ut.h>

namespace nw4hbm {
namespace ut {

template <typename T>
u32 TextWriterBase<T>::mFormatBufferSize = DEFAULT_FORMAT_BUFFER_SIZE;

template <typename T> T* TextWriterBase<T>::mFormatBuffer;

template <typename T>
TagProcessorBase<T> TextWriterBase<T>::mDefaultTagProcessor;

template <typename T>
TextWriterBase<T>::TextWriterBase()
    : mCharSpace(0.0f),
      mLineSpace(0.0f),
      mTabWidth(4),
      mDrawFlag(0),
      mTagProcessor(&mDefaultTagProcessor) {}

template <typename T> TextWriterBase<T>::~TextWriterBase() {}

template <typename T> void TextWriterBase<T>::SetLineHeight(f32 lineHeight) {
    const Font* pFont = GetFont();
    int lf = pFont != NULL ? pFont->GetLineFeed() : 0;

    mLineSpace = lineHeight - lf * GetScaleV();
}

template <typename T> f32 TextWriterBase<T>::GetLineHeight() const {
    const Font* pFont = GetFont();
    int lf = pFont != NULL ? pFont->GetLineFeed() : 0;

    return mLineSpace + lf * GetScaleV();
}

template <typename T> void TextWriterBase<T>::SetLineSpace(f32 space) {
    mLineSpace = space;
}

template <typename T> void TextWriterBase<T>::SetCharSpace(f32 space) {
    mCharSpace = space;
}

template <typename T> f32 TextWriterBase<T>::GetLineSpace() const {
    return mLineSpace;
}

template <typename T> f32 TextWriterBase<T>::GetCharSpace() const {
    return mCharSpace;
}

template <typename T> void TextWriterBase<T>::SetTabWidth(int width) {
    mTabWidth = width;
}

template <typename T> int TextWriterBase<T>::GetTabWidth() const {
    return mTabWidth;
}

template <typename T> void TextWriterBase<T>::SetDrawFlag(u32 flag) {
    mDrawFlag = flag;
}

template <typename T> u32 TextWriterBase<T>::GetDrawFlag() const {
    return mDrawFlag;
}

template <typename T>
void TextWriterBase<T>::SetTagProcessor(TagProcessorBase<T>* pProcessor) {
    mTagProcessor = pProcessor;
}

template <typename T> void TextWriterBase<T>::ResetTagProcessor() {
    mTagProcessor = &mDefaultTagProcessor;
}

template <typename T>
TagProcessorBase<T>& TextWriterBase<T>::GetTagProcessor() const {
    return *mTagProcessor;
}

template <typename T>
f32 TextWriterBase<T>::CalcFormatStringWidth(const T* pStr, ...) const {
    Rect rect;
    std::va_list args;

    va_start(args, pStr);
    CalcVStringRect(&rect, pStr, args);
    va_end(args);

    return rect.GetWidth();
}

template <typename T>
f32 TextWriterBase<T>::CalcFormatStringHeight(const T* pStr, ...) const {
    Rect rect;
    std::va_list args;

    va_start(args, pStr);
    CalcVStringRect(&rect, pStr, args);
    va_end(args);

    return rect.GetHeight();
}

template <typename T>
void TextWriterBase<T>::CalcFormatStringRect(Rect* pRect, const T* pStr,
                                             ...) const {
    std::va_list args;

    va_start(args, pStr);
    CalcVStringRect(pRect, pStr, args);
    va_end(args);
}

template <typename T>
f32 TextWriterBase<T>::AdjustCursor(f32* pX, f32* pY, const T* pStr, int len) {
    f32 textWidth = 0.0f;
    f32 textHeight = 0.0f;

    if (!IsDrawFlagSet(DRAWFLAG_MASK_ALL, DRAWFLAG_MASK_ALIGN_V) &&
        !IsDrawFlagSet(DRAWFLAG_MASK_ALL, 0)) {
        Rect rect;

        CalcStringRect(&rect, pStr, len);
        textWidth = rect.left + rect.right;
        textHeight = rect.top + rect.bottom;
    }

    if (IsDrawFlagSet(DRAWFLAG_MASK_ALIGN_H, DRAWFLAG_ALIGN_H_CENTER)) {
        *pX -= textWidth / 2.0f;
    } else if (IsDrawFlagSet(DRAWFLAG_MASK_ALIGN_H, DRAWFLAG_ALIGN_H_RIGHT)) {
        *pX -= textWidth;
    }

    if (IsDrawFlagSet(DRAWFLAG_MASK_ALIGN_V, DRAWFLAG_ALIGN_V_CENTER)) {
        *pY -= textHeight / 2.0f;
    } else if (IsDrawFlagSet(DRAWFLAG_MASK_ALIGN_V, DRAWFLAG_ALIGN_V_TOP)) {
        *pY -= textHeight;
    }

    if (IsDrawFlagSet(DRAWFLAG_MASK_ALIGN_TEXT, DRAWFLAG_ALIGN_TEXT_CENTER)) {
        f32 width = CalcLineWidth(pStr, len);
        f32 offset = (textWidth - width) / 2.0f;
        SetCursorX(*pX + offset);
    } else if (IsDrawFlagSet(DRAWFLAG_MASK_ALIGN_TEXT,
                             DRAWFLAG_ALIGN_TEXT_RIGHT)) {
        f32 width = CalcLineWidth(pStr, len);
        f32 offset = textWidth - width;
        SetCursorX(*pX + offset);
    } else {
        SetCursorX(*pX);
    }

    if (IsDrawFlagSet(DRAWFLAG_MASK_ALIGN_V, DRAWFLAG_MASK_ALIGN_V)) {
        SetCursorY(*pY);
    } else {
        SetCursorY(*pY + GetFontAscent());
    }

    return textWidth;
}

template <typename T> f32 TextWriterBase<T>::PrintImpl(const T* pStr, int len) {
    f32 cursorX = GetCursorX();
    f32 cursorY = GetCursorY();
    f32 orgCursorX = cursorX;
    f32 orgCursorY = cursorY;
    f32 cursorXAdj = 0.0f;
    f32 cursorYAdj = 0.0f;
    f32 textWidth = 0.0f;
    bool charSpace = false;

    textWidth = AdjustCursor(&cursorX, &cursorY, pStr, len);
    cursorXAdj = orgCursorX - GetCursorX();
    cursorYAdj = orgCursorY - GetCursorY();

    PrintContext<T> context = {this, pStr, cursorX, cursorY, 0};
    CharStrmReader reader = GetFont()->GetCharStrmReader();
    reader.Set(pStr);

    u16 ch = reader.Next();

    while (static_cast<const T*>(reader.GetCurrentPos()) - pStr <= len) {
        if (ch < ' ') {
            context.str = static_cast<const T*>(reader.GetCurrentPos());
            context.flags = 0;
            context.flags |= charSpace ? 0 : PrintContext<T>::FLAGS_CHARSPACE;

            typename TagProcessorType::Operation oper =
                mTagProcessor->Process(ch, &context);

            if (oper == TagProcessorType::OPERATION_NEXT_LINE) {
                if (IsDrawFlagSet(DRAWFLAG_MASK_ALIGN_TEXT,
                                  DRAWFLAG_ALIGN_TEXT_CENTER)) {
                    int remain = len - (context.str - pStr);
                    f32 width = CalcLineWidth(context.str, remain);
                    f32 offset = (textWidth - width) / 2.0f;

                    SetCursorX(context.x + offset);
                } else if (IsDrawFlagSet(DRAWFLAG_MASK_ALIGN_TEXT,
                                         DRAWFLAG_ALIGN_TEXT_RIGHT)) {
                    int remain = len - (context.str - pStr);
                    f32 width = CalcLineWidth(context.str, remain);
                    f32 offset = textWidth - width;

                    SetCursorX(context.x + offset);
                } else {
                    f32 width = GetCursorX() - context.x;

                    textWidth = Max(textWidth, width);
                    SetCursorX(context.x);
                }
                charSpace = false;
            } else if (oper == TagProcessorType::OPERATION_NO_CHAR_SPACE) {
                charSpace = false;
            } else if (oper == TagProcessorType::OPERATION_CHAR_SPACE) {
                charSpace = true;
            } else if (oper == TagProcessorType::OPERATION_END_DRAW) {
                break;
            }

            reader.Set(context.str);
        } else {
            f32 baseY = GetCursorY();

            if (charSpace) {
                MoveCursorX(GetCharSpace());
            }

            charSpace = true;

            {
                const Font* pFont = GetFont();
                f32 adj = -pFont->GetBaselinePos() * GetScaleV();

                MoveCursorY(adj);
            }

            CharWriter::Print(ch);
            SetCursorY(baseY);
        }

        ch = reader.Next();
    }

    if (IsDrawFlagSet(DRAWFLAG_MASK_ALIGN_V, DRAWFLAG_ALIGN_V_CENTER) ||
        IsDrawFlagSet(DRAWFLAG_MASK_ALIGN_V, DRAWFLAG_ALIGN_V_TOP)) {
        SetCursorY(orgCursorY);
    } else {
        MoveCursorY(cursorYAdj);
    }

    return textWidth;
}

template <typename T>
void TextWriterBase<T>::CalcStringRectImpl(Rect* pRect, const T* pStr,
                                           int len) {
    int remain = len;
    const T* pPos = pStr;

    pRect->left = 0.0f;
    pRect->right = 0.0f;
    pRect->top = 0.0f;
    pRect->bottom = 0.0f;

    SetCursor(0.0f, 0.0f);

    do {
        Rect rect;
        int read = CalcLineRectImpl(&rect, pPos, remain);

        pPos += read;
        remain -= read;

        pRect->left = Min(pRect->left, rect.left);
        pRect->top = Min(pRect->top, rect.top);
        pRect->right = Max(pRect->right, rect.right);
        pRect->bottom = Max(pRect->bottom, rect.bottom);
    } while (remain > 0);
}

template <typename T>
int TextWriterBase<T>::CalcLineRectImpl(Rect* pRect, const T* pStr, int len) {
    PrintContext<T> context = {this, pStr, 0.0f, 0.0f, 0};

    const Font* pFont = GetFont();
    f32 x = 0.0f;
    bool charSpace = false;

    CharStrmReader reader = pFont->GetCharStrmReader();

    pRect->left = 0.0f;
    pRect->right = 0.0f;
    pRect->top = Min(0.0f, GetLineHeight());
    pRect->bottom = Max(0.0f, GetLineHeight());

    reader.Set(pStr);

    for (u16 ch = reader.Next();
         static_cast<const T*>(reader.GetCurrentPos()) - pStr <= len;
         ch = reader.Next()) {
        if (ch < ' ') {
            typename TagProcessorType::Operation oper;
            Rect rect(x, 0.0f, 0.0f, 0.0f);

            context.str = static_cast<const T*>(reader.GetCurrentPos());
            context.flags = 0;
            context.flags |= charSpace ? 0 : PrintContext<T>::FLAGS_CHARSPACE;

            SetCursorX(x);

            oper = mTagProcessor->CalcRect(&rect, ch, &context);

            reader.Set(context.str);

            pRect->left = Min(pRect->left, rect.left);
            pRect->top = Min(pRect->top, rect.top);
            pRect->right = Max(pRect->right, rect.right);
            pRect->bottom = Max(pRect->bottom, rect.bottom);

            x = GetCursorX();

            if (oper == TagProcessorType::OPERATION_END_DRAW) {
                return len;
            } else if (oper == TagProcessorType::OPERATION_NO_CHAR_SPACE) {
                charSpace = false;
            } else if (oper == TagProcessorType::OPERATION_CHAR_SPACE) {
                charSpace = true;
            } else if (oper == TagProcessorType::OPERATION_NEXT_LINE) {
                break;
            }
        } else {
            if (charSpace) {
                x += GetCharSpace();
            }

            charSpace = true;

            if (IsWidthFixed()) {
                x += GetFixedWidth();
            } else {
                x += GetFont()->GetCharWidth(ch) * GetScaleH();
            }

            pRect->left = Min(pRect->left, x);
            pRect->right = Max(pRect->right, x);
        }
    }

    return static_cast<const T*>(reader.GetCurrentPos()) - pStr;
}

template <typename T>
f32 TextWriterBase<T>::CalcLineWidth(const T* pStr, int len) {
    Rect rect;
    TextWriterBase<T> clone(*this);

    clone.SetCursor(0.0f, 0.0f);
    clone.CalcLineRectImpl(&rect, pStr, len);

    return rect.GetWidth();
}

template <typename T> T* TextWriterBase<T>::GetBuffer() {
    return mFormatBuffer;
}

template <typename T> u32 TextWriterBase<T>::GetBufferSize() {
    return mFormatBufferSize;
}

template <typename T> T* TextWriterBase<T>::SetBuffer(u32 size) {
    T* pOldBuffer = mFormatBuffer;

    mFormatBuffer = NULL;
    mFormatBufferSize = size;

    return pOldBuffer;
}

template <typename T> T* TextWriterBase<T>::SetBuffer(T* pBuffer, u32 size) {
    T* pOldBuffer = mFormatBuffer;

    mFormatBuffer = pBuffer;
    mFormatBufferSize = size;

    return pOldBuffer;
}

template <typename T> f32 TextWriterBase<T>::Print(const T* pStr) {
    return Print(pStr, StrLen(pStr));
}

template <typename T> f32 TextWriterBase<T>::Print(const T* pStr, int len) {
    TextWriterBase<T> clone(*this);

    f32 width = clone.PrintImpl(pStr, len);
    SetCursor(clone.GetCursorX(), clone.GetCursorY());

    return width;
}

template <typename T>
f32 TextWriterBase<T>::VPrintf(const T* pStr, std::va_list args) {
    T* pBuffer = mFormatBuffer != NULL
                     ? mFormatBuffer
                     : static_cast<T*>(__alloca(mFormatBufferSize));

    int len = VSNPrintf(pBuffer, mFormatBufferSize, pStr, args);
    f32 width = Print(pBuffer, len);
    return width;
}

template <typename T> f32 TextWriterBase<T>::Printf(const T* pStr, ...) {
    std::va_list args;

    va_start(args, pStr);
    f32 width = VPrintf(pStr, args);
    va_end(args);

    return width;
}

template <typename T>
void TextWriterBase<T>::CalcStringRect(Rect* pRect, const T* pStr) const {
    CalcStringRect(pRect, pStr, StrLen(pStr));
}

template <typename T>
void TextWriterBase<T>::CalcStringRect(Rect* pRect, const T* pStr,
                                       int len) const {
    TextWriterBase<T> clone(*this);

    clone.CalcStringRectImpl(pRect, pStr, len);
}

template <typename T>
f32 TextWriterBase<T>::CalcStringHeight(const T* pStr) const {
    return CalcStringHeight(pStr, StrLen(pStr));
}

template <typename T>
f32 TextWriterBase<T>::CalcStringHeight(const T* pStr, int len) const {
    Rect rect;

    CalcStringRect(&rect, pStr, len);
    return rect.GetHeight();
}

template <typename T>
f32 TextWriterBase<T>::CalcStringWidth(const T* pStr) const {
    return CalcStringWidth(pStr, StrLen(pStr));
}

template <typename T>
f32 TextWriterBase<T>::CalcStringWidth(const T* pStr, int len) const {
    Rect rect;

    CalcStringRect(&rect, pStr, len);
    return rect.GetWidth();
}

template <typename T>
void TextWriterBase<T>::CalcVStringRect(Rect* pRect, const T* pStr,
                                        std::va_list args) const {
    T* pBuffer = mFormatBuffer != NULL
                     ? mFormatBuffer
                     : static_cast<T*>(__alloca(mFormatBufferSize));

    int len = VSNPrintf(pBuffer, mFormatBufferSize, pStr, args);
    CalcStringRect(pRect, pBuffer, len);
}

template class TextWriterBase<char>;
template class TextWriterBase<wchar_t>;

} // namespace ut
} // namespace nw4hbm
