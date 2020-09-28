//============================================================================
// UIHints.h
// Ensemble Studios (c) 2007
//============================================================================

#pragma once 
#include "flashscene.h"
#include "flashgateway.h"
#include "d3dtexturemanager.h"
#include "visual.h"
#include "entity.h"

class BHintMessage;

//============================================================================
// class BUIHints
//============================================================================
class BUIHints : public BFlashScene
{
public:
   BUIHints();
   virtual ~BUIHints();

   // objective widget defs
   enum
   {
      cHintLocation0=0,
      cHintLocation1,
      cHintLocation2,
      cHintLocation3,
      cHintLocation4,

      cHintLocationCount,
   };

   enum 
   {
      cFlagInitialized = 0,
      cFlagRenderTargetReady,
      cFlagTotal
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
   void setHintsVisible(bool bVisible) { mHintPanelVisible=bVisible; };
   bool isHintsVisible() const { return mHintPanelVisible; };
   BFlashMovieInstance*  getMovie() { return mpMovie; }


   BManagedTextureHandle getRenderTargetTexture();

   // Hints Specific Methods
   void displayHint(BHintMessage* hintMessage);
   void hideHint();                             // This will probably be updated using a hint ID


private:
   // helper methods

   BFlashMovieInstance*       mpMovie;
   BFlashGateway::BDataHandle mDataHandle;

   // Hint data

   // Flags
   bool mHintPanelVisible : 1;
};