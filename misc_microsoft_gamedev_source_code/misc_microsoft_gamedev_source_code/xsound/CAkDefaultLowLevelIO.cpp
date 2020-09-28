//////////////////////////////////////////////////////////////////////
//
// AkDefaultLowLevelIO.cpp
//
// Default XBox360 low level IO file system implementation.
// Calls platform API for IO, in blocking and overlapped
// fashion, according to the device that is currently set.
// File localization: Uses simple path concatenation logic.
// Exposes basic path functions for convenience.
// 
// Copyright (c) 2006-2007 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "common.h"
//#include "stdafx.h"
#include "CAkDefaultLowLevelIO.h"
#include <AK/Plugin/AkVorbisFactory.h>

#ifndef BUILD_FINAL
#include "timer.h"
#endif

#include <malloc.h>
#include <assert.h>
#include <stdio.h>
#define UNICODE

template< class TYPE > inline
const TYPE& AkTemplMax( const TYPE& in_left, const TYPE& in_right )
{
	return ( in_left < in_right ) ? in_right : in_left;
}

#define MAX_NUMBER_STRING_SIZE      (10)    // 4G
#define ID_TO_STRING_FORMAT_BANK    (L"%u.bnk")
#define ID_TO_STRING_FORMAT_WAV     (L"%u.wav")
#define ID_TO_STRING_FORMAT_XMA     (L"%u.xma")
#define ID_TO_STRING_FORMAT_VORBIS  (L"%u.ogg")
#define MAX_EXTENSION_SIZE          (4)     // .wav
#define MAX_FILETITLE_SIZE          (MAX_NUMBER_STRING_SIZE+MAX_EXTENSION_SIZE+1)   // null-terminated

// Default devices info.
#define XBOX360_DVD_SECTOR_SIZE     (2048)
#define XBOX360_DEVICE_DVD_NAME     (L"DVD")
#define XBOX360_DEVICE_HDD_NAME     (L"Cache drive")
#define CACHE_SIZE                  (128*1024)
#define CLUSTER_SIZE                (32*1024)

CAkDefaultLowLevelIO::CAkDefaultLowLevelIO( ) 
{
}

CAkDefaultLowLevelIO::~CAkDefaultLowLevelIO( )
{
}

AKRESULT CAkDefaultLowLevelIO::Init( )
{
	m_szBasePath[0] = NULL;
    m_szBankPath[0] = NULL;
    m_szAudioSrcPath[0] = NULL;
    m_szLangSpecificDirName[0] = NULL;
    m_curDeviceID = AK_INVALID_DEVICE_ID;
    m_uBlockSize= 0;

	// Do not mount the utility drive more then once
	static bool bIsMounted = false;

	// Init HD for writing.
	if(!bIsMounted)
	{
		DWORD dwError;
		dwError = XMountUtilityDrive( TRUE, CLUSTER_SIZE, CACHE_SIZE );
		if ( dwError == ERROR_SUCCESS )
		{
			bIsMounted = true; // Done only once
		}
	}

	return AK_Success;
}

void CAkDefaultLowLevelIO::Term( )
{
}

void CAkDefaultLowLevelIO::WaitWhilePaused()
{
   // rg - If we're not using archives there's no use trying to stop the sound engine from accessing the disk.
   if ((gFileManager.getConfigFlags() & BFileManager::cWillBeUsingArchives) == 0)
      return;
   if (gFileManager.getConfigFlags() & BFileManager::cEnableLooseFiles)
      return;
      
#ifndef BUILD_FINAL
   BTimer timer;
   timer.start();
#endif
   
   // This delay will be quite slow when XFS with loose files is enabled! FIXME: 
   // FIXME: Optimize this call to be lock-free.
   gFileManager.waitUntilAllCachesAreLoadedOrFailed();   
   
   while (gFileManager.isIOPaused())
   {
      Sleep(1);
   }
         
#ifndef BUILD_FINAL
   double totalTime = timer.getElapsedSeconds();
   if (totalTime > .0005f)
   {
      BFixedString<256> buf;
      buf.format("CAkDefaultLowLevelIO: Thread ID 0x%08X was gated for %3.3fms by the file manager\n", (DWORD)GetCurrentThreadId(), totalTime * 1000.0f);
      
      // We cannot use trace() here! It uses too much stack.
      OutputDebugStringA(buf.getPtr());
   }      
#endif
}

//-----------------------------------------------------------------------------
// Returns a file descriptor for a given file name (string).
// Performs the operations needed to make the file descriptor usable by
// the other methods of the interface (e.g. ask the OS for a valid file handle).
// Returns:
//     - AK_Success:       A valid file descriptor is returned
//     - AK_FileNotFound:  File was not found.
//     - AK_Fail:          File could not be open for any other reason.
// Returns: A file descriptor, that contains 
//         - an unique identifier to be used with functions of the low-level IO 
//           interface.
//         - the total stream size in bytes.
//         - the offset from the beginning of the file (in blocks).
//         - a device ID, that was obtained through AK::IAkStreamMgr::CreateDevice().
//-----------------------------------------------------------------------------
AKRESULT CAkDefaultLowLevelIO::Open( 
    AkLpCtstr       in_pszFileName,     // File name.
    AkOpenMode      in_eOpenMode,       // Open mode.
    AkFileSystemFlags * in_pFlags,      // Special flags. Can pass NULL.
    AkFileDesc &    out_fileDesc        // Returned file descriptor.
    )
{
   WaitWhilePaused();
   
    // Get the full file path, using path concatenation logic.
    out_fileDesc.iFileSize = 0;
    AkTChar szFullFilePath[AK_MAX_PATH];
    AKRESULT eRes = GetFullFilePath(
                        in_pszFileName,
                        in_pFlags,
                        out_fileDesc.uSector,
                        szFullFilePath );

    if ( eRes != AK_Success )
        return AK_Fail;

    bool bSyncIO = ( m_eCurDeviceType == DeviceHDD );

    eRes = OpenFile( szFullFilePath,
                     in_eOpenMode,
                     bSyncIO,
                     !bSyncIO,   // Hard drive device is hereby configured as using buffered IO.
                     out_fileDesc.hFile );

    if ( eRes != AK_Success )
        return AK_Fail;

    out_fileDesc.pCustomParam         = NULL;
    out_fileDesc.uCustomParamSize     = 0;
    
	ULARGE_INTEGER Temp;
	Temp.LowPart = ::GetFileSize( out_fileDesc.hFile,(LPDWORD)&Temp.HighPart );
	out_fileDesc.iFileSize	= Temp.QuadPart;
    out_fileDesc.uSector              = 0;
    out_fileDesc.deviceID             = m_curDeviceID;

    return AK_Success;
}

//-----------------------------------------------------------------------------
// Returns a file descriptor for a given file ID.
// Performs the operations needed to make the file descriptor usable by
// the other methods of the interface (e.g. ask the OS for a valid file handle).
// Returns: 
//     - AK_Success:       A valid file descriptor is returned
//     - AK_FileNotFound:  File was not found.
//     - AK_Fail:          File could not be open for any other reason.
// Returns: A file descriptor, that contains 
//         - an unique identifier to be used with functions of the low-level IO 
//           interface.
//         - the total stream size in bytes.
//         - the offset of the beginning of the file (in blocks). 
//         - a device ID, that was obtained through AK::IAkStreamMgr::CreateDevice().
//-----------------------------------------------------------------------------
AKRESULT CAkDefaultLowLevelIO::Open( 
    AkFileID        in_fileID,          // File ID.
    AkOpenMode      in_eOpenMode,       // Open mode.
    AkFileSystemFlags * in_pFlags,      // Special flags. Can pass NULL.
    AkFileDesc &    out_fileDesc        // Returned file descriptor.
    )
{
   WaitWhilePaused();
   
    // Get the full file path, using path concatenation logic.
    out_fileDesc.iFileSize = 0;
    AkTChar szFullFilePath[AK_MAX_PATH];
    AKRESULT eRes = GetFullFilePath(
                        in_fileID,
                        in_pFlags,
                        out_fileDesc.uSector,
                        szFullFilePath );

    if ( eRes != AK_Success )
        return AK_Fail;

    bool bSyncIO = ( m_eCurDeviceType == DeviceHDD );

    eRes = OpenFile( szFullFilePath,
                     in_eOpenMode,
                     bSyncIO,
                     !bSyncIO,
                     out_fileDesc.hFile );

    if ( eRes != AK_Success )
        return AK_Fail;

    out_fileDesc.pCustomParam         = NULL;
    out_fileDesc.uCustomParamSize     = 0;

	ULARGE_INTEGER Temp;
	Temp.LowPart = ::GetFileSize( out_fileDesc.hFile,(LPDWORD)&Temp.HighPart );

	out_fileDesc.iFileSize	= Temp.QuadPart;
    out_fileDesc.uSector              = 0;
    out_fileDesc.deviceID             = m_curDeviceID;

    return AK_Success;
}

//-----------------------------------------------------------------------------
// Name: Close()
// Desc: Cleans up the given file.
// Return: AK_Success or AK_Fail.
//-----------------------------------------------------------------------------
AKRESULT CAkDefaultLowLevelIO::Close(
    const AkFileDesc & in_fileDesc      ///< File descriptor.
    )
{
    if ( !::CloseHandle( in_fileDesc.hFile ) )
    {
        assert( !"Failed to close file handle" );
        return AK_Fail;
    }
    return AK_Success;
}

//-----------------------------------------------------------------------------
// Reads data from a file.
// Remark: AkIOTransferInfo is a platform-specific structure that may be 
// used to pass file position and synchronization objects. Users will 
// pass the same address when calling AK::IAkLowLevelIO::GetAsyncResult().
// Remark: If the high-level device is non-blocking, 
// io_transferInfo.pOverlapped->hEvent will contain a valid event.
// The Low-Level IO module is responsible for signaling that event.
// Then AK::IAkLowLevelIO::GetAsyncResult() is called by the Stream Manager.
// If the high-level device is blocking, io_transferInfo.pOverlapped->hEvent
// wil be NULL, and this method must return only when I/O request is complete 
// (whether it failed or not). It is thus not allowed to return AK_NoDataReady.
// Remark: File position is computed by the high-level device as 
// io_fileDesc.uSector * Block_Size + Stream_Position. Block size is obtained via GetBlockSize(). 
// Returns:
//     - AK_DataReady:   out_pBuffer is filled with data (amount = out_uSizeRead).
//     - AK_NoMoreData:  out_pBuffer is filled with data (can be 0 bytes), but reached EOF.
//     - AK_NoDataReady: IO is pending: the Stream Manager will call GetAsyncResult() after 
//						  Low-Level IO signaled io_transferInfo.pOverlapped->hEvent
//						  (applies to non-blocking devices only).
//     - AK_Fail:        an error occured.
//-----------------------------------------------------------------------------
AKRESULT CAkDefaultLowLevelIO::Read(
    AkFileDesc &    io_fileDesc,        ///< File descriptor.
    void *          out_pBuffer,        ///< Buffer to be filled with data.
    AkIOTransferInfo & io_transferInfo  ///< Platform-specific IO data transfer info. 
    )
{
    assert( out_pBuffer &&
            io_fileDesc.hFile != INVALID_HANDLE_VALUE );

    WaitWhilePaused();
    
    AKRESULT eResult = AK_DataReady;
    BOOL bOk;
    if ( !io_transferInfo.pOverlapped->hEvent )
    {
        // File was open with synchronous flag. Just read.
        // But set position first, if it differs from current file position.
        if ( !io_transferInfo.bIsSequential )
        {
            LARGE_INTEGER iOffset;
            iOffset.LowPart = io_transferInfo.pOverlapped->Offset;
            iOffset.HighPart = io_transferInfo.pOverlapped->OffsetHigh;
            if ( !::SetFilePointerEx( io_fileDesc.hFile, 
                                      iOffset,
                                      NULL,
                                      FILE_BEGIN ) )
            {
                return AK_Fail;
            }
        }

        bOk = ::ReadFile( io_fileDesc.hFile,
                          out_pBuffer,
                          io_transferInfo.uTransferSize,
                          &io_transferInfo.uSizeTransferred,
                          NULL );   // Not Overlapped
        if ( io_transferInfo.uSizeTransferred < io_transferInfo.uTransferSize )
            eResult = AK_NoMoreData;
    }
    else
    {
        // File was open with asynchronous flag. 
        // Read overlapped. Explicitely wait for event if in_bWait.
        bOk = ::ReadFile( io_fileDesc.hFile,
                          out_pBuffer,
                          io_transferInfo.uTransferSize,
                          &io_transferInfo.uSizeTransferred,
                          io_transferInfo.pOverlapped );   // Overlapped.

        if ( !bOk )
        {
            // IO not completed.
            DWORD dwError = ::GetLastError( );
            if ( dwError == ERROR_HANDLE_EOF )
            {
                bOk = TRUE; // Set to TRUE. EOF is not an error.
                eResult = AK_NoMoreData;
            }
            else if ( dwError == ERROR_IO_PENDING )
            {
                bOk = TRUE; // Set to TRUE. Pending IO is not an error.
                eResult = AK_NoDataReady;
            }

            // else there was an error.
        }
    }

    if ( !bOk )
    {
        return AK_Fail;
    }
    return eResult;
}

//-----------------------------------------------------------------------------
// Writes data to a file.
// Remarks: AkIOTransferInfo is a platform-specific structure that may be 
// used to pass file position and synchronization objects. Users will 
// pass the same address when calling AK::IAkLowLevelIO::GetAsyncResult().
// Remarks: If the high-level device is non-blocking, 
// io_transferInfo.pOverlapped->hEvent will contain a valid event.
// The Low-Level IO module is responsible for signaling that event.
// Then AK::IAkLowLevelIO::GetAsyncResult() is called by the Stream Manager.
// If the high-level device is blocking, io_transferInfo.pOverlapped->hEvent
// wil be NULL, and this method must return only when I/O request is complete 
// (whether it failed or not). It is thus not allowed to return AK_NoDataReady.
// Remarks: File position is computed by the high-level device as 
// io_fileDesc.uSector * Block_Size + Stream_Position. Block size is obtained via GetBlockSize().
// Returns:
//     - AK_DataReady:   out_pBuffer is filled with data (amount = out_uSizeRead).
//     - AK_NoDataReady: IO is pending: the Stream Manager will call GetAsyncResult() after 
//						  Low-Level IO signaled io_transferInfo.pOverlapped->hEvent
//						  (applies to non-blocking devices only).
//     - AK_Fail:        an error occured.
//-----------------------------------------------------------------------------
AKRESULT CAkDefaultLowLevelIO::Write(
    AkFileDesc &    io_fileDesc,        ///< File descriptor.
    void *          in_pData,           ///< Data to be written.
    AkIOTransferInfo & io_transferInfo  ///< Platform-specific IO operation info. 
    )
{
    assert( in_pData && 
            io_fileDesc.hFile != INVALID_HANDLE_VALUE );

    WaitWhilePaused();
    
    AKRESULT eResult = AK_DataReady;
    BOOL bOk;
    if ( !io_transferInfo.pOverlapped->hEvent )
    {
        // File was open with synchronous flag. Just read.
        // But set position first, if it differs from current file position.
        if ( !io_transferInfo.bIsSequential )
        {
            LARGE_INTEGER iOffset;
            iOffset.LowPart = io_transferInfo.pOverlapped->Offset;
            iOffset.HighPart = io_transferInfo.pOverlapped->OffsetHigh;
            if ( !::SetFilePointerEx( io_fileDesc.hFile, 
                                      iOffset,
                                      NULL,
                                      FILE_BEGIN ) )
            {
                return AK_Fail;
            }
        }

        bOk = ::WriteFile( io_fileDesc.hFile,
                           in_pData,
                           io_transferInfo.uTransferSize,
                           &io_transferInfo.uSizeTransferred,
                           NULL );   // Not Overlapped
    }
    else
    {
        // File was open with asynchronous flag. 
        // Read overlapped. Explicitely wait for event if in_bWait.
        bOk = ::WriteFile( io_fileDesc.hFile,
                           in_pData,
                           io_transferInfo.uTransferSize,
                           &io_transferInfo.uSizeTransferred,
                           io_transferInfo.pOverlapped );   // Overlapped.

        if ( !bOk )
        {
            // IO not completed.
            DWORD dwError = ::GetLastError( );
            if ( dwError == ERROR_IO_PENDING )
            {
                bOk = TRUE; // Set to TRUE. Pending IO is not an error.
                eResult = AK_NoDataReady;
            }

            // else there was an error.
        }
    }

    if ( !bOk )
    {
        return AK_Fail;
    }
    return eResult;
}

//-----------------------------------------------------------------------------
// Polling method for pending I/O operations.
// Used by non-blocking high-level devices. 
// Remarks: io_transferInfo.pOverlapped->hEvent always contains a valid event.
// The Low-Level IO module is responsible for signaling that event.
// Then this method is called by the Stream Manager.
// Returns:
//     - AK_DataReady:     buffer passed to Read() or Write() is ready.
//     - AK_NoMoreData:    buffer passed to Read() is filled with data 
//                         (can be 0 bytes), but reached EOF.
//     - AK_Fail:          an error occured.
//-----------------------------------------------------------------------------
AKRESULT CAkDefaultLowLevelIO::GetAsyncResult(
    AkFileDesc &    io_fileDesc,        ///< File descriptor.
    AkIOTransferInfo & io_transferInfo  ///< Platform-specific IO operation info. 
    )
{
    assert( io_fileDesc.hFile != INVALID_HANDLE_VALUE );
    assert( io_transferInfo.pOverlapped->hEvent );

    BOOL bOk;
    bOk = ::GetOverlappedResult( io_fileDesc.hFile, 
                                 io_transferInfo.pOverlapped, 
                                 &io_transferInfo.uSizeTransferred, 
                                 FALSE );

    if ( !bOk )
    {
        DWORD dwError = ::GetLastError( );
        if ( dwError == ERROR_HANDLE_EOF )
            return AK_NoMoreData;
        else if ( dwError == ERROR_IO_PENDING ||
                  dwError == ERROR_IO_INCOMPLETE )
            return AK_NoDataReady;
        else
        {
            return AK_Fail;
        }
    }

    return AK_DataReady;
}

//-----------------------------------------------------------------------------
// Returns the block size for the file or its storage device. 
// The block size is a constraint for clients
// of the Stream Manager: All reads, writes and position changes need to be a multiple of
// that size.
// Return: The block size for a specific file or storage device.
// Remark: Some files might be open with flags that require IO transfers to be a multiple 
//         of this size. The stream manager will query this function to resolve calls 
//         to IAk(Auto)Stream::GetBlockSize( ).
//         Also, AkFileDesc::uSector specifies a number of sectors in multiples
//         of this value.
//         Files/IO devices that do not require byte alignment should return 1.
//-----------------------------------------------------------------------------
AkUInt32 CAkDefaultLowLevelIO::GetBlockSize(
    const AkFileDesc &  in_fileDesc     ///< File descriptor.
    )
{
    return m_uBlockSize;
}


//-----------------------------------------------------------------------------
// Name: GetDeviceDesc()
// Return: Returns a device description for a given device ID.
//         For profiling purposes only.
//-----------------------------------------------------------------------------
#ifndef AK_OPTIMIZED
AKRESULT CAkDefaultLowLevelIO::GetDeviceDesc(
    AkDeviceID      in_deviceID,        ///< High-level device ID.
    AkDeviceDesc &  out_deviceDesc      ///< Description of associated low-level I/O device.
    )
{
    // This implementation supports only one device at a time.
    if ( in_deviceID != m_curDeviceID )
        return AK_InvalidParameter;

    if ( m_eCurDeviceType == DeviceHDD )
    {
        // Blocking scheduler: consider this as a hard drive.
        out_deviceDesc.deviceID       = m_curDeviceID;
        out_deviceDesc.bCanRead       = true;
        out_deviceDesc.bCanWrite      = true;
        wcscpy( out_deviceDesc.szDeviceName, XBOX360_DEVICE_HDD_NAME );
        out_deviceDesc.uStringSize   = (AkUInt32)wcslen( out_deviceDesc.szDeviceName ) + 1;
    }
    else if ( m_eCurDeviceType == DeviceDVD )
    {
        // Deferred scheduler: consider this as a DVD.
        out_deviceDesc.deviceID       = m_curDeviceID;
        out_deviceDesc.bCanRead       = true;
        out_deviceDesc.bCanWrite      = false;
        wcscpy( out_deviceDesc.szDeviceName, XBOX360_DEVICE_DVD_NAME );
        out_deviceDesc.uStringSize   = (AkUInt32)wcslen( out_deviceDesc.szDeviceName ) + 1;
    }
    else
    {
        assert( !"Invalid device" );
        return AK_Fail;
    }

    return AK_Success;
}
#endif


// Wrapper for Win32 CreateFile().
AKRESULT CAkDefaultLowLevelIO::OpenFile( 
    AkLpCtstr       in_pszFilename,     // File name.
    AkOpenMode      in_eOpenMode,       // Open mode.
    bool            in_bSynchronousIO,  // Synchronous IO flag.
    bool            in_bUnbufferedIO,   // Unbuffered IO flag.
    AkFileHandle &  out_hFile           // Returned file identifier/handle.
    )
{
    // Check parameters.
    if ( !in_pszFilename )
    {
        assert( !"NULL file name" );
        return AK_InvalidParameter;
    }
    
    WaitWhilePaused();

    // Open mode
    DWORD dwShareMode;
    DWORD dwAccessMode;
    DWORD dwCreationDisposition;
    switch ( in_eOpenMode )
    {
        case AK_OpenModeRead:
                dwShareMode = FILE_SHARE_READ;
                dwAccessMode = GENERIC_READ;
                dwCreationDisposition = OPEN_EXISTING;
            break;
        case AK_OpenModeWrite:
                dwShareMode = FILE_SHARE_WRITE;
                dwAccessMode = GENERIC_WRITE;
                dwCreationDisposition = OPEN_ALWAYS;
            break;
        case AK_OpenModeWriteOvrwr:
                dwShareMode = FILE_SHARE_WRITE;
                dwAccessMode = GENERIC_WRITE;
                dwCreationDisposition = CREATE_ALWAYS;
            break;
        case AK_OpenModeReadWrite:
                dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
                dwAccessMode = GENERIC_READ | GENERIC_WRITE;
                dwCreationDisposition = OPEN_ALWAYS;
            break;
        default:
                assert( !"Invalid open mode" );
                out_hFile = NULL;
                return AK_InvalidParameter;
            break;
    }

    // Flags
    DWORD dwFlags = FILE_FLAG_SEQUENTIAL_SCAN;
    if ( in_bUnbufferedIO )
        dwFlags |= FILE_FLAG_NO_BUFFERING;
    if ( !in_bSynchronousIO )
        dwFlags |= FILE_FLAG_OVERLAPPED;

// String conversion.
#ifdef UNICODE
	// ::CreateFileW does not exist: convert wstring to ansi string.
    int iConvRet;
    size_t uiNameSize = wcslen( in_pszFilename ) + 1;
    char * pszFileNameA = (char *) _alloca( uiNameSize );
    iConvRet = ::WideCharToMultiByte( CP_ACP, // code page
                                0, // performance and mapping flags
                                in_pszFilename, // wide-character string
                                uiNameSize, // number of chars in string : -1 = NULL terminated string.
                                pszFileNameA, // buffer for new string
                                uiNameSize, // size of buffer
                                NULL, // default for unmappable chars
                                NULL // set when default char used
                                );
    if ( iConvRet <= 0 )
    {
        assert( !"Could not convert file name string to ANSI" );
        return AK_Fail;
    }
#else
	LPCTSTR pszFileNameA = in_pszFilename;
#endif
	// Create the file handle.
    out_hFile = ::CreateFile( 
                            pszFileNameA,
                            dwAccessMode,
                            dwShareMode, 
                            NULL,
                            dwCreationDisposition,
                            dwFlags,
                            NULL );
    
	if( out_hFile == INVALID_HANDLE_VALUE )
    {
        DWORD dwAllocError = ::GetLastError( );
        if ( dwAllocError == ERROR_FILE_NOT_FOUND ||
             dwAllocError == ERROR_PATH_NOT_FOUND )
            return AK_FileNotFound;

		out_hFile = NULL;
        return AK_Fail;
    }

    return AK_Success;
}

// String overload.
AKRESULT CAkDefaultLowLevelIO::GetFullFilePath(
    AkLpCtstr           in_pszFileName, // File name.
    AkFileSystemFlags * in_pFlags,      // Special flags. Can pass NULL.
    AkUInt32 &          out_uSector,    // Start sector.
    AkTChar *           out_pszFullFilePath // Win32 file path.
    )
{
    if ( !in_pszFileName )
    {
        assert( !"Invalid file name" );
        return AK_InvalidParameter;
    }

    // Prepend string path (basic file system logic).

    // Compute file name with file system paths.
    size_t uiPathSize = wcslen( in_pszFileName );

    if ( uiPathSize >= AK_MAX_PATH )
    {
        assert( !"Input string too large" );
        return AK_InvalidParameter;        
    }
    
    wcscpy( out_pszFullFilePath, m_szBasePath );

    
    if ( in_pFlags )
    {
        // Add bank path if file is an AK sound bank.
        if ( in_pFlags->uCompanyID == AKCOMPANYID_AUDIOKINETIC &&
             in_pFlags->uCodecID == AKCODECID_BANK )
            wcscat( out_pszFullFilePath, m_szBankPath );

        // Note. Stream files do not use this overload.

        // Add language directory name if needed.
        if ( in_pFlags->bIsLanguageSpecific )
            wcscat( out_pszFullFilePath, m_szLangSpecificDirName );
    }
        
    // Append file title.
    uiPathSize += wcslen( out_pszFullFilePath );
    if ( uiPathSize >= AK_MAX_PATH )
    {
        assert( !"File name string too large" );
        return AK_Fail;        
    }
    wcscat( out_pszFullFilePath, in_pszFileName );
    return AK_Success;
}

// ID overload.
AKRESULT CAkDefaultLowLevelIO::GetFullFilePath(
    AkFileID            in_fileID,      // File ID.
    AkFileSystemFlags * in_pFlags,      // Special flags. Can pass NULL.
    AkUInt32 &          out_uSector,    // Start sector.
    AkTChar *           out_pszFullFilePath // Win32 file path.
    )
{
    // If the file descriptor could not be found, or if the script-based FS does not exist,
    // map file ID to file descriptor (string based) for Audiokinetic IDs.

    if ( !in_pFlags ||
         in_pFlags->uCompanyID != AKCOMPANYID_AUDIOKINETIC )
    {
        assert( !"Unhandled file type" );
        return AK_Fail;
    }

    // Compute file name with file system paths.
    
    // Copy base path. 
    wcscpy( out_pszFullFilePath, m_szBasePath );

    // Concatenate path for AK banks or streamed audio files (everything except banks).
     if ( in_pFlags->uCodecID == AKCODECID_BANK )
         wcscat( out_pszFullFilePath, m_szBankPath );
     else
         wcscat( out_pszFullFilePath, m_szAudioSrcPath );
    
    // Add language directory name if needed.
    if ( in_pFlags->bIsLanguageSpecific )
        wcscat( out_pszFullFilePath, m_szLangSpecificDirName );

    // Append file title.
    size_t uiPathSize = wcslen( out_pszFullFilePath );
    if ( ( uiPathSize + MAX_FILETITLE_SIZE ) <= AK_MAX_PATH )
    {
        AkTChar * pszTitle = out_pszFullFilePath + uiPathSize;
        if ( in_pFlags->uCodecID == AKCODECID_BANK )
			swprintf( pszTitle, ID_TO_STRING_FORMAT_BANK, in_fileID );
        else if ( in_pFlags->uCodecID == AKCODECID_XMA )
            swprintf( pszTitle, ID_TO_STRING_FORMAT_XMA, in_fileID );
		else if ( in_pFlags->uCodecID == AKCODECID_VORBIS )
            swprintf( pszTitle, ID_TO_STRING_FORMAT_VORBIS, in_fileID );
        else
            swprintf( pszTitle, ID_TO_STRING_FORMAT_WAV, in_fileID );
    }
    else
    {
        assert( !"String buffer too small" );
        return AK_Fail;
    }    
    
    return AK_Success;
}

AKRESULT CAkDefaultLowLevelIO::SetDevice(
    AkDeviceSettings & in_deviceSettings
    )
{
    if ( !AK::IAkStreamMgr::Get( ) )
    {
        assert( !"Stream manager must be initialized before creating devices" );
        return AK_Fail;
    }

    // Destroy current device.
    if ( m_curDeviceID != AK_INVALID_DEVICE_ID )
        AK::DestroyDevice( m_curDeviceID );

    // Create new scheduler.
    m_curDeviceID = AK::CreateDevice( &in_deviceSettings );
    if ( m_curDeviceID == AK_INVALID_DEVICE_ID )
        return AK_Fail;

    if ( ( in_deviceSettings.uSchedulerTypeFlags & AK_SCHEDULER_BLOCKING ) > 0 )
    {
        m_eCurDeviceType = DeviceHDD;
        m_uBlockSize     = 1;                       // Default block size for "HDD" device.
    }
    else
    {
        m_eCurDeviceType = DeviceDVD;
        m_uBlockSize     = XBOX360_DVD_SECTOR_SIZE;   // Default block size for "DVD" device.
    }
    return AK_Success;
}

AKRESULT CAkDefaultLowLevelIO::SetBasePath(
    AkLpCtstr   in_pszBasePath
    )
{
    if ( wcslen( in_pszBasePath ) + AkTemplMax( wcslen( m_szBankPath ), wcslen( m_szAudioSrcPath ) ) + wcslen( m_szLangSpecificDirName ) >= AK_MAX_PATH )
    {
        return AK_InvalidParameter;
    }
    wcscpy( m_szBasePath, in_pszBasePath );
    return AK_Success;
}
AKRESULT CAkDefaultLowLevelIO::SetBankPath(
    AkLpCtstr   in_pszBankPath
    )
{
    if ( wcslen( m_szBasePath ) + AkTemplMax( wcslen( in_pszBankPath ), wcslen( m_szAudioSrcPath ) ) + wcslen( m_szLangSpecificDirName ) >= AK_MAX_PATH )
    {
        return AK_InvalidParameter;
    }
    wcscpy( m_szBankPath, in_pszBankPath );
    return AK_Success;
}
AKRESULT CAkDefaultLowLevelIO::SetAudioSrcPath(
    AkLpCtstr   in_pszAudioSrcPath
    )
{
    if ( wcslen( m_szBasePath ) + AkTemplMax( wcslen( m_szBankPath ), wcslen( in_pszAudioSrcPath ) ) + wcslen( m_szLangSpecificDirName ) >= AK_MAX_PATH )
    {
        return AK_InvalidParameter;
    }
    wcscpy( m_szAudioSrcPath, in_pszAudioSrcPath );
    return AK_Success;
}
AKRESULT CAkDefaultLowLevelIO::SetLangSpecificDirName(
    AkLpCtstr   in_pszDirName
    )
{
    if ( wcslen( m_szBasePath ) + AkTemplMax( wcslen( m_szBankPath ), wcslen( m_szAudioSrcPath ) ) + wcslen( in_pszDirName ) >= AK_MAX_PATH )
    {
        return AK_InvalidParameter;
    }
    wcscpy( m_szLangSpecificDirName, in_pszDirName );
            return AK_Success;
}
