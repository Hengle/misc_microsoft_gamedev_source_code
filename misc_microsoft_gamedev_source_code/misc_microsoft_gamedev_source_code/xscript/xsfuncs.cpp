//==============================================================================
// xsfuncs.cpp
//
// Copyright (c) 2000-2003, Ensemble Studios
//==============================================================================

//==============================================================================
// Includes
#include "xscript.h"
#include "xsfuncs.h"
#include "xsdata.h"
#include "xsdefines.h"
#include "xsruntime.h"
#include "xssyscallmodule.h"

//==============================================================================
//Setup XS macros.  Don't change this.
#include "xsconfigmacros.h"



//==============================================================================
// xsDisableRule
//==============================================================================
void xsDisableRule(char *ruleName, BXSData *data)
{
   data->setRuleActive(ruleName, false, data->getCurrentTime());
}

//==============================================================================
// xsDisableSelf
//==============================================================================
void xsDisableSelf(BXSData *data)
{
   data->setRuleActive(data->getCurrentRuleID(), false, data->getCurrentTime());
}

//==============================================================================
// xsEnableRule
//==============================================================================
void xsEnableRule(char *ruleName, BXSData *data)
{
   data->setRuleActive(ruleName, true, data->getCurrentTime());
}

//==============================================================================
// xsIsRuleEnabled
//==============================================================================
bool xsIsRuleEnabled(char *ruleName, BXSData *data)
{
   return(data->isRuleActive(ruleName));
}

//==============================================================================
// xsSetRulePriority
//==============================================================================
void xsSetRulePriority(char *ruleName, long priority, BXSData *data)
{
   data->modifyRulePriority(data->getRuleID(ruleName), priority);
}

//==============================================================================
// xsSetRulePrioritySelf
//==============================================================================
void xsSetRulePrioritySelf(long priority, BXSData *data)
{
   data->modifyRulePriority(data->getCurrentRuleID(), priority);
}

//==============================================================================
// xsSetRuleMinInterval
//==============================================================================
void xsSetRuleMinInterval(char *ruleName, long minInterval, BXSData *data)
{
   data->modifyRuleMinInterval(data->getRuleID(ruleName), (DWORD)(minInterval*1000));
}

//==============================================================================
// xsSetRuleMinIntervalSelf
//==============================================================================
void xsSetRuleMinIntervalSelf(long minInterval, BXSData *data)
{
   data->modifyRuleMinInterval(data->getCurrentRuleID(), (DWORD)(minInterval*1000));
}

//==============================================================================
// xsSetRuleMaxInterval
//==============================================================================
void xsSetRuleMaxInterval(char *ruleName, long maxInterval, BXSData *data)
{
   data->modifyRuleMaxInterval(data->getRuleID(ruleName), (DWORD)(maxInterval*1000));
}

//==============================================================================
// xsSetRuleMaxIntervalSelf
//==============================================================================
void xsSetRuleMaxIntervalSelf(long maxInterval, BXSData *data)
{
   data->modifyRuleMaxInterval(data->getCurrentRuleID(), (DWORD)(maxInterval*1000));
}

//==============================================================================
// xsDisableRuleGroup
//==============================================================================
void xsDisableRuleGroup(char *ruleGroupName, BXSData *data)
{
   data->setRuleGroupActive(ruleGroupName, false, data->getCurrentTime());
}

//==============================================================================
// xsEnableRuleGroup
//==============================================================================
void xsEnableRuleGroup(char *ruleGroupName, BXSData *data)
{
   data->setRuleGroupActive(ruleGroupName, true, data->getCurrentTime());
}

//==============================================================================
// xsIsRuleGroupEnabled
//==============================================================================
bool xsIsRuleGroupEnabled(char *ruleGroupName, BXSData *data)
{
   return(data->isRuleGroupActive(ruleGroupName));
}

//==============================================================================
// xsVectorGetX
//==============================================================================
float xsVectorGetX(const BVector *v)
{
   if (v == NULL)
      return(0.0f);
   return(v->x);
}

//==============================================================================
// xsVectorGetY
//==============================================================================
float xsVectorGetY(const BVector *v)
{
   if (v == NULL)
      return(0.0f);
   return(v->y);
}

//==============================================================================
// xsVectorGetZ
//==============================================================================
float xsVectorGetZ(const BVector *v)
{
   if (v == NULL)
      return(0.0f);
   return(v->z);
}

//==============================================================================
// xsVectorSetX
//==============================================================================
BVector xsVectorSetX(const BVector *v, float x)
{
   BVector foo;
   if (v == NULL)
      foo=cOriginVector;
   else
      foo=*v;
   foo.x=x;
   return(foo);
}

//==============================================================================
// xsVectorSetY
//==============================================================================
BVector xsVectorSetY(const BVector *v, float y)
{
   BVector foo;
   if (v == NULL)
      foo=cOriginVector;
   else
      foo=*v;
   foo.y=y;
   return(foo);
}

//==============================================================================
// xsVectorSetZ
//==============================================================================
BVector xsVectorSetZ(const BVector *v, float z)
{
   BVector foo;
   if (v == NULL)
      foo=cOriginVector;
   else
      foo=*v;
   foo.z=z;
   return(foo);
}

//==============================================================================
// xsVectorSet
//==============================================================================
BVector xsVectorSet(float x, float y, float z)
{
   BVector foo(x, y, z);
   return(foo);
}

//==============================================================================
// xsVectorLength
//==============================================================================
float xsVectorLength(const BVector *v)
{
   if (v == NULL)
      return(0.0f);
   float rVal=v->length();
   return(rVal);
}

//==============================================================================
// xsVectorNormalize
//==============================================================================
BVector xsVectorNormalize(const BVector *v)
{
   if (v == NULL)
      return(cOriginVector);
   BVector foo(v->x, v->y, v->z);
   foo.normalize();
   return(foo);
}

//==============================================================================
// xsArrayCreateInt
//==============================================================================
long xsArrayCreateInt(long size, long defaultValue, const char* name, BXSData *data)
{
   return(data->arrayCreateInt(size, defaultValue, BSimString(name)));
}

//==============================================================================
// xsArraySetInt
//==============================================================================
bool xsArraySetInt(long arrayID, long index, long value, BXSData *data)
{
   return(data->arraySetInt(arrayID, index, value));
}

//==============================================================================
// xsArrayGetInt
//==============================================================================
long xsArrayGetInt(long arrayID, long index, BXSData *data)
{
   return(data->arrayGetInt(arrayID, index));
}

//==============================================================================
// xsArrayCreateFloat
//==============================================================================
long xsArrayCreateFloat(long size, float defaultValue, const char* name, BXSData *data)
{
   return(data->arrayCreateFloat(size, defaultValue, BSimString(name)));
}

//==============================================================================
// xsArraySetFloat
//==============================================================================
bool xsArraySetFloat(long arrayID, long index, float value, BXSData *data)
{
   return(data->arraySetFloat(arrayID, index, value));
}

//==============================================================================
// xsArrayGetFloat
//==============================================================================
float xsArrayGetFloat(long arrayID, long index, BXSData *data)
{
   return(data->arrayGetFloat(arrayID, index));
}

//==============================================================================
// xsArrayCreateBool
//==============================================================================
long xsArrayCreateBool(long size, bool defaultValue, const char* name, BXSData *data)
{
   return(data->arrayCreateBool(size, defaultValue, BSimString(name)));
}

//==============================================================================
// xsArraySetBool
//==============================================================================
bool xsArraySetBool(long arrayID, long index, bool value, BXSData *data)
{
   return(data->arraySetBool(arrayID, index, value));
}

//==============================================================================
// xsArrayGetBool
//==============================================================================
bool xsArrayGetBool(long arrayID, long index, BXSData *data)
{
   return(data->arrayGetBool(arrayID, index));
}

//==============================================================================
// xsArrayCreateString
//==============================================================================
long xsArrayCreateString(long size, const char* defaultValue, const char* name, BXSData *data)
{
   return(data->arrayCreateString(size, BSimString(defaultValue), BSimString(name)));
}

//==============================================================================
// xsArraySetString
//==============================================================================
bool xsArraySetString(long arrayID, long index, const char* value, BXSData *data)
{
   return(data->arraySetString(arrayID, index, BSimString(value)));
}

//==============================================================================
// xsArrayGetString
//==============================================================================
const char* xsArrayGetString(long arrayID, long index, BXSData *data)
{
   return(data->arrayGetString(arrayID, index));
}

//==============================================================================
// xsArrayCreateVector
//==============================================================================
long xsArrayCreateVector(long size, const BVector& defaultValue, const char* name, BXSData *data)
{
   return(data->arrayCreateVector(size, defaultValue, BSimString(name)));
}

//==============================================================================
// xsArraySetVector
//==============================================================================
bool xsArraySetVector(long arrayID, long index, const BVector& value, BXSData *data)
{
   return(data->arraySetVector(arrayID, index, value));
}

//==============================================================================
// xsArrayGetVector
//==============================================================================
BVector xsArrayGetVector(long arrayID, long index, BXSData *data)
{
   return(data->arrayGetVector(arrayID, index));
}


//==============================================================================
// xsArrayGetSize
//==============================================================================
long xsArrayGetSize(long arrayID, BXSData *data)
{
   return(data->arrayGetSize(arrayID));
}


//==============================================================================
// xsDumpArrays
//==============================================================================
void xsDumpArrays(BXSData *data)
{
   data->dumpArrays();
}

//==============================================================================
// xsGetContextPlayer
//==============================================================================
long xsGetContextPlayer(void)
{
   return(BXSRuntime::getXSContextPlayerID());
}

//==============================================================================
// xsSetContextPlayer
//==============================================================================
void xsSetContextPlayer(long playerID)
{
   BXSRuntime::setXSContextPlayerID(playerID);
}

//==============================================================================
// xsGetTime
//==============================================================================
long xsGetTime(BXSData *data)
{
   return(data->getCurrentTime());
}

//==============================================================================
// xsAddRuntimeEvent
//==============================================================================
bool xsAddRuntimeEvent(const char *runtimeName, const char *functionName, long parameter)
{
   BSimString runtimeName2(runtimeName);
   BXSRuntime *xsr=BXSRuntime::getXSRuntime(runtimeName2);
   if (xsr == NULL)
      return(false);
   return(xsr->addEvent(0, functionName, parameter));
}

//==============================================================================
// xsGetFunctionID
//==============================================================================
long xsGetFunctionID(const char *functionName, BXSData *data)
{
   return(data->getFunctionID(functionName));
}


//==============================================================================
// addXSFunctions.
bool addXSFunctions(BXSSyscallModule *sm)
{
   if (sm == NULL)
      return(false);

   //Rule control.
   XS_CONTEXTSYSCALL("xsDisableRule", BXSVariableEntry::cVoidVariable, &xsDisableRule, 0)
      XS_STRING_PARM("")
      XS_HELP("void xsDisableRule( string ruleName ): Disables the given rule.")
   XS_CONTEXTSYSCALL("xsDisableSelf", BXSVariableEntry::cVoidVariable, &xsDisableSelf, 0)
      XS_HELP("void xsDisableSelf( void ): Disables the current rule.")
   XS_CONTEXTSYSCALL("xsEnableRule", BXSVariableEntry::cVoidVariable, &xsEnableRule, 0)
      XS_STRING_PARM("")
      XS_HELP("void xsEnableRule( string ruleName ): Enables the given rule.")
   XS_CONTEXTSYSCALL("xsIsRuleEnabled", BXSVariableEntry::cBoolVariable, &xsIsRuleEnabled, 0)
      XS_STRING_PARM("")
      XS_HELP("bool xsIsRuleEnabled( string ruleName ): Returns true if the rule is enabled.")
   //Rule modifications.
   XS_CONTEXTSYSCALL("xsSetRulePriority", BXSVariableEntry::cVoidVariable, &xsSetRulePriority, 0)
      XS_STRING_PARM("")
      XS_INTEGER_PARM(0)
      XS_HELP("void xsSetRulePriority( string ruleName, int priority ): Sets the priority of the given rule.")
   XS_CONTEXTSYSCALL("xsSetRulePrioritySelf", BXSVariableEntry::cVoidVariable, &xsSetRulePrioritySelf, 0)
      XS_INTEGER_PARM(0)
      XS_HELP("void xsSetRulePrioritySelf( int priority ): Sets the priority of the current rule.")
   XS_CONTEXTSYSCALL("xsSetRuleMinInterval", BXSVariableEntry::cVoidVariable, &xsSetRuleMinInterval, 0)
      XS_STRING_PARM("")
      XS_INTEGER_PARM(0)
      XS_HELP("void xsSetRuleMinInterval( string ruleName, int interval ): Sets the min interval of the given rule.")
   XS_CONTEXTSYSCALL("xsSetRuleMinIntervalSelf", BXSVariableEntry::cVoidVariable, &xsSetRuleMinIntervalSelf, 0)
      XS_INTEGER_PARM(0)
      XS_HELP("void xsSetRuleMinIntervalSelf( int interval ): Sets the min interval of the current rule.")
   XS_CONTEXTSYSCALL("xsSetRuleMaxInterval", BXSVariableEntry::cVoidVariable, &xsSetRuleMaxInterval, 0)
      XS_STRING_PARM("")
      XS_INTEGER_PARM(0)
      XS_HELP("void xsSetRuleMaxInterval( string ruleName, int interval ): Sets the max interval of the given rule.")
   XS_CONTEXTSYSCALL("xsSetRuleMaxIntervalSelf", BXSVariableEntry::cVoidVariable, &xsSetRuleMaxIntervalSelf, 0)
      XS_INTEGER_PARM(0)
      XS_HELP("void xsSetRuleMaxIntervalSelf( int interval ): Sets the max interval of the current rule.")
   //Rule Group control.
   XS_CONTEXTSYSCALL("xsEnableRuleGroup", BXSVariableEntry::cVoidVariable, &xsEnableRuleGroup, 0)
      XS_STRING_PARM("")
      XS_HELP("void xsEnableRuleGroup( string ruleGroupName ): Enables all rule in the given rule group.")
   XS_CONTEXTSYSCALL("xsDisableRuleGroup", BXSVariableEntry::cVoidVariable, &xsDisableRuleGroup, 0)
      XS_STRING_PARM("")
      XS_HELP("void xsDisableRuleGroup( string ruleGroupName ): Disables all rules in the given rule group.")
   XS_CONTEXTSYSCALL("xsIsRuleGroupEnabled", BXSVariableEntry::cVoidVariable, &xsIsRuleGroupEnabled, 0)
      XS_STRING_PARM("")
      XS_HELP("void xsIsRuleGroupEnabled( string ruleGroupName ): Returns true if the rule group is enabled.")

   //Vectors.
   XS_SYSCALL("xsVectorGetX", BXSVariableEntry::cFloatVariable, &xsVectorGetX, 0)
      XS_VECTOR_PARM(cOriginVector)
      XS_HELP("float xsVectorGetX( vector v ): Returns the x component of the given vector.")
   XS_SYSCALL("xsVectorGetY", BXSVariableEntry::cFloatVariable, &xsVectorGetY, 0)
      XS_VECTOR_PARM(cOriginVector)
      XS_HELP("float xsVectorGetY( vector v ): Returns the y component of the given vector.")
   XS_SYSCALL("xsVectorGetZ", BXSVariableEntry::cFloatVariable, &xsVectorGetZ, 0)
      XS_VECTOR_PARM(cOriginVector)
      XS_HELP("float xsVectorGetZ( vector v ): Returns the z component of the given vector.")
   XS_SYSCALL("xsVectorSetX", BXSVariableEntry::cVectorVariable, &xsVectorSetX, 0)
      XS_VECTOR_PARM(cOriginVector)
      XS_FLOAT_PARM(0.0f)
      XS_HELP("float xsVectorSetX( vector v, float x ): Set the x component of the given vector, returns the new vector.")
   XS_SYSCALL("xsVectorSetY", BXSVariableEntry::cVectorVariable, &xsVectorSetY, 0)
      XS_VECTOR_PARM(cOriginVector)
      XS_FLOAT_PARM(0.0f)
      XS_HELP("float xsVectorSetY( vector v, float y ): Set the y component of the given vector, returns the new vector.")
   XS_SYSCALL("xsVectorSetZ", BXSVariableEntry::cVectorVariable, &xsVectorSetZ, 0)
      XS_VECTOR_PARM(cOriginVector)
      XS_FLOAT_PARM(0.0f)
      XS_HELP("float xsVectorSetZ( vector v, float z ): Set the z component of the given vector, returns the new vector.")
   XS_SYSCALL("xsVectorSet", BXSVariableEntry::cVectorVariable, &xsVectorSet, 0)
      XS_FLOAT_PARM(0.0f)
      XS_FLOAT_PARM(0.0f)
      XS_FLOAT_PARM(0.0f)
      XS_HELP("float xsVectorSet( float x, float y, float z ): Set the 3 components into a vector, returns the new vector.")
   XS_SYSCALL("xsVectorLength", BXSVariableEntry::cFloatVariable, &xsVectorLength, 0)
      XS_VECTOR_PARM(cOriginVector)
      XS_HELP("float xsVectorLength( vector v ): Returns the length of the given vector.")
   XS_SYSCALL("xsVectorNormalize", BXSVariableEntry::cVectorVariable, &xsVectorNormalize, 0)
      XS_VECTOR_PARM(cOriginVector)
      XS_HELP("float xsVectorNormalize( vector v): Returns the normalized version of the given vector.")

   // MS 1/6/2004: arrays
   XS_CONTEXTSYSCALL("xsArrayCreateInt", BXSVariableEntry::cIntegerVariable, &xsArrayCreateInt, 0)
      XS_INTEGER_PARM(-1)
      XS_INTEGER_PARM(0)
      XS_STRING_PARM("")
      XS_HELP("int xsArrayCreateInt(int size, int defaultValue, string name): creates a sized and named integer array, returning an arrayID.")
   XS_CONTEXTSYSCALL("xsArraySetInt", BXSVariableEntry::cBoolVariable, &xsArraySetInt, 0)
      XS_INTEGER_PARM(-1)
      XS_INTEGER_PARM(-1)
      XS_INTEGER_PARM(-1)
      XS_HELP("int xsArraySetInt(int arrayID, int index, int value): Sets a value at the specified index in the requested array.")
   XS_CONTEXTSYSCALL("xsArrayGetInt", BXSVariableEntry::cIntegerVariable, &xsArrayGetInt, 0)
      XS_INTEGER_PARM(-1)
      XS_INTEGER_PARM(-1)
      XS_HELP("int xsArrayGetInt(int arrayID, int index): Gets the value at the specified index in the requested array.")

   XS_CONTEXTSYSCALL("xsArrayCreateFloat", BXSVariableEntry::cIntegerVariable, &xsArrayCreateFloat, 0)
      XS_INTEGER_PARM(-1)
      XS_FLOAT_PARM(0.0f)
      XS_STRING_PARM("")
      XS_HELP("int xsArrayCreateFloat(int size, float defaultValue, string name): creates a sized and named float array, returning an arrayID.")
   XS_CONTEXTSYSCALL("xsArraySetFloat", BXSVariableEntry::cBoolVariable, &xsArraySetFloat, 0)
      XS_INTEGER_PARM(-1)
      XS_INTEGER_PARM(-1)
      XS_FLOAT_PARM(-1)
      XS_HELP("int xsArraySetFloat(int arrayID, int index, float value): Sets a value at the specified index in the requested array.")
   XS_CONTEXTSYSCALL("xsArrayGetFloat", BXSVariableEntry::cFloatVariable, &xsArrayGetFloat, 0)
      XS_INTEGER_PARM(-1)
      XS_INTEGER_PARM(-1)
      XS_HELP("float xsArrayGetFloat(int arrayID, int index): Gets the value at the specified index in the requested array.")

   XS_CONTEXTSYSCALL("xsArrayCreateBool", BXSVariableEntry::cIntegerVariable, &xsArrayCreateBool, 0)
      XS_INTEGER_PARM(-1)
      XS_BOOL_PARM(false)
      XS_STRING_PARM("")
      XS_HELP("int xsArrayCreateBool(int size, bool defaultValue, string name): creates a sized and named boolean array, returning an arrayID.")
   XS_CONTEXTSYSCALL("xsArraySetBool", BXSVariableEntry::cBoolVariable, &xsArraySetBool, 0)
      XS_INTEGER_PARM(-1)
      XS_INTEGER_PARM(-1)
      XS_BOOL_PARM(false)
      XS_HELP("int xsArraySetBool(int arrayID, int index, bool value): Sets a value at the specified index in the requested array.")
   XS_CONTEXTSYSCALL("xsArrayGetBool", BXSVariableEntry::cBoolVariable, &xsArrayGetBool, 0)
      XS_INTEGER_PARM(-1)
      XS_INTEGER_PARM(-1)
      XS_HELP("bool xsArrayGetInt(int arrayID, int index): Gets the value at the specified index in the requested array.")

   XS_CONTEXTSYSCALL("xsArrayCreateString", BXSVariableEntry::cIntegerVariable, &xsArrayCreateString, 0)
      XS_INTEGER_PARM(-1)
      XS_STRING_PARM("<default string>")
      XS_STRING_PARM("")
      XS_HELP("int xsArrayCreateString(int size, string defaultValue, string name): creates a sized and named string array, returning an arrayID.")
   XS_CONTEXTSYSCALL("xsArraySetString", BXSVariableEntry::cBoolVariable, &xsArraySetString, 0)
      XS_INTEGER_PARM(-1)
      XS_INTEGER_PARM(-1)
      XS_STRING_PARM("<default string>")
      XS_HELP("int xsArraySetString(int arrayID, int index, string value): Sets a value at the specified index in the requested array.")
   XS_CONTEXTSYSCALL("xsArrayGetString", BXSVariableEntry::cStringVariable, &xsArrayGetString, 0)
      XS_INTEGER_PARM(-1)
      XS_INTEGER_PARM(-1)
      XS_HELP("string xsArrayGetString(int arrayID, int index): Gets the value at the specified index in the requested array.")

   XS_CONTEXTSYSCALL("xsArrayCreateVector", BXSVariableEntry::cIntegerVariable, &xsArrayCreateVector, 0)
      XS_INTEGER_PARM(-1)
      XS_VECTOR_PARM(cInvalidVector)
      XS_STRING_PARM("")
      XS_HELP("int xsArrayCreateVector(int size, vector defaultValue, string name): creates a sized and named vector array, returning an arrayID.")
   XS_CONTEXTSYSCALL("xsArraySetVector", BXSVariableEntry::cBoolVariable, &xsArraySetVector, 0)
      XS_INTEGER_PARM(-1)
      XS_INTEGER_PARM(-1)
      XS_VECTOR_PARM(cInvalidVector)
      XS_HELP("int xsArraySetVector(int arrayID, int index, vector value): Sets a value at the specified index in the requested array.")
   XS_CONTEXTSYSCALL("xsArrayGetVector", BXSVariableEntry::cVectorVariable, &xsArrayGetVector, 0)
      XS_INTEGER_PARM(-1)
      XS_INTEGER_PARM(-1)
      XS_HELP("vector xsArrayGetVector(int arrayID, int index): Gets the value at the specified index in the requested array.")

   XS_CONTEXTSYSCALL("xsArrayGetSize", BXSVariableEntry::cIntegerVariable, &xsArrayGetSize, 0)
      XS_INTEGER_PARM(-1)
      XS_HELP("int xsArrayGetSize(int arrayID): Gets the specified array's size.")

   XS_CONTEXTSYSCALL("xsDumpArrays", BXSVariableEntry::cVoidVariable, &xsDumpArrays, 0)
      XS_HELP("int xsDumpArrays(): blogs out all XS arrays.")

      
   //Context.
   XS_SYSCALL("xsGetContextPlayer", BXSVariableEntry::cIntegerVariable, &xsGetContextPlayer, 0)
      XS_HELP("int xsGetContextPlayer( void ): Returns the current context player ID.")
   XS_SYSCALL("xsSetContextPlayer", BXSVariableEntry::cVoidVariable, &xsSetContextPlayer, 0)
      XS_INTEGER_PARM(-1)
      XS_HELP("void xsSetContextPlayer( int playerID ): Sets the current context player ID (DO NOT DO THIS IF YOU DO NOT KNOW WHAT YOU ARE DOING).")

   //Time.
   XS_CONTEXTSYSCALL("xsGetTime", BXSVariableEntry::cIntegerVariable, &xsGetTime, 0)
      XS_HELP("int xsGetTime( void ): Returns the current gametime (in milliseconds).")

   //Runtime event.
   XS_SYSCALL("xsAddRuntimeEvent", BXSVariableEntry::cBoolVariable, &xsAddRuntimeEvent, 0)
      XS_STRING_PARM("")
      XS_STRING_PARM("")
      XS_INTEGER_PARM(-1)
      XS_HELP("bool xsAddRuntimeEvent( string foo, string bar, int something ): Setups a runtime event.  Don't use this.")

   //Function ID.
   XS_CONTEXTSYSCALL("xsGetFunctionID", BXSVariableEntry::cIntegerVariable, &xsGetFunctionID, 0)
      XS_STRING_PARM("")
      XS_HELP("int xsGetFuntionID( string functionName ): Runs the secret XSFID for the function. USE WITH CAUTION.")

   return(true);
}
