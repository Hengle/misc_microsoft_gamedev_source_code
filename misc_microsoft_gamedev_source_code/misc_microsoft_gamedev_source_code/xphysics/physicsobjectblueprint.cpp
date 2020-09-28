//==============================================================================
// physicsobjectblueprint.cpp
//
// Copyright (c) 2004, Ensemble Studios
//==============================================================================
#include "common.h"
#include "physicsobjectblueprint.h"
#include "xmlreader.h"
#include "string\converttoken.h"
#include "physics.h"
#include "shape.h"


//==============================================================================
// BPhysicsObjectBlueprintOverrides::BPhysicsObjectBlueprintOverrides
//==============================================================================
BPhysicsObjectBlueprintOverrides::BPhysicsObjectBlueprintOverrides(void)
{
   reset();
}

//==============================================================================
// BPhysicsObjectBlueprintOverrides::BPhysicsObjectBlueprintOverrides
//==============================================================================
BPhysicsObjectBlueprintOverrides::~BPhysicsObjectBlueprintOverrides(void)
{
   reset();
}


//==============================================================================
// BPhysicsObjectBlueprintOverrides::BPhysicsObjectBlueprintOverrides 
//==============================================================================
BPhysicsObjectBlueprintOverrides::BPhysicsObjectBlueprintOverrides(const BPhysicsObjectBlueprintOverrides &ref)
{
   //-- call the assignment operator
  *this = ref;
}

//==============================================================================
// BPhysicsObjectBlueprintOverrides::operator=
//==============================================================================
BPhysicsObjectBlueprintOverrides& BPhysicsObjectBlueprintOverrides::operator=(const BPhysicsObjectBlueprintOverrides &ref)
{
   mShapeID = ref.getShapeID();
   mCollisionFilterInfo = ref.getCollisionFilterInfo();
   mHalfExtents = ref.getHalfExtents();
   mFlags = ref.mFlags;
   return *this;
}



//==============================================================================
// BPhysicsObjectBlueprintOverrides::reset
//==============================================================================
void BPhysicsObjectBlueprintOverrides::reset( void )
{
   mShapeID = -1;
   mCollisionFilterInfo =-1;
   mHalfExtents.zero();
   mFlags.setNumber(cNumberFlags);
   mFlags.clear();
}



//==============================================================================
// BPhysicsObjectBlueprint::BPhysicsObjectBlueprint
//==============================================================================
BPhysicsObjectBlueprint::BPhysicsObjectBlueprint(void)
{
   reset();
}


//==============================================================================
// BPhysicsObjectBlueprint::reset
//==============================================================================
void BPhysicsObjectBlueprint::reset(void)
{
   mMass = 1.0f;
   mFriction = 0.0f;
   mRestitution = 1.0f;
   mCenterOfMassOffset = cOriginVector;
   mAngularDamping = 0.1f;
   mLinearDamping = 0.1f;
   mShape = -1;
   mLoaded = false;
   mFailedToLoad = false;
   mHalfExtents = cOriginVector;
}


//==============================================================================
// BPhysicsObjectBlueprint::~BPhysicsObjectBlueprint
//==============================================================================
BPhysicsObjectBlueprint::~BPhysicsObjectBlueprint(void)
{
}


//==============================================================================
// BPhysicsObjectBlueprint::load
//==============================================================================
bool BPhysicsObjectBlueprint::load(long dirID)
{
   // Bail if we're already loaded (or if we tried and failed).
   if(mLoaded || mFailedToLoad)
      return(true);
      
   // Assume the worst.
   mLoaded = false;
   mFailedToLoad = true;
   
   // Clear things out.
   reset();

   // Assemble name with extension.
   BSimString fullname = mFilename;
   fullname += B(".blueprint");
   
   // Read in the xml.
   BXMLReader reader;
   bool ok = reader.load(dirID, fullname);
   if(!ok || !reader.getRootNode())
   {
      BSimString qualPath;
      gFileManager.constructQualifiedPath(dirID, fullname, qualPath);
      blogtrace("Failed to load %s", qualPath.getPtr());
      return(false);
   }
   
   BXMLNode root(reader.getRootNode());
   for(long i=0; i<root.getNumberChildren(); i++)
   {
      // Get child.
      BXMLNode child(root.getChild(i));
         
      BSimString tempStr;         
      if(child.getName().compare(B("mass")) == 0)
         child.getTextAsFloat(mMass);
      else if(child.getName().compare(B("friction")) == 0)
         child.getTextAsFloat(mFriction);
      else if(child.getName().compare(B("restitution")) == 0)
         child.getTextAsFloat(mRestitution);
      else if(child.getName().compare(B("centerOfMassOffset")) == 0)
         convertTokenToVector(child.getTextPtr(tempStr), mCenterOfMassOffset);
      else if(child.getName().compare(B("angularDamping")) == 0)
         child.getTextAsFloat(mAngularDamping);
      else if(child.getName().compare(B("linearDamping")) == 0)
         child.getTextAsFloat(mLinearDamping);
      else if(child.getName().compare(B("shape")) == 0)
         mShape = gPhysics->getShapeManager().getOrCreate(child.getTextPtr(tempStr));
      else if(child.getName().compare(B("halfExtents")) ==0)
         convertTokenToVector(child.getTextPtr(tempStr), mHalfExtents);
   }
   
   // Success.
   mLoaded = true;
   mFailedToLoad = false;
   return(true);
}

//==============================================================================
// BPhysicsObjectBlueprint::unload
//==============================================================================
void BPhysicsObjectBlueprint::unload(void)
{
   mLoaded = false;
}

//==============================================================================
//==============================================================================
void BPhysicsObjectBlueprint::loadShape()
{
   if (mShape != -1)
      gPhysics->getShapeManager().get(mShape, true);
}

//==============================================================================
//==============================================================================
void BPhysicsObjectBlueprint::unloadShape()
{
   if (mShape != -1)
   {
      BShape* pShape = gPhysics->getShapeManager().get(mShape, false);
      if (pShape)
         pShape->unload();
   }
}


//==============================================================================
// eof: physicsobjectblueprint.cpp
//==============================================================================
