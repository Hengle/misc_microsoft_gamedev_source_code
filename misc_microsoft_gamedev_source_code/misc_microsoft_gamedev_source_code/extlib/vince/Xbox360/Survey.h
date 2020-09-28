//	Survey.h : Class to hold survey questions
//
//	Created 2004/03/08 Rich Bonny <rbonny@microsoft.com>
//	Modified 2005/04/21 Rich Bonny
//         - Converted to use all Unicode
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2004 Microsoft Corp.  All rights reserved.


#pragma once

#include "SurveyQuestion.h"
#include "IQuestionPresenter.h"

#include <stdio.h>

#define MAX_QUESTIONS 100

namespace Vince
{
	enum FileLoadState
	{
		State_Empty,
		State_QuestionID,
		State_Question,
		State_Answer,
		State_Error
	};

	enum InputLineType
	{
		Type_Blank,
		Type_Label,
		Type_QuestionType,
		Type_Branch,
		Type_Text,
		Type_Setting,
		Type_Error
	};

	class CSurvey
	{
	public:
		CSurvey( CVince* vince );
		~CSurvey(void);

		bool Load();
		bool Show(const wchar_t* wcsQuestionID );
		bool Show(const char* cstrQuestionID, const char* cstrContext );
		bool Show(const wchar_t* wcsQuestionID, const wchar_t* wcsContext );
		bool ShowAsync(const char* cstrQuestionID, const char* cstrContext);
		bool ShowAsync(const wchar_t* wcsQuestionID, const wchar_t* wcsContext );
        void UpdateAsync();
		void BuildSummary();
		void ShowSummary();
		CSurveyQuestion* FindQuestion(const wchar_t* wcsQuestionID );
        bool IsVisible() { return(m_pPresenter != NULL && m_pCurQuestion != NULL); }
        void SetSurveyPos(float x, float y);
        void SetSurveyScale(float scaleX, float scaleY);

#ifndef _XBOX
        bool ProcessInputXuiPC(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *pResult); 
#endif

	protected:
		CVince* m_pVince;

		const wchar_t* PresentQuestion(IQuestionPresenter* pPresenter, const wchar_t* wcsQuestionID, const wchar_t* wcsContext);
		void AddQuestion(CSurveyQuestion* pSurveyQuestion);
		int QuestionCount(void) const;
		static InputLineType GetLineType(const wchar_t* cstrQuestionID);
		FILE* OpenSurveyFile(const char* cstrSurveyFileName);
		bool ReadNextLine(FILE* fSurvey, wchar_t* wcsLine, char* csLine);

        void SelectPresenter();
		CSurveyQuestion* Questions[MAX_QUESTIONS];
		int m_QuestionCount;
		bool m_fUnicode;
        CSurveyQuestion*    m_pCurQuestion;
        IQuestionPresenter* m_pPresenter;
        const wchar_t*      m_wcsContext;
	};
}