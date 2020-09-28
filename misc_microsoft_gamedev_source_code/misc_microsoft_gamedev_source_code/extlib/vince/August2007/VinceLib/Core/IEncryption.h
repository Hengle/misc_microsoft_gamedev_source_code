/////////////////////////////////////////////////////////////////////////////////////
//  IEncryption : Interface to encryption object that compresses
//  a data buffer on either Xbox or PC
//
//  Created 2006/04/19 Rich Bonny <rbonny@microsoft.com>                             
//                                                                                  
//  Microsoft Confidential.  Do NOT Distribute.                                     
//  Copyright (c) 2006 Microsoft Corp.  All rights reserved.                        
/////////////////////////////////////////////////////////////////////////////////////
#pragma once 

#ifdef _XBOX
    #include <xtl.h>
#else
    #include <windows.h>
#endif

namespace Vince
{
    class IEncryption
    {
	public:
                 IEncryption() {}
        virtual ~IEncryption() {}

		virtual bool Initialize() = 0;
		virtual unsigned int CreateHeader(char** header) = 0;
		virtual bool EncryptData(char* charArray, int count) = 0;
		virtual bool EncryptData(BYTE* charArray, int count) = 0;
    };
}
