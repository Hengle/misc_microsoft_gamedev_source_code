// local
#include "terrainPCH.h"
#include "terrain.h"
#include "TerrainDynamicAlpha.h"

//--------------------------------------
BTerrainDynamicAlpha gTerrainDynamicAlpha;
//============================================================================
// BTerrainDynamicAlpha::BTerrainDynamicAlpha
//============================================================================
BTerrainDynamicAlpha::BTerrainDynamicAlpha():
mpDynamicAlphaTexture(0),
mWidth(0),
mHeight(0)
{
} 
//============================================================================
// BTerrainDynamicAlpha::~BTerrainDynamicAlpha
//============================================================================
BTerrainDynamicAlpha::~BTerrainDynamicAlpha()
{
}

//============================================================================
// BTerrainDynamicAlpha::init
//============================================================================
bool BTerrainDynamicAlpha::init(void)
{
   ASSERT_MAIN_THREAD


      commandListenerInit();

   return true;
}
//============================================================================
// BTerrainDynamicAlpha::deinit
//============================================================================
bool BTerrainDynamicAlpha::deinit(void)
{
   ASSERT_MAIN_THREAD

      // Block for safety. 
      gRenderThread.blockUntilGPUIdle();

   commandListenerDeinit();


   return true;
}
//============================================================================
// BTerrainDynamicAlpha::initDeviceData
//============================================================================
void BTerrainDynamicAlpha::initDeviceData(void)
{
}
//============================================================================
// BTerrainDynamicAlpha::deinitDeviceData
//============================================================================
void BTerrainDynamicAlpha::deinitDeviceData(void)
{
}
//============================================================================
// BTerrainDynamicAlpha::destroy
//============================================================================
void BTerrainDynamicAlpha::destroy()
{
    gRenderThread.submitCommand(mCommandHandle,cTDA_Destroy);
}
//============================================================================
// BTerrainDynamicAlpha::processCommand
//============================================================================
void BTerrainDynamicAlpha::processCommand(const BRenderCommandHeader& header, const unsigned char* pData)
{
   ASSERT_RENDER_THREAD

      switch (header.mType)
   {
      case cTDA_Destroy:
         {
            destroyInternal();
            break;
         }
      case cTDA_SetAlpha:
         {
            setToRegionToValueInternal((dynamicAlphaPacket*)pData);
            break;
         }
   }
}
//============================================================================
// BTerrainDynamicAlpha::destroyInternal
//============================================================================
bool  BTerrainDynamicAlpha::destroyInternal()
{
    ASSERT_RENDER_THREAD
  if(mpDynamicAlphaTexture)
  {
     mpDynamicAlphaTexture->Release();
     mpDynamicAlphaTexture = NULL;
  }

   return true;
}
//============================================================================
// BTerrainDynamicAlpha::createAlphaTexture
//============================================================================
void BTerrainDynamicAlpha::createAlphaTexture()
{
   ASSERT_RENDER_THREAD

   SCOPEDSAMPLE(BTerrainDynamicAlpha_createAlphaTexture)

   mWidth = gTerrainVisual.getNumXVerts() * 4;
   mHeight = gTerrainVisual.getNumXVerts() * 4;
   const uint PackedWidth = mWidth>>2;
   //CLM [11.16.07] added D3DUSAGE_RUNCOMMANDBUFFER_TIMESTAMP to fix an assert w/ command buffers
   BD3D::mpDev->CreateTexture(PackedWidth,mHeight,1,D3DUSAGE_RUNCOMMANDBUFFER_TIMESTAMP,D3DFMT_LIN_DXT3A_1111,0,&mpDynamicAlphaTexture,0);

   //numXBlocks-1 + numXBlocks * numYBlocks
   const uint sizeToSet = (((mWidth>>4)-1) + (mWidth>>4) * ((mHeight>>2)-1)) * sizeof(int64);

   D3DLOCKED_RECT rect;
   mpDynamicAlphaTexture->LockRect(0,&rect,0,0);
   byte* pDat = (byte*)rect.pBits; 
   memset(pDat,0xFF,sizeToSet);
   mpDynamicAlphaTexture->UnlockRect(0);
}

//============================================================================
// BTerrainDynamicAlpha::setToRegionToValue
//============================================================================
void BTerrainDynamicAlpha::setToRegionToValueRectangleAligned(float minX, float minY, float maxX, float maxY, bool value)
{ 
   
      ASSERT_THREAD(cThreadIndexSim);
   dynamicAlphaPacket dp;
   dp.mAlphaShape = 2;  //rectangle aligned
   dp.mAlphaToState = value?0xFF:0x00;
   dp.mX0 = Math::Clamp<float>(minX,0,1);
   dp.mX1 = Math::Clamp<float>(maxX,0,1);
   dp.mY0 = Math::Clamp<float>(minY,0,1);
   dp.mY1 = Math::Clamp<float>(maxY,0,1);

   gRenderThread.submitCommand(mCommandHandle, cTDA_SetAlpha, dp);
}
//============================================================================
// BTerrainDynamicAlpha::setToRegionToValue
//============================================================================
void BTerrainDynamicAlpha::setToRegionToValueCircle(float centerX, float centerY, float radius, bool value)
{ 

   ASSERT_THREAD(cThreadIndexSim);
   dynamicAlphaPacket dp;
   dp.mAlphaShape = 1;  //circle
   dp.mAlphaToState = value?0xFF:0x00;
   dp.mX0 = Math::Clamp<float>(centerX,0,1);
   dp.mY0 = Math::Clamp<float>(centerY,0,1);
   dp.mY3 = radius;
   

   gRenderThread.submitCommand(mCommandHandle, cTDA_SetAlpha, dp);
}
//============================================================================
// BTerrainDynamicAlpha::setToRegionToValue
//============================================================================
void BTerrainDynamicAlpha::setToRegionToValueRectangleOriented(float x0, float y0, float x1, float y1, float x2, float y2,float x3, float y3, bool value)
{ 

   ASSERT_THREAD(cThreadIndexSim);
   dynamicAlphaPacket dp;
   dp.mAlphaShape = 0;  //rectangle oriented
   dp.mAlphaToState = value?0xFF:0x00;
   dp.mX0 = Math::Clamp<float>(x0,0,1);
   dp.mX1 = Math::Clamp<float>(x1,0,1);
   dp.mX2 = Math::Clamp<float>(x2,0,1);
   dp.mX3 = Math::Clamp<float>(x3,0,1);

   dp.mY0 = Math::Clamp<float>(y0,0,1);
   dp.mY1 = Math::Clamp<float>(y1,0,1);
   dp.mY2 = Math::Clamp<float>(y2,0,1);
   dp.mY3 = Math::Clamp<float>(y3,0,1);

   gRenderThread.submitCommand(mCommandHandle, cTDA_SetAlpha, dp);
}
//============================================================================
// BTerrainDynamicAlpha::setValueToImage
//============================================================================
#define cNumBits	   64
#define cSecAnd		cNumBits-1
#define cTopMask	   0x8000000000000000

//--------------------------------
void setContainerState(__int64 *value, bool state, __int64 mask)
{
   const int istate = state;
   (*value) ^= (-istate ^ (*value)) & mask;
}

//--------------------------------
void setState(__int64 *block, int index, bool state)
{
   const int sIndex = (index & cSecAnd);
   setContainerState(block,state,(cTopMask>>sIndex));
}
//--------------------------------
void BTerrainDynamicAlpha::setValueToImage(__int64* pDat,uint x, uint y, bool value)
{
   BDEBUG_ASSERT(pDat);

   if(x<0 || x>=mWidth || y<0 || y>= mHeight || mpDynamicAlphaTexture==NULL)
      return;

   const int numXBlocks = mWidth>>4;

   //determine our block
   const int xBlockIndex = x >> 4;  // div 16
   const int yBlockIndex = y >> 2;  // div 4

   __int64* block = &pDat[xBlockIndex + numXBlocks *yBlockIndex];

   //determine the position we're joined in
   const int xPixelIndex = 15-(x %16);   //CLM I had to flip this value to get them to appear right with DXT3A_1111
   const int yPixelIndex = (y % 4);   
   const int localIndex = (xPixelIndex + 16 * yPixelIndex); 
   setState(block,localIndex,value);
}
//============================================================================
// BTerrainDynamicAlpha::setCircleToValueInternal
//============================================================================
void BTerrainDynamicAlpha::setCircleToValueInternal(__int64* pDat,float centerX, float centerY, float radius, bool value)
{ 
   SCOPEDSAMPLE(BTerrainDynamicAlpha_setCircleToValueInternal)
   BDEBUG_ASSERT(pDat);

   if(mpDynamicAlphaTexture==NULL)
      return;

   const float rad2 = radius * radius;
   const uint cx = (uint)((centerX) * mWidth);
   const uint cy = (uint)((centerY) * mHeight);

   const uint minx = Math::Clamp<uint>((uint)(cx - radius),0,mWidth - 1);
   const uint miny = Math::Clamp<uint>((uint)(cy - radius),0,mHeight - 1);
   const uint maxx = Math::Clamp<uint>((uint)(cx + radius),0,mWidth - 1);
   const uint maxy = Math::Clamp<uint>((uint)(cy + radius),0,mHeight - 1);


   for(uint y=miny;y<maxy;y++)
   {
      for(uint x=minx;x<maxx;x++)
      {
         const float dist = (float)((cx - x) * (cx - x) + (cy - y) * (cy - y));
         if(dist >= rad2)
            continue;

         setValueToImage(pDat,x,y,value);
      }
   }
}
//============================================================================
// BTerrainDynamicAlpha::setAlignedRectToValueInternal
//============================================================================
void BTerrainDynamicAlpha::setAlignedRectToValueInternal(__int64* pDat,float minX, float maxX, float minZ, float maxZ, bool value)
{ 
   SCOPEDSAMPLE(BTerrainDynamicAlpha_setAlignedRectToValueInternal)
   BDEBUG_ASSERT(pDat);

   if(mpDynamicAlphaTexture==NULL)
      return;

   const uint minx = Math::Clamp<uint>((uint)(minX * mWidth),0,mWidth - 1);
   const uint miny = Math::Clamp<uint>((uint)(minZ * mHeight),0,mHeight - 1);
   const uint maxx = Math::Clamp<uint>((uint)(maxX * mWidth),0,mWidth - 1);
   const uint maxy = Math::Clamp<uint>((uint)(maxZ * mHeight),0,mHeight - 1);

   for(uint y=miny;y<maxy;y++)
   {
      for(uint x=minx;x<maxx;x++)
      {
        setValueToImage(pDat,x,y,value);
      }
   }
}
//============================================================================
// BTerrainDynamicAlpha::setTriangleToValueInternal
//============================================================================
void  BTerrainDynamicAlpha::setOrientedRectToValueInternal(__int64* pDat,float x0, float y0, float x1, float y1, float x2, float y2,float x3, float y3, bool value)
{
   SCOPEDSAMPLE(BTerrainDynamicAlpha_setOrientedRectToValueInternal)
   BDEBUG_ASSERT(pDat);

   if(mpDynamicAlphaTexture==NULL)
      return;


   //draw two triangles for this
   setTriangleToValueInternal(pDat,    x0,y0,   x1,y1,   x2,y2,   value);
   setTriangleToValueInternal(pDat,    x2,y2,   x3,y3,   x0,y0,   value);
}

//============================================================================
// BTerrainDynamicAlpha::setTriangleToValueInternal
//============================================================================

/*=========================
From Graphics Gems III
==========================*/
struct edge 
{
   int ymin, ymax, xi, si;
   int r, inc, dec;
};
int floor_div(int x, int y)
{
   if (x >= 0) return(x/y);
   else return((x/y) + (((x % y) == 0) ? 0 : -1));
}


struct edge *EdgeSetup(struct edge *e, int x0, int y0, int x1, int y1)
{
   /*
   *  Initializes the Bresenham-like scan conversion for a single edge,
   *  setting values in the structure containing the increment variables.
   */

   int sf, dx = x1-x0, dy = y1-y0;

   e->ymin = y0;
   e->ymax = y1;

   if (dy != 0) 
   {
      e->si = floor_div(dx, dy);
      e->xi = x0 + e->si;
      sf = dx - e->si * dy;
      e->r = 2*sf - dy;
      e->inc = sf;
      e->dec = sf - dy;
   }
   return(e);
}

int EdgeScan(struct edge *e)
{
   /*
   *  Returns the intersection of edge e with the next scanline.
   */
   int x = e->xi;

   if (e->r >= 0) 
   {
      e->xi += e->si + 1;
      e->r += e->dec;
   }
   else 
   {
      e->xi += e->si;
      e->r += e->inc;
   }
   return x;
}


void BTerrainDynamicAlpha::setTriangleToValueInternal(__int64* pDat, float x0f, float y0f, float x1f, float y1f, float x2f, float y2f, bool value)
{
   BDEBUG_ASSERT(pDat);

   if(mpDynamicAlphaTexture==NULL)
      return;


   uint x0=(uint)(x0f * mWidth);
   uint y0=(uint)(y0f * mHeight);
   uint x1=(uint)(x1f * mWidth);
   uint y1=(uint)(y1f * mHeight);
   uint x2=(uint)(x2f * mWidth);
   uint y2=(uint)(y2f * mHeight);


   if (y0>y1) { std::swap(y0,y1); std::swap(x0,x1); }
   if (y0>y2) { std::swap(y0,y2); std::swap(x0,x2); }
   if (y1>y2) { std::swap(y1,y2); std::swap(x1,x2); }

  

   int det, yi, xmin, xmax;
   struct edge left, right;

   /* Compute handedness of triangle (points left or right) */
   /* (see Pavlidis '82, [Computer Science Press], Ch 14) */
   det = (y1-y0)*(x2-x0) - (x1-x0)*(y2-y0);

   /* Setup first pair of edges */
   if (det < 0)
   {
      EdgeSetup(&left, x0, y0, x2, y2);
      EdgeSetup(&right, x0, y0, x1, y1); 
   }
   else
   {
      EdgeSetup(&left, x0, y0, x1, y1);
      EdgeSetup(&right, x0, y0, x2, y2); 
   }

   /* Scan first pair of edges. */
   for (yi = left.ymin + 1; yi <= min(left.ymax, right.ymax); yi++) 
   {
      xmin = EdgeScan(&left);
      xmax = EdgeScan(&right);
      for (int x=xmin+1; x<=xmax; x++) setValueToImage(pDat,x, yi, value);
   }

   /* Setup third edge */
   if (det >= 0) EdgeSetup(&left, x1, y1, x2, y2);
   else          EdgeSetup(&right, x1, y1, x2, y2);

   /* Scan remainder of triangle. */
   for (yi = max(left.ymin, right.ymin) + 1; yi <= left.ymax; yi++) 
   {
      xmin = EdgeScan(&left);
      xmax = EdgeScan(&right);
      for (int x=xmin+1; x<=xmax; x++) setValueToImage(pDat,x, yi, value);
   }
}

//============================================================================
// BTerrainDynamicAlpha::setToRegionToValue
//============================================================================
void BTerrainDynamicAlpha::setToRegionToValueInternal(const dynamicAlphaPacket *packet)
{ 
   SCOPEDSAMPLE(BTerrainDynamicAlpha_setToRegionToValueInternal)
   ASSERT_RENDER_THREAD

   if(mpDynamicAlphaTexture==NULL)
      return;



   //CLM added because this is unsafe.
   if (gRenderThread.getHasD3DOwnership())
      gRenderThread.blockUntilGPUIdle();

   const bool value = packet->mAlphaToState?1:0;

   D3DLOCKED_RECT rect;
   mpDynamicAlphaTexture->LockRect(0,&rect,0,0);
   __int64* pDat = (__int64*)rect.pBits; 

   if(packet->mAlphaShape==0)//RECTANGLE ORIENTED
   {
      setOrientedRectToValueInternal(pDat,packet->mX0,
                                          packet->mY0,
                                          packet->mX1,
                                          packet->mY1,
                                          packet->mX2,
                                          packet->mY2,
                                          packet->mX3,
                                          packet->mY3,
                                          value);
   }
   else if(packet->mAlphaShape==1) //CIRCLE
   {
      setCircleToValueInternal(pDat,   packet->mX0, 
                                       packet->mY0, 
                                       packet->mY3,
                                       value); 
   }
   else if(packet->mAlphaShape==2) //RECTANGLE ALIGNED
   {
      setAlignedRectToValueInternal(pDat, packet->mX0, 
                                          packet->mX1, 
                                          packet->mY0, 
                                          packet->mY1, 
                                          value);
   }

   mpDynamicAlphaTexture->UnlockRect(0);
  
}