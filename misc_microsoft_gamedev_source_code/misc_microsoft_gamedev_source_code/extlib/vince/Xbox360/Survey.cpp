//	Survey.h : Class to hold survey questions
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
#include "Survey.h"
#include "QuestionProperties.h"
#include "QuestionPresenter.h"
#include "XuiQuestionPresenter.h"
#include "TnTUtil.h"
#include "VinceUtil.h"
#include "Display.h"
#include "Controller.h"

#include <stdio.h>

//// Temporary fix for broken crt function on Xenon
#if defined(_XBOX) && _XBOX_VER >= 200 // Xenon
	wchar_t* fgetws_temp(wchar_t* wcsLine, int count, FILE* file)
	{
		wchar_t* wcsReturn = wcsLine;
		wchar_t nextChar = NULL;

		while (--count)
		{
			if ( feof(file) )
			{
				if ( wcsReturn == wcsLine)
				{
					wcsReturn = NULL;
				}
				break;
			}
			else if (NULL != fgetws(&nextChar, 2, file) )
			{
				nextChar = (wchar_t)_byteswap_ushort(nextChar);
				*wcsReturn++ = nextChar;
			}

			if ( nextChar == L'\n' )
				break;
		}
		if ( NULL != wcsReturn)
		{
			*wcsReturn = L'\0';
		}
		return wcsReturn;
	}
#endif
//// - End of temp fix

namespace Vince
{
	const int MAX_LINE_SIZE = 256;
	const int MAX_LABEL_SIZE = 64;

	CSurvey::CSurvey(CVince* vince)
	{
		m_pVince = vince;

		m_QuestionCount = 0;
        m_pPresenter    = NULL;
        m_pCurQuestion  = NULL;
        m_wcsContext    = NULL;
	}

	CSurvey::~CSurvey(void)
	{
		for ( int i = 0; i < m_QuestionCount; i++ )
		{
			SAFE_DELETE(Questions[i]);
		}
		m_QuestionCount = 0;

        SAFE_DELETE(m_pPresenter);

		// Destroy Controller and Display objects
		CDisplay::DestroyInstance();
		CController::DestroyInstance();
	}

	// Load a survey from the  specification file
	bool CSurvey::Load()
	{
		// First check to see if Surveys are enabled and, if not, exit immediately

		bool fPresentSurveys = m_pVince->pSettings->Fetch("PresentSurveys", true);

		if (!fPresentSurveys)
		{
			return false;
		}

		// Get the name of the survey question file from the config settings

		const char* cstrSurveyFileName = m_pVince->pSettings->Fetch("SurveyFileName", "SurveyQuestions.txt");

		// Open the file

		FILE* fSurvey = OpenSurveyFile(cstrSurveyFileName);
		bool bValidFile = false;
		int iLineCount = 0;
		int iBlankLineCount = 0;
		wchar_t wcsLine[MAX_LINE_SIZE] = L"";
		wchar_t contents[MAX_LABEL_SIZE + 1] = L"";
		char csLine[MAX_LINE_SIZE] = "";
		CSurveyQuestion* pCurrentQuestion = NULL;

		if (fSurvey) 
		{
			// The load process is a state model and expects certain kinds of input depending
			// on where it thinks it is in the file.

			FileLoadState loadState;
			loadState = State_Empty;
			bValidFile = true;
			while ( bValidFile && !feof(fSurvey) ) 
			{
				size_t length = 0;

				// Read depends on whether this is a Unicode file or not
				if (m_fUnicode)
				{
// Workaround for broken Xenon XDK
#if defined(_XBOX) && _XBOX_VER >= 200 // Xenon
					if ( NULL == fgetws_temp(wcsLine, MAX_LINE_SIZE-1, fSurvey) )
#else
					if ( NULL == fgetws(wcsLine, MAX_LINE_SIZE-1, fSurvey) )
#endif
					{
						break;
					}
					else
					{
						length = wcslen(wcsLine);
						if (length > 2)
						{
							// Trim end of line: for Unicode, this is CR+LF
							wcsLine[length - 2] = L'\0';
							length -= 2;
							const char* csTemp = TnT::MakeSingle(wcsLine);
							strncpy(csLine, csTemp, length + 1);
							SAFE_DELETE(csTemp);
						}
						else	// Treat as blank line
						{
							length = 0;
						}
					}
				}
				else	// ASCII survey file
				{
					if ( NULL == fgets(csLine, MAX_LINE_SIZE-1, fSurvey) )
					{
						break;
					}
					else
					{
						length = strlen(csLine);
						if (length > 2)
						{
							// Trim end of line: for ASCII, this is just LF
							csLine[length - 1] = L'\0';
							length--;
							TnT::CopyWide(csLine, wcsLine, length + 1);
						}
						else	// Treat as blank line
						{
							length = 0;
						}
					}
				}

				if ( NULL == wcsLine )
				{
					break;
				}

				length = wcslen(wcsLine);
				iLineCount++;

				InputLineType lineType = GetLineType( wcsLine );
				if ( Type_Branch == lineType || Type_Label == lineType )
				{
					// Make sure content portion is not too large
					if ( length > MAX_LABEL_SIZE + 2)
					{
						m_pVince->pLogWriter->WriteError("Survey", "Load", "Label or branch name is too long");
						loadState = State_Error;
						bValidFile = false;
					}
					else
					{
						// trim off leading token
						wcsncpy( contents, wcsLine + 1, length - 2 );
						// trim off trailing token
						contents[length - 2] = L'\0';	
					}
				}

				if (Type_Blank == lineType)
					// ignore blanks
					iBlankLineCount++;

				else if ( Type_Error == lineType)
					bValidFile = false;

				else if ( Type_Setting == lineType)
					bValidFile = m_pVince->pSettings->AddSetting( csLine + 1 );	// Pass ASCII version

				else switch (loadState)
				{
					case State_Empty:
						{
							switch (lineType)
							{
								case Type_Branch:
									bValidFile = false;
									break;
								case Type_Label:
									pCurrentQuestion = new CSurveyQuestion( m_pVince, contents );
									loadState = State_QuestionID;
									break;
								case Type_Text:
									bValidFile = false;
									break;
								case Type_QuestionType:
									bValidFile = false;
									break;
							}
						}
						break;

					case State_QuestionID:
						{
							switch (lineType)
							{
								case Type_Branch:
									bValidFile = false;
									break;
								case Type_Label:
									bValidFile = false;
									break;
								case Type_Text:
									pCurrentQuestion->SetQuestionText( wcsLine );
									loadState = State_Question;
									break;
								case Type_QuestionType:
									bValidFile = false;
									break;
							}
						}
						break;

					case State_Question:
						{
							switch (lineType)
							{
								case Type_Branch:
									pCurrentQuestion->SetQuestionBranch( contents );
									break;
								case Type_Label:
									bValidFile = false;
									break;
								case Type_Text:
									pCurrentQuestion->AddAnswer( wcsLine );
									loadState = State_Answer;
									break;
								case Type_QuestionType:
									bValidFile = pCurrentQuestion->SetQuestionType( csLine );
									break;
							}
						}
						break;

					case State_Answer:
						{
							switch (lineType)
							{
								case Type_Branch:
									pCurrentQuestion->SetAnswerBranch( contents );
									break;
								case Type_Label:
									AddQuestion( pCurrentQuestion );
									pCurrentQuestion = new CSurveyQuestion( m_pVince, contents );
									loadState = State_QuestionID;
									break;
								case Type_Text:
									pCurrentQuestion->AddAnswer( wcsLine );
									break;
								case Type_QuestionType:
									bValidFile = false;
									break;
							}
						}
						break;
				}
			}
			fclose(fSurvey);
			if ( bValidFile )
			{
				AddQuestion( pCurrentQuestion );
				return true;
			}
			else
			{
				char buffer[256];
				_snprintf(buffer, 255, "Error in survey file at line %d: %s", iLineCount, csLine);
				buffer[255] = '\0';
				m_pVince->pLogWriter->WriteError("Survey", "Load", buffer);
				SAFE_DELETE(pCurrentQuestion);
				return false;
			}

		}
		else
		{
			m_pVince->pLogWriter->WriteError("Survey", "Load", "Could not load survey question file");
			return false;
		}
	}

	// Open the survey file and determine if it is in Unicode or ASCII format
	FILE* CSurvey::OpenSurveyFile(const char* cstrSurveyFileName)
	{
		FILE* fSurvey = VinceFileOpen(cstrSurveyFileName, "rb", false);
		m_fUnicode = false;
		if (NULL != fSurvey)
		{
			wchar_t testChars[2];
			if (NULL != fgetws(testChars, 2, fSurvey))
			{
// Workaround for broken Xenon XDK
#if defined(_XBOX) && _XBOX_VER >= 200 // Xenon
			if ( 0xfffe == testChars[0] )
#else
			if ( 0xfeff == testChars[0] )
#endif
				{
					m_fUnicode = true;
					return fSurvey;
				}
			}
			fclose(fSurvey);
		}
		fSurvey = VinceFileOpen(cstrSurveyFileName, "rt", false);
		m_fUnicode = false;
		return fSurvey;
	}

	bool CSurvey::Show(const wchar_t* wcsQuestionID)
	{
		// If no context provided, just pass context as blank

		return Show(wcsQuestionID, L"");
	}

	// overload for passing single byte char array arguments
	bool CSurvey::Show(const char* cstrQuestionID, const char* cstrContext )
	{
		const wchar_t* wcsQuestionID = TnT::MakeWide(cstrQuestionID);
		const wchar_t* wcsContext = TnT::MakeWide(cstrContext);

		bool result = Show(wcsQuestionID, wcsContext);
		SAFE_DELETE_ARRAY(wcsQuestionID);
		SAFE_DELETE_ARRAY(wcsContext);
		return result;
	}

	// Show this question and continue showing questions as long as the
	// links continue to point to additional questions.
	bool CSurvey::Show(const wchar_t* wcsQuestionID, const wchar_t* wcsContext)
	{
        // Select the presenter to use
		CDisplay::Instance()->Begin(m_pVince);
        SelectPresenter();

		if ( NULL != m_pPresenter )
		{
			m_pPresenter->BeginSurvey();
		    const wchar_t* wcsNextQuestionID = wcsQuestionID;
			while ( NULL != wcsNextQuestionID )
			{
				wcsNextQuestionID = PresentQuestion( m_pPresenter, wcsNextQuestionID, wcsContext );
			}
			m_pPresenter->EndSurvey();
			delete m_pPresenter;
			m_pPresenter = NULL;
			return true;
		}
		else
        {
            return false;
        }
    }

	// overload for passing single byte char array arguments
	bool CSurvey::ShowAsync(const char* cstrQuestionID, const char* cstrContext)
	{
		const wchar_t* wcsQuestionID = TnT::MakeWide(cstrQuestionID);
		const wchar_t* wcsContext = TnT::MakeWide(cstrContext);

		bool result = ShowAsync(wcsQuestionID, wcsContext);
		SAFE_DELETE_ARRAY(wcsQuestionID);
		SAFE_DELETE_ARRAY(wcsContext);
		return result;
	}

	bool CSurvey::ShowAsync(const wchar_t* wcsQuestionID, const wchar_t* wcsContext)
	{
        // Don't show a survey while another is being shown
        if(m_pCurQuestion != NULL) 
        {
            return(false);
        }

        // Select the presenter to use
        SelectPresenter();

        // Let the question presenter know the survey is about to start
        m_pPresenter->BeginSurvey();

        // Get first question        
		m_pCurQuestion = FindQuestion(wcsQuestionID);
		if(m_pCurQuestion != NULL) 
        {
            m_wcsContext = wcsContext;
			bool ok = m_pCurQuestion->AskQuestionAsync(m_pPresenter, wcsContext);
            if(!ok) 
            {
                m_pCurQuestion = NULL;
                return(false);
            }
        }

		return(true);
	}

	void CSurvey::UpdateAsync()
    {
        if(m_pCurQuestion != NULL) 
        {
            // Update the question (renders it and checks to see if it was answered)
            bool questionChange;
            const wchar_t* nextQuestion = m_pCurQuestion->UpdateAsync(questionChange);

            // If the question has changed (either by being answered or cancelled)...
            if(questionChange) 
            {
                // We've reached the end of the survey, or the question was cancelled
                if(nextQuestion == NULL) 
                {
                    m_pPresenter->EndSurvey();
					delete m_pPresenter;
					m_pPresenter = NULL;
                    m_pCurQuestion = NULL;
                } 
                else 
                {
                    // Get the next question and ask it asynchronously
            		m_pCurQuestion = FindQuestion(nextQuestion);
			        bool ok = m_pCurQuestion && m_pCurQuestion->AskQuestionAsync(m_pPresenter, m_wcsContext);       
                    if(!ok) 
                    {
                        m_pPresenter->EndSurvey();
	 					delete m_pPresenter;
						m_pPresenter = NULL;
                        m_pCurQuestion = NULL;
                    }
                }
            }
        }
    }

    // Gets a pointer to the presentation object to be used
    void CSurvey::SelectPresenter()
    {
        if(m_pPresenter == NULL) 
        {
            // If not compiled with XUI support, ignore the UseXUI flag
#ifdef VINCE_XUI
		    bool UseXUI = m_pVince->pSettings->Fetch("UseXUI", false);
		    if (UseXUI)
		    {
			    m_pPresenter = new XuiQuestionPresenter();
		    }
		    else
#endif
		    {
 			    m_pPresenter = new QuestionPresenter(m_pVince);
		    }
        }
		// Initialize the presenter by passing it the device pointer and presentation parameters
		// For now, we get these from the display object.
		// Get pointer to singleton display object:
		CDisplay* pDisplay = Vince::CDisplay::Instance();
		if (NULL != pDisplay && NULL != m_pPresenter)
		{
			LPDIRECT3DDEVICE9 pDevice = (LPDIRECT3DDEVICE9) (pDisplay->GetDevice());			
			D3DPRESENT_PARAMETERS* pParameters = (D3DPRESENT_PARAMETERS*) (pDisplay->GetPresentationParameters());			
			HWND hWnd = pDisplay->GetHWND();
			m_pPresenter->InitDevice( pDevice, pParameters, hWnd );
		}
	}

	// Find the question object and ask it.
	const wchar_t* CSurvey::PresentQuestion(IQuestionPresenter *pDisplay, const wchar_t* wcsQuestionID, const wchar_t* wcsContext)
	{
		CSurveyQuestion* pQuestion = FindQuestion( wcsQuestionID );
        m_pCurQuestion = pQuestion;
		if ( NULL != pQuestion )
		{
			return pQuestion->AskQuestion(pDisplay, wcsContext);
		}
		else
		{
			char buffer[MAX_LABEL_SIZE + 1];
			_snprintf(buffer, MAX_LABEL_SIZE, "Could not find requested survey question: %s ", wcsQuestionID);	// UNIFIX?
			buffer[MAX_LABEL_SIZE] = '\0';
			m_pVince->pLogWriter->WriteError("Survey", "PresentQuestion", buffer);
			return NULL;
		}
	}

    void CSurvey::SetSurveyPos(float x, float y)
    {
        if(m_pPresenter != NULL) {
            m_pPresenter->SetSurveyPos(x, y);
        }
    }

    void CSurvey::SetSurveyScale(float scaleX, float scaleY)
    {
        if(m_pPresenter != NULL) {
            m_pPresenter->SetSurveyScale(scaleX, scaleY);
        }
    }


	// Parsing helper function. Determines what kind of line
	// this is: Blank, a config setting, a label, a branch, or text.
	InputLineType CSurvey::GetLineType(const wchar_t* inputLine)
	{
		size_t length = wcslen(inputLine);
		if ( 0 == length )
		{
			return Type_Blank;
		}
		else if ( L' ' == inputLine[0] || L'\t' == inputLine[0] )
		{
			return Type_Blank;
		}
		else if ( L':' == inputLine[0] )
		{
			return Type_Setting;
		}
		else if ( L'=' == inputLine[0] )
		{
			return Type_QuestionType;
		}
		else if ( L'[' == inputLine[0] )
		{
			if ( L']' == inputLine[length - 1] )
				return Type_Label;
			else
				return Type_Error;
		}
		else if ( L'<' == inputLine[0] )
		{
			if ( L'>' == inputLine[length - 1] )
				return Type_Branch;
			else
				return Type_Error;
		}
		else
		{
			return Type_Text;
		}
	}

	// Build the array of question objects
	void CSurvey::AddQuestion(CSurveyQuestion* pSurveyQuestion)
	{
		if ( m_QuestionCount < MAX_QUESTIONS )
		{
			if (NULL != pSurveyQuestion)
			{
				Questions[m_QuestionCount] = pSurveyQuestion;
				m_QuestionCount++;
			}
		}
		else
		{
			m_pVince->pLogWriter->WriteError("Survey", "AddQuestion", "Maximum question count exceeded");
			SAFE_DELETE(pSurveyQuestion);
		}
	}

	// Locate a question object based on its ID string
	CSurveyQuestion* CSurvey::FindQuestion(const wchar_t* wcsQuestionID)
	{
		// Search throught the array of question objects for one matching
		// the question ID. This is a brute force search, but since there
		// should not be a huge number of questions and this is not performance
		// critical code, that should be okay

		for ( int i = 0; i < m_QuestionCount; i++)
		{
			if ( 0 == wcscmp(wcsQuestionID, Questions[i]->GetQuestionID() ) )
			{
				return Questions[i];
			}
		}
		return NULL;
	}

	// Create a wrapper set of questions to branch to all survey questions,
	// kind of like a table of contents. This code should only be used by
	// the VINCE survey preview application.
	void CSurvey::BuildSummary()
	{
		// First let's restore default display settings:
		m_pVince->pSettings->AddSetting( "SurveyFont=Tahoma" );
		m_pVince->pSettings->AddSetting( "QuestionColor=0xffffff00" );
		m_pVince->pSettings->AddSetting( "SelectedAnswerColor=0xffffffff" );
		m_pVince->pSettings->AddSetting( "AnswerColor=0xff7f7f7f" );
		m_pVince->pSettings->AddSetting( "FooterColor=0xffffffff" );
		m_pVince->pSettings->AddSetting( "BackgroundColor=0xff000020" );
		m_pVince->pSettings->AddSetting( "BackgroundImage=" );

		// Figure out how many pages we need to show all the survey questions
		const int LINKS_PER_PAGE = 6;
		int cQuestionTotal = QuestionCount();
		wchar_t wcsSummaryID[10] = L"QSummary1";
		int iCurrentPageCount = 0;
		CSurveyQuestion* pSummaryQuestion = NULL;


		// Add each existing question to the summary questions
		for ( int i = 0; i < cQuestionTotal; i++ )
		{
			// If we do not have a page created, make one
			if ( 0 == iCurrentPageCount)
			{
				pSummaryQuestion = new CSurveyQuestion( m_pVince, wcsSummaryID );
				pSummaryQuestion->SetQuestionText(L"Choose a Survey Question to View");
				AddQuestion( pSummaryQuestion );

				// First answer is actually link to next page.
				// ( If more than LINKS_PER_PAGE total questions)
				{
					pSummaryQuestion->AddAnswer(L"Next Page of Questions");
					if ( i == cQuestionTotal -1 )
					{
						pSummaryQuestion->SetAnswerBranch( L"QSummary1" );
					}
					else
					{
						wcsSummaryID[8]++;
						pSummaryQuestion->SetAnswerBranch( wcsSummaryID );
					}
				}
			}

			pSummaryQuestion->AddAnswer( Questions[i]->GetQuestionID() );
			pSummaryQuestion->SetAnswerBranch( Questions[i]->GetQuestionID() );
			iCurrentPageCount++;

			// If this page is finished, add the quit branch

			if ( (LINKS_PER_PAGE == iCurrentPageCount) || (i == cQuestionTotal - 1) )
			{
				pSummaryQuestion->AddAnswer( L"Quit Survey Viewer" );
				pSummaryQuestion->SetAnswerBranch( L"**QUIT**" );
				iCurrentPageCount = 0;
			}

		}
	}

	// Display the summary survey.  This code should only be used by
	// the VINCE survey preview application.
	void CSurvey::ShowSummary()
	{
        // Select the presenter to use
        SelectPresenter();
		
		// This is similar to Show(), except we only exit to the table of contents from
		// any normal question

		const wchar_t* wcsNextQuestionID = L"QSummary1";
		bool looping = true;
		bool finished = false;

		// No point if there are no questions.
		if ( 0 == m_QuestionCount )
			finished = true;

		while ( !finished )
		{
			wcsNextQuestionID = L"QSummary1";
			looping = true;

			while ( looping )
			{
				wcsNextQuestionID = PresentQuestion( m_pPresenter, wcsNextQuestionID, L"" );
				if ( NULL == wcsNextQuestionID )
				{
					looping = false;
				}
				else if ( 0 == wcscmp(wcsNextQuestionID, L"**QUIT**" ) )
				{
					finished = true;
				}
			}
		}
	}

	// Return current number of total questions
	int CSurvey::QuestionCount(void) const
	{
		return m_QuestionCount;
	}

#ifndef _XBOX
    bool CSurvey::ProcessInputXuiPC(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *pResult)
    {
        if(m_pCurQuestion != NULL && m_pPresenter != NULL) {
            return(m_pPresenter->ProcessInputMessage(msg, wParam, lParam, pResult));
        }

        return(false);
    }
#endif

}

#endif // !NO_VINCE_SURVEYS
#endif // _VINCE_