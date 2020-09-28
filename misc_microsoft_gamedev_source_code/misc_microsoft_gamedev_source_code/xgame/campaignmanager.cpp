//==============================================================================
// campaignmanager.cpp
//
// Copyright (c) 2003-2007, Ensemble Studios
//==============================================================================

//==============================================================================
// Includes
#include "common.h"
#include "campaignmanager.h"
#include "xbox.h"
#include "gamedirectories.h"
#include "user.h"
#include "usermanager.h"
#include "database.h"
#include "userprofilemanager.h"
#include "config.h"
#include "configsgame.h"
#include "vincehelper.h"
#include "gamesettings.h"
#include "modemanager.h"
#include "ModeCampaign2.h"
#include "game.h"

//==============================================================================
// Defines


BCampaignManager gCampaignManager;


//==============================================================================
// BCampaignNode::BCampaignNode
//==============================================================================
BCampaignNode::BCampaignNode(void) :
   mID(-1),
   mName(""),
   mCaptions(""),
   mVideoNodeName(""),
   mImageIntro(""),
   mImageMinimap(""),
   mImageEndgame(""),
   mDisplayNameStringID(-1),
   mDisplayNameStringIndex(-1),
   mRolloverStringID(-1),
   mRolloverStringIndex(-1),
   mLoadTextID(-1),
   mLoadTextStringIndex(-1),
   mLoadMode(-1),
   mPresenceMovieIndex(0),
   mMusicWorld(-1),
   mAmbientWorld(-1),
   mLeaderboardIndex(0)
{
   setDefaultFlags();
}

//==============================================================================
// BCampaignNode::~BCampaignNode
//==============================================================================
BCampaignNode::~BCampaignNode(void)
{
}

//==============================================================================
// BCampaignNode::setDisplayNameStringID
//==============================================================================
void BCampaignNode::setDisplayNameStringID(long v)
{
   mDisplayNameStringID=v;
}

//==============================================================================
// BCampaignNode::getDisplayName
//==============================================================================
const BUString& BCampaignNode::getDisplayName( void )
{
   return gDatabase.getLocStringFromIndex(mDisplayNameStringIndex);
}

//==============================================================================
// BCampaignNode::getIntroText
//==============================================================================
const BUString& BCampaignNode::getIntroText( void )
{
   return gDatabase.getLocStringFromIndex(mIntroTextStringIndex);
}

//==============================================================================
// BCampaignNode::getCompletedText
//==============================================================================
const BUString& BCampaignNode::getCompletedText( void )
{
   return gDatabase.getLocStringFromIndex(mCompletedTextStringIndex);
}


//==============================================================================
// BCampaignNode::getLoadText
//==============================================================================
const BUString& BCampaignNode::getLoadText( void )
{
   return gDatabase.getLocStringFromIndex(mLoadTextStringIndex);
}

//==============================================================================
// BCampaignNode::setDefaultFlags
//==============================================================================
void BCampaignNode::setDefaultFlags(void)
{
   mFlags.setNumber(cNumberFlags);
   mFlags.clear();
   setFlag(cVisible, true);
   setFlag(cCinematic, false);
   setFlag(cPostgame, false);
   setFlag(cTutorial, false);
   setFlag(cEndOfCampaign, false);
   setFlag(cFlagLegendary, false);
}

//==============================================================================
// BCampaignNode::readXML
//==============================================================================
bool BCampaignNode::readXML(BXMLNode& node, const BSimString & /*filename*/)
{
   bool bval;

   //Get the building data.
   for (long j=0; j < node.getNumberChildren(); j++)
   {
      // Get the name of this node;
      node.getAttribValueAsString("name", mName);
      bval=false;
      node.getAttribValueAsBool("endofcampaign", bval);      
      setFlag(cEndOfCampaign, bval);

      //Get child node.
      BXMLNode node2(node.getChild(j));

      //Filename.
      BSimString temp;
      if (node2.getName().compare(B("Filename")) == 0)
      {
         node2.getText(temp);
#if !defined(BUILD_FINAL)
         if (gConfig.isDefined(cConfigUseDesignFolder))
         {
            temp.findAndReplace("final", "design");
         }
#endif
         setFilename(temp);
      }
      else if (node2.getName().compare(B("VideoNode")) == 0)
      {
         node2.getText(mVideoNodeName);
      }
      else if (node2.getName().compare(B("Legendary")) == 0)
      {
         setFlag(cFlagLegendary, true);
      }
      else if (node2.getName().compare(B("Captions")) == 0)
      {
         node2.getText(mCaptions);
      }
      //Display Name String ID.
      else if (node2.getName().compare(B("DisplayNameStringID")) == 0)
      {
         long stringID=-1;
         node2.getTextAsLong(stringID);
         if (stringID >= 0)
         {
            setDisplayNameStringID(stringID);
            mDisplayNameStringIndex=gDatabase.getLocStringIndex(stringID);
         }
      }
      else if (node2.getName().compare(B("IntroTextStringID")) == 0)
      {
         long stringID=-1;
         node2.getTextAsLong(stringID);
         if (stringID >= 0)
         {
            mIntroTextStringIndex=gDatabase.getLocStringIndex(stringID);
         }
      }
      else if (node2.getName().compare(B("CompletedTextStringID")) == 0)
      {
         long stringID=-1;
         node2.getTextAsLong(stringID);
         if (stringID >= 0)
         {
            mCompletedTextStringIndex=gDatabase.getLocStringIndex(stringID);
         }
      }

      //Visible.
      else if (node2.getName().compare(B("Visible")) == 0)
      {
         long flag=0;
         node2.getTextAsLong(flag);
         if (flag == 1)
            setFlag(cVisible, true);
         else
            setFlag(cVisible, false);
      }
      //Leaderboard index
      else if (node2.getName().compare(B("LeaderboardIndex")) == 0)
      {
         node2.getTextAsLong(mLeaderboardIndex);
      }
      //Presence movie index
      else if (node2.getName().compare(B("PresenceMovieIndex")) == 0)
      {
         node2.getTextAsLong(mPresenceMovieIndex);
      }      
      //Cinematic.
      else if (node2.getName().compare(B("Cinematic")) == 0)
      {
         long flag=0;
         node2.getTextAsLong(flag);
         if (flag == 1)
            setFlag(cCinematic, true);
         else
            setFlag(cCinematic, false);
      }
      //PostCinematic
      //This value indicates both that it's a cinematic, and that it is a postgame cinematic.
      else if (node2.getName().compare(B("PostCinematic")) == 0)
      {
         long flag=0;
         node2.getTextAsLong(flag);
         if (flag == 1)
         {
            setFlag(cCinematic, true);
            setFlag(cPostgame, true);
         }
         else
         {
            setFlag(cCinematic, false);
            setFlag(cPostgame, false);
         }
      }
      //Tutorial scenarios have special treatment, so use this to designate it as such.
      else if (node2.getName().compare(B("Tutorial")) == 0)
      {
         setFlag(cTutorial, true);
      }
      else if (node2.getName().compare(B("LoadImage")) == 0)
      {
         node2.getText(mLoadImage);
      }
      else if (node2.getName().compare(B("ImageIntro")) == 0)
      {
         node2.getText(mImageIntro);
      }
      else if (node2.getName().compare(B("ImageMinimap")) == 0)
      {
         node2.getText(mImageMinimap);
      }
      else if (node2.getName().compare(B("ImageEndgame")) == 0)
      {
         node2.getText(mImageEndgame);
      }
      else if (node2.getName().compare(B("LoadTextID")) == 0)
      {
         node2.getTextAsLong(mLoadTextID);
         if (mLoadTextID >= 0)
         {
            mLoadTextStringIndex=gDatabase.getLocStringIndex(mLoadTextID);
         }
      }
      else if (node2.getName().compare(B("LoadMode")) == 0)
      {
         node2.getTextAsLong(mLoadMode);
      }
      else if (node2.getName().compare(B("NoInitialWow")) == 0)
      {
         setFlag(cNoInitialWow, true);
      }
      //Note:  These are a late game hack.  The music is controlled by events sent to wise
      //and they don't go through our code base.  In our savegames, the events have already
      //triggered, so we need to do some basic setup.  Here we are setting the music category
      //for each map based on their world.  A real solution would involve replacing the generic PlaySound
      //effects in the scripts with special PlayMusic events so we could track and save the states.
      //The other option would be to find out how to poll wwise directly for the states when saving.
      //MWC 11/17/2008
      else if (node2.getName().compare(B("MusicWorld")) == 0)
      {
         BString musicName;
         node2.getText(musicName);
         if( musicName.compare(B("Harvest")) == 0 )
            mMusicWorld = cWorldHarvest;
         else if( musicName.compare(B("Arcadia")) == 0 )
            mMusicWorld = cWorldArcadia;
         else if( musicName.compare(B("Shield_Ext")) == 0 )
            mMusicWorld = cWorldShieldExt;
         else if( musicName.compare(B("Shield_Int")) == 0 )
            mMusicWorld = cWorldShieldInt;
      }
      else if (node2.getName().compare(B("AmbientWorld")) == 0)
      {
         BString ambientName;
         node2.getText(ambientName);
         if( ambientName.compare(B("Harvest")) == 0 )
            mAmbientWorld = cWorldHarvest;
         else if( ambientName.compare(B("Arcadia")) == 0 )
            mAmbientWorld = cWorldArcadia;
         else if( ambientName.compare(B("Shield_Ext")) == 0 )
            mAmbientWorld = cWorldShieldExt;
         else if( ambientName.compare(B("Shield_Int")) == 0 )
            mAmbientWorld = cWorldShieldInt;
      }
   }

   return(true);
}


//==============================================================================
// BCampaign::BCampaign
//==============================================================================
BCampaign::BCampaign(void) :
   mID(-1),
   mDisplayNameStringID(-1),
   mRolloverStringID(-1),
   mCurrentNodeID(0),
   // mFarthestNodeID(0),
   mPlayContinuous(false)
{
   mFlags.setNumber(cNumberFlags);
   mFlags.clear();

   mCurrentDifficulty = DifficultyType::cNormal;
}

//==============================================================================
// BCampaign::~BCampaign
//==============================================================================
BCampaign::~BCampaign(void)
{
   //Nuke our campaign node.
   for (long i=0; i < mNodes.getNumber(); i++)
   {
      if (mNodes[i] != NULL)
      {
         delete mNodes[i];
         mNodes[i]=NULL;
      }
   }
   mNodes.setNumber(0);
}


//==============================================================================
// BCampaign::setDisplayNameStringID
//==============================================================================
void BCampaign::setDisplayNameStringID(long v)
{
   mDisplayNameStringID=v;
}

//==============================================================================
// BCampaign::setRolloverStringID
//==============================================================================
void BCampaign::setRolloverStringID(long v)
{
   mRolloverStringID=v;
}


//==============================================================================
// BCampaign::setLastPlayedMode
//    Note (fixme) : with splitscreen and multiple users, we need to either pull 
//                   profile stuff out of the campaign or add a user for context.
//==============================================================================
void BCampaign::setLastPlayedMode(uint8 lastPlayedMode)
{
   BUser * user = gUserManager.getPrimaryUser();
   BASSERT(user);
   if ( user->isSignedIn() )
   {
      BUserProfile * profile = user->getProfile();
      if (profile)
         profile->setLastModePlayed(lastPlayedMode);  // does not save the value.
   }
}


//==============================================================================
// BCampaign::saveToCurrentProfile
//==============================================================================
void BCampaign::saveToCurrentProfile()
{
   BUser * user = gUserManager.getPrimaryUser();
   BASSERT(user);
   if ( user->isSignedIn() )
   {
      BUserProfile * profile = user->getProfile();
      if (profile)
      {
         profile->mCampaignProgress.setCurrentCampaignNode(mCurrentNodeID);

         //10/30/2008 Removing call to writeProfile as this calls XUserWriteProfileSettings which 
         //we are only allowed to call once per session or every 5 minutes per TCR #136
         //This function was being called here at the beginning and end of every campaign mission.  Now
         //only BPlayer::processGameEnd will call writeProfile at the end of a campaign mission.
         //gUserProfileManager.writeProfile(user);
      }
   }
}

//==============================================================================
// BCampaign::setSessionID
//==============================================================================
void BCampaign::setSessionID(uint16 val)
{
   BUser * user = gUserManager.getPrimaryUser();
   BASSERT(user);
   if ( user->isSignedIn() )
   {
      BUserProfile * profile = user->getProfile();
      if (profile)
         profile->mCampaignProgress.setSessionID(val);
   }
}

//==============================================================================
// BCampaign::incrementSessionID
//==============================================================================
void BCampaign::incrementSessionID()
{
   BUser * user = gUserManager.getPrimaryUser();
   BASSERT(user);
   if ( user->isSignedIn() )
   {
      BUserProfile * profile = user->getProfile();
      if (profile)
         profile->mCampaignProgress.incrementSessionID();
   }
}

//==============================================================================
// BCampaign::getSessionID
//==============================================================================
uint16 BCampaign::getSessionID()
{
   BUser * user = gUserManager.getPrimaryUser();
   BASSERT(user);
   if ( user->isSignedIn() )
   {
      BUserProfile * profile = user->getProfile();
      if (profile)
         return profile->mCampaignProgress.getSessionID();
   }
   return 0;
}

//==============================================================================
// BCampaign::getVideoNode
//==============================================================================
BCampaignNode* BCampaign::getVideoNode(BCampaignNode* pNode)
{
   if (!pNode)
      return NULL;

   if (pNode->getVideoNodeName().length() <= 0)
      return NULL;

   int index = -1;
   for (long i=0; i < mNodes.getNumber(); i++)
   {
      if ( (mNodes[i] != NULL) &&
           (mNodes[i]->getName().compare(pNode->getVideoNodeName()) == 0) )
      {
         // found our node
         index = i;
         break;
      }
   }

   // did we find a node for that filename?
   if ((index < 0) || (index >= mNodes.getNumber()) )
      return(NULL);

   return(mNodes[index]);
}

//==============================================================================
// BCampaign::getPreviousNode
//==============================================================================
BCampaignNode* BCampaign::getPreviousNode( BCampaignNode* pNode)
{
   if (!pNode)
      return NULL;

   long id = pNode->getID();
   id--;

   if ( (id<0) || (id>=mNodes.getNumber()) )
      return NULL;

   return mNodes[id];
}

//==============================================================================
// BCampaign::getNode
//==============================================================================
BCampaignNode* BCampaign::getNode( const char * mapName )
{
   int index = -1;

   for (long i=0; i < mNodes.getNumber(); i++)
   {
      if ( (mNodes[i] != NULL) &&
           (mNodes[i]->getFilename().compare(mapName) == 0) )
      {
         // found our node
         index = i;
         break;
      }
   }

   // did we find a node for that filename?
   if ((index < 0) || (index >= mNodes.getNumber()) )
      return(NULL);

   return(mNodes[index]);
}



//==============================================================================
// BCampaign::getNode
//==============================================================================
BCampaignNode* BCampaign::getNode(long id)
{
   if ((id < 0) || (id >= mNodes.getNumber()) )
      return(NULL);
   return(mNodes[id]);
}

//==============================================================================
// BCampaign::getNodeIDByName
//==============================================================================
long BCampaign::getNodeIDByName(BString &name)
{
   for (int i=0; i<mNodes.getNumber(); i++)
   {
      if( mNodes[i] && (mNodes[i]->getName() == name))
         return mNodes[i]->getID();
   }
   return -1;
}

//==============================================================================
// BCampaign::getNodeIDByFilename
//==============================================================================
long BCampaign::getNodeIDByFilename(BString &filename)
{
   for (int i=0; i<mNodes.getNumber(); i++)
   {
      if( mNodes[i] && (mNodes[i]->getFilename() == filename))
         return mNodes[i]->getID();
   }
   return -1;
}

//==============================================================================
// BCampaign::incrementCurrentNode
//==============================================================================
bool BCampaign::incrementCurrentNode(void)
{
   mCurrentNodeID++;
   if (mCurrentNodeID >= mNodes.getNumber())
   {
      mCurrentNodeID=mNodes.getNumber()-1;
      mPlayContinuous = false;
      return(false);
   }
/*
   if (mCurrentNodeID > mFarthestNodeID)
      mFarthestNodeID = mCurrentNodeID;
*/
   return(true);
}

//==============================================================================
// BCampaign::decrementCurrentNode
//==============================================================================
bool BCampaign::decrementCurrentNode(void)
{
   mCurrentNodeID--;
   if (mCurrentNodeID < 0)
   {
      mCurrentNodeID=0;
      return(false);
   }
   return(true);
}

//==============================================================================
// BCampaign::setCurrentNodeID(long v)
//==============================================================================
bool BCampaign::setCurrentNodeID(long v)
{
   mCurrentNodeID = v;
   if (mCurrentNodeID >= mNodes.getNumber())
   {
      mPlayContinuous = false;
      mCurrentNodeID=mNodes.getNumber()-1;
      return(false);
   }
/*
   if (mCurrentNodeID > mFarthestNodeID)
      mFarthestNodeID = mCurrentNodeID;
*/
   return true;
}

//==============================================================================
// BCampaign::readXML
//==============================================================================
bool BCampaign::readXML(const BString &filename)
{
   //Save the filename.
   mFilename=filename;
   BString path=mFilename;

   BXMLReader reader;
   if (!reader.load(cDirCampaign, filename, XML_READER_LOAD_DISCARD_ON_CLOSE))
      return(false);   

   //Get root node.
   BXMLNode root(reader.getRootNode());
   BASSERT(root.getName() == "Campaign");

   //Go through the entries.
   for (long i=0; i < root.getNumberChildren(); i++)
   {
      //Get child node.
      BXMLNode node(root.getChild(i));

      //Display Name String ID.
      if (node.getName().compare(B("DisplayNameStringID")) == 0)
      {
         long stringID=-1;
         node.getTextAsLong(stringID);
         if (stringID >= 0)
            setDisplayNameStringID(stringID);
      }
      //Rollover String ID.
      else if (node.getName().compare(B("RolloverStringID")) == 0)
      {
         long stringID=-1;
         node.getTextAsLong(stringID);
         if (stringID >= 0)
            setRolloverStringID(stringID);
      }
      // TutorialStyle.
      else if (node.getName().compare(B("TutorialStyle")) == 0)
      {
         long flag=0;
         node.getTextAsLong(flag);
         if (flag == 1)
            setFlag(cTutorialStyle, true);
         else
            setFlag(cTutorialStyle, false);
      }
      // Campaign Node
      else if (node.getName().compare(B("CampaignNode")) == 0)
      {
         //Create space.
         long newID=mNodes.getNumber();
         if (mNodes.setNumber(newID+1) == false)
            continue;

         //Allocate a new node.
         BCampaignNode *newNode=new BCampaignNode();
         
         newNode->setID(newID);
         mNodes[newID]=newNode;

         //Read the data.
         if (newNode->readXML(node, mFilename) == false)
         {
            continue;
         }
      }
   }

   return(true);
}

//==============================================================================
//==============================================================================
void BCampaign::resetCampaign()
{
   resetCurrentNode();

   // other stuff?
   setPlayContinuous(false);

   long difficulty = DifficultyType::cNormal;
   BUser* pUser = gUserManager.getPrimaryUser();
   if (pUser)
      difficulty = pUser->getOption_DefaultCampaignDifficulty();
   
   setCurrentDifficulty(difficulty);
}

//==============================================================================
// BCampaignManager::launchGame
//==============================================================================
void BCampaign::setGameSettings(bool skipCinematics, bool useSaveIfAvailable)
{
   BUser* pUser=gUserManager.getPrimaryUser();
   BASSERT(pUser);
   if (!pUser)
      return;

   // Get the current node
   BCampaignNode* pNode = getNode( getCurrentNodeID() );
   BASSERT(pNode);
   if (!pNode)
      return;

   if (skipCinematics)
   {
      while (pNode->getFlag(BCampaignNode::cCinematic))
      {
         if (!incrementCurrentNode())
            break;

         pNode = getNode( getCurrentNodeID() );
         BASSERT(pNode);
         if (!pNode)
            return;
      }
   }

   // Get the scenario settings
   BSimString mapName = pNode->getFilename();

   if (gConfig.isDefined(cConfigDemo))
   {
      if (mapName == "CampaignUNSC\\design\\PHXscn02\\PHXscn02")
         mapName = "development\\scenarioTGS\\scenarioTGS";
   }

   //------------------------------------------------------------
   gDatabase.resetGameSettings();
   BGameSettings* pSettings = gDatabase.getGameSettings();
   if(pSettings)
   {

      uint16 rank = 0;
      if (pUser->getProfile())
         rank = pUser->getProfile()->getRank().mValue;

      BSimString gameID;
      MVince_CreateGameID(gameID);

      BUser* pUser2=gUserManager.getSecondaryUser();

      // set up the game options
      pSettings->setLong(BGameSettings::cPlayerCount, 1);
      pSettings->setString(BGameSettings::cMapName, mapName);
      pSettings->setLong(BGameSettings::cMapIndex, -1);
      pSettings->setLong(BGameSettings::cRandomSeed, timeGetTime());
      pSettings->setString(BGameSettings::cGameID, gameID);
      pSettings->setWORD(BGameSettings::cPlayer1Rank, rank);
      pSettings->setString(BGameSettings::cPlayer1Name, pUser->getName());
      pSettings->setUInt64(BGameSettings::cPlayer1XUID, pUser->getXuid());
      pSettings->setLong(BGameSettings::cPlayer1Type, BGameSettings::cPlayerHuman);
      pSettings->setBYTE(BGameSettings::cLoadType, BGameSettings::cLoadTypeNone);
      pSettings->setLong(BGameSettings::cGameType, BGameSettings::cGameTypeCampaign);
      pSettings->setLong(BGameSettings::cNetworkType, BGameSettings::cNetworkTypeLocal);

      float diffVal = gDatabase.getDifficultyDefault();
      diffVal = gDatabase.getDifficulty(mCurrentDifficulty);

      pSettings->setFloat(BGameSettings::cPlayer1Difficulty, diffVal);
      pSettings->setLong(BGameSettings::cPlayer1DifficultyType, mCurrentDifficulty);

      pSettings->setLong(BGameSettings::cGameMode, 0);

      if (gGame.isSplitScreenAvailable() && pUser2->getFlagUserActive())
      {
         rank = 0;
         if (pUser2->getProfile())
            rank = pUser2->getProfile()->getRank().mValue;

         pSettings->setBool(BGameSettings::cCoop, true);
         pSettings->setLong(BGameSettings::cPlayerCount, 2);
         pSettings->setWORD(BGameSettings::cPlayer2Rank, rank);
         pSettings->setString(BGameSettings::cPlayer2Name, pUser2->getName());
         pSettings->setUInt64(BGameSettings::cPlayer2XUID, pUser2->getXuid());
         pSettings->setLong(BGameSettings::cPlayer2Type, BGameSettings::cPlayerHuman);
      }

      gCampaignManager.setCurrentCampaignGameIsFromASaveGame(false);
      if (useSaveIfAvailable)
      {
         //gSaveGame.prepCampaignLoad();
         pSettings->setBYTE(BGameSettings::cLoadType, BGameSettings::cLoadTypeSave);
         pSettings->setString(BGameSettings::cLoadName, "campaign");
		 gCampaignManager.setCurrentCampaignGameIsFromASaveGame(true);
      }
   }
}

//==============================================================================
// BCampaignManager::launchGame
//==============================================================================
void BCampaign::launchGame(bool skipCinematics, bool useSaveIfAvailable)
{
   BUser* pUser=gUserManager.getPrimaryUser();
   BASSERT(pUser);

   // Get the current node
   BCampaignNode* pNode = getNode( getCurrentNodeID() );
   BASSERT(pNode);

   // handle the end of the campaign
   if (pNode->getFlag(BCampaignNode::cEndOfCampaign))
   {
      BModeCampaign2* pMode = (BModeCampaign2*)gModeManager.getMode(BModeManager::cModeCampaign2);
      BASSERT(pMode);

      pMode->endOfCampaign();
      return;
   }

   // fill in all the settings
   setGameSettings(skipCinematics, useSaveIfAvailable);

   // [7/22/2008 xemu] ok, this is awful, but just porting over what we had before.  If you just do a direct mode transition
   //   you get caught in a weird UI state, so if we want to make it general we still need a deferring mechanism.
   BModeCampaign2* pMode = (BModeCampaign2*)gModeManager.getMode(BModeManager::cModeCampaign2);
   BASSERT(pMode);
   pMode->setNextState(BModeCampaign2::cStateStartSPCampaign);
   //gModeManager.setMode(BModeManager::cModeGame);
}




//==============================================================================
// BCampaignManager::BCampaignManager
//==============================================================================
BCampaignManager::BCampaignManager(void) :
   mID(-1),
   mCurrentCampaignID(-1),
   mCurrentCampaignGameIsFromASaveGame(false)
{
   mFlags.setNumber(cNumberFlags);
   mFlags.clear();
}

//==============================================================================
// BCampaignManager::~BCampaignManager
//==============================================================================
BCampaignManager::~BCampaignManager(void)
{
   shutdown();
}


//==============================================================================
// BCampaignManager::shutdown
//==============================================================================
bool BCampaignManager::shutdown(void)
{
   //Nuke our campaigns.
   for (long i=0; i < mCampaigns.getNumber(); i++)
   {
      if (mCampaigns[i] != NULL)
      {
         delete mCampaigns[i];
         mCampaigns[i]=NULL;
      }
   }
   mCampaigns.setNumber(0);
   return true;
}


//==============================================================================
// BCampaignManager::initialize
//==============================================================================
bool BCampaignManager::initialize(void)
{
   loadCampaign("HaloWars.xml");
   return(true);
}

//==============================================================================
// BCampaignManager::getCampaign
//==============================================================================
BCampaign* BCampaignManager::getCampaign(long id)
{
   if ((id < 0) || (id >= mCampaigns.getNumber()) )
      return(NULL);
   return(mCampaigns[id]);
}

//==============================================================================
// BCampaignManager::getCampaignID
//==============================================================================
long BCampaignManager::getCampaignID(const BString &filename) const
{
   for (long i=0; i < mCampaigns.getNumber(); i++)
   {
      if (mCampaigns[i]->getFilename() == filename)
         return(mCampaigns[i]->getID());
   }
   return(-1);
}

//==============================================================================
// BCampaignManager::loadCampaign
//==============================================================================
bool BCampaignManager::loadCampaign(const BString &filename)
{
   //Make space.
   long newID=mCampaigns.getNumber();
   if (mCampaigns.setNumber(newID+1) == false)
      return(false);

   //Create a new campaign.
   BCampaign *newCampaign=new BCampaign();
   if (newCampaign == NULL)                  // fixme - remove the null check
   {
      mCampaigns.setNumber(newID);
      return(false);
   }
   newCampaign->setID(newID);

   //Load it.
   if (newCampaign->readXML(filename) == false)
   {
      delete newCampaign;
      mCampaigns.setNumber(newID);
      return(false);
   }

   //Save it.
   mCampaigns[newID]=newCampaign;

   //Done.
   return(true);
}

//==============================================================================
// BCampaignManager::incrementCurrentCampaignNode
//==============================================================================
bool BCampaignManager::incrementCurrentCampaignNode(void)
{
   BCampaign *campaign=getCampaign(mCurrentCampaignID);
   if (campaign == NULL)
      return(false);
   return(campaign->incrementCurrentNode());
}

//==============================================================================
// BCampaignManager::decrementCurrentCampaignNode
//==============================================================================
bool BCampaignManager::decrementCurrentCampaignNode(void)
{
   BCampaign *campaign=getCampaign(mCurrentCampaignID);
   if (campaign == NULL)
      return(false);
   return(campaign->decrementCurrentNode());
}


//==============================================================================
// BCampaignManager::saveToProfile
//==============================================================================
bool BCampaignManager::saveToProfile()
{

   // save to profile:
   // - Current Campaign
   // - Current Scenario
   // - Farthest unlocked scenario

   return true;
}

//==============================================================================
// BCampaignManager::loadFromProfile
//==============================================================================
bool BCampaignManager::loadFromProfile()
{
   // retrieve from profile:
   // - Current Campaign
   // - Current Scenario
   // - Farthest unlocked scenario
/*
   //Get the desired campaign.
   BCampaign *campaign = getCampaign(nProfileCurrentCampaign);
   if (campaign == NULL)
      return false;

   // Tell the campaign mgr this is the current campaign & reset the scenario id.
   setCurrentCampaignID(nProfileCurrentCampaign);
   campaign->setFarthestNodeID(nProfileFarthestScenario);
   campaign->setCurrentNodeID(nProfileCurrentScenario);
*/
   return true;
}

//==============================================================================
// BCampaignManager::getLeaderboardIndex
//==============================================================================
long BCampaignManager::getLeaderboardIndex(const char * mapName)
{
   //BCampaign* campaign = getCampaign(getCurrentCampaignID());
   BCampaign* campaign = getCampaign(0);
   if (!campaign)
   {
      return 0;
   }
   BCampaignNode* node = campaign->getNode( mapName );
   if (!node)
   {
      return 0;
   }
   return (node->getLeaderboardLevelIndex());
}
