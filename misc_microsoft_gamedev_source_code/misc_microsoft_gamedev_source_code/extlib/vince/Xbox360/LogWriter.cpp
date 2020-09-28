//  LogWriter.cpp : Manages writing to log file
//  This hands off the actual logging to the logger component
//  if buffered file writing is enabled (TBD). Otherwise
//  it does simple appends to disk file
//
//  Created 2004/03/08 Rich Bonny <rbonny@microsoft.com>
//
//  MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//  Copyright (c) 2004 Microsoft Corp.  All rights reserved.

#include "VinceControl.h"

#ifdef _VINCE_

#include "Vince.h"
#include "LogWriter.h"
#include "TnTUtil.h"
#include "VinceUtil.h"

#ifndef NO_VINCE_COMPRESSION
#include "ZlibCompression.h"
#endif

#ifdef _XBOX
#include "XboxEncryption.h"
#else
#include "CryptoAPIEncryption.h"
#endif


namespace Vince
{
    CLogWriter::CLogWriter(void) :
        m_LogFileName(NULL),
        m_cstrSession(NULL),
        m_fLoggingActive(false),
        m_fInitialized(false),
        m_fLogOpened(false),
        m_fAutoUpload(false),
        m_fBuffered(false),
        m_fUnicode(false),
        m_iBufferSize(0),
        m_iBufferPos(0),
        m_pEventString(NULL),
        m_cstrOutputBuffer(NULL),
        m_pLogControl(NULL),
        m_pEncryption(NULL)
    {
    }

    CLogWriter::~CLogWriter(void)
    {
        SAFE_DELETE_ARRAY(m_LogFileName);
        SAFE_DELETE_ARRAY(m_cstrSession);
        SAFE_DELETE_ARRAY(m_cstrOutputBuffer);
        SAFE_DELETE(m_pLogControl);
        delete(m_pEventString);
        delete(m_pEncryption);
    }

    void CLogWriter::Initialize( CVince* vince, BOOL enableLog )
    {
        // Initialize only once.
        if (!m_fInitialized)
        {
            if( enableLog )
            {
                // First we check to see if logging has been completely disabled
                m_fLoggingActive = vince->pSettings->Fetch( "Logging", false );
            }
            else
            {
                m_fLoggingActive = false;
            }

            if ( !m_fLoggingActive )
            {
                // We will also disable Surveys. This overrided any setting in the
                // .ini file since it happens after that has been loaded.
                vince->pSettings->AddSetting( "PresentSurveys=false" );

                // Skip everything else
                m_fInitialized = true;
                return;
            }

            // We now check to see if the output files have been redirected from
            // .ini file settings
            const char* cstrLogFolder = vince->pSettings->Fetch( "LogFolder", "" );
            if ( strlen(cstrLogFolder) > 0 )
            {
                SetVinceDestinationFolder( cstrLogFolder );
            }

            m_pLogControl = new CLogControl();

            // Apply Compression, Encryption, and Retail settings
#ifdef NO_VINCE_COMPRESSION
            SetCompressed(false);
#else
            SetCompressed(vince->pSettings->Fetch("CompressLog", false));
#endif
            SetEncrypted(vince->pSettings->Fetch("EncryptLog", false));
            SetRetail(vince->pSettings->Fetch("Retail", false));
            m_fUnicode = vince->pSettings->Fetch("UnicodeLog", false);
            m_pEventString = new EventString(500, m_fUnicode);

            // Load settings from .ini file and save to static variables

            const char* cstrLogBaseName = vince->pSettings->Fetch( "LogFileBase", "Vince" );         
            m_pLogControl->SetBaseName(cstrLogBaseName);

            // Machine name comes from system

            const char* cstrMachineName = GetMachineName();         
            m_pLogControl->SetMachineName(cstrMachineName);

            const char* cstrSession = vince->pSettings->Fetch( "Session", "Unknown" );
            m_cstrSession = SAFE_COPY(cstrSession);

            // Check for buffered settings
            m_fBuffered = vince->pSettings->Fetch( "LogBuffered", false );
            if (m_fBuffered)
            {
                m_iBufferSize = vince->pSettings->Fetch( "BufferSize", 65536 );
                if ( m_iBufferSize < 1024 )
                {
                    m_iBufferSize = 1024;   // Enforce min buffer size of 1024
                }
                m_cstrOutputBuffer = new char[m_iBufferSize];
            }
            else
            {
                m_cstrOutputBuffer = NULL;
            }

            // If AutoUpload of log files is enabled, initialize the upload manager

            m_fAutoUpload = vince->pSettings->Fetch( "AutoUpload", false );
            if ( m_fAutoUpload )
            {
                const char* project   = vince->pSettings->Fetch("Project", "Unknown");
                const char* webServer = vince->pSettings->Fetch("UploadWebServer", "");
                const char* aspxPage  = vince->pSettings->Fetch("UploadaspxPage", "");
                int iPort             = vince->pSettings->Fetch("WebPort", 80);
                bool bDeleteLog       = vince->pSettings->Fetch("DeleteLogAfterUpload", false);
                bool bUseLSP          = vince->pSettings->Fetch("UseLSP", false);
                DWORD titleID         = vince->pSettings->Fetch("TitleID", 0x00000000);
                int cpuNum            = vince->pSettings->Fetch("UploadCPU", -1);
                m_pLogControl->InitUploadManager(project, webServer, aspxPage, (unsigned short)iPort, bDeleteLog, bUseLSP, titleID, cpuNum);
            }

            // Load job list for Log Controller and refresh status
            // However, refresh can be suppressed by .ini file setting

            m_pLogControl->Load();

            bool fUploadOnInit = vince->pSettings->Fetch( "UploadOnInit", true );
            if (fUploadOnInit)
                m_pLogControl->Refresh();

            m_LogFileName = NULL;
            Open(vince);
            m_fInitialized = true;
        }
    }

    void CLogWriter::SetCompressed(bool setting)
    {
        if (m_pLogControl)
            m_pLogControl->m_bCompressed = setting;
    }

    void CLogWriter::SetEncrypted(bool setting)
    {
        if (m_pLogControl)
            m_pLogControl->m_bEncrypted = setting;
    }

    void CLogWriter::SetRetail(bool setting)
    {
        if (m_pLogControl)
            m_pLogControl->m_bRetail = setting;
    }

    // Indicate whether logging is enabled
    bool CLogWriter::IsLoggingActive()
    {
        return m_fLoggingActive;
    }

    // Create the log file and clear out any
    // previous contents. This will be replaced by
    // an initialization call to the logger object
    // if buffered disk writing is enabled (TBD).
    void CLogWriter::Open(CVince* vince)
    {
        if ( !m_fLoggingActive )
            return;

        SAFE_DELETE_ARRAY(m_LogFileName);
        m_LogFileName = m_pLogControl->GetLogFileName();

        FILE* fLog = VinceFileOpen(m_LogFileName, "w", true);
        if ( NULL == fLog )
        {
            if(!(m_pLogControl->m_bRetail)) 
            {
                // Perhaps this failed because of a funky filename. If so, we will try to open as Vince.log
                // and report the error. Otherwise, we won't have much luck opening the log file to report
                // the error.
                char buffer[MAX_PATH + 50];
                _snprintf(buffer, MAX_PATH + 49, "Error trying to open log file: %s; using Vince.log", m_LogFileName);
                buffer[MAX_PATH +49] = '\0';
                SAFE_DELETE_ARRAY(m_LogFileName);
                m_LogFileName = SAFE_COPY("Vince.log");
                fLog = VinceFileOpen(m_LogFileName, "a+", true);
                if (fLog)
                {
                    fclose(fLog);       // WriteError will reopen
                    WriteError("LogWriter", "Open", buffer);
                    // Log creation successful
                    m_fLogOpened = true;
                }
            }
        }
        else
        {
            // Log creation successful
            fclose(fLog);
            m_fLogOpened = true;
        }

        // Report file to Log Control Manager
        // but not if the Retail flag is set.
        if (!m_pLogControl->m_bRetail)
        {
            CLogControlEntry* pEntry = m_pLogControl->AddEntry(m_LogFileName);
            pEntry->SetStatus("Logging");
            pEntry->SetTimestamp( NULL );
            m_pLogControl->ReportActivity(pEntry, "Log Opened");
            m_pLogControl->m_sequenceNumber++;
            m_pLogControl->Save();
        }

        // If we are encrypting, write the encryption header first
        if (m_pLogControl->m_bEncrypted)
        {
            if(m_pEncryption == NULL)
            {
#ifdef _XBOX
                m_pEncryption = new TnT::XboxEncryption();
#else
                m_pEncryption = new TnT::CryptoAPIEncryption();
#endif
            }
             
            // If Encrytped is true and we can not initialize successfully,
            // we disable logging entirely
            if (!m_pEncryption->Initialize())
            {
                m_fLoggingActive = false;
                return;
            }
            else
            {
                char* header = NULL;
                unsigned int cSize = m_pEncryption->CreateHeader(&header);
                WriteEncryptionHeader(header, cSize);
                SAFE_DELETE_ARRAY(header);
            }
        }

        WriteFileHeader(vince);
    
        // Finally, check to see if there were problems with the ini file
        // and report these to the log file.
        const char* cstrConfigError = vince->pSettings->GetErrorMessage();
        {
            if ( (cstrConfigError != NULL) && (cstrConfigError[0] != '\0') )
            {
                WriteError("Settings", "Load", cstrConfigError);
            }
        }
    }

    // Asynchronous files need to be closed (TBD)
    // Mark the file for processing by the upload manager.
    void CLogWriter::Close()
    {
        if ( !m_fLoggingActive )
            return;

        if (m_fBuffered)
        {
            FlushBuffer();
        }
        m_fLogOpened = false;

        // If we are set for retail, just queue the job without going
        // through Log Control Entry list
        if (m_pLogControl->m_bRetail)
        {
            m_pLogControl->QueueLogForUpload();
            // Toggle the sequence number
            m_pLogControl->m_sequenceNumber = 3 - m_pLogControl->m_sequenceNumber;
        }
        else
        {
            CLogControlEntry* pEntry = m_pLogControl->GetLastEntry();
            pEntry->SetTimestamp( NULL );
            m_pLogControl->ReportActivity(pEntry, "Log Closed");
            pEntry->SetStatus("Waiting");
            m_pLogControl->Refresh();
        }
    }

    // Close the current log file and open the next one in the sequence
    void CLogWriter::Cycle(CVince* vince)
    {
        if ( !m_fLoggingActive )
            return;

        Close();

        // We no longer require Retail builds to flush the job queue, since this
        // could result in a client freeze if network calls time out.
        //if (m_pLogControl->Retail)
        //{
        //  Transmit();
        //}

        Open(vince);
    }

    // Force uploading of all pending files
    void CLogWriter::Transmit()
    {
        if ( !m_fLoggingActive )
            return;

        m_pLogControl->Flush();
    }

    // Write the encryption header so that the receiving server can
    // derive the encryption key
    void CLogWriter::WriteEncryptionHeader(const char* cstrData, unsigned int cSize)
    {
        // If file status in not currently open, we write nothing
        // This could be because of the Logging=false setting.
        if ( !m_fLogOpened )
            return;

        WriteBytes(cstrData, cSize);
    }

    // Get the Project, Build, and Session information from the config
    // file and write as the first event record of the log file.
    void CLogWriter::WriteFileHeader(CVince* vince)
    {
        if( !m_fLoggingActive )
        {
            return;
        }

        // If this is a Unicode file, we need to add a couple of special bytes at the
        // beginning to signify it as such.
        if (m_fUnicode)
        {
// Big-Endian or Small Endian depends on platform
#ifdef _XBOX
            unsigned char special[2] = {0xfe, 0xff};
#else
            unsigned char special[2] = {0xff, 0xfe};
#endif
            Write((char*)special, 2);
        }

        const char* cstrProject = vince->pSettings->Fetch( "Project", "Unknown" );
        const char* cstrBuild = vince->pSettings->Fetch( "Build", "Unknown" );
        const char* cstrDateTime = GetCurrentDateTimeString();

        WriteEventTag("LogStart");
        WriteParameter("Project", "String", cstrProject);
        WriteParameter("Build", "String", cstrBuild);
        WriteParameter("Machine", "String", m_pLogControl->GetMachineName());
        WriteParameter("DateTime", "String", cstrDateTime);
        WriteParameter("VinceVersion", "String", VINCE_VERSION);
        WriteParameter("SequenceNumber", "int", m_pLogControl->m_sequenceNumber - 1);
        WriteEventTail();
        SAFE_DELETE_ARRAY(cstrDateTime);
    }

    // Build a string describing a parameter and write to log file. This overload is for a string
    // parameter type being passed.
    void CLogWriter::WriteParameter(const char* cstrName,  const char* cstrType, const char* cstrValue)
    {
       if( !m_fLoggingActive )
       {
          return;
       }

        m_pEventString->AppendParameterStart(cstrName, cstrType);
        m_pEventString->AppendSafe(cstrValue);
        m_pEventString->AppendParameterEnd();
    }

    // Build a string describing a parameter and write to log file. This overload is for a
    // wchar_t* parameter type being passed.
    void CLogWriter::WriteParameter(const char* cstrName,  const char* cstrType, const wchar_t* wcstrValue)
    {
       if( !m_fLoggingActive )
       {
          return;
       }

        m_pEventString->AppendParameterStart(cstrName, cstrType);
        m_pEventString->AppendSafe(wcstrValue);
        m_pEventString->AppendParameterEnd();
    }

    // Build a string describing a parameter and write to log file. This overload is for a
    // boolean parameter type being passed.
    void CLogWriter::WriteParameter(const char* cstrName,  const char* cstrType, bool bValue)
    {
       if( !m_fLoggingActive )
       {
          return;
       }

        m_pEventString->AppendParameterStart(cstrName, cstrType);
        m_pEventString->Append(bValue);
        m_pEventString->AppendParameterEnd();
    }

    // Build a string describing a parameter and write to log file. This overload is for a
    // BYTE parameter type being passed.
    void CLogWriter::WriteParameter(const char* cstrName,  const char* cstrType, BYTE bValue)
    {
       if( !m_fLoggingActive )
       {
          return;
       }

        m_pEventString->AppendParameterStart(cstrName, cstrType);
        m_pEventString->Append(bValue);
        m_pEventString->AppendParameterEnd();
    }

    // Build a string describing a parameter and write to log file. This overload is for an
    // integer parameter type being passed.
    void CLogWriter::WriteParameter(const char* cstrName,  const char* cstrType, int iValue)
    {
       if( !m_fLoggingActive )
       {
          return;
       }

        m_pEventString->AppendParameterStart(cstrName, cstrType);
        m_pEventString->Append(iValue);
        m_pEventString->AppendParameterEnd();
    }

    // Build a string describing a parameter and write to log file. This overload is for a
    // DWORD parameter type being passed.
    void CLogWriter::WriteParameter(const char* cstrName,  const char* cstrType, DWORD dwValue)
    {
       if( !m_fLoggingActive )
       {
          return;
       }

        m_pEventString->AppendParameterStart(cstrName, cstrType);
        m_pEventString->Append(dwValue);
        m_pEventString->AppendParameterEnd();
    }

    // Build a string describing a parameter and write to log file. This overload is for a
    // long parameter type being passed.
    void CLogWriter::WriteParameter(const char* cstrName,  const char* cstrType, long lValue)
    {
       if( !m_fLoggingActive )
       {
          return;
       }

        m_pEventString->AppendParameterStart(cstrName, cstrType);
        m_pEventString->Append(lValue);
        m_pEventString->AppendParameterEnd();
    }

    // Build a string describing a parameter and write to log file. This overload is for a
    // float parameter type being passed.
    void CLogWriter::WriteParameter(const char* cstrName,  const char* cstrType, float fValue)
    {
       if( !m_fLoggingActive )
       {
          return;
       }

        m_pEventString->AppendParameterStart(cstrName, cstrType);
        m_pEventString->Append(fValue);
        m_pEventString->AppendParameterEnd();
    }

    // Build a string describing a parameter and write to log file. This overload is for a
    // double parameter type being passed.
    void CLogWriter::WriteParameter(const char* cstrName,  const char* cstrType, double dValue)
    {
       if( !m_fLoggingActive )
       {
          return;
       }

        m_pEventString->AppendParameterStart(cstrName, cstrType);
        m_pEventString->Append(dValue);
        m_pEventString->AppendParameterEnd();
    }

    // Write the overall event tag for the event
    void CLogWriter::WriteEventTag(const char* cstrName)
    {
       if( !m_fLoggingActive )
       {
          return;
       }

        m_pEventString->Clear();
        m_pEventString->AppendEventTag(cstrName, m_cstrSession);
    }

    // Finish off the event tag
    void CLogWriter::WriteEventTail()
    {
       if( !m_fLoggingActive )
       {
          return;
       }

        m_pEventString->AppendEventTail();
        Write(m_pEventString->Buffer(), m_pEventString->ByteCount());
    }

    // Write an error to the log. This has a different tag (Error) than normal events.
    void CLogWriter::WriteError(const char* cstrSource, const char* cstrLocation, const char* cstrMessage)
    {
        if ( m_fLoggingActive )
        {
            m_pEventString->Clear();
            m_pEventString->AppendError(cstrSource, cstrLocation, cstrMessage);
            Write(m_pEventString->Buffer(), m_pEventString->ByteCount());
        }
    }

    // Write characters to log file
    void CLogWriter::Write(char* pBuffer, int byteCount)
    {
        // If file status in not currently open, we write nothing
        // This could be because of the Logging=false setting.

        if ( !m_fLogOpened )
            return;


        if (m_fBuffered)
        {
            // check to see if this would overflow buffer
            if ( (byteCount + m_iBufferPos + 1) >= m_iBufferSize )
            {
                FlushBuffer();
            }

            // Append new data to buffer. However, it's also possible that this single
			// write may be larger than the entire buffer
			if ((unsigned int)byteCount < m_iBufferSize)
			{
				memcpy(m_cstrOutputBuffer + m_iBufferPos, pBuffer, byteCount);
				m_iBufferPos = m_iBufferPos + byteCount;
			}
			else
			{
				// Bypass buffer and write directly
				WriteLogData(pBuffer, byteCount);
			}

            // Flush buffer if > 90% full
            if ( (m_iBufferPos / 9 ) > (m_iBufferSize / 10) )
            {
                FlushBuffer();
            }
        }
        else
        {
            // Since m_fLogOpened must be true to have gotten here, we
            // have previously opened the log successfully.
            WriteLogData(pBuffer, byteCount);
        }
    }

    // Flush contents of buffer to log file
    void CLogWriter::FlushBuffer()
    {
        if ( m_iBufferPos > 0 )
        {
            // To have gotten here, we must have previously opened the
            // log successfully.
            WriteLogData(m_cstrOutputBuffer, m_iBufferPos);
            m_iBufferPos = 0;
        }
    }

    // Compress and encrypt data as required.
    // Later, real-time streaming will also be an option.
    void CLogWriter::WriteLogData(char* srcData, int byteCount)
    {
        int srcBufSize = byteCount;
        char* compressedData = NULL;

        // Compress data in temporary buffer
        // Ignore if NO_VINCE_COMPRESSION is defined
#ifndef NO_VINCE_COMPRESSION
        if (m_pLogControl->m_bCompressed) 
        {
            TnT::ZlibCompression compression;
            
            // Get the size of the compressed data and allocate a buffer for it
            int destBufSize = compression.GetCompressionBufferSize(srcData, srcBufSize);
            compressedData = new char[destBufSize];

            // Compress the data into the buffer
            int compressedSize = compression.CompressData(srcData, srcBufSize, compressedData, destBufSize);

            // Adjust the source buffer and size for input into the encryption process
            srcBufSize = compressedSize;
            srcData = compressedData;
        }
#endif

        // Encrypt data in place
        if (m_pLogControl->m_bEncrypted && m_pEncryption != NULL)
        {
            m_pEncryption->EncryptData(srcData, srcBufSize);
        }

        // Write to file
        WriteBytes(srcData, srcBufSize);

        // If we used a compression buffer, delete it
        if(compressedData != NULL) 
        {
            delete[] compressedData;
        }
    }

    // Write raw bytes to log file. Note that the data may contain
    // zeros, so don't treat this as a null terminated string
    void CLogWriter::WriteBytes(const char* cData, int count)
    {
        // To be safe, we will assert that the file handle returned
        // is non-zero and bypass the write if it failed. Suggested
        // by prefix analysis.
        FILE* fLog = VinceFileOpen(m_LogFileName, "a+b", true);
        assert(fLog);
        if (fLog)
        {
            fwrite( cData, 1, count, fLog );
            fclose(fLog);
        }
    }
}

#endif // _VINCE_