using System;
using System.IO;
using System.Drawing;
using System.Drawing.Imaging;
using System.Collections.Generic;
using System.Text;
using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;
using Microsoft.DirectX.DirectInput;
using System.Windows.Forms;
using System.Threading;
using System;
using System.Diagnostics;
using System.Xml;
using System.Xml.Serialization;

using Rendering;
using EditorCore;
using SimEditor;

////


//------------------------------------------
namespace Terrain
{
   //----------------------------------
   //----------------------------------
   public class FoliageQNChunk
   {
      public int mOwnerQNIndex;
      public List<string> mSetsUsed = new List<string>();
      public List<VertexBuffer> mSetVBs = new List<VertexBuffer>();
      public List<IndexBuffer> mSetIBs = new List<IndexBuffer>();
      public List<int> mSetVertCount = new List<int>();
      public List<int> mSetPolyCount = new List<int>();

      public BTerrainQuadNodeDesc mOwnerNodeDesc;   //CLM this is just for offset info, DO NOT USE THIS FOR BB CALCS!!!!

      public FoliageQNChunk()
      {

      }
      ~FoliageQNChunk()
      {
         destroy();
         mSetVBs = null;
         mSetIBs = null;
      }

      public void init(int owningNodeIndex)
      {
         mOwnerQNIndex = owningNodeIndex;
         mOwnerNodeDesc = TerrainGlobals.getTerrain().getQuadNodeLeafArray()[mOwnerQNIndex].getDesc();
      }
      public void destroy()
      {
         destroyRenderVBs();
      }
      //-------------------------------------
      public void bladeAdded(string setName)
      {
         if (!mSetsUsed.Contains(setName))
         {
            mSetsUsed.Add(setName);
         }
         destroyRenderVBs();
      }
      public void bladeRemoved(int setIndex)
      {
         //if (!mSetsUsed.Contains(setIndex))
         //{
         //   mSetsUsed.Add(setIndex);
         //}
         destroyRenderVBs();
      }
      public bool doesContainSet(string setName)
      {
         return mSetsUsed.Contains(setName);
      }
      //-------------------------------------

      public void createRenderVBs()
      {
         destroyRenderVBs();
         int width = TerrainGlobals.getTerrain().getNumXVerts();
         for(int setI=0;setI<mSetsUsed.Count;setI++)
         {
            List<int> validIndexes = new List<int>();
            List<int> validBladeIndexes = new List<int>();
            for (int x = 0; x < BTerrainQuadNode.cMaxWidth; x++)
            {
               for (int z = 0; z < BTerrainQuadNode.cMaxWidth; z++)
               {
                  int index = (x + mOwnerNodeDesc.mMinXVert) + width * (z + mOwnerNodeDesc.mMinZVert);
                  FoliageVertData fvd = FoliageManager.mVertData.GetValue(index);
                  if (fvd.compare(FoliageManager.cEmptyVertData))
                     continue;

                  if(FoliageManager.giveIndexOfSet(fvd.mFoliageSetName) == FoliageManager.giveIndexOfSet(mSetsUsed[setI]))
                  {
                     int localIndex = x + FoliageManager.cNumXBladesPerChunk * z;
                     validIndexes.Add(localIndex);
                     validBladeIndexes.Add(fvd.mFoliageSetBladeIndex);
                  }
               }
            }

            if (validIndexes.Count == 0)
            {
               //remove this set from us
               mSetsUsed.RemoveAt(setI);
               setI--;
               continue;
            }

            //now that we have our valid indexes
            int numVertsPerBlade = 10;
            int totalNumVerts = (validIndexes.Count * numVertsPerBlade);

            //VERTEX BUFFERS
            mSetVBs.Add(new VertexBuffer(typeof(VertexTypes.Pos), totalNumVerts, BRenderDevice.getDevice(), Usage.WriteOnly, VertexTypes.Pos.FVF_Flags, Pool.Default));

            GraphicsStream gStream = mSetVBs[setI].Lock(0, 0, LockFlags.None);
            unsafe
            {
               VertexTypes.Pos* verts = (VertexTypes.Pos*)gStream.InternalDataPointer;

               mSetVertCount.Add(0);
               //ADD OTHER STRIPS
               for (int i = 0; i < validIndexes.Count; i++)
               {
                  int vC = mSetVertCount[setI];

                  int startInd = validIndexes[i] * numVertsPerBlade;
                  //add 10 verts with:  index (x*width+z), bladeIndex, 0, 0 
                  for (int k = 0; k < numVertsPerBlade; k++)
                  {
                     verts[vC].x = startInd + k;
                     verts[vC].y = validBladeIndexes[i];

                     vC++;
                  }
                 
                  mSetVertCount[setI] += numVertsPerBlade;
               }
            }

            gStream.Close();
            mSetVBs[setI].Unlock();


            //INDEX BUFFERS (UGG WE SHOULD BE USING TRI-STRIPS!!!)
            int numInds = (validIndexes.Count * ((numVertsPerBlade - 2) * 3));
            mSetIBs.Add(new IndexBuffer(typeof(short), numInds, BRenderDevice.getDevice(), Usage.WriteOnly, Pool.Default));

            gStream = mSetIBs[setI].Lock(0, 0, LockFlags.None);
            unsafe
            {
               short* inds = (short*)gStream.InternalDataPointer;

               mSetPolyCount.Add(0);

               //add first strip
               short startInd = (short)(0);
               int vC = 0;

               for (int i = 0; i < validIndexes.Count; i++)
               {
                  startInd = (short)(i * (numVertsPerBlade));

                  short pC = (short)((numVertsPerBlade - 2) >> 1);
                  short sI = 0;
                  for(int k=0;k<pC;k++)
                  {
                     inds[vC++] = (short)(startInd + sI + 0);
                     inds[vC++] = (short)(startInd + sI + 1);
                     inds[vC++] = (short)(startInd + sI + 2);

                     inds[vC++] = (short)(startInd + sI + 1);
                     inds[vC++] = (short)(startInd + sI + 2);
                     inds[vC++] = (short)(startInd + sI + 3);
                     sI += 2;
                  }
                  
                  
                  mSetPolyCount[setI] += (numVertsPerBlade - 2);
               }
            }
            gStream.Close();
            mSetIBs[setI].Unlock();
           
         }

         //COOL!
      }
      public void destroyRenderVBs()
      {
         for (int i = 0; i < mSetVBs.Count; i++)
         {
            mSetVBs[i].Dispose();
            mSetVBs[i] = null;
         }
         mSetVBs.Clear();
         for (int i = 0; i < mSetIBs.Count; i++)
         {
            mSetIBs[i].Dispose();
            mSetIBs[i] = null;
         }
         mSetIBs.Clear();
         mSetVertCount.Clear();
         mSetPolyCount.Clear();
      }
      public bool render()
      {
         if (mSetVBs.Count == 0)
         {
            createRenderVBs();
            if (mSetVBs.Count == 0)
               return false;
         }

         BRenderDevice.getDevice().VertexDeclaration = VertexTypes.Pos.vertDecl;

         //my shader is started in the parent FoliageManager class..

         BTerrainQuadNode qn = TerrainGlobals.getTerrain().getQuadNodeLeafArray()[mOwnerQNIndex];
         
         //find our visual handle index 
         //CLM the skirts fuck this up.. just find the highest LOD that's valid...
         BTerrainVisualDataHandle vdh = null;
         for(int i=0;i<(int)Terrain.BTerrainVisual.eLODLevel.cLODCount;i++)
         {
            if(qn.mVisualDataIndxPerLOD[i]!= -1)
            {
               vdh = TerrainGlobals.getVisual().getVisualHandle( qn.mVisualDataIndxPerLOD[i]);
               break;
            }
         }

         if(vdh==null)
            return false;

         //bind my QN positions texture
         FoliageManager.mFoliageGPUShader.mShader.SetValue(FoliageManager.mTerrainQNPosTexture, vdh.mHandle.mPositionsTexture);

         Vector4 QnData = new Vector4((BTerrainQuadNode.getMaxNodeWidth() >> vdh.LOD), TerrainGlobals.getTerrain().getTileScale(), mOwnerNodeDesc.mMinXVert, mOwnerNodeDesc.mMinZVert);
         FoliageManager.mFoliageGPUShader.mShader.SetValue(FoliageManager.mTerrainQNData, QnData);

         
         for (int setI = 0; setI < mSetsUsed.Count; setI++)
         {
            FoliageSet set = FoliageManager.giveSet(mSetsUsed[setI]);

            float rcpBladeImgWidth = 1.0f / (float)set.mNumBlades;
            FoliageManager.mFoliageGPUShader.mShader.SetValue(FoliageManager.mSetRCPBladeTexWidth, rcpBladeImgWidth);

            
            FoliageManager.mFoliageGPUShader.mShader.SetValue(FoliageManager.mSetAlbedoTexture, 
                                                                              set.mD3DAlbedoTexture==null?
                                                                              BRenderDevice.getTextureManager().getDefaultTexture(TextureManager.eDefaultTextures.cDefTex_Red):
                                                                              set.mD3DAlbedoTexture.mTexture);

            FoliageManager.mFoliageGPUShader.mShader.SetValue(FoliageManager.mSetOpacityTexture,
                                                                  set.mD3DOpacityTexture == null ?
                                                                  BRenderDevice.getTextureManager().getDefaultTexture(TextureManager.eDefaultTextures.cDefTex_White) :
                                                                  set.mD3DOpacityTexture.mTexture);


            FoliageManager.mFoliageGPUShader.mShader.SetValue(FoliageManager.mSetPositionsTexture, set.mD3DPositionsTexture);
            FoliageManager.mFoliageGPUShader.mShader.SetValue(FoliageManager.mSetUVsTexture, set.mD3DUVsTexture);

            //DRAW US FOOL!
            BRenderDevice.getDevice().SetStreamSource(0, mSetVBs[setI], 0);
            BRenderDevice.getDevice().Indices = mSetIBs[setI];

            FoliageManager.mFoliageGPUShader.mShader.CommitChanges();
            BRenderDevice.getDevice().DrawIndexedPrimitives(PrimitiveType.TriangleList, 0, 0, mSetVertCount[setI], 0, mSetPolyCount[setI]);
         }

         return true;
      }
   };

   //----------------------------------
   //----------------------------------
   public class FoliageVertData
   {
      public FoliageVertData()
      {
         mFoliageSetName = "";
         mFoliageSetBladeIndex=-1;
      }
      public FoliageVertData(string setName, int bladeIndex)
      {
         mFoliageSetName = setName;
         mFoliageSetBladeIndex = bladeIndex;
      }
      public String mFoliageSetName="";
      public int mFoliageSetBladeIndex=-1;

      public bool compare(FoliageVertData a)
      {
         return a.mFoliageSetName == mFoliageSetName && a.mFoliageSetBladeIndex == mFoliageSetBladeIndex;
      }


   };
   //----------------------------------
   //----------------------------------
   public class FoliageBlade
   {
      ~FoliageBlade()
      {
         verts.Clear();
         norms.Clear();
         uvs.Clear();
      }
      public List<Vector3> verts = new List<Vector3>();
      public List<Vector3> norms = new List<Vector3>();
      public List<Vector3> uvs = new List<Vector3>();
   };

   //----------------------------------
   //----------------------------------
   public class FoliageSet
   {
      public string mFullFileName = "";
      string mSetName = "";
      public int mNumBlades = 0;
      public int mNumVertsPerBlade = 1;
      public int mAlbedoImageWidth = 0;

      public Vector3 mBBMax = Vector3.Empty;
      public Vector3 mBBMin = Vector3.Empty;

      public List<FoliageBlade> mFoliageBlades = new List<FoliageBlade>();

      public Texture mD3DPositionsTexture = null;
      public Texture mD3DUVsTexture = null;
      public TextureHandle mD3DAlbedoTexture = null;
      public TextureHandle mD3DOpacityTexture = null;


      public FoliageSet()
      {

      }

      ~FoliageSet()
      {
         destroy();
         mFoliageBlades = null;
      }

      public void destroy()
      {
         mFoliageBlades.Clear();
      }
      public void denitDeviceData()
      {
         if(mD3DPositionsTexture!=null)
         {
            mD3DPositionsTexture.Dispose();
            mD3DPositionsTexture = null;
         }
         if (mD3DUVsTexture != null)
         {
            mD3DUVsTexture.Dispose();
            mD3DUVsTexture = null;
         }
         if (mD3DAlbedoTexture != null)
         {
            mD3DAlbedoTexture.freeD3DHandle();
            mD3DAlbedoTexture = null;
         }
         if (mD3DOpacityTexture != null)
         {
            mD3DOpacityTexture.freeD3DHandle();
            mD3DOpacityTexture = null;
         }
      }


      public void loadSet(string filename)
      {
        // saveSet(filename + "1");

         mFullFileName = filename;
         if (mFullFileName.Contains(".xml"))
            mFullFileName = mFullFileName.Substring(0, mFullFileName.LastIndexOf(".xml"));

         foliageSetXML fsXML = new foliageSetXML();

         XmlSerializer s = new XmlSerializer(typeof(foliageSetXML), new Type[] { });
         Stream st = File.OpenRead(filename);
         fsXML = (foliageSetXML)s.Deserialize(st);
         st.Close();

         mSetName = fsXML.SetName;
         mNumVertsPerBlade =Convert.ToInt32(fsXML.numVertsPerType);
         mNumBlades =Convert.ToInt32(fsXML.typecount);
         mBBMax = TextVectorHelper.FromString(fsXML.max);
         mBBMin = TextVectorHelper.FromString(fsXML.min);

         for(int i=0;i<mNumBlades;i++)
         {
            FoliageBlade fb = new FoliageBlade();
            for(int q =0;q<mNumVertsPerBlade;q++)
            {
               Vector3 v =TextVectorHelper.FromString(fsXML.mBlades[i].mVerts[q].pos);
               Vector3 n = TextVectorHelper.FromString(fsXML.mBlades[i].mVerts[q].norm);
               Vector3 uv = TextVectorHelper.FromString(fsXML.mBlades[i].mVerts[q].uv);

               fb.verts.Add(v);
               fb.norms.Add(n);
               fb.uvs.Add(uv);
            }

            mFoliageBlades.Add(fb);
         }

         fsXML = null;

         denitDeviceData();

         //load our albedo texture;
         string textureName = filename.Substring(0, filename.LastIndexOf(".")) + "_df.tga";
         if (File.Exists(textureName))
         {
            mD3DAlbedoTexture = BRenderDevice.getTextureManager().getTexture(textureName);
            SurfaceDescription sd = mD3DAlbedoTexture.mTexture.GetLevelDescription(0);
            mAlbedoImageWidth = sd.Width;
         }

         textureName = filename.Substring(0, filename.LastIndexOf(".")) + "_op.tga";
         if (File.Exists(textureName))
         {
            mD3DOpacityTexture = BRenderDevice.getTextureManager().getTexture(textureName);
         }
         
        
         //create our positions texture
         createD3DTexturesFromBlades();
      }
      void createD3DTexturesFromBlades()
      {
         if (mFoliageBlades.Count == 0)
            return;

         //create our foliage texures
         int numVerts = mFoliageBlades.Count * mNumVertsPerBlade;
         mD3DPositionsTexture = new Texture(BRenderDevice.getDevice(), numVerts, 1, 1, Usage.None, Format.A32B32G32R32F, Pool.Managed);
         mD3DUVsTexture = new Texture(BRenderDevice.getDevice(), numVerts, 1, 1, Usage.None, Format.A32B32G32R32F, Pool.Managed);
         unsafe
         {
            GraphicsStream gs = mD3DPositionsTexture.LockRectangle(0, 0);
            Vector4* verts = (Vector4*)gs.InternalDataPointer;
            GraphicsStream gsUV = mD3DUVsTexture.LockRectangle(0, 0);
            Vector4* UVs = (Vector4*)gsUV.InternalDataPointer;

            int vc=0;
            for (int i = 0; i < mFoliageBlades.Count;i++ )
            {
               for(int j=0;j<mNumVertsPerBlade;j++)
               {
                  verts[vc].X=mFoliageBlades[i].verts[j].X;
                  verts[vc].Y=mFoliageBlades[i].verts[j].Y;
                  verts[vc].Z=mFoliageBlades[i].verts[j].Z;
                  verts[vc].W=1;

                  //uvs
                  UVs[vc].X = mFoliageBlades[i].uvs[j].X;
                  UVs[vc].Y = mFoliageBlades[i].uvs[j].Y;
                  UVs[vc].Z = 0;
                  UVs[vc].W = 0;

                  vc++;
               }
            }
            gs.Close();
            mD3DPositionsTexture.UnlockRectangle(0);
            gsUV.Close();
            mD3DUVsTexture.UnlockRectangle(0);
         }
         

      }

      //void saveSet(string fname)
      //{
      //   foliageSetXML fsXML = new foliageSetXML();
      //   fsXML.typecount = "1";
      //   fsXML.numVertsPerType = "10";
      //   fsXML.SetName = "test";
      //   fsXML.mBlades = new List<foliageSetBladeXML>();

      //   foliageSetBladeXML fb = new foliageSetBladeXML();
      //   foliageSetVertXML fv = new foliageSetVertXML();
      //   fv.norm = "0,0,0";
      //   fv.pos = "0,0,0";
      //   fb.mVerts.Add(fv);
      //   fsXML.mBlades.Add(fb);


      //   XmlSerializer s = new XmlSerializer(typeof(foliageSetXML), new Type[] { });
      //   Stream st = File.Open(fname, FileMode.OpenOrCreate);
      //   s.Serialize(st,fsXML);
      //   st.Close();
      //}

      public class foliageSetVertXML
      {
         [XmlAttribute]
         public string pos;
         [XmlAttribute]
         public string norm;
         [XmlAttribute]
         public string uv;
      };

      public class foliageSetBladeXML
      {
         [XmlArrayItem(ElementName = "vert", Type = typeof(foliageSetVertXML))]
         [XmlArray("elementVerts")]
         public List<foliageSetVertXML> mVerts = new List<foliageSetVertXML>();
      };

      [XmlRoot("foliageset")]
      public class foliageSetXML
      {
         [XmlAttribute]
         public string typecount;

         [XmlAttribute]
         public string SetName;

         [XmlAttribute]
         public string numVertsPerType;

         [XmlAttribute]
         public string max;

         [XmlAttribute]
         public string min;

         [XmlArrayItem(ElementName = "setElement", Type = typeof(foliageSetBladeXML))]
         [XmlArray("setElements")]
         public List<foliageSetBladeXML> mBlades = new List<foliageSetBladeXML>();

      };
   };

   //----------------------------------
   //----------------------------------
   public class FoliageManager
   {
      static List<FoliageSet> mFoliageSets = new List<FoliageSet>();
      public static JaggedContainer<FoliageVertData> mVertData=null;
      public static FoliageVertData cEmptyVertData = new FoliageVertData();
      static List<FoliageQNChunk> mQNChunks = new List<FoliageQNChunk>();

      static public int cNumXBladesPerChunk = 64;        //THIS MUST BE 1:1 WITH TERRAIN VERT DENSITY!
      static public int cNumUsedXBladesPerChunk = 64;    //This allows us to fake 'spacing'.. but really we're just filtering our inputs to 'spaced' frequencies
      static public int cMaxNumBlades = 0x40000;

      public FoliageManager()
      {

      }
      ~FoliageManager()
      {
         destroy();
         mFoliageSets = null;
         mVertData = null;
         mQNChunks = null;
      }

      public static void init()
      {
         mVertData = new JaggedContainer<FoliageVertData>(TerrainGlobals.getTerrain().getNumXVerts() * TerrainGlobals.getTerrain().getNumZVerts());
         mVertData.SetEmptyValue(cEmptyVertData);

         loadShader();
      }
      public static void destroy()
      {
         if (mFoliageSets!=null)
            mFoliageSets.Clear();
         if (mVertData != null)
            mVertData.Clear();
         if (mQNChunks != null)
            mQNChunks.Clear();
         freeShader();
      }

      

      public static void render(bool renderDebugs)
      {
         if (mQNChunks.Count == 0)
            return;

    //     BRenderDevice.getDevice().SetRenderState(RenderStates.FillMode, (int)FillMode.WireFrame);

         BRenderDevice.getDevice().SetRenderState(RenderStates.CullMode, (int)Cull.None);

         Matrix g_matView;
         Matrix g_matProj;
         g_matView = BRenderDevice.getDevice().Transform.View;
         g_matProj = BRenderDevice.getDevice().Transform.Projection;
         Matrix worldViewProjection = g_matView * g_matProj;
         mFoliageGPUShader.mShader.SetValue(mShaderWVPHandle, worldViewProjection);

          BRenderDevice.getDevice().SetRenderState(RenderStates.AlphaTestEnable,true); 
         BRenderDevice.getDevice().SetRenderState(RenderStates.AlphaFunction, (int)Compare.Greater);
         BRenderDevice.getDevice().SetRenderState(RenderStates.ReferenceAlpha, 0x000000AA);
         


         mFoliageGPUShader.mShader.Begin(0);
         mFoliageGPUShader.mShader.BeginPass(0);

         BTerrainQuadNode[] nodes = TerrainGlobals.getTerrain().getQuadNodeLeafArray();
         for (int i = 0; i < mQNChunks.Count; i++)
         {
            if (nodes[mQNChunks[i].mOwnerQNIndex].getDesc().mIsVisibleThisFrame)
            {
               if(!mQNChunks[i].render())
               {
                  //this failed rendering.. remove it from our list..
                  mQNChunks.RemoveAt(i);
                  i--;
               }
            }
         }

         mFoliageGPUShader.mShader.EndPass();
         mFoliageGPUShader.mShader.End();

         BRenderDevice.getDevice().SetRenderState(RenderStates.CullMode, (int)Cull.CounterClockwise);
       //  BRenderDevice.getDevice().SetRenderState(RenderStates.FillMode, (int)FillMode.Solid);
         BRenderDevice.getDevice().SetRenderState(RenderStates.AlphaTestEnable,false); 
      }

      //set queary
      public static int newSet(string setFilename)
      {
         int setIndex = FoliageManager.giveIndexOfSet(setFilename);
         if(setIndex==-1)
         {
            FoliageSet fs = new FoliageSet();
            fs.loadSet(setFilename + ".xml");
            mFoliageSets.Add(fs);
            setIndex = mFoliageSets.Count - 1;
         }
         return setIndex;
      }
      public static int getNumSets()
      {
         return mFoliageSets.Count;
      }
      public static int giveIndexOfSet(string setFileName)
      {
         for (int i = 0; i < mFoliageSets.Count; i++)
            if (mFoliageSets[i].mFullFileName == setFileName)
               return i;

         return -1;
      }
      public static FoliageSet giveSet(int index)
      {
         if (index < 0 || index >= mFoliageSets.Count)
            return null;

         return mFoliageSets[index];
      }
      public static FoliageSet giveSet(string filename)
      {
         for (int i = 0; i < mFoliageSets.Count; i++)
            if (mFoliageSets[i].mFullFileName == filename)
               return mFoliageSets[i];

         return null;
      }
      public static bool isSetUsed(FoliageSet fs)
      {
         int setIndex = giveIndexOfSet(fs.mFullFileName);
         if (setIndex == -1)
            return false;

         long id;
         FoliageVertData maskValue;
         mVertData.ResetIterator();
         while (mVertData.MoveNext(out id, out maskValue))
         {
            if (maskValue.compare(cEmptyVertData))
               continue;

            if (giveIndexOfSet(maskValue.mFoliageSetName) == setIndex)
            {
               return true;
            }
         }
         return false;
      }

      //Editor UI & Painting      
      static int mSelectedSetIndex=0;
      static public int SelectedSetIndex
      {
         get {return mSelectedSetIndex;}
         set { mSelectedSetIndex = value;}
      }

      static int mSelectedBladeIndex=0;
      static public int SelectedBladeIndex
      {
         get {return mSelectedBladeIndex;}
         set { mSelectedBladeIndex = value;}
      }

      static public void setSelectedBladeToGrid(int idx, bool erase, bool forceErase)
      {
         int x = idx / TerrainGlobals.getTerrain().getNumXVerts();
         int z = idx - x * TerrainGlobals.getTerrain().getNumXVerts();

         setSelectedBladeToGrid(x,z, erase, forceErase);
      }
      static public void setSelectedBladeToGrid(int x, int z, bool erase, bool forceErase)
      {
         setBladeToGrid(x,z,mFoliageSets[mSelectedSetIndex].mFullFileName,mSelectedBladeIndex, erase, forceErase);
      }
      static public void setBladeToGrid(int idx, string setName, int bladeIndex, bool erase, bool forceErase)
      {
         int x = idx / TerrainGlobals.getTerrain().getNumXVerts();
         int z = idx - x * TerrainGlobals.getTerrain().getNumXVerts();
         setBladeToGrid(x, z, setName, bladeIndex, erase, forceErase);
      }
      static public void setBladeToGrid(int x, int z,string setName, int bladeIndex, bool erase, bool forceErase)
      {
         int terrainToFoliageGridMultiple = (int)(BTerrainQuadNode.cMaxWidth / FoliageManager.cNumUsedXBladesPerChunk);
         if (x % terrainToFoliageGridMultiple != 0 || z % terrainToFoliageGridMultiple != 0)
            return;

         int setIndex = giveIndexOfSet(setName);

         int idx = x + TerrainGlobals.getTerrain().getNumXVerts() * z;


         int minQNX = (int)(x / BTerrainQuadNode.cMaxWidth);
         int minQNZ = (int)(z / BTerrainQuadNode.cMaxHeight);
         int numXChunks = (int)(TerrainGlobals.getTerrain().getNumXVerts() / BTerrainQuadNode.cMaxWidth);
         int qnIndex = minQNX + numXChunks * minQNZ;
         int idex = giveChunkWithParentIndex(qnIndex);


         if(erase)
         {
            if (mVertData.ContainsValue(idx))
            {
               if(!forceErase)
               {
                  FoliageVertData vRep = mVertData.GetValue(idx);
                  if (giveIndexOfSet(vRep.mFoliageSetName) != setIndex || vRep.mFoliageSetBladeIndex != bladeIndex)
                     return;
               }
               
               mVertData.SetValue(idx, new FoliageVertData()); //Default 'empty' for a jagged<T> value is the default constructor..
               if (idex != -1)
               {
                  mQNChunks[idex].destroyRenderVBs();
               }
            }
         }
         else
         {
            mVertData.SetValue(idx, new FoliageVertData(setName, bladeIndex));

            if (idex == -1)//we need to add a new chunk
            {
               idex = mQNChunks.Count;
               addNewChunk(qnIndex);
            }
            mQNChunks[idex].bladeAdded(mFoliageSets[setIndex].mFullFileName);

         }  
      }

      //queary
      static public bool bladeExistsInBounds(int minxVert, int minZvert, int maxXVert, int maxZVert)
      {
         for(int i=minxVert;i<maxXVert;i++)
         {
            for (int j = minZvert; j < maxZVert; j++)
            {
               int index = i + TerrainGlobals.getTerrain().getNumXVerts() * j;
               if(mVertData.ContainsValue(index))
               {
                  return true;
               }
            }
         }
         return false;
      }
      static public bool isChunkUsed(BTerrainQuadNode node)
      {
         for(int i=0;i<mQNChunks.Count;i++)
         {
            if (mQNChunks[i].mOwnerNodeDesc.mMinXVert == node.getDesc().mMinXVert &&
                mQNChunks[i].mOwnerNodeDesc.mMinZVert == node.getDesc().mMinZVert)
               return true;
         }
         return false;
      }
      static public BBoundingBox giveMaxBBs()
      {
         BBoundingBox bb = new BBoundingBox();
         bb.empty();

         for (int i = 0; i < getNumSets();i++ )
         {
            bb.addPoint(mFoliageSets[i].mBBMin);
            bb.addPoint(mFoliageSets[i].mBBMax);
         }

            return bb;
      }

      //chunk management
      static public int addNewChunk(int qnParentIndex)
      {
         FoliageQNChunk qnc = new FoliageQNChunk();
         qnc.init(qnParentIndex);
         mQNChunks.Add(qnc);
         return mQNChunks.Count - 1;
      }
      static public int giveChunkWithParentIndex(int parentIndex)
      {
         for (int i = 0; i < mQNChunks.Count; i++)
            if (mQNChunks[i].mOwnerQNIndex == parentIndex)
               return i;
         return -1;
      }
      static public void clearAllChunkVBs()
      {
         for(int i=0;i<mQNChunks.Count;i++)
         {
            mQNChunks[i].destroyRenderVBs();
         }
      }
      static public int getNumChunks()
      {
         return mQNChunks.Count;
      }
      static public FoliageQNChunk getChunk(int index)
      {
         if (index < 0 || index >= mQNChunks.Count)
            return null;
         return mQNChunks[index];
      }

      //removals
      public static void eraseBladeFromMap(int setIndex, int bladeIndex)
      {
         long id;
         FoliageVertData maskValue;
         mVertData.ResetIterator();
         while (mVertData.MoveNext(out id, out maskValue))
         {
            if (maskValue.compare(cEmptyVertData))
               continue;
            if (giveIndexOfSet(maskValue.mFoliageSetName) == setIndex && maskValue.mFoliageSetBladeIndex == bladeIndex)
            {
               mVertData.SetValue(id, cEmptyVertData);
            }
         }
         clearAllChunkVBs();
      }
      public static void eraseSetFromMap(int setIndex)
      {
         long id;
         FoliageVertData maskValue;
         mVertData.ResetIterator();
         while (mVertData.MoveNext(out id, out maskValue))
         {
            if (maskValue.compare(cEmptyVertData))
               continue;
            if (FoliageManager.giveIndexOfSet(maskValue.mFoliageSetName) == setIndex)
            {
               mVertData.SetValue(id, cEmptyVertData);
            }
         }
         clearAllChunkVBs();
         mFoliageSets.RemoveAt(setIndex);
      }
      public static void eraseAllFoliage()
      {
         mVertData.Clear();
         clearAllChunkVBs();
         mQNChunks.Clear();
      }
      public static void removeUnusedSets()
      {
         for(int i=0;i<mFoliageSets.Count;i++)
         {
            if (!isSetUsed(mFoliageSets[i]))
               eraseSetFromMap(i);
         }
      }

      //shader stuff
      static public ShaderHandle mFoliageGPUShader = null;

      static EffectHandle mShaderWVPHandle = null;
      static public EffectHandle mSetPositionsTexture = null;
      static public EffectHandle mSetUVsTexture = null;
      
      static public EffectHandle mSetAlbedoTexture = null;
      static public EffectHandle mSetOpacityTexture = null;
      static public EffectHandle mSetRCPBladeTexWidth = null;

      static public EffectHandle mTerrainQNPosTexture = null;
      static public EffectHandle mTerrainQNData = null;
      
      static void loadShader()
      {
         mFoliageGPUShader = BRenderDevice.getShaderManager().getShader(CoreGlobals.getWorkPaths().mEditorShaderDirectory + "\\gpuTerrainFoliage.fx", mainShaderParams);
         mainShaderParams(null);
      }
      static public void mainShaderParams(string filename)
      {
         mShaderWVPHandle           = mFoliageGPUShader.getEffectParam("worldViewProj");
         mSetPositionsTexture       = mFoliageGPUShader.getEffectParam("foliagePositionsTexture");
         mSetUVsTexture             = mFoliageGPUShader.getEffectParam("foliageUVsTexture");
         
         mSetAlbedoTexture          = mFoliageGPUShader.getEffectParam("foliageAlbedoTexture");
         mSetOpacityTexture         = mFoliageGPUShader.getEffectParam("foliageOpacityTexture");

         mSetRCPBladeTexWidth       = mFoliageGPUShader.getEffectParam("gRCPBladeImageWidth");
         

         mTerrainQNPosTexture       = mFoliageGPUShader.getEffectParam("terrainPositionsTexture");
         mTerrainQNData             = mFoliageGPUShader.getEffectParam("gQNData");
      }
      static void freeShader()
      {
         if (mFoliageGPUShader != null)
         {
            if (mFoliageGPUShader.mShader != null)
            {
               mFoliageGPUShader.mShader.Dispose();
               mFoliageGPUShader.mShader = null;
            }

            BRenderDevice.getShaderManager().freeShader(mFoliageGPUShader.mFilename);
            mFoliageGPUShader = null;
         }
      }
     
     
   };
}