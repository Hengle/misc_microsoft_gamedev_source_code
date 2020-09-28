//============================================================================
// graphmanager.h
// Ensemble Studios (C) 2007
//============================================================================

#pragma once
#include "containers\\fixedBlockAllocator.h"
#include "threading\eventDispatcher.h"
#include "renderThread.h"
#include "graphattribs.h"

class BGraphManager;

typedef int BGraphHandle;
const int cInvalidGraphHandle = -1;

#pragma warning(push)
#pragma warning(disable:4324)

//============================================================================
//============================================================================
class BGraphManager : public BRenderCommandListener
{
   friend class BGraphAttribs;

   public:
      BGraphManager();
     ~BGraphManager();

      bool init();
      void deinit();

      // Sim Thread
      BGraphHandle createGraph(void);
      void destroyGraph(BGraphHandle handle);
      BGraphAttribs* getGraph(BGraphHandle handle);      
      void destroyAllGraphs();
      void updateRenderThread(double gametime);
      void simRenderGraphs();
      
      // Render Thread      
      void renderGraphs();
      
   private:

      enum 
      { 
         BGMRUpdate,
      };

      enum { cMaxGraphs = 32 };
      typedef BFixedBlockAllocator<sizeof(BGraphAttribs), cMaxGraphs> BGraphAllocator;

      //============================================================================
      // struct BGraphUpdateData
      //============================================================================
      struct BGraphUpdateData
      {
         BGraphUpdateData(double gameTime, const uchar* pGraphAttribs) : mGameTime(gameTime), mpGraphAttribs(pGraphAttribs) { }
         double mGameTime;
         const uchar* mpGraphAttribs;
         
      };

      void updateGraphs(float time);
      void renderUpdate(const BGraphUpdateData* pUpdateData);
      void workerRender(void* pData);

      virtual void initDeviceData(void);
      virtual void frameBegin(void);
      virtual void processCommand(const BRenderCommandHeader& header, const uchar* pData);
      virtual void frameEnd(void);
      virtual void deinitDeviceData(void);

      __declspec(align(16)) BGraphAllocator mSimGraphAttribs;
      __declspec(align(16)) BGraphAllocator mRenderGraphAttribs;

      bool mbInitialized:1;
};
extern BGraphManager gGraphManager;