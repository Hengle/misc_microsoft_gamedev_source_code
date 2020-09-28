/////////////////////////////////////////////////////////////////////////////////////
//	CryptoAPIEncryption.cpp : Version of encryption methods for use on Windows
//                            using CryptoAPI functions
//
//	Created 2006/04/19 Rich Bonny <rbonny@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2006 Microsoft Corp.  All rights reserved.
/////////////////////////////////////////////////////////////////////////////////////

#ifndef _XBOX       // PC only

#include "CryptoAPIEncryption.h"
#include <wincrypt.h>

#define MY_ENCODING_TYPE  (PKCS_7_ASN_ENCODING | X509_ASN_ENCODING)
#define KEYLENGTH  0x00800000
#define ENCRYPT_ALGORITHM CALG_RC4 
#define ENCRYPT_BLOCK_SIZE 8 

namespace Vince
{
	CryptoAPIEncryption::CryptoAPIEncryption() :
		m_hCryptProv(NULL), 
		m_hPublicKey(NULL),
		m_hSessionKey(NULL)
	{
        memset(m_bSalt, 0, sizeof(m_bSalt));
	}

	CryptoAPIEncryption::~CryptoAPIEncryption()
	{
		// Attempt to free resources, Don't bother checking return codes,
		// since there is little we could do about it.
		if (m_hPublicKey)
		{
			CryptDestroyKey(m_hPublicKey);
			m_hPublicKey = NULL;
		}
		if (m_hSessionKey)
		{
			CryptDestroyKey(m_hSessionKey);
			m_hSessionKey = NULL;
		}
		if (m_hCryptProv)
		{
			CryptReleaseContext(m_hCryptProv, 0);
			m_hCryptProv = NULL;
		}
	}

	bool CryptoAPIEncryption::Initialize()
	{
		bool success = false;
		if (AcquireContext())
		{
			success = LoadPublicKey();
		}
		return success;
	}


	unsigned int CryptoAPIEncryption::CreateHeader(char** header)
    {
        char* baseHeader = InitHeader();

		unsigned int size = 0;
		char* fullHeader = AddSessionKey(baseHeader, &size);
		delete[] baseHeader;

		// failure would be indicated by a null return
		if (NULL == fullHeader)
		{
			*header = NULL;
			return 0;
		}

		// Record the key size to the header
		unsigned int keySize = size - VINCE_ENCRYPTION_HEADER_SIZE;
		fullHeader[40] = fullHeader[26] ^ (char)(keySize & 0x000000ff);
		fullHeader[17] = fullHeader[44] ^ (char)((keySize & 0x0000ff00)>>8);

		// To make the session key portion of the header less obvious, we
		// do a quick obscuration pass
		ObscureData(fullHeader + VINCE_ENCRYPTION_HEADER_SIZE, keySize);

		*header = fullHeader;
		return size;
    }


	char* CryptoAPIEncryption::AddSessionKey(const char* header, unsigned int* size)
	{
		// We need to get a new session key and encrypt it using the public key

		// Initialize return values to failure state
		*size = 0;
		char* fullHeader = NULL;
		const unsigned int HEADER_SIZE = 64;

		// Destroy any previously used session keys
		if (m_hSessionKey)
		{
			CryptDestroyKey(m_hSessionKey);
		}

		// Create a random session key. 
		PBYTE pbKeyBlob; 
		DWORD dwKeyBlobLen; 

		if(CryptGenKey(
			m_hCryptProv, 
			ENCRYPT_ALGORITHM, 
			KEYLENGTH | CRYPT_EXPORTABLE, 
			&m_hSessionKey))
		{
			// Load salt value from header
			LoadSalt(header);

			// Set Salt value for session key
			BYTE* pbSalt = (BYTE*) m_bSalt;
			if (!CryptSetKeyParam(
				 m_hSessionKey,
				 KP_SALT,
				 pbSalt,
				 0))
			{
				return false;
			}

			// Now we need to encrypt the session key;
			// Determine size of the key BLOB, and allocate memory. 

			if(!CryptExportKey(
				m_hSessionKey, 
				m_hPublicKey, 
				SIMPLEBLOB, 
				0, 
				NULL, 
				&dwKeyBlobLen))
			{
				return NULL;	// can't continue
			}

			pbKeyBlob = new unsigned char[dwKeyBlobLen];
			if(NULL == pbKeyBlob)
			{ 
				return NULL;	// can't continue
			}

			// Encrypt and export the session key into a simple key BLOB. 
	    
			if(!CryptExportKey(
				m_hSessionKey, 
				m_hPublicKey, 
				SIMPLEBLOB, 
				0, 
				pbKeyBlob, 
				&dwKeyBlobLen))
			{
				delete [] pbKeyBlob;
				return NULL;	// can't continue
			}

			// Create a buffer big enough to hold the original header
			// plus the session key
			unsigned int newSize = HEADER_SIZE + dwKeyBlobLen;
			fullHeader = new char[newSize];
			fullHeader[0] = '\0';
			for (int i = 0; i < HEADER_SIZE; i++)
			{
				fullHeader[i] = header[i];
			}
			for (unsigned int i = 0; i < dwKeyBlobLen; i++)
			{
				fullHeader[i + HEADER_SIZE] = *(pbKeyBlob + i);
			}
			*size = newSize;

			// Free memory. Release session key?
			delete [] pbKeyBlob;
		} 
		return fullHeader;
	}

	bool CryptoAPIEncryption::AcquireContext()
	{
		// Get the handle to the enhanced provider.
		// Skip if we've already done this
		if (m_hCryptProv)
		{
			return true;
		}
		else
		{
			int result = CryptAcquireContext(&m_hCryptProv, 
											NULL, 
											MS_ENHANCED_PROV, 
											PROV_RSA_FULL, 
											CRYPT_VERIFYCONTEXT);
			return (0 != result);
		}
	}

	bool CryptoAPIEncryption::LoadPublicKey()
	{
		// Skip if we've already done this
		if (m_hPublicKey)
		{
			return true;
		}
		else
		{
			BYTE PublicKeyBlob[] =
			{   0x06, 0x02, 0x00, 0x00,
				0x00, 0xA4, 0x00, 0x00,
				0x52, 0x53, 0x41, 0x31,
				0x00, 0x02, 0x00, 0x00,
				0x01, 0x00, 0x01, 0x00,
				0xBD, 0x1B, 0x17, 0x61,
				0x68, 0x37, 0x94, 0x9C,
				0xB2, 0x9F, 0x6C, 0x0F,
				0xB6, 0xF0, 0x50, 0xD8,
				0xE7, 0x53, 0x3D, 0x15,
				0x3A, 0x11, 0x38, 0x90,
				0x9C, 0xDF, 0x50, 0xBA,
				0xEE, 0x87, 0x30, 0xA3,
				0x21, 0xAF, 0xBB, 0xC7,
				0xB7, 0x0E, 0xB7, 0x2C,
				0x7C, 0x5A, 0xA6, 0x7D,
				0xD7, 0x27, 0x09, 0x60,
				0xEB, 0xC2, 0xD6, 0x4D,
				0x48, 0x06, 0x8B, 0xE7,
				0x55, 0x32, 0xF6, 0x4B,
				0x03, 0xD0, 0x91, 0xDE
			};

			// Local variables
			PBYTE pbKeyBlob = (BYTE *)&PublicKeyBlob; 
			DWORD dwKeyBlobLen = 84; 

			// Now we import the key and return the success status
			int result = CryptImportKey(m_hCryptProv, 
										pbKeyBlob, 
										dwKeyBlobLen, 
										0, 
										0, 
										&m_hPublicKey);
			return (0 != result);
		}
	}

	void CryptoAPIEncryption::LoadSalt(const char* header)
	{
		// Salt values are used by the session key as a protection
		// against dictionary attacks. The salt values are loaded from
		// the random file header.
		m_bSalt[0] = header[55];
		m_bSalt[1] = header[42];
		m_bSalt[2] = header[48];
		m_bSalt[3] = header[8];
		m_bSalt[4] = header[47];
		m_bSalt[5] = header[27];
		m_bSalt[6] = header[30];
		m_bSalt[7] = header[2];
		m_bSalt[8] = header[39];
		m_bSalt[9] = header[59];
		m_bSalt[10] = header[16];
	}

	bool CryptoAPIEncryption::EncryptData(char* charArray, int count)
	{
		ObscureData(charArray, count);
		PBYTE pbBuffer = (PBYTE) charArray; 
		DWORD dwCount = count;
		DWORD dwBufferLen = count;
		int success = CryptEncrypt(m_hSessionKey, 
							        0, 
							        false, 
							        0, 
							        pbBuffer, 
							        &dwCount, 
							        dwBufferLen);
		return (success && (dwCount == dwBufferLen));
	} 
}

#endif