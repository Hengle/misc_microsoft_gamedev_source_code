#ifndef _TEST_HEAPS_HPP_
#define _TEST_HEAPS_HPP_
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

    /********************************************************************/
    /*                                                                  */
    /*   Constants exported from the class.                             */
    /*                                                                  */
    /*   The constants supplied here control the creation and           */
    /*   execution of the benchmark.                                    */
    /*                                                                  */
    /********************************************************************/

CONST SBIT32 MaxAddress				  = 10000;
CONST SBIT32 MaxCommands			  = 10000000;
CONST SBIT32 MaxSize				  = 1200;
CONST SBIT32 MaxThreads				  = 16;
CONST SBIT32 ResizeRate				  = 16;

    /********************************************************************/
    /*                                                                  */
    /*   A heap test harness                                            */
    /*                                                                  */
    /*   The heap test harness exposes a common interface for all       */
    /*   the heap that are tested.                                      */
    /*                                                                  */
    /********************************************************************/

class TEST_HEAPS
    {
    public:
        //
        //   Public functions.
        //
        TEST_HEAPS( VOID )
			{ /* void */ }

		//
		//   The benchmark interface.
		//
		BOOLEAN ExecuteTestThreads
			(
			SBIT32					  NumberOfHeaps,
			SBIT32					  NumberOfThreads,
			SBIT32					  *Time,
			BOOLEAN					  ThreadSafe,
			BOOLEAN					  Zero
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

        ~TEST_HEAPS( VOID )
			{ /* void */ }

	private:
		//
		//   Static private functions.
		//
		STATIC VOID ExecuteTestThread( VOID *Parameter );

		//
        //   Disabled operations.
        //
        TEST_HEAPS( CONST TEST_HEAPS & Copy );

        VOID operator=( CONST TEST_HEAPS & Copy );
    };
#endif
