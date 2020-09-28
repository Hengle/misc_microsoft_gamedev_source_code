//============================================================================
// uimanager.h
// 
// Copyright (c) 2006-2008 Ensemble Studios
//============================================================================

#pragma once

#include "flashhud.h"
#include "flashdecal.h"
#include "flashminimap.h"
#include "flashreticle.h"
#include "UICampaignObjectives.h"
#include "UISkirmishObjectives.h"
#include "UIWidgets.h"
#include "UICallouts.h"
#include "uihints.h"
#include "sceneLightManager.h"
#include "uigame.h"
#include "uicontext.h"
#include "UIEndGame.h"
#include "gamefilemacros.h"
#include "binkvideo.h"
#include "render2Dprimitiveutility.h"
#include "UIGlobals.h"
#include "mptypes.h"
#include "UIInfoDialog.h"

class BSquad;
class BUnit;
class BUICampaignPostGameScreen;
class BUISkirmishPostGameScreen;
class BUIGameMenu;
class BUIPauseDialog;

class BUIManager : public BUIGlobals::yornHandlerInterface, public IUIScreenHandler
{
   public:
      BUIManager();
      ~BUIManager();
      
      enum BUIFlag
      {
         eFlagWireframe = 0,
         eFlagTotal
      };

      bool init();
      bool initPlayerSpecific( bool bForceMinimapVisible = false );
      bool initAfterLoad();
      void deinit();
      void deinitPlayerSpecific();

      BUIContext* getOrCreateContext(BUser* pUser);
      void        releaseContext(BUIContext* pContext);

      void update(float elapsedTime);
      void updateRenderThread(double gameTime);
      
      void renderUser();
      void renderMinimap();
      void renderWidgets();
      void render2DPrimitives();

      void releaseGPUHeapResources();

      void workerRenderDecals();
      void workerRenderMinimapIconResources();

      void renderModel(BVisual* pVisual, BMatrix& worldMatrix, BManagedTextureHandle textureHandle, DWORD tintColor = cDWORDWhite, BDirLightParams* pDirLightParams=NULL);
      void handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail);

      void refreshUI();

      // Player Score
      void setScoresVisible(bool bON);

      void setPlayerScore(int playerID, const char* text);
      void setPlayerColor(int playerID, int color);                              

      bool isPowerOverlayVisible() const;
      void setPowerOverlayVisible(int powerID, bool bON);            
      void renderReticle();

      void renderCampaignObjectives();
      void renderSkirmishObjectives();

      void setUnitSelectionVisible(bool bVisible);
      void setGameTimeVisible(bool bVisible);      
      void setResourcePanelVisible(bool bVisible);      
      void setDPadPanelVisible(bool bVisible);  
      void setButtonPanelVisible(bool bVisible);
      void setReticleVisible(bool bVisible);

      int getDecalFlashMovieIndex(int decalType) const;
      BManagedTextureHandle getDecalTextureHandle(BFlashPropertyHandle handle) const;
      void getDecalTextureUV(int decalType, XMHALF4& uv, XMHALF4& uv2);
      bool getDecalSize(int protoID, float& sizeX, float& sizeY, float& sizeZ);
      int  getDecalProtoIconIndex(int protoID);
      const BFlashProtoIcon* getDecalProtoIcon(int index);
      void setEnableDecals(bool bEnable) { mbEnableDecals = bEnable; };
      bool getEnableDecals() const { return mbEnableDecals; };

      //-- Cinematics
      void setupCinematicUI(bool playCinematic);

      //-- Pause
      //void setPaused(bool paused, PlayerID playerID);

      void setScenarioResult( long result ) { mScenarioResult = result; }
      long getScenarioResult( void ) { return mScenarioResult; }

      //-- UI widgets
      BUIWidgets* getWidgetUI() { return mpUIWidgets; }
      void setWidgetsVisible(bool bVisible);
      bool isWidgetsVisible() const;

      //-- UI Callouts
      BUICallouts* getCalloutUI() { return mpUICallouts; }
      void setCalloutsVisible(bool bVisible);
      bool isCalloutsVisible() const;


      //-- Chat
      BBinkVideoHandle displayChat(BChatMessage* pChat, BBinkVideoStatus* pStatusCallback);
      void setChatVisible(bool bVisible);

      //-- Objective popups
      void showIngameObjective(bool bShow, long objectiveID = -1);
      void refreshObjective(long objectiveID);
      void hideIngameObjectives(bool easeOut);


      //-- Hints
      void displayHint(BHintMessage* pHint);
      void hideHint();
      void setHintsVisible(bool bVisible);      // stops the render of all hints
            
      //-- minimap interface
      void setMinimapRotationOffset(float degrees);
      void setMinimapVisible(bool bVisible);
      bool getMinimapVisible();
      bool getMinimapFullZoomOut() const;
      void setMinimapFullZoomOut(bool bFullZoomOut);
      void setMinimapMapTexture(BManagedTextureHandle mapHandle);
      void setMinimapSkirtMirroring(bool value);
      float getMinimapMapViewCenterX() const;
      float getMinimapMapViewCenterY() const;
      float getMinimapMapViewWidth() const;
      float getMinimapMapViewHeight() const;
      BFlashMinimap* getMinimap() { return mpMinimap; }
      void resetMinimap();
      void commitMinimap();
      void addMinimapIcon(BObject* pObject);
      void addMinimapIcon(BUnit* pUnit);      
      void addMinimapIcon(BSquad* pSquad); // [6/27/2008 xemu] removed platoon minimap icon since platoons are too transient, replaced with squad 
      void addMinimapFlare(BVector pos, int playerID, int flareType);
      void addMinimapAlert(BVector pos);
      void generateMinimapVisibility();
      void revealMinimap(const BVector& position, float los);
      void revealMinimap(const BVector& position, float width, float height);
      void blockMinimap(const BVector& position, float los);
      void blockMinimap(const BVector& position, float width, float height);  

      bool getFlag(int n) const { return(mFlags.isSet(n)!=0); }
      void setFlag(int n, bool v) { if(v) mFlags.set(n); else mFlags.unset(n); }

      GFDECLAREVERSION();
      bool save(BStream* pStream, int saveType);
      bool load(BStream* pStream, int saveType);

      // 2D Primitive rendering
      void clear2DPrimitives();
      void addPieProgress(const BMatrix& matrix, BManagedTextureHandle texture, DWORD color, float value, float scaleX, float scaleY, float offsetX, float offsetY, bool fill, bool clockwise, int layer, float timeout);
      void addSprite(const BMatrix& matrix, BManagedTextureHandle texture, DWORD color, BRender2DPrimitiveUtility::eRender2DBlendMode blendmode,  float scaleX, float scaleY, float offsetX, float offsetY, int layer, float timeout);

      // BUIGlobals::yornHandlerInterface
      virtual void yornResult(uint result, DWORD userContext, int port);
      
		// IUIScreenHandler
      virtual void handleUIScreenResult( BUIScreen* pScreen, long result );
      void handleGameMenuResult( long result );
      void handleEndGameScreenResult( long result );
      void handlePostGameStatsScreenResult( long result );

      // Non-Game UI Methods
      enum EResult 
      { 
         cResult_Resume, 
         cResult_Resign, 
         cResult_Restart, 
         cResult_Exit, 
         cResult_Continue,
         cResult_GoToAdvancedTutorial,
      };

      enum EScreen 
      { 
         cObjectivesScreen, 
         cGameMenu, 
         cEndGameScreen, 
         cPostGameStatsScreen, 
         cCampaignInfoDialog, 
         cMaxUIScreens 
      };

      bool isNonGameUIVisible( void );
      bool showNonGameUI( EScreen eScreen, BUser* pUser = NULL );
      bool hideNonGameUI( bool bRestoreInGameUI = true );
      void updateNonGameUI( float elapsedTime );
      bool handleNonGameUIInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail);
      void renderNonGameUI( void );
      BUser* getCurrentUser( void );
      EScreen getCurrentScreenEnum( void );
      void hideInGameUI( void );
      void restoreInGameUI( void );
      BUIScreen* getNonGameUIScreen( EScreen eScreen );

      void BUIManager::handlePlayerSwitch();

   private:
      bool initDecals(int civID);
      void deinitDecals();
      void updateDecals(float elapsedTime);
      void releaseMinimapGPUHeapTextures();
      void releaseDecalGPUHeapTextures();
      
      void showPauseDialog();
      void hidePauseDialog();
      
   protected:            
      int               mCivID;
      BDynamicSimArray<BUIContext*> mUserContexts;
      
      long mGameType;

      // Non-Game UI Screens
      EScreen getScreenEnum( BUIScreen* pScreen );
      BUIScreen* mScreens[cMaxUIScreens];
      BUIScreen* mpCurrentScreen;
      BUser* mpCurrentUser;
      
      long mScenarioResult;

      // HUD UI Stuff
      BUIHints*         mpUIHints;
      BUIWidgets*       mpUIWidgets;
      BUICallouts*      mpUICallouts;
      BFlashDecal*      mpDecal;
      BFlashMinimap*    mpMinimap;
      BUIInfoDialog*    mpInfoDialog;
      
      
      // 2D Helper Primitives
      template<class T> void updatePrimitives(T& list, double curTime);
      template<class T> void clearPrimitives(T& list, int category);
      float createTimeout(float durationInSeconds);
      BDynamicArray<B2DPrimitivePie, 16> mPiePrimitives;
      BDynamicArray<B2DPrimitiveSprite, 16> mSpritePrimitives;

      bool              mbFirstUpdate:1;
      bool              mbMinimapVisible:1;
      bool              mbReticleVisible:1;
      bool              mbEnableDecals:1;
      bool              mbGamePaused:1;
      bool              mbUnpauseOnHideGameMenu:1;
      UTBitVector<32>   mFlags;
};
extern BUIManager* gUIManager;