//--------------------------------------------------------------------------------------
// TriangleHLSL.cpp
//--------------------------------------------------------------------------------------
#include "xcore.h"
#include <fxl.h>
#include <xgraphics.h>

#include "xsystemlib.h"
#include "xvisual.h"
#include "xgranny.h"
#include "xgameRender.h"
#include "workDirSetup.h"
#include "consoleOutput.h"

#include "terrain.h"

#include "camera.h"
#include "fontSystem2.h"
#include "visibleLightManager.h"
#include "sceneLightManager.h"
#include "dirShadowManager.h"
#include "localShadowManager.h"
#include "LoadXBOXTerrain.h"

//==============================================================================
// Globals
//==============================================================================
static BCamera gCamera;

const float cRenderMaxBuildingUnitHeight = 16.0f;  // maximum height of shadow casting buildings/units relative to the terrain
const float cRenderDefaultFocusHeight = 2.0f; // default shadow focus height if raycast from camera to ground fails

//==============================================================================
// Directory IDs
//==============================================================================
long cDirData              = -1;
long cDirArt               = -1;
long cDirScenario          = -1;
long cDirSound             = -1;
long cDirFonts             = -1;
long cDirTerrain           = -1;
long cDirPhysics           = -1;
long cDirRenderEffects     = -1;

//==============================================================================
// setupGameDir
//==============================================================================
static long setupGameDir(const BString& dirName)
{
   BString tempDirName = gFileManager.getDirectory(cDirProduction);
   tempDirName.append(dirName);
   long dirID = gFileManager.addBaseDirectory(tempDirName);
   return dirID;
}

//==============================================================================
// setupUserDir
//==============================================================================
static long setupUserDir(const BString& userDir, const BString& dirName)
{
   BString tempDirName = userDir;
   tempDirName.append(dirName);
   long dirID = gFileManager.addBaseDirectory(tempDirName, 0, true);
   return dirID;
}

//==============================================================================
// setupGameDirectories
//
// Sets up all of our game-specific directories with the file manager. These 
// can later be obtained from the file manager using their ID.
//==============================================================================
static bool setupGameDirectories()
{
   // Validate current directory
   if(!gFileManager.doesFileExist(cDirCurrent, B("startup\\game.cfg")))
      return false;

   // Game directories
   BString gameDir = gFileManager.getDirectory(cDirProduction);
   cDirData = setupGameDir(B("data\\"));
   cDirArt = setupGameDir(B("art\\"));
   cDirScenario = setupGameDir(B("Scenario\\"));
   cDirSound = setupGameDir(B("sound\\"));
   cDirFonts = setupGameDir(B("Fonts\\"));
   cDirTerrain = setupGameDir(B("art\\terrain\\"));
   cDirPhysics = setupGameDir(B("physics"));
   cDirRenderEffects = setupGameDir("shaders\\");

   return true;
}

//==============================================================================
// consoleOutputFunc
//==============================================================================
static void consoleOutputFunc(DWORD data, BConsoleMessageCategory category, const char* pMessage)
{
   static const char* pCategoryStrings[] = { "debug", "warning", "error" };
   const uint cNumCategoryStrings = sizeof(pCategoryStrings)/sizeof(pCategoryStrings[0]);
   
   if ((!pMessage) || (category > cNumCategoryStrings))
      return;
         
   trace("[%s] %s", pCategoryStrings[category], pMessage);
}

//==============================================================================
// initLibs
//==============================================================================
static bool initLibs(void)
{
   // XSystem
   XSystemInfo xsystemInfo;
   xsystemInfo.mUseXFS=true;
   xsystemInfo.mStartupDir=B("Startup\\");
   xsystemInfo.mPreAssertCallback=NULL;
   xsystemInfo.mPostAssertCallback=NULL;
   xsystemInfo.mpConsoleInterface=NULL;
   XSystemCreate(&xsystemInfo);
   
   if (!setupGameDirectories())
   {
      XSystemRelease();
      return false;
   }
   
   // Renderer
   XGameRenderInfo renderInfo;
   renderInfo.mDirArt=cDirArt;
   renderInfo.mDirEffects=cDirRenderEffects;
   renderInfo.mDirFonts=cDirFonts;
   XGameRenderCreate(&renderInfo);

   // Visual manager
   XVisualInfo xvisualInfo;
   xvisualInfo.mDirID=cDirArt;
   xvisualInfo.mDirName="art\\";
   XVisualCreate(&xvisualInfo);
   
   gConsoleOutput.init(consoleOutputFunc, 0);
   
   return true;
}

//==============================================================================
// deinitLibs
//==============================================================================
static void deinitLibs(void)
{
   XVisualRelease();
   
   XGameRenderRelease();
   
   XSystemRelease();
}   

//==============================================================================
// initScene
//==============================================================================
static bool initScene(void)
{
   //BString filename("scenario\\blank\\blank");
   BString filename("scenario\\wasteland\\wasteland");
   if(!loadXBOXTerrain(-1, filename))
      return false;
      
   gCamera.setCameraLoc(BVector(10,10,10));
   
   gCamera.lookAtPosFixedY(BVector(20,2,20));
   
   return true;
}

//==============================================================================
// deinitScene
//==============================================================================
static void deinitScene(void)
{

}

//==============================================================================
// updateRenderMatrices
//==============================================================================
static void updateRenderMatrices(void)
{
   BCamera* camera=&gCamera;
   BMatrix matrix;
   camera->getViewMatrix(matrix);
   gRender.getViewParams().setViewMatrix(matrix);

   float l = 0.0f;
   float r = 1.0f;
   float t = 0.0f;
   float b = 1.0f;

   gRender.getViewParams().setOffCenterFactor(l, t, r, b);
   gRender.getViewParams().setViewportAndProjection(0, 0, gRender.getWidth(), gRender.getHeight(), .25f, 600.0f, gRender.getViewParams().getFOV());

   gRender.setRenderTarget(0, gRenderDraw.getDevBackBuffer());
   gRender.setDepthStencilSurface(gRenderDraw.getDevDepthStencil());
}

//==============================================================================
// renderTimings
//==============================================================================
static void renderTimings(void)
{
   BHandle fontHandle=gFontManager.findFont("Arial 16 Outlined");
   gFontManager.setFont(fontHandle);

   float sx=.1f;
   float sy=.1f;
   float yh=gFontManager.getLineHeight();

   BFixedString<128> t;
   t.format("Hello, world");
   
   gFontManager.drawText(fontHandle, sx, sy, t, cColorGreen, 1.0f, 1.0f, BFontManager2::cJustifyLeft);
}


//==============================================================================
// renderUpdate
//==============================================================================
static void renderUpdate(void)
{
   updateRenderMatrices();       

   const float lastFrameLength = 1.0f/30.0f;
   gRender.frameBegin(lastFrameLength);

   // Set the global world Y extents. These values are used by various rendering subsystems to improve culling.
   gRender.setWorldHeightExtent(gTerrain.getMin().y, gTerrain.getMax().y + cRenderMaxBuildingUnitHeight);

   gSceneLightManager.lockLights();
   gVisibleLightManager.update(gRender.getWorldMinY(), gRender.getWorldMaxY());

   gGrannyInstanceRenderer.renderBegin();
}

//==============================================================================
// renderBegin(void)
//==============================================================================
static void renderBegin(void)
{
   gRenderDraw.beginScene();      
}
//==============================================================================
// renderFinish
//==============================================================================
static void renderFinish(void)
{
   gSceneLightManager.unlockLights();

   gGrannyInstanceRenderer.reset();
   gGrannyInstanceRenderer.renderFinish();

   gRenderDraw.unsetAll();
   gRenderDraw.endScene();
   gRenderDraw.present(NULL, NULL);

   gRenderThread.frameEnd();
   gRenderThread.kickCommands();
}

//==============================================================================
// renderPrep
//==============================================================================
static void renderPrep(void)
{
   BScopedPIXNamedEventRender PIXNamedEvent2("BModeGameRenderPrep");

   // No actual rendering to the backbuffer can occur here! 
   // This method queues up all granny instances for later culling, and prepares
   // the terrain engine to render all visible+shadow casting chunks.

   float focusHeight = cRenderDefaultFocusHeight;

   BVec3 worldMin(&gTerrain.getMin().x);
   BVec3 worldMax(&gTerrain.getMax().x);

   // Increase the ceiling a bit to account for buildings and units.
   worldMax[1] += cRenderMaxBuildingUnitHeight;


   BVolumeCuller& volumeCuller = gRenderDraw.getMainActiveVolumeCuller();

   volumeCuller.disableBasePlanes();

   volumeCuller.enableInclusionPlanes(gVisibleLightManager.getAllLightInterestVolume(), gVisibleLightManager.getNumInterestVolumePlanes());

//   mWorld->render();
//   gUserManager.render(true);

   volumeCuller.disableInclusionPlanes();

   volumeCuller.enableBasePlanes();


   gLocalShadowManager.shadowGenPrep(worldMin, worldMax, focusHeight);

   gVisibleLightManager.prep();


   gGrannyInstanceRenderer.updateShadowStatus();

   gGrannyInstanceRenderer.sort();



   gDirShadowManager.shadowGenPrep(worldMin, worldMax, focusHeight);

   gTerrain.evalSceneNodes(BTerrain::cRPVisible, gRenderDraw.getMainSceneMatrixTracker().getWorldCamPosVec4(), gRenderDraw.getMainSceneVolumeCuller());

   for (uint shadowPassIndex = 0; shadowPassIndex < gDirShadowManager.getNumPasses(); shadowPassIndex++)
   {
      gTerrain.evalSceneNodes(static_cast<BTerrain::eRenderPass>(shadowPassIndex), gRenderDraw.getMainSceneMatrixTracker().getWorldCamPosVec4(), gDirShadowManager.getPassVolumeCuller(shadowPassIndex));
   }

   for (uint shadowPassIndex = 0; shadowPassIndex < gLocalShadowManager.getNumPasses(); shadowPassIndex++)
   {
      gTerrain.evalSceneNodes(BTerrain::cRPLocalShadowBuffer, gRenderDraw.getMainSceneMatrixTracker().getWorldCamPosVec4(), gLocalShadowManager.getPassVolumeCuller(shadowPassIndex));
   }

   gTerrain.evalLODLevels();      
}

//==============================================================================
// renderShadowBuffers
//==============================================================================
static void renderShadowBuffers(void)
{
   BScopedPIXNamedEventRender PIXNamedEvent2("BModeGameRenderShadowGen");

   for (uint shadowPassIndex = 0; shadowPassIndex < gDirShadowManager.getNumPasses(); shadowPassIndex++)
   {
      PIXBeginNamedEventNoColor("DirShadowPass%i", shadowPassIndex);

      gDirShadowManager.shadowGenBegin(shadowPassIndex);

      gRenderDraw.setRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

      gGrannyInstanceRenderer.flush(cUGXGeomPassShadowGen, gDirShadowManager.getPassVolumeCuller(shadowPassIndex));

      gRenderDraw.setRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

      gTerrain.render(cTRP_ShadowGen, (BTerrain::eRenderPass)shadowPassIndex);

      gDirShadowManager.shadowGenEnd(shadowPassIndex);

      PIXEndNamedEvent();
   }

   if (gLocalShadowManager.getNumPasses())
   {
      gLocalShadowManager.shadowGenInit();

      for (uint shadowPassIndex = 0; shadowPassIndex < gLocalShadowManager.getNumPasses(); shadowPassIndex++)
      {
         PIXBeginNamedEventNoColor("LocalShadowPass%i", shadowPassIndex);

         gLocalShadowManager.shadowGenBegin(shadowPassIndex);

         gRenderDraw.setRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

         gGrannyInstanceRenderer.flush(cUGXGeomPassShadowGen, shadowPassIndex);

         gRenderDraw.setRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

         gTerrain.render(cTRP_ShadowGen, BTerrain::cRPLocalShadowBuffer, &gRenderDraw.getMainSceneMatrixTracker().getWorldCamPosVec4(), &gLocalShadowManager.getPassVolumeCuller(shadowPassIndex));

         gLocalShadowManager.shadowGenEnd(shadowPassIndex);

         PIXEndNamedEvent();
      }

      gLocalShadowManager.shadowGenDeinit();
   }
}    

//==============================================================================
// renderVisibleScene
//==============================================================================
static void renderVisibleScene(void)
{
   BScopedPIXNamedEventRender PIXNamedEvent3("BModeGameRenderVisible");

   const DWORD clearColor = D3DCOLOR_ARGB(255,7,7,7); //D3DCOLOR_ARGB(255,154,156,252)
   gRenderDraw.clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_STENCIL | D3DCLEAR_ZBUFFER, clearColor, 1.0f, 0);

   const bool wireframe = false;
   gRenderDraw.setRenderState(D3DRS_FILLMODE, wireframe ? D3DFILL_WIREFRAME : D3DFILL_SOLID);

   gSceneLightManager.updateLightIntrinsics(cLCUnits);

   gGrannyInstanceRenderer.flush(cUGXGeomPassMain, gRenderDraw.getMainSceneVolumeCuller());

   gSceneLightManager.updateLightIntrinsics(cLCTerrain);
   
   gTerrain.render(cTRP_Full);

#ifdef ENABLE_PARTICLES
   gPSManager.render();
#endif

#ifndef BUILD_FINAL   
   gVisibleLightManager.drawDebugInfo();
   gSceneLightManager.drawDebugInfo();
#endif   

   gDebugPrimitives.render();
}
//==============================================================================
// renderUI
//==============================================================================
static void renderUI(void)
{
#ifndef BUILD_FINAL   
   gDirShadowManager.drawDebugInfo();
   gLocalShadowManager.drawDebugInfo();
#endif   

   renderTimings();
}

//==============================================================================
// render
//==============================================================================
static void render(void)
{  
   BScopedPIXNamedEventRender PIXNamedEvent("BModeGameRender");

   renderUpdate();

   renderPrep();

   renderBegin();
   
   renderShadowBuffers();

   renderVisibleScene();

   renderUI();

   renderFinish();
}

//==============================================================================
// update
//==============================================================================
static void update(void)
{
}

//==============================================================================
// mainLoop
//==============================================================================
static void mainLoop(void)
{
   for ( ; ; )
   {
      update();
      render();
   }      
}

//==============================================================================
// main
//==============================================================================
void main(void)
{
   if (!initLibs())
      return;

   if (initScene())
   {
      mainLoop();
   
      deinitScene();
   }
      
   deinitLibs();
}
