                          
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

#include "LfhHeapTest.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Class constructor.                                             */
    /*                                                                  */
    /*   Create a new test heap class and prepare it for use.           */
    /*                                                                  */
    /********************************************************************/

LFH_HEAP_TEST::LFH_HEAP_TEST( VOID )
	{
	Active = False;
	MaxThreads = 0;
	ZeroAll = False;

	Heaps = NULL;
	Threads = NULL;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Create a number of heaps.                                      */
    /*                                                                  */
    /*   A number of heaps are created and configured ready for         */
    /*   testing.  If this can not be done the call is failed.          */
    /*                                                                  */
    /********************************************************************/

BOOLEAN LFH_HEAP_TEST::CreateHeaps
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
//	if ( (! Active) && (NumberOfHeaps > 0) && (NumberOfThreads > 0) )
	if ( (! Active) && (NumberOfThreads > 0) && (ThreadSafe) )
		{
		//
		//   Create space for the heaps and the thread
		//   pointers to the heaps.
		//
		Heaps = ((HANDLE*) malloc( (NumberOfHeaps * sizeof(HANDLE)) ));

		Threads = ((HANDLE**) malloc( (NumberOfThreads * sizeof(HANDLE*)) ));

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
			for ( Count=0;Count < NumberOfHeaps;Count ++ )
				{
				AUTO ULONG LfhFlag = 2;

				Heaps[ Count ] = 
					(
					HeapCreate
						(
						((ThreadSafe) ? NULL : HEAP_NO_SERIALIZE),
						0,
						0
						)
					);

				if ( Heaps[ Count ] == NULL )
					{ Failure( "Heap create fails" ); }

#ifndef _XBOX
				if 
						( 
						HeapSetInformation
							( 
							Heaps[ Count ],
							HeapCompatibilityInformation,
							((PVOID) & LfhFlag),
							sizeof(ULONG) 
							) == NULL
						)
					{ Failure( "LFH not enabled" ); }
#endif					
				}

			//
			//   Create all the thread pointers.
			//
			for ( Count=0;Count < NumberOfThreads;Count ++ )
				{ Threads[ Count ] = & Heaps[ (Count % NumberOfHeaps) ]; }

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

VOID LFH_HEAP_TEST::Delete
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
		if (! HeapFree( (*Threads[ Thread ]),0,Address ) )
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

CHAR *LFH_HEAP_TEST::NameOfHeap( VOID )
	{ return ((CHAR*) "Windows LFH Heap (SMP heap)" ); }

    /********************************************************************/
    /*                                                                  */
    /*   Create a memory allocation.                                    */
    /*                                                                  */
    /*   Create a memory allocation on the selected heap.               */
    /*                                                                  */
    /********************************************************************/

VOID LFH_HEAP_TEST::New
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
		REGISTER DWORD Flags = ((ZeroAll) ? HEAP_ZERO_MEMORY : NULL);

		//
		//   Create a memory allocation.
		//
		(*Address) = HeapAlloc( (*Threads[ Thread ]),Flags,Size );

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

VOID LFH_HEAP_TEST::Resize
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
		REGISTER DWORD Flags = ((ZeroAll) ? HEAP_ZERO_MEMORY : NULL);

		//
		//   Resize a memory allocation.
		//
		(*Address) = 
			(
			HeapReAlloc
				(
				(*Threads[ Thread ]),
				Flags,
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

VOID LFH_HEAP_TEST::DeleteHeaps( VOID )
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
		for ( Count=0;Count < MaxHeaps;Count ++ )
			{ HeapDestroy( Heaps[ Count ] ); }

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

LFH_HEAP_TEST::~LFH_HEAP_TEST( VOID )
	{
	//
	//   Ensure the test harness is not active.
	//
	if ( Active )
		{ Failure( "Test heap still active" ); }
	}
