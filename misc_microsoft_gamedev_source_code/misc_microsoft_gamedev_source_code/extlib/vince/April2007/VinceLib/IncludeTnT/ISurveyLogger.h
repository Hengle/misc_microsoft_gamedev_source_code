//	ISurveyLogger : Interface to something that logs survey results
//
//	Created 2006/012/04 Rich Bonny <rbonny@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2004 Microsoft Corp.  All rights reserved.

#pragma once

#ifdef _XBOX
	#include <xtl.h>
#else
	#include <windows.h>
#endif

namespace SurveyLib
{
	class ISurveyLogger
	{
	public:
        virtual ~ISurveyLogger() {};

		// Refcounting
		virtual void AddRef()  = 0;
		virtual void Release() = 0;

        // Report an answer from survey
        virtual HRESULT ReportSurveyAnswer(const wchar_t* wcsQuestion, const wchar_t* wcsQuestionID, const wchar_t* wcsAnswer,
                                           int answerNumber, const wchar_t* wcsContext) = 0;	
	};
}