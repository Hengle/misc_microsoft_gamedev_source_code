// File: d3dScreenCap.cpp
#include "xcore.h"
#include "d3dScreenCap.h"
#include "timer.h"

static LRESULT CALLBACK WindowProc( HWND hWnd, UINT   msg, WPARAM wParam, LPARAM lParam )
{
   switch( msg )
   {	
   case WM_CLOSE:
   case WM_DESTROY:
      PostQuitMessage(0);
      break;
   default:
      return DefWindowProc( hWnd, msg, wParam, lParam );
   }

   return 0;
}

BD3DScreenCapture::BD3DScreenCapture() :
   mhWnd(0),
   mpD3D(NULL),
   mpD3DDevice(NULL),
   mpSurface(NULL),
   mWidth(0),
   mHeight(0),
   mCaptureTime(0.0f)
{
   WNDCLASSEX winClass; 
      
   winClass.lpszClassName = "SCREENCAP_WINDOWS_CLASS";
   winClass.cbSize        = sizeof(WNDCLASSEX);
   winClass.style         = CS_HREDRAW | CS_VREDRAW;
   winClass.lpfnWndProc   = WindowProc;
   winClass.hInstance     = (HINSTANCE)GetModuleHandle(0);
   winClass.hIcon	        = LoadIcon(NULL, IDI_WINLOGO);
   winClass.hIconSm	     = LoadIcon(NULL, IDI_WINLOGO);
   winClass.hCursor       = LoadCursor(NULL, IDC_ARROW);
   winClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
   winClass.lpszMenuName  = NULL;
   winClass.cbClsExtra    = 0;
   winClass.cbWndExtra    = 0;

   if( !RegisterClassEx(&winClass) )
   {
      trace("RegisterClassEx() failed.");
   }
}

BD3DScreenCapture::~BD3DScreenCapture()
{
   deinit();
}

HRESULT BD3DScreenCapture::init(void)
{
   int nDefaultWidth = 32;
   int nDefaultHeight = 32;
   DWORD dwWindowStyle = 0;//WS_OVERLAPPEDWINDOW | WS_VISIBLE;
   RECT rc;        
   SetRect( &rc, 0, 0, nDefaultWidth, nDefaultHeight );        
   AdjustWindowRect( &rc, dwWindowStyle, false);

   mhWnd = CreateWindowEx( WS_EX_NOACTIVATE, "SCREENCAP_WINDOWS_CLASS", 
      "screencap",
      dwWindowStyle,
      0, 0, (rc.right-rc.left), (rc.bottom-rc.top), NULL, NULL, (HINSTANCE)GetModuleHandle(0), NULL );

   if (!mhWnd)
   {
      trace("Unable to create dummy window.");
      deinit();
      return E_FAIL;
   }

   UpdateWindow(mhWnd);

   D3DDISPLAYMODE	ddm;
   D3DPRESENT_PARAMETERS	d3dpp;

   if ((mpD3D = Direct3DCreate9(D3D_SDK_VERSION)) == NULL)
   {
      trace("Direct3DCreate9 failed");
      deinit();
      return E_FAIL;
   }

   if (FAILED(mpD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &ddm)))
   {
      trace("GetAdapterDisplayMode failed");
      deinit();
      return E_FAIL;
   }

   ZeroMemory(&d3dpp,sizeof(D3DPRESENT_PARAMETERS));

   d3dpp.Windowed = true;
   d3dpp.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
   d3dpp.BackBufferFormat = ddm.Format;
   d3dpp.BackBufferHeight = mHeight = ddm.Height;
   d3dpp.BackBufferWidth = mWidth = ddm.Width;
   d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE;
   d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
   d3dpp.hDeviceWindow = mhWnd;
   d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;
   d3dpp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;

   if (FAILED(mpD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, mhWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING , &d3dpp, &mpD3DDevice)))
   {
      trace("Unable to Create Device");
      deinit();
      return E_FAIL;
   }

   if (FAILED(mpD3DDevice->CreateOffscreenPlainSurface(ddm.Width, ddm.Height, D3DFMT_A8R8G8B8, D3DPOOL_SCRATCH, &mpSurface, NULL)))
   {
      trace("Unable to Create Surface");
      deinit();
      return E_FAIL;
   }

   return S_OK;
}

void BD3DScreenCapture::deinit(void)
{
   if (mpSurface)
   {
      mpSurface->Release();
      mpSurface = NULL;
   }
   
   if (mpD3DDevice)
   {
      mpD3DDevice->Release();
      mpD3DDevice = NULL;
   }
   
   if (mpD3D)
   {
      mpD3D->Release();
      mpD3D = NULL;
   }
   
   if (mhWnd)
   {
      DestroyWindow(mhWnd);
      mhWnd = NULL;
   }
}

HRESULT BD3DScreenCapture::capture(void)
{
   if (!mpD3DDevice)
      return E_FAIL;
      
   BTimer timer;
   timer.start();
         
   HRESULT hres = mpD3DDevice->GetFrontBufferData(0, mpSurface);
   if (FAILED(hres))
   {
      reset();
      
      hres = mpD3DDevice->GetFrontBufferData(0, mpSurface);
   }
   
   mCaptureTime = timer.getElapsedSeconds();
   
   if (FAILED(hres))
      return hres;  
   
   return true;  
}

bool BD3DScreenCapture::getBits(const DWORD*& pData, uint& pitch)
{
   if (!mpSurface)
      return false;

   D3DLOCKED_RECT lockedRect;
   mpSurface->LockRect(&lockedRect, NULL, 0);
   mpSurface->UnlockRect();

   pData = static_cast<DWORD*>(lockedRect.pBits);
   pitch = lockedRect.Pitch;

   return true;
}

HRESULT BD3DScreenCapture::reset(void)
{
   D3DDISPLAYMODE	ddm;
   D3DPRESENT_PARAMETERS	d3dpp;

   if (mpSurface)														//Release the Surface - we need to get the latest surface
   {
      mpSurface->Release();
      mpSurface = NULL;
   }

   if (FAILED(mpD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &ddm)))	//User might have changed the mode - Get it afresh
   {
      trace("Unable to Get Adapter Display Mode");
      return E_FAIL;
   }

   ZeroMemory(&d3dpp,sizeof(D3DPRESENT_PARAMETERS));

   d3dpp.Windowed = true;
   d3dpp.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
   d3dpp.BackBufferFormat = ddm.Format;
   d3dpp.BackBufferHeight = mHeight = ddm.Height;
   d3dpp.BackBufferWidth = mWidth = ddm.Width;
   d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE;
   d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
   d3dpp.hDeviceWindow = mhWnd;
   d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;
   d3dpp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;

   if (FAILED(mpD3DDevice->Reset(&d3dpp)))
   {
      trace("Unable to Create Device");				
      return E_FAIL;
   }

   if (FAILED(mpD3DDevice->CreateOffscreenPlainSurface(ddm.Width, ddm.Height, D3DFMT_A8R8G8B8, D3DPOOL_SCRATCH, &mpSurface, NULL)))
   {
      trace("Unable to Create Surface");
      return E_FAIL;
   }
   
   return S_OK;
}
