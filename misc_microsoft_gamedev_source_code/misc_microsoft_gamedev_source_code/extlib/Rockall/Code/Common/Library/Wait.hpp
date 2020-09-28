#ifndef _WAIT_HPP_
#define _WAIT_HPP_
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
#include "Semaphore.hpp"
#include "SList.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Constants exported from the class.                             */
    /*                                                                  */
    /*   The constants supplied here control static semaphore list.     */
    /*                                                                  */
    /********************************************************************/

CONST SBIT32 MaxSemaphores			  = 256;

    /********************************************************************/
    /*                                                                  */
    /*   A provider of waits (not weights :-).                          */
    /*                                                                  */
    /*   When a thread needs to wait a suitable semaphore can be        */
    /*   dynamically acquired, used and later returned.  Care must      */
    /*   be taken to release the wait until the semaphore is free.      */
    /*                                                                  */
    /********************************************************************/

class WAIT : public ASSEMBLY
    {
		//
		//   Private type definitions.
		//
		typedef struct SEMAPHORE_LIST : public SLIST<SEMAPHORE_LIST>
			{
	        SEMAPHORE                 Semaphore;
			}
		SEMAPHORE_LIST;

		//
		//   Private data.
		//
		SEMAPHORE_LIST				  *Semaphore;

        //
        //   Static private data.
        //
		STATIC SBIT32				  Activations;
		STATIC SBIT32				  MaxThreads;
		STATIC SLIST<SEMAPHORE_LIST>  SList;

#ifdef DISABLE_HEAP_USE
		STATIC SEMAPHORE_LIST		  Semaphores[ MaxSemaphores ];
#endif

    public:
        //
        //   Public functions.
        //
        WAIT( SBIT32 NewMaxThreads = DefaultMaxThreads );

        VOID ClaimSemaphore( SEMAPHORE_LIST **Current = NULL );

        VOID ReleaseSemaphore( VOID );

        ~WAIT( VOID );

		//
		//   Public inline functions.
		//
		INLINE VOID Signal( SBIT32 Count = 1 )
			{
			AUTO SEMAPHORE_LIST *Current = Semaphore;

			if ( Current == NULL )
				{ ClaimSemaphore( & Current ); }

			Current -> Semaphore.Signal( Count ); 
			}

		INLINE BOOLEAN Wait( SBIT32 Timeout = INFINITE )
			{
			AUTO SEMAPHORE_LIST *Current = Semaphore;

			if ( Current == NULL )
				{ ClaimSemaphore( & Current ); }

			return Current -> Semaphore.Wait( Timeout ); 
			}

	private:
        //
        //   Disabled operations.
        //
        WAIT( CONST WAIT & Copy );

        VOID operator=( CONST WAIT & Copy );
    };
#endif
