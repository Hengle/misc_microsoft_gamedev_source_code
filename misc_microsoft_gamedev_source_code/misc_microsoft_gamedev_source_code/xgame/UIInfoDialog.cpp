//============================================================================
// UIInfoDialog.cpp
// Ensemble Studios (c) 2008
//============================================================================
#include "common.h"
#include "UIInfoDialog.h"
#include "soundmanager.h"
#include "database.h"
#include "modemanager.h"
#include "modegame.h"
#include "usermanager.h"
#include "user.h"

//============================================================================
//============================================================================
BUIInfoDialog::BUIInfoDialog( void ) :
   pBodyText( NULL ),
   mButtonDelay( 3.0f ),
   mButtonDelayCounter( 0.0f )
{
}

//============================================================================
//============================================================================
BUIInfoDialog::~BUIInfoDialog( void )
{
}

//============================================================================
//============================================================================
bool BUIInfoDialog::init( BXMLNode dataNode )
{
   bool result = __super::init( dataNode );

   if( result )
      result = mTitleText.init( this, "mDialog.mTitleText" );

   if( result )
      result = mBodyTextNoImage.init( this, "mDialog.mBodyTextNoImage" );

   if( result )
      result = mBodyTextWithImage.init( this, "mDialog.mBodyTextWithImage" );

   if( result )
      result = mImage.init( this, "mDialog.mImage" );

   if( result )
      result = mButtonText.init( this, "mDialog.mButton.btntxt" );

   if( result )
      mButtonText.setText( gDatabase.getLocStringFromID( 104 ) );

   BXMLNode buttonDelayNode;
   if( dataNode.getChild( "ButtonDelay", &buttonDelayNode ) )
      buttonDelayNode.getTextAsFloat(mButtonDelay);

   return result;
}

//============================================================================
//============================================================================
bool BUIInfoDialog::executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl )
{
   if( command == "close" )
   {
      gModeManager.getModeGame()->setPaused(false);
      return true;
   }
   return false;
}

//============================================================================
//============================================================================
bool BUIInfoDialog::handleInput( int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail )
{
   bool result = pBodyText && pBodyText->handleInput( port, event, controlType, detail );

   if( !result && mButtonDelayCounter <= 0.0f )
      result = __super::handleInput( port, event, controlType, detail );

   return true;
}

//============================================================================
//============================================================================
void BUIInfoDialog::setData( const BUString& titleText, const BUString& bodyText, const BString& voCue, const BString& imagePath )
{
   mTitleText.setText( titleText );
   
   if( !voCue.isEmpty() )
   {
      gSoundManager.playCue( voCue.getPtr() );
   }

   mImage.clearImages();

   if( !imagePath.isEmpty() )
   {
      mImage.addImage( imagePath.getPtr() );
      mImage.showNextPicture();
      
      mBodyTextWithImage.setText( bodyText.getPtr() );
      mBodyTextWithImage.show();
      mBodyTextNoImage.setText( L"" );
      mBodyTextNoImage.hide();
      pBodyText = &mBodyTextWithImage;
   }
   else
   {
      mBodyTextNoImage.setText( bodyText.getPtr() );
      mBodyTextNoImage.show();
      mBodyTextWithImage.setText( L"" );
      mBodyTextWithImage.hide();
      pBodyText = &mBodyTextNoImage;
   }
}

//============================================================================
//============================================================================
void BUIInfoDialog::enter( void )
{
   gModeManager.getModeGame()->setPaused(true);
   invokeActionScript( "mDialog.onEnter" );
   mButtonDelayCounter = mButtonDelay;
}

//============================================================================
//============================================================================
void BUIInfoDialog::update( float dt )
{
   if( mButtonDelayCounter > 0 )
   {
      mButtonDelayCounter -= dt;

      if( mButtonDelayCounter <= 0.0f )
      {
         invokeActionScript( "mDialog.showButton" );
      }
   }
   else if( !gModeManager.getModeGame()->getPaused() )
   {
      mpHandler->handleUIScreenResult( this, eResult_Close );
   }
}

//============================================================================
//============================================================================
void BUIInfoDialog::leave( void )
{
   invokeActionScript( "mDialog.onLeave" );
}