//==============================================================================
// xsdata.h
//
// Copyright (c) 2000-2003, Ensemble Studios
//==============================================================================

#ifndef _XSDATA_H_
#define _XSDATA_H_

//NOTE: BXSDATA DOES NO VALIDATION on the mSource member variable (for speed).  It
//assumes the world will have already blown itself to hell if an invalid source
//pointer is handed down to it.

//==============================================================================
// Includes
#include "xsactivationrecord.h"
#include "xsdefines.h"
#include "xsfunctionentry.h"
#include "xsrulemodule.h"
#include "xssymboltable.h"
#include "xssyscallentry.h"
#include "xsuserclassentry.h"
#include "xsvariableentry.h"
#include "color.h"

//==============================================================================
// Forward declarations
#ifdef _BANG
class BChunkReader;
class BChunkWriter;
#endif
class BXSFileEntry;
class BXSMessenger;
class BXSSource;


//==============================================================================
class BXSBreakpoint
{
   public:
      BXSBreakpoint( long fileID, long lineNumber ) :
         mFileID(fileID),
         mLineNumber(lineNumber)
         { }
      BXSBreakpoint( const BXSBreakpoint& b ) :
         mFileID(b.mFileID),
         mLineNumber(b.mLineNumber)
         { }
      BXSBreakpoint( void ) :
         mFileID(-1),
         mLineNumber(-1)
         { }
      ~BXSBreakpoint( void ) { }

      //Operators.
      long                       operator==( const BXSBreakpoint &b ) const { return((mFileID == b.mFileID) && (mLineNumber == b.mLineNumber)); }
      long                       operator!=( const BXSBreakpoint &b ) const { return(!(*this==b)); }
      BXSBreakpoint&             operator=( const BXSBreakpoint &b ) { mFileID=b.mFileID; mLineNumber=b.mLineNumber; return(*this); }

      //FileID.
      long                       getFileID( void ) const { return(mFileID); }
      void                       setFileID( long v ) { mFileID=v; }
      //Line Number.
      long                       getLineNumber( void ) const { return(mLineNumber); }
      void                       setLineNumber( long v ) { mLineNumber=v; }

      //Save and Load.
      #ifdef _BANG
      static bool                writeVersion( BChunkWriter *chunkWriter );
      static bool                readVersion( BChunkReader *chunkReader );
      static void                setVersion( DWORD v ) { msLoadVersion=v; }
      bool                       save( BChunkWriter* chunkWriter );
      bool                       load( BChunkReader* chunkReader );
      #endif

   protected:
      long                       mFileID;
      long                       mLineNumber;

      static const DWORD         msSaveVersion;
      static DWORD               msLoadVersion;
};
typedef BDynamicSimArray<BXSBreakpoint> BXSBreakpointArray;



//==============================================================================
class BXSEvent
{
   public:
      BXSEvent( long functionID, long parameter ) :
         mFunctionID(functionID),
         mParameter(parameter) { }
      BXSEvent( const BXSEvent &e ) :
         mFunctionID(e.mFunctionID),
         mParameter(e.mParameter) { }
      BXSEvent( void ) { reset(); }

      long                       operator==( const BXSEvent &e ) const { return(((mFunctionID == e.mFunctionID) && (mParameter == e.mParameter))); }
      long                       operator!=( const BXSEvent &e ) const { return(!(*this == e)); }
      BXSEvent&                  operator=( const BXSEvent& e ) { if (this == &e) return *this; mFunctionID=e.mFunctionID; mParameter=e.mParameter; return(*this); }
      
      long                       getFunctionID( void ) const { return(mFunctionID); }
      void                       setFunctionID( long v ) { mFunctionID=v; }
      long                       getParameter( void ) const { return(mParameter); }
      void                       setParameter( long v ) { mParameter=v; }

      void                       reset( void ) { mFunctionID=mParameter=-1; }

      //Save and Load.
      #ifdef _BANG
      static bool                writeVersion( BChunkWriter *chunkWriter );
      static bool                readVersion( BChunkReader *chunkReader );
      bool                       save( BChunkWriter* chunkWriter );
      bool                       load( BChunkReader* chunkReader );
      #endif

   protected:
      long                       mFunctionID;
      long                       mParameter;

      static const DWORD         msSaveVersion;
      static DWORD               msLoadVersion;
};
typedef BDynamicSimArray<BXSEvent> BXSEventArray;


//==============================================================================
class BXSArrayInfo
{
   public:
      BXSArrayInfo() : mArrayID(-1), mArrayType(-1) {};
      ~BXSArrayInfo() {}

      BSimString                    mName;
      long                       mArrayID;
      long                       mArrayType;
};

//==============================================================================
class BXSData : public BXSRuleModule
{
   public:
      //Ctors/Dtor.
      BXSData( long id, const BSimString &name, BXSMessenger *messenger, BXSSource *source );
      virtual ~BXSData( void );

      //ID.
      long                       getID( void ) const { return(mID); }
      void                       setID( long v ) { mID=v; }
      //Name.
      const BSimString&             getName( void ) const { return(mName); }
      void                       setName( const BSimString &v ) { mName=v; }

      //Ready to execute.
      bool                       getReadyToExecute( void ) const { return(mReadyToExecute); }
      void                       setReadyToExecute( bool v ) { mReadyToExecute=v; mBreakpointGo=cBreakpointGoNot; }

      // In use flag used by allocator of the data so it can reuse freed data objects.
      bool                       getInUse() const                 { return mInUse; }
      void                       setInUse(bool inUse)             { mInUse = inUse; }

      //File helpers.
      BXSFileEntry*              getFileEntryByCallStack( void ) const;
      long                       getFileIDByCallStack( void ) const;

      //Variables.
      long                       getNumberVariables( void ) const { return(mVariables.getNumber()); }
      BXSVariableEntry*          getVariableEntry( long id ) const;
      long                       addVariableEntry( BXSVariableEntry *newVE );
      //Exported variables.
      long                       getNumberExportedVariables( void ) const { return(mExportedVariables.getNumber()); }
      long                       getExportedVariableID( long index ) const;

      //Function lookup.
      long                       getNumberFunctions( void ) const { return(mFunctions.getNumber()); }
      BXSFunctionEntry*          allocateFunctionEntry( void );
      virtual BXSFunctionEntry*  getFunctionEntry( long id ) const;
      long                       getMainFunctionID( void ) const { return(mMainFunctionID); }
      void                       setMainFunctionID( long v ) { mMainFunctionID=v; }
      long                       getFunctionID( const char *name ) const;

      //UserClass lookup.
      long                       getNumberUserClasses( void ) const { return(mUserClasses.getNumber()); }
      BXSUserClassEntry*         allocateUserClassEntry( void );
      BXSUserClassEntry*         getUserClassEntry( long id ) const;
      long                       getUserClassID( const char *name ) const;

      // arrays
      long                       arrayCreateInt(long size, long defaultValue, const BSimString& name);
      bool                       arraySetInt(long arrayID, long index, long value);
      long                       arrayGetInt(long arrayID, long index);
      bool                       arrayResizeInt(long arrayID, long newSize);

      long                       arrayCreateFloat(long size, float defaultValue, const BSimString& name);
      bool                       arraySetFloat(long arrayID, long index, float value);
      float                      arrayGetFloat(long arrayID, long index);

      long                       arrayCreateBool(long size, bool defaultValue, const BSimString& name);
      bool                       arraySetBool(long arrayID, long index, bool value);
      bool                       arrayGetBool(long arrayID, long index);

      long                       arrayCreateString(long size, const BSimString& defaultValue, const BSimString& name);
      bool                       arraySetString(long arrayID, long index, const BSimString& value);
      const char*                arrayGetString(long arrayID, long index);

      long                       arrayCreateVector(long size, const BVector& defaultValue, const BSimString& name);
      bool                       arraySetVector(long arrayID, long index, const BVector& value);
      BVector                    arrayGetVector(long arrayID, long index);

      long                       arrayGetSize(long arrayID);
      void                       dumpArrays();

      //Events.
      long                       getNumberEvents( void ) const { return(mEvents.getNumber()); }
      BXSEvent*                  getEvent( long index );
      bool                       addEvent( long functionID, long parameter );
      void                       reduceEvents( long newNumber );
      void                       clearEvents( void ) { mEvents.setNumber(0); }

      //High level parsing (does checks to validate data).
      bool                       parseVariable( long *value );
      bool                       parseSyscallIndex( long *value );
      //Low level parsing (only does check to avoid pushing programCounter past the
      //end of the code.
      bool                       parseLong( long *value );
      bool                       parseWord( long *value );
      bool                       parseByte( long *value );

      
      //Stack and Heap.  These enums are here to just simplify how the heap storage
      //is done.  The overhead size is really just a simple abstraction to say that
      //the the heap adds X bytes to the front of each heap entry for management
      //(where X is the sum of the symbol, type, and length fields).
      enum
      {
         cHeapSymbolIDOffset=0,
         cHeapTypeOffset=sizeof(long),
         cHeapLengthOffset=sizeof(long)+sizeof(long),
         cHeapDataOffset=sizeof(long)+sizeof(long)+sizeof(long),
         cHeapOverheadSize=sizeof(long)*3
      };
      BDynamicSimLongArray&          getStack( void ) { return(mStack); }
      BDynamicSimBYTEArray&          getHeap( void ) { return(mHeap); }
      bool                       initializeHeap( void );
      bool                       deinitializeHeap( void );

      //Interpretation helpers.
      bool                       pushFunctionOn( const BXSFunctionEntry* fe, long oldPC, long oldStack, long oldHeap );
      bool                       popFunctionOff( void );

      bool                       pushVariableOn( const BXSVariableEntry* fe );
      bool                       pushVariableOn( long symbolID, long type, long dataLength, void *data );
      bool                       pushVariableOn( long heapIndex );
      bool                       popVariableOff( void );
      //Interpret counter.  A simple count of how many times some interpret method
      //has been called with this data (count pushed in from the interpreter).  
      //Helpful in determining that there could have been a change in the data (if
      //you track the last counter).
      long                       getInterpretCounter( void ) const { return(mInterpretCounter); }
      void                       setInterpretCounter( long v ) { mInterpretCounter=v; }
      void                       incrementInterpretCounter( void ) { mInterpretCounter++; }

      //PC.
      long                       getPC( void ) const;
      bool                       setPC( long v );
      bool                       incrementPC( long v );

      //Breakpoint methods.
      bool                       setBreakpoint( const char *filename, long lineNumber, bool on );
      bool                       setBreakpoint( long fileID, long lineNumber, bool on );
      bool                       setFunctionStartBreakpoint( long functionID, bool on );
      bool                       setStepOverBreakpoint(void);
      bool                       checkBreakpoints( void );
      bool                       getBreakpoint( void ) const;
      void                       setBreakpoint( bool v );
      bool                       hasBreakpoint( long fileID, long lineNumber ) const;
      bool                       checkAndClearStepOverBreakpoint(void);
      void                       breakNow(void) {mBreakNow=true;}
      bool                       checkAndClearBreakNow(void);
      //Breakpoint Go.
      enum
      {
         cBreakpointGoNot=0,
         cBreakpointGoRun=1,
         cBreakpointGoSingleStep=2,
         cBreakpointGoSingleStepOver=3
      };
      long                       getBreakpointGo( void ) const { return(mBreakpointGo); }
      void                       setBreakpointGo( long v ) { mBreakpointGo=v; }
      //Calling string.
      const char*                getCallingString( void ) const { return(mCallingString); }
      void                       setCallingString( const char *text );

      //DCP TODO 08/27/01: Rename this so it's not to be confused with the other getCurrentXXXX methods.
      //Current rule.
      long                       getCurrentRuleID( void ) const { return(mCurrentRuleID); }
      void                       setCurrentRuleID( long v ) { mCurrentRuleID=v; }

      //Resets.
      void                       resetAll( void );
      void                       resetStackAndHeap( void );
      void                       resetFailsafes( void );

      //Current File.
      long                       getCurrentFileID( void ) const;
      long                       getCurrentFunctionID( void ) const;
      //Current line.
      long                       getCurrentLineNumber( void ) const;
      void                       setCurrentLineNumber( long v );
      //Call stack (activation records).
      long                       getCallStackSize( void ) const { return(mCallStack.getNumber()); }
      BXSActivationRecordArray&  getCallStack( void ) { return(mCallStack); }
      long                       getTopStackStart( void );
      //Infinite loop limit.
      long                       getInfiniteLoopLimit( void ) const { return(mInfiniteLoopLimit); }
      void                       setInfiniteLoopLimit( long v ) { mInfiniteLoopLimit=v; }
      long                       getInfiniteLoopPCJumpValue( void ) const { return(mInfiniteLoopPCJumpValue); }
      void                       setInfiniteLoopPCJumpValue( long v ) { mInfiniteLoopPCJumpValue=v; }
      long                       getInfiniteLoopPCJumpCount( void ) const { return(mInfiniteLoopPCJumpCount); }
      void                       setInfiniteLoopPCJumpCount( long v ) { mInfiniteLoopPCJumpCount=v; }
      //Infinite recursion.
      long                       getInfiniteRecursionLimit( void ) const { return(mInfiniteRecursionLimit); }
      void                       setInfiniteRecursionLimit( long v ) { mInfiniteRecursionLimit=v; }

      //Current time.
      DWORD                      getCurrentTime( void ) const { return(mCurrentTime); }
      void                       setCurrentTime( DWORD v ) { mCurrentTime=v; }
      void                       resetRuleTime(DWORD currentTime);

      //Temp return value holder.
      bool                       allocateTempReturnValue( long size );
      bool                       setTempReturnValue( BYTE *v, long size );
      BYTE*                      getTempReturnValue( void ) const { return(mTempReturnValue); }
      long                       getTempReturnValueSize( void ) { return(mTempReturnValueSize); }

      //User heap.
      enum
      {
         cUserHeapTypeOffset=0,
         cUserHeapSizeOffset=sizeof(long),
         cUserHeapMaxSizeOffset=sizeof(long)*2,
         cUserHeapRefCountOffset=sizeof(long)*3,
         cUserHeapValueOffset=sizeof(long)*4,
         cUserHeapOverheadSize=sizeof(long)*4,
      };
      long                       getMaximumNumberUserHeapEntries( void ) const { return(mUserHeap.getNumber()); }
      BYTE*                      getUserHeapEntry( long index ) const;
      BYTE*                      getUserHeapValue( long index ) const;
      bool                       setUserHeapValue( long index, long type, long size, BYTE *value );
      bool                       incrementUserHeapRefCount( long index );
      bool                       decrementUserHeapRefCount( long index );
      long                       allocateUserHeapValue( long type, long size );
      long                       copyUserHeapValue( long index );

      //UnitID.
      long                       getUnitID( void ) const { return(mUnitID); }
      void                       setUnitID( long v ) { mUnitID=v; }

      //Output storage.
      bool                       getOutputChanged( void ) const { return(mOutputChanged); }
      void                       clearOutputChanged( void ) { mOutputChanged=false; }
      void                       addOutput( const BColor &color, const char *v, ... );
      const BDynamicSimArray<BSimString>& getOutput( void ) const { return(mOutput); }
      const BDynamicSimArray<BColor>& getOutputColors( void ) const { return(mOutputColors); }
      long                       getOutputLineNumber( void ) const { return(mOutputLineNumber); }
      void                       setOutputLineNumber( long v ) { mOutputLineNumber=v; }

      //Save and load.
      #ifdef _BANG
      bool                       save( BChunkWriter *chunkWriter );
      bool                       load( BChunkReader *chunkReader );
      #endif

      //Data copy (for multi-instanced data support in BXSRuntime).
      bool                       copyData( BXSData *data );

   protected:
      //Required methods based on BXSRuleModule.
      virtual long               getSymbolID( const char *name, long type ) const;
      virtual long               getSymbolValue( const char *name, long type ) const;
      void                       cleanUp( bool cleanID );

      void                       dumpIntegerArrayEntry(long i, long k) { blogtrace("          [%d]: %d", k, mIntegerArrays[i]->get(k)); }
      void                       dumpFloatArrayEntry(long i, long k)   { blogtrace("          [%d]: %f", k, mFloatArrays[i]->get(k)); }
      void                       dumpBoolArrayEntry(long i, long k)    { blogtrace("          [%d]: %s", k, mBoolArrays[i]->get(k) ? "true" : "false"); }
      void                       dumpStringArrayEntry(long i, long k)  { blogtrace("          [%d]: \"%s\"", k, mStringArrays[i]->get(k).getPtr()); }
      void                       dumpVectorArrayEntry(long i, long k)  { blogtrace("          [%d]: (%f, %f, %f)", k, mVectorArrays[i]->get(k).x, mVectorArrays[i]->get(k).y, mVectorArrays[i]->get(k).z); }


      //ID.
      long                       mID;
      //Name.
      BSimString                    mName;

      //Source.
      BXSSource*                 mSource;

      //Functions.
      BXSFunctionEntryArray      mFunctions;
      //Variables.
      BXSVariableEntryArray      mVariables;
      BDynamicSimLongArray           mExportedVariables;
      //User Classes.
      BXSUserClassEntryArray     mUserClasses;
      //Events.
      BXSEventArray              mEvents;

      //Arrays.
      BDynamicSimArray<BXSArrayInfo*>            mArrayInfos;
      BDynamicSimArray<BDynamicSimLongArray*>        mIntegerArrays;
      BDynamicSimArray<BDynamicSimFloatArray*>       mFloatArrays;
      BDynamicSimArray<BDynamicSimArray<bool>*>      mBoolArrays;
      BDynamicSimArray<BDynamicSimArray<BSimString>*>   mStringArrays;
      BDynamicSimArray<BDynamicSimVectorArray*>      mVectorArrays;

      //Interp stuff.
      BDynamicSimLongArray           mStack;
      BDynamicSimBYTEArray           mHeap;
      long                       mInterpretCounter;
      //Main function ID.
      long                       mMainFunctionID;
      //Activation records.
      BXSActivationRecordArray   mCallStack;
      //Current rule.
      long                       mCurrentRuleID;
      //Failsafes.
      long                       mInfiniteLoopPCJumpValue;
      long                       mInfiniteLoopPCJumpCount;
      long                       mInfiniteLoopLimit;
      long                       mInfiniteRecursionLimit;
      //Breakpoint.
      BXSBreakpointArray         mBreakpoints;
      long                       mBreakpointGo;
      //Calling string.  Temporary for each call.
      char                       mCallingString[512];
      //Time storage (pushed in from the outside).
      DWORD                      mCurrentTime;
      //Temp return value holder.
      BYTE*                      mTempReturnValue;
      long                       mTempReturnValueSize;
      long                       mTempReturnValueMaxSize;

      //User Heap.
      BDynamicSimArray<BYTE*>        mUserHeap;
      long                       mUnitID;

      //Output.
      BDynamicSimArray<BSimString>      mOutput;
      BDynamicSimArray<BColor>       mOutputColors;
      long                       mOutputLineNumber;

      //Messenger.
      BXSMessenger*              mMessenger;
      //Ready to execute.
      bool                       mReadyToExecute;
      bool                       mInUse;
      bool                       mBreakNow;
      bool                       mOutputChanged;

      //Static savegame stuff.
      static const DWORD         msSaveVersion;
};
typedef BDynamicSimArray<BXSData*> BXSDataArray;


//==============================================================================
#endif // _XSDATA_H_
