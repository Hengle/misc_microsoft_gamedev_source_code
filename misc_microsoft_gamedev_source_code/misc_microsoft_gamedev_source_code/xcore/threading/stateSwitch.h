//============================================================================
//
// File: stateSwitch.h
// Copyright (c) 2005-2006, Ensemble Studios
//
// rg [2/16/06] - Simple Win32 Fiber API Wrapper
//============================================================================
#pragma once

typedef void BInitFuncPtr(void* initFuncData);

const int cDefaultFiberStackCommitSize = 32768;
const int cDefaultFiberStackReservedSize = 2*65536;

class BStateSwitcher
{
public:
   BStateSwitcher();
   
   // Called by the main fiber to create a fiber.
   // The first time this function is called, the current thread will be switched to a fiber.
   BStateSwitcher(
      BInitFuncPtr pInitFunc, 
      void* initFuncData = 0, 
      bool preserveFPState = false,
      int stackCommitSize = cDefaultFiberStackCommitSize, 
      int stackReserveSize = cDefaultFiberStackReservedSize);
   
   // The destructor must be called from the main fiber!   
   // calls deinit();
   ~BStateSwitcher();

   // Called by the main fiber to create a fiber.
   // The first time this function is called, the current thread will be switched to a fiber.
   bool init(
      BInitFuncPtr pInitFunc, 
      void* initFuncData = 0, 
      bool preserveFPState = false,
      int stackCommitSize = cDefaultFiberStackCommitSize, 
      int stackReserveSize = cDefaultFiberStackReservedSize);
   
   // Called by the main fiber to begin or continue fiber execution.
   int begin(void);
   
   // Called by the fiber to set the status that will be returned to the main fiber on return from begin().
   inline void setReturnStatus(int status) { mStatus = status; }
   
   // Called by the fiber to (possibly temporarily) return to the main fiber.
   // The main fiber may call begin() to resume execution of the fiber.
   void ret(void);
   
   // Called by the main fiber to destroy a fiber.
   // Do not call from the worker fiber!
   void deinit(void);
   
   void* getFiber(void) const { return mpFiber; }
   void* getPrimaryFiber(void) const { return mpPrimaryFiber; }
   
private:
   BInitFuncPtr   mpInitFunc;
   void*          mInitFuncData;
   int            mStatus;

   void*          mpFiber;
   void*          mpPrimaryFiber;
};

class BFiber
{
public:
   // Any methods that start with fiber are only callable from the fiber, and vice versa.
   
   enum eState 
   {
      eInvalid,
      eReady,
      eActive,
      eFinished
   };
   
   BFiber() :
      mStateSwitcher(),
      mFiberState(eInvalid)
   {
   }
   
   virtual ~BFiber()
   {
      deinitFiber();
   }
   
   void initFiber(void)
   {
      BASSERT(mFiberState == eInvalid);
      
      if (mFiberState == eInvalid)
      {
         if (!mStateSwitcher.init(fiberInitFunc, this, true))
         {
            BFATAL_FAIL("BFiber::initFiber: state switcher init failed");
         }
         mFiberState = eReady;
      }         
   }
   
   void deinitFiber(void)
   {
      if (mFiberState != eInvalid)
      {
         BASSERT(mFiberState != eActive);
         
         mStateSwitcher.deinit();
         
         mFiberState = eInvalid;
      }
   }
   
   virtual int activateFiber(uint64 privateData = 0)
   {
      BASSERT(GetCurrentFiber() == mStateSwitcher.getPrimaryFiber());
      BASSERT((mFiberState != eInvalid) && (mFiberState != eFinished));
      
      mFiberPrivateData = privateData;
      
      return mStateSwitcher.begin();
   }
   
   eState getFiberState(void) const { return mFiberState; }

protected:   
   BStateSwitcher mStateSwitcher;
   eState mFiberState;
   uint64 mFiberPrivateData;
      
   static void fiberInitFunc(void* pData)
   {
      BFiber* pFiber = reinterpret_cast<BFiber*>(pData);
      
      pFiber->mFiberState = eActive;
      
      const int status = pFiber->fiberMain(pFiber->mFiberPrivateData);
      
      pFiber->mStateSwitcher.setReturnStatus(status);
            
      pFiber->mFiberState = eFinished;
      
      pFiber->mStateSwitcher.ret();   
      
      BFATAL_FAIL("fiberInitFunc: Can't activate a finished fiber");
   }

   virtual void fiberReturn(int status)
   {
      BDEBUG_ASSERT(GetCurrentFiber() == mStateSwitcher.getFiber());
      
      mStateSwitcher.setReturnStatus(status);
      
      mStateSwitcher.ret();
   }

   virtual int fiberMain(uint64 privateData) = 0;
};
