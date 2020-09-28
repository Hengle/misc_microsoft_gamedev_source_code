//	XeCR.h : Xenon Controller Recorder title include file.
//		Include this file from either your global include file or each file that
//		calls XTL input functions to add XeCR to your title.
//
//	Created 2004/09/21 Rich Bonny <rbonny@microsoft.com>
//	Based on XCR.h of XCR2 (Xbox Controller Recorder 2)
//  Most macros and namespaces retain references to XCR rather than XeCR to
//  maximize the commonality between implementations.
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2004 Microsoft Corp.  All rights reserved.


// Note: You MUST call all functions/macros in this file from the SAME THREAD.
// They are not designed to run concurrently.



#pragma once

#ifdef _XCR_

#ifndef _XENON_UTILITY_
#define _XENON_UTILITY_
#endif

#include "xecr_Core.h"

// driem - removed these messages as it spams an otherwise rather clean-building project
//#pragma message(" ")
//#pragma message("**This file was compiled with Xenon Controller Recorder support.**")
//#pragma message(" ")



namespace XCR
{
	extern XCR::ControlUnit::Core xcr;
}


// Call this function to tell the XeCR about states reached in the game.
// For example, if you have a main menu, options menu, and in-game states,
// call this function when each of those states begins.
// The state parameter is a string uniquely identifying the state.
// This function is NOT multithreading-safe.
#define XCRNotifyState(state) (XCR::xcr.NotifyState((state)))

// Set the signal callback processor.
#define XCRSetSignalCallback(signalCallback, signalData) (XCR::xcr.SetSignalCallback((signalCallback), (signalData)))

// State serialization:
#define XCRSaveState(file) (XCR::xcr.SaveStateWrapper((file)))
#define XCRResumeState(file) (XCR::xcr.ResumeStateWrapper((file)))

// Logging exposure bug fix:
#define XCRLogMessage(message)(XCR::xcr.LogMessage((message)))

#undef XInputGetCapabilities
#define XInputGetCapabilities (XCR::xcr.InputGetCapabilities)

#undef XInputGetState
#define XInputGetState (XCR::xcr.InputGetState)

#undef XInputSetState
#define XInputSetState (XCR::xcr.InputSetState)

#undef XInputGetKeystroke
#define XInputGetKeystroke (XCR::xcr.InputGetKeystroke)

#else // _XCR_

// No XCR, so no state notification.
#define XCRNotifyState(state) 

//JKBNEW: state serialization:
#define XCRSaveState(file)
#define XCRResumeState(file)

//JKBNEW: logging exposure bug fix:
#define XCRLogMessage(message)

#endif // _XCR_