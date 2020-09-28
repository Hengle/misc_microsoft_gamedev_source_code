//  XuiQuestionPages.h - XUI survey question pages
//
//	Created 2005/09/19 Dan Berke <dberke@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2005 Microsoft Corp.  All rights reserved.

#include "VinceControl.h"      // To pick up VINCE_XUI

#ifdef VINCE_XUI

#include "XuiQuestionPages.h"

namespace Vince {

    //--------------------------------------------------------------------------------------
    // Name: VinceQuestionSingleSelect::OnInit
    // Desc: Initialize the scene
    //--------------------------------------------------------------------------------------
    HRESULT VinceQuestionSingleSelect::OnInit(XUIMessageInit *pInitData, BOOL& bHandled)
    {
        GetChildById(L"XuiLabelQuestion",  &m_question);
        GetChildById(L"XuiButtonAnswer1",  &m_answer[0]);
        GetChildById(L"XuiButtonAnswer2",  &m_answer[1]);
        GetChildById(L"XuiButtonAnswer3",  &m_answer[2]);
        GetChildById(L"XuiButtonAnswer4",  &m_answer[3]);
        GetChildById(L"XuiButtonAnswer5",  &m_answer[4]);
        GetChildById(L"XuiButtonAnswer6",  &m_answer[5]);
        GetChildById(L"XuiButtonAnswer7",  &m_answer[6]);

        m_pQuestion = (CSurveyQuestion*)pInitData->pvInitData;
        m_question.SetText(m_pQuestion->GetQuestionText());

        int numAnswers = m_pQuestion->GetAnswerCount();
        if(numAnswers > MAX_ANSWER_BUTTONS) {
            numAnswers = MAX_ANSWER_BUTTONS;
        }

        // Get size of each answer
        float answerW, answerH;
        m_answer[0].GetBounds(&answerW, &answerH);

        // Get position of top answer
        D3DXVECTOR3 pos;
        m_answer[0].GetPosition(&pos);
        float x    = pos.x;
        float yMin = pos.y;

        // Get position of second answer to get gap size
        m_answer[1].GetPosition(&pos);
        float yGap = pos.y - yMin - answerH;
        
        // Get position of bottom answer
        m_answer[MAX_ANSWER_BUTTONS - 1].GetPosition(&pos);
        float yMax = pos.y + answerH;

        // Compute new starting Y position to center the needed buttons
        float ansTotalY = (float)numAnswers * answerH;
        float gapTotalY = (float)(numAnswers - 1) * yGap;
        float newY      = yMin + ((yMax - yMin - ansTotalY - gapTotalY) / 2.0f);

        // Set text of answers and positions
        int qnum = 0;
        for(qnum = 0; qnum < numAnswers; qnum++) {
            m_answer[qnum].SetText(m_pQuestion->GetAnswerText(qnum));
            
            D3DXVECTOR3 newPos;
            newPos.x = x;
            newPos.y = newY;
            newPos.z = 1.0;
            m_answer[qnum].SetPosition(&newPos);
            newY += answerH + yGap;
        }
        // Hide unused answer buttons
        for(; qnum < MAX_ANSWER_BUTTONS; qnum++) {
            m_answer[qnum].SetShow(false);
        }

        return(S_OK);
    }

    //--------------------------------------------------------------------------------------
    // Name: VinceQuestionSingleSelect::OnNotifyPress
    // Desc: Handle input in the scene
    //--------------------------------------------------------------------------------------
    HRESULT VinceQuestionSingleSelect::OnNotifyPress(HXUIOBJ hObjSource, BOOL& bHandled)
    {
        for(int i = 0; i < MAX_ANSWER_BUTTONS; i++) {
            if(hObjSource == m_answer[i]) {
                m_pQuestion->SetSelectedAnswer(i);
                m_pQuestion->SetAnswered();
                bHandled = TRUE;
            }
        }

        return(S_OK);
    }

    //--------------------------------------------------------------------------------------
    // Name: VinceQuestionSingleSelect::OnKeyDown
    // Desc: Check for specific key input in the scene
    //--------------------------------------------------------------------------------------
    HRESULT VinceQuestionSingleSelect::OnKeyDown(XUIMessageInput *pInputData, BOOL& bHandled)
    {
        // Pad B button and Escape key cancels
        if(pInputData->dwKeyCode == VK_PAD_B || pInputData->dwKeyCode == VK_ESCAPE) {
            m_pQuestion->SetCancelled();
            bHandled = TRUE;
        }

        return(S_OK);
    }


    //--------------------------------------------------------------------------------------
    // Name: VinceQuestionMultiSelect::OnInit
    // Desc: Initialize the scene
    //--------------------------------------------------------------------------------------
    HRESULT VinceQuestionMultiSelect::OnInit(XUIMessageInit *pInitData, BOOL& bHandled)
    {
        GetChildById(L"XuiLabelQuestion",    &m_question);
        GetChildById(L"XuiButtonAccept",     &m_okButton);
        GetChildById(L"XuiButtonCancel",     &m_cancelButton);
        GetChildById(L"XuiCheckboxAnswer1",  &m_answer[0]);
        GetChildById(L"XuiCheckboxAnswer2",  &m_answer[1]);
        GetChildById(L"XuiCheckboxAnswer3",  &m_answer[2]);
        GetChildById(L"XuiCheckboxAnswer4",  &m_answer[3]);
        GetChildById(L"XuiCheckboxAnswer5",  &m_answer[4]);
        GetChildById(L"XuiCheckboxAnswer6",  &m_answer[5]);
        GetChildById(L"XuiCheckboxAnswer7",  &m_answer[6]);

        m_pQuestion = (CSurveyQuestion*)pInitData->pvInitData;
        m_question.SetText(m_pQuestion->GetQuestionText());

        int numAnswers = m_pQuestion->GetAnswerCount();
        if(numAnswers > MAX_ANSWER_BUTTONS) {
            numAnswers = MAX_ANSWER_BUTTONS;
        }

        // Get size of each answer
        float answerW, answerH;
        m_answer[0].GetBounds(&answerW, &answerH);

        // Get position of top answer
        D3DXVECTOR3 pos;
        m_answer[0].GetPosition(&pos);
        float x    = pos.x;
        float yMin = pos.y;

        // Get position of second answer to get gap size
        m_answer[1].GetPosition(&pos);
        float yGap = pos.y - yMin - answerH;
        
        // Get position of bottom answer
        m_answer[MAX_ANSWER_BUTTONS - 1].GetPosition(&pos);
        float yMax = pos.y + answerH;

        // Compute new starting Y position to center the needed buttons
        float ansTotalY = (float)numAnswers * answerH;
        float gapTotalY = (float)(numAnswers - 1) * yGap;
        float newY      = yMin + ((yMax - yMin - ansTotalY - gapTotalY) / 2.0f);

        // Set text of answers and positions
        int qnum = 0;
        for(qnum = 0; qnum < numAnswers; qnum++) {
            m_answer[qnum].SetText(m_pQuestion->GetAnswerText(qnum));
            
            D3DXVECTOR3 newPos;
            newPos.x = x;
            newPos.y = newY;
            newPos.z = 1.0;
            m_answer[qnum].SetPosition(&newPos);
            newY += answerH + yGap;
        }
        // Hide unused answer buttons
        for(; qnum < MAX_ANSWER_BUTTONS; qnum++) {
            m_answer[qnum].SetShow(false);
        }

        return(S_OK);
    }

    //--------------------------------------------------------------------------------------
    // Name: VinceQuestionMultiSelect::OnNotifyPress
    // Desc: Handle input in the scene
    //--------------------------------------------------------------------------------------
    HRESULT VinceQuestionMultiSelect::OnNotifyPress(HXUIOBJ hObjSource, BOOL& bHandled)
    {
        for(int i = 0; i < MAX_ANSWER_BUTTONS; i++) {
            if(hObjSource == m_answer[i]) {
                m_pQuestion->SetChecked(i, m_answer[i].IsChecked() ? true : false);
                bHandled = TRUE;
            }
        }

        // Pressed Accept
        if(hObjSource == m_okButton) {
            m_pQuestion->SetAnswered();
            bHandled = TRUE;
        }

        // Pressed Cancel
        if(hObjSource == m_cancelButton) {
            m_pQuestion->SetCancelled();
            bHandled = TRUE;
        }

        return(S_OK);
    }

    //--------------------------------------------------------------------------------------
    // Name: VinceQuestionMultiSelect::OnKeyDown
    // Desc: Check for specific key input in the scene
    //--------------------------------------------------------------------------------------
    HRESULT VinceQuestionMultiSelect::OnKeyDown(XUIMessageInput *pInputData, BOOL& bHandled)
    {
        // Pad B button and Escape key cancels
        if(pInputData->dwKeyCode == VK_PAD_B || pInputData->dwKeyCode == VK_ESCAPE) {
            m_pQuestion->SetCancelled();
            bHandled = TRUE;
        }

        return(S_OK);
    }


    //--------------------------------------------------------------------------------------
    // Name: VinceQuestionSlider::OnInit
    // Desc: Initialize the scene
    //--------------------------------------------------------------------------------------
    HRESULT VinceQuestionSlider::OnInit(XUIMessageInit *pInitData, BOOL& bHandled)
    {
        GetChildById(L"XuiLabelQuestion",  &m_question);
        GetChildById(L"XuiLabelSelection", &m_selection);
        GetChildById(L"XuiSliderAnswer",   &m_slider);
        GetChildById(L"XuiButtonAccept",   &m_okButton);
        GetChildById(L"XuiButtonCancel",   &m_cancelButton);

        m_pQuestion = (CSurveyQuestion*)pInitData->pvInitData;
        m_question.SetText(m_pQuestion->GetQuestionText());

        m_slider.SetRange(1, m_pQuestion->GetAnswerCount());
        m_slider.SetValue(1);
        m_selection.SetText(m_pQuestion->GetAnswerText(0));

        return(S_OK);
    }

    //--------------------------------------------------------------------------------------
    // Name: VinceQuestionSlider::OnKeyDown
    // Desc: Check for specific key input in the scene
    //--------------------------------------------------------------------------------------
    HRESULT VinceQuestionSlider::OnKeyDown(XUIMessageInput *pInputData, BOOL& bHandled)
    {
        // Pad A button and Enter accepts change
        if(pInputData->dwKeyCode == VK_PAD_A || pInputData->dwKeyCode == VK_RETURN) {
            int val = 0;
            HRESULT hr = m_slider.GetValue(&val);
            if(FAILED(hr)) {
                return(hr);
            }
            m_pQuestion->SetSelectedAnswer(val - 1);     // Offset by 1 since control is 1-based

            m_pQuestion->SetAnswered();
            bHandled = TRUE;
        }

        // Pad B button and Escape key cancels
        if(pInputData->dwKeyCode == VK_PAD_B || pInputData->dwKeyCode == VK_ESCAPE) {
            m_pQuestion->SetCancelled();
            bHandled = TRUE;
        }

        return(S_OK);
    }

    //--------------------------------------------------------------------------------------
    // Name: VinceQuestionSlider::OnValueChanged
    // Desc: Handles slider value changing
    //--------------------------------------------------------------------------------------
    HRESULT VinceQuestionSlider::OnValueChanged(HXUIOBJ hObjSource, XUINotifyValueChanged *pNotifyValueChangedData, BOOL& bHandled)
    {
        int val = 0;
        HRESULT hr = m_slider.GetValue(&val);
        if(FAILED(hr)) {
            return(hr);
        }
        
        m_selection.SetText(m_pQuestion->GetAnswerText(val - 1));

        return(S_OK);
    }

    //--------------------------------------------------------------------------------------
    // Name: VinceQuestionSlider::OnNotifyPress
    // Desc: Handle input in the scene
    //--------------------------------------------------------------------------------------
    HRESULT VinceQuestionSlider::OnNotifyPress(HXUIOBJ hObjSource, BOOL& bHandled)
    {
        // Pressed Accept
        if(hObjSource == m_okButton) {
            int val = 0;
            HRESULT hr = m_slider.GetValue(&val);
            if(FAILED(hr)) {
                return(hr);
            }
            m_pQuestion->SetSelectedAnswer(val - 1);     // Offset by 1 since control is 1-based

            m_pQuestion->SetAnswered();
            bHandled = TRUE;
        }

        // Pressed Cancel
        if(hObjSource == m_cancelButton) {
            m_pQuestion->SetCancelled();
            bHandled = TRUE;
        }

        return(S_OK);
    }



    //--------------------------------------------------------------------------------------
    // Name: VinceQuestionText::OnInit
    // Desc: Initialize the scene
    //--------------------------------------------------------------------------------------
    HRESULT VinceQuestionText::OnInit(XUIMessageInit *pInitData, BOOL& bHandled)
    {
        GetChildById(L"XuiLabelQuestion", &m_question);
        GetChildById(L"XuiEditAnswer",    &m_edit);
        GetChildById(L"XuiButtonAccept",  &m_okButton);
        GetChildById(L"XuiButtonCancel",  &m_cancelButton);

        m_pQuestion = (CSurveyQuestion*)pInitData->pvInitData;
        m_question.SetText(m_pQuestion->GetQuestionText());

        return(S_OK);
    }

    //--------------------------------------------------------------------------------------
    // Name: VinceQuestionText::OnKeyDown
    // Desc: Check for specific key input in the scene
    //--------------------------------------------------------------------------------------
    HRESULT VinceQuestionText::OnKeyDown(XUIMessageInput *pInputData, BOOL& bHandled)
    {
        // Enter or Pad A button accepts text
        if(pInputData->dwKeyCode == VK_RETURN || pInputData->dwKeyCode == VK_PAD_A) {
            const wchar_t* text = m_edit.GetText();
            m_pQuestion->SetUserText(text);
            m_pQuestion->SetAnswered();
            bHandled = TRUE;
        }

        // Pad B button and Escape key cancels
        if(pInputData->dwKeyCode == VK_PAD_B || pInputData->dwKeyCode == VK_ESCAPE) {
            m_pQuestion->SetCancelled();
            bHandled = TRUE;
        }

        return(S_OK);
    }

    //--------------------------------------------------------------------------------------
    // Name: VinceQuestionText::OnNotifyPress
    // Desc: Handle input in the scene
    //--------------------------------------------------------------------------------------
    HRESULT VinceQuestionText::OnNotifyPress(HXUIOBJ hObjSource, BOOL& bHandled)
    {
        // Pressed Accept
        if(hObjSource == m_okButton) {
            const wchar_t* text = m_edit.GetText();
            m_pQuestion->SetUserText(text);
            m_pQuestion->SetAnswered();
            bHandled = TRUE;
        }

        // Pressed Cancel
        if(hObjSource == m_cancelButton) {
            m_pQuestion->SetCancelled();
            bHandled = TRUE;
        }

        return(S_OK);
    }
}

#endif

