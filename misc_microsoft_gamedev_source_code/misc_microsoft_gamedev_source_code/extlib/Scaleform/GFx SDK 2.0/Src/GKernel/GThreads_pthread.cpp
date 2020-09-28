/**********************************************************************

Filename    :   GThreads_sys.cpp
Content     :   Windows specific thread-related (safe) functionality
Created     :   May 5, 2003
Authors     :   Michael Antonov

Copyright   :   (c) 2003 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GThreads.h"

#ifndef GFC_NO_THREADSUPPORT

#include <pthread.h>
#include <time.h>

#ifdef GFC_OS_PS3
#include <sys/timer.h>
#define sleep(x) sys_timer_sleep(x)
#define usleep(x) sys_timer_usleep(x)
#else
#include <unistd.h>
#endif

// ***** Classes Implemented

class GMutex;
class GWaitCondition;


// ***** Mutex implementation

// Interface used internally in a mutex
class GMutex_AreadyLockedAcquireInterface : public GAcquireInterface
{
public:
    // Mutex we belong to
    GMutex *pMutex;

    GMutex_AreadyLockedAcquireInterface()
    {
        pMutex = 0;
    }

    // Acquire interface implementation
    virtual bool    CanAcquire();
    virtual bool    TryAcquire();
    virtual bool    TryAcquireCommit();
    virtual bool    TryAcquireCancel();

    // GInterface - no implementation
    virtual void        AddRef()                            { }
    virtual void        Release(UInt flags=0)               { }
    virtual bool        SetRefCountMode(UInt RefCountFlags) { return 1; }
};


// Acquire interface implementation
bool    GMutex_AreadyLockedAcquireInterface::CanAcquire()       { return 1; }
bool    GMutex_AreadyLockedAcquireInterface::TryAcquire()       { return pMutex->TryAcquire(); }
bool    GMutex_AreadyLockedAcquireInterface::TryAcquireCommit() { return pMutex->TryAcquireCommit(); }
bool    GMutex_AreadyLockedAcquireInterface::TryAcquireCancel() { return pMutex->TryAcquireCancel(); }


// *** Internal GMutex implementation structure

class GMutexImpl : public GNewOverrideBase
{
    // System mutex or semaphore
    pthread_mutex_t   Mutex;
    bool          Recursive;
    UInt          LockCount;

    GMutex_AreadyLockedAcquireInterface AreadyLockedAcquire;
    
    friend class GWaitConditionImpl;

public:
    // Constructor/destructor
    GMutexImpl(GMutex* pmutex, bool recursive = 1);
    ~GMutexImpl();

    // Locking functions
    void                Lock();
    bool                TryLock();
    void                Unlock(GMutex* pmutex);
    // Returns 1 if the mutes is currently locked
    bool                IsLockedByAnotherThread(GMutex* pmutex);        
    bool                IsSignaled() const;
    GAcquireInterface*  GetAcquireInterface(GMutex* pmutex);
};



// *** Constructor/destructor
GMutexImpl::GMutexImpl(GMutex* pmutex, bool recursive)
{   
    AreadyLockedAcquire.pMutex  = pmutex;
    Recursive           = recursive;
    LockCount           = 0;

    if (Recursive)
    {
    pthread_mutexattr_t mattr;

    pthread_mutexattr_init(&mattr);
    pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(&Mutex, &mattr);
    pthread_mutexattr_destroy(&mattr);
    }
    else
    pthread_mutex_init(&Mutex, 0);
}

GMutexImpl::~GMutexImpl()
{
    pthread_mutex_destroy(&Mutex);
}


// Lock and try lock
void GMutexImpl::Lock()
{
    if (!pthread_mutex_lock(&Mutex))
    LockCount++;
}

bool GMutexImpl::TryLock()
{
    if (!pthread_mutex_lock(&Mutex))
    {
    LockCount++;
    return 1;
    }
    else
    return 0;
}

void GMutexImpl::Unlock(GMutex* pmutex)
{
    UInt lockCount;
    LockCount--;
    lockCount = LockCount;

    // Release and Notify waitable objects
    pthread_mutex_unlock(&Mutex);

    if (lockCount == 0)
    pmutex->CallWaitHandlers();
}

bool    GMutexImpl::IsLockedByAnotherThread(GMutex* pmutex)
{
    // There could be multiple interpretations of IsLocked with respect to current thread
    if (LockCount == 0)
        return 0;
    if (!TryLock())
        return 1;
    Unlock(pmutex);
    return 0;
}

bool    GMutexImpl::IsSignaled() const
{
    // An mutex is signaled if it is not locked ANYWHERE
    // Note that this is different from IsLockedByAnotherThread function, that takes current thread into account
    return LockCount == 0;
}

// Obtain the acquisition interface
GAcquireInterface*  GMutexImpl::GetAcquireInterface(GMutex* pmutex)
{
    // If the mutex is already locked by us, return 'owned' acquire interface
    if (LockCount && !IsLockedByAnotherThread(pmutex))
        return &AreadyLockedAcquire;
    // Otherwise, just return pointer to us
    return pmutex;
}



// *** Actual GMutex class implementation

GMutex::GMutex(bool recursive)
{
    SetRefCountMode(GFC_REFCOUNT_THREADSAFE);

    pImpl = new GMutexImpl(this, recursive);
}
GMutex::~GMutex()
{
    delete pImpl;
}

// Lock and try lock
void GMutex::Lock()
{
    pImpl->Lock();
}
bool GMutex::TryLock()
{
    return pImpl->TryLock();
}
void GMutex::Unlock()
{
    pImpl->Unlock(this);
}
bool    GMutex::IsLockedByAnotherThread()
{
    return pImpl->IsLockedByAnotherThread(this);
}
bool    GMutex::IsSignaled() const
{
    return pImpl->IsSignaled();
}
// Obtain the acquisition interface
GAcquireInterface*  GMutex::GetAcquireInterface()
{
    return pImpl->GetAcquireInterface(this);
}

// Acquire interface implementation
bool    GMutex::CanAcquire()
{
    return !IsLockedByAnotherThread();
}
bool    GMutex::TryAcquire()
{
    return TryLock();
}
bool    GMutex::TryAcquireCommit()
{
    // Nothing.
    return 1;
}
bool    GMutex::TryAcquireCancel()
{
    Unlock();
    return 1;
}


// ***** Wait Condition Implementation

// Internal implementation class
class GWaitConditionImpl : public GNewOverrideBase
{
    pthread_mutex_t     Mutex;
    pthread_cond_t      Condv;

public:

    // Constructor/destructor
    GWaitConditionImpl();
    ~GWaitConditionImpl();

    // Wait on a condition until it is notified
    bool    Wait(UInt delay);
    // Release mutex and wait for condition. The mutex is re-aqured after the wait.
    bool    Wait(GMutex *pmutex, UInt delay = GFC_WAIT_INFINITE);

    // Notify a condition, releasing at one object waiting
    void    Notify();
    // Notify a condition, releasing all objects waiting
    void    NotifyAll();
};


GWaitConditionImpl::GWaitConditionImpl()
{
    pthread_mutex_init(&Mutex, 0);
    pthread_cond_init(&Condv, 0);
}

GWaitConditionImpl::~GWaitConditionImpl()
{
    pthread_mutex_destroy(&Mutex);
    pthread_cond_destroy(&Condv);
}
    
// Wait without a mutex
bool    GWaitConditionImpl::Wait(UInt delay)
{
    bool result;

    pthread_mutex_lock(&Mutex);

    if (delay == GFC_WAIT_INFINITE)
    result = (0 == pthread_cond_wait(&Condv, &Mutex));
    else
    {
    std::timespec ts;
    ts.tv_sec = delay;
    ts.tv_nsec = 0;
    result = (0 == pthread_cond_timedwait(&Condv, &Mutex, &ts));
    }

    pthread_mutex_unlock(&Mutex);
    return result;
}

bool    GWaitConditionImpl::Wait(GMutex *pmutex, UInt delay)
{
    bool            result = 1;
    UInt            lockCount = pmutex->pImpl->LockCount;

    // Mutex must have been locked
    if (lockCount == 0)
    return 0;

    pthread_mutex_lock(&Mutex);

    // Finally, release a mutex or semaphore
    if (pmutex->pImpl->Recursive)
    {
    // Release the recursive mutex N times
    pmutex->pImpl->LockCount = 0;
    for(int i=0; i<lockCount; i++)
        pthread_mutex_unlock(&pmutex->pImpl->Mutex);
    pmutex->CallWaitHandlers();
    }
    else
    {
    pmutex->pImpl->LockCount = 0;
    pthread_mutex_unlock(&pmutex->pImpl->Mutex);
    pmutex->CallWaitHandlers();
    }

    // Note that there is a gap here between mutex.Unlock() and Wait(). The other mutex protects this gap.

    if (delay == GFC_WAIT_INFINITE)
    pthread_cond_wait(&Condv,&Mutex);
    else
    {
    std::timespec ts;
    ts.tv_sec = delay;
    ts.tv_nsec = 0;

    if (pthread_cond_timedwait(&Condv,&Mutex, &ts))
        result = 0;
    }

    pthread_mutex_unlock(&Mutex);

    // Re-aquire the mutex
    for(int i=0; i<lockCount; i++)
    pmutex->Lock(); 

    // Return the result
    return result;
}

// Notify a condition, releasing the least object in a queue
void    GWaitConditionImpl::Notify()
{
    pthread_cond_signal(&Condv);
}

// Notify a condition, releasing all objects waiting
void    GWaitConditionImpl::NotifyAll()
{
    pthread_cond_broadcast(&Condv);
}



// *** Actual implementation of GWaitCondition

GWaitCondition::GWaitCondition()
{
    pImpl = new GWaitConditionImpl;
}
GWaitCondition::~GWaitCondition()
{
    delete pImpl;
}
    
// Wait without a mutex
bool    GWaitCondition::Wait(UInt delay)
{
    return pImpl->Wait(delay);
}
bool    GWaitCondition::Wait(GMutex *pmutex, UInt delay)
{
    return pImpl->Wait(pmutex, delay);
}
// Notification
void    GWaitCondition::Notify()
{
    pImpl->Notify();
}
void    GWaitCondition::NotifyAll()
{
    pImpl->NotifyAll();
}


// ***** Current thread

// Per-thread variable
/*
static __thread GThread* pCurrentThread = 0;

// Static function to return a pointer to the current thread
void    GThread::InitCurrentThread(GThread *pthread)
{
    pCurrentThread = pthread;
}

// Static function to return a pointer to the current thread
GThread*    GThread::GetThread()
{
    return pCurrentThread;
}
*/


// *** Custom Ref-Counting

// For GThread we use a lock for reference counting, so that we
// can ensure a safe transition to thread Finished state occurs,
// informing waiters only after our thread reference is released.
// Such semantics ensure that no external references to finished
// GThread object exit once it has been signaled.

void GFASTCALL AddRef_ThreadLocked(GRefCountBaseImpl *pbase)
{
    GLock::Locker lock(&((GWaitable*)pbase)->GetHandlersLock());
    pbase->RefCount++;
}
void GFASTCALL Release_ThreadLocked(GRefCountBaseImpl *pbase, UInt flags)
{
    GUNUSED(flags);

    bool deleteFlag;

    { // Scoped lock for duration of RefCount access.
        GLock::Locker lock(&((GWaitable*)pbase)->GetHandlersLock());
        pbase->RefCount--;
        deleteFlag = (pbase->RefCount==0);
    }

    if (deleteFlag)
        delete pbase;
}

static GRefCountImpl RefCountImpl_ThreadLocked =
{
    &AddRef_ThreadLocked,
    &Release_ThreadLocked
};


// *** GThread constructors.

GThread::GThread()
    : GWaitable(&RefCountImpl_ThreadLocked)
{
    // Clear the variables    
    ThreadFlags		= 0;
    ThreadHandle	= 0;
    ExitCode		= 0;
    SuspendCount	= 0;

    // Clear Function pointers
    ThreadFunction	= 0;    
    UserHandle		= 0;
}

GThread::GThread(GThread::ThreadFn threadFunction, Handle userHandle,
                 GThread::ThreadState initialState)
    : GWaitable(&RefCountImpl_ThreadLocked)
{
    // Clear the variables
    ThreadFlags		= 0;
    ThreadHandle	= 0;
    ExitCode		= 0;
    SuspendCount	= 0;

    // Initialize function pointers.
    ThreadFunction	= threadFunction;    
    UserHandle		= userHandle;
    // Launch the thread, if asked.
    if (initialState != NotRunning)
        Start(initialState);
}

GThread::~GThread()
{
    // Thread should not running while object is being destroyed,
    // this would indicate ref-counting issue.
    //GASSERT(IsRunning() == 0);

    // Clean up thread.    
    CleanupSystemThread();
    ThreadHandle = 0;
}



// *** Overridable User functions.

// Default Run implementation
SInt	GThread::Run()
{
    // Call pointer to function, if available.    
    return (ThreadFunction) ? ThreadFunction(this, UserHandle) : 0;
}
void	GThread::OnExit()
{	
}


// Finishes the thread and releases internal reference to it.
void    GThread::FinishAndRelease()
{
    // Perform release during the lock so that waiting threads are guaranteed
    // that the thread has released it's own reference to GThread object by
    // the time their wait completes.
    GLock::Locker lock(&GetHandlersLock());

    // Note: thread must be US
    ThreadFlags &= (UInt32)~(GFC_THREAD_STARTED);
    ThreadFlags |= GFC_THREAD_FINISHED;

    CallWaitHandlers();
    Release();
}



// *** ThreadList - used to tack all created threads

class GThreadList : public GNewOverrideBase
{
    //------------------------------------------------------------------------
    struct ThreadHashOp
    {
        size_t operator()(const GThread* ptr)
        {
            return (((size_t)ptr) >> 6) ^ (size_t)ptr;
        }
    };

    GTL::ghash_set<GThread*, ThreadHashOp> ThreadSet;
    GMutex                                 ThreadMutex;
    GWaitCondition                         ThreadsEmpty;
    // Track the root thread that created us.
    DWORD                                  RootThreadId;

    static GThreadList* volatile pRunningThreads;

    void addThread(GThread *pthread)
    {
        GMutex::Locker lock(&ThreadMutex);
        ThreadSet.add(pthread);
    }

    void removeThread(GThread *pthread)
    {
        GMutex::Locker lock(&ThreadMutex);
        ThreadSet.remove(pthread);
        if (ThreadSet.size() == 0)
            ThreadsEmpty.Notify();
    }

    void finishAllThreads()
    {
        // Only original root thread can call this.
        GASSERT(GetCurrentThreadId() == RootThreadId);

        GMutex::Locker lock(&ThreadMutex);
        while (ThreadSet.size() != 0)
            ThreadsEmpty.Wait(&ThreadMutex);
    }

public:

    GThreadList()
    {
        RootThreadId = GetCurrentThreadId();
    }
    ~GThreadList() { }


    static void AddRunningThread(GThread *pthread)
    {
        // Non-atomic creation ok since only the root thread
        if (!pRunningThreads)
        {
            pRunningThreads = new GThreadList;
            GASSERT(pRunningThreads);
        }
        pRunningThreads->addThread(pthread);
    }

    static void RemoveRunningThread(GThread *pthread)
    {
        GASSERT(pRunningThreads);        
        pRunningThreads->removeThread(pthread);
    }

    static void FinishAllThreads()
    {
        // This is ok because only root thread can wait for other thread finish.
        if (pRunningThreads)
        {           
            pRunningThreads->finishAllThreads();
            delete pRunningThreads;
            pRunningThreads = 0;
        }        
    }
};

// By default, we have no thread list.
GThreadList* volatile GThreadList::pRunningThreads = 0;


// FinishAllThreads - exposed publicly in GThread.
void GThread::FinishAllThreads()
{
    GThreadList::FinishAllThreads();
}

// *** Run override

SInt	GThread::PRun()
{
    // Suspend us on start, if requested
    if (ThreadFlags & GFC_THREAD_START_SUSPENDED)
    {
        Suspend();
        ThreadFlags &= (UInt32)~GFC_THREAD_START_SUSPENDED;
    }

    // Call the virtual run function
    ExitCode = Run();    
    return ExitCode;
}




// *** User overridables

bool	GThread::GetExitFlag() const
{
    return (ThreadFlags & GFC_THREAD_EXIT) != 0;
}		

void	GThread::SetExitFlag(bool exitFlag)
{
    // The below is atomic since ThreadFlags is GAtomicInt.
    if (exitFlag)
        ThreadFlags |= GFC_THREAD_EXIT;
    else
        ThreadFlags &= (UInt32) ~GFC_THREAD_EXIT;
}


// Determines whether the thread was running and is now finished
bool	GThread::IsFinished() const
{
    return (ThreadFlags & GFC_THREAD_FINISHED) != 0;
}
// Determines whether the thread is suspended
bool	GThread::IsSuspended() const
{	
    return SuspendCount > 0;
}
// Returns current thread state
GThread::ThreadState GThread::GetThreadState() const
{
    if (IsSuspended())
        return Suspended;    
    if (ThreadFlags & GFC_THREAD_STARTED)
        return Running;    
    return NotRunning;
}



// ***** Thread management

// The actual first function called on thread start
void* GThread_PthreadStartFn(void* phandle)
{
    GThread* pthread = (GThread*)phandle;
    DWORD       result = pthread->PRun();
    // Signal the thread as done and release it atomically.
    pthread->FinishAndRelease();
    GThreadList::RemoveRunningThread(pthread);
    return (void*) result;    
}


bool    GThread::Start(ThreadState initialState)
{
    if (initialState == NotRunning)
        return 0;

    if (!InitAttr)
    {
    pthread_attr_init(&Attr);
    pthread_attr_setdetachstate(&Attr, PTHREAD_CREATE_DETACHED);
    }

    // If the thread is already running then wait
    // until its finished to begin running this thread
    if ((GetThreadState() != NotRunning) && !Wait())
        return 0;

    ExitCode        = 0;
    SuspendCount    = 0;
    ThreadFlags     = (initialState == Running) ? 0 : GFC_THREAD_START_SUSPENDED;

    // AddRef to us until the thread is finished
    AddRef();
    GThreadList::AddRunningThread(this);

    if (pthread_create(&ThreadHandle, &Attr, GThread_PthreadStartFn, this))
    {
        ThreadFlags = 0;
        Release();
        GThreadList::RemoveRunningThread(this);
        return 0;
    }

    return 1;
}


// Suspend the thread until resumed
bool    GThread::Suspend()
{
    GFC_DEBUG_WARNING(1, "cannot suspend threads on this system");
    return 0;
}

// Resumes currently suspended thread
bool    GThread::Resume()
{
    return 0;
}


// Quits with an exit code  
void    GThread::Exit(SInt exitCode)
{
    // Can only exist the current thread
   // if (GetThread() != this)
   //     return;

    // Call the virtual OnExit function
    OnExit();   

    // Signal this thread object as done and release it's references.
    FinishAndRelease();
    GThreadList::RemoveRunningThread(this);

    pthread_exit((void *) exitCode);
}



void    GThread::CleanupSystemThread()
{
    // TODO
}



// *** Sleep functions

bool    GThread::Sleep(UInt secs)
{
    sleep(secs);
    return 1;
}
bool    GThread::MSleep(UInt msecs)
{
    usleep(msecs);
    return 1;
}

#endif  // GFC_NO_THREADSUPPORT
