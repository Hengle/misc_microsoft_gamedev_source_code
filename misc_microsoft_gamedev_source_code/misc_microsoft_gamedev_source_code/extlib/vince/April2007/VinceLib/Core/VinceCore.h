//	VinceCore.h : Public interface of the VinceCore.lib
//
//	Template Created 2006/07/27 Rich Bonny <rbonny@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) Microsoft Corp.  All rights reserved.
        
#pragma once
#include "LogWriter.h"
#include "Settings.h"
#include "Timers.h"
#include "ISurveyLogger.h"
#include "ILogFileWriter.h"

namespace Vince
{
    class VinceCore : public SurveyLib::ISurveyLogger
    {
    public:
		HRESULT Initialize();
        HRESULT OpenLog();
        HRESULT CycleLog();
        HRESULT CloseLog();
        HRESULT TransmitLog();
        bool IsUploading();
		HRESULT Terminate();
		HRESULT WriteError(const char* cstrSource, const char* cstrLocation, const char* cstrMessage);
		HRESULT WriteError(const wchar_t* wcsSource, const wchar_t* wcsLocation, const wchar_t* wcsMessage);

        // Although these are public, they would only be called by automatically generated functions
        // within VinceEvents.h

		HRESULT InitializeEvents(int maxEvents);
		HRESULT InitializeEvent(int iEventFlag, const char* cstrEventName, const char* cstrEventPath, bool fActive);
		bool IsSubEvent(const char* cstrEventName, const char* cstrCheckPath);
		bool StartEvent(int iEventFlag);
        HRESULT SendParameter(const char* cstrParameterName, const char* ParameterValue, bool Enumeration);
        HRESULT SendParameter(const char* cstrParameterName, const wchar_t* ParameterValue, bool Enumeration);
        HRESULT SendParameter(const char* cstrParameterName, bool ParameterValue);
        HRESULT SendParameter(const char* cstrParameterName, BYTE ParameterValue);
        HRESULT SendParameter(const char* cstrParameterName, int ParameterValue);
        HRESULT SendParameter(const char* cstrParameterName, DWORD ParameterValue);
        HRESULT SendParameter(const char* cstrParameterName, long ParameterValue);
        HRESULT SendParameter(const char* cstrParameterName, float ParameterValue);
        HRESULT SendParameter(const char* cstrParameterName, double ParameterValue);
        HRESULT SendEvent();

        HRESULT ActivateEvent(const char* cstrEventName, bool fActivate);
        bool IsEventActive(const char* cstrEventName);

		bool AddSetting(const wchar_t* wcstrSetting);
		bool AddSetting(const char* cstrSetting);
        Settings* GetSettings() {return m_pSettings;};

        static VinceCore* Instance();

        // Refcounting - not currently used, but required by ISurveyLogger
        virtual void AddRef() {};
        virtual void Release() {};

        // Support for external survey library
        virtual HRESULT ReportSurveyAnswer(const wchar_t* wcsQuestion, const wchar_t* wcsQuestionID, const wchar_t* wcsAnswer,
                                   int answerNumber, const wchar_t* wcsContext);	

        // Support for custom log file writers
        HRESULT SetLogFileWriter(ILogFileWriter* pLogFileWriter);

        // Timers and SessionID
        HRESULT StartGameTimer();
        HRESULT PauseGameTimer();
        HRESULT RefreshGameTimer();
        DWORD GetGameTime();
        DWORD GetSessionTime();
        DWORD GetSessionID();

    private:

		VinceCore();
		~VinceCore(void);
		static VinceCore* s_pInstance;
		static void DestroyInstance();
        void CreateSessionID();
        DWORD m_SessionID;

        // Here are the various components
        Settings* m_pSettings;
        CLogWriter* m_pLogWriter;
        Timers* m_pTimers;

        // Arrays of event data
        int m_eventCount;
        bool m_eventsInitialized;
        bool* EventActive;
		const char** EventName;
		const char** EventPath;
    };

} // namespace Vince
