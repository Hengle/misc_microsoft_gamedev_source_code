//==============================================================================
// File: builtInTypes.h
//
// Copyright (c) 2006-2008, Ensemble Studios
//==============================================================================
#pragma once

// BIsBuiltInType<ValueType>::Flag indicates whether or not ValueType is a simple/fundamental/built-in type.

template<class T> 
struct BIsBuiltInType 
{ 
   enum { Flag = false }; 
};

#define DEFINE_BUILT_IN_TYPE(TYPE) template<> struct BIsBuiltInType<TYPE> { enum { Flag = true }; };

DEFINE_BUILT_IN_TYPE(bool)

DEFINE_BUILT_IN_TYPE(char)
DEFINE_BUILT_IN_TYPE(unsigned char)

DEFINE_BUILT_IN_TYPE(short)
DEFINE_BUILT_IN_TYPE(unsigned short)

DEFINE_BUILT_IN_TYPE(int)
DEFINE_BUILT_IN_TYPE(unsigned int)

DEFINE_BUILT_IN_TYPE(long)
DEFINE_BUILT_IN_TYPE(unsigned long)

DEFINE_BUILT_IN_TYPE(__int64)
DEFINE_BUILT_IN_TYPE(unsigned __int64)

DEFINE_BUILT_IN_TYPE(float)
DEFINE_BUILT_IN_TYPE(double)
DEFINE_BUILT_IN_TYPE(long double)

DEFINE_BUILT_IN_TYPE(__wchar_t)

DEFINE_BUILT_IN_TYPE(void*)
DEFINE_BUILT_IN_TYPE(char*)
DEFINE_BUILT_IN_TYPE(unsigned char*)
DEFINE_BUILT_IN_TYPE(short*)
DEFINE_BUILT_IN_TYPE(unsigned short*)
DEFINE_BUILT_IN_TYPE(int*);
DEFINE_BUILT_IN_TYPE(unsigned int*);
DEFINE_BUILT_IN_TYPE(__int64*);
DEFINE_BUILT_IN_TYPE(unsigned __int64*);
DEFINE_BUILT_IN_TYPE(float*);
DEFINE_BUILT_IN_TYPE(double*);

DEFINE_BUILT_IN_TYPE(const void*)
DEFINE_BUILT_IN_TYPE(const char*)
DEFINE_BUILT_IN_TYPE(const unsigned char*)
DEFINE_BUILT_IN_TYPE(const short*)
DEFINE_BUILT_IN_TYPE(const unsigned short*)
DEFINE_BUILT_IN_TYPE(const int*);
DEFINE_BUILT_IN_TYPE(const unsigned int*);
DEFINE_BUILT_IN_TYPE(const __int64*);
DEFINE_BUILT_IN_TYPE(const unsigned __int64*);
DEFINE_BUILT_IN_TYPE(const float*);
DEFINE_BUILT_IN_TYPE(const double*);

#ifdef XBOX
   DEFINE_BUILT_IN_TYPE(__vector4)
   DEFINE_BUILT_IN_TYPE(__vector4*)
   DEFINE_BUILT_IN_TYPE(const __vector4*)   
#endif // XBOX

#define IS_BUILT_IN_TYPE(T) ((bool)BIsBuiltInType<T>::Flag)

template<class T, bool F = IS_BUILT_IN_TYPE(T)> struct BEnsureBuiltInType;
template<class T> struct BEnsureBuiltInType<T, true> { };
   