//	Core.cpp : Core of the XeCR.  Manages components and emulates XTL input functions.
//
//	Created 2004/10/06 Rich Bonny    <rbonny@microsoft.com>
//
//  - based on original XCR code:
//	Created 2003/03/XX David Eichorn <deichorn@microsoft.com>
//     and modified by Jon Burns     <jburns@microsoft.com>
//
//  Many of the core functions have been removed, since the input API set was pared
//  from 14 down to 4 when moving from Xbox to Xenon XDKs. Also, keyboards are now
//  handled much as standard controllers, so the conditional compilation code for
//  debug keyboards has largely been removed.
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2004 Microsoft Corp.  All rights reserved.




#ifdef _XCR_

#include "xecr_Core.h"
#ifdef XAM_HOOK		
#include <xamp.h>
#else
#include <xbdm.h>
#endif


namespace XCR
{
	//	Our own rand() functions (based on the ones in rand.c) to
	//		prevent unwanted interaction with the game.

	static long xcr_randval = 1L;
	static long xcr_packet = 1L;

	void xcr_srand(unsigned int seed)
	{
		xcr_randval = seed;
	}

	int xcr_rand()
	{
		xcr_randval = xcr_randval * 214013L + 2531011L;
		return (xcr_randval >> 16) & 0x7FFF;
	}

	namespace ControlUnit
	{


		// ----------------------------------------------------------------
		// Constructors / destructor
		// ----------------------------------------------------------------


		Core::Core(void) :
			m_recorder(NULL),
			m_player(NULL),
			m_keyboard_port(0x5),
			m_curComponent(NULL),
            m_signalCallback(NULL),
			m_signalData(NULL),
            m_hud_notification_handler(0),
            m_hud_user(XCR::Result::PORT_UNKNOWN),
            m_is_hud_active(XCR::Result::STATUS_UNKNOWN),
            m_dw_hud_user(0xffffffff),
            m_rs_controller(0)
		{
		}


		Core::~Core(void)
		{
		}




		// ----------------------------------------------------------------
		// Initialization
		// ----------------------------------------------------------------


		// Connect a component, giving it a name.
		//	parameters:
		//		name : the name to give this component
		//		component : the component to register
		//	returns false on failure
		XCR::Result Core::ConnectComponent(LPCSTR name, IComponent *component)
		{
			XCR::Result result = XCR::Result::SUCCESS;
			set_component(name, component);
			switch (component->GetComponentType())
			{
			case IComponent::COMPONENT_CONTROL:
				// Initialize
				{
					IController *controller = (IController *) component;
					result = controller->Initialize(this);
					controller->SelectComponent();
				}
				break;

			case IComponent::COMPONENT_REC:
				// Set as current recorder.
				{
					IRecorder *recorder = (IRecorder *) component;
					result = recorder->Initialize(this);
					if (result && m_recorder == NULL)
					{
						m_recorder = recorder;
						m_recorder->SelectComponent();
					}
				}
				break;

			case IComponent::COMPONENT_PLAY:
				// Initialize and set as current player.
				{
					IPlayer *player = (IPlayer *) component;
					result = player->Initialize(this);
					if (result && m_player == NULL)
					{
						m_player = player;
						m_player->SelectComponent();
					}
				}
				break;

			case IComponent::COMPONENT_LOG:
				// Add to logger component list.
				{
					ILogger *logger = (ILogger *) component;
					result = logger->Initialize();
					if (result)
					{
						logger->SelectComponent();
					}
				}
			}
			if (!result)
			{
				remove_component(name);
			}

			return result;
		}


		// Notify XCR of a state change.
		// parameters:
		//		state : a nul-terminated string specifying the new state.
		void Core::NotifyState(LPCSTR state)
		{
			for (ComponentMap::iterator it = m_control_components.begin(); it != m_control_components.end(); it++)
			{
				((IController *) it->second)->NotifyState(state);
			}
		}

        // Set the signal handler
        void Core::SetSignalCallback(PXCR_SIGNAL_CALLBACK signalCallback, PVOID signalData)
        {
            m_signalCallback = signalCallback;
			m_signalData = signalData;
        }

		// wrapper for saving state serialization:
		XCR::Result Core::SaveStateWrapper(LPCSTR file)
		{
			if (!file)
				return Result::ERROR_INVALID_FILE;

			Result res;

			DirectDiskOutputStream ddos;
			ddos.SetFilename(file);
			res = ddos.Start();		

			if (res) 
			{
				res = SaveState(&ddos);	
				ddos.Stop();

				return res;
			}

			return Result::ERROR_INVALID_FILE;
		}

		// wrapper to resume state serialization:
		XCR::Result Core::ResumeStateWrapper(LPCSTR file)
		{
			if (!file)
				return Result::ERROR_INVALID_FILE;

			Result res;

			DirectDiskInputStream ddis;
			ddis.SetFilename(file);
			res = ddis.Start();		

			if (res) 
			{
				res = ResumeState(&ddis);		
				ddis.Stop();

				DeleteFile(file);

				return res;
			}

			return Result::ERROR_INVALID_FILE;
		}




		// ----------------------------------------------------------------
		// IControllerInterface interface
		// ----------------------------------------------------------------

		void Core::LogMessage(LPCSTR message)
		{
			for (ComponentMap::iterator it = m_logger_components.begin(); it != m_logger_components.end(); it++)
			{
				((ILogger *) it->second)->LogMessage(message);
			}
		}


		// ----------------------------------------------------------------
		// IControllerInterface interface
		// ----------------------------------------------------------------

        // Set a component property to a string value.
        //	parameters:
        //		component_name : name of the component whose property to set.
        //		property : name of the property to set
        //		value : value to set the property to
        //	returns false on failure
        XCR::Result Core::SetComponentStringProperty(LPCSTR component_name, LPCSTR property, LPCSTR value1, LPCSTR value2)
        {
            IComponent *component = get_component(component_name);
            if (component == NULL)
            {
                return XCR::Result::ERROR_INVALID_COMPONENT;
            }
            return component->SetStringProperty(property, value1, value2);
        }


        // Set a component's properties to their default values.
        //	parameters:
        //		component_name : name of the component whose property to set.
        //	returns false on failure
        XCR::Result Core::SetComponentDefault(LPCSTR component_name)
        {
            IComponent *component = get_component(component_name);
            if (component == NULL)
            {
                return XCR::Result::ERROR_INVALID_COMPONENT;
            }
            return component->SetDefault();
        }


		//	Start a component.
		//	returns false on failure.
		XCR::Result Core::StartComponent(LPCSTR component_name)
		{
            return StartComponent(component_name, true);
		}

        XCR::Result Core::StartComponent(LPCSTR component_name, bool clear_rs_if_necessary)
        {
            XCR::Result result;
            IComponent *component = get_component(component_name);

            // Keep track of current component selected:
            m_curComponent = component;

            // Automatically select the component.
            if (result = SelectComponent(component_name, clear_rs_if_necessary))
            {
                result = component->Start();
            }
            return result;
        }

		//	Stop a component.
		XCR::Result Core::StopComponent(LPCSTR component_name)
		{
			IComponent *component = get_component(component_name);
			if (component)
			{
				return component->Stop();
			}
			return XCR::Result::ERROR_INVALID_COMPONENT;
		}


		// Select a component.
        XCR::Result Core::SelectComponent(LPCSTR component_name)
        {
            return SelectComponent(component_name, true);
        }

        XCR::Result Core::SelectComponent(LPCSTR component_name, bool clear_rs_if_necessary)
        {
			IComponent *component = get_component(component_name);
			if (!component)
			{
				return XCR::Result::ERROR_INVALID_COMPONENT;
			}

			// Keep track of current component selected:
			m_curComponent = component;

			switch (component->GetComponentType())
			{
            case IComponent::COMPONENT_CONTROL:
                if (0 == strcmp("rs", component_name))
                {
                    m_rs_controller = (IController *) component;
                }
                break;
			case IComponent::COMPONENT_REC:
                // If there's another recorder active, stop it now.
                if (0 != m_recorder)
                {
                    m_recorder->Stop();
                }
				// Set as active recorder.
				m_recorder = (IRecorder *) component;
				break;
			case IComponent::COMPONENT_PLAY:
                // If there's another player active, stop it now.
                if (true == clear_rs_if_necessary && 0 != m_rs_controller)
                {
                    m_rs_controller->Stop();
                    m_rs_controller = 0;
                }
                if (0 != m_player)
                {
                    m_player->Stop();
                }
				// Set as active player.
				m_player = (IPlayer *) component;
				break;
			}

			return component->SelectComponent();
		}


		// Get a component's status.
		XCR::Result Core::GetComponentStatus(LPCSTR component_name) const
		{
			IComponent *component = get_component(component_name);
			if (component)
			{
				return component->GetStatus();
			}
			return XCR::Result::ERROR_INVALID_COMPONENT;
		}

        // Get a component's status.
        XCR::Result Core::GetComponentStatus2(LPCSTR component_name) const
        {
            IComponent *component = get_component(component_name);
            XCR::Result result;
            if (component)
            {
                result = component->GetStatus();
            }
            else
            {
                return XCR::Result::ERROR_INVALID_COMPONENT;
            }
            if (result == XCR::Result::STATUS_IDLE)
            {
                bool bIsSelected = false;
                IComponent::ComponentType type = component->GetComponentType();
                switch (type)
                {
#pragma warning (disable : 4482) // non-standard extension
                case IComponent::ComponentType::COMPONENT_PLAY:
                    {
                        if (m_player == (IPlayer*) component)
                        {
                            bIsSelected = true;
                        }
                        break;
                    }
                case IComponent::ComponentType::COMPONENT_REC:
                    {
                        if (m_recorder == (IRecorder*) component)
                        {
                            bIsSelected = true;
                        }
                        break;
                    }
                case IComponent::ComponentType::COMPONENT_LOG:
                    {
                        for (ComponentMap::const_iterator it = m_logger_components.begin(); it != m_logger_components.end(); ++it)
                        {
                            if ((ILogger *) it->second == (ILogger*) component)
                            {
                                bIsSelected = true;
                                break;
                            }
                        }
                        break;
                    }
                case IComponent::ComponentType::COMPONENT_CONTROL:
                    {
                        for (ComponentMap::const_iterator it = m_control_components.begin(); it != m_control_components.end(); ++it)
                        {
                            if ((IController *) it->second == (IController*) component)
                            {
                                bIsSelected = true;
                                break;
                            }
                        }
                        break;
                    }
#pragma warning (default : 4482)
                }
                if (true == bIsSelected)
                {
                    return XCR::Result(XCR::Result::STATUS_SELECTED);
                }
                else
                {
                    return result;
                }
            }
            else
            {
                return result;
            }
        }


		XCR::Result Core::GetComponentHelpString(LPCSTR component_name, const char *&helpstring) const
		{
			IComponent *component = get_component(component_name);
			if (component)
			{
				helpstring = component->GetHelpString();
				return XCR::Result::SUCCESS;
			}
			return XCR::Result::ERROR_INVALID_COMPONENT;
		}


		void Core::GetComponentNames(ComponentNameList &names) const
		{
			names.reserve(m_components.size());
			for (ComponentMap::const_iterator it = m_components.begin(); it != m_components.end(); it++)
			{
				names.push_back(it->first);
			}
		}

        XCR::Result Core::ProcessSignal(LPCSTR msg) const
        {
            if (m_signalCallback)
                m_signalCallback(msg, m_signalData);

			return XCR::Result::SUCCESS;
        }

        // MJMXAM: added to support hud commands
        void Core::update_hud_user(XCR::Result::ResultCode hud_user)
        {
            m_hud_user = hud_user;
            switch (hud_user)
            {
            case XCR::Result::PORT_1:
                {
                    m_dw_hud_user = 0;
                    break;
                }
            case XCR::Result::PORT_2:
                {
                    m_dw_hud_user = 1;
                    break;
                }
            case XCR::Result::PORT_3:
                {
                    m_dw_hud_user = 2;
                    break;
                }
            case XCR::Result::PORT_4:
                {
                    m_dw_hud_user = 3;
                    break;
                }
            default:
                {
                    m_hud_user = XCR::Result::PORT_UNKNOWN;
                    m_dw_hud_user = 0xffffffff;
                    break;
                }
            }
        }

        // MJMXAM: added to support hud commands
        void Core::update_hud_user(DWORD dw_hud_user)
        {
            m_dw_hud_user = dw_hud_user;
            switch (dw_hud_user)
            {
            case 0:
                {
                    m_hud_user = XCR::Result::PORT_1;
                    break;
                }
            case 1:
                {
                    m_hud_user = XCR::Result::PORT_2;
                    break;
                }
            case 2:
                {
                    m_hud_user = XCR::Result::PORT_3;
                    break;
                }
            case 3:
                {
                    m_hud_user = XCR::Result::PORT_4;
                    break;
                }
            default:
                {
                    m_hud_user = XCR::Result::PORT_UNKNOWN;
                    m_dw_hud_user = 0xffffffff;
                    break;
                }
            }
        }

        // MJMXAM: added to support hud status command
        XCR::Result Core::GetHudUser()
        {
            XCR::Result hud_status = GetHudStatus();
            if (hud_status == XCR::Result::STATUS_ACTIVE)
            {
                return m_hud_user;
            }
            else
            {
                return XCR::Result::PORT_UNKNOWN;
            }
        }

        // MJMXAM: added to support hud status command
        XCR::Result Core::GetHudStatus()
        {
            initialize_if_necessary();

            if (0 != m_hud_notification_handler)
            {
                DWORD dwNotificationType = 0;
                ULONG_PTR pData = 0;
				//JKBXAM 9/21 - returns "nonzero" value, removed conditional check against TRUE:
                if (XNotifyGetNext(m_hud_notification_handler, XN_SYS_UI, &dwNotificationType,  &pData))
                {
                    BOOL bStatus = (BOOL) pData;
                    if (TRUE == bStatus)
                    {
                        m_is_hud_active = XCR::Result::STATUS_ACTIVE;
                    }
                    else
                    {
                        m_is_hud_active = XCR::Result::STATUS_INACTIVE;
                    }
                }
            }

            return m_is_hud_active;
        }

        // MJMXAM: added to support hud status command
        // The string version uses 1 based indices, so ports 1-4.
        XCR::Result Core::GetHudStatus(LPCSTR port)
        {
            if (0 == port)
                return XCR::Result::ERROR_INVALID_VALUE;
            if (1 != strlen(port))
                return XCR::Result::ERROR_INVALID_VALUE;

            DWORD dwport = static_cast<DWORD>(atoi(port)) - 1;

            return GetHudStatus(dwport);
        }

        // MJMXAM: added to support hud status command
        // The dword version uses 0 based indices, so ports 0-3.
        XCR::Result Core::GetHudStatus(DWORD port)
        {
            if (port > 3)
                return XCR::Result::ERROR_INVALID_VALUE;

            XCR::Result general_hud_status = GetHudStatus();
            if (general_hud_status == XCR::Result::STATUS_ACTIVE)
            {
                //JKBXAM 9/25 - this is confusing, perhaps later we should look at making this
				//    clearer to the user either by a new enum user specific status or
				//    changing this to a different call than GetHudStatus...
				if (XCR::Result::PORT_UNKNOWN != m_hud_user && m_dw_hud_user == port)
                {
                    return XCR::Result::STATUS_ACTIVE;
                }
                else
                {
                    return XCR::Result::STATUS_INACTIVE;
                }
				// I want to update the port if the status returns that the HUD is up... not sure
				//    if this is the best way or what...depends on what we hear back from Xbox guys.
				//if (XCR::Result::PORT_UNKNOWN == m_hud_user)
				//	update_hud_user(port);
            }
			else if (general_hud_status == XCR::Result::STATUS_INACTIVE)
			{
				// I want to update the port if the status returns that the HUD is up... not sure
				//    if this is the best way or what...depends on what we hear back from Xbox guys.
				//if (XCR::Result::PORT_UNKNOWN != m_hud_user)
				//	update_hud_user(XCR::Result::PORT_UNKNOWN);
			}

            return general_hud_status;
        }

        // MJMXAM: added to support toggle hud command
        // The string version uses 1 based indices, so ports 1-4.
        XCR::Result Core::ToggleHud(LPCSTR port)
        {
            if (0 == port)
                return XCR::Result::ERROR_INVALID_VALUE;
            if (1 != strlen(port))
                return XCR::Result::ERROR_INVALID_VALUE;

            DWORD dwport = static_cast<DWORD>(atoi(port)) - 1;

            return ToggleHud(dwport);
        }

        // MJMXAM: added to support toggle hud command
        // The dword version uses 0 based indices, so ports 0-3.
        XCR::Result Core::ToggleHud(DWORD port)
        {
            if (port > 3)
                return XCR::Result::ERROR_INVALID_VALUE;

#ifdef XAM_HOOK		
            BOOL fToggleSucceeded = XAutomationpInputXenonButton(port);
#else
			DM_XINPUT_GAMEPAD gamepad = {0};
			gamepad.wButtons = DM_XINPUT_GAMEPAD_SYSTEM_BUTTON_X360;

			BOOL fToggleSucceeded = SUCCEEDED(DmAutomationSetGamepadState(port, &gamepad));

#endif
            if (TRUE == fToggleSucceeded)
            {
                update_hud_user(port);
                return XCR::Result::SUCCESS;
            }
            else
            {
                update_hud_user(XCR::Result::PORT_UNKNOWN);
                return XCR::Result::ERROR;
            }
        }

        XCR::Result Core::GetActivePlayer(void)
        {
            LPCSTR lpcszPlayerName = get_component_name( (IComponent*) m_player );
            if (0 != m_rs_controller)
            {
                return XCR::Result::COMPONENT_RANDOM_STATE_PLAYER;
            }
            else if (0 == strcmp("ports", lpcszPlayerName))
            {
                return XCR::Result::COMPONENT_PORTS_PLAYER;
            }
            else if (0 == strcmp("monkey", lpcszPlayerName))
            {
                return XCR::Result::COMPONENT_MONKEY_PLAYER;
            }
            else if (0 == strcmp("seqplay", lpcszPlayerName))
            {
                return XCR::Result::COMPONENT_SEQUENCE_PLAYER;
            }
            else if (0 == strcmp("ddplay", lpcszPlayerName))
            {
                return XCR::Result::COMPONENT_DIRECT_DISK_PLAYER;
            }
            else if (0 == strcmp("bufplay", lpcszPlayerName))
            {
                return XCR::Result::COMPONENT_BUFFERED_PLAYER;
            }
            else
            {
                return XCR::Result::COMPONENT_UNKNOWN;
            }
        }

        XCR::Result Core::GetActiveRecorder(void)
        {
            LPCSTR lpcszRecorderName = get_component_name( (IComponent*) m_recorder );
            if (0 == strcmp("ddrec", lpcszRecorderName))
            {
                return XCR::Result::COMPONENT_DIRECT_DISK_RECORDER;
            }
            else if (0 == strcmp("bufrec", lpcszRecorderName))
            {
                return XCR::Result::COMPONENT_BUFFERED_RECORDER;
            }
            else
            {
                return XCR::Result::COMPONENT_UNKNOWN;
            }
        }

		// Save state information to enable later resumption
		XCR::Result Core::SaveState(DirectDiskOutputStream* ddos)
		{
			//???
			// ddos->Write((LPVOID)this, (DWORD)sizeof(this));

			Result res = Result::ERROR;

			// Write int value of current component index:
			int curCmpIndex = get_component_index(m_curComponent);
			if (!ddos->Write((LPVOID)&curCmpIndex, (DWORD)sizeof(int)))
				return XCR::Result::ERROR;

			// don't write if no current component, no need...
			if (curCmpIndex == -1)
				return Result::SUCCESS;

			// Write emulated port state:
			for (int i = 0; i < INPUT_DEVICE_PORTS; i++)
			{
				if (!ddos->Write((LPVOID)&m_ports[i],(DWORD)sizeof(InputDeviceData)))
					return XCR::Result::ERROR;
				// do I need to explicitly write any pointers to objects, ie) gamepad state??
			}

			// Write current component state:
			if (m_curComponent && 
				m_curComponent->GetComponentType() != IComponent::COMPONENT_CONTROL &&
				m_curComponent->GetComponentType() != IComponent::COMPONENT_LOG)
				res = m_curComponent->SaveState(ddos);

			//************************************************************************************************
			//JKB 10/7 - NEW THOUGHT: Perhaps we should iterate over all *components* and save state, not just
			//           the current component and the other controllers / loggers???
			//************************************************************************************************

			// Now give all controllers chance to save state:
			for (ComponentMap::iterator it = m_control_components.begin(); it != m_control_components.end(); it++)
			{
				res = ((IController *) it->second)->SaveState(ddos);

				if (res != Result::SUCCESS && res != Result::SUCCESS_UNHANDLED)
					return res;
			}

			// Now give all loggers chance to save state:
			for (ComponentMap::iterator it = m_logger_components.begin(); it != m_logger_components.end(); it++)
			{
				res = ((IController *) it->second)->SaveState(ddos);

				if (res != Result::SUCCESS && res != Result::SUCCESS_UNHANDLED)
					return res;
			}

			return res;
		}
		
		// Resume state from previous save
		XCR::Result Core::ResumeState(DirectDiskInputStream* ddis)
		{
			Result res = Result::ERROR;

			// Read int value of current component index:
			int curCmp = 0;
			if (!ddis->Read((LPVOID)&curCmp, (DWORD)sizeof(int)))
				return XCR::Result::ERROR;

			// don't read if no current component, no need...
			if (curCmp == -1)
				return Result::SUCCESS;

			IComponent* pCurCmp = get_component(curCmp);
			if (pCurCmp != NULL)
				m_curComponent = pCurCmp;
			else
				return Result::ERROR;

			SelectComponent(get_component_name(m_curComponent));

            // Read in emulated port state:
			for (int i = 0; i < INPUT_DEVICE_PORTS; i++)
			{
				// Note: Could be keyboard or gamepad; device type field distinguishes
				bool gpConnected = m_ports[i].IsConnected();

				// do the read of old state:
				if (!ddis->Read((LPVOID)&m_ports[i],(DWORD)sizeof(InputDeviceData)))
					return XCR::Result::ERROR;

				// fix connections if necessary:
				if (gpConnected && !m_ports[i].IsConnected())
				{
					m_ports[i].ConnectDevice(m_ports[i].GetCapabilities());
					m_ports[i].DisconnectDevice();
				}
				else if (!gpConnected && m_ports[i].IsConnected())
				{
					m_ports[i].DisconnectDevice();
					m_ports[i].ConnectDevice(m_ports[i].GetCapabilities());
				}
			}

			// Write current component state:
			if (m_curComponent && 
				m_curComponent->GetComponentType() != IComponent::COMPONENT_CONTROL &&
				m_curComponent->GetComponentType() != IComponent::COMPONENT_LOG)
				res = m_curComponent->ResumeState(ddis);

			// Now give all controllers chance to resume state:
			for (ComponentMap::iterator it = m_control_components.begin(); it != m_control_components.end(); it++)
			{
				res = ((IController *) it->second)->ResumeState(ddis);

				if (res != Result::SUCCESS && res != Result::SUCCESS_UNHANDLED)
					return res;
			}

			// Now give all loggers chance to resume state:
			for (ComponentMap::iterator it = m_logger_components.begin(); it != m_logger_components.end(); it++)
			{
				res = ((IController *) it->second)->ResumeState(ddis);

				if (res != Result::SUCCESS && res != Result::SUCCESS_UNHANDLED)
					return res;
			}

			return res;
		}


		// ----------------------------------------------------------------
		// IPlayerInterface interface
		// ----------------------------------------------------------------

		// Connect device with capabilities.
		//	Use this to connect your device to the emulated port and provide its capabilities.
		//	The port must currently not have a device connected.
		//	If it does, nothing happens.
		void Core::Connect(DWORD port, const XINPUT_CAPABILITIES &capabilities)
		{
			m_ports[port].ConnectDevice(capabilities);
		}

		// Disconnect.
		//	Use this to disconnect your device from the emulated port.
		//	The port must have a connected device.
		// If it doesn't, nothing happens.
		void Core::Disconnect(DWORD port)
		{
			m_ports[port].DisconnectDevice();
		}

		// Is a device connected to this port?
		bool Core::IsConnected(DWORD port) const
		{
			return m_ports[port].IsConnected();
		}

		// Retrieve capabilities.
		// There is no set function because you can only set the capabilities at connection.
		const XINPUT_CAPABILITIES &Core::GetCapabilities(DWORD port) const
		{
			return m_ports[port].GetCapabilities();
		}

		// Retrieve gamepad data.
		const XINPUT_GAMEPAD &Core::GetGamepadState(DWORD port) const
		{
			return m_ports[port].GamepadState.Gamepad;
		}

		// Set gamepad data.
		void Core::SetGamepadState(DWORD port, const XINPUT_GAMEPAD &state)
		{
			m_ports[port].GamepadState.Gamepad = state;
		}

		// a-dannat Gets the next keystroke by advancing the keystroke buffer.
		const XINPUT_KEYSTROKE &Core::GetNextKeystroke(DWORD port, DWORD dwFlags)
		{
			m_ports[port].AdvanceToNextKeystroke(dwFlags);
			return m_ports[port].KeyboardState;
		}

		// Retrieve keyboard data:
		const XINPUT_KEYSTROKE &Core::GetKeyboardState(DWORD port) const
		{
			// a-dannat Peeks at the current keystroke
			return m_ports[port].KeyboardState;
		}

		// a-dannat Store these events in a buffer. Advance the buffer with GetNextKeystroke.
		void Core::SetKeyboardState(DWORD port, const XINPUT_KEYSTROKE &pKeystroke)
		{
			m_ports[port].AddKeystroke( pKeystroke );
		}

		// Distinguish type :
		const DWORD Core::GetDeviceType(DWORD port) const
		{
			return m_ports[port].GetDeviceType();
		}

		// Set type
		void Core::SetDeviceType(DWORD port, DWORD type)
		{
			m_ports[port].SetDeviceType(type);
		}

		// ----------------------------------------------------------------
		// Emulation functions
		//
		// These are signature-identical replacements for the XTL's X* versions.
		// Except we do secret, controller recorder stuff in them.
		// ----------------------------------------------------------------


		// XInputGetCapabilities
		//	This function doesn't call the input source's InputGetCapabilities method, because there is none.
		//	Capabilities are always provided when the device is connected to the emulator, so we already know what they are.
		//	We just return our copy that was saved.
		// Note that port may be OR-ed with a device type, so we must strip that out first
		DWORD Core::InputGetCapabilities(DWORD port, DWORD dwFlags, PXINPUT_CAPABILITIES pCapabilities)
		{
			DWORD xresult;

			// Strip any type flags from the port number
			DWORD portNumber = (port & 0x000F);

			// If this is the first time through, we need to initialize
			// Also update the port in case it was inserted or removed.
			initialize_if_necessary();
			record_special_events_if_necessary(portNumber);

			// Retrieve the device type currently connected
			DWORD type = m_ports[portNumber].GetDeviceType();

			// If emulated device is disconnected or type does not match, fail.
			if (   NULL == type
				|| ((XINPUT_DEVTYPE_GAMEPAD == type) && (dwFlags == XINPUT_FLAG_KEYBOARD))
				|| ((XINPUT_DEVTYPE_USB_KEYBOARD == type) && !(dwFlags == XINPUT_FLAG_KEYBOARD)))
			{
				xresult = ERROR_DEVICE_NOT_CONNECTED;
			}
			else 
			{
				// Copy our data back.
				*pCapabilities = m_ports[port].GetCapabilities();
				xresult = ERROR_SUCCESS;
			}

			return xresult;
		}



		// XInputGetState
		//	Call's the current input source's InputGetState method to retrieve current state.
		//	If the input source doesn't implement such function, it will emulate it by copying back the last known state.
		DWORD Core::InputGetState(DWORD port, PXINPUT_STATE pState)
		{
			// If this is the first time through, we need to initialize
			// Also update the port in case it was inserted or removed.
			initialize_if_necessary();
			record_special_events_if_necessary(port);

			frame_check();

			DWORD xresult;

			// We need to make call to determine if port is connected
			// Unhandled usually means results are coming from playback.
			XCR::Result status = m_player->InputGetState(xresult, port, pState);
			if (status == XCR::Result::SUCCESS_UNHANDLED ||
				status == XCR::Result::ERROR_NOT_PLAYING)
			{
				// Emulate unhandled or stopped playing.  Just copy last known state back.
				*pState = m_ports[port].GamepadState;
				xresult = ERROR_SUCCESS;
			}

            // MJMXAM: for any player other than ports, we need to pass the gamepad
            //         state back to XAM in case the HUD is up. this will make sure 
            //         the automated input reaches the system.
            //JKBXAM 9/25: preserve hud status for this function:
			XCR::Result hudStatus = GetHudStatus();
            if (hudStatus == XCR::Result::STATUS_ACTIVE)
            {
                if (strcmp(get_component_name(m_player),"ports") != 0)
                {
#ifdef XAM_HOOK		
                    if (FALSE == XAutomationpInputSetState(port, &(pState->Gamepad)))
                    {
                        OutputDebugStringA("XAutomationInputSetState failed.  If the HUD is active it did not receive the intended input.");
                    }
#else
					DmAutomationSetGamepadState(port, (DM_XINPUT_GAMEPAD*)&(pState->Gamepad));
#endif
                }
				//JKBXAM 9/25 - this caused a bug: when recording/playing back HUD interactions, no button presses registered.
                // *pState = m_ports[port].GamepadState;
                // xresult = ERROR_SUCCESS;
            }

			// If emulated device is disconnected or not a gamepad, fail.
			if (XINPUT_DEVTYPE_GAMEPAD != m_ports[port].GetDeviceType())
			{
				// Always copies in last known state, even when disconnected.
				*pState = m_ports[port].GamepadState;
				xresult = ERROR_DEVICE_NOT_CONNECTED;
			}
			else
			{
				// Only record information if successful.
				if (m_recorder)
				{
					// This could fail, but most likely the failure would be that the recorder
					// was not recording. We will therefore skip the overhead of checking for
					// errors each time.
					m_recorder->RecordInputGetState(port, pState->Gamepad);
				}

				//JKBXAM 9/25 - if HUD opened, don't pass gamepad state back to game!
				if (hudStatus == XCR::Result::STATUS_ACTIVE)
				{
					//JKBXAM 9/25 - zero out memory here because the seq player uses the m_ports struct
					//    during playback, therefore assigning *pstate back to the m_ports obj was causing
					//    the bug where the game would get HUD interaction input on playback...
					memset(pState,0,sizeof(XINPUT_STATE));
					xresult = ERROR_SUCCESS;
				}
				else
				{
					// Sucessful, so save state we just got from input source.
					m_ports[port].GamepadState = *pState;
				}
			}
			// Old Code:	pState->dwPacketNumber = xcr_rand();
			// RWB: Instead of returning a random packet, which interferes with the reproducibility
			// of the monkey playback, we simply increment the packet number and makes sure we can
			// not hit an overflow condition
			pState->dwPacketNumber = xcr_packet++;
			if (xcr_packet > 0xffffff00)
				xcr_packet = 1;

			return xresult;
		}



		// XInputSetState  RWBFIX - not supported by current XDK
		DWORD Core::InputSetState(DWORD port, PXINPUT_VIBRATION pVibration)
		{
			// If this is the first time through, we need to initialize
			// Also update the port in case it was inserted or removed.
			initialize_if_necessary();
			record_special_events_if_necessary(port);

			DWORD xresult;

			// If emulated device is disconnected, fail.
			if (!m_ports[port].IsConnected())
			{
				xresult = ERROR_DEVICE_NOT_CONNECTED;
			}
			else
			{
				// Give input source a chance to handle.
				if (m_player->InputSetState(xresult, port, pVibration) == XCR::Result::SUCCESS_UNHANDLED)
				{
					// Emulate if not handled.
					// Have to do this to indicate that the input finished.

					// RWBFIX: However, XInputSetState is not currently handled in the XDK,
					// so at present there is no header field in the XINPUT_VIBRATION structure.

					//pVibration->Header->dwStatus = ERROR_SUCCESS;
					//if (pVibration->Header.hEvent != INVALID_HANDLE_VALUE && pVibration->Header.hEvent != NULL)
					//{
					//	SetEvent(pVibration->Header.hEvent);
					//}
					xresult = ERROR_IO_PENDING;
				}

			}
			return xresult;
		}


		// XInputGetKeystroke
		// This applies to both keyboards and gamepads
		DWORD Core::InputGetKeystroke(DWORD port, DWORD dwFlags, PXINPUT_KEYSTROKE pKeystroke)
		{
			// a-dannat  if the port == 255 (XUSER_INDEX_ANY) then check all ports in turn, 
			// returning the first keystroke we find or any result discovered or 
			// ERROR_DEVICE_NOT_CONNECTED if no devices are connected.
			DWORD xresult = ERROR_SUCCESS;
			if (port == XUSER_INDEX_ANY)
			{
				DWORD FailResult = ERROR_DEVICE_NOT_CONNECTED;
				for ( WORD i = 0; i < INPUT_DEVICE_PORTS; i++ )
				{
					xresult = InputGetKeystroke(i, dwFlags, pKeystroke);
					if ( xresult == ERROR_SUCCESS )
						return xresult;
					if ( xresult != ERROR_DEVICE_NOT_CONNECTED )
						FailResult = xresult;
				}
				return FailResult;
			}

			// If this is the first time through, we need to initialize
			// Also update the port in case it was inserted or removed.
			initialize_if_necessary();
			record_special_events_if_necessary(port);

			// Retrieve the device type currently connected
			DWORD type = m_ports[port].GetDeviceType();

			// this is where we do secret interception stuff:
			frame_check();

			// DWORD xresult = ERROR_SUCCESS;  // a-dannat: Moving this up so I can use it during port traversal
			
			// On playback, skip the real API call and return the correct pKeystroke values:

			// a-dannat Added dwFlags parameter to propogate the XINPUT_FLAG_GAMEPAD and 
			// XINPUT_FLAG_KEYBOARD flags if they are specified instead of XINPUT_FLAG_ANYDEVICE

			// a-dannat In order for the monkey to do it's work, we still need to call InputGetKeystroke even when all the devices are disconnected.
			//  The monkey needs the chance to reconnect a device. So do the check for no connected devices after calling InputGetKeystroke
			if (m_player->InputGetKeystroke(xresult, port, dwFlags, pKeystroke) == XCR::Result::SUCCESS_UNHANDLED)
			{
				*pKeystroke = GetNextKeystroke(port, dwFlags);
				if (pKeystroke->Unicode == 0x0 && 
					pKeystroke->VirtualKey == 0x0 &&
					pKeystroke->Flags == 0x0)
					xresult = ERROR_EMPTY; // ERROR_HANDLE_EOF;  // a-dannat Instead of returning ERROR_HANDLE_EOF which is not an expected return value, return ERROR_EMPTY
				else
				{
					xresult = ERROR_SUCCESS;
				}
			}
			// If emulated device is disconnected, fail.
			if (NULL == type)
			{
				// Always copies in last known state, even when disconnected.
				*pKeystroke = GetKeyboardState(port);
				xresult = ERROR_DEVICE_NOT_CONNECTED;
			}

			XCR::Result hudStatus = GetHudStatus();

			// On record, call the real API and record those keystroke values in the recorder:
			if (m_recorder)
			{
				// Record if any data present. Shift key, etc. may still be significant
				if ( xresult == ERROR_SUCCESS )
				{
						m_recorder->RecordInputGetKeystroke(port, pKeystroke);
				}
			}

			// MJMXAM: for any player other than ports, we need to pass the gamepad
            //         state back to XAM in case the HUD is up. this will make sure 
            //         the automated input reaches the system.
			//JKBXAM 9/25: preserve hud status for this function:

			//XCR::Result hudStatus = GetHudStatus();  // a-dannat Moved up so that the recording logic can use it.

			// a-dannat Added the check for System XUI and text sequence player so that during playback, the keystrokes
			//  are passed back to the System App and then zero'ed out before being passed back to the Title App
#ifdef XAM_HOOK		
            if (hudStatus == XCR::Result::STATUS_ACTIVE)
            { 
                if (strcmp(get_component_name(m_player),"ports") != 0 ) //seqplay") == 0)
                { 
					if (FALSE == XAutomationpInputSetState(port, &(m_ports[port].GamepadState.Gamepad)))
                    {
                        OutputDebugStringA("XAutomationInputSetState failed.  If the HUD is active it did not receive the intended input.");
                    }

					memset(pKeystroke,0,sizeof(XINPUT_KEYSTROKE));
					xresult = ERROR_EMPTY;
				}
            }
#endif

			return xresult;
		}


		// ----------------------------------------------------------------
		// Constant default component names
		// ----------------------------------------------------------------

		// Name of the default recorder.
		const LPSTR Core::s_rec_component_name = "rec";

		// Name of the default player.
		const LPSTR Core::s_play_component_name = "play";

			
			
			
		// ----------------------------------------------------------------
		// Implementation
		// ----------------------------------------------------------------

		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		// Static initialization
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


		// List of components to initialize.
		// This static array is defined in XCR.cpp.
		////static Core::ComponentInitialize s_initial_components[];


		// Check to see if initialization has occurred
		void Core::initialize_if_necessary()
		{
			static bool fInitialized = false;

			if (!fInitialized)
			{
				init();
				fInitialized = true;
			}

			// MJMXAM: if the notify listener hasn't been created, try to create it now.
            if (0 == m_hud_notification_handler)
            {
                m_hud_notification_handler = XNotifyCreateListener(XNOTIFY_SYSTEM);
            }
		}

		// Perform initialization.
		void Core::init()
		{
			// Initialize our default set of components.
			int i = 0;
			while (s_initial_components[i].name)
			{
				ConnectComponent(s_initial_components[i].name, s_initial_components[i].component);
				i++;
			};

			m_tick_count = GetTickCount();
		}




		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		// Input device emulation management
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

		// Pick up on any device insertions or removals
		// RWBFIX: Check for bad port numbers
		void Core::record_special_events_if_necessary(DWORD port)
		{
			if (m_ports[port].Changed)
			{
				// Clear change flag
				m_ports[port].Changed = false;

				// Record insertion or removal as appropriate
				if (m_recorder)
				{
					DWORD type = m_ports[port].GetDeviceType();
					if (XINPUT_DEVTYPE_GAMEPAD == type)
					{
						m_recorder->RecordGamepadConnect(port, m_ports[port].GetCapabilities(), m_ports[port].GamepadState.Gamepad);
					}
					else if (XINPUT_DEVTYPE_USB_KEYBOARD == type)
					{
						m_recorder->RecordKeyboardConnect(port, m_ports[port].KeyboardState);
					}
					else
					{
						m_recorder->RecordDeviceDisconnect(port);
					}
				}
			}
            if (m_recorder)
            {
                //JKBXAM 9/25 - changed type to get actual value not just bool...
                XCR::Result::ResultCode previousResultCode = m_is_hud_active;
                if (XCR::Result::STATUS_UNKNOWN != m_is_hud_active)
                {
                    GetHudStatus();
                    if (m_is_hud_active != previousResultCode)
                    {
                        if (XCR::Result::PORT_UNKNOWN != m_hud_user)
                        {
                            m_recorder->RecordToggleHud(m_dw_hud_user);
                        }
                        else
                        {
                            m_recorder->RecordToggleHud(0);
                        }
                    }
                }
            }
		}


		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		// Component management
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


		// Find a component.
		//	parameters:
		//		name : the name of the component to retrieve
		//	returns the a pointer to the IComponent interface or NULL if no such component exists.
		inline IComponent *Core::get_component(LPCSTR name) const
		{
			// Is it one of the default names?
			if (lstrcmpiA(name, s_rec_component_name) == 0)
			{
				return (IComponent *) m_recorder;
			}
			else if (lstrcmpiA(name, s_play_component_name) == 0)
			{
				return (IComponent *) m_player;
			}
			// Must be in the component list somewhere.
			ComponentMap::const_iterator it = m_components.find(name);
			return (it == m_components.end()) ? NULL : it->second;
		}


		// Set a component.  Add component to list or change an existing one.
		//	parameters:
		//		name : the name to give this component
		//		component : the component to add
		inline void Core::set_component(LPCSTR name, IComponent *component)
		{
			m_components[name] = component;
			if (component->GetComponentType() == IComponent::COMPONENT_CONTROL)
			{
				m_control_components[name] = component;
			}
			if (component->GetComponentType() == IComponent::COMPONENT_LOG)
			{
				m_logger_components[name] = component;
			}
		}


		// Remove a component.
		//	parameters:
		//		name : name of the component to remove
		inline void Core::remove_component(LPCSTR name)
		{
			// First remove it from being a current one of this type.
			IComponent *component = get_component(name);
			switch (component->GetComponentType())
			{
			case IComponent::COMPONENT_REC:
				// Remove as current recorder.
				if ((IComponent *) m_recorder == component)
				{
					m_recorder = NULL;
				}
				break;
			case IComponent::COMPONENT_PLAY:
				// Remove as current player.
				if ((IComponent *) m_player == component)
				{
					m_recorder = NULL;
				}
				break;
			case IComponent::COMPONENT_CONTROL:
				// Remove from controller list.
				m_control_components.erase(name);
				break;
			case IComponent::COMPONENT_LOG:
				m_logger_components.erase(name);
				break;
			}
			// Remove from the list of components.
			m_components.erase(name);
		}

		// Find a component name.
		LPCSTR Core::get_component_name(IComponent* cmp)
		{
			for (ComponentMap::const_iterator it = m_components.begin(); it != m_components.end(); it++)
			{
				if (it->second == cmp)
				{
					return (it->first).c_str();
				}
			}

			return "";
		}

		// Find a component index.
		int Core::get_component_index(IComponent* cmp)
		{
			int index = 0;
			for (ComponentMap::const_iterator it = m_components.begin(); it != m_components.end(); it++)
			{
				if (it->second == cmp)
				{
					return index;
				}

				index++;
			}

			return -1;
		}

		//JKBNEW: this is really inefficient, need to revisit...!!!
		IComponent* Core::get_component(int index) const
		{
			int i = 0;
			for (ComponentMap::const_iterator it = m_components.begin(); it != m_components.end(); it++)
			{
				if (i == index)
					return it->second;

				i++;
			}

			return NULL;
		}
			



		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		// Processing
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


		// Do any necessary per-frame processing.
		// Called in every function that may be called in the main loop.
		void Core::frame_check()
		{
            if (m_recorder)
            {
                m_recorder->FrameAdvance();
            }

			// Note that this function is going to be called more than once per frame, because multiple functions call it.
			DWORD new_tick_count = GetTickCount();
			if (new_tick_count > m_tick_count + 16)
			{
				m_tick_count = new_tick_count;
				m_frame_count++;
				do_work();
			}
		}


		// Gives all controllers an opportunity to do work.
		void Core::do_work()
		{
			for (ComponentMap::iterator it = m_control_components.begin(); it != m_control_components.end(); it++)
			{
				((IController *) it->second)->DoWork(m_frame_count, m_tick_count);
			}
		}
	}
}


#endif