//	ISurveyRenderer : Interface to a survey renderer
//
//	Created 2005/09/13 Rich Bonny <rbonny@microsoft.com>
//	Refactored 2006/12/21 Dan Berke  <dberke@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2006 Microsoft Corp.  All rights reserved.

#pragma once

#ifdef _XBOX
	#include <xtl.h>
#else					
	#include <windows.h>
#endif


namespace SurveyLib
{
	// Forward declarations
	class Survey;
	class SurveyQuestion;

	class ISurveyRenderer
	{
		public:
			virtual ~ISurveyRenderer() {};

			// Refcounting
			virtual void AddRef()  = 0;
			virtual void Release() = 0;

			// Functions called at the start and end of the survey before any questions are shown
			virtual HRESULT BeginSurvey(Survey* pSurvey) = 0;
			virtual HRESULT EndSurvey(Survey* pSurvey) = 0;

			// Functions called when a question is being shown and hidden
			virtual HRESULT BeginQuestion(SurveyQuestion* pQuestion) = 0;
			virtual HRESULT EndQuestion(SurveyQuestion* pQuestion)   = 0;

			// Render a question
			virtual HRESULT RenderQuestion(SurveyQuestion* pQuestion) = 0;

			// Process question input
			virtual HRESULT ProcessQuestionInput(SurveyQuestion* pQuestion) = 0;
	};
}