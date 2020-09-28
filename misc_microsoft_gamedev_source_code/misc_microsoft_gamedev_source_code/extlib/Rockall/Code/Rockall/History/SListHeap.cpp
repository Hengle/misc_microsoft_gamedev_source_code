                          
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

#include "InterfacePCH.hpp"

#include "Common.hpp"
#include "DefaultServices.hpp"
#include "New.hpp"
#include "Prefetch.hpp"
#include "SList.hpp"
#include "SListHeap.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Constants local to the class.                                  */
    /*                                                                  */
    /*   The constants supplied here try to make the layout of the      */
    /*   the caches easier to understand and update.                    */
    /*                                                                  */
    /********************************************************************/

CONST SBIT32 FindCacheSize			  = 8192;
CONST SBIT32 FindCacheThreshold		  = 0;
CONST SBIT32 FindSize				  = 4096;
CONST SBIT32 MaxSList				  = 256;
CONST SBIT32 Stride1				  = 8;
CONST SBIT32 Stride2				  = 1024;

    /********************************************************************/
    /*                                                                  */
    /*   Structures local to the class.                                 */
    /*                                                                  */
    /*   The structures supplied here describe the layout of the        */
    /*   per thread caches.                                             */
    /*                                                                  */
    /********************************************************************/

typedef struct SLIST_CACHE : public SLIST
	{
	SBIT32							  MaxSize;
	SBIT32							  Space;
	}
SLIST_CACHE;

    /********************************************************************/
    /*                                                                  */
    /*   The description of the heap.                                   */
    /*                                                                  */
    /*   A heap is a collection of fixed sized allocation caches.       */
    /*   An allocation cache consists of an allocation size, the        */
    /*   number of pre-built allocations to cache, a chunk size and     */
    /*   a parent page size which is sub-divided to create elements     */
    /*   for this cache.  A heap consists of two arrays of caches.      */
    /*   Each of these arrays has a stride (i.e. 'Stride1' and          */
    /*   'Stride2') which is typically the smallest common factor of    */
    /*   all the allocation sizes in the array.                         */
    /*                                                                  */
    /********************************************************************/

STATIC ROCKALL::CACHE_DETAILS Caches1[] =
	{
	    //
	    //   Bucket   Size Of   Bucket   Parent
	    //    Size     Cache    Chunks  Page Size
		//
		{        8,      128,       32,     4096 },
		{       16,      128,       64,     4096 },
		{       24,       64,     4096,     4096 },

		{       32,       64,     4096,     4096 },
		{       40,       64,     4096,     4096 },
		{       48,       64,     4096,     4096 },
		{       56,       64,	  4096,     4096 },

		{       64,       64,     4096,     4096 },
		{       80,       64,     4096,     4096 },
		{       96,       64,     4096,     4096 },
		{      112,       64,     4096,     4096 },

		{      128,       32,     4096,     4096 },
		{      160,       16,     4096,     4096 },
		{      192,       16,     4096,     4096 },
		{      224,       16,     4096,     4096 },

		{      256,       32,     4096,     4096 },
		{      320,       16,     4096,     4096 },
		{      384,       16,     4096,     4096 },
		{      448,       16,     4096,     4096 },
		{      512,       16,     4096,     4096 },
		{      576,        8,     8192,     8192 },
		{      640,        8,     4096,     4096 },
		{      704,        8,     4096,     4096 },
		{      768,        8,     4096,     4096 },
		{      832,        8,     8192,     8192 },
		{      896,        8,     8192,     8192 },
		{      960,        8,     4096,     4096 },
		{ 0,0,0,0 }
	};

STATIC ROCKALL::CACHE_DETAILS Caches2[] =
	{
	    //
	    //   Bucket   Size Of   Bucket   Parent
	    //    Size     Cache    Chunks  Page Size
		//
		{     1024,        8,     8192,     8192 },
		{     2048,        8,     8192,     8192 },
		{     3072,        4,    65536,    65536 },
		{     4096,        8,    65536,    65536 },
		{     5120,        4,    65536,    65536 },
		{     6144,        4,    65536,    65536 },
		{     7168,        4,    65536,    65536 },
		{     8192,        8,    65536,    65536 },
		{     9216,        0,    65536,    65536 },
		{    10240,        0,    65536,    65536 },
		{    12288,        0,    65536,    65536 },
		{    16384,        2,    65536,    65536 },
		{    21504,        0,    65536,    65536 },
		{    32768,        0,    65536,    65536 },

		{    65536,        0,    65536,    65536 },
		{    65536,        0,    65536,    65536 },
		{ 0,0,0,0 }
	};

    /********************************************************************/
    /*                                                                  */
    /*   The description bit vectors.                                   */
    /*                                                                  */
    /*   All heaps keep track of allocations using bit vectors.  An     */
    /*   allocation requires 2 bits to keep track of its state.  The    */
    /*   following array supplies the size of the available bit         */
    /*   vectors measured in 32 bit words.                              */
    /*                                                                  */
    /********************************************************************/

STATIC int NewPageSizes[] = { 1,4,16,64,0 };

    /********************************************************************/
    /*                                                                  */
    /*   Static data structures.                                        */
    /*                                                                  */
    /*   The static data structures are initialized and prepared for    */
    /*   use here.                                                      */
    /*                                                                  */
    /********************************************************************/

STATIC PREFETCH Prefetch;

    /********************************************************************/
    /*                                                                  */
    /*   Class constructor.                                             */
    /*                                                                  */
    /*   The overall structure and layout of the heap is controlled     */
    /*   by the various constants and calls made in this function.      */
    /*   There is a significant amount of flexibility available to      */
    /*   a heap which can lead to them having dramatically different    */
    /*   properties.                                                    */
    /*                                                                  */
    /********************************************************************/

SLIST_HEAP::SLIST_HEAP
		( 
		int							  MaxFreeSpace,
		bool						  Recycle,
		bool						  SingleImage,
		bool						  ThreadSafe 
		) :
		//
		//   Call the constructors for the contained classes.
		//
		ROCKALL
			(
			Caches1,
			Caches2,
			FindCacheSize,
			FindCacheThreshold,
			FindSize,
			MaxFreeSpace,
			NewPageSizes,
			(SERVICES::DefaultServices()),
			Recycle,
			SingleImage,
			Stride1,
			Stride2,
			ThreadSafe
			)
	{
	REGISTER SBIT32 MaxCaches;

	//
	//   Compute the number of cache descriptions
	//   and the largest allocation size for each
	//   cache description table.
	//
	MaxCaches = (ComputeSize( ((CHAR*) Caches1),sizeof(CACHE_DETAILS) ));
	MaxSize = Caches1[ (MaxCaches-1) ].AllocationSize;
	MaxSListSize = (MaxSize / Stride1);

	//
	//   Create the linked list headers and a thread 
	//   local store variable to point at each threads
	//   private cache.
	//
	SListCaches = 
		((SLIST_CACHE*) SpecialNew( (MaxSListSize * sizeof(SLIST_CACHE)) ));

	//
	//   We may only activate the the heap if we manage
	//   to allocate space we requested and the stride
	//   size of the cache descriptions is a power of two. 
	//
	if
			(
			(COMMON::ConvertDivideToShift( Stride1,((SBIT32*) & ShiftSize) ))
				&& 
			(SListCaches != NULL) 
			)
		{
		REGISTER SBIT32 Count;

		//
		//   Activate the heap.
		//
		for( Count=0;Count < MaxSListSize;Count ++ )
			{
			REGISTER SLIST_CACHE *SListCache = & SListCaches[ Count ];

			PLACEMENT_NEW( SListCache,SLIST );
			
			SListCache -> MaxSize = 1; 
			SListCache -> Space = 0; 
			}

		Active = true;
		}
	else
		{ Active = false; }
	}

    /********************************************************************/
    /*                                                                  */
    /*   Memory deallocation.                                           */
    /*                                                                  */
    /*   When we delete an allocation we try to put it in the slist     */
    /*   cache so it can be reallocated later.                          */
    /*                                                                  */
    /********************************************************************/

bool SLIST_HEAP::Delete( void *Address,int Size )
    {
	//
	//   Although it is very rare there is a chance
	//   that we failed to build the basic heap structures.
	//
	if ( Active )
		{
		AUTO int Space;

		//
		//   We would like to put the deleted 
		//   allocation back in the slist cache.
		//   However, we don't have any information
		//   about it so we need to get its size
		//   and verify it will fit in the cache.
		//
		if ( ROCKALL::Details( Address,& Space ) )
			{
			//
			//   The SList cache can only deal with
			//   a range of sizes.
			//
			if ( (Space > 0) && (Space <= MaxSize) )
				{
				REGISTER SLIST_CACHE *SListCache =
					& SListCaches[ ((Space-1) >> ShiftSize) ];

				//
				//   Slave the allocation if there
				//   is space.
				//
				if ( SListCache -> Size() < SListCache -> MaxSize )
					{
					if ( SListCache -> Space <= 0 )
						{
						SListCache -> MaxSize = MaxSList;
						SListCache -> Space = Space;
						}
                  
               //
               //    Export barrier
               //
               MemoryBarrier(); 

					SListCache -> Push( ((SLIST*) Address) );

					return true;
					}
				}

			//
			//   We know the size of the allocation at 
			//   this point so it is only sensible to 
			//   pass it along to delete to make it run 
			//   faster.
			//
			Size = Space;
			}
		}

	//
	//   If all else fails call the heap directly and
	//   return the result.
	//
	return (ROCKALL::Delete( Address,Size ));
	}

    /********************************************************************/
    /*                                                                  */
    /*   Delete all allocations.                                        */
    /*                                                                  */
    /*   We check to make sure the heap is not corrupt and force        */
    /*   the return of all heap space back to the operating system.     */
    /*                                                                  */
    /********************************************************************/

void SLIST_HEAP::DeleteAll( bool Recycle )
    {
	//
	//   Although it is very rare there is a chance
	//   that we failed to build the basic heap structures.
	//
	if ( Active )
		{
		REGISTER SBIT32 Count;

		//
		//   Flush all the local slists.
		//
		for( Count=0;Count < MaxSListSize;Count ++ )
			{
			AUTO SLIST *Ignore;

			SListCaches[ Count ].PopAll( & Ignore ); 
			}
		}

	//
	//   Delete all outstanding allocations.
	//
	ROCKALL::DeleteAll( Recycle );
	}

    /********************************************************************/
    /*                                                                  */
    /*   Memory allocation.                                             */
    /*                                                                  */
    /*   We allocate space for the current thread from the local        */
    /*   per thread cache.  If we run out of space we bulk load         */
    /*   additional elements from a central shared heap.                */
    /*                                                                  */
    /********************************************************************/

void *SLIST_HEAP::New( int Size,int *Space,bool Zero )
    {
	//
	//   Although it is very rare there is a chance
	//   that we failed to build the basic heap structures.
	//
	if ( Active )
		{
		//
		//   The per slist cache can only slave 
		//   certain allocation sizes.  If the size 
		//   is out of range then pass it along to 
		//   the allocator.
		//
		if ( (Size > 0) && (Size <= MaxSize) )
			{
			AUTO VOID *Address;
			REGISTER SLIST_CACHE *SListCache =
				& SListCaches[ ((Size-1) >> ShiftSize) ];

			//
			//   If there is some space in the 
			//   current cache stack we allocate it.
			//
			if ( SListCache -> Pop( ((SLIST**) & Address) ) )
				{
            //
            //    Import barrier
            //
            MemoryBarrier(); 
            
				//
				//   Prefetch the first cache line of  
				//   the allocation if we are running
				//   a Pentium III or better.
				//
				Prefetch.L1( ((CHAR*) Address),1 );

				//
				//   If the caller want to know the
				//   real size then we supply it.
				//
				if ( Space != NULL )
					{ (*Space) = SListCache -> Space; }

				//
				//   If we need to zero the allocation
				//   we do it here.
				//
				if ( Zero )
					{ ZeroMemory( Address,SListCache -> Space ); }

				return Address;
				}
			}
		}

	//
	//   If all else fails call the heap directly and
	//   return the result.
	//
	return (ROCKALL::New( Size,Space,Zero ));
	}

    /********************************************************************/
    /*                                                                  */
    /*   Search an slist cache.                                         */
    /*                                                                  */
    /*   Search an slist cache for a specific address for verify        */
    /*   and walk heap.                                                 */
    /*                                                                  */
    /********************************************************************/

bool SLIST_HEAP::SearchSListCache( void *Address,int Size )
    {
	AUTO void *SList;
	AUTO bool Result = false;

	//
	//   Find the associated slist.
	//
	if ( (Size > 0) && (Size <= MaxSize) )
		{
		REGISTER SLIST_CACHE *SListCache =
			& SListCaches[ ((Size-1) >> ShiftSize) ];

		//
		//   Extract the entire list.
		//
		SListCache -> PopAll( ((SLIST**) & SList) );

		//
		//   Walk the list and delete each allocation
		//   while checking to see it it matches the
		//   supplied address.
		//
		for 
				( 
				/* void */;
				SList != NULL;
				SList = (*((void**) SList))
				)
			{
			//
			//   Verify the supplied address is not
			//   in the cache.
			//
			if ( Address == SList )
				{ Result = true; }

			//
			//   It is really tricky to put the slist
			//   back and even if we do we may end up
			//   walking it again.  The best bet seems
			//   to be to delete any cached allocations.
			//
			Delete( SList,Size );
			}
		}

	return Result;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Verify memory allocation details.                              */
    /*                                                                  */
    /*   Extract information about a memory allocation and just for     */
    /*   good measure check the guard words at the same time.           */
    /*                                                                  */
    /********************************************************************/

bool SLIST_HEAP::Verify( void *Address,int *Space )
    {
	AUTO int Size;

	//
	//   Extract information about the memory 
	//   allocation.
	//
	if ( ROCKALL::Verify( Address,& Size ) )
		{
		//
		//   If the caller requested the allocation
		//   size then return it.
		//
		if ( Space != NULL )
			{ (*Space) = Size; }

		//
		//   Although it is very rare there is a 
		//   chance that we failed to build the 
		//   basic heap structures.
		//
		if ( Active )
			{
			//
			//   Search for the allocation in the
			//   slist cache.
			//
			return (! SearchSListCache( Address,Size ));
			}
		else
			{ return true; }
		}
	else
		{ return false; }
	}

    /********************************************************************/
    /*                                                                  */
    /*   Walk the heap.                                                 */
    /*                                                                  */
    /*   We have been asked to walk the heap.  It is hard to know       */
    /*   why anybody might want to do this given the rest of the        */
    /*   functionality available.  Nonetheless, we just do what is      */
    /*   required to keep everyone happy.                               */
    /*                                                                  */
    /********************************************************************/

bool SLIST_HEAP::Walk( bool *Activity,void **Address,int *Space )
    {
	//
	//   Walk the heap.
	//
	if ( ROCKALL::Walk( Activity,Address,Space ) )
		{
		//
		//   Although it is very rare there is a 
		//   chance that we failed to build the 
		//   basic heap structures.
		//
		if ( Active )
			{
			//
			//   Search for the allocation in the
			//   slist cache.
			//
			(*Activity) = (! SearchSListCache( (*Address),(*Space) ));
			}

		return true;
		}
	else
		{ return false; }
	}

    /********************************************************************/
    /*                                                                  */
    /*   Class destructor.                                              */
    /*                                                                  */
    /*   Destory the heap.                                              */
    /*                                                                  */
    /********************************************************************/

SLIST_HEAP::~SLIST_HEAP( void )
	{
	//
	//   Although it is very rare there is a chance
	//   that we failed to build the basic heap structures.
	//
	if ( Active )
		{
		REGISTER SBIT32 Count;

		//
		//   Deactivate the heap.
		//
		for( Count=0;Count < MaxSListSize;Count ++ )
			{
			AUTO SLIST *Ignore;
			REGISTER SLIST_CACHE *SListCache = & SListCaches[ Count ];

			SListCache -> PopAll( & Ignore ); 
			
			SListCache -> MaxSize = 0; 
			SListCache -> Space = 0; 

			PLACEMENT_DELETE( SListCache,SLIST );
			}

		Active = false;
		}
	}
