#ifndef _FEATURES_HPP_
#define _FEATURES_HPP_
//                                        Ruler
//       1         2         3         4         5         6         7         8
//345678901234567890123456789012345678901234567890123456789012345678901234567890

    /********************************************************************/
    /*                                                                  */
    /*   The standard layout.                                           */
    /*                                                                  */
    /*   The standard layout for 'hpp' files for this code is as        */
    /*   follows:                                                       */
    /*                                                                  */
    /*      1. Include files.                                           */
    /*      2. Constants exported from the class.                       */
    /*      3. Data structures exported from the class.                 */
	/*      4. Forward references to other data structures.             */
	/*      5. Class specifications (including inline functions).       */
    /*      6. Additional large inline functions.                       */
    /*                                                                  */
    /*   Any portion that is not required is simply omitted.            */
    /*                                                                  */
    /********************************************************************/

#include "Standard.hpp"
#include "System.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Active project list.	 							            */
    /*                                                                  */
    /*   The active project list contains all of the projects that      */
    /*   are currently selected.                                        */
    /*                                                                  */
    /********************************************************************/

//#define COMPILING_PIPELINE_SERVER	  1
#define COMPILING_ROCKALL_IV		  1
//#define COMPILING_TRIGGERS		  1
//#define COMPILING_WEB_SERVER		  1

    /********************************************************************/
    /*                                                                  */
    /*   Active fetaure lists.	 							            */
    /*                                                                  */
    /*   The active features list contain all of the features that      */
    /*   are currently selected for each project.                       */
    /*                                                                  */
    /********************************************************************/

#ifdef COMPILING_PIPELINE_SERVER
#ifdef DEBUGGING
#define COMPILING_ROCKALL_LIBRARY	  1			// Enable for testing.
//#define DISABLE_MULTIPLE_PROCESSORS	  1		// Disable for testing.
#define ENABLE_LOCK_STATISTICS		  1			// Enable for testing.
#endif
#define DISABLE_PRECOMPILED_HEADERS	  1		// Disable for testing.
#define DISABLE_STRUCTURED_EXCEPTIONS 1			// Disable for product groups.
#define ENABLE_OUTPUT_DEBUG_STRING    1
#define ENABLE_DEBUG_FILE             1			// Enable for testing.
#define ENABLE_GLOBAL_ROCKALL		  1			// Enable for pure speed.
#define ENABLE_NON_STANDARD_ASSEMBLY  1			// Enable for pure speed.
//#define PRINT_ACTIVE_PACKETS		  1			// Enable for testing.
#define SELECT_CACHE_LINE_SIZE		  32		// Set for Pentium III
#endif

#ifdef COMPILING_ROCKALL_IV

   #if defined(DEBUGGING) || defined(PROFILING)
	   #define ENABLE_ALLOCATION_STATISTICS  1    // Display stats on allocations. Debug only
	   #define ENABLE_HEAP_STATISTICS        1			// Enable stats.
	   #define ENABLE_OUTPUT_DEBUG_STRING    0
	   #define ENABLE_DEBUG_FILE             0			// Create a debug file.
   #else
	   #define DISABLE_DEBUG_HELP			  1			//retail builds shouldn't have debug help
   #endif
//	#define DISABLE_ATOMIC_FLAGS		  1			// Disable for product groups.
	#define DISABLE_GLOBAL_NEW			  1			// Disable for product groups.
	// Turning on Heap Use doesn't compile on either PC or Xenon. 
	#define DISABLE_HEAP_USE			  1			// Disable for product groups.
//	#define DISABLE_MULTIPLE_PROCESSORS	  1			// Disable for testing.
//	#define DISABLE_PRECOMPILED_HEADERS	  1			// Disable for testing.
	#define DISABLE_STRUCTURED_EXCEPTIONS 1			// Disable for product groups.
	#define ENABLE_NON_STANDARD_ASSEMBLY  1			// Enable for pure speed. (turns on PREFETCH and other intrinsics)
//	#define ENABLE_RECURSIVE_LOCKS		  1		// Enable to save lost of TLS.
	#define SELECT_CACHE_LINE_SIZE		  128		// Set for Pentium IV
	#ifdef _XENON
		#define USE_LARGE_PAGES				  1		// Use this to use the large pages on allocations to improve TLB misses. (XBOX ONLY!!!)
		
		// rg [2/11/08] - Do not check this in! Only useful for generating statistics used by HeapTune. Be sure to also define ENABLE_DEBUG_FILE
		//#define ENABLE_HEAP_STATISTICS        1			// Enable stats.
		//#undef ENABLE_DEBUG_FILE
		//#define ENABLE_DEBUG_FILE             1
	#endif
	#ifndef COMPILING_ROCKALL_LIBRARY
		#define COMPILING_ROCKALL_LIBRARY	  1			// Enable for testing.
	#endif
#endif

#ifdef COMPILING_TRIGGERS
#define DISABLE_DEBUG_HELP			  1
//#define DISABLE_MULTIPLE_PROCESSORS	  1		// Disable for testing.
#define DISABLE_PRECOMPILED_HEADERS	  1			// Disable for testing.
//#define DISABLE_STRUCTURED_EXCEPTIONS 1			// Disable for product groups.
#define ENABLE_OUTPUT_DEBUG_STRING    1
#define ENABLE_DEBUG_FILE             1			// Create a debug file.
#define ENABLE_GLOBAL_ROCKALL		  1			// Enable for pure speed.
//#define ENABLE_NON_STANDARD_ASSEMBLY  1			// Enable for pure speed.
//#define ENABLE_RECURSIVE_LOCKS		  1		// Enable to save lost of TLS.
#define SELECT_CACHE_LINE_SIZE		  32		// Set for Pentium III
#endif

#ifdef COMPILING_WEB_SERVER
#ifdef DEBUGGING
#define ENABLE_LOCK_STATISTICS		  1			// Enable for testing.
#define PRINT_ACTIVE_PACKETS		  1			// Enable for testing.
#endif
#define ENABLE_OUTPUT_DEBUG_STRING    1
#define ENABLE_DEBUG_FILE             1			// Create a debug file.
#define ENABLE_NON_STANDARD_ASSEMBLY  1			// Enable for pure speed.
#define SELECT_CACHE_LINE_SIZE		  32		// Set for Pentium III
#endif

    /********************************************************************/
    /*                                                                  */
    /*   The complete feature list.							            */
    /*                                                                  */
    /*   The code supports a significant number of optional features    */
    /*   which are listed in this file.  Any feature can be activated   */
    /*   by copying the approriate setting.  Please be sure to keep     */
    /*   all the flags up to date and leave at list one copy of each    */
    /*   flag below.                                                    */
    /*                                                                  */
    /********************************************************************/

#ifdef ACTIVATE_ALL_OPTIONS
	//
	//   Standard options for all code.
	//
#define ASSEMBLY_X86				  1
#define DEBUGGING                     1

#define DISABLE_GLOBAL_NEW			  1
#define DISABLE_PRECOMPILED_HEADERS	  1
#define DISABLE_STRUCTURED_EXCEPTIONS 1

	//
	//   Standard options for the library code.
	//
#define ENABLE_OUTPUT_DEBUG_STRING    1
#define ENABLE_DEBUG_FILE             1
#define ENABLE_GLOBAL_ROCKALL		  1
#define ENABLE_LOCK_STATISTICS		  1
#define ENABLE_NON_STANDARD_ASSEMBLY  1
#define ENABLE_RECURSIVE_LOCKS		  1

#define DISABLE_ATOMIC_FLAGS		  1
#define DISABLE_DEBUG_HELP			  1
#define DISABLE_HEAP_USE			  1
#define DISABLE_MULTIPLE_PROCESSORS	  1
#define DISABLE_STRING_LOCKS		  1

#define SELECT_CACHE_LINE_SIZE		  32

	//
	//   Rockall specific options.
	//
#define COMPILING_ROCKALL_DLL		  1
#define COMPILING_ROCKALL_LIBRARY	  1
#define ENABLE_ALLOCATION_STATISTICS  1
#define ENABLE_HEAP_STATISTICS        1
#define NO_DEFAULT_HEAP				  1

	//
	//   Pipeline Server specific options.
	//
#define ENABLE_BUFFER_LOCK			  1
#define ENABLE_ZERO_WRITE_BUFFER	  1

#define PRINT_ACTIVE_PACKETS		  1

	//
	//   Pipeline Server demo specific options.
	//
#define ENABLE_DATABASE				  1
#define ENABLE_READING				  1
#define ENABLE_TRANSACTIONS			  1
#define ENABLE_WRITING				  1

	//
	//   Web Server specific options.
	//
#define DISABLE_ASYNC_IO			  1
#define DISABLE_BUFFER_COPY			  1
#define DISABLE_WEB_LOCKS			  1
#endif
#endif
