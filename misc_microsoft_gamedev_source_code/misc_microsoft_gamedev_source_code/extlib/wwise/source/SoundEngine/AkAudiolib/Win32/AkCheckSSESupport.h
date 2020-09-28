/***********************************************************************
  The content of this file includes source code for the sound engine
  portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
  Two Source Code" as defined in the Source Code Addendum attached
  with this file.  Any use of the Level Two Source Code shall be
  subject to the terms and conditions outlined in the Source Code
  Addendum and the End User License Agreement for Wwise(R).

  Version: v2008.2.1  Build: 2821
  Copyright (c) 2006-2008 Audiokinetic Inc.
 ***********************************************************************/

// AkCheckSSESupport.h

#ifndef _AK_SSE_SUPPORT_CHECK_H_
#define _AK_SSE_SUPPORT_CHECK_H_

#ifdef WIN32

#include <excpt.h>
#define CPUID_STD_SSE          0x02000000	// Bit 25 represents processor support for SSE


static bool AkCheckSSESupport()
{
	// Step 1: Detect cpuid instruction support (level 1)
	__try 
	{
		__asm 
		{	
			xor		eax, eax
			cpuid
			test	eax, eax			// largest standard function==0?
			jz     SSENotSupported		// no standard features func
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER) 
	{
		goto SSENotSupported;
	}

	// Step 2: Check processor SSE support flag
	_asm 
	{      
         mov    eax, 1							// CPUID function #1 
         cpuid									// get signature/std feature flgs
         test   edx, CPUID_STD_SSE				// bit 25 indicates SSE support
		 jz		SSENotSupported					// No SSE support
	}

	// Step 3: Check OS SSE support
	__try 
	{
		__asm xorps xmm0, xmm0  // Streaming SIMD Extension
	}
	__except(EXCEPTION_EXECUTE_HANDLER) 
	{
		if (_exception_code() == STATUS_ILLEGAL_INSTRUCTION)
			goto SSENotSupported;
	}

	return true;

SSENotSupported:
	return false;
}

#endif // WIN32
#endif // _AK_SSE_SUPPORT_CHECK_H_