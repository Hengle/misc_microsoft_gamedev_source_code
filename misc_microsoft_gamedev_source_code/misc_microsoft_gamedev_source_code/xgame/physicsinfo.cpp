//==============================================================================
// physicsinfo.cpp
//
// Copyright (c) 2004, Ensemble Studios
//==============================================================================

// Includes
#include "common.h"



#include "physicsinfo.h"
#include "xmlreader.h"
#include "physics.h"
#include "physicsobjectblueprintmanager.h"
#include "physicsobjectblueprint.h"
#include "string\converttoken.h"
#include "visualmanager.h"
#include "soundmanager.h"
#include "database.h"
#include "terraineffectmanager.h"

//==============================================================================
//==============================================================================
BPhysicsVehicleInfo::BPhysicsVehicleInfo() :
   mSpringLength(1.0f),
   mWheelRadius(1.0f),
   mSpringStrength(1.0f),
   mDampingCompression(1.0f),
   mDampingRelaxation(1.0f),
   mNormalClippingAngle(0.1f),
   mChassisUnitInertiaYaw(1.0f),
   mChassisUnitInertiaRoll(1.0f),
   mChassisUnitInertiaPitch(1.0f),
   mTreadHardPointHeight(1.0f),
   mTreadRayLength(1.0f),
   mTreadVelocityScalar(-0.001f),
   mTreadWheelAttachments(false)
{

}

//==============================================================================
//==============================================================================
BPhysicsVehicleInfo::~BPhysicsVehicleInfo()
{
}

//==============================================================================
//==============================================================================
bool BPhysicsVehicleInfo::load(BXMLNode& node)
{
   bool retVal = true;

   for (int i = 0; i < node.getNumberChildren(); i++)
   {
//-- FIXING PREFIX BUG ID 1548
      const BXMLNode& child = node.getChild(i);
//--
      if (child.getName().compare(B("suspension")) == 0)
      {
         retVal |= child.getAttribValueAsFloat("springLength", mSpringLength);
         retVal |= child.getAttribValueAsFloat("wheelRadius", mWheelRadius);
         retVal |= child.getAttribValueAsFloat("springStrength", mSpringStrength);
         retVal |= child.getAttribValueAsFloat("dampingCompression", mDampingCompression);
         retVal |= child.getAttribValueAsFloat("dampingRelaxation", mDampingRelaxation);
         retVal |= child.getAttribValueAsFloat("normalClippingAngle", mNormalClippingAngle);
      }
      else if (child.getName().compare(B("chassisInertia")) == 0)
      {
         retVal |= child.getAttribValueAsFloat("yaw", mChassisUnitInertiaYaw);
         retVal |= child.getAttribValueAsFloat("pitch", mChassisUnitInertiaPitch);
         retVal |= child.getAttribValueAsFloat("roll", mChassisUnitInertiaRoll);
      }
      else if (child.getName().compare(B("hardpoints")) == 0)
      {
         for (int j = 0; j < child.getNumberChildren(); j++)
         {
//-- FIXING PREFIX BUG ID 1546
            const BXMLNode& hardpoint = child.getChild(j);
//--
            BVector vec;
            if (hardpoint.getTextAsVector(vec))
            {
               mHardpoints.add(vec);
            }
         }
      }
      else if (child.getName().compare(B("treadpoints")) == 0)
      {
         child.getAttribValueAsFloat("height", mTreadHardPointHeight);
         child.getAttribValueAsFloat("length", mTreadRayLength);
         child.getAttribValueAsFloat("velocityScalar", mTreadVelocityScalar);
         bool wheelAttachments = false;
         if (child.getAttribValueAsBool("wheelAttachments", wheelAttachments))
            mTreadWheelAttachments = wheelAttachments;

         for (int j = 0; j < child.getNumberChildren(); j++)
         {
//-- FIXING PREFIX BUG ID 1547
            const BXMLNode& treadpoint = child.getChild(j);
//--
            BVector vec;
            if (treadpoint.getTextAsVector(vec))
            {
               mTreadHardpoints.add(vec);
               float zOffset = 0.0f;
               treadpoint.getAttribValueAsFloat("zOffset", zOffset);
               mTreadHardpointZOffsets.add(zOffset);
            }
         }
      }
   }

   return retVal;
}

//==============================================================================
//==============================================================================
void BPhysicsInfo::loadBluePrints()
{
   uint count = mBlueprintIDs.size();
   for (uint i=0; i<count; i++)
   {
      BPhysicsObjectBlueprint* pBlueprint = gPhysics->getPhysicsObjectBlueprintManager().get(mBlueprintIDs[i], true);
      if (pBlueprint)
         pBlueprint->loadShape();
   }
}

//==============================================================================
// BPhysicsInfo::BPhysicsInfo
//==============================================================================
BPhysicsInfo::BPhysicsInfo(void) :
   mpVehicleInfo(NULL),
   mLoaded(false),
   mFailedToLoad(false),
   mVehicleType(cNone),
   mMotionType(cBoxInertia),
   mClamshell(false),
   mPhantom(false),
   mCenterOffset(0.0f, 0.0f, 0.0f),
   mUpperHeightOffset(0.0f),
   mLowerHeightOffset(0.0f),
   mImpactMinVelocity(0.0f),
   mID(-1)
{
   mBlueprintIDs.clear();
}

//==============================================================================
// BPhysicsInfo::~BPhysicsInfo
//==============================================================================
BPhysicsInfo::~BPhysicsInfo(void)
{
   if (mpVehicleInfo)
   {
      delete mpVehicleInfo;
      mpVehicleInfo = NULL;
   }
} 


//==============================================================================
// BPhysicsInfo::reset
//==============================================================================
void BPhysicsInfo::reset(void)
{
   if (mpVehicleInfo)
   {
      delete mpVehicleInfo;
      mpVehicleInfo = NULL;
   }

   mBlueprintIDs.clear();

   mThrownByProjectiles = false;
   mDestroyedByProjectiles = false;
   mPhysicsOnDeath = false;
   mSplashEffectHandle = NULL;
   mVehicleType = cNone;
   mMotionType = cBoxInertia;
   mClamshell = false;
   mPhantom = false;

   mCenterOffset.set(0.0f, 0.0f, 0.0f);
   mUpperHeightOffset = 0.0f;
   mLowerHeightOffset = 0.0f;
}


//==============================================================================
// BPhysicsInfo::load
//==============================================================================
bool BPhysicsInfo::load(long dirID)
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
   fullname += B(".physics");
   
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
      if(child.getName().compare(B("blueprint")) == 0)
      {
         long blueprintID = gPhysics->getPhysicsObjectBlueprintManager().getOrCreate(child.getTextPtr(tempStr));
         mBlueprintIDs.add(blueprintID);
      }
      else if(child.getName().compare(B("ThrownByProjectiles")) == 0)
      {
         bool thrownByProjectiles = false;
         ok = convertTokenToBool(child.getTextPtr(tempStr), thrownByProjectiles);
         if(!ok)
            child.logInfo("Expected true or false (0 or 1 also ok)");
         mThrownByProjectiles = thrownByProjectiles;
      }
      else if(child.getName().compare(B("DestroyedByProjectiles")) == 0)
      {
         bool destroyedByProjectiles = false;
         ok = convertTokenToBool(child.getTextPtr(tempStr), destroyedByProjectiles);
         if(!ok)
            child.logInfo("Expected true or false (0 or 1 also ok)");
         mDestroyedByProjectiles = destroyedByProjectiles;
      }
      else if(child.getName().compare(B("TerrainEffects")) == 0)
      {
         mTerrainEffectHandle = gTerrainEffectManager.getOrCreateTerrainEffect(child.getTextPtr(tempStr), false);
      }
      else if(child.getName().compare(B("PhysicsOnDeath")) == 0)
      {
         bool physicsOnDeath = false;
         ok = convertTokenToBool(child.getTextPtr(tempStr), physicsOnDeath);
         if(!ok)
            child.logInfo("Expected true or false (0 or 1 also ok)");
         mPhysicsOnDeath = physicsOnDeath;
      }
      else if(child.getName().compare(B("vehicle")) == 0)
      {
         mVehicleType = cNone;
         if (child.getText(tempStr).compare(B("warthog")) == 0)
            mVehicleType = cWarthog;
         else if (child.getText(tempStr).compare(B("ghost")) == 0)
            mVehicleType = cGhost;
         else if (child.getText(tempStr).compare(B("scorpion")) == 0)
            mVehicleType = cScorpion;
         else if (child.getText(tempStr).compare(B("cobra")) == 0)
            mVehicleType = cCobra;
         else if (child.getText(tempStr).compare(B("wolverine")) == 0)
            mVehicleType = cWolverine;
         else if (child.getText(tempStr).compare(B("grizzly")) == 0)
            mVehicleType = cGrizzly;
         else if (child.getText(tempStr).compare(B("elephant")) == 0)
            mVehicleType = cElephant;
         else if (child.getText(tempStr).compare(B("hawk")) == 0)
            mVehicleType = cHawk;
         else if (child.getText(tempStr).compare(B("hornet")) == 0)
            mVehicleType = cHornet;
         else if (child.getText(tempStr).compare(B("vulture")) == 0)
            mVehicleType = cVulture;
         else if (child.getText(tempStr).compare(B("banshee")) == 0)
            mVehicleType = cBanshee;
         else if (child.getText(tempStr).compare(B("vampire")) == 0)
            mVehicleType = cVampire;
         else if (child.getText(tempStr).compare(B("sentinel")) == 0)
            mVehicleType = cSentinel;
         else if (child.getText(tempStr).compare(B("ground")) == 0)
            mVehicleType = cGround;
         else if (child.getText(tempStr).compare(B("gremlin")) == 0)
            mVehicleType = cGremlin;
         else if (child.getText(tempStr).compare(B("chopper")) == 0)
            mVehicleType = cChopper;
         else if (child.getText(tempStr).compare(B("rhino")) == 0)
            mVehicleType = cRhino;
         else if (child.getText(tempStr).compare(B("reactor")) == 0)
            mVehicleType = cReactor;

         // Load vehicle info
         BXMLNode vehicleDataNode;
         if (child.getChild("info", &vehicleDataNode))
         {
            mpVehicleInfo = new BPhysicsVehicleInfo();
            bool loaded = mpVehicleInfo->load(vehicleDataNode);
            if (!loaded)
            {
               delete mpVehicleInfo;
               mpVehicleInfo = NULL;
            }
         }
      }
      else if(child.getName().compare(B("clamshell")) == 0)
      {
         // Must have 3 children (upper, lower, pelvis)
         if (child.getNumberChildren() != 3)
         {
            blogtrace("Failed to find 3 clamshell children in %s", fullname.getPtr());
            return false;
         }

         // Clear out blueprint IDs
         mBlueprintIDs.clear();

         BXMLNode clamshellChild;
         if (!child.getChild("upper", &clamshellChild))
         {
            blogtrace("Failed to find upper body in clamshell <%s>", fullname.getPtr());
            return false;
         }
         long blueprintID = gPhysics->getPhysicsObjectBlueprintManager().getOrCreate(clamshellChild.getTextPtr(tempStr));
         mBlueprintIDs.add(blueprintID);

         mUpperHeightOffset = 0.0f;
         clamshellChild.getAttribValueAsFloat("heightOffset", mUpperHeightOffset);

         if (!child.getChild("lower", &clamshellChild))
         {
            blogtrace("Failed to find lower body in clamshell <%s>", fullname.getPtr());
            return false;
         }
         blueprintID = gPhysics->getPhysicsObjectBlueprintManager().getOrCreate(clamshellChild.getTextPtr(tempStr));
         mBlueprintIDs.add(blueprintID);

         mLowerHeightOffset = 0.0f;
         clamshellChild.getAttribValueAsFloat("heightOffset", mLowerHeightOffset);

         if (!child.getChild("pelvis", &clamshellChild))
         {
            blogtrace("Failed to find pelvis body in clamshell <%s>", fullname.getPtr());
            return false;
         }
         blueprintID = gPhysics->getPhysicsObjectBlueprintManager().getOrCreate(clamshellChild.getTextPtr(tempStr));
         mBlueprintIDs.add(blueprintID);

         mClamshell = true;
      }
      else if(child.getName().compare(B("centerOffset")) == 0)
      {
         mCenterOffset.set(0.0f, 0.0f, 0.0f);
         ok = convertTokenToVector(child.getTextPtr(tempStr), mCenterOffset);
         if(!ok)
         {
            mCenterOffset.set(0.0f, 0.0f, 0.0f);
            BASSERT(0);
            child.logInfo("Invalid vector");
         }
      }
      else if(child.getName().compare(B("ImpactMinVelocity")) == 0)
      {
         child.getTextAsFloat(mImpactMinVelocity);
      }     
      else if (child.getName().compare(B("Phantom")) == 0)
      {
         mPhantom = true;
      }      
      else if(child.getName().compare(B("MotionType")) == 0)
      {         
         mMotionType = cBoxInertia;
         if (child.getText(tempStr).compare(B("normal")) == 0)
            mMotionType = cBoxInertia;
         else if (child.getText(tempStr).compare(B("fixed")) == 0)
            mMotionType = cFixed;
      }
     /* else if (child.getName().compare(B("SplashEffect"))== 0)
      {
         if (gModelManager && gModelManager->getParticleSystemManager())
         {
            gModelManager->getParticleSystemManager()->getParticleSet(child.getText(), &mSplashEffectHandle);
         }
      }*/

   }
   
   // Success.
   mLoaded = true;
   mFailedToLoad = false;
   return(true);
}


//==============================================================================
// BPhysicsInfo::unload
//==============================================================================
void BPhysicsInfo::unload(void)
{
   mLoaded = false;
   mFailedToLoad = false;
}

//==============================================================================
//==============================================================================
bool BPhysicsInfo::isAircraft() const
{
   switch (mVehicleType)
   {
      case cHawk:
      case cHornet:
      case cVulture:
      case cBanshee:
      case cVampire:
      case cSentinel:
         return true;
      default:
         return false;
   }
}


//==============================================================================
// eof: physicsinfo.cpp
//==============================================================================
