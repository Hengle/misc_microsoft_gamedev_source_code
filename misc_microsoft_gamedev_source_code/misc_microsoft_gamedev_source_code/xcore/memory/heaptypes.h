// File: heapTypes.h
#pragma once

enum eHeapType
{  
   cInvalidHeapType = -1,
   
   cCRunTimeHeap,
   cDLMallocHeap,
   
   // Rockall heap types
   cDebugHeap,
   cFastHeap,
   cSmallHeap, 
   cBlendedHeap,
   cPhysicalHeap,
   
   // Intended for testing only.
   cSingleRockallHeap,
         
   cPrimaryHeap,
   cRenderHeap,
   
   cNumHeaps
};