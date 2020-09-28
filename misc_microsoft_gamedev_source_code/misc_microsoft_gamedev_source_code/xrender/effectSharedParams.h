//============================================================================
//
// File: effectSharedParams.h
// rg [2/18/06] - All FXLEffect objects should use the effect pool managed by this object, otherwise correlation will be lost.
// Also, be sure to include shared\sharedParams.inc in all .fx files.
// 
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#pragma once

#include "renderThread.h"
#include "effect.h"
#include "matrixTracker.h"

// cModelToWorld,    // model to world
// cWorldToView,     // world to view
// cViewToProj,      // view to projection
// cProjToScreen,    // projection to screen

// cViewToWorld,     // view to world
// cScreenToProj,    // screen to projection

// cWorldToProj,     // world to projection
// cModelToProj,     // model to projection
// cScreenToView,    // screen to view

class BEffectSharedParamPool : public BRenderCommandListener
{
public:
   BEffectSharedParamPool();
   ~BEffectSharedParamPool();
   
   // Main thread
   void init(void);
   void deinit(void);
 
   void updateMatrices(void);
          
   // Worker thread
   BFXLEffectPool& getPool(void) { return mEffectPool; }
   FXLEffectPool* getEffectPool(void) { return mEffectPool.getEffectPool(); }
   void setupPoolParameters(void);
         
private:
   BFXLEffectPool mEffectPool;
   
   FXLHANDLE mMatrixTrackerParams[cNumMTMatrices];
   
   FXLHANDLE mWorldSunDir;
   FXLHANDLE mViewSunDir;
            
   virtual void initDeviceData(void);
   virtual void frameBegin(void);
   virtual void processCommand(const BRenderCommandHeader& header, const uchar* pData);
   virtual void frameEnd(void);
   virtual void deinitDeviceData(void);
     
   enum eCommands
   {
      cCommandUpdateMatrices,
   };

   void processUpdateMatricesCommand(void);
};

extern BEffectSharedParamPool gEffectSharedParamPool;
