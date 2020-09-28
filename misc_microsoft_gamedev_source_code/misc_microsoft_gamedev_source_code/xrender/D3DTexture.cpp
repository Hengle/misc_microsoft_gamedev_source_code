//============================================================================
//
//  File: D3DTexture.cpp
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#include "xrender.h"
#include "renderThread.h"
#include "D3DTexture.h"
#include "XboxTextureHeap.h"
#include "packedTextureManager.h"

namespace
{
   class BFreeD3DTextureRCO : BRenderCommandObjectInterface
   {
      mutable BD3DTexture mTexture;
      
   public:
      BFreeD3DTextureRCO(const BD3DTexture& t) : mTexture(t) { }
      
      virtual void processCommand(DWORD data) const
      {
         mTexture.releaseImmediate();
      }
   };
} // anonymous namespace
   
void BD3DTexture::release(void) 
{ 
   if (!getBaseTexture()) 
      return;

   if (GetCurrentThreadId() == gRenderThread.getMainThreadId())
   {
      gRenderThread.submitCopyOfCommandObject(BFreeD3DTextureRCO(*this));
      
      clear();
   }
   else
   {
      releaseImmediate();
   }
}

void BD3DTexture::releaseImmediate(void) 
{
   ASSERT_RENDER_THREAD;
   
   if (!getBaseTexture())
      return;

   if (gRenderThread.getHasD3DOwnership())      
   {
      for (uint samplerIndex = 0; samplerIndex < cMaxD3DTextureSamplers; samplerIndex++)
      {
         D3DBaseTexture* pTex;
         BD3D::mpDev->GetTexture(samplerIndex, &pTex);
         if (pTex)
         {
            if (pTex == getBaseTexture())
               BD3D::mpDev->SetTexture(samplerIndex, NULL);
            pTex->Release();
         }  
      }      
   }
            
   if (getD3DCreated())
   {
      getBaseTexture()->Release();
   }
   else
   {
      switch (mAllocator)
      {  
         case cXboxTextureHeap:
         {
            bool success = gpXboxTextureHeap->releaseValley(getBaseTexture());
            BVERIFY(success);
            
            delete getBaseTexture();
            
            break;
         }
         case cPackedTextureManager:
         {
            if (gpPackedTextureManager)
            {
               bool success = gpPackedTextureManager->release(getBaseTexture());
               BVERIFY(success);
            }
            
            delete getBaseTexture();
                        
            break;
         }
         default:
         {
            BASSERT(0);
            break;
         }
      }         
   }
      
   clear();
}

void BD3DTexture::updateIdentifier(void)
{
   if (!getBaseTexture())
      return;
      
   DWORD id = getBaseTexture()->GetIdentifier();
   
   getBaseTexture()->SetIdentifier(cBD3DTextureIdentifier | (mAllocator << cBD3DTextureAllocatorIndexShift) | (id & 0x00FFFFFF));
}

void BD3DTexture::setIdentifier(uint handle)
{
   if (!getBaseTexture())
      return;

   BDEBUG_ASSERT(handle <= 0x00FFFFFF);      

   getBaseTexture()->SetIdentifier(cBD3DTextureIdentifier | (mAllocator << cBD3DTextureAllocatorIndexShift) | (handle & 0x00FFFFFF));
}

uint BD3DTexture::getIdentifier() const
{
   if (!getBaseTexture())
      return 0;

   return getBaseTexture()->GetIdentifier() & 0x00FFFFFF;
}

void BD3DTexture::releaseWildTexture(IDirect3DBaseTexture9* pTex)
{
   ASSERT_RENDER_THREAD;
   
   if (!pTex)
      return;
      
   if (gRenderThread.getHasD3DOwnership())      
   {
      for (uint samplerIndex = 0; samplerIndex < cMaxD3DTextureSamplers; samplerIndex++)
      {
         D3DBaseTexture* pTex;
         BD3D::mpDev->GetTexture(samplerIndex, &pTex);
         if (pTex)
         {
            if (pTex == pTex)
               BD3D::mpDev->SetTexture(samplerIndex, NULL);
            pTex->Release();
         }  
      }      
   }
   
   DWORD id = pTex->GetIdentifier();
      
   eAllocator allocator = cD3D;
   
   if ((id & cBD3DTextureIdentifierMask) == cBD3DTextureIdentifier)
   {
      allocator = static_cast<eAllocator>((id >> cBD3DTextureAllocatorIndexShift) & cBD3DTextureAllocatorIndexMask);
      
      if (allocator >= cNumAllocators)
      {
         BASSERT(0);
         allocator = cD3D;
      }
   }
         
   switch (allocator)
   {
      case cD3D:
      {
         pTex->Release();
         break;
      }
      case cXboxTextureHeap:
      {
         bool success = gpXboxTextureHeap->releaseValley(pTex);
         BVERIFY(success);
         
         delete pTex;
         
         break;
      }
      case cPackedTextureManager:
      {
         if (gpPackedTextureManager)
         {
            bool success = gpPackedTextureManager->release(pTex);
            BVERIFY(success);
         }
         
         // rg [8/6/08] - delete was missing, why?
         delete pTex;
         
         break;
      }
   }
}
