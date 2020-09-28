//	SurveyQuestion.cpp : Class to hold an individual survey question
//
//	Created 2004/03/08 Rich Bonny <rbonny@microsoft.com>
//	Modified 2005/04/21 Rich Bonny
//         - Converted to use all Unicode
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2004 Microsoft Corp.  All rights reserved.

#include "VinceControl.h"

#ifdef _VINCE_
#ifndef NO_VINCE_SURVEYS

#include "Vince.h"
#include "SurveyQuestion.h"

#include "TnTUtil.h"
#include "Display.h"

namespace Vince
{
	CSurveyQuestion::CSurveyQuestion( CVince* vince, const wchar_t* QuestionID )
	{
		m_pVince = vince;

		Properties = new CQuestionProperties(m_pVince);
		m_QuestionID = SAFE_COPY(QuestionID);
		m_DefaultBranch = NULL;
		m_AnswerCount = 0;
		m_Context = NULL;
		m_Question = NULL;
		ZeroMemory(m_UserText, sizeof(m_UserText));

		// Initialize to Multiple Choice by default
		QuestionType = MultipleChoice;
        m_bAnswered  = false;
        m_bCancelled = false;
        m_pPresenter = NULL;
        ZeroMemory(&m_bChecked, sizeof(m_bChecked));
	}

	CSurveyQuestion::~CSurveyQuestion(void)
	{
		// free up string allocations

		for (int i = 0; i < m_AnswerCount; i++)
		{
			SAFE_DELETE_ARRAY(m_Answers[i]);
			if ( m_BranchIDs[i] != m_DefaultBranch )
			{
				SAFE_DELETE_ARRAY(m_BranchIDs[i]);
			}
		}
		m_AnswerCount = 0;

		SAFE_DELETE_ARRAY(m_Question);
		SAFE_DELETE_ARRAY(m_QuestionID);
		SAFE_DELETE_ARRAY(m_DefaultBranch);

		delete Properties;
	}

    //---------------------------------------------------------------------------
    // Functions for building the question - used by CSurvey
    //---------------------------------------------------------------------------
	void CSurveyQuestion::SetQuestionText( const wchar_t* wcsQuestionText )
	{
		size_t length = wcslen(wcsQuestionText);
		wchar_t* wcsQuestion = new wchar_t[length + 1];
		wcsncpy( wcsQuestion, wcsQuestionText, length + 1 );

		// This should only be called once per question, but just to make sure
		// not to leak memory, delete prior value if not null.
		if ( NULL != m_Question )
		{
			SAFE_DELETE_ARRAY(m_Question);
		}
		m_Question = wcsQuestion;
	}

	// Set the type of question
	bool CSurveyQuestion::SetQuestionType( const char* csQuestionType )
	{
		size_t length = strlen(csQuestionType);
		bool bValid = true;
		if (length == 3)
		{
			if (0 == stricmp(csQuestionType, "=MC"))
			{
				QuestionType = MultipleChoice;
			}
			else if ( 0 == stricmp(csQuestionType, "=MS"))
			{
				QuestionType = MultiSelect;
			}
			else if ( 0 == stricmp(csQuestionType, "=HS"))
			{
				QuestionType = HorizontalScale;
			}
			else if ( 0 == stricmp(csQuestionType, "=FF"))
			{
				QuestionType = FreeformText;
			}
			else
			{
				bValid = false;
			}
		}
		else
		{
			bValid = false;
		}
		return bValid;
	}

	// Set the default branch to the next question to present
	void CSurveyQuestion::SetQuestionBranch( const wchar_t* wcsBranchID )
	{
		m_DefaultBranch = SAFE_COPY(wcsBranchID);
	}

	// Set the next question branch for a specific answer
	void CSurveyQuestion::SetAnswerBranch( const wchar_t* wcsBranchID )
	{
		m_BranchIDs[m_AnswerCount - 1] = SAFE_COPY(wcsBranchID);
	}

	void CSurveyQuestion::SetUserText( const wchar_t* wcsUserText )
	{
        if(wcsUserText == NULL)  {
            wcsncpy(m_UserText, L"", MAX_USER_TEXT - 1);
        } else {
            // Keep a copy of the text since we have no control over the lifetime of the source 
            // data pointer (in XUI it is freed when the question page is hidden)
            wcsncpy(m_UserText, wcsUserText, MAX_USER_TEXT - 1);
        }
	}

	// Add an answer to the end of the answer list
	void CSurveyQuestion::AddAnswer( const wchar_t* wcsAnswerText )
	{
		assert( m_AnswerCount < MAX_ANSWERS );
		size_t length = wcslen(wcsAnswerText);
		wchar_t* wcsAnswer = new wchar_t[length + 1];
		wcsncpy( wcsAnswer, wcsAnswerText, length + 1 );
		m_Answers[m_AnswerCount] = wcsAnswer;
		m_BranchIDs[m_AnswerCount] = m_DefaultBranch;
		m_AnswerCount++;
	}


    //---------------------------------------------------------------------------
    // Present the question on screen and wait until something is selected or
	// the user quits out.
    //---------------------------------------------------------------------------
	const wchar_t* CSurveyQuestion::AskQuestion(IQuestionPresenter* pPresenter, const wchar_t* wcsContext)
	{
		bool resultSelected = false;
		m_Context = wcsContext;

        // Clear the answer/cancel flags from any previous use of the question
		//  RWBFIX: Not sure why this is necessary. resultSelected seems adequate.
        m_bAnswered  = false;
        m_bCancelled = false;

        // Clear out answers
        m_SelectedAnswer = 0;
        ZeroMemory(&m_bChecked, sizeof(m_bChecked));

		if ( NULL != pPresenter )
		{
			resultSelected = pPresenter->PresentQuestion(this);
		}

		// Record selected results or cancelled questions to log file.

		if ( resultSelected )
		{
			LogAnswerResult();
			return GetNextQuestion(m_SelectedAnswer);
		}
		else
		{
			LogAnswerCancel();
			return NULL;
		}
	}


    //---------------------------------------------------------------------------
    // Functions for asynchronous question display and updating
    //---------------------------------------------------------------------------
    bool CSurveyQuestion::AskQuestionAsync(IQuestionPresenter* pPresenter, const wchar_t* wcsContext)
    {
		m_Context        = wcsContext;
        m_bAnswered      = false;
        m_bCancelled     = false;
        m_pPresenter     = pPresenter;

        // Clear out answers
        m_SelectedAnswer = 0;
        ZeroMemory(&m_bChecked, sizeof(m_bChecked));

        if(m_pPresenter != NULL) 
        {
            return(pPresenter->PresentQuestionAsync(this));
        }
        else 
        {
            return(false);
        }
    }
    
    const wchar_t* CSurveyQuestion::UpdateAsync(bool& bDone)
    {
        bDone = false;

        if(m_pPresenter != NULL) 
        {
            m_pPresenter->UpdateAsync();

            // If question was answered, hide the question 
            if(m_bAnswered) 
            {
                m_pPresenter->HideQuestion();
			    LogAnswerResult();
                bDone = true;
			    return(GetNextQuestion(m_SelectedAnswer));
            }
 
            // If question was cancelled, hide the question and return false
            if(m_bCancelled) 
            {
                m_pPresenter->HideQuestion();
			    LogAnswerCancel();
                bDone = true;
                return(NULL);
            }
        }

        return(NULL);
    }
	
    //---------------------------------------------------------------------------
    // Question properties - used by IQuestionPresenter classes to determine what to draw
    //---------------------------------------------------------------------------
	// Return wide character text of question
	const wchar_t* CSurveyQuestion::GetQuestionText(void) const
	{
		return m_Question;
	}

	// Return wide character answer text
	const wchar_t* CSurveyQuestion::GetAnswerText(int AnswerNumber) const
	{
		return m_Answers[AnswerNumber];
	}

	// Return string identifier for current question
	const wchar_t* CSurveyQuestion::GetQuestionID(void) const
	{
		return m_QuestionID;
	}

	// Return total number of answers for this question
	int CSurveyQuestion::GetAnswerCount(void) const
	{
		return m_AnswerCount;
	}

	// Return question ID to branch to if this answer is selected
	const wchar_t* CSurveyQuestion::GetNextQuestion(int AnswerNumber) const
	{
		return m_BranchIDs[AnswerNumber];
	}

	// Write the response to a question as a log file event
	void CSurveyQuestion::LogAnswerResult()
	{
		// How we handle this depends on the question type
		switch (QuestionType)
		{
		case MultipleChoice:
		case HorizontalScale:
			LogSingleAnswer( m_SelectedAnswer, m_Answers[m_SelectedAnswer] );
			break;

		case FreeformText:
			LogSingleAnswer( m_SelectedAnswer, m_UserText);
			break;

		case MultiSelect:
			int iChecked = 0;
			for (int iAnswer = 0; iAnswer < m_AnswerCount; iAnswer++)
			{
				if ( m_bChecked[iAnswer] )
				{
					LogSingleAnswer( iAnswer, m_Answers[iAnswer] );
					iChecked++;
				}
			}
			if ( 0 == iChecked )
			{
				LogSingleAnswer( m_AnswerCount + 1, L"No Answers Selected" );
			}
			break;
		}
	}

	// BENST - Changed to take a member Vince object so that global Vince object is not necessary
	void CSurveyQuestion::LogSingleAnswer( int AnswerNumber, const wchar_t* wcsAnswerText )
	{
        // This does the actual writing to the log file
		m_pVince->pLogWriter->WriteEventTag("SurveyAnswer");
		m_pVince->pLogWriter->WriteParameter("Question", "string", m_Question);
		m_pVince->pLogWriter->WriteParameter("QuestionID", "string", m_QuestionID);
		m_pVince->pLogWriter->WriteParameter("Answer", "string", wcsAnswerText);
		m_pVince->pLogWriter->WriteParameter("AnswerNumber", "int", AnswerNumber);
		m_pVince->pLogWriter->WriteParameter("Context", "string", m_Context);
		m_pVince->pLogWriter->WriteEventTail();
	}

	// BENST - Changed to take a member Vince object so that global Vince object is not necessary
	// Cancel out of question and report accordingly to the log file
	void CSurveyQuestion::LogAnswerCancel()
	{
		m_pVince->pLogWriter->WriteEventTag("SurveyAnswer");
		m_pVince->pLogWriter->WriteParameter("Question", "string", m_Question);
		m_pVince->pLogWriter->WriteParameter("QuestionID", "string", m_QuestionID);
		m_pVince->pLogWriter->WriteParameter("Answer", "string", L"");
		m_pVince->pLogWriter->WriteParameter("AnswerNumber", "int", -1);
		m_pVince->pLogWriter->WriteParameter("Context", "string", m_Context);
		m_pVince->pLogWriter->WriteEventTail();
	}

}

#endif // !NO_VINCE_SURVEYS
#endif // _VINCE_