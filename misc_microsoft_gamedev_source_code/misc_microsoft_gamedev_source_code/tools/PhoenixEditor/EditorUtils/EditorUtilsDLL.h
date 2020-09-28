#pragma once
#include <windows.h>
#include <d3dx9.h>
#include <stdio.h>

#define EUDLL_INTERFACE_VERSION 3


//This forward Decl is NOT needed for C#; But we upkeep it for ease of read
extern "C" __declspec(dllexport) DWORD getVersion();

//XTD
struct XTDVisual;
struct QNTexInfo;
extern "C" __declspec(dllexport) int compressTerrainDataToMemory(const D3DXVECTOR3 *relPos,const D3DXVECTOR3 *normals,int numXVerts,XTDVisual *mVisDat);
extern "C" __declspec(dllexport) void cleanCompressedTerrain(XTDVisual &mVisDat);

//XBOX XGRAPHICS
extern "C" __declspec(dllexport) void ConvF32ToF16(unsigned char *out, const float *in,int count,bool doEndSwap);
extern "C" __declspec(dllexport) void tileCopyData(void *dst, const void *src,const int Width,const int Height,const int dxtFormat,const int pixelMemSize);


//IMAGE LOADING / SAVING
extern "C" __declspec(dllexport) bool loadImg(const char *filename,const bool isDDX,  const bool forceToRGBA8,int &numMips,int &width, int &height, int &D3DFormat,char **data ,int &dataMemSize);
extern "C" __declspec(dllexport) void freeLoadedImgData(unsigned char *dat);
