/////////////////////////////////////////////////////////////////////////////////////
//  ICompression : Interface to compression object that compresses
//  a data buffer on either Xbox or PC
//
//  Created 2006/04/19 Rich Bonny <rbonny@microsoft.com>                             
//                                                                                  
//  Microsoft Confidential.  Do NOT Distribute.                                     
//  Copyright (c) 2006 Microsoft Corp.  All rights reserved.                        
/////////////////////////////////////////////////////////////////////////////////////
#pragma once 

namespace TnT
{
    class ICompression
    {
    public:
                 ICompression() {}
        virtual ~ICompression() {}

        // Get the size of the buffer required by the compressor
        virtual int GetCompressionBufferSize(const char* srcBuf, int srcBufSize) = 0;

        // Perform the compression into the destination buffer.  Returns the actual compressed size.
	    virtual int CompressData(const char* srcBuf, int srcBufSize, char* destBuf, int destBufSize) = 0;
    };
}
