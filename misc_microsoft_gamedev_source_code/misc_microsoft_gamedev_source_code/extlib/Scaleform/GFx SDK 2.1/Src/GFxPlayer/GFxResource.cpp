/**********************************************************************

Filename    :   GFxResource.cpp
Content     :   Resource and Resource Library implementation for GFx
Created     :   January 11, 2007
Authors     :   Michael Antonov

Notes       :   

Copyright   :   (c) 2005-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include <stdio.h>
#include "GFxResource.h"
#include "GStd.h"


// ***** GFxResource

// Thread-Safe ref-count implementation.
void    GFxResource::AddRef()
{
    RefCount.Increment_NoSync();
  //  SInt32 val = RefCount.ExchangeAdd_NoSync(1);
  //  if (ResType == 100)
  //       printf("Thr %4d, %8x : GFxResource::AddRef (%d -> %d)\n", GetCurrentThreadId(), this, val, val+1);  
}
void    GFxResource::Release()
{
    if ((RefCount.ExchangeAdd_Acquire(-1) - 1) == 0)
    {  
        // Once the resource reaches RefCount of 0 it can no longer be revived
        // by the library (required to allow ops without lock).
        // Ensure removal from library.
        if (pLib)
        {
            pLib->RemoveResourceOnRelease(this);
            pLib = 0;
        }
        delete this;
    }
}

// Increments a reference count if it is not zero; this function is used by
// the library while it is locked.
// Returns 1 if increment was successful, and 0 if RefCount turned to 0.
bool    GFxResource::AddRef_NotZero()
{
    while (1)
    {
        SInt32 refCount = RefCount;
        if (refCount == 0)
            return 0;
        if (RefCount.CompareAndSet_NoSync(refCount, refCount+1))
            break;
    }
    return 1;
}

SInt32  GFxResource::GetRefCount() const
{
    return RefCount;
}


// ***** GFxResourceLib

GFxResourceLib::GFxResourceLib()
{
    SetRefCountMode(GFC_REFCOUNT_THREADSAFE);
    pWeakLib = new GFxResourceWeakLib(this);
}
GFxResourceLib::~GFxResourceLib()
{
    if (pWeakLib)
    {
        pWeakLib->UnpinAll();
        pWeakLib->Release();
    }
}


GFxResourceLib::ResourceSlot::ResourceSlot(GFxResourceWeakLib* plib, const GFxResourceKey& key)
{
    SetRefCountMode(GFC_REFCOUNT_THREADSAFE);
    GASSERT(plib);
    pLib      = plib;
    State     = Resolve_InProgress;
    pResource = 0;
    Key       = key;
}

GFxResourceLib::ResourceSlot::~ResourceSlot()
{
    GLock::Locker lock(&pLib->ResourceLock);

    if (State == Resolve_InProgress)
        pLib->Resources.remove(Key);
    else if (pResource)
        pResource->Release();
}


// *** Interface for Waiter

// If we are not responsible, wait to receive resource.
GFxResource*    GFxResourceLib::ResourceSlot::WaitForResolve()
{            
    // User AddRefs to us through BindHandle, while we AddRef to Lib. 
    
#ifndef GFC_NO_THREADSUPPORT

    // Wait until the resource is available, at which point
    // either pResource or ErrorMessage should be set.
    ResolveComplete.Wait();

    // In case of an error, pResource should be 0. It is safe
    // to use pResource pointer here because Resolve AddRefs
    // to it for us.
    if (pResource)
        pResource->AddRef();
    return pResource;

#else
    // With no thread support, we can never wait. Resource library
    // should always be in Resolved or not state.
    GASSERT(0);
    return 0;
#endif
}


const char*     GFxResourceLib::ResourceSlot::GetError() const
{
    return ErrorMessage.ToCStr();
}

// Check availability.
bool            GFxResourceLib::ResourceSlot::IsResolved() const
{
    GLock::Locker lock(&pLib->ResourceLock);
    return (State != Resolve_InProgress);
}


// If we are responsible call one of these two:
void    GFxResourceLib::ResourceSlot::Resolve(GFxResource* pres)
{
    GLock::Locker lock(&pLib->ResourceLock);

    GASSERT(State == Resolve_InProgress);
    GASSERT(pres);

    // Slot AddRef's to resource to ensure that it doesn't die
    // before BindHandle owning threads get a chance to react to
    // the signaled event (below).
    pres->AddRef();
    pResource = pres;
    State     = Resolve_Success;

    // Obtain the hash node and resolve it.
    GFxResourceWeakLib::ResourceNode* pnode = pLib->Resources.get(Key);
    GASSERT(pnode);
    GASSERT(pnode->Type == GFxResourceWeakLib::ResourceNode::Node_Resolver);

    pnode->pResource = pres;
    pnode->Type      = GFxResourceWeakLib::ResourceNode::Node_Resource;

    pres->SetOwnerResourceLib(pLib);

#ifndef GFC_NO_THREADSUPPORT    
    ResolveComplete.SetEvent();
#endif

}

void    GFxResourceLib::ResourceSlot::CancelResolve(const char* perrorMessage)
{
    GLock::Locker lock(&pLib->ResourceLock);

    GASSERT(State == Resolve_InProgress);    

    // Resolve failed with error.
    State        = Resolve_Fail;
    ErrorMessage = perrorMessage;

    // Remove node from hash.
    GASSERT(pLib->Resources.get(Key) != 0);
    pLib->Resources.remove(Key);

#ifndef GFC_NO_THREADSUPPORT
    ResolveComplete.SetEvent();
#endif
}



// If we are not responsible, wait to receive resource.
GFxResource*    GFxResourceLib::BindHandle::WaitForResolve()
{
    GASSERT(State == RS_WaitingResolve ||
            State == RS_Available ||
            State == RS_Error );

    // If resource already resolved, return appropriate value.
    if (State == RS_Available)
    {
        pResource->AddRef();        
        return pResource;
    }
    else if (State == RS_Error)
    {
        return 0;
    }

#ifndef GFC_NO_THREADSUPPORT

    ResourceSlot*   pslot = pSlot;
    GFxResource*    pres  = pslot->WaitForResolve();
    if (pres)
    {
        State     = RS_Available;
        pResource = pres;
        pResource->AddRef(); // AddRef one more time for ~BindHandle.
        pslot->Release();        

        // NOTE: pslot->WaitForResolve() AddRefs internally,
        // so our return value is also AddRefed.
        return pres;
    }
    
#else
    // WaitForResolve should never be called in RS_WaitingResolve state
    // with no thread support.
    GASSERT(0);
#endif

    State = RS_Error;
    return 0;
}



// ***** GFxResourceWeakLib

GFxResourceWeakLib::GFxResourceWeakLib(GFxResourceLib *pstrongLib)
{
    SetRefCountMode(GFC_REFCOUNT_THREADSAFE);
    // Back-pointer link  to strong lib
    pStrongLib = pstrongLib;
}

GFxResourceWeakLib::~GFxResourceWeakLib()
{
    GLock::Locker lock(&ResourceLock);
    
    // Clear pLib in all resources that are still alive.
    GTL::ghash_set<ResourceNode, ResourceNode::HashOp>::iterator ires;

    for(ires = Resources.begin(); ires != Resources.end(); ++ires)
    {
        // This must be a resource. If this is a resource slot,
        // this means that other threads are still trying to use the
        // library, so they must have AddRef'ed to WeakLib.
        GASSERT(ires->IsResource());
        ires->pResource->SetOwnerResourceLib(0);
    }
}


// Lookup resource, but only if resolved.
// All resources are considered AddRefed.
GFxResource*    GFxResourceWeakLib::GetResource(const GFxResourceKey &k)
{
    GLock::Locker lock(&ResourceLock);

    ResourceNode* pnode = Resources.get(k);
    if (pnode && pnode->IsResource())
    {
        if (pnode->pResource->AddRef_NotZero())
        {
            return pnode->pResource;
        }
    }
    return 0;
}


// Lookup resource and insert its slot. We get back a BindHandle.
//
GFxResourceLib::ResolveState GFxResourceWeakLib::BindResourceKey(GFxResourceLib::BindHandle *phandle, const GFxResourceKey &k)
{
    GASSERT(phandle->State == GFxResourceLib::RS_Unbound);

    GLock::Locker lock(&ResourceLock);

    // Need more locking, etc.
    ResourceNode* pnode = Resources.get(k);

    if (pnode)
    {
        if (pnode->IsResource())
        {
            if (pnode->pResource->AddRef_NotZero())
            {
                // GFxResource AddRefed, can return it.
                phandle->pResource = pnode->pResource;
                phandle->State     = GFxResourceLib::RS_Available;                
                return phandle->State;
            }
            else
            {
                // GFxResource was decremented to 0 during our lock.
                // It can no longer be used and must be removed from list.
                Resources.remove(k);

                // Fall through to create a new slot.
            }
        }
        else
        {
            phandle->pSlot = pnode->pResolver;
            phandle->State = GFxResourceLib::RS_WaitingResolve;
            phandle->pSlot->AddRef();
            return phandle->State;
        }
    }

    // Create a resolve slot
    ResourceSlot* presSlot = new ResourceSlot(this, k);
    if (!presSlot)
        return GFxResourceLib::RS_Error;

    // Add a resource node.
    ResourceNode n;
    n.Type      = ResourceNode::Node_Resolver;
    n.pResolver = presSlot;
    Resources.add(n);

    // Prepare return value.
    phandle->pSlot = presSlot;
    phandle->State = GFxResourceLib::RS_NeedsResolve;
    return phandle->State;
}



// Pin-management: for strong links.

// Returns 1 if resource is pinned in strong library.
bool    GFxResourceWeakLib::IsPinned(GFxResource* pres)
{
    GASSERT(pres->pLib == this);
    GLock::Locker lock(&ResourceLock);
    return (pStrongLib && pStrongLib->PinSet.get(pres) != 0);
}

void    GFxResourceWeakLib::PinResource(GFxResource* pres)
{
    GASSERT(pres->pLib == this);
    GLock::Locker lock(&ResourceLock);

    if (!pStrongLib)
    {
        GFC_DEBUG_WARNING(1, "GFxResource::PinResource failed - strong library not available");
    }
    else
    {
        if (!pStrongLib->PinSet.get(pres))
        {
            pStrongLib->PinSet.add(pres);
            pres->AddRef();
        }
    }
}

void    GFxResourceWeakLib::UnpinResource(GFxResource* pres)
{
    GASSERT(pres->pLib == this);
    GLock::Locker lock(&ResourceLock);

    if (pStrongLib)
    {
        pStrongLib->PinSet.remove(pres);
        pres->Release();
    }
}


void    GFxResourceWeakLib::UnpinAll()
{            
    GLock::Locker lock(&ResourceLock);

    if (pStrongLib)
    {
        // Release all references held by pins.
        GFxResourceLib::PinHashSet::iterator ihash = pStrongLib->PinSet.begin();
        while(ihash != pStrongLib->PinSet.end())
        {
            // Hopefully we support recursive locks.. :)
            (*ihash)->Release();
            ++ihash;
        }
        pStrongLib->PinSet.clear();
    }
}


// Virtual function called when resource RefCount has reached 0
// and the resource is about to die.
void    GFxResourceWeakLib::RemoveResourceOnRelease(GFxResource *pres)
{
    GFxResource *p = static_cast<GFxResource*>(pres);

    GLock::Locker lock(&ResourceLock);
    GASSERT(pres->GetRefCount() == 0);

    // GFxResource may have been removed or substituted by a library call.
    ResourceNode* pnode = Resources.get(p->GetKey());
    if (pnode &&
        pnode->Type == ResourceNode::Node_Resource &&
        pnode->pResource == pres)
    {
        Resources.remove(p->GetKey());
    }
}

// ***** GFxResourceId
UInt GFxResourceId::GenerateIdString(char* pbuffer, size_t bufferSize) const
{
    GASSERT(bufferSize >= 9);
    switch(GetIdType())
    {
    case IdType_GradientImage:
        *pbuffer++ = 'G';
        break;
    case IdType_DynFontImage:
    case IdType_FontImage:
        *pbuffer++ = 'F';
        break;
    default:
        *pbuffer++ = 'I';
        break;
    }
    return (UInt)gfc_sprintf(pbuffer, bufferSize, "%X", GetIdIndex());
}



