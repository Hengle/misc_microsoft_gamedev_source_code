//==============================================================================
// vincewrapper.cpp
//
// Copyright (c) 2006 Ensemble Studios
// May 2, 2007 - Microsoft bburli - This file has been replaced by the April 2007 
// TnTXDK implementation of Vince.  VinceHelper.cpp now uses that interface instead.
//==============================================================================

#ifdef _VINCE_

// Includes
#include "vincewrapper.h"

// Constructor
XVinceWrapper::XVinceWrapper()
{
	mEventsProcessing = 0;
	mUseVinceWrite = true;
}

// Destructor
XVinceWrapper::~XVinceWrapper()
{
}


// InitializeVince()
void XVinceWrapper::InitializeVince( void* pDevice, BOOL enableLog )
{
}


// EventStart()
// Use this when you want to begin sending a new VINCE event
void XVinceWrapper::EventStart(const char* eventName)
{
	// We don't want this to resolve to true EVER - multiple events should not fire at once
	// BUT, if they do, this is here to make sure that we throw away everything until
	// we get back to the state where no events are processing anymore.
	if (mEventsProcessing)
	{
		// In the (unlikely?) case that multiple threads try to write two events at once
		// we want to prevent that. The "preferred" method would be to sleep whatever thread
		// is the johnny-come-lately and let it wait for the first one to finish. However, until
		// a better understanding of the game's threading is understood, we'll just throw this
		// event away for now.

		// close off the event - we may get a malformed event, but at least we can keep writing new ones
		//mVince->pLogWriter->WriteEventTail();
		mEventsProcessing--;
	}
	else
	{
		mEventsProcessing++;

		if (mUseVinceWrite)
		{
			//mVince->pLogWriter->WriteEventTag(eventName);
		}
		else
		{
			// this space left blank for Ensemble to develop their own log-writing functionality if they want to
		}
	}
}

// EventSendParameter()
// Overloaded for different values, use these to send data for a VINCE event
void XVinceWrapper::EventSendParameter(const char* parameterName, const char* parameterValue)
{
	if (mEventsProcessing)
	{
		if (mUseVinceWrite)
		{
			//mVince->pLogWriter->WriteParameter(parameterName, "string", parameterValue);
		}
		else
		{
			// this space left blank for Ensemble to develop their own log-writing functionality if they want to
		}
	}
	else
	{
		// Don't be evil! You're not allowed to send data if you haven't started an event
	}
}

void XVinceWrapper::EventSendParameter(const char* parameterName, const wchar_t* parameterValue)
{
	if (mEventsProcessing)
	{
		if (mUseVinceWrite)
		{
			//mVince->pLogWriter->WriteParameter(parameterName, "string", parameterValue);
		}
		else
		{
			// this space left blank for Ensemble to develop their own log-writing functionality if they want to
		}
	}
	else
	{
		// Don't be evil! You're not allowed to send data if you haven't started an event
	}
}

void XVinceWrapper::EventSendParameter(const char* parameterName, bool parameterValue)
{
	if (mEventsProcessing)
	{
		if (mUseVinceWrite)
		{
			//mVince->pLogWriter->WriteParameter(parameterName, "bool", parameterValue);
		}
		else
		{
			// this space left blank for Ensemble to develop their own log-writing functionality if they want to
		}
	}
	else
	{
		// Don't be evil! You're not allowed to send data if you haven't started an event
	}
}

void XVinceWrapper::EventSendParameter(const char* parameterName, BYTE parameterValue)
{
	if (mEventsProcessing)
	{
		if (mUseVinceWrite)
		{
			//mVince->pLogWriter->WriteParameter(parameterName, "BYTE", parameterValue);
		}
		else
		{
			// this space left blank for Ensemble to develop their own log-writing functionality if they want to
		}
	}
	else
	{
		// Don't be evil! You're not allowed to send data if you haven't started an event
	}
}

void XVinceWrapper::EventSendParameter(const char* parameterName, int parameterValue)
{
	if (mEventsProcessing)
	{
		if (mUseVinceWrite)
		{
			//mVince->pLogWriter->WriteParameter(parameterName, "int", parameterValue);
		}
		else
		{
			// this space left blank for Ensemble to develop their own log-writing functionality if they want to
		}
	}
	else
	{
		// Don't be evil! You're not allowed to send data if you haven't started an event
	}
}

void XVinceWrapper::EventSendParameter(const char* parameterName, DWORD parameterValue)
{
	if (mEventsProcessing)
	{
		if (mUseVinceWrite)
		{
			//mVince->pLogWriter->WriteParameter(parameterName, "DWORD", parameterValue);
		}
		else
		{
			// this space left blank for Ensemble to develop their own log-writing functionality if they want to
		}
	}
	else
	{
		// Don't be evil! You're not allowed to send data if you haven't started an event
	}
}

void XVinceWrapper::EventSendParameter(const char* parameterName, long parameterValue)
{
	if (mEventsProcessing)
	{
		if (mUseVinceWrite)
		{
			//mVince->pLogWriter->WriteParameter(parameterName, "long", parameterValue);
		}
		else
		{
			// this space left blank for Ensemble to develop their own log-writing functionality if they want to
		}
	}
	else
	{
		// Don't be evil! You're not allowed to send data if you haven't started an event
	}
}

void XVinceWrapper::EventSendParameter(const char* parameterName, float parameterValue)
{
	if (mEventsProcessing)
	{
		if (mUseVinceWrite)
		{
			//mVince->pLogWriter->WriteParameter(parameterName, "float", parameterValue);
		}
		else
		{
			// this space left blank for Ensemble to develop their own log-writing functionality if they want to
		}
	}
	else
	{
		// Don't be evil! You're not allowed to send data if you haven't started an event
	}
}

void XVinceWrapper::EventSendParameter(const char* parameterName, double parameterValue)
{
	if (mEventsProcessing)
	{
		if (mUseVinceWrite)
		{
			//mVince->pLogWriter->WriteParameter(parameterName, "double", parameterValue);
		}
		else
		{
			// this space left blank for Ensemble to develop their own log-writing functionality if they want to
		}
	}
	else
	{
		// Don't be evil! You're not allowed to send data if you haven't started an event
	}
}


// EventEnd()
// Use this to end a VINCE event
void XVinceWrapper::EventEnd()
{
	if (mEventsProcessing)
	{
		if (mUseVinceWrite)
		{
			//mVince->pLogWriter->WriteEventTail();
		}
		else
		{
			// this space left blank for Ensemble to develop their own log-writing functionality if they want to
		}

		mEventsProcessing--;
	}
	else
	{
		// You can't end an event if you didn't start one in the first place!
	}
}

// OpenNewLog()
// Used to open a new log file
void XVinceWrapper::OpenNewLog()
{
	//mVince->pLogWriter->Open(mVince);
}

// CloseAndSendLog()
// Used to close the current log and send it up to the server
void XVinceWrapper::CloseAndSendLog()
{
	//mVince->pLogWriter->Close();
	//mVince->pLogWriter->Transmit();
}

#endif // _VINCE_