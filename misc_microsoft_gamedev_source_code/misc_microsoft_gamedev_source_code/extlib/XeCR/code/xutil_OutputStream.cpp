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

#include "xutil_OutputStream.h"


namespace XenonUtility
{

	// ================================================================
	//	DirectDiskOutputStream
	//
	//	Interface for objects that supply a stream of data directly to a file on disk.
	// ================================================================


	DirectDiskOutputStream::DirectDiskOutputStream() :
		m_file_handle(INVALID_HANDLE_VALUE)
	{
	}


	DirectDiskOutputStream::~DirectDiskOutputStream()
	{
		Stop();
	}


	// Open the file.
	Result DirectDiskOutputStream::Start()
	{
		// Can't start if already reading.
		if (m_file_handle != INVALID_HANDLE_VALUE)
		{
			return Result::ERROR_OPERATION_IN_PROGRESS;
		}

		// Open file to write to.
		m_file_handle = CreateFileA(m_filename.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_FLAG_SEQUENTIAL_SCAN, NULL);

		return (m_file_handle == INVALID_HANDLE_VALUE)
			? Result::ERROR_CANNOT_OPEN_FILE
			: Result::SUCCESS;
	}


	// Write data to the file.
	Result DirectDiskOutputStream::Write(LPCVOID data, DWORD data_size)
	{
		if (m_file_handle == INVALID_HANDLE_VALUE)
		{
			return Result::ERROR_OPERATION_NOT_IN_PROGRESS;
		}
		DWORD bytes_written;
		BOOL result = WriteFile(m_file_handle, data, data_size, &bytes_written, NULL);
		return result ? Result::SUCCESS : Result::ERROR_CANNOT_WRITE_FILE;
	}


	// Close the file.
	Result DirectDiskOutputStream::Stop()
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
	//   this is the same as start, except open file for appending if it exists...
	Result DirectDiskOutputStream::Continue()
	{
		// Can't start if already reading.
		if (m_file_handle != INVALID_HANDLE_VALUE)
		{
			return Result::ERROR_OPERATION_IN_PROGRESS;
		}

		// Open file to write to - fails if file doesn't exist!
		m_file_handle = CreateFileA(m_filename.c_str(), GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);

		// Move file ptr to actual end of file:
		if (m_file_handle != INVALID_HANDLE_VALUE)
		{
			SetFilePointer(m_file_handle,0,NULL,FILE_END);
		}

		return (m_file_handle == INVALID_HANDLE_VALUE)
			? Result::ERROR_CANNOT_OPEN_FILE
			: Result::SUCCESS;
	}

	//JKBNEW: state serialization support:
	Result DirectDiskOutputStream::SaveState(DirectDiskOutputStream* ddos)
	{
		bool fileInUse = false;
		if (m_file_handle != INVALID_HANDLE_VALUE)
		{
			// Write bool:
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

			this->Stop();
		}
		else
		{
			// Write bool:
			if (!ddos->Write((LPVOID)&fileInUse,(DWORD)sizeof(bool)))
				return Result::ERROR;
		}

		
		return Result::SUCCESS;
	}
	
	//JKBNEW: state serialization support:
	Result DirectDiskOutputStream::ResumeState(DirectDiskInputStream* ddis)
	{
		bool fileInUse = false;
		// Read bool:
		if (!ddis->Read((LPVOID)&fileInUse,(DWORD)sizeof(bool)))
			return Result::ERROR;

		if (fileInUse)
		{
			// read length of filename:
			int fnLen = 0;
			if (!ddis->Read((LPVOID)&fnLen,(DWORD)sizeof(int)))
				return Result::ERROR;

			// Now write filename:
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

			this->Continue();
		}
		else
		{
			// Write bool:
			if (!ddis->Read((LPVOID)&fileInUse,(DWORD)sizeof(bool)))
				return Result::ERROR;
		}
		
		return Result::SUCCESS;
	}


	// ================================================================
	//	BufferedAsynchronousDiskOutputStream
	//
	//	Interface for objects that supply a stream of data to a file on disk,
	//		using a buffer and/or separate thread.
	// ================================================================


	// Constructor.
	BufferedAsynchronousDiskOutputStream::BufferedAsynchronousDiskOutputStream() :
		// Buffer size is one Xbox HD sector.
		m_buffer_size(16384),
		m_use_thread(false),
		m_file_handle(INVALID_HANDLE_VALUE),
		m_data_buffer(NULL),
		m_write_buffer(NULL),
		m_write_thread_handle(NULL),
		m_write_buffer_full_event(NULL),
		m_write_buffer_empty_event(NULL)
	{
	}

	// Destructor.
	BufferedAsynchronousDiskOutputStream::~BufferedAsynchronousDiskOutputStream()
	{
	}


	// Open the file, initialize the buffers, and create the thread.
	Result BufferedAsynchronousDiskOutputStream::Start()
	{
		// Can't start if already reading.
		if (m_file_handle != INVALID_HANDLE_VALUE)
		{
			return Result::ERROR_OPERATION_IN_PROGRESS;
		}

		// Open the file.
		// Note the FILE_FLAG_NO_BUFFERING.  Buffered reads must be sector-sized.
		m_file_handle = CreateFile(m_filename.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_NO_BUFFERING, NULL);
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

		// Setup for asynchronous (if there is a need for it).
		if (m_use_thread)
		{
			// A second buffer will be needed to write to while writing the other one to disk.
			m_write_buffer = new (std::nothrow) BYTE[m_buffer_size];
			if (m_write_buffer == NULL)
			{
				cleanup_resources();
				return Result::ERROR_OUT_OF_MEMORY;
			}

			// Create events to signal buffer full and empty.
			m_write_buffer_full_event = CreateEvent(NULL, FALSE, FALSE, NULL);
			m_write_buffer_empty_event = CreateEvent(NULL, FALSE, TRUE, NULL);
			if (m_write_buffer_full_event == NULL || m_write_buffer_empty_event == NULL)
			{
				cleanup_resources();
				return Result::ERROR_UNKNOWN;
			}

			// Fire up thread.
			m_write_thread_done = false;
			m_write_thread_handle = CreateThread(NULL, 0, write_thread, this, 0, NULL);
			if (m_write_thread_handle == NULL)
			{
				cleanup_resources();
				return Result::ERROR_UNKNOWN;
			}
		}

		return Result::SUCCESS;
	}


	// Write to the buffer, switch to the spare buffer as needed, and wait for data reads.
	Result BufferedAsynchronousDiskOutputStream::Write(LPCVOID data, DWORD data_size)
	{
		// Can't read if not started.
		if (m_file_handle == INVALID_HANDLE_VALUE)
		{
			return Result::ERROR_CANNOT_OPEN_FILE;
		}

		BYTE *source_buffer = (BYTE *) data;

		// Will we fill up the current buffer with this data?
		DWORD bytes_left_in_buffer;
		while (data_size >= (bytes_left_in_buffer = m_buffer_size - m_data_buffer_index))
		{
			// Note that this loop handles the case where the data buffer gets exactly filled up.
			// Fill the buffer as much as we can.
			memcpy(m_data_buffer + m_data_buffer_index, source_buffer, m_buffer_size - m_data_buffer_index);

			// Flush this full buffer to disk.
			Result result;
			if (m_use_thread)
			{
				// Swap buffers and set event to trigger thread to do writing.
				//	Note that we wait for the other buffer's write to complete here,
				//		but likely this will never actually stall (given the buffer is big enough).
				//		To update the FAT, update cluster mappings, and write the file data would probably take
				//		no more than (20ms + 2ms) * 5 = 110ms, for a 16kb buffer, if no other I/O is going on.
				//		This is less than 7 frames of a 60 fps game,
				//		which cannot possibly generate 16kb of recording data.
				WaitForSingleObject(m_write_buffer_empty_event, INFINITE);
				BYTE *temp_buffer = m_write_buffer;
				m_write_buffer = m_data_buffer;
				m_data_buffer = temp_buffer;
				SetEvent(m_write_buffer_full_event);
			}
			else
			{
				// Just write directly to disk.
				DWORD bytes_written;
				// RWB - Fix for bug #350: Since we are flushing the buffer, the number of bytes
				// to write is m_buffer_size, not data_size. The latter will cause the write to fail,
				// since it is typically not a multiple of the sector size. Also, the data to be written
				// is m_data_buffer, not data.
				BOOL write_result = WriteFile(m_file_handle, m_data_buffer, m_buffer_size, &bytes_written, NULL);
				result = !write_result ? Result::ERROR_CANNOT_WRITE_FILE : Result::SUCCESS;
			}
			if (!result)
			{
				return result;
			}

			// Update the indices.
			source_buffer += bytes_left_in_buffer;
			data_size -= bytes_left_in_buffer;
			m_data_buffer_index = 0;
		}

		// Fill the buffer with what's left.
		memcpy(m_data_buffer + m_data_buffer_index, source_buffer, data_size);
		m_data_buffer_index += data_size;

		return Result::SUCCESS;
	}


	Result BufferedAsynchronousDiskOutputStream::Stop()
	{
		// Can't stop if it was never started.
		if (m_file_handle == INVALID_HANDLE_VALUE)
		{
			return Result::ERROR_OPERATION_NOT_IN_PROGRESS;
		}

		// Signal and wait for thread to finish.
		if (m_use_thread && m_write_thread_handle)
		{
			// Wait until the write buffer is empty.
			WaitForSingleObject(m_write_buffer_empty_event, INFINITE);
			// Set the quit flag.
			m_write_thread_done = true;
			// Trigger thread so it looks at the quit flag.
			SetEvent(m_write_buffer_full_event);
			// Wait for the thread to quit.
			WaitForSingleObject(m_write_thread_handle, INFINITE);
		}

		// Write anything that's left (pad buffer with 0s to sector granularity).
		Result result = Result::SUCCESS;
		DWORD sector_granular_buffer_size = (m_data_buffer_index + XBOX_HD_SECTOR_SIZE - 1) / XBOX_HD_SECTOR_SIZE * XBOX_HD_SECTOR_SIZE;
		if (sector_granular_buffer_size >= 0 && m_data_buffer != NULL)
		{
			memset(m_data_buffer + m_data_buffer_index, 0, sector_granular_buffer_size - m_data_buffer_index);
			DWORD bytes_written;
			BOOL write_result = WriteFile(m_file_handle, m_data_buffer, sector_granular_buffer_size, &bytes_written, NULL);
			result = !write_result ? Result::ERROR_CANNOT_WRITE_FILE : Result::SUCCESS;
		}

		cleanup_resources();
		
		return result;
	}


	// Set size of buffer.
	//	parameters:
	//		size - size to use for the buffer.  It will be rounded up to XBOX_HD_SECTOR_SIZE granularity.
	Result BufferedAsynchronousDiskOutputStream::SetBufferSize(DWORD size)
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
	Result BufferedAsynchronousDiskOutputStream::SetAsynchronous(bool use_asynch)
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



	// Clean up resources.
	void BufferedAsynchronousDiskOutputStream::cleanup_resources()
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
		delete [] m_write_buffer;
		m_write_buffer = NULL;

		if (m_write_thread_handle != NULL)
		{
			CloseHandle(m_write_thread_handle);
			m_write_thread_handle = NULL;
		}

		if (m_write_buffer_full_event != NULL)
		{
			CloseHandle(m_write_buffer_full_event);
			m_write_buffer_full_event = NULL;
		}

		if (m_write_buffer_empty_event != NULL)
		{
			CloseHandle(m_write_buffer_empty_event);
			m_write_buffer_empty_event = NULL;
		}
	}


	// This is the thread that does all the writing.
	DWORD WINAPI BufferedAsynchronousDiskOutputStream::write_thread(LPVOID lpParameter)
	{
		BufferedAsynchronousDiskOutputStream *that = (BufferedAsynchronousDiskOutputStream *) lpParameter;
		for (;;)
		{
			// Wait for them to fill a buffer for us or tell us it's time to quit.
			WaitForSingleObject(that->m_write_buffer_full_event, INFINITE);
			// Is it time to quit?
			if (that->m_write_thread_done)
			{
				// Quit without writing.  Final write is done in finish_output().
				break;
			}
			// Write it.
			// TODO: Now that we have asynchronous notification, we should handle errors here...
			DWORD bytes_written;
			WriteFile(that->m_file_handle, that->m_write_buffer, that->m_buffer_size, &bytes_written, NULL);
			// Notify them they can use this buffer again.
			SetEvent(that->m_write_buffer_empty_event);
		}
		return 0;
	}

	//JKBNEW: state serialization support:
	//   this is the same as start, except open file for appending if it exists...
	Result BufferedAsynchronousDiskOutputStream::Continue()
	{
		// Can't start if already reading.
		if (m_file_handle != INVALID_HANDLE_VALUE)
		{
			return Result::ERROR_OPERATION_IN_PROGRESS;
		}

		// Open the file.
		// Note the FILE_FLAG_NO_BUFFERING.  Buffered reads must be sector-sized.
		// NOTE: OPEN_EXISTING instead of CREATE_ALWAYS
		m_file_handle = CreateFileA(m_filename.c_str(), GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_NO_BUFFERING, NULL);
		if (m_file_handle == INVALID_HANDLE_VALUE)
		{
			return Result::ERROR_CANNOT_OPEN_FILE;
		}

		// Move file ptr to actual end of file:
		if (m_file_handle != INVALID_HANDLE_VALUE)
		{
			long numPaddedZeros = -(long)m_numPaddedZeros;
			SetFilePointer(m_file_handle,numPaddedZeros,NULL,FILE_END);
		}

		// Setup for asynchronous (if there is a need for it).
		if (m_use_thread)
		{
			// Create events to signal buffer full and empty.
			m_write_buffer_full_event = CreateEvent(NULL, FALSE, FALSE, NULL);
			m_write_buffer_empty_event = CreateEvent(NULL, FALSE, TRUE, NULL);
			if (m_write_buffer_full_event == NULL || m_write_buffer_empty_event == NULL)
			{
				cleanup_resources();
				return Result::ERROR_UNKNOWN;
			}

			// Fire up thread.
			m_write_thread_handle = CreateThread(NULL, 0, write_thread, this, 0, NULL);
			if (m_write_thread_handle == NULL)
			{
				cleanup_resources();
				return Result::ERROR_UNKNOWN;
			}
		}

		return Result::SUCCESS;
	}

	//JKBNEW: state serialization support:
	Result BufferedAsynchronousDiskOutputStream::SaveState(DirectDiskOutputStream* ddos)
	{
		bool fileInUse = false;
		if (m_file_handle != INVALID_HANDLE_VALUE)
		{
			// Write bool:
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

			// Have to record the #bytes of 0's to be padded onto the end of the file, so we can revert:
			DWORD sector_granular_buffer_size = (m_data_buffer_index + XBOX_HD_SECTOR_SIZE - 1) / XBOX_HD_SECTOR_SIZE * XBOX_HD_SECTOR_SIZE;
			DWORD sizePaddedZeros = sector_granular_buffer_size - m_data_buffer_index;
			if (!ddos->Write((LPVOID)&sizePaddedZeros,(DWORD)sizeof(DWORD)))
				return Result::ERROR;

		}
		else
		{
			// Write bool:
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

		// Buffer we are writing to disk now.
		bool writeBufNotNull = false;
		if (m_write_buffer != NULL)
		{
			writeBufNotNull = true;
			if (!ddos->Write((LPVOID)&writeBufNotNull,(DWORD)sizeof(bool)))
				return Result::ERROR;

			if (!ddos->Write((LPVOID)m_write_buffer,(DWORD)m_buffer_size))
				return Result::ERROR;
		}
		else
		{
			if (!ddos->Write((LPVOID)&writeBufNotNull,(DWORD)sizeof(bool)))
				return Result::ERROR;
		}

		// Where the data buffer pointer is currently located.
		if (!ddos->Write((LPVOID)&m_data_buffer_index,(DWORD)sizeof(DWORD)))
			return Result::ERROR;

		// Time for thread to quit?
		if (!ddos->Write((LPVOID)&m_write_thread_done,(DWORD)sizeof(bool)))
			return Result::ERROR;

		this->Stop();

		return Result::SUCCESS;
	}
	
	//JKBNEW: state serialization support:
	Result BufferedAsynchronousDiskOutputStream::ResumeState(DirectDiskInputStream* ddis)
	{
		bool fileInUse = false;
		// read bool:
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

			// Read in the #bytes of padded 0's from SaveState:
			if (!ddis->Read((LPVOID)&m_numPaddedZeros,(DWORD)sizeof(DWORD)))
				return Result::ERROR;
		}

		// Buffer size.
		if (!ddis->Read((LPVOID)&m_buffer_size,(DWORD)sizeof(DWORD)))
			return Result::ERROR;

		// Should use a thread to do I/O?
		if (!ddis->Read((LPVOID)&m_use_thread,(DWORD)sizeof(bool)))
			return Result::ERROR;

		// Buffer we are filling now in memory.
		m_data_buffer = new (std::nothrow) BYTE[m_buffer_size];
		// first check if it's null:
		bool dataBufNotNull = false;
		if (!ddis->Read((LPVOID)&dataBufNotNull,(DWORD)sizeof(bool)))
			return Result::ERROR;
		// now read buffer if ok:
		if (dataBufNotNull)
			if (!ddis->Read((LPVOID)m_data_buffer,(DWORD)m_buffer_size))
				return Result::ERROR;

		// Buffer we are writing to disk now.
		m_write_buffer = new (std::nothrow) BYTE[m_buffer_size];
		// first check write buffer to be null:
		bool writeBufNotNull = false;
		if (!ddis->Read((LPVOID)&writeBufNotNull,(DWORD)sizeof(bool)))
			return Result::ERROR;
		// now read buffer if ok:
		if (writeBufNotNull)
			if (!ddis->Read((LPVOID)m_write_buffer,(DWORD)m_buffer_size))
				return Result::ERROR;

		// Where the data buffer pointer is currently located.
		if (!ddis->Read((LPVOID)&m_data_buffer_index,(DWORD)sizeof(DWORD)))
			return Result::ERROR;

		// Time for thread to quit?
		if (!ddis->Read((LPVOID)&m_write_thread_done,(DWORD)sizeof(bool)))
			return Result::ERROR;

		this->Continue();

		return Result::SUCCESS;
	}

}

#endif // _XENON_UTILITY_