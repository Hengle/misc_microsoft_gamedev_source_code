//	GamepadControls.cpp : Controller individual control (buttons, thumbsticks, triggers, etc.) data types.
//		Useful for input sources that accept control names.
//
//	Created 2004/10/12 Rich Bonny    <rbonny@microsoft.com>
//
//  - based on original XCR code:
//	Created 2003/03/XX David Eichorn <deichorn@microsoft.com>
//     and modified by Jon Burns     <jburns@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2004 Microsoft Corp.  All rights reserved.




#ifdef _XCR_

#include "xecr_GamepadControls.h"




namespace XCR
{

    // RWB: Structure redefined im May 05 XDK
    XINPUT_CAPABILITIES GamepadControls::s_default_gamepad_capabilities = 
	{
		//BYTE    Type;
		XINPUT_DEVTYPE_GAMEPAD,
		//BYTE    SubType;
		XINPUT_DEVSUBTYPE_GAMEPAD,
		//WORD    Reserved;
		0,
		// XINPUT_GAMEPAD
		{
			//WORD    wButtons;
			0xFFFF,
			//BYTE    bLeftTrigger;
            255,
            //BYTE    bRightTrigger;
            255,
			//SHORT   sThumbLX;
			-1,
			//SHORT   sThumbLY;
			-1,
			//SHORT   sThumbRX;
			-1,
			//SHORT   sThumbRY;
			-1
		},
		// XINPUT_VIBRATION
		{
			//WORD   wLeftMotorSpeed;
			0xFFFF,
			//WORD   wRightMotorSpeed;
			0xFFFF
		}
	};


    // MMILLS: MARCH, 2005 XDK UPDATE
	// Maps control enum onto data for control.
	//JKBXAM 9/25 - addition of guide button (HUD).
	GamepadControls::ControlData GamepadControls::s_control_data[CONTROL_COUNT] =
	{
		{ ControlData::TYPE_DIGITAL, XINPUT_GAMEPAD_DPAD_UP },
		{ ControlData::TYPE_DIGITAL, XINPUT_GAMEPAD_DPAD_DOWN },
		{ ControlData::TYPE_DIGITAL, XINPUT_GAMEPAD_DPAD_LEFT },
		{ ControlData::TYPE_DIGITAL, XINPUT_GAMEPAD_DPAD_RIGHT },
		{ ControlData::TYPE_DIGITAL, XINPUT_GAMEPAD_START },
		{ ControlData::TYPE_DIGITAL, XINPUT_GAMEPAD_BACK },
		{ ControlData::TYPE_DIGITAL, XINPUT_GAMEPAD_LEFT_THUMB },
		{ ControlData::TYPE_DIGITAL, XINPUT_GAMEPAD_RIGHT_THUMB },
		{ ControlData::TYPE_DIGITAL, XINPUT_GAMEPAD_A },
		{ ControlData::TYPE_DIGITAL, XINPUT_GAMEPAD_B },
		{ ControlData::TYPE_DIGITAL, XINPUT_GAMEPAD_X },
		{ ControlData::TYPE_DIGITAL, XINPUT_GAMEPAD_Y },
		{ ControlData::TYPE_DIGITAL, XINPUT_GAMEPAD_RIGHT_SHOULDER },
		{ ControlData::TYPE_DIGITAL, XINPUT_GAMEPAD_LEFT_SHOULDER },
        { ControlData::TYPE_ANALOG, ControlData::LTRIGGER },
        { ControlData::TYPE_ANALOG, ControlData::RTRIGGER },
		{ ControlData::TYPE_THUMB, ControlData::LTHUMBX },
		{ ControlData::TYPE_THUMB, ControlData::LTHUMBY },
		{ ControlData::TYPE_THUMB, ControlData::RTHUMBX },
		{ ControlData::TYPE_THUMB, ControlData::RTHUMBY },
		{ ControlData::TYPE_GUIDE, ControlData::GUIDE },
	};

	// a-dannat Maps the controls enum into XINPUT_KEYSTROKE.virtual_keys for XInputGetKeystroke calls
	// Is the VK_GUIDE virtual key value the same as the guide button?
	WORD GamepadControls::s_control_keystroke[CONTROL_COUNT] =
	{
		{ VK_PAD_DPAD_UP },
		{ VK_PAD_DPAD_DOWN },
		{ VK_PAD_DPAD_LEFT },
		{ VK_PAD_DPAD_RIGHT },
		{ VK_PAD_START },
		{ VK_PAD_BACK },
		{ VK_PAD_LTHUMB_PRESS },
		{ VK_PAD_RTHUMB_PRESS },
		{ VK_PAD_A },
		{ VK_PAD_B },
		{ VK_PAD_X },
		{ VK_PAD_Y },
		{ VK_PAD_RSHOULDER },
		{ VK_PAD_LSHOULDER },
        { VK_PAD_LTRIGGER },
        { VK_PAD_RTRIGGER },
		{ VK_PAD_LTHUMB_LEFT },  // These are really place holders, the really VK value can be one of 8 and is derived.
		{ VK_PAD_LTHUMB_UP },
		{ VK_PAD_RTHUMB_LEFT },
		{ VK_PAD_RTHUMB_UP },
		{ VK_GUIDE },
	};

	// Maps control names onto control enum.
	//JKBXAM 9/25 - addition of guide button (HUD).
	LPCSTR GamepadControls::s_control_names[CONTROL_COUNT] =
	{
		"DU",
		"DD",
		"DL",
		"DR",
		"S",
		"Z",
		"J",
		"C",
		"A",
		"B",
		"X",
		"Y",
		"RS",
		"LS",
		"LT",
		"RT",
		"JX",
		"JY",
		"CX",
		"CY",
		"H"
	};


	// Retrive a control enum from a control name.
	GamepadControls::Control GamepadControls::get_control_for_name(LPCSTR name)
	{
		// This would be better if it was a binary search, but we only have 20 controls...
        if (lstrcmpiA(name, "all") == 0)
        {
            return CONTROL_COUNT;
        }
		for (Control result = CONTROL_FIRST; result < CONTROL_COUNT; result = (Control) (result + 1))
		{
			if (lstrcmpiA(name, s_control_names[result]) == 0)
			{
				return result;
			}
		}
		return CONTROL_INVALID;
	}

	// Pointers to thumb control members, in order of thumb enum.
	SHORT XINPUT_GAMEPAD::*GamepadControls::s_thumb_member_pointers[] =
	{
		&XINPUT_GAMEPAD::sThumbLX,
		&XINPUT_GAMEPAD::sThumbLY,
		&XINPUT_GAMEPAD::sThumbRX,
		&XINPUT_GAMEPAD::sThumbRY
	};

    // MMILLS: MARCH, 2005 XDK UPDATE
    // Pointers to trigger control members, in order of trigger enum.
    BYTE XINPUT_GAMEPAD::*GamepadControls::s_trigger_member_pointers[] = 
    {
        &XINPUT_GAMEPAD::bLeftTrigger,
        &XINPUT_GAMEPAD::bRightTrigger
    };

}


#endif