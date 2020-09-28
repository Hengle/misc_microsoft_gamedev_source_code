//	StringUtil.cpp : Utility routines for VINCE string manipulation
//
//	Created 2007/02/14 Rich Bonny <rbonny@microsoft.com>
//
//  Migrated from TnTUtil of TnTLib
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) Microsoft Corp.  All rights reserved.

#include "StringUtil.h"

#ifndef _XBOX
#include <stdlib.h>
#endif

#include <cassert>

namespace Vince
{
	// Copy a string to a new string allocation
	char* MakeCopy( const char* cstrInput )
	{
		if ( NULL == cstrInput )
		{
			return NULL;
		}

		size_t length = strlen(cstrInput);
		char* strOutput = new char[ length + 1 ];

		// It is unlikely that the allocation failed, but to appease
		// prefix, we check for a NULL return
		if (strOutput != NULL)
		{
			strncpy_s(strOutput, length + 1, cstrInput, length);
			strOutput[length] = '\0';
		}
		return strOutput;
	}

	// Remove leading and trailing whitespace in place.
	// Returns the length of the trimmed character array.
	size_t TrimLine( char* strInput )
	{
		if ( NULL == strInput )
		{
			return 0;
		}

		size_t length = strlen(strInput);

		// If length is 0, we are done
		if ( 0 == length )
		{
			return 0;
		}

		size_t first = 0;
		size_t last = length - 1;

		// Trim leading white space
		while ( (first < length) && (strInput[first] == ' ' || strInput[first] == '\t') )
		{
			first++;
		}

		// Trim trailing white space
		while ( last > 0 && (strInput[last] == ' ' || strInput[last] == '\t') )
		{
			last--;
		}

		if (first > last)
		{
			strInput[0] = '\0';
			return 0;
		}
		else
		{
			length = last - first + 1;
			if (first > 0)
			{
				for (size_t i = 0; i < length; i++)
				{
					strInput[i] = strInput[i + first];
				}
			}
			strInput[length] = '\0';
			return length;
		}
	}


	// Copy a string to a new string allocation
	// but strip leading or trailing whitespace
	char* MakeTrimCopy( const char* cstrInput )
	{
		if ( NULL == cstrInput )
		{
			return NULL;
		}

		size_t length = strlen(cstrInput);

		// For zero length string, we return NULL
		if ( 0 == length )
		{
			return NULL;
		}

		size_t first = 0;
		size_t last = length - 1;

		// Trim leading white space
		while ( (first < length) && (cstrInput[first] == ' ' || cstrInput[first] == '\t') )
		{
			first++;
		}

		// Trim trailing white space
		while ( last > 0 && (cstrInput[last] == ' ' || cstrInput[last] == '\t') )
		{
			last--;
		}

		if (first > last)
		{
			return NULL;
		}
		else
		{
			length = last - first + 1;
			char* strOutput = new char[ length + 1 ];
			assert(strOutput != NULL);
			strncpy_s(strOutput, length + 1, cstrInput+first, length);
			strOutput[length] = '\0';
			return strOutput;
		}
	}

	wchar_t* MakeCopy( const wchar_t* wcstrInput )
	{
		if ( NULL == wcstrInput )
		{
			return NULL;
		}

		size_t length = wcslen(wcstrInput);
		wchar_t* wcstrOutput = new wchar_t[ length + 1];
		if (wcstrOutput != NULL)
		{
			wcsncpy_s(wcstrOutput, length + 1, wcstrInput, length);
			wcstrOutput[length] = '\0';
		}
		return wcstrOutput;
	}

	// This function makes sure a char string is safe to include within
	// a quoted XML field.
	const char* MakeSafeXMLchars( const char* cstrInput )
	{
		if (NULL == cstrInput)
			return NULL;

		size_t length = strlen(cstrInput);

		// First determine if any control characters are in the string
		
		bool fControlChars = false;
		for (size_t i = 0; i < length; i++)
		{
			if ( (cstrInput[i] > 0) && (cstrInput[i] < 32) )
			{
				fControlChars = true;
				break;
			}
		}

		// Next we look for illegal characters and just spit back the input
		// if no problems were found.

		const char illegal[] = "\"\'<>&";

		size_t pos = strcspn( cstrInput, illegal );

		if ( !fControlChars && (pos == length) )
			return cstrInput;

		// Ok, now we have to actually replace the miserable things. To make life
		// easier (and since putting these characters in these strings was dumb in
		// the first place, we will just convert to a period.

		// first make a copy of the string
		char* strModified = new char[length + 1];
		strncpy_s(strModified, length + 1, cstrInput, length);
		strModified[length] = '\0';

		// Now we replace and control characters
		for (size_t i = 0; i < length; i++)
		{
			if ( cstrInput[i] < 32 )
			{
				strModified[i] = '.';
			}
		}

		// and now we replace illegal XML characters
		while ( pos < length )
		{
			strModified[pos] = '.';
			pos = strcspn( strModified, illegal );
		}

		// To avoid leaking memory, the caller should check to see if the char
		// pointer is the same as the original and, if not, delete the temp copy.
		return strModified;
	}

	// Helper function to convert text strings back to single width
	char* MakeSingle(const WCHAR* wcstrSource)
	{
		char* strReturn = NULL;
		if ( NULL == wcstrSource )
		{
			return NULL;
		}

		size_t iLength = wcslen(wcstrSource);
		strReturn = new char[iLength + 1];

		// Should return iSize = iLength + 1 since this includes null terminator
		size_t iSize = WideCharToMultiByte( CP_ACP, NULL, wcstrSource, (int) iLength + 1, strReturn, (int) iLength + 1, NULL, NULL);
		if (iSize != iLength + 1)
		{
			// Wrapped around if statement to avoid compiler warning
			assert( iSize == iLength + 1);
		}
		strReturn[iLength] = '\0';
		return strReturn;
	}

	//-----------------------------------------------------------------------------
	// Copied From: XBUtil_GetWide()
	// Installed locally to reduce platform dependency.
	//
	// Desc: Convert CHAR string to WCHAR string. dwMax includes the null byte.
	//       Never copies more than dwMax-1 characters into strWide.
	//          Ex: CopyWide( "abc", strWide, 3 ) gives strWide = L"ab"
	//-----------------------------------------------------------------------------
	void CopyWide( const CHAR* strThin, WCHAR* strWide, size_t sizeMax )
	{
		// Check for null input string
		if ( strThin == NULL )
		{
			strWide = NULL;
			return;
		}

		// dwMax includes the null bytes, so must always be at least one.
		// Furthermore, if sizeMax is 0, MultiByteToWideChar doesn't do any 
		// conversion, but returns the number of chars it *would* convert.
		assert( sizeMax > 0 );

		// Determine how many characters we will convert. Can never be more
		// than dwMax-1
		size_t nChars = strlen( strThin );
		if( nChars > sizeMax - 1 )
			nChars = sizeMax - 1;

		// Perform the conversion. This will crash on zero length strings
		assert( nChars > 0 );
		size_t nWide = MultiByteToWideChar( CP_ACP, 0, strThin, (int) nChars, 
										strWide, (int) sizeMax );
		// This behaves differently on Windows and Xbox. Need to figure out why.
//		assert( nChars == nWide );
		strWide[nChars] = 0;	// why is this necessary?
		(VOID)nWide; // avoid compiler warning in release mode
	}

//	Helper function to create a wide character version of a char string array
	wchar_t* MakeWide(const char* cstrInput)
	{
		wchar_t* wcsOutput = NULL;
		if (cstrInput)
		{
			size_t length = strlen(cstrInput);
			wcsOutput = new wchar_t[length+1];
			if (length > 0)
			{
				CopyWide(cstrInput, wcsOutput, length + 1);
			}
			wcsOutput[length] = L'\0';
		}
		return wcsOutput;
	}

} // namespace Vince