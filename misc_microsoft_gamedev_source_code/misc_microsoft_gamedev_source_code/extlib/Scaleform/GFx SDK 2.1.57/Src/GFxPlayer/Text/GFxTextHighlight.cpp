/**********************************************************************

Filename    :   GFxTextHighlight.cpp
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

#include "Text/GFxTextHighlight.h"
#include "Text/GFxTextEditorKit.h"

namespace
{
    struct IdComparator
    {
        int Compare(const GFxTextHighlightDesc& p1, UInt id) const
        {
            return (int)p1.Id - (int)id;
        }
    };
}

//////////////////////////////////////////////////////////////////////////
// GFxTextHighlightDesc
// 
void GFxTextHighlightInfo::Append(const GFxTextHighlightInfo& mergee)
{
    if (!HasUnderlineStyle() && mergee.HasUnderlineStyle())
    {
        SetUnderlineStyle(mergee.GetUnderlineStyle());
    }
    if (!HasBackgroundColor() && mergee.HasBackgroundColor())
        SetBackgroundColor(mergee.GetBackgroundColor());
    if (!HasTextColor() && mergee.HasTextColor())
        SetTextColor(mergee.GetTextColor());
    if (!HasUnderlineColor() && mergee.HasUnderlineColor())
        SetUnderlineColor(mergee.GetUnderlineColor());
}

void GFxTextHighlightInfo::Prepend(const GFxTextHighlightInfo& mergee)
{
    if (mergee.HasUnderlineStyle())
    {
        SetUnderlineStyle(mergee.GetUnderlineStyle());
    }
    if (mergee.HasBackgroundColor())
        SetBackgroundColor(mergee.GetBackgroundColor());
    if (mergee.HasTextColor())
        SetTextColor(mergee.GetTextColor());
    if (mergee.HasUnderlineColor())
        SetUnderlineColor(mergee.GetUnderlineColor());
}

bool GFxTextHighlightInfo::IsEqualWithFlags(const GFxTextHighlightInfo& right, UInt flags)
{
    if (flags & Flag_UnderlineStyle)
    {
        if (GetUnderlineStyle() != right.GetUnderlineStyle())
            return false;
    }
    if (flags & Flag_Background)
    {
        if (HasBackgroundColor() != right.HasBackgroundColor() || 
            GetBackgroundColor() != right.GetBackgroundColor())
            return false;
    }
    if (flags & Flag_TextColor)
    {
        if (HasTextColor() != right.HasTextColor() || 
            GetTextColor() != right.GetTextColor())
            return false;
    }
    if (flags & Flag_UnderlineColor)
    {
        if (HasUnderlineColor() != right.HasUnderlineColor() || 
            GetUnderlineColor() != right.GetUnderlineColor())
            return false;
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////
// GFxTextHighlighter
// 
GFxTextHighlighter::GFxTextHighlighter() :
    LastId(0),CorrectionPos(0),CorrectionLen(0), Valid(false), HasUnderline(0) 
{
}

GFxTextHighlightDesc* GFxTextHighlighter::CreateNewHighlighter(GFxTextHighlightDesc* pdesc)
{
    IdComparator cmp;
    Invalidate();
    do 
    {
        ++LastId;
        GFxTextHighlightDesc* pfdesc = GetHighlighterPtr(LastId);
        if (!pfdesc)
        {
            GASSERT(pdesc);
            pdesc->Id = LastId;
            SPInt i = GTL::find_insertion_index_in_sorted_array(Highlighters, pdesc->Id, cmp);
            GASSERT(i >= 0);
            Highlighters.insert((UPInt)i, *pdesc);
            return &Highlighters[(UPInt)i];       
        }
    } while(1);
    return NULL;
}

GFxTextHighlightDesc* GFxTextHighlighter::CreateHighlighter(const GFxTextHighlightDesc& desc)
{
    GASSERT(desc.Id > 0); // Id should be initialized to value other than zero

    Invalidate();
    // check, is id already occupied
    GFxTextHighlightDesc* pdesc = GetHighlighterPtr(desc.Id);
    if (!pdesc)
    {
        IdComparator cmp;
        SPInt i = GTL::find_insertion_index_in_sorted_array(Highlighters, desc.Id, cmp);
        GASSERT(i >= 0);
        Highlighters.insert((UPInt)i, desc);
        return &Highlighters[(UPInt)i];       
    }
    return NULL;
}

GFxTextHighlightDesc* GFxTextHighlighter::GetHighlighterPtr(UInt id)
{
    // TODO sorted search!
    for(UPInt i = 0, n = Highlighters.size(); i < n; ++i)
    {
        if (Highlighters[i].Id == id)
            return &Highlighters[i];
    }
    return NULL;
}


GFxTextHighlightDesc GFxTextHighlighter::GetHighlighter(UInt id) const
{
    // TODO sorted search!
    for(UPInt i = 0, n = Highlighters.size(); i < n; ++i)
    {
        if (Highlighters[i].Id == id)
            return Highlighters[i];
    }
    return GFxTextHighlightDesc();
}

bool GFxTextHighlighter::SetHighlighter(UInt id, const GFxTextHighlightInfo& info)
{
    GFxTextHighlightDesc* pdesc = GetHighlighterPtr(id);
    if (pdesc)
    {
        pdesc->Info = info;
        Invalidate();
        return true;
    }
    return false;
}

bool GFxTextHighlighter::FreeHighlighter(UInt id)
{
    for(UPInt i = 0, n = Highlighters.size(); i < n; ++i)
    {
        if (Highlighters[i].Id == id)
        {
            Highlighters.remove(i);
            Invalidate();
            return true;
        }
    }
    return false;
}

GFxTextHighlighterPosIterator GFxTextHighlighter::GetPosIterator(UPInt startPos, UPInt len) const
{
    return GFxTextHighlighterPosIterator(this, startPos, len);
}

GFxTextHighlighterRangeIterator GFxTextHighlighter::GetRangeIterator(UPInt startPos, UInt flags) const
{
    return GFxTextHighlighterRangeIterator(this, startPos, flags);
}

void GFxTextHighlighter::UpdateGlyphIndices(const GFxTextCompositionString* pcs)
{
    CorrectionPos = CorrectionLen = 0;
#ifndef GFC_NO_IME_SUPPORT
    if (pcs)
    {
        CorrectionPos = pcs->GetPosition();
        CorrectionLen = pcs->GetLength();
    }
#else
    GUNUSED(pcs);
#endif
    Invalidate();
    for(UPInt i = 0, n = Highlighters.size(); i < n; ++i)
    {
        GFxTextHighlightDesc& desc = Highlighters[i];

        desc.GlyphIndex = desc.StartPos;
        desc.GlyphNum   = desc.Length;
        if (CorrectionLen > 0)
        {
            if (desc.ContainsPos(CorrectionPos))
            {
                if (desc.Offset >= 0)
                    desc.GlyphIndex += desc.Offset;
                else
                    desc.GlyphNum += CorrectionLen;
            }
            else if (desc.StartPos > CorrectionPos)
            {
                desc.GlyphIndex += CorrectionLen;
            }
        }
    }
}

bool GFxTextHighlighter::HasUnderlineHighlight() const
{
    if (HasUnderline == 0)
    {
        HasUnderline = -1;
        for(UPInt i = 0, n = Highlighters.size(); i < n; ++i)
        {
            const GFxTextHighlightDesc& desc = Highlighters[i];
            if (desc.Info.HasUnderlineStyle())
            {
                HasUnderline = 1;
                break;
            }
        }
    }
    return HasUnderline == 1;
}

//////////////////////////////////////////////////////////////////////////
// GFxTextHighlighterRangeIterator
// 
GFxTextHighlighterRangeIterator::GFxTextHighlighterRangeIterator
(const GFxTextHighlighter* pmanager, UPInt startPos, UInt flags) : 
    pManager(pmanager), CurTextPos(startPos), CurRangeIndex(0), Flags(flags)
{
    InitCurDesc();
}

void GFxTextHighlighterRangeIterator::InitCurDesc()
{
    UPInt nearestNextPos;
    GFxTextHighlightDesc desc;
    do 
    {
        nearestNextPos = GFC_MAX_UPINT;
        for (UPInt i = 0, n = pManager->Highlighters.size(); i < n; ++i)
        {
            const GFxTextHighlightDesc& curd = pManager->Highlighters[i];

            if ((curd.Info.Flags & Flags) != 0)
            {
                if (curd.ContainsIndex(CurTextPos))
                {
                    if (desc.IsEmpty())
                    {
                        desc     = curd;
                        desc.GlyphNum -= CurTextPos - desc.GlyphIndex;
                        desc.GlyphIndex= CurTextPos;
                        if (desc.GlyphIndex + desc.GlyphNum > nearestNextPos)
                        {
                            desc.GlyphNum = nearestNextPos - desc.GlyphIndex;
                            nearestNextPos = desc.GlyphIndex + desc.GlyphNum;
                        }
                        //c//urLen   = curd.GlyphNum;
                    }
                    else
                    {
                        if ((curd.Info.Flags & Flags) != Flags)
                        {
                            desc.Info.Prepend(curd.Info);
                            desc.GlyphNum = GTL::gpmin(curd.GlyphIndex + curd.GlyphNum, desc.GlyphIndex + desc.GlyphNum) - desc.GlyphIndex;
                            nearestNextPos = desc.GlyphIndex + desc.GlyphNum;
                        }
                    }
                }
                if (curd.GlyphIndex > CurTextPos)
                {
                    nearestNextPos = GTL::gpmin(nearestNextPos, curd.GlyphIndex);
                    if (!desc.IsEmpty() && desc.GlyphIndex + desc.GlyphNum > nearestNextPos)
                        desc.GlyphNum = nearestNextPos - desc.GlyphIndex;
                }
            }
        }
        CurDesc = desc;
        CurTextPos = nearestNextPos;
    } while(desc.IsEmpty() && nearestNextPos != GFC_MAX_UPINT);
}


GFxTextHighlightDesc GFxTextHighlighterRangeIterator::operator*()
{
    return CurDesc;
}

void GFxTextHighlighterRangeIterator::operator++(int)
{
    if (!IsFinished())
    {
        //CurTextPos = CurDesc.GlyphIndex + CurDesc.GlyphNum;
        InitCurDesc();
    }
}

bool GFxTextHighlighterRangeIterator::IsFinished() const
{
    return !(CurRangeIndex < pManager->Highlighters.size()) || CurDesc.GlyphNum == 0;
}

//////////////////////////////////////////////////////////////////////////
// GFxTextHighlighterPosIterator
// 
GFxTextHighlighterPosIterator::GFxTextHighlighterPosIterator
    (const GFxTextHighlighter* pmanager, UPInt startPos, UPInt len) : 
    pManager(pmanager), CurGlyphIndex(startPos), NumGlyphs(len)
{
    InitCurDesc();
}

void GFxTextHighlighterPosIterator::InitCurDesc()
{
    if (!IsFinished())
    {
        GFxTextHighlightDesc desc;
        for (UPInt i = 0, n = pManager->Highlighters.size(); i < n; ++i)
        {
            const GFxTextHighlightDesc& curd = pManager->Highlighters[i];
            if (curd.ContainsIndex(CurGlyphIndex))
                desc.Info.Prepend(curd.Info);
        }
        CurDesc = desc;
        CurDesc.GlyphNum   = 1;
    }
    else
    {
        CurDesc.Info.Reset();
        CurDesc.GlyphNum   = 0;
    }
    CurDesc.GlyphIndex = CurGlyphIndex;
    CurDesc.Id         = 0;
}



void GFxTextHighlighterPosIterator::operator++(int)
{
    if (!IsFinished())
    {
        ++CurGlyphIndex;
        InitCurDesc();
    }
}

void GFxTextHighlighterPosIterator::operator+=(UPInt p)
{
    if (!IsFinished() && p > 0)
    {
        CurGlyphIndex += p;
        InitCurDesc();
    }
}

bool GFxTextHighlighterPosIterator::IsFinished() const
{
    return CurGlyphIndex >= NumGlyphs;
}

