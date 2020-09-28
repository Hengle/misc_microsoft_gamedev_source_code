//	LogWriter.h : PlaceHolder for writing to log file.
//  This is a thin wrapper that hands off the actual logging
//  to the logger component.
//
//	Created 2004/03/10 Rich Bonny <rbonny@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2004 Microsoft Corp.  All rights reserved.

#pragma once

#include "IEncryption.h"
#include "ILogFileWriter.h"

#define VINCE_VERSION	"02.03.04.1002"

namespace Vince
{
    // Forward declare classes and avoid excessive includes
    class CLogControl;
    class EventString;
    class WriteBuffer;

    class CLogWriter
	{
	public:
		CLogWriter(void);
		~CLogWriter(void);

        void Initialize();
		void Open();
		void Close();
		void Cycle();
		void Transmit();
      bool IsUploading();
		void WriteEventTag(const char* cstrName);
		void WriteEventTail();
		void WriteParameter(const char* cstrName,  const char* cstrType, const char* cstrValue);
		void WriteParameter(const char* cstrName,  const char* cstrType, const wchar_t* wcstrValue);
		void WriteParameter(const char* cstrName,  const char* cstrType, const bool bValue);
		void WriteParameter(const char* cstrName,  const char* cstrType, const BYTE bValue);
		void WriteParameter(const char* cstrName,  const char* cstrType, const int iValue);
		void WriteParameter(const char* cstrName,  const char* cstrType, const DWORD dwValue);
		void WriteParameter(const char* cstrName,  const char* cstrType, const long lValue);
		void WriteParameter(const char* cstrName,  const char* cstrType, const float fValue);
		void WriteParameter(const char* cstrName,  const char* cstrType, const double fValue);
		void WriteError(const char* cstrSource, const char* cstrLocation, const char* cstrMessage);
		void WriteError(const wchar_t* wcsSource, const wchar_t* wcsLocation, const wchar_t* wcsMessage);
        HRESULT SetLogFileWriter(ILogFileWriter* pLogFileWriter);
		bool IsLoggingActive();
		void SetCompressed(bool setting);
		void SetEncrypted(bool setting);
		void SetRetail(bool setting);

	protected:
		void WriteEncryptionHeader(const char* cstrData, unsigned int cSize);
		void WriteFileHeader();
		void Write(char* pBuffer, int byteCount);
		void WriteLogData(char* srcData, int byteCount);
		void WriteToLogFile(const char* cData, int count);
		void WriteBytes(const char* cData, int count);
		void WriteBytesAsync(const char* cData, int count);
        void StartAsynchronousThread(int cpuNum);
        static DWORD WINAPI AsyncThreadProc(void* param);
        DWORD AsyncThreadProcImpl();
		void FlushBuffer();

		const char* m_LogFileName;
		const char* m_cstrSession;
		bool m_fInitialized;
		bool m_fLoggingActive;
		bool m_fLogOpened;
		bool m_fAutoUpload;
		bool m_fBuffered;
		bool m_fUnicode;
        bool m_fThreadSafe;
        bool m_fAsynchronous;
        int m_pAsynchBufferSize;
        bool m_fAddingBuffer;
        bool m_fShutdownAsyncThread;
        WriteBuffer* m_pAsynchBuffer;
        HANDLE m_threadHandle;
        CRITICAL_SECTION m_lock;
		unsigned int m_iBufferSize;
		unsigned int m_iBufferPos;
		EventString* m_pEventString;
		char* m_cstrOutputBuffer;
        ILogFileWriter* m_pCustomFileWriter;
		CLogControl* m_pLogControl;
        TnT::IEncryption* m_pEncryption;
	};
}
