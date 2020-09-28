//============================================================================
// UIObjectiveScreen.h
// Ensemble Studios (c) 2008
//============================================================================

#pragma once 
#include "UIScreen.h"
#include "UIScrollableTextFieldControl.h"
#include "UIImageViewerControl.h"
#include "UIButtonBarControl.h"
#include "UIScrollingListControl.h"
#include "UIGamerTagLongControl.h"
#include "UILabelControl.h"
#include "UITextFieldControl.h"
#include "UIDifficultyDisplayControl.h"

class BUIObjectiveControl;
class BCampaignNode;
class BPlayer;


//============================================================================
//============================================================================
class BMinimapLocation
{
public:
   float mMapCenterX;
   float mMapCenterY;
   float mMapW;
   float mMapH;

   long mMapFlashX;
   long mMapFlashY;
   long mMapFlashW;
   long mMapFlashH;
};


//============================================================================
//============================================================================
class BUICampaignObjectives : public BUIScreen
{
   enum
   {
      cMinimapLocation16x9=0,
      cMinimapLocation4x3=1,

      cMinimapLocationCount,
   };

   enum
   {
      cControlIDList1=0,
      cControlIDList2,
      cControlIDList3,
      cControlIDPlayer1,
      cControlIDPlayer2
   };

   enum
   {
      cMaxObjectives=11,
   };


public:
   BUICampaignObjectives( void );
   virtual ~BUICampaignObjectives( void );

   bool handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail);
   virtual bool executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl );
   virtual bool handleUIControlEvent( BUIControlEvent& event );
   virtual void update(float elapsedTime);

   virtual bool init( const char* filename, const char* datafile );
   virtual bool init(BXMLNode dataNode);

   virtual void setVisible( bool visible );


   void populateScreen();

   virtual void enter( void );
   virtual void leave( void );

protected:
   // init routines
   void initImageViewer();
   void initPlayers(BXMLNode dataNode);
   bool reset();

   // data fill routines
   bool displayButtons();
   void populateObjectives();
   void populatePlayers();
   void populateImage();
   void showSelectedObjective();
   void selectFirstObjective();

   void showMinimap();
   void restoreMinimap();

   void updateGameTime();

   void updateRScrollButton();

   // helper methods
   bool addObjective(const WCHAR* menuText, int listID, int index, int controlID, int type, int status);
   void populatePlayer(BPlayer* player, BUIGamerTagLongControl& playerControl);
   void populateDifficulty();

   BUITextFieldControl           mTitle;
   BUITextFieldControl           mDescription;
   BUITextFieldControl           mGameTime;
   BUIDifficultyDisplayControl   mDifficulty;

   BUIScrollableTextFieldControl mHelpText;
   BUIImageViewerControl         mImageViewer;

   BUIButtonBarControl           mButtonBar;

   BUIGamerTagLongControl        mPlayer1;
   BUIGamerTagLongControl        mPlayer2;

   // Objective list and its objective items
   BUIScrollingListControl             mObjectiveList1;
   BDynamicArray<BUIObjectiveControl*> mPrimaryObjectiveItems;

   BUIScrollingListControl             mObjectiveList2;
   BDynamicArray<BUIObjectiveControl*> mOptionalObjectiveItems;

   BUIScrollingListControl             mObjectiveList3;
   BDynamicArray<BUIObjectiveControl*> mCompletedObjectiveItems;

   BUILabelControl   mLabelPrimaryObjectives;
   BUILabelControl   mLabelOptionalObjectives;
   BUILabelControl   mLabelCompletedObjectives;

   BMinimapLocation  mMinimapLocation[cMinimapLocationCount];
   BMinimapLocation  mOriginalLocation;

   BUIScrollingListControl*            mpLastObjectiveControlWithFocus;
   BUILabelControl*                    mpLastObjectiveLabelWithFocus;

   uint            mLastViewChangeTime;          //E3 hax - eric
 DWORD             mLastGameTimeUpdate;
};