//	Timers.h : Timers that report to the log file
//
//	Template Created 2007/02/13 Rich Bonny <rbonny@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) Microsoft Corp.  All rights reserved.
        
#pragma once
#include "VinceUtil.h"

namespace Vince
{
    class Timers
    {
    public:
		Timers();
		~Timers();
        HRESULT Initialize(const char* cstrBuildVersion);
        DWORD   GetSessionTime();
        HRESULT StartGameTimer();
        HRESULT PauseGameTimer();
        HRESULT RefreshGameTimer();
        DWORD   GetGameTime();

    protected:
        int FirstLoggedInUser();
        HRESULT LoadGameTimer();
        HRESULT LoadLocalGameTimer();
        HRESULT LoadUserGameTimer(int user);
        HRESULT ReadGameTimerFile(const char* cstrGameTimerSaveFile);
        HRESULT SaveGameTimer();
        HRESULT SaveLocalGameTimer();
        HRESULT SaveUserGameTimer(int user);
        HRESULT WriteGameTimerFile(const char* cstrGameTimerSaveFile);

    private:
        DWORD m_timeGamePrevious;
        DWORD m_timeLastSaved;
        DWORD m_timeGameStart;
        DWORD m_timeSessionStart;
        DWORD m_saveInterval;
        bool  m_gameTimerPaused;
        bool  m_initialized;
        const char* m_cstrBuildVersion;
    };
} // Namespace Timers
