//==============================================================================
// console.cpp
//
// Copyright (c) 2005 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "console.h"
#include "config.h"
#include "configsinput.h"
#include "xscompiler.h"
#include "xsdata.h"
#include "xsinterpreter.h"
#include "xsruntime.h"
#include "xssource.h"
#include "workdirsetup.h"
#include "debugConnection.h"

BConsole gConsole;
#ifndef BUILD_FINAL
   BStoredConsoleOutput gStoredConsoleOutput;
#endif

//==============================================================================
// BConsole::BConsole
//==============================================================================
BConsole::BConsole(void) :
   mExecuting(false),
   mNextFrameCommand(),
   mDelayedFile(),
   mDelayedCommand(),
   //mTempText(),
   //mTabBufferPosition(-1),
   //mTabBuffer doesn't need any ctor args.
   mCurrentFile(),
   mConsoleMessenger(NULL),
   mConsoleCompiler(NULL),
   mConsoleData(NULL),
   mConsoleSyscalls(NULL),
   mUnicodeConsoleParamKey(0),
   mpOutputTarget(NULL),
   mEventHandle(cInvalidEventReceiverHandle),
   mChannelEnabledMask(UINT64_MAX)
{
}

//==============================================================================
// BConsole::~BConsole
//==============================================================================
BConsole::~BConsole()
{
}

//==============================================================================
// BConsole::setup
//==============================================================================
bool BConsole::setup(REGISTER_CONSOLE_FUNCS_CALLBACK registerFuncs)
{
   mEventHandle = gEventDispatcher.addClient(this, cThreadIndexSim);
   
   //Set up the console compiler.
   if (!initializeConsoleCompiler(registerFuncs))
      return(false);
      
   
   #ifndef BUILD_FINAL
      gConsoleOutput.init(outputCallback, this);

      long perCategoryStringLimit = -1;
      gConfig.get(cConfigConsoleStringLimit, &perCategoryStringLimit);
      gStoredConsoleOutput.init(perCategoryStringLimit);
   #endif

   //setTraceCallback(traceCallback);

   trace("BConsole::setup");

   createConsoleChannels();
      
   return(true);
}

//==============================================================================
// BConsole::createConsoleChannels
//==============================================================================
void BConsole::createConsoleChannels()
{
   //-- Create our console channels
   mConsoleChannels[cMsgConsole]       = "console";
   mConsoleChannels[cMsgDebug]         = "debug";
   mConsoleChannels[cMsgStatus]        = "status";
   mConsoleChannels[cMsgWarning]       = "warning";
   mConsoleChannels[cMsgError]         = "error";
   mConsoleChannels[cMsgResource]      = "resource";
   mConsoleChannels[cMsgFileManager]   = "fileManager";
   //-- 360 Channels
   mConsoleChannels[cChannelCombat]    = "combat";
   mConsoleChannels[cChannelTriggers]  = "triggers";
   mConsoleChannels[cChannelAI]        = "ai";
   mConsoleChannels[cChannelSim]       = "sim";
   mConsoleChannels[cChannelUI]        = "ui";
   
   //-- Add 360 Channels
   for(long i = cMsgMax; i < cChannelMax; i++)
   {
      addChannel(mConsoleChannels[i]);
   }

   // Send these console channel strings to the renderer for later display
   #ifndef BUILD_FINAL
      sendCategoryHeadersForRenderStorage();
   #endif
}

//==============================================================================
// BConsole::traceCallback
//==============================================================================
void BConsole::traceCallback(const char* pMsg)
{
   BDEBUG_ASSERT(pMsg);
   gConsoleOutput.output(cMsgDebug, "%s", pMsg);
}

//==============================================================================
// BConsole::outputCallback
//==============================================================================
void BConsole::outputCallback(void* data, BConsoleMessageCategory category, const char* pMsg)
{
   static_cast<BConsole*>(data)->output(category, "%s", pMsg);
}

//==============================================================================
// BConsole::update
//==============================================================================
void BConsole::update(void)
{
   BString command;
   if (gDebugConnection.receiveCommand(command))
      execute(command.getPtr(), NULL);         

   if (mNextFrameCommand.length() > 0)
   {
      BSimString command(mNextFrameCommand);
      mNextFrameCommand.empty();
      execute(command.getPtr());      
   }
}

//==============================================================================
// class BConsoleOutputPayload
//==============================================================================
class BConsoleOutputPayload : public BEventPayload
{
public:
   BConsoleOutputPayload(const char* pMessage)
   {
      mString.set(pMessage);
   }
   
   virtual void deleteThis(bool delivered)
   {
      delete this;
   }

   BString mString;
};

//==============================================================================
// BConsole::output
//==============================================================================
void BConsole::rawOutput(const char *szMessage)
{
   gDebugConnection.sendOutput(szMessage);      
   
   if (!mpOutputTarget)
      return;
         
   if (GetCurrentThreadId() == gEventDispatcher.getThreadId(cThreadIndexSim))
      mpOutputTarget->formatMessage(szMessage);
   else
   {
      BConsoleOutputPayload* pPayload = new BConsoleOutputPayload(szMessage);
      gEventDispatcher.send(cInvalidEventReceiverHandle, mEventHandle, cConsoleEventClassOutput, 0, 0, pPayload);
   }
}

//==============================================================================
// BConsole::output
//==============================================================================
void BConsole::output(long category, const char* szMessage, ...)
{
   BDEBUG_ASSERT((category >= 0) && (category < cChannelMax));
   
   if ((mChannelEnabledMask & Utils::BBitMasks::get64(category)) == 0)
      return;
   
   //-- Parse the arguments
   char buf[1024];        
   va_list arglist;
   va_start( arglist, szMessage );
   StringCchVPrintf(buf, sizeof(buf), szMessage, arglist);
   va_end(arglist);
      
   //-- Add the channel tag
   char buf2[1024];
   sprintf_s(buf2, sizeof(buf2), "<%s>%s</%s>", mConsoleChannels[category].getPtr(), buf, mConsoleChannels[category].getPtr());

   //-- Send it out
   rawOutput(buf2);

   // Also send it to the render thread for later drawing
   #ifndef BUILD_FINAL
      sendMessageForRenderStorage(category, buf);
      if (cMsgStatus == category)
         sendStatusTextForRenderStorage(5000, 0xFFFFFFFF, category, buf);
   #endif
}

//==============================================================================
// BConsole::outputStatusText
// This is identical to output(...) above, except that it also sends the
// message and duration as status text for rendering to the center of the screen
//==============================================================================
void BConsole::outputStatusText(DWORD duration, DWORD color, long category, const char* szMessage, ...)
{
   //-- Parse the arguments
   char buf[1024];        
   va_list arglist;
   va_start( arglist, szMessage );
   StringCchVPrintf(buf, sizeof(buf), szMessage, arglist);
   va_end(arglist);
   
   BDEBUG_ASSERT((category >= 0) && (category < cChannelMax));

   //-- Add the channel tag
   char buf2[1024];
   sprintf_s(buf2, sizeof(buf2), "<%s>%s</%s>", mConsoleChannels[category].getPtr(), buf, mConsoleChannels[category].getPtr());

   //-- Send it out
   rawOutput(buf2);

   // Also send it to the render thread for later drawing
   #ifndef BUILD_FINAL
      sendMessageForRenderStorage(category, buf);
      sendStatusTextForRenderStorage(duration, color, category, buf);
   #endif
}

//==============================================================================
// Outputs text to the lower right corner of the screen
//==============================================================================
void BConsole::outputInfoText(DWORD color, const char* szMessage, ...)
{
#ifndef BUILD_FINAL
   //-- Parse the arguments
   char buf[1024];        
   va_list arglist;
   va_start( arglist, szMessage );
   StringCchVPrintf(buf, sizeof(buf), szMessage, arglist);
   va_end(arglist);

   // Also send it to the render thread for later drawing
   sendInfoTextForRenderStorage(color, buf);
#endif
}

//==============================================================================
// BConsole::addChannel
//==============================================================================
void BConsole::addChannel(const char* channelName)
{
   char buf[1024];

   sprintf_s(buf, sizeof(buf), "<add>%s</add>",channelName);
   rawOutput(buf);
}

//==============================================================================
// BConsole::checkDelayed
//==============================================================================
void BConsole::checkDelayed(void)
{
   if (mDelayedCommand.length() > 0)
   {
      BSimString tempcmd = mDelayedCommand;
      mDelayedCommand.empty();
      if (execute(tempcmd.getPtr()) != cErrOK)
      {
         output("Error: Unable to execute command!");
      }
   }

   if (mDelayedFile.length() > 0)
   {
      const long bufLen = 512;
      char buf[bufLen];
      char *token;
      StringCchCopyA(buf, bufLen, mDelayedFile.getPtr());
      mDelayedFile.empty();
      char *pStrTokContext = NULL;
      token = strtok_s(buf, " ",&pStrTokContext);
      while(token != NULL)
      {
         BSimString dirIDText(token);
         token = strtok_s(NULL, " ",&pStrTokContext);
         execFile(dirIDText.asLong(), token);
         token = strtok_s(NULL, " ",&pStrTokContext);
      }
   }
}

//==============================================================================
// BConsole::execute
//==============================================================================
long BConsole::execute(const char *command, const char *src)
{
   src;
   if (mExecuting)
   {
      executeDelayed(command);
      return(cErrOK);
   }

   mExecuting = true;

   //Do the console command.
   if (!mConsoleCompiler->parseImmediate(command, strlen(command)+1))
   {
      BFixedString256 buf(cVarArg, "Error: Command parse failed: %s", command);
      output(buf);
   }

   //Unset it.
   mExecuting = false;
   checkDelayed();

   return(cErrOK);
}

//==============================================================================
// BConsole::executeDelayed
//==============================================================================
void BConsole::executeDelayed(const char *command)
{
   mDelayedCommand.append(B(" "));
   mDelayedCommand.append(BSimString(command));
}

//==============================================================================
// BConsole::executeNextFrame
//==============================================================================
void BConsole::executeNextFrame(const char *command)
{
   mNextFrameCommand.append(B(" "));
   mNextFrameCommand.append(BSimString(command));
}

//==============================================================================
// BConsole::execFile
//==============================================================================
long BConsole::execFile(long dirID, const char *filename, const char *src)
{
   src;

   if (mExecuting)
   {
      execFileDelayed(dirID, filename);
      return(cErrOK);
   }

   BFile file;
   if(!file.openReadOnly(dirID, filename))
   {
      {setBlogError(0); blogerrortrace("Could not find console file %s!",BStrConv::toA(filename));}
      return(cErrFile);
   }

   //Count the valid lines in the file.  We also count the "size" of the
   //file (adding 1 space for each line).
   long numberLines=0;
   long totalSize=0;
   char buffer[1024];
   while (file.fgets(buffer, 1024) != NULL)
   {
      numberLines++;
      totalSize+=strlen(buffer)+1;
   }
   file.setOffset(0);

   if (gConfig.isDefined(cConfigConsoleFileEntire))
   {
      long bufLen = totalSize+1;
      char *totalBuffer=new char[bufLen];
      while (file.fgets(buffer, 1024) != NULL)
      {
         StringCchCatA(totalBuffer, bufLen, buffer);
         StringCchCatA(totalBuffer, bufLen, " ");
      }
      // clean it up
      buffer[totalSize] = '\0';
      file.close();

      /*long retval;
      retval = compile(buffer,src);
      if (retval != cErrOK)
      {
         delete totalBuffer;
         return(retval);
      }
      interpret();
      delete totalBuffer;*/
      mConsoleCompiler->parseImmediate(buffer, strlen(buffer)+1);
   }
   else
   {
      // store off the filename so that we can check it to disallow certain console funcs 
      // in certain situations
      mCurrentFile.set(filename);
      long currentNumberLines=0;
      while ((file.fgets(buffer, 1024) != NULL) && (currentNumberLines < numberLines))
      {
         execute(buffer, filename);
         memset(buffer, 0, sizeof(buffer));
         currentNumberLines++;
      }

      file.close();
      mCurrentFile.set("");
   }
   return(cErrOK);
}

//==============================================================================
// BConsole::execFileDelayed
//==============================================================================
void BConsole::execFileDelayed(long dirID, const char *filename)
{
   BSimString dirIDText;
   dirIDText.setToLong(dirID);
   mDelayedFile.append(B(" "));
   mDelayedFile.append(dirIDText);
   mDelayedFile.append(B(" "));
   mDelayedFile.append(BSimString(filename));
}

//==============================================================================
// xsMessage
//==============================================================================
void xsMessage(const char *message)
{
   gConsole.output(cMsgConsole, message);
}

//==============================================================================
// BConsole::initializeConsoleCompiler
//==============================================================================
bool BConsole::initializeConsoleCompiler(REGISTER_CONSOLE_FUNCS_CALLBACK registerFuncs)
{
   //Create a messenger.
   mConsoleMessenger=new BXSMessenger;
   if (mConsoleMessenger == NULL)
      return(false);

   mConsoleMessenger->setErrorMsgFun(xsMessage);
   mConsoleMessenger->setWarn(gConfig.isDefined(cConfigXSWarn));
   mConsoleMessenger->setWarnMsgFun(xsMessage);
   mConsoleMessenger->setInfo(gConfig.isDefined(cConfigXSInfo));
   mConsoleMessenger->setInfoMsgFun(xsMessage);

   //Allocate the XS source.
   mConsoleSource=new BXSSource(false);
   if (mConsoleSource == NULL)
      return(false);

   //Allocate the XS data.
   mConsoleData=new BXSData(12345678, B("Console"), mConsoleMessenger, mConsoleSource);
   if (mConsoleData == NULL)
      return(false);

   //Init the source/syscall module.
   mConsoleSyscalls=new BXSSyscallModule(mConsoleMessenger, mConsoleSource->getSymbolTable(), false, true, true);
   if (mConsoleSyscalls == NULL)
      return(false);

   //Add the syscalls.
   if(registerFuncs)
   {
      if(!registerFuncs(mConsoleSyscalls))
         return(false);
   }

   /*
   #ifndef BUILD_FINAL
   if(gConfig.isDefined("generateXSSyscallTypeFiles"))
      generateXSSyscallTypeFiles(sm);
   #endif
   */

   //Initialize the compiler.
   mConsoleCompiler=new BXSCompiler(mConsoleMessenger, mConsoleData, mConsoleSource, mConsoleSyscalls);
   if (mConsoleCompiler == NULL)
      return(false);

   bool ok = mConsoleCompiler->initialize(cDirProduction, cDirProduction);
   if (!ok)
      return(false);

   return(true);
}

//==============================================================================
// BConsole::destroyConsoleCompiler
//==============================================================================
void BConsole::destroyConsoleCompiler(void)
{
   BDELETE(mConsoleCompiler);
   BDELETE(mConsoleData);
   BDELETE(mConsoleSource);
   BDELETE(mConsoleSyscalls);
   BDELETE(mConsoleMessenger);
}


//============================================================================
//  BConsole::addUnicodeConsoleParam()
//============================================================================
void BConsole::addUnicodeConsoleParam(const BSimString& param, BSimString& key)
{
   key.format(B("%s%d%s"), B("|||"), mUnicodeConsoleParamKey++, B("|||"));

   UnicodeConsoleParam node;
   node.mKey   = key;
   node.mParam = param;
   mUnicodeConsoleParams.addToHead(node);
}


//============================================================================
//  BConsole::removeUnicodeConsoleParam()
//============================================================================
void BConsole::removeUnicodeConsoleParam(const BSimString& key, BSimString& param)
{
   //-- Search for the key.
   BHandle              hNode;
//-- FIXING PREFIX BUG ID 7790
   const UnicodeConsoleParam* pNode = mUnicodeConsoleParams.getHead(hNode);
//--
   while (pNode)
   {
      if (key == pNode->mKey)
      {
         param = pNode->mParam;
         mUnicodeConsoleParams.remove(hNode);
         return;
      }
      pNode = mUnicodeConsoleParams.getNext(hNode);
   }

   //-- No key match.   
   param.set(key);
}

//==============================================================================
// BConsole::setChannelEnabled
//==============================================================================
void BConsole::setChannelEnabled(long category, bool enabled)
{
   BDEBUG_ASSERT((category >= 0) && (category < cChannelMax));
   
   const uint64 bit = Utils::BBitMasks::get64(category);
   
   if (enabled)
      mChannelEnabledMask |= bit;
   else
      mChannelEnabledMask &= ~bit;
}

//==============================================================================
// BConsole::getChannelEnabled
//==============================================================================
bool BConsole::getChannelEnabled(long category) const
{
   BDEBUG_ASSERT((category >= 0) && (category < cChannelMax));
   
   return (mChannelEnabledMask & Utils::BBitMasks::get64(category)) != 0;
}

//==============================================================================
// BConsole::cleanup
//==============================================================================
bool BConsole::cleanup(void)
{
   destroyConsoleCompiler();
   
   if (mEventHandle != cInvalidEventReceiverHandle)
   {
      gEventDispatcher.removeClientImmediate(mEventHandle);
      mEventHandle = cInvalidEventReceiverHandle;
   }
   
   
   #ifndef BUILD_FINAL
      gConsoleOutput.deinit();
      gStoredConsoleOutput.deinit();
   #endif

   return(true);
}

//==============================================================================
// BConsole::registerRemoteOutputTarget
//==============================================================================
bool BConsole::registerRemoteOutputTarget(IRemoteOutputTarget *pTarget)
{
   mpOutputTarget=pTarget;
   return (true);
}


//==============================================================================
// BConsole::releaseRemoteOutputTarget
//==============================================================================
bool BConsole::releaseRemoteOutputTarget( void )
{
   mpOutputTarget=NULL;
   return (true);
}

//==============================================================================
// BConsole::receiveEvent
//==============================================================================
bool BConsole::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   switch (event.mEventClass)
   {
      case cConsoleEventClassOutput:
      {
         const BConsoleOutputPayload* pPayload = reinterpret_cast<const BConsoleOutputPayload*>(event.mpPayload);
         BDEBUG_ASSERT(pPayload);
      
         if (mpOutputTarget)
            mpOutputTarget->formatMessage(pPayload->mString.getPtr());

         break;            
      }
   }      

   return false;
}

#ifndef BUILD_FINAL
//==============================================================================
// BConsole::sendMessageForRenderStorage
//==============================================================================
void BConsole::sendMessageForRenderStorage(long category, const char* szMessage)
{
   BStoredConsoleOutputPayload* pPayload = new BStoredConsoleOutputPayload(category, szMessage);
   gEventDispatcher.send(cInvalidEventReceiverHandle, gStoredConsoleOutput.getEventHandle(), BStoredConsoleOutput::cStoredConsoleOutputEventClassMessage, 0, 0, pPayload);
}

//==============================================================================
// BConsole::sendCategoryHeadersForRenderStorage
//==============================================================================
void BConsole::sendCategoryHeadersForRenderStorage()
{
   for (long i = 0; i < cChannelMax; i++)
   {
      BStoredConsoleOutputPayload* pPayload = new BStoredConsoleOutputPayload(i, mConsoleChannels[i].getPtr());
      gEventDispatcher.send(cInvalidEventReceiverHandle, gStoredConsoleOutput.getEventHandle(), BStoredConsoleOutput::cStoredConsoleOutputEventClassHeader, 0, 0, pPayload);
   }
}

//==============================================================================
// BConsole::sendStatusTextForRenderStorage
//==============================================================================
void BConsole::sendStatusTextForRenderStorage(DWORD duration, DWORD color, long category, const char* szMessage)
{
   BStoredConsoleOutputStatusTextPayload* pPayload = new BStoredConsoleOutputStatusTextPayload(duration, color, category, szMessage);
   gEventDispatcher.send(cInvalidEventReceiverHandle, gStoredConsoleOutput.getEventHandle(), BStoredConsoleOutput::cStoredConsoleOutputEventClassStatusText, 0, 0, pPayload);
}

//==============================================================================
// BConsole::sendInfoTextForRenderStorage
//==============================================================================
void BConsole::sendInfoTextForRenderStorage(DWORD color, const char* szMessage)
{
   BStoredConsoleOutputStatusTextPayload* pPayload = new BStoredConsoleOutputStatusTextPayload(0, color, 0, szMessage);
   gEventDispatcher.send(cInvalidEventReceiverHandle, gStoredConsoleOutput.getEventHandle(), BStoredConsoleOutput::cStoredConsoleOutputEventClassInfoText, 0, 0, pPayload);
}
#endif


#ifndef BUILD_FINAL

//==============================================================================
//==============================================================================
// BStoredConsoleOutput implementation
//==============================================================================
//==============================================================================

//==============================================================================
// BStoredConsoleOutput::BStoredConsoleOutput()
//==============================================================================
BStoredConsoleOutput::BStoredConsoleOutput() :
   mStatusTextCategory(0),
   mStatusTextDuration(0),
   mPerCategoryStringLimit(-1)
{
}

//==============================================================================
// BStoredConsoleOutput::~BStoredConsoleOutput()
//==============================================================================
BStoredConsoleOutput::~BStoredConsoleOutput()
{
}

//==============================================================================
// BStoredConsoleOutput::init()
//==============================================================================
void BStoredConsoleOutput::init(int perCategoryStringLimit)
{
   ASSERT_THREAD(cThreadIndexSim);
   mPerCategoryStringLimit = perCategoryStringLimit;
   eventReceiverInit(cThreadIndexRender);
}

//==============================================================================
// BStoredConsoleOutput::deinit()
//==============================================================================
void BStoredConsoleOutput::deinit()
{
   ASSERT_THREAD(cThreadIndexSim);
   eventReceiverDeinit();
}

//==============================================================================
// BStoredConsoleOutput::receiveEvent()
//==============================================================================
bool BStoredConsoleOutput::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   switch (event.mEventClass)
   {
      case cStoredConsoleOutputEventClassMessage:
      {
         // Store message in specified category array
         const BStoredConsoleOutputPayload* pPayload = reinterpret_cast<const BStoredConsoleOutputPayload*>(event.mpPayload);
         BDEBUG_ASSERT(pPayload);
         if (pPayload->mCategory > 0 && pPayload->mCategory < cChannelMax)
         {
            // Remove old strings beyond limit to make room for new string
           // long currentNumStrings = mOutputMessageList[pPayload->mCategory].getNumber();
          //  if ((mPerCategoryStringLimit > 0) && (currentNumStrings >= mPerCategoryStringLimit))
           //    mOutputMessageList[pPayload->mCategory].erase(0, currentNumStrings - mPerCategoryStringLimit + 1);

            // Add new string
         //   if (mPerCategoryStringLimit != 0)
         //      mOutputMessageList[pPayload->mCategory].add(pPayload->mString.getPtr());
         }
         break;
      }
      case cStoredConsoleOutputEventClassHeader:
      {
         // Store category header in specified category
         const BStoredConsoleOutputPayload* pPayload = reinterpret_cast<const BStoredConsoleOutputPayload*>(event.mpPayload);
         BDEBUG_ASSERT(pPayload);
         if (pPayload->mCategory >= 0 && pPayload->mCategory < cChannelMax)
         {
            mHeaders[pPayload->mCategory].set(pPayload->mString.getPtr());
         }
         break;
      }
      case cStoredConsoleOutputEventClassStatusText:
      {
         // Store status text string / duration and start timer
         const BStoredConsoleOutputStatusTextPayload* pPayload = reinterpret_cast<const BStoredConsoleOutputStatusTextPayload*>(event.mpPayload);
         BDEBUG_ASSERT(pPayload);
         if ((pPayload->mDuration > 0) && (pPayload->mCategory >= 0 && pPayload->mCategory < cChannelMax))
         {
            mStatusTextCategory = pPayload->mCategory;
            mStatusTextDuration = pPayload->mDuration;
            mStatusTextColor = pPayload->mColor;
            mStatusTextTimer.start();
            mStatusText.set(pPayload->mString.getPtr());
         }
         break;
      }
      case cStoredConsoleOutputEventClassInfoText:
      {
         // Store status text string / duration and start timer
         const BStoredConsoleOutputStatusTextPayload* pPayload = reinterpret_cast<const BStoredConsoleOutputStatusTextPayload*>(event.mpPayload);
         BDEBUG_ASSERT(pPayload);

         mInfoTextColor = pPayload->mColor;
         mInfoText.set(pPayload->mString.getPtr());

         break;
      }
   }

   return false;
}

#endif

/*
#ifndef BUILD_FINAL
//==============================================================================
// BConsole::generateXSSyscallTypeFiles
//==============================================================================
int __cdecl tempStringSort(const void *a, const void *b)
{
   BSimString* s1=(BSimString*)a;
   BSimString* s2=(BSimString*)b;
   return(s1->compare(*s2));
}

void BConsole::generateXSSyscallTypeFiles(class BXSSyscallModule *sm)
{
   BDynamicArray<BSimString> stringList;
   char buffer[16];
   for(long i=0; i<sm->getNumberSyscalls(); i++)
   {
      const BXSSyscallEntry* e=sm->getSyscall(i);
      if(sm->buildSyscallTypeString(e, buffer, sizeof(buffer)) && buffer[0]!=NULL && buffer[1]!=NULL)
         stringList.uniqueAdd(BSimString(buffer));
   }
   stringList.sort(tempStringSort);

   BSimString text1, text2;

   coreOutput(B(""));
   for(long i=0; i<stringList.getNumber(); i++)
      coreOutput(stringList[i]);

   coreOutput(B(""));
   coreOutput(B("// xssyscalltypes.h"));
   coreOutput(B(""));

   coreOutput(B("enum"));
   coreOutput(B("{"));
   coreOutput(B("   cSyscallType_void,"));
   coreOutput(B("   cSyscallType_d,"));
   coreOutput(B("   cSyscallType_f,"));
   coreOutput(B("   cSyscallType_b,"));
   for(long i=0; i<stringList.getNumber(); i++)
   {
      text1=B("   cSyscallType_");
      text1.append(stringList[i]);
      text1.append(B(","));
      coreOutput(text1);
   }
   coreOutput(B("   cNumberSyscallTypes"));
   coreOutput(B("};"));
   coreOutput(B(""));

   const BCHAR_T* funcPrefix[]=
   {
      B("typedef void (*XSPROC"),
      B("typedef long (*XSFUNd"),
      B("typedef float (*XSFUNf"),
      B("typedef bool (*XSFUNb"),
      B("typedef char* (*XSFUNs"),
      B("typedef BVector (*XSFUNv"),
   };

   const BCHAR_T* typeSuffix[]={B("_d"), B("_f"), B("_b")};
   const BCHAR_T* typeName[]={B("long"), B("float"), B("bool"), B(""), B("BVector")};
   const BCHAR_T* defaultValue[]={B("-1"), B("-1.0f"), B("false"), B(""), B("cInvalidVector")};
   const BCHAR_T* parmSuffix[]={B("parms      "), B("parmsF     "), B("(bool)parms")};

   for(long i=0; i<6; i++)
   {
      text1=funcPrefix[i];
      text1.append(B(")(void);"));
      coreOutput(text1);
      coreOutput(B(""));

      for(long t=0; t<3; t++)
      {
         for(long j=1; j<15; j++)
         {
            text1=funcPrefix[i];
            text1.append(typeSuffix[t]);
            text2.setToLong(j);
            text1.append(text2);
            text1.append(B(")"));
            if(j<10)
               text1.append(B(" "));
            text1.append(B("("));
            for(long k=0; k<j; k++)
            {
               if(k>0)
                  text1.append(B(", "));
               text1.append(typeName[t]);
               text1.append(B(" a"));
               text2.setToLong(k+1);
               text1.append(text2);
            }
            text1.append(B(");"));
            coreOutput(text1);
         }
         coreOutput(B(""));
      }

      for(long j=0; j<stringList.getNumber(); j++)
      {
         text1=funcPrefix[i];
         text1.append(B("_"));
         text1.append(stringList[j]);
         text1.append(B(")"));
         long len=stringList[j].length();
         if(len<12)
         {
            for(long k=0; k<(12-len); k++)
               text1.append(B(" "));
         }
         text1.append(B("("));
         const char* str=stringList[j].getPtr();
         long index=0;
         long k=0;
         for(;;)
         {
            char ctype=str[index];
            if(ctype==0)
               break;
            index++;
            char ccount=str[index];
            if(ccount==0)
               break;
            index++;
            long lcount=ccount-'1'+1;
            for(long m=0; m<lcount; m++)
            {
               if(k>0)
                  text1.append(B(", "));
               if(ctype=='d')
                  text1.append(B("long a"));
               else if(ctype=='f')
                  text1.append(B("float a"));
               else if(ctype=='b')
                  text1.append(B("bool a"));
               else
                  BASSERT(0);
               text2.setToLong(k+1);
               text1.append(text2);
               k++;
            }
         }
         text1.append(B(");"));
         coreOutput(text1);
      }
      coreOutput(B(""));
   }


   const BCHAR_T* funcPrefix2[]=
   {
      B("XSPROC"),
      B("XSFUNd"),
      B("XSFUNf"),
      B("XSFUNb"),
      B("XSFUNs"),
      B("XSFUNv"),
   };

   const BCHAR_T* methodName[]=
   {
      B("callVoidSyscall"),
      B("callIntegerSyscall"),
      B("callFloatSyscall"),
      B("callBoolSyscall"),
      B("callStringSyscall"),
      B("callVectorSyscall"),
   };

   const BCHAR_T* methodHeader[]=
   {
      B("bool BXSSyscallModule::callVoidSyscall(long syscallID, long *parms, long numberParms)"),
      B("bool BXSSyscallModule::callIntegerSyscall(long syscallID, long *parms, long numberParms, long *sRV, BXSData *data)"),
      B("bool BXSSyscallModule::callFloatSyscall(long syscallID, long *parms, long numberParms, float *sRV, BXSData *data)"),
      B("bool BXSSyscallModule::callBoolSyscall(long syscallID, long *parms, long numberParms, bool *sRV, BXSData *data)"),
      B("bool BXSSyscallModule::callStringSyscall(long syscallID, long *parms, long numberParms, BXSData *data)"),
      B("bool BXSSyscallModule::callVectorSyscall(long syscallID, long *parms, long numberParms, BVector *sRV, BXSData *data)"),
   };

   coreOutput(B("// xssyscalltypes.cpp"));
   coreOutput(B(""));

   coreOutput(B("const char* gSyscallTypeString[] = "));
   coreOutput(B("{"));
   coreOutput(B("   \"void\","));
   coreOutput(B("   \"d\","));
   coreOutput(B("   \"f\","));
   coreOutput(B("   \"b\","));
   for(long i=0; i<stringList.getNumber(); i++)
   {
      text1=B("   \"");
      text1.append(stringList[i]);
      text1.append(B("\","));
      coreOutput(text1);
   }
   coreOutput(B("   \"\""));
   coreOutput(B("};"));
   coreOutput(B(""));

   for(long i=0; i<6; i++)
   {
      coreOutput(B("//=============================================================================="));
      text1=B("// BXSSyscallModule::");
      text1.append(methodName[i]);
      coreOutput(text1);
      coreOutput(B("//=============================================================================="));
      coreOutput(methodHeader[i]);
      coreOutput(B("{"));
      coreOutput(B("   //Bomb checks."));
      coreOutput(B("   if ((syscallID < 0) || (syscallID >= mSyscalls.getNumber()) )"));
      coreOutput(B("      return(false);"));
      coreOutput(B("   if (mSyscalls[syscallID] == NULL)"));
      coreOutput(B("      return(false);"));
      coreOutput(B("   if ( ((mSyscalls[syscallID]->getContext() == false) && (mSyscalls[syscallID]->getNumberParameters() != numberParms)) ||"));
      coreOutput(B("      ((mSyscalls[syscallID]->getContext() == true) && (mSyscalls[syscallID]->getNumberParameters() != numberParms-1))"));
      if(i!=0 && i!=4)
         coreOutput(B("      || (sRV == NULL))"));
      else
         coreOutput(B("      )"));
      coreOutput(B("      return(false);"));
      coreOutput(B(""));
      coreOutput(B("   //Do the call."));
      coreOutput(B("   long syscallAddress=mSyscalls[syscallID]->getAddress();"));
      coreOutput(B("   //If we have an invalid address, just return in a default way.  We return true"));
      coreOutput(B("   //so that the interpreter will keep going as if it all worked.  This allows us"));
      coreOutput(B("   //to fixup the syscall addresses in a savegame."));
      if(i==4)
         coreOutput(B("   static const char invalidString[]=\"INVALID\";"));
      coreOutput(B("   if (syscallAddress == -1)"));
      coreOutput(B("   {"));
      if(i!=4 && i!=0)
      {
         text1=B("      *sRV=");
         text1.append(defaultValue[i-1]);
         text1.append(B(";"));
         coreOutput(text1);
         coreOutput(B("      if (data != NULL)"));
         text1=B("         data->setTempReturnValue((BYTE*)sRV, sizeof(");
         text1.append(typeName[i-1]);
         text1.append(B("));"));
         coreOutput(text1);
      }
      else if(i!=0)
      {
         coreOutput(B("      if (data!=NULL)"));
         coreOutput(B("         data->setTempReturnValue((BYTE*)invalidString, strlen(invalidString));"));
      }
      coreOutput(B("      return(true);"));
      coreOutput(B("   }"));

      coreOutput(B(""));
      coreOutput(B("   float* parmsF = reinterpret_cast<float*>(parms);"));
      coreOutput(B("   long syscallType=mSyscalls[syscallID]->getSyscallType();"));
      if(i==4)
         coreOutput(B("   char *rV=NULL;"));
      coreOutput(B(""));
      coreOutput(B("   switch(syscallType)"));
      coreOutput(B("   {"));

      text1=B("      case cSyscallType_void: ");
      if(i!=0 && i!=4)
         text1.append(B("*sRV="));
      text1.append(B("(("));
      text1.append(funcPrefix2[i]);
      text1.append(B(")syscallAddress)(); break;"));
      coreOutput(text1);
      coreOutput(B(""));

      for(long t=0; t<3; t++)
      {
         text1=B("      case cSyscallType");
         text1.append(typeSuffix[t]);
         text1.append(B(":"));
         coreOutput(text1);
         coreOutput(B("         switch (numberParms)"));
         coreOutput(B("         {"));
         for(long j=1; j<15; j++)
         {
            text1=B("            case ");
            text2.setToLong(j);
            text1.append(text2);
            text1.append(B(": "));
            if(j<10)
               text1.append(B(" "));
            if(i==4)
               text1.append(B("rV="));
            else if(i!=0)
               text1.append(B("*sRV="));
            text1.append(B("(("));
            text1.append(funcPrefix2[i]);
            text1.append(typeSuffix[t]);
            text1.append(text2);
            text1.append(B(")"));
            if(j<10)
               text1.append(B(" "));
            text1.append(B("syscallAddress)("));
            for(long k=0; k<j; k++)
            {
               if(k>0)
                  text1.append(B(", "));
               text1.append(parmSuffix[t]);
               text1.append(B("["));
               text2.setToLong(k);
               text1.append(text2);
               text1.append(B("]"));
            }
            text1.append(B("); break;"));
            coreOutput(text1);
}

         coreOutput(B("            default: return(false);"));
         coreOutput(B("         }"));
         coreOutput(B("         break;"));
         coreOutput(B(""));
      }

      for(long j=0; j<stringList.getNumber(); j++)
      {
         text1=B("      case cSyscallType_");
         text1.append(stringList[j]);
         long len=stringList[j].length();
         if(len<12)
         {
            for(long k=0; k<(12-len); k++)
               text1.append(B(" "));
         }
         text1.append(B(": "));
         if(i==4)
            text1.append(B("rV="));
         else if(i!=0)
            text1.append(B("*sRV="));
         text1.append(B("(("));
         text1.append(funcPrefix2[i]);
         text1.append(B("_"));
         text1.append(stringList[j]);
         text1.append(B(")"));
         if(len<12)
         {
            for(long k=0; k<(12-len); k++)
               text1.append(B(" "));
         }
         text1.append(B("syscallAddress)("));
         const char* str=stringList[j].getPtr();
         long index=0;
         long k=0;
         for(;;)
         {
            char ctype=str[index];
            if(ctype==0)
               break;
            index++;
            char ccount=str[index];
            if(ccount==0)
               break;
            index++;
            long lcount=ccount-'1'+1;
            for(long m=0; m<lcount; m++)
            {
               if(k>0)
                  text1.append(B(", "));
               if(ctype=='d')
                  text1.append(B("parms      ["));
               else if(ctype=='f')
                  text1.append(B("parmsF     ["));
               else if(ctype=='b')
                  text1.append(B("(bool)parms["));
               else
                  BASSERT(0);
               text2.setToLong(k);
               text1.append(text2);
               text1.append(B("]"));
               k++;
            }
         }
         text1.append(B("); break;"));
         coreOutput(text1);
      }
      coreOutput(B("   }"));
      coreOutput(B(""));

      if(i!=4 && i!=0)
      {
         coreOutput(B("   if (data != NULL)"));
         text1=B("      data->setTempReturnValue((BYTE*)sRV, sizeof(");
         text1.append(typeName[i-1]);
         text1.append(B("));"));
         coreOutput(text1);
      }
      else if(i!=0)
      {
         coreOutput(B("   if (rV == NULL)"));
         coreOutput(B("      return(false);"));
         coreOutput(B(""));
         coreOutput(B("   if (data != NULL)"));
         coreOutput(B("   {"));
         coreOutput(B("      long stringLength=strlen(rV)+1;"));
         coreOutput(B("      long newValueIndex=data->allocateUserHeapValue(BXSVariableEntry::cStringVariable, stringLength);"));
         coreOutput(B("      if (newValueIndex < 0)"));
         coreOutput(B("         return(false);"));
         coreOutput(B("      if (data->setUserHeapValue(newValueIndex, BXSVariableEntry::cStringVariable, stringLength, (BYTE*)rV) == false)"));
         coreOutput(B("         return(false);"));
         coreOutput(B("      if (data->incrementUserHeapRefCount(newValueIndex) == false)"));
         coreOutput(B("         return(false);"));
         coreOutput(B("      //Set the temp return value."));
         coreOutput(B("      data->setTempReturnValue((BYTE*)&newValueIndex, sizeof(long));"));
         coreOutput(B("   }"));
      }

      coreOutput(B(""));
      coreOutput(B("   return(true);"));
      coreOutput(B("}"));
   }

   coreOutput(B(""));
}
#endif
*/


