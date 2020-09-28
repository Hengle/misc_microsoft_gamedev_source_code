//	Survey : Survey data and processing
//
//	Created 2004/03/08 Rich Bonny <rbonny@microsoft.com>
//	Modified 2005/04/21 Rich Bonny
//         - Converted to use all Unicode
//	Refactored 2006/12/21 Dan Berke  <dberke@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2004 Microsoft Corp.  All rights reserved.

#pragma once

#include "ISurveyRenderer.h"
#include "ISurveyLogger.h"

#define MAX_QUESTIONS 100

namespace SurveyLib
{
	// Forward declaration
	class SurveyQuestion;

	class Survey
	{
		public:
					 Survey();
			virtual ~Survey();

			virtual HRESULT Initialize(ISurveyLogger* pLogger, ISurveyRenderer* pRenderer);
			virtual HRESULT LoadQuestions(const char* surveyFileName);

			virtual HRESULT Show(const wchar_t* wcsQuestionID, const wchar_t* wcsContext = L"");
			virtual HRESULT Render();
			virtual HRESULT ProcessInput();

			virtual HRESULT AddQuestion(SurveyQuestion* pSurveyQuestion);
			virtual bool    IsVisible();
			virtual bool	IsEnabled();
			virtual void	Enable();
			virtual void	Disable();

		protected:
			SurveyQuestion* FindQuestion(const wchar_t* wcsQuestionID);

			SurveyQuestion* m_pQuestions[MAX_QUESTIONS];
			int				m_questionCount;
			SurveyQuestion* m_pCurQuestion;
			const wchar_t*  m_wcsContext;

			ISurveyRenderer* m_pRenderer;
			ISurveyLogger*	 m_pLogger;   
			
			bool			 m_bEnabled;
    };
}