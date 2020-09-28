/////////////////////////////////////////////////////////////////////////////////////
//	EncryptionBase.h : Base class for encryption methods
//
//	Created 2006/04/19 Rich Bonny <rbonny@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2006 Microsoft Corp.  All rights reserved.
/////////////////////////////////////////////////////////////////////////////////////

#include "EncryptionBase.h"
#define GETBYTE(x, y) (unsigned int)((x)>>(8*(y)))

namespace Vince
{
	EncryptionBase::EncryptionBase() :
		m_seed(0),
        m_aByte(0),
        m_bByte(0)
	{ 
	}

	EncryptionBase::~EncryptionBase()
	{
	}

	char* EncryptionBase::InitHeader()
	{
		// Start with a seed from system time counters
		m_seed = CreateSeed();
		char* header = new char[VINCE_ENCRYPTION_HEADER_SIZE];
		for (int i = 0; i < VINCE_ENCRYPTION_HEADER_SIZE; i++)
		{
			header[i] = GetRandomByte();
		}
		// Apply algorithmic watermark
		StampWatermark(header);

		// Initialize obscuration parameters
		m_aByte = header[54];
		m_bByte = header[36];

        return(header);
	}

	unsigned int EncryptionBase::HashHeader(const char* header)
	{
		return GetHash(header, 13, 51, 6, 23, 15, 37, 60, 22);
	}

	unsigned int EncryptionBase::GetHash(const char* header,
							unsigned int b1,
							unsigned int b2,
							unsigned int b3,
							unsigned int b4,
							unsigned int b5,
							unsigned int b6,
							unsigned int b7,
							unsigned int b8)
	{
		// hacky hash to take some of the 64 byte header
		// and form an unsigned int value. Don't let this mess with
		// sign bits on the chars.
		unsigned int hash = 0;
		hash ^= (unsigned int)(unsigned char)header[b1];
		hash ^= (unsigned int)(unsigned char)header[b2];
		hash ^= ((unsigned int)(unsigned char)header[b3])<<8;
		hash ^= ((unsigned int)(unsigned char)header[b4])<<8;
		hash ^= ((unsigned int)(unsigned char)header[b5])<<16;
		hash ^= ((unsigned int)(unsigned char)header[b6])<<16;
		hash ^= ((unsigned int)(unsigned char)header[b7])<<24;
		hash ^= ((unsigned int)(unsigned char)header[b8])<<24;
		return hash;
	}

	void EncryptionBase::StampWatermark(char* header)
	{
		// We encode the algorithm type into the header, although
		// some effort is made to disguise it.
		// Different watermarks depending on algorithm, which should
		// overload the GetWatermark function
		DWORD watermark = GetWatermark();

		header[29] = header[10] ^ ( (char)(watermark & 0x000000ff) );
		header[57] = header[20] ^ ( (char)((watermark & 0x0000ff00)>>8) );
		header[45] = header[31] ^ ( (char)((watermark & 0x00ff0000)>>16) );
		header[62] = header[3] ^ ( (char)((watermark & 0xff000000)>>24) );
	}

	bool EncryptionBase::EncryptData(BYTE* byteArray, int count)
	{
		return(EncryptData((char*) byteArray, count));
	}

	// Create a random number seed to initialize the generator. If available, we
	// use the secure rand_s function. Otherwise, we use the tick count timer.
	// -- Can't get it to link with rand_s, so that will be defered for now.
	// To add a bit more entropy, we throw in the clock time
	unsigned int EncryptionBase::CreateSeed()
	{
		unsigned int dwReturn = 0;
		SYSTEMTIME sysTime;

		dwReturn = GetTickCount();
		dwReturn ^= 0x45732698;
		GetSystemTime(&sysTime);
		dwReturn ^= sysTime.wMilliseconds;
		dwReturn ^= (sysTime.wSecond << 8);
		dwReturn ^= (sysTime.wMinute << 16);
		dwReturn ^= (sysTime.wHour   << 24);
		return dwReturn;
	}

	unsigned char EncryptionBase::GetRandomByte()
	{
		// Should move these constants where they can be
		// initialized only once.
		const long  m = 2147483647L;
		const long  q = 127773L;
		const short a = 16807;
		const short r = 2836;

		long hi = m_seed/q;
		long lo = m_seed%q;

		long test = a*lo - r*hi;

		if (test > 0)
			m_seed = test;
		else
			m_seed = test+ m;

		BYTE retVal = (BYTE)(GETBYTE(m_seed, 0) ^ GETBYTE(m_seed, 1) ^ GETBYTE(m_seed, 2) ^ GETBYTE(m_seed, 3));
		return retVal;
	}

	// A quick first pass on the data mangles it to make it harder to recognize in case a brute force
	// attack guesses the key for the real encryption that follows.
	void EncryptionBase::ObscureData(char* charArray, int count)
	{
		unsigned char xByte = 0;

		for (int i = 0; i < count; i++)
		{
			xByte = charArray[i];
			charArray[i] ^= m_aByte;
			m_aByte ^= m_bByte;
			m_bByte ^= xByte;
		}
	}

	// Reverses simple obscuration
	void EncryptionBase::UnobscureData(char* charArray, int count)
	{
		unsigned char xByte = 0;

		for (int i = 0; i < count; i++)
		{
			charArray[i] ^= m_aByte;
			xByte = charArray[i];
			m_aByte ^= m_bByte;
			m_bByte ^= xByte;
		}
	}
}