//	Vince.cpp : This module contains the public API
//  functions and just passes them through to the
//  VinceCore object
//
//	Created 2006/12/08 Rich Bonny <rbonny@microsoft.com>
//	Revised 2007/01/09 Rich Bonny <rbonny@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) Microsoft Corp.  All rights reserved.

#ifdef _XBOX
  #include <xtl.h>
#else
  #include <Windows.h>
#endif

#include "Vince.h"
#include "VinceCore.h"

namespace Vince
{
    HRESULT InitializeCore()
	{
        return VinceCore::Instance()->Initialize();
    }

    HRESULT OpenLog()
    {
        return VinceCore::Instance()->OpenLog();
    }

	HRESULT CycleLog()
    {
        return VinceCore::Instance()->CycleLog();
    }


	HRESULT CloseLog()
    {
        return VinceCore::Instance()->CloseLog();
    }

   HRESULT TransmitLog()
   {
      return VinceCore::Instance()->TransmitLog();
   }

   bool IsUploading()
   {
      return VinceCore::Instance()->IsUploading();
   }

	HRESULT Terminate()
    {
        return VinceCore::Instance()->Terminate();
    }

	HRESULT WriteError(const char* cstrSource, const char* cstrLocation, const char* cstrMessage)
    {
        return VinceCore::Instance()->WriteError(cstrSource, cstrLocation, cstrMessage);
    }

	HRESULT WriteError(const wchar_t* wcsSource, const wchar_t* wcsLocation, const wchar_t* wcsMessage)
    {
        return VinceCore::Instance()->WriteError(wcsSource, wcsLocation, wcsMessage);
    }

	// VinceCore is the ISurveyLogger implementation, so just return it's instance
    HRESULT GetSurveyLoggerInterface(SurveyLib::ISurveyLogger** ppSurveyLogger)
    {
        *ppSurveyLogger = (SurveyLib::ISurveyLogger*)VinceCore::Instance();
        return S_OK;
    }

	// Pass a pointer to a custom log writer routine
    HRESULT SetLogFileWriter(ILogFileWriter* pLogFileWriter)
    {
        return VinceCore::Instance()->SetLogFileWriter(pLogFileWriter);
    }

    HRESULT InitializeEvents(int maxEvents)
    {
        return VinceCore::Instance()->InitializeEvents(maxEvents);
    }

    HRESULT InitializeEvent(int iEventFlag, const char* cstrEventName, const char* cstrEventPath, bool fActive)
    {
        return VinceCore::Instance()->InitializeEvent(iEventFlag, cstrEventName, cstrEventPath, fActive);
    }

    bool IsSubEvent(const char* cstrEventName, const char* cstrCheckPath)
    {
        return VinceCore::Instance()->IsSubEvent(cstrEventName, cstrCheckPath);
    }

    bool StartEvent(int iEventFlag)
    {
        return VinceCore::Instance()->StartEvent(iEventFlag);
    }

    HRESULT SendParameter(const char* cstrParameterName, const char* ParameterValue, bool Enumeration)
    {
        return VinceCore::Instance()->SendParameter(cstrParameterName, ParameterValue, Enumeration);
    }

    HRESULT SendParameter(const char* cstrParameterName, const wchar_t* ParameterValue, bool Enumeration)
    {
        return VinceCore::Instance()->SendParameter(cstrParameterName, ParameterValue, Enumeration);
    }

    HRESULT SendParameter(const char* cstrParameterName, bool ParameterValue)
    {
        return VinceCore::Instance()->SendParameter(cstrParameterName, ParameterValue);
    }

    HRESULT SendParameter(const char* cstrParameterName, BYTE ParameterValue)
    {
        return VinceCore::Instance()->SendParameter(cstrParameterName, ParameterValue);
    }

    HRESULT SendParameter(const char* cstrParameterName, int ParameterValue)
    {
        return VinceCore::Instance()->SendParameter(cstrParameterName, ParameterValue);
    }

    HRESULT SendParameter(const char* cstrParameterName, DWORD ParameterValue)
    {
        return VinceCore::Instance()->SendParameter(cstrParameterName, ParameterValue);
    }

    HRESULT SendParameter(const char* cstrParameterName, long ParameterValue)
    {
        return VinceCore::Instance()->SendParameter(cstrParameterName, ParameterValue);
    }

    HRESULT SendParameter(const char* cstrParameterName, float ParameterValue)
    {
        return VinceCore::Instance()->SendParameter(cstrParameterName, ParameterValue);
    }

    HRESULT SendParameter(const char* cstrParameterName, double ParameterValue)
    {
        return VinceCore::Instance()->SendParameter(cstrParameterName, ParameterValue);
    }

    HRESULT SendEvent()
    {
        return VinceCore::Instance()->SendEvent();
    }

    HRESULT ActivateEvent(const char* cstrEventName, bool fActivate)
    {
        return VinceCore::Instance()->ActivateEvent(cstrEventName, fActivate);
    }

    bool IsEventActive(const char* cstrEventName)
    {
        return VinceCore::Instance()->IsEventActive(cstrEventName);
    }

	bool AddSetting(const wchar_t* wcstrSetting)
    {
        return VinceCore::Instance()->AddSetting(wcstrSetting);
    }

    bool AddSetting(const char* cstrSetting)
    {
        return VinceCore::Instance()->AddSetting(cstrSetting);
    }

    HRESULT StartGameTimer()
    {
        return VinceCore::Instance()->StartGameTimer();
    }

    HRESULT PauseGameTimer()
    {
        return VinceCore::Instance()->PauseGameTimer();
    }

    HRESULT RefreshGameTimer()
    {
        return VinceCore::Instance()->RefreshGameTimer();
    }

    DWORD GetSessionID()
    {
        return VinceCore::Instance()->GetSessionID();
    }
}
