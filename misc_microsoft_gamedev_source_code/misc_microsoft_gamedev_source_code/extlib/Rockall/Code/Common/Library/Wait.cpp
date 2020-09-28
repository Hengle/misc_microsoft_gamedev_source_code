                          
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

#include "Wait.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Static member initialization.                                  */
    /*                                                                  */
    /*   Static member initialization sets the initial value for all    */
    /*   static members.                                                */
    /*                                                                  */
    /********************************************************************/

SBIT32 WAIT::Activations = 0;
SBIT32 WAIT::MaxThreads = 0;
SLIST<WAIT::SEMAPHORE_LIST> WAIT::SList;

#ifdef DISABLE_HEAP_USE
WAIT::SEMAPHORE_LIST WAIT::Semaphores[ MaxSemaphores ];
#endif

    /********************************************************************/
    /*                                                                  */
    /*   Class constructor.                                             */
    /*                                                                  */
    /*   Create a new and prepare it for use.                           */
    /*                                                                  */
    /********************************************************************/

WAIT::WAIT( SBIT32 NewMaxThreads )
    {
	//
	//   When we have the heap unavailable and we
	//   are using static semaphore we need to prime
	//   the slist when the class is activated for the
	//   first time.
	//
	if ( AtomicIncrement( & Activations ) == 1 )
		{
#ifdef DISABLE_HEAP_USE
		REGISTER SBIT32 Count;

		//
		//   We push each static semaphore on to the
		//   slist.  If we run out at any point then
		//   we will fail.
		//
		for ( Count=0;Count < MaxSemaphores;Count ++ )
			{ SList.Push( & Semaphores[ Count ] ); }
#endif

		//
		//   Setup class contraol variables.
		//
		MaxThreads = NewMaxThreads;
		}
    }

    /********************************************************************/
    /*                                                                  */
    /*   Claim a semaphore.                                             */
    /*                                                                  */
    /*   When we need a semaphore we may claim one so that we can       */
    /*   sleep on it.                                                   */
    /*                                                                  */
    /********************************************************************/

VOID WAIT::ClaimSemaphore( SEMAPHORE_LIST **Update )
    {
	//
	//   We need to ensure that we don't already have
	//   an active semaphore.  If we do we just exit.
	//
	if ( Semaphore == NULL )
		{
		//
		//   We extract a free semaphore from the slist
		//   if one is available.
		//
		if ( ! SList.Pop( Update ) )
			{
#ifndef DISABLE_HEAP_USE
			//
			//   When we do not have a semaphore on the
			//   slist we try to create a new one.
			//
			(*Update) = new SEMAPHORE_LIST;
#endif
			}
         
      //
      //    Import barrier
      //
      MemoryBarrier(); 

		//
		//   We ensure that we have managed to obtain 
		//   a semaphore somehow.
		//
		if ( (*Update) != NULL )
			{
			REGISTER SEMAPHORE_LIST *Current =
				(
				(SEMAPHORE_LIST*) AtomicCompareExchangePointer
					( 
					((VOLATILE VOID**) & Semaphore),
					(*Update),
					NULL 
					)
				);

			//
			//   We now try to place the semaphore in
			//   the current slot.  If we find that we
			//   already have one somehow we push it on
			//   the slist for later.
			//
			if ( Current != NULL )
				{
				//
				//   We already have a semaphore so save
				//   the new one and return the current one.
				//
				SList.Push( (*Update) );

				(*Update) = Current;
				}
			}
		else
			{ Failure( "No semaphore in Create" ); }
		}
    }

    /********************************************************************/
    /*                                                                  */
    /*   Relaese a semaphore.                                           */
    /*                                                                  */
    /*   When we have finished with a semaphore we can return it for    */
    /*   reuse.  It is the callers responsibility to ensure the         */
    /*   semaphore is free.  If not a thread could sleep forever as     */
    /*   any wakeup signal could be sent to an alternative thread.      */
    /*                                                                  */
    /********************************************************************/

VOID WAIT::ReleaseSemaphore( VOID )
    {
	REGISTER SEMAPHORE_LIST *Current = Semaphore;

	//
	//   We ensure that we have a semaphore.  If not we
	//   just quitely exit.
	//
	if ( Current != NULL )
		{
		//
		//   We try to extract the semaphore so we can
		//   push it on the slist.  If we fail we just
		//   forget it.
		//
		if
				(
				AtomicCompareExchangePointer
					( 
					((VOLATILE VOID**) & Semaphore),
					NULL,
					Current
					) != NULL
				)
			{
            //
            //    Export barrier
            //
            MemoryBarrier(); 
            
            SList.Push( Current );
         }
		}
    }

    /********************************************************************/
    /*                                                                  */
    /*   Class destructor.                                              */
    /*                                                                  */
    /*   Destory a wait and if needed clean up.                         */
    /*                                                                  */
    /********************************************************************/

WAIT::~WAIT( VOID )
    {
	//
	//   When the last activation of the class has
	//   completed we empty the slist and delete any 
	//   semaphores if needed.
	//
	if ( AtomicDecrement( & Activations ) == 0 )
		{
		REGISTER SEMAPHORE_LIST *Current;

#ifdef DISABLE_HEAP_USE
		//
		//   Walk the slist to remove any semaphores.
		//
		while ( SList.Pop( & Current ) );
#else
		//
		//   Walk the slist and delete any semaphores.
		//
		while ( SList.Pop( & Current ) )
			{
            //
            //    Import barrier
            //
            MemoryBarrier(); 
            
            delete Current;
         }
#endif
		}
    }
