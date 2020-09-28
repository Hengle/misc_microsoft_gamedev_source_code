#ifndef _VECTOR_HPP_
#define _VECTOR_HPP_
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
#ifdef DEBUGGING

    /********************************************************************/
    /*                                                                  */
    /*   Constants exported from the class.                             */
    /*                                                                  */
    /*   The constants specified here control various aspects of the    */
    /*   vector classes debugging operations.                           */
    /*                                                                  */
    /********************************************************************/

CONST SBIT32 DebugDisplaySize		  = 8;
#endif

    /********************************************************************/
    /*                                                                  */
    /*   Vector creation, management and use.                           */
    /*                                                                  */
    /*   A vector is esentially an allocated array of data.  However,   */
    /*   vectors also have some special features.  A vector can be      */
    /*   aligned to any binary boundary and each element can also be    */
    /*   aligned to any binary boundary.  This is very helpful in       */
    /*   SMP systems as increased performance can be obtained by        */
    /*   using vectors on selected data structures.                     */
    /*                                                                  */
    /********************************************************************/

template <class TYPE> class VECTOR
    {
        //
        //   Private data.
        //
        CHAR                          *Address;
#ifdef DEBUGGING
		TYPE                          *Display[DebugDisplaySize];
#endif

        SBIT32                        Elements;
        SBIT32                        Size;

    public:
        //
        //   Public functions.
        //
        VECTOR
            ( 
            SBIT32					  NumberOfElements, 
            SBIT32					  AlignmentSize
            );

       VECTOR
            ( 
            SBIT32					  One, 
            SBIT32					  NumberOfElements, 
            SBIT32					  AlignmentSize
            );

       VECTOR
            ( 
            SBIT32					  One, 
            SBIT32					  Two, 
            SBIT32					  NumberOfElements, 
            SBIT32					  AlignmentSize
            );

        VOID Resize( SBIT32 Reallocate );

        ~VECTOR( VOID );

		//
		//   Public inline functions.
		//
        INLINE TYPE & operator[]( SBIT32 Index )
			{
#ifdef DEBUGGING
			if ( (Index < 0) || (Index >= Elements) )
				{ Failure( "Array subscript in VECTOR[]" ); }

#endif
			return (*((TYPE*) & Address[ (Index * Size) ]));
			}

        INLINE TYPE *operator&( VOID )
			{ return ((TYPE*) & Address[0]); }

		SBIT32 SizeOfVector( VOID ) 
			{ return Elements; }

	private:
		//
		//   Private functions.
		//
        VOID AllocateAndAlignVector
            ( 
            SBIT32                        NumberOfElements, 
            SBIT32                        AlignmentSize 
            );

		//
        //   Disabled operations.
        //
        VECTOR( CONST VECTOR & Copy );

        VOID operator=( CONST VECTOR & Copy );
    };

    /********************************************************************/
    /*                                                                  */
    /*   Class constructor.                                             */
    /*                                                                  */
    /*   Ceate a memory allocation and initialize it.  This call is     */
    /*   not thread safe and should only be made in a single thread     */
    /*   environment.                                                   */
    /*                                                                  */
    /********************************************************************/

template <class TYPE> VECTOR<TYPE>::VECTOR
        ( 
        SBIT32                        NumberOfElements, 
        SBIT32						  AlignmentSize
        )
    {
	REGISTER SBIT32 Count;

	//
	//   Allocate the storage.
	//
	AllocateAndAlignVector( NumberOfElements,AlignmentSize );

	//
	//   Call the constructors.
	//
	for ( Count = 0;Count < NumberOfElements;Count ++ )
		{ (VOID) PLACEMENT_NEW( & Address[ (Count * Size) ], TYPE ); }
    }

    /********************************************************************/
    /*                                                                  */
    /*   Class constructor.                                             */
    /*                                                                  */
    /*   Ceate a memory allocation and initialize it.  This call is     */
    /*   not thread safe and should only be made in a single thread     */
    /*   environment.                                                   */
    /*                                                                  */
    /********************************************************************/

template <class TYPE> VECTOR<TYPE>::VECTOR
        ( 
        SBIT32	                      One,
        SBIT32                        NumberOfElements, 
        SBIT32                        AlignmentSize
        )
    {
	REGISTER SBIT32 Count;

	//
	//   Allocate the storage.
	//
	AllocateAndAlignVector( NumberOfElements,AlignmentSize );

	//
	//   Call the constructors.
	//
	for ( Count = 0;Count < NumberOfElements;Count ++ )
		{ (VOID) PLACEMENT_NEW( & Address[ (Count * Size) ],TYPE( One ) ); }
    }

    /********************************************************************/
    /*                                                                  */
    /*   Class constructor.                                             */
    /*                                                                  */
    /*   Ceate a memory allocation and initialize it.  This call is     */
    /*   not thread safe and should only be made in a single thread     */
    /*   environment.                                                   */
    /*                                                                  */
    /********************************************************************/

template <class TYPE> VECTOR<TYPE>::VECTOR
        ( 
        SBIT32	                      One, 
        SBIT32                        Two, 
        SBIT32                        NumberOfElements, 
        SBIT32                        AlignmentSize 
        )
    {
	REGISTER SBIT32 Count;

	//
	//   Allocate the storage.
	//
	AllocateAndAlignVector( NumberOfElements,AlignmentSize );

	//
	//   Call the constructors.
	//
	for ( Count = 0;Count < NumberOfElements;Count ++ )
		{ (VOID) PLACEMENT_NEW( & Address[ (Count * Size) ],TYPE( One,Two ) ); }
    }

    /********************************************************************/
    /*                                                                  */
    /*   Allocate and align a vector.                                   */
    /*                                                                  */
    /*   We need to allocate some memory and align it as requested      */
    /*   by the user.                                                   */
    /*                                                                  */
    /********************************************************************/

template <class TYPE> VOID VECTOR<TYPE>::AllocateAndAlignVector
        ( 
        SBIT32                        NumberOfElements, 
        SBIT32                        AlignmentSize 
        )
    {
    if ( NumberOfElements > 0 )
        {
#ifdef DEBUGGING
		REGISTER SBIT32 Count;

#endif
        //
        //   We need to remember the initial alignment and count of
		//   elements for later.
        //   
        Elements = NumberOfElements;

        //
        //   Calculate the new element size using the requested alignment
		//   value.
        //
		Size = (AlignmentSize - (sizeof(TYPE) % AlignmentSize));
		Size = (Size == AlignmentSize) ? 0 : Size;
		Size += sizeof(TYPE);

        //
        //   Allocate memory and call constructor for each element.
        //   Calculate the start address for requested alignment.
        //
		Address = 
			(
			(CHAR*) _aligned_malloc
				( 
				(NumberOfElements * Size),
				CacheLineSize
				)
			);
#ifdef DEBUGGING

		//
		//   When we are in debug mode calculate the addresses
		//   of the first few elements of the vector.
		//
		for 
				( 
				Count = 0;
				(Count < NumberOfElements) && (Count < DebugDisplaySize);
				Count ++ 
				)
			{ Display[ Count ] = ((TYPE*) & Address[ (Count * Size) ]); }
#endif
        }
    else
        { Failure( "Allocation size in AllocateAndAlignVector" ); }
    }

    /********************************************************************/
    /*                                                                  */
    /*   Resize a memory allocation.                                    */
    /*                                                                  */
    /*   Resize a memory allocation and initialize the resized area.    */
    /*   This call is not thread safe and should only be made in a      */
    /*   single thread environment.                                     */
    /*                                                                  */
    /********************************************************************/

template <class TYPE> VOID VECTOR<TYPE>::Resize( SBIT32 Reallocate )
    {
	REGISTER SBIT32 Count;

    if ( Reallocate > 0 )
        {
		REGISTER SBIT32 Minimum = 
			((Elements < Reallocate) ? Elements : Reallocate);

		//
		//   Call the destructor for each element that is
		//   about to be deleted.
		//
		for ( Count = (Elements - 1);Count >= Minimum;Count -- )
			{ PLACEMENT_DELETE( & Address[ (Count * Size) ],TYPE ); }

		//
		//   Just ensure that the reallocation is not the same
		//   as the existing structure.
		//
		if ( Elements != Reallocate )
			{
			REGISTER CHAR *NewAddress = 
				(
				(CHAR*) _aligned_realloc
					( 
					Address,
					(Reallocate * Size),
					CacheLineSize 
					)
				);

			//
			//   Ensure we were able to allocate some memory.
			//
			if ( NewAddress != NULL )
				{

				//
				//   Call the constructor for each new element.
				//
				for ( Count = Elements;Count < Reallocate;Count ++ )
					{ 
					(VOID) PLACEMENT_NEW
						(
						& NewAddress[ (Count * Size) ], 
						TYPE 
						); 
					}
#ifdef DEBUGGING

				//
				//   When we are in debug mode calculate the addresses
				//   of the first few elements of the vector.
				//
				for 
						( 
						Count = 0;
						(Count < Reallocate) && (Count < DebugDisplaySize);
						Count ++ 
						)
					{ 
					Display[ Count ] = 
						((TYPE*) & NewAddress[ (Count * Size) ]); 
					}
#endif
	 
				//
				//   Finally, lets update the information about the new
				//   amended allocation.
				//   
				Address = NewAddress;
				Elements = Reallocate;
				}
			else
				{ Failure( "No memory available in Resize" ); }
 			}
       }
    else
        { Failure( "Rellocation size in Resize" ); }
    }

    /********************************************************************/
    /*                                                                  */
    /*   Class destructor.                                              */
    /*                                                                  */
    /*   Destory a vector.  This call is not thread safe and should     */
    /*   only be made in a single thread environment.                   */
    /*                                                                  */
    /********************************************************************/

template <class TYPE> VECTOR<TYPE>::~VECTOR( VOID )
    {
	REGISTER SBIT32 Count;

	//
	//   Call the destructors.
	//
	for ( Count = (Elements - 1);Count >= 0;Count -- )
		{ PLACEMENT_DELETE( & Address[ (Count * Size) ],TYPE ); }

	//
	//   Delete the storage.
	//
    _aligned_free( Address );
    }
#endif
