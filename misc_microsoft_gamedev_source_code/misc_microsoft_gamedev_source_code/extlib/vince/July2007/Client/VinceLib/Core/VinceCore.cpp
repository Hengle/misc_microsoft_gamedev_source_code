//	VinceCore.h : Public interface of the VinceCore.lib
//
//	Template Created 2006/07/27 Rich Bonny <rbonny@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) Microsoft Corp.  All rights reserved.
        

#include "VinceCore.h"
#include "StringUtil.h"
#include "VinceUtil.h"

namespace Vince
{
	VinceCore* VinceCore::s_pInstance = NULL;// initialize pointer

	VinceCore* VinceCore::Instance() 
	{
		if (s_pInstance == NULL)  // is it the first call?
		{  
			s_pInstance = new VinceCore(); // create sole instance
		}
		return s_pInstance; // address of sole instance
	}

    VinceCore::VinceCore() :
        m_eventCount(0), 
        m_eventsInitialized(0)
	{
		m_pSettings  = new Settings();
        m_pLogWriter = new CLogWriter();
        m_pTimers    = new Timers();
        CreateSessionID();
	}

	HRESULT VinceCore::InitializeEvents(int maxEvents)
	{
        EventActive = new bool[maxEvents];
		EventName = new const char*[maxEvents];
		EventPath = new const char*[maxEvents];
        m_eventCount = maxEvents;
        m_eventsInitialized = true;
        return S_OK;
	}

    HRESULT VinceCore::Initialize()
	{
		// Initialize default folder paths
		InitializePaths();

    	// We now check to see if the source files have been redirected from
		// a code-modified setting. This must happen before attempting to load
        // the .ini file, so this setting is meaningless inside the .ini file.
		const char* cstrSourceFolder = m_pSettings->Fetch( "VinceFolder", "" );
		if ( strlen(cstrSourceFolder) > 0 )
		{
			SetVinceSourceFolder( cstrSourceFolder );
		}

		// Load settings from .ini file, but only if the
		// pSettings object has been created and the loading
		// of the file has not been suppressed by a prior call
		if (m_pSettings->Fetch( "LoadIniFile", true ) )
		{  
			const char* cstrIniFilePath = GetFullFileName("Vince.ini", false);
			bool success = m_pSettings->Load(cstrIniFilePath);
			if (!success)
			{
				// The log file has not yet been opened, so at this point about the
				// best we can do is report the error to the debug channel.
				OutputDebugString("*** Error in Config File ***\n");
				OutputDebugString(cstrIniFilePath);
				OutputDebugString("\n");
				OutputDebugString(m_pSettings->GetErrorMessage());
			}
			SAFE_DELETE_ARRAY(cstrIniFilePath);
		}

        // Initialize logger after settings are loaded since it uses setting values
        m_pLogWriter->Initialize();

		// If logging has been disabled, deactivate all events
		if ( !m_pLogWriter->IsLoggingActive() )
		{
			ActivateEvent( NULL, false );
		}

		// Now we check to see if any events have been activated or deactivated
		// via config settings. We first deactivate. The order shouldn't really matter in most
		// cases, but this makes testing a little easier
		const char* eventname = m_pSettings->Fetch( "EventOff", (const char*)NULL );
		while (eventname)
		{
			ActivateEvent( eventname, false );
			eventname = m_pSettings->FetchNext();
		}

		// ... now check for Activated events.
		eventname = m_pSettings->Fetch( "EventOn", (const char*)NULL );
		while (eventname)
		{
			ActivateEvent( eventname, true );
			eventname = m_pSettings->FetchNext();
		}
        return S_OK;
	}

	VinceCore::~VinceCore(void)
	{
		ClearPaths();
		SAFE_DELETE(m_pSettings);
		SAFE_DELETE(m_pLogWriter);
        
        // Delete event arrays and entries
        for (int i = 0; i < m_eventCount; i++)
        {
            SAFE_DELETE_ARRAY(EventName[i]);
            SAFE_DELETE_ARRAY(EventPath[i]);
        }
        SAFE_DELETE_ARRAY(EventActive);
        SAFE_DELETE_ARRAY(EventName);
        SAFE_DELETE_ARRAY(EventPath);
	}

	// Close current log file and open next in sequence
	HRESULT VinceCore::OpenLog(void)
	{
 		m_pLogWriter->Open();
        return S_OK;
	}

	// Close current log file and open next in sequence
	HRESULT VinceCore::CycleLog(void)
	{
 		m_pLogWriter->Cycle();
        return S_OK;
	}

	// Close current log file and send pending files
	HRESULT VinceCore::CloseLog(void)
	{
 		m_pLogWriter->Close();
 		m_pLogWriter->Transmit();
        return S_OK;
	}

	// Should only be called by auto-generated code
    HRESULT VinceCore::InitializeEvent(int iEventFlag, const char* cstrEventName, const char* cstrEventPath, bool fActive)
	{
        if (!m_eventsInitialized) return E_FAIL;

		EventName[iEventFlag] = SAFE_COPY(cstrEventName);
		EventPath[iEventFlag] = SAFE_COPY(cstrEventPath);
		EventActive[iEventFlag] = fActive;
        return S_OK;
	}

	// Set activation flag for named event(s)
	// This may affect multiple settings, since there is
	// a heirarchy and all subevents are similarly affected.
	// If the first argument is null, all events are affected.
    HRESULT VinceCore::ActivateEvent(const char* cstrEventName, bool fActivate)
	{
        if (!m_eventsInitialized) return E_FAIL;

		if ( NULL == cstrEventName )
		{
			for (int i = 0; i < m_eventCount; i++)
			{
				EventActive[i] = fActivate;
			}

		}

		else
		{
			for (int i = 0; i < m_eventCount; i++)
			{
				if ( 0 == _stricmp(cstrEventName, EventName[i]) )
				{
					EventActive[i] = fActivate;
				}
				else if ( IsSubEvent(cstrEventName, EventPath[i]) )
				{
					EventActive[i] = fActivate;
				}
			}
		}
        return S_OK;
	}

	// Construct the first part of the event xml tag
	bool VinceCore::StartEvent(int iEventFlag)
	{
        if (!m_eventsInitialized) return false;

		if ( EventActive[iEventFlag] )
		{
			m_pLogWriter->WriteEventTag(EventName[iEventFlag]);
			return true;
		}
		else
		{
			return false;
		}
	}

	// Overloaded functions handle writing parameters of various types to
	// the event record. Only called by auto-generated code

    HRESULT VinceCore::SendParameter(const char* cstrParameterName, const char* ParameterValue, bool Enumeration)
	{
        if (Enumeration)
        {
		    m_pLogWriter->WriteParameter(cstrParameterName, "enum", ParameterValue);
        }
        else
        {
		    m_pLogWriter->WriteParameter(cstrParameterName, "string", ParameterValue);
        }
        return S_OK;
	}

    HRESULT VinceCore::SendParameter(const char* cstrParameterName, const wchar_t* ParameterValue, bool Enumeration)
	{
        if (Enumeration)
        {
		    m_pLogWriter->WriteParameter(cstrParameterName, "enum", ParameterValue);
        }
        else
        {
		    m_pLogWriter->WriteParameter(cstrParameterName, "string", ParameterValue);
        }
        return S_OK;
	}

    HRESULT VinceCore::SendParameter(const char* cstrParameterName, bool ParameterValue)
	{
		m_pLogWriter->WriteParameter(cstrParameterName, "bool", ParameterValue);
        return S_OK;
	}

    HRESULT VinceCore::SendParameter(const char* cstrParameterName, BYTE ParameterValue)
	{
		m_pLogWriter->WriteParameter(cstrParameterName, "BYTE", ParameterValue);
        return S_OK;
	}

    HRESULT VinceCore::SendParameter(const char* cstrParameterName, int ParameterValue)
	{
		m_pLogWriter->WriteParameter(cstrParameterName, "int", ParameterValue);
        return S_OK;
	}

    HRESULT VinceCore::SendParameter(const char* cstrParameterName, DWORD ParameterValue)
	{
		m_pLogWriter->WriteParameter(cstrParameterName, "DWORD", ParameterValue);
        return S_OK;
	}

    HRESULT VinceCore::SendParameter(const char* cstrParameterName, long ParameterValue)
	{
		m_pLogWriter->WriteParameter(cstrParameterName, "long", ParameterValue);
        return S_OK;
	}

    HRESULT VinceCore::SendParameter(const char* cstrParameterName, float ParameterValue)
	{
		m_pLogWriter->WriteParameter(cstrParameterName, "float", ParameterValue);
        return S_OK;
	}

    HRESULT VinceCore::SendParameter(const char* cstrParameterName, double ParameterValue)
	{
		m_pLogWriter->WriteParameter(cstrParameterName, "double", ParameterValue);
        return S_OK;
	}

    HRESULT VinceCore::SendEvent()
	{
		m_pLogWriter->WriteEventTail();
        return S_OK;
	}

	// Return boolean value based on whether event is currently activated.
    bool VinceCore::IsEventActive(const char* cstrEventName)
	{
        if (!m_eventsInitialized) return false;

		// Return true if no event specified just to indicate VINCE is alive
		if ( NULL == cstrEventName )
		{
			return true;
		}

		else
		{
			for (int i = 0; i < m_eventCount; i++)
			{
				if ( 0 == _stricmp(cstrEventName, EventName[i]) )
				{
					return EventActive[i];
				}
				// If any sub-event of an event group is active,
				// we consider the group to be active
				else if ( IsSubEvent(cstrEventName, EventPath[i]) )
				{
					if (EventActive[i])
					{
						return true;
					}
				}
			}
			return false;
		}
	}

	// Determines if an event is part of a larger heirarchy. Should only be called
	// by internal routines.
	bool VinceCore::IsSubEvent(const char* cstrEventName, const char* cstrCheckPath)
	{
        if (!m_eventsInitialized) return false;

		int iEventLength = (int)strlen(cstrEventName);
		int iCheckLength = (int)strlen(cstrCheckPath);

		// Path string must be at least as long as search path plus terminating '.'
		if ( iCheckLength > iEventLength )
		{
			int iLookPos = 0;
			int iDotPos = 0;
			while (iLookPos < iCheckLength)
			{
				// First we find the end of the current path segment and
				// make sure the segment name is the same length as the
				// target event set name. The test will also fail if no
				// '.' terminator character is found
				const char* subPath = cstrCheckPath + iLookPos;
				iDotPos = (int) (strchr(subPath, '.') - cstrCheckPath);
				if ( (iDotPos-iLookPos) == iEventLength )
				{
					if ( 0 == _strnicmp(subPath, cstrEventName, iEventLength) )
					{
						return true;
					}
				}
				// Check any remaining segments
				iLookPos = iDotPos + 1;
			}
		}

		// No match found
		return false;
	}

	bool VinceCore::AddSetting(const wchar_t* wcstrSetting)
	{
		return m_pSettings->AddSetting(wcstrSetting);
	}

	bool VinceCore::AddSetting(const char* cstrSetting)
	{
		return m_pSettings->AddSetting(cstrSetting);
	}

    // Write an error to the log. This has a different tag (Error) than normal events.
    HRESULT VinceCore::WriteError(const char* cstrSource, const char* cstrLocation, const char* cstrMessage)
    {
		m_pLogWriter->WriteError(cstrSource, cstrLocation, cstrMessage);
        return S_OK;
    }

    // Write an error to the log. This has a different tag (Error) than normal events.
    HRESULT VinceCore::WriteError(const wchar_t* wcsSource, const wchar_t* wcsLocation, const wchar_t* wcsMessage)
    {
		m_pLogWriter->WriteError(wcsSource, wcsLocation, wcsMessage);
        return S_OK;
    }

    HRESULT VinceCore::SetLogFileWriter(ILogFileWriter* pLogFileWriter)
    {
		return m_pLogWriter->SetLogFileWriter(pLogFileWriter);
    }

    HRESULT VinceCore::ReportSurveyAnswer(const wchar_t* wcsQuestion, const wchar_t* wcsQuestionID, const wchar_t* wcsAnswer,
                               int answerNumber, const wchar_t* wcsContext)
    {
        // Make sure Vince has been initialized before attempting this.
        if (NULL == m_pLogWriter || !m_eventsInitialized)
        {
            return E_FAIL;
        }
		m_pLogWriter->WriteEventTag("SurveyAnswer");
		m_pLogWriter->WriteParameter("Question", "string", wcsQuestion);
		m_pLogWriter->WriteParameter("QuestionID", "string", wcsQuestionID);
		m_pLogWriter->WriteParameter("Answer", "string", wcsAnswer);
		m_pLogWriter->WriteParameter("AnswerNumber", "int", answerNumber);
		m_pLogWriter->WriteParameter("Context", "string", wcsContext);
		m_pLogWriter->WriteEventTail();
        return S_OK;
    }

    HRESULT VinceCore::StartGameTimer()
    {
        return m_pTimers->StartGameTimer();
    }

    HRESULT VinceCore::PauseGameTimer()
    {
        return m_pTimers->PauseGameTimer();
    }

	// *** This is a hotfix patch for an older version of the code base. It avoids a circular
	// *** initialization problem that can cause a client crash. In the most current code base,
	// *** the timer class has been substantially revised, so this patch will not be checked
	// *** into the current code path.
	HRESULT VinceCore::RefreshGameTimer()
    {
		// Initialize the timers the first time we come through here
		static bool timersInitialized = false;
		if (!timersInitialized)
		{
            // Setup and start timers
		    const char* cstrBuild = m_pSettings->Fetch( "Build", "Unknown" );
            m_pTimers->Initialize(cstrBuild);
			timersInitialized = true;
		}

        return m_pTimers->RefreshGameTimer();
    }

    DWORD VinceCore::GetGameTime()
    {
         return m_pTimers->GetGameTime();
   }

    DWORD VinceCore::GetSessionTime()
    {
        return m_pTimers->GetSessionTime();
    }


    DWORD VinceCore::GetSessionID()
    {
        return m_SessionID;
    }

    void VinceCore::CreateSessionID()
    {
        FILETIME UTCFileTime;
        GetSystemTimeAsFileTime( &UTCFileTime );
        DWORD ID = GetTickCount();
        ID ^= UTCFileTime.dwHighDateTime;
        ID ^= UTCFileTime.dwLowDateTime;
        m_SessionID = ID;
    }

	HRESULT VinceCore::Terminate()
	{
		delete s_pInstance;
		s_pInstance = NULL;
        return S_OK;
	}
}