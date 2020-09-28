                          
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

#include "Spinlock.hpp"

#if 0
    /********************************************************************/
    /*                                                                  */
    /*   Class constructor.                                             */
    /*                                                                  */
    /*   Create a new lock and initialize it.  This call is not         */
    /*   thread safe and should only be made in a single thread         */
    /*   environment.                                                   */
    /*                                                                  */
    /********************************************************************/

SPINLOCK::SPINLOCK( SBIT32 NewMaxSpins,SBIT32 NewMaxThreads ) :
 		//
		//   Call the constructors for the contained classes.
		//
		WaitSemaphore( NewMaxThreads )
   {
	//
	//   Set the initial state.
	//
	MaxSpins = NewMaxSpins;
#ifdef ENABLE_RECURSIVE_LOCKS
	Owner = NULL;
	Recursive = 0;
#endif
    Spinlock = LockOpen;
    Waiting = 0;
#ifdef ENABLE_LOCK_STATISTICS

	//
	//   Zero the lock statistics.
	//
    TotalLocks = 0;
    TotalSleeps = 0;
    TotalSpins = 0;
    TotalTimeouts = 0;
    TotalWaits = 0;
#endif
    }

    /********************************************************************/
    /*                                                                  */
    /*   Wait for the spinlock.                                         */
    /*                                                                  */
    /*   Wait for the spinlock to become free and then claim it.        */
    /*                                                                  */
    /********************************************************************/

BOOLEAN SPINLOCK::WaitForLock( SBIT32 Sleep )
    {
	REGISTER LONG Cpus = ((LONG) NumberOfCpus());
#ifdef ENABLE_LOCK_STATISTICS
    REGISTER SBIT32 Sleeps = 0;
    REGISTER SBIT32 Spins = 0;
    REGISTER SBIT32 Waits = 0;

#endif
    do
        {
        REGISTER SBIT32 Count;
        
		//
		//   If there are already more threads waiting 
		//   than the number of CPUs then the odds of 
		//   getting the lock by spinning are slim, when 
		//   there is only one CPU the chance is zero, so 
		//   just bypass this step.
		//
		if ( (Cpus > 1) && (Cpus > Waiting) )
			{
			//
			//   Wait by spinning and repeatedly testing the
			//   spinlock.  We exit when the lock becomes free 
			//   or the spin limit is exceeded.
			//
			for 
					( 
					Count = MaxSpins;
					(Count > 0) && (Spinlock != LockOpen);
					Count -- 
					)
				{
				//
				//   When we are using Jackson MP machines
				//   we need to pause to give the other
				//   virtual processors a chance.
				//
				Pause();
				}
#ifdef ENABLE_LOCK_STATISTICS

			//
			//   Update the statistics.
			//
			Spins += (MaxSpins - Count);
			Waits ++;
#endif
			}
		else
			{ Count = 0; }

		//
		//   We have exhusted our spin count so it is time to
		//   sleep waiting for the lock to clear.
		//
        if ( Count == 0 )
            {
			//
			//   The lock is still closed so lets go to sleep on 
			//   a semaphore.  However, we must first increment
			//   the waiting count and test the lock one last time
			//   to make sure it is still busy and there is someone
			//   to wake us up later.
			//
			(VOID) AtomicIncrement( & Waiting );

			if ( ! ClaimSpinlock( & Spinlock ) )
				{
				//
				//   Sleep on the semaphore until we are awoken
				//   by the lock holder.
				//
				if ( ! WaitSemaphore.Wait( Sleep ) )
					{
#ifdef ENABLE_LOCK_STATISTICS
					//
					//   Count the number of times we have  
					//   timed out on this lock.
					//
					(VOID) AtomicIncrement( & TotalTimeouts );

#endif
					return False; 
					}
#ifdef ENABLE_LOCK_STATISTICS

				//
				//   Update the statistics.
				//
				Sleeps ++;
#endif
				}
			else
				{
				//
				//   Lucky - got the lock on the last attempt.
				//   Hence, lets decrement the sleep count and
				//   exit.
				// 
				(VOID) AtomicDecrement( & Waiting );
            
				break; 
				}
            }
        }
    while ( ! ClaimSpinlock( & Spinlock ) );
#ifdef ENABLE_LOCK_STATISTICS

	//
	//   Update the statistics.
	//
    TotalSleeps += Sleeps;
    TotalSpins += Spins;
    TotalWaits += Waits;
#endif

    return True;
    }

    /********************************************************************/
    /*                                                                  */
    /*   Wake all sleepers.                                             */
    /*                                                                  */
    /*   Wake all the sleepers who are waiting for the spinlock.        */
    /*   All sleepers are woken because this is much more efficent      */
    /*   and it is known that the lock latency is short.                */
    /*                                                                  */
    /********************************************************************/

VOID SPINLOCK::WakeAllSleepers( VOID )
    {
    REGISTER SBIT32 Wakeup = AtomicExchange( & Waiting, 0 );

	//
	//   We make sure there is still someone to be woken 
	//   up if not we check that the count has not become
	//   negative.
	//
    if ( Wakeup > 0 )
        {
		REGISTER SBIT32 Cpus = ((LONG) NumberOfCpus());

		//
		//   We will only wake enough threads to ensure that 
		//   there is one active thread per CPU.  So if an 
		//   application has hundreds of threads we will try 
		//   prevent the system from becoming swampped.
		//
		if ( Wakeup > Cpus )
			{
			(VOID) AtomicAdd( & Waiting,(Wakeup - Cpus) );
			Wakeup = Cpus; 
			}

        //
        //   Wake some sleepers as the lock has just been freed.
        //   It is a straight race to decide who gets the lock next.
        //
		WaitSemaphore.Signal( Wakeup );
#ifdef ENABLE_DYNAMIC_SEMAPHORES

		//
		//   Now for a stunning trick.  We want to reclaim the
		//   semaphore it is it not in use.  However, it could be
		//   reused at any point.  So here we do a double exchange
		//   and we reclaim the semaphore only it is it free.
		//
		if ( Waiting == 0 )
			{
			AUTO DOUBLE_SBIT32 Original;
			AUTO DOUBLE_SBIT32 Update;

			//
			//   Clone the current values of the waiting count
			//   and the semaphore handle.
			//
			(*((SBIT64*) & Original)) = (*((SBIT64*) & Waiting));
			(*((SBIT64*) & Update)) = (*((SBIT64*) & Original));

			//
			//   Update the current values.  We overwrite the 
			//   original value of the waiting count because
			//   it must be zero for this to work.  The new values
			//   will be zero as well.
			//
			Original.First = 0;

			Update.First = 0;
			Update.Second = 0;

			//
			//   So it is time to hold onto you seats.  We now
			//   atomically swap the waiting cound and the semaphore
			//   pointer only if the waiting count is zero.
			if
					( 
					AtomicCompareExchange64
						( 
						((SBIT64*) & Original),
						(*((SBIT64*) & Update)),
						(*((SBIT64*) & Original))
						) 
							!= 
					(*((SBIT64*) & Original))
					)
				{
				//
				//   It looks like it actually worked so we can
				//   release the semaphore
				//
				//   ******** Bug here ********
				//   I can't release the semaphore here because the
				//   waiting count might include a thread that has
				//   added itself to the waiting count but got the
				//   lock just before it went to sleep.  If I release
				//   the semaphore it will have 1 too many signals
				//   and later a thread on this lok will not have
				//   enough signals.
				//
				((WAIT*) Original.First) -> ReleaseSemaphore();
				}
			}
#endif
        }
    else
        {
        //
        //   When multiple threads pass through the critical  
        //   section it is possible for the 'Waiting' count  
		//   to become negative.  This should be very rare but 
		//   such a negative value needs to be preserved. 
        //
		if ( Wakeup < 0 )
			{ (VOID) AtomicAdd( & Waiting, Wakeup ); }
        }
    }

    /********************************************************************/
    /*                                                                  */
    /*   Class destructor.                                              */
    /*                                                                  */
    /*   Destory a lock.  This call is not thread safe and should       */
    /*   only be made in a single thread environment.                   */
    /*                                                                  */
    /********************************************************************/

SPINLOCK::~SPINLOCK( VOID )
    {
#ifdef ENABLE_LOCK_STATISTICS
	//
	//   Print the lock statistics.
	//
	DebugPrint
		(
		"Spinlock: %d locks, %d timeouts, %d locks per wait, "
		"%d spins per wait, %d waits per sleep.\n",
		TotalLocks,
		TotalTimeouts,
		(TotalLocks / ((TotalWaits <= 0) ? 1 : TotalWaits)),
		(TotalSpins / ((TotalWaits <= 0) ? 1 : TotalWaits)),
		(TotalWaits / ((TotalSleeps <= 0) ? 1 : TotalSleeps))
		);

#endif
    }
#endif
