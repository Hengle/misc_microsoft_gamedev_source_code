//==============================================================================
// xruleentry.cpp
//
// Copyright (c) 2000-2003, Ensemble Studios
//==============================================================================

//==============================================================================
// Includes
#include "xscript.h"
#include "xsruleentry.h"
#ifdef _BANG
#include "chunker.h"
#endif


//=============================================================================
// BXSRuleEntry::msSaveVersion
//=============================================================================
const DWORD BXSRuleEntry::msSaveVersion=1;
//=============================================================================
// BXSRuleEntry::msLoadVersion
//=============================================================================
DWORD BXSRuleEntry::msLoadVersion=0xFFFF;

//==============================================================================
// BXSRuleEntry::BXSRuleEntry
//==============================================================================
BXSRuleEntry::BXSRuleEntry(void) :
   mID(-1),
   mFunctionID(-1),
   mPriority(50),
   mMinInterval((DWORD)cMinimumMinInterval),
   mMaxInterval((DWORD)cMinimumMaxInterval),
   mLastExecuteTime((DWORD)0),
   mActive(true),
   mRunImmediately(false)
   //mGroups doesn't need any ctor args.
   {
   }

//==============================================================================
// BXSRuleEntry::~BXSRuleEntry
//==============================================================================
BXSRuleEntry::~BXSRuleEntry(void)
{
}

//==============================================================================
// BXSRuleEntry::setPriority
//==============================================================================
void BXSRuleEntry::setPriority(long v)
{
   mPriority=v;
   if (mPriority > cMaximumPriority)
      mPriority=cMaximumPriority;
   else if (mPriority < cMinimumPriority)
      mPriority=cMinimumPriority;
}

//==============================================================================
// BXSRuleEntry::setMinInterval
//==============================================================================
void BXSRuleEntry::setMinInterval(DWORD v)
{
   mMinInterval=v;
   if (mMinInterval < cMinimumMinInterval)
      mMinInterval=cMinimumMinInterval;
}

//==============================================================================
// BXSRuleEntry::setMaxInterval
//==============================================================================
void BXSRuleEntry::setMaxInterval(DWORD v)
{
   mMaxInterval=v;
   if (mMaxInterval < cMinimumMaxInterval)
      mMaxInterval=cMinimumMaxInterval;
}

//==============================================================================
// BXSRuleEntry::addGroup
//==============================================================================
bool BXSRuleEntry::addGroup(long groupID)
{
   for (long i=0; i < mGroups.getNumber(); i++)
   {
      if (mGroups[i] == groupID)
         return(true);
   }
   if (mGroups.add(groupID) == -1)
      return(false);
   return(true);
}

//==============================================================================
// BXSRuleEntry::removeGroup
//==============================================================================
bool BXSRuleEntry::removeGroup(long groupID)
{
   return(mGroups.remove(groupID));
}

//==============================================================================
// BXSRuleEntry::isMember
//==============================================================================
bool BXSRuleEntry::isMember(long groupID)
{
   for (long i=0; i < mGroups.getNumber(); i++)
   {
      if (mGroups[i] == groupID)
         return(true);
   }
   return(false);
}

#ifdef _BANG
//=============================================================================
// BXSRuleEntry::writeVersion
//=============================================================================
bool BXSRuleEntry::writeVersion(BChunkWriter *chunkWriter)
{
   if(!chunkWriter)
   {
      BASSERT(0);
      {setBlogError(4358); blogerror("BXSRuleEntry::writeVersion -- bad chunkWriter");}
      return(false);
   }
   long result=chunkWriter->writeTaggedDWORD(BCHUNKTAG("xu"), msSaveVersion);
   if(!result)
   {
      {setBlogError(4359); blogerror("BXSRuleEntry::writeVersion -- failed to READ version");}
      return(false);
   }
   return(true);
}

//=============================================================================
// BXSRuleEntry::readVersion
//=============================================================================
bool BXSRuleEntry::readVersion(BChunkReader *chunkReader)
{
   if(!chunkReader)
   {
      BASSERT(0);
      {setBlogError(4360); blogerror("BXSRuleEntry::readVersion -- bad chunkReader");}
      return(false);
   }
   long result=chunkReader->readTaggedDWORD(BCHUNKTAG("xu"), &msLoadVersion);
   if(!result)
   {
      {setBlogError(4361); blogerror("BXSRuleEntry::readVersion -- failed to read version");}
      return(false);
   }
   return(true);
}

//=============================================================================
// BXSRuleEntry::save
//=============================================================================
bool BXSRuleEntry::save(BChunkWriter *chunkWriter)
{
   //Check writer validity.
   if (chunkWriter == NULL)
   {
      BASSERT(0);
      return(false);
   }

   //Write tag.
   long mainHandle;
   long result=chunkWriter->writeTagPostSized(BCHUNKTAG("xv"), mainHandle);
   if(!result)
   {
      {setBlogError(4362); blogerror("BXSRuleEntry::save -- error writing tag.");}
      return(false);
   }

   //ID.
   CHUNKWRITESAFE(chunkWriter, Long, mID);
   //Function ID.
   CHUNKWRITESAFE(chunkWriter, Long, mFunctionID);
   //Priority.
   CHUNKWRITESAFE(chunkWriter, Long, mPriority);
   //Min interval.
   CHUNKWRITESAFE(chunkWriter, DWORD, mMinInterval);
   //Max interval.
   CHUNKWRITESAFE(chunkWriter, DWORD, mMaxInterval);
   //Last execute time.
   CHUNKWRITESAFE(chunkWriter, DWORD, mLastExecuteTime);
   //Active.
   CHUNKWRITESAFE(chunkWriter, Bool, mActive);
   //Run Immediately.
   CHUNKWRITESAFE(chunkWriter, Bool, mRunImmediately);
   //Groups.
   CHUNKWRITESAFE(chunkWriter, Long, mGroups.getNumber());
   CHUNKWRITEARRAYSAFE(chunkWriter, Long, mGroups.getNumber(), (long*)mGroups.getPtr());

   //Finish chunk.   
   result=chunkWriter->writeSize(mainHandle);
   if(!result)
   {
      {setBlogError(4363); blogerror("BXSRuleEntry::save -- failed to write chunk size!");}
      return(false);
   }
   return(true);
}

//=============================================================================
// BXSRuleEntry::load
//=============================================================================
bool BXSRuleEntry::load(BChunkReader *chunkReader)
{
   //Check reader validity.
   if (chunkReader == NULL)
   {
      BASSERT(0);
      return(false);
   }
   //Read tag.
   long result=chunkReader->readExpectedTag(BCHUNKTAG("xv"));
   if(!result)
   {
      {setBlogError(4364); blogerror("BXSRuleEntry::load -- error reading tag.");}
      return(false);
   }

   //ID.
   CHUNKREADSAFE(chunkReader, Long, mID);
   //Function ID.
   CHUNKREADSAFE(chunkReader, Long, mFunctionID);
   //Priority.
   CHUNKREADSAFE(chunkReader, Long, mPriority);
   //Min interval.
   CHUNKREADSAFE(chunkReader, DWORD, mMinInterval);
   //Max interval.
   CHUNKREADSAFE(chunkReader, DWORD, mMaxInterval);
   //Last execute time.
   CHUNKREADSAFE(chunkReader, DWORD, mLastExecuteTime);
   //Active.
   CHUNKREADSAFE(chunkReader, Bool, mActive);
   //Run Immediately.
   if (msLoadVersion >= 1)
      CHUNKREADSAFE(chunkReader, Bool, mRunImmediately);
   else
      mRunImmediately=false;
   //Groups.
   long numberGroups=0;
   CHUNKREADSAFE(chunkReader, Long, numberGroups);
   if (mGroups.setNumber(numberGroups) == false)
   {
      {setBlogError(4365); blogerror("BXSRuleEntry::load -- failed to allocate %d mGroups.", numberGroups);}
      return(false);
   }
   long readCount=0;
   CHUNKREADARRAYSAFE(chunkReader, Long, &readCount, (long*)mGroups.getPtr(), numberGroups);

   //Validate our reading of the chunk.
   result=chunkReader->validateChunkRead(BCHUNKTAG("xv"));
   if(!result)
   {
      {setBlogError(4366); blogerror("BXSRuleEntry::load -- did not read chunk properly!");}
      return(false);
   }
   return(true);
}
#endif

//=============================================================================
// BXSRuleEntry::copy
//=============================================================================
bool BXSRuleEntry::copy(BXSRuleEntry *v)
{
   if (v == NULL)
      return(false);

   mID=v->mID;
   mFunctionID=v->mFunctionID;
   mPriority=v->mPriority;
   mMinInterval=v->mMinInterval;
   mMaxInterval=v->mMaxInterval;
   mLastExecuteTime=v->mLastExecuteTime;
   mActive=v->mActive;
   mRunImmediately=v->mRunImmediately;
   if (mGroups.setNumber(v->mGroups.getNumber()) == false)
      return(false);
   memcpy(mGroups.getPtr(), v->mGroups.getPtr(), mGroups.getNumber()*sizeof(long));

   return(true);
}




//=============================================================================
// BXSRuleGroupEntry::msSaveVersion
//=============================================================================
const DWORD BXSRuleGroupEntry::msSaveVersion=0;
//=============================================================================
// BXSRuleGroupEntry::msLoadVersion
//=============================================================================
DWORD BXSRuleGroupEntry::msLoadVersion=0xFFFF;

//==============================================================================
// BXSRuleGroupEntry::BXSRuleGroupEntry
//==============================================================================
BXSRuleGroupEntry::BXSRuleGroupEntry(void) :
   mID(-1),
   mSymbolID(-1),
   mActive(true)
   //mRules doesn't need any ctor args.
   {
   }

//==============================================================================
// BXSRuleGroupEntry::~BXSRuleGroupEntry
//==============================================================================
BXSRuleGroupEntry::~BXSRuleGroupEntry(void)
{
}

//==============================================================================
// BXSRuleGroupEntry::addRule
//==============================================================================
bool BXSRuleGroupEntry::addRule(long ruleID)
{
   for (long i=0; i < mRules.getNumber(); i++)
   {
      if (mRules[i] == ruleID)
         return(true);
   }
   if (mRules.add(ruleID) == -1)
      return(false);
   return(true);
}

//==============================================================================
// BXSRuleGroupEntry::removeRule
//==============================================================================
bool BXSRuleGroupEntry::removeRule(long ruleID)
{
   return(mRules.remove(ruleID));
}

//==============================================================================
// BXSRuleGroupEntry::isMember
//==============================================================================
bool BXSRuleGroupEntry::isMember(long ruleID)
{
   for (long i=0; i < mRules.getNumber(); i++)
   {
      if (mRules[i] == ruleID)
         return(true);
   }
   return(false);
}

#ifdef _BANG
//=============================================================================
// BXSRuleGroupEntry::writeVersion
//=============================================================================
bool BXSRuleGroupEntry::writeVersion(BChunkWriter *chunkWriter)
{
   if(!chunkWriter)
   {
      BASSERT(0);
      {setBlogError(4367); blogerror("BXSRuleGroupEntry::writeVersion -- bad chunkWriter");}
      return(false);
   }
   long result=chunkWriter->writeTaggedDWORD(BCHUNKTAG("xw"), msSaveVersion);
   if(!result)
   {
      {setBlogError(4368); blogerror("BXSRuleGroupEntry::writeVersion -- failed to READ version");}
      return(false);
   }
   return(true);
}

//=============================================================================
// BXSRuleGroupEntry::readVersion
//=============================================================================
bool BXSRuleGroupEntry::readVersion(BChunkReader *chunkReader)
{
   if(!chunkReader)
   {
      BASSERT(0);
      {setBlogError(4369); blogerror("BXSRuleGroupEntry::readVersion -- bad chunkReader");}
      return(false);
   }
   long result=chunkReader->readTaggedDWORD(BCHUNKTAG("xw"), &msLoadVersion);
   if(!result)
   {
      {setBlogError(4370); blogerror("BXSRuleGroupEntry::readVersion -- failed to read version");}
      return(false);
   }
   return(true);
}

//=============================================================================
// BXSRuleGroupEntry::save
//=============================================================================
bool BXSRuleGroupEntry::save(BChunkWriter *chunkWriter)
{
   //Check writer validity.
   if (chunkWriter == NULL)
   {
      BASSERT(0);
      return(false);
   }

   //Write tag.
   long mainHandle;
   long result=chunkWriter->writeTagPostSized(BCHUNKTAG("xx"), mainHandle);
   if(!result)
   {
      {setBlogError(4371); blogerror("BXSRuleGroupEntry::save -- error writing tag.");}
      return(false);
   }

   //ID.
   CHUNKWRITESAFE(chunkWriter, Long, mID);
   //Symbol ID.
   CHUNKWRITESAFE(chunkWriter, Long, mSymbolID);
   //Active.
   CHUNKWRITESAFE(chunkWriter, Bool, mActive);
   //Rules.
   CHUNKWRITESAFE(chunkWriter, Long, mRules.getNumber());
   CHUNKWRITEARRAYSAFE(chunkWriter, Long, mRules.getNumber(), (long*)mRules.getPtr());

   //Finish chunk.   
   result=chunkWriter->writeSize(mainHandle);
   if(!result)
   {
      {setBlogError(4372); blogerror("BXSRuleGroupEntry::save -- failed to write chunk size!");}
      return(false);
   }
   return(true);
}

//=============================================================================
// BXSRuleGroupEntry::load
//=============================================================================
bool BXSRuleGroupEntry::load(BChunkReader *chunkReader)
{
   //Check reader validity.
   if (chunkReader == NULL)
   {
      BASSERT(0);
      return(false);
   }
   //Read tag.
   long result=chunkReader->readExpectedTag(BCHUNKTAG("xx"));
   if(!result)
   {
      {setBlogError(4373); blogerror("BXSRuleGroupEntry::load -- error reading tag.");}
      return(false);
   }

   //ID.
   CHUNKREADSAFE(chunkReader, Long, mID);
   //Symbol ID.
   CHUNKREADSAFE(chunkReader, Long, mSymbolID);
   //Active.
   CHUNKREADSAFE(chunkReader, Bool, mActive);
   //Rules.
   long numberRules=0;
   CHUNKREADSAFE(chunkReader, Long, numberRules);
   if (mRules.setNumber(numberRules) == false)
   {
      {setBlogError(4374); blogerror("BXSRuleGroupEntry::load -- failed to allocate %d mRules.", numberRules);}
      return(false);
   }
   long readCount=0;
   CHUNKREADARRAYSAFE(chunkReader, Long, &readCount, (long*)mRules.getPtr(), numberRules);

   //Validate our reading of the chunk.
   result=chunkReader->validateChunkRead(BCHUNKTAG("xx"));
   if(!result)
   {
      {setBlogError(4375); blogerror("BXSRuleGroupEntry::load -- did not read chunk properly!");}
      return(false);
   }
   return(true);
}
#endif

//=============================================================================
// BXSRuleGroupEntry::copy
//=============================================================================
bool BXSRuleGroupEntry::copy(BXSRuleGroupEntry *v)
{
   if (v == NULL)
      return(false);

   mID=v->mID;
   mSymbolID=v->mSymbolID;
   mActive=v->mActive;
   if (mRules.setNumber(v->mRules.getNumber()) == false)
      return(false);
   memcpy(mRules.getPtr(), v->mRules.getPtr(), mRules.getNumber()*sizeof(long));

   return(true);
}
