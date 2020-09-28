//============================================================================
// uicontext.h
// // Copyright (c) 2006-2008 Ensemble Studios
//============================================================================

#pragma once
#include "flashreticle.h"
#include "flashhud.h"

class BUser;

//============================================================================
//============================================================================
class BUIContext
{
   public:
      BUIContext(); 
      ~BUIContext();


      enum 
      {
         cReticleHelpButtonA=0,
         cReticleHelpButtonB,
         cReticleHelpButtonX,
         cReticleHelpButtonY,
      };

      bool init(BUser* pUser);
      void deinit();
      void update(float elapsedTime);

      void renderBegin();
      void render();
      void renderEnd();
      
      

      void handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail);

      // reticle Interface
      void setReticleVisible(bool bVisible);
      bool getReticleVisible() const;
      void setReticleMode(int mode, uint goodAgainstRating=0, const BCost* pCost=NULL);
      void setReticleHelp(int button, int frameID);
      const BReticleHelpButtonState* getReticleButtonState(int buttonID);
      void updateReticleButtons();
      void setReticlePosition(int x, int y, int width, int height);
      void renderReticle();

      // HUD Interface
      void renderHUD();
      //-- circle menu interface
      // Player Score

      // Tech Notification
      void playTechNotification(int techID);

      //-- Chat
      void displayChat(BChatMessage* pChat);
      void setChatVisible(bool bVisible);

      void setGameTimeVisible(bool bVisible);
      bool getGameTimeVisible() const;
      void setGameTime(const BUString& timeStr);

      bool isOverlayVisible() const;
      void setOverlayVisible(bool bON);
      bool isPowerOverlayVisible() const;
      void setPowerOverlayVisible(int powerID, bool bON);
      void setPowerOverlayText(int powerID, const BUString& text);


      void setDPadButtonIcon(int controlID, int iconID, int frameID);
      int  getInputFunctionIconID(int inputFunctionID);
      void setButtonState(int controlID, int keyFrameID);

      void setScoresVisible(bool bON);
      void setResourcePanelVisible(bool bVisible);
      bool getResourcePanelVisible() const;
      void setPowerPanelVisible(bool bVisible);
      void setDPadPanelVisible(bool bVisible);  
      bool isDPadPanelVisible() const;
      void setButtonPanelVisible(bool bVisible);
      bool isButtonPanelVisible() const;

      void setUnitSelectionVisible(bool bVisible);
      void setUnitCardVisible(bool bVisible);
      void setUnitSelection(int slotID, int frameID, BEntityID squadID, int protoSquadID, int protoObjID, bool bTagged, int abilityFrameID, int count, const BUString& detailStr, const BUString& nameStr, const BUString& roleStr, bool bShowGamerTag, const BUString& gamerTagStr, DWORD color);
      void updateUnitSelectionDisplay(int subSelectIndex, bool bDisplayArrows);
      void setUnitCard(int slotID, int frameID, int playerID, BEntityID squadID, int protoSquadID, int protoObjID, int abilityID, bool bTagged, int abilityFrameID, int count, const BUString& detailStr, const BUString& nameStr, const BUString& roleStr, bool bShowGamerTag, const BUString& gamerTagStr, DWORD color);
      void updateUnitCardDisplay();

      void setPlayerScore(int playerID, const BUString& text);
      void setPlayerColor(int playerID, int color);
      void setUnitStatsVisible(bool bON);
      bool getUnitStatsVisible() const;
      void setUnitStats(const BUString& name, float stat1, float stat2, float stat3, float stat4, float stat5);
      void setHUDLabelText(int labelID, const BUString& text);
      void setHUDLabelText(int labelID, const WCHAR* pText);
      void setHUDLabelTextVisible(int labelID, bool bVisible);
      void setAllHUDLabelTextVisible(bool bVisible);
      void refreshHUD();   
      void showCircleMenu(int type, const WCHAR* pText);
      void hideCircleMenu();      
      bool getCircleMenuVisible() const { return mpHUD ? mpHUD->getCircleMenuVisible() : false; }
      void resetCircleMenuPointingAt();
      void setCircleMenuVisible(bool bVisible, int type);
      void setCircleMenuBaseText(const WCHAR* text);
      void setCircleMenuBaseText2(const WCHAR* text);
      void setCircleMenuBaseTextDetail(const WCHAR* text);
      void clearCircleMenuItems();
      void clearCircleMenuDisplay();
      void clearInfoPanelStats();
      void clearInfoPanelDescription();
      long getCircleMenuItemIndex(long id) const;
      long addCircleMenuItem(long order, long id, const BCost* pCost, const BPopArray* pPops, long trainCount, long trainLimit, float trainPercent, long techPrereqID, bool unavail, const BUString& label, const BUString& infoText, const BUString& infoText2, const BUString& infoDetail, int itemType, int iconID, int ownerID, int unitStatProtoID);
      bool editCircleMenuItem(long order, long id, const BCost* pCost, const BPopArray* pPops, long trainCount, long trainLimit, float trainPercent, long techPrereqID, bool unavail, const BUString& label, const BUString& infoText, const BUString& infoText2, const BUString& infoDetail, int itemType, int iconID, int ownerID, int unitStatProtoID);
      void removeCircleMenuItem(long index);
      long getNumberCircleMenuItems() const;
      void refreshCircleMenu(bool bRefreshTrainProgressOnly);
      bool handleCircleMenuInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail);
      int  lookupIconID(const char* pIconType, const char* pIconName);
      int  getProtoIconCount() const;
      const BFlashProtoIcon* getProtoIcon(int index);
      void setCircleMenuExtraInfoVisible(bool bVisible);
      bool getCircleMenuExtraInfoVisible() const;

      void flashUIElement(int element, bool flashOn);

      void handlePlayerSwitch();

   private:

      bool initReticle(int civID, bool bSplitScreen);
      bool initHUD(int civID, bool bSplitScreen);

      void renderFlash();

      BUser*            mpParentUser;     
      BFlashReticle*    mpReticle;
      BFlashHUD*        mpHUD;

      int               mCivID;
      bool              mbFirstUpdate:1;
      bool              mbGameUIVisible:1;
      bool              mbReticleVisible:1;
};



