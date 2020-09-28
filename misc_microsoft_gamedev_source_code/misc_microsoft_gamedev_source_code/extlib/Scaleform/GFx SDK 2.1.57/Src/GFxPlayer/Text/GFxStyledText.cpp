/**********************************************************************

Filename    :   GFxStyledText.cpp
Content     :   Styled text implementation
Created     :   April 29, 2008
Authors     :   Artyom Bolgar

Notes       :   
History     :   

Copyright   :   (c) 1998-2008 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/
#include "Text/GFxStyledText.h"

//////////////////////////////////////////////////////////////////////////
// GFxTextAllocator
//
GFxTextParagraph* GFxTextAllocator::AllocateParagraph()
{
    return new GFxTextParagraph(this);
}

GFxTextParagraph* GFxTextAllocator::AllocateParagraph(const GFxTextParagraph& srcPara)
{
    return new GFxTextParagraph(srcPara, this);
}

//////////////////////////////////
// GFxWideStringBuffer
//
GFxTextParagraph::TextBuffer::TextBuffer(const TextBuffer& o, GFxTextAllocator* pallocator)
{
    GASSERT(pallocator);
    pText = pallocator->AllocText(o.Size);
    Allocated = Size = o.Size;
    memcpy(pText, o.pText, sizeof(wchar_t) * o.Size);
}

GFxTextParagraph::TextBuffer::~TextBuffer()
{
    // make sure text was freed
    GASSERT(pText == NULL && Size == 0 && Allocated == 0);
}

void GFxTextParagraph::TextBuffer::Free(GFxTextAllocator* pallocator)
{
    pallocator->FreeText(pText);
    pText = NULL;
    Size = Allocated = 0;
}

const wchar_t* GFxTextParagraph::TextBuffer::GetCharPtrAt(UPInt pos) const
{
    if (!pText || pos >= Size)
        return NULL;
    return &pText[pos];
}

void GFxTextParagraph::TextBuffer::SetString
(GFxTextAllocator* pallocator, const char* putf8Str, UPInt utf8length)
{
    UPInt len;
    if (utf8length == GFC_MAX_UPINT)
        len = GUTF8Util::GetLength(putf8Str, gfc_strlen(putf8Str));
    else
        len = utf8length;
    if (Allocated < len)
    {
        pText = pallocator->ReallocText(pText, Allocated, len);
        GASSERT(pText);
        Allocated = len;
    }
    if (len > 0)
    {
        GASSERT(pText);
        GUTF8Util::DecodeString(pText, putf8Str, len);
    }
    Size = len;
}

void GFxTextParagraph::TextBuffer::SetString
(GFxTextAllocator* pallocator, const wchar_t* pstr, UPInt length)
{
    if (length == GFC_MAX_UPINT)
        length = StrLen(pstr);
    if (Allocated < length)
    {
        pText = pallocator->ReallocText(pText, Allocated, length);
        GASSERT(pText);
        Allocated = length;
    }
    if (length > 0)
    {
        GASSERT(pText);
        memcpy(pText, pstr, length * sizeof(wchar_t));
    }
    Size = length;
}

wchar_t* GFxTextParagraph::TextBuffer::CreatePosition
(GFxTextAllocator* pallocator, UPInt pos, UPInt length)
{
    GASSERT(pos <= Size);

    if (Allocated < Size + length)        
    {
        pText = pallocator->ReallocText(pText, Allocated, Size + length);
        GASSERT(pText);
        Allocated = Size + length;
    }
    if (Size - pos > 0)
    {
        GASSERT(pText);
        memmove(pText + pos + length, pText + pos, (Size - pos)*sizeof(wchar_t));
    }
    Size += length;
    GASSERT(pText != NULL || pos == 0);
    return pText + pos;
}

void GFxTextParagraph::TextBuffer::Remove(UPInt pos, UPInt length)
{
    if (pos < Size)
    {
        if (pos + length >= Size)
            Size = pos;
        else
        {
            memmove(pText + pos, pText + pos + length, (Size - (pos + length))*sizeof(wchar_t));
            Size -= length;
        }
    }
}

SPInt GFxTextParagraph::TextBuffer::StrChr(const wchar_t* ptext, UPInt length, wchar_t c)
{
    for (UPInt i = 0; i < length; ++i)
    {
        GASSERT(ptext);
        if (ptext[i] == c)
            return (SPInt)i;
    }
    return -1;
}

UPInt GFxTextParagraph::TextBuffer::StrLen(const wchar_t* ptext)
{
    GASSERT(ptext);
    UPInt i = 0;
    while (ptext[i] != 0)
        ++i;
    return i;
}

void GFxTextParagraph::TextBuffer::StripTrailingNewLines()
{
    int len = int(Size);
    // check, is the content already null terminated
    if (len > 0 && pText[len -1] == 0)
        --len; //if yes, skip the '\0'
    for (int i = len - 1; i >= 0 && (pText[i] == '\n' || pText[i] == '\r'); --i)
    {
        --Size;
        pText[i] = 0;
    }
}

void GFxTextParagraph::TextBuffer::StripTrailingNull()
{
    if (Size > 0)
    {
        GASSERT(pText);
        if (pText[Size - 1] == '\0')
            --Size;
    }
}

void GFxTextParagraph::TextBuffer::AppendTrailingNull(GFxTextAllocator* pallocator)
{
    if (Size > 0 && Size < Allocated)
    {
        GASSERT(pText);
        pText[Size++] = '\0';
    }
    else
    {
        wchar_t* p = CreatePosition(pallocator, Size, 1); 
        GASSERT(p);
        if (p)
            *p  = '\0';
    }
}

UPInt GFxTextParagraph::TextBuffer::GetLength() const
{
    int len = int(Size);
    // check, is the content already null terminated
    if (len > 0 && pText[len - 1] == '\0')
        return Size - 1;

    return Size;
}

//////////////////////////////////
// GFxTextParagraph
//
GFxTextParagraph::GFxTextParagraph(GFxTextAllocator* ptextAllocator):
StartIndex(0), ModCounter(0) 
{
    GASSERT(ptextAllocator);
    UniqueId = ptextAllocator->AllocateParagraphId();
}

GFxTextParagraph::GFxTextParagraph(const GFxTextParagraph& o, GFxTextAllocator* ptextAllocator) : 
Text(o.Text, ptextAllocator), 
pFormat(o.pFormat), FormatInfo(o.FormatInfo), StartIndex(o.StartIndex), ModCounter(0)
{
    GASSERT(ptextAllocator);
    UniqueId = ptextAllocator->AllocateParagraphId();
}

void GFxTextParagraph::Clear()
{
    Text.Clear();
    FormatInfo.RemoveAll();
    ++ModCounter;
}

void GFxTextParagraph::SetText(GFxTextAllocator* pallocator, const wchar_t* pstring, UPInt length)
{
    GASSERT(length <= GFC_MAX_UPINT);
    if (length != GFC_MAX_UPINT)
    {
        // check if string contains '\0' inside. If yes - correct
        // the length.
        for (SPInt i = (SPInt)length - 1; i >= 0; --i)
        {
            if (pstring[i] == '\0')
            {
                length = (UPInt)i;
                break;
            }
        }
    }
    Text.SetString(pallocator, pstring, length);
    ++ModCounter;
}

void GFxTextParagraph::SetTextFormat(GFxTextAllocator* pallocator, const GFxTextFormat& fmt, UPInt startPos, UPInt endPos)
{
    FormatRunIterator it = GetIteratorAt(startPos);
    if (endPos < startPos) endPos = startPos;
    SPInt length = (endPos == GFC_MAX_UPINT) ? GFC_MAX_SPINT : endPos - startPos;
    while(length > 0 && !it.IsFinished())
    {
        const StyledTextRun& run = *it;
        const UPInt runIndex = run.Index, runLength = run.Length;
        SPInt curIndex;
        if (startPos > runIndex)
            curIndex = startPos;
        else
            curIndex = runIndex;
        GFxTextFormat format;
        GPtr<GFxTextFormat> pfmt;
        if (run.pFormat)
        {
            format = run.pFormat->Merge(fmt);
            pfmt = *pallocator->AllocateTextFormat(format);
        }
        else
        {
            pfmt = *pallocator->AllocateTextFormat(fmt);
        }

        UPInt newLen = GTL::gpmin(UPInt(runLength - (curIndex - runIndex)), (UPInt)length);
        FormatInfo.SetRange(curIndex, newLen, pfmt);
        length -= newLen;
        //++it;
        it.SetTextPos(runIndex + runLength);
    }

    //_dump(FormatInfo);

    ++ModCounter;
}

GFxTextFormat GFxTextParagraph::GetTextFormat(UPInt startPos, UPInt endPos) const
{
    FormatRunIterator it = GetIteratorAt(startPos);
    if (endPos < startPos) endPos = startPos;
    SPInt length = (endPos == GFC_MAX_UPINT) ? GFC_MAX_SPINT : endPos - startPos;
    GFxTextFormat finalTextFmt;
    UInt i = 0;
    while(length > 0 && !it.IsFinished())
    {
        const StyledTextRun& run = *it;

        if (run.pFormat)
        {
            if (i++ == 0)
                finalTextFmt = *run.pFormat;
            else
                finalTextFmt = run.pFormat->Intersection(finalTextFmt);
        }

        length -= run.Length;
        ++it;
    }
    return finalTextFmt;
}

// returns the actual pointer on text format at the specified position.
// Will return NULL, if there is no text format at the "pos"
GFxTextFormat* GFxTextParagraph::GetTextFormatPtr(UPInt startPos) const
{
    FormatRunIterator it = GetIteratorAt(startPos);
    GFxTextFormat* pfmt = NULL;
    if (!it.IsFinished())
    {
        const StyledTextRun& run = *it;

        if (run.pFormat)
        {
            pfmt = run.pFormat;
        }
    }
    return pfmt;
}

void GFxTextParagraph::ClearTextFormat(UPInt startPos, UPInt endPos)
{
    FormatRunIterator it = GetIteratorAt(startPos);
    if (endPos < startPos) endPos = startPos;
    SPInt length = (endPos == GFC_MAX_UPINT) ? GFC_MAX_SPINT : endPos - startPos;
    while(length > 0 && !it.IsFinished())
    {
        const StyledTextRun& run = *it;
        const UPInt runIndex = run.Index, runLength = run.Length;
        SPInt curIndex;
        if (startPos > runIndex)
            curIndex = startPos;
        else
            curIndex = runIndex;

        UPInt newLen = GTL::gpmin(UPInt(runLength - (curIndex - runIndex)), (UPInt)length);
        FormatInfo.ClearRange(curIndex, newLen);
        length -= newLen;
        //++it;
        it.SetTextPos(runIndex + runLength);
    }

    //_dump(FormatInfo);

    ++ModCounter;
}


void GFxTextParagraph::SetFormat(GFxTextAllocator* pallocator, const GFxTextParagraphFormat& fmt)
{
    GPtr<GFxTextParagraphFormat> pfmt;
    if (pFormat)
        pfmt = *pallocator->AllocateParagraphFormat(pFormat->Merge(fmt));
    else
        pfmt = *pallocator->AllocateParagraphFormat(fmt);
    pFormat = pfmt;
    ++ModCounter;
}

void GFxTextParagraph::SetFormat(const GFxTextParagraphFormat* pfmt)
{
    pFormat = const_cast<GFxTextParagraphFormat*>(pfmt);
    ++ModCounter;
}

GFxTextParagraph::FormatRunIterator GFxTextParagraph::GetIterator() const
{
    return FormatRunIterator(FormatInfo, Text);
}

GFxTextParagraph::FormatRunIterator GFxTextParagraph::GetIteratorAt(UPInt index) const
{
    return FormatRunIterator(FormatInfo, Text, index);
}

wchar_t* GFxTextParagraph::CreatePosition(GFxTextAllocator* pallocator, UPInt pos, UPInt length)
{
    if (length == 0)
        return NULL;
    wchar_t* p = Text.CreatePosition(pallocator, pos, length);
    GASSERT(p);
    FormatInfo.ExpandRange(pos, length);
    ++ModCounter;
    return p;
}

void GFxTextParagraph::SetTermNullFormat()
{
    if (HasTermNull())
    {
        UPInt len = GetLength();
        FormatInfo.ExpandRange(len, 1);
        FormatInfo.RemoveRange(len + 1, 1);
    }
}

void GFxTextParagraph::InsertString(GFxTextAllocator* pallocator, const wchar_t* pstr, UPInt pos, UPInt length, const GFxTextFormat* pnewFmt)
{
    if (length > 0)
    {
        if (length == GFC_MAX_UPINT)
            length = TextBuffer::StrLen(pstr);
        wchar_t* p = CreatePosition(pallocator, pos, length);
        GASSERT(p);
        if (p)
        {
            memcpy(p, pstr, length * sizeof(wchar_t));
            if (pnewFmt)
            {
                //if (HasTermNull() && pos + length == GetLength())
                //    ++length; // need to expand format info to the termination null symbol
                FormatInfo.SetRange(pos, length, const_cast<GFxTextFormat*>(pnewFmt));
            }

            SetTermNullFormat();
            ++ModCounter;
        }
        //_dump(FormatInfo);
    }
}

void GFxTextParagraph::AppendPlainText(GFxTextAllocator* pallocator, const wchar_t* pstr, UPInt length)
{
    if (length > 0)
    {
        if (length == GFC_MAX_UPINT)
            length = TextBuffer::StrLen(pstr);
        wchar_t* p = CreatePosition(pallocator, GetLength(), length);
        GASSERT(p);

        if (p)
        {
            memcpy(p, pstr, length * sizeof(wchar_t));
            ++ModCounter;
        }
        //_dump(FormatInfo);
    }
}

void GFxTextParagraph::AppendPlainText(GFxTextAllocator* pallocator, const char* putf8str, UPInt utf8StrSize)
{
    if (utf8StrSize > 0)
    {
        UPInt length;
        if (utf8StrSize == GFC_MAX_UPINT)
            length = (UPInt)GUTF8Util::GetLength(putf8str);
        else
            length = (UPInt)GUTF8Util::GetLength(putf8str, (SPInt)utf8StrSize);
        wchar_t* p = CreatePosition(pallocator, GetLength(), length);
        GASSERT(p);

        if (p)
        {
            GUTF8Util::DecodeString(p, putf8str, length);
            ++ModCounter;
        }
        //_dump(FormatInfo);
    }
}

void GFxTextParagraph::AppendTermNull(GFxTextAllocator* pallocator, const GFxTextFormat* pdefTextFmt)
{
    if (!HasTermNull())
    {
        UPInt pos = GetLength();
        wchar_t* p = CreatePosition(pallocator, pos, 1);
        GASSERT(p);
        if (p)
        {
            *p = '\0';
            GASSERT(FormatInfo.Count() || pdefTextFmt);
            if (FormatInfo.Count() == 0 && pdefTextFmt)
            {
                GPtr<GFxTextFormat> pfmt = *pallocator->AllocateTextFormat(*pdefTextFmt);
                FormatInfo.SetRange(pos, 1, pfmt);
            }
        }
    }
}

void GFxTextParagraph::RemoveTermNull()
{
    if (HasTermNull())
    {
        FormatInfo.RemoveRange(GetLength(), 1);
        Text.StripTrailingNull();
    }
}

bool GFxTextParagraph::HasTermNull() const
{
    UPInt l = Text.GetSize();
    return (l > 0 && *Text.GetCharPtrAt(l - 1) == '\0');
}

bool GFxTextParagraph::HasNewLine() const
{
    UPInt l = Text.GetSize();
    if (l > 0)
    {
        wchar_t c = *Text.GetCharPtrAt(l - 1);
        return (c == '\r' || c == '\n');
    }
    return false;
}

// returns true if paragraph is empty (no chars or only termination null)
UPInt GFxTextParagraph::GetLength() const
{
    UPInt len = GetSize();
    if (len > 0 && HasTermNull())
        --len;
    return len;
}

void GFxTextParagraph::Remove(UPInt startPos, UPInt endPos)
{
    GASSERT(endPos >= startPos);
    UPInt length = (endPos == GFC_MAX_UPINT) ? GFC_MAX_UPINT : (UPInt)(endPos - startPos);
    if (length > 0)
    {
        Text.Remove(startPos, length);

        //_dump(FormatInfo);
        FormatInfo.RemoveRange(startPos, length);
        SetTermNullFormat();
        //_dump(FormatInfo);

        ++ModCounter;
    }
}

// copies text and formatting from psrcPara paragraph to this one, starting from startSrcIndex in
// source paragraph and startDestIndex in destination one. The "length" specifies the number
// of positions to copy.
void GFxTextParagraph::Copy(GFxTextAllocator* pallocator, const GFxTextParagraph& psrcPara, UPInt startSrcIndex, UPInt startDestIndex, UPInt length)
{
    if (length > 0)
    {
        InsertString(pallocator, psrcPara.GetText() + startSrcIndex, startDestIndex, length);
        // copy format info
        FormatRunIterator fmtIt = psrcPara.GetIteratorAt(startSrcIndex);
        UPInt remainingLen = length;
        for(; !fmtIt.IsFinished() && remainingLen > 0; ++fmtIt)
        {
            const GFxTextParagraph::StyledTextRun& run = *fmtIt;

            SPInt idx;
            UPInt len;
            if (run.Index < SPInt(startSrcIndex))
            {
                idx = 0;
                len = run.Length - (startSrcIndex - run.Index);
            }
            else
            {
                idx = run.Index - startSrcIndex;
                len = run.Length;
            }
            len = GTL::gpmin(len, remainingLen);
            if (run.pFormat)
            {
                FormatInfo.SetRange(startDestIndex + idx, len, run.pFormat);
            }
            remainingLen -= len;
        }
        SetTermNullFormat();
        ++ModCounter;
    }
}

// Shrinks the paragraph's length by the "delta" value.
void GFxTextParagraph::Shrink(UPInt delta)
{
    UPInt len = Text.GetSize();
    delta = GTL::gpmin(delta, len);
    Remove(len - delta, len);
}

#ifdef GFC_BUILD_DEBUG
void GFxTextParagraph::CheckIntegrity() const
{
    const_cast<GFxTextParagraph*>(this)->FormatInfo.CheckIntegrity();

    // check the size and length are correctly set
    if (GetLength() != GetSize())
        GASSERT(HasTermNull());
    else
        GASSERT(!HasTermNull());

    // check if the null or newline chars are inside of the paragraph. 
    // Should be only at the end of it.
    for(UPInt i = 0, n = GetLength(); i < n; ++i)
    {
        GASSERT(Text.GetCharPtrAt(i));
        wchar_t c = *Text.GetCharPtrAt(i);
        GASSERT(c != 0);
        if (i + 1 < n)
            GASSERT(c != '\r' && c != '\n');
    }

    // also need to verify there are no term null and \n - \r symbols together
    for(UPInt i = 0, n = GetSize(), m = 0; i < n; ++i)
    {
        GASSERT(Text.GetCharPtrAt(i));
        wchar_t c = *Text.GetCharPtrAt(i);
        if (c == '\r')      { GASSERT(m == 0); m |= 1; }
        else if (c == '\n') { GASSERT(m == 0); m |= 2; }
        else if (c == 0)    { GASSERT(m == 0); m |= 4; }

        // also check, that every char is covered by format data
        GASSERT(GetTextFormatPtr(i));
    }

    // ensure, format info range array does not exceed the text size
    TextFormatArrayType::Iterator it = const_cast<GFxTextParagraph*>(this)->FormatInfo.Last();
    if (!it.IsFinished())
    {
        TextFormatRunType& r = *it; 
        SPInt ni = r.NextIndex();
        UPInt sz = GetSize();
        GASSERT(ni >= 0 && ((UPInt)ni) <= sz);
    }

}
#endif

//////////////////////////////////////////////////////////////////////////
// Paragraphs' iterators
GFxTextParagraph::FormatRunIterator::FormatRunIterator(const TextFormatArrayType& fmts, const TextBuffer& textHandle) : 
pFormatInfo(&fmts), FormatIterator(fmts.Begin()), pText(&textHandle),  
CurTextIndex(0)
{
}

GFxTextParagraph::FormatRunIterator::FormatRunIterator(const TextFormatArrayType& fmts, const TextBuffer& textHandle, UPInt index) : 
pFormatInfo(&fmts), FormatIterator(pFormatInfo->GetIteratorByNearestIndex(index)),
pText(&textHandle), CurTextIndex(0)
{
    if (!FormatIterator.IsFinished())
    {
        if (!FormatIterator->Contains((SPInt)index))
        {
            if ((SPInt)index > FormatIterator->Index)
            {
                CurTextIndex = (UPInt)FormatIterator->Index;
                CurTextIndex += FormatIterator->Length;
                ++FormatIterator;
            }
        }
        else
        {
            CurTextIndex = (UPInt)FormatIterator->Index;    
        }
    }
}
GFxTextParagraph::FormatRunIterator::FormatRunIterator(const FormatRunIterator& orig) :
pFormatInfo(orig.pFormatInfo), FormatIterator(orig.FormatIterator), pText(orig.pText),
CurTextIndex(orig.CurTextIndex)
{
}

GFxTextParagraph::FormatRunIterator& GFxTextParagraph::FormatRunIterator::operator=(const GFxTextParagraph::FormatRunIterator& orig)
{
    FormatIterator    = orig.FormatIterator;
    pFormatInfo       = orig.pFormatInfo;
    pText             = orig.pText;
    CurTextIndex      = orig.CurTextIndex;
    return *this;
}

const GFxTextParagraph::StyledTextRun& GFxTextParagraph::FormatRunIterator::operator* () 
{ 
    if (!FormatIterator.IsFinished())
    {
        const TextFormatRunType& fmtRange = *FormatIterator; //(*pFormatInfo)[CurFormatIndex];
        GASSERT(fmtRange.Index >= 0);
        if (CurTextIndex < (UPInt)fmtRange.Index)
        {
            // if index in text is lower than current range index - 
            // just return a text w/o format.

            // determine the length of text as fmtRange.Index - CurTextIndex
            return PlaceHolder.Set(pText->ToWStr() + CurTextIndex, CurTextIndex, fmtRange.Index - CurTextIndex, NULL);
        }
        else
        {
            GASSERT(fmtRange.Index >= 0 && UPInt(fmtRange.Index) == CurTextIndex);
            return PlaceHolder.Set(pText->ToWStr() + fmtRange.Index, fmtRange.Index, fmtRange.Length, fmtRange.GetData());
        }
    }
    return PlaceHolder.Set(pText->ToWStr() + CurTextIndex, CurTextIndex, pText->GetSize() - CurTextIndex, NULL); 
}

void GFxTextParagraph::FormatRunIterator::operator++() 
{ 
    //if (CurFormatIndex < pFormatInfo->Count())
    if (!FormatIterator.IsFinished())
    {
        const TextFormatRunType& fmtRange = *FormatIterator;
        GASSERT(fmtRange.Index >= 0);
        if (CurTextIndex < UPInt(fmtRange.Index))
        {
            CurTextIndex = fmtRange.Index;
        }
        else
        {
            GASSERT(UPInt(fmtRange.Index) == CurTextIndex);
            CurTextIndex += fmtRange.Length;
            //++CurFormatIndex;
            ++FormatIterator;
        }
    }
    else
        CurTextIndex = pText->GetSize();
}

void GFxTextParagraph::FormatRunIterator::SetTextPos(UPInt newTextPos)
{
    while(!IsFinished())
    {
        const GFxTextParagraph::StyledTextRun& stRun = *(*this); 
        //GASSERT(fmtRange.Index >= 0);
        if (stRun.Index >= (SPInt)newTextPos)
            break;
        operator++();
    }
}

//////////////////////////////////////////////////////////////////////////
GFxTextParagraph::CharactersIterator::CharactersIterator(const GFxTextParagraph* pparagraph) : 
pFormatInfo(&pparagraph->FormatInfo), FormatIterator(pFormatInfo->Begin()), pText(&pparagraph->Text),  
CurTextIndex(0)
{
}

GFxTextParagraph::CharactersIterator::CharactersIterator(const GFxTextParagraph* pparagraph, UPInt index) : 
pFormatInfo(&pparagraph->FormatInfo), FormatIterator(pFormatInfo->GetIteratorByNearestIndex(index)),
pText(&pparagraph->Text), CurTextIndex(index)
{
    if (!FormatIterator.IsFinished())
    {
        if (!FormatIterator->Contains(index) && (SInt)index > FormatIterator->Index)
        {
            ++FormatIterator;
        }
    }
}

GFxTextParagraph::CharacterInfo& GFxTextParagraph::CharactersIterator::operator*() 
{ 
    if (!IsFinished())
    {
        PlaceHolder.Character = *(pText->ToWStr() + CurTextIndex);
        PlaceHolder.Index     = CurTextIndex;
        if (!FormatIterator.IsFinished())
        {
            const TextFormatRunType& fmtRange = *FormatIterator; 
            GASSERT(fmtRange.Index >= 0);
            if (CurTextIndex < (UPInt)fmtRange.Index)
            {
                // if index in text is lower than current range index - 
                // just return a text w/o format.
                PlaceHolder.pFormat = NULL;
            }
            else
            {
                GASSERT(fmtRange.Contains(CurTextIndex));
                PlaceHolder.pFormat = fmtRange.GetData();
            }
        }
        else
            PlaceHolder.pFormat = NULL;
    }
    else
    {
        PlaceHolder.Character   = 0;
        PlaceHolder.Index       = CurTextIndex;
        PlaceHolder.pFormat     = NULL;
    }
    return PlaceHolder; 
}

void GFxTextParagraph::CharactersIterator::operator++() 
{ 
    if (!IsFinished())
    {
        ++CurTextIndex;
        if (!FormatIterator.IsFinished())
        {
            const TextFormatRunType& fmtRange = *FormatIterator; 
            GASSERT(fmtRange.Index >= 0);
            if (CurTextIndex >= UPInt(fmtRange.NextIndex()))
            {
                ++FormatIterator;
            }
        }
    }
    else
        CurTextIndex = pText->GetSize();
}

void GFxTextParagraph::CharactersIterator::operator+=(UPInt n) 
{
    for(UPInt i = 0; i < n; ++i)
    {
        operator++();
    }
}

const wchar_t* GFxTextParagraph::CharactersIterator::GetRemainingTextPtr(UPInt* plen) const 
{ 
    if (!IsFinished())
    {
        if (plen) *plen = pText->GetSize() - CurTextIndex;
        return pText->ToWStr() + CurTextIndex;
    }
    *plen = 0;
    return NULL;
}

//////////////////////////////////
// GFxStyledText
//
GFxStyledText::GFxStyledText():Paragraphs(), RTFlags(0) 
{
    pDefaultParagraphFormat = *new GFxTextParagraphFormat;
    pDefaultTextFormat      = *new GFxTextFormat;
}

GFxStyledText::GFxStyledText(GFxTextAllocator* pallocator) : 
pTextAllocator(pallocator),Paragraphs(), RTFlags(0)
{
    GASSERT(pallocator);
    pDefaultParagraphFormat = *pallocator->AllocateParagraphFormat(GFxTextParagraphFormat());
    pDefaultTextFormat      = *pallocator->AllocateTextFormat(GFxTextFormat());
}

GFxTextParagraph* GFxStyledText::AppendNewParagraph(const GFxTextParagraphFormat* pdefParaFmt)
{
    UPInt sz = Paragraphs.size();
    UPInt nextPos = 0;
    if (sz > 0)
    {
        GFxTextParagraph* ppara = Paragraphs[sz - 1];
        GASSERT(ppara);
        nextPos = ppara->GetStartIndex() + ppara->GetLength();
    }

    Paragraphs.append(GetAllocator()->AllocateParagraph());

    sz = Paragraphs.size();
    GFxTextParagraph* ppara = Paragraphs[sz - 1];
    GASSERT(ppara);
    ppara->SetFormat(pTextAllocator, (pdefParaFmt == NULL) ? *pDefaultParagraphFormat : *pdefParaFmt);
    ppara->SetStartIndex(nextPos);
    return ppara;
}

GFxTextParagraph* GFxStyledText::AppendCopyOfParagraph(const GFxTextParagraph& srcPara)
{
    UPInt sz = Paragraphs.size();
    UPInt nextPos = 0;
    if (sz > 0)
    {
        GFxTextParagraph* ppara = Paragraphs[sz - 1];
        GASSERT(ppara);
        nextPos = ppara->GetStartIndex() + ppara->GetLength();
    }

    Paragraphs.append(GetAllocator()->AllocateParagraph(srcPara));

    sz = Paragraphs.size();
    GFxTextParagraph* ppara = Paragraphs[sz - 1];
    GASSERT(ppara);
    ppara->SetStartIndex(nextPos);
    return ppara;
}

GFxTextParagraph* GFxStyledText::InsertNewParagraph(GFxStyledText::ParagraphsIterator& iter, 
                                                    const GFxTextParagraphFormat* pdefParaFmt)
{
    if (iter.IsFinished())
        return AppendNewParagraph(pdefParaFmt);
    else
    {
        UPInt index = UPInt(iter.get_index());
        UPInt nextPos = 0;
        if (index > 0)
        {
            GFxTextParagraph* ppara = Paragraphs[index - 1];
            GASSERT(ppara);
            nextPos = ppara->GetStartIndex() + ppara->GetLength();
        }

        Paragraphs.insert(index, GetAllocator()->AllocateParagraph());

        GFxTextParagraph* ppara = Paragraphs[index];
        GASSERT(ppara);
        ppara->SetFormat(pTextAllocator, (pdefParaFmt == NULL) ? *pDefaultParagraphFormat : *pdefParaFmt);
        ppara->SetStartIndex(nextPos);
        return ppara;
    }
}

GFxTextParagraph* GFxStyledText::InsertCopyOfParagraph(GFxStyledText::ParagraphsIterator& iter, 
                                                       const GFxTextParagraph& srcPara)
{
    if (iter.IsFinished())
        return AppendCopyOfParagraph(srcPara);
    else
    {
        UPInt index = UPInt(iter.get_index());
        UPInt nextPos = 0;
        if (index > 0)
        {
            GFxTextParagraph* ppara = Paragraphs[index - 1];
            GASSERT(ppara);
            nextPos = ppara->GetStartIndex() + ppara->GetLength();
        }

        Paragraphs.insert(index, GetAllocator()->AllocateParagraph(srcPara));

        GFxTextParagraph* ppara = Paragraphs[index];
        GASSERT(ppara);
        ppara->SetStartIndex(nextPos);
        return ppara;
    }
}

GFxTextParagraph* GFxStyledText::GetLastParagraph()
{
    ParagraphsIterator paraIter = Paragraphs.Last();
    if (!paraIter.IsFinished())
    {
        return paraIter->GetPtr();
    }
    return NULL;
}

const GFxTextParagraph* GFxStyledText::GetLastParagraph() const
{
    ParagraphsConstIterator paraIter = Paragraphs.Last();
    if (!paraIter.IsFinished())
    {
        return paraIter->GetPtr();
    }
    return NULL;
}

UPInt GFxStyledText::AppendString(const char* putf8String, UPInt stringSize,
                                  NewLinePolicy newLinePolicy)
{
    GASSERT(GetDefaultTextFormat() && GetDefaultParagraphFormat());
    return AppendString(putf8String, stringSize, newLinePolicy, 
        GetDefaultTextFormat(), GetDefaultParagraphFormat());
}

UPInt GFxStyledText::AppendString(const char* putf8String, UPInt stringSize,
                                  NewLinePolicy newLinePolicy,
                                  const GFxTextFormat* pdefTextFmt, 
                                  const GFxTextParagraphFormat* pdefParaFmt)
{
    GASSERT(pdefTextFmt && pdefParaFmt);
    // parse text, and form paragraphs
    const char* pbegin = putf8String;
    if (stringSize == GFC_MAX_UPINT)
        stringSize = gfc_strlen(putf8String);
    const char* pend = pbegin + stringSize;

    // parse text, and form paragraphs
    UInt32 uniChar = 0;

    UPInt totalAppenededLen = 0;
    UPInt curOffset;
    GFxTextParagraph* ppara = GetLastParagraph();
    if (!ppara)
        curOffset = 0;
    else
        curOffset = ppara->GetStartIndex();

    OnTextInserting(curOffset, stringSize, putf8String);

    UInt i = 0;
    do
    {
        UPInt posInPara;
        UPInt startOffset = curOffset;
        if (i++ > 0 || ppara == NULL)
        {
            // need to create new paragraph
            ppara = AppendNewParagraph(pdefParaFmt);
            posInPara = 0;
            ppara->SetStartIndex(startOffset);
        }
        else
        {
            // use existing paragraph
            ppara->RemoveTermNull();
            posInPara = ppara->GetLength();
        }
        GASSERT(ppara);
        UPInt paraLength = 0;
        const char* pstr = pbegin;
        UInt32 prevUniChar = uniChar;
        for (uniChar = ~0u; pstr < pend && uniChar != 0; )
        {
            UInt32 newUniChar = GUTF8Util::DecodeNextChar(&pstr);
            if (newLinePolicy == NLP_CompressCRLF && prevUniChar == '\r' && paraLength == 0)
            {
                prevUniChar = ~0u;
                if (newUniChar == '\n')
                {
                    ++pbegin;
                    continue; // skip extra newline
                }
            }
            uniChar = newUniChar;
            if (uniChar == '\n' || uniChar == '\r')
                break;
            ++paraLength;
        }
        if (uniChar == '\n' || uniChar == '\r')
            ++paraLength; 

        if (paraLength > 0)
        {
            wchar_t* pwstr = ppara->CreatePosition(pTextAllocator, posInPara, paraLength);

            pstr = pbegin;
            for (uniChar = ~0u; pstr < pend && uniChar != 0; )
            {
                uniChar = GUTF8Util::DecodeNextChar(&pstr);
                if (uniChar == '\r' || uniChar == '\n')  // replace '\r' by '\n'
                    uniChar = NewLineChar();
                *pwstr++ = (wchar_t)uniChar;
                if (uniChar == NewLineChar())
                    break;
            }
            ppara->SetTextFormat(pTextAllocator, *pdefTextFmt, posInPara, GFC_MAX_UPINT);
            pbegin = pstr;
            curOffset += paraLength + posInPara;
            totalAppenededLen += paraLength;
        }
    } while(pbegin < pend && uniChar != 0);

    // check the last char. If it is \n - create the empty paragraph
    if (uniChar == NewLineChar())
        ppara = AppendNewParagraph(pdefParaFmt);

    ppara->AppendTermNull(pTextAllocator, pdefTextFmt);

    if (pdefTextFmt->IsUrlSet())
        SetMayHaveUrl();

    CheckIntegrity();
    return totalAppenededLen;
}

UPInt GFxStyledText::AppendString(const wchar_t* pstr, UPInt length, NewLinePolicy newLinePolicy)
{
    GASSERT(GetDefaultTextFormat() && GetDefaultParagraphFormat());
    return AppendString(pstr, length, newLinePolicy, GetDefaultTextFormat(), GetDefaultParagraphFormat());
}

UPInt GFxStyledText::AppendString(const wchar_t* pstr, UPInt length,
                                  NewLinePolicy newLinePolicy,
                                  const GFxTextFormat* pdefTextFmt, 
                                  const GFxTextParagraphFormat* pdefParaFmt)
{
    GASSERT(pdefTextFmt && pdefParaFmt);

    // parse text, and form paragraphs
    UInt32 uniChar = 0;
    if (length == GFC_MAX_UPINT)
        length = gfc_wcslen(pstr);
    const wchar_t* pend = pstr + length;

    UPInt curOffset;
    GFxTextParagraph* ppara = GetLastParagraph();
    if (!ppara)
        curOffset = 0;
    else
        curOffset = ppara->GetStartIndex();

    OnTextInserting(curOffset, length, pstr);

    UPInt totalAppenededLen = 0;
    UInt i = 0;
    do
    {
        UPInt posInPara;
        UPInt startOffset = curOffset;
        if (i++ > 0 || ppara == NULL)
        {
            // need to create new paragraph
            ppara = AppendNewParagraph(pdefParaFmt);
            posInPara = 0;
            ppara->SetStartIndex(startOffset);
        }
        else
        {
            // use existing paragraph
            ppara->RemoveTermNull();
            posInPara = ppara->GetLength();
        }
        UPInt paraLength = 0;
        // check, do we need to skip extra new line char in the case if "\r\n" EOL
        // is used.
        if (newLinePolicy == NLP_CompressCRLF && uniChar == '\r' && pstr[0] == '\n')
        {
            ++pstr;
            --length;
            if (length == 0)
                break; // it is possible that that was the last char.
        }
        for(; paraLength < length; ++paraLength)
        {
            uniChar = pstr[paraLength];
            if (uniChar == '\n' || uniChar == '\r' || uniChar == '\0')
                break;
        }
        if (uniChar == '\n' || uniChar == '\r')
            ++paraLength; // skip \n

        wchar_t* pwstr = ppara->CreatePosition(pTextAllocator, posInPara, paraLength);
        memcpy(pwstr, pstr, paraLength * sizeof(wchar_t));
        pstr += paraLength;
        length -= paraLength;
        GASSERT(((SPInt)length) >= 0);

        if ((uniChar == '\n' || uniChar == '\r') && uniChar != NewLineChar())
        {
            // need to add '\n' instead of '\r'
            UPInt l = ppara->GetLength();
            if (l > 0)
            {
                wchar_t* p = ppara->GetText();
                p[l - 1] = NewLineChar();
            }
            uniChar = NewLineChar();
        }
        ppara->SetTextFormat(pTextAllocator, *pdefTextFmt, posInPara, GFC_MAX_UPINT);
        curOffset += paraLength + posInPara;
        totalAppenededLen += paraLength;
    } while(pstr < pend && uniChar != '\0');

    // check the last char. If it is \n - create the empty paragraph
    if (uniChar == NewLineChar())
        ppara = AppendNewParagraph(pdefParaFmt);

    ppara->AppendTermNull(pTextAllocator, pdefTextFmt);

    CheckIntegrity();
    return totalAppenededLen;
}

void GFxStyledText::SetText(const char* putf8String, UPInt stringSize)
{
    Clear();
    AppendString(putf8String, stringSize);
}

void GFxStyledText::SetText(const wchar_t* pstr, UPInt length)
{
    Clear();
    AppendString(pstr, length);
}

GFxStyledText::ParagraphsIterator GFxStyledText::GetParagraphByIndex(UPInt index, UPInt* pindexInParagraph)
{
    static ParagraphComparator cmp;
    SPInt i = GTL::find_index_in_sorted_array(Paragraphs, index, cmp);
    if (i != -1)
    {
        ParagraphsIterator it(Paragraphs.Begin() + (int)i);
        if (pindexInParagraph)
            *pindexInParagraph = index - it->GetPtr()->GetStartIndex();
        return it;    
    }
    return ParagraphsIterator();
}

GFxStyledText::ParagraphsIterator GFxStyledText::GetNearestParagraphByIndex(UPInt index, UPInt* pindexInParagraph)
{
    static ParagraphComparator cmp;
    SPInt i = GTL::find_nearest_index_in_sorted_array(Paragraphs, index, cmp);
    if (i != -1)
    {
        ParagraphsIterator it(Paragraphs.Begin() + (int)i);
        if (pindexInParagraph)
        {
            *pindexInParagraph = index - (*it)->GetStartIndex();
        }
        return it;    
    }
    return ParagraphsIterator();
}

void GFxStyledText::EnsureTermNull()
{
    GFxTextParagraph* ppara = GetLastParagraph();
    if (!ppara)
        ppara = AppendNewParagraph();
    if (ppara && !ppara->HasNewLine())
    {
        ppara->AppendTermNull(pTextAllocator, pDefaultTextFormat);
    }
}

void GFxStyledText::SetTextFormat(const GFxTextFormat& fmt, UPInt startPos, UPInt endPos)
{
    GASSERT(endPos >= startPos);
    UPInt indexInPara, endIndexInPara;
    ParagraphsIterator paraIter = GetParagraphByIndex(startPos, &indexInPara);
    UPInt runLen = endPos - startPos;
    while(!paraIter.IsFinished())
    {
        GFxTextParagraph* ppara = *paraIter;
        GASSERT(ppara);
        UPInt paraLength = ppara->GetLength();
        if (indexInPara + runLen > paraLength)
            endIndexInPara = indexInPara + paraLength - indexInPara;
        else
            endIndexInPara = indexInPara + runLen;

        if (endIndexInPara == paraLength && ppara->HasTermNull())
        {
            ++endIndexInPara; // include the termination null symbol
            if (runLen != GFC_MAX_UPINT) // preventing overflowing
                ++runLen;
        }

        ppara->SetTextFormat(pTextAllocator, fmt, indexInPara, endIndexInPara);

        GASSERT(runLen >= (endIndexInPara - indexInPara));
        runLen -= (endIndexInPara - indexInPara);
        indexInPara = 0;
        ++paraIter;
    }
    if (fmt.IsUrlSet())
        SetMayHaveUrl();
    CheckIntegrity();
}

void GFxStyledText::SetParagraphFormat(const GFxTextParagraphFormat& fmt, UPInt startPos, UPInt endPos)
{
    GASSERT(endPos >= startPos);
    UPInt indexInPara, endIndexInPara;
    ParagraphsIterator paraIter = GetParagraphByIndex(startPos, &indexInPara);
    UPInt runLen = endPos - startPos;
    while(!paraIter.IsFinished())
    {
        GFxTextParagraph* ppara = *paraIter;
        GASSERT(ppara);
        if (indexInPara == 0)
        {
            ppara->SetFormat(pTextAllocator, fmt);
        }
        if (runLen == 0)
            break;
        UPInt paraLength = ppara->GetLength();
        if (runLen > paraLength)
            endIndexInPara = indexInPara + paraLength - indexInPara;
        else
            endIndexInPara = indexInPara + runLen;
        GASSERT(runLen >= (endIndexInPara - indexInPara));
        runLen -= (endIndexInPara - indexInPara);
        indexInPara = 0;
        ++paraIter;
    }

    CheckIntegrity();
}

void GFxStyledText::ClearTextFormat(UPInt startPos, UPInt endPos)
{
    GASSERT(endPos >= startPos);
    UPInt indexInPara, endIndexInPara;
    ParagraphsIterator paraIter = GetParagraphByIndex(startPos, &indexInPara);
    UPInt runLen = endPos - startPos;
    while(!paraIter.IsFinished())
    {
        GFxTextParagraph* ppara = *paraIter;
        GASSERT(ppara);
        UPInt paraLength = ppara->GetLength();
        if (indexInPara + runLen > paraLength)
            endIndexInPara = indexInPara + paraLength - indexInPara;
        else
            endIndexInPara = indexInPara + runLen;

        if (endIndexInPara == paraLength && ppara->HasTermNull())
        {
            ++endIndexInPara; // include the termination null symbol
            if (runLen != GFC_MAX_UPINT) // preventing overflowing
                ++runLen;
        }

        ppara->ClearTextFormat(indexInPara, endIndexInPara);

        GASSERT(runLen >= (endIndexInPara - indexInPara));
        runLen -= (endIndexInPara - indexInPara);
        indexInPara = 0;
        ++paraIter;
    }
}

void GFxStyledText::GetTextAndParagraphFormat
(GFxTextFormat* pdestTextFmt, GFxTextParagraphFormat* pdestParaFmt, UPInt startPos, UPInt endPos)
{
    GASSERT(endPos >= startPos);
    GASSERT(pdestParaFmt || pdestTextFmt);

    UPInt indexInPara;
    UPInt runLen = endPos - startPos;
    ParagraphsIterator paraIter = GetParagraphByIndex(startPos, &indexInPara);
    GFxTextFormat finalTextFmt;
    GFxTextParagraphFormat finalParaFmt;
    int i = 0, pi = 0;
    while(runLen > 0 && !paraIter.IsFinished())
    {
        GFxTextParagraph* ppara = *paraIter;
        GASSERT(ppara);
        UPInt lengthInPara = GTL::gpmin(runLen, ppara->GetLength());
        if (lengthInPara == 0)
            break; // the only empty paragraph could be the last one, so, we good to break
        if (i++ == 0)
            finalTextFmt = ppara->GetTextFormat(indexInPara, indexInPara + lengthInPara);
        else
            finalTextFmt = ppara->GetTextFormat(indexInPara, indexInPara + lengthInPara).Intersection(finalTextFmt);
        if (indexInPara == 0)
        {
            const GFxTextParagraphFormat* ppfmt = ppara->GetFormat();
            if (ppfmt)
            {
                if (pi++ == 0)
                    finalParaFmt = *ppfmt;
                else
                    finalParaFmt = ppfmt->Intersection(finalParaFmt);
            }
        }
        GASSERT(runLen >= lengthInPara);
        runLen -= lengthInPara;
        ++paraIter;
    }
    if (pdestTextFmt)
        *pdestTextFmt = finalTextFmt;
    if (pdestParaFmt)
        *pdestParaFmt = finalParaFmt;
}

bool GFxStyledText::GetTextAndParagraphFormat
(const GFxTextFormat** ppdestTextFmt, const GFxTextParagraphFormat** ppdestParaFmt, UPInt pos)
{
    GASSERT(ppdestParaFmt || ppdestTextFmt);

    UPInt indexInPara;
    ParagraphsIterator paraIter = GetParagraphByIndex(pos, &indexInPara);
    const GFxTextParagraphFormat* pparaFmt = NULL;
    const GFxTextFormat* ptextFmt = NULL;
    bool rv = false;
    if (!paraIter.IsFinished())
    {
        GFxTextParagraph* ppara = *paraIter;
        GASSERT(ppara);
        ptextFmt = ppara->GetTextFormatPtr(indexInPara);
        pparaFmt = ppara->GetFormat();
        rv = true;
    }
    if (ptextFmt == NULL)
        ptextFmt = pDefaultTextFormat;
    if (pparaFmt == NULL)
        pparaFmt = pDefaultParagraphFormat;
    if (ppdestTextFmt)
        *ppdestTextFmt = ptextFmt;
    if (ppdestParaFmt)
        *ppdestParaFmt = pparaFmt;
    return rv;
}

void GFxStyledText::SetDefaultTextFormat(const GFxTextFormat& defaultTextFmt)
{
    if (defaultTextFmt.GetImageDesc())
    {
        GFxTextFormat textfmt = defaultTextFmt;
        textfmt.SetImageDesc(NULL);
        pDefaultTextFormat = *GetAllocator()->AllocateTextFormat(textfmt);
    }
    else 
        pDefaultTextFormat = *GetAllocator()->AllocateTextFormat(defaultTextFmt);
    GASSERT(pDefaultTextFormat);
}

void GFxStyledText::SetDefaultParagraphFormat(const GFxTextParagraphFormat& defaultParagraphFmt)
{
    pDefaultParagraphFormat = *GetAllocator()->AllocateParagraphFormat(defaultParagraphFmt);
    GASSERT(pDefaultParagraphFormat);
}

void GFxStyledText::SetDefaultTextFormat(const GFxTextFormat* pdefaultTextFmt)
{
    GASSERT(pdefaultTextFmt);
    if (pdefaultTextFmt->GetImageDesc())
    {
        SetDefaultTextFormat(*pdefaultTextFmt);
    }
    else
    {
        pDefaultTextFormat = const_cast<GFxTextFormat*>(pdefaultTextFmt);
    }
}

void GFxStyledText::SetDefaultParagraphFormat(const GFxTextParagraphFormat* pdefaultParagraphFmt)
{
    GASSERT(pdefaultParagraphFmt);
    pDefaultParagraphFormat = const_cast<GFxTextParagraphFormat*>(pdefaultParagraphFmt);
}


GFxString GFxStyledText::GetHtml() const
{
    GFxString retStr;
    ParagraphsIterator paraIter = GetParagraphIterator();
    for(UPInt np = 0, nn = Paragraphs.size(); !paraIter.IsFinished(); ++paraIter, ++np)
    {
        GFxTextParagraph* ppara = *paraIter;
        GASSERT(ppara);
        if (np + 1 == nn && ppara->GetLength() == 0)
            continue; // this is the last empty paragraph - ignore it
        retStr += "<TEXTFORMAT";
        GASSERT(ppara->GetFormat());
        const GFxTextParagraphFormat& paraFormat = *ppara->GetFormat();
        if (paraFormat.IsIndentSet())
        {
            retStr += " INDENT=\"";
            char buf[20];
            gfc_sprintf(buf, 20, "%d\"", paraFormat.GetIndent());
            retStr += buf;
        }
        if (paraFormat.IsBlockIndentSet())
        {
            retStr += " BLOCKINDENT=\"";
            char buf[20];
            gfc_sprintf(buf, 20, "%d\"", paraFormat.GetBlockIndent());
            retStr += buf;
        }
        if (paraFormat.IsLeftMarginSet())
        {
            retStr += " LEFTMARGIN=\"";
            char buf[20];
            gfc_sprintf(buf, 20, "%d\"", paraFormat.GetLeftMargin());
            retStr += buf;
        }
        if (paraFormat.IsRightMarginSet())
        {
            retStr += " RIGHTMARGIN=\"";
            char buf[20];
            gfc_sprintf(buf, 20, "%d\"", paraFormat.GetRightMargin());
            retStr += buf;
        }
        if (paraFormat.IsLeadingSet())
        {
            retStr += " LEADING=\"";
            char buf[20];
            gfc_sprintf(buf, 20, "%d\"", paraFormat.GetLeading());
            retStr += buf;
        }
        if (paraFormat.IsTabStopsSet())
        {
            retStr += " TABSTOPS=\"";
            UInt n;
            const UInt *ptabStops = paraFormat.GetTabStops(&n);
            for (UInt i = 0; i < n; ++i)
            {
                char buf[20];
                if (i > 0)
                    retStr += ",";
                gfc_sprintf(buf, 20, "%d", ptabStops[i]);
                retStr += buf;
            }
            retStr += "\"";
        }
        retStr += "><";
        if (!paraFormat.IsBullet())
            retStr += "P";
        else
            retStr += "LI";
        retStr += " ALIGN=\"";
        switch(paraFormat.GetAlignment())
        {
        case GFxTextParagraphFormat::Align_Left: retStr += "LEFT"; break;
        case GFxTextParagraphFormat::Align_Right: retStr += "RIGHT"; break;
        case GFxTextParagraphFormat::Align_Center: retStr += "CENTER"; break;
        case GFxTextParagraphFormat::Align_Justify: retStr += "JUSTIFY"; break;
        }
        retStr += "\">";
        GFxTextParagraph::FormatRunIterator it = ppara->GetIterator();
        for(; !it.IsFinished(); ++it)
        {
            const GFxTextParagraph::StyledTextRun& run = *it;
            if (run.Length == 1 && run.pText[0] == NewLineChar())
                break; // a spec case - do not include format for newline char into HTML,
            // if this format is different from rest of the text.
            bool isImageTag = false;
            if (run.pFormat)
            {
                GPtr<GFxTextHTMLImageTagDesc> pimage = run.pFormat->GetImageDesc();
                GASSERT(!pimage || run.pFormat->IsImageDescSet());
                if (pimage)
                {
                    if (pimage)
                    {
                        isImageTag = true;
                        retStr += "<IMG SRC=\"";

                        retStr += pimage->Url;
                        retStr += "\"";
                        if (pimage->ScreenWidth > 0)
                        {
                            retStr += " WIDTH=\"";
                            char buf[20];
                            gfc_sprintf(buf, 20, "%d\"", TwipsToPixels(pimage->ScreenWidth));
                            retStr += buf;
                        }
                        if (pimage->ScreenHeight > 0)
                        {
                            retStr += " HEIGHT=\"";
                            char buf[20];
                            gfc_sprintf(buf, 20, "%d\"", TwipsToPixels(pimage->ScreenHeight));
                            retStr += buf;
                        }
                        if (pimage->VSpace != 0)
                        {
                            retStr += " VSPACE=\"";
                            char buf[20];
                            gfc_sprintf(buf, 20, "%d\"", TwipsToPixels(pimage->VSpace));
                            retStr += buf;
                        }
                        if (pimage->HSpace != 0)
                        {
                            retStr += " HSPACE=\"";
                            char buf[20];
                            gfc_sprintf(buf, 20, "%d\"", TwipsToPixels(pimage->HSpace));
                            retStr += buf;
                        }
                        if (!pimage->Id.IsEmpty())
                        {
                            retStr += " ID=\"";
                            retStr += pimage->Id;
                            retStr += "\"";
                        }
                        retStr += " ALIGN=\"";
                        switch(pimage->Alignment)
                        {
                        case GFxStyledText::HTMLImageTagInfo::Align_Left:
                            retStr += "left";
                            break;
                        case GFxStyledText::HTMLImageTagInfo::Align_Right:
                            retStr += "right";
                            break;
                        case GFxStyledText::HTMLImageTagInfo::Align_BaseLine:
                            retStr += "baseline";
                            break;
                        default: break;
                        }
                        retStr += "\">";
                    }
                }
                else
                {
                    retStr += "<FONT";
                    if (run.pFormat->IsFontListSet())
                    {
                        retStr += " FACE=\"";
                        retStr += run.pFormat->GetFontList();
                        retStr += "\"";
                    }
                    if (run.pFormat->IsFontSizeSet())
                    {
                        retStr += " SIZE=\"";
                        char buf[20];
                        gfc_sprintf(buf, 20, "%d\"", (UInt)run.pFormat->GetFontSize());
                        retStr += buf;
                    }
                    if (run.pFormat->IsColorSet())
                    {
                        retStr += " COLOR=\"#";
                        char buf[20];
                        gfc_sprintf(buf, 20, "%.6X\"", (run.pFormat->GetColor32() & 0xFFFFFFu));
                        retStr += buf;
                    }
                    if (run.pFormat->IsLetterSpacingSet())
                    {
                        retStr += " LETTERSPACING=\"";
                        char buf[20];
                        gfc_sprintf(buf, 20, "%f\"", run.pFormat->GetLetterSpacing());
                        retStr += buf;
                    }
                    if (run.pFormat->IsAlphaSet())
                    {
                        retStr += " ALPHA=\"#";
                        char buf[20];
                        gfc_sprintf(buf, 20, "%.2X\"", (run.pFormat->GetAlpha() & 0xFFu));
                        retStr += buf;
                    }
                    retStr += " KERNING=\"";
                    retStr += (run.pFormat->IsKerning()) ? "1" : "0";
                    retStr += "\"";

                    retStr += ">";
                    if (run.pFormat->IsUrlSet())
                    {
                        retStr += "<A HREF=\"";
                        retStr += run.pFormat->GetUrl();
                        retStr += "\">";
                    }

                    if (run.pFormat->IsBold())
                        retStr += "<B>";
                    if (run.pFormat->IsItalic())
                        retStr += "<I>";
                    if (run.pFormat->IsUnderline())
                        retStr += "<U>";
                }
            }

            if (!isImageTag)
            {
                for (UPInt i = 0; i < run.Length; ++i)
                {
                    if (run.pText[i] == NewLineChar() || run.pText[i] == '\0')
                        continue;
                    switch(run.pText[i])
                    {
                    case '<': retStr += "&lt;"; break;
                    case '>': retStr += "&gt;"; break;
                    case '&': retStr += "&amp;"; break;
                    case '\"': retStr += "&quot;"; break;
                    case '\'': retStr += "&apos;"; break;
                    case GFX_NBSP_CHAR_CODE: retStr += "&nbsp;"; break;
                    default: retStr.AppendChar(run.pText[i]);
                    }
                }
                if (run.pFormat)
                {
                    if (run.pFormat->IsUnderline())
                        retStr += "</U>";
                    if (run.pFormat->IsItalic())
                        retStr += "</I>";
                    if (run.pFormat->IsBold())
                        retStr += "</B>";
                    if (run.pFormat->IsUrlSet())
                        retStr += "</A>";
                    retStr += "</FONT>";
                }
            }
        }

        retStr += "</";
        if (!paraFormat.IsBullet())
            retStr += "P";
        else
            retStr += "LI";
        retStr += "></TEXTFORMAT>";
    }
    return retStr;
}

UPInt GFxStyledText::GetLength() const
{
    UPInt length = 0;
    ParagraphsIterator paraIter = GetParagraphIterator();
    for(; !paraIter.IsFinished(); ++paraIter)
    {
        GFxTextParagraph* ppara = *paraIter;
        GASSERT(ppara);
        length += ppara->GetLength();
    }
    return length;
}

GFxString GFxStyledText::GetText() const
{
    GFxString retStr;
    ParagraphsIterator paraIter = GetParagraphIterator();
    for(; !paraIter.IsFinished(); ++paraIter)
    {
        GFxTextParagraph* ppara = *paraIter;
        GASSERT(ppara);
        const wchar_t* pstr = ppara->GetText();
        retStr.AppendString(pstr, ppara->GetLength());
    }
    return retStr;
}

void GFxStyledText::GetText(GFxWStringBuffer* pBuffer) const
{
    GASSERT(pBuffer);

    pBuffer->Resize(GetLength() + 1);

    ParagraphsIterator paraIter = GetParagraphIterator();
    UPInt oldSz = 0;
    for(; !paraIter.IsFinished(); ++paraIter)
    {
        GFxTextParagraph* ppara = *paraIter;
        GASSERT(ppara);
        const wchar_t* pstr = ppara->GetText();
        UPInt paraLen = ppara->GetLength();
        memcpy(pBuffer->GetBuffer() + oldSz, pstr, sizeof(wchar_t) * paraLen);
        oldSz += paraLen;
    }
    (*pBuffer)[oldSz] = '\0';
}

void GFxStyledText::GetText(GFxWStringBuffer* pBuffer,UPInt startPos, UPInt endPos) const
{
    GASSERT(endPos >= startPos);
    GASSERT(pBuffer);

    if (endPos == GFC_MAX_UPINT)
        endPos = GetLength();
    UPInt len = endPos - startPos;

    pBuffer->Resize(len + 1);
    UPInt indexInPara = 0, remainedLen = len;
    ParagraphsIterator paraIter = GetParagraphByIndex(startPos, &indexInPara);
    UPInt oldSize = 0;
    for(; !paraIter.IsFinished() && remainedLen > 0; ++paraIter)
    {
        GFxTextParagraph* ppara = *paraIter;
        GASSERT(ppara);
        UPInt lenInPara = ppara->GetLength() - indexInPara;
        if (lenInPara > remainedLen)
            lenInPara = remainedLen;
        const wchar_t* pstr = ppara->GetText();

        memcpy(pBuffer->GetBuffer() + oldSize, pstr + indexInPara, sizeof(wchar_t) * lenInPara);
        oldSize += lenInPara;

        indexInPara = 0;
        remainedLen -= lenInPara;
        GASSERT(((SPInt)remainedLen) >= 0);
    }
    (*pBuffer)[oldSize] = '\0';
}

GFxStyledText* GFxStyledText::CopyStyledText(UPInt startPos, UPInt endPos) const
{
    GFxStyledText* pstyledText = new GFxStyledText(GetAllocator());
    CopyStyledText(pstyledText, startPos, endPos);
    return pstyledText;
}

void GFxStyledText::CopyStyledText(GFxStyledText* pdest, UPInt startPos, UPInt endPos) const
{
    GASSERT(endPos >= startPos);

    if (endPos == GFC_MAX_UPINT)
        endPos = GetLength();
    UPInt len = endPos - startPos;
    UPInt indexInPara = 0, remainedLen = len;
    pdest->Clear();
    pdest->OnTextInserting(startPos, len, "");
    ParagraphsIterator paraIter = GetParagraphByIndex(startPos, &indexInPara);
    if (!paraIter.IsFinished())
    {
        // if we need to take the only part of the first paragraph then
        // do Copy for this part. Otherwise, whole paragraph will be copied later.
        if (indexInPara != 0)
        {
            GFxTextParagraph* psrcPara = *paraIter;
            GFxTextParagraph* pdestPara = pdest->AppendNewParagraph(psrcPara->GetFormat());
            UPInt lenToCopy = psrcPara->GetLength() - indexInPara;
            lenToCopy = GTL::gpmin(lenToCopy, len);
            pdestPara->Copy(pdest->GetAllocator(), *psrcPara, indexInPara, 0, lenToCopy);
            remainedLen -= lenToCopy;
            ++paraIter;
        }
        for(; !paraIter.IsFinished() && remainedLen > 0; ++paraIter)
        {
            GFxTextParagraph* ppara = *paraIter;
            GASSERT(ppara);
            UPInt lenInPara = ppara->GetLength();
            if (lenInPara > remainedLen)
            {
                // copy the part of the last paragraph
                GFxTextParagraph* psrcPara = *paraIter;
                GFxTextParagraph* pdestPara = pdest->AppendNewParagraph(psrcPara->GetFormat());
                pdestPara->Copy(pdest->GetAllocator(), *psrcPara, 0, 0, remainedLen);
                break;
            }
            pdest->AppendCopyOfParagraph(*ppara);
            remainedLen -= lenInPara;
            GASSERT(((SPInt)remainedLen) >= 0);
        }
    }
    // check the last char. If it is \n - create the empty paragraph
    GFxTextParagraph* plastPara = pdest->GetLastParagraph();
    if (plastPara && plastPara->HasNewLine())
    {
        pdest->AppendNewParagraph(plastPara->GetFormat());
    }

    pdest->EnsureTermNull();
    if (MayHaveUrl())
        pdest->SetMayHaveUrl();
    pdest->CheckIntegrity();
}

wchar_t* GFxStyledText::CreatePosition(UPInt pos, UPInt length)
{
    GUNUSED2(pos, length);
    return 0;
}

UPInt GFxStyledText::InsertString(const wchar_t* pstr, UPInt pos, UPInt length, NewLinePolicy newLinePolicy)
{
    return InsertString(pstr, pos, length, newLinePolicy, GetDefaultTextFormat(), GetDefaultParagraphFormat());
}

UPInt GFxStyledText::InsertString(const wchar_t* pstr, UPInt pos, UPInt length, 
                                  NewLinePolicy newLinePolicy,
                                  const GFxTextFormat* pdefTextFmt, 
                                  const GFxTextParagraphFormat* pdefParaFmt)
{
    if (length == 0)
        return 0;
    if (pos > GetLength())
        pos = GetLength();

    // Insert plain string at the position. Style will be inherited from the current style
    // at the insertion position. If there is no style at the insertion position, then
    // text will remain plain. New paragraph will be created after each CR.
    if (length == GFC_MAX_UPINT)
        length = GFxTextParagraph::TextBuffer::StrLen(pstr);

    OnTextInserting(pos, length, pstr);

    UPInt indexInPara = 0;
    UPInt remainingSrcStrLen = length;
    SPInt newLineIdx;
    ParagraphsIterator paraIter = GetNearestParagraphByIndex(pos, &indexInPara);

    UPInt insLineLen;
    UPInt nextParaStartingPos = (!paraIter.IsFinished()) ? (*paraIter)->GetStartIndex() : 0;
    wchar_t uniChar = 0;
    UPInt totalInsertedLen = 0;

    do
    {
        if (paraIter.IsFinished())
        {
            // ok, there is no more (or none) paragraphs - create a new one
            AppendNewParagraph(pdefParaFmt);
            paraIter = GetParagraphIterator();
            indexInPara = 0;
        }
        GFxTextParagraph* ppara = *paraIter;
        GASSERT(ppara);
        if (ppara->IsEmpty())
        {
            // if paragraph is empty we can apply the pdefParaFmt
            ppara->SetFormat(pTextAllocator, *pdefParaFmt);
        }

        insLineLen = 0;
        newLineIdx = -1;
        // check, do we need to skip extra new line char in the case if "\r\n" EOL
        // is used.
        if (newLinePolicy == NLP_CompressCRLF && uniChar == '\r' && pstr[insLineLen] == '\n')
        {
            ++pstr;
            --remainingSrcStrLen;
            if (remainingSrcStrLen == 0)
                break; // it is possible that that was the last char.
        }
        // look for a new-line char in the line being inserted.
        for(; insLineLen < remainingSrcStrLen; ++insLineLen)
        {
            uniChar = pstr[insLineLen];
            if (uniChar == '\n' || uniChar == '\r')
            {
                newLineIdx = insLineLen;
                break;
            }
            else if (uniChar == '\0')
                break;
        }
        if (uniChar == '\n' || uniChar == '\r')
            ++insLineLen; // skip \n

        // was there a new-line symbol?
        if (newLineIdx != -1)
        {
            // yes, make another paragraph first
            ParagraphsIterator newParaIter = paraIter;
            ++newParaIter;
            GFxTextParagraph& newPara = *InsertNewParagraph(newParaIter, pdefParaFmt);

            // new paragraph should inherit style from the previous ppara->
            newPara.SetFormat(ppara->GetFormat());
            GASSERT(newPara.GetFormat());

            // now, copy the text to the new para
            newPara.Copy(pTextAllocator, *ppara, indexInPara, 0, ppara->GetSize() - indexInPara);

            UPInt remLen = ppara->GetSize();
            GASSERT(remLen >= indexInPara);
            remLen -= indexInPara;

            // now, we can insert the sub-string in source para
            ppara->InsertString(pTextAllocator, pstr, indexInPara, insLineLen, pdefTextFmt);

            if (remLen > 0)
            {
                // now, make source paragraph shorter
                ppara->Shrink(remLen);
            }
        }
        else
        {
            // the string being inserted doesn't contain any '\n'
            // so, insert it as a substring
            ppara->InsertString(pTextAllocator, pstr, indexInPara, insLineLen, pdefTextFmt);
        }
        // do we need to replace the newline char?
        if ((uniChar == '\r' || uniChar == '\n') && uniChar != NewLineChar())
        {
            wchar_t* ptxt = ppara->GetText();
            GASSERT(ptxt[indexInPara + insLineLen - 1] == '\r' || 
                ptxt[indexInPara + insLineLen - 1] == '\n');
            ptxt[indexInPara + insLineLen - 1] = NewLineChar();
        }

        GASSERT(remainingSrcStrLen >= insLineLen);
        remainingSrcStrLen -= insLineLen;
        pstr += insLineLen;
        totalInsertedLen += insLineLen;
        indexInPara = 0;
        ppara->SetStartIndex(nextParaStartingPos);
        nextParaStartingPos += ppara->GetSize();
        ++paraIter;
    } while(remainingSrcStrLen > 0 && uniChar != '\0');

    // correct the starting indices for remaining paragraphs
    for(; !paraIter.IsFinished(); ++paraIter)
    {
        GFxTextParagraph* ppara = *paraIter;
        GASSERT(ppara);
        ppara->SetStartIndex(nextParaStartingPos);
        nextParaStartingPos += ppara->GetSize();
    }

    EnsureTermNull();

    if (pdefTextFmt->IsUrlSet())
        SetMayHaveUrl();
    CheckIntegrity();
    return totalInsertedLen;
}

UPInt GFxStyledText::InsertStyledText(const GFxStyledText& text, UPInt pos, UPInt length)
{
    UPInt textLen = text.GetLength();
    if (length == GFC_MAX_UPINT || length > textLen)
        length = textLen;
    if (length == 0 || text.Paragraphs.size() == 0)
        return 0;
    OnTextInserting(pos, length, ""); //??

    UPInt indexInPara = 0;
    ParagraphsIterator destParaIter = GetNearestParagraphByIndex(pos, &indexInPara);

    if (destParaIter.IsFinished())
    {
        // ok, there is no paragraphs - create a new one
        AppendNewParagraph();
        destParaIter = GetParagraphIterator();
        indexInPara = 0;
    }

    UPInt nextParaStartingPos = (!destParaIter.IsFinished()) ? (*destParaIter)->GetStartIndex() : 0;
    if (text.Paragraphs.size() == 1)
    {
        // special case if there is only one para to insert
        GFxTextParagraph* pdestPara = *destParaIter;
        const GFxTextParagraph* psrcPara = text.Paragraphs[0];
        GASSERT(psrcPara && pdestPara);
        pdestPara->Copy(pTextAllocator, *psrcPara, 0, indexInPara, psrcPara->GetLength());            
        if (indexInPara == 0)
            pdestPara->SetFormat(psrcPara->GetFormat());

        nextParaStartingPos += pdestPara->GetSize();
        ++destParaIter;
    }
    else
    {   
        UPInt remainedLen = length;
        ParagraphsIterator srcParaIter = text.GetParagraphIterator();
        GASSERT(!srcParaIter.IsFinished());

        // divide the target paragraph on two parts
        GFxTextParagraph* ppara = *destParaIter;
        ParagraphsIterator newParaIter = destParaIter;
        ++newParaIter;
        GFxTextParagraph& newPara = *InsertNewParagraph(newParaIter, ppara->GetFormat());

        // now, copy the text to the new para
        newPara.Copy(pTextAllocator, *ppara, indexInPara, 0, ppara->GetSize() - indexInPara);

        UPInt remLen = ppara->GetSize();
        GASSERT(remLen >= indexInPara);
        remLen -= indexInPara;

        // now, we can insert the first source paragraph
        GFxTextParagraph* psrcPara = *srcParaIter;
        GASSERT(psrcPara);
        ppara->Copy(pTextAllocator, *psrcPara, 0, indexInPara, psrcPara->GetLength());
        remainedLen -= psrcPara->GetLength();
        GASSERT(((SPInt)remainedLen) >= 0);
        if (indexInPara == 0)
            ppara->SetFormat(pTextAllocator, *psrcPara->GetFormat());

        if (remLen > 0)
        {
            // now, make source paragraph shorter
            ppara->Shrink(remLen);
        }
        nextParaStartingPos += ppara->GetLength();

        ++destParaIter;
        ++srcParaIter;
        // now we can insert remaining paras
        for(; !srcParaIter.IsFinished() && remainedLen > 0; ++srcParaIter, ++destParaIter)
        {
            const GFxTextParagraph* psrcPara = *srcParaIter;
            GASSERT(psrcPara);
            UPInt lenInPara = psrcPara->GetLength();

            // check, do we need to copy the remaining text to the last
            // paragraph, or we still need to create a new paragraph. 
            // Even if length are equal but src para has newline - we
            // still just copy the whole paragraph. Another case, when length
            // are equal is when src paragraph contains term zero. In this case
            // we need to copy paragraph partially (w/o trailing zero).
            if (lenInPara > remainedLen || (lenInPara == remainedLen && !psrcPara->HasNewLine()))
            {
                // copy the part of the last paragraph
                newPara.Copy(pTextAllocator, *psrcPara, 0, 0, lenInPara);
                newPara.SetFormat(pTextAllocator, *psrcPara->GetFormat());
                break;
            }
            InsertCopyOfParagraph(destParaIter, *psrcPara);
            remainedLen -= lenInPara;
            nextParaStartingPos += lenInPara;
            GASSERT(((SPInt)remainedLen) >= 0);
        }
        // update the starting pos for the newPara (the last affected paragraph)
        newPara.SetStartIndex(nextParaStartingPos);
        nextParaStartingPos += newPara.GetLength();
        ++destParaIter;
    }

    // correct the starting indices for remaining paragraphs
    for(; !destParaIter.IsFinished(); ++destParaIter)
    {
        GFxTextParagraph* ppara = *destParaIter;
        GASSERT(ppara);
        if (ppara->GetStartIndex() == nextParaStartingPos)
            break;
        ppara->SetStartIndex(nextParaStartingPos);
        nextParaStartingPos += ppara->GetSize();
    }
    EnsureTermNull();
    if (text.MayHaveUrl())
        SetMayHaveUrl();
    CheckIntegrity();
    return length;
}

void GFxStyledText::RemoveParagraph(ParagraphsIterator& paraIter, GFxTextParagraph* ppara)
{
    OnParagraphRemoving(*ppara);
    ppara->FreeText(pTextAllocator);
    paraIter.remove();
}

void GFxStyledText::Remove(UPInt startPos, UPInt length)
{
    if (length == GFC_MAX_UPINT)
        length = GetLength();

    OnTextRemoving(startPos, length);

    UPInt indexInPara;
    ParagraphsIterator paraIter = GetParagraphByIndex(startPos, &indexInPara);
    GFxTextParagraph* pprevPara = NULL;

    UPInt remainingLen = length;
    bool needUniteParas = false;
    // first stage - remove part of this para
    if (!paraIter.IsFinished())
    {
        GFxTextParagraph* ppara = *paraIter;
        GASSERT(ppara);

        UPInt lengthInPara = GTL::gpmin(remainingLen, ppara->GetSize() - indexInPara);
        if (lengthInPara <= ppara->GetSize())
        {
            needUniteParas = (indexInPara + lengthInPara >= ppara->GetSize());
            pprevPara = *paraIter;
            ppara->Remove(indexInPara, indexInPara + lengthInPara);
            remainingLen -= lengthInPara;
            ++paraIter;
        }
    }

    // second stage - remove all paragraphs we can remove completely
    while(!paraIter.IsFinished())
    {
        GFxTextParagraph* ppara = *paraIter;
        GASSERT(ppara);
        UPInt paraLen = ppara->GetSize();
        if (remainingLen >= paraLen)
        {
            RemoveParagraph(paraIter, ppara);
            remainingLen -= paraLen;
        }
        else if (pprevPara)
        {
            if (needUniteParas)
            {
                // 3rd stage: do a copy of remaining part of the last para
                UPInt insPos = pprevPara->GetSize();
                UPInt insLen = paraLen - remainingLen;
                pprevPara->Copy(pTextAllocator, *ppara, remainingLen, insPos, insLen);

                RemoveParagraph(paraIter, ppara);
                needUniteParas = false;
            }
            break;
        }
        else  
            break; //???
        if (remainingLen == 0) 
            break;
    }
    if (!paraIter.IsFinished())
    {
        GFxTextParagraph* ppara = *paraIter;
        if (ppara->GetSize() == 0)
        {
            RemoveParagraph(paraIter, ppara);
        }
        else if (pprevPara && needUniteParas)
        {
            // 3rd stage, if remainingLen == 0: do a copy of remaining part of the last para
            UPInt paraLen = ppara->GetSize();
            UPInt insPos = pprevPara->GetSize();
            UPInt insLen = paraLen;
            pprevPara->Copy(pTextAllocator, *ppara, 0, insPos, insLen);

            RemoveParagraph(paraIter, ppara);
        }
    }

    // correct the starting indices for remaining paragraphs
    for(; !paraIter.IsFinished(); ++paraIter)
    {
        GFxTextParagraph* ppara = *paraIter;
        GASSERT(ppara);
        ppara->SetStartIndex(ppara->GetStartIndex() - length);
    }
    EnsureTermNull();
    CheckIntegrity();
}

void GFxStyledText::OnTextInserting(UPInt startPos, UPInt length, const wchar_t* ptxt)
{
    GUNUSED3(startPos, length, ptxt);
}

void GFxStyledText::OnTextRemoving(UPInt startPos, UPInt length)
{
    GUNUSED2(startPos, length);
}

/*
void GFxStyledText::OnParagraphTextFormatChanging(const GFxTextParagraph& para, UPInt startPos, UPInt endPos, const GFxTextFormat& formatToBeSet)
{
GUNUSED(para);
GUNUSED3(startPos, endPos, formatToBeSet);
}
void GFxStyledText::OnParagraphTextFormatChanged(const GFxTextParagraph& para, UPInt startPos, UPInt endPos)
{
GUNUSED(para);
GUNUSED2(startPos, endPos);
}

void GFxStyledText::OnParagraphFormatChanging(const GFxTextParagraph& para, const GFxTextParagraphFormat& formatToBeSet)
{
GUNUSED(para);
GUNUSED(formatToBeSet);
}
void GFxStyledText::OnParagraphFormatChanged(const GFxTextParagraph& para)
{
GUNUSED(para);
}

void GFxStyledText::OnParagraphTextInserting(const GFxTextParagraph& para, UPInt insertionPos, UPInt insertingLen)
{
GUNUSED(para);
GUNUSED2(insertionPos, insertingLen);
}

void GFxStyledText::OnParagraphTextInserted(const GFxTextParagraph& para, UPInt startPos, UPInt endPos, const wchar_t* ptextInserted)
{
GUNUSED(para);
GUNUSED3(startPos, endPos, ptextInserted);
}

void GFxStyledText::OnParagraphTextRemoving(const GFxTextParagraph& para, UPInt removingPos, UPInt removingLen)
{
GUNUSED(para);
GUNUSED2(removingPos, removingLen);
}

void GFxStyledText::OnParagraphTextRemoved(const GFxTextParagraph& para, UPInt removedPos, UPInt removedLen)
{
GUNUSED(para);
GUNUSED2(removedPos, removedLen);
}
*/
void GFxStyledText::OnParagraphRemoving(const GFxTextParagraph& para)
{
    GUNUSED(para);
}

#ifdef GFC_BUILD_DEBUG
void GFxStyledText::CheckIntegrity() const
{
    ParagraphsIterator paraIter = GetParagraphIterator();
    UPInt pos = 0;
    for(; !paraIter.IsFinished(); ++paraIter)
    {
        GFxTextParagraph* ppara = *paraIter;
        GASSERT(ppara);
        GASSERT(ppara->GetSize() != 0);
        GASSERT(ppara->GetLength() != 0 || GetLastParagraph() == ppara);
        ppara->CheckIntegrity();
        GASSERT(pos == ppara->GetStartIndex());
        pos += ppara->GetLength();

        // only the last paragraph MUST have the terminal null
        GASSERT(GetLastParagraph() != ppara || ppara->HasTermNull());
    }
}
#endif

//////////////////////////////////////////////////////////////////////////
// GFxStyledText's iterators
//
GFxStyledText::CharactersIterator::CharactersIterator(GFxStyledText* ptext) : 
Paragraphs(ptext->GetParagraphIterator()), pText(ptext), FirstCharInParagraphIndex(0)
{
    if (!Paragraphs.IsFinished())
    {
        GFxTextParagraph* ppara = *Paragraphs;
        GASSERT(ppara);
        FirstCharInParagraphIndex = ppara->GetStartIndex();
        Characters = ppara->GetCharactersIterator();
    }
}
GFxStyledText::CharactersIterator::CharactersIterator(GFxStyledText* ptext, SInt index) : pText(ptext)
{
    UPInt indexInPara;
    Paragraphs = ptext->GetParagraphByIndex(index, &indexInPara);
    if (!Paragraphs.IsFinished())
    {
        GFxTextParagraph* ppara = *Paragraphs;
        GASSERT(ppara);
        FirstCharInParagraphIndex = ppara->GetStartIndex();
        Characters = ppara->GetCharactersIterator(indexInPara);
    }
}

GFxStyledText::CharacterInfo& GFxStyledText::CharactersIterator::operator*() 
{
    GFxTextParagraph::CharacterInfo& chInfo = *Characters;
    CharInfoPlaceHolder.Character = chInfo.Character;
    CharInfoPlaceHolder.Index     = chInfo.Index + FirstCharInParagraphIndex;
    CharInfoPlaceHolder.pOriginalFormat = chInfo.pFormat;
    CharInfoPlaceHolder.pParagraph = *Paragraphs;
    return CharInfoPlaceHolder;
}

void GFxStyledText::CharactersIterator::operator++() 
{ 
    ++Characters;
    if (Characters.IsFinished())
    {
        ++Paragraphs;
        if (!Paragraphs.IsFinished())
        {
            GFxTextParagraph* ppara = *Paragraphs;
            GASSERT(ppara);
            FirstCharInParagraphIndex = ppara->GetStartIndex();
            Characters = ppara->GetCharactersIterator();
        }
    }
}

void GFxStyledText::Clear()
{
    for (UPInt i = 0, n = Paragraphs.size(); i < n; ++i)
        Paragraphs[i]->FreeText(GetAllocator());
    Paragraphs.resize(0);
    ClearMayHaveUrl();
}

//////////////////////////////////////////////////////////////////////////
// HTML parsing routine
//
enum GFxHTMLElementsEnum
{
    GFxHTML_A,
    GFxHTML_B,
    GFxHTML_BR,
    GFxHTML_FONT,
    GFxHTML_I,
    GFxHTML_IMG,
    GFxHTML_LI,
    GFxHTML_P,
    GFxHTML_SPAN,
    GFxHTML_TAB,
    GFxHTML_TEXTFORMAT,
    GFxHTML_U,

    GFxHTML_ALIGN,
    GFxHTML_ALPHA,
    GFxHTML_BASELINE,
    GFxHTML_BLOCKINDENT,
    GFxHTML_CLASS,
    GFxHTML_COLOR,
    GFxHTML_FACE,
    GFxHTML_HEIGHT,
    GFxHTML_HREF,
    GFxHTML_HSPACE,
    GFxHTML_ID,
    GFxHTML_INDENT,
    GFxHTML_KERNING,
    GFxHTML_LEADING,
    GFxHTML_LEFTMARGIN,
    GFxHTML_LETTERSPACING,
    GFxHTML_RIGHTMARGIN,
    GFxHTML_SRC,
    GFxHTML_SIZE,
    GFxHTML_TARGET,
    GFxHTML_TABSTOPS,
    GFxHTML_VSPACE,
    GFxHTML_WIDTH,

    GFxHTML_LEFT,
    GFxHTML_RIGHT,
    GFxHTML_CENTER,
    GFxHTML_JUSTIFY
};

struct GFxSGMLElementDesc
{
    const char*         ElemName;
    GFxHTMLElementsEnum ElemId;
    bool                ReqEndElement;
    bool                EmptyElement;

    template <class Char>
    struct Comparable
    {
        const Char* Str;
        UPInt      Size;

        Comparable(const Char* p, UPInt sz):Str(p), Size(sz) {}
    };

    template <class Char>
    struct Comparator
    {
        int Compare(const GFxSGMLElementDesc& p1, const Comparable<Char>& p2) const
        {
            return -GFxSGMLCharIter<Char>::StrCompare(p2.Str, p1.ElemName, p2.Size);
        }
    };
    template <class Char>
    static const GFxSGMLElementDesc* FindElem
        (const Char* lookForElemName, UPInt nameSize, const GFxSGMLElementDesc* ptable, int tableSize)
    {
        static Comparator<Char> cmp;
        Comparable<Char> e(lookForElemName, nameSize);
        SPInt i = GTL::find_index_in_sorted_primitive_array(ptable, tableSize, e, cmp);
        if (i != -1)
        {
            return &ptable[i];
        }
        return NULL;
    }
};

template <class Char>
struct GFxSGMLStackElemDesc
{
    const Char*                 pElemName;
    UPInt                       ElemNameSize;
    const GFxSGMLElementDesc*   pElemDesc;
    UPInt                       StartPos;
    GFxTextFormat               TextFmt;
    GFxTextParagraphFormat      ParaFmt;

    GFxSGMLStackElemDesc():pElemName(NULL),ElemNameSize(0),pElemDesc(NULL) {}
    GFxSGMLStackElemDesc(const Char* pname, UPInt sz, const GFxSGMLElementDesc* pelemDesc, UPInt startPos) : 
    pElemName(pname), ElemNameSize(sz), pElemDesc(pelemDesc), StartPos(startPos) {}
};

template <class Char>
bool GFxStyledText::ParseHtmlImpl(const Char* phtml, 
                                  UPInt htmlSize, 
                                  GFxStyledText::HTMLImageTagInfoArray* pimgInfoArr, 
                                  bool multiline, 
                                  bool condenseWhite,
                                  const GFxTextStyleManager* pstyleMgr)
{
    static const GFxSGMLElementDesc elementsTable[] = 
    {  // must be sorted by name!
        { "A",      GFxHTML_A,      true, false },
        { "B",      GFxHTML_B,      true, false },
        { "BR",     GFxHTML_BR,     false, true },
        { "FONT",   GFxHTML_FONT,   true, false },
        { "I",      GFxHTML_I,      true, false },
        { "IMG",    GFxHTML_IMG,    false, true },
        { "LI",     GFxHTML_LI,     false, false },
        { "P",      GFxHTML_P,      false, false },
        { "SBR",    GFxHTML_BR,     false, true },
        { "SPAN",   GFxHTML_SPAN,   true,  false },
        { "TAB",    GFxHTML_TAB,     false, true },
        { "TEXTFORMAT", GFxHTML_TEXTFORMAT, true, false },
        { "U",      GFxHTML_U,      true, false }
    };

    static const GFxSGMLElementDesc attributesTable[] = 
    {  // must be sorted by name!
        { "ALIGN",      GFxHTML_ALIGN },
        { "ALPHA",      GFxHTML_ALPHA },
        { "BASELINE",   GFxHTML_BASELINE },
        { "BLOCKINDENT",GFxHTML_BLOCKINDENT },
        { "CENTER",     GFxHTML_CENTER },
        { "CLASS",      GFxHTML_CLASS },
        { "COLOR",      GFxHTML_COLOR },
        { "FACE",       GFxHTML_FACE },
        { "HEIGHT",     GFxHTML_HEIGHT },
        { "HREF",       GFxHTML_HREF },
        { "HSPACE",     GFxHTML_HSPACE },
        { "ID",         GFxHTML_ID },
        { "INDENT",     GFxHTML_INDENT },
        { "JUSTIFY",    GFxHTML_JUSTIFY },
        { "KERNING",    GFxHTML_KERNING },
        { "LEADING",    GFxHTML_LEADING },
        { "LEFT",       GFxHTML_LEFT },
        { "LEFTMARGIN", GFxHTML_LEFTMARGIN },
        { "LETTERSPACING", GFxHTML_LETTERSPACING },
        { "RIGHT",      GFxHTML_RIGHT },
        { "RIGHTMARGIN", GFxHTML_RIGHTMARGIN },
        { "SIZE",       GFxHTML_SIZE },
        { "SRC",        GFxHTML_SRC },
        { "TABSTOPS",   GFxHTML_TABSTOPS },
        { "TARGET",     GFxHTML_TARGET },
        { "VSPACE",     GFxHTML_VSPACE },
        { "WIDTH",      GFxHTML_WIDTH }
    };

    GTL::garray<GFxSGMLStackElemDesc<Char> > elementsStack;

    GFxSGMLParser<Char> parser(phtml, htmlSize);
    if (condenseWhite)
        parser.SetCondenseWhite();

    int type;
    GFxTextParagraph* pcurPara = NULL;
    const GFxSGMLElementDesc* plastElemDesc = NULL;
    int p_elem_is_opened = 0;

    GFxTextFormat defaultTextFmt            = *GetDefaultTextFormat();
    GFxTextParagraphFormat defaultParaFmt   = *GetDefaultParagraphFormat();
    if (!defaultTextFmt.IsUrlSet())
    {
        // This is necessary to prevent URL-style propagation after
        // closing </A> tag.
        defaultTextFmt.SetUrl("");
        SetDefaultTextFormat(defaultTextFmt);
    }
    GFxTextFormat lastFormat = defaultTextFmt;

    while((type = parser.GetNext()) != SGMLPS_FINISHED && type != SGMLPS_ERROR)
    {
        switch(type)
        {
        case SGMLPS_START_ELEMENT:
            {
                const Char* elemName;
                UPInt elemLen;

                parser.ParseStartElement(&elemName, &elemLen);

                const GFxSGMLElementDesc* pelemDesc = GFxSGMLElementDesc::FindElem<Char>
                    (elemName, elemLen, elementsTable, sizeof(elementsTable)/sizeof(elementsTable[0]));
                plastElemDesc = pelemDesc;

                int prevStackIndex = int(elementsStack.size()-1);

                if (pelemDesc && pelemDesc->EmptyElement)
                {
                    switch(pelemDesc->ElemId)
                    {
                    case GFxHTML_BR:
                        // new paragraph
                        if (multiline)
                            AppendString(NewLineStr(), 1);
                        break;
                    case GFxHTML_TAB:
                        AppendString("\t", 1);
                        break;
                    case GFxHTML_IMG:
                        {
                            if (pimgInfoArr)
                            {
                                const Char* pattrName;
                                UPInt      attrSz;
                                pimgInfoArr->push_back(HTMLImageTagInfo());
                                HTMLImageTagInfo&  imgInfo = (*pimgInfoArr)[pimgInfoArr->size() - 1];
                                pcurPara = GetLastParagraph();
                                if (pcurPara == NULL)
                                    pcurPara = AppendNewParagraph(&defaultParaFmt);
                                GFxTextFormat imgCharFmt;
                                imgInfo.ParaId = pcurPara->GetId();
                                while(parser.GetNextAttribute(&pattrName, &attrSz))
                                {
                                    const GFxSGMLElementDesc* pattrDesc = GFxSGMLElementDesc::FindElem<Char>
                                        (pattrName, attrSz, attributesTable, sizeof(attributesTable)/sizeof(attributesTable[0]));
                                    if (pattrDesc)
                                    {
                                        const Char* pattrVal;
                                        UPInt      attrValSz;                                        
                                        if (parser.GetNextAttributeValue(&pattrVal, &attrValSz))
                                        {
                                            switch(pattrDesc->ElemId)
                                            {
                                            case GFxHTML_SRC:
                                                imgInfo.Url.AppendString(pattrVal, attrValSz);
                                                break;
                                            case GFxHTML_ID:
                                                imgInfo.Id.AppendString(pattrVal, attrValSz);
                                                break;
                                            case GFxHTML_ALIGN:
                                                pattrDesc = GFxSGMLElementDesc::FindElem<Char>
                                                    (pattrVal, attrValSz, attributesTable, sizeof(attributesTable)/sizeof(attributesTable[0]));
                                                if (pattrDesc)
                                                {
                                                    switch(pattrDesc->ElemId)
                                                    {
                                                    case GFxHTML_LEFT:
                                                        imgInfo.Alignment = HTMLImageTagInfo::Align_Left;
                                                        break;
                                                    case GFxHTML_RIGHT:
                                                        imgInfo.Alignment = HTMLImageTagInfo::Align_Right;
                                                        break;
                                                    case GFxHTML_BASELINE:
                                                        imgInfo.Alignment = HTMLImageTagInfo::Align_BaseLine;
                                                        break;
                                                    default: break;
                                                    }
                                                }
                                                break;
                                            case GFxHTML_WIDTH:
                                                {
                                                    int v;
                                                    if (parser.ParseInt(&v, pattrVal, attrValSz) && v >= 0)
                                                    {
                                                        imgInfo.Width = PixelsToTwips((UInt)v);
                                                    }
                                                }
                                                break;
                                            case GFxHTML_HEIGHT:
                                                {
                                                    int v;
                                                    if (parser.ParseInt(&v, pattrVal, attrValSz) && v >= 0)
                                                    {
                                                        imgInfo.Height = PixelsToTwips((UInt)v);
                                                    }
                                                }
                                                break;
                                            case GFxHTML_VSPACE:
                                                {
                                                    int v;
                                                    if (parser.ParseInt(&v, pattrVal, attrValSz))
                                                    {
                                                        imgInfo.VSpace = PixelsToTwips(v);
                                                    }
                                                }
                                                break;
                                            case GFxHTML_HSPACE:
                                                {
                                                    int v;
                                                    if (parser.ParseInt(&v, pattrVal, attrValSz))
                                                    {
                                                        imgInfo.HSpace = PixelsToTwips(v);
                                                    }
                                                }
                                                break;
                                            default: break;
                                            }
                                        }
                                    }
                                }
                                GPtr<GFxTextHTMLImageTagDesc> ptextImageDesc = *new GFxTextHTMLImageTagDesc;
                                imgCharFmt.SetImageDesc(ptextImageDesc);
                                ptextImageDesc->ScreenWidth     = imgInfo.Width;
                                ptextImageDesc->ScreenHeight    = imgInfo.Height;
                                ptextImageDesc->VSpace          = imgInfo.VSpace;
                                ptextImageDesc->HSpace          = imgInfo.HSpace;
                                ptextImageDesc->Url             = imgInfo.Url;
                                ptextImageDesc->Id              = imgInfo.Id;
                                ptextImageDesc->Alignment       = imgInfo.Alignment;
                                imgInfo.pTextImageDesc = ptextImageDesc;
                                GFxTextFormat* ptextFmt;
                                const GFxTextParagraphFormat* pparaFmt;
                                if (prevStackIndex >= 0)
                                {
                                    ptextFmt = &elementsStack[prevStackIndex].TextFmt;
                                    pparaFmt = &elementsStack[prevStackIndex].ParaFmt;
                                }
                                else
                                {
                                    ptextFmt = &defaultTextFmt;
                                    pparaFmt = &defaultParaFmt;
                                }
                                AppendString(" ", 1, &imgCharFmt, pparaFmt);
                                ptextFmt->SetImageDesc(NULL);
                                SetDefaultTextFormat(*ptextFmt);
                            }
                        }
                        break;
                    default: break;
                    }
                }
                else
                {
                    UPInt curPos = GetLength();
                    elementsStack.push_back(GFxSGMLStackElemDesc<Char>(elemName, elemLen, pelemDesc, 0));
                    GFxSGMLStackElemDesc<Char>& stackElem = elementsStack[elementsStack.size()-1];
                    stackElem.StartPos = curPos;
                    if (prevStackIndex >= 0)
                    {
                        stackElem.TextFmt = elementsStack[prevStackIndex].TextFmt;
                        stackElem.ParaFmt = elementsStack[prevStackIndex].ParaFmt;
                    }
                    else
                    {
                        stackElem.TextFmt = defaultTextFmt;
                        stackElem.ParaFmt = defaultParaFmt;
                    }

                    if (!pelemDesc)
                    {
                        #ifndef GFC_NO_CSS_SUPPORT
                        // apply CSS class for arbitrary tag
                        if (pstyleMgr)
                        {
                            const GFxTextStyle* pstyle =
                                pstyleMgr->GetStyle(GFxTextStyleKey::CSS_Tag, elemName, elemLen);
                            if (pstyle)
                            {
                                stackElem.ParaFmt = stackElem.ParaFmt.Merge(pstyle->ParagraphFormat);
                                stackElem.TextFmt = stackElem.TextFmt.Merge(pstyle->TextFormat);
                            }
                        }
                        #endif //GFC_NO_CSS_SUPPORT
                    }
                    else
                    {
                        switch(pelemDesc->ElemId)
                        {
                        case GFxHTML_A:
                            const Char* pattrName;
                            UPInt      attrSz;
                            while(parser.GetNextAttribute(&pattrName, &attrSz))
                            {
                                const GFxSGMLElementDesc* pattrDesc = GFxSGMLElementDesc::FindElem<Char>
                                    (pattrName, attrSz, attributesTable, sizeof(attributesTable)/sizeof(attributesTable[0]));
                                if (pattrDesc)
                                {
                                    const Char* pattrVal;
                                    size_t      attrValSz;                                        
                                    if (parser.GetNextAttributeValue(&pattrVal, &attrValSz))
                                    {
                                        switch(pattrDesc->ElemId)
                                        {
                                        case GFxHTML_HREF:
                                            stackElem.TextFmt.SetUrl(pattrVal, attrValSz);
                                            break;
                                        default: break;
                                        }
                                    }
                                }
                            }
                            #ifndef GFC_NO_CSS_SUPPORT
                            if (pstyleMgr)
                            {
                                // apply CSS style, if any
                                const GFxTextStyle* pstyle = 
                                    pstyleMgr->GetStyle(GFxTextStyleKey::CSS_Tag, "a");
                                if (pstyle)
                                {
                                    //?stackElem.ParaFmt = stackElem.ParaFmt.Merge(pstyle->ParagraphFormat);
                                    stackElem.TextFmt = stackElem.TextFmt.Merge(pstyle->TextFormat);
                                }
                                if (stackElem.TextFmt.IsUrlSet())
                                {
                                    // get a:link CSS style since there is a HREF attr
                                    const GFxTextStyle* pstyle = 
                                        pstyleMgr->GetStyle(GFxTextStyleKey::CSS_Tag, "a:link");
                                    if (pstyle)
                                    {
                                        //?stackElem.ParaFmt = stackElem.ParaFmt.Merge(pstyle->ParagraphFormat);
                                        stackElem.TextFmt = stackElem.TextFmt.Merge(pstyle->TextFormat);
                                    }
                                }
                            }
                            #endif //GFC_NO_CSS_SUPPORT
                            break;
                        case GFxHTML_LI:
                        {
                            stackElem.ParaFmt.SetBullet();
                            pcurPara = GetLastParagraph();
                            if (pcurPara == NULL)
                                pcurPara = AppendNewParagraph(&stackElem.ParaFmt);
                            else if (pcurPara->GetLength() == 0)
                                pcurPara->SetFormat(GetAllocator(), stackElem.ParaFmt);
                            else
                            {
                                if (multiline)
                                    AppendString(NewLineStr(), 1); 
                                pcurPara = GetLastParagraph();
                                pcurPara->SetFormat(GetAllocator(), stackElem.ParaFmt);
                            }
                            #ifndef GFC_NO_CSS_SUPPORT
                            // apply CSS class (on tag basis)
                            if (pstyleMgr)
                            {
                                const GFxTextStyle* pstyle =
                                    pstyleMgr->GetStyle(GFxTextStyleKey::CSS_Tag, elemName, elemLen);
                                if (pstyle)
                                {
                                    stackElem.ParaFmt = stackElem.ParaFmt.Merge(pstyle->ParagraphFormat);
                                    stackElem.TextFmt = stackElem.TextFmt.Merge(pstyle->TextFormat);
                                    pcurPara->SetFormat(GetAllocator(), stackElem.ParaFmt);
                                }
                            }
                            #endif //GFC_NO_CSS_SUPPORT
                            break;
                        }
                        case GFxHTML_P:
                        {
                            if ((pcurPara = GetLastParagraph()) == NULL)
                            {
                                // no paragraphs yet
                                pcurPara = AppendNewParagraph();
                            }
                            else if (p_elem_is_opened)
                            {
                                if (multiline)
                                    AppendString(NewLineStr(), 1);
                                pcurPara = GetLastParagraph();
                            }
                            #ifndef GFC_NO_CSS_SUPPORT
                            // apply CSS class (on tag basis)
                            if (pstyleMgr)
                            {
                                const GFxTextStyle* pstyle =
                                    pstyleMgr->GetStyle(GFxTextStyleKey::CSS_Tag, elemName, elemLen);
                                if (pstyle)
                                {
                                    stackElem.ParaFmt = stackElem.ParaFmt.Merge(pstyle->ParagraphFormat);
                                    stackElem.TextFmt = stackElem.TextFmt.Merge(pstyle->TextFormat);
                                }
                            }
                            #endif //GFC_NO_CSS_SUPPORT

                            // new paragraph
                            const Char* pattrName;
                            UPInt       attrSz;
                            while(parser.GetNextAttribute(&pattrName, &attrSz))
                            {
                                const GFxSGMLElementDesc* pattrDesc = GFxSGMLElementDesc::FindElem<Char>
                                    (pattrName, attrSz, attributesTable, sizeof(attributesTable)/sizeof(attributesTable[0]));
                                if (pattrDesc)
                                {
                                    const Char* pattrVal;
                                    UPInt       attrValSz;                                        
                                    if (parser.GetNextAttributeValue(&pattrVal, &attrValSz))
                                    {
                                        switch(pattrDesc->ElemId)
                                        {
                                        case GFxHTML_CLASS:
                                            {
                                                #ifndef GFC_NO_CSS_SUPPORT
                                                // apply CSS class
                                                if (pstyleMgr)
                                                {
                                                    const GFxTextStyle* pstyle =
                                                        pstyleMgr->GetStyle(GFxTextStyleKey::CSS_Class, pattrVal, attrValSz);
                                                    if (pstyle)
                                                    {
                                                        stackElem.ParaFmt = stackElem.ParaFmt.Merge(pstyle->ParagraphFormat);
                                                        stackElem.TextFmt = stackElem.TextFmt.Merge(pstyle->TextFormat);
                                                    }
                                                }
                                                #endif //GFC_NO_CSS_SUPPORT
                                            }
                                            break;
                                        case GFxHTML_ALIGN:
                                            {
                                                pattrDesc = GFxSGMLElementDesc::FindElem<Char>
                                                    (pattrVal, attrValSz, attributesTable, sizeof(attributesTable)/sizeof(attributesTable[0]));
                                                if (pattrDesc)
                                                {
                                                    switch(pattrDesc->ElemId)
                                                    {
                                                    case GFxHTML_LEFT:
                                                        stackElem.ParaFmt.SetAlignment(GFxTextParagraphFormat::Align_Left);
                                                        break;
                                                    case GFxHTML_RIGHT:
                                                        stackElem.ParaFmt.SetAlignment(GFxTextParagraphFormat::Align_Right);
                                                        break;
                                                    case GFxHTML_CENTER:
                                                        stackElem.ParaFmt.SetAlignment(GFxTextParagraphFormat::Align_Center);
                                                        break;
                                                    case GFxHTML_JUSTIFY:
                                                        stackElem.ParaFmt.SetAlignment(GFxTextParagraphFormat::Align_Justify);
                                                        break;
                                                    default: break;
                                                    }
                                                }
                                            }
                                            break;
                                        default: break;
                                        }
                                    }
                                }
                            }
                            if (pcurPara->GetLength() == 0)
                                pcurPara->SetFormat(GetAllocator(), stackElem.ParaFmt);
                            //}
                            ++p_elem_is_opened;
                            break;
                        }
                        case GFxHTML_B:
                            stackElem.TextFmt.SetBold(true);
                            break;
                        case GFxHTML_I:
                            stackElem.TextFmt.SetItalic(true);
                            break;
                        case GFxHTML_U:
                            stackElem.TextFmt.SetUnderline(true);
                            break;
                        case GFxHTML_FONT:
                            {
                                const Char* pattrName;
                                UPInt       attrSz;
                                while(parser.GetNextAttribute(&pattrName, &attrSz))
                                {
                                    const GFxSGMLElementDesc* pattrDesc = GFxSGMLElementDesc::FindElem<Char>
                                        (pattrName, attrSz, attributesTable, sizeof(attributesTable)/sizeof(attributesTable[0]));
                                    if (pattrDesc)
                                    {
                                        const Char* pattrVal;
                                        UPInt       attrValSz;                                        
                                        if (parser.GetNextAttributeValue(&pattrVal, &attrValSz))
                                        {
                                            switch(pattrDesc->ElemId)
                                            {
                                            case GFxHTML_COLOR:
                                                // parse color: #RRGGBB
                                                if (*pattrVal == '#')
                                                {
                                                    UInt32 color;
                                                    if (parser.ParseHexInt(&color, pattrVal+1, attrValSz-1))
                                                    {
                                                        stackElem.TextFmt.SetColor32(color);    
                                                    }
                                                }
                                                break;
                                            case GFxHTML_FACE:
                                                stackElem.TextFmt.SetFontList(pattrVal, attrValSz);
                                                break;
                                            case GFxHTML_SIZE:
                                                // parse size
                                                int sz;
                                                if (parser.ParseInt(&sz, pattrVal, attrValSz) && sz >= 0)
                                                {
                                                    stackElem.TextFmt.SetFontSize((Float)sz);
                                                }
                                                break;
                                            case GFxHTML_KERNING:
                                                // parse kerning
                                                int kern;
                                                if (parser.ParseInt(&kern, pattrVal, attrValSz) && kern >= 0)
                                                {
                                                    if (kern)
                                                        stackElem.TextFmt.SetKerning();
                                                    else
                                                        stackElem.TextFmt.ClearKerning();
                                                }
                                                break;
                                            case GFxHTML_LETTERSPACING:
                                                // parse letterspacing
                                                Float ls;
                                                if (parser.ParseFloat(&ls, pattrVal, attrValSz))
                                                {
                                                    stackElem.TextFmt.SetLetterSpacing(ls);
                                                }
                                                break;
                                                // extension
                                            case GFxHTML_ALPHA:
                                                // parse alpha: #AA
                                                if (*pattrVal == '#')
                                                {
                                                    UInt32 alpha;
                                                    if (parser.ParseHexInt(&alpha, pattrVal+1, attrValSz-1))
                                                    {
                                                        stackElem.TextFmt.SetAlpha(UInt8(alpha));    
                                                    }
                                                }
                                                break;
                                            default: break;
                                            }
                                        }
                                    }
                                }
                            }

                            break;
                        case GFxHTML_TEXTFORMAT:
                            {
                                const Char* pattrName;
                                UPInt       attrSz;
                                while(parser.GetNextAttribute(&pattrName, &attrSz))
                                {
                                    const GFxSGMLElementDesc* pattrDesc = GFxSGMLElementDesc::FindElem<Char>
                                        (pattrName, attrSz, attributesTable, sizeof(attributesTable)/sizeof(attributesTable[0]));
                                    if (pattrDesc)
                                    {
                                        const Char* pattrVal;
                                        UPInt       attrValSz;                                        
                                        if (parser.GetNextAttributeValue(&pattrVal, &attrValSz))
                                        {
                                            switch(pattrDesc->ElemId)
                                            {
                                            case GFxHTML_INDENT:
                                                {
                                                    // parse indent
                                                    int indent;
                                                    if (parser.ParseInt(&indent, pattrVal, attrValSz))
                                                    {
                                                        stackElem.ParaFmt.SetIndent(indent);
                                                    }
                                                    break;
                                                }
                                            case GFxHTML_BLOCKINDENT:
                                                {
                                                    // parse indent
                                                    int indent;
                                                    if (parser.ParseInt(&indent, pattrVal, attrValSz) && indent >= 0)
                                                    {
                                                        stackElem.ParaFmt.SetBlockIndent((UInt)indent);
                                                    }
                                                    break;
                                                }
                                            case GFxHTML_LEADING:
                                                {
                                                    // parse indent
                                                    int leading;
                                                    if (parser.ParseInt(&leading, pattrVal, attrValSz))
                                                    {
                                                        stackElem.ParaFmt.SetLeading(leading);
                                                    }
                                                    break;
                                                }
                                            case GFxHTML_LEFTMARGIN:
                                                {
                                                    // parse margin
                                                    int margin;
                                                    if (parser.ParseInt(&margin, pattrVal, attrValSz) && margin >= 0)
                                                    {
                                                        stackElem.ParaFmt.SetLeftMargin((UInt)margin);
                                                    }
                                                    break;
                                                }
                                            case GFxHTML_RIGHTMARGIN:
                                                {
                                                    // parse margin
                                                    int margin;
                                                    if (parser.ParseInt(&margin, pattrVal, attrValSz) && margin >= 0)
                                                    {
                                                        stackElem.ParaFmt.SetRightMargin((UInt)margin);
                                                    }
                                                    break;
                                                }
                                            case GFxHTML_TABSTOPS:
                                                {
                                                    // parse tabstops as [xx,xx,xx]
                                                    // calculate number of tabstops first
                                                    UPInt i = 0;
                                                    while(i < attrValSz && GFxSGMLCharIter<Char>::IsSpace(pattrVal[i]))
                                                        ++i;
                                                    if (pattrVal[i] == '[')
                                                        ++i; // skip the [
                                                    UInt n = 1;
                                                    UPInt j;
                                                    for (j = i; j < attrValSz && pattrVal[j] != ']'; ++j)
                                                    {
                                                        if (pattrVal[j] == ',')
                                                            ++n;
                                                    }
                                                    stackElem.ParaFmt.SetTabStopsNum(n);
                                                    UInt idx = 0;
                                                    for (j = i; j < attrValSz && pattrVal[j] != ']'; )
                                                    {
                                                        UInt v = 0;
                                                        for (; j < attrValSz && GFxSGMLCharIter<Char>::IsDigit(pattrVal[j]); ++j)
                                                        {
                                                            v *= 10;
                                                            v += (pattrVal[j] - '0');
                                                        }
                                                        while(j < attrValSz && GFxSGMLCharIter<Char>::IsSpace(pattrVal[j]))
                                                            ++j;
                                                        stackElem.ParaFmt.SetTabStopsElement(idx, v);
                                                        ++idx;
                                                        if (pattrVal[j] == ',')
                                                        {
                                                            ++j;
                                                            while(j < attrValSz && GFxSGMLCharIter<Char>::IsSpace(pattrVal[j]))
                                                                ++j;
                                                        }
                                                    }
                                                }
                                                break;
                                            default: break;
                                            }
                                        }
                                    }
                                }
                            }
                            break;
                        case GFxHTML_SPAN:
                            {
                                #ifndef GFC_NO_CSS_SUPPORT
                                const Char* pattrName;
                                UPInt       attrSz;
                                while(parser.GetNextAttribute(&pattrName, &attrSz))
                                {
                                    const GFxSGMLElementDesc* pattrDesc = GFxSGMLElementDesc::FindElem<Char>
                                        (pattrName, attrSz, attributesTable, sizeof(attributesTable)/sizeof(attributesTable[0]));
                                    if (pattrDesc)
                                    {
                                        const Char* pattrVal;
                                        UPInt       attrValSz;                                        
                                        if (parser.GetNextAttributeValue(&pattrVal, &attrValSz))
                                        {
                                            switch(pattrDesc->ElemId)
                                            {
                                                case GFxHTML_CLASS:
                                                    // apply CSS class
                                                    if (pstyleMgr)
                                                    {
                                                        const GFxTextStyle* pstyle =
                                                            pstyleMgr->GetStyle(GFxTextStyleKey::CSS_Class, pattrVal, attrValSz);
                                                        if (pstyle)
                                                        {
                                                            //?stackElem.ParaFmt = stackElem.ParaFmt.Merge(pstyle->ParagraphFormat);
                                                            stackElem.TextFmt = stackElem.TextFmt.Merge(pstyle->TextFormat);
                                                        }
                                                    }
                                                    break;
                                                default: break;
                                            }
                                        }
                                    }
                                }
                                #endif //GFC_NO_CSS_SUPPORT
                            }
                            break;
                        default: break;
                        }
                    }
                }
            }
            break;
        case SGMLPS_CONTENT:
            {
                const Char* content;
                UPInt  contentSize;
                if (parser.ParseContent(&content, &contentSize))
                {
                    if (contentSize > 0)
                    {
                        pcurPara = GetLastParagraph();
                        // do not add space at the beginning of paragraph, if condenseWhite
                        if (!condenseWhite || contentSize > 1 || content[0] != ' ' || pcurPara->GetLength() > 0)
                        {
                            if (elementsStack.size() > 0)
                            {
                                GFxSGMLStackElemDesc<Char>& stackElem = elementsStack[elementsStack.size()-1];
                                AppendString(content, contentSize, &stackElem.TextFmt, &stackElem.ParaFmt);
                                lastFormat = stackElem.TextFmt;
                            }
                            else
                            {
                                AppendString(content, contentSize);
                            }
                        }
                    }
                }
            }
            break;
        case SGMLPS_END_ELEMENT:
        case SGMLPS_EMPTY_ELEMENT_FINISHED:
            {
                const Char* elemName = NULL;
                UPInt  elemLen       = 0;

                UPInt startPos;
                const GFxSGMLStackElemDesc<Char>* pstackElem = NULL;
                if (type == SGMLPS_END_ELEMENT)
                {
                    if (elementsStack.size() > 0)
                        pstackElem = &elementsStack[elementsStack.size()-1];
                    parser.ParseEndElement(&elemName, &elemLen);
                }
                else
                {
                    // ignore end elements for BR & TAB
                    if (plastElemDesc)
                    {
                        if (plastElemDesc->EmptyElement)
                            break;
                    }
                    if (elementsStack.size() > 0)
                    {
                        pstackElem = &elementsStack[elementsStack.size()-1];
                        elemName = pstackElem->pElemName;
                        elemLen  = pstackElem->ElemNameSize;
                    }
                    else
                        break; // shouldn't get here at all.
                }
                if (pstackElem)
                {
                    // verify matching of end-element and start-element
                    if (parser.StrCompare(elemName, elemLen, pstackElem->pElemName, pstackElem->ElemNameSize) != 0)
                    {
                        // mismatch!
                        // just ignore such an element for now. Do we need to print
                        // kinda warning?
                        const GFxSGMLElementDesc* pelemDesc = GFxSGMLElementDesc::FindElem<Char>
                            (elemName, elemLen, elementsTable, sizeof(elementsTable)/sizeof(elementsTable[0]));
                        if (pelemDesc && p_elem_is_opened && (pelemDesc->ElemId == GFxHTML_P || pelemDesc->ElemId == GFxHTML_LI))
                        {
                            // looks like, if </p> end tag is used w/o closing all tags nested to it
                            // then it should close all opened tags till <p>
                            while(elementsStack.size() > 0)
                            {
                                const GFxSGMLStackElemDesc<Char>& stackElem = elementsStack[elementsStack.size()-1];
                                if (stackElem.pElemDesc && pelemDesc->ElemId == stackElem.pElemDesc->ElemId)
                                    break;
                                elementsStack.pop_back();
                            }
                            if (elementsStack.size() > 0)
                                pstackElem = &elementsStack[elementsStack.size()-1];
                            else
                                break;
                        }
                        else
                            break;
                    }
                    bool isDisplaySet = pstackElem->ParaFmt.IsDisplaySet();
                    GFxTextParagraphFormat::DisplayType display = pstackElem->ParaFmt.GetDisplay();
                    plastElemDesc = pstackElem->pElemDesc;
                    startPos = pstackElem->StartPos;
                    elementsStack.pop_back();

                    if (plastElemDesc)
                    {
                        switch(plastElemDesc->ElemId)
                        {
                        case GFxHTML_P:
                            --p_elem_is_opened;
                            // Empty <P/> element doesn't produce a new paragraph in Flash
                            if (type == SGMLPS_EMPTY_ELEMENT_FINISHED)
                                break;
                        case GFxHTML_LI:
                            if (multiline)
                                AppendString(NewLineStr(), 1);
                            break;
                        default: break;
                        }
                    }
                    else
                    {
                        // arbitrary tags handling, if necessary
                        if (pstyleMgr)
                        {
                            // if CSS is used, check if we have block or none display
                            if (isDisplaySet)
                            {
                                switch(display)
                                {
                                case GFxTextParagraphFormat::Display_Block:
                                    // create a new paragraph
                                    if (multiline)
                                        AppendString(NewLineStr(), 1);
                                    break;
                                case GFxTextParagraphFormat::Display_None:
                                    {
                                    // create a new paragraph and remove the invisible text
                                    UPInt curPos = GetLength();
                                    if (multiline)
                                    {
                                        AppendString(NewLineStr(), 1);
                                    }
                                    Remove(startPos, curPos - startPos);
                                    break;
                                    }
                                default:;
                                }
                            }

                        }
                    }
                }
                else
                {
                    // wrong end elem, ignore...
                    break;
                }
            }
            break;
        default: break;
        }
    }
    if (elementsStack.size() > 0)
    {
        GFxSGMLStackElemDesc<Char>& stackElem = elementsStack[elementsStack.size()-1];
        SetDefaultTextFormat(stackElem.TextFmt);
        SetDefaultParagraphFormat(stackElem.ParaFmt);
    }
    else
    {
        SetDefaultTextFormat(defaultTextFmt);
        SetDefaultParagraphFormat(defaultParaFmt);
    }
    EnsureTermNull();

    // Flash treats empty paragraphs at the end of the specific way. For example, if 
    // HTML code is as follows: 
    // "<p align="left"><font face="Arial" size="20" color="#ffffff" letterSpacing="5" kerning="1">Test 12345</font></p><p align="left"></p>"
    // then the last paragraph should be empty w/o any format data (<p align="left"></p>). Though,
    // Flash propagates the last known text format on the empty paragraphs. The code below does
    // this in GFx.
    GFxTextParagraph* ppara = GetLastParagraph();
    if (ppara && ppara->GetLength() == 0)
    {
        ParagraphsIterator paraIter = Paragraphs.Last();
        while(!paraIter.IsFinished())
        {
            GFxTextParagraph* pcurPara = paraIter->GetPtr();
            if (pcurPara->GetLength() <= 1)
            {
                pcurPara->SetTextFormat(GetAllocator(), lastFormat, 0, GFC_MAX_UPINT);
            }
            else
                break;
            --paraIter;
        }
    }

    CheckIntegrity();
    return true;
}


bool GFxStyledText::ParseHtml(const char* phtml, UPInt  htmlSize, HTMLImageTagInfoArray* pimgInfoArr, 
               bool multiline, bool condenseWhite, const GFxTextStyleManager* pstyleMgr)
{
    return ParseHtmlImpl(phtml, htmlSize, pimgInfoArr, multiline, condenseWhite, pstyleMgr);
}

bool GFxStyledText::ParseHtml(const wchar_t* phtml, UPInt  htmlSize, HTMLImageTagInfoArray* pimgInfoArr, 
               bool multiline, bool condenseWhite, const GFxTextStyleManager* pstyleMgr)
{
    return ParseHtmlImpl(phtml, htmlSize, pimgInfoArr, multiline, condenseWhite, pstyleMgr);
}
