//                                        Ruler
//       1         2         3         4         5         6         7         8
//345678901234567890123456789012345678901234567890123456789012345678901234567890

    /********************************************************************/
    /*                                                                  */
    /*   Include files for necessary classes.                           */
    /*                                                                  */
    /*   The include files for classes are required in the              */
    /*   specification of this class.                                   */
    /*                                                                  */
    /********************************************************************/

#include "BenchmarkPCH.hpp"
#include "FastHeap.hpp"
#include "NasdaqHeap.hpp"
#include "New.hpp"
#include "Tls.hpp"

#ifndef NO_HEAP_PER_THREAD
    /********************************************************************/
    /*                                                                  */
    /*   Thread local store.                                            */
    /*                                                                  */
    /*   A thread local store value is a pointer that is unique for     */
    /*   each thread of execution.  We create a TLS value to store a    */
    /*   pointer to the current threads heap.                           */
    /*                                                                  */
    /********************************************************************/

static TLS Tls;

    /********************************************************************/
    /*                                                                  */
    /*   Create a new heap.                                             */
    /*                                                                  */
    /*   Create a new heap and assicate it with the current thread.     */
    /*                                                                  */
    /********************************************************************/

FAST_HEAP *NasdaqCreateHeap( void )
	{
	//
	//   We ensure the 'Tls' is available.
	//
	if ( Tls.Available() )
		{
		register FAST_HEAP *Heap = ((FAST_HEAP*) Tls.GetPointer());

		//
		//   If a thread does not already have an associated heap
		//   we try to create one.
		//
		if ( Heap == NULL )
			{
			register FAST_HEAP *Heap = 
#ifdef NO_DEFAULT_HEAP
				((FAST_HEAP*) malloc( sizeof(FAST_HEAP) ));
#else
				((FAST_HEAP*) DefaultHeap.New( sizeof(FAST_HEAP) ));
#endif

			if ( Heap != NULL )
				{
				PLACEMENT_NEW( Heap,FAST_HEAP )
					(
					4194304,
					True,
					True,
					True
					);

				Tls.SetPointer( ((void*) Heap) );
				}
			}

		return Heap;
		}
	else
		{ return NULL; }
	}

    /********************************************************************/
    /*                                                                  */
    /*   Delete a memory allocation.                                    */
    /*                                                                  */
    /*   Extract the heap pointer for the current thread and delete     */
    /*   a memory allocation.                                           */
    /*                                                                  */
    /********************************************************************/

bool NasdaqDelete( void *Store )
	{
	//
	//   We ensure the 'Tls' is available.
	//
	if ( Tls.Available() )
		{
		register FAST_HEAP *Heap = ((FAST_HEAP*) Tls.GetPointer());

		//
		//   If a thread does not already have an associated 
		//   heap we try to create one.
		//
		if ( Heap == NULL )
			{ Heap = NasdaqCreateHeap(); }

		//
		//   We now try to delete the allocation on the per 
		//   thread heap if it is available.
		//
		if ( Heap != NULL )
			{ return (Heap -> Delete( Store )); }
		}

	//
	//   If all else fails we try to use the default heap.
	//
#ifdef NO_DEFAULT_HEAP
	return (free( Store ), true);
#else
	return (DefaultHeap.Delete( Store ));
#endif
	}

    /********************************************************************/
    /*                                                                  */
    /*   Delete a heap.                                                 */
    /*                                                                  */
    /*   Delete any heap assocaited with this thread.                   */
    /*                                                                  */
    /********************************************************************/

void NasdaqDeleteHeap( void )
	{
	//
	//   We ensure the 'Tls' is available.
	//
	if ( Tls.Available() )
		{
		register FAST_HEAP *Heap = ((FAST_HEAP*) Tls.GetPointer());

		//
		//   If a thread has an assoicated heap then we 
		//   delete it.
		//
		if ( Heap != NULL )
			{ 
			PLACEMENT_DELETE( Heap,FAST_HEAP );

			Tls.SetPointer( ((void*) NULL) );
			}
		}
	}

    /********************************************************************/
    /*                                                                  */
    /*   Create a new memory allocation.                                */
    /*                                                                  */
    /*   Extract the heap pointer for the current thread and create     */
    /*   a new memory allocation.                                       */
    /*                                                                  */
    /********************************************************************/

void *NasdaqNew( size_t Size )
	{
	//
	//   We ensure the 'Tls' is available.
	//
	if ( Tls.Available() )
		{
		register FAST_HEAP *Heap = ((FAST_HEAP*) Tls.GetPointer());

		//
		//   If a thread does not already have an associated heap
		//   we try to create one.
		//
		if ( Heap == NULL )
			{ Heap = NasdaqCreateHeap(); }

		//
		//   We now try to create the allocation on the per
		//   thread heap if it is available.
		//
		if ( Heap != NULL )
			{ return (Heap -> New( ((int) Size) )); }
		}

	//
	//   If all else fails we try to use the default heap.
	//
#ifdef NO_DEFAULT_HEAP
	return (malloc( ((int) Size) ));
#else
	return (DefaultHeap.New( ((int) Size) ));
#endif
	}

    /********************************************************************/
    /*                                                                  */
    /*   Resize an existing allocation.                                 */
    /*                                                                  */
    /*   Extract the heap pointer for the current thread and resize     */
    /*   an existing allocation.                                        */
    /*                                                                  */
    /********************************************************************/

void *NasdaqRealloc( void *Store,size_t Size )
	{
	//
	//   We ensure the 'Tls' is available.
	//
	if ( Tls.Available() )
		{
		register FAST_HEAP *Heap = ((FAST_HEAP*) Tls.GetPointer());

		//
		//   If a thread does not already have an associated heap
		//   we try to create one.
		//
		if ( Heap == NULL )
			{ Heap = NasdaqCreateHeap(); }

		//
		//   We now try to resize the existing allocation on the
		//   per thread heap if it is available.
		//
		if ( Heap != NULL )
			{ return (Heap -> Resize( Store,((int) Size) )); }
		}

	//
	//   If all else fails we try to use the default heap.
	//
#ifdef NO_DEFAULT_HEAP
	return (realloc( Store,((int) Size) ));
#else
	return (DefaultHeap.Resize( Store,((int) Size) ));
#endif
	}
#endif
