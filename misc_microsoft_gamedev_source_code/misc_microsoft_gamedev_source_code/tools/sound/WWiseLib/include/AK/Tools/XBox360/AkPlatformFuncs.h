//////////////////////////////////////////////////////////////////////
//
// AkPlatformFuncs.h
//
// Audiokinetic platform-dependent functions definition.
//
// Copyright 2006-2008 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#ifndef _AK_PLATFORM_FUNCS_H_
#define _AK_PLATFORM_FUNCS_H_

#include "malloc.h"
#include <AK/Tools/Common/AkAssert.h>
#include <AK/SoundEngine/Common/AkTypes.h>

//////////////////////////////////////////////////////////////////////
// XBOX360
//////////////////////////////////////////////////////////////////////

#include <xtl.h>

//-----------------------------------------------------------------------------
// External variables.
//-----------------------------------------------------------------------------
// g_fFreqRatio is used by time helpers to return time values in milliseconds.
// It is declared and updated by the sound engine.
namespace AK
{
    extern AkReal32 g_fFreqRatio;
}

//-----------------------------------------------------------------------------
// Defines for XBox360.
//-----------------------------------------------------------------------------
#define AK_DECLARE_THREAD_ROUTINE( FuncName )   DWORD WINAPI FuncName(LPVOID lpParameter)
#define AK_THREAD_RETURN( _param_ )				return (_param_)
#define AK_THREAD_ROUTINE_PARAMETER             lpParameter
#define AK_RETURN_THREAD_OK                     0x00000000
#define AK_RETURN_THREAD_ERROR                  0x00000001
#define AK_DEFAULT_STACK_SIZE                   (8192)
#define AK_THREAD_PRIORITY_NORMAL				THREAD_PRIORITY_NORMAL
#define AK_THREAD_PRIORITY_ABOVE_NORMAL			THREAD_PRIORITY_ABOVE_NORMAL

// NULL objects
#define AK_NULL_THREAD                          NULL

#define AK_INFINITE                             INFINITE

#define AkMakeLong(a,b)							MAKELONG((a),(b))

#define AK_XBOX360_DEFAULT_PROCESSOR_ID			(4)		// Processor core#2, HW thread#0

#define AkMax(x1, x2)	(((x1) > (x2))? (x1): (x2))
#define AkMin(x1, x2)	(((x1) < (x2))? (x1): (x2))

namespace AKPLATFORM
{
	// Simple automatic event API
    // ------------------------------------------------------------------
	
	/// Platform Independent Helper
	inline void AkClearEvent( AkEvent & out_event )
    {
		out_event = NULL;
	}

	/// Platform Independent Helper
	inline AKRESULT AkCreateEvent( AkEvent & out_event )
    {
        out_event = ::CreateEvent( NULL,					// No security attributes
                                    false,					// Reset type: automatic
                                    false,					// Initial signaled state: not signaled
                                    NULL                    // No name
                                   );
		return ( out_event ) ? AK_Success : AK_Fail;
	}

	/// Platform Independent Helper
	inline void AkDestroyEvent( AkEvent & io_event )
	{
		if ( io_event )
			::CloseHandle( io_event );
		io_event = NULL;
	}

	/// Platform Independent Helper
	inline void AkWaitForEvent( AkEvent & in_event )
	{
        AKVERIFY( ::WaitForSingleObject( in_event, INFINITE ) == WAIT_OBJECT_0 );
	}

	/// Platform Independent Helper
	inline void AkSignalEvent( const AkEvent & in_event )
	{
		AKVERIFY( ::SetEvent( in_event ) );
	}


    // Atomic Operations
    // ------------------------------------------------------------------

	/// Platform Independent Helper
	inline AkInt32 AkInterlockedIncrement( AkInt32 * pValue )
	{
		return InterlockedIncrement( pValue );
	}

	/// Platform Independent Helper
	inline AkInt32 AkInterlockedDecrement( AkInt32 * pValue )
	{
		return InterlockedDecrement( pValue );
	}

    // Threads
    // ------------------------------------------------------------------

	/// Platform Independent Helper
	inline bool AkIsValidThread( AkThread * in_pThread )
	{
		return (*in_pThread != AK_NULL_THREAD);
	}

	/// Platform Independent Helper
	inline void AkClearThread( AkThread * in_pThread )
	{
		*in_pThread = AK_NULL_THREAD;
	}

	/// Platform Independent Helper
    inline void AkCloseThread( AkThread * in_pThread )
    {
		AKASSERT( in_pThread );
        AKASSERT( *in_pThread );
        AKVERIFY( ::CloseHandle( *in_pThread ) );
		AkClearThread( in_pThread );
    }

#define AkExitThread( _result ) return _result;

	/// Platform Independent Helper
	inline void AkGetDefaultThreadProperties( AkThreadProperties & out_threadProperties )
	{
		out_threadProperties.nPriority = AK_THREAD_PRIORITY_NORMAL;
		out_threadProperties.uStackSize= AK_DEFAULT_STACK_SIZE;
		out_threadProperties.dwProcessor = AK_XBOX360_DEFAULT_PROCESSOR_ID;
	}

	/// Platform Independent Helper
	inline void AkCreateThread( AkThreadRoutine pStartRoutine,				// Thread routine.
                                void * pParams,								// Routine params.
								AkThreadProperties * in_pThreadProperties,	// Properties. NULL for default.
                                AkThread * out_pThread,						// Returned thread handle.
								char * in_szThreadName )					// Opt thread name.
    {
        AKASSERT( out_pThread != NULL );

		AkThreadProperties threadProperties;
		if ( in_pThreadProperties )
			threadProperties = *in_pThreadProperties;
		else
			AKPLATFORM::AkGetDefaultThreadProperties( threadProperties );
		AKASSERT( threadProperties.nPriority >= THREAD_PRIORITY_LOWEST && threadProperties.nPriority <= THREAD_PRIORITY_HIGHEST );
		
        // Create thread suspended, set its processor.
        *out_pThread = ::CreateThread( NULL,							// No security attributes
                                       threadProperties.uStackSize,		// StackSize (0 uses system default)
                                       pStartRoutine,                   // Thread start routine
                                       pParams,                         // Thread function parameter
                                       CREATE_SUSPENDED,				// Creation flags: create suspended, resume below.
                                       NULL );							// No name
        // ::CreateThread() return NULL if it fails.
		if ( !*out_pThread )
        {
			AkClearThread( out_pThread );
            return;
        }

        // Set thread processor.
        DWORD dwPrevProcessor = ::XSetThreadProcessor( *out_pThread,
                                                       threadProperties.dwProcessor );
        if ( dwPrevProcessor == -1 )
        {
            AKASSERT( !"Could not set thread's processor" );
            AkCloseThread( out_pThread );
            return;
        }

		// Set priority.
        if ( !::SetThreadPriority( *out_pThread, threadProperties.nPriority ) )
        {
            AKASSERT( !"Failed setting IO thread priority" );
			AkCloseThread( out_pThread );
            return;
        }

        // Resume thread.
        if ( ::ResumeThread( *out_pThread ) == -1 )
        {
            AKASSERT( !"Could not resume thread" );
            AkCloseThread( out_pThread );
        }
    }

	/// Platform Independent Helper
    inline void AkWaitForSingleThread( AkThread * in_pThread )
    {
        AKASSERT( in_pThread );
        AKASSERT( *in_pThread );
        ::WaitForSingleObject( *in_pThread, INFINITE );
    }

	/// Platform Independent Helper
    inline void AkSleep( AkUInt32 in_ulMilliseconds )
    {
        ::Sleep( in_ulMilliseconds );
    }


	// Optimized memory functions
	// --------------------------------------------------------------------

	/// Platform Independent Helper
	inline void AkMemCpy( void * pDest, void * pSrc, AkUInt32 uSize )
	{
		XMemCpy( pDest, pSrc, uSize );
	}

	/// Platform Independent Helper
	inline void AkMemSet( void * pDest, AkInt32 iVal, AkUInt32 uSize )
	{
		XMemSet( pDest, iVal, uSize );
	}

	/// Platform Independent Helper
	inline bool CheckAlignedPtr(void* in_vPtr,AkUInt32 in_ZeroBits)
	{
		return ((long long)in_vPtr & (long long)in_ZeroBits) == 0;
	}

    // Time functions
    // ------------------------------------------------------------------

	/// Platform Independent Helper
    inline void PerformanceCounter( AkInt64 * out_piLastTime )
	{
		::QueryPerformanceCounter( (LARGE_INTEGER*)out_piLastTime );
	}

	/// Platform Independent Helper
	inline void PerformanceFrequency( AkInt64 * out_piFreq )
	{
		::QueryPerformanceFrequency( (LARGE_INTEGER*)out_piFreq );
	}

	/// Platform Independent Helper
    inline void UpdatePerformanceFrequency()
	{
        AkInt64 iFreq;
        PerformanceFrequency( &iFreq );
        AK::g_fFreqRatio = (AkReal32)( iFreq / 1000 );
	}

	/// Platform Independent Helper
    /// Returns a time range in milliseconds, using the sound engine's updated count->milliseconds ratio.
    inline AkReal32 Elapsed( const AkInt64 & in_iNow, const AkInt64 & in_iStart )
    {
        return ( in_iNow - in_iStart ) / AK::g_fFreqRatio;
    }
//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
	/// Platform Independent Helper
	inline AkInt32 AkWideCharToAnsi(AkLpCtstr	in_pszUnicodeString,
									AkUInt32		in_uiOutBufferSize,
									AkChar*		io_pszAnsiString )
	{
		AKASSERT( io_pszAnsiString != NULL );

		AkInt32 ConvRet = ::WideCharToMultiByte(CP_ACP,																	// code page
												0,																		// performance and mapping flags
												in_pszUnicodeString,													// wide-character string
												AkMin( (AkInt)( wcslen( in_pszUnicodeString ) + 1), in_uiOutBufferSize ),	// number of chars in string : -1 = NULL terminated string.
												io_pszAnsiString,														// buffer for new string
												in_uiOutBufferSize,														// size of buffer
												NULL,																	// default for unmappable chars
												NULL);																	// set when default char used
#ifdef _DEBUG
		if ( ConvRet <= 0 )
		{
			AKASSERT( !"Could not convert file name string to ANSI" );
		}
#endif
		return ConvRet;
	}
//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
	/// Platform Independent Helper
	inline AkInt32 AkAnsiToWideChar(const AkChar*	in_pszAnsiString,
									AkUInt32			in_uiOutBufferSize,
									void*			io_pvUnicodeStringBuffer )
	{
		AKASSERT( io_pvUnicodeStringBuffer != NULL );

		AkInt32 ConvRet = ::MultiByteToWideChar(	CP_ACP,							// code page
											0,									// performance and mapping flags
											(LPCSTR)in_pszAnsiString,			// wide-character string
											-1,									// number of chars in string : -1 = NULL terminated string.
											(AkTChar*)io_pvUnicodeStringBuffer,	// buffer for new string
											in_uiOutBufferSize);				// size of buffer
#ifdef _DEBUG
		if ( ConvRet <= 0 )
		{
			AKASSERT( !"Could not convert file name string to wide char" );
		}
#endif
		return ConvRet;
	}
}

#endif  // _AK_PLATFORM_FUNCS_H_