//============================================================================
// uiwidgets.h
// Ensemble Studios (c) 2007
//============================================================================

#pragma once 
#include "flashscene.h"
#include "flashgateway.h"
#include "d3dtexturemanager.h"
#include "visual.h"
#include "entity.h"
#include "UIButtonBar2.h"

#include "UITalkingHeadControl.h"
#include "UIObjectiveProgressControl.h"

#include "gamefilemacros.h"

//============================================================================
// class BUIWidgets
//============================================================================
class BUIWidgets : public BFlashScene, public IUIControlEventHandler
{
public:
   BUIWidgets();
   virtual ~BUIWidgets();

   // objective widget defs
   enum
   {
      cObjectiveWidget1=1,
      cObjectiveWidget2,
      cObjectiveWidget3,
      cObjectiveWidget4,
      cObjectiveWidget5,
   };

   enum 
   {
      cObjectiveWidgetIconOff=0,
      cObjectiveWidgetIconRadioActive,
      cObjectiveWidgetIconPeople,
      cObjectiveWidgetIconGrey,
   };

   enum
   {
      cObjectiveWidgetProgressTypeMissle=1,
      cObjectiveWidgetProgressTypeBar,
   };

   enum 
   {
      cFlagInitialized = 0,
      cFlagRenderTargetReady,
      cFlagTotal
   };

   enum
   {
      cReticulePointerTypeNone=0,      // disabled
      cReticulePointerTypeUnit,
      cReticulePointerTypeSquad,
      cReticulePointerTypeArea,
   };

   enum     
   {
      // misc. constants
      cNumGarrisonContainers=3,
      cNumReticulePointers=5,    // <- Note: *** if you change this value, change the action script also ***
      cNumCounterObjects=10,
      cTimerUpdateThreshold=100,
   };

   enum
   {
      cMiniGameCirleInner=0,
      cMiniGameCirleOuter,

      cNumMinigameCircles,

      cMaxMinigameButtons=5,
      cMaxMinigameMarkers=5,
      cMaxMinigameTargets=5,
   };

   enum
   {
      cMinigameTargetColorOff=0,
      cMinigameTargetColorBlue,
      cMinigameTargetColorYellow,
      cMinigameTargetColorRed,
      cMinigameTargetColorGreen,
   };

   enum
   {
      cMinigameButtonTypeA=0,
      cMinigameButtonTypeB,
      cMinigameButtonTypeX,
      cMinigameButtonTypeY,

      cMinigameButtonTypeCount,
   };

   // Overhead functions
   bool init(const char* filename, const char* datafile);
   void deinit();
   void enter();
   void leave();

   void update(float elapsedTime);
   void renderBegin();
   void render();
   void renderEnd();
   bool handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail);
   bool receiveEvent(const BEvent& event, BThreadIndex threadIndex);
   void setDimension(int x, int y, int width, int height);
   void setWidgetsVisible(bool bVisible);
   bool isWidgetsVisible() const { return mWidgetPanelVisible; };


   BManagedTextureHandle getRenderTargetTexture();

   //-- Timer Display
   void setTimerVisible(bool bVisible, int timerID=-1, int timerLabelID=-1, float startingElapsed=0.0f);
   void updateTimer(float elapsedTime);
   void renderTimer();
   void flashTimer();
   bool getTimerVisible() { return mTimerVisible; }
   void showTimer( void );
   void hideTimer( void );
   bool getTimerShown() const { return mTimerShown; }

   //-- Garrisoned in - 3 UI widgets.
   void setGarrisonedVisible(int id, bool bVisible, BEntityID container);
   void setGarrisonedVisible(int id, bool bVisible, int numGarrisoned);
   void updateGarrisoned();
   void flashGarrisoned(int id);
   void renderGarrisoned();

   //-- Citizens Saved Widget
   void setCitizensSavedVisible(bool bVisible);
   void setCitizensSaved(int total, int totalNeeded);
   void updateCitizensSaved();
   void renderCitizensSaved();

   //-- Objective reticule pointers
   void setReticulePointerVisible(int id, bool bVisible, BEntityID container, int type);
   void setReticulePointerVisible(int id, bool bVisible, BVector area);
   void updateReticulePointer();
   void renderReticulePointer();

   // Object counter (0-10)
   void setCounterVisible(bool bVisible, int numberObjects, int initialValue);
   void updateCounter(int newValue);
   int getCounterMax() { return mCounterMax; }

   // User Message stuff
   void displayUserMessage(const BUString& message);
   void setUserMessageVisible(bool bVisible, bool bForce = false);

   // IUIControlEventHandler
   virtual bool handleUIControlEvent( BUIControlEvent& event );
   BFlashMovieInstance*  getMovie() { return mpMovie; }

   BUITalkingHeadControl& getTalkingHead() { return mTalkingHeadScene.mTalkingHeadControl; }
   BUIObjectiveProgressControl& getObjectiveTracker() { return mObjectiveProgressControl; }

   void showObjectiveWidgets( void );
   void hideObjectiveWidgets( void );
   bool getObjectiveWidgetsShown() const { return mObjectiveWidgetsShown; }

   GFDECLAREVERSION();
   bool save(BStream* pStream, int saveType) const;
   bool load(BStream* pStream, int saveType);

private:
   // helper methods
   void setObjectiveWidgetVisible(int id, bool bVisible, int icon, BUString* pLabel = NULL);
   void setObjectiveWidgetText(int id, const BUString& text);
   void setObjectiveWidgetBar(int id, int type, int barsActive);
   void updateGarrisonedUnit(int id, bool force);
   void updateTimerWidget();


   BFlashMovieInstance*       mpMovie;
   BFlashGateway::BDataHandle mDataHandle;

   // Widget data
   
   // Object counter stuff
   int                  mCounterCurrent;
   int                  mCounterMax;
   BUString             mCounterString;

   // progress bar
   int                  mProgressBarMax;        // 0 - max, integer based.

   // hitpoint bar data.
   BEntityID            mHitpointBarEntity;
   int                  mLastHitpoints;

   // Timer data
   int                  mTimerID;
   int                  mTimerLabelID;
   BSimUString          mTimerLabel;
   DWORD                mLastTimerTime;
   BSimUString          mTimerString;
   float                mElapsedTimerTime;

   // Citizen and garrison counts
   int                  mNumCitizensSaved;
   int                  mNumCitizensNeeded;
   BUString             mCitizensSavedString;
   bool                 mGarrisonContainerVisible[cNumGarrisonContainers];
   BEntityID            mGarrisonContainerEntities[cNumGarrisonContainers];
   bool                 mGarrisonContainerUseEntity[cNumGarrisonContainers];
   int                  mGarrisonContainerCounts[cNumGarrisonContainers];
   BUString             mGarrisonContainerStrings[cNumGarrisonContainers];
   

   // reticule pointer
   bool                 mReticulePointersVisible[cNumReticulePointers];
   int                  mReticulePointerType[cNumReticulePointers];

   // Note: - If cReticulePointerTypeUnit or cReticulePointerTypeSquad, then use the value in mReticulePointersEntities
   //       - If cReticulePointerTypeArea, then use the value in mReticulePointerArea
   BVector              mReticulePointerArea[cNumReticulePointers];
   BEntityID            mReticulePointerEntities[cNumReticulePointers];
   int                  mPointerRotation[cNumReticulePointers];
   float                mPointerRotationFloat[cNumReticulePointers];

   // Cached minigame values;
   int                  mMinigameRotationCache[cNumMinigameCircles];
   float                mMinigameRotationFloat[cNumMinigameCircles];

   BUIButtonBar2        mBtnBar;
   BUITalkingHeadScene mTalkingHeadScene;
   BUIObjectiveProgressControl mObjectiveProgressControl;
   
   // Flags
   bool mWidgetPanelVisible   :1;
   bool mHitpointBarVisible   :1;
   bool mProgressBarVisible   :1;
   bool mTimerVisible         :1;
   bool mCitizensSavedVisible :1;
   bool mCounterVisible       :1;
   bool mMiniGameVisible      :1;
   bool mGameMenuBackgroundVisible:1;
   bool mTalkingHeadVisible   :1;
   bool mbFirstRender : 1;
   bool mTimerShown : 1;
   bool mObjectiveWidgetsShown : 1;
};