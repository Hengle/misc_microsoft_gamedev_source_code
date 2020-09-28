//-----------------------------------------------------------------------------
// File: keyboard_win32.h
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once
#ifndef KEYBOARD_WIN32
#define KEYBOARD_WIN32

#include "common/utils/utils.h"

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

namespace gr
{
	class KeyboardWin32
	{
		enum { NumKeys = 256 };
		enum { BufferSize = 10 };

		uint mKeyDownTable[NumKeys / 8];
		uint mKeyPressedTable[NumKeys / 8];
		uint mKeyIgnoreTickCount;

		LPDIRECTINPUT8 mpDI;
		LPDIRECTINPUTDEVICE8 mpDevice;

		bool getKeyState(const uint* pTable, int keyIndex) const
		{
			DebugRange(keyIndex, NumKeys);
			return (pTable[keyIndex >> 3] & (1 << (keyIndex & 7))) != 0;
		}
		
		void setKeyState(uint* pTable, int keyIndex, bool newState)
		{
			DebugRange(keyIndex, NumKeys);
			const int ofs = keyIndex >> 3;
			const int mask = 1 << (keyIndex & 7);

			if (newState)
				pTable[ofs] |= mask;
			else
				pTable[ofs] &= ~mask;
		}

	public:
		KeyboardWin32(void) :
			mpDI(NULL),
			mpDevice(NULL)
		{
			Utils::ClearObj(mKeyDownTable);
			Utils::ClearObj(mKeyPressedTable);
		}

		~KeyboardWin32()
		{
			deinit();
		}

		void init(HWND hWnd, HINSTANCE hInstance)
		{
			deinit();

			Verify(SUCCEEDED(DirectInput8Create(hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&mpDI, NULL)));
			Verify(SUCCEEDED(mpDI->CreateDevice(GUID_SysKeyboard, &mpDevice, NULL)));
			Verify(SUCCEEDED(mpDevice->SetDataFormat(&c_dfDIKeyboard)));
			Verify(SUCCEEDED(mpDevice->SetCooperativeLevel(hWnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE)));

			DIPROPDWORD dipdw; 
			dipdw.diph.dwSize = sizeof(DIPROPDWORD); 
			dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER); 
			dipdw.diph.dwObj = 0; 
			dipdw.diph.dwHow = DIPH_DEVICE; 
			dipdw.dwData = BufferSize; 
			Verify(SUCCEEDED(mpDevice->SetProperty(DIPROP_BUFFERSIZE, &dipdw.diph))); 
			
			mKeyIgnoreTickCount = 1;
		}

		void deinit(void)
		{
			if (mpDevice)
			{
				mpDevice->Release();
				mpDevice = NULL;
			}
			
			if (mpDI)
			{
				mpDI->Release();
				mpDI = NULL;
			}
		}

		void update(void)
		{
			if (!mpDevice)
				return;

			Utils::ClearObj(mKeyPressedTable);

			DIDEVICEOBJECTDATA inputBuffer[BufferSize];

			DWORD numEntries = BufferSize;
			HRESULT hres = mpDevice->GetDeviceData(sizeof(DIDEVICEOBJECTDATA), inputBuffer, &numEntries, 0);

			if (FAILED(hres))
			{
				clearState();

				if ((DIERR_NOTACQUIRED == hres) || 
						(DIERR_INPUTLOST == hres))
				{
					mpDevice->Acquire();

					numEntries = BufferSize;
					hres = mpDevice->GetDeviceData(sizeof(DIDEVICEOBJECTDATA), inputBuffer, &numEntries, 0);
				}
				
				if (FAILED(hres))
					return;
			}
			
			if (mKeyIgnoreTickCount)
			{
				mKeyIgnoreTickCount--;
				return;
			}
			
			for (DWORD i = 0; i < numEntries; i++)
			{
				const DWORD scanCode = inputBuffer[i].dwOfs;
				const bool pressed = (inputBuffer[i].dwData & 0x80) != 0;

				if (scanCode < NumKeys)
				{
					if (pressed)
					{
						setKeyState(mKeyDownTable, scanCode, true);
						setKeyState(mKeyPressedTable, scanCode, true);
					}
					else
					{
						setKeyState(mKeyDownTable, scanCode, false);
						setKeyState(mKeyPressedTable, scanCode, false);
					}
				}
			}
		}

		void clearState(void)
		{
			Utils::ClearObj(mKeyDownTable);
			Utils::ClearObj(mKeyPressedTable);
		}
		
		void ignoreKeys(int tickCount)
		{
			mKeyIgnoreTickCount = tickCount;
		}

		bool keyDown(int diKey) const
		{
			return getKeyState(mKeyDownTable, diKey);
		}

		bool keyDownOrPressed(int diKey) const
		{
			return getKeyState(mKeyDownTable, diKey) || getKeyState(mKeyPressedTable, diKey);
		}

		bool keyPressed(int diKey) const
		{
			return getKeyState(mKeyPressedTable, diKey);
		}

		int vkeyToDIKey(int vkey) const
		{
			switch (vkey)
			{
				case '0': return DIK_0;
				case '1': return DIK_1; 
				case '2': return DIK_2; 
				case '3': return DIK_3; 
				case '4': return DIK_4; 
				case '5': return DIK_5;
				case '6': return DIK_6;
				case '7': return DIK_7;
				case '8': return DIK_8;
				case '9': return DIK_9;
				case 'A': return DIK_A;
				case 'B': return DIK_B;
				case 'C': return DIK_C;
				case 'D': return DIK_D;
				case 'E': return DIK_E;
				case 'F': return DIK_F;
				case 'G': return DIK_G;
				case 'H': return DIK_H;
				case 'I': return DIK_I;
				case 'J': return DIK_J;
				case 'K': return DIK_K;
				case 'L': return DIK_L;
				case 'M': return DIK_M;
				case 'N': return DIK_N;
				case 'O': return DIK_O;
				case 'P': return DIK_P;
				case 'Q': return DIK_Q;
				case 'R': return DIK_R;
				case 'S': return DIK_S;
				case 'T': return DIK_T;
				case 'U': return DIK_U;
				case 'V': return DIK_V;
				case 'W': return DIK_W;
				case 'X': return DIK_X;
				case 'Y': return DIK_Y;
				case 'Z': return DIK_Z;
				case VK_DOWN: return DIK_DOWN;                /* DownArrow on arrow keypad */
				case VK_END: return DIK_END;                 /* End on arrow keypad */
				case VK_RETURN: return DIK_RETURN;              /* Enter on main keyboard */
				case VK_ESCAPE: return DIK_ESCAPE;
				case VK_F10: return DIK_F10;
				case VK_F11: return DIK_F11;
				case VK_F12: return DIK_F12;
				case VK_F1: return DIK_F1;
				case VK_F2: return DIK_F2;
				case VK_F3: return DIK_F3;
				case VK_F4: return DIK_F4;
				case VK_F5: return DIK_F5;
				case VK_F6: return DIK_F6;
				case VK_F7: return DIK_F7;
				case VK_F8: return DIK_F8;
				case VK_F9: return DIK_F9;
				case VK_HOME: return DIK_HOME;                /* Home on arrow keypad */
				case VK_LCONTROL: return DIK_LCONTROL;
				case VK_LEFT: return DIK_LEFT;                /* LeftArrow on arrow keypad */
				case VK_LMENU: return DIK_LMENU;               /* left Alt */
				case VK_LSHIFT: return DIK_LSHIFT;
				case VK_NEXT: return DIK_NEXT;                /* PgDn on arrow keypad */
				case VK_PRIOR: return DIK_PRIOR;               /* PgUp on arrow keypad */
				case VK_RCONTROL: return DIK_RCONTROL;
				case VK_RIGHT: return DIK_RIGHT;               /* RightArrow on arrow keypad */
				case VK_RMENU: return DIK_RMENU;               /* right Alt */
				case VK_RSHIFT: return DIK_RSHIFT;
				case VK_SPACE: return DIK_SPACE;
				case VK_TAB: return DIK_TAB;
				case VK_UP: return DIK_UP;                  /* UpArrow on arrow keypad */
			}
			return -1;
		}
	};

	extern KeyboardWin32 gKeyboard;
} // namespace gr

#endif // KEYBOARD_WIN32
