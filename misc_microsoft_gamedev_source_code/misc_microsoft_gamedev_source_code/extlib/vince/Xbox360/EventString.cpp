//	EventString.cpp : Class to build string for output to log file
//                    Handles Unicode option for output
//
//  This is a first cut implementation and generally executes separate
//  paths for each function depending on whether we are logging Unicode or
//  not. We may implement a more elegant solution at a later time.
//
//	Created 2006/07/03 Rich Bonny <rbonny@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) Microsoft Corp.  All rights reserved.

#include "VinceControl.h"

#ifdef _VINCE_

#include "EventString.h"

namespace Vince
{
	EventString::EventString(int charSize, bool unicode)
	{
		bUnicode = unicode;
		growSize = charSize;
		maxSize = charSize;

		int startSize = charSize;
		if (unicode)
		{
			startSize *= 2;
		}
		charBuffer = new char[startSize];
		length = 0;
	}

	EventString::~EventString(void)
	{
		if (charBuffer != NULL)
		{
			delete [] charBuffer;
		}
	}

	// Clear just resets to the beginning of the buffer.
	// We might want to "un-grow" a large buffer as well.
	bool EventString::Clear()
	{
		length = 0;
		return true;
	}

	// For now, we just brute force Unicode versus non-unicode
	bool EventString::AppendEventTag(const char* cstrName, const char* cstrSession)
	{
		if (bUnicode)
		{
			Append(L"<Event Name=\"");
			Append(cstrName);
			Append(L"\" TimeStamp=\"");
			Append(GetTickCount());
			Append(L"\" Session=\"");
			Append(cstrSession);
			Append(L"\">\r\n");
		}
		else
		{
			Append("<Event Name=\"");
			Append(cstrName);
			Append("\" TimeStamp=\"");
			Append(GetTickCount());
			Append("\" Session=\"");
			Append(cstrSession);
			Append("\">\r\n");
		}

		return true;
	}

	bool EventString::AppendEventTail()
	{
		if (bUnicode)
		{
			Append(L"</Event>\r\n\r\n");
		}
		else
		{
			Append("</Event>\r\n\r\n");
		}
		return true;
	}

	bool EventString::AppendError(const char* cstrSource, const char* cstrLocation, const char* cstrMessage)
	{
		if (bUnicode)
		{
			Append(L"<Error Source=\"");
			Append(cstrSource);
			Append(L"\" Location=\"");
			Append(cstrLocation);
			Append(L"\" Message=\"");
			Append(cstrMessage);
			Append(L"\" />\r\n\r\n");
		}
		else
		{
			Append("<Error Source=\"");
			Append(cstrSource);
			Append("\" Location=\"");
			Append(cstrLocation);
			Append("\" Message=\"");
			Append(cstrMessage);
			Append("\" />\r\n\r\n");
		}
		return true;
	}

	bool EventString::AppendParameterStart(const char* cstrName,  const char* cstrType)
	{
		if (bUnicode)
		{
			Append(L"   <Parameter Name=\"");
			Append(cstrName);
			Append(L"\" Type=\"");
			Append(cstrType);
			Append(L"\" Value=\"");
		}
		else
		{
			Append("   <Parameter Name=\"");
			Append(cstrName);
			Append("\" Type=\"");
			Append(cstrType);
			Append("\" Value=\"");
		}
		return true;
	}

	bool EventString::AppendParameterEnd()
	{
		if (bUnicode)
		{
			Append(L"\" />\r\n");
		}
		else
		{
			Append("\" />\r\n");
		}
		return true;
	}

	// The overloads for "const char*" and "const wchar_t*" are the
	// workhorses of the Append functions. They either do a simple copy
	// or in some cases need to upsize or downsize the strings.
	bool EventString::Append(const char* cstrAdd)
	{
		unsigned int lengthAdd = (unsigned int)strlen(cstrAdd);
		if (lengthAdd == 0)
		{
			return false;
		}
		
		if (bUnicode)
		{
			wchar_t* wstrAddPos = GetInsertWcharPointer(lengthAdd);
			// We don't check for errors, since we should have guaranteed enough space at this
			// point and there isn't much we could do should an error occur.
			MultiByteToWideChar( CP_ACP, NULL, cstrAdd, (int)lengthAdd, wstrAddPos, (int)lengthAdd);
		}
		else
		{
			char* strAddPos = GetInsertCharPointer(lengthAdd);
			strncpy(strAddPos, cstrAdd, lengthAdd);
		}
		length += lengthAdd;
		return true;
	}

	// The following set of overloaded functions are generic utilities to
	// append formatted text to the end of a string being built.

	bool EventString::Append(const wchar_t* wcstrAdd)
	{
		unsigned int lengthAdd = (unsigned int) wcslen(wcstrAdd);
		if (lengthAdd == 0)
		{
			return false;
		}
		
		if (bUnicode)
		{
			wchar_t* wstrAddPos = GetInsertWcharPointer(lengthAdd);
			wcsncpy(wstrAddPos, wcstrAdd, lengthAdd);
		}
		else
		{
			char* strAddPos = GetInsertCharPointer(lengthAdd);
			// We don't check for errors, since we should have guaranteed enough space at this
			// point and there isn't much we could do should an error occur.
			WideCharToMultiByte( CP_ACP, NULL, wcstrAdd, (int)lengthAdd, strAddPos, (int)lengthAdd, NULL, NULL);
		}
		length += lengthAdd;
		return true;
	}

	bool EventString::AppendSafe(const char* cstrAdd)
	{
		if (NULL == cstrAdd)
		{
			return false;
		}

		size_t lengthAdd = strlen(cstrAdd);

		// Build a string containing all illegal characters
		// (Do this the first time only)

		static char strIllegal[37];
		static bool bInitialized = false;
		if (!bInitialized)
		{
			for (char i = 0; i < 32; i++)
			{
				strIllegal[i] = 31 - i;
			}
			// Note that we wound up null terminated but don't want the null
			// terminator in that position. That's ok, since we need it for
			// for the following append;
			strncat(strIllegal, "\"\'<>&" , 5);
			strIllegal[36] = '\0';
			bInitialized = true;
		}

		// Next we look for illegal characters and just spit back the input
		// if no problems were found.

		size_t pos = strcspn( cstrAdd, strIllegal );
		if ( pos == lengthAdd )
		{
			return Append(cstrAdd);
		}

		// Ok, now we have to actually replace the miserable things. To make life
		// easier (and since putting these characters in these strings was dumb in
		// the first place, we will just convert to a period.

		// first make a copy of the string
		char* strModified = new char[lengthAdd + 1];
		strModified = strncpy(strModified, cstrAdd, lengthAdd);
		strModified[lengthAdd] = '\0';

		// Now we replace illegal XML characters
		while ( pos < lengthAdd )
		{
			strModified[pos] = '.';
			pos = strcspn( strModified, strIllegal );
		}

		// Append the temporary string and then delete it
		Append(strModified);
		delete [] strModified;
		return true;
	}

	bool EventString::AppendSafe(const wchar_t* wcstrAdd)
	{
		if (NULL == wcstrAdd)
		{
			return false;
		}

		size_t lengthAdd = wcslen(wcstrAdd);

		// Build a string containing all illegal characters
		// (Do this the first time only)

		static wchar_t wstrIllegal[37];
		static bool bInitialized = false;
		if (!bInitialized)
		{
			for (char i = 0; i < 32; i++)
			{
				wstrIllegal[i] = 31 - i;
			}
			// Note that we wound up null terminated but don't want the null
			// terminator in that position. That's ok, since we need it for
			// for the following append;
			wcsncat(wstrIllegal, L"\"\'<>&" , 5);
			wstrIllegal[36] = '\0';
			bInitialized = true;
		}

		// Next we look for illegal characters and just spit back the input
		// if no problems were found.

		size_t pos = wcscspn( wcstrAdd, wstrIllegal );
		if ( pos == lengthAdd )
		{
			return Append(wcstrAdd);
		}

		// Ok, now we have to actually replace the miserable things. To make life
		// easier (and since putting these characters in these strings was dumb in
		// the first place, we will just convert to a period.

		// first make a copy of the string
		wchar_t* wstrModified = new wchar_t[lengthAdd + 1];
		wstrModified = wcsncpy(wstrModified, wcstrAdd, lengthAdd);
		wstrModified[lengthAdd] = '\0';

		// Now we replace illegal XML characters
		while ( pos < lengthAdd )
		{
			wstrModified[pos] = L'.';
			pos = wcscspn( wstrModified, wstrIllegal );
		}

		// Append the temporary string and then delete it
		Append(wstrModified);
		delete [] wstrModified;
		return true;
	}

	bool EventString::Append(bool bAdd)
	{
		if (bUnicode)
		{
			if (bAdd)
				return Append(L"1");
			else
				return Append(L"0");
		}
		else
		{
			if (bAdd)
				return Append("1");
			else
				return Append("0");
		}
	}

	bool EventString::Append(BYTE bAdd)
	{
		const unsigned int szFormat = 5;

		if (bUnicode)
		{
			char wstrFormat[szFormat];
			int iSize = _snprintf(wstrFormat, szFormat-1, "%u", bAdd);
			if (iSize > 0)
			{
				wstrFormat[iSize] = '\0';
			}
			else
			{
				wstrFormat[szFormat-1] = '\0';
			}
			return Append(wstrFormat);
		}
		else
		{
			char strFormat[szFormat];
			int iSize = _snprintf(strFormat, szFormat-1, "%u", bAdd);
			if (iSize > 0)
			{
				strFormat[iSize] = '\0';
			}
			else
			{
				strFormat[szFormat-1] = '\0';
			}
			return Append(strFormat);
		}
	}

	bool EventString::Append(int iAdd)
	{
		const unsigned szFormat = 12;

		if (bUnicode)
		{
			char wstrFormat[szFormat];
			int iSize = _snprintf(wstrFormat, szFormat-1, "%d", iAdd);
			if (iSize > 0)
			{
				wstrFormat[iSize] = '\0';
			}
			else
			{
				wstrFormat[szFormat-1] = '\0';
			}
			return Append(wstrFormat);
		}
		else
		{
			char strFormat[szFormat];
			int iSize = _snprintf(strFormat, szFormat-1, "%d", iAdd);
			if (iSize > 0)
			{
				strFormat[iSize] = '\0';
			}
			else
			{
				strFormat[szFormat-1] = '\0';
			}
			return Append(strFormat);
		}
	}

	bool EventString::Append(DWORD dwAdd)
	{
		const unsigned szFormat = 12;
		if (bUnicode)
		{
			char wstrFormat[szFormat];
			int iSize = _snprintf(wstrFormat, szFormat-1, "%u", dwAdd);
			if (iSize > 0)
			{
				wstrFormat[iSize] = '\0';
			}
			else
			{
				wstrFormat[szFormat-1] = '\0';
			}
			return Append(wstrFormat);
		}
		else
		{
			char strFormat[szFormat];
			int iSize = _snprintf(strFormat, szFormat-1, "%u", dwAdd);
			if (iSize > 0)
			{
				strFormat[iSize] = '\0';
			}
			else
			{
				strFormat[szFormat-1] = '\0';
			}
			return Append(strFormat);
		}
	}

	bool EventString::Append(long lAdd)
	{
		const unsigned szFormat = 12;
		if (bUnicode)
		{
			char wstrFormat[szFormat];
			int iSize = _snprintf(wstrFormat, szFormat-1, "%d", lAdd);
			if (iSize > 0)
			{
				wstrFormat[iSize] = '\0';
			}
			else
			{
				wstrFormat[szFormat-1] = '\0';
			}
			return Append(wstrFormat);
		}
		else
		{
			char strFormat[szFormat];
			int iSize = _snprintf(strFormat, szFormat-1, "%d", lAdd);
			if (iSize > 0)
			{
				strFormat[iSize] = '\0';
			}
			else
			{
				strFormat[szFormat-1] = '\0';
			}
			return Append(strFormat);
		}
	}

	bool EventString::Append(float fAdd)
	{
		const unsigned szFormat = 16;
		if (bUnicode)
		{
			char wstrFormat[szFormat];
			int iSize = _snprintf(wstrFormat, szFormat-1, "%1.7G", fAdd);
			if (iSize > 0)
			{
				wstrFormat[iSize] = '\0';
			}
			else
			{
				wstrFormat[szFormat-1] = '\0';
			}
			return Append(wstrFormat);
		}
		else
		{
			char strFormat[szFormat];
			int iSize = _snprintf(strFormat, szFormat-1, "%1.7G", fAdd);
			if (iSize > 0)
			{
				strFormat[iSize] = '\0';
			}
			else
			{
				strFormat[szFormat-1] = '\0';
			}
			return Append(strFormat);
		}
	}

	bool EventString::Append(double dAdd)
	{
		const unsigned szFormat = 25;
		if (bUnicode)
		{
			char wstrFormat[szFormat];
			int iSize = _snprintf(wstrFormat, szFormat-1, "%1.16G", dAdd);
			if (iSize > 0)
			{
				wstrFormat[iSize] = '\0';
			}
			else
			{
				wstrFormat[szFormat-1] = '\0';
			}
			return Append(wstrFormat);
		}
		else
		{
			char strFormat[szFormat];
			int iSize = _snprintf(strFormat, szFormat-1, "%1.16G", dAdd);
			if (iSize > 0)
			{
				strFormat[iSize] = '\0';
			}
			else
			{
				strFormat[szFormat-1] = '\0';
			}
			return Append(strFormat);
		}
	}

	char* EventString::Buffer()
	{
		return charBuffer;
	}

	int EventString::ByteCount()
	{
		return (bUnicode ? 2*length : length);
	}

	// Get inserting point into the unicode buffer.
	// First checks to see if it is big enough and resizes if not.
	wchar_t* EventString::GetInsertWcharPointer(size_t lengthAdd)
	{
		if ( (lengthAdd + length) > maxSize )
		{
			GrowBuffer(lengthAdd);
		}
		return (wchar_t*)(charBuffer + 2*length);
	}

	// Get inserting point into the single byte buffer.
	// First checks to see if it is big enough and resizes if not.
	char* EventString::GetInsertCharPointer(size_t lengthAdd)
	{
		if ( (lengthAdd + length) > maxSize )
		{
			GrowBuffer(lengthAdd);
		}
		return charBuffer + length;
	}

	void EventString::GrowBuffer(size_t lengthAdd)
	{
		unsigned int newSize = maxSize;
		while ( (length + lengthAdd) > newSize )
		{
			newSize += growSize;
		}

		// we never should have been called unless growth is needed, but we check anyway
		if (newSize > maxSize)
		{
			unsigned int copySize = length;
			unsigned int byteSize = newSize;
			if (bUnicode)
			{
				copySize *= 2;
				byteSize *= 2;
			}
			char* newBuffer = new char[byteSize];
			memcpy(newBuffer, charBuffer, copySize);
			delete [] charBuffer;
			charBuffer = newBuffer;
			maxSize = newSize;
		}
	}

}	// namespace

#endif	// _VINCE_