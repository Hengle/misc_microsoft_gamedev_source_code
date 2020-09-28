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
        *(__int64*)(&Offset) = in_iPosition;
    }

    __int64 Position()
    {
        return reinterpret_cast<__int64>( Pointer );
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
        *(__int64*)(&Offset) += in_iOffset;
    }
    
    void DecrementPosition( 
        const AkUInt32 & in_iOffset
        )
    {
        *(__int64*)(&Offset) -= in_iOffset;
    }
};

#endif //_AK_OVERLAPPED_H_