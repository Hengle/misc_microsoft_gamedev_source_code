/**********************************************************************

Filename    :   GThreads_pthread.cpp
Content     :   pthreads support
Created     :   May 5, 2003
Authors     :   Andrew Reisse

Copyright   :   (c) 2003 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GThreads.h"

#ifndef GFC_NO_THREADSUPPORT

#include "GTimer.h"

#include <pthread.h>
#include <time.h>

#ifdef GFC_OS_PS3
#include <sys/sys_time.h>
#include <sys/timer.h>
#include <sys/synchronization.h>
#define sleep(x) sys_timer_sleep(x)
#define usleep(x) sys_timer_usleep(x)
using std::timespec;
#else
#include <unistd.h>
#include <sys/time.h>
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
    pthread_t     LockedBy;

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

pthread_mutexattr_t GLock::RecursiveAttr;
bool GLock::RecursiveAttrInit = 0;

// *** Constructor/destructor
GMutexImpl::GMutexImpl(GMutex* pmutex, bool recursive)
{   
    AreadyLockedAcquire.pMutex  = pmutex;
    Recursive           = recursive;
    LockCount           = 0;

    if (Recursive)
    {
        if (!GLock::RecursiveAttrInit)
        {
            pthread_mutexattr_init(&GLock::RecursiveAttr);
            pthread_mutexattr_settype(&GLock::RecursiveAttr, PTHREAD_MUTEX_RECURSIVE);
            GLock::RecursiveAttrInit = 1;
        }

        pthread_mutex_init(&Mutex, &GLock::RecursiveAttr);
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
    while (pthread_mutex_lock(&Mutex));
    LockCount++;
    LockedBy = pthread_self();
}

bool GMutexImpl::TryLock()
{
    if (!pthread_mutex_trylock(&Mutex))
    {
        LockCount++;
        LockedBy = pthread_self();
        return 1;
    }
    
    return 0;
}

void GMutexImpl::Unlock(GMutex* pmutex)
{
    GASSERT(pthread_self() == LockedBy && LockCount > 0);

    UInt lockCount;
    LockCount--;
    lockCount = LockCount;

    // At this point handlers, if any, MUST already be created and
    // lazy initialization for pHandlers can not be used. To address this,
    // we allow an optional handler enable flag to be passed in constructor.
    // If we allowed lazy initialization, a call to AddHandlers in another
    // thread could access us after pHandlers read, causing the handler
    // to never be called (never informed about Unlock).
    GMutex::CallableHandlers handlers;
    pmutex->GetCallableHandlers(&handlers);

    // Release and Notify waitable objects
    pthread_mutex_unlock(&Mutex);

    // Call wait handlers indirectly here in case owner
    // destroys mutex after finishing wait on it.
    if (lockCount == 0)
        handlers.CallWaitHandlers();
}

bool    GMutexImpl::IsLockedByAnotherThread(GMutex* pmutex)
{
    // There could be multiple interpretations of IsLocked with respect to current thread
    if (LockCount == 0)
        return 0;
    if (pthread_self() != LockedBy)
        return 1;
    return 0;
}

bool    GMutexImpl::IsSignaled() const
{
    // An mutex is signaled if it is not locked ANYWHERE
    // Note that this is different from IsLockedByAnotherThread function,
    // that takes current thread into account
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

GMutex::GMutex(bool recursive, bool multiWait)
    : GWaitable(multiWait)
{
    // NOTE: RefCount mode already thread-safe for all waitables.
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
        for(UInt i=0; i<lockCount; i++)
            pthread_mutex_unlock(&pmutex->pImpl->Mutex);
        // NOTE: Do not need to use CallableHanders here because mutex
        // can not be destroyed by user if we are to re-acquire it later.
        pmutex->CallWaitHandlers();
    }
    else
    {
        pmutex->pImpl->LockCount = 0;
        pthread_mutex_unlock(&pmutex->pImpl->Mutex);
        pmutex->CallWaitHandlers();
    }

    // Note that there is a gap here between mutex.Unlock() and Wait().
    // The other mutex protects this gap.

    if (delay == GFC_WAIT_INFINITE)
        pthread_cond_wait(&Condv,&Mutex);
    else
    {
        timespec ts;
#ifdef GFC_OS_PS3
        sys_time_sec_t s;
        sys_time_nsec_t ns;
        sys_time_get_current_time(&s, &ns);

        ts.tv_sec = s + delay;
        ts.tv_nsec = ns;

#else
    struct timeval tv;
    gettimeofday(&tv, 0);

        ts.tv_sec = tv.tv_sec + delay;
    ts.tv_nsec = tv.tv_usec * 1000;
#endif

        int r = pthread_cond_timedwait(&Condv,&Mutex, &ts);
        if (r)
            result = 0;
    }

    pthread_mutex_unlock(&Mutex);

    // Re-aquire the mutex
    for(UInt i=0; i<lockCount; i++)
        pmutex->Lock(); 

    // Return the result
    return result;
}

// Notify a condition, releasing the least object in a queue
void    GWaitConditionImpl::Notify()
{
    pthread_mutex_lock(&Mutex);
    pthread_cond_signal(&Condv);
    pthread_mutex_unlock(&Mutex);
}

// Notify a condition, releasing all objects waiting
void    GWaitConditionImpl::NotifyAll()
{
    pthread_mutex_lock(&Mutex);
    pthread_cond_broadcast(&Condv);
    pthread_mutex_unlock(&Mutex);
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


// *** GThread constructors.

GThread::GThread(UPInt stackSize, int processor) : GWaitable(1)
{
    // NOTE: RefCount mode already thread-safe for all GWaitable objects.

    // Clear the variables    
    ThreadFlags     = 0;
    ThreadHandle    = 0;
    ExitCode        = 0;
    SuspendCount    = 0;
    StackSize       = stackSize;
    Processor       = processor;

    // Clear Function pointers
    ThreadFunction  = 0;    
    UserHandle      = 0;
}

GThread::GThread(GThread::ThreadFn threadFunction, void*  userHandle, UPInt stackSize,
                 int processor, GThread::ThreadState initialState)
    : GWaitable(1)
{
    // Clear the variables
    ThreadFlags     = 0;
    ThreadHandle    = 0;
    ExitCode        = 0;
    SuspendCount    = 0;
    StackSize       = stackSize;
    Processor       = processor;

    // Initialize function pointers.
    ThreadFunction  = threadFunction;    
    UserHandle      = userHandle;
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
    ThreadHandle = 0;
}



// *** Overridable User functions.

// Default Run implementation
SInt    GThread::Run()
{
    // Call pointer to function, if available.    
    return (ThreadFunction) ? ThreadFunction(this, UserHandle) : 0;
}
void    GThread::OnExit()
{   
}


// Finishes the thread and releases internal reference to it.
void    GThread::FinishAndRelease()
{
    // Get callable handlers so that they can still be called
    // after GThread object is released.
    CallableHandlers handlers;
    GetCallableHandlers(&handlers);

    // Note: thread must be US.
    ThreadFlags &= (UInt32)~(GFC_THREAD_STARTED);
    ThreadFlags |= GFC_THREAD_FINISHED;

    // Release our reference; this is equivalent to 'delete this'
    // from the point of view of our thread.
    Release();

    // Call handlers, if any, signifying completion.
    handlers.CallWaitHandlers();
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
    pthread_t                              RootThreadId;

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
        GASSERT(pthread_self() == RootThreadId);

        GMutex::Locker lock(&ThreadMutex);
        while (ThreadSet.size() != 0)
            ThreadsEmpty.Wait(&ThreadMutex);
    }

public:

    GThreadList()
    {
        RootThreadId = pthread_self();
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

    // NOTE: 'pthread' might be a dead pointer when this is
    // called so it should not be accessed; it is only used
    // for removal.
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

SInt    GThread::PRun()
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

bool    GThread::GetExitFlag() const
{
    return (ThreadFlags & GFC_THREAD_EXIT) != 0;
}       

void    GThread::SetExitFlag(bool exitFlag)
{
    // The below is atomic since ThreadFlags is GAtomicInt.
    if (exitFlag)
        ThreadFlags |= GFC_THREAD_EXIT;
    else
        ThreadFlags &= (UInt32) ~GFC_THREAD_EXIT;
}


// Determines whether the thread was running and is now finished
bool    GThread::IsFinished() const
{
    return (ThreadFlags & GFC_THREAD_FINISHED) != 0;
}
// Determines whether the thread is suspended
bool    GThread::IsSuspended() const
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
    SInt     result = pthread->PRun();
    // Signal the thread as done and release it atomically.
    pthread->FinishAndRelease();
    // At this point GThread object might be dead; however we can still pass
    // it to RemoveRunningThread since it is only used as a key there.   
    GThreadList::RemoveRunningThread(pthread);
    return (void*) result;
}

int GThread::InitAttr = 0;
pthread_attr_t GThread::Attr;

bool    GThread::Start(ThreadState initialState)
{
    if (initialState == NotRunning)
        return 0;

    if (!InitAttr)
    {
        pthread_attr_init(&Attr);
        pthread_attr_setdetachstate(&Attr, PTHREAD_CREATE_DETACHED);
        pthread_attr_setstacksize(&Attr, 128 * 1024);
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

    int result;

    if (StackSize != 128 * 1024)
    {
        pthread_attr_t attr;

        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        pthread_attr_setstacksize(&attr, StackSize);
        result = pthread_create(&ThreadHandle, &attr, GThread_PthreadStartFn, this);
        pthread_attr_destroy(&attr);
    }
    else
        result = pthread_create(&ThreadHandle, &Attr, GThread_PthreadStartFn, this);

    if (result)
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



// *** Sleep functions

bool    GThread::Sleep(UInt secs)
{
    sleep(secs);
    return 1;
}
bool    GThread::MSleep(UInt msecs)
{
    usleep(msecs*1000);
    return 1;
}

/* static */
int     GThread::GetCPUCount()
{
    return 1;
}

#ifdef GFC_OS_PS3

sys_lwmutex_attribute_t GLock::LockAttr = { SYS_SYNC_PRIORITY, SYS_SYNC_RECURSIVE };

#endif

#endif  // GFC_NO_THREADSUPPORT
