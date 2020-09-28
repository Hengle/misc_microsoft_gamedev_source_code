date
//==============================================================================
// aifactoid.cpp
//
// Copyright (c) 2008 Ensemble Studios
//==============================================================================
 

// xgame
#include "common.h"
#include "aidebug.h"
#include "aifactoid.h"
#include "gamedirectories.h"
#include "player.h"
#include "random.h"
#include "uimanager.h"
#include "user.h"
#include "userprofilemanager.h"
#include "world.h"
#include "worldsoundmanager.h"

//==============================================================================
//==============================================================================
BAIFactoidEvent::BAIFactoidEvent()
{
   mFromPlayer = -1;
   mFactoidID = -1;         
   mPriority = -1;           
   mName.empty();                  
   mTimeIn = 0;                
   mTimeExpires = 0;      
   mDestLoc = cInvalidVector;              
   mStartLoc = cInvalidVector;             
   mFlareArrow = false;         
}


//==============================================================================
//==============================================================================
void BAIFactoidEvent::clear()
{
   mFromPlayer = -1;
   mFactoidID = -1;         
   mPriority = -1;           
   mName.empty();                  
   mTimeIn = 0;                
   mTimeExpires = 0;      
   mDestLoc = cInvalidVector;               
   mStartLoc = cInvalidVector;             
   mFlareArrow = false;         
}

//==============================================================================
//==============================================================================
BAIFactoidEvent::BAIFactoidEvent(BPlayerID from, long factoidID, long priority, const BString& pName, DWORD timeIn, DWORD timeExpires, BVector dest, BVector start, bool showArrow)
{
   mFromPlayer = from;              // Who sent this?
   mFactoidID = factoidID;          // Which specific event is this?
   mPriority = priority;            // How important/urgent?
   mName = pName;                   // Text name, mostly for debugging
   mTimeIn = timeIn;                // Game time (ms) when factoid was submitted
   mTimeExpires = timeExpires;      // Game time (ms) when this message will expire if not played.
   mDestLoc = dest;                 // Optional destination indicator for special minimap flare
   mStartLoc = start;               // Optional source location
   mFlareArrow = showArrow;         // Optionally show arrow from source to dest
}



//==============================================================================
//==============================================================================
BAIFactoid::BAIFactoid()
{
   mFactoidID = -1;
   mName.empty();
   mDefPriority = -1;
   mDefExpiration = 0;
}


//==============================================================================
//==============================================================================
BAIFactoidLine::BAIFactoidLine(long factoidID, const BString& mName)
{
   mFactoidID = factoidID;
   mFileName.empty();
   mFileName.append(mName);
   mUseCount = 0;
   mLastPlayTime = 0;
}


//==============================================================================
//==============================================================================
void BAIFactoidManager::setPlayerID(BPlayerID playerID)
{
   mPlayerID = playerID;
}


//==============================================================================
//==============================================================================
BAIFactoidManager::BAIFactoidManager(BPlayerID pID)
{
   mFactoids.empty();
   mFactoidLines.empty();
   mLastPlayTime = 0;
   mMinPriority = cFactoidPriorityTrivial;
   mLastLine.empty();
   mLineBeforeLast.empty();

   mLastUpdate = 0;

   uint pri = 0;
   for (pri = 0; pri < cFactoidNumPriorities; pri++)
      mFactoidEvents[pri].mFactoidID = -1;   // Invalid/empty

   // Read in the xml file, and populate the Factoids list and Factoid lines.
   BXMLReader factoidFileReader;
   if (!factoidFileReader.load(cDirData, "aifactoids.xml"))
   {
      BASSERTM(false, "Could not load aifactoids.xml");
   }

   BXMLNode factoidFileRoot(factoidFileReader.getRootNode());
   if (factoidFileRoot.getName() != "Factoids")
   {
      BASSERTM(false, "Could not get root node 'Factoids'.");
   }

   uint numFactoidNodes = static_cast<uint>(factoidFileRoot.getNumberChildren());
   mFactoidLines.reserve(numFactoidNodes);

   uint factoidCount = 0;
   BString previousEvent;
   previousEvent.empty();
   BString currentEvent;
   currentEvent.empty();
   long currentPri = -1;
   BString currentFile;
   currentFile.empty();
   BString currentLine;
   currentLine.empty();
   BString civName;
   civName.empty();

   BPlayer* pPlayer = gWorld->getPlayer(pID);
   BASSERT(pPlayer);
   

   bool isUNSC = false;
   if ( pPlayer->getCivID() == gDatabase.getCivID("UNSC") )
      isUNSC = true;

   for (uint lineCount=0; lineCount<numFactoidNodes; lineCount++)
   {
      BXMLNode factoidNode(factoidFileRoot.getChild(lineCount));
      if (factoidNode.getName() != "Factoid")
      {
         BASSERTM(false, "Could not load node Factoid.");
         continue;
      }
      // Format:  <Factoid Event="AttackIncomingGeneral"  defPri="3" File="unsc_coach_001"  Line="Enemy forces inbound."/>	   

      // Load the Factoid info
      factoidNode.getAttribValueAsString("Event", currentEvent);
      factoidNode.getAttribValueAsLong("defPri", currentPri);
      factoidNode.getAttribValueAsString("File", currentFile);
      factoidNode.getAttribValueAsString("Line", currentLine);
      factoidNode.getAttribValueAsString("Civ", civName);

      if ( civName.compare("UNSC") && isUNSC) 
         continue;   // This is a non-UNSC line, and I'm UNSC, so ignore it.
      if ( !civName.compare("UNSC") && !isUNSC )
         continue;   // This is a UNSC line, and I'm not UNSC, so ignore it.

      // If this is a new Event (different from previous line), add it to the factoid table.
      if( currentEvent.compare(previousEvent) )
      {
         BAIFactoid& factoid = mFactoids.grow();

         factoid.setFactoidID(factoidCount);
         factoid.setName(currentEvent);
         factoid.setDefPriority(currentPri);
         factoid.setDefExpiration(10000);

         previousEvent.copy(currentEvent);
         factoidCount++;
      }  


      // Process this line
      BAIFactoidLine& factoidLine = mFactoidLines.grow();

      factoidLine.setFactoidID(factoidCount - 1);
      factoidLine.setFileName(&currentFile);
      factoidLine.setUseCount(getRandRange(cSimRand, 0, 5));   // Add a bit of fuzziness to randomize first pick
      factoidLine.setLastPlayTime((DWORD)0);
      
   }
}


//==============================================================================
//==============================================================================
void BAIFactoidManager::update()
{
   const uint cMinInterval = 500;      // milliseconds between update checks

   if (gWorld->getGametime() < mLastUpdate)
      mLastUpdate = gWorld->getGametime();
   if ((gWorld->getGametime() - mLastUpdate) < cMinInterval)
      return;     

   mLastUpdate = gWorld->getGametime();



   // Compute the minimum priority that is eligible to play right now. 
   // Emergency priority always plays, lowest priority is only for use when things have been quiet.
   // Uses a simple formula to maintain a floating average of recent 'chattiness'.  
   //static float chattiness = 0.0f;
   //const float cChattyFactor = 0.1f;  // Higher numbers = faster decay = more chatting.
   //const float cMaxChattiness = 50.0f; // This limits how high our chattiness rolling average can get.
   //const float cSingleChatWeight = 0.50f; // How much each chat raises chattiness, as a percentage of
   // the range from the current level to the max.  Higher values
   // mean less chatting.


   //if (chattiness > 0.0f)
   //   chattiness -= (chattiness * cChattyFactor) * (cMinInterval / 1000.0f);  // Decay about 1% per second


   // If a file is playing, and we don't have an emergency-priority message waiting, bail
   


   // Priorities map to intervals using the following formula, with P for priority:
   // (P+2)^2 = maximum chattiness to permit this priority to be played.   
   // P = 4 means a chattiness of 36 would suppress it.  P = 2 would be suppressed by a chattiness of 16.  



   DWORD bar;
   for (mMinPriority = cFactoidPriorityTrivial; mMinPriority <= cFactoidPriorityEmergency; mMinPriority++)
   {
//      bar = (mMinPriority + 2) * (mMinPriority + 2);
      bar = 10000 * (cFactoidPriorityEmergency - mMinPriority);  // 0 at top pri, 10 sec per level
//      if (chattiness < bar)
//         break;
      if ( (gWorld->getGametime()- mLastPlayTime) > bar )
         break;
   }

   //if (mLastPlayTime < 0)  // Just started
   ///   mMinPriority = cFactoidPriorityTrivial;

   // Check the list of queued chats.  If we have one that is of a priority >= minPriority, play it.

   long testPriority;
   long playMe = -1;
   for (testPriority = cFactoidPriorityEmergency; testPriority >= cFactoidPriorityTrivial; testPriority--)
   {
      if ( (mFactoidEvents[testPriority].mFactoidID >= 0) && (testPriority >= mMinPriority) )
      {
         if (mFactoidEvents[testPriority].mTimeExpires > gWorld->getGametime())
            playMe = testPriority;           
         else
            mFactoidEvents[testPriority].clear();
      }
      if (playMe >= 0)
         break;
   }
   if ( ((gWorld->getGametime() - mLastPlayTime) < 5000 ) && (testPriority < cFactoidPriorityEmergency) )
      return;  // Never play twice within 5 seconds...except for emergencies.

   if (playMe >= 0)
   {
      //chattiness += (cMaxChattiness - chattiness) * cSingleChatWeight;

      // Select a file and play it.
      BAIFactoidLine* pLine = chooseFactoidLine(mFactoidEvents[playMe].mFactoidID);
      if (pLine)
      {
         BSimString fileName = pLine->getFileName();
         if (gUserManager.getPrimaryUser()->getPlayerID() == mPlayerID)
         {
            gSoundManager.playCue(fileName);
            pLine->setUseCount(pLine->getUseCount() + 10);  // Incrementing by 10s to overwhelm initial 0..5 randomness
            pLine->setLastPlayTime(gWorld->getGametime());
            mLastPlayTime = gWorld->getGametime();
            mLineBeforeLast.set(mLastLine);
            mLastLine.set(pLine->getFileName());
            mLastLine.append(" ");
            mLastLine.append(mFactoidEvents[playMe].mName);
         }
      }
      // Process the alerts

      BVector offsetDest = mFactoidEvents[playMe].mDestLoc;
      if (offsetDest.x > 30.0f)
         offsetDest.x = offsetDest.x - 30.0f; // Avoid over-writing player's flare


      if (gUserManager.getPrimaryUser()->getPlayerID() == mPlayerID)
      {
         if ( ! mFactoidEvents[playMe].mDestLoc.almostEqual(cInvalidVector) )
            gUIManager->addMinimapFlare(offsetDest, mFactoidEvents[playMe].mFromPlayer, BUIGame::cFlareLook);
         else  // Only do source flare if there was no dest
            if ( ! mFactoidEvents[playMe].mStartLoc.almostEqual(cInvalidVector) )
               gUIManager->addMinimapFlare(mFactoidEvents[playMe].mStartLoc, mFactoidEvents[playMe].mFromPlayer, BUIGame::cFlareLook);  // cFlareLook,cFlareHelp,cFlareMeet,cFlareAttack,
      }



      // Clear the record
      mFactoidEvents[playMe].clear();
   }
}


//==============================================================================
//==============================================================================
BAIFactoidLine* BAIFactoidManager::chooseFactoidLine(long factoidID)
{
   long i = 0;
   long numFound = 0;
   long firstFound = -1;

   for (i=0; i<mFactoidLines.getNumber(); i++)
   {
      if (mFactoidLines[i].getFactoidID() == factoidID)
      {
         if (firstFound < 0)
            firstFound = i;
         numFound++;
      }
      else
         if (numFound > 0)
            break;
   }

   long minPlays = -1;
   BAIFactoidLine* toPlay = NULL;
   for (i=firstFound; i < firstFound + numFound; i++)
   {
      if ( (minPlays < 0) || (mFactoidLines[i].getUseCount() < minPlays) )
      {
         minPlays = mFactoidLines[i].getUseCount();
         toPlay = &mFactoidLines[i];
      }
   }

   return(toPlay);
}

//==============================================================================
//==============================================================================
void BAIFactoidManager::submitAIFactoidEvent(BPlayerID from, const BString& eventName, long priority, DWORD deadline, BVector dest, BVector source, bool showArrow)
{
   // Use eventName to get an ID for this event.
   uint eventCount = 0;
   long eventID = -1;

   // Do not process factoids if no user, or if AIAdvisor is off
   BPlayer* pPlayer = gWorld->getPlayer(mPlayerID);
   BASSERT(pPlayer);
   BUser* pUser = pPlayer->getUser();
   if (!pUser)
      return;
   BUserProfile *profile = pUser->getProfile();
   if (!profile)
      return;
   if (!profile->getOption_AIAdvisorEnabled())
      return;

   for (eventCount = 0; eventCount < mFactoids.getSize(); eventCount++)
   {
      //BString temp; 
      //mFactoids[eventCount].getName()->copy(temp);
      //if (! eventName->compare(temp) )
      if ( eventName == mFactoids[eventCount].getName() )
      {
         eventID = mFactoids[eventCount].getFactoidID();
         break;
      }
   }

//   BASSERTM(eventID != -1, eventName);
   if (eventID < 0)
      return;

   // The mFactoidEvents array holds one factoid for each priority level.  Earlier ones are overwritten if a new arrival comes in.
   // Priority is used as an index into mFactoidEvents.

   // Set the start, end times, and priority. 
   long actualPriority = mFactoids[eventID].getDefPriority(); // Start with default priority
   if (priority >= 0)
      actualPriority = priority; // override with passed-in priority, use this as index into mFactoidEvents[].

   mFactoidEvents[actualPriority].mPriority = actualPriority;

   mFactoidEvents[actualPriority].mTimeIn = gWorld->getGametime();
   
   if (deadline <= 0)
      mFactoidEvents[actualPriority].mTimeExpires = mFactoidEvents[actualPriority].mTimeIn + 10000;
   else
      mFactoidEvents[actualPriority].mTimeExpires = mFactoidEvents[actualPriority].mTimeIn + deadline;

   // Get the text description of the event
   mFactoidEvents[actualPriority].mFactoidID = eventID;
   mFactoidEvents[actualPriority].mName = eventName;

   mFactoidEvents[actualPriority].mFromPlayer = from;

   // Set the vectors and the arrow flag.
   mFactoidEvents[actualPriority].mDestLoc = dest;
   mFactoidEvents[actualPriority].mStartLoc = source;
   mFactoidEvents[actualPriority].mFlareArrow = showArrow;

   // (Do not select a file yet, that only happens if it gets played.
}