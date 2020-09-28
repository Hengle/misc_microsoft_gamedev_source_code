/////////////////////////////////////////////////////////////////////////////////////
//	XboxEncryption.h : Version of encryption methods for use on Xbox 360
//
//	Created 2006/04/19 Rich Bonny <rbonny@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2006 Microsoft Corp.  All rights reserved.
/////////////////////////////////////////////////////////////////////////////////////

#include "XboxEncryption.h"

namespace Vince
{
    XboxEncryption::XboxEncryption()
    {
    }

    XboxEncryption::~XboxEncryption()
    {
    }

    bool XboxEncryption::Initialize()
    {
        return(true);
    }

	unsigned int XboxEncryption::CreateHeader(char** header)
	{
		// File initialization generates a random 64 byte header which has
		// different uses depending on the encryption method. It always
		// contains a watermark which identifies which algorithm is being
		// used. If CryptoAPI is being used, we will also append an
		// encrypted session key. A starting seed for the random number
		// generator is hashed for streaming encryption on Xbox 360. The
		// header is returned so that it can be written to the file and passed
		// to the decryption utility. Each new log file must re-Initialize.

		char* baseHeader = InitHeader();

		// Get Random number seed from header
		m_seed = HashHeader(baseHeader);

		// And skip through up to 256 initial entries
		// based on one of the header values
		unsigned int skip = (unsigned char)(baseHeader[33]);
		for ( unsigned int i = 0; i < skip; i++ )
		{
			GetRandomByte();
		}
		*header = baseHeader;
		return VINCE_ENCRYPTION_HEADER_SIZE;
	}

	bool XboxEncryption::EncryptData(char* charArray, int count)
	{
		for (int i = 0; i < count; i++)
		{
			charArray[i] ^= GetRandomByte();
		}
        return(true);
	}
}