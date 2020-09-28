                         
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

#include "Global.hpp"

#include "Detours.h"
#include "DebugHeap.hpp"
#include "FastHeap.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Constants local to the class.                                  */
    /*                                                                  */
    /*   The constants specify sizes for various data structures.       */
    /*   up access to it.                                               */
    /*                                                                  */
    /********************************************************************/

CONST SBIT32 DefualtSleepTime		  = 30;
CONST SBIT32 MaxEnvironmentString	  = 16;

    /********************************************************************/
    /*                                                                  */
    /*   Static member initialization.                                  */
    /*                                                                  */
    /*   Static member initialization sets the initial value for all    */
    /*   static members.                                                */
    /*                                                                  */
    /********************************************************************/

STATIC DEBUG_HEAP PrimaryHeap( 0,false,true,true,true,true );
STATIC DEBUG_HEAP SecondaryHeap( 0,false,true,true,true,true );
STATIC ROCKALL_FRONT_END *Heap = & PrimaryHeap;

    /********************************************************************/
    /*                                                                  */
    /*   An unused DLL export.                                          */
    /*                                                                  */
    /*   The DLL export symbol is specified here just to keep VC        */
    /*   happy and is not used in the code.                             */
    /*                                                                  */
    /********************************************************************/

DLL_EXPORT VOID *Unused = 0;

    /********************************************************************/
    /*                                                                  */
    /*   Detour functions.                                              */
    /*                                                                  */
    /*   The detour functions are overloaded by Rockall replacements    */
    /*   so that the standard windows heaps are avoided.                */
    /*                                                                  */
    /********************************************************************/

extern "C" 
	{
	//
	//   Heap calls.
	//
	//   The heap calls are detoured via the following
	//   detour specifications.
	//

    DETOUR_TRAMPOLINE
		(
		void * __cdecl Real_Calloc
			( 
			size_t Size 
			),
        calloc
		);

    DETOUR_TRAMPOLINE
		(
		void * __cdecl Real_Malloc
			( 
			size_t Size 
			),
        malloc
		);

    DETOUR_TRAMPOLINE
		(
		size_t __cdecl Real_MSize
			( 
			void *Address
			),
        _msize
		);

    DETOUR_TRAMPOLINE
		(
		void * __cdecl Real_Realloc
			(
			void *Address,
			size_t Size 
			),
        realloc
		);

    DETOUR_TRAMPOLINE
		(
		void __cdecl Real_Free
			( 
			void *Address
			),
        free
		);
	}

    /********************************************************************/
    /*                                                                  */
    /*   Debug thread function.                                         */
    /*                                                                  */
    /*   The debug thread function executes quitely in the background   */
    /*   to organize the debugging session.                             */
    /*                                                                  */
    /********************************************************************/

void ThreadFunction( void )
	{
	AUTO CHAR EnvironmentString[ MaxEnvironmentString ];
	AUTO SBIT32 StartTime = DefualtSleepTime;
	AUTO SBIT32 RunTime = DefualtSleepTime;
	AUTO SBIT32 EndTime = (DefualtSleepTime * 2);
 
	//
	//   We need to obtain any available configuration
	//   information from the external environment.
	//
	if 
			( 
			GetEnvironmentVariable
				( 
				"DETOUR_FOR_DYNAMIC_LEAKS",
				EnvironmentString,
				MaxEnvironmentString
				)
			)
		{
		//
		//   Extract any available configuration 
		//   data and update the variables.
		//
		sscanf
			(
			EnvironmentString,
			"%d,%d,%d",
			& StartTime,
			& RunTime,
			& EndTime
			);
		}

	//
	//   Now we are ready to start run.  However, we 
	//   output the configuration values to the debug
	//   stream to keep the user informed about what
	//   we are going to do.
	//
	DebugPrint
		(
		"Detour For Dynamic Leaks\n"
		"\n"
		"The runtime configuration values are as follows:\n"
		"Start Time	= %d secs\n"
		"Run Time	= %d secs\n"
		"End Time	= %d secs\n",
		StartTime,
		RunTime,
		EndTime
		);

	//
	//   Sleep for the specified time to allow the
	//   application to warm up.
	//
	Sleep( (StartTime * 1000) );

	//
	//   Now switch to the secondary heap.
	//
	Heap = & SecondaryHeap;

	//
	//   Sleep for the specified time while the
	//   secondary heap get used.
	//
	Sleep( (RunTime * 1000) );

	//
	//   Now switch back to the primary heap for the
	//   rest of the run.
	//
	Heap = & PrimaryHeap;

	//
	//   Sleep for the specified time to give the
	//   application a chance to delete the secondary
	//   heap allocations.
	//
	Sleep( (EndTime * 1000) );

	//
	//   Now logically anything that is left over 
	//   has a good chance of being a leak.
	//
	DebugPrint
		(
		"Final Results\n"
		"\n"
		"We have run the application %d secs to "
		"warm up.  We then switched the heap to\n"
		"a 'DEBUG_HEAP' and let the application "
		"run for a further %d secs.  Finally,\n"
		"we switched the heap back to the original "
		"heap and waited %d secs for any\n"
		"allocations on the 'DEBUG_HEAP' to be "
		"deleted.  The remaining memory allocations\n"
		"listed below are prime candidates for"
		"being memory leaks.\n",
		StartTime,
		RunTime,
		EndTime
		);

	SecondaryHeap.HeapLeaks();
	}

    /********************************************************************/
    /*                                                                  */
    /*   Detoured 'calloc'.                                             */
    /*                                                                  */
    /*   The detoured 'calloc' uses Rockall instead of the Visual       */
    /*   Studio heap.                                                   */
    /*                                                                  */
    /********************************************************************/

void * __cdecl Mine_Calloc( size_t Number,size_t Size )
	{ return Heap -> New( (Number * Size),NULL,true ); }

    /********************************************************************/
    /*                                                                  */
    /*   Detoured 'malloc'.                                             */
    /*                                                                  */
    /*   The detoured 'malloc' uses Rockall instead of the Visual       */
    /*   Studio heap.                                                   */
    /*                                                                  */
    /********************************************************************/

void * __cdecl Mine_Malloc( size_t Size )
	{ return Heap -> New( Size ); }

    /********************************************************************/
    /*                                                                  */
    /*   Detoured '_msize'.                                             */
    /*                                                                  */
    /*   The detoured '_msize' uses Rockall instead of the Visual       */
    /*   studio heap.  When the original memory was not from Rockall    */
    /*   the request is passed to the Visual Studio heap.               */
    /*                                                                  */
    /********************************************************************/

size_t _cdecl Mine_MSize( void *Address )
	{
	if ( Heap -> KnownArea( Address ) )
		{
		AUTO int Space;

		if ( Heap -> Details( Address,& Space ) )
			{ return Space; }
		else
			{ return 0; }
		}
	else
		{ return Real_MSize( Address ); }
	}

    /********************************************************************/
    /*                                                                  */
    /*   Detoured 'realloc'.                                            */
    /*                                                                  */
    /*   The detoured 'realloc' uses Rockall instead of the Visual      */
    /*   studio heap.  When the original memory was not from Rockall    */
    /*   the request is passed to the Visual Studio heap.               */
    /*                                                                  */
    /********************************************************************/

void * __cdecl Mine_Realloc( void *Address,size_t Size )
	{
	if ( Heap -> KnownArea( Address ) )
		{ return Heap -> Resize( Address,Size ); }
	else
		{ return Real_Realloc( Address,Size ); }
	}

    /********************************************************************/
    /*                                                                  */
    /*   Detoured 'free'.                                               */
    /*                                                                  */
    /*   The detoured 'free' uses Rockall instead of the Visual         */
    /*   studio heap.  When the original memory was not from Rockall    */
    /*   the request is passed to the Visual Studio heap.               */
    /*                                                                  */
    /********************************************************************/

void __cdecl Mine_Free( void *Address )
	{
	if ( Heap -> KnownArea( Address ) )
		{ Heap -> Delete( Address ); }
	else
		{ Real_Free( Address ); }
	}

    /********************************************************************/
    /*                                                                  */
    /*   The DLL main entry point.                                      */
    /*                                                                  */
    /*   The DLL main entry point is called whenever a process          */
    /*   or thread attaches or detaches.                                */
    /*                                                                  */
    /********************************************************************/

BOOL APIENTRY DllMain
		(
		HINSTANCE					Module, 
		DWORD						Reason, 
		PVOID						Reserved
		)
	{
	//
	//   We need to understand why we were called.  So here
	//   we examine 'Reason' to understand the purpose of the
	//   call.
	//
	switch( Reason ) 
		{
		case DLL_PROCESS_ATTACH:
			{
			//
			//   When a process attaches we need to detour
			//   all the functions of interest.
			//
			DetourFunctionWithTrampoline
				(
				((PBYTE)Real_Calloc),
				((PBYTE)Mine_Calloc)
				);

			DetourFunctionWithTrampoline
				(
				((PBYTE)Real_Malloc),
				((PBYTE)Mine_Malloc)
				);

			DetourFunctionWithTrampoline
				(
				((PBYTE)Real_MSize),
				((PBYTE)Mine_MSize)
				);

			DetourFunctionWithTrampoline
				(
				((PBYTE)Real_Realloc),
				((PBYTE)Mine_Realloc)
				);

			DetourFunctionWithTrampoline
				(
				((PBYTE)Real_Free),
				((PBYTE)Mine_Free)
				);
			break;
			}

		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
			{ break; }

		case DLL_PROCESS_DETACH:
			{ 
			//
			//   When a process detaches we need to remove 
			//   all the detours.
			//
			DetourRemove
				(
				((PBYTE)Real_Free),
				((PBYTE)Mine_Free)
				);

			DetourRemove
				(
				((PBYTE)Real_Realloc),
				((PBYTE)Mine_Realloc)
				);

			DetourRemove
				(
				((PBYTE)Real_MSize),
				((PBYTE)Mine_MSize)
				);

			DetourRemove
				(
				((PBYTE)Real_Malloc),
				((PBYTE)Mine_Malloc)
				);

			DetourRemove
				(
				((PBYTE)Real_Calloc),
				((PBYTE)Mine_Calloc)
				);

			//
			//   Output all the heap leaks.
			//
			SecondaryHeap.HeapLeaks();
			}
		}

	return TRUE;
	}
