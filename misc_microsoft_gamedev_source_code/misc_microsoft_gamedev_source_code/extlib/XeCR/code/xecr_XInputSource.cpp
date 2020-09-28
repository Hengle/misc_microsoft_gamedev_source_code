//	XInputSource.cpp : Input source encapsulation of the XTL input functions (i.e. the real controllers).
//
//	Created 2004/10/12 Rich Bonny    <rbonny@microsoft.com>
//
//  - based on original XCR code:
//	Created 2003/03/XX David Eichorn <deichorn@microsoft.com>
//     and modified by Jon Burns     <jburns@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2004 Microsoft Corp.  All rights reserved.




#ifdef _XCR_

#include "xecr_XInputSource.h"
#include "xecr_core.h"
#ifdef XAM_HOOK		
#include <xamp.h>
#else
#include <xbdm.h>
#endif


namespace XCR
{
	extern XCR::ControlUnit::Core xcr;

	// ----------------------------------------------------------------
	// Constructors / destructor
	// ----------------------------------------------------------------


	XInputSource::XInputSource(void)
	{
		for (int port = 0; port < INPUT_DEVICE_PORTS; port++)
		{
			//m_device_type[port] = NULL;
			
		}
	}


	XInputSource::~XInputSource(void)
	{
	}




	// ----------------------------------------------------------------
	// IPlayer interface
	// ----------------------------------------------------------------


	// Initialize this component.
	//	Gets the initial device connection state and type.
	//	We need to connect here to initialize capabilities.
	//	This should only be called once.
	XCR::Result XInputSource::Initialize(IPlayerInterface *devices)
	{
		// Store a pointer to the array of emulated port interfaces.
		m_devices = devices;

		// Go through each port and see what is connected.
		// All the connection and deconnection logic is handled elsewhere
		for (DWORD port = 0; port < INPUT_DEVICE_PORTS; port++)
		{
			detect_controller(port);
		}
		return XCR::Result::SUCCESS;
	}

	// Notify this component it is being selected.
	//	This checks the currently connected set of devices against what it has open.
	//	If it finds a descrepancy it will disconnect and connect the device as needed to fix it up.
	XCR::Result XInputSource::SelectComponent()
	{
		// Throw out any currently connected devices and tell it to connect ours.
		for (DWORD port = 0; port < INPUT_DEVICE_PORTS; port++)
		{
            //MJMXAM
#ifdef XAM_HOOK
			XAutomationpUnbindController(port);
#else
			DmAutomationUnbindController(port);
#endif

			XINPUT_CAPABILITIES capabilities;
			memset(&capabilities, 0, sizeof (capabilities));
			DWORD portType = m_devices->GetDeviceType(port);

			if (NULL != portType)
			{
				// Check if connected
				if (m_devices->IsConnected(port))
				{
					// Is the emulated device connected and do the capabilities match?
					// This call should now work for both gamepads and keyboards
					XInputGetCapabilities(port, 0, &capabilities);

					// If capabilities differ, disconnect device and connect ours.
					// RWBFIX - Is this necessary? Also need to check for device type
					// when keyboard support is functional.
					if (m_devices->GetCapabilities(port) != capabilities)
					{
						m_devices->Disconnect(port);
						m_devices->Connect(port, capabilities);
					}
				}
				else
				{
					// Not connected.  Connect us.
					m_devices->Connect(port, capabilities);
				}
			}
		}
		return XCR::Result::SUCCESS;
	}

    XCR::Result XInputSource::Start()
    {
        return SelectComponent();
    }

	//JKBNEW:
	XCR::Result XInputSource::SaveState(DirectDiskOutputStream* ddos)
	{
		//???
		// ddos->Write((LPVOID)this, (DWORD)sizeof(this));

		// Write real device port data:
		for (int i = 0; i < INPUT_DEVICE_PORTS; i++)
		{
			// Disable for now and get correct device type
			//if (!ddos->Write((LPCVOID)&m_device_type[i],(DWORD)sizeof(DWORD)))
			//	return XCR::Result::ERROR;
		}

		return XCR::Result::SUCCESS;
	}

	//JKBNEW:
	XCR::Result XInputSource::ResumeState(DirectDiskInputStream* ddis)
	{
		// Read real device port data:
		for (int i = 0; i < INPUT_DEVICE_PORTS; i++)
		{
			// Disable for now and get correct device type
			//if (!ddis->Read((LPVOID)&m_device_type[i],(DWORD)sizeof(DWORD)))
			//	return XCR::Result::ERROR;
		}

		return XCR::Result::SUCCESS;
	}



	// Emulate XInputGetState and detect insertions or removals.
	XCR::Result XInputSource::InputGetState(DWORD &xresult, DWORD dwPort, PXINPUT_STATE pState)
	{
		bool bWasConnected = m_devices->IsConnected(dwPort);

        //MJMXAM - we need to check our input state all the way back at the XAM layer.
        XCR::Result hud_status = XCR::xcr.GetHudStatus();

#ifdef XAM_HOOK		
        DWORD dwInputType = 0; // Default to getting input for title.
		if (hud_status == XCR::Result::STATUS_ACTIVE)
        {
            // If the HUD is active use get input for system instead.
            dwInputType = XINPUT_FLAG_SYSTEMAPP;
        }
        xresult = XamInputGetState(dwPort, dwInputType, pState);
#else
		xresult = XInputGetState(dwPort, pState);	
#endif
		// Detect removal and insertions.
		// RWBFIX: What do we want to do if keyboard has been connected?
		if (!bWasConnected && (xresult == ERROR_SUCCESS))
		{
			// Check port type
			XINPUT_CAPABILITIES tempCaps;
			xresult = XInputGetCapabilities(dwPort, XINPUT_FLAG_GAMEPAD, &tempCaps);

			// The previous call should almost never fail, but there is a remote
			// possibility it could happen.
			if (xresult == ERROR_SUCCESS)
			{
				m_devices->SetDeviceType(dwPort, XINPUT_DEVTYPE_GAMEPAD);
				m_devices->Connect(dwPort, tempCaps);
			}
		}
		// But then this could be a Keyboard. Try to use detect_controller: RWBFIX
		else if (bWasConnected && (xresult != ERROR_SUCCESS))
		{
			m_devices->Disconnect(dwPort);
		}
		//m_devices->SetGamepadState(dwPort, pState->Gamepad);
		return XCR::Result::SUCCESS;
	}


	// Emulate XInputSetState.
	XCR::Result XInputSource::InputSetState(DWORD &xresult, DWORD dwPort, PXINPUT_VIBRATION pVibration)
	{
		xresult = XInputSetState(dwPort, pVibration);
		return XCR::Result::SUCCESS;
	}


	//Emulation of keyboard:
	// a-dannat Added dwFlags parameter to propogate the XINPUT_FLAG_GAMEPAD and 
	// XINPUT_FLAG_KEYBOARD flags if they are specified instead of XINPUT_FLAG_ANYDEVICE
	XCR::Result XInputSource::InputGetKeystroke(DWORD &xresult, DWORD dwPort, DWORD dwFlags, PXINPUT_KEYSTROKE pKeystroke)
	{
        //MJMXAM - we need to check our input state all the way back at the XAM layer.
		// Why was this GetHudStatus(dwPort)? This checks to see if ToggleHud was called and m_hud_user was set.  
		//In the case of InputSource, it is not called. Which resulted in the hud code never being exectued 
		// even when the hud was up.
        XCR::Result hud_status = XCR::xcr.GetHudStatus();
#ifdef XAM_HOOK		
        DWORD dwInputType = 0; // Default to getting input for title.
		if (hud_status == XCR::Result::STATUS_ACTIVE)
        {
            // If the HUD is active use get input for system instead.
            dwInputType = XINPUT_FLAG_SYSTEMAPP;

			// This call returns an empty pKeystroke when used in a game XUI with dwInputType = 0.
			//  as well as in the system XUI with dwInputType = XINPUT_FLAG_SYSTEMAPP
			// XamInputGetKeystroke doesn't work, try using Gamestate since it does work.
			//XINPUT_STATE GameState;
			//xresult = XamInputGetState(dwPort, dwInputType, &GameState);
	        xresult = XamInputGetKeystroke(dwPort, dwInputType, pKeystroke);
        }
		else
#endif
			xresult = XInputGetKeystroke(dwPort, dwFlags, pKeystroke);

		return XCR::Result::SUCCESS;
	}

	// ----------------------------------------------------------------
	// Implementation
	// ----------------------------------------------------------------

	// Check the port to see what kind of controller (if any) is attached.
	// Also take care of detecting insertions and removals.
	void XInputSource::detect_controller(DWORD port)
	{
		// Note: only do something if things have changed

		XINPUT_CAPABILITIES capabilities;
		DWORD previousType = m_devices->GetDeviceType(port);
		DWORD currentType = NULL;

		// See if there is a device attached to this port
		DWORD resultCode = XInputGetCapabilities(port, 0, &capabilities);
		
		if (ERROR_SUCCESS == resultCode)
		{
			currentType = capabilities.Type;
		}

		// We only need to do something if things have changed.
		if (currentType != previousType)
		{
			m_devices->SetDeviceType(port, currentType);
			if (NULL == currentType)
			{
				m_devices->Disconnect(port);
			}
			else
			{
				m_devices->Connect(port, capabilities);
			}
		}
	}

	//JKB: retrieve real device type from the port:
	//inline DWORD XInputSource::get_type_for_port(DWORD port)
	//{
	//	if (port >= INPUT_DEVICE_PORTS)
	//	{
	//		return NULL;
	//	}
	//	return m_device_type[port];
	//}

	//JKB: set real device type from the port:
	//inline void XInputSource::set_type_for_port(DWORD port, DWORD type)
	//{
	//	if (port >= INPUT_DEVICE_PORTS)
	//	{
	//		// Huh?  What port is that again?
	//		return;
	//	}
	//	m_device_type[port] = type;
	//}

}


#endif