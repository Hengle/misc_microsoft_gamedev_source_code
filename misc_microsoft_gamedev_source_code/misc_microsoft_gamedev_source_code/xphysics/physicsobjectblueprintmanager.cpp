//============================================================================
//
//  physicsobjectblueprintmanager.cpp
//  
//  Copyright (c) 2003, Ensemble Studios
//
//============================================================================

#include "common.h"
#include "physicsobjectblueprintmanager.h"
#include "physicsobjectblueprint.h"
#include "workdirsetup.h"


//============================================================================
// BPhysicsObjectBlueprintManager::BPhysicsObjectBlueprintManager
//============================================================================
BPhysicsObjectBlueprintManager::BPhysicsObjectBlueprintManager() :
   mBaseDirectoryID(cDirProduction)
{
}


//============================================================================
// BPhysicsObjectBlueprintManager::~BPhysicsObjectBlueprintManager
//============================================================================
BPhysicsObjectBlueprintManager::~BPhysicsObjectBlueprintManager()
{
   cleanup();
}


//============================================================================
// BPhysicsObjectBlueprintManager::init
//============================================================================
bool BPhysicsObjectBlueprintManager::init(void)
{
   // Nothing yet.
   
   // Success.
   return(true);
}


//============================================================================
// BPhysicsObjectBlueprintManager::cleanup
//============================================================================
void BPhysicsObjectBlueprintManager::cleanup(void)
{
   // Kill off blueprints.
   for(long i=0; i<mBlueprints.getNumber(); i++)
   {
      delete mBlueprints[i];
      mBlueprints[i]=NULL;
   }
   mBlueprints.setNumber(0);

   // Clear hash table.
   mNameTable.clearAll();
}


//============================================================================
// BPhysicsObjectBlueprintManager::getOrCreate
//============================================================================
long BPhysicsObjectBlueprintManager::getOrCreate(const BCHAR_T *filename, bool forceLoad)
{
   // Sanity check.
   if(!filename)
   {
      BFAIL("Null filename passed to getOrCreate.");
      return(-1);
   }

   // Look for existing blueprint.
   long index=find(filename);

   // If found, return it.
   if(index>=0)
      return(index);

   // Create.
   index=create(filename);
   if(index<0)
      return(-1);

   // If we want to force load this blueprint, go ahead and do that now.
   if(forceLoad)
      mBlueprints[index]->load(mBaseDirectoryID);

   return(index);
}


//============================================================================
// BPhysicsObjectBlueprintManager::find
//============================================================================
long BPhysicsObjectBlueprintManager::find(const BCHAR_T *filename)
{
   // Look up in name table.
   long index=-1;
   bool found=mNameTable.find(filename, &index);

   // If we found a match, give back the index.
   if(found)
      return(index);

   // No match, give back -1.
   return(-1);
}


//============================================================================
// BPhysicsObjectBlueprintManager::create
//============================================================================
long BPhysicsObjectBlueprintManager::create(const BCHAR_T *filename)
{
   // Sanity check.
   if(!filename)
   {
      BFAIL("Null filename passed to create.");
      return(-1);
   }

   // Allocate new blueprint.
   BPhysicsObjectBlueprint *blueprint = new BPhysicsObjectBlueprint;
   if(!blueprint)
   {
      BFAIL("Could not allocate new blueprint.");
      return(-1);
   }

   // Set data.
   blueprint->setFilename(filename);

   // Add to list.
   long index=mBlueprints.add(blueprint);
   if(index<0)
   {
      BFAIL("Could not allocate space in array for new blueprint.");
      delete blueprint;
      return(-1);
   }

   // Put this into the hash table.
   long foundVal=0;
   mNameTable.add(filename, index, foundVal);

   // Hand back index.
   return(index);
}


//============================================================================
// BPhysicsObjectBlueprintManager::get
//============================================================================
BPhysicsObjectBlueprint *BPhysicsObjectBlueprintManager::get(long index, bool load)
{
   if(index<0 || index>=mBlueprints.getNumber())
      return(NULL);

   // Make sure blueprint is loaded if requested.
   if(load)
      mBlueprints[index]->load(mBaseDirectoryID);

   // Hand back pointer.
   return(mBlueprints[index]);
}


//============================================================================
// BPhysicsObjectBlueprintManager::unloadAll
//============================================================================
void BPhysicsObjectBlueprintManager::unloadAll(void)
{
   for(long i=0; i<mBlueprints.getNumber(); i++)
      mBlueprints[i]->unload();
}


//============================================================================
// eof: physicsobjectblueprintmanager.cpp
//============================================================================
