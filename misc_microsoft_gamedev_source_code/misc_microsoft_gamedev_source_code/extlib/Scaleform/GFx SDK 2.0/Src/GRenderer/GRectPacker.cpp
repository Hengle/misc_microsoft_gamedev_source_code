/**********************************************************************

Filename    :   GRectPacker.cpp
Content     :   
Created     :   2007
Authors     :   Maxim Shemanarev

Copyright   :   (c) 2001-2007 Scaleform Corp. All Rights Reserved.

Notes       :   Specialized simple containers and functions

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GRectPacker.h"

#ifdef GDEBUGDRAW
#include "AggDraw.h"
extern AggDraw* DrawPtr;
#endif

//----------------------------------------------------------------------------
GRectPacker::GRectPacker(): 
    Width(1024), 
    Height(1024) {}

//----------------------------------------------------------------------------
void GRectPacker::Pack()
{
    PackedRects.removeAll();
    Packs.removeAll();
    PackTree.removeAll();
    if (SrcRects.size() == 0) return;
    GAlg::QuickSort(SrcRects, cmpRects);

    MinWidth  = SrcRects[SrcRects.size() - 1].x;
    MinHeight = SrcRects[SrcRects.size() - 1].y;
    NumPacked = 0;

    do
    {
        unsigned prevPacked = NumPacked;
        PackTree.removeAll();
        NodeType rootNode;
        rootNode.x      = 0;
        rootNode.y      = 0;
        rootNode.Width  = Width; 
        rootNode.Height = Height; 
        rootNode.Id     = ~0U;
        rootNode.Node1  = ~0U;
        rootNode.Node2  = ~0U;
        PackTree.add(rootNode);
        packRects(0, 0);
        if(NumPacked > prevPacked)
        {
            PackType pack;
            pack.StartRect = PackedRects.size();
            emitPacked();
            pack.NumRects  = PackedRects.size() - pack.StartRect;
            Packs.add(pack);
        }
    }
    while(NumPacked < SrcRects.size());
}

//----------------------------------------------------------------------------
void GRectPacker::packRects(unsigned nodeIdx, unsigned start)
{
    unsigned i;
    const NodeType& node = PackTree[nodeIdx];
    if(node.Width >= MinWidth && node.Height >= MinHeight)
    {
        for(i = start; i < SrcRects.size(); ++i)
        {
            RectType& rect = SrcRects[i];
            if((rect.Id & Packed) == 0)
            {
                if(rect.x <= node.Width && rect.y <= node.Height)
                {
                    splitSpace(nodeIdx, rect);
                    rect.Id |= Packed;
                    ++NumPacked;
                    packRects(node.Node1, i);
                    packRects(node.Node2, i);
                    return;
                }
            }
        }
    }

}

//----------------------------------------------------------------------------
void GRectPacker::splitSpace(unsigned nodeIdx, const RectType& rect)
{
    // Split the working area vertically with respect
    // to the rect that is being stored.
    //---------------------------
    NodeType& node  = PackTree[nodeIdx];
    NodeType  node1 = node;
    NodeType  node2 = node;

    node1.x      += rect.x;
    node1.Height  = rect.y;
    node1.Width  -= rect.x;
    node2.y      += rect.y;
    node2.Height -= rect.y;

    PackTree.add(node1);
    PackTree.add(node2);

    // This pack area now represents the rect that is just stored, 
    // so save the relevant info to it, and assign the children.
    node.Width    = rect.x;
    node.Height   = rect.y;
    node.Id       = rect.Id;
    node.Node1    = PackTree.size() - 2;
    node.Node2    = PackTree.size() - 1;
}

//----------------------------------------------------------------------------
void GRectPacker::emitPacked()
{
    unsigned i;
    RectType   rect;
    for(i = 0; i < PackTree.size(); ++i)
    {
        const NodeType& node = PackTree[i];
        if(node.Id != ~0U)
        {
            rect.x        = node.x;
            rect.y        = node.y;
            rect.Id       = node.Id;
            PackedRects.add(rect);
        }
    }
}
