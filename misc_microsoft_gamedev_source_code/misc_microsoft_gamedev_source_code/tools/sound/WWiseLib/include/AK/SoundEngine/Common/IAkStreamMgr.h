//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2008 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

/// \file 
/// Defines the API of Audiokinetic's IO streaming solution.

#ifndef _IAK_STREAM_MGR_H_
#define _IAK_STREAM_MGR_H_

#include <AK/SoundEngine/Common/AkSoundEngineExport.h>
#include <AK/SoundEngine/Common/AkTypes.h>

//-----------------------------------------------------------------------------
// Defines. 
//-----------------------------------------------------------------------------

/// \name Profiling string lengths.
//@{
#define AK_MONITOR_STREAMNAME_MAXLENGTH    (64)
#define AK_MONITOR_DEVICENAME_MAXLENGTH    (16)
//@}
 
//-----------------------------------------------------------------------------
// Enums.
//-----------------------------------------------------------------------------

/// Stream status.
enum AkStmStatus
{
    AK_StmStatusIdle           	= 0,    	///< The stream is idle
    AK_StmStatusCompleted      	= 1,    	///< Operation completed / Automatic stream reached end
    AK_StmStatusPending        	= 2,    	///< Operation pending / The stream is waiting for IO
    AK_StmStatusCancelled      	= 3,		///< Operation cancelled
    AK_StmStatusError          	= 4     	///< The low-level IO reported an error
};

/// Move method for position change. 
/// \sa
/// - AK::IAkStdStream::SetPosition()
/// - AK::IAkAutoStream::SetPosition()
enum AkMoveMethod
{
    AK_MoveBegin               	= 0,    	///< Move offset from the start of the stream
    AK_MoveCurrent             	= 1,    	///< Move offset from the current stream position
    AK_MoveEnd                 	= 2     	///< Move offset from the end of the stream
};

/// File open mode.
enum AkOpenMode
{
    AK_OpenModeRead				= 0,		///< Read-only access
    AK_OpenModeWrite		    = 1,		///< Write-only access (opens the file if it already exists)
    AK_OpenModeWriteOvrwr	    = 2,		///< Write-only access (deletes the file if it already exists)
    AK_OpenModeReadWrite	    = 3	    	///< Read and write access
};


/// File descriptor. File identification for the low-level IO.
/// \sa
/// - AK::IAkLowLevelIO
struct AkFileDesc
{
    AkInt64				iFileSize;			///< File size in bytes
    AkUInt32			uSector;			///< Start sector (the sector size is specified by the low-level IO)
											///< \sa
											///< - AK::IAkLowLevelIO::GetBlockSize()
											///< - AK::IAkLowLevelIO::Read()
											///< - AK::IAkLowLevelIO::Write()
    AkUInt32			uCustomParamSize;	///< Size of the custom parameter
    void *				pCustomParam;		///< Custom parameter
    AkFileHandle		hFile;              ///< File handle/identifier
    AkDeviceID			deviceID;			///< Device ID, obtained from CreateDevice() \sa AK::IAkStreamMgr::CreateDevice()
};

/// File system flags for file descriptors mapping.
struct AkFileSystemFlags
{
    AkUInt32            uCompanyID;         ///< Company ID (Audiokinetic uses AKCOMPANYID_AUDIOKINETIC, defined in AkTypes.h)
    AkUInt32            uCodecID;           ///< File/codec type ID (defined in AkTypes.h)
    AkUInt32            uCustomParamSize;   ///< Size of the custom parameter
    void *              pCustomParam;       ///< Custom parameter
    bool                bIsLanguageSpecific;///< True when the file location depends on language
};

/// Stream information.
/// \sa
/// - AK::IAkStdStream::GetInfo()
/// - AK::IAkAutoStream::GetInfo()
struct AkStreamInfo
{
    AkDeviceID          deviceID;           ///< Device ID
    AkLpCtstr           pszName;            ///< User-defined stream name (specified through AK::IAkStdStream::SetStreamName() or AK::IAkAutoStream::SetStreamName())
    AkUInt64            uSize;              ///< Total stream/file size in bytes
};

/// Automatic streams heuristics.
struct AkAutoStmHeuristics
{
    AkReal32            fThroughput;        ///< Average throughput in bytes/ms
    AkUInt32            uLoopStart;         ///< Set to the start of loop (byte offset from the beginning of the stream) for streams that loop, 0 otherwise
    AkUInt32            uLoopEnd;           ///< Set to the end of loop (byte offset from the beginning of the stream) for streams that loop, 0 otherwise
    AkUInt32            uMinNumBuffers;     ///< Minimum number of buffers if you plan to own more than one buffer at a time, 0 or 1 otherwise
                                            ///< \remarks You should always release buffers as fast as possible, therefore this heuristic should be used only when 
                                            ///< dealing with special contraints, like drivers or hardware that require more than one buffer at a time.\n
                                            ///< Also, this is only a heuristic: it does not guarantee that data will be ready when calling AK::IAkAutoStream::GetBuffer().
    AkPriority          priority;           ///< The stream priority. it should be between AK_MIN_PRIORITY and AK_MAX_PRIORITY (included).
};

/// Automatic streams buffer settings/constraints.
struct AkAutoStmBufSettings
{
    AkUInt32			uBufferSize;		///< Hard user constraint: When non-zero, forces the IO buffer to be of size uBufferSize
											///< (overriding the device's granularity).
											///< Otherwise, the size is determined by the device's granularity.
    AkUInt32            uMinBufferSize;     ///< Soft user constraint: When non-zero, specifies a minimum buffer size
                                            ///< \remarks Ignored if uBufferSize is specified.
	AkUInt32            uBlockSize;  		///< Hard user constraint: When non-zero, buffer size will be a multiple of that number.
                                            ///< \remarks Ignored if uBufferSize is specified.    
};

/// \name Profiling structures.
//@{

/// Device descriptor.
struct AkDeviceDesc
{
    AkDeviceID          deviceID;           ///< Device ID
    bool                bCanWrite;          ///< Specifies whether or not the device is writable
    bool                bCanRead;           ///< Specifies whether or not the device is readable
    AkTChar             szDeviceName[AK_MONITOR_DEVICENAME_MAXLENGTH];      ///< Device name
    AkUInt32            uStringSize;        ///< Device name string's size (number of characters)
};

/// Stream general information.
struct AkStreamRecord
{
    AkUInt32            uStreamID;          ///< Unique stream identifier
    AkDeviceID          deviceID;           ///< Device ID
    AkTChar             szStreamName[AK_MONITOR_STREAMNAME_MAXLENGTH];       ///< Stream name
    AkUInt32            uStringSize;        ///< Stream name string's size (number of characters)
    AkUInt64            uFileSize;          ///< File size
    bool                bIsAutoStream;      ///< True for auto streams
};

/// Stream statistics.
struct AkStreamData
{
    AkUInt32            uStreamID;          ///< Unique stream identifier
    // Status (replace)
    AkUInt32            uPriority;          ///< Stream priority
    AkUInt64            uFilePosition;      ///< Current position
    AkUInt32            uBufferSize;        ///< Total stream buffer size (specific to IAkAutoStream)
    AkUInt32            uAvailableData;     ///< Size of available data (specific to IAkAutoStream)
    // Accumulate/Reset
    AkUInt32            uNumBytesTransfered;///< Transfered amount since the last query (Accumulate/Reset)
};
//@}

namespace AK
{
    /// \name Profiling interfaces.
    //@{
#ifndef AK_OPTIMIZED
    
    /// Profiling interface of streams.
	/// \warning The functions in this interface are not thread-safe, unless stated otherwise.
    class IAkStreamProfile
    {
    public:
        /// Returns the stream's record.
		/// \sa
		/// - \ref streamingdevicemanager
		/// - \ref streamingmanager_overriding
        virtual void    GetStreamRecord( 
            AkStreamRecord & out_streamRecord   ///< Returned stream record interface
            ) = 0;

        /// Returns the stream's statistics.
		/// \sa
		/// - \ref streamingdevicemanager
		/// - \ref streamingmanager_overriding
        virtual void    GetStreamData(
            AkStreamData &   out_streamData     ///< Returned periodic stream data interface
            ) = 0;

        /// Query the stream's "new" flag.
        /// \return True, until AK::IAkStreamProfile::ClearNew() is called.
		/// \sa
		/// - \ref streamingdevicemanager
		/// - \ref streamingmanager_overriding
        virtual bool    IsNew() = 0;

        /// Resets the stream's "new" flag.
		/// \sa
		/// - \ref streamingdevicemanager
		/// - \ref streamingmanager_overriding
        virtual void    ClearNew() = 0;
    };


    /// Profiling interface of high-level IO devices.
	/// \warning The functions in this interface are not thread-safe, unless stated otherwise.
    class IAkDeviceProfile
    {
    public:

        /// Query the device's description.
		/// \sa
		/// - \ref streamingdevicemanager
		/// - \ref streamingmanager_overriding
        virtual void    GetDesc(
            AkDeviceDesc &  out_deviceDesc      ///< Device descriptor.
            ) = 0;

        /// Query the device's "new" flag.
        /// \return True, until ClearNew() is called.
		/// \sa
		/// - \ref streamingdevicemanager
		/// - \ref streamingmanager_overriding
        virtual bool    IsNew() = 0;

        /// Resets the device's "new" flag.
		/// \sa
		/// - \ref streamingdevicemanager
		/// - \ref streamingmanager_overriding
        virtual void    ClearNew() = 0;

        /// Get the number of streams currently associated with that device.
        /// \return The number of streams
		/// \sa
		/// - \ref streamingdevicemanager
		/// - \ref streamingmanager_overriding
        virtual AkUInt32 GetNumStreams() = 0;

        /// Get a stream profile, for a specified stream index.
        /// \remarks GetStreamProfile() refers to streams by index, which must honor the call to AK::IAkDeviceProfile::GetNumStreams().
		/// \sa
		/// - \ref streamingdevicemanager
		/// - \ref streamingmanager_overriding
        virtual IAkStreamProfile * GetStreamProfile( 
			AkUInt32    in_uStreamIndex     ///< Stream index: [0,numStreams[
            ) = 0;
    };

    /// Profiling interface of the Stream Manager.
	/// \warning The functions in this interface are not thread-safe, unless stated otherwise.
    class IAkStreamMgrProfile
    {
    public:
        /// Start profile monitoring.
        /// \return AK_Success if successful, AK_Fail otherwise.
		/// \sa
		/// - \ref streamingdevicemanager
		/// - \ref streamingmanager_overriding
        virtual AKRESULT StartMonitoring() = 0;

        /// Stop profile monitoring.
		/// \sa
		/// - \ref streamingdevicemanager
		/// - \ref streamingmanager_overriding
	    virtual void     StopMonitoring() = 0;
        
        /// Device enumeration.
        /// \return The number of devices.
		/// \sa
		/// - \ref streamingdevicemanager
		/// - \ref streamingmanager_overriding
        virtual AkUInt32 GetNumDevices() = 0;

        /// Get a device profile for a specified device index.
        /// \remarks GetDeviceProfile() refers to devices by index, which must honor the call to AK::IAkStreamMgrProfile::GetNumDevices().
        /// \remarks The device index is not the same as the device ID (AkDeviceID).
		/// \sa
		/// - \ref streamingdevicemanager
		/// - \ref streamingmanager_overriding
        virtual IAkDeviceProfile * GetDeviceProfile( 
			AkUInt32    in_uDeviceIndex     ///< Device index: [0,numDevices[
            ) = 0;
    };
#endif
    //@}

    /// \name High-level streams API.
    //@{

    /// Interface of standard streams. Used as a handle to a standard stream. Has methods for 
    /// stream control. Obtained through the Stream Manager's AK::IAkStreamMgr::CreateStd() method.
	/// \warning The functions in this interface are not thread-safe, unless stated otherwise.
    class IAkStdStream
    {
    public:

        /// \name Stream management and settings.
        //@{
        /// Close the stream. The object is destroyed and the interface becomes invalid.
		/// \sa
		/// - \ref streamingdevicemanager
		/// - \ref streamingmanager_overriding
        virtual void      Destroy() = 0;

        /// Get information about a stream.
		/// \sa
		/// - \ref streamingdevicemanager
		/// - \ref streamingmanager_overriding
        virtual void      GetInfo(
            AkStreamInfo &      out_info        ///< Returned stream info
            ) = 0;

        /// Give the stream a name (appears in the Wwise Profiler).
		/// \sa
		/// - \ref streamingdevicemanager
		/// - \ref streamingmanager_overriding
        virtual AKRESULT  SetStreamName(
            AkLpCtstr       in_pszStreamName    ///< Stream name
            ) = 0;

        /// Get the I/O block size.
        /// \remark Queries the low-level IO, by calling AK::IAkLowLevelIO::GetBlockSize() with the
        /// stream's file descriptor.
        /// \return The block size, in bytes.
		/// \sa
		/// - \ref streamingdevicemanager
		/// - \ref streamingmanager_overriding
        virtual AkUInt32  GetBlockSize() = 0;   
        //@}

        /// \name IO operations.
        //@{
        
        /// Schedule a read request.
        /// \warning Use only with a multiple of the block size, queried via AK::IAkStdStream::GetBlockSize().
        /// \remarks If the call is asynchronous (in_bWait = false), wait until AK::IAkStdStream::GetStatus() stops returning AK_StmStatusPending.
        /// \return AK_Success if the operation was successfully scheduled (but not necessarily completed)
		/// \sa
		/// - \ref streamingdevicemanager
		/// - \ref streamingmanager_overriding
        virtual AKRESULT Read(
            void *          in_pBuffer,         ///< User buffer address 
            AkUInt32        in_uReqSize,        ///< Requested read size
            bool            in_bWait,           ///< Block until the operation is complete
            AkPriority      in_priority,        ///< Heuristic: operation priority
            AkReal32        in_fDeadline,       ///< Heuristic: operation deadline (ms)
            AkUInt32 &      out_uSize           ///< The size that was actually read
            ) = 0;

        /// Schedule a write request.
        /// \warning Use only with a multiple of the block size, queried via AK::IAkStdStream::GetBlockSize().
        /// \remarks If the call is asynchronous (in_bWait = false), wait until GetStatus() stops returning AK_StmStatusPending.
        /// \return AK_Success if the operation was successfully scheduled
		/// \sa
		/// - \ref streamingdevicemanager
		/// - \ref streamingmanager_overriding
        virtual AKRESULT Write(
            void *          in_pBuffer,         ///< User buffer address
            AkUInt32        in_uReqSize,        ///< Requested write size
            bool            in_bWait,           ///< Block until the operation is complete
            AkPriority      in_priority,        ///< Heuristic: operation priority
            AkReal32        in_fDeadline,       ///< Heuristic: operation deadline (ms)
            AkUInt32 &      out_uSize           ///< The size that was actually written
            ) = 0;

        /// Get the current stream position.
        /// \remarks If an operation is pending, there is no guarantee that the position was queried before (or after) the operation was completed.
        /// \return The current stream position
		/// \sa
		/// - \ref streamingdevicemanager
		/// - \ref streamingmanager_overriding
        virtual AkUInt64 GetPosition( 
            bool *          out_pbEndOfStream   ///< Returned end-of-stream flag, only for input streams (can pass NULL)
            ) = 0;

        /// Set the stream position. Modifies the position for the next read/write operation.
        /// \warning No operation should be pending.
        /// \remarks The new position will snap to the lowest block boundary.
        /// \return AK_Success if the stream position was changed
		/// \sa
		/// - \ref streamingdevicemanager
		/// - \ref streamingmanager_overriding
        virtual AKRESULT SetPosition(
            AkInt64         in_iMoveOffset,     ///< Seek offset
            AkMoveMethod    in_eMoveMethod,     ///< Seek method, from the beginning, end, or current file position
            AkInt64 *       out_piRealOffset    ///< The actual seek offset may differ from the expected value when the block size is bigger than 1.
                                                ///< In that case, the seek offset floors to the sector boundary. Can pass NULL.
            ) = 0;

        /// Cancel the current operation.
        /// \remarks When it returns, the caller is guaranteed that no operation is pending.
        /// \remarks This method can block the caller for the whole duration of the IO operation, if the request was already posted.
		/// \sa
		/// - \ref streamingdevicemanager
		/// - \ref streamingmanager_overriding
        virtual void Cancel() = 0;

        //@}

        /// \name Access to data and status.
        //@{
        /// Get user data (and accessed size).
        /// \return The address of data provided by user
		/// \sa
		/// - \ref streamingdevicemanager
		/// - \ref streamingmanager_overriding
        virtual void *   GetData( 
            AkUInt32 &      out_uSize           ///< Size actually read or written
            ) = 0;

        /// Get the stream's status.
        /// \return The stream status.
		/// \sa
		/// - \ref streamingdevicemanager
		/// - \ref streamingmanager_overriding
        virtual AkStmStatus GetStatus() = 0;  

        //@}
    };


    /// Interface of automatic streams. It is used as a handle to a stream, 
    /// I/O operations are triggered from here. 
    /// Obtained through the Stream Manager's AK::IAkStreamMgr::CreateAuto() method.
	/// \warning The functions in this interface are not thread-safe, unless stated otherwise.
	/// \sa
	/// - \ref streamingdevicemanager
	/// - \ref streamingmanager_overriding
    class IAkAutoStream 
    {
    public:

        /// \name Stream management, settings access, and run-time change.
        //@{
        /// Close the stream. The object is destroyed and the interface becomes invalid.
		/// \sa
		/// - \ref streamingdevicemanager
		/// - \ref streamingmanager_overriding
        virtual void      Destroy() = 0;

        /// Get information about the stream.
		/// \sa
		/// - \ref streamingdevicemanager
		/// - \ref streamingmanager_overriding
        virtual void      GetInfo(
            AkStreamInfo &      out_info        ///< Returned stream info
            ) = 0;

        /// Get the stream's heuristics.
		/// \sa
		/// - \ref streamingdevicemanager
		/// - \ref streamingmanager_overriding
        virtual void      GetHeuristics(
            AkAutoStmHeuristics & out_heuristics///< Returned stream heuristics
            ) = 0;

        /// Run-time change of the stream's heuristics.
		/// \sa
		/// - \ref streamingdevicemanager
		/// - \ref streamingmanager_overriding
        virtual AKRESULT  SetHeuristics(
            const AkAutoStmHeuristics & in_heuristics   ///< New stream heuristics
            ) = 0;

        /// Give the stream a name (appears in the Wwise profiler).
		/// \sa
		/// - \ref streamingdevicemanager
		/// - \ref streamingmanager_overriding
        virtual AKRESULT  SetStreamName(
            AkLpCtstr       in_pszStreamName    ///< Stream name
            ) = 0;

        /// Get the I/O block size.
        /// \remark Queries the actual low-level I/O device, by calling AK::IAkLowLevelIO::GetBlockSize() with the
        /// stream's file descriptor.
        /// \return The block size (in bytes)
		/// \sa
		/// - \ref streamingdevicemanager
		/// - \ref streamingmanager_overriding
        virtual AkUInt32  GetBlockSize() = 0;

        //@}

        /// \name Stream operations.
        //@{
        
        /// Start the automatic scheduling.
        /// \return AK_Success if the automatic scheduling was started successfully
		/// \sa
		/// - \ref streamingdevicemanager
		/// - \ref streamingmanager_overriding
        virtual AKRESULT Start() = 0;

        /// Stop the automatic scheduling.
        /// \return AK_Success if the automatic scheduling was stopped successfully.
		/// \sa
		/// - \ref streamingdevicemanager
		/// - \ref streamingmanager_overriding
        virtual AKRESULT Stop() = 0;

        /// Get the stream's position.
        /// \remarks The stream position is the position seen by the user, not the position of the file
        /// already streamed into the Stream Manager's memory. The stream position is updated when buffers 
        /// are released, using AK::IAkAutoStream::ReleaseBuffer().
        /// \return The position in bytes of the first buffer owned by the user
		/// \sa
		/// - \ref streamingdevicemanager
		/// - \ref streamingmanager_overriding
        virtual AkUInt64 GetPosition( 
            bool *          out_pbEndOfStream   ///< Returned end-of-stream flag (can pass NULL)
            ) = 0;  

        /// Set the stream's position. 
        /// The next call to AK::IAkAutoStream::GetBuffer() will grant data that corresponds to the position specified here.
        /// \remarks Data already streamed into the Stream Manager's memory might be flushed.
        /// \remarks The new position will round down to the low-level IO block size.
        /// \return AK_Success if the resulting position is valid
		/// \sa
		/// - \ref streamingdevicemanager
		/// - \ref streamingmanager_overriding
        virtual AKRESULT SetPosition(
            AkInt64         in_iMoveOffset,     ///< Seek offset
            AkMoveMethod    in_eMoveMethod,     ///< Seek method, from the beginning, end or current file position
            AkInt64 *       out_piRealOffset    ///< The actual seek offset may differ from the expected value when the low-level's block size is greater than 1.
                                                ///< In that case, the real absolute position rounds down to the block boundary. Can pass NULL.
            ) = 0;

        //@}


        /// \name Data/status access.
        //@{

        /// Get data from the Stream Manager buffers.
        /// \remarks Grants a buffer if data is available. Each successful call to this method returns a new 
        /// buffer of data, at the current stream position.
        /// Buffers should be released as soon as they are not needed, using AK::IAkAutoStream::ReleaseBuffer().
        /// \aknote AK::IAkAutoStream::ReleaseBuffer() does not take any argument, because it releases buffers in order. \endaknote
        /// \return
        ///     - AK_DataReady     : the buffer was granted
		///     - AK_NoDataReady   : the buffer is not granted yet (never happens when called with in_bWait flag)
        ///     - AK_NoMoreData    : the buffer was granted but reached end of file (next call will return with size 0)
        ///     - AK_Fail          : there was an IO error
		/// \sa
		/// - \ref streamingdevicemanager
		/// - \ref streamingmanager_overriding
        virtual AKRESULT GetBuffer(
            void *&         out_pBuffer,        ///< Returned address of granted data space
            AkUInt32 &      out_uSize,          ///< Returned size of granted data space
            bool            in_bWait            ///< Block until data is ready
            ) = 0;

        /// Release buffer granted through GetBuffer(). Buffers are released in order.
        /// \return AK_Success if a valid buffer was released, AK_Fail if the user did not own any buffer
		/// \sa
		/// - \ref streamingdevicemanager
		/// - \ref streamingmanager_overriding
        virtual AKRESULT ReleaseBuffer() = 0;
        //@}
    };

    //@}


    /// Interface of the Stream Manager.
	/// \warning The functions in this interface are not thread-safe, unless stated otherwise.
    class IAkStreamMgr
    {
    public:
        /// Global access to singleton.
        /// \return The interface of the global Stream Manager
		/// \sa
		/// - \ref streamingdevicemanager
		/// - \ref streamingmanager_overriding
        inline static IAkStreamMgr * Get()
        {
            return m_pStreamMgr;
        }

        /// Destroy the Stream Manager.
		/// \sa
		/// - \ref streamingdevicemanager
		/// - \ref streamingmanager_overriding
        virtual void     Destroy() = 0;


        /// \name Profiling.
        //@{
#ifndef AK_OPTIMIZED
        /// Get the profiling interface.
        /// \return The interface of the Stream Manager profiler
        virtual IAkStreamMgrProfile * GetStreamMgrProfile() = 0;
#endif
        //@}


        /// \name Stream creation interface.
        //@{
        
        // Standard stream creation methods.

        /// Create a standard stream (string overload).
        /// \return AK_Success if the stream was created successfully
        /// \remarks The string overlad of AK::IAkLowLevelIO::Open() will be called.
		/// \sa
		/// - \ref streamingdevicemanager
		/// - \ref streamingmanager_overriding
        virtual AKRESULT CreateStd(
            AkLpCtstr           in_pszFileName,     ///< Application-defined string (title only, or full path, or code...)
            AkFileSystemFlags * in_pFSFlags,        ///< Special file system flags. Can pass NULL
            AkOpenMode          in_eOpenMode,       ///< Open mode (read, write, ...)
            IAkStdStream *&     out_pStream         ///< Returned interface to a standard stream
            ) = 0;

        /// Create a standard stream (ID overload).
        /// \return AK_Success if the stream was created successfully
        /// \remarks The ID overlad of AK::IAkLowLevelIO::Open() will be called.
		/// \sa
		/// - \ref streamingdevicemanager
		/// - \ref streamingmanager_overriding
        virtual AKRESULT CreateStd(
            AkFileID            in_fileID,          ///< Application-defined ID
            AkFileSystemFlags * in_pFSFlags,        ///< Special file system flags (can pass NULL)
            AkOpenMode          in_eOpenMode,       ///< Open mode (read, write, ...)
            IAkStdStream *&     out_pStream         ///< Returned interface to a standard stream
            ) = 0;

        
        // Automatic stream create methods.

        /// Create an automatic stream (string overload).
        /// \return AK_Success if the stream was created successfully
        /// \remarks The stream needs to be started explicitly with AK::IAkAutoStream::Start().
        /// \remarks The string overlad of AK::IAkLowLevelIO::Open() will be called.
		/// \sa
		/// - \ref streamingdevicemanager
		/// - \ref streamingmanager_overriding
        virtual AKRESULT CreateAuto(
            AkLpCtstr                   in_pszFileName,     ///< Application-defined string (title only, or full path, or code...)
            AkFileSystemFlags *         in_pFSFlags,        ///< Special file system flags (can pass NULL)
            const AkAutoStmHeuristics & in_heuristics,      ///< Streaming heuristics
            AkAutoStmBufSettings *      in_pBufferSettings, ///< Stream buffer settings (it is recommended to pass NULL in order to use the default settings)
            IAkAutoStream *&            out_pStream         ///< Returned interface to an automatic stream
            ) = 0;

        /// Create an automatic stream (ID overload).
        /// \return AK_Success if the stream was created successfully
        /// \remarks The stream needs to be started explicitly with IAkAutoStream::Start().
        /// \remarks The ID overlad of AK::IAkLowLevelIO::Open() will be called.
		/// \sa
		/// - \ref streamingdevicemanager
		/// - \ref streamingmanager_overriding
        virtual AKRESULT CreateAuto(
            AkFileID                    in_fileID,          ///< Application-defined ID
            AkFileSystemFlags *         in_pFSFlags,        ///< Special file system flags (can pass NULL)
            const AkAutoStmHeuristics & in_heuristics,      ///< Streaming heuristics
            AkAutoStmBufSettings *      in_pBufferSettings, ///< Stream buffer settings (it is recommended to pass NULL to use the default settings)
            IAkAutoStream *&            out_pStream         ///< Returned interface to an automatic stream
            ) = 0;

        //@}

    protected:
        /// Definition of the global pointer to the interface of the Stream Manager singleton.
		/// \sa
		/// - \ref streamingdevicemanager
		/// - \ref streamingmanager_overriding
        static AKSTREAMMGR_API IAkStreamMgr * m_pStreamMgr;
    };

}
#endif //_IAK_STREAM_MGR_H_
