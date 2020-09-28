//	Logger.cpp : A .cpp file defining several Logger components.
//
//	Created 2004/10/12 Rich Bonny    <rbonny@microsoft.com>
//
//  - based on original XCR code:
//	Created 2003/08/29 David Eichorn <deichorn@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2004 Microsoft Corp.  All rights reserved.




#ifdef _XCR_

#include "xecr_Logger.h"




namespace XCR
{
	using namespace ControlUnit;


	// ================================================================
	//	OutputDebugStringLogger
	//
	//	Logs messages to the debugger.
	// ================================================================
	
    OutputDebugStringLogger::OutputDebugStringLogger()
        : is_running(true)
    {
    }

    XCR::Result OutputDebugStringLogger::SelectComponent()
    {
        // Selection does nothing, but we return Success just the same.
        return XCR::Result::SUCCESS;
    }

    XCR::Result OutputDebugStringLogger::Start()
    {
        if (is_running)
        {
            return XCR::Result::ERROR_ALREADY_RUNNING;
        }

        is_running = true;
        return XCR::Result::SUCCESS;
    }


    XCR::Result OutputDebugStringLogger::Stop()
    {
        if (!is_running)
        {
            return XCR::Result::ERROR_NOT_RUNNING;
        }

        is_running = false;
        return XCR::Result::SUCCESS;
    }

    void OutputDebugStringLogger::LogMessage(LPCSTR message)
	{
        if (true == is_running)
        {
		    OutputDebugStringA(message);
        }
	}


	StreamLogger::StreamLogger(OutputStream *output_stream) :
		is_running(false),
		output_stream(output_stream)
	{
	}


	void StreamLogger::LogMessage(LPCSTR message)
	{
		if (is_running)
		{
			output_stream->Write(message, lstrlenA(message));
		}
	}


	
	// Notify this component it is being selected.
	// Added Implementation: RWB Fix Bug #361
	XCR::Result StreamLogger::SelectComponent()
	{
		// Selection does nothing, but we return Success just the same.
		return XCR::Result::SUCCESS;
	}

	XCR::Result StreamLogger::Start()
	{
		if (is_running) return XCR::Result::ERROR_ALREADY_RUNNING;
		XCR::Result result = output_stream->Start();
		if (result)
		{
			is_running = true;
			return XCR::Result::SUCCESS;
		}
		return result;
	}


	XCR::Result StreamLogger::Stop()
	{
		if (!is_running) return XCR::Result::ERROR_NOT_RUNNING;
		output_stream->Write("", 2);
		XCR::Result result = 	output_stream->Stop();
		if (result)
		{
			is_running = false;
			return XCR::Result::SUCCESS;
		}
		return result;
	}
	
	XCR::Result StreamLogger::SaveState(DirectDiskOutputStream* ddos)
	{
		// Write isRunning bool:
		if (!ddos->Write((LPVOID)&is_running,(DWORD)sizeof(bool)))
			return XCR::Result::ERROR;

		if (!is_running)
			return Result::SUCCESS;
		
		output_stream->Write("", 2);
		return output_stream->SaveState(ddos);
	}
	
	XCR::Result StreamLogger::ResumeState(DirectDiskInputStream* ddis)
	{
		// Read isRunning bool:
		if (!ddis->Read((LPVOID)&is_running,(DWORD)sizeof(bool)))
			return XCR::Result::ERROR;

		if (!is_running)
			return Result::SUCCESS;
			
		return output_stream->ResumeState(ddis);
	}

	
	
	XCR::Result DirectDiskLogger::SetStringProperty(LPCSTR name, LPCSTR value1, LPCSTR value2)
	{
		if (lstrcmpiA(name, "file") == 0)
		{
			if (value2 != NULL)
			{
				return XCR::Result::ERROR_TOO_MANY_ARGUMENTS;
			}
			disk_output_stream.SetFilename(value1);
			return XenonUtility::Result::SUCCESS;
		}
		return ILogger::SetStringProperty(name, value1, value2);
	}


	XCR::Result BufferedAsynchronousDiskLogger::SetStringProperty(LPCSTR name, LPCSTR value1, LPCSTR value2)
	{
		if (lstrcmpiA(name, "file") == 0)
		{
			if (value2 != NULL)
			{
				return XCR::Result::ERROR_TOO_MANY_ARGUMENTS;
			}
			disk_output_stream.SetFilename(value1);
			return XenonUtility::Result::SUCCESS;
		}
		else if (lstrcmpiA(name, "asynch") == 0)
		{
			if (value2 != NULL)
			{
				return XCR::Result::ERROR_TOO_MANY_ARGUMENTS;
			}
			if (is_running)
			{
				return XCR::Result::ERROR_CANT_SET_PROPERTY_WHILE_USING;
			}
			if (lstrcmpiA(value1, "on") == 0 || lstrcmpiA(value1, "true") == 0)
			{
				disk_output_stream.SetAsynchronous(true);
				return XCR::Result::SUCCESS;
			}
			if (lstrcmpiA(value1, "off") == 0 || lstrcmpiA(value1, "false") == 0)
			{
				disk_output_stream.SetAsynchronous(false);
				return XCR::Result::SUCCESS;
			}
			return XCR::Result::ERROR_INVALID_VALUE;
		}
		else if (lstrcmpiA(name, "bufsize") == 0)
		{
			if (value2 != NULL)
			{
				return XCR::Result::ERROR_TOO_MANY_ARGUMENTS;
			}
			if (is_running)
			{
				return XCR::Result::ERROR_CANT_SET_PROPERTY_WHILE_USING;
			}
			long bufsize = atol(value1);
			if (bufsize > 0)
			{
				size_t buffer_size = (bufsize + XBOX_HD_SECTOR_SIZE - 1) / XBOX_HD_SECTOR_SIZE * XBOX_HD_SECTOR_SIZE;
				disk_output_stream.SetBufferSize(buffer_size);
				if (buffer_size > 16 * 1024 * 1024)
				{
					return XCR::Result::SUCCESS_OUTSIDE_RECOMMENDED_RANGE;
				}

				if (bufsize % XBOX_HD_SECTOR_SIZE != 0)
				{
					return XCR::Result::SUCCESS_BUFFER_NOT_MULTIPLE_OF_SECTOR_SIZE;
				}

				return XCR::Result::SUCCESS;
			}
			return XCR::Result::ERROR_INVALID_VALUE;
		}
		return ILogger::SetStringProperty(name, value1, value2);
	}

}


#endif