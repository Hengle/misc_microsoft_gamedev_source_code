//==============================================================================
// cinematic.h
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================
#pragma once

#include "xmlreader.h"
#include "boundingBox.h"
#include "tonemapManager.h"
#include "quaternion.h"
#include "binkvideo.h"


class BGrannyInstance;
class BCameraEffectInstance;
class BUnit;
class BSquad;
/*
class BVisual;
*/


//============================================================================
// BModelAnimationCombo
//============================================================================
class BModelAnimationCombo
{
public:
   long              mModelID;
   long              mAnimationID;
};



// Tag types
enum
{
   cShotTagTypeParticles,
   cShotTagTypeSound,
   cShotTagTypeTrigger,
   cShotTagTypeChat,
};


//============================================================================
// BShotTag
//============================================================================
class BShotTag
{
public:

   BShotTag();
   ~BShotTag();


   long        mTagType;
   BSimString  mName;
   float       mTime;
   float       mLifespan;
   BVector     mPosition;
   long        mToModelID;
   BSimString  mToBoneName;
   long        mToBoneHandle;
   BSimString  mTalkingHeadName;
   int         mTalkingHeadID;
   long        mStringID;
   long        mData0;
   bool        mBool0;
};


//============================================================================
// BShotTransition
//============================================================================
class BShotTransition
{
public:
   BShotTransition();
   ~BShotTransition();

   bool  mStarted;
   float mFadeUp;
   float mHold;
   float mFadeDown;
   BVector mColor;

   /*
   long mType;
   */
};


//============================================================================
// BShot
//============================================================================
class BShot
{
public:
   BShot();
   ~BShot();

   BSimString     mName;
   BCameraEffectInstance* mCameraEffectInstance;

   float          mDuration;

   float          mStartTime;
   float          mEndTime;

   BSmallDynamicSimArray<BModelAnimationCombo>  mAnimatedModels;
   BSmallDynamicSimArray<BShotTag>              mTags;

   BShotTransition*                             mpStartTransition;
   BShotTransition*                             mpEndTransition;
};


//============================================================================
// BCinematicModel
//============================================================================

// Model types
enum
{
   cModelTypeProto,
   cModelTypeGr2
};

// Start/End mode 
enum
{
   cModeStart,
   cModeEnd
};


class BCinematicModel
{
public:   
   BCinematicModel();
   ~BCinematicModel();

   BSimString        mName;
   BBoundingBox      mBox;
   
   long              mType;
   long              mProtoID;
   long              mGrannyModelID;
   long              mPlayerID;

   BUnit*            mpUnit;
   BSquad*           mpSquad;

   BGrannyInstance*  mpGrannyInstance;
   BMatrix           mWorldMatrix;

   bool              mIsPossessed;
};



//============================================================================
// BCinematic
//============================================================================
class BCinematic : public BBinkVideoStatus
#ifdef ENABLE_RELOAD_MANAGER
   , public BEventReceiver
#endif
{
public:

   enum
   {
      cStatePlaying,
      cStateEnded
   };

   enum
   {
      cCameraModeLocked,
      cCameraModeFree
   };


   BCinematic();
   ~BCinematic();

   bool                    init(long dirID, const BCHAR_T* pFileName);
   void                    deinit();

   bool                    load(BSimString *errorMsgs = NULL);
   bool                    reload();

   const BSimString&       getFilename() const { return mFilename; }
   void                    setFilename(const BCHAR_T* pFileName) { mFilename=pFileName; }

   const BSimString&       getScenarioName() const { return mScenarioFilename; }
   void                    setScenarioName(const BCHAR_T* pFileName) { mScenarioFilename=pFileName; }

   void                    render();
   void                    postRender();
   void                    update(float deltaTime);

/*
   void                    renderOverlay();
   void                    workerRenderTransitions(void* pData);   
*/

   void                    play(BSmallDynamicSimArray<BEntityID>* pPossessSquadList = NULL, bool* pPreRendered = NULL);
   void                    end();

   float                   getDuration() const { return(mTotalDuration); }
   float                   getCurrentTime() const { return(mCurrentTime); }

   long                    getState() const { return(mState); }
   long                    getCameraMode() const { return(mCameraMode); }

   void                    toggleCameraMode();

   const BShot*            getCurrentShot() const;

   void                    forwardToNextShot();
   void                    rewindToPreviousShot();

   void                    rewindToStart();

   bool                    hasTriggerTagFired(uint tagId);

   BBinkVideoHandle        getVideoHandle() const { return mVideoHandle; }

   virtual void            onVideoEnded(BBinkVideoHandle handle, BByteArray *preloadedData, eBinkStatusCode statusCode);

private:
   void                    updateVideo(float elapsedTime);

   void                    clearFlags();

   int                     getCurrentShotID();

   void                    createUnits();
   void                    releaseUnits();

   void                    startSound();
   void                    stopSound();
   void                    restartSound();

   bool                    readHead(BXMLNode pBodyNode, BSimString *errorMsgs);
   bool                    readBody(BXMLNode pBodyNode, BSimString *errorMsgs);
   bool                    readTag(BXMLNode pBodyNode, BShotTag *pTag, BSimString *errorMsgs);
   long                    findModel(BSimString &modelName);

#ifdef ENABLE_RELOAD_MANAGER
   virtual bool            receiveEvent(const BEvent& event, BThreadIndex threadIndex);
#endif

   void                    triggerTag(BShotTag *pTag);
   
   void                    setDOFParams();
   void                    restoreDOFParams();

   void                    clearActiveShot();


   long                    mState;
   long                    mCameraMode;

   BSimString              mFilename;

   float                   mTotalDuration;
   float                   mCurrentTime;
   XMMATRIX                mCurrentCameraMatrix;

   BSimString              mScenarioFilename;
   BSimString              mSoundFilename;            // WWISE sound bank name
   BCueHandle              mSoundCue;


   BSmallDynamicSimArray<BCinematicModel*>   mModels;
   BSmallDynamicSimArray<BShot*>             mShots;

   BSmallDynamicSimArray<float>              mTriggerTagTimes;

   BSmallDynamicSimArray<BEntityID>          mPossessSquadList;

   int                     mActiveShot;

   BBinkVideoHandle        mVideoHandle;

   float                   mStartCameraInterpolateDuration;
   float                   mEndCameraInterpolateDuration;

   bool                    mbStartCameraInterpolate;
   bool                    mbEndCameraInterpolate;

   bool                    mbIsScenarioStart;

   bool                    mbDOFEnabled;

   static BToneMapParams   s_cachedToneMapperDOFParams;

   static BQuaternion      s_cacheGameCameraOrient;
   static BVector          s_cacheGameCameraPos;
   static float            s_cacheGameCameraFOV;

   float                   mStoredNearClipPlane;
   float                   mStoredFarClipPlane;


   BSimString              *mLoadErrorsMsgPtr;


   bool                   mFlagIsLoaded:1;                     // 1 byte   (1/8)
   bool                   mFlagAreUnitsCreated:1;              //          (2/8)
   bool                   mFlagPlayingVideo:1;
   bool                   mFlagVideoLoadFailed:1;
   bool                   mFlagPlayingChat:1;
};
