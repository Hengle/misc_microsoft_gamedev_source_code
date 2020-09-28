//	RandomInputSource.cpp : Random input source.
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

#include "xecr_RandomInputSource.h"




namespace XCR
{

	extern int xcr_rand();
	extern void xcr_srand(unsigned int seed);

	// ----------------------------------------------------------------
	// Constructors / destructor
	// ----------------------------------------------------------------

	RandomInputSource::RandomInputSource(void) :
		m_selected_ports(0xF),
		m_random_seed(0)
	{
		int i;
		for (i = 0; i < INPUT_DEVICE_PORTS; i++)
		{
			// All ports are initially fully usable.
			m_port_random_parameters[i].connected = PortRandomParameters::RANDOM;
			m_port_random_parameters[i].connect_frequency = 1;
			m_port_random_parameters[i].interval = 0;
			m_port_random_parameters[i].last_change = 0;
			for (int j = CONTROL_FIRST; j < CONTROL_COUNT; j++)
			{
				m_port_random_parameters[i].controls[j].change_frequency = 50;
				m_port_random_parameters[i].controls[j].reset_frequency = 750;
				m_port_random_parameters[i].controls[j].default_value = 0;
			}
			//JKBXAM 9/25 - addition of guide button (HUD) support to monkey player.  set change freq to 1,
			//    otherwise freq of HUD toggles is too great...
			m_port_random_parameters[i].controls[CONTROL_GUIDEBUTTON].change_frequency = 0;
		}
	}


	RandomInputSource::~RandomInputSource(void)
	{
		// Nothing to destroy.
	}




	// ----------------------------------------------------------------
	// IComponent interface
	// ----------------------------------------------------------------


	// Connect component to the XCR Control Unit.
	bool RandomInputSource::Connect(IControllerInterface *pxcr)
	{
		update_connections();
		return true;
	}




	// Set a property for this component.
	//	parameters:
	//		name : name of property to set
	//		value : value to set property to
	//	returns result code
	XCR::Result RandomInputSource::SetStringProperty(LPCSTR name, LPCSTR value1, LPCSTR value2)
	{
		// True if the command is "freq", "reset", or "default" respectively.
		bool changeCmd = (lstrcmpiA(name, "freq") == 0);
		bool resetCmd = (lstrcmpiA(name, "reset") == 0);
		bool defaultCmd = (lstrcmpiA(name, "default") == 0);

        bool changeCmdEx = (lstrcmpiA(name, "freqex") == 0);
        bool resetCmdEx = (lstrcmpiA(name, "resetex") == 0);
        bool defaultCmdEx = (lstrcmpiA(name, "defaultex") == 0);

		// Setting which ports to describe in subsequent operations?
		if (lstrcmpiA(name, "port") == 0)
		{
			if (lstrcmpiA(value1, "all") == 0)
			{
				// Select all ports.
				m_selected_ports = 0xF;
			}
			else
			{
				// Translate value1 into a number and set that as the current port.
				long port = atol(value1);
				if (port < 1 || port > INPUT_DEVICE_PORTS)
				{
					return XCR::Result::ERROR_INVALID_VALUE;
				}
				m_selected_ports = 1 << (port - 1);
			}
			return XCR::Result::SUCCESS;
		}

        // This is an alternative version of port that uses a bitwise value from 1 
        // to 15 to select which ports are selected.
        else if (lstrcmpiA(name, "portex") == 0)
        {
            long port = atol(value1);
            if (port < 1 || port > 0xF)
            {
                return XCR::Result::ERROR_INVALID_VALUE;
            }
            m_selected_ports = port;
            return XCR::Result::SUCCESS;
        }

		// Set random number seed to initiate playback. 0 means use clock.
		else if (lstrcmpiA(name, "seed") == 0)
		{
			// Translate value1 into a number and set that as the seed.
			m_random_seed = atol(value1);
			return XCR::Result::SUCCESS;
		}

        else if (changeCmdEx || resetCmdEx || defaultCmdEx)
        {
            // Ensure we have values for setting the property.
            if (value1 == NULL || value2 == NULL)
            {
                return XCR::Result::ERROR_TOO_FEW_ARGUMENTS;
            }

            // Convert the values to int's to use internally.
            int controlFlags = atoi(value1);
            int amount = atoi(value2);

            // Validate the amount as best we can.  For default, we just validate 
            // the extreme ranges not the limits for each control.
            if ( ((changeCmdEx || resetCmdEx) && (amount > 1000 || amount < 0)) ||
                 ((defaultCmdEx) && (amount < -32768 || amount > 32767))
               )
            {
                return XCR::Result::ERROR_INVALID_VALUE;
            }

            // See if each control is part of the bitwise flag that came over.
            for (int i=0; i<CONTROL_COUNT; ++i)
            {
                if (controlFlags & (1 << i))
                {
                    // If this control is not a stick or trigger, 
                    // ensure that the amount is within range.  If it 
                    // is not set it to the lower/upper bound depending 
                    // on the amount.  If it is within the range 
                    // just use the amount we were sent.
                    SHORT samount = (SHORT) amount;
                    switch (s_control_data[i].type)
                    {
                    case ControlData::TYPE_DIGITAL:
                        {
                            if (amount < 0)
                            {
                                samount = 0;
                            }
                            else if (amount > 1)
                            {
                                samount = 1;
                            }
                            break;
                        }
                    case ControlData::TYPE_ANALOG:
                        {
                            if (amount < 0)
                            {
                                samount = 0;
                            }
                            else if (amount > 255)
                            {
                                samount = 255;
                            }
                            break;
                        }
                    }

                    // We got a hit on each control.  For each selected 
                    // port update the control.
                    for (int j=0; j<INPUT_DEVICE_PORTS; ++j)
                    {
                        if (m_selected_ports & (1 << j))
                        {
                            // Based on the property we're changing update 
                            // the right member on the structure.
                            if (changeCmdEx)
                            {
                                m_port_random_parameters[j].controls[i].change_frequency = amount;
                            }
                            else if (resetCmdEx) 
                            {
                                m_port_random_parameters[j].controls[i].reset_frequency = amount;
                            }
                            else 
                            {
                                m_port_random_parameters[j].controls[i].default_value = (SHORT) amount;
                            }
                        }
                    }
                }
            }

            return XCR::Result::SUCCESS;
        }

		// Setting frequency that a control should be activated for the currently selected set of ports?
		else if (changeCmd || resetCmd || defaultCmd)
		{
            if (value1 == NULL || value2 == NULL)
            {
                return XCR::Result::ERROR_TOO_FEW_ARGUMENTS;
            }

			// Translate value1 into one of the controls.
			Control c = get_control_for_name(value1);
			if (c == CONTROL_INVALID)
			{
				return XCR::Result::ERROR_INVALID_VALUE;
			}

			// Translate value2 into a number.
			long amount = atol(value2);

			// Range checking for frequency.
			if ((changeCmd || resetCmd) && (amount > 1000 || amount < 0))
			{
				return XCR::Result::ERROR_INVALID_VALUE;
			}

			// Range checking for default value.
			if (defaultCmd)
			{
				switch (s_control_data[c].type)
				{
				case ControlData::TYPE_DIGITAL:
					if (amount < 0 || amount > 1) return XCR::Result::ERROR_INVALID_VALUE;
					break;
				case ControlData::TYPE_ANALOG:
					if (amount < 0 || amount > 255) return XCR::Result::ERROR_INVALID_VALUE;
					break;
				case ControlData::TYPE_THUMB:
					if (amount < -32768 || amount > 32767) return XCR::Result::ERROR_INVALID_VALUE;
					break;
				}
			}

			// Set the frequency for this control.
			for (int i = 0; i < INPUT_DEVICE_PORTS; i++)
			{
				if (m_selected_ports & (1 << i))
				{
                    if (CONTROL_COUNT == c)
                    {
                        for (int j=0; j<CONTROL_COUNT; ++j)
                        {
                            if (changeCmd) m_port_random_parameters[i].controls[j].change_frequency = amount;
                            else if (resetCmd) m_port_random_parameters[i].controls[j].reset_frequency = amount;
                            else m_port_random_parameters[i].controls[j].default_value = (SHORT) amount;
                        }
                    }
                    else
                    {
					    if (changeCmd) m_port_random_parameters[i].controls[c].change_frequency = amount;
					    else if (resetCmd) m_port_random_parameters[i].controls[c].reset_frequency = amount;
					    else m_port_random_parameters[i].controls[c].default_value = (SHORT) amount;
                    }
				}
			}

			return XCR::Result::SUCCESS;
		}

		// Setting whether the port should be connected to or not?
		else if (lstrcmpiA(name, "connect") == 0)
		{
			// Always keep the port connected?
			if (lstrcmpiA(value1, "always") == 0)
			{
				for (int port = 0; port < INPUT_DEVICE_PORTS; port++)
				{
					if (m_selected_ports & (1 << port))
					{
						m_port_random_parameters[port].connected = PortRandomParameters::ALWAYS;
					}
				}
			}

			// Never connect to this port?
			else if (lstrcmpiA(value1, "never") == 0)
			{
				for (int port = 0; port < INPUT_DEVICE_PORTS; port++)
				{
					if (m_selected_ports & (1 << port))
					{
						m_port_random_parameters[port].connected = PortRandomParameters::NEVER;
					}
				}
			}

			// Randomly connect and disconnect from this port?
			else
			{
				long freq = atol(value1);
				// If this was a zero, check to see if this was really a "0" or just a bogus string
				if (freq == 0 && *value1 != '0')
				{
					return XCR::Result::ERROR_INVALID_VALUE;
				}
				else if (freq < 0 || freq > 1000)
				{
					return XCR::Result::ERROR_INVALID_VALUE;
				}
				for (int port = 0; port < INPUT_DEVICE_PORTS; port++)
				{
					if (m_selected_ports & (1 << port))
					{
						m_port_random_parameters[port].connect_frequency = freq;
						m_port_random_parameters[port].connected = PortRandomParameters::RANDOM;
					}
				}
			}

			return XCR::Result::SUCCESS;
		}

		// Setting the minimum interval between random updates
		else if (lstrcmpiA(name, "interval") == 0)
		{
			long interval = atol(value1);
			if (interval < 0 || interval > 10000)
			{
				return XCR::Result::ERROR_INVALID_VALUE;
			}
			// Set interval accordingly
			for (int port = 0; port < INPUT_DEVICE_PORTS; port++)
			{
				if (m_selected_ports & (1 << port))
				{
					m_port_random_parameters[port].interval = interval;
				}
			}
			return XCR::Result::SUCCESS;
		}
		return XCR::Result::ERROR_INVALID_PROPERTY;
	}


	// Start this component.
	//	returns result code
	XCR::Result RandomInputSource::Start()
	{
		if (m_is_playing)
		{
			return XCR::Result::ERROR_ALREADY_PLAYING;
		}

		// Initialize random number seed
		if (0 == m_random_seed)
			xcr_srand(GetTickCount());
		else
			xcr_srand(m_random_seed);

        // MMILLS: Removed redundant code and leveraged the functionality in update_connections.
        update_connections();

		m_is_playing = true;
		return XCR::Result::SUCCESS;
	}


	// Stop this component.
	//	returns result code
	// a-dannat The monkey reconnects the ports to be back to the way they were when it started
	XCR::Result RandomInputSource::Stop()
	{
		if (!m_is_playing)
		{
			return XCR::Result::ERROR_NOT_PLAYING;
		}
		
		// a-dannat re-detect the ports, do connection and disconnection to set the ports back to the
		//  state before the monkey started pulling plugs.
		XINPUT_CAPABILITIES pCapabilities;
		for (int port = 0; port < INPUT_DEVICE_PORTS; port++)
		{
			// See if there is a device attached to this port
			DWORD resultCode = XInputGetCapabilities(port, 0, &pCapabilities);
			if ( ERROR_SUCCESS == resultCode )
			{
				if ( !m_ports->IsConnected(port) )
				{
					m_ports->SetDeviceType(port, pCapabilities.Type);
					m_ports->Connect(port, pCapabilities);
				}
			}
			else if ( m_ports->IsConnected(port) )
				m_ports->Disconnect(port);
		}

		m_is_playing = false;
		return XCR::Result::SUCCESS;
	}

	// Retrieve status.
	XCR::Result RandomInputSource::GetStatus()
	{
		return m_is_playing
			? XCR::Result::STATUS_PLAYING
			: XCR::Result::STATUS_IDLE;
	}

	//JKBNEW:
	XCR::Result RandomInputSource::SaveState(DirectDiskOutputStream* ddos)
	{
		//???
		// ddos->Write((LPVOID)this, (DWORD)sizeof(this));

		// Write random parameters data:
		for (int i = 0; i < INPUT_DEVICE_PORTS; i++)
		{
			if (!ddos->Write((LPVOID)&m_port_random_parameters[i],(DWORD)sizeof(PortRandomParameters)))
				return XCR::Result::ERROR;
		}

		// Write selected ports data:
		if (!ddos->Write((LPVOID)&m_selected_ports,(DWORD)sizeof(DWORD)))
			return XCR::Result::ERROR;

		return XCR::Result::SUCCESS;
	}

	//JKBNEW:
	XCR::Result RandomInputSource::ResumeState(DirectDiskInputStream* ddis)
	{
		// REad random parameters data:
		for (int i = 0; i < INPUT_DEVICE_PORTS; i++)
		{
			if (!ddis->Read((LPVOID)&m_port_random_parameters[i],(DWORD)sizeof(PortRandomParameters)))
				return XCR::Result::ERROR;
		}

		// Read selected ports data:
		if (!ddis->Read((LPVOID)&m_selected_ports,(DWORD)sizeof(DWORD)))
			return XCR::Result::ERROR;

		return XCR::Result::SUCCESS;
	}


	// ----------------------------------------------------------------
	// IPlayer interface
	// ----------------------------------------------------------------


	// Initialize this component.
	//	parameters:
	//		pXCR : a pointer to the XCR Control Unit Ports interface used to connect and disconnect devices.
	//	returns result code.
	XCR::Result RandomInputSource::Initialize(IPlayerInterface *pXCR)
	{
		m_ports = pXCR;
		return XCR::Result::SUCCESS;
	}

	// Notify this component it is being selected.
	XCR::Result RandomInputSource::SelectComponent()
	{
		// We don't care which controllers are actually selected.
		// We will just work with whatever happens to be there.
		return XCR::Result::SUCCESS;
	}


	// Emulate XInputGetState
	XCR::Result RandomInputSource::InputGetState(DWORD &xresult, DWORD port, PXINPUT_STATE pState)
	{
		if(!m_is_playing)
		{
			return XCR::Result::ERROR_NOT_PLAYING;
		}

		bool something_changed = false;

		// Grab current state.
		pState->Gamepad = m_ports->GetGamepadState(port);

		// Make sure minimum interval has passed or else make a quick exit
		if ( 0 != m_port_random_parameters[port].interval )
		{
			DWORD currentTime = GetTickCount();
			if ( (currentTime - m_port_random_parameters[port].last_change) < m_port_random_parameters[port].interval )
			{
				// Too recent ... exit without changing anything
				xresult = ERROR_SUCCESS;
				return XCR::Result::SUCCESS;
			}
			else
			{
				// Update refresh time
				m_port_random_parameters[port].last_change = currentTime;
			}
		}

		// Shall we connect or disconnect?
		if (m_port_random_parameters[port].connected == PortRandomParameters::RANDOM
			&& (DWORD) xcr_rand() % 1000 < m_port_random_parameters[port].connect_frequency)
		{
			// Toggle the state.
			//JKB
			if (m_ports->IsConnected(port))
			{
                update_port(port, false);
			}
			else
			{
                update_port(port, true);
			}
		}

		// Get out of here if not connected
		if ( !m_ports->IsConnected(port) )
		{
			xresult = ERROR_DEVICE_NOT_CONNECTED;
			return XCR::Result::SUCCESS;
		}


		// a-dannat variables used to generate keystrokes as we push buttons
		WORD ThumbstickKey;
		WORD NewThumbstickKey;
		XINPUT_KEYSTROKE Keystroke;
		memset(&Keystroke, 0, sizeof(Keystroke));

		// Let's make up some state data!
		for (Control c = CONTROL_FIRST; c < CONTROL_COUNT; c = (Control) (c + 1))
		{
			if ((DWORD) xcr_rand() % 1000 < m_port_random_parameters[port].controls[c].change_frequency)
			{
				// Change the value of this control,  sometimes to a random value, sometimes to 0.
				SHORT r = ((DWORD) xcr_rand() % 1000 < m_port_random_parameters[port].controls[c].reset_frequency)
					? m_port_random_parameters[port].controls[c].default_value
					: (SHORT) ((s_control_data[c].type == ControlData::TYPE_THUMB) ? (xcr_rand() << 1) : xcr_rand());
				// Alter control data.
				switch (s_control_data[c].type)
				{
				case ControlData::TYPE_DIGITAL:
					// a-dannat (r & 1) is probably not what is meant here. The problem is that a random even number counts as 0.
					// You probably want any value that is not the default value to count as ~default value for digitial controls
					if (r & 1) // ( (pState->Gamepad.wButtons & s_control_data[c].bit_mask) == 0 )
					{
						pState->Gamepad.wButtons |= s_control_data[c].bit_mask;
						// a-dannat generate a keystroke for this event
						Keystroke.Flags = XINPUT_KEYSTROKE_KEYDOWN;
						Keystroke.VirtualKey = s_control_keystroke[c];
						m_ports->SetKeyboardState(port, Keystroke);
					}
					else 
					{
						pState->Gamepad.wButtons &= ~s_control_data[c].bit_mask;
						// a-dannat generate a keystroke for this event
						Keystroke.Flags = XINPUT_KEYSTROKE_KEYUP;
						Keystroke.VirtualKey = s_control_keystroke[c];
						m_ports->SetKeyboardState(port, Keystroke);
					}
					break;

				// MMILLS: MARCH, 2005 XDK UPDATE
				case ControlData::TYPE_ANALOG:
				    //pState->Gamepad.bAnalogButtons[s_control_data[c].analog_index] = (BYTE) (r & 0xFF);
                    pState->Gamepad.*(s_trigger_member_pointers[s_control_data[c].trigger_index]) = (BYTE) (r & 0xFF);

					// a-dannat generate a keystroke for this event
					Keystroke.Flags = (BYTE) (r & 0xFF)?XINPUT_KEYSTROKE_KEYUP:XINPUT_KEYSTROKE_KEYDOWN;
					Keystroke.VirtualKey = s_control_keystroke[c];
					m_ports->SetKeyboardState(port, Keystroke);
					break;

				case ControlData::TYPE_THUMB:
					// a-dannat Convert the thumb stick state to a virtual key
					ThumbstickKey = GetVirtualThumbstickKey( pState->Gamepad, s_control_keystroke[c] );

					// a-dannat Then change the gamepad thumb stick state
					pState->Gamepad.*(s_thumb_member_pointers[s_control_data[c].thumb_index]) = (SHORT) (r & 0xFFFF);

					// a-dannat Now check what the thumb stick state mapped to a virtual key is.
					NewThumbstickKey = GetVirtualThumbstickKey( pState->Gamepad, s_control_keystroke[c] );

					// generate a keystroke release for this stick unless we are still holding down the same stick
					if ( ThumbstickKey && (NewThumbstickKey != ThumbstickKey) )
					{
						Keystroke.Flags = XINPUT_KEYSTROKE_KEYUP;
						Keystroke.VirtualKey = ThumbstickKey;
						m_ports->SetKeyboardState(port, Keystroke);
					}
					//  Create a new keystroke event for the new thumb stick virtual key
					if ( NewThumbstickKey )
					{
						Keystroke.Flags = XINPUT_KEYSTROKE_KEYDOWN;
						Keystroke.VirtualKey = NewThumbstickKey;
						m_ports->SetKeyboardState(port, Keystroke);
					}
					break;

				//JKBXAM 9/25 - addition of guide button (HUD) support to monkey player.
				case ControlData::TYPE_GUIDE:
					if (r != m_port_random_parameters[port].controls[c].default_value)
					{
						IControllerInterface* base_core = static_cast<IControllerInterface*>(m_ports);
						base_core->ToggleHud(port);
					}
					break;
				}
				// Something changed.
				something_changed = true;
			}
		}
		pState->dwPacketNumber = pState->dwPacketNumber + (something_changed ? 1 : 0);
		xresult = ERROR_SUCCESS;
		return XCR::Result::SUCCESS;
	}

	// Emulate XInputGetKeystroke
	XCR::Result RandomInputSource::InputGetKeystroke(DWORD &xresult, DWORD dwPort, DWORD dwFlags, PXINPUT_KEYSTROKE pKeystroke)
	{
		// Call InputGetState to do the work of updating the Input Device.  
		// This in turn generates and adds the keystroke events to the m_ports buffer
		//  These can then be consumed by the caller as indicated with SUCCESS_UNHANDLED
		// We have to make sure we update the gamepad state that may have just changed 
		//  by taking care of the Gamepad structure returned. Not taking care of it resulted in InputGetState calls always returning empty states

		XINPUT_STATE GamepadState;

		GamepadState.Gamepad = m_ports->GetGamepadState(dwPort);

		InputGetState(xresult, dwPort, &GamepadState);

		m_ports->SetGamepadState(dwPort, GamepadState.Gamepad);

		xresult = ERROR_SUCCESS;
		return XCR::Result::SUCCESS_UNHANDLED;
	}


	// ----------------------------------------------------------------
	// Implementation
	// ----------------------------------------------------------------

	void RandomInputSource::update_connections()
	{
		// Make sure ports that should be always connected are, never aren't.
		for (int port = 0; port < INPUT_DEVICE_PORTS; port++)
		{
			// Disconnect devices that shouldn't be connected.
			// Keyboards are not supported in monkey play.
			if (m_ports->GetDeviceType(port) == XINPUT_DEVTYPE_USB_KEYBOARD)
				m_ports->Disconnect(port);

            // Disconnect devices that should not.
			if (m_port_random_parameters[port].connected == PortRandomParameters::NEVER)
			{
                update_port(port, false);
			}
			// Connect devices that should.
			else if (m_port_random_parameters[port].connected == PortRandomParameters::ALWAYS)
			{
                update_port(port, true);
			}
		}
	}

    void RandomInputSource::update_port(int port, bool connect)
    {
        // Connect or disconnect the port based on the connect 
        // parameter and the ports current state.
        if (true == connect && !m_ports->IsConnected(port))
        {
            m_ports->Connect(port, s_default_gamepad_capabilities);
        }
        else if (m_ports->IsConnected(port))
        {
            m_ports->Disconnect(port);
        }

        // If we were asked to update the port update the controls
        // whether the port was already connected or not.
        if (true == connect)
        {
            m_ports->Connect(port, s_default_gamepad_capabilities);
            XINPUT_GAMEPAD state;
            initialize_controls_to_defaults(port, state);
            m_ports->SetGamepadState(port, state);
        }
    }

    void RandomInputSource::initialize_controls_to_defaults(int port, XINPUT_GAMEPAD &state)
    {
        // Initialize everything to zero.
        memset(&state, 0, sizeof (state));

        // Spin through the controls and initialize the structure to their 
        // default state.
        for (Control c = CONTROL_FIRST; c < CONTROL_COUNT; c = (Control) (c + 1))
        {
            switch (s_control_data[c].type)
            {
            case ControlData::TYPE_DIGITAL:
                if (m_port_random_parameters[port].controls[c].default_value & 1)
                {
                    state.wButtons |= s_control_data[c].bit_mask;
                }
                else
                {
                    state.wButtons &= ~s_control_data[c].bit_mask;
                }
                break;
            case ControlData::TYPE_ANALOG:
                state.*(s_trigger_member_pointers[s_control_data[c].trigger_index]) = (BYTE) m_port_random_parameters[port].controls[c].default_value;
                break;
            case ControlData::TYPE_THUMB:
                state.*(s_thumb_member_pointers[s_control_data[c].thumb_index]) = (SHORT) m_port_random_parameters[port].controls[c].default_value;
                break;
            }
        }
	}

	// a-dannat Helper function to extrapolate a thumbstick virtual key from a gamepad state
	WORD RandomInputSource::GetVirtualThumbstickKey( XINPUT_GAMEPAD Gamepad, WORD Axis )
	{
		WORD xreturn = 0;
		SHORT XAxisValue = 0;
		SHORT YAxisValue = 0;
		if ( Axis == VK_PAD_LTHUMB_LEFT || Axis == VK_PAD_LTHUMB_UP )
		{
			// Check the left thumb stick
			XAxisValue = Gamepad.sThumbLX;
			YAxisValue = Gamepad.sThumbLY;
			if ( YAxisValue > 0 )
			{
				if ( XAxisValue > 0 )
					xreturn = VK_PAD_LTHUMB_UPRIGHT; 
				else if ( XAxisValue < 0 )
					xreturn = VK_PAD_LTHUMB_UPLEFT;
				else
					xreturn = VK_PAD_LTHUMB_UP;
			}
			else if ( YAxisValue < 0 )
			{
				if ( XAxisValue > 0 )
					xreturn = VK_PAD_LTHUMB_DOWNRIGHT; 
				else if ( XAxisValue < 0 )
					xreturn = VK_PAD_LTHUMB_DOWNLEFT;
				else
					xreturn = VK_PAD_LTHUMB_DOWN;
			}
			else if ( XAxisValue > 0 )
				xreturn = VK_PAD_LTHUMB_RIGHT;
			else if ( XAxisValue < 0 )
				xreturn = VK_PAD_LTHUMB_LEFT;
		}
		else if ( Axis == VK_PAD_RTHUMB_LEFT || Axis == VK_PAD_RTHUMB_UP )
		{
			// Now check the right thumb stick
			XAxisValue = Gamepad.sThumbRX;
			YAxisValue = Gamepad.sThumbRY;

			if ( YAxisValue > 0 )
			{
				if ( XAxisValue > 0 )
					xreturn = VK_PAD_RTHUMB_UPRIGHT; 
				else if ( XAxisValue < 0 )
					xreturn = VK_PAD_RTHUMB_UPLEFT;
				else
					xreturn = VK_PAD_RTHUMB_UP;
			}
			else if ( YAxisValue < 0 )
			{
				if ( XAxisValue > 0 )
					xreturn = VK_PAD_RTHUMB_DOWNRIGHT;  
				else if ( XAxisValue < 0 )
					xreturn = VK_PAD_RTHUMB_DOWNLEFT;
				else
					xreturn = VK_PAD_RTHUMB_DOWN;
			}
			else if ( XAxisValue > 0 )
				xreturn = VK_PAD_RTHUMB_RIGHT;
			else if ( XAxisValue < 0 )
				xreturn = VK_PAD_RTHUMB_LEFT;
		}
		return xreturn;
	}
}


#endif