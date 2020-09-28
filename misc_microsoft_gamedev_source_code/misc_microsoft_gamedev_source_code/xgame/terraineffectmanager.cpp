//==============================================================================
// terraineffectmanager.cpp
//
// Copyright (c) 2001-2007, Ensemble Studios
//==============================================================================


// Includes
#include "common.h"
#include "terraineffect.h"
#include "terraineffectmanager.h"
#include "world.h"
#include "archiveManager.h"

//============================================================================
// Globals
//============================================================================
BTerrainEffectManager gTerrainEffectManager;


//============================================================================
// BTerrainEffectManager::BTerrainEffectManager
//============================================================================
BTerrainEffectManager::BTerrainEffectManager()
{
   reset();
}

//============================================================================
// BTerrainEffectManager::~BTerrainEffectManager
//============================================================================
BTerrainEffectManager::~BTerrainEffectManager()
{
   reset();
}

//============================================================================
// BTerrainEffectManager::reset
//============================================================================
void BTerrainEffectManager::reset( void )
{
   long count = mTerrainEffects.getNumber();
   for (long i = 0; i < count; i++)
   {
      delete mTerrainEffects.get(i);
   }

   mTerrainEffects.clear();
   mTerrainEffectNameTable.clearAll();
   mTerrainEffectGrid.clear();
}

//============================================================================
// BTerrainEffectManager::clear
//============================================================================
void BTerrainEffectManager::clear() 
{ 
   mTerrainEffectGrid.clear(); 
}

//============================================================================
// BTerrainEffectManager::unloadAll
// this call cleans up the references to assets that also get unloaded between levels.
// It keeps the terrain effects around so since they are only ever loaded once in archive mode
// and can't be reloaded.
//============================================================================
void BTerrainEffectManager::unloadAll()
{
   long count = mTerrainEffects.getNumber();
   for (long i = 0; i < count; i++)
   {
      mTerrainEffects.get(i)->unloadAllAssets();
   }
}


//============================================================================
// BTerrainEffectManager::findTerrainEffect
//============================================================================
long BTerrainEffectManager::findTerrainEffect(const BCHAR_T* pFileName)
{
   // Clean path.
   BSimString standardizedFilename;
   standardizedFilename.standardizePathFrom(pFileName);

   // Look up in name table.
   short index=-1;
   bool found=mTerrainEffectNameTable.find(standardizedFilename, &index);

   // If we found a match, give back the index.
   if(found)
      return(index);

   // No match, give back -1.
   return(-1);
}

//============================================================================
// BTerrainEffectManager::createTerrainEffect
//============================================================================
long BTerrainEffectManager::createTerrainEffect(const BCHAR_T* pFileName)
{
   // ATTENTION: When running with archives don't allow any creation after BDatabase::setup
   //
   if (gArchiveManager.getArchivesEnabled() && gArchiveManager.getIsGameInitialized() && !gSaveGame.isLoading())
   {
      BSimString messageString;
      messageString.format("BTerrainEffectManager::createTerrainEffect - Loading a terrain effect after BDatabase::setup is not supported when running with archives.  TFX name: %s", pFileName);
      BASSERTM(false, messageString.getPtr());

      return -1;
   }

   // Clean path.
   BSimString standardizedFilename;
   standardizedFilename.standardizePathFrom(pFileName);


   BTerrainEffect* pTerrainEffect = new BTerrainEffect;
   if(!pTerrainEffect)
      return(-1);

   long index = mTerrainEffects.getNumber();
   
   pTerrainEffect->init(standardizedFilename, index);

   mTerrainEffects.setNumber(index + 1);
   mTerrainEffects[index] = pTerrainEffect;
   if(index<0)
      return -1;

   // Add to hash table.
   mTerrainEffectNameTable.add(standardizedFilename, static_cast<short>(index));

   // Hand back index.
   return(index);
}

//============================================================================
// BTerrainEffectManager::getOrCreateTerrainEffect
//============================================================================
long BTerrainEffectManager::getOrCreateTerrainEffect(const BCHAR_T* pFileName, bool loadFile)
{
   //SCOPEDSAMPLE(BTerrainEffectManager_getOrCreateTerrainEffect)
   // Check param.
   if(!pFileName)
   {
      BFAIL("NULL pFileName.");
      return(-1);
   }

   // Clean path.
   BSimString name;
   name.standardizePathFrom(pFileName);


   // Look for existing.
   long index=findTerrainEffect(name);


   // The loadFile parameter means different things in different contexts.  On loose files builds (non archive builds)
   // it's whether the TFX file and all its dependent assets should be loaded.  On archive builds it means whether the
   // dependent assets should be loaded or not.

   if (gArchiveManager.getArchivesEnabled())
   {
      if(index<0)
      {
         index=createTerrainEffect(name);

         BTerrainEffect* pTerrainEffect = getTerrainEffect(index, false);
         BASSERT(pTerrainEffect);

         pTerrainEffect->load();
      }

      BASSERT(index != -1);

      if(loadFile)
      {
         BTerrainEffect* pTerrainEffect = getTerrainEffect(index, false);
         BASSERT(pTerrainEffect);

         if (pTerrainEffect->isLoaded() && !pTerrainEffect->areAllAssetsLoaded())
         {
            pTerrainEffect->loadAllAssets();
         }
      }
   }
   else
   {
      if(index<0)
         index=createTerrainEffect(name);

      BASSERT(index != -1);

      if(loadFile)
      {
         BTerrainEffect* pTerrainEffect = getTerrainEffect(index, false);
         BASSERT(pTerrainEffect);

         if (!pTerrainEffect->isLoaded() && !pTerrainEffect->loadFailed())
         {
            pTerrainEffect->load();
            pTerrainEffect->loadAllAssets();
         }
      }
   }

   /*
   // None existing, must create.
   if(index<0)
   {
      index=createTerrainEffect(pFileName);
   }
   */

   // Give back result.
   return(index);
}

//============================================================================
// BTerrainEffectManager::getTerrainEffect
//============================================================================
BTerrainEffect* BTerrainEffectManager::getTerrainEffect(long index, bool ensureLoaded)
{
   // Check for bogus index.
   if(index<0 || index>=mTerrainEffects.getNumber())
      return(NULL);

   BTerrainEffect* pTerrainEffect = mTerrainEffects[index];

   if (gArchiveManager.getArchivesEnabled())
   {
      if(ensureLoaded && pTerrainEffect && pTerrainEffect->isLoaded() && !pTerrainEffect->areAllAssetsLoaded())
      {
         pTerrainEffect->loadAllAssets();
      }
   }
   else
   {
      if (ensureLoaded && pTerrainEffect)
      {
         if (!pTerrainEffect->isLoaded())
         {
            if(pTerrainEffect->loadFailed())
               return NULL;

            if(!pTerrainEffect->load())
               return NULL;
         }

         if (!pTerrainEffect->areAllAssetsLoaded())
         {
            pTerrainEffect->loadAllAssets();
         }
      }
   }

   return pTerrainEffect;
}

//============================================================================
//============================================================================
bool BTerrainEffectManager::checkMeter(int terrainEffectID, BVector pos)
{
   // Debug stats
   #ifndef BUILD_FINAL
      mStats.mNumImpactsSimTriedToCreate++;
   #endif

   // Get terrain effect
//-- FIXING PREFIX BUG ID 1569
   const BTerrainEffect* pTerrainEffect = getTerrainEffect(terrainEffectID, true);
//--
   if (!pTerrainEffect)
   {
      BASSERT(0);
      return false;
   }

   // Bail if no metering for this effect.
   DWORD meterLength = pTerrainEffect->getMeterLength();
   uint meterCount = pTerrainEffect->getMeterCount();
   if (meterLength == 0 || meterCount == 0)
   {
      // Debug stats
      #ifndef BUILD_FINAL
         mStats.mAddedImpacts++;
      #endif
      return true;
   }

   // Get active instance in the hash grid
   BActiveTerrainEffectKey key;
   initKey(pos, key);

   BTerrainEffectEffectGridHashMap::const_iterator it = mTerrainEffectGrid.find(key);
   BActiveTerrainEffectValue* pValue = NULL;
   BActiveTerrainEffectInstance* pActiveInstance = NULL;
   if (it != mTerrainEffectGrid.end())
   {
      pValue = (BActiveTerrainEffectValue*) &it->second;
      for (int i = 0; i < pValue->mValues.getNumber(); i++)
      {
         if (pValue->mValues[i].mTerrainEffectID == terrainEffectID)
         {
            pActiveInstance = &pValue->mValues[i];
            break;
         }
      }
   }

   // Get current time.     
   DWORD currTime = gWorld->getGametime();

   // Purge anything other than the time window.
   if (pActiveInstance)
   {
      // Find the time which is too old to care about.
      DWORD tooOldTime = currTime - meterLength;

      // Find the index of the first item we care about
      uint numTimes = pActiveInstance->mTimes.getNumber();
      uint i;
      for (i = 0; i < numTimes; i++)
      {
         DWORD logTime = pActiveInstance->mTimes[i];

         // If we've gotten to the stuff we still care about, bail out.
         if (logTime > tooOldTime)
            break;
      }

      // Remove old items
      if (i > 0)
      {
         pActiveInstance->mTimes.erase(0, i);
      }

      // If remaining count is too high, no effect allowed.
      uint numInstances = pActiveInstance->mTimes.getNumber();
      if (numInstances >= meterCount)
      {
         // Debug stats
         #ifndef BUILD_FINAL
            mStats.mDroppedImpacts++;
         #endif
         return false;
      }

      // Add a record of this new terrain effect instance
      pActiveInstance->mTimes.add(currTime);
   }
   else
   {
      // this is a brand new key add it.
      if (it == mTerrainEffectGrid.end())
      {
         BActiveTerrainEffectInstance newInstance;
         newInstance.mTerrainEffectID = terrainEffectID;
         newInstance.mTimes.add(currTime);

         BActiveTerrainEffectValue newValue;
         newValue.mValues.add(newInstance);
         mTerrainEffectGrid.insert(key, newValue);         
      }
      else
      {
         //-- its in the grid and we have one of the same type already then just add the object to that list.
         BASSERT(pValue);
         BActiveTerrainEffectInstance newInstance;
         newInstance.mTerrainEffectID = terrainEffectID;
         newInstance.mTimes.add(currTime);

         pValue->mValues.add(newInstance);
      }
   }

   // Debug stats
   #ifndef BUILD_FINAL
      mStats.mAddedImpacts++;
   #endif

   // Let it play.
   return(true);
}

//============================================================================
//============================================================================
void BTerrainEffectManager::initKey(const BVector& pos, BActiveTerrainEffectKey& key) const
{
   key.set( 
         Math::FloatToIntTrunc( pos.x / eVoxelXDim), 
         Math::FloatToIntTrunc( pos.y / eVoxelYDim), 
         Math::FloatToIntTrunc( pos.z / eVoxelZDim) );
}

#ifndef BUILD_FINAL
//============================================================================
//============================================================================
void BTerrainEffectManager::dumpVoxelData(BDebugTextDisplay& textDisplay)
{
   textDisplay.printf(cColorCyan, "Voxel Data");
   BTerrainEffectEffectGridHashMap::const_iterator it = mTerrainEffectGrid.begin();
   int voxelCount = 0;
   

   uint totalTypes = 0;
   uint totalObjects = 0;
   while(it != mTerrainEffectGrid.end())
   {      
      BActiveTerrainEffectValue* pValue = ((BActiveTerrainEffectValue*)&it->second);
      //BActiveTerrainEffectKey*   pKey   = ((BActiveTerrainEffectKey*)&it->first);
      
      totalTypes += pValue->mValues.getNumber();
      for (int i = 0 ; i < pValue->mValues.getNumber(); i++)
         totalObjects+= pValue->mValues[i].mTimes.getNumber();

      it++;
   }

   textDisplay.printf(cColorCyan, "Hashmap Size: %d   Total Types : %d    Total Objects : %d", mTerrainEffectGrid.getSize(), totalTypes, totalObjects);   

   it = mTerrainEffectGrid.begin();
   while(it != mTerrainEffectGrid.end())
   {      
      BActiveTerrainEffectValue* pValue = ((BActiveTerrainEffectValue*)&it->second);
      BActiveTerrainEffectKey*   pKey   = ((BActiveTerrainEffectKey*)&it->first);
      
      textDisplay.printf(cColorCyan, "[%d,%d,%d]", (int) pKey->x, (int) pKey->y, (int) pKey->z);
      for (int i = 0 ; i < pValue->mValues.getNumber(); i++)
      {
//-- FIXING PREFIX BUG ID 1570
         const BTerrainEffect* pTE = getTerrainEffect(pValue->mValues[i].mTerrainEffectID, true);
//--
         textDisplay.printf(cColorCyan, "   TerrainEffectID [%d] - %d / %d", pValue->mValues[i].mTerrainEffectID, pValue->mValues[i].mTimes.getNumber(), pTE->getMeterCount());
      }
      it++;
      voxelCount++;
   }
}
#endif