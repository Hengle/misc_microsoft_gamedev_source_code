//	TextSequencePlayer.h : A .h file defining TextSequencePlayer
//		in the Gamepad project.
//
//	Created 2004/10/06 Rich Bonny    <rbonny@microsoft.com>
//
//  - based on original XCR code:
//	Created 2003/07/XX David Eichorn <deichorn@microsoft.com>
//     and modified by Jon Burns     <jburns@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2004 Microsoft Corp.  All rights reserved.





#pragma once

#include "XenonUtility.h"
using namespace XenonUtility;

#include "xecr_SequencePlayer.h"
#include "xecr_GamepadControls.h"




namespace XCR
{
	namespace Sequence
	{

		// ================================================================
		//	TextEventInput sequence
		//
		//	Interprets events from a text InputStream.
		// ================================================================
		class TextEventInput :
			public IEventInput,
			protected GamepadControls
		{
		public:
			TextEventInput();
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
				return m_port;
			}
            inline virtual DWORD GetCombinedPort() const
            {
                return m_combined_port;
            }
			inline virtual const XINPUT_CAPABILITIES &GetCapabilities() const
			{
				return m_capabilities;
			}
			virtual void GetGamepadState(XINPUT_GAMEPAD &gamepad) const;
			virtual void GetKeyboardState(XINPUT_KEYSTROKE &keystroke) const;
			virtual std::string GetErrorMessage() const;

			// Set the data to play for this sequence.
			// returns true if data is valid, valse otherwise.
			virtual void SetData(const char *data);

			// Set the frame rate for time/frame conversions.
			virtual void SetFps(DWORD fps)
			{
				m_fps = fps;
			}

			//JKBNEW:
			virtual XCR::Result SaveState(DirectDiskOutputStream* ddos);
			virtual XCR::Result ResumeState(DirectDiskInputStream* ddis);

		protected:

			bool get_next_char();
            bool put_char_back();
			bool parse_event();
			bool parse_control_action_group();
			bool parse_control_action();
			bool parse_duration(unsigned long &frames, unsigned long &ms);
			bool parse_ulong(unsigned long &number);

			// Port initialization
			bool m_port_opened[INPUT_DEVICE_PORTS];
			bool m_retain_previous_port;
            bool must_open_port();
            bool must_open_port(DWORD port);

            std::string m_data;
            std::string::size_type m_data_index;
			char m_next_char;

			// Frames per second.
			DWORD m_fps;
			// Next event type.
			EventType m_type;
			// Timing of next event.
			DWORD m_time;
			// Frame number for next event.
			DWORD m_frame;
			// Port for next event.
			DWORD m_port;
            // Combined port for next event.
            DWORD m_combined_port;
			// Capabilities information for connect.
			XINPUT_CAPABILITIES m_capabilities;

			// Data about control events that will happen later.
			// This handles events such as turning a control off after some time period.
			struct ControlEventData
			{
				// Activation amount for this event.
				// a-dannat Changing from a DWORD to a SHORT to preserve the sign that is lost when converting from a DWORD to a SHORT
				SHORT Amount;
			};

			// Which controls are involved in this event.
			ControlEventData m_sequence_event_controls[CONTROL_COUNT];
			// Duration of event in frames.
			int m_sequence_event_frames;
			// Duration of event in ms.
			int m_sequence_event_ms;
			// Types of events in sequences.
			enum SequenceEventType { PRESS, HOLD, RELEASE, OTHER };
			// Type of event.
			SequenceEventType m_sequence_event_type;

			std::string error_message;
			inline void add_error_message(const char *message)
			{
				if (error_message.length() == 0) error_message = message + error_message;
				else error_message = message + ("  " + error_message);
			}
			inline void reset_error_message()
			{
				error_message = "";
			}
		};




		// ================================================================
		// TextSequencePlayer
		//
		//	Supplies a sequence of input events.
		// ================================================================
		class TextSequencePlayer :
			public Player
		{
		public:
			TextSequencePlayer(void);
			~TextSequencePlayer(void);

			// ----------------------------------------------------------------
			// IComponent interface
			// ----------------------------------------------------------------

			// Set a property for this component.
			XCR::Result SetStringProperty(LPCSTR name, LPCSTR value1, LPCSTR value2);

            XCR::Result SetDefault();

			virtual const char *GetHelpString()
			{
				return "Plays sequences of text commands.\vProperties:\v  data - text sequence to play\v  fps - frame rate to use in time/frame conversions\vSequence commands: T(wait),I(insert),O(remove)\vButtons: A,B,X,Y,LS,RS,LT,RT,S,Z\vDirectionals: D(pad)|J(left)|C(right)U|D|L|R\vTime: X/1s|f|ms\vHold: _X\vRelease: ^X";
			}

		protected:

			// Event sequence.
			TextEventInput m_text_event_sequence;
		};

	}
}