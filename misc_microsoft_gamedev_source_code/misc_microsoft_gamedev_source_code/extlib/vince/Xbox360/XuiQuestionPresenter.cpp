//  XuiQuestionPresenter.cpp - XUI survey renderer implementation
//
//	Created 2005/09/13 Dan Berke <dberke@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2005 Microsoft Corp.  All rights reserved.


#include "XuiQuestionPresenter.h"

#ifdef VINCE_XUI

#include "Vince.h"
#include "TnTUtil.h"
#include "XuiQuestionPages.h"
#include "Display.h"

// Required XUI libraries
#ifdef _XBOX
#pragma comment(lib, "xact.lib")
#pragma comment(lib, "xuiruna.lib")
#pragma comment(lib, "xuirender.lib")
#endif

namespace Vince {

    //--------------------------------------------------------------------------------------
    // Name: Constructor
    // Desc: Object setup
    //--------------------------------------------------------------------------------------
    XuiQuestionPresenter::XuiQuestionPresenter()
    {
        m_pd3dDevice   = NULL;
        m_pCurQuestion = NULL;
        m_scaleX       = 1.0f;
        m_scaleY       = 1.0f;
        m_posX         = 0.0f;
        m_posY         = 0.0f;
    }

    //--------------------------------------------------------------------------------------
    // Name: Destructor
    // Desc: Object cleanup
    //--------------------------------------------------------------------------------------
    XuiQuestionPresenter::~XuiQuestionPresenter()
    {
#if _XBOX
        XuiUninit();
        XuiRenderUninit();
#else
        UninitXuiPC();
#endif
    }

    //--------------------------------------------------------------------------------------
    // Name: Init
    // Desc: Initializes the XUI runtime 
    //--------------------------------------------------------------------------------------
    HRESULT XuiQuestionPresenter::InitDevice(IDirect3DDevice9 *pD3DDevice, D3DPRESENT_PARAMETERS *pD3Dpp, HWND hWnd)
    {
		if(m_pd3dDevice != NULL) {
			return(S_OK);
		}

#ifdef _XBOX
        HRESULT hr = InitShared(pD3DDevice, pD3Dpp, XuiD3DXTextureLoader);
        if(FAILED(hr)) {
            return(hr);
        }
        char *defaultFontName = "Arial Unicode MS";
        char *defaultFontFile = "file://game:/Vince/xarialuni.ttf";
#else
        HRESULT hr = InitXuiPC(pD3DDevice, pD3Dpp, hWnd);
        if(FAILED(hr)) {
            return(hr);
        }
        char *defaultFontName = "Arial Unicode MS";
        char *defaultFontFile = "file://arial.ttf";
#endif

        // Get font files
        const char* fontName = g_pVince->pSettings->Fetch("XuiFontName", defaultFontName);
        wchar_t wideName[256];
		TnT::CopyWide(fontName, (WCHAR*)&wideName, 256);
        const char* fontFile = g_pVince->pSettings->Fetch("XuiFontFile", defaultFontFile);
        wchar_t wideFile[256];
		TnT::CopyWide(fontFile, (WCHAR*)&wideFile, 256);

        // Register the fonts to use
        hr = RegisterDefaultTypeface(wideName, wideFile);
        if(FAILED(hr)) {
            OutputDebugString("RegisterDefaultTypeface() failed\n");
            return(hr);
        }

        // Load skin file
        const char* skinFile = g_pVince->pSettings->Fetch("XuiSkinFile", "file://vincexui.xzp#vinceskin.xur");
        wchar_t wideSkinFile[256];
		TnT::CopyWide(skinFile, (WCHAR*)&wideSkinFile, 256);
        XuiFreeVisuals(NULL);
        hr = XuiVisualSetBasePath(wideSkinFile, NULL);
        if(FAILED(hr)) {
            OutputDebugString("XuiVisualSetBasePath() failed\n");
            return(hr);
        }

        hr = LoadSkin(wideSkinFile);
        if(FAILED(hr)) {
            OutputDebugString("LoadSkin() failed\n");
            return(hr);
        }

        // Save device on success
        m_pd3dDevice = pD3DDevice;

        return(S_OK);
    }

    HRESULT XuiQuestionPresenter::RegisterXuiClasses()
    {
        VinceQuestionSingleSelect::Register();
        VinceQuestionMultiSelect::Register();
        VinceQuestionSlider::Register();
        VinceQuestionText::Register();
        return(S_OK);
    }

    HRESULT XuiQuestionPresenter::UnregisterXuiClasses()
    {
        VinceQuestionSingleSelect::Unregister();
        VinceQuestionMultiSelect::Unregister();
        VinceQuestionSlider::Unregister();
        VinceQuestionText::Unregister();
        return(S_OK);
    }

    wchar_t* XuiQuestionPresenter::GetXuiFilename()
    {
        // Select which file to use based on the question type
        wchar_t* xuiFile = NULL;
        switch(m_pCurQuestion->QuestionType) {
            case MultipleChoice:
                xuiFile = L"vincexui.xzp#VinceQuestionSingleSelect.xur";
                break;
            case MultiSelect:
                xuiFile = L"vincexui.xzp#VinceQuestionMultiSelect.xur";
                break;
            case HorizontalScale:
                xuiFile = L"vincexui.xzp#VinceQuestionSlider.xur";
                break;
            case FreeformText:
                xuiFile = L"vincexui.xzp#VinceQuestionText.xur";
                break;
        }

        return(xuiFile);
    }

    void XuiQuestionPresenter::BeginSurvey()
	{
	}

	void XuiQuestionPresenter::EndSurvey()
	{
		CDisplay::Instance()->End();
		Uninit();
	}

    //--------------------------------------------------------------------------------------
    // Name: PresentQuestion
    // Desc: Shows the specified question and does a render loop until it is answered.
    //       Returns true if answered, false if cancelled.
    //--------------------------------------------------------------------------------------
    bool XuiQuestionPresenter::PresentQuestion(CSurveyQuestion* pQuestion)
    {
        m_pCurQuestion = pQuestion;

        wchar_t* xuiFile = GetXuiFilename();

#ifdef _XBOX
        wchar_t* basePath = L"file://game:/Vince/";
#else
        wchar_t* basePath = L"file://";
#endif

        // Load the question XUI scene file
        HRESULT hr = LoadFirstScene(basePath, xuiFile, m_pCurQuestion);
        if(FAILED(hr)) {
            return(false);
        }

        for(;;) {
            // Draw the question
      		m_pd3dDevice->Clear(0L, NULL, D3DCLEAR_TARGET, 0xFF000000, 1.0f, 0L);
            RenderQuestion(m_pCurQuestion);
            m_pd3dDevice->Present(0, 0, 0, 0);

            // Send input to XUI
#ifdef _XBOX
            XINPUT_KEYSTROKE k;
            ZeroMemory(&k, sizeof(k));
            XInputGetKeystroke(XUSER_INDEX_ANY, XINPUT_FLAG_ANYDEVICE, &k);
            XuiProcessInput(&k);
#else 
            MSG msg;
	        if(PeekMessage(&msg, m_hWnd, 0U, 0U, PM_REMOVE)) {
	            TranslateMessage(&msg);
	            DispatchMessage(&msg);
 	        }	
#endif

            // If question was answered, hide the question and return true
            if(m_pCurQuestion->WasAnswered()) {
                HideQuestion();
                return(true);
            }
 
            // If question was cancelled, hide the question and return false
            if(m_pCurQuestion->WasCancelled()) {
                HideQuestion();
                return(false);
            }
        }
    }

    //--------------------------------------------------------------------------------------
    // Name: PresentQuestionAsync
    // Desc: Initiates asynchronous display of a question.
    //--------------------------------------------------------------------------------------
    bool XuiQuestionPresenter::PresentQuestionAsync(CSurveyQuestion* pQuestion)
    {
        m_pCurQuestion = pQuestion;

        wchar_t* xuiFile = GetXuiFilename();

#ifdef _XBOX
        wchar_t* basePath = L"file://game:/Vince/";
#else
        wchar_t* basePath = L"file://";
#endif

        // Load the question XUI scene file
        HRESULT hr = LoadFirstScene(basePath, xuiFile, m_pCurQuestion);
        if(FAILED(hr)) {
            return(false);
        }

        return(true);
    }

    //--------------------------------------------------------------------------------------
    // Name: UpdateAsync
    // Desc: Renders question asynchonously and checks for input
    //--------------------------------------------------------------------------------------
    void XuiQuestionPresenter::UpdateAsync()
    {
        if(m_pCurQuestion != NULL) {
            // Draw the question
            RenderQuestion(m_pCurQuestion);

#ifdef _XBOX
            // Send input to XUI
            XINPUT_KEYSTROKE k;
            ZeroMemory(&k, sizeof(k));
            XInputGetKeystroke(XUSER_INDEX_ANY, XINPUT_FLAG_ANYDEVICE, &k);
            XuiProcessInput(&k);
#endif
            // If question was answered, hide the question and return true
            if(m_pCurQuestion->WasAnswered()) {
                HideQuestion();
            }
 
            // If question was cancelled, hide the question and return false
            if(m_pCurQuestion->WasCancelled()) {
                HideQuestion();
            }

        }
    }

    //--------------------------------------------------------------------------------------
    // Name: RenderQuestion
    // Desc: Render the current question
    //--------------------------------------------------------------------------------------
    void XuiQuestionPresenter::RenderQuestion(CSurveyQuestion *q)
    {
        // Update UI animations
        RunFrame();

        XuiTimersRun();
        XuiRenderBegin(GetDC(), D3DCOLOR_ARGB(255, 0, 0, 0));

        D3DXMATRIX matOrigView;
        XuiRenderGetViewTransform(GetDC(), &matOrigView);

        // scale depending on the width of the render target
        D3DXMATRIX matScale;
        D3DXMatrixScaling(&matScale, m_scaleX, m_scaleY, 1);
        D3DXMATRIX matTrans;
        D3DXMatrixTranslation(&matTrans, m_posX, m_posY, 0.0f);
        D3DXMATRIX matView;
        D3DXMatrixMultiply(&matView, &matScale, &matTrans);
        XuiRenderSetViewTransform(GetDC(), &matView);

        XUIMessage msg;
        memset(&msg, 0x00, sizeof(msg));
        msg.cbSize = sizeof(XUIMessage);
        msg.cbData = sizeof(XUIMessageRender);
        msg.dwMessage = XM_RENDER;

        XUIMessageRender msgRender;
        memset(&msgRender, 0x00, sizeof(msgRender));
        msgRender.hDC = GetDC();
        msgRender.dwColorFactor = 0xffffffff;
        msg.pvData = &msgRender;
        msgRender.nBlendMode = XUI_BLEND_NORMAL;
        XuiSendMessage(GetRootObj(), &msg);

        XuiRenderSetViewTransform(GetDC(), &matOrigView);
        XuiRenderEnd(GetDC());
    }

    //--------------------------------------------------------------------------------------
    // Name: HideQuestion
    // Desc: Hides the current question
    //--------------------------------------------------------------------------------------
    void XuiQuestionPresenter::HideQuestion()
    {
        if(GetRootObj() != NULL) {
            CXuiElement canvas;
            CXuiElement element;

            canvas.Attach(m_hObjRoot);

            // Destroy all UI elements under the canvas
            canvas.GetFirstChild(&element);
            while(element.m_hObj != NULL) {
                if(element.m_hObj != NULL) {
	                element.Destroy();
                }
                canvas.GetFirstChild(&element);
            }
        }
    }

    //--------------------------------------------------------------------------------------
    // Name: ProcessInputMessage
    // Desc: Process window messages for the current question
    // Note: Xenon implementation here, PC version in PcXui.cpp.
    //--------------------------------------------------------------------------------------
#ifdef _XBOX
    bool XuiQuestionPresenter::ProcessInputMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* pResult)
    {
        return(true);
    }
#endif
}


#endif  // VINCE_XUI


