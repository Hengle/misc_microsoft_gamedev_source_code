#define _WIN32_WINNT 0x500
#include <windows.h>
#include <d3dx9.h>
#include <vector>
#include <assert.h>

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

// terrain
//#include "..\..\..\terrain\TerrainIO.h"

#include "endSwap.inl"

//FORWARD
extern "C" __declspec(dllexport) void tileCopyData(void *dst, const void *src,const int Width,const int Height,const int dxtFormat,const int pixelMemSize);

//-------------------------------------------------------------------------------------------
const char maxBits=127;
const float maxBitsF = 127.f;/*
//-------------------------------------------------------------------------------------------
float computeCanonicalAngle(const D3DXVECTOR3 *pNormal, const D3DXVECTOR3 *pTangent)
{
   // Compute canonical angle
   D3DXVECTOR3 reference;
   D3DXVECTOR3 axisX(1.0f, 0.0f, 0.0f);
   D3DXVECTOR3 axisZ(0.0f, 0.0f, 1.0f);

   float canonicalAngle = 0.0f;
   float dotX = D3DXVec3Dot(pNormal, &axisX);
   float dotZ = D3DXVec3Dot(pNormal, &axisZ);

   if(fabs(dotX) < fabs(dotZ))
   {
      D3DXVec3Cross(&reference, &axisX, pNormal);
      D3DXVec3Normalize(&reference, &reference);

      float dot = D3DXVec3Dot(pTangent, &reference);
      if(dot < -1.0f) dot = -1.0f;
      if(dot > 1.0f) dot = 1.0f;
      canonicalAngle = acos(dot) - (3.1415926536f * .5f);    // 90
   }
   else
   {
      D3DXVec3Cross(&reference, pNormal, &axisZ);
      D3DXVec3Normalize(&reference, &reference);

      float dot = D3DXVec3Dot(pTangent, &reference);
      if(dot < -1.0f) dot = -1.0f;
      if(dot > 1.0f) dot = 1.0f;
      canonicalAngle = acos(dot);
   }


   // Make sure angle is alway positive from 0 - 2PI
   if(canonicalAngle < 0.0f)
   {
      canonicalAngle += (3.1415926536f * 2.0f);    // 180
   }

   return(canonicalAngle);
}
//-------------------------------------------------------------------------------------------
void HTransformAxis(const int axis,const D3DXVECTOR3 *uncompressedData, const int width, const int height,D3DXVECTOR3 **coeffData,D3DXVECTOR3 &ranges)
{
   //H-Transform
   int numXBlocks = width>>1;
   int numZBlocks = height>>1;
   D3DXVECTOR3 *cfs = new D3DXVECTOR3[numXBlocks*numZBlocks];
   

   int counter=0;
   D3DXVECTOR3 mx(-99999.f,-99999.f,-99999.f);
   D3DXVECTOR3 mn(99999.f,99999.f,99999.f);

   for(int z=0;z<numZBlocks;z++)
   {
      for(int x=0;x<numXBlocks;x++)
      {
         //grab our 4x4 Y values
         float vals[2][2];
         for(int i=0;i<2;i++)   
         {
            for(int j=0;j<2;j++)
            {
               int indx= (((x*2)+i)*width) + ((z*2)+j);
               vals[1-i][1-j] = uncompressedData[indx][axis];
            }
         }

         //transform
         float h_0	=	0.25f*(vals[1][1]+vals[1][0]+vals[0][1]+vals[0][0]);//dc
         float h_x	=	0.25f*(vals[1][1]+vals[1][0]-vals[0][1]-vals[0][0]);//x
         float h_y	=	0.25f*(vals[1][1]-vals[1][0]+vals[0][1]-vals[0][0]);//y
         float h_c	=	0.25f*(vals[1][1]-vals[1][0]-vals[0][1]+vals[0][0]);//diag component

         if(h_0 > mx[0]) mx[0]=h_0;
         if(h_0 < mn[0]) mn[0]=h_0;

         if(h_x > mx[1]) mx[1]=h_x;
         if(h_x < mn[1]) mn[1]=h_x;

         if(h_y > mx[2]) mx[2]=h_y;
         if(h_y < mn[2]) mn[2]=h_y;

         cfs[counter][0]=h_0;
         cfs[counter][1]=h_x;
         cfs[counter][2]=h_y; 

         counter++;
      }
   }
   ranges=D3DXVECTOR3(mx[0]-mn[0],mx[1]-mn[1],mx[2]-mn[2]);

   *coeffData = cfs;
}
*/
extern "C" __declspec(dllexport) void FreeCompressedData(uchar **outTexture)
{
   delete [] (*outTexture);
}

extern "C" __declspec(dllexport) void CompressToFormat(byte *data, const int numXPixels,const int numZPixels, const int pixelMemSize,int desiredFormat,int desiredQuality,uchar **outTexture,int &outSize)
{

   BRGBAImage XDispImg(numXPixels, numZPixels);
   
   BRGBAColor *cPtr = XDispImg.getPtr();
   memcpy(cPtr,data,pixelMemSize * numXPixels * numZPixels);
   

   BByteArray DXTND;
   BDXTPacker packer;
   packer;
   bool success = packer.pack(XDispImg, (BDXTFormat)desiredFormat, (eDXTQuality)desiredQuality, false, false, DXTND);
   assert(success);

   outSize=DXTND.size();
   *outTexture = new uchar[outSize];
   memcpy(*outTexture,&DXTND[0],outSize);
   DXTND.clear();
} 

extern "C" __declspec(dllexport) void CompressToDXTNDirect(byte *data, const int numXBlocks,const int numZBlocks,uchar **DXTN,int &dxtnSize)
{
	BRGBAImage XDispImg(numXBlocks, numZBlocks);
	{
		int c=0;
		for(int i=0;i<numXBlocks;i++)
		{
			for(int j=0;j<numZBlocks;j++)
			{
				XDispImg(i, j) = BRGBAColor(data[c++],
											data[c++],
											0,
											0);
				
			}
		}

		BByteArray DXTND;
		BDXTPacker packer;
		packer;
		bool success = packer.pack(XDispImg, cDXN, cDXTQualityBest, false, false, DXTND);
		assert(success);

		dxtnSize=DXTND.size();
		*DXTN = new uchar[dxtnSize];
		memcpy(*DXTN,&DXTND[0],dxtnSize);
	}
}

extern "C" __declspec(dllexport) void CompressToDXTN(D3DXVECTOR3 *CoeffData, const int numXBlocks,const int numZBlocks,const float rangeH,const float rangeV,uchar **DXTN,int &dxtnSize)
{
	BRGBAImage XDispImg(numXBlocks, numZBlocks);
	{
		for(int i=0;i<numXBlocks;i++)
		{
			for(int j=0;j<numZBlocks;j++)
			{
				XDispImg(i, j) = BRGBAColor(compand_RngSqrt( CoeffData[i*numXBlocks+j].y,rangeH,maxBits)+maxBits,
					compand_RngSqrt( CoeffData[i*numXBlocks+j].z,rangeV,maxBits)+maxBits,
					0,0);
			}
		}

		BByteArray DXTND;
		BDXTPacker packer;
		packer;
		bool success = packer.pack(XDispImg, cDXN, cDXTQualityBest, false, false, DXTND);
		assert(success);
		
		dxtnSize=DXTND.size();
		*DXTN = new uchar[dxtnSize];
		memcpy(*DXTN,&DXTND[0],dxtnSize);
	}
}

//-------------------------------------------------------------------------------------------
void CompressToDXTN(D3DXVECTOR3 *CoeffData, const int numXBlocks,const int numZBlocks,const float rangeH,const float rangeV,BDynamicArray<uchar> &DXTN)
{
   BRGBAImage XDispImg(numXBlocks, numZBlocks);
   {
      for(int i=0;i<numXBlocks;i++)
      {
         for(int j=0;j<numZBlocks;j++)
         {
            XDispImg(i, j) = BRGBAColor(compand_RngSqrt( CoeffData[i*numXBlocks+j].y,rangeH,maxBits)+maxBits,
               compand_RngSqrt( CoeffData[i*numXBlocks+j].z,rangeV,maxBits)+maxBits,
               0,0);
         }
      }

      BDXTPacker packer;
      packer;
      bool success = packer.pack(XDispImg, cDXN, cDXTQualityBest, false, false, DXTN);
      assert(success);
   }
}/*
//-----------------------------------------------------------------------------
inline int psuedo_compand_RngSqrt(const float val,const float range,const char maxVal=127)
{
   const float rangeVal = 1.f / range;
   const int cVal = maxVal;

   float aVal = abs(val/rangeVal);
   float sVal = sqrt(aVal);
   float sign = (val<0)?-1:1;
   int comp = ((cVal*(sVal*sign)));
   return comp;
}

enum rangeReturn
{
   cGood=0,
   cRangeToLarge = 1,
   cRangeToSmall = 2
};
int hist[256];
int watchCount=0;
rangeReturn evaluateCompands(const D3DXVECTOR3 *coeffs,const int numCoeffs,const float range,const float compand, const int coeffIndex)
{
   if(watchCount>100)
      return cGood;

   float modRange=(1/(range*compand));

   //histogram the data
   memset(&hist,0x00,sizeof(int)*256);

   for(int i=0;i<numCoeffs;i++)
   {
      float val = coeffs[i][coeffIndex];
      int compandVal = psuedo_compand_RngSqrt( val,modRange,maxBits);
      if(compandVal > 127 || compandVal < -127)
         return cRangeToLarge;

      unsigned char kval = (unsigned char)compandVal+maxBits;
      assert(kval<255 && kval>=0);
      hist[kval]++;
   }

   //find min/max values
   int min=255;int max=0;
   for(int i=0;i<255;i++)
   {
      if(hist[i]!=0){min=i;break;}
   }

   for(int i=255;i>=0;i--)
   {
      if(hist[i]!=0){max=i;break;}
   }

   int tol = 244;
   if(max-min >=tol )
      return cGood;

   return cRangeToSmall;
}
//-----------------------------------------------------------------------------     
void findCompandValues(const D3DXVECTOR3 *coeffs,const int numCoeffs, const D3DXVECTOR3 ranges,float &compandV, float &compandH, float &compandD)
{
   
   float compands[3];
   for(int i=0;i<3;i++)
   {
      watchCount=0;
      float min=0; float max=1.f;
      float value = 0.5;
      rangeReturn retVal=evaluateCompands(coeffs,numCoeffs,ranges[i],value,i);
      while(retVal!=cGood )
      {
         if(retVal == cRangeToLarge)//ie the compand value is too small.
         {
            min=value;
            value = (max+min)/2.f;
         }
         else if(retVal == cRangeToSmall)      //ie the compand value is too large.
         {
            max=value;
            value = (max+min)/2.f;
         }

         watchCount++;
         retVal=evaluateCompands(coeffs,numCoeffs,ranges[i],value,i);
      }
      compands[i]=value;
   }
   compandD = compands[0];
   compandH = compands[1];
   compandV = compands[2];
}
//-----------------------------------------------------------------------------     
void compressAxis(const int axis, const D3DXVECTOR3 *uncompressedData,const int width, const int height,float *DCcoeffs,BDynamicArray<uchar> &HVDXTNData,D3DXVECTOR3 &ranges)
{
   int numXBlocks = width>>1;
   int numZBlocks = height>>1;

   D3DXVECTOR3 *coeffs=0;
   HTransformAxis(axis,uncompressedData, width, height,&coeffs,ranges);


   float rangeH=0;
   float rangeV=0;
   float rangeD=0;

   if(ranges[0]!=0 && ranges[1] !=0 && ranges[2] !=0)
   {
      float compandV = 1.f;
      float compandH = 1.f;
      float compandD = 1.f;

      findCompandValues(coeffs,numXBlocks*numZBlocks,ranges,compandV,compandH,compandD);

      //COMPRESS DATA INTO OUR DXT5 TEXTURE
      rangeD=ranges[0]=1/((ranges[0])*compandD);
      rangeH=ranges[1]=1/((ranges[1])*compandH);
      rangeV=ranges[2]=1/((ranges[2])*compandV);

      if(axis==1)
         rangeD=ranges[0]=0;
   }


   CompressToDXTN(coeffs, numXBlocks,numZBlocks,rangeH,rangeV,HVDXTNData);


   for(int i=0;i<numXBlocks;i++)
      for(int j=0;j<numZBlocks;j++)
         if(axis==1)
            DCcoeffs[i*numXBlocks+j]=coeffs[i*numXBlocks+j][0];
         else
            DCcoeffs[i*numXBlocks+j]=compand_RngSqrt(coeffs[i*numXBlocks+j][0],rangeD,maxBits)+maxBits;

   delete [] coeffs;
}

//-----------------------------------------------------------------------------
void  compressTerrainData(const D3DXVECTOR3 *uncompressedPositionData, const D3DXVECTOR3 *uncompressedNormalData, const float *uncompressedAmbientOcclusionData, const int width, const int height,XTDVisual *mVisDat)
{
   int numXBlocks = width>>1;
   int numZBlocks = height>>1;

   //yAxis
   float *yAxis_DCcoeffs=new float[numXBlocks*numZBlocks];
   BDynamicArray<uchar> yAxis_HVDXTNData;
   D3DXVECTOR3 yAxis_ranges;
   compressAxis(1, uncompressedPositionData,width,height,yAxis_DCcoeffs,yAxis_HVDXTNData,yAxis_ranges);

   //xAxis
   float *xAxis_DCcoeffs=new float[numXBlocks*numZBlocks];
   BDynamicArray<uchar> xAxis_HVDXTNData;
   D3DXVECTOR3 xAxis_ranges;
   compressAxis(0, uncompressedPositionData,width,height,xAxis_DCcoeffs,xAxis_HVDXTNData,xAxis_ranges);

   //zAxis
   float *zAxis_DCcoeffs=new float[numXBlocks*numZBlocks];
   BDynamicArray<uchar> zAxis_HVDXTNData;
   D3DXVECTOR3 zAxis_ranges;
   compressAxis(2, uncompressedPositionData,width,height,zAxis_DCcoeffs,zAxis_HVDXTNData,zAxis_ranges);

   // Basis Data
   BDynamicArray<uchar> BasisDXNData1;
   BDynamicArray<uchar> BasisDXNData2;
   BRGBAImage BasisImg1(width, height);
   BRGBAImage BasisImg2(width, height);
   BDXTPacker packer;
   bool success;

   for(int x=0;x<width;x++)
   {
      for(int z=0;z<height;z++)
      {
         int index = x * height + z;
         const D3DXVECTOR3 *normalVec = &uncompressedNormalData[index];
         float ambientOcclusion = uncompressedAmbientOcclusionData[index];
         BRGBAColor color1, color2;

         color1.r = int(((normalVec->x + 1.0f) / 2.0f) * 255.0f);
         color1.g = int(((normalVec->y + 1.0f) / 2.0f) * 255.0f);
         color1.b = 0;
         color1.a = 0;

         color2.r = int(((normalVec->z + 1.0f) / 2.0f) * 255.0f);
         color2.g = int(ambientOcclusion * 255.0f);
         color2.b = 0;
         color2.a = 0;

         BasisImg1(x, z) = color1;
         BasisImg2(x, z) = color2;
      }
   }

   // Compress
   success = packer.pack(BasisImg1, cDXN, cDXTQualityBest, false, false, BasisDXNData1);
   assert(success);

   success = packer.pack(BasisImg2, cDXN, cDXTQualityBest, false, false, BasisDXNData2);
   assert(success);




   //now that we have all of our data compressed, fill it into visDat appropriatly
   mVisDat->header.numXBlocks = numXBlocks;
   mVisDat->header.numZBlocks = numZBlocks;
   mVisDat->header.numXVerts = width;
   mVisDat->header.tileScale =0.25;
   mVisDat->header.yAxis_hvMemSize = yAxis_HVDXTNData.size();
   mVisDat->header.yAxis_dcMemSize = numXBlocks*numZBlocks*sizeof(D3DXFLOAT16);
   mVisDat->header.yAxis_ranges = D3DXVECTOR3(yAxis_ranges.x,yAxis_ranges.y,yAxis_ranges.z);
   mVisDat->header.xzAxis_dcMemSize= numXBlocks*numZBlocks*2;
   mVisDat->header.xAxis_hvMemSize = xAxis_HVDXTNData.size();
   mVisDat->header.zAxis_hvMemSize = zAxis_HVDXTNData.size();
   mVisDat->header.xAxis_ranges = D3DXVECTOR3(xAxis_ranges.x,xAxis_ranges.y,xAxis_ranges.z);
   mVisDat->header.zAxis_ranges = D3DXVECTOR3(zAxis_ranges.x,zAxis_ranges.y,zAxis_ranges.z);
   mVisDat->header.xyBasis_hvMemSize = BasisDXNData1.size();
   mVisDat->header.zwBasis_hvMemSize = BasisDXNData2.size();

   //yaxis
   mVisDat->yAxis.dCValues = new D3DXFLOAT16[numXBlocks*numZBlocks];
   D3DXFloat32To16Array(mVisDat->yAxis.dCValues,yAxis_DCcoeffs,mVisDat->header.numXBlocks*mVisDat->header.numZBlocks);
   tileCopyData(mVisDat->yAxis.dCValues, mVisDat->yAxis.dCValues,numXBlocks,numZBlocks,cDXTInvalid,2);

   mVisDat->yAxis.hvCoeffs = new uchar[yAxis_HVDXTNData.size()];
   tileCopyData(mVisDat->yAxis.hvCoeffs,(uchar*)&yAxis_HVDXTNData[0],numXBlocks,numZBlocks,cDXN,0);


   //xzaxis
   mVisDat->xzAxis.dCValues = new uchar[numXBlocks*numZBlocks*2];
   memset(mVisDat->xzAxis.dCValues,0,numXBlocks*numZBlocks*2);
   int c=0;
   for(int i=0;i<numXBlocks*numZBlocks;i++)
   {
      mVisDat->xzAxis.dCValues[c]    = xAxis_DCcoeffs[i];
      mVisDat->xzAxis.dCValues[c+1]  = zAxis_DCcoeffs[i];
      c+=2;
   }
   tileCopyData(mVisDat->xzAxis.dCValues, mVisDat->xzAxis.dCValues,numXBlocks,numZBlocks,cDXTInvalid,2);

   mVisDat->xzAxis.XhvCoeffs = new uchar[xAxis_HVDXTNData.size()];
   tileCopyData(mVisDat->xzAxis.XhvCoeffs,(uchar*)&xAxis_HVDXTNData[0],numXBlocks,numZBlocks,cDXN,0);

   mVisDat->xzAxis.ZhvCoeffs = new uchar[zAxis_HVDXTNData.size()];
   tileCopyData(mVisDat->xzAxis.ZhvCoeffs,(uchar*)&zAxis_HVDXTNData[0],numXBlocks,numZBlocks,cDXN,0);


   //basis
   mVisDat->basis.xyValues = new uchar[BasisDXNData1.size()];
   tileCopyData(mVisDat->basis.xyValues,(uchar*)&BasisDXNData1[0],width,height,cDXN,0);
   mVisDat->basis.zwValues = new uchar[BasisDXNData2.size()];
   tileCopyData(mVisDat->basis.zwValues,(uchar*)&BasisDXNData2[0],width,height,cDXN,0);


   delete [] yAxis_DCcoeffs;
   delete [] xAxis_DCcoeffs;
   delete [] zAxis_DCcoeffs;
   yAxis_HVDXTNData.clear();
   xAxis_HVDXTNData.clear();
   zAxis_HVDXTNData.clear();
   BasisDXNData1.clear();
   BasisDXNData2.clear();
}

//-------------------------------------------------------------------------------------------
void endSwapCompressedTerrainData(XTDVisual *visDat)
{
   //
   //swap all the bits to be Endian friendly
   //


   //yaxis
   int count = visDat->header.numXBlocks * visDat->header.numZBlocks;
   D3DXFLOAT16 *fd= visDat->yAxis.dCValues;
   for(int i=0;i<count;i++)
      fd[i]=endSwapW16((unsigned short*)&fd[i]);

   count = visDat->header.yAxis_hvMemSize / sizeof(WORD);
   WORD *dd= (WORD*)visDat->yAxis.hvCoeffs;
   for(int i=0;i<count;i++)
      dd[i] = endSwapW(dd[i]);


   //xzAxis
   count = visDat->header.xzAxis_dcMemSize / sizeof(WORD);
   dd= (WORD*)visDat->xzAxis.dCValues;
   for(int i=0;i<count;i++)
      dd[i] = endSwapW(dd[i]);

   count = visDat->header.xAxis_hvMemSize / sizeof(WORD);
   dd= (WORD*)visDat->xzAxis.XhvCoeffs;
   for(int i=0;i<count;i++)
      dd[i] = endSwapW(dd[i]);

   count = visDat->header.zAxis_hvMemSize / sizeof(WORD);
   dd= (WORD*)visDat->xzAxis.ZhvCoeffs;
   for(int i=0;i<count;i++)
      dd[i] = endSwapW(dd[i]);


   //basis
   count = visDat->header.xyBasis_hvMemSize / sizeof(WORD);
   dd= (WORD*)visDat->basis.xyValues;
   for(int i=0;i<count;i++)
      dd[i] = endSwapW(dd[i]);

   count = visDat->header.zwBasis_hvMemSize / sizeof(WORD);
   dd= (WORD*)visDat->basis.zwValues;
   for(int i=0;i<count;i++)
      dd[i] = endSwapW(dd[i]);


   //header
   visDat->header.numZBlocks      = endSwapI(visDat->header.numZBlocks);
   visDat->header.numXBlocks      = endSwapI(visDat->header.numXBlocks);
   visDat->header.numXVerts       = endSwapI(visDat->header.numXVerts);
   visDat->header.tileScale       = endSwapF32(visDat->header.tileScale);
   visDat->header.yAxis_hvMemSize = endSwapI(visDat->header.yAxis_hvMemSize);
   visDat->header.yAxis_dcMemSize = endSwapI(visDat->header.yAxis_dcMemSize);
   visDat->header.xAxis_hvMemSize = endSwapI(visDat->header.xAxis_hvMemSize);
   visDat->header.zAxis_hvMemSize = endSwapI(visDat->header.zAxis_hvMemSize);
   visDat->header.xzAxis_dcMemSize = endSwapI(visDat->header.xzAxis_dcMemSize);
   visDat->header.yAxis_ranges     = D3DXVECTOR3(0,endSwapF32(visDat->header.yAxis_ranges.y),endSwapF32(visDat->header.yAxis_ranges.z));
   visDat->header.xAxis_ranges     = D3DXVECTOR3(endSwapF32(visDat->header.xAxis_ranges.x),endSwapF32(visDat->header.xAxis_ranges.y),endSwapF32(visDat->header.xAxis_ranges.z));
   visDat->header.zAxis_ranges     = D3DXVECTOR3(endSwapF32(visDat->header.zAxis_ranges.x),endSwapF32(visDat->header.zAxis_ranges.y),endSwapF32(visDat->header.zAxis_ranges.z));
   visDat->header.xyBasis_hvMemSize = endSwapI(visDat->header.xyBasis_hvMemSize);
   visDat->header.zwBasis_hvMemSize = endSwapI(visDat->header.zwBasis_hvMemSize);


}
//-------------------------------------------------------------------------------------------


extern "C" __declspec(dllexport) int compressTerrainDataToMemory(const D3DXVECTOR3 *relPos, const D3DXVECTOR3 *normals, const float *ambientOcclusion, int numXVerts, XTDVisual *mVisDat)
{
   
   //write our compressed terrain data first
   compressTerrainData(relPos,normals,ambientOcclusion,numXVerts,numXVerts,mVisDat);


   int totalSize = 
         mVisDat->header.yAxis_hvMemSize+
         mVisDat->header.yAxis_dcMemSize+
         mVisDat->header.xzAxis_dcMemSize+
         mVisDat->header.xAxis_hvMemSize+
         mVisDat->header.zAxis_hvMemSize+
         mVisDat->header.xyBasis_hvMemSize+
         mVisDat->header.zwBasis_hvMemSize;

   //do our endian swapping here..
   endSwapCompressedTerrainData(mVisDat);
   

   return totalSize;
}

extern "C" __declspec(dllexport) void cleanCompressedTerrain(XTDVisual *mVisDat)
{
   delete [] mVisDat->yAxis.dCValues;
   delete [] mVisDat->yAxis.hvCoeffs;
   delete [] mVisDat->xzAxis.dCValues;
   delete [] mVisDat->xzAxis.XhvCoeffs;
   delete [] mVisDat->xzAxis.ZhvCoeffs;
   delete [] mVisDat->basis.xyValues;
   delete [] mVisDat->basis.zwValues;
}*/