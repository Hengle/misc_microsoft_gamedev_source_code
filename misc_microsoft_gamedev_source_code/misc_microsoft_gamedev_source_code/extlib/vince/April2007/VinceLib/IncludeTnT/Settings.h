//	Settings.h : Loads config file and provides settings
//  All member variables are declared static since we only need one copy
//  for all classes that use these settings.
//
//	This class was adapted from the original VINCE class to reside in TnTUtil
//  and serve the needs of both VINCE and TicketTracker.
//
//	Created 2005/12/05 Rich Bonny <rbonny@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2004 Microsoft Corp.  All rights reserved.

#pragma once
#include <cassert>

// The following includes will depend on the platform

#ifdef _XBOX
	#include <xtl.h>
#else
	#include <Windows.h>
	#include <stdio.h>
#endif

#define SETTINGS_ERROR_BUFFER_SIZE 128

namespace TnT
{
	class Setting
	{
	public:
		Setting();
		~Setting();
		const char* Name;
		const char* Value;
		Setting* Next;
		Setting* NextOnList;
	};

	class Settings
	{
	public:
		Settings(void);
		~Settings(void);
		bool Load(const char* cstrFileName);
	    bool AddSetting(const wchar_t* wcstrSetting);
		bool AddSetting(const char* cstrSetting);
		const char* Fetch(const char* cstrSettingName, const char* cstrDefaultValue);
		const char* FetchNext();
		DWORD Fetch(const char* cstrSettingName, DWORD dwDefaultValue);
		int Fetch(const char* cstrSettingName, int intDefaultValue);
		bool Fetch(const char* cstrSettingName, bool bDefaultValue);
		float Fetch(const char* cstrSettingName, float floatDefaultValue);
		const char* GetErrorMessage();
		void DumpSettings();

	protected:
		const char* SplitString(const char* strInput, char delimiter);
		void SaveSetting(const char* cstrSettingName, const char* cstrSettingValue);
		void AppendSetting(const char* cstrSettingName, const char* cstrSettingValue);
		void DeleteSetting(Setting* pSetting);
		Setting* FindSetting(const char* cstrSettingName);
		Setting* GetNextOnList();

		void ReportConfigFileError( int numLine, const char* line );
		bool m_Loaded;
		Setting* m_pFirstSetting;
		Setting* m_pNextOnList;
		char m_strErrorBuffer[SETTINGS_ERROR_BUFFER_SIZE];
	};
}
