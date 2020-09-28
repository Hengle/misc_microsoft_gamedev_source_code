//==============================================================================
// textvisualmanager.cpp
//
// Copyright (c) 2005, Ensemble Studios
//==============================================================================
#include "common.h"
#include "textvisualmanager.h"
#include "textvisual.h"
//#include "textvisualeffect.h"
//#include "renderdevice.h"
#include "textvisualdef.h"
#include "usermanager.h"
#include "user.h"


BTextVisualManager gTextVisualManager;

//==============================================================================
// BTextVisualManager::BTextVisualManager
//==============================================================================
BTextVisualManager::BTextVisualManager()
{
}


//==============================================================================
// BTextVisualManager::~BTextVisualManager
//==============================================================================
BTextVisualManager::~BTextVisualManager()
{
   cleanup();
}


//==============================================================================
// BTextVisualManager::cleanup
//==============================================================================
void BTextVisualManager::cleanup()
{
   reset();

   // Defs   
   for(long i=0; i<mTextVisualDefs.getNumber(); i++)
   {
      delete mTextVisualDefs[i];
      mTextVisualDefs[i] = NULL;
   }
   mTextVisualDefs.setNumber(0);
   mDefNameTable.clearAll();
}

//==============================================================================
// BTextVisualManager::reset
//==============================================================================
void BTextVisualManager::reset()
{
   // Visuals.
   for(long i=0; i<mTextVisuals.getNumber(); i++)
   {
      delete mTextVisuals[i];
      mTextVisuals[i] = NULL;
   }
   mTextVisuals.setNumber(0);

   mUnusedIndices.setNumber(0);
}


//==============================================================================
// BTextVisualManager::createVisual
//==============================================================================
long BTextVisualManager::createVisual(void)
{
   // Create.
   // jce [11/10/2005] -- TODO: pooling.
   BTextVisual *vis = new BTextVisual;
   if(!vis)
   {
      BFAIL("alloc error");
      return(-1);
   }
   
   // Find an index.
   long listIndex = mUnusedIndices.getNumber() - 1;
   long newIndex = -1;
   if(listIndex >= 0)
   {
      // Pull index from unused list.
      newIndex = mUnusedIndices[listIndex];
      mUnusedIndices.setNumber(listIndex);
      
      // Sanity.
      BASSERTM(mTextVisuals[newIndex] == NULL, "Used item flagged as unused.");
   }
   else
   {
      // No unused slots, so just add.
      newIndex = mTextVisuals.getNumber();
      mTextVisuals.setNumber(newIndex+1);
   }
   
   // Sanity, this should never happen.
   if(newIndex<0)
   {
      BFAIL("bad newindex");
      return(-1);
   }
   
   // Poke it in.
   mTextVisuals[newIndex] = vis;

   // Send back index.
   return(newIndex);
}


//==============================================================================
// BTextVisualManager::getVisual
//==============================================================================
BTextVisual *BTextVisualManager::getVisual(long id)
{
   // jce [11/10/2005] -- TODO: unit style ID recycling system.
    
   // Range check.
   if(id<0 || id>=mTextVisuals.getNumber())
      return(NULL);
   
   // Return it.
   return(mTextVisuals[id]);
}


//==============================================================================
// BTextVisualManager::update
//==============================================================================
void BTextVisualManager::update(DWORD elapsedTime)
{
   // Cap length.
   if(elapsedTime>200)
      elapsedTime = 200;
      
   // Walk list of visuals and update them.
   for(long i=0; i<mTextVisuals.getNumber(); i++)
   {
      if(mTextVisuals[i])
      {
         bool letItLive = mTextVisuals[i]->update(elapsedTime);
         if(!letItLive)
         {
            // jce [11/10/2005] -- TODO: pooling 
            delete mTextVisuals[i];
            mTextVisuals[i] = NULL;
            mUnusedIndices.add(i);
         }
      }
   }
}


//==============================================================================
// BTextVisualManager::render
//==============================================================================
void BTextVisualManager::render(int viewportIndex, bool bSplitScreen)
{
   // Walk list of visuals and render them.
   for(long i=0; i<mTextVisuals.getNumber(); i++)
   {      
      if(!mTextVisuals[i])
         continue;
     
      mTextVisuals[i]->render();
   }
}


//==============================================================================
// BTextVisualManager::getOrCreateVisualDef
//==============================================================================
long BTextVisualManager::getOrCreateVisualDef(const BCHAR_T *filename)
{
   // Sanity check.
   if(!filename)
   {
      BFAIL("Null filename passed to getOrCreate.");
      return(-1);
   }

   // Convert path to a standard form (lower case, using only backslashes, etc).
   BString standardizedPath;
   standardizedPath.standardizePathFrom(filename);

   // Look for existing texture.
   long index=-1;
   bool found=mDefNameTable.find(standardizedPath, &index);

   // If found, return it.
   if(found)
      return(index);

   // Create.
   index=createDef(standardizedPath);
   if(index<0)
      return(-1);

   return(index);
}


//==============================================================================
// BTextVisualManager::createDef
//==============================================================================
long BTextVisualManager::createDef(const BCHAR_T *filename)
{
   // Sanity check.
   if(!filename)
   {
      BFAIL("Null filename passed to create.");
      return(-1);
   }

   // Standardize path (lower case, slashes, etc.)
   BString standardizedPath;
   standardizedPath.standardizePathFrom(filename);

   // Allocate new texture.
   BTextVisualDef *def =new BTextVisualDef;
   if(!def)
   {
      BFAIL("Could not allocate new def.");
      return(-1);
   }

   // Set data.
   def->setFilename(standardizedPath);
   
   // Load it.
   // jce [11/15/2005] -- TODO: need demand-load? 
   def->load();

   // Add to list.
   long index=mTextVisualDefs.add(def);
   if(index<0)
   {
      BFAIL("Could not allocate space in array for new def.");
      delete def;
      return(-1);
   }

   // Put this into the hash table.
   mDefNameTable.add(def->getFilename().getPtr(), index);

   // Hand back index.
   return(index);
}


//==============================================================================
// BTextVisualManager::getVisualDef
//==============================================================================
BTextVisualDef *BTextVisualManager::getVisualDef(long id)
{
   // Range check.
   if(id < 0 || id >= mTextVisualDefs.getNumber())
      return(NULL);
      
   // Hand it back.
   return(mTextVisualDefs[id]);
}

//==============================================================================
// BTextVisualManager::create
//==============================================================================
void BTextVisualManager::create(long defID, long playerID, const BUString &text, const BVector &anchorPos, BDynamicSimLongArray *resultIDs)
{
   // Get the def requested.
   BTextVisualDef *def = getVisualDef(defID);
   if(!def)
      return;

   // Let it create the visual(s)
   def->create(playerID, text, anchorPos, resultIDs);
}

//==============================================================================
// BTextVisualManager::create
//==============================================================================
void BTextVisualManager::create(const BCHAR_T *defName, long playerID, const BUString &text, const BVector &anchorPos, BDynamicSimLongArray *resultIDs)
{
   // Get the def requested.
   long id = getOrCreateVisualDef(defName);
   BTextVisualDef *def = getVisualDef(id);
   if(!def)
      return;
   
   // Let it create the visual(s)
   def->create(playerID, text, anchorPos, resultIDs);
}


//==============================================================================
// BTextVisualManager::renderReset
//==============================================================================
void BTextVisualManager::renderReset(void)
{
   // For now, just nuke everything.  Someday could be more clever.
   cleanup();
}


//==============================================================================
// BTextVisualManager::reload
//==============================================================================
void BTextVisualManager::reload(BCHAR_T *filename)
{
   // For now, just nuke everything.  Someday could be more clever.
   filename;
   cleanup();
}


//==============================================================================
// BTextVisualManager::destroyVisual
//==============================================================================
void BTextVisualManager::destroyVisual(long id)
{
   BTextVisual *vis = getVisual(id);
   if(!vis)
      return;

   // jce [11/18/2005] -- TODO: pooling 
   delete mTextVisuals[id];
   mTextVisuals[id] = NULL;
   mUnusedIndices.add(id);
}


//==============================================================================
// eof: textvisualmanager.cpp
//==============================================================================