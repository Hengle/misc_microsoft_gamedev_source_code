
//	VinceEvents.h : Auto-generated file that contains
//	the definition of all available event calls
//
//	Template Created 2004/03/15 Rich Bonny <rbonny@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2004 Microsoft Corp.  All rights reserved.
        
//
// This file was automatically generated from EventSpecs.xml and GenerateEventHeader.xsl
// *** DO NOT HAND-EDIT ***

// The VINCE code can be largely excluded from project compilation by omitting the preprocessor
// definition _VINCE_. However, the #define's in this header file still need to be processed
// so that inline Vince macros are properly disabled. 

//	The following list of logging calls may be inserted into the application code.
//  Use of the macro format allows these calls to be conditionally excluded by the compiler.
//  See the #define section for compiler flags that can be used disable compilation
//  collectively or selectively.
        
// VINCE_DECLARE;
// VINCE_INITIALIZE(pDevice);
// VINCE_SETTING(setting);
// VINCE_ASSIGN_DEVICE(pDevice);
// VINCE_SAVE_PRESENTATION_PARAMETERS(pParameters);
// VINCE_LOG_OPEN;
// VINCE_LOG_CYCLE;
// VINCE_LOG_CLOSE;
// VINCE_EVENT_ACTIVATE(event, active);
// VINCE_EVENT_ACTIVE(event);   (returns true/false)
// VINCE_LOG_TERMINATE;
// VINCE_SURVEY(QuestionID, Context);
// VINCE_SURVEY_ASYNC(QuestionID, Context);
// VINCE_SURVEY_UPDATE;
		
// VINCE_EVENT_Message(Message);

#pragma once
#define VINCE_MAX_EVENTS 1

// Any files that need to be included to access object definitions:
        

// Declaration and initialization macros
#ifdef _VINCE_
  // BENST - Changing a bunch of these macros in the interest of getting rid of the global Vince object dependency

  // #define VINCE_DECLARE Vince::CVince* g_pVince = new Vince::CVince() // BENST - not necessary, we don't want a global Vince object
  #define VINCE_INITIALIZE(pVince, pDevice) if (pVince) pVince->Initialize(pDevice)
  #define VINCE_SETTING(pVince, setting) if (pVince) pVince->pSettings->AddSetting(setting)
  #define VINCE_ASSIGN_DEVICE(pVince, pDevice) if (pVince) pVince->SetDevice(pDevice)
  #define VINCE_SAVE_PRESENTATION_PARAMETERS(pVince, pParameters)  if (pVince) pVince->SavePresentationParameters(pParameters)
  #define VINCE_SET_HWND(pVince, hWnd)  if (pVince) pVince->SetHWND(hWnd)
  #define VINCE_LOG_OPEN(pVince) if (pVince) pVince->pLogEvent->OpenLog(pVince)
  #define VINCE_LOG_CYCLE(pVince) if (pVince) pVince->pLogEvent->CycleLog(pVince)
  #define VINCE_LOG_CLOSE(pVince) if (pVince) pVince->pLogEvent->CloseLog(pVince)

  // BENST - These aren't needed for the Ensemble implementation of VINCE
  // #define VINCE_EVENT_ACTIVATE(pVince, event, active) if (g_pVince) g_pVince->pLogEvent->ActivateEvent(event, active)
  // #define VINCE_EVENT_ACTIVE(event) ((NULL != g_pVince) && g_pVince->pLogEvent->IsEventActive(event))
  // #define VINCE_TERMINATE if (g_pVince) { delete g_pVince; g_pVince = NULL; }

  #ifndef VINCE_EVENT
  #define VINCE_EVENT(pVince, event, params)  if (pVince) pVince->pLogEvent->event params
  #endif

  #ifndef VINCE_SURVEY
    #ifdef NO_VINCE_SURVEYS
      #define VINCE_SURVEY(question, context)
      #define VINCE_SURVEY_ASYNC(question, context)
      #define VINCE_SURVEY_UPDATE
      #define VINCE_SURVEY_MSGPROC(msg, wParam, lParam)
    #else
      #define VINCE_SURVEY(pVince, question, context)  if (pVince) pVince->pSurvey->Show(question, context)
      #define VINCE_SURVEY_ASYNC(pVince, question, context)  if (pVince) pVince->pSurvey->ShowAsync(question, context)
      #define VINCE_SURVEY_UPDATE(pVince) if (pVince) pVince->pSurvey->UpdateAsync()
      #define VINCE_SURVEY_MSGPROC(pVince, msg, wParam, lParam) if (pVince && pVince->pSurvey) { LRESULT res; bool processed = pVince->pSurvey->ProcessInputXuiPC(msg, wParam, lParam, &res); if(processed) return(res); }
    #endif
  #endif
#else
  #define VINCE_DECLARE
  #define VINCE_INITIALIZE(pDevice)
  #define VINCE_SETTING(setting)
  #define VINCE_ASSIGN_DEVICE(pDevice)
  #define VINCE_SAVE_PRESENTATION_PARAMETERS(pParameters)
  #define VINCE_SET_HWND(hWnd)
  #define VINCE_LOG_OPEN
  #define VINCE_LOG_CYCLE
  #define VINCE_LOG_CLOSE
  #define VINCE_EVENT_ACTIVATE(event, active)
  #define VINCE_EVENT_ACTIVE(event) false
  #define VINCE_TERMINATE
  #define VINCE_EVENT(event, params)
  #define VINCE_SURVEY(question, context)
  #define VINCE_SURVEY_ASYNC(question, context)
  #define VINCE_SURVEY_UPDATE
#endif // _VINCE_

// Compiler Switches:
        
#ifndef VINCE_EVENT_Generic
#define VINCE_EVENT_Generic(event, params) VINCE_EVENT(event, params)
#endif
        
    #ifndef VINCE_EVENT_Message
	#define VINCE_EVENT_Message(Message)  VINCE_EVENT_Generic(LogEvent_Message, (Message))
    #endif
    
//
// LogEvent Class Definitions
//

// We skip the rest if _VINCE_ is not defined

#ifdef _VINCE_

namespace Vince
{
	enum ActiveFlag
	{
         Active_Message,
	};

	class CVince;

    class LogEvent
    {
    protected:
        
		static void InitializeEvent(int iEventFlag, const char* cstrEventName, const char* cstrEventPath, bool fActive);
		static bool IsSubEvent(const char* cstrEventName, const char* cstrCheckPath);
		
		// benst - Making these static functions public to expose them to our Phoenix VinceWrapper
		//		   Why? Because these allows us to bypass the clunky auto-generated header stuff
		//		   and write out data to the events ourselves. This makes our hooks/events far
		//         easier to work with and more extensible.

		/*
		static bool StartEvent(int iEventFlag);
        static void SendParameter(const char* cstrParameterName, const char* ParameterValue);
        static void SendParameter(const char* cstrParameterName, const wchar_t* ParameterValue);
        static void SendParameter(const char* cstrParameterName, bool ParameterValue);
        static void SendParameter(const char* cstrParameterName, BYTE ParameterValue);
        static void SendParameter(const char* cstrParameterName, int ParameterValue);
        static void SendParameter(const char* cstrParameterName, DWORD ParameterValue);
        static void SendParameter(const char* cstrParameterName, long ParameterValue);
        static void SendParameter(const char* cstrParameterName, float ParameterValue);
        static void SendParameter(const char* cstrParameterName, double ParameterValue);
        static void SendEvent();
		*/

		static bool EventActive[VINCE_MAX_EVENTS];
		static const char* EventName[VINCE_MAX_EVENTS];
		static const char* EventPath[VINCE_MAX_EVENTS];

    public:

		static bool StartEvent(CVince* vince, int iEventFlag);
        static void SendParameter(CVince* vince, const char* cstrParameterName, const char* ParameterValue);
        static void SendParameter(CVince* vince, const char* cstrParameterName, const wchar_t* ParameterValue);
        static void SendParameter(CVince* vince, const char* cstrParameterName, bool ParameterValue);
        static void SendParameter(CVince* vince, const char* cstrParameterName, BYTE ParameterValue);
        static void SendParameter(CVince* vince, const char* cstrParameterName, int ParameterValue);
        static void SendParameter(CVince* vince, const char* cstrParameterName, DWORD ParameterValue);
        static void SendParameter(CVince* vince, const char* cstrParameterName, long ParameterValue);
        static void SendParameter(CVince* vince, const char* cstrParameterName, float ParameterValue);
        static void SendParameter(CVince* vince, const char* cstrParameterName, double ParameterValue);
        static void SendEvent(CVince* vince);

        LogEvent();
        ~LogEvent();
        void OpenLog(CVince* vince);
        void CycleLog(CVince* vince);
        void CloseLog(CVince* vince);

        static void ActivateEvent(const char* cstrEventName, bool fActivate);
        static bool IsEventActive(const char* cstrEventName);
        
        static void Initialize()
        {
            InitializeEvent( Active_Message, "Message", "Generic.", true );
        };
 
        // BENST - Removed because it isn't necessary for the Ensemble VINCE implementation
        /* static void LogEvent_Message(const char* Message)
        {
			if ( StartEvent( Active_Message ) )
			{
                SendParameter( "Message", Message );
                SendEvent();
            }
        }; */
    
    };

} // namespace Vince
        
#endif // _VINCE_

    