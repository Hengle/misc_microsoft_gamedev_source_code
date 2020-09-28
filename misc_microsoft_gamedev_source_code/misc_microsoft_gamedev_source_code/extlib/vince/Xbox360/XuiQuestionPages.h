//  XuiQuestionPages.h - XUI survey question pages
//
//	Created 2005/09/19 Dan Berke <dberke@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2005 Microsoft Corp.  All rights reserved.

#pragma once

#include "VinceControl.h"

#ifdef VINCE_XUI

#ifdef _XBOX
#include <xtl.h>
#else
#include <d3d9.h>
#include <d3dx9math.h>
#include <dinput.h>
#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <stdlib.h>
#include <windowsx.h>
#include <tchar.h>
#include <commctrl.h>
#endif

#include <xui.h>
#include <xuiapp.h>

#include "SurveyQuestion.h"

namespace Vince {

    //--------------------------------------------------------------------------------------
    // Name: class VinceQuestionSingleSelect
    // Desc: Xui scene class for VINCE single-select questions
    //--------------------------------------------------------------------------------------
    class VinceQuestionSingleSelect : public CXuiSceneImpl
    {
        public:
            XUI_IMPLEMENT_CLASS(VinceQuestionSingleSelect, L"VinceQuestionSingleSelect", XUI_CLASS_SCENE);
            
        private:
            // Message map
            XUI_BEGIN_MSG_MAP()
                XUI_ON_XM_INIT(OnInit)
                XUI_ON_XM_KEYDOWN(OnKeyDown)
                XUI_ON_XM_NOTIFY_PRESS(OnNotifyPress)
            XUI_END_MSG_MAP()

            // Message callback functions
            HRESULT OnInit(XUIMessageInit *pInitData, BOOL& bHandled);
            HRESULT OnNotifyPress(HXUIOBJ hObjSource, BOOL& bHandled);
            HRESULT OnKeyDown(XUIMessageInput *pInputData, BOOL& bHandled);

            static const int MAX_ANSWER_BUTTONS = 7;

            // Scene controls
            CXuiControl m_question;
            CXuiControl m_answer[MAX_ANSWER_BUTTONS];

            // Question that this scene is displaying
            CSurveyQuestion *m_pQuestion;
    };

    //--------------------------------------------------------------------------------------
    // Name: class VinceQuestionMultiSelect
    // Desc: Xui scene class for VINCE multiple-choice (checkbox) questions
    //--------------------------------------------------------------------------------------
    class VinceQuestionMultiSelect : public CXuiSceneImpl
    {
        public:
            XUI_IMPLEMENT_CLASS(VinceQuestionMultiSelect, L"VinceQuestionMultiSelect", XUI_CLASS_SCENE);
            
        private:
            // Message map
            XUI_BEGIN_MSG_MAP()
                XUI_ON_XM_INIT(OnInit)
                XUI_ON_XM_KEYDOWN(OnKeyDown)
                XUI_ON_XM_NOTIFY_PRESS(OnNotifyPress)
            XUI_END_MSG_MAP()

            // Message callback functions
            HRESULT OnInit(XUIMessageInit *pInitData, BOOL& bHandled);
            HRESULT OnNotifyPress(HXUIOBJ hObjSource, BOOL& bHandled);
            HRESULT OnKeyDown(XUIMessageInput *pInputData, BOOL& bHandled);

            static const int MAX_ANSWER_BUTTONS = 7;

            // Scene controls
            CXuiControl  m_question;
            CXuiCheckbox m_answer[MAX_ANSWER_BUTTONS];
            CXuiControl  m_okButton;
            CXuiControl  m_cancelButton;

            // Question that this scene is displaying
            CSurveyQuestion *m_pQuestion;
    };

    //--------------------------------------------------------------------------------------
    // Name: class VinceQuestionSlider
    // Desc: Xui scene class for VINCE slider questions
    //--------------------------------------------------------------------------------------
    class VinceQuestionSlider : public CXuiSceneImpl
    {
        public:
            XUI_IMPLEMENT_CLASS(VinceQuestionSlider, L"VinceQuestionSlider", XUI_CLASS_SCENE);
            
        private:
            // Message map
            XUI_BEGIN_MSG_MAP()
                XUI_ON_XM_INIT(OnInit)
                XUI_ON_XM_KEYDOWN(OnKeyDown)
                XUI_ON_XM_NOTIFY_VALUE_CHANGED(OnValueChanged)
                XUI_ON_XM_NOTIFY_PRESS(OnNotifyPress)
            XUI_END_MSG_MAP()

            // Message callback functions
            HRESULT OnInit(XUIMessageInit *pInitData, BOOL& bHandled);
            HRESULT OnKeyDown(XUIMessageInput *pInputData, BOOL& bHandled);
            HRESULT OnValueChanged(HXUIOBJ hObjSource, XUINotifyValueChanged *pNotifyValueChangedData, BOOL& bHandled);
            HRESULT OnNotifyPress(HXUIOBJ hObjSource, BOOL& bHandled);

            // Scene controls
            CXuiControl m_question;
            CXuiControl m_selection;
            CXuiSlider  m_slider;
            CXuiControl m_okButton;
            CXuiControl m_cancelButton;

            // Question that this scene is displaying
            CSurveyQuestion *m_pQuestion;
    };

    //--------------------------------------------------------------------------------------
    // Name: class VinceQuestionText 
    // Desc: Xui scene class for VINCE free-form text questions
    //--------------------------------------------------------------------------------------
    class VinceQuestionText : public CXuiSceneImpl
    {
        public:
            XUI_IMPLEMENT_CLASS(VinceQuestionText, L"VinceQuestionText", XUI_CLASS_SCENE);
            
        private:
            // Message map
            XUI_BEGIN_MSG_MAP()
                XUI_ON_XM_INIT(OnInit)
                XUI_ON_XM_KEYDOWN(OnKeyDown)
                XUI_ON_XM_NOTIFY_PRESS(OnNotifyPress)
            XUI_END_MSG_MAP()

            // Message callback functions
            HRESULT OnInit(XUIMessageInit *pInitData, BOOL& bHandled);
            HRESULT OnKeyDown(XUIMessageInput *pInputData, BOOL& bHandled);
            HRESULT OnNotifyPress(HXUIOBJ hObjSource, BOOL& bHandled);

            // Scene controls
            CXuiControl m_question;
            CXuiEdit    m_edit;
            CXuiControl  m_okButton;
            CXuiControl  m_cancelButton;

            // Question that this scene is displaying
            CSurveyQuestion *m_pQuestion;
    };

}

#endif
