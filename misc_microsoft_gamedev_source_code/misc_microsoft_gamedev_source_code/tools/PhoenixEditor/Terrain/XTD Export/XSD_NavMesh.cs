using System;
using System.IO;
using System.Drawing;
using System.Collections.Generic;
using System.Text;
using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;
using Microsoft.DirectX.DirectInput;
using System.Windows.Forms;
using System.Threading;
using System.Xml;
using System.Xml.Serialization;
using System.Runtime.InteropServices;

using EditorCore;
using Rendering;
using SimEditor;

//----------------------------------
namespace Terrain
{
   public class XSD_NavMesh
   {

      #region OffsetENUMS
      //int[] mSimObjEdgeDefn = new int[] { 0x03F, 0x1F8,     //top , bottom
      //                                    0x0DB, 0x1B6,     //left , right
      //                                    0x05F, 0x137,     //topleft , topright
      //                                    0x1D9, 0x1F4,    //bottomleft, bottomright
      //                                    0x1FF,            //internal (surrounded)  
      //                                    0x010,            //island

      //                                    };
      //int[] mSimObjCornerDefn = new int[]{0x0FF, 0x1BF,     //inner corners
      //                                    0x1FB, 0x1FE,

      //                                    0x01B, 0x036,     //outer corners
      //                                    0x1B0, 0x0D8,

      //                                    0x037, 0x01F,     //complex outer corner
      //                                    0x1F0, 0x1D8,
      //                                    0x1B4, 0x0D9,
      //                                    0x136, 0x05B
      //                                     };

      int[] mVertNeighborLines = new int[] { 0x111, 0x092, 0x054, 0x038, };

      int[] mSimTileCornerDefn4 = new int[] { 0x01, 0x02, 0x04, 0x08,
                                             //0x0E, 0x0D, 0x07, 0x0B
                                             };

      int[] mSimTileCornerDefn16 = new int[] { 0x00CC, 0x0033, 0xCC00, 0x3300,

                                               0x0133, 0x08CC, 0xCC80, 0x3310,
                                               0x0037, 0x00CE, 0x7300, 0xEC00,
                     
                                               0x003F, 0x00CF, 0xF300, 0xFC00,
                                               0x1133, 0x88CC, 0x3311, 0xCC88
                                             };
      Point[] offsets = new Point[]
         {
            new Point(-1,-1),
            new Point( 0,-1),
            new Point( 1,-1),
            new Point(-1, 0),
            new Point( 0, 0),
            new Point(+1, 0),
            new Point(-1, 1),
            new Point( 0, 1),
            new Point( 1, 1),
         };

      Point[] tileOffsets4 = new Point[]
         {
            new Point( -1, -1),
            new Point(  0, -1),
            new Point( -1,  0),
            new Point(  0,  0),
         };
      Point[] tileOffsets16 = new Point[]
         {
            new Point(-2, 1),
            new Point(-1, 1),
            new Point( 0, 1),
            new Point( 1, 1),

            new Point(-2, 0),
            new Point(-1, 0),
            new Point( 0, 0),
            new Point( 1, 0),

            new Point(-2,-1),
            new Point(-1,-1),
            new Point( 0,-1),
            new Point( 1,-1),

            new Point(-2,-2),
            new Point(-1,-2),
            new Point( 0,-2),
            new Point( 1,-2),
         };
      #endregion

      List<Export360.XTD_DeulanyTIN.triangle> mPathMeshTris = null;
      List<Vector3> mPathMeshVerts = null;


      float mOldSimTileScale = 0;
      void updateBetterSim(bool restore)
      {
         TerrainCreationParams prms = new TerrainCreationParams();
         if (!restore)
         {
            mOldSimTileScale = 1.0f / TerrainGlobals.getEditor().getSimRep().getVisToSimScale();
            prms.initFromVisData(TerrainGlobals.getTerrain().getNumXVerts(), TerrainGlobals.getTerrain().getNumZVerts(), TerrainGlobals.getTerrain().getTileScale(), 1);
            TerrainGlobals.getEditor().getSimRep().reinit(prms.mNumSimXTiles, prms.mNumSimZTiles, prms.mSimTileSpacing, 1);
         }
         else
         {
            prms.initFromVisData(TerrainGlobals.getTerrain().getNumXVerts(), TerrainGlobals.getTerrain().getNumZVerts(), TerrainGlobals.getTerrain().getTileScale(), mOldSimTileScale);

            TerrainGlobals.getEditor().getSimRep().reinit(prms.mNumSimXTiles, prms.mNumSimZTiles, prms.mSimTileSpacing, 1.0f / (float)mOldSimTileScale);
         }

         TerrainGlobals.getEditor().getSimRep().update(false,false);//updateSimRep(TerrainGlobals.getEditor().getDetailPoints(), TerrainGlobals.getTerrain().getNumXVerts(), TerrainGlobals.getTerrain().getTileScale(), false);
         

      }
      //---------------------------------
      int giveSimValueForVisVert(int x, int z, bool smallOrLargeSet)
      {
         int numXTiles = 0;
         int numZTiles = 0;
         TerrainGlobals.getEditor().getSimRep().giveSimTileBounds(out numXTiles, out numZTiles);

         Point[] tileOffsets = smallOrLargeSet ? tileOffsets4 : tileOffsets16;
         //create the grouped bit descriptor
         int val = 0;
         for (int i = 0; i < tileOffsets.Length; i++)
         {
            val = val << 1;
            int dX = (int)BMathLib.Clamp(tileOffsets[i].X + x, 0, numXTiles - 1);
            int dZ = (int)BMathLib.Clamp(tileOffsets[i].Y + z, 0, numXTiles - 1);

            int v = TerrainGlobals.getEditor().getSimRep().getDataTiles().isTileLandObstructed(dX, dZ) ? 1 : 0;
            val |= v;

         }
         return val;
      }
      void identifyPrimeSimVerts(List<Point> primeSimVerts, uint vertQuantizationDist,uint maxNumQuantizedPoints)
      {
         int width = TerrainGlobals.getTerrain().getNumXVerts();
         //walk the tiles, generate a 3x3 bit definition
         for (int x = 0; x < width; x++)
         {
            for (int z = 0; z < width; z++)
            {
               //get the 4 tiles touching me

               int val = giveSimValueForVisVert(x, z, false);

               //if this mapping is in our ignore list, then ignore it.
               bool ok = false;
               for (int i = 0; i < mSimTileCornerDefn16.Length; i++)
               {
                  if (val == mSimTileCornerDefn16[i])
                  {
                     ok = true;
                     break;
                  }
               }


               if (ok)
               {
                  primeSimVerts.Add(new Point(x, z));
               }

            }
         }

         //don't quantize if we aren't told to
         if (maxNumQuantizedPoints == 0)
            return;

         //Now, QUANTIZE these verts into a working set...
         //CLM is this really what we want to do? We need to minimize the concave hull, but this actually eliminates the concave hull if the number of points are
         //small enough..
         int numDesiredQuantizedPoints = (int)(primeSimVerts.Count * 0.5f);// (int)Math.Min(maxNumQuantizedPoints, primeSimVerts.Count * 0.8f);
         Weighted_MinMax_Quantizer vq = new Weighted_MinMax_Quantizer();
         for (int i = 0; i < primeSimVerts.Count; i++)
         {
            Vector3 p = new Vector3(primeSimVerts[i].X, primeSimVerts[i].Y, 0);
            vq.insert(p, 1.0f);
         }
         vq.quantize(vertQuantizationDist, numDesiredQuantizedPoints);
         primeSimVerts.Clear();
         for (int i = 0; i < vq.num_output_cells(); i++)
            primeSimVerts.Add(new Point((int)vq.output_cell(i).X, (int)vq.output_cell(i).Y));
         
         

      }
      //---------------------------------
      void addEdgeVerts(List<Point> vertList)
      {
         vertList.Add(new Point(0, 0));
         vertList.Add(new Point(0, TerrainGlobals.getTerrain().getNumXVerts() - 1));
         vertList.Add(new Point(TerrainGlobals.getTerrain().getNumXVerts() - 1, 0));
         vertList.Add(new Point(TerrainGlobals.getTerrain().getNumXVerts() - 1, TerrainGlobals.getTerrain().getNumXVerts() - 1));

         int nodeWidth = (int)BTerrainQuadNode.cMaxWidth;
         int numEdgeVerts = (int)(TerrainGlobals.getTerrain().getNumXVerts() / BTerrainQuadNode.cMaxWidth);
         for (int i = 1; i < numEdgeVerts; i++)
         {
            vertList.Add(new Point(i * nodeWidth, 0));
            vertList.Add(new Point(0, i * nodeWidth));
            vertList.Add(new Point(i * nodeWidth, TerrainGlobals.getTerrain().getNumXVerts() - 1));
            vertList.Add(new Point(TerrainGlobals.getTerrain().getNumXVerts() - 1, i * nodeWidth));
         }
      }
      //---------------------------------
      void removeUnWantedVerts(List<Point> vertList)
      {


         int width = TerrainGlobals.getTerrain().getNumXVerts();
         //keep a list of verts we WANT to remove.
         List<int> ptToRem = new List<int>();
         for (int k = 0; k < vertList.Count; k++)
         {
            //create the 3x3 bit descriptor
            int val = 0;
            for (int i = 0; i < offsets.Length; i++)
            {
               val = val << 1;
               Point kkt = new Point(
                  (int)BMathLib.Clamp(vertList[k].X + offsets[i].X, 0, width - 1),
                  (int)BMathLib.Clamp(vertList[k].Y + offsets[i].Y, 0, width - 1));

               int v = vertList.Contains(kkt) ? 1 : 0;
               val |= v;
            }

            bool ok = false;
            for (int i = 0; i < mVertNeighborLines.Length; i++)
            {
               if (val == mVertNeighborLines[i])
               {
                  ptToRem.Add(i);
                  break;
               }
            }

         }

         //remove all the unwanted points..
         //walk from the back, so we don't got problems..
         for (int i = ptToRem.Count - 1; i >= 0; i--)
         {
            vertList.RemoveAt(ptToRem[i]);
         }

      }
      //---------------------------------
      void removeTri(List<Export360.XTD_DeulanyTIN.triangle> triList, int index)
      {
         if (triList[index].sharedTriEdge[0] != null) triList[index].sharedTriEdge[0].clearEdgeConnection(triList[index]);
         if (triList[index].sharedTriEdge[1] != null) triList[index].sharedTriEdge[1].clearEdgeConnection(triList[index]);
         if (triList[index].sharedTriEdge[2] != null) triList[index].sharedTriEdge[2].clearEdgeConnection(triList[index]);

         triList.RemoveAt(index);

      }
      void pruneTriList(List<Export360.XTD_DeulanyTIN.triangle> triList, List<Point> primeSimVerts)
      {
         //remove any trs that span impassible terrain
         for (int i = 0; i < triList.Count; i++)
         {
            Export360.XTD_DeulanyTIN.triangle tri = triList[i];

            Vector3 v0 = new Vector3(primeSimVerts[tri.vertIndex[0]].X, 0, primeSimVerts[tri.vertIndex[0]].Y);// TerrainGlobals.getTerrain().getPostDeformPos(primeSimVerts[tri.vertIndex[0]].X, primeSimVerts[tri.vertIndex[0]].Y);
            Vector3 v1 = new Vector3(primeSimVerts[tri.vertIndex[1]].X, 0, primeSimVerts[tri.vertIndex[1]].Y);//TerrainGlobals.getTerrain().getPostDeformPos(primeSimVerts[tri.vertIndex[1]].X, primeSimVerts[tri.vertIndex[1]].Y);
            Vector3 v2 = new Vector3(primeSimVerts[tri.vertIndex[2]].X, 0, primeSimVerts[tri.vertIndex[2]].Y);//TerrainGlobals.getTerrain().getPostDeformPos(primeSimVerts[tri.vertIndex[2]].X, primeSimVerts[tri.vertIndex[2]].Y);

            if (BMathLib.cFloatCompareEpsilon > BMathLib.areaOfTriangle(ref v0, ref v1, ref v2))
            {
               removeTri(triList, i);
               i--;
               continue;
            }

            //find the centroid of this triangle. bail if it's surrounded by impassible area
            Vector3 p0v = new Vector3(primeSimVerts[tri.vertIndex[0]].X, 0, primeSimVerts[tri.vertIndex[0]].Y);
            Vector3 p1v = new Vector3(primeSimVerts[tri.vertIndex[1]].X, 0, primeSimVerts[tri.vertIndex[1]].Y);
            Vector3 p2v = new Vector3(primeSimVerts[tri.vertIndex[2]].X, 0, primeSimVerts[tri.vertIndex[2]].Y);

            Vector3 centroid = BMathLib.centroidOfTriangle(ref p0v, ref p1v, ref p2v);

            Point cx = new Point((int)centroid.X, (int)centroid.Z);
            if (0x0F == giveSimValueForVisVert(cx.X, cx.Y, true))
            {
              removeTri(triList,i);
               i--;
               continue;
            }

            triList[i].myIndex = i;
         }
      }
      void pruneUnusedVerts(List<Export360.XTD_DeulanyTIN.triangle> triList, List<Vector3> verts)
      {
         for (int i = 0; i < verts.Count; i++)
         {
            bool usesMe = false;
            for(int j=0;j<triList.Count;j++)
            {
               if(triList[j].vertIndex[0]==i ||
                  triList[j].vertIndex[1]==i ||
                  triList[j].vertIndex[2]==i)
               {
                  usesMe = true;
                  break;
               }
            }
            if(!usesMe)
            {
               for (int j = 0; j < triList.Count; j++)
               {
                  if(triList[j].vertIndex[0]>i)triList[j].vertIndex[0]--;
                  if(triList[j].vertIndex[1]>i)triList[j].vertIndex[1]--;
                  if(triList[j].vertIndex[2]>i)triList[j].vertIndex[2]--;
               }
               verts.RemoveAt(i);
               i--;
            }
         }
      }
      //---------------------------------
      public void generateNavMesh()
      {
         generateNavMesh(128,0);
      }
      //NOTE, PASS 0 or numPoints for "I DON'T CARE"
      public void generateNavMesh(uint vertQuantizationDist, uint maxNumQuantizedPoints)
      {
         updateBetterSim(false);

         Export360.XTD_DeulanyTIN tin = new Export360.XTD_DeulanyTIN();

         //add edges into the mix.
         List<Point> primeSimVerts = new List<Point>();
         addEdgeVerts(primeSimVerts);

         identifyPrimeSimVerts(primeSimVerts, vertQuantizationDist, maxNumQuantizedPoints);

         ////  removeUnWantedVerts(primeSimVerts);

         //  add the sim verts to the TIN
         for (int i = 0; i < primeSimVerts.Count; i++)
            tin.addVertex(TerrainGlobals.getTerrain().getPostDeformPos(primeSimVerts[i].X, primeSimVerts[i].Y));



         //create our TIN
         tin.Triangulate();


         mPathMeshVerts = tin.getVertList();
         //remove impassible tris & Tris w/ no area
         mPathMeshTris = tin.getTriList();
         pruneTriList(mPathMeshTris, primeSimVerts);  //NOTE, this may leave some un-used verts in the list.. do we care? humm..
       //  pruneUnusedVerts(mPathMeshTris, primeSimVerts); //If we do, call this....?

         updateBetterSim(true); //put the sim back.. (temp)

      }
      //---------------------------------
      public void toVisualBuffer(ref VertexBuffer vb, ref IndexBuffer ib, ref int numVerts, ref int numPrims)
      {
         numVerts = mPathMeshVerts.Count;
         numPrims = mPathMeshTris.Count;

         int numInds = numPrims * 3;

         vb = new VertexBuffer(typeof(VertexTypes.Pos_Color), numVerts, BRenderDevice.getDevice(), Usage.None, VertexTypes.Pos_Color.FVF_Flags, Pool.Managed);
         GraphicsStream gStream = vb.Lock(0, 0, LockFlags.None);
         unsafe
         {

            VertexTypes.Pos_Color* vP = (VertexTypes.Pos_Color*)gStream.InternalDataPointer;
            for (int i = 0; i < numVerts; i++)
            {
               vP[i].x = mPathMeshVerts[i].X;
               vP[i].y = mPathMeshVerts[i].Y;
               vP[i].z = mPathMeshVerts[i].Z;
               vP[i].color = 0x440000FF;
            }
         }

         vb.Unlock();


         ib = new IndexBuffer(typeof(int), numInds, BRenderDevice.getDevice(), Usage.None, Pool.Managed);
         gStream = ib.Lock(0, 0, LockFlags.None);
         unsafe
         {
            int* iP = (int*)gStream.InternalDataPointer;
            for (int i = 0; i < numPrims; i++)
            {
               iP[i * 3 + 0] = mPathMeshTris[i].vertIndex[0];
               iP[i * 3 + 1] = mPathMeshTris[i].vertIndex[1];
               iP[i * 3 + 2] = mPathMeshTris[i].vertIndex[2];
            }
         }
         ib.Unlock();
      }
      //---------------------------------
      void CreatePTH(string filename, bool UsePerforce)
      {
         try
         {
            if (File.Exists(filename))
               File.Delete(filename);

            //genPTH -navmeshfile <*.NM> -pthfile <*.PTH>
            if (File.Exists(CoreGlobals.getWorkPaths().mPTHToolPath) == false)
            {
               MessageBox.Show("Can't find: " + CoreGlobals.getWorkPaths().mPTHToolPath, "Error exporting " + filename);
               return;
            }

            string navMeshFile = Path.ChangeExtension(filename, "NM");

            string arguments = "";
            arguments = arguments + " -navmeshfile \"" + navMeshFile + "\"";
            arguments = arguments + " -pthfile \"" + filename + "\"";
            //if (UsePerforce == true)
            //{
            //   arguments = arguments + " -checkout";
            //}

            System.Diagnostics.Process pthUtility;
            pthUtility = new System.Diagnostics.Process();
            pthUtility = System.Diagnostics.Process.Start(CoreGlobals.getWorkPaths().mPTHToolPath, arguments);
            pthUtility.WaitForExit();
            pthUtility.Close();

         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }
      }
      void WriteNM(string filename)
      {
         string tmpNMFile = Path.ChangeExtension(filename, "NM");
         if (File.Exists(tmpNMFile))
            File.Delete(tmpNMFile);

         FileStream s = File.Open(tmpNMFile, FileMode.OpenOrCreate, FileAccess.Write);
         BinaryWriter f = new BinaryWriter(s);

         f.Write(mPathMeshVerts.Count);
         f.Write(mPathMeshTris.Count);

         for (int i = 0; i < mPathMeshVerts.Count; i++)
         {
            f.Write( (int)(mPathMeshVerts[i].X * 100));
            f.Write( (int)(mPathMeshVerts[i].Z * 100));
         }

         for (int i = 0; i < mPathMeshTris.Count; i++)
         {
            f.Write( mPathMeshTris[i].vertIndex[0] );
            f.Write( mPathMeshTris[i].vertIndex[1] );
            f.Write( mPathMeshTris[i].vertIndex[2] );
         }

         f.Close();
         s.Close();
      }
      void DeleteNM(string filename)
      {
         string tmpNMFile = Path.ChangeExtension(filename, "NM");
         if (File.Exists(tmpNMFile))
            File.Delete(tmpNMFile);
      }
      
      public void exportNavMeshToPTH(string fileName)
      {
         if (mPathMeshTris.Count == 0)
            return;


         WriteNM(fileName);//open a temp file, and write this data to disk in a binary form.

         CreatePTH(fileName, true);

         DeleteNM(fileName);//delete temp file, now that we're done with it..
      }
      //---------------------------------
      public void exportNavMeshToOBJ()
      {
         if (mPathMeshTris.Count == 0)
            return;
         
         //ouput our data to the file.
         OBJFile output = new OBJFile();
         output.addObject("Terrain");
         List<int> inds = new List<int>();
         for (int i = 0; i < mPathMeshTris.Count; i++)
         {
            Export360.XTD_DeulanyTIN.triangle tri = mPathMeshTris[i];

            //if this tri has 0 area, get rid of it..


            //CLM deulany outputs a different clockwise winding than i want..
            //this SHOULD be a proper clockwise winding test
            inds.Add(tri.vertIndex[0]);
            inds.Add(tri.vertIndex[1]);
            inds.Add(tri.vertIndex[2]);

         }

         output.addVertexList(0, mPathMeshVerts, inds);

         string filenameNoExt = Path.GetFileNameWithoutExtension(CoreGlobals.ScenarioFile);
         string targetFilename = CoreGlobals.ScenarioDirectory + @"\" + filenameNoExt + ".obj";
         if (File.Exists(targetFilename))
            File.Delete(targetFilename);
         output.write(new FileStream(targetFilename, FileMode.OpenOrCreate));
         output = null;
      }
      //---------------------------------
     
   }
}