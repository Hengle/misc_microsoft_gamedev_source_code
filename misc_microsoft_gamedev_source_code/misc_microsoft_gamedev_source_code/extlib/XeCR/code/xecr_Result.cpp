//	Result.cpp : Result code.
//
//	Created 2004/10/12 Rich Bonny    <rbonny@microsoft.com>
//
//  - based on original XCR code:
//	Created 2003/03/XX David Eichorn <deichorn@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2004 Microsoft Corp.  All rights reserved.




#ifdef _XCR_

#include "xecr_Result.h"
#include <algorithm>




namespace XCR
{

	// Result messages.
	// Note that these must be sorted in order of the result code.
	const Result::ResultMessage Result::result_messages[] =
	{
		{ XenonUtility::Result::ERROR_UNKNOWN, "Unknown error." },
		{ XenonUtility::Result::ERROR_NOT_YET_IMPLEMENTED, "Not yet implemented." },
		{ XenonUtility::Result::ERROR_BUFFER_TOO_SMALL, "Buffer too small." },
		{ XenonUtility::Result::ERROR_EOF, "Unexpected end of file." },
		{ XenonUtility::Result::ERROR_OUT_OF_MEMORY, "Out of memory." },

		{ XenonUtility::Result::ERROR_OPERATION_IN_PROGRESS, "Operation in progress." },
		{ XenonUtility::Result::ERROR_OPERATION_NOT_IN_PROGRESS, "Operation not in progress." },

		{ XenonUtility::Result::ERROR_FILE_UNKNOWN, "Unknown file error." },
		{ XenonUtility::Result::ERROR_CANNOT_CREATE_FILE, "Cannot create file." },
		{ XenonUtility::Result::ERROR_CANNOT_OPEN_FILE, "Cannot open file." },
		{ XenonUtility::Result::ERROR_CANNOT_WRITE_FILE, "Cannot write to file." },
		{ XenonUtility::Result::ERROR_CANNOT_READ_FILE, "Cannot read from file." },
		{ XenonUtility::Result::ERROR_INVALID_FILE, "File is corrupt." },
		{ XenonUtility::Result::ERROR_OUT_OF_DISK_SPACE, "Out of disk space." },
		
		{ XCR::Result::ERROR_TOO_FEW_ARGUMENTS, "Not enough arguments." },
		{ XCR::Result::ERROR_TOO_MANY_ARGUMENTS, "Too many arguments." },
		{ XCR::Result::ERROR_UNKNOWN_COMMAND, "Unknown command." },
		{ XCR::Result::ERROR_INVALID_COMPONENT, "Invalid component." },
		{ XCR::Result::ERROR_INVALID_PROPERTY, "Invalid property." },
		{ XCR::Result::ERROR_INVALID_VALUE, "Invalid value." },
		// Added error for no sequence specified: RWB Fix Bug #357
		{ XCR::Result::ERROR_NO_SEQUENCE, "No Sequence Specified." },
		{ XCR::Result::ERROR_CANT_SET_PROPERTY_WHILE_USING, "Can't set property while in use." },

		{ XCR::Result::ERROR_ALREADY_RUNNING, "Component already running." },
		{ XCR::Result::ERROR_ALREADY_RECORDING, "Already recording." },
		{ XCR::Result::ERROR_ALREADY_PLAYING, "Already playing." },
		{ XCR::Result::ERROR_NOT_RUNNING, "Component not running." },
		{ XCR::Result::ERROR_NOT_RECORDING, "Not recording." },
		{ XCR::Result::ERROR_NOT_PLAYING, "Not playing." },
		{ XCR::Result::ERROR_CANNOT_START_COMPONENT, "Cannot start component." },
		{ XCR::Result::ERROR_CANNOT_STOP_COMPONENT, "Cannot stop component." },
		{ XCR::Result::ERROR_INCORRECT_NEXT_PLAYBACK_STEP, "Title responded incorrectly to playback." },
		
		{ XCR::Result::ERROR_UNKNOWN_RESULT, "Unknown result." },

		// Changed description to be more accurate: RWB Fix Bug #358
		{ XCR::Result::ERROR_CUSTOM, "Specialized error -- see log for details." },

		{ XenonUtility::Result::SUCCESS, "Ok." },
		{ XenonUtility::Result::SUCCESS_UNHANDLED, "Unhandled." },
		{ XenonUtility::Result::SUCCESS_TRUE, "true" },
		{ XenonUtility::Result::SUCCESS_FALSE, "false" },
		{ XenonUtility::Result::SUCCESS_BUFFER_TOO_SMALL, "Buffer too small." },
		{ XenonUtility::Result::SUCCESS_BUFFER_NOT_MULTIPLE_OF_SECTOR_SIZE, "Buffer specified isn't multiple of sector size (512).  The nearest multiple of the sector size was chosen automatically." },
		{ XenonUtility::Result::SUCCESS_EOF, "End of file." },
		
		{ XCR::Result::SUCCESS_RECORDING_STARTED, "Recording started successfully." },
		{ XCR::Result::SUCCESS_RECORDING_STOPPED, "Recording stopped successfully." },
		{ XCR::Result::SUCCESS_PLAYBACK_STARTED, "Playback started successfully." },
		{ XCR::Result::SUCCESS_PLAYBACK_STOPPED, "Playback stopped successfully." },
		{ XCR::Result::SUCCESS_OUTSIDE_RECOMMENDED_RANGE, "Ok.  Note: Value is outside recommended range." },
		
        { XCR::Result::STATUS_IDLE, "Idle." },
        { XCR::Result::STATUS_SELECTED, "Selected." },
		{ XCR::Result::STATUS_PLAYING, "Playing." },
		{ XCR::Result::STATUS_RECORDING, "Recording." },
		{ XCR::Result::STATUS_CONTROLLING, "Controlling." },

        // MJMXAM: added statuses for hud
        { XCR::Result::STATUS_UNKNOWN, "Unknown." },
        { XCR::Result::STATUS_ACTIVE, "Active." },
        { XCR::Result::STATUS_INACTIVE, "Inactive." },

        // MJMXAM: added ports for get hud user
        { XCR::Result::PORT_UNKNOWN, "Unknown Port" },
        { XCR::Result::PORT_1, "1" },
        { XCR::Result::PORT_2, "2" },
        { XCR::Result::PORT_3, "3" },
        { XCR::Result::PORT_4, "4" },

        { XCR::Result::COMPONENT_UNKNOWN, "Unknown Component" },

        { XCR::Result::COMPONENT_PORTS_PLAYER, "ports" },
        { XCR::Result::COMPONENT_MONKEY_PLAYER, "monkey" },
        { XCR::Result::COMPONENT_DIRECT_DISK_PLAYER, "ddplay" },
        { XCR::Result::COMPONENT_BUFFERED_PLAYER, "bufplay" },
        { XCR::Result::COMPONENT_SEQUENCE_PLAYER, "seqplay" },
        { XCR::Result::COMPONENT_RANDOM_STATE_PLAYER, "rs" },

        { XCR::Result::COMPONENT_DIRECT_DISK_RECORDER, "ddrec" },
        { XCR::Result::COMPONENT_BUFFERED_RECORDER, "bufrec" }
    };


	// Fill a buffer with an error string corresponding to an XCR::Result.
	//	Returns SUCCESS on success
	//		ERROR_UNKNOWN_RESULT if there is no message for it
	XCR::Result Result::GetMessage(const char *&message) const
	{
		// Find the matching message - rewritten to avoid template call that may be incompatible with
		// Xenon STL. Also simplified.
		int iMaxIndex = sizeof(result_messages)/sizeof(Result::ResultMessage);
		for (int i = 0; i < iMaxIndex; i++)
		{
			int currentResult = (((XenonUtility::Result)(result_messages[i].result)).GetCode());
			if (m_result == currentResult)
			{
				message = result_messages[i].message;
				return XCR::Result::SUCCESS;
			}
			else if (m_result < currentResult)
			{
				break;
			}
		}
		// Didn't find matching result
		message = "Couldn't retrieve result message.";
		return XCR::Result::ERROR_UNKNOWN_RESULT;
	}

}


#endif