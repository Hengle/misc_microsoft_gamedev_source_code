//	ZlibCompression.h : Methods for compressing a buffer
//
//  Uses zlib compression.
//
//	Created 2006/04/19 Rich Bonny <rbonny@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2006 Microsoft Corp.  All rights reserved.

#pragma once

#include "ICompression.h"

namespace Vince
{
	class ZlibCompression : public ICompression
	{
	public:
		         ZlibCompression();
		virtual ~ZlibCompression();

        virtual int GetCompressionBufferSize(const char* srcBuf, int srcBufSize);
	    virtual int CompressData(const char* srcBuf, int srcBufSize, char* destBuf, int destBufSize);
	};
}
