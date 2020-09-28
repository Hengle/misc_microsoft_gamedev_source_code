//	TextSequencePlayer.cpp : Plays text sequences.
//
//	Created 2004/10/12 Rich Bonny    <rbonny@microsoft.com>
//
//  - based on original XCR code:
//	Created 2003/07/XX David Eichorn <deichorn@microsoft.com>
//     and modified by Jon Burns     <jburns@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2004 Microsoft Corp.  All rights reserved.




#ifdef _XCR_

#include "xecr_TextSequencePlayer.h"




namespace XCR
{
	namespace Sequence
	{

		// Construct a sequence.
		TextEventInput::TextEventInput()
		{
			m_fps = 30;
			for (DWORD port = 0; port < INPUT_DEVICE_PORTS; port++)
			{
				m_port_opened[port] = false;
			}
			m_retain_previous_port = false;
		}

		// Start.
		XCR::Result TextEventInput::Start()
		{
            for (DWORD port = 0; port < INPUT_DEVICE_PORTS; port++)
            {
                m_port_opened[port] = false;
            }
            m_retain_previous_port = false;

            reset_error_message();
			m_time = 0;
			m_frame = 0;
			m_type = EVENT_END;
			m_data_index = 0;

			// Check for no sequence specified: RWB Fix Bug #357
			if (0 == m_data.length())
			{
				return XCR::Result::ERROR_NO_SEQUENCE;
			}

			if (!get_next_char() || !parse_event())
			{
				add_error_message("Trying to parse event.");
				return XCR::Result::ERROR_CUSTOM;
			}

			return XCR::Result::SUCCESS;
		}


		// Stop.
		XCR::Result TextEventInput::Stop()
		{
			return XCR::Result::SUCCESS;
		}


		// Set string data to use.
		// Returns true if it parses ok.
		void TextEventInput::SetData(const char *data)
		{
            m_data_index = 0;
            if (0 == data)
            {
                m_data.empty();
            }
            else
            {
			    m_data = data;
                get_next_char();
            }
		}


		// Retrieve next char in sequence.
		bool TextEventInput::get_next_char()
		{
			if (m_data_index == m_data.length())
			{
				m_next_char = '\0';
				error_message = "Unexpected end of sequence.";
				return false;
			}
			m_next_char = m_data[m_data_index];
			++m_data_index;
			return true;
		}

        // Restore the previous character in sequence.
        bool TextEventInput::put_char_back()
        {
            if (0 == m_data_index)
            {
                m_next_char = '\0';
                error_message = "Unexpected begin of sequence.";
                return false;
            }
            --m_data_index;
            m_next_char = m_data[m_data_index];
            return true;
        }


		// Advance event.
		XCR::Result TextEventInput::AdvanceToNextEvent()
		{
			// First see if we are in the middle of a press event.
			//if (m_sequence_event_type == SequenceEventType::PRESS)
			if (m_sequence_event_type == PRESS)
			{
				// Change the press into a release after the specified duration.
				//m_sequence_event_type = SequenceEventType::RELEASE;
				m_sequence_event_type = RELEASE;
				m_frame += m_sequence_event_frames;
				m_time += m_sequence_event_ms;
			}			
			else
			{
				// Insert small delay.
				if (m_sequence_event_type != OTHER)
				{
					unsigned long delay_frames = 5;
					unsigned long delay_ms = 1000 / m_fps;
					m_time += delay_ms;
					m_frame += delay_frames;
				}
				// Grab a new event.
				if (m_next_char == '\0')
				{
					m_type = EVENT_END;
				}
				else if (!parse_event())
				{
					add_error_message("Trying to parse event.");
					m_type = EVENT_END;
					return XCR::Result::ERROR_CUSTOM;
				}
			}
			return XCR::Result::SUCCESS;
		}


		std::string TextEventInput::GetErrorMessage() const
		{
			std::string msg = error_message;
			msg += " -- at character ";
			char index_buf[12];
			ultoa(m_data_index, index_buf, 10);
			msg += index_buf;
			if (m_next_char != '\0')
			{
				msg += " '";
				msg += m_next_char;
				msg += "'";
			}
			msg += ".\n";
			return msg;
		}

		//JKBNEW:
		XCR::Result TextEventInput::SaveState(DirectDiskOutputStream* ddos)
		{
			//???
			// ddos->Write((LPVOID)this, (DWORD)sizeof(this));

			// Write m_data len:
			int len = m_data.length();//sizeof(m_data);
			if (!ddos->Write((LPVOID)&len,(DWORD)sizeof(int)))
				return XCR::Result::ERROR;
			
			if (len > 0)
			{		
				// Write m_data string:
				if (!ddos->Write((LPVOID)m_data.data(),(DWORD)len))
					return XCR::Result::ERROR;
			}	

			// Write m_data size type:
            if (!ddos->Write((LPVOID)&m_data_index,(DWORD)sizeof(std::string::size_type)))
				return XCR::Result::ERROR;

			// Write next char:
			if (!ddos->Write((LPVOID)&m_next_char,(DWORD)sizeof(char)))
				return XCR::Result::ERROR;

			// Write fps:
			if (!ddos->Write((LPVOID)&m_fps,(DWORD)sizeof(DWORD)))
				return XCR::Result::ERROR;

			// Write event type:
			if (!ddos->Write((LPVOID)&m_type,(DWORD)sizeof(EventType)))
				return XCR::Result::ERROR;
			
			// Write time member info:
			if (!ddos->Write((LPVOID)&m_time,(DWORD)sizeof(DWORD)))
				return XCR::Result::ERROR;
			
			// Write frame member info:
			if (!ddos->Write((LPVOID)&m_frame,(DWORD)sizeof(DWORD)))
				return XCR::Result::ERROR;
			
            // Write port member info:
            if (!ddos->Write((LPVOID)&m_port,(DWORD)sizeof(DWORD)))
                return XCR::Result::ERROR;

            // Write combined port member info:
            if (!ddos->Write((LPVOID)&m_combined_port,(DWORD)sizeof(DWORD)))
                return XCR::Result::ERROR;

			// Write capabilities info:
			if (!ddos->Write((LPVOID)&m_capabilities, (DWORD)sizeof(XINPUT_CAPABILITIES)))
				return XCR::Result::ERROR;

			// Write seq event controls:
			if (!ddos->Write((LPVOID)&m_sequence_event_controls, (DWORD)(sizeof(ControlEventData)*CONTROL_COUNT)))
				return XCR::Result::ERROR;

			// Write seq event frames:
			if (!ddos->Write((LPVOID)&m_sequence_event_frames,(DWORD)sizeof(int)))
				return XCR::Result::ERROR;
			
			// Write seq event ms:
			if (!ddos->Write((LPVOID)&m_sequence_event_ms,(DWORD)sizeof(int)))
				return XCR::Result::ERROR;
			
			// Write seq event type:
			if (!ddos->Write((LPVOID)&m_sequence_event_type,(DWORD)sizeof(SequenceEventType)))
				return XCR::Result::ERROR;
			
			return XCR::Result::SUCCESS;
		}

		//JKBNEW:
		XCR::Result TextEventInput::ResumeState(DirectDiskInputStream* ddis)
		{
			//???
			// ddis->Read((LPVOID)this, (DWORD)sizeof(this));

			// Read m_data len:
			int len = 0;
			if (!ddis->Read((LPVOID)&len,(DWORD)sizeof(int)))
				return XCR::Result::ERROR;
			
			if (len > 0)
			{		
				char* tempStr = new char[len+1];
				tempStr[len] = '\0';
				// Read m_data string:
				if (!ddis->Read((LPVOID)tempStr,(DWORD)len))
					return XCR::Result::ERROR;

				m_data = tempStr;

				delete [] tempStr;
			}	

			// Read m_data size type:
            if (!ddis->Read((LPVOID)&m_data_index,(DWORD)sizeof(std::string::size_type)))
				return XCR::Result::ERROR;

			// Read next char:
			if (!ddis->Read((LPVOID)&m_next_char,(DWORD)sizeof(char)))
				return XCR::Result::ERROR;

			// Read fps:
			if (!ddis->Read((LPVOID)&m_fps,(DWORD)sizeof(DWORD)))
				return XCR::Result::ERROR;

			// Read event type:
			if (!ddis->Read((LPVOID)&m_type,(DWORD)sizeof(EventType)))
				return XCR::Result::ERROR;
			
			// Read time member info:
			if (!ddis->Read((LPVOID)&m_time,(DWORD)sizeof(DWORD)))
				return XCR::Result::ERROR;
			
			// Read frame member info:
			if (!ddis->Read((LPVOID)&m_frame,(DWORD)sizeof(DWORD)))
				return XCR::Result::ERROR;
			
            // Read port member info:
            if (!ddis->Read((LPVOID)&m_port,(DWORD)sizeof(DWORD)))
                return XCR::Result::ERROR;

            // Read port member info:
            if (!ddis->Read((LPVOID)&m_combined_port,(DWORD)sizeof(DWORD)))
                return XCR::Result::ERROR;

			// Read capabilities info:
			if (!ddis->Read((LPVOID)&m_capabilities, (DWORD)sizeof(XINPUT_CAPABILITIES)))
				return XCR::Result::ERROR;

			// Read seq event controls:
			if (!ddis->Read((LPVOID)&m_sequence_event_controls, (DWORD)(sizeof(ControlEventData)*CONTROL_COUNT)))
				return XCR::Result::ERROR;

			// Read seq event frames:
			if (!ddis->Read((LPVOID)&m_sequence_event_frames,(DWORD)sizeof(int)))
				return XCR::Result::ERROR;
			
			// Read seq event ms:
			if (!ddis->Read((LPVOID)&m_sequence_event_ms,(DWORD)sizeof(int)))
				return XCR::Result::ERROR;
			
			// Read seq event type:
			if (!ddis->Read((LPVOID)&m_sequence_event_type,(DWORD)sizeof(SequenceEventType)))
				return XCR::Result::ERROR;
			
			return XCR::Result::SUCCESS;
		}

        // MMILLS: MARCH, 2005 XDK UPDATE, changed from L,R,K,W to LT,RT,RS,LS
		// Parse next event from a sequence.
		// At end of successful parse, next char will be the first one after the event.
		bool TextEventInput::parse_event()
		{
			// sequence : event ( ',' event  )*
			// event : ( port number )? ( ( hold | release )? control action ) | wait ( duration )? ) | insert | remove
			// event : ( '1' | '2' | '3' | '4' )? ( ( '_' | '^' )? control_action ) | ( 'T' ( ulong )? ) | 'I' | 'O'
			// control_action_group : control_action ( '+' control_action )*
			// control_action : code (amount)? 

			// Reset defaults.
			if (m_retain_previous_port)
			{
				m_retain_previous_port = false;
			}
			else
			{
				m_port = 0;
                m_combined_port = 0;
			}
			reset_error_message();
			memset(m_sequence_event_controls, 0, sizeof (m_sequence_event_controls));
			m_sequence_event_type = OTHER;

			// parse event
			bool done = false;
			while (!done)
			{
				done = true;
				// parse initial port number or time delay command.
				switch (m_next_char)
				{
                // A combination port number?
                case 'P':
                    {
                        get_next_char();
                        if ('1' == m_next_char)
                        {
                            m_port = 0;
                            m_combined_port = 0;
                            get_next_char();
                            if ('0' <= m_next_char && '9' >= m_next_char)
                            {
                                if ('0' <= m_next_char && '5' >= m_next_char)
                                {
                                    m_combined_port = 10 + m_next_char - '0';
                                }
                                get_next_char();
                            }
                        }
                        else if ('2' <= m_next_char && '9' >= m_next_char)
                        {
                            switch (m_next_char)
                            {
                            case '2':
                                {
                                    m_port = 1;
                                    break;
                                }
                            case '4':
                                {
                                    m_port = 2;
                                    break;
                                }
                            case '8':
                                {
                                    m_port = 3;
                                    break;
                                }
                            default:
                                {
                                    m_combined_port = m_next_char - '0';
                                    break;
                                }
                            }
                            get_next_char();
                        }
                        if ('0' <= m_next_char && '9' >= m_next_char)
                        {
                            add_error_message("Invalid combined port sequence.  P must be followed by a number from 1-15.");
                            return false;
                        }
                        break;
                    }
				// A port number?
				case '1': 	case '2': 	case '3': 	case '4':
                    {
					    m_port = m_next_char - '1';
					    get_next_char();
                        if ('0' <= m_next_char && '9' >= m_next_char)
                        {
                            add_error_message("Invalid port sequence.  Port must be a number from 1-4.");
                            return false;
                        }
					    break;
                    }
				// A time delay command?
				case 'T':
					get_next_char();
					unsigned long delay_frames = 5;
					unsigned long delay_ms = 1000 / m_fps;
					if (m_next_char >= '0' && m_next_char <= '9')
					{
						if (!parse_duration(delay_frames, delay_ms))
						{
							add_error_message("Incomplete wait ('T') command.");
							return false;
						}
						m_time += delay_ms;
						m_frame += delay_frames;
					}
					done = false;
					// Eat separator.
					if (m_next_char == ',') 
						get_next_char();
					//JKBNEW: bug fix: allows user to execute sequences that end with a time wait...
					else if (m_next_char == '\0')
					{
						done = true;
					}

					continue;
				}

				// parse action
				unsigned long delay_frames = 5;
				unsigned long delay_ms = 1000 / m_fps;
				switch (m_next_char)
				{
				// Hold prefix?
				case '_':
					// Prior to attempting any control action, we make sure the referenced port
					// is connected.
					if (must_open_port())
					{
						break;
					}
					// ... otherwise, proceed.
					m_type = EVENT_KEYSTROKE;
					m_sequence_event_type = HOLD;
					if (!get_next_char() || !parse_control_action_group())
					{
						add_error_message("Expected control action group following hold prefix ('_').");
						return false;
					}
					break;
				// Release prefix?
				case '^':
					// Prior to attempting any control action, we make sure the referenced port
					// is connected.
					if (must_open_port())
					{
						break;
					}
					// ... otherwise, proceed.
					m_type = EVENT_KEYSTROKE;
					m_sequence_event_type = RELEASE;
					if (!get_next_char() || !parse_control_action_group())
					{
						add_error_message("Expected control action group following release prefix ('^').");
						return false;
					}
					break;
				// Control action?
				case 'A': case 'B': case 'X': case 'Y': case 'L': case 'R':
				case 'S': case 'Z':
				case 'J': case 'C': case 'D':
					// Prior to attempting any control action, we make sure the referenced port
					// is connected.
					if (must_open_port())
					{
						break;
					}
					// ... otherwise, proceed.
					m_type = EVENT_KEYSTROKE;
					m_sequence_event_type = PRESS;
					if (!parse_control_action_group())
					{
						add_error_message("Parsing control action group.");
						return false;
					}
					// Parse optional duration (only applies to press, not hold and release).
					if (m_next_char == '/')
					{
						if (!get_next_char() || !parse_duration(delay_frames, delay_ms))
						{
							add_error_message("Incomplete control wait suffix '/'.");
							return false;
						}
					}
					break;
				// Insert gamepad.
				case 'I':
					//JKB
					m_type = EVENT_CONNECT_GAMEPAD;
					m_capabilities = s_default_gamepad_capabilities;
					m_port_opened[m_port] = true;
					get_next_char();
					break;
				// Remove gamepad.
				case 'O':
					//JKB: ?? SHOULD THIS BE GAMEPAD SPECIFIC??
					m_type = EVENT_DISCONNECT_ANY_DEVICE;
					m_port_opened[m_port] = false;
					get_next_char();
					break;
                // Toggle hud.
                case 'H':
                    // MJMXAM: added hud toggling support
                    // Prior to attempting any control action, we make sure the referenced port
                    // is connected.
                    if (must_open_port())
                    {
                        break;
                    }
                    m_type = EVENT_TOGGLE_HUD;
                    get_next_char();
                    break;

				// Unexpected character.
				default:
					add_error_message("Event character expected.");
					return false;
				}
				m_sequence_event_ms = delay_ms;
				m_sequence_event_frames = delay_frames;
			}

			// Eat separator.
			if (m_next_char == ',') get_next_char();
			return true;
		}



        // MMILLS: MARCH, 2005 XDK UPDATE, changed from L,R,K,W to LT,RT,RS,LS
		// Parse a group of control actions.
		// Fills out m_sequence_event_controls for all control actions in the group.
		// On return, next character is after the end of the last parsed action.
		bool TextEventInput::parse_control_action_group()
		{
			// control_action_group : control_action ( '+' control_action )*
			// control_action : code (amount)?
			if (!parse_control_action())
			{
				add_error_message("Expecting control action.");
				return false;
			}
			static const char *another_control_action_message = "Expecting another control action following '+'.";
			while (m_next_char == '+')
			{
				get_next_char();
				switch (m_next_char)
				{
				case 'A': case 'B': case 'X': case 'Y': case 'L': case 'R':
				case 'S': case 'Z':
				case 'J': case 'C': case 'D':
					if (!parse_control_action())
					{
						add_error_message(another_control_action_message);
						return false;
					}
					break;

				default:
					add_error_message(another_control_action_message);
					return false;
				}
			}
			return true;
		}


        // MMILLS: MARCH, 2005 XDK UPDATE, changed from L,R,K,W to LT,RT,RS,LS
		// Parse a control action.
		// Fills out m_sequence_event_controls for control action parsed.
		// On return, next character is after the end of the parsed action, or recognizable subset thereof.
		bool TextEventInput::parse_control_action()
		{
			// control_action : code (amount)?

			Control control[2] = { CONTROL_INVALID, CONTROL_INVALID };
			int direction[2] = { 0, 0 };

			// Interpret control code into one or two affected controls.
			char c0 = m_next_char;
			switch (c0)
			{
			// Digital button.
            case 'A': control[0] = CONTROL_A; get_next_char(); break;
            case 'B': control[0] = CONTROL_B; get_next_char(); break;
            case 'X': control[0] = CONTROL_X; get_next_char(); break;
            case 'Y': control[0] = CONTROL_Y; get_next_char(); break;
			case 'S': control[0] = CONTROL_START; get_next_char(); break;
			case 'Z': control[0] = CONTROL_BACK; get_next_char(); break;

            // Handle L and R.
            case 'L':
                {
                    static const char *expecting_direction_message = "Expecting left shoulder or trigger (LS or LT).";
                    if (!get_next_char())
                    {
                        add_error_message(expecting_direction_message);
                        return false;
                    }
                    char st = m_next_char;
                    switch (st)
                    {
                    case 'S':
                        {
                            control[0] = CONTROL_LEFT_SHOULDER;
                            break;
                        }
                    case 'T':
                        {
                            control[0] = CONTROL_LEFT_TRIGGER;
                            break;
                        }
                    default:
                        {
                            add_error_message(expecting_direction_message);
                            return false;
                        }
                    }
                    get_next_char();
                    break;
                }
            case 'R':
                {
                    static const char *expecting_direction_message = "Expecting right shoulder or trigger (RS or RT).";
                    if (!get_next_char())
                    {
                        add_error_message(expecting_direction_message);
                        return false;
                    }
                    char st = m_next_char;
                    switch (st)
                    {
                    case 'S':
                        {
                            control[0] = CONTROL_RIGHT_SHOULDER;
                            break;
                        }
                    case 'T':
                        {
                            control[0] = CONTROL_RIGHT_TRIGGER;
                            break;
                        }
                    default:
                        {
                            add_error_message(expecting_direction_message);
                            return false;
                        }
                    }
                    get_next_char();
                    break;
                }

			// Digital directional control.
			case 'D':
				{
					// Must be followed by a direction.
					static const char *expecting_direction_message = "Expecting direction (U, D, L, R, UL, UR, DL, DR).";
					if (!get_next_char())
					{
						add_error_message(expecting_direction_message);
						return false;
					}
					char ud = m_next_char;
					switch (ud)
					{
					case 'U': case 'D':
						control[0] = ud == 'U' ? CONTROL_DPAD_UP : CONTROL_DPAD_DOWN;
						// Up and down can be followed by left and right.
						if (get_next_char())
						{
							switch (m_next_char)
							{
							case 'L': control[1] = CONTROL_DPAD_LEFT; get_next_char(); break;
							case 'R': control[1] = CONTROL_DPAD_RIGHT; get_next_char(); break;
							}
						}
						break;
					case 'L': control[0] = CONTROL_DPAD_LEFT; get_next_char(); break;
					case 'R': control[0] = CONTROL_DPAD_RIGHT; get_next_char(); break;
					default:
						add_error_message(expecting_direction_message);
						return false;
					}
				}
				break;

			// Analog directional control or stick button.
			case 'J': case 'C':
				// Can be followed by a direction.
				if (get_next_char())
				{
					char ud = m_next_char;
					switch (ud)
					{
					case 'U': case 'D':
						control[0] = c0 == 'J' ? CONTROL_LTHUMBY : CONTROL_RTHUMBY;
						direction[0] = ud == 'U' ? +1 : -1;
						// Up and down can be followed by left and right.
						if (get_next_char())
						{
							switch (m_next_char)
							{
							case 'L': control[1] = c0 == 'J' ? CONTROL_LTHUMBX : CONTROL_RTHUMBX; direction[1] = -1; get_next_char(); break;
							case 'R': control[1] = c0 == 'J' ? CONTROL_LTHUMBX : CONTROL_RTHUMBX; direction[1] = +1; get_next_char(); break;
							}
						}
						break;
					case 'L': control[0] = c0 == 'J' ? CONTROL_LTHUMBX : CONTROL_RTHUMBX; direction[0] = -1; get_next_char(); break;
					case 'R': control[0] = c0 == 'J' ? CONTROL_LTHUMBX : CONTROL_RTHUMBX; direction[0] = +1; get_next_char(); break;
						break;
					default:
						// Not a direction, so must indicate a stick button.
						control[0] = c0 == 'J' ? CONTROL_LEFT_THUMB : CONTROL_RIGHT_THUMB;
					}
				}
				else
				{
					// Not a direction, so must indicate a stick button.
					control[0] = c0 == 'J' ? CONTROL_LEFT_THUMB : CONTROL_RIGHT_THUMB;
				}
				break;

			default:
				add_error_message("Expected control code.");
				return false;
			}

			// See if control code is followed by activation amount.
			unsigned long amount = 32767;
			if (s_control_data[control[0]].type != GamepadControls::ControlData::TYPE_DIGITAL)
			{
				parse_ulong(amount);
				reset_error_message();
			}
			switch (s_control_data[control[0]].type)
			{
			case ControlData::TYPE_DIGITAL: if (amount > 1) amount = 1; break;
			case ControlData::TYPE_ANALOG: if (amount > 255) amount = 255; break;
			case ControlData::TYPE_THUMB: if (amount > 32767) amount = 32767; break;
			}

			// Add control to list to activate.
			for (int i = 0; i < 2; i++)
			{
				if (control[i] != CONTROL_INVALID)
				{
					// a-dannat taking out the -1 - amount and replacing it with a negative number for amount.
					//  I'm not sure what kind of two's complement was intended, but it can not being accounted for 
					//  during GetGamepadState where it converts the unsigned long to a signed short.
					//  This resulted in all thumbstick input being mapped to UP and/or RIGHT (postive values)
					m_sequence_event_controls[control[i]].Amount = (SHORT) amount;
					if ( direction[i] == -1 )
						m_sequence_event_controls[control[i]].Amount *= -1;
				}
			}

			return true;
		}


		// Try to parse a duration.
		// On return, current character is at the end of the recognizable subset.
		bool TextEventInput::parse_duration(unsigned long &frames, unsigned long &ms)
		{
			// duration : unsigned_long ( 's' | 'ms' | 'f' )?
			unsigned long duration;
			if (!parse_ulong(duration))
			{
				add_error_message("Expected duration.");
				return false;
			}
			switch (m_next_char)
			{
			// Seconds.
			case 's':
				get_next_char();
				ms = duration * 1000;
				frames = ms * m_fps / 1000;
				break;
			// Milliseconds
			case 'm':
				get_next_char();
				if (m_next_char != 's')
				{
					add_error_message("Expected 's' to follow 'm' in 'ms' unit.");
					return false;
				}
				get_next_char();
				ms = duration;
				frames = ms * m_fps / 1000;
				break;
			// Frames.
			case 'f':
				get_next_char();
				// Intentionally fall through to default case - assume Frames if omitted.
			default:
				frames = duration;
				ms = 1000 * frames / m_fps;
			}
			return true;
		}


		// Parse an unsigned long.
		// If it overflows, the results are unpredictable.
		bool TextEventInput::parse_ulong(unsigned long &number)
		{
			if (!(m_next_char >= '0' && m_next_char <= '9'))
			{
				add_error_message("Expected unsigned integer.");
				return false;
			}
			number = m_next_char - '0';
			while (get_next_char() && m_next_char >= '0' && m_next_char <= '9')
			{
				number *= 10;
				number += m_next_char - '0';
			}
			return true;
		}


		void TextEventInput::GetGamepadState(XINPUT_GAMEPAD &gamepad) const
		{
			//JKB
			if (m_type == EVENT_CONNECT_GAMEPAD)
			{
				// Clear gamepad state on initial connect.
				memset(&gamepad, 0, sizeof (gamepad));
			}
			else
			{
				for (Control c = CONTROL_FIRST; c < CONTROL_COUNT; ((int &) c)++)
				{
					// a-dannat Amount changed from a DWORD to a SHORT to presever sign. Amount could be positive or negative.
					if (m_sequence_event_controls[c].Amount != 0)
					{
						// Apply event data to gamepad state.
						// a-dannat Changing from a DWORD to a SHORT to preserve the sign during the conversion from DWORD to SHORT below
						SHORT amount = m_sequence_event_controls[c].Amount;
						bool release = m_sequence_event_type == RELEASE;
						switch (s_control_data[c].type)
						{
						case ControlData::TYPE_DIGITAL:
							if (!release)
							{
								gamepad.wButtons |= s_control_data[c].bit_mask;
							}
							else
							{
								gamepad.wButtons &= ~s_control_data[c].bit_mask;
							}
							break;
                        // MMILLS: MARCH, 2005 XDK UPDATE
						case ControlData::TYPE_ANALOG:
						    //gamepad.bAnalogButtons[s_control_data[c].analog_index] = (BYTE) (release ? 0 : amount);
                            gamepad.*(s_trigger_member_pointers[s_control_data[c].trigger_index]) = (BYTE) (release ? 0 : amount);
							break;
						case ControlData::TYPE_THUMB:
							// a-dannat  We loose the sign by taking the unsigned long (DWORD) Amount and converting to a (SHORT)
							//  negative amounts are stored in the DWORD as -1-Amount. JD/CD JDL/CDL and JL/CL did not work properly before.
							gamepad.*(s_thumb_member_pointers[s_control_data[c].thumb_index]) = (SHORT) (release ? 0 : amount);
							break;
						}
					}
				}
			}
		}

		// Keyboard state support:
		// a-dannat Adding keystroke support of gamepad buttons
		// Takes m_sequence_event_controls and m_sequence_event_type and puts them into an XINPUT_KEYSTROKE event structure
		void TextEventInput::GetKeyboardState(XINPUT_KEYSTROKE &keystroke) const
		{
			memset(&keystroke, 0, sizeof (keystroke));  // zero out the keystroke structure passed in

			for (Control c = CONTROL_FIRST; c < CONTROL_COUNT; ((int &) c)++)
			{
				// a-dannat Amount changed from a DWORD to a SHORT to preserve sign. Amount could be positive or negative.
				if ( m_sequence_event_controls[c].Amount != 0)
				{
					SHORT amount = m_sequence_event_controls[c].Amount;
					// Apply event data to keystroke state.
					switch ( s_control_keystroke[c] )
					{
						// Note that amount will never be 0 in here
						// This is messed up though if a thumbstick goes from positive to negative, the positive 'virtual key' would have to be released.. this doesn't do that.
					case VK_PAD_LTHUMB_UP:  
						// 6 possible outcomes
						if ( keystroke.VirtualKey == 0 )
							if ( amount > 0 )
								keystroke.VirtualKey = VK_PAD_LTHUMB_UP;
							else
								keystroke.VirtualKey = VK_PAD_LTHUMB_DOWN;
						else if ( keystroke.VirtualKey == VK_PAD_LTHUMB_LEFT )
							if ( amount > 0 )
								keystroke.VirtualKey = VK_PAD_LTHUMB_UPLEFT;
							else
								keystroke.VirtualKey = VK_PAD_LTHUMB_DOWNLEFT;
						else if ( keystroke.VirtualKey == VK_PAD_LTHUMB_RIGHT )
							if ( amount > 0 )
								keystroke.VirtualKey = VK_PAD_LTHUMB_UPRIGHT;
							else
								keystroke.VirtualKey = VK_PAD_LTHUMB_DOWNRIGHT;
						break;
					case VK_PAD_LTHUMB_LEFT:  
						// technically 6 possible outcomes here too, but since one of these values will be hit before the other,
						// only one of these cases needs all these if statements. But we don't know which that will be without hardcoding it.
						if ( keystroke.VirtualKey == 0 )
							if ( amount > 0 )
								keystroke.VirtualKey = VK_PAD_LTHUMB_RIGHT;
							else
								keystroke.VirtualKey = VK_PAD_LTHUMB_LEFT;
						else if ( keystroke.VirtualKey == VK_PAD_LTHUMB_UP )
							if ( amount > 0 )
								keystroke.VirtualKey = VK_PAD_LTHUMB_UPRIGHT;
							else
								keystroke.VirtualKey = VK_PAD_LTHUMB_UPLEFT;
						else if ( keystroke.VirtualKey == VK_PAD_LTHUMB_DOWN )
							if ( amount > 0 )
								keystroke.VirtualKey = VK_PAD_LTHUMB_DOWNRIGHT;
							else
								keystroke.VirtualKey = VK_PAD_LTHUMB_DOWNLEFT;
						break;
					case VK_PAD_RTHUMB_UP:
						if ( keystroke.VirtualKey == 0 )
							if ( amount > 0 )
								keystroke.VirtualKey = VK_PAD_RTHUMB_UP;
							else
								keystroke.VirtualKey = VK_PAD_RTHUMB_DOWN;
						else if ( keystroke.VirtualKey == VK_PAD_RTHUMB_LEFT )
							if ( amount > 0 )
								keystroke.VirtualKey = VK_PAD_RTHUMB_UPLEFT;
							else
								keystroke.VirtualKey = VK_PAD_RTHUMB_DOWNLEFT;
						else if ( keystroke.VirtualKey == VK_PAD_RTHUMB_RIGHT )
							if ( amount > 0 )
								keystroke.VirtualKey = VK_PAD_RTHUMB_UPRIGHT;
							else
								keystroke.VirtualKey = VK_PAD_RTHUMB_DOWNRIGHT;
						break;
					case VK_PAD_RTHUMB_LEFT:
						if ( keystroke.VirtualKey == 0 )
							if ( amount > 0 )
								keystroke.VirtualKey = VK_PAD_RTHUMB_RIGHT;
							else
								keystroke.VirtualKey = VK_PAD_RTHUMB_LEFT;
						else if ( keystroke.VirtualKey == VK_PAD_RTHUMB_UP )
							if ( amount > 0 )
								keystroke.VirtualKey = VK_PAD_RTHUMB_UPRIGHT;
							else
								keystroke.VirtualKey = VK_PAD_RTHUMB_UPLEFT;
						else if ( keystroke.VirtualKey == VK_PAD_RTHUMB_DOWN )
							if ( amount > 0 )
								keystroke.VirtualKey = VK_PAD_RTHUMB_DOWNRIGHT;
							else
								keystroke.VirtualKey = VK_PAD_RTHUMB_DOWNLEFT;
						break;
					default:
						keystroke.VirtualKey = s_control_keystroke[c];
						break;
					}
					switch ( m_sequence_event_type )
					{
					case PRESS:
					case HOLD:
						keystroke.Flags = XINPUT_KEYSTROKE_KEYDOWN;
						break;
						// HOLD events are when the button was be held down for a certain amount of time.
						// Is the amount of time a console setting?
						// This could eventually trigger a timer check to see if it is time to generate a key repeat.
					//case HOLD:
					//	keystroke.Flags = XINPUT_KEYSTROKE_REPEAT;
					//	break;
					case RELEASE:
						keystroke.Flags = XINPUT_KEYSTROKE_KEYUP;
						break;
					default:
						keystroke.Flags = 0;
						break;
					}
					keystroke.Unicode = 0;  // Since the Text Sequence player can only press gamepad buttons, this will always be 0 here.
					// We could break here since we found a keystroke
				}
			}
		}		



		// We assume that any time a text sequence is sent to a controller port, the user
		// wants it to execute. Without an explicit insert ("I"), however, the port may not
		// be connected. We force a connection by generating a gamepad connect event. This will
		// cause the ParseEvent handler to first pass along a connect event and then execute the
		// actual sequence the next time through. We must also be careful to retain any port index
		// that was specified earlier.
		bool TextEventInput::must_open_port()
		{
            bool combined_return_value = false;
            if (0 != m_combined_port)
            {
                for (UINT i=1; i<0xF; i<<=1)
                {
                    if (i == (m_combined_port & i))
                    {
                        if (true == must_open_port(i-1))
                        {
                            combined_return_value = true;
                        }
                    }
                }
            }
            else
            {
                combined_return_value = must_open_port(m_port);
            }
            return combined_return_value;
		}

        bool TextEventInput::must_open_port(DWORD port)
        {
            if (m_port_opened[port])
            {
                return false;
            }
            else
            {
                m_type = EVENT_CONNECT_GAMEPAD;
                m_capabilities = s_default_gamepad_capabilities;
                m_port_opened[port] = true;
                m_retain_previous_port = true;
                return true;
            }
        }

		// ================================================================
		// TextSequencePlayer
		//
		//	Supplies a sequence of input events.
		// ================================================================
		
		TextSequencePlayer::TextSequencePlayer(void) :
			Player(&m_text_event_sequence)
		{
			time_based_playback = true;
		}


		TextSequencePlayer::~TextSequencePlayer(void)
		{
		}

		// ----------------------------------------------------------------
		// IComponent interface
		// ----------------------------------------------------------------

		// Set a property for this component.
		XCR::Result TextSequencePlayer::SetStringProperty(LPCSTR name, LPCSTR value1, LPCSTR value2)
		{
			// Official name of property is data, but can also use file so startplayback will work too.
			if (lstrcmpiA(name, "data") == 0 || lstrcmpiA(name, "file") == 0)
			{
				if (value2 != NULL)
				{
					return XCR::Result::ERROR_TOO_MANY_ARGUMENTS;
				}
				m_text_event_sequence.SetData(value1);
				return XenonUtility::Result::SUCCESS;
			}
			else if (lstrcmpiA(name, "fps") == 0)
			{
				if (value2 != NULL)
				{
					return XCR::Result::ERROR_TOO_MANY_ARGUMENTS;
				}
				if (is_playing)
				{
					return XCR::Result::ERROR_CANT_SET_PROPERTY_WHILE_USING;
				}
				long fps = atol(value1);
				if (fps > 0)
				{
					m_text_event_sequence.SetFps(fps);
					return XCR::Result::SUCCESS;
				}
				return XCR::Result::ERROR_INVALID_VALUE;
			}
			return Player::SetStringProperty(name, value1, value2);
		}

        XCR::Result TextSequencePlayer::SetDefault()
        {
            m_text_event_sequence.SetData(0);
            m_text_event_sequence.SetFps(30);
            return Player::SetDefault();
        }
    }
}


#endif