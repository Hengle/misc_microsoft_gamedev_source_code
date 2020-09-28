//==============================================================================
// terraineffect.cpp
//
// Copyright (c) 2001-2007, Ensemble Studios
//==============================================================================

#include "common.h"
#include "terraineffect.h"
#include "terraineffectmanager.h"

#include "gamedirectories.h"
#include "particlegateway.h"
#include "database.h"


#include "terrainsimrep.h"
#include "terrainimpactdecal.h"
#include "world.h"
#include "worldsoundmanager.h"
#include "lightEffectManager.h"
#include "terraintraildecal.h"

// xsystem
#include "reloadManager.h"


//============================================================================
// BTerrainEffectItem::BTerrainEffectItem
//============================================================================
BTerrainEffectItem::BTerrainEffectItem(void) :
   mParticleEffectName(),
   mParticleEffectHandle(-1),
   mpImpactDecal(NULL),
   mProtoVisName(),
   mProtoVisID(-1),
   mLightEffectName(),
   mLightEffectHandle(-1),
   mLightEffectLifespan(0.0f),
   mWeightValue(1),
   mDecalTrailParms(0)
{
}


//============================================================================
// BTerrainEffectItem::~BTerrainEffectItem
//============================================================================
BTerrainEffectItem::~BTerrainEffectItem()
{
   if(mpImpactDecal)
   {
      delete mpImpactDecal;
      mpImpactDecal = NULL;
   }

   if(mDecalTrailParms)
   {
      delete mDecalTrailParms; 
      mDecalTrailParms = NULL;
   }
}


//============================================================================
// bool BTerrainEffectItem::load
//============================================================================
bool BTerrainEffectItem::loadXML(const BXMLNode &root)
{
   // Weight.
   mWeightValue = 1;
   root.getAttribValueAsDWORD("weight", mWeightValue);
   if(mWeightValue == 0)
      mWeightValue = 1;

   // Walk children.
   for(long i=0; i<root.getNumberChildren(); i++)
   {
      // Get child.
      BXMLNode child(root.getChild(i));
      if(!child)
         continue;

      if(child.getName().compare("particlefile") == 0)
      {
         child.getText(mParticleEffectName);
         //gParticleGateway.getOrCreateData(mParticleEffectName.getPtr(), (BParticleEffectDataHandle&)mParticleEffectHandle);
      }
      else if(child.getName().compare("impactdecal") == 0)
      {
         BString decalTextureName;
         child.getText(decalTextureName);

         mpImpactDecal = new BTerrainImpactDecalHandle;
         mpImpactDecal->mImpactTextureName = decalTextureName;

         child.getAttribValueAsFloat("sizeX", mpImpactDecal->mSizeX);
         child.getAttribValueAsFloat("sizeZ", mpImpactDecal->mSizeZ);
         child.getAttribValueAsFloat("opaque", mpImpactDecal->mTimeFullyOpaque);
         child.getAttribValueAsFloat("fadeouttime", mpImpactDecal->mFadeOutTime);

         BSimString orientationStr;
         child.getAttribValueAsString("orientation", orientationStr);

         if(orientationStr.compare("random") == 0)
            mpImpactDecal->mOrientation = false;
         else
            mpImpactDecal->mOrientation = true;

         //gImpactDecalManager.loadImpactDecal(decalTextureName);
      }
      else if(child.getName().compare("sound") == 0)
      {
         BString soundName;
         child.getText(soundName);

         mSoundName = soundName;
      }
      else if (child.getName().compare("vis") == 0)
      {
         child.getText(mProtoVisName);
         //mProtoVisID = gVisualManager.getOrCreateProtoVisual(mProtoVisName.getPtr(), false);
      }
      else if (child.getName().compare("light") == 0)
      {
         child.getText(mLightEffectName);

         //gLightEffectManager.getOrCreateData(mLightEffectName.getPtr(), mLightEffectHandle);
         child.getAttribValueAsFloat("lifespan", mLightEffectLifespan);
      }
      else if(child.getName().compare("trail") == 0)
      {
         mDecalTrailParms = new BRibbonCreateParams();

         child.getText(mDecalTrailParms->mTextureName);

         child.getAttribValueAsFloat("minDistBetweenNodes", mDecalTrailParms->mMinDist);
         child.getAttribValueAsFloat("width", mDecalTrailParms->mRibbonWidth);
         child.getAttribValueAsInt("numFramesFullAlpha",mDecalTrailParms->mNodeAmountFramesFullAlpha);
         child.getAttribValueAsInt("numFramesFadeOut", mDecalTrailParms->mNodeAmountFramesFading);
      }
      else
         child.logInfo("'%S' is not a valid tag.", BStrConv::toA(child.getName()));
   }

   // Success.
   return(true);
}

//============================================================================
//============================================================================
void BTerrainEffectItem::loadAllAssets()
{
   if((mParticleEffectHandle == -1) && (!mParticleEffectName.isEmpty()))
   {
      gParticleGateway.getOrCreateData(mParticleEffectName.getPtr(), (BParticleEffectDataHandle&)mParticleEffectHandle);
   }

   if((mProtoVisID == -1) && (!mProtoVisName.isEmpty()))
   {
      mProtoVisID = gVisualManager.getOrCreateProtoVisual(mProtoVisName.getPtr(), true);
   }

   if((mParticleEffectHandle == -1) && (!mLightEffectName.isEmpty()))
   {
      gLightEffectManager.getOrCreateData(mLightEffectName.getPtr(), mParticleEffectHandle);
   }

   if(mpImpactDecal && (!mpImpactDecal->mImpactTextureName.isEmpty()))
   {
      gImpactDecalManager.loadImpactDecal(mpImpactDecal->mImpactTextureName);
   }

   if(mDecalTrailParms)
   {
      gTerrainRibbonManager.loadAssets(*mDecalTrailParms);
   }
}

//============================================================================
//============================================================================
void BTerrainEffectItem::unloadAllAssets()
{
   mParticleEffectHandle = -1;
   mProtoVisID = -1;
   mParticleEffectHandle = -1;
   // don't destroy the decal trail params because we will need it next time we reload the assets. Moved this to the destructor when this item truly goes away.
   //if(mDecalTrailParms){delete mDecalTrailParms; mDecalTrailParms = NULL;};
}


/*
//============================================================================
// BTerrainEffectItem::getSoundSet
//============================================================================
const BString &BTerrainEffectItem::getSoundSet(long index) const
{
   if(index<0 || index>=mSoundSets.getNumber())
      return(sEmptyString);
   return(mSoundSets[index]);
}
*/




//============================================================================
//============================================================================
BTerrainEffectItemArray::~BTerrainEffectItemArray()
{
   cleanup();
}

//============================================================================
//============================================================================
void BTerrainEffectItemArray::loadAllAssets()
{
   for (uint i = 0; i < cNumImpactEffectSizes + 1; i++)
   {
      for (int j = 0; j < mItemsBySize[i].getNumber(); j++)
      {
         mItemsBySize[i][j]->loadAllAssets();
      }
   }
}

//============================================================================
//============================================================================
void BTerrainEffectItemArray::unloadAllAssets()
{
   for (uint i = 0; i < cNumImpactEffectSizes + 1; i++)
   {
      for (int j = 0; j < mItemsBySize[i].getNumber(); j++)
      {
         mItemsBySize[i][j]->unloadAllAssets();
      }
   }
}

//============================================================================
//============================================================================
void BTerrainEffectItemArray::cleanup()
{
   for (uint i = 0; i < cNumImpactEffectSizes + 1; i++)
   {
      for (int j = 0; j < mItemsBySize[i].getNumber(); j++)
      {
         delete mItemsBySize[i][j];
      }
      mItemsBySize[i].setNumber(0);
   }
}

//============================================================================
//============================================================================
void BTerrainEffectItemArray::addItem(uint sizeID, BTerrainEffectItem* pItem)
{
   if (sizeID > cNumImpactEffectSizes)
      sizeID = cNumImpactEffectSizes;
   mItemsBySize[sizeID].add(pItem);
}

//============================================================================
// BTerrainEffectItemArray::normalizeWeightValues
//
// "normalize" here should be in big Dusty-style air quotes since it isn't normalizing
// in the traditional sense
 //============================================================================
void BTerrainEffectItemArray::normalizeWeightValues(void)
{
   // Walk each type.
   for(long type=0; type<cNumImpactEffectSizes + 1; type++)
   {
      // Get the list for this type.
      BDynamicSimArray<BTerrainEffectItem*> &list = mItemsBySize[type];
      
      // Get count.
      long count = list.getNumber();

      // Don't bother if there are no entries.
      if(count <= 0)
         continue;

      // Sum the values.
      DWORD sum = 0;
      for(long i=0; i<count; i++)
      {
         if(!list[i])
            continue;

         // Get user's weight.
         DWORD weight = list[i]->getWeightValue();

         // Hey, no zero weights -- that freaks us out.
         if(weight == 0)
            weight = 1;

         // Add user weight to total.
         sum += weight;

         // New "normalized" weight is the sum of the previous weights.
         list[i]->setWeightValue(sum);
      }
   }
}

//============================================================================
//============================================================================
const BDynamicSimArray<BTerrainEffectItem*>& BTerrainEffectItemArray::getItemsBySize(long sizeID) const
{
   // Use default items if the sizeID is invalid or if the specific size has no items
   if (sizeID < 0 || sizeID > cNumImpactEffectSizes)
      sizeID = cNumImpactEffectSizes;
   else if (mItemsBySize[sizeID].getNumber() <= 0)
      sizeID = cNumImpactEffectSizes;

   return mItemsBySize[sizeID];
}



//============================================================================
// BTerrainEffect::BTerrainEffect
//============================================================================
BTerrainEffect::BTerrainEffect(void) :
   mFlags(),
   mID(-1),
   mMeterLength(0),
   mMeterCount(0)
{
#ifdef ENABLE_RELOAD_MANAGER
   eventReceiverInit(cThreadIndexSim, true);
#endif

   mItems.setNumber(gDatabase.getNumberSurfaceTypes() + 1);
}


//============================================================================
// BTerrainEffect::~BTerrainEffect
//============================================================================
BTerrainEffect::~BTerrainEffect()
{
   cleanup();
#ifdef ENABLE_RELOAD_MANAGER
   eventReceiverDeinit(true);
#endif
}


//==============================================================================
// BTerrainEffect::init
//==============================================================================
bool BTerrainEffect::init(const BCHAR_T* pName, int id)
{
   mName=pName;
   mID = id;

   return true;
}


//============================================================================
// BTerrainEffect::load
//============================================================================
bool BTerrainEffect::load()
{
   if(getFlag(cFlagLoadFailed))
      return false;


   BSimString fileNameWithExtension = mName;
   strPathAddExtension(fileNameWithExtension, "tfx");


#ifdef ENABLE_RELOAD_MANAGER
   gReloadManager.deregisterClient(mEventHandle);

   BReloadManager::BPathArray paths;
   BString fullpath;
   gFileManager.constructQualifiedPath(cDirArt, fileNameWithExtension, fullpath);
   paths.pushBack(fullpath);
   gReloadManager.registerClient(paths, BReloadManager::cFlagSynchronous, mEventHandle);
#endif


   BXMLReader reader;
   bool ok = reader.load(cDirArt, fileNameWithExtension, XML_READER_LOAD_DISCARD_ON_CLOSE);
   if (!ok)
   {
      setFlag(cFlagLoadFailed, true);
      return false;
   }

   //-- Get the root node.
   BXMLNode rootNode(reader.getRootNode());
   if (rootNode.getNumberChildren() <= 0)
   {
      setFlag(cFlagLoadFailed, true);
      return false;
   }

   if (!loadXML(rootNode))
   {
      setFlag(cFlagLoadFailed, true);
      return false;
   }

   setFlag(cFlagIsLoaded, true);

   return true;
}

//============================================================================
// BTerrainEffect::reload
//============================================================================
bool BTerrainEffect::reload(void)
{
   if (mName.isEmpty())
      return false;

   cleanup();
   
   return load();  
}

//============================================================================
// BTerrainEffect::loadXML
//============================================================================
bool BTerrainEffect::loadXML(const BXMLNode &root)
{
   // Walk children.
   for(long i=0; i<root.getNumberChildren(); i++)
   {
      // Get child.
      BXMLNode child(root.getChild(i));
      if(!child)
         continue;

      // Get the surface type.
      byte surfaceType = gDatabase.getSurfaceType(child.getName());
      if(surfaceType == 0)
      {
         if (child.getName().compare("default") == 0)
         {
            surfaceType = gDatabase.getNumberSurfaceTypes();
         }
         else
         {
            child.logInfo("'%S' is not a invalid impact type or 'default'.", child.getName());
            continue;
         }
      }

      // Create a new impact item.
      BTerrainEffectItem *newItem = new BTerrainEffectItem;
      if(!newItem)
      {
         BFAIL("Memory allocation error.");
         continue;
      }

      // Load it.
      bool ok = newItem->loadXML(child);
      if(!ok)
         continue;

      // Add it to the proper list.
      uint sizeID = cNumImpactEffectSizes; // default is the generic ID (non-size specific)
      BSimString sizeStr;
      if (child.getAttribValueAsString("size", sizeStr))
      {
         long impactEffectSizeID = gDatabase.getImpactEffectSize(sizeStr.asNative());
         if (impactEffectSizeID >= 0 && impactEffectSizeID < cNumImpactEffectSizes)
            sizeID = (uint) impactEffectSizeID;
      }
      mItems[surfaceType].addItem(sizeID, newItem);
   }

   // Fixup weights.
   normalizeWeightValues();

   // Success.
   return(true);
}



//==============================================================================
// BTerrainEffect::loadAllAssets
//==============================================================================
void BTerrainEffect::loadAllAssets()
{
   SCOPEDSAMPLE(BProtoVisual_loadAllAssets)
   if(getFlag(cFlagAreAllAssetsLoaded))
      return;

   for(long type = 0; type<(gDatabase.getNumberSurfaceTypes() + 1); type++)
   {
      mItems[type].loadAllAssets();
   }

   setFlag(cFlagAreAllAssetsLoaded, true);
}


//==============================================================================
// BTerrainEffect::unloadAllAssets
//==============================================================================
void BTerrainEffect::unloadAllAssets()
{
   if(!getFlag(cFlagAreAllAssetsLoaded))
      return;

   for(long type = 0; type<(gDatabase.getNumberSurfaceTypes() + 1); type++)
   {
      mItems[type].unloadAllAssets();
   }

   setFlag(cFlagAreAllAssetsLoaded, false);
}


//============================================================================
// BTerrainEffect::cleanup
//============================================================================
void BTerrainEffect::cleanup(void)
{
   // Nuke items.
   for(long type = 0; type<(gDatabase.getNumberSurfaceTypes() + 1); type++)
   {
      mItems[type].cleanup();
   }
}


//============================================================================
// BTerrainEffect::getItem
//============================================================================
BTerrainEffectItem *BTerrainEffect::getItem(byte surfaceType, long sizeID, long randTag) const
{
   return getItemByIndex(surfaceType, sizeID, getRandomItemIndex(surfaceType, sizeID, randTag));
}


//============================================================================
// BTerrainEffect::getItemByIndex
//============================================================================
BTerrainEffectItem *BTerrainEffect::getItemByIndex(byte surfaceType, long sizeID, long index) const
{
   // Range check.
   if(surfaceType<0 || surfaceType>=(gDatabase.getNumberSurfaceTypes() + 1))
      return(NULL);

   // Get the list for this type.
   const BDynamicSimArray<BTerrainEffectItem*> &list = mItems[surfaceType].getItemsBySize(sizeID);

   // Check for empty list.
   long count = list.getNumber();
   if(count <= 0)
      return(NULL);

   // check index
   if(index < 0 || index >= count)
      return(NULL);

   // return the item
   return list[index];
}


//============================================================================
// BTerrainEffect::getNumItems
//============================================================================
long BTerrainEffect::getNumItems(byte surfaceType, long sizeID) const
{
   // Range check.
   if(surfaceType<0 || surfaceType>=(gDatabase.getNumberSurfaceTypes() + 1))
      return(NULL);

   // Get the list for this type.
   const BDynamicSimArray<BTerrainEffectItem*> &list = mItems[surfaceType].getItemsBySize(sizeID);

   // return the number of items
   return list.getNumber();
}

//============================================================================
// BTerrainEffect::getRandomItemIndex
//============================================================================
long BTerrainEffect::getRandomItemIndex(byte surfaceType, long sizeID, long randTag) const
{
   // Range check.
   if(surfaceType<0 || surfaceType>=(gDatabase.getNumberSurfaceTypes() + 1))
      return -1;

   // Get the list for this type.
   const BDynamicSimArray<BTerrainEffectItem*> &list = mItems[surfaceType].getItemsBySize(sizeID);

   // Check for empty list or bad weightings.
   long count = list.getNumber();
   if(count <= 0)
      return -1;

   // Get weight sum.
   DWORD weightSum = list[count-1]->getWeightValue();

   // Roll a rand between 0 and sum.
   DWORD randVal = getRandRange(randTag, 0, weightSum);

   // Walk the list until we meet/exceed the weight value.  We skip the last one and return it if we 
   // fall through.
   for(long i=0; i<count-1; i++)
   {
      if(randVal <= list[i]->getWeightValue())
         return(i);
   }
   return(count-1);
}


//============================================================================
//============================================================================
void BTerrainEffect::normalizeWeightValues(void)
{
   for(long type=0; type<(gDatabase.getNumberSurfaceTypes() + 1); type++)
   {
      mItems[type].normalizeWeightValues();
   }
}

//============================================================================
// BTerrainEffect::getParticleEffectHandleForType
//============================================================================
long BTerrainEffect::getParticleEffectHandleForType(byte surfaceType, long sizeID)
{
   BTerrainEffectItem *impactEffectItem = getItemByIndex(surfaceType, sizeID, 0);
   if(!impactEffectItem)
      return -1;

   long pfxHandle = impactEffectItem->getParticleIndexHandle();

   return(pfxHandle);
}

//============================================================================
// BTerrainEffect::getTrailDecalHandleForType
//============================================================================
BRibbonCreateParams* BTerrainEffect::getTrailDecalHandleForType(byte surfaceType, long sizeID)
{
   BTerrainEffectItem *impactEffectItem = getItemByIndex(surfaceType, sizeID, 0);
   if(!impactEffectItem)
      return NULL;

   return impactEffectItem->getTrailParams();
}



//============================================================================
// BTerrainEffect::instantiateEffect
//============================================================================
void BTerrainEffect::instantiateEffect(byte surfaceType, long sizeID, const BVector &pos, const BVector &dir, bool visible, BPlayerID playerID,float lifespan, bool visibleToAll, int displayPriority, BObject** ppReturnObject) //, bool usePrimaryEffect, BUnit* pAttachToUnit, long dummyIndex)
{
   // Check if effect metering will let us play this one.  If not, we're done.
   bool okToPlay = gTerrainEffectManager.checkMeter(getID(), pos);
   if(!okToPlay)
      return;

   // Get ImpactEffectItem
   long itemIndex = getRandomItemIndex(surfaceType, sizeID, cUnsyncedRand);
   BTerrainEffectItem *impactEffectItem = getItemByIndex(surfaceType, sizeID, itemIndex);
   if(!impactEffectItem)
   {
      // Check for default surface type impactEffectItem
      byte defaultSurfaceTypeID = gDatabase.getNumberSurfaceTypes();
      itemIndex = getRandomItemIndex(defaultSurfaceTypeID, sizeID, cUnsyncedRand);
      impactEffectItem = getItemByIndex(defaultSurfaceTypeID, sizeID, itemIndex);
      if (!impactEffectItem)
         return;
   }


   // Do sound
   BSimString soundName =impactEffectItem->getSoundName();
   if(!soundName.isEmpty())
   {      
      // TODO:  need to store sound index ahead of time instead of always querying it. (SAT)
      BCueIndex index = gSoundManager.getCueIndex(soundName);
      gWorld->getWorldSoundManager()->addSound(pos, index, true, cInvalidCueIndex, true, true);
   }


   // Do particle effect
   long pfxHandle = impactEffectItem->getParticleIndexHandle();
   if(pfxHandle != -1)
   {
      BMatrix worldMatrix;
      worldMatrix.makeOrient(dir, cYAxisVector, cYAxisVector.cross(dir)); 
      worldMatrix.setTranslation(pos);

      BParticleCreateParams params;
      params.mDataHandle        = pfxHandle;
      params.mMatrix            = worldMatrix;
      params.mNearLayerEffect   = false;
      params.mTintColor         = cDWORDWhite;

      // TODO:  Fix this here.  This is dangerous if the pfx is looping since it will never
      //        be release.  Need BK to make better interface for fire and forget particles. (SAT)
      // Note: The autoRelease instance will correctly release sim instance data, but looping
      // pfx will still stick around.
      gParticleGateway.createAutoReleaseInstance(params);
   }


   // Do decal
   const BTerrainImpactDecalHandle* pImpactDecal = impactEffectItem->getImpactDecalHandle();
   if(pImpactDecal)
   {
      gImpactDecalManager.createImpactDecal(pos, pImpactDecal, dir);
   }

   // Do light
   long lightHandle = impactEffectItem->getLightEffectHandle();
   float lightLifespan = impactEffectItem->getLightEffectLifespan();
   if ((lightHandle >= 0) && (lightLifespan > 0.0f))
   {
      BMatrix mtx;
      mtx.makeTranslate(pos);
      DWORD lightEndTime = gWorld->getGametime() + static_cast<DWORD>(lightLifespan * 1000.0f);
      gLightEffectManager.createTimedInstance(lightHandle, mtx, lightEndTime);
   }

   // Do visual
   long visID = impactEffectItem->getProtoVisID();
   if (visID != -1)
   {
      BObjectCreateParms parms;
      parms.mPlayerID = playerID;
      parms.mPosition = pos;
      parms.mRight = cYAxisVector.cross(dir);
      parms.mForward = dir;
      BObject* pImpactObject = gWorld->createTempObject(parms, visID, lifespan, sizeID, visibleToAll, displayPriority);
      if (pImpactObject)
         pImpactObject->setFlagIsImpactEffect(true);
      if (ppReturnObject)
         *ppReturnObject = pImpactObject;
   }
   
   /*
   long itemIndex = getRandomItemIndex(impactType, cUnsyncedRand);
   BTerrainEffectItem *impactEffectItem = getItemByIndex(impactType, itemIndex);
   if(!impactEffectItem)
      return -1;

   if(game->getSoundManager() && game->getSoundManager()->getSoundSetManager())
   {
      BSoundParams params;
      params.setFlag(BSoundParams::cFlagCheckVisible, true);
      params.setFlag(BSoundParams::cFlagPositional, true);
      //params.enable3D();
      for(long i=0; i<impactEffectItem->getSoundSetCount(); i++)
      {
         BHandle soundHandle = game->getSoundManager()->getSoundSetManager()->getHandle(impactEffectItem->getSoundSet(i));
         BSoundSet *pSet = game->getSoundManager()->getSoundSetManager()->getSet(soundHandle);
         if(pSet)
            params.setDistance(pSet->getDistance());
         game->getSoundManager()->playPositionalSoundSet(-1, pos, soundHandle, &params);
      }
   }
   
   // If not visible, our work is done here.
   if(!visible)
      return -1;

   // Check if effect metering will let us play this one.  If not, we're done.
   bool okToPlay = checkMeter();
   if(!okToPlay)
      return -1;
      
   // MS 3/2/2005: god this system sucks, but it's time to move on to other stuff...
   // hopefully this can be fixed up sometime. Ideally, the model system would gain
   // the capability of having particle systems or sets attached to it, which
   // would obviate the need to have this messed up intermediate BModel that
   // contains a particle set. Blah.

   // figure out the name for our effect (e.g., our particle set)
   BString effectName;
   if(usePrimaryEffect || impactEffectItem->getSecondaryParticleSet().length() <= 0)
      effectName = impactEffectItem->getParticleSet();
   else
      effectName = impactEffectItem->getSecondaryParticleSet();

   bool bIsParticleSet = gModelManager->getParticleSystemManager()->getParticleSet(effectName) != NULL;

   // if we're attaching to a unit and this is not a particle set, try to do that,
   // otherwise just create at requested position using the particle system manager
   long attachIndex = -1;
   if(!bIsParticleSet && effectName.length() > 0 && pAttachToUnit && pAttachToUnit->getCurrentModelInstance() && gModelManager)
   {
      // MS 8/16/2005: 12267
      if(!pAttachToUnit->isAlive())
         return -1;

      long modelIndex = gModelManager->loadModel(effectName);
      BModel* pModel = gModelManager->getModel(modelIndex);
      attachIndex = pAttachToUnit->getCurrentModelInstance()->addUserAttachment(0, 0, cDummyImpact, dummyIndex, pModel);
      if(attachIndex >= 0)
      {
         BModelAttachment* pA = pAttachToUnit->getCurrentModelInstance()->getUserAttachment(attachIndex);
         if(pA)
         {
            const DWORD cDefaultExpirationTime = 20000;
            pA->setExpirationTime(game->getWorld()->getGametime() + cDefaultExpirationTime);
         }
      }
   }
   if(attachIndex < 0)
   {
      BMatrix xform;
      xform.makeOrient(dir, cYAxisVector, cYAxisVector.cross(dir)); 
      xform.setTranslation(pos);
      gModelManager->getParticleSystemManager()->createEffect(effectName, xform, 0.0f, 1.0f);
   }

   if(pAttachToUnit && impactEffectItem->getFireExistTime() > 0)
   {
      pAttachToUnit->setFlag(BUnit::cOnFire, true);
      pAttachToUnit->setOnFireEndTime(game->getWorld()->getGametime() + impactEffectItem->getFireExistTime());
   }


   // Create uivisual with desired anim.
   if(impactEffectItem->getAnimID() >= 0)
   {
      // Create a new UIVisual.
      long visID = game->getWorld()->getUIVisualManager()->create();
      BUIVisual *vis = game->getWorld()->getUIVisualManager()->getUIVisual(visID);
      if(vis)
      {
         // Position.
         vis->setPosition(pos);
         game->getWorld()->getUIVisualManager()->addUIVisualToScene(visID);

         // Set model.
         vis->setModel(playerID, impactEffectItem->getAnimID());
         vis->setUseDecal(true);

         // Lifepsan if requested.
         float lifespan = impactEffectItem->getAnimLifespan();
         if(lifespan>0.0f)
         {
            // Set it.
            vis->setLifespan(lifespan);

            // Set alpha fading.
            float fadeTime = impactEffectItem->getAnimAlphaFadeTime();
            if(lifespan<fadeTime)
            {
               // Lifespan is shorter than fade time, so just use lifespan as fade time.
               vis->setAlphaFadeOut(lifespan, 0.0f);
            }
            else
            {
               // Fade out over fadetime, but delay until the end of lifespan.
               vis->setAlphaFadeOut(fadeTime, lifespan-fadeTime);
            }
         }
      }
   }

   return itemIndex;
   */
}


/*
//============================================================================
// BTerrainEffect::checkMeter
//============================================================================
bool BTerrainEffect::checkMeter(BVector pos)
{
   // Sanity.
   if(!game || !game->getWorld())
      return(true);

   // Bail if no metering for this effect.
   if((mMeterLength == 0) || (mMeterCount == 0))
      return(true);
      
   // Get current time.     
   DWORD currTime = game->getWorld()->getGametime();
   
   // Purge anything other than the time window.
   if(currTime > mMeterLength)
   {
      // Find the time which is too old to care about.
      DWORD tooOldTime = currTime - mMeterLength;
      
      BHandle hItem = NULL;
      DWORD *logTime = mMeterLog.getHead(hItem);
      while(logTime)
      {
         // If we've gotten to the stuff we still care about, bail out.
         if(*logTime > tooOldTime)
            break;
         
         // Remove old item and check the next.
         logTime = mMeterLog.removeAndGetNext(hItem);
      }
   }
   
   // If remaining count is too high, no effect allowed.
   DWORD currNum = mMeterLog.getSize();
   if(currNum >= mMeterCount)
      return(false);
   
   // Add a record of this new impact.
   mMeterLog.addToTail(currTime);
   
   // Let it play.
   return(true);
}

//============================================================================
// BTerrainEffect::clearMetering
//============================================================================
void BTerrainEffect::clearMetering(void)
{
   // Clear the metering log.
   mMeterLog.empty();
}
*/


//============================================================================
// BTerrainEffect::receiveEvent
//============================================================================
#ifdef ENABLE_RELOAD_MANAGER
bool BTerrainEffect::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   if (event.mEventClass == cEventClassReloadNotify)
   {
      gConsoleOutput.status("Reloading terrain effect: %s", mName.getPtr());
      
      // Reload animation
      reload();
   }

   return false;
}
#endif

//============================================================================
// eof: terraineffect.cpp
//============================================================================


