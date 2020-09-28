//============================================================================
// UIMainMenuScreen.h
// Ensemble Studios (c) 2008
//============================================================================

#pragma once 
#include "UIScreen.h"
#include "UIGlobals.h"
#include "UIListControl.h"
#include "UIScrollableTextFieldControl.h"
#include "UIImageViewerControl.h"
#include "UIButtonBarControl.h"
#include "UIGamerTagControl.h"
#include "UIDifficultyDisplayControl.h"
#include "binkInterface.h"

class BUIMenuItemControl;

class BUIMainMenuScreen : public BUIScreen, public BUIGlobals::yornHandlerInterface
{

public:
   BUIMainMenuScreen( void );
   virtual ~BUIMainMenuScreen( void );

   virtual bool init( const char* filename, const char* datafile );
   virtual bool init(BXMLNode dataNode);

   virtual void render( void );

   bool handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail);
   virtual bool executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl );
   virtual bool handleUIControlEvent( BUIControlEvent& event );

   // yorn handler
   virtual void yornResult(uint result, DWORD userContext, int port);

   void populateMainMenu(int selectedItem);
   void populateMultiplayerMenu(int selectedItem);
   void populateOtherMenu(int selectedItem);
   void populateExtrasMenu(int selectedItem);

   void populateTutorialMenu();


protected:

   enum
   {
      cMainMenuControlIDMainMenu,
      cMainMenuControlIDSecondaryMenu,
   };

   enum
   {
      cMenuMain,
      cMenuMultiplayer,
      cMenuExtras,
      cMenuOptions,
      cMenuOther,
   };

   enum
   {
      cCommandCampaign,
      cCommandSkirmish,
      cCommandMultiplayer,
      cCommandTimeline,
      cCommandOptions,
      cCommandOptionsExtras,
      cCommandDownloads,
      cCommandTutorial,
      cCommandSystemLink,
      cCommandXboxLive,
      cCommandLeaderboards,
      cCommandServiceRecord,
      cCommandStorageDevice,
      cCommandExit,
      cCommandOther,
      cCommandSaveGame,
      cCommandRecordGame,
      cCommandCredits,
      cCommandScenario,
      cCommandCalibrate,
      cCommandFlash,
      cCommandCinematic,
      cCommandModelView,

      cCommandStartCampaign,
      cCommandContinueCampaign,

      cCommandBasicTutorial, 
      cCommandAdvancedTutorial, 

      cCommandDemoMovie,
      cCommandDemoNoMovie,
      cCommandDemoUNSCSkirmish,
      cCommandDemoCovenantSkirmish,

      cCommandCount,

      cMaxMenuItems=8,
      cMaxMainMenuSecondaryMenuItems=8,
   };

   void clearMenu();
   void disableMenuItems(int startngIndex);
   void disableSecondaryMenuItems(int startingIndex);
   void initMenuItems();

   bool addMenuItems();
   bool updateMenuItem(const WCHAR* menuText, int index, int controlID, bool enable);
   bool updateSecondaryMenuItem(const WCHAR* menuText, int index, int controlID, bool enable);
   bool addMenuItem(const WCHAR* menuText, int index, int controlID);
   bool addSecondaryMenuItem(const WCHAR* menuText, int index, int controlID);
   
   bool displayButtons();

   void startCampaignYorn();
   void startCampaign();


   void setupGamerTagControls();
   void loadBackgroundMovie(BXMLNode dataNode);

   void playCampaign(int startingNode /* = -1*/);
   void positionMovieClip(const char* firstMC, const char* secondMC );


   int                  mCurrentMenu;

   // My main controls
   BUIListControl       mMainMenu;
   BUIListControl       mSecondaryMenu;


   BUIButtonBarControl  mButtonBar;
   
   BUIGamerTagControl   mGamerTagPrimary;
   BUIGamerTagControl   mGamerTagSecondary;

   // Menu Items
   BDynamicArray<BUIMenuItemControl*> mMenuItems;
   BDynamicArray<BUIMenuItemControl*> mSecondaryMenuItems;

   BBinkVideoHandle           mBackgroundVideoHandle;
};