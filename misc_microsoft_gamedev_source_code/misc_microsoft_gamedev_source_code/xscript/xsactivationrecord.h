//==============================================================================
// xsactivationrecord.h
//
// Copyright (c) 2001-2003, Ensemble Studios
//==============================================================================

#ifndef _XSACTIVATIONRECORD_H_
#define _XSACTIVATIONRECORD_H_

//==============================================================================
// Includes

//==============================================================================
// Forward declarations
#ifdef _BANG
class BChunkReader;
class BChunkWriter;
#endif



//==============================================================================
class BXSActivationRecord
{
   public:
      BXSActivationRecord( void );
      BXSActivationRecord( const BXSActivationRecord &ar );
      ~BXSActivationRecord( void );

      //Function ID.  This is the function ID for the function.
      long                       getFunctionID( void ) const { return(mFunctionID); }
      void                       setFunctionID( long v ) { mFunctionID=v; }
      //Stack.  This is where the stack was BEFORE the function was 'called'.
      long                       getStack( void ) const { return(mStack); }
      void                       setStack( long v ) { mStack=v; }
      //Heap.  This is where the heap was BEFORE the function was 'called'.
      long                       getHeap( void ) const { return(mHeap); }
      void                       setHeap( long v ) { mHeap=v; }
      //PC.  This is the current PC for this function 'call'.  Changes as
      //the function progress with execution.
      long                       getPC( void ) const { return(mPC); }
      void                       setPC( long v ) { mPC=v; }
      void                       incrementPC( long v ) { mPC+=v; }
      //Line number.  Current line number for the function being 'called'.
      long                       getLineNumber( void ) const { return(mLineNumber); }
      void                       setLineNumber( long v ) { mLineNumber=v; }
      //Breakpoint.
      bool                       getBreakpoint( void ) const { return(mBreakpoint); }
      void                       setBreakpoint( bool v ) { mBreakpoint=v; }

      //Step Over Breakpoint -- used to indicate that we want to break when we pop BACK to this level for "step over".
      bool                       getStepOverBreakpoint( void ) const { return(mStepOverBreakpoint); }
      void                       setStepOverBreakpoint( bool v ) { mStepOverBreakpoint=v; }

      //Misc.
      void                       reset( void );
      long                       operator==( const BXSActivationRecord &ar ) const
                                 {
                                    return( (mFunctionID == ar.mFunctionID) &&
                                       (mStack == ar.mStack) &&
                                       (mHeap == ar.mHeap) &&
                                       (mPC == ar.mPC) &&
                                       (mLineNumber == ar.mLineNumber) &&
                                       (mBreakpoint == ar.mBreakpoint) );
                                 }
      long                       operator!=( const BXSActivationRecord &ar ) const
                                 {
                                    return(!(*this == ar));
                                 }
      BXSActivationRecord&       operator=( const BXSActivationRecord &ar )
                                 {
                                    if (this == &ar)
                                    {
                                       return *this;
                                    }
                                    mFunctionID=ar.mFunctionID;
                                    mStack=ar.mStack;
                                    mHeap=ar.mHeap;
                                    mPC=ar.mPC;
                                    mLineNumber=ar.mLineNumber;
                                    mBreakpoint=ar.mBreakpoint;
                                    mStepOverBreakpoint=ar.mStepOverBreakpoint;
                                    return(*this);
                                 }

      //Save and Load.
      #ifdef _BANG
      static bool             writeVersion( BChunkWriter *chunkWriter );
      static bool             readVersion( BChunkReader *chunkReader );
      static void             setVersion( DWORD v ) { msLoadVersion=v; }
      bool                    save( BChunkWriter* chunkWriter );
      bool                    load( BChunkReader* chunkReader );
      #endif

   protected:
      long                       mFunctionID;
      long                       mStack;
      long                       mHeap;
      long                       mPC;
      long                       mLineNumber;
      bool                       mBreakpoint;
      bool                       mStepOverBreakpoint;

      static const DWORD      msSaveVersion;
      static DWORD            msLoadVersion;
};
typedef BDynamicArray<BXSActivationRecord> BXSActivationRecordArray;


//==============================================================================
#endif // _XSACTIVATIONRECORD_H_

//==============================================================================
// eof: xsactivationrecord.h
//==============================================================================
