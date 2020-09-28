//	Vince.h : The top level class of the Vince interface.
//  This initializes the config file settings, logging file
//  and survey data.
//
//	Created 2004/04/01 Rich Bonny <rbonny@microsoft.com>
//  Modifed 2004/04/20 rbonny - include VinceControl.h
//                     and bypass most of code when _VINCE_
//                     flag is not set.
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2004 Microsoft Corp.  All rights reserved.

#pragma once

#include "VinceControl.h"
#include "TnTSettings.h" // benst: *** IMPORTANT *** - RENAMED to "TnTSettings.h" because Phoenix has a Settings.h that this otherwise conflicts with

#ifdef _VINCE_
#include "Survey.h"
#endif

#include "VinceEvents.h"


#ifdef _VINCE_
namespace Vince
{
	class CVince 
	{
	public:
		CVince();
		void Initialize( void* pDevice, BOOL enableLog );
		void InitializeViewer(void* pDevice, const char* lpSurveyFile);
        void SetHWND(HWND hWnd);
		void SetDevice(void* pDevice);
		void SavePresentationParameters(void* pParameters);
		~CVince(void);
		CSurvey* pSurvey;
	    CLogWriter* pLogWriter;
		LogEvent* pLogEvent;
        TnT::Settings* pSettings;
	};
}
// extern Vince::CVince* g_pVince;
#endif // _VINCE_