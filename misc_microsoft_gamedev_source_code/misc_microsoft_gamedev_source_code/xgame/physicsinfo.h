//==============================================================================
// physicsinfo.h
//
// Copyright (c) 2004-2007 Ensemble Studios
//==============================================================================
#pragma once 


//==============================================================================
// Includes

//==============================================================================
// Forward declarations

//==============================================================================
// Const declarations


//==============================================================================
class BPhysicsVehicleInfo
{
   public:
      BPhysicsVehicleInfo();
      ~BPhysicsVehicleInfo();

      bool  load(BXMLNode& node);

      BVectorArray mHardpoints;
      BVectorArray mTreadHardpoints;
      BDynamicSimFloatArray mTreadHardpointZOffsets;
      float mSpringLength;
      float mWheelRadius;
      float mSpringStrength;
      float mDampingCompression;
      float mDampingRelaxation;
      float mNormalClippingAngle;
      float mChassisUnitInertiaYaw;
      float mChassisUnitInertiaRoll;
      float mChassisUnitInertiaPitch;
      float mTreadHardPointHeight;
      float mTreadRayLength;
      float mTreadVelocityScalar;
      bool  mTreadWheelAttachments:1;
};

//==============================================================================
class BPhysicsInfo
{
   public:

                              BPhysicsInfo(void);
                              ~BPhysicsInfo();
      
      // Filename.
      void                    setFilename(const BCHAR_T *filename) {mFilename=filename;}
      const BSimString           &getFilename(void) const {return(mFilename);}

      // Load/unload.
      bool                    load(long dirID);
      void                    unload(void);

      void                    loadBluePrints();

      // Make sure this order matches the order they are loaded and placed in mBlueprintIDs
      enum eClamshellBodyIndex
      {
         cUpper,
         cLower,
         cPelvis
      };

      enum eVehicleType
      {
         cNone,
         cWarthog,
         cGhost,
         cScorpion,
         cHawk,
         cHornet,
         cVulture,
         cBanshee,
         cVampire,
         cSentinel,
         cCobra,
         cWolverine,
         cGrizzly,
         cElephant,
         cGround,
         cGremlin,
         cChopper,
         cRhino,
         cReactor
      };

      enum eMotionType
      {
         cBoxInertia,
         cFixed
      };

      long                    getBlueprintID(uint32 index) const { return (index >= (uint32) mBlueprintIDs.getNumber()) ? -1 : mBlueprintIDs[index]; }
      void                    setBlueprintID(uint32 index, uint32 blueprintID) { mBlueprintIDs.setAt(index, blueprintID); }
      
      bool                    isThrownByProjectiles(void) const {return(mThrownByProjectiles);}
      void                    setThrownByProjectiles(bool flag) { mThrownByProjectiles = flag; }

      bool                    isDestroyedByProjectiles(void) const {return(mDestroyedByProjectiles);}
      void                    setDestroyedByProjectiles(bool flag) { mDestroyedByProjectiles = flag; }

      bool                    isVehicle(void) const {return(mVehicleType != cNone);}
      eVehicleType            getVehicleType() const { return mVehicleType; }
      bool                    isAircraft() const;

      bool                    isFixed(void) const {return(mMotionType == cFixed);}
      eMotionType             getMotionType() const { return mMotionType; }

      bool                    getPhysicsOnDeath(void) const {return(mPhysicsOnDeath);}
      void                    setPhysicsOnDeath(bool flag) { mPhysicsOnDeath = flag; }


      long                    getTerrainEffectsHandle() const {return mTerrainEffectHandle;};
      // Clamshell data accessors
      bool                    isClamshell() const { return mClamshell; }
      const BVector           getCenterOffset() const { return mCenterOffset; }
      void                    setCenterOffset(const BVector centerOffset) { mCenterOffset = centerOffset; }
      const float             getUpperHeightOffset() const { return mUpperHeightOffset; }
      const float             getLowerHeightOffset() const { return mLowerHeightOffset; }

      void                    setSplashEffectHandle( BHandle handle ) { mSplashEffectHandle = handle; }
      BHandle                 getSplashEffectHandle( void ) const     { return mSplashEffectHandle;   }
      
      float                   getImpactMinVelocity() const { return mImpactMinVelocity; }

      long                    getID() const {return mID; }
      void                    setID(long id) { mID = id; }   

      bool                    isPhantom() const { return mPhantom; }

      BPhysicsVehicleInfo*    getVehicleInfo() const { return mpVehicleInfo; }

protected:
      void                    reset(void);
   
      BVector                 mCenterOffset;

      BDynamicSimLongArray    mBlueprintIDs;
      BPhysicsVehicleInfo     *mpVehicleInfo;

      float                   mUpperHeightOffset;
      float                   mLowerHeightOffset;

      long                    mID;

      // Collision Info
      float                   mImpactMinVelocity;                
      long                    mTerrainEffectHandle;

      eVehicleType            mVehicleType;

      eMotionType             mMotionType;

      // Demand-load related
      BSimString              mFilename;
      bool                    mLoaded : 1;
      bool                    mFailedToLoad : 1;
                 
      bool                    mThrownByProjectiles : 1;
      bool                    mDestroyedByProjectiles : 1;
      bool                    mPhysicsOnDeath : 1;
      bool                    mClamshell : 1;      
      bool                    mPhantom : 1;

      BHandle                 mSplashEffectHandle;
};
