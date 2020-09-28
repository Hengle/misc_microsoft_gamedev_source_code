/****************************************************************************************
	
    Copyright (C) NVIDIA Corporation 2003

    TO THE MAXIMUM EXTENT PERMITTED BY APPLICABLE LAW, THIS SOFTWARE IS PROVIDED
    *AS IS* AND NVIDIA AND ITS SUPPLIERS DISCLAIM ALL WARRANTIES, EITHER EXPRESS
    OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY
    AND FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL NVIDIA OR ITS SUPPLIERS
    BE LIABLE FOR ANY SPECIAL, INCIDENTAL, INDIRECT, OR CONSEQUENTIAL DAMAGES
    WHATSOEVER (INCLUDING, WITHOUT LIMITATION, DAMAGES FOR LOSS OF BUSINESS PROFITS,
    BUSINESS INTERRUPTION, LOSS OF BUSINESS INFORMATION, OR ANY OTHER PECUNIARY LOSS)
    ARISING OUT OF THE USE OF OR INABILITY TO USE THIS SOFTWARE, EVEN IF NVIDIA HAS
    BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.

*****************************************************************************************/
#pragma once

#include <dds/nvdxt_options.h>
#include <dds/nvErrorCodes.h>
// for .dds file reading and writing

// see TestDXT.cpp for examples

typedef NV_ERROR_CODE (*DXTReadCallback)(void *buffer, size_t count, void * userData);


struct MIPMapData
{
    size_t mipLevel; 
    size_t width;
    size_t height;

    int faceNumber; // current face number for this image

    int numFaces;   // total number of faces 
        // depth for volume textures
        // 6 for cube maps


};

typedef NV_ERROR_CODE (*DXTWriteCallback) (const void *buffer,
                                     size_t count, 
                                     const MIPMapData * mipMapData, // if nz, this is MIP data
                                     void * userData);




// DXT Error Codes




/*

   Compresses an image with a user supplied callback with the data for each MIP level created

*/

typedef enum nvPixelOrder
{
    nvRGBA,
    nvBGRA,
    nvRGB,
    nvBGR,
    nvGREY,  // one plance copied to RGB

};
class nvDDS
{

public:
    static NV_ERROR_CODE nvDXTcompress(const nvImageContainer & imageContainer,
        nvCompressionOptions * options,
        DXTWriteCallback fileWriteRoutine,  // call to .dds write routine
        const RECT * rect = 0);

    static NV_ERROR_CODE nvDXTcompress(const RGBAImage & srcImage,
        nvCompressionOptions * options,
        DXTWriteCallback fileWriteRoutine,  // call to .dds write routine
        const RECT * rect = 0);

    static NV_ERROR_CODE nvDXTcompress(const unsigned char * srcImage,
        size_t width,
        size_t height,
        size_t byte_pitch,
        nvPixelOrder pixelOrder,

        nvCompressionOptions * options,
        DXTWriteCallback fileWriteRoutine,  // call to .dds write routine
        const RECT * rect = 0);


    // image with MIP maps
    static NV_ERROR_CODE nvDXTcompress(const RGBAMipMappedImage & srcMIPImage,
        nvCompressionOptions * options,
        DXTWriteCallback fileWriteRoutine,  // call to .dds write routine
        const RECT * rect = 0);

    static NV_ERROR_CODE nvDXTcompress(const RGBAMipMappedCubeMap & srcMIPCubeMap,
        nvCompressionOptions * options,
        DXTWriteCallback fileWriteRoutine,  // call to .dds write routine
        const RECT * rect = 0);


    static NV_ERROR_CODE nvDXTcompress(const RGBAMipMappedVolumeMap & srcMIPVolumeMap,
        nvCompressionOptions * options,
        DXTWriteCallback fileWriteRoutine,  // call to .dds write routine
        const RECT * rect = 0);




    static NV_ERROR_CODE nvDXTcompress(const fpImage & srcImage,
        nvCompressionOptions * options,
        DXTWriteCallback fileWriteRoutine,
        const RECT * rect = 0);


    static NV_ERROR_CODE nvDXTcompress(const fpMipMappedImage & srcMIPImage,
        nvCompressionOptions * options,
        DXTWriteCallback fileWriteRoutine,
        const RECT * rect = 0);


    static NV_ERROR_CODE nvDXTcompress(const fpMipMappedCubeMap & srcMIPCubeMap,
        nvCompressionOptions * options,
        DXTWriteCallback fileWriteRoutine,  
        const RECT * rect = 0);



    static NV_ERROR_CODE nvDXTcompress(const fpMipMappedVolumeMap & srcMIPVolumeMap,
        nvCompressionOptions * options,
        DXTWriteCallback fileWriteRoutine,  
        const RECT * rect = 0);



    // array of images for volume map or cube map
    static NV_ERROR_CODE nvDXTcompress(const fpImageArray & srcImageArray,
        nvCompressionOptions * options,
        DXTWriteCallback fileWriteRoutine,  
        const RECT * rect = 0);




    // readMIPMapCount, number of MIP maps to load. 0 is all


    static NV_ERROR_CODE  nvDXTdecompress(
        nvImageContainer & imageData,
        nvPixelFormat pf,        
        int readMIPMapCount,

        DXTReadCallback fileReadRoutine,
        void * userData);


};



#ifndef EXCLUDE_LIBS

#if _DEBUG

 #if _MSC_VER >=1300
  #ifdef _MT
   #ifdef _DLL
    #ifdef _STATIC_CPPLIB
     #pragma message("Note: including lib: nvDXTlibMTDLL_Sd.lib") 
     #pragma comment(lib, "nvDXTlibMTDLL_Sd.lib")
    #else
     #pragma message("Note: including lib: nvDXTlibMTDLLd.lib") 
     #pragma comment(lib, "nvDXTlibMTDLLd.lib")
    #endif
   #else // DLL
    #ifdef _STATIC_CPPLIB
     #pragma message("Note: including lib: nvDXTlibMT_Sd.lib") 
     #pragma comment(lib, "nvDXTlibMT_Sd.lib")
    #else
     #pragma message("Note: including lib: nvDXTlibMTd.lib") 
     #pragma comment(lib, "nvDXTlibMTd.lib")
    #endif
   #endif //_DLL
  #else // MT
    #ifdef _STATIC_CPPLIB
     #pragma message("Note: including lib: nvDXTlib_Sd.lib") 
     #pragma comment(lib, "nvDXTlib_Sd.lib")
    #else
     #pragma message("Note: including lib: nvDXTlibd.lib") 
     #pragma comment(lib, "nvDXTlibd.lib")
    #endif
  #endif // _MT
 #else // _MSC_VER

  #ifdef _MT
   #ifdef _DLL                         
    #pragma message("Note: including lib: nvDXTlibMTDLL6.lib") 
    #pragma comment(lib, "nvDXTlibMTDLL6.lib")
   #else // _DLL
    #pragma message("Note: including lib: nvDXTlibMT6.lib") 
    #pragma comment(lib, "nvDXTlibMT6.lib")
   #endif //_DLL
  #else // _MT
   #pragma message("Note: including lib: nvDXTlib6.lib") 
   #pragma comment(lib, "nvDXTlib6.lib")
  #endif // _MT
 
 #endif // _MSC_VER

#else // _DEBUG


 #if _MSC_VER >=1300
  #ifdef _MT
   #ifdef _DLL
    #ifdef _STATIC_CPPLIB
     #pragma message("Note: including lib: nvDXTlibMTDLL_S.lib") 
     #pragma comment(lib, "nvDXTlibMTDLL_S.lib")
    #else
     #pragma message("Note: including lib: nvDXTlibMTDLL.lib") 
     #pragma comment(lib, "nvDXTlibMTDLL.lib")
    #endif
   #else // DLL
    #ifdef _STATIC_CPPLIB
     #pragma message("Note: including lib: nvDXTlibMT_S.lib") 
     #pragma comment(lib, "nvDXTlibMT_S.lib")
    #else
     #pragma message("Note: including lib: nvDXTlibMT.lib") 
     #pragma comment(lib, "nvDXTlibMT.lib")
    #endif
   #endif //_DLL
  #else // MT
    #ifdef _STATIC_CPPLIB
     #pragma message("Note: including lib: nvDXTlib_S.lib") 
     #pragma comment(lib, "nvDXTlib_S.lib")
    #else
     #pragma message("Note: including lib: nvDXTlib.lib") 
     #pragma comment(lib, "nvDXTlib.lib")
    #endif
  #endif // _MT
 #else // _MSC_VER

  #ifdef _MT
   #ifdef _DLL                         
    #pragma message("Note: including lib: nvDXTlibMTDLL6.lib") 
    #pragma comment(lib, "nvDXTlibMTDLL6.lib")
   #else // _DLL
    #pragma message("Note: including lib: nvDXTlibMT6.lib") 
    #pragma comment(lib, "nvDXTlibMT6.lib")
   #endif //_DLL
  #else // _MT
   #pragma message("Note: including lib: nvDXTlib6.lib") 
   #pragma comment(lib, "nvDXTlib6.lib")
  #endif // _MT
 
 #endif // _MSC_VER
#endif // _DEBUG

#endif













