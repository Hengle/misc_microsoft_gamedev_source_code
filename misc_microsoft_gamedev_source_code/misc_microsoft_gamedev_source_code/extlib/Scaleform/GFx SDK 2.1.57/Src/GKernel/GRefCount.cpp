/**********************************************************************

Filename    :   GRefCount.cpp
Content     :   Reference counting implementation
Created     :   January 14, 1999
Authors     :   Michael Antonov
Notes       :

History     :   

Copyright   :   (c) 1999-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GRefCount.h"
#include "GTLTypes.h"


#ifndef GFC_NO_THREADSUPPORT
#include "GAtomic.h"
#endif

// ***** Reference Count Base implementation

// ** Ref-Counting delegated functions

// Functions for nothing
void GFASTCALL AddRef_Null(GRefCountBaseImpl *pbase)
{   
    GUNUSED(pbase);
}
void GFASTCALL Release_Null(GRefCountBaseImpl *pbase, UInt flags)
{
    GUNUSED(pbase);
    GUNUSED(flags);
}

static GRefCountImpl RefCountImpl_Null =
{
    &AddRef_Null,
    &Release_Null
};

//Functions for reference counting
void GFASTCALL AddRef_Normal(GRefCountBaseImpl *pbase)
{
    pbase->RefCount++;
}
void GFASTCALL Release_Normal(GRefCountBaseImpl *pbase, UInt flags)
{
    GUNUSED(flags);
    pbase->RefCount--;
    if (pbase->RefCount==0)     
        delete pbase;       
}

static GRefCountImpl RefCountImpl_Normal =
{
    &AddRef_Normal,
    &Release_Normal
};


#ifndef GFC_NO_THREADSUPPORT
// Functions for thread-safe reference counting
void GFASTCALL AddRef_ThreadSafe(GRefCountBaseImpl *pbase)
{
    GAtomicOps<SInt>::ExchangeAdd_NoSync(&pbase->RefCount, 1);
}
void GFASTCALL Release_ThreadSafe(GRefCountBaseImpl *pbase, UInt flags)
{
    GUNUSED(flags);
    if ((GAtomicOps<SInt>::ExchangeAdd_NoSync(&pbase->RefCount, -1) - 1) == 0)
        delete pbase;
}

static GRefCountImpl RefCountImpl_ThreadSafe =
{
    &AddRef_ThreadSafe,
    &Release_ThreadSafe
};
#endif



// Constructor
// Does some custom processing to initialize the base
GRefCountBaseImpl::GRefCountBaseImpl()
{
    pWeakProxy = 0;
    // Reference counts one, no info by default
    if ( (RefCount==GFC_REFCOUNT_KEY1) && (UPInt(pRefCountImpl)==GFC_REFCOUNT_KEY2) )
    {
        RefCount = 1;
        pRefCountImpl = &RefCountImpl_Normal;
    }
    else
    {
        RefCount = 0;
        pRefCountImpl = &RefCountImpl_Null;     
    }
}

GRefCountBaseImpl::GRefCountBaseImpl(GRefCountImpl *pNormalImpl, GRefCountImpl *pNullImpl)
{
    pWeakProxy = 0;
    // Reference counts one, no info by default
    if ( (RefCount==GFC_REFCOUNT_KEY1) && (UPInt(pRefCountImpl)==GFC_REFCOUNT_KEY2) )
    {
        RefCount = 1;
        pRefCountImpl = pNormalImpl;
    }
    else
    {
        RefCount = 0;
        pRefCountImpl = pNullImpl ? pNullImpl : &RefCountImpl_Null;
    }
}

GRefCountBaseImpl::~GRefCountBaseImpl()
{
    // This may assert if 'delete' was called on a Ref-Couted object. This is
    // forbidden because Release must be called instead.
    // First generate a warning so that non-source customers have a message.
    GFC_DEBUG_ERROR(RefCount > 0,
        "Object destroyed with non-zero reference count. Did you call delete?");
    GASSERT(RefCount <= 0);

    if (pWeakProxy)
    {
        pWeakProxy->NotifyObjectDied();
        pWeakProxy->Release();
    }
}


bool GRefCountBaseImpl::SetRefCountMode(UInt mode)
{
    // If in null-ref counting mode fail.
    if (pRefCountImpl == &RefCountImpl_Null)
        return 0;

    switch(mode)
    {
        case GFC_REFCOUNT_NORMAL:
            pRefCountImpl = &RefCountImpl_Normal;
            return 1;

        case GFC_REFCOUNT_THREADSAFE:

#ifndef GFC_NO_THREADSUPPORT
            pRefCountImpl = &RefCountImpl_ThreadSafe;
            return 1;
#else
            return 0;
#endif
    }
    return 0;
}


// Create/return create proxy, users must release proxy when no longer needed
GWeakPtrProxy*  GRefCountBaseImpl::CreateWeakProxy() const
{
    if (!pWeakProxy)        
        if ((pWeakProxy = new GWeakPtrProxy)==0)
            return 0;          
    pWeakProxy->AddRef();
    return pWeakProxy;
}
