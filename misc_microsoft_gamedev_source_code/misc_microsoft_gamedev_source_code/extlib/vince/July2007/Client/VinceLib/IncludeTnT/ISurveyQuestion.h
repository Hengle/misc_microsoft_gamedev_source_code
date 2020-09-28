//	ISurveyQuestion.h : Interface to a survey question
//
//	Created 2007/01/23 Dan Berke <dberke@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2007 Microsoft Corp.  All rights reserved.
#pragma once

#include "ISurveyLogger.h"

namespace SurveyLib
{
    enum EQuestionType {
        MultipleChoice,
        MultiSelect,
        FreeformText,
        HorizontalScale
    };

    class ISurveyQuestion 
    {
    public:
				 ISurveyQuestion() {}
		virtual ~ISurveyQuestion() {}

        // Functions for building the question 
        virtual void			SetAnswerLogger(ISurveyLogger* pLogger) = 0;
        virtual void			SetUserText(const wchar_t* wcsUserText)	= 0;


        // Question properties - used by ISurveyRenderer classes to determine what to draw
        virtual const wchar_t*	GetQuestionID()					= 0;
        virtual EQuestionType	GetQuestionType()				= 0;
        virtual const wchar_t*	GetQuestionText()				= 0;
        virtual const wchar_t*	GetAnswerText(int answerNum)	= 0;
        virtual int				GetAnswerCount()				= 0;
        virtual const wchar_t*	GetNextQuestionID()				= 0;

        // Question state
        virtual void			ResetState(const wchar_t* wcsContext)		= 0;
		virtual bool			IsQuestionDone()							= 0;
        virtual void			SetSelectedAnswer(int answerNum)			= 0;
		virtual int 			SelectNextAnswer()							= 0;
		virtual int 			SelectPrevAnswer()							= 0;
        virtual int 			GetSelectedAnswer()							= 0;
        virtual void			SetCheckState(int answerNum, bool state)	= 0;
        virtual bool			GetCheckState(int answerNum)				= 0;
		virtual const wchar_t*  GetEnteredText()							= 0;
        virtual void			SetAnswered()								= 0;
        virtual void			SetCancelled()								= 0;
		virtual void			Enable()									= 0;
		virtual void			Disable()									= 0;
		virtual bool			IsEnabled()									= 0;
	};
}
