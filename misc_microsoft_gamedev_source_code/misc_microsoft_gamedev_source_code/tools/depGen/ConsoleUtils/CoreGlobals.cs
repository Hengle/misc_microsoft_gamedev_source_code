using System;
using System.IO;
using System.Text.RegularExpressions;
using System.Collections.Generic;
using System.Xml.Serialization;

using System.Xml;


namespace ConsoleUtils
{

   public class CoreGlobals
   {

      static public WorkPaths getWorkPaths() { return mWorkPaths; }

      static private WorkPaths mWorkPaths = new WorkPaths();

      static public void XmlDocumentLoadHelper(XmlDocument doc, string fileNameAbsolute)
      {
         try
         {
            doc.Load(fileNameAbsolute);
         }
         catch(System.Exception ex)
         {
            ConsoleOut.Write(ConsoleOut.MsgType.Error, "Invalid xml file \"{0}\".\n", fileNameAbsolute);
            ConsoleOut.Write(ConsoleOut.MsgType.Error, "{0}\n", ex.ToString());
            throw ex;
         }
      }
   }

   


   public class WorkPaths
   {
      public string mBaseDirectory;
      public string mLogsDirectory;
      public string mTedPath;
      public string mEditorSettings;
      public string mTemplateRoot;
      public string mEditorShaderDirectory;


      //public string mXTDPath;
      public string mBrushMasks;
      public string mTerrainTexturesPath;
      public string mGR2Dir;
      public string mClipArt;
      public string mRoadsPath;

      public string mGameDirectory;
      public string mGameArtDirectory;
      public string mGameDataDirectory;
      public string mGameScenarioDirectory;
      public string mGameLightsetBaseDirectory;
      public string mGameSkyBoxDirectory;
      public string mGameSoundDirectory;

      public string mNetworkErrorDirectory;
      public string mAppIconDir;

      public string mUnitDataExtension = ".vis";
      public string mWorkingTextureExtention = ".tga";

      public string mScriptPowerExtention = ".power";
      public string mScriptAbilityExtention = ".ability";
      public string mScriptExtention = ".triggerscript";
      public string mScriptTemplateExtention = ".template";
      public string mScriptPowerDirectory;
      public string mScriptAbilityDirectory;
      public string mScriptTriggerDirectory;


      public string mBlankTextureName;

      public string mParticleEffectDirectory;
      public string mParticleEffectExtension = ".pfx";
      public string mParticleEffectColorProgression;
      public string mParticleEffectFloatProgression;
      public string mParticleEffectLastSavePath;
      public string mParticleEffectLastLoadPath;

      public string mToolsDirectory;
      public string mXMBToolPath;
      public string mTAGToolPath;
      public string mPTHToolPath;
      public string mDOTToolPath;


      public WorkPaths()
      {
         
         mGameDirectory = computeGameDir();

         mEditorShaderDirectory = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "shaders");

         mBaseDirectory = mGameDirectory + @"\tools\phoenixeditor";//AppDomain.CurrentDomain.BaseDirectory;
         mLogsDirectory = mBaseDirectory + @"\logs";
         mBrushMasks = mBaseDirectory + @"\textures\masks";
         mEditorSettings = mBaseDirectory + @"\settings";
         mTemplateRoot = mEditorSettings + @"\Templates";
         //mClipArt = mBaseDirectory + @"ClipArt";
         mAppIconDir = mBaseDirectory + @"\Icons";

         mNetworkErrorDirectory = @"\\esfile\phoenix\tools\editor\exceptions";

         mToolsDirectory = mGameDirectory + @"\tools";
         mXMBToolPath = mToolsDirectory + @"\xmlComp\xmlComp.exe";
         mTAGToolPath = mToolsDirectory + @"\tagGen\tagGen.exe";
         mPTHToolPath = mToolsDirectory + @"\genPTH\genPTH.exe";
         mDOTToolPath = mToolsDirectory + @"\depGen\dot.exe";


         mGameArtDirectory = mGameDirectory + @"\art";
         mGameDataDirectory = mGameDirectory + @"\data";
         mGameSoundDirectory = mGameDirectory + @"\sound";
         mGameScenarioDirectory = mGameDirectory + @"\scenario";

         mTerrainTexturesPath = mGameArtDirectory + @"\terrain";

         mGameSkyBoxDirectory = mGameArtDirectory + @"\environment\sky";

         mClipArt = mGameArtDirectory + @"\clipart";

         mRoadsPath = mGameArtDirectory + @"\roads";

         //mXTDPath = mGameScenarioDirectory;

         mScriptPowerDirectory = mGameDataDirectory + @"\powers";
         mScriptAbilityDirectory = mGameDataDirectory + @"\abilities";
         mScriptTriggerDirectory = mGameDataDirectory + @"\triggerscripts";

         mGameLightsetBaseDirectory = mGameScenarioDirectory;// mGameArtDirectory + @"\xlightsets";

         mTedPath = mGameScenarioDirectory;

         mGR2Dir = mGameArtDirectory;

         mBlankTextureName = mTerrainTexturesPath + @"\blank_df.tga";

         mParticleEffectDirectory = mGameArtDirectory + @"\effects";
         mParticleEffectLastSavePath = mParticleEffectDirectory;
         mParticleEffectLastLoadPath = mParticleEffectDirectory;
         mParticleEffectColorProgression = mBaseDirectory + @"ColorProgression";
         mParticleEffectFloatProgression = mBaseDirectory + @"FloatProgression";
      }

      string computeGameDir()
      {
         string gameDir = AppDomain.CurrentDomain.BaseDirectory;// mBaseDirectory;

         if (gameDir.Contains("\\work\\"))
         {
            gameDir = gameDir.Substring(0, gameDir.IndexOf("\\work\\")) + "\\work";
         }
         else if (gameDir.Contains("\\production\\"))
         {
            gameDir = gameDir.Substring(0, gameDir.IndexOf("\\production\\")) + "\\production\\work";            
         }
         else if (gameDir.Contains("\\xbox\\"))
         {
            gameDir = gameDir.Substring(0, gameDir.IndexOf("\\xbox\\")) + "\\xbox\\work";
         }
         else if (gameDir.Contains("\\x\\"))
         {
            gameDir = gameDir.Substring(0, gameDir.IndexOf("\\x\\")) + "\\x\\work";
         }
         else if (gameDir.Contains("\\X\\"))
         {
            gameDir = gameDir.Substring(0, gameDir.IndexOf("\\X\\")) + "\\X\\work";
         }
         else
         {
            return "";
         }

         return gameDir;
      }


      public string ConverToLocalWorkPath(string nonLocalPath)
      {
         string localPath = "";
         if (nonLocalPath.Contains("\\work\\"))
         {
            int workNonLocalIndex = nonLocalPath.IndexOf("\\work\\") + 6;

            localPath = Path.Combine(mGameDirectory , nonLocalPath.Remove(0,workNonLocalIndex));
         }
         else
         {
            throw new Exception("can't convert to local directory: " + nonLocalPath);
         }

         return localPath;
      }
      public string GetDefaultLightset()
      {
         return mGameLightsetBaseDirectory + "\\defaultGlobalLights.gls"; 
      }
      public string GetProtoObjectFile()
      {
         return Path.Combine(CoreGlobals.getWorkPaths().mGameDataDirectory, "objects.xml");
      }

      

      public string getWorkingTexName(string textureName)
      {
         string fileName = Path.GetFileNameWithoutExtension(textureName);
         string filePath = Path.GetDirectoryName(textureName);
         string ext = @".tga";

         filePath = mTerrainTexturesPath + filePath.Remove(0, filePath.LastIndexOf("\\"));

         string fullFileName = filePath + "\\" + fileName + ext;

         if (File.Exists(fullFileName))
            return fullFileName;
         else
            return mBlankTextureName;
      }


      public string[] GetScenarioFiles()
      {
         List<string> scenarioFile = new List<string>(); 

         string[] subDirs = Directory.GetDirectories(CoreGlobals.getWorkPaths().mGameScenarioDirectory,"*", SearchOption.AllDirectories);

         foreach (string dir in subDirs)
         {
            string[] files = Directory.GetFiles(dir, "*.scn");
            scenarioFile.AddRange(files);
         }

         for (int i = 0; i < scenarioFile.Count; i++)
            scenarioFile[i] = scenarioFile[i].ToLower();

         return scenarioFile.ToArray();

      }
   }

   //Supplements the Path class
   public class ResourcePathInfo
   {
      //To add other detectable extension groups OR in blocks like (?<textureExt>bmp|tga|ddt|ddx) to ((?<textureExt>bmp|tga|ddt|ddx)|(?<otherExt>...))
      //    Please test carefully since breaking this would be really bad :)
      //static since it is a compiled regex
      static public Regex mResourcePathInfoEX = new Regex(@"(((?<preWorkPath>.*)\\work\\(?<workRelativeFilePath>.*))|\\(?<otherRelativeFilePath>.*)|(?<nonRelativePath>.*))[.](?<extension>((?<textureExt>bmp|tga|ddt|ddx|hdr|tif)|(?<otherExt>...)))");
      //static public Regex mResourcePathInfoEX = new Regex(@"(((?<preWorkPath>.*)\\work\\art\\(?<artRelativeFilePath>.*))|((?<preWorkPath>.*)\\work\\(?<workRelativeFilePath>.*))|\\(?<otherRelativeFilePath>.*)|(?<nonRelativePath>.*))[.](?<extension>((?<textureExt>bmp|tga|ddt|ddx|hdr|tif)|(?<otherExt>...)))");
      string mFileName = "";
      Match mMatchedEx = null;
      public ResourcePathInfo(string fileName)
      {
         mFileName = fileName;
         mMatchedEx = mResourcePathInfoEX.Match(mFileName);
      }


      public bool IsFilePath
      {
         get
         {
            return mMatchedEx.Success;
         }
      }
      public bool IsRelativePath
      {
         get
         {
            return IsFilePath && (mMatchedEx.Groups["otherRelativeFilePath"].Success || mMatchedEx.Groups["workRelativeFilePath"].Success);

         }
      }
      public bool IsWorkRelativePath
      {
         get
         {
            return IsFilePath && mMatchedEx.Groups["workRelativeFilePath"].Success;

         }
      }
      public bool IsTexture
      {
         get
         {
            return IsFilePath && mMatchedEx.Groups["textureExt"].Success;
         }
      }
      public string RelativePathNoExt
      {
         get
         {
            if (IsWorkRelativePath)
               return mMatchedEx.Groups["workRelativeFilePath"].Value;
            else
               return mMatchedEx.Groups["otherRelativeFilePath"].Value;
         }
      }
      public string RelativePath
      {
         get
         {
            return RelativePathNoExt + "." + mMatchedEx.Groups["extension"].Value;
         }

      }
      public string Value
      {
         get
         {
            return mFileName;
         }
      }

   }

   public class ResourcePathInfoOneOffArtRelative
   {
      //To add other detectable extension groups OR in blocks like (?<textureExt>bmp|tga|ddt|ddx) to ((?<textureExt>bmp|tga|ddt|ddx)|(?<otherExt>...))
      //    Please test carefully since breaking this would be really bad :)
      //static since it is a compiled regex
      //static public Regex mResourcePathInfoEX = new Regex(@"(((?<preWorkPath>.*)\\work\\(?<workRelativeFilePath>.*))|\\(?<otherRelativeFilePath>.*)|(?<nonRelativePath>.*))[.](?<extension>((?<textureExt>bmp|tga|ddt|ddx|hdr|tif)|(?<otherExt>...)))");
      static public Regex mResourcePathInfoEX = new Regex(@"(((?<preWorkPath>.*)\\work\\art\\(?<artRelativeFilePath>.*))|((?<preWorkPath>.*)\\work\\(?<workRelativeFilePath>.*))|\\(?<otherRelativeFilePath>.*)|(?<nonRelativePath>.*))[.](?<extension>((?<textureExt>bmp|tga|ddt|ddx|hdr|tif)|(?<otherExt>...)))");
      string mFileName = "";
      Match mMatchedEx = null;
      public ResourcePathInfoOneOffArtRelative(string fileName)
      {
         mFileName = fileName;
         mMatchedEx = mResourcePathInfoEX.Match(mFileName);
      }


      public bool IsFilePath
      {
         get
         {
            return mMatchedEx.Success;
         }
      }
      public bool IsRelativePath
      {
         get
         {
            return IsFilePath && (mMatchedEx.Groups["artRelativeFilePath"].Success || mMatchedEx.Groups["otherRelativeFilePath"].Success || mMatchedEx.Groups["workRelativeFilePath"].Success);

         }
      }
      public bool IsWorkRelativePath
      {
         get
         {
            return IsFilePath && mMatchedEx.Groups["workRelativeFilePath"].Success;

         }
      }
      public bool IsArtRelativePath
      {
         get
         {
            return IsFilePath && mMatchedEx.Groups["artRelativeFilePath"].Success;

         }
      }
      public bool IsTexture
      {
         get
         {
            return IsFilePath && mMatchedEx.Groups["textureExt"].Success;
         }
      }
      public string RelativePathNoExt
      {
         get
         {
            if (IsArtRelativePath)
               return mMatchedEx.Groups["artRelativeFilePath"].Value;
            else if (IsWorkRelativePath)
               return mMatchedEx.Groups["workRelativeFilePath"].Value;
            else
               return mMatchedEx.Groups["otherRelativeFilePath"].Value;
         }
      }
      public string RelativePath
      {
         get
         {
            return RelativePathNoExt + "." + mMatchedEx.Groups["extension"].Value;
         }

      }
      public string Value
      {
         get
         {
            return mFileName;
         }
      }

   }






}




