using System;
using System.Collections.Generic;
using System.IO;
using System.Runtime.InteropServices;
using System.Xml;
using System.Xml.Serialization;
using EditorCore;

namespace Export360
{
   public class DEP
   {
      TerrainXBOXDependents mTXD = new TerrainXBOXDependents();
      private string giveNameFromWork(String fullPathNameForFile)
      {
         string fname = fullPathNameForFile;
         int k = fullPathNameForFile.LastIndexOf(@"work") + 4;
         if (k < 4)
            k = 0;
         fname = fname.Substring(k, fullPathNameForFile.Length - k);
         if(fname.StartsWith("\\"))
            fname = fname.Remove(0,1);

         return fname;
      }


      public void addFileDependent(String filename, bool isOptional)
      {
         TerrainDependentFile tdf = new TerrainDependentFile();
         tdf.mFilename = giveNameFromWork(filename);
         tdf.isOptional = isOptional;
         mTXD.mDependents.Add(tdf);
      }

      public unsafe bool writeToFile(string filename)
      {
         string fileName = Path.ChangeExtension(filename,".DEP");
         if (File.Exists(fileName))
            File.Delete(fileName);

         XmlSerializer s = new XmlSerializer(typeof(TerrainXBOXDependents), new Type[] { });
         Stream st = File.Open(fileName, FileMode.OpenOrCreate);
         s.Serialize(st, mTXD);
         st.Close();

         return true;
      }
      public void clear()
      {
         mTXD.mDependents.Clear();
      }

      [XmlRoot("File")]
      public class TerrainDependentFile
      {
         [XmlAttribute]
         public bool isOptional = true;
         

         [XmlText]
         public string mFilename = "";
      };

      [XmlRoot("TerrainXBOXDependents")]
      public class TerrainXBOXDependents
      {
         [XmlArrayItem(ElementName = "File", Type = typeof(TerrainDependentFile))]
         [XmlArray("FileDependencies")]
         public List<TerrainDependentFile> mDependents = new List<TerrainDependentFile>();

      };

   }
}