//============================================================================
// UIMPSetupScreen.h
// Ensemble Studios (c) 2008
//============================================================================

#pragma once 

class BUIPlayer
{
public:
   enum
   {
      cEmpty,
      cNonLocalHuman,
      cAI,
      cMatchmaking,
      cLocalHuman,
   };

   BUIPlayer();

   bool  mActive;             // active - available for player,
   int8  mSlot;               // the visible slot this player is mapped to.
   int8  mSlotType;           // See enum above
   BUString mGamerTag;
   BSimString mGamerPic;      
   long  mLeaderStringIDIndex; // 
   uint16 mRank;               // rank, see userprofilemanager.h for BPRofileRank and how the data is bitpacked
   int8  mLeader;
   int8  mCiv;
   uint8 mVoice;              // voice state
   uint8 mPing;               // QOS indicator
   int8  mTeam;               // team the player is on
   int8  mControllerPort;     // controller that this player is tied to.
   int8  mID;
   bool  mInCenter:1;         // indicates that this player is in the center
   bool  mHost:1;             // host / not host
   bool  mReady:1;            // player is ready
   bool  mArrowLeft:1;        
   bool  mArrowRight:1;       
};

