/**********************************************************************

Filename    :   GFxResourceHandle.h
Content     :   Resource handle and resource binding support for GFx
Created     :   February 8, 2007
Authors     :   Michael Antonov

Notes       :   

Copyright   :   (c) 2005-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFXRESOURCEHANDLE_H
#define INC_GFXRESOURCEHANDLE_H

#include "GFxResource.h"


// ***** Classes Declared

class GFxResourceHandle;

struct GFxResourceBindData;
class GFxResourceBinding;
class GFxResourceData;


// ****** Resource Handle

/*
    GFxResourceHandle represents a handle to a GFxResource that can be
    represented either as a pointer or an index in a GFxResourceBinding
    table. An index is commonly used when resources which are referenced
    by a GFxMovieDataDef are bound to potentially different files in
    different GFxMovieDefImpl instances. Keeping an index allows the
    common loaded data to be shared among differently-bound implementations.

    In order to reference GFxResourceHandle's GFxResource we need to
    resolve it by using GetResource or GetResourceAndBinding methods,
    which rely on a binding table. The binding table itself is stored
    as a part of GFxMovieDefImpl.
*/

class GFxResourceHandle
{
public:
    enum HandleType
    {
        RH_Pointer,
        RH_Index
    };

protected:    
    HandleType  HType;
    union
    {
        UInt            BindIndex;
        GFxResource*    pResource;
    };

public:

    GFxResourceHandle()
    {
        HType       = RH_Pointer;
        pResource   = 0;
    }    
    
    explicit GFxResourceHandle(GFxResource* presource)
    {
        HType       = RH_Pointer;
        pResource   = presource;
        if (presource)
            presource->AddRef();
    }

    GFxResourceHandle(HandleType typeIndex, UInt index)
    {
        GUNUSED(typeIndex);
        GASSERT(typeIndex == RH_Index);
        HType =     RH_Index;
        BindIndex = index;
    }
    
    GFxResourceHandle(const GFxResourceHandle& src)
    {
        HType       = src.HType;
        pResource   = src.pResource;
        if ((HType == RH_Pointer) && pResource)
            pResource->AddRef();
    }

    ~GFxResourceHandle()
    {
        if ((HType == RH_Pointer) && pResource)
            pResource->Release();
    }

    GFxResourceHandle& operator = (const GFxResourceHandle& src)
    {
        if ((src.HType == RH_Pointer) && src.pResource)
            src.pResource->AddRef();
        if ((HType == RH_Pointer) && pResource)
            pResource->Release();
        HType       = src.HType;
        pResource   = src.pResource;
        return *this;
    }

    // Determines similarity of handles. In general, this is exceprt as operator ==,
    // however, due to indexing pointed-to results might be different if they are
    // in a logically different scope. For this reason we made a custom function.
    inline bool         Equals(const GFxResourceHandle &other) const
    {
        return (HType == other.HType) && (pResource == other.pResource);
    }

    // A handle/resource pointer is considered null iff (1) it has a pointer
    // type and (2) its pointer value is null.
    inline bool         IsNull() const          { return (HType == RH_Pointer) && (pResource == 0); }

    inline bool         IsIndex() const         { return (HType == RH_Index); }
    inline UInt         GetBindIndex() const    { GASSERT(IsIndex()); return BindIndex; }
    // Obtains resource pointer from handle, but only if types is pointer.
    inline GFxResource* GetResourcePtr() const { return (HType == RH_Pointer) ? pResource : 0; };
    // Obtains a pointer to the resource; can return a null pointer if binding failed or handle is null.
    inline GFxResource* GetResource(const GFxResourceBinding* pbindings) const;
    // Obtains a resource and its relevant binding, which may be different from a passed one.
    inline GFxResource* GetResourceAndBinding(GFxResourceBinding* pbindings, GFxResourceBinding** presBinding) const;

};

template<class C>
class GFxResourcePtr : public GFxResourceHandle
{    
public:

    GFxResourcePtr() : GFxResourceHandle() { }
    GFxResourcePtr(C* pres) : GFxResourceHandle(pres) { }
    GFxResourcePtr(const GFxResourcePtr& src) : GFxResourceHandle(src) { }
    
    GFxResourcePtr<C>& operator = (const GFxResourcePtr<C>& other)
    {
        *((GFxResourceHandle*)this) = (const GFxResourceHandle&)other;
        return *this;
    }

    GFxResourcePtr<C>& operator = (C* pres)
    {
        if (pres)
            pres->AddRef();
        if ((HType == RH_Pointer) && pResource)
            pResource->Release();
        HType       = RH_Pointer;
        pResource   = pres;
        return *this;
    }

    bool operator == (const GFxResourcePtr<C>& other) const
    {
        return ((HType == other.HType) && (pResource == other.pResource));
    }
    bool operator != (const GFxResourcePtr<C>& other) const
    {
        return !operator == (other);
    }
    
    // Sets a value from handle WITHOUT doing type check.
    void SetFromHandle(const GFxResourceHandle& src)
    {
        *((GFxResourceHandle*)this) = src;
    }
};



// For each resource, we store the Bindings in the context of which
// it is expected to work. In most cases, pBindings will be the same
// as 'this' class, however, it can be different for resources coming
// from imports.
struct GFxResourceBindData
{
    // We need to AddRef to resource because this may be the only place
    // where smart pointers to bound images and fonts are stored.
    // (Note that for imports AddRef is redundant but we do it anyway).
    GPtr<GFxResource>      pResource;
    GFxResourceBinding*    pBinding;

    GFxResourceBindData()
    {
        pBinding = 0;
    }
    GFxResourceBindData(const GFxResourceBindData& src)
    {
        pResource = src.pResource;
        pBinding  = src.pBinding;
    }

    GFxResourceBindData& operator = (const GFxResourceBindData& src)
    {
        pResource = src.pResource;
        pBinding  = src.pBinding;
        return *this;
    }
};



class GFxResourceBinding
{
    // Forward declaration for our owner GFxMovieDefImpl.
    friend class GFxMovieDefImpl;

private:
    
    volatile UInt                      ResourceCount;
    GFxResourceBindData* volatile      pResources;

    // We use lock for now - it can be optimized with recount-based
    // lock free algorithm later.
    mutable GLock                      ResourceLock;

    // If this flag is set, locking is not necessary on read since
    // resource is now real-only. This allows for more efficient
    // resource look up after loading has finished.
    volatile bool                      Frozen;

    // TBD: Another option: Lock and copy list in the beginning of Advance?

    // GFxMovieDefImpl that owns us. Since bindings are created only
    // in GFxMovieDefImpls, we store a pointer to it here.    
    class GFxMovieDefImpl*             pOwnerDefImpl;

    // It would be better to pass this in constructor, but doing so generates
    // a warning in GFxMovieDefImpl initializer list.
    void SetOwnerDefImpl(class GFxMovieDefImpl* powner) { pOwnerDefImpl = powner; }

public:

    GFxResourceBinding();
    ~GFxResourceBinding();

    // Called to destroy GFxResourceBinding table contents, releasing
    // its references, before destructor.
    void            Destroy();


    class GFxMovieDefImpl* GetOwnerDefImpl() const { return pOwnerDefImpl; }
    
    // TBD: Technically binding array can grow as Bind catches up
    // with Read. Therefore, we might need to deal with some threading
    // considerations here.
    void            SetBindData(UInt index, const GFxResourceBindData &bd);

    // Locked version GetResourceData. 
    void            GetResourceData_Locked(GFxResourceBindData *pdata, UInt index) const;
    

    // Inline version of GetResourceData for fast access to frozen resources.
    // Non-frozen binding tables are more expensive to access since they require a lock.
    // NOTE that returned GFxResourceBindData CAN be null; this can happen if an import
    // failed to load and this left an initialized binding slot.
    inline void     GetResourceData(GFxResourceBindData *pdata, UInt index) const
    {
        // For inline compactness, we only handle success of bound check here;
        // failure will be assigned correctly in GetResourceData_Locked.
        if (Frozen && (index < ResourceCount))        
            *pdata = pResources[index];
        else        
            GetResourceData_Locked(pdata, index);
    }


    // Get resource data by value.
    GFxResourceBindData    GetResourceData(const GFxResourceHandle &h)
    {
        GFxResourceBindData bd;

        if (h.IsIndex())        
            GetResourceData(&bd, h.GetBindIndex());
        else
        {
            bd.pBinding = this;
            bd.pResource= h.GetResourcePtr();
        }               
        return bd;
    }

 
    UInt GetResourceCount() const { return ResourceCount;  }

    // Freeze the Resource binding once it is no longer being modified.
    void    Freeze() { Frozen = 1;  }    
    

    template<class C>
    C* operator [] (const GFxResourcePtr<C> &ref) const
    {
        return static_cast<C*>(ref.GetResource(this));
    }
};

inline GFxResource* GFxResourceHandle::GetResource(const GFxResourceBinding* pbinding) const
{
    if (HType == RH_Pointer)
        return pResource;

    GFxResourceBindData rbd;
    pbinding->GetResourceData(&rbd, BindIndex);
    return  rbd.pResource;
}

// Obtains a resource and its relevant binding, which may be different from a passed one.
inline GFxResource* GFxResourceHandle::GetResourceAndBinding(GFxResourceBinding* pbinding,
                                                             GFxResourceBinding** presBinding) const
{
    if (HType == RH_Pointer)
    {
        *presBinding = pbinding;
        return pResource;
    }

    GFxResourceBindData rbd;
    pbinding->GetResourceData(&rbd, BindIndex);
    *presBinding = rbd.pBinding;
    return  rbd.pResource;

}


// ***** GFxResourceData - TBD: Move to GFxMovieDef?

class GFxLoadStates;

// A data object that can be used to create a resource. Data objects are created
// at load time and stored in GFxMovieDataDef; they allow resource binding index
// slots to be resolved when GFxMovieDefImpl is created. As an example, resource
// data for a gradient may provide enough information to create a GFxResource image
// that is used for a gradient in GFxMovieDefImpl. Every GFxMovieDefImpl may have
// a different gradient image based on their different gradient parameters.
class GFxResourceData
{
public:

    typedef void*   DataHandle;

    // DataInterface     
    class DataInterface
    {
    public:
        virtual ~DataInterface() { }

        // Reference counting on the data object, if necessary.
        // Default implementation assumes that DataHandle object is derived from
        // GRefCountBase<C>. If that is not the case, it needs to be overriden.
        virtual void    AddRef(DataHandle hdata)
        { GASSERT(hdata); ((GRefCountBaseImpl*)hdata)->AddRef(); }
        virtual void    Release(DataHandle hdata)
        { GASSERT(hdata); ((GRefCountBaseImpl*)hdata)->Release(); }

        // Creates/Loads resource based on data and loading process.
        // Fills in resource information into pbindData as follows:
        //  - pResource: set if CreateResource succeeds, 0 otherwise.
        //  - pResourceBinding : set only if resource requires custom binding;
        //                       otherwise retains its original value.
        virtual bool    CreateResource(DataHandle hdata, GFxResourceBindData *pbindData,
                                       GFxLoadStates *plp) const = 0;

        // Also need to create key ??
    };

protected:
    // Key data.
    DataInterface* pInterface;
    DataHandle     hData;

public:

    GFxResourceData()
    {
        pInterface = 0;
        hData      = 0;
    }

    GFxResourceData(DataInterface* pki, DataHandle hk)
    {
        if (pki)
            pki->AddRef(hk);
        pInterface = pki;
        hData      = hk;
    }

    GFxResourceData(const GFxResourceData &src)
    {
        if (src.pInterface)
            src.pInterface->AddRef(src.hData);
        pInterface = src.pInterface;
        hData      = src.hData;
    }

    ~GFxResourceData()
    {
        if (pInterface)
            pInterface->Release(hData);
    }

    GFxResourceData& operator = (const GFxResourceData& src)
    {
        if (src.pInterface)
            src.pInterface->AddRef(src.hData);
        if (pInterface)
            pInterface->Release(hData);
        pInterface = src.pInterface;
        hData      = src.hData;
        return *this;
    }


    // *** Interface implementation

    bool            IsValid() const           { return pInterface != 0; }

    // Accessors, useful for DataInterface implementation.
    DataInterface*  GetDataInterface() const  {  return pInterface; }
    DataHandle      GetData() const           { return hData; }

    bool            CreateResource(GFxResourceBindData *pbindData, GFxLoadStates *plp) const
    {        
        return pInterface ? pInterface->CreateResource(hData, pbindData, plp) : 0;
    }
};


#endif

