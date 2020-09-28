

//	LogWriter.h : PlaceHolder for writing to log file.
//  This is a thin wrapper that hands off the actual logging
//  to the logger component.
//
//	Created 2004/03/10 Rich Bonny <rbonny@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2004 Microsoft Corp.  All rights reserved.

#pragma once

#include "TnTSettings.h" // benst: *** IMPORTANT *** - RENAMED to "TnTSettings.h" because Phoenix has a Settings.h that this otherwise conflicts with
#include "LogControl.h"
#include "IEncryption.h"
#include "EventString.h"

#define VINCE_VERSION	"02.02.08.0401"

namespace Vince
{
	class CVince;

	class CLogWriter
	{
	public:
		CLogWriter(void);
		~CLogWriter(void);

      void Initialize( CVince* vince, BOOL enableLog );
		void Open(CVince* vince);
		void Close();
		void Cycle(CVince* vince);
		void Transmit();
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
		bool IsLoggingActive();
		void SetCompressed(bool setting);
		void SetEncrypted(bool setting);
		void SetRetail(bool setting);

	protected:
		void WriteEncryptionHeader(const char* cstrData, unsigned int cSize);
		void WriteFileHeader(CVince* vince);
		void Write(char* pBuffer, int byteCount);
		void WriteLogData(char* srcData, int byteCount);
		void WriteBytes(const char* cData, int count);
		void FlushBuffer();

		const char* m_LogFileName;
		const char* m_cstrSession;
		bool m_fInitialized;
		bool m_fLoggingActive;
		bool m_fLogOpened;
		bool m_fAutoUpload;
		bool m_fBuffered;
		bool m_fUnicode;
		unsigned int m_iBufferSize;
		unsigned int m_iBufferPos;
		EventString* m_pEventString;
		char* m_cstrOutputBuffer;
		CLogControl* m_pLogControl;
        TnT::IEncryption* m_pEncryption;
	};
}



