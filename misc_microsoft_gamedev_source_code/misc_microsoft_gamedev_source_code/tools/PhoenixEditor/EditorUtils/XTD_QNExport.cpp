#define _WIN32_WINNT 0x500
#include <windows.h>

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

#include "EndSwap.inl"


void tileCopyData(void *dst, const void *src,const int Width,const int Height,const BDXTFormat dxtFormat,const int pixelMemSize);
//-----------------------------------------------------------------------------

//---------------------------------------------------------------------
//---------------------------------------------------------------------
struct BXTDTerrainQuadNodeDesc
{
   int		mMinXTile,mMaxXTile;
   int		mMinZTile,mMaxZTile;

   int		mMinXVert,mMaxXVert;
   int		mMinZVert,mMaxZVert;

   float     m_min[3];
   float     m_max[3];
};
struct BXTDTerrainTexturingRenderData
{
   int type;                  
   int baseIndx;               
   int mIndxTexture;
   int mAlphaTexture;
};
struct BXTDTerrainQuadNodeRenderData
{
   float                   minX, minZ;
};
struct BXTDTerrainRenderPacket
{
   BXTDTerrainTexturingRenderData      *mTexturingData;
   BXTDTerrainQuadNodeRenderData       *mQNData;
};

class BXTDTerrainQuadNode
{
public:
   void traverseTest()
   {
      if(m_kids)
      {
         for(int i=0;i<4;i++)
            m_kids[i].traverseTest();
      }
   }

   BXTDTerrainQuadNodeDesc       mDesc;

   BXTDTerrainRenderPacket       *mRenderPacket;

   BXTDTerrainQuadNode		      *m_kids;
};
//---------------------------------------------------------------------
//---------------------------------------------------------------------
//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------
///-------------------------
struct QNTexInfo
{
   int texType;
   int baseTex;
   int indexTexMemLoc;
   int alphaTexMemLoc;
   int indexTexMemSize;
   int alphaTexMemSize;
};



//-------------------------------------------------------
extern "C" __declspec(dllexport) bool evalQNTexture( const WORD *bits, const WORD *alphabits, const int texWidth, const int texHeight, QNTexInfo &qnInfo)
{

   //histogram the data
   int rHisto[16];
   int gHisto[16];
   int bHisto[16];
   int arHisto[16];
   int agHisto[16];
   for(int i=0;i<16;i++)
      rHisto[i]=gHisto[i]=bHisto[i]=arHisto[i]=agHisto[i]=0;


   int width = texWidth;
   int height = texHeight;

   int maxVal = width*height;

   for(int i=0;i<maxVal;i++)
   {
      WORD val = bits[i];
      WORD aval = alphabits[i];

      char sr = (val & 0x0F00)>>8;
      char sg = (val & 0x00F0)>>4;
      char sb = (val & 0x000F);

      char srA = (aval & 0x0F00)>>8;
      char sgA = (aval & 0x00F0)>>4;

      rHisto[sr]++;
      gHisto[sg]++;
      bHisto[sb]++;
      arHisto[srA]++;
      agHisto[sgA]++;
   }

   //0 | 1
   if(agHisto[15]==maxVal || agHisto[0]==maxVal)
   {
      qnInfo.texType=1;

      //type 0 will have a single texture @ maxVal
      for(int k=0;k<maxVal;k++)
      {
         if(bHisto[k]> (maxVal*0.8))
         {
            qnInfo.texType=0;
            qnInfo.baseTex=k;
            qnInfo.alphaTexMemLoc=0;
            qnInfo.alphaTexMemSize=0;
            qnInfo.indexTexMemLoc=0;
            qnInfo.indexTexMemSize=0;
            break;
         }
      }

      //type 1 has no blending, but may contain many texture switches (ie a road across a desert)
      //Assert here : this doesn't seem likely...
      assert(qnInfo.texType==0);
   }


   //2 - 2 textures, 1 blend
   if(arHisto[0]==maxVal && (agHisto[15]!=maxVal && agHisto[0]!=maxVal))
   {
      qnInfo.texType=2;
      qnInfo.baseTex=0;
   }

   //3 - 3 textures, 2 blends
   if(arHisto[0]!=maxVal && (agHisto[15]!=maxVal && agHisto[0]!=maxVal))
   {
      qnInfo.texType=3;
      qnInfo.baseTex=0;
   }

   return (qnInfo.texType>1)?true:false;
}


//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------
//----------------------------------------
#if 0
bool convertQNTextures(const WORD *bits,
                       const int texWidth, const int texHeight, 
                       unsigned char *outBits, bool isAlpha,
                       int texType)
{ 
   if(texType==0 || texType==1)   return false;

   int width = texWidth;
   int height = texHeight;


   if(!outBits)
      return false;

   int bitsSize=0;

   int memLoc = bitsSize;
   if(texType==2)//2=2 blended textures (1 alpha(DXT5A), 2 textures(r8) )
   {
      if(!isAlpha)
      {//INDEX
         bitsSize = width*height*2;

         int c=0;
         for(int i=0;i<width*height;i++)
         {
            WORD val = bits[i];
            char sg = (val & 0x00F0)>>4;
            char sb = (val & 0x000F);

            outBits[c] =  sg;
            outBits[c+1] =  sb;
            c+=2;
         }
         MessageBox(NULL,"2I_DataCompacted","",MB_OK);
         //tileCopyData(outBits, outBits,width,height,cDXTInvalid,2);
         //  MessageBox(NULL,"2I_TileCopied","",MB_OK);

         /*   int count = bitsSize / sizeof(WORD);
         WORD* dd= (WORD*)outBits;
         for(int i=0;i<count;i++)
         dd[i] = endSwapW(dd[i]);
         MessageBox(NULL,"2I_DataSwizzled","",MB_OK);
         */
      }
      else if(isAlpha)
      {  //ALPHA

         //this needs to be compressed to DXT5A
         bitsSize = width*height;

         for(int i=0;i<width*height;i++)
         {
            WORD val = bits[i];
            char sg = (val & 0x00F0)>>4;
            sg=(float(sg) /15.f)*255;
            outBits[i] =  sg;
         } 
         MessageBox(NULL,"2A_DataCompacted","",MB_OK);

         //tileCopyData(outBits, outBits,width,height,cDXTInvalid,1);
      }
   }
   else if(texType==3)//2=3 blended textures (2 alpha(DXTN), 3 textures(a4r4g4b4) )
   {
      if(!isAlpha)
      {//INDEX
         bitsSize = width*height*sizeof(WORD);
         memcpy(outBits,bits,bitsSize);
         // tileCopyData(outBits, bits,width,height,cDXTInvalid,sizeof(WORD));
         MessageBox(NULL,"3I_TileCopied","",MB_OK);


         int count = bitsSize / sizeof(WORD);
         WORD* dd= (WORD*)outBits;
         for(int i=0;i<count;i++)
            dd[i] = endSwapW(dd[i]);
         MessageBox(NULL,"3I_EndSwapped","",MB_OK);

      }
      else if(isAlpha)
      { //ALPHA
         //this needs to be compressed to DXTN
         /*
         BDynamicArray<uchar> DXTN;
         BRGBAImage *XDispImg=new BRGBAImage(width, height);
         {
         int pitch = width*sizeof(WORD);

         for(int i=0;i<width;i++)
         {
         for(int j=0;j<height;j++)
         {
         WORD aval = alphabits[j*pitch+i];
         char srA = (aval & 0x0F00)>>8;
         char sgA = (aval & 0x00F0)>>4;

         srA = (float(srA) /15.f)*255;
         sgA = (float(srA) /15.f)*255;

         (*XDispImg)(i, j) = BRGBAColor(srA,sgA,0,0);
         }
         }

         BDXTPacker packer;
         packer;
         bool success = packer.pack((*XDispImg), cDXN, cDXTQualityBest, false, false, DXTN);
         assert(success);
         delete XDispImg;
         }

         outNode->alphaMemSize = DXTN.size();//width*height*sizeof(WORD);
         outNode->alphaTexture = new unsigned char[outNode->alphaMemSize];
         memcpy(outNode->alphaTexture,(unsigned char *)&DXTN[0],DXTN.size());
         //tileCopyData(outNode->alphaTexture, (unsigned char *)&DXTN[0],width,height,0,cDXN,0);
         DXTN.clear();
         */

         bitsSize = width*height*sizeof(WORD);
         memcpy(outBits,bits,bitsSize);
         // tileCopyData(outBits, bits,width,height,cDXTInvalid,sizeof(WORD));
         MessageBox(NULL,"3A_TileCopied","",MB_OK);


         int count = bitsSize / sizeof(WORD);
         WORD* dd= (WORD*)outBits;
         for(int i=0;i<count;i++)
            dd[i] = endSwapW(dd[i]);
         MessageBox(NULL,"3A_EndSwapped","",MB_OK);

      }

   }


   return true;
}
#endif