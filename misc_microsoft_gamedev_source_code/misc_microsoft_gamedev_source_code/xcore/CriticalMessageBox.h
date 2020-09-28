//============================================================================
//
//  CriticalMessageBox.h
//
// Copyright (c) 2000-2006, Ensemble Studios
//
//============================================================================


#ifndef __CRITICAL_MESSAGE_BOX_H__
#define __CRITICAL_MESSAGE_BOX_H__


//----------------------------------------------------------------------------
//  Public Constants
//----------------------------------------------------------------------------
const long CRITICAL_MESSAGE_BOX_FAILED = -1;


//----------------------------------------------------------------------------
//  Public Interface
//----------------------------------------------------------------------------
long criticalMessageBox(const WCHAR* pMessage, const WCHAR* pTitle, HINSTANCE hInstance, HWND hParentWnd, long numButtons = 0, const WCHAR** pButtonText = NULL);


#endif

