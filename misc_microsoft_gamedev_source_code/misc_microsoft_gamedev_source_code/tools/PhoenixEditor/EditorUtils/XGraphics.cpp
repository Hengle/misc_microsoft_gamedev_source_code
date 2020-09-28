#define _WIN32_WINNT 0x500
#include <windows.h>
#include <d3dx9.h>

#include "endswap.inl"

// xcore
#include "xcore.h"
#include "containers\dynamicArray.h"
#include "math\generalVector.h"

// ximage
#include "colorUtils.h"
#include "RGBAImage.h"

// compression
#include "DXTUtils.h"
#include "DXTPacker.h"
#include "DXTUnpacker.h"
#include "companders.h"


// xbox - xgraphics
#include <xgraphics.h>


extern "C" __declspec(dllexport) void ConvF32ToF16(unsigned char *out, const float *in,int count,bool doEndSwap)
{
   D3DXFloat32To16Array((D3DXFLOAT16*)out,in,count);
   if(doEndSwap)
      {   
         D3DXFLOAT16* fd= (D3DXFLOAT16*)out;
         for(int i=0;i<count;i++)
            fd[i]=endSwapW16((unsigned short*)&fd[i]);
   }
} 
//----------------------------------------------
/*
extern "C" __declspec(dllexport) void tileCopyData(void *dst, const void *src,const int Width,const int Height,const int dxtFormat,const int pixelMemSize)
{
   UINT uWidthInBlocks     = Width;
   UINT uHeightInBlocks    = Height;
   UINT srcBytesPerBlock   = pixelMemSize;
   UINT srcRowPitch        = srcBytesPerBlock * uWidthInBlocks;

   if(dxtFormat != cDXTInvalid)
   {
      uWidthInBlocks    = Width>>2;
      uHeightInBlocks   = Height>>2;
      srcBytesPerBlock  = (dxtFormat==cDXN)?16:8;// BDXTUtils::getSizeOfDXTBlock(dxtFormat);
      srcRowPitch       = srcBytesPerBlock * uWidthInBlocks;
   }

   XGTileSurface(  dst,  uWidthInBlocks, uHeightInBlocks,0,      
      src,  srcRowPitch ,  0,srcBytesPerBlock);
}*/

extern "C" __declspec(dllexport) bool tileCopyData(void *dst, const void *src,const int Width,const int Height,const int dxtFormat,const int pixelMemSize)
{

	

  UINT srcRowPitch        = 0;

   D3DFORMAT fmt;
   if(dxtFormat==0)  
   {
      if(Width%64!=0 || Height%64!=0)return false;
	   srcRowPitch = sizeof(short)*Width;
	   fmt = D3DFMT_R16F;
   }
   else if(dxtFormat==1)
   {
      if(Width%64!=0 || Height%64!=0)return false;
	   srcRowPitch = sizeof(short)*Width;
	   fmt = D3DFMT_G8R8;
   }
   else if(dxtFormat==2)
   {
      if(Width<128 || Height<128)return false;
	   srcRowPitch = 16 * (Width>>2);
	   fmt = D3DFMT_DXN;
   }
   else if(dxtFormat==3)
   {
     // if(Width<128 || Height<128)return false;
      srcRowPitch = Width;
      fmt = D3DFMT_L8;
   }
   else if(dxtFormat==4)
   {
      if(Width%64!=0 || Height%64!=0)return false;
      srcRowPitch = (Width>>2);
      fmt = D3DFMT_DXT5A;
   }
   else if(dxtFormat==5)
   {
      if(Width%64!=0 || Height%64!=0)return false;
      srcRowPitch = sizeof(DWORD) * Width;
      fmt = D3DFMT_R11G11B10;
   }
   else if(dxtFormat==6)
   {
      if(Width%64!=0 || Height%64!=0)return false;
      srcRowPitch = (Width>>2)*8;
      fmt = D3DFMT_DXT1;
   }
   else if(dxtFormat==7)
   {
      if(Width%64!=0 || Height%64!=0)return false;
      srcRowPitch = sizeof(short)*2*Width;
      fmt = D3DFMT_G16R16;
   }


   XGTileTextureLevel(Width, Height, 0, XGGetGpuFormat(fmt), 0, dst, NULL, src, srcRowPitch, NULL);


   return true;
}


