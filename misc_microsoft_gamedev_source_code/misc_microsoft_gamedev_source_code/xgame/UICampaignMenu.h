//============================================================================
// UICampaignMenu.h
// Ensemble Studios (c) 2008
//============================================================================

#pragma once 
#include "UIScreen.h"
#include "UIListControl.h"
#include "UIScrollableTextFieldControl.h"
#include "UIImageViewerControl.h"
#include "UIButtonBarControl.h"
#include "UIGamerTagControl.h"
#include "UIDifficultyDisplayControl.h"
#include "UITextFieldControl.h"
#include "binkInterface.h"
#include "UIGlobals.h"
#include "UIMenuItemControl.h"
#include "gamesettings.h"
#include "AsyncTaskManager.h"
#include "BackgroundMovies.h"

class BUICampaignMissionPicker;
class BUICampaignMoviePicker;
class BModeCampaign2;
class BUISkirmishPostGameScreen;
class BCampaignNode;

class BUICampaignMenu : public BUIScreen, public BUserNotificationListener, public BUIGlobals::yornHandlerInterface
{

public:
   BUICampaignMenu( void );
   virtual ~BUICampaignMenu( void );

   virtual bool init( const char* filename, const char* datafile );
   virtual bool init(BXMLNode dataNode);

   virtual void render( void );

   bool handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail);
   virtual bool executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl );
   virtual bool handleUIControlEvent( BUIControlEvent& event );

   int getNextPlayAllIndex();
   void refreshMoviePicker();

   bool isMissionPickerVisible(); 

   // BUserNotificationListener
   void userStatusChanged();

   // yornHandlerInterface
   virtual void yornResult(uint result, DWORD userContext, int port);

   void updateSaveGameData();

   bool saveGameExists();

protected:

   bool checkSaveDevice();

   enum
   {
      cCampaignControlIDMainMenu,
      cCampaignControlIDSecondaryMenu,
   };

   enum
   {
      cControlContinue,
      cControlNewCampaign,
      cControlLoadGame,
      cControlLoadGameStart,
      cControlLoadChangeDevice,

      cControlTutorial,
      cControlCampaignTutorial,
      cControlSkirmishTutorial,

      cControlMissionSelector,
      
      cControlCoop,
      cControlCoopLive,
      cControlCoopSysLink,

      cControlMovieSelector,

      cControlChangeDifficulty,
      cControlSetDifficultyEasy,
      cControlSetDifficultyNormal,
      cControlSetDifficultyHeroic,
      cControlSetDifficultyLegendary,

      cCountrolCount
   };

   BCampaignNode* getDisplayNode();
   void playCampaign(int startingNode = -1, bool useSaveIfAvailable = false);
   
   bool addMenuItem(const WCHAR* menuText, int index, int controlID, int data=-1);
   bool addFlyoutMenuItem(const WCHAR* menuText, int index, int controlID);
   bool displayButtons();
   void setDifficulty(long newDiff);
   void updateDifficultyText();

   BModeCampaign2* getCampaignMode();

   void setupGamerTagControls();
   void updateGamerTagControls();
   bool checkSecondaryUser(long port, long event, long controlType, BInputEventDetail& detail);


   void loadBackgroundMovie();

   void populateMenu(void);
   void showDifficultyFlyout(void);
   void showTutorialFlyout(void);
   void showCoopFlyout(void);

   void initMenuItems();

   bool updateSecondaryMenuItem(const WCHAR* menuText, int index, int controlID, bool enable, int data=-1);
   void disableSecondaryMenuItems(int startngIndex);
   bool addSecondaryMenuItem(const WCHAR* menuText, int index, int controlID);

   void positionMovieClip(const char* firstMC, const char* secondMC );

   int  getSaveGameMenuName(BUString& name);
   void showLoadGameFlyout();



   // My main controls
   BUIListControl       mCampaignMenu;
   BUIListControl       mSecondaryMenu;
   BDynamicArray<BUIMenuItemControl*> mSecondaryMenuItems;

/*
   BUIListControl       mFlyoutMenu;
   BUIMenuItemControl   mFlyoutMenuItems[cMAX_FLYOUT_ITEMS];
*/

   BUIButtonBarControl  mButtonBar;
   BUIGamerTagControl   mGamerTagPrimary;
   BUIGamerTagControl   mGamerTagSecondary;
   BUIDifficultyDisplayControl  mDifficultyText;

   BUITextFieldControl           mTitleLabel;
   BUITextFieldControl           mPlayerListLabel;

   // Menu Items
   static const long cMAX_MENU_ITEMS = 8;
   static const long cMAX_FLYOUT_ITEMS = 4;
   BUIMenuItemControl mMenuItems[cMAX_MENU_ITEMS];

   
   // Other Screens - should I put these on the mode?
   BUICampaignMissionPicker*  mpMissionPicker;
   BUICampaignMoviePicker*    mpMoviePicker;

   BBackgroundMovies          mBackgroundMovies;

   BBinkVideoHandle           mBackgroundVideoHandle;

   BGameSettings              mSavegameSettings;
   BUString                   mSavegameName;

   bool                       mFoundSaveGame:1;

};