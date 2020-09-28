//==============================================================================
// skullManager.cpp
//
// Copyright (c) 2001-2008, Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "skullManager.h"
#include "world.h"
#include "user.h"
#include "gamedirectories.h"
#include "userprofilemanager.h"
#include "usermanager.h"
#include "scoremanager.h"
#include "techtree.h"
#include "commands.h"
#include "commandmanager.h"
#include "gamesettings.h"
#include "uimanager.h"
#include "achievementmanager.h"
#include "file\xcontentStream.h"
#include "uiticker.h"

#include "object.h"


// Constants

// Globals
BCollectiblesManager gCollectiblesManager;

GFIMPLEMENTVERSION(BCollectiblesManager, 1);


//============================================================================
//============================================================================
//BSkullModifier
//============================================================================
//============================================================================

//============================================================================
//============================================================================
BSkullModifier::BSkullModifier() :
   mModifierType(cSK_None),
   mModifierValue(0.0f),
   mModifierTarget(cSKT_NoTarget)
{
}

//============================================================================
//============================================================================
BSkullModifier::~BSkullModifier()
{
}


//============================================================================
//============================================================================
//BProtoSkull
//============================================================================
//============================================================================

//============================================================================
//============================================================================
BProtoSkull::BProtoSkull() :
   mActive(false),
   mUIActive(false),
   mSelfActive(false),
   mSelfUIActive(false),
   mHidden(false),
   mCommandSent(false),
   mOnFromBeginning(false),
   mIndex(0),
   mObjectDBID(0),
   mDescriptionID(-1),
   mDisplayNameID(-1)
{
   mName.empty();
   mModifiers.empty();
}

//============================================================================
//============================================================================
BProtoSkull::~BProtoSkull()
{
   //Delete any modifiers created.
   for(int i=0; i<mModifiers.getNumber(); i++)
      delete mModifiers.get(i);
}

//==============================================================================
// BProtoSkull::loadXML
//==============================================================================
bool BProtoSkull::loadXML(BXMLNode node)
{
   // Sanity.
   if(!node)
      return(false);

   // Get the name of this achievement
   node.getAttribValueAsString("name", mName);

   // [10/7/2008 xemu] Get the object ID of the associated collectible
   node.getAttribValueAsLong("objectdbid", mObjectDBID);

   long nodeCount=node.getNumberChildren();
   for(long i=0; i<nodeCount; i++)
   {
      BXMLNode childNode(node.getChild(i));
      const BPackedString name(childNode.getName());
      BSimString typeStr;
      BSimString targetStr;
      if (name=="Effect")
      {
         BSkullModifier *mod = new BSkullModifier();

         childNode.getText(typeStr);
         childNode.getAttribValueAsFloat("value", mod->mModifierValue);
         childNode.getAttribValueAsString("target", targetStr) ;

         if(targetStr == "PlayerUnits")
            mod->mModifierTarget = cSKT_PlayerUnits;
         else if(targetStr == "NonPlayerUnits")
            mod->mModifierTarget = cSKT_NonPlayerUnits;
         else if(targetStr == "OwnerOnly")
            mod->mModifierTarget = cSKT_OwnerOnly;

         if(typeStr == "Score")
            mod->mModifierType = cSK_Score;
         else if(typeStr == "GruntTank")
            mod->mModifierType = cSK_GruntTank;
         else if(typeStr == "GruntConfetti")
            mod->mModifierType = cSK_GruntConfetti;
         else if(typeStr == "Physics")
            mod->mModifierType = cSK_Physics;
         else if(typeStr == "ScarabBeam")
            mod->mModifierType = cSK_ScarabBeam;
         else if(typeStr == "MinimapDisable")
            mod->mModifierType = cSK_MinimapDisable;
         else if(typeStr == "Weakness")
            mod->mModifierType = cSK_Weakness;
         else if(typeStr == "HitpointMod")
            mod->mModifierType = cSK_HitpointMod;
         else if(typeStr == "DamageMod")
            mod->mModifierType = cSK_DamageMod;
         else if(typeStr == "Veterancy")
            mod->mModifierType = cSK_Veterancy;
         else if(typeStr == "AbilityRecharge")
            mod->mModifierType = cSK_AbilityRecharge;
         else if(typeStr == "DeathExplode")
            mod->mModifierType = cSK_DeathExplode;
         else if(typeStr == "TrainMod")
            mod->mModifierType = cSK_TrainMod;
         else if(typeStr == "SupplyMod")
            mod->mModifierType = cSK_SupplyMod;
         else if(typeStr == "PowerRecharge")
            mod->mModifierType = cSK_PowerRecharge;
         else if(typeStr == "UnitModWarthog")
            mod->mModifierType = cSK_UnitModWarthog;
         else if(typeStr == "UnitModWraith")
            mod->mModifierType = cSK_UnitModWraith;

         mModifiers.add(mod);
      }
      else if (name=="DescriptionID")
      {
         mDescriptionID = -1;
         long id = -1;
         if(childNode.getTextAsLong(id))
            mDescriptionID = id;         
      }
      else if (name=="DisplayNameID")
      {
         mDisplayNameID = -1;
         long id = -1;
         if(childNode.getTextAsLong(id))
            mDisplayNameID = id;        
      }
      else if (name=="DisplayImageOn")
      {
         childNode.getText(mDisplayImageOn);
      }
      else if (name=="DisplayImageOff")
      {
         childNode.getText(mDisplayImageOff);
      }
      else if (name=="DisplayImageLocked")
      {
         childNode.getText(mDisplayImageLocked);
      }
      else if (name=="Hidden")
      {
         mHidden = true;
      }
   }

   // Success.
   return true;
}

//============================================================================
// BProtoSkull::isOwnerOnly
//============================================================================
bool BProtoSkull::isOwnerOnly() const
{
   int i;
   for (i=0; i < mModifiers.getNumber(); i++)
   {
      // [10/10/2008 xemu] at least one modifier needs to actively be owner only
      if (mModifiers[i]->mModifierTarget == cSKT_OwnerOnly)
         return(true);

   }

   return(false);
}


//============================================================================
// BProtoSkull::checkActive
//============================================================================
bool BProtoSkull::checkActive(long playerID) const 
{
   bool retval = false;
   // [10/10/2008 xemu] note that isOwnerOnly is only relevant when we are comparing to ourselves 
   // [10/10/2008 xemu] otherwise, we want to always allow other people's owner-only commands to go through 
   if (isOwnerOnly())
   {
      if (playerID == gUserManager.getPrimaryUser()->getPlayerID())
         retval = mSelfActive;
      else
         retval = false;
   }
   else
      retval = mActive;
   return(retval);
}

//============================================================================
//============================================================================
//BProtoTimeLineEvent
//============================================================================
//============================================================================

//============================================================================
//============================================================================
BProtoTimeLineEvent::BProtoTimeLineEvent() :
   mEventType(cTLET_Invalid),
   mObjectDBID(-1)
{
   mEventName.empty();
   mLeaderName.empty();
   mMapName.empty();
}

//============================================================================
//============================================================================
BProtoTimeLineEvent::~BProtoTimeLineEvent()
{
}

//==============================================================================
// BProtoSkull::loadXML
//==============================================================================
bool BProtoTimeLineEvent::loadXML(BXMLNode node)
{
   // Sanity.
   if(!node)
      return(false);

   // Get the name of this achievement
   node.getAttribValueAsString("eventname", mEventName);

   long nodeCount=node.getNumberChildren();
   for(long i=0; i<nodeCount; i++)
   {
      BXMLNode childNode(node.getChild(i));
      const BPackedString name(childNode.getName());
      if (name=="Leader")
      {
         childNode.getText(mLeaderName);
         BASSERT(mEventType == cTLET_Invalid);
         mEventType = cTLET_Leader;
      }
      else if (name=="Map")
      {
         childNode.getText(mMapName);
         BASSERT(mEventType == cTLET_Invalid);
         mEventType = cTLET_Map;
      }
      else if (name=="BlackBox")
      {
         mObjectDBID = -1;
         long id = -1;
         if(childNode.getTextAsLong(id))
            mObjectDBID = id;        
         BASSERT(mEventType == cTLET_Invalid);
         mEventType = cTLET_BlackBox;
      }
   }

   // Success.
   return true;
}


//============================================================================
//============================================================================
//BCollectiblesManager
//============================================================================
//============================================================================

//============================================================================
//============================================================================
BCollectiblesManager::BCollectiblesManager():
   mVersion(0)
{
   reset();
}

//============================================================================
//============================================================================
BCollectiblesManager::~BCollectiblesManager()
{
   reset();
}

//============================================================================
//============================================================================
void BCollectiblesManager::reset()
{
   //mCollectedSkullArray.clear();
   mVersion = 0;

   mbRocketAllGrunts = false;
   mbMinimapHidden = false;
   mBonusSquadLevels = 0;
   mDeathExplodeChance = 0.0f;
   mDeathExplodeObjectType = -1;
   mDeathExplodeProtoObject = -1;

   //Delete any definitions created.
   for(int i=0; i<mSkullDefinitions.getNumber(); i++)
      delete mSkullDefinitions.get(i);

   for(int i=0; i<mTimeLineEventDefinitions.getNumber(); i++)
      delete mTimeLineEventDefinitions.get(i);
}

//==============================================================================
// BCollectiblesManager::loadCollectiblesDefinitions
//==============================================================================
bool BCollectiblesManager::loadCollectiblesDefinitions()
{
   BXMLReader reader;
   if (!reader.load(cDirData, "skulls.xml", XML_READER_LOAD_DISCARD_ON_CLOSE))
      return(false);   

   BXMLNode rootNode(reader.getRootNode());

   BASSERT(rootNode.getName() == "CollectiblesDefinitions");

   //Create an achievement rule for each entry.
   for (long i=0; i < rootNode.getNumberChildren(); i++)
   {
      //Get child node.
      BXMLNode node(rootNode.getChild(i));

      // Create the class based on the tag
      // Check for version node.
      if (node.getName().compare("CollectiblesXMLVersion") == 0)
      {
         mVersion = 0;
         node.getTextAsUInt32(mVersion);
      }
      else if (node.getName().compare("Skull") == 0)
      {
         BProtoSkull* pObject = new BProtoSkull();
         BASSERT(pObject);

         // Load the achievement
         pObject->loadXML(node);

         pObject->mIndex = i;

         // Add the skull object to the definitions list
         mSkullDefinitions.add(pObject);
      }
      else if (node.getName().compare("TimeLineEvent") == 0)
      {
         BProtoTimeLineEvent *pObject = new BProtoTimeLineEvent();
         BASSERT(pObject);

         // Load the achievement
         pObject->loadXML(node);

         // Add the skull object to the definitions list
         mTimeLineEventDefinitions.add(pObject);
      }
   }

   return(true);
}

//==============================================================================
// BCollectiblesManager::reportSkullCollected
//==============================================================================
void BCollectiblesManager::reportSkullCollected(BUser *user, long objectdbid)
{
   long skullID = -1;
   for (int i=0; i<mSkullDefinitions.getNumber(); i++)
   {
      if( mSkullDefinitions.get(i)->mObjectDBID == objectdbid )
      {
         skullID = i;
         break;
      }
   }

   if (skullID == -1)
      return;

   BASSERT(skullID<32 && skullID>=0);

   BUser* user2 = NULL;
   if (user == NULL)
   {
      user = gUserManager.getPrimaryUser();
      user2 = gUserManager.getSecondaryUser();
   }

   bool showMessage = false;

   if (user && user->getPlayer() && (user->getPlayer()->getPlayerState() == BPlayer::cPlayerStatePlaying))
   {
      BUserProfile *profile = user->getProfile();
      if (profile)
      {
         uint32 bits = profile->getSkullBits();
         bits |= (1 << skullID);
         profile->setSkullBits(bits);
         showMessage=true;
      }
      // [10/3/2008 xemu] update achievements
      gAchievementManager.updateMisc(user, cMiscATSkulls);
   }
   if (user2 && user2->getPlayer() && (user2->getPlayer()->getPlayerState() == BPlayer::cPlayerStatePlaying) )
   {
      BUserProfile *profile = user2->getProfile();
      if (profile)
      {
         uint32 bits = profile->getSkullBits();
         bits |= (1 << skullID);
         profile->setSkullBits(bits);
         showMessage=true;
      }
      // [10/3/2008 xemu] update achievements
      gAchievementManager.updateMisc(user2, cMiscATSkulls);
   }

   if (showMessage)
   {
      const BProtoSkull* pSkull = getSkullDefinition(skullID);
      if (pSkull)
      {
         BUString message;
         message.locFormat(gDatabase.getLocStringFromID(25727), gDatabase.getLocStringFromID(pSkull->mDisplayNameID).getPtr());
         user->addFlashUserMessage(message, "", true, 3.0f);
      }
   }

}

//==============================================================================
// BCollectiblesManager::hasSkullBeenCollected
//==============================================================================
bool BCollectiblesManager::hasSkullBeenCollected(BUser *user, int index)
{
   BASSERT(index<32);

   #ifndef BUILD_FINAL
   if(gConfig.isDefined("unlockAllCampaignMissions"))
       return true;
   #endif

   BUserProfile *profile = user->getProfile();
   if (profile)
   {
      uint32 bits = profile->getSkullBits();
      if ( bits & (1 << index) )
         return true;
   }

   return false;
}

//==============================================================================
// BCollectiblesManager::hasBlackBoxBeenCollected
//==============================================================================
bool BCollectiblesManager::hasBlackBoxBeenCollected(BUser *user, int index)
{
   BASSERT(index<32);

   long firstBlackboxID = -1;
   for (int i=0; i<mTimeLineEventDefinitions.getNumber(); i++)
   {
      if( mTimeLineEventDefinitions.get(i)->mEventType == cTLET_BlackBox )
      {
         firstBlackboxID = i;
         break;
      }
   }
   BASSERT(firstBlackboxID >= 0);

   BUserProfile *profile = user->getProfile();
   if (profile)
   {
      uint64 bits = profile->getTimeLineBits();
      if ( bits & (uint64(1) << (firstBlackboxID+index)) )
         return true;
   }

   return false;
}

//==============================================================================
// BCollectiblesManager::getSkullDefinition
//==============================================================================
const BProtoSkull* BCollectiblesManager::getSkullDefinition(int index) const
{
   debugRangeCheck(index, mSkullDefinitions.getNumber());
   if (index < 0 || index >=mSkullDefinitions.getNumber())
      return NULL;

   return mSkullDefinitions[index];
}

//==============================================================================
// BCollectiblesManager::getNumberSkulls
//==============================================================================
int BCollectiblesManager::getNumberSkulls(bool countHidden) const
{
   if (countHidden)
     return mSkullDefinitions.getNumber();
   else
   {
      int c = 0;
      int i;
      for (i = 0; i < mSkullDefinitions.getNumber(); i++)
      {
         BProtoSkull *pSkull = mSkullDefinitions[i];
         BASSERT(pSkull);
         if (pSkull)
         {
            if (!pSkull->mHidden)
               c++;
         }
      }
      return(c);
   }
}

//==============================================================================
// BCollectiblesManager::getNumSkullsCollected
//==============================================================================
int BCollectiblesManager::getNumSkullsCollected(BUser *pUser, bool countHidden, bool countOwnerOnly) const
{
   if (pUser == NULL)
   {
      BASSERT(0);
      return(0);
   }
   const BUserProfile *pProfile = pUser->getProfile();
   if (pProfile == NULL)
      return(0);

   uint32 sb = pProfile->getSkullBits();
   int numSkulls = getNumberSkulls(true);
   int collectedCount = 0;
   int i;
   for (i=0; i < numSkulls; i++)
   {
      // [10/22/2008 xemu] if we are not counting hidden skulls, then just move along if we encounter one 
      if (!countHidden)
      {
         BProtoSkull *pSkull = mSkullDefinitions[i];
         BASSERT(pSkull);
         if (pSkull->mHidden)
            continue;
      }

      if (!countOwnerOnly)
      {
         BProtoSkull *pSkull = mSkullDefinitions[i];
         BASSERT(pSkull);
         if (pSkull->isOwnerOnly())
            continue;
      }

      if (sb & (0x1 << i))
         collectedCount++;
   }
   return(collectedCount);
}


//==============================================================================
// BCollectiblesManager::setSkullActivationUI
//==============================================================================
void BCollectiblesManager::setSkullActivationUI(int index, bool active, long playerID)
{
   if (index<0 || index>=mSkullDefinitions.getNumber())
      return;

   BProtoSkull *pSkull = mSkullDefinitions[index];
   BASSERT(pSkull);
   pSkull->mUIActive = active;
   pSkull->mSelfUIActive = active;

   pSkull->mCommandSent = false;
}

//==============================================================================
// BCollectiblesManager::sendSkullActivationCommands
//==============================================================================
void BCollectiblesManager::sendSkullActivationCommands(long playerID)
{
   int i;
   for (i=0; i < mSkullDefinitions.getNumber(); i++)
   {
      BProtoSkull *pSkull = mSkullDefinitions[i];
      // [10/9/2008 xemu] ok, if we are UI active but not really activated, send out a command to get us up and running
      // [10/9/2008 xemu] also added a check to prevent command spamming (since game can be paused during all this sending) 
      // [10/10/2008 xemu] added some complexity to handle self-only skulls which can be multiply activated
      bool alreadyActive;
      alreadyActive = pSkull->checkActive(playerID);
      if (pSkull->mUIActive && !alreadyActive && !pSkull->mCommandSent)
      {
         BGameCommand* pCommand=(BGameCommand*)gWorld->getCommandManager()->createCommand(playerID, cCommandGame);
         if(pCommand)
         {
            pCommand->setSenderType(BCommand::cPlayer);
            pCommand->setSenders(1, &playerID);
            pCommand->setRecipientType(BCommand::cGame);
            pCommand->setType(BGameCommand::cTypeActivateSkull);
            pCommand->setData(i);
            pCommand->setData2(1);
            gWorld->getCommandManager()->addCommandToExecute(pCommand);
            gConsoleOutput.status("Skull %d activated", i);
            pSkull->mCommandSent = true;
         }
      }
   }
}

//==============================================================================
// BCollectiblesManager::activateSkull
//==============================================================================
void BCollectiblesManager::setSkullActivationInternal(int index, bool active, long playerID)
{
   if (index<0 || index>=mSkullDefinitions.getNumber())
      return;

   BProtoSkull* skullDef = mSkullDefinitions[index];
   BASSERT(skullDef);

   // [10/9/2008 xemu] reject duplicate activations
   if (skullDef->checkActive(playerID))
      return;

   for( int i=0; i<skullDef->mModifiers.getNumber(); i++ )
   {
      BSkullModifier *mod = skullDef->mModifiers.get(i);
      switch(mod->mModifierType)
      {
         //float mModifierValue;
         //int   mModifierTarget;
         case cSK_Score: 
         {
            gScoreManager.addSkullModifier(mod->mModifierValue);
            break;
         }
         case cSK_GruntTank: 
         {
            mbRocketAllGrunts = true;
            break;
         }
         case cSK_GruntConfetti: 
         {
            BProtoTechID protoTechID = cInvalidTechID;
            protoTechID = gDatabase.getProtoTech("Skull_02");
            BASSERT(protoTechID != cInvalidTechID);

            for (int i=1; i<gWorld->getNumberPlayers(); i++)
            {
               BPlayer*     pPlayer = gWorld->getPlayer(i);
               BTechTree*   pTech   = (pPlayer ? pPlayer->getTechTree() : NULL);
               BASSERT(pTech);
               if (pTech)
                  pTech->activateTech(protoTechID, cInvalidObjectID, true);
            }
            break;
         }
         case cSK_Physics: 
         {
            BProtoTechID protoTechID = cInvalidTechID;
            protoTechID = gDatabase.getProtoTech("Skull_03");
            BASSERT(protoTechID != cInvalidTechID);

            for (int i=1; i<gWorld->getNumberPlayers(); i++)
            {
               BPlayer*     pPlayer = gWorld->getPlayer(i);
               BTechTree*   pTech   = (pPlayer ? pPlayer->getTechTree() : NULL);
               BASSERT(pTech);
               if (pTech)
                  pTech->activateTech(protoTechID, cInvalidObjectID, true);
            }
            break;
         }
         case cSK_ScarabBeam: 
         {
            BProtoTechID protoTechID = cInvalidTechID;
            protoTechID = gDatabase.getProtoTech("Skull_04");
            BASSERT(protoTechID != cInvalidTechID);

            for (int i=1; i<gWorld->getNumberPlayers(); i++)
            {
               BPlayer*     pPlayer = gWorld->getPlayer(i);
               BTechTree*   pTech   = (pPlayer ? pPlayer->getTechTree() : NULL);
               BASSERT(pTech);
               if (pTech)
                  pTech->activateTech(protoTechID, cInvalidObjectID, true);
            }
            break;
         }
         case cSK_MinimapDisable: 
         {
            mbMinimapHidden = true;
            gUIManager->setMinimapVisible(false);
            break;
         }
#ifdef OLD_FLOOD_SKULL
         case cSK_Flood: 
         {
            int floodCiv = -1;

            //
            // Try to find the flood civ
            //

            // Find a "SkullFlood" civ
            for (int i=1; i<gWorld->getNumberPlayers(); i++)
            {
               const BPlayer* pPlayer = gWorld->getPlayer(i);
               // No bad players.
               if (!pPlayer)
                  continue;
               // No non-npc players.
               if (!pPlayer->isNPC())
                  continue;
               if (pPlayer->getName() != "SkullFlood")
                  continue;
               floodCiv = i;
               break;
            }

            // No "SkullFlood", try "Creeps"
            if (floodCiv == -1)
            {
               for (int i=1; i<gWorld->getNumberPlayers(); i++)
               {
                  const BPlayer* pPlayer = gWorld->getPlayer(i);
                  // No bad players.
                  if (!pPlayer)
                     continue;
                  // No non-npc players.
                  if (!pPlayer->isNPC())
                     continue;
                  if (pPlayer->getName() != "Creeps")
                     continue;
                  floodCiv = i;
                  break;
               }
            }

            // Nope, no valid civs yet, find a civ that's hostile to all players
            if (floodCiv == -1)
            {
               BSmallDynamicSimArray<BPlayer*> players;

               // Get a list of all human players
               for (int i=1; i<gWorld->getNumberPlayers(); i++)
               {
                  BPlayer* pPlayer = gWorld->getPlayer(i);
                  if (pPlayer && pPlayer->isHuman())
                     players.add(pPlayer);
               }

               for (int i=1; i<gWorld->getNumberPlayers(); i++)
               {
                  BPlayer* pPlayer = gWorld->getPlayer(i);
                  if (pPlayer && pPlayer->isHuman())
                     continue;
                  
                  uint count = 0;
                  for (uint j=0; j<players.size(); ++j)
                  {
                     if (pPlayer->isEnemy(players[j]))
                        ++count;
                  }

                  if (count != players.size())
                     continue;

                  floodCiv = i;
                  break;
               }
            }

            // If this assert fires, there is not a valid civ for the flood.  If this is a scenario, the "SkullFlood" civ should exist.
            // If it's a skirmish, it should have "SkullFlood" or "Creeps".
            BASSERT(floodCiv != -1);

            for (int i=1; i<gWorld->getNumberPlayers(); i++)
            {
               BPlayer* pPlayer = gWorld->getPlayer(i);
               if (pPlayer->isHuman())
                  pPlayer->setFloodPoofPlayer(floodCiv);
            }
            break;
         }
#endif
         case cSK_Weakness: 
         {
            BProtoTechID protoTechID = cInvalidTechID;
            protoTechID = gDatabase.getProtoTech("Skull_06");

            BASSERT(protoTechID != cInvalidTechID);
            for (int i=1; i<gWorld->getNumberPlayers(); i++)
            {
               BPlayer*     pPlayer = gWorld->getPlayer(i);
               // [10/13/2008 xemu] apply only to human players 
               if (!pPlayer->isHuman())
                  continue;

               BTechTree*   pTech   = (pPlayer ? pPlayer->getTechTree() : NULL);
               BASSERT(pTech);
               if (pTech)
                  pTech->activateTech(protoTechID, cInvalidObjectID, true);
            }
            break;
         }

         case cSK_HitpointMod: 
         {
            BProtoTechID protoTechID = cInvalidTechID;
            if( mod->mModifierTarget == cSKT_PlayerUnits )
               protoTechID = gDatabase.getProtoTech("Skull_11");
            else if( mod->mModifierTarget == cSKT_NonPlayerUnits )
               protoTechID = gDatabase.getProtoTech("Skull_07");

            BASSERT(protoTechID != cInvalidTechID);
            for (int i=1; i<gWorld->getNumberPlayers(); i++)
            {
               BPlayer*     pPlayer = gWorld->getPlayer(i);
               BTechTree*   pTech   = (pPlayer ? pPlayer->getTechTree() : NULL);
               BASSERT(pTech);
               if (pTech)
                  pTech->activateTech(protoTechID, cInvalidObjectID, true);
            }
            break;
         }
         case cSK_DamageMod: 
         {
            BProtoTechID protoTechID = cInvalidTechID;
            protoTechID = gDatabase.getProtoTech("Skull_08");
            BASSERT(protoTechID != cInvalidTechID);

            for (int i=1; i<gWorld->getNumberPlayers(); i++)
            {
               BPlayer*     pPlayer = gWorld->getPlayer(i);
               BTechTree*   pTech   = (pPlayer ? pPlayer->getTechTree() : NULL);
               BASSERT(pTech);
               if (pTech)
                  pTech->activateTech(protoTechID, cInvalidObjectID, true);
            }
            break;
         }
         case cSK_Veterancy: 
         {
            // [10/8/2008 xemu] go through all the non-player squads and give them an XP boost
            for (int i=1; i<gWorld->getNumberPlayers(); i++)
            {
               BPlayer*     pPlayer = gWorld->getPlayer(i);
               if (!pPlayer->isHuman())
               {
                  BEntityIDArray squadList;
                  pPlayer->getSquads(squadList);
                  int s;
                  for (s=0; s < squadList.getNumber(); s++)
                  {
                     BSquad *pSquad = gWorld->getSquad(squadList[s]);
                     int level = pSquad->getLevel();
                     pSquad->upgradeLevel(level + 1, false);
                  }
               }
            }
            mBonusSquadLevels = 1;
         
            break;
         }
         case cSK_AbilityRecharge: 
         {
            BProtoTechID protoTechID = cInvalidTechID;
            protoTechID = gDatabase.getProtoTech("Skull_10");
            BASSERT(protoTechID != cInvalidTechID);

            for (int i=1; i<gWorld->getNumberPlayers(); i++)
            {
               BPlayer*     pPlayer = gWorld->getPlayer(i);
               BTechTree*   pTech   = (pPlayer ? pPlayer->getTechTree() : NULL);
               BASSERT(pTech);
               if (pTech)
                  pTech->activateTech(protoTechID, cInvalidObjectID, true);
            }
            break;
         }
         case cSK_DeathExplode: 
         {
            mDeathExplodeChance = mod->mModifierValue;
            mDeathExplodeObjectType = gDatabase.getObjectType("Unit");
            mDeathExplodeProtoObject = gDatabase.getProtoObject("pow_cp_explode_01");
            break;
         }
         case cSK_TrainMod: 
         {
            BProtoTechID protoTechID = cInvalidTechID;
            protoTechID = gDatabase.getProtoTech("Skull_13");
            BASSERT(protoTechID != cInvalidTechID);

            for (int i=1; i<gWorld->getNumberPlayers(); i++)
            {
               BPlayer*     pPlayer = gWorld->getPlayer(i);
               BTechTree*   pTech   = (pPlayer ? pPlayer->getTechTree() : NULL);
               BASSERT(pTech);
               if (pTech)
                  pTech->activateTech(protoTechID, cInvalidObjectID, true);
            }
            break;
         }
         case cSK_SupplyMod: 
         {
            BProtoTechID protoTechID = cInvalidTechID;
            protoTechID = gDatabase.getProtoTech("Skull_14");
            BASSERT(protoTechID != cInvalidTechID);

            for (int i=1; i<gWorld->getNumberPlayers(); i++)
            {
               BPlayer*     pPlayer = gWorld->getPlayer(i);
               BTechTree*   pTech   = (pPlayer ? pPlayer->getTechTree() : NULL);
               BASSERT(pTech);
               if (pTech)
                  pTech->activateTech(protoTechID, cInvalidObjectID, true);
            }
            break;
         }
         case cSK_PowerRecharge: 
         {
            BProtoTechID protoTechID = cInvalidTechID;
            protoTechID = gDatabase.getProtoTech("Skull_15");
            BASSERT(protoTechID != cInvalidTechID);

            for (int i=1; i<gWorld->getNumberPlayers(); i++)
            {
               BPlayer*     pPlayer = gWorld->getPlayer(i);
               BTechTree*   pTech   = (pPlayer ? pPlayer->getTechTree() : NULL);
               BASSERT(pTech);
               if (pTech)
                  pTech->activateTech(protoTechID, cInvalidObjectID, true);
            }
            break;
         }
         case cSK_UnitModWarthog: 
         {
            BProtoTechID protoTechID = cInvalidTechID;
            protoTechID = gDatabase.getProtoTech("LCE_warthog_unlock");

            BASSERT(protoTechID != cInvalidTechID);

            BPlayer *pPlayer = gWorld->getPlayer(playerID);
            BASSERT(pPlayer);
            if (pPlayer == NULL)
               break;

            BTechTree *pTech = (pPlayer ? pPlayer->getTechTree() : NULL);
            BASSERT(pTech);
            if (pTech)
               pTech->activateTech(protoTechID, cInvalidObjectID, true);
            break;
      }
         case cSK_UnitModWraith: 
            {
               BProtoTechID protoTechID = cInvalidTechID;
               protoTechID = gDatabase.getProtoTech("LCE_wraith_unlock");

               BASSERT(protoTechID != cInvalidTechID);

               BPlayer *pPlayer = gWorld->getPlayer(playerID);
               BASSERT(pPlayer);
               if (pPlayer == NULL)
                  break;

               BTechTree *pTech = (pPlayer ? pPlayer->getTechTree() : NULL);
               BASSERT(pTech);
               if (pTech)
                  pTech->activateTech(protoTechID, cInvalidObjectID, true);
               break;
   }
      }

   }

   // [10/10/2008 xemu] ok, so if we have received this command, and we sent it, mark us as self active 
   if (playerID == gUserManager.getPrimaryUser()->getPlayerID())
   {
      skullDef->mSelfActive = active;
   }

   // [10/10/2008 xemu] mark it as UI active so that other people in the game see it properly 
   if (!skullDef->isOwnerOnly())
   {
      // if these are owner only, then we don't want to set these as active.
      skullDef->mUIActive = active;
      skullDef->mActive = active;
   }

   // [10/20/2008 xemu] ok, if we are getting this in the first N seconds of the game, then it is considered on from the "beginning"
   if (active && (gWorld->getGametimeFloat() < 60.0f))
      skullDef->mOnFromBeginning = true;
}

//==============================================================================
// BCollectiblesManager::getSkullActivationUI
//==============================================================================
bool BCollectiblesManager::getSkullActivationUI(int index) const
{
   if (index<0 || index>=mSkullDefinitions.getNumber())
      return false;

   // [10/10/2008 xemu] ok, for most skulls this is just UI active, but for self-only skulls, we care only about self activation
   if (mSkullDefinitions[index]->isOwnerOnly())
     return mSkullDefinitions[index]->mSelfUIActive;

   return mSkullDefinitions[index]->mUIActive;
}

//==============================================================================
// BCollectiblesManager::canToggleSkull
//==============================================================================
bool BCollectiblesManager::canToggleSkull(int index) const
{
   if (index<0 || index>=mSkullDefinitions.getNumber())
      return false;

   BProtoSkull *pSkull = mSkullDefinitions[index];
   if (pSkull == NULL)
   {
      BASSERT(0);
      return false;
   }

   // [10/9/2008 xemu] active skulls can't be toggled 
   // [10/10/2008 xemu] note that if we are owner only our definition of active is more narrow
   if (pSkull->checkActive(gUserManager.getPrimaryUser()->getPlayerID()))
      return(false);

   // [10/9/2008 xemu] nor can ones with sent commands
   if (pSkull->mCommandSent)
      return(false);

   // [10/9/2008 xemu] otherwise, OK
   return(true);
}

//==============================================================================
// BCollectiblesManager::clearSkullActivations
//==============================================================================
void BCollectiblesManager::clearSkullActivations()
{   
   for(int index=0; index<mSkullDefinitions.getNumber(); index++)
   {
      mSkullDefinitions[index]->mActive = false;
      mSkullDefinitions[index]->mUIActive = false;
      mSkullDefinitions[index]->mSelfActive = false;
      mSkullDefinitions[index]->mSelfUIActive = false;
      mSkullDefinitions[index]->mCommandSent = false;
      mSkullDefinitions[index]->mOnFromBeginning = false;

   }
   gScoreManager.resetSkullModifier();
   mbRocketAllGrunts = false;
   mbMinimapHidden = false;
   mBonusSquadLevels = 0;
   mDeathExplodeChance = 0;
   mDeathExplodeObjectType = -1;
   mDeathExplodeProtoObject = -1;
}

//==============================================================================
// BCollectiblesManager::areAllDebuffSkullsActive
//==============================================================================
bool BCollectiblesManager::areAllDebuffSkullsActive() const
{
   for (int i=0; i<mSkullDefinitions.getNumber(); i++)
   {
      // [10/20/2008 xemu] check skulls that are not activated, so see if they can disqualify us from this skull
      // [10/20/2008 xemu] specifically, they have to be activated in the first few moments of the game, they don't count 
      BProtoSkull* skullDef = mSkullDefinitions[i];  
      BASSERT(skullDef);
      if( !skullDef->mActive || !skullDef->mOnFromBeginning) 
      {
         // [10/20/2008 xemu] ok, so this is skull is either not on, or wasn't on from the start.  But was it a debuff skull? 

         for( int j=0; j<skullDef->mModifiers.getNumber(); j++ )  //check the modifiers
         {
            BSkullModifier *mod = skullDef->mModifiers.get(j);
            // [10/20/2008 xemu] note a "debuff" is a score enhancer, thus check for modifier values > 1.0 
            if(mod->mModifierType == cSK_Score && (mod->mModifierValue > 1.0))  //if it is a score debuff, return false since they aren't all active
               return false;
         }
      }
   }

   return true;
}


//==============================================================================
// BCollectiblesManager::reportBlackBoxCollected
//==============================================================================
void BCollectiblesManager::reportBlackBoxCollected(BUser *user, const BProtoObject* pTarget)
{
   BASSERT(pTarget);
   if (!pTarget)
      return;

   long objectdbid = pTarget->getDBID();

   long blackboxID = -1;
   for (int i=0; i<mTimeLineEventDefinitions.getNumber(); i++)
   {
      if( (mTimeLineEventDefinitions.get(i)->mEventType == cTLET_BlackBox) && (mTimeLineEventDefinitions.get(i)->mObjectDBID == objectdbid) )
      {
         blackboxID = i;
         break;
      }
   }

   if (blackboxID == -1)
      return;

   BASSERT(blackboxID<64 && blackboxID>=0);

   BUser* user2 = NULL;
   if (user == NULL)
   {
      user = gUserManager.getPrimaryUser();
      user2 = gUserManager.getSecondaryUser();
   }

   bool showMessage = false;
   if (user && user->getPlayer() && (user->getPlayer()->getPlayerState() == BPlayer::cPlayerStatePlaying))
   {
      BUserProfile *profile = user->getProfile();
      if (profile)
      {
         uint64 bits = profile->getTimeLineBits();
         bits |= (uint64(1) << blackboxID);
         profile->setTimeLineBits(bits);
      }
      showMessage=true;
      gAchievementManager.updateMisc(user, cMiscATTimeline);
   }
   if (user2 && user2->getPlayer() && (user2->getPlayer()->getPlayerState() == BPlayer::cPlayerStatePlaying) )
   {
      BUserProfile *profile = user2->getProfile();
      if (profile)
      {
         uint64 bits = profile->getTimeLineBits();
         bits |= (uint64(1) << blackboxID);
         profile->setTimeLineBits(bits);
      }
      showMessage=true;
      gAchievementManager.updateMisc(user2, cMiscATTimeline);
   }

   if (showMessage)
   {
      BUString message;
      message.locFormat(gDatabase.getLocStringFromID(25728), gDatabase.getLocStringFromIndex(pTarget->getDisplayNameIndex()).getPtr());
      user->addFlashUserMessage(message, "", true, 3.0f);
   }
}

//==============================================================================
// BCollectiblesManager::updateGame
//==============================================================================
void BCollectiblesManager::updateGame(BUser *pUser, BGameSettings* pSettings)
{
   BASSERT(pUser && pSettings);
   if(!pUser || !pSettings)
      return;

   BFixedStringMaxPath mapName;
   BString leaderName;

   //Get the current map name
   bool result = pSettings->getString(BGameSettings::cMapName, mapName, MAX_PATH);
   BASSERT(result);
   int pos = mapName.findRight('\\');
   if ((pos >= 0) && (pos < ((int)mapName.getLen()-1)))
      mapName.crop((uint)pos+1, mapName.getLen());

   BUserProfile *profile = pUser->getProfile();
   if (!profile)
      return;

   leaderName = pUser->getPlayer()->getLeader()->mName;

   uint64 bits = profile->getTimeLineBits();
   for (int i=0; i<mTimeLineEventDefinitions.getNumber(); i++)
   {
      if( (uint64(1)<<i) & bits )
         continue;  //we've already unlocked this

      if (mTimeLineEventDefinitions.get(i)->mEventType == cTLET_Leader)
      {
         if(mTimeLineEventDefinitions.get(i)->mLeaderName.compare(leaderName)==0)
         {
            bits |= (uint64(1) << i);
            profile->setTimeLineBits(bits);
            gAchievementManager.updateMisc(pUser, cMiscATTimeline);
         }
      }
      else if (mTimeLineEventDefinitions.get(i)->mEventType == cTLET_Map)
      {
         if(mTimeLineEventDefinitions.get(i)->mMapName.compare(mapName)==0)
         {
            bits |= (uint64(1) << i);
            profile->setTimeLineBits(bits);
            gAchievementManager.updateMisc(pUser, cMiscATTimeline);
         }
      }
   }
}

//==============================================================================
// BCollectiblesManager::hasTimeLineEventBeenUnlocked
//==============================================================================
bool BCollectiblesManager::hasTimeLineEventBeenUnlocked(BUser *pUser, const BString &name)
{
   BASSERT(pUser);
   if(!pUser)
      return false;
   BUserProfile *profile = pUser->getProfile();
   if (!profile)
      return false;

   uint64 bits = profile->getTimeLineBits();
   for (int i=0; i<mTimeLineEventDefinitions.getNumber(); i++)
   {
      if (mTimeLineEventDefinitions.get(i)->mEventName.compare(name)==0)
      {
         if( (uint64(1)<<i) & bits )
            return true;  
         else
            return false;
      }
   }
   
   gConsoleOutput.debug("Timeline event not found! %s", name.getPtr() );
   BASSERT(0);  //We couldn't find the Timeline event passed in
   return false;
}

//==============================================================================
// BCollectiblesManager::haveNewTimeLineEventsBeenUnlocked
//==============================================================================
bool BCollectiblesManager::haveNewTimeLineEventsBeenUnlocked(BUser *pUser)
{
   BASSERT(pUser);
   if(!pUser)
      return false;
   BUserProfile *profile = pUser->getProfile();
   if (!profile)
      return false;

   uint64 bits = profile->getTimeLineBits();
   uint64 bitsVisited = profile->getTimeLineVisitedBits();
   
   return (bits != bitsVisited);
}


//==============================================================================
// BCollectiblesManager::hasTimeLineEventBeenSeen
//==============================================================================
bool BCollectiblesManager::hasTimeLineEventBeenSeen(BUser *pUser, const BString &name)
{
   BASSERT(pUser);
   if(!pUser)
      return false;
   BUserProfile *profile = pUser->getProfile();
   if (!profile)
      return false;

   uint64 bits = profile->getTimeLineBits();
   uint64 visitedBits = profile->getTimeLineVisitedBits();
   for (int i=0; i<mTimeLineEventDefinitions.getNumber(); i++)
   {
      if (mTimeLineEventDefinitions.get(i)->mEventName.compare(name)==0)
      {
         if( (uint64(1)<<i) & (bits ^ visitedBits) )
            return true;  
         else
            return false;
      }
   }
   
   gConsoleOutput.debug("Timeline event not found! %s", name.getPtr() );
   BASSERT(0);  //We couldn't find the Timeline event passed in
   return false;
}

//==============================================================================
// BCollectiblesManager::markAllTimeLineEventsSeen
//==============================================================================
bool BCollectiblesManager::markAllTimeLineEventsSeen(BUser *pUser)
{
   BASSERT(pUser);
   if(!pUser)
      return false;
   BUserProfile *profile = pUser->getProfile();
   if (!profile)
      return false;

   uint64 bits = profile->getTimeLineBits();
   
   profile->setTimeLineVisitedBits(bits);
   return (true);
}

//==============================================================================
// BCollectiblesManager::getNumTimelineEventsUnlocked
//==============================================================================
int BCollectiblesManager::getNumTimelineEventsUnlocked(BUser *pUser)
{
   BASSERT(pUser);
   if(!pUser)
      return 0;
   BUserProfile *profile = pUser->getProfile();
   if (!profile)
      return 0;
   uint64 bits = profile->getTimeLineBits();

   int i;
   int count = 0;
   int maxTimeLineEvents = mTimeLineEventDefinitions.getNumber();
   for (i=0; i < maxTimeLineEvents; i++)
   {
      if (bits & (uint64(1) << i))
      {
         count++;
      }
   }
   return(count);
}

//============================================================================
//============================================================================
void BCollectiblesManager::setupTickerInfo() 
{
   BUString tempString = "";
   BUser* pUser = gUserManager.getPrimaryUser();
   if(!pUser)
      return;

   BUserProfile *pProfile = pUser->getProfile();
   if( pProfile )
   {
      //Time spent playing
      {
         long seconds = pProfile->getTotalGameTime();
         long hours = seconds / 3600;
         seconds -= (hours * 3600);
         long minutes = seconds / 60;
         seconds -= (minutes * 60);

         tempString.locFormat(gDatabase.getLocStringFromID(25706), hours, minutes, seconds);
         BUITicker::addString(tempString, 3, -1, BUITicker::eTimePlayed);
      }
   }

   //Skulls collected
   tempString.locFormat(gDatabase.getLocStringFromID(25707), getNumSkullsCollected(pUser, false, false), getNumberSkulls(false));
   BUITicker::addString(tempString, 3, -1, BUITicker::eNumSkulls);

   //Timeline events unlocked
   tempString.locFormat(gDatabase.getLocStringFromID(25708), getNumTimelineEventsUnlocked(pUser), mTimeLineEventDefinitions.getNumber());
   BUITicker::addString(tempString, 3, -1, BUITicker::eNumTimelineEvents);

   if(haveNewTimeLineEventsBeenUnlocked(pUser))
      BUITicker::addString(gDatabase.getLocStringFromID(25709), 3, -1, BUITicker::eNewTimelineEvents);
   else
      BUITicker::addString(gDatabase.getLocStringFromID(25709), 3, 0, BUITicker::eNewTimelineEvents);
}


#ifndef BUILD_FINAL
//==============================================================================
// BCollectiblesManager::overrideSkullsCollected
//==============================================================================
void BCollectiblesManager::overrideSkullsCollected(BUser *user, uint32 bits)
{
   BUser* user2 = NULL;
   if (user == NULL)
   {
      user = gUserManager.getPrimaryUser();
      user2 = gUserManager.getSecondaryUser();
   }

   if (user && user->getPlayer() && (user->getPlayer()->getPlayerState() == BPlayer::cPlayerStatePlaying))
   {
      BUserProfile *profile = user->getProfile();
      if (profile)
      {
         profile->setSkullBits(bits);
      }
   }
   if (user2 && user2->getPlayer() && (user2->getPlayer()->getPlayerState() == BPlayer::cPlayerStatePlaying) )
   {
      BUserProfile *profile = user2->getProfile();
      if (profile)
      {
         profile->setSkullBits(bits);
      }
   }
}

//==============================================================================
// BCollectiblesManager::overrideBlackBoxesCollected
//==============================================================================
void BCollectiblesManager::overrideTimeLineEventsCollected(BUser *user, long type, bool collected)
{
   BUser* user2 = NULL;
   if (user == NULL)
   {
      user = gUserManager.getPrimaryUser();
      user2 = gUserManager.getSecondaryUser();
   }

   for(int j=0; j<2; j++)
   {
      BUser* pUser = user;
      if( j==1)
         pUser = user2;

      // [10/20/2008 xemu] disabled the "player state playing" test since it prevented pregame testing 
      if (pUser) // && pUser->getPlayer()) && (pUser->getPlayer()->getPlayerState() == BPlayer::cPlayerStatePlaying))
      {
         BUserProfile *profile = pUser->getProfile();
         if (profile)
         {
            uint64 bits = profile->getTimeLineBits();
            for(int i=0; i<mTimeLineEventDefinitions.getNumber(); i++)
            {
               if(mTimeLineEventDefinitions.get(i)->mEventType == type)
               {
                  if (collected)
                     bits |= (uint64(1) << i);
                  else
                     bits &= ~(uint64(1) << i); // tilde is the bitwise inverse.
               }
            }
            profile->setTimeLineBits(bits);
            // [10/3/2008 xemu] not sure about this, if this is only a dev override cmd shouldn't trigger achievements? 
            //gAchievementManager.updateMisc(pUser, cMiscATTimeline);
         }
      }
   }
}
#endif

//==============================================================================
// BCollectiblesManager::isSkullHidden
//==============================================================================
bool BCollectiblesManager::isSkullHidden(int index) const
{
   if ((index < 0) || (index >= mSkullDefinitions.getNumber()))
      return(false);

   BProtoSkull *pSkull = mSkullDefinitions[index];
   if (pSkull == NULL)
      return(false);

   return(pSkull->mHidden);
}

//==============================================================================
//==============================================================================
void BCollectiblesManager::unlockHiddenSkulls()
{
   // [10/8/2008 xemu] @FIXME -- change this to look in the marketplace content files once we have the real unlocking semaphore files 

   HANDLE hEnum = INVALID_HANDLE_VALUE;

   XCONTENT_DATA contentData[5];
   DWORD cbBuffer = 0;
   
   BUser *pUser = gUserManager.getPrimaryUser();
   int userIndex = pUser->getPort();
   DWORD retval = XContentCreateEnumerator(userIndex, XCONTENTDEVICE_ANY, XCONTENTTYPE_MARKETPLACE, 0, 5, &cbBuffer, &hEnum);
   if (retval != ERROR_SUCCESS) 
   {
      if (hEnum != INVALID_HANDLE_VALUE)
         CloseHandle(hEnum);
      return;
   }

   DWORD dwReturnCount;
   do 
   {
      DWORD dwRet = XEnumerate(hEnum, contentData, sizeof(contentData), &dwReturnCount, NULL);
      if (dwRet == ERROR_SUCCESS)
      {
         for (uint i=0; i < dwReturnCount; ++i)
         {
            // Mount the Package
            DWORD dwDisposition = 0;
            DWORD licenseMask = 0;
            DWORD dwContentFlags = XCONTENTFLAG_OPENEXISTING;
            DWORD dwDLCRet = XContentCreate(userIndex, "hwdlc", &contentData[i], dwContentFlags, &dwDisposition, &licenseMask, NULL);
            if (dwDLCRet != ERROR_SUCCESS)
               continue;

            bool unlock;
            int skullUnlockIndex;
            if (getUnlockData(unlock, skullUnlockIndex))
            {
               if (unlock)
                  unhideSkull(pUser, skullUnlockIndex);
            }

            // Unmount the package
            XContentClose("hwdlc", NULL);
         }
      }
   } while (dwReturnCount == 5);                  

   if (hEnum != INVALID_HANDLE_VALUE)
      CloseHandle(hEnum);

#ifndef BUILD_FINAL
   if (gConfig.isDefined("UnlockHiddenSkull-UnitModWarthog"))
   {
      unhideSkull(gUserManager.getPrimaryUser(), 15);
   }
   if (gConfig.isDefined("UnlockHiddenSkull-UnitModWraith"))
   {
      unhideSkull(gUserManager.getPrimaryUser(), 16);
   }
#endif

}

//==============================================================================
// BCollectiblesManager::unhideSkull
//==============================================================================
void BCollectiblesManager::unhideSkull(BUser *pUser, int skullUnlockIndex)
{
   BProtoSkull *pSkull = mSkullDefinitions[skullUnlockIndex];
   BASSERT(pSkull);
   if (pSkull)
   {
      // [10/8/2008 xemu] un-hide and "collect" the skull 
      pSkull->mHidden = false;
      uint32 skullbits = pUser->getProfile()->getSkullBits();
      skullbits = skullbits | (1 << skullUnlockIndex);
      pUser->getProfile()->setSkullBits(skullbits);
   }
}


//==============================================================================
// BCollectiblesManager::getUnlockData
//==============================================================================
bool BCollectiblesManager::getUnlockData(bool &unlock, int &skullUnlockIndex)
{
   unlock = false;
   skullUnlockIndex = -1;

   BWin32FileStream* pStream = new BWin32FileStream();

   BString filename;
   filename.format("hwdlc:\\hwPdlcUnlock.txt");
   uint flags = cSFReadable | cSFOpenExisting;

   if (!pStream->open(filename.getPtr(), flags, &gWin32LowLevelFileIO))
   {
      delete pStream;
      return false;
   }

   BString unlockText;
   if (!pStream->readLine(unlockText))
   {
      pStream->close();
      delete pStream;
      return false;
   }

   pStream->close();
   delete pStream;


   int i;
   for (i=0; i < mSkullDefinitions.getNumber(); i++)
   {
      BProtoSkull *pSkull = mSkullDefinitions[i];
      if (pSkull->mName.compare(unlockText) == 0)
      {
         skullUnlockIndex = i;
         unlock = true;
         break;
      }
   }

   return true;
}

//==============================================================================
// BCollectiblesManager::getScoreModifierUI
//==============================================================================
float BCollectiblesManager::getScoreModifierUI()
{
   // [10/9/2008 xemu] go through all the *UI* activated skulls and show our hypothetical score modifier
   float finalScoreMod = 1.0f;

   int i;
   for (i=0; i < mSkullDefinitions.getNumber(); i++)
   {
      BProtoSkull *pSkull = mSkullDefinitions[i];
      if (pSkull == NULL)
      {
         BASSERT(0);
         continue;
      }

      if (!pSkull->mUIActive)
         continue;

      int j;
      for (j = 0; j < pSkull->mModifiers.getNumber(); j++)
      {
         BSkullModifier *pMod = pSkull->mModifiers[j];
         if (pMod == NULL)
         {
            BASSERT(0);
            continue;
         }

         if (pMod->mModifierType == cSK_Score)
         {
            finalScoreMod = finalScoreMod * pMod->mModifierValue;
         }
      }
   }

   return(finalScoreMod);
}

//============================================================================
//============================================================================
bool BCollectiblesManager::save(BStream* pStream, int saveType) const
{
   GFWRITEVAR(pStream, bool, mbRocketAllGrunts);
   GFWRITEVAR(pStream, bool, mbMinimapHidden);
   GFWRITEVAR(pStream, int, mBonusSquadLevels);

   GFWRITEVAR(pStream, float, mDeathExplodeChance);
   GFWRITEVAR(pStream, int, mDeathExplodeObjectType);
   GFWRITEVAR(pStream, long, mDeathExplodeProtoObject);

   // [10/20/2008 xemu] write out activation state of all the skulls
   int i;
   for (i=0; i < mSkullDefinitions.getNumber(); i++)
   {
      BProtoSkull *pSkull = mSkullDefinitions[i];
      // [10/20/2008 xemu] if we have a bad skull here we're screwed for saving, really
      BASSERT(pSkull); 
      // [10/20/2008 xemu] these three skull bits are what are truly persistent, rest is all static or transitory 
      // [10/20/2008 xemu] kind of sucks that both types are mixed together, but not changing that now... 
      GFWRITEBITBOOL(pStream, pSkull->mActive);
      GFWRITEBITBOOL(pStream, pSkull->mSelfActive);
      GFWRITEBITBOOL(pStream, pSkull->mOnFromBeginning);
   }

   return true;
}

//============================================================================
//============================================================================
bool BCollectiblesManager::load(BStream* pStream, int saveType)
{
   GFREADVAR(pStream, bool, mbRocketAllGrunts);
   GFREADVAR(pStream, bool, mbMinimapHidden);
   GFREADVAR(pStream, int, mBonusSquadLevels);

   GFREADVAR(pStream, float, mDeathExplodeChance);
   GFREADVAR(pStream, int, mDeathExplodeObjectType);
   GFREADVAR(pStream, long, mDeathExplodeProtoObject);

   // [10/20/2008 xemu] write out activation state of all the skulls
   int i;
   for (i=0; i < mSkullDefinitions.getNumber(); i++)
   {
      BProtoSkull *pSkull = mSkullDefinitions[i];
      BASSERT(pSkull); 
      // [10/20/2008 xemu] these three skull bits are what are truly persistent, rest is all static or transitory 
      GFREADBITBOOL(pStream, pSkull->mActive);
      GFREADBITBOOL(pStream, pSkull->mSelfActive);
      GFREADBITBOOL(pStream, pSkull->mOnFromBeginning);

      // [10/20/2008 xemu] infer a little bit of other state, for UI consistency
      pSkull->mUIActive = pSkull->mActive;
      pSkull->mSelfUIActive = pSkull->mSelfActive;
   }


   // [10/20/2008 xemu] handle remapping
   gSaveGame.remapObjectType(mDeathExplodeObjectType);
   gSaveGame.remapProtoObjectID(mDeathExplodeProtoObject);

   // [10/20/2008 xemu] re-apply some skulls that effectively don't save their state
   if (mbMinimapHidden)
   {
      gUIManager->setMinimapVisible(false);
   }

   return true;
}


//==============================================================================
