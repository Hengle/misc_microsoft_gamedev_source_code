//==============================================================================
// cinematicbars.h
//
// Copyright (c) 2008 Ensemble Studios
//==============================================================================

//============================================================================
// BCinematicBars
//============================================================================
class BCinematicBars
{
public:

   BCinematicBars();
   ~BCinematicBars();

   bool init();
   void reset();
   void update(float deltatime);
   void postrender();
   void fadeIn();
   void fadeOut();
   bool visible();

private:

   float mScreenSizeX;
   float mScreenSizeY;
   float mStartFactor;
   float mEndFactor;
   float mCurrentTime;

   bool mbPlaying;
   bool mbUIVisible;
};