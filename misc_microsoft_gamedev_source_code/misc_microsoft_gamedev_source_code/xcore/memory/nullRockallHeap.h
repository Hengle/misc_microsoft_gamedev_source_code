//============================================================================
// nullRockallHeap.h
//
// Copyright (c) 2005-2006, Ensemble Studios
//
// Simple replacement of Rockall heap classes
//============================================================================
#pragma once

//----------------------------------------------------------------------------
// BNullRockallHeap
//----------------------------------------------------------------------------
class BNullRockallHeap
{
   public:
      BNullRockallHeap() { }
      BNullRockallHeap(int, bool, bool, bool) { }
      BNullRockallHeap(int, bool, bool, bool, bool, bool) { }

      void* New      (int size, int* actualSize=0, bool zero=false);
      bool  Delete   (void* pData, int size=-1);
      void* Resize   (void* pData, int newSize, int move=1, int* actualSize=0, bool noDelete=false, bool zero=false);
      bool  Details  (void* pData, int* size=0);
      bool  Walk     (bool* active, void** pData, int* size);
      void  DeleteAll(bool recycle = true);
      bool  Verify   (void* pData, int* size=0);
      void  HeapLeaks(void);
      void  SetOwnerThread(DWORD d) { d; }
};
