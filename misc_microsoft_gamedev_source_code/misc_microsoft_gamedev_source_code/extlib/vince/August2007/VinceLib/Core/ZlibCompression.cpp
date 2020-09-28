//	ZlibCompression.cpp : Methods for compressing a buffer
//
//  Uses zlib compression.
//
//	Created 2006/04/19 Rich Bonny <rbonny@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2006 Microsoft Corp.  All rights reserved.

#ifdef _XBOX
    #include <xtl.h>
#else
    #include <windows.h>
#endif

#include "ZlibCompression.h"
#include "zlib.h"

namespace Vince
{
	ZlibCompression::ZlibCompression()
	{ 
	}

	ZlibCompression::~ZlibCompression()
	{
	}

    int ZlibCompression::GetCompressionBufferSize(const char*, int srcBufSize)
    {
        // Zlib expects the destination buffer to be 0.1% larger than the source buffer size plus 12 bytes (see zlib.h)
        int size  = srcBufSize + (srcBufSize / 1000) + 12;
		
        // Add room for the size int at the head of the buffer
        size += sizeof(int);		
    
        return(size);
    }

	int ZlibCompression::CompressData(const char* srcBuf, int srcBufSize, char* destBuf, int destBufSize)
	{
        // Actual compressed size comes back in destBufSize
		compress((Bytef *)(destBuf + sizeof(int)), (uLongf *)&destBufSize, (Bytef *)srcBuf, (uLong)srcBufSize);

		// Store int with size of compressed data at head of buffer
		// This has to be correct endian orientation for the current platform
#ifdef _XBOX
		*((int *)destBuf) = destBufSize;
#else
		int reverse;
		((unsigned char*)&reverse)[0] = ((unsigned char*)&destBufSize)[3];
		((unsigned char*)&reverse)[1] = ((unsigned char*)&destBufSize)[2];
		((unsigned char*)&reverse)[2] = ((unsigned char*)&destBufSize)[1];
		((unsigned char*)&reverse)[3] = ((unsigned char*)&destBufSize)[0];
		*((int *)destBuf) = reverse;
#endif

        // Return actual compressed data size plus the size int
        return(destBufSize + sizeof(int));
	}

}