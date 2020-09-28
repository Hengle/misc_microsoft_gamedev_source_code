//============================================================================
//
// File: CLI_Interface.cpp
// Copyright (c) 2007, Ensemble Studios
//
//============================================================================
#include <windows.h>
#include <assert.h>

using namespace System;

#include "dxt.h"
namespace Compression
{
   public ref class DXT_CLI
   {
   public:

      #define FORMAT_DXT1      1
      #define FORMAT_DXT5      2
      #define FORMAT_DXT5YCOCG 3

      static public bool compressDXT(byte *InData, int inWidth, int inHeight, int format,byte *pOutData, int &outMemSize)
     {
         outMemSize =-1;

         byte *pInData = (byte*)memalign(16, inWidth*inHeight*4);
         memcpy(pInData,InData,inWidth*inHeight*4);

         if(pOutData==0)
         {
            assert(pOutData && "ERROR - outData must be initalized to width*height*4 in your C# app.");
            return false;
         }

        // *pOutData = (byte*)memalign(16, inWidth*inHeight*4);
       //  memset(*pOutData, 0, inWidth*inHeight*4);

         switch (format) {
        case FORMAT_DXT1:
           CompressImageDXT1( pInData, pOutData, inWidth, inHeight, outMemSize);
           break;
        case FORMAT_DXT5:
           CompressImageDXT5( pInData, pOutData, inWidth, inHeight, outMemSize);
           break;
        case FORMAT_DXT5YCOCG:
           CompressImageDXT5YCoCg( pInData, pOutData, inWidth, inHeight, outMemSize);
           break;
         }

         aligned_free(pInData);
         pInData=NULL;

         return outMemSize==-1?false:true;
      }
   };
}