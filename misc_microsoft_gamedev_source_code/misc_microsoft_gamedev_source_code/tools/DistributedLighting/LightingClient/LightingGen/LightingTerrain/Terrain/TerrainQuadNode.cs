using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;
using System.Collections.Generic;
using System;

using EditorCore;
using Rendering;
//-----------------------------------------------------
namespace LightingClient
{
   //-----------------------------------------------------
   public struct BTerrainQuadNodeDesc
   {
      public int mMinXVert;
      public int mMinZVert;
      

      public static uint cMaxWidth = 64;
      public static uint cMaxHeight = 64;

   };
}