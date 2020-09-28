//	ControllerXe.cpp : Class to manage controller for survey input
//	and encapsulate all Xenon-dependent input calls.
//  A single header file is used for all project versions,
//  although platform specific .cpp files are implemented as
//  required.
//
//	Created 2005/01/07 Rich Bonny <rbonny@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2005 Microsoft Corp.  All rights reserved.

#include "VinceControl.h"

#ifdef _VINCE_

#include "Controller.h"

namespace Vince
{
	CController* CController::s_pInstance = 0;// initialize pointer
	CController* CController::Instance()
	{
		if (s_pInstance == 0)  // is it the first call?
		{  
			s_pInstance = new CController(); // create sole instance
		}
		return s_pInstance; // address of sole instance
	}

	void CController::DestroyInstance() 
	{
		if (s_pInstance != 0)  // has it been initialized?
		{  
			delete s_pInstance; // free memory
			s_pInstance = 0;	// clear instance pointer
		}
	}

	CController::CController()
	{
		dwNextAllowedTime = 0;
		fButtonReleased = true;
		m_keyBuffer[0] = L'\0';
		m_keyBufferSize = 0;
	}

	CController::InputPress CController::CheckInput()
	{
		// Enforce a maximum repeat rate
		DWORD dwEntryTime = GetTickCount();
		if (dwEntryTime < dwNextAllowedTime)
		{
			return Input_None;
		}

		XINPUT_GAMEPAD* pGamepad = NULL;

		for( DWORD i=0; i < XUSER_MAX_COUNT; i++ )
		{
			// Read the input state. First zero out InputState to keep Prefix happy
			XINPUT_STATE InputState;
			ZeroMemory(&InputState, sizeof(XINPUT_STATE));

			if ( XInputGetState( i, &InputState ) == ERROR_SUCCESS )
			{
				pGamepad = &InputState.Gamepad;
				break;
			}
		}

		// If no gamepads found, return no input.

		if ( NULL == pGamepad )
		{
			return Input_None;
		}

		// Check for A button. Ignore repeat if it is being held down.

		if (pGamepad->wButtons & XINPUT_GAMEPAD_A )
		{
			dwNextAllowedTime = dwEntryTime + 250;	// 1/4 second delay
			if (fButtonReleased)
			{
				fButtonReleased = false;
				return Input_Ok;
			}
			else
			{
				return Input_None;
			}
		}

		// Check for B button

		if (pGamepad->wButtons & XINPUT_GAMEPAD_B)
		{
			dwNextAllowedTime = dwEntryTime + 500;	// 1/2 second delay
			return Input_Cancel;
		}

		// Check for X button

		if (pGamepad->wButtons & XINPUT_GAMEPAD_X)
		{
			dwNextAllowedTime = dwEntryTime + 500;	// 1/2 second delay
			return Input_Check;
		}

		// Check DPad Up

		if (pGamepad->wButtons & XINPUT_GAMEPAD_DPAD_UP)
		{
			dwNextAllowedTime = dwEntryTime + 250;	// 1/4 second delay
			return Input_Up;
		}

		// Check DPad Down

		if (pGamepad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN)
		{
			dwNextAllowedTime = dwEntryTime + 250;	// 1/4 second delay
			return Input_Down;
		}

		// Check DPad Left

		if (pGamepad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT)
		{
			dwNextAllowedTime = dwEntryTime + 250;	// 1/4 second delay
			return Input_Left;
		}

		// Check DPad Right

		if (pGamepad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)
		{
			dwNextAllowedTime = dwEntryTime + 250;	// 1/4 second delay
			return Input_Right;
		}

		// Check if Left Thumbstick sufficiently up

		if (pGamepad->sThumbLY > 15000)
		{
			dwNextAllowedTime = dwEntryTime + 250;	// 1/4 second delay
			return Input_Up;
		}

		// Check if Left Thumbstick sufficiently down

		if (pGamepad->sThumbLY < -15000)
		{
			dwNextAllowedTime = dwEntryTime + 250;	// 1/2 second delay
			return Input_Down;
		}

		// Check if Left Thumbstick sufficiently left

		if (pGamepad->sThumbLX < -15000)
		{
			dwNextAllowedTime = dwEntryTime + 250;	// 1/4 second delay
			return Input_Left;
		}

		// Check if Left Thumbstick sufficiently right

		if (pGamepad->sThumbLX > 15000)
		{
			dwNextAllowedTime = dwEntryTime + 250;	// 1/2 second delay
			return Input_Right;
		}

		// If none of the above, ignore anything else
		fButtonReleased = true;
		return Input_None;
	}

	// Return text entered so far.
	// Also check for new keystrokes.
	wchar_t* CController::GetKeyboardText()
	{
		// Structure to contain the returned keystroke
		XINPUT_KEYSTROKE keystroke;
		ZeroMemory(&keystroke, sizeof(XINPUT_KEYSTROKE));
		// Error code when fetching keystroke
		BYTE errorCode = 1;

		// Read the input state. First zero out InputState to keep Prefix happy
		XINPUT_STATE InputState;
		ZeroMemory(&InputState, sizeof(XINPUT_STATE));

		// Any attached keyboard will appear to be attached to all ports, so we
		// only need to check port 0

		if ( XInputGetState( 0, &InputState ) == ERROR_SUCCESS )
		{
			XINPUT_CAPABILITIES caps;
			DWORD retcode = XInputGetCapabilities( 0, XINPUT_FLAG_KEYBOARD, &caps );
			if (S_OK == retcode)
			{
				retcode = XInputGetKeystroke(0, XINPUT_FLAG_KEYBOARD, &keystroke);
				errorCode = (BYTE) (retcode & 0x000000ff);
			}
		}

		// If we found a keyboard, extract any keystrokes
		if (0 == errorCode)
		{
			// Only process keystrokes if they are keydown events
			if ( keystroke.Flags & XINPUT_KEYSTROKE_KEYDOWN )
			{
				wchar_t unicode = keystroke.Unicode;

				// Some special cases for special characters
				if ( unicode == 27 ) // Escape character
				{
					m_keyBuffer[0] = 0;
					m_keyBufferSize = 0;
				}
				else if ( unicode == 8 )	// back space
				{
					if (m_keyBufferSize > 0)
					{
						m_keyBuffer[--m_keyBufferSize] = 0;
					}
				}
				else if ( unicode == 10 )	// line feed
				{
					// Ignore, since it will screw up the display
				}
				else if (0 != unicode && m_keyBufferSize < CONTROLLER_TEXT_BUFFER_SIZE)
				{
					m_keyBuffer[m_keyBufferSize++] = unicode;
					m_keyBuffer[m_keyBufferSize] = 0;
				}
			}
		}
		return &m_keyBuffer[0];
	}

	void CController::ClearKeyboardText()
	{
		m_keyBuffer[0] = L'\0';
		m_keyBufferSize = 0;
	}

}

#endif // _VINCE_