
//                                        Ruler
//       1         2         3         4         5         6         7         8
//345678901234567890123456789012345678901234567890123456789012345678901234567890

    /********************************************************************/
    /*                                                                  */
    /*   The standard layout.                                           */
    /*                                                                  */
    /*   The standard layout for 'cpp' files in this code is as         */
    /*   follows:                                                       */
    /*                                                                  */
    /*      1. Include files.                                           */
    /*      2. Constants local to the class.                            */
    /*      3. Data structures local to the class.                      */
    /*      4. Data initializations.                                    */
    /*      5. Static functions.                                        */
    /*      6. Class functions.                                         */
    /*                                                                  */
    /*   The constructor is typically the first function, class         */
    /*   member functions appear in alphabetical order with the         */
    /*   destructor appearing at the end of the file.  Any section      */
    /*   or function this is not required is simply omitted.            */
    /*                                                                  */
    /********************************************************************/

#include "BenchmarkPCH.hpp"

#include "MultiSmartHeapTest.hpp"
#ifdef ENABLE_SMART_HEAP                       
	//
	//   Note: Must link with ..\..\..\Build\SmartHeap\Msvc\shlW32Mt.lib
	//
#include "../../../Build/SmartHeap/Include/SmrtHeap.hpp"
#include "../../../Build/SmartHeap/Include/ShMalloc.h"

    /********************************************************************/
    /*                                                                  */
    /*   A type to contain a smart heap pool.                           */
    /*                                                                  */
    /*   A type to contain a spart heap pool just for comile reasons.   */
    /*                                                                  */
    /********************************************************************/

class SMRTHEAP { public: MEM_POOL MemPool; };

    /********************************************************************/
    /*                                                                  */
    /*   Class constructor.                                             */
    /*                                                                  */
    /*   Create a new test heap class and prepare it for use.           */
    /*                                                                  */
    /********************************************************************/

MULTI_SMART_HEAP_TEST::MULTI_SMART_HEAP_TEST( VOID )
	{
	Active = False;
	MaxThreads = 0;
	ZeroAll = False;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Create a number of heaps.                                      */
    /*                                                                  */
    /*   A number of heaps are created and configured ready for         */
    /*   testing.  If this can not be done the call is failed.          */
    /*                                                                  */
    /********************************************************************/

BOOLEAN MULTI_SMART_HEAP_TEST::CreateHeaps
		( 
		SBIT32						  NumberOfHeaps,
		SBIT32						  NumberOfThreads,
		BOOLEAN						  ThreadSafe,
		BOOLEAN						  Zero
		)
	{
	//
	//   The test harnes must be idle and the number
	//   of heaps and threads must be positive.
	//
	if ( (! Active) && (NumberOfHeaps > 0) && (NumberOfThreads > 0) )
		{
		//
		//   Create space for the heaps and the thread
		//   pointers to the heaps.
		//
		Heaps = ((SMRTHEAP*) malloc( (NumberOfThreads * sizeof(SMRTHEAP)) ));

		Threads = ((SMRTHEAP**) malloc( (NumberOfThreads * sizeof(SMRTHEAP*)) ));

		//
		//   Ensure the allocations did not fail.
		//
		if ( (Heaps != NULL) && (Threads != NULL) )
			{
			REGISTER SBIT32 Count;

			//
			//   Set up the control information.
			//
			Active = True;
			MaxHeaps = NumberOfHeaps;
			MaxThreads = NumberOfThreads;
			ZeroAll = Zero;

			//
			//   Create all the heaps.
			//
			for ( Count=0;Count < NumberOfThreads;Count ++ )
				{
				Heaps[ Count ].MemPool = 
					(
					MemPoolInit
						(
						((ThreadSafe) ? MEM_POOL_SERIALIZE : NULL)
						)
					);

				if ( Heaps[ Count ].MemPool == NULL )
					{ Failure( "Heap create fails" ); }
				}

			//
			//   Create all the thread pointers.
			//
			for ( Count=0;Count < NumberOfThreads;Count ++ )
				{ Threads[ Count ] = & Heaps[ Count ]; }

			return True;
			}
		else
			{
			//
			//   Release any allocated memory.
			//
			if ( Heaps != NULL )
				{ free( Heaps ); }

			if ( Threads != NULL )
				{ free( Threads ); }
			}
		}

	return False;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Delete a memory allocation.                                    */
    /*                                                                  */
    /*   Delete a memory allocation of the selected heap.               */
    /*                                                                  */
    /********************************************************************/

VOID MULTI_SMART_HEAP_TEST::Delete
		( 
		VOID						  *Address,
		SBIT32						  Thread
		)
	{
	//
	//   Ensure the test harness is active and 
	//   that the thread number is valid.
	//
	if ( (Active) && (Thread >= 0) && (Thread < MaxThreads) )
		{
		//
		//   Delete the memory allocation.
		//
		delete( Address );
		}
	else
		{ Failure( "No test harness or thread" ); }
	}

    /********************************************************************/
    /*                                                                  */
    /*   The heap name.                                                 */
    /*                                                                  */
    /*   The name of the heap is returned to the caller.                */
    /*                                                                  */
    /********************************************************************/

CHAR *MULTI_SMART_HEAP_TEST::NameOfHeap( VOID )
	{ return ((CHAR*) "SmartHeap (Multiple heaps - Single heap image)" ); }

    /********************************************************************/
    /*                                                                  */
    /*   Create a memory allocation.                                    */
    /*                                                                  */
    /*   Create a memory allocation on the selected heap.               */
    /*                                                                  */
    /********************************************************************/

VOID MULTI_SMART_HEAP_TEST::New
		( 
		VOID						  **Address,
		SBIT32						  Size,
		SBIT32						  Thread
		)
	{
	//
	//   Ensure the test harness is active and 
	//   that the thread number is valid.
	//
	if ( (Active) && (Thread >= 0) && (Thread < MaxThreads) )
		{
		REGISTER DWORD Flags = ((ZeroAll) ? MEM_ZEROINIT : NULL);

		//
		//   Create a memory allocation.
		//
		(*Address) = new( (Threads[ Thread ] -> MemPool),Flags ) CHAR [ Size ];

		if ( (*Address) == NULL )
			{ Failure( "New fails" ); }
		}
	else
		{ Failure( "No test harness or thread" ); }
	}

    /********************************************************************/
    /*                                                                  */
    /*   Resize a memory allocation.                                    */
    /*                                                                  */
    /*   Resize a memory allocation on the selected heap.               */
    /*                                                                  */
    /********************************************************************/

VOID MULTI_SMART_HEAP_TEST::Resize
		( 
		VOID						  **Address,
		SBIT32						  NewSize,
		SBIT32						  Thread
		)
	{
	//
	//   Ensure the test harness is active and 
	//   that the thread number is valid.
	//
	if ( (Active) && (Thread >= 0) && (Thread < MaxThreads) )
		{
		//
		//   Resize a memory allocation.
		//
		(*Address) = 
			(
			realloc
				( 
				(*Address),
				NewSize
				)
			);

		if ( (*Address) == NULL )
			{ Failure( "Resize fails" ); }
		}
	else
		{ Failure( "No test harness or thread" ); }
	}

    /********************************************************************/
    /*                                                                  */
    /*   Delete all the heaps.                                          */
    /*                                                                  */
    /*   All the current heaps are deleted and as the test is over.     */
    /*                                                                  */
    /********************************************************************/

VOID MULTI_SMART_HEAP_TEST::DeleteHeaps( VOID )
	{
	//
	//   Ensure the test harness is still active.
	//
	if ( Active )
		{
		REGISTER SBIT32 Count;

		//
		//   Delete all the heaps.
		//
		for ( Count=0;Count < MaxThreads;Count ++ )
			{ MemPoolFree( Heaps[ Count ].MemPool ); }

		//
		//   Release any space.
		//
		free( Threads );
		free( Heaps );

		//
		//   Delete other control information.
		//
		ZeroAll = False;
		MaxThreads = 0;
		Active = False;
		}
	}

    /********************************************************************/
    /*                                                                  */
    /*   Class destructor.                                              */
    /*                                                                  */
    /*   Destory the test heap class.                                   */
    /*                                                                  */
    /********************************************************************/

MULTI_SMART_HEAP_TEST::~MULTI_SMART_HEAP_TEST( VOID )
	{
	//
	//   Ensure the test harness is not active.
	//
	if ( Active )
		{ Failure( "Test heap still active" ); }
	}
#endif
