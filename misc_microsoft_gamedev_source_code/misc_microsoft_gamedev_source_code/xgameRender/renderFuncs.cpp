//==============================================================================
// renderFuncs.cpp
//
// Copyright (c) 2006, Ensemble Studios
//==============================================================================
#include "xgameRender.h"
#include "xssyscallmodule.h"
#include "xsconfigmacros.h"
#include "console.h"
   
#ifndef BUILD_FINAL
   #include "render.h"
   #include "tiledAA.h"
   #include "D3DTextureManager.h"
   #include "TerrainVisual.h"
   #include "terrainTexturing.h"
   #include "terrain.h"
   #include "TerrainRender.h"
   #include "worldVisibility.h"
   #include "binkInterface.h"
#endif

#ifndef BUILD_FINAL
static void reloadTexturesRenderCallback(void* pData)
{
   // Can't reload/unload/load Scaleform textures because its texture managers resolve our handles to pointers.
   gD3DTextureManager.reloadAll(BD3DTextureManager::cUGXMaterial);
}

static void unloadTexturesRenderCallback(void* pData)
{
   gD3DTextureManager.unloadAll(BD3DTextureManager::cUGXMaterial);
}

static void loadTexturesRenderCallback(void* pData)
{
   gD3DTextureManager.loadAll(BD3DTextureManager::cUGXMaterial);
}

static void reloadTextures(void)
{
   gRenderThread.submitCallback(reloadTexturesRenderCallback, NULL);
         
   gConsole.output("Reloading all textures.");
}

static void unloadTextures(void)
{
   gRenderThread.submitCallback(unloadTexturesRenderCallback, NULL);

   gConsole.output("Unloading all textures.");
}

static void loadTextures(void)
{
   gRenderThread.submitCallback(loadTexturesRenderCallback, NULL);

   gConsole.output("Loading all textures.");
}
#endif

#ifndef BUILD_FINAL

static void terrainEnableLOD(bool onOff)
{
   // FIXME: NOT THREAD SAFE!
   gTerrainVisual.setLODEnabled(onOff);

   gConsoleOutput.output(cMsgConsole, "Changed terrain LOD setting to %i.", onOff);
}

static void terrainEnableTextureLOD(bool onOff)
{
   // FIXME: NOT THREAD SAFE!  
   gTerrainTexturing.setLODEnabled(onOff);

   gConsoleOutput.output(cMsgConsole, "Changed terrain texturing LOD setting to %i.", onOff);
}


static void terrainEnableMipView(bool onOff)
{
   gTerrainTexturing.setShowMips(onOff);

   gConsoleOutput.output(cMsgConsole, "Changed viewing mips to %i.", onOff);
}

static void terrainEnableRefinement(bool onOff)
{
   gTerrainVisual.setRefinementEnabled(onOff);

   gConsoleOutput.output(cMsgConsole, "Changed refinement setting to %i.", onOff);
}

static void terrainEnableBBRender(bool onOff)
{
   gTerrain.enableQuadGridBBRender(onOff);

   gConsoleOutput.output(cMsgConsole, "Terrain Rendering BBs: %i.", onOff);
}
static void terrainEnableTexturing(bool onOff)
{
   
   gTerrain.enableRenderTextures(onOff);

   gConsoleOutput.output(cMsgConsole, "Terrain Rendering Textures: %i.", onOff);
}

static void terrainToggleDCBs()
{
   bool onOff = !gTerrainRender.isUsingDCBs();
   gTerrainRender.toggleUsingDCB();

   gConsoleOutput.output(cMsgConsole, "Terrain using DCBs: %i.", onOff);
}



static void terrainEnableCacheView(bool onOff)
{
   gTerrainTexturing.setVisualizeCache(onOff);

   gConsoleOutput.output(cMsgConsole, "Changed viewing cache to %i.", onOff);
}
static void terrainEnableRoads(bool onOff)
{
   gTerrain.enableRenderRoads(onOff);

   gConsoleOutput.output(cMsgConsole, "Terrain Rendering Roads: %i.", onOff);
}
static void terrainEnableFoliage(bool onOff)
{
   gTerrain.enableRenderFoliage(onOff);

   gConsoleOutput.output(cMsgConsole, "Terrain Rendering Foliage: %i.", onOff);
}

// hack hack
extern long cDirData;
static void playBinkMovie(const char* pString)
{
   BBinkInterface::BLoadParams lp;
   lp.mLoopVideo = false;
   lp.mIOBufSize = 14*1024*1024;
   lp.mFileManagerThrottling = true;
   lp.mWidthScale = .2f;
   lp.mHeightScale = .2f;
   lp.mXOffset = 50.0f;
   lp.mYOffset = 50.0f;
   lp.mFilename.set(pString);
   lp.mCaptionDirID=cDirData;
   lp.mCaptionFilename.set("cin0.eng.xml");

   gBinkInterface.loadActiveVideo(lp);
}

static void setExploredBrightness(float value)
{
   BWorldVisibility::getInstance().setExploredBrightness(value);
}

static void setUnexploredBrightness(float value)
{
   BWorldVisibility::getInstance().setUnexploredBrightness(value);
}

static void setEdgeOfWorldTransition(float value)
{
   BWorldVisibility::getInstance().setEdgeOfWorldTransition(value);
}

#endif

bool addRenderFuncs(BXSSyscallModule *sm)
{
   if (sm == NULL)
      return(false);

#ifndef BUILD_FINAL
   XS_SYSCALL("reloadTextures", BXSVariableEntry::cVoidVariable, &reloadTextures, 0)
   XS_HELP("reloadTextures(): Reloads all textures managed by BD3DTextureManager")
   
   XS_SYSCALL("unloadTextures", BXSVariableEntry::cVoidVariable, &unloadTextures, 0)
   XS_HELP("unloadTextures(): Unloads all D3D texture manager textures")
   
   XS_SYSCALL("loadTextures", BXSVariableEntry::cVoidVariable, &loadTextures, 0)
   XS_HELP("loadTextures(): Loads all D3D texture manager textures")
            
   XS_SYSCALL("terrainEnableLOD", BXSVariableEntry::cVoidVariable, &terrainEnableLOD, 0)
      XS_BOOL_PARM(true)
      XS_HELP("terrainEnableLOD(true/false) : turns terrain LOD on/off")

  XS_SYSCALL("terrainEnableTextureLOD", BXSVariableEntry::cVoidVariable, &terrainEnableTextureLOD, 0)
      XS_BOOL_PARM(true)
      XS_HELP("terrainEnableTextureLOD(true/false) : turns terrain texturing LOD on/off")      

   XS_SYSCALL("terrainEnableRefinement", BXSVariableEntry::cVoidVariable, &terrainEnableRefinement, 0)
      XS_BOOL_PARM(true)
      XS_HELP("terrainEnableRefinement(true/false): if LOD is off, turns refinement on/off")

   XS_SYSCALL("terrainDrawBBs", BXSVariableEntry::cVoidVariable, &terrainEnableBBRender, 0)
   XS_BOOL_PARM(true)
   XS_HELP("terrainDrawBBs(true/false): turns drawing of terrain bounding boxes on/off")

   XS_SYSCALL("terrainDrawTextures", BXSVariableEntry::cVoidVariable, &terrainEnableTexturing, 0)
   XS_BOOL_PARM(true)
   XS_HELP("terrainDrawTextures(true/false): Turns terrain texturing on/off")
   
   XS_SYSCALL("terrainVisualizeMips", BXSVariableEntry::cVoidVariable, &terrainEnableMipView, 0)
   XS_BOOL_PARM(true)
   XS_HELP("terrainVisualizeMips(true/false): Turns visualization of mip levels on/off")

   XS_SYSCALL("terrainVisualizeCache", BXSVariableEntry::cVoidVariable, &terrainEnableCacheView, 0)
   XS_BOOL_PARM(true)
   XS_HELP("terrainVisualizeCache(true/false): Turns visualization of cache levels on/off")

   XS_SYSCALL("terrainDrawRoads", BXSVariableEntry::cVoidVariable, &terrainEnableRoads, 0)
   XS_BOOL_PARM(true)
   XS_HELP("terrainDrawRoads(true/false): Turns rendering of roads on/off")

   XS_SYSCALL("terrainDrawFoliage", BXSVariableEntry::cVoidVariable, &terrainEnableFoliage, 0)
   XS_BOOL_PARM(true)
   XS_HELP("terrainDrawRoads(true/false): Turns rendering of foliage on/off")

   XS_SYSCALL("terrainToggleDCBs", BXSVariableEntry::cVoidVariable, &terrainToggleDCBs, 0)
   XS_BOOL_PARM(true)
   XS_HELP("terrainToggleDCBs(): Toggles the usage of terrain DCB rendering")

   XS_SYSCALL("playVideo", BXSVariableEntry::cVoidVariable, &playBinkMovie, 0)
   XS_STRING_PARM("")
   XS_HELP("playVideo(\"name\"): plays a bink video")      

   XS_SYSCALL("setExploredBrightness", BXSVariableEntry::cVoidVariable, &setExploredBrightness, 0)
   XS_FLOAT_PARM(0.4f)
   XS_HELP("setExploredBrightness([0,1] value): sets the FoW explored brightness value")
   
   XS_SYSCALL("setUnexploredBrightness", BXSVariableEntry::cVoidVariable, &setUnexploredBrightness, 0)
   XS_FLOAT_PARM(0.0045f)
   XS_HELP("setUnexploredBrightness([0,1] value): sets the FoW unexplored brightness value")
   
   XS_SYSCALL("setEdgeOfWorldTransition", BXSVariableEntry::cVoidVariable, &setEdgeOfWorldTransition, 0)
   XS_FLOAT_PARM(19.0f)
   XS_HELP("setEdgeOfWorldTransition([0,1] value): sets the FoW edge of world transition distance")

#endif   
   
   return true;
}
