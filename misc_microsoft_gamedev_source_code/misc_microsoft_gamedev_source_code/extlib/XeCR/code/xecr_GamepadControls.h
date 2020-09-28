//	GamepadControls.h : Controller individual control (buttons, thumbsticks, triggers, etc.) data types.
//		Useful for input sources that accept control names.
//
//	Created 2004/10/06 Rich Bonny    <rbonny@microsoft.com>
//
//  - based on original XCR code:
//	Created 2003/03/XX David Eichorn <deichorn@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2004 Microsoft Corp.  All rights reserved.




#pragma once

#include <xtl.h>




namespace XCR
{

	// ================================================================
	//	GamepadControls
	//
	//	Provides data about individual controls on the gamepad.
	//	Inherit from this to access data such as control names.
	// ================================================================
	class GamepadControls
	{
	protected:

		static XINPUT_CAPABILITIES s_default_gamepad_capabilities;


		// Enumeration of controls.
		//JKBXAM 9/25 - addition of guide button (HUD).
        enum Control
        {
            CONTROL_FIRST = 0,
            CONTROL_DPAD_UP = CONTROL_FIRST,
            CONTROL_DPAD_DOWN,
            CONTROL_DPAD_LEFT,
            CONTROL_DPAD_RIGHT,
            CONTROL_START,
            CONTROL_BACK,
            CONTROL_LEFT_THUMB,
            CONTROL_RIGHT_THUMB,
            CONTROL_A,
            CONTROL_B,
            CONTROL_X,
            CONTROL_Y,
            CONTROL_RIGHT_SHOULDER,
            CONTROL_LEFT_SHOULDER,
            CONTROL_LEFT_TRIGGER,
            CONTROL_RIGHT_TRIGGER,
            CONTROL_LTHUMBX,
            CONTROL_LTHUMBY,
            CONTROL_RTHUMBX,
            CONTROL_RTHUMBY,
            CONTROL_GUIDEBUTTON,
            CONTROL_COUNT,
            CONTROL_INVALID = -1,
        };

        enum BitwiseControl
        {
            BITWISECONTROL_FIRST            = 0x00000001,
            BITWISECONTROL_DPAD_UP          = BITWISECONTROL_FIRST,
            BITWISECONTROL_DPAD_DOWN        = 0x00000002,
            BITWISECONTROL_DPAD_LEFT        = 0x00000004,
            BITWISECONTROL_DPAD_RIGHT       = 0x00000008,
            BITWISECONTROL_START            = 0x00000010,
            BITWISECONTROL_BACK             = 0x00000020,
            BITWISECONTROL_LEFT_THUMB       = 0x00000040,
            BITWISECONTROL_RIGHT_THUMB      = 0x00000080,
            BITWISECONTROL_A                = 0x00000100,
            BITWISECONTROL_B                = 0x00000200,
            BITWISECONTROL_X                = 0x00000400,
            BITWISECONTROL_Y                = 0x00000800,
            BITWISECONTROL_RIGHT_SHOULDER   = 0x00001000,
            BITWISECONTROL_LEFT_SHOULDER    = 0x00002000,
            BITWISECONTROL_LEFT_TRIGGER     = 0x00004000,
            BITWISECONTROL_RIGHT_TRIGGER    = 0x00008000,
            BITWISECONTROL_LTHUMBX          = 0x00010000,
            BITWISECONTROL_LTHUMBY          = 0x00020000,
            BITWISECONTROL_RTHUMBX          = 0x00040000,
            BITWISECONTROL_RTHUMBY          = 0x00080000,
            BITWISECONTROL_GUIDEBUTTON      = 0x00100000,
            BITWISECONTROL_COUNT            = 0x00200000,
            BITWISECONTROL_INVALID          = -1,
        };

        // MMILLS: MARCH, 2005 XDK UPDATE
		// Data for control.
		//JKBXAM 9/25 - addition of guide button (HUD).
		struct ControlData
		{
			enum ControlDataType { TYPE_DIGITAL, TYPE_ANALOG, TYPE_THUMB, TYPE_GUIDE } type;
			union
			{
				DWORD bit_mask;
				DWORD analog_index;
				enum ControlDataThumbType { LTHUMBX, LTHUMBY, RTHUMBX, RTHUMBY } thumb_index;
                enum ControlDataTriggerType { LTRIGGER, RTRIGGER } trigger_index;
				enum ControlDataGuideType { GUIDE } guide_index;
			};
		};

		// Maps control enum onto data for control.
		static ControlData s_control_data[CONTROL_COUNT];

		// a-dannat Maps control enum to XINPUT_KEYSTROKE virtual keys
		static WORD s_control_keystroke[CONTROL_COUNT];

		// Maps control names onto control enum.
		static LPCSTR s_control_names[CONTROL_COUNT];

		// Pointers to thumb control members, in order of thumb enum.
		static SHORT XINPUT_GAMEPAD::*s_thumb_member_pointers[];

        // MMILLS: MARCH, 2005 XDK UPDATE
        // Pointers to trigger control members, in order of trigger enum.
        static BYTE XINPUT_GAMEPAD::*s_trigger_member_pointers[];

		// Retrive a control enum from a control name.
		Control get_control_for_name(LPCSTR name);
	};

}