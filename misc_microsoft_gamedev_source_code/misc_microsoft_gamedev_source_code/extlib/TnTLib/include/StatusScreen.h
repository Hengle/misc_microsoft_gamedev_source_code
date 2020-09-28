//--------------------------------------------------------------------------------------
//	StatusScreen : Ticket Tracker status screen
//
//  Created 2006/07/16 Dan Berke     <dberke@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//  Microsoft Game Studios Tools and Technology (TnT)
//	Copyright (c) 2006 Microsoft Corp.  All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include <xtl.h>

namespace StatusScreen 
{
	class StatusFont;

	class StatusScreen     
	{
    public:
                 StatusScreen();
        virtual ~StatusScreen();

		HRESULT Initialize(IDirect3DDevice9* pDevice, char* fontFile);
		HRESULT ClearScreen(DWORD color);
		HRESULT DrawText(float x, float y, DWORD color, const wchar_t* msg);
		HRESULT Present();

	private:
		StatusFont*		  m_pFont;
		IDirect3DDevice9* m_pd3dDevice;
    };

}
