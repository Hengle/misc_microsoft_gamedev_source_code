/**********************************************************************

Filename    :   GFxDisplayContext.h
Content     :   Defines GFxDisplayContext class, used to pass state
                and binding information during rendering.
Created     :   
Authors     :   Michael Antonov

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   More implementation-specific classes are
                defined in the GFxPlayerImpl.h

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFXDISPLAYCONTEXT_H
#define INC_GFXDISPLAYCONTEXT_H

#include "GRefCount.h"
#include "GFxResource.h"
#include "GFxResourceHandle.h"

#include "GFxRenderConfig.h"
// Loader: For Gradient/Image states.
#include "GFxLoader.h"


// ***** Declared Classes

class GFxDisplayContext;

// External Classes
class GFxCharacter;


// *****  GFxDisplayContext - used by GFxCharacter::Display

// Display context - passes renderer/masking state.
// This class is here largely to support creation of nested masks;
// i.e. it uses a stack to track mask layers that were applied so far.

class GFxDisplayContext
{
public:    
    typedef GRenderer::Matrix Matrix;
   
    GPtr<GFxRenderConfig> pRenderConfig;
    GFxRenderConfig*    GetRenderConfig() const   { return pRenderConfig; }
    GRenderer*          GetRenderer() const       { return pRenderConfig->GetRenderer(); }
    UInt32              GetRenderFlags() const    { return pRenderConfig->GetRenderFlags(); }        

    // Statistics
    GFxRenderStats*     pRenderStats;


    // *** Image generation states

    // States useful during rendering
    // (used to generate gradients and dynamic images).
    GPtr<GFxGradientParams>     pGradientParams;
    GPtr<GFxImageCreator>       pImageCreator;
    GPtr<GFxFontCacheManager>   pFontCacheManager;
    GPtr<GFxLog>                pLog;               // Log used during rendering 

    // Library
    GPtr<GFxResourceWeakLib>    pWeakLib;

    // Resource binding: need this to look up.
    // Note that this variable changes as it is re-assigned during display traversal.
    // Can be null (initially, but not during shape rendering).
    GFxResourceBinding*         pResourceBinding;    

    // Tessellator Data

    // Pixel scale used for rendering
    Float                       PixelScale;
	Float						InvPixelScale;

    Matrix                      ViewportMatrix;


    // *** Mask stack support - necessary to ensure proper clipping.
    
    enum ContextConstants {
        Mask_MaxStackSize = 64,
    };    
    // Indicates how deep inside of the mask rendering we are (if > 0 we are rendering mask).
    // Technically masking of masks is impossible (?); but this is a safeguard.
    UInt            MaskRenderCount;

    // A stack layer of currently active masks.
    // Entries in this stack need to be re-applied if a layer is popped and
    // partially non-masked rendering must take place after it.
    UInt            MaskStackIndexMax;
    UInt            MaskStackIndex;
    GFxCharacter*   MaskStack[Mask_MaxStackSize];    

    

    GFxDisplayContext(const GFxSharedState *pstate,
                      GFxResourceWeakLib *plib,
                      GFxResourceBinding *pbinding,
                      Float pixelScale,
                      const Matrix& viewportMatrix);  
    ~GFxDisplayContext();
   
    // Mask stack helpers.
    void    PushAndDrawMask(GFxCharacter* pmask, StackData stackData);
    void    PopMask();

    Float   GetPixelScale() const   { return PixelScale; }

    // Statistics update
    void    AddTessTriangles(UInt count)
    {
        if (pRenderStats)
            pRenderStats->AddTessTriangles(count);
    }
};



#endif // INC_GFXDISPLAYCONTEXT_H
