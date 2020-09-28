/////////////////////////////////////////////////////////////////////////////////////
//	XboxEncryption.h : Version of encryption methods for use on Xbox 360
//
//	Created 2006/04/19 Rich Bonny <rbonny@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2006 Microsoft Corp.  All rights reserved.
/////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "EncryptionBase.h"

#define TNT_CRYPTO_WATERMARK_XBOX 0xc320a573

namespace TnT
{
	class XboxEncryption : public EncryptionBase
	{
	public:
		         XboxEncryption();
		virtual ~XboxEncryption();

		virtual bool Initialize();
        virtual unsigned int CreateHeader(char** header);
		virtual bool EncryptData(char* charArray, int count);

	protected:
		DWORD GetWatermark()	{return TNT_CRYPTO_WATERMARK_XBOX; };
	};
}
