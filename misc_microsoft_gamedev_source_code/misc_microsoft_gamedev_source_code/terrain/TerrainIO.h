//============================================================================
//
//  TerrainIO.h
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#pragma once
 
// xrender
#include "rendercommand.h"

// terrain
#include "TerrainVisual.h"

//-----------------------------------------

#define XTD_VERSION                 0x000C
#define cXTD_XTDHeader		         0x1111
#define cXTD_TerrainChunk	         0x2222
#define cXTD_TerrainAtlasLinkChunk  0x4444
#define cXTD_AtlasHeaderChunk       0x6666
#define cXTD_AtlasChunk             0x8888
#define cXTD_TessChunk              0xAAAA
#define cXTD_LightingChunk          0xBBBB
#define cXTD_AOChunk                0xCCCC
#define cXTD_AlphaChunk             0xDDDD

#define XTT_VERSION                  0x0004
#define cXTT_XTTHeader               0x1111
#define cXTT_TerrainAtlasLinkChunk   0x2222

#define cXTT_AtlasChunkAlbedo        0x6666
#define cXTT_RoadChunk               0x8888
#define cXTT_FoliageHeaderChunk      0xAAAA
#define cXTT_FoliageQNChunk          0xBBBB


#define cXTT_FilenameSize            256
//-----------------------------------------
 
struct XTDHeader
{
	int mVersion;

	int mNumXVerts;
	int mNumXChunks;
	float mTileScale;
   D3DXVECTOR3 worldMin;
   D3DXVECTOR3 worldMax;
};

struct XTDVisualChunkHeader
{
	int gridX ;
	int gridZ ;
	int  maxVStride;
	D3DXVECTOR3 mmin;
	D3DXVECTOR3 mmax;
	bool canCastShadows;

};
 


struct XTTHeader
{
   int mVersion;
   int   mNumActiveTextures;
   int   mNumActiveDecals;
   int   mNumActiveDecalInstances;

   void clear(void)
   {
      Utils::ClearObj(*this);
   }
};


struct XTTLinker
{
   int gridX ;
   int gridZ ;
   int specPassNeeded;
   int selfPassNeeded;  //used as bool
   int envMaskPassNeeded;  //used as bool
   int alphaPassNeeded; 
   int isFullyOpaque;

   int numSplatLayers;  //aligned to multiple of 4
   int numDecalLayers;  //aligned to multiple of 4
   

};
//-----------------------------------------

//-----------------------------------------
/*THIS class is used as a callback handler for the IO of terrain
As data is loaded, we issue the callbacks for the following sets
When the worker thread gets around to processing it, the callback occurs here.
All interfacing classes befriend TerrainIOLoader
*/

//--------------------------------------------
enum eTLCommands
{
   cTLLoad = 0,
};

class BTerrainQuadNode;

class BTerrainIOLoader : public BRenderCommandListenerInterface
#ifdef ENABLE_RELOAD_MANAGER
   , public BEventReceiver
#endif
{
public:
   BTerrainIOLoader();
   ~BTerrainIOLoader();
   
   bool init();
   bool deinit();
   
   void load(long dirID, const char *filename, long terrainDirID, bool loadVisRep);
   
   //CALLED FROM gTerrain::beginFrame when recieving a reload command
   bool reloadInternal();

private:
   BCommandListenerHandle mCommandListenerHandle;
   uchar* mDevicePtrCounter;

   bool initTexturesFromMemoryInternal(const int mipLevels,const int type, const bool alpha, const int memSize, const int widthInBlocks, void *mpPhysicalMemoryPointer);
   bool addTextureLinker(const XTTLinker &link,void *physicalMemSplat, int *layerIdsSplat,void *physicalMemDecal, int *layerIdsDecal);
   bool addVisualChunk(XTDVisualChunkHeader vch,int numXChunks);
   
   void calcGrowDist();
   void calcSkirtData(const XTDHeader &xtdHeader);
   void createSkirtChunks(uint minXChunkIndex, uint minZChunkIndex, uint size, const XTDHeader &xtdHeader, BTerrainQuadNode* pOwnerNode);
   void createBatchedSkirtChunk(uint minXChunkIndex, uint minZChunkIndex, uint maxBatchedChunkSize, const XTDHeader &xtdHeader);

   struct BLoadParams
   {
      BLoadParams()
      {
      }
      BLoadParams(long dirID, const char *filename, long terrainDirID) :
         mDirID(dirID),
         mFilename(filename),
         mTerrainDirID(terrainDirID),
         mLoadVisRep(true)
      {
      }

      long mDirID;
      long mTerrainDirID;
      BFixedStringMaxPath mFilename;
      bool mLoadVisRep;
   };
   
   bool loadInternal(const BLoadParams* pParams);
   bool loadXTDInternal(const BLoadParams *pParams,bool loadVisRep);
   bool loadXTTInternal(const BLoadParams *pParams);
   
   void reloadInit(BLoadParams &params);

   BLoadParams mLastLoadParams;

#ifdef ENABLE_RELOAD_MANAGER
   enum
   {
    cXTDXTTReloadFileEvent = cEventClassFirstUser
   };
   virtual bool               receiveEvent(const BEvent& event, BThreadIndex threadIndex);
#endif
   virtual void  processCommand(const BRenderCommandHeader& header, const unsigned char* pData);  
   virtual void frameBegin(void);
   virtual void frameEnd(void);
   virtual void initDeviceData(void);
   virtual void deinitDeviceData(void);
};
