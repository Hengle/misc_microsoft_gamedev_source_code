//==============================================================================
// XMLTypes.h
//
// Copyright (c) 2001, Ensemble Studios
//==============================================================================

#ifndef _XMLTypes_H_
#define _XMLTypes_H_

#ifndef XBOX

//==============================================================================
// Includes

// MSXML doesn't play nice with the memory manager
#ifdef new 
   #define crappy
   #undef new
#endif

#ifdef _XML30
#import "msxml3.dll" raw_interfaces_only
#else
#import "msxml4.dll" raw_interfaces_only
#endif

// MSXML doesn't play nice with the memory manager
#ifdef crappy
   #define new BANG_NEW
   #undef crappy
#endif

using namespace MSXML2;

#endif // XBOX

//==============================================================================
#endif // _XMLTypes_H_

//==============================================================================
// eof: XMLTypes.h
//==============================================================================