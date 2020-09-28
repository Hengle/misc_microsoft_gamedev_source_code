//==============================================================================
// xsruntime.cpp
//
// Copyright (c) 2000-2003, Ensemble Studios
//==============================================================================

//==============================================================================
// Includes
#include "xscript.h"
#include "xsruntime.h"
#ifdef _BANG
#include "chunker.h"
#endif
#include "xscompiler.h"
#include "xsdata.h"
#include "xsdefines.h"
#include "xsinterpreter.h"
#include "xsmessenger.h"
#include "xssource.h"
#include "config.h"
#include "econfigenum.h"

//=============================================================================
// Defines.


//==============================================================================
// BXSRuntime Static stuff
//==============================================================================
long BXSRuntime::mXSContextPlayerID=-1;
long BXSRuntime::mXSContextRuntimeID=-1;
long BXSRuntime::mNextRuntimeID=0;
BDynamicSimArray<BXSRuntime*> BXSRuntime::mXSRuntimes;
const DWORD BXSRuntime::msSaveVersion=3;

//==============================================================================
// BXSRuntime::BXSRuntime
//==============================================================================
BXSRuntime::BXSRuntime(const BSimString &name, long playerID, long instanceLimit) :
   mID(-1),
   mPlayerID(playerID),
   mName(name),
   mInstanceLimit(instanceLimit),
   mMessenger(NULL),
   mCompiler(NULL),
   mInterpreter(NULL),
   mSyscalls(NULL),
   mSource(NULL),
   //mDatas doesn't need any ctor args.
   mCaseSensitive(true),
   //mConfigFunctions doesn't need any ctor args.
   mWarningsOn(true),
   mInfoMessagesOn(true),
   mRunMessagesOn(true),
   mListInterpreter(false),
   mListFunctionEntry(false),
   mGenerateListing(true),
   mDebugTokenizer(false),
   mDebugCode(true),
   mErrorFunction(defaultOutputMessage),
   mWarningFunction(defaultOutputMessage),
   mInfoMessageFunction(defaultOutputMessage),
   mRunMessageFunction(defaultOutputMessage),
   mDebuggerMessageFunction(defaultOutputMessage),
   mBaseDirectoryID(-1),
   mBaseUWDirectoryID(-1)
{
   //Create the ID.
   mID=mNextRuntimeID;
   mNextRuntimeID++;
   //Add this to the XS runtimes list.
   mXSRuntimes.add(this);

   //Fixup instance limit if someone passed in an unacceptable value.
   BASSERT(mInstanceLimit >= 1);
   if (mInstanceLimit < 1)
      mInstanceLimit=1;
}

//==============================================================================
// BXSRuntime::~BXSRuntime
//==============================================================================
BXSRuntime::~BXSRuntime(void)
{
   cleanUpAll();
}

//==============================================================================
// BXSRuntime::initialize
//==============================================================================
bool BXSRuntime::initialize(bool defineMathVariables, long baseDirID, long baseUWDirID)
{
   //Clean out any old stuff.
   cleanUpData();

   //Save our directories.
   mBaseDirectoryID=baseDirID;
   mBaseUWDirectoryID=baseUWDirID;

   //Context Runtime.
   setXSContextRuntimeID(mID);

   //Create a messenger.
   mMessenger=new BXSMessenger;
   if (mMessenger == NULL)
      return(false);
   mMessenger->setErrorMsgFun(mErrorFunction);
   mMessenger->setWarn(mWarningsOn);
   mMessenger->setWarnMsgFun(mWarningFunction);
   mMessenger->setInfo(mInfoMessagesOn);
   mMessenger->setInfoMsgFun(mInfoMessageFunction);
   mMessenger->setRun(mRunMessagesOn);
   mMessenger->setRunMsgFun(mRunMessageFunction);
   mMessenger->setListInterpreter(mListInterpreter);
   mMessenger->setListFunctionEntry(mListFunctionEntry);

   //Allocate the source.
   mSource=new BXSSource(mCaseSensitive);
   if (mSource == NULL)
      return(false);

   //Allocate the data for compile.  This will be the 0-th data.
   if (mDatas.setNumber(1) == false)
      return(false);
   mDatas[0]=new BXSData(0, mName, mMessenger, mSource);
   if (mDatas[0] == NULL)
      return(false);

   //Init the syscall module.
   mSyscalls=new BXSSyscallModule(mMessenger, mSource->getSymbolTable(), mSource->getCaseSensitive(), true, true);
   if (mSyscalls == NULL)
      return(false);
   //Add the syscalls.
   for (long i=0; i < mConfigFunctions.getNumber(); i++)
   {
      BXSConfigFunction cf=(BXSConfigFunction)mConfigFunctions[i];
      cf(mSyscalls);
   }

   //Init the compiler.
   mCompiler=new BXSCompiler(mMessenger, mDatas[0], mSource, mSyscalls);
   if (mCompiler == NULL)
      return(false);
   if (mCompiler->initialize(mBaseDirectoryID, mBaseUWDirectoryID) == false)
      return(false);
   mCompiler->setListing(mGenerateListing);
   mCompiler->setDebugTokenizer(mDebugTokenizer);
   mCompiler->setDebugCode(mDebugCode);

   //Init the interp.
   mInterpreter=new BXSInterpreter(mMessenger, mSyscalls);
   if (mInterpreter == NULL)
      return(false);
   if (mInterpreter->initialize() == false)
      return(false);

   //Define the math variables if we have them "on".
   if (defineMathVariables == true)
      addCommonMathVariables();

   //Context Runtime.
   setXSContextRuntimeID(-1);

   return(true);
}

//==============================================================================
// BXSRuntime::addConfigFunction
//==============================================================================
bool BXSRuntime::addConfigFunction(BXSConfigFunction cf, bool apply)
{
   if (cf == NULL)
      return(false);

   //Context Runtime.
   setXSContextRuntimeID(mID);

   //Get the actual address.
   long address=(long)cf;
   //Add it.
   if (mConfigFunctions.add(address) == -1)
      return(false);
   //If we're supposed to apply it (and we have a compiler and an interp), do it.
   if ((apply == true) && (mSyscalls != NULL))
      cf(mSyscalls);

   //Context Runtime.
   setXSContextRuntimeID(-1);

   return(true);
}

//==============================================================================
// BXSRuntime::clearConfigFunctions
//==============================================================================
void BXSRuntime::clearConfigFunctions(void)
{
   mConfigFunctions.setNumber(0);
}

//==============================================================================
// BXSRuntime::getData
//==============================================================================
BXSData* BXSRuntime::getData(long dataID) const
{
   if ((dataID < 0) || (dataID >= mDatas.getNumber()) )
      return(NULL);
   return(mDatas[dataID]);
}

//==============================================================================
// BXSRuntime::allocateData
//==============================================================================
long BXSRuntime::allocateData(void)
{
   long newID = -1;

   // First see if there is one that can be reused
   long tempIndex;
   for (tempIndex = 0; tempIndex < mDatas.getNumber(); tempIndex++)
   {
      if ((mDatas[tempIndex] != NULL) && (mDatas[tempIndex]->getInUse() == false))
      {
         mDatas[tempIndex]->setInUse(true);
         newID = tempIndex;
         break;
      }
   }

   // Create a new one if one can't be reused
   if (newID == -1)
   {
      //If we're at or over our instance limit, fail.
      if (mDatas.getNumber() >= mInstanceLimit)
         return(-1);

      //Create a new one.
      newID=mDatas.getNumber();
      if (mDatas.setNumber(newID+1) == false)
         return(-1);
      mDatas[newID]=new BXSData(newID, mName, mMessenger, mSource);
      if (mDatas[newID] == NULL)
         return(-1);
   }

   //Copy over the 0th data.
   if (mDatas[newID]->copyData(mDatas[0]) == false)
   {
      delete mDatas[newID];
      mDatas[newID]=NULL;
      mDatas.setNumber(newID);
      return(-1);
   }

   //Done.
   return(newID);
}

//==============================================================================
// BXSRuntime::deleteData
//==============================================================================
bool BXSRuntime::deleteData( long dataID )
{
   if ((dataID >= 0) && (dataID < mDatas.getNumber()))
   {
      mDatas[dataID]->setInUse(false);
      return true;
   }
   return false;
}

//==============================================================================
// BXSRuntime::isReadyToExecute
//==============================================================================
bool BXSRuntime::isReadyToExecute(long dataID) const
{
   //Must have source.
   if (mSource == NULL)
      return(false);
   //Must have a valid data.
   BXSData *data=getData(dataID);
   if (data == NULL)
      return(false);
   //Data must be ready to execute.
   if (data->getReadyToExecute() == false)
      return(false);
   //If we have more than 1 data, we cannot execute on the 0th data as that's our control instance.
   if ((mInstanceLimit > 1) && (dataID == 0))
      return(false);
   //Must have interpreter.
   if (mInterpreter == NULL)
      return(false);
   return(true);
}

//==============================================================================
// BXSRuntime::getFunctionID
//==============================================================================
long BXSRuntime::getFunctionID(const char *functionName) const
{
   //Bomb checks.
   if ((functionName == NULL) || (isReadyToExecute(0) == false))
      return(-1);
   long functionID=mDatas[0]->getFunctionID(functionName);
   return(functionID);
}

//==============================================================================
// BXSRuntime::getNumberModules
//==============================================================================
long BXSRuntime::getNumberModules(void) const
{
   if (mCompiler == NULL)
      return(0);
   return(mCompiler->getNumberModules());
}

//==============================================================================
// BXSRuntime::getModuleFilename
//==============================================================================
const BSimString& BXSRuntime::getModuleFilename(long index) const
{
   if (mCompiler == NULL)
   {
      static BSimString foo;
      return(foo);
   }
   return(mCompiler->getModuleFilename(index));
}

//==============================================================================
// BXSRuntime::addModuleFilename
//==============================================================================
bool BXSRuntime::addModuleFilename(const BSimString &filename)
{
   if (mCompiler == NULL)
      return(false);
   return(mCompiler->addModuleFilename(filename));
}

//==============================================================================
// BXSRuntime::removeModuleFilename
//==============================================================================
bool BXSRuntime::removeModuleFilename(const BSimString &filename)
{
   if (mCompiler == NULL)
      return(false);
   return(mCompiler->removeModuleFilename(filename));
}

//==============================================================================
// BXSRuntime::compileModules
//==============================================================================
bool BXSRuntime::compileModules(bool deleteCompiler)
{
   //Bomb check.
   if ((mCompiler == NULL) || (mDatas.getNumber() <= 0) || (mDatas[0] == NULL) || (mInterpreter == NULL))
      return(false);

   //Context Runtime.
   setXSContextRuntimeID(mID);

   bool compileSuccess=mCompiler->compileModules(mName);
   //Cleanup.
   if (compileSuccess == false)
   {
      setXSContextRuntimeID(-1);
      return(false);
   }

   //Sort the rules.
   if (mDatas[0]->sortRules() == false)
   {
      BDELETE(mDatas[0]);
      mDatas.setNumber(0);
      setXSContextRuntimeID(-1);
      return(false);
   }
   //Init the heap.
   if (mDatas[0]->initializeHeap() == false)
   {
      BDELETE(mDatas[0]);
      mDatas.setNumber(0);
      setXSContextRuntimeID(-1);
      return(false);
   }

   //Delete the compiler if we're supposed to.
   if (deleteCompiler == true)
      BDELETE(mCompiler);

   //Context Runtime.
   setXSContextRuntimeID(-1);
   //We're ready to execute now.
   mDatas[0]->setReadyToExecute(true);
   return(true);
}

//==============================================================================
// BXSRuntime::compileFile
//==============================================================================
bool BXSRuntime::compileFile(const BSimString &filename, const BSimString &qualifiedFilename,
   const BSimString &listPrefix, bool deleteCompiler)
{
   //Bomb check.
   if ((mCompiler == NULL) || (mDatas.getNumber() <= 0) || (mDatas[0] == NULL) || (mInterpreter == NULL))
      return(false);

   //Open the file, create a buffer for the data, and then stuff the file data
   //into the buffer.  Try the BaseUWDir first, then the BaseDir.
   BFile file;
   if (file.openReadOnly(mBaseUWDirectoryID, qualifiedFilename) == false)
   {
      if (file.openReadOnly(mBaseDirectoryID, qualifiedFilename) == false)
		{
         {setBlogError(4393); blogerror("BXSRuntime::compileFile -- couldn't open xs file.");}
			return(false);
		}
   }
	
	

   //Get the size.
   unsigned long fileSize=0;
   if (file.getSize(fileSize) == false)
	{
		BSimString path;
		file.getPath(path);	
		{setBlogError(4393); blogerror("BXSRuntime::compileFile -- File::getSize failed. %s", path.getPtr());}
		return(false);
	}
   //Allocate a temporary buffer.
   char *buffer=new char[(long)fileSize+1];
   if (buffer == 0)
      return(false);
   //Read it in.
   if (file.read(buffer, fileSize) == false)
   {
      BDELETEARRAY(buffer);
      BSimString path;
      file.getPath(path);	
      {setBlogError(4393); blogerror("BXSRuntime::compileFile -- File::read failed. %s", path.getPtr());}
      return(false);
   }
   buffer[fileSize]='\0';

   //Context Runtime.
   setXSContextRuntimeID(mID);

   //Compile the code.
   bool compileSuccess=mCompiler->compileFile(filename, qualifiedFilename, listPrefix, buffer, (long)fileSize+1);
   //Cleanup.
   BDELETEARRAY(buffer);
   if (compileSuccess == false)
   {
      setXSContextRuntimeID(-1);
		BSimString path;
		file.getPath(path);	
		{setBlogError(4393); blogerror("BXSRuntime::compileFile -- compile failed. %s", path.getPtr());}
		return(false);
   }

   //Sort the rules.
   if (mDatas[0]->sortRules() == false)
   {
      BDELETE(mDatas[0]);
      mDatas.setNumber(0);
      setXSContextRuntimeID(-1);
		BSimString path;
		file.getPath(path);	
		{setBlogError(4393); blogerror("BXSRuntime::compileFile -- sortRules failed. %s", path.getPtr());}
      return(false);
   }
   //Init the heap.
   if (mDatas[0]->initializeHeap() == false)
   {
      BDELETE(mDatas[0]);
      mDatas.setNumber(0);
      setXSContextRuntimeID(-1);
		BSimString path;
		file.getPath(path);	
		{setBlogError(4393); blogerror("BXSRuntime::compileFile -- initialize Heap failed. %s", path.getPtr());}
		return(false);
   }
   //Delete the compiler if we're supposed to.
   if (deleteCompiler == true)
      BDELETE(mCompiler);

   //Context Runtime.
   setXSContextRuntimeID(-1);
   //We're ready to execute now.
   mDatas[0]->setReadyToExecute(true);
   return(true);
}

//==============================================================================
// BXSRuntime::interpretCode
//==============================================================================
bool BXSRuntime::interpretCode(long dataID, const char *callingString)
{
   //Bomb check.
   if (isReadyToExecute(dataID) == false)
      return(false);
   //If the data is in a breakpoint, this fails.
   BXSData *data=getData(dataID);
   if (data->getBreakpoint() == true)
      return(false);

   //Spit a quick listing helper.
   if (mMessenger->getListInterpreter() == true)
   {
      mMessenger->runMsg("");
      mMessenger->runMsg("");
      mMessenger->runMsg("BXSRuntime::interpretCode:");
   }
   //Set the calling string.
   data->setCallingString(callingString);
   //Context Runtime.
   setXSContextRuntimeID(mID);

   //Do the interp.
   if (mInterpreter->interpret(mSource, data) == false)
   {
      data->setReadyToExecute(false);
      setXSContextRuntimeID(-1);
      return(false);
   }

   //Unset calling string if we're not in a breakpoint.
   if (data->getBreakpoint() == false)
      data->setCallingString(NULL);
   //Context Runtime.
   setXSContextRuntimeID(-1);

   return(true);
}

//==============================================================================
// BXSRuntime::interpretFunction
//==============================================================================
bool BXSRuntime::interpretFunction(long dataID, const char *functionName, const char *callingString)
{
   //Bomb check.
   if ((isReadyToExecute(dataID) == false) || (functionName == NULL))
      return(false);
   //If the data is in a breakpoint, this fails.
   BXSData *data=getData(dataID);
   if (data->getBreakpoint() == true)
      return(false);

   //Spit a quick listing helper.
   if (mMessenger->getListInterpreter() == true)
   {
      mMessenger->runMsg("");
      mMessenger->runMsg("");
      mMessenger->runMsg("BXSRuntime::interpretFunction: '%s':", functionName);
   }
   //Set the calling string.
   data->setCallingString(callingString);
   //Context Runtime.
   setXSContextRuntimeID(mID);

   //Do the interp.
   if (mInterpreter->interpretFunction(mSource, data, functionName, -1, false) == false)
   {
      data->setReadyToExecute(false);
      setXSContextRuntimeID(-1);
      return(false);
   }

   //Unset calling string if we're not in a breakpoint.
   if (data->getBreakpoint() == false)
      data->setCallingString(NULL);
   //Context Runtime.
   setXSContextRuntimeID(-1);

   return(true);
}

//==============================================================================
// BXSRuntime::interpretRules
//==============================================================================
bool BXSRuntime::interpretRules(long dataID, DWORD currentTime, DWORD timeLimit, const char *callingString)
{
   //Bomb check.
   if (isReadyToExecute(dataID) == false)
      return(false);
   //If the data is in a breakpoint, this fails.
   BXSData *data=getData(dataID);
   if (data->getBreakpoint() == true)
      return(false);

   //Spit a quick listing helper.
   if (mMessenger->getListInterpreter() == true)
   {
      mMessenger->runMsg("");
      mMessenger->runMsg("");
      mMessenger->runMsg("BXSRuntime::interpretRules: currentTime=%d, timeLimit=%d:", currentTime, timeLimit);
   }
   //Set the calling string.
   data->setCallingString(callingString);
   //Context Runtime.
   setXSContextRuntimeID(mID);

   //Reset the sorted rule index.
   data->resetNextSortedRuleIndex();
   for (;;)
   {
      bool lastRule=false;
      if (mInterpreter->interpretRules(mSource, data, currentTime, timeLimit, lastRule) == false)
      {
         data->setReadyToExecute(false);
         setXSContextRuntimeID(-1);
         return(false);
      }

      //If we're now in a breakpoint, just return true.
      if (data->getBreakpoint() == true)
      {
         setXSContextRuntimeID(-1);
         return(true);
      }

      if (lastRule == true)
      {
         data->resetNextSortedRuleIndex();
         break;
      }
   }

   //Unset calling string if we're not in a breakpoint.
   if (data->getBreakpoint() == false)
      data->setCallingString(NULL);
   //Context Runtime.
   setXSContextRuntimeID(-1);

   return(true);
}

//==============================================================================
// BXSRuntime::resetRuleTime
//==============================================================================
void BXSRuntime::resetRuleTime(long dataID, DWORD currentTime)
{
   //If we have more than 1 data, we cannot execute on the 0th data as that's our control instance.
   if ((mInstanceLimit > 1) && (dataID == 0))
      return;
   BXSData *data=getData(dataID);
   if (data != NULL)
      data->resetRuleTime(currentTime);
}

//==============================================================================
// BXSRuntime::interpretTrigger
//==============================================================================
bool BXSRuntime::interpretTrigger(long dataID, DWORD currentTime, const char *callingString)
{
   //Bomb check.
   if (isReadyToExecute(dataID) == false)
      return(false);
   //If the data is in a breakpoint, this fails.
   BXSData *data=getData(dataID);
   if (data->getBreakpoint() == true)
      return(false);

   //DCP 08/27/02: If we're in the middle of a call (however that happened), finish
   //that and just return.
   if (data->getCallStackSize() > 0)
   {
      if (mInterpreter->interpretRestart(mSource, data, BXSData::cBreakpointGoRun) == false)
      {
         data->setReadyToExecute(false);
         return(false);
      }
      return(true);
   }

   //Spit a quick listing helper.
   if (mMessenger->getListInterpreter() == true)
   {
      mMessenger->runMsg("");
      mMessenger->runMsg("");
      mMessenger->runMsg("BXSRuntime::interpretTrigger: currentTime=%d:", currentTime);
   }
   //Set the calling string.
   data->setCallingString(callingString);
   //Context Runtime.
   setXSContextRuntimeID(mID);

   //Reset the sorted rule index.
   data->resetNextSortedRuleIndex();
   for (;;)
   {
      bool lastRule=false;
      if (mInterpreter->interpretRules(mSource, data, currentTime, (DWORD)0, lastRule) == false)
      {
         data->setReadyToExecute(false);
         setXSContextRuntimeID(-1);
         return(false);
      }

      if (lastRule == true)
      {
         data->resetNextSortedRuleIndex();
         break;
      }
   }

   //Unset calling string if we're not in a breakpoint.
   if (data->getBreakpoint() == false)
      data->setCallingString(NULL);
   //Context Runtime.
   setXSContextRuntimeID(-1);

   return(true);
}

//==============================================================================
// BXSRuntime::interpretHandler
//==============================================================================
bool BXSRuntime::interpretHandler(long dataID, long functionID, long parameter, const char *callingString)
{
   //Bomb check.
   if ((isReadyToExecute(dataID) == false) || (functionID < 0))
      return(false);
   //If the data is in a breakpoint, this fails.
   BXSData *data=getData(dataID);
   if (data->getBreakpoint() == true)
      return(false);

   //Spit a quick listing helper.
   if (mMessenger->getListInterpreter() == true)
   {
      mMessenger->runMsg("");
      mMessenger->runMsg("");
      mMessenger->runMsg("BXSRuntime::interpretHandler: functionID=%d, parameter=%d:", functionID, parameter);
   }
   //Set the calling string.
   data->setCallingString(callingString);
   //Context Runtime.
   setXSContextRuntimeID(mID);

   //Do the interp.  We pass the parameter down and tell it to push it on as
   //that's how XS 'Handlers' work.
   if (mInterpreter->interpretFunction(mSource, data, functionID, parameter, true) == false)
   {
      data->setReadyToExecute(false);
      setXSContextRuntimeID(-1);
      return(false);
   }

   //Unset calling string if we're not in a breakpoint.
   if (data->getBreakpoint() == false)
      data->setCallingString(NULL);
   //Context Runtime.
   setXSContextRuntimeID(-1);

   return(true);
}

//==============================================================================
// BXSRuntime::interpretHandler
//==============================================================================
bool BXSRuntime::interpretHandler(long dataID, const char *functionName, long parameter, const char *callingString)
{
   //Bomb check.
   if ((isReadyToExecute(dataID) == false) || (functionName == NULL))
      return(false);
   //If the data is in a breakpoint, this fails.
   BXSData *data=getData(dataID);
   if (data->getBreakpoint() == true)
      return(false);

   //Spit a quick listing helper.
   if (mMessenger->getListInterpreter() == true)
   {
      mMessenger->runMsg("");
      mMessenger->runMsg("");
      mMessenger->runMsg("BXSRuntime::interpretHandler: function=%s, parameter=%d:", functionName, parameter);
   }
   //Set the calling string.
   data->setCallingString(callingString);
   //Context Runtime.
   setXSContextRuntimeID(mID);

   //Do the interp.  We pass the parameter down and tell it to push it on as
   //that's how XS 'Handlers' work.
   if (mInterpreter->interpretFunction(mSource, data, functionName, parameter, true) == false)
   {
      data->setReadyToExecute(false);
      setXSContextRuntimeID(-1);
      return(false);
   }

   //Unset calling string if we're not in a breakpoint.
   if (data->getBreakpoint() == false)
      data->setCallingString(NULL);
   //Context Runtime.
   setXSContextRuntimeID(-1);

   return(true);
}

//==============================================================================
// BXSRuntime::interpretBreakpoint
//==============================================================================
bool BXSRuntime::interpretBreakpoint(long dataID)
{
   //Bomb check.
   if (isReadyToExecute(dataID) == false)
      return(false);
   //If the data is NOT in a breakpoint or we're not ready to go past the breakpoint, this fails.
   BXSData *data=getData(dataID);
   if ((data->getBreakpoint() == false) || (data->getBreakpointGo() == BXSData::cBreakpointGoNot))
      return(false);
   //If we're in breakpoint run mode, go back to breakpointNot.  If we're in single
   //step mode, we don't change anything.
   if (data->getBreakpointGo() == BXSData::cBreakpointGoRun)
      data->setBreakpointGo(BXSData::cBreakpointGoNot);

   //Spit a quick listing helper.
   if (mMessenger->getListInterpreter() == true)
   {
      mMessenger->runMsg("");
      mMessenger->runMsg("");
      mMessenger->runMsg("BXSRuntime::interpretBreakpoint:");
   }

   //Context Runtime.
   setXSContextRuntimeID(mID);

   //Do the interp restart.  Single step if we're in single step breakpoint mode.
   if (mInterpreter->interpretRestart(mSource, data, data->getBreakpointGo()) == false)
   {
      data->setReadyToExecute(false);
      setXSContextRuntimeID(-1);
      return(false);
   }
   if ((data->getBreakpointGo() == BXSData::cBreakpointGoSingleStep) || (data->getBreakpointGo() == BXSData::cBreakpointGoSingleStepOver))
   {
      //Reset this to GoNot so that we don't keep going next update.
      data->setBreakpointGo(BXSData::cBreakpointGoNot);
   }

   //Unset calling string if we're not in a breakpoint.
   if (data->getBreakpoint() == false)
      data->setCallingString(NULL);
   //Context Runtime.
   setXSContextRuntimeID(-1);

   return(true);
}

//==============================================================================
// BXSRuntime::getCallingString
//==============================================================================
const char* BXSRuntime::getCallingString(long dataID) const
{
   //If we have more than 1 data, we cannot execute on the 0th data as that's our control instance.
   if ((mInstanceLimit > 1) && (dataID == 0))
      return(NULL);
   BXSData *data=getData(dataID);
   if (data == NULL)
      return(NULL);
   return(data->getCallingString());
}

//==============================================================================
// BXSRuntime::setCallingString
//==============================================================================
void BXSRuntime::setCallingString(long dataID, const char *text)
{
   //If we have more than 1 data, we cannot execute on the 0th data as that's our control instance.
   if ((mInstanceLimit > 1) && (dataID == 0))
      return;
   BXSData *data=getData(dataID);
   if (data != NULL)
      data->setCallingString(text);
}

//==============================================================================
// BXSRuntime::addEvent
//==============================================================================
bool BXSRuntime::addEvent(long dataID, long functionID, long parameter)
{
   //If we have more than 1 data, we cannot execute on the 0th data as that's our control instance.
   if ((mInstanceLimit > 1) && (dataID == 0))
      return(false);
   BXSData *data=getData(dataID);
   if (data == NULL)
      return(false);
   return(data->addEvent(functionID, parameter));
}

//==============================================================================
// BXSRuntime::addEvent
//==============================================================================
bool BXSRuntime::addEvent(long dataID, const char *functionName, long parameter)
{
   //If we have more than 1 data, we cannot execute on the 0th data as that's our control instance.
   if ((mInstanceLimit > 1) && (dataID == 0))
      return(false);
   BXSData *data=getData(dataID);
   if (data == NULL)
      return(false);
   long functionID=data->getFunctionID(functionName);
   return(data->addEvent(functionID, parameter));
}

//==============================================================================
// BXSRuntime::interpretEvents
//==============================================================================
bool BXSRuntime::interpretEvents(long dataID, const char *callingString)
{
   //Bomb check.
    if (isReadyToExecute(dataID) == false)
     return(false);
   //If the data is in a breakpoint, this fails.
   BXSData *data=getData(dataID);
   if (data->getBreakpoint() == true)
      return(false);

   //Spit a quick listing helper.
   if (mMessenger->getListInterpreter() == true)
   {
      mMessenger->runMsg("");
      mMessenger->runMsg("");
      mMessenger->runMsg("BXSRuntime::interpretEvents: %d events:", data->getNumberEvents());
   }
   //Set the calling string.
   data->setCallingString(callingString);
   //Context Runtime.
   setXSContextRuntimeID(mID);

   //Run through each of the events.
   bool rVal=true;
   for (long i=0; i < data->getNumberEvents(); i++)
   {
      BXSEvent *xse=data->getEvent(i);
      if (xse == NULL)
         continue;
      if (interpretHandler(dataID, xse->getFunctionID(), xse->getParameter()) == false)
         rVal=false;

      //If we now have a breakpoint set, remove the events that are processed 
      //and then return.  We count the event that's been breakpointed so that we
      //get rid of it.
      if (data->getBreakpoint() == true)
      {
         long newNumberEvents=data->getNumberEvents()-(i+1);
         data->reduceEvents(newNumberEvents);
         setXSContextRuntimeID(-1);
         return(rVal);
      }
   }

   //Unset calling string if we're not in a breakpoint.
   if (data->getBreakpoint() == false)
      data->setCallingString(NULL);
   //Context Runtime.
   setXSContextRuntimeID(-1);

   //Reset the number of events.
   data->clearEvents();
   return(rVal);
}

//==============================================================================
// BXSRuntime::inBreakpoint
//==============================================================================
bool BXSRuntime::inBreakpoint(long dataID) const
{
   //If we have more than 1 data, we cannot execute on the 0th data as that's our control instance.
   if ((mInstanceLimit > 1) && (dataID == 0))
      return(false);
   BXSData *data=getData(dataID);
   if (data == NULL)
      return(false);
   return(data->getBreakpoint());
}

//==============================================================================
// BXSRuntime::setBreakpoint
//==============================================================================
bool BXSRuntime::setBreakpoint(long dataID, const BSimString &filename, long lineNumber, bool on)
{
   //If we have more than 1 data, we cannot execute on the 0th data as that's our control instance.
   if ((mInstanceLimit > 1) && (dataID == 0))
      return(false);
   BXSData *data=getData(dataID);
   if (data == NULL)
      return(false);
   return(data->setBreakpoint(filename.getPtr(), lineNumber, on));
}

//==============================================================================
// BXSRuntime::setBreakpoint
//==============================================================================
bool BXSRuntime::setBreakpoint(long dataID, long fileID, long lineNumber, bool on)
{
   //If we have more than 1 data, we cannot execute on the 0th data as that's our control instance.
   if ((mInstanceLimit > 1) && (dataID == 0))
      return(false);
   BXSData *data=getData(dataID);
   if (data == NULL)
      return(false);
   return(data->setBreakpoint(fileID, lineNumber, on));
}

//==============================================================================
// BXSRuntime::setFunctionStartBreakpoint
//==============================================================================
bool BXSRuntime::setFunctionStartBreakpoint(long dataID, long functionID, bool on)
{
   //If we have more than 1 data, we cannot execute on the 0th data as that's our control instance.
   if ((mInstanceLimit > 1) && (dataID == 0))
      return(false);
   BXSData *data=getData(dataID);
   if (data == NULL)
      return(false);
   return(data->setFunctionStartBreakpoint(functionID, on));
}

//==============================================================================
// BXSRuntime::setStepOverBreakpoint
//==============================================================================
bool BXSRuntime::setStepOverBreakpoint(long dataID)
{
   //If we have more than 1 data, we cannot execute on the 0th data as that's our control instance.
   if ((mInstanceLimit > 1) && (dataID == 0))
      return(false);
   BXSData *data=getData(dataID);
   if (data == NULL)
      return(false);
   return(data->setStepOverBreakpoint());
}

//==============================================================================
// BXSRuntime::getCurrentTime
//==============================================================================
DWORD BXSRuntime::getCurrentTime(long dataID) const
{
   //If we have more than 1 data, we cannot execute on the 0th data as that's our control instance.
   if ((mInstanceLimit > 1) && (dataID == 0))
      return((DWORD)0);
   BXSData *data=getData(dataID);
   if (data == NULL)
      return((DWORD)0);
   return(data->getCurrentTime());
}

//==============================================================================
// BXSRuntime::setCurrentTime
//==============================================================================
void BXSRuntime::setCurrentTime(long dataID, DWORD v)
{
   //If we have more than 1 data, we cannot execute on the 0th data as that's our control instance.
   if ((mInstanceLimit > 1) && (dataID == 0))
      return;
   BXSData *data=getData(dataID);
   if (data != NULL)
      data->setCurrentTime(v);
}

#ifdef _BANG
//=============================================================================
// BXSRuntime::save
//=============================================================================
bool BXSRuntime::save(BChunkWriter *chunkWriter, bool scenario)
{
   scenario;
   //Check writer validity.
   if (chunkWriter == NULL)
   {
      BASSERT(0);
      return(false);
   }

   //Write tag.
   long mainHandle;
   long result=chunkWriter->writeTagPostSized(BCHUNKTAG("XA"), mainHandle);
   if (result == false)
   {
      {setBlogError(4393); blogerror("BXSRuntime::save -- error writing tag.");}
      return(false);
   }

   //Version.
   CHUNKWRITESAFE(chunkWriter, DWORD, msSaveVersion);

   //PlayerID.
   CHUNKWRITESAFE(chunkWriter, Long, mPlayerID);
   //Name.
   CHUNKWRITESAFE(chunkWriter, BSimString, mName);

   //DO NOT save the messenger.  We'll assume it gets configured the same for now.
   //DO NOT save the compiler.  We'll assume it gets configured the same for now.
   //DO NOT save the interpreter.  We'll assume it gets configured the same for now.
   //Syscalls.
   if (mSyscalls->save(chunkWriter) == false)
   {
      {setBlogError(4394); blogerror("BXSRuntime::save -- failed to save mSyscalls.");}
      return(false);
   }
   //Source.
   if (mSource->save(chunkWriter) == false)
   {
      {setBlogError(4395); blogerror("BXSRuntime::save -- failed to save mSource.");}
      return(false);
   }
   //DO NOT save the config functions.  We'll assume it gets configured the same for now.
   //Datas.
   CHUNKWRITESAFE(chunkWriter, Long, mDatas.getNumber());
   for (long i=0; i < mDatas.getNumber(); i++)
   {
      if (mDatas[i] == NULL)
         CHUNKWRITESAFE(chunkWriter, Bool, false);
      else
      {
         CHUNKWRITESAFE(chunkWriter, Bool, true);
         if (mDatas[i]->save(chunkWriter) == false)
         {
            {setBlogError(4396); blogerror("BXSRuntime::save -- failed to save mDatas[%d].", i);}
            return(false);
         }
      }
   }

   //Misc.
   CHUNKWRITESAFE(chunkWriter, Long, mInstanceLimit);
   CHUNKWRITESAFE(chunkWriter, Bool, mCaseSensitive);
   CHUNKWRITESAFE(chunkWriter, Bool, mWarningsOn);
   CHUNKWRITESAFE(chunkWriter, Bool, mInfoMessagesOn);
   CHUNKWRITESAFE(chunkWriter, Bool, mRunMessagesOn);
   CHUNKWRITESAFE(chunkWriter, Bool, mListInterpreter);
   CHUNKWRITESAFE(chunkWriter, Bool, mListFunctionEntry);
   CHUNKWRITESAFE(chunkWriter, Bool, mGenerateListing);
   CHUNKWRITESAFE(chunkWriter, Bool, mDebugTokenizer);
   CHUNKWRITESAFE(chunkWriter, Bool, mDebugCode);
   CHUNKWRITESAFE(chunkWriter, Long, mBaseDirectoryID);
   CHUNKWRITESAFE(chunkWriter, Long, mBaseUWDirectoryID);

   //DO NOT save any of the output function pointers.
   //DO NOT save any of the static vars.

   //Finish chunk.   
   result=chunkWriter->writeSize(mainHandle);
   if (result == false)
   {
      {setBlogError(4397); blogerror("BXSRuntime::save -- failed to write chunk size!");}
      return(false);
   }

   return(true);
}

//=============================================================================
// BXSRuntime::load
//=============================================================================
bool BXSRuntime::load(BChunkReader *chunkReader, bool scenario)
{
   scenario;
   //Check reader validity.
   if (chunkReader == NULL)
   {
      BASSERT(0);
      return(false);
   }

   //Read tag.
   long result=chunkReader->readExpectedTag(BCHUNKTAG("XA"));
   if (result == false)
   {
      {setBlogError(4398); blogerror("BXSRuntime::load -- error reading tag.");}
      return(false);
   }

   //Version.
   DWORD version=(DWORD)0;
   CHUNKREADSAFE(chunkReader, DWORD, version);

   //ID.
	if(version<=2)
	{
		long tempID;
		CHUNKREADSAFE(chunkReader, Long, tempID);
		//mID is set at creation time.  No need to read it in.
	}
	
   //PlayerID.
   CHUNKREADSAFE(chunkReader, Long, mPlayerID);
   //Name.
   CHUNKREADSAFE(chunkReader, BSimString, mName);

   //DO NOT save the messenger.  We'll assume it gets configured the same for now.
   //DO NOT save the compiler.  We'll assume it gets configured the same for now.
   //DO NOT save the interpreter.  We'll assume it gets configured the same for now.
   //Syscalls.  Work our mojo to relink/validate the syscalls.
   mSyscalls->setLoading(true);
   if (mSyscalls->load(chunkReader) == false)
   {
      {setBlogError(4399); blogerror("BXSRuntime::load -- failed to load mSyscalls.");}
      return(false);
   }
   //Reconfigure the source now.
   for (long i=0; i < mConfigFunctions.getNumber(); i++)
   {
      BXSConfigFunction cf=(BXSConfigFunction)mConfigFunctions[i];
      cf(mSyscalls);
   }
   mSyscalls->setLoading(false);
   //Source.
   if (mSource->load(chunkReader) == false)
   {
      {setBlogError(4400); blogerror("BXSRuntime::load -- failed to load mSource.");}
      return(false);
   }
   //DO NOT save the config functions.  We'll assume it gets configured the same for now.
   //Datas.
   long numDatas=0;
   CHUNKREADSAFE(chunkReader, Long, numDatas);
   if (mDatas.setNumber(numDatas) == false)
   {
      {setBlogError(4401); blogerror("BXSRuntime::load -- failed to allocate %d mDatas.", numDatas);}
      return(false);
   }
   for (long i=0; i < mDatas.getNumber(); i++)
   {
      //Load the data.
      bool haveData=false;
      CHUNKREADSAFE(chunkReader, Bool, haveData);
      if (haveData == true)
      {
         //Allocate the data.
         // If we already have a data (xsAllocateRuntime allocates at least one..) then 
         // make sure it's deleted before allocating a new one. dlm 11/21/03
         if (mDatas[i])
         {
            delete mDatas[i];
            mDatas[i] = NULL;
         }
         mDatas[i]=new BXSData(i, mName, mMessenger, mSource);
         if (mDatas[i] == NULL)
         {
            {setBlogError(4402); blogerror("BXSRuntime::load -- failed to allocate mDatas[%d].", i);}
            return(false);
         }

         if (mDatas[i]->load(chunkReader) == false)
         {
            {setBlogError(4403); blogerror("BXSRuntime::load -- failed to load mDatas[%d].", i);}
            return(false);
         }
      }
      else
         mDatas[i]=NULL;
   }

   //Misc.
   CHUNKREADSAFE(chunkReader, Long, mInstanceLimit);
   CHUNKREADSAFE(chunkReader, Bool, mCaseSensitive);
   CHUNKREADSAFE(chunkReader, Bool, mWarningsOn);
   CHUNKREADSAFE(chunkReader, Bool, mInfoMessagesOn);
   CHUNKREADSAFE(chunkReader, Bool, mRunMessagesOn);
   CHUNKREADSAFE(chunkReader, Bool, mListInterpreter);
   CHUNKREADSAFE(chunkReader, Bool, mListFunctionEntry);
   CHUNKREADSAFE(chunkReader, Bool, mGenerateListing);
   CHUNKREADSAFE(chunkReader, Bool, mDebugTokenizer);
   CHUNKREADSAFE(chunkReader, Bool, mDebugCode);
   CHUNKREADSAFE(chunkReader, Long, mBaseDirectoryID);
   CHUNKREADSAFE(chunkReader, Long, mBaseUWDirectoryID);

   //DO NOT save any of the output function pointers.
   //DO NOT save any of the static vars.

   //Validate our reading of the chunk.
   result=chunkReader->validateChunkRead(BCHUNKTAG("XA"));
   if (result == false)
   {
      {setBlogError(4404); blogerror("BXSRuntime::load -- did not read chunk properly!");}
      return(false);
   }

   return(true);
}
#endif

//==============================================================================
// BXSRuntime::getOutputChanged
//==============================================================================
bool BXSRuntime::getOutputChanged(long dataID) const
{
   BXSData *data=getData(dataID);
   if (data == NULL)
      return(false);
   return(data->getOutputChanged());
}

//==============================================================================
// BXSRuntime::clearOutputChanged
//==============================================================================
void BXSRuntime::clearOutputChanged(long dataID)
{
   BXSData *data=getData(dataID);
   if (data != NULL)
      data->clearOutputChanged();
}

//==============================================================================
// BXSRuntime::addOutput
//==============================================================================
void BXSRuntime::addOutput(long dataID, const BColor &color, const char *v, ... )
{
   BXSData *data=getData(dataID);
   if (data != NULL)
   {
      static char out[1024];
      va_list va;
      va_start(va, v);
      bvsnprintf(out, sizeof(out), v, va);
      data->addOutput(color, out);
   }
}

//==============================================================================
// BXSRuntime::getOutput
//==============================================================================
const BDynamicSimArray<BSimString>& BXSRuntime::getOutput(long dataID) const
{
   BXSData *data=getData(dataID);
   if (data == NULL)
   {
      static BDynamicSimArray<BSimString> foo;
      return(foo);
   }
   return(data->getOutput());
}

//==============================================================================
// BXSRuntime::getOutputColors
//==============================================================================
const BDynamicSimArray<BColor>& BXSRuntime::getOutputColors(long dataID) const
{
   BXSData *data=getData(dataID);
   if (data == NULL)
   {
      static BDynamicSimArray<BColor> foo;
      return(foo);
   }
   return(data->getOutputColors());
}

//==============================================================================
// BXSRuntime::getOutputLineNumber
//==============================================================================
long BXSRuntime::getOutputLineNumber(long dataID) const
{
   BXSData *data=getData(dataID);
   if (data == NULL)
      return(0);
   return(data->getOutputLineNumber());
}

//==============================================================================
// BXSRuntime::setOutputLineNumber
//==============================================================================
void BXSRuntime::setOutputLineNumber(long dataID, long v) const
{
   BXSData *data=getData(dataID);
   if (data != NULL)
      data->setOutputLineNumber(v);
}

//==============================================================================
// BXSRuntime::defaultOutputMessage
//==============================================================================
void BXSRuntime::defaultOutputMessage(const char *text)
{
   OutputDebugStringA(text);
   puts(text);
}

//==============================================================================
// BXSRuntime::getXSRuntimeByID
//==============================================================================
BXSRuntime* BXSRuntime::getXSRuntimeByID(long id)
{
   for (long i=0; i < mXSRuntimes.getNumber(); i++)
   {
      if (mXSRuntimes[i]->getID() == id)
         return(mXSRuntimes[i]);
   }
   return(NULL);
}

//==============================================================================
// BXSRuntime::getXSRuntimeByIndex
//==============================================================================
BXSRuntime* BXSRuntime::getXSRuntimeByIndex(long index)
{
   if ((index < 0) || (index >= mXSRuntimes.getNumber()) )
      return(NULL);
   return(mXSRuntimes[index]);
}

//==============================================================================
// BXSRuntime::getXSRuntime
//==============================================================================
BXSRuntime* BXSRuntime::getXSRuntime(const BSimString &name)
{
   for (long i=0; i < mXSRuntimes.getNumber(); i++)
   {
      if (mXSRuntimes[i]->getName() == name)
         return(mXSRuntimes[i]);
   }
   return(NULL);
}

//==============================================================================
// BXSRuntime::addCommonMathVariables
//==============================================================================
bool BXSRuntime::addCommonMathVariables(void)
{
   if (mCompiler == NULL)
      return(false);

   mCompiler->addExternalVariable("const vector cInvalidVector=vector(-1.0, -1.0, -1.0);");
   mCompiler->addExternalVariable("const vector cOriginVector=vector(0.0, 0.0, 0.0);");
   return(true);
}

//==============================================================================
// BXSRuntime::cleanUpAll
//==============================================================================
void BXSRuntime::cleanUpAll(void)
{
   //Name.
   mName.empty();
   //ID.
   mID=-1;
   //Remove from the runtime list.
   mXSRuntimes.remove(this);
   //Finish the non-instance cleanup.
   cleanUpData();
}

//==============================================================================
// BXSRuntime::cleanUpData
//==============================================================================
void BXSRuntime::cleanUpData(void)
{
   //We DO NOT nuke the config functions.  We need them around to re-init later.

   //Datas.
   for (long i=0; i < mDatas.getNumber(); i++)
   {
      if (mDatas[i] != NULL)
      {
         delete mDatas[i];
         mDatas[i]=NULL;
      }
   }
   //Source.
   if (mSource != NULL)
   {
      delete mSource;
      mSource=NULL;
   }
   //Compiler.
   if (mCompiler != NULL)
   {
      delete mCompiler;
      mCompiler=NULL;
   }
   //Interpreter.
   if (mInterpreter != NULL)
   {
      delete mInterpreter;
      mInterpreter=NULL;
   }
   //Syscalls.
   if (mSyscalls != NULL)
   {
      delete mSyscalls;
      mSyscalls=NULL;
   }
   //Messenger.
   if (mMessenger != NULL)
   {
      delete mMessenger;
      mMessenger=NULL;
   }
}

