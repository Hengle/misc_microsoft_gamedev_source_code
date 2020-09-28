//============================================================================
// UIScreen.cpp
// Ensemble Studios (c) 2008
//============================================================================
#include "common.h"
#include "UIScreen.h"
#include "UIInputHandler.h"
#include "UITicker.h"
#include "lspManager.h"

//==============================================================================
//==============================================================================
BUIScreen::BUIScreen( void ) :
   mpHandler( NULL ),
   mUsage( ePreGame ),
   mpMovie( NULL ),
   mpParent(NULL),
   mpInputHandler(NULL),
   mbVisible( false ),
   mSoundEnabled( false ),
   mShowTicker( false )
{
   mName.set("");
}

//==============================================================================
//==============================================================================
BUIScreen::~BUIScreen( void )
{
   deinit();
   if (mpInputHandler != NULL)
      delete mpInputHandler;
}

//============================================================================
//============================================================================
void BUIScreen::install( void )
{
   BUITicker::install();
}

void BUIScreen::remove( void )
{
   BUITicker::remove();
}

// BFlashScene
//==============================================================================
//==============================================================================
bool BUIScreen::initScreen( const char* filename, BFlashAssetCategory category, const char* datafile )
{
   switch( category )
   {
   case cFlashAssetCategoryCommon:
         mUsage = eCommon;
      break;
   case cFlashAssetCategoryPreGame:
         mUsage = ePreGame;
      break;
   case cFlashAssetCategoryInGame:
         mUsage = eInGame;
      break;
   }
   gFlashGateway.getOrCreateData( filename, category, mDataHandle );
   mpMovie = gFlashGateway.createInstance( mDataHandle, false );
   

   if (datafile)
   {
      BXMLReader reader;

      if(!reader.load(cDirProduction, datafile))
      {
         BString temp;
         temp.format("Failed to load: %s", datafile);
         BASSERTM(0, temp.getPtr());
         return false;
      }

      BXMLNode root(reader.getRootNode());

      if (!loadData(root, &reader))
         return false;

      init(root);
   }

   initResolution();

   setFlag( cFlagInitialized, true );

   enableSound();

   return true;
}

//==============================================================================
//==============================================================================
bool BUIScreen::init( BXMLNode root )
{
   bool result = true;

   BXMLNode controlListNode = root.getChildNode( "ControlList" );
   if( controlListNode.getValid() )
   {
      BXMLNode controlsNode = controlListNode.getChildNode( "Controls" );
      if( controlsNode.getValid() )
      {
         if( mpInputHandler )
            delete mpInputHandler;

         mpInputHandler = new BUIInputHandler();

         result = mpInputHandler->loadControls( controlsNode, this );
         mpInputHandler->enterContext( "Main" );
      }
   }

   BXMLNode tickerNode = root.getChildNode( "Ticker" );
   mShowTicker = tickerNode.getValid();
   
   if( mShowTicker )
   {
      bool bShowTickerBackground = false;
      tickerNode.getAttribValueAsBool( "background", bShowTickerBackground );
      BUITicker::showBackground( bShowTickerBackground );
   }

   return result;
}

//==============================================================================
//==============================================================================
bool BUIScreen::init( const char* filename, const char* datafile )
{
   return initScreen( filename, cFlashAssetCategoryPreGame, datafile );
}

//==============================================================================
//==============================================================================
void BUIScreen::enter( void )
{
   
}

//==============================================================================
//==============================================================================
void BUIScreen::refreshScreen( void )
{
   // do nothing, in-game UI screens may want to react to this.
}

//==============================================================================
//==============================================================================
void BUIScreen::update( float dt )
{
   //Update Ticker info if visible
   if( (mShowTicker || gConfig.isDefined( "ForceTicker" )))  
   {
      //Maybe a timer on here will be necessary?
      BUITicker::updateTickerInfo();
   }
}

//==============================================================================
//==============================================================================
void BUIScreen::renderBegin( void )
{

}

//==============================================================================
//==============================================================================
void BUIScreen::render()
{
   SCOPEDSAMPLEID( FlashUIGame, 0XFFFF0000 );
   if( mbVisible )
   {
      mpMovie->render();
      if( (mShowTicker || gConfig.isDefined( "ForceTicker" )))
      {
         BUITicker::renderTicker();
      }
   }
}

//==============================================================================
//==============================================================================
void BUIScreen::renderEnd( void )
{

}

//==============================================================================
//==============================================================================
void BUIScreen::leave( void )
{

}

//==============================================================================
//==============================================================================
void BUIScreen::deinit( void )
{
   if( mpMovie )
   {
      gFlashGateway.unregisterEventHandler( mpMovie, mSimEventHandle );
      gFlashGateway.releaseInstance( mpMovie );
      mpMovie = NULL;
   }
}

//============================================================================
//============================================================================
bool BUIScreen::isInitialized( void )
{
   return getFlag( cFlagInitialized );
}

//============================================================================
//============================================================================
bool BUIScreen::isSoundEnabled( void )
{
   return mSoundEnabled;
}

//============================================================================
//============================================================================
bool BUIScreen::shouldPlaySound( void )
{
   return isSoundEnabled();
}

//==============================================================================
//==============================================================================
BFlashMovieInstance*  BUIScreen::getMovie( void )
{
   return mpMovie;
}

//============================================================================
//============================================================================
BUIScreen* BUIScreen::getScreen( void )
{
   return this;
}

// IUIControlEventHandler
//==============================================================================
//==============================================================================
bool BUIScreen::handleUIControlEvent( BUIControlEvent& event )
{
   return false;
}

//==============================================================================
//==============================================================================
bool BUIScreen::handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail)
{
   if (!mpInputHandler)
      return false;

   return mpInputHandler->handleInput(port, event, controlType, detail);
}


//==============================================================================
//==============================================================================
void BUIScreen::setDimension(int x, int y, int width, int height)
{
   if( mpMovie )
   {
      mpMovie->setDimension( x, y, width, height );
   }
}

//==============================================================================
//==============================================================================
BManagedTextureHandle BUIScreen::getRenderTargetTexture()
{
   BManagedTextureHandle handle = cInvalidManagedTextureHandle;

   if( mpMovie )
   {
      handle = mpMovie->mRenderTargetHandle;
   }

   return handle;
}

//==============================================================================
//==============================================================================
void BUIScreen::setVisible( bool visible )
{
   mbVisible = visible;

   if( mbVisible )
      gFlashGateway.registerEventHandler( mpMovie, mSimEventHandle );
   else
      gFlashGateway.unregisterEventHandler( mpMovie, mSimEventHandle );
}

//==============================================================================
//==============================================================================
bool BUIScreen::getVisible( void ) const
{
   return mbVisible;
}

//==============================================================================
//==============================================================================
void BUIScreen::showSafeAreaGuide(bool bVisible)
{
   GFxValue value;
   value.SetBoolean(bVisible);

   mpMovie->invokeActionScript("showSafeAreaGuide", &value, 1);
}

//============================================================================
//============================================================================
void BUIScreen::attachMovie( const char* id, const char* name )
{                             
   GFxValue args[2];
   args[0].SetString( id );
   args[1].SetString( name );

   mpMovie->invokeActionScript( "attachMovie", args, 2 );
}

//==============================================================================
//==============================================================================
bool BUIScreen::executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl )
{
   return false;
}

//============================================================================
//============================================================================
void BUIScreen::invokeActionScript( const char* method, GFxValue* args, int numArgs )
{
   getMovie()->invokeActionScript( method, args, numArgs );
}

//============================================================================
//============================================================================
void BUIScreen::enableSound( bool enable )
{
   mSoundEnabled = enable;
}

//============================================================================
//============================================================================
void BUIScreen::disableSound( void )
{
   enableSound( false );
}

//============================================================================
//============================================================================
void BUIScreen::showTicker( bool show /* = true */ )
{
   mShowTicker = show;
}

//============================================================================
//============================================================================
void BUIScreen::formatTime(DWORD time, BUString& timeString)
{
   DWORD s=time/1000;
   DWORD m=s/60;
   s-=m*60;
   DWORD h=m/60;
   m-=h*60;

   if(h>0)
      timeString.locFormat(L"%02d:%02d:%02d", h, m, s);
   else
      timeString.locFormat(L"%02d:%02d", m, s);
}
