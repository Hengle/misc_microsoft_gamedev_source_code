#ifndef _REFERENCES_HPP_
#define _REFERENCES_HPP_
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

    /********************************************************************/
    /*                                                                  */
    /*   The empty class.                                               */
    /*                                                                  */
    /*   The empty class is a useful device to allow a class to         */
    /*   appear at may levels in an inheritance heirarchy.              */
    /*                                                                  */
    /********************************************************************/

class EMPTY_CLASS
    {
    public:
        //
        //   Public inline functions.
        //
        EMPTY_CLASS( VOID )
			{ /* void */ }

        VIRTUAL ~EMPTY_CLASS( VOID )
			{ /* void */ }

	private:
        //
        //   Disabled operations.
        //
        EMPTY_CLASS( CONST EMPTY_CLASS & Copy );

        VOID operator=( CONST EMPTY_CLASS & Copy );
    };

    /********************************************************************/
    /*                                                                  */
    /*   Automated reference counting.                                  */
    /*                                                                  */
    /*   When we need to reference count an object we can use this      */
    /*   class to keep track of the references.                         */
    /*                                                                  */
    /********************************************************************/

template <class BASE=EMPTY_CLASS> class REFERENCES : public BASE
    {
		//
		//   Private data.
		//
		SBIT32						  References;

    public:
        //
        //   Public inline functions.
        //
        REFERENCES( VOID )
			{ References = 1; }

		INLINE SBIT32 GetReferences( VOID )
			{ return References; }

		INLINE SBIT32 Increment( VOID )
			{ return AtomicIncrement( & References ); }

		INLINE SBIT32 Decrement( VOID )
			{
			REGISTER SBIT32 Result = AtomicDecrement( & References );
			//
			//   Delete the entire object after
			//   the last reference is removed.
			//
			if ( Result == 0 )
				{ delete this; }

			return Result;
			}

        VIRTUAL ~REFERENCES( VOID )
			{
			//
			//   We really should have no outstanding
			//   references at this point.
			//
			if ( References != 0 )
				{ Failure( "References in destructor for REFERENCES" ); } 
			}

	private:
        //
        //   Disabled operations.
        //
        REFERENCES( CONST REFERENCES & Copy );

        VOID operator=( CONST REFERENCES & Copy );
    };
#endif
