                          
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

#include "AllHeaps.hpp"
#include "Barrier.hpp"
#include "Common.hpp"
#include "Thread.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Structures local to the class.                                 */
    /*                                                                  */
    /*   We have a number of structures that keep track of the          */
    /*   memory allocations while the benchmark is running.             */
    /*                                                                  */
    /********************************************************************/

typedef struct
	{
	VOID								  *Address;
	SBIT32								  Size;
	}
ALLOCATIONS;

    /********************************************************************/
    /*                                                                  */
    /*   Static structure initialization.                               */
    /*                                                                  */
    /*   Static structure initialization sets the initial value for     */
    /*   all static structures.                                         */
    /*                                                                  */
    /********************************************************************/

STATIC LONG Active;
STATIC BARRIER Barrier1;
STATIC BARRIER Barrier2;
STATIC BARRIER Barrier3;
STATIC BARRIER Barrier4;
STATIC THREAD Threads;

    /********************************************************************/
    /*                                                                  */
    /*   Compute the amount of memory.                                  */
    /*                                                                  */
    /*   Compute the amount of memory used by walking the allocated     */
    /*   memory assocaited with the process.                            */
    /*                                                                  */
    /********************************************************************/

SNATIVE ALL_HEAPS::CommittedSpace( VOID )
	{
	AUTO MEMORY_BASIC_INFORMATION Details;
	AUTO SNATIVE MaxAddress = (-1 >> 1);
	REGISTER CHAR *Address = NULL;
	REGISTER SNATIVE FreeAreas = 0;
	REGISTER SNATIVE Size = 0;


	//
	//   Walk the available address space.
	//
	for 
			( 
			/* void */;
			Address < ((CHAR*) MaxAddress);
			Address += Details.RegionSize
			)
		{
		//
		//   Extract the details from the OS.
		//
		VirtualQuery( Address,& Details,sizeof(Details) );

		//
		//   Count any committed memory.
		//
		if ( Details.State == MEM_COMMIT )
			{ Size += Details.RegionSize; }

		//
		//   It seems like Windows is not to happy
		//   about having 64-bits of address space.
		//   I walks through the high end free areas
		//   64k at a time saying "Nope - nothing here".  
		//   This gets boring after a while so after 4GB 
		//   we get bored and give up.
		//
		if ( Details.State == MEM_FREE )
			{
			if ( (FreeAreas += Details.RegionSize) > 0xffffffff )
				{ break; }
			}
		else
			{ FreeAreas = 0; }
		}

	return Size ;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Execute a heap test.                                           */
    /*                                                                  */
    /*   Create a number of heaps and threads and run a test.           */
    /*                                                                  */
    /********************************************************************/

BOOLEAN ALL_HEAPS::ExecuteTestThreads
		(
		BENCHMARK_FLAGS				  BenchmarkFlags,
		LONG						  *Cost,
		SBIT32						  NumberOfHeaps,
		SBIT32						  NumberOfThreads
		)
	{
	AUTO THREAD_INFO Array[ MaxThreads ];

	//
	//   When we need to compute the setup costs
	//   we compute the existing costs here.
	//
	if ( (BenchmarkFlags & MeasureSetup) )
		{
		//
		//   Set the initial cost to the current time
		//   or space usage.
		//
		if ( (BenchmarkFlags & MeasureSpace) )
			{ (*Cost) = ((SBIT32) CommittedSpace()); }

		if ( (BenchmarkFlags & MeasureTime) )
			{ (*Cost) = ((SBIT32) GetTickCount()); }
		}

	//
	//   Create all the heaps.
	//
	if 
			( 
			CreateHeaps
				( 
				NumberOfHeaps,
				NumberOfThreads,
				((BenchmarkFlags & MeasureLocks) != 0),
				((BenchmarkFlags & MeasureZeros) != 0)
				) 
			)
		{
		REGISTER SBIT32 Count;

		//
		//   Zero various values.
		//
		Active = 0;

		//
		//   We execute the threads if we are not
		//   measuring setup costs.
		//
		if ( ! (BenchmarkFlags & MeasureSetup) )
			{
			AUTO HASH_TABLE Hash;

			//
			//   Start all the threads.
			//
			for ( Count=0;Count < NumberOfThreads;Count ++ )
				{
				REGISTER THREAD_INFO *ThreadInfo = & Array[ Count ];

				//
				//   Setup the thread information.
				//
				ThreadInfo -> AllHeaps = this;
				ThreadInfo -> BenchmarkFlags = BenchmarkFlags;
				ThreadInfo -> Cost = Cost;
				ThreadInfo -> Hash = & Hash;
				ThreadInfo -> NumberOfThreads = NumberOfThreads;
				ThreadInfo -> Stride = 0;
				ThreadInfo -> Thread = Count;

				if ( (BenchmarkFlags & MeasureCache) )
					{ ThreadInfo -> Stride = CacheLineSize; }

				if ( (BenchmarkFlags & MeasureCacheSpans) )
					{ ThreadInfo -> Stride = CacheLineSize; }

				if ( (BenchmarkFlags & MeasurePages) )
					{ ThreadInfo -> Stride = (ENVIRONMENT::PageSize()); }

				if ( (BenchmarkFlags & MeasurePageSpans) )
					{ ThreadInfo -> Stride = (ENVIRONMENT::PageSize()); }

				if ( (BenchmarkFlags & MeasureSharing) )
					{ ThreadInfo -> Stride = CacheLineSize; }

				//
				//   Start the thread.
				//
				Threads.StartThread
					( 
					((NEW_THREAD) ExecuteTestThread),
					((VOID*) ThreadInfo)
					); 
				}

			//
			//   Wait for all threads to complete.
			//
			(VOID) Threads.WaitForThreads();
			}

		//
		//   We compute the setup space here.
		//
		if 
				( 
				(BenchmarkFlags & MeasureSetup) 
					&& 
				(BenchmarkFlags & MeasureSpace) 
				)
			{ (*Cost) = (((SBIT32)CommittedSpace()) - (*Cost)); }

		//
		//   Delete all the heaps.
		//
		DeleteHeaps();

		//
		//   We compute the setup time here.
		//
		if 
				( 
				(BenchmarkFlags & MeasureSetup) 
					&& 
				(BenchmarkFlags & MeasureTime)
				)
			{ (*Cost) = (((SBIT32) GetTickCount()) - (*Cost)); }

		return True;
		}

	//
	//   Just to be sure we never give a misleading result.
	//
	(*Cost) = 0;

	return False;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Execute a test thread.                                         */
    /*                                                                  */
    /*   Execute a test thread on each heap as fast as possible.        */
    /*                                                                  */
    /********************************************************************/

VOID ALL_HEAPS::ExecuteTestThread( VOID *Parameter )
	{
	REGISTER THREAD_INFO *ThreadInfo = ((THREAD_INFO*) Parameter);

	TRY
		{
		REGISTER ALL_HEAPS *TestHeap = ThreadInfo -> AllHeaps;
		REGISTER BENCHMARK_FLAGS BenchmarkFlags = ThreadInfo -> BenchmarkFlags;
		REGISTER HASH_TABLE *Hash = ThreadInfo -> Hash;
		REGISTER LONG *Cost = ThreadInfo -> Cost;
		REGISTER SBIT32 NumberOfThreads = ThreadInfo -> NumberOfThreads;
		REGISTER SBIT32 Stride = ThreadInfo -> Stride;
		REGISTER SBIT32 Thread = ThreadInfo -> Thread;

		//
		//   Ensure the thread data looks sane.
		//
		if 
				( 
				(NumberOfThreads > 0)
					&&
				(COMMON::PowerOfTwo( BaseSize ))
					&&
				((Thread >= 0) && (Thread < NumberOfThreads))
				)
			{
			AUTO ALLOCATIONS Allocations[ MaxAddress ];
			REGISTER SBIT32 Count;

			//
			//   Setup the randon number generator seed.
			//
			srand(1);

			//
			//   Wait for all the threads to arrive.
			//
			Barrier1.Synchronize( NumberOfThreads );

			//
			//   Start the cost counter when the last thread 
			//   arrives.
			//
			if ( InterlockedIncrement( & Active ) == NumberOfThreads )
				{ 
				//
				//   Set the initial cost to the current time
				//   or space usage.
				//
				if ( (BenchmarkFlags & MeasureCache) )
					{ (*Cost) = 0; }

				if ( (BenchmarkFlags & MeasureCacheSpans) )
					{ (*Cost) = 0; }

				if ( (BenchmarkFlags & MeasurePages) )
					{ (*Cost) = 0; }

				if ( (BenchmarkFlags & MeasurePageSpans) )
					{ (*Cost) = 0; }

				if ( (BenchmarkFlags & MeasureSharing) )
					{ (*Cost) = 0; }

				if ( (BenchmarkFlags & MeasureSpace) )
					{ (*Cost) = ((SBIT32) CommittedSpace()); }

				if ( (BenchmarkFlags & MeasureTime) )
					{ (*Cost) = ((SBIT32) GetTickCount()); }
				}

			//
			//   Wait for all the threads to arrive.
			//
			Barrier2.Synchronize( NumberOfThreads );

			//
			//   Create a number of allocations.
			//
			for ( Count=0;Count < MaxAddress;Count ++ )
				{ 
				REGISTER ALLOCATIONS *Allocation = & Allocations[ Count ];
				REGISTER SBIT32 Shift = (rand() % MaxBaseShift);
				REGISTER SBIT32 Mask = ((BaseSize << Shift) - 1);
				REGISTER SBIT32 Size = ((rand() & Mask) + 1);

				TestHeap -> New
					( 
					& Allocation -> Address,
					(Allocation -> Size = Size),
					Thread 
					);
				}

			//
			//   Generate a random allocation pattern.
			//
			for ( Count=0;Count < (MaxCommands - (MaxAddress*2));Count ++ )
				{
				REGISTER SBIT32 Offset = (rand() % MaxAddress);
				REGISTER ALLOCATIONS *Allocation = & Allocations[ Offset ];
				REGISTER SBIT32 Shift = (rand() % MaxBaseShift);
				REGISTER SBIT32 Mask = ((BaseSize << Shift) - 1);
				REGISTER SBIT32 Size = ((rand() & Mask) + 1);

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
					TestHeap -> Delete
						( 
						Allocation -> Address,
						Thread 
						);

					TestHeap -> New
						( 
						& Allocation -> Address,
						(Allocation -> Size = Size),
						Thread 
						);

					Count ++;
					}
				else
					{ 
					TestHeap -> Resize
						( 
						& Allocation -> Address,
						(Allocation -> Size = Size),
						Thread 
						); 
					}
				}

			//
			//   If we are computing cache usage, page
			//   usage, false sharing or space usage then
			//   evaluate these just before we do the
			//   final delete.
			//
			if 
					( 
					(BenchmarkFlags & MeasureCache)
						||
					(BenchmarkFlags & MeasureCacheSpans)
						||
					(BenchmarkFlags & MeasurePages) 
						||
					(BenchmarkFlags & MeasurePageSpans) 
						||
					(BenchmarkFlags & MeasureSharing) 
						||
					(BenchmarkFlags & MeasureSpace) 
					)
				{
				//
				//   Wait for all the threads to arrive.
				//
				Barrier3.Synchronize( NumberOfThreads );

				//
				//   Compute cache usage.
				//
				if
						( 
						(BenchmarkFlags & MeasureCache)
							||
						(BenchmarkFlags & MeasurePages)
							||
						(BenchmarkFlags & MeasureSharing)
						)
					{
					REGISTER SBIT32 Total = 0;

					//
					//   Each thread adds its cache lines
					//   to a shared hash table.
					//
					for ( Count=0;Count < MaxAddress;Count ++ )
						{
						REGISTER ALLOCATIONS *Allocation = & Allocations[ Count ];
						REGISTER SNATIVE Address = ((SNATIVE) Allocation -> Address);
						REGISTER SNATIVE End = (Address + Allocation -> Size);

						//
						//   Compute the cache line address and
						//   make sure it is not null.
						//
						if ( (Address &= ~(Stride-1)) != NULL )
							{
							//
							//   Plot each cache line in the
							//   hash table.
							//
							for ( /* void */;Address < End;Address += Stride )
								{
								AUTO SNATIVE Value;

								//
								//   Find any existing use of the
								//   cache line.
								//
								if ( ! Hash -> FindInHash( Address,& Value ) )
									{
									//
									//   Add the cache line to the
									//   hash table and update the
									//   count.
									//
									Hash -> AddToHash( Address,Thread );

									//
									//   Add to total associated total
									//   if the the predicates are met.
									//
									if ( (BenchmarkFlags & MeasureCache) )
										{ Total ++; }

									if ( (BenchmarkFlags & MeasurePages) )
										{ Total ++; }
									}
								else
									{
									//
									//   Add to total associated total
									//   if the the predicates are met.
									//
									if ( (BenchmarkFlags & MeasureSharing) )
										{
										if ( Thread != Value )
											{ Total ++; }
										}
									}
								}
							}
						}

					//
					//   Update the cost.
					//
					InterlockedExchangeAdd( Cost,Total );

					//
					//   Decrement the count of active 
					//   threads.
					//
					InterlockedDecrement( & Active );
					}

				//
				//   If we are computing cache spans or page
				//   spans then evaluate these just before 
				//   we do the final delete.
				//
				if 
						( 
						(BenchmarkFlags & MeasureCacheSpans)
							||
						(BenchmarkFlags & MeasurePageSpans) 
						)
					{
					REGISTER SBIT32 Total = 0;

					//
					//   Each thread adds its cache lines
					//   to a shared hash table.
					//
					for ( Count=0;Count < MaxAddress;Count ++ )
						{
						REGISTER ALLOCATIONS *Allocation = & Allocations[ Count ];
						REGISTER SNATIVE Address = ((SNATIVE) Allocation -> Address);
						REGISTER SNATIVE End = (Address + Allocation -> Size);
						REGISTER SNATIVE Expected = (((End - Address - 1) / Stride) + 1);

						//
						//   Compute the cache line address and
						//   make sure it is not null.
						//
						if ( (Address &= ~(Stride-1)) != NULL )
							{
							REGISTER SBIT32 Actual;

							//
							//   Plot each cache line in the
							//   hash table.
							//
							for 
									( 
									Actual=0;
									Address < End;
									Actual ++, Address += Stride 
									);

							Total += ((SBIT32) (Actual - Expected));
							}
						}

					//
					//   Update the cost.
					//
					InterlockedExchangeAdd( Cost,Total );

					//
					//   Decrement the count of active 
					//   threads.
					//
					InterlockedDecrement( & Active );
					}

				//
				//   Compute the space usage.
				//
				if ( (BenchmarkFlags & MeasureSpace) )
					{
					//
					//   Compute space when the last thread 
					//   arrives.
					//
					if ( InterlockedDecrement( & Active ) == 0 )
						{ (*Cost) = (((SBIT32) CommittedSpace()) - (*Cost)); }
					}

				//
				//   Wait for all the threads to arrive.
				//
				Barrier4.Synchronize( NumberOfThreads );
				}

			//
			//   Delete all outstanding of allocations.
			//
			for ( Count=(MaxAddress-1);Count >= 0;Count -- )
				{ 
				TestHeap -> Delete
					( 
					Allocations[ Count ].Address,
					Thread 
					); 
				}

			//
			//   If we are collecting timings.
			//
			if ( (BenchmarkFlags & MeasureTime) )
				{
				//
				//   Wait for all the threads to arrive.
				//
				Barrier3.Synchronize( NumberOfThreads );

				//
				//   Stop the timer when the first thread 
				//   exits.
				//
				if ( InterlockedDecrement( & Active ) == (NumberOfThreads-1) )
					{ (*Cost) = (((SBIT32) GetTickCount()) - (*Cost)); }

				//
				//   Wait for all the threads to arrive.
				//
				Barrier4.Synchronize( NumberOfThreads );
				}
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
