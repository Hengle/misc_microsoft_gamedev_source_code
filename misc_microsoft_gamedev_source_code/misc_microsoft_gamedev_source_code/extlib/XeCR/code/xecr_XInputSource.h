//	XInputSource.h : Input source encapsulation of the XTL input functions (i.e. the real controllers).
//
//	Created 2004/10/06 Rich Bonny    <rbonny@microsoft.com>
//
//  - based on original XCR code:
//	Created 2003/03/XX David Eichorn <deichorn@microsoft.com>
//     and modified by Jon Burns     <jburns@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2004 Microsoft Corp.  All rights reserved.




#pragma once

#include "xecr_Interfaces.h"




namespace XCR
{
	using namespace ControlUnit;

	// ================================================================
	//	InputSource
	//
	//	A gamepad data source that is the real input device ports on the XBOX.
	// ================================================================
	class XInputSource :
		public IPlayer
	{
	public:
		XInputSource(void);
		~XInputSource(void);

		// ----------------------------------------------------------------
		// IPlayer interface
		// ----------------------------------------------------------------
		
		// Initialize this component.
		virtual XCR::Result Initialize(IPlayerInterface *devices);

		// Notify this component it is being selected.
		virtual XCR::Result SelectComponent();

        virtual XCR::Result Start();
        
		virtual const char *GetHelpString()
		{
			return "Supplies input from the real Xbox controller ports.";
		}

		//JKBNEW:
		virtual XCR::Result SaveState(DirectDiskOutputStream* ddos);
		virtual XCR::Result ResumeState(DirectDiskInputStream* ddis);

		// Emulate XInputGetState.
		virtual XCR::Result InputGetState(DWORD &xresult, DWORD dwPort, PXINPUT_STATE pState);
		// Emulate XInputSetState.
		virtual XCR::Result InputSetState(DWORD &xresult, DWORD dwPort, PXINPUT_VIBRATION pVibration);
		// Emulate InputGetKeystroke
		// a-dannat Added dwFlags parameter to propogate the XINPUT_FLAG_GAMEPAD and 
		// XINPUT_FLAG_KEYBOARD flags if they are specified instead of XINPUT_FLAG_ANYDEVICE
		virtual XCR::Result InputGetKeystroke(DWORD &xresult, DWORD dwPort, DWORD dwFlags, PXINPUT_KEYSTROKE pKeystroke);

	protected:

		// ----------------------------------------------------------------
		// Implementation
		// ----------------------------------------------------------------

		void detect_controller(DWORD port);

		// Emulated input device pointer.
		IPlayerInterface *m_devices;

		//JKB: retrieve/set real device type from the port:
		//DWORD get_type_for_port(DWORD port);
		//void set_type_for_port(DWORD port, DWORD type);
	};

}