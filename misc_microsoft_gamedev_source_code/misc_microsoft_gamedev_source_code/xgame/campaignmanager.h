//==============================================================================
// campaignmanager.h
//
// Copyright (c) 2003-2007, Ensemble Studios
//==============================================================================

#pragma once

//==============================================================================
//Includes
#include "bitarray.h"

//==============================================================================
//Forward declarations.
class BXMLDocument;
class BXMLNode;


//==============================================================================
class BCampaignNode
{
   public:
      enum
      {
         cVisible=0,
         cCinematic,
         cPostgame,
         cEndOfCampaign,            // this should be put on the blur cinematic after the last scenario (if there is one)
         cTutorial,
         cFlagLegendary,
         cNoInitialWow,
         cNumberFlags
      };

      enum 
      {
         cWorldHarvest = 0,
         cWorldArcadia,
         cWorldShieldExt,
         cWorldShieldInt
      };

      BCampaignNode( void );
      ~BCampaignNode( void );

      //ID.
      long                    getID( void ) const { return(mID); }
      void                    setID( long id ) { mID=id; }
      long                    getLeaderboardLevelIndex( void ) const {return (mLeaderboardIndex); }
      long                    getPresenceMovieIndex( void ) const {return (mPresenceMovieIndex); }

      const BSimString&       getName() const { return mName; }
      void                    setName(const BString& v) { mName = v; }

      const BSimString&       getVideoNodeName() const { return mVideoNodeName; }

      const BSimString&       getCaptionsFile() const { return mCaptions; }

      //Filename.
      const BSimString&       getFilename( void ) const { return(mFilename); }
      void                    setFilename( const BString &v ) { mFilename=v; }

      //DisplayName.
      const BUString&         getDisplayName( void );
      long                    getDisplayNameStringID( void ) const { return(mDisplayNameStringID); }
      void                    setDisplayNameStringID( long v );

      const BUString&         getLoadText( void );
      const BSimString&       getLoadImage() const { return mLoadImage; }
      long                    getLoadTextID() const { return mLoadTextID; }
      long                    getLoadMode() const { return mLoadMode; }

      // pregame UI images
      const BSimString&       getImageIntro() const { return mImageIntro; }
      const BSimString&       getImageMinimap() const { return mImageMinimap; }
      const BSimString&       getImageEndgame() const { return mImageEndgame; }

      // pregame UI text
      const BUString&         getIntroText( void );

      const BUString&         getCompletedText( void );

      const long              getMusicWorld( void ) { return mMusicWorld; }
      const long              getAmbientWorld( void ) { return mAmbientWorld; }

      //Flags.
      bool                    getFlag( long n ) const { if (mFlags.isBitSet(n) > (DWORD)0) return(true); return(false); }
      void                    setFlag( long n, bool v ) { if (v == true) mFlags.setBit(n); else mFlags.clearBit(n); }

      //XML read and write.
      bool                    readXML( BXMLNode& node, const BSimString &filename );

   protected:
      void                    setDefaultFlags( void );
      BSimString              mName;

      BSimString              mFilename;
      BSimString              mLoadImage;
      BSimString              mVideoNodeName;
      BSimString              mImageIntro;
      BSimString              mImageMinimap;
      BSimString              mImageEndgame;
      BSimString              mCaptions;
      BBitArray               mFlags;
      long                    mID;
      long                    mDisplayNameStringID;
      long                    mRolloverStringID;
      long                    mLoadTextID;
      long                    mLoadMode;
      long                    mLeaderboardIndex;
      long                    mPresenceMovieIndex;

      // Text indexing for strings
      long                    mIntroTextStringIndex;
      long                    mCompletedTextStringIndex;

      // for speedier lookups
      long                    mDisplayNameStringIndex;
      long                    mRolloverStringIndex;
      long                    mLoadTextStringIndex;

      long                    mMusicWorld;
      long                    mAmbientWorld;
};


//==============================================================================
// In order to handle the tutorials, I added the "tutorialStyle" flag to the
// campaigns.  What this says, basically, is that the first campaign node in 
// the campaign will be launched automatically, you will automatically progress
// from campaign to campaign without going to the homecity or anywhere else,
// and when you quit, or reach the last campaign, you will go back to the main menu.
class BCampaign
{
   public:
      enum
      {
         cTutorialStyle=0,
         cUnusedFlag1,
         cUnusedFlag2,
         cUnusedFlag3,
         cUnusedFlag4,
         cUnusedFlag5,
         cUnusedFlag6,
         cUnusedFlag7,
         cNumberFlags
      };

      BCampaign( void );
      ~BCampaign( void );

      //ID.
      long                    getID( void ) const { return(mID); }
      void                    setID( long id ) { mID=id; }

      //Filename.
      const BString&          getFilename( void ) const { return(mFilename); }
      void                    setFilename( const BString &v ) { mFilename=v; }

      //DisplayName.
      const BUString&         getDisplayName( void );
      long                    getDisplayNameStringID( void ) const { return(mDisplayNameStringID); }
      void                    setDisplayNameStringID( long v );

      //Rollover.
      const BUString&         getRollover( void );
      long                    getRolloverStringID( void ) const { return(mRolloverStringID); }
      void                    setRolloverStringID( long v );

      // reset the campaign (all progress lost)
      void                    resetCurrentNode() { mCurrentNodeID = 0;}

      void                    resetCampaign();
      
      //Nodes.
      long                    getNumberNodes( void ) const { return(mNodes.getNumber()); }
      BCampaignNode*          getNode( long id );
      BCampaignNode*          getNode( const char * mapName );
      BCampaignNode*          getPreviousNode( BCampaignNode* pNode);
      BCampaignNode*          getVideoNode(BCampaignNode* pNode);

      //Current node.
      long                    getCurrentNodeID( void ) const { return(mCurrentNodeID); }
      bool                    setCurrentNodeID(long v);
      bool                    incrementCurrentNode( void );
      bool                    decrementCurrentNode( void );

      void                    setLastPlayedMode(uint8 lastPlayedMode);
      // saving to profile 
      void                    saveToCurrentProfile();

      // Mission session counter.
      void                    incrementSessionID();
      void                    setSessionID(uint16 val);
      uint16                  getSessionID();

      // Farthest node
/*
      long                    getFarthestNodeID() const { return mFarthestNodeID; }
      void                    setFarthestNodeID(long v) { mFarthestNodeID = v;}
*/

      //Flags.
      bool                    getFlag( long n ) const { if (mFlags.isBitSet(n) > (DWORD)0) return(true); return(false); }
      void                    setFlag( long n, bool v ) { if (v == true) mFlags.setBit(n); else mFlags.clearBit(n); }

      //XML.
      bool                    readXML( const BString &filename );

      bool                    getPlayContinuous() { return mPlayContinuous; }
      void                    setPlayContinuous(bool doPlayContinuous) { mPlayContinuous = doPlayContinuous; }


      long                    getCurrentDifficulty() { return mCurrentDifficulty; }
      void                    setCurrentDifficulty(long d) { mCurrentDifficulty = d; }

      void                    launchGame(bool skipCinematics, bool useSaveIfAvailable=false);
      void                    setGameSettings(bool skipCinematics, bool useSaveIfAvailable);

      long                    getNodeIDByName(BString &name);
      long                    getNodeIDByFilename(BString &filename);

   protected:
      BDynamicArray<BCampaignNode*> mNodes;
      BString                 mFilename;
      BBitArray               mFlags;
      long                    mID;
      long                    mDisplayNameStringID;
      long                    mRolloverStringID;
      long                    mCurrentNodeID;
      // long                    mFarthestNodeID;           // The highest node the player has advanced to so far.
      long                    mCurrentDifficulty;
      bool                    mPlayContinuous:1;
};


//==============================================================================
class BCampaignManager
{
   public:
      enum
      {
         cUnusedFlag0=0,
         cUnusedFlag1,
         cUnusedFlag2,
         cUnusedFlag3,
         cUnusedFlag4,
         cUnusedFlag5,
         cUnusedFlag6,
         cUnusedFlag7,
         cNumberFlags
      };

      BCampaignManager( void );
      ~BCampaignManager( void );

      //ID.
      long                    getID( void ) const { return(mID); }
      void                    setID( long id ) { mID=id; }
 
      //Init.
      bool                    initialize(void);
      bool                    shutdown(void);

      //Campaigns.
      long                    getNumberCampaigns( void ) const { return(mCampaigns.getNumber()); }
      BCampaign*              getCampaign( long id );
      long                    getCampaignID( const BString &filename ) const;
      bool                    loadCampaign( const BString &filename );
      //Current Campaign.
      long                    getCurrentCampaignID( void ) const { return(mCurrentCampaignID); }
      void                    setCurrentCampaignID( long v ) { mCurrentCampaignID=v; }
      bool                    incrementCurrentCampaignNode( void );
      bool                    decrementCurrentCampaignNode( void );

      // Profile
      bool                    saveToProfile();
      bool                    loadFromProfile();

      //Flags.
      bool                    getFlag( long n ) const { if (mFlags.isBitSet(n) > (DWORD)0) return(true); return(false); }
      void                    setFlag( long n, bool v ) { if (v == true) mFlags.setBit(n); else mFlags.clearBit(n); }

      //Helper method for leaderboards
      long                    getLeaderboardIndex(const char * mapName);
      bool                    isCurrentCampaignGameFromASaveGame() {return mCurrentCampaignGameIsFromASaveGame;};
      void                    setCurrentCampaignGameIsFromASaveGame(bool value) {mCurrentCampaignGameIsFromASaveGame=value;};

   protected:
      BBitArray               mFlags;
      BDynamicArray<BCampaign*> mCampaigns;
      long                    mID;
      long                    mCurrentCampaignID;
      bool                    mCurrentCampaignGameIsFromASaveGame;
};

extern BCampaignManager gCampaignManager;

