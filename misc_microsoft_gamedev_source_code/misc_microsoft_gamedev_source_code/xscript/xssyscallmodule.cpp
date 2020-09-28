//==============================================================================
// xssyscallmodule.cpp
//
//Copyright (c) 2000-2003, Ensemble Studios
//==============================================================================

//==============================================================================
//Includes
#include "xscript.h"
#include "xssyscallmodule.h"
#include "xsactivationrecord.h"
#include "xsdata.h"
#include "xsdefines.h"
#include "xsfileentry.h"
#include "xsmessenger.h"
#include "xsvariableentry.h"
#include "xssyscalltypes.h"
#ifdef _BANG
#include "chunker.h"
#endif

//==============================================================================
//Defines
extern const char* gSyscallTypeString[];


//==============================================================================
// BXSSyscallModule Static stuff
//==============================================================================
const DWORD BXSSyscallModule::msSaveVersion=2;
bool BXSSyscallModule::mSyscallTypesInitialized=false;
BStringTable<long> BXSSyscallModule::mSyscallTypeStrings;


//==============================================================================
// BXSSyscallModule::BXSSyscallModule
//==============================================================================
BXSSyscallModule::BXSSyscallModule(BXSMessenger *messenger,
   BXSSymbolTable *symbolTable, bool caseSensitive,
   bool addSymbols, bool storeHelp) :
   mMessenger(messenger),
   //mSyscalls doesn't need any ctor args.
   mSymbols(symbolTable),
   mCaseSensitive(caseSensitive),
   mAddSymbols(addSymbols),
   mStoreHelp(storeHelp),
   mNewSyscall(NULL),
   mLoading(false),
   mLoadSyscall(NULL),
   mLoadAddress(-1),
   mLoadCurrentParameter(-1),
   mLoadSyscallFailed(false)
{
   //Syscall type table.
   if(!mSyscallTypesInitialized)
      initializeSyscallTypes();

   //Symbols.
   if (mAddSymbols == true)
      mSymbols->initialize(113, caseSensitive, -1, BXSBinary::cInvalidSymbol);
}

//==============================================================================
// BXSSyscallModule::~BXSSyscallModule
//==============================================================================
BXSSyscallModule::~BXSSyscallModule(void)
{
   cleanUp();
}

//==============================================================================
//BXSSyscallModule::initializeSyscallTypes
//==============================================================================
void BXSSyscallModule::initializeSyscallTypes()
{
   if(mSyscallTypesInitialized)
      return;

  for(long i=0; i<cNumberSyscallTypes; i++)
  {
     const char* syscallTypeString=gSyscallTypeString[i];
     BASSERT(*syscallTypeString!=NULL);
     mSyscallTypeStrings.add(syscallTypeString, i);
  }

   mSyscallTypesInitialized=true;
}

//==============================================================================
//BXSSyscallModule::buildSyscallTypeString
//==============================================================================
bool BXSSyscallModule::buildSyscallTypeString(const BXSSyscallEntry* entry, char* buffer, long bufferSize) const
{
   if(!buffer || bufferSize<1)
      return false;

   long sameCount=0;
   long lastType=-1;
   long transitionCount=0;
   long bufferIndex=0;

   long parameterCount=entry->getNumberParameters();
   if(parameterCount==0)
   {
      buffer[0]=NULL;
      return true;
   }

   for(long i=0; i<=parameterCount; i++)
   {
      long parameterType;
      
      if(i==parameterCount)
         parameterType=-1;
      else
      {
         parameterType=entry->getParameterType(i);
         if(parameterType!=BXSVariableEntry::cFloatVariable && parameterType!=BXSVariableEntry::cBoolVariable)
            parameterType=BXSVariableEntry::cIntegerVariable;
         if(parameterType==lastType)
         {
            sameCount++;
            continue;
         }
      }

      if(sameCount>0)
      {
         char c='?';
         switch(lastType)
         {
            case BXSVariableEntry::cIntegerVariable: c='d'; break;
            case BXSVariableEntry::cFloatVariable  : c='f'; break;
            case BXSVariableEntry::cBoolVariable   : c='b'; break;
         }
         buffer[bufferIndex] = c;
         bufferIndex++;

         if(sameCount<10)
            buffer[bufferIndex]=(char)('1'+(sameCount-1));
         else
            buffer[bufferIndex]=(char)('A'+(sameCount-10));
         bufferIndex++;
      }

      lastType=parameterType;
      sameCount=1;
      transitionCount++;
   }

   // If all parameters are of the same type, then set the buffer to a single character.
   if(transitionCount==2)
      bufferIndex=1;

   buffer[bufferIndex]=NULL;

   return true;
}

//==============================================================================
//BXSSyscallModule::getSyscallType
//==============================================================================
long BXSSyscallModule::getSyscallType(const char* syscallTypeString) const
{
   if(syscallTypeString==NULL || *syscallTypeString==NULL)
      return cSyscallType_void;

   long val;
   if(mSyscallTypeStrings.find(syscallTypeString, &val))
      return val;

   // If you hit this assert, you've added a console function with an unsupported
   // param set.  You must add support for your function type in xssyscalltypes.h/.cpp
   BASSERT(0);
   return -1;
}

//==============================================================================
//BXSSyscallModule::getSyscall
//==============================================================================
const BXSSyscallEntry* BXSSyscallModule::getSyscall(long syscallID) const
{
   //Bomb checks.
   if ((syscallID < 0) || (syscallID >= mSyscalls.getNumber()) )
      return(NULL);
   return(mSyscalls[syscallID]);
}

//==============================================================================
// BXSSyscallModule::addSyscall
//==============================================================================
bool BXSSyscallModule::addSyscall(const char *syscallName, void *address, BYTE returnType)
{
   //If we have a new syscall, we didn't finish adding it, so we fail.
   if (mNewSyscall != NULL)
   {
      BASSERT(0);
      mMessenger->errorMsg("Error : unfinished syscall addition.");
      return(false);
   }

   //Now, process the new potential syscall addition.  Fail if there's a previous symbol match.
   long previousSyscallID=getSyscallID(syscallName);
   if (previousSyscallID >= 0)
   {
      //If we're loading, do the compare work.
      if (mLoading == true)
      {
         if (mLoadSyscall != NULL)
         {
            BASSERT(0);
            mLoadSyscall=NULL;
            mLoadAddress=-1;
            mLoadCurrentParameter=-1;
            mLoadSyscallFailed=false;
            mMessenger->errorMsg("Error : '%s' is stomping on the load of a previous syscall.", syscallName);
            return(false);
         }

         //Save the load syscall.
         if ((previousSyscallID >= 0) && (previousSyscallID < mSyscalls.getNumber()) )
         {
            mLoadSyscall=mSyscalls[previousSyscallID];
            mLoadAddress=(long)address;
            mLoadCurrentParameter=-1;
            mLoadSyscallFailed=false;
            return(true);
         }
      }

      //If we're here and we're supposed to be adding symbols, this is an error.
      if (mAddSymbols == true)
      {
         // MS 9/16/2003: small cosmetic change, ported from xpack.
         // First of all, only do this if we're not in final release mode.
         // Second, change the message to more accurately reflect what's going on.  dlm 8/12/03
         #ifndef BUILD_FINAL
         mMessenger->errorMsg("Error : '%s' was not previously defined for this savegame, and thus will be invalid.", syscallName);
         #endif
         return(false);
      }
   }
   //Verify the return type is something we support.
   if (isValidVariableType(returnType) == false)
   {
      mMessenger->errorMsg("Error : %d is not a valid type ID for a return value.", returnType);
      return(false);
   }

   //Create the new syscall entry.
   mNewSyscall=new BXSSyscallEntry();
   if (mNewSyscall == NULL)
   {
      mMessenger->errorMsg("Error : could not create syscall entry for '%s' syscall.", syscallName);
      return(false);
   }
   //Add an entry into the symbol table.  The type will be a syscall.  The value is -1 for now.
   long symbolID=-1;
   if (mAddSymbols == true)
   {
      symbolID=mSymbols->addSymbol(syscallName, BXSBinary::cSyscall, -1);
      if (symbolID < 0)
      {
         mMessenger->errorMsg("Error : unable to add a symbol for '%s' syscall.", syscallName);
         return(false);
      }
   }
   else
   {
      symbolID=getSymbolID(syscallName, BXSBinary::cSyscall);
      if (symbolID < 0)
      {
         mMessenger->errorMsg("Error : unable to find an expected match for '%s' syscall symbol.", syscallName);
         return(false);
      }
   }

   //Set the data for the syscall.
   mNewSyscall->setID(-1);
   mNewSyscall->setSymbolID(symbolID);
   mNewSyscall->setReturnType(returnType);
   mNewSyscall->setAddress((long)address);

   return(true);
}

//==============================================================================
// BXSSyscallModule::addSyscallIntegerParameter
//==============================================================================
bool BXSSyscallModule::addSyscallIntegerParameter(long defaultValue)
{
   return(addSyscallParameter(BXSVariableEntry::cIntegerVariable, &defaultValue));
}

//==============================================================================
// BXSSyscallModule::addSyscallFloatParameter
//==============================================================================
bool BXSSyscallModule::addSyscallFloatParameter(float defaultValue)
{
   return(addSyscallParameter(BXSVariableEntry::cFloatVariable, &defaultValue));
}

//==============================================================================
// BXSSyscallModule::addSyscallBoolParameter
//==============================================================================
bool BXSSyscallModule::addSyscallBoolParameter(bool defaultValue)
{
   return(addSyscallParameter(BXSVariableEntry::cBoolVariable, &defaultValue));
}

//==============================================================================
// BXSSyscallModule::addSyscallStringParameter
//==============================================================================
bool BXSSyscallModule::addSyscallStringParameter(const char *defaultValue)
{
   if (defaultValue == NULL)
   {
      mMessenger->errorMsg("Error : NULL string parameter.");
      return(false);
   }
   return(addSyscallParameter(BXSVariableEntry::cStringVariable, (void *)defaultValue));
}

//==============================================================================
// BXSSyscallModule::addSyscallVectorParameter
//==============================================================================
bool BXSSyscallModule::addSyscallVectorParameter(const BVector &defaultValue)
{
   return(addSyscallParameter(BXSVariableEntry::cVectorVariable, (void*)(&defaultValue)));
}

//==============================================================================
// BXSSyscallModule::setSyscallContext
//==============================================================================
bool BXSSyscallModule::setSyscallContext(bool v)
{
   //If we're loading, validate the parm against the load syscall.
   if (mLoading == true)
   {
      if (mLoadSyscall == NULL)
         return(false);
      if (mLoadSyscall->getContext() != v)
      {
         mLoadSyscallFailed=true;
         return(false);
      }
      return(true);
   }

   //Bomb check.
   if (mNewSyscall == NULL)
      return(false);
   mNewSyscall->setContext(v);
   return(true);
}

//==============================================================================
// BXSSyscallModule::addSyscallParameter
//==============================================================================
bool BXSSyscallModule::addSyscallParameter(BYTE parmType, void *data)
{
   //If we're loading, validate the parm against the load syscall.
   if (mLoading == true)
   {
      if (mLoadSyscall == NULL)
         return(false);
      mLoadCurrentParameter++;
      if (mLoadSyscall->getParameterType(mLoadCurrentParameter) != parmType)
         mLoadSyscallFailed=true;
      return(true);
   }

   //Bomb check.
   if (mNewSyscall == NULL)
      return(false);

   //Allocate the new parameter entry.
   BXSVariableEntry *newVE=new BXSVariableEntry();
   if (newVE == NULL)
      return(false);
   //Set the values.
   if (newVE->setAll(-1, parmType, data) == false)
   {
      delete newVE;
      return(false);
   }
   //Add it to the syscall.
   return(mNewSyscall->addParameter(newVE));
}

//==============================================================================
//BXSSyscallModule::setSyscallHelp
//==============================================================================
bool BXSSyscallModule::setSyscallHelp(const char *helpString)
{
   //If we're loading, set this into the load syscall as we don't save help
   //in the savegame.
   if (mLoading == true)
   {
      if (mLoadSyscall == NULL)
         return(false);
      if (mStoreHelp == false)
         return(true);
      return(mLoadSyscall->setHelp(helpString));
   }

   //Bomb check.
   if (mNewSyscall == NULL)
      return(false);
   if (mStoreHelp == false)
      return(true);
   return(mNewSyscall->setHelp(helpString));
}

//==============================================================================
// BXSSyscallModule::finishSyscallAdd
//==============================================================================
bool BXSSyscallModule::finishSyscallAdd(void)
{
   //If we're loading, call the load validate call instead.
   if (mLoading == true)
   {
      validateLoadSyscall();
      return(true);
   }

   //Fail this if we don't have a new syscall to add.
   if (mNewSyscall == NULL)
   {
      mMessenger->errorMsg("Error : unable to finish adding a non-existant syscall.");
      return(false);
   }

   //Get the new ID.
   long newSyscallID=mSyscalls.getNumber();
   //Add an entry to the syscall array.
   if (mSyscalls.setNumber(newSyscallID+1) == false)
   {
      mMessenger->errorMsg("Error : unable to add an entry to the call array.");
      return(false);
   }
   //Set the ID as the value of the symbol in the symbol table.
   if (mAddSymbols == true)
   {
      if (mSymbols->setSymbolValue(mNewSyscall->getSymbolID(), newSyscallID) == false)
      {
         mMessenger->errorMsg("Error : unable to add set syscall ID for this symbol.");
         return(false);
      }
   }

   //Calculate the syscall type.
   char buffer[16];
   if(buildSyscallTypeString(mNewSyscall, buffer, sizeof(buffer)))
   {
      long syscallType=getSyscallType(buffer);
      mNewSyscall->setSyscallType(syscallType);
   }

   //Actually add it.
   mNewSyscall->setID(newSyscallID);
   mSyscalls[newSyscallID]=mNewSyscall;

   //Lastly, NULL out the new syscall.
   mNewSyscall=NULL;
   //Done.
   return(true);
}

//==============================================================================
// BXSSyscallModule::getNewSyscallName
//==============================================================================
const char* BXSSyscallModule::getNewSyscallName(void) const
{
   static char n[]="NoNewSyscallDefined";
   if (mNewSyscall == NULL)
      return(n);
   const char* rVal=mSymbols->getSymbolByID(mNewSyscall->getSymbolID());
   if (rVal == NULL)
      return(n);
   return(rVal);
}

//==============================================================================
// BXSSyscallModule::clearSyscall
//==============================================================================
void BXSSyscallModule::clearSyscall(void)
{
   //If we have a partially loaded/created syscall, blow those away.
   if (mNewSyscall != NULL)
   {
      delete mNewSyscall;
      mNewSyscall=NULL;
   }
   //Reset everything.
   if (mLoadSyscall != NULL)
   {
      mLoadSyscall=NULL;
      mLoadAddress=-1;
      mLoadCurrentParameter=-1;
      mLoadSyscallFailed=false;
   }
}

//==============================================================================
// BXSSyscallModule::isValidVariableType
//==============================================================================
bool BXSSyscallModule::isValidVariableType(long type)
{
   if ((type == BXSVariableEntry::cVoidVariable) ||
      (type == BXSVariableEntry::cIntegerVariable) ||
      (type == BXSVariableEntry::cFloatVariable) ||
      (type == BXSVariableEntry::cBoolVariable) ||
      (type == BXSVariableEntry::cStringVariable) ||
      (type == BXSVariableEntry::cVectorVariable))
      return(true);
   return(false);
}

//==============================================================================
// BXSSyscallModule::addSymbol
//==============================================================================
long BXSSyscallModule::addSymbol(const char *symbol, BYTE type, long value)
{
   long rID=mSymbols->addSymbol(symbol, type, value);
   return(rID);
}

//==============================================================================
// BXSSyscallModule::getSymbolID
//==============================================================================
long BXSSyscallModule::getSymbolID(const char *symbol, BYTE type)
{
   long rID=mSymbols->getIDBySymbolAndType(symbol, type);
   return(rID);
}

//==============================================================================
// BXSSyscallModule::getSyscallID
//==============================================================================
long BXSSyscallModule::getSyscallID(const char *syscallName)
{
   if (syscallName == NULL)
      return(-1);
   return(mSymbols->getIDBySymbolAndType(syscallName, BXSBinary::cSyscall));
}

//==============================================================================
//BXSSyscallModule::getSyscallName
//==============================================================================
char* BXSSyscallModule::getSyscallName(long syscallID) const
{
   if ((syscallID < 0) || (syscallID >= mSyscalls.getNumber()) )
      return(NULL);

   //Get the symbol entry for this syscall's symbol.
   const BXSSymbolEntry *se=mSymbols->getEntryByID(mSyscalls[syscallID]->getSymbolID());
   if (se == NULL)
      return(NULL);
   return(se->mSymbol);
}

//==============================================================================
//BXSSyscallModule::matchSyscallName
//==============================================================================
void BXSSyscallModule::matchSyscallName(const char *matchString, bool prefix, bool complete, BDynamicSimLongArray &results) const
{
   //Reset results.
   results.setNumber(0);

   //Bomb check.
   if (matchString == NULL)
      return;

   //Rip through all of the syscalls.  Stuff the IDs for any matches into the results.
   long matchStringLength=strlen(matchString);
   for (long i=0; i < mSymbols->getNumberSymbols(); i++)
   {
      const BXSSymbolEntry *se=mSymbols->getEntryByID(i);
      if ((se == NULL) || (se->mType != BXSBinary::cSyscall))
         continue;

      //If we have a prefix req, then only check the prefix.
      if (mCaseSensitive == true)
      {
         if (prefix == true)
         {
            if (strncmp(se->mSymbol, matchString, matchStringLength) == 0)
               results.add(se->mValue);
         }
         //Else, if we have to be a complete match, check the whole thing.
         else if (complete == true)
         {
            if (strcmp(se->mSymbol, matchString) == 0)
               results.add(se->mValue);
         }
         //Else, check for any substring match.
         else
         {
            if (strstr(se->mSymbol, matchString) != 0)
               results.add(se->mValue);
         }
      }
      else
      {
         //Obviously not the fastest thing, but it's okay for now.
         char temp1[256];
         StringCchCopyA(temp1, 256, se->mSymbol);
         for (long j=0; j < (long)strlen(temp1); j++)
            temp1[j]=(char)toupper(temp1[j]);
         char temp2[256];
         StringCchCopyA(temp2, 256, matchString);
         for (long j=0; j < (long)strlen(temp2); j++)
            temp2[j]=(char)toupper(temp2[j]);

         if (prefix == true)
         {
            if (strncmp(temp1, temp2, matchStringLength) == 0)
               results.add(se->mValue);
         }
         //Else, if we have to be a complete match, check the whole thing.
         else if (complete == true)
         {
            if (strcmp(temp1, temp2) == 0)
               results.add(se->mValue);
         }
         //Else, check for any substring match.
         else
         {
            if (strstr(temp1, temp2) != 0)
               results.add(se->mValue);
         }
      }
   }
}

//==============================================================================
//BXSSyscallModule::matchSyscallHelp
//==============================================================================
void BXSSyscallModule::matchSyscallHelp(const char *matchString, bool prefix, bool complete, BDynamicSimLongArray &results) const
{
   //Reset results.
   results.setNumber(0);

   //Bomb check.
   if (matchString == NULL)
      return;

   //Rip through all of the syscalls.  Stuff the IDs for any matches into the results.
   long matchStringLength=strlen(matchString);
   for (long i=0; i < mSyscalls.getNumber(); i++)
   {
      if (mSyscalls[i]->getHelp() == NULL)
         continue;

      //If we have a prefix req, then only check the prefix.
      if (prefix == true)
      {
         if (strncmp(mSyscalls[i]->getHelp(), matchString, matchStringLength) == 0)
            results.add(i);
      }
      //Else, if we have to be a complete match, check the whole thing.
      else if (complete == true)
      {
         if (strcmp(mSyscalls[i]->getHelp(), matchString) == 0)
            results.add(i);
      }
      //Else, check for any substring match.
      else
      {
         if (strstr(mSyscalls[i]->getHelp(), matchString) != 0)
            results.add(i);
      }
   }
}

//==============================================================================
// BXSSyscallModule::validateLoadSyscall
//==============================================================================
void BXSSyscallModule::validateLoadSyscall(void)
{
   //If we're not loading or we don't have a load syscall, just ignore this.
   if ((mLoading == false) || (mLoadSyscall == NULL))
      return;

   //If we haven't already determined failure, check the last condition(s) for
   //a load match.
   if (mLoadSyscallFailed == false)
   {
      //Make sure we've matched all of the parameters.  If we have, save the address.
      if (mLoadCurrentParameter+1 == mLoadSyscall->getNumberParameters())
         mLoadSyscall->setAddress(mLoadAddress);
   }

   //Reset everything.
   mLoadSyscall=NULL;
   mLoadAddress=-1;
   mLoadCurrentParameter=-1;
   mLoadSyscallFailed=false;
}

//==============================================================================
// BXSSyscallModule::errorMsg
//==============================================================================
void BXSSyscallModule::errorMsg(const char *lpszFormat, ...)
{
   va_list args;
   va_start(args, lpszFormat);
   char szBuffer[1024];
   bvsnprintf(szBuffer, sizeof(szBuffer), lpszFormat, args);
   mMessenger->errorMsg(szBuffer);
   va_end(args);
}

//==============================================================================
// BXSSyscallModule::infoMsg
//==============================================================================
void BXSSyscallModule::infoMsg(const char *lpszFormat, ...)
{
   va_list args;
   va_start(args, lpszFormat);
   char szBuffer[1024];
   bvsnprintf(szBuffer, sizeof(szBuffer), lpszFormat, args);
   mMessenger->infoMsg(szBuffer);
   va_end(args);
}

//=============================================================================
// BXSSyscallModule::cleanUp
//=============================================================================
void BXSSyscallModule::cleanUp(void)
{
   for (long i=0; i < mSyscalls.getNumber(); i++)
   {
      delete mSyscalls[i];
      mSyscalls[i]=NULL;
   }
   mSyscalls.setNumber(0);

   BDELETE(mNewSyscall);
}

#ifdef _BANG
//=============================================================================
// BXSSyscallModule::save
//=============================================================================
bool BXSSyscallModule::save(BChunkWriter *chunkWriter)
{
   //Check writer validity.
   if (chunkWriter == NULL)
   {
      BASSERT(0);
      return(false);
   }

   //Write tag.
   long mainHandle;
   long result=chunkWriter->writeTagPostSized(BCHUNKTAG("yf"), mainHandle);
   if (result == false)
   {
      {setBlogError(4451); blogerror("BXSSyscallModule::save -- error writing tag.");}
      return(false);
   }

   //Version.
   CHUNKWRITESAFE(chunkWriter, DWORD, msSaveVersion);

   //Static save versions for things that this class allocates.
   if (BXSActivationRecord::writeVersion(chunkWriter) == false)
   {
      {setBlogError(4452); blogerror("BXSData::save -- failed to save BXSActivationRecord version.");}
      return(false);
   }
   if (BXSBreakpoint::writeVersion(chunkWriter) == false)
   {
      {setBlogError(4453); blogerror("BXSData::save -- failed to save BXSBreakpoint version.");}
      return(false);
   }
   if (BXSEvent::writeVersion(chunkWriter) == false)
   {
      {setBlogError(4454); blogerror("BXSData::save -- failed to save BXSBreakpoint version.");}
      return(false);
   }
   if (BXSVariableEntry::writeVersion(chunkWriter) == false)
   {
      {setBlogError(4455); blogerror("BXSData::save -- failed to save BXSVariableEntry version.");}
      return(false);
   }
   if (BXSFileEntry::writeVersion(chunkWriter) == false)
   {
      {setBlogError(4456); blogerror("BXSData::save -- failed to save BXSFileEntry version.");}
      return(false);
   }
   if (BXSFunctionEntry::writeVersion(chunkWriter) == false)
   {
      {setBlogError(4457); blogerror("BXSData::save -- failed to save BXSFunctionEntry version.");}
      return(false);
   }
   if (BXSSymbolEntry::writeVersion(chunkWriter) == false)
   {
      {setBlogError(4458); blogerror("BXSData::save -- failed to save BXSSymbolEntry version.");}
      return(false);
   }
   if (BXSSymbolTable::writeVersion(chunkWriter) == false)
   {
      {setBlogError(4459); blogerror("BXSData::save -- failed to save BXSSymbolTable version.");}
      return(false);
   }
   if (BXSSyscallEntry::writeVersion(chunkWriter) == false)
   {
      {setBlogError(4460); blogerror("BXSData::save -- failed to save BXSSyscallEntry version.");}
      return(false);
   }

   //DO NOT SAVE mMessenger.  It will get relinked from outside when this class is created.
   //Syscalls.
   CHUNKWRITESAFE(chunkWriter, Long, mSyscalls.getNumber());
   for (long i=0; i < mSyscalls.getNumber(); i++)
   {
      if (mSyscalls[i]->save(chunkWriter) == false)
      {
         {setBlogError(4461); blogerror("BXSSyscallModule::save -- failed to save mSyscalls[%d].", i);}
         return(false);
      }
   }
   //Symbols.
   if (mSymbols->save(chunkWriter) == false)
   {
      {setBlogError(4462); blogerror("BXSSyscallModule::save -- failed to save mSymbols->");}
      return(false);
   }
   //Case sensitive.
   CHUNKWRITESAFE(chunkWriter, Bool, mCaseSensitive);
   //Add symbols.
   CHUNKWRITESAFE(chunkWriter, Bool, mAddSymbols);
   //Store help.
   CHUNKWRITESAFE(chunkWriter, Bool, mStoreHelp);

   //Finish chunk.   
   result=chunkWriter->writeSize(mainHandle);
   if (result == false)
   {
      {setBlogError(4463); blogerror("BXSSyscallModule::save -- failed to write chunk size!");}
      return(false);
   }

   return(true);
}

//=============================================================================
// BXSSyscallModule::load
//=============================================================================
bool BXSSyscallModule::load(BChunkReader *chunkReader)
{
   //Check reader validity.
   if (chunkReader == NULL)
   {
      BASSERT(0);
      return(false);
   }
   //Cleanup.
   cleanUp();

   //Read tag.
   long result=chunkReader->readExpectedTag(BCHUNKTAG("yf"));
   if (result == false)
   {
      {setBlogError(4464); blogerror("BXSSyscallModule::load -- error reading tag.");}
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
   if (version >= 2)
   {
      if (BXSActivationRecord::readVersion(chunkReader) == false)
      {
         {setBlogError(4465); blogerror("BXSData::load -- failed to load BXSActivationRecord version.");}
         return(false);
      }
      if (BXSBreakpoint::readVersion(chunkReader) == false)
      {
         {setBlogError(4466); blogerror("BXSData::load -- failed to load BXSBreakpoint version.");}
         return(false);
      }
      if (BXSEvent::readVersion(chunkReader) == false)
      {
         {setBlogError(4467); blogerror("BXSData::load -- failed to load BXSBreakpoint version.");}
         return(false);
      }
      if (BXSVariableEntry::readVersion(chunkReader) == false)
      {
         {setBlogError(4468); blogerror("BXSData::load -- failed to load BXSVariableEntry version.");}
         return(false);
      }
      if (BXSFileEntry::readVersion(chunkReader) == false)
      {
         {setBlogError(4469); blogerror("BXSData::load -- failed to load BXSFileEntry version.");}
         return(false);
      }
      if (BXSFunctionEntry::readVersion(chunkReader) == false)
      {
         {setBlogError(4470); blogerror("BXSData::load -- failed to load BXSFunctionEntry version.");}
         return(false);
      }
      if (BXSSymbolEntry::readVersion(chunkReader) == false)
      {
         {setBlogError(4471); blogerror("BXSData::load -- failed to load BXSSymbolEntry version.");}
         return(false);
      }
      if (BXSSymbolTable::readVersion(chunkReader) == false)
      {
         {setBlogError(4472); blogerror("BXSData::load -- failed to load BXSSymbolTable version.");}
         return(false);
      }
      if (BXSSyscallEntry::readVersion(chunkReader) == false)
      {
         {setBlogError(4473); blogerror("BXSData::load -- failed to load BXSSyscallEntry version.");}
         return(false);
      }
   }
   else
   {
      BXSActivationRecord::setVersion((DWORD)0);
      BXSBreakpoint::setVersion((DWORD)0);
      BXSVariableEntry::setVersion((DWORD)0);
      BXSFileEntry::setVersion((DWORD)0);
      BXSFunctionEntry::setVersion((DWORD)1);
      BXSSymbolEntry::setVersion((DWORD)0);
      BXSSymbolTable::setVersion((DWORD)0);
      BXSSyscallEntry::setVersion((DWORD)0);
   }

   //DO NOT SAVE mMessenger.  It will get relinked from outside when this class is created.
   //Syscalls.
   long numberSyscalls=0;
   CHUNKREADSAFE(chunkReader, Long, numberSyscalls);
   if (mSyscalls.setNumber(numberSyscalls) == false)
   {
      {setBlogError(4474); blogerror("BXSSyscallModule::load -- failed to allocate %d mSyscalls.", numberSyscalls);}
      return(false);
   }
   for (long i=0; i < mSyscalls.getNumber(); i++)
   {
      mSyscalls[i]=new BXSSyscallEntry();
      if (mSyscalls[i] == NULL)
      {
         {setBlogError(4475); blogerror("BXSSyscallModule::load -- failed to allocate mSyscalls[i].", i);}
         return(false);
      }
      if (mSyscalls[i]->load(chunkReader) == false)
      {
         {setBlogError(4476); blogerror("BXSSyscallModule::load -- failed to load mSyscalls[i].", i);}
         return(false);
      }
   }
   //Symbols.
   if (mSymbols->load(chunkReader) == false)
   {
      {setBlogError(4477); blogerror("BXSSyscallModule::load -- failed to load mSymbols->");}
      return(false);
   }
   //Case sensitive.
   CHUNKREADSAFE(chunkReader, Bool, mCaseSensitive);
   //Add symbols.
   if (version >= 1)
   {
      CHUNKREADSAFE(chunkReader, Bool, mAddSymbols);
      CHUNKREADSAFE(chunkReader, Bool, mStoreHelp);
   }
   else
   {
      mAddSymbols=true;
      mStoreHelp=true;
   }

   //Validate our reading of the chunk.
   result=chunkReader->validateChunkRead(BCHUNKTAG("yf"));
   if (result == false)
   {
      {setBlogError(4478); blogerror("BXSSyscallModule::load -- did not read chunk properly!");}
      return(false);
   }

   return(true);
}
#endif

