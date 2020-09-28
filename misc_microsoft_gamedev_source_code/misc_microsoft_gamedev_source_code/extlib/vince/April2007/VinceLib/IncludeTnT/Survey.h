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
#include "ISurveyQuestion.h"
#include "SurveyError.h"

#define MAX_QUESTIONS 100

namespace SurveyLib
{
	class Survey
	{
		public:
					 Survey();
			virtual ~Survey();

			virtual HRESULT Initialize(ISurveyLogger* pLogger, ISurveyRenderer* pRenderer);
			virtual HRESULT LoadQuestions(const char* surveyFileName);

			virtual HRESULT StartSurvey(const wchar_t* wcsQuestionID, const wchar_t* wcsContext = L"");
			virtual HRESULT Render();
			virtual HRESULT ProcessInput(UINT keyCode);

			virtual HRESULT AddQuestion(ISurveyQuestion* pSurveyQuestion);
			virtual bool    IsVisible();
			virtual bool	IsEnabled();
			virtual void	Enable();
			virtual void	Disable();

		protected:
			ISurveyQuestion*  FindQuestion(const wchar_t* wcsQuestionID);

			ISurveyQuestion*  m_pQuestions[MAX_QUESTIONS];
			int				  m_questionCount;
			ISurveyQuestion*  m_pCurQuestion;
			const wchar_t*    m_wcsContext;

			ISurveyRenderer* m_pRenderer;
			ISurveyLogger*	 m_pLogger;   
			
			bool			 m_bEnabled;
    };
}