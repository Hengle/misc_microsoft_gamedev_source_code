//	QuestionProperties.h : Encapsulates display properties of a survey question
//
//	Created 2005/09/19 Rich Bonny <rbonny@microsoft.co13
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2004 Microsoft Corp.  All rights reserved.


#pragma once

namespace Vince
{
	class CVince;

	class CQuestionProperties
	{
	public:
		CQuestionProperties(CVince* vince);
		~CQuestionProperties(void);

		// We expose common properties as public members for now to allow
		// values to be set at accessed easily.

		// Color and font settings
		int SelectedFont;
		const char* BackgroundImage;
		DWORD QuestionColor;
		DWORD SelectedAnswerColor;
		DWORD AnswerColor;
		DWORD FooterColor;
		DWORD BackgroundColor;
	};
}
