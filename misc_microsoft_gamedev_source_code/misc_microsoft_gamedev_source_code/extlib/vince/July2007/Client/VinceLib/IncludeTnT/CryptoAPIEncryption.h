/////////////////////////////////////////////////////////////////////////////////////
//	CryptoAPIEncryption.h : Version of encryption methods for use on Windows
//                          using CryptoAPI functions
//
//	Created 2006/04/19 Rich Bonny <rbonny@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2006 Microsoft Corp.  All rights reserved.
/////////////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef _XBOX       // PC only

#include "EncryptionBase.h"

#define TNT_CRYPTO_WATERMARK_WIN  0x471CF406

namespace TnT
{
	class CryptoAPIEncryption : public EncryptionBase
	{
	public:
		         CryptoAPIEncryption();
		virtual ~CryptoAPIEncryption();
		virtual bool Initialize();
	    virtual unsigned int CreateHeader(char** header);
		virtual bool EncryptData(char* charArray, int count);

	protected:
		DWORD GetWatermark()	{ return TNT_CRYPTO_WATERMARK_WIN; };
		char* AddSessionKey(const char* header, unsigned int* size );
		bool AcquireContext();
		bool LoadPublicKey();
		void LoadSalt(const char* header);

	private:
		unsigned long m_hCryptProv; 
		unsigned long m_hPublicKey;
		unsigned long m_hSessionKey;
		char m_bSalt[11];
	};
}

#endif