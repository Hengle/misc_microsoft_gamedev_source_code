using System;
using System.IO;
using System.Runtime.InteropServices;
using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;
using System.Windows.Forms;
using System.Collections.Generic;
using System.Text;
using System.Diagnostics;
using System.ComponentModel;

using EditorCore;
using Rendering;
using Terrain;


/*
 * CLM 03.26.07
 * This class is responsible for splitting and exportation of the road data into pre-cache chunks.
 */
namespace Export360
{
   public class XTT_RoadExport
   {
      ~XTT_RoadExport()
      {
      }

      public class RoadVert
      {
         public Vector3 mPos;
         public Vector2 mUv0;
      }
      public class RoadTriangle
      {
         public RoadVert []mVerts = new RoadVert[3];
        
         public void ensureWinding()
         {
            //WANT CW WINDING!!!
            //Vector3 ray01 = BMathLib.Normalize(mVerts[1].mPos - mVerts[0].mPos);
            //Vector3 ray02 = BMathLib.Normalize(mVerts[2].mPos - mVerts[0].mPos);
            //Vector3 nrm = Vector3.Cross(ray01, ray02);
            //if (Vector3.Dot(nrm, BMathLib.unitY) < 0)
            //{
            //   RoadVert rv = new RoadVert();
            //   rv.mPos = mVerts[1].mPos;
            //   rv.mUv0 = mVerts[1].mUv0;

            //   mVerts[1].mPos = mVerts[2].mPos;
            //   mVerts[1].mUv0 = mVerts[2].mUv0;

            //   mVerts[2].mPos = rv.mPos;
            //   mVerts[2].mUv0 = rv.mUv0;
            //}
         }
      }
      class RoadQN
      {
         public int mOwnerQNIndex=0;
         public List<RoadTriangle> mTris = new List<RoadTriangle>();
      }

      RoadVert lerpVerts(RoadVert a, RoadVert b, float alpha)
      {
         RoadVert rv = new RoadVert();

         rv.mPos = ((a.mPos) + (((b.mPos) - (a.mPos)) * (alpha)));
         rv.mUv0 = ((a.mUv0) + (((b.mUv0) - (a.mUv0)) * (alpha)));
         return rv;
      }


      void clipPolygon(RoadTriangle roadTri, Plane plane, List<RoadVert> posResults, List<RoadVert> negResults)
      {
         float[] dist = new float[3];
         dist[0] = BMathLib.pointPlaneDistance(roadTri.mVerts[0].mPos, plane);
         dist[1] = BMathLib.pointPlaneDistance(roadTri.mVerts[1].mPos, plane);
         dist[2] = BMathLib.pointPlaneDistance(roadTri.mVerts[2].mPos, plane);

         List<RoadVert> results = new List<RoadVert>();
         results.Add(roadTri.mVerts[0]);
         results.Add(roadTri.mVerts[1]);
         results.Add(roadTri.mVerts[2]);

         //CLM This function ASSUMES that the poly is SPANNING the plane (prior tests determine this!)

         Vector3 ray = Vector3.Empty;
         Vector3 orig = Vector3.Empty;

         float tVal = 0;
         orig = roadTri.mVerts[0].mPos;
         ray = BMathLib.Normalize(roadTri.mVerts[1].mPos - roadTri.mVerts[0].mPos);
         if (BMathLib.rayPlaneIntersect(plane, orig, ray, false, ref tVal))
         {
            float tdist = (float)(tVal / Vector3.Length(roadTri.mVerts[1].mPos - roadTri.mVerts[0].mPos));
            if (tdist <= 1)
               results.Add(lerpVerts(roadTri.mVerts[0], roadTri.mVerts[1], tdist));
         }

         ray = BMathLib.Normalize(roadTri.mVerts[2].mPos - roadTri.mVerts[0].mPos);
         if (BMathLib.rayPlaneIntersect(plane, orig, ray, false, ref tVal))
         {
            float tdist = (float)(tVal / Vector3.Length(roadTri.mVerts[2].mPos - roadTri.mVerts[0].mPos));
            if (tdist <= 1)
               results.Add(lerpVerts(roadTri.mVerts[0], roadTri.mVerts[2], tdist));
         }

         orig = roadTri.mVerts[1].mPos;
         ray = BMathLib.Normalize(roadTri.mVerts[2].mPos - roadTri.mVerts[1].mPos);
         if (BMathLib.rayPlaneIntersect(plane, orig, ray, false, ref tVal))
         {
            float tdist = (float)(tVal / Vector3.Length(roadTri.mVerts[2].mPos - roadTri.mVerts[1].mPos));
            if(tdist<=1)
               results.Add(lerpVerts(roadTri.mVerts[1], roadTri.mVerts[2], tdist));
         }

         for (int i = 0; i < results.Count; i++)
         {
            for (int c = i + 1; c < results.Count; c++)
            {
               if (results[i] == results[c])
               {
                  results.RemoveAt(c);
               }
            }

            float tdist = BMathLib.pointPlaneDistance(results[i].mPos, plane);
            if (tdist > 0)
            {
               posResults.Add(results[i]);
            }
            else if (tdist < 0)
            {
               negResults.Add(results[i]);
            }
            else //tdist==0 (spanning vert)
            {
               posResults.Add(results[i]);
               negResults.Add(results[i]);
            }
           
         }
         results.Clear();
      }
      bool TriSpansPlane(RoadTriangle tri, Plane pl)
      {
        
         float[] dist = new float[3];
         dist[0] = BMathLib.pointPlaneDistance(tri.mVerts[0].mPos, pl);
         dist[1] = BMathLib.pointPlaneDistance(tri.mVerts[1].mPos, pl);
         dist[2] = BMathLib.pointPlaneDistance(tri.mVerts[2].mPos, pl);

         if (
            (dist[0] == 0 && dist[1] == 0) ||  //2 pts coplanar
            (dist[0] == 0 && dist[2] == 0) ||  //2 pts coplanar
            (dist[2] == 0 && dist[1] == 0) ||  //2 pts coplanar
            (dist[0] == 0 && dist[1] < 0 && dist[2] < 0) || //1 point coplanar, 
            (dist[0] == 0 && dist[1] > 0 && dist[2] > 0) || //1 point coplanar, 
            (dist[1] == 0 && dist[2] < 0 && dist[0] < 0) || //1 point coplanar, 
            (dist[1] == 0 && dist[2] > 0 && dist[0] > 0) || //1 point coplanar, 
            (dist[2] == 0 && dist[0] < 0 && dist[1] < 0) || //1 point coplanar, 
            (dist[2] == 0 && dist[0] > 0 && dist[1] > 0) || //1 point coplanar, 
            (dist[0] <= 0 && dist[1] <= 0 && dist[2] <= 0) ||    //3 points on one side
            (dist[0] >= 0 && dist[1] >= 0 && dist[2] >= 0)     //3 points on one side
            )
            return false;

         return true;
      }
      void splitPolyListAgainstPlane(Plane pl, List<RoadTriangle> triList)
      {
         for (int triIndex = 0; triIndex < triList.Count; triIndex++)
         {
            if (TriSpansPlane(triList[triIndex], pl))
            {
               
               List<RoadVert> posResults = new List<RoadVert>();
               List<RoadVert> negResults = new List<RoadVert>();

               //split the poly, add results from both sides to end of list
               clipPolygon(triList[triIndex], pl, posResults, negResults);

               if(posResults.Count ==3)
               {
                  RoadTriangle rt = new RoadTriangle();
                  rt.mVerts[0] = posResults[0];
                  rt.mVerts[1] = posResults[1];
                  rt.mVerts[2] = posResults[2];

                  triList.Add(rt);
               }
               else if (posResults.Count == 4)
               {

                  //CLM we assume that clipPolygon returns 
                  //the origional tri points first in the list
                  RoadTriangle rt0 = new RoadTriangle();
                  rt0.mVerts[0] = posResults[0];
                  rt0.mVerts[1] = posResults[1];
                  rt0.mVerts[2] = posResults[2];

                  triList.Add(rt0);


                  RoadTriangle rt1 = new RoadTriangle();
                  rt1.mVerts[0] = posResults[1];
                  rt1.mVerts[1] = posResults[2];
                  rt1.mVerts[2] = posResults[3];

                  triList.Add(rt1);
               }
               if (negResults.Count == 3)
               {
                  RoadTriangle rt = new RoadTriangle();
                  rt.mVerts[0] = negResults[0];
                  rt.mVerts[1] = negResults[1];
                  rt.mVerts[2] = negResults[2];
                  triList.Add(rt);
               }
               else if (negResults.Count == 4)
               {

                  //CLM we assume that clipPolygon returns 
                  //the origional tri points first in the list
                  RoadTriangle rt0 = new RoadTriangle();
                  rt0.mVerts[0] = negResults[0];
                  rt0.mVerts[1] = negResults[1];
                  rt0.mVerts[2] = negResults[2];

                  triList.Add(rt0);


                  RoadTriangle rt1 = new RoadTriangle();
                  rt1.mVerts[0] = negResults[1];
                  rt1.mVerts[1] = negResults[2];
                  rt1.mVerts[2] = negResults[3];

                  triList.Add(rt1);
               }

               triList.RemoveAt(triIndex);
               triIndex--;

               posResults.Clear();
               negResults.Clear();
            }
         }
      }

      bool triContainedInBox(RoadTriangle tri, float minx, float minz, float maxx, float maxz)
      {
         if(tri.mVerts[0].mPos.X >= minx && tri.mVerts[0].mPos.X <= maxx && tri.mVerts[0].mPos.Z >= minz && tri.mVerts[0].mPos.Z <= maxz && 
            tri.mVerts[1].mPos.X >= minx && tri.mVerts[1].mPos.X <= maxx && tri.mVerts[1].mPos.Z >= minz && tri.mVerts[1].mPos.Z <= maxz &&
            tri.mVerts[2].mPos.X >= minx && tri.mVerts[2].mPos.X <= maxx && tri.mVerts[2].mPos.Z >= minz && tri.mVerts[2].mPos.Z <= maxz)
            return true;

         return false;
      }

      
      public void exportRoads(ref ExportResults results)
      {
         DateTime n = DateTime.Now;

         //for each road, 
         for(int roadIndex=0;roadIndex < RoadManager.giveNumRoads();roadIndex++)
         {
            List<RoadTriangle> triList = new List<RoadTriangle>();
            Road rd = RoadManager.giveRoad(roadIndex);

            if (rd.getNumControlPoints() == 0)  //don't export roads w/o points on the map
               continue;

            //Step 1, generate polygon lists////////////////////////////////////////////////////
            for(int rcpIndex=0;rcpIndex<rd.getNumControlPoints();rcpIndex++)
            {
               roadControlPoint rcp = rd.getPoint(rcpIndex);
               int numTris = (int)(rcp.mVerts.Count / 3);
               for(int triIndex=0;triIndex<numTris;triIndex++)
               {
                  RoadTriangle rt = new RoadTriangle();
                  rt.mVerts[0] = new RoadVert();
                  rt.mVerts[0].mPos = new Vector3( rcp.mVerts[triIndex * 3 + 0].x,rcp.mVerts[triIndex * 3 + 0].y,rcp.mVerts[triIndex * 3 + 0].z);
                  rt.mVerts[0].mUv0 = new Vector2( rcp.mVerts[triIndex * 3 + 0].u0,rcp.mVerts[triIndex * 3 + 0].v0);

                  rt.mVerts[1] = new RoadVert();
                  rt.mVerts[1].mPos = new Vector3(rcp.mVerts[triIndex * 3 + 1].x, rcp.mVerts[triIndex * 3 + 1].y, rcp.mVerts[triIndex * 3 + 1].z);
                  rt.mVerts[1].mUv0 = new Vector2(rcp.mVerts[triIndex * 3 + 1].u0, rcp.mVerts[triIndex * 3 + 1].v0);

                  rt.mVerts[2] = new RoadVert();
                  rt.mVerts[2].mPos = new Vector3(rcp.mVerts[triIndex * 3 + 2].x, rcp.mVerts[triIndex * 3 + 2].y, rcp.mVerts[triIndex * 3 + 2].z);
                  rt.mVerts[2].mUv0 = new Vector2(rcp.mVerts[triIndex * 3 + 2].u0, rcp.mVerts[triIndex * 3 + 2].v0);


                  triList.Add(rt);
               }
            }

            //now add our triStrip segments (into triLists)
            for(int segIndex=0;segIndex<rd.getNumRoadSegments();segIndex++)
            {
               roadSegment rs = rd.getRoadSegment(segIndex);
               int numTris = rs.mNumVerts - 2;
               int vertIndex = rs.mStartVertInParentSegment + 2;
               for(int i=0;i<numTris;i++)
               {
                  RoadTriangle rt = new RoadTriangle();
                  rt.mVerts[0] = new RoadVert();
                  rt.mVerts[0].mPos = new Vector3(rd.mVerts[vertIndex - 2].x, rd.mVerts[vertIndex - 2].y, rd.mVerts[vertIndex - 2].z);
                  rt.mVerts[0].mUv0 = new Vector2(rd.mVerts[vertIndex - 2].u0, rd.mVerts[vertIndex - 2].v0);

                  rt.mVerts[1] = new RoadVert();
                  rt.mVerts[1].mPos = new Vector3(rd.mVerts[vertIndex - 1].x, rd.mVerts[vertIndex - 1].y, rd.mVerts[vertIndex - 1].z);
                  rt.mVerts[1].mUv0 = new Vector2(rd.mVerts[vertIndex - 1].u0, rd.mVerts[vertIndex - 1].v0);

                  rt.mVerts[2] = new RoadVert();
                  rt.mVerts[2].mPos = new Vector3(rd.mVerts[vertIndex].x, rd.mVerts[vertIndex].y, rd.mVerts[vertIndex].z);
                  rt.mVerts[2].mUv0 = new Vector2(rd.mVerts[vertIndex].u0, rd.mVerts[vertIndex].v0);

                 

                  triList.Add(rt);
                  vertIndex++;
               }
            }


            //Step 2, split Polygons////////////////////////////////////////////////////
            int width = (int)(BTerrainQuadNode.cMaxWidth);
            int numPlanes = (int)(TerrainGlobals.getTerrain().getNumXVerts() / width);

            //Split along XPlane
            for(int planeIndex =0;planeIndex<numPlanes;planeIndex++)
            {
               Plane pl = Plane.FromPointNormal(new Vector3(planeIndex*width*TerrainGlobals.getTerrain().getTileScale(),0,0),BMathLib.unitX);
               splitPolyListAgainstPlane(pl, triList);
            }

            //split along ZPlane
            for(int planeIndex =0;planeIndex<numPlanes;planeIndex++)
            {
               Plane pl = Plane.FromPointNormal(new Vector3(0, 0, planeIndex * width * TerrainGlobals.getTerrain().getTileScale()), BMathLib.unitZ);
               splitPolyListAgainstPlane(pl, triList);
            }

            //Step 3, add polies to qn./////////////////////////////////////////////////
            List<RoadQN> roadQNs = new List<RoadQN>();
            int numQNs = (int)(TerrainGlobals.getTerrain().getNumXVerts() / width);
            int qnWidth = (int)(width * TerrainGlobals.getTerrain().getTileScale());
            for (int qnX = 0; qnX < numQNs; qnX++)
            {
               for (int qnZ = 0; qnZ < numQNs; qnZ++)
               {
                  float x = qnX * qnWidth;
                  float z = qnZ * qnWidth;
                  RoadQN rqn = new RoadQN();

                  for (int triIndex = 0; triIndex < triList.Count; triIndex++)
                  {
                     if(triContainedInBox(triList[triIndex],x, z, x + qnWidth, z + qnWidth))
                     {
                        triList[triIndex].ensureWinding();
                        rqn.mTris.Add(triList[triIndex]);
                        rqn.mOwnerQNIndex = qnX * numQNs + qnZ;
                     }
                  }

                  if(rqn.mTris.Count!=0)
                     roadQNs.Add(rqn);
                  
               }
            }

            //Step 4, write road chunk to disk./////////////////////////////////////////////
            ECF.ECFChunkHolder chunkHolder = new ECF.ECFChunkHolder();
            chunkHolder.mDataMemStream = new MemoryStream();
            BinaryWriter binWriter = new BinaryWriter(chunkHolder.mDataMemStream);

            //Filename
            string fName = CoreGlobals.getWorkPaths().mRoadsPath + "\\" + rd.getRoadTextureName();
            fName = fName.Remove(0, CoreGlobals.getWorkPaths().mGameArtDirectory.Length + 1);
            char[] filename = new char[32];
            fName.CopyTo(0, filename, 0, fName.Length);
            binWriter.Write(filename);

            ExportTo360.addTextureChannelDependencies(CoreGlobals.getWorkPaths().mRoadsPath + "\\" + rd.getRoadTextureName());

            float zero = 0;
            int totalMemory = 0;
            //write our chunks
            binWriter.Write(Xbox_EndianSwap.endSwapI32(roadQNs.Count));
            for (int qnI = 0; qnI < roadQNs.Count;qnI++ )
            {
               binWriter.Write(Xbox_EndianSwap.endSwapI32(roadQNs[qnI].mOwnerQNIndex));
               binWriter.Write(Xbox_EndianSwap.endSwapI32(roadQNs[qnI].mTris.Count));
               int memSize = roadQNs[qnI].mTris.Count * (3 * (sizeof(short) * 6));
               binWriter.Write(Xbox_EndianSwap.endSwapI32(memSize));

               List<float> vList = new List<float>();
               for (int c = 0; c < roadQNs[qnI].mTris.Count; c++)
               {
                  for (int i = 0; i < 3; i++)
                  {
                     vList.Add(roadQNs[qnI].mTris[c].mVerts[i].mPos.X);
                     vList.Add(roadQNs[qnI].mTris[c].mVerts[i].mPos.Y);
                     vList.Add(roadQNs[qnI].mTris[c].mVerts[i].mPos.Z);
                     vList.Add(zero);  //padd;
                     vList.Add(roadQNs[qnI].mTris[c].mVerts[i].mUv0.X);
                     vList.Add(roadQNs[qnI].mTris[c].mVerts[i].mUv0.Y);
                  }
               }
               float16[] sList = BMathLib.Float32To16Array(vList.ToArray());
               for (int c = 0; c < vList.Count; c++)
               {
                  ushort s = ((ushort)sList[c].getInternalDat());
                  binWriter.Write(Xbox_EndianSwap.endSwapI16(s));
               }
               totalMemory += memSize;

            }

            ExportTo360.mECF.addChunk((int)eXTT_ChunkID.cXTT_RoadsChunk, chunkHolder, binWriter.BaseStream.Length);
            binWriter.Close();
            binWriter = null;
            chunkHolder.Close();
            chunkHolder = null;

            roadQNs.Clear();
            triList.Clear();

            TimeSpan ts = DateTime.Now - n;
            results.terrainRoadTime = ts.TotalMinutes;
            results.terrainRoadMemory = totalMemory;
         }
      }
   }
}