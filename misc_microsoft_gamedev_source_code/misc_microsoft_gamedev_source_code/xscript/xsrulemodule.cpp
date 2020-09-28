//==============================================================================
// xrulemodule.cpp
//
// Copyright (c) 2000-2003, Ensemble Studios
//==============================================================================

//==============================================================================
// Includes
#include "xscript.h"
#include "xsrulemodule.h"
#include "xsdefines.h"
#include "xsfunctionentry.h"
#include "xsvariableentry.h"
#ifdef _BANG
#include "chunker.h"
#endif


//==============================================================================
// BXSRuleModule Static stuff
//==============================================================================
const DWORD BXSRuleModule::msSaveVersion=0;

//==============================================================================
// BXSRuleModule::BXSRuleModule
//==============================================================================
BXSRuleModule::BXSRuleModule(void) :
   //mRules doesn't need any ctor args.
   //mSortedRules doesn't need any ctor args.
   //mRuleGroups doesn't need any ctor args.
   mNextSortedRuleIndex(-1)
   {
   }

//==============================================================================
// BXSRuleModule::~BXSRuleModule
//==============================================================================
BXSRuleModule::~BXSRuleModule(void)
{
   cleanUp();
}

//==============================================================================
// BXSRuleModule::cleanUp
//==============================================================================
void BXSRuleModule::cleanUp()
{
   for (long i=0; i < mRules.getNumber(); i++)
      BDELETE(mRules[i]);
   //We don't delete anything in mSortedRules because that just reuses the pointers
   //in mRules.
   for (long i=0; i < mRuleGroups.getNumber(); i++)
      BDELETE(mRuleGroups[i]);

   mRules.clear();
   mSortedRules.clear();
   mRuleGroups.clear();
}

//==============================================================================
// BXSRuleModule::allocateRules
//==============================================================================
bool BXSRuleModule::allocateRules(long number)
{
   if (mRules.setNumber(number) == false)
      return(false);
   return(true);
}

//==============================================================================
// BXSRuleModule::allocateRuleEntry
//==============================================================================
BXSRuleEntry* BXSRuleModule::allocateRuleEntry(void)
{
   //Allocate.
   BXSRuleEntry* newRE=new BXSRuleEntry();
   if (newRE == NULL)
      return(NULL);
   //Store.
   if (mRules.add(newRE) < 0)
      return(NULL);
   //Set the ID.
   newRE->setID(mRules.getNumber()-1);
   return(newRE);
}

//==============================================================================
// BXSRuleModule::getRuleEntry
//==============================================================================
BXSRuleEntry* BXSRuleModule::getRuleEntry(long id) const
{
   if ((id < 0) || (id >= mRules.getNumber()) )
      return(NULL);
   return(mRules[id]);
}

//==============================================================================
// BXSRuleModule::getRuleID
//==============================================================================
long BXSRuleModule::getRuleID(long functionID)
{
   for (long i=0; i < mRules.getNumber(); i++)
   {
      if (mRules[i]->getFunctionID() == functionID)
         return(mRules[i]->getID());
   }
   return(-1);
}

//==============================================================================
// BXSRuleModule::getRuleID
//==============================================================================
long BXSRuleModule::getRuleID(const char *name)
{
   long functionID=getSymbolValue(name, BXSBinary::cFunction);
   if (functionID == -1)
      return(-1);

   for (long i=0; i < mRules.getNumber(); i++)
   {
      if (mRules[i]->getFunctionID() == functionID)
         return(mRules[i]->getID());
   }
   return(-1);
}

//==============================================================================
// BXSRuleModule::setRuleActive
//==============================================================================
bool BXSRuleModule::setRuleActive(long id, bool v, DWORD currentTime)
{
   if ((id < 0) || (id >= mRules.getNumber()) )
      return(false);
   //If we were inactive and we're now going active, reset the last execute time.
   //Else, leave it alone.
   if ((mRules[id]->getActive() == false) && (v == true))
      mRules[id]->setLastExecuteTime(currentTime);
   //Set our active state.
   mRules[id]->setActive(v);

   //Set the activation time for this rule if we're activating it.
   if (v == true)
   {
      //Get the function entry for this rule.
      BXSFunctionEntry *fe=getFunctionEntry(mRules[id]->getFunctionID());
      if (fe != NULL)
      {
         //Get the first non-parm variable entry (which is the activation time).
         BXSVariableEntry *ve=fe->getVariable(fe->getNumberParameters());
         if (ve != NULL)
         {
            long activationTime=(long)currentTime;
            ve->setData((void*)&activationTime);
         }
      }
   }

   return(true);
}

//==============================================================================
// BXSRuleModule::setRuleActive
//==============================================================================
bool BXSRuleModule::setRuleActive(const char *name, bool v, DWORD currentTime)
{
   long ruleID=getRuleID(name);
   return(setRuleActive(ruleID, v, currentTime));
}

//==============================================================================
// BXSRuleModule::isRuleActive
//==============================================================================
bool BXSRuleModule::isRuleActive(long id)
{
   if ((id < 0) || (id >= mRules.getNumber()) )
      return(false);
   return(mRules[id]->getActive());
}

//==============================================================================
// BXSRuleModule::isRuleActive
//==============================================================================
bool BXSRuleModule::isRuleActive(const char *name)
{
   long ruleID=getRuleID(name);
   if ((ruleID < 0) || (ruleID >= mRules.getNumber()) )
      return(false);
   return(mRules[ruleID]->getActive());
}

//==============================================================================
// BXSRuleModule::modifyRulePriority
//==============================================================================
bool BXSRuleModule::modifyRulePriority(long ruleID, long newPriority)
{
   if ((ruleID < 0) || (ruleID >= mRules.getNumber()) )
      return(false);
   //Set the new priority.
   mRules[ruleID]->setPriority(newPriority);
   //We must resort now.
   sortRules();
   return(true);
}

//==============================================================================
// BXSRuleModule::modifyRuleMinInterval
//==============================================================================
bool BXSRuleModule::modifyRuleMinInterval(long ruleID, DWORD newMinInterval) const
{
   if ((ruleID < 0) || (ruleID >= mRules.getNumber()) )
      return(false);
   //Set the new min interval.
   mRules[ruleID]->setMinInterval(newMinInterval);
   return(true);
}

//==============================================================================
// BXSRuleModule::modifyRuleMaxInterval
//==============================================================================
bool BXSRuleModule::modifyRuleMaxInterval(long ruleID, DWORD newMaxInterval) const
{
   if ((ruleID < 0) || (ruleID >= mRules.getNumber()) )
      return(false);
   //Set the new max interval.
   mRules[ruleID]->setMaxInterval(newMaxInterval);
   return(true);
}

//==============================================================================
// BXSRuleModule::getSortedRuleID
//==============================================================================
long BXSRuleModule::getSortedRuleID(long index)
{
   if ((index < 0) || (index >= mSortedRules.getNumber()) )
      return(false);
   return(mSortedRules[index]->getID());
}

//==============================================================================
// BXSRuleModule::sortRules
//==============================================================================
bool BXSRuleModule::sortRules(void)
{
   //Setup the used tracker.  Slow.
   static BDynamicSimBYTEArray used;
   used.setNumber(mRules.getNumber());
   for (long a=0; a < used.getNumber(); a++)
      used[a]=0;

   //Reset our sorted rule count.
   mSortedRules.setNumber(0);

   //Slow.
   while (mSortedRules.getNumber() != mRules.getNumber())
   {
      long bestIndex=-1;
      long bestPriority=BXSRuleEntry::cMinimumPriority-1;
      for (long i=0; i < mRules.getNumber(); i++)
      {
         //Skip any 'used' rules.
         if (used[i] != 0)
            continue;
         if (mRules[i]->getPriority() > bestPriority)
         {
            bestIndex=i;
            bestPriority=mRules[i]->getPriority();
         }
      }

      //Sanity check.
      if (bestIndex < 0)
      {
         BASSERT(0);
         mSortedRules.setNumber(0);
         return(false);
      }

      //Add the rule to the sorted list.
      if (mSortedRules.add(mRules[bestIndex]) == -1)
      {
         BASSERT(0);
         mSortedRules.setNumber(0);
         return(false);
      }
      //Set the used indicator.
      used[bestIndex]=1;

      //Reset trackers.
      bestIndex=-1;
      bestPriority=BXSRuleEntry::cMinimumPriority-1;
   }

   mNextSortedRuleIndex=-1;

   return(true);
}

//==============================================================================
// BXSRuleModule::allocateRuleGroups
//==============================================================================
bool BXSRuleModule::allocateRuleGroups(long number)
{
   if (mRuleGroups.setNumber(number) == false)
      return(false);
   return(true);
}

//==============================================================================
// BXSRuleModule::allocateRuleGroupEntry
//==============================================================================
BXSRuleGroupEntry* BXSRuleModule::allocateRuleGroupEntry(void)
{
   //Allocate.
   BXSRuleGroupEntry* newRGE=new BXSRuleGroupEntry();
   if (newRGE == NULL)
      return(NULL);
   //Store.
   if (mRuleGroups.add(newRGE) < 0)
      return(NULL);
   //Set the ID.
   newRGE->setID(mRuleGroups.getNumber()-1);
   return(newRGE);
}

//==============================================================================
// BXSRuleModule::getRuleGroupEntry
//==============================================================================
BXSRuleGroupEntry* BXSRuleModule::getRuleGroupEntry(long id) const
{
   if ((id < 0) || (id >= mRuleGroups.getNumber()) )
      return(NULL);
   return(mRuleGroups[id]);
}

//==============================================================================
// BXSRuleModule::setRuleGroupActive
//==============================================================================
bool BXSRuleModule::setRuleGroupActive(long groupID, bool v, DWORD currentTime)
{
   if ((groupID < 0) || (groupID >= mRuleGroups.getNumber()) )
      return(false);

   //Set the active flag.
   mRuleGroups[groupID]->setActive(v);

   //Now, rip through and set the active on each rule that's part of this group.
   long *ruleIDs=mRuleGroups[groupID]->getRules();
   for (long i=0; i < mRuleGroups[groupID]->getNumberRules(); i++)
      setRuleActive(ruleIDs[i], v, currentTime);
   return(true);
}

//==============================================================================
// BXSRuleModule::setRuleGroupActive
//==============================================================================
bool BXSRuleModule::setRuleGroupActive(const char *name, bool v, DWORD currentTime)
{
   //Get the group ID for the name.  This will be the symbol's value.
   long groupID=getSymbolValue(name, BXSBinary::cRuleGroup);
   return(setRuleGroupActive(groupID, v, currentTime));
}

//==============================================================================
// BXSRuleModule::isRuleGroupActive
//==============================================================================
bool BXSRuleModule::isRuleGroupActive(long groupID)
{
   if ((groupID < 0) || (groupID >= mRuleGroups.getNumber()) )
      return(false);
   return(mRuleGroups[groupID]->getActive());
}

//==============================================================================
// BXSRuleModule::isRuleGroupActive
//==============================================================================
bool BXSRuleModule::isRuleGroupActive(const char *name)
{
   //Get the group ID for the name.  This will be the symbol's value.
   long groupID=getSymbolValue(name, BXSBinary::cRuleGroup);
   if ((groupID < 0) || (groupID >= mRuleGroups.getNumber()) )
      return(false);
   return(mRuleGroups[groupID]->getActive());
}

//==============================================================================
// BXSRuleModule::resetNextSortedRuleIndex
//==============================================================================
void BXSRuleModule::resetNextSortedRuleIndex(void)
{
   mNextSortedRuleIndex=-1;
   incrementNextSortedRuleIndex();
}

//==============================================================================
// BXSRuleModule::getNextSortedRuleID
//==============================================================================
long BXSRuleModule::getNextSortedRuleID(void) const
{
   if ((mNextSortedRuleIndex < 0) || (mNextSortedRuleIndex >= mSortedRules.getNumber()) )
      return(-1);
   return(mSortedRules[mNextSortedRuleIndex]->getID());
}

//==============================================================================
// BXSRuleModule::incrementNextSortedRuleIndex
//==============================================================================
long BXSRuleModule::incrementNextSortedRuleIndex(void)
{
   //If we don't have any sorted rules, it's -1.
   if (mSortedRules.getNumber() <= 0)
   {
      mNextSortedRuleIndex=-1;
      return(mNextSortedRuleIndex);
   }

   //If we have an invalid ID, set it to 0.  If that rule is active, just return.
   if ((mNextSortedRuleIndex < 0) || (mNextSortedRuleIndex >= mSortedRules.getNumber()) )
   {
      mNextSortedRuleIndex=0;
      if (mSortedRules[mNextSortedRuleIndex]->getActive() == true)
         return(mNextSortedRuleIndex);
   }

   //Find the next active rule in the sorted rule list.
   for (long i=mNextSortedRuleIndex+1; i < mSortedRules.getNumber(); i++)
   {
      if (mSortedRules[i]->getActive() == true)
      {
         mNextSortedRuleIndex=i;
         return(mNextSortedRuleIndex);
      }
   }

   //Else, set it to invalid.
   mNextSortedRuleIndex=-1;
   return(mNextSortedRuleIndex);
}

//=============================================================================
// BXSRuleModule::copyData
//=============================================================================
bool BXSRuleModule::copyData(BXSRuleModule *rm)
{
    if (rm == NULL)
      return(false);

   cleanUp();

   //Rules.
   if (mRules.setNumber(rm->mRules.getNumber()) == false)
      return(false);
   for (long i=0; i < mRules.getNumber(); i++)
   {
      mRules[i]=new BXSRuleEntry;
      if (mRules[i] == NULL)
         return(false);
      mRules[i]->copy(rm->mRules[i]);
   }

   //Sorted Rules.  Since this is just a pointer indexing list into the mRules
   //list, we can't just copy the values in *rm (as that's the wrong thing to
   //point at).  So, we'll just resort off of our mRules.
   if (sortRules() == false)
      return(false);

   //Rule Groups.
   if (mRuleGroups.setNumber(rm->mRuleGroups.getNumber()) == false)
      return(false);
   for (long i=0; i < mRuleGroups.getNumber(); i++)
   {
      mRuleGroups[i]=new BXSRuleGroupEntry;
      if (mRuleGroups[i] == NULL)
         return(false);
      mRuleGroups[i]->copy(rm->mRuleGroups[i]);
   }

   //Done.   
   return(true);
}

#ifdef _BANG
//=============================================================================
// BXSRuleModule::save
//=============================================================================
bool BXSRuleModule::save(BChunkWriter *chunkWriter)
{
   //Check writer validity.
   if (chunkWriter == NULL)
   {
      BASSERT(0);
      return(false);
   }

   //Write tag.
   long mainHandle;
   long result=chunkWriter->writeTagPostSized(BCHUNKTAG("xt"), mainHandle);
   if (result == false)
   {
      {setBlogError(4376); blogerror("BXSRuleModule::save -- error writing tag.");}
      return(false);
   }

   //Version.
   CHUNKWRITESAFE(chunkWriter, DWORD, msSaveVersion);
   //Static save versions for things that this class allocates.
   if (BXSRuleEntry::writeVersion(chunkWriter) == false)
   {
      {setBlogError(4377); blogerror("BXSRuleModule::save -- failed to save BXSRuleEntry version.");}
      return(false);
   }
   if (BXSRuleGroupEntry::writeVersion(chunkWriter) == false)
   {
      {setBlogError(4378); blogerror("BXSRuleModule::save -- failed to save BXSRuleGroupEntry version.");}
      return(false);
   }

   //Rules.
   CHUNKWRITESAFE(chunkWriter, Long, mRules.getNumber());
   for (long i=0; i < mRules.getNumber(); i++)
   {
      if (mRules[i]->save(chunkWriter) == false)
      {
         {setBlogError(4379); blogerror("BXSRuleModule::save -- failed to save mRules[%d].", i);}
         return(false);
      }
   }
   //Sorted rules.
   CHUNKWRITESAFE(chunkWriter, Long, mSortedRules.getNumber());
   for (long i=0; i < mSortedRules.getNumber(); i++)
      CHUNKWRITESAFE(chunkWriter, Long, mSortedRules[i]->getID());
   //Functions.
   CHUNKWRITESAFE(chunkWriter, Long, mRuleGroups.getNumber());
   for (long i=0; i < mRuleGroups.getNumber(); i++)
   {
      if (mRuleGroups[i]->save(chunkWriter) == false)
      {
         {setBlogError(4380); blogerror("BXSRuleModule::save -- failed to save mRuleGroups[%d].", i);}
         return(false);
      }
   }
   //Next sorted rule index..
   CHUNKWRITESAFE(chunkWriter, Long, mNextSortedRuleIndex);

   //Finish chunk.   
   result=chunkWriter->writeSize(mainHandle);
   if (result == false)
   {
      {setBlogError(4381); blogerror("BXSRuleModule::save -- failed to write chunk size!");}
      return(false);
   }

   return(true);
}

//=============================================================================
// BXSRuleModule::load
//=============================================================================
bool BXSRuleModule::load(BChunkReader *chunkReader)
{
   //Check reader validity.
   if (chunkReader == NULL)
   {
      BASSERT(0);
      return(false);
   }

   //Read tag.
   long result=chunkReader->readExpectedTag(BCHUNKTAG("xt"));
   if (result == false)
   {
      {setBlogError(4382); blogerror("BXSRuleModule::load -- error reading tag.");}
      return(false);
   }

   //Version.
   DWORD version=(DWORD)0;
   CHUNKREADSAFE(chunkReader, DWORD, version);
   //Don't load old format versions.
   //if (version < X)
   //{
   //   blog("BXSRuntime::load -- old format is incompatible, sorry.");
   //   return(false);
   //}
   //Static load versions for things that this class allocates.
   if (BXSRuleEntry::readVersion(chunkReader) == false)
   {
      {setBlogError(4383); blogerror("BXSRuleModule::load -- failed to load BXSRuleEntry version.");}
      return(false);
   }
   if (BXSRuleGroupEntry::readVersion(chunkReader) == false)
   {
      {setBlogError(4384); blogerror("BXSRuleModule::load -- failed to load BXSRuleGroupEntry version.");}
      return(false);
   }

   //Rules.
   long numberRules=0;
   CHUNKREADSAFE(chunkReader, Long, numberRules);
   if (mRules.setNumber(numberRules) == false)
   {
      {setBlogError(4385); blogerror("BXSRuleModule::load -- failed to allocate %d mRules.", numberRules);}
      return(false);
   }
   for (long i=0; i < mRules.getNumber(); i++)
   {
      mRules[i]=new BXSRuleEntry;
      if (mRules[i] == NULL)
      {
         {setBlogError(4386); blogerror("BXSRuleModule::load -- failed to allocate mRules[%d].", i);}
         return(false);
      }
      if (mRules[i]->load(chunkReader) == false)
      {
         {setBlogError(4387); blogerror("BXSRuleModule::load -- failed to load mRules[%d].", i);}
         return(false);
      }
   }
   //Sorted rules.
   long numberSortedRules=0;
   CHUNKREADSAFE(chunkReader, Long, numberSortedRules);
   if (mSortedRules.setNumber(numberSortedRules) == false)
   {
      {setBlogError(4388); blogerror("BXSRuleModule::load -- failed to allocate %d mSortedRules.", numberSortedRules);}
      return(false);
   }
   for (long i=0; i < mSortedRules.getNumber(); i++)
   {
      long ruleID=-1;
      CHUNKREADSAFE(chunkReader, Long, ruleID);
      mSortedRules[i]=getRuleEntry(ruleID);
   }
   //Rule groups.
   long numberRuleGroups=0;
   CHUNKREADSAFE(chunkReader, Long, numberRuleGroups);
   if (mRuleGroups.setNumber(numberRuleGroups) == false)
   {
      {setBlogError(4389); blogerror("BXSRuleModule::load -- failed to allocate %d mRuleGroups.", numberRuleGroups);}
      return(false);
   }
   for (long i=0; i < mRuleGroups.getNumber(); i++)
   {
      mRuleGroups[i]=new BXSRuleGroupEntry;
      if (mRuleGroups[i] == NULL)
      {
         {setBlogError(4390); blogerror("BXSRuleModule::load -- failed to allocate mRuleGroups[%d].", i);}
         return(false);
      }
      if (mRuleGroups[i]->load(chunkReader) == false)
      {
         {setBlogError(4391); blogerror("BXSRuleModule::load -- failed to load mRuleGroups[%d].", i);}
         return(false);
      }
   }
   //Next sorted rule index.
   CHUNKREADSAFE(chunkReader, Long, mNextSortedRuleIndex);

   //Validate our reading of the chunk.
   result=chunkReader->validateChunkRead(BCHUNKTAG("xt"));
   if (result == false)
   {
      {setBlogError(4392); blogerror("BXSRuleModule::load -- did not read chunk properly!");}
      return(false);
   }

   return(true);
}
#endif
