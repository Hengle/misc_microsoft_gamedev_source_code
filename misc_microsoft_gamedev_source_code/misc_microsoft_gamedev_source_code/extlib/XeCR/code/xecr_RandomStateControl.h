//	RandomStateControl.h : State-Based Random Test Scenario Executor.
//
//	Created 2004/10/06 Rich Bonny    <rbonny@microsoft.com>
//
//  - based on original XCR code:
//	Created 2003/05/05 David Eichorn <deichorn@microsoft.com>
//     and modified by Jon Burns     <jburns@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2004 Microsoft Corp.  All rights reserved.




#pragma once

#include "xecr_Interfaces.h"

#include <string>
#include <vector>
#include <map>

#include "xutil_InputStream.h"
#include "xecr_RandomStateSystem.h"

namespace XCR
{
	using namespace ControlUnit;
	using namespace RandomStateSystem;

	// ================================================================
	//	RandomStateControl
	//
	//	A controller that does random state-based testing.
	// ================================================================
	class RandomStateControl :
		public IController
	{
	public:
		RandomStateControl(void);
		~RandomStateControl(void);
		
		// ----------------------------------------------------------------
		// IComponent interface
		// ----------------------------------------------------------------
		
		// Notify this component it is being selected.
		// Added Implementation: RWB Fix Bug #349
		virtual XCR::Result SelectComponent();

		// Set a property for this component.
		virtual XCR::Result SetStringProperty(LPCSTR name, LPCSTR value1, LPCSTR value2 = NULL);

		// Start this component.
		virtual XCR::Result Start();

		// Stop this component.
		virtual XCR::Result Stop();

		// Retrieve status.
		virtual XCR::Result GetStatus();

		// Retrieve helpstring.
		virtual const char *GetHelpString()
		{
			static const char *helpstring = "Does random state-based testing.\vProperties:\v  file - a state definition file compiled with RandomStateCompiler\v  notify - a notify string\v  state - a state to switch to\v  seed 0-4294967295\vSee documentation for more details.";
			return helpstring;
		}

		// ----------------------------------------------------------------
		// IController interface
		// ----------------------------------------------------------------

		// Initialize this controller.
		virtual XCR::Result Initialize(IControllerInterface *pXCR);

		// Do one frame worth of work.
		virtual void DoWork(DWORD dwFrameNum, DWORD dwTimeMs);

		// Notifies this controller of a game state change.
		virtual void NotifyState(LPCSTR szState);

		//JKBNEW:
		virtual XCR::Result SaveState(DirectDiskOutputStream* ddos);
		virtual XCR::Result ResumeState(DirectDiskInputStream* ddis);

		// ----------------------------------------------------------------
		// Implementation
		// ----------------------------------------------------------------

	protected:

		// Base class for all classes that can execute an action.
		class ActionExecutor
		{
		public:
			// Initializes the action using the supplied data.
			virtual void Initialize(char *data) = 0;

			// Does the work for the action.
			// returns false if finished.
			virtual bool DoWork(IControllerInterface *control_unit, bool should_stop = false) = 0;
		};
			

		// Base class for input playback action executors.
		class PlaybackExecutor :
			public ActionExecutor
		{
		public:
			PlaybackExecutor()
			{
				m_component_name = "play";
				m_primary_data_property_name = "file";
			}

			// Initializes the action using the supplied data.
			void Initialize(char *data)
			{
				m_primary_data = data;
				m_started = false;
			}

			// Does the work for the action.
			// returns false if finished.
			bool DoWork(IControllerInterface *control_unit, bool should_stop)
			{
				if (!m_started && !should_stop)
				{
					control_unit->SelectComponent(m_component_name, false);
					control_unit->SetComponentStringProperty(m_component_name, m_primary_data_property_name, m_primary_data.c_str());
					control_unit->StartComponent(m_component_name, false);
					m_started = true;
					return true;
				}
				if (should_stop)
				{
					control_unit->StopComponent(m_component_name);
					return false;
				}
				return !(control_unit->GetComponentStatus(m_component_name) == XCR::Result::STATUS_IDLE);
			}
		
		protected:
			// Whether we've started playing yet or not.
			bool m_started;
			// Filename/text to play;
            std::string m_primary_data;
			// Component to use in playing.
			char *m_component_name;
			// Name of property to set primary data to.
			char *m_primary_data_property_name;
		};


		// Plays back a file from disk.
		class DiskPlaybackExecutor :
			public PlaybackExecutor
		{
		public:
			DiskPlaybackExecutor()
			{
				m_component_name = "bufplay";
			}
		};


		// Plays back a text sequence.
		class TextPlaybackExecutor :
			public PlaybackExecutor
		{
		public:
			TextPlaybackExecutor()
			{
				m_component_name = "seqplay";
			}
		};


		// The control unit.
		IControllerInterface *m_control_unit;

		// Are we running?
		bool m_running;
		
		// The current state we are in.
		state_index m_current_state;

		// The current transition we are effecting.
		choice_index m_current_transition;

		// Last state notify string seen.
        std::string m_current_notify_state;

		// The execution of the current action.
		ActionExecutor *m_current_action_executor;

		// Input stream used to load file.
		DirectDiskInputStream m_input;

		// Pointer to all data in the XRS file.
		BYTE *m_xrs_data;
		// Pointer to States table.
		State *m_states;
		// Number of states.
		long m_states_length;
		// Pointer to Choices table.
		Choice *m_choices;
		// Pointer to Actions table.
		Action *m_actions;
		// Pointer to Notifications table.
		Notification *m_notifications;
		// Length of notifications table.
		long m_notifications_length;
		// Initial state.
		state_index m_initial_state;

		// Pointer to string table.
		char *m_strings;

		// Retrieves state given a notify code string.
		// Returns state number or -1 if no matching state found.
		state_index get_notify_state(LPCSTR notifyString);

		// Retrieve a pointer to an ActionExecutor prepared to execute this action.
		// Returns NULL if it can't.
		ActionExecutor *get_action_executor(Action &action);

       	// Make a random choice from those available to the state.
		// Returns -1 if no choice can be made.
		choice_index random_choice(long state);

		// Random seed to initialize with
		unsigned int m_random_seed;

		//JKBNEW: state serialization support: store file name in m_input:
        std::string m_curRSFile;
		Action* m_curAction;
	};



}