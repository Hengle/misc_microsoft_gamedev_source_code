//==============================================================================
// GarbageCollected.h
//
// Copyright (c) 2002-2008, Ensemble Studios
//==============================================================================

#pragma once 

//==============================================================================
class BGarbageCollected
{
   public:
      // Constructors
      BGarbageCollected();

      // Destructors
      virtual ~BGarbageCollected();

      // Functions
      virtual HRESULT dispose();      

      // Variables

   protected:
      bool isDisposing() const { return mDisposing; }

   private:

      // Functions
      static void CALLBACK apcProc(ULONG_PTR dwParam);

      // Variables
      bool mDisposing;

}; // BGarbageCollected
