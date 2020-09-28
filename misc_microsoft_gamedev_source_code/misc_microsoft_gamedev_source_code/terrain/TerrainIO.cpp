//============================================================================
//
//  TerrainIO.cpp
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#include "terrainPCH.h"
#include "terrain.h"
#include "TerrainIO.h"
#include "TerrainVisual.h"
#include "TerrainQuadNode.h"
#include "TerrainTexturing.h"
#include "TerrainMetric.h"
#include "TerrainRender.h"
#include "TerrainHeightField.h"
#include "RoadManager.h"
#include "Terrainfoliage.h"
#include "TerrainDynamicAlpha.h"

// xcore
#include "consoleOutput.h"
#include "resource\ecfUtils.h"

// xsystem
#include "bfileStream.h"

// xrender
#include "renderThread.h"
#include "bd3d.h"
#include "renderDraw.h"

// xgame
#include "gamedirectories.h"

//CLM HACKATRON
bool skipSpecMapTextureCache = true;
bool skipSelfTextureCache = true;
bool skipEnvMapTextureCache = true;
//============================================================================
// BTerrainIOLoader::BTerrainIOLoader
//============================================================================
BTerrainIOLoader::BTerrainIOLoader():
   mCommandListenerHandle(cInvalidCommandListenerHandle),
   mDevicePtrCounter(0)
{ 
}

//============================================================================
// BTerrainIOLoader::~BTerrainIOLoader
//============================================================================
BTerrainIOLoader::~BTerrainIOLoader()
{
}

//============================================================================
// BTerrainIOLoader::init
//============================================================================
bool  BTerrainIOLoader::init()
{
   ASSERT_MAIN_THREAD
   
   if (mCommandListenerHandle == cInvalidCommandListenerHandle)
   {
      mCommandListenerHandle = gRenderThread.registerCommandListener(this);
   }

#ifdef ENABLE_RELOAD_MANAGER
   eventReceiverInit(cThreadIndexRender);
#endif
   
   return true;
}
//============================================================================
// BTerrainIOLoader::deinit
//============================================================================
bool  BTerrainIOLoader::deinit()
{
   ASSERT_MAIN_THREAD
   
   if (mCommandListenerHandle != cInvalidCommandListenerHandle)
   {
      gRenderThread.freeCommandListener(mCommandListenerHandle);
      mCommandListenerHandle = cInvalidCommandListenerHandle;
   }

#ifdef ENABLE_RELOAD_MANAGER
   gReloadManager.deregisterClient(mEventHandle);
   eventReceiverDeinit();
#endif

   Utils::ClearObj(mLastLoadParams);

   return true;
}
//============================================================================
// BTerrainIOLoader::load
//============================================================================
void BTerrainIOLoader::load(long dirID, const char *filename, long terrainDirID, bool loadVisRep)
{      
   gConsoleOutput.resource("Loading terrain: %s", filename);
   
   mLastLoadParams.mDirID=dirID;
   mLastLoadParams.mFilename = filename;
   mLastLoadParams.mTerrainDirID = terrainDirID;
   mLastLoadParams.mLoadVisRep = loadVisRep;
   
   gRenderThread.submitCommand(mCommandListenerHandle, cTLLoad, mLastLoadParams);
   gRenderThread.kickCommands();

   reloadInit(mLastLoadParams);
}


int convMirrorCoordToOrigCoord(int mirrorIndex, int numXChunks)
{
   int mirrorx = mirrorIndex / (numXChunks*3);
   int mirrorz = mirrorIndex % (numXChunks*3);

   int origx = ((mirrorx / numXChunks) % 2) ? mirrorx % numXChunks : (numXChunks - 1) - mirrorx % numXChunks;
   int origz = ((mirrorz / numXChunks) % 2) ? mirrorz % numXChunks : (numXChunks - 1) - mirrorz % numXChunks;

   return(origx * numXChunks + origz);
}
int convMirrorCoordToQuadrant(int mirrorIndex, int numXChunks)
{
   int mirrorx = mirrorIndex / (numXChunks*3);
   int mirrorz = mirrorIndex % (numXChunks*3);

   int quadx = mirrorx / numXChunks;
   int quadz = mirrorz / numXChunks;

   int quadrant = -1;
   if((quadx == 0) && (quadz == 0))
   {
      quadrant = 0;
   }
   else if((quadx == 0) && (quadz == 1))
   {
      quadrant = 1;
   }
   else if((quadx == 0) && (quadz == 2))
   {
      quadrant = 2;
   }
   else if((quadx == 1) && (quadz == 0))
   {
      quadrant = 7;
   }
   else if((quadx == 1) && (quadz == 1))
   {
      quadrant = -1;
   }
   else if((quadx == 1) && (quadz == 2))
   {
      quadrant = 3;
   }
   else if((quadx == 2) && (quadz == 0))
   {
      quadrant = 6;
   }
   else if((quadx == 2) && (quadz == 1))
   {
      quadrant = 5;
   }
   else if((quadx == 2) && (quadz == 2))
   {
      quadrant = 4;
   }
   else
   {
      assert(0);
   }


   return(quadrant);
}

//============================================================================
// BTerrainIOLoader::loadXTDInternal
//============================================================================
bool BTerrainIOLoader::loadXTDInternal(const BLoadParams *pParams, bool loadVisRep)
{
   //SCOPEDSAMPLE(BTerrainIOLoader_loadXTDInternal)
#define TerrLoadError()\
   gConsoleOutput.output(cMsgError, "%s", buf);\
   gTerrain.setLoadPending(false);\
   return false;

   TRACEMEM
   
   char buf[256];

   //get our XTD name
   BString path(pParams->mFilename);
   path += ".xtd";
   gConsoleOutput.resource("BTerrainIOLoader::loadXTDInternal: %s", path.getPtr());
         
   /////////////////////////////////////////////////////////////////////////////
   ///////////////////////////////////////////////////////////////////
   //CLM CHANGES!

   BFileSystemStream tfile;
   if (!tfile.open(pParams->mDirID, path, cSFReadable | cSFSeekable | cSFDiscardOnClose))
   {
      sprintf_s(buf, sizeof(buf), "BTerrainIOLoader::loadInternal : Unable to open file %s\n", path.getPtr());
      TerrLoadError()
   }

   BECFFileStream ecfReader;
   if(!ecfReader.open(&tfile)) //file checking and version ID done during constructor
   {
      sprintf_s(buf, sizeof(buf), "BTerrainIOLoader::loadInternal : ECFHeader or Checksum invalid  %s\n", path.getPtr());
      TerrLoadError()
   }

   BECFHeader ecfHeader = ecfReader.getHeader();
   int numECFChunks = ecfHeader.getNumChunks();


   //find our XTDChunk header
   int headerIndex = ecfReader.findChunkByID(cXTD_XTDHeader);

   if(headerIndex==-1)
   { 
      sprintf_s(buf, sizeof(buf), "LoadXBOXTerrain : Counld not find XTD Chunk Header\n");
      TerrLoadError()
   }

   ecfReader.seekToChunk(headerIndex);

   XTDHeader xtdHeader;
   ecfReader.getStream()->readBytes((void*)&xtdHeader,sizeof(XTDHeader));

   if(xtdHeader.mVersion!=XTD_VERSION)
   { 
      sprintf_s(buf, sizeof(buf), "LoadXBOXTerrain : XTD file header version (%i) of file %s conflicts desired version (%i). Please re-export\n",xtdHeader.mVersion,path.getPtr(),XTD_VERSION);
      TerrLoadError()
   }

   int numXChunks = xtdHeader.mNumXChunks;

   //initalize our base values from header information
   gTerrainVisual.mTileScale = xtdHeader.mTileScale;
   gTerrainVisual.mNumXVerts = xtdHeader.mNumXVerts;
   gTerrain.mNumXChunks = numXChunks;
   gTerrain.mWorldMin = xtdHeader.worldMin;
   gTerrain.mWorldMax = xtdHeader.worldMax;
   gTerrain.mWorldMaxTotal = gTerrain.mWorldMax+D3DXVECTOR3(gTerrain.mWorldMax.x,0,gTerrain.mWorldMax.z); 
   gTerrain.mWorldMinTotal = gTerrain.mWorldMin-D3DXVECTOR3(gTerrain.mWorldMax.x,0,gTerrain.mWorldMax.z); 
   gTerrainVisual.mOOTileScale = 1.0f / xtdHeader.mTileScale;
  
   if(!loadVisRep)
   {
      tfile.close();
      return true;
   }

   //create our 2D array of quadnodes
   gTerrain.mpQuadGrid = new BTerrainQuadNode[numXChunks*numXChunks];

   BTerrainMetrics::addCPUMem(sizeof(BTerrainQuadNode) * numXChunks*numXChunks);

   sprintf_s(buf, sizeof(buf), "LoadXBOXTerrain : Unable to load file %s\n", path.getPtr());

   TRACEMEM

   //INITALIZE OUR HARDWARE TESSELATION WHILE WE'RE HERE
   gTerrainVisual.initTesselatorData(xtdHeader.mNumXVerts, xtdHeader.mNumXVerts);

   TRACEMEM

   //CLEAR OUR ADDITIVE DIFFUSE
   gTerrainVisual.clearLightTex();


   //load each of our terrain chunks
   for(int i=0;i<numECFChunks;i++)
   {
      const BECFChunkHeader& cHeader = ecfReader.getChunkHeader(i);
      const uint64 id = cHeader.getID();
      switch(id)
      {
      case cXTD_TerrainChunk:
         {
            //SCOPEDSAMPLE(BTerrainIOLoader_cXTD_TerrainChunk)
            ecfReader.seekToChunk(i);
            XTDVisualChunkHeader vch;

            ecfReader.getStream()->readObj(vch);

           // int sizeLeft = cHeader.getSize()-sizeof(XTDVisualChunkHeader);

            addVisualChunk(vch,numXChunks);

         }
         break;
      case cXTD_AOChunk:
         {
            //SCOPEDSAMPLE(BTerrainIOLoader_cXTD_AOChunk)
            ecfReader.seekToChunk(i);
            int sizeLeft = cHeader.getSize();

            gTerrainVisual.mVisualData->mpPhysicalMemoryPointerAO = XPhysicalAlloc( sizeLeft, MAXULONG_PTR, 0, PAGE_READWRITE);
            BVERIFY(gTerrainVisual.mVisualData->mpPhysicalMemoryPointerAO);
            if (!ecfReader.getStream()->readBytes(gTerrainVisual.mVisualData->mpPhysicalMemoryPointerAO,sizeLeft))
            {
               XPhysicalFree(gTerrainVisual.mVisualData->mpPhysicalMemoryPointerAO);
               gTerrainVisual.mVisualData->mpPhysicalMemoryPointerAO = NULL;

               gConsoleOutput.output(cMsgError, "%s", buf);
               gTerrain.setLoadPending(false);
               return false;
            }
            BTerrainMetrics::addTerrainGPUMem(sizeLeft);

            // rg [8/19/06] - The CPU's cached view of this memory must be manually flushed, otherwise the GPU could read stale data!
            Utils::FlushCacheLines(gTerrainVisual.mVisualData->mpPhysicalMemoryPointerAO, sizeLeft);

            TRACEMEM


            break;
         }
      case cXTD_AlphaChunk:
         {
            //SCOPEDSAMPLE(BTerrainIOLoader_cXTD_AlphaChunk)
            ecfReader.seekToChunk(i);
            int sizeLeft = cHeader.getSize();

            gTerrainVisual.mVisualData->mpPhysicalMemoryPointerALPHA = XPhysicalAlloc( sizeLeft, MAXULONG_PTR, 0, PAGE_READWRITE);
            BVERIFY(gTerrainVisual.mVisualData->mpPhysicalMemoryPointerALPHA);
            if (!ecfReader.getStream()->readBytes(gTerrainVisual.mVisualData->mpPhysicalMemoryPointerALPHA,sizeLeft))
            {
               XPhysicalFree(gTerrainVisual.mVisualData->mpPhysicalMemoryPointerALPHA);
               gTerrainVisual.mVisualData->mpPhysicalMemoryPointerALPHA = NULL;

               gConsoleOutput.output(cMsgError, "%s", buf);
               gTerrain.setLoadPending(false);
               return false;
            }
            BTerrainMetrics::addTerrainGPUMem(sizeLeft);


            // rg [8/19/06] - The CPU's cached view of this memory must be manually flushed, otherwise the GPU could read stale data!
            Utils::FlushCacheLines(gTerrainVisual.mVisualData->mpPhysicalMemoryPointerALPHA, sizeLeft);

            TRACEMEM

            break;
         }
      case cXTD_AtlasChunk:
         {
            //SCOPEDSAMPLE(BTerrainIOLoader_cXTD_AtlasChunk)
            ecfReader.seekToChunk(i);
  
            int sizeLeft = cHeader.getSize() - (sizeof(XMFLOAT3)*2);

            
            gTerrainVisual.mVisualData = new BTerrain3DSimpleVisualPacket();
            BTerrainMetrics::addCPUMem(sizeof(BTerrain3DSimpleVisualPacket));
            ecfReader.getStream()->readObj(gTerrainVisual.mVisualData->mPosCompMin);
            ecfReader.getStream()->readObj(gTerrainVisual.mVisualData->mPosCompRange);

            gTerrainVisual.mVisualData->mpPhysicalMemoryPointer = XPhysicalAlloc( sizeLeft, MAXULONG_PTR, 0, PAGE_READWRITE);
            BVERIFY(gTerrainVisual.mVisualData->mpPhysicalMemoryPointer);
            if (!ecfReader.getStream()->readBytes(gTerrainVisual.mVisualData->mpPhysicalMemoryPointer,sizeLeft))
            {
               XPhysicalFree(gTerrainVisual.mVisualData->mpPhysicalMemoryPointer);
               gTerrainVisual.mVisualData->mpPhysicalMemoryPointer = NULL;

               gConsoleOutput.output(cMsgError, "%s", buf);
               gTerrain.setLoadPending(false);
               return false;
            }
            BTerrainMetrics::addTerrainGPUMem(sizeLeft);
                        
            // rg [8/19/06] - The CPU's cached view of this memory must be manually flushed, otherwise the GPU could read stale data!
            Utils::FlushCacheLines(gTerrainVisual.mVisualData->mpPhysicalMemoryPointer, sizeLeft);
                        
            TRACEMEM

            break;
         }
      case cXTD_TessChunk:
         {
           //SCOPEDSAMPLE(BTerrainIOLoader_cXTD_TessChunk)
            ecfReader.seekToChunk(i);

            ecfReader.getStream()->readObj(gTerrainVisual.mNumXPatches);
            ecfReader.getStream()->readObj(gTerrainVisual.mNumZPatches);

            int numQuads = gTerrainVisual.mNumXPatches * gTerrainVisual.mNumZPatches;

            int MemSize = numQuads * sizeof(byte);
            gTerrainVisual.mpMaxPatchTessallation = (byte*)XPhysicalAlloc( MemSize, MAXULONG_PTR, 0, PAGE_READWRITE);
            BVERIFY(gTerrainVisual.mpMaxPatchTessallation);
            memset(gTerrainVisual.mpMaxPatchTessallation,0,MemSize);
            

            if (!ecfReader.getStream()->readBytes(gTerrainVisual.mpMaxPatchTessallation,MemSize))
            {
               XPhysicalFree(gTerrainVisual.mpMaxPatchTessallation);
               gTerrainVisual.mpMaxPatchTessallation = NULL;

               gConsoleOutput.output(cMsgError, "%s", buf);
               gTerrain.setLoadPending(false);
               return false;
            }
            BTerrainMetrics::addTesselationGPUMem(MemSize);


            MemSize = numQuads * 2 * sizeof(XMVECTOR);
            gTerrainVisual.mpPatchBBs = (XMVECTOR*)XPhysicalAlloc( MemSize, MAXULONG_PTR, 0, PAGE_READWRITE);
            BVERIFY(gTerrainVisual.mpPatchBBs);
            if (!ecfReader.getStream()->readBytes(gTerrainVisual.mpPatchBBs,MemSize))
            {
               XPhysicalFree(gTerrainVisual.mpPatchBBs);
               gTerrainVisual.mpPatchBBs = NULL;

               gConsoleOutput.output(cMsgError, "%s", buf);
               gTerrain.setLoadPending(false);
               return false;
            }
            BTerrainMetrics::addTesselationGPUMem(MemSize);

            //CLM [05.16.07] This doesn't need to be a physical allocation...
            gTerrainVisual.mpWorkingPatchTess = new float[numQuads];
            BVERIFY(gTerrainVisual.mpWorkingPatchTess);
         //   BTerrainMetrics::addTesselationCPUMem(numQuads*sizeof(float));

            memset(gTerrainVisual.mpWorkingPatchTess,0,numQuads*sizeof(float));
           // memcpy(gTerrainVisual.mpWorkingPatchTess,gTerrainVisual.mpMaxPatchTessallation,numQuads * sizeof(float));

            gTerrainVisual.mMulByXPatchesLookup.resize(gTerrainVisual.mNumZPatches);
            for (int i = 0; i < gTerrainVisual.mNumZPatches; i++)
               gTerrainVisual.mMulByXPatchesLookup[i] = i * gTerrainVisual.mNumZPatches;

            //gTerrainVisual.mTempPatchAreas.resize(gTerrainVisual.mNumZPatches);

            TRACEMEM

            break;
         }
      case cXTD_LightingChunk:
         {
            //SCOPEDSAMPLE(BTerrainIOLoader_cXTD_LightingChunk)
            ecfReader.seekToChunk(i);

            int MemSize = 0;
            ecfReader.getStream()->readObj(MemSize);

            gTerrainVisual.mpPhysicalLightingDataPointer = XPhysicalAlloc( MemSize, MAXULONG_PTR, 0, PAGE_READWRITE);
            BVERIFY(gTerrainVisual.mpPhysicalLightingDataPointer);
            memset(gTerrainVisual.mpPhysicalLightingDataPointer,0,MemSize);

            BTerrainMetrics::addPrecomputedLightingGPU(MemSize);

            if (!ecfReader.getStream()->readBytes(gTerrainVisual.mpPhysicalLightingDataPointer,MemSize))
            {
               XPhysicalFree(gTerrainVisual.mpPhysicalLightingDataPointer);
               gTerrainVisual.mpPhysicalLightingDataPointer = NULL;

               gConsoleOutput.output(cMsgError, "%s", buf);
               gTerrain.setLoadPending(false);
               return false;
            }
            
            gTerrainVisual.convertLightingFromMemory();


            
         }
      }
   }


   tfile.close();
   
   //load all of our visual data
   gTerrainVisual.mVisualData->convertFromMemoryMain();

   calcSkirtData(xtdHeader);
   
   calcGrowDist();

   TRACEMEM
   
   return true;
}
//============================================================================
// BTerrainIOLoader::calcGrowDist
//============================================================================
void BTerrainIOLoader::calcGrowDist()
{
  //SCOPEDSAMPLE(BTerrainIOLoader_calcGrowDist)
   // Determine the bounding box "growth" value that will account for overhangs.
   float growDist = 0.0f;

   for (int x=0;x<gTerrain.mNumXChunks;x++)
   {
      int outerIndx = (x ) * (gTerrain.mNumXChunks );
      for (int z=0;z<gTerrain.mNumXChunks;z++)
      {
         int indx = outerIndx + (z );

         // Get the chunk's actual bounding box.
         const D3DXVECTOR3& boundsMin = gTerrain.mpQuadGrid[indx].getDesc().m_min;
         const D3DXVECTOR3& boundsMax = gTerrain.mpQuadGrid[indx].getDesc().m_max;

         // Calculate the chunk's bounding box size, assuming no XZ displacement.
         D3DXVECTOR3 staticBoundsMin(
            x * gTerrainVisual.getTileScale() * BTerrainQuadNode::getMaxNodeWidth(), 
            0.0f,
            z * gTerrainVisual.getTileScale() * BTerrainQuadNode::getMaxNodeDepth());

         D3DXVECTOR3 staticBoundsMax(
            (x + 1) * gTerrainVisual.getTileScale() * BTerrainQuadNode::getMaxNodeWidth(), 
            0.0f,
            (z + 1) * gTerrainVisual.getTileScale() * BTerrainQuadNode::getMaxNodeDepth());

         // Compare the actual bounding box vs. the chunk's static bounding box.
         growDist = Math::Max(growDist, fabs(boundsMax.x - staticBoundsMax.x));
         growDist = Math::Max(growDist, fabs(staticBoundsMin.x - boundsMin.x));

         growDist = Math::Max(growDist, fabs(boundsMax.z - staticBoundsMax.z));
         growDist = Math::Max(growDist, fabs(staticBoundsMin.z - boundsMin.z));
      }
   }           

   gTerrain.mBoundingBoxGrowDist = growDist;
}
//============================================================================
// BTerrainIOLoader::loadXTTInternal
//============================================================================
void addPoint(D3DXVECTOR3 &min, D3DXVECTOR3 &max, D3DXVECTOR3 point)
{

   if(point.x < min.x)  min.x = point.x;
   if(point.y < min.y)  min.y = point.y;
   if(point.z < min.z)  min.z = point.z;

   if(point.x > max.x)  max.x = point.x;
   if(point.y > max.y)  max.y = point.y;
   if(point.z > max.z)  max.z = point.z;
}


void BTerrainIOLoader::createSkirtChunks(uint minXChunkIndex, uint minZChunkIndex, uint size, const XTDHeader &xtdHeader, BTerrainQuadNode* pOwnerNode)
{
   //calculate the bounding box for this 'chunk' set
   D3DXVECTOR3 min=pOwnerNode->getDesc().m_min;
   D3DXVECTOR3 max=pOwnerNode->getDesc().m_max;
   if(size!=1)
   {
      for(uint mainNodeX = minXChunkIndex; mainNodeX < minXChunkIndex+size; mainNodeX++)
      {
         for(uint mainNodeZ = minZChunkIndex; mainNodeZ < minZChunkIndex+size; mainNodeZ++)
         {
            uint mainNodeIndex = mainNodeX * xtdHeader.mNumXChunks + mainNodeZ;
            addPoint(min,max,gTerrain.mpQuadGrid[mainNodeIndex].mDesc.m_min);
            addPoint(min,max,gTerrain.mpQuadGrid[mainNodeIndex].mDesc.m_max);
         }
      }
   }
   
   for(uint quadrant=0;quadrant<8;quadrant++)
   {
      BTerrainQuadNode btqn;
      btqn.mSkirtInfo.mIsSkirtChunk = true;
      btqn.mSkirtInfo.mQuadrant = quadrant;
      btqn.mSkirtInfo.mpOwnerNode = pOwnerNode;
      btqn.mSkirtInfo.mSkirtBatchSize = size;

      D3DXVECTOR3 lmin=min;
      D3DXVECTOR3 lmax=max;
      D3DXVec3TransformCoord(&lmin, &min, &gTerrain.mQuadrantMatrices[quadrant]);
      D3DXVec3TransformCoord(&lmax, &max, &gTerrain.mQuadrantMatrices[quadrant]);

      if (lmax.x < lmin.x) std::swap(lmax.x, lmin.x);
      if (lmax.y < lmin.y) std::swap(lmax.y, lmin.y);
      if (lmax.z < lmin.z) std::swap(lmax.z, lmin.z);

      btqn.mDesc.m_min = lmin;
      btqn.mDesc.m_max = lmax;

      gTerrain.mQuadSkirtChunkList.push_back(btqn);
   }
}

void BTerrainIOLoader::createBatchedSkirtChunk(uint minXChunkIndex, uint minZChunkIndex, uint maxBatchedChunkSize, const XTDHeader &xtdHeader)
{
   const uint batchedChunkSize=maxBatchedChunkSize;


   bool hybridChunk = (minXChunkIndex==0 || minZChunkIndex==0 || 
                       minXChunkIndex+batchedChunkSize >= (uint)xtdHeader.mNumXChunks ||
                       minZChunkIndex+batchedChunkSize >= (uint)xtdHeader.mNumXChunks);


  

   if(!hybridChunk)
   {
      //easy case!
       uint mainNodeIndex = minXChunkIndex * xtdHeader.mNumXChunks + minZChunkIndex;
      createSkirtChunks(minXChunkIndex,minZChunkIndex,batchedChunkSize,xtdHeader,&gTerrain.mpQuadGrid[mainNodeIndex]);
   }
   else
   {
      //we have to figure out what kind of hybrid setup we have.....

      if(batchedChunkSize==2)
      {
         //this whole 2x2 section MUST be edge nodes to not qualify for above..
         for(uint mainNodeX = minXChunkIndex; mainNodeX < minXChunkIndex+batchedChunkSize; mainNodeX++)
         {
            for(uint mainNodeZ = minZChunkIndex; mainNodeZ < minZChunkIndex+batchedChunkSize; mainNodeZ++)
            {
                uint mainNodeIndex = mainNodeX * xtdHeader.mNumXChunks + mainNodeZ;
               createSkirtChunks(mainNodeX,mainNodeZ,1,xtdHeader,&gTerrain.mpQuadGrid[mainNodeIndex]);
            }
         }
      }
      else if(batchedChunkSize==4)
      {
         //recursivly generate these chunks..
         for(uint mainNodeX = minXChunkIndex; mainNodeX < minXChunkIndex+batchedChunkSize; mainNodeX+=2)
         {
            for(uint mainNodeZ = minZChunkIndex; mainNodeZ < minZChunkIndex+batchedChunkSize; mainNodeZ+=2)
            {
               createBatchedSkirtChunk(mainNodeX,mainNodeZ,2,xtdHeader);
            }
         }
      }
   }
}
void BTerrainIOLoader::calcSkirtData(const XTDHeader &xtdHeader)
{
  //SCOPEDSAMPLE(BTerrainIOLoader_calcSkirtData)
   // Initialize quadrantMatrices
   //
   //    2   3   4
   //    1  -1   5
   //    0   7   6

   float worldSizeX = xtdHeader.mNumXVerts * xtdHeader.mTileScale;
   float worldSizeZ = xtdHeader.mNumXVerts * xtdHeader.mTileScale;

   D3DXMatrixIdentity(&gTerrain.mQuadrantMatrices[0]);
   gTerrain.mQuadrantMatrices[0]._11 = -1;
   gTerrain.mQuadrantMatrices[0]._33 = -1;

   D3DXMatrixIdentity(&gTerrain.mQuadrantMatrices[1]);
   gTerrain.mQuadrantMatrices[1]._11 = -1;

   D3DXMatrixIdentity(&gTerrain.mQuadrantMatrices[2]);
   gTerrain.mQuadrantMatrices[2]._11 = -1;
   gTerrain.mQuadrantMatrices[2]._33 = -1;
   gTerrain.mQuadrantMatrices[2]._43 = worldSizeZ * 2;

   D3DXMatrixIdentity(&gTerrain.mQuadrantMatrices[3]);
   gTerrain.mQuadrantMatrices[3]._33 = -1;
   gTerrain.mQuadrantMatrices[3]._43 = worldSizeZ * 2;

   D3DXMatrixIdentity(&gTerrain.mQuadrantMatrices[4]);
   gTerrain.mQuadrantMatrices[4]._11 = -1;
   gTerrain.mQuadrantMatrices[4]._33 = -1;
   gTerrain.mQuadrantMatrices[4]._41 = worldSizeX * 2;
   gTerrain.mQuadrantMatrices[4]._43 = worldSizeZ * 2;

   D3DXMatrixIdentity(&gTerrain.mQuadrantMatrices[5]);
   gTerrain.mQuadrantMatrices[5]._11 = -1;
   gTerrain.mQuadrantMatrices[5]._41 = worldSizeX * 2;

   D3DXMatrixIdentity(&gTerrain.mQuadrantMatrices[6]);
   gTerrain.mQuadrantMatrices[6]._11 = -1;
   gTerrain.mQuadrantMatrices[6]._33 = -1;
   gTerrain.mQuadrantMatrices[6]._41 = worldSizeX * 2;

   D3DXMatrixIdentity(&gTerrain.mQuadrantMatrices[7]);
   gTerrain.mQuadrantMatrices[7]._33 = -1;


   //CLM [02.21.08] skirt chunks can now be massivly out of scale. We support 1, 2, and 4 batched chunks
   //So I've removed the normal grid and gone to a list of odd shaped objects.
   //In truth, we don't even support maps that don't scale to 4x skirt batching....
   gTerrain.mQuadSkirtChunkList.clear();

   uint maxBatchTarget=4;
   if(xtdHeader.mNumXChunks %4 ==0)
      maxBatchTarget = 4;
   else if(xtdHeader.mNumXChunks %2 ==0)
      maxBatchTarget = 2;
   else
      return;


   for(uint mainNodeX = 0; mainNodeX < (uint)xtdHeader.mNumXChunks; mainNodeX+=maxBatchTarget)
   {
      for(uint mainNodeZ = 0; mainNodeZ < (uint)xtdHeader.mNumXChunks; mainNodeZ+=maxBatchTarget)
      {
         createBatchedSkirtChunk(mainNodeX, mainNodeZ, maxBatchTarget, xtdHeader);
      }
   }
         


   BTerrainMetrics::addCPUMem(sizeof(BTerrainQuadNode) * gTerrain.mQuadSkirtChunkList.size());



}
//============================================================================
// BTerrainIOLoader::loadXTTInternal
//============================================================================
bool BTerrainIOLoader::loadXTTInternal(const BLoadParams *pParams)
{
   //SCOPEDSAMPLE(BTerrainIOLoader_loadXTTInternal)
   skipSelfTextureCache = true;
   skipEnvMapTextureCache = true;

#define TerrLoadError()\
   gConsoleOutput.output(cMsgError, "%s", buf);\
   gTerrain.setLoadPending(false);\
   return false;

   TRACEMEM
   
   char buf[256];

   //   void* pDeviceData;
   //  void* pLogicData;

   //get our XTT name
   BString path(pParams->mFilename);
   path += ".xtt";
   
   gConsoleOutput.resource("BTerrainIOLoader::loadXTTInternal: %s", path.getPtr());

   BFileSystemStream tfile;
   if (!tfile.open(pParams->mDirID, path, cSFReadable | cSFSeekable | cSFDiscardOnClose))
   {
      sprintf_s(buf, sizeof(buf), "BTerrainIOLoader::loadInternal : Unable to open file %s\n", path.getPtr());
      TerrLoadError()
   }

   BECFFileStream ecfReader;
   if(!ecfReader.open(&tfile)) //file checking and version ID done during constructor
   {
      sprintf_s(buf, sizeof(buf), "BTerrainIOLoader::loadInternal : ECFHeader or Checksum invalid  %s\n", path.getPtr());
      TerrLoadError()
   }

   BECFHeader ecfHeader = ecfReader.getHeader();
   int numECFChunks = ecfHeader.getNumChunks();


   //find our XTTChunk header
   int headerIndex = ecfReader.findChunkByID(cXTT_XTTHeader);

   if(headerIndex==-1)
   { 
      sprintf_s(buf, sizeof(buf), "LoadXBOXTerrain : Counld not find XTT Chunk Header\n");
      TerrLoadError()
   }

   ecfReader.seekToChunk(headerIndex);

   XTTHeader xttHeader;
   ecfReader.getStream()->readBytes((void*)&xttHeader,sizeof(XTTHeader));

   if(xttHeader.mVersion!=XTT_VERSION)
   { 
      sprintf_s(buf, sizeof(buf), "LoadXBOXTerrain : XTT file header version (%i) of file %s conflicts desired version (%i). Please re-export\n",xttHeader.mVersion,path.getPtr(),XTD_VERSION);
      TerrLoadError()
   }

   TRACEMEM
   

   BTerrainActiveTextureInfo *texInfo = new BTerrainActiveTextureInfo[xttHeader.mNumActiveTextures];

   //load our active textures
   for(int i=0;i<xttHeader.mNumActiveTextures;i++)
   {
      ecfReader.getStream()->readBytes(&texInfo[i].mFilename,cXTT_FilenameSize);
      ecfReader.getStream()->readObj(texInfo[i].mUScale);
      ecfReader.getStream()->readObj(texInfo[i].mVScale);
      ecfReader.getStream()->readObj(texInfo[i].mBlendOp);
   }


   //load our working set textures
   if(!gTerrainTexturing.loadTextures(pParams->mTerrainDirID, xttHeader,texInfo))
   {
      sprintf_s(buf, sizeof(buf), "LoadXBOXTerrain : Terrain textures didn't load\n");
      gConsoleOutput.output(cMsgError, "%s", buf);
      gTerrain.setLoadPending(false);
      return false;
   }
   
   //load decal textures (if we have them)
   if(xttHeader.mNumActiveDecals >0)
   {
      BTerrainActiveDecalInfo *dcll = new BTerrainActiveDecalInfo[xttHeader.mNumActiveDecals];
      for(int i=0;i<xttHeader.mNumActiveDecals;i++)
         ecfReader.getStream()->readBytes(dcll[i].mFilename,cXTT_FilenameSize);
      
      BTerrainActiveDecalInstanceInfo *dclil = new BTerrainActiveDecalInstanceInfo[xttHeader.mNumActiveDecalInstances]; 
      for(int i=0;i<xttHeader.mNumActiveDecalInstances;i++)
      {
         ecfReader.getStream()->readObj(dclil[i].mActiveDecalIndex);
         ecfReader.getStream()->readObj(dclil[i].mRotation);
         ecfReader.getStream()->readObj(dclil[i].mTileCenterX);
         ecfReader.getStream()->readObj(dclil[i].mTileCenterY);
         ecfReader.getStream()->readObj(dclil[i].mUScale);
         ecfReader.getStream()->readObj(dclil[i].mVScale);
         
      }

      //load our decals
      if(!gTerrainTexturing.setLoadDecalTextures(pParams->mTerrainDirID, xttHeader,dcll,dclil))
      {
         sprintf_s(buf, sizeof(buf), "LoadXBOXTerrain : Terrain decal textures didn't load\n");
         gConsoleOutput.output(cMsgError, "%s", buf);
         gTerrain.setLoadPending(false);
         return false;
      }

   }
    

   sprintf_s(buf, sizeof(buf), "LoadXBOXTerrain : Unable to load file %s\n", path.getPtr());

   TRACEMEM

   //load each of our terrain chunks
   for(int i=0;i<numECFChunks;i++)
   {
      const BECFChunkHeader& cHeader = ecfReader.getChunkHeader(i);
      const uint64 id = cHeader.getID();
      switch(id)
      {
      case cXTT_TerrainAtlasLinkChunk:
         {
            //SCOPEDSAMPLE(BTerrainIOLoader_cXTT_TerrainAtlasLinkChunk)
            XTTLinker link;
            ecfReader.getStream()->readObj(link);
            
            if (link.numSplatLayers < 0)
            {
               gConsoleOutput.output(cMsgError, "%s", buf);
               gTerrain.setLoadPending(false);
               return false;
            }

            int *splatLayerIDs = NULL;
            void* mpPhysicalMemoryPointerSplats =NULL;
            //SPLATTIES
            {
               //read our layer active TextureIDs
               splatLayerIDs = new int[link.numSplatLayers];
               ecfReader.getStream()->readBytes(splatLayerIDs,link.numSplatLayers*sizeof(int));
               BTerrainMetrics::addCPUMem(link.numSplatLayers*sizeof(int));



               if(link.numSplatLayers>1)
               {
                  //read our layers
                  int numAlignedLayers = ((link.numSplatLayers-1)>>2)+1;
                  int bitsPerPixel =XGBitsPerPixelFromFormat((D3DFORMAT)cAlphaLayersFormat);
                  int memSize = (numAlignedLayers * BTerrainTexturing::getAlphaTextureWidth() * BTerrainTexturing::getAlphaTextureHeight() * bitsPerPixel)>>3;

                  mpPhysicalMemoryPointerSplats = XPhysicalAlloc( memSize, MAXULONG_PTR, 0, PAGE_READWRITE);
                  BVERIFY(mpPhysicalMemoryPointerSplats);
                  if (!ecfReader.getStream()->readBytes(mpPhysicalMemoryPointerSplats,memSize))
                  {
                     XPhysicalFree(mpPhysicalMemoryPointerSplats);
                     mpPhysicalMemoryPointerSplats = NULL;

                     gConsoleOutput.output(cMsgError, "%s", buf);
                     gTerrain.setLoadPending(false);
                     return false;
                  }
                  BTerrainMetrics::addTextureLayersGPUMem(memSize);

                  Utils::FlushCacheLines(mpPhysicalMemoryPointerSplats,memSize);
               }
                              
            }
            
            int *decalLayerIDs = NULL;
            void* mpPhysicalMemoryPointerDecals =NULL;
            //DECALIES
            if(link.numDecalLayers)
            {
               //read our layer active TextureIDs
               decalLayerIDs = new int[link.numDecalLayers];
               {
                  ecfReader.getStream()->readBytes(decalLayerIDs,link.numDecalLayers*sizeof(int));
                  BTerrainMetrics::addCPUMem(link.numDecalLayers*sizeof(int));

                  if(link.numDecalLayers>0)
                  {
                     //read our layers
                     int numAlignedLayers = ((link.numDecalLayers-1)>>2)+1;
                     int bitsPerPixel =XGBitsPerPixelFromFormat((D3DFORMAT)cAlphaLayersFormat);
                     int memSize = (numAlignedLayers * BTerrainTexturing::getAlphaTextureWidth() * BTerrainTexturing::getAlphaTextureHeight() * bitsPerPixel)>>3;

                     mpPhysicalMemoryPointerDecals = XPhysicalAlloc( memSize, MAXULONG_PTR, 0, PAGE_READWRITE);
                     BVERIFY(mpPhysicalMemoryPointerDecals);
                     if (!ecfReader.getStream()->readBytes(mpPhysicalMemoryPointerDecals,memSize))
                     {
                        XPhysicalFree(mpPhysicalMemoryPointerDecals);
                        mpPhysicalMemoryPointerDecals = NULL;

                        gConsoleOutput.output(cMsgError, "%s", buf);
                        gTerrain.setLoadPending(false);
                        return false;
                     }
                     BTerrainMetrics::addTextureLayersGPUMem(memSize);

                     Utils::FlushCacheLines(mpPhysicalMemoryPointerDecals,memSize);
                  }
               }
            }


            addTextureLinker(link,mpPhysicalMemoryPointerSplats,splatLayerIDs,
                                  mpPhysicalMemoryPointerDecals,decalLayerIDs);

            // rg [9/4/08] - Adding to fix leak
            if (splatLayerIDs)
            {
               delete [] splatLayerIDs;
               splatLayerIDs = NULL;
            }
            
            // rg [9/4/08] - Adding to fix leak
            if (decalLayerIDs)
            {
               delete [] decalLayerIDs;
               decalLayerIDs = NULL;
            }
                                    
            break;
         } 
      case cXTT_AtlasChunkAlbedo:
         {
           //SCOPEDSAMPLE(BTerrainIOLoader_cXTT_AtlasChunkAlbedo)
            ecfReader.seekToChunk(i);

            int memSize=0;

            ecfReader.getStream()->readObj(memSize);
            ecfReader.getStream()->readObj(gTerrainTexturing.mLargeUniqueTextureWidth);
            ecfReader.getStream()->readObj(gTerrainTexturing.mLargeUniqueTextureHeight);
            ecfReader.getStream()->readObj(gTerrainTexturing.mLargeUniqueTextureMipCount);
            

            gTerrainTexturing.mpLargeUniqueAlbedoDataPointer = XPhysicalAlloc( memSize, MAXULONG_PTR, 0, PAGE_READWRITE);
            BVERIFY(gTerrainTexturing.mpLargeUniqueAlbedoDataPointer);
            memset(gTerrainTexturing.mpLargeUniqueAlbedoDataPointer,0,memSize);

            if (!ecfReader.getStream()->readBytes(gTerrainTexturing.mpLargeUniqueAlbedoDataPointer,memSize))
            {
               XPhysicalFree(gTerrainTexturing.mpLargeUniqueAlbedoDataPointer);
               gTerrainTexturing.mpLargeUniqueAlbedoDataPointer = NULL;

               gConsoleOutput.output(cMsgError, "%s", buf);
               gTerrain.setLoadPending(false);
               return false;
            }
            BTerrainMetrics::addUniqueAlbedoGPU(memSize);

             
            gTerrainTexturing.convertUniqueAlbedoFromMemory();
            break;
         }
      case cXTT_RoadChunk:
         {
           //SCOPEDSAMPLE(BTerrainIOLoader_cXTT_RoadChunk)
            ecfReader.seekToChunk(i);

            int roadCount=gRoadManager.mRoads.size();

            BTerrainRoad *rd = new BTerrainRoad();
            gRoadManager.mRoads.push_back(rd);
            
            
            //read texture
            BFixedString256 mFilename;
            ecfReader.getStream()->readBytes(&mFilename,cXTT_FilenameSize);

            int numChunks =0;
            ecfReader.getStream()->readObj(numChunks);
            
            for(int qnIndex=0;qnIndex<numChunks;qnIndex++)
            {
               BTerrainRoadQNChunk *rqn = new BTerrainRoadQNChunk();
               gRoadManager.mRoads[roadCount]->mRoadChunks.push_back(rqn);

                ecfReader.getStream()->readObj(gRoadManager.mRoads[roadCount]->mRoadChunks[qnIndex]->mQNParentIndex);
                ecfReader.getStream()->readObj(gRoadManager.mRoads[roadCount]->mRoadChunks[qnIndex]->mNumPrims);
                int memSize =0;
                ecfReader.getStream()->readObj(memSize);

                gRoadManager.mRoads[roadCount]->mRoadChunks[qnIndex]->mpPhysicalMemoryPointer = XPhysicalAlloc( memSize, MAXULONG_PTR, 0, PAGE_READWRITE);
                BVERIFY(gRoadManager.mRoads[roadCount]->mRoadChunks[qnIndex]->mpPhysicalMemoryPointer);
                if (!ecfReader.getStream()->readBytes(gRoadManager.mRoads[roadCount]->mRoadChunks[qnIndex]->mpPhysicalMemoryPointer,memSize))
                {
                   XPhysicalFree(gRoadManager.mRoads[roadCount]->mRoadChunks[qnIndex]->mpPhysicalMemoryPointer);
                   gRoadManager.mRoads[roadCount]->mRoadChunks[qnIndex]->mpPhysicalMemoryPointer = NULL;

                   gConsoleOutput.output(cMsgError, "%s", buf);
                   gTerrain.setLoadPending(false);
                   return false;
                }
                gRoadManager.mRoads[roadCount]->mRoadChunks[qnIndex]->mSegmentVB = new D3DVertexBuffer();
                XGSetVertexBufferHeader(memSize,0,0,0,gRoadManager.mRoads[roadCount]->mRoadChunks[qnIndex]->mSegmentVB);
                XGOffsetResourceAddress( gRoadManager.mRoads[roadCount]->mRoadChunks[qnIndex]->mSegmentVB, gRoadManager.mRoads[roadCount]->mRoadChunks[qnIndex]->mpPhysicalMemoryPointer ); 

            }

               //trigger the road texture load
               gRoadManager.mRoads[roadCount]->setRoadTexture(cDirArt,mFilename.c_str());
               
            

            break;
         }
      case cXTT_FoliageHeaderChunk:
         {
            //SCOPEDSAMPLE(BTerrainIOLoader_cXTT_FoliageHeaderChunk)
            ecfReader.seekToChunk(i);
            uint numSetsUsed=0;
            ecfReader.getStream()->readObj(numSetsUsed);

            //read names
            BFixedString256 mFilename;
            for(uint i=0;i<numSetsUsed;i++)
            {
               ecfReader.getStream()->readBytes(&mFilename,cXTT_FilenameSize);
               gFoliageManager.newSet(mFilename); //string expected as - "foliage\\foliageset"
            }
            

            break;
         }
      case cXTT_FoliageQNChunk:
         {
            //SCOPEDSAMPLE(BTerrainIOLoader_cXTT_FoliageQNChunk)
            ecfReader.seekToChunk(i);

            BTerrainFoliageQNChunk *qnc = new BTerrainFoliageQNChunk();
            ecfReader.getStream()->readObj(qnc->mQNParentIndex);
            ecfReader.getStream()->readObj(qnc->mNumSets);

            //read our set arrays
            qnc->mSetIndexes = new int[qnc->mNumSets];
            ecfReader.getStream()->readBytes(qnc->mSetIndexes,qnc->mNumSets * sizeof(uint));

            qnc->mSetPolyCount = new int[qnc->mNumSets];
            ecfReader.getStream()->readBytes(qnc->mSetPolyCount,qnc->mNumSets * sizeof(uint));

            int totalPhysicalMemory=0;
            ecfReader.getStream()->readObj(totalPhysicalMemory);

            int *indMemSizes = new int[qnc->mNumSets];
            ecfReader.getStream()->readBytes(indMemSizes,qnc->mNumSets * sizeof(uint));

            qnc->mpPhysicalMemoryPointer = XPhysicalAlloc( totalPhysicalMemory, MAXULONG_PTR, 0, PAGE_READWRITE);
            BVERIFY(qnc->mpPhysicalMemoryPointer );
            if (!ecfReader.getStream()->readBytes(qnc->mpPhysicalMemoryPointer ,totalPhysicalMemory))
            {
               XPhysicalFree(qnc->mpPhysicalMemoryPointer );
               qnc->mpPhysicalMemoryPointer  = NULL;

               gConsoleOutput.output(cMsgError, "%s", buf);
               gTerrain.setLoadPending(false);
               return false;
            }

            BTerrainMetrics::addFoliageGPU(totalPhysicalMemory);

            qnc->mSetIBs = new LPDIRECT3DINDEXBUFFER9[qnc->mNumSets];
            unsigned char *ptr = (unsigned char *)qnc->mpPhysicalMemoryPointer;
            for(uint i=0;i<qnc->mNumSets;i++)
            {
               qnc->mSetIBs[i] = new D3DIndexBuffer();
               XGSetIndexBufferHeader(indMemSizes[i],0,D3DFMT_INDEX32,0,0,qnc->mSetIBs[i]);
               XGOffsetResourceAddress( qnc->mSetIBs[i], ptr ); 
               ptr+=indMemSizes[i];
            }


            delete []indMemSizes;


            gFoliageManager.mFoliageChunks.push_back(qnc);

            break;
         }
      }
   }
   tfile.close();

   TRACEMEM
   
      //CLM cache init now down as a post level load operation to allow memory for archives
   gTerrainTexturing.setCacheChannelSkips(skipSpecMapTextureCache,skipSelfTextureCache,skipEnvMapTextureCache);
   //gTerrainTexturing.initCaches(skipSpecMapTextureCache,skipSelfTextureCache,skipEnvMapTextureCache);  //CLM this should REALLY go somewhere else more internal!!!!
   
   TRACEMEM

   return true;
}

//============================================================================
// BTerrainIOLoader::loadInternal
//============================================================================
bool BTerrainIOLoader::loadInternal(const BLoadParams* pParams)
{
   ASSERT_RENDER_THREAD
   SCOPEDSAMPLE(BTerrainIOLoader_loadInternal)

   BTerrainMetrics::clearAll();

   bool loadOK = true;
      
   loadOK &= gTerrainHeightField.loadECFXTHTInternal(pParams->mDirID,pParams->mFilename);

   if(!loadOK)
      return false; // gTerrainHeightField.computeHeightField();//CLM TEMP! REMOVE THIS ONCE EVERY TERRAIN GENERATES THIS!

   gTerrain.setLoadSuccessful(false);  //if we don't load the vis rep, don't let the terrain render
   gTerrain.setLoadPending(true);

   loadOK &= loadXTDInternal(pParams,pParams->mLoadVisRep);
   
   //if we're not supposed to load the visrep, early out here.
   if(!pParams->mLoadVisRep)
   {
      gTerrain.setLoadSuccessful(false);
      gTerrain.setLoadPending(false);
      return loadOK;
   }


   
   loadOK &= loadXTTInternal(pParams);
   
   gTerrain.setLoadSuccessful(loadOK);
   gTerrain.setLoadPending(false);

   if(loadOK)
   {
      gTerrainDynamicAlpha.createAlphaTexture();
      return true;
   }
      
   gTerrainHeightField.releaseHeightField();
   return false;
   
}
//============================================================================
// BTerrainIOLoader::addVisualChunk
//============================================================================
bool BTerrainIOLoader::reloadInternal()
{
   ASSERT_RENDER_THREAD
   return true;//loadInternal(&mLastLoadParams); //CLM COMMENTED OUT DUE TO ISSUES W/ QUICKVIEW
}
//============================================================================
// BTerrainIOLoader::addVisualChunk
//============================================================================
bool  BTerrainIOLoader::addVisualChunk(XTDVisualChunkHeader vch,int numXChunks)
{
	ASSERT_RENDER_THREAD

      
	int index = vch.gridX * numXChunks + vch.gridZ;

	gTerrain.mpQuadGrid[index].mDesc.m_min=vch.mmin;
	gTerrain.mpQuadGrid[index].mDesc.m_max=vch.mmax;
   
	
   gTerrain.mpQuadGrid[index].mDesc.mMinXVert = vch.gridX * BTerrainQuadNode::getMaxNodeWidth();
   gTerrain.mpQuadGrid[index].mDesc.mMaxXVert = gTerrain.mpQuadGrid[index].mDesc.mMinXVert+ BTerrainQuadNode::getMaxNodeWidth();
   gTerrain.mpQuadGrid[index].mDesc.mMinZVert = vch.gridZ * BTerrainQuadNode::getMaxNodeWidth();
   gTerrain.mpQuadGrid[index].mDesc.mMaxZVert = gTerrain.mpQuadGrid[index].mDesc.mMinZVert+ BTerrainQuadNode::getMaxNodeWidth();
   gTerrain.mpQuadGrid[index].mDesc.mCanCastShadows = vch.canCastShadows;


   int minXChunk = gTerrain.mpQuadGrid[index].mDesc.mMinXVert >> 6;
   int minZChunk = gTerrain.mpQuadGrid[index].mDesc.mMinZVert >> 6;
   int minXPatch = minXChunk << 4;
   int patchPitch = gTerrain.getNumXChunks() << 4;
   gTerrain.mpQuadGrid[index].mDesc.mTessPatchStartIndex =((minXPatch + patchPitch * minZChunk)<<2);


   gTerrain.mpQuadGrid[index].mSkirtInfo.mIsSkirtChunk=false;
   gTerrain.mpQuadGrid[index].mSkirtInfo.mpOwnerNode=NULL;
   gTerrain.mpQuadGrid[index].mSkirtInfo.mQuadrant=-1;

   if(!gTerrain.mpQuadGrid[index].mRenderPacket)
   {
      gTerrain.mpQuadGrid[index].mRenderPacket = new BTerrainRenderPacket();
      BTerrainMetrics::addCPUMem(sizeof(BTerrainRenderPacket));
   }
  
	return true;
}

//============================================================================
// BTerrainIOLoader::addTextureLinker
//============================================================================
bool  BTerrainIOLoader::addTextureLinker(const XTTLinker &link,void *physicalMemSplat, int *layerIdsSplat,void *physicalMemDecal, int *layerIdsDecal)
{
   int index = link.gridX * gTerrain.mNumXChunks + link.gridZ;

   BDEBUG_ASSERT(link.gridZ<gTerrain.mNumXChunks);
   BDEBUG_ASSERT(link.gridX<gTerrain.mNumXChunks);

   if(!gTerrain.mpQuadGrid[index].mRenderPacket)
   {
      gTerrain.mpQuadGrid[index].mRenderPacket = new BTerrainRenderPacket();
      BTerrainMetrics::addCPUMem(sizeof(BTerrainRenderPacket));
   }
      

   BASSERT(!gTerrain.mpQuadGrid[index].mRenderPacket->mTexturingData);


   gTerrain.mpQuadGrid[index].mRenderPacket->mTexturingData= new BTerrainTexturingRenderData();
   BTerrainMetrics::addCPUMem(sizeof(BTerrainTexturingRenderData));


   gTerrain.mpQuadGrid[index].mRenderPacket->mTexturingData->mCachedUniqueTexture = NULL;

   BTerrainTextureLayerContainer *container = &gTerrain.mpQuadGrid[index].mRenderPacket->mTexturingData->mLayerContainer;


   //SPLATS
   {
      int numAlignedLayers = ((link.numSplatLayers-1)>>2)+1;
      gTerrain.mpQuadGrid[index].mRenderPacket->mTexturingData->mLayerContainer.mSplatData.mNumLayers = link.numSplatLayers;
      gTerrain.mpQuadGrid[index].mRenderPacket->mTexturingData->mLayerContainer.mSplatData.mNumAlignedLayers = numAlignedLayers;

      int imgSize=0;
      unsigned char *mDevicePtrCounter = (unsigned char *)physicalMemSplat;

      container->mSplatData.mAlphaLayerTexture=NULL;
      if(link.numSplatLayers > 1)
      {
         container->mSplatData.mAlphaLayerTexture = new IDirect3DArrayTexture9();
         BTerrainMetrics::addCPUMem(sizeof(IDirect3DArrayTexture9));

         imgSize = XGSetArrayTextureHeader(BTerrainTexturing::getAlphaTextureWidth(),BTerrainTexturing::getAlphaTextureHeight(),numAlignedLayers,1,0,(D3DFORMAT)cAlphaLayersFormat,0,0,XGHEADER_CONTIGUOUS_MIP_OFFSET,0,container->mSplatData.mAlphaLayerTexture,NULL,NULL);
       //  imgSize = XGSetArrayTextureHeaderEx(BTerrainTexturing::getAlphaTextureWidth(),BTerrainTexturing::getAlphaTextureHeight(),numAlignedLayers,1,0,(D3DFORMAT)cAlphaLayersFormat,0,XGHEADEREX_BORDER,0,XGHEADER_CONTIGUOUS_MIP_OFFSET,0,container->mSplatData.mAlphaLayerTexture,NULL,NULL);
         XGOffsetResourceAddress( container->mSplatData.mAlphaLayerTexture, mDevicePtrCounter); 
         mDevicePtrCounter+=imgSize;

      }



      container->mSplatData.mNumAlignedLayers = numAlignedLayers;
      container->mSplatData.mLayerData = new D3DXVECTOR4[link.numSplatLayers];
      memset(container->mSplatData.mLayerData, 0, sizeof(float) * link.numSplatLayers * 4);
      BTerrainMetrics::addCPUMem(sizeof(D3DXVECTOR4) * link.numSplatLayers);


      float rcpTextureID = 1.0f  / ((float)gTerrainTexturing.mXTDTextureInfo.mNumActiveTextures-1);
      for(int i=0;i<link.numSplatLayers;i++)
      {  
         container->mSplatData.mLayerData[i].w = 0;
         container->mSplatData.mLayerData[i].x = 0;

         if(layerIdsSplat[i])
         {
            container->mSplatData.mLayerData[i].w = (layerIdsSplat[i] * rcpTextureID);
            container->mSplatData.mLayerData[i].x = (FLOAT)layerIdsSplat[i];
         }

         container->mSplatData.mLayerData[i].y = (FLOAT)gTerrainTexturing.mActiveTexInfo[layerIdsSplat[i]].mUScale;
         container->mSplatData.mLayerData[i].z = (FLOAT)gTerrainTexturing.mActiveTexInfo[layerIdsSplat[i]].mVScale;
      }
   }

   //DECALS
   {
      int numAlignedLayers = ((link.numDecalLayers-1)>>2)+1;
      gTerrain.mpQuadGrid[index].mRenderPacket->mTexturingData->mLayerContainer.mDecalData.mNumLayers = link.numDecalLayers;
      gTerrain.mpQuadGrid[index].mRenderPacket->mTexturingData->mLayerContainer.mDecalData.mNumAlignedLayers = numAlignedLayers;

      int imgSize=0;
      unsigned char *mDevicePtrCounter = (unsigned char *)physicalMemDecal;


      container->mDecalData.mAlphaLayerTexture=NULL;
      if(link.numDecalLayers > 0)
      {
         container->mDecalData.mAlphaLayerTexture = new IDirect3DArrayTexture9();
         BTerrainMetrics::addCPUMem(sizeof(IDirect3DArrayTexture9));

         imgSize = XGSetArrayTextureHeader(BTerrainTexturing::getAlphaTextureWidth(),BTerrainTexturing::getAlphaTextureHeight(),numAlignedLayers,1,0,(D3DFORMAT)cAlphaLayersFormat,0,0,XGHEADER_CONTIGUOUS_MIP_OFFSET,0,container->mDecalData.mAlphaLayerTexture,NULL,NULL);
         XGOffsetResourceAddress( container->mDecalData.mAlphaLayerTexture, mDevicePtrCounter); 
         mDevicePtrCounter+=imgSize;

      }

      container->mDecalData.mNumAlignedLayers = numAlignedLayers;
      if(link.numDecalLayers)
      {
         container->mDecalData.mActiveDecalIndexes = new int[link.numDecalLayers];

         memcpy(container->mDecalData.mActiveDecalIndexes,layerIdsDecal,sizeof(float) * link.numDecalLayers);
         BTerrainMetrics::addCPUMem(sizeof(float) * link.numDecalLayers);
      }
   }

   container->mSpecPassNeeded = link.specPassNeeded != 0;
   container->mEnvMaskPassNeeded = link.envMaskPassNeeded != 0;
   container->mSelfPassNeeded = link.selfPassNeeded != 0;
   container->mAlphaPassNeeded = link.alphaPassNeeded != 0;
   container->mIsFullyOpaque = link.isFullyOpaque !=0;

   //EARLY OUT TO SAVE MEMORY ON SOME MAPS!
   skipSpecMapTextureCache &= !link.specPassNeeded;
   skipEnvMapTextureCache &= !link.envMaskPassNeeded;
   skipSelfTextureCache   &= !link.selfPassNeeded;

   gTerrain.mpQuadGrid[index].mRenderPacket->mTexturingData->mLayerContainer.mSplatData.mpPhysicalMemoryPtr = physicalMemSplat;

   
   return true;
}
//============================================================================
// BTerrainIOLoader::initDeviceData
//============================================================================
void  BTerrainIOLoader::initDeviceData(void)
{
}
//============================================================================
// BTerrainIOLoader::deinitDeviceData
//============================================================================
void  BTerrainIOLoader::deinitDeviceData(void)
{
}
//============================================================================
// BTerrainIOLoader::frameBegin
//============================================================================
void BTerrainIOLoader::frameBegin(void)
{
}

//============================================================================
// BTerrainIOLoader::frameEnd
//============================================================================
void BTerrainIOLoader::frameEnd(void)
{
}

//============================================================================
// BTerrainIOLoader::processCommand
//============================================================================
void  BTerrainIOLoader::processCommand(const BRenderCommandHeader& header, const unsigned char* pData)
{
   switch (header.mType)
   {
      case cTLLoad:
      {
         loadInternal((BLoadParams*)pData);
         break;
      }
   };
}
//============================================================================
// BTerrainIOLoader::reloadInit
//============================================================================
void BTerrainIOLoader::reloadInit(BLoadParams &params)
{
#ifdef ENABLE_RELOAD_MANAGER
   BReloadManager::BPathArray paths;

   BString pathXTD(params.mFilename);
   pathXTD += ".xtd";
   paths.pushBack(pathXTD);

   BString pathXTT(params.mFilename);
   pathXTT += ".xtt";
   paths.pushBack(pathXTT);


   gReloadManager.registerClient(paths, BReloadManager::cFlagSynchronous, mEventHandle, cXTDXTTReloadFileEvent, 0);
#endif
}
//============================================================================
// BTerrainIOLoader::receiveEvent
//============================================================================
#ifdef ENABLE_RELOAD_MANAGER
bool BTerrainIOLoader::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   ASSERT_THREAD(cThreadIndexRender);
   switch (event.mEventClass)
   {
   case cXTDXTTReloadFileEvent:
      {
         gTerrain.setReloadPending(true);

         break;            
      }
   };
   return false;
}
#endif