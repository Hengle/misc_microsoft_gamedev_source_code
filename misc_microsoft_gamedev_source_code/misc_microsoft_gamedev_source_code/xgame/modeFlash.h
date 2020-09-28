// File: modeFlash.h
#pragma once

// Includes
#include "mode.h"
#include "ui.h"
#include "uilist.h"
#include "renderThread.h"
#include "threading\eventDispatcher.h"

class BFlashMovie;

//==============================================================================
// BModeFlash
//==============================================================================
class BModeFlash : public BMode, public BRenderCommandListener, public BEventReceiver
{
public:
   enum
   {
      cStateMain,
      cStateExit,
   };

   BModeFlash(long modeType);
   virtual           ~BModeFlash();

   virtual bool      setup();
   virtual void      shutdown();

   virtual void      preEnter(BMode* lastMode);
   virtual void      enter(BMode* lastMode);
   virtual void      leave(BMode* newMode);

   virtual void      renderBegin();
   virtual void      render();
   virtual void      renderEnd();
   virtual void      update();
   virtual bool      handleInput(long port, long event, long controlType, BInputEventDetail& detail);

   void              setNextState(long state) { mNextState=state; }

protected:
   long              mState;
   long              mNextState;

   BUIList           mList;
   long              mLastMainItem;
   
   BString           mHUDText;
      
   //BGFXMoviePlayer   mRenderMoviePlayer;

   BFlashMovie*      mpMovie;
   
   bool              mLoaded;
      
   struct BLoadMovieData
   {
      BFixedString256 mFilename;
   };
   
   class BChangeStatePayload : public BEventPayload
   {
   public:
      BChangeStatePayload(const char* pHUDText, bool loaded) :
         mHUDText(pHUDText),
         mLoaded(loaded)
      {
      }
      
      virtual void deleteThis(bool delivered)
      {
         delivered;
         delete this;
      }
      
      BString mHUDText;
      bool mLoaded;
   };
   
   enum
   {
      cECChangeState = cEventClassFirstUser
   };
      
   void workerLoadMovie(void* pData);
   void workerUnloadMovie(void* pData);
   void workerRenderMovie(void* pData);   

   virtual void initDeviceData(void);
   virtual void frameBegin(void);
   virtual void processCommand(const BRenderCommandHeader& header, const uchar* pData);
   virtual void frameEnd(void);
   virtual void deinitDeviceData(void);
   
   virtual bool receiveEvent(const BEvent& event, BThreadIndex threadIndex);
};


