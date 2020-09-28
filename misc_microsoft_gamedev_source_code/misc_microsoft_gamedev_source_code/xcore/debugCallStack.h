//==============================================================================
// debugcallstack.h
//
// Copyright (c) 2000-2006, Ensemble Studios
//==============================================================================

#pragma once

#ifndef _DEBUGCALLSTACK_H_
#define _DEBUGCALLSTACK_H_

//==============================================================================
// class BDebugCallStackEntry
//==============================================================================
class BDebugCallStackEntry
{
   public:
      enum
      {
         cMaxStringSize=256
      };

      BDebugCallStackEntry() {mModule[0]=0; mFunctionName[0]=0; mFunctionOffset=0; mFile[0]=0; mLine=0; mLineOffset=0; memset(&mContext, 0, sizeof(mContext));}
      ~BDebugCallStackEntry() {}

      // Module.
      WCHAR                   mModule[cMaxStringSize];

      // Function name/offset.
      WCHAR                   mFunctionName[cMaxStringSize];
      DWORD                   mFunctionOffset;

      // File and line.
      WCHAR                   mFile[cMaxStringSize];
      DWORD                   mLine;
      DWORD                   mLineOffset;

      // Context.
      CONTEXT                 mContext;
};


//==============================================================================
// class BDebugCallstack
//==============================================================================
// This uses old-sk00l memory functions because it gets called in places where the memory manager may not be
// available. 
#ifdef new 
   #define crappy
   #undef new
#endif

class BDebugCallstack
{
   public:
                              BDebugCallstack() : mEntries(NULL), mCount(0) {}
                              ~BDebugCallstack() 
                              {
                                 if(mEntries)
                                 {
                                    for(long i=0; i<mCount; i++)
                                       mEntries[i].~BDebugCallStackEntry();
                                    free(mEntries);
                                 }
                              }

      long                    getCount(void) const {return(mCount);}
      const BDebugCallStackEntry *getEntries(void) const {return(mEntries);}

      BDebugCallStackEntry    *grow(void)
                              {
                                 // Allocate/reallocate
                                 long size = sizeof(mEntries[0])*(mCount+1);
                                 
                                 // Get space.
                                 BDebugCallStackEntry *newEntries = (BDebugCallStackEntry*)realloc(mEntries, size);
                                 if(!newEntries)
                                    return(NULL);
                                 
                                 // Save new pointer.
                                 mEntries = newEntries;

                                 // Construct.
                                 new(&(mEntries[mCount])) BDebugCallStackEntry;

                                 // Inc count.
                                 mCount++;

                                 return(&(mEntries[mCount-1]));
                              }

   protected:
      BDebugCallStackEntry    *mEntries;
      long                    mCount;
};

#ifdef crappy
   #define new BANG_NEW
   #undef crappy
#endif


#endif
