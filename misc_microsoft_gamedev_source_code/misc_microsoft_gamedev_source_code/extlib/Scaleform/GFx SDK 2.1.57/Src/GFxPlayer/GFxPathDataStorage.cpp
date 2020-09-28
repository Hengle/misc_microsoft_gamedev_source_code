/**********************************************************************

Filename    :   GFxPathDataStorage.cpp
Content     :   
Created     :   2007
Authors     :   Maxim Shemanarev

Copyright   :   (c) 2001-2007 Scaleform Corp. All Rights Reserved.

Notes       :   Compact path data storage

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

For information regarding Commercial License Agreements go to:
online - http://www.scaleform.com/licensing.html or
email  - sales@scaleform.com 

**********************************************************************/

#include "GFxPathDataStorage.h"


//------------------------------------------------------------------------
// Data types. xxxx - payload, S - sign bit; Bit order in bytes: 76543210
//
//           Byte 0      Byte 1
// UInt15:  |xxxxxxx|0|            : 0...127
//          |xxxxxxx|1| |xxxxxxxx| : 128...32767
//
//           Byte 0      Byte 1
// SInt15:  |xxxxxxx|0|            : -64...63
//          |xxxxxxx|1| |Sxxxxxxx| : -16384...16383
//
//           Byte 0      Byte 1     Byte 2     Byte 3
// UInt30:  |xxxxxx|00|                                  : 0...63
//          |xxxxxx|01| |xxxxxxxx|                       : 128...32767
//          |xxxxxx|10| |xxxxxxxx| |xxxxxxxx|            : 32768...2^22-1
//          |xxxxxx|11| |xxxxxxxx| |xxxxxxxx| |xxxxxxxx| : 2^22...2^30-1
//
//           Byte 0      Byte 1     Byte 2     Byte 3
// SInt30:  |xxxxxx|00|                                  : -32...31
//          |xxxxxx|01| |xxxxxxxx|                       : -8192...8191
//          |xxxxxx|10| |xxxxxxxx| |xxxxxxxx|            : -2^21...2^21-1
//          |xxxxxx|11| |xxxxxxxx| |xxxxxxxx| |Sxxxxxxx| : -2^29...2^29-1
//
// Edges:
//          Data for the edges consists of 4-bit edge type and the payload in the 
//          4 bits of this byte plus 1...9 next bytes. 
//
//           Byte 0
//          |xxxx|eeee| |xxxxxxxx| ... where "eeee" - 4-bit edge type
//
// Edge types:
//          Edge_H12 = 0,  //  2 bytes - 12-bit horizontal line (4 + 12      = 16 = 2 bytes)
//          Edge_H20 = 1,  //  3 bytes - 20-bit horizontal line (4 + 20      = 24 = 3 bytes)
//          Edge_V12 = 2,  //  2 bytes - 12-bit vertical line   (4 + 12      = 16 = 2 bytes)
//          Edge_V20 = 3,  //  3 bytes - 20-bit vertical line   (4 + 20      = 24 = 3 bytes)
//          Edge_L6  = 4,  //  2 bytes -  6-bit general line    (4 + 6  + 6  = 16 = 2 bytes)
//          Edge_L10 = 5,  //  3 bytes - 10-bit general line    (4 + 10 + 10 = 24 = 3 bytes)
//          Edge_L14 = 6,  //  4 bytes - 14-bit general line    (4 + 14 + 14 = 32 = 4 bytes)
//          Edge_L18 = 7,  //  5 bytes - 18-bit general line    (4 + 18 + 18 = 40 = 5 bytes)
//          Edge_C5  = 8,  //  3 bytes -  5-bit quadratic curve (4 + 5  + 5  + 5  + 5  = 3 bytes)
//          Edge_C7  = 9,  //  4 bytes -  7-bit quadratic curve (4 + 7  + 7  + 7  + 7  = 4 bytes)
//          Edge_C9  = 10, //  5 bytes -  9-bit quadratic curve (4 + 9  + 9  + 9  + 9  = 5 bytes)
//          Edge_C11 = 11, //  6 bytes - 11-bit quadratic curve (4 + 11 + 11 + 11 + 11 = 6 bytes)
//          Edge_C13 = 12, //  7 bytes - 13-bit quadratic curve (4 + 13 + 13 + 13 + 13 = 7 bytes)
//          Edge_C15 = 13, //  8 bytes - 15-bit quadratic curve (4 + 15 + 15 + 15 + 15 = 8 bytes)
//          Edge_C17 = 14, //  9 bytes - 17-bit quadratic curve (4 + 17 + 17 + 17 + 17 = 9 bytes)
//          Edge_C19 = 15  // 10 bytes - 19-bit quadratic curve (4 + 19 + 19 + 19 + 19 = 10 bytes)
//
// Edge data: 
//          X,Y may mean absolute values as well as relative ones 
//          Horizontal and Vertical lines: X or Y respectively. 
//          General Lines:                 X, Y
//          Quadratic Curves:              CX, CY, AX, AY
//
// An example of Edge_C7:
//           Byte 0      Byte 1      Byte 2      Byte 3
//          |aaaa|1001| |bbbbb|Aaa| |cccccc|Bb| |Ddddddd|C| 
//
// Where:    Aaaaaaa - CX, Bbbbbbb - CY, Ccccccc - AX, Ddddddd - AY
//           (capital letter means sign bit.)
//
//------------------------------------------------------------------------
          

const UInt8 GFxPathDataStorage::Sizes[16] = {1,2,1,2,1,2,3,4,2,3,4,5,6,7,8,9};

//------------------------------------------------------------------------
GFxPathDataStorage::GFxPathDataStorage() : 
    Data(1024)
{
}

//------------------------------------------------------------------------
void GFxPathDataStorage::RemoveAll()
{
    Data.removeAll();
}

//------------------------------------------------------------------------
UInt GFxPathDataStorage::WriteUInt30(UInt v)
{
    if (v <= MaxUInt6)
    {
        Data.add(UInt8(v << 2));
        return 1;
    }
    if (v <= MaxUInt14)
    {
        Data.add(UInt8((v << 2) | 1));
        Data.add(UInt8 (v >> 6));
        return 2;
    }
    if (v <= MaxUInt22)
    {
        Data.add(UInt8((v << 2) | 2));
        Data.add(UInt8 (v >> 6));
        Data.add(UInt8 (v >> 14));
        return 3;
    }
    Data.add(UInt8((v << 2) | 3));
    Data.add(UInt8 (v >> 6));
    Data.add(UInt8 (v >> 14));
    Data.add(UInt8 (v >> 22));
    return 4;
}


//------------------------------------------------------------------------
UInt GFxPathDataStorage::WriteSInt30(SInt v)
{
    if (v >= MinSInt6 && v <= MaxSInt6)
    {
        Data.add(UInt8(v << 2));
        return 1;
    }
    if (v >= MinSInt14 && v <= MaxSInt14)
    {
        Data.add(UInt8((v << 2) | 1));
        Data.add(UInt8 (v >> 6));
        return 2;
    }
    if (v >= MinSInt22 && v <= MaxSInt22)
    {
        Data.add(UInt8((v << 2) | 2));
        Data.add(UInt8 (v >> 6));
        Data.add(UInt8 (v >> 14));
        return 3;
    }
    Data.add(UInt8((v << 2) | 3));
    Data.add(UInt8 (v >> 6));
    Data.add(UInt8 (v >> 14));
    Data.add(UInt8 (v >> 22));
    return 4;
}


//------------------------------------------------------------------------
UInt GFxPathDataStorage::ReadUInt30(UInt pos, UInt* v) const
{
    UInt t = UInt8(Data[pos]);
    switch(t & 3)
    {
    case 0:
        *v = t >> 2;
        return 1;

    case 1:
        t  = t >> 2;
        *v = t | (Data[pos+1] << 6);
        return 2;

    case 2:
        t  = t >> 2;
        t  = t | (Data[pos+1] << 6);
        *v = t | (Data[pos+2] << 14);
        return 3;
    }
    t  = t >> 2;
    t  = t | (Data[pos+1] << 6);
    t  = t | (Data[pos+2] << 14);
    *v = t | (Data[pos+3] << 22);
    return 4;
}


//------------------------------------------------------------------------
UInt GFxPathDataStorage::ReadSInt30(UInt pos, SInt* v) const
{
    SInt t = SInt8(Data[pos]);
    switch(t & 3)
    {
    case 0:
        *v = t >> 2;
        return 1;

    case 1:
        t  = SInt((t >> 2) & MaxUInt6);
        *v = SInt (t | (SInt8(Data[pos+1]) << 6));
        return 2;

    case 2:
        t  = SInt((t >> 2) & MaxUInt6);
        t  = SInt (t | (UInt8(Data[pos+1]) << 6));
        *v = SInt (t | (SInt8(Data[pos+2]) << 14));
        return 3;
    }
    t  = SInt((t >> 2) & MaxUInt6);
    t  = SInt (t | (UInt8(Data[pos+1]) << 6));
    t  = SInt (t | (UInt8(Data[pos+2]) << 14));
    *v = SInt (t | (SInt8(Data[pos+3]) << 22));
    return 4;
}

//------------------------------------------------------------------------
UInt GFxPathDataStorage::WriteHLine(SInt x)
{
    if (x >= MinSInt12 && x <= MaxSInt12)
    {
        Data.add(UInt8((x << 4) | Edge_H12));
        Data.add(UInt8 (x >> 4));
        return 2;
    }
    Data.add(UInt8((x << 4) | Edge_H20));
    Data.add(UInt8 (x >> 4));
    Data.add(UInt8 (x >> 12));
    return 3;
}

//------------------------------------------------------------------------
UInt GFxPathDataStorage::WriteVLine(SInt y)
{
    if (y >= MinSInt12 && y <= MaxSInt12)
    {
        Data.add(UInt8((y << 4) | Edge_V12));
        Data.add(UInt8 (y >> 4));
        return 2;
    }
    Data.add(UInt8((y << 4) | Edge_V20));
    Data.add(UInt8 (y >> 4));
    Data.add(UInt8 (y >> 12));
    return 3;
}

//------------------------------------------------------------------------
UInt GFxPathDataStorage::WriteLine(SInt x, SInt y)
{
    enum { m2=3, m6=0x3F };

    if (x >= MinSInt6 && x <= MaxSInt6 && y >= MinSInt6 && y <= MaxSInt6)
    {
        Data.add(UInt8( (x << 4) | Edge_L6));
        Data.add(UInt8(((x >> 4) & m2) | (y << 2)));
        return 2;
    }
    if (x >= MinSInt10 && x <= MaxSInt10 && y >= MinSInt10 && y <= MaxSInt10)
    {
        Data.add(UInt8 ((x << 4) | Edge_L10));
        Data.add(UInt8(((x >> 4) & m6) | (y << 6)));
        Data.add(UInt8  (y >> 2));
        return 3;
    }
    if (x >= MinSInt14 && x <= MaxSInt14 && y >= MinSInt14 && y <= MaxSInt14)
    {
        Data.add(UInt8 ((x << 4) | Edge_L14));
        Data.add(UInt8  (x >> 4));
        Data.add(UInt8(((x >> 12) & m2) | (y << 2)));
        Data.add(UInt8  (y >> 6));
        return 4;
    }
    Data.add(UInt8 ((x << 4) | Edge_L18));
    Data.add(UInt8  (x >> 4));
    Data.add(UInt8(((x >> 12) & m6) | (y << 6)));
    Data.add(UInt8  (y >> 2));
    Data.add(UInt8  (y >> 10));
    return 5;
}

//------------------------------------------------------------------------
UInt GFxPathDataStorage::WriteQuad(SInt cx, SInt cy, SInt ax, SInt ay)
{
    SInt minV = cx;
    SInt maxV = cx;
    if (cy < minV) minV = cy;
    if (cy > maxV) maxV = cy;
    if (ax < minV) minV = ax;
    if (ax > maxV) maxV = ax;
    if (ay < minV) minV = ay;
    if (ay > maxV) maxV = ay;

    enum { m1=1, m2=3, m3=7, m4=0xF, m5=0x1F, m6=0x3F, m7=0x7F };

    if (minV >= MinSInt5 && maxV <= MaxSInt5)
    {
        Data.add(UInt8(  (cx << 4) | Edge_C5                            ));
        Data.add(UInt8( ((cx >> 4) & m1) | ((cy << 1) & m6) | (ax << 6) ));
        Data.add(UInt8( ((ax >> 2) & m3) |  (ay << 3)                   ));
        return 3;
    }
    if (minV >= MinSInt7 && maxV <= MaxSInt7)
    {
        Data.add(UInt8(  (cx << 4) | Edge_C7            ));
        Data.add(UInt8( ((cx >> 4) & m3) | (cy << 3)    ));
        Data.add(UInt8( ((cy >> 5) & m2) | (ax << 2)    ));
        Data.add(UInt8( ((ax >> 6) & m1) | (ay << 1)    ));
        return 4;
    }
    if (minV >= MinSInt9 && maxV <= MaxSInt9)
    {
        Data.add(UInt8(  (cx << 4) | Edge_C9            ));
        Data.add(UInt8( ((cx >> 4) & m5) | (cy << 5)    ));
        Data.add(UInt8( ((cy >> 3) & m6) | (ax << 6)    ));
        Data.add(UInt8( ((ax >> 2) & m7) | (ay << 7)    ));
        Data.add(UInt8(  (ay >> 1)                      ));
        return 5;
    }
    if (minV >= MinSInt11 && maxV <= MaxSInt11)
    {
        Data.add(UInt8(  (cx << 4) | Edge_C11           ));
        Data.add(UInt8( ((cx >> 4) & m7) | (cy << 7)    ));
        Data.add(UInt8(  (cy >> 1)                      ));
        Data.add(UInt8( ((cy >> 9) & m2) | (ax << 2)    ));
        Data.add(UInt8( ((ax >> 6) & m5) | (ay << 5)    ));
        Data.add(UInt8(  (ay >> 3)                      ));
        return 6;
    }
    if (minV >= MinSInt13 && maxV <= MaxSInt13)
    {
        Data.add(UInt8(  (cx << 4) | Edge_C13           ));
        Data.add(UInt8(  (cx >> 4)                      ));
        Data.add(UInt8( ((cx >> 12) & m1) | (cy << 1)   ));
        Data.add(UInt8( ((cy >> 7)  & m6) | (ax << 6)   ));
        Data.add(UInt8(  (ax >> 2)                      ));
        Data.add(UInt8( ((ax >> 10) & m3) | (ay << 3)   ));
        Data.add(UInt8(  (ay >> 5)                      ));
        return 7;
    }
    if (minV >= MinSInt15 && maxV <= MaxSInt15)
    {
        Data.add(UInt8(  (cx << 4) | Edge_C15           ));
        Data.add(UInt8(  (cx >> 4)                      ));
        Data.add(UInt8( ((cx >> 12) & m3) | (cy << 3)   ));
        Data.add(UInt8(  (cy >> 5)                      ));
        Data.add(UInt8( ((cy >> 13) & m2) | (ax << 2)   ));
        Data.add(UInt8(  (ax >> 6)                      ));
        Data.add(UInt8( ((ax >> 14) & m1) | (ay << 1)   ));
        Data.add(UInt8(  (ay >> 7)                      ));
        return 8;
    }
    if (minV >= MinSInt17 && maxV <= MaxSInt17)
    {
        Data.add(UInt8(  (cx << 4) | Edge_C17           ));
        Data.add(UInt8(  (cx >> 4)                      ));
        Data.add(UInt8( ((cx >> 12) & m5) | (cy << 5)   ));
        Data.add(UInt8(  (cy >> 3)                      ));
        Data.add(UInt8( ((cy >> 11) & m6) | (ax << 6)   ));
        Data.add(UInt8(  (ax >> 2)                      ));
        Data.add(UInt8( ((ax >> 10) & m7) | (ay << 7)   ));
        Data.add(UInt8(  (ay >> 1)                      ));
        Data.add(UInt8(  (ay >> 9)                      ));
        return 9;
    }
    Data.add(UInt8(  (cx << 4) | Edge_C19               ));
    Data.add(UInt8(  (cx >> 4)                          ));
    Data.add(UInt8( ((cx >> 12) & m7) | (cy << 7)       ));
    Data.add(UInt8(  (cy >> 1)                          ));
    Data.add(UInt8(  (cy >> 9)                          ));
    Data.add(UInt8( ((cy >> 17) & m2) | (ax << 2)       ));
    Data.add(UInt8(  (ax >> 6)                          ));
    Data.add(UInt8( ((ax >> 14) & m5) | (ay << 5)       ));
    Data.add(UInt8(  (ay >> 3)                          ));
    Data.add(UInt8(  (ay >> 11)                         ));
    return 10;
}

//------------------------------------------------------------------------
UInt GFxPathDataStorage::ReadEdge(UInt pos, SInt* data) const
{
    UInt8 buff[10];
    UInt  nb = ReadRawEdge(pos, buff);

    switch(buff[0] & 0xF)
    {
    case Edge_H12:
        data[0] = Edge_HLine;
        data[1] = SInt(buff[0] >> 4) | (SInt8(buff[1]) << 4);
        break;

    case Edge_H20:
        data[0] = Edge_HLine;
        data[1] = SInt(buff[0] >> 4) | SInt(buff[1] << 4) | (SInt8(buff[2]) << 12);
        break;

    case Edge_V12:
        data[0] = Edge_VLine;
        data[1] = SInt(buff[0] >> 4) | (SInt8(buff[1]) << 4);
        break;

    case Edge_V20:
        data[0] = Edge_VLine;
        data[1] = SInt(buff[0] >> 4) | SInt(buff[1] << 4) | (SInt8(buff[2]) << 12);
        break;

    case Edge_L6 :
        data[0] = Edge_Line;
        data[1] = SInt (buff[0] >> 4) | (SInt8(buff[1] << 6) >> 2);
        data[2] = SInt8(buff[1]) >> 2;
        break;

    case Edge_L10:
        data[0] = Edge_Line;
        data[1] = SInt(buff[0] >> 4) | (SInt8(buff[1]  << 2) << 2);
        data[2] = SInt(buff[1] >> 6) | (SInt8(buff[2]) << 2);
        break;

    case Edge_L14:
        data[0] = Edge_Line;
        data[1] = SInt(buff[0] >> 4) |  SInt (buff[1]  << 4) | (SInt8(buff[2] << 6) << 6);
        data[2] = SInt(buff[2] >> 2) | (SInt8(buff[3]) << 6);
        break;

    case Edge_L18:
        data[0] = Edge_Line;
        data[1] = SInt(buff[0] >> 4) | SInt(buff[1] << 4) | (SInt8(buff[2]  << 2) << 10);
        data[2] = SInt(buff[2] >> 6) | SInt(buff[3] << 2) | (SInt8(buff[4]) << 10);
        break;

    case Edge_C5 :
        data[0] = Edge_Quad;
        data[1] = SInt(buff[0] >> 4) | (SInt8(buff[1] << 7) >> 3);
        data[2] =                       SInt8(buff[1] << 2) >> 3;
        data[3] = SInt(buff[1] >> 6) | (SInt8(buff[2] << 5) >> 3);
        data[4] =                       SInt8(buff[2])      >> 3;
        break;

    case Edge_C7 :
        data[0] = Edge_Quad;
        data[1] = SInt(buff[0] >> 4) | (SInt8(buff[1] << 5) >> 1);
        data[2] = SInt(buff[1] >> 3) | (SInt8(buff[2] << 6) >> 1);
        data[3] = SInt(buff[2] >> 2) | (SInt8(buff[3] << 7) >> 1);
        data[4] =                       SInt8(buff[3])      >> 1;
        break;

    case Edge_C9 :
        data[0] = Edge_Quad;
        data[1] = SInt(buff[0] >> 4) | (SInt8(buff[1] << 3) << 1);
        data[2] = SInt(buff[1] >> 5) | (SInt8(buff[2] << 2) << 1);
        data[3] = SInt(buff[2] >> 6) | (SInt8(buff[3] << 1) << 1);
        data[4] = SInt(buff[3] >> 7) | (SInt8(buff[4]     ) << 1);
        break;

    case Edge_C11:
        data[0] = Edge_Quad;
        data[1] = SInt(buff[0] >> 4) |                      (SInt8(buff[1] << 1) << 3);
        data[2] = SInt(buff[1] >> 7) | SInt(buff[2] << 1) | (SInt8(buff[3] << 6) << 3);
        data[3] = SInt(buff[3] >> 2) |                      (SInt8(buff[4] << 3) << 3);
        data[4] = SInt(buff[4] >> 5) |                      (SInt8(buff[5]     ) << 3);
        break;

    case Edge_C13:
        data[0] = Edge_Quad;
        data[1] = SInt(buff[0] >> 4) | SInt(buff[1] << 4) | (SInt8(buff[2] << 7) << 5);
        data[2] = SInt(buff[2] >> 1) |                      (SInt8(buff[3] << 2) << 5);
        data[3] = SInt(buff[3] >> 6) | SInt(buff[4] << 2) | (SInt8(buff[5] << 5) << 5);
        data[4] = SInt(buff[5] >> 3) |                      (SInt8(buff[6]     ) << 5);
        break;

    case Edge_C15:
        data[0] = Edge_Quad;
        data[1] = SInt(buff[0] >> 4) | SInt(buff[1] << 4) | (SInt8(buff[2] << 5) << 7);
        data[2] = SInt(buff[2] >> 3) | SInt(buff[3] << 5) | (SInt8(buff[4] << 6) << 7);
        data[3] = SInt(buff[4] >> 2) | SInt(buff[5] << 6) | (SInt8(buff[6] << 7) << 7);
        data[4] = SInt(buff[6] >> 1) |                      (SInt8(buff[7]     ) << 7);
        break;

    case Edge_C17:
        data[0] = Edge_Quad;
        data[1] = SInt(buff[0] >> 4) | SInt(buff[1] << 4) | (SInt8(buff[2] << 3) << 9);
        data[2] = SInt(buff[2] >> 5) | SInt(buff[3] << 3) | (SInt8(buff[4] << 2) << 9);
        data[3] = SInt(buff[4] >> 6) | SInt(buff[5] << 2) | (SInt8(buff[6] << 1) << 9);
        data[4] = SInt(buff[6] >> 7) | SInt(buff[7] << 1) | (SInt8(buff[8]     ) << 9);
        break;

    case Edge_C19:
        data[0] = Edge_Quad;
        data[1] = SInt(buff[0] >> 4) | SInt(buff[1] << 4) | (SInt8(buff[2] << 1) << 11);
        data[2] = SInt(buff[2] >> 7) | SInt(buff[3] << 1) |  SInt (buff[4] << 9) | (SInt8(buff[5] << 6) << 11);
        data[3] = SInt(buff[5] >> 2) | SInt(buff[6] << 6) | (SInt8(buff[7] << 3) << 11);
        data[4] = SInt(buff[7] >> 5) | SInt(buff[8] << 3) | (SInt8(buff[9]     ) << 11);
        break;
    }
    return nb;
}

//------------------------------------------------------------------------
bool GFxPathDataStorage::PathsEqual(UInt pos, const GFxPathDataStorage& cmpStorage, UInt cmpPos) const
{
    UInt size1, size2;
    UInt pos1 = pos;
    UInt pos2 = cmpPos;
    pos1 +=            ReadUInt30(pos1, &size1);
    pos2 += cmpStorage.ReadUInt30(pos2, &size2);

    if (size1 != size2)
        return false;

    size1 >>= 1;
    size2 >>= 1;

    UInt8 edge1[10];
    UInt8 edge2[10];
    while (size1--)
    {
        UInt nb1 =            ReadRawEdge(pos1, edge1);
        UInt nb2 = cmpStorage.ReadRawEdge(pos2, edge2);

        if (nb1 != nb2)
            return false;

        if (memcmp(edge1, edge2, nb1))
            return false;

        pos1 += nb1;
        pos2 += nb2;
    }
    return true;
}

//------------------------------------------------------------------------
UInt32 GFxPathDataStorage::ComputePathHash(UInt pos) const
{
    UInt size;
    UInt32 h = 0;
    UInt8 edge[10];

    pos += ReadUInt30(pos, &size);
    size >>= 1;

    while (size--)
    {
        UInt nb = ReadRawEdge(pos, edge);
        pos += nb;
        for (UInt i = 0; i < nb; ++i)
            h = ((h << 5) + h) ^ (UInt32)edge[i];
    }
    return h;
}

//------------------------------------------------------------------------
void GFxPathDataStorage::Serialize(void* ptr, UInt start, UInt size) const
{
    UInt8* dst = (UInt8*)ptr;
    for(UInt i = 0; i < size; ++i)
        *dst++ = Data.valueAt(start + i);
}

//------------------------------------------------------------------------
void GFxPathDataStorage::Deserialize(const void* ptr, UInt size)
{
    const UInt8* src = (const UInt8*)ptr;
    for(UInt i = 0; i < size; ++i)
        Data.add(*src++);
}

