/**********************************************************************

Filename    :   GImageInfo.h
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

#include "GImageInfo.h"
#include "GImage.h"


// ***** GFxImageInfoBaseImpl implementation

GImageInfoBaseImpl::GImageInfoBaseImpl(GTexture *ptexture)
    : pTexture(ptexture)
{
    if (ptexture)
        ptexture->AddChangeHandler(this);
}

GImageInfoBaseImpl::~GImageInfoBaseImpl()
{
    if (pTexture)
        pTexture->RemoveChangeHandler(this);
}

GTexture*   GImageInfoBaseImpl::GetTexture(GRenderer* prenderer)
{
    if (pTexture)
    {
        // We currently only support one renderer
        GASSERT(pTexture->GetRenderer() == prenderer);
        return pTexture;
    }

    pTexture = *prenderer->CreateTexture();
    if (pTexture)
    {
        // Use our function to override the texture.
        if (Recreate(prenderer))
            pTexture->AddChangeHandler(this);
        else
            pTexture = 0;
    }
    return pTexture.GetPtr();
}


// GTexture::ChangeHandler implementation           
void    GImageInfoBaseImpl::OnChange(GRenderer* prenderer, EventType changeType)
{
    GUNUSED(prenderer);

    // Currently we just handle renderer death by destroying its textures
    // Texture Data loss does not need to be handled because Recreate will
	// restore the image data if possible. For RT textures we handle it separately.
    if (pTexture)
    {
        if (changeType == Event_RendererReleased)
        {
            pTexture->RemoveChangeHandler(this);
            pTexture = 0;
        }       
    }
}


// ***** GImageInfo implementation

GImageInfo::GImageInfo(GImage *pimage)
    : pImage(pimage), TargetWidth(0), TargetHeight(0)
{
}
 
GImageInfo::GImageInfo(GImage *pimage, UInt targetWidth, UInt targetHeight)
    : pImage(pimage), TargetWidth(targetWidth), TargetHeight(targetHeight)
{    
}

GImageInfo::GImageInfo(GTexture *ptexture, UInt targetWidth, UInt targetHeight)
    : GImageInfoBaseImpl(ptexture), pImage(0),
      TargetWidth(targetWidth), TargetHeight(targetHeight)
{
    // Note: GImageInfoBaseImpl will install a texture handler on us.
}

GImageInfo::~GImageInfo()
{
    // pTexture handler is removed in ~GImageInfoBaseImpl.
}

// GImageInfoBase Implementation
UInt    GImageInfo::GetWidth() const
{    
    return TargetWidth ? TargetWidth : (pImage ? pImage->Width : 0); 
}
UInt    GImageInfo::GetHeight() const
{    
    return TargetHeight ? TargetHeight : (pImage ? pImage->Height : 0); 
}


// *** Data Assignment

void    GImageInfo::SetImage(GImage* pimage)
{
    pImage = pimage;   
    if (pTexture)
        Recreate(pTexture->GetRenderer());    
}
void    GImageInfo::SetImage(GImage* pimage, UInt targetWidth, UInt targetHeight)
{
    TargetWidth  = targetWidth;
    TargetHeight = targetHeight;
    SetImage(pimage);
}

// Sets the texture and potential target dimensions.
void    GImageInfo::SetTexture(GTexture *ptexture)
{        
    if (pTexture == ptexture)
        return;
    // Clear image, if any.
    pImage = 0;
    if (pTexture)
        pTexture->RemoveChangeHandler(this);
    if (ptexture)
        ptexture->AddChangeHandler(this);
    pTexture = ptexture;
}
void    GImageInfo::SetTexture(GTexture *ptexture, UInt targetWidth, UInt targetHeight)
{    
    TargetWidth  = targetWidth;
    TargetHeight = targetHeight;
    SetTexture(ptexture);
}



// Override GetTexture so that no attempt to create texture takes place if
// there is no backup image date for initialization.
GTexture*   GImageInfo::GetTexture(GRenderer* prenderer)
{
    if (pTexture)
    {
        // We currently only support one renderer
        GASSERT(pTexture->GetRenderer() == prenderer);
        return pTexture;
    }

    // If there is no image, no need to call Recreate on a texture
    // since it can not possibly succeed.
    if (!pImage)
        return 0;

    pTexture = *prenderer->CreateTexture();
    if (pTexture)
    {
        // Use our function to override the texture.
        if (Recreate(prenderer))
            pTexture->AddChangeHandler(this);
        else
            pTexture = 0;
    }
    return pTexture.GetPtr();
}


// Override OnChange so that we can clear out pTexture pointer on data loss
// when there is no backup image. Users can override this behavior if desired.
void    GImageInfo::OnChange(GRenderer* prenderer, EventType changeType)
{
    GImageInfoBaseImpl::OnChange(prenderer, changeType);	

    // Unlike regular ImageInfo which can Recreate its data based on GImage,
    // GImageInfo texture can not do so (instead it waits for the user to call
    // SetTexture again). Until such call takes place we just handle data
    // loss by releasing the texture object.	
    if (pTexture && (changeType == Event_DataLost) &&
        (pImage.GetPtr() == 0) )
    {
        pTexture->RemoveChangeHandler(this);
        pTexture = 0;
    }
}

// Override recreate - to initialize texture data.
// We actually populate texture data from the image here.
bool    GImageInfo::Recreate(GRenderer* prenderer)
{
    GUNUSED(prenderer);

    if (pTexture)
    {
        if (pImage)
        {
            if (pTexture->InitTexture(pImage, TargetWidth, TargetHeight))
                return 1;
        }
        else
        {
            // This may happen if images were not loaded or bound by user correctly.
            GFC_DEBUG_WARNING(1,
                "GImageInfo::GetTexture failed, image not available. "
                "Please check GFxLoader::LoadImageData flag.");
        }
    }
    return 0;
}



/*
///////////////////////////////
GImageFileInfo::~GImageFileInfo()
{
    if (pFileName) GFREE(pFileName);
}

void GImageFileInfo::SetFileName(const char* name)
{
    if (pFileName)
        GFREE((void*)pFileName);

    char* ptr = 0;
    if (name)
    {
        int len = int(strlen(name));
        ptr = (char*)GALLOC(len + 1);
        memcpy(ptr, name, len+1);
    }
    pFileName = ptr;
}

GTexture* GImageFileInfo::GetTexture(class GRenderer* prenderer)
{
    if (pTexture)
    {
        // We currently only support one renderer
        GASSERT(pTexture->GetRenderer() == prenderer);
        return pTexture;
    }

    if (pFileName)
    {
        if ((pTexture = *prenderer->CreateTextureFromFile(pFileName, TargetWidth, TargetHeight)))
        {
            pTexture->AddChangeHandler(this);
        }
    }
    else
    {
        // This may happen if images were not loaded or bound by user correctly.
        GFC_DEBUG_WARNING(1, "GImageFileInfo::GetTexture failed, image not available. Please check GFxLoader::LoadImageData flag.");
    }
    return pTexture;

}

// GTexture::ChangeHandler implementation           
void    GImageFileInfo::OnChange(GRenderer* prenderer, EventType changeType)
{
    GUNUSED(prenderer);

    // Currently we just handle renderer death by destroying its textures
    // Texture Data loss does not need to be handled because
    if (pTexture)
    {
        if (changeType == Event_RendererReleased)
        {
            pTexture->RemoveChangeHandler(this);
            pTexture = 0;
        }       
    }
}

bool    GImageFileInfo::Recreate(GRenderer* prenderer)
{
    GUNUSED(prenderer);

    if (pTexture && pFileName)
        return pTexture->InitTextureFromFile(pFileName, TargetWidth, TargetHeight);
    return 0;
}
*/
