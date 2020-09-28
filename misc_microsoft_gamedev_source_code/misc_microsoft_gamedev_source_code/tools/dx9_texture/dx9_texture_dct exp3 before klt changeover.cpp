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

const uint cNumImageTextures = 5;
const uint cChromaTextureIndex = 4;
const uint cFirstDCTTexture = cNumImageTextures;
const uint cNumDCTTextures = 3;
const uint NumTextures = cNumImageTextures+cNumDCTTextures;
LPDIRECT3DTEXTURE9      g_pTextures[NumTextures];

BVec4                   g_ChromaScale;
BVec4                   g_ChromaOfs;
BVec4                   g_ImageScale[4];
BVec4                   g_TextureDim;

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

#if 0
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
#endif   
      
   BVec4 aLow(low);
   BVec4 aHigh(high);

#if 0         
   for (uint channel = 0; channel < 4; channel++)
   {
      uint total = 0;
      for (uint i = 0; i < cHistSize; i++)
         total += hist[channel][i];

      const uint thresh = (channel == 3) ? 1 : 1;

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
#endif   
   
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

         dst(x, y).set(r, g, b, a);         
      }
   }     
   
   return BVec4(colorRange, colorRange, colorRange, alphaRange);
}

inline double Factorial(double i)
{
   for (int j = i - 1; j >= 2; j--)
      i *= j;

   return i;
}

inline double NumPermutations(double n)
{
   return Factorial(n);
}

inline double NumCombinations(double n, double k)
{
   assert(n >= k);
   assert(n > 0);
   assert(k > 0);
   if (n == k)
      return 1;

   return Factorial(n) / (Factorial(n - k) * Factorial(k));
}

template<int N>
void CalculateEigensystem(const BMatrixNxN<N,N>& src, BVecN<N>& lambda, BMatrixNxN<N,N>& r)
{
   BMatrixNxN<N,N> a(src);
      
   r.setIdentity();
   
   BVecN<N> b, d, z;
   for (uint i = 0; i < N; i++)
      b[i] = src[i][i];

   d = b;      
   z.setZero();
   
   const double epsilon = 1.0e-10F;
   const int maxSweeps = 50;
   
   for (int s = 0; s < maxSweeps; s++)
   {
      uint i;
      for (i = 0; i < N; i++)
      {
         if (fabs(d[i]) >= epsilon)
            break;
      }
      
      if (i == N)
         break;
 
      double sum = 0.0f;
      for (uint x = 0; x < N - 1; x++)
         for (uint y = x + 1; y < N; y++)
            sum += fabs(a[x][y]);
      if (sum == 0.0f)
         break;
         
      double thresh;
      if (i < 3)
         thresh = 0.2f * sum / (N * N);
      else
         thresh = 0.0f; 
         
      for (uint p = 0; p < N - 1; p++)
      {
         for (uint q = p + 1; q < N; q++)   
         {
            double g = 100.0f * fabs(a[p][q]);
                     
            if ( (s > 3) && (fabs(d[p])+g == fabs(d[p])) && (fabs(d[q])+g == fabs(d[q])) )
            {
               a[p][q] = 0.0f;
            }
            else if (fabs(a[p][q]) > thresh)
            {
               // p,p = 1,2 or 0,1
               // p=p, p=p
               
               double h = d[q] - d[p];
               double t;
               if (fabs(h)+g == fabs(h))
               {
                  t = a[p][q]/h;
               }
               else
               {
                  //double u = .5f * h / a[p][q];
                  //double u2 = u * u;
                  //double u2p1 = u2 + 1.0F;
                  //t = (u2p1 != u2) ? ((u < 0.0F) ? -1.0f : 1.0f) * (fabs(u) + sqrt(u2p1)) : 0.5f / u;
                  
                  //theta = 0.5_sp*h/a(ip,iq)
                  //   t = 1.0_sp/(ABS(theta)+SQRT(1.0_sp+theta**2))
                  //   IF(theta < 0.0) t = -t
                     
                  double theta = .5f * h / a[p][q];
                  
                  t = 1.0f/(fabs(theta)+sqrt(1.0f+theta*theta));
                  if (theta < 0.0f)
                     t = -t;
               }
               
               double c = 1.0f / sqrt(1.0f + t * t);
               double s = c * t;
               double tau = s / (1.0f + c);

               h = t * a[p][q];
               
               z[p] = z[p] - h;
               z[q] = z[q] + h;
               d[p] = d[p] - h;
               d[q] = d[q] + h;
                           
               a[p][q] = 0.0f;

               //float a1, a2;
               for (uint i = 0; i < p; i++)
               {
                  //a1 = a[i][p] - s * (a[i][q] + a[i][p] * tau);
                  //a2 = a[i][q] + s * (a[i][p] - a[i][q] * tau);
                              
                  double temp = c * a[i][p] - s * a[i][q];
                  a[i][q] = s * a[i][p] + c * a[i][q];
                  a[i][p] = temp;
               }
               
               for (uint i = p + 1; i < q; i++)
               {
                  double temp = c * a[p][i] - s * a[i][q];
                  a[i][q] = s * a[p][i] + c * a[i][q];
                  a[p][i] = temp;
               }
               
               for (uint i = q + 1; i < N; i++)
               {
                  double temp = c * a[p][i] - s * a[q][i];
                  a[q][i] = s * a[p][i] + c * a[q][i];
                  a[p][i] = temp;
               }

               for (uint i = 0; i < N; i++)
               {
                  double temp = c * r[i][p] - s * r[i][q];
                  r[i][q] = s * r[i][p] + c * r[i][q];
                  r[i][p] = temp;
               }  
            }               
         }
      }
      
      b = b + z;
      d = b;
      z.setZero();
   }

   for (uint i = 0; i < N; i++)
      lambda[i] = d[i];
}

template<typename T, typename U, typename V>
void CalcPCA(const T& vecs, U& eigenvectors, V& eigenvalues, V& mean)
{
   const uint N = V::numElements;

   eigenvalues.set(1.0f);
   eigenvectors.setIdentity();
   
   const uint numVecs = vecs.size();   
   if (!numVecs)
      return;
      
   mean.setZero();
   
   for (uint i = 0; i < numVecs; i++)
      mean += vecs[i];
   
   mean /= float(numVecs);
   
   BMatrixNxN<N,N> covar;
   covar.setZero();
   
   for (uint i = 0; i < numVecs; i++)
   {
      const V v(vecs[i] - mean);
            
      for (uint x = 0; x < N; x++)
         for (uint y = x; y < N; y++)
            covar[x][y] = covar[x][y] + v[x] * v[y];
   }
   
   for (uint x = 0; x < N - 1; x++)
      for (uint y = x + 1; y < N; y++)
         covar[y][x] = covar[x][y];
   
   for (uint x = 0; x < N; x++)
      for (uint y = 0; y < N; y++)         
         assert(covar[x][y]==covar[y][x]);
   
   covar *= 1.0f/float(numVecs);
   
   BMatrixNxN<N,N> temp(covar);
   
   CalculateEigensystem<N>(covar, eigenvalues, eigenvectors);
         
   float q;
   V k;
   V ev;
   for (uint i = 0; i < N; i++)
   {
      ev = eigenvectors.getColumn(i);
      k = ev * temp;
      q = k.len() / ev.len();
      k *= 1.0f/q;
   }
      
   for (uint i = 0; i < N - 1; i++)
   {
      for (uint j = i + 1; j < N; j++)
      {
         q = eigenvectors.getColumn(i) * eigenvectors.getColumn(j);
      }
   }
}

//-----------------------------------------------------------------------------
// Name: loadTexture2()
// Desc: 
//-----------------------------------------------------------------------------
bool loadTexture2(const char* pSrcFilename, const char* pDstFilename, int quality, int codebookSize, bool virtualCodebook, float bitRate, int method, bool subsampling, bool useAveDelta, bool useVisualQuant)
{
   //uint k = NumCombinations(3, 2);
  
   

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

#if 0
{
   typedef BAlignedArray< BVec4 > BVec4Array;
   
   BMatrix44 rot(BMatrix44::makeRotate(Math::fDegToRad(45.0f)));
   
   BVec4Array vecs;
   //for (uint i = 0; i < 256; i++)
   //   vecs.pushBack(BVec2(i,i));
   vecs.pushBack(BVec4(0,0,0,0)*rot);
   vecs.pushBack(BVec4(10,0,0,1)*rot);
   vecs.pushBack(BVec4(10,1,2,2)*rot);
   vecs.pushBack(BVec4(0,1,-2,3)*rot);
      
   BMatrix44 eigenvectors;
   BVec4 eigenvalues;
   BVec4 mean;
   CalcPCA(vecs, eigenvectors, eigenvalues, mean);
   
   BVec4 l(1e+30f);
   BVec4 h(-1e+30f);
   
   BVec4 mean(0.0f);
   for (uint i = 0; i < vecs.size(); i++)
      mean += vecs[i];
   mean /= (float)vecs.size();
   
   for (uint i = 0; i < vecs.size(); i++)
   {
      BVec4 v(vecs[i] - mean);
      
      for (uint j = 0; j < 4; j++)
      {  
         float d = eigenvectors.getColumn(j) * v;
         l[j] = Math::Min(l[j], d);
         h[j] = Math::Max(h[j], d);
      }
   }
;
}
#endif

   typedef BVecN<16> BVec16;
   typedef BMatrixNxN<16,16> BMatrix16x16;
   typedef BAlignedArray< BVec16 > BVec16Array;
   
   BVec16Array vecs;
      
   for (uint sy = 0; sy < cellsY; sy++)
   {
      for (uint sx = 0; sx < cellsX; sx++)
      {
         BVec16 d;
         
         for (uint y = 0; y < 4; y++)
            for (uint x = 0; x < 4; x++)
            {
               BRGBAColor16 yCoCg;
               BColorUtils::RGBToYCoCgR(image(sx*4+x, sy*4+y), yCoCg);
               d[x+y*4] = yCoCg.r/255.0f;
            }
         
         vecs.pushBack(d);         
      }            
   }

   BMatrix16x16 eigenvectors;
   BVec16 eigenvalues;
   BVec16 mean;
   CalcPCA(vecs, eigenvectors, eigenvalues, mean);
   
   BRGBAImage basisImage(512, 512);
   
   BVec16 t(eigenvalues);
   int eigenOrder[16];
   BVec16 eigenVecs[16];
   
   for (uint cy = 0; cy < 4; cy++)
   {
      for (uint cx = 0; cx < 4; cx++)
      {
         float maxT = 0.0f;
         int maxI = 0;
         for (uint i = 0; i < 16; i++)
         {
            if (fabs(t[i]) > maxT)
            {
               maxT = fabs(t[i]);
               maxI = i;
            }
         }
         
         debugPrintf("%i,%i %f\n", maxI&3, maxI/4, t[maxI]);
         t[maxI] = 0.0f;
      
         BVec16 v(eigenvectors.getColumn(maxI));
         v.tryNormalize();
         
         eigenVecs[cx+cy*4] = v;
         eigenOrder[cx+cy*4] = maxI;
         
         for (uint sy = 0; sy < 4; sy++)
         {
            for (uint sx = 0; sx < 4; sx++)
            {
               BRGBAColor c;
               c.set(v[sx+sy*4]*127+128);
               basisImage.fillRect(cx*48+sx*2, cy*48+sy*2, 2, 2, c);
            }
         }
      }
   }  
   
   BImageUtils::writeTGA24("basis.tga", basisImage);       

#if 0   
   BRGBAImage kltImage(image.getWidth(), image.getHeight());
   
   for (uint cy = 0; cy < cellsY; cy++)
   {
      for (uint cx = 0; cx < cellsX; cx++)
      {
         float d[4][4];
         
         for (uint y = 0; y < 4; y++)
            for (uint x = 0; x < 4; x++)
            {
               BRGBAColor16 yCoCg;
               BColorUtils::RGBToYCoCgR(image(cx*4+x, cy*4+y), yCoCg);
               d[x][y] = yCoCg.r/255.0f - mean[x+y*4];
            }
            
         float r[4][4];
         Utils::ClearObj(r);
            
         for (uint i = 0; i < 13; i++)
         {
            float sum = 0;
            
            for (uint y = 0; y < 4; y++)
               for (uint x = 0; x < 4; x++)
                  sum += eigenVecs[i][x+y*4] * d[x][y];

            if (i > 0.0f)
            {
               float sign = Math::Sign(sum);
               
               float q[] = 
               {
                  0, //0
                  64,
                  64,
                  64,
                  
                  4, //4
                  4,
                  4,
                  4,
                  
                  4, //8
                  4,
                  4,
                  4,
                  
                  4, //12
                  4,
                  4,
                  4
               
               };
               
               sum = floor(fabs(sum) * q[i]) / q[i] * sign;
            }

            for (uint y = 0; y < 4; y++)
               for (uint x = 0; x < 4; x++)
                  r[x][y] += eigenVecs[i][x+y*4] * sum;
         }
               
         for (uint y = 0; y < 4; y++)
            for (uint x = 0; x < 4; x++)
               kltImage(cx*4+x, cy*4+y).set((r[x][y]+mean[x+y*4])*255.0f+.5f);
      }
   }
      
   BImageUtils::writeTGA24("kltImage.tga", kltImage);    
#endif   

   const bool hasAlpha = false;
   D3DFORMAT d3dFormat = D3DFMT_DXT1;
   BDXTFormat dxtFormat = cDXT1;
   
   BAlignedArray<uchar> compressedData;
   
   uint bytesPerBlock = 8;   
   if (dxtFormat >= cDXT3)
      bytesPerBlock = 16;
   
   BMatrixNxN<4,4> dct( BMatrixNxNCreateDCT< BMatrixNxN<4,4> >() );

   BArray2D<BVec4> dctTextures[cNumDCTTextures];
   for (uint i = 0; i < cNumDCTTextures; i++)
      dctTextures[i].resize(4, 4);

   for (uint sy = 0; sy < 4; sy++)
   {
      for (uint sx = 0; sx < 4; sx++)
      {
         BMatrix44 i, d;
         
         i.setZero();
         i[sx][sy] = 1.0f;
         
         d = BMatrixNxNIDCT(i, dct);

#define A 3
#define R 0
#define G 1
#define B 2
         static const struct 
         {  
            char textureIndex;
            char compIndex;
         } coeffTransTable[4][4] = 
         {
            {{  0,  A }, {  1,  A }, {  1,  R }, {  2,  G }}, 
            {{  2,  A }, {  3,  A }, {  3,  R }, {  1,  B }},
            {{  1,  G }, {  3,  G }, {  3,  B }, {  0,  0 }},
            {{  2,  R }, {  2,  B }, {  0,  0 }, {  0,  0 }}
         };
#undef A
#undef R         
#undef G
#undef B
         const int textureIndex = coeffTransTable[sy][sx].textureIndex;
         const int compIndex = coeffTransTable[sy][sx].compIndex;
         if (textureIndex == 0)
            continue;
            
         for (uint y = 0; y < 4; y++)
            for (uint x = 0; x < 4; x++)
               dctTextures[textureIndex-1](x, y)[compIndex] = d[x][y] * 2.0f;
      }
   }
   
   for (uint i = 0; i < cNumDCTTextures; i++)
   {
      if (FAILED(g_pd3dDevice->CreateTexture(4, 4, 1,0, D3DFMT_A16B16G16R16F, D3DPOOL_MANAGED, &g_pTextures[cFirstDCTTexture + i],0)))
         return false;

      D3DLOCKED_RECT tLockedRect;
      if(!FAILED(g_pTextures[cFirstDCTTexture + i]->LockRect(0, &tLockedRect,0,0)))
      {
         //   memcpy(tLockedRect.pBits, (BYTE *)&dxtData[0], (dctImage[i].getWidth() / 4) * (dctImage[i].getHeight() / 4) * 16);
         for (uint y = 0; y < 4; y++)
            D3DXFloat32To16Array((D3DXFLOAT16*)((BYTE*)tLockedRect.pBits+tLockedRect.Pitch*y), &dctTextures[i](0,y)[0], 4 * 4);
                     
         g_pTextures[cFirstDCTTexture+i]->UnlockRect(0);
      }
   }
   
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
            {
               BRGBAColor16 yCoCg;
               BColorUtils::RGBToYCoCgR(image(sx*4+x, sy*4+y), yCoCg);
               d[x][y] = yCoCg.r;
            }

         i = BMatrixNxNDCT(d, dct);
                     
         float a0 = i[0][0] - 128.0f * 4;
         float r0 = 0.0f;
         float g0 = 0.0f;
         float b0 = 0.0f;
         
         float a1 = sqrtSign(i[1][0]/512.0f);
         float r1 = sqrtSign(i[2][0]/512.0f);
         float g1 = sqrtSign(i[0][2]/512.0f);
         float b1 = i[3][1]/512.0f;
                          
         float a2 = sqrtSign(i[0][1]/512.0f);
         float g2 = sqrtSign(i[3][0]/512.0f);
         float r2 = sqrtSign(i[0][3]/512.0f);
         float b2 = i[1][3]/512.0f;
         
         float a3 = sqrtSign(i[1][1]/512.0f);         
         float r3 = sqrtSign(i[2][1]/512.0f);
         float g3 = sqrtSign(i[1][2]/512.0f);
         float b3 = i[2][2]/512.0f;
                  
         //float g3 = sqrtSign(i[3][1]/512.0f);
         //float r3 = sqrtSign(i[1][3]/512.0f);
         //float b3 = 0.0f;
         
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
      
   g_ImageScale[0] = dctImageRange[0] * .25f * 1.0f/255.0f;// / 512.0f;
   g_ImageScale[1] = dctImageRange[1];
   g_ImageScale[2] = dctImageRange[2];
   g_ImageScale[3] = dctImageRange[3];

   for (uint i = 0; i < 4; i++)
   {
      char filename[256];
      sprintf(filename, "origdct%i.tga", i);
      BImageUtils::writeTGA24(filename, dctImage[i]);
      sprintf(filename, "origdcta%i.tga", i);
      BImageUtils::writeTGA24(filename, dctImage[i], 3);
      
      BAlignedArray<uchar> dxtData;
      
      if (i > 0)
      {
         BDXTPacker packer;
         packer.pack(dctImage[i], cDXT5, false, false, false, dxtData);
                                 
         BDXTUnpacker unpacker;   
         unpacker.unpack(dctImage[i], &dxtData[0], cDXT5, dctImage[i].getWidth(), dctImage[i].getHeight());
      }
      
      D3DFORMAT d3dFormat = i ? D3DFMT_DXT5 : D3DFMT_L8;
      
      if (FAILED(g_pd3dDevice->CreateTexture(dctImage[i].getWidth(), dctImage[i].getHeight(), 1,0, d3dFormat, D3DPOOL_MANAGED, &g_pTextures[i],0)))
         return false;
      
      D3DLOCKED_RECT tLockedRect;
      if(!FAILED(g_pTextures[i]->LockRect(0, &tLockedRect,0,0)))
      {
         if (d3dFormat == D3DFMT_DXT5)
            memcpy(tLockedRect.pBits, (BYTE *)&dxtData[0], (dctImage[i].getWidth() / 4) * (dctImage[i].getHeight() / 4) * 16);
         else
         {
            for (uint y = 0; y < dctImage[i].getHeight(); y++)
               for (uint x = 0; x < dctImage[i].getWidth(); x++)
                  ((BYTE*)tLockedRect.pBits)[x + y * tLockedRect.Pitch] = dctImage[i](x, y).a;
         }
         
         g_pTextures[i]->UnlockRect(0);
      }
      
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
         const BRGBAColor& c0 = dctImage[0](sx,sy);
         float fc0a = ((c0.a - 128) / 127.0f) * dctImageRange[0][3];
         float a0 = fc0a + 128.0f * 4.0f;
         float r0 = ((c0.r - 123) / 127.0f) * dctImageRange[0][0];
         float g0 = ((c0.g - 125) / 127.0f) * dctImageRange[0][1];
         float b0 = ((c0.b - 123) / 127.0f) * dctImageRange[0][2];
         
         r0 = Math::Sign(r0)*r0*r0*512.0f;
         g0 = Math::Sign(g0)*g0*g0*512.0f;
         b0 *= 512.0f;
         
         const BRGBAColor& c1 = dctImage[1](sx,sy);         
         float a1 = (c1.a-128)/127.0f*dctImageRange[1][3];
         float g1 = (c1.g-125)/127.0f*dctImageRange[1][1];
         float r1 = (c1.r-123)/127.0f*dctImageRange[1][0];
         float b1 = (c1.b-123)/127.0f*dctImageRange[1][2];
         
         a1 = Math::Sign(a1)*a1*a1*512.0f;
         r1 = Math::Sign(r1)*r1*r1*512.0f;
         g1 = Math::Sign(g1)*g1*g1*512.0f;
         b1 *= 512.0f;

         const BRGBAColor& c2 = dctImage[2](sx,sy);
         float a2 = (c2.a-128)/127.0f*dctImageRange[2][3];
         float r2 = (c2.r-123)/127.0f*dctImageRange[2][0];
         float g2 = (c2.g-125)/127.0f*dctImageRange[2][1];
         float b2 = (c2.b-123)/127.0f*dctImageRange[2][2];
         
         a2 = Math::Sign(a2)*a2*a2*512.0f;
         r2 = Math::Sign(r2)*r2*r2*512.0f;
         g2 = Math::Sign(g2)*g2*g2*512.0f;
         b2 *= 512.0f;

         const BRGBAColor& c3 = dctImage[3](sx,sy);
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

         d[1][0] = a1;
         d[2][0] = r1;
         d[0][2] = g1;
         d[3][1] = b1;
         
         d[0][1] = a2;
         d[0][3] = r2;
         d[3][0] = g2;
         d[1][3] = b2;
         
         d[1][1] = a3;
         d[2][1] = r3;
         d[1][2] = g3;
         d[2][2] = b3;
         
         i = BMatrixNxNIDCT(d, dct);
         
         for (uint y = 0; y < 4; y++)
            for (uint x = 0; x < 4; x++)
               dctComp(sx*4+x, sy*4+y).set(i[x][y]);
         
      }
   }
            
   BImageUtils::writeTGA24("dctcomp.tga", dctComp);      
   
   g_TextureDim.set(cellsX, cellsY, 0, 0);
      
   //------------------------------------------------------------------------
         
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
   
   g_ChromaScale = BVec4(yccRange.r/255.0f, yccRange.g/255.0f, yccRange.b/255.0f, 0);
   g_ChromaOfs = BVec4(yccLo.r/255.0f, yccLo.g/255.0f, yccLo.b/255.0f, 0);
      
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
               
   BDXTPacker dxtPacker;
   
   BRGBAImage subsampledChromaImage;
   BImageResampler resampler;
   resampler.resample(chromaImage, subsampledChromaImage, image.getWidth()/4, image.getHeight()/4, 4, false, false, "kaiser", 1.0f);
   
   BAlignedArray<uchar> subsampledChromaDXTImage;
   dxtPacker.pack(subsampledChromaImage, dxtFormat, false, false, false, subsampledChromaDXTImage);   
         
   BRGBAImage unpackedChromaImage;
   BDXTUnpacker::unpack(unpackedChromaImage, &subsampledChromaDXTImage[0], dxtFormat, subsampledChromaImage.getWidth(), subsampledChromaImage.getHeight());
   
   BImageUtils::writeTGA24("unpackedChroma.tga", unpackedChromaImage);    
   
   BRGBAImage upsampledUnpackedChromaImage;
   resampler.resample(unpackedChromaImage, upsampledUnpackedChromaImage, image.getWidth(), image.getHeight(), 4, false, false, "tent", 1.0f);

   BRGBAImage finalImage(image.getWidth(), image.getHeight());      
   for (uint y = 0; y < finalImage.getHeight(); y++)
   {
      for (uint x = 0; x < finalImage.getWidth(); x++)
      {
         BRGBAColor& c = upsampledUnpackedChromaImage(x, y);
         
         BRGBAColor& l = dctComp(x, y);
         BRGBAColor32 yc(l.g, c.r, c.g, 0);
                                                      
         yc = ((yc * yccRange + BRGBAColor32(128,128,128,0)) / BRGBAColor32(255,255,255,255)) + yccLo;
         
         BColorUtils::YCoCgRToRGB(BRGBAColor16(yc), finalImage(x, y));
      }
   }
   
   BImageUtils::writeTGA24("final.tga", finalImage);    
   BImageUtils::writeTGA24("finalA.tga", finalImage, 3);    
         
   if (FAILED(g_pd3dDevice->CreateTexture(subsampledChromaImage.getWidth(),subsampledChromaImage.getHeight(),1,0, d3dFormat,D3DPOOL_MANAGED,&g_pTextures[4],0)))
      return false;

   D3DLOCKED_RECT tLockedRect;
      
   if(!FAILED(g_pTextures[cChromaTextureIndex]->LockRect(0, &tLockedRect,0,0)))
   {
      memcpy(tLockedRect.pBits,(BYTE *)&subsampledChromaDXTImage[0], (subsampledChromaImage.getWidth()/4) * (subsampledChromaImage.getHeight()/4) * bytesPerBlock);
      g_pTextures[cChromaTextureIndex]->UnlockRect(0);
   }
   
   for (uint i = 0; i < 5; i++)
   {
      IDirect3DSurface9* pSurface;
      if (FAILED(g_pTextures[i]->GetSurfaceLevel(0,  &pSurface)))
         return false;
      
      char filename[256];
      sprintf(filename, "texture%i.dds", i);
      
      hres = D3DXSaveSurfaceToFile(filename,
         D3DXIFF_DDS,
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
      
    for (uint i = 0; i < NumTextures; i++)
    {
      if( g_pTextures[i] != NULL ) 
           g_pTextures[i]->Release();
    }           
    
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

    for (uint i = 0; i < NumTextures; i++)
    {
      g_pd3dDevice->SetTexture( i, g_pTextures[i] ); 
    
      const DWORD filter = (i == cChromaTextureIndex) ? D3DTEXF_LINEAR : D3DTEXF_POINT;
                     
      g_pd3dDevice->SetSamplerState(i, D3DSAMP_MINFILTER, filter);
      g_pd3dDevice->SetSamplerState(i, D3DSAMP_MAGFILTER, filter);
      g_pd3dDevice->SetSamplerState(i, D3DSAMP_MIPFILTER, filter);
    }
    
    if (g_pPixelShader)
    {
      g_pd3dDevice->SetPixelShader(g_pPixelShader);
    
      g_pd3dDevice->SetPixelShaderConstantF(0, &g_ChromaScale[0], 1);
      g_pd3dDevice->SetPixelShaderConstantF(1, &g_ChromaOfs[0], 1);
      g_pd3dDevice->SetPixelShaderConstantF(2, (float*)&g_TextureDim, 1);
      g_pd3dDevice->SetPixelShaderConstantF(3, (float*)&g_ImageScale, 4);
    }
    
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

#if 0
   static uint frameCount;
   static uint lastTick;
   frameCount++;
   if (frameCount==200)
   {
      uint curTick = GetTickCount();
      debugPrintf("%f frames/sec\n", frameCount / ((curTick - lastTick) * .001f));
      
      frameCount = 0;
      lastTick = curTick;
   }
#endif   
   
}




