//	EventInputSequence.cpp : Event input sequence.
//
//	Created 2004/10/12 Rich Bonny    <rbonny@microsoft.com>
//
//  - based on original XCR code:
//	Created 2003/08/XX David Eichorn <deichorn@microsoft.com>
//     and modified by Jon Burns     <jburns@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2004 Microsoft Corp.  All rights reserved.




#ifdef _XCR_

#include "xecr_EventInputSequence.h"

// Enable debugging information
// #define REPORT(x) OutputDebugString(x)
// Disable debugging information.
#define REPORT(x)
#define CHECK_RESULT(a,b) if (!a) OutputDebugStringA(b)



namespace XCR
{
	namespace Sequence
	{
		// Constructor.
		BinaryEventInput::BinaryEventInput(InputStream *input)
		{
			m_input = input;
		}


		// Start the binary event input sequence.
		XCR::Result BinaryEventInput::Start()
		{
			FileHeader header;
			XCR::Result result = m_input->Start();
			if (!result)
			{
				return result;
			}
			result = m_input->Read(&header, sizeof (header));
			if (!result)
			{
				m_input->Stop();
				return result;
			}
			if (!header.IsValid())
			{
				m_input->Stop();
				return XCR::Result::ERROR_INVALID_FILE;
			}
			m_time = 0;
			m_frame = 0;
			m_type = EVENT_END;
			result = get_next_event();
			return result;
		}


		// Stop.
		XCR::Result BinaryEventInput::Stop()
		{
			return m_input->Stop();
		}


		// Advance event.
		XCR::Result BinaryEventInput::AdvanceToNextEvent()
		{
			return get_next_event();
		}


		// Applies data changes to gamepad.
		// RWBFIX: This will be affected by GAMEPAD redefinition.
		void BinaryEventInput::GetGamepadState(XINPUT_GAMEPAD &gamepad) const
		{
			//JKB:
			_ASSERTE(m_type == EVENT_GAMEPAD_STATE || m_type == EVENT_CONNECT_GAMEPAD);
			BYTE *new_data = (BYTE *) &gamepad;
			BYTE *diff_data = (BYTE *) m_gamepad_data;
			DWORD i;
            //MMILLS: MARCH, 2005 XDK UPDATE
            // Digital buttons and analog buttons (1 word, 2 bytes = 4 bytes).
			for (i = 0; i < 4; i++, new_data++)
			{
				if (m_gamepad_data_flags & (1 << i))
				{
					*new_data = *diff_data;
					diff_data++;
				}
			}
			// Thumbsticks (4 shorts).
			for (i = 10; i < 14; i++, new_data += 2)
			{
				if (m_gamepad_data_flags & (1 << i))
				{
					*(SHORT *) new_data = *(SHORT *) diff_data;
					diff_data += 2;
				}
			}
		}

		// Keyboard state support:
		void BinaryEventInput::GetKeyboardState(XINPUT_KEYSTROKE &keystroke) const
		{
			_ASSERTE(m_type == EVENT_KEYBOARD_STATE || m_type == EVENT_CONNECT_KEYBOARD);
			BYTE *new_data = (BYTE *) &keystroke;
			BYTE *diff_data = (BYTE *) m_keyboard_data;
			// Virtual key, unicode, flags: 7 bytes.
			for (DWORD i = 0; i < 7; i++, new_data++)
			{
				if (m_keyboard_data_flags & (1 << i))
				{
					*new_data = *diff_data;
					diff_data++;
				}
			}
		}

		// Retrieve next event from disk.
		XCR::Result BinaryEventInput::get_next_event()
		{
			InputEvent event;
			InputEvent::Type type;
			XCR::Result result;
			do
			{
				result = m_input->Read(&event, sizeof (event));
				if (!result)
				{
					return result;
				}
				type = (InputEvent::Type) event.type;
				switch (type)
				{
				case InputEvent::TYPE_EOF:
					{
						REPORT("Get event end\n");
						m_type = EVENT_END;
					}
					break;

				// RWBFIX: Verify that this still works after GAMEPAD changes
				case InputEvent::TYPE_INPUT_CONNECT_GAMEPAD:
					{
						GamepadConnectEvent entry;
						REPORT("Get event connect\n");
						result = m_input->Read((char *) &entry + sizeof (event), sizeof (entry) - sizeof (event));
						CHECK_RESULT(result, "!!Failed to read gamepad connect event.\n");
						m_port = entry.port;
						m_capabilities = entry.capabilities;
						m_gamepad_data_flags = 0x3FFF;
						memcpy(m_gamepad_data, &entry.state, sizeof (entry.state));
						m_type = EVENT_CONNECT_GAMEPAD;
					}
					break;

				// Disconnect any device support:
				case InputEvent::TYPE_INPUT_DISCONNECT_ANY_DEVICE:
					{
						AnyDeviceDisconnectEvent entry;
						REPORT("Get event disconnect any device\n");
						result = m_input->Read((char *) &entry + sizeof (event), sizeof (entry) - sizeof (event));
						CHECK_RESULT(result, "!!Failed to read device disconnect event.\n");
						m_port = entry.port;
						m_type = EVENT_DISCONNECT_ANY_DEVICE;
					}
					break;

				case InputEvent::TYPE_INPUT_GAMEPAD_STATE:
					{
						GamepadStateEvent entry;
						REPORT("Get event gamepad\n");
						// Read the mask (everything except the diff_data).
						result = m_input->Read((char *) &entry + sizeof (event), sizeof (entry) - sizeof (event) - sizeof (entry.diff_data));
						CHECK_RESULT(result, "!!Failed to read gamepad state.\n");
						m_gamepad_data_flags = entry.mask;
						// Count how many bytes changed and read them.
						DWORD bytes_changed = 0;
						DWORD i;
                        //MMILLS: MARCH, 2005 XDK UPDATE
						for (i = 0; i < 4; i++)
						{
							if (entry.mask & (1 << i))
							{
								bytes_changed++;
							}
						}
						for (i = 10; i < 14; i++)
						{
							if (entry.mask & (1 << i))
							{
								bytes_changed += 2;
							}
						}
						result = m_input->Read(m_gamepad_data, bytes_changed);
						CHECK_RESULT(result, "!!Failed to read gamepad data.\n");
						m_port = entry.port;
						m_type = EVENT_GAMEPAD_STATE;
					}
					break;

				case InputEvent::TYPE_INPUT_CONNECT_KEYBOARD:
					{
						KeyboardConnectEvent entry;
						REPORT("Get event connect debug keyboard\n");
						result = m_input->Read((char *) &entry + sizeof (event), sizeof (entry) - sizeof (event));
						CHECK_RESULT(result, "!!Failed to read keyboard connect event.\n");
						m_port = entry.port;
						m_keyboard_data_flags = 0x3FFF;
						m_type = EVENT_CONNECT_KEYBOARD;
					}
					break;

				//JKB: debug keyboard input state support:
				case InputEvent::TYPE_INPUT_KEYBOARD_STATE:
					{
						KeyboardStateEvent entry;
						REPORT("Get event debug keystroke\n");
						// Read the mask (everything except the diff_data).
						result = m_input->Read((char *) &entry + sizeof (event), sizeof (entry) - sizeof (event) - sizeof (entry.diff_data));
						CHECK_RESULT(result, "!!Failed to read keyboard state.\n");
						m_keyboard_data_flags = entry.mask;
						// Always writes full 7 bytes
						DWORD bytes_changed = 7;
						result = m_input->Read(m_keyboard_data, bytes_changed);
						CHECK_RESULT(result, "!!Failed to read keyboard data.\n");
						m_port = entry.port;
						m_type = EVENT_KEYBOARD_STATE;
					}
					break;

				case InputEvent::TYPE_WAIT:
					{
						WaitEvent wait;
						REPORT("Get event wait\n");
						// Copy what we read and read the rest of the wait entry.
						_ASSERTE(sizeof (wait) > sizeof (event));
						memcpy(&wait, &event, sizeof (event));
						result = m_input->Read((char *) &wait + sizeof (event), sizeof (wait) - sizeof (event));
						CHECK_RESULT(result, "!!Failed to read wait event.\n");
						m_frame += wait.frame_count_delta;
						m_time += wait.tick_count_delta;
						m_type = EVENT_DISCONNECT_ANY_DEVICE;
					}
					break;

                // MJMXAM: added to support toggle hud recording
                case InputEvent::TYPE_INPUT_TOGGLE_HUD:
                    {
                        DeviceInputEvent entry(InputEvent::TYPE_INPUT_TOGGLE_HUD);
                        REPORT("Get event toggle hud\n");
                        memcpy(&entry, &event, sizeof (event));
                        result = m_input->Read((char *) &entry + sizeof (event), sizeof (entry) - sizeof (event));
                        CHECK_RESULT(result, "!!Failed to read toggle hud event.\n");
                        m_port = entry.port;
                        m_type = EVENT_TOGGLE_HUD;
                        break;
                    }

				default:
					result = XCR::Result::ERROR_INVALID_FILE;
				}
			}
			while (result && type == InputEvent::TYPE_WAIT);
			m_frame += event.frame_count_delta;
			m_time += event.tick_count_delta;
			return result ? XCR::Result::SUCCESS : result;
		}

		XCR::Result BinaryEventInput::SaveState(DirectDiskOutputStream* ddos)
		{
			//???
			// ddos->Write((LPVOID)this, (DWORD)sizeof(this));

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
			
			// Write capabilities info:
			if (!ddos->Write((LPVOID)&m_capabilities, (DWORD)sizeof(XINPUT_CAPABILITIES)))
				return XCR::Result::ERROR;

			// Write gamepad state info:
			if (!ddos->Write((LPVOID)&m_gamepad_data, (DWORD)sizeof(XINPUT_GAMEPAD)))
				return XCR::Result::ERROR;
			
			// Write gamepad flags member info:
			if (!ddos->Write((LPVOID)&m_gamepad_data_flags,(DWORD)sizeof(DWORD)))
				return XCR::Result::ERROR;

			// Write kbd state info:
			if (!ddos->Write((LPVOID)&m_keyboard_data, (DWORD)sizeof(XINPUT_KEYSTROKE)))
				return XCR::Result::ERROR;
			
			// Write kbd flags member info:
			if (!ddos->Write((LPVOID)&m_keyboard_data_flags,(DWORD)sizeof(DWORD)))
				return XCR::Result::ERROR;
			
			//----------------------------------
			// input_stream serialization work:
			//----------------------------------
			return m_input->SaveState(ddos);
			//bool isPlaying = false;
			//if (m_input->Stop() != Result::ERROR_OPERATION_NOT_IN_PROGRESS)
			//{
			//	// Write playing bool:
			//	isPlaying = true;
			//	if (!ddos->Write((LPVOID)&isPlaying,(DWORD)sizeof(bool)))
			//		return XCR::Result::ERROR;

			//	// Get filename:
			//	string fn = m_input->GetFilename();
			//	
			//	// Write length of filename:
			//	int fnLen = sizeof(fn);//fn.length();
			//	if (!ddos->Write((LPVOID)&fnLen,(DWORD)sizeof(int)))
			//		return XCR::Result::ERROR;

			//	// Now write filename:
			//	if (fnLen > 0)
			//	{
			//		if (!ddos->Write((LPVOID)&fn,(DWORD)fnLen))//fnLen + 1))
			//			return XCR::Result::ERROR;
			//	}
			//}
			//else
			//{
			//	// Write recording bool:
			//	if (!ddos->Write((LPVOID)&isPlaying,(DWORD)sizeof(bool)))
			//		return XCR::Result::ERROR;
			//}

			//return XCR::Result::SUCCESS;
		}

		//JKBNEW:
		XCR::Result BinaryEventInput::ResumeState(DirectDiskInputStream* ddis)
		{
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
			
			// Read capabilities info:
			if (!ddis->Read((LPVOID)&m_capabilities, (DWORD)sizeof(XINPUT_CAPABILITIES)))
				return XCR::Result::ERROR;

			// Read gamepad state info:
			if (!ddis->Read((LPVOID)&m_gamepad_data, (DWORD)sizeof(XINPUT_GAMEPAD)))
				return XCR::Result::ERROR;
			
			// Read gamepad flags member info:
			if (!ddis->Read((LPVOID)&m_gamepad_data_flags,(DWORD)sizeof(DWORD)))
				return XCR::Result::ERROR;

			// Read kbd state info:
			if (!ddis->Read((LPVOID)&m_keyboard_data, (DWORD)sizeof(XINPUT_KEYSTROKE)))
				return XCR::Result::ERROR;
			
			// Read kbd flags member info:
			if (!ddis->Read((LPVOID)&m_keyboard_data_flags,(DWORD)sizeof(DWORD)))
				return XCR::Result::ERROR;
			
			//----------------------------------
			// input_stream serialization work:
			//----------------------------------
			return m_input->ResumeState(ddis);

			//bool isPlaying = false;
			//// Read playing bool:
			//if (!ddis->Read((LPVOID)&isPlaying,(DWORD)sizeof(bool)))
			//	return XCR::Result::ERROR;

			//if (isPlaying)
			//{
			//	// Get filename:
			//	string fn = "";
			//	
			//	// Read length of filename:
			//	int fnLen = 0;
			//	if (!ddis->Read((LPVOID)&fnLen,(DWORD)sizeof(int)))
			//		return XCR::Result::ERROR;

			//	if (fnLen > 0)
			//	{
			//		// Now Read filename:
			//		if (!ddis->Read((LPVOID)&fn,(DWORD)fnLen))
			//			return XCR::Result::ERROR;

			//		m_input->SetFilename(fn.c_str());

			//		// continue with playback:
			//		return m_input->Continue();
			//	}
			//}

			//return XCR::Result::SUCCESS;
		}

	}
}


#endif