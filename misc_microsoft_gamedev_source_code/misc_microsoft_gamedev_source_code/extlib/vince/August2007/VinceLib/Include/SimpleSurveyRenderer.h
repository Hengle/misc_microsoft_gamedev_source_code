//	SimpleSurveyRenderer : Simple renderer for surveys
//
//	Created    2004/03/10 Rich Bonny <rbonny@microsoft.com>
//	Refactored 2006/12/21 Dan Berke  <dberke@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2006 Microsoft Corp.  All rights reserved.
#pragma once

#ifdef _XBOX
	#include <xtl.h>
#else
	#include <windows.h>
	#include <d3dx9.h>
#endif

#include "Survey.h"

namespace SurveyLib
{
	// Forward declarations
	class ISurveyInput;
	class TextRenderer;

	class SimpleSurveyRenderer : public ISurveyRenderer
	{
		public:
					 SimpleSurveyRenderer();
			virtual ~SimpleSurveyRenderer();

			// Refcounting
			virtual void AddRef();
			virtual void Release();

			HRESULT Initialize(IDirect3DDevice9 *pD3DDevice);

			// ISurveyRenderer implementation
            virtual HRESULT BeginSurvey(Survey* pSurvey); 
            virtual HRESULT EndSurvey(Survey* pSurvey);   
            virtual HRESULT BeginQuestion(SurveyQuestion* pQuestion);
            virtual HRESULT EndQuestion(SurveyQuestion* pQuestion);
			virtual HRESULT RenderQuestion(SurveyQuestion* pQuestion);
			virtual HRESULT ProcessQuestionInput(SurveyQuestion* pQuestion);

			// Renderer properties
			void	SetQuestionColor(DWORD color);
			void	SetAnswerColor(DWORD color);
			void	SetSelectionColor(DWORD color);
			void	SetFooterColor(DWORD color);
			void	SetBackgroundColor(DWORD color);
			HRESULT LoadFont(const char* fontFile);

			void    SetSurveySize(UINT width, UINT height);
			void    SetSurveyPos(UINT x, UINT y);

		protected:
			HRESULT RenderQuestionMC(SurveyQuestion* pQuestion);
			HRESULT RenderQuestionMS(SurveyQuestion* pQuestion);
			HRESULT RenderQuestionHS(SurveyQuestion* pQuestion);
			HRESULT RenderQuestionFF(SurveyQuestion* pQuestion);

			HRESULT	ProcessInputMC(SurveyQuestion* pQuestion);
			HRESULT	ProcessInputMS(SurveyQuestion* pQuestion);
			HRESULT	ProcessInputHS(SurveyQuestion* pQuestion);
			HRESULT	ProcessInputFF(SurveyQuestion* pQuestion);

			void DisplayBackground(RECT& bounds);
			void DisplayQuestionText(const wchar_t* wcsQuestion, RECT& bounds);
			void DisplayAnswersText(SurveyQuestion* pQuestion, RECT& bounds);
			void DisplayFreeFormText(const wchar_t* wcsText, RECT& bounds);
			void DisplayCheckMarks(SurveyQuestion* pQuestion, RECT& bounds);
			void DisplaySlider(SurveyQuestion* pQuestion, RECT& bounds);
			void DisplayFooter(SurveyQuestion* pQuestion, RECT& bounds);
			wchar_t* SplitQuestionText(float maxWidth, wchar_t* text);

			long		  m_refCount;
			TextRenderer* m_pTextRenderer;
			ISurveyInput* m_pInput;

			DWORD m_questionColor;
			DWORD m_answerColor;
			DWORD m_selectionColor;
			DWORD m_footerColor;
			DWORD m_bgColor;
			int	  m_fontNum;
			UINT  m_surveyWidth;
			UINT  m_surveyHeight;
			UINT  m_surveyPosX;
			UINT  m_surveyPosY;
	};
}
