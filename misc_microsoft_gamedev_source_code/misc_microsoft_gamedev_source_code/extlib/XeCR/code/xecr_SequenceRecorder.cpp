//	SequenceRecorder.cpp : Records an event sequence.
//
//	Created 2004/10/12 Rich Bonny    <rbonny@microsoft.com>
//
//  - based on original XCR code:
//	Created 2003/09/29 David Eichorn <deichorn@microsoft.com>
//     and modified by Jon Burns     <jburns@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2004 Microsoft Corp.  All rights reserved.




#ifdef _XCR_

#include "xecr_SequenceRecorder.h"




namespace XCR
{
	namespace Sequence
	{
		using namespace ControlUnit;

		// ================================================================
		//	Recorder
		//
		//	Base class that records an EventOutput sequence.
		// ================================================================

		Recorder::Recorder(IEventOutput *event_sequence) :
			core(NULL),
			event_sequence(event_sequence),
			is_recording(false),
			//JKBNEW: adjuster on resume state after reboot:
			m_tickCountAdjuster(0x0)
		{
		}


		Recorder::~Recorder()
		{
			if (is_recording)
			{
				event_sequence->Stop();
			}
		}


		// ----------------------------------------------------------------
		// IComponent interface
		// ----------------------------------------------------------------


		XCR::Result Recorder::SelectComponent()
		{
			if (is_recording)
			{
				XCR::Result result = update_all_ports();
				return result ? XCR::Result::SUCCESS : result;
			}
			return XCR::Result::SUCCESS;
		}


		XCR::Result Recorder::SetStringProperty(LPCSTR name, LPCSTR value1, LPCSTR value2)
		{
			return IRecorder::SetStringProperty(name, value1, value2);
		}


		XCR::Result Recorder::Start()
		{
			if (is_recording) return XCR::Result::ERROR_ALREADY_RECORDING;

			is_recording = true;
			absolute_start_time_ms = GetTickCount();
			relative_current_time_ms = 0;
			relative_current_frame = 0;
			
			XCR::Result result = event_sequence->Start();
			if (result) result = update_all_ports();
			if (!result)
			{
				is_recording = false;
				core->LogMessage("XCR: ");
				if (result == XCR::Result::ERROR_CUSTOM)
				{
					core->LogMessage(event_sequence->GetErrorMessage().c_str());
				}
				else
				{
					const char *result_message;
					result.GetMessage(result_message);
					core->LogMessage(result_message);
				}
				core->LogMessage("\r\n");
				return result;
			}

			return XCR::Result::SUCCESS_RECORDING_STARTED;
		}


		XCR::Result Recorder::Stop()
		{
			if (!is_recording) return XCR::Result::ERROR_NOT_RECORDING;
			XCR::Result result = update_time();
			if (result) result = event_sequence->Stop();
			is_recording = false;
			return result ? XCR::Result::SUCCESS_RECORDING_STOPPED : result;
		}


		XCR::Result Recorder::GetStatus()
		{
			return is_recording ? XCR::Result::STATUS_RECORDING : XCR::Result::STATUS_IDLE;
		}


		//JKBNEW:
		XCR::Result Recorder::SaveState(DirectDiskOutputStream* ddos)
		{
			//???
			// ddos->Write((LPVOID)this, (DWORD)sizeof(this));

			// Write recording bool:
			if (!ddos->Write((LPVOID)&is_recording,(DWORD)sizeof(bool)))
				return XCR::Result::ERROR;
			
			// Write gamepad data array: (NECESSARY?)
			for (int i = 0; i < INPUT_DEVICE_PORTS; i++)
			{
				if (!ddos->Write((LPVOID)&gamepad_data[i],(DWORD)sizeof(GamepadData)))
					return XCR::Result::ERROR;
			}

			// Write dbg kbd data array: (NECESSARY?)
			for (int i = 0; i < INPUT_DEVICE_PORTS; i++)
			{
				if (!ddos->Write((LPVOID)&keyboard_data[i],(DWORD)sizeof(KeyboardData)))
					return XCR::Result::ERROR;
			}

			// Write timing info:
			if (!ddos->Write((LPVOID)&absolute_start_time_ms,(DWORD)sizeof(DWORD)))
				return XCR::Result::ERROR;
			if (!ddos->Write((LPVOID)&relative_current_time_ms,(DWORD)sizeof(DWORD)))
				return XCR::Result::ERROR;
			if (!ddos->Write((LPVOID)&relative_current_frame,(DWORD)sizeof(DWORD)))
				return XCR::Result::ERROR;
			DWORD curTickCnt = GetTickCount();
			if (!ddos->Write((LPVOID)&curTickCnt,(DWORD)sizeof(DWORD)))
				return XCR::Result::ERROR;

			// Write event_sequence data:
			return event_sequence->SaveState(ddos);
		}

		//JKBNEW:
		XCR::Result Recorder::ResumeState(DirectDiskInputStream* ddis)
		{
			// read recording bool:
			if (!ddis->Read((LPVOID)&is_recording,(DWORD)sizeof(bool)))
				return XCR::Result::ERROR;

			// read gamepad data array: (NECESSARY?)
			for (int i = 0; i < INPUT_DEVICE_PORTS; i++)
			{
				if (!ddis->Read((LPVOID)&gamepad_data[i],(DWORD)sizeof(GamepadData)))
					return XCR::Result::ERROR;
			}

			// read kbd data array: (NECESSARY?)
			for (int i = 0; i < INPUT_DEVICE_PORTS; i++)
			{
				if (!ddis->Read((LPVOID)&keyboard_data[i],(DWORD)sizeof(KeyboardData)))
					return XCR::Result::ERROR;
			}

			// read timing info:
			if (!ddis->Read((LPVOID)&absolute_start_time_ms,(DWORD)sizeof(DWORD)))
				return XCR::Result::ERROR;
			if (!ddis->Read((LPVOID)&relative_current_time_ms,(DWORD)sizeof(DWORD)))
				return XCR::Result::ERROR;
			if (!ddis->Read((LPVOID)&relative_current_frame,(DWORD)sizeof(DWORD)))
				return XCR::Result::ERROR;
			DWORD prevTickCnt = 0x0;
			if (!ddis->Read((LPVOID)&prevTickCnt,(DWORD)sizeof(DWORD)))
				return XCR::Result::ERROR;

			// Set tick count adjuster value:
			DWORD curTickCnt = GetTickCount();
			if (prevTickCnt > 0 && prevTickCnt > curTickCnt)
				m_tickCountAdjuster = prevTickCnt - curTickCnt;

			// Write event_sequence data:
			XCR::Result res = event_sequence->ResumeState(ddis);

			// DON'T NEED TO, EVERYTHING IN Start() we should have handled: 
			//finally, start recording if we were:
			//if (res == Result::SUCCESS && is_recording)
			//	return Start();

			return res;
		}




		// ----------------------------------------------------------------
		// IRecorder interface
		// ----------------------------------------------------------------

		XCR::Result Recorder::Initialize(IRecorderInterface *core_interface)
		{
			core = core_interface;
			return XCR::Result::SUCCESS;
		}

		//JKB:
		//XCR::Result Recorder::RecordGamepadDisconnect(DWORD port)
		//{
		//	if (!is_recording) return XCR::Result::ERROR_NOT_RECORDING;
		//	update_time();
		//	//JKB:
		//	return event_sequence->WriteDisconnectGamepadEvent(port);
		//}

		//JKB: general disconnect support:
		XCR::Result Recorder::RecordDeviceDisconnect(DWORD port)
		{
			if (!is_recording) return XCR::Result::ERROR_NOT_RECORDING;
			update_time();
			return event_sequence->WriteDisconnectAnyDeviceEvent(port);
		}

        // MJMXAM: add record toggle hud feature
        XCR::Result Recorder::RecordToggleHud(DWORD port)
        {
            if (!is_recording) return XCR::Result::ERROR_NOT_RECORDING;
            update_time();
            return event_sequence->WriteToggleHudEvent(port);
        }

        //JKB:
		XCR::Result Recorder::RecordGamepadConnect(DWORD port, const XINPUT_CAPABILITIES &capabilities, const XINPUT_GAMEPAD &state)
		{
			if (!is_recording) return XCR::Result::ERROR_NOT_RECORDING;
			update_time();
			//JKB
			return event_sequence->WriteConnectGamepadEvent(port, capabilities, state);
		}

		XCR::Result Recorder::RecordInputGetState(DWORD port, const XINPUT_GAMEPAD &gamepad_state)
		{
			if (!is_recording) return XCR::Result::ERROR_NOT_RECORDING;
			update_time();
			return event_sequence->WriteGamepadState(port, gamepad_state);
		}

		//JKB: debug keyboard disconnect support:
		/*XCR::Result Recorder::RecordKeyboardDisconnect(DWORD port)
		{
			if (!is_recording) return XCR::Result::ERROR_NOT_RECORDING;
			update_time();
			return event_sequence->WriteDisconnectKeyboardEvent(port);
		}*/

		//JKB: debug keyboard connect support:
		XCR::Result Recorder::RecordKeyboardConnect(DWORD port, const XINPUT_KEYSTROKE &keyboard_state)
		{
			if (!is_recording) return XCR::Result::ERROR_NOT_RECORDING;
			update_time();
			return event_sequence->WriteConnectKeyboardEvent(port, keyboard_state);
		}

		//JKB: debug keyboard detection/handling:
		XCR::Result Recorder::RecordInputGetKeystroke(DWORD port, const PXINPUT_KEYSTROKE pKeystroke)
		{
			if (!is_recording) return XCR::Result::ERROR_NOT_RECORDING;
			update_time();
			return event_sequence->WriteKeyboardState(port, *pKeystroke);
		}
	
		XCR::Result Recorder::FrameAdvance()
		{
			relative_current_frame++;
			return XCR::Result::SUCCESS;
		}


		XCR::Result Recorder::update_all_ports()
		{
			XCR::Result result = XCR::Result::SUCCESS;
			// Update recorder to match current core state.
			for (int port = 0; port < INPUT_DEVICE_PORTS; port++)
			{
				if (core->GetDeviceType(port) == XINPUT_DEVTYPE_GAMEPAD)
				{
					if (core->IsConnected(port))
					{
						if (result) result = event_sequence->WriteConnectGamepadEvent(port, core->GetCapabilities(port), core->GetGamepadState(port));
					}
					else
					{
						if (result) result = event_sequence->WriteDisconnectAnyDeviceEvent(port);
					}
				}
				else if (core->GetDeviceType(port) == XINPUT_DEVTYPE_USB_KEYBOARD)
				{
					if (core->IsConnected(port))
					{
						XINPUT_KEYSTROKE empty_keyboard_state;
						memset((void*)&empty_keyboard_state,0,sizeof(empty_keyboard_state));
						if (result) result = event_sequence->WriteConnectKeyboardEvent(port, empty_keyboard_state);
					}
					else
					{
						if (result) result = event_sequence->WriteDisconnectAnyDeviceEvent(port);
					}
				}
				else // no devices at all plugged in: 
				{
					if (result) result = event_sequence->WriteDisconnectAnyDeviceEvent(port);
				}
			}
			return result;
		}


		XCR::Result Recorder::update_time()
		{
			//JKBNEW: on resume, need to keep tick count structure same as previous counts:
			//relative_current_time_ms = GetTickCount() - absolute_start_time_ms;
			relative_current_time_ms = GetTickCount() + m_tickCountAdjuster - absolute_start_time_ms;
			return event_sequence->SetTime(relative_current_time_ms, relative_current_frame);
		}

	}
}


#endif