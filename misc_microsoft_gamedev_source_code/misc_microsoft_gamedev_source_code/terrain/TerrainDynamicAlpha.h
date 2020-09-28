//============================================================================
//
//  TerrainDynamicAlpha.h
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#pragma once

// local
#include "terrain.h"

// xrender
#include "rendercommand.h"
#include "effect.h"

// xcore
#include "threading\eventDispatcher.h"
//-----------------------------------
//-----------------------------------
class dynamicAlphaPacket
{
public:
   int   mAlphaShape;         // 0 = rectangle Oriented, 1 = circle, 2 = rectangle aligned
   float mX0;   float mY0;
   float mX1;   float mY1;
   float mX2;   float mY2;
   float mX3;   float mY3; //mY3 holds circle radius
   byte  mAlphaToState;    //are we adding, or removing alpha
};
//-----------------------------------
class BTerrainDynamicAlpha : public BRenderCommandListener
{
public:
   BTerrainDynamicAlpha();
   ~BTerrainDynamicAlpha();


   bool           init();
   bool           deinit();

   void           destroy();

   //called from simthread
   void           setToRegionToValueRectangleOriented(float x0, float y0, float x1, float y1, float x2, float y2,float x3, float y3, bool value);
   void           setToRegionToValueRectangleAligned(float minX, float minY, float maxX, float maxY, bool value);
   void           setToRegionToValueCircle(float centerX, float centerY, float radius, bool value);

   IDirect3DTexture9    *getDynamicAlphaTexture(){return mpDynamicAlphaTexture;};
   uint           getDynamicAlphaTextureWidth(){return mWidth;}
   uint           getDynamicAlphaTextureHeight(){return mHeight;}

   void           createAlphaTexture();

   

private:
   uint                        mWidth;
   uint                        mHeight;
   IDirect3DTexture9*          mpDynamicAlphaTexture;
 
   // this is a handler to the below operations
   void                       setToRegionToValueInternal(const dynamicAlphaPacket *packet);

   //these systems all expect the input coordinates to be normalized [0,1]. Conversion to the resolution of the image is done internally.
   void                       setAlignedRectToValueInternal(__int64* pDat,float minX, float maxX, float minZ, float maxZ, bool value);
   void                       setOrientedRectToValueInternal(__int64* pDat,float x0, float y0, float x1, float y1, float x2, float y2,float x3, float y3, bool value);
   void                       setCircleToValueInternal(__int64* pDat,float centerX, float centerY, float radius, bool value);
   void                       setTriangleToValueInternal(__int64* pDat, float x0, float y0, float x1, float y1, float x2, float y2, bool value);

   //this actually sets the value into the given image
   void                       setValueToImage(__int64* pDat,uint x, uint y, bool value);


private:
   enum
   {
      cTDA_Destroy=0,
      cTDA_SetAlpha
   };
   bool                       destroyInternal();
   
   //BRenderCommandListenerInterface
   virtual void               processCommand(const BRenderCommandHeader& header, const unsigned char* pData);  
   virtual void               frameBegin(void){};
   virtual void               frameEnd(void){};
   virtual void               initDeviceData(void);
   virtual void               deinitDeviceData(void);

};

extern BTerrainDynamicAlpha gTerrainDynamicAlpha;