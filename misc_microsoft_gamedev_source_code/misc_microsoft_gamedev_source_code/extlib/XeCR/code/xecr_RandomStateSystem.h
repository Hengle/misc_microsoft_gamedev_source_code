//	RandomStateSystem.h : State-Based Random Test Scenario description.
//		A collection of classes that describe the random state data
//		for the random state control of XCR.
//		This file is included by the standalone compiler and the XCR.
//
//	Created 2004/10/06 Rich Bonny    <rbonny@microsoft.com>
//
//  - based on original XCR code:
//	Created 2003/05/05 David Eichorn <deichorn@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2004 Microsoft Corp.  All rights reserved.




#pragma once




// ================================================================
//	RandomStateSystem
//

// ================================================================


namespace XCR
{
	namespace RandomStateSystem
	{

		typedef long state_index;
		typedef long choice_index;
		typedef long action_index;
		typedef unsigned long string_index;
		typedef long type_index;


		struct XRSFileHeader
		{
		public:
			void Init()
			{
				magic = '\0SRX';
				version = 0x00020000;
				header_length = sizeof (*this);
			}

			bool IsValid() const
			{
				return
					magic == '\0SRX'
					&& version == 0x00020000
					&& header_length == sizeof (*this);
			}

			DWORD magic;
			DWORD version;
			DWORD header_length;
			// Offsets of items in the file.
			DWORD StatesOffset;
			DWORD ChoicesOffset;
			DWORD ActionsOffset;
			DWORD StringsOffset;
			DWORD NotificationsOffset;
			DWORD NotificationsLength;
			// The initial state.
			state_index InitialState;
		};


		// A notification.
		// Maps a string code to a state to switch to.
		class Notification
		{
		public:
			Notification(string_index notify_string, state_index state) :
				NotifyString(notify_string),
				State(state)
			{
			}
			// Offset of notification string in the string table.
			string_index NotifyString;
			// State to switch to.
			state_index State;
		};


		// An action to perform.
		// Specific action depends on type and data.
		// May be, for example, the playback of a recorded gamepad file.
		class Action
		{
		public:
			Action(type_index type, string_index data_string) :
				Type(type),
				DataString(data_string)
			{
			}

			// Type of action this is.
			type_index Type;
			// Offset of data string in the string table.
			string_index DataString;
		};


		// A choice of something to do when in a state.
		// This may be a transition or a superstate.
		class Choice
		{
		public:
			Choice(float weight, action_index action, state_index state) :
				Weight(weight),
				Action(action),
				State(state)
			{
			}
			Choice(float weight, state_index target) :
				Weight(weight),
				Action(-1),
				Target(target)
			{
			}

			// Weight for this choice.  Higher means it is executed more often.
			float Weight;
			// Action associated with this choice.
			// -1 means it is a superstate.
			action_index Action;
			// State number that is the target or superstate.
			union
			{
				state_index State;
				state_index Target;
			};
		};


		// A state.
		// Contains a list of choices.
		class State
		{
		public:
			State(string_index name_string) :
				NameString(name_string),
				Choices(-1),
				ChoiceCount(0)
			{
			}
			// Name of this state.  Used in logging.
			string_index NameString;
			// Index of first choice in the choices table.  -1 if no choices.
			choice_index Choices;
			// Number of choices.
			unsigned long ChoiceCount;
		};

	}
}