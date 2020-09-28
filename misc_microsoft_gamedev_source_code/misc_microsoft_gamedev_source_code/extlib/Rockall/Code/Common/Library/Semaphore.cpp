                          
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

#include "Semaphore.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Class constructor.                                             */
    /*                                                                  */
    /*   Create a new semaphore and initialize it.  This call is not    */
    /*   thread safe and should only be made in a single thread         */
    /*   environment.                                                   */
    /*                                                                  */
    /********************************************************************/

#if 0

SEMAPHORE::SEMAPHORE( SBIT32 NewMaxThreads )
    {
	//
	//   We need to ensure that the number of threads
	//   makes sense.
	//
	if ( NewMaxThreads > 0 )
		{
		//
		//   Setup class control information.
		//
		MaxThreads = NewMaxThreads;
		Semaphore = NULL;
		Signals = 0;
		Waiting = 0;
		}
	else
		{ Failure( "Max threads in constructor for SEMAPHORE" ); }
    }

    /********************************************************************/
    /*                                                                  */
    /*   Signal a semaphore.                                            */
    /*                                                                  */
    /*   Signal a semaphore to wake sleeping threads.  We go to some    */
    /*   trouble to deal with the interesting case were the OS will     */
    /*   not allow us to create an OS semaphore.                        */
    /*                                                                  */
    /********************************************************************/

VOID SEMAPHORE::Signal( SBIT32 Count )
    {
	//
	//   We need to find out whether there is an existing
	//   semaphore.  If not then things can be tricky.
	//
	if ( Semaphore == NULL )
		{
      //
      //    Export barrier
      //
      MemoryBarrier();         
      
		//
		//   When we don't have a semaphore we increment
		//   a counter.
		//
		AtomicAdd( & Signals,Count );

		//
		//   We need to be sure that there is no race
		//   condition where a semaphore is created at
		//   the same time it is signaled.  If so we
		//   update the semaphore.
		//
		if ( Semaphore != NULL )
			{ UpdateSemaphore(); }
		}
	else
		{
		//
		//   When we have an existing semaphore things
		//   are easy.  We just update it.
		//
		if ( ! ReleaseSemaphore( Semaphore,Count,NULL ) )
			{ Failure( "Wakeup failed in Signal()" ); }
		}
    }

    /********************************************************************/
    /*                                                                  */
    /*   Wait on a semaphore.                                           */
    /*                                                                  */
    /*   We sleep on a semaphore until a signal allows us to proceed.   */
    /*   We go to some trouble to deal with the interesting case where  */
    /*   the OS will not allow us to create an OS semaphore.            */
    /*                                                                  */
    /********************************************************************/

BOOLEAN SEMAPHORE::Wait( SBIT32 Timeout )
    {
	REGISTER BOOLEAN Result = False;

	//
	//   We keep a count of threads waiting on the 
	//   semaphore.
	//
	AtomicIncrement( & Waiting );

	//
	//   When there is no existing semaphore we need
	//   to create one or just sleep for a while and
	//   try again later.
	//
	while ( Semaphore == NULL )
		{
		REGISTER SBIT32 Original = Signals;

		//
		//   Great, it looks like the semaphore has
		//   already been signalled.
		//
		if ( Original > 0 )
			{
			REGISTER SBIT32 Update = (Original - 1);

			//
			//   Lets try to atomically decrement the 
			//   signal count.
			//
			if 
					( 
					AtomicCompareExchange( & Signals,Update,Original ) 
						== 
					Original 
					)
				{
				//
				//   Great, we have actually done it.  We
				//   can safely exit as we have matched a
				//   wait and a signal.
				//
				AtomicDecrement( & Waiting );

            //
            //    Import barrier
            //
            MemoryBarrier();         
            
				return True; 
				}
			}

		//
		//   Lets see if we can create a semaphore so we
		//   can sleep on it.
		//
		if ( ! UpdateSemaphore() )
			{ Sleep( 1 ); }
		}

	//
	//   The threads sleep here waiting to be signalled 
	//   so they can be restarted.
	//
	if ( Semaphore != NULL )
		{
		REGISTER DWORD Status = WaitForSingleObject( Semaphore,Timeout );

		//
		//   A wait can return various status values.  We
		//   just ensure we get one we like.
		//
		switch ( Status )
			{
			case WAIT_OBJECT_0:
				{ 
				Result = True;
				break;
				}

			case WAIT_TIMEOUT:
				{ 
				Result = False;
				break;
				}

			default:
				{ Failure( "Wait status in Wait" ); }
			}
		}

	//
	//   Finally, we decrement the count of waiting threads.
	//
	AtomicDecrement( & Waiting );
   
   //
   //    Import barrier
   //
   MemoryBarrier(); 

	return Result;
    }

    /********************************************************************/
    /*                                                                  */
    /*   Update the semaphore.                                          */
    /*                                                                  */
    /*   When we sleep we may decide that we need to create a new       */
    /*   semaphore.  We try to achieve that if we can otherwise we      */
    /*   have to pospone it until later.                                */
    /*                                                                  */
    /********************************************************************/

BOOLEAN SEMAPHORE::UpdateSemaphore( VOID )
    {
	//
	//   We need to verify that we don't already have a
	//   semaphore and still need to create one.
	//
	if ( Semaphore == NULL )
		{
		REGISTER HANDLE Handle = CreateSemaphore( NULL,0,MaxThreads,NULL );

		//
		//   We need to ensure that we managed to create
		//   a semaphore.  If not we just exit.
		//
		if ( Handle != NULL )
			{
			REGISTER VOLATILE VOID **Address = ((VOLATILE VOID**) & Semaphore);

			//
			//   There is a clear race condition here in
			//   that another thread may manage to create
			//   a new semaphore before we do.  If so we 
			//   delete our semaphore.
			//
			if ( AtomicCompareExchangePointer( Address,Handle,NULL ) != NULL )
				{ CloseHandle( Handle ); }
			}
		}

	//
	//   When a semaphore is created we need to update it
	//   with the outstanding signals.
	//
	if ( Semaphore != NULL )
		{
		REGISTER SBIT32 Count = AtomicExchange( & Signals,0 );

		//
		//   We only need to update the semaphore if the
		//   signal count is greater than zero.
		//
		if ( Count > 0 )
			{
			//
			//   Update the semaphore.
			//
			if ( ! ReleaseSemaphore( Semaphore,Count,NULL ) )
				{ Failure( "Wakeup failed in UpdateSemaphore()" ); }
			}

		return True; 
		}
	else
		{ return False; }
    }

    /********************************************************************/
    /*                                                                  */
    /*   Class destructor.                                              */
    /*                                                                  */
    /*   Destory a semaphore.  This call is not thread safe and should  */
    /*   only be made in a single thread environment.                   */
    /*                                                                  */
    /********************************************************************/

SEMAPHORE::~SEMAPHORE( VOID )
    {
	//
	//   When we create a semaphore it is necessary to
	//   delete it.
	//
	if ( Semaphore != NULL )
		{
		//
		//   We just verify that we were able to delete the
		//   semaphore.
		//
		if ( ! CloseHandle( Semaphore ) )
			{ Failure( "Close semaphore in destructor for SEMAPHORE" ); }
		}
    }
#endif