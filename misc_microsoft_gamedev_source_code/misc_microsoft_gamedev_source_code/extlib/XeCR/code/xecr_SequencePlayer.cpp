//	SequencePlayer.cpp : Plays event input sequences.
//
//	Created 2004/10/12 Rich Bonny    <rbonny@microsoft.com>
//
//  - based on original XCR code:
//	Created 2003/06/04 David Eichorn <deichorn@microsoft.com>
//     and modified by Jon Burns     <jburns@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2004 Microsoft Corp.  All rights reserved.




#ifdef _XCR_

#include "xecr_SequencePlayer.h"
using namespace XCR;

// Enable debugging information
// #define REPORT(x) OutputDebugString(x)
// Disable debugging information.
#define REPORT(x)




namespace XCR
{
	namespace Sequence
	{

		// Construct a sequence player using the provided IEventInputSequence.
		// Default is for time-based playback.
		Player::Player(IEventInput *event_sequence) :
			core(NULL),
			event_sequence(event_sequence),
			is_playing(false),
			time_based_playback(true),
			//JKBNEW: adjuster on resume state after reboot:
			m_tickCountAdjuster(0x0)

		{
			// Set our copy of emulated ports to disconnected.
			for (DWORD port = 0; port < INPUT_DEVICE_PORTS; port++)
			{
				gamepad_data[port].is_connected = false;
				memset(&gamepad_data[port].gamepad_state, 0, sizeof (gamepad_data[port].gamepad_state));

				// Yeah, do same for keyboard here.
				keyboard_data[port].is_connected = false;
				memset(&keyboard_data[port].keystroke,0,sizeof(keyboard_data[port].keystroke));
			}
		}


		// Destructor.
		// This will probably never be called.
		Player::~Player(void)
		{
			if (is_playing)
			{
				event_sequence->Stop();
			}
		}




		// ----------------------------------------------------------------
		// IComponent interface
		// ----------------------------------------------------------------

		// Notify this component it is being selected.
		XCR::Result Player::SelectComponent()
		{
			if (is_playing)
			{
				update_connected_devices();
			}
			else
			{
				// Update our device state so it matches core's.
				for (int port = 0; port < INPUT_DEVICE_PORTS; port++)
				{
					// RWBFIX: Need to address device type issues when keyboards are functional
					gamepad_data[port].is_connected = core->IsConnected(port);
					if (gamepad_data[port].is_connected)
					{
						gamepad_data[port].gamepad_state.Gamepad = core->GetGamepadState(port);
						gamepad_data[port].capabilities = core->GetCapabilities(port);
					}

					// Keyboard sync with core:
					// RWBFIX - need to make keyboard specific check
					keyboard_data[port].is_connected = core->IsConnected(port);
				}
			}
			return XCR::Result::SUCCESS;
		}


		// Set a property for this component.
		// Properties to set:
		//		synch (frame|time) : sets playback synchronization mode.
		//	parameters:
		//		name : name of property to set
		//		value : value to set property to
		//	returns an error code
		XCR::Result Player::SetStringProperty(LPCSTR name, LPCSTR value1, LPCSTR value2)
		{
			if (lstrcmpiA(name, "synch") == 0)
			{
				if (value2 != NULL)
				{
					return XCR::Result::ERROR_TOO_MANY_ARGUMENTS;
				}
				if (lstrcmpiA(value1, "frame") == 0)
				{
					time_based_playback = false;
					return XCR::Result::SUCCESS;
				}
				else if (lstrcmpiA(value1, "time") == 0)
				{
					time_based_playback = true;
					return XCR::Result::SUCCESS;
				}
				return XCR::Result::ERROR_INVALID_VALUE;
			}
			return IPlayer::SetStringProperty(name, value1, value2);
		}

        XCR::Result Player::SetDefault()
        {
            time_based_playback = true;
            return XCR::Result::SUCCESS;
        }


		// Start this component.
		XCR::Result Player::Start()
		{
			if (is_playing)
			{
				return XCR::Result::ERROR_ALREADY_PLAYING;
			}

			is_playing = true;
			absolute_start_time_ms = GetTickCount();
			relative_current_frame = 0;
			XCR::Result result = event_sequence->Start();
			if (!result)
			{
				is_playing = false;
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

			return XCR::Result::SUCCESS_PLAYBACK_STARTED;
		}


		// Stop this component.
		XCR::Result Player::Stop()
		{
			if (!is_playing) return XCR::Result::ERROR_NOT_PLAYING;

			event_sequence->Stop();
			is_playing = false;

			return XCR::Result::SUCCESS_PLAYBACK_STOPPED;
		}


		// Retrieve status.
		XCR::Result Player::GetStatus()
		{
			return is_playing ? XCR::Result::STATUS_PLAYING : XCR::Result::STATUS_IDLE;
		}


		//JKBNEW:
		XCR::Result Player::SaveState(DirectDiskOutputStream* ddos)
		{
			//???
			// ddos->Write((LPVOID)this, (DWORD)sizeof(this));

			// Write playing bool:
			if (!ddos->Write((LPVOID)&is_playing,(DWORD)sizeof(bool)))
				return XCR::Result::ERROR;

			// Write time based playback bool:
			if (!ddos->Write((LPVOID)&time_based_playback,(DWORD)sizeof(bool)))
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
		XCR::Result Player::ResumeState(DirectDiskInputStream* ddis)
		{
			// read playing bool:
			if (!ddis->Read((LPVOID)&is_playing,(DWORD)sizeof(bool)))
				return XCR::Result::ERROR;

			// read time based playback bool:
			if (!ddis->Read((LPVOID)&time_based_playback,(DWORD)sizeof(bool)))
				return XCR::Result::ERROR;

			// read gamepad data array: (NECESSARY?)
			for (int i = 0; i < INPUT_DEVICE_PORTS; i++)
			{
				if (!ddis->Read((LPVOID)&gamepad_data[i],(DWORD)sizeof(GamepadData)))
					return XCR::Result::ERROR;
			}

			// read dbg kbd data array: (NECESSARY?)
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

			return res;
		}



		// ----------------------------------------------------------------
		// IPlayer interface
		// ----------------------------------------------------------------


		// Initialize this component.
		XCR::Result Player::Initialize(IPlayerInterface *core_interface)
		{
			core = core_interface;
			return XCR::Result::SUCCESS;
		}

		// Emulate XInputGetState.
		XCR::Result Player::InputGetState(DWORD &xresult, DWORD dwPort, PXINPUT_STATE pState)
		{
			do_work();
			return XCR::Result::SUCCESS_UNHANDLED;
		}

		// Emulate XInputGetKeystroke
		// a-dannat Added dwFlags parameter to propogate the XINPUT_FLAG_GAMEPAD and 
		// XINPUT_FLAG_KEYBOARD flags if they are specified instead of XINPUT_FLAG_ANYDEVICE
		XCR::Result Player::InputGetKeystroke(DWORD &xresult, DWORD dwPort, DWORD dwFlags, PXINPUT_KEYSTROKE pKeystroke)
		{
			do_work();
			return XCR::Result::SUCCESS_UNHANDLED;
		}



		// ----------------------------------------------------------------
		// helper functions
		// ----------------------------------------------------------------


		// Checks if time has advanced sufficiently for next event.
		inline bool Player::time_for_next_event()
		{
			//JKBNEW: need to adjust relative times based on resume state reboot:
			//  relative_current_time_ms = GetTickCount() - absolute_start_time_ms;
			relative_current_time_ms = GetTickCount() + m_tickCountAdjuster - absolute_start_time_ms;

			// Need to write debug output to see what is going on
			//char strMessage[64] = "";
			//sprintf(strMessage, "Time Compare (%d): %d >= %d ?\n", time_based_playback, relative_current_time_ms
			//	                                          , event_sequence->GetTime() );
			//REPORT(strMessage);

			return (time_based_playback && relative_current_time_ms >= event_sequence->GetTime())
				|| (!time_based_playback && relative_current_frame >= event_sequence->GetFrame());
		}


        XCR::Result Player::handle_events()
        {
            XCR::Result result = XCR::Result::SUCCESS;
            while (time_for_next_event() && result)
            {
                if (IEventInput::EVENT_END == event_sequence->GetEventType())
                {
                    result = handle_events(0);
                    break;
                }
                else if (0 == event_sequence->GetCombinedPort())
                {
                    result = handle_events(event_sequence->GetPort());
                }
                else
                {
                    DWORD combined_port = event_sequence->GetCombinedPort();
                    for (UINT i=1, j=0; i<0xF; i<<=1, ++j)
                    {
                        if (i == (combined_port & i))
                        {
                            result = handle_events(j);
                            if (!result)
                            {
                                break;
                            }
                        }
                    }
                }
                if (!result)
                {
                    continue;
                }
                result = event_sequence->AdvanceToNextEvent();
            }
            return result;
        }

		// Handle events.
		XCR::Result Player::handle_events(DWORD port)
		{
            XCR::Result result = XCR::Result::SUCCESS;

            switch (event_sequence->GetEventType())
			{
			case IEventInput::EVENT_GAMEPAD_STATE:
				{
					REPORT("STATE\n");
					event_sequence->GetGamepadState(gamepad_data[port].gamepad_state.Gamepad);
					core->SetGamepadState(port, gamepad_data[port].gamepad_state.Gamepad);
				}
				break;

			//JKB:
			case IEventInput::EVENT_CONNECT_GAMEPAD:
				// Connect controller to core.
				// No effect if controller with matching capabilities is already connected.
				{
					gamepad_data[port].is_connected = true;
					gamepad_data[port].capabilities = event_sequence->GetCapabilities();
					// If it doesn't match current connected core device, then disconnect it.
					// This should also pick up controller type changes.
					if (core->IsConnected(port) && 
						core->GetCapabilities(port) != gamepad_data[port].capabilities)
					{
						core->Disconnect(port);
					}
					// Connect it to core.
					if (!core->IsConnected(port))
					{
						core->SetDeviceType(port, XINPUT_DEVTYPE_GAMEPAD);
						core->Connect(port, gamepad_data[port].capabilities);
					}
					event_sequence->GetGamepadState(gamepad_data[port].gamepad_state.Gamepad);
					core->SetGamepadState(port, gamepad_data[port].gamepad_state.Gamepad);
				}
				break;


			case IEventInput::EVENT_DISCONNECT_ANY_DEVICE:
				// Disconnect any device from core.
				// No effect if no device is already disconnected.
				{
					keyboard_data[port].is_connected = false;
					gamepad_data[port].is_connected = false;
					if (core->IsConnected(port))
					{
						core->Disconnect(port);
					}
				}
				break;

			// Keyboard event support in event_sequence object!
			case IEventInput::EVENT_KEYBOARD_STATE:
				{
					REPORT("KEYBOARD STATE\n");
					event_sequence->GetKeyboardState(keyboard_data[port].keystroke);
					core->SetKeyboardState(port, keyboard_data[port].keystroke);
				}
				break;
				
			// Keyboard connect event:
			case IEventInput::EVENT_CONNECT_KEYBOARD:
				// Connect keyboard to core.
				// No effect if debug keyboard with matching capabilities is already connected.
				{
					REPORT("CONNECT KEYBOARD\n");
					keyboard_data[port].is_connected = true;
					// RWBFIX: Review - are capabilities really garbage?
					// Connect it to core.
					if (!core->IsConnected(port))
					{
						XINPUT_CAPABILITIES garbage_capabilities;
						core->SetDeviceType(port, XINPUT_DEVTYPE_USB_KEYBOARD);
						core->Connect(port, garbage_capabilities);
					}
				}
				break;

				// a-dannat Added the keystroke event that adds a keystroke and changes the gamepad state.
				//  Currently only generated by a Text Sequence
			case IEventInput::EVENT_KEYSTROKE:
				{
					REPORT("KEYSTROKE\n");
					event_sequence->GetGamepadState(gamepad_data[port].gamepad_state.Gamepad);
					core->SetGamepadState(port, gamepad_data[port].gamepad_state.Gamepad);
					// a-dannat if there was a gamepad state change, we need to make this event available
					//  to both gamepad_data and keyboard_data since keyboard_data can support gamepad butons.
					//  The KEYBOARD_STATE and GAMEPAD_STATE events could be combined. 
					//  If one is updated, the other one should be as well.
					//  One updates for polling (XInputGetState) on the gamepad and
					//  the other updates for keystroke events (XInputGetKeystroke) on the gamepad and keyboard
					event_sequence->GetKeyboardState(keyboard_data[port].keystroke);
					core->SetKeyboardState(port, keyboard_data[port].keystroke);
				}
				break;

            case IEventInput::EVENT_TOGGLE_HUD:
                {
                    REPORT("TOGGLE HUD\n");
                    IControllerInterface* base_core = static_cast<IControllerInterface*>(core);
                    base_core->ToggleHud(port);
                    break;
                }

			case IEventInput::EVENT_END:
				core->LogMessage("XCR: Sequence playback complete.\r\n");
				result = Stop();

				//JKBNEW:
				m_tickCountAdjuster = 0x0;

				return result;
				
			}

			return result;
		}


		// Updates connected devices to match our state.
		void Player::update_connected_devices()
		{
			// Throw out any currently connected devices and tell it to connect ours.
			for (DWORD port = 0; port < INPUT_DEVICE_PORTS; port++)
			{
				// Is it supposed to be connected?
				if (gamepad_data[port].is_connected)
				{
					// Is the emulated device connected and do the capabilities match?
					if (core->IsConnected(port))
					{
						// If capabilities differ, disconnect device and connect ours.
						if (core->GetCapabilities(port) != gamepad_data[port].capabilities)
						{
							REPORT("Update: Disconnect\n");
							core->Disconnect(port);
							REPORT("Update: Connect\n");
							core->Connect(port, gamepad_data[port].capabilities);
						}
						// Otherwise just leave connected.
					}
					else
					{
						// Not connected.  Connect us.
						REPORT("Update: Connect\n");
						core->Connect(port, gamepad_data[port].capabilities);
					}
				}
				// Keyboard considerations:
				// RWBFIX re-examine keyboard issues when fixed in XDK
				//else if (keyboard_data[port].is_connected)
				//{
				//	// Is the emulated device connected?
				//	if (!core->IsConnected(XINPUT_DEVTYPE_USB_KEYBOARD, port))
				//	{
				//		// Not connected.  Connect us.
				//		REPORT("Update: Connect\n");
				//		XINPUT_CAPABILITIES garbage_capabilities;
				//		core->Connect(port, garbage_capabilities);
				//	}
				//	// otherwise leave connected...
				//}
				// Device is not open.  Should not be connected.
				else
				{
					if (core->IsConnected(port))
					{
						REPORT("Update: Disconnect Device\n");
						core->Disconnect(port);
					}
				}
			}
		}


		void Player::do_work()
		{
			if (is_playing)
			{
				XCR::Result result = handle_events();
				if (!result)
				{
					core->LogMessage("XCR: Playback unexpectedly stopped: ");
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
					Stop();
				}
				relative_current_frame++;
			}
		}

	}
}


#endif