//	DebugChannelControl.cpp : XCR control component that accepts commands over the debug channel.
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

#include "xecr_DebugChannelControl.h"
#include "xecr_core.h"
#include "xecr_EventDumper.h"

#include <crtdbg.h>
#pragma comment(lib, "XbDm.lib")
#include <algorithm>




namespace XCR
{
	using namespace ControlUnit;


	// ----------------------------------------------------------------
	// Miscellaneous defined constants
	// ----------------------------------------------------------------

	//name of our debug channel
	#define XCR_DEBUG_CHANNEL		"XCR"

	#define XCR_HELLO "hello..."

	// List of commands and their helpstrings.
	// If you add to this, you MUST put commands in ALPHABETICAL ORDER by command name.
	const DebugChannelControl::Command DebugChannelControl::s_commands[] =
	{
		{ Command::DumpEvents, "dumpevents",  "<full path> <full path> - rewrite recorded file to readable format" },
        { Command::GetActivePlayer, "getactiveplayer", "- returns currently selected player" },
        { Command::GetActiveRecorder, "getactiverecorder", "- returns currently selected recorder" },
        { Command::GetHudStatus, "gethudstatus", "- returns Unknown, Active, or Inactive" },
        { Command::GetHudUser, "gethuduser", "- returns Unknown, Active, or Inactive" },
        { Command::GetStatus, "getstatus",  "<component> - get current execution status of component" },
        { Command::GetStatus2, "getstatus2",  "<component> - get current execution status of component" },
		{ Command::Hello, "hello", " - check if XeCR is in your game.  Should respond with \"hello...\"" },
		{ Command::Help, "help", "[commands|components] - get help on commands or components" },
        { Command::MultipleCommand, "multiplecommand",  "executes multiple commands with one debug channel call, commands must be wrapped in quotes and space separated, this will only work with commands that return success because the first non-success result returns" },
		{ Command::ResumeState, "resumestate", "<fileName> - resume XCR state" },
		{ Command::SaveState, "savestate", "<fileName> - save current XCR state" },
		{ Command::Select, "select", "<component> - select a component" },
		{ Command::SelfTest, "selftest", "- test basic XeCR features" },
        { Command::Set, "set",  "<component> <property> [<index>] <value> - set a property" },
        { Command::SetDefault, "setdefault",  "<component> - restore a component to its default state" },
		{ Command::Signal, "signal", "<message> - signal the app with a message" },
		{ Command::Start, "start",  "<component> - start component recording/playback/action" },
		{ Command::StartPlayback, "startplayback", "<full path> - start current player, using full path (or text sequence)" },
		{ Command::StartRecording, "startrecording",  "<full path> - start current recorder, saving to full path" },
		{ Command::Stop, "stop", "<component> - stop component recording/playback/action" },
		{ Command::StopPlayback, "stopplayback",  "- stop current player" },
		{ Command::StopRecording, "stoprecording", "- stop current recorder" },
		{ Command::ToggleHud, "togglehud", "<index> - toggles the HUD on for this index if it's not active or off if it's active for this index" },
        { Command::Version, "version", "- check XCR version" }
	};



	// ----------------------------------------------------------------
	// Constructors / destructors
	// ----------------------------------------------------------------

	DebugChannelControl::DebugChannelControl(void) :
		m_control_unit(NULL),
		m_command_ready(false),
		m_response_ready_event(NULL)
	{
	}

	DebugChannelControl::~DebugChannelControl(void)
	{
		if (m_response_ready_event)
		{
			CloseHandle(m_response_ready_event);
		}
		if (s_XCR_debug_channel_critical_section_initialized)
		{
			DeleteCriticalSection(s_XCR_debug_channel_cmd_proc_critical_section);
		}
	}




	// ----------------------------------------------------------------
	// IComponent interface
	// ----------------------------------------------------------------


	// Set a property for this component.
	//	parameters:
	//		name : name of property to set
	//		value : value to set property to
	//	returns false on failure
	XCR::Result DebugChannelControl::SetStringProperty(LPCSTR name, LPCSTR value1, LPCSTR value2)
	{
		// We don't have any properties currently.
		return XCR::Result::ERROR_INVALID_PROPERTY;
	}


	// Start this component.
	//	returns result code
	XCR::Result DebugChannelControl::Start()
	{
		// We're always running!
		return XCR::Result::ERROR_ALREADY_RUNNING;
	}


	// Stop this component.
	//	returns result code
	XCR::Result DebugChannelControl::Stop()
	{
		// We don't really want you to stop us...
		return XCR::Result::ERROR_CANNOT_STOP_COMPONENT;
	}




	// ----------------------------------------------------------------
	// IController interface
	// ----------------------------------------------------------------


	// Initialize this controller.
	//	parameters:
	//		pXCR : a pointer to the XCR Control Unit interface this controller will use to issue commands.
	//	returns false if an error occurred.
	XCR::Result DebugChannelControl::Initialize(IControllerInterface *pXCR)
	{
		// TODO: Error checking.
		m_control_unit = pXCR;
		m_command_ready = false;
		m_response_ready_event = CreateEvent(NULL, FALSE, FALSE, NULL);
		_ASSERTE(m_response_ready_event != NULL);
		register_debug_channel();
		return XCR::Result::SUCCESS;
	}


	// Do one frame worth of work.
	//	parameters:
	//		dwFrameNum : the frame number
	//		dwTimeMs : the time in ms
	void DebugChannelControl::DoWork(DWORD dwFrameNum, DWORD dwTimeMs)
	{
		// Wait for notification that there is a command to process.
		// Since this is only processed when we poll it we don't want to use an event and have it sleep while waiting.
		// A polled variable is fine for synchronization.
		if (m_command_ready)
		{
			// Process command.
			XCR::Result result = XCR::Result::ERROR_UNKNOWN;
			bool custom_result_message = false;

			// Create a copy of the command data.
			LPSTR command_buffer = new (std::nothrow) CHAR[strlen(m_command_data.command) + 1];
			if (!command_buffer)
			{
				result = XCR::Result::ERROR_OUT_OF_MEMORY;
			}
			else
			{
				// Break into arguments.
				lstrcpyA(command_buffer, m_command_data.command);
                execute_command(command_buffer, result, custom_result_message);
			}

			// Done with command buffer.
			delete [] command_buffer;

			// Write response.
			if (!custom_result_message)
			{
				const char *result_message = NULL;
				result.GetMessage(result_message);
                copy_string(m_command_data.response, result_message, m_command_data.response_size);
			}
			
			m_command_ready = false;

			// Notify that response is ready.
			SetEvent(m_response_ready_event);
		}
	}

    void DebugChannelControl::execute_command(LPSTR command_buffer, XCR::Result &result, bool &custom_result_message)
    {
        LPSTR argv[128];
        DWORD argc = 128;
        parse_command(command_buffer, argv, &argc);
        const int command_index = 0;
        const int message_index = 1;
        const int component_index = 1;
        const int property_index = 2;
        const int filename_index = 1;
        const int value1_index = 3;
        const int value2_index = 4;

        LPCSTR file_property = "file";

        // Figure out what the command is.
        if (argc <= command_index)
        {
            // No command.
            result = XCR::Result::ERROR_UNKNOWN_COMMAND;
        }
        else
        {
            // Find the command we're trying to execute.
            const char *command_string = 0;
            if (argv[command_index][0] != 'x')
            {
                command_string = argv[command_index];
            }
            else
            {
                command_string = argv[command_index] + lengthof (XCR_DEBUG_CHANNEL);
            }
            const Command *commands_end = s_commands + lengthof (s_commands);
            const Command *command = find_command(command_string, commands_end);
            if (command == NULL)
            {
                result = XCR::Result::ERROR_UNKNOWN_COMMAND;
            }
            else
            {
                //JKBNEW: state serialization support:
                DirectDiskOutputStream ddos;
                DirectDiskInputStream ddis;

                switch (command->Type)
                {
                case Command::Set:
                    result = check_arg_count(argc, value1_index + 1);
                    if (result)
                    {
                        result = m_control_unit->SetComponentStringProperty(argv[component_index], argv[property_index], argv[value1_index]);
                    }
                    else
                    {
                        result = check_arg_count(argc, value2_index + 1);
                        if (result)
                        {
                            result = m_control_unit->SetComponentStringProperty(argv[component_index], argv[property_index], argv[value1_index], argv[value2_index]);
                        }
                    }
                    break;

                case Command::MultipleCommand:

                    if (1 == argc)
                    {
                        result = XCR::Result::ERROR_TOO_FEW_ARGUMENTS;
                    }
                    else
                    {
                        for (DWORD i=1; i<argc; ++i)
                        {
                            execute_command(argv[i], result, custom_result_message);
                            if (!result)
                            {
                                break;
                            }
                        }
                    }
                    break;

                case Command::Signal:
                    if (argc < message_index)
                    {
                        result = m_control_unit->ProcessSignal(0);
                    }
                    else
                    {
                        char signal_message[8192];
                        signal_message[0] = 0;
                        build_signal_message(signal_message, 8192, argv, message_index, argc);
                        result = m_control_unit->ProcessSignal(signal_message);
                    }
                    break;

                case Command::Start:
                    result = check_arg_count(argc, component_index + 1);
                    if (result) result = m_control_unit->StartComponent(argv[component_index]);
                    break;

                case Command::Stop:
                    result = check_arg_count(argc, component_index + 1);
                    if (result) result = m_control_unit->StopComponent(argv[component_index]);
                    break;

                case Command::Select:
                    result = check_arg_count(argc, component_index + 1);
                    if (result) result = m_control_unit->SelectComponent(argv[component_index]);
                    break;

                case Command::SetDefault:
                    result = check_arg_count(argc, component_index + 1);
                    if (result) result = m_control_unit->SetComponentDefault(argv[component_index]);
                    break;

                case Command::GetStatus:
                    result = check_arg_count(argc, component_index + 1);
                    if (result) result = m_control_unit->GetComponentStatus(argv[component_index]);
                    break;

                case Command::GetStatus2:
                    result = check_arg_count(argc, component_index + 1);
                    if (result) result = m_control_unit->GetComponentStatus2(argv[component_index]);
                    break;

                case Command::GetActivePlayer:
                    result = m_control_unit->GetActivePlayer();
                    break;

                case Command::GetActiveRecorder:
                    result = m_control_unit->GetActiveRecorder();
                    break;

                case Command::StartRecording:
                    result = check_arg_count(argc, filename_index + 1);
                    if (result) result = m_control_unit->SetComponentStringProperty(Core::s_rec_component_name, file_property, argv[filename_index]);
                    if (result) result = m_control_unit->StartComponent(Core::s_rec_component_name);
                    break;

                case Command::StopRecording:
                    result = check_arg_count(argc, command_index + 1);
                    if (result)	result = m_control_unit->StopComponent(Core::s_rec_component_name);
                    break;

                case Command::StartPlayback:
                    result = check_arg_count(argc, filename_index + 1);
                    if (result) result = m_control_unit->SetComponentStringProperty(Core::s_play_component_name, file_property, argv[filename_index]);
                    if (result) result = m_control_unit->StartComponent(Core::s_play_component_name);
                    break;

                case Command::StopPlayback:
                    result = check_arg_count(argc, command_index + 1);
                    if (result) result = m_control_unit->StopComponent(Core::s_play_component_name);
                    break;

                case Command::DumpEvents:
                    result = check_arg_count(argc, filename_index + 2);
                    if (result)
                    {
                        // Create a dump object and do the conversion
                        Dumper::EventDumper dump(argv[filename_index], argv[filename_index+1]);
                        result = dump.Translate();
                    }
                    break;

                case Command::Hello:
                    copy_string(m_command_data.response, XCR_HELLO, m_command_data.response_size);
                    custom_result_message = true;
                    break;

                case Command::Version:
                    copy_string(m_command_data.response, XCR_VERSION_STRING, m_command_data.response_size);
                    custom_result_message = true;
                    break;

                case Command::GetHudUser:
                    {
                        result = m_control_unit->GetHudUser();
                        break;
                    }

                case Command::GetHudStatus:
                    {
                        result = check_arg_count(argc, component_index + 1);
                        if (result)
                            result = m_control_unit->GetHudStatus(argv[component_index]);
                        else
                            result = m_control_unit->GetHudStatus();
                        break;
                    }

                case Command::ToggleHud:
                    {
                        result = check_arg_count(argc, component_index + 1);
                        if (result) result = m_control_unit->ToggleHud(argv[component_index]);
                        break;
                    }

                case Command::SaveState:
                    result = check_arg_count(argc, filename_index + 1);
                    if (result)
                    {
                        ddos.SetFilename(argv[filename_index]);
                        result = ddos.Start();
                        if (result) result = m_control_unit->SaveState(&ddos);
                        if (result) result = ddos.Stop();
                    }
                    break;

                case Command::ResumeState:
                    result = check_arg_count(argc, filename_index + 1);
                    if (result)
                    {
                        ddis.SetFilename(argv[filename_index]);
                        result = ddis.Start();
                        if (result) result = m_control_unit->ResumeState(&ddis);
                        if (result) result = ddis.Stop();
                    }
                    break;

                case Command::Help:
                    // General help, command help, or component help.
                    {
                        char *response_buffer = m_command_data.response;
                        char *response_buffer_end = m_command_data.response + m_command_data.response_size;
                        if (argc <= command_index + 1)
                        {
                            copy_string(m_command_data.response, "Topics: commands, components", m_command_data.response_size);
                            custom_result_message = true;
                        }
                        else if (lstrcmpiA(argv[component_index], "commands") == 0)
                        {
                            response_buffer += copy_string(response_buffer, "Commands: ", response_buffer_end - response_buffer) - 1;
                            for (const Command *command = s_commands; command != commands_end; command++)
                            {
                                response_buffer += copy_string(response_buffer, command->Name, response_buffer_end - response_buffer) - 1;
                                response_buffer += copy_string(response_buffer, " ", response_buffer_end - response_buffer) - 1;
                            }
                            custom_result_message = true;
                        }
                        else if (lstrcmpiA(argv[component_index], "components") == 0)
                        {
                            response_buffer += copy_string(response_buffer, "Components: ", response_buffer_end - response_buffer) - 1;
                            IControllerInterface::ComponentNameList names;
                            m_control_unit->GetComponentNames(names);
                            for (IControllerInterface::ComponentNameList::const_iterator it = names.begin(); it != names.end(); it++)
                            {
                                response_buffer += copy_string(response_buffer, it->c_str(), response_buffer_end - response_buffer) - 1;
                                response_buffer += copy_string(response_buffer, " ", response_buffer_end - response_buffer) - 1;
                            }
                            custom_result_message = true;
                        }
                        else
                        {
                            // Maybe it's a command.
                            const Command* help_request_command = find_command(argv[component_index], commands_end);
                            if (help_request_command == NULL)
                            {
                                // Maybe it's a component.
                                LPCSTR helpstring;
                                result = m_control_unit->GetComponentHelpString(argv[component_index], helpstring);
                                if (result)
                                {
                                    copy_string(response_buffer, helpstring, response_buffer_end - response_buffer);
                                    custom_result_message = true;
                                }
                                else
                                {
                                    result = XCR::Result::ERROR_INVALID_VALUE;
                                }
                            }
                            else
                            {
                                response_buffer += copy_string(response_buffer, help_request_command->Name, response_buffer_end - response_buffer) - 1;
                                response_buffer += copy_string(response_buffer, " ", response_buffer_end - response_buffer) - 1;
                                response_buffer += copy_string(response_buffer, help_request_command->Helpstring, response_buffer_end - response_buffer) - 1;
                                custom_result_message = true;
                            }
                        }
                    }
                    break;

                case Command::SelfTest:
                    result = self_test();
                    break;

                default:
                    result = XCR::Result::ERROR_UNKNOWN_COMMAND;
                }
            }
        }
    }


	// Notifies this controller of a game state change.
	//	parameters:
	//		szState : a string describing the state change
	void DebugChannelControl::NotifyState(LPCSTR szState)
	{
		// We don't really care about this.
	}




	// ----------------------------------------------------------------
	// Implementation
	// ----------------------------------------------------------------


	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Debug channel
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


	//	Register a threaded command proc to handle messages sent via the debug channel.
	void DebugChannelControl::register_debug_channel()
	{
		// Set the static pointer for DebugChannelControl to be used to me.
		s_XCR_debug_channel_control = this;

		// Initialize critical section used in the handler.
		if (!s_XCR_debug_channel_critical_section_initialized)
		{
			InitializeCriticalSection(&s_XCR_debug_channel_critical_section);
			s_XCR_debug_channel_critical_section_initialized = true;
		}

		// Register handler.
		HRESULT hr = DmRegisterThreadedCommandProcessor(XCR_DEBUG_CHANNEL, XCR_debug_channel_cmd_proc);
		_ASSERTE(SUCCEEDED(hr));
		// ... to avoid compiler warning 4189: local variable initialized but not referenced
		hr = 0;
	}


	// XCR debug channel command processor.  Called by the system when the PC sends a command to us.
	// Since it is registered as threaded, this handler may be called in multiple threads.
	HRESULT __stdcall DebugChannelControl::XCR_debug_channel_cmd_proc(LPCSTR lpszCommand, LPSTR lpszResponse, DWORD cchResp, PDM_CMDCONT pdmcc)
	{
		// There should be a command on the command line!
		_ASSERTE(lpszCommand);

		// The debug channel control should have been set in the constructor.
		_ASSERTE(s_XCR_debug_channel_control);

		HRESULT result;

		// Allow only one thread of XCR_debug_channel_cmd_proc execution to process a command at a time.
		EnterCriticalSection(&s_XCR_debug_channel_critical_section);
		result = s_XCR_debug_channel_control->process_command(lpszCommand, lpszResponse, cchResp);
		LeaveCriticalSection(&s_XCR_debug_channel_critical_section);

		return result;
	}


	// Critical section used to make debug channel processors operate one at a time.
	CRITICAL_SECTION DebugChannelControl::s_XCR_debug_channel_critical_section;

	// Flag indicating that the critical section was initialized.
	bool DebugChannelControl::s_XCR_debug_channel_critical_section_initialized = false;

	// A pointer to the DebugChannelControl object that the debug channel command processor should use.
	DebugChannelControl *DebugChannelControl::s_XCR_debug_channel_control = NULL;




	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// Command processing
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


	// The member function that will be called by the debug channel command processor to process the command.
	// This function does not have to be reentrant as only one debug channel command processor will call it at a time.
	// However, it will be called from the debug channel command processor's thread.
	HRESULT DebugChannelControl::process_command(LPCSTR command, LPSTR response, DWORD response_size)
	{
		// Write in the command data.
		m_command_data.command = command;
		m_command_data.response = response;
		m_command_data.response_size = response_size;
		m_command_ready = true;
	    
		// Wait for response to be ready.
		WaitForSingleObject(m_response_ready_event, INFINITE);

		return S_OK;
	}


	// Break a command into individual arguments.
	// Arguments are separated by white space, or surrounded by quotation marks.
	//	parameters:
	//		command : [in, out] The entire string.  The processor will place nul characters in it to divide arguments.
	//		argv : [out] An array of pointers to strings.  Will be filled with pointers to each argument within command.
	//		argc : [in] The maximum number of arguments to break it into (space in argv).
	//			[out] The actual number of arguments broken into.
	// Returns false if argv wasn't big enough to hold pointers to all the arguments.
	bool DebugChannelControl::parse_command(LPSTR command, LPSTR *argv, PDWORD argc)
	{
		DWORD arguments_found = 0;
		DWORD i = 0;
		char ch;
		enum { FIND_ARG, FIND_WS, FIND_ENDQUOTE } state = FIND_ARG;
		// Loop for each character.
		while ((ch = command[i]) != '\0' && arguments_found < *argc)
		{
			switch (ch)
			{
			// Whitespace.
			case ' ':
			case '\t':
				if (state == FIND_WS)
				{
					// That was the end of that argument.
					// Use a nul character to terminate it.
					command[i] = '\0';
					// Now we're looking for the next argument.
					state = FIND_ARG;
				}
				break;
			// Quotation mark.
			case '\"':
				if (state == FIND_ARG)
				{
					// We found our argument.
					argv[arguments_found] = command + i + 1;
					arguments_found++;
					// Now we're looking for a closing quotation mark.
					state = FIND_ENDQUOTE;
				}
				else if (state == FIND_ENDQUOTE)
				{
					// That was the end of that argument.
					// Use a nul character to terminate it.
					command[i] = '\0';
					// Now we're looking for the next argument.
					state = FIND_ARG;
				}
				break;
			// Any other character:
			default:
				if (state == FIND_ARG)
				{
					// Looks like we found an argument.
					argv[arguments_found] = command + i;
					arguments_found++;
					// Now we'll look for the whitespace at the end.
					state = FIND_WS;
				}
			}
			i++;
		}
		*argc = arguments_found;
		return ch == '\0';
	}


	// Check number of arguments and return an error code.
	//	parameters:
	//		actual : the actual number of arguments supplied
	//		required : the required number of arguments
	//	returns ERROR_TOO_MANY_ARGUMENTS, ERROR_TOO_FEW_ARGUMENTS, or SUCCESS
	XCR::Result DebugChannelControl::check_arg_count(DWORD actual, DWORD required)
	{
		if (actual < required)
		{
			return XCR::Result::ERROR_TOO_FEW_ARGUMENTS;
		}
		else if (actual > required)
		{
			return XCR::Result::ERROR_TOO_MANY_ARGUMENTS;
		}
		return XCR::Result::SUCCESS;
	}


	// Version of strcpy that always terminates with a null.
	//	parameters:
	//		destination_buffer : [out] buffer to copy to.  A terminating nul will always be placed at the end.
	//		source_buffer : [in] source data
	//		size : [in] amount of space available the buffer.
	//	returns number of characters copied, including terminating nul.
	size_t DebugChannelControl::copy_string(LPSTR destination_buffer, LPCSTR source_buffer, size_t max_size)
	{
		if (max_size > 0)
		{
			// Size of source string + terminating nul.
			size_t copy_size = lstrlenA(source_buffer) + 1;
			// But copy no more than buffer size.
			copy_size = max_size < copy_size ? max_size : copy_size;
			// Perform copy.  Always terminates with nul.
			lstrcpynA(destination_buffer, source_buffer, copy_size);
			return copy_size;
		}
		return 0;
	}

	const DebugChannelControl::Command* DebugChannelControl::find_command(LPCSTR command_string, const Command* commands_end)
	{
		for (const Command* command = s_commands; command != commands_end; command++)
		{
			int iCompare = lstrcmpiA(command->Name, command_string);
			if (iCompare == 0)
			{
				return command;
			}
			else if (iCompare > 0)
			{
				return NULL;
			}
		}
		return NULL;
	}

	XCR::Result DebugChannelControl::self_test()
	{
		m_control_unit->LogMessage("XCR: Test loggers.\r\n");
		return XCR::Result::SUCCESS;
	}

    void DebugChannelControl::build_signal_message(LPSTR signal_message, int signal_message_length, LPSTR* argv, int message_index, int argc)
    {
        if (1 == argc)
        {
            return;
        }
        if (2 == argc)
        {
            copy_string(signal_message, argv[1], signal_message_length);
            return;
        }
        int append_length = 0;
        int message_length = 0;
        bool use_quotes = false;
        for (int i=message_index; i<argc && append_length<signal_message_length; ++i)
        {
            message_length = lstrlenA(argv[i]);
            use_quotes = false;
            for (int j=0; j<message_length; ++j)
            {
                if (' ' == argv[i][j])
                {
                    use_quotes = true;
                    break;
                }
            }

            // copy space (if appropriate) and opening quote (if appropriate)
            if (true == use_quotes)
            {
                if (0 == append_length)
                {
                    copy_string(&(signal_message[append_length]), "\"", signal_message_length);
                    ++append_length;
                }
                else
                {
                    copy_string(&(signal_message[append_length]), " \"", signal_message_length);
                    append_length += 2;
                }
            }
            else
            {
                if (0 != append_length)
                {
                    copy_string(&(signal_message[append_length]), " ", signal_message_length);
                    ++append_length;
                }
            }

            // copy message
            copy_string(&(signal_message[append_length]), argv[i], signal_message_length);
            append_length += message_length;

            // copy closing quote (if appropriate)
            if (true == use_quotes)
            {
                copy_string(&(signal_message[append_length]), "\"", signal_message_length);
                ++append_length;
            }
        }
    }

}


#endif