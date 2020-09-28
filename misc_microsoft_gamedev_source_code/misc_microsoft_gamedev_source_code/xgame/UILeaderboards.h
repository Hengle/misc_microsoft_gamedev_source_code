//============================================================================
// UILeaderboards.h
// Ensemble Studios (c) 2008
//============================================================================
#pragma once

#include "UIControl.h"
#include "UIScreen.h"
#include "UIListControl.h"
#include "UIButtonBarControl.h"
#include "UITextFieldControl.h"
#include "UIPanelControl.h"
#include "UIFlyoutListControl.h"
#include "LiveSystem.h"

//============================================================================
// BLeaderboardView
//============================================================================
class BLeaderboardView
{
public:
   struct IHandler
   {
      virtual void onLeaderboardViewReady( BLeaderboardView* pView ) = 0;
      virtual void onLeaderboardViewCancelled( BLeaderboardView* pView ) = 0;
   };

public:
   BLeaderboardView( uint32 viewSize );
   ~BLeaderboardView( void );

   void setHandler( IHandler* pHandler );

   enum EFilter { eAll, eFriends, ePivot, NUM_FILTERS };
   void executeQuery( uint32 lbIndex, EFilter eFilter = eAll, XUID xuid = 0 );
   void cancelQuery( void );

   bool next( uint32 step = 1 );
   bool prev( uint32 step = 1 );
   bool nextPage( void );
   bool prevPage( void );

   uint32 getRecordCount( void ) const;
   
   const leaderBoardReturnDataRow* operator [] (uint32 i) const;
   
   const leaderBoardReturnDataRow* getRecordByCacheIndex( uint32 i ) const;
   const leaderBoardReturnDataRow* getRecordByViewIndex( uint32 i ) const;

   void update( void );

   uint32 getCacheSize( void ) const;

   bool isWaiting() const { return mStatus == eWaiting; }
   bool isError() const { return mbXboxLIVEError; }

private:
   uint32 getCacheStartIndex( void ) const;
   uint32 getCacheEndIndex( void ) const;

   uint32 getViewStartIndex( void ) const;
   uint32 getViewEndIndex( void ) const;

   uint32 getBufferAboveView( void ) const;
   uint32 getBufferBelowView( void ) const;

   bool updateCache( void );

   void clearQueryBuffer( void );
   void getMoreRecords( uint32 startIndex, uint32 numRecords = 0 );

   // Handler
   IHandler* mpHandler;

   // Query stuff
   enum EStatus{ eWaiting, eReady, eCancelling };
   EStatus mStatus;
   EFilter mFilter;
   struct SPendingQuery
   {
      uint32 startIndex;
      uint32 numRecords;
   } mPendingQuery;

   bool mbCancelRequested;
   bool mbXboxLIVEError;

   uint32 mLBIndex, mRecordsReturned, mTotalRecordCount;
   XUID mXUID;
   leaderBoardReturnDataRow* mpQueryResults;

   // Cache stuff
   uint32 mCacheIndex, mMinBufferSize, mMaxBufferSize;
   BDynamicArray<leaderBoardReturnDataRow> mCache;

   // View stuff
   uint32 mViewSize, mViewIndex;
};

//============================================================================
// BUILeaderboardRecordControl
//============================================================================
class BUILeaderboardRecordControl : public BUIControl
{
public:
   enum Events
   {
      eSelected = UILeaderboardRecordControlID,
   };

   BEGIN_EVENT_MAP( UILeaderboardRecordControl )
      MAP_CONTROL_EVENT( Selected );
   END_EVENT_MAP()

   IMPLEMENT_CONTROL_TYPE_ACCESSORS( UILeaderboardRecordControl );

public:
   BUILeaderboardRecordControl( void );
   virtual ~BUILeaderboardRecordControl( void );

   bool init( IUIControlEventHandler* parent, const char* controlPath, int controlID = -1, BXMLNode* initData = NULL );

   bool executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl );

   void setData( int eType, const leaderBoardReturnDataRow* pData );
   XUID getXUID( void );

   void focus( bool force = false );
   void unfocus( bool force = false );

protected:

   void show3Columns( bool bShow );

   BUITextFieldControl mRankLabel;
   BUITextFieldControl mGamertagLabel;
   BUITextFieldControl mField1Label;
   BUITextFieldControl mField2Label;
   BUITextFieldControl mField3Label;
   BUITextFieldControl mField2WLabel;

   XUID mXUID;
};


//============================================================================
// BUILeaderboards
//============================================================================
class BUILeaderboards : public BUIScreen, public BLeaderboardView::IHandler
{
public:
   BUILeaderboards( void );
   virtual ~BUILeaderboards( void );

   // BFlashScene
   virtual bool init( BXMLNode root );
   virtual bool handleUIControlEvent( BUIControlEvent& event );

   virtual bool executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl );
   virtual bool handleInput( int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail );

   virtual void executeQuery( void );
   virtual void update( float dt );

   virtual void onLeaderboardViewReady( BLeaderboardView* pView );
   virtual void onLeaderboardViewCancelled( BLeaderboardView* pView );

protected:

   BUITextFieldControl mDebugText;

   enum EOption { eLeaderboardType, eFilter, eGameSize, eParty, eGameType, eDifficulty, eSPCMapIndex, eLeaderIndex };

   BUITextFieldControl mTitle;

   // Top-Level Focus
   BUIListControl mFocusList;

   // Options
   struct SOption
   {
      BUITextFieldControl name;
      BUIFlyoutListControl values;
   };

   SOption mLBTypeOption;
   SOption mFilterOption;
   SOption mOptions[3];
   
   BUIListControl mOptionList;

   // Headers
   BUITextFieldControl mRankLabel;
   BUITextFieldControl mGamertagLabel;
   BUITextFieldControl mField1Label;
   BUITextFieldControl mField2Label;
   BUITextFieldControl mField3Label;
   BUITextFieldControl mField2WLabel;

   // Rows
   static const long NUM_ROWS = 15;
   BUIListControl mRowList;
   BUILeaderboardRecordControl mRows[NUM_ROWS];

   // Status Text
   BUITextFieldControl mStatusText;

   // Button Bar
   BUIButtonBarControl mButtonBar;


   // Leaderboard View
   BLeaderboardView mLeaderboardView;
   
   void populateOption( SOption& sOption, EOption eOption );

   void getOptionsForLBType( int eType, BDynamicArray<EOption>& options );

   int getOptionValueByIndex( int index );

   void setHeadersForLBType( int eType );

   void populateOptionList( void );

   void populateResults( void );

   BUIFlyoutListControl* getOptionByType( EOption eType );

   void validateOptions( void );

   void show3Columns( bool bShow );

   BOOL mCancelling;
};