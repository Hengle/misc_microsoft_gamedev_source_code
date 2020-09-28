//============================================================================
// UIControl.h
// Ensemble Studios (c) 2008
//============================================================================
#pragma once

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
#define DECLARE_CONTROL_TYPE( name )\
   static const char* name##Name = #name;\
   static const int name##ID = __COUNTER__ * 100;\
//////////////////////////////////////////////////////////////////////////
// CONTROL TYPE DECLARATIONS
// NEWCONTROLTYPE
//    1 - Add a DECLARE_CONTROL_TYPE to the list below
//    2 - In the .h for your new class, add a public enum of UIControlEvents,
//        starting with your new controlTypeID
//    3 - Use the IMPLEMENT_CONTROL_TYPE_ACCESSORS macro below your events enum
//    4 - Add a node to UIControlInput.xml with the name of your new control type
//////////////////////////////////////////////////////////////////////////

DECLARE_CONTROL_TYPE( UIControl );
DECLARE_CONTROL_TYPE( UIButtonBarControl );
DECLARE_CONTROL_TYPE( UIButtonControl );
DECLARE_CONTROL_TYPE( UICheckboxControl );
DECLARE_CONTROL_TYPE( UIFlyoutListControl );
DECLARE_CONTROL_TYPE( UIGamerTagControl );
DECLARE_CONTROL_TYPE( UIImageViewerControl );
DECLARE_CONTROL_TYPE( UIListControl );
DECLARE_CONTROL_TYPE( UIMenuItemControl );
DECLARE_CONTROL_TYPE( UIMoviePlayerControl );
DECLARE_CONTROL_TYPE( UIOptionsMenuItemControl );
DECLARE_CONTROL_TYPE( UIScrollableTextFieldControl );
DECLARE_CONTROL_TYPE( UISliderControl );
DECLARE_CONTROL_TYPE( UITabControl );
DECLARE_CONTROL_TYPE( UITalkingHeadControl );
DECLARE_CONTROL_TYPE( UIObjectiveControl );
DECLARE_CONTROL_TYPE( UILabelControl );
DECLARE_CONTROL_TYPE( UIScrollingListControl );
DECLARE_CONTROL_TYPE( UIPanelControl );
DECLARE_CONTROL_TYPE( UIGridControl );
DECLARE_CONTROL_TYPE( UIGridCellControl );
DECLARE_CONTROL_TYPE( UIScrollableCalloutControl );
DECLARE_CONTROL_TYPE( UIGameSetupPlayerControl );
DECLARE_CONTROL_TYPE( UITextFieldControl );
DECLARE_CONTROL_TYPE( UIDifficultyDisplayControl );
DECLARE_CONTROL_TYPE( UITwoColumnScrollTextFieldControl );
DECLARE_CONTROL_TYPE( UITimelineCalloutControl );
DECLARE_CONTROL_TYPE( UILeaderboardRecordControl );
DECLARE_CONTROL_TYPE( UICheckedMenuItemControl );
DECLARE_CONTROL_TYPE( UIObjectiveProgressControl );
DECLARE_CONTROL_TYPE( UITimelineEventControl );
DECLARE_CONTROL_TYPE( UIMiniTimelineEventControl );

//////////////////////////////////////////////////////////////////////////
#define IMPLEMENT_CONTROL_TYPE_ACCESSORS( name )\
public:\
   virtual int getControlTypeID( void ) const { return name##ID; }\
   virtual const char* getControlTypeName( void ) const { return name##Name; }\
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
#define BEGIN_EVENT_MAP( control )\
   virtual void initEventMap( void )\
   {\
      __super::initEventMap();\
      int controlID = control##ID;\
      if( sEventMap[controlID].size() == 0 )\
      {\

#define MAP_CONTROL_EVENT( eventName )\
         sEventMap[controlID][e##eventName] = #eventName;\

#define END_EVENT_MAP()\
      }\
   }\

#define INIT_EVENT_MAP()\
   initEventMap();\

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
#include "flashscene.h"
#include "flashgateway.h"
#include "xcore.h"

class BUIControl;
class BUIScreen;
class BUIInputHandler;
class GfxValue;
class BUIControlEvent;
class BXMLReader;

class IUIControlEventHandler
{
public:
   virtual bool handleUIControlEvent( BUIControlEvent& event ) = 0;
   virtual BFlashMovieInstance* getMovie( void ) = 0;

   virtual BUIScreen* getScreen( void ) { return NULL; }
   virtual bool isSoundEnabled( void ) { return true; }
   virtual bool shouldPlaySound( void ) { return true; }
};

class BUIControl : public IUIControlEventHandler, public IInputControlEventHandler
{
public:
   enum Events
   {
      eEnable = UIControlID,
      eDisable,
      eFocus,
      eUnfocus,
      eShow,
      eHide,
      eChildControlEvent,
      eStringEvent,
   };

public:
   BUIControl( void );
   virtual ~BUIControl( void );

   virtual bool init( IUIControlEventHandler* parent, const char* controlPath, int controlID = -1, BXMLNode* initData = NULL );
   
   // IUIControlEventHandler
   virtual bool handleUIControlEvent( BUIControlEvent& event );

   // IInputControlEventHandler 
   virtual bool executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl );
   virtual void enterContext( const char* contextName );
   virtual bool handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail);

   // Common UI Control functions
   virtual void enable( void );
   virtual void disable( void );

   virtual void focus( bool force=false );
   virtual void unfocus( bool force=false );

   virtual void show( bool force=false );
   virtual void hide( bool force=false );

   // Position and Scale
   virtual void setPosition( int x, int y );
   virtual void setSize( int x, int y );

   // Accessors
   virtual int getControlTypeID( void ) const = 0;
   virtual const char* getControlTypeName( void ) const = 0;
   virtual void setControlID( int controlID );
   virtual int getControlID( void ) const;
   virtual bool isEnabled( void ) const;
   virtual bool isFocused( void ) const;
   virtual bool isShown( void ) const;
   virtual const BString& getControlPath( void ) const;

   virtual BFlashMovieInstance* getMovie( void ); 
   virtual BUIScreen* getScreen( void );

   virtual void attachMovie( const char* id, const char* name );

   virtual void enableSound( bool enabled = true );
   virtual void disableSound( void );

   static const long cInvalidPort = -1;
   virtual long getInputPort( void ) const { return mInputPort; }

protected:
   bool initInput( BXMLNode* initData );
   bool loadDefaultControls( void );

   bool initSound( BXMLNode* initData );

   void fireUIControlEvent( int eventID, BUIControlEvent* childEvent = NULL );
   void fireUIControlEvent( const BCHAR_T* str, BUIControlEvent* childEvent = NULL );

   void invokeActionScript( const char* method, GFxValue* args = NULL, int numArgs = 0 );

   virtual void initEventMap( void );

   virtual bool getEventName( int eventID, BString& eventName, int controlTypeID = -1 );

   virtual bool isSoundEnabled( void );
   virtual bool shouldPlaySound( void );
   virtual bool playEventSound( BUIControlEvent& event );


   IUIControlEventHandler* mpParent;
   BString mControlPath;
   BString mScriptPrefix;
   BString mControlType;

   BUIInputHandler* mpInputHandler;
   long mInputPort;

   static BXMLReader spControlDefaults;

   int mControlID;

   bool mbEnabled;
   bool mbFocused;
   bool mbShown;

   bool mbSoundEnabled;

   BFlashMovieInstance* mpMovie;
   BUIScreen* mpScreen;

   typedef BHashMap<int, BString> EventIDNameMap;
   typedef BHashMap<int, EventIDNameMap > ControlIDEventsMap;
   static ControlIDEventsMap sEventMap;

   typedef BHashMap<BString, BString> SoundMap;
   SoundMap mSoundMap;
};

class BUIControlEvent
{
public:
   BUIControlEvent( BUIControl* control, int id, BUIControlEvent* childEvent = NULL );
   BUIControlEvent( BUIControl* control, int id, const BCHAR_T* str );
   BUIControlEvent( BUIControl* control, int id, BUIControlEvent* childEvent, const char* str );

   ~BUIControlEvent( void );

   BUIControl* getControl( void ) const;
   int getID( void ) const;
   const BUIControlEvent* getChildEvent( void ) const;
   const BString& getString( void ) const { return mString; }
   bool hasSoundPlayed( void ) const { return mSoundPlayed; }
   void setSoundPlayed() { mSoundPlayed = true; }
private:
   BUIControl* mControl;
   int mID;
   BUIControlEvent* mChildEvent;
   BString mString;
   bool mSoundPlayed;
};


