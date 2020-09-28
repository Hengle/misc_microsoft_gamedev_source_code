//============================================================================
// UICallouts.h
// Ensemble Studios (c) 2007
//============================================================================

#pragma once 
#include "flashscene.h"
#include "flashgateway.h"
#include "d3dtexturemanager.h"
#include "visual.h"
#include "entity.h"


class BUICallout 
{


public:
   BUICallout::BUICallout():mVisible(false),mX(0),mY(0){};

   enum 
   {
      cCalloutTypeLocation=0,
      cCalloutTypeObject,
      cCalloutTypeUnit,
      cCalloutTypeSquad,
   };

   bool save(BStream* pStream, int saveType);
   bool load(BStream* pStream, int saveType);

   int      mID;              // Unique ID to give to the trigger system to identify this callout
   uint     mType;            // type of callout (see above);

   BVector  mLocation;        // if type location
   BEntityID mEntityID;       // if type squad or object
   BEntityID mCalloutEntityID;   // entityID of the 3D pointer

   long     mLocStringIndex;   // Index into the string table for this callout
   int      mUICalloutID;     // ties this to a specific callout

   int      mX;               // cache of its current position
   int      mY;   
   bool     mVisible;         // cache of its current visibility
};

//============================================================================
// class BUICallouts
//============================================================================
class BUICallouts : public BFlashScene
{
public:
   BUICallouts();
   virtual ~BUICallouts();

   // objective widget defs
   enum
   {
      cNumCallouts=5,
   };

   enum 
   {
      cFlagInitialized = 0,
      cFlagRenderTargetReady,
      cFlagTotal,
   };

   // Overhead functions
   bool init(const char* filename, const char* datafile);
   void deinit();
   void enter();
   void leave();
   BFlashMovieInstance*  getMovie() { return mpMovie; }

   void update(float elapsedTime);
   void renderBegin();
   void render();
   void renderEnd();
   bool handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail);
   bool receiveEvent(const BEvent& event, BThreadIndex threadIndex);
   void setDimension(int x, int y, int width, int height);
   void setCalloutsVisible(bool bVisible) { mPanelVisible = bVisible; };
   bool isCalloutsVisible() const { return mPanelVisible; };

   BManagedTextureHandle getRenderTargetTexture();

   //-- In-world callout methods
   int addCallout(BEntityID entity, uint type, long locStringID);
   int addCallout(BVector area, int type, long locStringID);
   bool removeCallout(int id);

   GFDECLAREVERSION();
   bool save(BStream* pStream, int saveType);
   bool load(BStream* pStream, int saveType);

private:
   BFlashMovieInstance*       mpMovie;
   BFlashGateway::BDataHandle mDataHandle;

   BDynamicArray<BUICallout*> mCallouts;

   bool updateCallouts();  // returns true if we should render
   void setCalloutPosition(BUICallout* pCallout, int x, int y);
   void setCalloutVisible(BUICallout* pCallout, bool bVisible);
   void setCalloutText(BUICallout* pCallout);

   bool createCalloutObject(BUICallout* pCallout, BVector position);
   bool assignCalloutWidget(BUICallout* pCallout);
   bool freeCalloutWidget(BUICallout* pCallout);


   long mCalloutWidgets[cNumCallouts];

   // Flags
   int mWidth;
   int mHeight;
   int mX;
   int mY;

   int mNextCalloutID;
   bool mPanelVisible   : 1;
   bool mCalloutsVisible : 1;
};