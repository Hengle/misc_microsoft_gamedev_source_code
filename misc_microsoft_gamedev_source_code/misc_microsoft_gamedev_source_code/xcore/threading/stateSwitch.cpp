//============================================================================
//
// File: stateSwitch.cpp
// Copyright (c) 2005-2006, Ensemble Studios
//
//============================================================================
#include "xcore.h"
#include "stateSwitch.h"

BStateSwitcher::BStateSwitcher()
{
   memset(this, 0, sizeof(*this));
}

BStateSwitcher::BStateSwitcher(
   BInitFuncPtr pInitFunc, 
   void* initFuncData, 
   bool preserveFPState,
   int stackCommitSize, 
   int stackReserveSize)
{
   memset(this, 0, sizeof(*this));
   
   init(pInitFunc, initFuncData, preserveFPState, stackCommitSize, stackReserveSize);
}

BStateSwitcher::~BStateSwitcher()
{
   deinit();
}
         
bool BStateSwitcher::init(BInitFuncPtr pInitFunc, void* initFuncData, bool preserveFPState, int stackCommitSize, int stackReserveSize)
{
   deinit();
                  
   BASSERT(NULL != pInitFunc);
   BASSERT(stackCommitSize >= 4096);
   BASSERT(stackReserveSize >= 4096);
   BASSERT(stackCommitSize <= stackReserveSize);
   
   if (!mpPrimaryFiber)
   {
      mpPrimaryFiber = ConvertThreadToFiber(NULL);
      
      if (NULL == mpPrimaryFiber)
         mpPrimaryFiber = GetCurrentFiber();
      
      if (mpPrimaryFiber == NULL)
         return false;
   }

#ifdef XBOX
   mpFiber = CreateFiber(stackCommitSize, (LPFIBER_START_ROUTINE)pInitFunc, initFuncData);
#else         
   const DWORD flags = preserveFPState ? FIBER_FLAG_FLOAT_SWITCH : 0;
                     
   mpFiber = CreateFiberEx(stackCommitSize, stackReserveSize, flags, (LPFIBER_START_ROUTINE)pInitFunc, initFuncData);
#endif   
   
   if (!mpFiber)
      return false;
   return true;
}

int BStateSwitcher::begin(void)
{
   BASSERT(mpFiber);
   if (mpFiber)
      SwitchToFiber(mpFiber);
   return mStatus;
}

void BStateSwitcher::ret(void)
{
   BASSERT(mpFiber);
   BASSERT(mpPrimaryFiber);
         
   if (mpPrimaryFiber)
      SwitchToFiber(mpPrimaryFiber);
}

void BStateSwitcher::deinit(void)
{ 
   if (mpFiber)
   {
      DeleteFiber(mpFiber);
      mpFiber = NULL;
   }
   
   mpPrimaryFiber = NULL;
}

