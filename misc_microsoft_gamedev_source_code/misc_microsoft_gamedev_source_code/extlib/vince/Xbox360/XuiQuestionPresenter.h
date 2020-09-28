//  XuiQuestionPresenter.h - XUI survey renderer implementation
//
//	Created 2005/09/13 Dan Berke <dberke@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2005 Microsoft Corp.  All rights reserved.

#pragma once

#include "VinceControl.h"      // To pick up VINCE_XUI

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
#include <initguid.h>
#include "xuidevice.h"
#endif

#include "xui.h"
#include "xuiapp.h" 
#include "IQuestionPresenter.h"

namespace Vince {

    class XuiQuestionPresenter : public CXuiModule, public IQuestionPresenter
    {
        public:
            XuiQuestionPresenter();
            virtual ~XuiQuestionPresenter();

            virtual HRESULT InitDevice(IDirect3DDevice9 *pDevice, D3DPRESENT_PARAMETERS *pD3Dpp, HWND hWnd);

            virtual bool PresentQuestion(CSurveyQuestion* pQuestion);
            virtual bool PresentQuestionAsync(CSurveyQuestion* pQuestion);
            virtual void UpdateAsync();
            virtual void HideQuestion();

            virtual void BeginSurvey(); 
            virtual void EndSurvey();   
            virtual bool ProcessInputMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* pResult);

            virtual void SetSurveyPos(float x, float y) { m_posX = x; m_posY = y; }
            virtual void SetSurveyScale(float scaleX, float scaleY) { m_scaleX = scaleX; m_scaleY = scaleY; }

        private:
            HRESULT RegisterXuiClasses();
            HRESULT UnregisterXuiClasses();
            
            wchar_t* GetXuiFilename();
            void RenderQuestion(CSurveyQuestion* pQuestion);

            IDirect3DDevice9* m_pd3dDevice;
            CSurveyQuestion*  m_pCurQuestion;

            float m_posX;
            float m_posY;
            float m_scaleX;
            float m_scaleY;

#ifndef _XBOX
            HRESULT InitXuiPC(IDirect3DDevice9 *pD3DDevice, D3DPRESENT_PARAMETERS *pD3Dpp, HWND hWnd);
            void    UninitXuiPC();
            HWND    m_hWnd;
#endif
    };
}

#endif
