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

#include "GFxTextHighlight.h"

namespace
{
    struct IdComparator
    {
        int Compare(const GFxTextHighlighter::HighlighterAssoc& p1, int id) const
        {
            return p1.Id - id;
        }
    };
}

//////////////////////////////////////////////////////////////////////////
// GFxTextHighlightDesc
// 
void GFxTextHighlightDesc::Merge(const GFxTextHighlightDesc& mergee)
{
    if (!IsSingleUnderline() && mergee.IsSingleUnderline())
        SetSingleUnderline();
    if (!IsDoubleUnderline() && mergee.IsDoubleUnderline())
        SetDoubleUnderline();
    if (!IsDottedUnderline() && mergee.IsDottedUnderline())
        SetDottedUnderline();
    if (!HasBackgroundColor() && mergee.HasBackgroundColor())
        SetBackgroundColor(mergee.GetBackgroundColor());
    if (!HasTextColor() && mergee.HasTextColor())
        SetTextColor(mergee.GetTextColor());
    if (!HasUnderlineColor() && mergee.HasUnderlineColor())
        SetUnderlineColor(mergee.GetUnderlineColor());
}

//////////////////////////////////////////////////////////////////////////
// GFxTextHighlighter
// 
bool GFxTextHighlighter::CreateHighlighter(const GFxTextHighlightDesc& desc, int *pid)
{
    IdComparator cmp;
    do 
    {
        ++LastId;
        SPInt i = GTL::find_index_in_sorted_array(Id2IndexAssoc, LastId, cmp);
        if (i == -1)
        {
            UPInt index = Highlighters.size();
            Highlighters.append(desc);
            
            i = GTL::find_insertion_index_in_sorted_array(Id2IndexAssoc, LastId, cmp);
            GASSERT(i >= 0);
            Id2IndexAssoc.insert((UPInt)i, GFxTextHighlighter::HighlighterAssoc(LastId, (UInt)index));
            *pid = LastId;
        }
    } while(1);
    return true;       
}

bool GFxTextHighlighter::CreateHighlighter(int id, const GFxTextHighlightDesc& desc)
{
    // check, is id already occupied
    IdComparator cmp;
    SPInt i = GTL::find_index_in_sorted_array(Id2IndexAssoc, LastId, cmp);
    if (i == -1)
    {
        // add highlighter
        UPInt index = Highlighters.size();
        Highlighters.append(desc);

        i = GTL::find_insertion_index_in_sorted_array(Id2IndexAssoc, id, cmp);
        GASSERT(i >= 0);
        Id2IndexAssoc.insert((UPInt)i, GFxTextHighlighter::HighlighterAssoc(id, (UInt)index));
        return true;
    }
    return false;
}

const GFxTextHighlightDesc* GFxTextHighlighter::GetHighlighter(int id) const
{
    IdComparator cmp;
    SPInt i = GTL::find_index_in_sorted_array(Id2IndexAssoc, id, cmp);
    if (i != -1)
    {
        return &Highlighters[Id2IndexAssoc[(UPInt)i].Index];
    }
    return NULL;
}

GFxTextHighlightDesc* GFxTextHighlighter::GetHighlighter(int id)
{
    IdComparator cmp;
    SPInt i = GTL::find_index_in_sorted_array(Id2IndexAssoc, id, cmp);
    if (i != -1)
    {
        return &Highlighters[Id2IndexAssoc[(UPInt)i].Index];
    }
    return NULL;
}

void GFxTextHighlighter::FreeHighlighter(int id)
{
    IdComparator cmp;
    SPInt i = GTL::find_index_in_sorted_array(Id2IndexAssoc, id, cmp);
    if (i != -1)
    {
        const UPInt ui = (UPInt)i;
        Highlighters.remove(Id2IndexAssoc[ui].Index);
        Id2IndexAssoc.remove(ui);
    }
}

//////////////////////////////////////////////////////////////////////////
// GFxTextHighlighterRangeIterator
// 
GFxTextHighlighterRangeIterator::GFxTextHighlighterRangeIterator(GFxTextHighlighter* pmanager) : 
    pManager(pmanager), CurStartingPos(0)
{
    InitCurDesc(CurStartingPos);
}

void GFxTextHighlighterRangeIterator::InitCurDesc(UInt startingPos)
{
    UInt minStartPosIndex, minEndPosIndex;
    int minStartPos = INT_MAX, minEndPos = INT_MAX;
    // find the combined range boundaries first...
    UPInt i, n = pManager->Id2IndexAssoc.size();
    for (i = 0; i < n; ++i)
    {
        UInt descIndex = pManager->Id2IndexAssoc[i].Index;
        const GFxTextHighlightDesc& desc = pManager->Highlighters[descIndex];
        if (desc.StartPos + desc.Length <= startingPos)
            continue;
        if (desc.StartPos < minStartPos)
        {
            minStartPos = desc.StartPos;
            minStartPosIndex = descIndex;
        }
        if (desc.StartPos < minEndPos)
        {
            minEndPos = desc.StartPos;
            minEndPosIndex = descIndex;
        }
        else if (desc.StartPos + (int)desc.Length < minEndPos)
        {
            minEndPos = desc.StartPos + desc.Length;
            minEndPosIndex = descIndex;
        }
    }
    CurDesc.Reset();
    for (i = 0; i < n; ++i)
    {
        UInt descIndex = pManager->Id2IndexAssoc[i].Index;
        const GFxTextHighlightDesc& desc = pManager->Highlighters[descIndex];
        if (desc.StartPos + desc.Length <= startingPos)
            continue;
        if (desc.StartPos >= minStartPos && desc.StartPos <= minEndPos)
        {
            CurDesc.Merge(desc);
        }
    }
    CurDesc.StartPos = minStartPos;
    CurDesc.Length   = minEndPos - minStartPos;
}


GFxTextHighlightDesc GFxTextHighlighterRangeIterator::operator*()
{
    return CurDesc;
}

void GFxTextHighlighterRangeIterator::operator++(int)
{
    if (!IsFinished())
    {
        CurStartingPos = CurDesc.StartPos + CurDesc.Length;
        InitCurDesc(CurStartingPos);
    }
}

bool GFxTextHighlighterRangeIterator::IsFinished() const
{
    return CurDesc.StartPos >= 0;
}

