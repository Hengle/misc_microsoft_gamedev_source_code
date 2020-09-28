//-----------------------------------------------------------------------------
// File: timer_x86.h
// x86 timer helpers.
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once
#ifndef TIMER_X86_H
#define TIMER_X86_H

#include "common/core/core.h"

namespace gr
{
	namespace Time
	{
		uint32 ReadCycleCounter(void);
		double CyclesToSeconds(uint32 cycles);
	} // namespace Time

} // namespace gr

#endif // TIMER_X86_H
