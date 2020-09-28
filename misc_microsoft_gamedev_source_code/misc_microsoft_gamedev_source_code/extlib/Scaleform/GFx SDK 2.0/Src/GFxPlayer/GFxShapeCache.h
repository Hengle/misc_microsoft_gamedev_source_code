/**********************************************************************

Filename    :   GFxShapeCache.h
Content     :   Simple LRU cache for GFxShapeCharacterDef
Created     :   6/21/2007
Authors     :   Maxim Shemanarev

Copyright   :   (c) 2001-2007 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

----------------------------------------------------------------------
The code of these classes was taken from the Anti-Grain Geometry
Project and modified for the use by Scaleform. 
Permission to use without restrictions is hereby granted to 
Scaleform Corporation by the author of Anti-Grain Geometry Project.
See http://antigtain.com for details.
**********************************************************************/

#ifndef INC_GFxShapeCache_H
#define INC_GFxShapeCache_H

#include "GTLTypes.h"
#include "GFxShape.h"
#include "GContainers.h"


//------------------------------------------------------------------------
class GFxShapeCacheNode : public GPodDListNode<GFxShapeCacheNode>
{
public:
    GPtr<GFxShapeCharacterDef> pShape;
    UInt                       Stamp;
};


//------------------------------------------------------------------------
class GFxShapeCache : public GNewOverrideBase
{
public:
    GFxShapeCache(UInt maxNumShapes) :
        NumShapes(0)
    {
        Shapes.resize(maxNumShapes);
    };

    GFxShapeCacheNode* GetNewShapeNode()
    {
        GFxShapeCacheNode* shape;
        if (NumShapes < Shapes.size())
        {
            shape = &Shapes[NumShapes++];
            shape->pShape = 0;
            shape->Stamp  = 0;
            Queue.PushBack(shape);
            return shape;
        }
        shape = Queue.GetFirst();
        shape->pShape = 0;
        shape->Stamp++;
        Queue.SendToBack(shape);
        return shape;
    }

    void SendToBack(GFxShapeCacheNode* shape)
    {
        Queue.SendToBack(shape);
    }

private:
    GTL::garray<GFxShapeCacheNode> Shapes;
    UInt                           NumShapes;
    GPodDList<GFxShapeCacheNode>   Queue;
};



#endif




