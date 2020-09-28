//============================================================================
// UISkirmishPostGameScreen.h
// Ensemble Studios (c) 2008
//============================================================================

#pragma once 
#include "UIScreen.h"

#include "UIButtonBarControl.h"
#include "UITabControl.h"
#include "UIGridControl.h"
#include "UIGamerTagControl.h"
#include "UIScrollableCalloutControl.h"
#include "graphmanager.h"
#include "UITextFieldControl.h"
#include "UIServiceRecordScreen.h"

class BUIPanelControl;
class BPlayer;

//============================================================================
//============================================================================
class BTimelineRect
{
public:
   float mX;
   float mY;
   float mWidth;
   float mHeight;
};


class BUISkirmishPostGameScreen : public BUIScreen
{
public:

   enum
   {
      cTimelineLocation16x9=0,
      cTimelineLocation4x3=1,

      cTimelineLocationCount,
   };

   enum
   {
      cSkirmishPostgameTabScore=0,
      cSkirmishPostgameTabMilitary,
      cSkirmishPostgameTabEconomy,
      cSkirmishPostgameTabTimeline,

      cSkirmishPostgameTabNumber,

   };

   enum
   {
      cStatsTimelineScore,
      cStatsTimelineSupplies,
      cStatsTimelineTechLevel,
      cStatsTimelinePop,
      cStatsTimelineBases,       // need to add these

      cStatsTimelineCount,
   };

   enum
   {
      cGridRows=6,
      cGridColumns=6,
      cMaxPlayers=6
   };

   BUISkirmishPostGameScreen( void );
   virtual ~BUISkirmishPostGameScreen( void );

   virtual bool init(BXMLNode dataNode);

   bool handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail);
   virtual bool executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl );
   virtual bool handleUIControlEvent( BUIControlEvent& event );
   virtual void handleUIScreenResult( BUIScreen* pScreen, long result );
   virtual void render();

   void update(float elapsedTime);
   
   void populate();

   bool displayButtons();

   void setStatsVisible(bool bVisible);

   // other methods
   void setStatsViewed(bool viewed) { mStatsViewed = viewed; }
   const bool getStatsViewed() const { return mStatsViewed; }

   void workerRenderMovie(void* pData);

   virtual void enter( void );

   enum EResult { eResult_Back = -1 };
protected:

   // init routines
   void initPlayers(BXMLNode dataNode);
   void initPanels();
   void initGrids();

   void initSingleGrid(BUIGridControl* pGrid, const char* pCellPrefix);

   // populate routines
   void populatePlayers();
   void populateScoreTab();
   void populateMilitaryTab();
   void populateEconomyTab();
   void populateTimelineTab();

   void populatePlayer(BPlayer* pPlayer, BUIGamerTagControl& playerControl);

   void hideRow(BUIGridControl* pGridControl, int row);


   // helper routines
   bool addTab(const char * labelName, BUIPanelControl *panel, const WCHAR* labelText);
   void fixupPlayerFocus();
   void checkColumnTitle();

   BUIControl* getSelectedCell();

   // timeline methods
   void renderTimelineTab();
   void addTimeline(int id, BDynamicArray<float>& points, DWORD playerColor, int playerNumber);
   void showTimeline(bool animate=true);

   int getPlayerWithFocus();

   // My main data and controls 
   // Timeline data
   BGraphHandle         mTimelineHandles[cStatsTimelineCount];
   int                  mCurrentTimeline;
   float                mGraphFadeTime;
   BTimelineRect        mTimelinePostions[cTimelineLocationCount];

   BUITextFieldControl  mColumnHeader;

   bool                 mShowingStats;
   bool                 mStatsViewed;
   BUITabControl        mTabControl;
   BUIButtonBarControl  mButtonBar;
   BDynamicArray<BUIControl*> mControls;

   BUIGamerTagControl   mPlayers[cMaxPlayers];

   // grid controls for the tabs
   BUIGridControl*      mpGridControlScore;
   BUIGridControl*      mpGridControlMilitary;
   BUIGridControl*      mpGridControlEconomy;
   BUIGridControl*      mpGridControlTimeline;

   // call out controls
   BUIScrollableCalloutControl    mCallout;

   BUITextFieldControl mTeam1Label;
   BUITextFieldControl mTeam2Label;

   uint                 mLastUpdateGameTime;

/*
   // SRL - pulled - phx-16437
   BUIServiceRecordScreen mServiceRecord;
*/
};