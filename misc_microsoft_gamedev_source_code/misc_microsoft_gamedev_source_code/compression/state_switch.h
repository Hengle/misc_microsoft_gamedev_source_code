// File: state_switch.h
// Simple Win32 Fiber API Wrapper
#ifndef STATE_SWITCH_H
#define STATE_SWITCH_H
#pragma once

namespace gr
{
   typedef void BInitFuncPtr(void* initFuncData);
   
   const int DefaultFiberStackCommitSize = 16384;
   const int DefaultFiberStackReservedSize = 65536;
   
   class BStateSwitcher
   {
   public:
      BStateSwitcher();
      
      // Called by the main program to create a fiber.
      // The first time this function is called, the current thread will be switched to a fiber.
      BStateSwitcher(
         BInitFuncPtr pInitFunc, 
         void* initFuncData = 0, 
         bool preserveFPState = false,
         int stackCommitSize = DefaultFiberStackCommitSize, 
         int stackReserveSize = DefaultFiberStackReservedSize);
      
      // The destructor must be called from the main program!   
      // calls deinit();
      ~BStateSwitcher();
   
      // Called by the main program to create a fiber.
      // The first time this function is called, the current thread will be switched to a fiber.
      bool init(
         BInitFuncPtr pInitFunc, 
         void* initFuncData = 0, 
         bool preserveFPState = false,
         int stackCommitSize = DefaultFiberStackCommitSize, 
         int stackReserveSize = DefaultFiberStackReservedSize);
      
      // Called by the main program to begin or continue fiber execution.
      int begin(void);
      
      // Called by the fiber to set the status that will be returned to the main fiber by StateSwitchBegin().
      inline void setReturnStatus(int status) { mStatus = status; }
      
      // Called by the fiber to (possibly temporarily) return to the main program.
      // The main program may call StateSwitchBegin() to resume execution of the fiber.
      void ret(void);
      
      // Called by the main program to destroy a fiber.
      // Do not call from the worker fiber!
      void deinit(void);
      
   private:
      BInitFuncPtr   mpInitFunc;
      void*          mInitFuncData;
      int            mStatus;

      void*          mpFiber;
   };
   
} // namespace gr

#endif // STATE_SWITCH_H
