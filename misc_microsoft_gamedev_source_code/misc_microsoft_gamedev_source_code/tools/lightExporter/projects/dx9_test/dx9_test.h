//-----------------------------------------------------------------------------
// File: dx9_test.h
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once

#include "common/math/quat.h"
#include "common/math/vector.h"
#include "common/render/camera.h"
#include "common/utils/logfile.h"

#include "x86/win32/dx9/render/render_viewport.h"
#include "x86/win32/dx9/render/d3d_texture_factory.h"
#include "x86/win32/dx9/utils/tweaker_dx9.h"
#include "x86/win32/audio/AudioSystem.h"

//-----------------------------------------------------------------------------
// Defines, and constants
//-----------------------------------------------------------------------------
#define DXAPP_KEY        TEXT("Software\\DirectX AppWizard Apps\\dx9_test")

namespace gr
{

	// Struct to store the current input state
	struct UserInput
	{
			BOOL bDoConfigureDisplay;
			BOOL bLoadFile;
			BOOL bReloadFile;
			BOOL bSaveFile;

			BOOL bMouseDown[3];
			int mouseX, prevMouseX;
			int mouseY, prevMouseY;

			bool bUp, bDown, bLeft, bRight;
	};

	//-----------------------------------------------------------------------------
	// Name: class CMyD3DApplication
	// Desc: Application class. The base class (CD3DApplication) provides the 
	//       generic functionality needed in all Direct3D samples. CMyD3DApplication 
	//       adds functionality specific to this sample program.
	//-----------------------------------------------------------------------------
	class CMyD3DApplication : public CD3DApplication
	{
			UserInput               m_UserInput;            // Struct for storing user input 

			Matrix44 mOrient;
			Matrix44 mTrueOrient;

			Vec4 mCamPOI;
			Vec4 mTrueCamPOI;
			float mFov;
			
			float mCamDist;
			float mTrueCamDist;

			float mCurAnimTime;

			CD3DFont mMenuFont;

			bool mMenuVisible;

			float mSceneAnimSpeed;
			float mSceneAnimPrevSpeed;
			bool mSceneAnimPaused;
			bool mSceneAnimReset;
			bool mSceneAnimPrevPaused;
			float mSceneAnimTime;
			float mMouseInputDelay;

			D3DTextureFactory* mpD3DTextureFactory;
			
			RenderViewport mRenderViewport;

			int mNumFrames;
			float mFPS;
			int mFPSNumFrames;
			uint mFPSTickCount;
			
			LogFile mLogfile;
			
			int mCurCameraIndex;
			
			AudioSystem* mpAudioSys;
			
			float mAudioStartSkip;
			bool mParsedCommandLine;
			bool mLoadedNewScene;
			
			BigString mSceneFilename;
			
			float mCamSpinSpeed;
			float mRenderCPUTime;
			float mMinRenderCPUTime;
						
	protected:
			virtual HRESULT OneTimeSceneInit();
			virtual HRESULT InitDeviceObjects();
			virtual HRESULT RestoreDeviceObjects();
			virtual HRESULT InvalidateDeviceObjects();
			virtual HRESULT DeleteDeviceObjects();
			virtual HRESULT Render();
			virtual HRESULT FrameMove();
			virtual HRESULT FinalCleanup();
			virtual HRESULT ConfirmDevice( D3DCAPS9*, DWORD, D3DFORMAT );
			VOID    Pause( bool bPause );

			static void filterConcat(char*& pDst, const char* pStr);
			void loadFile(void);
			void saveFile(void);
			
			VOID readSettings();
			VOID writeSettings();
			
			void renderPointer(const char* pName,	const Vec4& pos, const Vec4& dir, float radius, float size, DWORD color);

			Quat makeArcBallQuat(const Vec3& viewPos, const Vec3& va, const Vec3& vb);

			void renderGuides(void);

			void renderUI(void);
			
			bool isVKeyDown(int vkey);
			bool wasVKeyPressed(int vkey);

			void tickUpDownControl(float& v, int downVKey, int upVKey, float speed, float low, float high, bool wrap = false);
			void updateCameraOrientRelEuler(float spd, float x, float y);
			void updateCameraMatrix(void);
			void tickGamepadControls(void);
			void tickKeyboardControls(void);
			void tickMouseControls(void);
			void tickControls(void);
			void tickInputDevices(void);
			void tickMenus(void);
			void updateCamPOI(const Vec4& ofs);
			void resetInputDevices(void);

			void initTweaker(void);

			void tickTweaker(void);
			float nicePower(float f, float p);
			void updateCamera(void);
			void render2D(void);
			void renderScene(void);
			void updateRenderViewport(void);
			void updateMainCamera(void);
			void updateFramerate(void);
			void loadScene(const BigString& filename);
			void updateSceneAnim(void);
			void loadDefaultScene(void);
			void resetCamera(void);
			void reloadScene(void);
			void createLightProbe(void);
			void createIrradVolume(void);

	public:
			LRESULT MsgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );
			CMyD3DApplication();
			virtual ~CMyD3DApplication();
	};

} // namespace gr


