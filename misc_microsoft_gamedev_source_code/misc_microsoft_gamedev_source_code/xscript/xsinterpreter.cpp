//==============================================================================
// xsinterpreter.cpp
//
// Copyright (c) 2000-2003, Ensemble Studios
//==============================================================================

//==============================================================================
//Includes
#include "xscript.h"
#include "xsinterpreter.h"
#include "xsdata.h"
#include "xsdefines.h"
#include "xsmessenger.h"
#include "xsopcodes.h"
#include "xsruntime.h"
#include "xssource.h"

//==============================================================================
//Defines
#define DEBUGINTERPRET

namespace 
{
   __forceinline float readFloat(const void* pFloat)
   {
#ifdef XBOX
      // rg [8/22/05] - Changed xsvariableentry.h so bool's are 4 bytes instead of 1, which should fix the float alignment problem.      
      // Floats are written by memcpy() so that's why the writes never had a problem.
      BDEBUG_ASSERT(0 == (reinterpret_cast<DWORD>(pFloat) & 3));
      
      //const DWORD temp = *reinterpret_cast<const DWORD*>(pFloat);
      //return *reinterpret_cast<const float*>(&temp);
      return *reinterpret_cast<const float*>(pFloat);
#else
      return *reinterpret_cast<const float*>(pFloat);
#endif
   }
}

//==============================================================================
// BXSInterpreter::BXSInterpreter
//==============================================================================
BXSInterpreter::BXSInterpreter(BXSMessenger *messenger, BXSSyscallModule *sm) :
   mSyscalls(sm),
   mMessenger(messenger)
{
}

//==============================================================================
// BXSInterpreter::~BXSInterpreter
//==============================================================================
BXSInterpreter::~BXSInterpreter(void)
{
   //DO NOT DELETE mSyscalls.  We don't own that memory.
   //DO NOT DELETE mMessenger.  We don't own that memory.
}

//==============================================================================
// BXSInterpreter::initialize
//==============================================================================
bool BXSInterpreter::initialize(void)
{
   return(true);
}

//==============================================================================
// BXSInterpreter::interpret
//==============================================================================
bool BXSInterpreter::interpret(BXSSource *source, BXSData *data)
{
   //Bomb check.
   if (data == NULL)
   {
      mMessenger->errorMsg("Error: data is NULL.");
      return(false);
   }

   return(interpretFunction(source, data, data->getMainFunctionID(), -1, false));
}

//==============================================================================
// BXSInterpreter::interpretRule
//==============================================================================
bool BXSInterpreter::interpretRule(BXSSource *source, BXSData *data, long ruleID)
{
   //Bomb check.
   if (data == NULL)
   {
      mMessenger->errorMsg("Error: data is NULL.");
      return(false);
   }

   //Get the rule entry.
   BXSRuleEntry *ruleEntry=data->getRuleEntry(ruleID);
   if (ruleEntry == NULL)
   {
      mMessenger->errorMsg("Error: %d is an invalid ruleID.", ruleID);
      return(false);
   }

   //Set the current rule ID.
   data->setCurrentRuleID(ruleID);

   //Do the interp.
   bool rVal=interpretFunction(source, data, ruleEntry->getFunctionID(), -1, false);
   //If we are now in a breakpoint, return.
   if (data->getBreakpoint() == true)
      return(rVal);

   //Unset the current rule ID.
   data->setCurrentRuleID(-1);
   return(rVal);
}

//==============================================================================
// BXSInterpreter::interpretRules
//==============================================================================
bool BXSInterpreter::interpretRules(BXSSource *source, BXSData *data, DWORD currentTime, DWORD timeLimit, bool &lastRule)
{
   //Bomb check.
   if (data == NULL)
   {
      mMessenger->errorMsg("Error: data is NULL.");
      return(false);
   }
   lastRule=false;

   //Simple stand-in timing.
   //DCP TODO 05/29/01: Replace this with something better.
   //DWORD startTime=timeGetTime();
   // fixme ajl 1/27/06 - hack in an attempt to make triggers run in sync for MP campaign
   DWORD timeSpent=0;

   //Go into the rule processing loop.  We can exit this if we haven't finished a full
   //rule pass or not.
   for(;;)
   {
      //Get the next sorted rule ID.  If it's invalid, we've hit the last rule and we break out.
      long ruleID=data->getNextSortedRuleID();
      if (ruleID == -1)
      {
         lastRule=true;
         break;
      }

      //Get the rule entry.
      BXSRuleEntry *ruleEntry=data->getRuleEntry(ruleID);
      if (ruleEntry == NULL)
      {
         mMessenger->warningMsg("Warning: %d is an invalid ruleID.", ruleID);
         data->incrementNextSortedRuleIndex();
         continue;
      }

      //Get the execution time difference.
      DWORD ruleTime=ruleEntry->getLastExecuteTime();
      DWORD timeDiff=currentTime-ruleTime;
      // fixme ajl 1/27/06 - hack in an attempt to make triggers run in sync for MP campaign
      //DWORD timeSpent=timeGetTime()-startTime;
      timeSpent++;

      //Do the standard rule execute checks unless this is the first time
      //and this rule is runImmediately rule.
      if ((currentTime > (DWORD)0) || (ruleEntry->getRunImmediately() == false))
      {
         //Don't execute this rule if it hasn't met its min interval yet.
         if (timeDiff < ruleEntry->getMinInterval() || ruleTime > currentTime)
         {
            data->incrementNextSortedRuleIndex();
            continue;
         }
         //If we're over our time limit (a limit that's greater than 0), we only execute
         //the rules that MUST execute because they're over their max interval.
         if ((timeLimit > (DWORD)0) && (timeSpent >= timeLimit) && (timeDiff < ruleEntry->getMaxInterval()) )
         {
            data->incrementNextSortedRuleIndex();
            continue;
         }
      }

      //If we're here, we're going to execute the rule, so we set its time.
      ruleEntry->setLastExecuteTime(currentTime);

      //Set the current rule ID.
      data->setCurrentRuleID(ruleID);
      //Interp it.
      //if (mMessenger->getListInterpreter() == true)
      //   mMessenger->runMsg("InterpretRules: ruleID=%d.", ruleID);
      if (interpretFunction(source, data, ruleEntry->getFunctionID(), -1, false) == false)
      {
         mMessenger->warningMsg("Warning: ruleID=%d failed execution.", ruleID);
         data->incrementNextSortedRuleIndex();
         //Unset the current rule ID.
         data->setCurrentRuleID(-1);
         continue;
      }

      //If we are now in a breakpoint, return.
      if (data->getBreakpoint() == true)
         return(true);

      //Unset the current rule ID.
      data->setCurrentRuleID(-1);
      //Go to the next rule.
      data->incrementNextSortedRuleIndex();
   }

   return(true);
}

//==============================================================================
// BXSInterpreter::interpretFunction
//==============================================================================
bool BXSInterpreter::interpretFunction(BXSSource *source, BXSData *data, long functionID,
   long parameter, bool pushParameter)
{
   //Bomb check.
   if ((source == NULL) || (data == NULL))
   {
      mMessenger->errorMsg("Error: source or data is NULL.");
      return(false);
   }

   //Reset the failsafe measures.
   data->resetFailsafes();

   //Get the size of the stack now since the heap init may have thrown
   //something on it.
   long oldStack=data->getStack().getNumber();
   long oldHeap=data->getHeap().getNumber();

   //Get the function entry.
   const BXSFunctionEntry *fe=data->getFunctionEntry(functionID);
   if (fe == NULL)
   {
      mMessenger->errorMsg("Error: invalid function entry.");
      data->resetAll();
      return(false);
   }
   /*if (mMessenger->getListInterpreter() == true)
   {
      char foo[512];
      bsnprintf(foo, 512, "  InterpretFunction: FID=%d (%s).", fe->getID(), source->getSymbol(fe->getSymbolID()));
      mMessenger->runMsg(foo);
   }*/

   //Before we push the function on, we need to see if we are supposed to push the
   //code-based parameter that we were given.  If we are, we MUST check to ensure
   //that the function we're being asked to interpret takes a single int argument
   //(and nothing more).  We push the parameter on as a literal integer.  The way
   //the stack is "popped" (by resetting its size to the pre-parm/function size)
   //will automatically pop this parameter off.
   if (pushParameter == true)
   {
      //Make sure this function takes a single int parm.
      if ((fe->getNumberParameters() != 1) ||
         (fe->getVariable(0) == NULL) ||
         (fe->getVariable(0)->getType() != BXSVariableEntry::cIntegerVariable))
      {
         mMessenger->errorMsg("Error: Function '%s' is not a valid handler function (does not take a single int parm).", source->getSymbol(fe->getSymbolID()));
         data->resetAll();
         return(false);
      }
      //Push the parm on.
      if (data->pushVariableOn(-1, BXSVariableEntry::cIntegerVariable, BXSVariableEntry::cIntegerDataSize, &parameter) == false)
      {
         mMessenger->errorMsg("Error: failed to push '%d' parameter on stack before interp.", parameter);
         data->resetAll();
         return(false);
      }
   }
   //Push the function on now.  This creates the activation record
   //for this call.
   if (data->pushFunctionOn(fe, fe->getCodeOffset(), oldStack, oldHeap) == false)
   {
      mMessenger->errorMsg("Error: failed to setup function on stack.");
      data->resetAll();
      return(false);
   }

   //Do the interp.
   long evalResult=interpretLowLevel(source, data, false);

   //Return.
   if (evalResult == cEvaluateFail)
      return(false);
   return(true);
}

//==============================================================================
// BXSInterpreter::interpretFunction
//==============================================================================
bool BXSInterpreter::interpretFunction(BXSSource *source, BXSData *data, const char *functionName,
   long parameter, bool pushParameter)
{
   //Bomb check.
   if (data == NULL)
   {
      mMessenger->errorMsg("Error: data is NULL.");
      return(false);
   }
   if (functionName == NULL)
   {
      mMessenger->errorMsg("Error: function name is NULL.");
      return(false);
   }

   //Get the function ID (the value of the symbol in the data).
   long functionID=data->getFunctionID(functionName);
   return(interpretFunction(source, data, functionID, parameter, pushParameter));
}

//==============================================================================
// BXSInterpreter::interpretRestart
//==============================================================================
bool BXSInterpreter::interpretRestart(BXSSource *source, BXSData *data, long singleStep)
{
   //Bomb check.
   if (data == NULL)
   {
      mMessenger->errorMsg("Error: data is NULL.");
      return(false);
   }
   //DCP 08/27/02:  This needs to be commented out so that the saved half-done
   //trigger rule execution can call it w/o being in a breakpoint.  Fix this later:)
   //Make sure we're currently in a breakpoint.
   /*if (data->getBreakpoint() == false)
   {
      BASSERT(0);
      return(false);
   }*/

   //Do the interp.
   long evalResult=interpretLowLevel(source, data, singleStep);

   //If we aren't in a breakpoint anymore, unset the current rule.
   if (data->getBreakpoint() == false)
      data->setCurrentRuleID(-1);

   //Return.
   if (evalResult == cEvaluateFail)
      return(false);
   return(true);
}

//==============================================================================
// BXSInterpreter::interpretLowLevel
//==============================================================================
long BXSInterpreter::interpretLowLevel(BXSSource *source, BXSData *data, long singleStep)
{
   //Increment the data's interpret counter.
   data->incrementInterpretCounter();

   //We sit in a simple while loop here.  When we no longer have anything on
   //the callstack, we exit this loop and deinit the heap.  This loop also lets
   //us return out in the middle if we hit an error or breakpoint.   If we hit
   //a breakpoint, when we restart, we'll jump back into this loop to pickup
   //where we left off.  This while loop is important as it lets us pickup in
   //a non top-level function and then return to interpreting the calling function.
   while (data->getCallStackSize() > 0)
   {
      //Do the eval.
      long evalResult=evaluateCode(source, data, singleStep);

      //If we failed, handle that.
      if (evalResult == cEvaluateFail)
      {
         //Grab the function entry for the top of the call stack.  If we had an error,
         //the function the error occured in should be at the top of the callstack.
         BXSFileEntry *fileEntry=NULL;
         if (data->getCallStackSize() > 0)
         {
            BXSFunctionEntry *functionEntry=data->getFunctionEntry(data->getCallStack()[data->getCallStackSize()-1].getFunctionID());
            if (functionEntry != NULL)
               fileEntry=source->getFileEntry(functionEntry->getFileID());
         }

         //We print this out with a +1 to the line number because VC and most other things don't
         //start with "Line 0".
         if ((data->getCurrentLineNumber() >= 0) && (fileEntry != NULL))
            mMessenger->sourceErrorMsg(fileEntry->getFilename(), data->getCurrentLineNumber()+1, "Error: failed evaluating code on line number %d.", data->getCurrentLineNumber()+1);
         else if (data->getCurrentLineNumber() >= 0)
            mMessenger->sourceErrorMsg(NULL, data->getCurrentLineNumber()+1, "Error: failed evaluating code on line number %d.", data->getCurrentLineNumber()+1);
         else if (fileEntry != NULL)
            mMessenger->sourceErrorMsg(fileEntry->getFilename(), -1, "Error: failed evaluating code on an indeterminate source line.");
         else
            mMessenger->errorMsg("Error: failed evaluating code on an indeterminate source line.");

         data->resetAll();
         return(cEvaluateFail);
      }
      //Else, if we hit a breakpoint, return out IMMEDIATELY.
      else if (evalResult == cEvaluateBreakpoint)
         return(cEvaluateBreakpoint);
   }

   return(cEvaluateSuccess);
}

//==============================================================================
// BXSInterpreter::evaluateCode
//==============================================================================
long BXSInterpreter::evaluateCode(BXSSource *source, BXSData *data, long singleStep)
{
   //Establish the start code point.
   SBYTE *code=source->getCode()+data->getPC();
   //Skip the section opcode.
   SBYTE opcode=*code;

   //Display the heap and stack.
   #ifdef DEBUGINTERPRET
   if ((data->getBreakpoint() == false) && (mMessenger->getListInterpreter() == true))
   {
      mMessenger->runMsg("Initial? Stack and Heap:");
      displayStackAndHeap(source, data);
   }
   #endif

   //Go into the big nasty loop that interprets the opcodes.  Ugh.
   for (;;)
   {
      //Make sure PC doesn't go outside the bounds of our code.
      if ((data->getPC() < 0) || (data->getPC() > source->getCodeSize()) )
      {
         mMessenger->errorMsg("Error: evaluateCode PC=%d (Code goes from 0 to %d), stopping.", data->getPC(), source->getCodeSize());
         break;
      }

      //Break out if necessary.
      if (data->checkAndClearBreakNow())
      {
         data->setBreakpoint(true);
         return(cEvaluateBreakpoint);
      }

      //Update code pointer.
      code=source->getCode()+data->getPC();
      //Get the opcode.
      opcode=*code;
      //Increment the PC.
      data->incrementPC(1);
      //Debug.
      #ifdef DEBUGINTERPRET
      displayOpcodeDebugMessage(opcode, data->getPC());
      #endif

      //Interpret the opcode.
      switch (opcode)
      {
         //NOP doesn't do anything.
         case BXSQuadOpcode::cNOP:
            break;
      

         //RET.  Return.
         case BXSQuadOpcode::cRET:
         {
            //Get the function's entry.
            const BXSFunctionEntry *fe=data->getFunctionEntry(data->getCurrentFunctionID());
            if (fe == NULL)
               return(cEvaluateFail);

            //Get the return value off of the heap (if we have one).
            if (fe->getReturnType() != BXSVariableEntry::cVoidVariable)
            {
               //Make a temp copy of the return value.
               long returnValueIndex=data->getStack()[data->getStack().getNumber()-1];
               long returnValueType=*(long*)(data->getHeap().getPtr()+returnValueIndex+BXSData::cHeapTypeOffset);
               long returnValueSize=*(long*)(data->getHeap().getPtr()+returnValueIndex+BXSData::cHeapLengthOffset);
               //If we need to allocate a bigger rv holder, do it.
               if (data->allocateTempReturnValue(returnValueSize) == false)
                  return(cEvaluateFail);

               data->getHeap().copyPointerFrom(data->getTempReturnValue(), returnValueIndex+BXSData::cHeapOverheadSize, returnValueSize);

               //Pop the function off.
               //Note: there is processing within popFunctionOff that handles the "step over" 
               //functionality (as well as in the cLINE processing).
               if (data->popFunctionOff() == false)
                  return(cEvaluateFail);

               //Push the return value onto the stack and heap (now that we've cleared it
               //off from the function).  Not super efficient from a memory standpoint, but it
               //works.
               if (data->pushVariableOn(-1, returnValueType, returnValueSize, data->getTempReturnValue()) == false)
                  return(cEvaluateFail);

               //Debugger return code message.
               if ((mMessenger->getDebuggerInterpreter() == true) && (data->getBreakpoint() == true))
               {
                  char buffer[256] = { 0 };
                  switch (returnValueType)
                  {
                     case BXSVariableEntry::cIntegerVariable:
                        bsnprintf(buffer, sizeof(buffer), "'%s' return value: %d.", source->getSymbol(fe->getSymbolID()), *(long*)data->getTempReturnValue());
                        break;
                     case BXSVariableEntry::cFloatVariable:
                        bsnprintf(buffer, sizeof(buffer), "'%s' return value: %f.", source->getSymbol(fe->getSymbolID()), readFloat(data->getTempReturnValue()));
                        break;
                     case BXSVariableEntry::cBoolVariable:
                        bsnprintf(buffer, sizeof(buffer), "'%s' return value: %d.", source->getSymbol(fe->getSymbolID()), *(bool*)data->getTempReturnValue());
                        break;
                     case BXSVariableEntry::cStringVariable:
                     {
                        long stringIndex=*(long*)data->getTempReturnValue();
                        char *string=(char*)data->getUserHeapValue(stringIndex);
                        bsnprintf(buffer, sizeof(buffer), "'%s' return value: %s.", source->getSymbol(fe->getSymbolID()), string);
                        break;
                     }
                     case BXSVariableEntry::cVectorVariable:
                        bsnprintf(buffer, sizeof(buffer), "'%s' return value: (%f, %f, %f).", source->getSymbol(fe->getSymbolID()),
                           readFloat(data->getTempReturnValue()),
                           readFloat(data->getTempReturnValue()+sizeof(float)),
                           readFloat(data->getTempReturnValue()+sizeof(float)*2) );
                        break;
                  }
                  mMessenger->debuggerMsg(buffer);
               }
            }
            else
            {
               //Debugger return code message.
               if ((mMessenger->getDebuggerInterpreter() == true) && (data->getBreakpoint() == true))
                  mMessenger->debuggerMsg("'%s' return value: Void.", source->getSymbol(fe->getSymbolID()));
               
               //Pop the function off.
               data->popFunctionOff();
            }

            //Exit log message.
            if (mMessenger->getListFunctionEntry() == true)
               mMessenger->infoMsg("Exit '%s' function.", source->getSymbol(fe->getSymbolID()) );

            //If we're supposed to break, do that.
            //DCP 12/04/02: Don't recall why this is commented out.
            /*
            if(evaluateBreak)
            {
               data->setBreakpoint(true);
               return(cEvaluateBreakpoint);
            }
            */
            
            return(cEvaluateSuccess);
         }


         //JUMP.
         case BXSQuadOpcode::cJUMP:
         {
            //Parse the infinite loop check.
            long checkInfiniteLoop=0;
            if (data->parseByte(&checkInfiniteLoop) == false)
               return(cEvaluateFail);
            //Parse the jump address out.
            long newPCValue=-1;
            if (data->parseLong(&newPCValue) == false)
               return(cEvaluateFail);

            //If we're checking infinite loops on this JUMP and we have valid loop
            //limit set, check it.  We simply make sure that we're not jumping to the
            //same PC value time after time.  If we're over the limit, we pop a warning
            //and just don't set the PC.  This takes us past the problematic code block.
            if ((checkInfiniteLoop == 1) && (data->getInfiniteLoopLimit() > 0))
            {
               if (newPCValue == data->getInfiniteLoopPCJumpValue())
               {
                  data->setInfiniteLoopPCJumpCount(data->getInfiniteLoopPCJumpCount()+1);
                  if (data->getInfiniteLoopPCJumpCount() >= data->getInfiniteLoopLimit())
                  {
                     //Debugger/Warning message.
                     if (mMessenger->getDebuggerInterpreter() == true)
                        mMessenger->debuggerMsg("Warning: Possible infinite loop jump to PC=%d avoided.", newPCValue);
                     else
                        mMessenger->warningMsg("Warning: Possible infinite loop jump to PC=%d avoided.", newPCValue);
                  }
                  else
                     data->setPC(newPCValue);
               }
               else
               {
                  //Since we don't have the same PC, we reset the count.
                  data->setInfiniteLoopPCJumpCount(0);
                  data->setPC(newPCValue);
               }

               //Save the last PC jump value.
               data->setInfiniteLoopPCJumpValue(data->getPC());
            }
            //Else, just set the new PC value.
            else
               data->setPC(newPCValue);

            break;
         }
         //JUMPZ and JUMPNZ.
         case BXSQuadOpcode::cJUMPZ:
         case BXSQuadOpcode::cJUMPNZ:
         {
            long popStack=0;
            if (data->parseByte(&popStack) == false)
               return(cEvaluateFail);
            long newPCValue=-1;
            if (data->parseLong(&newPCValue) == false)
               return(cEvaluateFail);

            //Grab the bool value on the top of the stack.  If it's false, do the jump.
            long variableOffset=data->getStack()[data->getStack().getNumber()-1];
            long variableType=*(long*)(data->getHeap().getPtr()+variableOffset+BXSData::cHeapTypeOffset);
            if (variableType != BXSVariableEntry::cBoolVariable)
            {
               BASSERT(0);
               return(cEvaluateFail);
            }
            bool variableValue=*(bool*)(data->getHeap().getPtr()+variableOffset+BXSData::cHeapDataOffset);
            if ( ((opcode == BXSQuadOpcode::cJUMPZ) && (variableValue == false)) ||
               ((opcode == BXSQuadOpcode::cJUMPNZ) && (variableValue == true)) )
               data->setPC(newPCValue);

            //Pop the stack if we're supposed to.
            if (popStack != 0)
               data->popVariableOff();

            break;
         }
         //LABEL.
         case BXSQuadOpcode::cLABEL:
            break;


         //PUSH.  Push a variable onto the heap/stack given a variable index.
         case BXSQuadOpcode::cPUSH:
         {
            long var=0;
            if (data->parseVariable(&var) == false)
               return(cEvaluateFail);
            long varOffset=0;
            if (data->parseVariable(&varOffset) == false)
               return(cEvaluateFail);

            //Get the stack start from the top of the callstack's AR.
            long stackStart=data->getTopStackStart();

            //Calculate the heap index to copy from.
            long heapIndex;
            if (var >= 0)
               heapIndex=data->getStack()[stackStart+var];
            else
               heapIndex=data->getStack()[-var-1];

            //Push a full copy of the variable at the given address.
            if (data->pushVariableOn(heapIndex) == false)
               return(cEvaluateFail);
            break;
         }
         //PUSHADD.  Push a variable's address onto the stack.
         case BXSQuadOpcode::cPUSHADD:
         {
            long var=0;
            if (data->parseVariable(&var) == false)
               return(cEvaluateFail);
            long varOffset=0;
            if (data->parseVariable(&varOffset) == false)
               return(cEvaluateFail);

            //Get the stack start from the top of the callstack's AR.
            long stackStart=data->getTopStackStart();

            //Calculate the heap index to copy from.
            long heapIndex;
            if (var >= 0)
               heapIndex=data->getStack()[stackStart+var];
            else
               heapIndex=data->getStack()[-var-1];

            if (data->getStack().add(heapIndex) == -1)
               return(cEvaluateFail);
            break;
         }
         //PUSHI.  Push an immediate variable value onto the heap/stack given a type and a value.
         case BXSQuadOpcode::cPUSHI:
         {
            long variableType;
            if (data->parseByte(&variableType) == false)
               return(cEvaluateFail);
            long variableValue;
            if (data->parseLong(&variableValue) == false)
               return(cEvaluateFail);

            //Push the variable.
            switch (variableType)
            {
               case BXSVariableEntry::cIntegerVariable:
               {
                  if (data->pushVariableOn(-1, BXSVariableEntry::cIntegerVariable, BXSVariableEntry::cIntegerDataSize, &variableValue) == false)
                     return(cEvaluateFail);
                  break;
               }
               case BXSVariableEntry::cFloatVariable:
               {
                  float vv=readFloat(&variableValue);
                  if (data->pushVariableOn(-1, BXSVariableEntry::cFloatVariable, BXSVariableEntry::cFloatDataSize, &vv) == false)
                     return(cEvaluateFail);
                  break;
               }
               case BXSVariableEntry::cBoolVariable:
               {
                  bool vv=true;
                  if (variableValue == 0)
                     vv=false;
                  if (data->pushVariableOn(-1, BXSVariableEntry::cBoolVariable, BXSVariableEntry::cBoolDataSize, &vv) == false)
                     return(cEvaluateFail);
                  break;
               }
            }

            break;
         }
         //POP.  Pops X things off of the stack and the heap.
         case BXSQuadOpcode::cPOP:
         {
            long numberToPop=0;
            if (data->parseByte(&numberToPop) == false)
               return(cEvaluateFail);
            for (long i=0; i < numberToPop; i++)
               data->popVariableOff();
            break;
         }
         //POPADD.  Pops X things off of JUST the stack (presuming they are addresses).
         case BXSQuadOpcode::cPOPADD:
         {
            long numberToPop=0;
            if (data->parseByte(&numberToPop) == false)
               return(cEvaluateFail);
            data->getStack().setNumber(data->getStack().getNumber()-numberToPop);
            break;
         }
         //CALLS.  Call a syscall.
         case BXSQuadOpcode::cCALLS:
         {
            //Get the ID.
            long syscallID=-1;
            if (data->parseSyscallIndex(&syscallID) == false)
               return(cEvaluateFail);
            //Get the function's entry.
            const BXSSyscallEntry *se=mSyscalls->getSyscall(syscallID);
            if (se == NULL)
               return(cEvaluateFail);

            //Remember the current stack and heap index.
            long savedStack=data->getStack().getNumber()-se->getNumberParameters();
            long savedHeap=-1;
            //If there are no parms on the stack, then the currentHeapIndex is
            //the size of the heap as it is now.
            if (savedStack == data->getStack().getNumber())
               savedHeap=data->getHeap().getNumber();
            //Else, the currentHeapIndex is the address of the first parm.
            else
               savedHeap=data->getStack()[savedStack];

            //Build the parm list.  We have to do this here in case the address of the heap variables
            //have moved since some of them got added.  If we have a simple type (e.g. long, float, or bool),
            //we just shove the value on the list.  Else, we shove the pointer.
            long syscallNumberArguments=se->getNumberParameters();
            long spi[BXSSyscallModule::cMaximumNumberSyscallParms+1];
            for (long i=0; i < syscallNumberArguments; i++)
            {
               long stackIndex=data->getStack().getNumber()-syscallNumberArguments+i;
               long heapIndex=data->getStack()[stackIndex];
               long *foo=(long*)(data->getHeap().getPtr()+heapIndex+BXSData::cHeapDataOffset);
               //Get the variable type.
               long variableType=*(long*)(data->getHeap().getPtr()+heapIndex+BXSData::cHeapTypeOffset);
               //Get the syscall variable type (as this is what we have to push).
               long syscallVariableType=se->getParameter(i)->getType();

               //If we have a string or vector type expected, but we don't have that, we done.
               if (syscallVariableType != variableType)
               {
                  if ((syscallVariableType == BXSVariableEntry::cStringVariable) ||
                     (syscallVariableType == BXSVariableEntry::cVectorVariable) ||
                     (variableType == BXSVariableEntry::cStringVariable) ||
                     (variableType == BXSVariableEntry::cVectorVariable))
                     return(cEvaluateFail);
               }

               //Do the switch on the syscall variable type.
               switch (syscallVariableType)
               {
                  case BXSVariableEntry::cIntegerVariable:
                     //If we don't have an integer variable type, convert before we save the value.
                     if (variableType == BXSVariableEntry::cFloatVariable)
                     {
                        float temp1=readFloat(data->getHeap().getPtr()+heapIndex+BXSData::cHeapDataOffset);
                        spi[i]=(long)temp1;
                     }
                     else if (variableType == BXSVariableEntry::cBoolVariable)
                     {
                        bool temp1=*(bool*)(data->getHeap().getPtr()+heapIndex+BXSData::cHeapDataOffset);
                        if (temp1 == false)
                           spi[i]=0;
                        else
                           spi[i]=1;
                     }
                     else
                        spi[i]=*foo;
                     break;
                  case BXSVariableEntry::cFloatVariable:
                     //If we don't have a float variable type, convert before we save the value.
                     if (variableType == BXSVariableEntry::cIntegerVariable)
                     {
                        float temp1=(float)*foo;
                        spi[i]=*((long*)&temp1);
                     }
                     else if (variableType == BXSVariableEntry::cBoolVariable)
                     {
                        bool temp1=*(bool*)(data->getHeap().getPtr()+heapIndex+BXSData::cHeapDataOffset);
                        float temp2;
                        if (temp1 == true)
                           temp2=1.0f;
                        else
                           temp2=0.0f;
                        spi[i]=*((long*)&temp2);
                     }
                     else
                        spi[i]=*foo;
                     break;
                  case BXSVariableEntry::cBoolVariable:
                     //If we don't have a bool variable type, convert before we save the value.
                     if(variableType == BXSVariableEntry::cBoolVariable)
                     {
                        bool temp1=*((bool*)foo);
                        spi[i]=(long)temp1;
                     }
                     else if (variableType == BXSVariableEntry::cFloatVariable)
                     {
                        float temp1=readFloat(data->getHeap().getPtr()+heapIndex+BXSData::cHeapDataOffset);
                        bool temp2=true;
                        if ((temp1 < cFloatCompareEpsilon) && (temp1 > -cFloatCompareEpsilon))
                           temp2=false;
                        spi[i]=(long)temp2;
                     }
                     else
                     {
                        bool temp1=true;
                        if (*foo == 0)
                           temp1=false;
                        spi[i]=(long)temp1;
                     }
                     break;

                  case BXSVariableEntry::cStringVariable:
                     foo=(long*)data->getUserHeapValue(*foo);
                     spi[i]=(long)foo;
                     break;
                  case BXSVariableEntry::cVectorVariable:
                     spi[i]=(long)foo;
                     break;
                  default:
                     BASSERT(0);
                     return(cEvaluateFail);
               }
            }

            //If we have a context syscall, add the data pointer as the last parm.
            if (se->getContext() == true)
            {
               if (syscallNumberArguments >= BXSSyscallModule::cMaximumNumberSyscallParms)
                  return(cEvaluateFail);
               spi[syscallNumberArguments]=(long)data;
               syscallNumberArguments++;
            }

            //Call the syscall (based on the return type).  If it's a syscall that returns something,
            //the return value will be copied into the destination on the heap (pointed to by the
            //'copyToIndex' variable.
            static char debuggerMsg[1024];
            switch (se->getReturnType())
            {
               //Void.
               case BXSVariableEntry::cVoidVariable:
               {
                  if (mSyscalls->callVoidSyscall(syscallID, spi, syscallNumberArguments) == false)
                     return(cEvaluateFail);
                  //Debugger return code message.
                  if ((mMessenger->getDebuggerInterpreter() == true) && (data->getBreakpoint() == true))
                     bsnprintf(debuggerMsg, 1024, "'%s' return value: Void.", source->getSymbol(se->getSymbolID()));
                  break;
               }
               //Bool.
               case BXSVariableEntry::cBoolVariable:
               {
                  bool boolResult=false;
                  if (mSyscalls->callBoolSyscall(syscallID, spi, syscallNumberArguments, &boolResult, data) == false)
                     return(cEvaluateFail);
                  //Debugger return code message.
                  if ((mMessenger->getDebuggerInterpreter() == true) && (data->getBreakpoint() == true))
                     bsnprintf(debuggerMsg, 1024, "'%s' return value: %d.", source->getSymbol(se->getSymbolID()), boolResult);
                  break;
               }
               //Integer.
               case BXSVariableEntry::cIntegerVariable:
               {
                  long intResult=0;
                  if (mSyscalls->callIntegerSyscall(syscallID, spi, syscallNumberArguments, &intResult, data) == false)
                     return(cEvaluateFail);
                  //Debugger return code message.
                  if ((mMessenger->getDebuggerInterpreter() == true) && (data->getBreakpoint() == true))
                     bsnprintf(debuggerMsg, 1024, "'%s' return value: %d.", source->getSymbol(se->getSymbolID()), intResult);
                  break;
               }
               //Float.
               case BXSVariableEntry::cFloatVariable:
               {
                  float floatResult=0.0f;
                  if (mSyscalls->callFloatSyscall(syscallID, spi, syscallNumberArguments, &floatResult, data) == false)
                     return(cEvaluateFail);
                  //Debugger return code message.
                  if ((mMessenger->getDebuggerInterpreter() == true) && (data->getBreakpoint() == true))
                     bsnprintf(debuggerMsg, 1024, "'%s' return value: %f.", source->getSymbol(se->getSymbolID()), floatResult);
                  break;
               }
               //String.
               case BXSVariableEntry::cStringVariable:
               {
                  if (mSyscalls->callStringSyscall(syscallID, spi, syscallNumberArguments, data) == false)
                     return(cEvaluateFail);
                  //Debugger return code message.
                  if ((mMessenger->getDebuggerInterpreter() == true) && (data->getBreakpoint() == true))
                     bsnprintf(debuggerMsg, 1024, "'%s' return value: %s.", source->getSymbol(se->getSymbolID()), data->getTempReturnValue());
                  break;
               }
               //Vector.
               case BXSVariableEntry::cVectorVariable:
               {
                  BVector vectorResult(0.0f);
                  if (mSyscalls->callVectorSyscall(syscallID, spi, syscallNumberArguments, &vectorResult, data) == false)
                     return(cEvaluateFail);
                  //Debugger return code message.
                  if ((mMessenger->getDebuggerInterpreter() == true) && (data->getBreakpoint() == true))
                     bsnprintf(debuggerMsg, 1024, "'%s' return value: (%f, %f, %f).", source->getSymbol(se->getSymbolID()), vectorResult.x, vectorResult.y, vectorResult.z);
                  break;
               }
               //Whoops.
               default:
                  BASSERT(0);
                  return(cEvaluateFail);
            }

            //Before we whack the stack and heap back to where they used to be, we need to
            //decrement the refs to any user heap values on the stack.
            for (long a=0; a < se->getNumberParameters(); a++)
            {
               long heapOffset=data->getStack()[data->getStack().getNumber()-1-a];
               long variableType=*(long*)(data->getHeap().getPtr()+heapOffset+BXSData::cHeapTypeOffset);
               if (variableType & (BXSVariableEntry::cStringVariable|BXSVariableEntry::cUserClassVariable) )
               {
                  long userHeapIndex=*(long*)(data->getHeap().getPtr()+heapOffset+BXSData::cHeapDataOffset);
                  data->decrementUserHeapRefCount(userHeapIndex);
               }
            }

            //Now, reset the stack pointer back to where it used to be before we called the function.
            //Ditto for the heap.
            data->getStack().setNumber(savedStack);
            data->getHeap().setNumber(savedHeap);

            //If we have a return type, push it on the stack.
            if (se->getReturnType() != BXSVariableEntry::cVoidVariable)
            {
               //Push the return value onto the stack and heap.
               if (data->pushVariableOn(-1, se->getReturnType(), data->getTempReturnValueSize(), data->getTempReturnValue()) == false)
                  return(cEvaluateFail);
            }

            //Spit out the debugger msg if we are supposed to.
            if ((mMessenger->getDebuggerInterpreter() == true) && (data->getBreakpoint() == true))
               mMessenger->debuggerMsg(debuggerMsg);

            break;
         }

         //CALLF.  Call an XS function.  This handles all return value setting and
         //popping, too.
         case BXSQuadOpcode::cCALLF:
         {
            //Get the function ID.
            long functionID=-1;
            if (data->parseSyscallIndex(&functionID) == false)
               return(cEvaluateFail);
            //Get the function's entry.
            const BXSFunctionEntry *fe=data->getFunctionEntry(functionID);
            if (fe == NULL)
               return(cEvaluateFail);

            //Remember the current stack and heap index.
            long savedStack=data->getStack().getNumber()-fe->getNumberParameters();
            long savedHeap=-1;
            //If there are no parms on the stack, then the currentHeapIndex is
            //the size of the heap as it is now.
            if (savedStack == data->getStack().getNumber())
               savedHeap=data->getHeap().getNumber();
            //Else, the currentHeapIndex is the address of the first parm.
            else
               savedHeap=data->getStack()[savedStack];

            //Push the function onto the stack.
            if (data->pushFunctionOn(fe, fe->getCodeOffset(), savedStack, savedHeap) == false)
            {
               mMessenger->errorMsg("Error: failed to add function to call stack.");
               return(cEvaluateFail);
            }

            //Entry log message.
            if (mMessenger->getListFunctionEntry() == true)
               mMessenger->infoMsg("Enter '%s' function.", source->getSymbol(fe->getSymbolID()) );

            //If we're checking for infinite recursion, do it.
            if ((data->getInfiniteRecursionLimit() > 0) && (checkInfiniteRecursion(data, fe->getID()) == true))
            {
               //Debugger/Warning message.
               if (mMessenger->getDebuggerInterpreter() == true)
                  mMessenger->debuggerMsg("Warning: Infinite recursion limit of %d hit, failing execution.", data->getInfiniteRecursionLimit());
               else
                  mMessenger->warningMsg("Warning: Infinite recursion limit of %d hit, failing execution.", data->getInfiniteRecursionLimit());
               return(cEvaluateFail);
            }

            //Make the function call.
            long evalResult=evaluateCode(source, data, singleStep);
            if (evalResult != cEvaluateSuccess)
               return(evalResult);
            break;
         }


         //BOOLEAN OPCODES.
         case BXSQuadOpcode::cNOT:
         {
            //Get the variable address and type.
            long variableAddress=data->getStack()[data->getStack().getNumber()-1];
            long variableType=*(long*)(data->getHeap().getPtr()+variableAddress+BXSData::cHeapTypeOffset);
            if (variableType != BXSVariableEntry::cBoolVariable)
               return(cEvaluateFail);
            //NOT the value.
            bool variableValue=*(bool*)(data->getHeap().getPtr()+variableAddress+BXSData::cHeapDataOffset);
            variableValue=!variableValue;
            //Copy the value back over the old value.
            data->getHeap().copyPointerInto((BYTE*)&variableValue, variableAddress+BXSData::cHeapDataOffset, BXSVariableEntry::cBoolDataSize);
            break;
         }
         case BXSQuadOpcode::cAND:
         case BXSQuadOpcode::cOR:
         {
            //Get the variable addresses and types.
            long yAddress=data->getStack()[data->getStack().getNumber()-2];
            long zAddress=data->getStack()[data->getStack().getNumber()-1];
            long yType=*(long*)(data->getHeap().getPtr()+yAddress+BXSData::cHeapTypeOffset);
            long zType=*(long*)(data->getHeap().getPtr()+zAddress+BXSData::cHeapTypeOffset);
            if ((yType != BXSVariableEntry::cBoolVariable) || (zType != BXSVariableEntry::cBoolVariable))
               return(cEvaluateFail);
            //Get the operand values.
            bool yValue=*(bool*)(data->getHeap().getPtr()+yAddress+BXSData::cHeapDataOffset);
            bool zValue=*(bool*)(data->getHeap().getPtr()+zAddress+BXSData::cHeapDataOffset);

            bool result=true;
            if (opcode == BXSQuadOpcode::cAND)
            {
               if ((yValue == false) || (zValue == false))
                  result=false;
            }
            else
            {
               if ((yValue == false) && (zValue == false))
                  result=false;
            }

            //Pop the two operands off.
            data->popVariableOff();
            data->popVariableOff();

            //Push the result value.
            if (data->pushVariableOn(-1, BXSVariableEntry::cBoolVariable, BXSVariableEntry::cBoolDataSize, &result) == false)
               return(cEvaluateFail);

            break;
         }


         //COMPARISION OPCODES.  Looks at the top two values on the stack and compares
         //them.  Pops them off when done and pushes a bool (true or false).
         case BXSQuadOpcode::cGT:
         case BXSQuadOpcode::cGE:
         case BXSQuadOpcode::cNE:
         case BXSQuadOpcode::cEQ:
         case BXSQuadOpcode::cLE:
         case BXSQuadOpcode::cLT:
         {
            long yOffset=data->getStack()[data->getStack().getNumber()-2];
            long zOffset=data->getStack()[data->getStack().getNumber()-1];

            //Get the types of the variables.
            long yType=*(long*)(data->getHeap().getPtr()+yOffset+BXSData::cHeapTypeOffset);
            long zType=*(long*)(data->getHeap().getPtr()+zOffset+BXSData::cHeapTypeOffset);

            //Result.
            bool result=false;
            
            //Ugly switch to do the compare.  This also error checks to avoid
            //incompatible checks.
            switch (yType)
            {
               case BXSVariableEntry::cIntegerVariable:
               {
                  if ((zType != BXSVariableEntry::cIntegerVariable) && (zType != BXSVariableEntry::cFloatVariable))
                     return(cEvaluateFail);
                  long yVal=*(long*)(data->getHeap().getPtr()+yOffset+BXSData::cHeapDataOffset);
                  long zVal=0;
                  if (zType == BXSVariableEntry::cIntegerVariable)
                     zVal=*(long*)(data->getHeap().getPtr()+zOffset+BXSData::cHeapDataOffset);
                  else
                  {
                     float tzv=readFloat(data->getHeap().getPtr()+zOffset+BXSData::cHeapDataOffset);
                     zVal=(long)tzv;
                  }
                  if (compareIntegers(yVal, zVal, &result, opcode) == false)
                     return(cEvaluateFail);
                  break;
               }
               case BXSVariableEntry::cFloatVariable:
               {
                  if ((zType != BXSVariableEntry::cIntegerVariable) && (zType != BXSVariableEntry::cFloatVariable))
                     return(cEvaluateFail);
                  float yVal=readFloat(data->getHeap().getPtr()+yOffset+BXSData::cHeapDataOffset);
                  float zVal=0;
                  if (zType == BXSVariableEntry::cFloatVariable)
                     zVal=readFloat(data->getHeap().getPtr()+zOffset+BXSData::cHeapDataOffset);
                  else
                  {
                     long tzv=*(long*)(data->getHeap().getPtr()+zOffset+BXSData::cHeapDataOffset);
                     zVal=(float)tzv;
                  }
                  if (compareFloats(yVal, zVal, &result, opcode) == false)
                     return(cEvaluateFail);
                  break;
               }
               case BXSVariableEntry::cBoolVariable:
               {
                  if (zType != BXSVariableEntry::cBoolVariable)
                     return(cEvaluateFail);
                  bool yVal=*(bool*)(data->getHeap().getPtr()+yOffset+BXSData::cHeapDataOffset);
                  bool zVal=*(bool*)(data->getHeap().getPtr()+zOffset+BXSData::cHeapDataOffset);
                  if (compareBools(yVal, zVal, &result, opcode) == false)
                     return(cEvaluateFail);
                  break;
               }
               case BXSVariableEntry::cStringVariable:
               {
                  if (zType != BXSVariableEntry::cStringVariable)
                     return(cEvaluateFail);

                  long yIndex=*(long*)(data->getHeap().getPtr()+yOffset+BXSData::cHeapDataOffset);
                  long zIndex=*(long*)(data->getHeap().getPtr()+zOffset+BXSData::cHeapDataOffset);
                  char *yVal=(char*)(data->getUserHeapValue(yIndex));
                  char *zVal=(char*)(data->getUserHeapValue(zIndex));

                  if (compareStrings(yVal, zVal, &result, opcode) == false)
                     return(cEvaluateFail);
                  break;
               }
               case BXSVariableEntry::cVectorVariable:
               {
                  if (zType != BXSVariableEntry::cVectorVariable)
                     return(cEvaluateFail);
                  BVector *yVal=(BVector*)(data->getHeap().getPtr()+yOffset+BXSData::cHeapDataOffset);
                  BVector *zVal=(BVector*)(data->getHeap().getPtr()+zOffset+BXSData::cHeapDataOffset);
                  if (compareVectors(yVal, zVal, &result, opcode) == false)
                     return(cEvaluateFail);
                  break;
               }
            }

            //Pop the two full variables off of the stack.
            data->popVariableOff();
            data->popVariableOff();

            //Push the result on.
            if (data->pushVariableOn(-1, BXSVariableEntry::cBoolVariable, BXSVariableEntry::cBoolDataSize, &result) == false)
               return(cEvaluateFail);

            break;
         }

         //MATH MANIPULATION OPCODES.
         //Unary Math opcodes.
         case BXSQuadOpcode::cNEG:
         {
            long variableAddress=data->getStack()[data->getStack().getNumber()-1];
            long variableType=*(long*)(data->getHeap().getPtr()+variableAddress+BXSData::cHeapTypeOffset);
            long variableSize=*(long*)(data->getHeap().getPtr()+variableAddress+BXSData::cHeapLengthOffset);

            //We can only NEG integers, floats, and vectors.
            if (variableType == BXSVariableEntry::cIntegerVariable)
            {
               long variableValue=*(long*)(data->getHeap().getPtr()+variableAddress+BXSData::cHeapDataOffset);
               variableValue=-variableValue;
               data->getHeap().copyPointerInto((BYTE*)&variableValue, variableAddress+BXSData::cHeapDataOffset, variableSize);
            }
            else if (variableType == BXSVariableEntry::cFloatVariable)
            {
               float variableValue=readFloat(data->getHeap().getPtr()+variableAddress+BXSData::cHeapDataOffset);
               variableValue=-variableValue;
               data->getHeap().copyPointerInto((BYTE*)&variableValue, variableAddress+BXSData::cHeapDataOffset, variableSize);
            }
            else if (variableType == BXSVariableEntry::cVectorVariable)
            {
               BVector *variableValue=(BVector*)(data->getHeap().getPtr()+variableAddress+BXSData::cHeapDataOffset);
               variableValue->x=-variableValue->x;
               variableValue->y=-variableValue->y;
               variableValue->z=-variableValue->z;
               data->getHeap().copyPointerInto((BYTE*)variableValue, variableAddress+BXSData::cHeapDataOffset, variableSize);
            }
            else
               return(cEvaluateFail);

            break;
         }
         //Binary Math opcodes.
         case BXSQuadOpcode::cADD:
         case BXSQuadOpcode::cSUB:
         case BXSQuadOpcode::cMUL:
         case BXSQuadOpcode::cDIV:
         case BXSQuadOpcode::cMOD:
         {
            long yOffset=data->getStack()[data->getStack().getNumber()-2];
            long zOffset=data->getStack()[data->getStack().getNumber()-1];

            //Get the types of the variables.
            long yType=*(long*)(data->getHeap().getPtr()+yOffset+BXSData::cHeapTypeOffset);
            long zType=*(long*)(data->getHeap().getPtr()+zOffset+BXSData::cHeapTypeOffset);

            //Vector.
            if (yType == BXSVariableEntry::cVectorVariable)
            {
               BVector result(0.0f);
               BVector *yVal=(BVector*)(data->getHeap().getPtr()+yOffset+BXSData::cHeapDataOffset);
               switch (zType)
               {
                  //Vector and Integer (cast integer to float).
                  case BXSVariableEntry::cIntegerVariable:
                  {
                     long tempZVal=*(long*)(data->getHeap().getPtr()+zOffset+BXSData::cHeapDataOffset);
                     float zVal=(float)tempZVal;
                     if (calculateVectorResult(yVal, zVal, &result, opcode) == false)
                        return(cEvaluateFail);
                     break;
                  }
                  //Vector and Float.
                  case BXSVariableEntry::cFloatVariable:
                  {
                     float zVal=readFloat(data->getHeap().getPtr()+zOffset+BXSData::cHeapDataOffset);
                     if (calculateVectorResult(yVal, zVal, &result, opcode) == false)
                        return(cEvaluateFail);
                     break;
                  }
                  //Vector and Vector.
                  case BXSVariableEntry::cVectorVariable:
                  {
                     BVector *zVal=(BVector*)(data->getHeap().getPtr()+zOffset+BXSData::cHeapDataOffset);
                     if (calculateVectorResult(yVal, zVal, &result, opcode) == false)
                        return(cEvaluateFail);
                     break;
                  }
                  //Error.
                  default:
                     return(cEvaluateFail);
               }

               //Pop the operands.
               data->popVariableOff();
               data->popVariableOff();
               //Push the result.
               if (data->pushVariableOn(-1, BXSVariableEntry::cVectorVariable, BXSVariableEntry::cVectorDataSize, &result) == false)
                  return(cEvaluateFail);
            }
            //String.  Strings automatically convert other args to a string format during an add.
            else if ((yType == BXSVariableEntry::cStringVariable) || (zType == BXSVariableEntry::cStringVariable))
            {
               if (opcode != BXSQuadOpcode::cADD)
                  return(cEvaluateFail);

               //Get the string values.  Do the auto conversion if necessary.
               char *yString=NULL;
               char *zString=NULL;
               static char tempYString[BXSVariableEntry::cTempStringConversionMaxSize];
               static char tempZString[BXSVariableEntry::cTempStringConversionMaxSize];
               //First operand.
               if (yType == BXSVariableEntry::cStringVariable)
               {
                  long yIndex=*(long*)(data->getHeap().getPtr()+yOffset+BXSData::cHeapDataOffset);
                  yString=(char*)(data->getUserHeapValue(yIndex));
               }
               else
               {
                  switch (yType)
                  {
                     case BXSVariableEntry::cIntegerVariable:
                        bsnprintf(tempYString, sizeof(tempYString), "%d", *(long*)(data->getHeap().getPtr()+yOffset+BXSData::cHeapDataOffset));
                        break;
                     case BXSVariableEntry::cFloatVariable:
                        bsnprintf(tempYString, sizeof(tempYString), "%f", readFloat(data->getHeap().getPtr()+yOffset+BXSData::cHeapDataOffset));
                        break;
                     case BXSVariableEntry::cBoolVariable:
                        bsnprintf(tempYString, sizeof(tempYString), "%d", *(bool*)(data->getHeap().getPtr()+yOffset+BXSData::cHeapDataOffset));
                        break;
                     case BXSVariableEntry::cVectorVariable:
                     {
                        BVector foo=*(BVector*)(data->getHeap().getPtr()+yOffset+BXSData::cHeapDataOffset);
                        bsnprintf(tempYString, sizeof(tempYString), "(%f, %f, %f)", foo.x, foo.y, foo.z);
                        break;
                     }
                     default:
                        return(cEvaluateFail);
                  }
                  yString=tempYString;
               }
               //Second operand.
               if (zType == BXSVariableEntry::cStringVariable)
               {
                  long zIndex=*(long*)(data->getHeap().getPtr()+zOffset+BXSData::cHeapDataOffset);
                  zString=(char*)(data->getUserHeapValue(zIndex));
               }
               else
               {
                  switch (zType)
                  {
                     case BXSVariableEntry::cIntegerVariable:
                        bsnprintf(tempZString, sizeof(tempZString), "%d", *(long*)(data->getHeap().getPtr()+zOffset+BXSData::cHeapDataOffset));
                        break;
                     case BXSVariableEntry::cFloatVariable:
                        bsnprintf(tempZString, sizeof(tempZString), "%f", readFloat(data->getHeap().getPtr()+zOffset+BXSData::cHeapDataOffset));
                        break;
                     case BXSVariableEntry::cBoolVariable:
                        bsnprintf(tempZString, sizeof(tempZString), "%d", *(bool*)(data->getHeap().getPtr()+zOffset+BXSData::cHeapDataOffset));
                        break;
                     case BXSVariableEntry::cVectorVariable:
                     {
                        BVector foo=*(BVector*)(data->getHeap().getPtr()+zOffset+BXSData::cHeapDataOffset);
                        bsnprintf(tempZString, sizeof(tempZString), "(%f, %f, %f)", foo.x, foo.y, foo.z);
                        break;
                     }
                     default:
                        return(cEvaluateFail);
                  }
                  zString=tempZString;
               }

               //Ugh.  Do the add.
               long yLength=strlen(yString);
               long zLength=strlen(zString);
               long bufSize = yLength+zLength+1;
               if (data->allocateTempReturnValue(bufSize) == false)
                  return(cEvaluateFail);
               StringCchCopyNExA((char*)data->getTempReturnValue(), bufSize, yString, yLength+1, NULL, NULL, STRSAFE_FILL_BEHIND_NULL);
               StringCchCatNA((char*)data->getTempReturnValue(), bufSize, zString, zLength+1);
               //Now save it (again).
               long newValueIndex=data->allocateUserHeapValue(BXSVariableEntry::cStringVariable, yLength+zLength+1);
               if (newValueIndex < 0)
                  return(cEvaluateFail);
               if (data->setUserHeapValue(newValueIndex, BXSVariableEntry::cStringVariable, yLength+zLength+1, data->getTempReturnValue()) == false)
                  return(cEvaluateFail);
               if (data->incrementUserHeapRefCount(newValueIndex) == false)
                  return(cEvaluateFail);

               //Pop the operands.
               data->popVariableOff();
               data->popVariableOff();
               //Push the result.
               if (data->pushVariableOn(-1, BXSVariableEntry::cStringVariable, sizeof(long), &newValueIndex) == false)
                  return(cEvaluateFail);
            }
            //Float.
            else if (yType == BXSVariableEntry::cFloatVariable)
            {
               float result=0.0f;
               float yVal=readFloat(data->getHeap().getPtr()+yOffset+BXSData::cHeapDataOffset);
               float zVal=0.0f;
               if (zType == BXSVariableEntry::cIntegerVariable)
               {
                  long tempZVal=*(long*)(data->getHeap().getPtr()+zOffset+BXSData::cHeapDataOffset);
                  zVal=(float)tempZVal;
               }
               else if (zType == BXSVariableEntry::cFloatVariable)
                  zVal=readFloat(data->getHeap().getPtr()+zOffset+BXSData::cHeapDataOffset);
               else
                  return(cEvaluateFail);

               if (calculateFloatResult(yVal, zVal, &result, opcode) == false)
                  return(cEvaluateFail);

               //Pop the operands.
               data->popVariableOff();
               data->popVariableOff();
               //Push the result.
               if (data->pushVariableOn(-1, BXSVariableEntry::cFloatVariable, BXSVariableEntry::cFloatDataSize, &result) == false)
                  return(cEvaluateFail);
            }
            //Int.
            else if (yType == BXSVariableEntry::cIntegerVariable)
            {
               long result=0;
               long yVal=*(long*)(data->getHeap().getPtr()+yOffset+BXSData::cHeapDataOffset);
               if (zType == BXSVariableEntry::cIntegerVariable)
               {
                  long zVal=*(long*)(data->getHeap().getPtr()+zOffset+BXSData::cHeapDataOffset);
                  if (calculateIntegerResult(yVal, zVal, &result, opcode) == false)
                     return(cEvaluateFail);
               }
               else if (zType == BXSVariableEntry::cFloatVariable)
               {
                  float zVal=readFloat(data->getHeap().getPtr()+zOffset+BXSData::cHeapDataOffset);
                  float tempResult=0.0f;
                  if (calculateFloatResult((float)yVal, zVal, &tempResult, opcode) == false)
                     return(cEvaluateFail);
                  result=(long)tempResult;
               }
               else
                  return(cEvaluateFail);

               //Pop the operands.
               data->popVariableOff();
               data->popVariableOff();
               //Push the result.
               if (data->pushVariableOn(-1, BXSVariableEntry::cIntegerVariable, BXSVariableEntry::cIntegerDataSize, &result) == false)
                  return(cEvaluateFail);
            }
            else
               return(cEvaluateFail);
            break;
         }
         //ASSIGNMENT OPCODE.  This assigns a value to a variable, handling any necessary/allowed
         //type conversion during the process.
         case BXSQuadOpcode::cASS:
         {
            long variableAddress=data->getStack()[data->getStack().getNumber()-2];
            long valueAddress=data->getStack()[data->getStack().getNumber()-1];

            //Get the copy from data.
            long copyFromIndex=valueAddress+BXSData::cHeapDataOffset;
            long copyFromSize=*(long*)(data->getHeap().getPtr()+valueAddress+BXSData::cHeapLengthOffset);
            long copyFromType=*(long*)(data->getHeap().getPtr()+valueAddress+BXSData::cHeapTypeOffset);

            //Calculate the index of the given variable.
            long copyToIndex=variableAddress+BXSData::cHeapDataOffset;
            long copyToType=*(long*)(data->getHeap().getPtr()+variableAddress+BXSData::cHeapTypeOffset);

            //If the types are the same, we're good.
            if (copyFromType == copyToType)
            {
               //If we have a string, decrement the old index ref count and increment the new one.
               if (copyFromType & (BXSVariableEntry::cStringVariable|BXSVariableEntry::cUserClassVariable) )
               {
                  long oldUHIndex=*(long*)(data->getHeap().getPtr()+copyToIndex);
                  long newUHIndex=*(long*)(data->getHeap().getPtr()+copyFromIndex);
                  if (data->decrementUserHeapRefCount(oldUHIndex) == false)
                     return(false);
                  if (data->incrementUserHeapRefCount(newUHIndex) == false)
                     return(false);
               }

               data->getHeap().copyPointerInto(data->getHeap().getPtr()+copyFromIndex, copyToIndex, copyFromSize);
               break;
            }
            //Strings and Vectors and User Variables have to be of the same type.
            if ( (copyFromType & (BXSVariableEntry::cStringVariable|BXSVariableEntry::cVectorVariable|BXSVariableEntry::cUserClassVariable)) &&
               (copyFromType != copyToType))
               return(cEvaluateFail);
            //Integers, Floats, and Bools can all be converted between each other.  The conversion
            //has to happen before the assignment, though.
            if (copyToType == BXSVariableEntry::cIntegerVariable)
            {
               if (copyFromType == BXSVariableEntry::cFloatVariable)
               {
                  float temp1=readFloat(data->getHeap().getPtr()+copyFromIndex);
                  long temp2=(long)temp1;
                  data->getHeap().copyPointerInto((BYTE*)&temp2, copyToIndex, BXSVariableEntry::cIntegerDataSize);
               }
               else if (copyFromType == BXSVariableEntry::cBoolVariable)
               {
                  bool temp1=*(bool*)(data->getHeap().getPtr()+copyFromIndex);
                  long temp2=(long)temp1;
                  data->getHeap().copyPointerInto((BYTE*)&temp2, copyToIndex, BXSVariableEntry::cIntegerDataSize);
               }
               else
                  return(cEvaluateFail);
               break;
            }
            if (copyToType == BXSVariableEntry::cFloatVariable)
            {
               if (copyFromType == BXSVariableEntry::cIntegerVariable)
               {
                  long temp1=*(long*)(data->getHeap().getPtr()+copyFromIndex);
                  float temp2=(float)temp1;
                  data->getHeap().copyPointerInto((BYTE*)&temp2, copyToIndex, BXSVariableEntry::cFloatDataSize);
               }
               else if (copyFromType == BXSVariableEntry::cBoolVariable)
               {
                  bool temp1=*(bool*)(data->getHeap().getPtr()+copyFromIndex);
                  float temp2=(float)temp1;
                  data->getHeap().copyPointerInto((BYTE*)&temp2, copyToIndex, BXSVariableEntry::cFloatDataSize);
               }
               else
                  return(cEvaluateFail);
               break;
            }
            if (copyToType == BXSVariableEntry::cBoolVariable)
            {
               if (copyFromType == BXSVariableEntry::cIntegerVariable)
               {
                  long temp1=*(long*)(data->getHeap().getPtr()+copyFromIndex);
                  bool temp2=true;
                  if (temp1 == 0)
                     temp2=false;
                  data->getHeap().copyPointerInto((BYTE*)&temp2, copyToIndex, BXSVariableEntry::cBoolDataSize);
               }
               else if (copyFromType == BXSVariableEntry::cFloatVariable)
               {
                  float temp1=readFloat(data->getHeap().getPtr()+copyFromIndex);
                  bool temp2=false;
                  if (fabs(temp1) > cFloatCompareEpsilon)
                     temp2=true;
                  data->getHeap().copyPointerInto((BYTE*)&temp2, copyToIndex, BXSVariableEntry::cBoolDataSize);
               }
               else
                  return(cEvaluateFail);
               break;
            }
            break;
         }


         //DBG.  Simply spits out the variable (in the right format) w/o having
         //to call a syscall.
         case BXSQuadOpcode::cDBG:
         {
            long variableAddress=data->getStack()[data->getStack().getNumber()-1];
            long variableType=*(long*)(data->getHeap().getPtr()+variableAddress+BXSData::cHeapTypeOffset);

            switch (variableType)
            {
               default:
               case BXSVariableEntry::cIntegerVariable:
                  mMessenger->runMsg("%d.", *(long*)(data->getHeap().getPtr()+variableAddress+BXSData::cHeapDataOffset));
                  break;
               case BXSVariableEntry::cBoolVariable:
               {
                  bool foo=*(bool*)(data->getHeap().getPtr()+variableAddress+BXSData::cHeapDataOffset);
                  if (foo == true)
                     mMessenger->runMsg("true.");
                  else
                     mMessenger->runMsg("false.");
                  break;
               }
               case BXSVariableEntry::cFloatVariable:
                  mMessenger->runMsg("%f.", readFloat(data->getHeap().getPtr()+variableAddress+BXSData::cHeapDataOffset));
                  break;
               case BXSVariableEntry::cStringVariable:
               {
                  long stringIndex=*(long*)(data->getHeap().getPtr()+variableAddress+BXSData::cHeapDataOffset);
                  char *string=(char*)data->getUserHeapValue(stringIndex);
                  mMessenger->runMsg("%s.", string);
                  break;
               }
               case BXSVariableEntry::cVectorVariable:
                  mMessenger->runMsg("%(%f, %f, %f).",
                     readFloat(data->getHeap().getPtr()+variableAddress+BXSData::cHeapDataOffset),
                     readFloat(data->getHeap().getPtr()+variableAddress+BXSData::cHeapDataOffset+sizeof(float)),
                     readFloat(data->getHeap().getPtr()+variableAddress+BXSData::cHeapDataOffset+sizeof(float)*2) );
                  break;
            }
            break;
         }

         //ILL.  Sets the internal infinite loop limit maximum.
         case BXSQuadOpcode::cILL:
         {
            //Parse the new limit out.
            long newLimit=-1;
            if (data->parseLong(&newLimit) == false)
               return(cEvaluateFail);
            if (newLimit < -1)
               return(cEvaluateFail);

            data->setInfiniteLoopLimit(newLimit);
            break;
         }

         //IRL.  Sets the internal infinite recusion limit maximum.
         case BXSQuadOpcode::cIRL:
         {
            //Parse the new limit out.
            long newLimit=-1;
            if (data->parseLong(&newLimit) == false)
               return(cEvaluateFail);
            if (newLimit < -1)
               return(cEvaluateFail);

            data->setInfiniteRecursionLimit(newLimit);
            break;
         }

         //LINE.  Sets the current line number.
         case BXSQuadOpcode::cLINE:
         {
            //Parse the new limit out.
            long newLineNumber=-1;
            if (data->parseLong(&newLineNumber) == false)
               return(cEvaluateFail);
            if (newLineNumber < 0)
               return(cEvaluateFail);

            data->setCurrentLineNumber(newLineNumber);
            if (mMessenger->getListInterpreter() == true)
               mMessenger->runMsg("CURRENT LINE=%d.", data->getCurrentLineNumber()+1);

            //If we're in single step mode, we return here with a breakpoint set.
            if (singleStep == BXSData::cBreakpointGoSingleStep)
            {
               data->setBreakpoint(true);
               return(cEvaluateBreakpoint);
            }
            else if(singleStep == BXSData::cBreakpointGoSingleStepOver)
            {
               // If we're stepping over and we're getting a line tag on a activation
               // record that has a step over breakpoint, we need to break.
               if(data->checkAndClearStepOverBreakpoint())
               {
                  data->setBreakpoint(true);
                  return(cEvaluateBreakpoint);
               }
            }

            //Check the breakpoints.  If that returns true, return that we
            //hit a breakpoint.
            if (data->checkBreakpoints() == true)
            {
               //Print an info message (for now).
               BXSFileEntry *fileEntry=data->getFileEntryByCallStack();
               if (fileEntry == NULL)
                  mMessenger->warningMsg("XS Breakpoint: UNKNOWN FILE, line %d.", data->getCurrentLineNumber()+1);
               else
                  mMessenger->warningMsg("XS Breakpoint: File '%s', line %d.", fileEntry->getFilename(), data->getCurrentLineNumber()+1);

               //Have the data save the breakpoint data.
               data->setBreakpoint(true);
               return(cEvaluateBreakpoint);
            }
            break;
         }

         //FILE.  Sets the current file number.
         /*case BXSQuadOpcode::cFILE:
         {
            //Parse the new limit out.
            long newFileNumber=-1;
            if (data->parseLong(&newFileNumber) == false)
               return(cEvaluateFail);
            if (newFileNumber < 0)
               return(cEvaluateFail);

            mCurrentFileNumber=newFileNumber;
            break;
         }*/

         //BPNT.  Breakpoint.
         case BXSQuadOpcode::cBPNT:
         {
            //Print an info message (for now).
            BXSFileEntry *fileEntry=data->getFileEntryByCallStack();
            if (fileEntry == NULL)
               mMessenger->warningMsg("XS Breakpoint: UNKNOWN FILE, line %d.", data->getCurrentLineNumber()+1);
            else
               mMessenger->warningMsg("XS Breakpoint: File '%s', line %d.", fileEntry->getFilename(), data->getCurrentLineNumber()+1);

            //Have the data save the breakpoint data.
            data->setBreakpoint(true);

            return(cEvaluateBreakpoint);
         }

         default:
            mMessenger->errorMsg("Error: unexpected opcode=%d.", opcode);
            return(cEvaluateFail);
      }

      //Display the heap and stack.
      #ifdef DEBUGINTERPRET
      displayStackAndHeap(source, data);
      #endif
   }

   return(cEvaluateSuccess);
}

//==============================================================================
// BXSInterpreter::calculateIntegerResult
//==============================================================================
bool BXSInterpreter::calculateIntegerResult(long v1, long v2, long *result, long opcode)
{
   if (result == NULL)
      return(false);

   switch (opcode)
   {
      case BXSQuadOpcode::cADD:
         *result=v1+v2;
         return(true);
      case BXSQuadOpcode::cSUB:
         *result=v1-v2;
         return(true);
      case BXSQuadOpcode::cMUL:
         *result=v1*v2;
         return(true);
      case BXSQuadOpcode::cDIV:
         if (v2 == 0)
            *result=0x7fffFFF;
         else
            *result=v1/v2;
         return(true);
      case BXSQuadOpcode::cMOD:
         if (v2 == 0)
            *result=0;
         else
            *result=v1%v2;
         return(true);
   }

   return(false);
}

//==============================================================================
// BXSInterpreter::calculateFloatResult
//==============================================================================
bool BXSInterpreter::calculateFloatResult(float v1, float v2, float *result, long opcode)
{
   if (result == NULL)
      return(false);

   switch (opcode)
   {
      case BXSQuadOpcode::cADD:
         *result=v1+v2;
         return(true);
      case BXSQuadOpcode::cSUB:
         *result=v1-v2;
         return(true);
      case BXSQuadOpcode::cMUL:
         *result=v1*v2;
         return(true);
      case BXSQuadOpcode::cDIV:
         if ((-cFloatCompareEpsilon < v2) && (v2 < cFloatCompareEpsilon))
            *result=cMaximumFloat;
         else
            *result=v1/v2;
         return(true);
      case BXSQuadOpcode::cMOD:
      {
         long tv1=(long)v1;
         long tv2=(long)v2;
         if (tv2 == 0)
            *result=0.0f;
         else
            *result=(float)(tv1%tv2);
         return(true);
      }
   }

   return(false);
}

//==============================================================================
// BXSInterpreter::calculateVectorResult
//==============================================================================
bool BXSInterpreter::calculateVectorResult(BVector *v1, float v2, BVector *result, long opcode)
{
   if ((v1 == NULL) || (result == NULL))
      return(false);

   switch (opcode)
   {
      case BXSQuadOpcode::cMUL:
         result->x=v1->x*v2;
         result->y=v1->y*v2;
         result->z=v1->z*v2;
         return(true);
      case BXSQuadOpcode::cDIV:
         if ((-cFloatCompareEpsilon < v2) && (v2 < cFloatCompareEpsilon))
         {
            result->x=cMaximumFloat;
            result->x=cMaximumFloat;
            result->x=cMaximumFloat;
         }
         else
         {
            result->x=v1->x/v2;
            result->y=v1->y/v2;
            result->z=v1->z/v2;
         }
         return(true);
   }

   return(false);
}

//==============================================================================
// BXSInterpreter::calculateVectorResult
//==============================================================================
bool BXSInterpreter::calculateVectorResult(BVector *v1, BVector *v2, BVector *result, long opcode)
{
   if ((v1 == NULL) || (v2 == NULL) || (result == NULL))
      return(false);

   switch (opcode)
   {
      case BXSQuadOpcode::cADD:
         result->x=v1->x+v2->x;
         result->y=v1->y+v2->y;
         result->z=v1->z+v2->z;
         return(true);
      case BXSQuadOpcode::cSUB:
         result->x=v1->x-v2->x;
         result->y=v1->y-v2->y;
         result->z=v1->z-v2->z;
         return(true);
   }

   return(false);
}

//==============================================================================
// BXSInterpreter::compareIntegers
//==============================================================================
bool BXSInterpreter::compareIntegers(long v1, long v2, bool *result, long opcode)
{
   if (result == NULL)
      return(false);

   switch (opcode)
   {
      case BXSQuadOpcode::cGT:
         *result=(v1 > v2);
         return(true);
      case BXSQuadOpcode::cGE:
         *result=(v1 >= v2);
         return(true);
      case BXSQuadOpcode::cNE:
         *result=(v1 != v2);
         return(true);
      case BXSQuadOpcode::cEQ:
         *result=(v1 == v2);
         return(true);
      case BXSQuadOpcode::cLE:
         *result=(v1 <= v2);
         return(true);
      case BXSQuadOpcode::cLT:
         *result=(v1 < v2);
         return(true);
   }

   return(false);
}

//==============================================================================
// BXSInterpreter::compareFloats
//==============================================================================
bool BXSInterpreter::compareFloats(float v1, float v2, bool *result, long opcode)
{
   if (result == NULL)
      return(false);

   switch (opcode)
   {
      case BXSQuadOpcode::cGT:
         *result=(v1 > v2);
         return(true);
      case BXSQuadOpcode::cGE:
         *result=(v1 >= v2);
         return(true);
      case BXSQuadOpcode::cNE:
         *result=(v1 != v2);
         return(true);
      case BXSQuadOpcode::cEQ:
         *result=(v1 == v2);
         return(true);
      case BXSQuadOpcode::cLE:
         *result=(v1 <= v2);
         return(true);
      case BXSQuadOpcode::cLT:
         *result=(v1 < v2);
         return(true);
   }

   return(false);
}

//==============================================================================
// BXSInterpreter::compareBools
//==============================================================================
bool BXSInterpreter::compareBools(bool v1, bool v2, bool *result, long opcode)
{
   if (result == NULL)
      return(false);

   switch (opcode)
   {
      case BXSQuadOpcode::cNE:
         *result=(v1 != v2);
         return(true);
      case BXSQuadOpcode::cEQ:
         *result=(v1 == v2);
         return(true);
   }

   return(false);
}

//==============================================================================
// BXSInterpreter::compareStrings
//==============================================================================
bool BXSInterpreter::compareStrings(char *v1, char *v2, bool *result, long opcode)
{
   if ((result == NULL) || (v1 == NULL) || (v2 == NULL))
      return(false);

   long r=stricmp(v1, v2);
   switch (opcode)
   {
      case BXSQuadOpcode::cGT:
         *result=(r > 0);
         return(true);
      case BXSQuadOpcode::cGE:
         *result=(r >= 0);
         return(true);
      case BXSQuadOpcode::cNE:
         *result=(r != 0);
         return(true);
      case BXSQuadOpcode::cEQ:
         *result=(r == 0);
         return(true);
      case BXSQuadOpcode::cLE:
         *result=(r <= 0);
         return(true);
      case BXSQuadOpcode::cLT:
         *result=(r < 0);
         return(true);
   }

   return(false);
}

//==============================================================================
// BXSInterpreter::compareVectors
//==============================================================================
bool BXSInterpreter::compareVectors(BVector *v1, BVector *v2, bool *result, long opcode)
{
   if ((result == NULL) || (v1 == NULL) || (v2 == NULL))
      return(false);

   float dX=(float)fabs(v1->x-v2->x);
   float dY=(float)fabs(v1->y-v2->y);
   float dZ=(float)fabs(v1->z-v2->z);
   bool equal=true;
   if ((dX > cFloatCompareEpsilon) || (dY > cFloatCompareEpsilon) || (dZ > cFloatCompareEpsilon))
      equal=false;

   switch (opcode)
   {
      case BXSQuadOpcode::cNE:
         *result=(equal == false);
         return(true);
      case BXSQuadOpcode::cEQ:
         *result=(equal == true);
         return(true);
   }

   return(false);
}

//==============================================================================
// BXSInterpreter::displayOpcodeDebugMessage
//==============================================================================
void BXSInterpreter::displayOpcodeDebugMessage(long opcode, long pc)
{
   if (mMessenger->getListInterpreter() == false)
      return;

   //Spacer.
   char buffer[80];
   StringCchCopyA(buffer, 80, "\n");
   mMessenger->runMsg(buffer);

   //Opcode.
   bsnprintf(buffer, sizeof(buffer), "OPCODE '%s':  PC=%06d.\n", BXSQuadOpcode::getName(opcode), pc);
   mMessenger->runMsg(buffer);
}

//==============================================================================
// BXSInterpreter::displayStackAndHeap
//==============================================================================
void BXSInterpreter::displayStackAndHeap(BXSSource *source, BXSData *data)
{
   if (mMessenger->getListInterpreter() == false)
      return;

   //long dontCareLimit=30900;

   //Stack.
   char buffer[256];
   bsnprintf(buffer, sizeof(buffer), "  stack: %d elements.", data->getStack().getNumber());
   mMessenger->runMsg(buffer);
   long i=0;
   for (i=0; i < data->getStack().getNumber(); i++)
   {
      //DCP: Display hack to limit specific static junk.
      //if (data->getStack()[i] < dontCareLimit)
      //   continue;
      bsnprintf(buffer, sizeof(buffer), "    [%03d]=%d.", i, data->getStack()[i]);
      mMessenger->runMsg(buffer);
   }

   //Heap.
   bsnprintf(buffer, sizeof(buffer), "  heap: %d bytes.", data->getHeap().getNumber());
   mMessenger->runMsg(buffer);
   while (i < data->getHeap().getNumber())
   {
      long symbolID=*(long*)(data->getHeap().getPtr()+i+BXSData::cHeapSymbolIDOffset);
      long variableType=*(long*)(data->getHeap().getPtr()+i+BXSData::cHeapTypeOffset);
      long dataLength=*(long*)(data->getHeap().getPtr()+i+BXSData::cHeapLengthOffset);

      //Get the symbol.
      const char *symbol=source->getSymbol(symbolID);
      //Skip temp vars and extern variables.
      /*if ((symbol != NULL) && (strstr(symbol, "EXTERN") != NULL))
      {
         i+=BXSData::cHeapDataOffset+dataLength;
         continue;
      }*/
      static char noSymbol[]="TEMP STACK VARIABLE";
      if (symbol == NULL)
         symbol=noSymbol;

      //if (i > dontCareLimit)
      //{
         switch (variableType)
         {
            case BXSVariableEntry::cIntegerVariable:
               bsnprintf(buffer, sizeof(buffer), "    [%03d]: INT, %4d, v=%d (%s).",
                  i, dataLength, *(long*)(data->getHeap().getPtr()+i+BXSData::cHeapDataOffset), symbol);
               break;
            case BXSVariableEntry::cFloatVariable:
               bsnprintf(buffer, sizeof(buffer), "    [%03d]: FLT, %4d, v=%f (%s).",
                  i, dataLength, readFloat(data->getHeap().getPtr()+i+BXSData::cHeapDataOffset), symbol);
               break;
            case BXSVariableEntry::cBoolVariable:
               bsnprintf(buffer, sizeof(buffer), "    [%03d]: BOO, %4d, v=%d (%s).",
                  i, dataLength, *(bool*)(data->getHeap().getPtr()+i+BXSData::cHeapDataOffset), symbol);
               break;
            case BXSVariableEntry::cStringVariable:
            {
               long stringIndex=*(long*)(data->getHeap().getPtr()+i+BXSData::cHeapDataOffset);
               char *string=(char*)data->getUserHeapValue(stringIndex);
               bsnprintf(buffer, sizeof(buffer), "    [%03d]: STR, %4d, UserHeapIndex='%d', String='%s', (%s).",
                  i, dataLength, stringIndex, string, symbol);
               break;
            }
            case BXSVariableEntry::cVectorVariable:
               bsnprintf(buffer, sizeof(buffer), "    [%03d]: VEC, %4d, v=(%f, %f, %f) (%s).",
                  i, dataLength, readFloat(data->getHeap().getPtr()+i+BXSData::cHeapDataOffset),
                  readFloat(data->getHeap().getPtr()+i+BXSData::cHeapDataOffset+sizeof(float)),
                  readFloat(data->getHeap().getPtr()+i+BXSData::cHeapDataOffset+sizeof(float)*2),
                  symbol);
               break;
            default:
               bsnprintf(buffer, sizeof(buffer), "    [%03d]: ???, %4d, v=%d (%s).",
                  i, dataLength, *(long*)(data->getHeap().getPtr()+i+BXSData::cHeapDataOffset), symbol);
               break;
         }
         mMessenger->runMsg(buffer);
      //}

      i+=BXSData::cHeapDataOffset+dataLength;
   }

   //User Heap.
   bsnprintf(buffer, sizeof(buffer), "  User heap: %d entries.", data->getMaximumNumberUserHeapEntries());
   mMessenger->runMsg(buffer);
   for (i=0; i < data->getMaximumNumberUserHeapEntries(); i++)
   {
      long type=*(long*)(data->getUserHeapEntry(i)+BXSData::cUserHeapTypeOffset);
      long size=*(long*)(data->getUserHeapEntry(i)+BXSData::cUserHeapSizeOffset);
      long maxSize=*(long*)(data->getUserHeapEntry(i)+BXSData::cUserHeapMaxSizeOffset);
      long refCount=*(long*)(data->getUserHeapEntry(i)+BXSData::cUserHeapRefCountOffset);
      BYTE *value=data->getUserHeapEntry(i)+BXSData::cUserHeapValueOffset;

      switch (type)
      {
         case BXSVariableEntry::cStringVariable:
         {
            char *string=(char*)value;
            bsnprintf(buffer, sizeof(buffer), "    [%03d]: String='%s', (Size=%d, MaxSize=%d, Refs=%d).",
               i, string, size, maxSize, refCount);
            break;
         }
         default:
            bsnprintf(buffer, sizeof(buffer), "    [%03d]: ???, UnknownType=%d, (Size=%d, MaxSize=%d, Refs=%d).",
               i, type, size, maxSize, refCount);
            break;
      }
      mMessenger->runMsg(buffer);
   }
}

//==============================================================================
// BXSInterpreter::checkInfiniteRecursion
//==============================================================================
bool BXSInterpreter::checkInfiniteRecursion(BXSData *data, long functionID)
{
   //Skip if we're not tracking this.
   if (data->getInfiniteRecursionLimit() <= 0)
      return(false);

   //Simply count the number of times this functionID appears on the call stack.
   //If it's too many, return true.
   long count=0;
   for (long i=0; i < data->getCallStackSize(); i++)
   {
      if (data->getCallStack()[i].getFunctionID() == functionID)
      {
         count++;
         if (count >= data->getInfiniteRecursionLimit())
            return(true);
      }
   }

   return(false);
}
