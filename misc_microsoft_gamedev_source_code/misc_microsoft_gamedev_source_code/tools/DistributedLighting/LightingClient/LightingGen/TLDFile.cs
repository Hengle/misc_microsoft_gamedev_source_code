using System;
using System.Collections.Generic;
using System.IO;
using System.Xml;
using System.Xml.Serialization;

using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;

using Xceed.FileSystem;
using Xceed.Compression;
using Xceed.Zip;

using EditorCore;
using Rendering;
using Sim;

namespace LightingClient
{

   public partial class LightingClientMain
   {
      public bool loadTempDataFile(string mFileName)
      {
         try
         {
            DiskFile zipFile = new DiskFile(mFileName);
            if (!zipFile.Exists)
            {
               Console.ForegroundColor = ConsoleColor.Red;
               Console.WriteLine("..Could not load source Disk file " + mFileName);
               Console.ForegroundColor = ConsoleColor.White;

               return false;
            }
            ZipArchive zip = new ZipArchive(zipFile);

            if (!loadTerrainDat(zip))
            {
               Console.ForegroundColor = ConsoleColor.Red;
               Console.WriteLine("..Could not load terrain data from source file " + mFileName);
               Console.ForegroundColor = ConsoleColor.White;
               return false;
            }

            if (!loadModelsSet(zip))
            {
               Console.ForegroundColor = ConsoleColor.Red;
               Console.WriteLine("..Could not load model data (or it does not exist) in source file " + mFileName);
               Console.ForegroundColor = ConsoleColor.White;
               //return ;
            }
         }
         catch(Exception e)
         {
            return false;
         }

         return true;
      }
      bool loadTerrainDat(ZipArchive zip)
      {
         AbstractFile file = zip.GetFile("terrain.TDL");
         if(!file.Exists)
            return false;

         try
         {
            Stream stream = file.OpenRead();
            BinaryReader br = new BinaryReader(stream);

            TLDHeader header = new TLDHeader();
            header.Version = br.ReadInt32();

            //TERRAIN DATA

            TerrainGlobals.getTerrain().mNumXVerts = br.ReadUInt32();
            TerrainGlobals.getTerrain().mNumZVerts = br.ReadUInt32();
            TerrainGlobals.getTerrain().mTileScale = br.ReadSingle();

            TerrainGlobals.getTerrain().mTerrainBBMin = new Vector3();
            TerrainGlobals.getTerrain().mTerrainBBMin.X = br.ReadSingle();
            TerrainGlobals.getTerrain().mTerrainBBMin.Y = br.ReadSingle();
            TerrainGlobals.getTerrain().mTerrainBBMin.Z = br.ReadSingle();

            TerrainGlobals.getTerrain().mTerrainBBMax = new Vector3();
            TerrainGlobals.getTerrain().mTerrainBBMax.X = br.ReadSingle();
            TerrainGlobals.getTerrain().mTerrainBBMax.Y = br.ReadSingle();
            TerrainGlobals.getTerrain().mTerrainBBMax.Z = br.ReadSingle();

            TerrainGlobals.getTerrain().mTerrainRelativePositions = new Vector3[TerrainGlobals.getTerrain().mNumXVerts * TerrainGlobals.getTerrain().mNumZVerts];
            TerrainGlobals.getTerrain().mTerrainNormals = new Vector3[TerrainGlobals.getTerrain().mNumXVerts * TerrainGlobals.getTerrain().mNumZVerts];
            TerrainGlobals.getTerrain().mTerrainAOVals = new float[TerrainGlobals.getTerrain().mNumXVerts * TerrainGlobals.getTerrain().mNumZVerts];

            //start reading terrain data
            for (int i = 0; i < TerrainGlobals.getTerrain().mNumXVerts * TerrainGlobals.getTerrain().mNumZVerts; i++)
            {
               TerrainGlobals.getTerrain().mTerrainRelativePositions[i] = new Vector3();
               TerrainGlobals.getTerrain().mTerrainRelativePositions[i].X = br.ReadSingle();
               TerrainGlobals.getTerrain().mTerrainRelativePositions[i].Y = br.ReadSingle();
               TerrainGlobals.getTerrain().mTerrainRelativePositions[i].Z = br.ReadSingle();
            }

            for (int i = 0; i < TerrainGlobals.getTerrain().mNumXVerts * TerrainGlobals.getTerrain().mNumZVerts; i++)
            {
               TerrainGlobals.getTerrain().mTerrainNormals[i] = new Vector3();
               TerrainGlobals.getTerrain().mTerrainNormals[i].X = br.ReadSingle();
               TerrainGlobals.getTerrain().mTerrainNormals[i].Y = br.ReadSingle();
               TerrainGlobals.getTerrain().mTerrainNormals[i].Z = br.ReadSingle();
            }


            //Read our quadNode Descriptions
            int numQuadNodes = br.ReadInt32();
            TerrainGlobals.getTerrain().mQuadNodeDescArray = new BTerrainQuadNodeDesc[numQuadNodes];
            for (int i = 0; i < numQuadNodes; i++)
            {
               TerrainGlobals.getTerrain().mQuadNodeDescArray[i] = new BTerrainQuadNodeDesc();
               TerrainGlobals.getTerrain().mQuadNodeDescArray[i].mMinXVert = br.ReadInt32();
               TerrainGlobals.getTerrain().mQuadNodeDescArray[i].mMinZVert = br.ReadInt32();
            }

            br.Close();
            stream.Close();
         }
         catch(Exception e)
         {
            Console.WriteLine("TDL loading FAILED!!!");
            return false;
         }
         return true;
      }



      public class ObjectInstanceXML
      {
         [XmlIgnore]
         private Matrix mOrientation;
         public void setOrientation(Matrix m)
         {
            mOrientation = m;
         }
         public Matrix getMatrix()
         {
            return mOrientation;
         }
         [XmlAttribute]
         public string Orientation
         {
            get
            {
               string outV = mOrientation.M11 + "," + mOrientation.M12 + "," + mOrientation.M13 + "," + mOrientation.M14 + "," +
                              mOrientation.M21 + "," + mOrientation.M22 + "," + mOrientation.M23 + "," + mOrientation.M24 + "," +
                              mOrientation.M31 + "," + mOrientation.M32 + "," + mOrientation.M33 + "," + mOrientation.M34 + "," +
                              mOrientation.M41 + "," + mOrientation.M42 + "," + mOrientation.M43 + "," + mOrientation.M44;

               return outV;
            }
            set
            {
               char[] sep = new char[] { ',' };
               string[] vs = value.Split(sep);

               mOrientation.M11 = System.Convert.ToSingle(vs[0]); mOrientation.M12 = System.Convert.ToSingle(vs[1]); mOrientation.M13 = System.Convert.ToSingle(vs[2]); mOrientation.M14 = System.Convert.ToSingle(vs[3]);
               mOrientation.M21 = System.Convert.ToSingle(vs[4]); mOrientation.M22 = System.Convert.ToSingle(vs[5]); mOrientation.M23 = System.Convert.ToSingle(vs[6]); mOrientation.M24 = System.Convert.ToSingle(vs[7]);
               mOrientation.M31 = System.Convert.ToSingle(vs[8]); mOrientation.M32 = System.Convert.ToSingle(vs[9]); mOrientation.M33 = System.Convert.ToSingle(vs[10]); mOrientation.M34 = System.Convert.ToSingle(vs[11]);
               mOrientation.M41 = System.Convert.ToSingle(vs[12]); mOrientation.M42 = System.Convert.ToSingle(vs[13]); mOrientation.M43 = System.Convert.ToSingle(vs[14]); mOrientation.M44 = System.Convert.ToSingle(vs[15]);

            }
         }
         [XmlAttribute]
         public string GR2Filename;
      };

      [XmlRoot("SceneObjects")]
      public class SceneObjectsXML
      {
         [XmlAttribute]
         public string aabbmax;

         [XmlAttribute]
         public string aabbmin;

         [XmlArrayItem(ElementName = "gr2names", Type = typeof(string))]
         [XmlArray("gr2names")]
         public List<string> objectGR2Names = new List<string>();

         [XmlArrayItem(ElementName = "objectInstances", Type = typeof(ObjectInstanceXML))]
         [XmlArray("objectInstances")]
         public List<ObjectInstanceXML> objectinstances = new List<ObjectInstanceXML>();

      };

      static public Vector3 Vec3FromString(string s)
      {
         string[] values = s.Split(',');
         return new Vector3(System.Convert.ToSingle(values[0]), System.Convert.ToSingle(values[1]), System.Convert.ToSingle(values[2]));
      }

      bool loadModelsSet(ZipArchive zip)
      {
            AbstractFile file = zip.GetFile("modelPositions.xml");
            if (!file.Exists)
               return false;


            SceneObjectsXML objectsXLM = new SceneObjectsXML();
            Stream stream = null;
            try
            {
               stream = file.OpenRead();   
                  XmlSerializer s = new XmlSerializer(typeof(SceneObjectsXML), new Type[] { });
                  objectsXLM = (SceneObjectsXML)s.Deserialize(stream);
               stream.Close();
            }
            catch(Exception e)
            {
               if(stream!=null)
                  stream.Close();
               Console.ForegroundColor = ConsoleColor.Red;
               Console.WriteLine("Error serializing the modelPositions.xml file");
               Console.ForegroundColor = ConsoleColor.White;
               return false;
            }
         

         try 
         {
            AbstractFolder fold = zip.GetFolder("models");

            for(int modelIdx = 0; modelIdx < objectsXLM.objectGR2Names.Count;modelIdx++)
            {
               if (!ModelManager.loadModelFromDisk(TerrainGlobals.mGameDir + objectsXLM.objectGR2Names[modelIdx]))
              // if(!ModelManager.loadModelFromArchive(objectsXLM.objectGR2Names[modelIdx], fold))
                  Console.WriteLine("Error loading model " + objectsXLM.objectGR2Names[modelIdx]);
            }

            for(int instIdx = 0; instIdx < objectsXLM.objectinstances.Count;instIdx++)
            {
               Matrix orient = objectsXLM.objectinstances[instIdx].getMatrix();
               ModelManager.addInstance(TerrainGlobals.mGameDir + objectsXLM.objectinstances[instIdx].GR2Filename, orient);
            }
            
            ModelManager.calcModelInstanceBuffers();
            
            if (objectsXLM.aabbmin != null)     ModelManager.mAABBMin = Vec3FromString(objectsXLM.aabbmin);
            if (objectsXLM.aabbmax != null)     ModelManager.mAABBMax = Vec3FromString(objectsXLM.aabbmax);
         }
         catch(Exception e)
         {
            Console.ForegroundColor = ConsoleColor.Red;
            Console.WriteLine("Error loading gr2 model data from source file");
            Console.ForegroundColor = ConsoleColor.White;
            return false;
         }

         return true;
      }
   }

   public class TLDHeader
   {
      public int Version;


   };
}
