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

//-----------------------------------------------------------------------------
// GLOBALS
//-----------------------------------------------------------------------------
HWND                    g_hWnd          = NULL;
LPDIRECT3D9             g_pD3D          = NULL;
LPDIRECT3DDEVICE9       g_pd3dDevice    = NULL;
LPDIRECT3DVERTEXBUFFER9 g_pVertexBuffer = NULL;
LPDIRECT3DTEXTURE9      g_pTexture      = NULL;

#define D3DFVF_CUSTOMVERTEX ( D3DFVF_XYZRHW | D3DFVF_TEX1 )

struct Vertex
{
    float x, y, z, w;
    float tu, tv;
};

int gWindowWidth = 512;
int gWindowHeight = 512;

Vertex g_quadVertices[] =
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

void unpackColor(WORD packed, int& r, int& g, int& b, bool scaled)
{
   if (scaled)
   {
      r = (((packed >> 11) & 31) * 255 + 15) / 31;
      g = (((packed >> 5) & 63) * 255 + 31) / 63;
      b = ((packed & 31) * 255 + 15) / 31;
   }
   else
   {
      r = (packed >> 11) & 31;
      g = (packed >> 5) & 63;
      b = packed & 31;
   }
}

WORD packColor(int r, int g, int b, bool scaled)
{
   if (scaled)
   {
      if (r < 0) r = 0; else if (r > 255) r = 255;
      if (g < 0) g = 0; else if (g > 255) g = 255;
      if (b < 0) b = 0; else if (b > 255) b = 255;
      
      r = (r * 31 + 127) / 255;
      g = (g * 63 + 127) / 255;
      b = (b * 31 + 127) / 255;
   }
   
   if (r < 0) r = 0; else if (r > 31) r = 31;
   if (g < 0) g = 0; else if (g > 63) g = 63;
   if (b < 0) b = 0; else if (b > 31) b = 31;
   
   return (r << 11) | (g << 5) | b;
}

//-----------------------------------------------------------------------------
// Name: writeTGA16()
// Desc:
//-----------------------------------------------------------------------------
void writeTGA16(const char* pFilename, const WORD* pColors, int numXCells, int numYCells)
{
   tga_writer tgaWriter;
   if (tgaWriter.open(pFilename, numXCells, numYCells, TGA_IMAGE_TYPE_BGR))
      return;
   BAlignedArray<unsigned char> buf(numXCells * 3);
   
   for (int y = 0; y < numYCells; y++)
   {
      for (int x = 0; x < numXCells; x++)
      {
         int r, g, b;
         unpackColor(pColors[x + y * numXCells], r, g, b, true);
         buf[x * 3 + 2] = r;
         buf[x * 3 + 1] = g;
         buf[x * 3 + 0] = b;
      }
      if (tgaWriter.write_line(&buf[0]))
         return;
   }
   tgaWriter.close();
}



class BCodebookEntry
{
   enum { cNumComps = 6 };
   float v[cNumComps];

public:
   BCodebookEntry() 
   { 
      setZero();
   }

   BCodebookEntry(const BCodebookEntry& b)
   {
      for (int i = 0; i < cNumComps; i++)
         v[i] = b.v[i];
   }

   BCodebookEntry& operator=(const BCodebookEntry& b)
   {
      for (int i = 0; i < cNumComps; i++)
         v[i] = b.v[i];
      return *this;
   }

   BCodebookEntry(float r0, float g0, float b0, float r1, float g1, float b1)
   {
      v[0] = r0;
      v[1] = g0;
      v[2] = b0;
      v[3] = r1;
      v[4] = g1;
      v[5] = b1;
   }
      
   uint numComps(void) const { return cNumComps; }
   float operator[] (uint i) const { assert(i < cNumComps); return v[i]; }
   const float& operator[] (uint i) { assert(i < cNumComps); return v[i]; }

   BCodebookEntry& setZero(void)
   {
      for (int i = 0; i < cNumComps; i++)
         v[i] = 0.0f;
      return *this;
   }

   BCodebookEntry& setDefault(void)
   {
      setZero();
      return *this;
   }

   void normalize(void)
   {
      for (int i = 0; i < cNumComps; i++)
      {
         if (v[i] < 0.0f)
            v[i] = 0.0f;
         else if (v[i] > 1.0f)
            v[i] = 1.0f;
      }
   }
   
   float dot(const BCodebookEntry& b) const
   {
      float total = 0.0f;
      for (int i = 0; i < cNumComps; i++)
         total += b.v[i] * v[i];
      return total;
   }

   float dist2(const BCodebookEntry& b) const
   {
      float total = 0.0f;
      for (int i = 0; i < cNumComps; i++)
         total += (b.v[i] - v[i]) * (b.v[i] - v[i]);
      return total;
   }

   BCodebookEntry uniformPerturb(float mul) const
   {
      BCodebookEntry ret;
      for (int i = 0; i < cNumComps; i++)
         ret.v[i] = v[i] + mul * .00125f/255.0f;
      return ret;
   }

   BCodebookEntry randomPerturb(void) const
   {
      BCodebookEntry ret;
      for (int i = 0; i < cNumComps; i++)
         ret.v[i] = v[i] + frand(-.00125f/255.0f, .00125f/255.0f);
      return ret;
   }

   friend BCodebookEntry operator* (const BCodebookEntry& b, float w) 
   {
      BCodebookEntry ret;
      for (int i = 0; i < cNumComps; i++)
         ret.v[i] = b.v[i] * w;
      return ret;
   }

   BCodebookEntry& operator*= (float w) 
   {
      for (int i = 0; i < cNumComps; i++)
         v[i] *= w;
      return *this;
   }

   friend BCodebookEntry operator+ (const BCodebookEntry& a, const BCodebookEntry& b)
   {
      BCodebookEntry ret;
      for (int i = 0; i < cNumComps; i++)
         ret.v[i] = a.v[i] + b.v[i];
      return ret;
   }

   friend BCodebookEntry operator- (const BCodebookEntry& a, const BCodebookEntry& b)
   {
      BCodebookEntry ret;
      for (int i = 0; i < cNumComps; i++)
         ret.v[i] = a.v[i] - b.v[i];
      return ret;
   }

   BCodebookEntry& operator+= (const BCodebookEntry& a)
   {
      for (int i = 0; i < cNumComps; i++)
         v[i] += a.v[i];
      return *this;
   }

   bool operator== (const BCodebookEntry& a) const
   {
      for (int i = 0; i < cNumComps; i++)
         if (v[i] != a.v[i])
            return false;
      return true;
   }

   bool operator< (const BCodebookEntry& a) const
   {
      for (int i = 0; i < cNumComps; i++)
      {
         if (v[i] < a.v[i])
            return true;
         else if (v[i] > a.v[i])
            return false;
      }
      return false;
   }
private:
   static float frand(float l, float h)
   {
      return (((float)(l)) + ((float)((h)-(l)))*rand()/((float)RAND_MAX));
   }
};

void RGBToYCoCg(int r, int g, int b, int& y, int& co, int& cg)
{
   y  =  (r >> 2) + (g >> 1) + (b >> 2);
   co =  (r >> 1)            - (b >> 1);
   cg = -(r >> 2) + (g >> 1) - (b >> 2);
}

void YCoCgToRGB(int& y, int& co, int cg, int& r, int& g, int& b)
{
   g = y + cg;
   int t = y - cg;
   r = t + co;
   b = t - co;
}

void RGBToYCoCgR(int r, int g, int b, int& y, int& co, int& cg)
{
   co = r - b;
   int t = b + (co >> 1);
   cg = g - t;
   y = t + (cg >> 1);
}

void YCoCgRToRGB(int& y, int& co, int cg, int& r, int& g, int& b)
{
   int t = y - (cg >> 1);
   g = cg + t;
   b = t - (co >> 1);
   r = b + co;
}

int clamp(int i, int l = 0, int h = 255)
{
   if (i < l) 
      i = l; 
   else if (i > h) 
      i = h;
      
   return i;
}

int colorDistance(int e1r, int e1g, int e1b, int e2r, int e2g, int e2b)
{
   int r, g, b;
   int rmean;

   rmean = ( (int)e1r + (int)e2r ) / 2;
   r = (int)e1r - (int)e2r;
   g = (int)e1g - (int)e2g;
   b = (int)e1b - (int)e2b;
   return (((512+rmean)*r*r)>>8) + 4*g*g + (((767-rmean)*b*b)>>8);
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

#if 0
{
   BVec3 mean(0.0f);
   
   for (uint sy = 0; sy < image.getHeight(); sy++)
      for (uint sx = 0; sx < image.getWidth(); sx++)
         mean += BVec3(image(sx, sy).r/255.0f, image(sx, sy).g/255.0f, image(sx, sy).b/255.0f);

   mean /= float(image.getWidth()*image.getHeight());

   BVec3 a(0.0f);
   BVec3 b(0.0f);

   for (uint sy = 0; sy < image.getHeight(); sy++)
   {
      for (uint sx = 0; sx < image.getWidth(); sx++)
      {
         const BRGBAColor& pixelColor = image(sx, sy);

         BVec3 v(pixelColor.r/255.0f, pixelColor.g/255.0f, pixelColor.b/255.0f);
         v -= mean;

         a += BVec3::multiply(v, v);
         b += BVec3(v[0] * v[1], v[0] * v[2], v[1] * v[2]);
      }
   }

   a /= float(image.getWidth()*image.getHeight());
   b /= float(image.getWidth()*image.getHeight());

   Lengyel::Matrix3D covariance;   

   covariance(0,0) = a[0];
   covariance(1,1) = a[1];
   covariance(2,2) = a[2];
   covariance(0,1) = b[0]; covariance(1,0) = b[0];
   covariance(0,2) = b[1]; covariance(2,0) = b[1];
   covariance(1,2) = b[2]; covariance(2,1) = b[2];

   float lambda[3];
   Lengyel::Matrix3D rm;            
   Lengyel::CalculateEigensystem(covariance, lambda, rm);

   lambda[0] = fabs(lambda[0]);
   lambda[1] = fabs(lambda[1]);
   lambda[2] = fabs(lambda[2]);

   BVec3 axis[3];
   for (uint i = 0; i < 3; i++)
   {
      axis[i] = BVec3(rm(0, i), rm(1, i), rm(2, i));
      axis[i].normalize();
   }
   
   for (uint i = 0; i < 2; i++)
   {
      for (uint j = 0; j < 3 - i - 1; j++)
      {
         if (lambda[j]<lambda[j+1])
         {
            std::swap(lambda[j], lambda[j+1]);
            std::swap(axis[j], axis[j+1]);
         }
      }
   }
   
   BVec3Interval aabb(BVec3Interval::cInitExpand);
   
   for (uint sy = 0; sy < image.getHeight(); sy++)
   {
      for (uint sx = 0; sx < image.getWidth(); sx++)
      {
         const BRGBAColor& pixelColor = image(sx, sy);

         BVec3 v(pixelColor.r/255.0f, pixelColor.g/255.0f, pixelColor.b/255.0f);
         v -= mean;
         
         aabb.expand(BVec3(v * axis[0], v * axis[1], v * axis[2]));
      }
   }
            
#if 0
   BRGBAImage decor(image.getWidth(), image.getHeight());
   for (uint y = 0; y < image.getHeight(); y++)
   {
      for (uint x = 0; x < image.getWidth(); x++)
      {
         const BRGBAColor& pixelColor = image(x, y);

         BVec3 v(pixelColor.r/255.0f, pixelColor.g/255.0f, pixelColor.b/255.0f);
         
         v -= mean;
         
         BVec3 t(v * axis[0], v * axis[1], v * axis[2]);
                  
         t -= aabb.low();
         if (aabb.dimension(0)) t[0] /= aabb.dimension(0);                  
         if (aabb.dimension(1)) t[1] /= aabb.dimension(1);                  
         if (aabb.dimension(2)) t[2] /= aabb.dimension(2);
         
         decor(x, y) = BRGBAColor(Math::Clamp<int>((int)(t[0]*255.0f+.5f), 0, 255), Math::Clamp<int>((int)(t[1]*255.0f+.5f), 0, 255), Math::Clamp<int>((int)(t[2]*255.0f+.5f), 0, 255), 0);
      }
   }
   
   BImageUtils::writeTGA24("decor.tga", decor);
#endif   
 
   BVec3 colorDiagnol(aabb.diagonal());
 
   BAlignedArray<uchar> stream;
      
   BRGBAImage newImage;
   BRGBAImage temp(image.getWidth(), image.getHeight());
   for (uint y = 0; y < image.getHeight(); y++)
   {
      for (uint x = 0; x < image.getWidth(); x++)
      {
         const BRGBAColor& pixelColor = image(x, y);
         BRGBAColor16 yCoCg;
         BColorUtils::RGBToYCoCgR(pixelColor, yCoCg);
         temp(x, y).set(yCoCg.r, (yCoCg.g / 2) + 128, (yCoCg.b / 2) + 128, 0);
      }
   }
   
   newImage.setSize(image.getWidth(), image.getHeight());
   
   const int ni = 8;
   for (uint i = 0; i < ni; i++)
   {   
      const float bitRate[3] = { 2.0f, .3f, .3f };
      const float q = Math::Lerp(.1f, .6f, i / float(ni - 1));
      
      for (uint i = 0; i < 3; i++)
      {
         stream.clear();
         BEmbeddedDCTCodec::pack(temp, i, bitRate[i] * q, stream);
         
         LARGE_INTEGER decompStartTime;
         LARGE_INTEGER decompEndTime;
         QueryPerformanceCounter(&decompStartTime);
         BEmbeddedDCTCodec::unpack(newImage, i, stream);
         QueryPerformanceCounter(&decompEndTime);
         
         LARGE_INTEGER freq;
         QueryPerformanceFrequency(&freq);

         DWORD decompTime = static_cast<DWORD>(((*(unsigned __int64*)&decompEndTime - *(unsigned __int64*)&decompStartTime) * 1000) / *(unsigned __int64*)&freq);
         BDXTUtils::debugPrintf("DecompTime: %ims\n", decompTime);
      }
      
      for (uint y = 0; y < image.getHeight(); y++)
      {
         for (uint x = 0; x < image.getWidth(); x++)
         {
            BRGBAColor16 yCoCg(newImage(x, y));
            yCoCg.g = (yCoCg.g - 128) * 2;
            yCoCg.b = (yCoCg.b - 128) * 2;
            
            BColorUtils::YCoCgRToRGB(yCoCg, newImage(x, y));
         }
      }
      
      char filename[256];
      sprintf(filename, "decomp%03i.tga", (int)(100.0f*q*(bitRate[0]+bitRate[1]+bitRate[2])));
      BImageUtils::writeTGA24(filename, newImage);
   }      
}
#elif 1
   const bool useYCoCgR = false;
   
   BRGBAColor32 yccLo(255,255,255,255);
   BRGBAColor32 yccHi(0,-255,-255,0);
   BRGBAColor32 yccRange;
   
   if (useYCoCgR)
   {
      
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
      
      //yccLo.set(0,-255,-255,0);
      //yccHi.set(255,255,255,0);
      
      yccRange = yccHi - yccLo;
      BRGBAColor32 yccRangeHalf(yccRange / 2);
      if (yccRange[0] == 0) yccRange[0] = 1;
      if (yccRange[1] == 0) yccRange[1] = 1;
      if (yccRange[2] == 0) yccRange[2] = 1;
      yccRange.a = 1;
      
      for (uint y = 0; y < image.getHeight(); y++)
      {
         for (uint x = 0; x < image.getWidth(); x++)
         {
            BRGBAColor& pixelColor = image(x, y);
            
            BRGBAColor16 yCoCg16;
            BColorUtils::RGBToYCoCgR(pixelColor, yCoCg16);
            BRGBAColor32 yCoCg(yCoCg16);

#if 0            
            pixelColor.r = 0;//pixelColor.a;         
            pixelColor.a = (uchar)yCoCg.r;
            pixelColor.b = (uchar)Math::Clamp<int>((yCoCg.g / 2) + 123, 0, 255);
            pixelColor.g = (uchar)Math::Clamp<int>((yCoCg.b / 2) + 125, 0, 255);
#endif            
            yCoCg = yCoCg - yccLo;
            yCoCg = yCoCg * BRGBAColor32(255,255,255,0);
            yCoCg = (yCoCg + yccRangeHalf) / yccRange;
            yCoCg.clamp(0, 255);

            pixelColor.g = (uchar)yCoCg.r; //g=y
            
            pixelColor.r = (uchar)yCoCg.g; //r=co
            pixelColor.b = (uchar)yCoCg.b; //cb=g
            
            pixelColor.a = 255;
         }
      }
   }      
   
   BDXTPacker dxtPacker;
   const bool fast = false;
   const bool perceptual = useYCoCgR ? false : true;
   const bool dithering = false;
   
   LARGE_INTEGER packStartTime;
   LARGE_INTEGER packEndTime;
   QueryPerformanceCounter(&packStartTime);
   
   dxtImage.clear();
   dxtPacker.pack(image, dxtFormat, fast, perceptual, dithering, dxtImage);
   
   QueryPerformanceCounter(&packEndTime);
   
   BRGBAImage unpackedImage;
   LARGE_INTEGER unpackStartTime;
   LARGE_INTEGER unpackEndTime;
   
   QueryPerformanceCounter(&unpackStartTime);
   
   BDXTUnpacker::unpack(unpackedImage, &dxtImage[0], dxtFormat, image.getWidth(), image.getHeight());
   
   QueryPerformanceCounter(&unpackEndTime);
   
   if (useYCoCgR)
   {
      for (uint y = 0; y < unpackedImage.getHeight(); y++)
      {
         for (uint x = 0; x < unpackedImage.getWidth(); x++)
         {
            BRGBAColor& c = unpackedImage(x, y);
                     
            //BRGBAColor16 yc(c.a, (c.b - 123) * 2, (c.g - 125) * 2, c.r);
                                    
            BRGBAColor32 yc(c.g, c.r, c.b, 0);
            
            yc = ((yc * yccRange + BRGBAColor32(128,128,128,0)) / BRGBAColor32(255,255,255,1)) + yccLo;
            
            BColorUtils::YCoCgRToRGB(BRGBAColor16(yc), c);
         }
      }
   }  
   
   BImageUtils::writeTGA24("unpacked.tga", unpackedImage, false);    
   
   LARGE_INTEGER freq;
   QueryPerformanceFrequency(&freq);

   DWORD packTime = static_cast<DWORD>(((*(unsigned __int64*)&packEndTime - *(unsigned __int64*)&packStartTime) * 1000) / *(unsigned __int64*)&freq);
   DWORD unpackTime = static_cast<DWORD>(((*(unsigned __int64*)&unpackEndTime - *(unsigned __int64*)&unpackStartTime) * 1000) / *(unsigned __int64*)&freq);
   debugPrintf("Pack time: %u, Unpack time: %u\n", packTime, unpackTime);   
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

      

   g_pTexture=0;
   if (FAILED(g_pd3dDevice->CreateTexture(width,height,1,0, d3dFormat,D3DPOOL_MANAGED,&g_pTexture,0)))
      return false;

   D3DLOCKED_RECT tLockedRect;
   if(!FAILED(g_pTexture->LockRect(0, &tLockedRect,0,0)))
   {
      memcpy(tLockedRect.pBits,(BYTE *)&dxtImage[0],cellsX * cellsY * bytesPerBlock);
      g_pTexture->UnlockRect(0);
   }
   
   if (pDstFilename)
   {
      IDirect3DSurface9* pSurface;
      if (FAILED(g_pTexture->GetSurfaceLevel(0,  &pSurface)))
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
    void *pVertices = NULL;

    for (int i = 0; i < 4; i++)
    {
      g_quadVertices[i].x = (g_quadVertices[i].x * gWindowWidth) - (.5f / gWindowWidth);
      g_quadVertices[i].y = (g_quadVertices[i].y * gWindowHeight) - (.5f / gWindowHeight);
    }
    
    g_pVertexBuffer->Lock( 0, sizeof(g_quadVertices), (void**)&pVertices, 0 );
    memcpy( pVertices, g_quadVertices, sizeof(g_quadVertices) );
    g_pVertexBuffer->Unlock();

    D3DXMATRIX matProj;
    //D3DXMatrixPerspectiveFovLH( &matProj, D3DXToRadian( 45.0f ), 
    //                            512.0f / 512.0f, 0.1f, 100.0f );
    D3DXMatrixIdentity(&matProj);
    g_pd3dDevice->SetTransform( D3DTS_PROJECTION, &matProj );

	g_pd3dDevice->SetRenderState(D3DRS_LIGHTING, FALSE);

   initTexture();
}

//-----------------------------------------------------------------------------
// Name: shutDown()
// Desc: 
//-----------------------------------------------------------------------------
void shutDown( void )
{
    if( g_pTexture != NULL ) 
        g_pTexture->Release();

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

    g_pd3dDevice->SetTexture( 0, g_pTexture );
    g_pd3dDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
    g_pd3dDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
    g_pd3dDevice->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
    g_pd3dDevice->SetStreamSource( 0, g_pVertexBuffer, 0, sizeof(Vertex) );
	 g_pd3dDevice->SetFVF( D3DFVF_CUSTOMVERTEX );
	 //g_pd3dDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_ALPHAREPLICATE|D3DTA_TEXTURE);
	 g_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, 0, 2 );

    g_pd3dDevice->EndScene();
    g_pd3dDevice->Present( NULL, NULL, NULL, NULL );
    
    Sleep(60);
}

