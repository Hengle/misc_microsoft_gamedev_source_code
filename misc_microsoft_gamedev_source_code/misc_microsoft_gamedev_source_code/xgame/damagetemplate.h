//============================================================================
// damagetemplate.h
//
// Copyright (c) 2004-2007 Ensemble Studios
//============================================================================
#pragma once


//============================================================================
// Includes
#include "gamefilemacros.h"

//============================================================================
//  Forward Declarations
class BXMLNode;
class BVisual;
class BDamageAction;
class BDamageTemplate;
class BUnit;
class BStream;

//============================================================================
// BImpactPointTracker
//============================================================================
class BImpactPointTracker
{
   public:
      BImpactPointTracker() : mImpactPointIndex(-1), mNumEventsTriggered(0) {}
      ~BImpactPointTracker() {}

      bool save(BStream* pStream, int saveType) const;
      bool load(BStream* pStream, int saveType);

      long     mImpactPointIndex;
      long     mNumEventsTriggered;
};

//============================================================================
// BDamageTracker
//============================================================================
class BDamageTracker
{
   public:

      BDamageTracker();
      ~BDamageTracker();

      bool     init(long damageTemplateID);
      long     getDamageTemplateID() const { return mDamageTemplateID; }
      bool     isDestroyed(long impactPointIndex) const;

      long     getImpactPointTrackerCount() const { return mImpactPointTrackers.getNumber(); }

      long     impactPointHit(long impactPointIndex);
      /*
      void     fillMeshRenderMask(BBitArray& mask);
      */

      long     getAliveImpactPointCount() const { return mAliveImpactPointCount; }

      long     getAliveEventCountForImpactPoint(long impactPointIndex);

      void     updatePercentageBaseDamage(float hp, const BDamageTemplate* pTemplate, BUnit *pUnit, const BVector* pOverrideForceDir, float force, bool onlyFinalEvent = false);
      void     updatePercentageBaseDamageSilent(float hp, const BDamageTemplate* pTemplate, BUnit *pUnit);

      void     shatterDeath(const BDamageTemplate* pTemplate, BUnit *pUnit, const BVector* pOverrideForceDir, float force);

      void     updateThrownParts(float elapsed);
      void     addThrownPart(BEntityID newPart);
      const BEntityIDArray& getRecentlyThrownParts() const { return mRecentThrownParts; }

      GFDECLAREVERSION();
      bool save(BStream* pStream, int saveType) const;
      bool load(BStream* pStream, int saveType);

   protected:

      void                                      updateModelRenderMaskIfNeeded(float newHP, float currentHP, BUnit *pUnit);

      BImpactPointTracker*                      getImpactPointTracker(long index) const;
      void                                      reset();

      long                                      mDamageTemplateID;
      BDynamicSimArray<BImpactPointTracker*>    mImpactPointTrackers;

      long                                      mAliveImpactPointCount;
      //long                                      mAlivePercentageBasedEventCount;
      float                                     mCurrentHP;

      BEntityIDArray                            mRecentThrownParts;
      float                                     mTimeToClearParts;
};


//============================================================================
// BDamageEvent
//============================================================================
class BDamageEvent
{
public:
   BDamageEvent();
   ~BDamageEvent();

   const BDamageAction *   getAction( long index ) const;
   long                    getActionCount( void ) const { return mActions.getNumber(); }


   void                    reset( void );
   bool                    load(const BXMLNode &root, long modelIndex, bool onlyAdditive = false);

protected:
   BDynamicSimArray<BDamageAction*>    mActions;
};


//============================================================================
// BDamageImpactPoint
//============================================================================
class BDamageImpactPoint
{
public:
   BDamageImpactPoint();
   ~BDamageImpactPoint();


   void                    setIndex(long index)    { mIndex = index; }
   long                    getIndex( void ) const  { return mIndex; }

   void                    setPrereqID( long id ) { mPrereqID = id; }
   long                    getPrereqID( void ) const { return mPrereqID; }

   void                    setReplaceShapeID( long id ) { mReplaceShapeID = id; }
   long                    getReplaceShapeID( void ) const { return mReplaceShapeID; }

   void                    setSplashEffectHandle( BHandle handle ) { mSplashEffectHandle = handle; }
   BHandle                 getSplashEffectHandle( void ) const { return mSplashEffectHandle; }

   bool                    getDisableAfterDestruction( void ) const { return mDisableAfterDestruction; }
   void                    setDisableAfterDestruction( bool flag )  { mDisableAfterDestruction = flag; }

   bool                    getFadeAfterMotionStops( void ) const { return mFadeAfterMotionStops; }
   void                    setFadeAfterMotionStops( bool flag )  { mFadeAfterMotionStops = flag; }

   float                   getMotionStopFadeTime( void ) const { return mMotionStopFadeTime; }
   void                    setMotionStopFadeTime( float v )  { mMotionStopFadeTime = v; }

   float                   getMotionStopFadeDelay( void ) const { return mMotionStopFadeDelay; }
   void                    setMotionStopFadeDelay( float v )  { mMotionStopFadeDelay = v; }

   float                   getImpactEffectOffset( void ) const { return mImpactEffectOffset; }
   void                    setImpactEffectOffset( float v )  { mImpactEffectOffset = v; }

   long                    addEvent( BDamageEvent *pEvent);
   const BDamageEvent*     getEvent( long index ) const;
   long                    getEventCount( void ) const { return mEvents.getNumber(); }
   const BDamageEvent*     getFinalThrowPartEvent() const { return getEvent(mFinalThrowPartEventIndex); }

   const BString&          getPartImpactSoundSet() const { return mPartImpactSoundSet; }
   const BString&          getHeavyPartImpactSoundSet() const { return mHeavyPartImpactSoundSet; }

   void                    reset( void );
   bool                    load(const BXMLNode &root, long modelIndex);


protected:


   bool                             mDisableAfterDestruction;
   bool                             mFadeAfterMotionStops;
   float                            mMotionStopFadeTime;
   float                            mMotionStopFadeDelay;
   float                            mImpactEffectOffset;
   long                             mIndex;
   long                             mPrereqID;
   long                             mReplaceShapeID;
   BHandle                          mSplashEffectHandle;
   BDynamicSimArray<BDamageEvent*>  mEvents;
   BString                          mPartImpactSoundSet;
   BString                          mHeavyPartImpactSoundSet;
   long                             mFinalThrowPartEventIndex;
};

//============================================================================
// BDamageTemplate
//============================================================================
class BDamageTemplate
#ifdef ENABLE_RELOAD_MANAGER
   : public BEventReceiver
#endif
{
public:
   BDamageTemplate();
   ~BDamageTemplate();

   // Data used to demand load
   bool                          isLoaded() const                          { return mIsLoaded; }
   const BString&                getFileName() const                       { return mFileName; }
   void                          setFileName(const BString &fileName)      { mFileName = fileName; }
   long                          getModelIndex() const                     { return mModelIndex; }
   void                          setModelIndex(long modelIndex)            { mModelIndex = modelIndex; }

   const BDamageImpactPoint*     getImpactPoint( long impactIndex ) const;
   const BDamageImpactPoint*     getImpactPointByIndex( long index ) const;
   long                          getImpactPointCount( void ) const { return mImpactPoints.getNumber(); }

   long                          chooseClosestImpactPoint( const BDamageTracker* pTracker, BVisual* pVisual, BVector& impactPosModelSpace, bool includeDestroyed = false ) const;
   
   const BDamageEvent*           getPercentageBasedEvent( long index ) const;
   long                          getPercentageBasedEventCount( void ) const { return mPercentageBasedEvents.getNumber(); }

   const BDamageEvent*           getCryoPercentageBasedEvent( long index ) const;
   long                          getCryoPercentageBasedEventCount( void ) const { return mCryoPercentageBasedEvents.getNumber(); }

   const BDamageEvent*           getShatterDeathEvent() const { return mShatterDeathEvent; }

   void                          setManagerIndex(long index) { mManagerIndex = index; }
   long                          getManagerIndex(void) const { return mManagerIndex; }

   void                          reset( void );

   bool                          load();   
   bool                          reload();

protected:
#ifdef ENABLE_RELOAD_MANAGER
   virtual bool                  receiveEvent(const BEvent& event, BThreadIndex threadIndex);
#endif
   bool                          load(const BXMLNode &root, long modelIndex);
   bool                          loadImpactPoints(const BXMLNode &rootNode, long modelIndex, BDynamicSimArray<BDamageImpactPoint*> &impactPoints);

   BDynamicSimArray<BDamageImpactPoint*>     mImpactPoints;
   BDynamicSimArray<BDamageEvent*>           mPercentageBasedEvents;
   BDynamicSimArray<BDamageEvent*>           mCryoPercentageBasedEvents;
   BDamageEvent*                             mShatterDeathEvent;

	long                                      mManagerIndex;

   BString                                   mFileName;        // Used during loading
   long                                      mModelIndex;      // Used during loading
   bool                                      mIsLoaded;

   static BDynamicBYTEArray                   mScratchPad;
};

