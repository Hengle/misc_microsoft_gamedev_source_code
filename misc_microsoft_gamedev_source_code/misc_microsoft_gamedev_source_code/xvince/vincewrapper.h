//==============================================================================
// vincewrapper.h
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================

#ifndef _VINCEWRAPPER_H_
#define _VINCEWRAPPER_H_

// Vince include
#include "Vince.h"

#ifdef _VINCE_

class XVinceWrapper
{
	private:
		// nothing...?

	protected:
		unsigned int mEventsProcessing;
		
		XVinceWrapper();
		~XVinceWrapper();

	public:
		bool mUseVinceWrite;

		inline static XVinceWrapper& GetInstance()
		{
			static XVinceWrapper instance;
			return instance;
		}

		void InitializeVince( void* pDevice, BOOL enableLog );

		void EventStart(const char* eventName);
		void EventSendParameter(const char* parameterName, const char* parameterValue);
		void EventSendParameter(const char* parameterName, const wchar_t* parameterValue);
		void EventSendParameter(const char* parameterName, bool parameterValue);
		void EventSendParameter(const char* parameterName, BYTE parameterValue);
		void EventSendParameter(const char* parameterName, int parameterValue);
		void EventSendParameter(const char* parameterName, DWORD parameterValue);
		void EventSendParameter(const char* parameterName, long parameterValue);
		void EventSendParameter(const char* parameterName, float parameterValue);
		void EventSendParameter(const char* parameterName, double parameterValue);
		void EventEnd();

		void OpenNewLog();
		void CloseAndSendLog();
};

#endif // _VINCE_

#endif // _VINCEWRAPPER_H_