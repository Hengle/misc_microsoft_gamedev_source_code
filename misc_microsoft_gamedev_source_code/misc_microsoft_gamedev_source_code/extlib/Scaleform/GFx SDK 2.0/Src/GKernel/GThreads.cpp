/**********************************************************************

Filename    :   GThreads.cpp
Content     :   System-independent thread-related (safe) functionality
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

#define GTHREADS_CPP

#include "GMemory.h"
#include "GTimer.h"


// ***** Classes implemented

class   GWaitable;
class   GAcquireInterface;
class   GEvent;
class   GSemaphore;


// ***** GWaitable interface implementation

bool    GWaitable::AddWaitHandler(WaitHandler handler, Handle pdata)
{
    HandlerStruct   hs(handler, pdata);
    GLock::Locker   lock(&HandlersLock);    
    Handlers.push_back(hs); 
    return 1;
}

bool    GWaitable::RemoveWaitHandler(WaitHandler handler, Handle pdata)
{
    HandlerStruct   hs(handler, pdata);
    bool            result = 0;
    GLock::Locker   lock(&HandlersLock);   

    // Find and erase the element    
    for(UInt i=0; i<Handlers.size(); i++)
        if (Handlers[i] == hs)
        {
            Handlers.remove(i);
            result = 1;
            break;
        }   
    return result;
}

void    GWaitable::CallWaitHandlers()
{
    GLock::Locker lock(&HandlersLock);
    UInt size = (UInt)Handlers.size();

    // No items... nothing to call
    if (size == 0)    
        return;    
    
    if (size == 1)
    {
        HandlerStruct hs(Handlers[0]);
        // Just one handler to call
        Handlers[0].Handler(this, Handlers[0].pUserData);
    }
    else
    {
        // Have to copy the handlers
        GTL::garray<HandlerStruct>  HandlersCopy(Handlers);
        // call each one of the handlers
        for(UInt i=0; i<HandlersCopy.size(); i++)
            HandlersCopy[i].Handler(this, HandlersCopy[i].pUserData);
    }
}

// Acquires the current object based on associated acquisition interface
bool    GWaitable::Acquire(UInt delay)
{
    GWaitable*  pw = this;
    return GAcquireInterface::AcquireOneOfMultipleObjects(&pw, 1, delay) == 0;
}


// Handler used to wait for a single object to become signaled
void    GWaitable_SingleWaitHandler(GWaitable* pwaitable, Handle pdata)
{
    if (pwaitable->IsSignaled())        
        ((GEvent*)pdata)->PulseEvent();
}


// Wait for the current object to be signaled
bool    GWaitable::Wait(UInt delay)
{
    if (IsSignaled())
        return 1;
    if (delay == 0)
        return 0;

    // Event that we will be waiting for
    GEvent  event;
    // Install handler
    if (!AddWaitHandler(GWaitable_SingleWaitHandler, &event))
        return 0;

    // What about atomic transition from check to wait ?? Is it necessary here?

    // Wait for the event
    // We need to time the delays in between the functions so the wait is correct
    bool        result = 0;
    UInt64      dt = 0;
    UInt        adjustedDelay = delay;
    if (delay != GFC_WAIT_INFINITE)
        dt = GTimer::GetTicks();

    // Wait on the event
    while (event.Wait(adjustedDelay))
    {   
        // Acquire all, if possible
        if (IsSignaled())
        {
            result = 1;
            break;
        }

        // Failed to acquire; adjust the delay and keep waiting
        if (delay != GFC_WAIT_INFINITE)
        {           
            UInt delta = (UInt)(GTimer::GetTicks() - dt);
            // If time has passed, fail
            if (delta >= delay)
                break;
            adjustedDelay = delay - delta;
        }   
    }

    // Remove the handler
    RemoveWaitHandler(GWaitable_SingleWaitHandler, &event);
    return result;
}

// Always signaled by default
bool    GWaitable::IsSignaled() const
{
    return 1;
}

// Just return an acquire interface that is always true
GAcquireInterface*  GWaitable::GetAcquireInterface()
{
    return GDefaultAcquireInterface::GetDefaultAcquireInterface();
}



// *** Acquire Interface implementation


GDefaultAcquireInterface* GDefaultAcquireInterface::GetDefaultAcquireInterface()
{
    static GDefaultAcquireInterface di;
    return &di;
}


bool    GAcquireInterface::CanAcquire()
{
    // By default, the resource can always be acquired
    return 1;
}
    
// Try to acquire the resource, return 1 if succeeded.
bool    GAcquireInterface::TryAcquire()
{
    // Default acquire does nothing, so it always succeeds
    return 1;
}
// Complete resource acquisition. Should not fail in general.
bool    GAcquireInterface::TryAcquireCommit()
{
    return 1;
}
// Cancel resource acquisition. Should not fail in general.
bool    GAcquireInterface::TryAcquireCancel()
{
    return 1;
}



struct GAcquireInterface_AcquireDesc
{
    // Objects waiting
    GWaitable**             pWaitList;
    UInt                    WaitCount;
    // Event to signal
    GEvent*                 pEvent;
    // Acquire list, if any
    GAcquireInterface**     pAcquireList;
    
    // Simple constructor
    GAcquireInterface_AcquireDesc(GWaitable** pwaitList, UInt waitCount, GEvent *pevent, GAcquireInterface** pacquireList = 0)
    {
        pWaitList   = pwaitList;
        WaitCount   = waitCount;
        pEvent      = pevent;       
        pAcquireList = pacquireList;
    }

    // Handler add/remove functions
    bool    AddHandlers(GWaitable::WaitHandler handler);
    void    RemoveHandlers(GWaitable::WaitHandler handler);
};

// Structure that incorporates a simple acquire list
struct GAcquireInterface_AcquireList
{
    GAcquireInterface** pList;
    GAcquireInterface*  StaticList[32];

    GAcquireInterface_AcquireList(GWaitable** pwaitList, UInt waitCount)
    {       
        // Allocate & fill in acquire list
        pList = (waitCount>32) ? (GAcquireInterface**) GALLOC(sizeof(GAcquireInterface*) * waitCount) : StaticList; 
        for(UInt i=0; i<waitCount; i++)
            pList[i] = pwaitList[i]->GetAcquireInterface();
    }

    ~GAcquireInterface_AcquireList()
    {
        if (pList != StaticList)
            GFREE(pList);
    }
};




bool    GAcquireInterface_AcquireDesc::AddHandlers(GWaitable::WaitHandler handler)
{
    // Install handlers on all waitable objects
    for(UInt i=0; i<WaitCount; i++)
    {
        // Add a handler
        if (!pWaitList[i]->AddWaitHandler(handler, this))
        {
            while(i > 0)
                pWaitList[--i]->RemoveWaitHandler(handler, this);
            return 0;
        }
    }
    return 1;
}

void    GAcquireInterface_AcquireDesc::RemoveHandlers(GWaitable::WaitHandler handler)
{
    // Remove handlers from all waitable objects
    for(UInt i=0; i<WaitCount; i++)
        pWaitList[i]->RemoveWaitHandler(handler, this);
}


void GAcquireInterface_MultipleWaitHandler(GWaitable *pwaitable, Handle pdata)
{
    GUNUSED(pwaitable);
    GAcquireInterface_AcquireDesc* pdesc = (GAcquireInterface_AcquireDesc*)pdata;
    
    // If at least a single object cannot be acquired, fail
    for(UInt i=0; i<pdesc->WaitCount; i++)
        if (!pdesc->pAcquireList[i]->CanAcquire())
            return;
    // All resources can be acquired, signal the event
    pdesc->pEvent->PulseEvent();
}

bool GAcquireInterface_TryAcquireAll(GAcquireInterface** pacquireList, UInt waitCount)
{
    UInt    i;
    // First try to acquire all objects, see if it succeeds
    for(i=0; i<waitCount; i++)
        if (!pacquireList[i]->TryAcquire())
            break;
    // If acquisition succeeded, commit and we are done
    if (i == waitCount)
    {
        for(i=0; i<waitCount; i++)
            pacquireList[i]->TryAcquireCommit();
        return 1;
    }
    else
    {
        // Otherwise cancel all acquisitions that worked
        while(i > 0)
            pacquireList[--i]->TryAcquireCancel();
    }
    return 0;
}

// Static function to acquire multiple objects simultaneously
bool    GAcquireInterface::AcquireMultipleObjects(GWaitable** pwaitList, UInt waitCount, UInt delay)
{   
    GAcquireInterface_AcquireList   acquireList(pwaitList, waitCount);
    if (GAcquireInterface_TryAcquireAll(acquireList.pList, waitCount))
        return 1;
    // If delay was 0, just fail
    if (delay == 0)
        return 0;

    // Create a event & acquire descriptor
    GEvent                      event;
    GAcquireInterface_AcquireDesc   acquireDesc(pwaitList, waitCount, &event, acquireList.pList);   

    // Install handlers on all waitable objects
    if (!acquireDesc.AddHandlers(GAcquireInterface_MultipleWaitHandler))
        return 0;
    
    // We need to time the delays in between the functions so the wait is correct
    bool        result = 0;
    UInt64      dt = 0;
    UInt        adjustedDelay = delay;
    if (delay != GFC_WAIT_INFINITE)
        dt = GTimer::GetTicks();

    // Wait on the event
    while (event.Wait(adjustedDelay))
    {   
        // Acquire all, if possible
        if (GAcquireInterface_TryAcquireAll(acquireList.pList, waitCount))
        {
            result = 1;
            break;
        }

        // Failed to acquire; adjust the delay and keep waiting
        if (delay != GFC_WAIT_INFINITE)
        {           
            UInt delta = (UInt)(GTimer::GetTicks() - dt);
            // If time has passed, fail
            if (delta >= delay)
                break;
            adjustedDelay = delay - delta;
        }   
    }

    // Remove the wait handlers, done
    acquireDesc.RemoveHandlers(GAcquireInterface_MultipleWaitHandler);      
    return result;
}


// Wait handler for only one of multiple objects to acquire
void GAcquireInterface_OneOfMultipleWaitHandler(GWaitable *pwaitable, Handle pdata)
{
    GAcquireInterface_AcquireDesc* pdesc = (GAcquireInterface_AcquireDesc*)pdata;

    // If at least a single object can be acquired, signal the event
    if (pwaitable->GetAcquireInterface()->CanAcquire())
        pdesc->pEvent->PulseEvent();
    else
    {       
        for(UInt i=0; i<pdesc->WaitCount; i++)
            if (pdesc->pWaitList[i]->GetAcquireInterface()->CanAcquire())
            {       
                pdesc->pEvent->PulseEvent();
                break;
            }   
    }
}

// Function to try acquire one of the objects in the least
SInt GAcquireInterface_TryAcquireOne(GWaitable** pwaitList, UInt waitCount)
{
    UInt    i;
    // First try to at least one object
    for(i=0; i<waitCount; i++)
    {
        GAcquireInterface* pi = pwaitList[i]->GetAcquireInterface();
        if (pi->TryAcquire())
        {
            pi->TryAcquireCommit();
            // Return the index acquired
            return (SInt) i;
        }
    }   
    // Return -1 for fail
    return -1;
}


// Static function to acquire one object out of a list
SInt    GAcquireInterface::AcquireOneOfMultipleObjects(GWaitable** pwaitList, UInt waitCount, UInt delay)
{
    SInt    result = GAcquireInterface_TryAcquireOne(pwaitList, waitCount);
    
    // If acquired, we are done
    if (result !=-1)
        return result;
    // If delay was 0, just fail
    if (delay == 0)
        return -1;

    // Create a event & acquire descriptor
    GEvent                      event;
    GAcquireInterface_AcquireDesc   acquireDesc(pwaitList, waitCount, &event);  
    // Install handlers on all waitable objects
    if (!acquireDesc.AddHandlers(GAcquireInterface_OneOfMultipleWaitHandler))
        return 0;

    // We need to time the delays in between the functions so the wait is correct   
    UInt64      dt = 0;
    UInt        adjustedDelay = delay;
    if (delay != GFC_WAIT_INFINITE)
        dt = GTimer::GetTicks();

    // Wait on the event
    if ((result = GAcquireInterface_TryAcquireOne(pwaitList, waitCount)) == -1)
        while (event.Wait(adjustedDelay))
        {   
            // Acquire one, if possible
            if ((result = GAcquireInterface_TryAcquireOne(pwaitList, waitCount)) != -1)
                break;

            // Failed to acquire; adjust the delay and keep waiting
            if (delay != GFC_WAIT_INFINITE)
            {
                // Milliseconds that already passed
                UInt delta = (UInt)(GTimer::GetTicks() - dt);
                // If time has passed, fail
                if (delta >= delay)
                    break;
                adjustedDelay = delay - delta;
            }   
        }

    // Remove the wait handlers
    acquireDesc.RemoveHandlers(GAcquireInterface_OneOfMultipleWaitHandler);
    return result;
}




// ***** GEvent implementation

// Constructor/destructor
GEvent::GEvent(bool setInitially)
{
    SetRefCountMode(GFC_REFCOUNT_THREADSAFE);
    State   = setInitially;
    Temporary   = 0;
}

GEvent::~GEvent()
{
}

// Wait on an event condition until it is set
bool    GEvent::Wait(UInt delay)
{
    GMutex::Locker lock(&StateMutex);   

    // Do the correct amount of waiting
    switch(delay)
    {   
        case 0:
            break;

        case GFC_WAIT_INFINITE:
            while(!State)
                StateWaitCondition.Wait(&StateMutex);
            break;      

        default:
            if (!State)
                StateWaitCondition.Wait(&StateMutex, delay);
            break;
    }
    
    bool state = State;
    // Take care of temporary 'pulsing' of a state
    if (Temporary)
    {
        Temporary   = 0;
        State       = 0;
    }
    return state;
}

    
// Set en event, releasing objects waiting on it
void    GEvent::SetEvent()
{
    StateMutex.Lock();
    State       = 1;
    Temporary   = 0;
    StateWaitCondition.NotifyAll();
    StateMutex.Unlock();
    CallWaitHandlers(); 
}

// Reset an event, un-signaling it
void    GEvent::ResetEvent()
{
    GMutex::Locker lock(&StateMutex);
    State       = 0;
    Temporary   = 0;    
}

// Set and then reset an event once a waiter is released, will release only one waiter
// If threads are already waiting, one will be notified and released
// If threads are not waiting, the event is set until the first thread comes in
void    GEvent::PulseEvent()
{
    StateMutex.Lock();
    State       = 1;
    Temporary   = 1;
    StateWaitCondition.Notify();
    StateMutex.Unlock();
    CallWaitHandlers(); 
}

// Signaled override
bool    GEvent::IsSignaled() const
{
    //GMutex::Locker lock(&StateMutex);
    return State;
}

GAcquireInterface* GEvent::GetAcquireInterface()
{
    return this;
}

// Acquire interface implementation 
bool    GEvent::CanAcquire()
{
    return IsSignaled();
}
bool    GEvent::TryAcquire()
{
    return IsSignaled();
}
bool    GEvent::TryAcquireCommit()
{
    GMutex::Locker lock(&StateMutex);
    if (Temporary)
    {
        Temporary   = 0;
        State       = 0;
    }
    return 1;
}
bool    GEvent::TryAcquireCancel()
{
    return 1;
}



// ***** GSemaphore implementation

GSemaphore::GSemaphore(SInt maxValue)
{
    SetRefCountMode(GFC_REFCOUNT_THREADSAFE);
    MaxValue = maxValue;
    Value    = 0;
}

GSemaphore::~GSemaphore()
{
}
    
// Get current value and max
SInt        GSemaphore::GetMaxValue() const
{
    return MaxValue;
}
SInt        GSemaphore::GetValue() const
{   
    return Value;
}
SInt        GSemaphore::GetAvailable() const
{   
    return MaxValue - Value;
}
    
// *** Actions

// Obtains multiple value of a semaphore
// Returns 0 if query failed (count > max value error or timeout)
bool        GSemaphore::ObtainSemaphore(SInt count, UInt delay)
{
    if (count > MaxValue)
        return 0;

    GMutex::Locker  lock(&ValueMutex);
    
    // If value in range, done
    if ((Value + count) <= MaxValue)
    {
        Value += count;
        return 1;
    }

    // If delay == 0, return immediately
    if (delay == 0)
        return 0;

    // If infinite wait, special loop
    if (delay == GFC_WAIT_INFINITE)
    {
        while ((Value + count) > MaxValue)
            ValueWaitCondition.Wait(&ValueMutex);
        Value += count;
        return 1;
    }
    
    // More complicated version of wait needs to take time into account
    UInt        adjustedDelay = delay;
    UInt64      dt = GTimer::GetTicks();
    
    while(ValueWaitCondition.Wait(&ValueMutex, adjustedDelay))
    {
        // If value in range, done
        if ((Value + count) <= MaxValue)
        {
            Value += count;
            return 1;
        }

        UInt delta = (UInt)(GTimer::GetTicks() - dt);
        // If time has passed, fail
        if (delta >= delay)
            break;
        adjustedDelay = delay - delta;      
    }
    
    // Time-out occurred
    return 0;
}


// Release semaphore values
// Returns success code
bool        GSemaphore::ReleaseSemaphore(SInt count)
{
    if (count == 0)
        return 1;

    ValueMutex.Lock();
    
    // Always assign final result
    // If available() is called, it does not maker whether we get the old or new
    // value, but we should never get an intermediate state (such as a negative value)
    if ((Value - count) >= 0)
        Value = Value - count;      
    else
    {
        // Value decremented too far!!
        Value = 0;
    }           

    if (count == 1)
        ValueWaitCondition.Notify();
    else
        ValueWaitCondition.NotifyAll();
    ValueMutex.Unlock();
    CallWaitHandlers();
    return 1;   
}


// *** Operators

// Postfix increment/decrement, return value before operation
SInt        GSemaphore::operator ++ (SInt)
{
    GMutex::Locker  lock(&ValueMutex);  
    // Wait for the value available
    while (Value >= MaxValue)
        ValueWaitCondition.Wait(&ValueMutex);
    Value++;
    return Value;
}

SInt        GSemaphore::operator -- (SInt)
{
    ValueMutex.Lock();
    if (Value > 0)
        Value--;    
    ValueWaitCondition.Notify();    
    ValueMutex.Unlock();    
    CallWaitHandlers();
    return Value;
}

// Postfix increment/decrement, return value before operation
SInt        GSemaphore::operator += (SInt count)
{
    GMutex::Locker  lock(&ValueMutex);  
    // Wait for the value available
    while ((Value + count) > MaxValue)
        ValueWaitCondition.Wait(&ValueMutex);
    Value += count;
    return Value;
}

SInt        GSemaphore::operator -= (SInt count)
{
    ValueMutex.Lock();

    if ((Value - count) >= 0)
        Value = Value - count;      
    else        
        Value = 0;
        
    ValueWaitCondition.NotifyAll();
    ValueMutex.Unlock();
    CallWaitHandlers();
    return Value;
}

// Acquire interface implementation
// Default
bool        GSemaphore::CanAcquire()
    { return GetAvailable() > 0; }
bool        GSemaphore::TryAcquire()
    { return ObtainSemaphore(1, 0); }
bool        GSemaphore::TryAcquireCommit()
    { return 1; }
bool        GSemaphore::TryAcquireCancel()
    { return ReleaseSemaphore(1); }


// GWaitable implementation
bool            GSemaphore::IsSignaled() const
    { return GetAvailable() > 0; }
GAcquireInterface*  GSemaphore::GetAcquireInterface()
    { return this; }



// Waitable increment semaphore acquire interface
// Can be used to acquire multiple values of a semaphore at the same time
class GSemaphoreWaitableIncrement : public GWaitable, public GAcquireInterface
{
    GSemaphore *pSemaphore;
    SInt        Count;

public:
    // Constructor/Destructor
    GSemaphoreWaitableIncrement()
        { pSemaphore = 0; Count = 0; }
    GSemaphoreWaitableIncrement(GSemaphore *psemaphore, SInt count);
    ~GSemaphoreWaitableIncrement();


    // Acquire interface implementation
    // Default
    virtual bool                CanAcquire();
    virtual bool                TryAcquire();
    virtual bool                TryAcquireCommit();
    virtual bool                TryAcquireCancel();

    // GWaitable implementation
    virtual bool                IsSignaled() const; 
    virtual GAcquireInterface*  GetAcquireInterface();

    // GInterface - no implementation
    virtual void        AddRef()                            { }
    virtual void        Release(UInt flags=0)               { GUNUSED(flags); }
    virtual bool        SetRefCountMode(UInt refCountFlags) { GUNUSED(refCountFlags); return 1; }
};

// Wait handler install in the owned semaphore in order to notify us
void GSemaphoreWaitableIncrement_SemaphoreWaitHandler(GWaitable *pw, Handle hdata)
{
    GUNUSED(pw);
    GWaitable* pincrement = (GWaitable*) hdata;
    pincrement->CallWaitHandlers();
}

GSemaphoreWaitableIncrement::GSemaphoreWaitableIncrement(GSemaphore *psemaphore, SInt count)
{
    pSemaphore  = psemaphore;
    Count       = count;
    // Install a handler so that we get informed about semaphore state changes
    pSemaphore->AddWaitHandler(GSemaphoreWaitableIncrement_SemaphoreWaitHandler, this);
}
GSemaphoreWaitableIncrement::~GSemaphoreWaitableIncrement()
{
    pSemaphore->RemoveWaitHandler(GSemaphoreWaitableIncrement_SemaphoreWaitHandler, this);
}

// Acquisition interface
bool        GSemaphoreWaitableIncrement::CanAcquire()
{
    if (!pSemaphore) return 0;
    return pSemaphore->GetAvailable() >= Count;
}
bool        GSemaphoreWaitableIncrement::TryAcquire()
{
    if (!pSemaphore) return 0;
    return pSemaphore->ObtainSemaphore(Count, 0);
}
bool        GSemaphoreWaitableIncrement::TryAcquireCommit()
{
    if (!pSemaphore) return 0;
    return 1;
}
bool        GSemaphoreWaitableIncrement::TryAcquireCancel()
{
    if (!pSemaphore) return 0;
    return pSemaphore->ReleaseSemaphore(Count);
}

// GWaitable implementation
bool            GSemaphoreWaitableIncrement::IsSignaled() const
    { if (!pSemaphore) return 0;    
      return pSemaphore->GetAvailable() >= Count; }
GAcquireInterface*  GSemaphoreWaitableIncrement::GetAcquireInterface()
    { return this; }



// Create a semaphore acquisition object that would increment a semaphore by a user-defined count
// This object can be passed to AcquireMultipleObjects functions, and will acquire several values form a semaphore
// This object must be released before the semaphore
GWaitable*      GSemaphore::CreateWaitableIncrement(SInt count)
{
    return new GSemaphoreWaitableIncrement(this, count);
}


#endif  // GFC_NO_THREADSUPPORT
