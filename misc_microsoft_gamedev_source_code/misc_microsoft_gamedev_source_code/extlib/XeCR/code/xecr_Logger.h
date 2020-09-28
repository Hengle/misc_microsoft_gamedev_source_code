//	Logger.h : A .h file defining several Logger components.
//
//	Created 2004/10/06 Rich Bonny    <rbonny@microsoft.com>
//
//  - based on original XCR code:
//	Created 2003/08/29 David Eichorn <deichorn@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2004 Microsoft Corp.  All rights reserved.




#pragma once

#include "xecr_interfaces.h"

#include "xutil_OutputStream.h"




namespace XCR
{
	using namespace ControlUnit;


	// ================================================================
	//	OutputDebugStringLogger
	//
	//	Logs messages to the debugger.
	// ================================================================
	class OutputDebugStringLogger :
		public ILogger
	{
    public:

        OutputDebugStringLogger();

        virtual XCR::Result SelectComponent();
        virtual XCR::Result Start();
        virtual XCR::Result Stop();

        void LogMessage(LPCSTR message);
		
		// Retrieve helpstring.
		virtual const char *GetHelpString()
		{
			static const char *helpstring = "Logs XeCR messages to the debugger using OutputDebugString().";
			return helpstring;
		}

        // Retrieve status.
        virtual XCR::Result GetStatus()
        {
            if (is_running)
                return XCR::Result::STATUS_RECORDING;

            return XCR::Result::STATUS_IDLE;
        }

    protected:
        bool is_running;

    };


	
	// ================================================================
	//	StreamLogger
	//
	//	Logs messages to a stream.
	// ================================================================
	class StreamLogger :
		public ILogger
	{
	public:
		StreamLogger(OutputStream *output_stream);

		// ----------------------------------------------------------------
		// IComponent interface
		// ----------------------------------------------------------------
		
		// Notify this component it is being selected.
		// Added Implementation: RWB Fix Bug #361
		virtual XCR::Result SelectComponent();
		virtual XCR::Result Start();
		virtual XCR::Result Stop();

		void LogMessage(LPCSTR message);
		virtual const char *GetHelpString()
		{
			return "Logs XeCR messages to an output stream.";
		}

		// Retrieve status.
		virtual XCR::Result GetStatus()
		{
			if (is_running)
				return XCR::Result::STATUS_RECORDING;

			return XCR::Result::STATUS_IDLE;
		}

		virtual XCR::Result SaveState(DirectDiskOutputStream* ddos);
		virtual XCR::Result ResumeState(DirectDiskInputStream* ddis);

	protected:
		bool is_running;
		OutputStream *output_stream;
	};




	// ================================================================
	//	DirectDiskLogger
	//
	//	Logs messages directly to a file on disk.
	// ================================================================
	class DirectDiskLogger :
		public StreamLogger
	{
	public:
		DirectDiskLogger() :
			StreamLogger(&disk_output_stream)
		{
			disk_output_stream.SetFilename("game:\\xcr.log");
		}

		XCR::Result SetStringProperty(LPCSTR name, LPCSTR value1, LPCSTR value2 = NULL);

		virtual const char *GetHelpString()
		{
			return "Logs XeCR messages to a file.\vProperties:\v  file <full path> - file to log to";
		}

        virtual XCR::Result SetDefault()
        {
            disk_output_stream.SetFilename("game:\\xcr.log");
            return XCR::Result::SUCCESS;
        }

	protected:
		DirectDiskOutputStream disk_output_stream;
	};




	// ================================================================
	//	BufferedAsynchronousDiskLogger
	//
	//	Logs messages to a file on disk, using buffering and threading.
	// ================================================================
	class BufferedAsynchronousDiskLogger :
		public StreamLogger
	{
	public:
		BufferedAsynchronousDiskLogger() :
			StreamLogger(&disk_output_stream)
		{
			disk_output_stream.SetFilename("game:\\xcr.log");
		}

		XCR::Result SetStringProperty(LPCSTR name, LPCSTR value1, LPCSTR value2 = NULL);

		virtual const char *GetHelpString()
		{
			return "Logs XeCR messages to a file.\vProperties:\v  file <full path> - file to log to\v  asynch on|off|true|false - use thread to perform asynchronous writes?\v  bufsize <bytes> - size of buffer to use";
		}

        virtual XCR::Result SetDefault()
        {
            disk_output_stream.SetFilename("game:\\xcr.log");
            disk_output_stream.SetAsynchronous(false);
            disk_output_stream.SetBufferSize(16384);
            return XCR::Result::SUCCESS;
        }


	protected:
		BufferedAsynchronousDiskOutputStream disk_output_stream;
	};
}