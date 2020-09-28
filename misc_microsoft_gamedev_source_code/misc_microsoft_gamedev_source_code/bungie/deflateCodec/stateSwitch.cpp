//============================================================================
// stateSwitch.cpp
// Copyright (c) 1996-2006, Ensemble Studios
//
// rgeldreich@ensemblestudios.com
//============================================================================
#include "ens.h"
#include "stateSwitch.h"

namespace ens
{

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
         
bool BStateSwitcher::init(BInitFuncPtr pInitFunc, void* initFuncData, bool preserveFPState, int stackCommitSize, int stackReserveSize, void* pPrimaryFiber)
{
   deinit();
                  
   assert(NULL != pInitFunc);
   assert(stackCommitSize >= 4096);
   assert(stackReserveSize >= 4096);
   assert(stackCommitSize <= stackReserveSize);
   
   if (pPrimaryFiber)
      mpPrimaryFiber = pPrimaryFiber;
  
   if (!mpPrimaryFiber)
   {
      // Try to detect if the current thread has already been converted to a fiber, because ConvertThreadToFiber() can only
      // be called one time per thread (it seems to leak when called multiple times).
      mpPrimaryFiber = GetCurrentFiber();
      
      // Hack: GetCurrentFiber() returns 0x1E00 under WinXP if the current thread has yet to be converted to a fiber.
      // But it returns 0 on 360. WTF?
      if (mpPrimaryFiber < (LPVOID)0x10000)
         mpPrimaryFiber = ConvertThreadToFiber(NULL);
      
      if (mpPrimaryFiber == NULL)
         return false;
   }

#ifdef _XBOX
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
   assert(mpFiber);
   if (mpFiber)
      SwitchToFiber(mpFiber);
   return mStatus;
}

void BStateSwitcher::ret(void)
{
   assert(mpFiber);
   assert(mpPrimaryFiber);
         
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

} // namespace ens
