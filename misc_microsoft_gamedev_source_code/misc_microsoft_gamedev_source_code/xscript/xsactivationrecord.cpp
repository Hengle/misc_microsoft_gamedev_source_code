//==============================================================================
// xsactivationrecord.cpp
//
// Copyright (c) 2001-2003, Ensemble Studios
//==============================================================================

//==============================================================================
// Includes
#include "xscript.h"
#include "xsactivationrecord.h"
#ifdef _BANG
#include "chunker.h"
#endif


//=============================================================================
// BXSActivationRecord::msSaveVersion
//=============================================================================
const DWORD BXSActivationRecord::msSaveVersion=0;
//=============================================================================
// BXSActivationRecord::msLoadVersion
//=============================================================================
DWORD BXSActivationRecord::msLoadVersion=0xFFFF;

//==============================================================================
// BXSActivationRecord::BXSActivationRecord
//==============================================================================
BXSActivationRecord::BXSActivationRecord(void) :
   mFunctionID(-1),
   mStack(-1),
   mHeap(-1),
   mPC(-1),
   mLineNumber(-1),
   mBreakpoint(false),
   mStepOverBreakpoint(false)
{
}

//==============================================================================
// BXSActivationRecord::BXSActivationRecord
//==============================================================================
BXSActivationRecord::BXSActivationRecord(const BXSActivationRecord &ar) :
   mFunctionID(ar.mFunctionID),
   mStack(ar.mStack),
   mHeap(ar.mHeap),
   mPC(ar.mPC),
   mLineNumber(ar.mLineNumber),
   mBreakpoint(ar.mBreakpoint)
{
}

//==============================================================================
// BXSActivationRecord::~BXSActivationRecord
//==============================================================================
BXSActivationRecord::~BXSActivationRecord(void)
{
}

//==============================================================================
// BXSActivationRecord::reset
//==============================================================================
void BXSActivationRecord::reset(void)
{
   mFunctionID=-1;
   mStack=-1;
   mHeap=-1;
   mPC=-1;
   mLineNumber=-1;
   mBreakpoint=false;
   mStepOverBreakpoint=false;
}

#ifdef _BANG
//=============================================================================
// BXSActivationRecord::writeVersion
//=============================================================================
bool BXSActivationRecord::writeVersion(BChunkWriter *chunkWriter)
{
   if(!chunkWriter)
   {
      BASSERT(0);
      {setBlogError(4279); blogerror("BXSActivationRecord::writeVersion -- bad chunkWriter");}
      return(false);
   }
   long result=chunkWriter->writeTaggedDWORD(BCHUNKTAG("xa"), msSaveVersion);
   if(!result)
   {
      {setBlogError(4280); blogerror("BXSActivationRecord::writeVersion -- failed to READ version");}
      return(false);
   }
   return(true);
}

//=============================================================================
// BXSActivationRecord::readVersion
//=============================================================================
bool BXSActivationRecord::readVersion(BChunkReader *chunkReader)
{
   if(!chunkReader)
   {
      BASSERT(0);
      {setBlogError(4281); blogerror("BXSActivationRecord::readVersion -- bad chunkReader");}
      return(false);
   }
   long result=chunkReader->readTaggedDWORD(BCHUNKTAG("xa"), &msLoadVersion);
   if(!result)
   {
      {setBlogError(4282); blogerror("BXSActivationRecord::readVersion -- failed to read version");}
      return(false);
   }
   return(true);
}

//=============================================================================
// BXSActivationRecord::save
//=============================================================================
bool BXSActivationRecord::save(BChunkWriter *chunkWriter)
{
   //Check writer validity.
   if (chunkWriter == NULL)
   {
      BASSERT(0);
      return(false);
   }

   //Write tag.
   long mainHandle;
   long result=chunkWriter->writeTagPostSized(BCHUNKTAG("xb"), mainHandle);
   if(!result)
   {
      {setBlogError(4283); blogerror("BXSActivationRecord::save -- error writing tag.");}
      return(false);
   }

   //Function ID.
   CHUNKWRITESAFE(chunkWriter, Long, mFunctionID);
   //Stack.
   CHUNKWRITESAFE(chunkWriter, Long, mStack);
   //Heap.
   CHUNKWRITESAFE(chunkWriter, Long, mHeap);
   //PC.
   CHUNKWRITESAFE(chunkWriter, Long, mPC);
   //Line number.
   CHUNKWRITESAFE(chunkWriter, Long, mLineNumber);
   //Breakpoint.
   CHUNKWRITESAFE(chunkWriter, Bool, mBreakpoint);
   //Step over breakpoint.
   CHUNKWRITESAFE(chunkWriter, Bool, mStepOverBreakpoint);

   //Finish chunk.   
   result=chunkWriter->writeSize(mainHandle);
   if(!result)
   {
      {setBlogError(4284); blogerror("BXSActivationRecord::save -- failed to write chunk size!");}
      return(false);
   }
   return(true);
}

//=============================================================================
// BXSActivationRecord::load
//=============================================================================
bool BXSActivationRecord::load(BChunkReader *chunkReader)
{
   //Check reader validity.
   if (chunkReader == NULL)
   {
      BASSERT(0);
      return(false);
   }
   //Read tag.
   long result=chunkReader->readExpectedTag(BCHUNKTAG("xb"));
   if(!result)
   {
      {setBlogError(4285); blogerror("BXSActivationRecord::load -- error reading tag.");}
      return(false);
   }

   //Function ID.
   CHUNKREADSAFE(chunkReader, Long, mFunctionID);
   //Stack.
   CHUNKREADSAFE(chunkReader, Long, mStack);
   //Heap.
   CHUNKREADSAFE(chunkReader, Long, mHeap);
   //PC.
   CHUNKREADSAFE(chunkReader, Long, mPC);
   //Line number.
   CHUNKREADSAFE(chunkReader, Long, mLineNumber);
   //Breakpoint.
   CHUNKREADSAFE(chunkReader, Bool, mBreakpoint);
   //Step over breakpoint.
   CHUNKREADSAFE(chunkReader, Bool, mStepOverBreakpoint);

   //Validate our reading of the chunk.
   result=chunkReader->validateChunkRead(BCHUNKTAG("xb"));
   if(!result)
   {
      {setBlogError(4286); blogerror("BXSActivationRecord::load -- did not read chunk properly!");}
      return(false);
   }
   return(true);
}
#endif
