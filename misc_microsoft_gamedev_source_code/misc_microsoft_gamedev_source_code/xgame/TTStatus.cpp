//--------------------------------------------------------------------------------------
//	TTStatus : Sample TicketTracker status screen
//
//  Created 2007/01/25 Dan Berke     <dberke@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//  Microsoft Game Studios Tools and Technology (TnT)
//	Copyright (c) 2007 Microsoft Corp.  All rights reserved.
//--------------------------------------------------------------------------------------
#include "common.h"

#ifdef _TICKET_TRACKER_

#include "TTStatus.h"
#include "TnTUtil.h"

#include "renderThread.h"

TTStatus::TTStatus()
{
}

TTStatus::~TTStatus()
{
}

//--------------------------------------------------------------------------------------
// Initialization
//--------------------------------------------------------------------------------------
HRESULT TTStatus::Initialize(IDirect3DDevice9* pDevice, char *fontFile)
{
	return(m_statusScreen.Initialize(pDevice, fontFile));
}

//--------------------------------------------------------------------------------------
// Generic text display
//--------------------------------------------------------------------------------------
void TTStatus::DrawText(float x, float y, DWORD color, wchar_t* text)
{
	m_statusScreen.DrawText(x, y, color, text);
}

//--------------------------------------------------------------------------------------
// TicketTracker status message display functions
//--------------------------------------------------------------------------------------
void TTStatus::OnBeginTicketTracker()
{
	RenderMessageScreen("TicketTracker data collection starting...", NULL);
}

void TTStatus::OnFinishTicketTracker()
{
    bool bCounter = false;
    int nSecondsTilResume = 20;
    DWORD color = 0;

	wchar_t sStatus[128] = L"";
	while(nSecondsTilResume > 0) {
		gRenderThread.releaseThreadOwnership();
		m_statusScreen.ClearScreen(0xff000000);
    	m_statusScreen.DrawText(50.0f, 20.0f,  0xffffff00, L"Ticket Tracker Version " TT_VERSION);

		// give the user some feedback on how the upload went
		if(bCounter) {
			color = 0xffff0000;
		} else {
			color = 0xffffffff;
		}
		bCounter = !bCounter;
		_snwprintf_s(sStatus, 128, 127, L"Processing done!\nYou may now reboot or manually debug as necessary!\nDeferring to debugger in %u seconds...", nSecondsTilResume/2);
		nSecondsTilResume--;
		
		m_statusScreen.DrawText(50.0f, 100.0f, color, sStatus);
		m_statusScreen.Present();
		gRenderThread.acquireThreadOwnership();
		Sleep(500);
	}
}

void TTStatus::OnTakeScreenshot(const char* filename)
{
	RenderMessageScreen("Took screenshot:", filename);
}

void TTStatus::OnCreateMinidump(const char* filename)
{
	RenderMessageScreen("Creating minidump:", filename);
}

void TTStatus::OnCompressFile(const char* filename)
{
	RenderMessageScreen("Compressing file:", filename);
}

void TTStatus::OnEncryptFile(const char* filename)
{
	RenderMessageScreen("Encrypting file:", filename);
}

void TTStatus::OnCreateBuildVersionFile(const char* filename)
{
	RenderMessageScreen("Creating build version file:", filename);
}

void TTStatus::OnStartUpload(const char* uploaderName)
{
	RenderMessageScreen("Starting upload:", uploaderName);
}

void TTStatus::OnFinishUpload(TicketTracker::TTResult result)
{
	const char* msg = TicketTracker::ITicketTracker::GetErrorText(result);
	RenderMessageScreen("Finished upload:", msg);
}

void TTStatus::OnStartUploadFile(const char* filename)
{
	RenderMessageScreen("Uploading file:", filename);
}

void TTStatus::OnFinishUploadFile(const char* filename, TicketTracker::TTResult result)
{
    const char* msg = TicketTracker::ITicketTracker::GetErrorText(result);
	RenderMessageScreen("Finished file upload:", msg);
}

//--------------------------------------------------------------------------------------
// Render common elements on the screen as well as two message lines
//--------------------------------------------------------------------------------------
void TTStatus::RenderMessageScreen(const char* msg, const char *filename)
{
	gRenderThread.releaseThreadOwnership();
	m_statusScreen.ClearScreen(0xff000000);
	m_statusScreen.DrawText(50.0f, 20.0f,  0xffffff00, L"Ticket Tracker Version " TT_VERSION);

	wchar_t* wMsg = TnT::MakeWide(msg);
	m_statusScreen.DrawText(50.0f, 100.0f, 0xffffffff, wMsg);
	delete[] wMsg;

	if(filename != NULL) {
		wchar_t* wFilename = TnT::MakeWide(filename);
		m_statusScreen.DrawText(50.0f, 130.0f, 0xffffffff, wFilename);
		delete[] wFilename;
	}

	m_statusScreen.Present();
	gRenderThread.acquireThreadOwnership();
	Sleep(1000);
}

#endif // _TICKET_TRACKER_