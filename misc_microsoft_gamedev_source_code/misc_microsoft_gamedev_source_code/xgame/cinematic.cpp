//==============================================================================
// cinematic.cpp
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "cinematic.h"
#include "database.h"
#include "gamesettings.h"
#include "render.h"
#include "camera.h"

#include "user.h"
#include "usermanager.h"
#include "renderControl.h"
#include "world.h"
#include "worldsoundmanager.h"
#include "scenario.h"
#include "particlegateway.h"
#include "modemanager.h"
#include "triggermanager.h"
#include "triggerscript.h"
#include "syncmacros.h"
#include "gamedirectories.h"
#include "generaleventmanager.h"
#include "uimanager.h"
/*
#include "screenTransition.h"
*/

// xsystem
#include "reloadManager.h"
#include "xmlreader.h"

// xvisual
#include "visualmanager.h"
#include "xvisual.h"

// xgranny
#include "grannymanager.h"

// xsound
#include "soundmanager.h"

// xgamerender
#include "lightEffectManager.h"


#include "tonemapManager.h"


#include "primDraw2D.h"

#include "protoobject.h"


#include "physics.h"
#include "physicsobject.h"
#include "physicsinfo.h"
#include "physicsinfomanager.h"

#include "transitionManager.h"
#include "configsgame.h"
#include "binkInterface.h"
#include <io.h>
#include "modegame.h"

char *gShotTagTypeNames[] = {     "Particle",
                                  "Sound",
                                  "Trigger",
                                  "Chat" };

char *gModelTypeNames[] = {       "proto",
                                  "gr2" };

char *gStartEndModeNames[] = {    "start",
                                  "end" };

#ifndef ARGBToDWORD
   #define ARGBToDWORD(a, r, g, b) DWORD((a<<24)|(r<<16)|(g<<8)|(b))
#endif


//============================================================================
// BMiniParticleManager (class used to release particle instances)
//============================================================================
class BMiniParticleManager
{
private:
   class BMiniParticle
   {
   public:
      BMiniParticle() {}
      BMiniParticle(BParticleInstance* pInstance, float lifetime) :
                     mpParticleInstance(pInstance),
                     mTimeLeft(lifetime) {}
      ~BMiniParticle() {}

      BParticleInstance*   mpParticleInstance;
      float                mTimeLeft;
   };


public:
   void clear()
   {
      for(int i = 0; i < mParticles.getNumber(); i++)
      {
         gParticleGateway.releaseInstance(mParticles[i].mpParticleInstance, true);
      }

      mParticles.clear();
   }

   void update(float elapsedTime)
   {
      for(int i = 0; i < mParticles.getNumber(); i++)
      {
         mParticles[i].mTimeLeft -= elapsedTime;
         if(mParticles[i].mTimeLeft < 0.0f)
         {
            gParticleGateway.releaseInstance(mParticles[i].mpParticleInstance, false);
            mParticles.removeIndex(i);
         }
      }
   }

   void add(BParticleInstance* pInstance, float lifetime)
   {
      mParticles.add(BMiniParticle(pInstance, lifetime));
   }

private:
   BSmallDynamicSimArray<BMiniParticle> mParticles;
};


static BMiniParticleManager sCinematicParticleManager;


//============================================================================
// BShotTag::BShotTag
//============================================================================
BShotTag::BShotTag() :
   mTagType(cShotTagTypeParticles),
   mTime(0.0f),
   mLifespan(0.0f),
   mPosition(cOriginVector),
   mToModelID(-1),
   mToBoneHandle(-1),
   mTalkingHeadID(-1),
   mStringID(-1),
   mData0(-1),
   mBool0(false)
{
   mPosition.set(cMinimumFloat, cMinimumFloat, cMinimumFloat);
}

//============================================================================
// BShotTag::~BShotTag
//============================================================================
BShotTag::~BShotTag()
{
}



//============================================================================
// BShotTransition::BShotTransition
//============================================================================
BShotTransition::BShotTransition() :
   mFadeUp(0.0f),
   mHold(0.0f),
   mFadeDown(0.0f),
   mStarted(false)
{
   mColor.set(0.0f, 0.0f, 0.0f);
}

//============================================================================
// BShotTransition::~BShotTransition
//============================================================================
BShotTransition::~BShotTransition()
{
}




//============================================================================
// BShot::BShot
//============================================================================
BShot::BShot() :
   mCameraEffectInstance(NULL),
   mDuration(0.0f),
   mStartTime(0.0f),
   mEndTime(0.0f),
   mpStartTransition(NULL),
   mpEndTransition(NULL)
{
}

//============================================================================
// BShot::~BShot
//============================================================================
BShot::~BShot()
{
   if(mCameraEffectInstance != NULL)
   {
      gLightEffectManager.releaseCameraInstance(mCameraEffectInstance);
   }

   // Release transition
   if(mpStartTransition)
   {
      delete(mpStartTransition);
   }

   if(mpEndTransition)
   {
      delete(mpEndTransition);
   }
}


BToneMapParams    BCinematic::s_cachedToneMapperDOFParams;

BQuaternion       BCinematic::s_cacheGameCameraOrient;
BVector           BCinematic::s_cacheGameCameraPos;
float             BCinematic::s_cacheGameCameraFOV;

//============================================================================
// BCinematicModel::BCinematicModel
//============================================================================
BCinematicModel::BCinematicModel() :
   mpGrannyInstance(NULL),
   mIsPossessed(false),
   mpUnit(NULL),
   mpSquad(NULL),
   mType(cModelTypeProto),
   mProtoID(-1),
   mGrannyModelID(-1),
   mPlayerID(0)
{
   mWorldMatrix = XMMatrixIdentity();
}

//============================================================================
// BCinematicModel::~BCinematicModel
//============================================================================
BCinematicModel::~BCinematicModel()
{
   sCinematicParticleManager.clear();
}


//============================================================================
// BCinematic::BCinematic
//============================================================================
BCinematic::BCinematic() :
   mState(cStateEnded),
   mCameraMode(cCameraModeLocked),
   mActiveShot(-1),
   mVideoHandle(cInvalidVideoHandle),
   mCurrentTime(0.0f),
   mTotalDuration(0.0f),
   mSoundCue(cInvalidCueHandle),
   mbIsScenarioStart(false),
   mbDOFEnabled(false),
   mLoadErrorsMsgPtr(NULL)
{
   clearFlags();
   mCurrentCameraMatrix = XMMatrixIdentity();

#ifdef ENABLE_RELOAD_MANAGER
   eventReceiverInit();
#endif
}

//============================================================================
// BCinematic::~BCinematic
//============================================================================
BCinematic::~BCinematic()
{
#ifdef ENABLE_RELOAD_MANAGER
   gReloadManager.deregisterClient(mEventHandle);
#endif
   deinit();

#ifdef ENABLE_RELOAD_MANAGER
   eventReceiverDeinit();
#endif
}


//==============================================================================
// BCinematic::init
//==============================================================================
bool BCinematic::init(long dirID, const BCHAR_T* pfilename)
{
   // Cleanse out any existing info.
   deinit();

   // Check param.
   if(!pfilename)
      return(false);

   BString filename;
   gFileManager.constructQualifiedPath(dirID, BString(pfilename), filename);
   if (filename.findLeft("game:\\") == 0)
      filename.remove(0, 6);
   
   mFilename = filename;

   return(true);
}

//==============================================================================
// BCinematic::deinit
//==============================================================================
void BCinematic::deinit()
{
   gBinkInterface.unregisterValidCallback(this);

   clearFlags();

   mActiveShot = -1;
   mCurrentTime = 0.0f;
   mTotalDuration = 0.0f;
   mScenarioFilename.empty();
   mSoundFilename.empty();
   mCurrentCameraMatrix = XMMatrixIdentity();

   mStartCameraInterpolateDuration = 2.0f;
   mEndCameraInterpolateDuration = 2.0f;
   mbStartCameraInterpolate = false;
   mbEndCameraInterpolate = false;

   stopSound();

   for(int i = 0; i < mModels.getNumber(); i++)
   {
      if(mModels[i]->mpGrannyInstance)
         gGrannyManager.releaseInstance(mModels[i]->mpGrannyInstance);
      delete(mModels[i]);
   }
   mModels.clear();


   for(int i = 0; i < mShots.getNumber(); i++)
   {
      mShots[i]->mAnimatedModels.clear();
      delete(mShots[i]);
   }

   mShots.clear();
}


//==============================================================================
// BCinematic::clearFlags
//==============================================================================
void BCinematic::clearFlags()
{
   //-- flags   
   mFlagIsLoaded=false;
   mFlagAreUnitsCreated=false;
   mFlagPlayingVideo=false;
   mFlagVideoLoadFailed=false;
   mFlagPlayingChat=false;
}

//==============================================================================
// BCinematic::load
//==============================================================================
bool BCinematic::load(BSimString *errorMsgs)
{
   mFlagIsLoaded = false;

   mLoadErrorsMsgPtr = errorMsgs;
   if(errorMsgs) errorMsgs->empty();


   BSimString errorMsg;

#ifdef ENABLE_RELOAD_MANAGER
   gReloadManager.deregisterClient(mEventHandle);


   BReloadManager::BPathArray paths;
   BString fullpath;
   gFileManager.constructQualifiedPath(0, mFilename, fullpath);
   paths.pushBack(fullpath);
   gReloadManager.registerClient(paths, BReloadManager::cFlagSynchronous, mEventHandle);
#endif

   BXMLReader reader;
   if(!reader.load(0, mFilename.getPtr()))
   {
      errorMsg.format("Error:  Invalid XML file \"%s\"\n", mFilename.getPtr());
      if(errorMsgs) errorMsgs->append(errorMsg);
      gConsoleOutput.output(cMsgError, "BCinematic::load - %s", errorMsg.getPtr());
      return false;
   }

   BXMLNode rootNode(reader.getRootNode());


   // Read scenario
   rootNode.getAttribValueAsString("scenariofile", mScenarioFilename);


   // Read sound file
   rootNode.getAttribValueAsString("soundfile", mSoundFilename);

   
   // Read scenario start
   rootNode.getAttribValueAsBool("scenariostart", mbIsScenarioStart);


   // Read shots
   long nodeCount=rootNode.getNumberChildren();
   for(long i=0; i<nodeCount; i++)
   {
      BXMLNode xmlNode(rootNode.getChild(i));
      const BPackedString nodeName(xmlNode.getName());

      if(nodeName=="head")
      {
         readHead(xmlNode, errorMsgs);
      }
      else if(nodeName=="body")
      {
         readBody(xmlNode, errorMsgs);
      }
   }


   
   // create trigger tags
   mTriggerTagTimes.clear();
   for(uint i=0; i<mShots.getSize(); i++)
   {
      uint numTags = mShots[i]->mTags.getSize();
      for(uint j=0; j<numTags; j++)
      {
         if(mShots[i]->mTags[j].mTagType == cShotTagTypeTrigger)
         {
            float triggerTabTime = mShots[i]->mStartTime + mShots[i]->mTags[j].mTime;
            mTriggerTagTimes.add(triggerTabTime);
         }
      }
   }

   mFlagIsLoaded = true;
   return(true);
}

//==============================================================================
// BCinematic::readHead
//==============================================================================
bool BCinematic::readHead(BXMLNode headNode, BSimString *errorMsgs)
{   
   BSimString errorMsg;

   long nodeCount=headNode.getNumberChildren();
   for(long i=0; i<nodeCount; i++)
   {
      BXMLNode xmlNode(headNode.getChild(i));
      const BPackedString nodeName(xmlNode.getName());

      if(nodeName=="model")
      {
         BSimString modelName, typeName, modelFile;

         // read model name
         xmlNode.getAttribValueAsString("name", modelName);

         long modelType = cModelTypeGr2;
         if(xmlNode.getAttribValueAsString("type", typeName))
         {
            if(typeName == gModelTypeNames[0])
               modelType = cModelTypeProto;     
            else if(typeName == gModelTypeNames[1])
               modelType = cModelTypeGr2;
         }

         long playerID = 0;
         xmlNode.getAttribValueAsLong("player", playerID);

         xmlNode.getAttribValueAsString("modelfile", modelFile);

         long protoID = -1;
         long grannyModelID = -1;

         switch(modelType)
         {
            case cModelTypeProto:
               protoID = gDatabase.getProtoObject(modelFile);
               if(protoID==-1)
               {
                  errorMsg.format("Warning:  Model file \"%s\" is not a valid unit for model name \"%s\"\n", modelFile.getPtr(), modelName.getPtr());
                  if(errorMsgs) errorMsgs->append(errorMsg);
                  gConsoleOutput.output(cMsgError, "BCinematic::load - %s", errorMsg.getPtr());
                  continue;
               }
               break;

            case cModelTypeGr2:
               
               grannyModelID = gGrannyManager.getOrCreateModel(modelFile, true);
               if(grannyModelID == -1)
               {
                  errorMsg.format("Warning:  Model file \"%s\" not found for model name \"%s\"\n", modelFile.getPtr(), modelName.getPtr());
                  if(errorMsgs) errorMsgs->append(errorMsg);
                  gConsoleOutput.output(cMsgError, "BCinematic::load - %s", errorMsg.getPtr());
                  continue;
               }
               break;
         }


         BCinematicModel *pCinModel = new BCinematicModel();

         pCinModel->mName = modelName;
         pCinModel->mType = modelType;
         pCinModel->mPlayerID = playerID;

         pCinModel->mProtoID = protoID;
         pCinModel->mGrannyModelID = grannyModelID;

         mModels.add(pCinModel);
      }
      else if(nodeName=="cameralerp")
      {
         BSimString modeName;
         long mode = cModeStart;
         float duration;

         // Read attributes
         //
         xmlNode.getAttribValueAsFloat("duration", duration);

         // read mode
         if(xmlNode.getAttribValueAsString("mode", modeName)) 
         {
            if(modeName == gStartEndModeNames[0])
            {
               mode = cModeStart;
            }
            else if(modeName == gStartEndModeNames[1])
            {
               mode = cModeEnd;
            }               
            else
            {                     
               errorMsg.format("Warning:  Unknown mode name \"%s\" for camera lerp\n", modeName.getPtr());
               if(errorMsgs) errorMsgs->append(errorMsg);
               gConsoleOutput.output(cMsgError, "BCinematic::load - %s", errorMsg.getPtr());
               continue;
            }
         }

         // check for valid mode
         if((mode == cModeStart) && (mbStartCameraInterpolate))
         {
            errorMsg.format("Warning:  More than one start cameralerp node found.  Only one allowed.\n");
            if(errorMsgs) errorMsgs->append(errorMsg);
            gConsoleOutput.output(cMsgError, "BCinematic::load - %s", errorMsg.getPtr());
            continue;
         }
         if((mode == cModeEnd) && (mbEndCameraInterpolate))
         {
            errorMsg.format("Warning:  More than one start cameralerp node found.  Only one allowed.\n");
            if(errorMsgs) errorMsgs->append(errorMsg);
            gConsoleOutput.output(cMsgError, "BCinematic::load - %s", errorMsg.getPtr());
            continue;
         }


         switch(mode)
         {
            case cModeStart:
               mStartCameraInterpolateDuration = duration;
               mbStartCameraInterpolate = true;
               break;
            case cModeEnd:               
               mEndCameraInterpolateDuration = duration;
               mbEndCameraInterpolate = true;
               break;
         }

      }
      else if(nodeName=="depthoffield")
      {
         xmlNode.getAttribValueAsBool("enabled", mbDOFEnabled);
      }
   }
   return(true);
}

//==============================================================================
// BCinematic::readBody
//==============================================================================
bool BCinematic::readBody(BXMLNode bodyNode, BSimString *errorMsgs)
{
   BSimString errorMsg;

   // Read shots
   long nodeCount=bodyNode.getNumberChildren();
   for(long i=0; i<nodeCount; i++)
   {
      BXMLNode xmlNode(bodyNode.getChild(i));
      const BPackedString nodeName(xmlNode.getName());


      if(nodeName=="shot")
      {
         BShot *pShot = new BShot();
         float cameraDuration = -1.0f;
         float maxAnimDuration = -1.0f;

         // read shot name
         xmlNode.getAttribValueAsString("name", pShot->mName);

         // read camera animation
         BSimString cameraFile;
         xmlNode.getAttribValueAsString("camera", cameraFile);

         if(!cameraFile.isEmpty())
         {
            long effectID = -1;
            gLightEffectManager.getOrCreateData(cameraFile, effectID);

            if(effectID != -1)
            {
               BMatrix mtx;
               mtx.makeIdentity();

               BCameraEffectInstance *effectInstance = gLightEffectManager.createCameraInstance(effectID, mtx);

               if(effectInstance->hasCamera())
               {
                  pShot->mCameraEffectInstance = effectInstance;
                  cameraDuration = pShot->mCameraEffectInstance->getDuration();
               }
               else
               {
                  errorMsg.format("Warning:  Camera animation \"%s\" does not have a camera\n", cameraFile.getPtr());
                  if(errorMsgs) errorMsgs->append(errorMsg);
                  gConsoleOutput.output(cMsgError, "BCinematic::load - %s", errorMsg.getPtr());
                  gLightEffectManager.releaseCameraInstance(effectInstance);
               }
            }

         }


         // read all animated models in shot
         long nodeChildCount=xmlNode.getNumberChildren();
         for(long j=0; j<nodeChildCount; j++)
         {
            BXMLNode nodeChild(xmlNode.getChild(j));
            const BPackedString nodeChildName(nodeChild.getName());

            if(nodeChildName=="animatedmodel")
            {
               BSimString modelName, animFile;
               BModelAnimationCombo modelAnimCombo;

               // Read and set model
               //
               nodeChild.getAttribValueAsString("model", modelName);

               long modelID = findModel(modelName);

               if(modelID == -1)
               {
                  errorMsg.format("Warning:  Unable to find model \"%s\" for shot \"%s\"\n", modelName.getPtr(), pShot->mName.getPtr());
                  if(errorMsgs) errorMsgs->append(errorMsg);
                  gConsoleOutput.output(cMsgError, "BCinematic::load - %s", errorMsg.getPtr());
                  continue;
               }

               modelAnimCombo.mModelID = modelID;


               // Read and set animation
               //
               nodeChild.getAttribValueAsString("animationfile", animFile);

               modelAnimCombo.mAnimationID = gGrannyManager.getOrCreateAnimation(animFile, true);
               BGrannyAnimation *pAnim = gGrannyManager.getAnimation(modelAnimCombo.mAnimationID);

               if(pAnim == NULL)
               {
                  errorMsg.format("Warning:  Animation file \"%s\" not found for shot \"%s\"\n", animFile.getPtr(), pShot->mName.getPtr());
                  if(errorMsgs) errorMsgs->append(errorMsg);
                  gConsoleOutput.output(cMsgError, "BCinematic::load - %s", errorMsg.getPtr());
                  continue;
               }
               else
               {
                  float animDuration = pAnim->getDuration();
                  maxAnimDuration = max(maxAnimDuration, animDuration);

                  if(cameraDuration == -1.0)
                  {
                     if(cameraDuration != animDuration)
                     {
                        errorMsg.format("Warning:  Not all model/camera animations have the same duration in shot \"%s\"\n", pShot->mName.getPtr());
                        if(errorMsgs) errorMsgs->append(errorMsg);
                        gConsoleOutput.output(cMsgError, "BCinematic::load - %s", errorMsg.getPtr());
                     }
                  }

                  pShot->mAnimatedModels.add(modelAnimCombo);
               }
            }
            else if(nodeChildName=="tag")
            {
               BShotTag tag;
               if(readTag(nodeChild, &tag, errorMsgs))
               {
                  pShot->mTags.add(tag);
               }
            }
            else if(nodeChildName=="transition")
            {
               BSimString modeName;
               long mode = cModeStart;
               float fUp = 0.0f, fDown = 0.0f, hold = 0.0f;
               BVector color(0.0f, 0.0f, 0.0f);

               // Read attributes
               //

               // read duration
               nodeChild.getAttribValueAsFloat("fadeup", fUp);
               nodeChild.getAttribValueAsFloat("hold", hold);
               nodeChild.getAttribValueAsFloat("fadedown", fDown);

               // read mode
               if(nodeChild.getAttribValueAsString("mode", modeName))
               {
                  if(modeName == gStartEndModeNames[0])
                  {
                     mode = cModeStart;
                  }
                  else if(modeName == gStartEndModeNames[1])
                  {
                     mode = cModeEnd;
                  }               
                  else
                  {                     
                     errorMsg.format("Warning:  Unknown mode name \"%s\" in transition for shot \"%s\"\n", modeName.getPtr(), pShot->mName.getPtr());
                     if(errorMsgs) errorMsgs->append(errorMsg);
                     gConsoleOutput.output(cMsgError, "BCinematic::load - %s", errorMsg.getPtr());
                     continue;
                  }
               }

               // Read children
               //
               const int nodeCount = nodeChild.getNumberChildren();
               for(int i = 0; i < nodeCount; i++)
               {
                  BXMLNode node(nodeChild.getChild(i));
                  const BPackedString nodeName(node.getName());
                  
                  if (nodeName=="color")
                  {
                     node.getTextAsVector(color);
                  }
               }


               // Validation
               //

               // check valid duration
               if((fUp + hold + fDown) == 0.0f)
               {
                  errorMsg.format("Warning:  Shot \"%s\" has a transition with duration of 0.0f.  This is invalid.\n", pShot->mName.getPtr());
                  if(errorMsgs) errorMsgs->append(errorMsg);
                  gConsoleOutput.output(cMsgError, "BCinematic::load - %s", errorMsg.getPtr());
                  continue;
               }

               // check for valid mode
               if((mode == cModeStart) && (pShot->mpStartTransition != NULL))
               {
                  errorMsg.format("Warning:  Shot \"%s\" has more than one start transition.  Only one allowed.\n", pShot->mName.getPtr());
                  if(errorMsgs) errorMsgs->append(errorMsg);
                  gConsoleOutput.output(cMsgError, "BCinematic::load - %s", errorMsg.getPtr());
                  continue;
               }
               if((mode == cModeEnd) && (pShot->mpEndTransition != NULL))
               {
                  errorMsg.format("Warning:  Shot \"%s\" has more than one end transition.  Only one allowed.\n", pShot->mName.getPtr());
                  if(errorMsgs) errorMsgs->append(errorMsg);
                  gConsoleOutput.output(cMsgError, "BCinematic::load - %s", errorMsg.getPtr());
                  continue;
               }


               // Create transition
               //

               BShotTransition *transition = new BShotTransition();
               transition->mFadeUp = fUp;
               transition->mHold = hold;
               transition->mFadeDown = fDown;
               transition->mColor = color;

               switch(mode)
               {
                  case cModeStart:
                     pShot->mpStartTransition = transition;
                     break;
                  case cModeEnd:
                     pShot->mpEndTransition = transition;
                     break;
               }
            }
         }

         // The shot duration will be the camera duration (if it exists) or the when it doesn't the
         // largets of the model anim durations
         float shotDuration = -1.0f;

         if(cameraDuration != -1.0f)
            shotDuration = cameraDuration;
         else
            shotDuration = maxAnimDuration;

         pShot->mDuration = (shotDuration > 0.0f) ? shotDuration : 0.0f;
         pShot->mStartTime = mTotalDuration;
         pShot->mEndTime = pShot->mStartTime + pShot->mDuration;
         mShots.add(pShot);

         mTotalDuration += pShot->mDuration;
      }
   }

   return(true);
}


//============================================================================
// BCinematic::readTag
//============================================================================
bool BCinematic::readTag(BXMLNode node, BShotTag *pTag, BSimString *errorMsgs)
{
   BSimString errorMsg;

   const BXMLAttribute attr(node.getAttribute("type"));
   if(attr)
   {
      BSimString type;
      attr.getValue(type);
      if(type == gShotTagTypeNames[0])
         pTag->mTagType = cShotTagTypeParticles;     
      else if(type == gShotTagTypeNames[1])
         pTag->mTagType = cShotTagTypeSound;     
      else if(type == gShotTagTypeNames[2])
         pTag->mTagType = cShotTagTypeTrigger;     
      else if(type == gShotTagTypeNames[3])
         pTag->mTagType = cShotTagTypeChat;

   }

   node.getAttribValueAsFloat("time", pTag->mTime);

   switch(pTag->mTagType)
   {
      case cShotTagTypeParticles:
         {
            node.getAttribValueAsString("name", pTag->mName);
            node.getAttribValueAsVector("position", pTag->mPosition);
            node.getAttribValueAsFloat("lifespan", pTag->mLifespan);

            BSimString modelName;
            if(node.getAttribValueAsString("tomodel", modelName))
            {
               pTag->mToModelID = findModel(modelName);

               if(pTag->mToModelID == -1)
               {
                  errorMsg.format("Warning:  Unable to find model \"%s\" for tag at position \"%f\"\n", modelName.getPtr(), pTag->mPosition);
                  if(errorMsgs) errorMsgs->append(errorMsg);
                  gConsoleOutput.output(cMsgError, "BCinematic::load - %s", errorMsg.getPtr());

                  return(false);
               }
            }

            if(node.getAttribValueAsString("tobone", pTag->mToBoneName))
            {
               // Make sure using "proto" type if attaching to bone
               if(mModels[pTag->mToModelID]->mType != cModelTypeProto)
               {
                  errorMsg.format("Warning:  Tag with \"tobone\" attachment must point to a model of type \"proto\".  Models of type \"gr2\" are not supported.\n");
                  if(errorMsgs) errorMsgs->append(errorMsg);
                  gConsoleOutput.output(cMsgError, "BCinematic::load - %s", errorMsg.getPtr());
                  
                  return(false);
               }
            }

            gParticleGateway.getOrCreateData(pTag->mName, (BParticleEffectDataHandle&)pTag->mData0);
         }
         break;

      case cShotTagTypeSound:
         node.getAttribValueAsString("name", pTag->mName);
         break;      
      
      case cShotTagTypeTrigger:
         node.getAttribValueAsString("name", pTag->mName);
         break;

      case cShotTagTypeChat:
         {
            node.getAttribValueAsBool("queue", pTag->mBool0);
            node.getAttribValueAsFloat("duration", pTag->mLifespan);
            node.getAttribValueAsLong("stringID", pTag->mStringID);
            node.getAttribValueAsString("sound", pTag->mToBoneName);
            node.getAttribValueAsString("talkinghead", pTag->mTalkingHeadName);
            #ifndef BUILD_FINAL
            if (!gConfig.isDefined(cConfigDisableTalkingHeads))
            #endif
            {
               if (!pTag->mTalkingHeadName.isEmpty())
               {
                  BSimString dir;
                  gFileManager.getDirListEntry(dir, cDirTalkingHead);
                  BSimString fileName = dir;
                  fileName += pTag->mTalkingHeadName;
                  strPathAddExtension(fileName, "bik");
                  int talkingHeadID = gWorld->getTalkingHeadID(fileName);
                  if (talkingHeadID == -1)
                  {
                     talkingHeadID = gWorld->getNextTalkingHeadID();
                     gWorld->addTalkingHead(fileName, talkingHeadID, true);
                  }
                  pTag->mTalkingHeadID = talkingHeadID;
               }
            }
         }
         break;
   }

   return(true);
}


//============================================================================
// BCinematic::findModel
//============================================================================
long BCinematic::findModel(BSimString &modelName)
{
   // Find cinematic model index given the model name
   //
   for(long i = mModels.getNumber() - 1; i >= 0; i--)
   {
      if(mModels[i]->mName.compare(modelName) == 0)
      {
         return i;
      }
   }

   return -1;
}

//============================================================================
// BCinematic::reload
//============================================================================
bool BCinematic::reload()
{
   if (mFilename.isEmpty())
      return false;
      
   gConsoleOutput.status("Reloading cinematic: %s", mFilename.getPtr());

   releaseUnits();
   deinit();

   return load(mLoadErrorsMsgPtr);  
}


/*
//============================================================================
// BCinematic::findVisual
//============================================================================
int BCinematic::findVisual(BSimString &visualName)
{
   for(i = mVisuals.getNumber() - 1; i >= 0; i--)
   {
      if(mVisuals[i]->mpName->compare(visualName) == 0)
      {
         return i;
      }
   }

   return -1;
}

//============================================================================
// BCinematic::addVisual
//============================================================================
int BCinematic::addVisual(BSimString &visualName)
{
   // Load the visual first
   BVisual 

   // Visual
   long visualIndex=gVisualManager.getOrCreateProtoVisual("unsc\\infantry\\unsc_inf_spartan_01\\unsc_inf_spartan_01", true);
   mpVisual=gVisualManager.createVisual(visualIndex, false, 0);

}
*/


//============================================================================
// BCinematic::render
//============================================================================
void BCinematic::render()
{
   if((!mFlagIsLoaded) || (mActiveShot == -1))
      return;


   // Make render attributes
   BVisualRenderAttributes renderAttribs;
   BVector objectMin;
   BVector objectMax;



   // Only render models in current shot
   if(mActiveShot != -1)
   {
      long numAnimatedModels = mShots[mActiveShot]->mAnimatedModels.getNumber();
      for(long i = 0; i < numAnimatedModels; i++)
      {
         int modelID = mShots[mActiveShot]->mAnimatedModels[i].mModelID;
         BCinematicModel *pCineModel = mModels[modelID];

         switch(pCineModel->mType)
         {
            case(cModelTypeGr2):
               gRender.setWorldMatrix(pCineModel->mWorldMatrix);

               pCineModel->mBox.computeMinCornerAABB(objectMin);
               pCineModel->mBox.computeMaxCornerAABB(objectMax);
               renderAttribs.setBounds(objectMin, objectMax);
               renderAttribs.mPixelXFormColor = gWorld->getPlayerColor(pCineModel->mPlayerID, BWorld::cPlayerColorContextObjects);

               pCineModel->mpGrannyInstance->render(&renderAttribs);
               break;
         }
      }
   }

   if((mCameraMode == cCameraModeFree) &&
      (mActiveShot != -1) &&
      (mShots[mActiveShot]->mCameraEffectInstance))
   {
      // Apply global matrix
      gRender.setWorldMatrix(XMMatrixIdentity());

      // When the camera is free use debug primitive to this play where the camera
      // during the current shot.
      BMatrix matrix = mCurrentCameraMatrix;
      
      BVector right, forward, up, trans;
      matrix.getRight(right);
      matrix.getForward(forward);
      matrix.getUp(up);
      matrix.getTranslation(trans);

      right.normalize();
      forward.normalize();
      forward.inverse();
      up.normalize();
      up.inverse();
            
      matrix.makeOrient(forward, up, right);
      matrix.setTranslation(trans);


      float size = 0.4f;
      gpDebugPrimitives->addDebugBox(matrix, BVector(size, size, size*2), cDWORDRed, BDebugPrimitives::cCategoryFormations);
      gpDebugPrimitives->addDebugArrow(matrix, BVector(size, size, size*2), cDWORDGreen, BDebugPrimitives::cCategoryFormations);

      BMatrix rotMatrix;
      rotMatrix.makeRotateZ(180.0f * cRadiansPerDegree);
     
      matrix.mult(rotMatrix, matrix);
      gpDebugPrimitives->addDebugArrow(matrix, BVector(size, size, size*2), cDWORDGreen, BDebugPrimitives::cCategoryFormations);
   }
}




//============================================================================
// BCinematic::postRender
//============================================================================
void BCinematic::postRender()
{

}


/*
//============================================================================
// BCinematic::renderOverlay
//============================================================================
void BCinematic::renderOverlay()
{
   gRenderThread.submitFunctor(BRenderThread::BFunctor(this, &BCinematic::workerRenderTransitions));
}


//============================================================================
// BCinematic::workerRenderTransitions
//============================================================================
void BCinematic::workerRenderTransitions(void* pData)
{
   ASSERT_RENDER_THREAD

   // Render transitions
   //
   if(mActiveShot != -1)
   {
      long numTransitions = mShots[mActiveShot]->mTransitions.getNumber();
      for(long i = 0; i < numTransitions; i++)
      {
         BScreenTransition *pTransition = mShots[mActiveShot]->mTransitions[i];
         pTransition->render();
      }
   }
}
*/

//============================================================================
// BCinematic::update
//============================================================================
void BCinematic::update(float elapsedTime)
{
   // Early out if not loaded
   if(!mFlagIsLoaded)
      return;

   if(mState == cStateEnded)
      return;

   if (mFlagPlayingVideo)
   {
      if (mFlagVideoLoadFailed)
         onVideoEnded(mVideoHandle, NULL, cBinkStatus_ReadError);
      else
         updateVideo(elapsedTime);
      return;
   }

   if(!mFlagAreUnitsCreated)
      createUnits();

   float lastFrameTime = mCurrentTime;
   mCurrentTime += elapsedTime;

   // End cinematic
   if(mCurrentTime > mTotalDuration)
   {      
      end();
      return;
   }

   sCinematicParticleManager.update(elapsedTime);

   // Compute current shot
   int currentShot = getCurrentShotID();

   if(currentShot == -1)
      return;

   float shotTime = mCurrentTime - mShots[currentShot]->mStartTime;

   if(mActiveShot != currentShot)
   {
      if(mActiveShot != -1)
      {
         clearActiveShot();
      }

      mActiveShot = currentShot;


      BShot *pShot = mShots[mActiveShot];
      for(int i = 0; i < pShot->mAnimatedModels.getNumber(); i++)
      {
         long modelID = pShot->mAnimatedModels[i].mModelID;
         long animID = pShot->mAnimatedModels[i].mAnimationID;

         BCinematicModel *mCineModel = mModels[modelID];
         switch(mCineModel->mType)
         {
            case(cModelTypeProto):
               if(mCineModel->mpUnit)
               {
                  mCineModel->mpUnit->removeAllActionsOfType(BAction::cActionTypeUnitPlayBlockingAnimation);
                  mCineModel->mpUnit->beginPlayBlockingAnimation(BObjectAnimationState::cAnimationStateMisc, cAnimTypeCinematic, 
                                                                  true,       // applyInstantly
                                                                  false,      // useMaxHeight
                                                                  true,       // forceReset
                                                                  animID);    // forceAnim

                  // position unit
                  BGrannyAnimation *pAnim = gGrannyManager.getAnimation(animID);
                  granny_transform transform = pAnim->getGrannyFileInfo()->Animations[0]->TrackGroups[0]->InitialPlacement;

                  BMatrix matrix;
                  GrannyBuildCompositeTransform4x4(&transform, (granny_real32*)&matrix);

                  mCineModel->mpUnit->setPosition(matrix.getRow(3));
                  mCineModel->mpUnit->setRotation(matrix);

                  if(!mCineModel->mIsPossessed)
                  {
                     // Make visible
                     mCineModel->mpUnit->setFlagNoRender(false);
                  }
               }
               break;

            case(cModelTypeGr2):
               {
                  mCineModel->mpGrannyInstance->playAnimation(0, animID, 1.0f, shotTime, 1, 0.0f);
                  mCineModel->mpGrannyInstance->playAnimation(1, animID, 1.0f, shotTime, 1, 0.0f);

                  // position unit
                  BGrannyAnimation *pAnim = gGrannyManager.getAnimation(animID);
                  granny_transform transform = pAnim->getGrannyFileInfo()->Animations[0]->TrackGroups[0]->InitialPlacement;

                  GrannyBuildCompositeTransform4x4(&transform, (granny_real32*)&mCineModel->mWorldMatrix);
               }

               break;
         }
      }
   }      
   

   if(mActiveShot != -1)
   {
      // Update animation time
      //
      BShot *pShot = mShots[mActiveShot];
      for(int i = 0; i < pShot->mAnimatedModels.getNumber(); i++)
      {
         long modelID = pShot->mAnimatedModels[i].mModelID;

         BCinematicModel *mCineModel = mModels[modelID];
         switch(mCineModel->mType)
         {
            case(cModelTypeProto):
               break;

            case(cModelTypeGr2):
               mCineModel->mpGrannyInstance->update(elapsedTime);

               if (mCineModel->mpGrannyInstance->hasMotionExtraction())
               {
                  BMatrix motionMatrix;
                  motionMatrix = mCineModel->mWorldMatrix;

                  mCineModel->mpGrannyInstance->getExtractedMotion(elapsedTime, motionMatrix);

                  BVector newPosition;
                  motionMatrix.getTranslation(newPosition);

                  if(Math::IsValidFloat(newPosition.x))
                  {
                     mCineModel->mWorldMatrix = motionMatrix;
                  }
               }

               break;
         }
      }
   }

   
   float fov = DEGREES_TO_RADIANS(45);
   mCurrentCameraMatrix = XMMatrixIdentity();


   BShot *pShot = mShots[mActiveShot];


   // Trigger tags
   //
   for(int i = 0; i < pShot->mTags.getNumber(); i++)
   {
      float absoluteTagTime = pShot->mTags[i].mTime + pShot->mStartTime;

      float p1 = lastFrameTime;
      float p2 = mCurrentTime;

      if((absoluteTagTime<cFloatCompareEpsilon && p1<cFloatCompareEpsilon && p2>=cFloatCompareEpsilon) ||
         (absoluteTagTime>p1 && absoluteTagTime<=p2))
      {
         // Trigger tag
         triggerTag(&pShot->mTags[i]);
      }
   }      
   

   // Update model bounding boxes for "gr2" types.
   //
   long numModels = pShot->mAnimatedModels.getNumber();
   for(int i = 0; i < numModels; i++)
   {
      long modelID = pShot->mAnimatedModels[i].mModelID;
      BGrannyInstance *pGrannyInst = mModels[modelID]->mpGrannyInstance;
      if(pGrannyInst)
      {
         BVector min, max;
         pGrannyInst->computeBoundingBox(&min, &max, true);

         mModels[modelID]->mBox.initializeTransformed(min, max, mModels[modelID]->mWorldMatrix);
      }
   }


   float dofDepth = 0.0f;
   float dofNear = 0.0f;
   float dofFar = 0.0f;
   float nearClip = 0.0f;
   float farClip = 0.0f;
   bool dofParamsValid = false;
   bool clippingParamsValid = false;
      
   if(pShot->mCameraEffectInstance)
   {
      // Clamp camera time
      if(shotTime > pShot->mCameraEffectInstance->getDuration())
         shotTime = pShot->mCameraEffectInstance->getDuration() - 0.000001f;

      // Update camera matrix
      pShot->mCameraEffectInstance->setTime(shotTime);
      pShot->mCameraEffectInstance->tick(0.0f, XMMatrixIdentity(), mCurrentCameraMatrix, fov, dofDepth, dofNear, dofFar, nearClip, farClip);

      // -200 is a fudge factor so the dof focal plane can be set behind the camera - max doesn't allow values < 0
      dofNear -= 200.0f;
      
      dofParamsValid = (dofNear <= dofFar) && (dofFar > 0.0f) && (dofFar < 100000.0f) && (dofDepth >= dofNear) && (dofDepth <= dofFar);
      clippingParamsValid = (nearClip > 0.0f) && (farClip > 0.0f) && (nearClip < farClip);
   }


   
   BUser * const pUser = gUserManager.getPrimaryUser();
   if(!pUser)
      return;

   if (!pUser->getFlagFreeCamera())
   {
      // Update camera
      switch(mCameraMode)
      {
         case cCameraModeLocked:
            {
               BCamera* camera=pUser->getCamera();

               BVector right(mCurrentCameraMatrix.m[0][0], mCurrentCameraMatrix.m[0][1], mCurrentCameraMatrix.m[0][2]);
               BVector up(mCurrentCameraMatrix.m[1][0], mCurrentCameraMatrix.m[1][1], mCurrentCameraMatrix.m[1][2]);
               BVector dir(mCurrentCameraMatrix.m[2][0], mCurrentCameraMatrix.m[2][1], mCurrentCameraMatrix.m[2][2]);
               BVector loc(mCurrentCameraMatrix.m[3][0], mCurrentCameraMatrix.m[3][1], mCurrentCameraMatrix.m[3][2]);

               right.normalize();
               up.normalize();
               dir.normalize();
               dir.inverse();



               float lerpFactor = 1.0f;      // 1 = animated shot camera, 0 = user ingame camera

               // Check if we need to be lerping
               if(mbStartCameraInterpolate && (mCurrentTime < mStartCameraInterpolateDuration))
               {
                  lerpFactor = (mCurrentTime / mStartCameraInterpolateDuration);
               }
               if(mbEndCameraInterpolate && ((mTotalDuration - mCurrentTime) < mEndCameraInterpolateDuration))
               {
                  lerpFactor = ((mTotalDuration - mCurrentTime) / mEndCameraInterpolateDuration);
               }

               if(lerpFactor < 1.0f)
               {
                  BQuaternion cineCamOrient, userCamOrient;
                  BVector cineCamPos, userCamPos;
                  float cineCamFOV, userCamFOV;

                  BQuaternion newCamOrient;  // result
                  BVector newCamPos;
                  float newCamFOV;


                  BMatrix cineMat;
                  cineMat.makeOrient(dir, up, right);

                  cineCamOrient.set(cineMat);
                  cineCamPos = loc;
                  cineCamFOV = fov;

                  userCamOrient = s_cacheGameCameraOrient;
                  userCamPos = s_cacheGameCameraPos;
                  userCamFOV = s_cacheGameCameraFOV;


                  userCamOrient.slerp(cineCamOrient, lerpFactor, newCamOrient);
                  newCamPos.lerpPosition(lerpFactor, userCamPos, cineCamPos);
                  newCamFOV = userCamFOV + ((cineCamFOV - userCamFOV) * lerpFactor);


                  BMatrix mat;
                  newCamOrient.toMatrix(mat);

                  mat.getRight(right);
                  mat.getForward(dir);
                  mat.getUp(up);
                  loc.set(newCamPos);
                  fov = newCamFOV;
               }



               camera->setCameraRight(right);
               camera->setCameraDir(dir);
               camera->setCameraUp(up);
               camera->setCameraLoc(loc);
               camera->setFOV(fov);
               
               if(mbDOFEnabled)
               {
                  if(dofParamsValid)
                  {
                     BToneMapParams curToneMapParams(gToneMapManager.getParams(0));
                     
                     curToneMapParams.mDOFEnabled = true;
                                       
                     curToneMapParams.mDOFFarBlurPlaneDist = dofFar;
                     curToneMapParams.mDOFFocalPlaneDist = dofDepth;
                     curToneMapParams.mDOFNearBlurPlaneDist = dofNear;
                     curToneMapParams.mDOFMaxBlurriness = 1.0f;
                     
                     gToneMapManager.setParams(curToneMapParams, 0);
                  }
                  else
                  {
                     setDOFParams();
                  }
               }

               if(clippingParamsValid)
               {
                  gRenderControl.setNearFarClipPlanes(nearClip, farClip);
               }
            }
            break;

         case cCameraModeFree:
         {
            BToneMapParams curToneMapParams(gToneMapManager.getParams(0));
            curToneMapParams.mDOFEnabled = false;
            gToneMapManager.setParams(curToneMapParams, 0);
            
            break;
         }            
      }   
   }
   
   // Update transitions
   //
   if(pShot->mpStartTransition && !pShot->mpStartTransition->mStarted && (shotTime <= (pShot->mpStartTransition->mFadeUp + pShot->mpStartTransition->mHold + pShot->mpStartTransition->mFadeDown)))
   {
      BColor color;
      color.r = pShot->mpStartTransition->mColor.x;
      color.g = pShot->mpStartTransition->mColor.y;
      color.b = pShot->mpStartTransition->mColor.z;

      gWorld->getTransitionManager()->doFadeUpHoldFadeDown(pShot->mpStartTransition->mFadeUp, pShot->mpStartTransition->mHold, pShot->mpStartTransition->mFadeDown, color);
      pShot->mpStartTransition->mStarted = true;
   }
   else if(pShot->mpEndTransition && !pShot->mpEndTransition->mStarted && ((pShot->mDuration - shotTime) < (pShot->mpEndTransition->mFadeDown + (pShot->mpEndTransition->mHold * 0.5f))))
   {
      BColor color;
      color.r = pShot->mpEndTransition->mColor.x;
      color.g = pShot->mpEndTransition->mColor.y;
      color.b = pShot->mpEndTransition->mColor.z;

      gWorld->getTransitionManager()->doFadeDownHoldFadeUp(pShot->mpEndTransition->mFadeDown, pShot->mpEndTransition->mHold, pShot->mpEndTransition->mFadeUp, color);
      pShot->mpEndTransition->mStarted = true;
   }
}

//============================================================================
// BCinematic::updateVideo
//============================================================================
void BCinematic::updateVideo(float elapsedTime)
{
   if (mFlagPlayingChat)
   {
      BGeneralEventSubscriber* pSubscription = gWorld->getChatManager()->getChatCompletedEventSubscriber();
      if (pSubscription)
      {
         if (pSubscription->hasFired())
         {
            BChatMessage* pMessage = gWorld->getChatManager()->getChat();
            if (pMessage != NULL)
            {
               gWorld->getChatManager()->removeChat(pMessage);
               gUIManager->setChatVisible(false);
            }
            mFlagPlayingChat=false;
         }
      }
   }

   float lastFrameTime = mCurrentTime;
   mCurrentTime += elapsedTime;

   int mActiveShot = getCurrentShotID();
   if (mActiveShot == -1)
      return;

   BShot *pShot = mShots[mActiveShot];

   // Trigger tags
   for(int i = 0; i < pShot->mTags.getNumber(); i++)
   {
      BShotTag& tag = pShot->mTags[i];
      if (tag.mTagType != cShotTagTypeChat)
         continue;

      float absoluteTagTime = tag.mTime + pShot->mStartTime;

      float p1 = lastFrameTime;
      float p2 = mCurrentTime;

      if((absoluteTagTime<cFloatCompareEpsilon && p1<cFloatCompareEpsilon && p2>=cFloatCompareEpsilon) ||
         (absoluteTagTime>p1 && absoluteTagTime<=p2))
      {
         triggerTag(&tag);
      }
   }      
}

//============================================================================
// BCinematic::setDOFParams
//============================================================================
void BCinematic::setDOFParams()
{
   // Cache tonemapper DOF params 
   BToneMapParams toneMapperParams(s_cachedToneMapperDOFParams);

   // Set new tonemapper DOF params
   toneMapperParams.mDOFEnabled = mbDOFEnabled;

   gToneMapManager.setParams(toneMapperParams, 0);
}

//============================================================================
// BCinematic::restoreDOFParams
//============================================================================
void BCinematic::restoreDOFParams()
{
   gToneMapManager.setParams(s_cachedToneMapperDOFParams, 0);
}

//============================================================================
// BCinematic::play
//============================================================================
void BCinematic::play(BSmallDynamicSimArray<BEntityID>* pPossessSquadList, bool* pPreRendered)
{
   if (pPreRendered)
      *pPreRendered = false;

   // Early out if not loaded
   if(!mFlagIsLoaded)
      return;

   if(mState == cStatePlaying)
      return;

   mState = cStatePlaying;

   bool checkForVideo = true;

   #ifndef BUILD_FINAL
      if (gConfig.isDefined(cConfigWowRecord) || gConfig.isDefined(cConfigWowPlay) || (gModeManager.getModeType() == BModeManager::cModeCinematic))
      {
         checkForVideo = false;

         uint numScripts = gScenario.getNumTriggerScriptIDs();
         for (uint i=0; i<numScripts; i++)
         {
            BTriggerScript* pScript = gTriggerManager.getTriggerScript(gScenario.getTriggerScriptID(i));
            if (pScript)
               pScript->setLogEffects(true);
         }
      }
   #endif

   syncTriggerData("BCinematic::play checkForVideo", checkForVideo);

   mFlagPlayingVideo=false;

   if (checkForVideo)
   {
      mFlagVideoLoadFailed=true;
      mFlagPlayingVideo = true;
      if (pPreRendered)
         *pPreRendered = true;
      if(!gConfig.isDefined(cConfigDisableWow))
      {
         BSimString name;
         strPathGetFilename(mFilename, name);
         strPathRemoveExtension(name);
         syncTriggerData("BCinematic::play name", name);
         BSimString filename;
         //Changing to WTF for precert1 build - eric
         //filename.format("game:\\wtf\\%s.bik", name.getPtr());
         filename.format("game:\\video\\%s.bik", name.getPtr());
         if (_access(filename, 0) == 0)
         {
            BBinkInterface::BLoadParams lp;
            lp.mFilename = filename;
            lp.mCaptionDirID = -1;
            lp.mCaptionFilename.clear();
            lp.mpStatusCallback = this;
            lp.mLoopVideo = false;
            lp.mFullScreen = true;
            lp.mIOBufSize = 4 * 1024 * 1024;
            mVideoHandle = gBinkInterface.loadActiveVideo(lp);
            gBinkInterface.registerValidCallback(this);
            if (mVideoHandle != cInvalidVideoHandle)
            {
               gModeManager.getModeGame()->setFlagPlayingVideo(true);
               gRenderControl.enableWorkerRender(false);
               mFlagVideoLoadFailed=false;
            }
         }
      }
   }

   syncTriggerData("BCinematic::play mFlagPlayingVideo", mFlagPlayingVideo);

   //gSoundManager.overrideBackgroundMusic(true);

   startSound();

   if (!mFlagPlayingVideo)
   {
      sCinematicParticleManager.clear();

      s_cachedToneMapperDOFParams = gToneMapManager.getParams(0);
      
      setDOFParams();
         
      // Set clip planes distances
      mStoredNearClipPlane = gRenderControl.getNearClipPlane();
      mStoredFarClipPlane = gRenderControl.getFarClipPlane();

      // Store posses squads if non-empty
      if(pPossessSquadList)
         mPossessSquadList = *pPossessSquadList;
      else
         mPossessSquadList.clear();

      // Save camera params
      BMatrix camMatrix;

//-- FIXING PREFIX BUG ID 5267
      const BCamera* camera=gUserManager.getUser(BUserManager::cPrimaryUser)->getCamera();
//--
      camMatrix.makeOrient(camera->getCameraDir(), camera->getCameraUp(), camera->getCameraRight());
      camMatrix.setTranslation(camera->getCameraLoc());

      s_cacheGameCameraOrient.set(camMatrix);
      camMatrix.getTranslation(s_cacheGameCameraPos);
      s_cacheGameCameraFOV = camera->getFOV();
   }
}


//============================================================================
// BCinematic::end
//============================================================================
void BCinematic::end()
{
   // Early out if not loaded
   if(!mFlagIsLoaded)
      return;

   if(mState == cStateEnded)
      return;

   mState = cStateEnded;

   #ifndef BUILD_FINAL
      if (gConfig.isDefined(cConfigWowRecord) || gConfig.isDefined(cConfigWowPlay) || (gModeManager.getModeType() == BModeManager::cModeCinematic))
      {
         uint numScripts = gScenario.getNumTriggerScriptIDs();
         for (uint i=0; i<numScripts; i++)
         {
            BTriggerScript* pScript = gTriggerManager.getTriggerScript(gScenario.getTriggerScriptID(i));
            if (pScript)
               pScript->setLogEffects(false);
         }
      }
   #endif

   releaseUnits();

   //gSoundManager.overrideBackgroundMusic(false);

   stopSound();
   sCinematicParticleManager.clear();

   if (!mFlagPlayingVideo)
   {
      // Restore DOF
      restoreDOFParams();

      // Restore clipping planes
      gRenderControl.setNearFarClipPlanes(mStoredNearClipPlane, mStoredFarClipPlane);
   }
}

//============================================================================
// BCinematic::receiveEvent
//============================================================================
#ifdef ENABLE_RELOAD_MANAGER
bool BCinematic::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   if (event.mEventClass == cEventClassReloadNotify)
   {
      // Reload animation
      reload();
   }

   return false;
}
#endif
//==============================================================================
// BCinematic::startSound
//==============================================================================
void BCinematic::startSound()
{
   if(mState != cStatePlaying)
      return;

   //float gameSpeed = 1.0f;
   //gConfig.get(cConfigGameSpeed, &gameSpeed);
   //if(gameSpeed != 1.0f)
   //   return;

   if(mCurrentTime != 0.0f)
      return;

   if(!mSoundFilename.isEmpty())
   {
      mSoundCue = gSoundManager.playCue(mSoundFilename);
   }
}

//==============================================================================
// BCinematic::stopSound
//==============================================================================
void BCinematic::stopSound()
{
   if(mSoundCue != cInvalidCueHandle)
   {
      gSoundManager.playCue("stop_all_cinematic_sound");
      mSoundCue = cInvalidCueHandle;
   }
}

//==============================================================================
// BCinematic::restartSound
//==============================================================================
void BCinematic::restartSound()
{
   stopSound();

   // Need to update the sound mananger here or else the sound will not get restarted.  It has
   // something to do with sending the stop and play cue events on the same frame.
   gSoundManager.update();

   startSound();
}

//============================================================================
// BCinematic::getCurrentShot
//============================================================================
const BShot *BCinematic::getCurrentShot() const
{
   // Early out if not loaded
   if(!mFlagIsLoaded)
      return(NULL);

   if(mActiveShot == -1)
   {
      return(NULL);
   }
   else
   {
      return(mShots[mActiveShot]);
   }
}

//============================================================================
// BCinematic::toggleCameraMode
//============================================================================
void BCinematic::toggleCameraMode()
{
   if(mCameraMode == cCameraModeFree)
      mCameraMode = cCameraModeLocked;
   else
   {
      mCameraMode = cCameraModeFree;

      // restore FOV to default
      BCamera* camera=gUserManager.getUser(BUserManager::cPrimaryUser)->getCamera();
      camera->setFOV(40.0f*cRadiansPerDegree);
   }
}


//============================================================================
// BCinematic::forwardToNextShot
//============================================================================
void BCinematic::forwardToNextShot()
{
   int currentShot = getCurrentShotID();
   if(currentShot == -1)
      return;

   currentShot++;

   int numShots = mShots.getNumber();
   if(currentShot >= numShots)
   {
      currentShot = 0;
   }

   // Clear active shot
   if(mActiveShot != -1)
   {
      clearActiveShot();
   }

   mCurrentTime = mShots[currentShot]->mStartTime;

   if(mState == cStatePlaying)
   {
      // Stop or restart sound 
      if(mCurrentTime != 0.0f)
         stopSound();
      else
         restartSound();
   }

   sCinematicParticleManager.clear();
}

//============================================================================
// BCinematic::rewindToPreviousShot
//============================================================================
void BCinematic::rewindToPreviousShot()
{
   int currentShot = getCurrentShotID();
   if(currentShot == -1)
      return;

   if((mCurrentTime - mShots[currentShot]->mStartTime) < 0.5f)
   {
      currentShot--;
      if(currentShot <= 0)
      {
         currentShot = 0;
      }
   }

   // Clear active shot
   if(mActiveShot != -1)
   {
      clearActiveShot();
   }

   mCurrentTime = mShots[currentShot]->mStartTime;

   if(mState == cStatePlaying)
   {
      // Stop or restart sound 
      if(mCurrentTime != 0.0f)
         stopSound();
      else
         restartSound();
   }
   
   sCinematicParticleManager.clear();
}

//============================================================================
// BCinematic::rewindToStart
//============================================================================
void BCinematic::rewindToStart()
{   
   // Clear active shot
   if(mActiveShot != -1)
   {
      clearActiveShot();
   }
   mCurrentTime = 0.0f;

   if(mState == cStatePlaying)
   {
      // Restart sound
      restartSound();
   }
   
   sCinematicParticleManager.clear();
}

//============================================================================
// BCinematic::hasTriggerTagFired
//============================================================================
bool BCinematic::hasTriggerTagFired(uint tagId)
{   
   if(tagId >= mTriggerTagTimes.getSize())
      return false;

   if(mCurrentTime >= mTriggerTagTimes[tagId])
      return true;
   else
      return false;
}


//============================================================================
// BCinematic::getCurrentShotID
//============================================================================
int BCinematic::getCurrentShotID()
{
   // Compute current shot
   int currentShot = -1;
   for(int i = 0; i < mShots.getNumber(); i++)
   {
      BShot *pShot = mShots[i];
      if((mCurrentTime >= pShot->mStartTime) && (mCurrentTime < pShot->mEndTime))
      {
         currentShot = i;
         break;
      }
   }

   return (currentShot);
}


//============================================================================
// BCinematic::createUnits
//============================================================================
void BCinematic::createUnits()
{
   //-- lock down the physics for writing
   BScopedPhysicsWrite physicsWriteMarker;
   physicsWriteMarker.init(gWorld->getPhysicsWorld());


   long curSquadPossess = 0;
   long squadPossessCount = mPossessSquadList.getSize();

   long curCinModel = 0;
   long cinModelsCount = mModels.getNumber();

   // Possess 
   //
   while((curSquadPossess < squadPossessCount) && (curCinModel < cinModelsCount))
   {
      BSquad *pSquad = gWorld->getSquad(mPossessSquadList.get(curSquadPossess));

      if (pSquad)
      {
         long curChild = 0;
         long childCount = pSquad->getNumberChildren();

         while((curChild < childCount) && (curCinModel < cinModelsCount))
         {
            BUnit *pUnit = gWorld->getUnit(pSquad->getChild(curChild));
            BCinematicModel *pCineModel = mModels[curCinModel];

            // make sure the protoID match
            if((pCineModel->mType == cModelTypeProto) && (pUnit->getProtoID() == pCineModel->mProtoID))
            {
               pCineModel->mIsPossessed = true;
               pCineModel->mpUnit = pUnit;
               pCineModel->mpSquad = pSquad;

               pUnit->setPosition(cOriginVector);
               pUnit->setForward(cZAxisVector);
               pUnit->setRight(cXAxisVector);
               pUnit->calcUp();

               // disable IK
               pUnit->setFlagIKDisabled(true);

               // Enable cinematic control
               pUnit->setFlagIsUnderCinematicControl(true);

               // disable leashing
               pSquad->setFlagIgnoreLeash(true);

               // Deactivate physics
               const BProtoObject* pProtoObject = pUnit->getProtoObject();
               if(pProtoObject)
               {
                  BPhysicsInfo *pInfo = gPhysicsInfoManager.get(pProtoObject->getPhysicsInfoID(), true);
                  if(pInfo && pInfo->isVehicle())
                  {
                     BPhysicsObject* pPO = pUnit->getPhysicsObject();
                     BASSERT(pPO);
                     pPO->forceDeactivate();
                  }
               }

               curChild++;
               curCinModel++;
            }
            else
            {
               break;
            }
         }
      }

      curSquadPossess++;
   }


   // Create non-Possess (proto & gr2) types
   for(int i = curCinModel; i < cinModelsCount; i++)
   {
      BCinematicModel *pCineModel = mModels[i];

      switch(pCineModel->mType)
      {      
         case cModelTypeProto:
            {
               BUnit *pUnit = NULL;
               BSquad *pSquad = NULL;

               BASSERTM((pCineModel->mPlayerID < gWorld->getNumberPlayers()), "Invalid player ID in cinematic model");

               BEntityID entityID = gWorld->createEntity(pCineModel->mProtoID, false, pCineModel->mPlayerID, cOriginVector, cZAxisVector, cXAxisVector, true, false, true);


               pSquad = gWorld->getSquad(entityID);
               if (pSquad && pSquad->getNumberChildren() == 1)
               {
                  pUnit = gWorld->getUnit(pSquad->getLeader());
               }

               if(pUnit && pSquad)
               {
                  pCineModel->mIsPossessed = false;
                  pCineModel->mpUnit = pUnit;
                  pCineModel->mpSquad = pSquad;

                  pUnit->setPosition(cOriginVector);
                  pUnit->setForward(cZAxisVector);
                  pUnit->setRight(cXAxisVector);
                  pUnit->calcUp();

                  // disable IK
                  pUnit->setFlagIKDisabled(true);

                  // Enable cinematic control
                  pUnit->setFlagIsUnderCinematicControl(true);

                  // disable leashing
                  pSquad->setFlagIgnoreLeash(true);

                  // Deactivate physics
                  const BProtoObject* pProtoObject = pUnit->getProtoObject();
                  if(pProtoObject)
                  {
                     BPhysicsInfo *pInfo = gPhysicsInfoManager.get(pProtoObject->getPhysicsInfoID(), true);
                     if(pInfo && pInfo->isVehicle())
                     {
                        BPhysicsObject* pPO = pUnit->getPhysicsObject();
                        BASSERT(pPO);
                        pPO->forceDeactivate();
                     }
                  }
               }
            }
            break;

         case cModelTypeGr2:
            {
               pCineModel->mpGrannyInstance=gGrannyManager.createInstance();
               BASSERT(pCineModel->mpGrannyInstance);

               if(!pCineModel->mpGrannyInstance->init(pCineModel->mGrannyModelID, NULL))
               {
                  BASSERT(0);
               }
            }
            break;
      }
   }


   mFlagAreUnitsCreated = true;
}

//============================================================================
// BCinematic::releaseUnits
//============================================================================
void BCinematic::releaseUnits()
{   
   for(int i = 0; i < mModels.getNumber(); i++)
   {
      BCinematicModel *pCinModel = mModels[i];

      // disable IK
      if(pCinModel->mpUnit)
      {
         pCinModel->mpUnit->setFlagIKDisabled(false);
         pCinModel->mpUnit->setFlagIsUnderCinematicControl(false);
      }

      // disable ignore leashing, disagle cinematic control
      if(pCinModel->mpSquad)
      {
         pCinModel->mpSquad->setFlagIgnoreLeash(false);
      }


      switch(pCinModel->mType)
      {      
         case cModelTypeProto:
            {
               if(pCinModel->mIsPossessed && pCinModel->mpUnit)
               {
                  // Remove play animation action
                  pCinModel->mpUnit->removeAllActionsOfType(BAction::cActionTypeUnitPlayBlockingAnimation);

                  // Set the squad and platoon position
                  if(pCinModel->mpSquad)
                  {
                     BSquad* pSquad=pCinModel->mpSquad;
                     BVector averagePosition = XMVectorZero();
                     long numUnits = pSquad->getNumberChildren();
                     for (long i = 0; i < numUnits; i++)
                     {
                        BUnit *pUnit = gWorld->getUnit(pSquad->getChild(i));
                        if (pUnit)
                        {
                           pUnit->endPlayBlockingAnimation();
                           averagePosition += pUnit->getPosition();
                        }
                     }

                     averagePosition /= float(numUnits);

                     // Set squad position based on average unit position
                     pSquad->setLeashPosition(averagePosition);
                     pSquad->setPosition(averagePosition);        
                     pSquad->tieToGround();

                     BPlatoon* pPlatoon = pSquad->getParentPlatoon();
                     if (pPlatoon && (pPlatoon->getNumberChildren() == 1))
                     {
                        pPlatoon->setPosition(pSquad->getPosition());
                     }
                  }



                  // IK planting -- E3 Hack
                  const BProtoObject *pProto = pCinModel->mpUnit->getProtoObject();
                  BVisual *mpVisual = pCinModel->mpUnit->getVisual();

                  if(pProto && mpVisual)
                  {
                     long numGroundIKNodes = pProto->getNumberGroundIKNodes();
                     long numIKNodes = numGroundIKNodes;
                     long nodeIndex = 0;

                     if (numIKNodes > 0)
                     {
                        // Get model's world matrix
                        BMatrix worldMatrix;
                        pCinModel->mpUnit->getWorldMatrix(worldMatrix);

                        if (numGroundIKNodes > 0)
                        {
                           // Iterate through ground IK nodes and calculate target positions
                           for (long i = 0; i < numGroundIKNodes; i++)
                           {
                              long boneHandle = mpVisual->getIKNodeBoneHandle(nodeIndex);

                              // Get bone position from animation
                              BVector animatedBonePos;
                              if (mpVisual->getBone(boneHandle, &animatedBonePos, NULL, NULL, &worldMatrix, false))
                              {
                                 // Set IK node
                                 mpVisual->setIKNode(nodeIndex, animatedBonePos);
                                 mpVisual->setIKNodeActive(nodeIndex, true);
                                 // Lock node
                                 mpVisual->lockIKNodeToGround(boneHandle, false, 0.0f, 0.0f);
                                 mpVisual->lockIKNodeToGround(boneHandle, true, 0.0f, 0.0f);
                              }
                              else
                              {
                                 mpVisual->setIKNodeActive(nodeIndex, false);
                              }

                              nodeIndex++;
                           }
                        }
                     }
                  }



                  // Activate physics
                  if(pProto)
                  {
                     BPhysicsInfo *pInfo = gPhysicsInfoManager.get(pProto->getPhysicsInfoID(), true);
                     if(pInfo && pInfo->isVehicle())
                     {
                        BPhysicsObject* pPO = pCinModel->mpUnit->getPhysicsObject();
                        BASSERT(pPO);
                        pPO->forceActivate();
                     }
                  }


                  continue;
               }

               if(pCinModel->mpSquad)
               {
                  pCinModel->mpSquad->kill(true);

                  pCinModel->mpUnit = NULL;
                  pCinModel->mpSquad = NULL;
               }
            }
            break;

         case cModelTypeGr2:
            {
               if(pCinModel->mpGrannyInstance)
               {
                  gGrannyManager.releaseInstance(pCinModel->mpGrannyInstance);
                  pCinModel->mpGrannyInstance = NULL;
               }
            }
            break;
      }
   }

   
   mFlagAreUnitsCreated = false;
}



//============================================================================
// BCinematic::triggerTag
//============================================================================
void BCinematic::triggerTag(BShotTag *pTag)
{
   switch(pTag->mTagType)
   {
      case cShotTagTypeParticles:
         {
            BMatrix toMat;
            
            if(pTag->mPosition.x != cMinimumFloat)
            {
               // Use position instead of bone if set
               toMat.makeIdentity();
               toMat.setTranslation(pTag->mPosition);

               BParticleCreateParams params;
               params.mDataHandle = pTag->mData0;
               params.mMatrix = toMat;
               params.mNearLayerEffect = false;
               params.mTintColor = cDWORDWhite;

               BParticleInstance* pParticleInstance = gParticleGateway.createInstance(params);

               sCinematicParticleManager.add(pParticleInstance, pTag->mLifespan);
            }
            else
            {
               // Use tomodel and tobone
               if(pTag->mToModelID != -1)
               {
                  BGrannyInstance* grannyInst = NULL;


                  if(mModels[pTag->mToModelID]->mpUnit && mModels[pTag->mToModelID]->mpUnit->getVisual())
                  {
                     grannyInst = (BGrannyInstance *) mModels[pTag->mToModelID]->mpUnit->getVisual()->mpInstance;
                  }
                  /*
                  else if(mModels[pTag->mToModelID]->mpGrannyInstance)
                  {
                     grannyInst = mModels[pTag->mToModelID]->mpGrannyInstance;
                  }
                  */

                  if(grannyInst)
                  {
                     if(pTag->mToBoneHandle == -1)
                     {
                        // Find bone handles
                        //
                        pTag->mToBoneHandle = grannyInst->getBoneHandle(pTag->mToBoneName);
                     }
                  }

                  if(mModels[pTag->mToModelID]->mpUnit)
                  {
                     BVisual *pVisual = mModels[pTag->mToModelID]->mpUnit->getVisual();

                     if(pVisual)
                     {
                        BMatrix worldMatrix;
                        mModels[pTag->mToModelID]->mpUnit->getWorldMatrix(worldMatrix);

                        DWORD playerColor = gWorld->getPlayerColor(mModels[pTag->mToModelID]->mpUnit->getPlayerID(), BWorld::cPlayerColorContextObjects);

                        pVisual->addAttachment( cVisualAssetParticleSystem,   //visualAssetType
                                                0,                            //animationTrack
                                                1,                            //animType
                                                0.0f,                         //tweenTime
                                                pTag->mData0, pTag->mToBoneHandle, pTag->mLifespan, false, playerColor, worldMatrix);
                     }
                  }
               }
            }

            break;
         }
         break;

      case cShotTagTypeSound:
         {
            //BCueIndex index = gSoundManager.getCueIndex(mName);
            //gWorld->getWorldSoundManager()->addSound(BVector(0.0f, 0.0f, 0.0f), BVector(0.0f, 0.0f, 0.0f), index, false);
            
            gSoundManager.playCue(pTag->mName);
         }
         break;

      case cShotTagTypeChat:
         {
            int stringID = pTag->mStringID;
            int talkingHeadID = -1;

            #ifndef BUILD_FINAL
            if (!gConfig.isDefined(cConfigDisableTalkingHeads))
            #endif
            {
               talkingHeadID = pTag->mTalkingHeadID;
            }

            if (gConfig.isDefined(cConfigWowRecord) || (gModeManager.getModeType() == BModeManager::cModeCinematic))
            {
               talkingHeadID=-1;
               stringID=-1;
            }

            BPlayerIDArray recipients;
            BChatMessage* pChat = gWorld->getChatManager()->addChat(stringID, pTag->mToBoneName, pTag->mBool0, true, talkingHeadID, recipients, pTag->mLifespan);
            if (pChat && pTag->mLifespan == 0.0f)
               pChat->setAutoExpire(true);

            // Reset or create subscriber
            BGeneralEventSubscriber* pSubscription = gWorld->getChatManager()->getChatCompletedEventSubscriber();
            if (pSubscription)
               pSubscription->mFired = false;
            else
            {
               pSubscription = gGeneralEventManager.newSubscriber(BEventDefinitions::cChatCompleted);
               gWorld->getChatManager()->setChatCompletedEventSubscriber(pSubscription);
            }
         }
         break;
   }
}


/*
//============================================================================
// BCinematic::freeAllGrannyControls
//============================================================================
void BCinematic::freeAllGrannyControls()
{
   for(int i = 0; i < mModels.getNumber(); i++)
   {
      BCinematicModel *mCineModel = mModels[i];
      switch(mCineModel->mType)
      {
         case(cModelTypeProto):
            if(mCineModel->mpUnit)
            {
               mCineModel->mpUnit->removeAllActionsOfType(BAction::cActionTypeUnitPlayBlockingAnimation);
               ((BGrannyInstance*)mCineModel->mpUnit->getVisual()->mpInstance)->freeGrannyControls();
            }
            break;

         case(cModelTypeGr2):
            mCineModel->mpGrannyInstance->freeGrannyControls();
            break;
      }
   }
}
*/

//==============================================================================
// BCinematic::onVideoEnded
//==============================================================================
void BCinematic::onVideoEnded(BBinkVideoHandle handle, BByteArray *preloadedData, eBinkStatusCode statusCode)
{
   if (handle == mVideoHandle)
   {
      mFlagPlayingVideo = false;
      mVideoHandle = cInvalidVideoHandle;
      mState = cStateEnded;
      gModeManager.getModeGame()->setFlagPlayingVideo(false);
      gRenderControl.enableWorkerRender(true);
      
      // If for some reason we got preloaded data back (not expected here at the moment), delete it since otherwise
      // it would be orphaned.
      if(preloadedData)
      {
         delete preloadedData;
         preloadedData = NULL;
      }
      
      //gSoundManager.overrideBackgroundMusic(false);
   }
}


//==============================================================================
// BCinematic::clearActiveShot
//==============================================================================
void BCinematic::clearActiveShot()
{
   if(mActiveShot == -1)
      return;

   // Clear previous shot animations
   BShot *pLastShot = mShots[mActiveShot];
   for(int i = 0; i < pLastShot->mAnimatedModels.getNumber(); i++)
   {
      long modelID = pLastShot->mAnimatedModels[i].mModelID;

      BCinematicModel *mCineModel = mModels[modelID];
      switch(mCineModel->mType)
      {
         case(cModelTypeProto):
            if(mCineModel->mpUnit)
            {
               mCineModel->mpUnit->removeAllActionsOfType(BAction::cActionTypeUnitPlayBlockingAnimation);
               if(!mCineModel->mIsPossessed)
               {
                  // Make invisible
                  mCineModel->mpUnit->setFlagNoRender(true);
               }
            }
            break;

         case(cModelTypeGr2):
            break;
      }
   }

   // Clear previous shot lights
   if(mShots[mActiveShot]->mCameraEffectInstance)
   {
      mShots[mActiveShot]->mCameraEffectInstance->clearLights();
   }


   mActiveShot = -1;
}