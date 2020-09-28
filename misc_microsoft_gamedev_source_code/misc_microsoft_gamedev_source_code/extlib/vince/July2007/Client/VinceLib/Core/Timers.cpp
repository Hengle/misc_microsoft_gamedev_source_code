//	Timers.cpp : Timers that report to the log file
//
//	Template Created 2007/02/13 Rich Bonny <rbonny@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) Microsoft Corp.  All rights reserved.
        
#ifdef _XBOX
  #include <xtl.h>
  #define TIMER_SAVEFILE "save:VinceGameTimer.dat"
#else
  #include <Windows.h>
  #define TIMER_SAVEFILE "VinceGameTimer.dat"
#endif

#include "Timers.h"
#include "VinceUtil.h"
#include "StringUtil.h"

namespace Vince
{
    Timers::~Timers()
    {
       SAFE_DELETE_ARRAY(m_cstrBuildVersion);
       SAFE_DELETE_ARRAY(m_cstrGameTimerSaveFile);
    }

    HRESULT Timers::Initialize(const char* cstrBuildVersion)
    {
        HRESULT hr = S_OK;
        m_cstrBuildVersion = SAFE_COPY(cstrBuildVersion);

        // We may want to do this differently for Xbox by placing the file in
        // the user content area of the disk, but since this involves logging
        // in to a profile, this adds an additional requirement not previously
        // imposed on VINCE integrations, so we need to review that strategy.
        // We may do something different for Retail versus test deployments.
        m_cstrGameTimerSaveFile = GetFullFileName("VinceGameTimer.dat", true);

        // Session Timer
        m_timeSessionStart = GetTickCount();

        // Game Timer
        m_timeGameStart = 0;
        m_timeLastSaved = 0;
        m_gameTimerPaused = true;
        LoadGameTimer();
        if (m_timeGamePrevious == 0)
        {
            hr = SaveGameTimer();
        }
        return hr;
    }

    HRESULT Timers::StartGameTimer()
    {
        m_timeGameStart = GetTickCount();
        m_gameTimerPaused = false;
        return S_OK;
    }

    // We always do a save on pause, just to
    // simplify matters.
    HRESULT Timers::PauseGameTimer()
    {
        m_timeGamePrevious += GetTickCount() - m_timeGameStart;
        m_gameTimerPaused = true;
        return SaveGameTimer();
    }


    // Only refresh if the game timer has
    // incremented since the last update
    // and it has been at least 60 seconds
    // since the last update.
    HRESULT Timers::RefreshGameTimer()
    {
        HRESULT hr = S_OK;
        if (!m_gameTimerPaused)
        {
            if (GetTickCount() - m_timeLastSaved > 1000)
            {                
                hr = SaveGameTimer();
            }
        }
        return hr;
    }

    DWORD Timers::GetGameTime()
    {
        DWORD timeReturn = m_timeGamePrevious;
        if (!m_gameTimerPaused)
        {
            timeReturn += GetTickCount() - m_timeGameStart;
        }
        return timeReturn;
    }

    DWORD Timers::GetSessionTime()
    {
        return GetTickCount() - m_timeSessionStart;
    }

    // Differs slightly for Xbox versus PC, but
    // both wind up reading data from file.
    // Still need to match build values.
    HRESULT Timers::LoadGameTimer()
    {
        m_timeGamePrevious = 0;

        HANDLE hFile = CreateFile( m_cstrGameTimerSaveFile, GENERIC_READ,
            FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );

        if( hFile != INVALID_HANDLE_VALUE )
        {
            // Read the time and build value

            CHAR szBuffer[32];
            DWORD dwRead;

            if( ReadFile( hFile, (VOID*)szBuffer, ARRAYSIZE(szBuffer), &dwRead, NULL) == 0 )
            {
                CloseHandle( hFile );
                return E_FAIL;
            }

            // Extract information. If build ID does not match
            // the timer winds up getting reset.
            if (0 == _stricmp(szBuffer+4, m_cstrBuildVersion))
            {
                memcpy((void*)(&m_timeGamePrevious), szBuffer, 4);
            }
            CloseHandle( hFile );
        }
        else
        {
            return E_FAIL;
        }

        return S_OK;
    }

    // Differs slightly for Xbox versus PC, but
    // both wind up writing data to file.
    HRESULT Timers::SaveGameTimer()
    {
        m_timeLastSaved = GetTickCount();
        DWORD timeToSave = GetGameTime();

        HANDLE hFile = CreateFile( m_cstrGameTimerSaveFile, GENERIC_WRITE, 0,
            NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );

        if( hFile != INVALID_HANDLE_VALUE )
        {
            // Read the time and build value

            CHAR szBuffer[32];
            DWORD dwWritten;
            memcpy(szBuffer, &timeToSave, 4);
            size_t length = strlen(m_cstrBuildVersion);
            size_t copyLength = length > 28 ? 28 : length;
            strncpy_s(szBuffer + 4, 28, m_cstrBuildVersion, copyLength);

            if( WriteFile( hFile, (VOID*)szBuffer, 32, &dwWritten, NULL ) == 0)
            {
                CloseHandle( hFile );
                return E_FAIL;
            }
            CloseHandle( hFile );
        }
        else
        {
            return E_FAIL;
        }

        return S_OK;
    }

} // Namespace Timers
