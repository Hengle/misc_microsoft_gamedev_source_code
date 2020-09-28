//============================================================================
// UICpgSummaryPanelControl.h
// Ensemble Studios (c) 2008
//============================================================================
#pragma once

#include "xcore.h"

#include "UIPanelControl.h"

#include "UIGamerTagLongControl.h"
#include "UITextFieldControl.h"
#include "UIImageViewerControl.h"
#include "UIListControl.h"

class BPlayer;

class BUICpgGradePanelControl : public BUIPanelControl
{

public:

   enum
   {
      cPrimaryCampaignPlayer,
      cCoopCampaignPlayer,

      cMaxCampaignPlayers,
   };

   BUICpgGradePanelControl( void );
   virtual ~BUICpgGradePanelControl( void );

   virtual bool init( IUIControlEventHandler* parent, const char* controlPath, int controlID = -1, BXMLNode* initData = NULL );

   void show( bool force=false );
   void hide( bool force=false );

   bool handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail);
   virtual bool executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl );

   bool handleUIControlEvent( BUIControlEvent& event );

   bool populate();


protected:

   void initLabels();

   BPlayer * getCoopPlayer(BPlayer* player);
   void populateParTime();
   void populateLabels();
   void populatePlayers();
   void populatePlayer(BPlayer* player, int playerIndex);
   void populateScore(BPlayer* player, int playerIndex);
   void formatTime(DWORD time, BUString& timeString);

   void populateMissionResult();
   void populateMedal();

   void hidePlayer2();
   void movePlayer1ToCenter();

   // Labels
   BUITextFieldControl     mTitle;
   BUITextFieldControl     mParTimeLabel;
   BUITextFieldControl     mParTimeText;

   BUITextFieldControl     mFinalScoreLabel;
   BUITextFieldControl     mFinalScoreText;

   //BUITextFieldControl     mPlayerListLabel;


   // Score Labels
   BUITextFieldControl     mBaseScoreObjectiveLabel[cMaxCampaignPlayers];
/*
   BUITextFieldControl     mBaseScoreMilitaryLabel[cMaxCampaignPlayers];
   BUITextFieldControl     mBaseScoreTotalLabel[cMaxCampaignPlayers];
*/

   BUITextFieldControl     mCombatBonusLabel[cMaxCampaignPlayers];
   BUITextFieldControl     mTimeBonusLabel[cMaxCampaignPlayers];
   BUITextFieldControl     mSkullsBonusLabel[cMaxCampaignPlayers];

   BUITextFieldControl     mTotalScoreLabel[cMaxCampaignPlayers];


   // Score Fields
   BUITextFieldControl     mBaseScoreObjective[cMaxCampaignPlayers];
   BUITextFieldControl     mBaseScoreMilitary[cMaxCampaignPlayers];
   BUITextFieldControl     mBaseScoreTotal[cMaxCampaignPlayers];

   BUITextFieldControl     mCombatBonusText[cMaxCampaignPlayers];
   BUITextFieldControl     mTimeBonusText[cMaxCampaignPlayers];
   BUITextFieldControl     mSkullsBonusText[cMaxCampaignPlayers];

   BUITextFieldControl     mTotalScoreText[cMaxCampaignPlayers];

   BUITextFieldControl     mMissionStatusText;

   BUITextFieldControl     mScoreRangeValues;
   BUITextFieldControl     mScoreRangeLabels;

   BUIImageViewerControl   mMedalImage;

   // Column Headers
//   BUITextFieldControl     mPlayerLabel[cMaxCampaignPlayers];

   // GamerTags
   BUIListControl          mPlayerList;
   BUIGamerTagLongControl      mPlayer[cMaxCampaignPlayers];
};