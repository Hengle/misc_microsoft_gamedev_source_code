//============================================================================
// UITicker.h
// Ensemble Studios (c) 2008
//============================================================================
#pragma once
#include "xcore.h"
#include "UIScreen.h"
#include "campaignprogress.h"
#include "userprofilemanager.h"

const int cMaxGamerTagTickers = 10;


class BUITicker : public BUIScreen
{

public:

   enum 
   { 
      cDefaultScrollSpeed = 75,
      cDefaultSpacing = 50,
   };

   enum ticker_ids
   {
      eStart = 1000,
      eFriendGamerTag,
      eFriendsOnline = eFriendGamerTag + cMaxGamerTagTickers,
      eFriendsPlayingHW, 
      eTimePlayed,
      eNumTimelineEvents,
      eNewTimelineEvents,
      eNumSkulls,
      eMedalsEarned,
      eNumParMet = eMedalsEarned + (CAMPAIGN_MAX_MODES * CAMPAIGN_DIFFICULTY_LEVELS),
      eLifetimeCampaignScore,
      eNumAchievements,
      eLastAchievement,

      eRankedLeaderGames,
      eRankedMapGames = eRankedLeaderGames + SR_MAX_NUM_LEADERS,
      eRankedModeGames = eRankedMapGames + SR_MAX_NUM_MAPS,
      eUnrankedLeaderGames = eRankedModeGames + SR_MAX_NUM_MODES,
      eUnrankedMapGames = eUnrankedLeaderGames + SR_MAX_NUM_LEADERS,
      eUnrankedModeGames = eUnrankedMapGames + SR_MAX_NUM_MAPS,
      eTipString = eUnrankedModeGames + SR_MAX_NUM_MODES,
      eEndTipString = eTipString + 1000,
      eLast,
   };

   static void addString( const BUString& text, int bucket = 0, int duration = -1, int64 id = -1 );
   static void clear();
   static void setSpeed( float speed );
   static void setSpacing( float spacing );
   static void showBackground( bool bShow );
   static void setSeparatorString( const BUString& text );
   static void updateTickerInfo();
   static void enable( bool enable );
   static void profileUpdate();

   static bool isEnabled();

   void updateTickerInfoInternal();
   void enableInternal( bool enable );
   void profileUpdateInternal();

   bool isTickerEnabled() { return mEnabled; }

protected:
   friend class BUIScreen;
   static void install();
   static void remove();

   static void renderTicker();

   bool  mTipsSetup;
   bool  mProfileSetup;
   bool  mEnabled;
   DWORD mNextUpdate;


private:
   BUITicker();
   virtual ~BUITicker();
};