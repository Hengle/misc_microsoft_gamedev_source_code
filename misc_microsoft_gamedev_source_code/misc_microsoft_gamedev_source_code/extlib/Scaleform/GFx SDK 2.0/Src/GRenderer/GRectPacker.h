/**********************************************************************

Filename    :   GRectPacker.h
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

#ifndef INC_GRectPacker_H
#define INC_GRectPacker_H

#include "GContainers.h"



//----------------------------------------------------------------------------
class GRectPacker
{
    enum { Packed = 0x80000000U };

public:
    struct RectType
    {
        unsigned x, y, Id;
    }; 

    struct PackType
    {
        unsigned StartRect;
        unsigned NumRects;
    }; 

    GRectPacker();

    //============= Set parameters
    void     SetWidth    (unsigned w) { Width  = w;}
    void     SetHeight   (unsigned h) { Height = h;}
    unsigned GetWidth()     const { return Width;  }
    unsigned GetHeight()    const { return Height; }

    //============= Prepare data and pack
    void RemoveAll()
    {
        SrcRects.removeAll();
        PackedRects.removeAll();
        Packs.removeAll();
        PackTree.removeAll();
        Failed.removeAll();
    }

    void AddRect(unsigned w, unsigned h, unsigned id)
    {
        if(w && h && w <= Width && h <= Height)
        {
            RectType r;
            r.x  = w;
            r.y  = h;
            r.Id = id;
            SrcRects.add(r);
        }
        else
        {
            Failed.add(id);
        }
    }

    void Pack();

    //============= Access the result
    unsigned        GetNumPacks()             const { return Packs.size(); }
    const PackType& GetPack(unsigned packIdx) const { return Packs[packIdx]; }
    const RectType& GetRect(const PackType& pack, unsigned rectIdx) const
    {
        return PackedRects[pack.StartRect + rectIdx];
    }
    unsigned        GetNumFailed()          const { return Failed.size(); }
    unsigned        GetFailed(unsigned idx) const { return Failed[idx]; }

private:
    struct NodeType
    {
        unsigned x, y, Width, Height, Id, Node1, Node2;
    };

    static bool cmpRects(const RectType& a, const RectType& b)
    {
        if(b.y != a.y) return b.y < a.y;
        return b.x < a.x;
    }
    void packRects(unsigned nodeIdx, unsigned start);
    void splitSpace(unsigned nodeIdx, const RectType& rect);
    void emitPacked();

    unsigned                  Width;
    unsigned                  Height;
    unsigned                  NumPacked;
    unsigned                  MinWidth;
    unsigned                  MinHeight;
    GPodBVector<RectType, 8>  SrcRects;
    GPodBVector<RectType, 8>  PackedRects;
    GPodBVector<PackType, 4>  Packs;
    GPodBVector<NodeType, 8>  PackTree;
    GPodBVector<unsigned>     Failed;
};


#endif
