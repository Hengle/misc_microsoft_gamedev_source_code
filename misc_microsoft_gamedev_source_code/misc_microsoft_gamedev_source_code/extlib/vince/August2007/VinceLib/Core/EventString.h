//	EventString.h : Class to build string for output to log file
//                  Handles Unicode option for output
//
//	Created 2006/07/03 Rich Bonny <rbonny@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) Microsoft Corp.  All rights reserved.

#pragma once

#ifdef _XBOX
  #include <xtl.h>
  #include <cstdio>
#else
  #include <Windows.h>
  #include <stdio.h>
#endif

namespace Vince
{
	class EventString
	{
	public:
		EventString(int charSize, bool unicode);
		~EventString(void);
		bool Clear();
		bool AppendEventTag(const char* cstrName, const char* cstrSession);
		bool AppendEventTail();
		bool AppendError(const char* cstrSource, const char* cstrLocation, const char* cstrMessage);
	    bool AppendError(const wchar_t* wcsSource, const wchar_t* wcsLocation, const wchar_t* wcsMessage);
		bool AppendParameterStart(const char* cstrName, const char* cstrType);
		bool AppendParameterEnd();
		bool Append(const char* cstrAdd);
		bool Append(const wchar_t* wcstrAdd);
		bool AppendSafe(const char* cstrAdd);
		bool AppendSafe(const wchar_t* wcstrAdd);
		bool Append(bool bAdd);
		bool Append(BYTE bAdd);
		bool Append(int iAdd);
		bool Append(DWORD dwAdd);
		bool Append(long lAdd);
		bool Append(float fAdd);
		bool Append(double dAdd);
		char* Buffer();
		int ByteCount();

	protected:
		bool m_unicode;
		unsigned int m_length;	// these are all in character units, not bytes
		unsigned int m_maxSize;
		unsigned int m_growSize;
		char* m_charBuffer;

		wchar_t* GetInsertWcharPointer(unsigned int lengthAdd);
		char* GetInsertCharPointer(unsigned int lengthAdd);
		void GrowBuffer(size_t lengthAdd);
	};
}	// namespace
