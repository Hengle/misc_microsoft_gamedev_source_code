//============================================================================
//
//  BuildOptions.h
//  
// Copyright (c) 2000-2006, Ensemble Studios
//
//============================================================================
#pragma once

//----------------------------------------------------------------------------
//  Build Configuration
//----------------------------------------------------------------------------
#if defined(BUILD_CHECKED)
   //-- Checked Build Configuration
   
   #ifndef BUILD_DEBUG
      #error BUILD_DEBUG must also be defined in checked builds.
   #endif
      
   // jce [11/12/2002] -- turns on debugonly and normal asserts
   //#define ENABLE_BASSERT_DEBUGONLY
   #define ENABLE_BASSERT_NORMAL

   // rg [1/27/05] - Enables noreturn versions of the assert/fail macros, which prevents leaf funcs from being changed to non-leaf funcs on 360.
   #define ENABLE_BASSERT_NORETURN

   // jce [11/21/2002] -- need to define DEBUG for backwards compat with older code
   #define DEBUG

   #define DEBUG_VALIDATE_LIST_HANDLES 0
   
   // rg [9/13/2004] -- enable debug/normal range checking
   #define ENABLE_DEBUG_RANGE_CHECKS
   #define ENABLE_NORMAL_RANGE_CHECKS
         
   #ifdef XBOX
      #define ALLOCATION_LOGGER 1
   #endif

   #define TRACK_HEAP_UTILIZATION 0
   
   #define DYNAMIC_ARRAY_TRACKING 0
   #define FREELIST_TRACKING 0  
#elif defined(BUILD_DEBUG)

   //-- Debug Build Configuration
      
   // jce [11/12/2002] -- turns on debugonly and normal asserts
   #define ENABLE_BASSERT_DEBUGONLY
   #define ENABLE_BASSERT_NORMAL

   // rg [1/27/05] - Enables noreturn versions of the assert/fail macros, which prevents leaf funcs from being changed to non-leaf funcs on 360.
   #define ENABLE_BASSERT_NORETURN

   // jce [11/21/2002] -- need to define DEBUG for backwards compat with older code
   #define DEBUG

   #define DEBUG_VALIDATE_LIST_HANDLES 0
   
   // rg [9/13/2004] -- enable debug/normal range checking
   #define ENABLE_DEBUG_RANGE_CHECKS
   #define ENABLE_NORMAL_RANGE_CHECKS
         
   #ifdef XBOX
      #define ALLOCATION_LOGGER 1
   #endif

   #define TRACK_HEAP_UTILIZATION 0
   
   #define DYNAMIC_ARRAY_TRACKING 0
   #define FREELIST_TRACKING 0
#elif defined(BUILD_PLAYTEST)

   //-- Playtest Build Configuration

   // jce [11/12/2002] -- turns on normal asserts
   #define ENABLE_BASSERT_NORMAL
   
   #define ENABLE_BASSERT_NORETURN
   
   //#define ENABLE_BASSERT_NORETURN

   #define DEBUG_VALIDATE_LIST_HANDLES 0
   
   // rg [9/13/2004] -- enable normal range checking
   #define ENABLE_NORMAL_RANGE_CHECKS
   
   #ifdef XBOX
      #define ALLOCATION_LOGGER 1
   #endif
   
   #define DYNAMIC_ARRAY_TRACKING 0
   #define FREELIST_TRACKING 0
            
#elif defined(BUILD_PROFILE)
      
   #define DEBUG_VALIDATE_LIST_HANDLES 0
         
   #ifdef XBOX
      #define ALLOCATION_LOGGER 1
   #endif
   
   #define TRACK_HEAP_UTILIZATION 0      
   #define DYNAMIC_ARRAY_TRACKING 0
   #define FREELIST_TRACKING 0
            
#else  //-- defined(BUILD_FINAL)

   #if !defined(BUILD_FINAL)
      #pragma message("Must define BUILD_DEBUG, BUILD_PLAYTEST, BUILD_PROFILE, or BUILD_FINAL -- BUILD_FINAL assumed!")
   #endif
   
   #define DEBUG_VALIDATE_LIST_HANDLES 0
            
#endif
