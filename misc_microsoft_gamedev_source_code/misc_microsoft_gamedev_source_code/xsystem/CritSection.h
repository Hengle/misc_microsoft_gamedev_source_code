//==============================================================================
// CritSection.h
//
// Copyright (c) 2001, Ensemble Studios
//==============================================================================

#ifndef _CritSection_H_
#define _CritSection_H_

//==============================================================================
// A wrapper for a Win32 CRITICAL_SECTION.
//
class BCritSection
{
   public:

      class Lock
      {
         public:

            Lock( BCritSection& cs ) : mcs( cs )  { mcs.enter(); }
            ~Lock()  { mcs.leave(); }

         private:

            BCritSection& mcs;

            Lock( const Lock& other );
            Lock& operator=( const Lock& rhs );

      };

      BCritSection() : mIsIn( false )  
      { 
#ifdef XBOX
         InitializeCriticalSection(&mcs); 
#else      
         ::InitializeCriticalSection(&mcs); 
#endif         
      }
      
      ~BCritSection()  
      { 
#ifdef XBOX      
         DeleteCriticalSection(&mcs); 
#else
         ::DeleteCriticalSection(&mcs); 
#endif         
      }  

      void enter()  { ::EnterCriticalSection( &mcs ); mIsIn = true; }
      void leave()  { mIsIn = false; ::LeaveCriticalSection( &mcs ); }

      bool isEntered()  { return mIsIn; }

   private:

      CRITICAL_SECTION mcs;
      bool             mIsIn;

      // Hidden to deny access
      //
      BCritSection( const BCritSection& other );
      BCritSection& operator=( const BCritSection& rhs );
};

//==============================================================================
#endif // _CritSection_H_

//==============================================================================
// eof: CritSection.h
