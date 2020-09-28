// File: renderEventClasses.h
#pragma once

enum eRenderEventClass
{
   cRenderEventClassTextureStatusChanged = cEventClassFirstUser + 500,
   
   cRenderEventClassTextureAddAllocation,
   cRenderEventClassTextureRemoveAllocation, 
      
   cRenderEventClassTextureManager,
   cRenderEventClassTextureManagerSetDirID,
   cRenderEventClassTextureManagerLoadAll,
   cRenderEventClassTextureManagerUnloadAll,
   cRenderEventClassTextureManagerDestroyAll,
   cRenderEventClassTextureManagerReloadAll,
   cRenderEventClassTextureManagerReload,
   cRenderEventClassTextureManagerReloadManager,
      
   cRenderEventClassEffectCompileRequest,
   cRenderEventClassEffectCompileResults,

   // FIXME: Move this crap to xgamerender
   cRenderEventClassUGXGeomInit,
   cRenderEventClassUGXGeomStatusChanged,
   cRenderEventClassUGXGeomRenderInfo,
   
   cRenderEventClassMax,

   cRECForceDWORD = 0xFFFFFFFF
};
