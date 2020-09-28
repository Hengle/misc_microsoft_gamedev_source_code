
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

#include "LibraryPCH.hpp"

#ifdef ENABLE_GLOBAL_ROCKALL
#include "DefaultHeap.hpp"
#endif
#include "New.hpp"
#ifndef DISABLE_GLOBAL_NEW

    /********************************************************************/
    /*                                                                  */
    /*   The memory allocation operator.                                */
    /*                                                                  */
    /*   The memory allocation operator 'new' is overloaded to          */
    /*   provide a consistent interface.                                */
    /*                                                                  */
    /********************************************************************/

VOID* CDECL operator new( size_t Size )
    {
#ifdef ENABLE_GLOBAL_ROCKALL
    REGISTER VOID *Store = DefaultHeap.New( Size );
#else
    REGISTER VOID *Store = malloc( Size );
#endif

    if ( Store == NULL )
        { Failure( "Out of system memory" ); }

    return Store;
    }

    /********************************************************************/
    /*                                                                  */
    /*   The memory allocation operator.                                */
    /*                                                                  */
    /*   The memory allocation operator 'new' is overloaded to          */
    /*   provide a consistent interface.                                */
    /*                                                                  */
    /********************************************************************/

VOID* CDECL operator new[]( size_t Size )
    {
#ifdef ENABLE_GLOBAL_ROCKALL
    REGISTER VOID *Store = DefaultHeap.New( Size );
#else
    REGISTER VOID *Store = malloc( Size );
#endif

    if ( Store == NULL )
        { Failure( "Out of system memory" ); }

    return Store;
    }

    /********************************************************************/
    /*                                                                  */
    /*   The memory deallocation operator.                              */
    /*                                                                  */
    /*   The memory deallocation operator releases allocated memory.    */
    /*                                                                  */
    /********************************************************************/

VOID CDECL operator delete( VOID *Store )
	{
#ifdef ENABLE_GLOBAL_ROCKALL
    DefaultHeap.Delete( Store );
#else
    free( Store );
#endif
	}

    /********************************************************************/
    /*                                                                  */
    /*   The memory deallocation operator.                              */
    /*                                                                  */
    /*   The memory deallocation operator releases allocated memory.    */
    /*                                                                  */
    /********************************************************************/

VOID CDECL operator delete[]( VOID *Store )
	{
#ifdef ENABLE_GLOBAL_ROCKALL
    DefaultHeap.Delete( Store );
#else
    free( Store );
#endif
	}
#endif
