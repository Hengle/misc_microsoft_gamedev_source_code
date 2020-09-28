//============================================================================
// UIScreen.h
// Ensemble Studios (c) 2008
//============================================================================
#pragma once

// INCLUDES
#include "flashscene.h"
#include "flashgateway.h"
#include "d3dtexturemanager.h"
#include "UIControl.h"

class BUIScreen;

// IUIScreenHandler
class IUIScreenHandler
{
   public:
      virtual void handleUIScreenResult( BUIScreen* pScreen, long result ) = 0;
};

// BUIScreen
class BUIScreen : public BFlashScene, public IUIScreenHandler, public IUIControlEventHandler, public IInputControlEventHandler
{
public:
   BUIScreen( void );
   virtual ~BUIScreen( void );

   static void install( void );
   static void remove( void );

   // BFlashScene
   virtual bool initScreen( const char* filename, BFlashAssetCategory category, const char* datafile );
   virtual bool init( const char* filename, const char* datafile );

   // IUIScreenHandler
   virtual void handleUIScreenResult( BUIScreen* pScreen, long result ) {}
   virtual void setHandler( IUIScreenHandler* pHandler ) { mpHandler = pHandler; }

   // subclasses override this.
   virtual bool init(BXMLNode dataNode);

   virtual void enter( void );
   virtual void update( float dt );
   virtual void refreshScreen(); 
   virtual void renderBegin( void );
   virtual void render( void );
   virtual void renderEnd( void );
   virtual void leave( void );
   virtual void deinit( void );
   virtual bool isInitialized( void );

   virtual bool isSoundEnabled( void );
   virtual bool shouldPlaySound( void );

   virtual BFlashMovieInstance*  getMovie( void );
   virtual BUIScreen* getScreen( void );
   
   virtual bool handleUIControlEvent( BUIControlEvent& event );
   
   virtual bool executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl );
   virtual bool handleInput( int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail );
   
   virtual void setDimension( int x, int y, int width, int height );
   virtual BManagedTextureHandle getRenderTargetTexture( void );

   virtual void setVisible( bool visible );
   virtual bool getVisible( void ) const;

   virtual BUIScreen* getParent() { return mpParent; }
   virtual void setParent(BUIScreen* parent) { mpParent = parent; }

   void showSafeAreaGuide(bool bVisible);

   void attachMovie( const char* id, const char* name );

   void showTicker( bool show = true );

   static void formatTime(DWORD time, BUString& timeString);

   const BString& getScreenName() const { return mName; }

protected:
   void invokeActionScript( const char* method, GFxValue* args = NULL, int numArgs = 0 );
   void enableSound( bool enable = true );
   void disableSound( void );

   enum 
   {
      cFlagInitialized = 0,
      cFlagRenderTargetReady,
      cFlagTotal
   };

   enum EUsage{ ePreGame, eInGame, eCommon };
   EUsage mUsage;

   BString              mName;
   IUIScreenHandler* mpHandler;
   BFlashMovieInstance* mpMovie;
   BFlashGateway::BDataHandle mDataHandle;
   BUIScreen*           mpParent;
   bool                 mbVisible;
   BUIInputHandler*     mpInputHandler;
   bool mSoundEnabled;
   bool mShowTicker;
};