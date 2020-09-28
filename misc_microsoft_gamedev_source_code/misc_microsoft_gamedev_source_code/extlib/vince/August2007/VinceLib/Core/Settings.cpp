//	Settings.cpp : Loads config file and provides settings
//
//	It is also possible to create or change setting later via the exposed
//  AddSetting function.
//
//	Created 2005/12/05 Rich Bonny <rbonny@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2005 Microsoft Corp.  All rights reserved.

#include "VinceUtil.h"
#include "StringUtil.h"
#include "Settings.h"

namespace Vince
{

	// Individual Setting Class
	Setting::Setting(void)
	{
		Name = NULL;
		Value = NULL;
		Next = NULL;
		NextOnList = NULL;
	}

	Setting::~Setting(void)
	{
		// Need to delete all strings
		SAFE_DELETE_ARRAY(Name);
		SAFE_DELETE_ARRAY(Value);
		// Check for attached lists and delete those items as well
		// These deletions should chain from one to the next.
		while (NULL != NextOnList)
		{
			SAFE_DELETE(NextOnList);
		}
	}

	// Settings collection class
	Settings::Settings(void)
	{
		m_pFirstSetting = NULL;
		m_pNextOnList = NULL;
		m_Loaded = false;
        m_Validate = true;
		m_strErrorBuffer[0] = '\0';
	}

	Settings::~Settings(void)
	{
		// Need to traverse linked lists
		Setting* current = m_pFirstSetting;
   		while ( NULL != current )
		{
			Setting* next = current->Next;
			delete current;
			current = next;
		}
		m_pFirstSetting = NULL;
		m_pNextOnList = NULL;
	}

	// Read Vince.ini file settings into Settings object
	bool Settings::Load(const char* cstrFileName)
	{
		// Only load config file once
		if (m_Loaded)
		{
			return true;
		}

		// Need to open file
		FILE* fConfig = NULL;
        if (0 != fopen_s(&fConfig, cstrFileName, "rt"))
        {
            fConfig = NULL;
        }
		bool bValidFile = false;
		int iLineCount = 0;
		char line[256] = "Could not open config file";

		// Parse each line of the file and look for var=value format

		if (fConfig) 
		{
			bValidFile = true;
			while ( bValidFile && !feof(fConfig) ) 
			{

				if ( NULL == fgets(line, sizeof(line)-1, fConfig) )
				{
					break;
				}

				size_t length = strlen(line);
				// trim off new line, if it is there. The fgets call should return something of
				// non-zero length, but we will check just to be safe, as suggested by Prefix.
				if ( (length > 0) && ('\n' == line[length - 1]) )
				{
					line[length - 1] = '\0';
				}
				iLineCount++;

				// Rather than temporarilly allocating memory and creating trimmed copies of the input,
				// we now trim in place to avoid memory fragmentation.
                length = TrimLine(line);

				// ignore empty lines and blanks
				if ( 0 == length)
				{
				}

				// Section headers are ignored and are for file readability only
				else if ( '[' == line[0] )
				{
				}

				// Semicolons treated as comment lines
				else if ( ';' == line[0] )
				{
				}

				else
				{
					bValidFile = AddSetting(line);
				}
			}
			fclose(fConfig);
			if ( !bValidFile )
			{
				ReportConfigFileError( iLineCount, line );
			}
		}
		else
		{
			ReportConfigFileError( 0, cstrFileName );
		}

		m_Loaded = bValidFile;
		return bValidFile;
	}

	bool Settings::AddSetting(const wchar_t* wcstrSetting)
	{
		bool fResult = false;

		if (wcstrSetting)
		{
			const char* cstrSetting = MakeSingle(wcstrSetting);
			fResult = AddSetting(cstrSetting);
			SAFE_DELETE_ARRAY(cstrSetting);
		}

        return fResult;
	}

	// Store a setting of the form "setting=value"
	// Modified to take a const char* and no longer modify the input string
	bool Settings::AddSetting(const char* cstrSetting)
	{
		bool fResult = false;
		char* strCopy = MakeCopy(cstrSetting);

		if ( NULL != cstrSetting )
		{
			const char* strSplit = SplitString( strCopy, '=' );
			if (NULL != strSplit)
			{
				strCopy[ strSplit - strCopy ] = '\0';
				SaveSetting( strCopy, strSplit + 1 );
				fResult = true;
			}
			else
			{
				// Adding an item to a list is specified using either
				// the "<" or the "+" operator
				strSplit = SplitString( strCopy, '<' );
				if (NULL == strSplit)
				{
					strSplit = SplitString( strCopy, '+' );
				}
				if (NULL != strSplit)
				{
					strCopy[ strSplit - strCopy ] = '\0';
					AppendSetting( strCopy, strSplit + 1 );
					fResult = true;
				}
			}
		}

		SAFE_DELETE_ARRAY(strCopy);
		return fResult;
	}

	const char* Settings::SplitString(const char* strInput, char delimiter)
	{
		if (NULL == strInput)
		{
			return NULL;
		}

		const char* strSplit = strchr(strInput, delimiter);
		size_t length = strlen(strInput);

		if (NULL != strSplit && (strSplit - strInput) > 0 && (strSplit - strInput) < (int) length)
		{
			return strSplit;
		}
		else
		{
			return NULL;
		}
	}

	// Return the corresponding setting from Settings object: const char* overload version
	const char* Settings::Fetch(const char* cstrSettingName, const char* cstrDefaultValue)
	{
		Setting* item = FindSetting( cstrSettingName );
		if ( NULL == item )
		{
			return cstrDefaultValue;
		}
		else
		{
			// Return the value, but also set the pointer to the next item in case this is a list
			m_pNextOnList = item->NextOnList;
			return item->Value;
		}
	}

	// FetchNext is only valid for character arrays. It is used to retrieve multiple items in a
	// list associated with a setting Name. Fetch returns the first instance and FetchNext is
	// then repeatedly called to recover additional values until a null string is returned.
	const char* Settings::FetchNext()
	{
		Setting* pNext = GetNextOnList();
		if (NULL == pNext)
		{
			return NULL;
		}
		else
		{
			return pNext->Value;
		}
	}

	// Return the corresponding setting from Settings object: DWORD overload version
	DWORD Settings::Fetch(const char* cstrSettingName, DWORD dwDefaultValue)
	{
		Setting* item = FindSetting( cstrSettingName );
		if ( NULL == item )
		{
			return dwDefaultValue;
		}
		else
		{
			// Empty string means use default value
			if ( 0 == strlen(item->Value) )
			{
				return dwDefaultValue;
			}
			DWORD dwResult = 0;
			int retValue = sscanf_s( item->Value, "%i", &dwResult );
			if ( 1 != retValue )
			{
				ReportConfigFileError( "Improper format for DWORD value", cstrSettingName);
				dwResult = dwDefaultValue;
			}
			return dwResult;
		}
	}

	// Return the corresponding setting from Settings object: int overload version
	int Settings::Fetch(const char* cstrSettingName, int intDefaultValue)
	{
		Setting* item = FindSetting( cstrSettingName );
		if ( NULL == item )
		{
			return intDefaultValue;
		}
		else
		{
			int intResult = 0;
			int retValue = sscanf_s( item->Value, "%i", &intResult );
			if ( 1 != retValue )
			{
				ReportConfigFileError("Improper format for integer value", cstrSettingName);
				intResult = intDefaultValue;
			}
			return intResult;
		}
	}

	// Return the corresponding setting from Settings object: bool overload version
	// For TicketTracker's sake, we will recognize 1=true
	bool Settings::Fetch(const char* cstrSettingName, bool bDefaultValue)
	{
		Setting* item = FindSetting( cstrSettingName );
		if ( NULL == item )
		{
			return bDefaultValue;
		}
		else
		{
			if ( 0 == _stricmp("true", item->Value) )
				return true;
			else if ( 0 == strcmp("1", item->Value) )
				return true;
			else
				return false;
		}
	}

	// Return the corresponding setting from Settings object: float overload version
	float Settings::Fetch(const char* cstrSettingName, float floatDefaultValue)
	{
		Setting* item = FindSetting( cstrSettingName );
		if ( NULL == item )
		{
			return floatDefaultValue;
		}
		else
		{
			float floatResult = 0.0f;
			int retValue = sscanf_s( item->Value, "%f", &floatResult );
			if ( 1 != retValue )
			{
				ReportConfigFileError( "Improper format for float value", cstrSettingName);
				floatResult = floatDefaultValue;
			}
			return floatResult;
		}
	}


	// Stores the setting, value pair in static arrays
	void Settings::SaveSetting(const char* cstrSettingName, const char* cstrSettingValue)
	{
		// If the setting doesn't exist, create it and stick it on the front of the list.
		// If the setting already exists, change it to the new value.
		// If the new value is null, this indicates that the setting should be deleted

		// Setting name must be a non-empty string
		const char* cstrName = MakeTrimCopy(cstrSettingName);
		const char* cstrValue = NULL;
        bool deleteCopy = false;

		if (NULL == cstrName)
		{
			return;
		}
		else
		{
			cstrValue = MakeTrimCopy(cstrSettingValue);
		}

		Setting* pItem = FindSetting( cstrName );

		if ( NULL == pItem )
		{
			// Create a new setting object, but only if a value
			// is being assigned
			if ( NULL != cstrValue )
			{
				pItem = new Setting();
				assert(pItem);
				pItem->Name = cstrName;
				pItem->Value = cstrValue;
				pItem->Next = m_pFirstSetting;
				pItem->NextOnList = NULL;
				m_pFirstSetting = pItem;
			}
		}
		else
		{
			// The named setting already exists, so get rid
			// of our new copy of the name string. Don't do it yet
            // though, because we may still need it for a validation check.
            deleteCopy = true;

			// If a value is assigned, replace the previous value
			if ( NULL != cstrValue )
			{
				SAFE_DELETE_ARRAY(pItem->Value);
				pItem->Value = cstrValue;
			}
			// otherwise, delete the item altogether
			else
			{
				DeleteSetting(pItem);
			}
		}

        // By default, we validate the setting against the acceptable list.
        // This can be disabled with "ValidateSettings=false";
        if (m_Validate)
        {
            if (!Validate(cstrName))
            {
				ReportConfigFileError( "Unrecognized configuration setting", cstrName);
            }
        }
        // Have we changed our validation setting?
        if (0 == _stricmp(cstrName, "ValidateSettings"))
        {
            m_Validate = Fetch("ValidateSettings", true);
        }
        // Are we done with the (trimmed) copy?
        if (deleteCopy)
        {
            SAFE_DELETE_ARRAY(cstrName);
        }
	}

	void Settings::DeleteSetting(Setting* pSetting)
	{
		// We will find the setting in the list,
		// delete it's components, and remove it
		Setting* pCurrent = m_pFirstSetting;
		Setting* pPrevious = NULL;
		while ( NULL != pCurrent )
		{
			if ( pCurrent == pSetting )
			{
				// Move list pointers
				if (NULL == pPrevious)
				{
					m_pFirstSetting = pCurrent->Next;
				}
				else
				{
					pPrevious->Next = pCurrent->Next;
				}
				SAFE_DELETE(pCurrent);
				pCurrent = NULL;
			}
			else
			{
				pPrevious = pCurrent;
				pCurrent = pPrevious->Next;
			}
		}
	}

	// Adds a setting to a list
	void Settings::AppendSetting(const char* cstrSettingName, const char* cstrSettingValue)
	{
		// If the setting doesn't exist, create it and stick it on the front of the list.
		// If the setting already exists add it to the end of the sub-list

		const char* cstrName = MakeTrimCopy(cstrSettingName);
		const char* cstrValue = MakeTrimCopy(cstrSettingValue);
		Setting* pItem = FindSetting( cstrName );

		// Not previously set, so just add as a primary value
		if ( NULL == pItem )
		{
			pItem = new Setting();
			assert(pItem);
			pItem->Name = cstrName;
			pItem->Value = cstrValue;
			pItem->Next = m_pFirstSetting;
			pItem->NextOnList = NULL;
			m_pFirstSetting = pItem;
		}
		// We will add to the end of the list to preserve order
		else
		{
			Setting* pSubItem = new Setting();
			assert(pSubItem);
			while (NULL != pItem->NextOnList)
			{
				pItem = pItem->NextOnList;
			}
			pItem->Name = cstrName;
			pSubItem->Value = cstrValue;
			pItem->NextOnList = pSubItem;
		}
	}

	// Locate a particular setting value by name
	Setting* Settings::FindSetting(const char* cstrSettingName)
	{
		Setting* current = m_pFirstSetting;
   		while ( NULL != current )
		{
 			if ( 0 == _stricmp(cstrSettingName, current->Name) )
			{
				return current;
			}
			current = current->Next;
		}
		return NULL;
	}

	Setting* Settings::GetNextOnList()
	{
		Setting* next = m_pNextOnList;
		if (NULL != next)
		{
			m_pNextOnList = next->NextOnList;
		}
		return next;
	}

	// Report any errors to log file
	void Settings::ReportConfigFileError( int numLine, const char* line )
	{
		// Store error message for later retrieval. Message format is a little different if file
		// could not even be opened.
		if (numLine == 0)
		{
			_snprintf_s(m_strErrorBuffer, SETTINGS_ERROR_BUFFER_SIZE, SETTINGS_ERROR_BUFFER_SIZE - 1, "Could not open config file: %s", line);
		}
		else
		{
			_snprintf_s(m_strErrorBuffer, SETTINGS_ERROR_BUFFER_SIZE, SETTINGS_ERROR_BUFFER_SIZE - 1, "Error in config file at line %d: %s", numLine, line);
		}
		m_strErrorBuffer[SETTINGS_ERROR_BUFFER_SIZE -1] = '\0';
	}

    // Overload to report validation errors
    void Settings::ReportConfigFileError( const char* cstrError, const char* cstrSetting )
    {
        _snprintf_s(m_strErrorBuffer, SETTINGS_ERROR_BUFFER_SIZE, SETTINGS_ERROR_BUFFER_SIZE - 1, "%s: %s", cstrError, cstrSetting);
		m_strErrorBuffer[SETTINGS_ERROR_BUFFER_SIZE -1] = '\0';
    }

	// Return last error message, on request
	const char* Settings::GetErrorMessage()
	{
		return m_strErrorBuffer;
	}

    // Vince-specific settings validation
	// This can be disabled, but provides a sanity check for settings
	bool Settings::Validate(const char* strSetting)
	{
        // There should be no way to call this with a null string, but we
        // check anyway to avoid a potential crash.
        if (NULL == strSetting)
        {
            return false;
        }

        // Just a brute force string compare against all possible values.
        // This only happens at initialization time, so it shouldn't present
        // any performance issues. It can be suppressed with ValidateSettings=false
        if (0 == _stricmp(strSetting, "AutoUpload")) return true;
        if (0 == _stricmp(strSetting, "BufferSize")) return true;
        if (0 == _stricmp(strSetting, "Build")) return true;
        if (0 == _stricmp(strSetting, "CompressLog")) return true;
        if (0 == _stricmp(strSetting, "DeleteLogAfterUpload")) return true;
        if (0 == _stricmp(strSetting, "EncryptLog")) return true;
        if (0 == _stricmp(strSetting, "EventOff")) return true;
        if (0 == _stricmp(strSetting, "EventOn")) return true;
        if (0 == _stricmp(strSetting, "GameTimeSaveInterval")) return true;
        if (0 == _stricmp(strSetting, "LoadIniFile")) return true;
        if (0 == _stricmp(strSetting, "LogBuffered")) return true;
        if (0 == _stricmp(strSetting, "LogFileBase")) return true;
        if (0 == _stricmp(strSetting, "LogFolder")) return true;
        if (0 == _stricmp(strSetting, "Logging")) return true;
        if (0 == _stricmp(strSetting, "Project")) return true;
        if (0 == _stricmp(strSetting, "Retail")) return true;
        if (0 == _stricmp(strSetting, "Session")) return true;
        if (0 == _stricmp(strSetting, "ThreadSafe")) return true;
        if (0 == _stricmp(strSetting, "TitleID")) return true;
        if (0 == _stricmp(strSetting, "UnicodeLog")) return true;
        if (0 == _stricmp(strSetting, "UploadaspxPage")) return true;   // Legacy setting retained for interim releases
        if (0 == _stricmp(strSetting, "UploadUrl")) return true;
        if (0 == _stricmp(strSetting, "UploadCPU")) return true;
        if (0 == _stricmp(strSetting, "UploadOnInit")) return true;
        if (0 == _stricmp(strSetting, "UploadWebServer")) return true;
        if (0 == _stricmp(strSetting, "UseLSP")) return true;
        if (0 == _stricmp(strSetting, "ValidateSettings")) return true;
        if (0 == _stricmp(strSetting, "VinceFolder")) return true;
        if (0 == _stricmp(strSetting, "WebPort")) return true;
        if (0 == _stricmp(strSetting, "WriteAsync")) return true;
        if (0 == _stricmp(strSetting, "WriteAsyncCPU")) return true;
		return false;
	}

	// Debugging code to show current list
	// This is temporary code that may be deleted at a later time.
	void Settings::DumpSettings()
	{
		char strOutputLine[SETTINGS_ERROR_BUFFER_SIZE];
		Setting* current = m_pFirstSetting;
   		while ( NULL != current )
		{
			_snprintf_s(strOutputLine, SETTINGS_ERROR_BUFFER_SIZE, SETTINGS_ERROR_BUFFER_SIZE - 1, "%s = %s\n", current->Name, current->Value);
			OutputDebugString(strOutputLine);

			Setting* next = current->NextOnList;
			while (NULL != next)
			{
				_snprintf_s(strOutputLine, SETTINGS_ERROR_BUFFER_SIZE, SETTINGS_ERROR_BUFFER_SIZE - 1, " >>> %s\n", next->Value);
				OutputDebugString(strOutputLine);
				next = next->NextOnList;
			}

			current = current->Next;
		}
		OutputDebugString("\n");
	}
}

