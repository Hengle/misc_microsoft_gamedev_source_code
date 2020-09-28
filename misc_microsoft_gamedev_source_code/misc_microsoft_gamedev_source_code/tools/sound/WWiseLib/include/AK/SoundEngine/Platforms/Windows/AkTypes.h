//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2008 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

// AkTypes.h

/// \file 
/// Data type definitions.

#ifndef _AK_DATA_TYPES_PLATFORM_H_
#define _AK_DATA_TYPES_PLATFORM_H_

#include <windows.h>

#define AK_LFECENTER							///< Internal use

#define AK_RESTRICT		__restrict				///< Refers to the __restrict compilation flag available on some platforms
#define AkForceInline	__forceinline			///< Force inlining
#define AkNoInline		__declspec(noinline)	///< Disable inlining

#define AK_ALIGNED_16							///< Platform-specific data alignment on 16
#define AK_ALIGNED_128							///< Platform-specific data alignment on 128

typedef UINT8				AkUInt8;			///< Unsigned 8-bit integer
typedef UINT16				AkUInt16;			///< Unsigned 16-bit integer
typedef unsigned long		AkUInt32;			///< Unsigned 32-bit integer
typedef unsigned __int64	AkUInt64;			///< Unsigned 64-bit integer

typedef INT8			AkInt8;					///< Signed 8-bit integer
typedef INT16			AkInt16;				///< Signed 16-bit integer
typedef long   			AkInt32;				///< Signed 32-bit integer
typedef int 			AkInt;					///< Signed integer
typedef __int64			AkInt64;				///< Signed 64-bit integer

typedef wchar_t         AkTChar;				///< Generic character
typedef LPCWSTR			AkLpCtstr;				///< Generic wide character string
typedef LPCSTR			AkLpCstr;				///< Generic character string

typedef float			AkReal32;				///< 32-bit floating point
typedef double          AkReal64;				///< 64-bit floating point

typedef GUID			AkGuid;					///< Globally Unique ID
typedef CHAR			AkChar;					///< Character

typedef HANDLE					AkThread;		///< Thread handle
typedef LPTHREAD_START_ROUTINE  AkThreadRoutine;///< Thread routine
typedef HANDLE					AkEvent;		///< Event handle
typedef HANDLE					AkFileHandle;	///< File handle

// For strings.
#define AK_MAX_PATH     (MAX_PATH)				///< Maximum path length.

typedef DWORD			AkFourcc;				///< Riff chunk
/// Create Riff chunk
#define AkmmioFOURCC( ch0, ch1, ch2, ch3 )									    \
		( (AkFourcc)(AkUInt8)(ch0) | ( (AkFourcc)(AkUInt8)(ch1) << 8 ) |		\
		( (AkFourcc)(AkUInt8)(ch2) << 16 ) | ( (AkFourcc)(AkUInt8)(ch3) << 24 ) )

#define AK_BANK_PLATFORM_DATA_ALIGNMENT	(16)	///< Required memory alignment for bank loading by memory address (see LoadBank())

#endif //_AK_DATA_TYPES_PLATFORM_H_

