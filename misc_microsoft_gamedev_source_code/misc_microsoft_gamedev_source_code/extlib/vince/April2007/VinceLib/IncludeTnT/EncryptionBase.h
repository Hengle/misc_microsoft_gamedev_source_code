/////////////////////////////////////////////////////////////////////////////////////
//	EncryptionBase.h : Base class for encryption methods
//
//	Created 2006/04/19 Rich Bonny <rbonny@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2006 Microsoft Corp.  All rights reserved.
/////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "IEncryption.h"

#define TNT_ENCRYPTION_HEADER_SIZE 64

namespace TnT
{
	// VinceEncryption is a singleton class used by the LogWriter class
	class EncryptionBase : public IEncryption
	{
	public:
		         EncryptionBase();
		virtual ~EncryptionBase();

		virtual bool Initialize() = 0;
		virtual unsigned int CreateHeader(char** header) = 0;
		virtual bool EncryptData(char* charArray, int count) = 0;
		virtual bool EncryptData(BYTE* byteArray, int count);

	protected:
		virtual unsigned int CreateSeed();
		virtual unsigned char GetRandomByte();
		virtual char* InitHeader();
		virtual unsigned int HashHeader(const char* header);
		virtual unsigned int GetHash(const char* header, unsigned int b1, unsigned int b2, unsigned int b3, unsigned int b4,
							 unsigned int b5, unsigned int b6, unsigned int b7, unsigned int b8);
		virtual DWORD GetWatermark() = 0;
		virtual void  StampWatermark(char* header);
		virtual void ObscureData(char* charArray, int count);
		virtual void UnobscureData(char* charArray, int count);

	protected:
		unsigned int m_seed;
		unsigned char m_aByte;
		unsigned char m_bByte;
	};
}
