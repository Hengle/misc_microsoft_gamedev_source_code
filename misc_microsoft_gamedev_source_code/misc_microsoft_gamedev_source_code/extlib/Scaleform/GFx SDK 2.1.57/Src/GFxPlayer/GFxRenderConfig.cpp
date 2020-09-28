/**********************************************************************

Filename    :   GFxRenderConfig.h
Content     :   Public SWF loading interface for GFxPlayer
Created     :   January 14, 2007
Authors     :   Michael Antonov

Notes       :   

Copyright   :   (c) 2005-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GFxRenderConfig.h"


GFxRenderConfig::GFxRenderConfig()
    : GFxState(State_RenderConfig)
{
    MaxCurvePixelError  = 1.0f;
    RenderFlags         = 0;    
    StrokerAAWidth      = 1.2f;
    RendererCapBits     = 0;
    RendererVtxFmts     = 0;
}

GFxRenderConfig::GFxRenderConfig(UInt rendererFlags, 
                                 UInt rendererCapBits, 
                                 UInt rendererVtxFmts)
    : GFxState(State_RenderConfig)
{
    MaxCurvePixelError  = 1.0f;
    RenderFlags         = rendererFlags;    
    StrokerAAWidth      = 1.2f;
    RendererCapBits     = rendererCapBits;
    RendererVtxFmts     = rendererVtxFmts;
}

GFxRenderConfig::GFxRenderConfig(GRenderer* prenderer, UInt rendererFlags)
    : GFxState(State_RenderConfig)
{    
    MaxCurvePixelError  = 1.0f;
    RenderFlags         = rendererFlags;        
    StrokerAAWidth      = 1.2f;
    RendererCapBits     = 0;
    RendererVtxFmts     = 0;
    // Call SetRenderer so that RendererCapBits are initalized properly.
    SetRenderer(prenderer);
}

GFxRenderConfig::GFxRenderConfig(const GFxRenderConfig &src)
    : GFxState(State_RenderConfig)
{
    pRenderer           = src.pRenderer;
    RendererCapBits     = src.RendererCapBits;
    RendererVtxFmts     = src.RendererVtxFmts;
    MaxCurvePixelError  = src.MaxCurvePixelError;
    RenderFlags         = src.RenderFlags;    
    StrokerAAWidth      = src.StrokerAAWidth;    
}

GFxRenderConfig& GFxRenderConfig::operator = (const GFxRenderConfig &src)
{
    pRenderer           = src.pRenderer;
    RendererCapBits     = src.RendererCapBits;
    RendererVtxFmts     = src.RendererVtxFmts;
    MaxCurvePixelError  = src.MaxCurvePixelError;
    RenderFlags         = src.RenderFlags;        
    StrokerAAWidth      = src.StrokerAAWidth;    
    return *this;
}

void   GFxRenderConfig::SetRenderer(GRenderer* prenderer)
{
    GRenderer::RenderCaps rc;
    pRenderer = prenderer;

    if (prenderer)
    {
        prenderer->GetRenderCaps(&rc);
        RendererCapBits = rc.CapBits;
        RendererVtxFmts = rc.VertexFormats;
    }
    else
    {
        RendererCapBits = 0;
        RendererVtxFmts = 0;
    }
}


