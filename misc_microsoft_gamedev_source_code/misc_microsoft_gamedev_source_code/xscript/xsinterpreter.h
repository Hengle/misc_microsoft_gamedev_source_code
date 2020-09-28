//==============================================================================
// xsinterpreter.h
//
// Copyright (c) 2000-2003, Ensemble Studios
//==============================================================================

#ifndef _XSINTERPRETER_H_
#define _XSINTERPRETER_H_

//==============================================================================
// Includes
#include "xsdefines.h"

//==============================================================================
// Forward declarations
class BXSData;
class BXSFunctionEntry;
class BXSInterpreter;
class BXSMessenger;
class BXSSource;
class BXSSyscallModule;
class BXSVariableEntry;

//==============================================================================
// Const declarations


//==============================================================================
class BXSInterpreter
{
   public:
      //Ctors/Dtor.
      BXSInterpreter( BXSMessenger *messenger, BXSSyscallModule *sm );
      virtual ~BXSInterpreter( void );

      //Init.
      bool                       initialize( void );

      //Interpret.
      bool                       interpret( BXSSource *source, BXSData *data );
      //interpretRule interprets the specific rule and then returns.  interpretRules
      //runs as many rule interps as it can (in the time given), stopping if it runs out
      //of rules.  Persistent 'data' is stored in the BXSData structure.
      bool                       interpretRule( BXSSource *source, BXSData *data, long ruleID );
      bool                       interpretRules( BXSSource *source, BXSData *data, DWORD currentTime, DWORD timeLimit, bool &lastRule );
      //interpretFunction interprets the given function and then returns.
      bool                       interpretFunction( BXSSource *source, BXSData *data, long functionID, long parameter, bool pushParameter );
      bool                       interpretFunction( BXSSource *source, BXSData *data, const char *functionName, long parameter, bool pushParameter );
      //interpretRestart picks up from where a breakpoint left off.
      bool                       interpretRestart( BXSSource *source, BXSData *data, long singleStep );

   protected:
      //LowLevelInterpret.  Shared code between the big interpretFunction and interpretRestart
      //methods.
      long                       interpretLowLevel( BXSSource *source, BXSData *data, long singleStep );

      //Evaluate code.  The real worker method.
      enum
      {
         cEvaluateFail=0,
         cEvaluateSuccess=1,
         cEvaluateBreakpoint=2
      };
      long                       evaluateCode( BXSSource *source, BXSData *data, long singleStep );

      //Math helpers.
      bool                       calculateIntegerResult( long v1, long v2, long *result, long opcode );
      bool                       calculateFloatResult( float v1, float v2, float *result, long opcode );
      bool                       calculateVectorResult( BVector *v1, float v2, BVector *result, long opcode );
      bool                       calculateVectorResult( BVector *v1, BVector *v2, BVector *result, long opcode );

      //Bool ops.
      bool                       compareIntegers( long v1, long v2, bool *result, long opcode );
      bool                       compareFloats( float v1, float v2, bool *result, long opcode );
      bool                       compareBools( bool v1, bool v2, bool *result, long opcode );
      bool                       compareStrings( char *v1, char *v2, bool *result, long opcode );
      bool                       compareVectors( BVector *v1, BVector *v2, bool *result, long opcode );

      //Debug display methods.
      void                       displayOpcodeDebugMessage( long opcode, long pc );
      void                       displayStackAndHeap( BXSSource *source, BXSData *data );

      //Debugging.
      bool                       checkInfiniteRecursion( BXSData *data, long functionID );

      //Variables.
      BXSSyscallModule*          mSyscalls;
      BXSMessenger*              mMessenger;
};

//==============================================================================
#endif // _XSINTERPRETER_H_
