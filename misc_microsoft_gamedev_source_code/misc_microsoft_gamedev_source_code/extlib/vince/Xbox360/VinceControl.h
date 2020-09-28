//	VinceControl.h : This header file exists as a convenience
//  and a custom version should be created for each project. It
//  permits various compile flags and build options to be set. It also
//  allows Events and Event Categories to be selectively disabled
//  by defining the event macros to null strings. The Event macro
//  names can be cut and pasted from the generated VinceEvents.h file
//  for the project. As examples:
//
//  #define VINCE_EVENT_PlayerKilled(PlayerName, PositionX, PositionY, PositionZ)
//  might disable logging of player kill events, and
//
//  #define VINCE_EVENT_Combat(event, params)
//  might disable all events in the "Combat" Event Set.
//
//  These defines do not have to be placed in this file, but it is a convenient place
//  to manage all such exclusions. The null-defined macros will produce no compiled
//  code in the executable. This file should only be used to exclude code from compilation.
//  It can also be used to enable/disable compilation of the VINCE components in their entirety
//  by specifying either:
//
//  #define _VINCE_            // To Enable
//  #undef _VINCE_             // To Disable
//
//  Such an entry would take precedence over any project preprocessor definition settings.
//
//	Created 2004/04/20 Rich Bonny <rbonny@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2004 Microsoft Corp.  All rights reserved.

// The definition of the VINCE_INCLUDE() macro follows. This is useful if you have additional
// setup code that you do not want to compile when you are excluding an event from compiling.
// To do this, you might specify:
//
// #define VINCE_EVENT_PlayerKilled(PlayerName, PositionX, PositionY, PositionZ)
// #define EXCLUDE_PlayerKilled
//
// You could then wrap your application code with the conditional:
//
// #if VINCE_INCLUDE(PlayerKilled)
//    ...
//    conditional code goes here
//    ...
// #endif
//
// The compilation of the wrapped code would be skipped if either _VINCE_ were undefined,
// or if EXCLUDE_PlayerKilled were defined. If the call to VINCE_EVENT_PlayerKilled() was within
// this block of code, you don't even need the first #define VINCE_EVENT_PlayerKilled(...) above

#define VINCE_INCLUDE(event) defined( _VINCE_ ) && !defined( EXCLUDE_ ## event )

// User defines go here:

// Will use a full set of optional defines to check the various levels of conditional compilation

// First we switch the whole system on and off
// In this sample, we let the main app take care of the definition
//#define _VINCE_
//#undef _VINCE_

// Now we just turn off Surveys
// #define VINCE_SURVEY(question, context)

// Disable individual events at all levels
//#define VINCE_EVENT_Singleton(Message)
//#define VINCE_EVENT_VibrationTestSummary(TestDuration, MaxLeftMotorSpeed, MaxRightMotorSpeed)
//#define VINCE_EVENT_EnterPage(Page)

// Disable event sets
//#define VINCE_EVENT_Pages_PagesEntered(event, params)
//#define VINCE_EVENT_Controller(event, params)

// Exclude Flag
//#define EXCLUDE_TestFlag

// Retail Flag
//#define VINCE_RETAIL

// The following set of flags enables selective inclusion or exclusion
// of VINCE code segments. This may prove useful for reducing image sizes
// by omitting compilation of unneeded functionality. It also can eliminate
// dependencies on particular libraries (xonlinep.lib and zlib.lib)

// LSP flag - Allows VINCE to use LSP file uploads
//#define VINCE_LSP

// XUI flag - allows XUI to be used for surveys
//#define VINCE_XUI

// NO_VINCE_COMPRESSION flag - define to save image space and disable linking to zlib
//#define NO_VINCE_COMPRESSION

// NO_VINCE_BACKGROUNDS flag - define to save image space and disable survey background images
//#define NO_VINCE_BACKGROUNDS

// NO_VINCE_SURVEYS flag - define to save image space and disable on-screen surveys
//#define NO_VINCE_SURVEYS