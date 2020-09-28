//============================================================================
//
// File: simpletypes.h
//  
// Copyright (c) 1999-2007, Ensemble Studios
//
//============================================================================
#pragma once

#ifdef UNICODE
   typedef WORD            BCHAR_T;
   const int BCHAR_T_MAX = 0xFFFF;
#else
   typedef char            BCHAR_T;
   const int BCHAR_T_MAX = 0xFF;
#endif

typedef BCHAR_T*           PBCHAR_T;

// Useful enums
enum eComponent 
{ 
   cX = 0, 
   cY = 1, 
   cZ = 2, 
   cW = 3,
   
   cComponentForceInt = 0x7FFFFFFF
};

enum eResult
{
   cFAILURE     = -1,
   cSUCCESS     = 0,
   cPARALLEL    = 1,
   
   cINSIDE      = 2,
   cOUTSIDE     = 3,
   cPARTIAL     = 4,
   
   cAINSIDEB    = 5,
   cBINSIDEA    = 6,
   
   cResultForceInt = 0x7FFFFFFF
};

enum eIndex
{
   cInvalidIndex = -1,
   
   cIndexForceInt = 0x7FFFFFFF
};

enum eVarArg 
{ 
   cVarArg,
   
   cVarArgForceInt = 0x7FFFFFFF
};

enum eClear
{
   cClear,
   
   cClearForceInt = 0x7FFFFFFF
};

template<typename CharType>
struct BStringDefaults
{
   static const CharType* getEmptyString(void);
   static const CharType* getSpaceString(void);
   static const CharType* getDelimiterString(void);
   static const CharType* getWhiteSpaceString(void);
};

template<> inline const char* BStringDefaults<char>::getEmptyString(void) { return ""; }
template<> inline const WCHAR* BStringDefaults<WCHAR>::getEmptyString(void) { return L""; }

template<> inline const char* BStringDefaults<char>::getSpaceString(void) { return " "; }
template<> inline const WCHAR* BStringDefaults<WCHAR>::getSpaceString(void) { return L" "; }

template<> inline const char* BStringDefaults<char>::getDelimiterString(void) { return "$$"; }
template<> inline const WCHAR* BStringDefaults<WCHAR>::getDelimiterString(void) { return L"$$"; }

template<> inline const char* BStringDefaults<char>::getWhiteSpaceString(void) { return " \t\n\r"; }
template<> inline const WCHAR* BStringDefaults<WCHAR>::getWhiteSpaceString(void) { return L" \t\n\r"; }

template<typename CharType>
struct BOppositeCharType
{
};

template<> struct BOppositeCharType<WCHAR> { typedef char t; };
template<> struct BOppositeCharType<char> { typedef WCHAR t; };

typedef void* BHandle;
  