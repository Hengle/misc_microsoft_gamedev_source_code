//==============================================================================
// xsruleentry.h
//
// Copyright (c) 2000-2003, Ensemble Studios
//==============================================================================

#ifndef _XSRULEENTRY_H_
#define _XSRULEENTRY_H_

//==============================================================================
// Includes

//==============================================================================
// Forward declarations
#ifdef _BANG
class BChunkReader;
class BChunkWriter;
#endif


//==============================================================================
class BXSRuleEntry
{
   public:
      enum
      {
         cMaximumPriority=100,
         cMinimumPriority=0,
         cMinimumMinInterval=1,     //1 millisecond
         cMinimumMaxInterval=60000  //1 minute
      };

      BXSRuleEntry( void );
      ~BXSRuleEntry( void );

      //ID.
      long                       getID( void ) const { return(mID); }
      void                       setID( long v ) { mID=v; }
      //Function ID.
      long                       getFunctionID( void ) const { return(mFunctionID); }
      void                       setFunctionID( long v ) { mFunctionID=v; }
      //Priority.
      long                       getPriority( void ) const { return(mPriority); }
      void                       setPriority( long v );
      //Intervals.
      DWORD                      getMinInterval( void ) const { return(mMinInterval); }
      void                       setMinInterval( DWORD v );
      DWORD                      getMaxInterval( void ) const { return(mMaxInterval); }
      void                       setMaxInterval( DWORD v );
      //Execute Time.
      DWORD                      getLastExecuteTime( void ) const { return(mLastExecuteTime); }
      void                       setLastExecuteTime( DWORD v ) { mLastExecuteTime=v; }
      //Active.
      bool                       getActive( void ) const { return(mActive); }
      void                       setActive( bool v ) { mActive=v; }
      //Run Immediately.
      bool                       getRunImmediately( void ) const { return(mRunImmediately); }
      void                       setRunImmediately( bool v ) { mRunImmediately=v; }
      //Groups.
      long                       getNumberGroups( void ) const { return(mGroups.getNumber()); }
      long*                      getGroups( void ) { return(mGroups.getPtr()); }
      bool                       addGroup( long groupID );
      bool                       removeGroup( long groupID );
      bool                       isMember( long groupID );

      //Save and Load.
      #ifdef _BANG
      static bool                writeVersion( BChunkWriter *chunkWriter );
      static bool                readVersion( BChunkReader *chunkReader );
      bool                       save( BChunkWriter* chunkWriter );
      bool                       load( BChunkReader* chunkReader );
      #endif

      //Copy.
      bool                       copy( BXSRuleEntry *v );

   protected:
      long                       mID;
      long                       mFunctionID;
      long                       mPriority;
      DWORD                      mMinInterval;
      DWORD                      mMaxInterval;
      DWORD                      mLastExecuteTime;
      bool                       mActive;
      bool                       mRunImmediately;
      BDynamicSimLongArray           mGroups;

      static const DWORD         msSaveVersion;
      static DWORD               msLoadVersion;
};
typedef BDynamicSimArray<BXSRuleEntry*> BXSRuleEntryArray;


//==============================================================================
class BXSRuleGroupEntry
{
   public:
      BXSRuleGroupEntry( void );
      ~BXSRuleGroupEntry( void );

      //ID.
      long                       getID( void ) const { return(mID); }
      void                       setID( long v ) { mID=v; }
      //Symbol ID.
      long                       getSymbolID( void ) const { return(mSymbolID); }
      void                       setSymbolID( long v ) { mSymbolID=v; }
      //Active.
      bool                       getActive( void ) const { return(mActive); }
      void                       setActive( bool v ) { mActive=v; }
      //Rules.
      long                       getNumberRules( void ) const { return(mRules.getNumber()); }
      long*                      getRules( void ) { return(mRules.getPtr()); }
      bool                       addRule( long ruleID );
      bool                       removeRule( long ruleID );
      bool                       isMember( long ruleID );

      //Save and Load.
      #ifdef _BANG
      static bool                writeVersion( BChunkWriter *chunkWriter );
      static bool                readVersion( BChunkReader *chunkReader );
      bool                       save( BChunkWriter* chunkWriter );
      bool                       load( BChunkReader* chunkReader );
      #endif

      //Copy.
      bool                       copy( BXSRuleGroupEntry *v );

   protected:
      long                       mID;
      long                       mSymbolID;
      bool                       mActive;
      BDynamicSimLongArray       mRules;

      static const DWORD         msSaveVersion;
      static DWORD               msLoadVersion;
};
typedef BDynamicSimArray<BXSRuleGroupEntry*> BXSRuleGroupEntryArray;


//==============================================================================
#endif // _XSRULEENTRY_H_
