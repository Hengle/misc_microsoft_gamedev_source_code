/**********************************************************************

Filename    :   GRendererCommonImpl.cpp
Content     :   Renderer classes implementation shared by all back ends
Created     :   June 29, 2005
Authors     :   Michael Antonov

Notes       :   
History     :   

Copyright   :   (c) 1998-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GRendererCommonImpl.h"



// ***** GTextureImplNode Implementation

GTextureImplNode::GTextureImplNode(GRendererNode *plistRoot) : GRendererNode(plistRoot)
{
    UserHandle      = 0;
    HandlerArrayFlag= 0;
    pHandler        = 0;
}

GTextureImplNode::~GTextureImplNode()
{
    if (pHandler && HandlerArrayFlag)
        delete pHandlerArray;
    if (pFirst)
        RemoveNode();
}

// Add/Remove notification
void    GTextureImplNode::AddChangeHandler(ChangeHandler *phandler)
{
    if (pHandler)
    {
        if (!HandlerArrayFlag)              
        {
            ChangeHandler *poldHandler = pHandler;
            if ((pHandlerArray = new GTL::garray<ChangeHandler*>)!=0)
            {
                pHandlerArray->push_back(poldHandler);
                HandlerArrayFlag = 1;
            }
            else
                return;
        }
        pHandlerArray->push_back(phandler);
        return;
    }
    // If no array, just store handler
    pHandler = phandler;
}

void    GTextureImplNode::RemoveChangeHandler(ChangeHandler *phandler)
{
    if (HandlerArrayFlag)
    {
        for (UInt i=0; i<pHandlerArray->size(); i++)
            if ((*pHandlerArray)[i] == phandler)
            {
                // Handler found -> remove
                pHandlerArray->remove(i);
                if (pHandlerArray->size() == 1)
                {                       
                    ChangeHandler *poldHandler = (*pHandlerArray)[0];
                    delete pHandlerArray;
                    pHandler = poldHandler;
                    HandlerArrayFlag = 0;
                    return;
                }
            }           
    }
    else if (pHandler == phandler)
    {
        pHandler = 0;
    }
}

// Routine to call handlers
void    GTextureImplNode::CallHandlers(ChangeHandler::EventType event)
{
    GRenderer*  prenderer = GetRenderer();

    if (HandlerArrayFlag)
    {
        for (UInt i=0; i<pHandlerArray->size(); i++)
            (*pHandlerArray)[i]->OnChange(prenderer, event);
    }
    else if (pHandler)
    {
        pHandler->OnChange(prenderer, event);
    }
}

bool    GTextureImplNode::CallRecreate()
{
    GRenderer*  prenderer = GetRenderer();
    
    if (HandlerArrayFlag)
    {
        for (UInt i=0; i<pHandlerArray->size(); i++)        
            if ((*pHandlerArray)[i]->Recreate(prenderer))
                return 1;
    }
    else if (pHandler)
    {
        if (pHandler->Recreate(prenderer))
            return 1;
    }
    return 0;
}


GArrayNode::GArrayNode(GRenderer* prenderer, GRendererNode* proot, const void* pdata, UInt size, GRenderer::CachedData* pcache)
    : GRendererNode(proot)
{
    GUNUSED(prenderer);
    pData = pdata;
    Size = size;
    pCache = pcache;
}

GArrayNode* GArrayNode::AddNode(GRenderer* prenderer, GRenderer::CachedDataType type, const void* pvdata, UInt size, GRenderer::CacheProvider* pcache)
{
    GRenderer::CachedData* pdata = pcache->GetCachedData(prenderer);
    if (pdata)
        return (GArrayNode*) pdata;

    pdata = pcache->CreateCachedData(type,prenderer);
    if (!pdata)
        return 0;
    GArrayNode* pnode = new GArrayNode(prenderer, this, pvdata, size, pdata);
    pdata->SetRendererData(pnode);
    return (GArrayNode*) pdata;
}

void GArrayNode::ReleaseData(GRenderer::CachedData* pdata)
{
    GArrayNode* pnode = (GArrayNode*) pdata->GetRendererData();
    pnode->RemoveNode();
    delete pnode;
}

void GArrayNode::ReleaseRenderer()
{
    while (pFirst != this)
    {
        GArrayNode *pnode = (GArrayNode*)pFirst;
        pnode->pCache->ReleaseDataByRenderer();
        pnode->RemoveNode();
        delete pnode;
    }
}


GRendererEventHandlerImpl::~GRendererEventHandlerImpl()
{
    if (pHandler && HandlerArrayFlag)
        delete pHandlerArray;
}

bool    GRendererEventHandlerImpl::AddHandler(GRenderer::EventHandler *phandler)
{
    if (pHandler)
    {
        if (!HandlerArrayFlag)              
        {
            GRenderer::EventHandler *poldHandler = pHandler;
            if ((pHandlerArray = new GTL::garray<GRenderer::EventHandler*>)!=0)
            {
                pHandlerArray->push_back(poldHandler);
                HandlerArrayFlag = 1;
            }
            else
                return 0;
        }
        pHandlerArray->push_back(phandler);
        return 1;
    }
    // If no array, just store handler
    pHandler = phandler;
    return 1;
}

void    GRendererEventHandlerImpl::RemoveHandler(GRenderer::EventHandler *phandler)
{
    if (HandlerArrayFlag)
    {
        for (UInt i=0; i<pHandlerArray->size(); i++)
            if ((*pHandlerArray)[i] == phandler)
            {
                // Handler found -> remove
                pHandlerArray->remove(i);
                if (pHandlerArray->size() == 1)
                {                       
                    GRenderer::EventHandler *poldHandler = (*pHandlerArray)[0];
                    delete pHandlerArray;
                    pHandler = poldHandler;
                    HandlerArrayFlag = 0;
                    return;
                }
            }           
    }
    else if (pHandler == phandler)
    {
        pHandler = 0;
    }
}

// Routine to call handlers
void    GRendererEventHandlerImpl::CallHandlers(GRenderer*  prenderer, GRenderer::EventHandler::EventType event)
{
    if (HandlerArrayFlag)
    {
        for (UInt i=0; i<pHandlerArray->size(); i++)
            (*pHandlerArray)[i]->OnEvent(prenderer, event);
    }
    else if (pHandler)
    {
        pHandler->OnEvent(prenderer, event);
    }
}

// ***** CacheProvider Testing Implementation

#ifdef GFC_RENDERER_TEST_CACHEDATA

void    CacheNode::VerifyCachedData(GRenderer *prenderer, GRenderer::CacheProvider *pcache, BufferType buffType,
                                    bool verifyFlag, int check1, int check2)
{
    if (pcache)
    {
        GRenderer::CachedData* pdata = pcache->GetCachedData(prenderer);
        if (pdata)
        {
            CacheNode* pnode = (CacheNode*) pdata->GetRendererData();
            
            // Verify assertions.
            // This will fail if caller passed CacheProvider for the wrong data set type.
            GASSERT(pnode->BuffType == buffType);           
            if (verifyFlag)
            {
                // These would fail if buffer did not match our expected cached version of data.
                GASSERT(pnode->Check1 == check1);
                GASSERT(pnode->Check2 == check2);
            }
        }
        else if (pdata = pcache->CreateCachedData((GRenderer::CachedDataType)buffType, prenderer))
        {
            CacheNode* pnode = new CacheNode(this);
            pnode->pCacheData   = pdata;
            pnode->BuffType     = buffType;
            pnode->Check1       = check1;
            pnode->Check2       = check2;
            pdata->SetRendererData(pnode);
        }
    }
}

// Releases list, assuming that this is a Root node. Used by ~GRenderer.
void    CacheNode::ReleaseList()
{
    while (pFirst != this)
    {
        CacheNode *pnode = (CacheNode*)pFirst;
        pnode->pCacheData->ReleaseDataByRenderer();
        pnode->RemoveNode();
        delete pnode;
    }
}

void    CacheNode::ReleaseCachedData(GRenderer::CachedData *pdata, GRenderer::CachedDataType type)
{
    // Implements release of cached data that was allocated from the cache provider.
    // No buffers: does nothing.
    if (pdata)
    {
        CacheNode* pnode = (CacheNode*) pdata->GetRendererData();
        GASSERT(pnode->BuffType == type);
        pnode->RemoveNode();
        delete pnode;
    }
}

#endif 

