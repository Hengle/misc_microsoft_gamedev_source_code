//	EventInputSequence.h : Event input sequence.
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
#include "xutil_InputStream.h"
using namespace XenonUtility;

#include "xecr_Interfaces.h"
#include "xecr_FileFormat.h"
#include "xecr_GamepadControls.h"




namespace XCR
{
	namespace Sequence
	{

		// ================================================================
		// IEventInput sequence
		//
		//	Supplies a sequence of input events.
		// ================================================================
		class IEventInput
		{
		public:
			virtual XCR::Result Start() = 0;
			virtual XCR::Result Stop() = 0;
			virtual XCR::Result AdvanceToNextEvent() = 0;
			enum EventType
			{
				//JKB: device considerations:
				EVENT_END,
				EVENT_CONNECT_GAMEPAD,
				EVENT_DISCONNECT_ANY_DEVICE,
				EVENT_GAMEPAD_STATE,
				EVENT_CONNECT_KEYBOARD,
				EVENT_KEYBOARD_STATE,
				EVENT_KEYSTROKE, // a-dannat Added to support Text Sequencing changing both the gamepad state and adding a keystroke at the same time.  This could replace both gamepad and keyboard state events
                EVENT_TOGGLE_HUD // MJMXAM: added to support toggle hud recording
			};
			virtual EventType GetEventType() const = 0;
			virtual DWORD GetTime() const = 0;
			virtual DWORD GetFrame() const = 0;
			virtual DWORD GetPort() const = 0;
            virtual DWORD GetCombinedPort() const
            {
                return 0;
            }
			virtual const XINPUT_CAPABILITIES &GetCapabilities() const = 0;
			virtual void GetGamepadState(XINPUT_GAMEPAD &gamepad) const = 0;
			virtual void GetKeyboardState(XINPUT_KEYSTROKE &keystroke) const = 0;
			virtual std::string GetErrorMessage() const = 0;

			virtual XCR::Result SaveState(DirectDiskOutputStream* ddos) = 0;
			virtual XCR::Result ResumeState(DirectDiskInputStream* ddis) = 0;

		};



		// ================================================================
		//	BinaryEventInput sequence
		//
		//	Interprets events from a binary InputStream.
		// ================================================================
		class BinaryEventInput :
			public IEventInput,
			public IFileFormat
		{
		public:
			BinaryEventInput(InputStream *input);

			virtual XCR::Result Start();
			virtual XCR::Result Stop();
			virtual XCR::Result AdvanceToNextEvent();
			inline virtual EventType GetEventType() const
			{
				return m_type;
			}
			inline virtual DWORD GetTime() const
			{
				return m_time;
			}
			inline virtual DWORD GetFrame() const
			{
				return m_frame;
			}
			inline virtual DWORD GetPort() const
			{
				_ASSERTE
				(
					//JKB: device considerations: 
					m_type == EVENT_CONNECT_GAMEPAD
					|| m_type == EVENT_CONNECT_KEYBOARD
					|| m_type == EVENT_DISCONNECT_ANY_DEVICE
					|| m_type == EVENT_GAMEPAD_STATE
					|| m_type == EVENT_KEYBOARD_STATE
					|| m_type == EVENT_KEYSTROKE  // a-dannat Added to support XInputGetKeystroke during text sequence playback
                    || m_type == EVENT_TOGGLE_HUD // MJMXAM: added to support toggle hud recording
				);
				return m_port;
			}
			inline virtual const XINPUT_CAPABILITIES &GetCapabilities() const
			{
				//JKB
				_ASSERTE(m_type == EVENT_CONNECT_GAMEPAD);
				return m_capabilities;
			}
			virtual void GetGamepadState(XINPUT_GAMEPAD &gamepad) const;
			virtual void GetKeyboardState(XINPUT_KEYSTROKE &keystroke) const;
			
			virtual std::string GetErrorMessage() const
			{
				return "No extended error information.";
			}

			//JKBNEW:
			virtual XCR::Result SaveState(DirectDiskOutputStream* ddos);
			virtual XCR::Result ResumeState(DirectDiskInputStream* ddis);

		protected:

			// Our input stream.
			InputStream *m_input;
			// Next event type.
			EventType m_type;
			// Timing of next event.
			DWORD m_time;
			// Frame number for next event.
			DWORD m_frame;
			// Port for next event.
			DWORD m_port;
			// Capabilities information for connect.
			XINPUT_CAPABILITIES m_capabilities;
			// State information for gamepad.
			char m_gamepad_data[sizeof (XINPUT_GAMEPAD)];
			// Flags indicating which information to use.
			DWORD m_gamepad_data_flags;

			// State info for keyboard:
			char m_keyboard_data[sizeof (XINPUT_KEYSTROKE)];
			//JKB: Flags indicating which informatin to use
			DWORD m_keyboard_data_flags;

			// Retrieve next event from disk.
			XCR::Result get_next_event();
		};

	}
}