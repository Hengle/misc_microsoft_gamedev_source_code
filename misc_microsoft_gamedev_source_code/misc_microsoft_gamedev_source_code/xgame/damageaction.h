//============================================================================
// damageaction.h
//
// Copyright (c) 2004-2007 Ensemble Studios
//============================================================================
#pragma once


//============================================================================
// Includes

//============================================================================
//  Forward Declarations
class BUnit;


//============================================================================
// BDamagePart
//============================================================================
class BDamagePart
{
public:
   BDamagePart();
   ~BDamagePart();

   void        reset( void );

   long        mBoneHandle;
   long        mMeshIndex;
   BVector     mMin;
   BVector     mMax;
   BVector     mCenterOffset;
};



//============================================================================
// BDamageAction
//============================================================================
class BDamageAction
{
public:
   BDamageAction();
   virtual ~BDamageAction();

   enum
   {
      cThrowPart,
      cSwapPart,
      cThrowAttachment,
      cAttachParticle,
      cUVOffset,
      cMultiframeTextureIndex,
      cPlaySound,
      cHidePart,
      cNumberTypes,
   };

   long        getType( void ) const          { return mType; }
   
   virtual  bool        load(const BXMLNode &root, long modelIndex) = 0;
   virtual  void        execute(BUnit* pUnit, bool bVisible, const BVector *pModelSpacePoint, float force, bool ballisticHit, const BVector* pOverrideForceDir, BEntityID* outEntityID = NULL ) const = 0;
   virtual  void        doExecuteSilent(BUnit* pUnit) const = 0;
   virtual  void        undoExecuteSilent(BUnit* pUnit) const = 0;

protected:

   long        mType;
};

//============================================================================
// BDamageActionThrowPart
//============================================================================
class BDamageActionThrowPart : public BDamageAction
{
public:
   BDamageActionThrowPart();
   ~BDamageActionThrowPart();

   long       getBPID( void ) const          { return mBPID; }
   long       getReleaseEffectID( void ) const  { return mReleaseEffectID; }
   long       getStreamerEffectID( void ) const { return mStreamerEffectID; }

   void       setBPID(long i)                { mBPID = i; }
   void       setReleaseEffectID(long e)     { mReleaseEffectID = e; }   
   void       setStreamerEffectID(long e)    { mStreamerEffectID = e; }

   const BDamagePart *     getPart( long index ) const;
   long                    getPartCount( void ) const { return mParts.getNumber(); }

   void        reset( void );

   virtual  bool        load(const BXMLNode &root, long modelIndex);
   virtual  void        execute(BUnit* pUnit, bool bVisible, const BVector *pModelSpacePoint, float force, bool ballisticHit, const BVector* pOverrideForceDir, BEntityID* outEntityID = NULL ) const;
   virtual  void        doExecuteSilent(BUnit* pUnit) const;
   virtual  void        undoExecuteSilent(BUnit* pUnit) const;

protected:


   long        mBPID;                  // partthrow
   long        mReleaseEffectID;       // partthrow
   long        mStreamerEffectID;      // partthrow
   int8        mImpactSoundSetID;      // partthrow
   long        mTerrainEffectID;       // partthrow
   float       mForceMultiplier;
   bool        mDisregardForce;

   BDynamicSimArray<BDamagePart*>    mParts;
};


//============================================================================
// BDamageActionSwapPart
//============================================================================
class BDamageActionSwapPart : public BDamageAction
{
public:
   BDamageActionSwapPart();
   ~BDamageActionSwapPart();


   const BDamagePart *     getPart( long index ) const;
   long                    getPartCount( void ) const { return mParts.getNumber(); }

   void        reset( void );

   virtual  bool        load(const BXMLNode &root, long modelIndex);
   virtual  void        execute(BUnit* pUnit, bool bVisible, const BVector *pModelSpacePoint, float force, bool ballisticHit, const BVector* pOverrideForceDir, BEntityID* outEntityID = NULL ) const;
   virtual  void        doExecuteSilent(BUnit* pUnit) const;
   virtual  void        undoExecuteSilent(BUnit* pUnit) const;

protected:

   BDynamicSimArray<BDamagePart*>    mParts;
};

//============================================================================
// BDamageActionHidePart
//============================================================================
class BDamageActionHidePart : public BDamageAction
{
public:
   BDamageActionHidePart();
   ~BDamageActionHidePart();

   void        reset( void );

   virtual  bool        load(const BXMLNode &root, long modelIndex);
   virtual  void        execute(BUnit* pUnit, bool bVisible, const BVector *pModelSpacePoint, float force, bool ballisticHit, const BVector* pOverrideForceDir, BEntityID* outEntityID = NULL ) const;
   virtual  void        doExecuteSilent(BUnit* pUnit) const;
   virtual  void        undoExecuteSilent(BUnit* pUnit) const;

protected:

   BDynamicSimArray<long>    mParts;
   bool                      mAllParts:1;
};

//============================================================================
// BDamageActionThrowAttachment
//============================================================================
class BDamageActionThrowAttachment : public BDamageAction
{
public:
   BDamageActionThrowAttachment();
   ~BDamageActionThrowAttachment();

   long        getBPID( void ) const          { return mBPID; }
   long        getReleaseEffectID( void ) const  { return mReleaseEffectID; }
   long        getStreamerEffectID( void ) const { return mStreamerEffectID; }

   void        setBPID(long i)                { mBPID = i; }
   void        setReleaseEffectID(long e)     { mReleaseEffectID = e; }   
   void        setStreamerEffectID(long e)    { mStreamerEffectID = e; }
   void        setBoneHandle(long bh)         { mBoneHandle = bh; }

   void        reset( void );

   virtual  bool        load(const BXMLNode &root, long modelIndex);
   virtual  void        execute(BUnit* pUnit, bool bVisible, const BVector *pModelSpacePoint, float force, bool ballisticHit, const BVector* pOverrideForceDir, BEntityID* outEntityID = NULL ) const;
   virtual  void        doExecuteSilent(BUnit* pUnit) const;
   virtual  void        undoExecuteSilent(BUnit* pUnit) const;

protected:


   long        mBPID;                  // partthrow
   long        mReleaseEffectID;       // partthrow
   long        mStreamerEffectID;      // partthrow
   int8        mImpactSoundSetID;      // partthrow
   float       mForceMultiplier;
   bool        mDisregardForce;

   long        mBoneHandle;
};


//============================================================================
// BDamageActionAttachParticle
//============================================================================
class BDamageActionAttachParticle : public BDamageAction
{
public:
   BDamageActionAttachParticle();
   ~BDamageActionAttachParticle();

   void        reset( void );

   virtual  bool        load(const BXMLNode &root, long modelIndex);
   virtual  void        execute(BUnit* pUnit, bool bVisible, const BVector *pModelSpacePoint, float force, bool ballisticHit, const BVector* pOverrideForceDir, BEntityID* outEntityID = NULL ) const;
   virtual  void        doExecuteSilent(BUnit* pUnit) const;
   virtual  void        undoExecuteSilent(BUnit* pUnit) const;

protected:

   long        mEffectID;              // particleattach
   float       mLifeSpan;
   long        mBoneHandle;
   BVector     mCenterOffset;
};



/*
//============================================================================
// BDamageActionUVOffset
//============================================================================
class BDamageActionUVOffset : public BDamageAction
{
public:
   BDamageActionUVOffset();
   ~BDamageActionUVOffset();

   void        reset( void );

   virtual  bool        load(const BXMLNode &root, long modelIndex);
   virtual  void        execute(BUnit* pUnit, bool bVisible, const BVector *pModelSpacePoint, float force, bool ballisticHit, const BVector* pOverrideForceDir, BEntityID* outEntityID = NULL ) const;
   virtual  void        doExecuteSilent(BUnit* pUnit) const = 0;
   virtual  void        undoExecuteSilent(BUnit* pUnit) const = 0;

protected:

   uint     mChannel;
   float    mUOffset;
   float    mVOffset;

};
*/




//============================================================================
// BDamageActionMultiframeTextureIndex
//============================================================================
class BDamageActionMultiframeTextureIndex : public BDamageAction
{
public:
   BDamageActionMultiframeTextureIndex();
   ~BDamageActionMultiframeTextureIndex();

   void        reset( void );

   virtual  bool        load(const BXMLNode &root, long modelIndex);
   virtual  void        execute(BUnit* pUnit, bool bVisible, const BVector *pModelSpacePoint, float force, bool ballisticHit, const BVector* pOverrideForceDir, BEntityID* outEntityID = NULL ) const;
   virtual  void        doExecuteSilent(BUnit* pUnit) const;
   virtual  void        undoExecuteSilent(BUnit* pUnit) const;

protected:

   uint     mFromIndex;
   uint     mToIndex;
};


//============================================================================
// BDamageActionPlaySound
//============================================================================
class BDamageActionPlaySound : public BDamageAction
{
public:
   BDamageActionPlaySound();
   ~BDamageActionPlaySound();

   void        reset( void );

   virtual  bool        load(const BXMLNode &root, long modelIndex);
   virtual  void        execute(BUnit* pUnit, bool bVisible, const BVector *pModelSpacePoint, float force, bool ballisticHit, const BVector* pOverrideForceDir, BEntityID* outEntityID = NULL ) const;
   virtual  void        doExecuteSilent(BUnit* pUnit) const;
   virtual  void        undoExecuteSilent(BUnit* pUnit) const;

protected:

   long        mSoundID;
   long        mStopSoundID;
};

