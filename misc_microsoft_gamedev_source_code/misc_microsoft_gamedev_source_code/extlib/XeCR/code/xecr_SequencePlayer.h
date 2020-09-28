//	SequencePlayer.h : Plays an event sequence.
//
//	Created 2004/10/06 Rich Bonny    <rbonny@microsoft.com>
//
//  - based on original XCR code:
//	Created 2003/06/04 David Eichorn <deichorn@microsoft.com>
//     and modified by Jon Burns     <jburns@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2004 Microsoft Corp.  All rights reserved.




#pragma once

#include "xecr_Interfaces.h"
#include <string>

#include "xecr_EventInputSequence.h"
#include "xutil_InputStream.h"




namespace XCR
{
	namespace Sequence
	{
		using namespace ControlUnit;

		// ================================================================
		//	Player
		//
		//	Base class that plays an EventInput sequence.
		// ================================================================
		class Player :
			public IPlayer
		{
		public:
			Player(IEventInput *event_sequence);
			virtual ~Player();

			// ----------------------------------------------------------------
			// IComponent interface
			// ----------------------------------------------------------------

			virtual XCR::Result SelectComponent();
            virtual XCR::Result SetStringProperty(LPCSTR name, LPCSTR value1, LPCSTR value2 = NULL);
            virtual XCR::Result SetDefault();
			virtual XCR::Result Start();
			virtual XCR::Result Stop();

			virtual XCR::Result SaveState(DirectDiskOutputStream* ddos);
			virtual XCR::Result ResumeState(DirectDiskInputStream* ddis);

			// ----------------------------------------------------------------
			// IPlayer interface
			// ----------------------------------------------------------------

			virtual XCR::Result Initialize(IPlayerInterface *core_interface);
			virtual XCR::Result InputGetState(DWORD &xresult, DWORD dwPort, PXINPUT_STATE pState);

			// a-dannat Added dwFlags parameter to propogate the XINPUT_FLAG_GAMEPAD and 
			// XINPUT_FLAG_KEYBOARD flags if they are specified instead of XINPUT_FLAG_ANYDEVICE
			virtual XCR::Result InputGetKeystroke(DWORD &xresult, DWORD dwPort, DWORD dwFlags, PXINPUT_KEYSTROKE pKeystroke);
			virtual XCR::Result GetStatus();

		protected:

			// ----------------------------------------------------------------
			// Implementation
			// ----------------------------------------------------------------

			IPlayerInterface *core;
			IEventInput *event_sequence;
			bool is_playing;
			bool time_based_playback;

			struct GamepadData
			{
				bool is_connected;
				XINPUT_STATE gamepad_state;
				XINPUT_CAPABILITIES capabilities;
			};
			GamepadData gamepad_data[INPUT_DEVICE_PORTS];

			struct KeyboardData
			{
				bool is_connected;
				XINPUT_KEYSTROKE keystroke;
			};
			KeyboardData keyboard_data[INPUT_DEVICE_PORTS];

			DWORD absolute_start_time_ms;
			DWORD relative_current_time_ms;
			DWORD relative_current_frame;

			//JKBNEW: adjuster on resume state after reboot:
			DWORD m_tickCountAdjuster;

			bool time_for_next_event();
            XCR::Result handle_events();
            XCR::Result handle_events(DWORD port);
			void update_connected_devices();
			void do_work();
		};

	}
}
