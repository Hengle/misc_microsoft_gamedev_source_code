                         
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
#include "SmpHeap.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Static member initialization.                                  */
    /*                                                                  */
    /*   Static member initialization sets the initial value for all    */
    /*   static members.                                                */
    /*                                                                  */
    /********************************************************************/

STATIC SMP_HEAP Heap;

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
    /*   Detoured 'calloc'.                                             */
    /*                                                                  */
    /*   The detoured 'calloc' uses Rockall instead of the Visual       */
    /*   Studio heap.                                                   */
    /*                                                                  */
    /********************************************************************/

void * __cdecl Mine_Calloc( size_t Number,size_t Size )
	{ return Heap.New( (Number * Size),NULL,true ); }

    /********************************************************************/
    /*                                                                  */
    /*   Detoured 'malloc'.                                             */
    /*                                                                  */
    /*   The detoured 'malloc' uses Rockall instead of the Visual       */
    /*   Studio heap.                                                   */
    /*                                                                  */
    /********************************************************************/

void * __cdecl Mine_Malloc( size_t Size )
	{ return Heap.New( Size ); }

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
	if ( Heap.KnownArea( Address ) )
		{
		AUTO int Space;

		if ( Heap.Details( Address,& Space ) )
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
	REGISTER VOID *NewAddress = Heap.Resize( Address,Size );

	if ( NewAddress == NULL )
		{ NewAddress = Real_Realloc( Address,Size ); }

	return NewAddress;
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
	if ( ! Heap.Delete( Address ) )
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
			}
		}

	return TRUE;
	}
