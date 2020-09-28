//	EventOutputSequence.h : Event output sequence.
//
//	Created 2004/10/06 Rich Bonny    <rbonny@microsoft.com>
//
//  - based on original XCR code:
//	Created 2003/08/XX David Eichorn <deichorn@microsoft.com>
//     and modified by Jon Burns     <jburns@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2004 Microsoft Corp.  All rights reserved.




#pragma once

#include "XenonUtility.h"
#include "xutil_OutputStream.h"
using namespace XenonUtility;

#include "xecr_FileFormat.h"




namespace XCR
{
	namespace Sequence
	{

		class IEventOutput
		{
		public:
			//JKB: debug keyboard considerations / device type mods:
			virtual XCR::Result Start() = 0;
			virtual XCR::Result Stop() = 0;
			virtual XCR::Result SetTime(DWORD time_ms, DWORD frame) = 0;
			virtual XCR::Result WriteConnectGamepadEvent(DWORD port, const XINPUT_CAPABILITIES &capabilities, const XINPUT_GAMEPAD &state) = 0;
			//virtual XCR::Result WriteDisconnectGamepadEvent(DWORD port) = 0;
			virtual XCR::Result WriteDisconnectAnyDeviceEvent(DWORD port) = 0;
			virtual XCR::Result WriteGamepadState(DWORD port, const XINPUT_GAMEPAD &gamepad_state) = 0;
			virtual XCR::Result WriteKeyboardState(DWORD port, const XINPUT_KEYSTROKE &keyboard_state) = 0;
			virtual XCR::Result WriteConnectKeyboardEvent(DWORD port, const XINPUT_KEYSTROKE &keyboard_state) = 0;
            virtual XCR::Result WriteToggleHudEvent(DWORD port) = 0; // MJMXAM: added to support toggle hud recording
			//virtual XCR::Result WriteDisconnectKeyboardEvent(DWORD port) = 0;
			virtual std::string GetErrorMessage() const = 0;

			//JKBNEW:
			virtual XCR::Result SaveState(DirectDiskOutputStream* ddos) = 0;
			virtual XCR::Result ResumeState(DirectDiskInputStream* ddis) = 0;
		};




		class BinaryEventOutput :
			public IEventOutput,
			public IFileFormat
		{
		public:
			BinaryEventOutput(OutputStream *output);

			//JKB: debug keyboard considerations / device type mods:
			virtual XCR::Result Start();
			virtual XCR::Result Stop();
			virtual XCR::Result SetTime(DWORD time_ms, DWORD frame);
			virtual XCR::Result WriteConnectGamepadEvent(DWORD port, const XINPUT_CAPABILITIES &capabilities, const XINPUT_GAMEPAD &state);
			//virtual XCR::Result WriteDisconnectGamepadEvent(DWORD port);
			virtual XCR::Result WriteDisconnectAnyDeviceEvent(DWORD port);
			virtual XCR::Result WriteGamepadState(DWORD port, const XINPUT_GAMEPAD &gamepad_state);
			virtual XCR::Result WriteKeyboardState(DWORD port, const XINPUT_KEYSTROKE &keyboard_state);
			virtual XCR::Result WriteConnectKeyboardEvent(DWORD port, const XINPUT_KEYSTROKE &keyboard_state);
			//virtual XCR::Result WriteDisconnectKeyboardEvent(DWORD port);
            virtual XCR::Result WriteToggleHudEvent(DWORD port); // MJMXAM: added to support toggle hud recording

			virtual std::string GetErrorMessage() const
			{
				return "No extended error information.";
			}

			//JKBNEW:
			virtual XCR::Result SaveState(DirectDiskOutputStream* ddos);
			virtual XCR::Result ResumeState(DirectDiskInputStream* ddis);

		protected:
			OutputStream *output_stream;
			XINPUT_GAMEPAD previous_gamepad_state[INPUT_DEVICE_PORTS];

			DWORD current_event_time_ms;
			DWORD current_event_frame;
			DWORD previous_event_time_ms;
			DWORD previous_event_frame;

			//JKB: device considerations:
			XCR::Result init_device_event_entry(DeviceInputEvent &event, DWORD port);
			XCR::Result init_event_entry(InputEvent &event);
		};

	}
}