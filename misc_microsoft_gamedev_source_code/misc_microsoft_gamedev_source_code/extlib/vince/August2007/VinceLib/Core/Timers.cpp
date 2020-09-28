//	Timers.cpp : Timers that report to the log file
//
//  ** Game Timer notes:
//  The only real difference between local and user timer files is the
//  file name. For local timers, we simply use the Vince path. For user
//  timers, we mount a drive using XContentCreate and use that as part
//  of the file path name. PC will always use local game timers, but they
//  are typically stored in user document folders, so are user-specific.
//
//	Created 2007/02/13 Rich Bonny <rbonny@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) Microsoft Corp.  All rights reserved.
        
#include "Timers.h"
#include "StringUtil.h"
#include "Settings.h"
#include "VinceCore.h"

namespace Vince
{
    Timers::Timers() :
        m_initialized(0),
        m_saveInterval(0)
    {
    }

    Timers::~Timers()
    {
       SAFE_DELETE_ARRAY(m_cstrBuildVersion);
    }

    HRESULT Timers::Initialize(const char* cstrBuildVersion)
    {
        HRESULT hr = S_OK;
        m_cstrBuildVersion = SAFE_COPY(cstrBuildVersion);

        // Session Timer
        m_timeSessionStart = GetTickCount();

        // Game Timer
        m_timeGameStart = 0;
        m_timeLastSaved = 0;
        m_gameTimerPaused = true;

        Settings* pSettings = VinceCore::Instance()->GetSettings();
        DWORD interval = pSettings->Fetch("GameTimeSaveInterval", 60);
        if (interval > 3600)
        {
            interval = 3600;
        }
        m_saveInterval = 1000 * interval;
        if (m_saveInterval > 0)
        {
            LoadGameTimer();
            if (m_timeGamePrevious == 0)
            {
                hr = SaveGameTimer();
            }
        }
        m_initialized = true;
        return hr;
    }

    HRESULT Timers::StartGameTimer()
    {
        if (!m_initialized)
        {
            return E_FAIL;
        }
        m_timeGameStart = GetTickCount();
        m_gameTimerPaused = false;
        return S_OK;
    }

    // We always do a save on pause, just to
    // simplify matters.
    HRESULT Timers::PauseGameTimer()
    {
        if (!m_initialized)
        {
            return E_FAIL;
        }
        m_timeGamePrevious += GetTickCount() - m_timeGameStart;
        m_gameTimerPaused = true;
        return SaveGameTimer();
    }


    // Only refresh if the game timer has
    // incremented since the last update
    // and the specified interval has elapsed.
    HRESULT Timers::RefreshGameTimer()
    {
        if (!m_initialized)
        {
            return E_FAIL;
        }
        else if (m_saveInterval == 0)
        {
            return S_OK;
        }

        HRESULT hr = S_OK;
        if (!m_gameTimerPaused)
        {
            DWORD currentTime = GetTickCount();
            if (currentTime - m_timeLastSaved > m_saveInterval)
            {                
                hr = SaveGameTimer();
                // Even if we failed, we don't want to keep trying this
                // more often than the save interval
                m_timeLastSaved = currentTime;
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

    HRESULT Timers::LoadGameTimer()
    {
        m_timeGamePrevious = 0;

        int user = FirstLoggedInUser();

        if (user < 0)
        {
            return LoadLocalGameTimer();
        }
        else
        {
            return LoadUserGameTimer(user);
        }
    }

    // See if any users are signed in. We don't care if they are
    // signed in locally or to Live, since all we want to do is
    // access their content area.
    int Timers::FirstLoggedInUser()
    {
#ifdef _XBOX
        for( UINT nUser = 0; nUser < XUSER_MAX_COUNT; nUser++ )
        {
            XUSER_SIGNIN_STATE State = XUserGetSigninState( nUser );

            if( State != eXUserSigninState_NotSignedIn )
            {
                return (int)nUser;
            }
        }
#endif
        return -1;
    }

    HRESULT Timers::LoadLocalGameTimer()
    {
        const char* cstrGameTimerSaveFile = GetFullFileName("VinceGameTimer.dat", true);
        
        HRESULT result = ReadGameTimerFile(cstrGameTimerSaveFile);
        SAFE_DELETE(cstrGameTimerSaveFile);   // To avoid leaking memory
        return result;
    }

    HRESULT Timers::ReadGameTimerFile(const char* cstrGameTimerSaveFile)
    {
        if (NULL == cstrGameTimerSaveFile)
        {
            return E_FAIL;
        }

        HANDLE hFile = CreateFile( cstrGameTimerSaveFile, GENERIC_READ,
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

    // This could fail for a number of reasons, including the
    // TitleID not being set and the timer having never been
    // saved for the current user. No big deal if it does, we
    // just start with a gametimer of 0.
    HRESULT Timers::LoadUserGameTimer(int user)
    {

#ifdef _XBOX
    
        DWORD dwErr = S_OK;

        // First we need to enumerate the content data
        XCONTENT_DATA contentData;
        HANDLE hEnum;
        DWORD cbBuffer;

        // Create enumerator for the default device
        DWORD dwRet;
        dwRet = XContentCreateEnumerator(user,
                                         XCONTENTDEVICETYPE_HDD,
                                         XCONTENTTYPE_SAVEDGAME,
                                         0,
                                         1,
                                         &cbBuffer,
                                         &hEnum );

        if( dwRet != ERROR_SUCCESS )
        {
            return dwRet;
        }


        DWORD dwReturnCount;
        DWORD cbContent = sizeof(contentData);
        dwErr = XEnumerate( hEnum, &contentData, cbContent, &dwReturnCount, NULL );
        if( dwErr == ERROR_SUCCESS )
        {
            if (dwReturnCount == 1)
            {
                // Mount the device associated with the display name for writing
                DWORD dwDisposition = 0;
                dwErr = XContentCreate( user, "Vince", &contentData,
                                    XCONTENTFLAG_OPENEXISTING, &dwDisposition, NULL, NULL);
                if( dwDisposition == XCONTENT_OPENED_EXISTING)
                {
                    dwErr = ReadGameTimerFile("Vince:\\VinceGameTimer.dat");
                }
            }
        }
        XContentClose( "Vince", NULL );
        return dwErr;
#else
        user;
        return E_FAIL;
#endif
    }

    HRESULT Timers::SaveGameTimer()
    {
        TRACE("%d .. Saving Game Timer\n", GetTickCount());
        if ( 0 == m_saveInterval )
        {
            return S_OK;
        }

        int user = FirstLoggedInUser();

        if (user >= 0)
        {
            DWORD dwResult = SaveUserGameTimer(user);
            if (dwResult == S_OK)
            {
                return S_OK;
            }
        }

        // It could be we got here because no user is logged in
        // or because the attempt to save to a user content file
        // failed. Either way, we fall back to a local save.
        return SaveLocalGameTimer();
    }

    HRESULT Timers::SaveLocalGameTimer()
    {
        const char* cstrGameTimerSaveFile = GetFullFileName("VinceGameTimer.dat", true);       
        HRESULT result = WriteGameTimerFile(cstrGameTimerSaveFile);
        SAFE_DELETE_ARRAY(cstrGameTimerSaveFile);   // To avoid leaking memory

        return result;

    }

    HRESULT Timers::WriteGameTimerFile(const char* cstrGameTimerSaveFile)
    {
        if (NULL == cstrGameTimerSaveFile)
        {
            return E_FAIL;
        }

        DWORD timeToSave = GetGameTime();

        HANDLE hFile = CreateFile( cstrGameTimerSaveFile, GENERIC_WRITE, 0,
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

    // This could fail for a number of reasons, including the
    // TitleID not being set. This also does not apply to PC
    HRESULT Timers::SaveUserGameTimer(int user)
    {
#ifdef _XBOX
        // Initialize Content Data
        XCONTENT_DATA contentData = {0};
        strcpy_s( contentData.szFileName, "VinceGameTimer.dat" );
        wcscpy_s( contentData.szDisplayName, L"VinceGameTimer" );
        contentData.dwContentType = XCONTENTTYPE_SAVEDGAME;
        contentData.DeviceID = XCONTENTDEVICETYPE_HDD;
        DWORD dwDisposition = 0;

        // Mount the device associated with the display name for writing
        DWORD dwErr = XContentCreate( user, "Vince", &contentData,
                            XCONTENTFLAG_CREATEALWAYS, &dwDisposition, NULL, NULL);
        if( dwDisposition != XCONTENT_CREATED_NEW &&
            dwDisposition != XCONTENT_OPENED_EXISTING)
        {
            XContentClose( "Vince", NULL );
            return dwErr;
        }

        const char strGameTimerSaveFile[] = "Vince:\\VinceGameTimer.dat";
        dwErr = WriteGameTimerFile(strGameTimerSaveFile);
        XContentClose( "Vince", NULL );

        return dwErr;
#else
        user;
        return E_FAIL;
#endif
    }
} // Namespace Vince
