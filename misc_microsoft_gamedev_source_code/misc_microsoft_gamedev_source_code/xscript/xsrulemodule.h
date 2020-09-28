//==============================================================================
// xsrulemodule.h
//
// Copyright (c) 2000-2003, Ensemble Studios
//==============================================================================

#ifndef _XSRULEMODULE_H_
#define _XSRULEMODULE_H_

//==============================================================================
// Includes
#include "xsruleentry.h"

//==============================================================================
// Forward declarations
#ifdef _BANG
class BChunkReader;
class BChunkWriter;
#endif
class BXSFunctionEntry;


//==============================================================================
class BXSRuleModule
{
   public:
      BXSRuleModule( void );
      virtual ~BXSRuleModule( void );

      //getFunctionEntry must be defined by BXSData.
      virtual BXSFunctionEntry*  getFunctionEntry( long id ) const = 0;

      //Rule methods.
      bool                       allocateRules( long number );
      long                       getNumberRules( void ) const { return(mRules.getNumber()); }
      BXSRuleEntry*              allocateRuleEntry( void );
      BXSRuleEntry*              getRuleEntry( long id ) const;
      long                       getRuleID( long functionID );
      long                       getRuleID( const char *name );
      //Rule active-ness.
      bool                       setRuleActive( long id, bool v, DWORD currentTime );
      bool                       setRuleActive( const char *name, bool v, DWORD currentTime );
      bool                       isRuleActive( long id );
      bool                       isRuleActive( const char *name );
      //Runtime rule modifications.
      bool                       modifyRulePriority( long ruleID, long newPriority );
      bool                       modifyRuleMinInterval( long ruleID, DWORD newMinInterval ) const;
      bool                       modifyRuleMaxInterval( long ruleID, DWORD newMaxInterval ) const;
      //Rule sorting.
      long                       getSortedRuleID( long index );
      bool                       sortRules( void );

      //Rule Group methods.
      bool                       allocateRuleGroups( long number );
      long                       getNumberRuleGroups( void ) const { return(mRuleGroups.getNumber()); }
      BXSRuleGroupEntry*         allocateRuleGroupEntry( void );
      BXSRuleGroupEntry*         getRuleGroupEntry( long id ) const;
      bool                       setRuleGroupActive( long id, bool v, DWORD currentTime );
      bool                       setRuleGroupActive( const char *name, bool v, DWORD currentTime );
      bool                       isRuleGroupActive( long id );
      bool                       isRuleGroupActive( const char *name );

      //Time-sliced processing methods.
      void                       resetNextSortedRuleIndex( void );
      long                       getNextSortedRuleID( void ) const;
      long                       getNextSortedRuleIndex( void ) const { return(mNextSortedRuleIndex); }
      long                       incrementNextSortedRuleIndex( void );

      //Data copy (for multi-instanced data support in BXSRuntime).
      bool                       copyData( BXSRuleModule *rm );

      //Save and load.
      #ifdef _BANG
      bool                       save( BChunkWriter *chunkWriter );
      bool                       load( BChunkReader *chunkReader );
      #endif

   protected:
      void                       cleanUp();

      //This is a pure virtual function because it needs to be overridden by a child
      //class with some sort of symbol lookup.
      virtual long               getSymbolID( const char *name, long type ) const = 0;
      virtual long               getSymbolValue( const char *name, long type ) const = 0;

      //Rules.
      BXSRuleEntryArray          mRules;
      //Sorted Rules.
      BXSRuleEntryArray          mSortedRules;
      //Rule Groups.
      BXSRuleGroupEntryArray     mRuleGroups;

      long                       mNextSortedRuleIndex;

      //Static savegame stuff.
      static const DWORD         msSaveVersion;
};


//==============================================================================
#endif // _XSRULEMODULE_H_
