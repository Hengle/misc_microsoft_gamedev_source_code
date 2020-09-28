//-----------------------------------------------------------------------------
// File: timer_x86.cpp
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "timer_x86.h"

namespace gr
{
	namespace Time
	{
		// RG HACK HACK
		const double CPUCyclesPerSecond = 1666666666.667;
		const double OOCPUCyclesPerSecond = 1.0 / 1666666666.667;
		
		__declspec(naked) uint32 ReadCycleCounter(void)
		{ 
			_asm 
			{
				Push ebx
				Mov eax, 1     
				Cpuid          
				Pop ebx

				// Rdtsc
				_emit 0fh
				_emit 031h
				Ret
			}
		}
						
		double CyclesToSeconds(uint32 cycles)
		{
			return cycles * OOCPUCyclesPerSecond;
		}
	} // namespace Time
} // namespace gr

