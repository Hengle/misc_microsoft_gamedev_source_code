//	Timers.h : Timers that report to the log file
//
//	Template Created 2007/02/13 Rich Bonny <rbonny@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) Microsoft Corp.  All rights reserved.
        
#pragma once

namespace Vince
{
    class Timers
    {
    public:
		~Timers();
        HRESULT Initialize(const char* cstrBuildVersion);
        DWORD   GetSessionTime();
        HRESULT StartGameTimer();
        HRESULT PauseGameTimer();
        HRESULT RefreshGameTimer();
        DWORD   GetGameTime();

    protected:
        HRESULT LoadGameTimer();
        HRESULT SaveGameTimer();

    private:
        DWORD m_timeGamePrevious;
        DWORD m_timeLastSaved;
        DWORD m_timeGameStart;
        DWORD m_timeSessionStart;
        bool  m_gameTimerPaused;
        const char* m_cstrBuildVersion;
        const char* m_cstrGameTimerSaveFile;
    };
} // Namespace Timers
