//============================================================================
//
//  Overlapped.h
//
//  Copyright 2007 Ensemble Studios
//
//============================================================================
#pragma once


//----------------------------------------------------------------------------
// Class BOverlapped
// This class is derived from the overlapped structure.
//----------------------------------------------------------------------------
class BOverlapped : public OVERLAPPED
{
public:
   //-- Construction/Destruction
   BOverlapped(bool createEvent, HANDLE fileHandle=INVALID_HANDLE_VALUE);
   ~BOverlapped();
   
   // accessor methods for the OVERLAPPED data structure
   void setOffset(uint64 offset);
   uint64 getOffset();

   HANDLE getEventHandle() { return hEvent; }
   void setEventHandle(HANDLE hevent)  { hEvent = hevent; }

   HANDLE getFileHandle() { return mFileHandle; }
   void setFileHandle(HANDLE handle) { mFileHandle = handle; }

   // helpful methods
   void resetEvent() { ResetEvent(hEvent); }
   BOOL hasOverlappedIOCompleted();
   BOOL getOverlappedResult(DWORD * pNumBytesTransferred, BOOL bWait=true);


protected:
   HANDLE         mFileHandle;       // file handle

};
