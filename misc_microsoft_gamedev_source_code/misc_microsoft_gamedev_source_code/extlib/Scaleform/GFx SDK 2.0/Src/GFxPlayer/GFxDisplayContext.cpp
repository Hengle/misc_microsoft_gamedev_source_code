/**********************************************************************

Filename    :   GFxDisplayContext.cpp
Content     :   Implements GFxDisplayContext class, used to pass state
                and binding information during rendering.
Created     :   
Authors     :   Michael Antonov

Copyright   :   (c) 2001-2007 Scaleform Corp. All Rights Reserved.

Notes       :   More implementation-specific classes are
                defined in the GFxPlayerImpl.h

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GFxDisplayContext.h"

#include "GFxCharacter.h"

// Necessary to avoid template related issued wiuth GASEnvironment.
#include "GFxAction.h"


// ***** GFxDisplayContext

GFxDisplayContext::GFxDisplayContext(const GFxSharedState *pstate,
                                     GFxResourceWeakLib *plib,
                                     GFxResourceBinding *pbinding,
                                     Float pixelScale,
                                     const Matrix& viewportMatrix)
{
    // Capture the state for efficient access
    // (Usually called in the beginning for Advance)

    const static GFxState::StateType stateQuery[5] =
    { GFxState::State_RenderConfig, GFxState::State_GradientParams,
      GFxState::State_ImageCreator, GFxState::State_FontCacheManager,
      GFxState::State_Log };

    GFxState* pstates[5] = {0,0,0,0,0};

    // Get multiple states at once to avoid extra locking.
    pstate->GetStatesAddRef(pstates, stateQuery, 5);
    pRenderConfig     = *(GFxRenderConfig*)     pstates[0];
    pGradientParams   = *(GFxGradientParams*)   pstates[1];
    pImageCreator     = *(GFxImageCreator*)     pstates[2];
    pFontCacheManager = *(GFxFontCacheManager*) pstates[3];
    pLog              = *(GFxLog*)              pstates[4];

    pWeakLib          = plib;
    pResourceBinding  = pbinding;

    pRenderStats      = 0;

    MaskRenderCount   = 0;
    MaskStackIndex    = 0;
    MaskStackIndexMax = 0;

    PixelScale        = pixelScale;
	InvPixelScale     = 1.0f / PixelScale;
    ViewportMatrix    = viewportMatrix;
}

GFxDisplayContext::~GFxDisplayContext()
{
    GASSERT(MaskStackIndex == 0);
    GASSERT(MaskStackIndexMax == 0);
    GASSERT(MaskRenderCount == 0);
}


// Mask Stack management
void GFxDisplayContext::PushAndDrawMask(GFxCharacter* pmask, StackData stackData)
{
    GRenderer*  prenderer = GetRenderer();
    // Only clear stencil if no masks were applied before us; otherwise
    // no clear is necessary because we are building a cumulative mask.
    prenderer->BeginSubmitMask((MaskStackIndex == 0) ?
                               GRenderer::Mask_Clear : GRenderer::Mask_Increment);

    // Stack bounds check. We increment anyway, but access only lower stack.
    if (MaskStackIndex < GFxDisplayContext::Mask_MaxStackSize)
        MaskStack[MaskStackIndex] = pmask;
    MaskStackIndex++;
    MaskStackIndexMax++;
    MaskRenderCount++;

    // Draw mask.
    prenderer->PushBlendMode(pmask->GetBlendMode());
    pmask->Display(*this,stackData);
    prenderer->PopBlendMode();

    // Done rendering mask.
    MaskRenderCount--;
    prenderer->EndSubmitMask();
}

void GFxDisplayContext::PopMask()
{
    GASSERT(MaskStackIndex > 0);
    MaskStackIndex--;
    // If we reached the bottom of stack, disable mask checks.        
    if (MaskStackIndex == 0)
    {
        GetRenderer()->DisableMask();
        MaskStackIndexMax = 0;
    }
}
