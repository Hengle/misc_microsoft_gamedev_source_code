//==============================================================================
// Thread.h
//
// Copyright (c) 2001, Ensemble Studios
//==============================================================================

#ifndef _Thread_H_
#define _Thread_H_

//==============================================================================
// Includes
//

struct BThreadID
{
   BThreadID() : mID( 0 ), mh( 0 )  {}

   DWORD  mID;
   HANDLE mh;
};

//==============================================================================
//
//==============================================================================
typedef struct tagTHREADNAME_INFO
{
	DWORD             dwType;          // must be 0x1000
	const char        *szName;       // pointer to name -- limited to 9 characters
	DWORD             dwThreadID;    // thread ID ( -1 = caller thread)
   DWORD             dwFlags;       // reserved for future use; must be zero;
} THREADNAME_INFO;

void     SetThreadName(DWORD dwThread, const char *szThreadName, DWORD dwFlags);

//==============================================================================
// Thread wrapper class
//
class BThread
{   
   public:

      BThread();

      typedef void* (__cdecl *ThreadFunc)( void* );

      bool createThread( ThreadFunc pFunc, void* pArg = 0, unsigned stackSize = 0, bool detached = true );

      bool setPriority(long priority);
      void setThreadProcessor(DWORD processor); 

      bool attach( const BThreadID& threadID );

      bool suspend  ();
      bool resume   ();

      bool waitForThread( DWORD timeout = INFINITE );

      const BThreadID& getTID() const  { return mThreadID; }

      operator const BThreadID&() const  { return getTID(); }

   protected:

      BThreadID mThreadID;

   private:

      // Hidden to deny access
      //
      BThread( const BThread& other );
      BThread& operator=( const BThread& rhs );
};

//==============================================================================
#endif // _Thread_H_

//==============================================================================
// eof: Thread.h
