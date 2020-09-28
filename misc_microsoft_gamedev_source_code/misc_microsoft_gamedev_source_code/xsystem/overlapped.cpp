//----------------------------------------------------------------------------
//
//  overlapped.cpp
//
//  Copyright 2007 Ensemble Studios
//
//----------------------------------------------------------------------------
#include "xsystem.h"
#include "overlapped.h"

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOverlapped::BOverlapped(bool createEvent, HANDLE fileHandle) :
   mFileHandle(fileHandle)
{
   hEvent = INVALID_HANDLE_VALUE;
   if (createEvent)
      hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
   Internal=0;
   InternalHigh=0;
   Offset=0;
   OffsetHigh=0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOverlapped::~BOverlapped()
{
   // fixme - do I need to check for a pending overlapped operation here?
   if (hEvent != INVALID_HANDLE_VALUE)
      CloseHandle(hEvent);
   hEvent = INVALID_HANDLE_VALUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BOverlapped::setOffset(uint64 offset)
{
   Offset = (uint)offset;
   OffsetHigh = (uint)(offset >> 32U);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
uint64 BOverlapped::getOffset()
{
   uint64 offset64 = OffsetHigh;
   offset64 = offset64 << 32U;                // shift into position
   offset64 += Offset;

   return offset64;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL BOverlapped::hasOverlappedIOCompleted()
{
   return HasOverlappedIoCompleted(this);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL BOverlapped::getOverlappedResult(DWORD * pNumBytesTransferred, BOOL bWait)
{
   return GetOverlappedResult(mFileHandle, this, pNumBytesTransferred, bWait);
}




