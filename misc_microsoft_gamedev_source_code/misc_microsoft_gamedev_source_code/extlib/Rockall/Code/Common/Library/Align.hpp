#ifndef _ALIGN_HPP_
#define _ALIGN_HPP_
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

#include "Global.hpp"

#include "New.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Alignment of structures to cache line boundaries.              */
    /*                                                                  */
    /*   This class aligns data structures to cache line boundaries.    */
    /*                                                                  */
    /********************************************************************/

template <class TYPE> class ALIGN
    {
        //
        //   Private data.
        //
        TYPE                          *Adress;

    public:
        //
        //   Public functions.
        //
        ALIGN( VOID );

        ~ALIGN( VOID );

		//
		//   Public inline functions.
		//
        INLINE TYPE *operator&( VOID )
			{ return Address; }

	private:
        //
        //   Disabled operations.
        //
        ALIGN( CONST ALIGN & Copy );

        VOID operator=( CONST ALIGN & Copy );
    };

    /********************************************************************/
    /*                                                                  */
    /*   Class constructor.                                             */
    /*                                                                  */
    /*   Ceate an aligned memory allocation and initialize it.          */
    /*                                                                  */
    /********************************************************************/

template <class TYPE> ALIGN<TYPE>::ALIGN( VOID )
    {
    REGISTER CHAR *Address;

	//
	//   Allocate an aligned memory allocation.
	//
	Address = _aligned_malloc( sizeof(TYPE),CacheLineSize );

	//
	//   Call the constructor to initialize the type.
	//
    PLACEMENT_NEW( Address,TYPE );
    }

    /********************************************************************/
    /*                                                                  */
    /*   Class destructor.                                              */
    /*                                                                  */
    /*   Destory the memory allocation.                                 */
    /*                                                                  */
    /********************************************************************/

template <class TYPE> ALIGN<TYPE>::~ALIGN( VOID )
    { 
	//
	//   Call the destructor for the aligned type.
	//
    PLACEMENT_DELETE( Address,TYPE );

	//
	//   Delete the aligned memory allocation.
	//
	_aligned_free( Address ); 
	}
#endif
