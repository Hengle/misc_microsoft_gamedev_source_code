//	RandomInputSource.h : Random input source.
//
//	Created 2004/10/06 Rich Bonny    <rbonny@microsoft.com>
//
//  - based on original XCR code:
//	Created 2003/03/XX David Eichorn <deichorn@microsoft.com>
//     and modified by Jon Burns     <jburns@microsoft.com>
//	Modified 2004/09/08 jkburns : JKBNEW: state serialization support additions
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2004 Microsoft Corp.  All rights reserved.




#pragma once

#include "xecr_Interfaces.h"
#include "xecr_GamepadControls.h"




namespace XCR
{
	using namespace ControlUnit;

	// ================================================================
	//	RandomInputSource
	//
	//	A gamepad data source that just plays random input.
	// ================================================================
	class RandomInputSource :
		public IPlayer,
		public GamepadControls
	{
	public:
		RandomInputSource(void);
		~RandomInputSource(void);

		// ----------------------------------------------------------------
		// IComponent interface
		// ----------------------------------------------------------------

		// Connect component to the XCR Control Unit.
		//virtual bool Connect(IControllerInterface *pxcr) = 0;

		// Set a property for this component.
		//	parameters:
		//		name : name of property to set
		//		value : value to set property to
		//	returns result code
		virtual XCR::Result SetStringProperty(LPCSTR name, LPCSTR value1, LPCSTR value2 = NULL);

		// Start this component.
		//	returns result code
		virtual XCR::Result Start();

		// Stop this component.
		//	returns result code
		virtual XCR::Result Stop();

		// Retrieve status.
		virtual XCR::Result GetStatus();

		// Select this component.
		virtual bool Connect(IControllerInterface *pxcr);

		// Retrieve helpstring.
		virtual const char *GetHelpString()
		{
			return "Generates per-controller random input.\v"
				"Properties:\v  port 1-4|all\v  connect 0-1000|always|never\v"
				"  seed 0-4294967295\v  interval 0-10000\v"
				"  freq <control> 0-1000\v  reset <control> 0-1000\v  default <control> 0-32768\v"
				"<control>: A B X Y LS RS LT RT S Z DU DD DL DR J C JX CX JY CY ALL";
		}

		//JKBNEW:
		virtual XCR::Result SaveState(DirectDiskOutputStream* ddos);
		virtual XCR::Result ResumeState(DirectDiskInputStream* ddis);

		// ----------------------------------------------------------------
		// IPlayer interface
		// ----------------------------------------------------------------

		// Initialize this component.
		virtual XCR::Result Initialize(IPlayerInterface *pXCR);

		// Notify this component it is being selected.
		virtual XCR::Result SelectComponent();

		// Emulate XInputGetState, but uses port instead of handle.
		virtual XCR::Result InputGetState(DWORD &xresult, DWORD dwPort, PXINPUT_STATE pState);

		// Emulate XInputGetKeystroke
		virtual XCR::Result InputGetKeystroke(DWORD &xresult, DWORD dwPort, DWORD dwFlags, PXINPUT_KEYSTROKE pKeystroke);


	protected:

		// ----------------------------------------------------------------
		// Implementation
		// ----------------------------------------------------------------

		void update_connections();
        void update_port(int port, bool connect);
        void initialize_controls_to_defaults(int port, XINPUT_GAMEPAD &state);
		// a-dannat helper function to extrapolate a thumbstick virtual key from a gamepad state 
		WORD GetVirtualThumbstickKey( XINPUT_GAMEPAD Gamepad, WORD Axis );
		
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		// Emulated input devices
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

		// Interface to ports used to connect and disconnect emulated devices and query their state.
		IPlayerInterface *m_ports;

		
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		// Random data and property interpretation
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

		// Structure holding random parameters.
		struct PortRandomParameters
		{
			// Can this device be connected and disconnected?
			enum Connected { RANDOM, NEVER, ALWAYS } connected;
			// How often should we disconnect / reconnect this port?
			DWORD connect_frequency;
			// What is the minimum time (in milliseconds) between changes?
			DWORD interval;
			// When was the last time the control considered changing?
			DWORD last_change;
			// Describes the random action of each control.
			struct ControlRandomParameters
			{
				// How often should we mess with the control?
				DWORD change_frequency;
				// How often of the times we mess with it should it be reset to default?
				DWORD reset_frequency;
				// What the default value should be.
				SHORT default_value;
			};
			// Random action of each control.
			ControlRandomParameters controls[CONTROL_COUNT];
		};

		// Random parameters for all the devices.
		PortRandomParameters m_port_random_parameters[INPUT_DEVICE_PORTS];

		// Current ports we are working with.
		DWORD m_selected_ports;

		// Current play status
		bool m_is_playing;

		// Random seed to initialize with
		unsigned int m_random_seed;

	};

}