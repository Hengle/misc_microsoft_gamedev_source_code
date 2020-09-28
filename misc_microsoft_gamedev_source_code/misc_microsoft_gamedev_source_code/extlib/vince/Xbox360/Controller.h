//	Controller.h : Class to manage controller for survey input,
//	and encapsulate all platform dependent calls (PC, Xbox and Xenon).
//  A single header file is used for all project versions,
//  although platform specific .cpp files are implemented as
//  required.
//
//	Created 2004/03/10 Rich Bonny <rbonny@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2004 Microsoft Corp.  All rights reserved.

#pragma once

// The following includes will depend on the platform

#ifdef _XBOX
	#include <XTL.h>
#else
	#include <Windows.h>
#endif

#define CONTROLLER_TEXT_BUFFER_SIZE 256
namespace Vince
{
	// CController is a singleton class used for survey responses
	class CController
	{
	public:
		static CController* Instance();
		static void DestroyInstance();
		enum InputPress 
		{
			Input_None,
			Input_Up,
			Input_Down,
			Input_Left,
			Input_Right,
			Input_Ok,
			Input_Check,
			Input_Cancel,
		};

		CController::InputPress CheckInput();
		wchar_t* GetKeyboardText();
		void ClearKeyboardText();

	protected:
		CController();
		CController(const CController&);
		CController& operator= (const CController&);
		DWORD dwNextAllowedTime;
		bool fButtonReleased;
		wchar_t m_keyBuffer[CONTROLLER_TEXT_BUFFER_SIZE];
		size_t m_keyBufferSize;

	private:
		static CController* s_pInstance;
	};
}
