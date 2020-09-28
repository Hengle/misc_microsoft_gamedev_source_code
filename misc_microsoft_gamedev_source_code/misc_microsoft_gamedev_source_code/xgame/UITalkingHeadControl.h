//============================================================================
// UITalkingHeadControl.h
// Ensemble Studios (c) 2008
//============================================================================
#pragma once
#include "UIControl.h"
#include "binkInterface.h"
#include "UIScreen.h"
#include "gamefilemacros.h"

// Control
class BUITalkingHeadControl : public BUIControl
{
public:
   enum Events
   {
      eTalkingHeadControlEvent = UITalkingHeadControlID,
   };
   IMPLEMENT_CONTROL_TYPE_ACCESSORS( UITalkingHeadControl );

public:
   BUITalkingHeadControl( void );
   virtual ~BUITalkingHeadControl( void );

   virtual bool executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl );

   virtual void show( bool force = false );
   virtual void hide( bool force = false );
   void update( void );

   void setShowBackground(bool bShowBackground);
   bool getShowBackground() { return mShowBackground; }

   void showTalkingHead(BBinkVideoHandle videoHandle);
   void hideTalkingHead();

   void showObjectiveText(long objectiveID);
   void hideObjectiveText(bool easeOut = true);
   void showTalkingHeadText(const BUString& talkingHeadText);
   void hideTalkingHeadText();

   void playTextShowSound();

   long getObjectiveID() { return mObjectiveID; }

   BBinkVideoHandle getVideoHandle( void ) { return mVideoHandle; }

   GFDECLAREVERSION();
   bool save(BStream* pStream, int saveType) const;
   bool load(BStream* pStream, int saveType);

   void renderVideo( void );

protected:

   BUString mTalkingHeadText;
   BBinkVideoHandle mVideoHandle;
   long mObjectiveID;
   long mLastCount;
   bool mShowBackground;
   bool mObjectiveVisible;
   bool mTalkingHeadVisible;
};

// Screen
class BUITalkingHeadScene : public BUIScreen
{
public:
   bool init( const char* filename, const char* datafile );
   bool init(BXMLNode dataNode);

   BUITalkingHeadControl mTalkingHeadControl;
};