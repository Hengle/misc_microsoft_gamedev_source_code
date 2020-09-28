//============================================================================
// flashhud.h
//
//============================================================================

#pragma once 
#include "flashscene.h"
#include "flashgateway.h"
#include "d3dtexturemanager.h"
#include "visual.h"

const int cMaxEnabledDecals = 64;

class BFlashDecalLookup
{
   public:
   BFlashDecalLookup(): mProtoID(-1), mIndex(-1) {};
  ~BFlashDecalLookup(){};

   int mProtoID;
   int mIndex;
   int mIconDataIndex;
   int mIconDataIndex2;
};

//============================================================================
// class BFlashDecalUpdateData
//============================================================================
struct BFlashDecalUpdateData
{
   int   mCount;
   void* mpData;
};

//============================================================================
// class BFlashDecal
//============================================================================
class BFlashDecal : public BFlashScene
{
public:
   BFlashDecal();
   virtual ~BFlashDecal();

   enum 
   {
      cFlagInitialized = 0,
      cFlagTotal
   };

   enum eFlashDecalCommand
   {
      eFDCUpdate = 0,
      eFDCCommandTotal
   };

   bool init(const char* filename, const char* datafile);
   void deinit();
   void enter();
   void leave();
   BFlashMovieInstance*  getMovie() { return NULL; }

   void update(float elapsedTime);
   void renderBegin();
   void render();
   void renderEnd();
   bool handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail);
   bool receiveEvent(const BEvent& event, BThreadIndex threadIndex);
   void setType(int decalType);

   void updateRenderThread();

   void relaseGPUHeapTextures();
   bool getDecalUV(int decalID, XMHALF4& uv, XMHALF4& uv2);   
   bool getDecalSize(int protoID, float& sizeX, float& sizeY, float& sizeZ);
   int  getProtoIconIndex(int protoID);
   

   void workerRender();

   int getMovieInstanceIndex(int decalID);
   BManagedTextureHandle getDecalTextureHandle(BFlashPropertyHandle propertyHandle);

private:   

   // Called from the worker thread to process commands.
   virtual void processCommand(const BRenderCommandHeader& header, const uchar* pData);
   
   bool initLookups();
   bool initMovies();

   BBitArray mSimEnableStates;
   
   
   BFlashGateway::BDataHandle mDataHandle;
   BDynamicSimArray<BFlashDecalLookup> mDecalLookup;

   
   typedef BHashMap<BFixedString128, int> BFlashDecalHashmap;
   BFlashDecalHashmap mHashmap;
   BDynamicSimArray<BFlashMovieInstance*> mFlashMovieLookup;


   BDynamicSimArray<int> mMovies;

   // worker thread data
   BDynamicArray<int> mWorkerRenderList;

};