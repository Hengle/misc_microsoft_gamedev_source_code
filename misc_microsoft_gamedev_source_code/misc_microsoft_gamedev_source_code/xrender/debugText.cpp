// File: debugText.cpp
#include "xrender.h"
#include "debugText.h"
#include "renderThread.h"
#include "xexception\xdebugtext.h"

#ifndef BUILD_FINAL

//============================================================================
// BRender::initDebugText
//============================================================================
void BDebugText::init(void)
{
   if (G_debug_text.get_buffer())
      return;
      
   if (!gRenderThread.getInitialized())
   {
      // Init D3D, this will mess up the regular init sequence, but we've got to do something.
      gRenderThread.init(BD3D::BCreateDeviceParams(), NULL, true);
   }
      
   G_debug_text.set_buffer(NULL, 0, 0, 0, D3DFMT_LE_X8R8G8B8);

   IDirect3DDevice9* pDevice = gRenderThread.releaseThreadOwnership();

   const DWORD clearColor = D3DCOLOR_ARGB(0x10, 0x10, 0x11, 0x16);

   pDevice->Clear(0, NULL, D3DCLEAR_TARGET, clearColor, 0.0f, 0);

   pDevice->Present(NULL, NULL, NULL, NULL);

   IDirect3DTexture9* pFrontBuf;
   pDevice->GetFrontBuffer(&pFrontBuf);

   D3DRESOURCETYPE resType = pFrontBuf->GetType();
   resType;
   BASSERT(D3DRTYPE_TEXTURE == resType);

   D3DSURFACE_DESC desc;
   pFrontBuf->GetLevelDesc(0, &desc);
   BASSERT((desc.Format == D3DFMT_LE_X8R8G8B8) || (desc.Format == D3DFMT_LE_X2R10G10B10));

   // rg [7/24/05] - Create VRAM address. 
   uchar* pPixel = reinterpret_cast<uchar*>(pFrontBuf->Format.BaseAddress << 12);

   G_debug_text.set_buffer(pPixel, desc.Width, desc.Height, desc.Width * 4, desc.Format);

//-- FIXING PREFIX BUG ID 7209
   const DWORD* pSurf = reinterpret_cast<const DWORD*>(pPixel);
//--

   // Sleep for a while to give the GPU time to finish copying
   Sleep(16);

   // Verify the buffer contains the right pixel. Otherwise our pointer isn't pointing to the VRAM copy of the surface.
   for (uint y = 0; y < desc.Height; y += 64)
   {
      for (uint x = 0; x < desc.Width; x++)
      {
         const uint ofs = XGAddress2DTiledOffset(x, y, desc.Width, sizeof(DWORD));

         DWORD c;

         __try
         {         
            c = pSurf[ofs];
         }
         __except(EXCEPTION_EXECUTE_HANDLER)
         {
            G_debug_text.set_buffer(NULL, 0, 0, 0, D3DFMT_LE_X8R8G8B8);
            goto done;
         }

         EndianSwitchDWords(&c, 1);
         
         if (desc.Format == D3DFMT_LE_X2R10G10B10)
         {
            uint b = (c & 1023) >> 2;
            uint g = ((c >> 10) & 1023) >> 2;
            uint r = ((c >> 20) & 1023) >> 2;
            c = D3DCOLOR_ARGB(clearColor >> 24, r, g, b);
         }

         if (c != clearColor)
         {
            G_debug_text.set_buffer(NULL, 0, 0, 0, D3DFMT_LE_X8R8G8B8);
            goto done;
         }
      }
   }

done:
   pFrontBuf->Release();

   pDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_ARGB(255, 0, 0, 0), 0.0f, 0);

   pDevice->Present(NULL, NULL, NULL, NULL);

   gRenderThread.acquireThreadOwnership();
}

void BDebugText::deinit(void)
{
   G_debug_text.set_buffer(NULL, 0, 0, 0, D3DFMT_LE_X8R8G8B8);
}

void BDebugText::renderRaw(uint cell_x, uint cell_y, const char* pBuf)
{
   if (gFinalBuild)
      return;

   if ((gEventDispatcher.getThreadIndex() == cThreadIndexSim) || (gEventDispatcher.getThreadIndex() == cThreadIndexRender))
      gRenderThread.blockUntilGPUIdle();
         
   G_debug_text.render_raw(cell_x, cell_y, pBuf);
}

uint BDebugText::renderCooked(uint cell_x, uint cell_y, const char* pBuf)
{
   if ((gEventDispatcher.getThreadIndex() == cThreadIndexSim) || (gEventDispatcher.getThreadIndex() == cThreadIndexRender))
      gRenderThread.blockUntilGPUIdle();
      
   return G_debug_text.render_cooked(cell_x, cell_y, pBuf);
}

uint BDebugText::renderFmt(uint cell_x, uint cell_y, const char *pFmt, ...)
{
   char buf[2048];

   va_list args;
   va_start(args, pFmt);
   _vsnprintf_s(buf, sizeof(buf), pFmt, args);
   va_end(args);		
   
   if ((gEventDispatcher.getThreadIndex() == cThreadIndexSim) || (gEventDispatcher.getThreadIndex() == cThreadIndexRender))
      gRenderThread.blockUntilGPUIdle();

   return G_debug_text.render_cooked(cell_x, cell_y, buf);
}

#endif