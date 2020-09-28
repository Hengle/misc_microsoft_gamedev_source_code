//============================================================================
// uiwidgets.h
// Ensemble Studios (c) 2007
//============================================================================

#pragma once 
#include "flashscene.h"
#include "flashgateway.h"

class BUIButtonBar2
{
public:
   enum
   {
      cFlashButtonOff,
      cFlashButtonY,
      cFlashButtonA,
      cFlashButtonB,
      cFlashButtonX,
      cFlashButtonBack,
      cFlashButtonStart,
      cFlashButtonLeftTrigger,
      cFlashButtonRightTrigger,
      cFlashButtonLRTrigger,
      cFlashButtonLeftButton,
      cFlashButtonRightButton,
      cFlashButtonLRButton,
      cFlashButtonLStick,
      cFlashButtonRStick,
      cFlashButtonLRDPad,
      cFlashButtonUDDPad,
      
      cFlashButtonDpad,
      cFlashButtonlrLStick,
      cFlashButtonudLStick,
      cFlashButtonlrRStick,
      cFlashButtonudRStick,


      cFlashButtonStateCount,
   };

   BUIButtonBar2() : mpMovie(NULL) {};
   ~BUIButtonBar2() {};

   void init(BFlashMovieInstance* movie) { mpMovie = movie; }
   void deinit() { mpMovie = NULL; }

   void setButtonStates(uint state1, uint state2, uint state3, uint state4, uint state5, uint state6, uint state7);
   void setButtonTexts(const BUString& text1,
                        const BUString& text2,
                        const BUString& text3,
                        const BUString& text4,
                        const BUString& text5,
                        const BUString& text6,
                        const BUString& text7 );

   void     setMovieClip(const char * movieClipName) { mMovieClipName.set(movieClipName); }

protected:

   // helper methods
   void getASName(BSimString& fullName, const char* methodName);
   void invokeActionScript(const char* method, const GFxValue* pArgs, int numArgs);

   // fields
   BFlashMovieInstance* mpMovie;
   BSimString           mMovieClipName;

};
