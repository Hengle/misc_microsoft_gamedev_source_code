//==============================================================================
// xsdata.cpp
//
// Copyright (c) 2000-2003, Ensemble Studios
//==============================================================================

//==============================================================================
// Includes
#include "xscript.h"
#include "xsdata.h"
#ifdef _BANG
#include "chunker.h"
#endif
#include "xsmessenger.h"
#include "xssource.h"


//=============================================================================
// BXSBreakpoint::msSaveVersion
//=============================================================================
const DWORD BXSBreakpoint::msSaveVersion=0;
//=============================================================================
// BXSBreakpoint::msLoadVersion
//=============================================================================
DWORD BXSBreakpoint::msLoadVersion=0xFFFF;

#ifdef _BANG
//=============================================================================
// BXSBreakpoint::writeVersion
//=============================================================================
bool BXSBreakpoint::writeVersion(BChunkWriter *chunkWriter)
{
   if(!chunkWriter)
   {
      BASSERT(0);
      {setBlogError(4287); blogerror("BXSBreakpoint::writeVersion -- bad chunkWriter");}
      return(false);
   }
   long result=chunkWriter->writeTaggedDWORD(BCHUNKTAG("xc"), msSaveVersion);
   if(!result)
   {
      {setBlogError(4288); blogerror("BXSBreakpoint::writeVersion -- failed to READ version");}
      return(false);
   }
   return(true);
}

//=============================================================================
// BXSBreakpoint::readVersion
//=============================================================================
bool BXSBreakpoint::readVersion(BChunkReader *chunkReader)
{
   if(!chunkReader)
   {
      BASSERT(0);
      {setBlogError(4289); blogerror("BXSBreakpoint::readVersion -- bad chunkReader");}
      return(false);
   }
   long result=chunkReader->readTaggedDWORD(BCHUNKTAG("xc"), &msLoadVersion);
   if(!result)
   {
      {setBlogError(4290); blogerror("BXSBreakpoint::readVersion -- failed to read version");}
      return(false);
   }
   return(true);
}

//=============================================================================
// BXSBreakpoint::save
//=============================================================================
bool BXSBreakpoint::save(BChunkWriter *chunkWriter)
{
   //Check writer validity.
   if (chunkWriter == NULL)
   {
      BASSERT(0);
      return(false);
   }

   //Write tag.
   long mainHandle;
   long result=chunkWriter->writeTagPostSized(BCHUNKTAG("xd"), mainHandle);
   if(!result)
   {
      {setBlogError(4291); blogerror("BXSBreakpoint::save -- error writing tag.");}
      return(false);
   }

   //File ID.
   CHUNKWRITESAFE(chunkWriter, Long, mFileID);
   //Line number.
   CHUNKWRITESAFE(chunkWriter, Long, mLineNumber);

   //Finish chunk.   
   result=chunkWriter->writeSize(mainHandle);
   if(!result)
   {
      {setBlogError(4292); blogerror("BXSBreakpoint::save -- failed to write chunk size!");}
      return(false);
   }
   return(true);
}

//=============================================================================
// BXSBreakpoint::load
//=============================================================================
bool BXSBreakpoint::load(BChunkReader *chunkReader)
{
   //Check reader validity.
   if (chunkReader == NULL)
   {
      BASSERT(0);
      return(false);
   }
   //Read tag.
   long result=chunkReader->readExpectedTag(BCHUNKTAG("xd"));
   if(!result)
   {
      {setBlogError(4293); blogerror("BXSBreakpoint::load -- error reading tag.");}
      return(false);
   }

   //File ID.
   CHUNKREADSAFE(chunkReader, Long, mFileID);
   //Line number.
   CHUNKREADSAFE(chunkReader, Long, mLineNumber);

   //Validate our reading of the chunk.
   result=chunkReader->validateChunkRead(BCHUNKTAG("xd"));
   if(!result)
   {
      {setBlogError(4294); blogerror("BXSBreakpoint::load -- did not read chunk properly!");}
      return(false);
   }
   return(true);
}
#endif


//=============================================================================
// BXSEvent::msSaveVersion
//=============================================================================
const DWORD BXSEvent::msSaveVersion=0;
//=============================================================================
// BXSEvent::msLoadVersion
//=============================================================================
DWORD BXSEvent::msLoadVersion=0xFFFF;

#ifdef _BANG
//=============================================================================
// BXSEvent::writeVersion
//=============================================================================
bool BXSEvent::writeVersion(BChunkWriter *chunkWriter)
{
   if(!chunkWriter)
   {
      BASSERT(0);
      {setBlogError(4295); blogerror("BXSEvent::writeVersion -- bad chunkWriter");}
      return(false);
   }
   long result=chunkWriter->writeTaggedDWORD(BCHUNKTAG("yc"), msSaveVersion);
   if(!result)
   {
      {setBlogError(4296); blogerror("BXSEvent::writeVersion -- failed to READ version");}
      return(false);
   }
   return(true);
}

//=============================================================================
// BXSEvent::readVersion
//=============================================================================
bool BXSEvent::readVersion(BChunkReader *chunkReader)
{
   if(!chunkReader)
   {
      BASSERT(0);
      {setBlogError(4297); blogerror("BXSEvent::readVersion -- bad chunkReader");}
      return(false);
   }
   long result=chunkReader->readTaggedDWORD(BCHUNKTAG("yc"), &msLoadVersion);
   if(!result)
   {
      {setBlogError(4298); blogerror("BXSEvent::readVersion -- failed to read version");}
      return(false);
   }
   return(true);
}

//=============================================================================
// BXSEvent::save
//=============================================================================
bool BXSEvent::save(BChunkWriter *chunkWriter)
{
   //Check writer validity.
   if (chunkWriter == NULL)
   {
      BASSERT(0);
      return(false);
   }

   //Write tag.
   long mainHandle;
   long result=chunkWriter->writeTagPostSized(BCHUNKTAG("yd"), mainHandle);
   if(!result)
   {
      {setBlogError(4299); blogerror("BXSEvent::save -- error writing tag.");}
      return(false);
   }

   //Function ID.
   CHUNKWRITESAFE(chunkWriter, Long, mFunctionID);
   //Parameter.
   CHUNKWRITESAFE(chunkWriter, Long, mParameter);

   //Finish chunk.   
   result=chunkWriter->writeSize(mainHandle);
   if(!result)
   {
      {setBlogError(4300); blogerror("BXSEvent::save -- failed to write chunk size!");}
      return(false);
   }
   return(true);
}

//=============================================================================
// BXSEvent::load
//=============================================================================
bool BXSEvent::load(BChunkReader *chunkReader)
{
   //Check reader validity.
   if (chunkReader == NULL)
   {
      BASSERT(0);
      return(false);
   }
   //Read tag.
   long result=chunkReader->readExpectedTag(BCHUNKTAG("yd"));
   if(!result)
   {
      {setBlogError(4301); blogerror("BXSEvent::load -- error reading tag.");}
      return(false);
   }

   //Function ID.
   CHUNKREADSAFE(chunkReader, Long, mFunctionID);
   //Parameter.
   CHUNKREADSAFE(chunkReader, Long, mParameter);

   //Validate our reading of the chunk.
   result=chunkReader->validateChunkRead(BCHUNKTAG("yd"));
   if(!result)
   {
      {setBlogError(4302); blogerror("BXSEvent::load -- did not read chunk properly!");}
      return(false);
   }
   return(true);
}
#endif



//==============================================================================
// BXSData Static stuff
//==============================================================================
const DWORD BXSData::msSaveVersion=4;


//==============================================================================
// Array macros
//==============================================================================

// validates array parameters when setting to or getting from an array
#define VALIDATEARRAYPARAMS(type,arrayID,index,string,failReturnVal,checkIndex) \
   if(!mArrayInfos.validIndex(arrayID) || !mArrayInfos[arrayID]) \
   { \
      mMessenger->debuggerMsg("arrayID %d doesn't point to a valid array.", arrayID, string); \
      return(##failReturnVal##); \
   } \
   if(mArrayInfos[arrayID]->mArrayType != BXSVariableEntry::c##type##Variable) \
   { \
      mMessenger->debuggerMsg("arrayID %d doesn't point to a %s array.", arrayID, string); \
      return(##failReturnVal##); \
   } \
   if(!m##type##Arrays[mArrayInfos[arrayID]->mArrayID]) \
   { \
      mMessenger->debuggerMsg("array %d is invalid.", arrayID); \
      return(##failReturnVal##); \
   } \
   if(checkIndex && !m##type##Arrays[mArrayInfos[arrayID]->mArrayID]->validIndex(index)) \
   { \
      mMessenger->debuggerMsg("index %d for array %d is invalid.", index, arrayID); \
      return(##failReturnVal##); \
   }

// Creates an array of a specified type (Integer, Float, String, Bool, or Vector).
// arrayType corresponds to the specific BDynamicSimArray used (e.g., BDynamicSimLongArray).
#define CREATEARRAY(type, arrayType, size, defaultValue, name) \
   if(size < 0) \
   { \
      mMessenger->debuggerMsg("size specified as %d", size); \
      return false; \
   } \
   if(name.length() > 0) \
      for(long i = 0; i < mArrayInfos.getNumber(); i++) \
         if(!name.compare(mArrayInfos[i]->mName)) \
         { \
            mMessenger->debuggerMsg("array with that name already exists."); \
            return -1; \
         } \
   ##arrayType## *array = new (##arrayType##); \
   if(!array) \
   { \
      BASSERT(0); \
      return -1; \
   } \
   BXSArrayInfo *info = new BXSArrayInfo; \
   if(!info) \
   { \
      delete array; \
      BASSERT(0); \
      return -1; \
   } \
   info->mName = name; \
   info->mArrayID = m##type##Arrays.getNumber(); \
   info->mArrayType = BXSVariableEntry::c##type##Variable; \
   mArrayInfos.add(info); \
   array->setNumber(size); \
   array->setAll(defaultValue); \
   m##type##Arrays.add(array); \
   return(mArrayInfos.getNumber() - 1);

// dumps info for a type of array; for script debugging purposes
#define DUMPARRAYINFO(type, string) \
   blogtrace("  %s arrays: ", string); \
   for(i = 0; i < m##type##Arrays.getNumber(); i++) \
   { \
      if(!m##type##Arrays[i]) \
         continue; \
      long arrayID = -1; \
      for(j = 0; j < mArrayInfos.getNumber(); j++) \
      { \
         if(mArrayInfos[j] && \
            mArrayInfos[j]->mArrayType == BXSVariableEntry::c##type##Variable && \
            mArrayInfos[j]->mArrayID == i) \
         { \
            arrayID = j; \
            break; \
         } \
      } \
      if(arrayID == -1) \
         continue; \
      blogtrace("    \"%s\", id %d, size %d:", BStrConv::toA(mArrayInfos[arrayID]->mName), arrayID, m##type##Arrays[i]->getNumber()); \
      for(k = 0; k < m##type##Arrays.get(i)->getNumber(); k++) \
      { \
         dump##type##ArrayEntry(i, k); \
      } \
   }
   
//==============================================================================
// BXSData::BXSData
//==============================================================================
BXSData::BXSData(long id, const BSimString &name, BXSMessenger *messenger, BXSSource *source) :
   mID(id),
   mName(name),
   mMessenger(messenger),
   mSource(source),
   //mFunctions doesn't need any ctor args.
   //mVariables doesn't need any ctor args.
   //mExportedVariables doesn't need any ctor args.
   //mUserClasses doesn't need any ctor args.
   //mEvents doesn't need any ctor args.
   //mStack doesn't need any ctor args.
   //mHeap doesn't need any ctor args.
   mInterpretCounter(0),
   mMainFunctionID(-1),
   //mCallStack doesn't need any ctor args.
   mCurrentRuleID(-1),
   mInfiniteLoopPCJumpValue(-1),
   mInfiniteLoopPCJumpCount(0),
   mInfiniteLoopLimit(-1),
   mInfiniteRecursionLimit(-1),
   //mBreakpoints doesn't need any ctor args.
   mBreakpointGo(cBreakpointGoNot),
   mBreakNow(false),
   mCurrentTime((DWORD)0),
   mTempReturnValue(NULL),
   mTempReturnValueSize(0),
   mTempReturnValueMaxSize(0),
   //mUserHeap doesn't need any ctor args.
   mUnitID(-1),
   //mOutput doesn't need any ctor args.
   //mOutputColors doesn't need any ctor args.
   mOutputLineNumber(2000),
   mOutputChanged(false),
   mReadyToExecute(false),
   mInUse(true)
{
   setCallingString(NULL);
}

//==============================================================================
// BXSData::~BXSData
//==============================================================================
BXSData::~BXSData(void)
{
   cleanUp(true);
}

//==============================================================================
// BXSData::getFileEntryByCallStack
//==============================================================================
BXSFileEntry* BXSData::getFileEntryByCallStack(void) const
{
   if (mCallStack.getNumber() <= 0)
      return(NULL);

   long functionID=mCallStack[mCallStack.getNumber()-1].getFunctionID();
   BXSFunctionEntry *functionEntry=getFunctionEntry(functionID);
   if (functionEntry == NULL)
      return(NULL);
   return(mSource->getFileEntry(functionEntry->getFileID()) );
}

//==============================================================================
// BXSData::getFileIDByCallStack
//==============================================================================
long BXSData::getFileIDByCallStack(void) const
{
   if (mCallStack.getNumber() <= 0)
      return(NULL);

   long functionID=mCallStack[mCallStack.getNumber()-1].getFunctionID();
   BXSFunctionEntry *functionEntry=getFunctionEntry(functionID);
   if (functionEntry == NULL)
      return(NULL);
   return(functionEntry->getFileID());
}

//==============================================================================
// BXSData::getVariableEntry
//==============================================================================
BXSVariableEntry* BXSData::getVariableEntry(long id) const
{
   if ((id < 0) || (id >= mVariables.getNumber()) )
      return(NULL);
   return(mVariables[id]);
}

//==============================================================================
// BXSData::addVariableEntry
//==============================================================================
long BXSData::addVariableEntry(BXSVariableEntry *newVE)
{
   long newID=mVariables.getNumber();
   //Allocate space.
   if (mVariables.setNumber(newID+1) == false)
      return(-1);
   //Save.
   mVariables[newID]=newVE;
   return(newID);
}

//==============================================================================
// BXSData::getExportedVariableID
//==============================================================================
long BXSData::getExportedVariableID(long index) const
{
   if ((index < 0) || (index >= mExportedVariables.getNumber()) )
      return(-1);
   return(mExportedVariables[index]);
}

//==============================================================================
// BXSData::allocateFunctionEntry
//==============================================================================
BXSFunctionEntry* BXSData::allocateFunctionEntry(void)
{
   //Allocate.
   BXSFunctionEntry *newFE=new BXSFunctionEntry();
   if (newFE == NULL)
      return(NULL);
   //Store.
   if (mFunctions.add(newFE) < 0)
      return(NULL);
   //Set the ID.
   newFE->setID(mFunctions.getNumber()-1);
   return(newFE);
}

//==============================================================================
// BXSData::getFunctionEntry
//==============================================================================
BXSFunctionEntry* BXSData::getFunctionEntry(long id) const
{
   if ((id < 0) || (id >= mFunctions.getNumber()) )
      return(NULL);
   return(mFunctions[id]);
}

//==============================================================================
// BXSData::getFunctionID
//==============================================================================
long BXSData::getFunctionID(const char *name) const
{
   return(getSymbolValue(name, BXSBinary::cFunction));   
}

//==============================================================================
// BXSData::allocateUserClassEntry
//==============================================================================
BXSUserClassEntry* BXSData::allocateUserClassEntry(void)
{
   //Allocate.
   BXSUserClassEntry *newUCE=new BXSUserClassEntry();
   if (newUCE == NULL)
      return(NULL);
   //Store.
   if (mUserClasses.add(newUCE) < 0)
      return(NULL);
   //Set the ID.
   newUCE->setID(mUserClasses.getNumber()-1);
   return(newUCE);
}

//==============================================================================
// BXSData::getUserClassEntry
//==============================================================================
BXSUserClassEntry* BXSData::getUserClassEntry(long id) const
{
   if ((id < 0) || (id >= mUserClasses.getNumber()) )
      return(NULL);
   return(mUserClasses[id]);
}

//==============================================================================
// BXSData::getUserClassID
//==============================================================================
long BXSData::getUserClassID(const char *name) const
{
   return(getSymbolValue(name, BXSBinary::cClass));   
}

//==============================================================================
// BXSData::arrayCreateInt
//==============================================================================
long BXSData::arrayCreateInt(long size, long defaultValue, const BSimString &name)
{
   CREATEARRAY(Integer, BDynamicSimLongArray, size, defaultValue, name);
}

//==============================================================================
// BXSData::arraySetInt
//==============================================================================
bool BXSData::arraySetInt(long arrayID, long index, long value)
{
   VALIDATEARRAYPARAMS(Integer, arrayID, index, "integer", false, true);
   mIntegerArrays[mArrayInfos[arrayID]->mArrayID]->setAt(index, value);
   return true;
}

//==============================================================================
// BXSData::arrayGetInt
//==============================================================================
long BXSData::arrayGetInt(long arrayID, long index)
{
   VALIDATEARRAYPARAMS(Integer, arrayID, index, "integer", -1, true);
   return(mIntegerArrays[mArrayInfos[arrayID]->mArrayID]->get(index));
}


//==============================================================================
// BXSData::arrayResizeInt
//==============================================================================
bool BXSData::arrayResizeInt(long arrayID, long newSize)
{
   if(newSize < 0)
      return false;

   VALIDATEARRAYPARAMS(Integer, arrayID, 0, "integer", false, false);
   return(mIntegerArrays[mArrayInfos[arrayID]->mArrayID]->setNumber(newSize));
}


//==============================================================================
// BXSData::arrayCreateFloat
//==============================================================================
long BXSData::arrayCreateFloat(long size, float defaultValue, const BSimString &name)
{
   CREATEARRAY(Float, BDynamicSimFloatArray, size, defaultValue, name);
}

//==============================================================================
// BXSData::arraySetFloat
//==============================================================================
bool BXSData::arraySetFloat(long arrayID, long index, float value)
{
   VALIDATEARRAYPARAMS(Float, arrayID, index, "float", false, true);
   mFloatArrays[mArrayInfos[arrayID]->mArrayID]->setAt(index, value);
   return true;
}

//==============================================================================
// BXSData::arrayGetFloat
//==============================================================================
float BXSData::arrayGetFloat(long arrayID, long index)
{
   VALIDATEARRAYPARAMS(Float, arrayID, index, "float", -1.0f, true);
   return(mFloatArrays[mArrayInfos[arrayID]->mArrayID]->get(index));
}

//==============================================================================
// BXSData::arrayCreateBool
//==============================================================================
long BXSData::arrayCreateBool(long size, bool defaultValue, const BSimString &name)
{
   CREATEARRAY(Bool, BDynamicSimArray<bool>, size, defaultValue, name);
}

//==============================================================================
// BXSData::arraySetBool
//==============================================================================
bool BXSData::arraySetBool(long arrayID, long index, bool value)
{
   VALIDATEARRAYPARAMS(Bool, arrayID, index, "boolean", false, true);
   mBoolArrays[mArrayInfos[arrayID]->mArrayID]->setAt(index, value);
   return true;
}

//==============================================================================
// BXSData::arrayGetBool
//==============================================================================
bool BXSData::arrayGetBool(long arrayID, long index)
{
   VALIDATEARRAYPARAMS(Bool, arrayID, index, "boolean", false, true);
   return(mBoolArrays[mArrayInfos[arrayID]->mArrayID]->get(index));
}

//==============================================================================
// BXSData::arrayCreateString
//==============================================================================
long BXSData::arrayCreateString(long size, const BSimString& defaultValue, const BSimString &name)
{
   CREATEARRAY(String, BDynamicSimArray<BSimString>, size, defaultValue, name);
}

//==============================================================================
// BXSData::arraySetString
//==============================================================================
bool BXSData::arraySetString(long arrayID, long index, const BSimString& value)
{
   VALIDATEARRAYPARAMS(String, arrayID, index, "string", false, true);
   mStringArrays[mArrayInfos[arrayID]->mArrayID]->setAt(index, value);
   return true;
}

//==============================================================================
// BXSData::arrayGetString
//==============================================================================
const char* BXSData::arrayGetString(long arrayID, long index)
{
   VALIDATEARRAYPARAMS(String, arrayID, index, "string", "", true);
   return(mStringArrays[mArrayInfos[arrayID]->mArrayID]->get(index).getPtr());
}

//==============================================================================
// BXSData::arrayCreateVector
//==============================================================================
long BXSData::arrayCreateVector(long size, const BVector& defaultValue, const BSimString &name)
{
   CREATEARRAY(Vector, BDynamicSimVectorArray, size, defaultValue, name);
}

//==============================================================================
// BXSData::arraySetVector
//==============================================================================
bool BXSData::arraySetVector(long arrayID, long index, const BVector& value)
{
   VALIDATEARRAYPARAMS(Vector, arrayID, index, "Vector", false, true);
   mVectorArrays[mArrayInfos[arrayID]->mArrayID]->setAt(index, value);
   return true;
}

//==============================================================================
// BXSData::arrayGetVector
//==============================================================================
BVector BXSData::arrayGetVector(long arrayID, long index)
{
   BVector vec;
   vec.zero();
   VALIDATEARRAYPARAMS(Vector, arrayID, index, "Vector", vec, true);
   vec = mVectorArrays[mArrayInfos[arrayID]->mArrayID]->get(index);
   return(vec);
}


//==============================================================================
// BXSData::arrayGetSize
//==============================================================================
long BXSData::arrayGetSize(long arrayID)
{
   if(!mArrayInfos.validIndex(arrayID))
      return 0; // MS 2/20/2004: per Kidd's request, returning 0

   switch(mArrayInfos.get(arrayID)->mArrayType)
   {
      case BXSVariableEntry::cIntegerVariable:
         return mIntegerArrays[mArrayInfos[arrayID]->mArrayID]->getNumber();
      case BXSVariableEntry::cFloatVariable:
         return mFloatArrays[mArrayInfos[arrayID]->mArrayID]->getNumber();
      case BXSVariableEntry::cBoolVariable:
         return mBoolArrays[mArrayInfos[arrayID]->mArrayID]->getNumber();
      case BXSVariableEntry::cStringVariable:
         return mStringArrays[mArrayInfos[arrayID]->mArrayID]->getNumber();
      case BXSVariableEntry::cVectorVariable:
         return mVectorArrays[mArrayInfos[arrayID]->mArrayID]->getNumber();
      default:
         return -1; // MS 2/20/2004: still makes sense to return -1 here if the array type is unknown
   }
}


//==============================================================================
// BXSData::dumpArrays
//==============================================================================
void BXSData::dumpArrays()
{
   blogtrace("====================================================================================");
   blogtrace("XS Arrays    ");

   long i, j, k;
   
   DUMPARRAYINFO(Integer, "Integer");
   DUMPARRAYINFO(Float,   "Float");
   DUMPARRAYINFO(Bool,    "Boolean");
   DUMPARRAYINFO(String,  "String");
   DUMPARRAYINFO(Vector,  "Vector");

   blogtrace("====================================================================================");
}


//==============================================================================
// BXSData::getEvent
//==============================================================================
BXSEvent* BXSData::getEvent(long index)
{
   if ((index < 0) || (index >= mEvents.getNumber()) )
      return(NULL);
   return(&(mEvents[index]));
}

//==============================================================================
// BXSData::addEvent
//==============================================================================
bool BXSData::addEvent(long functionID, long parameter)
{
   if (functionID < 0)
      return(false);
   if (mEvents.add(BXSEvent(functionID, parameter)) == -1)
      return(false);
   return(true);
}

//==============================================================================
// BXSData::reduceEvents
//==============================================================================
void BXSData::reduceEvents(long newNumber)
{
   //TODO: Make this smarter when I'm not so tired.
   if (newNumber <= 0)
      mEvents.setNumber(0);
   else
   {
      long numberToRemove=mEvents.getNumber()-newNumber;
      for (long i=0; i < numberToRemove; i++)
         mEvents.removeIndex(0);
   }
}

//==============================================================================
// BXSData::parseVariable
//==============================================================================
bool BXSData::parseVariable(long *value)
{
   if (parseWord(value) == false)
      return(false);
   return(true);
}

//==============================================================================
// BXSData::parseSyscallIndex
//==============================================================================
bool BXSData::parseSyscallIndex(long *value)
{
   if (parseWord(value) == false)
      return(false);

   //Check result.
   //TODO SYSCALL VERSION.
   /*if ((*value < 0) || (*value >= header->mNumberSyscalls))
   {
      *value=-1;
      return(false);
   }*/
   return(true);
}

//==============================================================================
// BXSData::parseLong
//==============================================================================
bool BXSData::parseLong(long *value)
{
   //Fail if we're going to read past the end of the code.
   long newPC=getPC()+sizeof(long);
   long codeSize=mSource->getCodeSize();
   if (newPC >= codeSize)
   {
      mMessenger->errorMsg("Error: parsing (4 bytes?) beyond end of code.");
      return(false);
   }

   //Get the value and return it.
   SBYTE *code=mSource->getCode();
   SBYTE *codePointer=code+getPC();
   long v=*((long *)codePointer);
   *value=(long)v;
   //Set our new PC.
   setPC(newPC);

   return(true);
}

//==============================================================================
// BXSData::parseWord
//==============================================================================
bool BXSData::parseWord(long *value)
{
   //Fail if we're going to read past the end of the code.
   long newPC=getPC()+sizeof(short);
   long codeSize=mSource->getCodeSize();
   if (newPC >= codeSize)
   {
      mMessenger->errorMsg("Error: parsing (2 bytes?) beyond end of code.");
      return(false);
   }

   //Get the value and return it.
   SBYTE *code=mSource->getCode();
   SBYTE *codePointer=code+getPC();
   short v=*((short *)codePointer);
   *value=(long)v;
   //Set our new PC.
   setPC(newPC);

   return(true);
}

//==============================================================================
// BXSData::parseByte
//==============================================================================
bool BXSData::parseByte(long *value)
{
   //Fail if we're going to read past the end of the code.
   long newPC=getPC()+sizeof(BYTE);
   long codeSize=mSource->getCodeSize();
   if (newPC >= codeSize)
   {
      mMessenger->errorMsg("Error: parsing (1 bytes?) beyond end of code.");
      return(false);
   }

   //Get the value and return it.
   SBYTE *code=mSource->getCode();
   SBYTE *codePointer=code+getPC();
   BYTE v=*((BYTE *)codePointer);
   *value=(long)v;
   //Set our new PC.
   setPC(newPC);

   return(true);
}

//==============================================================================
// BXSData::initializeHeap
//==============================================================================
bool BXSData::initializeHeap(void)
{
   //"Element" format:
   //  long    Type
   //  long    Data Length
   //  data (variable sized, but won't change once allocated)

   //Add all of the global variables.
   for (long i=0; i < getNumberVariables(); i++)
   {
      const BXSVariableEntry* ve=getVariableEntry(i);
      pushVariableOn(ve);
   }

   return(true);
}

//==============================================================================
// BXSData::deinitializeHeap
//==============================================================================
bool BXSData::deinitializeHeap(void)
{
   //"Element" format:
   //  long    Type
   //  long    Data Length
   //  data (variable sized, but won't change once allocated)

   //Remove all of the global variables and stuff their values back into their
   //"persistent" storage.
   for (long i=getNumberVariables()-1; i >= 0; i--)
   {
      //Get the variable entry.
      BXSVariableEntry* ve=getVariableEntry(i);
      if (ve == NULL)
         return(false);

      //Get the start of the heap index.
      long heapIndex=mStack[mStack.getNumber()-1];
      //Pop the stack.
      mStack.setNumber(mStack.getNumber()-1);

      //Get the size of the variable that we're popping off.
      //long variableSize=*((long*)(mHeap+heapIndex+BXSData::cHeapLengthOffset));
      //Get the data pointer.
      BYTE *variableData=(mHeap.getPtr()+heapIndex+BXSData::cHeapDataOffset);

      //"Save" the variable's value.
      //ve->setData(variableSize, variableData, true);
      ve->setData(variableData);

      //Knock this var off of the heap.
      mHeap.setNumber(heapIndex);
   }

   //Check to make sure that the stack and heap is fully cleaned up.
   if (mStack.getNumber() > 0)
   {
      //BASSERT(0);
      mStack.setNumber(0);
   }
   if (mHeap.getNumber() > 0)
   {
      //BASSERT(0);
      mHeap.setNumber(0);
   }

   //Clean out any active callstacks.
   mCallStack.setNumber(0);

   return(true);
}

//==============================================================================
// BXSData::pushFunctionOn
//==============================================================================
bool BXSData::pushFunctionOn(const BXSFunctionEntry *fe, long oldPC, long oldStack, long oldHeap)
{
   //Bomb check.
   if (fe == NULL)
      return(false);

   //Push all of the variables.
   for (long i=fe->getNumberParameters(); i < fe->getNumberVariables(); i++)
      pushVariableOn(fe->getVariable(i));

   //Add the activation record.
   long n=mCallStack.getNumber();
   if (mCallStack.setNumber(n+1) == false)
      return(false);
   mCallStack[n].setFunctionID(fe->getID());
   mCallStack[n].setPC(oldPC);
   mCallStack[n].setStack(oldStack);
   mCallStack[n].setHeap(oldHeap);

   return(true);
}

//==============================================================================
// BXSData::popFunctionOff
//==============================================================================
bool BXSData::popFunctionOff(void)
{
   //Make sure we have something to pop off.
   long csi=mCallStack.getNumber()-1;
   if (csi < 0)
   {
      BASSERT(0);
      return(false);
   }
   //Get the function entry for the top of the stack.
   long functionID=mCallStack[csi].getFunctionID();
   BXSFunctionEntry *fe=getFunctionEntry(functionID);
   if (fe == NULL)
   {
      BASSERT(0);
      return(false);
   }

   //If this function has a return value, just pop it off.  No need to do anything with
   //the actual value.
   if (fe->getReturnType() != BXSVariableEntry::cVoidVariable)
   {
      long returnValueIndex=mStack[mStack.getNumber()-1];
      long returnValueSize=BXSData::cHeapOverheadSize+ *(long*)(mHeap.getPtr()+returnValueIndex+BXSData::cHeapLengthOffset);
      mStack.setNumber(mStack.getNumber()-1);
      mHeap.setNumber(mHeap.getNumber()-returnValueSize);
   }

   //Now, we go through the function's variables (but NOT the parms) to see if any
   //of these variables are static.  If so, we stuff them back into the function's
   //initial value slot for this variable.  Yes, this is a goofatron system for
   //static variables, but it was easier to implement given the way everything else
   //around the variables worked (than a more standard secret global system).
   //for (long i=fe->getNumberVariables()-1, j=1; i >= fe->getNumberParameters(); i--, j++)
   for (long i=fe->getNumberVariables()-1, j=1; i >= 0; i--, j++)
   {
      //Get the address.
      long heapIndex=mStack[mStack.getNumber()-j];

      //If this is a user heap variable, decrement the ref count in the user heap.
      long variableType=*(long*)(mHeap.getPtr()+heapIndex+BXSData::cHeapTypeOffset);
      if (variableType & (BXSVariableEntry::cStringVariable|BXSVariableEntry::cUserClassVariable))
      {
         long userHeapIndex=*(long*)(mHeap.getPtr()+heapIndex+BXSData::cHeapDataOffset);
         if (decrementUserHeapRefCount(userHeapIndex) == false)
            return(false);
      }

      //Do the old static variable logic.
      if (i >= fe->getNumberParameters())
      {
         //Get the variable entry.
         BXSVariableEntry* ve=fe->getVariable(i);
         if (ve == NULL)
         {
            BASSERT(0);
            continue;
         }
         //Skip this one if it's not a static variable.
         if (ve->getStatic() == false)
            continue;
         //Get the size of the variable that we're popping off.
         //long variableSize=*((long*)(mHeap+heapIndex+BXSData::cHeapLengthOffset));
         //Get the data pointer.
         BYTE *variableData=(mHeap.getPtr()+heapIndex+BXSData::cHeapDataOffset);
         //"Save" the variable's value.
         //ve->setData(variableSize, variableData, true);
         ve->setData(variableData);
      }
   }

   // If we have a step over breakpoint, transfer it up the stack one level so we stop in the next function.
   if(mCallStack[csi].getStepOverBreakpoint() && (csi>0))
      mCallStack[csi-1].setStepOverBreakpoint(true);

   //Reset the stack and the heap to their old values (to purge any parms for
   //the function).
   mStack.setNumber(mCallStack[csi].getStack());
   mHeap.setNumber(mCallStack[csi].getHeap());

   //Pop the callstack.
   mCallStack.setNumber(csi);

   //The PC gets 'automatically' reset to its old value by just popping the AR off.
   //The code that calls popFunctionOff will take care of any additional incrementing
   //that needs to happen to jump past this call.

   return(true);
}

//==============================================================================
// BXSData::pushVariableOn
//==============================================================================
bool BXSData::pushVariableOn(const BXSVariableEntry *ve)
{
   if (ve == NULL)
      return(false);
   
   //User heap variable.
   //TODO: Cache the values like data length.
   if (ve->getType() & (BXSVariableEntry::cStringVariable|BXSVariableEntry::cUserClassVariable) )
   {
      long newValueIndex=allocateUserHeapValue(BXSVariableEntry::cStringVariable, ve->getDataLength());
      if (newValueIndex < 0)
         return(false);
      if (setUserHeapValue(newValueIndex, ve->getType(), ve->getDataLength(), ve->getData()) == false)
         return(false);
      if (incrementUserHeapRefCount(newValueIndex) == false)
         return(false);
      return(pushVariableOn(ve->getSymbolID(), ve->getType(), BXSVariableEntry::cUserHeapDataSize, &newValueIndex));
   }

   return(pushVariableOn(ve->getSymbolID(), ve->getType(), ve->getDataLength(), ve->getData() ));
}

//==============================================================================
// BXSData::pushVariableOn
//==============================================================================
bool BXSData::pushVariableOn(long symbolID, long type, long dataLength, void *data)
{
   //Push the heap index onto the stack.
   if (mStack.add(mHeap.getNumber()) == -1)
      return(false);

   //SymbolID.
   if (mHeap.add((BYTE*)&(symbolID), sizeof(long)) == false)
      return(false);
   //Type.
   if (mHeap.add((BYTE*)&(type), sizeof(long)) == false)
      return(false);
   //Length.
   if (mHeap.add((BYTE*)&(dataLength), sizeof(long)) == false)
      return(false);
   //Value.
   if (mHeap.add((BYTE*)data, dataLength) == false)
      return(false);

   return(true);
}

//==============================================================================
// BXSData::pushVariableOn
//==============================================================================
bool BXSData::pushVariableOn(long heapIndex)
{
   //User heap variable.  Make a copy.
   long variableType=*(long*)(mHeap.getPtr()+heapIndex+BXSData::cHeapTypeOffset);
   if (variableType & (BXSVariableEntry::cStringVariable|BXSVariableEntry::cUserClassVariable) )
   {
      long variableSymbol=*(long*)(mHeap.getPtr()+heapIndex);
      long variableLength=*(long*)(mHeap.getPtr()+heapIndex+cHeapLengthOffset);
      long userHeapIndex=*(long*)(mHeap.getPtr()+heapIndex+BXSData::cHeapDataOffset);
      //Make a copy of this variable in the user heap.
      long newUserHeapIndex=copyUserHeapValue(userHeapIndex);
      if (newUserHeapIndex < 0)
         return(false);
      if (incrementUserHeapRefCount(newUserHeapIndex) == false)
         return(false);

      pushVariableOn(variableSymbol, variableType, variableLength, &newUserHeapIndex);
      return(true);
   }

   long heapSize=mHeap.getNumber();
   //Push the heap index onto the stack.
   if (mStack.add(heapSize) == -1)
      return(false);

   //This pushes a full copy of the entry at the given heap index.
   long copySize=*(long*)(mHeap.getPtr()+heapIndex+BXSData::cHeapLengthOffset)+BXSData::cHeapDataOffset;
   //Push it.
   if (mHeap.setNumber(heapSize+copySize) == false)
      return(false);
   mHeap.copyPointerInto(mHeap.getPtr()+heapIndex, heapSize, copySize);

   return(true);
}

//==============================================================================
// BXSData::popVariableOff
//==============================================================================
bool BXSData::popVariableOff(void)
{
   //Get the start of the heap index.
   long heapIndex=mStack[mStack.getNumber()-1];

   //Pop the stack.
   mStack.setNumber(mStack.getNumber()-1);

   #ifdef _DEBUG
   //Get the size of the variable that we're popping off.
   long variableSize=*((long *)(mHeap.getPtr()+heapIndex+BXSData::cHeapLengthOffset));
   variableSize+=BXSData::cHeapDataOffset;
   if (variableSize != mHeap.getNumber()-heapIndex)
   {
      BASSERT(0);
      mMessenger->runMsg("INVALID VARIABLE SIZE ON STACK (%d vs %d).", variableSize, mHeap.getNumber()-heapIndex);
   }
   #endif

   //Get the variable type.  Decrement the user heap ref count if needed.
   long variableType=*(long*)(mHeap.getPtr()+heapIndex+BXSData::cHeapTypeOffset);
   if (variableType & (BXSVariableEntry::cStringVariable|BXSVariableEntry::cUserClassVariable) )
   {
      long userHeapIndex=*(long*)(mHeap.getPtr()+heapIndex+BXSData::cHeapDataOffset);
      if (decrementUserHeapRefCount(userHeapIndex) == false)
         return(false);
   }

   //Knock this var off of the heap.
   mHeap.setNumber(heapIndex);

   return(true);
}

//==============================================================================
// BXSData::getPC
//==============================================================================
long BXSData::getPC(void) const
{
   if (mCallStack.getNumber() <= 0)
      return(-1);
   return(mCallStack[mCallStack.getNumber()-1].getPC());
}

//==============================================================================
// BXSData::setPC
//==============================================================================
bool BXSData::setPC(long v)
{
   if (mCallStack.getNumber() <= 0)
      return(false);
   mCallStack[mCallStack.getNumber()-1].setPC(v);
   return(true);
}

//==============================================================================
// BXSData::incrementPC
//==============================================================================
bool BXSData::incrementPC(long v)
{
   if (mCallStack.getNumber() <= 0)
      return(false);
   mCallStack[mCallStack.getNumber()-1].incrementPC(v);
   return(true);
}

//==============================================================================
// BXSData::setBreakpoint
//==============================================================================
bool BXSData::setBreakpoint(const char *filename, long lineNumber, bool on)
{
   //Bomb check.
   if (filename == NULL)
      return(false);

   //Get the file ID.
   long fileID=getSymbolValue(filename, BXSBinary::cFilename);
   if (fileID == -1)
      return(false);

   return(setBreakpoint(fileID, lineNumber, on));
}

//==============================================================================
// BXSData::setBreakpoint
//==============================================================================
bool BXSData::setBreakpoint(long fileID, long lineNumber, bool on)
{
   //Bomb check.
   if ((fileID < 0) || (fileID >= mSource->getNumberFiles()) )
      return(false);

   //If we're removing it, do that.
   if (on == false)
      return(mBreakpoints.remove(BXSBreakpoint(fileID, lineNumber)) );

   //Else, add it.  No dupes.
   for (long i=0; i < mBreakpoints.getNumber(); i++)
   {
      if ((mBreakpoints[i].getFileID() == fileID) && (mBreakpoints[i].getLineNumber() == lineNumber))
         return(true);
   }
   if (mBreakpoints.add(BXSBreakpoint(fileID, lineNumber)) == -1)
      return(false);
   return(true);
}

//==============================================================================
// BXSData::checkBreakpoints
//==============================================================================
bool BXSData::checkBreakpoints(void)
{
   //Skip if we're not tracking this.
   if (mBreakpoints.getNumber() <= 0)
      return(false);
   long currentLineNumber=getCurrentLineNumber();

   //See if the current file ID and line number match any of the breakpoints that
   //we have set.  If they match, return TRUE.  Else, return FALSE.
   BXSFileEntry *fileEntry=getFileEntryByCallStack();
   if (fileEntry == NULL)
      return(false);
   long fileID=fileEntry->getID();
   for (long i=0; i < mBreakpoints.getNumber(); i++)
   {
      if ((mBreakpoints[i].getFileID() == fileID) && (mBreakpoints[i].getLineNumber() == currentLineNumber))
         return(true);
   }

   return(false);
}

//==============================================================================
// BXSData::getBreakpoint
//==============================================================================
bool BXSData::getBreakpoint(void) const
{
   if (mCallStack.getNumber() <= 0)
      return(false);
   return(mCallStack[mCallStack.getNumber()-1].getBreakpoint());
}

//==============================================================================
// BXSData::setBreakpoint
//==============================================================================
void BXSData::setBreakpoint(bool v)
{
   if (mCallStack.getNumber() <= 0)
      return;
   mCallStack[mCallStack.getNumber()-1].setBreakpoint(v);
}

//==============================================================================
// BXSData::setFunctionStartBreakpoint
//==============================================================================
bool BXSData::setFunctionStartBreakpoint(long functionID, bool on)
{
   // Get the function entry.
   BXSFunctionEntry *mainEntry=getFunctionEntry(functionID);
   if(!mainEntry) 
      return(false);

   // Get the file ID.
   long fileID=mainEntry->getFileID();

   // Get line number.
   long lineNumber=mainEntry->getLineNumber();

   // Set the breakpoint.
   bool ok=setBreakpoint(fileID, lineNumber, on);
   return(ok);
}

//==============================================================================
// BXSData::setStepOverBreakpoint
//==============================================================================
bool BXSData::setStepOverBreakpoint(void)
{
   // Get top of callstack.
   long csi=mCallStack.getNumber()-1;

   // Can't do anything if no callstack.
   if(csi<0)
      return(false);

   // Set up to get a breakpoint next time we pop back to this level.
   mCallStack[csi].setStepOverBreakpoint(true);
   return(true);
}

//==============================================================================
// BXSData::hasBreakpoint
//==============================================================================
bool BXSData::hasBreakpoint(long fileID, long lineNumber) const
{
   for (long i=0; i < mBreakpoints.getNumber(); i++)
   {
      if ((mBreakpoints[i].getFileID() == fileID) && (mBreakpoints[i].getLineNumber() == lineNumber))
         return(true);
   }
   return(false);
}

//==============================================================================
// BXSData::checkAndClearStepOverBreakpoint
//==============================================================================
bool BXSData::checkAndClearStepOverBreakpoint(void)
{
   // Get top of callstack.
   long csi=mCallStack.getNumber()-1;

   // Can't do anything if no callstack.
   if(csi<0)
      return(false);

   // Got one?
   bool breakpt=mCallStack[csi].getStepOverBreakpoint();

   // Clear.
   mCallStack[csi].setStepOverBreakpoint(false);

   // Return result.
   return(breakpt);
}

//==============================================================================
// BXSData::checkAndClearBreakNow
//==============================================================================
bool BXSData::checkAndClearBreakNow(void)
{
   // Break now?   
   bool breakNow=mBreakNow;

   // Clear.
   mBreakNow=false;

   // Return result.
   return(breakNow);
}

//==============================================================================
// BXSData::setCallingString
//==============================================================================
void BXSData::setCallingString(const char *text)
{
   if (text == NULL)
      StringCchCopyA(mCallingString, 512, "None");
   else
      StringCchCopyNExA(mCallingString, 512, text, 511, NULL, NULL, STRSAFE_FILL_BEHIND_NULL);
}

//==============================================================================
// BXSData::resetAll
//==============================================================================
void BXSData::resetAll(void)
{
   resetStackAndHeap();
   resetFailsafes();
}

//==============================================================================
// BXSData::resetStackAndHeap
//==============================================================================
void BXSData::resetStackAndHeap(void)
{
   mStack.setNumber(0);
   mHeap.setNumber(0);
}

//==============================================================================
// BXSData::resetFailsafes
//==============================================================================
void BXSData::resetFailsafes(void)
{
   mInfiniteLoopPCJumpValue=-1;
   mInfiniteLoopPCJumpCount=0;
   mInfiniteLoopLimit=-1;
   mInfiniteRecursionLimit=-1;
}

//==============================================================================
// BXSData::getCurrentFileID
//==============================================================================
long BXSData::getCurrentFileID(void) const
{
   long n=mCallStack.getNumber()-1;
   if (n < 0)
      return(-1);
   BXSFunctionEntry *fe=getFunctionEntry(mCallStack[n].getFunctionID());
   if (fe == NULL)
      return(-1);
   return(fe->getFileID());
}

//==============================================================================
// BXSData::getCurrentFunctionID
//==============================================================================
long BXSData::getCurrentFunctionID(void) const
{
   long n=mCallStack.getNumber()-1;
   if (n < 0)
      return(-1);
   return(mCallStack[n].getFunctionID());
}

//==============================================================================
// BXSData::getCurrentLineNumber
//==============================================================================
long BXSData::getCurrentLineNumber(void) const
{
   long n=mCallStack.getNumber()-1;
   if (n < 0)
      return(-1);
   return(mCallStack[n].getLineNumber());
}

//==============================================================================
// BXSData::setCurrentLineNumber
//==============================================================================
void BXSData::setCurrentLineNumber(long v)
{
   long n=mCallStack.getNumber()-1;
   if (n >= 0)
      mCallStack[n].setLineNumber(v);
}

//==============================================================================
// BXSData::getTopStackStart
//==============================================================================
long BXSData::getTopStackStart(void)
{
   if (mCallStack.getNumber() <= 0)
      return(-1);
   return(mCallStack[mCallStack.getNumber()-1].getStack());
}

//==============================================================================
// BXSData::resetRuleTime
//==============================================================================
void BXSData::resetRuleTime(DWORD currentTime)
{
   long count = mRules.getNumber();
   for (long idx=0; idx<count; idx++)
   {
      mRules[idx]->setLastExecuteTime(currentTime);
   }
}

//==============================================================================
// BXSData::allocateTempReturnValue
//==============================================================================
bool BXSData::allocateTempReturnValue(long size)
{
   //Zero out whatever we have now.
   mTempReturnValueSize=0;

   //If we have a valid size, just be done.
   if (mTempReturnValueMaxSize >= size)
      return(true);

   //Else, we have to allocate new memory.
   if (mTempReturnValue != NULL)
   {
      delete [] mTempReturnValue;
      mTempReturnValue=NULL;
   }
   mTempReturnValue=new BYTE[size];
   if (mTempReturnValue == NULL)
      return(false);
   mTempReturnValueMaxSize=size;

   return(true);
}  

//==============================================================================
// BXSData::setTempReturnValue
//==============================================================================
bool BXSData::setTempReturnValue(BYTE *v, long size)
{
   if (v == NULL)
      return(false);

   //Get enough space.
   if (allocateTempReturnValue(size) == false)
      return(false);

   memcpy(mTempReturnValue, v, size);
   mTempReturnValueSize=size;
   return(true);
}  

//==============================================================================
// BXSData::getUserHeapEntry
//==============================================================================
BYTE* BXSData::getUserHeapEntry(long index) const
{
   if ((index < 0) || (index >= mUserHeap.getNumber()) )
      return(NULL);
   return(mUserHeap[index]);
}

//==============================================================================
// BXSData::getUserHeapValue
//==============================================================================
BYTE* BXSData::getUserHeapValue(long index) const
{
   //Validate.
   if ((index < 0) || (index >= mUserHeap.getNumber()) )
      return(NULL);
   if (mUserHeap[index] == NULL)
      return(NULL);
   //Return just the value part.
   BYTE *rVal=mUserHeap[index];
   rVal+=cUserHeapValueOffset;
   return(rVal);
}

//==============================================================================
// BXSData::setUserHeapValue
//==============================================================================
bool BXSData::setUserHeapValue(long index, long type, long newSize, BYTE *newValue)
{
   //Check for invalid index.
   if ((index < 0) || (index >= mUserHeap.getNumber()) )
      return(false);
   BASSERT(newSize > 0);

   //Get everything out of the old space (if there is one).
   if (mUserHeap[index] != NULL)
   {
      BYTE *oldEntry=mUserHeap[index];
      long oldType=*(long*)(oldEntry+cUserHeapTypeOffset);
      long oldMaxSize=*(long*)(oldEntry+cUserHeapMaxSizeOffset);

      //Fail if types don't match.
      if ((oldType != type) && (oldType >= 0))
         return(false);

      //If we have enough size, just dump it and be done.
      if (oldMaxSize >= newSize)
      {
         memcpy(oldEntry, &type, sizeof(long));
         memcpy(oldEntry+cUserHeapSizeOffset, &newSize, sizeof(long));
         memcpy(oldEntry+cUserHeapValueOffset, newValue, newSize);
         return(true);
      }
      //Else, delete it (so we can reallocate later).
      delete oldEntry;
   }

   //Allocate actual size, plus space for type and size storage.
   long realNewSize=newSize+cUserHeapOverheadSize;
   BYTE *realNewValue=new BYTE[realNewSize];
   if (realNewValue == NULL)
      return(false);
   memcpy(realNewValue, &type, sizeof(long));
   memcpy(realNewValue+cUserHeapSizeOffset, &newSize, sizeof(long));
   memcpy(realNewValue+cUserHeapMaxSizeOffset, &newSize, sizeof(long));
   memcpy(realNewValue+cUserHeapValueOffset, newValue, newSize);

   //Save it.
   mUserHeap[index]=realNewValue;

   return(true);
}

//==============================================================================
// BXSData::incrementUserHeapRefCount
//==============================================================================
bool BXSData::incrementUserHeapRefCount(long index)
{
   //Validate.
   if ((index < 0) || (index >= mUserHeap.getNumber()) )
      return(false);
   if (mUserHeap[index] == NULL)
      return(false);

   //Get the old count.
   long refCount=*(long*)(mUserHeap[index]+cUserHeapRefCountOffset);
   refCount++;
   memcpy(mUserHeap[index]+cUserHeapRefCountOffset, &refCount, sizeof(long));

   return(true);
}

//==============================================================================
// BXSData::decrementUserHeapRefCount
//==============================================================================
bool BXSData::decrementUserHeapRefCount(long index)
{
   //Validate.
   if ((index < 0) || (index >= mUserHeap.getNumber()) )
      return(false);
   if (mUserHeap[index] == NULL)
      return(false);

   //Get the old count.
   long refCount=*(long*)(mUserHeap[index]+cUserHeapRefCountOffset);
   refCount--;
   if (refCount < 0)
      refCount=0;
   memcpy(mUserHeap[index]+cUserHeapRefCountOffset, &refCount, sizeof(long));

   return(true);
}

//==============================================================================
// BXSData::allocateUserHeapValue
//==============================================================================
long BXSData::allocateUserHeapValue(long type, long size)
{
   //TODO: This could be a *lot* smarter with some more work.  Things like
   //actual memory management, garbage collection, and a not-rock-stupid-take-
   //the-first-free-slot approach would be nice.

   //Don't allocate invalid length stuff.
   if (size < 0)
      return(-1);

   //Get the new index.
   long newIndex=-1;
   bool allocate=false;
   //See if we have an old index we can reuse.
   for (long i=0; i < mUserHeap.getNumber(); i++)
   {
      //Get the ref count for this one.
      long refCount=*(long*)(mUserHeap[i]+cUserHeapRefCountOffset);
      if (refCount > 0)
         continue;

      newIndex=i;
      long maxSize=*(long*)(mUserHeap[i]+cUserHeapMaxSizeOffset);
      if (maxSize >= size)
      {
         //Zero out new value for happy debugging.
         memset(mUserHeap[i], 0, sizeof(BYTE)*maxSize+cUserHeapOverheadSize);
         //Set the type.
         memcpy(mUserHeap[i]+cUserHeapTypeOffset, &type, sizeof(long));
         //Set the maximum size.
         memcpy(mUserHeap[i]+cUserHeapMaxSizeOffset, &maxSize, sizeof(long));
      }
      else
      {
         delete mUserHeap[i];
         mUserHeap[i]=NULL;
         allocate=true;
      }
      break;
   }
   //Make a slot for this if we didn't have any free.
   if (newIndex == -1)
   {
      newIndex=mUserHeap.getNumber();
      if (mUserHeap.setNumber(newIndex+1) == false)
         return(-1);
      allocate=true;
   }
   //Allocate actual size, plus overhead space.
   long realSize=size+cUserHeapOverheadSize;
   if (allocate == true)
   {
      BYTE *newValue=new BYTE[realSize];
      if (newValue == NULL)
      {
         mUserHeap.setNumber(newIndex);
         return(-1);
      }
      //Save it.
      mUserHeap[newIndex]=newValue;
   }

   //Zero out new value for happy debugging.
   memset(mUserHeap[newIndex], 0, sizeof(BYTE)*realSize);
   //Set the type.
   memcpy(mUserHeap[newIndex]+cUserHeapTypeOffset, &type, sizeof(long));
   //Set the maximum size.
   memcpy(mUserHeap[newIndex]+cUserHeapMaxSizeOffset, &size, sizeof(long));

   //Return index.
   return(newIndex);
}

//=============================================================================
// BXSData::copyUserHeapValue
//=============================================================================
long BXSData::copyUserHeapValue(long index)
{
   //Check for invalid or unused index.
   if ((index < 0) || (index >= mUserHeap.getNumber()) || (mUserHeap[index] == NULL))
      return(-1);

   //Get the size.
   BYTE *originalEntry=mUserHeap[index];
   long originalType=*(long*)(originalEntry+cUserHeapTypeOffset);
   long originalSize=*(long*)(originalEntry+cUserHeapSizeOffset);

   //Allocate a new one.
   long newIndex=allocateUserHeapValue(originalType, originalSize);
   if (newIndex < 0)
      return(-1);

   //Set the value.
   if (setUserHeapValue(newIndex, originalType, originalSize, originalEntry+cUserHeapValueOffset) == false)
      return(-1);

   return(newIndex);
}

//=============================================================================
// BXSData::addOutput
//=============================================================================
void BXSData::addOutput(const BColor &color, const char* v, ... )
{
   v;
   #ifndef BUILD_FINAL
	//Store mOutputLineNumber lines.
   //TODO: If someone resets mOutputLineNumber after there is already output, this
   //obviously will not handle that.
	if (mOutput.getNumber() > mOutputLineNumber)
   {
	   mOutput.removeIndex(0);
      mOutputColors.removeIndex(0);
   }
   //Format it.
   static char out[1024];
   va_list va;
   va_start(va, v);
   bvsnprintf(out, sizeof(out), v, va);
   //Add it.
   mOutput.add(BSimString(out));
   mOutputColors.add(color);
   //Set the changed output flag.
   mOutputChanged=true;
   #endif
}

#ifdef _BANG
//=============================================================================
// BXSData::save
//=============================================================================
bool BXSData::save(BChunkWriter *chunkWriter)
{
   //Check writer validity.
   if (chunkWriter == NULL)
   {
      BASSERT(0);
      return(false);
   }

   //Write tag.
   long mainHandle;
   long result=chunkWriter->writeTagPostSized(BCHUNKTAG("XC"), mainHandle);
   if (result == false)
   {
      {setBlogError(4303); blogerror("BXSData::save -- error writing tag.");}
      return(false);
   }

   //Version.
   CHUNKWRITESAFE(chunkWriter, DWORD, msSaveVersion);

   //Base rule module save.
   if (BXSRuleModule::save(chunkWriter) == false)
   {
      {setBlogError(4304); blogerror("BXSData::save -- failed to save BXSRuleModule.");}
      return(false);
   }

   //DO NOT SAVE mMessenger.  It will get relinked from outside when this class is created.
   //DO NOT SAVE mSource.  It will get relinked from outside when this class is created.
   //ID.
   CHUNKWRITESAFE(chunkWriter, Long, mID);
   //Name.
   CHUNKWRITESAFE(chunkWriter, BSimString, mName);
   //Functions.
   CHUNKWRITESAFE(chunkWriter, Long, mFunctions.getNumber());
   for (long i=0; i < mFunctions.getNumber(); i++)
   {
      if (mFunctions[i]->save(chunkWriter, this) == false)
      {
         {setBlogError(4305); blogerror("BXSData::save -- failed to save mFunctions[%d].", i);}
         return(false);
      }
   }
   //Variables.
   CHUNKWRITESAFE(chunkWriter, Long, mVariables.getNumber());
   for (long i=0; i < mVariables.getNumber(); i++)
   {
      if (mVariables[i]->save(chunkWriter, this) == false)
      {
         {setBlogError(4306); blogerror("BXSData::save -- failed to save mVariables[%d].", i);}
         return(false);
      }
   }
   //Exported variables.
   CHUNKWRITESAFE(chunkWriter, Long, mExportedVariables.getNumber());
   CHUNKWRITEARRAYSAFE(chunkWriter, Long, mExportedVariables.getNumber(), (long*)mExportedVariables.getPtr());
   //User Classes.
   CHUNKWRITESAFE(chunkWriter, Long, mUserClasses.getNumber());
   for (long i=0; i < mUserClasses.getNumber(); i++)
   {
      if (mUserClasses[i]->save(chunkWriter, this) == false)
      {
         {setBlogError(4307); blogerror("BXSData::save -- failed to save mUserClasses[%d].", i);}
         return(false);
      }
   }
   //Events.
   CHUNKWRITESAFE(chunkWriter, Long, mEvents.getNumber());
   for (long i=0; i < mEvents.getNumber(); i++)
   {
      if (mEvents[i].save(chunkWriter) == false)
      {
         {setBlogError(4308); blogerror("BXSData::save -- failed to save mEvents[%d].", i);}
         return(false);
      }
   }
   //Stack.
   CHUNKWRITESAFE(chunkWriter, Long, mStack.getNumber());
   CHUNKWRITEARRAYSAFE(chunkWriter, Long, mStack.getNumber(), (long*)mStack.getPtr());
   //Heap.
   CHUNKWRITESAFE(chunkWriter, Long, mHeap.getNumber());
   CHUNKWRITEARRAYSAFE(chunkWriter, BYTE, mHeap.getNumber(), (BYTE*)mHeap.getPtr());
   //Interpret counter.
   CHUNKWRITESAFE(chunkWriter, Long, mInterpretCounter);
   //Main function ID.
   CHUNKWRITESAFE(chunkWriter, Long, mMainFunctionID);
   //Call stack.
   CHUNKWRITESAFE(chunkWriter, Long, mCallStack.getNumber());
   for (long i=0; i < mCallStack.getNumber(); i++)
   {
      if (mCallStack[i].save(chunkWriter) == false)
      {
         {setBlogError(4309); blogerror("BXSData::save -- failed to save mCallStack[%d].", i);}
         return(false);
      }
   }
   //Current rule ID.
   CHUNKWRITESAFE(chunkWriter, Long, mCurrentRuleID);
   //Failsafes.
   CHUNKWRITESAFE(chunkWriter, Long, mInfiniteLoopPCJumpValue);
   CHUNKWRITESAFE(chunkWriter, Long, mInfiniteLoopPCJumpCount);
   CHUNKWRITESAFE(chunkWriter, Long, mInfiniteLoopLimit);
   CHUNKWRITESAFE(chunkWriter, Long, mInfiniteRecursionLimit);
   //Breakpoints.
   CHUNKWRITESAFE(chunkWriter, Long, mBreakpoints.getNumber());
   for (long i=0; i < mBreakpoints.getNumber(); i++)
   {
      if (mBreakpoints[i].save(chunkWriter) == false)
      {
         {setBlogError(4310); blogerror("BXSData::save -- failed to save mBreakpoints[%d].", i);}
         return(false);
      }
   }
   //Breakpoint Go.
   CHUNKWRITESAFE(chunkWriter, Long, mBreakpointGo);
   //Break now.
   CHUNKWRITESAFE(chunkWriter, Bool, mBreakNow);
   //Current time.
   CHUNKWRITESAFE(chunkWriter, DWORD, mCurrentTime);
   //Ready to execute.
   CHUNKWRITESAFE(chunkWriter, Bool, mReadyToExecute);

   //Output.
   bool hasOutput=true;
   #ifdef BUILD_FINAL
   hasOutput=false;
   #endif
   CHUNKWRITESAFE(chunkWriter, Bool, hasOutput);
   if (hasOutput == true)
   {
      CHUNKWRITESAFE(chunkWriter, Long, mOutput.getNumber());
      for (long i=0; i < mOutput.getNumber(); i++)
      {
         CHUNKWRITESAFE(chunkWriter, BSimString, mOutput[i]);
         if (mOutputColors[i].save(chunkWriter) == false)
         {
            {setBlogError(4311); blogerror("BXSData::save -- failed to save mOutputColors[%d].", i);}
            return(false);
         }
      }
      CHUNKWRITESAFE(chunkWriter, Long, mOutputLineNumber);
   }

   // Array saving
   // array infos
   long j = 0;
   CHUNKWRITESAFE(chunkWriter, Long, mArrayInfos.getNumber());
   for(long i = 0; i < mArrayInfos.getNumber(); i++)
   {
      chunkWriter->writeBString(mArrayInfos.get(i)->mName);
      CHUNKWRITESAFE(chunkWriter, Long, mArrayInfos.get(i)->mArrayID);
      CHUNKWRITESAFE(chunkWriter, Long, mArrayInfos.get(i)->mArrayType);
   }
   // integer arrays
   CHUNKWRITESAFE(chunkWriter, Long, mIntegerArrays.getNumber());
   for(long i = 0; i < mIntegerArrays.getNumber(); i++)
   {
      BDynamicSimLongArray *array = mIntegerArrays[i];
      if(!array)
         BASSERT(0);
      CHUNKWRITEARRAYSAFE(chunkWriter, Long, array->getNumber(), array->getPtr());
   }
   // float arrays
   CHUNKWRITESAFE(chunkWriter, Long, mFloatArrays.getNumber());
   for(long i = 0; i < mFloatArrays.getNumber(); i++)
   {
      BDynamicSimFloatArray *array = mFloatArrays[i];
      if(!array)
         BASSERT(0);
      CHUNKWRITEARRAYSAFE(chunkWriter, Float, array->getNumber(), array->getPtr());
   }
   // bool arrays
   CHUNKWRITESAFE(chunkWriter, Long, mBoolArrays.getNumber());
   for(long i = 0; i < mBoolArrays.getNumber(); i++)
   {
      BDynamicSimArray<bool> *array = mBoolArrays[i];
      if(!array)
         BASSERT(0);
      CHUNKWRITEARRAYSAFE(chunkWriter, Bool, array->getNumber(), array->getPtr());
   }
   // string arrays
   CHUNKWRITESAFE(chunkWriter, Long, mStringArrays.getNumber());
   for(long i = 0; i < mStringArrays.getNumber(); i++)
   {
      BDynamicSimArray<BSimString> *array = mStringArrays[i];
      if(!array)
         BASSERT(0);
      CHUNKWRITESAFE(chunkWriter, Long, array->getNumber());
      for(j = 0; j < array->getNumber(); j++)
         chunkWriter->writeBString(array->get(j));
   }
   // vector arrays
   CHUNKWRITESAFE(chunkWriter, Long, mVectorArrays.getNumber());
   for(long i = 0; i < mVectorArrays.getNumber(); i++)
   {
      BDynamicSimVectorArray *array = mVectorArrays[i];
      if(!array)
         BASSERT(0);
      CHUNKWRITEARRAYSAFE(chunkWriter, Vector, array->getNumber(), array->getPtr());
   }

   //User Heap.
   CHUNKWRITESAFE(chunkWriter, Long, mUserHeap.getNumber());
   for (long i=0; i < mUserHeap.getNumber(); i++)
   {
      BYTE *he=mUserHeap[i];
      long heSize=*(long*)(he+cUserHeapMaxSizeOffset);
      long realSize=heSize+cUserHeapOverheadSize;
      CHUNKWRITEARRAYSAFE(chunkWriter, BYTE, realSize, (BYTE*)he);
   }

   //UnitID.
   CHUNKWRITESAFE(chunkWriter, Long, mUnitID);

   //Finish chunk.   
   result=chunkWriter->writeSize(mainHandle);
   if (result == false)
   {
      {setBlogError(4312); blogerror("BXSData::save -- failed to write chunk size!");}
      return(false);
   }

   return(true);
}

//=============================================================================
// BXSData::load
//=============================================================================
bool BXSData::load(BChunkReader *chunkReader)
{
   //Check reader validity.
   if (chunkReader == NULL)
   {
      BASSERT(0);
      return(false);
   }

   //Read tag.
   long result=chunkReader->readExpectedTag(BCHUNKTAG("XC"));
   if (result == false)
   {
      {setBlogError(4313); blogerror("BXSData::load -- error reading tag.");}
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

   //Base rule module load.
   if (BXSRuleModule::load(chunkReader) == false)
   {
      {setBlogError(4314); blogerror("BXSData::load -- failed to load BXSRuleModule.");}
      return(false);
   }

   //DO NOT SAVE mMessenger.  It will get relinked from outside when this class is created.
   //DO NOT SAVE mSource.  It will get relinked from outside when this class is created.
   //ID.
   CHUNKREADSAFE(chunkReader, Long, mID);
   //Name.
   CHUNKREADSAFE(chunkReader, BSimString, mName);
   //Functions.
   long numberFunctions=0;
   CHUNKREADSAFE(chunkReader, Long, numberFunctions);
   if (mFunctions.setNumber(numberFunctions) == false)
   {
      {setBlogError(4315); blogerror("BXSData::load -- failed to allocate %d mFunctions.", numberFunctions);}
      return(false);
   }
   for (long i=0; i < mFunctions.getNumber(); i++)
   {
      mFunctions[i]=new BXSFunctionEntry;
      if (mFunctions[i] == NULL)
      {
         {setBlogError(4316); blogerror("BXSData::load -- failed to allocate mFunctions[%d].", i);}
         return(false);
      }
      if (mFunctions[i]->load(chunkReader, this) == false)
      {
         {setBlogError(4317); blogerror("BXSData::load -- failed to load mFunctions[%d].", i);}
         return(false);
      }
   }
   //Variables.
   long numberVariables=0;
   CHUNKREADSAFE(chunkReader, Long, numberVariables);
   if (mVariables.setNumber(numberVariables) == false)
   {
      {setBlogError(4318); blogerror("BXSData::load -- failed to allocate %d mVariables.", numberVariables);}
      return(false);
   }
   for (long i=0; i < mVariables.getNumber(); i++)
   {
      mVariables[i]=new BXSVariableEntry;
      if (mVariables[i] == NULL)
      {
         {setBlogError(4319); blogerror("BXSData::load -- failed to allocate mVariables[%d].", i);}
         return(false);
      }
      if (mVariables[i]->load(chunkReader, this) == false)
      {
         {setBlogError(4320); blogerror("BXSData::load -- failed to load mVariables[%d].", i);}
         return(false);
      }
   }
   //Exported variables.
   long numberExportedVariables=0;
   CHUNKREADSAFE(chunkReader, Long, numberExportedVariables);
   if (mExportedVariables.setNumber(numberExportedVariables) == false)
   {
      {setBlogError(4321); blogerror("BXSData::load -- failed to allocate %d mExportedVariables.", numberExportedVariables);}
      return(false);
   }
   long readCount=0;
   CHUNKREADARRAYSAFE(chunkReader, Long, &readCount, (long*)mExportedVariables.getPtr(), numberExportedVariables);
   //User Classes.
   long numberUserClasses=0;
   CHUNKREADSAFE(chunkReader, Long, numberUserClasses);
   if (mUserClasses.setNumber(numberUserClasses) == false)
   {
      {setBlogError(4322); blogerror("BXSData::load -- failed to allocate %d mUserClasses.", numberUserClasses);}
      return(false);
   }
   for (long i=0; i < mUserClasses.getNumber(); i++)
   {
      mUserClasses[i]=new BXSUserClassEntry;
      if (mUserClasses[i] == NULL)
      {
         {setBlogError(4323); blogerror("BXSData::load -- failed to allocate mUserClasses[%d].", i);}
         return(false);
      }
      if (mUserClasses[i]->load(chunkReader, this) == false)
      {
         {setBlogError(4324); blogerror("BXSData::load -- failed to load mUserClasses[%d].", i);}
         return(false);
      }
   }
   //Events.
   long numberEvents=0;
   CHUNKREADSAFE(chunkReader, Long, numberEvents);
   if (mEvents.setNumber(numberEvents) == false)
   {
      {setBlogError(4325); blogerror("BXSData::load -- failed to allocate %d mEvents.");}
      return(false);
   }
   for (long i=0; i < mEvents.getNumber(); i++)
   {
      if (mEvents[i].load(chunkReader) == false)
      {
         {setBlogError(4326); blogerror("BXSData::load -- failed to load mEvents[%d].", i);}
         return(false);
      }
   }
   //Stack.
   long stackSize=0;
   CHUNKREADSAFE(chunkReader, Long, stackSize);
   if (mStack.setNumber(stackSize) == false)
   {
      {setBlogError(4327); blogerror("BXSData::load -- failed to allocate %d mStack.", stackSize);}
      return(false);
   }
   readCount=0;
   CHUNKREADARRAYSAFE(chunkReader, Long, &readCount, (long*)mStack.getPtr(), stackSize);
   //Heap.
   long heapSize=0;
   CHUNKREADSAFE(chunkReader, Long, heapSize);
   if (mHeap.setNumber(heapSize) == false)
   {
      {setBlogError(4328); blogerror("BXSData::load -- failed to allocate %d mHeap.", heapSize);}
      return(false);
   }
   readCount=0;
   CHUNKREADARRAYSAFE(chunkReader, BYTE, &readCount, (BYTE*)mHeap.getPtr(), heapSize);
   //Interpret counter.
   CHUNKREADSAFE(chunkReader, Long, mInterpretCounter);
   //Main function ID.
   CHUNKREADSAFE(chunkReader, Long, mMainFunctionID);
   //Call stack.
   long callStackSize=0;
   CHUNKREADSAFE(chunkReader, Long, callStackSize);
   if (mCallStack.setNumber(callStackSize) == false)
   {
      {setBlogError(4329); blogerror("BXSData::load -- failed to allocate %d mCallStack.", callStackSize);}
      return(false);
   }
   for (long i=0; i < mCallStack.getNumber(); i++)
   {
      if (mCallStack[i].load(chunkReader) == false)
      {
         {setBlogError(4330); blogerror("BXSData::load -- failed to load mCallStack[%d].", i);}
         return(false);
      }
   }
   //Current rule ID.
   CHUNKREADSAFE(chunkReader, Long, mCurrentRuleID);
   //Failsafes.
   CHUNKREADSAFE(chunkReader, Long, mInfiniteLoopPCJumpValue);
   CHUNKREADSAFE(chunkReader, Long, mInfiniteLoopPCJumpCount);
   CHUNKREADSAFE(chunkReader, Long, mInfiniteLoopLimit);
   CHUNKREADSAFE(chunkReader, Long, mInfiniteRecursionLimit);
   //Breakpoints.
   long numberBreakpoints=0;
   CHUNKREADSAFE(chunkReader, Long, numberBreakpoints);
   if (mBreakpoints.setNumber(numberBreakpoints) == false)
   {
      {setBlogError(4331); blogerror("BXSData::load -- failed to allocate %d mBreakpoints.", numberBreakpoints);}
      return(false);
   }
   for (long i=0; i < mBreakpoints.getNumber(); i++)
   {
      if (mBreakpoints[i].load(chunkReader) == false)
      {
         {setBlogError(4332); blogerror("BXSData::load -- failed to load mBreakpoints[%d].", i);}
         return(false);
      }
   }
   //Breakpoint Go.
   CHUNKREADSAFE(chunkReader, Long, mBreakpointGo);
   //Break now.
   CHUNKREADSAFE(chunkReader, Bool, mBreakNow);
   //Current time.
   CHUNKREADSAFE(chunkReader, DWORD, mCurrentTime);
   //Ready to execute.
   CHUNKREADSAFE(chunkReader, Bool, mReadyToExecute);

   //Output
   bool hasOutput=false;
   CHUNKREADSAFE(chunkReader, Bool, hasOutput);
   if (hasOutput == true)
   {
      long num=0;
      CHUNKREADSAFE(chunkReader, Long, num);
      BSimString temp;
      for (long i=0; i < num; i++)
      {
         CHUNKREADSAFE(chunkReader, BSimString, temp);
         BColor tempColor;
         if (tempColor.load(chunkReader) == false)
         {
            {setBlogError(4333); blogerror("BXSRuntime::load -- failed to load mOutputColors[%d].", i);}
            return(false);
         }
         mOutput.add(temp);
         mOutputColors.add(tempColor);
      }
      CHUNKREADSAFE(chunkReader, Long, mOutputLineNumber);
   }
   mOutputChanged=true;

   // Array loading
   // array infos
   if(version >= 2)
   {
      long j = 0;
      long length = 0, length2 = 0, numRead = 0;
      BSimString tempStr;
      CHUNKREADSAFE(chunkReader, Long, length);
      mArrayInfos.setNumber(length);
      for(long i = 0; i < mArrayInfos.getNumber(); i++)
      {
         BXSArrayInfo *info = new BXSArrayInfo;
         if(!info)
            return false;
         
         chunkReader->readBSimString(&(info->mName));
         CHUNKREADSAFE(chunkReader, Long, info->mArrayID);
         CHUNKREADSAFE(chunkReader, Long, info->mArrayType);
         mArrayInfos[i] = info;
      }
      
      // integer arrays
      CHUNKREADSAFE(chunkReader, Long, length);
      mIntegerArrays.setNumber(length);
      for(long i = 0; i < mIntegerArrays.getNumber(); i++)
      {
         BDynamicSimLongArray *array = new BDynamicSimLongArray;
         if(!array)
            return false;
         chunkReader->peekArrayLength(&length2);
         array->setNumber(length2);
         CHUNKREADARRAYSAFE(chunkReader, Long, &numRead, array->getData(), length2);
         mIntegerArrays[i] = array;
      }
      
      // float arrays
      CHUNKREADSAFE(chunkReader, Long, length);
      mFloatArrays.setNumber(length);
      for(long i = 0; i < mFloatArrays.getNumber(); i++)
      {
         BDynamicSimFloatArray *array = new BDynamicSimFloatArray;
         if(!array)
            return false;
         chunkReader->peekArrayLength(&length2);
         array->setNumber(length2);
         CHUNKREADARRAYSAFE(chunkReader, Float, &numRead, array->getData(), length2);
         mFloatArrays[i] = array;
      }
      
      // bool arrays
      CHUNKREADSAFE(chunkReader, Long, length);
      mBoolArrays.setNumber(length);
      for(long i = 0; i < mBoolArrays.getNumber(); i++)
      {
         BDynamicSimArray<bool> *array = new BDynamicSimArray<bool>;
         if(!array)
            return false;
         chunkReader->peekArrayLength(&length2);
         array->setNumber(length2);
         CHUNKREADARRAYSAFE(chunkReader, Bool, &numRead, array->getData(), length2);
         mBoolArrays[i] = array;
      }
      
      // string arrays
      CHUNKREADSAFE(chunkReader, Long, length);
      mStringArrays.setNumber(length);
      for(long i = 0; i < mStringArrays.getNumber(); i++)
      {
         BDynamicSimArray<BSimString> *array = new BDynamicSimArray<BSimString>;
         if(!array)
            return false;
         long count = 0;
         CHUNKREADSAFE(chunkReader, Long, count);
         for(j = 0; j < count; j++)
         {
            chunkReader->readBSimString(&tempStr);
            array->add(tempStr);
         }
         mStringArrays[i] = array;
      }
      
      // vector arrays
      CHUNKREADSAFE(chunkReader, Long, length);
      mVectorArrays.setNumber(length);
      for(long i = 0; i < mVectorArrays.getNumber(); i++)
      {
         BDynamicSimVectorArray *array = new BDynamicSimVectorArray;
         if(!array)
            return false;
         chunkReader->peekArrayLength(&length2);
         array->setNumber(length2);
         CHUNKREADARRAYSAFE(chunkReader, Vector, &numRead, array->getData(), length2);
         mVectorArrays[i] = array;
      }
   }

   //User Heap.
   if (version >= 4)
   {
      long numberUserHeap=0;
      CHUNKREADSAFE(chunkReader, Long, numberUserHeap);
      if (mUserHeap.setNumber(numberUserHeap) == false)
      {
         {setBlogError(0000); blogerror("BXSRuntime::load -- failed to allocate %d mUserHeap.", numberUserHeap);}
         return(false);
      }
      
      for (long i=0; i < mUserHeap.getNumber(); i++)
      {
         long length=0;
         chunkReader->peekArrayLength(&length);
         mUserHeap[i]=new BYTE[length];
         if (mUserHeap[i] == NULL)
         {
            {setBlogError(0000); blogerror("BXSRuntime::load -- failed to allocate mUserHeap[%d].", i);}
            return(false);
         }
         long numRead;
         CHUNKREADARRAYSAFE(chunkReader, BYTE, &numRead, mUserHeap[i], length);
      }
   }

   //UnitID.
   if (version >= 3)
      CHUNKREADSAFE(chunkReader, Long, mUnitID);
   else
      mUnitID=-1;

   //Validate our reading of the chunk.
   result=chunkReader->validateChunkRead(BCHUNKTAG("XC"));
   if (result == false)
   {
      {setBlogError(4334); blogerror("BXSData::load -- did not read chunk properly!");}
      return(false);
   }

   return(true);
}
#endif

//==============================================================================
// BXSData::copyData
//==============================================================================
bool BXSData::copyData(BXSData *data)
{
   //Bomb check.
   if (data == NULL)
      return(false);
      
   //Save our
   //NOTE: This is way over anal. It copies many things that don't need to be
   //in the name of completeness.  If perf is an issue, some of this can go away.
   cleanUp(false);

   //DO NOT mess with mID or mName.  Those are setup outside of our purview.

   //Source.
   mSource=data->mSource;

   //Rule Module copy.
   if (BXSRuleModule::copyData(data) == false)
      return(false);

   //Functions.
   if (mFunctions.setNumber(data->mFunctions.getNumber()) == false)
      return(false);
   for (long i=0; i < mFunctions.getNumber(); i++)
   {
      mFunctions[i]=new BXSFunctionEntry;
      if (mFunctions[i] == NULL)
         return(false);
      mFunctions[i]->copy(data->mFunctions[i]);
   }
   //Variables.
   if (mVariables.setNumber(data->mVariables.getNumber()) == false)
      return(false);
   for (long i=0; i < mVariables.getNumber(); i++)
   {
      mVariables[i]=new BXSVariableEntry;
      if (mVariables[i] == NULL)
         return(false);
      mVariables[i]->copy(data->mVariables[i]);
   }
   //User Classes.
   if (mUserClasses.setNumber(data->mUserClasses.getNumber()) == false)
      return(false);
   for (long i=0; i < mUserClasses.getNumber(); i++)
   {
      mUserClasses[i]=new BXSUserClassEntry;
      if (mUserClasses[i] == NULL)
         return(false);
      mUserClasses[i]->copy(data->mUserClasses[i]);
   }
   //Events.
   if (mEvents.setNumber(data->mEvents.getNumber()) == false)
      return(false);
   for (long i=0; i < mEvents.getNumber(); i++)
      mEvents[i]=data->mEvents[i];

   //Stack.
   if (mStack.setNumber(data->mStack.getNumber()) == false)
      return(false);
   memcpy(mStack.getPtr(), data->mStack.getPtr(), mStack.getNumber()*sizeof(long));
   //Heap.
   if (mHeap.setNumber(data->mHeap.getNumber()) == false)
      return(false);
   memcpy(mHeap.getPtr(), data->mHeap.getPtr(), mHeap.getNumber()*sizeof(BYTE));
   //Interpret counter.
   mInterpretCounter=data->mInterpretCounter;
   //Main function Id.
   mMainFunctionID=data->mMainFunctionID;
   //Callstack.
   if (mCallStack.setNumber(data->mCallStack.getNumber()) == false)
      return(false);
   for (long i=0; i < mCallStack.getNumber(); i++)
      mCallStack[i]=data->mCallStack[i];
   //Current rule.
   mCurrentRuleID=data->mCurrentRuleID;
   //Failsafes.
   mInfiniteLoopPCJumpValue=data->mInfiniteLoopPCJumpValue;
   mInfiniteLoopPCJumpCount=data->mInfiniteLoopPCJumpCount;
   mInfiniteLoopLimit=data->mInfiniteLoopLimit;
   mInfiniteRecursionLimit=data->mInfiniteRecursionLimit;
   //Breakpoints.
   if (mBreakpoints.setNumber(data->mBreakpoints.getNumber()) == false)
      return(false);
   for (long i=0; i < mBreakpoints.getNumber(); i++)
      mBreakpoints[i]=data->mBreakpoints[i];
   mBreakNow=data->mBreakNow;
   mBreakpointGo=data->mBreakpointGo;
   //Calling String.
   StringCchCopyNExA(mCallingString, sizeof(mCallingString), data->mCallingString, sizeof(mCallingString), NULL, NULL, STRSAFE_FILL_BEHIND_NULL);
   //Current Time.
   mCurrentTime=data->mCurrentTime;
   //SKIP Temp return value.  It will get created as needed later.
   //User heap.
   if (mUserHeap.setNumber(data->mUserHeap.getNumber()) == false)
      return(false);
   for (long i=0; i < mUserHeap.getNumber(); i++)
   {
      BYTE *he=data->mUserHeap[i];
      long heSize=*(long*)(he+cUserHeapMaxSizeOffset);
      long realSize=heSize+cUserHeapOverheadSize;
      mUserHeap[i]=new BYTE[realSize];
      if (mUserHeap[i] == NULL)
         return(false);
      memcpy(mUserHeap[i], data->mUserHeap[i], realSize);
   }
   //Output.
   if (mOutput.setNumber(data->mOutput.getNumber()) == false)
      return(false);
   if (mOutputColors.setNumber(data->mOutput.getNumber()) == false)
      return(false);
   for (long i=0; i < mOutput.getNumber(); i++)
   {
      mOutput[i]=data->mOutput[i];
      mOutputColors[i]=data->mOutputColors[i];
   }
   mOutputLineNumber=data->mOutputLineNumber;
   mOutputChanged=data->mOutputChanged;
   //SKIP Messenger.  It was given to us.
   //RTE.
   mReadyToExecute=data->mReadyToExecute;

   return(true);
}

//==============================================================================
// BXSData::getSymbolID
//==============================================================================
long BXSData::getSymbolID(const char *name, long type) const
{
   if (name == NULL)
      return(-1);
   const BXSSymbolEntry *se=mSource->getSymbols().getEntryBySymbol(name);
   if (se == NULL)
      return(-1);
   if (se->mType != type)
      return(-1);
   return(se->mID);
}

//==============================================================================
// BXSData::getSymbolValue
//==============================================================================
long BXSData::getSymbolValue(const char *name, long type) const
{
   if (name == NULL)
      return(-1);
   const BXSSymbolEntry *se=mSource->getSymbols().getEntryBySymbol(name);
   if (se == NULL)
      return(-1);
   if (se->mType != type)
      return(-1);
   return(se->mValue);
}

//==============================================================================
// BXSData::cleanUp
//==============================================================================
void BXSData::cleanUp(bool cleanID)
{
   resetStackAndHeap();

   for (long i=0; i < mUserHeap.getNumber(); i++)
   {
      if (mUserHeap[i] != NULL)
      {
         delete mUserHeap[i];
         mUserHeap[i]=NULL;
      }
   }
   mUserHeap.setNumber(0);

   for (long i=0; i < mFunctions.getNumber(); i++)
   {
      delete mFunctions[i];
      mFunctions[i]=NULL;
   }
   mFunctions.setNumber(0);
   for (long i=0; i < mVariables.getNumber(); i++)
   {
      delete mVariables[i];
      mVariables[i]=NULL;
   }
   mVariables.setNumber(0);
   mExportedVariables.setNumber(0);
   for (long i=0; i < mUserClasses.getNumber(); i++)
   {
      delete mUserClasses[i];
      mUserClasses[i]=NULL;
   }
   mUserClasses.setNumber(0);

   //Temp return value.
   if (mTempReturnValue != NULL)
   {
      BDELETE(mTempReturnValue);  
      mTempReturnValueSize=0;
      mTempReturnValueMaxSize=0;
   }

   mInfiniteLoopPCJumpValue=-1;
   mInfiniteLoopPCJumpCount=0;
   mInfiniteLoopLimit=-1;
   mInfiniteRecursionLimit=-1;
   mBreakpoints.setNumber(0);
   mBreakpointGo=cBreakpointGoNot;
   mBreakNow=false;
   mCurrentTime=(DWORD)0;
   mOutput.setNumber(0);
   mOutputColors.setNumber(0);
   mOutputLineNumber=2000;
   mOutputChanged=false;
   mReadyToExecute=false;
   mCurrentRuleID=-1;
   mCallStack.setNumber(0);
   mMainFunctionID=-1;
   mInterpretCounter=0;
   mSource=NULL;
   
   if (cleanID == true)
   {
      mName.empty();
      mID=-1;
   }

   for(long i = 0; i < mIntegerArrays.getNumber(); i++)
      delete mIntegerArrays[i];

   for(long i = 0; i < mFloatArrays.getNumber(); i++)
      delete mFloatArrays[i];

   for(long i = 0; i < mBoolArrays.getNumber(); i++)
      delete mBoolArrays[i];

   for(long i = 0; i < mStringArrays.getNumber(); i++)
      delete mStringArrays[i];

   for(long i = 0; i < mVectorArrays.getNumber(); i++)
      delete mVectorArrays[i];

   for(long i = 0; i < mArrayInfos.getNumber(); i++)
      delete mArrayInfos[i];
}
