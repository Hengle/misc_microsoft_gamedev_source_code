// Use xcore\memory\memstack.cpp/h
#if 0
//============================================================================
// memorystack.cpp
// Ensemble Studios (Microsoft) (c) 2004
//
// Memory Stack implementation based on memory stack article, 
// "Frame Based Memory Allocation" by Steven Ranck (pp. 92-100) in 
// Game Programming Gems 1. 
// "Portions Copyright (C) Steven Ranck, 2000"
//============================================================================
#include "xsystem.h"
#include "memorystack.h"


//============================================================================
// BMemoryStackFrame::BMemoryStackFrame
//============================================================================
BMemoryStackFrame::BMemoryStackFrame(long heapIndex) : 
   mpFrame(0), 
   mHeapIndex(-1) 
{
   gMemoryStack.getFrame(*this, heapIndex);
};

//============================================================================
// BMemoryStackFrame::BMemoryStackFrame()
//============================================================================
BMemoryStackFrame::~BMemoryStackFrame()
{
   gMemoryStack.releaseMemory(*this);
}

//============================================================================
// BMemoryStackFrame::getMemory
//============================================================================
BYTE *BMemoryStackFrame::getMemory(long sizeInBytes)
{
   return(gMemoryStack.getMemory(sizeInBytes, mHeapIndex));
}




//============================================================================
// BMemoryStack::BMemoryStack()
//============================================================================
BMemoryStack::BMemoryStack():
   mpMemoryBlock(0),
   mSize(0),
   mByteAlignment(-1)
{
   memset(mpHeap,  0, sizeof(BYTE*) * cNumberHeaps);
   memset(mpFrame, 0, sizeof(BYTE*) * cNumberHeaps);
}

//============================================================================
// BMemoryStack::BMemoryStack()
//============================================================================
BMemoryStack::BMemoryStack(long sizeInBytes, long byteAlignment):
   mpMemoryBlock(NULL),
   mSize(0),
   mByteAlignment(-1)
{
   memset(mpHeap,  NULL, sizeof(BYTE*) * cNumberHeaps);
   memset(mpFrame, NULL, sizeof(BYTE*) * cNumberHeaps);
   init(sizeInBytes, byteAlignment);
}

//============================================================================
// BMemoryStack::BMemoryStack()
//============================================================================
BMemoryStack::~BMemoryStack()
{
   kill();
}

//============================================================================
// BMemoryStack::init
//============================================================================
bool BMemoryStack::init(long sizeInBytes, long byteAlignment)
{
   //-- kill anything we had before if we get initialized twice
   kill();

   //-- align the number of bytes to the passed in byte alignment
   //-- for coherency.  This allows us to use this for different
   //-- memory alignment schemes
   sizeInBytes = alignHeap(sizeInBytes, byteAlignment);
   mSize = sizeInBytes;

   mpMemoryBlock = new BYTE[mSize];
   if (!mpMemoryBlock)
      return false;

   //-- store our byte alignment
   mByteAlignment = byteAlignment;

   //-- initialize and align the Bottom Heap
   mpHeap[cMemoryStackBottomHeap] = (BYTE*) alignHeap((DWORD) mpMemoryBlock, mByteAlignment);
   //-- initialize and align the Top Heap
   mpHeap[cMemoryStackTopHeap] = (BYTE*) alignHeap ((DWORD)mpMemoryBlock + mSize, mByteAlignment);

   BASSERT(mpHeap[cMemoryStackBottomHeap] == mpMemoryBlock);
   BYTE* pAssert = mpMemoryBlock+(sizeof(BYTE)*mSize);
   BASSERT(mpHeap[cMemoryStackTopHeap] == pAssert);

   //-- initialize our initial free memory frame pointers
   mpFrame[cMemoryStackBottomHeap] = mpHeap[cMemoryStackBottomHeap];
   mpFrame[cMemoryStackTopHeap]    = mpHeap[cMemoryStackTopHeap];

   return true;
}

//============================================================================
// BMemoryStack::kill
//============================================================================
void BMemoryStack::kill()
{
   if (mpMemoryBlock)
      delete [] mpMemoryBlock;

   mpMemoryBlock  = NULL;
   mByteAlignment = -1;
   mSize          = 0;
   memset(mpHeap,  NULL, sizeof(BYTE*) * cNumberHeaps);
   memset(mpFrame, NULL, sizeof(BYTE*) * cNumberHeaps);   
}

//============================================================================
// BMemoryStack::empty()
// forces the stack to be empty. Once empty is called calling systems
// should not use any pointers they have to memory locations into the
// memory stack b/c they are probably going to be used by some other
// system.
//============================================================================
void BMemoryStack::empty()
{
   //-- set the frame pointers to their intial heap positions.
   //-- this makes the memory available again to any caller system.
   mpFrame[cMemoryStackBottomHeap] = mpHeap[cMemoryStackBottomHeap];
   mpFrame[cMemoryStackTopHeap]    = mpHeap[cMemoryStackTopHeap];
}

//============================================================================
// BMemoryStack::getFrame()
// returns a snapshot of the current starting frame of the given heap
// MemoryStackFrames are used to release previously requested memory.
// Any memory that was requested after this snapshot is taken will be
// freed as well.
//============================================================================
bool BMemoryStack::getFrame(BMemoryStackFrame& frame, long heapIndex)
{   
   if (heapIndex < 0 || heapIndex >= cNumberHeaps)
      return false;
   
   frame.mpFrame    = mpFrame[heapIndex];
   frame.mHeapIndex = heapIndex;

   return true;
}
//============================================================================
// BMemoryStack::getMemory()
// allows a system to request some memory from either bottom or top heap
// The bottom and top heaps are pointers at the the edges of the allocated
// memory that converge in as memory gets used.  When the top and bottom
// Frame pointers cross the memory stack is in full use.
//============================================================================
BYTE* BMemoryStack::getMemory(long sizeInBytes, long heapIndex, BMemoryStackFrame* pFrame)
{
   BYTE* pMemory = NULL;

   //-- align the size to our byte alignment
   sizeInBytes = alignHeap(sizeInBytes, mByteAlignment);

   //-- make sure we have enough memory available; if our frame pointers
   //-- cross each other then we don't have enough free memory left so
   //-- we bail out b/c we can request that much space.
   if (mpFrame[0] + sizeInBytes > mpFrame[1])
      return NULL;

   //-- if they also want a frame snapshot then give it to them.
   if (pFrame)
   {
      if (!getFrame(*pFrame, heapIndex))
         return NULL;
   }

   //-- get memory from the top or bottom heap
   if (heapIndex == cMemoryStackTopHeap)
   {
      //-- decrement our pointer downward first b/c we are getting memory 
      //-- from the top heap
      mpFrame[cMemoryStackTopHeap] -= sizeInBytes;
      pMemory = mpFrame[cMemoryStackTopHeap];
   }
   else
   {
      //-- from the botom heap the start of the passed in memory is
      //-- the actual bottom memory frame
      pMemory = mpFrame[cMemoryStackBottomHeap];

      //-- increment our frame pointer by the used up amount
      mpFrame[cMemoryStackBottomHeap] += sizeInBytes;
   }

   return pMemory;
}

//============================================================================
// BMemoryStack::releaseMemory()
// Releases all the memory from this frame forward.
// This means that if you get 3 frames of memory but then release frame 0
// all of the memory from Frame 0 on is freed.  This way the memory
// behaves like a stack and we can release whole memory stack frames.
//============================================================================
void BMemoryStack::releaseMemory(const BMemoryStackFrame& frame)
{
   //-- validate proper release of bottom heap
   //-- if this is the bottom heap make sure that the frame pointer to be released
   //-- is smaller or equal to the most recently used lower heap
   //-- memory block that was requested.
   BASSERT(frame.mHeapIndex == cMemoryStackTopHeap || frame.mpFrame <= mpFrame[cMemoryStackBottomHeap]);
   
   //-- do a real if check for release mode so we don't corrupt the stack in release mode when
   //-- asserts go away.
   if (frame.mHeapIndex == cMemoryStackBottomHeap && frame.mpFrame > mpFrame[cMemoryStackBottomHeap])
   {
      //-- If this happens someone is using the memory stack incorrectly
      BASSERT(0);
      return;
   }

   //-- validate proper relase of the top Heap
   //-- if this is the top heap then make sure that the frame pointer to be
   //-- release is greater or equal to the most recently used top heap
   //-- memory block that was requested.
   BASSERT(frame.mHeapIndex == cMemoryStackBottomHeap || frame.mpFrame >= mpFrame[cMemoryStackTopHeap]);
   //-- do a real if check for release mode so we don't corrupt the stack in release mode when
   //-- asserts go away.
   if ((frame.mHeapIndex == cMemoryStackTopHeap) && (frame.mpFrame < mpFrame[cMemoryStackTopHeap]))
   {
      //-- If this happens someone is using the memory stack incorrectly
      BASSERT(0);
      return;
   }

	if (frame.mHeapIndex<0 || frame.mHeapIndex>=cNumberHeaps)
	{
		//-- If this happens someone is using the memory stack incorrectly
		BASSERT(0);
		return;
	}

   //-- ok we are good available memory frame pointer back
   //-- to the spot that we want to release
   mpFrame[frame.mHeapIndex] = frame.mpFrame;
}

//============================================================================
//============================================================================
DWORD BMemoryStack::alignHeap(DWORD address, DWORD bytes)
{
   DWORD temp = address;
   temp += (bytes - 1);
   temp &= (~ (bytes - 1));   
   return temp;
}
//============================================================================
// BMemoryStack::validateIntegrity()
//============================================================================
bool BMemoryStack::validateIntegrity()
{
   //-- make sure that the top frame pointer is not pointed outside our
   //-- allocated memory.
   if (mpFrame[cMemoryStackTopHeap] > (mpMemoryBlock + (sizeof(BYTE)*mSize)))
      return false;

   //-- make sure that the top frame Pointer is larger or equal to the
   //-- memory block pointer
   if (mpFrame[cMemoryStackTopHeap] < mpMemoryBlock)
      return false;

   //-- make sure that the bottom frame pointer is larger or equal to the 
   //-- memory block pointer
   if (mpFrame[cMemoryStackBottomHeap] < mpMemoryBlock)
      return false;

   //-- make sure that the bottom heap never exceeds our memory block dimensions
   if (mpFrame[cMemoryStackBottomHeap] > (mpMemoryBlock + (sizeof(BYTE)*mSize)))
      return false;
   
   //-- validate that the heap pointers are valid
   if (mpHeap[cMemoryStackBottomHeap] != mpMemoryBlock)
      return false;

   if (mpHeap[cMemoryStackTopHeap] != (mpMemoryBlock + (sizeof(BYTE)*mSize)))
      return false;

   return true;
}

//============================================================================
// BMemoryStack::isEmpty()
// validates that the memory stack is empty.
//============================================================================
bool BMemoryStack::isEmpty()
{
   if (mpHeap[cMemoryStackTopHeap] != mpFrame[cMemoryStackTopHeap])
      return false;

   if (mpHeap[cMemoryStackBottomHeap] != mpFrame[cMemoryStackBottomHeap])
      return false;

   return true;
}
#endif

