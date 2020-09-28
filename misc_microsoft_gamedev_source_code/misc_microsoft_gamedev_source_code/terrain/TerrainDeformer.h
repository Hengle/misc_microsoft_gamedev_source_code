//============================================================================
//
//  TerrainDeformer.h
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
class deformPacket
{
public:
   float mMinXPerc;      //[0,1] perc on map to make deformation at
   float mMaxXPerc;      //[0,1] perc on map to make deformation at
   float mMinZPerc;      //[0,1] perc on map to make deformation at
   float mMaxZPerc;      //[0,1] perc on map to make deformation at

   float mDesiredHeight; //in worldspace
   float mFalloffPerc; //[0,1]
};
//-----------------------------------
class BTerrainDeformer : public BRenderCommandListener
{
public:
   BTerrainDeformer();
   ~BTerrainDeformer();


   bool           init();
   bool           deinit();


   //these functions will deform the terrain based upon specific input

   //instant functions will wait until the next available free frame and deform the terrain to the specific height
   void           queueFlattenCommand(float mMinXPerc,float mMaxXPerc,float mMinZPerc,float mMaxZPerc,float mDesiredHeight,float mFalloffPerc);
   void           flushQueuedFlattenCommands();

private:

   BDynamicArray<deformPacket> mQueuedFlattenPackets;
   //These two functions will take in the specific data that the textures in BTerrainVisual
   //stores to render the terrain in the vertex shader. The returned format greatly depends on the 
   //texture types defined by the rendering setup in BTerrainVisual, and gpuTerrainVS.inc
   DWORD          packPosToVisualFmt(float x, float y, float z);
   DWORD          packNormalToVisualFMT(float nx, float ny, float nz);

   void           unpackVisualToPos(DWORD in, float &x, float &y, float &z);
   void           unpackVisualToNormal(DWORD in, float &nx, float &ny, float &nz);

   void           setDeformedRegionTesselation(int minx, int maxx, int minz, int maxz, byte tessValue);
   void           flattenTerrainInstant(float mMinXPerc,float mMaxXPerc,float mMinZPerc,float mMaxZPerc,float mDesiredHeight,float mFalloffPerc);

   //BRenderCommandListenerInterface
   virtual void               processCommand(const BRenderCommandHeader& header, const unsigned char* pData);  
   virtual void               frameBegin(void){};
   virtual void               frameEnd(void){};
   virtual void               initDeviceData(void);
   virtual void               deinitDeviceData(void);

};
//-----------------------------------
extern BTerrainDeformer gTerrainDeformer;