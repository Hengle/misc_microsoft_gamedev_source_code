using System;
using System.Drawing;
using System.Collections.Generic;
using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;
using System.Runtime.InteropServices;
using System.Text;
using System.Windows.Forms;
using System.IO;
using Rendering;

namespace ModelSystem
{

   class grannyElement
   {
      public BRenderGrannyMesh mesh;
      public string            filename;
   }

   public class GrannyManager
   {
      public static BRenderGrannyMesh getOrLoadGR2(string filename)
      {
         return getOrLoadGR2(filename, false);
      }
      public static BRenderGrannyMesh getOrLoadGR2(string filename, bool forceReload)
      {
         GC.AddMemoryPressure(1000000);
         try
         {

            for (int i = 0; i < mMeshes.Count; i++)
            {
               if (mMeshes[i].filename.Equals(filename))
               {
                  if (forceReload)
                  {
                     mMeshes.RemoveAt(i);
                     break;
                  }
                  else
                  {
                     return mMeshes[i].mesh;
                  }
               }
            }

            grannyElement ele = new grannyElement();
            ele.mesh = GrannyBridge.LoadGR2(filename);
            ele.filename = filename;
            mMeshes.Add(ele);
         }
         finally
         {
            GC.RemoveMemoryPressure(1000000);
         }
         return mMeshes[mMeshes.Count - 1].mesh;
      }

      public static void clearGR2Cache()
      {
         for(int i=0;i<mMeshes.Count;i++)
         {
            mMeshes[i].mesh = null;
            mMeshes[i] = null;
         }
         mMeshes.Clear();
      }

      private static List<grannyElement> mMeshes = new List<grannyElement>();
   }
}