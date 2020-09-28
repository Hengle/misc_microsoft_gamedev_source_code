//-----------------------------------------------------------------------------
// File: joystick_win32.h
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once
#ifndef JOYSTICK_WIN32_H
#define JOYSTICK_WIN32_H

#include "common/utils/utils.h"
#include "common/math/vector.h"

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

namespace gr
{
	enum EGamePadButton
	{
		eButtonA,
		eButtonB,
		eButtonX,
		eButtonY,
		eButtonBlack,
		eButtonWhite,
		eButtonStart,
		eButtonBack,
		eButtonLeftThumb,
		eButtonRightThumb,

		eNumButtons
	};

	class JoystickWin32
	{
		enum { AxisScale = 4096 };
		enum { AutoRepeatTime = 65, AutoRepeatThresh = 250 };

		LPDIRECTINPUT8 mpDI;
		LPDIRECTINPUTDEVICE8 mpDevice;
		
		DIJOYSTATE2 mDevState;
		bool mDevStateValid;

		DIJOYSTATE2 mPrevDevState;
		bool mPrevDevStateValid;

		DWORD mDPadDownTickCount;

		DWORD mPrevUpdateTickCount;
		DWORD mAutoRepeatTimer;
		bool mCanAutoRepeat;

		static BOOL CALLBACK DIEnumDevicesCallback(LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef)
		{
			JoystickWin32* pThis = reinterpret_cast<JoystickWin32*>(pvRef);

			Assert(pThis);

			HRESULT hres = pThis->mpDI->CreateDevice(lpddi->guidInstance, &pThis->mpDevice, NULL);

			if (FAILED(hres)) 
				return DIENUM_CONTINUE;

			return DIENUM_STOP;
		}

		static BOOL CALLBACK DIEnumObjectsCallback( const DIDEVICEOBJECTINSTANCE* pdidoi, VOID* pContext )
		{
			JoystickWin32* pThis = reinterpret_cast<JoystickWin32*>(pContext);

			if( pdidoi->dwType & DIDFT_AXIS )
			{
				DIPROPRANGE diprg; 
				diprg.diph.dwSize       = sizeof(DIPROPRANGE); 
				diprg.diph.dwHeaderSize = sizeof(DIPROPHEADER); 
				diprg.diph.dwHow        = DIPH_BYID; 
				diprg.diph.dwObj        = pdidoi->dwType; // Specify the enumerated axis
				diprg.lMin              = -AxisScale; 
				diprg.lMax              = +AxisScale; 
	  
				// Set the range for the axis
				if( FAILED( pThis->mpDevice->SetProperty( DIPROP_RANGE, &diprg.diph ) ) ) 
					return DIENUM_STOP;
	    }

			return DIENUM_CONTINUE;
		}

		static Vec2 applyDeadZone(Vec2 v, float deadZone)
		{
			if (fabs(v[0]) < deadZone) 
				v[0] = 0;
			else
				v[0] = Math::Sign(v[0]) * ((fabs(v[0]) - deadZone) / (1.0f - deadZone));

			if (fabs(v[1]) < deadZone) 
				v[1] = 0;
			else
				v[1] = Math::Sign(v[1]) * ((fabs(v[1]) - deadZone) / (1.0f - deadZone));

			return v;
		}

		void updateAutoRepeat(DWORD deltaTicks, DWORD curTickCount)
		{
			mCanAutoRepeat = false;
			if (mAutoRepeatTimer <= deltaTicks)
			{
				mCanAutoRepeat = true;
				mAutoRepeatTimer = Math::Clamp<DWORD>(mAutoRepeatTimer - deltaTicks + AutoRepeatTime, 0, AutoRepeatTime);
			}
			else
				mAutoRepeatTimer -= deltaTicks;

			if (0xffffffff == mPrevDevState.rgdwPOV[0])
			{
				if (dPadDown() >= 0.0f)
					mDPadDownTickCount = curTickCount;
			}
		}

	public:
		JoystickWin32(void) :
			mpDI(NULL),
			mpDevice(NULL)
		{
			clearState();
		}

		~JoystickWin32(void)
		{
			deinit();
		}

		// true on failure
		bool init(HWND hWnd, HINSTANCE hInstance)
		{
			deinit();

			Verify(SUCCEEDED(DirectInput8Create(hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&mpDI, NULL)));

			if (FAILED(mpDI->EnumDevices(DI8DEVCLASS_GAMECTRL, DIEnumDevicesCallback, this, DIEDFL_ATTACHEDONLY)))
			{
				deinit();
				return true;
			}

			if (!mpDevice)
			{
				deinit();
				return true;
			}

			if (FAILED(mpDevice->SetDataFormat(&c_dfDIJoystick2)))
			{
				deinit();
				return true;
			}
    
			if (FAILED(mpDevice->SetCooperativeLevel(hWnd, DISCL_EXCLUSIVE | DISCL_FOREGROUND)))
			{
				deinit();
				return true;
			}

			if (FAILED(mpDevice->EnumObjects(DIEnumObjectsCallback, (VOID*)this, DIDFT_ALL)))
			{
				deinit();
				return true;
			}
      
			return false;
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

			clearState();
		}

		void clearState(void)
		{
			Utils::ClearObj(mDevState);
			mDevStateValid = false;

			Utils::ClearObj(mPrevDevState);
			mPrevDevStateValid = false;

			mDPadDownTickCount = 0;
			mPrevUpdateTickCount = GetTickCount();
			mAutoRepeatTimer = 0;
			mCanAutoRepeat = false;
		}

		// true on failure
		bool update(void)	
		{
			if (!mpDevice)
			{
				clearState();
				return true;
			}
			
			HRESULT hres = mpDevice->Poll();
			if (FAILED(hres)) 
			{
				hres = mpDevice->Acquire();
				
				if (FAILED(hres))
				{
					clearState();
					return true;
				}
        				
				hres = mpDevice->Poll();
				if (FAILED(hres))
				{
					clearState();
					return true;
				}
			}

			mPrevDevState = mDevState;
			mPrevDevStateValid = mDevStateValid;
			
			hres = mpDevice->GetDeviceState(sizeof(DIJOYSTATE2), &mDevState);
			if (FAILED(hres))
			{
				clearState();
				return true;
			}

			mDevStateValid = true;
			
			DWORD curTickCount = GetTickCount();
			DWORD deltaTicks = curTickCount - mPrevUpdateTickCount;
			mPrevUpdateTickCount = curTickCount;
		
			updateAutoRepeat(deltaTicks, curTickCount);
						      
			return false;
		}

		Vec2 leftThumb(float deadZone = 0.0f) const
		{
			if (!mDevStateValid)
				return Vec2(0);
			
			return applyDeadZone(Vec2(mDevState.lX, mDevState.lY) * (1.0f / float(AxisScale)), deadZone);
		}

		Vec2 rightThumb(float deadZone) const
		{
			if (!mDevStateValid)
				return Vec2(0);

			return applyDeadZone(Vec2(mDevState.lRx, mDevState.lRy) * (1.0f / float(AxisScale)), deadZone);
		}

		float trigger(int triggerIndex) const
		{
			if (!mDevStateValid)
				return 0.0f;

			DebugRange(triggerIndex, 2);
			
			if (triggerIndex == 0)
				return mDevState.lZ * (1.0f / float(AxisScale));
			else
				return mDevState.lRz * (1.0f / float(AxisScale));
		}

		// Returns negative value if dpad not active, otherwise degrees
		float dPadDown(void) const
		{
			if (!mDevStateValid)
				return -1.0f;

			if (0xffffffff == mDevState.rgdwPOV[0])
				return -1.0f;

			return mDevState.rgdwPOV[0] * (1.0f / 100.0f);
		}

		float dPadPressed(bool autoRepeat = true) const
		{
			if ((!mPrevDevStateValid) || (!mDevStateValid))
				return -1.0f;
      
			if (dPadDown() < 0)
				return -1.0f;

			DWORD since = GetTickCount() - mDPadDownTickCount;
			if ((since > AutoRepeatThresh) && (mCanAutoRepeat))
				return dPadDown();

			if (0xffffffff == mPrevDevState.rgdwPOV[0])
				return dPadDown();

			return -1.0f;
		}

		bool buttonDown(EGamePadButton buttonIndex) const
		{
			DebugRange(buttonIndex, eNumButtons);
			return (mDevState.rgbButtons[buttonIndex] & 0x80) != 0;
		}

		bool buttonPressed(EGamePadButton buttonIndex) const
		{
			DebugRange(buttonIndex, eNumButtons);

			if ((!mPrevDevStateValid) || (!mDevStateValid))
				return false;

			if ((mPrevDevState.rgbButtons[buttonIndex] & 0x80) == 0)
				return buttonDown(buttonIndex);

			return false;
		}
	};

	extern JoystickWin32 gJoystick;
  
} // namespace gr

#endif // JOYSTICK_WIN32_H 

