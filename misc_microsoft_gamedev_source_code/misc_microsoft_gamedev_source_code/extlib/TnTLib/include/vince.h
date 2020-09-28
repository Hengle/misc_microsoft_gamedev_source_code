//	Vince.h : The top level class of the Vince interface.
//  This initializes the config file settings and the log file.
//
//	Created 2006/08/01 Rich Bonny <rbonny@microsoft.com>
//	Revised 2007/01/09 Rich Bonny <rbonny@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) Microsoft Corp.  All rights reserved.

#pragma once

// Declaration and initialization macros
// Use of the macro format allows these calls to be conditionally excluded by the compiler.

#ifdef _VINCE_
  #define VINCE_SETTING(setting) Vince::AddSetting(setting)
  #define VINCE_LOG_OPEN Vince::OpenLog()
  #define VINCE_LOG_CYCLE Vince::CycleLog()
  #define VINCE_LOG_CLOSE Vince::CloseLog()
  #define VINCE_EVENT_ACTIVATE(event, active) Vince::ActivateEvent(event, active)
  #define VINCE_EVENT_ACTIVE(event) Vince::IsEventActive(event)
  #define VINCE_TERMINATE Vince::Terminate()
  #define VINCE_WRITE_ERROR(source, location, message) Vince::WriteError(source, location, message)

  // Timers and SessionID
  #define VINCE_START_GAME_TIMER Vince::StartGameTimer()
  #define VINCE_PAUSE_GAME_TIMER Vince::PauseGameTimer()
  #define VINCE_RESUME_GAME_TIMER Vince::StartGameTimer()
  #define VINCE_REFRESH_GAME_TIMER  Vince::RefreshGameTimer()
  #define VINCE_SESSION_ID Vince::GetSessionID()

  // These Vince functions are callable from client code, or they can be accessed via
  // the preceding macros.

  #include "ISurveyLogger.h"
  #include "ILogFileWriter.h"

  namespace Vince
  {
    // The first set of functions are the basic set of functions used by client code
    // for integrating Vince into an application.

    HRESULT OpenLog();
    HRESULT CycleLog();
    HRESULT CloseLog();
    HRESULT Terminate();
    HRESULT WriteError(const char* cstrSource, const char* cstrLocation, const char* cstrMessage);
    HRESULT WriteError(const wchar_t* cstrSource, const wchar_t* cstrLocation, const wchar_t* cstrMessage);

    HRESULT ActivateEvent(const char* cstrEventName, bool fActivate);
	bool IsEventActive(const char* cstrEventName);

	bool AddSetting(const wchar_t* wcstrSetting);
	bool AddSetting(const char* cstrSetting);

    // These functions should not be called directly, but are required by the
    // automatically-generated application-specific event API code.

	HRESULT InitializeEvents(int maxEvents);
	HRESULT InitializeEvent(int iEventFlag, const char* cstrEventName, const char* cstrEventPath, bool fActive);
    HRESULT InitializeCore();
	bool IsSubEvent(const char* cstrEventName, const char* cstrCheckPath);
	bool StartEvent(int iEventFlag);
	HRESULT SendParameter(const char* cstrParameterName, const char* ParameterValue, bool Enumeration = false);
	HRESULT SendParameter(const char* cstrParameterName, const wchar_t* ParameterValue, bool Enumeration = false);
	HRESULT SendParameter(const char* cstrParameterName, bool ParameterValue);
	HRESULT SendParameter(const char* cstrParameterName, BYTE ParameterValue);
	HRESULT SendParameter(const char* cstrParameterName, int ParameterValue);
	HRESULT SendParameter(const char* cstrParameterName, DWORD ParameterValue);
	HRESULT SendParameter(const char* cstrParameterName, long ParameterValue);
	HRESULT SendParameter(const char* cstrParameterName, float ParameterValue);
	HRESULT SendParameter(const char* cstrParameterName, double ParameterValue);
	HRESULT SendEvent();

    // This function returns a pointer to an ISurveyLogger interface for use by the external
    // survey presentation library.
    HRESULT GetSurveyLoggerInterface(SurveyLib::ISurveyLogger** ppSurveyLogger);

    // This function sets a pointer to an ILogFileWriter interface for use by custom
    // file writing code.
    HRESULT SetLogFileWriter(ILogFileWriter* pLogFileWriter);

    // Timers and SessionID
    HRESULT StartGameTimer();
    HRESULT PauseGameTimer();
    HRESULT RefreshGameTimer();
    DWORD GetSessionID();
  } // namespace Vince

#else
  #define VINCE_SETTING(setting)
  #define VINCE_LOG_OPEN
  #define VINCE_LOG_CYCLE
  #define VINCE_LOG_CLOSE
  #define VINCE_EVENT_ACTIVATE(event, active)
  #define VINCE_EVENT_ACTIVE(event) false
  #define VINCE_TERMINATE
  #define VINCE_EVENT(event, params)
  #define VINCE_WRITE_ERROR(source, location, message)
  #define VINCE_START_GAME_TIMER
  #define VINCE_PAUSE_GAME_TIMER
  #define VINCE_RESUME_GAME_TIMER
  #define VINCE_SESSION_ID 0
#endif // _VINCE_

// Also need to include VinceEvents.h,
// at least to load macros.
//#include "VinceEvents.h"