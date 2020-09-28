//	WriteBuffer.cpp : Simple class to hold temporary
//  buffers waiting to be written to disk in
//  asynchronous logging operations
//
//	Created 2007/01/18 Rich Bonny <rbonny@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) Microsoft Corp.  All rights reserved.

#include "WriteBuffer.h"
#include "StringUtil.h"
#include <string.h>

namespace Vince
{
    WriteBuffer::WriteBuffer(const char* strBuffer, int bufferSize)
    {
        m_size = bufferSize;
        m_pBuffer = new char[bufferSize];
        memcpy(m_pBuffer, strBuffer, bufferSize);
        m_pNext = NULL;
    }

    WriteBuffer::~WriteBuffer(void)
    {
        SAFE_DELETE_ARRAY(m_pBuffer);
    }
}   // namespace vince
