using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using System.Drawing;
using System.Drawing.Imaging;
using System.Xml;
using System.Windows.Forms;
using System.Xml.Serialization;

using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;
using Xceed.Zip;
using Xceed.FileSystem;

using Terrain;
using EditorCore;
using SimEditor;


namespace Terrain
{

   
   //public class MetaDataIte

   //keywords text file

   //map reference text file
   public class ClipArtMetaDataManager
   {


   }

   public class ClipArtMetaData
   {
      public List<string> mKeywords = new List<string>();
      public string mDescription = "";
      public int mVertCount = 0;
      public string mVersion = "notSet";
      public string mTags = "";
      public string mSourceControl = "";
      public string mCreator = "";

      public void Read(Stream s)
      {
         XmlDocument d = new XmlDocument();
         StreamReader r = new StreamReader(s);
         d.LoadXml(r.ReadToEnd());
         
         XmlNode root = d.ChildNodes[0];
         foreach (XmlNode node in root)
         {
            if(node.Name == "Description")
            {
               if (node.ChildNodes.Count > 0)
                  mDescription = node.ChildNodes[0].Value;
            }
            if (node.Name == "VertCount")
            {
               if (node.ChildNodes.Count > 0)
                  mVertCount = System.Convert.ToInt32(node.ChildNodes[0].Value);
            }
            if (node.Name == "Version")
            {
               if (node.ChildNodes.Count > 0)
                  mVersion = node.ChildNodes[0].Value;
            }
            if (node.Name == "Tags")
            {
               if (node.ChildNodes.Count > 0)
                  mTags = node.ChildNodes[0].Value;
            }
            if (node.Name == "SourceControl")
            {
               if (node.ChildNodes.Count > 0)
                  mSourceControl = node.ChildNodes[0].Value;
            }
            if (node.Name == "Creator")
            {
               if (node.ChildNodes.Count > 0)
                  mCreator = node.ChildNodes[0].Value;
            }
         }

       
         s.Close();
      }
      public void Write(Stream s)
      {
         XmlDocument doc = new XmlDocument();
         XmlElement header = (XmlElement)doc.AppendChild(doc.CreateElement("MetaData"));
         XmlElement desc = (XmlElement)header.AppendChild(doc.CreateElement("Description"));
         desc.AppendChild(doc.CreateTextNode(mDescription));

         XmlElement vCount = (XmlElement)header.AppendChild(doc.CreateElement("VertCount"));
         vCount.AppendChild(doc.CreateTextNode(mVertCount.ToString()));

         XmlElement ver = (XmlElement)header.AppendChild(doc.CreateElement("Version"));
         ver.AppendChild(doc.CreateTextNode(mVersion));

         XmlElement tags = (XmlElement)header.AppendChild(doc.CreateElement("Tags"));
         tags.AppendChild(doc.CreateTextNode(mTags));

         XmlElement sourcecontrol = (XmlElement)header.AppendChild(doc.CreateElement("SourceControl"));
         sourcecontrol.AppendChild(doc.CreateTextNode(mSourceControl));
                
         XmlElement user = (XmlElement)header.AppendChild(doc.CreateElement("Creator"));
         user.AppendChild(doc.CreateTextNode(mCreator));

         doc.Save(s);
         s.Close();

      }

   }

   public class ClipArtData
   {
      public Image mThumbnail = null;
      public ClipArtMetaData mMetadata = new ClipArtMetaData();
      public string mFileName = "";

      private Dictionary<string, ClipboardData> mData = new Dictionary<string, ClipboardData>();
      public Dictionary<string, bool> mDoSaveData = new Dictionary<string, bool>();
      private AbstractFolder m_folder;
      const string mDataFolderName = "Data";

      int mThumbnailSize = 128;

      public ClipArtData()
      {
         myCallback = new Image.GetThumbnailImageAbort(ThumbnailCallback);

      }
      public ClipArtData(string fileName)
      {
         myCallback = new Image.GetThumbnailImageAbort(ThumbnailCallback);

         mFileName = fileName;
      }
      public string getComponentList() 
      {
         StringWriter w = new StringWriter();

         foreach(string key in mData.Keys)
         {
            w.WriteLine(key);

         }
         return w.ToString();
      }

      bool mbPreviewed = false;
      public bool Preview()
      {
         DiskFile zipFile = new DiskFile(mFileName);
         if (!zipFile.Exists)
         {
            return false;
         }
         ZipArchive zip = new ZipArchive(zipFile);
         AbstractFile[] files = zip.GetFiles(false, null);
         foreach (AbstractFile file in files)
         {
            if(file.Name.Contains("Thumbnail"))
            {
               
               Stream s = file.OpenRead();
               mThumbnail = Image.FromStream(s);
               s.Close();               
            }
            else if(file.Name == "MetaData.xml" )
            {
               Stream s = file.OpenRead();
               //StreamReader r = new StreamReader(s);
               //mMetadata = r.ReadToEnd();
               mMetadata.Read(s);
               s.Close();
            }
         }
         m_folder = zip.GetFolder(mDataFolderName);
         AbstractFile[] datafiles = m_folder.GetFiles(false, null);
         foreach (AbstractFile file in datafiles)
         {
            mData[file.Name] = null;
         }
         mbPreviewed = true;
         return true;
      }

      private int getVertCount()
      {
         if (mData.ContainsKey("CopiedVertexData"))
         {
            CopiedVertexData d = mData["CopiedVertexData"] as CopiedVertexData;
            if (d == null)
               return 0;
            return d.getVertCount();
         }
         if (mData.ContainsKey("CopiedTextureData"))
         {
            CopiedTextureData d = mData["CopiedTextureData"] as CopiedTextureData;
            if (d == null)
               return 0;
            return d.getVertCount();
         }

         return 0;
      }
      
      public ClipboardData getData(string name)
      {
         if (!mData.ContainsKey(name))
         {
            mData[name] = Factory(name);

            return mData[name];
         }
         else
         {
            if(mData[name] == null)
            {
               mData[name] = Factory(name);
               //load
               Stream s = GetDataReadStream(name);
               mData[name].Load(s);
            }
            return mData[name];
         }
      }
      ClipboardData Factory(string name)
      {
         if (name.Contains( "CopiedVertexData" ))
         {
            return new CopiedVertexData();
         }
         else if (name.Contains("CopiedSelectionMask"))
         {
            return new CopiedSelectionMask();
         }
         else if (name.Contains("CopiedTextureData"))
         {
            return new CopiedTextureData();
         }
         else if(name.Contains("CopiedUnitData"))
         {
            return new CopiedUnitData();
         }


         return null;
      }

      public void SetThumbnail(Image thumbnail)
      {
         mThumbnail = thumbnail;
      }  


      public bool HasData()
      {
         foreach (ClipboardData d in mData.Values)
         {
            if (d.HasData())
               return true;
         }
         return false;
      }

      //load and save... yep
      public bool Update(bool overwriteReadOnly)
      {
         if (mMetadata.mVersion != CoreGlobals.getClipArtVersion())
         {
            if (Load())
            {
               return Save(true, overwriteReadOnly);
            }
            else 
            {
               return false;
            }
         }
         else
         {
            return true;
         }
      }
     
      public bool Load()
      {
         if (!mbPreviewed)
         {
            if ( !Preview() )
               return false;
         }
         bool retval = true;
         //List<ClipboardData> newData = new List<ClipboardData>();
         //foreach (string dataChannel in mData.Keys)
         //{
         //}
         List<string> channels = new List<string>();
         channels.AddRange(mData.Keys);
         foreach (string dataChannel in channels)
         {
            getData(dataChannel);
         }

         //   getData(dataChannel);
            //ClipboardData d = Factory(dataChannel);
            //retval =  retval && d.Load(GetDataReadStream(d.GetTypeName()));
            //mData[d.GetTypeName()] = d;
         
         return retval;
      }
      public bool Save()
      {
         return Save(true, true);
      }
      public bool Save(bool overwrite, bool checkReadOnly)
      {
         mbPreviewed = true;//because we need this data to save with...

         if (File.Exists(mFileName))
         {
            if (!overwrite)
            {
               return false;
            }
            if (checkReadOnly && File.GetAttributes(mFileName) == FileAttributes.ReadOnly)
            {
               return false;
            }
            File.SetAttributes(mFileName, FileAttributes.Normal);
            File.Delete(mFileName);           
         }
         
         DiskFile zipFile = new DiskFile(mFileName);
         zipFile.Create();

         
         if (!zipFile.Exists)
         {
            return false;
         }
         ZipArchive zip = new ZipArchive(zipFile);

         //metadata
         SetAdditionalMetadata();
         AbstractFile md = zip.CreateFile("MetaData.xml", true);
         //StreamWriter s = new StreamWriter(md.OpenWrite(true));
         Stream s = md.OpenWrite(true);
         mMetadata.Write(s);
         //s.Write(mMetadata);
         s.Close();         

         //thumbnail
         AbstractFile thumbnail = zip.CreateFile("Thumbnail.jpg", true);
         Stream st = thumbnail.OpenWrite(true);
         MemoryStream ms = new MemoryStream();

         //resize thumbnail
         if (mThumbnail.Width != mThumbnailSize || mThumbnail.Height != mThumbnailSize)
         {
            Image temp = mThumbnail.GetThumbnailImage(mThumbnailSize, mThumbnailSize, myCallback, IntPtr.Zero);
            mThumbnail.Dispose();
            mThumbnail = temp;
         }
         mThumbnail.Save(ms, ImageFormat.Jpeg);
         byte[] data = ms.ToArray();
         st.Write(data, 0, data.Length);
         st.Close();

         //data
         m_folder = zip.CreateFolder(mDataFolderName);
         foreach (ClipboardData d in mData.Values)
         {
            if(mDoSaveData[d.GetTypeName()])
               d.Save(GetDataWriteStream(d.GetTypeName()));
         } 
         
         return false;
      }

      public void SetAdditionalMetadata()
      {
         mMetadata.mVertCount = getVertCount();
      }


      //Does nothing...
      public Image.GetThumbnailImageAbort myCallback;
      public bool ThumbnailCallback()
      {
         return false;
      }

      private Stream GetDataReadStream(string fileName)
      {
         AbstractFile file = m_folder.GetFile(fileName);
         if (file != null)
         {
            return file.OpenRead();
         }
         return null;
      }
      private Stream GetDataWriteStream(string fileName)
      {
         bool overwrite = true;
         AbstractFile file = m_folder.CreateFile(fileName, overwrite);
         if (file != null)
         {
            return file.OpenWrite(overwrite);
         }
         return null;
      }			

   }

   public class ClipboardData
   {
      public virtual bool Save(Stream s) { return false; }
      public virtual bool Load(Stream s) { return false; }
      public virtual string GetTypeName() { return "ClipboardData"; }
      public virtual bool HasData() { return false; }

      public virtual void transformData(float xScale, float yScale, float zScale, float rotAngle) { }

      public string mStreamName = "";
   }

   public class CopiedSelectionMask : ClipboardData
   {
      public Dictionary<long, float> mSelection = new Dictionary<long, float>();
      //public string mStreamName = "unnammed";
      public override string GetTypeName() 
      {
         return "CopiedSelectionMask"; 
      }
      public override bool HasData()
      {
         if (mSelection.Count > 0 )
         {
            return true;
         }
         return false;
      }
      public override bool Save(Stream s)
      {
         BinaryWriter b = new BinaryWriter(s);

         int count = mSelection.Count;
         b.Write(count);
         Dictionary<long, float>.Enumerator eSelect = mSelection.GetEnumerator();
         while (eSelect.MoveNext())
         {
            b.Write(eSelect.Current.Key);
            b.Write(eSelect.Current.Value);
         }   
         b.Close();
         return true;
      }
      public override bool Load(Stream s)
      {
         BinaryReader b = new BinaryReader(s);
         int count = b.ReadInt32();
         mSelection.Clear();
         for (int i = 0; i < count; i++)
         {
            long key = b.ReadInt64();
            float alpha = b.ReadSingle();
            mSelection[key] = alpha;
         }
       
         b.Close();

         
         return true;
      }





   }

   public class CopiedUnitData : ClipboardData
   {
      CopiedUnitDataXML mCopiedUnitDataXML = new CopiedUnitDataXML();
      public Vector3 mIntersectCenter = new Vector3();
      private float mTransScaleX = 1.0f;
      private float mTransScaleZ = 1.0f;
      private float mTransRotate = 0.0f;

      public int mTransSizeX = 0;
      public int mTransSizeZ = 0;

      public override bool HasData()
      {
         if (mCopiedUnitDataXML.HasData())
         {
            return true;
         }
         return false;
      }
      public override string GetTypeName()
      {
         return "CopiedUnitData";
      }

      public override bool Save(Stream s)
      {
         XmlSerializer sk = new XmlSerializer(typeof(CopiedUnitDataXML), new Type[] { });
         sk.Serialize(s, mCopiedUnitDataXML);
         s.Close();

         //Not used by game no xmb needed

         return true;
      }
      public override bool Load(Stream s)
      {
         XmlSerializer sk = new XmlSerializer(typeof(CopiedUnitDataXML), new Type[] { });
         mCopiedUnitDataXML = (CopiedUnitDataXML)sk.Deserialize(s);
         s.Close();  

         //load models and objects
         SimGlobals.getSimMain().clipartAddObjects(mCopiedUnitDataXML.mUnits, mCopiedUnitDataXML.mLights);

         mTransScaleX = 1.0f;
         mTransScaleZ = 1.0f;
         mTransRotate = 0.0f;

         return true;
      }

      public void copySelectedData(BTerrainEditor editor)
      {
         List<BTerrainQuadNode> mNodes = new List<BTerrainQuadNode>();
         TerrainGlobals.getTerrain().getQuadNodeRoot().getLeafMaskedNodes(mNodes);
         List<SimEditor.EditorObject> mTotalObjects = new List<EditorObject>();

         BBoundingBox bbox = new BBoundingBox();
         mCopiedUnitDataXML.mLights.Clear();
         mCopiedUnitDataXML.mUnits.Clear();

         for(int i=0;i<mNodes.Count;i++)
         {
            bbox.max = mNodes[i].getDesc().m_max;
            bbox.min = mNodes[i].getDesc().m_min;
            bbox.min.Y = -300.0f;
            bbox.max.Y = 300.0f;

            List<SimEditor.EditorObject> mObjects = SimGlobals.getSimMain().selectItemsAABB(bbox);

            foreach (EditorObject eobj in mObjects)
            {
               if (eobj.GetType() == typeof(SimObject))
               {
                  SimObject obj = eobj as SimObject;
                  if (obj.ProtoObject != null && !mTotalObjects.Contains(eobj))
                  {
                     mCopiedUnitDataXML.mUnits.Add(obj.toXMLData(false));
                     mTotalObjects.Add(eobj);
                  }
               }
            }
            mObjects = null;
         }

         mTotalObjects = null;
      }
      public void cutSelectedData(BTerrainEditor editor)
      {
         copySelectedData(editor);
      }

      public void Preview(BTerrainEditor editor, int xOffset, int zOffset)
      {
         SimGlobals.getSimMain().clipartAddObjects(mCopiedUnitDataXML.mUnits, mCopiedUnitDataXML.mLights);

         mIntersectCenter = TerrainGlobals.getEditor().mBrushIntersectionPoint;

         int worldX;
         int worldZ;

         Vector3 average = Vector3.Empty;
         float minx = int.MaxValue;
         float minz = int.MaxValue;
         foreach (ScenarioSimUnitXML clipboardobj in mCopiedUnitDataXML.mUnits)
         {
            average += clipboardobj.mPosition;

            minx = Math.Min(clipboardobj.mPosition.X, minx);
            minz = Math.Min(clipboardobj.mPosition.Z, minz);

         }
         average *= (1 / (float)mCopiedUnitDataXML.mUnits.Count);

         worldX = (xOffset);// - (mTransSizeX >> 1));
         worldZ = (zOffset);// - (mTransSizeZ >> 1));
          //worldX = (int)(mIntersectCenter.X);// - (mTransSizeX >> 1));
          //worldZ = (int)(mIntersectCenter.Z);// - (mTransSizeZ >> 1));

          //worldX = (int)(mIntersectCenter.X + -(mTransSizeZ >> 1) + average.X);// - (mTransSizeX >> 1));
          //worldZ = (int)(mIntersectCenter.Z + -(mTransSizeX >> 1) + average.Z);// - (mTransSizeZ >> 1));
          //worldX = (int)(xOffset + -(mTransSizeZ >> 1) + ( 0.5 * average.X));// - (mTransSizeX >> 1));
          //worldZ = (int)(zOffset + -(mTransSizeX >> 1) + ( 0.5 * average.Z));// - (mTransSizeZ >> 1));
          //worldX = (int)(xOffset + -(mTransSizeZ >> 1) + 85);// - (mTransSizeX >> 1));
          //worldZ = (int)(zOffset + -(mTransSizeX >> 1) + 75);// - (mTransSizeZ >> 1));

         //bunch of bullshit.
          //worldX = (int)(xOffset + -(mTransSizeZ >> 1) + 1.25 * ( average.X - minx));// - (mTransSizeX >> 1));
          //worldZ = (int)(zOffset + -(mTransSizeX >> 1) + 1.25 * ( average.Z - minz));// - (mTransSizeZ >> 1));

         mIntersectCenter = new Vector3(worldX, 0, worldZ);

         SimGlobals.getSimMain().clipartUpdateObjects(-mTransRotate, mIntersectCenter);
      }

      public override void transformData(float xScale, float yScale, float zScale, float rotAngle)
      {
         mTransScaleX = xScale;
         mTransScaleZ = zScale;
         mTransRotate = rotAngle;

         SimGlobals.getSimMain().clipartAddObjects(mCopiedUnitDataXML.mUnits, mCopiedUnitDataXML.mLights);

         mIntersectCenter = TerrainGlobals.getEditor().mBrushIntersectionPoint;
         SimGlobals.getSimMain().clipartUpdateObjects(mTransRotate, mIntersectCenter);
      }

      

      [XmlRoot("SimObjectData")]
      public class CopiedUnitDataXML : ICloneable
      {
         [XmlArrayItem(ElementName = "Object", Type = typeof(ScenarioSimUnitXML))]
         [XmlArray("Objects")]
         public List<ScenarioSimUnitXML> mUnits = new List<ScenarioSimUnitXML>();

         [XmlArrayItem(ElementName = "Light", Type = typeof(LightXML))]
         [XmlArray("Lights")]
         public List<LightXML> mLights = new List<LightXML>();

         public bool HasData()
         {
            return (mUnits.Count + mLights.Count > 0);
         }

         public object Clone()
         {
            CopiedUnitDataXML newObj = new CopiedUnitDataXML();
    
            foreach(ScenarioSimUnitXML unit in mUnits)
            {
               newObj.mUnits.Add((ScenarioSimUnitXML)unit.Clone());
            }
            foreach(LightXML light in mLights)
            {
               newObj.mLights.Add((LightXML)light);
            }
            return newObj;
         }

      }

   }

   public class CopiedTextureData : ClipboardData
   {
      public Vector3 mIntersectCenter = new Vector3();

      private JaggedContainer<BTerrainTextureVector> mTextureData = new JaggedContainer<BTerrainTextureVector>(MaskFactory.mMaxCapacity);
      private List<string> mActiveTexturesUsed = new List<string>();
      private int mSelectedVertexCount = 0;
      private int mSelectedWidth = 0;
      private int mSelectedHeight = 0;

      //transformed versions
      private JaggedContainer<BTerrainTextureVector> mTransformedTextureData = new JaggedContainer<BTerrainTextureVector>(MaskFactory.mMaxCapacity);
      private int mTransWidth = 0;
      private int mTransHeight = 0;
      private float mTransScaleX = 1.0f;
      private float mTransScaleZ = 1.0f;
      private float mTransRotate = 0.0f;

      private BTileBoundingBox mPostLoadBBox = new BTileBoundingBox();

      public override bool HasData()
      {
         return mTextureData.HasData();
      }
      public override string GetTypeName()
      {
         return "CopiedTextureData";
      }

      public override bool Save(Stream s)
      {
         BinaryWriter b = new BinaryWriter(s);

         b.Write(mSelectedVertexCount);
         b.Write(mSelectedWidth);
         b.Write(mSelectedHeight);
        
         //write the texture names we're using
         b.Write(mActiveTexturesUsed.Count);
         for (int i = 0; i < mActiveTexturesUsed.Count; i++)
            b.Write(mActiveTexturesUsed[i]);

         long Key;
         BTerrainTextureVector layerDat = null;
         mTextureData.ResetIterator();
         int ck = 0;
         while (mTextureData.MoveNext(out Key, out layerDat))
         {
            if (layerDat == null || layerDat.mLayers == null || layerDat.mLayers.Count == 0)
               continue;

            b.Write(Key);
            b.Write(layerDat.mLayers.Count);
            for (int i = 0; i < layerDat.mLayers.Count; i++)
            {
               b.Write(layerDat.mLayers[i].mActiveTextureIndex);
               b.Write((byte)layerDat.mLayers[i].mAlphaContrib);
               b.Write((byte)layerDat.mLayers[i].mLayerType);
            }
            ck++;
            layerDat = null;
         }

         if (ck != mSelectedVertexCount)
            MessageBox.Show("There was a problem loading this clipart with the texture strides");
         b.Close();
         return true;
      }
      public override bool Load(Stream s)
      {
         mTextureData.ResetIterator();
         BinaryReader b = new BinaryReader(s);

         mSelectedVertexCount = b.ReadInt32();
         mSelectedWidth = b.ReadInt32();
         mSelectedHeight = b.ReadInt32();

         //write the texture names we're using
         int numUsed = b.ReadInt32();
         for (int i = 0; i < numUsed; i++)
            mActiveTexturesUsed.Add(b.ReadString());

         loadUsedTextures();

         mTextureData.Clear();
         long Key;


         mPostLoadBBox.empty();
         bool bTextureIndexProblems = false;
         for (int i = 0; i < mSelectedVertexCount; i++)
         {
            Key = b.ReadInt64();
            int numLayers = b.ReadInt32();
            BTerrainTextureVector layerDat = new BTerrainTextureVector();
            layerDat.mLayers = new List<BTerrainPerVertexLayerData>();
            for(int k=0;k<numLayers;k++)
            {
               BTerrainPerVertexLayerData dat = new BTerrainPerVertexLayerData();
               int texIndex = b.ReadInt32();
               if (texIndex < 0 || texIndex >= mActiveTexturesUsed.Count)
               {
                  bTextureIndexProblems = true;
                  continue;  
               }
               //rescale our index into the new set
               dat.mActiveTextureIndex = SimTerrainType.getActiveSetIndex(SimTerrainType.getFromTypeName(mActiveTexturesUsed[texIndex]));
               dat.mAlphaContrib = b.ReadByte();
               dat.mLayerType = (BTerrainTexturingLayer.eLayerType)b.ReadByte();
               layerDat.mLayers.Add(dat);
            }
            layerDat.ensureProperLayerOrdering();
            mTextureData.SetValue(Key,layerDat);
            layerDat = null;

            long x = Key % mSelectedWidth;
            long z = Key / mSelectedWidth;
            mPostLoadBBox.addPoint((int)x, (int)z);
         }

         if (bTextureIndexProblems == true)
         {
            CoreGlobals.getErrorManager().LogErrorToNetwork("### Clipart Load Texture Index Error "  + CoreGlobals.ScenarioFile + "  " );
         }

         b.Close();

         mTransScaleX = 1.0f;
         mTransScaleZ = 1.0f;
         mTransRotate = 0;

         mTransWidth = mSelectedWidth;
         mTransHeight = mSelectedHeight;

       
         transformData(1, 1, 1, 0);

         return true;
      }
      Dictionary<int, int> mTexIndexToActiveIndex = new Dictionary<int, int>();
      private void loadUsedTextures()
      {
         for (int i = 0; i < mActiveTexturesUsed.Count; i++)
         {
            
            TerrainTextureDef ttd = SimTerrainType.getFromTypeName(mActiveTexturesUsed[i]);
            if (ttd == null)
            {
               
               MessageBox.Show("WARNING: Terrain type " + mActiveTexturesUsed[i] + " not found! Replacing with a default type");

               ttd = SimTerrainType.getFromTypeName(SimTerrainType.mBlankTex);
               if (ttd == null)
               {
                  SimTerrainType.addDefaultBlankType();
                  ttd = SimTerrainType.getFromTypeName(SimTerrainType.mBlankTex);
               }
            }
            int ind = SimTerrainType.getIndexFromDef(ttd);

            mTexIndexToActiveIndex[ind] = i;

            if(-1==SimTerrainType.getActiveSetIndex(ttd))
               SimTerrainType.mActiveWorkingSet.Add(ind);
            TerrainGlobals.getTexturing().addActiveTexture(SimTerrainType.getWorkingTexName(ttd));
         }
      }

      public BTileBoundingBox copySelectedData(BTerrainEditor editor)
      {
         mActiveTexturesUsed.Clear();
         mTextureData.Clear();
         mSelectedVertexCount = 0;

         //walk through all the verts in the terrain. If they have mask data, copy them
         int w = TerrainGlobals.getTerrain().getNumXVerts();
         int h = TerrainGlobals.getTerrain().getNumZVerts();

         //we must get our bounds first
         //BTileBoundingBox

         mSelectedWidth = 1 + (Masking.mCurrSelectionMaskExtends.maxX - Masking.mCurrSelectionMaskExtends.minX);
         mSelectedHeight = 1 + (Masking.mCurrSelectionMaskExtends.maxZ - Masking.mCurrSelectionMaskExtends.minZ);

         List<TerrainTextureDef> activeList = SimTerrainType.getDefsOfActiveSet();

         for(int x=0;x<w;x++)
         {
            for(int z=0;z<h;z++)
            {
               float factor=0;
               if(Masking.isPointSelected(x,z,ref factor))
               {
                  BTerrainTextureVector vtd = TerrainGlobals.getEditor().giveTextureDataAtVertex(x,z);
                  if (vtd == null)
                     continue;
                  if (vtd.mLayers.Count == 0)
                     continue;

                  //add any textures to the list that we need.
                  for(int k=0;k<vtd.mLayers.Count;k++)
                  {
                     //don't store decal data ????
                     if(vtd.mLayers[k].mLayerType == BTerrainTexturingLayer.eLayerType.cLayer_Decal)
                     {
                        vtd.mLayers.RemoveAt(k);
                        k--;
                        continue;
                     }

                     string name = activeList[vtd.mLayers[k].mActiveTextureIndex].TypeName;
                     if (!mActiveTexturesUsed.Contains(name))
                        mActiveTexturesUsed.Add(name);

                     vtd.mLayers[k].mActiveTextureIndex = mActiveTexturesUsed.IndexOf(name);
                  }

                  //shift the key value to the origin..
                  int clipX = x - Masking.mCurrSelectionMaskExtends.minX;
                  int clipZ = z - Masking.mCurrSelectionMaskExtends.minZ;
                  int keyValue = clipX + mSelectedWidth * clipZ;
                  mTextureData.SetValue(keyValue, vtd);
                  mSelectedVertexCount++;
               }
            }
         }

   
         return Masking.mCurrSelectionMaskExtends;
      }
      public BTileBoundingBox cutSelectedData(BTerrainEditor editor)
      {
         //copy the data, 
         BTileBoundingBox btb = copySelectedData(editor);

         //then clear all the selected verts
         int w = TerrainGlobals.getTerrain().getNumXVerts();
         int h = TerrainGlobals.getTerrain().getNumZVerts();

         for (int x = 0; x < w; x++)
         {
            for (int z = 0; z < h; z++)
            {
               float factor = 0;
               if (Masking.isPointSelected(x, z, ref factor))
               {
                  TerrainGlobals.getEditor().clearTextureDataAtVertex(x, z);  
               }
            }
         }

         //then call remove unused layers
         List<BTerrainQuadNode> nodes = new List<BTerrainQuadNode>();
         TerrainGlobals.getTerrain().getQuadNodeRoot().getTileBoundsIntersection(nodes, btb.minX, btb.maxX, btb.minZ, btb.maxZ);
         for (int i = 0; i < nodes.Count;i++ )
            nodes[i].mLayerContainer.removeBlankLayers();

         return btb;
      }
      public int mTransSizeX = 0;
      public int mTransSizeZ = 0;

      public BTileBoundingBox Preview(BTerrainEditor editor, int xOffset, int zOffset)
      { 
         int xDiff = mSelectedWidth - mTransSizeZ;
         int zDiff = mSelectedHeight - mTransSizeX;
         int halfX = (mTransSizeZ >> 1);
         int halfZ = (mTransSizeX >> 1);
         BTileBoundingBox bounds = new BTileBoundingBox();
         long x;
         long z;
         long Key;
         long destinationID;
         BTerrainTextureVector layerDat = null;
         BTerrainTextureVector vec = new BTerrainTextureVector();
         mTransformedTextureData.ResetIterator();
         while (mTransformedTextureData.MoveNext(out Key, out layerDat))
         {
            if (layerDat == null || layerDat.mLayers == null || layerDat.mLayers.Count == 0)
               continue;

            int sizeX = TerrainGlobals.getTerrain().getNumXVerts();
            int sizeZ = TerrainGlobals.getTerrain().getNumZVerts();


            x = Key % mTransWidth;
            z = Key / mTransWidth;
            x -= mPostLoadBBox.minX;
            z -= mPostLoadBBox.minZ;


            int worldX = (int)(x + (xOffset - halfX));
            int worldZ = (int)(z + (zOffset - halfZ));

             //worldX = (int)(z + (xOffset - (mTransWidth >> 1)));
             //worldZ = (int)(x + (zOffset - (mTransHeight >> 1)));

            if (worldX < 0 || worldZ < 0 || worldX >= sizeX || worldZ >= sizeZ)
               continue;

            bounds.addPoint(worldX, worldZ);

            float selectionWeight = TerrainGlobals.getTerrain().getSoftSelectionWeight((int)worldX, (int)worldZ);
            if (selectionWeight == 0.0f)
               continue;

            destinationID = worldX + TerrainGlobals.getTerrain().getNumZVerts() * worldZ;

            //merge with the world that's out there..
           // TerrainGlobals.getTexturing().getTextureVector(worldX, worldZ).copyTo(vec);
            //vec.mergeWith(layerDat, false);

            editor.getCurrBrushDeformationTextures().SetValue(destinationID, layerDat);

            layerDat = null;
         }
         layerDat = null;
         vec = null;

         return bounds;
      }

      public int getVertCount()
      {
         return mSelectedVertexCount;
      }

      public override void transformData(float xScale, float yScale, float zScale, float rotAngle)
      {
         
         mTransScaleX = xScale;
         mTransScaleZ = zScale;
         mTransRotate = rotAngle;

         ImageManipulation.eFilterType FilterType = ImageManipulation.eFilterType.cFilter_Linear;

         //we need to extract, resize, and rotate each textureing channel we have.
         BTerrainTextureVector layerProto = new BTerrainTextureVector();

         long Key;
         BTerrainTextureVector layerDat = null;
         mTextureData.ResetIterator();
         while (mTextureData.MoveNext(out Key, out layerDat))
         {
            if (layerDat == null || layerDat.mLayers == null || layerDat.mLayers.Count == 0)
               continue;

            layerProto.mergeWith(layerDat,true);
         }

         mTransformedTextureData.Clear();
         mTransformedTextureData.SetEmptyValue(null);

         long x;
         long z;

         for (int i = 0; i < layerProto.getNumLayers(); i++)
         {
            byte[] extDat = new byte[mSelectedWidth * mSelectedHeight];

            //extract
            mTextureData.ResetIterator();
            while (mTextureData.MoveNext(out Key, out layerDat))
            {
               if (layerDat == null || layerDat.mLayers == null || layerDat.mLayers.Count == 0)
                  continue;

               int idx = layerDat.giveLayerIndex(layerProto.mLayers[i].mActiveTextureIndex, layerProto.mLayers[i].mLayerType);
               if(idx != -1)
               {
                  x = Key / mSelectedWidth;
                  z = Key - x * mSelectedWidth;
                  extDat[z + x * mSelectedWidth] = layerDat.mLayers[idx].mAlphaContrib;
               }
            }

            //scale
            int tW = (int)(mTransScaleX * mSelectedWidth);
            int tH = (int)(mTransScaleZ * mSelectedHeight);
            byte[] imgScaledX = ImageManipulation.resizeGreyScaleImg(extDat, mSelectedWidth, mSelectedHeight, tW, tH, FilterType);

            //rotate
            int nW = tW;
            int nH = tH;
            float nRot = (float)(mTransRotate * (180.0f / Math.PI));
            byte[] imgRotatedX = ImageManipulation.rotateGreyScaleImg(imgScaledX, tW, tH, nRot, false, out nW, out nH, FilterType);

            mTransWidth = nW;
            mTransHeight = nH;

            //pass 
            for (int k = 0; k < nW; k++)
            {
               for (int j = 0; j < nH; j++)
               {
                  //Mem problems:??

                  //BTerrainTextureVector v = new BTerrainTextureVector();
                  //byte value = imgRotatedX[k + nW * j];

                  //int ttdIndex = j * nW + k;
                  //BTerrainTextureVector od = mTransformedTextureData.GetValue(ttdIndex);
                  //if (od != null)
                  //   od.copyTo(v);

                  //v.addValueToTexID(layerProto.mLayers[i].mActiveTextureIndex, value, layerProto.mLayers[i].mLayerType);

                  //mTransformedTextureData.SetValue(ttdIndex, v);

                  //Fixed memory
                  byte value = imgRotatedX[k + nW * j];
                  if (value == 0)
                     continue;
                  int ttdIndex = j * nW + k;
                  BTerrainTextureVector od = mTransformedTextureData.GetValue(ttdIndex);
                  if (od == null)
                  {
                     od = new BTerrainTextureVector();
                  }
                  //od.addValueToTexID(layerProto.mLayers[i].mActiveTextureIndex, value, layerProto.mLayers[i].mLayerType);
                  od.initLayer(i,layerProto.mLayers[i].mActiveTextureIndex, value, layerProto.mLayers[i].mLayerType);
                  mTransformedTextureData.SetValue(ttdIndex, od);




                  ////Fixed memory 2?
                  //byte value = imgRotatedX[k + nW * j];
                  ////if (value == 0)
                  ////   continue;
                  //int ttdIndex = j * nW + k;
                  //BTerrainTextureVector od = null;
                  //if (mTransformedTextureData..(ttdIndex) == false)
                  //{
                  //   od = new BTerrainTextureVector();
                  //   mTransformedTextureData.SetValue(ttdIndex, od);
                  //}
                  //else
                  //{
                  //   od = mTransformedTextureData.GetValue(ttdIndex);
                  //}
                  //od.addValueToTexID(layerProto.mLayers[i].mActiveTextureIndex, value, layerProto.mLayers[i].mLayerType);
                  ////mTransformedTextureData.SetValue(ttdIndex, od);
                  
               }
            }
         }

      }
   }

   public class CopiedVertexData : ClipboardData
   {
      private JaggedContainer<float> mOrigSelection = new JaggedContainer<float>(MaskFactory.mMaxCapacity);
      private JaggedContainer<Vector3> mOrigDetail = new JaggedContainer<Vector3>(MaskFactory.mMaxCapacity);
      private int mOrigStride = 0;
      private int mOrigMidX = 0;
      private int mOrigMidZ = 0;

      private JaggedContainer<float> mTransSelection = new JaggedContainer<float>(MaskFactory.mMaxCapacity);
      private JaggedContainer<Vector3> mTransDetail = new JaggedContainer<Vector3>(MaskFactory.mMaxCapacity);
      private int mTransStride = 0;
      private int mTransMidX = 0;
      private int mTransMidZ = 0;
      public int mTransSizeX = 0;
      public int mTransSizeZ = 0;

      private float mLowestPoint = float.MaxValue;
      private float mHighestPoint = float.MinValue;

      private int mOrigSizeX = 0;
      private int mOrigSizeZ = 0;

      private float mTransScaleX = 1.0f;
      private float mTransScaleY = 1.0f;
      private float mTransScaleZ = 1.0f;
      private float mTransRotate = 0.0f;

      public void GetOriginalSize(out Vector3 min, out Vector3 max)
      {
         //min = new Vector3(mOrigSizeX - mOrigMidX, mLowestPoint, mOrigSizeZ - mOrigMidZ);
         //max = new Vector3(mOrigSizeX - (mOrigSizeX - mOrigMidX), mHighestPoint, mOrigSizeZ - (mOrigSizeZ - mOrigMidZ));

         if (mLowestPoint == int.MaxValue || mHighestPoint == int.MinValue)
         {

         }

         min = new Vector3(-mOrigSizeX / 4, mLowestPoint, -mOrigSizeZ / 4);
         max = new Vector3(mOrigSizeX / 4, mHighestPoint, mOrigSizeZ / 4);


      }


      public int mCount = 0;
      public override bool  HasData()
      {
         if (mOrigSelection.HasData() && mOrigDetail.HasData())
          {
             return true;
             
          }
          return false;
      }
      public override string GetTypeName()
      {
         return "CopiedVertexData";
      }

      public override bool Save(Stream s)
      {
         //FileStream f = new FileStream("clip.out", FileMode.Create);
         BinaryWriter b = new BinaryWriter(s);

         int count = mCount;// mSelection.Count;
         b.Write(count);
         b.Write(mOrigStride);
         b.Write(mOrigMidX);
         b.Write(mOrigMidZ);
         b.Write(mLowestPoint);
         long Key;
         float maskValue;
         mOrigSelection.ResetIterator();
         while (mOrigSelection.MoveNext(out Key, out maskValue))
         {
            if (maskValue == 0) 
               continue;
            b.Write(Key);
            b.Write(maskValue);
         }
         //Vector3 Value;
         //mDetail.ResetIterator();
         //while (mDetail.MoveNext(out Key, out Value))
         //{
         //   b.Write(Key);
         //   b.Write(Value.X);
         //   b.Write(Value.Y);
         //   b.Write(Value.Z);
         //}

         Vector3 Value;
         mOrigSelection.ResetIterator();
         while (mOrigSelection.MoveNext(out Key, out maskValue))
         {
            if (maskValue == 0)
               continue;

            b.Write(Key);
            Value = mOrigDetail.GetValue(Key);
            b.Write(Value.X);
            b.Write(Value.Y);
            b.Write(Value.Z);
         }
         b.Close();
         return true;
      }
      public override bool Load(Stream s)
      {
         //FileStream f = new FileStream("clip.out", FileMode.Open);
         BinaryReader b = new BinaryReader(s);

         int count = b.ReadInt32();
         mCount = count;
         mOrigStride = b.ReadInt32();
         mOrigMidX = b.ReadInt32();
         mOrigMidZ = b.ReadInt32();
         mLowestPoint = b.ReadSingle();

         mOrigSelection.Clear();
         for (int i = 0; i < count; i++)
         {
            long key = b.ReadInt64();
            float alpha = b.ReadSingle();
            mOrigSelection.SetValue(key, alpha);
         }
         mOrigDetail.Clear();
         for (int i = 0; i < count; i++)
         {
            long key = b.ReadInt64();
            float x = b.ReadSingle();
            float y = b.ReadSingle();
            float z = b.ReadSingle();
            mOrigDetail.SetValue(key, new Vector3(x, y, z));


            if (mLowestPoint > y)
               mLowestPoint = y;

            if (mHighestPoint < y)
               mHighestPoint = y;
         }

         b.Close();

         computeSize();

         mTransScaleX = 1.0f;
         mTransScaleY = 1.0f;
         mTransScaleZ = 1.0f;
         mTransRotate = 0;
         mTransMidX = mOrigMidX;
         mTransMidZ = mOrigMidZ;
         mTransSizeZ = mOrigSizeZ;
         mTransSizeX = mOrigSizeX;
         mTransStride = mOrigStride;
         transformData(1, 1, 1, 0);
         return true;
      }
      public int getVertCount()
      {
         return mCount;
      }
      public void MakeCopy(BTerrainEditor editor)
      {


         //Load(); return;
         mCount = 0;

         mOrigStride = TerrainGlobals.getTerrain().getNumZVerts();
         mOrigSelection.Clear();
         mOrigDetail.Clear();

         long x;
         long z;

         long totalx = 0;
         long totalz = 0;
         mLowestPoint = float.MaxValue;

         long min = 0;
         long max = mOrigStride * mOrigStride;
         try
         {

            long id;
            float value;
            Masking.getCurrSelectionMaskWeights().ResetIterator();
            while (Masking.getCurrSelectionMaskWeights().MoveNext(out id, out value))
            {
               if (value == 0)
                  continue;
               if (id > max || id < min)
                  continue;

               mCount++;
               x = id / mOrigStride;
               z = id - x * mOrigStride;
               totalx += x;
               totalz += z;

               mOrigSelection.SetValue(id, value);
               Vector3 detail = (Vector3)TerrainGlobals.getEditor().getDetailPoints().GetValue(id);

               if (mLowestPoint > detail.Y)
                  mLowestPoint = detail.Y;

               if (mHighestPoint < detail.Y)
                  mHighestPoint = detail.Y;

               mOrigDetail.SetValue(id, detail);
            }
            mOrigMidX = (int)(totalx / (float)mCount);//mDetail.Keys.Count);
            mOrigMidZ = (int)(totalz / (float)mCount);//mDetail.Keys.Count);

            // compute size
            computeSize();

            //Save();

         }
         catch(OutOfMemoryException ex)
         {
            CoreGlobals.getErrorManager().SendToErrorWarningViewer("Out of memory when copying terrain!");

            mOrigMidX = (int)(totalx / (float)mCount);
            mOrigMidZ = (int)(totalz / (float)mCount);

            // compute size
            computeSize();
         }

      }

      public BTileBoundingBox DirectCut(BTerrainEditor editor, bool bUpperCut)
      {
         BTileBoundingBox bounds = new BTileBoundingBox();

         long x;
         long z;
         Vector3 point;

         float referencepoint = (bUpperCut) ? mHighestPoint  : mLowestPoint;

         Vector3 temp = new Vector3(0, 0, 0);

         try
         {
            long id;
            float maskValue;
            mOrigSelection.ResetIterator();
            while (mOrigSelection.MoveNext(out id, out maskValue))
            {
               if (maskValue == 0) 
                  continue;

               x = id / mOrigStride;
               z = id - x * mOrigStride;
               bounds.addPoint((int)x, (int)z);

               point = editor.getDetailPoints()[id];


               //if (bUpperCut)
               //{
               //   //editor.getDetailPoints()[id] = new Vector3(point.X, referencepoint * maskValue + point.Y * (1 - maskValue), point.Z);

               //   temp.X = point.X;
               //   temp.Y = referencepoint * maskValue + point.Y * (1 - maskValue);
               //   temp.Z = point.Z;
               //   editor.getDetailPoints()[id] = temp;
               //}
               //else
               {
                  //editor.getDetailPoints()[id] = new Vector3(point.X, referencepoint * maskValue + point.Y * (1 - maskValue), point.Z);
                  temp.X = point.X;
                  temp.Y = referencepoint * maskValue + point.Y * (1 - maskValue);
                  temp.Z = point.Z;
                  editor.getDetailPoints()[id] = temp;
               }

               //getCurrBrushDeformations
            }
         }
         catch(System.Exception ex)
         {
            CoreGlobals.getErrorManager().SendToErrorWarningViewer("error copying data");

         }

         return bounds;
      }

   

      float mHeightModifier = 0;
      public void SetHeightModifier(float modifier)
      {
         mHeightModifier = 60 * (modifier - 0.5f);

      }

      public override void transformData(float xScale, float yScale, float zScale, float rotAngle)
      {
         /*
         mTransDetail.Clear();
         mTransSelection.Clear();
         long tid;
         float tmaskValue;
            mOrigSelection.ResetIterator();
            while (mOrigSelection.MoveNext(out tid, out tmaskValue))
            {
               if (tmaskValue == 0)
                  continue;

               mTransDetail.SetValue(tid, mOrigDetail.GetValue(tid));
               mTransSelection.SetValue(tid, tmaskValue);

            }

            return;
         */
         //mTransScaleX += xScale;
         //mTransScaleY += zScale;
         //mTransRotate += rotAngle;

         mTransScaleX = xScale;
         mTransScaleY = yScale;
         mTransScaleZ = zScale;
         mTransRotate = rotAngle;


         ImageManipulation.eFilterType FilterType = ImageManipulation.eFilterType.cFilter_Linear;

         //convert our data to greyscale
         int tX = mOrigSizeX + 1;
         int tZ = mOrigSizeZ + 1;
         float[] tDatX = new float[tX * tZ];
         float[] tDatY = new float[tX * tZ];
         float[] tDatZ = new float[tX * tZ];
         float[] tDatM = new float[tX * tZ];

         for (int i = 0; i < tX * tZ; i++)
         {
            tDatM[i] = 0;
            tDatX[i] = 0;
            tDatY[i] = 0;
            tDatZ[i] = 0;  
         }

         long x = 0;
         long z = 0;
         long id;
         float maskValue;

         //get bound for scaling
         BTileBoundingBox bounds = new BTileBoundingBox();
         mOrigSelection.ResetIterator();
         while (mOrigSelection.MoveNext(out id, out maskValue))
         {
             if (maskValue == 0)
                continue;
             //THE ID THAT WE GET BACK IS IN MULTIPLES OF THE ORIGIONAL DATA MAP SIZE
             x = id / mOrigStride;
             z = id - x * mOrigStride;
             bounds.addPoint((int)x, (int)z);
          }


         mOrigSelection.ResetIterator();
         while (mOrigSelection.MoveNext(out id, out maskValue))
         {
            if (maskValue == 0)
               continue;
            //THE ID THAT WE GET BACK IS IN MULTIPLES OF THE ORIGIONAL DATA MAP SIZE
            x = id / mOrigStride;
            z = id - x * mOrigStride;

            int kX = (int)(x - bounds.minX);
            int kZ = (int)(z - bounds.minZ);

            if (kX > mOrigSizeX || kZ > mOrigSizeZ || kX < 0 || kZ < 0)
               id--;

            int kID = (int)(kX + tX * kZ);
            Vector3 val = mOrigDetail.GetValue(id);
            tDatX[kID] = val.X;
            tDatY[kID] = val.Y;
            tDatZ[kID] = val.Z;
            tDatM[kID] = mOrigSelection.GetValue(id); 
         }

         //do transforms
         int tW = (int)(mTransScaleX * mOrigSizeX);
         int tH = (int)(mTransScaleZ * mOrigSizeZ);
         float[] imgScaledX = ImageManipulation.resizeF32Img(tDatX, tX, tZ, tW, tH, FilterType);
         float[] imgScaledY = ImageManipulation.resizeF32Img(tDatY, tX, tZ, tW, tH, FilterType);
         float[] imgScaledZ = ImageManipulation.resizeF32Img(tDatZ, tX, tZ, tW, tH, FilterType);
         float[] imgScaledM = ImageManipulation.resizeF32Img(tDatM, tX, tZ, tW, tH, FilterType);

         int nW = tW;   
         int nH = tH;
         float nRot = (float)(mTransRotate * (180.0f / Math.PI));
         float[] imgRotatedX = ImageManipulation.rotateF32Img(imgScaledX, tW, tH, nRot, false, out nW, out nH, FilterType);
         float[] imgRotatedY = ImageManipulation.rotateF32Img(imgScaledY, tW, tH, nRot, false, out nW, out nH, FilterType);
         float[] imgRotatedZ = ImageManipulation.rotateF32Img(imgScaledZ, tW, tH, nRot, false, out nW, out nH, FilterType);
         float[] imgRotatedM = ImageManipulation.rotateF32Img(imgScaledM, tW, tH, nRot, false, out nW, out nH, FilterType);

         mTransDetail.Clear();
         mTransSelection.Clear();
         //convert back to floating point
         int count = 0;
         int xCount = 0;
         int yCount = 0;
         Matrix rotMat = Matrix.RotationY(mTransRotate);
         for (int i = 0; i < nW; i++)
         {
            for (int j = 0; j < nH; j++)
            {
               int srcIndex = i + nW * j;
               Vector3 v = new Vector3(imgRotatedX[srcIndex], imgRotatedY[srcIndex], imgRotatedZ[srcIndex]);
               float mVal = imgRotatedM[srcIndex];
               v.TransformCoordinate(rotMat);
               //BMathLib.vec4Transform
               //if (TerrainGlobals.getEditor().mTransformClipartValues)
               {
                  //v *= mTransScaleY;
                  //v.X *= mTransScaleX;
                  //v.X *= mTransScaleX;
                  //v.Z *= mTransScaleZ;
               }
               v.X *= mTransScaleX;
               v.Y *= mTransScaleY;
               v.Z *= mTransScaleZ;

               //transpose the values here.
               srcIndex = j + nH * i;
               mTransDetail.SetValue(srcIndex, v);
               mTransSelection.SetValue(srcIndex, mVal);
            }
         }
         //transpose the values here.
         mTransStride = nH;
         mTransSizeX = nH;
         mTransSizeZ = nW;

         imgScaledX = null;
         imgScaledY = null;
         imgScaledZ = null;
         imgScaledM = null;
         imgRotatedX = null;
         imgRotatedY = null;
         imgRotatedZ = null;
         imgRotatedM = null;

      }

      public BTileBoundingBox Preview(BTerrainEditor editor, int xOffset, int zOffset, float yOffset, int rotateCount, BTerrainFrontend.ePasteOperation op)//)
      {
        
         bool bReplaceMode = false;
         bool bMinPlane = false;

         if(op == BTerrainFrontend.ePasteOperation.Add || op == BTerrainFrontend.ePasteOperation.AddYOnly)
         {
            bReplaceMode = false;
            bMinPlane = false;
         }
         else if(op == BTerrainFrontend.ePasteOperation.Replace)
         {
            bReplaceMode = true;
            bMinPlane = false;
         }
         else if (op == BTerrainFrontend.ePasteOperation.Merge)
         {
            bReplaceMode = true;
            bMinPlane = true;
         }



         long x;
         long z;
         long destinationID;
         long id;

         float maskValue;
         //get our bounding box
         BTileBoundingBox bounds = new BTileBoundingBox();

         int sizeX = TerrainGlobals.getTerrain().getNumXVerts();
         int sizeZ = TerrainGlobals.getTerrain().getNumZVerts();

         
         mTransSelection.ResetIterator();
         while (mTransSelection.MoveNext(out id, out maskValue))
         {
            if (maskValue == 0) 
               continue;
            x = id / mTransStride;
            z = id - x * mTransStride;
            Vector3 displacement = mTransDetail.GetValue(id);
            

            //convert coordinates to the world
            int kX = (int)(x );//- bounds.minX);
            int kZ = (int)(z );//- bounds.minZ);

            int worldX = (int)(kX + (xOffset - (mTransSizeZ >> 1)));
            int worldZ = (int)(kZ + (zOffset - (mTransSizeX >> 1)));
            if (worldX < 0 || worldZ < 0 || worldX >= sizeX || worldZ >= sizeZ)
               continue;

            destinationID = worldX * sizeZ + worldZ;


            bounds.addPoint((int)worldX, (int)worldZ);
            float selectionWeight = TerrainGlobals.getTerrain().getSoftSelectionWeight((int)worldX, (int)worldZ);
            if (selectionWeight == 0.0f)
               continue;

            Vector3 currPos = new Vector3(0, 0, 0);
            Vector3 final = new Vector3(0, 0, 0);

            //Vector3 v = new Vector3(0, /*mLowestPoint -*/ mHeightModifier, 0);
            Vector3 v = new Vector3(0, /*mLowestPoint -*/ -yOffset, 0);
            
            switch(op)
            {
               case BTerrainFrontend.ePasteOperation.Add:
                  Vector3 normal = TerrainGlobals.getTerrain().getNormal((int)worldX, (int)worldZ);
                  Matrix matRotation = Matrix.LookAtLH(new Vector3(0.0f, 0.0f, 0.0f), new Vector3(normal.X, normal.Z, normal.Y), new Vector3(0.0f, 1.0f, 0.0f));
                  displacement.TransformCoordinate(matRotation);                  
                  final = (maskValue * (displacement - v) - (currPos * (maskValue))) * selectionWeight;
                  break;
               case BTerrainFrontend.ePasteOperation.AddYOnly:
                  //Vector3 normal = TerrainGlobals.getTerrain().getNormal((int)worldX, (int)worldZ);
                  //Matrix matRotation = Matrix.LookAtLH(new Vector3(0.0f, 0.0f, 0.0f), new Vector3(normal.X, normal.Z, normal.Y), new Vector3(0.0f, 1.0f, 0.0f));
                  //displacement.TransformCoordinate(matRotation);
                  final = (maskValue * (displacement - v) - (currPos * (maskValue))) * selectionWeight;
                  break;
               case BTerrainFrontend.ePasteOperation.Replace:                 
                  currPos = TerrainGlobals.getTerrain().getRelPos((int)worldX, (int)worldZ);
                  final = (maskValue * (displacement - v) - (currPos * (maskValue))) * selectionWeight;                
                  break;
               case BTerrainFrontend.ePasteOperation.Merge:
                  currPos = TerrainGlobals.getTerrain().getRelPos((int)worldX, (int)worldZ);
                  final = (maskValue * (displacement - v) - (currPos * (maskValue))) * selectionWeight;                  
                  break;

            };

            if (bMinPlane && final.Y + currPos.Y < currPos.Y)
            {
               continue;
            }

            editor.getCurrBrushDeformations().SetValue(destinationID, final);

         }


         return bounds;

      }

      public BTileBoundingBox Preview(BTerrainEditor editor, int xOffset, int zOffset, bool alternate)
      {
         long x;
         long z;
         long destinationID;

         BTileBoundingBox bounds = new BTileBoundingBox();

         int sizeX = TerrainGlobals.getTerrain().getNumXVerts();
         int sizeZ = TerrainGlobals.getTerrain().getNumZVerts();


         xOffset -= mOrigMidX;
         zOffset -= mOrigMidX;

         long id;
         float maskValue;
         mTransSelection.ResetIterator();
         while (mTransSelection.MoveNext(out id, out maskValue))
         {
            if (maskValue == 0) 
               continue;
            x = id / mTransStride;
            z = id - x * mTransStride;

            x += xOffset;
            z += zOffset;

            if (x < 0 || z < 0 || x >= sizeX || z >= sizeZ)
               continue;

            float selectionWeight = TerrainGlobals.getTerrain().getSoftSelectionWeight((int)x, (int)z);
            if (selectionWeight == 0.0f)
               continue;
            
            bounds.addPoint((int)x, (int)z);
            destinationID = x * sizeZ + z;

            Vector3 currPos = new Vector3(0, 0, 0);
            Vector3 displacement = mTransDetail.GetValue(id); //mDetail[id];

            if (alternate)
            {
               currPos = TerrainGlobals.getTerrain().getRelPos((int)x, (int)z);
            }
            else
            {
               Vector3 normal = TerrainGlobals.getTerrain().getNormal((int)x, (int)z);
               Matrix matRotation = Matrix.LookAtLH(new Vector3(0.0f, 0.0f, 0.0f), new Vector3(normal.X, normal.Z, normal.Y), new Vector3(0.0f, 1.0f, 0.0f));

               displacement.TransformCoordinate(matRotation);
            }


            Vector3 v = new Vector3(0, mLowestPoint, 0);
            editor.getCurrBrushDeformations().SetValue(destinationID, (maskValue * (displacement - v) - currPos) * selectionWeight);
         }
         return bounds;
      }

      public BTileBoundingBox getBounds(int xPos, int zPos)
      {
         BTileBoundingBox bounds = new BTileBoundingBox();

         int minX = xPos - (int)(mOrigSizeX / 2.0f);
         if(minX < 0)
            minX = 0;

         int maxX = xPos + (int)(mOrigSizeX / 2.0f);
         if(maxX >= TerrainGlobals.getTerrain().getNumXVerts())
            maxX = TerrainGlobals.getTerrain().getNumXVerts() - 1;

         int minZ = zPos - (int)(mOrigSizeZ / 2.0f);
         if(minZ < 0)
            minZ = 0;

         int maxZ = zPos + (int)(mOrigSizeZ / 2.0f);
         if(maxZ >= TerrainGlobals.getTerrain().getNumZVerts())
            maxZ = TerrainGlobals.getTerrain().getNumZVerts() - 1;

         bounds.minX = minX;
         bounds.maxX = maxX;
         bounds.minZ = minZ;
         bounds.maxZ = maxZ;

         return bounds;
      }

      private void computeSize()
      {
         // compute size
         BTileBoundingBox bounds = new BTileBoundingBox();
         long x_index;
         long z_index;

         long id;
         float maskValue;
         mOrigSelection.ResetIterator();
         while (mOrigSelection.MoveNext(out id, out maskValue))
         {
            if (maskValue == 0) 
               continue;
            x_index = id / mOrigStride;
            z_index = id - x_index * mOrigStride;

            bounds.addPoint((int)x_index, (int)z_index);
         }

         mOrigSizeX = 1+(bounds.maxX - bounds.minX);
         mOrigSizeZ = 1+(bounds.maxZ - bounds.minZ);
      }

   }


   public class ObjExporter
   {
      public void writeAll(string filename)
      {
         int minX = 0;
         int minZ = 0;
         int maxX = TerrainGlobals.getTerrain().getNumXVerts();
         int maxZ = TerrainGlobals.getTerrain().getNumZVerts();

         write(filename,minX, minZ, maxX, maxZ);
      }

      public void writeSelection(string filename)
      {
         // Check if there is something selected
         if (Masking.mCurrSelectionMaskExtends.isEmpty())
         {
            if (MessageBox.Show("Nothing to export since nothing is selected.  Please mask select the are you would like to export.", "Nothing Selected!", MessageBoxButtons.OK) == DialogResult.OK)
            {
               return;
            }
         }

         int minX = Masking.mCurrSelectionMaskExtends.minX;
         int minZ = Masking.mCurrSelectionMaskExtends.minZ;
         int maxX = Masking.mCurrSelectionMaskExtends.maxX;
         int maxZ = Masking.mCurrSelectionMaskExtends.maxZ;

         write(filename,minX, minZ, maxX, maxZ);
      }

      private void write(string filename, int minX, int minZ, int maxX, int maxZ)
      {
         //SaveFileDialog save = new SaveFileDialog();
         //save.Filter = ".obj files (*.obj)|*.obj";
         //save.ShowDialog();

         if (filename != "")
         {
            if (File.Exists(filename))
               File.Delete(filename);

            OBJFile output = new OBJFile();
            output.addObject("Terrain");

            for (int x = minX; x <= maxX; x++)
            {
               for (int z = minZ; z <= maxZ; z++)
               {
                  Vector3 detailA = TerrainGlobals.getTerrain().getPostDeformPos(x, z);
                  Vector3 detailB = TerrainGlobals.getTerrain().getPostDeformPos(x + 1, z + 1);
                  Vector3 detailC = TerrainGlobals.getTerrain().getPostDeformPos(x, z + 1);

                  output.addTriangle(0, detailA, detailB, detailC);

                  detailA = TerrainGlobals.getTerrain().getPostDeformPos(x, z);
                  detailB = TerrainGlobals.getTerrain().getPostDeformPos(x + 1, z);
                  detailC = TerrainGlobals.getTerrain().getPostDeformPos(x + 1, z + 1);
              
                  output.addTriangle(0, detailA, detailB, detailC);
               }

            }
            output.write(new FileStream(filename, FileMode.OpenOrCreate));
            Cursor.Current = Cursors.Default;
         }
      }
   }
}
