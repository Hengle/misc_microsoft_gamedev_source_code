/**********************************************************************

Filename    :   GAtomic.h
Content     :   Contains atomic operations and inline fastest locking
                functionality. Will contain #ifdefs for OS efficiency.
                Have non-thread-safe implementaion if not available.
Created     :   May 5, 2003
Authors     :   Michael Antonov, Andrew Reisse

Copyright   :   (c) 2003-2007 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GAtomic.h"

#ifndef GFC_NO_THREADSUPPORT


// ***** Windows Lock implementation


#if defined(GFC_OS_WIN32) || defined(GFC_OS_WINCE) || defined(GFC_OS_XBOX) || defined(GFC_OS_XBOX360)

#if !defined(GFC_FAST_GLOCK)

// ***** Standard Win32 GLock implementation

#if defined(GFC_OS_WIN32)
// Spin count init critical section function prototype for Window NT
typedef BOOL (WINAPI *Function_InitializeCriticalSectionAndSpinCount) 
             (LPCRITICAL_SECTION lpCriticalSection, DWORD dwSpinCount);
#endif

// Constructors
GLock::GLock(UInt spinCount)
{
#if defined(GFC_OS_WIN32)
    // Try to load function dynamically so that we don't require NT
    // On Windows NT we will use InitializeCriticalSectionAndSpinCount
    static  bool initTried = 0;
    static  Function_InitializeCriticalSectionAndSpinCount pInitFn = 0;

    if (!initTried)
    {
        HMODULE hmodule = ::LoadLibrary(GSTR("kernel32.dll"));
        pInitFn     = (Function_InitializeCriticalSectionAndSpinCount)
            ::GetProcAddress(hmodule, "InitializeCriticalSectionAndSpinCount");
        initTried   = 1;
    }

    // Initialize the critical section
    if (pInitFn)
        pInitFn(&cs, spinCount);
    else
        ::InitializeCriticalSection(&cs);

#elif defined(GFC_OS_XBOX360)    
    ::InitializeCriticalSectionAndSpinCount(&cs, spinCount);
#else 
    ::InitializeCriticalSection(&cs);
#endif
}

GLock::~GLock()
{
    DeleteCriticalSection(&cs);
}


#else

// ***** Fast Win32 + X86 GLock implementation

GLock::GLock(UInt spinCount)
{    
    RecursiveLockCount  = 0;
    LockedThreadId.Value= 0;
    WaiterCount.Value   = 0;
    hSemaphore          = 0;
    SetSpinMax(spinCount);
}

GLock::~GLock()
{
    if (hSemaphore)
        CloseHandle(hSemaphore);
}


// *** Locking functions.

void GLock::Lock()
{
    DWORD threadId = GetCurrentThreadId();

    if (threadId != LockedThreadId.Value)
    {
        if ((LockedThreadId.Value == 0) && PerfLockImmediate(threadId))
        {
            // Successful single instruction quick-lock
        }
        else
        {
            PerfLock(threadId); 
        }
    }
    RecursiveLockCount++;
}

void GLock::Unlock()
{
    GASSERT(RecursiveLockCount > 0);
    --RecursiveLockCount;
    if (RecursiveLockCount == 0)
        PerfUnlock();
}


void GLock::PerfUnlock()
{   
    //_WriteBarrier();    

    // We need to use release semantics here, however calling 'Store_Release'
    // on x86 is slow here because it uses 'xchg'. It is performance critical
    // to perform an unlock through a regular assignment, even though if this
    // was non-x86 we would've used LockedThreadId.Store_Release(0).
    // TBD: Details of Release effect on X86 need to be researched, to see
    // where xchg is actually necessary.            
    LockedThreadId.Value = 0;    

    // AFTER it is released we check if there're waiters.
    if (WaiterCount.Load_Acquire() > 0)
    {        
        WaiterMinus();

        GASSERT(hSemaphore);
        GASSERT(ReleaseSemaphore(hSemaphore, 1, NULL) != 0);
    }
}

void    GLock::PerfLock(DWORD threadId)
{
    // Attempt spin-lock
    for (UInt spin = 0; spin < SpinMax; spin++)
    {
        if (PerfLockImmediate(threadId))
            return;

#ifdef GFC_CPU_X86
        GASM { pause };
#endif        
        //YieldProcessor();
    }

    // Ensure we have the kernel event created
    AllocateKernelSemaphore();

    // Do a kernel lock with potential semaphore wait.
    while (1)
    {        
        WaiterPlus();
        if (PerfLockImmediate(threadId))
            break;
        GASSERT(WaitForSingleObject(hSemaphore, INFINITE) == WAIT_OBJECT_0);
    }

    WaiterMinus();    
}


// Utility functions to access system Info & Init semaphore.

static UInt SpinMax_ProcessorCount = 0xFFFFFFFF;

void GLock::SetSpinMax(UInt spinMax)
{
    if (SpinMax_ProcessorCount == 0xFFFFFFFF)
    {
        SYSTEM_INFO stSI;
        GetSystemInfo(&stSI);
        SpinMax_ProcessorCount = (UInt) stSI.dwNumberOfProcessors;
    }
    
    SpinMax = (SpinMax_ProcessorCount > 1) ? spinMax : 0;
}

void    GLock::AllocateKernelSemaphore()
{
    if (!hSemaphore)
    {
        HANDLE hnewSemaphore = CreateSemaphore(NULL, 0, 0x7FFFFFFF, NULL);
        GASSERT(hnewSemaphore != 0);
        if (InterlockedCompareExchangePointer(&hSemaphore, hnewSemaphore, NULL))
            CloseHandle(hnewSemaphore); // we're late
    }
}

#endif

#endif

#endif // GFC_NO_THREADSUPPORT
