//============================================================================
//
//  File: globalObjects.h
//  
// Copyright (c) 1999-2006, Ensemble Studios
//
//============================================================================
#pragma once

class BInitialKernelMemoryTracker
{
   DWORD mInitialPhysicalFree;
   
public:
   BInitialKernelMemoryTracker();
   
   DWORD getInitialPhysicalFree(void) const { return mInitialPhysicalFree; }
};

extern BInitialKernelMemoryTracker gInitialKernelMemoryTracker;