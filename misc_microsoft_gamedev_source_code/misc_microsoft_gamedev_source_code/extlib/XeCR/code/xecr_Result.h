//	Result.h : Result code.
//
//	Created 2004/10/06 Rich Bonny    <rbonny@microsoft.com>
//
//  - based on original XCR code:
//	Created 2003/08/XX David Eichorn <deichorn@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2004 Microsoft Corp.  All rights reserved.




#pragma once

#include "XenonUtility.h"
using namespace XenonUtility;




namespace XCR
{

	// ================================================================
	// Result class
	//
	// Object representing a result from an operation.
	// Can represent any number of a set of errors or success.
	// ================================================================

	class Result :
		public XenonUtility::Result
	{
	public:

		enum ResultCode
		{
			SUCCESS_RECORDING_STARTED = SUCCESS_MAX,
			SUCCESS_RECORDING_STOPPED,
			SUCCESS_PLAYBACK_STARTED,
			SUCCESS_PLAYBACK_STOPPED,
			SUCCESS_OUTSIDE_RECOMMENDED_RANGE,

            SUCCESS_HIGH_BOUND,

            STATUS_LOW_BOUND,
			
			STATUS_IDLE,
            STATUS_SELECTED,
			STATUS_PLAYING,
			STATUS_RECORDING,
			STATUS_CONTROLLING,

            // MJMXAM: added statuses for hud
            STATUS_UNKNOWN,
            STATUS_ACTIVE,
            STATUS_INACTIVE,

            STATUS_HIGH_BOUND,

            PORT_LOW_BOUND,

            // MJMXAM: added ports for get hud user
            PORT_UNKNOWN,
            PORT_1,
            PORT_2,
            PORT_3,
            PORT_4,

            PORT_HIGH_BOUND,

            COMPONENT_LOW_BOUND,

            COMPONENT_UNKNOWN,

            COMPONENT_PORTS_PLAYER,
            COMPONENT_MONKEY_PLAYER,
            COMPONENT_DIRECT_DISK_PLAYER,
            COMPONENT_BUFFERED_PLAYER,
            COMPONENT_SEQUENCE_PLAYER,
            COMPONENT_RANDOM_STATE_PLAYER,

            COMPONENT_DIRECT_DISK_RECORDER,
            COMPONENT_BUFFERED_RECORDER,

            COMPONENT_HIGH_BOUND,

			ERROR_TOO_FEW_ARGUMENTS = ERROR_MAX,
			ERROR_TOO_MANY_ARGUMENTS,
			ERROR_UNKNOWN_COMMAND,
			ERROR_INVALID_COMPONENT,
			ERROR_INVALID_PROPERTY,
			ERROR_INVALID_VALUE,
			ERROR_NO_SEQUENCE,						// Added error for no sequence specified: RWB Fix Bug #357
			ERROR_CANT_SET_PROPERTY_WHILE_USING,

			ERROR_ALREADY_RUNNING,
			ERROR_ALREADY_RECORDING,
			ERROR_ALREADY_PLAYING,
			ERROR_NOT_RUNNING,
			ERROR_NOT_RECORDING,
			ERROR_NOT_PLAYING,
			ERROR_CANNOT_START_COMPONENT,
			ERROR_CANNOT_STOP_COMPONENT,
			ERROR_INCORRECT_NEXT_PLAYBACK_STEP,

			ERROR_UNKNOWN_RESULT,
			
			ERROR_CUSTOM,

            ERROR_LOW_BOUND
		};

		Result(XCR::Result::ResultCode result) :
			XenonUtility::Result(result)
		{
		}

		Result(XenonUtility::Result::ResultCode result = SUCCESS) :
			XenonUtility::Result(result)
		{
		}

		Result(XenonUtility::Result result) :
			XenonUtility::Result(result)
		{
		}

		inline bool operator ==(XCR::Result::ResultCode const &result) const
		{
			return m_result == result;
		}

		inline bool operator !=(XCR::Result::ResultCode const &result) const
		{
			return m_result != result;
		}

		inline bool operator ==(XenonUtility::Result::ResultCode const &result) const
		{
			return m_result == result;
		}

		inline bool operator !=(XenonUtility::Result::ResultCode const &result) const
		{
			return m_result != result;
		}

		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		// Result messages
		// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

		// Retrieve the result message
		XCR::Result GetMessage(const char *&message) const;

		// Result to result message string correspondence structure.
		struct ResultMessage;

		// Result strings.
		static const ResultMessage result_messages[];
	};


	struct Result::ResultMessage
	{
		XCR::Result result;
		const char *message;
	};
}