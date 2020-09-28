//==============================================================================
// unitactiondome.cpp
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#include "common.h"
#include "unitactiondome.h"
#include "entity.h"
#include "unit.h"
#include "squad.h"
#include "tactic.h"
#include "visual.h"
#include "grannyinstance.h"
#include "world.h"
#include "grannymanager.h"
#include "grannymodel.h"
#include "gamedirectories.h"
#include "unitactionplayblockinganimation.h"

BSmallDynamicSimArray<BModelOpacityAnim> BUnitActionDome::mModelOpacityAnimArray;
float BUnitActionDome::mHole1MaxAnimTime = 0.0f;
float BUnitActionDome::mHole2MaxAnimTime = 0.0f;
float BUnitActionDome::mHole3MaxAnimTime = 0.0f;
float BUnitActionDome::mFastHoleMaxAnimTime = 0.0f;
int BUnitActionDome::mMeshAnimDataRefCount = 0;
int BUnitActionDome::mNumMeshes = 0;

//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BUnitActionDome, 1, &gSimHeap);

//==============================================================================
//==============================================================================
BUnitActionDome::~BUnitActionDome()
{
   // Release mesh visuals in case it wasn't done in disconnect
   for (int i = 0; i < mMeshVisuals.getNumber(); i++)
   {
      BVisual* pVis = mMeshVisuals[i];
      gVisualManager.releaseVisual(pVis);
   }
   mMeshVisuals.clear();
}

//==============================================================================
//==============================================================================
bool BUnitActionDome::init(void)
{
   if (!BAction::init())
      return(false);

   mAnimTime = 0.0f;
   mHoleState = cStateHoleIdle;

   return (true);
}

//==============================================================================
//==============================================================================
bool BUnitActionDome::connect(BEntity* pOwner, BSimOrder* pOrder)
{
   if (!BAction::connect(pOwner, pOrder))
      return(false);

   BObject* pObject = mpOwner->getObject();
   BASSERT(pObject);

   // Initialize static anim data if not done so already
   // Inc refcount to static data
   bool loaded = true;
   if (mMeshAnimDataRefCount == 0)
      loaded = initMeshAnimData(pObject);
   if (!loaded)
      return false;
   mMeshAnimDataRefCount++;

   // Setup the render override callback
   if (pObject)
      pObject->setOverrideRenderCallback(BUnitActionDome::renderStatic);

   // Allocate a new visual for each animating mesh
   BVisual* pBaseVis = pObject->getVisual();
   if (pBaseVis)
   {
      BMatrix mtx;
      pObject->getWorldMatrix(mtx);

      mMeshVisuals.setNumber(mNumMeshes);
      for (int i = 0; i < mNumMeshes; i++)
      {
         BVisual* pNewVis = gVisualManager.createVisual(pBaseVis, false, 0, cDWORDWhite, mtx);
         mMeshVisuals.setAt(i, pNewVis);
      }
   }

   // Set in wait state
   setState(cStateWait);

   return(true);
}

//==============================================================================
//==============================================================================
void BUnitActionDome::disconnect()
{
   // Decrement ref count on static data and clear if there are no more references
   mMeshAnimDataRefCount--;
   if (mMeshAnimDataRefCount == 0)
   {
      mModelOpacityAnimArray.clear();
      mHole1MaxAnimTime = 0.0f;
      mHole2MaxAnimTime = 0.0f;
      mHole3MaxAnimTime = 0.0f;
      mFastHoleMaxAnimTime = 0.0f;
      mNumMeshes = 0;
   }
   mMeshAnimDataRefCount = Math::Max(0, mMeshAnimDataRefCount); // don't let refcount go below 0

   // Release mesh visuals
   for (int i = 0; i < mMeshVisuals.getNumber(); i++)
   {
      BVisual* pVis = mMeshVisuals[i];
      gVisualManager.releaseVisual(pVis);
   }
   mMeshVisuals.clear();

   BAction::disconnect();
}


//==============================================================================
//==============================================================================
bool BUnitActionDome::setState(BActionState state)
{
   BObject* pObject = mpOwner->getObject();
   BASSERT(pObject);
   if (!pObject)
      return false;

   switch (state)
   {
      case cStateNone:
      case cStateWait:
         break;

      case cStateWorking:
         break;
   }
   return BAction::setState(state);
}


//==============================================================================
//==============================================================================
bool BUnitActionDome::update(float elapsed)
{
   BASSERT(mpOwner);
   if (mpProtoAction->getFlagDisabled())
      return (true);

   BObject* pObject = mpOwner->getObject();
   BASSERT(pObject);

   // Check animation state for notifying when to change hole anim state
   long animType = pObject->getAnimationType(cActionAnimationTrack);
   switch (animType)
   {
      case cAnimTypeWalk:
         mHoleState = cStateHole1Forward;
         setState(cStateWorking);
         break;
      case cAnimTypeJog:
         mHoleState = cStateHole2Forward;
         setState(cStateWorking);
         break;
      case cAnimTypeRun:
         mHoleState = cStateHole3Forward;
         setState(cStateWorking);
         break;
      case cAnimTypeWalkIdle:
         mHoleState = cStateHole1Reverse;
         setState(cStateWorking);
         break;
      case cAnimTypeJogIdle:
         mHoleState = cStateHole2Reverse;
         setState(cStateWorking);
         break;
      case cAnimTypeRunIdle:
         mHoleState = cStateHole3Reverse;
         setState(cStateWorking);
         break;
      case cAnimTypeTrain:
         mHoleState = cStateFastHoleForward;
         setState(cStateWorking);
         break;
      case cAnimTypeResearch:
         mHoleState = cStateFastHoleReverse;
         setState(cStateWorking);
         break;
      default:
         mHoleState = cStateHoleIdle;
         setState(cStateWait);
         break;
   }

   // Update
   switch (mState)
   {
      case cStateWorking:
         updateAnimTime(elapsed);
         break;
   }

   if(!BAction::update(elapsed))
      return (false);
   return (true);
}

//==============================================================================
//==============================================================================
void BUnitActionDome::render(BVisualRenderAttributes* pRenderAttributes, DWORD subUpdate, DWORD subUpdatesPerUpdate, float elapsedSubUpdateTime)
{
   BObject* pObject = mpOwner->getObject();
   BASSERT(pObject);

   // If the hole is idling closes, just draw the parent visual
   if (mHoleState == cStateHoleIdle)
   {
      BVisual* pVisual = pObject->getVisual();
      if (!pVisual)
         return;
      pVisual->render(pRenderAttributes);
      return;
   }

   // For this subupdate, calc elapsed time since last real update.  Reverse the time if we're animating backward.  And clamp the times as necessary.
   float animTime = mAnimTime;
   float updateCompletion = gWorld->getUpdateCompletion(pObject->getSubUpdateNumber());
   float elapsedTimeSinceLastUpdate = gWorld->getLastUpdateLengthFloat() * updateCompletion;
   switch (mHoleState)
   {
      case cStateHole1Forward:
      {
         animTime += elapsedTimeSinceLastUpdate;
         animTime = Math::Min(animTime, mHole1MaxAnimTime);
         break;
      }
      case cStateHole2Forward:
      {
         animTime += elapsedTimeSinceLastUpdate;
         animTime = Math::Min(animTime, mHole2MaxAnimTime);
         break;
      }
      case cStateHole3Forward:
      {
         animTime += elapsedTimeSinceLastUpdate;
         animTime = Math::Min(animTime, mHole3MaxAnimTime);
         break;
      }
      case cStateFastHoleForward:
      {
         animTime += elapsedTimeSinceLastUpdate;
         animTime = Math::Min(animTime, mFastHoleMaxAnimTime);
         break;
      }
      case cStateHole1Reverse:
      case cStateHole2Reverse:
      case cStateHole3Reverse:
      case cStateFastHoleReverse:
      {
         animTime -= elapsedTimeSinceLastUpdate;
         animTime = Math::Max(animTime, 0.0f);
         break;
      }
   }

   // Otherwise, it is animating and we need to draw the individual meshes at their animated opacities

   // Get anim index from hole state
   int animIndex;
   if (mHoleState >= cStateHole1Forward && mHoleState <= cStateHole3Reverse)
      animIndex = 0;
   else
      animIndex = 1;

   BModelOpacityAnim& modelAnim = mModelOpacityAnimArray[animIndex];

   BASSERT(mNumMeshes == mMeshVisuals.getNumber());
   for (int i = 0; i < modelAnim.getNumber(); i++)
   {
      BMeshOpacityAnim& meshAnim = modelAnim[i];
      if (meshAnim.mMeshIndex < 0)
         continue;

      // Get visual and granny instance for this mesh
      BVisual* pMeshVisual = mMeshVisuals[meshAnim.mMeshIndex];
      if (!pMeshVisual)
         continue;
      BGrannyInstance* pGrannyInstance = pMeshVisual->getGrannyInstance();
      if (!pGrannyInstance)
         return;

      // Get interpolated opacity
      float opacity;
      meshAnim.mOpacityTable.interp(animTime, opacity);

      // Skip if invisible
      if (opacity <= 0.0f)
         continue;

      // Set only this mesh visible
      pGrannyInstance->clearMeshRenderMask();
      pGrannyInstance->setMeshVisible(meshAnim.mMeshIndex, true);

      // Set overall alpha
      DWORD alpha = static_cast<DWORD>(Math::Clamp(opacity * 0.01f * 255.0f, 0.0f, 255.0f));
      pRenderAttributes->mTintColor = D3DCOLOR_ARGB(alpha, 0, 0, 0);

      // Render
      pMeshVisual->render(pRenderAttributes);
   }
}

//==============================================================================
//==============================================================================
void BUnitActionDome::renderStatic(BObject* pObject, BVisualRenderAttributes* pRenderAttributes, DWORD subUpdate, DWORD subUpdatesPerUpdate, float elapsedSubUpdateTime)
{
   if (!pObject)
      return;

   BUnitActionDome* pDomeAction = reinterpret_cast<BUnitActionDome*>(pObject->getActionByType(BAction::cActionTypeUnitDome));
   if (pDomeAction)
      pDomeAction->render(pRenderAttributes, subUpdate, subUpdatesPerUpdate, elapsedSubUpdateTime);
}

//==============================================================================
//==============================================================================
bool BUnitActionDome::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;
   GFWRITEVAR(pStream, float, mAnimTime);
   GFWRITEVAR(pStream, BYTE, mHoleState);
   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionDome::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;
   GFREADVAR(pStream, float, mAnimTime);
   GFREADVAR(pStream, BYTE, mHoleState);

   // Do some initialization
   // Initialize static anim data if not done so already
   BObject* pObject = mpOwner->getObject();
   BASSERT(pObject);
   bool loaded = true;
   if (mMeshAnimDataRefCount == 0)
      loaded = initMeshAnimData(pObject);
   if (!loaded)
      return false;
   mMeshAnimDataRefCount++;

   // Setup the render override callback
   if (pObject)
      pObject->setOverrideRenderCallback(BUnitActionDome::renderStatic);

   // Allocate a new visual for each animating mesh
   BVisual* pBaseVis = pObject->getVisual();
   if (pBaseVis)
   {
      BMatrix mtx;
      pObject->getWorldMatrix(mtx);

      mMeshVisuals.setNumber(mNumMeshes);
      for (int i = 0; i < mNumMeshes; i++)
      {
         BVisual* pNewVis = gVisualManager.createVisual(pBaseVis, false, 0, cDWORDWhite, mtx);
         mMeshVisuals.setAt(i, pNewVis);
      }
   }

   return true;
}

//==============================================================================
//==============================================================================
void BUnitActionDome::updateAnimTime(float elapsed)
{
   BObject* pObject = mpOwner->getObject();
   BASSERT(pObject);
   BVisual* pVisual = pObject->getVisual();
   if (!pVisual)
      return;
   BGrannyInstance* pGrannyInstance = pVisual->getGrannyInstance();
   if (!pGrannyInstance)
      return;

   // Update anim time based on hole type and forward/reverse
   switch (mHoleState)
   {
      case cStateHole1Forward:
      {
         mAnimTime += elapsed;
         if (mAnimTime >= mHole1MaxAnimTime)
         {
            mAnimTime = mHole1MaxAnimTime;
            setState(cStateWait);
         }
         break;
      }
      case cStateHole2Forward:
      {
         mAnimTime += elapsed;
         if (mAnimTime >= mHole2MaxAnimTime)
         {
            mAnimTime = mHole2MaxAnimTime;
            setState(cStateWait);
         }
         break;
      }
      case cStateHole3Forward:
      {
         mAnimTime += elapsed;
         if (mAnimTime >= mHole3MaxAnimTime)
         {
            mAnimTime = mHole3MaxAnimTime;
            setState(cStateWait);
         }
         break;
      }
      case cStateFastHoleForward:
      {
         mAnimTime += elapsed;
         if (mAnimTime >= mFastHoleMaxAnimTime)
         {
            mAnimTime = mFastHoleMaxAnimTime;
            setState(cStateWait);
         }
         break;
      }
      case cStateHole1Reverse:
      case cStateHole2Reverse:
      case cStateHole3Reverse:
      case cStateFastHoleReverse:
      {
         mAnimTime -= elapsed;
         if (mAnimTime <= 0.0f)
         {
            mAnimTime = 0.0f;
            pObject->unlockAnimation();
            pObject->setAnimationState(BObjectAnimationState::cAnimationStateIdle, cAnimTypeIdle, true, true);
            pObject->computeAnimation();
            setState(cStateWait);
         }
         break;
      }
   }
}

//==============================================================================
//==============================================================================
bool BUnitActionDome::initMeshAnimData(BObject* pObject)
{
   if (!pObject)
      return false;
   BVisual* pVisual = pObject->getVisual();
   if (!pVisual)
      return false;
   BGrannyInstance* pGrannyInstance = pVisual->getGrannyInstance();
   if (!pGrannyInstance)
      return false;
   BGrannyModel* pGrannyModel = gGrannyManager.getModel(pGrannyInstance->getModelIndex(), true);
   if (!pGrannyModel)
      return false;

   // Reset anim data
   mModelOpacityAnimArray.setNumber(0);

   // Load anim 1
   bool result = loadMeshAnim("effects\\shield_dome\\shield_flicker_animation_01.xml", pGrannyModel);
   if (!result)
   {
      mModelOpacityAnimArray.setNumber(0);
      return false;
   }

   // Set anim times for first anim
   mHole3MaxAnimTime = 0.0f;
   BModelOpacityAnim& modelAnim = mModelOpacityAnimArray[0];
   for (int i = 0; i < modelAnim.getNumber(); i++)
   {
      mHole3MaxAnimTime = Math::Max(mHole3MaxAnimTime, modelAnim[i].mOpacityTable.getLastKey());
   }
   mHole2MaxAnimTime = mHole3MaxAnimTime * 0.667f;
   mHole1MaxAnimTime = mHole3MaxAnimTime * 0.333f;

   // Load anim 2
   result = loadMeshAnim("effects\\shield_dome\\shield_fade_fast_01.xml", pGrannyModel);
   if (!result)
   {
      mModelOpacityAnimArray.setNumber(0);
      return false;
   }

   // Set anim times for second anim
   mFastHoleMaxAnimTime = 0.0f;
   BModelOpacityAnim& modelAnim2 = mModelOpacityAnimArray[1];
   for (int i = 0; i < modelAnim2.getNumber(); i++)
   {
      mFastHoleMaxAnimTime = Math::Max(mFastHoleMaxAnimTime, modelAnim2[i].mOpacityTable.getLastKey());
   }

   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionDome::loadMeshAnim(const char* filename, BGrannyModel* pGrannyModel)
{
   const BXMLReader* pReader = gDatabase.getPreloadXmlFile(cDirArt, filename);
   if(!pReader)
   {
      BASSERT(0);
      return false;
   }

   BModelOpacityAnim& modelAnim = mModelOpacityAnimArray.grow();
   modelAnim.setNumber(0);

   mNumMeshes = 0;
   BXMLNode root=pReader->getRootNode();
   int numChildren = root.getNumberChildren();
   for (int i = 0; i < numChildren; i++)
   {
      BXMLNode& child = root.getChild(i);
      if (child.getName() == "object")
      {
         // Get mesh index from name of object
         BSimString name;
         child.getAttribValueAsString("name", name);
         int meshIndex = pGrannyModel->getMeshIndex(name);
         if (meshIndex < 0)
            continue;
         
         BMeshOpacityAnim& newMeshAnim = modelAnim.grow();
         newMeshAnim.mMeshIndex = meshIndex;
         newMeshAnim.mOpacityTable.load(child);
         mNumMeshes = Math::Max(mNumMeshes, meshIndex + 1);
      }
   }

   return true;
}
