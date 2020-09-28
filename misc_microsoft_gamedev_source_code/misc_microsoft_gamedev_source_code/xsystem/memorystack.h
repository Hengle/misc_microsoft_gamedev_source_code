// Use xcore\memory\memstack.cpp/h
#if 0
//============================================================================
// memorystack.h
// Ensemble Studios (Microsoft) (C) 2004
// 
// Memory Stack implementation based on memory stack article, 
// "Frame Based Memory Allocation" by Steven Ranck (pp. 92-100) in 
// Game Programming Gems 1. 
// "Portions Copyright (C) Steven Ranck, 2000"
//============================================================================
#pragma once

// rg [5/18/06] - DO NOT use this on Xbox yet. Having a global memstack sitting
// around is too wasteful of memory. We should change this to use VirtualAlloc/
// VirtualFree so we can decommit unused pages.
#error Don't use this right now.

const long cDefaultMemoryStackSize = 1024 * 2048; //2MB

class BMemoryStackFrame;

//============================================================================
// class BMemoryStack
//============================================================================
class BMemoryStack
{
   public:
      enum 
      {
         cMemoryStackBottomHeap = 0,
         cMemoryStackTopHeap,
         cNumberHeaps,
      };

      BMemoryStack();
      BMemoryStack(long sizeInBytes, long byteAlignment = 4);
     ~BMemoryStack();

      //-- initialization
      bool              init             (long sizeInBytes, long byteAlignment = 4);
      void              kill             ();
      void              empty            ();
      long              getByteAlignment () const {return mByteAlignment;}
      long              getSize          () const {return mSize;}
      bool              isEmpty          ();
      
      //-- Standard API
      bool              validateIntegrity();

      friend class BMemoryStackFrame;
      
   private:
      bool              getFrame         (BMemoryStackFrame& frame, long heapIndex = cMemoryStackBottomHeap);
      BYTE*             getMemory        (long sizeInBytes, long heapIndex = cMemoryStackBottomHeap, BMemoryStackFrame* pFrame = 0 );
      void              releaseMemory    (const BMemoryStackFrame& frame);   

      DWORD alignHeap(DWORD address, DWORD bytes);

      BYTE* mpMemoryBlock;
      long  mSize;
      long  mByteAlignment;

      BYTE* mpHeap[2];  //-- Two Heaps      (Bottom Heap  = 0 || Top Heap = 1)
      BYTE* mpFrame[2]; //-- Tow Frame Ptrs (Bottom Frame = 0 || Top Frame = 1)
};

extern BMemoryStack gMemoryStack;

//============================================================================
// class BMemoryStackFrame
//============================================================================
class BMemoryStackFrame
{
   public:
                              BMemoryStackFrame(long heapIndex=BMemoryStack::cMemoryStackBottomHeap);
                              ~BMemoryStackFrame();
     
      bool                    isValid(void) const {return(mpFrame != NULL);}
      BYTE*                   getMemory(long sizeInBytes);
      
      friend class BMemoryStack;

   protected:
   
      BYTE* mpFrame;
      long  mHeapIndex;
};
#endif
