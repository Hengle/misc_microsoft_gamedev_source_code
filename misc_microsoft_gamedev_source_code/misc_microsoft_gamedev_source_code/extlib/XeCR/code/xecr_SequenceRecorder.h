//	SequenceRecorder.h : Records an event sequence.
//
//	Created 2004/10/06 Rich Bonny    <rbonny@microsoft.com>
//
//  - based on original XCR code:
//	Created 2003/09/29 David Eichorn <deichorn@microsoft.com>
//     and modified by Jon Burns     <jburns@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2004 Microsoft Corp.  All rights reserved.




#pragma once

#include "xecr_Interfaces.h"
#include <string>

#include "xecr_EventOutputSequence.h"




namespace XCR
{
	namespace Sequence
	{
		using namespace ControlUnit;

		// ================================================================
		//	Recorder
		//
		//	Base class that records an EventOutput sequence.
		// ================================================================
		class Recorder :
			public IRecorder
		{
		public:
			Recorder(IEventOutput *event_sequence);
			virtual ~Recorder();

			// ----------------------------------------------------------------
			// IComponent interface
			// ----------------------------------------------------------------

			virtual XCR::Result SelectComponent();
			virtual XCR::Result SetStringProperty(LPCSTR name, LPCSTR value1, LPCSTR value2 = NULL);
			virtual XCR::Result Start();
			virtual XCR::Result Stop();

			//JKBNEW:
			virtual XCR::Result SaveState(DirectDiskOutputStream* ddos);
			virtual XCR::Result ResumeState(DirectDiskInputStream* ddis);

			// ----------------------------------------------------------------
			// IRecorder interface
			// ----------------------------------------------------------------

			virtual XCR::Result Initialize(IRecorderInterface *core_interface);
			//virtual XCR::Result RecordGamepadDisconnect(DWORD port);
            virtual XCR::Result RecordDeviceDisconnect(DWORD port);
            virtual XCR::Result RecordToggleHud(DWORD port); // MJMXAM: add record toggle hud feature
			virtual XCR::Result RecordGamepadConnect(DWORD port, const XINPUT_CAPABILITIES &capabilities, const XINPUT_GAMEPAD &state);
			virtual XCR::Result RecordInputGetState(DWORD port, const XINPUT_GAMEPAD &gamepad_state);
			//virtual XCR::Result RecordKeyboardDisconnect(DWORD port);
			virtual XCR::Result RecordKeyboardConnect(DWORD port, const XINPUT_KEYSTROKE &keyboard_state);
			virtual XCR::Result RecordInputGetKeystroke(DWORD port, const PXINPUT_KEYSTROKE pKeystroke);
			virtual XCR::Result FrameAdvance();
			virtual XCR::Result Recorder::GetStatus();
			virtual const char *GetHelpString()
			{
				return "XeCR Sequence Recorder.  Records input data to a sequence.";
			}

		protected:

			// ----------------------------------------------------------------
			// Implementation
			// ----------------------------------------------------------------

			IRecorderInterface *core;
			IEventOutput *event_sequence;
			bool is_recording;

			struct GamepadData
			{
				bool is_connected;
				XINPUT_GAMEPAD gamepad_state;
				XINPUT_CAPABILITIES capabilities;
			};
			GamepadData gamepad_data[INPUT_DEVICE_PORTS];

			// Keyboard data:
			struct KeyboardData
			{
				bool is_connected;
				XINPUT_KEYSTROKE keyboard_state;
			};
			KeyboardData keyboard_data[INPUT_DEVICE_PORTS];

			DWORD absolute_start_time_ms;
			DWORD relative_current_time_ms;
			DWORD relative_current_frame;

			//JKBNEW: adjuster on resume state after reboot:
			DWORD m_tickCountAdjuster;

			XCR::Result update_all_ports();
			XCR::Result update_time();
		};

	}
}
