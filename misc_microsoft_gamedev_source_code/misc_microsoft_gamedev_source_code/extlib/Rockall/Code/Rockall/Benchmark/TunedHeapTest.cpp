                          
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

#include "TunedHeapTest.hpp"
#include "New.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Constants local to the class.                                  */
    /*                                                                  */
    /*   The constants supplied here control the max number of heaps.   */
    /*                                                                  */
    /********************************************************************/

CONST SBIT32 MaxNumberOfHeaps		  = 32;

    /********************************************************************/
    /*                                                                  */
    /*   Class constructor.                                             */
    /*                                                                  */
    /*   Create a new test heap class and prepare it for use.           */
    /*                                                                  */
    /********************************************************************/

TUNED_HEAP_TEST::TUNED_HEAP_TEST( VOID )
	{
#ifdef CHEAT_BY_LEAVING_HEAP_ACTIVE
	REGISTER SBIT32 Count;

#endif
	//
	//   Set up the test framework.
	//
	Active = False;
	MaxThreads = 0;
	ZeroAll = False;

#ifdef CHEAT_BY_LEAVING_HEAP_ACTIVE
	//
	//   A number of heaps cheat by leaving the heap
	//   active between execution cycles of the benchmark.
	//   So we here we have a configuration to model this
	//   for 'SMP_HEAP'.
	//
	Heaps = ((FAST_HEAP*) malloc( (MaxNumberOfHeaps * sizeof(FAST_HEAP)) ));
	Threads = NULL;

	//
	//   Create all the heaps.
	//
	for ( Count=0;Count < MaxNumberOfHeaps;Count ++ )
		{
		PLACEMENT_NEW( & Heaps[ Count ],FAST_HEAP )
			(
			(32 * 4194304),
			True,
			True,
			True
			);
		}
#else
	//
	//   The normal and more honest configuration for heaps
	//   that play fair and don't cheat.
	//
	Heaps = NULL;
	Threads = NULL;
#endif
	}

    /********************************************************************/
    /*                                                                  */
    /*   Create a number of heaps.                                      */
    /*                                                                  */
    /*   A number of heaps are created and configured ready for         */
    /*   testing.  If this can not be done the call is failed.          */
    /*                                                                  */
    /********************************************************************/

BOOLEAN TUNED_HEAP_TEST::CreateHeaps
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
#ifdef CHEAT_BY_LEAVING_HEAP_ACTIVE
		//
		//   A number of heaps cheat by leaving the heap
		//   active between execution cycles of the benchmark.
		//   So we here we have a configuration to model this
		//   for 'SMP_HEAP'.
		//
		Threads = ((FAST_HEAP**) malloc( (NumberOfThreads * sizeof(FAST_HEAP*)) ));

		if ( NumberOfHeaps > MaxNumberOfHeaps )
			{ Failure( "MaxNumberOfHeaps limit exceeded in CreateHeaps" ); }
#else
		//
		//   Create space for the heaps and the thread
		//   pointers to the heaps.
		//
		Heaps = ((FAST_HEAP*) malloc( (NumberOfThreads * sizeof(FAST_HEAP)) ));

		Threads = ((FAST_HEAP**) malloc( (NumberOfThreads * sizeof(FAST_HEAP*)) ));
#endif

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
#ifndef CHEAT_BY_LEAVING_HEAP_ACTIVE

			//
			//   Create all the heaps.
			//
			for ( Count=0;Count < NumberOfThreads;Count ++ )
				{
				PLACEMENT_NEW( & Heaps[ Count ],FAST_HEAP )
					(
					4194304,
					True,
					(NumberOfHeaps != NumberOfThreads),
					ThreadSafe
					);
				}
#endif

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

VOID TUNED_HEAP_TEST::Delete
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
		if (! Threads[ Thread ] -> Delete( Address ))
			{ Failure( "Delete fails" ); }
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

CHAR *TUNED_HEAP_TEST::NameOfHeap( VOID )
	{ return ((CHAR*) "Rockall (Custom tuned heap (see docs))" ); }

    /********************************************************************/
    /*                                                                  */
    /*   Create a memory allocation.                                    */
    /*                                                                  */
    /*   Create a memory allocation on the selected heap.               */
    /*                                                                  */
    /********************************************************************/

VOID TUNED_HEAP_TEST::New
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
		//
		//   Create a memory allocation.
		//
		(*Address) = Threads[ Thread ] -> New( Size,NULL,ZeroAll );

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

VOID TUNED_HEAP_TEST::Resize
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
			Threads[ Thread ] -> Resize
				( 
				(*Address),
				NewSize,
				1,
				NULL,
				False,
				ZeroAll 
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

VOID TUNED_HEAP_TEST::DeleteHeaps( VOID )
	{
	//
	//   Ensure the test harness is still active.
	//
	if ( Active )
		{
#ifdef CHEAT_BY_LEAVING_HEAP_ACTIVE
		//
		//   A number of heaps cheat by leaving the heap
		//   active between execution cycles of the benchmark.
		//   So we here we have a configuration to model this
		//   for 'SMP_HEAP'.
		//
		free( Threads );
#else
		REGISTER SBIT32 Count;

		//
		//   Delete all the heaps.
		//
		for ( Count=0;Count < MaxThreads;Count ++ )
			{ PLACEMENT_DELETE( & Heaps[ Count ],FAST_HEAP ); }

		//
		//   Release any space.
		//
		free( Threads );
		free( Heaps );
#endif

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

TUNED_HEAP_TEST::~TUNED_HEAP_TEST( VOID )
	{
#ifdef CHEAT_BY_LEAVING_HEAP_ACTIVE
	REGISTER SBIT32 Count;

	//
	//   Delete all the heaps.
	//
	for ( Count=0;Count < MaxNumberOfHeaps;Count ++ )
		{ PLACEMENT_DELETE( & Heaps[ Count ],FAST_HEAP ); }

	//
	//   A number of heaps cheat by leaving the heap
	//   active between execution cycles of the benchmark.
	//   So we here we have a configuration to model this
	//   for 'SMP_HEAP'.
	//
	free( Heaps );

#endif
	//
	//   Ensure the test harness is not active.
	//
	if ( Active )
		{ Failure( "Test heap still active" ); }
	}
