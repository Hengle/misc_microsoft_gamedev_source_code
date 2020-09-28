//==============================================================================
// userachievement.h
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

#pragma once

#include "achievementmanager.h"

// xcore
//#include "containers\hashTable.h"

// Forward Declarations
class BUser;
class BDynamicStream;
class BInflateStream;

//==============================================================================
// BUserAchievement
//==============================================================================
class BUserAchievement
{
   public:
      BUserAchievement();
      virtual ~BUserAchievement();

      // methods
      bool isType(int type) const { return (type==mType); }
      int getType() const { return mType; }
      void setType(int type) { mType = (AchievementType)type; }

      bool isGranted() const { return mGranted; }
      void setGranted(uint32 gametime) { mGranted = true; mGrantGameTime = gametime; }
      uint32 getGrantGameTime() {return mGrantGameTime;}

      virtual bool writeAchievementToStream(BDynamicStream &stream);
      virtual bool readAchievementFromStream(BInflateStream* pInfStream);


   protected:
      // common base class fields
      AchievementType mType;
      bool mGranted;
      uint32 mGrantGameTime;
};

//==============================================================================
// BUserAccumulatorAchievement
//==============================================================================
class BUserAccumulatorAchievement : public BUserAchievement
{

public:
   BUserAccumulatorAchievement();
   ~BUserAccumulatorAchievement();

   virtual bool writeAchievementToStream(BDynamicStream &stream);
   virtual bool readAchievementFromStream(BInflateStream* pInfStream);

   void addQuantity(int quantity) { mQuantity += quantity; }
   int getQuantity() const { return mQuantity; }
   void setQuantity(int quantity) { mQuantity = quantity; }

protected:
   int     mQuantity;            
};


//==============================================================================
// BUserCampaignAchievement
//==============================================================================
class BUserCampaignAchievement : public BUserAchievement
{

public:
   BUserCampaignAchievement();
   ~BUserCampaignAchievement();

   virtual bool writeAchievementToStream(BDynamicStream &stream);
   virtual bool readAchievementFromStream(BInflateStream* pInfStream);

   void addQuantity(int quantity) { mQuantity += quantity; }
   int getQuantity() const { return mQuantity; }
   void setQuantity(int quantity) { mQuantity = quantity; }

protected:
   int         mQuantity;
};

//==============================================================================
// BUserSkirmishAchievement
//==============================================================================
class BUserSkirmishAchievement : public BUserAchievement
{

public:
   BUserSkirmishAchievement();
   ~BUserSkirmishAchievement();

   virtual bool writeAchievementToStream(BDynamicStream &stream);
   virtual bool readAchievementFromStream(BInflateStream* pInfStream);

   bool getMapCompleted(int i) const;
   void setMapCompleted(int i);
   bool getLeaderCompleted(int i) const;
   void setLeaderCompleted(int i);
   bool getModeCompleted(int i) const;
   void setModeCompleted(int i);

protected:
   uint16 mMapsCompleted;
   uint8  mLeadersCompleted;
   uint8  mModesCompleted;
};

//==============================================================================
// BUserTriggerAchievement
//==============================================================================
class BUserTriggerAchievement : public BUserAchievement
{

public:
   BUserTriggerAchievement();
   ~BUserTriggerAchievement();

   virtual bool writeAchievementToStream(BDynamicStream &stream);
   virtual bool readAchievementFromStream(BInflateStream* pInfStream);

protected:
};



//==============================================================================
// BUserMapAchievement
//==============================================================================
class BUserMapAchievement : public BUserAchievement
{

public:
   BUserMapAchievement();
   ~BUserMapAchievement();

   virtual bool writeAchievementToStream(BDynamicStream &stream);
   virtual bool readAchievementFromStream(BInflateStream* pInfStream);

protected:
   // these two arrays must correspond to eachother 1:1
   BDynamicSimArray<BString> mMaps;
   BDynamicSimArray<int> mQuantity;
};

//==============================================================================
// BUserAchievementList
//==============================================================================
class BUserAchievementList
{
public:
   BUserAchievementList();
   ~BUserAchievementList();

   void initialize();
   bool readFromStream( BInflateStream* pInfStream );
   bool writeToStream( BDynamicStream &stream );

   BUserAchievement * getAchievement(uint i);
   const BUserAchievement * getAchievement(uint i) const;

protected:
   void setNextState(uint newState);
   BDynamicSimArray<BUserAchievement *>   mAchievements;
   uint32 mVersion;
   
   void clearAchievments();
};
