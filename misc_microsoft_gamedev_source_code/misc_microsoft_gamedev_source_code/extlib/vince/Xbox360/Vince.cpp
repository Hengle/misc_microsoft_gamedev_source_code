//	Vince.cpp : The top level class of the Vince interface.
//  This initializes the config file settings, logging file
//  and survey data.
//
//	Created 2004/04/01 Rich Bonny <rbonny@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2004 Microsoft Corp.  All rights reserved.

#include "VinceControl.h"

#ifdef _VINCE_

#include "Vince.h"
#include "TnTUtil.h"
#include "VinceUtil.h"
#include "Display.h"
 
namespace Vince
{
	// This lightweight constructor only instantiates the Settings
	// object to enable presets from code. Most of the initialization
	// is handled in the Initialize call.
	CVince::CVince()
	{
        pSurvey    = NULL;
        pLogEvent  = NULL;
		pSettings  = new TnT::Settings();
        pLogWriter = new CLogWriter();
	}

    void CVince::Initialize( void* pDevice, BOOL enableLog )
	{
        SetDevice(pDevice);

		// Initialize default folder paths
		InitializePaths();

		// Load settings from .ini file, but only if the
		// pSettings object has been created and the loading
		// of the file has not been suppressed by a prior call
		if (pSettings->Fetch( "LoadIniFile", true ) )
		{  
			const char* cstrIniFilePath = GetFullFileName("Vince.ini", false);
			bool success = pSettings->Load(cstrIniFilePath);
			if (!success)
			{
				// The log file has not yet been opened, so at this point about the
				// best we can do is report the error to the debug channel.
				OutputDebugString("*** Error in Config File ***\n");
				OutputDebugString(cstrIniFilePath);
				OutputDebugString("\n");
				OutputDebugString(pSettings->GetErrorMessage());
			}
			SAFE_DELETE_ARRAY(cstrIniFilePath);
		}

		// We now check to see if the source files have been redirected from
		// a code-modified setting
		const char* cstrSourceFolder = pSettings->Fetch( "VinceFolder", "" );
		if ( strlen(cstrSourceFolder) > 0 )
		{
			SetVinceSourceFolder( cstrSourceFolder );
		}

      // Initialize logger after settings are loaded since it uses setting values
      pLogWriter->Initialize( this, enableLog );

		pLogEvent = new Vince::LogEvent();
		pLogEvent->Initialize();

		// If logging has been disabled, deactivate all events
		if ( !pLogWriter->IsLoggingActive() )
		{
			pLogEvent->ActivateEvent( NULL, false );
		}

		// Now we check to see if any events have been activated or deactivated
		// via config settings. We first deactivate. The order shouldn't really matter in most
		// cases, but this makes testing a little easier
		const char* eventname = pSettings->Fetch( "EventOff", (const char*)NULL );
		while (eventname)
		{
			pLogEvent->ActivateEvent( eventname, false );
			eventname = pSettings->FetchNext();
		}

		// ... now check for Activated events.
		eventname = pSettings->Fetch( "EventOn", (const char*)NULL );
		while (eventname)
		{
			pLogEvent->ActivateEvent( eventname, true );
			eventname = pSettings->FetchNext();
		}


// We may have compiled out all survey code
#ifndef NO_VINCE_SURVEYS
		pSurvey = new Vince::CSurvey(this);
		pSurvey->Load();
#endif
	}

	// This Initialization code is not for general use.
	// It is only used by the survey previewer to allow
	// viewing surveys without creating a log.
	void CVince::InitializeViewer(void* pDevice, const char* lpSurveyFile)
	{
        // Initialize logger first 
        pLogWriter->Initialize( this, false );

        SetDevice(pDevice);

		//pSettings->ReportErrors = false;
		// Temp hardcode RWBFIx
		pSettings->Load("Vince.ini");

		pSettings->AddSetting("Logging=false");

		pLogEvent = new Vince::LogEvent();
		pLogEvent->Initialize();
		// Deactivate all events, since we are not logging
		pLogEvent->ActivateEvent( NULL, false );

		// If a command line argument was passed, use that
		// as the survey file
		if ( strlen(lpSurveyFile) > 0 )
		{
			char chSetFile[MAX_PATH + 16];
			strcpy(chSetFile, "SurveyFileName=");
			strcat(chSetFile, lpSurveyFile);
			pSettings->AddSetting(chSetFile);
		}

		// Activate survey displays in spite of logging being
		// turned off.

// We may have compiled out all survey code, although this would make little
// sense for the VinceViewer application
#ifndef NO_VINCE_SURVEYS
		pSettings->AddSetting("PresentSurveys=true");
		pSurvey = new Vince::CSurvey(this);
		pSurvey->Load();
#endif
	}

	void CVince::SetHWND(HWND hWnd)
	{
// Not applicable if Surveys have been disabled
#ifndef NO_VINCE_SURVEYS
		CDisplay* pDisplay = CDisplay::Instance();
		pDisplay->SetHWND(hWnd);
#endif
	}

	void CVince::SetDevice(void* pDevice)
	{
// Not applicable if Surveys have been disabled
#ifndef NO_VINCE_SURVEYS
		CDisplay* pDisplay = CDisplay::Instance();
		pDisplay->SetDevice(pDevice);
#endif
	}

	void CVince::SavePresentationParameters(void* pParameters)
	{
// Not applicable if Surveys have been disabled
#ifndef NO_VINCE_SURVEYS
		CDisplay* pDisplay = CDisplay::Instance();
		pDisplay->SavePresentationParameters(pParameters);
#endif
	}

	CVince::~CVince(void)
	{
		ClearPaths();
		SAFE_DELETE(pSettings);
		SAFE_DELETE(pLogWriter);
		SAFE_DELETE(pLogEvent);
#ifndef NO_VINCE_SURVEYS
		SAFE_DELETE(pSurvey);
#endif
	}
}

#endif // _VINCE_