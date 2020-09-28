// File: memoryHeapAnalyzer.h
#pragma once

#if defined(XBOX) && !defined(BUILD_FINAL)

class BMemoryHeapAnalyzer
{
   BMemoryHeapAnalyzer(const BMemoryHeapAnalyzer&);
   BMemoryHeapAnalyzer& operator= (const BMemoryHeapAnalyzer&);
   
public:
   BMemoryHeapAnalyzer();
   
   bool begin(BMemoryHeap* pHeap);
   bool end(void);
   
   bool isAnalyzing() const { return mBegun; }

private:
   BMemoryHeap*            mpHeap;
   ROCKALL_FRONT_END*      mpRockall;
   
   BDynamicCHeapArray<BYTE> mPageMap;
   BDynamicCHeapArray<BYTE> mPageRead;
   BDynamicCHeapArray<BYTE> mPageWritten;
   BDynamicCHeapArray<BYTE> mPageFailed;
   
   BLightWeightMutex       mMutex;
   
   bool                    mBegun : 1;

   void createResults(uint numPagesFailed);
   
   static void setBit(BDynamicCHeapArray<BYTE>& bytes, uint bitIndex, bool value);
   static bool isBitSet(BDynamicCHeapArray<BYTE>& bytes, uint bitIndex);
   static int exceptionFilterFuncPtr(_EXCEPTION_POINTERS* pExcept, DWORD data);
};

#endif

