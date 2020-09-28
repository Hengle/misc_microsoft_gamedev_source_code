//============================================================================
// UIOptionsMenu.cpp
// Ensemble Studios (c) 2008
//============================================================================

#include "common.h"

#include "UIOptionsMenu.h"
#include "UIListControl.h"
#include "UILabelControl.h"

#include "usermanager.h"
#include "user.h"
#include "userprofilemanager.h"

#include "modemanager.h"
#include "modemenu.h"

#include "database.h"

#include "game.h"

#include "hintengine.h"
#include "configsgame.h"

#include "gamesettings.h"

#include "humanPlayerAITrackingData.h"

//============================================================================
//============================================================================
BUIOptionsMenu::BUIOptionsMenu( void ) :
   mpUser( NULL ),
   mSeconds( -1 )
{
}

//============================================================================
//============================================================================
BUIOptionsMenu::~BUIOptionsMenu( void )
{
   for( int i = 0; i < (int)mControls.size(); ++i )
   {
      delete mControls[i];
   }

   if (gGame.getUIGlobals() && gGame.getUIGlobals()->isYorNBoxVisible() && gGame.getUIGlobals()->IsYornHandler(this))
      gGame.getUIGlobals()->cancel();
}

//============================================================================
//============================================================================
bool BUIOptionsMenu::init( BXMLNode root )
{
   __super::init( root );

   // Initialize the DESCRIPTION label
   if( mDescriptionLabel.init( this, "mDescriptionLabel" ) )
   {
      mDescriptionLabel.setText( gDatabase.getLocStringFromID( 25677 ) );
   }

   // Find the detail text node
   BXMLNode detailTextNode;
   if( root.getChild( "detailtext", &detailTextNode ) )
   {
      BString detailTextPath;
      detailTextNode.getAttribValueAsString( "path", detailTextPath );
      mDetailText.init( this, detailTextPath );
   }

   // Find the detail image node
   BXMLNode detailImageNode;
   if( root.getChild( "detailimage", &detailImageNode ) )
   {
      BString detailImagePath;
      detailImageNode.getAttribValueAsString( "path", detailImagePath );
      mDetailImage.init( this, detailImagePath );
      mDetailImage.setAutoTransition( false );
   }

   // Find the tabs in the init data
   BXMLNode tabsNode;
   if( root.getChild( "tabs", &tabsNode ) )
   {
      // Init the tab control
      BString tabPath;
      tabsNode.getAttribValueAsString( "path", tabPath );
      mTabs.init( this, tabPath, eTabControlID );

      // For each tab...
      for( int tabIdx = 0; tabIdx < tabsNode.getNumberChildren(); ++tabIdx )
      {
         BXMLNode tabNode = tabsNode.getChild( tabIdx );
         BASSERT( tabNode.getValid() );

         BUString tabLabel;
         tabNode.getAttribValueAsString( "label", tabLabel );

         BString tabPath;
         tabPath.format( "Tab%d", tabIdx + 1 );

         // Get the tab image, if specified
         BString tabImage;
         tabNode.getAttribValueAsString( "image", tabImage );
         mTabImages.add( tabImage );

         // Create and init an actual "tab" control
         BUILabelControl* tabControl = new BUILabelControl();
         mControls.add( tabControl );

         tabControl->init( &mTabs, tabPath );
         tabControl->setText( gDatabase.decodeLocString( tabLabel ) );
         tabControl->unfocus(true);
         
         // Create and init an optionList
         BUIListControl* optionList = new BUIListControl();
         mControls.add( optionList );

         BString listPath;
         listPath.format( "List%d", tabIdx + 1 );

         addOptionPane( listPath );
         optionList->init( this, listPath, eOptionListID );

         // For each option...
         int optionIdx = 1;
         int numOptions = tabNode.getNumberChildren();
         for( int nodeIdx = 0; nodeIdx < numOptions; ++nodeIdx )
         {
            BXMLNode optionNode = tabNode.getChild( nodeIdx );   
            
            // Early-out for config-specific options
            BString configAttrib;
            if( optionNode.getAttribValueAsString( "config", configAttrib ) && !gConfig.isDefined( configAttrib ) )
            {
               continue;
            }

            // Early-out for usage-specific options
            BXMLAttribute usageAttrib;
            if( optionNode.getAttribute( "pregame", &usageAttrib ) && mUsage != ePreGame )
            {
               continue;
            }
            if( optionNode.getAttribute( "ingame", &usageAttrib ) && mUsage != eInGame)
            {
               continue;
            }

            BString optionPath;
            optionPath.format( "Option%d", optionIdx++ );

            optionList->attachMovie( "UIOptionRowMC", optionPath );
            
            // Create and init an optionControl and add it to the optionList
            BUIOptionsMenuItemControl* optionControl = new BUIOptionsMenuItemControl();
            mControls.add( optionControl );
         
            optionControl->init( optionList, (listPath + ".") + optionPath, -1, &optionNode );
            optionList->addControl( optionControl );
         }
         optionList->setIndex( 0 );
         mTabs.addTab( tabControl, optionList );
         if( tabIdx != 0 )
            optionList->hide();
      }
      mTabs.setActiveTab( 0 );
   }

   if( mpUser )
      setUser( mpUser );

   mTitle.init(this, "mTitle");
   mTitle.setText(gDatabase.getLocStringFromID(25291));

   mTimeLabel.init( this, "mGameTime" );
   mTimeLabel.setText( L"" );

   mDifficulty.init( this, "mDifficulty" );

   mButtonBar.init( this, "mButtonBar" );

#if !defined( BUILD_PLAYTEST ) && !defined( BUILD_FINAL )
      mButtonBar.setButtonStates( BUIButtonBarControl::cFlashButtonA, BUIButtonBarControl::cFlashButtonB, BUIButtonBarControl::cFlashButtonX, BUIButtonBarControl::cFlashButtonOff, BUIButtonBarControl::cFlashButtonY );
      mButtonBar.setButtonTexts( gDatabase.getLocStringFromID(25292), gDatabase.getLocStringFromID(25293), gDatabase.getLocStringFromID(25294), L"", gDatabase.getLocStringFromID(25295) );
#else
      mButtonBar.setButtonStates( BUIButtonBarControl::cFlashButtonA, BUIButtonBarControl::cFlashButtonB, BUIButtonBarControl::cFlashButtonX );
      mButtonBar.setButtonTexts( gDatabase.getLocStringFromID(25292), gDatabase.getLocStringFromID(25293), gDatabase.getLocStringFromID(25294) );
#endif

   return true;
}

//============================================================================
//============================================================================
bool BUIOptionsMenu::handleUIControlEvent( BUIControlEvent& event )
{
   int controlID = event.getControl()->getControlID();
   int eventID = event.getID();
   bool tabChanged = controlID == eTabControlID && eventID == BUITabControl::eTabChanged;
   bool optionChanged = controlID == eOptionListID && eventID == BUIListControl::eSelectionChanged;

   if( tabChanged )
   {
      BUIListControl* optionList = (BUIListControl*)mTabs.getActivePane();
      optionList->setIndex( 0 );
   }

   if( tabChanged || optionChanged )
   {
      updateDescription();
      return true;
   }

   return false;
}

//============================================================================
//============================================================================
void BUIOptionsMenu::enter( void )
{
   __super::enter();
   mTabs.setActiveTab( 0 );
}

//============================================================================
//============================================================================
void BUIOptionsMenu::update( float dt )
{
   __super::update( dt );

   if( mpUser->getPlayer() != NULL )
   {
      long gameTime = gGame.getGametime();
      long mins = gameTime / 60000;
      long secs = (gameTime - (mins*60000)) / 1000;

      if( secs != mSeconds )
      {
         mSeconds = secs;
         BUString timeStr;
         timeStr.locFormat( L"%02d:%02d", mins, secs );
         mTimeLabel.setText( timeStr );
      }
   }
}

//============================================================================
//============================================================================
bool BUIOptionsMenu::executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl )
{
   if (command=="cancel")
   {
      gUI.playCancelSound();
      gUserProfileManager.writeProfile( mpUser );
      if( mpHandler )
      {
         mpHandler->handleUIScreenResult( this, 0 );
      }
      return true;
   }
   else if( command == "defaults" )
   {
      // reset all options to their default values?
      gGame.getUIGlobals()->showYornBox( this, gDatabase.getLocStringFromID(25348), BUIGlobals::cDialogButtonsOKCancel, eResetAllOptions );
      return true;
   }
   else if( command == "resetHints" )
   {
#if !defined( BUILD_PLAYTEST ) && !defined( BUILD_FINAL )
      // Reset Hints?
      gGame.getUIGlobals()->showYornBox( this, gDatabase.getLocStringFromID(25349), BUIGlobals::cDialogButtonsOKCancel, eResetHints );
      return true;
#endif
   }
   return false;
}

//============================================================================
//============================================================================
bool BUIOptionsMenu::handleInput( int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail )
{
   bool handled = false;

   handled = mTabs.handleInput( port, event, controlType, detail );

   if( !handled )
   {
      BUIListControl* activeList = (BUIListControl*)mTabs.getPane( mTabs.getActiveTab() );
      if( activeList )
      {
         BUIOptionsMenuItemControl* activeOptionControl = (BUIOptionsMenuItemControl*)activeList->getSelectedControl();
         if( activeOptionControl )
            handled = activeOptionControl->handleInput( port, event, controlType, detail );

         if( !handled )
         {
            handled = activeList->handleInput( port, event, controlType, detail );
         }
      }   
   }

   if( !handled )
   {
      handled = __super::handleInput( port, event, controlType, detail );
   }

   // Return true here because this screen can come up in-game and we don't want
   // the game to handle any input.
   return true;
}

//============================================================================
//============================================================================
void BUIOptionsMenu::setUser( BUser* pUser )
{
   mpUser = pUser;

   uint numControls = mControls.getSize();
   for( uint i = 0; i < numControls ; ++i )
   {
      BUIControl* control = mControls[i];
      if( control->getControlTypeID() == UIOptionsMenuItemControlID )
      {
         BUIOptionsMenuItemControl* optionControl = (BUIOptionsMenuItemControl*)control;
         optionControl->setUser( mpUser );
      }
   }

   if( mpUser && mpUser->getPlayer() != NULL )
   {
      BDifficultyType playerDifficulty = mpUser->getPlayer()->getDifficultyType();
      mDifficulty.update( playerDifficulty );

      BGameSettings* pSettings = gDatabase.getGameSettings();
      if( pSettings )
      {
         long diffType = -1;
         pSettings->getLong( BGameSettings::cPlayer1DifficultyType, diffType );

         if( diffType == DifficultyType::cAutomatic )
         {
            BHumanPlayerAITrackingData trackingData;

            int autoDiffLevel = 0;
            BUString autoText;
            autoText.empty();

            if (trackingData.loadValuesFromMemoryBlock(gUserManager.getPrimaryUser()->getProfile()->getAITrackingDataMemoryPointer()))
            {
               autoDiffLevel = trackingData.getAutoDifficultyLevel();
               autoText.locFormat(gDatabase.getLocStringFromID(25976).getPtr(), gDatabase.getDifficultyStringByType(DifficultyType::cAutomatic).getPtr(), autoDiffLevel);
            }
            else
            {
               autoText.set(gDatabase.getDifficultyStringByType(DifficultyType::cAutomatic));
            }
            mDifficulty.setText( autoText );
         }
      }
   }
}

//============================================================================
//============================================================================
void BUIOptionsMenu::yornResult(uint result, DWORD userContext, int port)
{
   if( result == BUIGlobals::cDialogResultCancel )
      return;

   switch( userContext )
   {
      case eResetOneOption:
         break;

      case eResetOnePane:
         break;

      case eResetAllOptions:
         if (mpUser) mpUser->resetAllOptions();
         break;

      case eResetHints:
         if( mpUser && mpUser->getPlayer() && mpUser->getPlayer()->getHintEngine() )
            mpUser->getPlayer()->getHintEngine()->reset( false );
         else
            gConfig.define( "ForceHintReset" );
         break;
   }

   setUser(mpUser);
}

//============================================================================
//============================================================================
void BUIOptionsMenu::setTab( int index )
{
   if( index >= 0 && index < mTabs.getNumberTabs() )
   {
      mTabs.setActiveTab( index );
   }
}

//============================================================================
//============================================================================
void BUIOptionsMenu::addOptionPane( const char* name )
{
   GFxValue args[1];
   args[0].SetString( name );

   invokeActionScript( "addOptionPane", args, 1 );
}

//============================================================================
//============================================================================
void BUIOptionsMenu::updateDescription( void )
{
   BUIListControl* optionList = (BUIListControl*)mTabs.getActivePane();
   if( optionList != NULL )
   {
//-- FIXING PREFIX BUG ID 1367
      const BUIOptionsMenuItemControl* option = (BUIOptionsMenuItemControl*)optionList->getSelectedControl();
//--
      mDetailText.setText( option->getDescription() );

      BString detailImage = option->getImageURL();
      if( detailImage.isEmpty() )
         detailImage = mTabImages[mTabs.getActiveTab()];
      mDetailImage.clearImages();
      mDetailImage.addImage( detailImage );
      mDetailImage.showNextPicture();

      switch( option->getOptionType() )
      {
         case BUIOptionsMenuItemControl::eCheckbox:
            mButtonBar.setButtonState( 0, BUIButtonBarControl::cFlashButtonA );
            mButtonBar.setButtonText( 0, gDatabase.getLocStringFromID(25345) );    // toggle
            break;

         case BUIOptionsMenuItemControl::eSlider:
            mButtonBar.setButtonState( 0, BUIButtonBarControl::cFlashButtonOff );
            mButtonBar.setButtonText( 0, L"" );
            break;

         case BUIOptionsMenuItemControl::eFlyout:
            mButtonBar.setButtonState( 0, BUIButtonBarControl::cFlashButtonA );
            mButtonBar.setButtonText( 0, gDatabase.getLocStringFromID(25346) );    // Select
            break;

         case BUIOptionsMenuItemControl::eButton:
            mButtonBar.setButtonState( 0, BUIButtonBarControl::cFlashButtonA );
            mButtonBar.setButtonText( 0, gDatabase.getLocStringFromID(25347) );      // execute
            break;
      }
   }
}