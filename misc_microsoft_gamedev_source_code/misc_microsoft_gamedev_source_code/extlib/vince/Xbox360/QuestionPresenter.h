//	QuestionPresenter.h : Default survey question presenter logic
//
//	Created 2005/09/08 Rich Bonny <rbonny@microsoft.co13
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2004 Microsoft Corp.  All rights reserved.


#pragma once

#include "IQuestionPresenter.h"
#ifdef _XBOX
	#include <XTL.h>
#else
	#include <Windows.h>
#endif

#include "Display.h"
#include "Controller.h"

namespace Vince
{
	inline float MAX_VINCE( float a, float b ) { return a > b ? a : b; }

	class CVince;

	class QuestionPresenter : public IQuestionPresenter
	{
	public:
		QuestionPresenter(CVince* vince);
        virtual ~QuestionPresenter();

    // IQuestionPresenter implementation
	public:
        virtual HRESULT InitDevice(IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* pD3Dpp, HWND hWnd);
		virtual void BeginSurvey();
		virtual void EndSurvey();
		virtual bool PresentQuestion(CSurveyQuestion* pQuestion);

        // RWBFIX: Stubbed out functions required for async display
		// Disable warnings about unreferenced parameters
#pragma warning(push)
#pragma warning(disable :4100)
        virtual bool ProcessInputMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* pResult) { return(true); }
        virtual void HideQuestion() {}
        virtual bool PresentQuestionAsync(CSurveyQuestion* pQuestion) { return(false); }
        virtual void UpdateAsync() {}

        virtual void SetSurveyPos(float x, float y) {}
        virtual void SetSurveyScale(float scaleX, float scaleY) {}
#pragma warning(pop)

	protected:
		CVince* m_pVince;

		bool PresentQuestionMC(CSurveyQuestion* pQuestion);
		bool PresentQuestionMS(CSurveyQuestion* pQuestion);
		bool PresentQuestionHS(CSurveyQuestion* pQuestion);
		bool PresentQuestionFF(CSurveyQuestion* pQuestion);
		void DisplayQuestion(const wchar_t* wcsQuestion);
		void DisplayAnswer(int numAnswer);
		void DisplayFreeFormText(const wchar_t* wcsText);
		void DisplayCheckMark(int numAnswer, bool isChecked);
		void DisplaySlider();
		void DisplayFooter();
		void SetFooterText(const char* cstrFooterText);
		void SplitQuestion(const wchar_t* wcsFull, wchar_t** wcsTop, wchar_t** wcsBottom);
		CSurveyQuestion* m_pQuestion;
		int m_SelectedAnswer;
		const wchar_t* m_FooterText;

	private:
		static QuestionPresenter* s_pInstance;
		CDisplay* m_pDisplay;
		CController* m_pController;
	};
}
