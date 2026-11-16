#include <nw4hbm/lyt.h>
#include <nw4hbm/ut.h>

#include <cstring>
#include <cwchar>

namespace nw4hbm {
namespace lyt {

NW4R_UT_RTTI_DEF_DERIVED(TextBox, Pane);

} // namespace lyt
} // namespace nw4hbm

/******************************************************************************
 *
 * Utility functions
 *
 ******************************************************************************/
namespace {

using namespace nw4hbm;
using namespace nw4hbm::lyt;

u8 ClampColor(s16 value);

ut::Color GetColor(const GXColorS10& rSrc) {
    GXColor dst;

    dst.r = ClampColor(rSrc.r);
    dst.g = ClampColor(rSrc.g);
    dst.b = ClampColor(rSrc.b);
    dst.a = ClampColor(rSrc.a);

    return dst;
}

u8 ClampColor(s16 value) {
    return value < 0 ? 0 : (value > 255 ? 255 : value);
}

template <typename T>
int CalcLineRectImpl(ut::Rect* pRect, ut::TextWriterBase<T>* pTextWriter,
                     const T* pStr, int len, f32 maxWidth, bool* pbOver) {
    ut::PrintContext<T> context = {pTextWriter, pStr, 0.0f, 0.0f, 0};
    const ut::Font* pFont = pTextWriter->GetFont();

    f32 x = 0.0f;
    bool charSpace = false;

    ut::CharStrmReader reader = pFont->GetCharStrmReader();
    const T* pPrevPos = static_cast<const T*>(reader.GetCurrentPos());

    pRect->left = 0.0f;
    pRect->right = 0.0f;
    pRect->top = ut::Min(0.0f, pTextWriter->GetLineHeight());
    pRect->bottom = ut::Max(0.0f, pTextWriter->GetLineHeight());

    *pbOver = false;

    reader.Set(pStr);

    ut::Rect prevRect = *pRect;

    for (u16 ch = reader.Next();
         static_cast<const T*>(reader.GetCurrentPos()) - pStr <= len;
         pPrevPos = static_cast<const T*>(reader.GetCurrentPos()),
             ch = reader.Next(), prevRect = *pRect) {
        if ((int)ch < L' ') {
            typename ut::TagProcessorBase<T>::Operation oper;
            ut::Rect rect(x, 0.0f, 0.0f, 0.0f);

            context.str = static_cast<const T*>(reader.GetCurrentPos());
            context.flags = 0;
            context.flags |=
                charSpace ? 0 : ut::PrintContext<T>::FLAGS_CHARSPACE;

            pTextWriter->SetCursorX(x);

            oper = pTextWriter->GetTagProcessor().CalcRect(&rect, ch, &context);

            reader.Set(context.str);

            pRect->left = ut::Min(pRect->left, rect.left);
            pRect->top = ut::Min(pRect->top, rect.top);
            pRect->right = ut::Max(pRect->right, rect.right);
            pRect->bottom = ut::Max(pRect->bottom, rect.bottom);

            x = pTextWriter->GetCursorX();

            if (pRect->GetWidth() > maxWidth) {
                *pbOver = true;
                break;
            }

            if (oper == ut::TagProcessorBase<T>::OPERATION_END_DRAW) {
                return len;
            } else if (oper ==
                       ut::TagProcessorBase<T>::OPERATION_NO_CHAR_SPACE) {
                charSpace = false;
            } else if (oper == ut::TagProcessorBase<T>::OPERATION_CHAR_SPACE) {
                charSpace = true;
            } else if (oper == ut::TagProcessorBase<T>::OPERATION_NEXT_LINE) {
                goto end_draw;
            }
        } else {
            if (charSpace) {
                x += pTextWriter->GetCharSpace();
            }

            charSpace = true;

            if (pTextWriter->IsWidthFixed()) {
                x += pTextWriter->GetFixedWidth();
            } else {
                x += pTextWriter->GetFont()->GetCharWidth(ch) *
                     pTextWriter->GetScaleH();
            }

            pRect->left = ut::Min(pRect->left, x);
            pRect->right = ut::Max(pRect->right, x);

            if (pRect->GetWidth() > maxWidth) {
                *pbOver = true;
                goto end_draw;
            }
        }
    }

end_draw:
    if (*pbOver && pPrevPos != NULL) {
        *pRect = prevRect;
        return pPrevPos - pStr;
    } else {
        return static_cast<const T*>(reader.GetCurrentPos()) - pStr;
    }
}

template <typename T>
void CalcStringRectImpl(ut::Rect* pRect, ut::TextWriterBase<T>* pTextWriter,
                        const T* pStr, int len, f32 maxWidth) {
    int remain = len;
    const T* pPos = pStr;

    pRect->left = 0.0f;
    pRect->right = 0.0f;
    pRect->top = 0.0f;
    pRect->bottom = 0.0f;
    pTextWriter->SetCursor(0.0f, 0.0f);

    do {
        ut::Rect rect;
        bool over;
        int read =
            CalcLineRectImpl(&rect, pTextWriter, pPos, remain, maxWidth, &over);

        if (over) {
            CalcLineRectImpl(&rect, pTextWriter, L"\n", 1, maxWidth, &over);
        }

        pPos += read;
        remain -= read;

        pRect->left = ut::Min(pRect->left, rect.left);
        pRect->top = ut::Min(pRect->top, rect.top);
        pRect->right = ut::Max(pRect->right, rect.right);
        pRect->bottom = ut::Max(pRect->bottom, rect.bottom);
    } while (remain > 0);
}

template <typename T>
int CalcLineStrNum(f32* pWidth, ut::TextWriterBase<T>* pTextWriter,
                   const T* pStr, int len, f32 maxWidth, bool* pbOver) {
    ut::Rect rect;
    ut::TextWriterBase<T> clone = *pTextWriter;
    clone.SetCursor(0.0f, 0.0f);

    int ret = CalcLineRectImpl(&rect, &clone, pStr, len, maxWidth, pbOver);

    *pWidth = rect.GetWidth();
    return ret;
}

template <typename T>
void CalcStringRect(ut::Rect* pRect, ut::TextWriterBase<T>* pTextWriter,
                    const T* pStr, int len, f32 maxWidth) {
    ut::TextWriterBase<T> clone = *pTextWriter;
    CalcStringRectImpl(pRect, &clone, pStr, len, maxWidth);
}

} // namespace

namespace nw4hbm {
namespace lyt {

/******************************************************************************
 *
 * TextBox
 *
 ******************************************************************************/
TextBox::TextBox(u16 len, const wchar_t* pStr, const ut::Font* pFont) : Pane() {
    Init(len);
    SetString(pStr);
    SetFont(pFont);
}

TextBox::TextBox(const res::TextBox* pRes, const ResBlockSet& rBlockSet)
    : Pane(pRes) {

    u16 len = pRes->textBufBytes / sizeof(wchar_t);
    if (len > 0) {
        len--;
    }

    Init(len);

    if (pRes->textStrBytes >= sizeof(wchar_t) && mTextBuf != NULL) {
        const wchar_t* const pResStr =
            detail::ConvertOffsToPtr<wchar_t>(pRes, pRes->textStrOffset);

        u16 resLen = pRes->textStrBytes / sizeof(wchar_t) - 1;
        SetString(pResStr, 0, resLen);
    }

    for (int i = 0; i < TEXTCOLOR_MAX; i++) {
        mTextColors[i] = pRes->textCols[i];
    }

    mFontSize = pRes->fontSize;
    mTextPosition = pRes->textPosition;
    mCharSpace = pRes->charSpace;
    mLineSpace = pRes->lineSpace;

    const res::Font* const pResFonts = detail::ConvertOffsToPtr<res::Font>(
        rBlockSet.pFontList, sizeof(res::FontList));

    const char* const pFontName = detail::ConvertOffsToPtr<char>(
        pResFonts, pResFonts[pRes->fontIdx].nameStrOffset);

    ut::Font* pFont = rBlockSet.pResAccessor->GetFont(pFontName);

    if (pFont != NULL) {
        mpFont = pFont;
    } else {
        void* pFontBinary = rBlockSet.pResAccessor->GetResource(
            ArcResourceAccessor::RES_TYPE_FONT, pFontName, NULL);

        if (pFontBinary != NULL) {
            void* pFontBuf = Layout::AllocMemory(sizeof(ut::ResFont));

            if (pFontBuf != NULL) {
                ut::ResFont* pResFont = new (pFontBuf) ut::ResFont();
                pResFont->SetResource(pFontBinary);

                mpFont = pResFont;
                mBits.bAllocFont = true;
            }
        }
    }

    void* pMaterialBuf = Layout::AllocMemory(sizeof(Material));

    if (pMaterialBuf != NULL) {
        const u32* const pMatOffsetTbl = detail::ConvertOffsToPtr<u32>(
            rBlockSet.pMaterialList, sizeof(res::MaterialList));

        const res::Material* const pResMaterial =
            detail::ConvertOffsToPtr<res::Material>(
                rBlockSet.pMaterialList, pMatOffsetTbl[pRes->materialIdx]);

        Material* pMaterial =
            new (pMaterialBuf) Material(pResMaterial, rBlockSet);

        mpMaterial = pMaterial;
    }
}

void TextBox::Init(u16 len) {
    mTextBuf = NULL;
    mTextBufBytes = 0;
    mTextLen = 0;

    mpFont = NULL;
    mFontSize = Size(0.0f, 0.0f);

    SetTextPositionH(HORIZONTALPOSITION_CENTER);
    SetTextPositionV(VERTICALPOSITION_CENTER);

    mLineSpace = 0.0f;
    mCharSpace = 0.0f;

    mpTagProcessor = NULL;

    std::memset(&mBits, 0, sizeof(mBits));

    if (len > 0) {
        AllocStringBuffer(len);
    }
}

TextBox::~TextBox() {
    SetFont(NULL);

    if (mpMaterial != NULL && !mpMaterial->IsUserAllocated()) {
        mpMaterial->~Material();
        Layout::FreeMemory(mpMaterial);
        mpMaterial = NULL;
    }

    FreeStringBuffer();
}

ut::Color TextBox::GetVtxColor(u32 idx) const {
    return GetTextColor(idx / TEXTCOLOR_MAX);
}

void TextBox::SetVtxColor(u32 idx, ut::Color color) {
    SetTextColor(idx / TEXTCOLOR_MAX, color);
}

u8 TextBox::GetVtxColorElement(u32 idx) const {
    return reinterpret_cast<const u8*>(
        &mTextColors[idx / (4 * TEXTCOLOR_MAX)])[idx % 4];
}

void TextBox::SetVtxColorElement(u32 idx, u8 value) {
    reinterpret_cast<u8*>(&mTextColors[idx / (4 * TEXTCOLOR_MAX)])[idx % 4] =
        value;
}

const ut::Rect TextBox::GetTextDrawRect(const DrawInfo& rInfo) const {
    ut::WideTextWriter writer;

    writer.SetFont(*mpFont);
    writer.SetFontSize(mFontSize.width, mFontSize.height);
    writer.SetLineSpace(mLineSpace);
    writer.SetCharSpace(mCharSpace);

    if (mpTagProcessor != NULL) {
        writer.SetTagProcessor(mpTagProcessor);
    }

    ut::Rect rect = GetTextDrawRect(&writer);

    if (rInfo.IsYAxisUp()) {
        rect.top = -rect.top;
        rect.bottom = -rect.bottom;
    }

    return rect;
}

void TextBox::DrawSelf(const DrawInfo& rInfo) {
    if (mTextBuf == NULL || mpFont == NULL || mpMaterial == NULL) {
        return;
    }

    LoadMtx(rInfo);

    ut::WideTextWriter writer;
    writer.SetFont(*mpFont);
    writer.SetFontSize(mFontSize.width, mFontSize.height);
    writer.SetLineSpace(mLineSpace);
    writer.SetCharSpace(mCharSpace);

    ut::Color top =
        detail::MultipleAlpha(mTextColors[TEXTCOLOR_TOP], mGlbAlpha);
    ut::Color bottom =
        detail::MultipleAlpha(mTextColors[TEXTCOLOR_BOTTOM], mGlbAlpha);

    writer.SetGradationMode(top != bottom ? ut::CharWriter::GRADMODE_V
                                          : ut::CharWriter::GRADMODE_NONE);
    writer.SetTextColor(top, bottom);

    ut::Color min = GetColor(mpMaterial->GetTevColor(TEVCOLOR_REG0));
    ut::Color max = GetColor(mpMaterial->GetTevColor(TEVCOLOR_REG1));
    writer.SetColorMapping(min, max);

    if (mpTagProcessor != NULL) {
        writer.SetTagProcessor(mpTagProcessor);
    }

    writer.SetupGX();

    ut::Rect rect = GetTextDrawRect(&writer);

    f32 hMag = GetTextMagH();
    wchar_t* pPos = mTextBuf;
    f32 textWidth = rect.GetWidth();

    writer.SetCursor(rect.left, rect.top);

    for (int remain = mTextLen; remain > 0;) {
        f32 lineWidth;
        bool over;
        int lineStrNum = CalcLineStrNum(&lineWidth, &writer, pPos, remain,
                                        mSize.width, &over);
        f32 textPosX = hMag * (textWidth - lineWidth);

        writer.SetCursorX(rect.left + textPosX);
        writer.Print(pPos, lineStrNum);

        if (over) {
            writer.Print(L"\n");
        }

        pPos += lineStrNum;
        remain -= lineStrNum;
    }
}

u16 TextBox::GetStringBufferLength() const {
    if (mTextBufBytes == 0) {
        return 0;
    }

    return mTextBufBytes / sizeof(wchar_t) - 1;
}

void TextBox::AllocStringBuffer(u16 len) {
    if (len == 0) {
        return;
    }

    u16 chars = len + 1;
    u16 bytes = chars * sizeof(wchar_t);

    if (bytes > mTextBufBytes) {
        FreeStringBuffer();
        mTextBuf = static_cast<wchar_t*>(Layout::AllocMemory(bytes));

        if (mTextBuf != NULL) {
            mTextBufBytes = bytes;
        }
    }
}

void TextBox::FreeStringBuffer() {
    if (mTextBuf == NULL) {
        return;
    }

    Layout::FreeMemory(mTextBuf);
    mTextBuf = NULL;
    mTextBufBytes = 0;
}

u16 TextBox::SetString(const wchar_t* pStr, u16 pos) {
    return SetString(pStr, pos, std::wcslen(pStr));
}

u16 TextBox::SetString(const wchar_t* pStr, u16 pos, u16 len) {
    if (mTextBuf == NULL) {
        return 0;
    }

    const u16 maxlen = GetStringBufferLength();
    if (pos >= maxlen) {
        return 0;
    }

    const u16 chars = ut::Min<u16>(len, maxlen - pos);
    memcpy(&mTextBuf[pos], pStr, chars * sizeof(wchar_t));

    mTextLen = pos + chars;
    mTextBuf[mTextLen] = L'\0';

    return chars;
}

const ut::Font* TextBox::GetFont() const {
    return mpFont;
}

void TextBox::SetFont(const ut::Font* pFont) {
    if (mBits.bAllocFont) {
        mpFont->~Font();
        Layout::FreeMemory(const_cast<ut::Font*>(mpFont));
        mBits.bAllocFont = false;
    }

    mpFont = pFont;

    if (mpFont != NULL) {
        SetFontSize(Size(static_cast<f32>(mpFont->GetWidth()),
                         static_cast<f32>(mpFont->GetHeight())));
    } else {
        SetFontSize(Size(0.0f, 0.0f));
    }
}

const ut::Rect TextBox::GetTextDrawRect(ut::WideTextWriter* pWriter) const {
    ut::Rect rect;

    pWriter->SetCursor(0.0f, 0.0f);
    CalcStringRect(&rect, pWriter, mTextBuf, mTextLen, mSize.width);

    math::VEC2 base = Pane::GetVtxPos();

    rect.MoveTo(base.x + (mSize.width - rect.GetWidth()) * GetTextMagH(),
                base.y + (mSize.height - rect.GetHeight()) * GetTextMagV());

    return rect;
}

f32 TextBox::GetTextMagH() const {
    f32 mag = 0.0f;

    switch (GetTextPositionH()) {
    default:
    case HORIZONTALPOSITION_LEFT: {
        mag = 0.0f;
        break;
    }

    case HORIZONTALPOSITION_CENTER: {
        mag = 0.5f;
        break;
    }

    case HORIZONTALPOSITION_RIGHT: {
        mag = 1.0f;
        break;
    }
    }

    return mag;
}

f32 TextBox::GetTextMagV() const {
    f32 mag = 0.0f;

    switch (GetTextPositionV()) {
    default:
    case VERTICALPOSITION_TOP: {
        mag = 0.0f;
        break;
    }

    case VERTICALPOSITION_CENTER: {
        mag = 0.5f;
        break;
    }

    case VERTICALPOSITION_BOTTOM: {
        mag = 1.0f;
        break;
    }
    }

    return mag;
}

} // namespace lyt
} // namespace nw4hbm
