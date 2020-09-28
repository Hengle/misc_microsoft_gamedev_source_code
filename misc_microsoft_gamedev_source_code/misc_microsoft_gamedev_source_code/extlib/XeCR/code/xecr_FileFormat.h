//	FileFormat.h : Recorded input file format data types.
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

#pragma pack(push, 1)




namespace XCR
{
	namespace Sequence
	{

		// ================================================================
		// IFileFormat interface
		//
		//	A collection of structures that define the file format.
		// ================================================================
		class IFileFormat
		{
		public:
			struct FileHeader
			{
				FileHeader(bool init = false)
				{
					if (init)
					{
						magic = '\0RCX';
						version = 0x00020000;
						header_length = sizeof (*this);
					}
				}

				bool IsValid() const
				{
					return
						magic == '\0RCX'
						&& version == 0x00020000
						&& header_length == sizeof (*this);
				}

				DWORD magic;
				DWORD version;
				DWORD header_length;
			};

			struct InputEvent
			{
				//JKB: device type changes & debug keyboard additions:
				enum Type
				{
					TYPE_EOF = 0,
					TYPE_WAIT = 1,
					TYPE_INPUT_CONNECT_GAMEPAD = 2,
					TYPE_INPUT_DISCONNECT_ANY_DEVICE = 3,
					TYPE_INPUT_GAMEPAD_STATE = 4,
					TYPE_INPUT_CONNECT_KEYBOARD = 5,
					TYPE_INPUT_KEYBOARD_STATE = 6,
                    TYPE_INPUT_TOGGLE_HUD = 7
				};

				InputEvent(Type type = TYPE_EOF) :
					type((BYTE) type)
				{
				}

				BYTE type;
				WORD frame_count_delta;
				WORD tick_count_delta;
			};

			struct WaitEvent
			{
				WaitEvent() :
					type(InputEvent::TYPE_WAIT)
				{
				}

				BYTE type;
				DWORD frame_count_delta;
				DWORD tick_count_delta;
			};

			struct EndEvent :
				public InputEvent
			{
				EndEvent() :
					InputEvent(TYPE_EOF)
				{
				}
			};

			//JKB:
			struct DeviceInputEvent :
				public InputEvent
			{
				DeviceInputEvent(Type type) :
					InputEvent(type)
				{
				}
				BYTE port;
			};

			//JKB:
			struct GamepadConnectEvent :
				public DeviceInputEvent
			{
				GamepadConnectEvent() :
					DeviceInputEvent(TYPE_INPUT_CONNECT_GAMEPAD)
				{
				}
				XINPUT_CAPABILITIES capabilities;
				XINPUT_GAMEPAD state;
			};

			//JKB:
			/*struct GamepadDisconnectEvent :
				public DeviceInputEvent
			{
				GamepadDisconnectEvent() :
					DeviceInputEvent(TYPE_INPUT_DISCONNECT_GAMEPAD)
				{
				}
			};*/

			//JKB:
			struct GamepadStateEvent :
				public DeviceInputEvent
			{
				GamepadStateEvent() :
					DeviceInputEvent(TYPE_INPUT_GAMEPAD_STATE)
				{
				}
				WORD mask;
				BYTE diff_data[sizeof (XINPUT_GAMEPAD)];
			};

			// Keyboard connect event support:
			struct KeyboardConnectEvent :
				public DeviceInputEvent
			{
				KeyboardConnectEvent() :
					DeviceInputEvent(TYPE_INPUT_CONNECT_KEYBOARD)
				{
				}
				XINPUT_CAPABILITIES capabilities;
				XINPUT_KEYSTROKE keystroke;
			};

			// Keyboard state event support:
			struct KeyboardStateEvent :
				public DeviceInputEvent
			{
				KeyboardStateEvent() :
					DeviceInputEvent(TYPE_INPUT_KEYBOARD_STATE)
				{
				}
				WORD mask;
				BYTE diff_data[sizeof (XINPUT_KEYSTROKE)];
			};

			struct AnyDeviceDisconnectEvent :
				public DeviceInputEvent
			{
				AnyDeviceDisconnectEvent() :
					DeviceInputEvent(TYPE_INPUT_DISCONNECT_ANY_DEVICE)
				{
				}
			};
		};

	}
}


#pragma pack(pop)