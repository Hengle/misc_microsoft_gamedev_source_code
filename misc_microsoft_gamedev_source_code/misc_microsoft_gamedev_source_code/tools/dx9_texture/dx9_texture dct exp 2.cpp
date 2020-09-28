//-----------------------------------------------------------------------------
//           Name: dx9_texture.cpp
//         Author: Kevin Harris
//  Last Modified: 02/01/05
//    Description: This sample demonstrates how to texture geometry with 
//                 Direct3D.
//-----------------------------------------------------------------------------

#define STRICT
#define WIN32_LEAN_AND_MEAN

#include "ecore.h"
#include "compression.h"
#include <d3d9.h>
#include <d3dx9.h>
#include "resource.h"

#include <algorithm>
#include <map>

#include "jpegcodec\jpgdecode\writetga.h"
#include "jpegcodec\jpgencode\readtga.h"

#include "bytepacker.h"
#include "alignedarray.h"
#include "colorutils.h"
#include "RGBAImage.h"
#include "imageutils.h"
#include "generalvector.h"
#include "DXTUtils.h"
#include "dxtmunge.h"
#include "dxtunpacker.h"
#include "dxtpacker.h"

#include "bitarray2D.h"
#include "array2D.h"
#include "staticArray2D.h"

#include "EmbeddedDCTCodec.h"

#include "generalmatrix.h"
#include "LinearAlgebra.h"
#include "ImageResample.h"

//-----------------------------------------------------------------------------
// GLOBALS
//-----------------------------------------------------------------------------
HWND                    g_hWnd          = NULL;
LPDIRECT3D9             g_pD3D          = NULL;
LPDIRECT3DDEVICE9       g_pd3dDevice    = NULL;
LPDIRECT3DVERTEXBUFFER9 g_pVertexBuffer = NULL;

LPDIRECT3DTEXTURE9      g_pTexture0     = NULL;
LPDIRECT3DTEXTURE9      g_pTexture1     = NULL;
BVec4                   g_Scale;
BVec4                   g_Ofs;
IDirect3DPixelShader9*  g_pPixelShader = NULL;

#define D3DFVF_CUSTOMVERTEX ( D3DFVF_XYZRHW | D3DFVF_TEX1 )

struct Vertex
{
    float x, y, z, w;
    float tu, tv;
};

int gWindowWidth = 512;
int gWindowHeight = 512;

const Vertex g_quadVertices[] =
{
	{ 0.0f, 0.0f, 0.0f, 1.0f,  0.0f,0.0f },
	{ 1.0f, 0.0f, 0.0f, 1.0f,  1.0f,0.0f },
	{ 0.0f, 1.0f, 0.0f, 1.0f,  0.0f,1.0f },
	{ 1.0f, 1.0f, 0.0f, 1.0f,  1.0f,1.0f }
};

//-----------------------------------------------------------------------------
// PROTOTYPES
//-----------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, 
				   LPSTR lpCmdLine, int nCmdShow);
LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
void loadTexture(float qualityLow, float qualityHigh);
void init(void);
void shutDown(void);
void render(void);

//-----------------------------------------------------------------------------
// Name: WinMain()
// Desc: The application's entry point
//-----------------------------------------------------------------------------
int WINAPI WinMain(	HINSTANCE hInstance,
					HINSTANCE hPrevInstance,
					LPSTR     lpCmdLine,
					int       nCmdShow )
{
	WNDCLASSEX winClass; 
	MSG        uMsg;

    memset(&uMsg,0,sizeof(uMsg));
    
	winClass.lpszClassName = "MY_WINDOWS_CLASS";
	winClass.cbSize        = sizeof(WNDCLASSEX);
	winClass.style         = CS_HREDRAW | CS_VREDRAW;
	winClass.lpfnWndProc   = WindowProc;
	winClass.hInstance     = hInstance;
	winClass.hIcon	       = LoadIcon(hInstance, (LPCTSTR)IDI_DIRECTX_ICON);
    winClass.hIconSm	   = LoadIcon(hInstance, (LPCTSTR)IDI_DIRECTX_ICON);
	winClass.hCursor       = LoadCursor(NULL, IDC_ARROW);
	winClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	winClass.lpszMenuName  = NULL;
	winClass.cbClsExtra    = 0;
	winClass.cbWndExtra    = 0;

	if( !RegisterClassEx(&winClass) )
		return E_FAIL;

   for (int i = 1; i < __argc; i++)
   {
      const char* pOption = __argv[i];
      if (pOption[0] == '-')
      {
         if (pOption[1] == 'w')
         {
            gWindowWidth = atoi(&pOption[2]);
            if (gWindowWidth < 64) gWindowWidth = 64; else if (gWindowWidth > 2048) gWindowWidth = 2048;
         }
         else if (pOption[1] == 'h')
         {
            gWindowHeight = atoi(&pOption[2]);
            if (gWindowHeight < 64) gWindowHeight = 64; else if (gWindowHeight > 2048) gWindowHeight = 2048;
         }
      }         
   }
   
   int nDefaultWidth = gWindowWidth;
   int nDefaultHeight = gWindowHeight;
   DWORD dwWindowStyle = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
   RECT rc;        
   SetRect( &rc, 0, 0, nDefaultWidth, nDefaultHeight );        
   AdjustWindowRect( &rc, dwWindowStyle, false);

	g_hWnd = CreateWindowEx( NULL, "MY_WINDOWS_CLASS", 
                            "Direct3D (DX9) - Texturing",
						          dwWindowStyle,
					             0, 0, (rc.right-rc.left), (rc.bottom-rc.top), NULL, NULL, hInstance, NULL );

	if( g_hWnd == NULL )
		return E_FAIL;

    ShowWindow( g_hWnd, nCmdShow );
    UpdateWindow( g_hWnd );

	init();

	while( uMsg.message != WM_QUIT )
	{
		if( PeekMessage( &uMsg, NULL, 0, 0, PM_REMOVE ) )
		{ 
			TranslateMessage( &uMsg );
			DispatchMessage( &uMsg );
		}
        else
		    render();
	}

	shutDown();

    UnregisterClass( "MY_WINDOWS_CLASS", winClass.hInstance );

	return uMsg.wParam;
}

//-----------------------------------------------------------------------------
// Name: WindowProc()
// Desc: The window's message handler
//-----------------------------------------------------------------------------
LRESULT CALLBACK WindowProc( HWND   hWnd, 
							 UINT   msg, 
							 WPARAM wParam, 
							 LPARAM lParam )
{
    switch( msg )
    {	
      case WM_KEYDOWN:
		{
			switch( wParam )
			{
				case VK_ESCAPE:
					PostQuitMessage(0);
					break;
			}
		}
      break;

		case WM_CLOSE:
		{
			PostQuitMessage(0);	
		}
		
      case WM_DESTROY:
		{
            PostQuitMessage(0);
		}
      break;

		default:
		{
			return DefWindowProc( hWnd, msg, wParam, lParam );
		}
		break;
	}

	return 0;
}

static void debugPrintf(const char* pMsg, ...)
{
   va_list args;
   va_start(args, pMsg);
   char buf[512];
   vsprintf(buf, pMsg, args);
   va_end(args);
   OutputDebugString(buf);
}

static int round(float f)
{
   return (int)(Math::Sign(f)*floor(fabs(f)));
}

static float sqrtSign(float f)
{
   return Math::Sign(f)*sqrt(fabs(f));
}

static BVec4 createScaledImage(const BArray2D<BVec4>& src, BRGBAImage& dst)
{
   dst.setSize(src.getWidth(), src.getHeight());
      
   BVec4 low(1e+30f);
   BVec4 high(-1e+30f);      
   for (uint y = 0; y < src.getHeight(); y++)
   {
      for (uint x = 0; x < src.getWidth(); x++)
      {
         const BVec4& pixel = src(x, y);
         
         low = BVec4::elementMin(low, pixel);
         high = BVec4::elementMax(high, pixel);
      }
   }
   
   BVec4 range(high - low);

   const int cHistSize = 1024;
   BAlignedArray<uint> hist[4];
   for (uint i = 0; i < 4; i++)
      hist[i].resize(cHistSize);
         
   for (uint y = 0; y < src.getHeight(); y++)
   {
      for (uint x = 0; x < src.getWidth(); x++)
      {
         const BVec4& pixel = src(x, y);

         for (uint i = 0; i < 4; i++)
         {
            float bucket = (pixel[i] - low[i]);
            if (range[i])
               bucket /= range[i];
               
            int iBucket = Math::Clamp((int)floor(bucket * (cHistSize - 1) + .5f), 0, cHistSize - 1);
                                       
            hist[i][iBucket]++;
         }
      }
   }
      
   BVec4 aLow(low);
   BVec4 aHigh(high);
         
   for (uint channel = 0; channel < 4; channel++)
   {
      uint total = 0;
      for (uint i = 0; i < cHistSize; i++)
         total += hist[channel][i];

      const uint thresh = (channel == 3) ? 1 : 8;

      uint l;
      uint c = 0;
      for (l = 0; l < cHistSize - 1; l++)
      {
         c += hist[channel][l];
         if (c >= thresh)
            break;
      }

      uint h;
      c = 0;
      for (h = cHistSize - 1; h > l; h--)
      {
         c += hist[channel][h];
         if (c >= thresh)
            break;
      }

      if (l < h)
      {
         aLow[channel] = (l / float(cHistSize - 1)) * range[channel] + low[channel];
         aHigh[channel] = (h / float(cHistSize - 1)) * range[channel] + low[channel];
      }
   }
   
   BVec4 aRange(BVec4::elementMax(BVec4::abs(aLow), BVec4::abs(aHigh)));
   
   float colorRange = Math::Max3(aRange[0], aRange[1], aRange[2]);
   float alphaRange = aRange[3];
   
   // 123 125 123 128
      
   for (uint y = 0; y < src.getHeight(); y++)
   {
      for (uint x = 0; x < src.getWidth(); x++)
      {
         const BVec4& pixel = src(x, y);

         int a = round(pixel[3] / alphaRange * 127.0f) + 128;
         int r = round(pixel[0] / colorRange * 127.0f) + 123;
         int g = round(pixel[1] / colorRange * 127.0f) + 125;
         int b = round(pixel[2] / colorRange * 127.0f) + 123;

         dst(x, y) = BRGBAColor(r, g, b, a);         
      }
   }     
   
   return BVec4(colorRange, colorRange, colorRange, alphaRange);
}

//-----------------------------------------------------------------------------
// Name: loadTexture2()
// Desc: 
//-----------------------------------------------------------------------------
bool loadTexture2(const char* pSrcFilename, const char* pDstFilename, int quality, int codebookSize, bool virtualCodebook, float bitRate, int method, bool subsampling, bool useAveDelta, bool useVisualQuant)
{
   LPDIRECT3DTEXTURE9 pTex = NULL;
   
   HRESULT hres = D3DXCreateTextureFromFileEx( 
      g_pd3dDevice, 
      pSrcFilename, 
      D3DX_DEFAULT,
      D3DX_DEFAULT,
      1,
      0,
      D3DFMT_A8R8G8B8,
      D3DPOOL_SYSTEMMEM,
      D3DX_DEFAULT,
      D3DX_DEFAULT,
      0,
      NULL,
      NULL,
      &pTex);

   if (FAILED(hres))
      return false;

   D3DSURFACE_DESC desc;
   pTex->GetLevelDesc(0, &desc);
               
   int width = desc.Width;
   int pitch = width * 3;
   int height = desc.Height;
   
   if ((width < 4) || (height < 4))
   {
      pTex->Release();
      return false;
   }
   
   const int cellsX = width >> 2;
   const int cellsY = height >> 2;
   const int totalCells = cellsX * cellsY;
   
   BRGBAImage image(width, height);
   
   D3DLOCKED_RECT lockedRect;
   pTex->LockRect(0, &lockedRect, NULL, 0);
      
   for (int y = 0; y < height; y++)
   {
      uchar* pSrc = ((uchar*)lockedRect.pBits) + y * lockedRect.Pitch;
      BRGBAColor* pDst = &image(0, y);
      
      for (int x = 0; x < width; x++)
      {
         pDst->r = pSrc[2];
         pDst->g = pSrc[1];
         pDst->b = pSrc[0];
         pDst->a = pSrc[3];
         
         pSrc += 4;
         pDst++;
      }
   }
   
   pTex->Release();
   pTex = NULL;

   const bool hasAlpha = false;
   D3DFORMAT d3dFormat = D3DFMT_DXT1;
   BDXTFormat dxtFormat = cDXT1;
   
   BAlignedArray<uchar> compressedData;
   
   uint bytesPerBlock = 8;   
   if (dxtFormat >= cDXT3)
      bytesPerBlock = 16;

   BAlignedArray<uchar> dxtImage;//(cellsX * cellsY * bytesPerBlock);

   BMatrixNxN<4,4> dct( BMatrixNxNCreateDCT< BMatrixNxN<4,4> >() );
   
   BArray2D<BVec4> fdctImage[4];
   for (uint i = 0; i < 4; i++)
      fdctImage[i].resize(cellsX, cellsY);
            
   for (uint sy = 0; sy < cellsY; sy++)
   {
      for (uint sx = 0; sx < cellsX; sx++)
      {
         BMatrixNxN<4,4> d, i;
         
         for (uint y = 0; y < 4; y++)
            for (uint x = 0; x < 4; x++)
               d[x][y] = image(sx*4+x, sy*4+y).getGreyscale();

         i = BMatrixNxNDCT(d, dct);
                     
         float a0 = i[0][0] - 128.0f * 4;
         float r0 = sqrtSign(i[2][0]/512.0f);
         float g0 = sqrtSign(i[0][2]/512.0f);
         float b0 = i[2][2]/512.0f;
                           
         float a1 = sqrtSign(i[1][0]/512.0f);
         float g1 = sqrtSign(i[3][0]/512.0f);
         float r1 = sqrtSign(i[0][3]/512.0f);
         float b1 = 0.0f;//i[3][2]/512.0f;
         
         float a2 = sqrtSign(i[0][1]/512.0f);
         float r2 = sqrtSign(i[2][1]/512.0f);
         float g2 = sqrtSign(i[1][2]/512.0f);
         float b2 = 0.0f;//i[2][3]/512.0f;
         
         float a3 = sqrtSign(i[1][1]/512.0f);
         float g3 = sqrtSign(i[3][1]/512.0f);
         float r3 = sqrtSign(i[1][3]/512.0f);
         float b3 = 0.0f;//i[3][3]/512.0f;
         
         fdctImage[0](sx, sy).set(r0, g0, b0, a0);
         fdctImage[1](sx, sy).set(r1, g1, b1, a1);
         fdctImage[2](sx, sy).set(r2, g2, b2, a2);
         fdctImage[3](sx, sy).set(r3, g3, b3, a3);
      }
   }
   
   BRGBAImage dctImage[4];
   BVec4 dctImageRange[4];
   for (uint i = 0; i < 4; i++)
      dctImageRange[i] = createScaledImage(fdctImage[i], dctImage[i]);

   BRGBAImage temp(cellsX*2,cellsY*2);
   for (uint sy = 0; sy < cellsY; sy++)
   {
      for (uint sx = 0; sx < cellsX; sx++)
      {
         temp(sx*2+0,sy*2) = dctImage[0](sx,sy);
         temp(sx*2+1,sy*2) = dctImage[0](sx,sy);
         temp(sx*2+0,sy*2+1) = dctImage[0](sx,sy);
         temp(sx*2+1,sy*2+1) = dctImage[0](sx,sy);
      }
   }      
   dctImage[0] = temp;
      
   for (uint i = 0; i < 4; i++)
   {
      char filename[256];
      sprintf(filename, "origdct%i.tga", i);
      BImageUtils::writeTGA24(filename, dctImage[i]);
      sprintf(filename, "origdcta%i.tga", i);
      BImageUtils::writeTGA24(filename, dctImage[i], 3);
      
      BAlignedArray<uchar> dxtData;
      
      BDXTPacker packer;
      packer.pack(dctImage[i], cDXT5, false, false, false, dxtData);
                              
      BDXTUnpacker unpacker;   
      unpacker.unpack(dctImage[i], &dxtData[0], cDXT5, dctImage[i].getWidth(), dctImage[i].getHeight());
      
      sprintf(filename, "dct%i.tga", i);
      BImageUtils::writeTGA24(filename, dctImage[i]);
      sprintf(filename, "dcta%i.tga", i);
      BImageUtils::writeTGA24(filename, dctImage[i], 3);
   }
   
   BRGBAImage dctComp(width, height);
   
   for (uint sy = 0; sy < cellsY; sy++)
   {
      for (uint sx = 0; sx < cellsX; sx++)
      {
         const BRGBAColor& c0 = dctImage[0](sx*2,sy*2);
         const BRGBAColor& c1 = dctImage[1](sx,sy);
         const BRGBAColor& c2 = dctImage[2](sx,sy);
         const BRGBAColor& c3 = dctImage[3](sx,sy);
                     
         float fc0a = ((c0.a - 128) / 127.0f) * dctImageRange[0][3];
                              
         float a0 = fc0a + 128.0f * 4.0f;
         float r0 = ((c0.r - 123) / 127.0f) * dctImageRange[0][0];
         float g0 = ((c0.g - 125) / 127.0f) * dctImageRange[0][1];
         float b0 = ((c0.b - 123) / 127.0f) * dctImageRange[0][2];
         
         r0 = Math::Sign(r0)*r0*r0*512.0f;
         g0 = Math::Sign(g0)*g0*g0*512.0f;
         b0 *= 512.0f;
                  
         float a1 = (c1.a-128)/127.0f*dctImageRange[1][3];
         float g1 = (c1.g-125)/127.0f*dctImageRange[1][1];
         float r1 = (c1.r-123)/127.0f*dctImageRange[1][0];
         float b1 = (c1.b-123)/127.0f*dctImageRange[1][2];
         
         a1 = Math::Sign(a1)*a1*a1*512.0f;
         r1 = Math::Sign(r1)*r1*r1*512.0f;
         g1 = Math::Sign(g1)*g1*g1*512.0f;
         b1 *= 512.0f;

         float a2 = (c2.a-128)/127.0f*dctImageRange[2][3];
         float r2 = (c2.r-123)/127.0f*dctImageRange[2][0];
         float g2 = (c2.g-125)/127.0f*dctImageRange[2][1];
         float b2 = (c2.b-123)/127.0f*dctImageRange[2][2];
         
         a2 = Math::Sign(a2)*a2*a2*512.0f;
         r2 = Math::Sign(r2)*r2*r2*512.0f;
         g2 = Math::Sign(g2)*g2*g2*512.0f;
         b2 *= 512.0f;

         float a3 = (c3.a-128)/127.0f*dctImageRange[3][3];
         float g3 = (c3.g-125)/127.0f*dctImageRange[3][1];
         float r3 = (c3.r-123)/127.0f*dctImageRange[3][0];
         float b3 = (c3.b-123)/127.0f*dctImageRange[3][2];
         
         a3 = Math::Sign(a3)*a3*a3*512.0f;
         r3 = Math::Sign(r3)*r3*r3*512.0f;
         g3 = Math::Sign(g3)*g3*g3*512.0f;
         b3 *= 512.0f;
                  
         BMatrixNxN<4,4> d, i;
         
         d.setZero();
         d[0][0] = a0;
         d[2][0] = r0;
         d[0][2] = g0;
         d[2][2] = b0;
         
         d[1][0] = a1;
         d[3][0] = g1;
         d[0][3] = r1;
         d[3][2] = b1;
         
         d[0][1] = a2;
         d[2][1] = r2;
         d[1][2] = g2;
         d[2][3] = b2;
         
         d[1][1] = a3;
         d[3][1] = g3;
         d[1][3] = r3;
         d[3][3] = b3;
         
         i = BMatrixNxNIDCT(d, dct);
         
         for (uint y = 0; y < 4; y++)
            for (uint x = 0; x < 4; x++)
               dctComp(sx*4+x, sy*4+y).set(i[x][y]);
         
      }
   }
            
   BImageUtils::writeTGA24("dctcomp.tga", dctComp);      
   
#if 0   
   BRGBAImage dctBasis(512, 512);
   
   BMatrixNxN<4,4> idct;
   for (uint sy = 0; sy < 4; sy++)
   {
      for (uint sx = 0; sx < 4; sx++)
      {
         BMatrixNxN<4,4> m;
         m.setZero();
         m[sx][sy] = 1.0f;
                  
         idct =  BMatrixNxNIDCT(m, dct);
                  
         for (uint i = 0; i < 4; i++)
         {
            for (uint j = 0; j < 4; j++)
            {
               dctBasis(sx*24+j*2, sy*24+i*2).set(128 + idct[i][j] * 127.0f);
               dctBasis(sx*24+j*2+1, sy*24+i*2).set(128 + idct[i][j] * 127.0f);
               dctBasis(sx*24+j*2+1, sy*24+i*2+1).set(128 + idct[i][j] * 127.0f);
               dctBasis(sx*24+j*2, sy*24+i*2+1).set(128 + idct[i][j] * 127.0f);
            }
         }
      }
   }
      
   BImageUtils::writeTGA24("dctbasis.tga", dctBasis);      
#endif   

#if 1
      
   BRGBAColor32 yccLo(255,255,255,255);
   BRGBAColor32 yccHi(0,-255,-255,0);
   BRGBAColor32 yccRange;
      
   for (uint y = 0; y < image.getHeight(); y++)
   {
      for (uint x = 0; x < image.getWidth(); x++)
      {
         BRGBAColor& pixelColor = image(x, y);

         BRGBAColor16 yCoCg16;
         BColorUtils::RGBToYCoCgR(pixelColor, yCoCg16);
         BRGBAColor32 yCoCg(yCoCg16);
         
         for (uint i = 0; i < 3; i++)
         {
            if (yCoCg[i] < yccLo[i]) yccLo[i] = yCoCg[i];
            if (yCoCg[i] > yccHi[i]) yccHi[i] = yCoCg[i];
         }
      }
   }
   
   yccLo.r = 0;
   yccHi.r = 255;
   yccLo.a = 0;
   yccHi.a = 255;
   
   yccRange = yccHi - yccLo;
   BRGBAColor32 yccRangeHalf(yccRange / 2);
   if (yccRange[0] == 0) yccRange[0] = 1;
   if (yccRange[1] == 0) yccRange[1] = 1;
   if (yccRange[2] == 0) yccRange[2] = 1;
   
   g_Scale = BVec4(yccRange.r/255.0f, yccRange.g/255.0f, yccRange.b/255.0f, 0);
   g_Ofs = BVec4(yccLo.r/255.0f, yccLo.g/255.0f, yccLo.b/255.0f, 0);
      
   BRGBAImage chromaImage(image.getWidth(), image.getHeight());
   for (uint y = 0; y < image.getHeight(); y++)
   {
      for (uint x = 0; x < image.getWidth(); x++)
      {
         BRGBAColor& pixelColor = image(x, y);
         
         BRGBAColor16 yCoCg16;
         BColorUtils::RGBToYCoCgR(pixelColor, yCoCg16);
         BRGBAColor32 yCoCg(yCoCg16);

         yCoCg = yCoCg - yccLo;
         yCoCg = yCoCg * BRGBAColor32(255,255,255,0);
         yCoCg = (yCoCg + yccRangeHalf) / yccRange;
         yCoCg.clamp(0, 255);
                                    
         BRGBAColor& chromaPixelColor = chromaImage(x, y);
         chromaPixelColor.r = (uchar)yCoCg.g; 
         chromaPixelColor.g = (uchar)yCoCg.b;
         chromaPixelColor.b = 0;//pixelColor.a / 2;
         chromaPixelColor.a = 0;

         pixelColor.r = 0; 
         pixelColor.g = (uchar)yCoCg.r; 
         pixelColor.b = pixelColor.a / 3;
         pixelColor.a = 0;
      }
   }

#if 1  // ------------------------------------------------------------------------------------------
{
      BImageUtils::writeTGA24("before.tga", image, 1);
      
      BRGBAImage htransImage(image.getWidth()/2, image.getHeight()/2);
               
      typedef BStaticArray2D<BVec4, 2, 2> BBlockVec;
      BArray2D<BBlockVec> blockArray(cellsX, cellsY);
      
      uint hHist[3][256];
      Utils::ClearObj(hHist);

#if 0      
      BRGBAImage temp(8192, 8192);
#endif      
            
      for (uint cy = 0; cy < cellsY; cy++)
      {
         for (uint cx = 0; cx < cellsX; cx++)
         {
            BBlockVec h;
            
            float hSum[3];
            Utils::ClearObj(hSum);

#if 0                        
            for (uint q = 0; q < 64; q++)
            {
               temp.setPixel(cx*64+q,cy*64,BRGBAColor(64));
               temp.setPixel(cx*64,cy*64+q,BRGBAColor(64));
            }
#endif            
            
            for (uint sy = 0; sy < 2; sy++)
            {
               for (uint sx = 0; sx < 2; sx++)
               {
                  uint x = cx*4+sx*2;
                  uint y = cy*4+sy*2;
            
                  int a00 = image(x+0,y+0).g;   
                  int a10 = image(x+1,y+0).g;   
                  int a01 = image(x+0,y+1).g;   
                  int a11 = image(x+1,y+1).g;

                  int h0 = (a11 + a10 + a01 + a00) / 4;
                  int h1 = (a11 + a10 - a01 - a00); // / 4;
                  int h2 = (a11 - a10 + a01 - a00); // / 4;
                  int h3 = (a11 - a10 - a01 + a00); // / 4;
                  
                  float fh1 = h1 / 4.0f;
                  float fh2 = h2 / 4.0f;
                  float fh3 = h3 / 4.0f;
                  
                  //fh1 = -127.5f;
                  //fh2 = -127.5f;
                  //fh3 = -127.5f;
                  
                  fh1 /= 127.5f;
                  fh2 /= 127.5f;
                  fh3 /= 127.5f;
                  
                  //float mag = sqrt(fh1*fh1+fh2*fh2);
                  //float angle = Math::Clamp((atan2(fh1, fh2) + Math::fPi) / Math::fTwoPi, 0.0f, 1.0f);
                                                   
                  //temp(cx*2+sx, cy*2+sy).set(mag * 255.0f, angle * 255.0f, 0, 0);
#if 0                  
                  {
                     int xx = cx*64+Math::Clamp<int>(32.5f+fh1*32.0f, 0, 63);
                     int yy = cy*64+Math::Clamp<int>(32.5f+fh2*32.0f, 0, 63);
                     temp.setPixel(xx,yy,BRGBAColor(255));
                  }
#endif                  
                                                                       
                  fh1 = Math::Sign(fh1)*sqrt(fabs(fh1));
                  fh2 = Math::Sign(fh2)*sqrt(fabs(fh2));
                  //fh3 = Math::Sign(fh3)*sqrt(fabs(fh3));
                  
                  fh1 *= 127.0f;
                  fh2 *= 127.0f;
                  fh3 *= 127.0f;
                  
                  // -127 to 127
                  h(sx, sy)[0] = h0;
                  h(sx, sy)[1] = fh1;
                  h(sx, sy)[2] = fh2;
                  h(sx, sy)[3] = fh3;
                  
                  hSum[0] += fh1*fh1;
                  hSum[1] += fh2*fh2;
                  hSum[2] += fh3*fh3;
                                    
                  h1 = (int)((fh1 >= 0.0f) ? floor(fh1) : ceil(fh1));
                  h2 = (int)((fh2 >= 0.0f) ? floor(fh2) : ceil(fh2));
                  h3 = (int)((fh3 >= 0.0f) ? floor(fh3) : ceil(fh3));
                  
                  h1 = Math::Clamp(h1+128, 1, 255);
                  h2 = Math::Clamp(h2+128, 1, 255);
                  h3 = Math::Clamp(h3+128, 1, 255);
                  
                  hHist[0][h1]++;
                  hHist[1][h2]++;
                  hHist[2][h3]++;
               }
            }

#if 0            
            int minC;
            if ((hsum[0] < hsum[1]) && (hsum[0] < hsum[2]))                                            
            {
               minC = 1;
            }
            else if ((hsum[1] < hsum[0]) && (hsum[1] < hsum[2]))                                            
            {
               minC = 2;
            }
            else
            {
               minC = 3;
            }
            
            for (uint sy = 0; sy < 2; sy++)
               for (uint sx = 0; sx < 2; sx++)
                  h[sx][sy][minC] = 0;
#endif                  
            blockArray(cx, cy) = h;
         }
      }

#if 0      
      BImageUtils::writeTGA24("anglemag.tga", temp);
#endif      

      BVec3 range(127.0f);

#if 1      
      for (uint channel = 0; channel < 3; channel++)
      {
         uint total = 0;
         for (uint i = 1; i < 256; i++)
            total += hHist[channel][i];
            
         const uint thresh = 5;//Math::Max<uint>(1, (total * 5) / 1000);
         
         uint l;
         uint c = 0;
         for (l = 1; l <= 127; l++)
         {
            c += hHist[channel][l];
            if (c >= thresh)
               break;
         }
         
         uint h;
         c = 0;
         for (h = 255; h > l; h--)
         {
            c += hHist[channel][h];
            if (c >= thresh)
               break;
         }
         
         if ((l < h) && (l <= 127))
         {
            range[channel] = Math::Max(labs(l - 128), labs(h - 128));
         }
      }
#endif      

      range[0] = Math::Clamp(range[0], 1.0f, 127.0f);
      range[1] = Math::Clamp(range[1], 1.0f, 127.0f);
      range[2] = Math::Clamp(range[2], 1.0f, 127.0f);
      range[0] = range[1] = range[2] = Math::Max3(range[0], range[1], range[2]);
      //range[2] = Math::Clamp(range[2]*2.0f, 1.0f, 127.0f);
      
      for (uint cy = 0; cy < cellsY; cy++)
      {
         for (uint cx = 0; cx < cellsX; cx++)
         {
            const BBlockVec& block = blockArray(cx, cy);
            
            for (uint sy = 0; sy < 2; sy++)
            {
               for (uint sx = 0; sx < 2; sx++)
               {  
                  float fh0 = block(sx, sy)[0];
                  float fh1 = block(sx, sy)[1];
                  float fh2 = block(sx, sy)[2];
                  float fh3 = block(sx, sy)[3];
                  
                  fh1 = fh1 / range[0];
                  fh2 = fh2 / range[1];
                  fh3 = fh3 / range[2];
                                    
                  fh1 = Math::Clamp(fh1, -1.0f, 1.0f) * 127.0f;
                  fh2 = Math::Clamp(fh2, -1.0f, 1.0f) * 127.0f;
                  fh3 = Math::Clamp(fh3, -1.0f, 1.0f) * 127.0f;
                                                                                          
                  int h0 = (int)fh0;
                  int h1 = (int)(Math::Sign(fh1)*floor(.5f+fabs(fh1)));
                  int h2 = (int)(Math::Sign(fh2)*floor(.5f+fabs(fh2)));
                  int h3 = (int)(Math::Sign(fh3)*floor(.5f+fabs(fh3)));
                  
                  h1 += 125;
                  h2 += 123;
                  h3 += 123;

                  h1 = Math::Clamp(h1, 0, 255);
                  h2 = Math::Clamp(h2, 0, 255);
                  h3 = Math::Clamp(h3, 0, 255);

                  htransImage(cx*2+sx,cy*2+sy).a = h0;
                  htransImage(cx*2+sx,cy*2+sy).r = h2;
                  htransImage(cx*2+sx,cy*2+sy).g = h1;
                  htransImage(cx*2+sx,cy*2+sy).b = h3;
               }
            }                        
         } // cx
      } // cy            
      
      //a11 = (h0 + h1 + h2 + h3);
      //a10 = (h0 + h1 - h2 - h3);
      //a01 = (h0 - h1 + h2 - h3);
      //a00 = (h0 - h1 - h2 + h3);

      //image(x+0,y+0).set(a00);   //ave
      //image(x+1,y+0).set(a10);   //bottom-top
      //image(x+0,y+1).set(a01);   //left-right
      //image(x+1,y+1).set(a11);   //diagnol
      
      BDXTPacker packer;
      BAlignedArray<uchar> htransDXT5;
      packer.pack(htransImage, cDXT5, false, false, false, htransDXT5);
      
      BRGBAImage unpackedHTrans;
      BDXTUnpacker unpacker;
      unpacker.unpack(unpackedHTrans, &htransDXT5[0], cDXT5, htransImage.getWidth(), htransImage.getHeight());
      
      BImageUtils::writeTGA24("hunpacked.tga", unpackedHTrans);
      BImageUtils::writeTGA24("hunpackedA.tga", unpackedHTrans, 3);
      
      BRGBAImage newImage(image.getWidth(), image.getHeight());
      
      for (uint y = 0; y < newImage.getHeight(); y+=2)
      {
         for (uint x = 0; x < newImage.getWidth(); x+=2)
         {
            int h0 = unpackedHTrans(x/2,y/2).a;
            int h2 = unpackedHTrans(x/2,y/2).r-123;
            int h1 = unpackedHTrans(x/2,y/2).g-125;
            int h3 = unpackedHTrans(x/2,y/2).b-123;
                                                
            h1 = Math::Clamp(h1, -127, 127);
            h2 = Math::Clamp(h2, -127, 127);
            h3 = Math::Clamp(h3, -127, 127);
                                   
            float fh1 = (h1 / 127.0f); fh1 = fh1*fh1*Math::Sign(h1) * range[0];
            float fh2 = (h2 / 127.0f); fh2 = fh2*fh2*Math::Sign(h2) * range[1];
            //float fh3 = (h3 / 127.0f); fh3 = fh3*fh3*Math::Sign(h3) * range[2];
            
            //float fh1 = (h1 / 127.0f); fh1 = fh1 * range[0];
            //float fh2 = (h2 / 127.0f); fh2 = fh2 * range[1];
            float fh3 = (h3 / 127.0f); fh3 = fh3 * range[2];
                                    
            int a11 = Math::Clamp((int)(.5f + h0 + fh1 + fh2 + fh3), 0, 255);
            int a10 = Math::Clamp((int)(.5f + h0 + fh1 - fh2 - fh3), 0, 255);
            int a01 = Math::Clamp((int)(.5f + h0 - fh1 + fh2 - fh3), 0, 255);
            int a00 = Math::Clamp((int)(.5f + h0 - fh1 - fh2 + fh3), 0, 255);
                                                                                    
            //int a11 = Math::Clamp((h0 * 4 + h1 + h2 + h3)/4,0,255);
            //int a10 = Math::Clamp((h0 * 4 + h1 - h2 - h3)/4,0,255);
            //int a01 = Math::Clamp((h0 * 4 - h1 + h2 - h3)/4,0,255);
            //int a00 = Math::Clamp((h0 * 4 - h1 - h2 + h3)/4,0,255);

            newImage(x+0,y+0).set(a00);   
            newImage(x+1,y+0).set(a10);   
            newImage(x+0,y+1).set(a01);   
            newImage(x+1,y+1).set(a11);
            
            image(x+0,y+0).g = a00;   
            image(x+1,y+0).g = a10;   
            image(x+0,y+1).g = a01;   
            image(x+1,y+1).g = a11;
         }
      }
                        
      BImageUtils::writeTGA24("after.tga", newImage);
}  
#endif  // ------------------------------------------------------------------------------------------
               
   BDXTPacker dxtPacker;
      
   dxtImage.clear();
   //BImageUtils::ditherImage(image, image, 1);
   dxtPacker.pack(image, dxtFormat, false, false, false, dxtImage);
   
   BRGBAImage subsampledChromaImage;
   BImageResampler resampler;
   resampler.resample(chromaImage, subsampledChromaImage, image.getWidth()/4, image.getHeight()/4, 4, false, false, "kaiser", 1.0f);
   
   BAlignedArray<uchar> subsampledChromaDXTImage;
   dxtPacker.pack(subsampledChromaImage, dxtFormat, false, false, false, subsampledChromaDXTImage);   
   
   BRGBAImage unpackedImage;
   BDXTUnpacker::unpack(unpackedImage, &dxtImage[0], dxtFormat, image.getWidth(), image.getHeight());
   
   BRGBAImage unpackedChromaImage;
   BDXTUnpacker::unpack(unpackedChromaImage, &subsampledChromaDXTImage[0], dxtFormat, subsampledChromaImage.getWidth(), subsampledChromaImage.getHeight());
   
   BImageUtils::writeTGA24("unpackedLuma.tga", unpackedImage);    
   BImageUtils::writeTGA24("unpackedChroma.tga", unpackedChromaImage);    
   
   BRGBAImage upsampledUnpackedChromaImage;
   resampler.resample(unpackedChromaImage, upsampledUnpackedChromaImage, image.getWidth(), image.getHeight(), 4, false, false, "tent", 1.0f);

   BRGBAImage finalImage(image.getWidth(), image.getHeight());      
   for (uint y = 0; y < finalImage.getHeight(); y++)
   {
      for (uint x = 0; x < finalImage.getWidth(); x++)
      {
         BRGBAColor& c = upsampledUnpackedChromaImage(x, y);
         
         BRGBAColor& l = unpackedImage(x, y);
         BRGBAColor32 yc(l.g, c.r, c.g, 0);
         
         // wavelet
         //BRGBAColor32 yc(image(x,y).g, c.r, c.g, 0);
                           
         yc = ((yc * yccRange + BRGBAColor32(128,128,128,0)) / BRGBAColor32(255,255,255,255)) + yccLo;
         
         BColorUtils::YCoCgRToRGB(BRGBAColor16(yc), finalImage(x, y));
         
         //finalImage(x, y).a = (uchar)Math::Clamp(c.b * 2, 0, 255);
         finalImage(x, y).a = (uchar)Math::Clamp(l.b * 3, 0, 255);
      }
   }
   
   BImageUtils::writeTGA24("final.tga", finalImage);    
   BImageUtils::writeTGA24("finalA.tga", finalImage, 3);    
   
   

      
#else
   //BImageUtils::ditherImage(image, image);

   int lowQuality = 1;
   int highQuality = 99;
   
   int closestQuality;
   float closestBitRate = 64.0f;
   int closestCompressedDataSize;
   
   while (lowQuality <= highQuality)
   {
      if (bitRate != 0.0f)
         quality = (lowQuality + highQuality) / 2;
      
      compressedData.resize(0);   
      
      LARGE_INTEGER compStartTime;
      LARGE_INTEGER compEndTime;
      QueryPerformanceCounter(&compStartTime);
                  
      bool success;
      if (method == 2)
         success = BDXTMunger::compressMethod2(image, hasAlpha, quality, codebookSize, virtualCodebook, subsampling, useAveDelta, useVisualQuant, compressedData);
      else
         success = BDXTMunger::compressMethod1(image, hasAlpha, quality, compressedData);
      if (!success)
         return false;
         
      QueryPerformanceCounter(&compEndTime);
      
      LARGE_INTEGER decompStartTime;
      LARGE_INTEGER decompEndTime;
      QueryPerformanceCounter(&decompStartTime);
                     
      BAlignedArray<uchar> trialDXTImage(cellsX * cellsY * bytesPerBlock);
      success = BDXTMunger::decompress((uchar*)&trialDXTImage[0], dxtFormat, width, height, &compressedData[0], compressedData.size());
      if (!success)
         return false;
         
      QueryPerformanceCounter(&decompEndTime);
      
      LARGE_INTEGER freq;
      QueryPerformanceFrequency(&freq);
      
      DWORD compTime = static_cast<DWORD>(((*(unsigned __int64*)&compEndTime - *(unsigned __int64*)&compStartTime) * 1000) / *(unsigned __int64*)&freq);
      DWORD decompTime = static_cast<DWORD>(((*(unsigned __int64*)&decompEndTime - *(unsigned __int64*)&decompStartTime) * 1000) / *(unsigned __int64*)&freq);
      
      const float compressedBitRate = (compressedData.size() * 8.0f) / (width * height);
                  
      char buf[512];
      sprintf(buf, "SrcFile: %s, DstFile: %s, Method: %i, CompTime: %ums, DecompTime: %ums, Quality: %i, DXT Size: %i, Munged Size: %i, Bits per texel: %f\n", 
         pSrcFilename, pDstFilename ? pDstFilename : "", method, compTime, decompTime, quality, dxtImage.size(), compressedData.size(), compressedBitRate);
      OutputDebugString(buf);
               
      if ((bitRate == 0.0f) || (fabs(compressedBitRate - bitRate) < fabs(closestBitRate - bitRate)))
      {
         closestBitRate = compressedBitRate;
         closestQuality = quality;
         closestCompressedDataSize = compressedData.size();
         dxtImage = trialDXTImage;
         if (bitRate == 0.0f)
            break;
      }
   
      if (compressedBitRate < bitRate)
         lowQuality = quality + 1;
      else
         highQuality = quality - 1;
   }  
   
   FILE* pFile = fopen("complog.txt", "a+");
   if (pFile)
   {
      char buf[512];
      sprintf(buf, "SrcFile: %s, DstFile: %s, Quality: %i, Subsample: %i, Method: %i, AveDelta: %i, VisualQuant: %i, DXT Size: %i, Munged Size: %i, Bits per texel: %f\n", 
         pSrcFilename, pDstFilename ? pDstFilename : "", closestQuality, subsampling, method, useAveDelta, useVisualQuant, dxtImage.size(), closestCompressedDataSize, closestBitRate);
      fprintf(pFile, "%s", buf);
      fclose(pFile);      
   }
   
   BRGBAImage unpackedImage;
   BDXTUnpacker::unpack(unpackedImage, &dxtImage[0], dxtFormat, width, height);
#endif

   if (g_pTexture0)
   {
      g_pTexture0->Release();
      g_pTexture0 = NULL;
   }
   if (g_pTexture1)
   {
      g_pTexture1->Release();
      g_pTexture1 = NULL;
   }
   
   if (FAILED(g_pd3dDevice->CreateTexture(width,height,1,0, d3dFormat,D3DPOOL_MANAGED,&g_pTexture0,0)))
      return false;
      
   if (FAILED(g_pd3dDevice->CreateTexture(subsampledChromaImage.getWidth(),subsampledChromaImage.getHeight(),1,0, d3dFormat,D3DPOOL_MANAGED,&g_pTexture1,0)))
      return false;

   D3DLOCKED_RECT tLockedRect;
   if(!FAILED(g_pTexture0->LockRect(0, &tLockedRect,0,0)))
   {
      memcpy(tLockedRect.pBits,(BYTE *)&dxtImage[0],cellsX * cellsY * bytesPerBlock);
      g_pTexture0->UnlockRect(0);
   }
   
   if(!FAILED(g_pTexture1->LockRect(0, &tLockedRect,0,0)))
   {
      memcpy(tLockedRect.pBits,(BYTE *)&subsampledChromaDXTImage[0], (subsampledChromaImage.getWidth()/4) * (subsampledChromaImage.getHeight()/4) * bytesPerBlock);
      g_pTexture1->UnlockRect(0);
   }
   
   if (pDstFilename)
   {
      IDirect3DSurface9* pSurface;
      if (FAILED(g_pTexture0->GetSurfaceLevel(0,  &pSurface)))
         return false;
      
      hres = D3DXSaveSurfaceToFile(pDstFilename,
         D3DXIFF_BMP,
         pSurface,
         NULL,
         NULL);
      
      pSurface->Release();
      
      if (FAILED(hres))
         return false;
   }         
   
   return true;
}

void initTexture(void)
{
   if (__argc >= 2)
   {
      const char* pSrcFilename = NULL;
      const char* pDstFilename = NULL;
      int quality = 90;
      bool exitAfterLoading = false;
      float bitRate = 0.0f;
      int method = 2;
      bool subsampling = false;
      bool useAveDelta = true;
      bool useVisualQuant = false;
      int codebookSize = 256;
      bool virtualCodebook = true;

      for (int i = 1; i < __argc; i++)
      {
         const char* pOption = __argv[i];
         if (pOption[0] == '-')
         {
            if (pOption[1] == 'q')
            {
               quality = atoi(&pOption[2]);
               if (quality < 1) quality = 1; else if (quality > 99) quality = 99;
            }
            else if (pOption[1] == 'x')
               exitAfterLoading = true;
            else if (pOption[1] == 'b')
            {
               bitRate = static_cast<float>(atof(&pOption[2]));
               if (bitRate < 0.0f)
                  bitRate = 0.0f;
            }
            else if (pOption[1] == 'm')
            {
               method = atoi(&pOption[2]);
               if (method < 1) method = 1; else if (method > 2) method = 2;
            }
            else if (pOption[1] == 's')
               subsampling = true;
            else if (pOption[1] == 'a')
               useAveDelta = false;
            else if (pOption[1] == 'v')
               useVisualQuant = true;
            else if (pOption[1] == 'r')
               virtualCodebook = false;
            else if (pOption[1] == 'c')
            {
               codebookSize = atoi(&pOption[2]);
               if (codebookSize < 0)
                  codebookSize = 0;
               else if (codebookSize > 16384)
                  codebookSize = 16384;
            }
         }
         else if (!pSrcFilename)
            pSrcFilename = pOption;
         else if (!pDstFilename)
            pDstFilename = pOption;
      }

      if (pSrcFilename) 
      {
         loadTexture2(pSrcFilename, pDstFilename, quality, codebookSize, virtualCodebook, bitRate, method, subsampling, useAveDelta, useVisualQuant);
         if (exitAfterLoading)
            PostQuitMessage(0);
      }
   }
}

//-----------------------------------------------------------------------------
// Name: init()
// Desc: 
//-----------------------------------------------------------------------------
void init( void)
{
    g_pD3D = Direct3DCreate9( D3D_SDK_VERSION );

    D3DDISPLAYMODE d3ddm;

    g_pD3D->GetAdapterDisplayMode( D3DADAPTER_DEFAULT, &d3ddm );

    D3DPRESENT_PARAMETERS d3dpp;
    ZeroMemory( &d3dpp, sizeof(d3dpp) );
	
    d3dpp.Windowed               = TRUE;
    d3dpp.SwapEffect             = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat       = d3ddm.Format;
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
    d3dpp.PresentationInterval   = D3DPRESENT_INTERVAL_IMMEDIATE;

    g_pD3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, g_hWnd,
                          D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                          &d3dpp, &g_pd3dDevice );
		
	g_pd3dDevice->CreateVertexBuffer( 4*sizeof(Vertex), D3DUSAGE_WRITEONLY, 
                                      D3DFVF_CUSTOMVERTEX, D3DPOOL_DEFAULT, 
                                     &g_pVertexBuffer, NULL );
    
    D3DXMATRIX matProj;
    //D3DXMatrixPerspectiveFovLH( &matProj, D3DXToRadian( 45.0f ), 
    //                            512.0f / 512.0f, 0.1f, 100.0f );
    D3DXMatrixIdentity(&matProj);
    g_pd3dDevice->SetTransform( D3DTS_PROJECTION, &matProj );

	g_pd3dDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
      
   const char* pSrcFilename = "pixelshader.hlsl";
   LPD3DXBUFFER pShader = NULL;
   LPD3DXBUFFER pErrors = NULL;
   DWORD flags = 0;
   HRESULT hres = D3DXCompileShaderFromFile(pSrcFilename, NULL, NULL, "main", "ps_2_0", flags, &pShader, &pErrors, NULL);
   if (FAILED(hres))
   {
      OutputDebugString((char*)pErrors->GetBufferPointer());
      OutputDebugString("\n");
      DebugBreak();
   }
   else
   {  
      hres = g_pd3dDevice->CreatePixelShader((const DWORD*)pShader->GetBufferPointer(), &g_pPixelShader);
   }
   
   if (pShader)
      pShader->Release();
   
   if (pErrors)
      pErrors->Release();

   initTexture();   
   
}

//-----------------------------------------------------------------------------
// Name: shutDown()
// Desc: 
//-----------------------------------------------------------------------------
void shutDown( void )
{
   if (g_pPixelShader)
      g_pPixelShader->Release();
      
    if( g_pTexture0 != NULL ) 
        g_pTexture0->Release();
        
    if( g_pTexture1 != NULL ) 
       g_pTexture1->Release();

    if( g_pVertexBuffer != NULL ) 
        g_pVertexBuffer->Release(); 

    if( g_pd3dDevice != NULL )
        g_pd3dDevice->Release();

    if( g_pD3D != NULL )
        g_pD3D->Release();
}

//-----------------------------------------------------------------------------
// Name: render()
// Desc: 
//-----------------------------------------------------------------------------
void render( void )
{
    g_pd3dDevice->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
                         D3DCOLOR_COLORVALUE(0.0f,0.0f,0.0f,1.0f), 1.0f, 0 );

    D3DXMATRIX matWorld;
    D3DXMatrixTranslation( &matWorld, 0.0f, 0.0f, 0.0f );
    g_pd3dDevice->SetTransform( D3DTS_WORLD, &matWorld );

    g_pd3dDevice->BeginScene();

    g_pd3dDevice->SetTexture( 0, g_pTexture0 );
    g_pd3dDevice->SetTexture( 1, g_pTexture1 );
    g_pd3dDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    g_pd3dDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    g_pd3dDevice->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
    g_pd3dDevice->SetSamplerState(1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    g_pd3dDevice->SetSamplerState(1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    g_pd3dDevice->SetSamplerState(1, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
    
    if (g_pPixelShader)
    {
      g_pd3dDevice->SetPixelShader(g_pPixelShader);
    
      g_pd3dDevice->SetPixelShaderConstantF(0, &g_Scale[0], 1);
      g_pd3dDevice->SetPixelShaderConstantF(1, &g_Ofs[0], 1);
    }
        
    //g_pd3dDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_ALPHAREPLICATE|D3DTA_TEXTURE);

    void *pVertices = NULL;

    static float pixelOfs = -.5f;

    Vertex vertices[4];
    for (int i = 0; i < 4; i++)
    {
      vertices[i] = g_quadVertices[i];
      vertices[i].x = (g_quadVertices[i].x * gWindowWidth) + pixelOfs;
      vertices[i].y = (g_quadVertices[i].y * gWindowHeight) + pixelOfs;
    }

    g_pVertexBuffer->Lock( 0, sizeof(vertices), (void**)&pVertices, 0 );
    memcpy( pVertices, vertices, sizeof(vertices) );
    g_pVertexBuffer->Unlock();
        
    g_pd3dDevice->SetStreamSource( 0, g_pVertexBuffer, 0, sizeof(Vertex) );
	 g_pd3dDevice->SetFVF( D3DFVF_CUSTOMVERTEX );
	 
	 g_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, 0, 2 );

    g_pd3dDevice->EndScene();
    g_pd3dDevice->Present( NULL, NULL, NULL, NULL );
    
    Sleep(60);
}

