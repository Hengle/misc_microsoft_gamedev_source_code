
//==============================================================================
// xscompiler.cpp
//
//Copyright (c) 1999-2003, Ensemble Studios
//==============================================================================

//==============================================================================
//Includes
#include "xscript.h"
#include "xscompiler.h"
#include "xsdata.h"
#include "xsdefines.h"
#include "xsemitter.h"
#include "xsmessenger.h"
#include "xsopcodes.h"
#include "xsoptimizer.h"
#include "xsparsetree.h"
#include "xssource.h"
#include "xssymboltable.h"
#include "xstokenizer.h"
#include "config.h"
#include "econfigenum.h"

//==============================================================================
//Defines
//#define DEBUGEMITLISTSONERROR
//#define DEBUGTRACEPARSE

//==============================================================================
// Current Last Error Number is 0404.
// Current Last Warning Number is 0003.
// Current Last Info Number is 0006.


//==============================================================================
// BXSCompiler::BXSCompiler
//==============================================================================
BXSCompiler::BXSCompiler(BXSMessenger *messenger, BXSData *data, BXSSource *source,
   BXSSyscallModule *sm) :
   mBaseTokenizer(NULL),
   //mTokenizers doesn't need any ctor args.
   //mUnusedTokenizers doesn't need any ctor args.
   mEmitter(NULL),
   mOptimizer(NULL),
   mListing(false),
   mDebugTokenizer(false),
   mDebugCode(true),
   //mCurrentScope gets initialized below.
   mCurrentFunctionID(-1),
   mCurrentClassID(-1),
   //mFunctionOverloads doesn't need any ctor args.
   //mLabels doesn't need any ctor args.
   //mBreakLabelIDs doesn't need any ctor args.
   //mContinueLabelIDs doesn't need any ctor args.
   //mQuads doesn't need any ctor args.
   //mIncludePath doesn't need any ctor args.
   mListFile(NULL),
   //mModuleFilenames doesn't need any ctor args.
   mSyscalls(sm),
   mSource(source),
   mData(data),
   mMessenger(messenger)
   //mParseImmediateStrings doesn't need any ctor args.
   //mParseImmediateVectors doesn't need any ctor args.
{
   StringCchCopyA(mCurrentScope, BXSTokenizer::cMaxTokenSize+2, "");
}

//==============================================================================
// BXSCompiler::~BXSCompiler
//==============================================================================
BXSCompiler::~BXSCompiler(void)
{
   cleanUp();
}

//==============================================================================
// BXSCompiler::initialize
//==============================================================================
bool BXSCompiler::initialize(void)
{
   //Init/reset key vars.
   mCurrentFunctionID=-1;
   mCurrentClassID=-1;
   StringCchCopyA(mCurrentScope, BXSTokenizer::cMaxTokenSize+2, "");

   //Base Tokenizer.
   mBaseTokenizer=new BXSTokenizer(mMessenger);
   if (mBaseTokenizer != NULL)
   {
      mBaseTokenizer->initialize();
      mBaseTokenizer->setDebug(mDebugTokenizer);
   }

   //Emitter.
   mEmitter=new BXSEmitter(mMessenger);
   if (mEmitter == NULL)
      return(false);

   //Optimizer.
   //mOptimizer=new BXSOptimizer(mMessenger);
   //if (mOptimizer == NULL)
   //   return(false);

   return(true);
}

//==============================================================================
// BXSCompiler::initialize
//==============================================================================
bool BXSCompiler::initialize(long baseDirID, long baseUWDirID)
{
   //Init/reset key vars.
   mCurrentFunctionID=-1;
   mCurrentClassID=-1;
   StringCchCopyA(mCurrentScope, BXSTokenizer::cMaxTokenSize+2, "");

   //Base Tokenizer.
   mBaseTokenizer=new BXSTokenizer(mMessenger);
   if (mBaseTokenizer != NULL)
   {
      mBaseTokenizer->initialize();
      mBaseTokenizer->setDebug(mDebugTokenizer);
   }

   //Emitter.
   mEmitter=new BXSEmitter(mMessenger);
   if (mEmitter == NULL)
      return(false);

   //Optimizer.
   //mOptimizer=new BXSOptimizer(mMessenger);
   //if (mOptimizer == NULL)
   //   return(false);

   mBaseDirID = baseDirID;
   mBaseUWDirID = baseUWDirID;

   return(true);
}

//==============================================================================
// BXSCompiler::getModuleFilename
//==============================================================================
const BSimString& BXSCompiler::getModuleFilename(long index) const
{
   if ((index < 0) || (index >= mModuleFilenames.getNumber()))
   {
      static BSimString foo;
      return(foo);
   }
   return(mModuleFilenames[index]);
}

//==============================================================================
// BXSCompiler::addModuleFilename
//==============================================================================
bool BXSCompiler::addModuleFilename(const BSimString &filename)
{
   if (mModuleFilenames.add(filename) < 0)
      return(false);
   return(true);
}

//==============================================================================
// BXSCompiler::removeModuleFilename
//==============================================================================
bool BXSCompiler::removeModuleFilename(const BSimString &filename)
{
   return(mModuleFilenames.removeValue(filename));
}

//==============================================================================
// BXSCompiler::addExternalVariable
//==============================================================================
bool BXSCompiler::addExternalVariable(const char *source)
{
   //Check the tokenizer.
   if (getTokenizer() == NULL)
   {
      mMessenger->errorMsg("Error 0001: tokenizer is NULL.");
      return(false);
   }
   if (source == NULL)
   {
      mMessenger->errorMsg("Error 0002: invalid source.");
      return(false);
   }

   //Do the tokenize.
   getTokenizer()->setSource(B("EXTERN"), source, strlen(source)+1);
   getTokenizer()->tokenize();

   //Parse.
   parse();

   //If we have errors, return false.
   if (mMessenger->getErrorCount() > 0)
      return(false);
   return(true);
}

//==============================================================================
// BXSCompiler::compileModules
//==============================================================================
bool BXSCompiler::compileModules(const BSimString &name)
{
   //Prep.
   prepareCompilation(B(""), name, name);

   //Parse each module in turn.
   char *sourceBuffer=NULL;
   long sourceBufferLength=0;
   for (long i=0; i < mModuleFilenames.getNumber(); i++)
   {
      //String filename.
      BSimString qualifiedFilename;
      if (mIncludePath.length() > 0)
         qualifiedFilename.format(B("%s%s"), mIncludePath.getPtr(), mModuleFilenames[i].getPtr());
      else
         qualifiedFilename=mModuleFilenames[i];

      //Open the file, create a buffer for the data, and then stuff the file data
      //into the buffer.
      BFile file;
      bool ok = file.openReadOnly(mBaseUWDirID, mModuleFilenames[i]);
      if(!ok)
         ok = file.openReadOnly(mBaseDirID, mModuleFilenames[i]);
      if (!ok)
      {
         srcErrMsg("Error 0354: failed to open file '%s'.", qualifiedFilename.getPtr());
         BDELETEARRAY(sourceBuffer);
         return(false);
      }
      //Get the size.
      unsigned long fileSize=0;
      if (file.getSize(fileSize) == false)
      {
         srcErrMsg("Error 0355: failed to get size of file '%s'.", qualifiedFilename.getPtr());
         BDELETEARRAY(sourceBuffer);
         return(false);
      }
      //Allocate a temporary buffer.
      if ((sourceBuffer == NULL) || (sourceBufferLength < (long)fileSize+1))
      {
         BDELETEARRAY(sourceBuffer);
         sourceBuffer=new char[(long)fileSize+1];
         if (sourceBuffer == NULL)
         {
            srcErrMsg("Error 0357: could not allocate buffer.");
            return(false);
         }
         sourceBufferLength=(long)fileSize+1;
      }
      //Read it in.
      if (file.read(sourceBuffer, fileSize) == false)
      {
         srcErrMsg("Error 0358: failed to read file data.");
         BDELETEARRAY(sourceBuffer);
         return(false);
      }
      sourceBuffer[fileSize]='\0';

      //Actually do the include.
      bool rVal=parseSource(qualifiedFilename, mModuleFilenames[i], sourceBuffer, (long)fileSize+1);
      if (rVal == false)
      {
         BDELETEARRAY(sourceBuffer);
         return(false);
      }
   }

   //Cleanup.
   BDELETEARRAY(sourceBuffer);
   //Done.
   finishCompilation(B(""), name);
   return(true);
}

//==============================================================================
// BXSCompiler::compileFile
//==============================================================================
bool BXSCompiler::compileFile(const BSimString &filename, const BSimString &qualifiedFilename,
   const BSimString &listPrefix, const char *source, long sourceLength)
{
   //Prep.
   prepareCompilation(listPrefix, filename, qualifiedFilename);

   //Parse the source.
   if (parseSource(qualifiedFilename, filename, source, sourceLength) == false)
   {
      mMessenger->errorMsg("Error 0003: could not compile file '%s'.", qualifiedFilename.getPtr());
      return(false);
   }

   //Done.
   finishCompilation(listPrefix, filename);
   return(true);
}

//==============================================================================
// BXSCompiler::parseImmediate
//==============================================================================
bool BXSCompiler::parseImmediate(const char *source, long sourceLength)
{
   #ifdef DEBUGTRACEPARSE
   mMessenger->infoMsg("ParseImmediate.");
   #endif

   //Check the tokenizer.
   if (getTokenizer() == NULL)
   {
      mMessenger->errorMsg("BXSC ERROR 0001: no tokenizer.");
      return(false);
   }
   if (source == NULL)
   {
      mMessenger->errorMsg("BXSC ERROR 0002: no source.");
      return(false);
   }

   //Do the tokenize.
   getTokenizer()->setSource(B(""), source, sourceLength);
   getTokenizer()->tokenize();

   //Rip through the tokens.  Call the syscalls as they come.
   bool executedSomething=false;
   for (long token=getTokenizer()->getCurrentToken(); (token != BXSTokenizer::cEOS) && (token != BXSTokenizer::cUnknown); token=getTokenizer()->getDarkSpaceToken(this))
   {
      //Make sure we have a name token.
      if (token != BXSTokenizer::cName)
         continue;

      //Grab the symbol entry.
      const BXSSymbolEntry *se=getSymbolEntry(getTokenizer()->getCurrentTokenText(), true);
      if ((se == NULL) ||
         (se->mType != BXSBinary::cSyscall) ||
         (se->mValue < 0) || (se->mValue >= mSyscalls->getNumberSyscalls()) )
         continue;

      //Get the syscall.
      long syscallID=se->mValue;
      const BXSSyscallEntry *syscall=mSyscalls->getSyscall(syscallID);
      if (syscall == NULL)
      {
         mMessenger->errorMsg("BXSC ERROR 0015: error finding syscall '%s' (id=%d).", getTokenizer()->getCurrentTokenText(), syscallID);
         return(false);
      }

      //Get the syscall data.
      long numberArguments=syscall->getNumberParameters();

      //Space for the arguments to the syscall.
      long spi[BXSSyscallModule::cMaximumNumberSyscallParms];

      //The left paren is used to signal that arguments are being passed.
      long numberArgumentsCounted=0;
      token=getTokenizer()->getDarkSpaceToken(this);
      //If we don't have a left (, we are not using the token so push it back.
      if (token != BXSTokenizer::cLeftParen) 
         getTokenizer()->decrementCurrentToken(this);
      //Else, if we have a left (, check for a ) paren.
      if (numberArguments == 0)
      {
         if (token == BXSTokenizer::cLeftParen)
         {
            long nextToken=getTokenizer()->getDarkSpaceToken(this);
            if (nextToken != BXSTokenizer::cRightParen)
            {
               mMessenger->errorMsg("BXSC ERROR 0003: error parsing command '%s'.", source);
               return(false);
            }
         }
      }
      //Else, we have args to parse.
      else
      {
         bool needSeparator=false;
         for (;;)
         {
            //Grab the next token.
            token=getTokenizer()->getToken(this);
            //If we have a comma, deal with that.
            if (token == BXSTokenizer::cComma)
            {
               if (needSeparator == false)
                  return(false);
               needSeparator=false;
               continue;
            }
            //End of string.
            if (token == BXSTokenizer::cEOS)
               break;
            //Break if we hit the right paren (since any/all args of a syscall are now optional).
            if (token == BXSTokenizer::cRightParen)
               break;
            //If we need a separator, we're busted.
            if (needSeparator == true)
            {
               mMessenger->errorMsg("BXSC ERROR 0004: error parsing command '%s'.", source);
               return(false);
            }
            //If we've counted too many args, bail.
            if (numberArgumentsCounted >= numberArguments)
            {
               mMessenger->errorMsg("BXSC ERROR 0012: error parsing command '%s'.", source);
               return(false);
            }

            //Pop it back.
            getTokenizer()->decrementCurrentToken(this);

            //Get the arg type.
            long argumentType=mSyscalls->getSyscall(syscallID)->getParameter(numberArgumentsCounted)->getType();

            //Parse the arg.
            switch (argumentType)
            {
               case BXSVariableEntry::cIntegerVariable:
               {
                  long defaultValue;
                  if (parseIntegerConstant(&defaultValue) == false)
                  {
                     mMessenger->errorMsg("BXSC ERROR 0005: error parsing command '%s'.", source);
                     return(false);
                  }
                  spi[numberArgumentsCounted]=defaultValue;
                  break;
               }
               case BXSVariableEntry::cFloatVariable:
               {
                  float defaultValue;
                  if (parseFloatConstant(&defaultValue) == false)
                  {
                     mMessenger->errorMsg("BXSC ERROR 0006: error parsing command '%s'.", source);
                     return(false);
                  }
                  spi[numberArgumentsCounted]=*((long*)&defaultValue);
                  break;
               }
               //Bool.
               case BXSVariableEntry::cBoolVariable:
               {
                  bool defaultValue;
                  if (parseBoolConstant(&defaultValue) == false)
                  {
                     mMessenger->errorMsg("BXSC ERROR 0007: error parsing command '%s'.", source);
                     return(false);
                  }
                  spi[numberArgumentsCounted]=(long)defaultValue;
                  break;
               }
               //String.
               case BXSVariableEntry::cStringVariable:
               {
                  token=getTokenizer()->getDarkSpaceToken(this);
                  if (token != BXSTokenizer::cString)
                  {
                     mMessenger->errorMsg("BXSC ERROR 0008: error parsing command '%s'.", source);
                     return(false);
                  }
                  //Make a copy of the string into the next immediate string spot.
                  StringCchCopyNExA(mParseImmediateStrings[numberArgumentsCounted],
                                    sizeof(mParseImmediateStrings[numberArgumentsCounted]), getTokenizer()->getCurrentTokenText(), BXSTokenizer::cMaxTokenSize, NULL, NULL, STRSAFE_FILL_BEHIND_NULL);
                  //Stuff that pointer into the SPI list.
                  spi[numberArgumentsCounted]=(long)&(mParseImmediateStrings[numberArgumentsCounted]);
                  break;
               }
               case BXSVariableEntry::cVectorVariable:
               {
                  token=getTokenizer()->getDarkSpaceToken(this);
                  if (token != BXSTokenizer::cVector)
                  {
                     mMessenger->errorMsg("BXSC ERROR 0009: error parsing command '%s'.", source);
                     return(false);
                  }
                  BVector defaultValue;
                  if (parseVectorConstant(&defaultValue) == false)
                  {
                     mMessenger->errorMsg("BXSC ERROR 0010: error parsing command '%s'.", source);
                     return(false);
                  }
                  //Stuff that pointer into the SPI list.
                  mParseImmediateVectors[numberArgumentsCounted]=defaultValue;
                  spi[numberArgumentsCounted]=(long)&(mParseImmediateVectors[numberArgumentsCounted]);
                  break;
               }
               default:
                  mMessenger->errorMsg("BXSC ERROR 0011: error parsing command '%s'.", source);
                  return(false);
            }

            //Increment how many args we have counted.
            numberArgumentsCounted++;

            //Need a separator between args.
            needSeparator=true;
        }
      }

      //Fill in any optional arguments required.
      for ( ; numberArgumentsCounted < numberArguments; numberArgumentsCounted++)
      {
         BXSVariableEntry *parm=mSyscalls->getSyscall(syscallID)->getParameter(numberArgumentsCounted);
         if (parm == NULL)
         {
            mMessenger->errorMsg("BXSC ERROR 0013: error parsing command '%s'.", source);
            return(false);
         }
         switch (parm->getType())
         {
            case BXSVariableEntry::cIntegerVariable:
            case BXSVariableEntry::cFloatVariable:
               spi[numberArgumentsCounted]=*((long*)parm->getData());
               break;
            case BXSVariableEntry::cBoolVariable:
            {
               bool temp=*((bool*)parm->getData());
               spi[numberArgumentsCounted]=(long)temp;
               break;
            }
            case BXSVariableEntry::cStringVariable:
            case BXSVariableEntry::cVectorVariable:
               spi[numberArgumentsCounted]=(long)parm->getData();
               break;
         }
      }

      //Make sure we had the right number of pushes.
      if (numberArguments != numberArgumentsCounted)
      {
         mMessenger->errorMsg("BXSC ERROR 0014: error parsing command '%s'.", source);
         return(false);
      }

      //Make the syscall.  We ignore return values.
      switch (mSyscalls->getSyscall(syscallID)->getReturnType())
      {
         case BXSVariableEntry::cVoidVariable:
            mSyscalls->callVoidSyscall(syscallID, spi, numberArguments);
            break;
         case BXSVariableEntry::cIntegerVariable:
         {
            long foo;
            mSyscalls->callIntegerSyscall(syscallID, spi, numberArguments, &foo, NULL);
            break;
         }
         case BXSVariableEntry::cFloatVariable:
         {
            float foo;
            mSyscalls->callFloatSyscall(syscallID, spi, numberArguments, &foo, NULL);
            break;
         }
         case BXSVariableEntry::cBoolVariable:
         {
            bool foo;
            mSyscalls->callBoolSyscall(syscallID, spi, numberArguments, &foo, NULL);
            break;
         }
         default:
            mMessenger->errorMsg("BXSC ERROR 0015: error parsing command '%s'.", source);
            return(false);
      }

      //We've executed something.
      executedSomething=true;
   }

   if (executedSomething == false)
      return(false);
   return(true);
}

//==============================================================================
// BXSCompiler::setLabelPosition
//==============================================================================
void BXSCompiler::setLabelPosition(long labelID, long labelPosition)
{
   if ((labelID < 0) || (labelID >= mLabels.getNumber()) )
      return;
   BASSERT(mLabels[labelID].mPosition == -1);
   mLabels[labelID].mPosition=labelPosition;
}

//==============================================================================
// BXSCompiler::addLineNumberQuad
//==============================================================================
void BXSCompiler::addLineNumberQuad(long lineNumber)
{
   if (lineNumber < 0)
   {
      BASSERT(0);
      return;
   }

   //LINE opcode.
   if (mQuads.add(BXSQuad(BXSQuadOpcode::cLINE, lineNumber)) == -1)
   {
      srcErrMsg("Error 0302: LINE quad addition failed.");
      return;
   }

   //Get current function entry.
   BXSFunctionEntry *entry=getCurrentFunctionEntry();
   //Update starting line number if not set yet.
   if ((entry != NULL) && (entry->getLineNumber() < 0))
      entry->setLineNumber(lineNumber);
}

//==============================================================================
// BXSCompiler::addFileNumberQuad
//==============================================================================
void BXSCompiler::addFileNumberQuad(long fileNumber)
{
   if (fileNumber < 0)
   {
      BASSERT(0);
      return;
   }

   //FILE opcode.
   if (mQuads.add(BXSQuad(BXSQuadOpcode::cFILE, fileNumber)) == -1)
   {
      srcErrMsg("Error 0306: FILE quad addition failed.");
      return;
   }
}

//==============================================================================
// BXSCompiler::setIncludePath
//==============================================================================
void BXSCompiler::setIncludePath(const BSimString &v)
{
   mIncludePath=v;
}

//==============================================================================
// BXSCompiler::dumpTables
//==============================================================================
void BXSCompiler::dumpTables(BFile *dumpFile)
{
   if (dumpFile == NULL)
      return;

   //Header.
   dumpFile->fprintf("//==============================================================================\r\n");
   dumpFile->fprintf("// CXSDump\r\n");
   dumpFile->fprintf("//==============================================================================\r\n");
   dumpFile->fprintf("Main FunctionID=%d.\r\n\r\n", mData->getMainFunctionID());

   //File data.
   dumpFile->fprintf("File Data:\r\n");
   dumpFile->fprintf("  Files:      %5d files.\r\n", mSource->getNumberFiles());
   dumpFile->fprintf("  Symbols:    %5d symbols.\r\n", mSource->getSymbols().getNumberSymbols());
   dumpFile->fprintf("  Variables:  %5d variables.\r\n", mData->getNumberVariables());
   dumpFile->fprintf("  Functions:  %5d functions.\r\n", mData->getNumberFunctions());
   dumpFile->fprintf("  RuleGroups: %5d groups.\r\n", mData->getNumberRuleGroups());
   dumpFile->fprintf("  Rules:      %5d rules.\r\n", mData->getNumberRules());
   dumpFile->fprintf("  Code:       %d bytes (MAX=%d).\r\n", mSource->getCodeSize(), mSource->getMaxCodeSize());
   dumpFile->fprintf("\r\n");


   //Variable types.
   dumpFile->fprintf("6 Variable Types:\r\n");
   dumpFile->fprintf("  Void   (Value=%d).\r\n", BXSVariableEntry::cVoidVariable);
   dumpFile->fprintf("  Int    (Value=%d).\r\n", BXSVariableEntry::cIntegerVariable);
   dumpFile->fprintf("  Float  (Value=%d).\r\n", BXSVariableEntry::cFloatVariable);
   dumpFile->fprintf("  Bool   (Value=%d).\r\n", BXSVariableEntry::cBoolVariable);
   dumpFile->fprintf("  String (Value=%d).\r\n", BXSVariableEntry::cStringVariable);
   dumpFile->fprintf("  Vector (Value=%d).\r\n", BXSVariableEntry::cVectorVariable);
   dumpFile->fprintf("\r\n");

   //Files.
   dumpFile->fprintf("%d Files:\r\n", mSource->getNumberFiles());
   for (long i=0; i < mSource->getNumberFiles(); i++)
   {
      BXSFileEntry *fileEntry=mSource->getFileEntry(i);
      if (fileEntry != NULL)
         dumpFile->fprintf("  %-30s (ID=%4d).\r\n", fileEntry->getFilename(), fileEntry->getID());
   }
   dumpFile->fprintf("\r\n");

   //Symbols.
   dumpFile->fprintf("%d Symbols:\r\n", mSource->getSymbols().getNumberSymbols());
   for (long i=0; i < mSource->getSymbols().getNumberSymbols(); i++)
   {
      const BXSSymbolEntry *se=mSource->getSymbols().getEntryByID(i);
      if (se == NULL)
         continue;
      dumpFile->fprintf("  %-30s (ID=%4d) SymType=%2d, SymValue=%d.\r\n", se->mSymbol, se->mID, se->mType, se->mValue);
   }
   dumpFile->fprintf("\r\n");

   //Variables.
   dumpFile->fprintf("%d 'Global (as in not in a function)' Variables:\r\n", mData->getNumberVariables());
   for (long i=0; i < mData->getNumberVariables(); i++)
   {
      BXSVariableEntry *ve=mData->getVariableEntry(i);
      switch (ve->getType())
      {
         case BXSVariableEntry::cIntegerVariable:
            dumpFile->fprintf("  %-30s (SymID=%4d), INT, Value=%d.\r\n",
               mSource->getSymbols().getSymbolByID(ve->getSymbolID()), ve->getSymbolID(), *(long*)(ve->getData()));
            break;
         case BXSVariableEntry::cFloatVariable:
            dumpFile->fprintf("  %-30s (SymID=%4d), FLOAT, Value=%f.\r\n",
               mSource->getSymbols().getSymbolByID(ve->getSymbolID()), ve->getSymbolID(), *(float*)(ve->getData()));
            break;
         case BXSVariableEntry::cBoolVariable:
            dumpFile->fprintf("  %-30s (SymID=%4d), BOOL, Value=%d.\r\n",
               mSource->getSymbols().getSymbolByID(ve->getSymbolID()), ve->getSymbolID(), *(bool*)(ve->getData()));
            break;
         case BXSVariableEntry::cStringVariable:
            dumpFile->fprintf("  %-30s (SymID=%4d), STRING, Value=%s.\r\n",
               mSource->getSymbols().getSymbolByID(ve->getSymbolID()), ve->getSymbolID(), (char*)(ve->getData()));
            break;
         case BXSVariableEntry::cVectorVariable:
         {
            BVector foo=*(BVector*)(ve->getData());
            dumpFile->fprintf("  %-30s (SymID=%4d), VECTOR, Value=(%f, %f, %f).\r\n",
               mSource->getSymbols().getSymbolByID(ve->getSymbolID()), ve->getSymbolID(), foo.x, foo.y, foo.z);
            break;
         }
         default:
            dumpFile->fprintf("  %-30s (SymID=%4d), Type=%s, Value=???.\r\n",
               mSource->getSymbols().getSymbolByID(ve->getSymbolID()), ve->getSymbolID(), getVariableTypeName(ve->getType()) );
            break;
      }
   }
   dumpFile->fprintf("\r\n");

   //Labels.
   dumpFile->fprintf("%d Labels:\r\n", mLabels.getNumber());
   for (long i=0; i < mLabels.getNumber(); i++)
      dumpFile->fprintf("  %-30s (SymID=%4d), Position=%d.\r\n",
         mSource->getSymbols().getSymbolByID(mLabels[i].mSymbolID), mLabels[i].mSymbolID, mLabels[i].mPosition);
   dumpFile->fprintf("\r\n");

   //User Functions.
   dumpFile->fprintf("%d User Functions:\r\n", mData->getNumberFunctions());
   for (long i=0; i < mData->getNumberFunctions(); i++)
   {
      BXSFunctionEntry *fe=mData->getFunctionEntry(i);
      if (fe == NULL)
         continue;

      dumpFile->fprintf("  %-15s%s(",
         getVariableTypeName(fe->getReturnType()),
         mSource->getSymbols().getSymbolByID(fe->getSymbolID()));
      for (long j=0; j < fe->getNumberParameters(); j++)
      {
         dumpFile->fprintf(" %s", getVariableTypeName(fe->getVariable(j)->getType()));
         if (j < fe->getNumberParameters()-1)
            dumpFile->fprintf(",");
      }
      if (fe->getNumberParameters() > 0)
         dumpFile->fprintf(" ");
      dumpFile->fprintf(") ID=%d.\r\n", fe->getID());

      dumpFile->fprintf("                   CodeOffset=%d.\r\n", fe->getCodeOffset());
      dumpFile->fprintf("                   %d Parameters.\r\n", fe->getNumberParameters());
      dumpFile->fprintf("                   %d Variables (Parms AND Vars).\r\n", fe->getNumberVariables());
      for (long j=0; j < fe->getNumberVariables(); j++)
      {
         switch (fe->getVariable(j)->getType())
         {
            case BXSVariableEntry::cIntegerVariable:
               dumpFile->fprintf("                     V[%d]: %s, INT, Value=%d.\r\n",
                  j, mSource->getSymbols().getSymbolByID(fe->getVariable(j)->getSymbolID()),
                  *(long*)(fe->getVariable(j)->getData()) );
               break;
            case BXSVariableEntry::cFloatVariable:
               dumpFile->fprintf("                     V[%d]: %s, FLOAT, Value=%f.\r\n",
                  j, mSource->getSymbols().getSymbolByID(fe->getVariable(j)->getSymbolID()),
                  *(float*)(fe->getVariable(j)->getData()) );
               break;
            case BXSVariableEntry::cBoolVariable:
               dumpFile->fprintf("                     V[%d]: %s, BOOL, Value=%d.\r\n",
                  j, mSource->getSymbols().getSymbolByID(fe->getVariable(j)->getSymbolID()),
                  *(bool*)(fe->getVariable(j)->getData()) );
               break;
            case BXSVariableEntry::cStringVariable:
               dumpFile->fprintf("                     V[%d]: %s, STRING, Value=%s.\r\n",
                  j, mSource->getSymbols().getSymbolByID(fe->getVariable(j)->getSymbolID()),
                  (char*)(fe->getVariable(j)->getData()) );
               break;
            case BXSVariableEntry::cVectorVariable:
            {
               BVector foo=*(BVector*)(fe->getVariable(j)->getData());
               dumpFile->fprintf("                     V[%d]: %s, VECTOR, Value=(%f, %f, %f).\r\n",
                  j, mSource->getSymbols().getSymbolByID(fe->getVariable(j)->getSymbolID()),
                  foo.x, foo.y, foo.z);
               break;
            }
            case BXSVariableEntry::cUserClassVariable:
            {
               dumpFile->fprintf("                     V[%d]: %s, USERCLASS, Length=%d.\r\n",
                  j, mSource->getSymbols().getSymbolByID(fe->getVariable(j)->getSymbolID()),
                  fe->getVariable(j)->getDataLength());
               break;
            }
            default:
               dumpFile->fprintf("                     V[%d]: %s, Type=%s, Value=???.\r\n",
                  j, mSource->getSymbols().getSymbolByID(fe->getVariable(j)->getSymbolID()),
                  getVariableTypeName(fe->getVariable(j)->getType()) );
               break;
         }
      }
   }
   dumpFile->fprintf("\r\n");

   //Function overloads.
   dumpFile->fprintf("%d Function Overloads:\r\n", mFunctionOverloads.getNumber());
   for (long i=0; i < mFunctionOverloads.getNumber(); i++)
      dumpFile->fprintf("  Old FID=%d, New FID=%d.\r\n", mFunctionOverloads[i].mOldFunctionID,
      mFunctionOverloads[i].mNewFunctionID);
   dumpFile->fprintf("\r\n");

   //Rule Groups.
   dumpFile->fprintf("%d User Rule Groups:\r\n", mData->getNumberRuleGroups());
   for (long i=0; i < mData->getNumberRuleGroups(); i++)
   {
      dumpFile->fprintf("  %s.\r\n",
         mSource->getSymbols().getSymbolByID(mData->getRuleGroupEntry(i)->getSymbolID()) );
   }
   dumpFile->fprintf("\r\n");

   //Rules.
   dumpFile->fprintf("%d User Rules:\r\n", mData->getNumberRules());
   for (long i=0; i < mData->getNumberRules(); i++)
   {
      BXSFunctionEntry *fe=mData->getFunctionEntry(mData->getRuleEntry(i)->getFunctionID());
      dumpFile->fprintf("  %s:\r\n",
         mSource->getSymbols().getSymbolByID(fe->getSymbolID()));
      dumpFile->fprintf("    Priority=%d.\r\n", mData->getRuleEntry(i)->getPriority());
      dumpFile->fprintf("    Interval=%d to %d.\r\n", mData->getRuleEntry(i)->getMinInterval(), mData->getRuleEntry(i)->getMaxInterval());
      dumpFile->fprintf("    FunctionID=%d.\r\n", mData->getRuleEntry(i)->getFunctionID());
      for (long j=0; j < mData->getRuleEntry(i)->getNumberGroups(); j++)
         dumpFile->fprintf("    Group '%s'.\r\n", mSource->getSymbols().getSymbolByID(mData->getRuleGroupEntry(mData->getRuleEntry(i)->getGroups()[j])->getSymbolID()) );
   }
   dumpFile->fprintf("\r\n");

   //Syscalls.
   dumpFile->fprintf("%d Syscalls:\r\n", mSyscalls->getNumberSyscalls());
   for (long i=0; i < mSyscalls->getNumberSyscalls(); i++)
   {
      const BXSSyscallEntry *syscall=mSyscalls->getSyscall(i);
      if (syscall == NULL)
         continue;

      dumpFile->fprintf("  %-15s%s(",
         getVariableTypeName(syscall->getReturnType()),
         mSource->getSymbols().getSymbolByID(syscall->getSymbolID()));
      for (long j=0; j < syscall->getNumberParameters(); j++)
      {
         switch (syscall->getParameter(j)->getType())
         {
            case BXSVariableEntry::cIntegerVariable:
               dumpFile->fprintf(" %s=%d",
                  getVariableTypeName(syscall->getParameter(j)->getType()),
                  *(long*)(syscall->getParameter(j)->getData()) );
               break;
            case BXSVariableEntry::cFloatVariable:
               dumpFile->fprintf(" %s=%f",
                  getVariableTypeName(syscall->getParameter(j)->getType()),
                  *(float*)(syscall->getParameter(j)->getData()) );
               break;
            case BXSVariableEntry::cBoolVariable:
               dumpFile->fprintf(" %s=%d",
                  getVariableTypeName(syscall->getParameter(j)->getType()),
                  *(bool*)(syscall->getParameter(j)->getData()) );
               break;
            case BXSVariableEntry::cStringVariable:
               dumpFile->fprintf(" %s=%s",
                  getVariableTypeName(syscall->getParameter(j)->getType()),
                  (char*)(syscall->getParameter(j)->getData()) );
               break;
            case BXSVariableEntry::cVectorVariable:
            {
               BVector foo=*(BVector*)(syscall->getParameter(j)->getData());
               dumpFile->fprintf("  %s=(%f, %f, %f)",
                  getVariableTypeName(syscall->getParameter(j)->getType()), 
                  foo.x, foo.y, foo.z);
               break;
            }
            default:
               dumpFile->fprintf(" %s=???",
                  getVariableTypeName(syscall->getParameter(j)->getType()) );
               break;
         }

         if (j < syscall->getNumberParameters()-1)
            dumpFile->fprintf(",");
      }
      if (syscall->getNumberParameters() > 0)
         dumpFile->fprintf(" ");
      dumpFile->fprintf(") ID=%d.\r\n", syscall->getID());
   }
   dumpFile->fprintf("\r\n");

   //User Classes.
   dumpFile->fprintf("%d User Classes:\r\n", mData->getNumberUserClasses());
   for (long i=0; i < mData->getNumberUserClasses(); i++)
   {
      BXSUserClassEntry *uce=mData->getUserClassEntry(i);
      if (uce == NULL)
         continue;

      dumpFile->fprintf("  %s:", mSource->getSymbols().getSymbolByID(uce->getSymbolID()) );
      dumpFile->fprintf("    %d Variables.\r\n", uce->getNumberVariables());
      for (long j=0; j < uce->getNumberVariables(); j++)
      {
         switch (uce->getVariable(j)->getType())
         {
            case BXSVariableEntry::cIntegerVariable:
               dumpFile->fprintf("    V[%d]: %s, INT, Value=%d.\r\n",
                  j, mSource->getSymbols().getSymbolByID(uce->getVariable(j)->getSymbolID()),
                  *(long*)(uce->getVariable(j)->getData()) );
               break;
            case BXSVariableEntry::cFloatVariable:
               dumpFile->fprintf("    V[%d]: %s, FLOAT, Value=%f.\r\n",
                  j, mSource->getSymbols().getSymbolByID(uce->getVariable(j)->getSymbolID()),
                  *(float*)(uce->getVariable(j)->getData()) );
               break;
            case BXSVariableEntry::cBoolVariable:
               dumpFile->fprintf("    V[%d]: %s, BOOL, Value=%d.\r\n",
                  j, mSource->getSymbols().getSymbolByID(uce->getVariable(j)->getSymbolID()),
                  *(bool*)(uce->getVariable(j)->getData()) );
               break;
            case BXSVariableEntry::cStringVariable:
               dumpFile->fprintf("    V[%d]: %s, STRING, Value=%s.\r\n",
                  j, mSource->getSymbols().getSymbolByID(uce->getVariable(j)->getSymbolID()),
                  (char*)(uce->getVariable(j)->getData()) );
               break;
            case BXSVariableEntry::cVectorVariable:
            {
               BVector foo=*(BVector*)(uce->getVariable(j)->getData());
               dumpFile->fprintf("    V[%d]: %s, VECTOR, Value=(%f, %f, %f).\r\n",
                  j, mSource->getSymbols().getSymbolByID(uce->getVariable(j)->getSymbolID()),
                  foo.x, foo.y, foo.z);
               break;
            }
            default:
               dumpFile->fprintf("    V[%d]: %s, Type=%s, Value=???.\r\n",
                  j, mSource->getSymbols().getSymbolByID(uce->getVariable(j)->getSymbolID()),
                  getVariableTypeName(uce->getVariable(j)->getType()) );
               break;
         }
      }
   }
   dumpFile->fprintf("\r\n");

   //Cleanup.
   dumpFile->fprintf("\r\n\r\n");
}

//==============================================================================
// BXSCompiler::getSyscallName
//==============================================================================
const char* BXSCompiler::getSyscallName(long id)
{
   if ((id < 0) || (id > mSyscalls->getNumberSyscalls()) )
      return(NULL);
   return(mSource->getSymbols().getSymbolByID(mSyscalls->getSyscall(id)->getSymbolID()));
}

//==============================================================================
// BXSCompiler::getFunctionName
//==============================================================================
const char* BXSCompiler::getFunctionName(long id)
{
   BXSFunctionEntry *fe=mData->getFunctionEntry(id);
   if (fe == NULL)
      return(NULL);
   return(mSource->getSymbols().getSymbolByID(fe->getSymbolID()));
}
 
//==============================================================================
// BXSCompiler::getVariableName
//==============================================================================
const char* BXSCompiler::getVariableName(long id)
{
   BXSVariableEntry *ve=NULL;
   if (id >= 0)
   {
      BXSFunctionEntry *fe=mData->getFunctionEntry(mCurrentFunctionID);
      if (fe == NULL)
         return(NULL);
      ve=fe->getVariable(id);
   }
   else
   {
      long realID=-id-1;
      if ((realID < 0) || (realID >= mData->getNumberVariables()) )
         return(NULL);
      ve=mData->getVariableEntry(realID);
   }

   if (ve == NULL)
      return(NULL);
   return(mSource->getSymbols().getSymbolByID(ve->getSymbolID()));
}

//==============================================================================
// BXSCompiler::getVariableTypeName
//==============================================================================
const char* BXSCompiler::getVariableTypeName(long variableType)
{
   static char vd[]="void";
   static char i[]="int";
   static char f[]="float";
   static char b[]="bool";
   static char s[]="string";
   static char vector[]="vector";
   static char u[]="unknown";

   switch (variableType)
   {
      case BXSVariableEntry::cVoidVariable:
         return(vd);
      case BXSVariableEntry::cIntegerVariable:
         return(i);
      case BXSVariableEntry::cFloatVariable:
         return(f);
      case BXSVariableEntry::cBoolVariable:
         return(b);
      case BXSVariableEntry::cStringVariable:
         return(s);
      case BXSVariableEntry::cVectorVariable:
         return(vector);
   }

   return(u);
}

//==============================================================================
// BXSCompiler::getLabelName
//==============================================================================
const char* BXSCompiler::getLabelName(long id)
{
   if ((id < 0) || (id > mLabels.getNumber()) )
      return(NULL);
   return(mSource->getSymbols().getSymbolByID(mLabels[id].mSymbolID));
}

//==============================================================================
// BXSCompiler::prepareCompilation
//==============================================================================
bool BXSCompiler::prepareCompilation(const BSimString &listPrefix, const BSimString &filename,
   const BSimString &qualifiedFilename)
{
   mMessenger->infoMsg("//==============================================================================");
   mMessenger->infoMsg("Info 0000: PrepareCompilation: file '%s'.", filename.getPtr());

   //Setup the listing file.
   if (mListing == true)
   {
      //Create the list filename.
      BSimString listFilename;
      if (listPrefix.length() > 0)
         listFilename.format(B("%s%s.lxs"), listPrefix.getPtr(), filename.getPtr());
      else
         listFilename.format(B("%s.lxs"), filename.getPtr());

      //Allocate the file.
      mListFile=new BFile;
      //Open it.
      bool fileOpened = mListFile->openWriteable(mBaseUWDirID, listFilename, BFILE_OPEN_ENABLE_BUFFERING);
      if ((mListFile == NULL) || !fileOpened)
         mMessenger->warningMsg("Warning 0001: unable to open the listing file '%s'.", listFilename.getPtr());
      else
      {
         mMessenger->infoMsg("Info 0002: Generating listing file '%s'.", listFilename.getPtr());
         mEmitter->setListFile(mListFile);
         mEmitter->addListLine(true, "//====================");
         mEmitter->addListLine(true, "// '%s':", qualifiedFilename.getPtr());
         mEmitter->addListLine(true, "//====================");
      }
   }

   return(true);
}

//==============================================================================
// BXSCompiler::finishCompilation
//==============================================================================
bool BXSCompiler::finishCompilation(const BSimString &listPrefix, const BSimString &filename)
{
   //Backpatch the jumps/labels and overloads.
   if (fixupLabelsAndOverloads() == false)
   {
      mMessenger->errorMsg("Error 0004: labels could not be fixed up.");
      return(0);
   }

   //Optimize the quads.
   /*if (mOptimizer->optimizeQuads(mQuads) == false)
   {
      mMessenger->errorMsg("Error 0005: optimization failed.");
      return(0);
   }
   if (mMessenger->getInfo() == true)
      mMessenger->infoMsg("Info 0003: %d quad(s) optimized with %d byte(s) saved",
         mOptimizer->getNumberQuadsOptimized(), mOptimizer->getNumberBytesSaved());*/

   //Close the list file if we have one open.
   if (mListFile != NULL)
   {
      if (mEmitter != NULL)
         mEmitter->setListFile(NULL);
      mListFile->close();
      delete mListFile;
      mListFile=NULL;
   }

   //Dump tables if we need to.
   if (mMessenger->getInfo() == true)
   {
      //Open the dump file.
      BSimString dumpFilename;
      if (listPrefix.length() > 0)
         dumpFilename.format(B("%s%s.dmp.txt"), listPrefix.getPtr(), filename.getPtr());
      else
         dumpFilename.format(B("%s.dmp.txt"), filename.getPtr());
      BFile dumpFile;
      bool ok = dumpFile.openWriteable(mBaseUWDirID, dumpFilename, BFILE_OPEN_ENABLE_BUFFERING);
      if (ok)
         dumpTables(&dumpFile);
   }

   return(true);
}

//==============================================================================
// BXSCompiler::parseSource
//==============================================================================
bool BXSCompiler::parseSource(const BSimString &qualifiedFilename, const BSimString &filename,
   const char *source, long sourceLength)
{
   //Bomb check.
   if (source == NULL)
   {
      mMessenger->errorMsg("Error 0009: invalid source.");
      return(false);
   }

   //If we already have a tokenizer active that has the same filename, fail.
   if (getTokenizer(qualifiedFilename) != NULL)
   {
      mMessenger->errorMsg("Error 0010: cannot have circular file includes.");
      return(false);
   }

   //Allocate a tokenizer for this source.
   BXSTokenizer *t=createTokenizer();
   if (t == NULL)
   {
      mMessenger->errorMsg("Error 0011: allocated tokenizer is NULL.");
      return(false);
   }

   //Info output.
   mMessenger->runMsg("Info 0001: Compiling file '%s'.", qualifiedFilename.getPtr());

   //Do the tokenize.
   getTokenizer()->setSource(qualifiedFilename, source, sourceLength);
   getTokenizer()->tokenize();

   //Add this file to our list of files.
   BXSFileEntry *newFE=mSource->allocateFileEntry();
   if (newFE == NULL)
   {
      srcErrMsg("Error 0303: failed to create file entry.");
      return(false);
   }
   //Set the filename for the file entry.
   if (newFE->setFilename(qualifiedFilename.getPtr()) == false)
   {
      srcErrMsg("Error 0304: failed to set filename in file entry.");
      return(false);
   }

   //Add a symbol for the NON-QUALIFIED filename.
   long symbolID=mSource->getSymbols().addSymbol(filename.getPtr(), BXSBinary::cFilename, newFE->getID());
   if (symbolID < 0)
   {
      srcErrMsg("Error 0320: unable to add a symbol for this filename.");
      return(false);
   }

   //Set the file's ID into the tokenizer (so that we can restore it if we end up
   //pushing another tokenizer onto the stack).
   getTokenizer()->setFileID(newFE->getID());
   //If we're doing debug code, add the source to the file entry, too.
   if (mDebugCode == true)
   {
      if (newFE->setSource(source) == false)
      {
         srcErrMsg("Error 0307: failed to set source in file entry.");
         return(false);
      }
   }

   //Parse this file's code.
   parse();

   //Remove the tokenizer.  This will cause the file ID to get reset.
   popTokenizer();

   //Finish up.
   if (mMessenger->getErrorCount() > 0)
   {
      mMessenger->infoMsg("//==============================================================================");
      mMessenger->runMsg("%d error(s) found in file '%s'.", mMessenger->getErrorCount(), filename.getPtr());
		

      return(false);
   }
   mMessenger->runMsg("Info 0004: No errors found in file '%s'.", filename.getPtr());
   mMessenger->infoMsg("//==============================================================================\n\n");

   return(true);
}

//==============================================================================
// BXSCompiler::getTokenizer
//==============================================================================
BXSTokenizer* BXSCompiler::getTokenizer(void) const
{
   if (mTokenizers.getNumber() <= 0)
      return(mBaseTokenizer);
   return(mTokenizers[mTokenizers.getNumber()-1]);
}

//==============================================================================
// BXSCompiler::getTokenizer
//==============================================================================
BXSTokenizer* BXSCompiler::getTokenizer(const BSimString &filename) const
{
   for (long i=0; i < mTokenizers.getNumber(); i++)
   {
      if ((filename == mTokenizers[i]->getFilename()) ||
         (filename == mTokenizers[i]->getFilenameWithoutExtension()) )
         return(mTokenizers[i]);
   }

   return(NULL);
}

//==============================================================================
// BXSCompiler::createTokenizer
//==============================================================================
BXSTokenizer* BXSCompiler::createTokenizer(void)
{
   BXSTokenizer *t=NULL;
   if (mUnusedTokenizers.getNumber() <= 0)
   {
      t=new BXSTokenizer(mMessenger);
      if (t != NULL)
      {
         t->initialize();
         t->setDebug(mDebugTokenizer);
      }
   }
   else
   {
      t=mUnusedTokenizers[mUnusedTokenizers.getNumber()-1];
      mUnusedTokenizers.setNumber(mUnusedTokenizers.getNumber()-1);
   }

   if (t == NULL)
      return(NULL);

   if (mTokenizers.add(t) == -1)
   {
      delete t;
      return(NULL);
   }

   return(t);
}

//==============================================================================
// BXSCompiler::popTokenizer
//==============================================================================
bool BXSCompiler::popTokenizer(void)
{
   if (mTokenizers.getNumber() <= 0)
      return(false);

   BXSTokenizer *t=mTokenizers[mTokenizers.getNumber()-1];
   mTokenizers.setNumber(mTokenizers.getNumber()-1);

   if (mUnusedTokenizers.add(t) == -1)
   {
      delete t;
      return(false);
   }

   return(true);
}

//==============================================================================
// BXSCompiler::parse
//==============================================================================
void BXSCompiler::parse(void)
{
   #ifdef DEBUGTRACEPARSE
   mMessenger->infoMsg("Parse.");
   #endif

   //Loop until the keyToken check breaks us out.
   for (;;)
   {
      //Set our scope back to global.
      bsnprintf(mCurrentScope, sizeof(mCurrentScope), "%s$", getTokenizer()->getFilenameWithoutExtension().getPtr());
      mCurrentFunctionID=-1;

      //Figure out whether or not we have a variable declaration or a function definition.
      //Don't let it go forever.
      long index=0;
      long keyToken=-1;
      while ((keyToken != BXSTokenizer::cAssign) &&
         (keyToken != BXSTokenizer::cLeftParen) &&
         (keyToken != BXSTokenizer::cInclude) &&
         (keyToken != BXSTokenizer::cEOS) &&
         (keyToken != BXSTokenizer::cRule) &&
         (keyToken != BXSTokenizer::cClass) &&
         (index < 10000))
      {
         keyToken=getTokenizer()->getFutureToken(index, false);
         index++;
      }

      //Token inc or not.
      bool incToken=true;
      if (getTokenizer()->getCurrentTokenNumber() <= 0)
         incToken=false;

      //Include.
      if (keyToken == BXSTokenizer::cInclude)
      {
         if (parseInclude(incToken) == false)
            break;
      }
      //Variable.
      else if (keyToken == BXSTokenizer::cAssign)
      {
         //Parse the global variable.  We reuse the parseVarDecl function
         //because we can and it gives us 'easy' reuse of the initial value
         //setting, etc.  We DO NOT want the quads that it generates, though,
         //so we clean the quads out when this is done (otherwise, they'll
         //show up in the first function, which is, um, bad).
         if (parseVarDecl(incToken, true, false, true) == false)
            break;
         //Clear the quads out.
         mQuads.setNumber(0);
      }
      //Function.
      else if (keyToken == BXSTokenizer::cLeftParen)
      {
         if (parseFunctionDecl(incToken) == false)
            break;
      }
      //Rule.
      else if (keyToken == BXSTokenizer::cRule)
      {
         if (parseRule(incToken) == false)
            break;
      }
      //Class.
      else if (keyToken == BXSTokenizer::cClass)
      {
         if (parseClassDefinition(incToken) == false)
            break;
      }
      //End.
      else if (keyToken == BXSTokenizer::cEOS)
         break;
      //Error.
      else
      {
         srcErrMsg("Error 0012: found '%s' when looking for a variable/function/rule declaration or an include statement.", getTokenizer()->getCurrentTokenText());
         break;
      }
   }
}

//==============================================================================
// BXSCompiler::parseVarDecl
//==============================================================================
bool BXSCompiler::parseVarDecl(bool incrementTokenFirst, bool stripSemiColon,
   bool parameter, bool forceConstantInitialValue)
{
   #ifdef DEBUGTRACEPARSE
   mMessenger->infoMsg("ParseVarDecl.");
   #endif

   //Get the token we need to look at.
   long token;
   if (incrementTokenFirst == true)
      token=getTokenizer()->getDarkSpaceToken(this);
   else
      token=getTokenizer()->getCurrentToken();

   //Parse any qualifiers we might have.
   bool exportVariable=false;
   bool externVariable=false;
   bool constVariable=false;
   bool staticVariable=false;
   while ((token == BXSTokenizer::cExport) ||
      (token == BXSTokenizer::cExtern) ||
      (token == BXSTokenizer::cStatic) ||
      (token == BXSTokenizer::cConst))
   {
      switch (token)
      {
         case BXSTokenizer::cExport:
            //Check the simple stuff.
            if ((parameter == true) || (mCurrentFunctionID >= 0))
            {
               mMessenger->errorMsg("Error 0347: Cannot 'export' a parameter or function variable.");
               return(false);
            }
            //If const or static has been set already, badness.
            if ((constVariable == true) || (staticVariable == true))
            {
               mMessenger->errorMsg("Error 0348: Cannot 'export' a 'const' or 'static' variable.");
               return(false);
            }
               
            exportVariable=true;
            externVariable=true;
            break;

         case BXSTokenizer::cExtern:
            //Check the simple stuff.
            if ((parameter == true) || (mCurrentFunctionID >= 0))
            {
               mMessenger->errorMsg("Error 0349: Cannot 'extern' a parameter or function variable.");
               return(false);
            }
            //If static has been set already, badness.
            if (staticVariable == true)
            {
               mMessenger->errorMsg("Error 0350: Cannot 'extern' a 'static' variable.");
               return(false);
            }

            externVariable=true;
            break;

         case BXSTokenizer::cStatic:
            //Check the simple stuff.
            if ((parameter == true) || (mCurrentFunctionID < 0))
            {
               mMessenger->errorMsg("Error 0351: Cannot 'static' a parameter or non-function variable.");
               return(false);
            }
            //Cannot be combined with export or const.
            if ((exportVariable == true) || (externVariable == true) || (constVariable == true))
            {
               mMessenger->errorMsg("Error 0352: Cannot 'static' an 'export', 'extern', or 'const' variable.");
               return(false);
            }
            staticVariable=true;
            break;

         case BXSTokenizer::cConst:
            //Cannot be combined with export or static.
            if ((exportVariable == true) || (staticVariable == true))
            {
               mMessenger->errorMsg("Error 0353: Cannot 'const' an 'export' or 'static' variable.");
               return(false);
            }

            constVariable=true;
            break;
      }

      //Get the next token.
      token=getTokenizer()->getDarkSpaceToken(this);
   }

   //Get the type name.
   BYTE variableType=parseVariableTypeName(token, false);
   if (variableType == BXSVariableEntry::cInvalidVariable)
   {
      srcErrMsg("Error 0013: '%s' is not a valid variable type.", getTokenizer()->getCurrentTokenText());
      return(false);
   }

   //Get the variable name.  It's an error if it's already defined.
   token=getTokenizer()->getDarkSpaceToken(this);
   const BXSSymbolEntry *se=getSymbolEntry(getTokenizer()->getCurrentTokenText(), true);
   if (se != NULL)
   {
      srcErrMsg("Error 0014: '%s' is already defined.", getTokenizer()->getCurrentTokenText());
      return(false);
   }
   //Create it.
   char variableName[(BXSTokenizer::cMaxTokenSize+1)*2];
   //If we have an extern variable, we don't put any scope qualifier on it.
   if (externVariable == true)
      bsnprintf(variableName, sizeof(variableName), "EXTERN$%s", getTokenizer()->getCurrentTokenText());
   else
   {
      BXSUserClassEntry *uce=mData->getUserClassEntry(mCurrentClassID);
      if (uce != NULL)
         bsnprintf(variableName, sizeof(variableName), "%s$%s", mSource->getSymbols().getSymbolByID(uce->getSymbolID()), getTokenizer()->getCurrentTokenText());
      else
         bsnprintf(variableName, sizeof(variableName), "%s%s", mCurrentScope, getTokenizer()->getCurrentTokenText());
   }

   //Allocate the new variable entry.
   BXSVariableEntry *newVE=new BXSVariableEntry();
   if (newVE == NULL)
   {
      srcErrMsg("Error 0015: failed to create variable entry.");
      return(false);
   }
   //Set the qualifiers.
   newVE->setExport(exportVariable);
   newVE->setExtern(externVariable);
   newVE->setConst(constVariable);
   newVE->setStatic(staticVariable);

   //Default value.  If we're doing a parameter, we have a const variable, we have a
   //static variable, or we are forcing a constant initial value (i.e. not an expression),
   //we have to have a constant initial value.  Else, we "default" the variable to the
   //logical 0 and are allowed to do a variable expression assignment.
   if ((parameter == true) || (constVariable == true) ||
      (forceConstantInitialValue == true) || (staticVariable == true))
   {
      //'='
      token=getTokenizer()->getDarkSpaceToken(this);
      if (token != BXSTokenizer::cAssign) 
      {
         srcErrMsg("Error 0016: expected '=' and found '%s' (you must initialize all variables to a literal or const value).", getTokenizer()->getCurrentTokenText());
         return(false);
      }

      switch (variableType)
      {
         case BXSVariableEntry::cIntegerVariable:
         {
            long defaultValue;
            if (parseIntegerConstant(&defaultValue) == false)
            {
               srcErrMsg("Error 0017: parseIntegerConstant failed.");
               return(false);
            }
            if (newVE->setAll(-1, variableType, &defaultValue) == false)
            {
               srcErrMsg("Error 0018: setAll failed.");
               return(false);
            }
            break;
         }
         case BXSVariableEntry::cFloatVariable:
         {
            float defaultValue;
            if (parseFloatConstant(&defaultValue) == false)
            {
               srcErrMsg("Error 0019: parseFloatConstant failed.");
               return(false);
            }
            if (newVE->setAll(-1, variableType, &defaultValue) == false)
            {
               srcErrMsg("Error 0020: setAll failed.");
               return(false);
            }
            break;
         }
         //Bool.
         case BXSVariableEntry::cBoolVariable:
         {
            bool defaultValue;
            if (parseBoolConstant(&defaultValue) == false)
            {
               srcErrMsg("Error 0021: parseBoolConstant failed.");
               return(false);
            }
            if (newVE->setAll(-1, variableType, &defaultValue) == false)
            {
               srcErrMsg("Error 0022: setAll failed.");
               return(false);
            }
            break;
         }
         //String.
         case BXSVariableEntry::cStringVariable:
            token=getTokenizer()->getDarkSpaceToken(this);
            if (token != BXSTokenizer::cString)
            {
               srcErrMsg("Error 0023: expected a string constant and found '%s'.", getTokenizer()->getCurrentTokenText());
               return(false);
            }
            if (newVE->setAll(-1, variableType, getTokenizer()->getCurrentTokenText()) == false)
            {
               srcErrMsg("Error 0024: setAll failed.");
               return(false);
            }
            break;
         //Vector.
         case BXSVariableEntry::cVectorVariable:
         {
            //We have to kludgily get the token first.
            token=getTokenizer()->getDarkSpaceToken(this);

            //This vector has to be a constant since this is a var declaration.
            BVector defaultValue;
            if (parseVectorConstant(&defaultValue) == false)
            {
               srcErrMsg("Error 0026: parseVectorConstant failed.");
               return(false);
            }
            if (newVE->setAll(-1, variableType, &defaultValue) == false)
            {
               srcErrMsg("Error 0027: setAll failed.");
               return(false);
            }
            break;
         }

         default:
            srcErrMsg("Error 0028: don't understand the default type for this variable.");
            return(false);
      }

      //';'.
      if (stripSemiColon == true)
      {
         token=getTokenizer()->getDarkSpaceToken(this);
         if (token != BXSTokenizer::cSemiColon) 
         {
            srcErrMsg("Error 0029: expected ';' and found '%s'.", getTokenizer()->getCurrentTokenText());
            return(false);
         }
      }
   }
   else
   {
      //Default value.
      switch (variableType)
      {
         case BXSVariableEntry::cIntegerVariable:
         {
            long defaultValue=0;
            if (newVE->setAll(-1, variableType, &defaultValue) == false)
            {
               srcErrMsg("Error 0284: setAll failed.");
               return(false);
            }
            break;
         }
         case BXSVariableEntry::cFloatVariable:
         {
            float defaultValue=0.0f;
            if (newVE->setAll(-1, variableType, &defaultValue) == false)
            {
               srcErrMsg("Error 0285: setAll failed.");
               return(false);
            }
            break;
         }
         //Bool.
         case BXSVariableEntry::cBoolVariable:
         {
            bool defaultValue=false;
            if (newVE->setAll(-1, variableType, &defaultValue) == false)
            {
               srcErrMsg("Error 0286: setAll failed.");
               return(false);
            }
            break;
         }
         //String.
         case BXSVariableEntry::cStringVariable:
            if (newVE->setAll(-1, variableType, "DSV") == false)
            {
               srcErrMsg("Error 0287: setAll failed.");
               return(false);
            }
            break;
         //Vector.
         case BXSVariableEntry::cVectorVariable:
         {
            BVector defaultValue(0.0f);
            if (newVE->setAll(-1, variableType, &defaultValue) == false)
            {
               srcErrMsg("Error 0288: setAll failed.");
               return(false);
            }
            break;
         }
         default:
            srcErrMsg("Error 0289: don't understand the default type for this variable.");
            return(false);
      }
   }

   //Add this to the global variable array if we have a global variable, to the 
   //current function if we have one, or to the current class if we have one of
   //those.
   if (mCurrentClassID >= 0)
   {
      BXSUserClassEntry *uce=mData->getUserClassEntry(mCurrentClassID);
      if (uce == NULL)
         return(false);
      long newVariableID=uce->getNumberVariables();

      //Add it to the symbol table.  The symbol type is cClassVariable.  The ID of variable is
      //the value.  addSymbol returns the unique ID for this variable, so we stuff that
      //into the ID slot in the variable table so that we can reference it later.
      long symbolID=mSource->getSymbols().addSymbol(variableName, BXSBinary::cClassVariable, newVariableID);
      if (symbolID < 0)
      {
         srcErrMsg("Error 0379: unable to add an entry to the symbol table for this class member variable.");
         return(false);
      }

      //Set the symbol ID now that we have it.
      newVE->setSymbolID(symbolID);
      //Add it to the class.
      uce->addVariable(newVE);
   }
   else if (mCurrentFunctionID >= 0)
   {
      BXSFunctionEntry *fe=mData->getFunctionEntry(mCurrentFunctionID);
      if (fe == NULL)
         return(false);
      long newVariableID=fe->getNumberVariables();

      //Add it to the symbol table.  The symbol type is cStackVariable.  The ID of variable is
      //the value.  addSymbol returns the unique ID for this variable, so we stuff that
      //into the ID slot in the variable table so that we can reference it later.
      long symbolID=mSource->getSymbols().addSymbol(variableName, BXSBinary::cStackVariable, newVariableID);
      if (symbolID < 0)
      {
         srcErrMsg("Error 0032: unable to add an entry to the file symbol table for this stack variable.");
         return(false);
      }

      //Set the symbol ID now that we have it.
      newVE->setSymbolID(symbolID);
      //Add it to the function.
      if (parameter == true)
         fe->addParameter(newVE);
      else
         fe->addVariable(newVE);
   }
   else
   {
      long newVariableID=mData->addVariableEntry(newVE);

      //Add it to the symbol table.  The symbol type is cVariable.  The ID of variable is
      //the value (see NOTE).  addSymbol returns the unique ID for this variable, so we
      //stuff that into the ID slot in the variable table so that we can reference it later.
      //NOTE: In order to unify the opcodes, we negate and offset the global variable IDs
      //in everything.  We negate and subtract one (to offset the 0th variable to -1).
      long symbolID=mSource->getSymbols().addSymbol(variableName, BXSBinary::cVariable, -newVariableID-1);
      if (symbolID < 0)
      {
         srcErrMsg("Error 0031: unable to add an entry to the file symbol table for this global variable.");
         return(false);
      }

      //Set the symbol ID now that we have it.
      newVE->setSymbolID(symbolID);
   }

   //Now, parse the assignment (if we're not doing a constant assignment, a const variable,
   //or a static variable).
   if ((parameter == false) && (constVariable == false) &&
      (forceConstantInitialValue == false) && (staticVariable == false))
   {
      if (parseVariableAssignment(token, getTokenizer()->getCurrentTokenText(), stripSemiColon, true) == false)
      {
         srcErrMsg("Error 0290: parseVariableAssignment failed.");
         return(false);
      }
   }

   return(true);
}

//==============================================================================
// BXSCompiler::parseFunctionDecl
//==============================================================================
bool BXSCompiler::parseFunctionDecl(bool incrementTokenFirst)
{
   #ifdef DEBUGTRACEPARSE
   mMessenger->infoMsg("ParseFunctionDecl.");
   #endif

   //NOTE: parseRule is a lot like this function (since rules are basically functions).
   //If you make a change to how functions work here, check parseRule to make sure
   //that you change that appropriately.

   //Start code offset.
   long codeOffset=mSource->getCodeSize();

   //Get the first token.
   long token;
   if (incrementTokenFirst)
      token=getTokenizer()->getDarkSpaceToken(this);
   else
      token=getTokenizer()->getCurrentToken();
   //Check extern.
   /*bool externFunction=false;
   if (token == BXSTokenizer::cExtern)
   {
      externFunction=true;
      token=getTokenizer()->getDarkSpaceToken(this);
   }*/
   //Check mutable.
   bool mutableFunction=false;
   if (token == BXSTokenizer::cMutable)
   {
      mutableFunction=true;
      token=getTokenizer()->getDarkSpaceToken(this);
   }
   //Return type.
   BYTE returnType=parseVariableTypeName(token, true);
   if (returnType == BXSVariableEntry::cInvalidVariable)
   {
      srcErrMsg("Error 0033: '%s' is not a valid return type.", getTokenizer()->getCurrentTokenText());
      return(false);
   }

   //Get the function name.  It's an error if it's already defined.
   bool replaceFunction=false;
   token=getTokenizer()->getDarkSpaceToken(this);
   const BXSSymbolEntry *se=getSymbolEntry(getTokenizer()->getCurrentTokenText(), false);
   if (se != NULL)
   {
      //Get the mutable-ness of the old function.
      if ((se->mValue >= 0) && (se->mValue < mData->getNumberFunctions()) )
         mutableFunction=mData->getFunctionEntry(se->mValue)->getMutable();

      if (mutableFunction == false)
      {
         srcErrMsg("Error 0034: '%s' is already defined and cannot be a function name (use 'mutable' to allow overriding).", getTokenizer()->getCurrentTokenText());
         return(false);
      }
      replaceFunction=true;
   }

   //Create the new function entry.
   BXSFunctionEntry *newFE=mData->allocateFunctionEntry();
   if (newFE == NULL)
   {
      srcErrMsg("Error 0035: cannot create new function entry.");
      return(false);
   }

   //Create the function name and set the scope.
   char functionName[BXSTokenizer::cMaxTokenSize+1];
   StringCchCopyA(functionName, BXSTokenizer::cMaxTokenSize+1, getTokenizer()->getCurrentTokenText());
   /*if (externFunction == true)
      bsnprintf(mCurrentScope, sizeof(mCurrentScope), "EXTERN$%s$", functionName);
   else*/
      bsnprintf(mCurrentScope, sizeof(mCurrentScope), "%s$%s%d$",
         getTokenizer()->getFilenameWithoutExtension().getPtr(), functionName, newFE->getID());

   //This is now the current function.
   mCurrentFunctionID=newFE->getID();

   //Add some debug listing.
   mEmitter->addListLine(true, "");
   mEmitter->addListLine(true, "//====================");
   mEmitter->addListLine(true, "// Function '%s':", functionName);
   mEmitter->addListLine(true, "//====================");

   //Add the symbol for the function name.  The symbol type is cFunction.  The value
   //is the unique ID for the function.  addSymbol returns the unique symbol ID for
   //this variable, so we stuff that into the ID slot in the variable table so that
   //we can reference it later.
   long symbolID=-1;
   long replacedFunctionID=-1;
   if (replaceFunction == false)
   {
      symbolID=mSource->getSymbols().addSymbol(functionName, BXSBinary::cFunction, newFE->getID());
      if (symbolID < 0)
      {
         srcErrMsg("Error 0037: unable to add an entry to the symbol table.");
         return(false);
      }
   }
   else
   {
      //Save the old FID.
      replacedFunctionID=se->mValue;
      //Add the overload.
      if (addFunctionOverload(se->mValue, newFE->getID()) == false)
      {
         srcErrMsg("Error 0363: unable to add a function overload entry.");
         return(false);
      }
      //Symbol ID is the same as the old declaration.
      symbolID=se->mID;
      //Update the symbol value, though.
      mSource->getSymbols().setSymbolValue(symbolID, newFE->getID());
   }

   //Set the data for the function.
   newFE->setSymbolID(symbolID);
   newFE->setReturnType(returnType);
   newFE->setFileID(getTokenizer()->getFileID());
   //Carry the mutable-ness of the function forward.
   newFE->setMutable(mutableFunction);

   //Check for the main function.
   if (stricmp(functionName, "main") == 0)
   {
      if (mData->getMainFunctionID() != -1)
      {
         srcErrMsg("Error 0038: the 'main' function has already been defined.");
         return(false);
      }
      mData->setMainFunctionID(newFE->getID());
   }

   //'('
   token=getTokenizer()->getDarkSpaceToken(this);
   if (token != BXSTokenizer::cLeftParen) 
   {
      srcErrMsg("Error 0039: expected '(' and found '%s'.", getTokenizer()->getCurrentTokenText());
      return(false);
   }

   //Get the function parms until we get to the ')'.
   token=getTokenizer()->getDarkSpaceToken(this);
   while (token != BXSTokenizer::cRightParen)
   {
      //Make sure we have a variable type name token.
      /*if (token != BXSTokenizer::cName)
      {
         srcErrMsg("Error 0040: found '%s' when looking for a parameter type name.");
         return(false);
      }*/

      //Get the value of the variable type symbol.  It should be an index between 0 and 
      //the number of variable types.
      BYTE parmType=parseVariableTypeName(token, true);
      if (parmType == BXSVariableEntry::cInvalidVariable)
      {
         srcErrMsg("Error 0041: '%s' is not a valid parameter type.", getTokenizer()->getCurrentTokenText());
         return(false);
      }

      //If we have a void type and it's our first arg, we expect the ')' next.
      if (parmType == BXSVariableEntry::cVoidVariable)
      {
         token=getTokenizer()->getDarkSpaceToken(this);
         if (token != BXSTokenizer::cRightParen)
         {
            srcErrMsg("Error 0043: '%s' is not a valid parameter type.", getTokenizer()->getCurrentTokenText());
            return(false);
         }

         break;
      }

      //Main cannot take any arguments.
      if (newFE->getID() == mData->getMainFunctionID())
      {
         srcErrMsg("Error 0044: the main function cannot take any arguments.");
         return(false);
      }

      //Parse the parameter and its initialization.  We have to push the type name
      //back on the tokenizer, first, though.
      if (parseVarDecl(false, false, true, true) == false)
      {
         srcErrMsg("Error 0045: could not parse parameter declaration.");
         return(false);
      }

      //Get the next token.  If it's a ')', we're done.  Else, it should be a comma.
      token=getTokenizer()->getDarkSpaceToken(this);
      if (token == BXSTokenizer::cRightParen)
         break;
      if (token != BXSTokenizer::cComma)
      {
         srcErrMsg("Error 0046: found '%s' when ',' was expected.", getTokenizer()->getCurrentTokenText());
         return(false);
      }
      //Skip the comma.
      token=getTokenizer()->getDarkSpaceToken(this);
   }

   //If we're trying to replace a function, the two function entries must match return
   //type and full parm lists (type and number).
   if (replaceFunction == true)
   {
      if (replacedFunctionID < 0)
      {
         srcErrMsg("Error 0365: wacky error where replaced function doesn't exist.  Nutty.");
         return(false);
      }
      //Get some pointers to make this easier.
      BXSFunctionEntry *replacedFE=mData->getFunctionEntry(replacedFunctionID);
      BXSFunctionEntry *overrideFE=newFE;

      //Validate return values.
      if (replacedFE->getReturnType() != overrideFE->getReturnType())
      {
         srcErrMsg("Error 0366: Mismatched return types on function overload.");
         return(false);
      }
      //Validate number parms.
      if (replacedFE->getNumberParameters() != overrideFE->getNumberParameters())
      {
         srcErrMsg("Error 0367: Mismatched number of parameters on function overload.");
         return(false);
      }
      //Ensure each parm matches type.
      for (long a=0; a < replacedFE->getNumberParameters(); a++)
      {
         if (replacedFE->getParameterType(a) != overrideFE->getParameterType(a))
         {
            srcErrMsg("Error 0368: Mismatched parameter type on paramater #%d on function overload.", a);
            return(false);
         }
      }
      //Else, we're good.
   }

   //Parse the code inside the function.
   bool returnAdded=false;
   if (parseCode(false, true, &returnAdded) == false)
   {
      #ifdef DEBUGEMITLISTSONERROR
      emitTheLists(mQuads, &codeOffset);
      #endif
      srcErrMsg("Error 0047: could not parse the code for '%s' function.", functionName);
      return(false);
   }

   //Add the RET quad (if there wasn't one already added).
   if (returnAdded == false)
   {
      //If we have a non-void return value, this means we have a problem because
      //we won't be actually returning anything.
      if (returnType != BXSVariableEntry::cVoidVariable)
      {
         srcErrMsg("Error 0404: '%s' is a non-void function, but does not return anything.", functionName);
         return(false);
      }
   
      if (mQuads.add(BXSQuad(BXSQuadOpcode::cRET)) == -1)
      {
         srcErrMsg("Error 0048: could not add RET quad for '%s' function.", functionName);
         return(false);
      }
   }

   //Now that we've parsed the function, we need to emit the quads for it.
   if (mQuads.getNumber() > 0)
   {
      if ((emitTheLists(mQuads, &codeOffset) == false) || (codeOffset < 0))
      {
         srcErrMsg("Error 0049: could not emit quads for '%s' function.", functionName);
         return(false);
      }
   }
   //Empty the quads.
   mQuads.setNumber(0);
   //Now that we have a valid code offset, save it in the function.
   newFE->setCodeOffset(codeOffset);

   return(true);
}

//==============================================================================
// BXSCompiler::parseConditionDecl
//==============================================================================
bool BXSCompiler::parseConditionDecl(void)
{
   #ifdef DEBUGTRACEPARSE
   mMessenger->infoMsg("ParseConditionDecl.");
   #endif

   //Initialize.
   long token=getTokenizer()->getDarkSpaceToken(this);

   //Create the labels for the then and else jumps.
   long endLabelID=createLabel(NULL, true);
   long elseLabelID=createLabel(NULL, true);

   //Parse the conditional expression.
   if (parseExpression(true, true, false, false, false, true, BXSVariableEntry::cBoolVariable) == false)
   {
      srcErrMsg("Error 0050: parseExpr failed for condition.");
      return(false);
   }

   //Add the JUMPZ quad.
   if (mQuads.add(BXSQuad(BXSQuadOpcode::cJUMPZ, 1, elseLabelID)) == -1)
   {
      srcErrMsg("Error 0051: could not add JUMPZ quad.");
      return(false);
   }

   //Parse the THEN block.
   if (parseCode(true, false, NULL) == false)
   {
      srcErrMsg("Error 0052: could not parse the code for THEN block.");
      return(false);
   }

   //Grab the next token.
   token=getTokenizer()->getDarkSpaceToken(this);


   //Parse the ELSE (if any).
   if (token == BXSTokenizer::cElse)
   {
      //Add the jump quad so the THEN block can jump over the ELSE.
      if (mQuads.add(BXSQuad(BXSQuadOpcode::cJUMP, 0, endLabelID)) == -1)
      {
         srcErrMsg("Error 0053: could not add JUMP quad for then block.");
         return(false);
      }

      //Add the ELSE label (after the THEN jump:).
      if (mQuads.add(BXSQuad(BXSQuadOpcode::cLABEL, elseLabelID)) == -1)
      {
         srcErrMsg("Error 0054: could not add LABEL quad for else block.");
         return(false);
      }

      //Parse the code in this block.
      if (parseCode(true, false, NULL) == false)
      {
         srcErrMsg("Error 0055: could not parse the code for ELSE block.");
         return(false);
      }

      //Add the corresponding label for the THEN jump.  We push the label up
      //so that it will get put in the 'right' spot before any line quads.
      if (addLabelQuad(endLabelID, true) == false)
      {
         srcErrMsg("Error 0056: could not add LABEL quad for then block.");
         return(false);
      }
   }
   else
   {
      //Push the token back.
      getTokenizer()->decrementCurrentToken(this);

      //Add the ELSE label.  We push the label up so that it will get put
      //in the 'right' spot before any line quads.
      if (addLabelQuad(elseLabelID, true) == false)
      {
         srcErrMsg("Error 0057: could not add LABEL quad for else block.");
         return(false);
      }
   }

   return(true);
}

//==============================================================================
// BXSCompiler::parseLabelDecl
//==============================================================================
bool BXSCompiler::parseLabelDecl(void)
{
   #ifdef DEBUGTRACEPARSE
   mMessenger->infoMsg("ParseLabelDecl.");
   #endif

   //Make sure we have a name token.
   long token=getTokenizer()->getDarkSpaceToken(this);
   if (token != BXSTokenizer::cName)
   {
      srcErrMsg("Error 0058: found '%s' when looking for a label name.", getTokenizer()->getCurrentTokenText());
      return(false);
   }

   //Create the scoped label name.
   char labelName[(BXSTokenizer::cMaxTokenSize+1)*2];
   bsnprintf(labelName, sizeof(labelName), "%s%s", mCurrentScope, getTokenizer()->getCurrentTokenText());

   //';'.
   token=getTokenizer()->getDarkSpaceToken(this);
   if (token != BXSTokenizer::cSemiColon) 
   {
      srcErrMsg("Error 0059: expected ';' and found '%s'.", getTokenizer()->getCurrentTokenText());
      return(false);
   }

   //Lookup the label in the symbol table.
   const BXSSymbolEntry *se=mSource->getSymbols().getEntryBySymbol(labelName);
   //If the label name has already been used as something else, forget it.
   if ((se != NULL) && (se->mType != BXSBinary::cLabel))
   {
      srcErrMsg("Error 0060: '%s' is an invalid label.", getTokenizer()->getCurrentTokenText());
      return(false);
   }
   //Get the label ID.
   long labelID=-1;
   if (se == NULL)
      labelID=createLabel(labelName, false);
   else
      labelID=se->mValue;
   if (labelID < 0)
   {
      srcErrMsg("Error 0061: invalid label ID.");
      return(false);
   }

   //Add a LABEL quad.  F1 is the ID for this label.
   if (mQuads.add(BXSQuad(BXSQuadOpcode::cLABEL, labelID)) == -1)
   {
      srcErrMsg("Error 0062: LABEL quad addition failed.");
      return(false);
   }

   return(true);
}

//==============================================================================
// BXSCompiler::parseGotoDecl
//==============================================================================
bool BXSCompiler::parseGotoDecl(void)
{
   #ifdef DEBUGTRACEPARSE
   mMessenger->infoMsg("ParseGotoDecl.");
   #endif

   //Make sure we have a name token.
   long token=getTokenizer()->getDarkSpaceToken(this);
   if (token != BXSTokenizer::cName)
   {
      srcErrMsg("Error 0063: found '%s' when looking for a label name.", getTokenizer()->getCurrentTokenText());
      return(false);
   }

   //Create the scoped label name.
   char labelName[(BXSTokenizer::cMaxTokenSize+1)*2];
   bsnprintf(labelName, sizeof(labelName), "%s%s", mCurrentScope, getTokenizer()->getCurrentTokenText());
   const BXSSymbolEntry *se=mSource->getSymbols().getEntryBySymbol(labelName);
   //If the label name has already been used as something else, forget it.
   if ((se != NULL) && (se->mType != BXSBinary::cLabel))
   {
      srcErrMsg("Error 0064: '%s' is an invalid label.", getTokenizer()->getCurrentTokenText());
      return(false);
   }
   //Get the label ID.
   long labelID=-1;
   if (se == NULL)
      labelID=createLabel(labelName, false);
   else
      labelID=se->mValue;
   if (labelID < 0)
   {
      srcErrMsg("Error 0065: invalid label ID.");
      return(false);
   }

   //';'.
   token=getTokenizer()->getDarkSpaceToken(this);
   if (token != BXSTokenizer::cSemiColon) 
   {
      srcErrMsg("Error 0066: expected ';' and found '%s'.", getTokenizer()->getCurrentTokenText());
      return(false);
   }

   //Add a JUMP quad.  The F1 is the label ID.
   if (mQuads.add(BXSQuad(BXSQuadOpcode::cJUMP, 1, labelID)) == -1)
   {
      srcErrMsg("Error 0067: JUMP quad addition failed.");
      return(false);
   }

   return(true);
}

//==============================================================================
// BXSCompiler::parseClassDefinition
//==============================================================================
bool BXSCompiler::parseClassDefinition(bool incrementTokenFirst)
{
   #ifdef DEBUGTRACEPARSE
   mMessenger->infoMsg("ParseClassDefinition.");
   #endif

   //We cannot be inside a function.
   if (mCurrentFunctionID >= 0)
   {
      srcErrMsg("Error 0380: classes may not be defined inside a function.");
      return(false);
   }

   //Make sure this is 'class'.
   long token;
   if (incrementTokenFirst == true)
      token=getTokenizer()->getDarkSpaceToken(this);
   else
      token=getTokenizer()->getCurrentToken();
   if (token != BXSTokenizer::cClass)
   {
      srcErrMsg("Error 0369: expected 'class' and found '%s'.", getTokenizer()->getCurrentTokenText());
      return(false);
   }

   //Make sure we have a name token.
   token=getTokenizer()->getDarkSpaceToken(this);
   if (token != BXSTokenizer::cName)
   {
      srcErrMsg("Error 0370: found '%s' when looking for a class name.", getTokenizer()->getCurrentTokenText());
      return(false);
   }

   //Create the scoped class name.
   char className[(BXSTokenizer::cMaxTokenSize+1)*2];
   bsnprintf(className, sizeof(className), "%s", getTokenizer()->getCurrentTokenText());
   const BXSSymbolEntry *se=mSource->getSymbols().getEntryBySymbol(className);
   //If the name has already been used as something else, forget it.
   if (se != NULL)
   {
      srcErrMsg("Error 0371: '%s' is an invalid class name (or is already defined).", getTokenizer()->getCurrentTokenText());
      return(false);
   }

   //Create the new class entry.
   BXSUserClassEntry *newUCE=mData->allocateUserClassEntry();
   if (newUCE == NULL)
   {
      srcErrMsg("Error 0372: cannot create new class entry.");
      return(false);
   }

   //Add it to the symbol table.  The symbol type is cClass.  The ID of class is
   //the value.
   long symbolID=mSource->getSymbols().addSymbol(className, BXSBinary::cClass, newUCE->getID());
   if (symbolID < 0)
   {
      srcErrMsg("Error 0373: unable to add an entry to the symbol table for '%s' class.", getTokenizer()->getCurrentTokenText());
      return(false);
   }
   //Save the symbol ID.
   newUCE->setSymbolID(symbolID);
   //This is now the current class.
   mCurrentClassID=newUCE->getID();

   //'{'
   token=getTokenizer()->getDarkSpaceToken(this);
   if (token != BXSTokenizer::cLeftBrace)
   {
      srcErrMsg("Error 0374: expected '{' and found '%s'.", getTokenizer()->getCurrentTokenText());
      return(false);
   }

   //Variables.
   token=getTokenizer()->getDarkSpaceToken(this);
   while (token != BXSTokenizer::cRightBrace)
   {
      //Get the value of the variable type symbol.  It should be an index between 0 and 
      //the number of variable types.
      BYTE parmType=parseVariableTypeName(token, true);
      if (parmType == BXSVariableEntry::cInvalidVariable)
      {
         srcErrMsg("Error 0375: '%s' is not a valid variable type.", getTokenizer()->getCurrentTokenText());
         return(false);
      }

      //Parse the member and its initialization.  We have to push the type name
      //back on the tokenizer, first, though.
      if (parseVarDecl(false, true, true, true) == false)
      {
         srcErrMsg("Error 0376: could not parse member variable declaration.");
         return(false);
      }

      //Get the next token.
      token=getTokenizer()->getDarkSpaceToken(this);
   }

   //';'.
   token=getTokenizer()->getDarkSpaceToken(this);
   if (token != BXSTokenizer::cSemiColon) 
   {
      srcErrMsg("Error 0378: expected ';' and found '%s'.", getTokenizer()->getCurrentTokenText());
      return(false);
   }

   //Turn off current class.
   mCurrentClassID=-1;
   return(true);
}

//==============================================================================
// BXSCompiler::parseClassDeclaration
//==============================================================================
bool BXSCompiler::parseClassDeclaration(bool incrementTokenFirst, bool stripSemiColon,
   bool parameter)
{
   #ifdef DEBUGTRACEPARSE
   mMessenger->infoMsg("parseClassDeclaration.");
   #endif

   //Get the token we need to look at.
   long token;
   if (incrementTokenFirst == true)
      token=getTokenizer()->getDarkSpaceToken(this);
   else
      token=getTokenizer()->getCurrentToken();

   //Parse any qualifiers we might have.
   bool exportVariable=false;
   bool externVariable=false;
   bool constVariable=false;
   bool staticVariable=false;
   while ((token == BXSTokenizer::cExport) ||
      (token == BXSTokenizer::cExtern) ||
      (token == BXSTokenizer::cStatic) ||
      (token == BXSTokenizer::cConst))
   {
      switch (token)
      {
         case BXSTokenizer::cExport:
            //Check the simple stuff.
            if ((parameter == true) || (mCurrentFunctionID >= 0))
            {
               mMessenger->errorMsg("Error 0385: Cannot 'export' a parameter or function variable.");
               return(false);
            }
            //If const or static has been set already, badness.
            if ((constVariable == true) || (staticVariable == true))
            {
               mMessenger->errorMsg("Error 0386: Cannot 'export' a 'const' or 'static' variable.");
               return(false);
            }
               
            exportVariable=true;
            externVariable=true;
            break;

         case BXSTokenizer::cExtern:
            //Check the simple stuff.
            if ((parameter == true) || (mCurrentFunctionID >= 0))
            {
               mMessenger->errorMsg("Error 0387: Cannot 'extern' a parameter or function variable.");
               return(false);
            }
            //If static has been set already, badness.
            if (staticVariable == true)
            {
               mMessenger->errorMsg("Error 0388: Cannot 'extern' a 'static' variable.");
               return(false);
            }

            externVariable=true;
            break;

         case BXSTokenizer::cStatic:
            //Check the simple stuff.
            if ((parameter == true) || (mCurrentFunctionID < 0))
            {
               mMessenger->errorMsg("Error 0389: Cannot 'static' a parameter or non-function variable.");
               return(false);
            }
            //Cannot be combined with export or const.
            if ((exportVariable == true) || (externVariable == true) || (constVariable == true))
            {
               mMessenger->errorMsg("Error 0390: Cannot 'static' an 'export', 'extern', or 'const' variable.");
               return(false);
            }
            staticVariable=true;
            break;

         case BXSTokenizer::cConst:
            //Cannot be combined with export or static.
            if ((exportVariable == true) || (staticVariable == true))
            {
               mMessenger->errorMsg("Error 0391: Cannot 'const' an 'export' or 'static' variable.");
               return(false);
            }

            constVariable=true;
            break;
      }

      //Get the next token.
      token=getTokenizer()->getDarkSpaceToken(this);
   }

   //We expect a cName token here.  We also expect that to be a defined class at this point.
   if (token != BXSTokenizer::cName)
   {
      srcErrMsg("Error 0382: expected a user class name and found '%s'.", getTokenizer()->getCurrentTokenText());
      return(false);
   }
   //Get the UC type ID.
   long ucID=mData->getUserClassID(getTokenizer()->getCurrentTokenText());
   if (ucID < 0)
   {
      srcErrMsg("Error 0383: '%s' is not a valid user class name.", getTokenizer()->getCurrentTokenText());
      return(false);
   }
   //Get the type name.
   /*BYTE variableType=parseVariableTypeName(token, false);
   if (variableType == BXSVariableEntry::cInvalidVariable)
   {
      srcErrMsg("Error 0013: '%s' is not a valid variable type.");
      return(false);
   }*/

   //Get the variable name.  It's an error if it's already defined.
   token=getTokenizer()->getDarkSpaceToken(this);
   const BXSSymbolEntry *se=getSymbolEntry(getTokenizer()->getCurrentTokenText(), true);
   if (se != NULL)
   {
      srcErrMsg("Error 0384: '%s' is already defined.", getTokenizer()->getCurrentTokenText());
      return(false);
   }
   //Create it.
   char variableName[(BXSTokenizer::cMaxTokenSize+1)*2];
   //If we have an extern variable, we don't put any scope qualifier on it.
   if (externVariable == true)
      bsnprintf(variableName, sizeof(variableName), "EXTERN$%s", getTokenizer()->getCurrentTokenText());
   else
      bsnprintf(variableName, sizeof(variableName), "%s%s", mCurrentScope, getTokenizer()->getCurrentTokenText());

   //Allocate the new variable entry.
   BXSVariableEntry *newVE=new BXSVariableEntry();
   if (newVE == NULL)
   {
      srcErrMsg("Error 0392: failed to create variable entry.");
      return(false);
   }
   //Set the qualifiers.
   newVE->setExport(exportVariable);
   newVE->setExtern(externVariable);
   newVE->setConst(constVariable);
   newVE->setStatic(staticVariable);

   //Init the new VE with the UCE.
   BXSUserClassEntry *uce=mData->getUserClassEntry(ucID);
   if (uce == NULL)
   {
      srcErrMsg("Error 0393: failed to find user class entry definition.");
      return(false);
   }
   if (newVE->initUserClass(uce) == false)
   {
      srcErrMsg("Error 0394: user class init failed.");
      return(false);
   }

   //Default value.  If we're doing a parameter, we have a const variable, we have a
   //static variable, or we are forcing a constant initial value (i.e. not an expression),
   //we have to have a constant initial value.  Else, we "default" the variable to the
   //logical 0 and are allowed to do a variable expression assignment.
   /*if ((parameter == true) || (constVariable == true) ||
      (forceConstantInitialValue == true) || (staticVariable == true))
   {
      //'='
      token=getTokenizer()->getDarkSpaceToken(this);
      if (token != BXSTokenizer::cAssign) 
      {
         srcErrMsg("Error 0016: expected '=' and found '%s' (you must initialize all variables to a literal or const value).");
         return(false);
      }

      switch (variableType)
      {
         case BXSVariableEntry::cIntegerVariable:
         {
            long defaultValue;
            if (parseIntegerConstant(&defaultValue) == false)
            {
               srcErrMsg("Error 0017: parseIntegerConstant failed.");
               return(false);
            }
            if (newVE->setAll(-1, variableType, &defaultValue) == false)
            {
               srcErrMsg("Error 0018: setAll failed.");
               return(false);
            }
            break;
         }
         case BXSVariableEntry::cFloatVariable:
         {
            float defaultValue;
            if (parseFloatConstant(&defaultValue) == false)
            {
               srcErrMsg("Error 0019: parseFloatConstant failed.");
               return(false);
            }
            if (newVE->setAll(-1, variableType, &defaultValue) == false)
            {
               srcErrMsg("Error 0020: setAll failed.");
               return(false);
            }
            break;
         }
         //Bool.
         case BXSVariableEntry::cBoolVariable:
         {
            bool defaultValue;
            if (parseBoolConstant(&defaultValue) == false)
            {
               srcErrMsg("Error 0021: parseBoolConstant failed.");
               return(false);
            }
            if (newVE->setAll(-1, variableType, &defaultValue) == false)
            {
               srcErrMsg("Error 0022: setAll failed.");
               return(false);
            }
            break;
         }
         //String.
         case BXSVariableEntry::cStringVariable:
            token=getTokenizer()->getDarkSpaceToken(this);
            if (token != BXSTokenizer::cString)
            {
               srcErrMsg("Error 0023: expected a string constant and found '%s'.");
               return(false);
            }
            if (newVE->setAll(-1, variableType, getTokenizer()->getCurrentTokenText()) == false)
            {
               srcErrMsg("Error 0024: setAll failed.");
               return(false);
            }
            break;
         //Vector.
         case BXSVariableEntry::cVectorVariable:
         {
            //We have to kludgily get the token first.
            token=getTokenizer()->getDarkSpaceToken(this);

            //This vector has to be a constant since this is a var declaration.
            BVector defaultValue;
            if (parseVectorConstant(&defaultValue) == false)
            {
               srcErrMsg("Error 0026: parseVectorConstant failed.");
               return(false);
            }
            if (newVE->setAll(-1, variableType, &defaultValue) == false)
            {
               srcErrMsg("Error 0027: setAll failed.");
               return(false);
            }
            break;
         }

         default:
            srcErrMsg("Error 0028: don't understand the default type for this variable.");
            return(false);
      }

      //';'.
      if (stripSemiColon == true)
      {
         token=getTokenizer()->getDarkSpaceToken(this);
         if (token != BXSTokenizer::cSemiColon) 
         {
            srcErrMsg("Error 0029: expected ';' and found '%s'.");
            return(false);
         }
      }
   }
   else
   {
      //Default value.
      switch (variableType)
      {
         case BXSVariableEntry::cIntegerVariable:
         {
            long defaultValue=0;
            if (newVE->setAll(-1, variableType, &defaultValue) == false)
            {
               srcErrMsg("Error 0284: setAll failed.");
               return(false);
            }
            break;
         }
         case BXSVariableEntry::cFloatVariable:
         {
            float defaultValue=0.0f;
            if (newVE->setAll(-1, variableType, &defaultValue) == false)
            {
               srcErrMsg("Error 0285: setAll failed.");
               return(false);
            }
            break;
         }
         //Bool.
         case BXSVariableEntry::cBoolVariable:
         {
            bool defaultValue=false;
            if (newVE->setAll(-1, variableType, &defaultValue) == false)
            {
               srcErrMsg("Error 0286: setAll failed.");
               return(false);
            }
            break;
         }
         //String.
         case BXSVariableEntry::cStringVariable:
            if (newVE->setAll(-1, variableType, "DSV") == false)
            {
               srcErrMsg("Error 0287: setAll failed.");
               return(false);
            }
            break;
         //Vector.
         case BXSVariableEntry::cVectorVariable:
         {
            BVector defaultValue(0.0f);
            if (newVE->setAll(-1, variableType, &defaultValue) == false)
            {
               srcErrMsg("Error 0288: setAll failed.");
               return(false);
            }
            break;
         }
         default:
            srcErrMsg("Error 0289: don't understand the default type for this variable.");
            return(false);
      }
   }*/

   //Add this to the global variable array if we have a global variable, to the 
   //current function if we have one.
   if (mCurrentFunctionID >= 0)
   {
      BXSFunctionEntry *fe=mData->getFunctionEntry(mCurrentFunctionID);
      if (fe == NULL)
         return(false);
      long newVariableID=fe->getNumberVariables();

      //Add it to the symbol table.  The symbol type is cStackVariable.  The ID of variable is
      //the value.  addSymbol returns the unique ID for this variable, so we stuff that
      //into the ID slot in the variable table so that we can reference it later.
      long symbolID=mSource->getSymbols().addSymbol(variableName, BXSBinary::cStackVariable, newVariableID);
      if (symbolID < 0)
      {
         srcErrMsg("Error 399: unable to add an entry to the file symbol table for this stack variable.");
         return(false);
      }

      //Set the symbol ID now that we have it.
      newVE->setSymbolID(symbolID);
      //Add it to the function.
      if (parameter == true)
         fe->addParameter(newVE);
      else
         fe->addVariable(newVE);
   }
   else
   {
      long newVariableID=mData->addVariableEntry(newVE);

      //Add it to the symbol table.  The symbol type is cVariable.  The ID of variable is
      //the value (see NOTE).  addSymbol returns the unique ID for this variable, so we
      //stuff that into the ID slot in the variable table so that we can reference it later.
      //NOTE: In order to unify the opcodes, we negate and offset the global variable IDs
      //in everything.  We negate and subtract one (to offset the 0th variable to -1).
      long symbolID=mSource->getSymbols().addSymbol(variableName, BXSBinary::cVariable, -newVariableID-1);
      if (symbolID < 0)
      {
         srcErrMsg("Error 400: unable to add an entry to the file symbol table for this global variable.");
         return(false);
      }

      //Set the symbol ID now that we have it.
      newVE->setSymbolID(symbolID);
   }

   //Now, parse the assignment (if we're not doing a constant assignment, a const variable,
   //or a static variable).
   /*if ((parameter == false) && (constVariable == false) &&
      (forceConstantInitialValue == false) && (staticVariable == false))
   {
      if (parseVariableAssignment(token, getTokenizer()->getCurrentTokenText(), stripSemiColon, true) == false)
      {
         srcErrMsg("Error 0290: parseVariableAssignment failed.");
         return(false);
      }
   }*/

   //Strip semi-colon.
   //if ((stripSemiColon == true) && (token == BXSTokenizer::cSemiColon))
   if (stripSemiColon == true)
      token=getTokenizer()->getDarkSpaceToken(this);

   return(true);
}

//==============================================================================
// BXSCompiler::parseVariableAssignment
//==============================================================================
bool BXSCompiler::parseVariableAssignment(long token, char *tokenText, bool stripSemiColon,
   bool forceAssignment)
{
   #ifdef DEBUGTRACEPARSE
   mMessenger->infoMsg("ParseVariableAssignment.");
   #endif

   //If the token isn't a name, forget it.
   if (token != BXSTokenizer::cName)
   {
      srcErrMsg("Error 0068: invalid name token in LVALUE in variable assignment.");
      return(false);
   }

   //LVALUE.  Get the symbol entry for this token.
   const BXSSymbolEntry *se=getSymbolEntry(tokenText, true);
   if (se == NULL)
   {
      srcErrMsg("Error 0069: bad LVALUE in variable assignment.");
      return(false);
   }
   //If it's not a variable, fail it.
   if ((se->mType != BXSBinary::cVariable) && (se->mType != BXSBinary::cStackVariable))
   {
      srcErrMsg("Error 0070: '%s' is not a valid variable for assignment into.", getTokenizer()->getCurrentTokenText());
      return(false);
   }
   long variableType=-1;
   if (se->mType == BXSBinary::cVariable)
      variableType=mData->getVariableEntry(-se->mValue-1)->getType();
   else
   {
      BXSFunctionEntry *fe=mData->getFunctionEntry(mCurrentFunctionID);
      BASSERT(fe != NULL);
      variableType=fe->getVariable(se->mValue)->getType();
   }

   //If this variable is const, fail.
   if (se->mValue >= 0)
   {
      if (mData->getFunctionEntry(mCurrentFunctionID)->getVariable(se->mValue)->getConst() == true)
      {
         srcErrMsg("Error 0071: '%s' is a const variable.", getTokenizer()->getCurrentTokenText());
         return(false);
      }
   }
   else
   {
      if (mData->getVariableEntry(-se->mValue-1)->getConst() == true)
      {
         srcErrMsg("Error 0072: '%s' is a const variable.", getTokenizer()->getCurrentTokenText());
         return(false);
      }
   }

   //'=', '++', '--'.
   long assignToken=getTokenizer()->getDarkSpaceToken(this);
   if (assignToken != BXSTokenizer::cAssign)
   {
      //If we're supposed to force assignment, this is an error.
      if (forceAssignment == true)
      {
         srcErrMsg("Error 0291: expected '=' and found '%s' (you must initialize all variables to a literal or const value).", getTokenizer()->getCurrentTokenText());
         return(false);
      }

      if ((assignToken != BXSTokenizer::cPlusPlus) && (assignToken != BXSTokenizer::cMinusMinus))
      {
         srcErrMsg("Error 0073: expected '=', '++', or '--' and found '%s'.", getTokenizer()->getCurrentTokenText());
         return(false);
      }
      if ((variableType != BXSVariableEntry::cIntegerVariable) && (variableType != BXSVariableEntry::cFloatVariable))
      {
         srcErrMsg("Error 0074: '%s' operator is only valid for integers and floats.", getTokenizer()->getCurrentTokenText());
         return(false);
      }

      //Increment value.
      long incrementValue=1;
      if (assignToken == BXSTokenizer::cMinusMinus)
         incrementValue=-1;

      if (createIncrementVariableCode(se->mValue, incrementValue) == false)
      {
         srcErrMsg("Error 0075: createIncrementVariableCode failed.");
         return(false);
      }
   }
   else
   {
      //Add the push address opcode.
      if (mQuads.add(BXSQuad(BXSQuadOpcode::cPUSHADD, se->mValue, 0)) == -1)
      {
         srcErrMsg("Error 0076: PUSHADD quad addition failed.");
         return(false);
      }

      //Parse the expression.
      if (parseExpression(true, false, true, false, false, false, variableType) == false)
      {
         srcErrMsg("Error 0077: parseExpression failed.");
         return(false);
      }

      //Add the ASS.
      if (mQuads.add(BXSQuad(BXSQuadOpcode::cASS)) == -1)
      {
         srcErrMsg("Error 0078: ASS quad addition failed.");
         return(false);
      }

      //Pop the value.
      if (mQuads.add(BXSQuad(BXSQuadOpcode::cPOP, 1)) == -1)
      {
         srcErrMsg("Error 0079: POP quad addition failed.");
         return(false);
      }

      //Pop the address of the variable that we assigned into.
      if (mQuads.add(BXSQuad(BXSQuadOpcode::cPOPADD, 1)) == -1)
      {
         srcErrMsg("Error 0080: POPADD quad addition failed.");
         return(false);
      }
   }

   //';'.
   if (stripSemiColon == true)
   {
      token=getTokenizer()->getDarkSpaceToken(this);
      if (token != BXSTokenizer::cSemiColon)
      {
         srcErrMsg("Error 0081: expected ';' and found '%s'.", getTokenizer()->getCurrentTokenText());
         return(false);
      }
   }

   return(true);
}

//==============================================================================
// BXSCompiler::parseForLoop
//==============================================================================
bool BXSCompiler::parseForLoop(void)
{
   #ifdef DEBUGTRACEPARSE
   mMessenger->infoMsg("ParseForLoop.");
   #endif

   //Initialize.
   long token=getTokenizer()->getDarkSpaceToken(this);

   //'('
   if (token != BXSTokenizer::cLeftParen)
   {
      srcErrMsg("Error 0082: expected '(' and found '%s'.", getTokenizer()->getCurrentTokenText());
      return(false);
   }

   //Get/Create the iterator variable.
   token=getTokenizer()->getDarkSpaceToken(this);
   if (token != BXSTokenizer::cName)
   {
      srcErrMsg("Error 0083: invalid variable in for loop.");
      return(false);
   }
   //Get the symbol entry for this token.  If it doesn't exist, create a variable.
   long indexVariableID=-1;
   BXSVariableEntry *indexVariableEntry=NULL;
   const BXSSymbolEntry *se=getSymbolEntry(getTokenizer()->getCurrentTokenText(), true);
   if (se == NULL)
   {
      //Create it.
      char variableName[(BXSTokenizer::cMaxTokenSize+1)*2];
      bsnprintf(variableName, sizeof(variableName), "%s%s", mCurrentScope, getTokenizer()->getCurrentTokenText());

      //Allocate the new variable entry.
      BXSVariableEntry *newVE=new BXSVariableEntry();
      if (newVE == NULL)
      {
         srcErrMsg("Error 0084: could not create variable entry.");
         return(false);
      }
      //Default it to 0.
      long defaultValue=0;
      if (newVE->setAll(-1, BXSVariableEntry::cIntegerVariable, &defaultValue) == false)
      {
         srcErrMsg("Error 0085: setAll failed.");
         return(false);
      }
      //Get the id.
      long newVariableID=mData->getFunctionEntry(mCurrentFunctionID)->getNumberVariables();
      //Add it to the symbol table.  The symbol type is cStackVariable.  The ID of variable is
      //the value.  addSymbol returns the unique ID for this variable, so we stuff that
      //into the ID slot in the variable table so that we can reference it later.
      long symbolID=mSource->getSymbols().addSymbol(variableName, BXSBinary::cStackVariable, newVariableID);
      if (symbolID < 0)
      {
         srcErrMsg("Error 0086: unable to add an entry to the file symbol table for this stack variable.");
         return(false);
      }

      //Set the symbol ID now that we have it.
      newVE->setSymbolID(symbolID);
      //Add the variable to the function.
      mData->getFunctionEntry(mCurrentFunctionID)->addVariable(newVE);

      indexVariableID=newVariableID;
      indexVariableEntry=newVE;
   }
   else
   {
      //If it's not a variable, fail it.
      if ((se->mType != BXSBinary::cVariable) && (se->mType != BXSBinary::cStackVariable))
      {
         srcErrMsg("Error 0087: '%s' is not a valid variable.", getTokenizer()->getCurrentTokenText());
         return(false);
      }
      indexVariableID=se->mValue;
      if (se->mValue >= 0)
         indexVariableEntry=mData->getFunctionEntry(mCurrentFunctionID)->getVariable(se->mValue);
      else
         indexVariableEntry=mData->getVariableEntry(-se->mValue-1);
   }

   //Parse the variable assignment now.
   if (parseVariableAssignment(token, getTokenizer()->getCurrentTokenText(), true, true) == false)
   {
      srcErrMsg("Error 0088: parseVariableAssignment failed.");
      return(false);
   }

   //Relation operator.
   long compareOpcode=-1;
   long incrementValue=1;
   token=getTokenizer()->getDarkSpaceToken(this);
   switch (token)
   {
      case BXSTokenizer::cLT:
         compareOpcode=BXSQuadOpcode::cLT;
         incrementValue=1;
         break;
      case BXSTokenizer::cLE:
         compareOpcode=BXSQuadOpcode::cLE;
         incrementValue=1;
         break;
      case BXSTokenizer::cGE:
         compareOpcode=BXSQuadOpcode::cGE;
         incrementValue=-1;
         break;
      case BXSTokenizer::cGT:
         compareOpcode=BXSQuadOpcode::cGT;
         incrementValue=-1;
         break;
      default:
         srcErrMsg("Error 0094: expected '<', '<=', '>', or '>=' and found '%s'.", getTokenizer()->getCurrentTokenText());
         return(false);
   }

   //Parse the expression for the final 'value'.
   BXSParseTreeNode *finalRoot=parseExpression2(true, true, false, false, false, false, BXSVariableEntry::cIntegerVariable);
   if (finalRoot == NULL)
   {
      srcErrMsg("Error 0218: parseExpression2 failed.");
      return(false);
   }

   //')'
   token=getTokenizer()->getDarkSpaceToken(this);
   if (token != BXSTokenizer::cRightParen)
   {
      srcErrMsg("Error 0097: expected ')' and found '%s'.", getTokenizer()->getCurrentTokenText());
      delete finalRoot;
      return(false);
   }

   //Create the necessary labels.
   long continueLabelID=createLabel(NULL, true);
   long loopLabelID=createLabel(NULL, true);
   long endLabelID=createLabel(NULL, true);
   //Add the continue label as a continue label.
   if (mContinueLabelIDs.add(continueLabelID) == -1)
   {
      srcErrMsg("Error 0329: error adding for-loop continue label ID.");
      return(false);
   }
   //Add the end label as the break label.
   if (mBreakLabelIDs.add(endLabelID) == -1)
   {
      srcErrMsg("Error 0242: error adding for-loop break label ID.");
      return(false);
   }

   //Add the LOOP label.
   if (mQuads.add(BXSQuad(BXSQuadOpcode::cLABEL, loopLabelID)) == -1)
   {
      srcErrMsg("Error 0098: could not add LOOP LABEL quad for FOR loop.");
      delete finalRoot;
      return(false);
   }

   //Add the compare.
   if (mQuads.add(BXSQuad(BXSQuadOpcode::cPUSH, indexVariableID, 0)) == -1)
   {
      srcErrMsg("Error 0099: PUSH quad addition failed.");
      delete finalRoot;
      return(false);
   }
   //Spit out the final expression parse tree.
   if (finalRoot->outputQuads(mQuads, mMessenger) == false)
   {
      srcErrMsg("Error 0219: final quad output failed.");
      delete finalRoot;
      return(false);
   }
   delete finalRoot;

   //Add the actual compare opcode.
   if (mQuads.add(BXSQuad(compareOpcode)) == -1)
   {
      srcErrMsg("Error 0102: compare quad addition failed.");
      return(false);
   }

   //Add the JUMPZ quad.
   if (mQuads.add(BXSQuad(BXSQuadOpcode::cJUMPZ, 1, endLabelID)) == -1)
   {
      srcErrMsg("Error 0103: JUMPZ quad addition failed.");
      return(false);
   }

   //While we're parsing the code block, we do set the index variable to const
   //so that it cannot be assigned into during the block code.
   indexVariableEntry->setConst(true);

   //Parse the code block.
   if (parseCode(true, false, NULL) == false)
   {
      srcErrMsg("Error 0104: could not parse the code for FOR loop.");
      return(false);
   }

   //Add the CONTINUE label.
   if (mQuads.add(BXSQuad(BXSQuadOpcode::cLABEL, continueLabelID)) == -1)
   {
      srcErrMsg("Error 0330: could not add CONTINUE label quad for FOR loop.");
      return(false);
   }

   //Set the index variable back to non-const.
   indexVariableEntry->setConst(false);

   //Inc/decrement the index variable.
   if (createIncrementVariableCode(indexVariableID, incrementValue) == false)
   {
      srcErrMsg("Error 0105: createIncrementVariableCode failed.");
      return(false);
   }

   //Add the jump back to the loop label.
   if (mQuads.add(BXSQuad(BXSQuadOpcode::cJUMP, 1, loopLabelID)) == -1)
   {
      srcErrMsg("Error 0106: could not add JUMP quad for loop label for FOR loop.");
      return(false);
   }

   //Add the END label.
   if (mQuads.add(BXSQuad(BXSQuadOpcode::cLABEL, endLabelID)) == -1)
   {
      srcErrMsg("Error 0107: could not add END label quad for FOR loop.");
      return(false);
   }

   //Remove the continue label ID that we added.
   mContinueLabelIDs.remove(continueLabelID);
   //Remove the break label ID that we added.
   mBreakLabelIDs.remove(endLabelID);

   return(true);
}

//==============================================================================
// BXSCompiler::parseDBG
//==============================================================================
bool BXSCompiler::parseDBG(long token, char *tokenText)
{
   token;
   #ifdef DEBUGTRACEPARSE
   mMessenger->infoMsg("ParseDBG.");
   #endif

   //LVALUE.  Get the symbol entry for this token.
   const BXSSymbolEntry *se=getSymbolEntry(tokenText, true);
   if (se == NULL)
   {
      srcErrMsg("Error 0108: '%s' is an invalid token.", getTokenizer()->getCurrentTokenText());
      return(false);
   }
   //If it's not a variable, fail it.
   if ((se->mType != BXSBinary::cVariable) && (se->mType != BXSBinary::cStackVariable))
   {
      srcErrMsg("Error 0109: '%s' is not a valid variable for DBG.", getTokenizer()->getCurrentTokenText());
      return(false);
   }

   //Quick thing that allows printing of debug values w/o calling syscalls.

   //PUSHADD the variable's index.
   if (mQuads.add(BXSQuad(BXSQuadOpcode::cPUSHADD, se->mValue, 0)) == -1)
   {
      srcErrMsg("Error 0110: PUSHADD quad addition failed.");
      return(false);
   }
   //DBG opcode.
   if (mQuads.add(BXSQuad(BXSQuadOpcode::cDBG)) == -1)
   {
      srcErrMsg("Error 0111: DBG quad addition failed.");
      return(false);
   }
   //POPADD opcode.
   if (mQuads.add(BXSQuad(BXSQuadOpcode::cPOPADD, 1)) == -1)
   {
      srcErrMsg("Error 0112: POPADD quad addition failed.");
      return(false);
   }

   return(true);
}

//==============================================================================
// BXSCompiler::parseILL
//==============================================================================
bool BXSCompiler::parseILL(void)
{
   #ifdef DEBUGTRACEPARSE
   mMessenger->infoMsg("ParseILL.");
   #endif

   long loopLimit=-1;
   if (parseIntegerConstant(&loopLimit) == false)
   {
      srcErrMsg("Error 0294: parseIntegerConstant failed.");
      return(false);
   }
   if (loopLimit < -1)
   {
      srcErrMsg("Error 0295: %d is an invalid infinite loop limit.", loopLimit);
      return(false);
   }

   //ILL opcode.
   if (mQuads.add(BXSQuad(BXSQuadOpcode::cILL, loopLimit)) == -1)
   {
      srcErrMsg("Error 0296: ILL quad addition failed.");
      return(false);
   }

   return(true);
}

//==============================================================================
// BXSCompiler::parseIRL
//==============================================================================
bool BXSCompiler::parseIRL(void)
{
   #ifdef DEBUGTRACEPARSE
   mMessenger->infoMsg("ParseIRL.");
   #endif

   long limit=-1;
   if (parseIntegerConstant(&limit) == false)
   {
      srcErrMsg("Error 0297: parseIntegerConstant failed.");
      return(false);
   }
   if (limit < -1)
   {
      srcErrMsg("Error 0298: %d is an invalid infinite recursion limit.", limit);
      return(false);
   }

   //IRL opcode.
   if (mQuads.add(BXSQuad(BXSQuadOpcode::cIRL, limit)) == -1)
   {
      srcErrMsg("Error 0299: IRL quad addition failed.");
      return(false);
   }

   return(true);
}

//==============================================================================
// BXSCompiler::parseReturn
//==============================================================================
bool BXSCompiler::parseReturn(void)
{
   #ifdef DEBUGTRACEPARSE
   mMessenger->infoMsg("ParseReturn.");
   #endif

   //A return consists of the appropriate variable PUSH (if needed) and a STOP.

   //Get the return type for the function that we're currently in.  If we're not in
   //a function, this is an error.
   BXSFunctionEntry *fe=mData->getFunctionEntry(mCurrentFunctionID);
   if (fe == NULL)
   {
      srcErrMsg("Error 0113: '%s' is not valid outside of a function.", getTokenizer()->getCurrentTokenText());
      return(false);
   }
   long returnType=fe->getReturnType();

   //Decide what to do based on the token.
   long token=getTokenizer()->getDarkSpaceToken(this);

   //If we have a void return type, anything other than a ';' or '()' is invalid.
   if (returnType == BXSVariableEntry::cVoidVariable)
   {
      //If we have a semicolon, we're good.
      if (token == BXSTokenizer::cSemiColon)
      {
         if (mQuads.add(BXSQuad(BXSQuadOpcode::cRET)) == -1)
         {
            srcErrMsg("Error 0311: could not add RET quad for '%s' function.", mCurrentScope);
            return(false);
         }
         return(true);
      }
      //If we have a left paren, make sure that the next token is the right paren.
      if (token == BXSTokenizer::cLeftParen)
      {
         token=getTokenizer()->getDarkSpaceToken(this);
         if (token == BXSTokenizer::cRightParen)
         {
            if (mQuads.add(BXSQuad(BXSQuadOpcode::cRET)) == -1)
            {
               srcErrMsg("Error 0115: could not add RET quad for '%s' function.", mCurrentScope);
               return(false);
            }
         }
         else
         {
            srcErrMsg("Error 0313: expected ')' and found '%s'.", getTokenizer()->getCurrentTokenText());
            return(false);
         }

         //';'.
         token=getTokenizer()->getDarkSpaceToken(this);
         if (token != BXSTokenizer::cSemiColon)
         {
            srcErrMsg("Error 0312: expected ';' and found '%s'.", getTokenizer()->getCurrentTokenText());
            return(false);
         }
         return(true);
      }

      srcErrMsg("Error 0116: '%s' expected empty return parameter.", getTokenizer()->getCurrentTokenText());
      return(false);
   }

   //If we have a '(', decrement the token (so the standard parseExpression parsing will
   //have a complete expr to parse).
   if (token == BXSTokenizer::cLeftParen)
      getTokenizer()->decrementCurrentToken(this);

   //Parse the expression for the return value.
   if (parseExpression(true, false, true, false, false, false, returnType) == false)
   {
      srcErrMsg("Error 0117: parseExpression failed.");
      return(false);
   }

   //Add the RET quad.
   if (mQuads.add(BXSQuad(BXSQuadOpcode::cRET)) == -1)
   {
      srcErrMsg("Error 0118: could not add RET quad for '%s' function.", mCurrentScope);
      return(false);
   }

   //';'.
   token=getTokenizer()->getDarkSpaceToken(this);
   if (token != BXSTokenizer::cSemiColon)
   {
      srcErrMsg("Error 0119: expected ';' and found '%s'.", getTokenizer()->getCurrentTokenText());
      return(false);
   }

   return(true);
}

//==============================================================================
// BXSCompiler::parseInclude
//==============================================================================
bool BXSCompiler::parseInclude(bool incrementTokenFirst)
{
   #ifdef DEBUGTRACEPARSE
   mMessenger->infoMsg("ParseInclude.");
   #endif

   //Include.
   long token;
   if (incrementTokenFirst == true)
      token=getTokenizer()->getDarkSpaceToken(this);
   else
      token=getTokenizer()->getCurrentToken();
   if (token != BXSTokenizer::cInclude)
   {
      srcErrMsg("Error 0120: expected 'include' and found '%s'.", getTokenizer()->getCurrentTokenText());
      return(false);
   }

   //Filename.
   BSimString filename;
   token=getTokenizer()->getDarkSpaceToken(this);
   if (token != BXSTokenizer::cString)
   {
      srcErrMsg("Error 0121: expected filename and found '%s'.", getTokenizer()->getCurrentTokenText());
      return(false);
   }
   filename.set(getTokenizer()->getCurrentTokenText());
   //Qualified filename.
   BSimString qualifiedFilename;
   if (mIncludePath.length() > 0)
      qualifiedFilename.format(B("%s%s"), mIncludePath.getPtr(), filename.getPtr());
   else
      qualifiedFilename.set(filename.getPtr());

   //';'.
   token=getTokenizer()->getDarkSpaceToken(this);
   if (token != BXSTokenizer::cSemiColon)
   {
      srcErrMsg("Error 0122: expected ';' and found '%s'.", getTokenizer()->getCurrentTokenText());
      return(false);
   }

   //Open the file, create a buffer for the data, and then stuff the file data
   //into the buffer.
   BFile file;
   bool ok = file.openReadOnly(mBaseUWDirID, filename);
   if(!ok)
      ok = file.openReadOnly(mBaseDirID, filename);
   if (!ok)
   {
      srcErrMsg("Error 0123: failed to open file '%s'.", qualifiedFilename.getPtr());
      return(false);
   }
   //Get the size.
   unsigned long fileSize=0;
   if (file.getSize(fileSize) == false)
   {
      srcErrMsg("Error 0124: failed to get size of file '%s'.", qualifiedFilename.getPtr());
      return(false);
   }
   //Allocate a temporary buffer.
   char *buffer=new char[(long)fileSize+1];
   if (buffer == 0)
   {
      srcErrMsg("Error 0126: could not allocate buffer.");
      return(false);
   }

   //Read it in.
   if (file.read(buffer, fileSize) == false)
   {
      srcErrMsg("Error 0127: failed to read file data.");
      BDELETEARRAY(buffer);
      return(false);
   }
   buffer[fileSize]='\0';

   //Actually do the include.
   bool rVal=parseSource(qualifiedFilename, filename, buffer, (long)fileSize+1);

   //Cleanup.
   BDELETEARRAY(buffer);

   return(rVal);
}

//==============================================================================
// BXSCompiler::parseSwitch
//==============================================================================
bool BXSCompiler::parseSwitch(bool incrementTokenFirst)
{
   #ifdef DEBUGTRACEPARSE
   mMessenger->infoMsg("ParseSwitch.");
   #endif

   //Switch.
   long token;
   if (incrementTokenFirst == true)
      token=getTokenizer()->getDarkSpaceToken(this);
   else
      token=getTokenizer()->getCurrentToken();
   if (token != BXSTokenizer::cSwitch)
   {
      srcErrMsg("Error 0220: expected 'switch' and found '%s'.", getTokenizer()->getCurrentTokenText());
      return(false);
   }

   //'('.
   token=getTokenizer()->getDarkSpaceToken(this);
   if (token != BXSTokenizer::cLeftParen)
   {
      srcErrMsg("Error 0222: expected ';' and found '%s'.", getTokenizer()->getCurrentTokenText());
      return(false);
   }

   //Create the end label.
   long endLabelID=createLabel(NULL, true);
   //Add the end label as the break label.
   if (mBreakLabelIDs.add(endLabelID) == -1)
   {
      srcErrMsg("Error 0241: error adding switch break label ID.");
      return(false);
   }

   //Create a temporary variable for the switch value.
   long switchVIValue=0;
   long switchVI=createTemporaryVariable(BXSVariableEntry::cIntegerVariable, &switchVIValue);
   if (switchVI < 0)
   {
      srcErrMsg("Error 0223: createTempVariable failed.");
      return(false);
   }
   //Push the switch value variable address.
   if (mQuads.add(BXSQuad(BXSQuadOpcode::cPUSHADD, switchVI, 0)) == -1)
   {
      srcErrMsg("Error 0224: PUSHADD quad addition failed.");
      return(false);
   }

   //Parse the expression (which will leave the value on TOS).
   if (parseExpression(true, true, false, false, false, true, BXSVariableEntry::cIntegerVariable) == false)
   {
      srcErrMsg("Error 0225: parseExpression failed.");
      return(false);
   }

   //Do the assignment.
   if (mQuads.add(BXSQuad(BXSQuadOpcode::cASS)) == -1)
   {
      srcErrMsg("Error 0226: ASS quad addition failed.");
      return(false);
   }
   //Pop the switch value.
   if (mQuads.add(BXSQuad(BXSQuadOpcode::cPOP, 1)) == -1)
   {
      srcErrMsg("Error 0227: POP quad addition failed.");
      return(false);
   }
   //Pop the address of the variable that we assigned into.
   if (mQuads.add(BXSQuad(BXSQuadOpcode::cPOPADD, 1)) == -1)
   {
      srcErrMsg("Error 0228: POPADD quad addition failed.");
      return(false);
   }

   //'{'.
   token=getTokenizer()->getDarkSpaceToken(this);
   if (token != BXSTokenizer::cLeftBrace)
   {
      srcErrMsg("Error 0229: expected '{' and found '%s'.", getTokenizer()->getCurrentTokenText());
      return(false);
   }

   //Case loop.
   token=getTokenizer()->getDarkSpaceToken(this);
   while (token != BXSTokenizer::cRightBrace)
   {
      //Case.
      if ((token != BXSTokenizer::cCase) && (token != BXSTokenizer::cDefault))
      {
         srcErrMsg("Error 0230: expected 'case' or 'default' and found '%s'.", getTokenizer()->getCurrentTokenText());
         return(false);
      }

      //Process a case.
      if (token == BXSTokenizer::cCase)
      {
         //Create this case label.
         long caseLabelID=createLabel(NULL, true);

         //Expr.
         if (parseExpression(true, false, false, false, true, true, BXSVariableEntry::cIntegerVariable) == false)
         {
            srcErrMsg("Error 0231: parseExpression failed.");
            return(false);
         }

         //Push the switch value on.
         if (mQuads.add(BXSQuad(BXSQuadOpcode::cPUSH, switchVI, 0)) == -1)
         {
            srcErrMsg("Error 0232: PUSH quad addition failed.");
            return(false);
         }

         //Add the compare opcode.
         if (mQuads.add(BXSQuad(BXSQuadOpcode::cEQ)) == -1)
         {
            srcErrMsg("Error 0233: compare quad addition failed.");
            return(false);
         }
         //Add the JUMPZ quad.
         if (mQuads.add(BXSQuad(BXSQuadOpcode::cJUMPZ, 1, caseLabelID)) == -1)
         {
            srcErrMsg("Error 0234: JUMPZ quad addition failed.");
            return(false);
         }

         //Actual code.
         if (parseCode(true, false, NULL) == false)
         {
            srcErrMsg("Error 0235: parseCode failed.");
            return(false);
         }

         //Jump to END.
         if (mQuads.add(BXSQuad(BXSQuadOpcode::cJUMP, 0, endLabelID)) == -1)
         {
            srcErrMsg("Error 0236: JUMP quad addition failed.");
            return(false);
         }

         //Add the case label.
         if (mQuads.add(BXSQuad(BXSQuadOpcode::cLABEL, caseLabelID)) == -1)
         {
            srcErrMsg("Error 0237: LABEL quad addition failed.");
            return(false);
         }
      }
      else
      {
         //':'.
         token=getTokenizer()->getDarkSpaceToken(this);
         if (token != BXSTokenizer::cColon)
         {
            srcErrMsg("Error 0255: expected ':' and found '%s'.", getTokenizer()->getCurrentTokenText());
            return(false);
         }

         //Actual code.
         if (parseCode(true, false, NULL) == false)
         {
            srcErrMsg("Error 0256: parseCode failed.");
            return(false);
         }

         //Jump to END.
         if (mQuads.add(BXSQuad(BXSQuadOpcode::cJUMP, 0, endLabelID)) == -1)
         {
            srcErrMsg("Error 0257: JUMP quad addition failed.");
            return(false);
         }
      }

      token=getTokenizer()->getDarkSpaceToken(this);
   }

   //Add the end label.
   if (mQuads.add(BXSQuad(BXSQuadOpcode::cLABEL, endLabelID)) == -1)
   {
      srcErrMsg("Error 0238: LABEL quad addition failed.");
      return(false);
   }
   //Remove the break label ID that we added.
   mBreakLabelIDs.remove(endLabelID);

   return(true);
}

//==============================================================================
// BXSCompiler::parseWhile
//==============================================================================
bool BXSCompiler::parseWhile(bool incrementTokenFirst)
{
   #ifdef DEBUGTRACEPARSE
   mMessenger->infoMsg("ParseWhile.");
   #endif

   //while.
   long token;
   if (incrementTokenFirst == true)
      token=getTokenizer()->getDarkSpaceToken(this);
   else
      token=getTokenizer()->getCurrentToken();
   if (token != BXSTokenizer::cWhile)
   {
      srcErrMsg("Error 0243: expected 'while' and found '%s'.", getTokenizer()->getCurrentTokenText());
      return(false);
   }

   //Create the necessary labels.
   long loopLabelID=createLabel(NULL, true);
   long endLabelID=createLabel(NULL, true);
   //Add the loop label as a continue label.
   if (mContinueLabelIDs.add(loopLabelID) == -1)
   {
      srcErrMsg("Error 0331: error adding while-loop continue label ID.");
      return(false);
   }
   //Add the end label as the break label.
   if (mBreakLabelIDs.add(endLabelID) == -1)
   {
      srcErrMsg("Error 0244: error adding while-loop break label ID.");
      return(false);
   }

   //Add the LOOP label.
   if (mQuads.add(BXSQuad(BXSQuadOpcode::cLABEL, loopLabelID)) == -1)
   {
      srcErrMsg("Error 0245: LABEL quad addition failed.");
      return(false);
   }

   //'('
   token=getTokenizer()->getDarkSpaceToken(this);
   if (token != BXSTokenizer::cLeftParen)
   {
      srcErrMsg("Error 0246: expected '(' and found '%s'.", getTokenizer()->getCurrentTokenText());
      return(false);
   }

   //Parse the conditional expression.
   if (parseExpression(true, true, false, false, false, true, BXSVariableEntry::cBoolVariable) == false)
   {
      srcErrMsg("Error 0247: parseExpression failed.");
      return(false);
   }

   //Add the JUMPZ quad.
   if (mQuads.add(BXSQuad(BXSQuadOpcode::cJUMPZ, 1, endLabelID)) == -1)
   {
      srcErrMsg("Error 0248: JUMPZ quad addition failed.");
      return(false);
   }

   //Parse the loop block.
   if (parseCode(true, false, NULL) == false)
   {
      srcErrMsg("Error 0249: parseCode failed.");
      return(false);
   }

   //Add the jump back to the loop label.
   if (mQuads.add(BXSQuad(BXSQuadOpcode::cJUMP, 1, loopLabelID)) == -1)
   {
      srcErrMsg("Error 0250: JUMP quad addition failed.");
      return(false);
   }

   //Add the end label.
   if (mQuads.add(BXSQuad(BXSQuadOpcode::cLABEL, endLabelID)) == -1)
   {
      srcErrMsg("Error 0251: LABEL quad addition failed.");
      return(false);
   }
   //Remove the continue label ID that we added.
   mContinueLabelIDs.remove(loopLabelID);
   //Remove the break label ID that we added.
   mBreakLabelIDs.remove(endLabelID);

   return(true);
}

//==============================================================================
// BXSCompiler::parseBreak
//==============================================================================
bool BXSCompiler::parseBreak(bool incrementTokenFirst)
{
   #ifdef DEBUGTRACEPARSE
   mMessenger->infoMsg("ParseBreak.");
   #endif

   //break.
   long token;
   if (incrementTokenFirst == true)
      token=getTokenizer()->getDarkSpaceToken(this);
   else
      token=getTokenizer()->getCurrentToken();
   if (token != BXSTokenizer::cBreak)
   {
      srcErrMsg("Error 0252: expected 'break' and found '%s'.", getTokenizer()->getCurrentTokenText());
      return(false);
   }

   //If we don't have any break labels in the stack, this is an error.
   if (mBreakLabelIDs.getNumber() <= 0)
   {
      srcErrMsg("Error 0253: invalid 'break' location.");
      return(false);
   }

   //Add the jump to the top label in the break label ID stack.
   if (mQuads.add(BXSQuad(BXSQuadOpcode::cJUMP, 0, mBreakLabelIDs[mBreakLabelIDs.getNumber()-1])) == -1)
   {
      srcErrMsg("Error 0254: JUMP quad addition failed.");
      return(false);
   }

   return(true);
}

//==============================================================================
// BXSCompiler::parseContinue
//==============================================================================
bool BXSCompiler::parseContinue(bool incrementTokenFirst)
{
   #ifdef DEBUGTRACEPARSE
   mMessenger->infoMsg("ParseContinue.");
   #endif

   //continue.
   long token;
   if (incrementTokenFirst == true)
      token=getTokenizer()->getDarkSpaceToken(this);
   else
      token=getTokenizer()->getCurrentToken();
   if (token != BXSTokenizer::cContinue)
   {
      srcErrMsg("Error 0326: expected 'continue' and found '%s'.", getTokenizer()->getCurrentTokenText());
      return(false);
   }

   //If we don't have any continue labels in the stack, this is an error.
   if (mContinueLabelIDs.getNumber() <= 0)
   {
      srcErrMsg("Error 0327: invalid 'continue' location.");
      return(false);
   }

   //Add the jump to the top label in the continue label ID stack.
   if (mQuads.add(BXSQuad(BXSQuadOpcode::cJUMP, 0, mContinueLabelIDs[mContinueLabelIDs.getNumber()-1])) == -1)
   {
      srcErrMsg("Error 0328: JUMP quad addition failed.");
      return(false);
   }

   return(true);
}

//==============================================================================
// BXSCompiler::parseBreakpoint
//==============================================================================
bool BXSCompiler::parseBreakpoint(void)
{
   #ifdef DEBUGTRACEPARSE
   mMessenger->infoMsg("ParseBreakpoint.");
   #endif

   //breakpoint.
   long token=getTokenizer()->getCurrentToken();
   if (token != BXSTokenizer::cBreakpoint)
   {
      srcErrMsg("Error 0318: expected 'breakpoint' and found '%s'.", getTokenizer()->getCurrentTokenText());
      return(false);
   }

   //Add the breakpoint quad.
   if (mQuads.add(BXSQuad(BXSQuadOpcode::cBPNT)) == -1)
   {
      srcErrMsg("Error 0319: BPNT quad addition failed.");
      return(false);
   }

   return(true);
}

//==============================================================================
// BXSCompiler::parseRule
//==============================================================================
bool BXSCompiler::parseRule(bool incrementTokenFirst)
{
   #ifdef DEBUGTRACEPARSE
   mMessenger->infoMsg("ParseRule.");
   #endif
   if (getTokenizer() == NULL)
      return(false);
   
   //NOTE: Rules are just functions that are called differently.  So, this code ends
   //up looking a lot like the function code.  If you make changes to the function-ish
   //stuff here, you should propagate that in parseFunctionDecl.

   //rule.
   long token;
   if (incrementTokenFirst == true)
      token=getTokenizer()->getDarkSpaceToken(this);
   else
      token=getTokenizer()->getCurrentToken();
   if (token != BXSTokenizer::cRule)
   {
      srcErrMsg("Error 0258: expected 'rule' and found '%s'.", getTokenizer()->getCurrentTokenText());
      return(false);
   }

   //Start code offset.
   long codeOffset=mSource->getCodeSize();

   //Get the rule name.  It's an error if it's already defined.
   token=getTokenizer()->getDarkSpaceToken(this);
   const BXSSymbolEntry *se=getSymbolEntry(getTokenizer()->getCurrentTokenText(), true);
   if (se != NULL)
   {
      srcErrMsg("Error 0259: '%s' is already defined and cannot be a rule name.", getTokenizer()->getCurrentTokenText());
      return(false);
   }
   //Create the rule name and set the scope.
   char ruleName[BXSTokenizer::cMaxTokenSize+1];
   BASSERT(getTokenizer()->getCurrentTokenText() != NULL);
   StringCchCopyA(ruleName, BXSTokenizer::cMaxTokenSize+1, getTokenizer()->getCurrentTokenText());
   bsnprintf(mCurrentScope, sizeof(mCurrentScope), "%s$%s$", getTokenizer()->getFilenameWithoutExtension().getPtr(), ruleName);

   //Create the new function entry for the rule.
   BXSFunctionEntry *newFE=mData->allocateFunctionEntry();
   if (newFE == NULL)
   {
      srcErrMsg("Error 0260: cannot create new function entry for '%s' rule.", ruleName);
      return(false);
   }
   //This is now the current function ID.
   mCurrentFunctionID=newFE->getID();

   //Add some debug listing.
   mEmitter->addListLine(true, "");
   mEmitter->addListLine(true, "//====================");
   mEmitter->addListLine(true, "// Rule '%s':", ruleName);
   mEmitter->addListLine(true, "//====================");

   //Add the symbol for the rule name.  The symbol type is cFunction.  The value
   //is the unique ID for the function.  addSymbol returns the unique symbol ID for
   //this variable, so we stuff that into the ID slot in the variable table so that
   //we can reference it later.
   long symbolID=mSource->getSymbols().addSymbol(ruleName, BXSBinary::cFunction, newFE->getID());
   if (symbolID < 0)
   {
      srcErrMsg("Error 0262: unable to add an entry to the symbol table.");
      return(false);
   }

   //Set the data for the function side of the rule.
   newFE->setSymbolID(symbolID);
   newFE->setReturnType(BXSVariableEntry::cVoidVariable);
   newFE->setFileID(getTokenizer()->getFileID());

   //Create the new rule entry for the rule.
   BXSRuleEntry *newRE=mData->allocateRuleEntry();
   if (newRE == NULL)
   {
      srcErrMsg("Error 0263: cannot create new rule entry for '%s' rule.", ruleName);
      return(false);
   }
   //Set the rule data.
   newRE->setFunctionID(newFE->getID());

   //Parse the optional rule stuff:  Priority, Interval, and Group(s).
   token=getTokenizer()->getDarkSpaceToken(this);
   bool setPriority=false;
   bool setMinInterval=false;
   bool setMaxInterval=false;
   bool setActiveState=false;
   bool setRunImmediatelyState=false;
   while (token != BXSTokenizer::cLeftBrace)
   {
      switch (token)
      {
         //Priority.
         case BXSTokenizer::cPriority:
         {
            if (setPriority == true)
            {
               srcErrMsg("Error 0265: rule priority cannot be set twice.");
               return(false);
            }
            long v=0;
            if (parseIntegerConstant(&v) == false)
            {
               srcErrMsg("Error 0266: parseIntegerConstant failed.");
               return(false);
            }
            newRE->setPriority(v);
            setPriority=true;
            break;
         }
         //Min Interval.
         case BXSTokenizer::cMinInterval:
         {
            if (setMinInterval == true)
            {
               srcErrMsg("Error 0267: rule min interval cannot be set twice.");
               return(false);
            }
            long v=0;
            if (parseIntegerConstant(&v) == false)
            {
               srcErrMsg("Error 0268: parseIntegerConstant failed.");
               return(false);
            }
            //This is specified as seconds in the XS file, so we convert to ms here.
            newRE->setMinInterval((DWORD)(v*1000));
            setMinInterval=true;
            break;
         }
         //Max Interval.
         case BXSTokenizer::cMaxInterval:
         {
            if (setMaxInterval == true)
            {
               srcErrMsg("Error 0280: rule max interval cannot be set twice.");
               return(false);
            }
            long v=0;
            if (parseIntegerConstant(&v) == false)
            {
               srcErrMsg("Error 0281: parseIntegerConstant failed.");
               return(false);
            }
            //This is specified as seconds in the XS file, so we convert to ms here.
            newRE->setMaxInterval((DWORD)(v*1000));
            setMaxInterval=true;
            break;
         }
         //High Frequency.
         case BXSTokenizer::cHighFrequency:
         {
            if (setMinInterval == true)
            {
               srcErrMsg("Error 0314: rule min interval cannot be set twice.");
               return(false);
            }
            if (setMaxInterval == true)
            {
               srcErrMsg("Error 0315: rule max interval cannot be set twice.");
               return(false);
            }

            //Set the min to the min and the max to the max.
            newRE->setMinInterval(BXSRuleEntry::cMinimumMinInterval);
            newRE->setMaxInterval(BXSRuleEntry::cMinimumMaxInterval);
            setMinInterval=true;
            setMaxInterval=true;
            break;
         }
         //Active.
         case BXSTokenizer::cActive:
         case BXSTokenizer::cInactive:
         {
            if (setActiveState == true)
            {
               srcErrMsg("Error 0316: rule active/inactive cannot be set twice.");
               return(false);
            }

            //Set the rule active.
            if (token == BXSTokenizer::cActive)
               newRE->setActive(true);
            else
               newRE->setActive(false);
            setActiveState=true;
            break;
         }
         //RunImmediately.
         case BXSTokenizer::cRunImmediately:
         {
            if (setRunImmediatelyState == true)
            {
               srcErrMsg("Error 0361: runImmediately cannot be set twice.");
               return(false);
            }

            //Set the rule to runImmediately.
            newRE->setRunImmediately(true);
            setRunImmediatelyState=true;
            break;
         }
         //Groups.
         case BXSTokenizer::cGroup:
         {
            token=getTokenizer()->getDarkSpaceToken(this);
            if (token != BXSTokenizer::cName)
            {
               srcErrMsg("Error 0269: expected group name and found '%s'.", getTokenizer()->getCurrentTokenText());
               return(false);
            }

            //Grab the symbol entry (if it's there).
            long groupID=-1;
            const BXSSymbolEntry *se2=getSymbolEntry(getTokenizer()->getCurrentTokenText(), true);
            if (se2 == NULL)
            {
               groupID=createRuleGroup(getTokenizer()->getCurrentTokenText());
               if (groupID < 0)
               {
                  srcErrMsg("Error 0270: createRuleGroup failed.");
                  return(false);
               }
            }
            else
            {
               if (se2->mType != BXSBinary::cRuleGroup)
               {
                  srcErrMsg("Error 0271: '%s' is already in use and cannot be a group name.", getTokenizer()->getCurrentTokenText());
                  return(false);
               }
               groupID=se2->mValue;
            }

            //Actually add this rule to the group and vice versa.
            mData->getRuleGroupEntry(groupID)->addRule(newRE->getID());
            newRE->addGroup(groupID);
            break;
         }

         default:
            srcErrMsg("Error 0272: expected 'priority', 'minInterval', 'maxInterval', or 'group' and found '%s'.", getTokenizer()->getCurrentTokenText());
            return(false);
      }
      token=getTokenizer()->getDarkSpaceToken(this);
   }
   //Back up (to put the '{' back as the current token).
   getTokenizer()->decrementCurrentToken(this);

   //Add the activation time variable.
   char activationVariableName[(BXSTokenizer::cMaxTokenSize+1)*2];
   bsnprintf(activationVariableName, sizeof(activationVariableName), "%scActivationTime", mCurrentScope);
   //Allocate the new variable entry.
   BXSVariableEntry *activationVE=new BXSVariableEntry();
   if (activationVE == NULL)
   {
      srcErrMsg("Error 0321: failed to create variable entry for rule activation variable.");
      return(false);
   }
   //This is a static, const variable as far as the compiler is concerned.
   activationVE->setConst(true);
   activationVE->setStatic(true);
   long defaultActivationValue=0;
   if (activationVE->setAll(-1, BXSVariableEntry::cIntegerVariable, &defaultActivationValue) == false)
   {
      srcErrMsg("Error 00322: setAll failed for rule activation variable.");
      return(false);
   }
   //Add it to the symbol table.  The symbol type is cStackVariable.  The ID of variable is
   //0 since a rule doesn't have parms and this is the first variable in the rule.  addSymbol
   //returns the unique ID for this variable, so we stuff that into the ID slot in the variable
   //table so that we can reference it later.
   long activationSymbolID=mSource->getSymbols().addSymbol(activationVariableName, BXSBinary::cStackVariable, 0);
   if (activationSymbolID < 0)
   {
      srcErrMsg("Error 0323: unable to add an entry to the file symbol table for the rule activation variable.");
      return(false);
   }
   //Set the symbol ID now that we have it.
   activationVE->setSymbolID(activationSymbolID);
   newFE->addVariable(activationVE);


   //Parse the code inside the function/rule.
   bool returnAdded=false;
   if (parseCode(false, true, &returnAdded) == false)
   {
      #ifdef DEBUGEMITLISTSONERROR
      emitTheLists(mQuads, &codeOffset);
      #endif
      srcErrMsg("Error 0273: could not parse the code for '%s' rule.", ruleName);
      return(false);
   }

   //Add the RET quad (if there wasn't one already added).
   if (returnAdded == false)
   {
      if (mQuads.add(BXSQuad(BXSQuadOpcode::cRET)) == -1)
      {
         srcErrMsg("Error 0274: could not add RET quad for '%s' rule.", ruleName);
         return(false);
      }
   }

   //Now that we've parsed the rule, we need to emit the quads for it.
   if (mQuads.getNumber() > 0)
   {
      if ((emitTheLists(mQuads, &codeOffset) == false) || (codeOffset < 0))
      {
         srcErrMsg("Error 0275: could not emit quads for '%s' rule.", ruleName);
         return(false);
      }
   }
   //Empty the quads.
   mQuads.setNumber(0);
   //Now that we have a valid code offset, save it in the function entry.
   newFE->setCodeOffset(codeOffset);

   return(true);
}

//==============================================================================
// BXSCompiler::parseCode
//==============================================================================
bool BXSCompiler::parseCode(bool singleStatement, bool topLevel, bool *returnAdded)
{
   #ifdef DEBUGTRACEPARSE
   mMessenger->infoMsg("ParseCode.");
   #endif
   if (returnAdded != NULL)
      *returnAdded=false;

   //This expects code that is either surrounded by a left and right brace or a single
   //line of code (braced or unbraced); this is controlled by the singleStatement
   //bool.  All code is dumped straight into the quads.

   //'{'.  We must have the brace if singleStatement == false.
   long token=getTokenizer()->getDarkSpaceToken(this);
   bool hasLeftBrace=false;
   if (token == BXSTokenizer::cLeftBrace)
      hasLeftBrace=true;
   if ((token != BXSTokenizer::cLeftBrace) && (singleStatement == false))
   {
      srcErrMsg("Error 0128: expected '{' and found '%s'.", getTokenizer()->getCurrentTokenText());
      return(false);
   }

   //Parse the actual function definition.  Grab another token if we have a left brace.
   if (hasLeftBrace == true)
      token=getTokenizer()->getDarkSpaceToken(this);
   bool outputListSeparator=true;
   while (token != BXSTokenizer::cRightBrace)
   {
      //Prep.
      bool stripSemiColon=false;
      outputListSeparator=true;

      //Process the token based on what it is.
      switch (token)
      {
         //Variable declaration.
         case BXSTokenizer::cInteger:
         case BXSTokenizer::cFloat:
         case BXSTokenizer::cBool:
         case BXSTokenizer::cString:
         case BXSTokenizer::cVector:
         case BXSTokenizer::cConst:
         case BXSTokenizer::cStatic:
         {
            if (parseVarDecl(false, true, false, false) == false)
            {
               srcErrMsg("Error 0136: parseVarDecl failed.");
               return(false);
            }
            outputListSeparator=false;
            break;
         }

         //If statement.
         case BXSTokenizer::cIf:
         {
            if (parseConditionDecl() == false)
            {
               srcErrMsg("Error 0135: parseConditionDecl failed.");
               return(false);
            }
            break;
         }

         //Label.
         case BXSTokenizer::cLabel:
         {
            if (parseLabelDecl() == false)
            {
               srcErrMsg("Error 0137: parseLabelDecl failed.");
               return(false);
            }
            break;
         }
         //Goto.
         case BXSTokenizer::cGoto:
         {
            if (parseGotoDecl() == false)
            {
               srcErrMsg("Error 0138: parseGotoDecl failed.");
               return(false);
            }
            break;
         }

         //For.
         case BXSTokenizer::cFor:
         {
            if (parseForLoop() == false)
            {
               srcErrMsg("Error 0139: parseForLoop failed.");
               return(false);
            }
            break;
         }

         //DBG.
         case BXSTokenizer::cDBG:
         {
            token=getTokenizer()->getDarkSpaceToken(this);
            if (parseDBG(token, getTokenizer()->getCurrentTokenText()) == false)
            {
               srcErrMsg("Error 0140: parseDBG failed.");
               return(false);
            }
            stripSemiColon=true;
            break;
         }

         //ILL.
         case BXSTokenizer::cILL:
         {
            token=getTokenizer()->getDarkSpaceToken(this);
            if (parseILL() == false)
            {
               srcErrMsg("Error 0292: parseILL failed.");
               return(false);
            }
            stripSemiColon=true;
            break;
         }

         //IRL.
         case BXSTokenizer::cIRL:
         {
            token=getTokenizer()->getDarkSpaceToken(this);
            if (parseIRL() == false)
            {
               srcErrMsg("Error 0293: parseIRL failed.");
               return(false);
            }
            stripSemiColon=true;
            break;
         }

         //Return.
         case BXSTokenizer::cReturn:
         {
            if (parseReturn() == false)
            {
               srcErrMsg("Error 0141: parseReturn failed.");
               return(false);
            }
            //Denote that we did add a return quad already.
            if (returnAdded != NULL)
               *returnAdded=true;
            stripSemiColon=false;
            break;
         }

         //Switch.
         case BXSTokenizer::cSwitch:
         {
            if (parseSwitch(false) == false)
            {
               srcErrMsg("Error 0221: parseSwitch failed.");
               return(false);
            }
            stripSemiColon=false;
            break;
         }

         //While.
         case BXSTokenizer::cWhile:
         {
            if (parseWhile(false) == false)
            {
               srcErrMsg("Error 0239: parseWhile failed.");
               return(false);
            }
            stripSemiColon=false;
            break;
         }

         //Break.
         case BXSTokenizer::cBreak:
         {
            if (parseBreak(false) == false)
            {
               srcErrMsg("Error 0240: parseBreak failed.");
               return(false);
            }
            stripSemiColon=true;
            break;
         }

         //Breakpoint.
         case BXSTokenizer::cBreakpoint:
         {
            if (parseBreakpoint() == false)
            {
               srcErrMsg("Error 0317: parseBreakpoint failed.");
               return(false);
            }
            stripSemiColon=true;
            break;
         }

         //Continue.
         case BXSTokenizer::cContinue:
         {
            if (parseContinue(false) == false)
            {
               srcErrMsg("Error 0325: parseContinue failed.");
               return(false);
            }
            stripSemiColon=true;
            break;
         }

         //Stuff that needs a symbol lookup (e.g. syscall).
         default:
         {
            //Grab the symbol entry.
            const BXSSymbolEntry *se=getSymbolEntry(getTokenizer()->getCurrentTokenText(), true);
            if (se == NULL)
            {
               srcErrMsg("Error 0310: invalid symbol lookup for '%s'.", getTokenizer()->getCurrentTokenText());
               return(false);
            }

            //Syscalls and Functions.
            if ((se->mType == BXSBinary::cSyscall) || (se->mType == BXSBinary::cFunction))
            {
               BXSParseTreeNode* pt=parseSysFuncCall(se);
               if (pt == NULL)
               {
                  if (se->mType == BXSBinary::cSyscall)
                     srcErrMsg("Error 0131: '%s' is a bad syscall - unable to parse.", getTokenizer()->getCurrentTokenText());
                  else
                     srcErrMsg("Error 0132: '%s' is a bad function - unable to parse.", getTokenizer()->getCurrentTokenText());
                  return(false);
               }

               //Spit out the parse tree for this syscall.
               if (pt->outputQuads(mQuads, mMessenger) == false)
               {
                  srcErrMsg("Error 0133: PT::outputQuads failed.");
                  return(false);
               }

               //Pop the return value (if any).
               long returnType=BXSVariableEntry::cVoidVariable;
               if (se->mType == BXSBinary::cSyscall)
                  returnType=mSyscalls->getSyscall(se->mValue)->getReturnType();
               else
                  returnType=mData->getFunctionEntry(se->mValue)->getReturnType();
               if (returnType != BXSVariableEntry::cVoidVariable)
               {
                  if (mQuads.add(BXSQuad(BXSQuadOpcode::cPOP, 1)) == -1)
                  {
                     srcErrMsg("Error 0134: POP quad addition failed.");
                     return(false);
                  }
               }

               //Clean up.
               delete pt;

               stripSemiColon=true;
            }
            //Variable stuff.
            else if ((se->mType == BXSBinary::cVariable) || (se->mType == BXSBinary::cStackVariable))
            {
               if (parseVariableAssignment(token, getTokenizer()->getCurrentTokenText(), true, false) == false)
               {
                  srcErrMsg("Error 0142: parseVariableAssignment failed.");
                  return(false);
               }
            }
            //User Class.
            else if (se->mType == BXSBinary::cClass)
            {
               if (parseClassDeclaration(false, true, false) == false)
               {
                  srcErrMsg("Error 0381: parseClassDeclaration failed.");
                  return(false);
               }
               outputListSeparator=false;
               break;
            }
            else
            {
               srcErrMsg("Error 0143: '%s' is an unrecognized/currently unsupported symbol - unable to parse.", getTokenizer()->getCurrentTokenText());
               return(false);
            }

            break;
         }
      }

      //';'
      if (stripSemiColon == true)
      {
         token=getTokenizer()->getDarkSpaceToken(this);
         if (token != BXSTokenizer::cSemiColon)
         {
            srcErrMsg("Error 0144: expected ';' and found '%s'.", getTokenizer()->getCurrentTokenText());
            return(false);
         }
      }

      //If we have a left brace, we expect a right brace, so we only stop on that.
      if (hasLeftBrace == true)
      {
         token=getTokenizer()->getDarkSpaceToken(this);
         if (token == BXSTokenizer::cRightBrace)
            return(true);
      }
      //Else, if we don't have a left brace, we only do one thing (which we have if we're
      //here).  So we return.
      else
         return(true);

      //If we're top level, spit out a SEP quad.
      if ((topLevel == true) && (outputListSeparator == true))
         mQuads.add(BXSQuad(BXSQuadOpcode::cSEP));
   }

   return(true);
}

//==============================================================================
// BXSCompiler::parseSysFuncCall
//==============================================================================
BXSParseTreeNode* BXSCompiler::parseSysFuncCall(const BXSSymbolEntry *se)
{
   #ifdef DEBUGTRACEPARSE
   mMessenger->infoMsg("ParseSysFuncCall.");
   #endif

   //Double check that the ID is valud.  Get the number of args, too.
   long numberArguments=0;
   if (se->mType == BXSBinary::cSyscall)
   {
      if ((se->mValue < 0) || (se->mValue >= mSyscalls->getNumberSyscalls()) )
      {
         srcErrMsg("Error 0146: %d is an invalid syscall ID.", se->mValue);
         return(NULL);
      }
      numberArguments=mSyscalls->getSyscall(se->mValue)->getNumberParameters();
   }
   else
   {
      if ((se->mValue < 0) || (se->mValue >= mData->getNumberFunctions()) )
      {
         srcErrMsg("Error 0147: %d is an invalid functionID.", se->mValue);
         return(NULL);
      }
      numberArguments=mData->getFunctionEntry(se->mValue)->getNumberParameters();
   }

   //Create the root parse node.
   BXSParseTreeNode *root=new BXSParseTreeNode;
   if (root == NULL)
   {
      srcErrMsg("Error 0148: could not allocate root PTNode.");
      return(NULL);
   }
   root->setNodeType(se->mType);
   root->setValue(se->mValue);

   //The left paren is used to signal that arguments are being passed.
   long numberArgumentsCounted=0;
   long token=getTokenizer()->getDarkSpaceToken(this);
   //If we don't have a left (, we are not using the token so push it back.
   if (token != BXSTokenizer::cLeftParen) 
      getTokenizer()->decrementCurrentToken(this);
   //Else, if we have a left (, check for a ) paren.
   if ((numberArguments == 0) && (token == BXSTokenizer::cLeftParen))
   {
      long nextToken=getTokenizer()->getDarkSpaceToken(this);
      if (nextToken != BXSTokenizer::cRightParen)
      {
         getTokenizer()->decrementCurrentToken(this);
         srcErrMsg("Error 0149: this syscall doesn't allow arguments, so the call shouldn't include parenthesis.");
         delete root;
         return(NULL);
      }
   }
   //Else, we have args to parse.
   else
   {
      bool needSeparator=false;
      for (;;)
      {
         //Grab the next token.
         token=getTokenizer()->getToken(this);

         //End of string.
         if (token == BXSTokenizer::cEOS)
         {
            srcErrMsg("Error 0150: Unexpected end of input.");
            delete root;
            return(NULL);
         }
         //Break if we hit the right paren (since any/all args of a syscall are now optional).
         if (token == BXSTokenizer::cRightParen)
            break;
         //Separator.
         if (token == BXSTokenizer::cComma)
         {
            needSeparator=false;
            continue;
         }
         if (needSeparator == true)
         {
            srcErrMsg("Error 0151: Missing ',' after arg %d - '%s'.", numberArgumentsCounted, se->mSymbol);
            delete root;
            return(NULL);
         }

         //Are there too many arguments?
         numberArgumentsCounted++;
         if (numberArgumentsCounted > numberArguments)
         {
            srcErrMsg("Error 0152: too many arguments for sys call '%s'.", se->mSymbol);
            delete root;
            return(NULL);
         }

         //Get the arg type.
         long parmType=BXSVariableEntry::cInvalidVariable;
         if (se->mType == BXSBinary::cSyscall)
            parmType=mSyscalls->getSyscall(se->mValue)->getParameter(numberArgumentsCounted-1)->getType();
         else
            parmType=mData->getFunctionEntry(se->mValue)->getVariable(numberArgumentsCounted-1)->getType();

         //Parse the expression for this parm.
         BXSParseTreeNode *ppt=parseExpression2(false, true, false, true, false, false, parmType);
         if (ppt == NULL)
         {
            srcErrMsg("Error 0153: parseExpression2 failed.");
            delete root;
            return(NULL);
         }

         //Add this parse tree as a parm.
         if (root->addParameter(ppt) == false)
         {
            srcErrMsg("Error 0154: addParm failed.");
            delete ppt;
            delete root;
            return(NULL);
         }

         //Need a separator between args.
         needSeparator=true;
     }
   }

   //Fill in any optional arguments required.
   for ( ; numberArgumentsCounted < numberArguments; numberArgumentsCounted++)
   {
      BXSVariableEntry *parm=NULL;
      if (se->mType == BXSBinary::cSyscall)
         parm=mSyscalls->getSyscall(se->mValue)->getParameter(numberArgumentsCounted);
      else
         parm=mData->getFunctionEntry(se->mValue)->getVariable(numberArgumentsCounted);
      if (parm == NULL)
      {
         srcErrMsg("Error 0155: optional parm creation failed.");
         delete root;
         return(NULL);
      }

      //Create an operand node for the 'optional' argument.
      BXSParseTreeNode *optPT=new BXSParseTreeNode;
      if (optPT == NULL)
      {
         delete root;
         srcErrMsg("Error 0282: new PTNode allocation failed.");
         return(NULL);
      }

      //If we have a string or a vector, create the temp var first.
      if ((parm->getType() == BXSVariableEntry::cStringVariable) || (parm->getType() == BXSVariableEntry::cVectorVariable))
      {
         //Actually create the temporary.
         long variableIndex=createTemporaryVariable(parm->getType(), parm->getData());
         if (variableIndex < 0)
         {
            srcErrMsg("Error 0156: createTempVar failed.");
            delete optPT;
            delete root;
            return(NULL);
         }
         optPT->setNodeType(BXSBinary::cVariable);
         optPT->setResultType(parm->getType());
         optPT->setValue(variableIndex);
      }
      else
      {
         optPT->setNodeType(BXSBinary::cImmediateVariable);
         optPT->setResultType(parm->getType());
         long v=0;
         //Cast bools differently due to size.
         if (parm->getType() == BXSVariableEntry::cBoolVariable)
            v=(long)*((bool*)parm->getData());
         else
            v=*((long*)parm->getData());
         optPT->setValue(v);
      }

      //Now, add it as a parm.
      if (root->addParameter(optPT) == false)
      {
         srcErrMsg("Error 0283: addParm failed.");
         delete optPT;
         delete root;
         return(NULL);
      }
   }

   //Make sure we had the right number of pushes.
   if (numberArguments != numberArgumentsCounted)
   {
      srcErrMsg("Error 0159: %d args read.", numberArgumentsCounted);
      delete root;
      return(NULL);
   }

   return(root);
}

//==============================================================================
// BXSCompiler::parseAtomic
//==============================================================================
bool BXSCompiler::parseAtomic(long token, char* tokenText, long *variableIndex,
   long *variableValue, long *variableType, long *variableOffset, bool *immediateVariable)
{
   #ifdef DEBUGTRACEPARSE
   mMessenger->infoMsg("ParseAtomic.");
   #endif

   //Default.
   *variableIndex=-1;
   *variableValue=-1;
   *variableType=BXSVariableEntry::cInvalidVariable;
   *variableOffset=0;
   *immediateVariable=false;

   //INT, FLOAT, BOOL, or STRING constants.
   if ((token == BXSTokenizer::cInteger) ||
      (token == BXSTokenizer::cFloat) ||
      (token == BXSTokenizer::cBool) ||
      (token == BXSTokenizer::cString) ||
      (token == BXSTokenizer::cVector))
   {
      switch (token)
      {
         case BXSTokenizer::cInteger:
            *variableType=BXSVariableEntry::cIntegerVariable;
            break;
         case BXSTokenizer::cFloat:
            *variableType=BXSVariableEntry::cFloatVariable;
            break;
         case BXSTokenizer::cBool:
            *variableType=BXSVariableEntry::cBoolVariable;
            break;
         case BXSTokenizer::cString:
            *variableType=BXSVariableEntry::cStringVariable;
            break;
         case BXSTokenizer::cVector:
            *variableType=BXSVariableEntry::cVectorVariable;
            break;
      }

      //Create the temporary for strings and vectors (return the index in variableIndex).
      //Return the value (as a long) for floats, bools, and integers (in variableValue).
      switch (*variableType)
      {
         case BXSVariableEntry::cIntegerVariable:
         {
            *variableValue=atoi(tokenText);
            *immediateVariable=true;
            return(true);
         }
         case BXSVariableEntry::cFloatVariable:
         {
            float value=(float)atof(tokenText);
            *variableValue=*(long*)&value;
            *immediateVariable=true;
            return(true);
         }
         case BXSVariableEntry::cBoolVariable:
         {
            if (stricmp(tokenText, "false") == 0)
               *variableValue=false;
            else
               *variableValue=true;
            *immediateVariable=true;
            return(true);
         }
         case BXSVariableEntry::cStringVariable:
            *variableIndex=createTemporaryVariable(*variableType, tokenText);
            break;
         case BXSVariableEntry::cVectorVariable:
         {
            BVector value;
            if (parseVectorConstant(&value) == false)
            //TODO: Replace the constant parse call with the commented out one once the
            //new function is done.
            //if (parseVector(&value) == false)
            {
               srcErrMsg("Error 0160: parseVectorConstant failed.");
               return(false);
            }
            *variableIndex=createTemporaryVariable(*variableType, &value);
            break;
         }
      }

      if (*variableIndex < 0)
      {
         srcErrMsg("Error 0161: invalid variableIndex.");
         return(false);
      }
      return(true);
   }

   //Get the symbol entry for the name.
   const BXSSymbolEntry *se=getSymbolEntry(tokenText, true);
   if ((se == NULL) || (token != BXSTokenizer::cName))
   {
      srcErrMsg("Error 0162: '%s' is an unrecognized variable name.", getTokenizer()->getCurrentTokenText());
      return(false);
   }

   //VARIABLES.
   if ((se->mType == BXSBinary::cVariable) || (se->mType == BXSBinary::cStackVariable))
   {
      //Get the variable type for this variable.
      BXSVariableEntry *ve=NULL;
      if (se->mType == BXSBinary::cVariable)
         ve=mData->getVariableEntry(-se->mValue-1);
      else
      {
         BXSFunctionEntry *fe=mData->getFunctionEntry(mCurrentFunctionID);
         BASSERT(fe != NULL);
         ve=fe->getVariable(se->mValue);
      }
      *variableType=ve->getType();

      //Bad symbol types.
      if ((*variableType != BXSVariableEntry::cIntegerVariable) &&
         (*variableType != BXSVariableEntry::cFloatVariable) &&
         (*variableType != BXSVariableEntry::cBoolVariable) &&
         (*variableType != BXSVariableEntry::cStringVariable) &&
         (*variableType != BXSVariableEntry::cVectorVariable) &&
         (*variableType != BXSVariableEntry::cUserClassVariable))
      {
         srcErrMsg("Error 0163: variable '%s' is an unrecognized variable type.", getTokenizer()->getCurrentTokenText());
         return(false);
      }

      //Index is the value of the symbol entry.
      *variableIndex=se->mValue;

      //If we have a user class and the next tokens are '.' and a name, parse that as
      //the class '.' notation.  Use that .name to figure out the offset.  VariableType then
      //becomes the type of that offset variable inside the class.
      if (*variableType == BXSVariableEntry::cUserClassVariable)
      {
         //Expect a '.'.
         long nextToken=getTokenizer()->getDarkSpaceToken(this);
         if (nextToken != BXSTokenizer::cPeriod)
         {
            srcErrMsg("Error 0401: expected '.' and found %s.", getTokenizer()->getCurrentTokenText());
            return(false);
         }
         //We expect a name now.
         nextToken=getTokenizer()->getDarkSpaceToken(this);
         if (nextToken != BXSTokenizer::cName)
         {
            srcErrMsg("Error 0402: expected a class member variable name and found %s.", getTokenizer()->getCurrentTokenText());
            return(false);
         }

         return(false);

      }
      return(true);
   }

   srcErrMsg("Error 0164: found '%s' when looking for either an atomic term.", getTokenizer()->getCurrentTokenText());
   return(false);
}

//==============================================================================
// BXSCompiler::parseExpression
//==============================================================================
bool BXSCompiler::parseExpression(bool incrementTokenFirst, bool stopOnParen,
   bool stopOnSemiColon, bool stopOnComma, bool stopOnColon, bool stripStopToken,
   long resultType)
{
   #ifdef DEBUGTRACEPARSE
   mMessenger->infoMsg("ParseExpression.");
   #endif

   //Get a parse tree for the expression.
   BXSParseTreeNode *root=parseExpression2(incrementTokenFirst, stopOnParen,
      stopOnSemiColon, stopOnComma, stopOnColon, stripStopToken, resultType);
   if (root == NULL)
   {
      srcErrMsg("Error 0165: high level parseExpr2 failed.");
      return(false);
   }

   //Spit out the quads.
   if (root->outputQuads(mQuads, mMessenger) == false)
   {
      srcErrMsg("Error 0166: quad output failed.");
      return(false);
   }

   //Clean up.
   delete root;
   return(true);
}

//==============================================================================
// BXSCompiler::parseExpression2
//==============================================================================
BXSParseTreeNode* BXSCompiler::parseExpression2(bool incrementTokenFirst,
   bool stopOnParen, bool stopOnSemiColon, bool stopOnComma, bool stopOnColon,
   bool stripStopToken, long resultType)
{
   #ifdef DEBUGTRACEPARSE
   mMessenger->infoMsg("  ParseExpression2.");
   #endif

   //Root node.
   BXSParseTreeNode *root=NULL;
   BXSParseTreeNode *cur=NULL;

   //Build the parse tree from the expression.
   long token=-1;
   if (incrementTokenFirst == true)
      token=getTokenizer()->getDarkSpaceToken(this);
   else
      token=getTokenizer()->getCurrentToken();

   for (;;)
   {
      //Check for the end condition.
      if ( ((stopOnParen == true) && (token == BXSTokenizer::cRightParen)) ||
         ((stopOnSemiColon == true) && (token == BXSTokenizer::cSemiColon)) ||
         ((stopOnComma == true) && (token == BXSTokenizer::cComma)) ||
         ((stopOnColon == true) && (token == BXSTokenizer::cColon)) )
         break;

      //Denotes whether or not we had an operand.
      long component=-1;

      //Simple Integers, Floats, Bools, Strings, and variables.
      const BXSSymbolEntry *se=getSymbolEntry(getTokenizer()->getCurrentTokenText(), true);
      if ((token == BXSTokenizer::cInteger) ||
         (token == BXSTokenizer::cFloat) ||
         (token == BXSTokenizer::cBool) ||
         (token == BXSTokenizer::cString) ||
         (token == BXSTokenizer::cVector) ||
         ((token == BXSTokenizer::cName) &&
            (se != NULL) &&
            ((se->mType == BXSBinary::cVariable) || (se->mType == BXSBinary::cStackVariable))) )
      {
         component=0;

         //Create the operand node.
         BXSParseTreeNode *newNode=new BXSParseTreeNode;
         if (newNode == NULL)
         {
            BDELETE(root);
            srcErrMsg("Error 0167: new PTNode allocation failed.");
            return(NULL);
         }
         long variableIndex=-1;
         long variableValue=-1;
         long variableOffset=0;
         long variableType=BXSVariableEntry::cInvalidVariable;
         bool immediateVariable=false;
         if (parseAtomic(token, getTokenizer()->getCurrentTokenText(), &variableIndex, &variableValue, &variableType, &variableOffset, &immediateVariable) == false)
         {
            delete newNode;
            BDELETE(root);
            srcErrMsg("Error 0168: parseAtomic failed.");
            return(NULL);
         }
         if (immediateVariable == true)
         {
            newNode->setNodeType(BXSBinary::cImmediateVariable);
            newNode->setResultType(variableType);
            newNode->setValue(variableValue);
            newNode->setOffset(variableOffset);
         }
         else
         {
            newNode->setNodeType(BXSBinary::cVariable);
            newNode->setResultType(variableType);
            newNode->setValue(variableIndex);
            newNode->setOffset(variableOffset);
         }

         //Add it to the parse tree.
         if (cur == NULL)
            cur=newNode;
         else
         {
            if (cur->getLeft() == NULL)
               cur->setLeft(newNode);
            else
               cur->setRight(newNode);
         }
      }
      //Syscalls and Functions.  These must have a return value.
      else if ((token == BXSTokenizer::cName) && (se != NULL) &&
         ((se->mType == BXSBinary::cSyscall) || (se->mType == BXSBinary::cFunction)) )
      {
         component=0;

         //Get the return type.
         long returnType=BXSVariableEntry::cVoidVariable;
         if (se->mType == BXSBinary::cFunction)
            returnType=mData->getFunctionEntry(se->mValue)->getReturnType();
         else
            returnType=mSyscalls->getSyscall(se->mValue)->getReturnType();
         if (returnType == BXSVariableEntry::cVoidVariable)
         {
            BDELETE(root);
            srcErrMsg("Error 0169: void syscall/function cannot be called here.");
            return(NULL);
         }

         //If this is a valid call, parse it.
         BXSParseTreeNode* newNode=parseSysFuncCall(se);
         if (newNode == NULL)
         {
            BDELETE(root);
            srcErrMsg("Error 0170: parseSysFuncCall failed.");
            return(NULL);
         }
         newNode->setResultType(returnType);

         //Add it to the parse tree.
         if (cur == NULL)
            cur=newNode;
         else
         {
            if (cur->getLeft() == NULL)
               cur->setLeft(newNode);
            else
               cur->setRight(newNode);
         }
      }
      //Parenthesized sub expressions.
      else if (token == BXSTokenizer::cLeftParen)
      {
         component=-1;

         //Get the parse tree for the sub expr.
         BXSParseTreeNode *newNode=parseExpression2(true, true, false, false, false, true, BXSVariableEntry::cInvalidVariable);
         if (newNode == NULL)
         {
            BDELETE(root);
            srcErrMsg("Error 0171: parseExpression2 failed.");
            return(NULL);
         }

         //Add it to the parse tree.
         if (cur == NULL)
            cur=newNode;
         else
         {
            if (cur->getLeft() == NULL)
               cur->setLeft(newNode);
            else
               cur->setRight(newNode);
         }
      }
      //Operator.
      else
      {
         component=1;

         long opcode=getOperatorOpcode(token);
         if (opcode == BXSQuadOpcode::cERROR)
         {
            srcErrMsg("Error 0172: '%s' is not a valid operator.", getTokenizer()->getCurrentTokenText());
            BDELETE(root);
            return(NULL);
         }
         #ifdef DEBUGTRACEPARSE
         mMessenger->infoMsg("    Opcode=%s.", BXSQuadOpcode::getName(opcode));
         #endif

         //Create the opcode node.
         BXSParseTreeNode *newNode=new BXSParseTreeNode;
         if (newNode == NULL)
         {
            BDELETE(root);
            srcErrMsg("Error 0173: new PTNode allocation failed.");
            return(NULL);
         }
         newNode->setNodeType(BXSBinary::cOpcode);
         newNode->setValue(opcode);

         //If we don't have a node yet, this is it.
         if (cur == NULL)
            cur=newNode;
         //Check precedence if we're not locked.  If this new operator is higher
         //precedence than the current operator AND we have a right node, push
         //this new operator down to the right child of the current operator and
         //the previous right child down to the left child of this new operator.
         else if ((cur->getLocked() == false) && 
            (isHigherPrecedence(opcode, cur->getValue()) == true) &&
            (cur->getRight() != NULL))
         {
            newNode->setLeft(cur->getRight());
            cur->setRight(newNode);
            cur=newNode;
         }
         else
         {
            while ((isHigherPrecedence(cur->getValue(), newNode->getValue()) == true) && (cur->getParent() != NULL))
               cur=cur->getParent();
            //Relink the parent of this node (if it exists).
            if (cur->getParent() != NULL)
               cur->getParent()->setRight(newNode);
            //Set the current node as the left child of this node.
            newNode->setLeft(cur);
            //The new node is the new current node.
            cur=newNode;
         }
      }

      //Keep the root updated.
      if (cur != NULL)
         root=cur->getRoot();

      #ifdef DEBUGTRACEPARSE
      mMessenger->infoMsg("    Current Parse Tree:");
      root->print(mMessenger, true);
      mMessenger->infoMsg("    Root:");
      root->print(mMessenger, false);
      mMessenger->infoMsg("    Cur:");
      cur->print(mMessenger, false);
      mMessenger->infoMsg("");
      #endif

      //Get the next token.
      token=getTokenizer()->getDarkSpaceToken(this);

      //If we just had an operand, we must have a non-operand next.  Likewise, if we just had an
      //operator, we can't have one of those next.
      if (component == 0)
      {
         if ((token == BXSTokenizer::cInteger) ||
            (token == BXSTokenizer::cFloat) ||
            (token == BXSTokenizer::cBool) ||
            (token == BXSTokenizer::cString) ||
            (token == BXSTokenizer::cVector) ||
            (token == BXSTokenizer::cName))
         {
            BDELETE(root);
            srcErrMsg("Error 0300: invalid expression, '%s' cannot follow another operand.", getTokenizer()->getCurrentTokenText());
            return(NULL);
         }
      }
      else if (component == 1)
      {
         long opcode=getOperatorOpcode(token);
         if (opcode != BXSQuadOpcode::cERROR)
         {
            BDELETE(root);
            srcErrMsg("Error 0301: invalid expression, '%s' cannot follow another operator.", getTokenizer()->getCurrentTokenText());
            return(NULL);
         }
      }
   }

   //If we don't have a root node, bail.
   if (root == NULL)
   {
      srcErrMsg("Error 0324: invalid expression, likely is not enclosed by parentheses.");
      return(NULL);
   }

   //If we're not supposed to strip the stop token, decrement the current token.
   if (stripStopToken == false)
      getTokenizer()->decrementCurrentToken(this);

   //Lock this tree so that parent processing will override the default
   //precedence of operators.
   if (root != NULL)
      root->setLocked(true);

   //Validate the validity of this expression tree.
   long treeResultType=BXSVariableEntry::cInvalidVariable;
   if (root->validate(treeResultType) == false)
   {
      BDELETE(root);
      srcErrMsg("Error 0308: illogical or invalid expression.");
      return(NULL);
   }

   //If we're expecting a certain return type, make sure our expression result type either
   //matches or can be converted on-the-fly by the interpreter.
   if ((resultType != BXSVariableEntry::cInvalidVariable) && (resultType != treeResultType))
   {
      //Based on our resultType, decide what we can convert to that.
      switch (resultType)
      {
         //VOID: Nothing.
         case BXSVariableEntry::cVoidVariable:
            BDELETE(root);
            srcErrMsg("Error 0395: invalid expression; cannot convert anything to 'void' type.");
            return(NULL);
         //INT: Int, Float, Bool.
         case BXSVariableEntry::cIntegerVariable:
            if ((treeResultType != BXSVariableEntry::cIntegerVariable) &&
               (treeResultType != BXSVariableEntry::cFloatVariable) &&
               (treeResultType != BXSVariableEntry::cBoolVariable))
            {
               BDELETE(root);
               srcErrMsg("Error 0396: invalid expression; only ints, floats, and bools may be converted to 'int' type.");
               return(NULL);
            }
            break;
         //Float: Int, Float, Bool.
         case BXSVariableEntry::cFloatVariable:
            if ((treeResultType != BXSVariableEntry::cIntegerVariable) &&
               (treeResultType != BXSVariableEntry::cFloatVariable) &&
               (treeResultType != BXSVariableEntry::cBoolVariable))
            {
               BDELETE(root);
               srcErrMsg("Error 0397: invalid expression; only ints, floats, and bools may be converted to 'int' type.");
               return(NULL);
            }
            break;
         //Default.  Everything must match exactly.
         default:
            BDELETE(root);
            srcErrMsg("Error 0398: invalid expression; expression type does not match expected type.");
            return(NULL);
      }
      //Integers, Floats, and Bools can be converted between each other.  But nothing else.
      /*if ((resultType == BXSVariableEntry::cStringVariable) || (treeResultType == BXSVariableEntry::cStringVariable) ||
         (resultType == BXSVariableEntry::cVectorVariable) || (treeResultType == BXSVariableEntry::cVectorVariable))
      {
         BDELETE(root);
         srcErrMsg("Error 0309: expression type does not match expectation/requirement.");
         return(NULL);
      }*/
   }

   return(root);
}

//==============================================================================
// BXSCompiler::parseVariableTypeName
//==============================================================================
BYTE BXSCompiler::parseVariableTypeName(long token, bool voidAllowed)
{
   #ifdef DEBUGTRACEPARSE
   mMessenger->infoMsg("ParseVariableTypeName.");
   #endif

   //Return the appropriate variable type.
   switch (token)
   {
      case BXSTokenizer::cVoid:
         if (voidAllowed == true)
            return(BXSVariableEntry::cVoidVariable);
         break;
      case BXSTokenizer::cInteger:
         return(BXSVariableEntry::cIntegerVariable);
      case BXSTokenizer::cFloat:
         return(BXSVariableEntry::cFloatVariable);
      case BXSTokenizer::cBool:
         return(BXSVariableEntry::cBoolVariable);
      case BXSTokenizer::cString:
         return(BXSVariableEntry::cStringVariable);
      case BXSTokenizer::cVector:
         return(BXSVariableEntry::cVectorVariable);
   }

   //Failed.
   srcErrMsg("Error 0174: '%d' is not a valid variable type.", token);
   return(BXSVariableEntry::cInvalidVariable);
}

//==============================================================================
// BXSCompiler::parseIntegerConstant
//==============================================================================
bool BXSCompiler::parseIntegerConstant(long *value)
{
   #ifdef DEBUGTRACEPARSE
   mMessenger->infoMsg("ParseIntegerConstant.");
   #endif

   //Default.
   *value=0;

   //Get and parse the token.
   long token=getTokenizer()->getDarkSpaceToken(this);
   if (token != BXSTokenizer::cInteger)
   {
      //If the token isn't a name, forget it.
      if (token != BXSTokenizer::cName)
      {
         srcErrMsg("Error 0332: expected an integer constant and found '%s'.", getTokenizer()->getCurrentTokenText());
         return(false);
      }
      //Get the symbol entry for this token.
      const BXSSymbolEntry *se=getSymbolEntry(getTokenizer()->getCurrentTokenText(), true);
      if (se == NULL)
      {
         srcErrMsg("Error 0333: bad symbol in integer constant.");
         return(false);
      }
      //If it's not a variable, fail it.
      if ((se->mType != BXSBinary::cVariable) && (se->mType != BXSBinary::cStackVariable))
      {
         srcErrMsg("Error 0334: '%s' is not a valid variable for an integer constant.", getTokenizer()->getCurrentTokenText());
         return(false);
      }
      long variableType=-1;
      if (se->mType == BXSBinary::cVariable)
         variableType=mData->getVariableEntry(-se->mValue-1)->getType();
      else
         variableType=mData->getFunctionEntry(mCurrentFunctionID)->getVariable(se->mValue)->getType();
      //If this isn't an integer variable, fail.
      if (variableType != BXSVariableEntry::cIntegerVariable)
      {
         srcErrMsg("Error 0335: '%s' is not a const integer variable.", getTokenizer()->getCurrentTokenText());
         return(false);
      }
      //If this variable isn't const, fail.
      if ( ((se->mValue >= 0) && (mData->getFunctionEntry(mCurrentFunctionID)->getVariable(se->mValue)->getConst() == false)) ||
         ((se->mValue < 0) && (mData->getVariableEntry(-se->mValue-1)->getConst() == false)) )
      {
         srcErrMsg("Error 0336: '%s' is not a const variable.", getTokenizer()->getCurrentTokenText());
         return(false);
      }

      if (se->mValue >= 0)
         *value=*((long*)mData->getFunctionEntry(mCurrentFunctionID)->getVariable(se->mValue)->getData());
      else
         *value=*((long*)mData->getVariableEntry(-se->mValue-1)->getData());
      return(true);
   }

   if (getTokenizer()->getCurrentTokenText() != NULL)
      *value=atoi(getTokenizer()->getCurrentTokenText());
   else
      *value=0;
   return(true);
}

//==============================================================================
// BXSCompiler::parseFloatConstant
//==============================================================================
bool BXSCompiler::parseFloatConstant(float *value)
{
   #ifdef DEBUGTRACEPARSE
   mMessenger->infoMsg("ParseFloatConstant.");
   #endif

   //Default.
   *value=0.0f;

   //Get and parse the token.
   long token=getTokenizer()->getDarkSpaceToken(this);
   if ((token != BXSTokenizer::cFloat) && (token != BXSTokenizer::cInteger))
   {
      //If the token isn't a name, forget it.
      if (token != BXSTokenizer::cName)
      {
         srcErrMsg("Error 0337: expected a float constant and found '%s'.");
         return(false);
      }
      //Get the symbol entry for this token.
      const BXSSymbolEntry *se=getSymbolEntry(getTokenizer()->getCurrentTokenText(), true);
      if (se == NULL)
      {
         srcErrMsg("Error 0338: bad symbol in float constant.");
         return(false);
      }
      //If it's not a variable, fail it.
      if ((se->mType != BXSBinary::cVariable) && (se->mType != BXSBinary::cStackVariable))
      {
         srcErrMsg("Error 0339: '%s' is not a valid variable for a float constant.", getTokenizer()->getCurrentTokenText());
         return(false);
      }
      long variableType=-1;
      if (se->mType == BXSBinary::cVariable)
         variableType=mData->getVariableEntry(-se->mValue-1)->getType();
      else
         variableType=mData->getFunctionEntry(mCurrentFunctionID)->getVariable(se->mValue)->getType();
      //If this isn't an integer variable, fail.
      if (variableType != BXSVariableEntry::cFloatVariable)
      {
         srcErrMsg("Error 0340: '%s' is not a const float variable.", getTokenizer()->getCurrentTokenText());
         return(false);
      }
      //If this variable isn't const, fail.
      if ( ((se->mValue >= 0) && (mData->getFunctionEntry(mCurrentFunctionID)->getVariable(se->mValue)->getConst() == false)) ||
         ((se->mValue < 0) && (mData->getVariableEntry(-se->mValue-1)->getConst() == false)) )
      {
         srcErrMsg("Error 0341: '%s' is not a const variable.", getTokenizer()->getCurrentTokenText());
         return(false);
      }

      if (se->mValue >= 0)
         *value=*((float*)mData->getFunctionEntry(mCurrentFunctionID)->getVariable(se->mValue)->getData());
      else
         *value=*((float*)mData->getVariableEntry(-se->mValue-1)->getData());
      return(true);
   }

   if (getTokenizer()->getCurrentTokenText() != NULL)
      *value=(float)atof(getTokenizer()->getCurrentTokenText());
   else
      *value=0.0f;

   //If the next token is 'f', eat that too as it's just a floating point designator.
   token=getTokenizer()->getDarkSpaceToken(this);
   if ((token != BXSTokenizer::cName) || (stricmp(getTokenizer()->getCurrentTokenText(), "f") != 0))
      getTokenizer()->decrementCurrentToken(this);

   return(true);
}

//==============================================================================
// BXSCompiler::parseBoolConstant
//==============================================================================
bool BXSCompiler::parseBoolConstant(bool *value)
{
   #ifdef DEBUGTRACEPARSE
   mMessenger->infoMsg("ParseBoolConstant.");
   #endif

   //Default.
   *value=true;

   //Get and parse the token.
   long token=getTokenizer()->getDarkSpaceToken(this);
   if (token != BXSTokenizer::cBool)
   {
      //If the token isn't a name, forget it.
      if (token != BXSTokenizer::cName)
      {
         srcErrMsg("Error 0342: expected a bool constant and found '%s'.", getTokenizer()->getCurrentTokenText());
         return(false);
      }
      //Get the symbol entry for this token.
      const BXSSymbolEntry *se=getSymbolEntry(getTokenizer()->getCurrentTokenText(), true);
      if (se == NULL)
      {
         srcErrMsg("Error 0343: bad symbol in bool constant.");
         return(false);
      }
      //If it's not a variable, fail it.
      if ((se->mType != BXSBinary::cVariable) && (se->mType != BXSBinary::cStackVariable))
      {
         srcErrMsg("Error 0344: '%s' is not a valid variable for a bool constant.", getTokenizer()->getCurrentTokenText());
         return(false);
      }
      long variableType=-1;
      if (se->mType == BXSBinary::cVariable)
         variableType=mData->getVariableEntry(-se->mValue-1)->getType();
      else
      {
         BXSFunctionEntry *fe=mData->getFunctionEntry(mCurrentFunctionID);
         BASSERT(fe != NULL);
         variableType=fe->getVariable(se->mValue)->getType();
      }
      //If this isn't an integer variable, fail.
      if (variableType != BXSVariableEntry::cBoolVariable)
      {
         srcErrMsg("Error 0345: '%s' is not a const bool variable.", getTokenizer()->getCurrentTokenText());
         return(false);
      }
      //If this variable isn't const, fail.
      if ( ((se->mValue >= 0) && (mData->getFunctionEntry(mCurrentFunctionID)->getVariable(se->mValue)->getConst() == false)) ||
         ((se->mValue < 0) && (mData->getVariableEntry(-se->mValue-1)->getConst() == false)) )
      {
         srcErrMsg("Error 0346: '%s' is not a const variable.", getTokenizer()->getCurrentTokenText());
         return(false);
      }

      if (se->mValue >= 0)
         *value=*((bool*)mData->getFunctionEntry(mCurrentFunctionID)->getVariable(se->mValue)->getData());
      else
         *value=*((bool*)mData->getVariableEntry(-se->mValue-1)->getData());
      return(true);
   }

   if (getTokenizer()->getCurrentTokenText() == NULL)
      *value=false;
   else if (stricmp(getTokenizer()->getCurrentTokenText(), "true") == 0)
      *value=true;
   else if (stricmp(getTokenizer()->getCurrentTokenText(), "false") == 0)
      *value=false;
   else
   {
      srcErrMsg("Error 0178: expected a bool constant and found '%s'.");
      return(false);
   }

   return(true);
}

//==============================================================================
// BXSCompiler::parseVectorConstant
//==============================================================================
bool BXSCompiler::parseVectorConstant(BVector *value)
{
   #ifdef DEBUGTRACEPARSE
   mMessenger->infoMsg("ParseVectorConstant.");
   #endif

   //Get and parse the token.  Don't increment it.
   long token=getTokenizer()->getCurrentToken();
   if (token != BXSTokenizer::cVector)
   {
      //If the token isn't a name, forget it.
      if (token != BXSTokenizer::cName)
      {
         srcErrMsg("Error 0369: expected a vector constant and found '%s'.", getTokenizer()->getCurrentTokenText());
         return(false);
      }
      //Get the symbol entry for this token.
      const BXSSymbolEntry *se=getSymbolEntry(getTokenizer()->getCurrentTokenText(), true);
      if (se == NULL)
      {
         srcErrMsg("Error 0370: bad symbol in vector constant.");
         return(false);
      }
      //If it's not a variable, fail it.
      if ((se->mType != BXSBinary::cVariable) && (se->mType != BXSBinary::cStackVariable))
      {
         srcErrMsg("Error 0371: '%s' is not a valid variable for a bool constant.", getTokenizer()->getCurrentTokenText());
         return(false);
      }
      long variableType=-1;
      if (se->mType == BXSBinary::cVariable)
         variableType=mData->getVariableEntry(-se->mValue-1)->getType();
      else
      {
         BXSFunctionEntry *fe=mData->getFunctionEntry(mCurrentFunctionID);
         BASSERT(fe != NULL);
         variableType=fe->getVariable(se->mValue)->getType();
      }
      //If this isn't a vector variable, fail.
      if (variableType != BXSVariableEntry::cVectorVariable)
      {
         srcErrMsg("Error 0372: '%s' is not a vector variable.", getTokenizer()->getCurrentTokenText());
         return(false);
      }
      //If this variable isn't const, fail.
      if ( ((se->mValue >= 0) && (mData->getFunctionEntry(mCurrentFunctionID)->getVariable(se->mValue)->getConst() == false)) ||
         ((se->mValue < 0) && (mData->getVariableEntry(-se->mValue-1)->getConst() == false)) )
      {
         srcErrMsg("Error 0373: '%s' is not a const variable.", getTokenizer()->getCurrentTokenText());
         return(false);
      }

      if (se->mValue >= 0)
         *value=*((BVector*)mData->getFunctionEntry(mCurrentFunctionID)->getVariable(se->mValue)->getData());
      else
         *value=*((BVector*)mData->getVariableEntry(-se->mValue-1)->getData());
      return(true);
   }

   //Default.
   value->x=0.0f;
   value->y=0.0f;
   value->z=0.0f;

   //'('.
   token=getTokenizer()->getDarkSpaceToken(this);
   if (token != BXSTokenizer::cLeftParen)
   {
      srcErrMsg("Error 0179: expected '(' and found '%s'.", getTokenizer()->getCurrentTokenText());
      return(false);
   }

   //X.
   float x;
   if (parseFloatConstant(&x) == false)
   {
      srcErrMsg("Error 0180: expected float x value and found '%s'.", getTokenizer()->getCurrentTokenText());
      return(false);
   }

   //','.
   token=getTokenizer()->getDarkSpaceToken(this);
   if (token != BXSTokenizer::cComma)
   {
      srcErrMsg("Error 0181: expected ',' and found '%s'.", getTokenizer()->getCurrentTokenText());
      return(false);
   }

   //Y.
   float y;
   if (parseFloatConstant(&y) == false)
   {
      srcErrMsg("Error 0182: expected float y value and found '%s'.", getTokenizer()->getCurrentTokenText());
      return(false);
   }

   //','.
   token=getTokenizer()->getDarkSpaceToken(this);
   if (token != BXSTokenizer::cComma)
   {
      srcErrMsg("Error 0183: expected ',' and found '%s'.", getTokenizer()->getCurrentTokenText());
      return(false);
   }

   //Z.
   float z;
   if (parseFloatConstant(&z) == false)
   {
      srcErrMsg("Error 0184: expected float z value and found '%s'.", getTokenizer()->getCurrentTokenText());
      return(false);
   }

   //')'.
   token=getTokenizer()->getDarkSpaceToken(this);
   if (token != BXSTokenizer::cRightParen)
   {
      srcErrMsg("Error 0185: expected '(' and found '%s'.", getTokenizer()->getCurrentTokenText());
      return(false);
   }

   //Assign it all.
   value->x=x;
   value->y=y;
   value->z=z;

   return(true);
}

//==============================================================================
// BXSCompiler::parseVector
//==============================================================================
bool BXSCompiler::parseVector(BVector *value)
{
   //TODO.  This is going to require a mucky vector temp variable and some ugly
   //parsing code.
   value;


   #ifdef DEBUGTRACEPARSE
   mMessenger->infoMsg("ParseVector.");
   #endif
   return(false);

   //Default.
   /*value->x=0.0f;
   value->y=0.0f;
   value->z=0.0f;

   //'('.
   long token=getTokenizer()->getDarkSpaceToken(this);
   if (token != BXSTokenizer::cLeftParen)
   {
      srcErrMsg("Error 0XXX: expected '(' and found '%s'.");
      return(false);
   }

   //X.
   float x;
   if (parseFloatConstant(&x) == false)
   {
      srcErrMsg("Error 0XXX: expected float value/variable and found '%s'.");
      return(false);
   }

   //','.
   token=getTokenizer()->getDarkSpaceToken(this);
   if (token != BXSTokenizer::cComma)
   {
      srcErrMsg("Error 0XXX: expected ',' and found '%s'.");
      return(false);
   }

   //Y.
   float y;
   if (parseFloatConstant(&y) == false)
   {
      srcErrMsg("Error 0XXX: expected float value/variable and found '%s'.");
      return(false);
   }

   //','.
   token=getTokenizer()->getDarkSpaceToken(this);
   if (token != BXSTokenizer::cComma)
   {
      srcErrMsg("Error 0XXX: expected ',' and found '%s'.");
      return(false);
   }

   //Z.
   float z;
   if (parseFloatConstant(&z) == false)
   {
      srcErrMsg("Error 0XXX: expected float value/variable and found '%s'.");
      return(false);
   }

   //')'.
   token=getTokenizer()->getDarkSpaceToken(this);
   if (token != BXSTokenizer::cRightParen)
   {
      srcErrMsg("Error 0XXX: expected '(' and found '%s'.");
      return(false);
   }

   //Assign it all.
   value->x=x;
   value->y=y;
   value->z=z;

   return(true);*/
}

//==============================================================================
// BXSCompiler::addLabelQuad
//==============================================================================
bool BXSCompiler::addLabelQuad(long labelID, bool pushLabelUp)
{
   //Bomb check.
   if (labelID < 0)
      return(false);

   //Add the quad at the end.
   long oldNumber=mQuads.getNumber();
   if (mQuads.add(BXSQuad(BXSQuadOpcode::cLABEL, labelID)) == -1)
      return(false);

   //If we're not pushing the label up or we don't have any other quads or the
   //last quad in the list isn't a LINE, just return now.
   if ((pushLabelUp == false) || (oldNumber <= 0) || (mQuads[oldNumber-1].mOpcode != BXSQuadOpcode::cLINE))
      return(true);

   //Now, we swap the label quad up past any line quads.
   long index=oldNumber-1;
   while ((index > 0) && (mQuads[index].mOpcode == BXSQuadOpcode::cLINE))
   {
      mQuads[index+1]=mQuads[index];
      mQuads[index]=BXSQuad(BXSQuadOpcode::cLABEL, labelID);
      index--;
   }

   return(true);
}

//==============================================================================
// BXSCompiler::fixupLabelsAndOverloads
//==============================================================================
bool BXSCompiler::fixupLabelsAndOverloads(void)
{
   if (mData == NULL)
      return(false);

   long codeSize=mSource->getCodeSize();
   SBYTE *code=mSource->getCode();

   //Run through the code and fixup any jumps.
   for (long p=0; p < codeSize; )
   {
      //Get the opcode.
      SBYTE opcode=code[p];
      //Get the amount of data to skip for this opcode.
      long opcodeIncrement=mEmitter->getNumberBytesEmitted(opcode);

      //If this is a JUMP, get the label ID for it and overwrite that with the actual
      //position from the label jump table.  The jump position will be the four bytes
      //after the opcode.
      if (opcode == BXSQuadOpcode::cJUMP)
      {
         //Skip the JUMP opcode and the 1 byte check infinite loop tag.
         p+=2;
         opcodeIncrement-=2;
      }
      //If this is an JUMPZ, we need to fixup it's else jump.  The jump position will
      //be the 4 bytes after the 1 byte opcode and the 1 byte pop stack tag.
      else if ((opcode == BXSQuadOpcode::cJUMPZ) || (opcode == BXSQuadOpcode::cJUMPNZ))
      {
         //Skip the JUMPZ/JUMPNZ opcode and the 1 byte pop stack tag.
         p+=2;
         opcodeIncrement-=2;
      }
      //If we have a CALLF, check the function overload list to see if we need to
      //patch in a new FID.
      else if (opcode == BXSQuadOpcode::cCALLF)
      {
         //Skip the opcode.
         p++;
         opcodeIncrement--;

         //Grab the FID.
         short shortFID=*((short *)(code+p));
         long fid=(long)shortFID;
         //Make sure we got a valid functionID out of this.
         if ((fid < 0) || (fid >= mData->getNumberFunctions()) )
         {
            srcErrMsg("Error 0362: %d is an invalid FID during overload fixup.", fid);
            return(false);
         }

         //See if we have a mapping for this FID to a newer FID.  If so, slip it in.
         for (long i=0; i < mFunctionOverloads.getNumber(); i++)
         {
            //If we have a match, pop the new one in and break out.
            if (fid == mFunctionOverloads[i].mOldFunctionID)
            {
               shortFID=(short)mFunctionOverloads[i].mNewFunctionID;
               memcpy(code+p, &shortFID, sizeof(short));
               break;
            }
         }
         p+=opcodeIncrement;
         continue;
      }
      else
      {
         //FIXME XBOX - ajl 6/29/05 Added below check because opcodeIncrement was 0 causing in an endless loop. opcode was  BXSQuadOpcode::cNOP
         if(opcodeIncrement==0)
         {
            p++;
            continue;
         }
         p+=opcodeIncrement;
         continue;
      }

      //Get the jump label ID out of the code.
      long labelID=-1;
      memcpy(&labelID, code+p, sizeof(long));
      if ((labelID < 0) || (labelID >= mLabels.getNumber()) )
      {
         srcErrMsg("Error 0186: could not find a fixup index for jump at position %d.", p);
         return(false);
      }

      //If it's a valid ID, slip the position in the code instead.
      long labelPosition=mLabels[labelID].mPosition;
      memcpy(code+p, &labelPosition, sizeof(long));

      p+=opcodeIncrement;
   }

   return(true);
}

//==============================================================================
// BXSCompiler::createLabel
//==============================================================================
long BXSCompiler::createLabel(char *name, bool useScope)
{
   //Create a unique label name.
   char labelName[(BXSTokenizer::cMaxTokenSize+1)*2];
   if (name != NULL)
   {
      if (useScope == true)
         bsnprintf(labelName, sizeof(labelName), "%s%s", mCurrentScope, name);
      else
         StringCchCopyA(labelName, (BXSTokenizer::cMaxTokenSize+1)*2, name);
   }
   else
   {
      if (useScope == true)
         bsnprintf(labelName, sizeof(labelName), "%sL%d", mCurrentScope, mLabels.getNumber());
      else
         bsnprintf(labelName, sizeof(labelName), "L%d", mLabels.getNumber());
   }

   //These are supposed to be unique.
   const BXSSymbolEntry *se=mSource->getSymbols().getEntryBySymbol(labelName);
   if (se != NULL)
   {
      srcErrMsg("Error 0187: could not create label name.");
      return(-1);
   }

   //Add the label.
   long newLabelID=mLabels.getNumber();
   if (mLabels.setNumber(newLabelID+1) == false)
   {
      srcErrMsg("Error 0188: could not add jump label.");
      return(-1);
   }
   //Add it to the symbol table.  The symbol type is cLabel.  The ID of label is
   //the value.  addSymbol returns the unique ID for this label, so we stuff that
   //into the ID slot in the label table so that we can reference it later.
   long symbolID=mSource->getSymbols().addSymbol(labelName, BXSBinary::cLabel, newLabelID);
   if (symbolID < 0)
   {
      srcErrMsg("Error 0189: unable to add an entry to the symbol table for label.");
      return(-1);
   }
   //Set the values.
   mLabels[newLabelID].mSymbolID=symbolID;
   mLabels[newLabelID].mPosition=-1;

   //Return the ID of the label.
   return(newLabelID);
}

//==============================================================================
// BXSCompiler::addFunctionOverload
//==============================================================================
bool BXSCompiler::addFunctionOverload(long oldFID, long newFID)
{
   //If we have a newFID to overload an oldFID that we've already overloaded,
   //update that.
   bool foundOld=false;
   bool foundNew=false;
   for (long i=0; i < mFunctionOverloads.getNumber(); i++)
   {
      if (mFunctionOverloads[i].mOldFunctionID == oldFID)
      {
         mFunctionOverloads[i].mNewFunctionID=newFID;
         foundOld=true;
      }
      else if (mFunctionOverloads[i].mNewFunctionID == oldFID)
      {
         mFunctionOverloads[i].mNewFunctionID=newFID;
         foundNew=true;
      }
   }
   //If we found some old FIDs to update and didn't find any new FIDs, return.
   if ((foundOld == true) && (foundNew == false))
      return(true);

   //If we're here, add a new mapping.
   long n=mFunctionOverloads.getNumber();
   if (mFunctionOverloads.setNumber(n+1) == false)
   {
      srcErrMsg("Error 0364: unable to add a function overload entry.");
      return(false);
   }
   mFunctionOverloads[n].mOldFunctionID=oldFID;
   mFunctionOverloads[n].mNewFunctionID=newFID;
   return(true);
}

//==============================================================================
// BXSCompiler::createTemporaryVariable
//==============================================================================
long BXSCompiler::createTemporaryVariable(long variableType, void *data)
{
   //If we're not in a function, this fails.
   if (mCurrentFunctionID < 0)
   {
      srcErrMsg("Error 0190: currentFunction is invalid.");
      return(-1);
   }

   //Allocate the new variable entry.
   BXSVariableEntry *newVE=new BXSVariableEntry();
   if (newVE == NULL)
   {
      srcErrMsg("Error 0191: new variable entry allocation failed.");
      return(-1);
   }

   //Create a magic, unique symbol name.
   long newVariableID=mData->getFunctionEntry(mCurrentFunctionID)->getNumberVariables();
   char variableName[(BXSTokenizer::cMaxTokenSize+1)*2];
   bsnprintf(variableName, sizeof(variableName), "%sTEMPVAR%d", mCurrentScope, newVariableID);

   //Add it to the symbol table.  The symbol type is cStackVariable.  The ID of variable is
   //the value.  addSymbol returns the unique ID for this variable, so we stuff that
   //into the ID slot in the variable table so that we can reference it later.
   long symbolID=mSource->getSymbols().addSymbol(variableName, BXSBinary::cStackVariable, newVariableID);
   if (symbolID < 0)
   {
      srcErrMsg("Error 0192: unable to add an entry for temporary variable.");
      return(-1);
   }

   //Set the values.
   if (newVE->setAll(symbolID, variableType, data) == false)
   {
      srcErrMsg("Error 0193: setAll failed.");
      return(-1);
   }
   //Make it temporary.
   newVE->setTemp(true);
   //Add it to the function.
   mData->getFunctionEntry(mCurrentFunctionID)->addVariable(newVE);

   return(newVariableID);
}

//==============================================================================
// BXSCompiler::createIncrementVariableCode
//==============================================================================
bool BXSCompiler::createIncrementVariableCode(long variableIndex, long incrementValue)
{
   //If this variable is const, fail.
   if (variableIndex >= 0)
   {
      BXSFunctionEntry *fe=mData->getFunctionEntry(mCurrentFunctionID);
      BASSERT(fe != NULL);
      BASSERT(fe->getVariable(variableIndex) != NULL);
      if (fe->getVariable(variableIndex)->getConst() == true)
      {
         srcErrMsg("Error 0194: cannot assign to a const variable.");
         return(false);
      }
   }
   else
   {
      if (mData->getVariableEntry(-variableIndex-1)->getConst() == true)
      {
         srcErrMsg("Error 0195: cannot assign to a const variable.");
         return(false);
      }
   }

   //Add the push address opcode.
   if (mQuads.add(BXSQuad(BXSQuadOpcode::cPUSHADD, variableIndex, 0)) == -1)
   {
      srcErrMsg("Error 0196: PUSHADD quad addition failed.");
      return(false);
   }

   //Push a copy of the variable's old value.
   if (mQuads.add(BXSQuad(BXSQuadOpcode::cPUSH, variableIndex, 0)) == -1)
   {
      srcErrMsg("Error 0197: PUSH quad addition failed.");
      return(false);
   }

   //Push the increment value onto the stack.
   if (mQuads.add(BXSQuad(BXSQuadOpcode::cPUSHI, BXSVariableEntry::cIntegerVariable, incrementValue)) == -1)
   {
      srcErrMsg("Error 0198: PUSHI quad addition failed.");
      return(false);
   }

   //Add the ADD.
   if (mQuads.add(BXSQuad(BXSQuadOpcode::cADD)) == -1)
   {
      srcErrMsg("Error 0199: ADD quad addition failed.");
      return(false);
   }
   //Add the ASS.
   if (mQuads.add(BXSQuad(BXSQuadOpcode::cASS)) == -1)
   {
      srcErrMsg("Error 0200: ASS quad addition failed.");
      return(false);
   }

   //Pop the result value.
   if (mQuads.add(BXSQuad(BXSQuadOpcode::cPOP, 1)) == -1)
   {
      srcErrMsg("Error 0201: POP quad addition failed.");
      return(false);
   }
   //Pop the address of the variable that we assigned into.
   if (mQuads.add(BXSQuad(BXSQuadOpcode::cPOPADD, 1)) == -1)
   {
      srcErrMsg("Error 0202: POPADD quad addition failed.");
      return(false);
   }

   return(true);
}

//==============================================================================
// BXSCompiler::createVariableAssignCode
//==============================================================================
bool BXSCompiler::createVariableAssignCode(long lValueIndex, long rValueIndex)
{
   //If this variable is const, fail.
   if (lValueIndex >= 0)
   {
      BXSFunctionEntry *fe=mData->getFunctionEntry(mCurrentFunctionID);
      BASSERT(fe != NULL);
      BASSERT(fe->getVariable(lValueIndex) != NULL);
      if (fe->getVariable(lValueIndex)->getConst() == true)
      {
         srcErrMsg("Error 0203: cannot assign to a const variable.");
         return(false);
      }
   }
   else
   {
      if (mData->getVariableEntry(-lValueIndex-1)->getConst() == true)
      {
         srcErrMsg("Error 0204: cannot assign to a const variable.");
         return(false);
      }
   }

   //Push the index variable address.
   if (mQuads.add(BXSQuad(BXSQuadOpcode::cPUSHADD, lValueIndex, 0)) == -1)
   {
      srcErrMsg("Error 0205: PUSHADD quad addition failed.");
      return(false);
   }
   //Push the initial value.
   if (mQuads.add(BXSQuad(BXSQuadOpcode::cPUSH, rValueIndex, 0)) == -1)
   {
      srcErrMsg("Error 0206: PUSH quad addition failed.");
      return(false);
   }
   //Do the assignment.
   if (mQuads.add(BXSQuad(BXSQuadOpcode::cASS)) == -1)
   {
      srcErrMsg("Error 0207: ASS quad addition failed.");
      return(false);
   }
   //Pop the initial value.
   if (mQuads.add(BXSQuad(BXSQuadOpcode::cPOP, 1)) == -1)
   {
      srcErrMsg("Error 0208: POP quad addition failed.");
      return(false);
   }
   //Pop the address of the variable that we assigned into.
   if (mQuads.add(BXSQuad(BXSQuadOpcode::cPOPADD, 1)) == -1)
   {
      srcErrMsg("Error 0209: POPADD quad addition failed.");
      return(false);
   }

   return(true);
}

//==============================================================================
// BXSCompiler::createVariableAssignCodeImmediate
//==============================================================================
bool BXSCompiler::createVariableAssignCodeImmediate(long lValueIndex, long type, long value)
{
   //If this variable is const, fail.
   if (lValueIndex >= 0)
   {
      BXSFunctionEntry *fe=mData->getFunctionEntry(mCurrentFunctionID);
      BASSERT(fe != NULL);
      BASSERT(fe->getVariable(lValueIndex) != NULL);
      if (fe->getVariable(lValueIndex)->getConst() == true)
      {
         srcErrMsg("Error 0210: cannot assign to a const variable.");
         return(false);
      }
   }
   else
   {
      if (mData->getVariableEntry(-lValueIndex-1)->getConst() == true)
      {
         srcErrMsg("Error 0211: cannot assign to a const variable.");
         return(false);
      }
   }

   //Push the index variable address.
   if (mQuads.add(BXSQuad(BXSQuadOpcode::cPUSHADD, lValueIndex, 0)) == -1)
   {
      srcErrMsg("Error 0212: PUSHADD quad addition failed.");
      return(false);
   }
   //Push the initial value.
   if (mQuads.add(BXSQuad(BXSQuadOpcode::cPUSHI, type, value)) == -1)
   {
      srcErrMsg("Error 0213: PUSHI quad addition failed.");
      return(false);
   }
   //Do the assignment.
   if (mQuads.add(BXSQuad(BXSQuadOpcode::cASS)) == -1)
   {
      srcErrMsg("Error 0214: ASS quad addition failed.");
      return(false);
   }
   //Pop the initial value.
   if (mQuads.add(BXSQuad(BXSQuadOpcode::cPOP, 1)) == -1)
   {
      srcErrMsg("Error 0215: POP quad addition failed.");
      return(false);
   }
   //Pop the address of the variable that we assigned into.
   if (mQuads.add(BXSQuad(BXSQuadOpcode::cPOPADD, 1)) == -1)
   {
      srcErrMsg("Error 0216: POPADD quad addition failed.");
      return(false);
   }

   return(true);
}

//==============================================================================
// BXSCompiler::createRuleGroup
//==============================================================================
long BXSCompiler::createRuleGroup(char *name)
{
   if (name == NULL)
   {
      srcErrMsg("Error 0276: expected group name and found '%s'.", getTokenizer()->getCurrentTokenText());
      return(-1);
   }

   //Allocate the new rule group entry.
   BXSRuleGroupEntry *newRGE=mData->allocateRuleGroupEntry();
   if (newRGE == NULL)
   {
      srcErrMsg("Error 0277: failed to create rule group entry.");
      return(false);
   }

   //Add the name as a symbol.
   long symbolID=mSource->getSymbols().addSymbol(name, BXSBinary::cRuleGroup, newRGE->getID());
   if (symbolID < 0)
   {
      srcErrMsg("Error 0279: unable to add an entry to the file symbol table for this global variable.");
      return(false);
   }

   //Set the symbol ID now that we have it.
   newRGE->setSymbolID(symbolID);

   return(newRGE->getID());
}

//==============================================================================
// BXSCompiler::getDefaultLongVariableValue
//==============================================================================
long BXSCompiler::getDefaultLongVariableValue(long variableID)
{
   if (variableID >= 0)
   {
      if (mCurrentFunctionID < 0)
         return(-1);
      if (mData->getFunctionEntry(mCurrentFunctionID)->getNumberVariables() <= variableID)
         return(-1);
   
      BXSFunctionEntry *fe=mData->getFunctionEntry(mCurrentFunctionID);
      if (fe == NULL)
         return(-1);
      BXSVariableEntry *ve=fe->getVariable(variableID);
      if (ve == NULL)
         return(-1);
      long rVal=*((long*)ve->getData());
      return(rVal);
   }

   long globalVariableID=-variableID-1;
   if ((globalVariableID < 0) || (mData->getNumberVariables() <= globalVariableID))
      return(-1);

   long rVal=*((long*)mData->getVariableEntry(globalVariableID)->getData());
   return(rVal);
}

//==============================================================================
// BXSCompiler::getSymbolEntry
//==============================================================================
const BXSSymbolEntry* BXSCompiler::getSymbolEntry(char *name, bool useGlobalScope)
{
   if (name == NULL)
      return(NULL);

   //Grab the symbol entry.
   const BXSSymbolEntry *se=mSource->getSymbols().getEntryBySymbol(name);
   if (se == NULL)
   {
      //Try to get the name with scope.
      char scopedName[(BXSTokenizer::cMaxTokenSize+1)*2];
      //Try this scope.
      bsnprintf(scopedName, sizeof(scopedName), "%s%s", mCurrentScope, name);
      se=mSource->getSymbols().getEntryBySymbol(scopedName);
      //Try file global scope.
      if ((se == NULL) && (useGlobalScope == true))
      {
         bsnprintf(scopedName, sizeof(scopedName), "%s$%s", getTokenizer()->getFilenameWithoutExtension().getPtr(), name);
         se=mSource->getSymbols().getEntryBySymbol(scopedName);

         //If it's still NULL, try EXTERN scope.
         if (se == NULL)
         {
            bsnprintf(scopedName, sizeof(scopedName), "EXTERN$%s", name);
            se=mSource->getSymbols().getEntryBySymbol(scopedName);
         }
      }
   }

   return(se);
}

//==============================================================================
// BXSCompiler::getOperatorOpcode
//==============================================================================
long BXSCompiler::getOperatorOpcode(long token)
{
   switch (token)
   {
      case BXSTokenizer::cAdd:
         return(BXSQuadOpcode::cADD);
      case BXSTokenizer::cSub:
         return(BXSQuadOpcode::cSUB);
      case BXSTokenizer::cMult:
         return(BXSQuadOpcode::cMUL);
      case BXSTokenizer::cDiv:
         return(BXSQuadOpcode::cDIV);
      case BXSTokenizer::cMod:
         return(BXSQuadOpcode::cMOD);
      case BXSTokenizer::cAssign:
         return(BXSQuadOpcode::cASS);
      case BXSTokenizer::cLT:
         return(BXSQuadOpcode::cLT);
      case BXSTokenizer::cLE:
         return(BXSQuadOpcode::cLE);
      case BXSTokenizer::cGT:
         return(BXSQuadOpcode::cGT);
      case BXSTokenizer::cGE:
         return(BXSQuadOpcode::cGE);
      case BXSTokenizer::cEQ:
         return(BXSQuadOpcode::cEQ);
      case BXSTokenizer::cNE:
         return(BXSQuadOpcode::cNE);
      case BXSTokenizer::cAnd:
         return(BXSQuadOpcode::cAND);
      case BXSTokenizer::cOr:
         return(BXSQuadOpcode::cOR);
   }

   return(BXSQuadOpcode::cERROR);
}

//==============================================================================
// BXSCompiler::isHigherPrecedence
//==============================================================================
bool BXSCompiler::isHigherPrecedence(long opcode1, long opcode2)
{
   //Returns true if opcode1 is higher precedence than opcode2.
   long p1=BXSQuadOpcode::getPrecedence(opcode1);
   long p2=BXSQuadOpcode::getPrecedence(opcode2);
   if (p1 > p2)
      return(true);

   return(false);
}

//==============================================================================
// BXSCompiler::isLowerPrecedence
//==============================================================================
bool BXSCompiler::isLowerPrecedence(long opcode1, long opcode2)
{
   //Returns true if opcode1 is lower precedence than opcode2.
   long p1=BXSQuadOpcode::getPrecedence(opcode1);
   long p2=BXSQuadOpcode::getPrecedence(opcode2);
   if (p1 < p2)
      return(true);

   return(false);
}

//==============================================================================
// BXSCompiler::getCurrentFunctionEntry
//==============================================================================
BXSFunctionEntry *BXSCompiler::getCurrentFunctionEntry(void)
{
   return(mData->getFunctionEntry(mCurrentFunctionID));
}

//==============================================================================
// BXSCompiler::srcErrMsg
//==============================================================================
void BXSCompiler::srcErrMsg(const char *message, long arg1)
{
   mMessenger->sourceErrorMsg(getTokenizer()->getFilename().getPtr(), getTokenizer()->getCurrentTokenLineNumber(), message, arg1);
}

//==============================================================================
// BXSCompiler::srcErrMsg
//==============================================================================
void BXSCompiler::srcErrMsg(const char *message, const char *arg1)
{
   mMessenger->sourceErrorMsg(getTokenizer()->getFilename().getPtr(), getTokenizer()->getCurrentTokenLineNumber(), message, arg1);
}

//==============================================================================
// BXSCompiler::srcErrMsg
//==============================================================================
void BXSCompiler::srcErrMsg(const char *message, long arg1, const char *arg2)
{
   mMessenger->sourceErrorMsg(getTokenizer()->getFilename().getPtr(), getTokenizer()->getCurrentTokenLineNumber(), message, arg1, arg2);
}

//==============================================================================
// BXSCompiler::srcErrMsg
//==============================================================================
void BXSCompiler::srcErrMsg(const char *message, const char *arg1, const char *arg2)
{
   mMessenger->sourceErrorMsg(getTokenizer()->getFilename().getPtr(), getTokenizer()->getCurrentTokenLineNumber(), message, arg1, arg2);
}

//==============================================================================
// BXSCompiler::srcErrMsg
//==============================================================================
void BXSCompiler::srcErrMsg(const char *message)
{
   mMessenger->sourceErrorMsg(getTokenizer()->getFilename().getPtr(), 
                              getTokenizer()->getCurrentTokenLineNumber(),
                              message);
}

//==============================================================================
// BXSCompiler::emitTheLists
//==============================================================================
bool BXSCompiler::emitTheLists(BXSQuadArray &quads, long *offset)
{
   //Emit the quads unless there were errors.
   #ifndef DEBUGEMITLISTSONERROR
   if (mMessenger->getErrorCount() > 0)
   {
      *offset=-1;
      return(false);
   }
   #endif

   if (mEmitter->emitQuads(quads, mSource, offset, this) == false)
   {
      mMessenger->errorMsg("Error 0217: problem emitting the list.");
      return(false);
   } 
   return(true);
}

//==============================================================================
// BXSCompiler::cleanUp
//==============================================================================
void BXSCompiler::cleanUp(void)
{
   //We do not cleanup mMessenger because that's handed to us.
   //We do not cleanup mData because that's handed to us.

   //Close the list file if we have one open.
   if (mListFile != NULL)
   {
      if (mEmitter != NULL)
         mEmitter->setListFile(NULL);
      mListFile->close();
      delete mListFile;
      mListFile=NULL;
   }

   //Delete the tokenizers.
   BDELETE(mBaseTokenizer);
   for (long i=0; i < mTokenizers.getNumber(); i++)
      BDELETE(mTokenizers[i]);
   mTokenizers.setNumber(0);
   for (long i=0; i < mUnusedTokenizers.getNumber(); i++)
      BDELETE(mUnusedTokenizers[i]);
   mUnusedTokenizers.setNumber(0);

   BDELETE(mEmitter)
   //BDELETE(mOptimizer)

   //DO NOT DELETE mSyscalls.  We don't own that memory.
   //DO NOT DELETE mData.  We don't own that memory.
   //DO NOT DELETE mMessenger.  We don't own that memory.
}
