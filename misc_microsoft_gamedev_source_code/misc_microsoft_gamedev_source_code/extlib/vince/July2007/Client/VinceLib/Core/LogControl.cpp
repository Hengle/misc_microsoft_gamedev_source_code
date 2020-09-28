//  LogControl.cpp : Classes to manage log upload status file
//
//  Created 2004/04/22 Rich Bonny <rbonny@microsoft.com>
//
//  MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//  Copyright (c) 2004 Microsoft Corp.  All rights reserved.

#include "LogControl.h"
#include "StringUtil.h"
#include "VinceUtil.h"

namespace Vince
{
    // We declare a global pointer to the upload manager so that we don't
    // have to expose it in the header file. Otherwise, compilation conflicts
    // between Winsock and Winsock2 can arise in the main program.


    // Public methods for CLogControlEntry

    CLogControlEntry::CLogControlEntry(const char* cstrFileName)
    {
        m_FileName = SAFE_COPY(cstrFileName);
        m_Status = NULL;
        m_DateTime = NULL;
        pNext = NULL;
    }

    CLogControlEntry::~CLogControlEntry(void)
    {
        SAFE_DELETE_ARRAY(m_FileName);
        SAFE_DELETE_ARRAY(m_Status);
        SAFE_DELETE_ARRAY(m_DateTime);
    }

    // Save logging/update status as string member
    void CLogControlEntry::SetStatus(const char* cstrStatus)
    {
        // Release previous string allocation

        if ( NULL != m_Status )
        {
            SAFE_DELETE_ARRAY(m_Status);
        }

        // Copy status string

        m_Status = SAFE_COPY(cstrStatus);
    }

    // Set the timestamp associated with current entry. Updated
    // whenever status changes or from control file entries at
    // load time.
    void CLogControlEntry::SetTimestamp(const char* cstrDateTime)
    {
        // Release previous string allocation

        if ( NULL != m_DateTime )
        {
            SAFE_DELETE_ARRAY(m_DateTime);
        }

        // Passing in a null string means get current time

        if ( NULL != cstrDateTime )
        {
            m_DateTime = SAFE_COPY(cstrDateTime);
        }
        else
        {
            m_DateTime = GetCurrentDateTimeString();
        }
    }

    // Methods for CLogControl

    CLogControl::CLogControl(void)
    {
        m_pUploadManager = NULL;
        m_pLogEntryHead = NULL;
        m_cstrLogBaseName = NULL;
        m_cstrMachineName = NULL;
        m_bCompressed = false;
        m_bEncrypted = false;
        m_bRetail = false;
        m_sequenceNumber = 1;
    }

    CLogControl::~CLogControl(void)
    {
        Clear();
        
        if(m_pUploadManager != NULL) {
            m_pUploadManager->WaitForAllFinished();
            delete m_pUploadManager;
        }
 
        // This string is determined at run time, not by config settings,
        // so it must be deleted.
        SAFE_DELETE_ARRAY(m_cstrMachineName);

        // Do not attempt to delete the other member string objects,
        // since these are just pointers to settings strings.
        // The CSettings destructor is responsible for these.
    }

    void CLogControl::InitUploadManager(const char* titleName, const char* webServer, const char* aspxPage, unsigned short iPort,
                                        bool bDeleteLog, bool bUseLSP, DWORD titleID, int cpuNum)
    {
        m_pUploadManager = new LogUploadManager(titleName, webServer, aspxPage, iPort, bDeleteLog, bUseLSP, titleID);
        m_pUploadManager->Init(cpuNum);
    }


    // Load in the file that keeps track of current file upload status
    bool CLogControl::Load()
    {
        // We do none of this for a retail build, which only manages one file at at time
        if (m_bRetail)
        {
            return true;
        }

        // First clear any previous data

        Clear();

        FILE* fControl = VinceFileOpen(CONTROL_FILE, "rt", true);
        bool bValidFile = false;
        bool bEntryComplete = true;
        CLogControlEntry* pEntry = NULL;
        int iLineCount = 0;
        char line[256] = "";

        if (fControl) 
        {
            bValidFile = true;
            while ( bValidFile && !feof(fControl) ) 
            {
                // Should be no blank lines

                if ( NULL == fgets(line, sizeof(line)-1, fControl) )
                {
                    bValidFile = false;
                }
                else
                {
                    size_t length = strlen(line);
                    // trim off new line
                    if ( length > 0 )
                    {
                        line[length - 1] = '\0';
                    }

                    // First line contains next file sequence number

                    if ( 0 == iLineCount )
                    {
                        int intResult = 0;
                        int retValue = sscanf_s( line, "%i", &intResult );
                        if ( 1 == retValue )
                        {
                            m_sequenceNumber = intResult;
                        }
                        else
                        {
                            m_sequenceNumber = 1;
                            bValidFile = false;
                        }
                    }

                    // Each entry contains three lines containing file name, status, and time stamp

                    else
                    {
                        switch ( iLineCount % 3 )
                        {
                            case 1:
                                pEntry = AddEntry(line);
                                bEntryComplete = false;
                                break;
                            case 2:
                                // Any entry that said it was logging from a previous run
                                // should be awaiting upload.
                                if ( 0 == strcmp(line, "Logging") )
                                    pEntry->SetStatus("Waiting");
                                else
                                    pEntry->SetStatus(line);
                                bEntryComplete = false;
                                break;
                            case 0:
                                pEntry->SetTimestamp(line);
                                bEntryComplete = true;
                                break;
                        }
                    }
                    iLineCount++;
                }   // else
            } // while
            fclose(fControl);
            bValidFile = bEntryComplete;
        }
        return bValidFile;
    }

    // Update the status of each log file and queue jobs for upload.
    void CLogControl::Refresh()
    {

        // Bypass for retail build
        if (m_bRetail)
        {
            return;
        }

        CLogControlEntry* pEntry = GetFirstEntry();
        CLogControlEntry* pNextEntry = NULL;

        // Unless the upload manager is active, there is nothing to update here

        if ( NULL != m_pUploadManager )
        {
            while (NULL != pEntry) 
            {
                // Get next now, before possible deletion

                pNextEntry = GetNextEntry(pEntry);

                // Take appropriate action based on current status.
                // Actually, there is really very little we can do
                // except for initiating pending upload requests.
                // Attempting to ascertain status of jobs is difficult
                // and not that valuable, since it can only be attempted
                // for upload requests initiated from the current session.

                if ( 0 == strcmp(pEntry->GetStatus(), "Waiting") )
                {
                    // Queue for upload and update status.
                    // There really is no way for GetFilename() to ever return a null value, but
                    // we will check anyway, just for safety sake.
                    const char* cstrLogFileName = GetFullFileName(pEntry->GetFilename(), true);
                    if ( NULL == cstrLogFileName )
                    {
                        pEntry->SetStatus("Failed");
                        pEntry->SetTimestamp( NULL );
                        ReportActivity(pEntry, "Error: Could not recover file name");
                    }
                    else
                    {
                        m_pUploadManager->QueueFileUpload(cstrLogFileName);
                        pEntry->SetStatus("Uploading");
                        pEntry->SetTimestamp( NULL );
                        ReportActivity(pEntry, "Beginning Upload");
                    }
                }

                else if ( 0 == strcmp(pEntry->GetStatus(), "Uploading") )
                {
                    // We will not confirm success; just assume it is okay and
                    // delete the entry. This will be modified pending revisions
                    // to CUploadManager to return upload status.

                    pEntry->SetTimestamp( NULL );
                    ReportActivity(pEntry, "Removing Job");
                    RemoveEntry(pEntry);
                }

                pEntry = pNextEntry;

            } // while
        } // if
        Save();
    }

    // Flush all requests and block until done
    void CLogControl::Flush()
    {
        // If Auto-Upload is disabled, upload manager will
        // be null, so check before trying to flush
        if ( NULL != m_pUploadManager )
        {
            m_pUploadManager->WaitForAllFinished();
        }
    }


    // Save status to file. Overwrite previous version.
    void CLogControl::Save()
    {
        // Bypass for retail build
        if (m_bRetail)
        {
            return;
        }

        FILE* fControl = VinceFileOpen(CONTROL_FILE, "w", true);

        if (fControl)
        {
            // Write the current sequence number as first line of file

            fprintf(fControl, "%d\n", m_sequenceNumber);

            // Write three lines for each entry
            
            CLogControlEntry* pEntry = GetFirstEntry();

            while (NULL != pEntry) 
            {
                fprintf( fControl, "%s\n", pEntry->GetFilename() );
                fprintf( fControl, "%s\n", pEntry->GetStatus() );
                fprintf( fControl, "%s\n", pEntry->GetTimeStamp() );
                pEntry = GetNextEntry(pEntry);
            } // while
            fclose(fControl);
        }
    }

    // Delete all current entries from upload control list
    void CLogControl::Clear()
    {
        CLogControlEntry* pCurrent = m_pLogEntryHead;
        CLogControlEntry* pNext = NULL;

        while ( NULL != pCurrent )
        {
            pNext = pCurrent->pNext;
            SAFE_DELETE(pCurrent);
            pCurrent = pNext;
        }

        m_pLogEntryHead = NULL;
    }

    // Used in retail configurations to place a file on the upload queue
    // without going through the entry queue
    void CLogControl::QueueLogForUpload()
    {
        const char* cstrLogFileName = GetFullFileName(GetLogFileName(), true);
        if(cstrLogFileName != NULL) 
        {
            if(m_pUploadManager != NULL) 
            {
                m_pUploadManager->QueueFileUpload(cstrLogFileName);
            }
        }
        SAFE_DELETE_ARRAY(cstrLogFileName);
    }

    // Assign a value for first part of log file names
    void CLogControl::SetBaseName(const char* cstrBaseName)
    {
        m_cstrLogBaseName = cstrBaseName;
    }

    // Assign machine name used as second part of log file names
    void CLogControl::SetMachineName(const char* cstrMachineName)
    {
        m_cstrMachineName = cstrMachineName;
    }

    // Return machine name
    const char* CLogControl::GetMachineName() const
    {
        return m_cstrMachineName;
    }

    // Construct a log file name from base name, machine name, and sequence number
    // Path information will be added by VinceFileOpen
    const char* CLogControl::GetLogFileName()
    {
        char buffer[MAX_PATH];
        _snprintf_s(buffer, MAX_PATH, MAX_PATH - 1, "%s_%s_%04d.log", m_cstrLogBaseName, m_cstrMachineName, m_sequenceNumber);
        
        // Compressed files have .cmp added to the name
        if (m_bCompressed)
        {
            strncat_s(buffer, MAX_PATH, ".cmp", 4);
        }

        // Encrypted files have .enc added to the name
        if (m_bEncrypted)
        {
            strncat_s(buffer, MAX_PATH, ".enc", 4);
        }

        buffer[MAX_PATH - 1] = '\0';
        return SAFE_COPY(buffer);
    }

    // Add a file upload entry to end of linked list
    CLogControlEntry* CLogControl::AddEntry(const char* cstrFilename)
    {
        CLogControlEntry* pNewEntry = new CLogControlEntry(cstrFilename);

        if ( NULL == m_pLogEntryHead )
        {
            m_pLogEntryHead = pNewEntry;
        }
        else
        {
            CLogControlEntry* pNext = m_pLogEntryHead;
            while ( NULL != pNext->pNext )
            {
                pNext = pNext->pNext;
            }
            pNext->pNext = pNewEntry;
        }
        return pNewEntry;
    }

    // Remove the specified entry from the linked list
    void CLogControl::RemoveEntry(CLogControlEntry* pEntry)
    {
        CLogControlEntry* pPreviousEntry = NULL;
        CLogControlEntry* pCurrentEntry = m_pLogEntryHead;

        // Find the entry. We assume it is actually there,
        // which should be a safe assumption, but we will
        // check for the end of the list, just in case

        while ( pEntry != pCurrentEntry && NULL != pCurrentEntry)
        {
            pPreviousEntry = pCurrentEntry;
            pCurrentEntry = pCurrentEntry->pNext;
        }

        // This should never happen, but just to be safe:
        if ( NULL == pCurrentEntry )
        {
            // Don't remove anything; couldn't find it
            return;
        }

        // Remove from list and delete
        if ( NULL == pPreviousEntry )
        {
            m_pLogEntryHead = pEntry->pNext;
        }
        else
        {
            pPreviousEntry->pNext = pEntry->pNext;
        }
        SAFE_DELETE(pEntry);
    }

    // Get the entry at the head of the linked list
    CLogControlEntry* CLogControl::GetFirstEntry()
    {
        return m_pLogEntryHead;
    }

    // Get the entry after the specified entry on the linked list
    CLogControlEntry* CLogControl::GetNextEntry(CLogControlEntry* pEntry)
    {
        if ( NULL == pEntry )
            return NULL;
        else
            return pEntry->pNext;
    }

    // Go to the end of the linked list
    CLogControlEntry* CLogControl::GetLastEntry()
    {
        // Last entry is the most current log file.

        CLogControlEntry* pNext = m_pLogEntryHead;
        while ( NULL != pNext->pNext )
        {
            pNext = pNext->pNext;
        }
        return pNext;
    }

    // Write status message to history file.
    void CLogControl::ReportActivity( CLogControlEntry* pEntry, const char* cstrMessage ) const
    {

        // Bypass for retail build
        if (m_bRetail)
        {
            return;
        }

        FILE* fReport= VinceFileOpen(REPORT_FILE, "a+", true);

        if (fReport)
        {
            // if the Entry pointer is null, this is a general message and not related
            // to a specific entry.

            if ( NULL == pEntry )
            {
                const char* cstrDateTime = GetCurrentDateTimeString();
                fputs( cstrDateTime, fReport );
                fputs( "  Upload Problem -- ", fReport );
                fputs( cstrMessage, fReport );
                fputs( "\n", fReport );
                SAFE_DELETE_ARRAY(cstrDateTime);
            }
            else
            {
                fputs( pEntry->GetTimeStamp(), fReport );
                fputs( "  ", fReport );
                fputs( pEntry->GetFilename(), fReport );
                fputs( "  ", fReport );
                fputs( cstrMessage, fReport );
                fputs( "\n", fReport );
            }
            fclose(fReport);
        }
    }

}