//	SurveyQuestion.h : Class to hold an individual survey question
//
//	Created 2004/03/08 Rich Bonny <rbonny@microsoft.com>
//	Modified 2005/04/21 Rich Bonny
//         - Converted to use all Unicode
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2004 Microsoft Corp.  All rights reserved.


#pragma once

#include "LogWriter.h"
#include "QuestionProperties.h"
#include "IQuestionPresenter.h"

#define MAX_ANSWERS     10
#define MAX_USER_TEXT   1024

namespace Vince
{
	class CSettings;

	enum EQuestionType
	{
		MultipleChoice,
		MultiSelect,
		FreeformText,
		HorizontalScale
	};

	class CSurveyQuestion 
	{
	public:
		CSurveyQuestion( CVince* vince, const wchar_t* QuestionID );
		~CSurveyQuestion(void);

        // Functions for building the question - used by CSurvey
        void SetQuestionText( const wchar_t* wcsQuestionText );
		bool SetQuestionType( const char* csQuestionType );
		void SetQuestionBranch( const wchar_t* wcsBranchID );
		void SetAnswerBranch( const wchar_t* wcsBranchID );
		void SetUserText( const wchar_t* wcsUserText );
		void AddAnswer(const wchar_t* AnswerText);

        // Question display
		const wchar_t* AskQuestion(IQuestionPresenter* pPresenter, const wchar_t* wcsContext);
        
        // Functions for asynchronous question display and updating
        bool AskQuestionAsync(IQuestionPresenter* pPresenter, const wchar_t* wcsContext);
        const wchar_t* UpdateAsync(bool& bDone);

        // Question properties - used by IQuestionPresenter classes to determine what to draw
        const wchar_t* GetQuestionText(void) const;
		const wchar_t* GetAnswerText(int AnswerNumber) const;
		const wchar_t* GetQuestionID(void) const;
		int            GetAnswerCount(void) const;
        const wchar_t* GetNextQuestion(int AnswerNumber) const;

        void SetSelectedAnswer(int answerNum) { m_SelectedAnswer = answerNum; }
        void SetChecked(int answerNum, bool state) { m_bChecked[answerNum] = state; }

        // Question answer state
        bool WasAnswered()               { return(m_bAnswered);     }
        bool WasCancelled()              { return(m_bCancelled);    }
        void SetAnswered() 
        { 
            m_bAnswered  = true;
            m_bCancelled = false;
        }
        void SetCancelled() 
        { 
            m_bCancelled = true;
            m_bAnswered  = false;
        }

	protected:
		// BENST - Added to remove dependency on global Vince object
		CVince* m_pVince;

		void LogAnswerResult();
		void LogSingleAnswer( int AnswerNumber, const wchar_t* wcsAnswerText );
		void LogAnswerCancel();

		int m_SelectedFont;
		const wchar_t* m_Question;
		const wchar_t* m_QuestionID;
		const wchar_t* m_DefaultBranch;
		const wchar_t* m_Answers[MAX_ANSWERS];
		const wchar_t* m_BranchIDs[MAX_ANSWERS];
		const wchar_t* m_Context;
  
		// Basic Question Info
		int m_AnswerCount;
		int m_SelectedAnswer;
        bool m_bAnswered;
        bool m_bCancelled;
        bool m_bChecked[MAX_ANSWERS];   // Check states for multiple-select questions
        wchar_t m_UserText[MAX_USER_TEXT];

        IQuestionPresenter* m_pPresenter;

		// Pointers to public data members
	public:
		// BENST - changing this to a pointer to support removal of Vince global object
		CQuestionProperties* Properties;
		EQuestionType QuestionType;

	};
}
