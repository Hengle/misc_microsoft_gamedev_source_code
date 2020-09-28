#ifndef _NASDAQ_HEAP_HPP_
#define _NASDAQ_HEAP_HPP_
//                                        Ruler
//       1         2         3         4         5         6         7         8
//345678901234567890123456789012345678901234567890123456789012345678901234567890

    /********************************************************************/
    /*                                                                  */
    /*   The standard layout.                                           */
    /*                                                                  */
    /*   The standard layout for 'hpp' files for this code is as        */
    /*   follows:                                                       */
    /*                                                                  */
    /*      1. Include files.                                           */
    /*      2. Constants exported from the class.                       */
    /*      3. Data structures exported from the class.                 */
	/*      4. Forward references to other data structures.             */
	/*      5. Class specifications (including inline functions).       */
    /*      6. Additional large inline functions.                       */
    /*                                                                  */
    /*   Any portion that is not required is simply omitted.            */
    /*                                                                  */
    /********************************************************************/

#include "DefaultHeap.hpp"
#include "FastHeap.hpp"

#ifdef NO_HEAP_PER_THREAD
#ifdef NO_DEFAULT_HEAP
    /********************************************************************/
    /*                                                                  */
    /*   The Nasdaq Rockall interface.                                  */
    /*                                                                  */
    /*   The Nasdaq Rockall interface allows a free-standing C++        */
    /*   application to interface with Rockall by simply overloading    */
    /*   the global new and delete operators.  The simplest method      */
    /*   redirects all the heap calls to the Rockall default heap.      */
    /*                                                                  */
    /********************************************************************/

__forceinline void NasdaqCreateHeap( void )
	{ /* void */ }

__forceinline bool NasdaqDelete( void *Store )
	{ return (free( Store ), true); }

__forceinline void NasdaqDeleteHeap( void )
	{ /* void */ }

__forceinline void *NasdaqNew( size_t Size )
	{ return malloc( ((int) Size) ); }

__forceinline void *NasdaqRealloc( void *Store,size_t Size )
	{ return realloc( Store,((int) Size) ); }

#else
    /********************************************************************/
    /*                                                                  */
    /*   The Nasdaq Rockall interface.                                  */
    /*                                                                  */
    /*   The Nasdaq Rockall interface allows a free-standing C++        */
    /*   application to interface with Rockall by simply overloading    */
    /*   the global new and delete operators.  The simplest method      */
    /*   redirects all the heap calls to the Rockall default heap.      */
    /*                                                                  */
    /********************************************************************/

__forceinline void NasdaqCreateHeap( void )
	{ /* void */ }

__forceinline bool NasdaqDelete( void *Store )
	{ return DefaultHeap.Delete( Store ); }

__forceinline void NasdaqDeleteHeap( void )
	{ /* void */ }

__forceinline void *NasdaqNew( size_t Size )
	{ return DefaultHeap.New( ((int) Size) ); }

__forceinline void *NasdaqRealloc( void *Store,size_t Size )
	{ return DefaultHeap.Resize( Store,((int) Size) ); }

#endif
#else
 
    /********************************************************************/
    /*                                                                  */
    /*   The Nasdaq Rockall interface.                                  */
    /*                                                                  */
    /*   The Nasdaq Rockall interface allows a free-standing C++        */
    /*   application to interface with Rockall by simply overloading    */
    /*   the global new and delete operators.  A more complex method    */
    /*   supported by the following code redirects all heaps calls      */
    /*   to a private heap per thread using Rockall's 'SingleImage'     */
    /*   feature.  Although this version is somewhat faster it does     */
    /*   not clean up heaps when threads exist unless the               */
    /*   'NasdaqDeleteHeap' function is called by the application.      */
    /*                                                                  */
    /********************************************************************/

extern FAST_HEAP *NasdaqCreateHeap( void );
extern bool NasdaqDelete( void *Store );
extern void NasdaqDeleteHeap( void );
extern void *NasdaqNew( size_t Size );
extern void *NasdaqRealloc( void *Store,size_t NewSize );
#endif
#ifndef DISABLE_GLOBAL_NEW

    /********************************************************************/
    /*                                                                  */
    /*   Overloading new and delete.                                    */
    /*                                                                  */
    /*   The following code overloads the gloabl new and delete         */
    /*   operators as needed.                                           */
    /*                                                                  */
    /********************************************************************/

__forceinline void *operator new( size_t Size )
	{ return NasdaqNew( Size ); }

__forceinline void operator delete( void *Store )
	{ (void) NasdaqDelete( Store ); }
#endif
#endif
