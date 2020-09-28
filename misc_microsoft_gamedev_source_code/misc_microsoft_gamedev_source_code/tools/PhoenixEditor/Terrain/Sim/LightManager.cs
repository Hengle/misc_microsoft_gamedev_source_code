using System;
using System.Collections.Generic;
using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;
using SimEditor;
using EditorCore;
using Rendering;

namespace Terrain
{

   class lightInstance
   {
      ~lightInstance()
      {
         mSimEditorObject = null;
         mWorldChunksImAffecting.Clear();
      }
      public SimEditor.LocalLight mSimEditorObject = null;
      public List<worldChunk> mWorldChunksImAffecting = new List<worldChunk>();
   }
   class worldChunk
   {
      ~worldChunk()
      {
         mLightsAffectingMe.Clear();
         mQNPointer = null;
      }
      public List<lightInstance> mLightsAffectingMe = new List<lightInstance>();
      public BTerrainQuadNode mQNPointer = null;
   }

   class LightManager
   {
      static private List<lightInstance> mLights = new List<lightInstance>();
      static private worldChunk[,] mWorldChunks = null;
      
      static public void init(int numXChunks, int numZChunks)
      {
         //initalize our chunk pointers to the main QNs
         mWorldChunks = new worldChunk[numXChunks, numZChunks];
         for (int x = 0; x < mWorldChunks.GetLength(0); x++)
         {
            for (int y = 0; y < mWorldChunks.GetLength(1); y++)
            {
               int xPt = (int)(x * BTerrainQuadNode.cMaxWidth + 1);
               int yPt = (int)(y * BTerrainQuadNode.cMaxHeight + 1);
               mWorldChunks[x, y] = new worldChunk();
               mWorldChunks[x, y].mQNPointer = TerrainGlobals.getTerrain().getQuadNodeRoot().getLeafNodeContainingPoint(xPt, yPt);
            }
         }
      }
      static public void destroy()
      {
         mLights.Clear();
         mWorldChunks = null;
      }


      //utils
      static public int giveLightIndex(int editorObjectIndex)
      {
         SimEditor.LocalLight ll = getSimObjectAsLight(editorObjectIndex);
         for (int i = 0; i < mLights.Count; i++)
         {
            if (mLights[i].mSimEditorObject == ll)
               return i;
         }
         return -1;
      }
      static public lightInstance giveLightInstace(int editorObjectIndex)
      {
         int indx = giveLightIndex(editorObjectIndex);
         if(indx ==-1)
            return null;

         return mLights[indx];
      }
      static public SimEditor.LocalLight getSimObjectAsLight(int simObjectIndex)
      {
         return SimGlobals.getSimMain().getEditorObject(simObjectIndex) as LocalLight;
      }
      static public bool hasTerrainLightData()
      {
         if (mLights.Count == 0 || mWorldChunks == null)
            return false;

         return true;
      }

      //list utils
      static private void copyList(List<worldChunk> src, List<worldChunk> dst)
      {
         for(int i=0;i<src.Count;i++)
         {
            dst.Add(src[i]);
         }
      }
      static private void mergeList(List<worldChunk> src, List<worldChunk> dst)
      {
         for (int i = 0; i < src.Count; i++)
         {
            if (!dst.Contains(src[i]))
               dst.Add(src[i]);
         }
      }

      //add a light to our manager
      static public void registerLight(int simObjectIndex)
      {
         int lightIndex = mLights.Count;
         mLights.Add(new lightInstance());
         mLights[lightIndex].mSimEditorObject = getSimObjectAsLight(simObjectIndex);
         addLightToGrid(mLights[lightIndex]);
      }
      //light deleted, remove from our manager
      static public void freeLight(int editorObjectIndex)
      {
         int lightIndex = giveLightIndex(editorObjectIndex);
         if (lightIndex == -1)
            return;
         List<worldChunk> wcList = new List<worldChunk>();
         copyList(mLights[lightIndex].mWorldChunksImAffecting, wcList);

         //remove this light from the world grid
         removeLightFromGrid(mLights[lightIndex]);

         //reload affected qns
         for (int i = 0; i < wcList.Count; i++)
            rasterLightsToNode(wcList[i]);

         TerrainGlobals.getTerrain().rebuildDirtyPostDeform(BRenderDevice.getDevice());

         //remove this light from the main list
         mLights.RemoveAt(lightIndex);

      }

      //world grid management
      static private void addLightToGrid(lightInstance light)
      {
         LocalLight l = light.mSimEditorObject;
         BBoundingBox bb = new BBoundingBox();
         bb.addPoint(l.mAABB.min);
         bb.addPoint(l.mAABB.max);
         bb.min += l.getPosition();
         bb.max += l.getPosition();

         //walk every world chunk, update our bounds, do intersections
         BBoundingBox worldAABB = new BBoundingBox();
         for (int x = 0; x < mWorldChunks.GetLength(0); x++)
         {
            for (int y = 0; y < mWorldChunks.GetLength(1); y++)
            {
               worldAABB.max = mWorldChunks[x, y].mQNPointer.getDesc().m_maxPostDeform;
               worldAABB.min = mWorldChunks[x, y].mQNPointer.getDesc().m_minPostDeform;
               worldAABB.max.Y = 300;
               worldAABB.min.Y = -300;

               if (bb.intersect(worldAABB))
               {
                  mWorldChunks[x, y].mLightsAffectingMe.Add(light);
                  light.mWorldChunksImAffecting.Add(mWorldChunks[x, y]);
               }
            }
         }
      }
      static private void removeLightFromGrid(lightInstance light)
      {
         for (int i = 0; i < light.mWorldChunksImAffecting.Count; i++)
            light.mWorldChunksImAffecting[i].mLightsAffectingMe.Remove(light);

         light.mWorldChunksImAffecting.Clear();
      }

      //CLM Called when a light is moved by the user
      static public void moveLight(int editorObjectIndex)
      {
         int lightIndex = giveLightIndex(editorObjectIndex);
         if (lightIndex == -1)
            return;

         //copy our influence list
         List<worldChunk> wcList = new List<worldChunk>();
         copyList(mLights[lightIndex].mWorldChunksImAffecting, wcList);
         
         //remove this light from the world grid
         removeLightFromGrid(mLights[lightIndex]);

         //add this light to the world grid
         addLightToGrid(mLights[lightIndex]);

         //merge the two lists
         mergeList(mLights[lightIndex].mWorldChunksImAffecting, wcList);

         //reload affected qns
         for(int i=0;i<wcList.Count;i++)
            rasterLightsToNode(wcList[i]);
         
         TerrainGlobals.getTerrain().rebuildDirtyPostDeform(BRenderDevice.getDevice());
      }

      //CLM called when a light's properties have been changed
      static public void lightChangedMinor(int editorObjectIndex)
      {
         int lightIndex = giveLightIndex(editorObjectIndex);

         //reload all of the chunks that this light affected
         for(int i=0;i<mLights[lightIndex].mWorldChunksImAffecting.Count;i++)
            rasterLightsToNode(mLights[lightIndex].mWorldChunksImAffecting[i]);
         

         TerrainGlobals.getTerrain().rebuildDirtyPostDeform(BRenderDevice.getDevice());
      }
      //CLM called when a light has changed radius or any other visual rep value
      static public void lightChangedMajor(int editorObjectIndex)
      {
         moveLight(editorObjectIndex);
      }

      static public void rasterLightsToNode(BTerrainQuadNode node)
      {
         worldChunk wc = null;
         for (int x = 0; x < mWorldChunks.GetLength(0); x++)
         {
            for (int y = 0; y < mWorldChunks.GetLength(1); y++)
            {
               if (mWorldChunks[x, y].mQNPointer == node)
               {
                  wc = mWorldChunks[x, y];
                  break;
               }
            }
            if (wc != null)
               break;
         }

         if (wc == null)
            return;

         rasterLightsToNode(wc);
      }
      static public void rasterLightsToNode(worldChunk wc)
      {
         if (!TerrainGlobals.getEditor().doShowLighting())
            return;

         clearLightInfluence(wc.mQNPointer.getDesc().mMinXVert, wc.mQNPointer.getDesc().mMinZVert);
         wc.mQNPointer.mDirty = true;
         wc.mQNPointer.clearVisibleDatHandle();
         for(int i=0;i<wc.mLightsAffectingMe.Count;i++)
         {
            SimEditor.LocalLight ll = wc.mLightsAffectingMe[i].mSimEditorObject;
            rasterLightToGrid(ll, TerrainGlobals.getEditor().getLightValues(),wc.mQNPointer.getDesc().mMinXVert, wc.mQNPointer.getDesc().mMinZVert);
         }

      }
      static private void clearLightInfluence(int minXVert, int minZVert)
      {
         Vector3[] lht = TerrainGlobals.getEditor().getLightValues();
         for (int x = 0; x < BTerrainQuadNode.cMaxWidth; x++)
         {
            for (int y = 0; y < BTerrainQuadNode.cMaxHeight; y++)
            {
               int index = (minXVert + x) * TerrainGlobals.getTerrain().getNumXVerts() + (minZVert + y);

               lht[index].X = 0;
               lht[index].Y = 0;
               lht[index].Z = 0;
            }
         }
      }

      static private void rasterLightToGrid(SimEditor.LocalLight light,Vector3[]lht, int minXVert, int minZVert)
      {
         //CLM this code is a duplicate of what's on the 360!
         //ANY CHANGES TO THE 360 CODE MUST BE DUPLICATED HERE!

         Vector3 lightColor = new Vector3(light.LightData.LightColor.R / 255.0f,
                                          light.LightData.LightColor.G / 255.0f,
                                          light.LightData.LightColor.B / 255.0f);

         ////terrain fill, don't let the ligths go above 2.0;
        //convert to linear and multiply by intensity
         //srgb*srgb *intensity
         lightColor = BMathLib.Vec3Mul(lightColor,lightColor) * light.LightData.Intensity;

         float farAttenStart = Math.Min(0.999f, light.LightData.FarAttnStart);

         float omniOOFalloffRange = 1.0f / (1.0f - farAttenStart);
         float omniMul = -((1.0f / light.LightData.Radius) * omniOOFalloffRange);
         float omniAdd = 1.0f + farAttenStart * omniOOFalloffRange;

         float spotMul = 0.0f;
         float spotAdd = 100.0f;
         Vector3 spotDir = new Vector3(0, 0, 0);
         if (light is SimEditor.SpotLight)
         {
            float fovFudge = 0;
            spotDir = Vector3.Normalize((light as SimEditor.SpotLight).mDirection);
            float spotInner = Math.Max(Geometry.DegreeToRadian(.0125f), Geometry.DegreeToRadian(light.LightData.InnerAngle) - Geometry.DegreeToRadian(fovFudge));
            float spotOuter = Math.Max(Geometry.DegreeToRadian(.025f), Geometry.DegreeToRadian(light.LightData.OuterAngle) - Geometry.DegreeToRadian(fovFudge));
            spotMul = (float)(1.0f / ((Math.Cos(spotInner * .5f) - Math.Cos(spotOuter * .5f))));
            spotAdd = (float)(1.0f - Math.Cos(spotInner * .5f) * spotMul);

         }

         //for each vert in this chunk.. 
         for(int x=0;x<BTerrainQuadNode.cMaxWidth;x++)
         {
            for (int y = 0; y < BTerrainQuadNode.cMaxHeight; y++)
            {
               Vector3 wpos = TerrainGlobals.getTerrain().getPostDeformPos(x + minXVert, y + minZVert);
               Vector3 normal = TerrainGlobals.getTerrain().getPostDeformNormal(x + minXVert, y + minZVert);

               Vector3 lightVec = light.getPosition() - wpos;

               float dist = lightVec.Length();
               float ooLightDist = BMathLib.Clamp((float)(1.0f / dist), 0, 1);
               Vector3 lightNorm = BMathLib.Normalize(lightVec);

               float spotAngle = -BMathLib.Dot(ref spotDir, ref lightNorm);

               float lDot = BMathLib.Clamp(BMathLib.Dot(ref lightNorm, ref normal), 0, 1);
               Vector3 atten;
               atten.X = lDot;
               atten.Y = BMathLib.Clamp(spotAngle * spotMul + spotAdd, 0, 1);
               atten.Z = BMathLib.Clamp(dist * omniMul + omniAdd, 0, 1);

               atten.X = atten.X * atten.X * (-2.0f * atten.X + 3.0f);
               atten.Y = atten.Y * atten.Y * (-2.0f * atten.Y + 3.0f);
               atten.Z = atten.Z * atten.Z * (-2.0f * atten.Z + 3.0f);

               atten.X *= atten.Z;
               atten.X *= BMathLib.Clamp(light.LightData.DecayDist * ooLightDist, 0, 1);
               atten.X *= atten.Y;

               int index = (minXVert + x) * TerrainGlobals.getTerrain().getNumXVerts() + (minZVert + y);

               lht[index].X = (atten.X * lightColor.Z + lht[index].X);
               lht[index].Y = (atten.X * lightColor.Y + lht[index].Y);
               lht[index].Z = (atten.X * lightColor.X + lht[index].Z);
            }
         }
      }

      //CLM called during normal export
      static public void rasterTerrainLightsToExportGrid(Vector3 []lht)
      {
         //walk each world grid, raster the lights that are terrain only to the grid
         worldChunk wc = null;
         for (int x = 0; x < mWorldChunks.GetLength(0); x++)
         {
            for (int y = 0; y < mWorldChunks.GetLength(1); y++)
            {
               for(int i=0;i<mWorldChunks[x,y].mLightsAffectingMe.Count;i++)
               {
                  SimEditor.LocalLight ll = mWorldChunks[x,y].mLightsAffectingMe[i].mSimEditorObject;
                  if(ll.LightData.TerrainOnly==true)
                  {
                     rasterLightToGrid(ll, lht, mWorldChunks[x, y].mQNPointer.getDesc().mMinXVert, mWorldChunks[x, y].mQNPointer.getDesc().mMinZVert);
                  }
               }
            }
         }
      }
   }
}