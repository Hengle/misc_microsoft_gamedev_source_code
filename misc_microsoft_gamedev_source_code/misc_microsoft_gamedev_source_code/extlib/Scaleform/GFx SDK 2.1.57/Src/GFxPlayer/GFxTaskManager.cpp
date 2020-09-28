/**********************************************************************

Filename    :   GFxTaskManager.cpp
Content     :   Threaded task manager implementatoin
Created     :   May 31, 2007
Authors     :   Michael Antonov

Notes       :   A GFxTask is a unit of work that can be executed
                on several threads.

Copyright   :   (c) 2005-2007 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GFxTaskManager.h"
#include "GThreads.h"

#ifndef GFC_NO_THREADSUPPORT

// ***** GFxTasksContainer

// This class is used to keep track of currently running tasks by a task manager.
// and it is needed for TaskManager to send an abandon signal for running tasks.
class GFxTasksContainer
{
public:
    GFxTasksContainer(GMutex* ptaskMutex) : pTasksMutex(ptaskMutex) {}
    ~GFxTasksContainer() 
    {
        AbandonAllTask();
    }

    void AddTask(GFxTask* ptask) 
    {
        GASSERT(ptask);
        if (!ptask) return;
        GMutex::Locker guard(pTasksMutex);
        Tasks.push_back(ptask);
    }
    bool RemoveTask(GFxTask* ptask)
    {
        GASSERT(ptask);
        if (!ptask) return false;
        GMutex::Locker guard(pTasksMutex);
        for(UInt i=0; i<Tasks.size(); i++)
        {
            if (Tasks[i] == ptask)
            {
                Tasks.remove(i);
                return true;
            }
        }
        return false;
    }
    bool FindTask(GFxTask* ptask)
    {
        GASSERT(ptask);
        if (!ptask) return false;
        GMutex::Locker guard(pTasksMutex);
        for(UInt i=0; i<Tasks.size(); i++)
        {
            if (Tasks[i] == ptask)
                return true;
        }
        return false;
    }
    bool AbandonTask(GFxTask* ptask)
    {
        GASSERT(ptask);
        if (!ptask) return false;
        GMutex::Locker guard(pTasksMutex);
        for(UInt i=0; i<Tasks.size(); i++)
        {
            if (Tasks[i] == ptask)
            {
                Tasks[i]->OnAbandon(true);
                return true;
            }
        }
        return false;
    }
    void AbandonAllTask() 
    {
        GMutex::Locker guard(pTasksMutex);
        for(UInt i=0; i<Tasks.size(); i++)
            Tasks[i]->OnAbandon(true);
    }
private:
    GMutex*               pTasksMutex;
    GTL::garray<GFxTask*> Tasks;
};

// ***** GFxThreadTaskManagerImpl

// Threaded Task Manager is divided in two part to avoid circular dependences. 
// a task can created another tasks and can hold a strong pointer to GFxThreadedTaskManager
// a task thread also need task manager so it holds a strong pointer to GFxThreadedTaskManagerImpl
class GFxTaskThreadPool;
class GFxThreadedTaskManagerImpl: public GRefCountBase<GFxThreadedTaskManagerImpl>
{
public:
    GFxThreadedTaskManagerImpl();
    ~GFxThreadedTaskManagerImpl();

    bool               AddWorkerThreads(UInt taskMask, UInt count, UPInt stackSize, int processor = -1);
    GFxTasksContainer* GetRunningTasks () { return &RunningTasks; }
    GFxTaskThreadPool* GetThreadPool   ()   { return pThreadPool; }
    bool               AbandonTask     (GFxTask* ptask);

    void               RequestShutdown ();
private:
    friend class GFxTaskThreadPool;
    GMutex             TasksMutex;
    GFxTasksContainer  RunningTasks;
    GFxTaskThreadPool* pThreadPool;
};

// ***** GFxTaskThread

// an object of this class execute a tasks in a separate thread
// upon task completion the thread object gets delete

class GFxTaskThread : public GThread
{
public:
    GFxTaskThread(GFxTask* ptask, GFxThreadedTaskManagerImpl* ptm, UPInt stackSize = 128 * 1024, int processor = -1)
        : GThread(stackSize,processor), pTask(ptask), pTaskManager(ptm)
    { 
    //    printf("GFxTaskThread::GFxTaskThread : %x, thread: %d\n", this, GetCurrentThreadId()); 
    }
    ~GFxTaskThread();

    virtual SInt Run();
protected:
    GPtr<GFxTask>                    pTask;
    GPtr<GFxThreadedTaskManagerImpl> pTaskManager;
};

// ***** GFxTaskThreadInPool

// objects of this call should be added into a thread pool. 
// after tasks completion thread stays in the pool waiting 
// for anther task

class GFxTaskThreadInPool : public GFxTaskThread
{
public:
    GFxTaskThreadInPool(UInt taskMask, GFxThreadedTaskManagerImpl* ptm, UPInt stackSize = 128 * 1024, int processor = -1)
       : GFxTaskThread(0, ptm, stackSize, processor), TaskMask(taskMask)
    {}
    ~GFxTaskThreadInPool() {}

    bool         SetTask(GFxTask* ptask);
    virtual SInt Run();

    UInt         GetTaskType() const { return TaskMask; }

private:
    UInt TaskMask;    
};

// ***** GFxTaskThreadPool

// Threaded pool implementation
class GFxTaskThreadPool: public GNewOverrideBase
{
public:
    GFxTaskThreadPool(GFxThreadedTaskManagerImpl* ptm);
    ~GFxTaskThreadPool();

    bool     AddTask      (GFxTask* ptask);
    bool     AbandonTask  (GFxTask* ptask);
    bool     AddThreads   (UInt taskMask, UInt count, UPInt stackSize, int processor = -1);
    bool     RemoveThread (GFxTaskThread*);

    // adds a task's ref count and returns it from the tasks queue. 
    // if there is no task in the queue waits on a condition variable until 
    // queue is not empty. Returns 0 if shutdown is requested. 
    GFxTask* GetTaskAddRef(UInt taskMask);

    // instruct the pool to stop accepting new task and send abandon signal 
    // for all tasks (in the task queue and currently running)
    void     RequestShutdown();

private:
    GFxTask* FindTaskByMask(UInt taskMask);

    GTL::garray<GPtr<GFxTask> >       TasksQueue;
    GTL::garray<GFxTaskThreadInPool*> Threads;
    GLock                             ThreadsLock;
    GWaitCondition                    TaskWaitCondition;
    volatile bool                     ShutdownRequested;
    GFxThreadedTaskManagerImpl*       pTaskManager;
};

/******************************************************************/
GFxThreadedTaskManagerImpl::GFxThreadedTaskManagerImpl()
    : RunningTasks(&TasksMutex), pThreadPool(0)
{
    //printf("GFxThreadedTaskManagerImpl::GFxThreadedTaskManagerImpl : %x, thread : %d\n", this, GetCurrentThreadId());
    SetRefCountMode(GFC_REFCOUNT_THREADSAFE);
}

GFxThreadedTaskManagerImpl::~GFxThreadedTaskManagerImpl()
{
    //printf("GFxThreadedTaskManagerImpl::~GFxThreadedTaskManagerImpl : %x, thread : %d\n", this,GetCurrentThreadId());
    RunningTasks.AbandonAllTask();
    delete pThreadPool;
}

bool GFxThreadedTaskManagerImpl::AddWorkerThreads(UInt taskMask, UInt count, UPInt stackSize, int processor)
{
    if (!pThreadPool)
        pThreadPool = new GFxTaskThreadPool(this);
    return pThreadPool->AddThreads(taskMask,count,stackSize,processor);
}

void GFxThreadedTaskManagerImpl::RequestShutdown() 
{ 
    RunningTasks.AbandonAllTask();
    if (pThreadPool) pThreadPool->RequestShutdown(); 
}

bool GFxThreadedTaskManagerImpl::AbandonTask(GFxTask* ptask)
{
    if (pThreadPool) 
        return pThreadPool->AbandonTask(ptask);
    return RunningTasks.AbandonTask(ptask);
}

/******************************************************************/
GFxTaskThread::~GFxTaskThread()
{
    //printf("GFxTaskThread::GFxTaskThread : %x, thread : %d\n", this, GetCurrentThreadId());
    if (pTask)
    {
        pTask->OnAbandon(true);
        pTaskManager->GetRunningTasks()->RemoveTask(pTask);
    }
    GFxTaskThreadPool* pool = pTaskManager->GetThreadPool();
    if (pool) pool->RemoveThread(this);
}

SInt GFxTaskThread::Run()
{
    GASSERT(pTask);
    if (pTask) 
    {
        //pTaskManager->GetRunningTasks()->AddTask(pTask);
        pTask->Execute();        
        pTaskManager->GetRunningTasks()->RemoveTask(pTask);
        pTask = 0; // Release task's resources.
    }
    return 1;
}

bool GFxTaskThreadInPool::SetTask(GFxTask* ptask) 
{
    GASSERT(!pTask);
    GASSERT(ptask);
    if (pTask) return false;
    pTask = ptask;
    return true;
}

SInt GFxTaskThreadInPool::Run()
{   
    GPtr<GFxTask> ptask;
    while ((ptask = *pTaskManager->GetThreadPool()->GetTaskAddRef(TaskMask)))
    {
        SetTask(ptask);
        GFxTaskThread::Run();
        ptask = 0; // if ptask is not release here it would lead to memory leak.
    }
    return 1;
}

/******************************************************************/
GFxTaskThreadPool::GFxTaskThreadPool(GFxThreadedTaskManagerImpl* ptm) 
    : ShutdownRequested(false), pTaskManager(ptm)
{
    GASSERT(pTaskManager);
}

GFxTaskThreadPool::~GFxTaskThreadPool() 
{
    RequestShutdown();
    for (UInt i=0; i<Threads.size(); i++)
        Threads[i]->Wait();
}

bool GFxTaskThreadPool::AddTask(GFxTask* ptask) 
{
    GASSERT(ptask);
    if (ShutdownRequested) 
        return false;
    {
        GLock::Locker guard(&ThreadsLock);
        UInt i;
        for (i=0; i<Threads.size(); i++)
        {
            if ((UInt)ptask->GetTaskType() == Threads[i]->GetTaskType())
                break;
        }
        GFC_DEBUG_ERROR(i == Threads.size(), "There is no a thread in thread pool which can process a given task");
        if (i == Threads.size())
            return false;
    }
    GMutex::Locker guard(&pTaskManager->TasksMutex);
    TasksQueue.push_back(ptask);
    TaskWaitCondition.NotifyAll();
    return true;
}


bool GFxTaskThreadPool::AddThreads(UInt taskMask, UInt count, UPInt stackSize, int processor)
{
    if (ShutdownRequested)
        return false;
    // TODO: it may be useful to check and give an warring if stackSize is deferent from the one
    // that has already been set for threads with a given taskMask
    GLock::Locker guard(&ThreadsLock);
    while (count-- > 0)
    {
        GPtr<GFxTaskThreadInPool> thread = *new GFxTaskThreadInPool(taskMask, pTaskManager, stackSize, processor);
        thread->Start();
        Threads.push_back(thread);
    }
    return true;
}
bool GFxTaskThreadPool::RemoveThread(GFxTaskThread* thread)
{
    GLock::Locker guard(&ThreadsLock);
    for (UInt i=0; i<Threads.size(); i++)
    {
        if (Threads[i] == thread) 
        {
            Threads.remove(i);
            return true;
        }
    }
    return false;
}

GFxTask* GFxTaskThreadPool::GetTaskAddRef(UInt taskMask)
{
    if (ShutdownRequested)
        return 0;
    GMutex::Locker guard(&pTaskManager->TasksMutex);
    GFxTask* ptask = 0;
    while (!ShutdownRequested && (ptask = FindTaskByMask(taskMask)) == 0)
        TaskWaitCondition.Wait(&pTaskManager->TasksMutex);
    if (ptask)
        pTaskManager->GetRunningTasks()->AddTask(ptask);
    return ptask;
}

GFxTask* GFxTaskThreadPool::FindTaskByMask(UInt taskMask)
{
    GFxTask *ptask = 0;
    for (UInt i=0; i<TasksQueue.size(); i++)
    {
        if (TasksQueue[i]->GetTaskType() & taskMask)
        {
            ptask = TasksQueue[i];
            ptask->AddRef();
            TasksQueue.remove(i);
            break;
        }
    }
    return ptask;
}

void GFxTaskThreadPool::RequestShutdown() 
{ 
    GMutex::Locker guard(&pTaskManager->TasksMutex);
    if (ShutdownRequested)
        return;
    ShutdownRequested = true;
    for (UInt i=0; i<TasksQueue.size(); i++)
        TasksQueue[i]->OnAbandon(false);
    TasksQueue.clear();
    TaskWaitCondition.NotifyAll();
}

bool GFxTaskThreadPool::AbandonTask(GFxTask* ptask)
{
    GASSERT(ptask);
    if (!ptask) return false;
    GMutex::Locker guard(&pTaskManager->TasksMutex);
    //fist check in the task queue
    for (UInt i=0; i<TasksQueue.size(); i++)
    {
        if (TasksQueue[i] == ptask)
        {
            TasksQueue[i]->OnAbandon(false);
            TasksQueue.remove(i);
            return true;
        }
    }
    // if did not find there try running tasks
    return pTaskManager->GetRunningTasks()->AbandonTask(ptask);
}


/******************************************************************/
GFxThreadedTaskManager::GFxThreadedTaskManager(UPInt stackSize) 
    : ThreadStackSize(stackSize)
{
    //printf("GFxThreadedTaskManager::GFxThreadedTaskManager : %x, thread: %d\n", this, GetCurrentThreadId());
    pImpl = new GFxThreadedTaskManagerImpl;
}

GFxThreadedTaskManager::~GFxThreadedTaskManager()
{
    //printf("GFxThreadedTaskManager::~GFxThreadedTaskManager : %x, thread: %d\n", this, GetCurrentThreadId());
    if (pImpl) 
    {
        pImpl->RequestShutdown();
        pImpl->Release();
    }
}

bool GFxThreadedTaskManager::AddTask(GFxTask* ptask)
{    
    GASSERT(ptask != 0);
    if (!ptask) return false;
    GFxTaskThreadPool* pthreadPool = pImpl->GetThreadPool();
    if (pthreadPool && pthreadPool->AddTask(ptask)) 
        return true;
    
    GPtr<GFxTaskThread> ptaskThread = *new GFxTaskThread(ptask, pImpl, ThreadStackSize);
    if (ptaskThread)
    {        
        pImpl->GetRunningTasks()->AddTask(ptask);
        ptaskThread->Start();
        // Thread will release itself once the task is finished.
        return true;
    }
    return false;
}

bool GFxThreadedTaskManager::AbandonTask(GFxTask* ptask)
{
    return pImpl->AbandonTask(ptask);
}

bool GFxThreadedTaskManager::AddWorkerThreads(UInt taskMask, UInt count, UPInt stackSize, int processor)
{
    return pImpl->AddWorkerThreads(taskMask,count,stackSize,processor);
}
void GFxThreadedTaskManager::RequestShutdown()
{
    pImpl->RequestShutdown();
}

#endif
