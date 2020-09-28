//==============================================================================
// xsdefines.h
//
// Copyright (c) 1999-2003, Ensemble Studios
//==============================================================================

#ifndef _XSDEFINES_H_
#define _XSDEFINES_H_

//==============================================================================
// Includes

//==============================================================================
// Const declarations

//==============================================================================
// Defines


//==============================================================================
// Config function prototype.
typedef bool (*BXSConfigFunction)( class BXSSyscallModule *sm );

//Signed BYTE.
typedef signed char SBYTE;


//==============================================================================
class BXSBinary
{
   public:
      //Symbol Types.
      enum
      {
         cInvalidSymbol=255,
         cSyscall=0,
         cVariable,
         cStackVariable,
         cFunction,
         cLabel,
         cOpcode,
         cImmediateVariable,
         cRuleGroup,
         cFilename,
         cClass,
         cClassVariable
      };
};


//==============================================================================
class BXSLabelEntry
{
   public:
      long                       mPosition;
      long                       mSymbolID;
};
typedef BDynamicSimArray<BXSLabelEntry> BXSLabelArray;


//==============================================================================
class BXSFunctionOverloadEntry
{
   public:
      long                       mOldFunctionID;
      long                       mNewFunctionID;
};
typedef BDynamicSimArray<BXSFunctionOverloadEntry> BXSFunctionOverloadArray;


//==============================================================================
#endif // _XSDEFINES_H_
