//--------------------------------------------------------------------------------------
//	TTStatus : Sample TicketTracker status screen
//
//  Created 2007/01/25 Dan Berke     <dberke@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//  Microsoft Game Studios Tools and Technology (TnT)
//	Copyright (c) 2007 Microsoft Corp.  All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#ifdef _TICKET_TRACKER_

#include <xtl.h>

#include "TicketTracker.h"
#include "StatusScreen.h"

class TTStatus : public TicketTracker::IStatusListener
{
	public:
				 TTStatus();
		virtual ~TTStatus();

		HRESULT Initialize(IDirect3DDevice9* pDevice, char *fontFile);
		void	DrawText(float x, float y, DWORD color, wchar_t* text);

		virtual void OnBeginTicketTracker();
		virtual void OnFinishTicketTracker();
		virtual void OnTakeScreenshot(const char* filename);
		virtual void OnCreateMinidump(const char* filename);
		virtual void OnCompressFile(const char* filename);
		virtual void OnEncryptFile(const char* filename);
		virtual void OnCreateBuildVersionFile(const char* filename);
		virtual void OnStartUpload(const char* uploaderName);
		virtual void OnFinishUpload(TicketTracker::TTResult result);
		virtual void OnStartUploadFile(const char* filename);
		virtual void OnFinishUploadFile(const char* filename, TicketTracker::TTResult result);

	private:
		void RenderMessageScreen(const char* msg, const char* filename);

		StatusScreen::StatusScreen m_statusScreen;
};

#endif // _TICKET_TRACKER_