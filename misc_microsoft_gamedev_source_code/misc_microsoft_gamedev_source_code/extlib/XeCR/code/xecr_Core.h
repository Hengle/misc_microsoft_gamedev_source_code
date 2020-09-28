//	Core.h : Core of the XCR.  Manages components and emulates XTL input functions.
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
#include <map>
#include <string>
#include <set>

typedef void (__cdecl *PXCR_SIGNAL_CALLBACK)(LPCSTR, PVOID);


namespace XCR
{
    #define XCR_VERSION_STRING "02.92.08.0301"
    #define XCR_VERSION_WIDESTRING L"02.92.08.0301"

	#define XCR_ASSERT(c,m) if (c) { OutputDebugStringA("XCR_ASSERT: "); OutputDebugStringA(m); __debugbreak();}

	namespace ControlUnit
	{
		// ================================================================
		//	Core class
		//
		//	This is the central control unit of the XCR.
		//	It manages the players, recorders, and controllers, and interfaces with the game.
		// ================================================================
		class Core :
			public IControllerInterface
		{
		public:
			Core(void);
			~Core(void);


			// ----------------------------------------------------------------
			// ICoreInterface interface
			// ----------------------------------------------------------------

			// Log a message to all registered loggers.
			virtual void LogMessage(LPCSTR message);


			// ----------------------------------------------------------------
			// IControllerInterface interface
			// ----------------------------------------------------------------

			void GetComponentNames(ComponentNameList &names) const;
            virtual XCR::Result SetComponentStringProperty(LPCSTR component_name, LPCSTR property, LPCSTR value1, LPCSTR value2 = NULL);
            virtual XCR::Result SetComponentDefault(LPCSTR component_name);
            virtual XCR::Result StartComponent(LPCSTR component_name);
            virtual XCR::Result StartComponent(LPCSTR component_name, bool clear_rs_if_necessary);
			virtual XCR::Result StopComponent(LPCSTR component_name);
            virtual XCR::Result SelectComponent(LPCSTR component_name);
            virtual XCR::Result SelectComponent(LPCSTR component_name, bool clear_rs_if_necessary);
            virtual XCR::Result GetComponentStatus(LPCSTR component) const;
            virtual XCR::Result GetComponentStatus2(LPCSTR component) const;
			virtual XCR::Result GetComponentHelpString(LPCSTR component, const char *&helpstring) const;
			virtual XCR::Result SaveState(DirectDiskOutputStream* ddos);
			virtual XCR::Result ResumeState(DirectDiskInputStream* ddis);
            virtual XCR::Result ProcessSignal(LPCSTR msg) const;
            virtual XCR::Result GetHudUser(); // MJMXAM: added to support hud user command
            virtual XCR::Result GetHudStatus(); // MJMXAM: added to support hud status command
            virtual XCR::Result GetHudStatus(LPCSTR port); // MJMXAM: added to support hud status command
            virtual XCR::Result GetHudStatus(DWORD port); // MJMXAM: added to support hud status command
            virtual XCR::Result ToggleHud(LPCSTR port); // MJMXAM: added to support hud automation
            virtual XCR::Result ToggleHud(DWORD port); // MJMXAM: added to support hud automation
            virtual XCR::Result GetActivePlayer(void);
            virtual XCR::Result GetActiveRecorder(void);

			
			// ----------------------------------------------------------------
			// IPlayerInterface interface
			// ----------------------------------------------------------------

			// Connect device with capabilities.
			//	Use this to connect your device to the emulated port and provide its capabilities.
			//	The port must currently not have a device connected.
			//	If it does, nothing happens.
			virtual void Connect(DWORD port, const XINPUT_CAPABILITIES &capabilities);

			// Disconnect.
			//	Use this to disconnect your device from the emulated port.
			//	The port must have a connected device.
			// If it doesn't, nothing happens.
			virtual void Disconnect(DWORD port);

			// Is a device connected to this port?
			virtual bool IsConnected(DWORD port) const;

			// Retrieve capabilities.
			// There is no set function because you can only set the capabilities at connection.
			virtual const XINPUT_CAPABILITIES &GetCapabilities(DWORD port) const;

			// Retrieve gamepad data.
			virtual const XINPUT_GAMEPAD &GetGamepadState(DWORD port) const;

			// Set gamepad data.
			virtual void SetGamepadState(DWORD port, const XINPUT_GAMEPAD &state);

			// Retrieve current keystroke:
			virtual const XINPUT_KEYSTROKE &GetKeyboardState(DWORD port) const;

			// Retrieve next keystroke:
			virtual const XINPUT_KEYSTROKE &GetNextKeystroke(DWORD port, DWORD dwFlags);

			// Set keyboard data:
			virtual void SetKeyboardState(DWORD port, const XINPUT_KEYSTROKE &pKeystroke);

			// Type distinguish:
			virtual const DWORD GetDeviceType(DWORD port) const;
			virtual void SetDeviceType(DWORD port, DWORD type);
			
			// ----------------------------------------------------------------
			// XCR Core methods.
			// ----------------------------------------------------------------

			// Connect a component, giving it a name.
			//	parameters:
			//		name : the name to give this component
			//		component : the component to register
			//	returns false on failure
			XCR::Result ConnectComponent(LPCSTR name, IComponent *component);

			// Notify XCR of a state change.
			// parameters:
			//		state : a nul-terminated string specifying the new state.
			void NotifyState(LPCSTR state);

            // Set the signal command handler.
            void SetSignalCallback(PXCR_SIGNAL_CALLBACK signalCallback, PVOID signalData);

			// Wrapper for state serialization:
			XCR::Result SaveStateWrapper(LPCSTR file);
			XCR::Result ResumeStateWrapper(LPCSTR file);

			// ----------------------------------------------------------------
			// Emulation functions
			//
			// These are signature-identical replacements for the XTL's X* versions.
			// Except we do secret, controller recorder stuff in them.
			// ----------------------------------------------------------------
			DWORD InputGetCapabilities(DWORD dwPort, DWORD dwFlags, PXINPUT_CAPABILITIES pCapabilities);
			DWORD InputGetState(DWORD dwPort, PXINPUT_STATE pState);
			DWORD InputSetState(DWORD dwPort, PXINPUT_VIBRATION pVibration);
			DWORD InputGetKeystroke(DWORD dwPort, DWORD dwFlags, PXINPUT_KEYSTROKE pKeystroke);						

			// ----------------------------------------------------------------
			// Constant default component names
			// ----------------------------------------------------------------
			
			// Name of the default recorder.
			static const LPSTR s_rec_component_name;

			// Name of the default player.
			static const LPSTR s_play_component_name;


		protected:
			
			// ----------------------------------------------------------------
			// Implementation
			// ----------------------------------------------------------------

			// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			// Static initialization
			// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

			// Static initialization record.
			struct ComponentInitialize
			{
				LPCSTR name;
				IComponent *component;
			};

			// List of components to initialize.
			static ComponentInitialize s_initial_components[];

			// Check if initialized yet
			void Core::initialize_if_necessary();

			// Perform initialization.
			void init();


			// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			// Input device emulation management
			// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

			void record_special_events_if_necessary(DWORD port);

			// Current device data for each port.
			InputDeviceData m_ports[INPUT_DEVICE_PORTS];

			// Keyboard port.
			DWORD m_keyboard_port;
		
			// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			// Component management
			// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

			// Component data structure.
			typedef std::map<std::string, IComponent *> ComponentMap;

		protected:

			// Map of all the components.
			ComponentMap m_components;

			// List of all the control components.
			// (Note that they are included in the component map as well for name lookup.)
			ComponentMap m_control_components;	

			// List of all the logger components.
			// (Note that they are included in the component map as well for name lookup.)
			ComponentMap m_logger_components;

			// Find a component.
			IComponent *get_component(LPCSTR name) const;

			//JKBNEW: Find a component name.
			IComponent *get_component(int index) const;
			LPCSTR get_component_name(IComponent* cmp);
			int    get_component_index(IComponent* cmp);

			// Set a component.  Add component to list or change an existing one.
			void set_component(LPCSTR name, IComponent *component);

			// Remove a component.
			void remove_component(LPCSTR name);

			// Pointer to default recorder.
			IRecorder *m_recorder;

			// Pointer to default player.
			IPlayer *m_player;

			// State tracking from the controller level:
			IComponent* m_curComponent;

            // Signal handler
            PXCR_SIGNAL_CALLBACK m_signalCallback;
			PVOID			     m_signalData;

			// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			// Processing
			// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

			// Do any necessary per-frame processing.
			// Called in every function that may be called in the main loop.
			void frame_check();
			
			// Gives all controllers an opportunity to do work.
			void do_work();

			// Number of ms elapsed.
			DWORD m_tick_count;

			// Number of frames elapsed.
			DWORD m_frame_count;

            // MJMXAM: added to store current hud status
            HANDLE m_hud_notification_handler;
            XCR::Result::ResultCode m_hud_user;
            DWORD m_dw_hud_user;
            XCR::Result::ResultCode m_is_hud_active;

            void update_hud_user(XCR::Result::ResultCode hud_user);
            void update_hud_user(DWORD dw_hud_user);
            // MJMXAM: end of XAM changes

            IController* m_rs_controller;

		};
	}
}