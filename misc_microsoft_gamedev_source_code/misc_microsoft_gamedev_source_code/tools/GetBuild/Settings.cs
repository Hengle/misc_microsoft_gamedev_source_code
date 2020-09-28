using System;
using System.Collections.Generic;
using System.Text;
using System.Xml.Serialization;

namespace GetBuild
{
   //==============================================================================
   // Settings
   //==============================================================================
   [XmlRoot("Settings")]
   public class Settings
   {
      [XmlElement("BuildFolder")]
      public string mBuildFolder = @"\\esfile3\HaloWars\FinalBuilds";

      [XmlElement("LocalFolder")]
      public string mLocalFolder = @"c:\x\build";

      [XmlElement("PreBatchFile")]
      public string mPreBatchFile = "";

      [XmlElement("PostBatchFile")]
      public string mPostBatchFile = "";

      [XmlElement("FileSet")]
      public string mFileSet = "All";

      [XmlElement("XboxName")]
      public string mXboxName = "";

      [XmlElement("XboxFolder")]
      public string mXboxFolder = @"xe:\x\build";

      [XmlElement("DeleteFiles")]
      public bool mDeleteFiles = true;

      [XmlElement("SeparateFolder")]
      public bool mSeparateFolder = false;

      [XmlElement("Config", typeof(string))]
      public List<string> mConfigs = new List<string>();

      [XmlElement("Get", typeof(SettingsGet))]
      public List<SettingsGet> mGets = new List<SettingsGet>();

      [XmlElement("LastBuild")]
      public string mLastBuild = "";

      [XmlElement("LastFileSet")]
      public string mLastFileSet = "";

      [XmlElement("LastFolder")]
      public string mLastFolder = "";

      [XmlElement("CopyMethod")]
      public string mCopyMethod = "xfsCopy";

      [XmlElement("ServerIP")]
      public string mServerIP = "";

      [XmlElement("Exe")]
      public string mExe = "";

      [XmlElement("XFS")]
      public bool mXFS = true;
   };

   //==============================================================================
   // SettingsGet
   //==============================================================================
   public class SettingsGet
   {
      [XmlElement("Build")]
      public string mBuild = "";

      [XmlElement("FileSet")]
      public string mFileSet = "";

      [XmlElement("Folder")]
      public string mFolder = "";

      [XmlElement("DeleteFiles")]
      public bool mDeleteFiles = false;

      [XmlElement("SeparateFolder")]
      public bool mSeparateFolder = false;

      [XmlElement("Copy", typeof(SettingsCopy))]
      public List<SettingsCopy> mCopies = new List<SettingsCopy>();
   };

   //==============================================================================
   // SettingsCopy
   //==============================================================================
   public class SettingsCopy
   {
      [XmlElement("Xbox")]
      public string mXbox = "";

      [XmlElement("Folder")]
      public string mFolder = "";

      [XmlElement("Copied")]
      public bool mCopied = false;
   };
}
