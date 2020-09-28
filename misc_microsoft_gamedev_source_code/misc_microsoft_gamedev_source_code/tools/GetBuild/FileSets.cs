using System;
using System.Collections.Generic;
using System.Text;
using System.Xml.Serialization;

namespace GetBuild
{
   //==============================================================================
   // FileSets
   //==============================================================================
   [XmlRoot("FileSets")]
   public class FileSets
   {
      [XmlElement("BaseFile", typeof(string))]
      public List<string> mBaseFiles = new List<string>();

      [XmlElement("VideoFile", typeof(string))]
      public List<string> mVideoFiles = new List<string>();

      [XmlElement("CampaignFile", typeof(string))]
      public List<string> mCampaignFiles = new List<string>();

      [XmlElement("SkirmishFile", typeof(string))]
      public List<string> mSkirmishFiles = new List<string>();

      [XmlElement("FileSet", typeof(FileSet))]
      public List<FileSet> mFileSets = new List<FileSet>();
   };

   //==============================================================================
   // FileSet
   //==============================================================================
   public class FileSet
   {
      [XmlElement("Name")]
      public string mName = "";

      [XmlElement("BaseFiles")]
      public bool mBaseFiles = false;

      [XmlElement("VideoFiles")]
      public bool mVideoFiles = false;

      [XmlElement("CampaignFiles")]
      public bool mCampaignFiles = false;

      [XmlElement("SkirmishFiles")]
      public bool mSkirmishFiles = false;

      [XmlElement("CustomFile", typeof(string))]
      public List<string> mCustomFiles = new List<string>();
   };
}
