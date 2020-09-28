//-----------------------------------------------------------------------------
// File: dx9_test.cpp
// Copyright (c) Microsoft Corporation. All rights reserved.
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
#include "common/filesys/file_system.h"

#include "common/render/spherical_harmonic.h"
#include "x86/win32/dx9/render/d3d_cubemap.h"

#include "x86/win32/dx9/render/render_engine.h"
#include "x86/win32/dx9/render/vector_font.h"
#include "x86/win32/dx9/render/hdr_screenshot.h"
#include "x86/win32/dx9/render/console_dx9.h"
#include "x86/win32/dx9/render/scene.h"

#include "x86/win32/input/keyboard_win32.h"
#include "x86/win32/input/joystick_win32.h"

#include "x86/win32/dx9/core/system_dx9.h"

#include "x86/core/timer_x86.h"

#if DEBUG
  #include <crtdbg.h>
#endif

#include "sph_poly/sph_poly.h"

#define TITLE_MESSAGE			"FD Viewer"
#define COPYRIGHT_MESSAGE "(C) Copyright 2003-2004 Microsoft Corp.\n"
    
namespace gr
{

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

Scene* gpScene;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void CMyD3DApplication::initTweaker(void)
{
  gSharedTweaker.init();

  gSharedTweaker.createEntry(eMisc, new IntegralTweakerEntry("Scene anim reset", &mSceneAnimReset));
  gSharedTweaker.createEntry(eMisc, new IntegralTweakerEntry("Scene anim pause", &mSceneAnimPaused));
  gSharedTweaker.createEntry(eMisc, new IntegralTweakerEntry("Scene anim speed", &mSceneAnimSpeed, 0.1f, 3.0f, .01f, false, 0, NULL, false));
  gSharedTweaker.createEntry(eMisc, new IntegralTweakerEntry("Scene anim time", &mSceneAnimTime, -8000.0f, 8000.0f, 1.0f/7.5f, false, 0, NULL, false));
  gSharedTweaker.createEntry(eMisc, new IntegralTweakerEntry("Scene camera index (-1=free)", &mCurCameraIndex, -1, 50, 1, true));
  gSharedTweaker.createEntry(eMisc, new IntegralTweakerEntry("Camera spin speed", &mCamSpinSpeed, -360.0f, 360.0f, 5.0f, false, 0, NULL, false));
  gSharedTweaker.createEntry(eMisc, new IntegralTweakerEntry("Audio start skip", &mAudioStartSkip, 0.0f, 5.0f, .0125f, false, 0, NULL, false));

  RenderEngine::initStaticTweakers();

  Scene::initStaticTweakers();
}

//-----------------------------------------------------------------------------
// Name: CMyD3DApplication()
// Desc: Application constructor.   Paired with ~CMyD3DApplication()
//       Member variables should be initialized to a known state here.  
//       The application window has not yet been created and no Direct3D device 
//       has been created, so any initialization that depends on a window or 
//       Direct3D should be deferred to a later stage. 
//-----------------------------------------------------------------------------

CMyD3DApplication::CMyD3DApplication() :
  mMenuFont("Impact", 20, 0)
{
		//Camera camera;
		//camera.setViewToProj(1.0f, 0.00001f, 100.0f, Math::fDegToRad(179.999f));

    gDX9System.enableConsoleWindow();
    mLogfile.setTraceEcho(true);
    
    mLogfile.printf(TITLE_MESSAGE " Build %s %s\n", __DATE__, __TIME__);
    mLogfile.printf(COPYRIGHT_MESSAGE);
    
    //mLogfile.printf("Setting process affinity mask to CPU 0\n");
    //SetProcessAffinityMask(GetCurrentProcess(), 1);
        
    m_dwCreationWidth           = 800;
    m_dwCreationHeight          = 608;
    m_strWindowTitle            = TEXT( "FD Viewer" );
    m_d3dEnumeration.AppUsesDepthBuffer   = TRUE;
    m_bStartFullscreen          = false;
    m_bShowCursorWhenFullscreen = true;
	
		resetCamera();
        
    ZeroMemory( &m_UserInput, sizeof(m_UserInput) );

    mCurAnimTime = 0.0f;

    mMenuVisible = false;

    initTweaker();

    m_UserInput.prevMouseX = -100;
    m_UserInput.prevMouseY = -100;
    
#if 0
    // Read settings from registry
    ReadSettings();
#endif

    mSceneAnimSpeed = 1.0f;
    mSceneAnimPrevSpeed = 1.0f;
    mSceneAnimPaused = false;
    mSceneAnimReset = false;
    mSceneAnimPrevPaused = false;
    mSceneAnimTime = 0.0f;

    mpD3DTextureFactory = NULL;

    mFov = 67.5f;
    mMouseInputDelay = .5f;
    
    mNumFrames = 0;
    mFPS = 0;
    mFPSNumFrames = 0;
    mFPSTickCount = GetTickCount();
    
    mCurCameraIndex = -1;
    
    mpAudioSys = NULL;
    mAudioStartSkip = 0.0f;
    
    mParsedCommandLine = false;
    mLoadedNewScene = false;
    
    mCamSpinSpeed = 0.0f;
    
    mRenderCPUTime = 0.0f;
    mMinRenderCPUTime = Math::fNearlyInfinite;
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
    RenderEngine::setMainWindow(m_hWnd);

    RenderEngine::oneTimeSceneInit();

    Assert(!gpScene);
    gpScene = new Scene();      
    gpScene->oneTimeSceneInit();
    
    gDX9Console.setFont(&mMenuFont);
    gDX9Console.setDisplayParameters(12, 12, .5f, .5f, .75f, D3DCOLOR_ARGB(255, 246, 220, 220));

    gKeyboard.init(m_hWnd, g_hInst);
    gJoystick.init(m_hWnd, g_hInst);

    Assert(!mpD3DTextureFactory);
    mpD3DTextureFactory = new D3DTextureFactory();
    RenderEngine::textureManager().add(mpD3DTextureFactory);
        
    mpAudioSys = AudioSystem::CreateAudioSystem(m_hWnd);
            
    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: ReadSettings()
// Desc: Read the app settings from the registry
//-----------------------------------------------------------------------------
VOID CMyD3DApplication::readSettings()
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
VOID CMyD3DApplication::writeSettings()
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
  RenderEngine::setDevice(m_pd3dDevice);
       
  RenderEngine::initDeviceObjects();

  gpScene->initDeviceObjects();
  
  mMenuFont.InitDeviceObjects(m_pd3dDevice);
  
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
  mMenuFont.RestoreDeviceObjects();
  
  RenderEngine::restoreDeviceObjects();

  gpScene->restoreDeviceObjects();
      
  return S_OK;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void CMyD3DApplication::filterConcat(char*& pDst, const char* pSrc)
{
  int len = strlen(pSrc);
  memcpy(pDst, pSrc, len + 1);
  pDst += strlen(pSrc) + 1;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void CMyD3DApplication::reloadScene(void)
{
	if (!mSceneFilename.empty())
		loadScene(mSceneFilename);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void CMyD3DApplication::loadScene(const BigString& filename)
{
	mSceneFilename = filename;
	
	const BigString sceneRootPath(BigString(filename).removeFilename().convertToPathname());
	const BigString sceneRawFilename(BigString(filename).removePath());
	
	Status("loadScene: Root path: \"%s\", Scene filename: \"%s\"\n", sceneRootPath.c_str(), sceneRawFilename.c_str());
	
  gFileSystem.clearSearchPaths();
  gFileSystem.addSearchPath(sceneRootPath);
  gFileSystem.addSearchPath(sceneRootPath + "data");
  
  gpScene->clear();
  
  mpAudioSys->Clear();
          
  bool failed = gpScene->load(sceneRawFilename);
  
  mCurCameraIndex = -1;
        
  if (failed)
    MessageBox(m_hWnd, "Error", "Scene load failed!", MB_OK|MB_ICONEXCLAMATION); 
  else
  {
    if (gpScene->numCameras())
      mCurCameraIndex = 0;
      
    std::pair<BigString, int> audioFilename(
			gFileSystem.resolveFilename(BigString(sceneRawFilename).removeExtension() + ".wav")
			);
                
    const bool succeeded = mpAudioSys->LoadTrack(std::string(audioFilename.first));
    
    if (succeeded)
    {
      mLogfile.printf("Loaded file \"%s\"\n", audioFilename.first.c_str());       
      
      mpAudioSys->Seek(mAudioStartSkip);
    }
    else
      mLogfile.printf("Unable to load file \"%s\"\n", audioFilename.first.c_str());       
  }
  
  mSceneAnimPrevPaused = true;
  mSceneAnimPaused = false;
  mSceneAnimTime = 0;
  mSceneAnimPrevSpeed = mSceneAnimSpeed = 1.0f;
  
  mLoadedNewScene = true;
  

mSceneAnimPaused = true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void CMyD3DApplication::loadFile(void)
{
  OPENFILENAME ofn;
  char filter[256];
  BigString fileName;
  BigString initialDir(".");
  int status;

  memset(filter, 0, sizeof(filter));
  char* pDst = filter;
  //filterConcat(pDst, "Geometry Files (*.ugf)");
  //filterConcat(pDst, "*.ugf");
  //filterConcat(pDst, "Animation Files (*.anm)");
  //filterConcat(pDst, "*.anm");
  filterConcat(pDst, "Scene Files (*.scn)");
  filterConcat(pDst, "*.scn");
      
  memset(&ofn, 0, sizeof(ofn));

  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = m_hWnd;
  ofn.hInstance = (HINSTANCE)GetModuleHandle(NULL);
  ofn.lpstrFilter = filter;
  ofn.lpstrFile = fileName;
  ofn.nMaxFile = fileName.bufSize();
  ofn.lpstrTitle = "Load Scene";
  ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
  ofn.lpstrDefExt = "scn";
  ofn.lpstrInitialDir = initialDir.c_str();

  status = GetOpenFileName(&ofn);

  if (status == 0)
    return;

  BigString filename(ofn.lpstrFile);
  BigString extension(filename.getExtension().tolower());
  
  if (extension == "scn")
  {
    loadScene(filename);
  }

  // HACK HACK
  mMouseInputDelay = .5f;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void CMyD3DApplication::saveFile(void)
{
  OPENFILENAME ofn;
  char filter[256];
  BigString fileName;
  BigString initialDir(".");
  int status;

  memset(filter, 0, sizeof(filter));
  char* pDst = filter;
  filterConcat(pDst, "Radiance Files (*.hdr)");
  filterConcat(pDst, "*.hdr");
      
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
    HDRScreenshot::writeRadianceFromSurf(pFile, RenderEngine::bufferManager().getSurface(eDABuffer));
    if (ferror(pFile) == 0)
      failed = false;
    if (fclose(pFile) != 0)
      failed = true;
  }
  
  if (failed)
    MessageBox(m_hWnd, "Error", "File save failed!", MB_OK|MB_ICONEXCLAMATION); 
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

Quat CMyD3DApplication::makeArcBallQuat(const Vec3& viewPos, const Vec3& va, const Vec3& vb)
{
  Vec3 b(vb);
  Vec3 a(va);

  // Flipped due to the orbit cam flip rotation.
  b[1] *= -1.0f;
  a[1] *= -1.0f;
  
  static float arcballFudge = 2.0f;

  return ArcBall(Vec3(0,0,0), viewPos, b, a, arcballFudge);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

bool CMyD3DApplication::isVKeyDown(int vkey)
{
  const int diKey = gKeyboard.vkeyToDIKey(vkey);
  if (diKey != -1)
    return gKeyboard.keyDown(diKey);
  return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

bool CMyD3DApplication::wasVKeyPressed(int vkey)
{
  const int diKey = gKeyboard.vkeyToDIKey(vkey);
  if (diKey != -1)
    return gKeyboard.keyPressed(diKey);
  return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void CMyD3DApplication::tickUpDownControl(float& v, int downVKey, int upVKey, float speed, float low, float high, bool wrap)
{
  float dir = 0.0f;
  
  if (-1 == upVKey)
  {
    if (isVKeyDown(downVKey))
    {
      dir = 1.0f;
      if ((isVKeyDown(VK_LSHIFT)) || (isVKeyDown(VK_RSHIFT)))
        dir = -1.0f;
    }
  }
  else
  {
    if (isVKeyDown(downVKey))
      dir = -1.0f;
    else if (isVKeyDown(upVKey))
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
//-----------------------------------------------------------------------------

void CMyD3DApplication::tickTweaker(void)
{
  if ((gJoystick.buttonPressed(eButtonStart)) || (wasVKeyPressed(VK_RETURN)))
    mMenuVisible = !mMenuVisible;

  if (mMenuVisible)
  {
    const float dPadPressed = gJoystick.dPadPressed();
    const float dPadDown = gJoystick.dPadDown();

    int xDir = 0, yDir = 0;
    bool first = false;

    if (dPadPressed >= 0)
      first = true;

    if (dPadDown >= 0) 
    {
      xDir = Math::Clamp<float>(256 * sin(Math::fDegToRad(dPadDown)), -1.0f, 1.0f);
      yDir = -Math::Clamp<float>(256 * cos(Math::fDegToRad(dPadDown)), -1.0f, 1.0f);
    }

    if ((wasVKeyPressed(VK_LEFT)) || (wasVKeyPressed(VK_RIGHT)) ||
        (wasVKeyPressed(VK_UP)) || (wasVKeyPressed(VK_DOWN)))
    {
      first = true;
    }

    if (isVKeyDown(VK_LEFT)) xDir = -1;
    if (isVKeyDown(VK_RIGHT)) xDir = 1;
    if (isVKeyDown(VK_UP)) yDir = -1;
    if (isVKeyDown(VK_DOWN)) yDir = 1;
    
//    Trace("%i %i %i\n", xDir, yDir, first);

    gSharedTweaker.get().tick(first, xDir, yDir);
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void CMyD3DApplication::updateCameraOrientRelEuler(float degsPerSec, float x, float y)
{
  EulerAngles eul(EulerAngles::makeFromMatrix(mOrient, EulOrdYXZr));
  float r = Math::fDegToRad(m_fElapsedTime * degsPerSec);
  eul.x += x * r;
  eul.y += y * r;
  eul.y = Math::Clamp(eul.y, Math::fDegToRad(-89.9f), Math::fDegToRad(89.9f));
  mOrient = EulerAngles::createMatrix(eul);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void CMyD3DApplication::updateMainCamera(void)
{
  mOrient.orthonormalize();
  mTrueOrient.orthonormalize();

  Quat::slerp(Quat(mTrueOrient), Quat(mOrient), .5f).toMatrix(mTrueOrient);

  mTrueCamPOI = Vec4::lerp(mTrueCamPOI, mCamPOI, .5f);
  mTrueCamDist = Math::Lerp(mTrueCamDist, mCamDist, .5f);

  Matrix44 tran0(Matrix44::makeTranslate(Vec4(-mTrueCamPOI, 1.0f)));
  Matrix44 tran1(Matrix44::makeTranslate(Vec4(0,0,-mTrueCamDist,1.0f)));
  Matrix44 flip(Matrix44::makeScale(Vec4(-1,1,-1.0f,1.0f)));
  
  const float fAspect = ((float)m_d3dsdBackBuffer.Width) / m_d3dsdBackBuffer.Height;
  const float zNear = 2.5f;//3.0f;
  const float zFar = 10000.0f;
    
  if (mCurCameraIndex >= gpScene->numCameras())
    mCurCameraIndex = gpScene->numCameras() - 1;
      
  if (-1 == mCurCameraIndex) 
  {
    mRenderViewport.camera().setWorldToView(tran0 * mTrueOrient * tran1 * flip);
      
    mRenderViewport.camera().setViewToProj(fAspect, zNear, zFar, Math::fDegToRad(mFov));
  }
  else
  {
    mRenderViewport.camera() = gpScene->getCamera(mCurCameraIndex, fAspect, zNear, zFar);
    
    mCamPOI = mTrueCamPOI = mRenderViewport.camera().viewToWorld().getTranslate();    
  }
  
  mRenderViewport.updateMatrices();
  
	gpScene->setInstanceModelToWorld("probeobject", Matrix44::makeTranslate(mTrueCamPOI));
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void CMyD3DApplication::updateCamPOI(const Vec4& ofs)
{
  mCamPOI += ofs;

  mCamPOI[0] = Math::Clamp(mCamPOI[0], -50000.0f, 50000.0f);
  mCamPOI[2] = Math::Clamp(mCamPOI[2], -50000.0f, 50000.0f);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

float CMyD3DApplication::nicePower(float f, float p)
{
  return Math::Sign(f) * pow(fabs(f), p);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void CMyD3DApplication::tickGamepadControls(void)
{
  //if (mMenuVisible)
  //  return;

  Vec2 thumb(gJoystick.rightThumb(.18f));
  if (thumb.squaredLen())
    updateCameraOrientRelEuler(130.0f, nicePower(thumb[0], 2.0f), nicePower(thumb[1], 2.0f));
    
  if (mCamSpinSpeed)
		updateCameraOrientRelEuler(fabs(mCamSpinSpeed), Math::Sign(mCamSpinSpeed), 0.0f);
  
  if (gJoystick.buttonDown(eButtonA))
  {
    mCamDist -= m_fElapsedTime * 200.0f;
    mCamDist = Math::Clamp(mCamDist, 10.0f, 1000.0f);
  }
  if (gJoystick.buttonDown(eButtonB))
  {
    mCamDist += m_fElapsedTime * 200.0f;
    mCamDist = Math::Clamp(mCamDist, 10.0f, 1000.0f);
  }

  if (gJoystick.buttonDown(eButtonBlack))
    mFov += 45.0f * m_fElapsedTime;
  else if (gJoystick.buttonDown(eButtonWhite))
    mFov -= 45.0f * m_fElapsedTime;

  mFov = Math::Clamp(mFov, 5.0f, 150.0f);
  
#if 0
  const float dPadDown = gJoystick.dPadDown();
  Vec2 dpad(0.0f);
  
  if (dPadDown >= 0) 
  {
    dpad[0] = sin(Math::fDegToRad(dPadDown));
    dpad[1] = cos(Math::fDegToRad(dPadDown));
  }
#endif

  Vec2 dpad(gJoystick.leftThumb(.18f));
  dpad[1] *= -1.0f;
  dpad[0] = nicePower(dpad[0], 2.0f);
  dpad[1] = nicePower(dpad[1], 2.0f);

  if (dpad.squaredLen())
  {
    const float dist = 250.0f * m_fElapsedTime;
    
    if (gJoystick.trigger(1) >= .5f)
    {
      const Vec4 w(0.0f, dpad[1], 0.0f, 0.0f);
      updateCamPOI(dist * w); 
    }
    else
    {
      Vec4 v0(dpad[0], dpad[1], 0.0f, 0.0f);
      
      Vec4 w0(v0 * mRenderViewport.camera().viewToWorld());

      Vec4 v1(dpad[0], 0.0f, dpad[1], 0.0f);
      
      Vec4 w1(v1 * mRenderViewport.camera().viewToWorld());
      
      w0.y = 0.0f;
      w0.toVector();
      w1.y = 0.0f;
      w1.toVector();

      Vec4 w(w0);
      if (w1.len() > w0.len())
        w = w1;

      updateCamPOI(dist * w); 
    }
              
    
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void CMyD3DApplication::resetCamera(void)
{
  mOrient.setIdentity();
  mTrueOrient.setIdentity();
  mCamPOI.set(0,30,0,1);
  mTrueCamPOI = mCamPOI;
  mCamDist = mTrueCamDist = 150.0f;  
}
	
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void CMyD3DApplication::tickKeyboardControls(void)
{
  if (wasVKeyPressed(VK_F1))
  {
    Pause(true);

    RenderEngine::reloadEffects();

    Pause(false);
  }
  
  if (wasVKeyPressed(VK_F5))
  {
		Pause(true);
		
		reloadScene();
		
		Pause(false);
  }
  
  if (wasVKeyPressed('P'))
		mSceneAnimPaused = !mSceneAnimPaused;

  if (mMenuVisible)
    return;

  if (isVKeyDown(VK_UP))    updateCameraOrientRelEuler(90.0f, 0.0f, +1.0f);
  if (isVKeyDown(VK_DOWN))  updateCameraOrientRelEuler(90.0f, 0.0f, -1.0f);
  if (isVKeyDown(VK_LEFT))  updateCameraOrientRelEuler(90.0f, -1.0f, 0.0f);
  if (isVKeyDown(VK_RIGHT)) updateCameraOrientRelEuler(90.0f, +1.0f, 0.0f);
  
  if (isVKeyDown(VK_SPACE))	resetCamera();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void CMyD3DApplication::tickMouseControls(void)
{
  if (mMouseInputDelay > 0.0f)
  {
    m_UserInput.prevMouseX = m_UserInput.mouseX;
    m_UserInput.prevMouseY = m_UserInput.mouseY;
    m_UserInput.bMouseDown[0] = false;
    m_UserInput.bMouseDown[1] = false;
    return;
  }

  if (-100 != m_UserInput.prevMouseX)
  {
    if (m_UserInput.bMouseDown[0])
    {
      m_UserInput.bMouseDown[0] = false;
      
      if ((m_UserInput.prevMouseX != m_UserInput.mouseX) ||
          (m_UserInput.prevMouseY != m_UserInput.mouseY))
      {
        Vec3 va( mRenderViewport.matrixTracker().getViewVector(Vec2(m_UserInput.prevMouseX, m_UserInput.prevMouseY)) );
        Vec3 vb( mRenderViewport.matrixTracker().getViewVector(Vec2(m_UserInput.mouseX, m_UserInput.mouseY)) );
        
        static float arcballZ = 1.0f;
        mOrient = mOrient * Quat::createMatrix(makeArcBallQuat(Vec3(0, 0, arcballZ), va, vb));
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
      
      Vec4 w0(v0 * mRenderViewport.getMatrix(eViewToWorld));

      Vec4 v1(
        (m_UserInput.mouseX - m_UserInput.prevMouseX),
        0.0f,
        -(m_UserInput.mouseY - m_UserInput.prevMouseY),
        0.0f);
      
      Vec4 w1(v1 * mRenderViewport.getMatrix(eViewToWorld));
      
      w0.y = 0.0f;
      w0.toVector();
      w1.y = 0.0f;
      w1.toVector();

      Vec4 w(w0);
      if (w1.len() > w0.len())
        w = w1;
     
      if ((isVKeyDown(VK_LSHIFT)) || (isVKeyDown(VK_RSHIFT)))
      {
        w.x = 0.0f;
        w.y = v1.z;
        w.z = 0.0f;
      }

      mCamPOI += w;

      mCamPOI[0] = Math::Clamp(mCamPOI[0], -50000.0f, 50000.0f);
      mCamPOI[2] = Math::Clamp(mCamPOI[2], -50000.0f, 50000.0f);
    }
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void CMyD3DApplication::tickControls(void)
{
  tickGamepadControls();
  tickKeyboardControls();
  tickMouseControls();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void CMyD3DApplication::tickInputDevices(void)
{
  gKeyboard.update();
  gJoystick.update();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void CMyD3DApplication::tickMenus(void)
{
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

    loadFile();

    Pause(false);
  }
  
  if (m_UserInput.bReloadFile)
  {
		m_UserInput.bReloadFile = false;
		
		Pause(true);
		
		reloadScene();
		
		Pause(false);
  }

  if (m_UserInput.bSaveFile)
  {
    m_UserInput.bSaveFile = false;

    Pause(true);

    saveFile();

    Pause(false);
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void CMyD3DApplication::updateFramerate(void)
{
  mNumFrames++;
  mFPSNumFrames++;
  const uint curTickCount = GetTickCount();
  if ((curTickCount - mFPSTickCount) >= 1000)
  {
    mFPS = mFPSNumFrames / (.001f*float(curTickCount - mFPSTickCount));
    mFPSNumFrames = 0;
    mFPSTickCount = curTickCount;
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void CMyD3DApplication::updateSceneAnim(void)
{
  if (mSceneAnimReset)
  {
    mSceneAnimReset = false;
            
    mSceneAnimPaused = mSceneAnimPrevPaused = true;
    mSceneAnimSpeed = mSceneAnimPrevSpeed = 1.0f;
    mSceneAnimTime = 0.0f;
                    
    gpScene->resetTime();
    
    mpAudioSys->Stop();
    mpAudioSys->Seek(mAudioStartSkip);
  }
  
  bool animRepeat = false;
  
  if (!mSceneAnimPaused)
    animRepeat = gpScene->advanceTime(m_fElapsedTime * mSceneAnimSpeed);
  else
  {
    const float timeDelta = mSceneAnimTime - gpScene->curTime();
    if (timeDelta)
      animRepeat = gpScene->advanceTime(timeDelta);
  }
  
  if (animRepeat)
    mpAudioSys->Seek(mAudioStartSkip);
  
  mSceneAnimTime = gpScene->curTime();
  
  if (mSceneAnimPrevPaused != mSceneAnimPaused)
  {
    mSceneAnimPrevPaused = mSceneAnimPaused;
    
    if (mSceneAnimPaused)
      mpAudioSys->Stop();
    else
      mpAudioSys->Play(mSceneAnimSpeed);
  }
  else if ((mSceneAnimPrevSpeed != mSceneAnimSpeed) && (!mSceneAnimPaused))
  {
    mpAudioSys->Play(mSceneAnimSpeed);
  }
    
  mSceneAnimPrevSpeed = mSceneAnimSpeed;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void CMyD3DApplication::loadDefaultScene(void)
{
  if ((!mParsedCommandLine) && (mNumFrames > 10))
  {
    mParsedCommandLine = true;
    
    int argc = 0;
    LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    
    if (argc == 2)
    {
      int bufferSize = 1 + WideCharToMultiByte(CP_ACP, 0, argv[1], -1, NULL, 0, NULL, NULL);

      char *buffer = new char[bufferSize];
      ZeroMemory(buffer, sizeof(char) * bufferSize);

      WideCharToMultiByte(CP_ACP, 0, argv[1], -1, buffer, bufferSize, NULL, NULL);
            
      loadScene(buffer);
      
      delete[] buffer;
    }
  }
}

//-----------------------------------------------------------------------------
// Name: FrameMove()
// Desc: Called once per frame, the call is the entry point for animating
//       the scene.
//-----------------------------------------------------------------------------
HRESULT CMyD3DApplication::FrameMove()
{
  if (mLoadedNewScene)
  {
    mLoadedNewScene = false;
    m_fElapsedTime = 0;
  }
    
  updateFramerate();
  
  mMouseInputDelay = Math::Max<float>(0.0f, mMouseInputDelay - m_fElapsedTime);

  gConsole.clear();

  tickInputDevices();
      
  tickMenus();
  
  tickControls();
  
  updateMainCamera();

  tickTweaker();

  RenderEngine::tick(m_fElapsedTime);
  
  gpScene->tick(m_fElapsedTime);
  
  updateSceneAnim();
    
  loadDefaultScene();
        
  return S_OK;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void CMyD3DApplication::renderPointer(
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

  if (pName)
  {
    const float s = .1f;
    const Vec3 up(0,s,0);
    const Vec3 right(s,0,0);
    VectorFont::textOutInit();
    VectorFont::textOutModelSpace(pName, pos, up, right, color);
    VectorFont::textOutDeinit();
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void CMyD3DApplication::renderUI(void)
{
  D3D::invalidateCache();
  
  if (mMenuVisible)
  {
    gConsole.clear();

    gConsole.printf("FPS: %3.3f, Frame: %i, POI: %f %f %f", mFPS, mNumFrames, mTrueCamPOI[0], mTrueCamPOI[1], mTrueCamPOI[2]);
    gConsole.printf("CPU Render Time: %4.3fms Min: %4.3fms\n", mRenderCPUTime * 1000.0f, mMinRenderCPUTime * 1000.0f);

    gSharedTweaker.get().display();
  }
          
  gConsole.display();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
  
void CMyD3DApplication::renderGuides(void)
{
  D3D::setTransform(D3DTS_WORLD, Matrix44::I);

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

  renderPointer(NULL, mTrueCamPOI, Vec4(0), 0, 5, D3DCOLOR_ARGB(255,255,0,0));
  //RenderPointer("OmniPos", mOmniPos, Vec4::makeCartesian(Vec4(Math::fDegToRad(mSpotYaw), Math::fDegToRad(mSpotPitch), 1.0f, 0.0f)), mOmniRadius, 5, D3DCOLOR_ARGB(255,255,0,255));
  
  //const float s = (Vec4(mCamPOI).toPoint() * gShaderManager.getMatrix(eWorldToView)).len()/((1.0f/.015f)*25.0f);
  //const Vec3 up(Vec4(0,1,0,0) * gShaderManager.getMatrix(eViewToWorld) * s);
  //const Vec3 right(Vec4(1,0,0,0) * gShaderManager.getMatrix(eViewToWorld) * s);
}

//-----------------------------------------------------------------------------
// Name: render2D()
//-----------------------------------------------------------------------------
void CMyD3DApplication::render2D(void)
{
  //gShaderManager.setMatrix(eWorldToView, mCamera.worldToView());
  //gShaderManager.setMatrix(eViewToProj, mCamera.viewToProj());
    
  renderGuides();
  
  RenderEngine::flushLines();
                            
  renderUI();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void CMyD3DApplication::updateRenderViewport(void)
{
  mRenderViewport.setSurf(RenderEngine::bufferManager().getSurface(eKBuffer));
  
  D3DVIEWPORT9 viewport;
  viewport.X = 0;
  viewport.Y = 0;
  viewport.Width = RenderEngine::bufferManager().getBackbufferWidth();
  viewport.Height = RenderEngine::bufferManager().getBackbufferHeight();
  viewport.MinZ = 0.0f;
  viewport.MaxZ = 1.0f;
  
  mRenderViewport.setViewport(viewport);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void CMyD3DApplication::createLightProbe(void)
{
	int CubemapDim = 64;
	
	int minDim = Math::Min(RenderEngine::bufferManager().getBackbufferWidth(), RenderEngine::bufferManager().getBackbufferHeight());
	if (minDim < CubemapDim)
		return;
			
  D3DVIEWPORT9 viewport;
  viewport.X = 0;
  viewport.Y = 0;
  viewport.Width = CubemapDim;
  viewport.Height = CubemapDim;
  viewport.MinZ = 0.0f;
  viewport.MaxZ = 1.0f;
	
	RenderViewport probeViewport(mRenderViewport);
  
  probeViewport.setViewport(viewport);
  
  const float fAspect = 1.0f;
  const float zNear = 2.5f;
  const float zFar = 10000.0f;
  probeViewport.camera().setViewToProj(fAspect, zNear, zFar, Math::fDegToRad(90.0f));
    
	Vec4 camLoc(probeViewport.camera().getPos());
	
	IDirect3DCubeTexture9* pCubeTex = D3D::createCubeTexture(CubemapDim, 1, 0, D3DFMT_A16B16G16R16F, D3DPOOL_SYSTEMMEM);
	Verify(pCubeTex);
	
	IDirect3DSurface9* pTempSurf = D3D::createOffscreenPlainSurface(
    RenderEngine::bufferManager().getBackbufferWidth(),
    RenderEngine::bufferManager().getBackbufferHeight(),
    D3DFMT_A16B16G16R16F,
    D3DPOOL_SYSTEMMEM);
	
	Vec4 camDir;
	for (int i = 0; i < 6; i++)
	{
		camDir = Vec4::makeAxisVector(i >> 1, (i & 1) ? -1.0f : 1.0f);
		
		float roll = 0.0f;
		if ((i == D3DCUBEMAP_FACE_NEGATIVE_Y) || (i == D3DCUBEMAP_FACE_POSITIVE_Y))
			roll = Math::fDegToRad(-90.0f);
			
		probeViewport.camera().setWorldToView(
			Matrix44::makeCamera(camLoc, camLoc + camDir, Vec4(0, 1, 0, 0), roll)
			);
					
		probeViewport.updateMatrices();
		
		m_pd3dDevice->Clear( 0L, NULL, D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL, D3DCOLOR_ARGB(255,0,0,0), 1.0f, 0L );
		
		gpScene->startOfFrame();  
		  
		RenderEngine::beginScene(probeViewport);

		gpScene->render(false);
				
		RenderEngine::endScene();
                
		D3D::getRenderTargetData(RenderEngine::bufferManager().getSurface(eFBuffer), pTempSurf);
		
		D3DLOCKED_RECT srcRect;
		Verify(SUCCEEDED(pTempSurf->LockRect(&srcRect, NULL, D3DLOCK_READONLY)));

		IDirect3DSurface9* pCubeSurf;
		Verify(SUCCEEDED(pCubeTex->GetCubeMapSurface(static_cast<D3DCUBEMAP_FACES>(i), 0, &pCubeSurf)));
		
		D3DLOCKED_RECT dstRect;
		Verify(SUCCEEDED(pCubeSurf->LockRect(&dstRect, NULL, 0)));

		const int bpp = sizeof(DWORD) * 2;
		for (int y = 0; y < CubemapDim; y++)
		{
			Assert(dstRect.Pitch >= CubemapDim * bpp);
			
			memcpy(
				reinterpret_cast<uchar*>(dstRect.pBits) + y * dstRect.Pitch, 
				reinterpret_cast<const uchar*>(srcRect.pBits) + y * srcRect.Pitch,
				CubemapDim * bpp);
		}
				
		pCubeSurf->UnlockRect();
		
		pCubeSurf->Release();		
				
		pTempSurf->UnlockRect();
	}
	
	HRESULT hres = D3DXSaveTextureToFile("lightprobe_spec.dds", D3DXIFF_DDS, pCubeTex, NULL);
	
	SphericalHarmonic::Vec9Vector coeff = SphericalHarmonic::project(D3DCubeMap(pCubeTex));

	D3DCubeMap irrad(CubemapDim, CubemapDim);
	SphericalHarmonic::unprojectIrrad(irrad, coeff);
	irrad.get(pCubeTex);
	hres = D3DXSaveTextureToFile("lightprobe_diff.dds", D3DXIFF_DDS, pCubeTex, NULL);
			
	pTempSurf->Release();	
	pCubeTex->Release();
}

void CMyD3DApplication::renderScene(void)
{
	uint32 startCycles = Time::ReadCycleCounter();
	
  RenderEngine::startOfFrame(m_fElapsedTime);
    
  gpScene->startOfFrame();

  updateRenderViewport();

  RenderEngine::beginScene(mRenderViewport);
  
  gpScene->render(true);
  
  RenderEngine::endScene();
          
  render2D();
  
	if (wasVKeyPressed(VK_F9))
		createLightProbe();

#if 0
	if (wasVKeyPressed(VK_F8))
		RenderEngine::dynaCube().createIrradVolume(gpScene);
	
	if (wasVKeyPressed(VK_F7))
		RenderEngine::dynaCube().loadIrradVolume();
#endif		

	uint32 endCycles = Time::ReadCycleCounter();
	
	mRenderCPUTime = Time::CyclesToSeconds(endCycles - startCycles);
	if (0 == (mNumFrames % 400))
		mMinRenderCPUTime = Math::fNearlyInfinite;
	mMinRenderCPUTime = Math::Min(mMinRenderCPUTime, mRenderCPUTime);
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
    m_pd3dDevice->Clear( 0L, NULL, D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL, D3DCOLOR_ARGB(255,0,0,0), 1.0f, 0L );

    if (SUCCEEDED(m_pd3dDevice->BeginScene()))
    {
      renderScene();
      
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

    if (!bPause)
      resetInputDevices();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void CMyD3DApplication::resetInputDevices(void)
{
  gJoystick.clearState();
  gKeyboard.clearState();

  m_UserInput.prevMouseX = -100;
  m_UserInput.prevMouseY = -100;
}

//-----------------------------------------------------------------------------
// Name: MsgProc()
// Desc: Overrrides the main WndProc, so the sample can do custom message
//       handling (e.g. processing mouse, keyboard, or menu commands).
//-----------------------------------------------------------------------------
static void getWindowPadding(HWND hWnd, int& extraX, int& extraY)
{
  WINDOWINFO winInfo;
  GetWindowInfo(hWnd, &winInfo);
  DWORD m_dwWindowStyle = winInfo.dwStyle;
  
  bool hasMenu = true;

  RECT rc;
  SetRect( &rc, 0, 0, 1, 1);        
  AdjustWindowRect( &rc, m_dwWindowStyle, hasMenu);

  extraX = (rc.right - rc.left) - 1;
  extraY = (rc.bottom - rc.top) - 1;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

LRESULT CMyD3DApplication::MsgProc( HWND hWnd, UINT msg, WPARAM wParam,
                                    LPARAM lParam )
{
    switch( msg )
    {
				case WM_ACTIVATE:
				{
					//if (LOWORD(wParam) == WA_INACTIVE)
					{
						gKeyboard.clearState();
						gKeyboard.ignoreKeys(1);
						Trace("Clearing keyboard state\n");
					}
					
					break;			
	      }
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
             case ID_FILE_RELOAD:
								m_UserInput.bReloadFile = TRUE;
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
        case WM_GETMINMAXINFO:
        {
          int extraX, extraY;
          getWindowPadding(hWnd, extraX, extraY);

          LPMINMAXINFO pMinMaxInfo = (LPMINMAXINFO)lParam;
          
          const int tileWidth = RenderEngine::lightingEngine().getTileWidth();
          const int tileHeight = RenderEngine::lightingEngine().getTileHeight();

          int maxCellsX = (pMinMaxInfo->ptMaxSize.x - extraX) / tileWidth;
          int maxCellsY = (pMinMaxInfo->ptMaxSize.y - extraY) / tileHeight;

          pMinMaxInfo->ptMaxSize.x = extraX + Math::Max(1, maxCellsX - 1) * tileWidth;
          pMinMaxInfo->ptMaxSize.y = extraY + Math::Max(1, maxCellsY - 1) * tileHeight;

          int minCellsX = (pMinMaxInfo->ptMinTrackSize.x - extraX) / tileWidth;
          int minCellsY = (pMinMaxInfo->ptMinTrackSize.y - extraY) / tileHeight;

          pMinMaxInfo->ptMinTrackSize.x = extraX + Math::Max(1, minCellsX + 1) * tileWidth;
          pMinMaxInfo->ptMinTrackSize.y = extraY + Math::Max(1, minCellsY + 1) * tileHeight;

          return 0;
        }
        case WM_WINDOWPOSCHANGING:
        {
          LPWINDOWPOS pPos = (LPWINDOWPOS)lParam;

          if (pPos->cx > 0)
          {
            int extraX, extraY;
            getWindowPadding(hWnd, extraX, extraY);

            const int tileWidth = RenderEngine::lightingEngine().getTileWidth();
            const int tileHeight = RenderEngine::lightingEngine().getTileHeight();
            
            int clientWidth = pPos->cx - extraX;
            int clientHeight = pPos->cy - extraY;
            int newClientWidth = (clientWidth / tileWidth) * tileWidth;
            int newClientHeight = (clientHeight / tileHeight) * tileHeight;

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
  RenderEngine::invalidateDeviceObjects();
  
  gpScene->invalidateDeviceObjects();
  
  mMenuFont.InvalidateDeviceObjects();
  
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
  RenderEngine::deleteDeviceObjects();  

  gpScene->deleteDeviceObjects();
        
  mMenuFont.DeleteDeviceObjects();

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
  writeSettings();
  
  delete gpScene;//, _alignof(Scene));
  gpScene = NULL;
  
  RenderEngine::clear();
       
  return S_OK;
}

} // namespace gr

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

static void SetCrtDebugFlag(void)
{
#if DEBUG
  int tmp;

  // Get the current bits
  tmp = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);

  // Clear the upper 16 bits and OR in the desired freqency
  tmp = (tmp & 0x0000FFFF);// | _CRTDBG_CHECK_EVERY_16_DF;

  // Set the new bits
  _CrtSetDbgFlag(tmp);
#endif
}

//-----------------------------------------------------------------------------
// Name: WinMain()
// Desc: Entry point to the program. Initializes everything, and goes into a
//       message-processing loop. Idle time is used to render the scene.
//-----------------------------------------------------------------------------
INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR, INT )
{
  SetCrtDebugFlag();

  gr::CMyD3DApplication d3dApp;

  gr::g_pApp  = &d3dApp;
  gr::g_hInst = hInst;

  InitCommonControls();

  if( FAILED( d3dApp.Create( hInst ) ) )
      return 0;

  return d3dApp.Run();
}
