//============================================================================
// impacteffectmanager.h
// Ensemble Studios (C) 2008
//============================================================================

#include "common.h"
#include "impacteffectmanager.h"
#include "database.h"
#include "world.h"
#include "protoimpacteffect.h"
#include "TerrainVisual.h"
#include "debugTextDisplay.h"
#include "configsgame.h"
#include "visual.h"

BImpactEffectManager gImpactEffectManager;

//============================================================================
//============================================================================
BImpactEffectManager::BImpactEffectManager():
   mEnableLimits(true)
{
}

//============================================================================
//============================================================================
BImpactEffectManager::~BImpactEffectManager()
{
}

//============================================================================
//============================================================================
void BImpactEffectManager::init()
{
   //-- Load Impact Effect Object
   mImpactEffectObjectID = gDatabase.getProtoObject("fx_impact_effect_01");
}

//============================================================================
//============================================================================
void BImpactEffectManager::deinit()
{
   mImpactGrid.clear();
   mImpactEffects.clear();
}

//============================================================================
//============================================================================
void BImpactEffectManager::update(float elapsedTime)
{
}

//============================================================================
//============================================================================
void BImpactEffectManager::release(BObject* pObject)
{
   if (!pObject)
      return;

   const BVector pos(pObject->getPosition());

   BActiveImpactKey key;  
   initKey(pos, key);

   BImpactEffectGridHashMap::iterator it(mImpactGrid.find(key));
   
   if (it == mImpactGrid.end())
   {  
      BASSERT(!"The impact effect moved since it was created");
      return;
   }

   BActiveImpactValue* pValue = (BActiveImpactValue*) &it->second;
      
   uint numValues = pValue->mValues.getNumber();
   int objectIndex = cInvalidIndex;
   uint valueIndex;
   for (valueIndex = 0; valueIndex < numValues; valueIndex++)
   {
      objectIndex = pValue->mValues[valueIndex].mObjects.find(pObject);
      if (objectIndex != cInvalidIndex)
         break;
   }
   
   if (valueIndex >= numValues)
   {
      BASSERT(!"The impact effect moved since it was created");
      return;
   }
   
   BSmallDynamicSimArray<BObject*>& objPtrArray = pValue->mValues[valueIndex].mObjects;

   if (objectIndex != cInvalidIndex)
      objPtrArray.eraseUnordered(objectIndex);

   if (objPtrArray.isEmpty())
   {
      pValue->mValues.eraseUnordered(valueIndex);
      if (pValue->mValues.isEmpty())
      {
         mImpactGrid.erase(it);         
      }
   }
}

//============================================================================
//============================================================================
void BImpactEffectManager::addImpactEffect(const BImpactEffectData& effect)
{
   if (effect.mProtoID < 0)
      return;

   debugRangeCheck(effect.mProtoID, gDatabase.getNumberImpactEffects());

#ifndef BUILD_FINAL
   mStats.mNumImpactsSimTriedToCreate++;
#endif

   mImpactEffects.add(effect);
}

//============================================================================
//============================================================================
void BImpactEffectManager::clearImpactEffects()
{

}

//============================================================================
//============================================================================
void BImpactEffectManager::flushImpactEffects()
{
#ifndef BUILD_FINAL
   setEnableLimits(gConfig.isDefined(cConfigEnableImpactLimits));
#endif

   for (int i = 0; i < mImpactEffects.getNumber(); ++i)
   {
      createImpactEffect(mImpactEffects[i]);
   }
   
   mImpactEffects.resize(0);

#if 0
   static BVector scale((float) eVoxelXDim,(float) eVoxelYDim, (float) eVoxelZDim);
   BMatrix m;
   m.makeIdentity();
   m.setTranslation((float) eVoxelXDim, (float) eVoxelYDim * 0.5f, (float) eVoxelZDim);

   gpDebugPrimitives->addDebugBox(m, scale, cDWORDGreen);
#endif
}

//============================================================================
//============================================================================
void BImpactEffectManager::initKey(const BVector& pos, BActiveImpactKey& key) const
{
   key.set( 
         Math::FloatToIntTrunc( pos.x / eVoxelXDim), 
         Math::FloatToIntTrunc( pos.y / eVoxelYDim), 
         Math::FloatToIntTrunc( pos.z / eVoxelZDim) );
}

//============================================================================
//============================================================================
void BImpactEffectManager::createImpactEffect(const BImpactEffectData& data)
{


   const BProtoImpactEffect* pProtoData = gDatabase.getProtoImpactEffectFromIndex(data.mProtoID);
   if (!pProtoData)
      return;

   BActiveImpactKey key;
   initKey(data.mObjectParams.mPosition, key);

   const bool bEnableLimit = true;
   const bool bDrawDebugSpheres = false;
   int bLimit = pProtoData->mLimit;   
   
   BImpactEffectGridHashMap::const_iterator it = mImpactGrid.find(key);
   BActiveImpactValue* pValue = NULL;
   BActiveImpactInstance* pActiveInstance = NULL;
   if (it != mImpactGrid.end())
   {
      pValue = (BActiveImpactValue*) &it->second;
      for (int i = 0; i < pValue->mValues.getNumber(); i++)
      {
         if (pValue->mValues[i].mProtoID == data.mProtoID)
         {
            int count = pValue->mValues[i].mObjects.getNumber(); 

            //-- we have reached our limit don't create this impact
            if (mEnableLimits && (count >= bLimit))
            {
#ifndef BUILD_FINAL               
/*
               if (bDrawDebugSpheres)
                  gpDebugPrimitives->addDebugSphere(data.mObjectParams.mPosition, 5.0f, cDWORDRed);
*/
               mStats.mDroppedImpacts++;
#endif
               return;
            }

            pActiveInstance = &pValue->mValues[i];
         }
      }      
   }


   //-- if we got this far then we can create an impact.
   BObjectCreateParms params;
   params = data.mObjectParams;
   params.mProtoObjectID = mImpactEffectObjectID;
   params.mStartBuilt = true;
   params.mType = BEntity::cClassTypeObject;
   BObject* pObject = gWorld->createObject(params);
   if (pObject)
   {
#ifndef BUILD_FINAL
/*
      if (bDrawDebugSpheres)
         gpDebugPrimitives->addDebugSphere(data.mObjectParams.mPosition, 3.0f, cDWORDGreen);
*/
      mStats.mAddedImpacts++;
#endif

      pObject->setVisual(pProtoData->mProtoVisualIndex, pObject->getProtoObject()->getVisualDisplayPriority(), (int64) data.mUserdata);
      pObject->setLifespan((DWORD) Math::FloatToIntTrunc(pProtoData->mLifespan * 1000.0f));
      pObject->setFlagFadeOnDeath(true);
      pObject->setFlagVisibleToAll(data.mVisibleToAll);
      pObject->setFlagIsImpactEffect(true);

      //force the visual display priority of impact effects to be combat priority
      BVisual* pVisual = pObject->getVisual();
      if (pVisual)
         pVisual->setDisplayPriority(cVisualDisplayPriorityCombat);
      
      //-- if there is an existing entry in the hashmap then this should be valid
      if (it != mImpactGrid.end())
      {
         //-- its in the grid and we have one of the same type already then just add the object to that list.
         BDEBUG_ASSERT(pValue != NULL);
         if (pActiveInstance)
            pActiveInstance->mObjects.add(pObject);
         else
         {
            BActiveImpactInstance newInstance;
            newInstance.mProtoID = data.mProtoID;
            newInstance.mObjects.add(pObject);

            pValue->mValues.add(newInstance);
         }
      }
      else
      {
         // this is a brand new key add it.
         BActiveImpactInstance newInstance;
         newInstance.mProtoID = data.mProtoID;
         newInstance.mObjects.add(pObject);

         BActiveImpactValue newValue;
         newValue.mValues.add(newInstance);
         mImpactGrid.insert(key, newValue);         
      }            

      // Call impact effect object callback func
      if (data.mCallback)
         data.mCallback(pObject, data.mCallbackData);
   }
}
#ifndef BUILD_FINAL
//============================================================================
//============================================================================
void BImpactEffectManager::dumpVoxelData(BDebugTextDisplay& textDisplay) const
{
   textDisplay.printf(cColorCyan, "Voxel Data");
   BImpactEffectGridHashMap::const_iterator it = mImpactGrid.begin();
   int voxelCount = 0;
   

   uint totalTypes = 0;
   uint totalObjects = 0;
   while(it != mImpactGrid.end())
   {      
      BActiveImpactValue* pValue = ((BActiveImpactValue*)&it->second);
      //BActiveImpactKey*   pKey   = ((BActiveImpactKey*)&it->first);
      
      totalTypes += pValue->mValues.getNumber();
      for (int i = 0 ; i < pValue->mValues.getNumber(); i++)
         totalObjects+= pValue->mValues[i].mObjects.getNumber();

      it++;
   }

   textDisplay.printf(cColorCyan, "Hashmap Size: %d   Total Types : %d    Total Objects : %d", mImpactGrid.getSize(), totalTypes, totalObjects);   

   it = mImpactGrid.begin();
   while(it != mImpactGrid.end())
   {      
      BActiveImpactValue* pValue = ((BActiveImpactValue*)&it->second);
      BActiveImpactKey*   pKey   = ((BActiveImpactKey*)&it->first);
      
      textDisplay.printf(cColorCyan, "[%d,%d,%d]", (int) pKey->x, (int) pKey->y, (int) pKey->z);
      for (int i = 0 ; i < pValue->mValues.getNumber(); i++)
      {
         const BProtoImpactEffect* pData = gDatabase.getProtoImpactEffectFromIndex(pValue->mValues[i].mProtoID);
         textDisplay.printf(cColorCyan, "   ProtoID [%d] - %d / %d", pValue->mValues[i].mProtoID, pValue->mValues[i].mObjects.getNumber(), pData->mLimit);
      }
      it++;
      voxelCount++;
   }
}
#endif