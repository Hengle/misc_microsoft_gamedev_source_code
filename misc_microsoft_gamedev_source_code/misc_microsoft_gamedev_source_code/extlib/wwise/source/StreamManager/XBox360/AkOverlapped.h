//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2008 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#ifndef _AK_OVERLAPPED_H_
#define _AK_OVERLAPPED_H_

class CAkOverlapped : public OVERLAPPED
{
public:
    
    void Position( 
        const __int64 & in_iPosition 
        )
    {
        Offset = (DWORD)( in_iPosition & 0xFFFFFFFF );
        OffsetHigh = (DWORD)( in_iPosition >> 32 );
    }

    __int64 Position()
    {
        LARGE_INTEGER iPosition;
        iPosition.LowPart = Offset;
        iPosition.HighPart = OffsetHigh;
        return iPosition.QuadPart;
    }

    void ClearPosition()
    {
        Offset = 0;
        OffsetHigh = 0;
        hEvent = NULL;
    }

    void IncrementPosition( 
        const AkUInt32 & in_iOffset
        )
    {
        DWORD dwTmp = Offset;
        Offset += in_iOffset;
        if ( dwTmp > Offset )
            OffsetHigh++;
    }
    
    void DecrementPosition( 
        const AkUInt32 & in_iOffset
        )
    {
        DWORD dwTmp = Offset;
        Offset -= in_iOffset;
        if ( dwTmp < Offset )
            OffsetHigh--;
    }
};

#endif //_AK_OVERLAPPED_H_