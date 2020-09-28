//============================================================================
// UIMPSetupScreen.h
// Ensemble Studios (c) 2008
//============================================================================

#pragma once 

#include "UIScreen.h"

// UI Controls
#include "UIImageViewerControl.h"
#include "UIButtonBarControl.h"
#include "UIGameSetupPlayerControl.h"
#include "UIGridControl.h"
#include "UIListControl.h"
#include "modemanager.h"
#include "UIPlayer.h"
#include "UITextFieldControl.h"
#include "UILeaderPicker.h"
#include "UIGlobals.h"

class BUIMenu;
class BUIMenuItemControl;

//============================================================================
// class BUISkirmishSetupScreen
//============================================================================
class BUISkirmishSetupScreen : 
   public BUIScreen, 
   public ILeaderPickerEventHandler, 
   public BUIGlobals::yornHandlerInterface,
   public BUserNotificationListener
{
public:

   enum
   {
      cControlIDMainMenu,
      cControlIDPlayerMenu,
      cControlIDSecondaryMenu,

      cMaxMainMenuItems=10,
      cMaxPlayerMenuItems=3,
      cMaxSecondaryMenuItems=16,

      cPRNumPlayersSlotsPerTeam=3,
      cPRNumPlayerSlots=6,

      cMaxSlotNumCol1=2,
   };

   enum
   {
      cMenuCommandSetLobby,
      cMenuCommandSetPlayerCount,
      cMenuCommandSetMap,
      cMenuCommandSetGameMode,
      cMenuCommandSetAIDifficulty,
      cMenuCommandSetTeamType,
      cMenuCommandSetCampaignMission,

      cPlayerMenuCommandAddAI,
      cPlayerMenuCommandKickAI,
      cPlayerMenuCommandChangeSettings,
      cPlayerMenuCommandViewGamerCard,
      cPlayerMenuCommandKick,


   };

   enum
   {
      cPRViewPlayerList,         // UI focus is on the player options (for custom at this point)
      cPRViewPlayerEdit,         // UI focus is on the editing a single player.
      cPRViewMenu,               // user is navigating the menu
      cPRViewSubMenu,            // user is navigating the sub menu
      cPRPlayerMissionPicker,    // user is in the scenario picker screen
      cPRViewPlayerMenu,         // user is in the player menu
//      cPRViewLanGameList,        // user is in the LAN Game Browser

   };


   enum 
   {
      cPRSlotStateEmpty=0,
      cPRSlotStateNormal,
      cPRSlotStateCenter,

      cPRSlotStateCount,               // Number of different states
   };

   enum
   {
      cPRSlotKF_Empty=0,
      cPRSlotKF_EaseIn,
      cPRSlotKF_Normal,             // State
      cPRSlotKF_Center,             // state

      cPRSlotKF_N2C,                // Normal to Center
      cPRSlotKF_C2N,                // Center to Normal

      cPRSlotKF_TransitionCount,
   };


   enum
   {
      cPlayerSlotCount=6,
   };


   BUISkirmishSetupScreen();
   virtual ~BUISkirmishSetupScreen();

   virtual bool init(BXMLNode dataNode);

   // Overhead functions
   // Input handler
   virtual bool handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail);
   virtual bool executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl );
   virtual void render( void );
   virtual void update( float dt );

   // event handler (events generated from control input)
   virtual bool handleUIControlEvent( BUIControlEvent& event );

   bool leaderPickerEvent(const BSimString& command);

   void populate();
   void displayButtons();

   // BUserNotificationListener
   void userStatusChanged();

   // yorn handler
   virtual void yornResult(uint result, DWORD userContext, int port);

protected:
   void setInitialValues();
   void setDefaultMapIndex();

   void editPlayer(int slot);

   // screen management methods
   void setTitle(const BUString& pTitle);
   void setImage(const char * imageURL);
   void setHelpText(const BUString& helpText);
   void positionMovieClip(const char* firstMC, const char* secondMC );

   // help methods
   void updateHelp();
   void updatePlayerListHelp();
   void updateMenuHelp();


   void updateMap();

   // Main Menu Methods
   void populateMenu();
   void populateCustomModeMenu();
   void populateCampaignModeMenu();

   void populateLobbyMenu();
   void populateRandomMapMenu();
   void populateGameModes();
   void populateDifficultyMenu();
   void populatePlayerCountMenu();
   void populateTeamMenu();
   void populateCampaignMissionPicker();
   void populatePlayerMenu(int slot);


   void clearPlayerSlotData(int slot,const BUString& text);
   void refreshPlayerSlots();
   void updatePlayerSlot(int slot);
   void resetSlotStates();
   void setSlotState(int slot, int slotState);
   int getSlotTransitionState(int slot, bool inCenter);




   void BUISkirmishSetupScreen::updatePlayerList();
   void setPlayerCurrentSlotFocus(int newSlot);
   BUIPlayer* getPlayer(int slot);
   int getPlayerSlotByPort(int port);
   void setPlayerSlotActive(int slot, bool active);
   int getPlayerCurrentSlotFocus();
   void setPlayerSlotData(int slot);

   bool isValidMapType(long mapType);
   bool isValidMap(int mapIndex, int numPlayers, bool matchPlayersExactly=false);
   bool getFirstValidMap(int numPlayers, int& mapIndex, bool matchPlayersExactly=false);
   bool getRandomValidMap(int numPlayers, int& mapIndex, bool matchPlayersExactly);

   const long getView() const { return mPartyRoomView; }
   void setView(long view) { mPartyRoomView = view; mLastViewChangeTime = timeGetTime();}

   // animation control for the slots
   void getSettingsChangeFromSecondaryMenuItem(BUIMenuItemControl * pMenuItemControl);

   bool getUseDefaultMap() { return mUseDefaultMap; }
   void setUseDefaultMap(bool value) { mUseDefaultMap=value; }


   void initHostPlayer();
   void setPlayerSlotsVisible(int numVisible, bool team);
   bool updateMapAndPlayers(int newMapIndex, int newNumPlayers);
   bool updateMap(int newMapIndex);

   void kickPlayer(int slot);
   bool isSlotEmpty(int slot);
   int getFirstOpenSlot(bool bTeam);

   void setPlayer(BUser* user, int leader, int civ, int slot, int team);
   void refreshPlayer(BUser* user, int slot);
   void addAI(int slot);

   void acceptPlayerChanges();
   void cancelPlayerChanges();

   bool decrementLeader(int slot);
   bool incrementLeader(int slot);

   bool getDifficultyValueForType( long type, float& value );

   bool canStartGame(bool bTeam);
   bool startGame();

   bool checkSecondaryUser(long port, long event, long controlType, BInputEventDetail& detail);
   bool isValidSlot(int slot);

   bool isUserType(int slot, int userType);

   long mPartyRoomView;       // The view we currently have in the lobby

   // Temp Variable
   int            mEditSlot;
   int            mPartyRoomViewPrevious;

   // Local State variables
   int            mNumPlayers;
   int            mMapIndex;
   int            mDifficulty;
   int            mGameMode;
   bool           mRandomTeam;

   BUIPlayer      mPlayers[cPlayerSlotCount];
   BString        mLastMapImage;


   BUILeaderPicker*           mpLeaderPicker;

   // old - would like to remove this if possible.
   BUIMenu*                   mpGamesList;

   BUITextFieldControl        mTitle;
   BUITextFieldControl        mHelpText;

   BUITextFieldControl        mTeamAlphaLabel;
   BUITextFieldControl        mTeamBravoLabel;


   // New Style UI Controls
   BUIImageViewerControl      mImageViewer;

   BUIListControl             mMainMenu;
   BUIListControl             mPlayerMenu;
   BUIListControl             mSecondaryMenu;

   BUIGameSetupPlayerControl  mPlayerControls[cPlayerSlotCount];
   BUIButtonBarControl        mButtonBar;
   BUIGridControl             mGridControl;

   BDynamicArray<BUIMenuItemControl*> mMenuItems;
   BDynamicArray<BUIMenuItemControl*> mPlayerMenuItems;
   BDynamicArray<BUIMenuItemControl*> mSecondaryMenuItems;

   int                        mSlotStates[cPlayerSlotCount];
   BDynamicSimArray<BString>  mSlotKeyFrames;
   uint                       mLastBumperKeyInputTime;
   uint                       mLastViewChangeTime;          //E3 hax - eric
   DWORD                      mLastGamerPicUpdate;

   bool                       mUseDefaultMap:1;
   bool                       mDoRefresh:1;
};