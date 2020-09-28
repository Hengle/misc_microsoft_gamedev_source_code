//	XenonUtility.h : A .h file defining XBUtil
//		in the Miscellaneous Files project.
//
//	Created 2003/05/28 David Eichorn <deichorn@microsoft.com>
//	Modified 2004/02/03 jkburns : Removed semicolon from #define at top of file
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2004 Microsoft Corp.  All rights reserved.


#pragma once

#ifdef _XENON_UTILITY_

#include <xtl.h>

// Define sector size for Xenon - RWB
#define XBOX_HD_SECTOR_SIZE 512

#define lengthof(x) (sizeof (x) / sizeof ((x)[0]))

namespace XenonUtility
{

	// ================================================================
	// Result class
	//
	// Object representing a result from an operation.
	// Can represent any number of a set of errors or success.
	// ================================================================
	class Result
	{
	public:
		enum ResultCode
		{
			SUCCESS = 0,

			SUCCESS_UNHANDLED,
			SUCCESS_TRUE,
			SUCCESS_FALSE,
			SUCCESS_BUFFER_TOO_SMALL,
			SUCCESS_BUFFER_NOT_MULTIPLE_OF_SECTOR_SIZE,
			SUCCESS_EOF,

			SUCCESS_MAX,

			ERROR = 0x80000000,

			ERROR_UNKNOWN = ERROR,
			ERROR_NOT_YET_IMPLEMENTED,
			ERROR_BUFFER_TOO_SMALL,
			ERROR_EOF,
			ERROR_OUT_OF_MEMORY,

			ERROR_OPERATION_IN_PROGRESS,
			ERROR_OPERATION_NOT_IN_PROGRESS,
			
			ERROR_FILE_UNKNOWN,
			ERROR_CANNOT_CREATE_FILE,
			ERROR_CANNOT_OPEN_FILE,
			ERROR_CANNOT_WRITE_FILE,
			ERROR_CANNOT_READ_FILE,
			ERROR_INVALID_FILE,
			ERROR_OUT_OF_DISK_SPACE,

			ERROR_MAX
		};
		
		Result(ResultCode result = SUCCESS) :
			m_result(result)
		{
		}

		Result(bool result) :
			m_result(result ? SUCCESS : ERROR)
		{
		}
		
		inline operator bool() const
		{
			return m_result >= SUCCESS;
		}

		inline bool operator ==(Result const &result) const
		{
			return m_result == result.m_result;
		}

		inline bool operator !=(Result const &result) const
		{
			return m_result != result.m_result;
		}

		inline bool operator ==(ResultCode const &result) const
		{
			return m_result == result;
		}

		inline bool operator !=(ResultCode const &result) const
		{
			return m_result != result;
		}

		inline bool operator <(Result const &result) const
		{
			return m_result < result.m_result;
		}

		inline bool operator >(Result const &result) const
		{
			return m_result > result.m_result;
		}

		inline bool operator <=(Result const &result) const
		{
			return m_result <= result.m_result;
		}

		inline bool operator >=(Result const &result) const
		{
			return m_result >= result.m_result;
		}

		// Added function to retrieve code
		inline int GetCode()
		{
			return m_result;
		}

	protected:
		Result(int code) : m_result(code) { }

		int m_result;

		// The error code.
		//ResultCode m_result;
	};
}

#endif // _XENON_UTILITY_