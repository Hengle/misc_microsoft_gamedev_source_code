//--------------------------------------------------------------------------------------
//  TicketTracker : Ticket Tracker interface
//
//  Modified 2006/05/19 Dan Berke     <dberke@microsoft.com>
//  - Refactored into classes
//  Created 2005/04/13 Rich Bonny    <rbonny@microsoft.com>
//  - based on original Ticket Tracker code:
//  Created by Ian Latham <ilatham@microsoft.com>
//
//  MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//  Microsoft Game Studios Tools and Technology (TnT)
//  Copyright (c) 2006 Microsoft Corp.  All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include <xtl.h>
#include "UploadInfo.h"
#include "ICompression.h"
#include "IEncryption.h"

#define TT_VERSION  L"00.00.00.0000"

template< typename T > class TestFixture;

namespace TicketTracker 
{
    // Dump types that can be generated
    enum DumpType {
        InclusiveOnly,
        InclusiveCommitted,
        ExclusiveCommitted
    };

    // Error codes
    enum TTResult {
        Success                             = 0x00000000,
        ErrorReadingConfigFile              = 0x80000001,
        ErrorWritingDump                    = 0x80000002,
        ErrorNoUploader                     = 0x80000003,
        ErrorNoTitleNameSpecified           = 0x80000004,
        ErrorGetUploadSessionFailed         = 0x80000005,
        ErrorUploadFailed                   = 0x80000006,
        ErrorCreatingFileList               = 0x80000007
    };

    // Status listener interface for objects that want to observe TicketTracker events
    class IStatusListener
    {
    public:
                 IStatusListener() {}
        virtual ~IStatusListener() {}

        virtual void OnBeginTicketTracker()                                     = 0;
        virtual void OnFinishTicketTracker()                                    = 0;
        virtual void OnTakeScreenshot(const char* filename)                     = 0;
        virtual void OnCreateMinidump(const char* filename)                     = 0;
        virtual void OnCompressFile(const char* filename)                       = 0;
        virtual void OnEncryptFile(const char* filename)                        = 0;
        virtual void OnCreateBuildVersionFile(const char* filename)             = 0;
        virtual void OnStartUpload(const char* uploaderName)                    = 0;
        virtual void OnFinishUpload(TTResult result)                            = 0;
        virtual void OnStartUploadFile(const char* filename)                    = 0;
        virtual void OnFinishUploadFile(const char* filename, TTResult result)  = 0;
    };


    // Ticket Tracker interface
    class ITicketTracker
    {
    public:
                 ITicketTracker() {}
        virtual ~ITicketTracker() {}

        // Initialization
        virtual TTResult            LoadSettings(const char* filename)   = 0;
        virtual void                SetDevice(IDirect3DDevice9* pDevice) = 0;
        virtual IDirect3DDevice9*   GetDevice()                          = 0;

        // Basic settings
        virtual void                SetBaseFolder(const char* folder)             = 0;
        virtual void                SetTitleName(const char* titleName)           = 0;
        virtual void                SetBuildVersion(const char* verStr)           = 0;
        virtual void                SetAutoCreateBuildVersionFile(bool bCreate)   = 0;
        virtual void                SetStatusListener(IStatusListener* pListener) = 0;
        virtual const char*         GetBaseFolder()                               = 0;
        virtual const char*         GetTitleName()                                = 0;
        virtual const char*         GetBuildVersion()                             = 0;
        virtual bool                GetAutoCreateBuildVersionFile()               = 0;
        virtual IStatusListener*    GetStatusListener()                           = 0;

        // Minidump settings
        virtual void                InstallExceptionHandler()                             = 0;
        virtual void                RemoveExceptionHandler()                              = 0;
        virtual void                EnableMinidumps(bool bEnable)                         = 0;
        virtual void                EnableMinidumpsWhenDebuggerPresent(bool bEnable)      = 0;
        virtual void                OverwriteDumpFiles(bool bOverwrite)                   = 0;
        virtual void                SetDumpType(DumpType type)                            = 0;
        virtual void                SetTakeScreenShot(bool bTake)                         = 0;
        virtual void                SetReservedMemory(int numPages)                       = 0;
        virtual void                AddCustomMemoryStream(void *pMem, unsigned long size) = 0;
        virtual bool                MinidumpsEnabled()                                    = 0;
        virtual bool                MinidumpsWhenDebuggerPresentEnabled()                 = 0;
        virtual bool                OverwriteDumpFilesEnabled()                           = 0;
        virtual DumpType            GetDumpType()                                         = 0;
        virtual bool                GetTakeScreenShot()                                   = 0;
        virtual int                 GetNumReservedMemoryPages()                           = 0;

        // Compression and encryption
        virtual void                EnableCompression(bool bEnable)             = 0;
        virtual void                EnableEncryption(bool bEnable)              = 0;
        virtual void                SetCompressor(TnT::ICompression* pCompress) = 0;
        virtual void                SetEncryptor(TnT::IEncryption* pEncrypt)    = 0;
        virtual void                DeleteIntermediateFiles(bool bDelete)       = 0;
        virtual bool                CompressionEnabled()                        = 0;
        virtual bool                EncryptionEnabled()                         = 0;
        virtual TnT::ICompression*  GetCompressor()                             = 0;
        virtual TnT::IEncryption*   GetEncryptor()                              = 0;
        virtual bool                DeleteIntermediateFilesEnabled()            = 0;

        // Upload settings
        virtual void                EnableUploads(bool bEnable)					= 0;
        virtual void                DeleteDumpFilesAfterUpload(bool bDelete)	= 0;
        virtual void                AddUploadInfo(UploadInfo* pUploadInfo)		= 0;
        virtual void                AddUploadFile(const char* filename)			= 0;
        virtual bool                UploadsEnabled()							= 0;
        virtual bool                DeleteDumpFilesAfterUploadEnabled()			= 0;
		virtual void				SetUploadTimeout(int ms)				    = 0;
		virtual void			    UseNonBlockingSockets(bool bUseNonblocking) = 0;
		virtual unsigned int        GetNumUploadInfo()							= 0;
        virtual UploadInfo*         GetUploadInfo(unsigned int index)			= 0;
        virtual UploadInfo*         GetUploadInfo(const char* name)				= 0;
        virtual unsigned int        GetNumUploadFiles()							= 0;
        virtual const char*         GetUploadFile(unsigned int fileNum)			= 0;
		virtual int					GetUploadTimeout()							= 0;
		virtual bool				GetUseNonBlockingSockets()					= 0;

        // Exception handler to be called in catch()
        virtual long ExceptionHandler(unsigned long exceptionCode, LPEXCEPTION_POINTERS exceptionInformation) = 0;

        // Error status
        static const char* GetErrorText(TTResult err);

        // Test hook
        template<typename T> friend class TestFixture;
    };

    class TicketTrackerFactory
    {
    public:
        static ITicketTracker* GetTicketTracker();
    };
}