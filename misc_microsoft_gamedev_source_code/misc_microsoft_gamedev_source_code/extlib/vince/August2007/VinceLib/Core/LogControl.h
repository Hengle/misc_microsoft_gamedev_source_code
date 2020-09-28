//	LogControl.h : Classes to manage log upload status file
//
//	Created 2004/04/22 Rich Bonny <rbonny@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2004 Microsoft Corp.  All rights reserved.

#pragma once

#define CONTROL_FILE "UploadControl.dat"
#define REPORT_FILE "LoggingHistory.log"

#include "LogUploadManager.h"

namespace Vince
{
	class CLogControlEntry
	{

	public:
		CLogControlEntry(const char* cstrFileName);
		~CLogControlEntry(void);
		void SetStatus(const char* cstrStatus);
		void SetTimestamp(const char* cstrDateTime);
		const char* GetFilename() const { return m_FileName; }
		const char* GetStatus() const { return m_Status; }
		const char* GetTimeStamp() const { return m_DateTime; }

		CLogControlEntry* pNext;

	protected:
		const char* m_FileName;
		const char* m_Status;
		const char* m_DateTime;
	};

	class CLogControl
	{
	public:
		CLogControl(void);
		~CLogControl(void);

		bool Load();
		void Refresh();
		void Save();
		void Clear();
        void Flush();
		void QueueLogForUpload();
		void InitUploadManager();
		void SetBaseName(const char* cstrBaseName);
		const char* GetMachineName() const;
		void SetMachineName(const char* cstrMachineName);
		void SetProjectName(const char* cstrProjectName);
		const char* GetLogFileName();
		void ReportActivity( CLogControlEntry* pEntry, const char* cstrMessage ) const;
		CLogControlEntry* AddEntry(const char* cstrFilename);
		void RemoveEntry(CLogControlEntry* pEntry);
		CLogControlEntry* GetFirstEntry();
		CLogControlEntry* GetNextEntry(CLogControlEntry* pEntry);
		CLogControlEntry* GetLastEntry();
		bool m_bCompressed;
		bool m_bEncrypted;
		bool m_bRetail;

		int m_sequenceNumber;

	protected:
        LogUploadManager* m_pUploadManager;
		CLogControlEntry* m_pLogEntryHead;
		const char* m_cstrLogBaseName;
		const char* m_cstrMachineName;
		const char* m_cstrProjectName;
	};
}
