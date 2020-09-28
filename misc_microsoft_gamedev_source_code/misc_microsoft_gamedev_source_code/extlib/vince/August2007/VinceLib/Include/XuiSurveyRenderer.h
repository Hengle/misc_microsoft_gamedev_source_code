//  XuiSurveyRenderer.h - XUI survey renderer implementation
//
//	Created 2005/09/13 Dan Berke <dberke@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2005 Microsoft Corp.  All rights reserved.

#pragma once

#ifdef _XBOX

#include <xtl.h>
#include <xui.h>
#include <xuiapp.h>
#include "Survey.h"

namespace SurveyLib {

    class XuiSurveyRenderer : public CXuiModule, public ISurveyRenderer
    {
        public:
				     XuiSurveyRenderer();
            virtual ~XuiSurveyRenderer();

			// Refcounting
			virtual void AddRef();
			virtual void Release();

            HRESULT Initialize(IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* pD3Dpp);
			HRESULT Shutdown();

			HRESULT LoadXuiFont(wchar_t* fontName, wchar_t* fontFile);
			HRESULT LoadXuiSkin(wchar_t* skinFile);
            void	SetSurveyPos(float x, float y);
            void	SetSurveyScale(float scaleX, float scaleY);

			// ISurveyRenderer implementation
            virtual HRESULT BeginSurvey(Survey* pSurvey); 
            virtual HRESULT EndSurvey(Survey* pSurvey);   
            virtual HRESULT BeginQuestion(SurveyQuestion* pQuestion);
            virtual HRESULT EndQuestion(SurveyQuestion* pQuestion);
			virtual HRESULT RenderQuestion(SurveyQuestion* pQuestion);
			virtual HRESULT ProcessQuestionInput(SurveyQuestion* pQuestion);

        private:
            HRESULT  RegisterXuiClasses();
            HRESULT	 UnregisterXuiClasses();
            wchar_t* GetXuiFilename(SurveyQuestion* pQuestion);

            IDirect3DDevice9* m_pd3dDevice;
            float			  m_posX;
            float			  m_posY;
            float			  m_scaleX;
            float			  m_scaleY;
			long			  m_refCount;
    };
}

#endif
