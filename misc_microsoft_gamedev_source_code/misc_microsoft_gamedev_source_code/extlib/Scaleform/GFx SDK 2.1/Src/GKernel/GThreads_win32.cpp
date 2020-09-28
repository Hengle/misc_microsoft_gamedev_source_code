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

// For _beginthreadex / _endtheadex
#include <process.h>

// ***** Classes Implemented

class GLock;
class GMutex;
class GWaitCondition;



// ***** Win32 Mutex implementation

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
    virtual void        Release(UInt flags=0)               { GUNUSED(flags); }
    virtual bool        SetRefCountMode(UInt refCountFlags) { GUNUSED(refCountFlags); return 1; }
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
    HANDLE          hMutexOrSemaphore;
    bool            Recursive;
    volatile UInt   LockCount;

    GMutex_AreadyLockedAcquireInterface AreadyLockedAcquire;
    
    friend class GWaitConditionImpl;


public:
    // Constructor/destructor
    GMutexImpl(GMutex* pmutex, bool recursive = 1);
    ~GMutexImpl();
/*
#if defined(GFC_BUILD_DEFINE_NEW) && defined(GFC_DEFINE_NEW)
#undef new
#endif
    // Allocation - ensure we use our allocator
    void *  operator new(size_t sz)     
        { return GSTATICALLOC(sz); }
    void    operator delete(void* pmem) 
        { GSTATICFREE(pmem); }
    void*   operator new(size_t sz, int blocktype, const char* pfilename, int line)
        { return GSTATICALLOC(sz); }
    void    operator delete(void *p, int blocktype, const char* pfilename, int line)
        { GSTATICFREE(p); }
#if defined(GFC_BUILD_DEFINE_NEW) && defined(GFC_DEFINE_NEW)
#define new GFC_DEFINE_NEW
#endif
*/
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
    Recursive                   = recursive;
    LockCount                   = 0;
    hMutexOrSemaphore           = Recursive ? CreateMutex(NULL, 0, NULL) : CreateSemaphore(NULL, 1, 1, NULL);
}
GMutexImpl::~GMutexImpl()
{
    CloseHandle(hMutexOrSemaphore);
}


// Lock and try lock
void GMutexImpl::Lock()
{
    if (::WaitForSingleObject(hMutexOrSemaphore, INFINITE) != WAIT_OBJECT_0)
        return;
    LockCount++;
}

bool GMutexImpl::TryLock()
{
    DWORD ret;
    if ((ret=::WaitForSingleObject(hMutexOrSemaphore, 0)) != WAIT_OBJECT_0)
        return 0;
    LockCount++;
    return 1;
}

void GMutexImpl::Unlock(GMutex* pmutex)
{
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
    if ((Recursive ? ReleaseMutex(hMutexOrSemaphore) :
                     ReleaseSemaphore(hMutexOrSemaphore, 1, NULL))  != 0)
    {
        // Call wait handlers indirectly here in case owner
        // destroys mutex after finishing wait on it.
        if (lockCount == 0)
            handlers.CallWaitHandlers();
    }
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




// ***** Win32 Wait Condition Implementation

// Internal implementation class
class GWaitConditionImpl : public GNewOverrideBase
{   
    // Event pool entries for extra events
    struct EventPoolEntry  : public GNewOverrideBase
    {
        HANDLE          hEvent;
        EventPoolEntry  *pNext;
        EventPoolEntry  *pPrev;
    };
    
    GLock               WaitQueueLoc;
    // Stores free events that can be used later
    EventPoolEntry  *   pFreeEventList;
    
    // A queue of waiting objects to be signaled    
    EventPoolEntry*     pQueueHead;
    EventPoolEntry*     pQueueTail;

    // Allocation functions for free events
    EventPoolEntry*     GetNewEvent();
    void                ReleaseEvent(EventPoolEntry* pevent);

    // Queue operations
    void                QueuePush(EventPoolEntry* pentry);
    EventPoolEntry*     QueuePop();
    void                QueueFindAndRemove(EventPoolEntry* pentry);


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
    pFreeEventList  = 0;
    pQueueHead      =
    pQueueTail      = 0;
}

GWaitConditionImpl::~GWaitConditionImpl()
{
    // Free all the resources
    EventPoolEntry* p       = pFreeEventList;
    EventPoolEntry* pentry;

    while(p)
    {
        // Move to next
        pentry = p;
        p = p->pNext;
        // Delete old
        ::CloseHandle(pentry->hEvent);
        delete pentry;  
    }   
    // Shouldn't we also consider the queue?

    // To be safe
    pFreeEventList  = 0;
    pQueueHead      =
    pQueueTail      = 0;
}


// Allocation functions for free events
GWaitConditionImpl::EventPoolEntry* GWaitConditionImpl::GetNewEvent()
{
    EventPoolEntry* pentry;

    // If there are any free nodes, use them
    if (pFreeEventList)
    {
        pentry          = pFreeEventList;
        pFreeEventList  = pFreeEventList->pNext;        
    }
    else
    {
        // Allocate a new node
        pentry          = new EventPoolEntry;
        pentry->pNext   = 0;
        pentry->pPrev   = 0;
        // Non-signaled manual event
        pentry->hEvent  = ::CreateEvent(NULL, TRUE, 0, NULL);
    }
    
    return pentry;
}

void        GWaitConditionImpl::ReleaseEvent(EventPoolEntry* pevent)
{
    // Mark event as non-signaled
    ::ResetEvent(pevent->hEvent);
    // And add it to free pool
    pevent->pNext   = pFreeEventList;
    pevent->pPrev   = 0;
    pFreeEventList  = pevent;
}

// Queue operations
void        GWaitConditionImpl::QueuePush(EventPoolEntry* pentry)
{
    // Items already exist? Just add to tail
    if (pQueueTail)
    {
        pentry->pPrev       = pQueueTail;
        pQueueTail->pNext   = pentry;
        pentry->pNext       = 0;        
        pQueueTail          = pentry;       
    }
    else
    {
        // No items in queue
        pentry->pNext   = 
        pentry->pPrev   = 0;
        pQueueHead      =
        pQueueTail      = pentry;
    }
}

GWaitConditionImpl::EventPoolEntry* GWaitConditionImpl::QueuePop()
{
    EventPoolEntry* pentry = pQueueHead;

    // No items, null pointer
    if (pentry)
    {
        // More items after this one? just grab the first item
        if (pQueueHead->pNext)
        {       
            pQueueHead  = pentry->pNext;
            pQueueHead->pPrev = 0;      
        }
        else
        {
            // Last item left
            pQueueTail =
            pQueueHead = 0;
        }
    }   
    return pentry;
}

void        GWaitConditionImpl::QueueFindAndRemove(EventPoolEntry* pentry)
{
    // Do an exhaustive search looking for an entry
    EventPoolEntry* p = pQueueHead;

    while(p)
    {
        // Entry found? Remove.
        if (p == pentry)
        {
            
            // Remove the node form the list
            // Prev link
            if (pentry->pPrev)
                pentry->pPrev->pNext = pentry->pNext;
            else
                pQueueHead = pentry->pNext;
            // Next link
            if (pentry->pNext)
                pentry->pNext->pPrev = pentry->pPrev;
            else
                pQueueTail = pentry->pPrev;
            // Done
            return;
        }

        // Move to next item
        p = p->pNext;
    }
}
    

bool    GWaitConditionImpl::Wait(GMutex *pmutex, UInt delay)
{
    bool            result = 0;
    UInt            i;
    UInt            lockCount = pmutex->pImpl->LockCount;
    EventPoolEntry* pentry;

    // Mutex must have been locked
    if (lockCount == 0)
        return 0;
    
    // Add an object to the wait queue
    WaitQueueLoc.Lock();
    QueuePush(pentry = GetNewEvent());
    WaitQueueLoc.Unlock();

    // Finally, release a mutex or semaphore
    if (pmutex->pImpl->Recursive)
    {
        // Release the recursive mutex N times
        pmutex->pImpl->LockCount = 0;
        for(i=0; i<lockCount; i++)
            ::ReleaseMutex(pmutex->pImpl->hMutexOrSemaphore);
        // NOTE: Do not need to use CallableHanders here because mutex
        // can not be destroyed by user if we are to re-acquire it later.
        pmutex->CallWaitHandlers();
    }
    else
    {
        pmutex->pImpl->LockCount = 0;
        ::ReleaseSemaphore(pmutex->pImpl->hMutexOrSemaphore, 1, NULL);
        pmutex->CallWaitHandlers();
    }

    // Note that there is a gap here between mutex.Unlock() and Wait(). However,
    // if notify() comes in at this point in the other thread it will set our
    // corresponding event so wait will just fall through, as expected.

    // Block and wait on the event
    DWORD waitResult = ::WaitForSingleObject(pentry->hEvent,
                            (delay == GFC_WAIT_INFINITE) ? INFINITE : delay);

    WaitQueueLoc.Lock();
    switch(waitResult)
    {
        case WAIT_ABANDONED:
        case WAIT_OBJECT_0: 
            result = 1;
            // Wait was successful, therefore the event entry should already be removed
            // So just add entry back to a free list
            ReleaseEvent(pentry);
            break;
        default:
            // Timeout, our entry should still be in a queue
            QueueFindAndRemove(pentry);
            ReleaseEvent(pentry);
    }
    WaitQueueLoc.Unlock();

    // Re-aquire the mutex
    for(i=0; i<lockCount; i++)
        pmutex->Lock(); 

    // Return the result
    return result;
}

// Notify a condition, releasing the least object in a queue
void    GWaitConditionImpl::Notify()
{
    GLock::Locker   lock(&WaitQueueLoc);
    
    // Pop last entry & signal it
    EventPoolEntry* pentry = QueuePop();    
    if (pentry)
        ::SetEvent(pentry->hEvent); 
}

// Notify a condition, releasing all objects waiting
void    GWaitConditionImpl::NotifyAll()
{
    GLock::Locker   lock(&WaitQueueLoc);

    // Pop and signal all events
    // NOTE : There is no need to release the events, it's the waiters job to do so 
    EventPoolEntry* pentry = QueuePop();
    while (pentry)
    {
        ::SetEvent(pentry->hEvent);
        pentry = QueuePop();
    }
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

//  Per-thread variable
//  MA: Don't use TLS for now - portability issues with DLLs on 360, etc.
/*
#if !defined(GFC_CC_MSVC) || (GFC_CC_MSVC < 1300)
__declspec(thread)  GThread*    pCurrentThread      = 0;
#else
#pragma data_seg(".tls$")
__declspec(thread)  GThread*    pCurrentThread      = 0;
#pragma data_seg(".rwdata")
#endif
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
    CleanupSystemThread();
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



/* MA: Don't use TLS for now.

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
unsigned WINAPI GThread_Win32StartFn(void * phandle)
{
    GThread *   pthread = (GThread*)phandle;
    if (pthread->Processor != -1)
    {
#if defined(GFC_OS_XBOX360)
        DWORD ret = XSetThreadProcessor(GetCurrentThread(), pthread->Processor);
        GFC_DEBUG_ERROR(ret == (DWORD)-1, "Could not set hardware processor for the thread");
        GUNUSED(ret);
#elif defined(GFC_OS_WIN32)
        DWORD_PTR ret = SetThreadAffinityMask(GetCurrentThread(), (DWORD)pthread->Processor);
        GFC_DEBUG_ERROR(ret == 0, "Could not set hardware processor for the thread");
        GUNUSED(ret);
#endif
    }
    DWORD       result = pthread->PRun();
    // Signal the thread as done and release it atomically.
    pthread->FinishAndRelease();
    // At this point GThread object might be dead; however we can still pass
    // it to RemoveRunningThread since it is only used as a key there.    
    GThreadList::RemoveRunningThread(pthread);
    return (unsigned) result;
}


bool    GThread::Start(ThreadState initialState)
{
    if (initialState == NotRunning)
        return 0;

    // If the thread is already running then wait
    // until its finished to begin running this thread
    if ((GetThreadState() != NotRunning) && !Wait())
        return 0;

    // Free old thread handle before creating the new one
    CleanupSystemThread();

    // AddRef to us until the thread is finished.
    AddRef();
    GThreadList::AddRunningThread(this);
    
    ExitCode        = 0;
    SuspendCount    = 0;
    ThreadFlags     = (initialState == Running) ? 0 : GFC_THREAD_START_SUSPENDED;
    ThreadHandle = (HANDLE) _beginthreadex(0, (unsigned)StackSize, GThread_Win32StartFn, this, 0, 0);

    // Failed? Fail the function
    if (ThreadHandle == 0)
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
    // Can't suspend a thread that wasn't started
    if (!(ThreadFlags & GFC_THREAD_STARTED))
        return 0;

    if (::SuspendThread(ThreadHandle) != 0xFFFFFFFF)
    {        
        SuspendCount++;        
        return 1;
    }
    return 0;
}

// Resumes currently suspended thread
bool    GThread::Resume()
{
    // Can't suspend a thread that wasn't started
    if (!(ThreadFlags & GFC_THREAD_STARTED))
        return 0;

    // Decrement count, and resume thread if it is 0
    SInt32 oldCount = SuspendCount.ExchangeAdd_Acquire(-1);
    if (oldCount >= 1)
    {
        if (oldCount == 1)
        {
            if (::ResumeThread(ThreadHandle) != 0xFFFFFFFF)            
                return 1;            
        }
        else
        {
            return 1;
        }
    }   
    return 0;
}


// Quits with an exit code  
void    GThread::Exit(SInt exitCode)
{
    // Can only exist the current thread.
    // MA: Don't use TLS for now.
    //if (GetThread() != this)
    //    return;

    // Call the virtual OnExit function.
    OnExit();   

    // Signal this thread object as done and release it's references.
    FinishAndRelease();
    GThreadList::RemoveRunningThread(this);

    // Call the exit function.    
    _endthreadex((unsigned)exitCode);
}


void    GThread::CleanupSystemThread()
{
    if (ThreadHandle != 0)
    {
        ::CloseHandle(ThreadHandle);
        ThreadHandle = 0;
    }
}

// *** Sleep functions

bool    GThread::Sleep(UInt secs)
{
    ::Sleep(secs*1000);
    return 1;
}
bool    GThread::MSleep(UInt msecs)
{
    ::Sleep(msecs);
    return 1;
}

/* static */
int     GThread::GetCPUCount()
{
#if defined(GFC_OS_WIN32) 
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    return (int) sysInfo.dwNumberOfProcessors;
#elif defined(GFC_OS_XBOX)
    return 1;
#elif defined(GFC_OS_XBOX360)
    return 6;
#endif
}


#endif


