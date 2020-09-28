/**********************************************************************

Filename    :   GFxStyledText.h
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

#ifndef INC_TEXT_GFXSTYLEDTEXT_H
#define INC_TEXT_GFXSTYLEDTEXT_H

#include "Text/GFxTextCore.h"
#include "Text/GFxSGMLParser.h"

#ifndef GFC_NO_CSS_SUPPORT
#include "Text/GFxStyleSheet.h"
#else
class GFxTextStyleManager;
#endif

#define GFX_NBSP_CHAR_CODE          160

// Represents a paragraph of text. Paragraph - is the text line terminating by new line symbol or '\0'
class GFxTextParagraph : public GNewOverrideBase
{
    friend class GFxStyledText;
    friend class GFxTextCompositionString;

    typedef GRangeDataArray<GPtr<GFxTextFormat> >   TextFormatArrayType;
    typedef GRangeData<GPtr<GFxTextFormat> >        TextFormatRunType;

    class TextBuffer
    {
        wchar_t* pText;
        UPInt    Size;
        UPInt    Allocated;

        TextBuffer(const TextBuffer& ) { GASSERT(0); }
    public:
        TextBuffer(): pText(0), Size(0), Allocated(0) {}
        TextBuffer(const TextBuffer& o, GFxTextAllocator* pallocator);
        /*GFxWideStringBuffer(GFxTextAllocator* pallocator):pAllocator(pallocator), pText(0), Size(0), Allocated(0)
        {
        GASSERT(pAllocator);
        }*/
        ~TextBuffer();

        wchar_t* ToWStr() const
        {
            return pText;
        }
        UPInt GetSize() const { return Size; }
        UPInt GetLength() const;
        const wchar_t* GetCharPtrAt(UPInt pos) const;

        void SetString(GFxTextAllocator* pallocator, const char* putf8Str, UPInt utf8length = GFC_MAX_UPINT);
        void SetString(GFxTextAllocator* pallocator, const wchar_t* pstr, UPInt length = GFC_MAX_UPINT);
        wchar_t* CreatePosition(GFxTextAllocator* pallocator, UPInt pos, UPInt length);
        void Remove(UPInt pos, UPInt length);
        void Clear() { Size = 0; } // doesn't free mem!
        void Free(GFxTextAllocator* pallocator);

        SPInt StrChr(wchar_t c) { return StrChr(pText, GetLength(), c); }
        void StripTrailingNewLines();
        void StripTrailingNull();
        void AppendTrailingNull(GFxTextAllocator* pallocator);

        static SPInt StrChr(const wchar_t* ptext, UPInt length, wchar_t c);
        static UPInt StrLen(const wchar_t* ptext);

        //GFxTextAllocator* GetAllocator() const { return pAllocator; }
    };

    //GFxTextAllocator*   GetAllocator() const  { return Text.GetAllocator(); }

    void SetStartIndex(UPInt i) { StartIndex = i; }

    // returns the actual pointer on text format at the specified position.
    // Will return NULL, if there is no text format at the "pos"
    GFxTextFormat* GetTextFormatPtr(UPInt pos) const;
    void SetFormat(const GFxTextParagraphFormat* pfmt);

    void AppendTermNull(GFxTextAllocator* pallocator, const GFxTextFormat* pdefTextFmt);
    void RemoveTermNull();
    bool HasTermNull() const;
    bool HasNewLine() const;
    void SetTermNullFormat();

    // returns true if paragraph is empty (no chars or only termination null)
    bool IsEmpty() const { return GetLength() == 0; }

    void MarkToReformat() { ++ModCounter; }
public:
    // this struct is returned by FormatRunIterator::operator*
    struct StyledTextRun
    {
        const wchar_t*  pText;
        SPInt           Index;
        UPInt           Length;
        GPtr<GFxTextFormat>  pFormat;

        StyledTextRun():pText(0), Index(0), Length(0) {}
        StyledTextRun(const wchar_t* ptext, SPInt index, UPInt len, GFxTextFormat* pfmt)
        { Set(ptext, index, len, pfmt); }

        StyledTextRun& Set(const wchar_t* ptext, SPInt index, UPInt len, GFxTextFormat* pfmt)
        {
            pText   = ptext;
            Index   = index;
            Length  = len;
            pFormat = pfmt;
            return *this;
        }
    };

    // Iterates through all format ranges in the paragraph, returning both
    // format structure and text chunks as StyledTextRun
    class FormatRunIterator
    {
        StyledTextRun                       PlaceHolder;
        const TextFormatArrayType*          pFormatInfo;
        TextFormatArrayType::ConstIterator  FormatIterator;
        const TextBuffer*                   pText;

        UPInt                               CurTextIndex;
    public:
        FormatRunIterator(const TextFormatArrayType& fmts, const TextBuffer& textHandle);
        FormatRunIterator(const TextFormatArrayType& fmts, const TextBuffer& textHandle, UPInt index);
        FormatRunIterator(const FormatRunIterator& orig);

        FormatRunIterator& operator=(const FormatRunIterator& orig);

        inline bool IsFinished() const  { return CurTextIndex >= pText->GetSize(); }

        const StyledTextRun& operator* ();
        const StyledTextRun* operator->() { return &operator*(); }
        void operator++();
        inline void operator++(int) { operator++(); }

        void SetTextPos(UPInt newTextPos);
    };

    struct CharacterInfo
    {
        GPtr<GFxTextFormat> pFormat;
        UPInt               Index;
        wchar_t             Character;

        CharacterInfo() : pFormat(NULL), Index(0), Character(0) {}
        CharacterInfo(wchar_t c, UPInt index, GFxTextFormat* pf) : pFormat(pf), Index(index), Character(c) {}
    };
    class CharactersIterator
    {
        CharacterInfo                       PlaceHolder;
        const TextFormatArrayType*          pFormatInfo;
        TextFormatArrayType::ConstIterator  FormatIterator;
        const TextBuffer*                   pText;

        UPInt                               CurTextIndex;
    public:
        CharactersIterator() : pFormatInfo(NULL), pText(NULL), CurTextIndex(0) {}
        CharactersIterator(const TextBuffer& buf) : pFormatInfo(NULL), pText(&buf), CurTextIndex(0) {}
        CharactersIterator(const GFxTextParagraph* pparagraph);
        CharactersIterator(const GFxTextParagraph* pparagraph, UPInt index);

        inline bool IsFinished() const { return pText == NULL || CurTextIndex >= pText->GetSize(); }

        CharacterInfo&              operator*();
        const CharacterInfo&        operator*() const
        {
            return const_cast<CharactersIterator*>(this)->operator*();
        }
        inline CharacterInfo*       operator->() { return &operator*(); }
        inline const CharacterInfo* operator->() const { return &operator*(); }

        void operator++();
        void operator++(int) { operator++(); }
        void operator+=(UPInt n);

        const wchar_t* GetRemainingTextPtr(UPInt * plen) const;
        UPInt GetCurTextIndex() const { return CurTextIndex; }
    };
    CharactersIterator GetCharactersIterator() const            { return CharactersIterator(this); }
    CharactersIterator GetCharactersIterator(UPInt index) const { return CharactersIterator(this, index); }

private:
    GFxTextParagraph(const GFxTextParagraph& o);
public:
    GFxTextParagraph(GFxTextAllocator* ptextAllocator);
    GFxTextParagraph(const GFxTextParagraph& o, GFxTextAllocator* ptextAllocator);
    GFxTextParagraph() {  } // shouldn't be used! 
    ~GFxTextParagraph() {}

    FormatRunIterator GetIterator() const;
    FormatRunIterator GetIteratorAt(UPInt index) const;

    UPInt GetStartIndex() const { return StartIndex; }
    // returns length, not including terminal null (if exists)
    UPInt GetLength() const;
    // returns length, including terminal null (if exists)
    UPInt GetSize() const       { return Text.GetSize(); }
    UPInt GetNextIndex() const  { return StartIndex + GetLength(); }

    const wchar_t* GetText() const  { return Text.ToWStr(); }
    wchar_t*       GetText()        { return Text.ToWStr(); }

    wchar_t* CreatePosition(GFxTextAllocator* pallocator, UPInt pos, UPInt length = 1);
    void InsertString(GFxTextAllocator* pallocator, const wchar_t* pstr, UPInt pos, UPInt length, const GFxTextFormat* pnewFmt);
    void InsertString(GFxTextAllocator* pallocator, const wchar_t* pstr, UPInt pos, UPInt length = GFC_MAX_UPINT)
    {
        InsertString(pallocator, pstr, pos, length, NULL);
    }
    // AppendPlainText methods doesn't expand format ranges
    void AppendPlainText(GFxTextAllocator* pallocator, const wchar_t* pstr, UPInt length = GFC_MAX_UPINT);
    void AppendPlainText(GFxTextAllocator* pallocator, const char* putf8str, UPInt utf8StrSize = GFC_MAX_UPINT);

    void Remove(UPInt startPos, UPInt endPos = GFC_MAX_UPINT);
    void Clear();
    void FreeText(GFxTextAllocator* pallocator) { Text.Free(pallocator); }

    void SetText(GFxTextAllocator* pallocator, const wchar_t* pstring, UPInt length = GFC_MAX_UPINT);
    void SetTextFormat(GFxTextAllocator* pallocator, const GFxTextFormat& fmt, UPInt startPos = 0, UPInt endPos = GFC_MAX_UPINT);
    GFxTextFormat GetTextFormat(UPInt startPos, UPInt endPos = GFC_MAX_UPINT) const;

    void ClearTextFormat(UPInt startPos = 0, UPInt endPos = GFC_MAX_UPINT);

    void SetFormat(GFxTextAllocator* pallocator, const GFxTextParagraphFormat& fmt);
    const GFxTextParagraphFormat* GetFormat() const { return pFormat.GetPtr(); }

    UInt32 GetId() const        { return UniqueId; }
    UInt16 GetModCounter() const{ return ModCounter; }

    // copies text and formatting from psrcPara paragraph to this one, starting from startSrcIndex in
    // source paragraph and startDestIndex in destination one. The "length" specifies the number
    // of positions to copy.
    void Copy(GFxTextAllocator* pallocator, const GFxTextParagraph& psrcPara, UPInt startSrcIndex, UPInt startDestIndex, UPInt length);

    // Shrinks the paragraph's length by the "delta" value.
    void Shrink(UPInt delta);

#ifdef GFC_BUILD_DEBUG
    void CheckIntegrity() const;
#else
    void CheckIntegrity() const {}
#endif

private: // data
    TextBuffer                      Text;
    GPtr<GFxTextParagraphFormat>    pFormat;
    TextFormatArrayType             FormatInfo;
    UPInt                           StartIndex;
    UInt32                          UniqueId;
    UInt16                          ModCounter;
};

class GFxStyledText : public GRefCountBase<GFxStyledText>
{
    friend class GFxTextDocView;
    friend class GFxTextCompositionString;
public:
    class ParagraphPtrWrapper
    {
        mutable GFxTextParagraph* pPara;
    public:

        ParagraphPtrWrapper():pPara(NULL) {}
        ParagraphPtrWrapper(GFxTextParagraph* ptr) : pPara(ptr) {}
        ParagraphPtrWrapper(const GFxTextParagraph* ptr) : pPara(const_cast<GFxTextParagraph*>(ptr)) {}

        ParagraphPtrWrapper(const ParagraphPtrWrapper& ptr) : pPara(ptr.pPara) { ptr.pPara = NULL; }
        ~ParagraphPtrWrapper() { delete pPara; }

        GFxTextParagraph& operator*() { return *pPara; }
        const GFxTextParagraph& operator*() const { return *pPara; }

        GFxTextParagraph* operator-> () { return pPara; }
        const GFxTextParagraph* operator-> () const { return pPara; }

        GFxTextParagraph* GetPtr() { return pPara; }
        const GFxTextParagraph* GetPtr() const { return pPara; }

        operator GFxTextParagraph*()                { return pPara; }
        operator const GFxTextParagraph*() const    { return pPara; }

        ParagraphPtrWrapper& operator=(const GFxTextParagraph* p)
        {
            if (pPara != const_cast<GFxTextParagraph*>(p))
                delete pPara;
            pPara = const_cast<GFxTextParagraph*>(p);
            return *this;
        }
        ParagraphPtrWrapper& operator=(GFxTextParagraph* p)
        {
            if (pPara != p)
                delete pPara;
            pPara = p;
            return *this;
        }
        ParagraphPtrWrapper& operator=(const ParagraphPtrWrapper& p)
        {
            if (pPara != p.pPara)
                delete pPara;
            pPara = p.pPara;
            p.pPara = NULL;
            return *this;
        }
        bool operator==(const ParagraphPtrWrapper& p) const
        {
            return pPara == p.pPara;
        }
        bool operator==(const GFxTextParagraph* p) const
        {
            return pPara == p;
        }
    };
    struct HTMLImageTagInfo
    {
        GPtr<GFxTextImageDesc> pTextImageDesc;
        GFxString Url;
        GFxString Id;
        UInt      Width, Height; // in twips
        SInt      VSpace, HSpace; // in twips
        UInt      ParaId;
        enum
        {
            Align_BaseLine,
            Align_Right,
            Align_Left
        };
        UByte     Alignment;

        HTMLImageTagInfo() : Width(0), Height(0), VSpace(0), HSpace(0), 
            ParaId(~0u), Alignment(Align_BaseLine) {}
    };

    typedef GTL::garray<HTMLImageTagInfo>       HTMLImageTagInfoArray;
    typedef GTL::garray<ParagraphPtrWrapper>    ParagraphArrayType;
    typedef ParagraphArrayType::Iterator        ParagraphsIterator;
    typedef ParagraphArrayType::ConstIterator   ParagraphsConstIterator;

    struct CharacterInfo
    {
        GPtr<GFxTextFormat> pOriginalFormat;
        GFxTextParagraph*   pParagraph;
        UPInt               Index;
        wchar_t             Character;

        CharacterInfo() : Index(0), Character(0) {}
    };

    class CharactersIterator
    {
        ParagraphsIterator                      Paragraphs;
        GFxTextParagraph::CharactersIterator    Characters;
        GPtr<GFxStyledText>                     pText;
        UPInt                                   FirstCharInParagraphIndex;
        CharacterInfo                           CharInfoPlaceHolder;
    public:
        CharactersIterator(GFxStyledText* ptext);
        CharactersIterator(GFxStyledText* ptext, SInt index);
        inline bool IsFinished() const { return Characters.IsFinished() && Paragraphs.IsFinished(); }

        CharacterInfo& operator*();
        void operator++();
        inline void operator++(int) { operator++(); }
    };
private:
    struct ParagraphComparator
    {
        int Compare(const GFxTextParagraph* pp1, UPInt index) const
        {
            UPInt si1 = pp1->GetStartIndex();
            if (index >= si1 && index < (si1 + pp1->GetSize()))
                return 0; 
            return (int)(si1 - index);
        }
    };
    void EnsureTermNull();
public:
    GFxStyledText();
    GFxStyledText(GFxTextAllocator* pallocator);
    ~GFxStyledText() { Clear(); }

    void Clear();

    UPInt GetLength() const;

    void SetText(const char* putf8String, UPInt stringSize = GFC_MAX_UPINT);
    void SetText(const wchar_t* pstring, UPInt length = GFC_MAX_UPINT);

    GFxString GetText() const;
    void GetText(GFxWStringBuffer* pBuffer) const;
    void GetText(GFxWStringBuffer* pBuffer,UPInt startPos, UPInt endPos) const;
    void CopyStyledText(GFxStyledText* pdest, UPInt startPos = 0, UPInt endPos = GFC_MAX_UPINT) const;
    GFxStyledText* CopyStyledText(UPInt startPos = 0, UPInt endPos = GFC_MAX_UPINT) const;

    void SetHtml(const GFxString&);
    void SetHtml(const wchar_t* pstring, UPInt length = GFC_MAX_UPINT);
    GFxString GetHtml() const;

    void GetTextAndParagraphFormat(GFxTextFormat* pdestTextFmt, GFxTextParagraphFormat* pdestParaFmt, UPInt startPos, UPInt endPos = GFC_MAX_UPINT);
    bool GetTextAndParagraphFormat(const GFxTextFormat** ppdestTextFmt, const GFxTextParagraphFormat** pdestParaFmt, UPInt pos);
    void SetTextFormat(const GFxTextFormat& fmt, UPInt startPos = 0, UPInt endPos = GFC_MAX_UPINT);
    void SetParagraphFormat(const GFxTextParagraphFormat& fmt, UPInt startPos = 0, UPInt endPos = GFC_MAX_UPINT);

    // wipe out text format for the specified range. Might be useful, if need to set
    // format without merging with existing one.
    void ClearTextFormat(UPInt startPos = 0, UPInt endPos = GFC_MAX_UPINT);

    // assign pdefaultTextFmt as a default text format
    void SetDefaultTextFormat(const GFxTextFormat* pdefaultTextFmt);
    // makes a copy of text format and assign
    void SetDefaultTextFormat(const GFxTextFormat& defaultTextFmt);
    // assign pdefaultTextFmt as a default text format
    void SetDefaultParagraphFormat(const GFxTextParagraphFormat* pdefaultParagraphFmt);
    // makes a copy of text format and assign
    void SetDefaultParagraphFormat(const GFxTextParagraphFormat& defaultParagraphFmt);

    const GFxTextFormat*          GetDefaultTextFormat() const 
    { GASSERT(pDefaultTextFormat); return pDefaultTextFormat; }
    const GFxTextParagraphFormat* GetDefaultParagraphFormat() const 
    { GASSERT(pDefaultParagraphFormat); return pDefaultParagraphFormat; }

    const GFxTextParagraph* GetParagraph(UInt paragraphIndex) const
    {
        return (paragraphIndex < GetParagraphsCount()) ? Paragraphs[paragraphIndex].GetPtr() : NULL;
    }
    UInt GetParagraphsCount() const { return (UInt)Paragraphs.size(); }

    wchar_t* CreatePosition(UPInt pos, UPInt length = 1);

    // Insert/append funcs
    enum NewLinePolicy
    {
        NLP_CompressCRLF, // CR LF will be compressed into one EOL
        NLP_ReplaceCRLF   // each CR and/or LF will be replaced by EOL
    };
    UPInt AppendString(const char* putf8String, UPInt stringSize = GFC_MAX_UPINT, 
        NewLinePolicy newLinePolicy = NLP_ReplaceCRLF);
    UPInt AppendString(const char* putf8String, UPInt stringSize, 
        NewLinePolicy newLinePolicy,  
        const GFxTextFormat* pdefTextFmt, const GFxTextParagraphFormat* pdefParaFmt);
    UPInt AppendString(const char* putf8String, UPInt stringSize, 
        const GFxTextFormat* pdefTextFmt, const GFxTextParagraphFormat* pdefParaFmt)
    {
        return AppendString(putf8String, stringSize, NLP_ReplaceCRLF, pdefTextFmt, pdefParaFmt);
    }
    UPInt AppendString(const wchar_t* pstr, UPInt length = ~0u,
        NewLinePolicy newLinePolicy = NLP_ReplaceCRLF);
    UPInt AppendString(const wchar_t* pstr, UPInt length,
        NewLinePolicy newLinePolicy,
        const GFxTextFormat* pdefTextFmt, const GFxTextParagraphFormat* pdefParaFmt);
    UPInt AppendString(const wchar_t* pstr, UPInt length,
        const GFxTextFormat* pdefTextFmt, const GFxTextParagraphFormat* pdefParaFmt)
    {
        return AppendString(pstr, length, NLP_ReplaceCRLF, pdefTextFmt, pdefParaFmt);
    }

    UPInt InsertString(const wchar_t* pstr, UPInt pos, UPInt length = GFC_MAX_UPINT,
        NewLinePolicy newLinePolicy = NLP_ReplaceCRLF);
    UPInt InsertString(const wchar_t* pstr, UPInt pos, UPInt length, 
        NewLinePolicy newLinePolicy,
        const GFxTextFormat* pdefTextFmt, const GFxTextParagraphFormat* pdefParaFmt);
    UPInt InsertString(const wchar_t* pstr, UPInt pos, UPInt length, 
        const GFxTextFormat* pdefTextFmt, const GFxTextParagraphFormat* pdefParaFmt)
    {
        return InsertString(pstr, pos, length, NLP_ReplaceCRLF, pdefTextFmt, pdefParaFmt);
    }
    // insert styled text
    UPInt InsertStyledText(const GFxStyledText& text, UPInt pos, UPInt length = GFC_MAX_UPINT);

    void Remove(UPInt startPos, UPInt length);

    CharactersIterator GetCharactersIterator() { return CharactersIterator(this); }

    void SetNewLine0D()        { RTFlags |= RTFlags_NewLine0D; }
    void ClearNewLine0D()      { RTFlags &= (~(RTFlags_NewLine0D)); }
    bool IsNewLine0D() const   { return (RTFlags & (RTFlags_NewLine0D)) != 0; }
    unsigned char NewLineChar() const { return IsNewLine0D() ? '\r' : '\n'; }
    const char*   NewLineStr()  const { return IsNewLine0D() ? "\r" : "\n"; }

#ifdef GFC_BUILD_DEBUG
    void CheckIntegrity() const;
#else
    void CheckIntegrity() const {}
#endif

private:
    template <class Char>
    bool ParseHtmlImpl(const Char* phtml, UPInt  htmlSize, HTMLImageTagInfoArray* pimgInfoArr = NULL, 
        bool multiline = true, bool condenseWhite = false, const GFxTextStyleManager* pstyleMgr = NULL);

protected:
    bool ParseHtml(const char* phtml, UPInt  htmlSize, HTMLImageTagInfoArray* pimgInfoArr = NULL, 
        bool multiline = true, bool condenseWhite = false, const GFxTextStyleManager* pstyleMgr = NULL);
    bool ParseHtml(const wchar_t* phtml, UPInt  htmlSize, HTMLImageTagInfoArray* pimgInfoArr = NULL, 
        bool multiline = true, bool condenseWhite = false, const GFxTextStyleManager* pstyleMgr = NULL);

    void RemoveParagraph(ParagraphsIterator& paraIter, GFxTextParagraph* ppara);
    GFxTextAllocator* GetAllocator() const
    {
        if (!pTextAllocator)
            pTextAllocator = *new GFxTextAllocator();
        return pTextAllocator;
    }

    void SetMayHaveUrl()        { RTFlags |= RTFlags_MayHaveUrl; }
    void ClearMayHaveUrl()      { RTFlags &= (~(RTFlags_MayHaveUrl)); }
    bool MayHaveUrl() const     { return (RTFlags & (RTFlags_MayHaveUrl)) != 0; }

    ParagraphsIterator GetParagraphByIndex(UPInt index, UPInt* pindexInParagraph = NULL);
    ParagraphsIterator GetParagraphByIndex(UPInt index, UPInt* pindexInParagraph = NULL) const
    {
        return const_cast<GFxStyledText*>(this)->GetParagraphByIndex(index, pindexInParagraph);
    }
    ParagraphsIterator GetNearestParagraphByIndex(UPInt index, UPInt* pindexInParagraph = NULL);
    ParagraphsIterator GetNearestParagraphByIndex(UPInt index, UPInt* pindexInParagraph = NULL) const
    {
        return const_cast<GFxStyledText*>(this)->GetNearestParagraphByIndex(index, pindexInParagraph);
    }
    ParagraphsIterator GetParagraphIterator() { return Paragraphs.Begin(); }
    ParagraphsIterator GetParagraphIterator() const { return const_cast<GFxStyledText*>(this)->Paragraphs.Begin(); }

    GFxTextParagraph* AppendNewParagraph(const GFxTextParagraphFormat* pdefParaFmt = NULL);
    GFxTextParagraph* InsertNewParagraph(GFxStyledText::ParagraphsIterator& iter, 
        const GFxTextParagraphFormat* pdefParaFmt = NULL);
    GFxTextParagraph* AppendCopyOfParagraph(const GFxTextParagraph& srcPara);
    GFxTextParagraph* InsertCopyOfParagraph(GFxStyledText::ParagraphsIterator& iter, 
        const GFxTextParagraph& srcPara);

    GFxTextParagraph*       GetLastParagraph();
    const GFxTextParagraph* GetLastParagraph() const;

    // atomic callbacks. These methods might be invoked multiple times 
    // from inside of the InsertString or Remove methods. Client SHOULD NOT do
    // anything "heavy" in these handlers, like re-formatting (though, the flag 
    // "re-formatting is neeeded" might be set).
    //virtual void OnParagraphTextFormatChanging(const GFxTextParagraph& para, UPInt startPos, UPInt endPos, const GFxTextFormat& formatToBeSet);
    //virtual void OnParagraphTextFormatChanged(const GFxTextParagraph& para, UPInt startPos, UPInt endPos);
    //virtual void OnParagraphFormatChanging(const GFxTextParagraph& para, const GFxTextParagraphFormat& formatToBeSet);
    //virtual void OnParagraphFormatChanged(const GFxTextParagraph& para);
    //virtual void OnParagraphTextInserting(const GFxTextParagraph& para, UPInt insertionPos, UPInt insertingLen);
    //virtual void OnParagraphTextInserted(const GFxTextParagraph& para, UPInt startPos, UPInt endPos, const wchar_t* ptextInserted);
    //virtual void OnParagraphTextRemoving(const GFxTextParagraph& para, UPInt removingPos, UPInt removingLen);
    //virtual void OnParagraphTextRemoved(const GFxTextParagraph& para, UPInt removedPos, UPInt removedLen);
    virtual void OnTextInserting(UPInt startPos, UPInt length, const wchar_t* ptxt);
    virtual void OnTextInserting(UPInt startPos, UPInt length, const char* ptxt)
    { GUNUSED3(startPos, length, ptxt); };
    virtual void OnTextRemoving(UPInt startPos, UPInt length);
    virtual void OnTextInserted(UPInt startPos, UPInt length, const wchar_t* ptxt) 
    { GUNUSED3(startPos, length, ptxt); };
    virtual void OnParagraphRemoving(const GFxTextParagraph& para);

    // This callback will be fired at the end of big change, like InsertString, Remove, etc
    virtual void OnDocumentChanged() {}
protected: //data
    mutable GPtr<GFxTextAllocator>  pTextAllocator;
    ParagraphArrayType              Paragraphs;
    GPtr<GFxTextParagraphFormat>    pDefaultParagraphFormat;
    GPtr<GFxTextFormat>             pDefaultTextFormat;

    enum 
    {
        RTFlags_MayHaveUrl = 0x1,
        RTFlags_NewLine0D  = 0x2 // use '\r' as internal new-line char
    };
    UByte                           RTFlags;
};

#endif // INC_TEXT_GFXSTYLEDTEXT_H
