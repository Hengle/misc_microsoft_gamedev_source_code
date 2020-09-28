//============================================================================
// UIControl.cpp
// Ensemble Studios (c) 2008
//============================================================================
#include "common.h"
#include "UIControl.h"
#include "UIInputHandler.h"
#include "UIScreen.h"
#include "gamedirectories.h"
#include "soundmanager.h"

BXMLReader BUIControl::spControlDefaults;
BUIControl::ControlIDEventsMap BUIControl::sEventMap;

//==============================================================================
//==============================================================================
BUIControl::BUIControl( void ) : 
   mpParent( NULL ),
   mpInputHandler( NULL ),
   mbEnabled( true ),
   mbFocused( false ),
   mbShown( true ),
   mpMovie( NULL ),
   mpScreen( NULL ),
   mbSoundEnabled( true ),
   mInputPort( cInvalidPort )
{
}

//==============================================================================
//==============================================================================
BUIControl::~BUIControl( void )
{
   if (mpInputHandler)
      delete mpInputHandler;
}

//==============================================================================
//==============================================================================
bool BUIControl::init( IUIControlEventHandler* parent, const char* controlPath, int controlID, BXMLNode* initData /*= NULL*/ )
{
   BASSERT( parent );
   mpParent = parent;

   BASSERT( controlPath );
   mControlPath = controlPath;
   mScriptPrefix = mControlPath + ".";

   mControlID = controlID;

   // LOAD INIT DATA FROM XML NODE HERE!

   return initInput( initData ) && initSound( initData );
}

//============================================================================
// Example of a node that we can send it.
// Basically the scene calling this init function has to make sure that it is
//    passing in a root node that matches this pattern.
// <SomeRootNode> (initData)
//    <Controls>
//       <Mode name="Main">
//          <Control name="StickRightUp"         command="scrollUp"   event="start stop" digital="" threshold="0.90" sound="" />
//          <Control name="StickRightDown"       command="scrollDown" event="start stop" digital="" threshold="0.90" sound="" />
// 
//          <Control name="ButtonY"              command="cancel"     event="start" sound="" />
//       </Mode>
//    </Controls>
// </SomeRootNode>
//============================================================================
bool BUIControl::initInput( BXMLNode* initData )
{
   mpInputHandler = new BUIInputHandler();

   bool success = false;
   BXMLNode controlsNode;
   if (initData && initData->getChild( "controls", &controlsNode ) )
   {
      success = mpInputHandler->loadControls( controlsNode, this );
   }
   else
   {
      success = loadDefaultControls();
   }

   if (success)
      mpInputHandler->enterContext("Main");

   return true;
}

//============================================================================
//============================================================================
bool BUIControl::initSound( BXMLNode* initData )
{
   BXMLNode soundsNode;
   if( !initData || !initData->getChild( "sounds", &soundsNode ) )
   {
      if( !spControlDefaults.getValid() )
      {
         if( !spControlDefaults.load( cDirData, "uiControlDefaults.xml" ) )
         {
            return false;
         }
      }

      // Look for our control
      BXMLNode controlNode;
      const char* pControlType = getControlTypeName();
      if( spControlDefaults.getRootNode().getChild( pControlType, &controlNode ) )
      {
         controlNode.getChild( "sounds", &soundsNode );
      }
   }
   
   if( soundsNode.getValid() )
   {
      int numNodes = soundsNode.getNumberChildren();
      for( int i = 0; i < numNodes; ++i )
      {
         BXMLNode soundNode = soundsNode.getChild( i );
         if( soundNode.getValid() )
         {
            BString soundPath;
            soundNode.getText( soundPath );
            mSoundMap[soundNode.getName().getPtr()] = soundPath;
         }
      }
   }
   return true;
}

//==============================================================================
//==============================================================================
bool BUIControl::handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail)
{
   bool result = false;
   mInputPort = port;
   
   if (mpInputHandler)
      result = mpInputHandler->handleInput(port, event, controlType, detail);

   mInputPort = cInvalidPort;

   return result;
}


//==============================================================================
// IUIControlEventHandler
//==============================================================================
bool BUIControl::handleUIControlEvent( BUIControlEvent& event )
{
   fireUIControlEvent( eChildControlEvent, &event );
   return false;
}

//============================================================================
//============================================================================
bool BUIControl::getEventName( int eventID, BString& eventName, int controlTypeID )
{
   if( controlTypeID == -1 )
      controlTypeID = getControlTypeID();

   ControlIDEventsMap::const_iterator typeIt = sEventMap.find( controlTypeID );
   if( typeIt != sEventMap.end() )
   {
      EventIDNameMap::const_iterator eventIt = typeIt->second.find( eventID );
      if( eventIt != typeIt->second.end() )
      {
         eventName = eventIt->second;
         return true;
      }
   }
   
   return false;
}

//============================================================================
//============================================================================
bool BUIControl::isSoundEnabled( void )
{
   return mbSoundEnabled;
}

//============================================================================
//============================================================================
bool BUIControl::shouldPlaySound( void )
{
   return isSoundEnabled() && (!mpParent || mpParent->shouldPlaySound());
}

//============================================================================
//============================================================================
bool BUIControl::playEventSound( BUIControlEvent& event )
{
   if( shouldPlaySound() )
   {
      BString eventName;
      if( getEventName( event.getID(), eventName ) )
      {
         SoundMap::const_iterator it = mSoundMap.find( eventName );
         if( it != mSoundMap.end() )
         {
            gSoundManager.playCue( it->second );
            event.setSoundPlayed();
            return true;
         }
      }
   }
   return false;
}

//==============================================================================
// IInputControlEventHandler 
//==============================================================================
bool BUIControl::executeInputEvent(long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl)
{
   return false;
}

//==============================================================================
//==============================================================================
void BUIControl::enterContext(const char* contextName)
{
   if (mpInputHandler)
      mpInputHandler->enterContext(contextName);
}

//==============================================================================
// Common UI Control functions
//==============================================================================
void BUIControl::enable( void ) 
{ 
   if( !mbEnabled )
   {
      mbEnabled = true;
      invokeActionScript( "onEnable" );
      fireUIControlEvent( eEnable );
   }
}

//==============================================================================
//==============================================================================
void BUIControl::disable( void ) 
{ 
   if( mbEnabled )
   {
      mbEnabled = false;
      invokeActionScript( "onDisable" ); 
      fireUIControlEvent( eDisable );
   }
}

//==============================================================================
//==============================================================================
void BUIControl::focus(bool force) 
{ 
   if( !mbFocused || force)
   {
      mbFocused = true;
      invokeActionScript( "onFocus" ); 
      fireUIControlEvent( eFocus );
   }
}

//==============================================================================
//==============================================================================
void BUIControl::unfocus(bool force) 
{ 
   if( mbFocused || force)
   {
      mbFocused = false;
      invokeActionScript( "onUnfocus" ); 
      fireUIControlEvent( eUnfocus );
   }
}

//==============================================================================
//==============================================================================
void BUIControl::show( bool force )
{ 
   if( !mbShown || force)
   {
      mbShown = true;
      invokeActionScript( "onShow" );
      fireUIControlEvent( eShow );
   }
}

//==============================================================================
//==============================================================================
void BUIControl::hide( bool force )
{ 
   if( mbShown || force)
   {
      mbShown = false;
      invokeActionScript( "onHide" ); 
      fireUIControlEvent( eHide );
   }
}


//==============================================================================
// Position and Scale
//==============================================================================
void BUIControl::setPosition( int x, int y )
{
   GFxValue args[2];
   args[0].SetNumber( x );
   args[1].SetNumber( y );
   invokeActionScript( "setPosition", args, 2 );
}

//==============================================================================
//==============================================================================
void BUIControl::setSize( int x, int y )
{
   GFxValue args[2];
   args[0].SetNumber( x );
   args[1].SetNumber( y );
   invokeActionScript( "setSize", args, 2 );
}

// Accessors
//==============================================================================
//==============================================================================
int BUIControl::getControlID( void ) const
{
   return mControlID;
}

//==============================================================================
//==============================================================================
void BUIControl::setControlID( int controlID )
{
   mControlID=controlID;
}

//==============================================================================
//==============================================================================
const BString& BUIControl::getControlPath( void ) const
{ 
   return mControlPath; 
}

//==============================================================================
//==============================================================================
BFlashMovieInstance* BUIControl::getMovie( void )
{
   if( mpMovie == NULL )
   {
      mpMovie = mpParent->getMovie();
   }
   
   return mpMovie;
}

//============================================================================
//============================================================================
BUIScreen* BUIControl::getScreen( void )
{
   if( mpScreen == NULL && mpParent )
   {
      mpScreen = mpParent->getScreen();
   }

   return mpScreen;
}

//==============================================================================
//==============================================================================
bool BUIControl::isEnabled( void ) const
{
   return mbEnabled;
}

//==============================================================================
//==============================================================================
bool BUIControl::isFocused( void ) const
{
   return mbFocused;
}

//==============================================================================
//==============================================================================
bool BUIControl::isShown( void ) const
{
   return mbShown;
}

//==============================================================================
//==============================================================================
void BUIControl::fireUIControlEvent( int eventID, BUIControlEvent* childEvent /* = NULL */ )
{
   BUIControlEvent event( this, eventID, childEvent );
   if( mpParent )
      mpParent->handleUIControlEvent( event );
   if( !event.hasSoundPlayed() )
   {
      playEventSound( event );
   }
}

//==============================================================================
//==============================================================================
void BUIControl::fireUIControlEvent( const BCHAR_T* str, BUIControlEvent* childEvent /* = NULL */ )
{
   BUIControlEvent event( this, eStringEvent, childEvent, str );
   if( mpParent )
      mpParent->handleUIControlEvent( event );
   if( !event.hasSoundPlayed() )
   {
      playEventSound( event );
   }
}

//==============================================================================
//==============================================================================
void BUIControl::invokeActionScript( const char* method, GFxValue* args, int numArgs )
{
   if (mControlPath.length() != 0)
      getMovie()->invokeActionScript( mScriptPrefix + method, args, numArgs );
}

//==============================================================================
//==============================================================================
bool BUIControl::loadDefaultControls( void )
{
   if (!spControlDefaults.getValid())
   {
      if(!spControlDefaults.load(cDirData, "uiControlDefaults.xml"))
         return false;
   }

   // Look for our control
   BXMLNode controlNode;
   const char* pControlType = getControlTypeName();
   if( spControlDefaults.getRootNode().getChild( pControlType, &controlNode ) )
   {
      // Look for default controls
      BXMLNode controlsNode;
      if( controlNode.getChild( "controls", &controlsNode ) )
      {
         return mpInputHandler->loadControls( controlsNode, this );
      }
   }
   return false;
}


//============================================================================
//============================================================================
void BUIControl::attachMovie( const char* id, const char* name )
{  
   GFxValue args[2];
   args[0].SetString( id );
   args[1].SetString( name );

   invokeActionScript( "attachMovieClip", args, 2 );
}

//============================================================================
//============================================================================
void BUIControl::enableSound( bool enabled )
{
   mbSoundEnabled = enabled;
}

//============================================================================
//============================================================================
void BUIControl::disableSound( void )
{
   enableSound( false );
}

//============================================================================
//============================================================================
void BUIControl::initEventMap( void )
{
   int controlID = UIControlID;
   if( sEventMap[controlID].size() == 0 )
   {
      MAP_CONTROL_EVENT( Enable )
      MAP_CONTROL_EVENT( Disable )
      MAP_CONTROL_EVENT( Focus )
      MAP_CONTROL_EVENT( Unfocus )
      MAP_CONTROL_EVENT( Show )
      MAP_CONTROL_EVENT( Hide )
   }
}

//////////////////////////////////////////////////////////////////////////
//==============================================================================
//==============================================================================
BUIControlEvent::BUIControlEvent( BUIControl* control, int id, BUIControlEvent* childEvent /*= NULL*/ ) :
   mControl( control ),
   mID( id ),
   mChildEvent( childEvent ),
   mSoundPlayed( false )
{

   BASSERT( control );
}

//==============================================================================
//==============================================================================
BUIControlEvent::BUIControlEvent( BUIControl* control, int id, const BCHAR_T* str ) :
   mControl( control ),
   mID( id ),
   mChildEvent( NULL ),
   mString( str ),
   mSoundPlayed( false )
{
   BASSERT( control );
   BASSERT( str );
}

//==============================================================================
//==============================================================================
BUIControlEvent::BUIControlEvent( BUIControl* control, int id, BUIControlEvent* childEvent, const BCHAR_T* str ) :
   mControl( control ),
   mID( id ),
   mChildEvent( childEvent ),
   mString( str ),
   mSoundPlayed( false )
{
   BASSERT( control );
   BASSERT( str );
}

//==============================================================================
//==============================================================================
BUIControlEvent::~BUIControlEvent( void )
{
}


//==============================================================================
//==============================================================================
BUIControl* BUIControlEvent::getControl( void ) const
{
   return mControl;
}

//==============================================================================
//==============================================================================
int BUIControlEvent::getID( void ) const
{
   return mID;
}

//==============================================================================
//==============================================================================
const BUIControlEvent* BUIControlEvent::getChildEvent( void ) const
{
   return mChildEvent;
}