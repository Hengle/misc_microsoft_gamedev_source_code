//////////////////////////////////////////////////////////////////////
//
// AkDeviceBase.h
//
// Device implementation that is common across all high-level IO devices.
// Specific to Win32 API.
//
// Copyright (c) 2006-2008 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////
#ifndef _AK_DEVICE_BASE_H_
#define _AK_DEVICE_BASE_H_

#include <AK/Tools/Common/AkObject.h>
#include "IAkDevice.h"
#include <AK/Tools/Common/AkLock.h>
#include <AK/Tools/Common/AkArray.h>
#include "AkStreamMgr.h"
#include <AkOverlapped.h>
#include <AK/Tools/Common/AkPlatformFuncs.h>
#include <AK/SoundEngine/Common/AkStreamMgrModule.h>

#if !defined(WIN32) && !defined(XBOX360)
#error Platform not supported
#endif

#ifdef AK_INSTRUMENT_STM_MGR
	#include <stdio.h>
#endif

// ------------------------------------------------------------------------------
// Defines.
// ------------------------------------------------------------------------------

/// Stream type.
enum AkStmType
{
    AK_StmTypeStandard         	= 0,    	///< Standard stream for manual IO (AK::IAkStdStream).
    AK_StmTypeAutomatic        	= 1     	///< Automatic stream (AK::IAkAutoStream): IO requests are scheduled automatically into internal Stream Manager memory.
};

namespace AK
{
    class CAkStmTask;

    AK_DEFINE_ARRAY_POOL( ArrayPoolLocal, CAkStreamMgr::GetObjPoolID( ) );

    

    //-----------------------------------------------------------------------------
    // Name: CAkDeviceBase
    // Desc: Base implementation of the high-level I/O device interface.
    //       Implements the I/O thread, provides Stream Tasks (CAkStmTask) 
    //       scheduling services for data transfer. 
    // Note: Device description and access to platform API is handled by low-level.
    //       The I/O thread calls pure virtual method PerformIO(), that has to be
    //       implemented by derived classes according to how they communicate with
    //       the Low-Level IO.
    //       Implementation of the device logic is distributed across the device
    //       and its streaming objects.
    //-----------------------------------------------------------------------------
    class CAkDeviceBase : public CAkObject,
						  public IAkDevice
                          
    {
    public:

        CAkDeviceBase( );
        virtual ~CAkDeviceBase( );
        
        virtual AKRESULT Init( 
            const AkDeviceSettings & in_settings,
            AkDeviceID               in_deviceID 
            );
        virtual void     Destroy();

        virtual AkDeviceID  GetDeviceID();

        // Stream creation interface.
        // --------------------------------------------------------

        // Standard stream.
        virtual AKRESULT CreateStd(
            AkFileDesc &		in_fileDesc,        // Application defined ID.
            AkOpenMode          in_eOpenMode,       // Open mode (read, write, ...).
            IAkStdStream *&     out_pStream         // Returned interface to a standard stream.
            );

        
        // Automatic stream
        virtual AKRESULT CreateAuto(
            AkFileDesc &				in_fileDesc,        // Application defined ID.
            const AkAutoStmHeuristics & in_heuristics,      // Streaming heuristics.
            AkAutoStmBufSettings *      in_pBufferSettings, // Stream buffer settings. Pass NULL to use defaults (recommended).
            IAkAutoStream *&            out_pStream         // Returned interface to an automatic stream.
            );
        
        // Methods used by stream objects.
        // --------------------------------------------------------

        // Lock scheduler from outside. Locks the tasks list lock.
        inline void LockScheduler()
        {
            m_lockTasksList.Lock( );
        }
        inline void UnlockScheduler()
        {
            m_lockTasksList.Unlock( );
        }

        // Scheduler thread control.
        /* Semaphore usage:
        StdSem: Signaled whenever at least one standard stream is waiting for IO (pending operation).
        AutoSem: Signaled whenever at least one automatic stream is waiting for IO (running), and memory
        usage allows it.
        Memory notifications: When set to "idle", AutoSem event is inhibated: the I/O thread will not 
        wake up to schedule automatic streams. This occurs when there is no more memory, and the scheduler
        decides not to reassign buffers.
        It is signaled whenever a streaming buffer is freed (ReleaseBuffer(), Destroy(), SetPosition()->Flush()),
        or if a new stream is created (requires memory usage reevaluation), through NotifyMemChange().
        */
        void StdSemIncr();      // Increment standard streams sempahore count (signal).
        void StdSemDecr();      // Decrement standard streams sempahore count (reset if count=0).
        void AutoSemIncr();     // Increment automatic streams sempahore count (signal).
        void AutoSemDecr();     // Decrement automatic streams sempahore count (reset if count=0).
        // Warning. Mem notifications are not protected. They must be enclosed in m_lockAutoSems along with
        // allocs and frees. Users call CAkDeviceBase::MemLock();
        void NotifyMemChange(); // Notify that a streaming buffer was freed.
        void NotifyMemIdle();   // Notify that all streaming buffers are used and should not be reassigned
                                // (inhibates AutoSem).
        // Locks and unlocks memory allocation and notification.
        // Note: It is legal to call these from streams' status locked sections, but the opposite is illegal:
        // Do not try to obtain a stream's status lock with Mem locked.
        void LockMem();
        void UnlockMem();

        // Globals.
        inline AkUInt32 GetIOThreadWaitTime()
        {
            return m_uWaitTime;
        }
        inline AkUInt32 GetGranularity()
        {
            return m_uGranularity;
        }
        inline AkReal32 GetTargetAutoStmBufferLength()
        {
            return m_fTargetAutoStmBufferLength;
        }
        inline AkMemPoolId GetIOPoolID()
        {
            return m_streamIOPoolId;
        }
        inline AkInt64 GetTime()
        {
            return m_time;
        }

#ifdef AK_INSTRUMENT_STM_MGR
		inline FILE * DumpFile() { return m_pDump; }
#endif
        
        // Device Profile Ex interface.
        // --------------------------------------------------------
#ifndef AK_OPTIMIZED
        inline AkMemPoolId GetIOPoolSize()
        {
            return m_streamIOPoolSize;  // IO memory size.
        }

	    // Monitoring status.
        virtual AKRESULT     StartMonitoring();
	    virtual void         StopMonitoring();
        inline bool          IsMonitoring() { return m_bIsMonitoring; }

        // Device profiling.
        virtual void     GetDesc( 
            AkDeviceDesc & out_deviceDesc 
            );
        virtual bool     IsNew();
        virtual void     ClearNew();
        
        // Stream profiling.
        virtual AkUInt32 GetNumStreams();
        // Note. The following functions refer to streams by index, which must honor the call to GetNumStreams().
        virtual AK::IAkStreamProfile * GetStreamProfile( 
            AkUInt32    in_uStreamIndex             // [0,numStreams[
            );
#endif

    protected:

        // Add a new task to the list.
        AKRESULT AddTask( 
            CAkStmTask * in_pStmTask
            );

        // Init/term scheduler objects.
        AKRESULT CreateScheduler( 
            AkThreadProperties * in_pThreadProperties 
            );
        void     DestroyScheduler();

        // Destroys all streams.
        void ClearStreams();

        // I/O Scheduler.
        static DWORD WINAPI IOSchedThread( 
            LPVOID lpParameter 
            );
        // I/O Algorithm. Depends on the type of device. 
        // Implement in derived class.
        virtual void PerformIO() = 0;

        // Scheduler algorithm.
        // Finds the next task for which an I/O request should be issued.
        // Return: If a task is found, a valid pointer to a task is returned, as well
        // as the address in/from which to perform a data transfer.
        // Otherwise, returns NULL.
        CAkStmTask *    SchedulerFindNextTask(
            void *& out_pBuffer     // Returned I/O buffer address.
            );
        // Finds next task among standard streams only (typically when there is no more memory for automatic streams).
        CAkStmTask *    ScheduleStdStmOnly(
            void *& out_pBuffer     // Returned I/O address.
            );

        // IO thread semaphore.
        HANDLE          m_hStdSem;              // Event; signaled when at least one standard stream task is ready.
        HANDLE          m_hAutoSem;             // Event; signaled when at least one automatic stream task is ready.
        AkInt32         m_cPendingStdStms;      // Number of standard stream tasks waiting for I/O. When it reaches 0, m_hStdSem is reset.
        AkInt32         m_cRunningAutoStms;     // Number of automatic stream tasks waiting for I/O. When it reaches 0, m_hAutoSem is reset.
        bool            m_bDoWaitMemoryChange;  // When true, automatic streams semaphore is inhibated, because there is no free buffers, and they cannot be reassigned. 
#ifdef _DEBUG
        bool            m_bIsAutoSemSignaled;   // True when semaphore is signaled (result of m_cRunningAutoStms>0 and "don't sleep on memory"). For debug purposes.
#endif
        
        AkUInt32        m_uWaitTime;            // Maximum sleep time of I/O thread. 
                                                // It is usually the user specified dwIdleWaitTime heuristics (in ms) but becomes INFINITE if no task is waiting for I/O.

        // IO thread handles.
        HANDLE          m_hIOThreadStopEvent;
        HANDLE          m_hIOThread;

        // Locks for semaphores (event+count atomic).
        // Memory notifications must be atomic with allocs and frees, and with the AutoSem, because it uses
        // the same event.
        // Note. Alloc/Free/Mem notifications mechanism works well because alloc sizes are always the same: freeing
        // ensures that next allocation will succeed.
        CAkLock         m_lockAutoSems;

        // Task list.
        // Tasks live in m_arTasks from the time they are created until they are completely destroyed (by the I/O thread).
        // It is more efficient to query the tasks every time scheduling occurs than to add/remove them from the list, 
        // every time, from other threads.
        typedef AkArray<CAkStmTask*, CAkStmTask*, ArrayPoolLocal, AK_STM_OBJ_POOL_BLOCK_SIZE/sizeof(CAkStmTask*)> TaskArray;
        TaskArray       m_arTasks;              // Array of tasks.
        CAkLock         m_lockTasksList;        // Protects tasks array.

        // Settings.
        AkUInt32        m_uGranularity;
        AkReal32        m_fTargetAutoStmBufferLength;
        DWORD           m_dwIdleWaitTime;
        AkDeviceID      m_deviceID;

        // Memory.
        AkMemPoolId	    m_streamIOPoolId;		// IO memory.

        // Time in milliseconds. Stamped at every scheduler pass.
        AkInt64         m_time;

        // Profiling specifics.
#ifndef AK_OPTIMIZED
		AkUInt32        m_streamIOPoolSize;     // IO memory size.

        bool            m_bIsMonitoring;
        AkUInt32        m_uNextStreamID;
        bool            m_bIsNew;
        typedef AkArray<AK::IAkStreamProfile*,AK::IAkStreamProfile*,ArrayPoolLocal,AK_STM_OBJ_POOL_BLOCK_SIZE/sizeof(CAkStmTask*)> ArrayStreamProfiles;
        ArrayStreamProfiles m_arStreamProfiles; // Tasks pointers are copied there when GetNumStreams() is called, to avoid 
                                                // locking-unlocking the real tasks list to query each stream's profiling data.
#endif

#ifdef AK_INSTRUMENT_STM_MGR
		FILE *			m_pDump;				// Instrumentation file.
#endif
    };




    //-----------------------------------------------------------------------------
    //
    // Stream objects implementation.
    //
    //-----------------------------------------------------------------------------

    //-----------------------------------------------------------------------------
    // Name: class CAkStmTask
    // Desc: Base implementation common to streams. Defines the interface used by
    //       the device scheduler.
    //-----------------------------------------------------------------------------
    class CAkStmTask
#ifndef AK_OPTIMIZED
        : public IAkStreamProfile
#endif
    {
    public:

        CAkStmTask();
        virtual ~CAkStmTask();

        // Task management.
        // Returns true when the object is ready to be destroyed. Common. 
        inline bool IsToBeDestroyed()             
        {
            // Note. When profiling, we need to have the profiler's agreement to destroy the stream.
        #ifndef AK_OPTIMIZED
            return m_bIsToBeDestroyed && 
                ( !m_pDevice->IsMonitoring( ) || m_bIsProfileDestructionAllowed );
        #else
            return m_bIsToBeDestroyed;
        #endif
        }

        // Destroys the object. Must be called only if IsToBeDestroyed() returned True.
        virtual void InstantDestroy() = 0;          

        // Settings access.
        inline AkStmType StmType()      // Task stream type.
        {
            return m_eStmType;
        }
        inline bool IsWriteOp()         // Task operation type (read or write).
        {
            return m_bIsWriteOp;
        }
        inline AkPriority Priority()    // Priority.
        {
            AKASSERT( m_priority >= AK_MIN_PRIORITY &&
                    m_priority <= AK_MAX_PRIORITY );
            return m_priority;
        }

        // Get information for data transfer. Common.
        virtual void TransferInfo( 
            AkFileDesc *& out_pFileDesc,    // Stream's associated file descriptor.
            OVERLAPPED *& out_pOverlapped,  // Asynchronous data and file position.
            bool &      out_bIsSequential,  // Returned !bIsPositionDirty flag.
            AkUInt32 *  out_puSize          // Required transfer size.
            ) = 0;        
        // Update stream object after I/O.
        virtual void Update(
            void *      in_pBuffer,         // Address of data.
            AkUInt32    in_uIOSize,         // Size actually read/written.
            bool        in_bWasIOSuccessful // IO was successful (no error).
            ) = 0;

        // Data buffer access.
        // Tries to get a buffer for I/O. It might fail if there is no more memory (automatic streams)
        // or if the operation was cancelled (standard streams). In such a case it returns NULL.
        // Returns the address of the user's buffer.
        virtual void * TryGetIOBuffer() = 0;
        // For buffer reassignment. The scheduler calls this if there is no memory available and it considers
        // that this task could give away some of its buffers.
        // Automatic stream specific. Not implemented in standard stream objects.
        // Returns NULL if failed, and notifies that memory management should become idle.
        // Fails if the user got all buffers before the scheduler.
        virtual void * PopIOBuffer() = 0;

        // Scheduling heuristics.
        virtual AkReal32 EffectiveDeadline() = 0;   // Compute task's effective deadline for next operation, in ms.
        AkReal32 TimeSinceLastTransfer(             // Time elapsed since last I/O transfer.
            const AkInt64 & in_liNow                // Time stamp.
            )
        {
            return AKPLATFORM::Elapsed( in_liNow, m_iIOStartTime );
        }

        // Returns True if the task is waiting for I/O.
        // Standard streams wait for I/O when they are Pending.
        // Automatic streams wait for I/O when they are Running and !EOF.
        virtual bool IsWaitingForIO() = 0;

        // Profiling.
#ifndef AK_OPTIMIZED
        
        // IAkStreamProfile interface.
        // ---------------------------
        virtual bool IsNew()                        // Returns true if stream is tagged "New".
        {
            return m_bIsNew;
        }
        virtual void ClearNew()                     // Clears stream's "New" tag.
        {
            m_bIsNew = false;
        }
        virtual void GetStreamRecord( 
            AkStreamRecord & out_streamRecord
            );
        // ---------------------------

        inline bool IsProfileNew()                  // Returns true if stream is tagged "New".
        {
            return m_bIsNew;
        }
        inline AK::IAkStreamProfile * GetStreamProfile()    // Returns associated stream profile interface.
        {
            return this;
        }
        inline void SetStreamID(                    // Assigns a stream ID used by profiling.
            AkUInt32 in_uStreamID 
            )
        {
            m_uStreamID = in_uStreamID;
        }
        inline bool ProfileIsToBeDestroyed()        // True when the stream has been scheduled for destruction.
        {
            return m_bIsToBeDestroyed;
        }
        inline void ProfileAllowDestruction()       // Signals that stream can be destroyed.
        {
            AKASSERT( m_bIsToBeDestroyed );
            m_bIsProfileDestructionAllowed = true;
        }

#endif

        // Common attributes.
        // ------------------
    protected:
        
        // File info.
        AkFileDesc          m_fileDesc;         // File descriptor:
                                                // uFileSize: File size.
                                                // uSector: Position of beginning of file (relative to handle).
                                                // hFile: System handle.
                                                // Custom parameter and size (owned by Low-level).
                                                // Device ID.
        CAkOverlapped       m_overlapped;       // Overlapped structure for low-level IO.
        CAkLock             m_lockStatus;       // Lock for status integrity.
        AkInt64             m_iCurPosition;     // Stream position.
        AkInt64		        m_iIOStartTime;     // Time when I/O started. 
        CAkDeviceBase *     m_pDevice;          // Access to owner device.
        AkTChar *           m_pszStreamName;    // User defined stream name.   
        AkUInt32            m_uBufferSize;      // Remaining size to fill. 
        AkUInt32            m_uLLBlockSize;     // Low-level IO block size (queried once at init).
        HANDLE              m_hBlockEvent;      // Event used for blocking I/O.

        // Profiling.
#ifndef AK_OPTIMIZED
        AkUInt32            m_uStreamID;        // Profiling stream ID.
        AkUInt32            m_uBytesTransfered; // Number of bytes transferred (replace).
#endif

        AkPriority          m_priority;         // IO priority. Keeps last operation's priority.

        AkStmType           m_eStmType      :2; // Stream type. 2 types, avoid sign bit.
        AkUInt32            m_bIsWriteOp    :1; // Operation type (automatic streams are always reading).
        AkUInt32            m_bIsPositionDirty  :1; // Dirty flag for position (between saved ulCurPosition and file pointer).
        AkUInt32            m_bHasReachedEof    :1; // True when file pointer reached eof.
        AkUInt32            m_bIsToBeDestroyed  :1; // True when this stream is scheduled to be destroyed.

        // Profiling.
#ifndef AK_OPTIMIZED
        AkUInt32            m_bIsNew        :1; // "New" flag.
        AkUInt32            m_bIsProfileDestructionAllowed  :1; // True when profiler gave its approbation for destruction.
#endif
                                
    };


    //-----------------------------------------------------------------------------
    // Name: class CAkStmBase
    // Desc: Base implementation for standard streams.
    //-----------------------------------------------------------------------------
    class CAkStdStmBase : public CAkObject,
						  public AK::IAkStdStream,
                          public CAkStmTask
    {
    public:

        // Construction/destruction.
        CAkStdStmBase();
        virtual ~CAkStdStmBase();

        virtual AKRESULT Init(
            CAkDeviceBase *     in_pDevice,         // Owner device.
            const AkFileDesc &  in_fileDesc,        // File descriptor.
            AkOpenMode          in_eOpenMode        // Open mode.
            );
        void     Term();

        //-----------------------------------------------------------------------------
        // AK::IAkStdStream interface.
        //-----------------------------------------------------------------------------

        // Closes stream. The object is destroyed and the interface becomes invalid.
        virtual void      Destroy();

        // Stream info access.
        virtual void      GetInfo(
            AkStreamInfo &      out_info        // Returned stream info.
            );
        // Name the stream (appears in Wwise profiler).
        virtual AKRESULT  SetStreamName(
            AkLpCtstr       in_pszStreamName    // Stream name.
            );
        // Returns I/O block size.
        virtual AkUInt32  GetBlockSize();       // Returns block size for optimal/unbuffered IO.
        
        // Operations.
        // ---------------------------------------
        
        // Read/Write.
        // Ask for a multiple of the device's atomic block size, 
        // obtained through IAkStdStream::GetBlockSize().
        virtual AKRESULT Read(
            void *          in_pBuffer,         // User buffer address. 
            AkUInt32        in_uReqSize,        // Requested read size.
            bool            in_bWait,           // Block until operation is complete.
            AkPriority      in_priority,        // Heuristic: operation priority.
            AkReal32        in_fDeadline,       // Heuristic: operation deadline (s).
            AkUInt32 &      out_uSize           // Size actually read.
            );
        virtual AKRESULT Write(
            void *          in_pBuffer,         // User buffer address. 
            AkUInt32        in_uReqSize,        // Requested write size. 
            bool            in_bWait,           // Block until operation is complete.
            AkPriority      in_priority,        // Heuristic: operation priority.
            AkReal32        in_fDeadline,       // Heuristic: operation deadline (s).
            AkUInt32 &      out_uSize           // Size actually written.
            );
        
        // Get current stream position.
        virtual AkUInt64 GetPosition( 
            bool *          out_pbEndOfStream   // Input streams only. Can pass NULL.
            );
        // Set stream position. Modifies position of next read/write.
        virtual AKRESULT SetPosition(
            AkInt64         in_iMoveOffset,     // Seek offset.
            AkMoveMethod    in_eMoveMethod,     // Seek method, from beginning, end or current file position.
            AkInt64 *       out_piRealOffset    // Actual seek offset may differ from expected value when unbuffered IO.
                                                // In that case, floors to sector boundary. Pass NULL if don't care.
            );
        // Cancel.
        virtual void Cancel();
        
        /// Query user data and size.
        virtual void * GetData( 
            AkUInt32 &      out_uSize           // Size actually read or written.
            );
        // Status.
        virtual AkStmStatus GetStatus();        // Get operation status.

        //-----------------------------------------------------------------------------
        // CAkStmTask interface.
        //-----------------------------------------------------------------------------

        // Task management.
        virtual void InstantDestroy();          // Destroys the object.

        // Get information for data transfer. 
        virtual void TransferInfo( 
            AkFileDesc *& out_pFileDesc,    // Stream's associated file descriptor.
            OVERLAPPED *& out_pOverlapped,  // Asynchronous data and file position.
            bool &      out_bIsSequential,  // Returned !bIsPositionDirty flag.
            AkUInt32 *  out_puSize          // Required transfer size.
            );

        // Update task after data transfer.
        virtual void Update(
            void *      in_pBuffer,             // Address of data.
            AkUInt32    in_uIOSize,             // Size actually read/written.
            bool        in_bWasIOSuccessful     // IO was successful (no error).
            );

        // Data buffer access.
        // Tries to get a buffer for I/O. It might fail if the current operation was cancelled 
        // while scheduling (returns NULL).
        // Returns the address of the user's buffer.
        virtual void * TryGetIOBuffer();
        // Automatic streams specific: does not apply.
        virtual void * PopIOBuffer();

        // Scheduling heuristics.
        virtual AkReal32 EffectiveDeadline();   // Compute task's effective deadline for next operation, in ms.

        // Returns True if the task is Pending.
        virtual bool IsWaitingForIO();

        //-----------------------------------------------------------------------------
        // Profiling.
        //-----------------------------------------------------------------------------
#ifndef AK_OPTIMIZED
        
        // IAkStreamProfile interface.
        virtual void GetStreamData(
            AkStreamData &   out_streamData
            );
#endif

        //-----------------------------------------------------------------------------
        // Helpers.
        //-----------------------------------------------------------------------------

        // Secured task status set. Increment and release Std semaphore.
        AKRESULT SetStatus(
            AkStmStatus in_eStatus              // New status.
            );

    protected:

        // Synchronisation
        CAkLock             m_lockIO;           // Lock while I/O is pending.

        void *              m_pBuffer;          // Buffer for IO.
        AkUInt32            m_uActualSize;      // Actual size read/written.
        AkReal32            m_fDeadline;        // Deadline. Keeps last operation's deadline.
        
        // Stream settings.
        AkOpenMode          m_eOpenMode     :3; // Either input (read), output (write) or both. 4 values, avoid sign bit.
        
        // Operation info.
        AkStmStatus         m_eStmStatus    :4; // Stream operation status. 5 values, avoid sign bit.
    };

    //-----------------------------------------------------------------------------
    // Name: class CAkAutoStmBase
    // Desc: Base automatic stream implementation.
    //-----------------------------------------------------------------------------
    class CAkAutoStmBase : public CAkObject,
						   public AK::IAkAutoStream,
                           public CAkStmTask
    {
    public:
    
    	// ------------------------------------------------------------------------------
	    // Stream buffer record used for automatic streams memory management.
	    // ------------------------------------------------------------------------------
	    struct AkStmBuffer
	    {
	        void *      pBuffer;
	        AkUInt32    uDataSize;
	    };

        // Construction/destruction.
        CAkAutoStmBase();
        virtual ~CAkAutoStmBase();

        virtual AKRESULT Init( 
            CAkDeviceBase *             in_pDevice,         // Owner device.
            const AkFileDesc &          in_pFileDesc,       // File descriptor.
            const AkAutoStmHeuristics & in_heuristics,      // Streaming heuristics.
            AkAutoStmBufSettings *      in_pBufferSettings, // Stream buffer settings. Pass NULL to use defaults (recommended).
            AkUInt32                    in_uGranularity     // Device's I/O granularity.
            );
        void     Term();

        //-----------------------------------------------------------------------------
        // AK::IAkAutoStream interface.
        //-----------------------------------------------------------------------------

        // Closes stream. The object is destroyed and the interface becomes invalid.
        virtual void      Destroy();

        // Stream info access.
        virtual void      GetInfo(
            AkStreamInfo &      out_info        // Returned stream info.
            );
        // Stream heuristics access.
        virtual void      GetHeuristics(
            AkAutoStmHeuristics & out_heuristics// Returned stream heuristics.
            );
        // Stream heuristics run-time change.
        virtual AKRESULT  SetHeuristics(
            const AkAutoStmHeuristics & in_heuristics   // New stream heuristics.
            );
        // Name the stream (appears in Wwise profiler).
        virtual AKRESULT  SetStreamName(
            AkLpCtstr       in_pszStreamName    // Stream name.
            );
        // Returns I/O block size.
        virtual AkUInt32  GetBlockSize();


        // Operations.
        // ---------------------------------------
        
        // Starts automatic scheduling.
        virtual AKRESULT Start();
        // Stops automatic scheduling.
        virtual AKRESULT Stop();

        // Get stream position.
        virtual AkUInt64 GetPosition( 
            bool *          out_pbEndOfStream   // Set to true if reached end of stream. Can pass NULL.
            );   
        // Set stream position. Modifies position in stream for next read user access.
        virtual AKRESULT SetPosition(
            AkInt64         in_iMoveOffset,     // Seek offset.
            AkMoveMethod    in_eMoveMethod,     // Seek method, from beginning, end or current file position.
            AkInt64 *       out_piRealOffset    // Actual seek offset may differ from expected value when unbuffered IO.
                                                // In that case, floors to sector boundary. Pass NULL if don't care.
            );

        // Data/status access. 
        // -----------------------------------------

        // GetBuffer.
        // Return values : 
        // AK_DataReady     : if buffer was granted.
        // AK_NoDataReady   : if buffer was not granted yet.
        // AK_NoMoreData    : if buffer was granted but reached end of file (next call will return with size 0).
        // AK_Fail          : there was an IO error.
        virtual AKRESULT GetBuffer(
            void *&         out_pBuffer,        // Address of granted data space.
            AkUInt32 &      out_uSize,          // Size of granted data space.
            bool            in_bWait            // Block until data is ready.
            );

        // Release buffer granted through GetBuffer().
        virtual AKRESULT ReleaseBuffer();

        //-----------------------------------------------------------------------------
        // CAkStmTask interface.
        //-----------------------------------------------------------------------------

        // Task management.
        virtual void InstantDestroy();          // Destroys the object.

        // Get information for data transfer.
        virtual void TransferInfo( 
            AkFileDesc *& out_pFileDesc,        // Stream's associated file descriptor.
            OVERLAPPED *& out_pOverlapped,      // Asynchronous data and file position.
            bool &      out_bIsSequential,      // Returned !bIsPositionDirty flag.
            AkUInt32 *  out_puSize              // Required transfer size.
            );

        // Update task after data transfer.
        virtual void Update(
            void *      in_pBuffer,             // Address of data.
            AkUInt32    in_uIOSize,             // Size actually read/written.
            bool        in_bWasIOSuccessful     // IO was successful (no error).
            );
        // Data buffer access.
        // Tries to get a buffer for I/O. It might fail if there is no more memory (returns NULL).
        // Returns the address of the user's buffer.
        virtual void * TryGetIOBuffer();
        // Automatic stream specific. 
        // For buffer reassignment. The scheduler calls this if there is no memory available and it considers
        // that this task could give away some of its buffers.
        virtual void * PopIOBuffer();

        // Returns True if the task is Running and did not reach end-of-file.
        virtual bool IsWaitingForIO();

        // Scheduling heuristics.
        virtual AkReal32 EffectiveDeadline();   // Compute task's effective deadline for next operation, in ms.
        
#ifndef AK_OPTIMIZED
        // IAkStreamProfile interface.
        //----------------------------
        virtual void GetStreamData(
            AkStreamData & out_streamData
            );
#endif
        //-----------------------------------------------------------------------------
        // Helpers.
        //-----------------------------------------------------------------------------

        // File/Client position management.
        void UpdateUserPosition(
            AkUInt32 in_uOffset                 // Relative offset.
            );
        void ForceFilePosition(
            const AkInt64 & in_iNewPosition     // New stream position (absolute).
            );

        // Scheduling status management.
        void UpdateSchedulingStatus(
            bool    in_bForceSignalSem=false    // To force the I/O thread to stay awake for this stream (used to ensure that it is cleaned up right away).
            );

        // Returns a buffer filled with data. NULL if no data is ready.
        void *   GetReadBuffer(     
            AkUInt32 &  out_uSize               // Buffer size.
            );
        // Releases a buffer granted to user. Returns AK_Fail if no buffer was granted,
        // (or in Multiple Access Mode, if the address supplied does not match a buffer that was granted).
        AKRESULT ReleaseReadBuffer( 
            AkUInt32 &  out_uReleasedSize       // Size of buffer released.
            );
        // Flushes all stream buffers that are not currently granted.
        void Flush();
        
    protected:

        AkInt64             m_iTmpSeekPosition; // New temporary position forced by user.
        
        // Stream heuristics.
        AkReal32            m_fThroughput;      // Average throughput in bytes/ms. 
        AkUInt32            m_uLoopStart;       // Set to start of loop (byte offset from beginning of stream) for streams that loop, 0 otherwise.
        AkUInt32            m_uLoopEnd;         // Set to end of loop (byte offset from beginning of stream) for streams that loop, 0 otherwise.
        AkUInt32            m_uMinNumBuffers;   // Specify a minimal number of buffers if you plan to own more than one buffer at a time, 0 or 1 otherwise.

        // Streaming buffers.
        AkUInt32            m_uAvailDataSize;   // Available data size. Used to minimize scheduler computation. Note. Must be 32-bits aligned.
        AkUInt32            m_uLoopEndPosition; // Index of first buffer that corresponds to a look-ahead loop.
        typedef AkArray<AkStmBuffer, const AkStmBuffer&, ArrayPoolLocal, AK_STM_OBJ_POOL_BLOCK_SIZE/sizeof(AkStmBuffer)> AkBuffersArray;
        AkBuffersArray      m_arBuffers;
        AkUInt16            m_uLoopedBufferIdx; // Index of first buffer that corresponds to a look-ahead loop.
        AkUInt8             m_uNextToGrant;     // Index of next buffer to grant (this implementation supports a maximum of 255 concurrently granted buffers).
        AkUInt8             m_uPositionChgIdx;  // Index of buffer which corresponds to the new position forced by user.

        // Stream status.
        AkUInt32            m_bIsRunning    :1; // Running or paused.
        AkUInt32            m_bIOError      :1; // Stream encountered I/O error.
        AkUInt32            m_bRequiresScheduling   :1; // Stream's own indicator saying if it counts in the automatic streams semaphore.
        AkUInt32            m_bFilePosNotSequential :1; // File position is not sequential (due to user position change, look-ahead looping, stm buffer reassigned, or invalidated I/O).
    };
}

#endif //_AK_DEVICE_BASE_H_
