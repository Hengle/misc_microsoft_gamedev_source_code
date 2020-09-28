#ifndef _SLIST_HEAP_HPP_
#define _SLIST_HEAP_HPP_
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

#include "Rockall.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Class forward references.                                      */
    /*                                                                  */
    /*   We need to refer to the following classes before they are      */
    /*   fully specified so here we list them as forward references.    */
    /*                                                                  */
    /********************************************************************/

struct SLIST_CACHE;

    /********************************************************************/
    /*                                                                  */
    /*   A SList heap.                                                  */
    /*                                                                  */
    /*   A SList heap is optimized for SMP performance.  Each thread    */
    /*   is allocates memory from a lockless cache.                     */
    /*                                                                  */
    /********************************************************************/

class ROCKALL_DLL_LINKAGE SLIST_HEAP : public ROCKALL
    {
		//
		//   Private data.
		//
		bool						  Active;

		int							  MaxSize;
		int							  MaxSListSize;
		int							  ShiftSize;

		SLIST_CACHE					  *SListCaches;

   public:
        //
        //   Public functions.
        //
        SLIST_HEAP
			( 
			int						  MaxFreeSpace = (4 * HalfMegabyte),
			bool					  Recycle = true,
			bool					  SingleImage = false,
			bool					  ThreadSafe = true 
			);

		//
		//   Manipulate allocations.
		//
		//   The first group of functions manipulate 
		//   single or small arrays of allocations. 
		//
		virtual bool Delete
			( 
			void					  *Address,
			int						  Size = NoSize 
			);

		virtual void *New
			( 
			int						  Size,
			int						  *Space = NULL,
			bool					  Zero = false
			);

		virtual bool Verify
			( 
			void					  *Address = NULL,
			int						  *Space = NULL 
			);

		//
		//   Manipulate the heap.
		//
		//   The second group of functions act upon a heap
		//   as a whole.
		//
		virtual void DeleteAll( bool Recycle = true );

		virtual bool Walk
			(
			bool					  *Active,
			void					  **Address,
			int						  *Space
			);

        ~SLIST_HEAP( void );

   private:
		//
	    //   Private functions.
		//
	    bool SearchSListCache( void *Address,int Size );

	    //
        //   Disabled operations.
 		//
		//   All copy constructors and class assignment 
		//   operations are disabled.
        //
        SLIST_HEAP( const SLIST_HEAP & Copy );

        void operator=( const SLIST_HEAP & Copy );
    };
#endif
