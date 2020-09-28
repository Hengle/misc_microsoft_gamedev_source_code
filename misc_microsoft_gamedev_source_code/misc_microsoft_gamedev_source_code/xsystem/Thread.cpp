//==============================================================================
// Thread.cpp
//
// Copyright (c) 2001, Ensemble Studios
//==============================================================================

//==============================================================================
// Includes
//
#include "xsystem.h"
#include <process.h>

#include "Thread.h"
#include "CritSection.h"


//=============================================================================
// SetThreadName
//=============================================================================
void SetThreadName(DWORD dwThread, const char *szThreadName, DWORD dwFlags)
{
   THREADNAME_INFO info;
   info.dwType = 0x1000;
   info.szName = szThreadName;
   info.dwFlags = dwFlags;
   info.dwThreadID = dwThread;

   __try
   {
	   RaiseException (0x406D1388, 0, sizeof(info)/sizeof(DWORD), (DWORD*)&info);
   }
   __except(EXCEPTION_CONTINUE_EXECUTION)
   {
   }

} // SetThreadName

//==============================================================================
//
class ThreadContext
{
   public:
      ThreadContext() : pFunc( 0 ), pArg( 0 ), detached( false )  {}
      ~ThreadContext() 
      {
      }

      BThread::ThreadFunc pFunc;
      void*               pArg;
      BThreadID           threadID;
      BCritSection        cs;
      bool                detached;

   private:
      // Hidden to deny access
      //
      ThreadContext( const ThreadContext& other );
      ThreadContext& operator=( const ThreadContext& rhs );
};

//==============================================================================
//
#pragma warning(push)
#pragma warning(disable:4571) // warning C4571: catch(...) blocks compiled with /EHs do not catch or re-throw Structured Exceptions
#pragma warning(disable:4530) // warning C4571: catch(...) blocks compiled with /EHs do not catch or re-throw Structured Exceptions

static unsigned __stdcall ThreadWrapperFunc( void* pArg )
{
   ThreadContext* pContext = reinterpret_cast< ThreadContext* >( pArg );
   if ( !pContext )
      return 1;  // Should never happen.

   unsigned result = 1;

   pContext->cs.enter();

#if 0
   try
   {
      result = (unsigned)pContext->pFunc( pContext->pArg );
   }
   catch( ... )
   {
      // Eat any exceptions
   }
#endif

   __try
   {
      result = (unsigned)pContext->pFunc( pContext->pArg );
   }
#ifdef XBOX   
   __except(gAssertionSystem.xboxHandleAssertException(GetExceptionInformation()))
#else
   __except(EXCEPTION_EXECUTE_HANDLER)
#endif   
   {
   }
   
   if ( pContext->detached )
      ::CloseHandle( pContext->threadID.mh );

   pContext->cs.leave();

   delete pContext;

   return (unsigned)result;
}
#pragma warning(pop)
// ThreadWrapperFunc

//==============================================================================
//
BThread::BThread()
{
}
// BThread::BThread

//==============================================================================
//
bool BThread::createThread( ThreadFunc pFunc, void* pArg /*= 0*/, unsigned stackSize /*= 0*/, bool detached /*= false*/ )
{
   ThreadContext* pContext = new ThreadContext;
   if ( !pContext )
      return false;

   pContext->cs.enter();

   pContext->pFunc = pFunc;
   pContext->pArg = pArg;
   pContext->detached = detached;

   mThreadID.mh = (HANDLE)::_beginthreadex( 0, stackSize, ThreadWrapperFunc, pContext, 0, (unsigned*)&mThreadID.mID );
   if ( mThreadID.mh == 0 )
   {
      pContext->cs.leave();
      delete pContext;
      return false;
   }

   pContext->threadID = mThreadID;

   pContext->cs.leave();

   return true;
} 
// BThread::createThread

//==============================================================================
//
bool BThread::setPriority(long priority)
{
   BOOL res = SetThreadPriority(mThreadID.mh, priority);
   return (res != FALSE);
}

//==============================================================================
//
void BThread::setThreadProcessor(DWORD processor)
{
   #ifdef XBOX
      XSetThreadProcessor(mThreadID.mh, processor);
   #endif
}

//==============================================================================
//
bool BThread::attach( const BThreadID& threadID )
{
   mThreadID = threadID;

   return true;
}
// BThread::attach

//==============================================================================
//
bool BThread::suspend()
{
   if ( mThreadID.mh == 0 )
      return false;

   return ::SuspendThread( mThreadID.mh ) != -1;
} 
// BThread::suspend

//==============================================================================
//
bool BThread::resume()
{
   if ( mThreadID.mh == 0 )
      return false;

   return ::ResumeThread( mThreadID.mh ) != -1;
} 
// BThread::resume

//==============================================================================
//
bool BThread::waitForThread( DWORD timeout /*= INFINITE*/ )
{
   if ( mThreadID.mh == 0 )
      return false;

   bool result = ::WaitForSingleObject( mThreadID.mh, timeout ) == WAIT_OBJECT_0;

   if ( result )
      ::CloseHandle( mThreadID.mh );

   return result;
} 
// BThread::resume

//==============================================================================
// eof: Thread.cpp
