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
   BRGBAImage lumaImage(image.getWidth()/2, image.getHeight());
   
   for (uint y = 0; y < image.getHeight(); y++)
   {
      for (uint x = 0; x < image.getWidth(); x++)
      {
         const BRGBAColor& pixelColor = image(x, y);
         
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

         //pixelColor.r = 0; 
         //pixelColor.g = (uchar)yCoCg.r; 
         //pixelColor.b = pixelColor.a / 3;
         //pixelColor.a = 0;
         
         if (x >= image.getWidth()/2)
         {
            BRGBAColor& l = lumaImage(x - image.getWidth()/2, y);
            int firstG = l.g;
            int secondG = yCoCg.r;
            l.g = (firstG + secondG) / 2;
            l.r = Math::Clamp<int>(123 + (firstG - secondG) / 2, 0, 255);
         }
         else
         {
            lumaImage(x, y).g = (uchar)yCoCg.r;
         }
      }
   }
                     
   BDXTPacker dxtPacker;
      
   dxtImage.clear();
   dxtPacker.pack(lumaImage, dxtFormat, false, false, false, dxtImage);
   
   BRGBAImage unpackedLumaImage;
   BDXTUnpacker::unpack(unpackedLumaImage, &dxtImage[0], dxtFormat, lumaImage.getWidth(), lumaImage.getHeight());
   
   BRGBAImage subsampledChromaImage;
   BImageResampler resampler;
   resampler.resample(chromaImage, subsampledChromaImage, image.getWidth()/4, image.getHeight()/4, 4, false, false, "kaiser", 1.0f);
   
   BAlignedArray<uchar> subsampledChromaDXTImage;
   dxtPacker.pack(subsampledChromaImage, dxtFormat, false, false, false, subsampledChromaDXTImage);   
         
   BRGBAImage unpackedChromaImage;
   BDXTUnpacker::unpack(unpackedChromaImage, &subsampledChromaDXTImage[0], dxtFormat, subsampledChromaImage.getWidth(), subsampledChromaImage.getHeight());
   
   BImageUtils::writeTGA24("unpackedLuma.tga", unpackedLumaImage);    
   BImageUtils::writeTGA24("unpackedChroma.tga", unpackedChromaImage);    
   
   BRGBAImage upsampledUnpackedChromaImage;
   resampler.resample(unpackedChromaImage, upsampledUnpackedChromaImage, image.getWidth(), image.getHeight(), 4, false, false, "tent", 1.0f);

   BImageUtils::writeTGA24("lumaimage.tga", lumaImage);  
   
   BRGBAImage finalImage(image.getWidth(), image.getHeight());      
   for (uint y = 0; y < finalImage.getHeight(); y++)
   {
      for (uint x = 0; x < finalImage.getWidth(); x++)
      {
         BRGBAColor& c = upsampledUnpackedChromaImage(x, y);
                  
         int a;

         BRGBAColor l;// = unpackedLumaImage(x % lumaImage.getWidth(), y);           
         
         if (x >= lumaImage.getWidth())
            l = unpackedLumaImage(x - image.getWidth()/2, y);
         else
            l = unpackedLumaImage(x, y);

         int ya = l.g * 2;
         int yd = (l.r - 123) * 2;
         if (x >= lumaImage.getWidth())
            a = (ya - yd)/2;
         else
            a = (ya + yd)/2;

         a = Math::Clamp(a, 0, 255);
                  
         BRGBAColor32 yc(a, c.r, c.g, 0);
         
         // wavelet
         //BRGBAColor32 yc(image(x,y).g, c.r, c.g, 0);
                           
         yc = ((yc * yccRange + BRGBAColor32(128,128,128,0)) / BRGBAColor32(255,255,255,255)) + yccLo;
                  
         BColorUtils::YCoCgRToRGB(BRGBAColor16(yc), finalImage(x, y));
         
         //finalImage(x, y).a = (uchar)Math::Clamp(c.b * 2, 0, 255);
         //finalImage(x, y).a = (uchar)Math::Clamp(l.b * 3, 0, 255);
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

