//	IQuestionPresenter.h : Interface supported by Survey Question Presenters
//
//  These are the methods that must be supported by an object which shows VINCE
//  Survey questions to users and requests responses.
//
//	Created 2005/09/13 Rich Bonny <rbonny@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2005 Microsoft Corp.  All rights reserved.

#pragma once

// RWBFIX: Unnecessary?
#ifdef _XBOX
	#include <XTL.h>
#else					
	#include <D3DX9.h>
#endif

namespace Vince
{
	// Forward declaration
	class CSurveyQuestion;

	class IQuestionPresenter
	{
	public:
        virtual ~IQuestionPresenter() {};

		// Initialize the display
		virtual HRESULT InitDevice(IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* pD3Dpp, HWND hWnd) = 0;

		// Functions called at the start and end of the survey before any questions are shown
		virtual void BeginSurvey() = 0;
		virtual void EndSurvey() = 0;

		// Blocking question display.  Returns true if question answered and false if cancelled.
		virtual bool PresentQuestion(CSurveyQuestion* pQuestion) = 0;

        // Survey position and scaling
        virtual void SetSurveyPos(float x, float y) = 0;
        virtual void SetSurveyScale(float scaleX, float scaleY) = 0;

        // Hide question
        virtual void HideQuestion() = 0;

        // Non-blocking question display.  Returns true if question answered and false if cancelled.
        virtual bool PresentQuestionAsync(CSurveyQuestion* pQuestion) = 0;

        // Asynchonous update of a question.  Should be called in main app's render loop before the D3D Present()
        virtual void UpdateAsync() = 0;

        // Input processing
        virtual bool ProcessInputMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* pResult) = 0;
	};
}