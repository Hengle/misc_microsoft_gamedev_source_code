//==============================================================================
// aifactoid.h
//
// Copyright (c) 2008 Ensemble Studios
//==============================================================================
#pragma once

// xgame includes
#include "aitypes.h"

 
// forward declarations




//==============================================================================
// BAIFactoidEvent
//
// A 'factoid' is anything in the game that might be worthy of a mention by
// the AI 'coach' voice that assists human players.   A note about enemy actions,
// an ally's response to a command, notes about game status, all qualify.
//
// A FactoidEvent is like a telegram to the human player's coach, notifying it that
// a condition has occurred and may be worth talking about.
//
// The name was chosen to avoid confusion with 'chat' and 'hint' and 'tutorial' names.
//==============================================================================
class BAIFactoidEvent
{
public:
   BAIFactoidEvent();
   ~BAIFactoidEvent(){};
   BAIFactoidEvent(BPlayerID from, long factoidID, long prioirity, const BString& pName, DWORD timeIn, DWORD timeExpires, BVector dest, BVector start, bool showArrow);
   void clear();

   // All public
   BPlayerID   mFromPlayer;   // Who sent this?
   long        mFactoidID;    // Which specific event is this?
   long        mPriority;     // How important is this?
   BString     mName;         // Text name, mostly for debugging
   DWORD       mTimeIn;       // Game time (ms) when factoid was submitted
   DWORD       mTimeExpires;  // Game time (ms) when this message will expire if not played.
   BVector     mDestLoc;      // Optional destination indicator for special minimap flare
   BVector     mStartLoc;     // Optional source location
   bool        mFlareArrow;   // Optional flag to draw arrow on minimap from start to dest.

};

//==============================================================================
// BAIFactoid
//
// Each factoid represents an observation that can be made about a game, such as an attack
// being launched, or a clash between specific unit types, or certain strategies 
// being appropriate, etc.    
//
// Each factoid has an ID to uniquely identify the particular event or situation that has
// occurred, i.e. enemy is launching an attack on Base X.  There may be several messages
// that can be played in response to a given event, i.e. four variations on 'incoming attack
// at this location!'.
//
// 
// 
//==============================================================================
class BAIFactoid
{
public:
   BAIFactoid();
   ~BAIFactoid(){};
   void setFactoidID(long fID){mFactoidID = fID;}
   long getFactoidID() { return mFactoidID; }
   void setName(const BString& name) { mName = name; }
   const BString& getName() { return mName; }
   void setDefPriority(long pri) { mDefPriority = pri; }
   long getDefPriority() { return mDefPriority; }
   void setDefExpiration(DWORD exp) { mDefExpiration = exp; }

protected:
   long mFactoidID;        // Unique identifier for the situation that has occurred, i.e. grunts just got owned by flamers
   BString mName;          // Text description, for debugging
   long mDefPriority;       // Usual priority for this factoid
   DWORD mDefExpiration;    // Usual delay for expiration, i.e. add this to the queued time to set the expiration time.
};


//==============================================================================
// BAIFactoidLine
//
// One line per recorded sound file, so there may be multiple FactoidLines for
// a given Factoid.   This list is used to choose which line to play, and also
// to track which lines have been played, and how recently they've been played.
//==============================================================================
class BAIFactoidLine
{
public:
   BAIFactoidLine(){} 
   BAIFactoidLine(long mFactoidID, const BString& mName);
   ~BAIFactoidLine(){}
   void setFactoidID(long fID){mFactoidID = fID;}
   long getFactoidID() { return mFactoidID; }
   void setFileName(BString* name) { name->asBCHAR(mFileName); }
   const BString& getFileName() { return mFileName; }
   void setUseCount(long count) { mUseCount = count; }
   long getUseCount() { return mUseCount; }
   void setLastPlayTime(DWORD time) { mLastPlayTime = time; }
   DWORD getLastPlayTime() { return mLastPlayTime; }


protected:
   long mFactoidID;        // Which factoid do I belong to?
   BString mFileName;      // Audio file to play
   long mUseCount;         // Times played this game
   DWORD mLastPlayTime;
};

enum {
   cFactoidPriorityTrivial = 0,
   cFactoidPriorityLow = 1,
   cFactoidPriorityMedium = 2,
   cFactoidPriorityHigh = 3,
   cFactoidPriorityUrgent = 4,
   cFactoidPriorityEmergency = 5,
   cFactoidNumPriorities,
};

//==============================================================================
// BAIFactoidManager
//
// One is created for each human player, and is accessed via a pointer on BPlayer.
// Holds the queue of pending factoids, and decides which to play, when.
//==============================================================================
class BAIFactoidManager
{
public:
   BAIFactoidManager(BPlayerID pID);
   ~BAIFactoidManager(){};

   void update();

   void setPlayerID(BPlayerID playerID);     // Which player owns me?
   void submitAIFactoidEvent(BPlayerID from, const BString& eventName, long priority, DWORD deadline, BVector dest, BVector source, bool showArrow);

   BAIFactoidLine* chooseFactoidLine(long factoidID);
   DWORD getLastPlayTime() const { return (mLastPlayTime); }
   const BAIFactoidEvent* getFactoidEvent(uint pri) { if (pri >= 1 && pri <= 5) return (&mFactoidEvents[pri]); return (NULL); }
   long getMinPriority() const { return (mMinPriority); }
   BString mLastLine;
   BString mLineBeforeLast;

protected:
   BPlayerID mPlayerID;
   BAIFactoidEvent mFactoidEvents[cFactoidNumPriorities];   // The queue of factoids that could be played (time permitting) for this player.
   BDynamicSimArray<BAIFactoid> mFactoids;              // A look-up table for the types of factoids that can occur
   BDynamicSimArray<BAIFactoidLine> mFactoidLines;          // A look-up table for the lines that can be chatted when an FactoidEvent occurs.
   DWORD mLastPlayTime;
   long mMinPriority;

   DWORD mLastUpdate;
};