//	QuestionProperties.cpp : Encapsulates display properties of a survey question
//
//	Created 2005/09/19 Rich Bonny <rbonny@microsoft.co13
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2004 Microsoft Corp.  All rights reserved.

#include "VinceControl.h"

#ifdef _VINCE_

#include "Vince.h"
#include "QuestionProperties.h"
#include "Display.h"
#include "TnTUtil.h"
#include "VinceUtil.h"

// Set default properties if none specified.
#define DEFAULT_QUESTION_COLOR 0xffffff00
#define DEFAULT_SELECTED_ANSWER_COLOR 0xffffffff
#define DEFAULT_ANSWER_COLOR 0xff7f7f7f
#define DEFAULT_FOOTER_COLOR 0xffffffff
#define DEFAULT_BACKROUND_COLOR 0xff000020
#ifdef _XBOX
  #if _XBOX_VER >= 200 // Xenon
	#define DEFAULT_FONT "Arial_16.xpr"
  #else
	#define DEFAULT_FONT "Font16.xpr"
  #endif
#else
	#define DEFAULT_FONT "Tahoma"
#endif


namespace Vince
{
	// We check current settings or use default values for
	// any of the display properties
	CQuestionProperties::CQuestionProperties(CVince* vince)
	{
		// Set the font. If no font can be used, we have a problem.
		// (an error will get logged and the question will not be displayed)

		const char* cstrFontName = vince->pSettings->Fetch("SurveyFont", DEFAULT_FONT);
		CDisplay* pDisplay = Vince::CDisplay::Instance();
		SelectedFont = pDisplay->LoadFont(vince, cstrFontName);

		QuestionColor = vince->pSettings->Fetch("QuestionColor", (DWORD) DEFAULT_QUESTION_COLOR);
		SelectedAnswerColor = vince->pSettings->Fetch("SelectedAnswerColor", (DWORD) DEFAULT_SELECTED_ANSWER_COLOR);
		AnswerColor = vince->pSettings->Fetch("AnswerColor", (DWORD) DEFAULT_ANSWER_COLOR);
		FooterColor = vince->pSettings->Fetch("FooterColor", (DWORD) DEFAULT_FOOTER_COLOR);

		BackgroundColor = vince->pSettings->Fetch("BackgroundColor", (DWORD) DEFAULT_BACKROUND_COLOR);
		const char* cstrBackgroundImage = vince->pSettings->Fetch("BackgroundImage", (const char *)NULL);
		if (NULL == cstrBackgroundImage)
		{
			BackgroundImage = NULL;
		}
		else
		{
			//
			if (0 == strlen(cstrBackgroundImage))
			{
				BackgroundImage = NULL;
			}
			else
			{
				BackgroundImage = GetFullFileName(cstrBackgroundImage, false);
			}
		}
	}

	CQuestionProperties::~CQuestionProperties()
	{
		SAFE_DELETE_ARRAY(BackgroundImage);
	}

}
#endif	// _VINCE_