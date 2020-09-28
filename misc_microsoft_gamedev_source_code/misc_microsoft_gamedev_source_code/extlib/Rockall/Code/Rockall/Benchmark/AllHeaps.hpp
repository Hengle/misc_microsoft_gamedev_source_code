#ifndef _ALL_HEAPS_HPP_
#define _ALL_HEAPS_HPP_
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

#include "Global.hpp"

#include "Hash.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Constants exported from the class.                             */
    /*                                                                  */
    /*   The constants supplied here control the creation and           */
    /*   execution of the benchmark.                                    */
    /*                                                                  */
    /********************************************************************/

//#define ALLOC_FREE_TEST				  1
#define CACHE_TEST					  1
//#define NO_ALLOC_TEST				  1

#ifdef ALLOC_FREE_TEST
CONST SBIT32 MaxAddress				  = 500000;
CONST SBIT32 MaxCommands			  = 1000000;
#endif

#ifdef CACHE_TEST
CONST SBIT32 MaxAddress				  = 100000;
CONST SBIT32 MaxCommands			  = 10000000;
#endif

#ifdef NO_ALLOC_TEST
CONST SBIT32 MaxAddress				  = 256;
CONST SBIT32 MaxCommands			  = 1000000;
#endif

CONST SBIT32 BaseSize				  = 8;
CONST SBIT32 MaxBaseShift			  = 8;
CONST SBIT32 ResizeRate				  = 64;

CONST SBIT32 MinTest				  = 1;
CONST SBIT32 MaxTest				  = 2;
//CONST SBIT32 MaxTest				  = 12;

//CONST SBIT32 MinThreads				  = 1;
//CONST SBIT32 MaxThreads				  = 8;

CONST SBIT32 MinThreads				  = 1;
CONST SBIT32 MaxThreads				  = 4;

    /********************************************************************/
    /*                                                                  */
    /*   Data structures exported from the class.                       */
    /*                                                                  */
    /*   A common selection of data structures need to be exported      */
    /*   to ensure consistent execution.                                */
    /*                                                                  */
    /********************************************************************/

typedef enum
	{
	MeasureCache					  = 1,
	MeasureCacheSpans				  = 2,
	MeasureLocks					  = 4,
	MeasurePages					  = 8,
	MeasurePageSpans				  = 16,
	MeasureSetup					  = 32,
	MeasureSharing					  = 64,
	MeasureSpace					  = 128,
	MeasureTime						  = 256,
	MeasureZeros					  = 512
	}
BENCHMARK_FLAGS;

    /********************************************************************/
    /*                                                                  */
    /*   A heap test harness                                            */
    /*                                                                  */
    /*   The heap test harness exposes a common interface for all       */
    /*   the heap that are tested.                                      */
    /*                                                                  */
    /********************************************************************/

class ALL_HEAPS
    {
		//
		//   Private type definitions.
		//
		typedef HASH<SNATIVE,SNATIVE,FULL_LOCK> HASH_TABLE;

		typedef struct
			{
			ALL_HEAPS				  *AllHeaps;
			BENCHMARK_FLAGS			  BenchmarkFlags;
			LONG					  *Cost;
			HASH_TABLE				  *Hash;
			SBIT32					  NumberOfThreads;
			SBIT32					  Stride;
			SBIT32					  Thread;
			}
		THREAD_INFO;

    public:
        //
        //   Public functions.
        //
        ALL_HEAPS( VOID )
			{ /* void */ }

		//
		//   The benchmark interface.
		//
		BOOLEAN ExecuteTestThreads
			(
			BENCHMARK_FLAGS			  BenchmarkFlags,
			LONG					  *Cost,
			SBIT32					  NumberOfHeaps,
			SBIT32					  NumberOfThreads
			);

		//
		//   The benchmark heap interface.
		//
		VIRTUAL BOOLEAN CreateHeaps
			( 
			SBIT32					  NumberOfHeaps,
			SBIT32					  NumberOfThreads,
			BOOLEAN					  ThreadSafe,
			BOOLEAN					  Zero
			) = 0;

		VIRTUAL VOID Delete
			( 
			VOID					  *Address,
			SBIT32					  Heap 
			) = 0;

		VIRTUAL CHAR *NameOfHeap( VOID ) = 0;

		VIRTUAL VOID New
			( 
			VOID					  **Address,
			SBIT32					  Heap,
			SBIT32					  Size 
			) = 0;

		VIRTUAL VOID Resize
			( 
			VOID					  **Address,
			SBIT32					  Heap,
			SBIT32					  NewSize 
			) = 0;

		VIRTUAL VOID DeleteHeaps( VOID ) = 0;

        VIRTUAL ~ALL_HEAPS( VOID )
			{ /* void */ }

	private:
		//
		//   Static private functions.
		//
		STATIC SNATIVE CommittedSpace( VOID );

		STATIC VOID ExecuteTestThread( VOID *Parameter );

		//
        //   Disabled operations.
        //
        ALL_HEAPS( CONST ALL_HEAPS & Copy );

        VOID operator=( CONST ALL_HEAPS & Copy );
    };
#endif
