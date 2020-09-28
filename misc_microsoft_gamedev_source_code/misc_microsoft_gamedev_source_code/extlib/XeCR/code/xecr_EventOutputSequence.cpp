//	EventOutputSequence.cpp : Event output sequence.
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

#include "xecr_EventOutputSequence.h"

// Enable debugging information
// #define REPORT(x) OutputDebugString(x)
// Disable debugging information.
#define REPORT(x)
#define CHECK_RESULT(a,b) if (!a) OutputDebugStringA(b)

namespace XCR
{
	namespace Sequence
	{
		BinaryEventOutput::BinaryEventOutput(OutputStream *output)
		{
			output_stream = output;
		}


		XCR::Result BinaryEventOutput::Start()
		{
			REPORT("Start Binary Events Out\n");
			XCR::Result result = output_stream->Start();
			if (result)
			{
				FileHeader header(true);
				result = output_stream->Write(&header, sizeof (header));
				CHECK_RESULT(result, "!!Failed to write header.\n");
				current_event_time_ms = 0;
				current_event_frame = 0;
				previous_event_time_ms = 0;
				previous_event_frame = 0;
			}
			if (!result)
			{
				output_stream->Stop();
			}
			return result;
		}


		XCR::Result BinaryEventOutput::Stop()
		{
			EndEvent entry;
			REPORT("Stop Binary Events Out\n");
			XCR::Result result = init_event_entry(entry);
			if (result) result = output_stream->Write(&entry, sizeof (entry));
			if (result) result = output_stream->Stop();
			else output_stream->Stop();
			return result ? XCR::Result::SUCCESS : result;
		}


		XCR::Result BinaryEventOutput::SetTime(DWORD time_ms, DWORD frame)
		{
			current_event_time_ms = time_ms;
			current_event_frame = frame;
			return XCR::Result::SUCCESS;
		}


		//JKB
		XCR::Result BinaryEventOutput::WriteConnectGamepadEvent(DWORD port, const XINPUT_CAPABILITIES &capabilities, const XINPUT_GAMEPAD &state)
		{
			GamepadConnectEvent entry;
			REPORT("Write Connect Gamepad Event\n");
			//JKB
			init_device_event_entry(entry, port);
			entry.capabilities = capabilities;
			entry.state = state;
			return output_stream->Write(&entry, sizeof(entry));
		}


		//JKB
		/*XCR::Result BinaryEventOutput::WriteDisconnectGamepadEvent(DWORD port)
		{
			GamepadDisconnectEvent entry;
			init_gamepad_event_entry(entry, port);
			return output_stream->Write(&entry, sizeof (entry));
		}*/

		//JKB: 
		XCR::Result BinaryEventOutput::WriteDisconnectAnyDeviceEvent(DWORD port)
		{
			AnyDeviceDisconnectEvent entry;
			REPORT("Write Disconnect Any Device Event\n");
			init_device_event_entry(entry, port);
			return output_stream->Write(&entry, sizeof(entry));
		}
		
		// RWBFIX: This will be affected by GAMEPAD change
		XCR::Result BinaryEventOutput::WriteGamepadState(DWORD port, const XINPUT_GAMEPAD &gamepad_state)
		{
			// Is it new data?  Figure out what the differences are.
			GamepadStateEvent entry;
			WORD diff_flags = 0;
			static long changeCount = 0;
			//char stringCount[64];
			DWORD bytes_changed = 0;
			{
				BYTE *old_data = (BYTE *) &previous_gamepad_state[port];
				BYTE *new_data = (BYTE *) &gamepad_state;
				BYTE *diff_data = (BYTE *) &entry.diff_data;
				DWORD i;
                //MMILLS: MARCH, 2005 XDK UPDATE
                // Digital buttons and analog buttons (1 word, 2 bytes = 4 bytes).
				for (i = 0; i < 4; i++, old_data++, new_data++)
				{
					if (*old_data != * new_data)
					{
						*diff_data = *new_data;
						diff_data++;
						diff_flags |= (1 << i);
					}
				}
				// Thumbsticks (4 shorts).
				for (i = 10; i < 14; i++, old_data += 2, new_data += 2)
				{
					if (*(SHORT *) old_data != *(SHORT *) new_data)
					{
						*(SHORT *) diff_data = *(SHORT *) new_data;
						diff_data += 2;
						diff_flags |= (1 << i);
					}
				}
				// Total number of bytes changed.
				bytes_changed = diff_data - (BYTE *) &entry.diff_data;
			}

			if (diff_flags != 0)
			{
				//sprintf(stringCount, "Gamepad State Change #%d\n", ++changeCount);
				//REPORT(stringCount);
				// Save for later.
				previous_gamepad_state[port] = gamepad_state;
				// Write new packet data to disk.
				WaitEvent wait;
				//JKB
				init_device_event_entry(entry, port);
				entry.mask = diff_flags;
				return output_stream->Write(&entry, sizeof (entry) - sizeof (entry.diff_data) + bytes_changed);
			}

			return XCR::Result::SUCCESS;
		}

		// Keyboard connect support:
		XCR::Result BinaryEventOutput::WriteConnectKeyboardEvent(DWORD port, const XINPUT_KEYSTROKE &keyboard_state)
		{
			KeyboardConnectEvent entry;
			REPORT("Write Keyboard Connect Event\n");
			init_device_event_entry(entry, port);
			entry.keystroke = keyboard_state;
			return output_stream->Write(&entry, sizeof(entry));
		}
			
		//JKB: debug keyboard disconnect support:
		/*XCR::Result BinaryEventOutput::WriteDisconnectDebugKeyboardEvent(DWORD port)
		{
			KeyboardDisconnectEvent entry;
			init_device_event_entry(entry, port);
			return output_stream->Write(&entry, sizeof (entry));
		}*/

		// Save Keystroke data
		XCR::Result BinaryEventOutput::WriteKeyboardState(DWORD port, const XINPUT_KEYSTROKE &keyboard_state)
		{
			// Unlike gamepad states, keystrokes are event generated, so we can assume there
			// is new data each time. We will save seven bytes of data for each keystroke.
			KeyboardStateEvent entry;
			WORD diff_flags = 0;
			DWORD bytes_changed = 0;

			// Write all 7 bytes and don't compare to previous
			BYTE *new_data = (BYTE *) &keyboard_state;
			BYTE *diff_data = (BYTE *) &entry.diff_data;
			// Virtualkey, unicode, flags: 7 bytes.
			for (DWORD i = 0; i < 7; i++, new_data++)
			{
				*diff_data = *new_data;
				diff_data++;
				diff_flags |= (1 << i);
			}

			// Total number of bytes changed (always 7)
			bytes_changed = diff_data - (BYTE *) &entry.diff_data;

			if (diff_flags != 0)
			{
				// Write packet data to disk.
				WaitEvent wait;
				init_device_event_entry(entry, port);
				entry.mask = diff_flags;
				return output_stream->Write(&entry, sizeof (entry) - sizeof (entry.diff_data) + bytes_changed);
			}

			return XCR::Result::SUCCESS;
		}
		
		// Initialize the event entry.  Sets the current frame and time.
		// Returns true if a wait entry needs to be inserted.
		inline XCR::Result BinaryEventOutput::init_event_entry(InputEvent &event)
		{
			DWORD frame_delta = current_event_frame - previous_event_frame;
			DWORD time_delta = current_event_time_ms - previous_event_time_ms;
			previous_event_frame = current_event_frame;
			previous_event_time_ms = current_event_time_ms;
			event.frame_count_delta = (WORD) (frame_delta < 0x10000 ? frame_delta : 0);
			event.tick_count_delta = (WORD) (time_delta < 0x10000 ? time_delta : 0);
			if (frame_delta > 0xFFFF || time_delta > 0xFFFF)
			{
				WaitEvent wait;
				wait.frame_count_delta = frame_delta;
				wait.tick_count_delta = time_delta;
				return output_stream->Write(&wait, sizeof (wait));
			}
			return XCR::Result::SUCCESS;
		}


		// Initialize the event entry.  Sets the current frame, time, and port.
		// Returns true if a wait entry needs to be inserted.
		//JKB
		inline XCR::Result BinaryEventOutput::init_device_event_entry(DeviceInputEvent &event, DWORD port)
		{
			event.port = (BYTE) port;
			return init_event_entry(event);
		}

		//JKBNEW:
		XCR::Result BinaryEventOutput::SaveState(DirectDiskOutputStream* ddos)
		{
			//???
			// ddos->Write((LPVOID)this, (DWORD)sizeof(this));

			// Write gamepad state:
			for (int i = 0; i < INPUT_DEVICE_PORTS; i++)
			{
				if (!ddos->Write((LPVOID)&previous_gamepad_state[i],(DWORD)sizeof(XINPUT_GAMEPAD)))
					return XCR::Result::ERROR;
			}

			// Write keyboard state: No - not necessary
			//for (int i = 0; i < INPUT_DEVICE_PORTS; i++)
			//{
			//	if (!ddos->Write((LPVOID)&previous_keyboard_state[i],(DWORD)sizeof(XINPUT_KEYSTROKE)))
			//		return XCR::Result::ERROR;
			//}

			// Write timing info:
			if (!ddos->Write((LPVOID)&current_event_time_ms,(DWORD)sizeof(DWORD)))
				return XCR::Result::ERROR;
			if (!ddos->Write((LPVOID)&current_event_frame,(DWORD)sizeof(DWORD)))
				return XCR::Result::ERROR;
			if (!ddos->Write((LPVOID)&previous_event_time_ms,(DWORD)sizeof(DWORD)))
				return XCR::Result::ERROR;
			if (!ddos->Write((LPVOID)&previous_event_frame,(DWORD)sizeof(DWORD)))
				return XCR::Result::ERROR;
			
			//----------------------------------
			// output_stream serialization work:
			//----------------------------------
			return output_stream->SaveState(ddos);

			//bool isRec = false;
			//if (output_stream->Stop() != Result::ERROR_OPERATION_NOT_IN_PROGRESS)
			//{
			//	// Write recording bool:
			//	isRec = true;
			//	if (!ddos->Write((LPVOID)&isRec,(DWORD)sizeof(bool)))
			//		return XCR::Result::ERROR;

			//	// Get filename:
			//	string fn = output_stream->GetFilename();
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
			//	if (!ddos->Write((LPVOID)&isRec,(DWORD)sizeof(bool)))
			//		return XCR::Result::ERROR;
			//}

			//return XCR::Result::SUCCESS;
		}

		//JKBNEW:
		XCR::Result BinaryEventOutput::ResumeState(DirectDiskInputStream* ddis)
		{
			// Read gamepad state:
			for (int i = 0; i < INPUT_DEVICE_PORTS; i++)
			{
				if (!ddis->Read((LPVOID)&previous_gamepad_state[i],(DWORD)sizeof(XINPUT_GAMEPAD)))
					return XCR::Result::ERROR;
			}

			//for (int i = 0; i < INPUT_DEVICE_PORTS; i++)
			//{
			//	if (!ddis->Read((LPVOID)&previous_keyboard_state[i],(DWORD)sizeof(XINPUT_KEYSTROKE)))
			//		return XCR::Result::ERROR;
			//}

			// Read timing info:
			if (!ddis->Read((LPVOID)&current_event_time_ms,(DWORD)sizeof(DWORD)))
				return XCR::Result::ERROR;
			if (!ddis->Read((LPVOID)&current_event_frame,(DWORD)sizeof(DWORD)))
				return XCR::Result::ERROR;
			if (!ddis->Read((LPVOID)&previous_event_time_ms,(DWORD)sizeof(DWORD)))
				return XCR::Result::ERROR;
			if (!ddis->Read((LPVOID)&previous_event_frame,(DWORD)sizeof(DWORD)))
				return XCR::Result::ERROR;
			
			//----------------------------------
			// output_stream serialization work:
			//----------------------------------
			return output_stream->ResumeState(ddis);

			//bool isRec = false;
			//if (!ddis->Read((LPVOID)&isRec,(DWORD)sizeof(bool)))
			//	return XCR::Result::ERROR;

			//if (isRec)
			//{
			//	string fn = "";

			//	// Get filename:
			//	int fnLen = 0;
			//	if (!ddis->Read((LPVOID)&fnLen,(DWORD)sizeof(int)))
			//		return XCR::Result::ERROR;

			//	// Now read filename && attempt to resume recording:
			//	if (fnLen > 0)
			//	{
			//		if (!ddis->Read((LPVOID)&fn,(DWORD)fnLen))// + 1))
			//			return XCR::Result::ERROR;

			//		// assign filename:
			//		output_stream->SetFilename(fn.c_str());

			//		return output_stream->Continue();
			//	}
			//}
			//
			//return XCR::Result::SUCCESS;
		}

        // MJMXAM: added to support toggle hud recording
        XCR::Result BinaryEventOutput::WriteToggleHudEvent(DWORD port)
        {
            DeviceInputEvent entry(InputEvent::TYPE_INPUT_TOGGLE_HUD);
            init_device_event_entry(entry, port);
            return output_stream->Write(&entry, sizeof(entry));
        }

	}
}


#endif