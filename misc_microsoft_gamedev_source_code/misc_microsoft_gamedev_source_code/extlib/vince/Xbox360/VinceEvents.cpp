//	VinceEvents.cpp : Class used to log events.
//	The specific logging calls are defined in the
//  auto-generated VinceEvents.h file. This file
//  contains the standard functions used by those
//  event calls.
//
//	Created 2004/03/15 Rich Bonny <rbonny@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2004 Microsoft Corp.  All rights reserved.

#include "VinceControl.h"

#ifdef _VINCE_

#include "Vince.h"
#include "VinceEvents.h"
#include "TnTUtil.h"

namespace Vince
{

	// must declare static arrays and members

	bool LogEvent::EventActive[VINCE_MAX_EVENTS];
	const char* LogEvent::EventName[VINCE_MAX_EVENTS];
	const char* LogEvent::EventPath[VINCE_MAX_EVENTS];

	LogEvent::LogEvent(void)
	{
	}

	LogEvent::~LogEvent(void)
	{
		for( int i = 0; i < VINCE_MAX_EVENTS; i++)
		{
			SAFE_DELETE_ARRAY( EventName[i] );
			SAFE_DELETE_ARRAY( EventPath[i] );
		}
	}

	// Close current log file and open next in sequence
	void LogEvent::OpenLog(CVince* vince)
	{
		vince->pLogWriter->Open(vince);
	}

	// Close current log file and open next in sequence
	void LogEvent::CycleLog(CVince* vince)
	{
		vince->pLogWriter->Cycle(vince);
	}

	// Close current log file and send pending files
	void LogEvent::CloseLog(CVince* vince)
	{
		vince->pLogWriter->Close();
		vince->pLogWriter->Transmit();
	}

	// Should only be called by auto-generated code
    void LogEvent::InitializeEvent(int iEventFlag, const char* cstrEventName, const char* cstrEventPath, bool fActive)
	{
		EventName[iEventFlag] = SAFE_COPY(cstrEventName);
		EventPath[iEventFlag] = SAFE_COPY(cstrEventPath);
		EventActive[iEventFlag] = fActive;
	}

	// Set activation flag for named event(s)
	// This may affect multiple settings, since there is
	// a heirarchy and all subevents are similarly affected.
	// If the first argument is null, all events are affected.
    void LogEvent::ActivateEvent(const char* cstrEventName, bool fActivate)
	{
		if ( NULL == cstrEventName )
		{
			for (int i = 0; i < VINCE_MAX_EVENTS; i++)
			{
				EventActive[i] = fActivate;
			}

		}

		else
		{
			for (int i = 0; i < VINCE_MAX_EVENTS; i++)
			{
				if ( 0 == stricmp(cstrEventName, EventName[i]) )
				{
					EventActive[i] = fActivate;
				}
				else if ( IsSubEvent(cstrEventName, EventPath[i]) )
				{
					EventActive[i] = fActivate;
				}
			}
		}
	}

	// Return boolean value based on whether event is currently activated.
    bool LogEvent::IsEventActive(const char* cstrEventName)
	{
		// Return true if no event specified just to indicate VINCE is alive
		if ( NULL == cstrEventName )
		{
			return true;
		}

		else
		{
			for (int i = 0; i < VINCE_MAX_EVENTS; i++)
			{
				if ( 0 == stricmp(cstrEventName, EventName[i]) )
				{
					return EventActive[i];
				}
				// If any sub-event of an event group is active,
				// we consider the group to be active
				else if ( IsSubEvent(cstrEventName, EventPath[i]) )
				{
					if (EventActive[i])
						return true;
				}
			}
			return false;
		}
	}

	// Determines if an event is part of a larger heirarchy. Should only be called
	// by internal routines.
	bool LogEvent::IsSubEvent(const char* cstrEventName, const char* cstrCheckPath)
	{
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
					if ( 0 == strnicmp(subPath, cstrEventName, iEventLength) )
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

	// Construct the first part of the event xml tag
	bool LogEvent::StartEvent(CVince* vince, int iEventFlag)
	{
		if ( EventActive[iEventFlag] )
		{
			vince->pLogWriter->WriteEventTag(EventName[iEventFlag]);
			return true;
		}
		else
		{
			return false;
		}
	}

	// Overloaded functions handle writing parameters of various types to
	// the event record. Only called by auto-generated code

    void LogEvent::SendParameter(CVince* vince, const char* cstrParameterName, const char* ParameterValue)
	{
		vince->pLogWriter->WriteParameter(cstrParameterName, "string", ParameterValue);
	}

	// Re-enabled.
    void LogEvent::SendParameter(CVince* vince, const char* cstrParameterName, const wchar_t* ParameterValue)
	{
		vince->pLogWriter->WriteParameter(cstrParameterName, "string", ParameterValue);
	}

    void LogEvent::SendParameter(CVince* vince, const char* cstrParameterName, bool ParameterValue)
	{
		vince->pLogWriter->WriteParameter(cstrParameterName, "bool", ParameterValue);
	}

    void LogEvent::SendParameter(CVince* vince, const char* cstrParameterName, BYTE ParameterValue)
	{
		vince->pLogWriter->WriteParameter(cstrParameterName, "BYTE", ParameterValue);
	}

    void LogEvent::SendParameter(CVince* vince, const char* cstrParameterName, int ParameterValue)
	{
		vince->pLogWriter->WriteParameter(cstrParameterName, "int", ParameterValue);
	}

    void LogEvent::SendParameter(CVince* vince, const char* cstrParameterName, DWORD ParameterValue)
	{
		vince->pLogWriter->WriteParameter(cstrParameterName, "DWORD", ParameterValue);
	}

    void LogEvent::SendParameter(CVince* vince, const char* cstrParameterName, long ParameterValue)
	{
		vince->pLogWriter->WriteParameter(cstrParameterName, "long", ParameterValue);
	}

    void LogEvent::SendParameter(CVince* vince, const char* cstrParameterName, float ParameterValue)
	{
		vince->pLogWriter->WriteParameter(cstrParameterName, "float", ParameterValue);
	}

    void LogEvent::SendParameter(CVince* vince, const char* cstrParameterName, double ParameterValue)
	{
		vince->pLogWriter->WriteParameter(cstrParameterName, "double", ParameterValue);
	}

    void LogEvent::SendEvent(CVince* vince)
	{
		vince->pLogWriter->WriteEventTail();
	}
}

#endif // _VINCE_