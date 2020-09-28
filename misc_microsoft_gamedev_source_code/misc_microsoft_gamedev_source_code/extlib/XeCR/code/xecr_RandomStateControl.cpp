//	RandomStateControl.cpp : State-Based Random Test Scenario Executor.
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

#include "xecr_RandomStateControl.h"
#include <fstream>

namespace XCR
{
	// ================================================================
	//	RandomStateControl
	//
	//	A controller that does random state-based testing.
	// ================================================================

	extern int xcr_rand();
	extern void xcr_srand(unsigned int seed);

	RandomStateControl::RandomStateControl(void) :
		m_running(false),
		m_current_state(0),
		m_current_transition(-1),
		m_states_length(0),
		m_curRSFile(""),
		m_random_seed(0)
	{
		m_curAction = new Action(0,0);
	}


	RandomStateControl::~RandomStateControl(void)
	{
		//JKBNEW: state serialization support:
		delete m_curAction;
	}




	// ----------------------------------------------------------------
	// IComponent interface
	// ----------------------------------------------------------------


	// Notify this component it is being selected.
	// Added Implementation: RWB Fix Bug #349
	XCR::Result RandomStateControl::SelectComponent()
	{
		// Selection does nothing, but we return Success just the same.
		return XCR::Result::SUCCESS;
	}

	// Set a property for this component.
	XCR::Result RandomStateControl::SetStringProperty(LPCSTR name, LPCSTR value1, LPCSTR value2)
	{
		if (lstrcmpiA(name, "file") == 0)
		{
			if (value2 != NULL) return XCR::Result::ERROR_TOO_MANY_ARGUMENTS;
			if (value1 == NULL) return XCR::Result::ERROR_TOO_FEW_ARGUMENTS;
			if (m_running) return XCR::Result::ERROR_CANT_SET_PROPERTY_WHILE_USING;

			//JKBNEW: state serialization support: store file name in m_input:
			m_curRSFile = value1;

			// Try to open and read file.
			HANDLE handle = CreateFile(value1, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
			if (handle == INVALID_HANDLE_VALUE)
			{
				return Result::ERROR_CANNOT_OPEN_FILE;
			}
			XRSFileHeader header;
			DWORD bytes_read = 0;
			BOOL read_result = FALSE;
			DWORD file_size = GetFileSize(handle, NULL);
			if (file_size != -1)
			{
				// Read header and verify.
				read_result = ReadFile(handle, &header, sizeof (header), &bytes_read, NULL);
				if (read_result && bytes_read == sizeof (header) && header.IsValid())
				{
					// Allocate block for XRS resource and load it.
					file_size -= sizeof (header);
					delete [] m_xrs_data;
					m_xrs_data = NULL;
					m_xrs_data = new (std::nothrow) BYTE[file_size];
					if (m_xrs_data == NULL)
					{
						CloseHandle(handle);
						return XCR::Result::ERROR_OUT_OF_MEMORY;
					}
					BYTE *xrs_file_base = m_xrs_data - sizeof (header);
					read_result = ReadFile(handle, m_xrs_data, file_size, &bytes_read, NULL);
					if (read_result && bytes_read == file_size)
					{
						// Set pointers into data.
						m_states = (State *) (xrs_file_base + header.StatesOffset);
						m_choices = (Choice *) (xrs_file_base + header.ChoicesOffset);
						m_actions = (Action *) (xrs_file_base + header.ActionsOffset);
						m_strings = (char *) (xrs_file_base + header.StringsOffset);
						m_notifications = (Notification *) (xrs_file_base + header.NotificationsOffset);
						m_notifications_length = header.NotificationsLength;
						m_states_length = (header.ChoicesOffset - header.StatesOffset) / sizeof (State);
						m_initial_state = header.InitialState;
					}
				}
			}

			CloseHandle(handle);
			
			// Clean up failures.
			if (!read_result)
			{
				delete [] m_xrs_data;
				m_xrs_data = NULL;
				return XCR::Result::ERROR_CANNOT_READ_FILE;
			}

			return XCR::Result::SUCCESS;
		}
		else if (lstrcmpiA(name, "notify") == 0)
		{
			if (value2 != NULL) return XCR::Result::ERROR_TOO_MANY_ARGUMENTS;
			if (value1 == NULL) return XCR::Result::ERROR_TOO_FEW_ARGUMENTS;
			m_control_unit->LogMessage("RS: Set property state notification.\r\n");
			NotifyState(value1);
			return XCR::Result::SUCCESS;
		}
		else if (lstrcmpiA(name, "state") == 0)
		{
			if (value2 != NULL) return XCR::Result::ERROR_TOO_MANY_ARGUMENTS;
			if (value1 == NULL) return XCR::Result::ERROR_TOO_FEW_ARGUMENTS;
			if (m_xrs_data == NULL) return XCR::Result::ERROR_INVALID_FILE;
			for (state_index i = 0; i < m_states_length; i++)
			{
				if (lstrcmpiA(value1, m_strings + m_states[i].NameString) == 0)
				{
					m_current_state = i;
					m_control_unit->LogMessage("RS: Set property jump to state: ");
					m_control_unit->LogMessage(m_strings + m_states[m_current_state].NameString);
					m_control_unit->LogMessage("\r\n");
					break;
				}
			}
			return XCR::Result::SUCCESS;
		}

		// Set random number seed to initiate playback. 0 means use clock.
		else if (lstrcmpiA(name, "seed") == 0)
		{
			// Translate value1 into a number and set that as the seed.
			m_random_seed = atol(value1);
			return XCR::Result::SUCCESS;
		}

		return XCR::Result::ERROR_INVALID_PROPERTY;
	}

	// Start this component.
	//	returns result code
	XCR::Result RandomStateControl::Start()
	{
		XCR::Result result = XCR::Result::SUCCESS;

		if (m_running)
		{
			return Result::ERROR_OPERATION_IN_PROGRESS;
		}
		
		// If no file was specifed, return an invalid property error: RWB Fix Bug #351
		if (0 == m_curRSFile.length())
		{
			return XCR::Result::ERROR_INVALID_PROPERTY;
		}

		if (m_xrs_data == NULL)
		{
			return XCR::Result::ERROR_CANNOT_OPEN_FILE;
		}

		m_running = true;

		// Initialize random number seed
		if (0 == m_random_seed)
			xcr_srand(GetTickCount());
		else
			xcr_srand(m_random_seed);

		m_control_unit->LogMessage("RS: Started.\r\n");
		// Find the last state we were notified about and set the current state to that, or use default.
		state_index notify_start_state = get_notify_state(m_current_notify_state.c_str());
		if (notify_start_state >= 0)
		{
			m_current_state = notify_start_state;
			m_control_unit->LogMessage("RS: Initial state (from notification string: ");
			m_control_unit->LogMessage(m_current_notify_state.c_str());
			m_control_unit->LogMessage("): ");
		}
		else
		{
			m_current_state = m_initial_state;
			m_control_unit->LogMessage("RS: Initial state (specified by file): ");
		}
		m_control_unit->LogMessage(m_strings + m_states[m_current_state].NameString);
		m_control_unit->LogMessage("\r\n");

		m_current_action_executor = NULL;
		m_current_transition = -1;

		return Result::SUCCESS;
	}


	// Stop this component.
	//	returns result code
	XCR::Result RandomStateControl::Stop()
	{
		// Stop currently playing item.
		if (m_current_action_executor)
		{
			m_current_action_executor->DoWork(m_control_unit, true);
		}

		m_running = false;

		m_control_unit->LogMessage("RS: Stopped.\r\n");

		return XCR::Result::SUCCESS;
	}


	// Retrieve status.
	XCR::Result RandomStateControl::GetStatus()
	{
		return m_running
			? XCR::Result::STATUS_PLAYING
			: XCR::Result::STATUS_IDLE;
	}

	//JKBNEW:
	XCR::Result RandomStateControl::SaveState(DirectDiskOutputStream* ddos)
	{
		//???
		// ddos->Write((LPVOID)this, (DWORD)sizeof(this));

		// Write whether we're running or not:
		if (!ddos->Write((LPVOID)&m_running, (DWORD)sizeof(bool)))
			return XCR::Result::ERROR;

		// Don't write anything else if we're not running RS:
		if (!m_running) 
			return XCR::Result::SUCCESS;

		// Write current state:
		if (!ddos->Write((LPVOID)&m_current_state, (DWORD)sizeof(state_index)))
			return XCR::Result::ERROR;

		// Write current transition:
		if (!ddos->Write((LPVOID)&m_current_transition, (DWORD)sizeof(choice_index)))
			return XCR::Result::ERROR;

		// Write notify string length:
		int len = m_current_notify_state.length();//sizeof(m_current_notify_state);
		if (!ddos->Write((LPVOID)&len, (DWORD)sizeof(int)))
			return XCR::Result::ERROR;

		// Now Write notify string:
		if (len > 0)
		{
			if (!ddos->Write((LPVOID)m_current_notify_state.data(), (DWORD)len))
				return XCR::Result::ERROR;
		}

		// m_input - write file name length: 
		// THIS IS WHERE i NEED TO RELOAD THAT FILE USING SETSTRINGFILEPROPERTY!
		len = m_curRSFile.length();//sizeof(fn);
		if (!ddos->Write((LPVOID)&len, (DWORD)sizeof(int)))
			return XCR::Result::ERROR;

		// Now write file name:
		if (len > 0)
		{
			if (!ddos->Write((LPVOID)m_curRSFile.data(),(DWORD)len))
				return XCR::Result::ERROR;
		}

		// write current action executor:
		bool curActionExecNotNull = false;
		if (m_current_action_executor)
		{
			curActionExecNotNull = true;
			if (!ddos->Write((LPVOID)&curActionExecNotNull, (DWORD)sizeof(bool)))
				return XCR::Result::ERROR;

			if (!ddos->Write((LPVOID)m_curAction, (DWORD)sizeof(Action)))
				return XCR::Result::ERROR;

			//if (!ddos->Write((LPVOID)m_current_action_executor, (DWORD)sizeof(ActionExecutor)))
			//	return XCR::Result::ERROR;
		}
		else
		{
			if (!ddos->Write((LPVOID)&curActionExecNotNull, (DWORD)sizeof(bool)))
				return XCR::Result::ERROR;
		}

		// Write states length:
		if (!ddos->Write((LPVOID)&m_states_length, (DWORD)sizeof(long)))
			return XCR::Result::ERROR;

		// Write notifications length:
		if (!ddos->Write((LPVOID)&m_notifications_length, (DWORD)sizeof(long)))
			return XCR::Result::ERROR;

		// Write initial state:
		if (!ddos->Write((LPVOID)&m_initial_state, (DWORD)sizeof(state_index)))
			return XCR::Result::ERROR;

		return XCR::Result::SUCCESS;
	}

	//JKBNEW:
	XCR::Result RandomStateControl::ResumeState(DirectDiskInputStream* ddis)
	{
		// Read whether we're running or not, BUT ASSIGN LATER 
		//   AFTER SETTING THE PROPERTY FILENAME...
		bool isRunning = false;
		if (!ddis->Read((LPVOID)&isRunning, (DWORD)sizeof(bool)))
			return XCR::Result::ERROR;

		// Don't read anything else if we're not running RS:
		if (!isRunning) 
			return XCR::Result::SUCCESS;
		
		// Read current state:
		if (!ddis->Read((LPVOID)&m_current_state, (DWORD)sizeof(state_index)))
			return XCR::Result::ERROR;

		// Read current transition:
		if (!ddis->Read((LPVOID)&m_current_transition, (DWORD)sizeof(choice_index)))
			return XCR::Result::ERROR;

		// Read notify string length:
		int len = 0;
		if (!ddis->Read((LPVOID)&len, (DWORD)sizeof(int)))
			return XCR::Result::ERROR;

		// Now Read notify string:
		if (len > 0)
		{
			char* tempStr = new char[len+1];
			tempStr[len] = '\0';
			// Read m_data string:
			if (!ddis->Read((LPVOID)tempStr,(DWORD)len))
				return XCR::Result::ERROR;

			m_current_notify_state = tempStr;

			delete tempStr;
		}

		// m_curRSFile - Read file name length: 
		len = 0;
		if (!ddis->Read((LPVOID)&len, (DWORD)sizeof(int)))
			return XCR::Result::ERROR;

		// Now Read file name:
		if (len > 0)
		{
			char* tempStr = new char[len+1];
			tempStr[len] = '\0';
			// Read m_data string:
			if (!ddis->Read((LPVOID)tempStr,(DWORD)len))
				return XCR::Result::ERROR;

			m_curRSFile = tempStr;

			delete [] tempStr;
		}

		// reload state file
		if (m_curRSFile != "")
			SetStringProperty("file", m_curRSFile.c_str());
		m_running = isRunning;

		// Read current action executor:
		bool curActionExecNotNull = false;
		if (!ddis->Read((LPVOID)&curActionExecNotNull, (DWORD)sizeof(bool)))
			return XCR::Result::ERROR;

		if (curActionExecNotNull)
		{
			if (!ddis->Read((LPVOID)m_curAction, (DWORD)sizeof(Action)))
				return XCR::Result::ERROR;

			if (m_curAction)
				m_current_action_executor = get_action_executor(*m_curAction);

			//if (!ddis->Read((LPVOID)m_current_action_executor, (DWORD)sizeof(ActionExecutor)))
			//	return XCR::Result::ERROR;
		}

		// Read states length:
		if (!ddis->Read((LPVOID)&m_states_length, (DWORD)sizeof(long)))
			return XCR::Result::ERROR;

		// Read notifications length:
		if (!ddis->Read((LPVOID)&m_notifications_length, (DWORD)sizeof(long)))
			return XCR::Result::ERROR;

		// Read initial state:
		if (!ddis->Read((LPVOID)&m_initial_state, (DWORD)sizeof(state_index)))
			return XCR::Result::ERROR;

		return XCR::Result::SUCCESS;
	}


	// ----------------------------------------------------------------
	// IController interface
	// ----------------------------------------------------------------

	// Initialize this controller.
	//	parameters:
	//		pXCR : a pointer to the XCR Control Unit interface this controller will use to issue commands.
	//	returns false if an error occurred.
	XCR::Result RandomStateControl::Initialize(IControllerInterface *pXCR)
	{
		m_control_unit = pXCR;

		// Load state-test.ini if possible.
		//SetStringProperty("file", "state-test.ini");

		return XCR::Result::SUCCESS;
	}


	// Do one frame worth of work.
	//	parameters:
	//		dwFrameNum : the frame number
	//		dwTimeMs : the time in ms
	void RandomStateControl::DoWork(DWORD dwFrameNum, DWORD dwTimeMs)
	{
		// Only do anything if we are running.
		if (m_running)
		{
			// If we are playing an action, keep playing it until it's done.
			if (m_current_action_executor)
			{
				bool more_to_do = m_current_action_executor->DoWork(m_control_unit);
				if (!more_to_do)
				{
					m_current_action_executor = NULL;
					m_control_unit->LogMessage("RS: Action complete.\r\n");
					// Transition completes now.  Go to next state, if one was specified.
					state_index target_state = m_choices[m_current_transition].State;
					if (target_state != -1)
					{
						m_current_state = target_state;
						m_control_unit->LogMessage("RS: Transition to state: ");
						m_control_unit->LogMessage(m_strings + m_states[target_state].NameString);
						m_control_unit->LogMessage("\r\n");
					}
					// This transition is done.
					m_current_transition = -1;
				}
			}
			else
			{
				// Is it time for a new random selection?
				if (m_current_transition == -1)
				{
					// Get a new random transition to do.
					state_index next_choice = random_choice(m_current_state);
					while (next_choice != -1 && m_choices[next_choice].Action == -1)
					{
						next_choice = random_choice(m_choices[next_choice].State);
					}
					m_current_transition = next_choice;
					// Get an executor for its action.  It will start working in the next DoWork().
					if (m_current_transition != -1)
					{
						Action &action = m_actions[m_choices[m_current_transition].Action];
						m_current_action_executor = get_action_executor(action);

						//JKBNEW: state serialization support:
						m_curAction = &m_actions[m_choices[m_current_transition].Action];

						m_control_unit->LogMessage("RS: Starting action: ");
						static const char *action_types[] = { "play", "playback", "sequence" };
						m_control_unit->LogMessage(action_types[action.Type]);
						m_control_unit->LogMessage("(\"");
						m_control_unit->LogMessage (m_strings + action.DataString);
						m_control_unit->LogMessage("\")\r\n");
					}
				}
				else
				{
					// We are still working on this transition.
					// Actually we should never come here.
					_ASSERTE(false);
				}
			}
		}
	}


	// Notifies this controller of a game state change.
	//	parameters:
	//		szState : a string describing the state change
	void RandomStateControl::NotifyState(LPCSTR szState)
	{
		m_control_unit->LogMessage("RS: State notification: ");
		m_control_unit->LogMessage(szState);
		if (m_running)
		{
			// See if the state we're being told about exists in our list, and set the current state to that.
			state_index new_state = get_notify_state(szState);
			if (new_state >= 0)
			{
				m_current_state = new_state;
				m_control_unit->LogMessage("\nRS: Notification state change: ");
				m_control_unit->LogMessage(m_strings + m_states[m_current_state].NameString);
			}
		}
		// Save this for later, so even if we're not running we remember what state we're in.
		m_current_notify_state = szState;
		m_control_unit->LogMessage("\r\n");
	}



	// Make a random choice from those available to the current state.
	// Returns -1 if no choice can be made.
	choice_index RandomStateControl::random_choice(state_index state)
	{
		// Add up all weights for options we have.
		float weight_total = 0;
		Choice *choices = m_choices + m_states[state].Choices;
		long choice_count = m_states[state].ChoiceCount;
		for (int i = 0; i < choice_count; i++)
		{
			weight_total += choices[i].Weight;
		}

		// If there's not enough weight, we can't choose anything.
		if (weight_total <= 0)
		{
			return -1;
		}

		// Pick a random number between 0 and the sum of the weights.
		float rand_choice = ((float) xcr_rand() / RAND_MAX) * weight_total;

		// Find which choice this belonged to and return it.
		weight_total = 0;
		for (int i = 0; i < choice_count; i++)
		{
			weight_total += choices[i].Weight;
			if (rand_choice <= weight_total)
			{
				return m_states[state].Choices + i;
			}
		}

		return -1;
	}


	// Retrieve a pointer to an ActionExecutor prepared to execute this action.
	RandomStateControl::ActionExecutor *RandomStateControl::get_action_executor(Action &action)
	{
		ActionExecutor *executor;
		static PlaybackExecutor playback;
		static DiskPlaybackExecutor disk;
		static TextPlaybackExecutor text;
		switch (action.Type)
		{
		case 0: executor = &playback; break;
		case 1: executor = &disk; break;
		case 2: executor = &text; break;
		default: return NULL;
		}
        executor->Initialize(m_strings + action.DataString);
		return executor;
	}


	// Retrieves state given a notify code string.
	// Returns state number or -1 if no matching state found.
	// TODO: OPTIMIZE: Linear search could be binary if the strings are sorted by compiler.
	state_index RandomStateControl::get_notify_state(LPCSTR notifyString)
	{
		for (int i = 0; i < m_notifications_length; i++)
		{
			if (lstrcmpiA(m_strings + m_notifications[i].NotifyString, notifyString) == 0)
			{
				return m_notifications[i].State;
			}
		}
		return -1;
	}

}


#endif