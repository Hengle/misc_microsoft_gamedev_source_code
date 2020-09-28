//-----------------------------------------------------------------------------
// File: dx9_test.cpp
//
// Desc: DirectX window application created by the DirectX AppWizard
//-----------------------------------------------------------------------------
#define STRICT
#define DIRECTINPUT_VERSION 0x0800
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <basetsd.h>
#include <math.h>
#include <stdio.h>
#include <d3dx9.h>
#include <dxerr9.h>
#include <tchar.h>
#include <dinput.h>
#include "DXUtil.h"
#include "D3DEnumeration.h"
#include "D3DSettings.h"
#include "D3DApp.h"
#include "D3DFont.h"
#include "D3DUtil.h"
#include "DIUtil.h"
#include "resource.h"
#include "dx9_test.h"

#include "common/core/core.h"
#include "common/math/quat.h"
#include "common/math/arcball.h"
#include "common/math/euler_angles.h"

#include "x86/win32/dx9/render/state_cache.h"
#include "x86/win32/dx9/render/device.h"
#include "x86/win32/dx9/render/shader_manager.h"
#include "x86/win32/dx9/render/lighting_engine.h"

#include "x86/win32/dx9/render/vector_font.h"
#include "x86/win32/dx9/render/hdr_screenshot.h"

#include "common/geom/convex_polyhedron.h"
#include "common/render/tilerizer.h"

#include "unigeom_viewer.h"

using namespace gr;

//-----------------------------------------------------------------------------
// Defines, and constants
//-----------------------------------------------------------------------------
// This GUID must be unique for every game, and the same for 
// every instance of this app.  // {BF4E76CD-6E61-4B04-BD58-76388AF643D1}
// The GUID allows DirectInput to remember input settings
GUID g_guidApp = { 0xBF4E76CD, 0x6E61, 0x4B04, { 0xBD, 0x58, 0x76, 0x38, 0x8A, 0xF6, 0x43, 0xD1 } };

//-----------------------------------------------------------------------------
// Global access to the app (needed for the global WndProc())
//-----------------------------------------------------------------------------
CMyD3DApplication* g_pApp  = NULL;
HINSTANCE          g_hInst = NULL;

UnigeomViewer gUnigeomViewer;

//-----------------------------------------------------------------------------
// Name: WinMain()
// Desc: Entry point to the program. Initializes everything, and goes into a
//       message-processing loop. Idle time is used to render the scene.
//-----------------------------------------------------------------------------
INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR, INT )
{
    CMyD3DApplication d3dApp;

    g_pApp  = &d3dApp;
    g_hInst = hInst;

    InitCommonControls();

    if( FAILED( d3dApp.Create( hInst ) ) )
        return 0;

    return d3dApp.Run();
}




//-----------------------------------------------------------------------------
// Name: CMyD3DApplication()
// Desc: Application constructor.   Paired with ~CMyD3DApplication()
//       Member variables should be initialized to a known state here.  
//       The application window has not yet been created and no Direct3D device 
//       has been created, so any initialization that depends on a window or 
//       Direct3D should be deferred to a later stage. 
//-----------------------------------------------------------------------------

float saturate(float i)
{
	if (i < 0) i = 0;
	if (i > 1) i = 1;
	return i;
}

CMyD3DApplication::CMyD3DApplication()
{
		SetProcessAffinityMask(GetCurrentProcess(), 2);

    m_dwCreationWidth           = 640;
    m_dwCreationHeight          = 480;
    m_strWindowTitle            = TEXT( "dx9_test" );
    m_d3dEnumeration.AppUsesDepthBuffer   = TRUE;
    m_bStartFullscreen          = false;
    m_bShowCursorWhenFullscreen = false;

    m_bLoadingApp               = TRUE;

    mOrient.setIdentity();
    mCamPOI.set(0,30,0,1);
    mCamDist = 150.0f;  

    ZeroMemory( &m_UserInput, sizeof(m_UserInput) );

    mCurAnimTime = 0.0f;

    mOmniPos.set(40,40,-85,1);
		mOmniRadius = 166.0f;
		mOmniInner = .75f;
		mOmniOuter = 1.0f;
		mOmniPower = 2.0f;
		mOmniInten = 4.0f;

		mSpotInner = 30.0f;
		mSpotOuter = 60.0f;
		mSpotPower = 1.0f;
		mSpotYaw = 0.0f;
		mSpotPitch = -60.0f;

    // Read settings from registry
//    ReadSettings();
}




//-----------------------------------------------------------------------------
// Name: ~CMyD3DApplication()
// Desc: Application destructor.  Paired with CMyD3DApplication()
//-----------------------------------------------------------------------------
CMyD3DApplication::~CMyD3DApplication()
{
}




//-----------------------------------------------------------------------------
// Name: OneTimeSceneInit()
// Desc: Paired with FinalCleanup().
//       The window has been created and the IDirect3D9 interface has been
//       created, but the device has not been created yet.  Here you can
//       perform application-related initialization and cleanup that does
//       not depend on a device.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::OneTimeSceneInit()
{
    // TODO: perform one time initialization

    // Drawing loading status message until app finishes loading
    SendMessage( m_hWnd, WM_PAINT, 0, 0 );

    m_bLoadingApp = FALSE;

		gShaderManager.setMainWindow(m_hWnd);

		gUnigeomViewer.oneTimeInit();
				
    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: ReadSettings()
// Desc: Read the app settings from the registry
//-----------------------------------------------------------------------------
VOID CMyD3DApplication::ReadSettings()
{
    HKEY hkey;
    if( ERROR_SUCCESS == RegCreateKeyEx( HKEY_CURRENT_USER, DXAPP_KEY, 
        0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkey, NULL ) )
    {
        // TODO: change as needed

        // Read the stored window width/height.  This is just an example,
        // of how to use DXUtil_Read*() functions.
        DXUtil_ReadIntRegKey( hkey, TEXT("Width"), &m_dwCreationWidth, m_dwCreationWidth );
        DXUtil_ReadIntRegKey( hkey, TEXT("Height"), &m_dwCreationHeight, m_dwCreationHeight );

        RegCloseKey( hkey );
    }
}




//-----------------------------------------------------------------------------
// Name: WriteSettings()
// Desc: Write the app settings to the registry
//-----------------------------------------------------------------------------
VOID CMyD3DApplication::WriteSettings()
{
    HKEY hkey;

    if( ERROR_SUCCESS == RegCreateKeyEx( HKEY_CURRENT_USER, DXAPP_KEY, 
        0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkey, NULL ) )
    {
        // TODO: change as needed

        // Write the window width/height.  This is just an example,
        // of how to use DXUtil_Write*() functions.
        DXUtil_WriteIntRegKey( hkey, TEXT("Width"), m_rcWindowClient.right );
        DXUtil_WriteIntRegKey( hkey, TEXT("Height"), m_rcWindowClient.bottom );

        RegCloseKey( hkey );
    }
}

//-----------------------------------------------------------------------------
// Name: ConfirmDevice()
// Desc: Called during device initialization, this code checks the display device
//       for some minimum set of capabilities
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::ConfirmDevice( D3DCAPS9* pCaps, DWORD dwBehavior,
                                          D3DFORMAT Format )
{
    UNREFERENCED_PARAMETER( Format );
    UNREFERENCED_PARAMETER( dwBehavior );
    UNREFERENCED_PARAMETER( pCaps );
		    
    BOOL bCapsAcceptable;

    bCapsAcceptable = TRUE;

		if (dwBehavior == (D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_PUREDEVICE))
			bCapsAcceptable = FALSE;

    if( bCapsAcceptable )         
        return S_OK;
    else
        return E_FAIL;
}




//-----------------------------------------------------------------------------
// Name: InitDeviceObjects()
// Desc: Paired with DeleteDeviceObjects()
//       The device has been created.  Resources that are not lost on
//       Reset() can be created here -- resources in D3DPOOL_MANAGED,
//       D3DPOOL_SCRATCH, or D3DPOOL_SYSTEMMEM.  Image surfaces created via
//       CreateImageSurface are never lost and can be created here.  Vertex
//       shaders and pixel shaders can also be created here as they are not
//       lost on Reset().
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::InitDeviceObjects()
{
    D3D::setDevice(m_pd3dDevice);
    
    // TODO: create device objects
        
    gShaderManager.init();
    gLightingEngine.init();
		
    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: RestoreDeviceObjects()
// Desc: Paired with InvalidateDeviceObjects()
//       The device exists, but may have just been Reset().  Resources in
//       D3DPOOL_DEFAULT and any other device state that persists during
//       rendering should be set here.  Render states, matrices, textures,
//       etc., that don't change during rendering can be set once here to
//       avoid redundant state setting during Render() or FrameMove().
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::RestoreDeviceObjects()
{
  gShaderManager.restore();
  gLightingEngine.restore();

  // Set miscellaneous render states
  D3D::setRenderState( D3DRS_DITHERENABLE,    TRUE );
  D3D::setRenderState( D3DRS_SPECULARENABLE,  FALSE );
  D3D::setRenderState( D3DRS_ZENABLE,         D3DZB_TRUE );
  D3D::setRenderState( D3DRS_AMBIENT,         D3DCOLOR_ARGB(0,20,20,20) );
  D3D::setRenderState( D3DRS_LIGHTING,        TRUE );
	D3D::setRenderState(D3DRS_LASTPIXEL,			  TRUE);
  
  D3DXVECTOR3 vFromPt   = D3DXVECTOR3( 0.0f, 0.0f, -5.0f );
  D3DXVECTOR3 vLookatPt = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );
  D3DXVECTOR3 vUpVec    = D3DXVECTOR3( 0.0f, 1.0f, 0.0f );
  Matrix44 matView;
  D3DXMatrixLookAtLH( (D3DXMATRIX*)&matView, &vFromPt, &vLookatPt, &vUpVec );
  gShaderManager.setMatrix(eWorldToView, matView );
 
  FLOAT fAspect = ((FLOAT)m_d3dsdBackBuffer.Width) / m_d3dsdBackBuffer.Height;
  const float zNear = 10.0f;
  const float zFar = 5000.0f;
  Matrix44 matProj;
  D3DXMatrixPerspectiveFovLH( (D3DXMATRIX*)&matProj, D3DX_PI/4, fAspect, zNear, zFar );
  gShaderManager.setMatrix( eViewToProj, matProj );
   
  return S_OK;
}

void CMyD3DApplication::FilterConcat(char*& pDst, const char* pSrc)
{
  int len = strlen(pSrc);
  memcpy(pDst, pSrc, len + 1);
  pDst += strlen(pSrc) + 1;
}

void CMyD3DApplication::LoadFile(void)
{
  OPENFILENAME ofn;
  char filter[256];
  BigString fileName;
  BigString initialDir(".");
  int status;

  memset(filter, 0, sizeof(filter));
  char* pDst = filter;
  FilterConcat(pDst, "Geometry Files (*.ugf)");
  FilterConcat(pDst, "*.ugf");
  FilterConcat(pDst, "Animation Files (*.anm)");
  FilterConcat(pDst, "*.anm");
      
  memset(&ofn, 0, sizeof(ofn));

  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = m_hWnd;
  ofn.hInstance = (HINSTANCE)GetModuleHandle(NULL);
  ofn.lpstrFilter = filter;
  ofn.lpstrFile = fileName;
  ofn.nMaxFile = fileName.bufSize();
  ofn.lpstrTitle = "Load Geometry/Animation";
  ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
  ofn.lpstrDefExt = "ugm";
  ofn.lpstrInitialDir = initialDir.c_str();

  status = GetOpenFileName(&ofn);

  if (status == 0)
    return;

  BigString filename(ofn.lpstrFile);
  BigString extension(filename.getExtension().tolower());
  
  if (extension == "ugf")
  {
    if (gUnigeomViewer.loadGeom(filename))
      MessageBox(m_hWnd, "Error", "Geometry load failed!", MB_OK|MB_ICONEXCLAMATION); 
  }
  else if (extension == "anm")
  {
    if (gUnigeomViewer.loadAnim(filename))
      MessageBox(m_hWnd, "Error", "Animation load failed!", MB_OK|MB_ICONEXCLAMATION); 
  }
}

void CMyD3DApplication::SaveFile(void)
{
  OPENFILENAME ofn;
  char filter[256];
  BigString fileName;
  BigString initialDir(".");
  int status;

  memset(filter, 0, sizeof(filter));
  char* pDst = filter;
  FilterConcat(pDst, "Radiance Files (*.hdr)");
  FilterConcat(pDst, "*.hdr");
      
  memset(&ofn, 0, sizeof(ofn));

  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = m_hWnd;
  ofn.hInstance = (HINSTANCE)GetModuleHandle(NULL);
  ofn.lpstrFilter = filter;
  ofn.lpstrFile = fileName;
  ofn.nMaxFile = fileName.bufSize();
  ofn.lpstrTitle = "Save Radiance HDR Screenshot";
  ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
  ofn.lpstrDefExt = "hdr";
  ofn.lpstrInitialDir = initialDir.c_str();

  status = GetSaveFileName(&ofn);

  if (status == 0)
    return;
    
  bool failed = true;

  FILE* pFile = fopen(ofn.lpstrFile, "wb");
  if (pFile)
  {
    HDRScreenshot::writeRadianceFromSurf(pFile, gShaderManager.getSurface(eDABuffer));
    if (ferror(pFile) == 0)
      failed = false;
    if (fclose(pFile) != 0)
      failed = true;
  }
  
  if (failed)
    MessageBox(m_hWnd, "Error", "File save failed!", MB_OK|MB_ICONEXCLAMATION); 
}

Quat CMyD3DApplication::MakeArcBallQuat(const Vec3& viewPos, const Vec3& va, const Vec3& vb)
{
  Vec3 b(vb);
  Vec3 a(va);

	// Flipped due to the orbit cam flip rotation.
  b[1] *= -1.0f;
  a[1] *= -1.0f;

  return ArcBall(Vec3(0,0,0), viewPos, b, a, 3.0f);
}

bool CMyD3DApplication::IsVKeyDown(int vkey)
{
	if (GetForegroundWindow() != m_hWnd)
		return false;
	return GetAsyncKeyState(vkey) < 0;
}

bool CMyD3DApplication::WasVKeyPressed(int vkey)
{
	if (GetForegroundWindow() != m_hWnd)
		return false;
	return GetAsyncKeyState(vkey) & 1;
}

void CMyD3DApplication::tickUpDownControl(float& v, int downVKey, int upVKey, float speed, float low, float high, bool wrap)
{
	float dir = 0.0f;
	
	if (-1 == upVKey)
	{
		if (IsVKeyDown(downVKey))
		{
			dir = 1.0f;
			if (IsVKeyDown(VK_SHIFT))
				dir = -1.0f;
		}
	}
	else
	{
		if (IsVKeyDown(downVKey))
			dir = -1.0f;
		else if (IsVKeyDown(upVKey))
			dir = 1.0f;
	}

	v += dir * speed * m_fElapsedTime;
	
	if (wrap)
	{
		if (v < low)
			v = high;
		else if (v > high)
			v = low;
	}
	
	v = Math::Clamp(v, low, high);
}

//-----------------------------------------------------------------------------
// Name: FrameMove()
// Desc: Called once per frame, the call is the entry point for animating
//       the scene.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::FrameMove()
{
	if (IsVKeyDown(VK_F1))
	{
		Pause(true);
		
		gShaderManager.reloadEffects();

		Pause(false);
	}

  if( m_UserInput.bDoConfigureDisplay )
  {
    // One-shot per keypress
    m_UserInput.bDoConfigureDisplay = FALSE;

    Pause(true);

    // Configure the display device
    UserSelectNewDevice();

    Pause(false);
  }

  if (m_UserInput.bLoadFile)
  {
    m_UserInput.bLoadFile = false;

    Pause(true);

    LoadFile();

    Pause(false);
  }

  if (m_UserInput.bSaveFile)
  {
    m_UserInput.bSaveFile = false;

    Pause(true);

    SaveFile();

    Pause(false);
  }
  
  EulerAngles eul(EulerAngles::makeFromMatrix(mOrient, EulOrdYXZr));

  const float R = m_fElapsedTime * 45.0f;
  if (IsVKeyDown(VK_UP))
  {
    eul.y += Math::fDegToRad(R);
    mOrient = EulerAngles::createMatrix(eul);
  }
  if (IsVKeyDown(VK_DOWN))
  {
    eul.y -= Math::fDegToRad(R);
    mOrient = EulerAngles::createMatrix(eul);
  }
  if (IsVKeyDown(VK_LEFT))
  {
    eul.x -= Math::fDegToRad(R);
    mOrient = EulerAngles::createMatrix(eul);
  }
  if (IsVKeyDown(VK_RIGHT))
  {
    eul.x += Math::fDegToRad(R);
    mOrient = EulerAngles::createMatrix(eul);
  }
  else if (m_UserInput.bMouseDown[0])
  {
    m_UserInput.bMouseDown[0] = false;
    
    if ((m_UserInput.prevMouseX != m_UserInput.mouseX) ||
        (m_UserInput.prevMouseY != m_UserInput.mouseY))
    {
      Vec3 va( gShaderManager.getViewVector(Vec2(m_UserInput.prevMouseX, m_UserInput.prevMouseY)) );
      Vec3 vb( gShaderManager.getViewVector(Vec2(m_UserInput.mouseX, m_UserInput.mouseY)) );
      
      mOrient = mOrient * Quat::createMatrix(MakeArcBallQuat(Vec3(0,0,15.0f), va, vb));
    }
  }
  else if (m_UserInput.bMouseDown[2])
  {
    m_UserInput.bMouseDown[2] = false;

    mCamDist += m_UserInput.mouseY - m_UserInput.prevMouseY;
    mCamDist = Math::Clamp(mCamDist, 10.0f, 1000.0f);
  }
  else if (m_UserInput.bMouseDown[1])
  {
    m_UserInput.bMouseDown[1] = false;

    Vec4 v0(
      (m_UserInput.mouseX - m_UserInput.prevMouseX),
      -(m_UserInput.mouseY - m_UserInput.prevMouseY),
      0.0f,
      0.0f);
    
    Vec4 w0(v0 * gShaderManager.getMatrix(eViewToWorld));

    Vec4 v1(
      (m_UserInput.mouseX - m_UserInput.prevMouseX),
      0.0f,
      -(m_UserInput.mouseY - m_UserInput.prevMouseY),
      0.0f);
    
    Vec4 w1(v1 * gShaderManager.getMatrix(eViewToWorld));
    
    w0.y = 0.0f;
    w0.toVector();
    w1.y = 0.0f;
    w1.toVector();

    Vec4 w(w0);
    if (w1.len() > w0.len())
      w = w1;
   
		if (IsVKeyDown(VK_SHIFT))
		{
			w.x = 0.0f;
			w.y = -w.z;
			w.z = 0.0f;
		}

		if (IsVKeyDown(VK_CONTROL))
    {
      mOmniPos += w;
            
      mOmniPos[0] = Math::Clamp(mOmniPos[0], -2000.0f, 2000.0f);
      mOmniPos[2] = Math::Clamp(mOmniPos[2], -2000.0f, 2000.0f);
    }
    else
    {
      mCamPOI += w;

      mCamPOI[0] = Math::Clamp(mCamPOI[0], -2000.0f, 2000.0f);
      mCamPOI[2] = Math::Clamp(mCamPOI[2], -2000.0f, 2000.0f);
    }
  }

  mOrient.orthonormalize();
  
  Matrix44 tran0(Matrix44::makeTranslate(Vec4(-mCamPOI, 1.0f)));
  
  Matrix44 tran1(Matrix44::makeTranslate(Vec4(0,0,-mCamDist,1.0f)));

  Matrix44 flip(Matrix44::makeScale(Vec4(-1,1,-1.0f,1.0f)));
    
  gShaderManager.setMatrix(eWorldToView, tran0 * mOrient * tran1 * flip);

	for (int i = 0; i < eNumToneMappers; i++)
	{
		if (WasVKeyPressed('1' + i))
		{
			gLightingEngine.toneMapper() = static_cast<EToneMapper>(i);
			break;
		}
	}

	tickUpDownControl(gLightingEngine.displayGamma(), 'Q', -1, .5f, .5f, 4.0f);
	tickUpDownControl(gLightingEngine.exposure(), 'E', -1, 1.5f, -10.0f, 10.0f);
  
  if (gLightingEngine.toneMapper() == eEXRToneMapper)
	{
		tickUpDownControl(gLightingEngine.getEXRToneMapParams().mKneeLow, 'R', -1, 1.0f, -3.0f, 3.0f);
		tickUpDownControl(gLightingEngine.getEXRToneMapParams().mKneeHigh, 'T', -1, 1.0f, 3.5001f, 32.0f);
	}

	tickUpDownControl(mOmniRadius, 'A', -1, 50.0f, 1.0f, 1000.0f);
	tickUpDownControl(mOmniInner, 'S', -1, 1.0f/3.0f, 0.0f, mOmniOuter - .000125f);
	tickUpDownControl(mOmniOuter, 'D', -1, 1.0f/3.0f, mOmniInner + .000125f, 1.0f);
	tickUpDownControl(mOmniPower, 'F', -1, 1.0f, 0.05f, 50.0f);
	tickUpDownControl(mOmniInten, 'G', -1, 1.0f, 0.0f, 200.0f);
	tickUpDownControl(mSpotInner, 'C', -1, 40.0f, 0.0f, mSpotOuter - .001f);
	tickUpDownControl(mSpotOuter, 'V', -1, 40.0f, mSpotInner + .001f, 160.0f);
	tickUpDownControl(mSpotPower, 'B', -1, 1.0f, 0.001f, 40.0f);
	tickUpDownControl(mSpotYaw, 'N', -1, 120.0f, -180.0f, 180.0f, true);
	tickUpDownControl(mSpotPitch, 'M', -1, 120.0f, -180.0f, 180.0f, true);
	
  return S_OK;
}

void CMyD3DApplication::RenderCharacter(void)
{
  D3D::setRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
  D3D::setRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
      
  gUnigeomViewer.render(Matrix44::makeMaxToD3D(), mCurAnimTime);
}

void CMyD3DApplication::RenderPointer(
  const char* pName,
  const Vec4& pos,
	const Vec4& dir,
	float radius,
  float size,
  DWORD color)
{
  VectorFont::renderLineInit();
  
  VectorFont::renderLine(
    pos + Vec4(-size, 0.0f, 0.0f,0),
    pos + Vec4(size, 0.0f, 0.0f,0),
    D3DCOLOR_ARGB(255,255,0,0)
    );

  VectorFont::renderLine(
    pos + Vec4(0, -size, 0.0f,0),
    pos + Vec4(0, size, 0.0f,0),
    D3DCOLOR_ARGB(255,0,255,0)
    );

  VectorFont::renderLine(
    pos + Vec4(0, 0.0f, -size,0),
    pos + Vec4(0, 0.0f, size,0),
    D3DCOLOR_ARGB(255,0,0,255)
    );

	if (dir.norm())
	{
		VectorFont::renderLine(
    pos + Vec4(0, 0.0f, 0,0),
    pos + dir * radius,
    D3DCOLOR_ARGB(255,255,255,0)
    );
	}

  VectorFont::renderLineDeinit();

  const float s = .1f;
  const Vec3 up(0,s,0);
  const Vec3 right(s,0,0);
  VectorFont::textOutInit();
  VectorFont::textOutModelSpace(pName, pos, up, right, color);
  VectorFont::textOutDeinit();
}

void CMyD3DApplication::RenderUI(void)
{
  int height = gShaderManager.getBackbufferHeight();
  
  float s = .4f;
  const Vec2 up(0, -s);
  const Vec2 right(s, 0);
  float lineHeight = 35.0f*s;
	float colWidth = 250.0f;

  Vec2 pos(15.5f, height * .75f);
  
	VectorFont::textOutInit();
  VectorFont::textOutScreenSpace(BigString(eVarArg, "q Display Gamma: %f", gLightingEngine.displayGamma()), pos+Vec2(0, lineHeight*0), up, right);
  VectorFont::textOutScreenSpace(BigString(eVarArg, "e Exposure: %f", gLightingEngine.exposure()), pos+Vec2(0, lineHeight*1), up, right);

  if (gLightingEngine.toneMapper() == eEXRToneMapper)
	{
		VectorFont::textOutScreenSpace(BigString(eVarArg, "r Knee Low: %f", gLightingEngine.getEXRToneMapParams().mKneeLow), pos+Vec2(0, lineHeight*2), up, right);
		VectorFont::textOutScreenSpace(BigString(eVarArg, "t Knee High: %f", gLightingEngine.getEXRToneMapParams().mKneeHigh), pos+Vec2(0, lineHeight*3), up, right);
	}

	VectorFont::textOutScreenSpace(BigString(eVarArg, "a Omni Radius: %f", mOmniRadius), pos+Vec2(colWidth*1, lineHeight*0), up, right);
	VectorFont::textOutScreenSpace(BigString(eVarArg, "s Omni Inner: %f", mOmniInner), pos+Vec2(colWidth*1, lineHeight*1), up, right);
	VectorFont::textOutScreenSpace(BigString(eVarArg, "d Omni Outer: %f", mOmniOuter), pos+Vec2(colWidth*1, lineHeight*2), up, right);
	VectorFont::textOutScreenSpace(BigString(eVarArg, "f Omni Falloff Power: %f", mOmniPower), pos+Vec2(colWidth*1, lineHeight*3), up, right);
	VectorFont::textOutScreenSpace(BigString(eVarArg, "g Omni Log2 Intensity: %f Actual: %f", mOmniInten, Math::Max<float>(pow(2.0f, mOmniInten) - 1.0f, 0.0f)), pos+Vec2(colWidth*1, lineHeight*4), up, right);
	VectorFont::textOutScreenSpace(BigString(eVarArg, "c Spot Inner: %f, v Outer: %f", mSpotInner, mSpotOuter), pos+Vec2(colWidth*1, lineHeight*5), up, right);
	VectorFont::textOutScreenSpace(BigString(eVarArg, "b Spot Angle Falloff Power: %f", mSpotPower), pos+Vec2(colWidth*1, lineHeight*6), up, right);
	VectorFont::textOutScreenSpace(BigString(eVarArg, "n Spot Yaw: %f, m Spot Pitch: %f", mSpotYaw, mSpotPitch), pos+Vec2(colWidth*1, lineHeight*7), up, right);

	const char* pToneMapperName = NULL;
	switch (gLightingEngine.toneMapper())
	{
		case eEXRToneMapper:		pToneMapperName = "EXR Tone Mapper"; break;
		case eLinearToneMapper: pToneMapperName = "Linear Tone Mapper"; break;
		case eRawToneMapper:		pToneMapperName = "Raw Tone Mapper (No Gamma/Exposure)"; break;
		case eViewNormals:			pToneMapperName = "PTS Z Axis (Viewspace Normals)"; break;
		case eViewTangents:			pToneMapperName = "PTS X Axis (Viewspace Tangents)"; break;
		case eViewBinormals:		pToneMapperName = "PTS Y Axis (Viewspace Binormals)"; break;
		case eViewAlbedo:				pToneMapperName = "Albedo"; break;
	}

	VectorFont::textOutScreenSpace(BigString(eVarArg, "%s", pToneMapperName), pos+Vec2(colWidth*0, lineHeight*5), up, right);

  VectorFont::textOutDeinit();
}
  
void CMyD3DApplication::RenderGuides(void)
{
  gShaderManager.setMatrix(eModelToWorld, Matrix44::I);

  Vec3 cameraPos(0,.05f,0);//gShaderManager.getMatrix(eViewToWorld).getTranslate());
  //cameraPos[1] = 0.05f;

/*
  VectorFont::renderLineInit();

  const float xWidth = 1000.0f;
  const float zWidth = 1000.0f;
  const int NumXSegs = 100;
  const int NumZSegs = 100;

  for (int z = -zWidth; z <= zWidth; z += (zWidth * 2) / NumZSegs)
  {
    VectorFont::renderLine(
      cameraPos + Vec3(-xWidth, 0.0f, z),
      cameraPos + Vec3(+xWidth, 0.0f, z),
      D3DCOLOR_ARGB(255,40,40,40)
      );
   }

  for (int x = -xWidth; x <= xWidth; x += (xWidth * 2) / NumXSegs)
  {
    VectorFont::renderLine(
      cameraPos + Vec3(x, 0.0f, -zWidth),
      cameraPos + Vec3(x, 0.0f, +zWidth),
      D3DCOLOR_ARGB(255,40,40,40)
      );
  }
  
  VectorFont::renderLineDeinit();
*/

  RenderPointer("CamPOI", mCamPOI, Vec4(0), 0, 5, D3DCOLOR_ARGB(255,255,0,0));
  RenderPointer("OmniPos", mOmniPos, Vec4::makeCartesian(Vec4(Math::fDegToRad(mSpotYaw), Math::fDegToRad(mSpotPitch), 1.0f, 0.0f)), mOmniRadius, 5, D3DCOLOR_ARGB(255,255,0,255));
  
  //const float s = (Vec4(mCamPOI).toPoint() * gShaderManager.getMatrix(eWorldToView)).len()/((1.0f/.015f)*25.0f);
  //const Vec3 up(Vec4(0,1,0,0) * gShaderManager.getMatrix(eViewToWorld) * s);
  //const Vec3 right(Vec4(1,0,0,0) * gShaderManager.getMatrix(eViewToWorld) * s);
  /*
  const float s = .08f;
  const Vec3 up(0,s,0);
  const Vec3 right(s,0,0);
  VectorFont::textOutInit();
  VectorFont::textOut(BigString(eVarArg, "%3.3f %3.3f %3.3f %4f", mCamPOI[0], mCamPOI[1], mCamPOI[2], mCamDist), mCamPOI, up, right, D3DCOLOR_ARGB(255,255,255,0));
  VectorFont::textOutDeinit();
  */

#if 0
	if (0)
	{
		//static float oe = 1.035f;//16
		//static float re = 1.035f;//16
		static float oe = 1.045f;//12
		static float re = 1.045f;//12
		
		ConvexPolyhedron cone;
		cone.createCappedCone(
			mOmniPos, 
			Vec4::makeCartesian(Vec4(Math::fDegToRad(mSpotYaw), Math::fDegToRad(mSpotPitch), 1.0f, 0.0f)), 
			Math::fDegToRad(mSpotOuter * oe), mOmniRadius * re, 12);

		cone.clipAgainstPlane(gPolyhedron, Plane(Vec3(0,1,0), -.5f));
		
		Frustum frustum(gShaderManager.getMatrix(eWorldToProj));
		ConvexPolyhedron(gPolyhedron).clipAgainstFrustum(gPolyhedron, frustum);
				    
		D3D::setFVF(D3DFVF_XYZ);

		D3D::setRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);		
		
		D3D::setRenderState(D3DRS_LIGHTING, FALSE);
		D3D::setRenderState(D3DRS_ZENABLE, D3DZB_FALSE);

		//D3D::enableCulling();
		D3D::disableCulling();
		//D3D::enableCullingCW();
		
		if (gPolyhedron.numVerts())
		{
			D3D::drawIndexedPrimitiveUP(
				D3DPT_TRIANGLELIST,
				0,
				gPolyhedron.numVerts(),
				gPolyhedron.numTris(),
				&gPolyhedron.tri(0),
				D3DFMT_INDEX16,
				&gPolyhedron.vert(0),
				sizeof(Vec3));
		}
				
		D3D::enableCulling();
		
		D3D::setRenderState(D3DRS_ZENABLE, D3DZB_TRUE );
		D3D::setRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
		//D3D::setRenderState(D3DRS_LIGHTING, TRUE);
	}
	
	static bool s = true;
	if (s)
	{
		gPolyhedron.createSphere(
			mOmniPos, 
			mOmniRadius * 1.0f, 8, 8);

		Frustum frustum(gShaderManager.getMatrix(eWorldToProj));
		ConvexPolyhedron(gPolyhedron).clipAgainstFrustum(gPolyhedron, frustum);

		D3D::setFVF(D3DFVF_XYZ);

		D3D::setRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);		
		
		D3D::setRenderState(D3DRS_LIGHTING, FALSE);
		D3D::setRenderState(D3DRS_ZENABLE, D3DZB_FALSE);

		//D3D::disableCulling();

		D3D::enableCullingCW();
		
		if (gPolyhedron.numVerts())
		{
			D3D::drawIndexedPrimitiveUP(
				D3DPT_TRIANGLELIST,
				0,
				gPolyhedron.numVerts(),
				gPolyhedron.numTris(),
				&gPolyhedron.tri(0),
				D3DFMT_INDEX16,
				&gPolyhedron.vert(0),
				sizeof(Vec3));
		}
				
		D3D::enableCulling();
		
		D3D::setRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
		D3D::setRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
		//D3D::setRenderState(D3DRS_LIGHTING, TRUE);
	}

	{
		int width = gShaderManager.getBackbufferWidth();
		int height = gShaderManager.getBackbufferHeight();
		Tilerizer tilerizer(width, height, 16, 16);

		Matrix44 worldToScreen(gShaderManager.getMatrix(eWorldToProj) * gShaderManager.getMatrix(eProjToScreen));
		for (int i = 0; i < gPolyhedron.numTris(); i++)
		{
			Vec3 v[3];

			for (int j = 0; j < 3; j++)
			{
				const Vec4 t(Vec4(gPolyhedron.vert(gPolyhedron.tri(i)[j]), 1.0f) * worldToScreen);
				
				const float oow = (t.w > 0.0f) ? 1.0f / t.w : 0.0f;

				v[j][0] = t.x * oow;
				v[j][1] = t.y * oow;
				v[j][2] = oow;

				v[j][0] = Math::Clamp<float>(v[j][0], 0, gShaderManager.getBackbufferWidth() - 1);
				v[j][1] = Math::Clamp<float>(v[j][1], 0, gShaderManager.getBackbufferHeight() - 1);
			}

			Plane p;
			bool badTri = p.setFromTriangle(Vec3(v[0][0], v[0][1], 0), Vec3(v[1][0], v[1][1], 0),	Vec3(v[2][0], v[2][1], 0));
			
			if ((!badTri) && (p.normal()[2] < 0.0f))
       	tilerizer.rasterizeTri(v);
		}
		
/*		
		static Vec3 p[4][3];

		if (WasVKeyPressed('P'))
		{
			static int count;
			srand(count);
			count++;
			for (int i = 0; i < 4; i++)
			{
				p[i][0] = Vec3(Math::fRand(0, width - 1), Math::fRand(0, height - 1), Math::fRand(0, 255));
				p[i][1] = Vec3(Math::fRand(0, width - 1), Math::fRand(0, height - 1), Math::fRand(0, 255));
				p[i][2] = Vec3(Math::fRand(0, width - 1), Math::fRand(0, height - 1), Math::fRand(0, 255));
			}
		}
		for (int i = 0; i < 4; i++)
			tilerizer.rasterizeTri(p[i]);
*/

		VectorFont::renderLineInit();

		const int c = -10;
		const int d = -9;
		Vec4 pos(400 + tilerizer.cellsX() * .5f * c, 400 + tilerizer.cellsY() * .5f * c, -400);
		
		for (int y = 0; y < tilerizer.cellsY(); y++)
		{
			for (int x = 0; x < tilerizer.cellsX(); x++)
			{
				const int tx = x;
				const int ty = y;
				bool occupied = tilerizer.isCellOccupied(tx, ty);

				int xOfs = x * c;
				int yOfs = y * c;
				
				float z = tilerizer.getCellLimits(tx, ty).lowestZ();
				int iz = 255;//Math::Clamp(Math::FloatToIntTrunc(z + .5f), 0, 255);
				
				VectorFont::renderLine(
					pos + Vec4(xOfs, yOfs, 0.0f,0),
					pos + Vec4(xOfs + d, yOfs, 0.0f,0),
					occupied ? D3DCOLOR_ARGB(255,iz,iz,iz) : D3DCOLOR_ARGB(255,64,64,0));

				VectorFont::renderLine(
					pos + Vec4(xOfs + d, yOfs, 0.0f,0),
					pos + Vec4(xOfs + d, yOfs + d, 0.0f,0),
					occupied ? D3DCOLOR_ARGB(255,iz,iz,iz) : D3DCOLOR_ARGB(255,64,64,0));

				VectorFont::renderLine(
					pos + Vec4(xOfs + d, yOfs + d, 0.0f,0),
					pos + Vec4(xOfs, yOfs + d, 0.0f,0),
					occupied ? D3DCOLOR_ARGB(255,iz,iz,iz) : D3DCOLOR_ARGB(255,64,64,0));

				VectorFont::renderLine(
					pos + Vec4(xOfs, yOfs + d, 0.0f,0),
					pos + Vec4(xOfs, yOfs, 0.0f,0),
					occupied ? D3DCOLOR_ARGB(255,iz,iz,iz) : D3DCOLOR_ARGB(255,64,64,0));
			}
		} 

		/*
		for (int t = 0; t < 4; t++)
		{
			Vec4 l[3];
			for (int i = 0; i < 3; i++)
			{
				float cx = p[t][i][0] / tilerizer.cellWidth();
				float cy = p[t][i][1] / tilerizer.cellHeight();
				l[i] = pos + Vec4(cx * c, cy * c, 0, 0);
			}

			VectorFont::renderLine(l[0], l[1], D3DCOLOR_ARGB(255,255,255,255));
			VectorFont::renderLine(l[1], l[2], D3DCOLOR_ARGB(255,255,255,255));
			VectorFont::renderLine(l[2], l[0], D3DCOLOR_ARGB(255,255,255,255));
		}
		*/
		
		VectorFont::renderLineDeinit();
	}
#endif

}

//-----------------------------------------------------------------------------
// Name: Render()
// Desc: Called once per frame, the call is the entry point for 3d
//       rendering. This function sets up render states, clears the
//       viewport, and renders the scene.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::Render()
{
    // Clear the viewport
    m_pd3dDevice->Clear( 0L, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,
                         D3DCOLOR_ARGB(255,30,30,30), 1.0f, 0L );

    // Begin the scene
    if( SUCCEEDED( m_pd3dDevice->BeginScene() ) )
    {
      gShaderManager.beginFrame();
      gLightingEngine.beginFrame();

      D3D::invalidateCache();

      gLightingEngine.beginScene();
      
      RenderCharacter();

      gLightingEngine.endScene();

			gLightingEngine.beginLight();

/*
      static float o;
			o += m_fElapsedTime * 60.0f;

			srand(1234);
			for (int i = 0; i < 10; i++)
			{
				float r = Math::fDegToRad(o + i / 10.0f * 360.0f);
				float x = sin(r) * Math::fRand(20, 100);
				float y = cos(r) * Math::fRand(20, 100);
				Vec4 d(Math::fRand(.3f, 1.0f), Math::fRand(.3f, 1.0f), Math::fRand(.3f, 1.0f), 1.0f);
				gLightingEngine.drawLight(LightParams(Vec4(x,Math::fRand(1,50),y,1), Math::fRand(5000, 10000), d, Vec4(1), false));
			}
*/

			static Vec4 spec(1, 1, 1, .1);

			//float r=207;
			//float g=171;
			//float b=145;
			float r=255;
			float g=255;
			float b=255;
			float tt = 1.0;
			static Vec4 diff(tt*pow(r/255.0f, 2.2f), tt*pow(g/255.0f, 2.2f), tt*pow(b/255.0f, 2.2f), 1);	

			srand(1234);
			int n = 1;
			for (int i = 0; i < n; i++)
			{
				float r = (n > 1) ? 128.0f : 0;

				//Vec4 ofs = Vec4(Math::fRand(-r, r), Math::fRand(-r, r), Math::fRand(-r, r), 0);
				float t = Math::fDegToRad(360.0f * i / float(n));
				Vec4 ofs = Vec4(cos(t) * r, 0, sin(t) * r, 0);
			
				gLightingEngine.drawLight(
					LightParams(
						mOmniPos + ofs,
						Vec4::makeCartesian(Vec4(Math::fDegToRad(mSpotYaw), Math::fDegToRad(mSpotPitch), 1.0f, 0.0f)),
						mOmniRadius, 
						mOmniInner, 
						mOmniOuter, 
						mOmniPower, 
						Math::fDegToRad(mSpotInner),
						Math::fDegToRad(mSpotOuter),
						mSpotPower,
						(1.0f/float(n) * diff * Math::Max<float>(pow(2.0f, mOmniInten) - 1.0f, 0.0f)).toPoint(), 
						(1.0f/float(n) * spec * Math::Max<float>(0, pow(2.0f, mOmniInten) - 1.0f)).toPoint(), 
						false)
					);
			}

      gLightingEngine.endLight();

      float tb = 0.075f * 1.5f;
			float bb = 0.04f * 1.5f;
			FinalPassParams finalPassParams;
			finalPassParams.mHemiTop = Vec4(.2,.2,.8,0)*tb;
			finalPassParams.mHemiBottom = Vec4(.2,.5,.3,0)*bb;
			finalPassParams.mHemiAxis = Vec4(0,1,1,0).normalize();
			finalPassParams.mSkinColor = Vec4(1,0,0,0);
      gLightingEngine.beginFinal(finalPassParams);

      gLightingEngine.endFinal();
			
      RenderGuides();
      
      gUnigeomViewer.renderHier(Matrix44::makeMaxToD3D(), mCurAnimTime);

			mCurAnimTime += m_fElapsedTime;
					
      RenderUI();
      
      D3D::flushCache();

      // End the scene.
      m_pd3dDevice->EndScene();
    }

    return S_OK;
}



//-----------------------------------------------------------------------------
// Name: Pause()
// Desc: Called in to toggle the pause state of the app.
//-----------------------------------------------------------------------------
VOID CMyD3DApplication::Pause( bool bPause )
{
    CD3DApplication::Pause( bPause );
}




//-----------------------------------------------------------------------------
// Name: MsgProc()
// Desc: Overrrides the main WndProc, so the sample can do custom message
//       handling (e.g. processing mouse, keyboard, or menu commands).
//-----------------------------------------------------------------------------
LRESULT CMyD3DApplication::MsgProc( HWND hWnd, UINT msg, WPARAM wParam,
                                    LPARAM lParam )
{
    switch( msg )
    {
        case WM_KEYDOWN:
        {
          break;
        }
        case WM_MOUSEMOVE:
        {
          if (wParam & MK_LBUTTON)
            m_UserInput.bMouseDown[0] = true;

          if (wParam & MK_RBUTTON)
            m_UserInput.bMouseDown[1] = true;
          
          if (wParam & MK_MBUTTON)
            m_UserInput.bMouseDown[2] = true;
          
          m_UserInput.prevMouseX = m_UserInput.mouseX;
          m_UserInput.prevMouseY = m_UserInput.mouseY;
          m_UserInput.mouseX = LOWORD(lParam);
          m_UserInput.mouseY = HIWORD(lParam);
          break;
        }
        case WM_PAINT:
        {
            break;
        }
        case WM_COMMAND:
        {
            switch( LOWORD(wParam) )
            {
                case ID_FILE_OPEN:
                    m_UserInput.bLoadFile = TRUE;
                  break;
                case ID_FILE_SAVE_HDR:
                  m_UserInput.bSaveFile = TRUE;
                  break;
                
                case IDM_CHANGEDEVICE:
                  m_UserInput.bDoConfigureDisplay = TRUE;
                  return 0; // Don't hand off to parent
            }
            break;
        }
				case WM_WINDOWPOSCHANGING:
				{
					LPWINDOWPOS pPos = (LPWINDOWPOS)lParam;

					if (pPos->cx > 0)
					{
						WINDOWINFO winInfo;
						GetWindowInfo(hWnd, &winInfo);
						DWORD m_dwWindowStyle = winInfo.dwStyle;
						
						bool hasMenu = true;

						RECT rc;
						SetRect( &rc, 0, 0, 1, 1);        
						AdjustWindowRect( &rc, m_dwWindowStyle, hasMenu);

						int extraX = (rc.right - rc.left) - 1;
						int extraY = (rc.bottom - rc.top) - 1;

						int clientWidth = pPos->cx - extraX;
						int clientHeight = pPos->cy - extraY;
						int newClientWidth = clientWidth & ~15;
						int newClientHeight = clientHeight & ~15;

						pPos->cx += newClientWidth - clientWidth;
						pPos->cy += newClientHeight - clientHeight;
					}
          					
					break;
				}
    }

    return CD3DApplication::MsgProc( hWnd, msg, wParam, lParam );
}




//-----------------------------------------------------------------------------
// Name: InvalidateDeviceObjects()
// Desc: Invalidates device objects.  Paired with RestoreDeviceObjects()
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::InvalidateDeviceObjects()
{
    gShaderManager.release();
    gLightingEngine.release();

    D3D::clearTextures();
    D3D::invalidateCache();
    
    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: DeleteDeviceObjects()
// Desc: Paired with InitDeviceObjects()
//       Called when the app is exiting, or the device is being changed,
//       this function deletes any device dependent objects.  
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::DeleteDeviceObjects()
{
    gUnigeomViewer.clear();

    gShaderManager.deinit();
    gLightingEngine.deinit();
    
    D3D::clearTextures();
    D3D::invalidateCache();

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: FinalCleanup()
// Desc: Paired with OneTimeSceneInit()
//       Called before the app exits, this function gives the app the chance
//       to cleanup after itself.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::FinalCleanup()
{
    // Write the settings to the registry
    WriteSettings();

    return S_OK;
}

