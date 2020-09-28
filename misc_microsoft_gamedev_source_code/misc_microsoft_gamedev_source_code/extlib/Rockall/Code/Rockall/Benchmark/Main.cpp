 
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
#include "BlendedHeapTest.hpp"
#include "Environment.hpp"
#include "DebugHeapTest.hpp"
#include "FastHeapTest.hpp"
#include "GlobalHeapTest.hpp"
#if defined(WIN32)
#include "LfhHeapTest.hpp"
#endif
#include "NasdaqHeapTest.hpp"
#include "NoHeapTest.hpp"
#include "NtHeapTest.hpp"
#include "SmallHeapTest.hpp"
#include "SmpHeapTest.hpp"
#include "TunedHeapTest.hpp"
#include "VcHeapTest.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Static class initialization.                                   */
    /*                                                                  */
    /*   Static class initialization creates an initial instance of     */
    /*   the class.                                                     */
    /*                                                                  */
    /********************************************************************/

STATIC ENVIRONMENT Environment;

STATIC BLENDED_HEAP_TEST BlendedHeapTest;
STATIC DEBUG_HEAP_TEST DebugHeapTest;
STATIC FAST_HEAP_TEST FastHeapTest;
STATIC GLOBAL_HEAP_TEST GlobalHeapTest;
#if defined(WIN32)
STATIC LFH_HEAP_TEST LfhHeapTest;
#endif
STATIC NO_HEAP_TEST NoHeapTest;
STATIC NT_HEAP_TEST NtHeapTest;
STATIC SMALL_HEAP_TEST SmallHeapTest;
STATIC SMP_HEAP_TEST SmpHeapTest;
STATIC VC_HEAP_TEST VcHeapTest;

#ifdef ENABLE_OTHER_HEAPS                       
STATIC NASDAQ_HEAP_TEST NasdaqHeapTest;
STATIC TUNED_HEAP_TEST TunedHeapTest;
STATIC MP_HEAP_TEST MpHeapTest;
#endif

STATIC ALL_HEAPS *AllHeaps[] =
	{ 
	& NoHeapTest,
//	& LkMallocHeapTest,
//	& MpHeapTest,
//	& DebugHeapTest,				//  Tuned for testing.
//	& SmallHeapTest,				//  Tuned for space not speed.
//	& BlendedHeapTest,				//  Tuned for space not speed.
	& FastHeapTest,
	& GlobalHeapTest,
//	& NasdaqHeapTest,
	& SmpHeapTest,
//	& TunedHeapTest,
//	& SmartHeapTest,				//  Tuned for space not speed.
//	& MultiSmartHeapTest,
	& VcHeapTest,
#if defined(WIN32)
	& LfhHeapTest,
#endif
	& NtHeapTest
	};

    /********************************************************************/
    /*                                                                  */
    /*   Main program.                                                  */
    /*                                                                  */
    /*   The main program is the initial entry point for the system.    */
    /*                                                                  */
    /********************************************************************/

int _cdecl main( INT ArgC, CHAR *ArgV[] )
    {
	TRY
		{
		//
		//   When there is no parameters we run the
		//   banchmark and out the results.
		//
		if ( ArgC == 1 )
			{
			REGISTER int Count1;
			REGISTER SBIT32 NumberOfTestHeaps = 
				(sizeof(AllHeaps) / sizeof(ALL_HEAPS*));

			//
			//   Output the benchmark headers.
			//
			fprintf
				( 
				stderr,
				"\n\n\n"
				"Multi-Heap Benchmark\n"
				"********************\n\n"
				"Global Benchmark Settings:\n\n"
				"\tA total of %d heap calls per thread (i.e. the total is \n"
				"\tproportional to the number of threads).  The heap calls are 'malloc',\n" 
				"\t'realloc' and 'free' or their equivalents for the heap being tested.\n"
				"\tA steady state average of %d outstanding allocations per thread.\n"
				"\tA weighted distribution of allocation sizes between 1 and %d bytes\n"
				"\t(The smallest sizes being %d times more common than the largest sizes).\n"
				"\tAn average of %d new allocations per resize.\n"
				"\tThe system contains %d CPUs and %d MB of RAM.\n"
				"\tAll times are elapsed seconds.\n\n"
				"\tNote: Do not run this benchmark under the VC shell as the performance\n"
				"\t      of the NT and VC based heaps is significantly degraded.\n"
				"\nDescription of the heaps tested:\n\n",
				MaxCommands,
				MaxAddress,
				(BaseSize << (MaxBaseShift-1)),
				MaxBaseShift,
				ResizeRate,
				Environment.NumberOfCpus(),
				(Environment.MemorySize() / 1024 / 1024)
				);

			//
			//   Output a description of each heap.
			//
			for ( Count1=0;Count1 < NumberOfTestHeaps;Count1 ++ )
				{
				fprintf
					( 
					stderr,
					"\tHeap %d is %s.\n",
					Count1,
					AllHeaps[ Count1 ] -> NameOfHeap()
					);
				}

			//
			//   Execute each benchmark.
			//
			for ( Count1=MinTest;Count1 <= MaxTest;Count1 ++ )
				{
				REGISTER SBIT32 Count2;
				REGISTER SBIT32 Flags = 0;

				//
				//   Output the run headers.
				//
				fprintf
					( 
					stderr,
					"\n\n\n"
					"Benchmark %d\n"
					"************\n\n"
					"Test Run Settings:\n\n",
					Count1
					);

				//
				//   Compute the setting to measure the
				//   required value.
				//
				switch ( Count1 )
					{
					case 1:
					case 2:
					case 3:
					case 4:
					case 5:
						{
						//
						//   Measure time.
						//
						Flags &= ~((SBIT32) MeasureCache);
						Flags &= ~((SBIT32) MeasureCacheSpans);
						Flags &= ~((SBIT32) MeasurePages);
						Flags &= ~((SBIT32) MeasurePageSpans);
						Flags &= ~((SBIT32) MeasureSharing);
						Flags &= ~((SBIT32) MeasureSpace);
						Flags |= ((SBIT32) MeasureTime);

						//
						//   Compute the setting to measure time
						//   when the memory is zeroed.
						//
						switch ( Count1 )
							{
							case 2:
							case 4:
								{ 
								Flags |= ((SBIT32) MeasureZeros);
								break;
								}

							default:
								{ 
								Flags &= ~((SBIT32) MeasureZeros);
								break;
								}
							}
						break;
						}
					
					case 6:
					case 7:
						{
						//
						//   Measure space.
						//
						Flags &= ~((SBIT32) MeasureCache);
						Flags &= ~((SBIT32) MeasureCacheSpans);
						Flags &= ~((SBIT32) MeasurePages);
						Flags &= ~((SBIT32) MeasurePageSpans);
						Flags &= ~((SBIT32) MeasureSharing);
						Flags |= ((SBIT32) MeasureSpace);
						Flags &= ~((SBIT32) MeasureTime);
						break;
						}

					case 8:
						{ 
						//
						//   Measure cache lines.
						//
						Flags |= ((SBIT32) MeasureCache);
						Flags &= ~((SBIT32) MeasureCacheSpans);
						Flags &= ~((SBIT32) MeasurePages);
						Flags &= ~((SBIT32) MeasurePageSpans);
						Flags &= ~((SBIT32) MeasureSharing);
						Flags &= ~((SBIT32) MeasureSpace);
						Flags &= ~((SBIT32) MeasureTime);
						break;
						}

					case 9:
						{
						//
						//   Measure cache spans.
						//
						Flags &= ~((SBIT32) MeasureCache);
						Flags |= ((SBIT32) MeasureCacheSpans);
						Flags &= ~((SBIT32) MeasurePages);
						Flags &= ~((SBIT32) MeasurePageSpans);
						Flags &= ~((SBIT32) MeasureSharing);
						Flags &= ~((SBIT32) MeasureSpace);
						Flags &= ~((SBIT32) MeasureTime);
						break;
						}

					case 10:
						{
						//
						//   Measure pages.
						//
						Flags &= ~((SBIT32) MeasureCache);
						Flags &= ~((SBIT32) MeasureCacheSpans);
						Flags |= ((SBIT32) MeasurePages);
						Flags &= ~((SBIT32) MeasurePageSpans);
						Flags &= ~((SBIT32) MeasureSharing);
						Flags &= ~((SBIT32) MeasureSpace);
						Flags &= ~((SBIT32) MeasureTime);
						break;
						}

					case 11:
						{
						//
						//   Measure page spans.
						//
						Flags &= ~((SBIT32) MeasureCache);
						Flags &= ~((SBIT32) MeasureCacheSpans);
						Flags &= ~((SBIT32) MeasurePages);
						Flags |= ((SBIT32) MeasurePageSpans);
						Flags &= ~((SBIT32) MeasureSharing);
						Flags &= ~((SBIT32) MeasureSpace);
						Flags &= ~((SBIT32) MeasureTime);
						break;
						}

					case 12:
						{
						//
						//   Measure false sharing.
						//
						Flags &= ~((SBIT32) MeasureCache);
						Flags &= ~((SBIT32) MeasureCacheSpans);
						Flags &= ~((SBIT32) MeasurePages);
						Flags &= ~((SBIT32) MeasurePageSpans);
						Flags |= ((SBIT32) MeasureSharing);
						Flags &= ~((SBIT32) MeasureSpace);
						Flags &= ~((SBIT32) MeasureTime);
						break;
						}

					default:
						{ 
						printf( "\n\nUnknown benchmark\n" );

						exit(1);
						}
					}

				//
				//   Compute the locking mechanism.
				//
				switch ( Count1 )
					{
					default:
						{ 
						Flags |= ((SBIT32) MeasureLocks);
						break;
						}

					case 3:
					case 4:
						{ 
						Flags &= ~((SBIT32) MeasureLocks);
						break;
						}
					}

				//
				//   Compute setup or use tests.
				//
				switch ( Count1 )
					{
					default:
						{ 
						Flags &= ~((SBIT32) MeasureSetup);
						break;
						}

					case 5:
					case 6:
						{ 
						Flags |= ((SBIT32) MeasureSetup);
						break;
						}
					}

				//
				//   Output a description of the current flags which
				//   describe the benchmark that is about to run.
				//
				if ( (Flags & MeasureSetup) )
					{
					//
					//   All the following benchmarks measure the
					//   resources consumed during setup.
					//
					if ( (Flags & MeasureSpace) )
						{
						fprintf( stderr,"\tSpace to setup each type of heap.\n\n" );

						fprintf( stderr,"\tA single (image) shared heap (with locks).\n" );
						fprintf( stderr,"\tAll mesurements are committed space (in MB).\n" );
						}

					if ( (Flags & MeasureTime) )
						{
						fprintf( stderr,"\tTime to setup each type of heap.\n\n" );

						fprintf( stderr,"\tA single (image) shared heap (with locks).\n" );
						fprintf( stderr,"\tAll mesurements are execution time (in seconds).\n" );
						}
					}
				else
					{
					//
					//   All the following benchmarks measure the
					//   resources consumed after setup.
					//
					if ( (Flags & MeasureCache) )
						{
						fprintf( stderr,"\tCache lines for %d heap calls per thread\n",MaxAddress );
						fprintf( stderr,"\t (after %d heap calls per thread).\n\n",(MaxCommands - MaxAddress) );

						fprintf( stderr,"\tA single (image) shared heap (with locks).\n" );
						fprintf( stderr,"\tAll mesurements are allocated cache lines.\n" );
						fprintf( stderr,"\tAll setup costs have been excluded from the run.\n" );
						}

					if ( (Flags & MeasureCacheSpans) )
						{
						fprintf( stderr,"\tCache lines spanned for %d heap calls per thread\n",MaxAddress );
						fprintf( stderr,"\t (after %d heap calls per thread).\n\n",(MaxCommands - MaxAddress) );

						fprintf( stderr,"\tA single (image) shared heap (with locks).\n" );
						fprintf( stderr,"\tAll mesurements are avoidable spans across cache lines.\n" );
						fprintf( stderr,"\tAll setup costs have been excluded from the run.\n" );
						}

					if ( (Flags & MeasurePages) )
						{
						fprintf( stderr,"\tHeap data pages for %d heap calls per thread\n",MaxAddress );
						fprintf( stderr,"\t (after %d heap calls per thread).\n\n",(MaxCommands - MaxAddress) );

						fprintf( stderr,"\tA single (image) shared heap (with locks).\n" );
						fprintf( stderr,"\tAll mesurements are committed OS pages.\n" );
						fprintf( stderr,"\tAll setup costs have been excluded from the run.\n" );
						}

					if ( (Flags & MeasurePageSpans) )
						{
						fprintf( stderr,"\tHeap data pages spanned for %d heap calls per thread\n",MaxAddress );
						fprintf( stderr,"\t (after %d heap calls per thread).\n\n",(MaxCommands - MaxAddress) );

						fprintf( stderr,"\tA single (image) shared heap (with locks).\n" );
						fprintf( stderr,"\tAll mesurements are avoidable spans across OS pages.\n" );
						fprintf( stderr,"\tAll setup costs have been excluded from the run.\n" );
						}

					if ( (Flags & MeasureSharing) )
						{
						fprintf( stderr,"\tFalse shared cache lines for %d heap calls per thread\n",MaxAddress );
						fprintf( stderr,"\t (after %d heap calls per thread).\n\n",(MaxCommands - MaxAddress) );

						fprintf( stderr,"\tA single (image) shared heap (with locks).\n" );
						fprintf( stderr,"\tAll mesurements are false shared cache lines.\n" );
						fprintf( stderr,"\tAll setup costs have been excluded from the run.\n" );
						}

					if ( (Flags & MeasureSpace) )
						{
						fprintf( stderr,"\tSpace for %d heap calls per thread\n",MaxAddress );
						fprintf( stderr,"\t (after %d heap calls per thread).\n\n",(MaxCommands - MaxAddress) );

						fprintf( stderr,"\tA single (image) shared heap (with locks).\n" );
						fprintf( stderr,"\tAll mesurements are committed space (in MB).\n" );
						fprintf( stderr,"\tAll setup costs have been excluded from the run.\n" );
						}

					if ( (Flags & MeasureTime) )
						{
						fprintf( stderr,"\tTime to execute %d heap calls per thread.\n\n",MaxCommands );

						if ( (Flags & MeasureLocks) )
							{ fprintf( stderr,"\tA single (image) shared heap (with locks).\n" ); }
						else
							{ fprintf( stderr,"\tA private heap per thread (with no locks).\n" ); }

						if ( (Flags & MeasureZeros) )
							{ fprintf( stderr,"\tAll allocations are zeroed.\n" ); }
						else
							{ fprintf( stderr,"\tNo allocations are zeroed.\n" ); }

						fprintf( stderr,"\tAll mesurements are execution time (in seconds).\n" );
						fprintf( stderr,"\tAll setup costs have been excluded from the run.\n" );
						}
					}

				//
				//   Output the benchmark headers.
				//
				fprintf( stderr,"\n\n  No   " );
				
				for ( Count2=0;Count2 < NumberOfTestHeaps;Count2 ++ )
					{ fprintf( stderr,"  Time  " ); }

				fprintf( stderr,"\nThreads" );
				
				for ( Count2=0;Count2 < NumberOfTestHeaps;Count2 ++ )
					{ fprintf( stderr," Heap %-2d",Count2 ); }

				//
				//   Flush the current results.
				//
				fflush(NULL);

				//
				//   Execute the benchmark.
				//
				for ( Count2=MinThreads;Count2 <= MaxThreads;Count2 <<= 1 )
					{
					REGISTER SBIT32 Count3;
					REGISTER SBIT32 NumberOfHeaps = 1;

					//
					//   Output the current number of
					//   CPUs being used.
					//
					fprintf( stderr,"\n  %2d   ",Count2 );

					//
					//   Compute the number of heaps.
					//
					switch ( Count1 )
						{
						default:
							{ 
							NumberOfHeaps = 1;
							break;
							}

						case 3:
						case 4:
							{ 
							NumberOfHeaps = Count2;
							break;
							}
						}

					//
					//   Benchmark each heap.
					//
					for ( Count3=0;Count3 < NumberOfTestHeaps;Count3 ++ )
						{
						AUTO LONG Cost = 0;

						//
						//   Benchmark a single heap.
						//
						if 
								(
								AllHeaps[ Count3 ] -> ExecuteTestThreads
									(
									((BENCHMARK_FLAGS) Flags),
									& Cost,
									NumberOfHeaps,
									Count2
									)
								)
							{
							REGISTER CONST CHAR *Format = " %6.0f ";
							REGISTER DOUBLE Final = ((DOUBLE) Cost);

							//
							//   Scale the cost to the associated 
							//   units.
							//
							if ( (Flags & MeasureSpace) )
								{
								Format = " %5.1fM ";
								Final /= (1024 * 1024);
								}

							if ( (Flags & MeasureTime) )
								{ 
								Format = " %5.1fs ";
								Final /= 1000; 
								}

							//
							//   Output final cost.
							//
							fprintf( stderr,Format,Final ); 
							}
						else
							{ fprintf( stderr,"   N/A  " ); }

						//
						//   Flush the current results.
						//
						fflush(NULL);
						}

					//
					//   Mark off thread per cpu.
					//
					if 
							( 
							(Count2 < MaxThreads) 
								&& 
							(Count2 == Environment.NumberOfCpus())
							)
						{
						//
						//   Output separator.
						//
						fprintf( stderr,"\n-+-+-+-+" );

						for ( Count3=0;Count3 < NumberOfTestHeaps;Count3 ++ )
							{ fprintf( stderr,"-+-+-+-+" ); }     
						}

					//
					//   Flush the current results.
					//
					fflush(NULL);
					}

				//
				//   The benchmark is complete.
				//
				fprintf( stderr,"\n" );
				}

			return 0;
			}
		else
			{ fprintf( stderr,"Usage: no options available\n" ); }
		}
#ifdef DISABLE_STRUCTURED_EXCEPTIONS
	catch ( FAULT &Message )
		{ 
		//
		//   Print the exception message and exit.
		//
		DebugPrint( "Exception caught: %s\n",((char*) Message) ); 
		}
	catch ( ... )
		{ 
		//
		//   Print the exception message and exit.
		//
		DebugPrint( "Exception caught: (unknown)\n" ); 
		}
#else
	__except ( EXCEPTION_EXECUTE_HANDLER ) 
		{ 
		//
		//   Print the exception message and exit.
		//
		DebugPrint( "Structured exception caught: (unknown)\n" ); 
		}
#endif

	return 1;
    }
