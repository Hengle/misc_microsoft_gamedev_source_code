//============================================================================
// UISkirmishObjectives.h
// Ensemble Studios (c) 2008
//============================================================================

#pragma once 
#include "UIScreen.h"
#include "UIButtonBarControl.h"
#include "UIScrollingListControl.h"
#include "UIGamerTagLongControl.h"
#include "UILabelControl.h"
#include "UIScrollableTextFieldControl.h"
#include "UITextFieldControl.h"
#include "UIDifficultyDisplayControl.h"

class BUIObjectiveControl;
class BPlayer;

//============================================================================
//============================================================================
class BSkirmMinimapLocation
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
class BUISkirmishObjectives : public BUIScreen
{
   enum
   {
      cMaxObjectives=11,
      cMaxSkirmishPlayers=6
   };

   enum
   {
      cMinimapLocation16x9=0,
      cMinimapLocation4x3=1,

      cMinimapLocationCount,
   };



public:
   BUISkirmishObjectives( void );
   virtual ~BUISkirmishObjectives( void );

   bool handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail);
   virtual bool executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl );
   virtual bool handleUIControlEvent( BUIControlEvent& event );
   virtual void update(float elapsedTime);

   virtual bool init( const char* filename, const char* datafile );
   virtual bool init(BXMLNode dataNode);

   virtual void setVisible( bool visible );

   void populateScreen();
//   void showSelectedObjective();

   virtual void enter( void );
   virtual void leave( void );

protected:
   // init routines
   void initPlayers(BXMLNode dataNode);
   bool reset();

   void showMinimap();
   void restoreMinimap();

   void updateRScrollButton();

   void updateGameTime();
   void updatePlayerStatus();

   // data fill routines
   bool displayButtons();
//   void populateObjectives();
   void populatePlayers();
   void populateDifficulty();
   void populateMapName();

   // helper methods
//   bool addObjective(const WCHAR* menuText, int listID, int index, int controlID, int type, int status);
   void populatePlayer(BPlayer* player, BUIGamerTagLongControl& playerControl);
  
   int getPlayerWithFocus();
   void clearPlayerFocus();

   BUITextFieldControl           mTitle;
   BUITextFieldControl           mTeam1;
   BUITextFieldControl           mTeam2;
   BUITextFieldControl           mDescription;
   BUIScrollableTextFieldControl mHelpText;

   BUITextFieldControl           mGameTime;
   BUIDifficultyDisplayControl   mDifficulty;

   BUIButtonBarControl           mButtonBar;

   BUIListControl                mPlayerList;
   BUIGamerTagLongControl        mPlayers[cMaxSkirmishPlayers];

   // Objective list and its objective items
/*
   BUIScrollingListControl             mObjectiveList;
   BDynamicArray<BUIObjectiveControl*> mObjectiveItems;
*/

//   BUILabelControl               mObjectiveTitle;

   BSkirmMinimapLocation         mMinimapLocation[cMinimapLocationCount];
   BSkirmMinimapLocation         mOriginalLocation;

   DWORD             mLastGameTimeUpdate;
};