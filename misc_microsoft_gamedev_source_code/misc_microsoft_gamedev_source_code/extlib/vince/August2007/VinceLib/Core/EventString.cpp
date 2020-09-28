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

#include "EventString.h"
#include "VinceCore.h"

namespace Vince
{
	EventString::EventString(int charSize, bool unicode)
	{
		m_unicode = unicode;
		m_growSize = charSize;
		m_maxSize = charSize;

		int startSize = charSize;
		if (unicode)
		{
			startSize *= 2;
		}
		m_charBuffer = new char[startSize];
		m_length = 0;
	}

	EventString::~EventString(void)
	{
		if (m_charBuffer != NULL)
		{
			delete [] m_charBuffer;
		}
	}

	// Clear just resets to the beginning of the buffer.
	// We might want to "un-grow" a large buffer as well.
	bool EventString::Clear()
	{
		m_length = 0;
		return true;
	}

	// For now, we just brute force Unicode versus non-unicode
	bool EventString::AppendEventTag(const char* cstrName, const char* cstrSession)
	{
        DWORD sessionTime = VinceCore::Instance()->GetSessionTime();
        DWORD gameTime    = VinceCore::Instance()->GetGameTime();

		if (m_unicode)
		{
			Append(L"<Event Name=\"");
			Append(cstrName);
			Append(L"\" SessionTime=\"");
            Append(sessionTime);
			Append(L"\" GameTime=\"");
			Append(gameTime);
			Append(L"\" Session=\"");
			Append(cstrSession);
			Append(L"\">\r\n");
		}
		else
		{
			Append("<Event Name=\"");
			Append(cstrName);
			Append("\" SessionTime=\"");
			Append(sessionTime);
			Append("\" GameTime=\"");
			Append(gameTime);
			Append("\" Session=\"");
			Append(cstrSession);
			Append("\">\r\n");
		}

		return true;
	}

	bool EventString::AppendEventTail()
	{
		if (m_unicode)
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
		if (m_unicode)
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

	bool EventString::AppendError(const wchar_t* wcsSource, const wchar_t* wcsLocation, const wchar_t* wcsMessage)
	{
		if (m_unicode)
		{
			Append(L"<Error Source=\"");
			Append(wcsSource);
			Append(L"\" Location=\"");
			Append(wcsLocation);
			Append(L"\" Message=\"");
			Append(wcsMessage);
			Append(L"\" />\r\n\r\n");
		}
		else
		{
			Append("<Error Source=\"");
			Append(wcsSource);
			Append("\" Location=\"");
			Append(wcsLocation);
			Append("\" Message=\"");
			Append(wcsMessage);
			Append("\" />\r\n\r\n");
		}
		return true;
	}

	bool EventString::AppendParameterStart(const char* cstrName,  const char* cstrType)
	{
		if (m_unicode)
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
		if (m_unicode)
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
		
		if (m_unicode)
		{
			wchar_t* wstrAddPos = GetInsertWcharPointer(lengthAdd);
			// We don't check for errors, since we should have guaranteed enough space at this
			// point and there isn't much we could do should an error occur.
			MultiByteToWideChar( CP_ACP, NULL, cstrAdd, (int)lengthAdd, wstrAddPos, (int)lengthAdd);
		}
		else
		{
			char* strAddPos = GetInsertCharPointer(lengthAdd);
            // To use secure string function, we need to calculate the space remaining
            // in the char buffer. It should always be greater than lengthAdd.
            int lengthRemaining = m_maxSize - m_length;
			strncpy_s(strAddPos, lengthRemaining, cstrAdd, lengthAdd);
		}
		m_length += lengthAdd;
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
		
		if (m_unicode)
		{
			wchar_t* wstrAddPos = GetInsertWcharPointer(lengthAdd);
            // To use secure string function, we need to calculate the space remaining
            // in the char buffer (this is in chars, not bytes)
            int lengthRemaining = m_maxSize - m_length;
			wcsncpy_s(wstrAddPos, lengthRemaining, wcstrAdd, lengthAdd);
		}
		else
		{
			char* strAddPos = GetInsertCharPointer(lengthAdd);
			// We don't check for errors, since we should have guaranteed enough space at this
			// point and there isn't much we could do should an error occur.
			WideCharToMultiByte( CP_ACP, NULL, wcstrAdd, (int)lengthAdd, strAddPos, (int)lengthAdd, NULL, NULL);
		}
		m_length += lengthAdd;
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

        const int ILLEGAL_CHAR_COUNT = 37;
		static char strIllegal[ILLEGAL_CHAR_COUNT];
		static bool bInitialized = false;
		if (!bInitialized)
		{
			for (char i = 0; i < 32; i++)
			{
				strIllegal[i] = 31 - i;
			}
			// Note that we wound up null terminated but don't want the null
			// terminator in that position. That's ok, since we need it
			// for the following append;
			strncat_s(strIllegal, ILLEGAL_CHAR_COUNT,  "\"\'<>&" , 5);
			strIllegal[ILLEGAL_CHAR_COUNT - 1] = '\0';
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
		if (0 != strncpy_s(strModified, lengthAdd + 1, cstrAdd, lengthAdd))
        {
            return false;
        }
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

        const int ILLEGAL_CHAR_COUNT = 37;
		static wchar_t wstrIllegal[ILLEGAL_CHAR_COUNT];
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
			wcsncat_s(wstrIllegal, ILLEGAL_CHAR_COUNT,  L"\"\'<>&" , 5);
			wstrIllegal[ILLEGAL_CHAR_COUNT - 1] = '\0';
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
		if (0 != wcsncpy_s(wstrModified, lengthAdd + 1, wcstrAdd, lengthAdd))
        {
            return false;
        }
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
		if (m_unicode)
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

		char strFormat[szFormat];
		int iSize = _snprintf_s(strFormat, szFormat, szFormat-1, "%u", bAdd);
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

	bool EventString::Append(int iAdd)
	{
		const unsigned szFormat = 12;

		char strFormat[szFormat];
		int iSize = _snprintf_s(strFormat, szFormat, szFormat-1, "%d", iAdd);
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

	bool EventString::Append(DWORD dwAdd)
	{
		const unsigned szFormat = 12;

		char strFormat[szFormat];
		int iSize = _snprintf_s(strFormat, szFormat, szFormat-1, "%u", dwAdd);
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

	bool EventString::Append(long lAdd)
	{
		const unsigned szFormat = 12;

		char strFormat[szFormat];
		int iSize = _snprintf_s(strFormat, szFormat, szFormat-1, "%d", lAdd);
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

	bool EventString::Append(float fAdd)
	{
		const unsigned szFormat = 16;

		char strFormat[szFormat];
		int iSize = _snprintf_s(strFormat, szFormat, szFormat-1, "%1.7G", fAdd);
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

	bool EventString::Append(double dAdd)
	{
		const unsigned szFormat = 25;

		char strFormat[szFormat];
		int iSize = _snprintf_s(strFormat, szFormat, szFormat-1, "%1.16G", dAdd);
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

	char* EventString::Buffer()
	{
		return m_charBuffer;
	}

	int EventString::ByteCount()
	{
		return (m_unicode ? 2*m_length : m_length);
	}

	// Get inserting point into the unicode buffer.
	// First checks to see if it is big enough and resizes if not.
    // Note that we want to grow even if it matches max size, since
    // we might need an extra character for null terminator.
	wchar_t* EventString::GetInsertWcharPointer(size_t lengthAdd)
	{
		if ( (lengthAdd + m_length) >= m_maxSize )
		{
			GrowBuffer(lengthAdd);
		}
		return (wchar_t*)(m_charBuffer + 2*m_length);
	}

	// Get inserting point into the single byte buffer.
	// First checks to see if it is big enough and resizes if not.
	char* EventString::GetInsertCharPointer(size_t lengthAdd)
	{
		if ( (lengthAdd + m_length) >= m_maxSize )
		{
			GrowBuffer(lengthAdd);
		}
		return m_charBuffer + m_length;
	}

	void EventString::GrowBuffer(size_t lengthAdd)
	{
		unsigned int newSize = m_maxSize;
		while ( (m_length + lengthAdd) >= newSize )
		{
			newSize += m_growSize;
		}

		// we never should have been called unless growth is needed, but we check anyway
		if (newSize > m_maxSize)
		{
			unsigned int copySize = m_length;
			unsigned int byteSize = newSize;
			if (m_unicode)
			{
				copySize *= 2;
				byteSize *= 2;
			}
			char* newBuffer = new char[byteSize];
			memcpy(newBuffer, m_charBuffer, copySize);
			delete [] m_charBuffer;
			m_charBuffer = newBuffer;
			m_maxSize = newSize;
		}
	}

}	// namespace