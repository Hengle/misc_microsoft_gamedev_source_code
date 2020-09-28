#ifndef _ROCKALL_FRONT_END_HPP_
#define _ROCKALL_FRONT_END_HPP_
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

#include <stddef.h>
#include "Defines.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   The hardware word size.                                        */
    /*                                                                  */
    /*   We need to deduce the word size for the current hardware so    */
    /*   we can create suitable constants and interfaces.               */
    /*                                                                  */
    /********************************************************************/

#ifndef _WIN64
typedef __w64 __int32				  SNATIVE;
typedef __w64 unsigned __int32		  UNATIVE;
#else
typedef __int64						  SNATIVE;
typedef unsigned __int64			  UNATIVE;
#endif

    /********************************************************************/
    /*                                                                  */
    /*   Memory allocation constants.                                   */
    /*                                                                  */
    /*   The memory allocation constants are denote special situations  */
    /*   where optimizations are possible or failures have cccured.     */
    /*                                                                  */
    /********************************************************************/

const SNATIVE AllocationFailure		  = 0;
const SNATIVE GuardMask				  = (sizeof(void*)-1);
const SNATIVE GuardSize				  = sizeof(void*);
const SNATIVE HalfMegabyte			  = (512 * 1024);
const SNATIVE NoSize				  = -1;

#ifndef _WIN64
const SNATIVE GuardValue			  = 0xDeadBeef;
#else
const SNATIVE GuardValue			  = 0xDeadBeefDeadBeef;
#endif


    /********************************************************************/
    /*                                                                  */
    /*   Class forward references.                                      */
    /*                                                                  */
    /*   We need to refer to the following classes before they are      */
    /*   fully specified so here we list them as forward references.    */
    /*                                                                  */
    /********************************************************************/

class CACHE;
class FIND;
class HEAP;
class NEW_PAGE;
class ROCKALL_BACK_END;
class THREAD_SAFE;


    /********************************************************************/
    /*                                                                  */
    /*   The memory allocation interface.                               */
    /*                                                                  */
    /*   The memory allocator can be configured in a wide variety       */
    /*   of ways to closely match the needs of specific programs.       */
    /*   The interface outlined here can be overloaded to support       */
    /*   whatever customization is necessary.                           */
    /*                                                                  */
    /********************************************************************/

class ROCKALL_DLL_LINKAGE ROCKALL_FRONT_END
    {
    public:
		//
		//   Public types.
		//
		//   A heap is constructed of a collection of 
		//   fixed sized buckets each with an associated
		//   cache.  The details of these buckets are
		//   supplied to the heap using the following
		//   structure.
		//
		typedef struct
			{
			int						  AllocationSize;
			int						  CacheSize;
			int						  ChunkSize;
			int						  PageSize;
			}
		CACHE_DETAILS;

		//
		//   Public data.
		//
		//   The internals linkages in a heap are built
		//   dynamically during the execution of a heaps
		//   constructor.  The member that follow relate
		//   to key internal classes.
		//
		CACHE						  **Array;
		CACHE						  *Caches;
		HEAP						  *Heap;
		NEW_PAGE					  *NewPage;
		FIND						  *PrivateFind;
		FIND						  *PublicFind;
		ROCKALL_BACK_END			  *RockallBackEnd;
		THREAD_SAFE					  *ThreadSafe;
				
		//
		//   A heap constructor is required to preserve 
		//   a small amount of information for the heap
		//   destructor.
		//
		bool						  GlobalDelete;
		SNATIVE						  GuardWord;
		int							  NumberOfCaches;
		int							  TotalSize;
		
		DWORD                  OwnerThread;
		bool                   ThreadSafeFlag;

        //
        //   Public functions.
		//
		//   A heaps public interface consists of a number
		//   of groups of related APIs.
        //
        ROCKALL_FRONT_END
			(
			CACHE_DETAILS			  *Caches1,
			CACHE_DETAILS			  *Caches2,
			int						  FindCacheSize,
			int						  FindCacheThreshold,
			int						  FindSize,
			int						  MaxFreeSpace,
			int						  *NewPageSizes,
			ROCKALL_BACK_END		  *NewRockallBackEnd,
			bool					  Recycle,
			bool					  SingleImage,
			int						  Stride1,
			int						  Stride2,
			bool					  ThreadSafeFlag
			);

		//
		//   Manipulate allocations.
		//
		//   The first group of functions manipulate 
		//   single or small arrays of allocations. 
		//
		virtual bool Delete
			( 
			void					  *Address,
			int						  Size = NoSize 
			);

		virtual bool Details
			( 
			void					  *Address,
			int						  *Space = NULL 
			);

		virtual bool KnownArea( void *Address );

		virtual bool MultipleDelete
			( 
			int						  Actual,
			void					  *Array[],
			int						  Size = NoSize
			);

		virtual bool MultipleNew
			( 
			int						  *Actual,
			void					  *Array[],
			int						  Requested,
			int						  Size,
			int						  *Space = NULL,
			bool					  Zero = false
			);

		virtual void *New
			( 
			int						  Size,
			int						  *Space = NULL,
			bool					  Zero = false
			);

		virtual void *Resize
			( 
			void					  *Address,
			int						  NewSize,
			int						  Move = -64,
			int						  *Space = NULL,
			bool					  NoDelete = false,
			bool					  Zero = false
			);

		virtual bool Verify
			( 
			void					  *Address = NULL,
			int						  *Space = NULL 
			);

		//
		//   Manipulate the heap.
		//
		//   The second group of functions act upon a heap
		//   as a whole.
		//
		virtual void DeleteAll( bool Recycle = true );

		virtual void LockAll( void );

		virtual bool Truncate( int MaxFreeSpace = 0 );

		virtual void UnlockAll( void );

		virtual bool Walk
			(
			bool					  *Active,
			void					  **Address,
			int						  *Space = NULL
			);

        virtual ~ROCKALL_FRONT_END( void );

		//
		//   Public inline functions.
		//
		inline bool Available( void )
			{ return (GuardWord == GuardValue); }

		inline bool Corrupt( void )
			{ return (GuardWord != GuardValue); }

			//	aleksger -
			//	Print internal statistics for runtime information
		void PrintMemStatistics( void );
		
		void SetOwnerThread( DWORD ownerThread = GetCurrentThreadId() ) { OwnerThread = ownerThread; }
	protected:
		//
		//   Protected inline functions.
		//
		//   A heap needs to compute the size of certain
		//   user supplied structures.  This task is 
		//   performed by the following function.
		//
		int ComputeSize( char *Array,int Stride );

		//
		//   Execptional situations.
		//
		//   The third group of functions are called in
		//   exceptional situations.
		//
		virtual void Exception(const char *Message );

		//
		//   We would like to allow access to the internal
		//   heap allocation function from classes that 
		//   inherit from the heap.  The memory supplied by
		//   this function survies all heap operations and
		//   is cleaned up as part of heap deletion.
		//
		virtual void *SpecialNew( int Size );

	private:
        //
        //   Disabled operations.
		//
		//   All copy constructors and class assignment 
		//   operations are disabled.
        //
        ROCKALL_FRONT_END( const ROCKALL_FRONT_END & Copy );

        void operator=( const ROCKALL_FRONT_END & Copy );
    };
#endif
