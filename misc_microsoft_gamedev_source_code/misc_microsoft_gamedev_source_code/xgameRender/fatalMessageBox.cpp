// File: fatalMessageBox.cpp
#include "xgameRender.h"
#include "BD3D.h"

#ifndef BUILD_FINAL

LPCWSTR g_pwstrButtons[1] = { L"Okay" };

class BXboxMessageBox
{
public:
   BXboxMessageBox(void) :
      mpD3D(NULL),
      mpDev(NULL)
   {
   }
   
   void show(const char* pTitle, const char* pMsg)
   {
      long c = MultiByteToWideChar(CP_ACP, 0, pTitle, strlen(pTitle), mTitle, 255);
      mTitle[c] = L'\0';
      
      c = MultiByteToWideChar(CP_ACP, 0, pMsg, strlen(pMsg), mMessage, 255);
      mMessage[c] = L'\0';

      ZeroMemory(&mOverlapped, sizeof(mOverlapped));

      initD3D();

      showMessageBox();

      deinitD3D();
   }
      
private:
   XOVERLAPPED         mOverlapped;           
   WCHAR               mTitle[256];   
   WCHAR               mMessage[256];         
   MESSAGEBOX_RESULT   mResult;               
      
   IDirect3D9*         mpD3D;
   IDirect3DDevice9*   mpDev;
   
   void initD3D(void)
   {
      // Create the D3D object.
      mpD3D = Direct3DCreate9( D3D_SDK_VERSION );
      BVERIFY(mpD3D);

      // Set up the structure used to create the D3DDevice.

      D3DPRESENT_PARAMETERS pp; 
      ZeroMemory( &pp, sizeof(pp) );
      pp.BackBufferWidth        = 640;
      pp.BackBufferHeight       = 480;
      pp.BackBufferFormat       = D3DFMT_A8R8G8B8;
      pp.MultiSampleType        = D3DMULTISAMPLE_NONE;
      pp.MultiSampleQuality     = 0;
      pp.BackBufferCount        = 1;
      pp.EnableAutoDepthStencil = TRUE;
      pp.AutoDepthStencilFormat = D3DFMT_D24S8;
      pp.SwapEffect             = D3DSWAPEFFECT_DISCARD;
      pp.PresentationInterval   = D3DPRESENT_INTERVAL_ONE;

      // Create the Direct3D device.
      mpD3D->CreateDevice( 0, D3DDEVTYPE_HAL, NULL, D3DCREATE_HARDWARE_VERTEXPROCESSING, &pp, &mpDev);   
      BVERIFY(mpDev);
   }
   
   void deinitD3D(void)
   {
      if (mpDev) { mpDev->Release(); mpDev = NULL; }
      if (mpD3D) { mpD3D->Release(); mpD3D = NULL; }
   }
   
   void showMessageBox(void)
   {
      DWORD dwRet;

      ZeroMemory(&mOverlapped, sizeof(mOverlapped));
      ZeroMemory(&mResult, sizeof(mResult));

      dwRet = XShowMessageBoxUI( XUSER_INDEX_ANY,
         mTitle,                   // Message box title
         mMessage,  // Message string
         sizeof(g_pwstrButtons)/sizeof(g_pwstrButtons[0]),// Number of buttons
         g_pwstrButtons,             // Button captions
         0,                          // Button that gets focus
         XMB_ERRORICON,              // Icon to display
         &mResult,                  // Button pressed result
         &mOverlapped );

      if (dwRet == ERROR_IO_PENDING)
      {
         for ( ; ; )
         {
            mpDev->Clear(0, NULL, D3DCLEAR_STENCIL|D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, D3DCOLOR_ARGB(255,60,30,30), 1.0f, 0);
            
            Sleep(14);
            
            mpDev->Present(NULL, NULL, NULL, NULL);
            
            if( XHasOverlappedIoCompleted( &mOverlapped ) )
               break;
         }
      }         
   }
};

void showFatalMessageBox(const char* pTitle, const char* pMsg)
{
   // Should only be called before the device is initialized in the normal manner.
   if (BD3D::mpDev)
      return;
      
   BXboxMessageBox messageBox;
   
   messageBox.show(pTitle, pMsg);
}
#endif
