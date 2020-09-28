/**********************************************************************

Filename    :   GFxGlyphFitter.cpp
Content     :   
Created     :   
Authors     :   Maxim Shemanarev

Copyright   :   (c) 2001-2007 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GFxGlyphFitter.h"


//------------------------------------------------------------------------
void GFxGlyphFitter::Reset() 
{ 
    Contours.removeAll(); 
    Vertices.removeAll(); 
    LerpRampX.removeAll();
    LerpRampY.removeAll();
}

//------------------------------------------------------------------------
void GFxGlyphFitter::MoveTo(int x, int y)
{
    ContourType c;
    VertexType  v;
    c.NumVertices = 1;
    c.StartVertex = Vertices.size();
    v.x = SInt16(x);
    v.y = SInt16(y);
    Contours.add(c);
    Vertices.add(v);
}

//------------------------------------------------------------------------
void GFxGlyphFitter::LineTo(int x, int y)
{
    const VertexType& pv = Vertices.last();
    VertexType  tv;
    tv.x = SInt16(x);
    tv.y = SInt16(y);
    if(tv.x != pv.x || tv.y != pv.y)
    {
        Vertices.add(tv);
        ++Contours.last().NumVertices;
    }
}

//------------------------------------------------------------------------
void GFxGlyphFitter::removeDuplicateClosures()
{
    UInt i;
    for(i = 0; i < Contours.size(); ++i)
    {
        ContourType& c = Contours[i];
        if(c.NumVertices > 2)
        {
            const VertexType& v1 = Vertices[c.StartVertex];
            const VertexType& v2 = Vertices[c.StartVertex + c.NumVertices - 1];
            if(v1.x == v2.x && v1.y == v2.y)
            {
                --c.NumVertices;
            }
        }
    }
}

//------------------------------------------------------------------------
void GFxGlyphFitter::computeBounds()
{
    UInt i, j;

    SInt16 minX =  32767;
    SInt16 minY =  32767;
    SInt16 maxX = -32767;
    SInt16 maxY = -32767;

    MinX =  32767;
    MinY =  32767;
    MaxX = -32767;
    MaxY = -32767;

    for(i = 0; i < Contours.size(); ++i)
    {
        const ContourType& c = Contours[i];
        if(c.NumVertices > 2)
        {
            VertexType v1 = Vertices[c.StartVertex + c.NumVertices - 1];
            int sum = 0;
            for(j = 0; j < c.NumVertices; ++j)
            {
                const VertexType& v2 = Vertices[c.StartVertex + j];
                if(v2.x < minX) minX = v2.x;
                if(v2.y < minY) minY = v2.y;
                if(v2.x > maxX) maxX = v2.x;
                if(v2.y > maxY) maxY = v2.y;
                sum += v1.x * v2.y - v1.y * v2.x;
                v1 = v2;
            }

            if(minX < MinX || minY < MinY || maxX > MaxX || maxY > MaxY)
            {
                MinX = minX;
                MinY = minY;
                MaxX = maxX;
                MaxY = maxY;
                Direction = (sum > 0) ? DirCCW : DirCW;
            }
        }
    }
}

//------------------------------------------------------------------------
void GFxGlyphFitter::detectEvents(FitDir dir)
{
    UInt i, j;
    int idx;
    int minCoord = (dir == FitX) ? MinX : MinY;

    Events.allocate((dir == FitX) ? MaxX - MinX + 1 : MaxY - MinY + 1);
    Events.zero();

    for(i = 0; i < Contours.size(); ++i)
    {
        const ContourType& c = Contours[i];
        if(c.NumVertices > 2)
        {
            Events[0]                 = DirCW | DirCCW;
          //Events[Events.size() - 1] = DirCW | DirCCW;

            for(j = 0; j < c.NumVertices; ++j)
            {
                unsigned i1 = j;
                unsigned i2 = (j + 1) % c.NumVertices;
                unsigned i3 = (j + 2) % c.NumVertices;
                VertexType v1 = Vertices[c.StartVertex + i1]; 
                VertexType v2 = Vertices[c.StartVertex + i2]; 
                VertexType v3 = Vertices[c.StartVertex + i3];

                if(dir == FitX)
                {
                    SInt16 t;
                    t = v1.x; v1.x = -v1.y; v1.y = t;
                    t = v2.x; v1.x = -v2.y; v2.y = t;
                    t = v3.x; v1.x = -v3.y; v3.y = t;
                }

                bool done = false;
                if((v1.y >= v2.y && v3.y >= v2.y) ||
                   (v1.y <= v2.y && v3.y <= v2.y))
                {
                    // Local min or max
                    //------------------
                    idx = v2.y - minCoord;
                    if(v1.x <= v2.x && v2.x <= v3.x)
                    {
                        Events[idx] |= (Direction == DirCW)? DirCCW: DirCW;
                        done = true;
                    }
                    if(v1.x >= v2.x && v2.x >= v3.x)
                    {
                        Events[idx] |= (Direction == DirCW)? DirCW: DirCCW;
                        done = true;
                    }
                }

                if(!done)
                {
                    if(v1.y == v2.y)
                    {
                        // Flat shoulder
                        //-------------------
                        idx = v2.y - minCoord;
                        if(v1.x < v2.x)
                            Events[idx] |= (Direction == DirCW)? DirCCW: DirCW;

                        if(v1.x > v2.x)
                            Events[idx] |= (Direction == DirCW)? DirCW: DirCCW;
                    }
                }
            }
        }
    }
}

//------------------------------------------------------------------------
void GFxGlyphFitter::computeLerpRamp(FitDir dir, int unitsPerPixel, int middle,
                                     int lowerCaseTop, int upperCaseTop)
{
    LerpPairs.removeAll();
    VertexType lerpPair;

    lerpPair.x = SInt16(-SnappedHeight * 4);
    lerpPair.y = SInt16(-SnappedHeight * 4);
    LerpPairs.add(lerpPair);

    int prev  = -32767;
    int top1  = lowerCaseTop;
    int top2  = upperCaseTop;
    int top1s = snapToPixel(top1 + unitsPerPixel, unitsPerPixel);
    int top2s = snapToPixel(top2 + unitsPerPixel, unitsPerPixel);
    int y, snapped;
    int minCoord = (dir == FitX) ? MinX : MinY;
    int minDist = unitsPerPixel + 1;//unitsPerPixel / 2;
    UByte event;

    unsigned i;

    for(i = 0; i < Events.size(); ++i)
    {
        y = int(i) + minCoord;
        event = Events[i];

        if(y <= middle || dir == FitX)
        {
            // Snap to bottom
            //----------------------
            if(event & DirCW)
            {
                if(y > prev + minDist)
                {
                    snapped = snapToPixel(y + unitsPerPixel / 2 + 1, unitsPerPixel);
                    if(LerpPairs.last().y != snapped)
                    {
                        lerpPair.x = (SInt16)y;
                        lerpPair.y = (SInt16)snapped;
                        LerpPairs.add(lerpPair);
                    }
                    prev = y;
                }
            }
        }
        else
        {
            // Snap to top
            //----------------------
            if(event & DirCCW)
            {
                bool done = false;
                if(top2)
                {
                    if(y >= top2 && y < top2 + minDist)
                    {
                        if(y <= prev + minDist ||
                           LerpPairs.last().y + unitsPerPixel >= top2s)
                        {
                            LerpPairs.removeLast();
                        }
                        lerpPair.x = (SInt16)y;
                        lerpPair.y = (SInt16)top2s;
                        LerpPairs.add(lerpPair);
                        prev = y;
                        done = true;
                    }
                    else
                    if(y >= top1 && y < top1 + minDist)
                    {
                        if(y <= prev + minDist ||
                           LerpPairs.last().y + unitsPerPixel >= top1s)
                        {
                            LerpPairs.removeLast();
                        }
                        lerpPair.x = (SInt16)y;
                        lerpPair.y = (SInt16)top1s;
                        LerpPairs.add(lerpPair);
                        prev = y;
                        done = true;
                    }
                }
                if(!done)
                {
                    snapped = snapToPixel(y + unitsPerPixel, unitsPerPixel);
                    if(y <= prev + minDist ||
                       LerpPairs.last().y + unitsPerPixel >= snapped)
                    {
                        LerpPairs.removeLast();
                    }
                    lerpPair.x = (SInt16)y;
                    lerpPair.y = (SInt16)snapped;
                    LerpPairs.add(lerpPair);
                    prev = y;
                }
            }
        }
    }

    lerpPair.x = SInt16(SnappedHeight * 4);
    lerpPair.y = SInt16(SnappedHeight * 4);
    LerpPairs.add(lerpPair);

    GPodVector<SInt16>& ramp = (dir == FitX) ? LerpRampX : LerpRampY;
    ramp.allocate(Events.size());

    VertexType v1 = LerpPairs[0];
    VertexType v2 = LerpPairs[1];
    unsigned topIdx = 2;
    for(i = 0; i < Events.size(); ++i)
    {
        y = int(i) + minCoord;
        if(y >= v2.x && topIdx < LerpPairs.size())
        {
            v1 = v2;
            v2 = LerpPairs[topIdx++];
        }
        ramp[i] = SInt16(v1.y + (y - v1.x) * (v2.y - v1.y) / (v2.x - v1.x) - minCoord);
    }
}

//------------------------------------------------------------------------
void GFxGlyphFitter::FitGlyph(int heightInPixels, int widthInPixels, 
                              int lowerCaseTop,   int upperCaseTop)
{
    UnitsPerPixelX = widthInPixels  ? NominalFontHeight / widthInPixels  : 1;
    UnitsPerPixelY = heightInPixels ? NominalFontHeight / heightInPixels : 1;
    SnappedHeight  = NominalFontHeight / UnitsPerPixelY * UnitsPerPixelY;

    if(heightInPixels || widthInPixels)
    {
        removeDuplicateClosures();
        computeBounds();
        if(heightInPixels && MaxY > MinY) 
        {
            detectEvents(FitY);
            computeLerpRamp(FitY, UnitsPerPixelY, 
                            MinY + (MaxY - MinY)/3, 
                            lowerCaseTop, upperCaseTop);
        }
        if(widthInPixels && MaxY > MinY) 
        {
            detectEvents(FitX);
            computeLerpRamp(FitX, UnitsPerPixelX, 
                            MinX + (MaxX - MinX)/3, 0, 0);
        }
    }
}
