// local
#include "terrainPCH.h"
#include "TerrainDeformer.h"
#include "TerrainHeightField.h"

//--------------------------------------
BTerrainDeformer gTerrainDeformer;
//============================================================================
// BTerrainDeformer::BTerrainDeformer
//============================================================================
BTerrainDeformer::BTerrainDeformer()
{
} 
//============================================================================
// BTerrainDeformer::~BTerrainDeformer
//============================================================================
BTerrainDeformer::~BTerrainDeformer()
{
}

//============================================================================
// BTerrainDeformer::init
//============================================================================
bool BTerrainDeformer::init(void)
{
   ASSERT_MAIN_THREAD

    
   commandListenerInit();
  
   return true;
}
//============================================================================
// BTerrainDeformer::deinit
//============================================================================
bool BTerrainDeformer::deinit(void)
{
   ASSERT_MAIN_THREAD

      // Block for safety. 
   gRenderThread.blockUntilGPUIdle();

   commandListenerDeinit();


   return true;
}
//============================================================================
// BTerrainDeformer::initDeviceData
//============================================================================
void BTerrainDeformer::initDeviceData(void)
{
}
//============================================================================
// BTerrainDeformer::deinitDeviceData
//============================================================================
void BTerrainDeformer::deinitDeviceData(void)
{
}
//============================================================================
// BTerrainDeformer::processCommand
//============================================================================
void BTerrainDeformer::processCommand(const BRenderCommandHeader& header, const unsigned char* pData)
{
}
//============================================================================
// BTerrainDeformer::packPosToVisualFmt
//============================================================================
DWORD BTerrainDeformer::packPosToVisualFmt(float x, float y, float z)
{ 
   XMVECTOR                  vMin= gTerrainVisual.getPosMin();
   XMVECTOR                  vRange= gTerrainVisual.getPosRange();

   DWORD outval=0;
   //X = 11bits
   //Y = 11bits
   //Z = 10bits
   const int bitMax10 = 1023; //0x3ff
   //const int bitMax9 = 511;   //0x1ff;


   //scale based upon range
   ushort range = (ushort)(((x+vMin.x) / vRange.x) * (bitMax10));
   ushort output =  (range&bitMax10);
   outval |= output << 22;

   range = (ushort)(((y+vMin.y) / vRange.y) * (bitMax10));
   output = (range&bitMax10);
   outval |= output << 11;

   range = (ushort)(((z+vMin.z) / vRange.z) * (bitMax10));
   output = (range&bitMax10);
   outval |= output;

   return outval;
}
//============================================================================
// BTerrainDeformer::unpackVisualToPos
//============================================================================
void BTerrainDeformer::unpackVisualToPos(DWORD in, float &x, float &y, float &z)
{ 
   XMVECTOR                  vMin= gTerrainVisual.getPosMin();
   XMVECTOR                  vRange= gTerrainVisual.getPosRange();

   //X = 11bits
   //Y = 11bits
   //Z = 10bits
   const int bitMax10 = 1023; //0x3ff
   const float rcpBitMax10 = 1/1023.0f;
   //const int bitMax9 = 511;   //0x1ff;

   ushort sz = (ushort)(in & bitMax10);
   ushort sy = (ushort)((in>>11) & bitMax10);
   ushort sx = (ushort)((in>>22) & bitMax10);

   x = (((sx * rcpBitMax10) * vRange.x)-vMin.x);
   y = (((sy * rcpBitMax10) * vRange.y)-vMin.y);
   z = (((sz * rcpBitMax10) * vRange.z)-vMin.z);

   
}

//============================================================================
// BTerrainDeformer::packNormalToVisualFMT
//============================================================================
DWORD BTerrainDeformer::packNormalToVisualFMT(float nx, float ny, float nz)
{
   DWORD outval = 0;
   //X = 11bits
   //Y = 11bits
   //Z = 10bits
   const int bitMax10 = 1023; //0x3ff
   //const int bitMax9 = 511;   //0x1ff;

   //scale based upon range
   ushort range = (ushort)((((nx + 1) * 0.5f) * (bitMax10)));
   ushort output = (ushort)(range & bitMax10);
   outval |= output << 22;

   range = (ushort)(((ny + 1) * 0.5f) * (bitMax10));
   output = (ushort)(range & bitMax10);
   outval |= output << 11;

   range = (ushort)((((nz + 1) * 0.5f) * (bitMax10)));
   output = (ushort)(range & bitMax10);
   outval |= output;

   
   return outval;
}
//============================================================================
// BTerrainDeformer::unpackVisualToNormal
//============================================================================
void BTerrainDeformer::unpackVisualToNormal(DWORD in, float &nx, float &ny, float &nz)
{
   //X = 11bits
   //Y = 11bits
   //Z = 10bits
   const int bitMax10 = 1023; //0x3ff
   const float rcpBitMax10 = 1/1023.0f;
   //const int bitMax9 = 511;   //0x1ff;

   ushort sz = (ushort)(in & bitMax10);
   ushort sy = (ushort)((in>>11) & bitMax10);
   ushort sx = (ushort)((in>>22) & bitMax10);

   nx = (((sx * rcpBitMax10) * 2.0f)-1.0f);
   ny = (((sy * rcpBitMax10) * 2.0f)-1.0f);
   nz = (((sz * rcpBitMax10) * 2.0f)-1.0f);

}
//============================================================================
// BTerrainDeformer::setDeformedRegionTesselation
//============================================================================
void BTerrainDeformer::setDeformedRegionTesselation(int minx, int maxx, int minz, int maxz, byte tessValue)
{
   ASSERT_RENDER_THREAD

//   int numXVertsPerPatch = 16;

   int minXPatch = minx >>4 ; // DIV 16
   int minZPatch = minz >>4 ; // DIV 16

   int maxXPatch = maxx >>4 ; // DIV 16
   int maxZPatch = maxz >>4 ; // DIV 16

   for(int y = minZPatch; y <= maxZPatch; y++)
   {
      for(int x =  minXPatch; x <= maxXPatch; x++)   
      {
         if(x < 0 || y  < 0 ||
            x >= gTerrainVisual.mNumXPatches || y  >= gTerrainVisual.mNumZPatches)
            continue;

         int patchIndex = x * gTerrainVisual.mNumXPatches + y;
         gTerrainVisual.mpMaxPatchTessallation[patchIndex] = tessValue;
      }
   }
}

//============================================================================
// BTerrainDeformer::flattenTerrainInstant
//============================================================================
void BTerrainDeformer::flattenTerrainInstant(float mMinXPerc,float mMaxXPerc,float mMinZPerc,float mMaxZPerc,float desiredHeight,float mFalloffPerc)
{

   ASSERT_RENDER_THREAD

   int minZ = (int)(gTerrainVisual.getNumXVerts() * mMinXPerc);
   int maxZ = (int)(gTerrainVisual.getNumXVerts() * mMaxXPerc);
   int minX = (int)(gTerrainVisual.getNumXVerts() * mMinZPerc);
   int maxX = (int)(gTerrainVisual.getNumXVerts() * mMaxZPerc);

   int xLen = maxX - minX;
   int zLen = maxZ - minZ;

   int xCenter = minX + (xLen>>1);
   int zCenter = minZ + (zLen>>1);

   int xFalloffStart = (int)((xLen>>1) * (1.0f-mFalloffPerc));
   int xFalloffLen = (int)((xLen>>1) * mFalloffPerc);
   int zFalloffStart = (int)((zLen>>1) * (1.0f-mFalloffPerc));
   int zFalloffLen = (int)((zLen>>1) * mFalloffPerc);
   

   BTerrain3DSimpleVisualPacket *terrDat = gTerrainVisual.getVisData();

   unsigned char* pTerrainMemStart = reinterpret_cast<unsigned char*>(terrDat->mpPhysicalMemoryPointer);
//-- FIXING PREFIX BUG ID 6959
   const unsigned char* pNormalsMemStart = pTerrainMemStart + terrDat->mPositionsSize;
//--


   DWORD *cachedPositions = reinterpret_cast<DWORD*>(GPU_CONVERT_CPU_TO_CPU_CACHED_READONLY_ADDRESS(pTerrainMemStart));//(terrDat->mpPhysicalMemoryPointer);
   DWORD *cachedNormals = reinterpret_cast<DWORD*>(GPU_CONVERT_CPU_TO_CPU_CACHED_READONLY_ADDRESS(pNormalsMemStart));//(terrDat->mpPhysicalMemoryPointer);

   
   unsigned char* pTerrain = reinterpret_cast<unsigned char*>(terrDat->mpPhysicalMemoryPointer);
   unsigned char* pNormals = pTerrain+terrDat->mPositionsSize;
   DWORD *positions = reinterpret_cast<DWORD*>(pTerrain);
   DWORD *normals = reinterpret_cast<DWORD*>(pNormals);
   

   for(int y = minZ; y < maxZ; y++)
   {
      for(int x =  minX; x < maxX; x++)   
      {
         if(x < 0 || y  < 0 ||
            x >= gTerrainVisual.getNumXVerts() || y  >= gTerrainVisual.getNumXVerts())
            continue;

         uint index = XGAddress2DTiledOffset(x, y, gTerrainVisual.getNumXVerts(), sizeof(DWORD));

         int xDiff = x-xCenter;
         int distX = (int)Math::fSqrt((float)(xDiff*xDiff));
         float inflPercX  = 1;
         if(distX>=xFalloffStart)
            inflPercX = Math::Clamp<float>(1.0f-((distX-xFalloffStart)/(float)xFalloffLen),0,1);


         int zDiff = y-zCenter;
         int distZ = (int)Math::fSqrt((float)(zDiff*zDiff));
         float inflPercZ  = 1;
         if(distZ >= zFalloffStart)
            inflPercZ = Math::Clamp<float>(1.0f-((distZ-zFalloffStart)/(float)zFalloffLen),0,1);
         
         float inflPerc = Math::Min<float>(inflPercX,inflPercZ);

         if(inflPerc < 1.0f)
         {
            XMFLOAT3 pv;
            unpackVisualToPos(cachedPositions[index],pv.x,pv.y,pv.z);
            positions[index] = packPosToVisualFmt(0,(inflPerc*desiredHeight) + ((1-inflPerc)*pv.y),0);

            unpackVisualToNormal(cachedNormals[index],pv.x,pv.y,pv.z);
            normals[index] = packNormalToVisualFMT(0,1,0);
         }
         else
         {
            positions[index] = packPosToVisualFmt(0,desiredHeight,0);
            normals[index] = packNormalToVisualFMT(0,1,0);
            //terrDat->mpTerrainAO[index] = 1;
         }
      }
   }

   int totalSize = terrDat->mPositionsSize + terrDat->mNormalsSize;
   BD3D::mpDev->InvalidateGpuCache(terrDat->mpPhysicalMemoryPointer,totalSize,0);

   setDeformedRegionTesselation(minX,maxX,minZ,maxZ,2);
   gTerrainHeightField.flattenAreaInstant(mMinXPerc,mMaxXPerc,mMinZPerc,mMaxZPerc,desiredHeight);

  

}

//============================================================================
// BTerrainDeformer::queueFlattenCommand
//============================================================================
void BTerrainDeformer::queueFlattenCommand(float mMinXPerc,float mMaxXPerc,float mMinZPerc,float mMaxZPerc,float mDesiredHeight,float mFalloffPerc)
{
   ASSERT_RENDER_THREAD
   deformPacket packet;
   packet.mMinXPerc = mMinXPerc;
   packet.mMaxXPerc = mMaxXPerc;
   packet.mMinZPerc = mMinZPerc;
   packet.mMaxZPerc = mMaxZPerc;
   packet.mDesiredHeight = mDesiredHeight;
   packet.mFalloffPerc = mFalloffPerc;

   mQueuedFlattenPackets.add(packet);
}
//============================================================================
// BTerrainDeformer::flushQueuedFlattenCommands
//============================================================================
void BTerrainDeformer::flushQueuedFlattenCommands()
{
   ASSERT_RENDER_THREAD
   

      if(!mQueuedFlattenPackets.size())
         return;

      //CLM added because this is unsafe.
      gRenderThread.blockUntilGPUIdle();

      for(uint i=0;i<mQueuedFlattenPackets.size();i++)
      {
         flattenTerrainInstant(mQueuedFlattenPackets[i].mMinXPerc,
                               mQueuedFlattenPackets[i].mMaxXPerc,
                               mQueuedFlattenPackets[i].mMinZPerc,
                               mQueuedFlattenPackets[i].mMaxZPerc,
                               mQueuedFlattenPackets[i].mDesiredHeight,
                               mQueuedFlattenPackets[i].mFalloffPerc);
      }
      mQueuedFlattenPackets.clear();
}