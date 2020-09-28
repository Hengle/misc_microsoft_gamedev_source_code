//==============================================================================
// lightVisualInstance.cpp
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================

// Includes
#include "xgameRender.h"
#include "lightVisualInstance.h"
#include "lightVisualData.h"
#include "debugprimitives.h"

// Defines
//#define DEBUG_LIGHT_TRANSFORM

//==============================================================================
// BLightVisualInstance::BLightVisualInstance
//==============================================================================
BLightVisualInstance::BLightVisualInstance() :
   mLightHandle(cInvalidLocalLightHandle)
{

}

//==============================================================================
// BLightVisualInstance::~BLightVisualInstance()
//==============================================================================
BLightVisualInstance::~BLightVisualInstance()
{
   
}

//==============================================================================
// BLightVisualInstance::init
//==============================================================================
bool BLightVisualInstance::init(BLightVisualData* pData, const BMatrix* pTransform)
{
   if(!pData)
      return false;
   mpData=pData;

   mLightHandle = gSimSceneLightManager.createLocalLight();
   if(mLightHandle==cInvalidLocalLightHandle)
      return false;

   gSimSceneLightManager.setLocalLightRadius(mLightHandle, pData->mRadius);
   
   if (pData->mType == cLTSpot)
   {
      gSimSceneLightManager.setLocalLightSpotOuter(mLightHandle, pData->mOuterAngle);
      gSimSceneLightManager.setLocalLightSpotInner(mLightHandle, pData->mInnerAngle);
   }
   gSimSceneLightManager.setLocalLightShadows(mLightHandle, pData->mShadows);

   BVector v;
   pTransform->getTranslation(v);
   gSimSceneLightManager.setLocalLightPos(mLightHandle, (const XMFLOAT3*)&v);

   if (pData->mType == cLTSpot)
		v = pData->mDirection;
   else
		pTransform->getForward(v);
   gSimSceneLightManager.setLocalLightAt(mLightHandle, (const XMFLOAT3*)&v);

   if (pData->mType == cLTSpot)
		v = pData->mDirection.cross(BVector(0,1,0));
   else
		pTransform->getRight(v);
   gSimSceneLightManager.setLocalLightRight(mLightHandle, (const XMFLOAT3*)&v);

   BLocalLightParams& params0 = gSimSceneLightManager.getLocalLightParams(mLightHandle);
   params0.setColor(pData->mColor * pData->mIntensity);
   params0.setType(pData->mType);
   params0.setDecayDist(pData->mDecayDist);
   params0.setFarAttenStart(pData->mFarAttenStart);
   params0.setSpecular(pData->mSpecular);
   params0.setShadowDarkness(pData->mShadowDarkness);
   params0.setLightBuffered(pData->mLightBuffered);

   gSimSceneLightManager.enforceLimits(mLightHandle);

   gSimSceneLightManager.setLocalLightEnabled(mLightHandle, true);

   return true;
}

//==============================================================================
// BLightVisualInstance::deinit
//==============================================================================
void BLightVisualInstance::deinit()
{
   if(mLightHandle!=cInvalidLocalLightHandle)
   {
      gSimSceneLightManager.freeLocalLight(mLightHandle);
      mLightHandle=cInvalidLocalLightHandle;
   }
}

//==============================================================================
// BLightVisualInstance::updateWorldMatrix
//==============================================================================
void BLightVisualInstance::updateWorldMatrix(BMatrix worldMatrix, BMatrix* pLocalMatrix)
{
   BMatrix finalMatrix;
   if (pLocalMatrix)      
      finalMatrix.mult(*pLocalMatrix, worldMatrix);
   else
      finalMatrix = worldMatrix;   

   BVector pos;
   finalMatrix.getTranslation(pos);
   gSimSceneLightManager.setLocalLightPos(mLightHandle, (const XMFLOAT3*)&pos);

   BVector forward;
   finalMatrix.getForward(forward);
   forward.normalize();
   gSimSceneLightManager.setLocalLightAt(mLightHandle, (const XMFLOAT3*)&forward);

   BVector right;
   finalMatrix.getRight(right);
   right.normalize();
   gSimSceneLightManager.setLocalLightRight(mLightHandle, (const XMFLOAT3*)&right);

#ifdef DEBUG_LIGHT_TRANSFORM
   
   gpDebugPrimitives->addDebugArrow(finalMatrix, 4.0f, cDWORDRed);

   /*
   BVector up;
   up.assignCrossProduct(forward, right);
   up.normalize();

   gpDebugPrimitives->addDebugAxis(pos, right, up, forward, 4.0f);
   */
#endif
}

//==============================================================================
// BLightVisualInstance::setVisibility
//==============================================================================
void BLightVisualInstance::setVisibility(bool bState)
{
}

//==============================================================================
// BLightVisualInstance::update
//==============================================================================
void BLightVisualInstance::update(float elapsedTime, bool synced)
{
   elapsedTime;
   synced;
}