                          
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

#include "Sharelock.hpp"

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
SHARELOCK::SHARELOCK
		( 
		SBIT32						  NewMaxSpins,
		SBIT32						  NewMaxThreads,
		SBIT32						  NewMaxUsers 
		) :
		//
		//   Call the constructors for the contained classes.
		//
		NormalSemaphore( NewMaxThreads ),
		PrioritySemaphore( NewMaxThreads )
    {
#if defined(_XENON) || defined(_XBOX)
		assert(!((int)(&ExclusiveUsers)%8) && "Must ensure allignment for Interlocked* operations");
#endif
	//
	//   Set the initial state.
	//
	ExclusiveUsers = 0;
	TotalUsers = 0;

    NormalWaiting = 0;
    PriorityWaiting = 0;

#ifdef ENABLE_RECURSIVE_LOCKS
	Owner = NULL;
	Recursive = 0;

#endif
	//
	//   Check the configurable values.
	//
	if ( NewMaxSpins > 0 )
		{ MaxSpins = NewMaxSpins; }
	else
		{ Failure( "Maximum spins invalid in constructor for SHARELOCK" ); }

	if ( (NewMaxUsers > 0) && (NewMaxUsers <= NormalSemaphore.GetMaxThreads()) )
		{ MaxUsers = NewMaxUsers; }
	else
		{ Failure( "Maximum share invalid in constructor for SHARELOCK" ); }
#ifdef ENABLE_LOCK_STATISTICS

	//
	//   Zero the statistics.
	//
    TotalExclusiveLocks = 0;
    TotalShareLocks = 0;
    TotalSleeps = 0;
    TotalSpins = 0;
    TotalTimeouts = 0;
    TotalWaits = 0;
#endif
    }

    /********************************************************************/
    /*                                                                  */
    /*   Sleep waiting for the lock.                                    */
    /*                                                                  */
    /*   We have decided it is time to sleep waiting for the lock       */
    /*   to become free.                                                */
    /*                                                                  */
    /********************************************************************/

BOOLEAN SHARELOCK::SleepWaitingForLock
		( 
		SEMAPHORE					  *Semaphore,
		SBIT32						  Sleep,
		VOLATILE SBIT32				  *Waiting 
		)
    {
	//
	//   We have been spinning waiting for the lock but it
	//   has not become free.  Hence, it is now time to 
	//   give up and sleep for a while.
	//
	(VOID) AtomicIncrement( Waiting );

	MemoryBarrier();
	
	//
	//   Just before we go to sleep we do one final check
	//   to make sure that the lock is still busy and that
	//   there is someone to wake us up when it becomes 
	//   free.
	//
	if ( TotalUsers > 0 )
		{
#ifdef ENABLE_LOCK_STATISTICS
		//
		//   Count the number of times we have slept on 
		//   this lock.
		//
		(VOID) AtomicIncrement( & TotalSleeps );

#endif
		//
		//   When we sleep we awoken when the lock  
		//   becomes free or when we timeout.  If we  
		//   timeout we simply exit after decrementing 
		//   various counters.
		//
		if ( ! Semaphore -> Wait( Sleep ) )
			{ 
#ifdef ENABLE_LOCK_STATISTICS
			//
			//   Count the number of times we have timed  
			//   out on this lock.
			//
			(VOID) AtomicIncrement( & TotalTimeouts );

#endif
			return False; 
			}
		}
	else
		{
		//
		//   Lucky - the lock was just freed so lets
		//   decrement the sleep count and exit without
		//   sleeping.
		// 
		(VOID) AtomicDecrement( Waiting );
		}
		
		MemoryBarrier();
	
	return True;
    }

    /********************************************************************/
    /*                                                                  */
    /*   Update the spin limit.                                         */
    /*                                                                  */
    /*   Update the maximum number of spins while waiting for the lock. */
    /*                                                                  */
    /********************************************************************/

BOOLEAN SHARELOCK::UpdateMaxSpins( SBIT32 NewMaxSpins )
    {
	if ( NewMaxSpins > 0 )
		{ 
		MaxSpins = NewMaxSpins; 

		return True;
		}
	else
		{ return False; }
	}

    /********************************************************************/
    /*                                                                  */
    /*   Update the sharing limit.                                      */
    /*                                                                  */
    /*   Update the maximum number of users that can share the lock.    */
    /*                                                                  */
    /********************************************************************/

BOOLEAN SHARELOCK::UpdateMaxUsers( SBIT32 NewMaxUsers )
    {
	//
	//   We need to verify the new value makes sense.
	//
	if ( (NewMaxUsers > 0) && (NewMaxUsers <= NormalSemaphore.GetMaxThreads()) )
		{
		//
		//   We claim an exclusive lock so we know
		//   we can update the control information
		//   atomically.
		//
		ClaimExclusiveLock();

		//
		//   Update the maximum number of users.
		//
		MaxUsers = NewMaxUsers;
		
		//
		//   Release any lock we claimed earlier.
		//
		ReleaseExclusiveLock();

		return True;
		}
	else
		{ return False; }
	}

    /********************************************************************/
    /*                                                                  */
    /*   Wait for an exclusive lock.                                    */
    /*                                                                  */
    /*   Wait for the spinlock to become free and then claim it.        */
    /*                                                                  */
    /********************************************************************/

BOOLEAN SHARELOCK::WaitForExclusiveLock( SBIT32 Sleep )
    {
	REGISTER LONG Cpus = ((LONG) NumberOfCpus());
#ifdef ENABLE_LOCK_STATISTICS
	REGISTER SBIT32 Spins = 0;
	REGISTER SBIT32 Waits = 0;

#endif
   
   MemoryBarrier();
   
	//
	//   We will loop round in this function until the
	//   following condition becomes false.
	//
	while ( TotalUsers != 1 )
		{
		//
		//   The lock is busy so release it and spin waiting
		//   for it to become free.
		//
		(VOID) AtomicDecrement( & TotalUsers );
    
		//
		//   We will only try spinning and sleeping if we
		//   are permitted to do so by the parameters.
		//   
		if ( Sleep != 0 )
			{
			REGISTER SBIT32 Count;
    
			//
			//   If there are already more threads waiting 
			//   than the number of CPUs then the odds of 
			//   getting the lock by spinning are slim, when 
			//   there is only one CPU the chance is zero, so 
			//   just bypass this step.
			//
			if ( (Cpus > 1) && (Cpus > PriorityWaiting) )
				{
				//
				//   Wait by spinning and repeatedly testing 
				//   the spinlock.  We exit when the lock  
				//   becomes free or the spin limit is exceeded.
				//
				for 
						( 
						Count = MaxSpins;
						(Count > 0) && (TotalUsers > 0);
						Count -- 
						)
					{
					//
					//   When we are using Jackson MP machines
					//   we need to pause to give the other
					//   virtual processors a chance.
					//
					Pause();
					MemoryBarrier();
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
			//   We have exhusted our spin count so it is 
			//   time to sleep waiting for the lock to clear.
			//
			if ( Count == 0 )
				{
				//
				//   We have decide that we need to sleep but are
				//   still holding an exclusive lock so lets drop it
				//   before sleeping.
				//
				(VOID) AtomicDecrement( & ExclusiveUsers );

				//
				//   We have decied that it is time to go to sleep
				//   when we wake up the lock should be available
				//   (or just aquired) unless we have timed out in
				//   wich case we exit.
				//
				if 
						( 
						! SleepWaitingForLock
							( 
							& PrioritySemaphore,
							Sleep,
							& PriorityWaiting 
							) 
						)
					{ return False; }

				//
				//   We have woken up again so lets reclaim the
				//   exclusive lock we had earlier.
				//
				(VOID) AtomicIncrement( & ExclusiveUsers );
				}
			}
		else
			{ 
			//
			//   We have decide that we need to exit but are 
			//   still holding an exclusive lock.  so lets drop 
			//   it and leave.
			//
			(VOID) AtomicDecrement( & ExclusiveUsers );

			return False; 
			} 
		//
		//   Lets test the lock again.
		//
		(VOID) AtomicIncrement( & TotalUsers );
		
		MemoryBarrier();
		
		}

#ifdef ENABLE_LOCK_STATISTICS

	//
	//   Update the statistics.
	//
	(VOID) AtomicAdd( & TotalSpins, Spins );
	(VOID) AtomicAdd( & TotalWaits, Waits );
#endif

   MemoryBarrier();

	return True;
    }

    /********************************************************************/
    /*                                                                  */
    /*   Wait for a shared lock.                                        */
    /*                                                                  */
    /*   Wait for the lock to become free and then claim it.            */
    /*                                                                  */
    /********************************************************************/

BOOLEAN SHARELOCK::WaitForShareLock( SBIT32 Sleep )
    {
	REGISTER LONG Cpus = ((LONG) NumberOfCpus());
#ifdef ENABLE_LOCK_STATISTICS
	REGISTER SBIT32 Spins = 0;
	REGISTER SBIT32 Waits = 0;

#endif
   MemoryBarrier();
   
	//
	//   We will loop round in this function until the
	//   following condition becomes false.
	//
	while ( (ExclusiveUsers > 0) || (TotalUsers > MaxUsers) )
		{
		//
		//   The lock is busy so release it and spin waiting
		//   for it to become free.
		//
		(VOID) AtomicDecrement( & TotalUsers );

		//
		//   We will only try spinning and sleeping if we
		//   are permitted to do so by the parameters.
		//   
		if ( Sleep != 0 )
			{
			REGISTER SBIT32 Count;
    
			//
			//   If there are already more threads waiting 
			//   than the number of CPUs then the odds of 
			//   getting the lock by spinning are slim, when 
			//   there is only one CPU the chance is zero, so 
			//   just bypass this step.
			//
			if ( (Cpus > 1) && (Cpus > (PriorityWaiting + NormalWaiting)) )
				{
				//
				//   Wait by spinning and repeatedly testing 
				//   the spinlock.  We exit when the lock  
				//   becomes free or the spin limit is exceeded.
				//
				for 
						( 
						Count = MaxSpins;
						(Count > 0) 
							&& 
						((ExclusiveUsers > 0) || (TotalUsers >= MaxUsers));
						Count -- 
						)
					{
					//
					//   When we are using Jackson MP machines
					//   we need to pause to give the other
					//   virtual processors a chance.
					//
					Pause();
					MemoryBarrier();
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
			//   We have exhusted our spin count so it is 
			//   time to sleep waiting for the lock to clear.
			//
			if ( Count == 0 )
				{
				//
				//   We have decied that it is time to go to sleep
				//   when we wake up the lock should be available
				//   (or just aquired) unless we have timed out in
				//   wich case we exit.
				//
				if 
						( 
						! SleepWaitingForLock
							( 
							& NormalSemaphore,
							Sleep,
							& NormalWaiting 
							) 
						)
					{ return False; }
				}
			}
		else
			{ return False; }

		//
		//   Lets test the lock again.
		//
		(VOID) AtomicIncrement( & TotalUsers );
		
		MemoryBarrier();
		}
#ifdef ENABLE_LOCK_STATISTICS

	//
	//   Update the statistics.
	//
	(VOID) AtomicAdd( & TotalSpins, Spins );
	(VOID) AtomicAdd( & TotalWaits, Waits );
#endif

   MemoryBarrier();

	return True;
    }

    /********************************************************************/
    /*                                                                  */
    /*   Wake all sleepers.                                             */
    /*                                                                  */
    /*   Wake all the sleepers who are waiting for the spinlock.        */
    /*   All sleepers are woken because this is much more efficent      */
    /*   and it is known that the lock latency is typically short.      */
    /*                                                                  */
    /********************************************************************/

VOID SHARELOCK::WakeAllSleepers( VOID )
    {
    REGISTER SBIT32 PriorityWakeup = AtomicExchange( & PriorityWaiting, 0 );

	//
	//   We try to wake any priority sleepers before
	//   waking any normal sleepers.
	//
    if ( PriorityWakeup > 0 )
        {
		REGISTER SBIT32 Cpus = ((LONG) NumberOfCpus());

		//
		//   We will only wake enough threads to ensure that 
		//   there is one active thread per CPU.  So if an 
		//   application has hundreds of threads we will try 
		//   prevent the system from becoming swampped.
		//
		if ( PriorityWakeup > Cpus )
			{
			(VOID) AtomicAdd( & PriorityWaiting,(PriorityWakeup - Cpus) );
			PriorityWakeup = Cpus; 
			}

        //
        //   Wake some sleepers as the lock has just been freed.
        //   It is a straight race to decide who gets the lock next.
        //
		PrioritySemaphore.Signal( PriorityWakeup );
        }
    else
        {
		REGISTER SBIT32 NormalWakeup = AtomicExchange( & NormalWaiting, 0 );

		//
		//   Well as there are no priority sleepers lets  
		//   wake some of the normal sleepers.
		//
		if ( NormalWakeup > 0 )
			{
			REGISTER SBIT32 Cpus = ((LONG) NumberOfCpus());

			//
			//   We will only wake enough threads to ensure that 
			//   there is one active thread per CPU.  So if an 
			//   application has hundreds of threads we will try 
			//   prevent the system from becoming swampped.
			//
			if ( NormalWakeup > Cpus )
				{
				(VOID) AtomicAdd( & NormalWaiting,(NormalWakeup - Cpus) );
				NormalWakeup = Cpus; 
				}

			//
			//   Wake some sleepers as the lock has just been freed.
			//   It is a straight race to decide who gets the lock next.
			//
			NormalSemaphore.Signal( NormalWakeup );
			}
		else
			{
			//
			//   When multiple threads pass through the critical  
			//   section it is possible for the waiting count  
			//   to become negative.  This should be very rare but 
			//   such a negative value needs to be preserved. 
			//
			if ( NormalWakeup < 0 )
				{ (VOID) AtomicAdd( & NormalWaiting, NormalWakeup ); }
			}

        //
        //   When multiple threads pass through the critical  
        //   section it is possible for the waiting count  
		//   to become negative.  This should be very rare but 
		//   such a negative value needs to be preserved. 
        //
		if ( PriorityWakeup < 0 )
			{ (VOID) AtomicAdd( & PriorityWaiting, PriorityWakeup ); }
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

SHARELOCK::~SHARELOCK( VOID )
    {
#ifdef ENABLE_LOCK_STATISTICS
	//
	//   Print the lock statistics.
	//
	DebugPrint
		(
		"Sharelock: %d exclusive, %d shared, %d timeouts, " 
		"%d locks per wait, %d spins per wait, %d waits per sleep.\n",
		TotalExclusiveLocks,
		TotalShareLocks,
		TotalTimeouts,
		((TotalExclusiveLocks + TotalShareLocks) / ((TotalWaits <= 0) ? 1 : TotalWaits)),
		(TotalSpins / ((TotalWaits <= 0) ? 1 : TotalWaits)),
		(TotalWaits / ((TotalSleeps <= 0) ? 1 : TotalSleeps))
		);
#endif
    }

#endif