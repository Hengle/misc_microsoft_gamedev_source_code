// File: temporalAA.h
#pragma once

#include "threading\eventDispatcher.h"
#include "math\generalMatrix.h"

#include "effect.h"
#include "renderThread.h"

class BTemporalAAManager : public BEventReceiver, BRenderCommandListener
{
public:
   BTemporalAAManager();
   ~BTemporalAAManager();

   void init(uint width, uint height);
      
   void deinit(void);
   
   bool isInitialized(void) const { return 0 != mWidth; }

   void begin(uint frameIndex, const BMatrix44& view, const BMatrix44& projection, float& l, float& t, float& r, float& b);

   void resolve(void);

   void filter(uint x = 0, uint y = 0);

private:
   uint mWidth;
   uint mHeight;

   enum { cNumFrames = 2 };
   IDirect3DTexture9* mpTextures[cNumFrames];

   uint mWorkerFrameIndex;
   BMatrix44 mViewMatrices[cNumFrames];
   BMatrix44 mProjMatrices[cNumFrames];
   
   float mCurLerpFactor;

   BFXLEffect mEffect;

   struct BBeginData
   {
      uint mFrameIndex;
      //BMatrix44 mView;
      //BMatrix44 mProj;
   };
   
   struct BFilterData
   {
      WORD mX;
      WORD mY;
      float mLerpFactor;
   };

   enum 
   {
      cTAACommandBegin,
      cTAACommandResolve,
      cTAACommandFilter
   };

   virtual bool receiveEvent(const BEvent& event, BThreadIndex threadIndex);
   virtual void initDeviceData(void);
   virtual void frameBegin(void);

   void workerBegin(const BRenderCommandHeader& header, const uchar* pData);
   void workerResolve(const BRenderCommandHeader& header, const uchar* pData);
   void workerFilter(const BRenderCommandHeader& header, const uchar* pData);

   virtual void processCommand(const BRenderCommandHeader& header, const uchar* pData);
   virtual void frameEnd(void);
   virtual void deinitDeviceData(void);
};

extern BTemporalAAManager gTemporalAAManager;
