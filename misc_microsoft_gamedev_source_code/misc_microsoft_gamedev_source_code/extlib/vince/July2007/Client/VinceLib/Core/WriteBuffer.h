//	WriteBuffer.h : Simple class to hold temporary
//  buffers waiting to be written to disk in
//  asynchronous logging operations
//
//	Created 2007/01/18 Rich Bonny <rbonny@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) Microsoft Corp.  All rights reserved.

#pragma once

namespace Vince
{
	class WriteBuffer
	{
	public:
		WriteBuffer(const char* strBuffer, int bufferSize);
		~WriteBuffer(void);
        WriteBuffer* GetNext() {return m_pNext;};
        void SetNext(WriteBuffer* pNext) {m_pNext = pNext;};
        int GetSize() {return m_size;};
        char* GetBuffer() {return m_pBuffer;};

    protected:
        int m_size;
        char* m_pBuffer;
        WriteBuffer* m_pNext;
    };
}   // namespace vince
