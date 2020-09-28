//============================================================================
// miniLoadManager.h
// Ensemble Studios (C) 2007
//
// 11/21/2008
// This class was added to fix TCR #6 (Non-Interactive Pause) bugs.  We use it
// to render a lightweight flash loading screen during our several second delays
// when we transfer modes.  It can be used in two ways:
//
// 1) By calling 'start()' and 'stop()' which will kick off a new thread which
// contains a tight loop for drawing and presenting.  The device MUST be free
// in this case as the thread that is kicked off will aquire and release it.  It
// is used in this way by the flashBackgroundPlayer.  When the
// flash background player is started from the main thread, it will tell the
// render thread to release the device, and then it will release it itself.  This
// will allow the device to be free when 'start()' is called.
//
// 2) By calling 'draw()' or 'drawTimed()' yourself.  If you call these yourself
// the calling thread MUST own the device.  It is used this way by the renderThread
// when it is put into it's mini load screen rendering mode.  The renderThread
// has it's own update loop (the function 'commandLoop') so it can simply call
// draw; it does not need to launch a separate thread to do this.
//============================================================================

#pragma once

class BGFXMoviePlayer;
class BGFxMovieManager;

class MiniLoadManager
{
public:

   MiniLoadManager();

   // Initializes the GFX objects.  Must be called before calling draw or drawTimed, but is called automatically by start
   void init();

   // Deinitializes the GFX objects.  
   void deinit();

   // Starts the render loop thread.  The calling thread MUST ensure the device is free as the thread that is created
   // will attempt to aquire it
   void start();

   // Stops the render loop thread. 
   void stop();

   // clears d3d, draws the mini loading screen and calls present.  The calling thread MUST have device ownership.
   void draw();

   // calls draw no more then every DRAW_TIMED_MILLISECONDS 
   void drawTimed();
   
   // threadsafe functions for getting and setting 'shouldContinue'.  Stop uses this bool as a way to terminate the
   // thread that start creates.
   void setShouldContinue(bool shouldCont);
   bool getShouldContinue();

private:

   HANDLE               mThreadHandle;
   bool                 mShouldContinue;

   BGFXMoviePlayer*     mpMoviePlayerMiniLoad;
   BGFxMovieManager*    mpMovieManagerMiniLoad;
   
   static BCriticalSection mShouldContinueLock;

};

// The function that is executed on a new thread.  Contains a simple render loop.
DWORD __stdcall MiniLoadManagerThreadCallback(LPVOID lpThreadParameter);

extern MiniLoadManager gMiniLoadManager;
