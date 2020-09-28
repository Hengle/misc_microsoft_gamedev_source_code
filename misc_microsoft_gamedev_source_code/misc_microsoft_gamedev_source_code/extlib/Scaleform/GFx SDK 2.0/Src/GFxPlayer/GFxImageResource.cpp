/**********************************************************************

Filename    :   GFxImageResource.cpp
Content     :   Image resource representation for GFxPlayer
Created     :   January 30, 2007
Authors     :   Michael Antonov

Notes       :   

Copyright   :   (c) 2005-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GFxImageResource.h"


// ***** GFxImageFileKeyData implementation


class GFxImageFileInfoKeyData : public GRefCountBase<GFxImageFileInfoKeyData>
{
    // Image States.  
    GPtr<GFxFileOpener>     pFileOpener;
    GPtr<GFxImageCreator>   pImageCreator;

public:
    // Key File Info - provides file name and image dimensions.
    // Note that this key data is potentially different from the ResourceFileInfo
    // specified in the export because its filename could have been translated
    // and/or adjusted to have a different relative/absolute path.
    GPtr<GFxImageFileInfo>  pFileInfo;

    GFxImageFileInfoKeyData(GFxImageFileInfo* pfileInfo,
                            GFxFileOpener* pfileOpener,
                            GFxImageCreator* pimageCreator)
    {
        pFileInfo   = pfileInfo;
        pFileOpener = pfileOpener;
        pImageCreator = pimageCreator;
    }

    bool operator == (GFxImageFileInfoKeyData& other) const
    {
        return (pFileOpener == other.pFileOpener &&
                pImageCreator == other.pImageCreator &&
                pFileInfo->FileName == other.pFileInfo->FileName);
    }
    bool operator != (GFxImageFileInfoKeyData& other) const
    {
        return !operator == (other);
    }

    size_t  GetHashCode() const
    {
        return pFileInfo->GetHashCode() ^
            ((size_t)pFileOpener.GetPtr()) ^ (((size_t)pFileOpener.GetPtr()) >> 7) ^
            ((size_t)pImageCreator.GetPtr()) ^ (((size_t)pImageCreator.GetPtr()) >> 7);
    }
};

class GFxImageFileKeyInterface : public GFxResourceKey::KeyInterface
{
public:
    typedef GFxResourceKey::KeyHandle KeyHandle;    

    // GFxResourceKey::KeyInterface implementation.    
    virtual void                AddRef(KeyHandle hdata);  
    virtual void                Release(KeyHandle hdata);
    virtual GFxResourceKey::KeyType GetKeyType(KeyHandle hdata) const;
    virtual size_t              GetHashCode(KeyHandle hdata) const;
    virtual bool                KeyEquals(KeyHandle hdata, const GFxResourceKey& other);
   // const  GFxResourceFileInfo* GetFileInfo(KeyHandle hdata) const;

     const  char* GetFileURL(KeyHandle hdata) const;
};

static GFxImageFileKeyInterface GFxImageFileKeyInterface_Instance;


// Reference counting on the data object, if necessary.
void    GFxImageFileKeyInterface::AddRef(KeyHandle hdata)
{
    GFxImageFileInfoKeyData* pdata = (GFxImageFileInfoKeyData*) hdata;
    GASSERT(pdata);
    pdata->AddRef();
}

void    GFxImageFileKeyInterface::Release(KeyHandle hdata)
{
    GFxImageFileInfoKeyData* pdata = (GFxImageFileInfoKeyData*) hdata;
    GASSERT(pdata);
    pdata->Release();
}

// Key/Hash code implementation.
GFxResourceKey::KeyType GFxImageFileKeyInterface::GetKeyType(KeyHandle hdata) const
{
    GUNUSED(hdata);
    return GFxResourceKey::Key_File;
}

size_t  GFxImageFileKeyInterface::GetHashCode(KeyHandle hdata) const
{
    GFxImageFileInfoKeyData* pdata = (GFxImageFileInfoKeyData*) hdata;
    return pdata->GetHashCode();    
}

bool    GFxImageFileKeyInterface::KeyEquals(KeyHandle hdata, const GFxResourceKey& other)
{
    if (this != other.GetKeyInterface())
        return 0;

    GFxImageFileInfoKeyData* pthisData = (GFxImageFileInfoKeyData*) hdata;
    GFxImageFileInfoKeyData* potherData = (GFxImageFileInfoKeyData*) other.GetKeyData();
    GASSERT(pthisData);
    GASSERT(potherData);
    return (*pthisData == *potherData);
}

// Get file info about the resource, potentially available with Key_File.
/*
const GFxResourceFileInfo* GFxImageFileKeyInterface::GetFileInfo(KeyHandle hdata) const
{
    GFxImageFileInfoKeyData* pdata = (GFxImageFileInfoKeyData*) hdata;
    GASSERT(pdata);
    return pdata->pFileInfo.GetPtr();
}
*/

const char* GFxImageFileKeyInterface::GetFileURL(KeyHandle hdata) const
{
    GFxImageFileInfoKeyData* pdata = (GFxImageFileInfoKeyData*) hdata;
    GASSERT(pdata);
    return pdata->pFileInfo->FileName.ToCStr();
}


// ***** GFxImageResource

GFxResourceKey  GFxImageResource::CreateImageFileKey(GFxImageFileInfo* pfileInfo,
                                                     GFxFileOpener* pfileOpener,
                                                     GFxImageCreator* pimageCreator)
{
    GPtr<GFxImageFileInfoKeyData> pdata =
        *new GFxImageFileInfoKeyData(pfileInfo, pfileOpener, pimageCreator);

    return GFxResourceKey(&GFxImageFileKeyInterface_Instance,
                          (GFxResourceKey::KeyHandle)pdata.GetPtr() );
}


