//	QuestionPresenter.cpp : Default survey question presenter logic
//
//	Created 2005/09/08 Rich Bonny <rbonny@microsoft.co13
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2004 Microsoft Corp.  All rights reserved.

#include "VinceControl.h"

#ifdef _VINCE_

#include "QuestionPresenter.h"
#include "SurveyQuestion.h"
#include "Display.h"
#include "Controller.h"
#include "TnTUtil.h"

namespace Vince
{
	// Constructor
	QuestionPresenter::QuestionPresenter(CVince* vince)
    {
		m_pVince = vince;

		m_pDisplay = NULL;
		m_pController = NULL;
	}

	// Destructor
	QuestionPresenter::~QuestionPresenter()
    {
    }

	// Only initialization is to save a copy of the display and control pointers
	// Disable the warning about unreferenced parameters since those are only used by XUI
#pragma warning(push)
#pragma warning(disable :4100)
	HRESULT QuestionPresenter::InitDevice(IDirect3DDevice9 *pD3DDevice, D3DPRESENT_PARAMETERS *pD3Dpp, HWND hWnd)
    {
		m_pDisplay = Vince::CDisplay::Instance();
		m_pController = Vince::CController::Instance();
		return S_OK;
    }
#pragma warning(pop)

	void QuestionPresenter::BeginSurvey() 
	{
		m_pDisplay->Begin(m_pVince);
	}

	void QuestionPresenter::EndSurvey() 
	{
		m_pDisplay->End(m_pVince);
	}

	bool QuestionPresenter::PresentQuestion(CSurveyQuestion* pQuestion)
	{
		// There is no point in even attempting this if we do not have a valid font defined
		// for the question. Note that it will attempt to assign a default font if the custom
		// font load failed, but at least one valid font must have been loaded.

		if ( pQuestion->Properties->SelectedFont < 0 )
		{
			// Worry about logging errors later
			//g_pVince->pLogWriter->WriteError("SurveyQuestion", "AskQuestion", "No valid font available to present question");
			return NULL;
		}

		// Save to member variable for later use
		m_pQuestion = pQuestion;

		switch (pQuestion->QuestionType)
		{
		case MultipleChoice:
			return PresentQuestionMC(pQuestion);
			break;
		case MultiSelect:
			return PresentQuestionMS(pQuestion);
			break;
		case HorizontalScale:
			return PresentQuestionHS(pQuestion);
			break;
		case FreeformText:
			return PresentQuestionFF(pQuestion);
			break;
		}
		return false;
	}

	// Present Multiple Choice Question
	bool QuestionPresenter::PresentQuestionMC(CSurveyQuestion* pQuestion)
	{
		bool exitLoop	= false;
		bool resultSelected = false;
		m_SelectedAnswer = 0;
		pQuestion->SetSelectedAnswer(m_SelectedAnswer);

		while (!exitLoop) 
		{
			// Render the screen

			m_pDisplay->Clear(pQuestion->Properties->BackgroundColor);
			if ( NULL != pQuestion->Properties->BackgroundImage )
			{
				m_pDisplay->ShowBackground(pQuestion->Properties->BackgroundImage);
			}

			m_pDisplay->SetFont(pQuestion->Properties->SelectedFont);
			m_pDisplay->BeginText();
			DisplayQuestion(pQuestion->GetQuestionText());
			for (int numAnswer = 0; numAnswer < pQuestion->GetAnswerCount(); numAnswer++)
			{
				DisplayAnswer(numAnswer);
			}
			DisplayFooter();
			m_pDisplay->EndText();

			m_pDisplay->Present();

			// Now check for input

			switch ( m_pController->CheckInput() ) 
			{
			case CController::Input_Up:
					if (m_SelectedAnswer > 0) 
					{
						--m_SelectedAnswer;
					} 
					else 
					{
						m_SelectedAnswer = pQuestion->GetAnswerCount() - 1;
					}
					break;

				case CController::Input_Down:
					++m_SelectedAnswer;
					if ( m_SelectedAnswer >= pQuestion->GetAnswerCount() ) 
					{
						m_SelectedAnswer = 0;
					}
					break;

				case CController::Input_Ok:
					resultSelected	= true;
					exitLoop		= true;
					break;

				case CController::Input_Cancel:
					resultSelected	= false;
					exitLoop		= true;
					break;
			}
			pQuestion->SetSelectedAnswer(m_SelectedAnswer);
		}
		return resultSelected;
	}

	// Present Multiple Selection Question
	bool QuestionPresenter::PresentQuestionMS(CSurveyQuestion* pQuestion)
	{
		bool exitLoop	= false;
		bool resultSelected = false;
		bool checked[MAX_ANSWERS];
		for (int numAnswer = 0; numAnswer < pQuestion->GetAnswerCount(); numAnswer++)
		{
			checked[numAnswer] = false;
			pQuestion->SetChecked(numAnswer, false);
		}

		m_SelectedAnswer = 0;
		pQuestion->SetSelectedAnswer(m_SelectedAnswer);

		while (!exitLoop) 
		{
			// Render the screen

			m_pDisplay->Clear(pQuestion->Properties->BackgroundColor);
			if ( NULL != pQuestion->Properties->BackgroundImage )
			{
				m_pDisplay->ShowBackground(pQuestion->Properties->BackgroundImage);
			}

			m_pDisplay->SetFont(pQuestion->Properties->SelectedFont);
			m_pDisplay->BeginText();
			DisplayQuestion(pQuestion->GetQuestionText());
			for (int numAnswer = 0; numAnswer < pQuestion->GetAnswerCount(); numAnswer++)
			{
				DisplayAnswer(numAnswer);
				DisplayCheckMark(numAnswer, checked[numAnswer]);
			}
			DisplayFooter();
			m_pDisplay->EndText();

			m_pDisplay->Present();

			// Now check for input

			switch ( m_pController->CheckInput() ) 
			{
			case CController::Input_Up:
					if (m_SelectedAnswer > 0) 
					{
						--m_SelectedAnswer;
					} 
					else 
					{
						m_SelectedAnswer = pQuestion->GetAnswerCount() - 1;
					}
					break;

				case CController::Input_Down:
					++m_SelectedAnswer;
					if ( m_SelectedAnswer >= pQuestion->GetAnswerCount() ) 
					{
						m_SelectedAnswer = 0;
					}
					break;

				case CController::Input_Ok:
					resultSelected	= true;
					exitLoop		= true;
					break;

				case CController::Input_Check:
					checked[m_SelectedAnswer] = !checked[m_SelectedAnswer];
					pQuestion->SetChecked(m_SelectedAnswer, checked[m_SelectedAnswer]);
					break;

				case CController::Input_Cancel:
					resultSelected	= false;
					exitLoop		= true;
					break;
			}
			pQuestion->SetSelectedAnswer(m_SelectedAnswer);
		}
		return resultSelected;
	}

	// Present Horizontal Scale Question
	bool QuestionPresenter::PresentQuestionHS(CSurveyQuestion* pQuestion)
	{
		bool exitLoop	= false;
		bool resultSelected = false;
		m_SelectedAnswer = 0;
		pQuestion->SetSelectedAnswer(m_SelectedAnswer);

		while (!exitLoop) 
		{
			// Render the screen

			m_pDisplay->Clear(pQuestion->Properties->BackgroundColor);
			if ( NULL != pQuestion->Properties->BackgroundImage )
			{
				m_pDisplay->ShowBackground(pQuestion->Properties->BackgroundImage);
			}

			m_pDisplay->SetFont(pQuestion->Properties->SelectedFont);
			m_pDisplay->BeginText();
			DisplayQuestion(pQuestion->GetQuestionText());
			DisplaySlider();
			DisplayFooter();
			m_pDisplay->EndText();

			m_pDisplay->Present();

			// Now check for input

			switch ( m_pController->CheckInput() ) 
			{
			case CController::Input_Left:
					if (m_SelectedAnswer > 0) 
					{
						--m_SelectedAnswer;
					} 
					else 
					{
						m_SelectedAnswer = pQuestion->GetAnswerCount() - 1;
					}
					break;

				case CController::Input_Right:
					++m_SelectedAnswer;
					if ( m_SelectedAnswer >= pQuestion->GetAnswerCount() ) 
					{
						m_SelectedAnswer = 0;
					}
					break;

				case CController::Input_Ok:
					resultSelected	= true;
					exitLoop		= true;
					break;

				case CController::Input_Cancel:
					resultSelected	= false;
					exitLoop		= true;
					break;
			}
			pQuestion->SetSelectedAnswer(m_SelectedAnswer);
		}
		return resultSelected;
	}

	// Present Free Form Text Input Question
	bool QuestionPresenter::PresentQuestionFF(CSurveyQuestion* pQuestion)
	{
		bool exitLoop	= false;
		bool resultSelected = false;
		m_SelectedAnswer = 0;
		pQuestion->SetSelectedAnswer(m_SelectedAnswer);

		m_pController->ClearKeyboardText();

		while (!exitLoop) 
		{
			// Render the screen

			m_pDisplay->Clear(pQuestion->Properties->BackgroundColor);
			if ( NULL != pQuestion->Properties->BackgroundImage )
			{
				m_pDisplay->ShowBackground(pQuestion->Properties->BackgroundImage);
			}

			m_pDisplay->SetFont(pQuestion->Properties->SelectedFont);
			m_pDisplay->BeginText();
			DisplayQuestion(pQuestion->GetQuestionText());

			// Answer is freeform input
			const wchar_t* answerText = m_pController->GetKeyboardText();
			
			// Need to be able to set answer text. For now, just display locally
			DisplayFreeFormText(answerText);

			DisplayFooter();
			m_pDisplay->EndText();

			m_pDisplay->Present();

			// Now check for input - only check for OK or cancel

			switch ( m_pController->CheckInput() ) 
			{
				case CController::Input_Ok:
					pQuestion->SetUserText(answerText);
					resultSelected	= true;
					exitLoop		= true;
					break;

				case CController::Input_Cancel:
					pQuestion->SetUserText(NULL);
					resultSelected	= false;
					exitLoop		= true;
					break;
			}
		}
		return resultSelected;
	}


	// Put question text up on screen
	void QuestionPresenter::DisplayQuestion(const wchar_t* wcsQuestion)
	{
		// Constants for screen layout
		const float xCenter = 320.0f;
		const float yCenter = 70.0f;
		const float xScale = 1.5f;
		const float yScale = 2.0f;
		const float xSplitWidth = 800.0f;
		const float xShrinkWidth = 560.0f;

		// Check width of printed Question

		m_pDisplay->ScaleFont(xScale, yScale);
		float textWidth = m_pDisplay->GetTextWidth(wcsQuestion);

		// If it is very long, split into two lines

		if ( textWidth > xSplitWidth )
		{
			wchar_t* wcsTopLine = NULL;
			wchar_t* wcsBottomLine = NULL;
			SplitQuestion( wcsQuestion, &wcsTopLine, &wcsBottomLine);

			// Shrink the lines horizontally, if needed

			textWidth = MAX_VINCE( m_pDisplay->GetTextWidth( wcsTopLine ), m_pDisplay->GetTextWidth( wcsBottomLine ) );
			if ( textWidth > xShrinkWidth )
			{
				float xRescale = xScale * xShrinkWidth / textWidth;
				m_pDisplay->ScaleFont(xRescale, yScale);
			}
			else
			{
				m_pDisplay->ScaleFont(xScale, yScale);
			}

			// Now write the top and bottom lines

			m_pDisplay->DrawText(xCenter,  yCenter - 20, m_pQuestion->Properties->QuestionColor, wcsTopLine);
			m_pDisplay->DrawText(xCenter,  yCenter + 20, m_pQuestion->Properties->QuestionColor, wcsBottomLine);
			SAFE_DELETE_ARRAY(wcsTopLine);
			SAFE_DELETE_ARRAY(wcsBottomLine);
		}

		// If it is sort of long, squeeze it a bit

		else if ( textWidth > xShrinkWidth )
		{
			float xRescale = xScale * xShrinkWidth / textWidth;
			m_pDisplay->ScaleFont(xRescale, yScale);
			m_pDisplay->DrawText(xCenter,  yCenter, m_pQuestion->Properties->QuestionColor, wcsQuestion);
		}

		// Otherwise, just center it on top

		else
		{
			m_pDisplay->ScaleFont(xScale, yScale);
			m_pDisplay->DrawText(xCenter,  yCenter, m_pQuestion->Properties->QuestionColor, wcsQuestion);
		}
	}

	// Split the question into top and bottom parts
	void QuestionPresenter::SplitQuestion(const wchar_t* wcsFull, wchar_t** wcsTop, wchar_t** wcsBottom)
	{
		const float xMaxSize = 1500.0f;

		wchar_t* wcsTopLine = NULL;
		wchar_t* wcsBottomLine = NULL;

		size_t fullLength = wcslen(wcsFull);
		wcsTopLine = new wchar_t[fullLength + 1];
		wcsncpy( wcsTopLine, wcsFull, fullLength + 1 );

		while ( m_pDisplay->GetTextWidth(wcsTopLine) > xMaxSize)
		{
			// This assert will never fail, but added to keep Prefix happy
			assert(fullLength > 0);
			wcsTopLine[fullLength--] = '\0';
		}

		// Now, try to find a good split point near the middle

		int direction = 1;
		size_t midPoint = fullLength / 2;

		for ( int i = 1; i < 15; i++ )
		{
			if ( L' ' == wcsTopLine[midPoint] )
			{
				wcsTopLine[midPoint] = '\0';
				wcsBottomLine = new wchar_t[ fullLength - midPoint + 1];
				wcscpy( wcsBottomLine, wcsTopLine + midPoint + 1);
				break;
			}
			else
			{
				midPoint += direction * i;
				direction *= -1;
			}
		}

		// Didn't find one? - Split in the middle

		if ( NULL == wcsBottomLine )
		{
			midPoint = fullLength / 2;
			wcsBottomLine = new wchar_t[ fullLength - midPoint + 2];
			wcscpy( wcsBottomLine, wcsTopLine + midPoint);
			wcsTopLine[midPoint] = '\0';
		}

		// Set return strings to new top and bottom

		*wcsTop = wcsTopLine;
		*wcsBottom = wcsBottomLine;
	}

	// Put text for single answer up on screen
	void QuestionPresenter::DisplayAnswer(int numAnswer)
	{
		const float x = 320.0;
		const float y = 140.0;
		const float xScale = 1.0;
		const float yScale = 1.5;

		DWORD dwColor = m_pQuestion->Properties->AnswerColor;
		if (numAnswer == m_SelectedAnswer)
		{
			dwColor = m_pQuestion->Properties->SelectedAnswerColor;
		}

		float spacing = 250.0f / ((float) (m_pQuestion->GetAnswerCount()) );
		float yPos = y + ( (float) numAnswer ) * spacing;
		m_pDisplay->ScaleFont(xScale, yScale);
	    m_pDisplay->DrawText( x,  yPos,	dwColor, m_pQuestion->GetAnswerText(numAnswer) );
	}

	// Display up to 4 lines of freeform text
	void QuestionPresenter::DisplayFreeFormText(const wchar_t* wcsText)
	{
		const float x = 320.0;
		const float y = 200.0;
		const float xScale = 1.0;
		const float yScale = 1.5;
		const int LINE_SIZE = 50;
		wchar_t wcsLine[LINE_SIZE + 1];

		wcsLine[LINE_SIZE] = L'\0';

		DWORD dwColor = m_pQuestion->Properties->SelectedAnswerColor;

		m_pDisplay->ScaleFont(xScale, yScale);
		
		size_t length = wcslen(wcsText);

		// Simple word wrap for now
		int startPos = 0;
		float yPos = y;

		while (length > LINE_SIZE)
		{
			wcsncpy(wcsLine, wcsText + startPos, LINE_SIZE);
			m_pDisplay->DrawText( x, yPos, dwColor, wcsLine );
			startPos += LINE_SIZE;
			length -= LINE_SIZE;
			yPos += 30;
		}
	    m_pDisplay->DrawText( x, yPos, dwColor, wcsText + startPos );
	}

	// Put check mark for single answer up on screen
	void QuestionPresenter::DisplayCheckMark(int numAnswer, bool isChecked)
	{
		static float x = 50.0;
		const float y = 140.0;
		const float xScale = 1.0;
		const float yScale = 1.5;

		DWORD dwColor = m_pQuestion->Properties->AnswerColor;
		if (numAnswer == m_SelectedAnswer)
		{
			dwColor = m_pQuestion->Properties->SelectedAnswerColor;
		}

		float spacing = 250.0f / ((float) (m_pQuestion->GetAnswerCount()) );
		float yPos = y + ( (float) numAnswer ) * spacing;
		m_pDisplay->ScaleFont(xScale, yScale);

		//m_pDisplay->DrawText( x,  yPos,	dwColor, L"_" );
		DWORD dwBackColor = m_pQuestion->Properties->BackgroundColor;
		m_pDisplay->DrawBox( x - 25.0f, yPos - 25.0f, x + 25.0f, yPos + 25.0f, dwColor, dwBackColor);
		if (isChecked)
		{
			m_pDisplay->DrawText( x,  yPos,	dwColor, L"X" );
		}
	}

	// Show Horizontal slider for selection
	void QuestionPresenter::DisplaySlider()
	{
		static float x = 320.0;
		static float y = 200.0;
		const float xScale = 1.0;
		const float yScale = 1.5;

		m_pDisplay->ScaleFont(xScale, yScale);

		DWORD dwColor = m_pQuestion->Properties->AnswerColor;
		int numAnswers = m_pQuestion->GetAnswerCount();
		float spacing = 100.0f;
		if (numAnswers > 5)
		{
			 spacing = 400.0f / (float)(numAnswers - 1);
		}
		
		float start = 320.0f - (numAnswers - 1) * .5f * spacing;
		wchar_t digits[2];
		digits[1] = L'\0';

		for (int iAnswer = 0; iAnswer < numAnswers; iAnswer++)
		{
			DWORD dwColor = m_pQuestion->Properties->AnswerColor;
			if (iAnswer == m_SelectedAnswer)
			{
				dwColor = m_pQuestion->Properties->SelectedAnswerColor;
			}
			digits[0] = L'0' + (wchar_t)(iAnswer + 1);
			m_pDisplay->DrawText( start + iAnswer * spacing, y, dwColor, digits );
		}

		// Show the full text of the selected answer
		dwColor = m_pQuestion->Properties->SelectedAnswerColor;
	    m_pDisplay->DrawText( x, y + 40.0f, dwColor, m_pQuestion->GetAnswerText(m_SelectedAnswer) );
	}

	// Show instructions as bottom of question screen
	void QuestionPresenter::DisplayFooter()
	{
		static float x = 320.0;
		static float y = 410.0;
		static float xScale = 1.2f;
		static float yScale = 1.2f;

		DWORD dwColor = m_pQuestion->Properties->AnswerColor;

		m_pDisplay->ScaleFont(xScale, yScale);

		if (m_pQuestion->QuestionType == MultiSelect )
		{
			m_pDisplay->DrawText( x,  y, dwColor, FOOTER_TEXT_MULTI_SELECT);
		}
		else
		{
			m_pDisplay->DrawText( x,  y, dwColor, FOOTER_TEXT_STANDARD);
		}

		// If no customized text has been defined, use standard instruction
		// RWBFIX: Revisit this later
		//if (NULL == m_FooterText)
		//{
		//	DrawText( x,  y, dwColor,
		//		  L"Press " GLYPH_A_BUTTON L" to select, or " GLYPH_B_BUTTON L" to cancel");
		//}
		//else
		//{
		//	DrawText( x,  y, dwColor, m_FooterText);
		//}
	}

	// Display instruction line at the bottom of survey display
	void QuestionPresenter::SetFooterText(const char* cstrFooterText)
	{
		// Should only be called once, but check anyway
		if (NULL != m_FooterText)
		{
			SAFE_DELETE_ARRAY(m_FooterText);
		}
		wchar_t buffer[128];
		int inchar = 0;
		int outchar = 0;
		while ('\0' != cstrFooterText[inchar])
		{
			if ( '0' == cstrFooterText[inchar])
			{
				int hex = 0;
				sscanf(cstrFooterText+inchar, "%6i", &hex);
				buffer[outchar++] = (unsigned short)hex;
				inchar += 6;
			}
			else
			{
				buffer[outchar++] = cstrFooterText[inchar++];
			}
		}
		buffer[outchar] = L'\0';
		m_FooterText = SAFE_COPY(buffer);
	}
}
#endif // _VINCE_