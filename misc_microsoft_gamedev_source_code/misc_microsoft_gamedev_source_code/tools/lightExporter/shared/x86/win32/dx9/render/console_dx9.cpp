//-----------------------------------------------------------------------------
// File: console_dx9.cpp
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include "console_dx9.h"

namespace gr
{
	DX9Console gDX9ConsoleObject;
	DX9Console& gDX9Console = gDX9ConsoleObject;
	Console& gConsole = gDX9ConsoleObject;
} // namespace gr