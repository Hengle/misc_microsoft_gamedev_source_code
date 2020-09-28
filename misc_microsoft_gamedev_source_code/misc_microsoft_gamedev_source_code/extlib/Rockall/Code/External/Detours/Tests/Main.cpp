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

#include <stdlib.h>
#include <malloc.h>
#include <stdio.h>
#include <Windows.h>

    /********************************************************************/
    /*                                                                  */
    /*   Main program.                                                  */
    /*                                                                  */
    /*   The main program is the initial entry point for the system.    */
    /*                                                                  */
    /********************************************************************/

int _cdecl main()
    {
	auto void *Address;
#ifndef MANUAL_LOAD

	//
	//   Manually load the detour library.
	//
	LoadLibrary( "..\\DetourToDebugHeap\\Release\\DetourToDebugHeap.dll" );
#endif
#ifdef WAIT_FOR_DEBUGGER
	static volatile int Wait;

	//
	//   Wait for debugger to attach.
	//
	for ( Wait=15;Wait > 0;Wait -- ) 
		{ Sleep( 1000 ); }
#endif

	//
	//   Output some startup titles.
	//
	printf( "Start of tests ...\n" );

	//
	//   Call all the heap APIs and ensure
	//   there is a leak.
	//
	Address = calloc( 1,8 );

	Address = malloc( 12 );

	Address = realloc( Address,16 );

	free( Address );

	//
	//   Output some shutdown titles.
	//
	printf( "End of tests ...\n" );

	return 0;
    }
