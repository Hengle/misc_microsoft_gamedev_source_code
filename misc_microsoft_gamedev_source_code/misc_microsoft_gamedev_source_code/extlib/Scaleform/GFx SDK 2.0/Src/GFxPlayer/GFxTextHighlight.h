/**********************************************************************

Filename    :   GFxTextHighlight.h
Content     :   
Created     :   August 6, 2007
Authors     :   Artyom Bolgar

Notes       :   
History     :   

Copyright   :   (c) 1998-2007 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFXTEXTHIGHLIGHT_H
#define INC_GFXTEXTHIGHLIGHT_H

#include "GTLTypes.h"
#include "GRange.h"
#include "GColor.h"

struct GFxTextHighlightDesc
{
    int         StartPos;
    UInt        Length;
    enum
    {
        Flag_SingleUnderline    = 0x1,
        Flag_DoubleUnderline    = 0x2,
        Flag_DottedUnderline    = 0x4,
        Flag_Background         = 0x8,
        Flag_TextColor          = 0x10,
        Flag_UnderlineColor     = 0x20
    };
    GColor      BackgroundColor;
    GColor      TextColor;
    GColor      UnderlineColor;
    UByte       Flags;

    GFxTextHighlightDesc() { Reset(); }
    GFxTextHighlightDesc(int pos, UInt len):StartPos(pos),Length(len),Flags(0) {}

    void Reset() { StartPos = -1; Length = 0; Flags = 0; }
    void Merge(const GFxTextHighlightDesc& mergee);

    void SetSingleUnderline()     { Flags |= Flag_SingleUnderline; }
    void ClearSingleUnderline()   { Flags &= (~Flag_SingleUnderline); }
    bool IsSingleUnderline() const{ return (Flags & Flag_SingleUnderline) != 0; }

    void SetDoubleUnderline()     { Flags |= Flag_DoubleUnderline; }
    void ClearDoubleUnderline()   { Flags &= (~Flag_DoubleUnderline); }
    bool IsDoubleUnderline() const{ return (Flags & Flag_DoubleUnderline) != 0; }

    void SetDottedUnderline()     { Flags |= Flag_DottedUnderline; }
    void ClearDottedUnderline()   { Flags &= (~Flag_DottedUnderline); }
    bool IsDottedUnderline() const{ return (Flags & Flag_DottedUnderline) != 0; }

    void SetBackgroundColor(const GColor& backgr) { Flags |= Flag_Background; BackgroundColor = backgr; }
    GColor GetBackgroundColor() const             { return (HasBackgroundColor()) ? BackgroundColor : GColor(0, 0, 0, 0); }
    void ClearBackgroundColor()                   { Flags &= (~Flag_Background); }
    bool HasBackgroundColor() const               { return (Flags & Flag_Background) != 0; }

    void SetTextColor(const GColor& color) { Flags |= Flag_TextColor; TextColor = color; }
    GColor GetTextColor() const             { return (HasTextColor()) ? TextColor : GColor(0, 0, 0, 0); }
    void ClearTextColor()                   { Flags &= (~Flag_TextColor); }
    bool HasTextColor() const               { return (Flags & Flag_TextColor) != 0; }

    void SetUnderlineColor(const GColor& color) { Flags |= Flag_UnderlineColor; UnderlineColor = color; }
    GColor GetUnderlineColor() const            { return (HasUnderlineColor()) ? UnderlineColor : GColor(0, 0, 0, 0); }
    void ClearUnderlineColor()                  { Flags &= (~Flag_UnderlineColor); }
    bool HasUnderlineColor() const              { return (Flags & Flag_UnderlineColor) != 0; }

};

class GFxTextHighlighter
{
    friend class GFxTextHighlighterRangeIterator;
    friend class GFxTextHighlighterPosIterator;
public:
    struct HighlighterAssoc
    {
        int     Id;
        UInt    Index;

        HighlighterAssoc():Id(0), Index(0) {}
        HighlighterAssoc(int id, UInt index):Id(id), Index(index) {}
    };
protected:
    GTL::gindexed_array<GFxTextHighlightDesc> Highlighters;
    GTL::garray<HighlighterAssoc>         Id2IndexAssoc;
    int LastId;
public:
    GFxTextHighlighter():LastId(0) {}

    bool CreateHighlighter(const GFxTextHighlightDesc&, int *pid);
    bool CreateHighlighter(int id, const GFxTextHighlightDesc&);

    const GFxTextHighlightDesc* GetHighlighter(int id) const;
    GFxTextHighlightDesc* GetHighlighter(int id);
    void FreeHighlighter(int id);


};

class GFxTextHighlighterPosIterator
{

};

class GFxTextHighlighterRangeIterator
{
    GFxTextHighlighter*  pManager;
    GFxTextHighlightDesc        CurDesc;
    UInt                    CurStartingPos;

    void InitCurDesc(UInt startingPos);
public:
    GFxTextHighlighterRangeIterator(GFxTextHighlighter* pmanager);

    GFxTextHighlightDesc operator*();
    void operator++(int);

    bool IsFinished() const;
};

#endif // INC_GFXTEXTHIGHLIGHT_H
