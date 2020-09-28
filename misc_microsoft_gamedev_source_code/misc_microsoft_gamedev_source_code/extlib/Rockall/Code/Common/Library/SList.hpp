#ifndef _SLIST_HPP_
#define _SLIST_HPP_
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

#include "Assembly.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   A lockless list.                                               */
    /*                                                                  */
    /*   An SList is a lockless list suitable for high contention       */
    /*   SMP access.                                                    */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,SBIT32 OFFSET=0> class SLIST : public ASSEMBLY
    {
		//
		//   Private type definitions.
		//
		typedef struct
			{
	        SLIST                     *Address;
	        SBIT16                    Size;
	        SBIT16                    Version;
			}
		HEADER;

        //
        //   Private data.
        //
        VOLATILE HEADER	              Header;

    public:
        //
        //   Public functions.
        //
        SLIST( VOID );

		VOID CloneList( SLIST *List );

		VOID JoinList( SLIST *List );

		VOID MoveList( SLIST *List );

		VOID ZeroList( VOID );

		BOOLEAN Pop( TYPE **Type );

		VOID Push( TYPE *Type );

        ~SLIST( VOID );

		//
		//   Public inline functions.
		//
		INLINE TYPE *Address( VOID )
			{ return ((TYPE*) (((CHAR*) Header.Address) - OFFSET)); }

		INLINE BOOLEAN Empty( VOID )
			{ return (Header.Size == 0); }

		INLINE SBIT32 Size( VOID )
			{ return Header.Size; }

	private:
        //
        //   Disabled operations.
        //
        SLIST( CONST SLIST & Copy );

        VOID operator=( CONST SLIST & Copy );
    };

    /********************************************************************/
    /*                                                                  */
    /*   Class constructor.                                             */
    /*                                                                  */
    /*   Create a new slist and initialize it.  This call is not        */
    /*   thread safe and should only be made in a single thread         */
    /*   environment.                                                   */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,SBIT32 OFFSET> SLIST<TYPE,OFFSET>::SLIST( VOID )
    {
	//
	//   Zero the list head.
	//
	Header.Address = NULL;
	Header.Size = 0;
	Header.Version = 0;
    }

    /********************************************************************/
    /*                                                                  */
    /*   Clone all of the list.                                         */
    /*                                                                  */
    /*   Clone the head of the slist to allow it to be examined.        */
    /*   Extreme caution is required with is function as grave          */
    /*   disorder may occur if any element is removed from the          */
    /*   list after a clone have been made.                             */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,SBIT32 OFFSET> VOID SLIST<TYPE,OFFSET>::CloneList
		( 
		SLIST						  *List 
		)
    {
	AUTO HEADER Original;

	//
	//   We repeatedly try to update the list until
	//   we are sucessful.
	//
	do 
		{
		//
		//   Clone the current head of the list.
		//
		(*((SBIT64*) & Original)) = (*((SBIT64*) & Header));
		}
	while 
		( 
		AtomicCompareExchange64
			( 
			((SBIT64*) & Header),
			(*((SBIT64*) & Original)),
			(*((SBIT64*) & Original))
			) 
				!= 
		(*((SBIT64*) & Original))
		);

	//
	//   We now store the extracted list in the target
	//   to allow it to be processed.  We know the new
	//   clone is a valid snapshot because of the atomic
	//   compare and update. 
	//
	(*((SBIT64*) List)) = (*((SBIT64*) & Original));
    }

    /********************************************************************/
    /*                                                                  */
    /*   Join two lists.                                                */
    /*                                                                  */
    /*   Join two lists together and form a single larger list.         */
    /*   The head of the one list is zeroed so it may continue to       */
    /*   be used in the normal way.                                     */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,SBIT32 OFFSET> VOID SLIST<TYPE,OFFSET>::JoinList
		( 
		SLIST						  *List 
		)
    {
	AUTO SLIST NewList;

	//
	//   When we join two lists we must safely extract
	//   the first list so we can join it without the
	//   fear of further updates.
	//
	List.MoveList( & NewList );

	//
	//   We may find the the new list is empty in which
	//   case we don't need to do anything.
	//
	if ( NewList.Address != NULL )
		{
		AUTO HEADER Original;
		AUTO HEADER Update;
		AUTO SLIST *Current;

		//
		//   Walk along the list and find the tail so we
		//   can join it to the head of the other list.
		//
		for 
			( 
			Current = NewList.Address;
			Current -> Address != NULL;
			Current = Current -> Address 
			);

		//
		//   We repeatedly try to update the list until
		//   we are sucessful.
		//
		do 
			{
			//
			//   Clone the current head of the other list.
			//
			(*((SBIT64*) & Original)) = (*((SBIT64*) & Header));
			(*((SBIT64*) & Update)) = (*((SBIT64*) & Original));

			//
			//   Join the tail of the new list with the 
			//   head of the other list.
			//
			Current -> Header.Address = Original.Address;

			//
			//   Update the list head.
			//
			Update.Address = NewList.Address;
			Update.Size += NewList.Size;
			Update.Version ++;
			}
		while 
			( 
			AtomicCompareExchange64
				( 
				((SBIT64*) & Header),
				(*((SBIT64*) & Update)),
				(*((SBIT64*) & Original))
				) 
					!= 
			(*((SBIT64*) & Original))
			);
		}
    }

    /********************************************************************/
    /*                                                                  */
    /*   Move all of the list.                                          */
    /*                                                                  */
    /*   Move the head of the slist to allow it to be examined.         */
    /*   The head of the list is zeroed so it may continue to be        */
    /*   used in the normal way.                                        */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,SBIT32 OFFSET> VOID SLIST<TYPE,OFFSET>::MoveList
		( 
		SLIST						  *List 
		)
    {
	AUTO HEADER Original;
	AUTO SBIT64 Update = 0;

	//
	//   We repeatedly try to update the list until
	//   we are sucessful.
	//
	do 
		{
		//
		//   Clone the current head of the list.
		//
		(*((SBIT64*) & Original)) = (*((SBIT64*) & Header));

		//
		//   Update the version number to ensure
		//   a 'ZeroList' and 'Pop' don't race if
		//   they are repeatedly used in close 
		//   succession.
		//
		Update.Version = (Original.Version + 1);
		}
	while 
		( 
		AtomicCompareExchange64
			( 
			((SBIT64*) & Header),
			(*(SBIT64*) & Update),
			(*(SBIT64*) & Original)
			) 
				!= 
		(*((SBIT64*) & Original))
		);

	//
	//   We now store the extracted list in the target
	//   to allow it to be processed.  There is still
	//   only one copy of the head of the list so all
	//   operations are still safe.
	//
	(*((SBIT64*) List)) = (*((SBIT64*) & Original));
    }

    /********************************************************************/
    /*                                                                  */
    /*   Zero all of the list.                                          */
    /*                                                                  */
    /*   Zero the head of the so it no longer contains any elements.    */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,SBIT32 OFFSET> VOID SLIST<TYPE,OFFSET>::ZeroList( VOID )
    {
	AUTO HEADER Original;
	AUTO SBIT64 Update = 0;

	//
	//   We repeatedly try to update the list until
	//   we are sucessful.
	//
	do 
		{
		//
		//   Clone the current head of the list.
		//
		(*((SBIT64*) & Original)) = (*((SBIT64*) & Header));

		//
		//   Update the version number to ensure
		//   a 'ZeroList' and 'Pop' don't race if
		//   they are repeatedly used in close 
		//   succession.
		//
		Update.Version = (Original.Version + 1);
		}
	while 
		( 
		AtomicCompareExchange64
			( 
			((SBIT64*) & Header),
			(*(SBIT64*) & Update),
			(*(SBIT64*) & Original)
			) 
				!= 
		(*((SBIT64*) & Original))
		);
    }
    /********************************************************************/
    /*                                                                  */
    /*   Pop an element.                                                */
    /*                                                                  */
    /*   Pop an element from the list.                                  */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,SBIT32 OFFSET> BOOLEAN SLIST<TYPE,OFFSET>::Pop
		( 
		TYPE						  **Type 
		)
    {
	AUTO HEADER Original = { 0,0,0 };
	AUTO HEADER Update = { 0,0,0 };

	//
	//   We repeatedly try to update the list until
	//   we are sucessful.
	//
	do 
		{
		//
		//   Clone the current head of the list.
		//
		(*((SBIT64*) & Original)) = (*((SBIT64*) & Header));
		(*((SBIT64*) & Update)) = (*((SBIT64*) & Original));

		//
		//   We need to be sure that there is an element
		//   to extract.  If not we exit.
		//
		if ( Original.Address != NULL )
			{
			//
			//   Update the list head.  We only need to update
			//   the version on either 'Pop' or 'Push' but not
			//   both so we do it on 'Push'.
			//
			Update.Address = Original.Address -> Header.Address;
			Update.Size --;
			}
		else
			{ return False; }
		}
	while 
		( 
		AtomicCompareExchange64
			( 
			((SBIT64*) & Header),
			(*((SBIT64*) & Update)),
			(*((SBIT64*) & Original))
			) 
				!= 
		(*((SBIT64*) & Original))
		);

	//
	//   The slist elements point to the next slist link
	//   instead of the start of the class or structure.
	//   The adjustment to the head of the structure is
	//   made by the following code.
	//
	(*Type) = ((TYPE*) (((CHAR*) Original.Address) - OFFSET));

	return True;
    }

    /********************************************************************/
    /*                                                                  */
    /*   Push an element.                                               */
    /*                                                                  */
    /*   Push an element onto the list.                                 */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,SBIT32 OFFSET> VOID SLIST<TYPE,OFFSET>::Push
		( 
		TYPE						  *Type 
		)
    {
	AUTO HEADER Original = { 0,0,0 };
	AUTO HEADER Update = { 0,0,0 };
	AUTO SLIST *SList = NULL;

	//
	//   We assume we have been supplied a pointer 
	//   to the sart of the class or structure.  The  
	//   adjustment to the head of the structure is
	//   made by the following code.
	//
	SList = ((SLIST*) (((CHAR*) Type) + OFFSET));

	//
	//   We repeatedly try to update the list until
	//   we are sucessful.
	//
	do 
		{
		//
		//   Clone the current head of the list.
		//
		(*((SBIT64*) & Original)) = (*((SBIT64*) & Header));
		(*((SBIT64*) & Update)) = (*((SBIT64*) & Original));

		//
		//   The current list head is copied to 
		//   the new element pointer.
		//
		SList -> Header.Address = Original.Address;

		//
		//   Update the list head.  We only need to update
		//   the version on either 'Pop' or 'Push' but not
		//   both so we do it on 'Push'.
		//
		Update.Address = SList;
		Update.Size ++;
		Update.Version ++;
		}
	while 
		( 
		AtomicCompareExchange64
			( 
			((SBIT64*) & Header),
			(*((SBIT64*) & Update)),
			(*((SBIT64*) & Original))
			) 
				!= 
		(*((SBIT64*) & Original))
		);
    }

    /********************************************************************/
    /*                                                                  */
    /*   Class destructor.                                              */
    /*                                                                  */
    /*   Destory a SList.  This call is not thread safe and should      */
    /*   only be made in a single thread environment.                   */
    /*                                                                  */
    /********************************************************************/

template <class TYPE,SBIT32 OFFSET> SLIST<TYPE,OFFSET>::~SLIST( VOID )
    {
	//
	//   The list should be empty.
	//
    if ( (Header.Address != NULL) || (Header.Size != 0) )
		{ Failure( "Non-empty list in destructor for SLIST" ); }
    }
#endif
