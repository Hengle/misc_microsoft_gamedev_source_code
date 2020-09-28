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
#include "ModePartyRoom2.h"

// Live Stuff
#include "liveSystem.h"
#include "UIPlayer.h"
#include "UITextFieldControl.h"


class BUIMenu;
class BUIMenuItemControl;


//============================================================================
// class BUIMPSetupScreen
//============================================================================
class BUIMPSetupScreen : public BUIScreen, public IMenuEventHandler
{
public:
   BUIMPSetupScreen();
   virtual ~BUIMPSetupScreen();

   enum
   {
      cControlIDMainMenu,
      cControlIDPlayerMenu,
      cControlIDSecondaryMenu,

      cMaxMainMenuItems=10,
      cMaxPlayerMenuItems=3,
      cMaxSecondaryMenuItems=16,

      cMaxSlotNumCol1=2,
   };

   enum 
   {
      cFlagInitialized = 0,
      cFlagRenderTargetReady,
      cFlagTotal
   };

   enum
   {
      cPlayerStateFocus=0,
      cPlayerStateNoFocus,
   };

   enum
   {
      cPlayerSlotStateNormal,
      cPlayerSlotStateCenter,

      cPlayerSlotCount=6,
   };

   enum
   {
      cTeamLabelStateOff=0,
      cTeamLabelStateActive,
      cTeamLabelStateInactive,
   };

   enum
   {
      cFlashButtonOff,
      cFlashButtonY,
      cFlashButtonA,
      cFlashButtonB,
      cFlashButtonX,
      cFlashButtonBack,
      cFlashButtonStart,
      
      cFlashButtonStateCount,
   };

   enum
   {
      cPRViewPlayerList,         // UI focus is on the player options (for custom at this point)
      cPRViewPlayerEdit,         // UI focus is on the editing a single player.
      cPRViewMenu,               // user is navigating the menu
      cPRViewSubMenu,            // user is navigating the sub menu
      cPRPlayerMissionPicker,    // user is in the scenario picker screen
      cPRViewPlayerMenu,         // user is in the player menu
      cPRViewLanGameList,        // user is in the LAN Game Browser

   };

   enum
   {
      cPlayerMenuCommandChangeSettings,
      cPlayerMenuCommandViewGamerCard,
      cPlayerMenuCommandAddAI,
      cPlayerMenuCommandKick,
      cPlayerMenuCommandKickAI,
   };

   enum MainMenuMM
   {
      cMMMenuLobby=0,
      cMMMenuHopper,
      //cMMMMenuGameMode,

      cMMMenuItemCount,
   };

   enum MainMenuCust
   {
      cCustomMenuLobby=0,
      cCustomMenuPlayerCount,
      cCustomMenuMap,
      cCustomMenuGameMode,
      // shawn - remove center column
      // cCustomMenuTeam,

      cCustomMenuItemCount,
   };



   enum SecondaryMenuCommands
   {
      // All sub menus
      cMenuCommandSetLobby=0,
      cMenuCommandLivePartyType,

      // custom menu
      cMenuCommandSetMap,
      cMenuCommandSetPlayerCount,
      cMenuCommandSetTeamType,
      cMenuCommandSetGameMode,
      cMenuCommandSetAIDifficulty,


      // matchmaking
      cMenuCommandSetMatchmakingHopper,

      // Campaign
      cMenuCommandSetCampaignMission,

      cMenuCommandCount
   };

   enum
   {
      cCustomTeamTypeTeams=0,
      cCustomTeamTypeRandom,
   };

   enum MainMenuCampaign
   {
      cCampaignMenuLobby=0,
      cCampaignMenuMissionPicker,

      cCampaignMenuItemCount,
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



   virtual bool init(BXMLNode dataNode);

   // Overhead functions
   // Input handler
   virtual bool handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail);
   virtual bool executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl );

   // event handler (events generated from control input)
   virtual bool handleUIControlEvent( BUIControlEvent& event );
   virtual void update( float dt );



   // screen management methods
   void setTitle(const BUString& pTitle);
   void setImage(const char * imageURL);
   void setHelpText(const BUString& helpText);
   void setInfoText(const BUString& infoText);
   void updatePopulation();

   void positionMovieClip(const char* firstMC, const char* secondMC );

   // player list management calls
   void setPlayerCurrentSlotFocus(int8 slot); // -1 means no slot has focus
   void setPlayerNavigationEnabled(bool value);
   void setPlayerVoiceStates(int slot0, int slot1, int slot2, int slot3, int slot4, int slot5);
   void refreshPlayerSlots();                 // this updates all the slots
   BUIPlayer* getPlayer(uint8 slot);
   void clearPlayerSlot(uint8 slot);
   void initializePlayerSlots();
   void setPlayerSlotActive(uint8 slot, bool active);  // Not active - not visible;
   int8 getPlayerCurrentSlotFocus();
   void resetPlayerInput();

   // Button bar accessor (until I can refactor)
   BUIButtonBarControl& getButtonBar() { return mButtonBar; }

   const long getView() const { return mPartyRoomView; }
   void setView(long view);


   // Main Menu Methods
   void populateCustomModeMenu(const BPartySessionHostSettings* pHostSettings);
   void populateCampaignModeMenu(const BPartySessionHostSettings* pHostSettings);
   void populateMatchmakingModeMenu(const BPartySessionHostSettings* pHostSettings);

   void populateGamesList();
   bool populatePlayerMenu(BPartySessionPartyMember* pMember, int8 slot);



   void populateLobbyMenu(const BPartySessionHostSettings* pHostSettings, bool lanMode);
   void populatePartyType(const BPartySessionHostSettings* pHostSettings);
   void populateRandomMapMenu(const BPartySessionHostSettings* pHostSettings);
   void populateGameModes(const BPartySessionHostSettings* pHostSettings);
   void populateDifficultyMenu(const BPartySessionHostSettings* pHostSettings);
   void populatePlayerCountMenu(const BPartySessionHostSettings* pHostSettings);
   void populateTeamMenu(const BPartySessionHostSettings* pHostSettings);
   void populateMatchmakingHopperMenu(const BPartySessionHostSettings* pHostSettings);
   void populateCampaignMissionPicker(const BPartySessionHostSettings* pHostSettings);



   // LAN Games List
   BUIMenu* getLanGamesList() { return mpGamesList; }

   BUIListControl& getPlayerMenuNew() { return mPlayerMenu; }
   BUIListControl& getMainMenu() { return mMainMenu; }

   // IMenuEventHandler interface
   virtual bool menuEvent(const BSimString& command, BUIMenu* pMenu);

   void updateHelp();
   void updatePlayerListHelp();
   void updateMenuHelp();

   void displayButtons();

   bool getFirstValidMap(uint8 numPlayers, uint8& mapIndex, bool matchPlayersExactly=false);

   void reset();
   void  resetSlotStates();

   void cancelSubMenuIfUp();

private:

   // player management calls
   void updatePlayerSlot(uint8 slot);
   //void setPlayerSlotData(uint8 slot, BSimString& gamerTag, int ping, bool ready, int8 leader, int8 civ, bool isLocal, bool isHost);
   void setPlayerSlotData(uint8 slot);
   void setPlayerSlotDataMatchmaking(uint8 slot, BUString& gamerTag, bool ready);
   void clearPlayerSlotData(uint8 slot);
   bool haveRoomToInvitePlayers();


   void addMenuItem(long index, BUIMenuStrip* pMenuStrip, const WCHAR* text, int commandID, int8 id);
   // Session helpers

   // fixme - do we want these here or back on ModePartyRoom?
   BOOL isLocalPlayer(BPartySessionPartyMember* pPartyMember) const;
   BOOL isLocalPlayerHost() const;
   BPartySession* getPartySession() const;
   BPartySessionPartyMember* getMemberByXUID(XUID xuid);
   bool isLocalPlayerReady();
   BPartySessionPartyMember* BUIMPSetupScreen::getLocalMember();

   bool isValidMapType(long mapType);
   bool isValidMap(uint8 mapIndex, uint8 numPlayers, bool matchPlayersExactly=false);


   // animation control for the slots  
   uint8 getSlotTransitionState(uint8 slot, bool inCenter);
   void  setSlotState(uint8 slot, uint8 slotState);

   void getSettingsChangeFromSecondaryMenuItem(BUIMenuItemControl * pMenuItemControl);


   long mPartyRoomView;       // The view we currently have in the lobby
   BString mLastMapImage;

   // old - would like to remove this if possible.
   BUIMenu*             mpGamesList;

   BUIPlayer            mPlayers[cPlayerSlotCount];


   // New Style UI Controls
   BUITextFieldControl        mTeamAlphaLabel;
   BUITextFieldControl        mTeamBravoLabel;

   BUITextFieldControl        mHelpText2;
   BUITextFieldControl        mSecondaryTitle;

   BUITextFieldControl        mGamesListTitle;
   BUIImageViewerControl      mImageViewer;

   // this is not following good UI control stuff, but at this point it's getting it done.
   BUITextFieldControl        mPlayerListTitle;
   BUIListControl             mPlayerMenu;

   BUIListControl             mMainMenu;
   BUIListControl             mSecondaryMenu;

   BUIGameSetupPlayerControl  mPlayerControls[cPlayerSlotCount];
   BUIButtonBarControl        mButtonBar;
   BUIGridControl             mGridControl;

   BDynamicArray<BUIMenuItemControl*> mMenuItems;
   BDynamicArray<BUIMenuItemControl*> mPlayerMenuItems;
   BDynamicArray<BUIMenuItemControl*> mSecondaryMenuItems;

   uint8                      mSlotStates[cPlayerSlotCount];
   DWORD                      mLastPopUpdateTime;
   DWORD                      mLastHelp2Clear;
   DWORD                      mLastGamerPicUpdate;
   BDynamicSimArray<BString>  mSlotKeyFrames;
   uint                       mLastBumperKeyInputTime;
   uint                       mLastViewChangeTime;          //E3 hax - eric

   // fixme - until I can refactor things
   BModePartyRoom2*           mpMode;
};