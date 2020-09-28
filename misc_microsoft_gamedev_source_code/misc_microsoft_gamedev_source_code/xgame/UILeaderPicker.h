//============================================================================
// uiLeaderPicker.h
// Ensemble Studios (c) 2007
//============================================================================

#pragma once 
#include "flashscene.h"
#include "flashgateway.h"
#include "d3dtexturemanager.h"
//#include "visual.h"
#include "entity.h"
#include "UIButtonBar.h"
#include "UITextFieldControl.h"

// Forward declarations
class BUIInputHandler;

class BLeaderPickerData
{
   public:
      enum 
      {
         cLeaderPickerCiv0=0,
         cLeaderPickerCiv1=1,
         cLeaderPickerRandomCiv=2,
      };
      int8  mFlashCivID;
      int8  mCiv;
      int8  mLeader;
      BSimString mImageName;
      long  mNameLocID;
      long  mDescriptionLocID;
};

class ILeaderPickerEventHandler
{
public:
   // this may change
   virtual bool leaderPickerEvent(const BSimString& command)=0;
};

//============================================================================
// class BUILeaderPicker
//============================================================================
class BUILeaderPicker : public BFlashScene, public IInputControlEventHandler, IUIControlEventHandler
{
public:
   BUILeaderPicker();
   virtual ~BUILeaderPicker();

   enum 
   {
      cFlagInitialized = 0,
      cFlagRenderTargetReady,
      cFlagTotal
   };

   enum BHUDASFunctions
   {
      eASFunctionSetSelectedLeader=0,
      eASFunctionEaseIn,
      eASFunctionInitLeaders,
      eASFunctionLoadLeader,
      eASFunctionChangeSelection,
      eASFunctionSetTitle,

      eASFunctionTotal,
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
   // BFlashMovieInstance*  getMovie() { return mpMovie; }


   BManagedTextureHandle getRenderTargetTexture();

   void setEventHandler(ILeaderPickerEventHandler* eventHandler) { mpEventHandler = eventHandler; }
   
   void setIsVisible(bool bVisible) { mPanelVisible = bVisible; };
   bool isVisible() const { return mPanelVisible; };

   void show();
   void hide();

   void setCurrentLeader(int8 civ, int8 leader);
   const BLeaderPickerData* getCurrentLeader();

   void setPreviousLeader(int8 civ, int8 leader);
   void setNextLeader(int8 civ, int8 leader);

   // ----- IInputControlEventHandler - callback handler
   bool executeInputEvent(long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl);

   // ----- IUIControlEventHandler
   virtual bool handleUIControlEvent( BUIControlEvent& event ) { return false; }
   virtual BFlashMovieInstance* getMovie( void ) { return mpMovie; }


private:
   // helper methods
   BFlashMovieInstance*       mpMovie;
   BFlashGateway::BDataHandle mDataHandle;

   BUIInputHandler*           mpInputHandler;
   ILeaderPickerEventHandler* mpEventHandler;

   // control the button bar on the bottom
   BUIButtonBar               mBtnBar;
   BUITextFieldControl        mTitle;

   int8 mSelectedLeader;

   void easeIn(bool easeIn);

   void setTitle(const BUString& title);
   void initLeaders(int8 firstLeaderSlot, int8 numLeaders);
   void loadLeader(int8 slot, const BSimString& leaderImage, int8 civ, const BUString& leaderName, const BUString& leaderDescription);
   void setSelectedLeader(int8 slot);
   bool changeLeaderSelection(bool goRight);

   BDynamicSimArray<BLeaderPickerData> mLeaderData;

   // Flags
   bool mPanelVisible   : 1;
   bool mInputWaitForStart : 1;
};