// File: state_switch.cpp
// Simple Win32 Fiber API Wrapper
#include "common.h"

#define WINVER 0x0400
#define _WIN32_WINNT 0x0500
#define _WIN32_WINDOWS 0x0410
#include <windows.h>

#include "state_switch.h"

namespace gr
{
   static void* gpPrimaryFiber;

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
                     
      assert(pInitFunc);
      assert(stackCommitSize >= 4096);
      assert(stackReserveSize >= 4096);
      assert(stackCommitSize <= stackReserveSize);
      
      if (!gpPrimaryFiber)
         gpPrimaryFiber = ConvertThreadToFiber(NULL);
      
      const DWORD flags = preserveFPState ? FIBER_FLAG_FLOAT_SWITCH : 0;
                        
      mpFiber = CreateFiberEx(stackCommitSize, stackReserveSize, flags, (LPFIBER_START_ROUTINE)pInitFunc, initFuncData);
      
      if (!mpFiber)
         return false;
      return true;
   }

   int BStateSwitcher::begin(void)
   {
      assert(mpFiber);
      SwitchToFiber(mpFiber);
      return mStatus;
   }
   
   void BStateSwitcher::ret(void)
   {
      assert(mpFiber);
      SwitchToFiber(gpPrimaryFiber);
   }

   void BStateSwitcher::deinit(void)
   { 
      if (mpFiber)
      {
         DeleteFiber(mpFiber);
         mpFiber = NULL;
      }
   }
} // namespace gr



















