//	OutputStream.h : Classes that can receive a stream of output data.
//		Includes Xbox Asynchronous disk file writer.
//
//	Created 2003/09/16 David Eichorn <deichorn@microsoft.com>
//	Modified 2004/09/08 jkburns : JKBNEW: state serialization support additions
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2004 Microsoft Corp.  All rights reserved.

#pragma once

#ifdef _XENON_UTILITY_

#include "XenonUtility.h"

//JKBNEW:
#include "xutil_InputStream.h"

#include <string>
//using namespace std;

namespace XenonUtility
{
	//JKBNEW:
	class DirectDiskOutputStream;
	class DirectDiskInputStream;

	// ================================================================
	//	OutputStream
	//
	//	Interface for objects that accept a stream of data.
	// To use, call
	//		Start() to begin writing
	//		Write() to write data in chunks
	//		Stop() to finish writing
	// ================================================================
	class OutputStream
	{
	public:
		virtual ~OutputStream(void)
		{
		}

		// Start writing to stream.
		//	Call this before calling Write().
		//	returns result code
		virtual Result Start() = 0;

		// Write data to stream.
		//	parameters:
		//		data - pointer to data buffer
		//		data_size - how many bytes to write
		//	returns SUCCESS, or failure code.
		virtual Result Write(LPCVOID data, DWORD data_size) = 0;

		// Stop writing to stream.
		//	Call this when you no longer need to write to the stream.
		//	returns result code
		virtual Result Stop() = 0;

		//JKBNEW: state serialization support(PROBABLY NOT NEEDED):
		virtual Result Continue() = 0;
        virtual std::string GetFilename() = 0;
		virtual void SetFilename(const char* filename) = 0;

		//JKBNEW: state serialization support:
		virtual Result SaveState(DirectDiskOutputStream* ddos) = 0;
		virtual Result ResumeState(DirectDiskInputStream* ddis) = 0;
	};




	// ================================================================
	//	DiskOutputStream
	//
	//	Interface for objects that supply a stream of data to a file on disk.
	// ================================================================
	class DiskOutputStream :
		public OutputStream
	{
	public:
		// Set the filename to write to.
		//	parameters:
		//		filename - the name of the file.
		//			On Xbox, this must be a complete path.
		virtual void SetFilename(const char *filename)
		{
            if (0 != filename)
            {
                m_filename = filename;
            }
            else
            {
                m_filename.clear();
            }
		}

		//JKBNEW: state serialization support(PROBABLY NOT NEEDED):
        virtual std::string GetFilename()
		{
			return m_filename;
		}

		//JKBNEW: state serialization support(PROBABLY NOT NEEDED):
		virtual Result Continue() = 0;

		//JKBNEW: state serialization support:
		virtual Result SaveState(DirectDiskOutputStream* ddos) = 0;
		virtual Result ResumeState(DirectDiskInputStream* ddis) = 0;

	protected:
        std::string m_filename;
	};




	// ================================================================
	//	DirectDiskOutputStream
	//
	//	Interface for objects that supply a stream of data directly to a file on disk.
	// ================================================================
	class DirectDiskOutputStream :
		public DiskOutputStream
	{
	public:
		DirectDiskOutputStream();
		~DirectDiskOutputStream();

		// OutputStream interface.
		virtual Result Start();
		virtual Result Write(LPCVOID data, DWORD data_size);
		virtual Result Stop();

		//JKBNEW: state serialization support(PROBABLY NOT NEEDED):
		virtual Result Continue();

		//JKBNEW: state serialization support:
		virtual Result SaveState(DirectDiskOutputStream* ddos);
		virtual Result ResumeState(DirectDiskInputStream* ddis);

	protected:
		HANDLE m_file_handle;
	};




	// ================================================================
	//	BufferedAsynchronousDiskOutputStream
	//
	//	Interface for objects that supply a stream of data to a file on disk,
	//		using a buffer and/or separate thread.
	// ================================================================
	class BufferedAsynchronousDiskOutputStream :
		public DiskOutputStream
	{
	public:
		BufferedAsynchronousDiskOutputStream();
		~BufferedAsynchronousDiskOutputStream();

		virtual Result Start();
		virtual Result Write(LPCVOID data, DWORD data_size);
		virtual Result Stop();

		//JKBNEW: state serialization support(PROBABLY NOT NEEDED):
		virtual Result Continue();

		// Set size of buffer.
		//	parameters:
		//		size - size to use for the buffer.  It will be rounded up to XBOX_HD_SECTOR_SIZE granularity.
		virtual Result SetBufferSize(DWORD size);

		// Set the asynchronous mode.
		//	parameters:
		//		use_asynch - true to use asynchronous thread-based file operations.
		virtual Result SetAsynchronous(bool use_asynch);

		//JKBNEW: state serialization support:
		virtual Result SaveState(DirectDiskOutputStream* ddos);
		virtual Result ResumeState(DirectDiskInputStream* ddis);

	protected:
		// Buffer size.
		DWORD m_buffer_size;
		// Should use a thread to do I/O?
		bool m_use_thread;
		// Handle for file.
		HANDLE m_file_handle;
		// Buffer we are filling now in memory.
		BYTE *m_data_buffer;
		// Buffer we are writing to disk now.
		BYTE *m_write_buffer;
		// Where the data buffer pointer is currently located.
		DWORD m_data_buffer_index;
		// Handle for thread that writes.
		//	This may be NULL even if m_use_thread is true, if the data is small
		//	enough to be written in a single chunk and the thread wasn't started.
		HANDLE m_write_thread_handle;
		// Write buffer ready to be read from disk.
		HANDLE m_write_buffer_full_event;
		// Write buffer finished being read from disk.
		HANDLE m_write_buffer_empty_event;
		// Time for thread to quit?
		bool m_write_thread_done;

		// Write thread procedure.
		static DWORD WINAPI write_thread(LPVOID lpParameter);

		// Cleanup resources allocated in the process.
		void cleanup_resources();

		//JKBNEW: state serialization support:
		DWORD m_numPaddedZeros;
	};
}

#endif // _XENON_UTILITY_