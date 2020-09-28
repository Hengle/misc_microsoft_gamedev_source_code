//	Interfaces.h : XeCR component interface definitions, data types, and error class.
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

#include <xtl.h>
#include <crtdbg.h>

#include <map>
#include <vector>
#include <string>

#include "XenonUtility.h"
using namespace XenonUtility;
#include "xecr_Result.h"

#include "xutil_OutputStream.h"
#include "xutil_InputStream.h"

#define INPUT_DEVICE_PORTS 4




namespace XCR
{
	namespace ControlUnit
	{

		// Forward declaration of class to appear later.
		class InputDeviceData;




		// ================================================================
		//	IComponent interface
		//
		//	Base interface for all XCR components.
		// ================================================================
		class IComponent
		{
		public:
			enum ComponentType { COMPONENT_REC, COMPONENT_PLAY, COMPONENT_CONTROL, COMPONENT_LOG };
			virtual ~IComponent() { }

			// Notify this component it is being selected.
			virtual XCR::Result SelectComponent()
			{
				return XCR::Result::SUCCESS_UNHANDLED;
			}

            // Set a property for this component.
            //	parameters:
            //		name : name of property to set
            //		value : value to set property to
            //	returns result code
            virtual XCR::Result SetStringProperty(LPCSTR name, LPCSTR value1, LPCSTR value2 = NULL)
            {
                return XCR::Result::ERROR_INVALID_PROPERTY;
            }

            // Set a property for this component.
            //	parameters:
            //		name : name of property to set
            //		value : value to set property to
            //	returns result code
            virtual XCR::Result SetDefault(void)
            {
                return XCR::Result::ERROR_INVALID_PROPERTY;
            }

			// Start this component.
			//	returns result code
			virtual XCR::Result Start()
			{
				return XCR::Result::ERROR_CANNOT_START_COMPONENT;
			}

			// Stop this component.
			//	returns result code
			virtual XCR::Result Stop()
			{
				return XCR::Result::ERROR_CANNOT_STOP_COMPONENT;
			}
			
			// Retrieve component type.
			//	returns component type.
			virtual ComponentType GetComponentType() = 0;

			// Retrieve status.
			virtual XCR::Result GetStatus()
			{
				return XCR::Result::STATUS_IDLE;
			}

			// Retrieve helpstring.
			virtual const char *GetHelpString()
			{
				return "XeCR Component.";
			}

			//JKBNEW:
			virtual XCR::Result SaveState(DirectDiskOutputStream *ddos)
			{
				return XCR::Result::SUCCESS_UNHANDLED;
			}

			//JKBNEW:
			virtual XCR::Result ResumeState(DirectDiskInputStream* ddis)
			{
				return XCR::Result::SUCCESS_UNHANDLED;
			}
		};




		// ================================================================
		//	ICoreInterface interface
		//
		//	Components use this interface to talk to the control unit.
		//	The central XCR component will implement this interface
		//		and hand it out to components to use.
		// ================================================================
		class ICoreInterface
		{
		public:
			// Send a message to all loggers.
            virtual void LogMessage(LPCSTR message) = 0;
		};




		// ================================================================
		// IRecorderInterface interface
		//
		//	Interface for monitoring (emulated) input devices attached to
		//		the (XCR-Core emulated) Xbox controller ports.
		// ================================================================
		class IRecorderInterface :
			public ICoreInterface
		{
		public:
			// Is a device connected to this port?
			//JKB: device type considerations:
			virtual bool IsConnected(DWORD port) const = 0;
			// Retrieve capabilities.
			virtual const XINPUT_CAPABILITIES &GetCapabilities(DWORD port) const = 0;
			// Retrieve gamepad data.
			virtual const XINPUT_GAMEPAD &GetGamepadState(DWORD port) const = 0;
			// Keyboard state:
			virtual const XINPUT_KEYSTROKE &GetKeyboardState(DWORD port) const = 0;			
			// Distinguish type:
			virtual const DWORD GetDeviceType(DWORD port) const = 0;
		};




		// ================================================================
		// IPlayerInterface interface
		//
		//	Interface for manipulating (emulated) input devices attached to
		//		the (XCR-Core emulated) Xbox controller ports.
		// ================================================================
		class IPlayerInterface :
			public IRecorderInterface
		{
		public:
			// Connect device with capabilities.
			//	Use this to connect your device to the emulated port and provide its capabilities.
			//	The port must currently not have a device connected.
			//	If it does, nothing happens.
			//  This no longer considers device type
			virtual void Connect(DWORD port, const XINPUT_CAPABILITIES &capabilities) = 0;
			// Disconnect.
			//	Use this to disconnect your device from the emulated port.
			//	The port must have a connected device.
			// If it doesn't, nothing happens.
			virtual void Disconnect(DWORD port) = 0;
			// Set gamepad data.
			virtual void SetGamepadState(DWORD port, const XINPUT_GAMEPAD &state) = 0;
			// Set keyboard data:
			virtual void SetKeyboardState(DWORD port, const XINPUT_KEYSTROKE &pKeystroke) = 0;
			// Type distinguish:
			virtual void SetDeviceType(DWORD port, DWORD type) = 0;
		};




		// ================================================================
		//	IControllerInterface interface
		//
		//	Controllers use this interface to talk to the control unit.
		//	The central XCR component will implement this interface
		//		and hand it out to controllers to use.
		// ================================================================
		class IControllerInterface :
			public IPlayerInterface
		{
		public:
			typedef std::vector<std::string> ComponentNameList;
			virtual void GetComponentNames(ComponentNameList &names) const = 0;
            virtual XCR::Result SetComponentStringProperty(LPCSTR component_name, LPCSTR property, LPCSTR value1, LPCSTR value2 = NULL) = 0;
            virtual XCR::Result SetComponentDefault(LPCSTR component_name) = 0;
			virtual XCR::Result StartComponent(LPCSTR component) = 0;
            virtual XCR::Result StartComponent(LPCSTR component, bool clear_rs_if_necessary) = 0;
			virtual XCR::Result StopComponent(LPCSTR component) = 0;
            virtual XCR::Result SelectComponent(LPCSTR component) = 0;
            virtual XCR::Result SelectComponent(LPCSTR component, bool clear_rs_if_necessary) = 0;
            virtual XCR::Result GetComponentStatus(LPCSTR component) const = 0;
            virtual XCR::Result GetComponentStatus2(LPCSTR component) const = 0;
			virtual XCR::Result GetComponentHelpString(LPCSTR component, const char *&helpstring) const = 0;
			virtual XCR::Result ProcessSignal(LPCSTR msg) const = 0;
            virtual XCR::Result GetHudUser() = 0; // MJMXAM: added for hud user command
            virtual XCR::Result GetHudStatus() = 0; // MJMXAM: added for hud status command
            virtual XCR::Result GetHudStatus(LPCSTR port) = 0; // MJMXAM: added for hud status command
            virtual XCR::Result GetHudStatus(DWORD port) = 0; // MJMXAM: added for hud status command
            virtual XCR::Result ToggleHud(LPCSTR port) = 0; // MJMXAM: added to support hud automation
            virtual XCR::Result ToggleHud(DWORD port) = 0; // MJMXAM: added to support hud automation
			virtual XCR::Result SaveState(DirectDiskOutputStream* ddos) = 0;
			virtual XCR::Result ResumeState(DirectDiskInputStream* ddis) = 0;
            virtual XCR::Result GetActivePlayer(void) = 0;
            virtual XCR::Result GetActiveRecorder(void) = 0;
		};




		// ================================================================
		//	IController interface
		//
		//	Base interface for all controllers.
		// Controllers are components that can control other components.
		// ================================================================
		class IController :
			public IComponent
		{
		public:
			// Initialize this controller.
			//	parameters:
			//		pXCR : a pointer to the XCR Control Unit interface this controller will use to issue commands.
			//	returns false if an error occurred.
			virtual XCR::Result Initialize(IControllerInterface *pXCR)
			{
				return XCR::Result::SUCCESS_UNHANDLED;
			}

			// Do one frame worth of work.
			//	parameters:
			//		dwFrameNum : the frame number
			//		dwTimeMs : the time in ms
			virtual void DoWork(DWORD dwFrameNum, DWORD dwTimeMs)
			{
				// Do nothing if unhandled.
			}

			// Notifies this controller of a game state change.
			//	parameters:
			//		szState : a string describing the state change
			virtual void NotifyState(LPCSTR szState)
			{
				// Do nothing if unhandled.
			}

			// Retrieve component type.
			//	returns component type.
			virtual ComponentType GetComponentType()
			{
				return COMPONENT_CONTROL;
			}

			// Retrieve helpstring.
			virtual const char *GetHelpString()
			{
				return "XeCR Controller.  Controls other XeCR components.";
			}
		};




		// ================================================================
		//	IPlayer interface
		//
		//	Base interface for all XCR input sources.
		//	Derive from this interface to implement your own input source.
		//	Note that you don't have to implement InputOpen and InputClose because handles
		//		are handled for you in the XCRCore.  Use the InputDeviceData array pointer
		//		to connect and disconnect devices during any of the functions.
		// ================================================================
		class IPlayer :
			public IComponent
		{
		public:
			// Initialize this component.
			virtual XCR::Result Initialize(IPlayerInterface *devices)
			{
				return XCR::Result::SUCCESS_UNHANDLED;
			}

			// Emulate XInputGetState.
			virtual XCR::Result InputGetState(DWORD &xresult, DWORD dwPort, PXINPUT_STATE pState)
			{
				return XCR::Result::SUCCESS_UNHANDLED;
			}

			// Emulate XInputSetState.
			virtual XCR::Result InputSetState(DWORD &xresult, DWORD dwPort, PXINPUT_VIBRATION pVibration)
			{
				return XCR::Result::SUCCESS_UNHANDLED;
			}

			// Emulate XInputGetKeystroke
			// a-dannat Added dwFlags parameter to propogate the XINPUT_FLAG_GAMEPAD and 
			// XINPUT_FLAG_KEYBOARD flags if they are specified instead of XINPUT_FLAG_ANYDEVICE
			virtual XCR::Result InputGetKeystroke(DWORD &xresult, DWORD dwPort, DWORD dwFlags, PXINPUT_KEYSTROKE pKeystroke)
			{
				return XCR::Result::SUCCESS_UNHANDLED;
			}

			// Retrieve component type.
			virtual ComponentType GetComponentType()
			{
				return COMPONENT_PLAY;
			}

			// Retrieve helpstring.
			virtual const char *GetHelpString()
			{
				return "XeCR Player.  Plays input data.";
			}
		};




		// ================================================================
		//	IRecorder interface
		//
		//	Base interface for all XCR output sinks.
		// ================================================================
		class IRecorder :
			public IComponent
		{
		public:
			// Initialize.
			virtual XCR::Result Initialize(IRecorderInterface *pXCR)
			{
				return XCR::Result::SUCCESS_UNHANDLED;
			}

			// Inform recorder of device disconnection.
			//JKB: changed to distinguish gamepads:
			//virtual XCR::Result RecordGamepadDisconnect(DWORD port)
			//{
			//	return XCR::Result::SUCCESS_UNHANDLED;
			//}

			//JKB: general disconnect support:
			virtual XCR::Result RecordDeviceDisconnect(DWORD port)
			{
				return XCR::Result::SUCCESS_UNHANDLED;
			}

            // MJMXAM: add record toggle hud feature
            virtual XCR::Result RecordToggleHud(DWORD port)
            {
                return XCR::Result::SUCCESS_UNHANDLED;
            }

			// Inform recorder of device connection.
			//JKB: distinguish gamepads:
			virtual XCR::Result RecordGamepadConnect(DWORD port, const XINPUT_CAPABILITIES &capabilities, const XINPUT_GAMEPAD &gamepad_state)
			{
				return XCR::Result::SUCCESS_UNHANDLED;
			}

			// Inform recorder of device data.
			virtual XCR::Result RecordInputGetState(DWORD port, const XINPUT_GAMEPAD &gamepad_state)
			{
				return XCR::Result::SUCCESS_UNHANDLED;
			}

			// Keyboard connect support:
			virtual XCR::Result RecordKeyboardConnect(DWORD port, const XINPUT_KEYSTROKE &keyboard_state)
			{
				return XCR::Result::SUCCESS_UNHANDLED;
			}

			//// Keyboard disconnect support:
			//virtual XCR::Result RecordKeyboardDisconnect(DWORD port)
			//{
			//	return XCR::Result::SUCCESS_UNHANDLED;
			//}

			// Inform recorder of keyboard data
			virtual XCR::Result RecordInputGetKeystroke(DWORD dwPort, const PXINPUT_KEYSTROKE pKeystroke)
			{
				return XCR::Result::SUCCESS_UNHANDLED;
			}

			// Inform recorder of frame advance.
			virtual XCR::Result FrameAdvance()
			{
				return XCR::Result::SUCCESS_UNHANDLED;
			}

			// Retrieve component type.
			virtual ComponentType GetComponentType()
			{
				return COMPONENT_REC;
			}

			// Retrieve helpstring.
			virtual const char *GetHelpString()
			{
				return "XeCR Recorder.  Records input data.";
			}
		};




		// ================================================================
		//	ILogger interface
		//
		//	Base interface for all loggers.
		// Loggers are components that handle output status messages.
		// ================================================================
		class ILogger :
			public IComponent
		{
		public:
			// Initialize this logger.
			//	returns false if an error occurred.
			virtual XCR::Result Initialize()
			{
				return XCR::Result::SUCCESS_UNHANDLED;
			}

			// Passes a message to this logger to log.
			//	parameters:
			//		message : a string to log
			virtual void LogMessage(LPCSTR message)
			{
				// Do nothing if unhandled.
			}

			// Retrieve component type.
			//	returns component type.
			virtual ComponentType GetComponentType()
			{
				return COMPONENT_LOG;
			}

			// Retrieve helpstring.
			virtual const char *GetHelpString()
			{
				return "XeCR Logger.  Logs event messages.";
			}
		};




		// ================================================================
		// InputDeviceData class
		//
		// Object representing data about an input device.
		// ================================================================
		class InputDeviceData
		{
		public:
			InputDeviceData();

			// Disconnect.
			//	Use this to disconnect your device from the emulated port.
			//	The port must have a connected device.
			void DisconnectDevice();

			// Connect device with capabilities.
			//	Use this to connect your device to the emulated port and provide its capabilities.
			//	The port must currently not have a device connected.
			void ConnectDevice(const XINPUT_CAPABILITIES &capabilities);

			// Is a device connected to this port?
			bool IsConnected() const;

			// Retrieve capabilities.
			const XINPUT_CAPABILITIES &GetCapabilities() const;

			// Get and set this device's type:
			const DWORD GetDeviceType() const;
			void SetDeviceType(DWORD type);

			// This device's current state.
			XINPUT_STATE GamepadState;

			// Keyboard support:
			// a-dannat Changed KeyboardState so that it is the current keystroke available for reading. It is not really a state.
			XINPUT_KEYSTROKE KeyboardState;
			DWORD AdvanceToNextKeystroke(DWORD dwFlags);  // Side effect: sets KeyboardState.  returns ERROR_SUCCESS or ERROR_EMPTY
			DWORD AddKeystroke( const XINPUT_KEYSTROKE &pKeystroke );  // returns ERROR_SUCCESS or ERROR_DISK_FULL

			// Change flag to signal recorder
			bool Changed;

		protected:
			// Is there a device connected?
			DWORD m_is_connected : 1;
			// This device's capabilities.
			XINPUT_CAPABILITIES m_capabilities;
			//JKB: device type:
			DWORD m_device_type;

			// a-dannat creating a buffer for keystrokes. This must be greater then 1.
			static const WORD KEYSTROKE_BUFFER = 10;
			XINPUT_KEYSTROKE m_KeystrokeBuffer[KEYSTROKE_BUFFER];
			WORD m_CurrentKeystrokeIndex;
			WORD m_CurrentKeystrokeWriteIndex;
		};




		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		// Emulated input devices
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

		inline InputDeviceData::InputDeviceData() :
			m_is_connected(false)
		{
			memset(&m_capabilities, 0, sizeof (m_capabilities));
			memset(&GamepadState, 0, sizeof (GamepadState));
			memset(&KeyboardState,0,sizeof(KeyboardState));
			m_device_type = NULL;

			// a-dannat initialize the circular buffer
			memset(&m_KeystrokeBuffer, 0, sizeof(XINPUT_KEYSTROKE)*KEYSTROKE_BUFFER);
			m_CurrentKeystrokeIndex = KEYSTROKE_BUFFER - 1;
			m_CurrentKeystrokeWriteIndex = 0;
		}


		// Disconnect a port.
		inline void InputDeviceData::DisconnectDevice()
		{
			_ASSERTE(m_is_connected);
			m_is_connected = false;
			m_device_type = NULL;
			Changed = true;
		}


		// Connect device with certain capabilities.
		inline void InputDeviceData::ConnectDevice(const XINPUT_CAPABILITIES &capabilities)
		{
			m_is_connected = true;
			m_capabilities = capabilities;
			m_device_type = capabilities.Type;
			Changed = true;
		}


		// Is a port connected?
		inline bool InputDeviceData::IsConnected() const
		{
			return m_is_connected;
		}


		// Retrieve capabilities.
		inline const XINPUT_CAPABILITIES &InputDeviceData::GetCapabilities() const
		{
			return m_capabilities;
		}

		// Get this device's type:
		inline const DWORD InputDeviceData::GetDeviceType() const
		{
			return m_device_type;
		}

		// Set this device's type
		inline void InputDeviceData::SetDeviceType(DWORD type)
		{
			m_device_type = type;
		}

        // MMILLS: MARCH, 2005 XDK UPDATE
		inline bool operator ==(const XINPUT_CAPABILITIES &a, const XINPUT_CAPABILITIES &b)
		{
			return a.SubType == b.SubType
                && a.Gamepad.bLeftTrigger == b.Gamepad.bLeftTrigger
                && a.Gamepad.bRightTrigger == b.Gamepad.bRightTrigger
				&& a.Gamepad.sThumbLX == b.Gamepad.sThumbLX
				&& a.Gamepad.sThumbLY == b.Gamepad.sThumbLY
				&& a.Gamepad.sThumbRX == b.Gamepad.sThumbRX
				&& a.Gamepad.sThumbRY == b.Gamepad.sThumbRY
				&& a.Gamepad.wButtons == b.Gamepad.wButtons
				&& a.Vibration.wLeftMotorSpeed == a.Vibration.wLeftMotorSpeed
				&& a.Vibration.wRightMotorSpeed == b.Vibration.wRightMotorSpeed;
		}


		inline bool operator !=(const XINPUT_CAPABILITIES &a, const XINPUT_CAPABILITIES &b)
		{
			return !(a == b);
		}

		// a-dannat returns ERROR_SUCCESS or ERROR_EMPTY
		//  Side effect: sets KeyboardState.
		inline DWORD InputDeviceData::AdvanceToNextKeystroke(DWORD dwFlags)
		{
			// zero out the old current keystroke we are finished with
			memset(&m_KeystrokeBuffer[m_CurrentKeystrokeIndex], 0, sizeof(m_KeystrokeBuffer[m_CurrentKeystrokeIndex]));

			// Determine the next index to read from
			WORD NextIndex = m_CurrentKeystrokeIndex+1;
			// Loop around if necessary
			if ( NextIndex >= KEYSTROKE_BUFFER )
				NextIndex = 0;

			// determine if we are looking for a gamepad keystroke or a keyboard keystroke and check the next input to see if it is the specified type
			if ( dwFlags == XINPUT_FLAG_GAMEPAD && (m_KeystrokeBuffer[NextIndex].VirtualKey < VK_PAD_A || m_KeystrokeBuffer[NextIndex].VirtualKey > VK_PAD_RTHUMB_DOWNLEFT)  )
			{
				// we are waiting for a gamepad button, return empty.
				// zero out KeyboardState and return ERROR_EMPTY
				KeyboardState = m_KeystrokeBuffer[m_CurrentKeystrokeIndex];  // The buffer is currently pointing to an empty keystroke.
				return ERROR_EMPTY;
			}
			if ( dwFlags == XINPUT_FLAG_KEYBOARD && m_KeystrokeBuffer[NextIndex].VirtualKey >= VK_PAD_A && m_KeystrokeBuffer[NextIndex].VirtualKey <= VK_PAD_RTHUMB_DOWNLEFT  )
			{
				// we are waiting for a keyboard button, return empty.
				// zero out KeyboardState and return ERROR_EMPTY
				KeyboardState = m_KeystrokeBuffer[m_CurrentKeystrokeIndex];  // The buffer is currently pointing to an empty keystroke.
				return ERROR_EMPTY;
			}

			// Check to see if the next read location is the write location. If so, we have no new input yet.
			if ( NextIndex == m_CurrentKeystrokeWriteIndex )
			{
				// zero out KeyboardState and return ERROR_EMPTY
				KeyboardState = m_KeystrokeBuffer[m_CurrentKeystrokeIndex];  // The buffer is currently pointing to an empty keystroke.
				return ERROR_EMPTY;
			}
			else
			{
				// We have a new keystroke to read, advance to it, copy it over to KeyboardState for everyone to read, and return success.
				m_CurrentKeystrokeIndex = NextIndex;
				// copy over the new keystroke for everyone to grab
				KeyboardState = m_KeystrokeBuffer[m_CurrentKeystrokeIndex];
				// return success
				return ERROR_SUCCESS;
			}
		}

		// a-dannat returns ERROR_SUCCESS or ERROR_DISK_FULL
		inline DWORD InputDeviceData::AddKeystroke( const XINPUT_KEYSTROKE &pKeystroke )
		{
			// Check to see if we are up against the read index (full buffer, can't write)
			if ( m_CurrentKeystrokeWriteIndex == m_CurrentKeystrokeIndex )
				return ERROR_DISK_FULL;
			// add the new XINPUT_KEYSTROKE structure to the buffer at the appropriate index
			m_KeystrokeBuffer[m_CurrentKeystrokeWriteIndex] = pKeystroke;
			// advance to the next write index
			m_CurrentKeystrokeWriteIndex++;
			// loop around if necessary
			if ( m_CurrentKeystrokeWriteIndex >= KEYSTROKE_BUFFER )
				m_CurrentKeystrokeWriteIndex = 0;
			return ERROR_SUCCESS;
		}

	}
}
