//============================================================================
//
//  shapemanager.cpp
//  
//  Copyright (c) 2003, Ensemble Studios
//
//============================================================================

#include "common.h"
#include "shapemanager.h"
#include "shape.h"
#include "physics.h"
#include "workdirsetup.h"
#include "physicsobject.h"


//============================================================================
// BShapeManager::BShapeManager
//============================================================================
BShapeManager::BShapeManager() :
   mBaseDirectoryID(cDirProduction)
{
}


//============================================================================
// BShapeManager::~BShapeManager
//============================================================================
BShapeManager::~BShapeManager()
{
   cleanup();
}


//============================================================================
// BShapeManager::init
//============================================================================
bool BShapeManager::init(void)
{
   // Nothing yet.
   
   // Success.
   return(true);
}


//============================================================================
// BShapeManager::cleanup
//============================================================================
void BShapeManager::cleanup(void)
{
   // Kill off shapes.
   for(long i=0; i<mShapes.getNumber(); i++)
   {
      delete mShapes[i];
      mShapes[i]=NULL;
   }
   mShapes.setNumber(0);

   // Clear hash table.
   mNameTable.clearAll();
}


//============================================================================
// BShapeManager::getOrCreate
//============================================================================
long BShapeManager::getOrCreate(const BCHAR_T *filename, bool forceLoad)
{
   // Sanity check.
   if(!filename)
   {
      BFAIL("Null filename passed to getOrCreate.");
      return(-1);
   }

   // Look for existing shape.
   long index=find(filename);

   // If found, return it.
   if(index>=0)
      return(index);

   // Create.
   index=create(filename);
   if(index<0)
      return(-1);

   // If we want to force load this shape, go ahead and do that now.
   if(forceLoad)
      mShapes[index]->load(mBaseDirectoryID);

   return(index);
}


//============================================================================
// BShapeManager::find
//============================================================================
long BShapeManager::find(const BCHAR_T *filename)
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
// BShapeManager::create
//============================================================================
long BShapeManager::create(const BCHAR_T *filename)
{
   // Sanity check.
   if(!filename)
   {
      BFAIL("Null filename passed to create.");
      return(-1);
   }

   // Allocate new shape.
   BShape *shape = new BShape;
   if(!shape)
   {
      BFAIL("Could not allocate new shape.");
      return(-1);
   }

   // Set data.
   shape->setFilename(filename);

   // Add to list.
   long index=mShapes.add(shape);
   if(index<0)
   {
      BFAIL("Could not allocate space in array for new shape.");
      delete shape;
      return(-1);
   }

   // Put this into the hash table.
   long foundVal=0;
   mNameTable.add(filename, index, foundVal);

   // Hand back index.
   return(index);
}

//============================================================================
// BShapeManager::shapeIDFunc
//============================================================================
long BShapeManager::shapeIDFunc(const BCHAR_T *filename)
{
   return gPhysics->getShapeManager().getOrCreate(filename);
}

//============================================================================
// BShapeManager::get
//============================================================================
BShape *BShapeManager::get(long index, bool load)
{
   if(index<0 || index>=mShapes.getNumber())
      return(NULL);

   // Make sure shape is loaded if requested.
   if(load)
      mShapes[index]->load(mBaseDirectoryID);

   // Hand back pointer.
   return(mShapes[index]);
}

//============================================================================
// BShapeManager::loadAll
//============================================================================
void BShapeManager::loadAll(void)
{
   for(long i=0; i<mShapes.getNumber(); i++)
      mShapes[i]->load(mBaseDirectoryID);
}

//============================================================================
// BShapeManager::unloadAll
//============================================================================
void BShapeManager::unloadAll(void)
{
   for(long i=0; i<mShapes.getNumber(); i++)
      mShapes[i]->unload();
}

//============================================================================
// BShapeManager::addDynamicShape
//============================================================================
 long BShapeManager::addDynamicShape(BShape *pShape, const BCHAR_T *name)
 {
    if (!pShape)
       return (-1);

    long id = mShapes.add(pShape);
    if (id == -1)
       return (-1);

    // Put this into the hash table.
    long foundVal=0;
    mNameTable.add(name, id, foundVal);

    pShape->setFilename(name);
    return id;
 }

  //============================================================================
 // BShapeManager::resetShape
 //============================================================================
 bool BShapeManager::resetShape(long id )
 {
    //-- valid id?
    if (id < 0 || id >= mShapes.getNumber())
       return (false);


    BShape *pShape = get(id, false);

    if (pShape)
    {
       //-- now walk the known physics objects and update them with the new shape
       //-- if they are affected by this reload
       BPointerList<BPhysicsObject> &list = BPhysicsObject::getPhysicsObjectList();
       BHandle hItem = NULL;
       BPhysicsObject *pItem = list.getHead(hItem);
       while (pItem)
       {
          if (pItem->getShapeID() == id)
          {
             //pItem->removeFromWorld();
             pItem->setShape(*pShape);
             //pItem->addToWorld();
          }

          //-- grab the next one
          pItem = list.getNext(hItem);
       }
    }

    return (true);
 }



//============================================================================
// eof: shapemanager.cpp
//============================================================================
