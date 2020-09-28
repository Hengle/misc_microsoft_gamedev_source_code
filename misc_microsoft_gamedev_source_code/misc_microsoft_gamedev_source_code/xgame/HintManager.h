//==============================================================================
// HintManager.h
//
// HintManager manages all hints
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

// Includes
#include "simtypes.h"
#include "containers\freelist.h"
#include "gamefilemacros.h"

#pragma once

//==============================================================================
// BHintMessage
//==============================================================================
class BHintMessage
{
public:
   BHintMessage() {}
   BHintMessage(long stringID, bool allPlayers, const BPlayerIDArray& recipientIDs, float duration);
   ~BHintMessage() {}

   void resetDisplayTime(float duration) { mTimeToDisplay = duration; }
   void updateTime(float elapsedTime);
   bool hasExpired() { if(mbNeverExpire) return false; return (mTimeToDisplay <= 0.0f); }
   void expire() { mTimeToDisplay = 0.0f; }

   bool getIsNew() const { return (mbIsNew); }
   bool getAllPlayers() const { return (mbAllPlayers); }

   void setIsNew(bool v) { mbIsNew = v; }

   // hint string
   bool                 hasHintString() { return (mStringID >= 0); }
   const BUString&      getHintString();
   int                  getHintStringID() { return mStringID; }

   bool save(BStream* pStream, int saveType) const;
   bool load(BStream* pStream, int saveType);

protected:
   BPlayerIDArray       mRecipientIDs;
   long                 mStringID;
   float                mTimeToDisplay;
   long                 mStringIDIndex;

   bool                 mbIsNew        : 1;
   bool                 mbAllPlayers   : 1;
   bool                 mbNeverExpire  : 1;
};


//==========================================
// BHintManager
//
// Management and access to game hints
//==========================================
class BHintManager
{
   public:

      BHintManager( void );
      virtual ~BHintManager( void );      

      // Management functions 
      bool init( void );
      void reset( void );

      // hint queue manipulation
      void addHint(long stringID, bool allPlayers, const BPlayerIDArray& recipientIDs, float duration);
      void removeHint( BHintMessage* hintMessage);
      BHintMessage* getHint();

      GFDECLAREVERSION();
      bool save(BStream* pStream, int saveType) const;
      bool load(BStream* pStream, int saveType);

protected:
      BDynamicSimArray<BHintMessage*>   mHintMessageList;
};