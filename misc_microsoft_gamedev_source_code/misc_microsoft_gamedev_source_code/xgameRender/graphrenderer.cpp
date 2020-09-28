//============================================================================
// graphrenderer.cpp
// Ensemble Studios (C) 2007
//============================================================================
#include "xgameRender.h"
#include "graphrenderer.h"
#include "render.h"
#include "renderThread.h"
#include "reloadManager.h"
#include "asyncFileManager.h"
#include "consoleOutput.h"
#include "image.h"
#include "BD3D.h"

#include "color.h"
#include "primDraw2D.h"

BGraphRenderer gGraphRenderer;

const uint cHackMaxSampleSize = 1000;

//============================================================================
//============================================================================
BGraphRenderer::BGraphRenderer():
   mbInitialized(false),
   mbEdgeAntiAlias(true),
   mTextureDotAdditive(cInvalidManagedTextureHandle),
   mTextureDotAdditive2(cInvalidManagedTextureHandle),
   mTextureDotAlphablend(cInvalidManagedTextureHandle),
   mTextureLineAdditive(cInvalidManagedTextureHandle),
   mTextureLineAlphablend(cInvalidManagedTextureHandle),
   mTextureTick(cInvalidManagedTextureHandle)
{

}

//============================================================================
//============================================================================
BGraphRenderer::~BGraphRenderer()
{   
}

//============================================================================
//============================================================================
bool BGraphRenderer::init()
{
   ASSERT_MAIN_OR_WORKER_THREAD
   eventReceiverInit(cThreadIndexRender);
   commandListenerInit();
   reloadInit();
   loadEffect();
   initVertexDeclarations();   
   return true;
}

//============================================================================
//============================================================================
void BGraphRenderer::deinit()
{
   gRenderThread.blockUntilGPUIdle();
   reloadDeinit();
   commandListenerDeinit();
   eventReceiverDeinit();
   mbInitialized = false;
}

//==============================================================================
// BGraphRenderer::reloadInit
//==============================================================================
void BGraphRenderer::reloadInit(void)
{
   BReloadManager::BPathArray paths;
   BString effectFilename;
   eFileManagerError result =gFileManager.getDirListEntry(effectFilename, gRender.getEffectCompilerDefaultDirID());
   BVERIFY(cFME_SUCCESS == result);

   strPathAddBackSlash(effectFilename);
   effectFilename += "graph\\graph*.bin";
   paths.pushBack(effectFilename);

   gReloadManager.registerClient(paths, BReloadManager::cFlagSynchronous, mEventHandle, eEventClassReloadEffects);
}

//==============================================================================
// BGraphRenderer::reloadDeinit
//==============================================================================
void BGraphRenderer::reloadDeinit(void)
{
   gReloadManager.deregisterClient(mEventHandle);
}

//==============================================================================
//==============================================================================
void BGraphRenderer::loadEffect(void)
{   
   BFixedString256 filename;
   filename.format("graph\\graph.bin");

   BAsyncFileManager::BRequestPacket* pPacket = gAsyncFileManager.newRequestPacket();

   pPacket->setDirID(gRender.getEffectCompilerDefaultDirID());
   pPacket->setFilename(filename);
   pPacket->setReceiverHandle(mEventHandle);
   pPacket->setPrivateData0(0); //-- effect slot;
   pPacket->setSynchronousReply(true);
   gAsyncFileManager.submitRequest(pPacket);
}

//==============================================================================
// BGraphRenderer::receiveEvent
//==============================================================================
bool BGraphRenderer::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   ASSERT_RENDER_THREAD
   switch (event.mEventClass)
   {
      case cEventClassAsyncFile:
         {
            ASSERT_RENDER_THREAD
            initEffect(event);
            break;
         }

      case cEventClassClientRemove:
         {
            ASSERT_RENDER_THREAD
            killEffect();
            break;
         }

      case eEventClassReloadEffects:
         {
            loadEffect();
            break;
         }
   }
   return false;
}

//==============================================================================
// BGraphRenderer::initEffect
//==============================================================================
void BGraphRenderer::initEffect(const BEvent& event)
{
   ASSERT_RENDER_THREAD
   BAsyncFileManager::BRequestPacket* pPacket = reinterpret_cast<BAsyncFileManager::BRequestPacket *>(event.mpPayload);
   BDEBUG_ASSERT(pPacket!=NULL);

   if (!pPacket->getSucceeded())
   {
      gConsoleOutput.output(cMsgError, "BGraphRenderer::initEffect: Async load of file %s failed", pPacket->getFilename().c_str());
   }
   else
   {
      mEffect.clear();
      HRESULT hres = mEffect.createFromCompiledData(BD3D::mpDev, pPacket->getData());
      if (FAILED(hres))
      {
         gConsoleOutput.output(cMsgError, "BFlashMinimapRenderer::initEffect: Effect creation of file %s failed", pPacket->getFilename().c_str());
      }
      else
      {
         trace("BGraphRenderer::initEffect: Effect creation of file %s succeeded", pPacket->getFilename().c_str());
      }

      
      //BDEBUG_ASSERT(mEffect.getNumTechniques() == 2);

      // Get techniques and params
      mTechnique = mEffect.getTechnique("graphDefault");
      mDiffuseTexture = mEffect("gDiffuseSampler");
      BDEBUG_ASSERT(mTechnique.getValid());      
      mbInitialized = true;
   }
}

//==============================================================================
//==============================================================================
void BGraphRenderer::killEffect()
{   
   mTechnique.clear();
   mEffect.clear();   

   mbInitialized = false;
}

//==============================================================================
//==============================================================================
void BGraphRenderer::initVertexDeclarations(void)
{   
   
}

//==============================================================================
// WORKER THREAD FUNCTIONS
//==============================================================================
void BGraphRenderer::initDeviceData(void)
{
   ASSERT_RENDER_THREAD      
   const D3DVERTEXELEMENT9 BLineVertex_Elements[] =
   {  
      { 0, 0,   D3DDECLTYPE_FLOAT4,     D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,  0 },
      { 0, 16,  D3DDECLTYPE_FLOAT2,     D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,  0 },
      { 0, 24,  D3DDECLTYPE_D3DCOLOR ,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,     0 },      
      D3DDECL_END()
   };
   BD3D::mpDev->CreateVertexDeclaration(BLineVertex_Elements, &mpLineVertexDecl);      


   mTextureDotAdditive = gD3DTextureManager.getOrCreateHandle("ui\\flash\\shared\\textures\\misc\\graph\\graphDotAdditive", BFILE_OPEN_NORMAL, BD3DTextureManager::cUI, false, cDefaultTextureRed, true, false, "GraphRenderer");
   mTextureDotAdditive2 = gD3DTextureManager.getOrCreateHandle("ui\\flash\\shared\\textures\\misc\\graph\\graphDotAdditive2", BFILE_OPEN_NORMAL, BD3DTextureManager::cUI, false, cDefaultTextureRed, true, false, "GraphRenderer");
   mTextureDotAlphablend = gD3DTextureManager.getOrCreateHandle("ui\\flash\\shared\\textures\\misc\\graph\\graphDotAlphablend", BFILE_OPEN_NORMAL, BD3DTextureManager::cUI, false, cDefaultTextureRed, true, false, "GraphRenderer");
   mTextureLineAlphablend = gD3DTextureManager.getOrCreateHandle("ui\\flash\\shared\\textures\\misc\\graph\\graphLineAlphablend", BFILE_OPEN_NORMAL, BD3DTextureManager::cUI, false, cDefaultTextureRed, true, false, "GraphRenderer");
   mTextureLineAdditive = gD3DTextureManager.getOrCreateHandle("ui\\flash\\shared\\textures\\misc\\graph\\graphLineAdditive", BFILE_OPEN_NORMAL, BD3DTextureManager::cUI, false, cDefaultTextureRed, true, false, "GraphRenderer");
   mTextureTick = gD3DTextureManager.getOrCreateHandle("ui\\flash\\shared\\textures\\misc\\graph\\ticktexture", BFILE_OPEN_NORMAL, BD3DTextureManager::cUI, false, cDefaultTextureRed, true, false, "GraphRenderer");
}

//==============================================================================
//==============================================================================
void BGraphRenderer::frameBegin(void)
{
   ASSERT_RENDER_THREAD
}
//==============================================================================
// Called from the worker thread to process commands.
//==============================================================================
void BGraphRenderer::processCommand(const BRenderCommandHeader& header, const uchar* pData)
{
   ASSERT_RENDER_THREAD

      switch(header.mType)
   {
      case cGRCommandDrawGraph:      
         {            
            break;
         }      
   };
}

//==============================================================================
// Called from worker thread.
//==============================================================================
void BGraphRenderer::frameEnd(void)
{
   ASSERT_RENDER_THREAD
}

//=============================================================================
// deinit will be called from the worker thread before the RCL is freed, 
// but always before the D3D device is release.
//=============================================================================
void BGraphRenderer::deinitDeviceData(void)
{
   ASSERT_RENDER_THREAD      
   if (mpLineVertexDecl)
   {
      mpLineVertexDecl->Release();
      mpLineVertexDecl = NULL;
   }   
}

//=============================================================================
//=============================================================================
void BGraphRenderer::drawGraphInternal(int techniquePass)
{   
   ASSERT_RENDER_THREAD
   
   // draw   
   int numIcons = 1;
   mTechnique.beginRestoreDefaultState();
      mTechnique.beginPass(techniquePass);                  
         mTechnique.commit();         
         BD3D::mpDev->DrawVertices(D3DPT_POINTLIST, 0, numIcons);
      mTechnique.endPass();
   mTechnique.end();

   // reset renderer
   gRenderDraw.clearStreamSource(0);
   gRenderDraw.setVertexDeclaration(NULL);
}

//=============================================================================
//=============================================================================
void BGraphRenderer::drawBackgroundInternal(const BGraphAttribs& attribs)
{
   if (!attribs.getRenderBackground())
      return;

   int x = (int) attribs.getPosition().getX();
   int y      = (int) attribs.getPosition().getY();
   int width  = (int) attribs.getSizeX();
   int height = (int) attribs.getSizeY();
   DWORD color  = attribs.getBackgroundColor();

   gRenderDraw.setRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
   gRenderDraw.setRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
   gRenderDraw.setRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
   gRenderDraw.setRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
   gRenderDraw.setRenderState(D3DRS_ALPHATESTENABLE, FALSE);

   BPrimDraw2D::drawSolidRect2D(x, y, x+width, y+height, 0, 0, 0, 0, color, cDWORDWhite,  cPosDiffuseVS, cDiffusePS);
}

//=============================================================================
//=============================================================================
void BGraphRenderer::drawLegend(const BGraphAttribs& attribs)
{
   float width   = attribs.mSizeX;
   float height  = attribs.mSizeY;
   float originX = attribs.getPosition().getX();
   float originY = attribs.getPosition().getY()+height;

   int hSmallTickInterval = attribs.getHorizontalSmallTickInterval();
   int hLargeTickInterval = attribs.getHorizontalLargeTickInterval();   
   XMVECTOR hTickPos;  
   
   float smallTickSizeX = attribs.getHorizontalSmallTickSizeX();
   float smallTickSizeY = attribs.getHorizontalSmallTickSizeY();
   float largeTickSizeX = attribs.getHorizontalLargeTickSizeX();
   float largeTickSizeY = attribs.getHorizontalLargeTickSizeY();

   float tickSizeX;
   float tickSizeY;

   // Horizontal ticks
   float x;   
   float alpha;
   XMCOLOR color;
   for (int i = 0; i <= 100; i+=hSmallTickInterval)   
   {
      if ((i % hLargeTickInterval) == 0)
      {
         tickSizeX = largeTickSizeX;
         tickSizeY = largeTickSizeY;  
         color = cDWORDWhite;
      }
      else
      {
         tickSizeX = smallTickSizeX;
         tickSizeY = smallTickSizeY;
         color = cDWORDDarkGrey;
      }

      alpha = Math::Clamp((float)i/100.0f, 0.0f, 1.0f);
      x     = originX + (alpha * width);      
      hTickPos = XMVectorSet(x, originY-(tickSizeY*0.5f), 0, 1.0f);           
      addSprite(hTickPos, BRender2DPrimitiveUtility::eRender2DBlendMode_Alphablend, mTextureTick, color, tickSizeX, tickSizeY, 0.0f, 0.0f, 0, -1);
   }

   /*
   static int vSmallTickInterval = 5;
   static int vLargeTickInterval = 10;
   XMVECTOR vTickPos;
   // vertical ticks
   static bool bUseVTicks = false;
   
   if (bUseVTicks)
   {
      float y;   
      for (int i = 0; i <= 100; i+=vSmallTickInterval)   
      {
         if ((i % vLargeTickInterval) == 0)
         {
            tickSizeX = largeTickSizeY;
            tickSizeY = largeTickSizeX;         
         }
         else
         {
            tickSizeX = smallTickSizeX;
            tickSizeY = smallTickSizeY;
         }

         alpha = Math::Clamp((float)i/100.0f, 0.0f, 1.0f);
         y = originY - (alpha * height);      
         vTickPos = XMVectorSet(originX+(tickSizeX*0.5f), y, 0, 1.0f);           
         addSprite(vTickPos, BRender2DPrimitiveUtility::eRender2DBlendMode_Alphablend, attribs.mTickTexture, cDWORDWhite, tickSizeX, tickSizeY, 0.0f, 0.0f, 0, -1);
      }
   }
   else
   {
      static int numBars = 4;
      XMCOLOR barColor = cDWORDDarkGrey;
      XMCOLOR color1;
      XMCOLOR color2;
      XMVECTOR vBarColor = XMLoadColor(&barColor);

      static float barOpacity1 = 0.25f;
      static float barOpacity2 = 0.4f;

      vBarColor.w = barOpacity1;      
      XMStoreColor(&color1, vBarColor);
      vBarColor.w = barOpacity2;     
      XMStoreColor(&color2, vBarColor);            

      XMCOLOR curColor;
      for (int i = 0; i <numBars; i++)
      {
         float p1Alpha = ( (float) i     / (float) numBars);
         float p2Alpha = ( ((float) i+1) / (float) numBars);

         float delta   = Math::Clamp(p2Alpha - p1Alpha, 0.0f, 1.0f);
         
         float barStart  = p1Alpha * height;
         float barHeight = delta   * height;
         vTickPos = XMVectorSet(originX+(width * 0.5f), originY - (barHeight * 0.5f) - barStart , 0, 1);

         if (i % 2 == 0)
            curColor = color1;
         else 
            curColor = color2;

         addSprite(vTickPos, BRender2DPrimitiveUtility::eRender2DBlendMode_Alphablend, attribs.mTickTexture, curColor, width, barHeight, 0.0f, 0.0f, 0, -1);
      }
   }
   */
}

//=============================================================================
//=============================================================================
void BGraphRenderer::tesselateLine(float x1, float y1, float x2, float y2, float thickness, DWORD color, void* pBuffer)
{
   BLineVertex* pVB = (BLineVertex*) pBuffer;
   
   BVector pt1;
   BVector pt2;

   pt1.set(x1,y1,0.0f);
   pt2.set(x2,y2,0.0f);

   XMCOLOR lineColor;
   XMCOLOR edgeColor;
   lineColor.c = color;
   edgeColor.c = lineColor;
   edgeColor.a = 0;
      
   float aliasScalar = thickness * 0.5f;

   BVector forward = pt2-pt1;
   forward.normalize();

   BVector antialiasForward;
   BVector antialiasRight;
   BVector right;
   right.set(-forward.y, forward.x, 0.0f);
   right.normalize();

   antialiasForward = forward;
   antialiasForward.scale(aliasScalar);
   antialiasRight=right;
   antialiasRight.scale(aliasScalar);

   right.scale(thickness);
         
   //-- main line quad
   pVB[0].x = pt1.x - right.x;
   pVB[0].y = pt1.y - right.y;
   pVB[0].z = 0.0f;
   pVB[0].rhw=1.0f;
   pVB[0].diffuse = color;
   pVB[0].tu = 0.0f;
   pVB[0].tv = 0.0f;

   pVB[1].x = pt2.x - right.x;
   pVB[1].y = pt2.y - right.y;
   pVB[1].z = 0.0f;
   pVB[1].rhw=1.0f;
   pVB[1].diffuse = color;
   pVB[1].tu=1.0f;
   pVB[1].tv=0.0f;

   pVB[2].x = pt1.x + right.x;
   pVB[2].y = pt1.y + right.y;
   pVB[2].z = 0.0f;
   pVB[2].rhw=1.0f;
   pVB[2].diffuse = color;
   pVB[2].tu = 0.0f;
   pVB[2].tv = 1.0f;

   pVB[3] = pVB[2];
   pVB[4] = pVB[1];

   pVB[5].x = pt2.x + right.x;
   pVB[5].y = pt2.y + right.y;
   pVB[5].z = 0.0f;
   pVB[5].rhw=1.0f;
   pVB[5].tu =1.0f;
   pVB[5].tv =1.0f;
   pVB[5].diffuse = color;
      
   if (mbEdgeAntiAlias)
   {

      // left edge antialias verts
      pVB[6]   = pVB[0];      
      pVB[7]   = pVB[0];
      pVB[7].x = pVB[7].x - antialiasRight.x - antialiasForward.x;
      pVB[7].y = pVB[7].y - antialiasRight.y - antialiasForward.y;
      pVB[7].diffuse = edgeColor;
      pVB[8]   = pVB[1];
      pVB[8].x = pVB[8].x - antialiasRight.x + antialiasForward.x;
      pVB[8].y = pVB[8].y - antialiasRight.y + antialiasForward.y;
      pVB[8].diffuse = edgeColor;

      pVB[9]   = pVB[0];
      pVB[10]  = pVB[8];
      pVB[11]  = pVB[1];

      //-- top edge antialas verts
      pVB[12]  = pVB[1];
      pVB[13]  = pVB[8];
      pVB[14]  = pVB[5];

      pVB[15]  = pVB[5];
      pVB[16]  = pVB[8];
      pVB[17]  = pVB[5];
      pVB[17].x =pVB[17].x + antialiasRight.x + antialiasForward.x;
      pVB[17].y =pVB[17].y + antialiasRight.y + antialiasForward.y;
      pVB[17].diffuse = edgeColor;


      //-- right edge
      pVB[18]  = pVB[5];
      pVB[19]  = pVB[17];
      pVB[20]  = pVB[2];
      pVB[20].x = pVB[20].x + antialiasRight.x - antialiasForward.x;
      pVB[20].y = pVB[20].y + antialiasRight.y - antialiasForward.y;
      pVB[20].diffuse = edgeColor;

      pVB[21] = pVB[5];
      pVB[22] = pVB[20];
      pVB[23] = pVB[2];

      //-- bottom edge
      pVB[24] = pVB[2];
      pVB[25] = pVB[20];
      pVB[26] = pVB[0];
      
      pVB[27] = pVB[0];
      pVB[28] = pVB[20];
      pVB[29] = pVB[7];            
   }
}
//=============================================================================
//=============================================================================
void BGraphRenderer::drawGraphTimelinesInternal3(const BGraphAttribs& attribs, const BGraphRenderTimelineAttribs& renderAttribs)
{   
   float width   = attribs.mSizeX;
   float height  = attribs.mSizeY;
   float originX = attribs.getPosition().getX();
   float originY = attribs.getPosition().getY()+height;   

   if (attribs.mTimelines.getSize() < 1)
      return;
            
   int dotBlendMode = BRender2DPrimitiveUtility::eRender2DBlendMode_Alphablend;
   BManagedTextureHandle textureHandle = mTextureLineAlphablend;
   BManagedTextureHandle dotTextureHandle = mTextureDotAlphablend;
  
   if (renderAttribs.mBlendmode == cBlendModeAdditive)
      textureHandle = mTextureLineAdditive;

   if (renderAttribs.mDotBlendmode == cBlendModeAdditive)
      dotTextureHandle = mTextureDotAdditive;
   
   BD3DTextureManager::BManagedTexture* pTexture = NULL;
   if (renderAttribs.mUseTexture)
   {
      pTexture = gD3DTextureManager.getManagedTextureByHandle(textureHandle);
      if (!pTexture)
         return;
   }

   float dotSize = renderAttribs.mDotSizeX;
   
   setBlendMode(renderAttribs.mBlendmode);

   gRenderDraw.setRenderState(D3DRS_ALPHATESTENABLE, FALSE);
   gRenderDraw.setRenderState(D3DRS_VIEWPORTENABLE, FALSE);
   
   for (int i =0 ; i < attribs.mTimelines.getNumber(); ++i)
   {      
      int sampleCount = attribs.mTimelines[i].mData.getNumber();
      if (sampleCount < 2)
         continue;

      //--BTK Temp Hack to fix crash bug PHX-4861
      //-- Fix this the right way after the alpha
      if (sampleCount > cHackMaxSampleSize)
         continue;

      int lineCount    = sampleCount-1;
      
      int vertsPerLine = mbEdgeAntiAlias ? 30 : 6;
      float curHPos = originX + (width * (float) attribs.mTimelines[i].mFadeAlpha);

      int maxLineVerts = vertsPerLine*lineCount;
      
      BLineVertex* pVB=(BLineVertex*)gRenderDraw.lockDynamicVB(maxLineVerts, sizeof(pVB[0]));
      BLineVertex* v = pVB;

      float p1Alpha = 0.0f;
      float p2Alpha = 0.0f;
      float x1,x2,y1,y2;
      int   actualLineCount = 0;
      bool bStop = false;
      for (int j = 0; j < lineCount && !bStop; ++j)
      {
         p1Alpha = (float) j / (float) (lineCount);
         p2Alpha = (float) (j+1) / (float) (lineCount);

         x1 = originX + (p1Alpha * width);         
         x2 = originX + (p2Alpha * width);

         y1 = originY - (float) attribs.mTimelines[i].mData[j] * height;
         y2 = originY - (float) attribs.mTimelines[i].mData[j+1] * height;

         if (x2 >= curHPos)
         {
            float segmentWidth = x2 - x1;
            float curSegmentAlpha = 0.0f;                
            if (segmentWidth > cFloatCompareEpsilon)
               curSegmentAlpha = Math::Clamp((curHPos-x1) / segmentWidth, 0.0f, 1.0f);

            x2 = curHPos;
            y2 = Math::Lerp(y1, y2, curSegmentAlpha);            
            bStop = true;            
         }
                  
         if (lineCount == 1)
         {
            x1 = originX+20;
            x2 = originX+40;
            y1 = originY;
            y2 = originY-300;
         }

         XMVECTOR p  = XMVectorSet(x1, y1, 0.0f, 1.0f);   
         XMVECTOR p2 = XMVectorSet(x2, y2, 0.0f, 1.0f);   

         XMCOLOR color = renderAttribs.mOverrideLineColor ? renderAttribs.mLineColor : attribs.mTimelines[i].mColor;
         XMCOLOR dotColor = renderAttribs.mOverrideDotColor ? renderAttribs.mDotColor : attribs.mTimelines[i].mColor;
         XMVECTOR colorV = XMLoadColor(&color);
         
         if (renderAttribs.mBlendmode == cBlendModeAlpha)
            colorV.w = renderAttribs.mColorAlpha;
         else
         {
            colorV.x *= renderAttribs.mColorAlpha;
            colorV.y *= renderAttribs.mColorAlpha;
            colorV.z *= renderAttribs.mColorAlpha;
         }
         XMStoreColor(&color, colorV);
                 
         if (renderAttribs.mRenderDots)
         {
            addSprite(p, dotBlendMode, dotTextureHandle, dotColor, dotSize, dotSize, 0, 0, 255, -1);

            if (j == (lineCount - 1))
               addSprite(p2, dotBlendMode, dotTextureHandle, dotColor, dotSize, dotSize, 0, 0, 255, -1);
         }

         tesselateLine(x1,y1,x2,y2, renderAttribs.mLineSize, color, v);        
         v+=vertsPerLine;
         actualLineCount++;         
      }

      int actualLineVerts = actualLineCount * vertsPerLine;

      // We're done copying in data, so unlock.
      gRenderDraw.unlockDynamicVB();

      // declare vertex type
      BD3D::mpDev->SetVertexDeclaration(mpLineVertexDecl);

      // Set the stream source to the dynamic vb.
      gRenderDraw.setStreamSource(0, gRenderDraw.getDynamicVB(), 0, sizeof(v[0]));
      
      // update the intrinsics
      mEffect.updateIntrinsicParams();

      uint techniquePassIndex = renderAttribs.mUseTexture ? 2 : 1;
      mTechnique.beginRestoreDefaultState();
         mTechnique.beginPass(techniquePassIndex); 
            
            if (renderAttribs.mUseTexture)
               mDiffuseTexture = pTexture->getD3DTexture().getTexture();

            mTechnique.commit();               
            BD3D::mpDev->DrawVertices(D3DPT_TRIANGLELIST, 0, actualLineVerts);
         mTechnique.endPass();
      mTechnique.end();

      BD3D::mpDev->SetVertexDeclaration(NULL);
      BD3D::mpDev->SetStreamSource(0, NULL, 0, 0);
      BD3D::mpDev->SetPixelShader(NULL);
      BD3D::mpDev->SetVertexShader(NULL);
   }
}

//=============================================================================
//=============================================================================
void BGraphRenderer::drawGraphTimelinesDots(const BGraphAttribs& attribs, const BGraphRenderTimelineAttribs& renderAttribs)
{   
   float width   = attribs.mSizeX;
   float height  = attribs.mSizeY;
   float originX = attribs.getPosition().getX();
   float originY = attribs.getPosition().getY()+height;   
         
   for (int i =0 ; i < attribs.mTimelines.getNumber(); ++i)
   {      
      int sampleCount = attribs.mTimelines[i].mData.getNumber();
      if (sampleCount < 2)
         continue;

      //--BTK Temp Hack to fix crash bug PHX-4861
      //-- Fix this the right way after the alpha
      if (sampleCount > cHackMaxSampleSize)
         continue;

      int lineCount    = sampleCount-1;            
      float curHPos = originX + (width * (float) attribs.mTimelines[i].mFadeAlpha);
      
      float p1Alpha = 0.0f;
      float p2Alpha = 0.0f;
      float x1,x2,y1,y2;      
      bool bStop = false;
      
      static float highlightAlpha = 0.0f;
      highlightAlpha += 0.01f;
      if (highlightAlpha > 1.0f)
         highlightAlpha = 0.0f;


      int blendmode = renderAttribs.mBlendmode == cBlendModeAlpha ? BRender2DPrimitiveUtility::eRender2DBlendMode_Alphablend : BRender2DPrimitiveUtility::eRender2DBlendMode_Additive;
      BManagedTextureHandle textureHandle = renderAttribs.mTextureHandle;

      for (int j = 0; j < lineCount && !bStop; ++j)
      {
         p1Alpha = (float) j / (float) (lineCount);
         p2Alpha = (float) (j+1) / (float) (lineCount);

         x1 = originX + (p1Alpha * width);         
         x2 = originX + (p2Alpha * width);

         y1 = originY - (float) attribs.mTimelines[i].mData[j] * height;
         y2 = originY - (float) attribs.mTimelines[i].mData[j+1] * height;
         
         if (x2 >= curHPos)
         {
            float segmentWidth = x2 - x1;
            float curSegmentAlpha = 0.0f;                
            if (segmentWidth > cFloatCompareEpsilon)
               curSegmentAlpha = Math::Clamp((curHPos-x1) / segmentWidth, 0.0f, 1.0f);

            x2 = curHPos;
            y2 = Math::Lerp(y1, y2, curSegmentAlpha);            
            bStop = true;            
         }

         //-- highlight
         /*
         if (attribs.mTimelines[i].mbEnableHighlight)
         {
            if (highlightAlpha >= p1Alpha)
               if (highlightAlpha <= p2Alpha)
               {
                  float delta = highlightAlpha - p1Alpha;
                  float range = p2Alpha - p1Alpha;
                  if (range > cFloatCompareEpsilon)
                  {
                     float spriteAlphaForHighlight = Math::Clamp(delta/range, 0.0f, 1.0f);
                     
                     float hX = Math::Lerp(x1, x2, spriteAlphaForHighlight);
                     float hY = Math::Lerp(y1, y2, spriteAlphaForHighlight);                        
                     XMVECTOR p = XMVectorSet(hX, hY, 0.0f, 1.0f);
                     addSprite(p, BRender2DPrimitiveUtility::eRender2DBlendMode_Additive, mTextureDotAdditive, cDWORDWhite, 40, 40, 0, 0, 255, -1);                  
                  }
               }
         }
         */
                  
         if (lineCount == 1)
         {
            x1 = originX+20;
            x2 = originX+40;
            y1 = originY;
            y2 = originY-300;
         }
                  
         float dotInterval = renderAttribs.mDotInterval; //attribs.mTimelines[i].mDotInterval;
         float dotSizeX    = renderAttribs.mDotSizeX;
         float dotSizeY    = renderAttribs.mDotSizeY;//attribs.mTimelines[i].mSizeY;
         int curMaxDots = 0;
         XMVECTOR position;
         XMVECTOR a = XMVectorSet(x1, y1, 0, 0.0);
         XMVECTOR b = XMVectorSet(x2, y2, 0, 0.0);
         XMVECTOR length = XMVector3Length(XMVectorSubtract(b,a));
         XMVECTOR sizeV  = XMVectorReciprocal(XMVectorSplatX(XMLoadScalar(&dotInterval)));
         XMVECTOR tempIndex = XMConvertVectorFloatToInt(XMVectorTruncate(XMVectorMultiply(length, sizeV)), 0);         
         XMStoreScalar(&curMaxDots, tempIndex);

         int layerID = 200;
         
         if (attribs.mTimelines[i].mbIsSelected)
         {            
            layerID  = 225;            
         }

         float dotAlpha;
         float dotY;
         float dotX;

         XMCOLOR color = renderAttribs.mOverrideLineColor ? renderAttribs.mLineColor : attribs.mTimelines[i].mColor;
         
         for (int dotCount = 0; dotCount <= curMaxDots; ++dotCount)
         {
            dotAlpha = (float) dotCount / (float) curMaxDots;
            dotY = Math::Lerp(y1, y2, dotAlpha);
            dotX = Math::Lerp(x1, x2, dotAlpha);

            position = XMVectorSet(dotX, dotY, 0.0f, 1.0f);                   
            addSprite(position, blendmode, textureHandle, color.c, dotSizeX, dotSizeY, 0, 0, layerID, -1);            
         }                  
      }      
   }
}

//=============================================================================
//=============================================================================
void BGraphRenderer::drawFilledGraphTimelinesInternal(const BGraphAttribs& attribs)
{  
   if (!attribs.getRenderFilled())
      return;

   float width   = attribs.mSizeX;
   float height  = attribs.mSizeY;
   float originX = attribs.getPosition().getX();
   float originY = attribs.getPosition().getY()+height;
   static float thickness = 2.0f;      
 
   setBlendMode(cBlendModeAlpha);
      
   gRenderDraw.setRenderState(D3DRS_ALPHATESTENABLE, FALSE);
   gRenderDraw.setRenderState(D3DRS_VIEWPORTENABLE, FALSE);

   XMCOLOR color;
   for (int i =0 ; i < attribs.mTimelines.getNumber(); ++i)
   {      
      int sampleCount = attribs.mTimelines[i].mData.getNumber();
      if (sampleCount < 2)
         continue;

      //--BTK Temp Hack to fix crash bug PHX-4861
      //-- Fix this the right way after the alpha
      if (sampleCount > cHackMaxSampleSize)
         continue;

      int lineCount    = sampleCount-1;      
      int vertsPerLine = 6;

      int numLineVerts = vertsPerLine*lineCount;
      BLineVertex* pVB=(BLineVertex*)gRenderDraw.lockDynamicVB(numLineVerts, sizeof(pVB[0]));
      BLineVertex* v = pVB;

      float p1Alpha = 0.0f;
      float p2Alpha = 0.0f;
      float x1,x2,y1,y2;
      float yDelta;
      float xDelta;
      float slope;

      
      color.c = attribs.mTimelines[i].mColor;
      color.a = 128;
      color.c = cDWORDGrey;
      for (int j = 0; j < lineCount; ++j)
      {
         p1Alpha = (float) j / (float) (lineCount);
         p2Alpha = (float) (j+1) / (float) (lineCount);

         x1 = originX + (p1Alpha * width);         
         x2 = originX + (p2Alpha * width);

         y1 = originY - ((float) attribs.mTimelines[i].mData[j] * height * (float) attribs.mTimelines[i].mFadeAlpha);
         y2 = originY - ((float) attribs.mTimelines[i].mData[j+1] * height * (float) attribs.mTimelines[i].mFadeAlpha);


         yDelta = y2-y1;
         xDelta = x2-x1;
         slope = 0.0f;     

         v[0].x = x1;
         v[0].y = originY;
         v[0].z = 0.0f;                        
         v[0].rhw=1.0f;
         v[0].diffuse=color.c;

         v[1].x=float(x1);
         v[1].y=float(y1);
         v[1].z=0.0f;
         v[1].rhw=1.0f;
         v[1].diffuse=color.c;

         v[2].x=float(x2);
         v[2].y=float(y2);
         v[2].z=0.0f;
         v[2].rhw=1.0f;
         v[2].diffuse=color.c;

         v[3] = v[0];
         v[4] = v[2];

         v[5].x = x2;
         v[5].y = originY;
         v[5].z = 0.0f;
         v[5].rhw = 1.0f;
         v[5].diffuse=color.c;

         v+=vertsPerLine;
      }

      // We're done copying in data, so unlock.
      gRenderDraw.unlockDynamicVB();

      // declare vertex type
      BD3D::mpDev->SetVertexDeclaration(mpLineVertexDecl);

      // Set the stream source to the dynamic vb.
      gRenderDraw.setStreamSource(0, gRenderDraw.getDynamicVB(), 0, sizeof(v[0]));

      // update the intrinsics
      mEffect.updateIntrinsicParams();

      static uint techniquePassIndex = 1;
      mTechnique.beginRestoreDefaultState();
      mTechnique.beginPass(techniquePassIndex);

      mTechnique.commit();   
      
      BD3D::mpDev->DrawVertices(D3DPT_TRIANGLELIST, 0, numLineVerts);

      mTechnique.endPass();
      mTechnique.end();

      BD3D::mpDev->SetVertexDeclaration(NULL);
      BD3D::mpDev->SetStreamSource(0, NULL, 0, 0);
      BD3D::mpDev->SetPixelShader(NULL);
      BD3D::mpDev->SetVertexShader(NULL);
   }
}

//=============================================================================1
//=============================================================================
void BGraphRenderer::setBlendMode(uint blendmode)
{
   DWORD source = D3DBLEND_ONE;
   DWORD dest   = D3DBLEND_ZERO;      
   switch (blendmode)
   {
      case cBlendModeAlpha:
         source = D3DBLEND_SRCALPHA;
         dest   = D3DBLEND_INVSRCALPHA;
         break;
      case cBlendModeAdditive:
         source = D3DBLEND_ONE;
         dest   = D3DBLEND_ONE;
         break;                  
   }

   gRenderDraw.setRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
   gRenderDraw.setRenderState(D3DRS_SRCBLEND,  source);
   gRenderDraw.setRenderState(D3DRS_DESTBLEND, dest);
   gRenderDraw.setRenderState(D3DRS_BLENDOP,   D3DBLENDOP_ADD);
}

//=============================================================================1
//=============================================================================
void BGraphRenderer::render(const BGraphAttribs& attribs)
{
   ASSERT_RENDER_THREAD
   if (!mbInitialized)
      return;

   static bool bWireFrame = false;
   gRenderDraw.setRenderState(D3DRS_FILLMODE, bWireFrame ? D3DFILL_WIREFRAME : D3DFILL_SOLID);

   drawBackgroundInternal(attribs);  

   drawLegend(attribs);
           
   bool drawDots = true;
   if (attribs.getType() == BGraphAttribs::cLineGraphDots  || drawDots)
   {
      BGraphRenderTimelineAttribs renderAttribs;

      float cLineSizeX = 25.0;
      float cLineSizeY = 25.0;
      float dotInterval = 3.0;
      bool  bUseAlphablend = false;

      float cGlowLineSizeX = 7.0;
      float cGlowLineSizeY = 7.0;
      float dGlowDotInterval = 1.0f;
      bool  bGlowUseAlphablend = false;

      renderAttribs.mDotInterval = dotInterval;
      renderAttribs.mDotSizeX    = cLineSizeX;
      renderAttribs.mDotSizeY    = cLineSizeY;
      renderAttribs.mLineColor   = cDWORDWhite;
      renderAttribs.mOverrideLineColor = false;
      renderAttribs.mBlendmode = bUseAlphablend ? cBlendModeAlpha : cBlendModeAdditive;
      renderAttribs.mTextureHandle = mTextureDotAdditive;
      drawGraphTimelinesDots(attribs, renderAttribs);

      renderAttribs.mDotInterval = dGlowDotInterval;
      renderAttribs.mDotSizeX    = cGlowLineSizeX;
      renderAttribs.mDotSizeY    = cGlowLineSizeY;
      renderAttribs.mLineColor   = cDWORDWhite;
      renderAttribs.mOverrideLineColor = true;
      renderAttribs.mBlendmode = bGlowUseAlphablend ? cBlendModeAlpha : cBlendModeAdditive;
      renderAttribs.mTextureHandle = mTextureDotAdditive2;
      drawGraphTimelinesDots(attribs, renderAttribs);
   }
   else
   {      
      static float cLineSize = 6.0f;      
      static bool  bUseTexture = true;
      static bool  bRenderDots = true;
      static float dotSize = 20;
      static bool bUseAdditiveBlendmode = false;
      static bool bDotUseAdditiveBlendmode = false;
      static DWORD lineColor = cDWORDWhite;
      static DWORD dotColor  = cDWORDWhite;
      static bool  bOverrideDotColor = false;



      static float cGlowLineSize = 1.0f;      
      static bool  bGlowUseTexture = true;
      static bool  bGlowRenderDots = true;
      static float glowDotSize = 20;      
      static bool  bGlowUseAdditiveBlendmode = false;
      static bool bGlowDotUseAdditiveBlendmode = false;
      static DWORD glowLineColor = cDWORDWhite;
      static DWORD glowDotColor  = cDWORDWhite;
      static bool  bGlowOverrideDotColor = false;

      

      static float colorAlpha = 1.0f;   
      static bool bUsePlayerColorForDot = false;
      static XMCOLOR glowColor = cDWORDWhite;
            
      BGraphRenderTimelineAttribs renderAttribs;

      renderAttribs.mBlendmode = bUseAdditiveBlendmode ? cBlendModeAdditive : cBlendModeAlpha;
      renderAttribs.mDotBlendmode = bDotUseAdditiveBlendmode ? cBlendModeAdditive : cBlendModeAlpha;
      renderAttribs.mColorAlpha = 1.0f;
      renderAttribs.mLineColor = lineColor;
      renderAttribs.mDotColor = dotColor;
      renderAttribs.mLineSize = cLineSize;
      renderAttribs.mRenderDots = bRenderDots;
      renderAttribs.mUseTexture = bUseTexture;
      renderAttribs.mOverrideLineColor = false;
      renderAttribs.mOverrideDotColor = bOverrideDotColor;
      renderAttribs.mDotSizeX = dotSize;
      renderAttribs.mDotSizeY = dotSize;
      drawGraphTimelinesInternal3(attribs, renderAttribs);

      renderAttribs.mBlendmode = bGlowUseAdditiveBlendmode? cBlendModeAdditive : cBlendModeAlpha;
      renderAttribs.mDotBlendmode = bGlowDotUseAdditiveBlendmode ? cBlendModeAdditive : cBlendModeAlpha;
      renderAttribs.mColorAlpha = 1.0f;
      renderAttribs.mLineColor = glowLineColor;
      renderAttribs.mDotColor = glowDotColor;
      renderAttribs.mLineSize = cGlowLineSize;
      renderAttribs.mRenderDots = bGlowRenderDots;
      renderAttribs.mUseTexture = bGlowUseTexture;
      renderAttribs.mOverrideLineColor = true;
      renderAttribs.mOverrideDotColor = bGlowOverrideDotColor;
      renderAttribs.mDotSizeX = glowDotSize;
      renderAttribs.mDotSizeY = glowDotSize;
      drawGraphTimelinesInternal3(attribs, renderAttribs);
   }

   BR2DPUUpdateData updateData;
   updateData.mCount = mSpritePrimitives.getSize();
   updateData.mpData = mSpritePrimitives.getPtr();
   updateData.mThreadIndex = cThreadIndexRender;
   gRender2DPrimitiveUtility.workerUpdate(BRender2DPrimitiveUtility::eBR2DUUpdate2DSprite, &updateData);

   int threadIndex = cThreadIndexRender;
   gRender2DPrimitiveUtility.workerRender(&threadIndex);

   clearPrimitives(mSpritePrimitives, -1);
}

//==============================================================================
// template<class T> void BUIManager::clearPrimitives(T& list, int category);
//==============================================================================
template<class T> void BGraphRenderer::clearPrimitives(T& list, int category)
{
   if (category == -1)
   {    
      list.resize(0);
      return;
   }

   for (uint i = 0; i < list.getSize();i++)
   {      
      //-- is this primitive dead?
      if ((list[i].mCategory == category))
      {
         list[i] = list.back();
         list.popBack();
         i--;
      }
   }
}

//=============================================================================1
//=============================================================================
void BGraphRenderer::addSprite(XMVECTOR position, int blendMode, BManagedTextureHandle texture, DWORD color, float scaleX, float scaleY, float offsetX, float offsetY, int layer, float timeout)
{
   XMMATRIX matrix = XMMatrixIdentity();
   matrix.r[3] = position;

   B2DPrimitiveSprite* pPrim = mSpritePrimitives.pushBackNoConstruction(1);
   pPrim->clear();
   pPrim->mTextureHandle = texture;
   pPrim->mMatrix = matrix;
   pPrim->mColor  = color;
   pPrim->mOffsetX = offsetX;
   pPrim->mOffsetY = offsetY;
   pPrim->mScaleX = scaleX;
   pPrim->mScaleY = scaleY;
   pPrim->mTimeout = -1;
   pPrim->mBlendMode = blendMode;
   pPrim->mCategory  = 0;
   pPrim->mLayer = layer;
   return;
}