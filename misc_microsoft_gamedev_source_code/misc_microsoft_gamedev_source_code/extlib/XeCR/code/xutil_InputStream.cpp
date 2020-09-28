//	InputStream.cpp : Classes that can supply a stream of input data.
//		Includes Xbox Asynchronous disk file reader.
//
//	Created 2003/05/27 David Eichorn <deichorn@microsoft.com>
//	Modified 2003/08/21 deichorn Fixed new operations to use (std::nothrow).
//	Modified 2004/09/08 jkburns : JKBNEW: state serialization support additions
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2004 Microsoft Corp.  All rights reserved.


#ifdef _XENON_UTILITY_

#include "xutil_InputStream.h"

// Enable debugging information
// #define REPORT(x) OutputDebugString(x)
// Disable debugging information.
#define REPORT(x)
#define CHECK_RESULT(a,b) if (!a) OutputDebugStringA(b)

namespace XenonUtility
{

	// ================================================================
	//	DirectDiskInputStream
	//
	//	Interface for objects that supply a stream of data read directly from a file on disk.
	// ================================================================


	DirectDiskInputStream::DirectDiskInputStream() :
		m_file_handle(INVALID_HANDLE_VALUE)
	{
	}


	DirectDiskInputStream::~DirectDiskInputStream()
	{
		Stop();
	}


	// Open the file.
	Result DirectDiskInputStream::Start()
	{
		// Can't start if already reading.
		if (m_file_handle != INVALID_HANDLE_VALUE)
		{
			return Result::ERROR_OPERATION_IN_PROGRESS;
		}

		// Open file to read from.
		m_file_handle = CreateFile(m_filename.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);

		return (m_file_handle == INVALID_HANDLE_VALUE)
			? Result::ERROR_CANNOT_OPEN_FILE
			: Result::SUCCESS;
	}


	// Read data from the file.
	Result DirectDiskInputStream::Read(LPVOID data, DWORD data_size)
	{
		// Can't read if not started.
		if (m_file_handle == INVALID_HANDLE_VALUE)
		{
			return Result::ERROR_OPERATION_NOT_IN_PROGRESS;
		}

		// Read the data.
		DWORD bytes_read;
		BOOL read_result = ReadFile(m_file_handle, data, data_size, &bytes_read, NULL);
		REPORT("Read Event:\n");

		return (read_result == FALSE)
			? Result::ERROR_CANNOT_READ_FILE
			: (	(bytes_read < data_size) ? Result::SUCCESS_EOF : Result::SUCCESS);
	}


	// Close the file.
	Result DirectDiskInputStream::Stop()
	{
		// Can't close if it was never open.
		if (m_file_handle == INVALID_HANDLE_VALUE)
		{
			return Result::ERROR_OPERATION_NOT_IN_PROGRESS;
		}

		// Close file handle.
		CloseHandle(m_file_handle);
		m_file_handle = INVALID_HANDLE_VALUE;

		return Result::SUCCESS;
	}

	//JKBNEW: state serialization support:
	//   this is the same as start, except return file pointer to last spot before reboot
	Result DirectDiskInputStream::Continue()
	{
		// Can't start if already reading.
		if (m_file_handle != INVALID_HANDLE_VALUE)
		{
			return Result::ERROR_OPERATION_IN_PROGRESS;
		}

		// Open file to read from.
		m_file_handle = CreateFile(m_filename.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);

		// Move file ptr to last point of reading:
		if (m_file_handle != INVALID_HANDLE_VALUE && m_curFilePtrLoc > 0)
		{
			SetFilePointer(m_file_handle,(long)m_curFilePtrLoc,NULL,FILE_BEGIN);
		}

		return (m_file_handle == INVALID_HANDLE_VALUE)
			? Result::ERROR_CANNOT_OPEN_FILE
			: Result::SUCCESS;
	}

	//JKBNEW: state serialization support:
	Result DirectDiskInputStream::SaveState(DirectDiskOutputStream* ddos)
	{
		bool fileInUse = false;
		
		// Write file info if we have one opened:
		if (m_file_handle != INVALID_HANDLE_VALUE)
		{
			fileInUse = true;
			if (!ddos->Write((LPVOID)&fileInUse,(DWORD)sizeof(bool)))
					return Result::ERROR;

			// Write length of filename:
			int fnLen = m_filename.length();//sizeof(m_filename);
			if (!ddos->Write((LPVOID)&fnLen,(DWORD)sizeof(int)))
				return Result::ERROR;

			// Now write filename:
			if (fnLen > 0)
			{
				if (!ddos->Write((LPVOID)m_filename.data(),(DWORD)fnLen))
					return Result::ERROR;
			}

			// Now write current file pointer location:
			m_curFilePtrLoc = SetFilePointer(m_file_handle,0,NULL,FILE_CURRENT);
			if (!ddos->Write((LPVOID)&m_curFilePtrLoc,(DWORD)sizeof(DWORD)))
					return Result::ERROR;

			// Now stop and close the file:
			this->Stop();
		}
		else
		{
			// File not in use...
			if (!ddos->Write((LPVOID)&fileInUse,(DWORD)sizeof(bool)))
				return Result::ERROR;
		}

		return Result::SUCCESS;
	}
	
	//JKBNEW: state serialization support:
	Result DirectDiskInputStream::ResumeState(DirectDiskInputStream* ddis)
	{
		bool fileInUse = false;
		
		// Read whether we had one opened prior to save state:
		if (!ddis->Read((LPVOID)&fileInUse,(DWORD)sizeof(bool)))
			return Result::ERROR;

		if (fileInUse)
		{
			// read length of filename:
			int fnLen = 0;
			if (!ddis->Read((LPVOID)&fnLen,(DWORD)sizeof(int)))
				return Result::ERROR;

			// Now read filename:
			if (fnLen > 0)
			{
				char* tempStr = new char[fnLen+1];
				tempStr[fnLen] = '\0';
				// Read m_data string:
				if (!ddis->Read((LPVOID)tempStr,(DWORD)fnLen))
					return Result::ERROR;

				m_filename = tempStr;

				delete [] tempStr;
			}

			// Now write current file pointer location:
			m_curFilePtrLoc = 0x0;
			if (!ddis->Read((LPVOID)&m_curFilePtrLoc,(DWORD)sizeof(DWORD)))
					return Result::ERROR;

			// Now stop and close the file:
			this->Continue();
		}

		return Result::SUCCESS;
	}



	// ================================================================
	//	BufferedAsynchronousDiskInputStream
	//
	//	Interface for objects that supply a stream of data read from a file on disk,
	//		using a buffer and/or separate thread.
	// ================================================================


	// Constructor.
	BufferedAsynchronousDiskInputStream::BufferedAsynchronousDiskInputStream() :
		// Buffer size is one Xbox HD sector.
		m_buffer_size(16384),
		m_use_thread(false),
		m_file_handle(INVALID_HANDLE_VALUE),
		m_data_buffer(NULL),
		m_read_buffer(NULL),
		m_read_thread_handle(NULL),
		m_read_buffer_full_event(NULL),
		m_read_buffer_empty_event(NULL)
	{
	}

	// Destructor.
	BufferedAsynchronousDiskInputStream::~BufferedAsynchronousDiskInputStream()
	{
	}


	// Open the file, initialize the buffers, and create the thread.
	Result BufferedAsynchronousDiskInputStream::Start()
	{
		// Can't start if already reading.
		if (m_file_handle != INVALID_HANDLE_VALUE)
		{
			return Result::ERROR_OPERATION_IN_PROGRESS;
		}

		// Open the file.
		// Note the FILE_FLAG_NO_BUFFERING.  Buffered reads must be sector-sized.
		m_file_handle = CreateFile(m_filename.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_NO_BUFFERING, NULL);
		if (m_file_handle == INVALID_HANDLE_VALUE)
		{
			return Result::ERROR_CANNOT_OPEN_FILE;
		}

		// Allocate a new data buffer and start from the beginning.
		m_data_buffer = new (std::nothrow) BYTE[m_buffer_size];
		if (m_data_buffer == NULL)
		{
			cleanup_resources();
			return Result::ERROR_OUT_OF_MEMORY;
		}
		m_data_buffer_index = 0;

		// Read initial chunk of data into data buffer.
		DWORD bytes_read;
		BOOL read_result = ReadFile(m_file_handle, m_data_buffer, m_buffer_size, &bytes_read, NULL);
		if (read_result == FALSE)
		{
			cleanup_resources();
			return Result::ERROR_CANNOT_READ_FILE;
		}

		// Setup for asynchronous (if there is a need for it).
		if (bytes_read == m_buffer_size && m_use_thread)
		{
			// A second buffer will be needed to read from while reading the other one from disk.
			m_read_buffer = new (std::nothrow) BYTE[m_buffer_size];
			if (m_read_buffer == NULL)
			{
				cleanup_resources();
				return Result::ERROR_OUT_OF_MEMORY;
			}

			// Create events to signal buffer full and empty.
			m_read_buffer_full_event = CreateEvent(NULL, FALSE, FALSE, NULL);
			m_read_buffer_empty_event = CreateEvent(NULL, FALSE, TRUE, NULL);
			if (m_read_buffer_full_event == NULL || m_read_buffer_empty_event == NULL)
			{
				cleanup_resources();
				return Result::ERROR_UNKNOWN;
			}

			// Fire up thread.
			m_read_thread_done = false;
			m_read_thread_handle = CreateThread(NULL, 0, read_thread, this, 0, NULL);
			if (m_read_thread_handle == NULL)
			{
				cleanup_resources();
				return Result::ERROR_UNKNOWN;
			}
		}

		// Cancel cleanup and return success.
		return Result::SUCCESS;
	}


	// Read from the buffer, switch to the spare buffer as needed, and wait for data reads.
	Result BufferedAsynchronousDiskInputStream::Read(LPVOID data, DWORD data_size)
	{
		// Can't read if not started.
		if (m_file_handle == INVALID_HANDLE_VALUE)
		{
			return Result::ERROR_CANNOT_OPEN_FILE;
		}

		BYTE *dest_buffer = (BYTE *) data;

		// Will we use up the current buffer reading this data?
		DWORD bytes_left_in_buffer;
		while (data_size >= (bytes_left_in_buffer = m_buffer_size - m_data_buffer_index))
		{
			// Note that this loop handles the case where the data buffer gets exactly used up.
			// Use up what's left.
			memcpy(dest_buffer, m_data_buffer + m_data_buffer_index, m_buffer_size - m_data_buffer_index);

			// Load more data from disk into this buffer, and switch to the other one.
			if (m_use_thread && m_read_thread_handle)
			{
				// Swap buffers and set event to trigger thread to do reading.
				//	Note that we wait for the other buffer's read to complete here,
				//		but likely this will never actually stall (given the buffer is big enough).
				//		To read the FAT, read cluster mappings, and read the file data would probably take
				//		no more than (20ms + 2ms) * 3 = 66ms, for a 16kb buffer, if no other I/O is going on.
				//		This is less than 5 frames of a 60 fps game,
				//		which cannot possibly have 16kb of recording data.
				WaitForSingleObject(m_read_buffer_full_event, INFINITE);
				BYTE *temp_buffer = m_read_buffer;
				m_read_buffer = m_data_buffer;
				m_data_buffer = temp_buffer;
				SetEvent(m_read_buffer_empty_event);
			}
			else
			{
				// Just read directly from disk.
				DWORD bytes_read;
				BOOL bResult = ReadFile(m_file_handle, m_data_buffer, m_buffer_size, &bytes_read, NULL);
				CHECK_RESULT(bResult, "Buffered Read Failed.\n");
			}

			// Update the indices.
			dest_buffer += bytes_left_in_buffer;
			data_size -= bytes_left_in_buffer;
			m_data_buffer_index = 0;
		}

		// Read what's left.
		memcpy(dest_buffer, m_data_buffer + m_data_buffer_index, data_size);
		m_data_buffer_index += data_size;

		return Result::SUCCESS;
	}


	Result BufferedAsynchronousDiskInputStream::Stop()
	{
		// Can't stop if it was never started.
		if (m_file_handle == INVALID_HANDLE_VALUE)
		{
			return Result::ERROR_OPERATION_NOT_IN_PROGRESS;
		}

		// Signal and wait for thread to finish.
		if (m_use_thread && m_read_thread_handle)
		{
			// Wait until the read buffer is full.
			WaitForSingleObject(m_read_buffer_full_event, INFINITE);
			// Set the quit flag.
			m_read_thread_done = true;
			// Trigger thread so it looks at the quit flag.
			SetEvent(m_read_buffer_empty_event);
			// Wait for the thread to quit.
			WaitForSingleObject(m_read_thread_handle, INFINITE);
		}

		cleanup_resources();
		
		return Result::SUCCESS;
	}

	//JKBNEW: state serialization support: same as Start, except
	//        return file ptr to last position before reboot
	Result BufferedAsynchronousDiskInputStream::Continue()
	{
		// Can't start if already reading.
		if (m_file_handle != INVALID_HANDLE_VALUE)
		{
			return Result::ERROR_OPERATION_IN_PROGRESS;
		}

		// Open the file.
		// Note the FILE_FLAG_NO_BUFFERING.  Buffered reads must be sector-sized.
		m_file_handle = CreateFile(m_filename.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_NO_BUFFERING, NULL);
		if (m_file_handle == INVALID_HANDLE_VALUE)
		{
			return Result::ERROR_CANNOT_OPEN_FILE;
		}

		// Move file ptr to last point of reading:
		if (m_file_handle != INVALID_HANDLE_VALUE && m_curFilePtrLoc > 0)
		{
			SetFilePointer(m_file_handle,(long)m_curFilePtrLoc,NULL,FILE_BEGIN);
		}

		// Setup for asynchronous (if there is a need for it).
		if (m_use_thread)
		{
			// Create events to signal buffer full and empty.
			m_read_buffer_full_event = CreateEvent(NULL, FALSE, FALSE, NULL);
			m_read_buffer_empty_event = CreateEvent(NULL, FALSE, TRUE, NULL);
			if (m_read_buffer_full_event == NULL || m_read_buffer_empty_event == NULL)
			{
				cleanup_resources();
				return Result::ERROR_UNKNOWN;
			}

			// Fire up thread.
			m_read_thread_handle = CreateThread(NULL, 0, read_thread, this, 0, NULL);
			if (m_read_thread_handle == NULL)
			{
				cleanup_resources();
				return Result::ERROR_UNKNOWN;
			}
		}

		// Cancel cleanup and return success.
		return Result::SUCCESS;
	}


	// Set size of buffer.
	//	parameters:
	//		size - size to use for the buffer.  It will be rounded up to XBOX_HD_SECTOR_SIZE granularity.
	Result BufferedAsynchronousDiskInputStream::SetBufferSize(DWORD size)
	{
		// Cannot change while in the middle of operation.
		if (m_file_handle != INVALID_HANDLE_VALUE)
		{
			return Result::ERROR_OPERATION_IN_PROGRESS;
		}

		// Set buffer size.
		m_buffer_size = (size + XBOX_HD_SECTOR_SIZE - 1) / XBOX_HD_SECTOR_SIZE * XBOX_HD_SECTOR_SIZE;

		return Result::SUCCESS;
	}



	// Set the asynchronous mode.
	//	parameters:
	//		use_asynch - true to use asynchronous thread-based file operations.
	Result BufferedAsynchronousDiskInputStream::SetAsynchronous(bool use_asynch)
	{
		// Cannot change while in the middle of operation.
		if (m_file_handle != INVALID_HANDLE_VALUE)
		{
			return Result::ERROR_OPERATION_IN_PROGRESS;
		}

		// Set property.
		m_use_thread = use_asynch;

		return Result::SUCCESS;
	}

	//JKBNEW: state serialization support:
	Result BufferedAsynchronousDiskInputStream::SaveState(DirectDiskOutputStream* ddos)
	{
		bool fileInUse = false;
		
		// Write file info if we have one opened:
		if (m_file_handle != INVALID_HANDLE_VALUE)
		{
			fileInUse = true;
			if (!ddos->Write((LPVOID)&fileInUse,(DWORD)sizeof(bool)))
					return Result::ERROR;

			// Write length of filename:
			int fnLen = m_filename.length();//sizeof(m_filename);
			if (!ddos->Write((LPVOID)&fnLen,(DWORD)sizeof(int)))
				return Result::ERROR;

			// Now write filename:
			if (fnLen > 0)
			{
				if (!ddos->Write((LPVOID)m_filename.data(),(DWORD)fnLen))
					return Result::ERROR;
			}

			// Now write current file pointer location:
			m_curFilePtrLoc = SetFilePointer(m_file_handle,0,NULL,FILE_CURRENT);
			if (!ddos->Write((LPVOID)&m_curFilePtrLoc,(DWORD)sizeof(DWORD)))
					return Result::ERROR;
		}
		else
		{
			// File not in use...
			if (!ddos->Write((LPVOID)&fileInUse,(DWORD)sizeof(bool)))
				return Result::ERROR;
		}

		// Buffer size.
		if (!ddos->Write((LPVOID)&m_buffer_size,(DWORD)sizeof(DWORD)))
			return Result::ERROR;

		// Should use a thread to do I/O?
		if (!ddos->Write((LPVOID)&m_use_thread,(DWORD)sizeof(bool)))
			return Result::ERROR;

		// Buffer we are filling now in memory.
		bool dataBufNotNull = false;
		if (m_data_buffer != NULL)
		{
			dataBufNotNull = true;
			if (!ddos->Write((LPVOID)&dataBufNotNull,(DWORD)sizeof(bool)))
			return Result::ERROR;

			if (!ddos->Write((LPVOID)m_data_buffer,(DWORD)m_buffer_size))
				return Result::ERROR;
		}
		else
		{
			if (!ddos->Write((LPVOID)&dataBufNotNull,(DWORD)sizeof(bool)))
				return Result::ERROR;
		}

		// Buffer we are reading from disk now.
		bool readBufNotNull = false;
		if (m_read_buffer != NULL)
		{
			readBufNotNull = true;
			if (!ddos->Write((LPVOID)&readBufNotNull,(DWORD)sizeof(bool)))
				return Result::ERROR;

			if (!ddos->Write((LPVOID)m_read_buffer,(DWORD)m_buffer_size))
				return Result::ERROR;
		}
		else
		{
			if (!ddos->Write((LPVOID)&readBufNotNull,(DWORD)sizeof(bool)))
				return Result::ERROR;
		}

		// Where the data buffer pointer is currently located.
		if (!ddos->Write((LPVOID)&m_data_buffer_index,(DWORD)sizeof(DWORD)))
			return Result::ERROR;

		// Time for thread to quit?
		if (!ddos->Write((LPVOID)&m_read_thread_done,(DWORD)sizeof(bool)))
			return Result::ERROR;

		this->Stop();

		return Result::SUCCESS;
	}
	
	//JKBNEW: state serialization support:
	Result BufferedAsynchronousDiskInputStream::ResumeState(DirectDiskInputStream* ddis)
	{
		bool fileInUse = false;
		if (!ddis->Read((LPVOID)&fileInUse,(DWORD)sizeof(bool)))
			return Result::ERROR;
			
		// read file info if we have one opened:
		if (fileInUse)
		{
			// read length of filename:
			int fnLen = 0;
			if (!ddis->Read((LPVOID)&fnLen,(DWORD)sizeof(int)))
				return Result::ERROR;

			// Now read filename:
			if (fnLen > 0)
			{
				char* tempStr = new char[fnLen+1];
				tempStr[fnLen] = '\0';
				// Read m_data string:
				if (!ddis->Read((LPVOID)tempStr,(DWORD)fnLen))
					return Result::ERROR;

				m_filename = tempStr;

				delete [] tempStr;
			}

			// Now read current file pointer location:
			m_curFilePtrLoc = 0x0;
			if (!ddis->Read((LPVOID)&m_curFilePtrLoc,(DWORD)sizeof(DWORD)))
					return Result::ERROR;
		}

		// Buffer size.
		if (!ddis->Read((LPVOID)&m_buffer_size,(DWORD)sizeof(DWORD)))
			return Result::ERROR;

		// Should use a thread to do I/O?
		if (!ddis->Read((LPVOID)&m_use_thread,(DWORD)sizeof(bool)))
			return Result::ERROR;

		// Buffer we are getting data from now in memory.
		m_data_buffer = new (std::nothrow) BYTE[m_buffer_size];
		// first check if it's null:
		bool dataBufNotNull = false;
		if (!ddis->Read((LPVOID)&dataBufNotNull,(DWORD)sizeof(bool)))
			return Result::ERROR;
		// now read buffer if ok:
		if (dataBufNotNull)
			if (!ddis->Read((LPVOID)m_data_buffer,(DWORD)m_buffer_size))
				return Result::ERROR;

		// Buffer we are reading from disk now.
		m_read_buffer = new (std::nothrow) BYTE[m_buffer_size];
		// first check if it's null:
		bool readBufNotNull = false;
		if (!ddis->Read((LPVOID)&readBufNotNull,(DWORD)sizeof(bool)))
			return Result::ERROR;
		// now read buffer if ok:
		if (readBufNotNull)
			if (!ddis->Read((LPVOID)m_read_buffer,(DWORD)m_buffer_size))
				return Result::ERROR;

		// Where the data buffer pointer is currently located.
		if (!ddis->Read((LPVOID)&m_data_buffer_index,(DWORD)sizeof(DWORD)))
			return Result::ERROR;

		// Time for thread to quit?
		if (!ddis->Read((LPVOID)&m_read_thread_done,(DWORD)sizeof(bool)))
			return Result::ERROR;

		this->Continue();

		return Result::SUCCESS;
	}



	// Clean up resources.
	void BufferedAsynchronousDiskInputStream::cleanup_resources()
	{
		// Close file.
		if (m_file_handle != INVALID_HANDLE_VALUE)
		{
			CloseHandle(m_file_handle);
			m_file_handle = INVALID_HANDLE_VALUE;
		}

		// Deallocate data buffer.
		delete [] m_data_buffer;
		m_data_buffer = NULL;

		// Deallocate spare buffer.
		delete [] m_read_buffer;
		m_read_buffer = NULL;

		if (m_read_thread_handle != NULL)
		{
			CloseHandle(m_read_thread_handle);
			m_read_thread_handle = NULL;
		}

		if (m_read_buffer_full_event != NULL)
		{
			CloseHandle(m_read_buffer_full_event);
			m_read_buffer_full_event = NULL;
		}

		if (m_read_buffer_empty_event != NULL)
		{
			CloseHandle(m_read_buffer_empty_event);
			m_read_buffer_empty_event = NULL;
		}
	}


	// This is the thread that does all the reading.
	DWORD WINAPI BufferedAsynchronousDiskInputStream::read_thread(LPVOID lpParameter)
	{
		BufferedAsynchronousDiskInputStream *that = (BufferedAsynchronousDiskInputStream *) lpParameter;
		for (;;)
		{
			// Wait for them to empty a buffer for us or tell us it's time to quit.
			WaitForSingleObject(that->m_read_buffer_empty_event, INFINITE);
			// Is it time to quit?
			if (that->m_read_thread_done)
			{
				// Quit without reading.
				break;
			}
			// Read it.  Don't care much about errors...
			// Then again, we will assert that the read succeeded.
			DWORD bytes_read;
			BOOL bResult = ReadFile(that->m_file_handle, that->m_read_buffer, that->m_buffer_size, &bytes_read, NULL);
			CHECK_RESULT(bResult, "Buffered Asynchronous Read Failed.\n");

			// Notify them they can use this buffer again.
			SetEvent(that->m_read_buffer_full_event);
		}
		return 0;
	}

}

#endif // _XENON_UTILITY_