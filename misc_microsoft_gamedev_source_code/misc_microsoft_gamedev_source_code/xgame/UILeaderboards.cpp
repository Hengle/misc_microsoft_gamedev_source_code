//============================================================================
// UILeaderboards.cpp
// Ensemble Studios (c) 2008
//============================================================================
#include "common.h"
#include "UILeaderboards.h"
#include "usermanager.h"
#include "user.h"
#include "database.h"
#include "leaders.h"
#include "campaignmanager.h"
#include "game.h"

#define LOC( id ) gDatabase.getLocStringFromID( id )

//============================================================================
//============================================================================
BUILeaderboards::BUILeaderboards( void ) :
   mLeaderboardView( NUM_ROWS ),
   mCancelling(FALSE)
{
}

//============================================================================
//============================================================================
BUILeaderboards::~BUILeaderboards( void )
{
}

//============================================================================
//============================================================================
bool BUILeaderboards::init( BXMLNode root )
{
   __super::init( root );

   bool result = true;

   mDebugText.init( this, "mDebugText" );

   // Title
   mTitle.init( this, "mTitle" );
   mTitle.setText( LOC(25096) );

   // Leaderboard Type Option
   mLBTypeOption.name.init( this, "mLBTypeOption.mLabel" );
   mLBTypeOption.values.init( this, "mLBTypeOption.mList" );
   populateOption( mLBTypeOption, eLeaderboardType );
   
   // Filter Option
   mFilterOption.name.init( this, "mFilterOption.mLabel" );
   mFilterOption.values.init( this, "mFilterOption.mList" );
   populateOption( mFilterOption, eFilter );

   // Other Options per LeaderboardType
   mOptions[0].name.init( this, "mOption1.mLabel" );
   mOptions[0].values.init( this, "mOption1.mList" );
   
   mOptions[1].name.init( this, "mOption2.mLabel" );
   mOptions[1].values.init( this, "mOption2.mList" );

   mOptions[2].name.init( this, "mOption3.mLabel" );
   mOptions[2].values.init( this, "mOption3.mList" );

   for( int i = 0; i < 3; ++i )
   {
      mOptions[i].name.hide();
      mOptions[i].values.hide();
   }

   mOptionList.init( this, "" );

   // Column Headers
   if( result )
      result = mRankLabel.init( this, "mRankLabel" );

   if( result )
      result = mGamertagLabel.init( this, "mGamertagLabel" );
   
   if( result )
      result = mField1Label.init( this, "mField1Label" );
   
   if( result )
      result = mField2Label.init( this, "mField2Label" );

   if( result )
      result = mField3Label.init( this, "mField3Label" );
   
   if( result )
      result = mField2WLabel.init( this, "mField2WLabel" );

   if( result )
   {
      mRowList.setAlignment( BUIListControl::eVertical );
      result = mRowList.init( this, "mRows" );
   }

   // Rows
   for( int rowIdx = 0; result && rowIdx < NUM_ROWS; ++rowIdx )
   {
      BString rowPath;
      rowPath.format( "mRows.mRow%d", rowIdx + 1 );

      result = mRows[rowIdx].init( this, rowPath );
      
      if( result )
      {
         mRows[rowIdx].setData( 0, NULL );
         mRowList.addControl( &mRows[rowIdx] );
      }
   }

   if( result )
   {
      mRowList.setIndex( 0 );
      mRows[0].unfocus();
   }

   // Top-Level Focus
   mFocusList.init( this, "" );
   mFocusList.setAlignment( BUIListControl::eHorizontal );
   mFocusList.addControl( &mOptionList );
   mFocusList.addControl( &mRowList );
   mFocusList.setIndex( 0 );

   // Status Text
   mStatusText.init( this, "mStatus" );

   // Button Bar
   if( result )
      result = mButtonBar.init( this, "mButtonBar" );

   if( result )
   {
      mButtonBar.setButtonStates( BUIButtonBarControl::cFlashButtonA, BUIButtonBarControl::cFlashButtonB, BUIButtonBarControl::cFlashButtonY );
      mButtonBar.setButtonTexts( LOC(102), LOC(101), LOC(25586) );
   }


   // Initialize Options and run the default query
   populateOption( mLBTypeOption, eLeaderboardType );
   populateOptionList();
   validateOptions();
   
   mLeaderboardView.setHandler( this );
   executeQuery();

   return result;
}

//============================================================================
//============================================================================
bool BUILeaderboards::handleUIControlEvent( BUIControlEvent& event )
{
   if( event.getID() == BUIFlyoutListControl::eSelectionChanged )
   {
      // If a new LB Type was selected, update the list of options
      if( event.getControl() == &mLBTypeOption.values )
      {
         populateOptionList();
      }

      validateOptions();

      // Execute the new query
      populateResults();
      executeQuery();
      return true;
   }
   else if( event.getControl() == &mRowList )
   {
      if( (event.getID() == BUIListControl::eStopBegin && mLeaderboardView.prev()) || 
          (event.getID() == BUIListControl::eStopEnd && mLeaderboardView.next()) )
      {
         populateResults();
         event.setSoundPlayed();
         return true;
      }
      else if( event.getID() == BUIListControl::eFocus )
      {
         mButtonBar.setButtonStates( BUIButtonBarControl::cFlashButtonA, 
                                     BUIButtonBarControl::cFlashButtonB, 
                                     BUIButtonBarControl::cFlashButtonY, 
                                     BUIButtonBarControl::cFlashButtonLeftButton, 
                                     BUIButtonBarControl::cFlashButtonRightButton );
         mButtonBar.setButtonTexts( LOC(102), LOC(101), LOC(25586), LOC(25129), LOC(25130) );
         return true;
      }
      else if( event.getID() == BUIListControl::eUnfocus )
      {
         mButtonBar.setButtonStates( BUIButtonBarControl::cFlashButtonA, 
                                     BUIButtonBarControl::cFlashButtonB,
                                     BUIButtonBarControl::cFlashButtonY );
         mButtonBar.setButtonTexts( LOC(102), LOC(101), LOC(25586) );
         return true;
      }
      else
         return false;
   }
   else if( event.getID() == BUILeaderboardRecordControl::eSelected )
   {
      BUILeaderboardRecordControl* pRecordControl = reinterpret_cast<BUILeaderboardRecordControl*>( event.getControl() );
      XShowGamerCardUI( gUserManager.getPrimaryUser()->getPort(), pRecordControl->getXUID() );
      return true;
   }
   else
      return __super::handleUIControlEvent( event );
}

//============================================================================
//============================================================================
bool BUILeaderboards::executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl )
{
   if( command == "cancel" )
   {
      mLeaderboardView.cancelQuery();
      return true;
   }
   else if( command == "cycleFilter" )
   {
      mFilterOption.values.setIndex( ( mFilterOption.values.getSelectedIndex() + 1 ) % mFilterOption.values.getNumValues() );
      //populateResults();
      executeQuery();
      return true;
   }
   else if( command == "pageUp" && mRowList.isFocused() )
   {
      if (mLeaderboardView.prevPage())
         populateResults();
      return true;
   }
   else if( command == "pageDown" && mRowList.isFocused() )
   {
      if (mLeaderboardView.nextPage())
         populateResults();
      return true;
   }

   return false;
}

//============================================================================
//============================================================================
bool BUILeaderboards::handleInput( int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail )
{
   if( mOptionList.isFocused() && ((BUIFlyoutListControl*)mOptionList.getSelectedControl())->isOpen() )
      return mOptionList.handleInput( port, event, controlType, detail );

   bool result = false;

   if( !result )
      result = mFocusList.handleInput( port, event, controlType, detail );
   
   if( !result )
      result = __super::handleInput( port, event, controlType, detail );

   return result;
}

//============================================================================
//============================================================================
void BUILeaderboards::executeQuery( void )
{
   BLeaderBoardTypes eType = (BLeaderBoardTypes)mLBTypeOption.values.getValue();

   uint32 lbIndex = STATS_VIEW_MPWINSLIFETIME_1V1_GT1;
   
   switch( eType )
   {
      case cLeaderBoardTypeMPSkill:
      case cLeaderBoardTypeMPGametypeWinsLifetime:
      case cLeaderBoardTypeMPGametypeWinsMonthly:
         lbIndex = gLiveSystem->findLeaderBoardIndex( eType, getOptionValueByIndex( 0 ), !!getOptionValueByIndex( 1 ), 0, 0, getOptionValueByIndex( 2 ), 0 );
         break;

      case cLeaderBoardTypeCampaignLifetime:
         lbIndex = gLiveSystem->findLeaderBoardIndex( eType, 0, !!getOptionValueByIndex( 0 ), 0, 0, 0, 0 );
         break;

      case cLeaderBoardTypeCampaignBestScore:
         lbIndex = gLiveSystem->findLeaderBoardIndex( eType, 0, 0, getOptionValueByIndex( 0 ), getOptionValueByIndex( 1 ), 0, 0 );
         break;
         
      case cLeaderBoardTypeMPLeaderWinsMonthly:
         lbIndex = gLiveSystem->findLeaderBoardIndex( eType, 0, 0, 0, 0, getOptionValueByIndex( 1 ), getOptionValueByIndex( 0 ) );
         break;
   }

   mRowList.clearControls();
   for( int i = 0; i < NUM_ROWS; ++i )
   {
      mRows[i].setData( 0 , NULL );
      if( mRows[i].isFocused() )
         mRows[i].unfocus();
   }

   mLeaderboardView.executeQuery( lbIndex, (BLeaderboardView::EFilter)mFilterOption.values.getValue(), gUserManager.getPrimaryUser()->getXuid() );
   mStatusText.setText( LOC(25132) );
}

//============================================================================
//============================================================================
void BUILeaderboards::update( float dt )
{
   __super::update( dt );

   mLeaderboardView.update();

   // [9/30/2008 xemu] make sure we haven't been signed out of Live
   if (!mCancelling && !gUserManager.getPrimaryUser()->isSignedIntoLive())
   {
      mCancelling = TRUE;

      // [9/30/2008 xemu] cancel out, as if the user had done so 
      mLeaderboardView.cancelQuery();

      // [9/30/2008 xemu] and show a little dialog to explain what happened.
      BUIGlobals* pUIGlobals = gGame.getUIGlobals();
      BASSERT(pUIGlobals);
      if (pUIGlobals)
      {
         pUIGlobals->showYornBox(NULL, gDatabase.getLocStringFromID(25444), BUIGlobals::cDialogButtonsOK);
      }
   }

#if 0
   uint32 cacheSize = mLeaderboardView.getCacheSize();

   BUString continuousStr = L"continuous";
   if( mLeaderboardView.getCacheSize() > 0 )
   {
      uint32 firstRank = mLeaderboardView.getRecordByCacheIndex( 0 )->mRank;
      uint32 lastRank = mLeaderboardView.getRecordByCacheIndex( cacheSize - 1 )->mRank;
      if( lastRank - firstRank != cacheSize - 1 )
      { 
         continuousStr.prepend( L"not " );
      }
   }

   BUString debugText;
   debugText.format( L"Cache Size: %d (%s)", cacheSize, continuousStr.getPtr() );

   mDebugText.setText( debugText );
#endif
}

//============================================================================
//============================================================================
void BUILeaderboards::onLeaderboardViewReady( BLeaderboardView* pView )
{
   int eType = mLBTypeOption.values.getValue();
   setHeadersForLBType( eType );
  
   if( mLeaderboardView.getRecordCount() > 0 )
   {
      populateResults();

      mRowList.enable();
      if( !mRowList.isFocused() )
      {
         mRowList.setIndex( 0 );
         mRows[0].unfocus();
      }
   }
   else
      mRowList.disable();

   BUString statusText;
   statusText.locFormat( L"%d %s", mLeaderboardView.getRecordCount(), LOC(25131).getPtr() );
   mStatusText.setText( statusText );
}

//============================================================================
//============================================================================
void BUILeaderboards::onLeaderboardViewCancelled( BLeaderboardView* pView )
{
   if( mpHandler )
      mpHandler->handleUIScreenResult( this, 0 );
}

//============================================================================
//============================================================================
void BUILeaderboards::populateOption( SOption& sOption, EOption eOption )
{
   sOption.values.clear();
   sOption.values.setControlID( eOption );

   switch( eOption )
   {
      case eLeaderboardType:
         sOption.name.setText( LOC(25097) );
         sOption.values.addItem( LOC(25106), cLeaderBoardTypeMPSkill );
         sOption.values.addItem( LOC(25107), cLeaderBoardTypeCampaignLifetime );
         sOption.values.addItem( LOC(25108), cLeaderBoardTypeCampaignBestScore );
         sOption.values.addItem( LOC(25109), cLeaderBoardTypeMPGametypeWinsLifetime );
         sOption.values.addItem( LOC(25110), cLeaderBoardTypeMPGametypeWinsMonthly );
         sOption.values.addItem( LOC(25111), cLeaderBoardTypeMPLeaderWinsMonthly );
         sOption.values.setIndex( 0 );
      break;

      case eFilter:
         sOption.name.setText( LOC(25098) );
         sOption.values.addItem( LOC(25112), BLeaderboardView::eAll );
         sOption.values.addItem( LOC(25113), BLeaderboardView::eFriends );
         sOption.values.addItem( LOC(25114), BLeaderboardView::ePivot );
         sOption.values.setIndex( 0 );
      break;

      case eGameSize:
         sOption.name.setText( LOC(25099) );
         sOption.values.addItem( LOC(25115), 2 );
         sOption.values.addItem( LOC(25116), 4 );
         sOption.values.addItem( LOC(25117), 6 );
         sOption.values.setIndex( 0 );
      break;

      case eParty:
         if( mLBTypeOption.values.getValue() == cLeaderBoardTypeCampaignLifetime )
            sOption.name.setText( LOC(25102) );
         else
            sOption.name.setText( LOC(25100) );
         
         sOption.values.addItem( LOC(25118),   1 );
         sOption.values.addItem( LOC(25119),    0 );
         sOption.values.setIndex( 1 );
      break;

      case eGameType:
         sOption.name.setText( LOC(25101) );
         sOption.values.addItem( LOC(25120), 1 );
         sOption.values.addItem( LOC(8002), 2 );
         sOption.values.setIndex( 0 );
      break;

      case eDifficulty:
         sOption.name.setText( LOC(25103) );
         sOption.values.addItem( LOC(22255), 1 );
         sOption.values.addItem( LOC(22256), 2 );
         sOption.values.addItem( LOC(22257), 3 );
         sOption.values.addItem( LOC(22258), 4 );
         sOption.values.setIndex( 1 );
      break;

      case eSPCMapIndex:
      {
         sOption.name.setText( LOC(25104) );
         BCampaign* pCampaign = gCampaignManager.getCampaign( 0 );

         int scenarioIndex = 1;

         for( int nodeIdx = 0; nodeIdx < pCampaign->getNumberNodes(); ++nodeIdx )
         {
            BCampaignNode* pNode = pCampaign->getNode( nodeIdx );
            
            if( pNode->getFlag( BCampaignNode::cCinematic ) )
               continue;

            sOption.values.addItem( pNode->getDisplayName(), scenarioIndex++ );
         }

         sOption.values.setIndex( 0 );
         break;
      }

      case eLeaderIndex:
      {
         sOption.name.setText( LOC(25105) );

         for( int leaderIdx = 1; leaderIdx < gDatabase.getNumberLeaders(); ++leaderIdx )
         {
            BLeader* pLeader = gDatabase.getLeader( leaderIdx );
            BUString leaderName = gDatabase.getLocStringFromIndex(pLeader->mNameIndex).getPtr();

            if( !pLeader->mTest )
               sOption.values.addItem( leaderName, leaderIdx );
         }

         sOption.values.setIndex( 0 );
         break;
      }
   }
}

//============================================================================
//============================================================================
void BUILeaderboards::getOptionsForLBType( int eType, BDynamicArray<EOption>& options )
{
   options.clear();
   
   switch( eType )
   {
      case cLeaderBoardTypeMPSkill: 
      case cLeaderBoardTypeMPGametypeWinsLifetime: 
      case cLeaderBoardTypeMPGametypeWinsMonthly:
         options.push_back( eGameSize );
         options.push_back( eParty );
         options.push_back( eGameType );
      break;

      case cLeaderBoardTypeCampaignLifetime: 
         options.push_back( eParty );
      break;

      case cLeaderBoardTypeCampaignBestScore: 
         options.push_back( eDifficulty );
         options.push_back( eSPCMapIndex );
      break;

      case cLeaderBoardTypeMPLeaderWinsMonthly:
         options.push_back( eLeaderIndex );
         options.push_back( eGameType );
      break;
   }
}

//============================================================================
//============================================================================
int BUILeaderboards::getOptionValueByIndex( int index )
{
   return mOptions[index].values.getValue();
}

//============================================================================
//============================================================================
void BUILeaderboards::setHeadersForLBType( int eType )
{
   mRankLabel.setText( LOC(25121) );
   mGamertagLabel.setText( LOC(25122) );

   switch( eType )
   {
      case cLeaderBoardTypeMPSkill: 
         mField1Label.setText( LOC(25123) );
         mField2Label.setText( L"" );
         mField3Label.setText( L"" );
         mField2WLabel.setText( LOC(25124) );
         show3Columns( false );
         break;

      case cLeaderBoardTypeCampaignLifetime: 
         mField1Label.setText( LOC(25125) );
         mField2Label.setText( L"" );
         mField3Label.setText( L"" );
         mField2WLabel.setText( LOC(25124) );
         show3Columns( false );
         break;

      case cLeaderBoardTypeCampaignBestScore: 
         mField1Label.setText( LOC(25126) );
         mField2Label.setText( L"" );
         mField3Label.setText( L"" );
         mField2WLabel.setText( LOC(25127) );
         show3Columns( false );
         break;

      case cLeaderBoardTypeMPGametypeWinsLifetime: 
      case cLeaderBoardTypeMPLeaderWinsMonthly:
      case cLeaderBoardTypeMPGametypeWinsMonthly:
         mField1Label.setText( LOC(25128) );
         mField2Label.setText( LOC(25124) );
         mField3Label.setText( LOC(25125) );
         mField2WLabel.setText( L"" );
         show3Columns( true );
         break;  
   }
}

//============================================================================
//============================================================================
void BUILeaderboards::populateOptionList( void )
{
   mOptionList.clearControls();
   mOptionList.addControl( &mLBTypeOption.values );
   mOptionList.addControl( &mFilterOption.values );

   BDynamicArray<EOption> optionTypes;
   getOptionsForLBType( mLBTypeOption.values.getValue(), optionTypes );

   uint i = 0;
   for( ; i < optionTypes.size(); ++i )
   {
      mOptions[i].name.show();
      mOptions[i].values.enable();
      mOptions[i].values.show();
      populateOption( mOptions[i], optionTypes[i] );
      mOptionList.addControl( &(mOptions[i].values) );
   }

   mOptionList.setIndex( 0 );

   for( ; i < 3; ++i )
   {
      mOptions[i].name.hide();
      mOptions[i].values.hide();
   }
}

//============================================================================
//============================================================================
void BUILeaderboards::populateResults( void )
{
   int eType = mLBTypeOption.values.getValue();

   int selectedIndex = mRowList.getSelectedIndex();
   
   if( mRowList.getSelectedControl() && mRowList.getSelectedControl()->isFocused() )
      mRowList.getSelectedControl()->unfocus();

   mRowList.clearControls();

   for( uint32 rowIdx = 0; rowIdx < NUM_ROWS; ++rowIdx )
   {
      const leaderBoardReturnDataRow* pData = mLeaderboardView[rowIdx];
      mRows[rowIdx].setData( eType, pData );
      if( pData )
         mRowList.addControl( &mRows[rowIdx] );
   }

   mRowList.setIndex( Math::Clamp( selectedIndex, 0, mRowList.getControlCount() - 1 ) );
   if( mRowList.getSelectedControl() )
      mRowList.getSelectedControl()->focus( true );
}

//============================================================================
//============================================================================
BUIFlyoutListControl* BUILeaderboards::getOptionByType( EOption eType )
{
   BUIFlyoutListControl* pOption = NULL;
   for( int i = 0; i < mOptionList.getControlCount(); ++i )
   {
      BUIFlyoutListControl* optionControl = (BUIFlyoutListControl*)mOptionList.getControl( i );
      if( optionControl->getControlID() == eType )
      {
         pOption = optionControl;
         break;
      }
   }
   return pOption;
}

//============================================================================
//============================================================================
void BUILeaderboards::validateOptions( void )
{
   // If a new game size was selected, ensure that the Party option is valid
   BUIFlyoutListControl* pGameSizeOption = getOptionByType( eGameSize );
   BUIFlyoutListControl* pPartyOption = getOptionByType( eParty );

   if( pPartyOption && pGameSizeOption )
   {
      if( pGameSizeOption->getValue() == 2 )
      {
         pPartyOption->setValue( 0 );
         pPartyOption->disable();
      }
      else
      {
         pPartyOption->enable();
      }
   }
}

//============================================================================
//============================================================================
void BUILeaderboards::show3Columns( bool bShow )
{
   GFxValue val;
   val.SetNumber( bShow ? 1 : 2 );

   invokeActionScript( "gotoAndStop", &val, 1 );
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// UILeaderboardRecordControl
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//============================================================================
//============================================================================
BUILeaderboardRecordControl::BUILeaderboardRecordControl( void )
{
   INIT_EVENT_MAP();
}

//============================================================================
//============================================================================
BUILeaderboardRecordControl::~BUILeaderboardRecordControl( void )
{
}

//============================================================================
//============================================================================
bool BUILeaderboardRecordControl::init( IUIControlEventHandler* parent, const char* controlPath, int controlID , BXMLNode* initData )
{
   bool result = __super::init( parent, controlPath, controlID, initData );

   if( result )
      result = mRankLabel.init( this, mScriptPrefix + "mRankLabel" );
      
   if( result )
      result = mGamertagLabel.init( this, mScriptPrefix + "mGamertagLabel" );

   if( result )
      result = mField1Label.init( this, mScriptPrefix + "mField1Label" );

   if( result )
      result = mField2Label.init( this, mScriptPrefix + "mField2Label" );

   if( result )
      result = mField3Label.init( this, mScriptPrefix + "mField3Label" );

   if( result )
      result = mField2WLabel.init( this, mScriptPrefix + "mField2WLabel" );

   return result;
}

//============================================================================
//============================================================================
bool BUILeaderboardRecordControl::executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl )
{
   if( command == "select" )
   {
      fireUIControlEvent( eSelected );
      return true;
   }
   
   return false;
}

//============================================================================
//============================================================================
void BUILeaderboardRecordControl::setData( int eType, const leaderBoardReturnDataRow* pData )
{
   if( pData )
   {
      BUString rankStr;
      rankStr.locFormat( L"%d", pData->mRank );
      mRankLabel.setText( rankStr );

      mGamertagLabel.setText( pData->mGamerTag );

      BUString ratingStr;
      ratingStr.locFormat( L"%d", pData->mRating );

      BUString val1Str;
      val1Str.locFormat( L"%d", pData->mIntValue1 );

      BUString val2Str;
      val2Str.locFormat( L"%d", pData->mIntValue2 );

      switch( eType )
      {
         case cLeaderBoardTypeMPSkill: 
            mField1Label.setText( val1Str );
            mField2Label.setText( L"" );
            mField3Label.setText( L"" );
            mField2WLabel.setText( val2Str );
            show3Columns( false );
            break;

         case cLeaderBoardTypeCampaignLifetime: 
            mField1Label.setText( ratingStr );
            mField2Label.setText( L"" );
            mField3Label.setText( L"" );
            mField2WLabel.setText( val1Str );
            show3Columns( false );
            break;

         case cLeaderBoardTypeCampaignBestScore: 
            mField1Label.setText( ratingStr );
            mField2Label.setText( L"" );
            mField3Label.setText( L"" );
            mField2WLabel.setText( pData->mIntValue1 ? LOC(25133) : LOC(25134) );
            show3Columns( false );
            break;

         case cLeaderBoardTypeMPGametypeWinsLifetime: 
         case cLeaderBoardTypeMPLeaderWinsMonthly:
         case cLeaderBoardTypeMPGametypeWinsMonthly:
            mField1Label.setText( ratingStr );
            mField2Label.setText( val1Str );
            mField3Label.setText( val2Str );
            show3Columns( true );
            break;
      }
      mXUID = pData->mXuid;
   }
   else
   {
      mRankLabel.setText( L"" );
      mGamertagLabel.setText( L"" );
      mField1Label.setText( L"" );
      mField2Label.setText( L"" );
      mField3Label.setText( L"" );
      mField2WLabel.setText( L"" );
   }
}

//============================================================================
//============================================================================
XUID BUILeaderboardRecordControl::getXUID( void )
{
   return mXUID;
}


//============================================================================
//============================================================================
void BUILeaderboardRecordControl::focus( bool force/* =false */ )
{
   __super::focus( force );
}

//============================================================================
//============================================================================
void BUILeaderboardRecordControl::unfocus( bool force/* =false */ )
{
   __super::unfocus( force );
}

//============================================================================
//============================================================================
void BUILeaderboardRecordControl::show3Columns( bool bShow )
{
   GFxValue val;
   val.SetNumber( bShow ? 1 : 2 );
   invokeActionScript( "mBar.gotoAndStop", &val, 1 );
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// LeaderboardView
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
BLeaderboardView::BLeaderboardView( uint32 viewSize ) :
   mpHandler( NULL ),
   mStatus( eReady ),
   mFilter( eAll ),
   mLBIndex( 0 ),
   mXUID( 0 ),
   mRecordsReturned( 0 ),
   mTotalRecordCount( 0 ),
   mpQueryResults( NULL ),
   mbCancelRequested( false ),
   mbXboxLIVEError(false),
   mViewSize( max( 1, viewSize ) ),
   mViewIndex( 0 ),
   mMinBufferSize( viewSize + 1 ),
   mMaxBufferSize( viewSize * 2 ),
   mCacheIndex( 0 )
{
   mPendingQuery.startIndex = 0;
   mPendingQuery.numRecords = 0;
}

//============================================================================
//============================================================================
BLeaderboardView::~BLeaderboardView( void )
{
   clearQueryBuffer();
}

//============================================================================
//============================================================================
void BLeaderboardView::setHandler( IHandler* pHandler )
{
   mpHandler = pHandler;
}

//============================================================================
//============================================================================
void BLeaderboardView::executeQuery( uint32 lbIndex, EFilter eFilter, XUID xuid /*= 0*/ )
{
   mFilter = eFilter;
   mLBIndex = lbIndex;
   mXUID = xuid;
   mViewIndex = 0;
   mCacheIndex = 0;
   mCache.clear();
   getMoreRecords( 0, mViewSize + mMinBufferSize * 2 );
}

//============================================================================
//============================================================================
void BLeaderboardView::cancelQuery( void )
{
   if( mStatus == eReady )
   {
      if( mpHandler )
         mpHandler->onLeaderboardViewCancelled( this );
   }
   else
   {
      mbCancelRequested = true;

      gLiveSystem->leaderBoardCancelQuery();

      mStatus = eCancelling;
   }
}

//============================================================================
//============================================================================
bool BLeaderboardView::next( uint32 step /* = 1 */ )
{
   if (step == 0)
      return false;

   if( mStatus == eReady && getViewEndIndex() + step < mTotalRecordCount && mTotalRecordCount > mViewSize )
   {
      mViewIndex += step;
      return updateCache();
   }
   return false;
}

//============================================================================
//============================================================================
bool BLeaderboardView::prev( uint32 step /* = 1 */ )
{
   if (step == 0)
      return false;

   if( mStatus == eReady && getBufferAboveView() >= step )
   {
      mViewIndex -= step;
      return updateCache();
   }
   return false;
}

//============================================================================
//============================================================================
bool BLeaderboardView::nextPage( void )
{
   return next( min( mViewSize, mTotalRecordCount - getViewEndIndex() - 1 ) );
}

//============================================================================
//============================================================================
bool BLeaderboardView::prevPage( void )
{
   return prev( min( mViewSize, getViewStartIndex() ) );
}

//============================================================================
//============================================================================
uint32 BLeaderboardView::getRecordCount( void ) const
{
   return mTotalRecordCount;
}

//============================================================================
//============================================================================
const leaderBoardReturnDataRow* BLeaderboardView::operator [] (uint32 i) const
{
   return getRecordByViewIndex( i );
}

//============================================================================
//============================================================================
const leaderBoardReturnDataRow* BLeaderboardView::getRecordByCacheIndex( uint32 i ) const
{
   return i < mCache.getSize() ? &(mCache[i]) : NULL;
}

//============================================================================
//============================================================================
const leaderBoardReturnDataRow* BLeaderboardView::getRecordByViewIndex( uint32 i ) const
{
   return getRecordByCacheIndex( i + getViewStartIndex() - getCacheStartIndex() );
}

//============================================================================
//============================================================================
void BLeaderboardView::update( void )
{
   BLiveSystem::leaderBoardStatusResult queryStatus = gLiveSystem->leaderBoardQueryStatus();
   if( mStatus == eWaiting )
   {
      //BASSERTM(queryStatus != BLiveSystem::cLeaderBoardStatusNoQueryPending, "Unexpected Leaderboard State!" );

      if( queryStatus == BLiveSystem::cLeaderBoardStatusQueryComplete )
      {
         mbXboxLIVEError = false;

         if( mFilter == eFriends )
            mTotalRecordCount = mRecordsReturned;

         if( mRecordsReturned > 0 )
         {
            if( mCache.isEmpty() || mpQueryResults[0].mRank < mCache[0].mRank )
            {
               mCache.insert( 0, mRecordsReturned, mpQueryResults );

               mCacheIndex -= min( mCacheIndex, mRecordsReturned );
            }
            else
               mCache.pushBack( mpQueryResults, mRecordsReturned );

            if (mFilter == ePivot)
            {
               mCacheIndex = mpQueryResults[0].mRank - 1;

               // if there are fewer records than our view size then there's no reason to search for us
               // otherwise, we'll want to search for us in the list and attempt to make us the first row
               for (uint i=0; i < mRecordsReturned; ++i)
               {
                  if (mpQueryResults[i].mXuid == mXUID)
                  {
                     mViewIndex = mpQueryResults[i].mRank - 1;
                     break;
                  }
                  else if (mRecordsReturned - i <= mViewSize)
                  {
                     mViewIndex = mpQueryResults[i].mRank - 1;
                     break;
                  }
               }
            }
         }
         else
            mTotalRecordCount = 0;

         clearQueryBuffer();

         mStatus = eReady;

         if (mFilter == ePivot)
         {
            mFilter = eAll;
            updateCache();
         }

         if( mpHandler )
            mpHandler->onLeaderboardViewReady( this );
      }
      else if (queryStatus == BLiveSystem::cLeaderBoardStatusNoQueryPending)
      {
         if (gLiveSystem->leaderBoardError())
         {
            mbXboxLIVEError = true;

            BUIGlobals* pUIGlobals = gGame.getUIGlobals();
            BASSERT(pUIGlobals);
            if (pUIGlobals)
               pUIGlobals->showYornBox(NULL, gDatabase.getLocStringFromID(25450), BUIGlobals::cDialogButtonsOK);
         }

         mStatus = eReady;

         if( mpHandler )
         {
            if (mbXboxLIVEError)
               mpHandler->onLeaderboardViewCancelled( this );
            else
               mpHandler->onLeaderboardViewReady( this );
         }
      }
   }
   else if( mStatus == eCancelling && queryStatus != BLiveSystem::cLeaderBoardStatusQueryRunning )
   {
      clearQueryBuffer();
      mStatus = eReady;

      if( !mbCancelRequested )
         getMoreRecords( mPendingQuery.startIndex, mPendingQuery.numRecords );
      else if( mpHandler )
         mpHandler->onLeaderboardViewCancelled( this );

      //mbCancelRequested = false;
   }
}

//============================================================================
//============================================================================
uint32 BLeaderboardView::getCacheSize( void ) const
{
   return mCache.getSize();
}

//============================================================================
//============================================================================
uint32 BLeaderboardView::getCacheStartIndex( void ) const
{
   return mCacheIndex;
}

//============================================================================
//============================================================================
uint32 BLeaderboardView::getCacheEndIndex( void ) const
{
   return mCacheIndex + mCache.getSize() - 1;
}

//============================================================================
//============================================================================
uint32 BLeaderboardView::getViewStartIndex( void ) const
{
   return mViewIndex;
}

//============================================================================
//============================================================================
uint32 BLeaderboardView::getViewEndIndex( void ) const
{
   return mViewIndex + mViewSize - 1;
}

//============================================================================
//============================================================================
uint32 BLeaderboardView::getBufferAboveView( void ) const
{
   BASSERT(getCacheStartIndex() <= getViewStartIndex());
   return getViewStartIndex() - getCacheStartIndex();
}

//============================================================================
//============================================================================
uint32 BLeaderboardView::getBufferBelowView( void ) const
{
   uint32 viewEnd = getViewEndIndex();
   uint32 cacheEnd = getCacheEndIndex();

   if( viewEnd > cacheEnd )
      return 0;
   else
      return getCacheEndIndex() - getViewEndIndex();
}

//============================================================================
//============================================================================
bool BLeaderboardView::updateCache( void )
{
   bool fromCache = true;

   // Above
   uint32 bufferAboveView = getBufferAboveView();
   if( bufferAboveView >= mMaxBufferSize )
   {
      uint32 numErased = bufferAboveView - mMinBufferSize;
      numErased = min(numErased, mCache.getSize());
      mCache.erase( 0, numErased );
      mCacheIndex += numErased;
   }
   else if( getCacheStartIndex() > 0 && bufferAboveView <= mMinBufferSize )
   {
      fromCache = false;
      uint numRecords = min(getCacheStartIndex(), mViewSize);
      getMoreRecords( getCacheStartIndex() - min( getCacheStartIndex(), mViewSize ), numRecords );
   }

   // Below
   uint32 bufferBelowView = getBufferBelowView();
   if( bufferBelowView >= mMaxBufferSize )
   {
      uint32 first = getViewEndIndex() - getCacheStartIndex() + mMinBufferSize;
      uint32 last = mCache.getSize();
      if( first < last )
         mCache.erase( first, last );
   }
   else if( getCacheEndIndex() < mTotalRecordCount - 1 && bufferBelowView <= mMinBufferSize )
   {
      fromCache = false;
      getMoreRecords( getCacheEndIndex() + 1 );
   }

   return fromCache;
}

//============================================================================
//============================================================================
void BLeaderboardView::clearQueryBuffer()
{
   BASSERT( gLiveSystem->leaderBoardQueryStatus() != BLiveSystem::cLeaderBoardStatusQueryRunning );
      
   if( mpQueryResults )
      delete[] mpQueryResults;

   mpQueryResults = NULL;
}

//============================================================================
//============================================================================
void BLeaderboardView::getMoreRecords( uint32 startIndex, uint32 numRecords /* = 0*/ )
{
   if( mStatus == eWaiting )
   {
      if (mPendingQuery.startIndex == startIndex && mPendingQuery.numRecords == numRecords)
         return;

      gLiveSystem->leaderBoardCancelQuery();
      mPendingQuery.startIndex = startIndex;
      mPendingQuery.numRecords = numRecords;

      mStatus = eCancelling;
   }
   else if (mStatus == eReady)
   {
      if (numRecords == 0)
         numRecords = max( numRecords, mViewSize );

      if( mFilter == eFriends )
         numRecords = 101;

      clearQueryBuffer();
      mpQueryResults = new leaderBoardReturnDataRow[numRecords];
      mRecordsReturned = numRecords;
      mTotalRecordCount = numRecords;

      bool bResult = false;
      
      switch( mFilter )
      {
      case eAll:
         bResult = gLiveSystem->leaderBoardLaunchQuery( mLBIndex, 0, startIndex + 1, mpQueryResults, &mRecordsReturned, &mTotalRecordCount );
         break;

      case eFriends:
         bResult = gLiveSystem->leaderBoardFriendFilterLaunchQuery( mLBIndex, mXUID, mpQueryResults, &mRecordsReturned, &mTotalRecordCount );
         break;

      case ePivot:
         bResult = gLiveSystem->leaderBoardLaunchQuery( mLBIndex, mXUID, startIndex + 1, mpQueryResults, &mRecordsReturned, &mTotalRecordCount );
         break;
      }

      BASSERT( bResult );

      mStatus = eWaiting;
   }
}