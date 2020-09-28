//==============================================================================
// gamecallbacks.h
//
// Copyright (c) 2005-2007 Ensemble Studios
//==============================================================================
#pragma once

//==============================================================================
// Forward declarations
//==============================================================================
class BXSSyscallModule;

//==============================================================================
// ECore callbacks
//==============================================================================
void gamePreAssert( const char* expression, const char* msg, const char* file, long line, bool fatal, bool noreturn, const BDebugCallstack* pCallstack, void* param1, void* param2, const char* cpFullAssertError );
void gamePostAssert( const char* expression, const char* msg, const char* file, long line, bool fatal, bool noreturn, const BDebugCallstack* pCallstack, void* param1, void* param2, const char* cpFullAssertError );
long CALLBACK memoryMessageFunc(const BCHAR_T* pMessage, const BCHAR_T* pTitle, long numButtons, const BCHAR_T** pButtonText, void* pParam);

//==============================================================================
// XInputsystem callbacks
//==============================================================================
bool registerConsoleFuncs(BXSSyscallModule* sm);