//////////////////////////////////////////////////////////////////////
//
// AkDefaultLowLevelIO.h
//
// Default Windows low level IO file system implementation.
// Calls platform API for IO, in blocking and overlapped
// fashion, according to the device that is currently set.
// File localization: Uses simple path concatenation logic.
// Exposes basic path functions for convenience.
//
// Copyright (c) 2006-2008 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#ifndef AK_DEFAULT_LOW_LEVEL_IO_H
#define AK_DEFAULT_LOW_LEVEL_IO_H

#include <AK/SoundEngine/Common/IAkStreamMgr.h>
#include <AK/SoundEngine/Platforms/XBox360/AkStreamMgrModule.h>
#include <AK/SoundEngine/common/AkMemoryMgr.h>

//-----------------------------------------------------------------------------
// Name: class BAkDefaultLowLevelIO.
// Desc: Low-level IO implementation that accesses Win32 IO API.
//		 Simple implementation of the file system.
//       Otherwise performs simple path concatenation logic.
//-----------------------------------------------------------------------------
class BAkDefaultLowLevelIO : public AK::IAkLowLevelIO
{
public:

    BAkDefaultLowLevelIO( );
    virtual ~BAkDefaultLowLevelIO( );

    AKRESULT Init();
    void Term();


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
    virtual AKRESULT Open( 
        AkLpCtstr       in_pszFileName,     // File name.
        AkOpenMode      in_eOpenMode,       // Open mode.
        AkFileSystemFlags * in_pFlags,      // Special flags. Can pass NULL.
        AkFileDesc &    out_fileDesc        // Returned file descriptor.
        );

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
    virtual AKRESULT Open( 
        AkFileID        in_fileID,          // File ID.
        AkOpenMode      in_eOpenMode,       // Open mode.
        AkFileSystemFlags * in_pFlags,      // Special flags. Can pass NULL.
        AkFileDesc &    out_fileDesc        // Returned file descriptor.
        );

    //-----------------------------------------------------------------------------
    // Name: Close()
    // Desc: Cleans up the given file.
    // Return: AK_Success or AK_Fail.
    //-----------------------------------------------------------------------------
    virtual AKRESULT Close(
        const AkFileDesc & in_fileDesc      // File descriptor.
        );

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
    virtual AKRESULT Read(
        AkFileDesc &    io_fileDesc,        // File descriptor.
        void *          out_pBuffer,        // Buffer to be filled with data.
        AkIOTransferInfo & io_transferInfo  // Platform-specific IO data transfer info. 
        );

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
    virtual AKRESULT Write(
        AkFileDesc &    io_fileDesc,        // File descriptor.
        void *          in_pData,           // Data to be written.
        AkIOTransferInfo & io_transferInfo  // Platform-specific IO operation info. 
        );

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
    virtual AKRESULT GetAsyncResult(
        AkFileDesc &    io_fileDesc,        // File descriptor.
        AkIOTransferInfo & io_transferInfo  // Platform-specific IO operation info. 
        );

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
    virtual AkUInt32 GetBlockSize(
        const AkFileDesc &  in_fileDesc     // File descriptor.
        );

    //-----------------------------------------------------------------------------
    // Name: GetDeviceDesc()
    // Return: Returns a device description for a given device ID.
    //         For profiling purposes only.
    //-----------------------------------------------------------------------------
#ifndef AK_OPTIMIZED
    virtual AKRESULT GetDeviceDesc(
        AkDeviceID      in_deviceID,        // High-level device ID.
        AkDeviceDesc &  out_deviceDesc      // Description of associated low-level I/O device.
        );
#endif


    // Current device.
    // This low-level IO implementation supports both blocking and deferred schedulers,
    // as an example, but one at a time. Set to a device that uses a blocking scheduler 
    // or a deferred, lined-up scheduler. The former will be considered as a hard drive,
    // whereas the latter will be considered as a DVD.
    // They return different block sizes and description.
    // Note: This function uses functions of the stream manager that are not thread safe.
    // No stream should exist when they are called.
    AKRESULT SetDevice(
        AkDeviceSettings & in_deviceSettings
        );
    inline AkDeviceID GetCurDeviceID( ) { return m_curDeviceID; }

    // Global path functions.
    // Base path is prepended to all file names.
    // Audio source path is appended to base path whenever uCompanyID is AK and uCodecID specifies an audio source.
    // Bank path is appended to base path whenever uCompanyID is AK and uCodecID specifies a sound bank.
    // Language specific dir name is appended to path whenever "bIsLanguageSpecific" is true.
    AKRESULT SetBasePath(
        AkLpCtstr   in_pszBasePath
        );
    AKRESULT SetBankPath(
        AkLpCtstr   in_pszBankPath
        );
    AKRESULT SetAudioSrcPath(
        AkLpCtstr   in_pszAudioSrcPath
        );
    AKRESULT SetLangSpecificDirName(
        AkLpCtstr   in_pszDirName
        );

protected:

    // Helpers.

    // String overload.
    AKRESULT GetFullFilePath(
        AkLpCtstr           in_pszFileName, // File name.
        AkFileSystemFlags * in_pFlags,      // Special flags. Can pass NULL.
        AkUInt32 &          out_uSector,    // Start sector.
        AkTChar *           out_pszFullFilePath // Win32 file path.
        );  

    // ID overload. 
    AKRESULT GetFullFilePath(
        AkFileID            in_fileID,      // File ID.
        AkFileSystemFlags * in_pFlags,      // Special flags. Can pass NULL.
        AkUInt32 &          out_uSector,    // Start sector.
        AkTChar *           out_pszFullFilePath // Win32 file path.
        );

    // Wrapper for Win32 CreateFile().
    AKRESULT OpenFile( 
        AkLpCtstr       in_pszFilename,     // File name.
        AkOpenMode      in_eOpenMode,       // Open mode.
        bool            in_bSynchronousIO,  // Synchronous IO flag.
        bool            in_bUnbufferedIO,   // Unbuffered IO flag.
        AkFileHandle &  out_hFile           // Returned file identifier/handle.
        );

    // Attributes.
    // Device.
    enum DeviceType
    {
        DeviceHDD,
        DeviceDVD
    };
    DeviceType          m_eCurDeviceType;
    AkDeviceID          m_curDeviceID;      // Obtained through StreamMgr::CreateDevice()
    AkUInt32            m_uBlockSize;       // Low-level block (sector) size.
    // Simple file system with path concatenation logic. 
    // Paths.
    AkTChar             m_szBasePath[AK_MAX_PATH];
    AkTChar             m_szBankPath[AK_MAX_PATH];
    AkTChar             m_szAudioSrcPath[AK_MAX_PATH];
    AkTChar             m_szLangSpecificDirName[AK_MAX_PATH];
};

#endif //AK_DEFAULT_LOW_LEVEL_IO_H