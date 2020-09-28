                          
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

#include "Barrier.hpp"
#include "TestHeaps.hpp"
#include "Thread.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Static structure initialization.                               */
    /*                                                                  */
    /*   Static structure initialization sets the initial value for     */
    /*   all static structures.                                         */
    /*                                                                  */
    /********************************************************************/

STATIC BARRIER Barrier;
STATIC LONG GlobalThreadID = 0;
STATIC THREAD Threads;

    /********************************************************************/
    /*                                                                  */
    /*   Execute a heap test.                                           */
    /*                                                                  */
    /*   Create a number of heaps and threads and run a test.           */
    /*                                                                  */
    /********************************************************************/

BOOLEAN TEST_HEAPS::ExecuteTestThreads
		(
		SBIT32						  NumberOfHeaps,
		SBIT32						  NumberOfThreads,
		SBIT32						  *Time,
		BOOLEAN						  ThreadSafe,
		BOOLEAN						  Zero
		)
	{
	AUTO SNATIVE Data[2] = { 0,0 };

	//
	//   Create all the heaps.
	//
	if ( CreateHeaps( NumberOfHeaps,NumberOfThreads,ThreadSafe,Zero ) )
		{
		REGISTER SBIT32 Count;

		//
		//   Setup the control information for 
		//   all the threads.
		//
		Data[0] = NumberOfThreads;
		Data[1] = ((SNATIVE) this);

		GlobalThreadID = 0;

		//
		//   Get the start time.
		//
		(*Time) = ((SBIT32) GetTickCount());

		//
		//   Start all the threads.
		//
		for ( Count=0;Count < NumberOfThreads;Count ++ )
			{
			Threads.StartThread
				( 
				((NEW_THREAD) ExecuteTestThread),
				((VOID*) Data)
				); 
			}

		//
		//   Wait for all threads to complete.
		//
		(VOID) Threads.WaitForThreads();

		//
		//   Get the end time.
		//
		(*Time) = (((SBIT32) GetTickCount()) - (*Time));

		//
		//   Delete all the heaps.
		//
		DeleteHeaps();

		return True;
		}

	return False;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Execute a test thread.                                         */
    /*                                                                  */
    /*   Execute a test thread on each heap as fast as possible.        */
    /*                                                                  */
    /********************************************************************/

VOID TEST_HEAPS::ExecuteTestThread( VOID *Parameter )
	{
	REGISTER SNATIVE *Data = ((SNATIVE*) Parameter);

	TRY
		{
		REGISTER SBIT32 NumberOfThreads = ((SBIT32) Data[0]);

		//
		//   Ensure the thread data looks sane.
		//
		if ( (NumberOfThreads >= 0) && (GlobalThreadID >= 0) )
			{
			AUTO VOID *Allocation[ MaxAddress ];
			REGISTER SBIT32 Count;
			REGISTER TEST_HEAPS *TestHeap = ((TEST_HEAPS*) Data[1]);
			REGISTER SBIT32 Thread = (InterlockedIncrement( & GlobalThreadID )-1);

			//
			//   Wait for all the threads to arrive.
			//
			Barrier.Synchronize( NumberOfThreads );

			//
			//   Decrement the ID to be tidy.
			//
			InterlockedDecrement( & GlobalThreadID );

			//
			//   Create a number of allocations.
			//
			for ( Count=0;Count < MaxAddress;Count ++ )
				{ 
				REGISTER SBIT32 Size = ((rand() % MaxSize) + 1);

				TestHeap -> New( & Allocation[ Count ],Size,Thread );
				}

			//
			//   Generate a random allocation pattern.
			//
			for ( Count=0;Count < (MaxCommands - (MaxAddress*2));Count ++ )
				{
				REGISTER VOID **Address = & Allocation[ (rand() % MaxAddress) ];
				REGISTER SBIT32 Size = ((rand() % MaxSize) + 1);

				//
				//   We randomly insert a 'Resize' operation
				//   into the command stream.
				//
				if ( (rand() % ResizeRate) != 1 )
					{
					//
					//   We have picked an allocation to
					//   replace to delete the current
					//   pointer and create a new a new
					//   pointer.
					//
					TestHeap -> Delete( (*Address),Thread );

					TestHeap -> New( Address,Size,Thread );

					Count ++;
					}
				else
					{ TestHeap -> Resize( Address,Size,Thread ); }
				}

			//
			//   Delete all outstanding of allocations.
			//
			for ( Count=(MaxAddress-1);Count >= 0;Count -- )
				{ TestHeap -> Delete( Allocation[ Count ],Thread ); }
			}
		else
			{ Failure( "Thread data is wrong" ); }
		}
#ifdef DISABLE_STRUCTURED_EXCEPTIONS
	catch ( FAULT &Message )
		{
		//
		//   Print the exception message and exit.
		//
		DebugPrint( "Exception caught: %s\n",(char*) Message );

		exit(1);
		}
	catch ( ... )
		{
		//
		//   Print the exception message and exit.
		//
		DebugPrint( "Exception caught: (unknown)\n" );

		exit(1);
		}
#else
	__except ( EXCEPTION_EXECUTE_HANDLER ) 
		{ 
		//
		//   Print the exception message and exit.
		//
		DebugPrint( "Structured exception caught: (unknown)\n" ); 

		exit(1);
		}
#endif
	}
