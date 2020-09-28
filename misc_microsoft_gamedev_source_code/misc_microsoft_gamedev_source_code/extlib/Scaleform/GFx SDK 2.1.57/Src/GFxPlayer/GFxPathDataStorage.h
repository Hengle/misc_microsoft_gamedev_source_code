/**********************************************************************

Filename    :   GFxPathDataStorage.h
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

#ifndef INC_GFxPathDataStorage_H
#define INC_GFxPathDataStorage_H

#include "GTypes.h"
#include "GContainers.h"

//------------------------------------------------------------------------
class GFxPathDataStorage
{
public:
    enum RawEdgeType
    {
        Edge_H12 = 0,  //  2 bytes
        Edge_H20 = 1,  //  3 bytes
        Edge_V12 = 2,  //  2 bytes
        Edge_V20 = 3,  //  3 bytes
        Edge_L6  = 4,  //  2 bytes
        Edge_L10 = 5,  //  3 bytes
        Edge_L14 = 6,  //  4 bytes
        Edge_L18 = 7,  //  5 bytes
        Edge_C5  = 8,  //  3 bytes
        Edge_C7  = 9,  //  4 bytes
        Edge_C9  = 10, //  5 bytes
        Edge_C11 = 11, //  6 bytes
        Edge_C13 = 12, //  7 bytes
        Edge_C15 = 13, //  8 bytes
        Edge_C17 = 14, //  9 bytes
        Edge_C19 = 15  // 10 bytes
    };

    enum RangeType
    {
        MaxUInt6  =  (1 << 6)  - 1,
        MaxUInt7  =  (1 << 7)  - 1,
        MaxUInt14 =  (1 << 14) - 1,
        MaxUInt22 =  (1 << 22) - 1,
        MaxUInt30 =  (1 << 30) - 1,

        MinSInt5  = -(1 << 4),
        MaxSInt5  =  (1 << 4)  - 1,
        MinSInt6  = -(1 << 5),
        MaxSInt6  =  (1 << 5)  - 1,
        MinSInt7  = -(1 << 6),
        MaxSInt7  =  (1 << 6)  - 1,
        MinSInt8  = -(1 << 7),
        MaxSInt8  =  (1 << 7)  - 1,
        MinSInt9  = -(1 << 8),
        MaxSInt9  =  (1 << 8)  - 1,
        MinSInt10 = -(1 << 9),
        MaxSInt10 =  (1 << 9)  - 1,
        MinSInt11 = -(1 << 10),
        MaxSInt11 =  (1 << 10) - 1,
        MinSInt12 = -(1 << 11),
        MaxSInt12 =  (1 << 11) - 1,
        MinSInt13 = -(1 << 12),
        MaxSInt13 =  (1 << 12) - 1,
        MinSInt14 = -(1 << 13),
        MaxSInt14 =  (1 << 13) - 1,
        MinSInt15 = -(1 << 14),
        MaxSInt15 =  (1 << 14) - 1,
        MinSInt17 = -(1 << 16),
        MaxSInt17 =  (1 << 16) - 1,
        MinSInt18 = -(1 << 17),
        MaxSInt18 =  (1 << 17) - 1,
        MinSInt19 = -(1 << 18),
        MaxSInt19 =  (1 << 18) - 1,
        MinSInt20 = -(1 << 19),
        MaxSInt20 =  (1 << 19) - 1,
        MinSInt22 = -(1 << 21),
        MaxSInt22 =  (1 << 21) - 1,
    };

    enum EdgeType
    {
        Edge_HLine,
        Edge_VLine,
        Edge_Line,
        Edge_Quad
    };

    GFxPathDataStorage();

    void RemoveAll();

    void WriteChar(char v) { Data.add(UInt8(v)); }

    void WriteUInt16fixlen(UInt v);
    void WriteUInt32fixlen(UInt v);

    void WriteSInt16fixlen(SInt v);
    void WriteSInt32fixlen(SInt v);

    void UpdateUInt16fixlen(UInt pos, UInt v);
    void UpdateUInt32fixlen(UInt pos, UInt v);

    void UpdateSInt16fixlen(UInt pos, SInt v);
    void UpdateSInt32fixlen(UInt pos, SInt v);

    UInt WriteUInt15(UInt v);
    UInt WriteUInt30(UInt v);

    UInt WriteSInt15(SInt v);
    UInt WriteSInt30(SInt v);

    UInt WriteHLine(SInt x);
    UInt WriteVLine(SInt y);
    UInt WriteLine (SInt x, SInt y);
    UInt WriteQuad (SInt cx, SInt cy, SInt ax, SInt ay);

    void CutAt(UInt pos) { Data.cutAt(pos); }

    char ReadChar(UInt pos) const { return (char)Data.valueAt(pos); }

    UInt ReadUInt16fixlen(UInt pos) const;
    UInt ReadUInt32fixlen(UInt pos) const;

    SInt ReadSInt16fixlen(UInt pos) const;
    SInt ReadSInt32fixlen(UInt pos) const;

    UInt ReadSInt15(UInt pos, SInt* v) const;
    UInt ReadSInt30(UInt pos, SInt* v) const;

    UInt ReadUInt15(UInt pos, UInt* v) const;
    UInt ReadUInt30(UInt pos, UInt* v) const;

    UInt ReadRawEdge(UInt pos, UInt8* data) const; // data must be at least UInt8[10];
    UInt ReadEdge   (UInt pos, SInt*  data) const; // data must be at least SInt[5];

    UInt GetSize() const { return Data.size(); }
    void Serialize(void* ptr, UInt start, UInt size) const;
    void Deserialize(const void* ptr, UInt size);

    bool    PathsEqual(UInt pos, const GFxPathDataStorage& cmpStorage, UInt cmpPos) const;
    UInt32  ComputePathHash(UInt pos) const;

private:
    typedef GPodBVector<UInt8, 12> CoordArrayType;
    CoordArrayType                 Data;
    static const UInt8             Sizes[16];
};

//------------------------------------------------------------------------
inline void GFxPathDataStorage::WriteUInt16fixlen(UInt v)
{
    Data.add(UInt8(v));
    Data.add(UInt8(v >> 8));
}

//------------------------------------------------------------------------
inline void GFxPathDataStorage::WriteUInt32fixlen(UInt v)
{
    Data.add(UInt8(v));
    Data.add(UInt8(v >> 8));
    Data.add(UInt8(v >> 16));
    Data.add(UInt8(v >> 24));
}

//------------------------------------------------------------------------
inline void GFxPathDataStorage::WriteSInt16fixlen(SInt v)
{
    Data.add(UInt8(v));
    Data.add(UInt8(v >> 8));
}

//------------------------------------------------------------------------
inline void GFxPathDataStorage::WriteSInt32fixlen(SInt v)
{
    Data.add(UInt8(v));
    Data.add(UInt8(v >> 8));
    Data.add(UInt8(v >> 16));
    Data.add(UInt8(v >> 24));
}

//------------------------------------------------------------------------
inline void GFxPathDataStorage::UpdateUInt16fixlen(UInt pos, UInt v)
{
    Data[pos  ] = UInt8(v);
    Data[pos+1] = UInt8(v >> 8);
}

//------------------------------------------------------------------------
inline void GFxPathDataStorage::UpdateUInt32fixlen(UInt pos, UInt v)
{
    Data[pos  ] = UInt8(v);
    Data[pos+1] = UInt8(v >> 8);
    Data[pos+2] = UInt8(v >> 16);
    Data[pos+3] = UInt8(v >> 24);
}

//------------------------------------------------------------------------
inline void GFxPathDataStorage::UpdateSInt16fixlen(UInt pos, SInt v)
{
    Data[pos  ] = UInt8(v);
    Data[pos+1] = UInt8(v >> 8);
}

//------------------------------------------------------------------------
inline void GFxPathDataStorage::UpdateSInt32fixlen(UInt pos, SInt v)
{
    Data[pos  ] = UInt8(v);
    Data[pos+1] = UInt8(v >> 8);
    Data[pos+2] = UInt8(v >> 16);
    Data[pos+3] = UInt8(v >> 24);
}

//------------------------------------------------------------------------
inline UInt GFxPathDataStorage::WriteUInt15(UInt v)
{
    if (v <= MaxUInt7)
    {
        Data.add(UInt8(v << 1));
        return 1;
    }
    Data.add(UInt8((v << 1) | 1));
    Data.add(UInt8 (v >> 7));
    return 2;
}

//------------------------------------------------------------------------
inline UInt GFxPathDataStorage::WriteSInt15(SInt v)
{
    if (v >= MinSInt7 && v <= MaxSInt7)
    {
        Data.add(UInt8(v << 1));
        return 1;
    }
    Data.add(UInt8((v << 1) | 1));
    Data.add(UInt8 (v >> 7));
    return 2;
}

//------------------------------------------------------------------------
inline UInt GFxPathDataStorage::ReadUInt16fixlen(UInt pos) const
{
    return Data.valueAt(pos) | (Data.valueAt(pos + 1) << 8);
}

//------------------------------------------------------------------------
inline UInt GFxPathDataStorage::ReadUInt32fixlen(UInt pos) const
{
    return Data.valueAt(pos)| 
          (Data.valueAt(pos + 1) << 8 )|
          (Data.valueAt(pos + 2) << 16)|
          (Data.valueAt(pos + 3) << 24);
}

//------------------------------------------------------------------------
inline SInt GFxPathDataStorage::ReadSInt16fixlen(UInt pos) const
{
    return SInt(ReadUInt16fixlen(pos));
}

//------------------------------------------------------------------------
inline SInt GFxPathDataStorage::ReadSInt32fixlen(UInt pos) const
{
    return SInt(ReadUInt32fixlen(pos));
}

//------------------------------------------------------------------------
inline UInt GFxPathDataStorage::ReadUInt15(UInt pos, UInt* v) const
{
    UInt t = UInt8(Data[pos]);
    if ((t & 1) == 0)
    {
        *v = t >> 1;
        return 1;
    }
    t  = UInt(t >> 1);
    *v = UInt(t | (UInt8(Data[pos+1]) << 7));
    return 2;
}

//------------------------------------------------------------------------
inline UInt GFxPathDataStorage::ReadSInt15(UInt pos, SInt* v) const
{
    SInt t = SInt8(Data[pos]);
    if ((t & 1) == 0)
    {
        *v = t >> 1;
        return 1;
    }
    t  = SInt((t >> 1) & MaxUInt7);
    *v = SInt (t | (SInt8(Data[pos+1]) << 7));
    return 2;
}

//------------------------------------------------------------------------
inline UInt GFxPathDataStorage::ReadRawEdge(UInt pos, UInt8* data) const
{
    *data = Data.valueAt(pos++);
    UInt nb = Sizes[*data & 0xF];
    UInt i;
    ++data;
    for (i = 0; i < nb; i++)
        *data++ = Data.valueAt(pos++);

    return nb + 1;
}



#endif
