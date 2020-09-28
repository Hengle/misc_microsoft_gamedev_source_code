//	InputStream.h : Classes that can supply a stream of input data.
//		Includes Xbox Asynchronous disk file reader.
//
//	Created 2003/05/27 David Eichorn <deichorn@microsoft.com>
//	Modified 2004/09/08 jkburns : JKBNEW: state serialization support additions
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2004 Microsoft Corp.  All rights reserved.

#pragma once

#ifdef _XENON_UTILITY_

#include "XenonUtility.h"

//JKBNEW:
#include "xutil_OutputStream.h"


#include <string>
//using namespace std;

namespace XenonUtility
{
	//JKBNEW:
	class DirectDiskInputStream;
	class DirectDiskOutputStream;
	
	// ================================================================
	//	InputStream
	//
	//	Interface for objects that supply a stream of data.
	// To use, call
	//		Start() to begin reading
	//		Read() to get data in chunks
	//		Stop() to finish reading
	// ================================================================
	class InputStream
	{
	public:
		// Destructor.
		virtual ~InputStream(void)
		{
		}

		// Start reading from stream.
		//	Call this before calling Read().
		//	returns result code
		virtual Result Start() = 0;

		// Read data from stream.
		//	parameters:
		//		data - pointer to buffer to store data in
		//		data_size - how many bytes to read
		//	returns SUCCESS, SUCCESS_EOF if end of stream read, or failure code.
		virtual Result Read(LPVOID data, DWORD data_size) = 0;

		// Stop reading from stream.
		//	Call this when you no longer need to read from the stream.
		//	returns result code
		virtual Result Stop() = 0;

		//JKBNEW: state serialization support (PROBABLY NOT NEEDED):
		virtual Result Continue() = 0;
        virtual std::string GetFilename() = 0;
		virtual void SetFilename(const char* filename) = 0;

		//JKBNEW: state serialization support:
		virtual Result SaveState(DirectDiskOutputStream* ddos) = 0;
		virtual Result ResumeState(DirectDiskInputStream* ddis) = 0;
	};




	// ================================================================
	//	DiskInputStream
	//
	//	Interface for objects that supply a stream of data read from a file on disk.
	// ================================================================
	class DiskInputStream :
		public InputStream
	{
	public:
		// Set the filename to read from.
		//	parameters:
		//		filename - the name of the file to read from.
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

		//JKBNEW: state serialization support (PROBABLY NOT NEEDED):
        virtual std::string GetFilename()
		{
			return m_filename;
		}

		//JKBNEW: state serialization support (PROBABLY NOT NEEDED):
		virtual Result Continue() = 0;

		//JKBNEW: state serialization support:
		virtual Result SaveState(DirectDiskOutputStream* ddos) = 0;
		virtual Result ResumeState(DirectDiskInputStream* ddis) = 0;

	protected:
		// Name of file to read from.
        std::string m_filename;

		//JKBNEW: state serialization support:
		DWORD m_curFilePtrLoc;
	};




	// ================================================================
	//	DirectDiskInputStream
	//
	//	Interface for objects that supply a stream of data read directly from a file on disk.
	// ================================================================
	class DirectDiskInputStream :
		public DiskInputStream
	{
	public:
		DirectDiskInputStream();
		~DirectDiskInputStream();

		// InputStream interface.
		virtual Result Start();
		virtual Result Read(LPVOID data, DWORD data_size);
		virtual Result Stop();

		//JKBNEW: state serialization support(PROBABLY NOT NEEDED):
		virtual Result Continue();

		//JKBNEW: state serialization support:
		virtual Result SaveState(DirectDiskOutputStream* ddos);
		virtual Result ResumeState(DirectDiskInputStream* ddis);

	protected:
		// Handle for input file.
		HANDLE m_file_handle;
	};




	// ================================================================
	//	BufferedAsynchronousDiskInputStream
	//
	//	Interface for objects that supply a stream of data read from a file on disk,
	//		using a buffer and/or separate thread.
	// ================================================================
	class BufferedAsynchronousDiskInputStream :
		public DiskInputStream
	{
	public:
		// Constructor.
		BufferedAsynchronousDiskInputStream();
		// Destructor.
		~BufferedAsynchronousDiskInputStream();

		// InputStream interface.
		virtual Result Start();
		virtual Result Read(LPVOID data, DWORD data_size);
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

		// Handle for input file.
		HANDLE m_file_handle;

		// Buffer we are getting data from now in memory.
		BYTE *m_data_buffer;

		// Buffer we are reading from disk now.
		BYTE *m_read_buffer;

		// Where the data buffer pointer is currently located.
		DWORD m_data_buffer_index;

		// Handle for thread that reads.
		//	This may be NULL even if m_use_thread is true, if the file is small
		//	enough to be read in a single chunk and the thread wasn't started.
		HANDLE m_read_thread_handle;

		// Read buffer ready to be read from disk.
		HANDLE m_read_buffer_full_event;

		// Read buffer finished being read from disk.
		HANDLE m_read_buffer_empty_event;

		// Time for thread to quit?
		bool m_read_thread_done;

		// Read thread procedure.
		static DWORD WINAPI read_thread(LPVOID lpParameter);

		// Cleanup resources allocated in the process.
		void cleanup_resources();
	};
}

#endif // _XENON_UTILITY_