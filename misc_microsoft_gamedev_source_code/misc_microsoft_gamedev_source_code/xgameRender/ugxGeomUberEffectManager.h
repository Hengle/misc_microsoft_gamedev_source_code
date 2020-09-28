//============================================================================
//
//  ugxGeomUberEffectManager.h
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#pragma once

#include "effect.h"

//============================================================================
// enum eUberEffectTechniques
// Must match the technique order in parametricShader.fx
//============================================================================
enum eUberEffectTechniques
{
   cUberEffectTechniqueVisible,
   cUberEffectTechniqueSky,
   //cUberEffectTechniqueUI,
   cUberEffectTechniqueVis,
   
   cUberEffectTechniqueShadowGen,
   cUberEffectTechniqueShadowGenOverallAlpha,
   cUberEffectTechniqueShadowGenAlphaTest,
   
   cUberEffectTechniqueDPShadowGen,
   cUberEffectTechniqueDPShadowGenOverallAlpha,
   cUberEffectTechniqueDPShadowGenAlphaTest,
   
   cUberEffectTechniqueDistortion,

   cUberEffectTechniqueNum
};

//============================================================================
// class BUGXGeomUberEffectManager
//============================================================================
class BUGXGeomUberEffectManager : public BEventReceiver
{
public:
   BUGXGeomUberEffectManager();
   ~BUGXGeomUberEffectManager();

   void init(long effectDirID, BFXLEffectIntrinsicPool* pIntrinsicPool = NULL);
   void deinit(void);

   void changeDevice(IDirect3DDevice9* pDev);

   BFXLEffect& getEffect(bool bumpMapped, bool disableDirShadowReception);
         
   uint getLoadCount(void) const { return mLoadCount; }
   
   BFXLEffectIntrinsicPool* getIntrinsicPool(void) const { return mpIntrinsicPool; }

private:   
   // effectIndex bit 0 = tangents
   // effectIndex bit 1 = dir shadowing
#if 0
   enum { cMaxEffects = 4 };
#else   
   enum { cMaxEffects = 2 };
#endif   
   
   long mEffectDirID;
   long mLoadCount;

   BFXLEffect mEffects[cMaxEffects];
   
   BFXLEffectIntrinsicPool* mpIntrinsicPool;

   enum 
   {
      cUGXGeomReloadEventClass = cEventClassFirstUser
   };

   virtual bool receiveEvent(const BEvent& event, BThreadIndex threadIndex);

   void loadEffects(int index = -1);
};

extern BUGXGeomUberEffectManager gUGXGeomUberEffectManager;
