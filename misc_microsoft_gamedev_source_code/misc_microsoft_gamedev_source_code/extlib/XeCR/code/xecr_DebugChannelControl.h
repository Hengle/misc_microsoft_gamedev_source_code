//	DebugChannelControl.h : XCR control component that accepts commands over the debug channel.
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
#include <XbDm.h>




namespace XCR
{
	using namespace ControlUnit;

	// ================================================================
	//	DebugChannelControl class
	//
	//	This is an XCR Controller that receives commands over the debug channel and processes them.
	//	It uses a debug channel threaded command processor to handle the commands.
	//	If multiple connections to the same command processor are opened, each request will be
	//		completely processed before any other command is processed.
	//	Only a single instance of this control should be created and connected to the XCR Control Unit.
	//	The last instance that is initialized is the one that will be called on to process the commands received.
	// ================================================================
	class DebugChannelControl :
		public IController
	{
	public:
		DebugChannelControl(void);
		~DebugChannelControl(void);


		// ----------------------------------------------------------------
		// IComponent interface
		// ----------------------------------------------------------------

		// Set a property for this component.
		//	parameters:
		//		name : name of property to set
		//		value : value to set property to
		//	returns false on failure
		virtual XCR::Result SetStringProperty(LPCSTR name, LPCSTR value1, LPCSTR value2 = NULL);

		// Start this component.
		//	returns result code
		virtual XCR::Result Start();

		// Stop this component.
		//	returns result code
		virtual XCR::Result Stop();


		// Retrieve helpstring.
		virtual const char *GetHelpString()
		{
			static const char *helpstring = "Accepts commands from the debug channel and controls other components.";
			return helpstring;
		}

		// ----------------------------------------------------------------
		// IController interface
		// ----------------------------------------------------------------

		// Initialize this controller.
		//	parameters:
		//		pXCR : a pointer to the XCR Control Unit interface this controller will use to issue commands.
		//	returns false if an error occurred.
		virtual XCR::Result Initialize(IControllerInterface *pXCR);

		// Do one frame worth of work.
		//	parameters:
		//		dwFrameNum : the frame number
		//		dwTimeMs : the time in ms
		virtual void DoWork(DWORD dwFrameNum, DWORD dwTimeMs);

		// Notifies this controller of a game state change.
		//	parameters:
		//		szState : a string describing the state change
		virtual void NotifyState(LPCSTR szState);


	protected:
		// ----------------------------------------------------------------
		// Implementation
		// ----------------------------------------------------------------

		// XCR Control Unit interface.  Use this to command the other components.
		IControllerInterface *m_control_unit;

		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		// Debug channel
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

		// Registers the debug channel.
		void	register_debug_channel();

		// XCR debug channel command processor.  Called by the system when the PC sends a command to us.
		// Since it is registered as threaded, this handler may be called in multiple threads.
		static	HRESULT __stdcall XCR_debug_channel_cmd_proc(LPCSTR lpszCommand, LPSTR lpszResponse, DWORD cchResp, PDM_CMDCONT pdmcc);

		// Critical section used to make debug channel processors operate one at a time.
		static CRITICAL_SECTION s_XCR_debug_channel_critical_section;

		// Flag indicating that the critical section was initialized.
		static bool s_XCR_debug_channel_critical_section_initialized;

		// A pointer to the DebugChannelControl object that the debug channel command processor should use.
		static DebugChannelControl *s_XCR_debug_channel_control;


		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		// Command processing
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

		// The member function that will be called by the debug channel command processor to process the command.
		// This function does not have to be reentrant as only one debug channel command processor will call it at a time.
		// However, it will be called from the debug channel command processor's thread.
		HRESULT process_command(LPCSTR command, LPSTR response, DWORD response_size);

		// Structure to hold data for the command we are working on, to be synchronized.
		struct CommandData
		{
			LPCSTR command;
			LPSTR response;
			DWORD response_size;
		};

		// Data for the command we are working on.
		CommandData m_command_data;

		// Synchronizes the response.
		HANDLE m_response_ready_event;

		// Synchronizes the command.
		bool m_command_ready;

		// Break a command into individual arguments.
		// Arguments are separated by white space, or surrounded by quotation marks.
		bool parse_command(LPSTR command, LPSTR *argv, PDWORD argc);

		// Check number of arguments and return an error code.
		XCR::Result check_arg_count(DWORD actual, DWORD required);

		// Version of strcpy that always terminates with a nul.
		size_t copy_string(LPSTR destination_buffer, LPCSTR source_buffer, size_t size);

		XCR::Result self_test();

		// Command information.
		struct Command
		{
			enum CommandType
			{
				Hello, Status, Version, Help,
				Select, Set, Signal, Start, Stop, GetStatus,
				StartRecording, StopRecording, StartPlayback, StopPlayback,
				SaveState, ResumeState,
				DumpEvents, SelfTest, GetHudStatus, ToggleHud, GetHudUser, 
                GetActivePlayer, GetActiveRecorder, GetStatus2, MultipleCommand, 
                SetDefault
			}
			Type;
			const char *Name;
			const char *Helpstring;
		};
		const static Command s_commands[];

		// Supporting function to find a command matching a specified string. Returns
		// NULL if not found. This function replaces the previous use of the STL lower_bound
		// template as there appear to be Xenon-specific STL incompatibilities.
		const Command* find_command(LPCSTR command_string, const Command* commands_end);

        void execute_command(LPSTR command_buffer, XCR::Result &result, bool &custom_result_message);

        void build_signal_message(LPSTR signal_message, int signal_message_length, LPSTR* argv, int message_index, int argc);
	};

}