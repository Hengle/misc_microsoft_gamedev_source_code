using System;
using System.IO;
using System.Windows.Forms;
using System.Text.RegularExpressions;
using System.Collections.Generic;
using System.Xml.Serialization;

namespace EditorCore
{

   public class CoreGlobals
   {
      static public BEditorMain getEditorMain() { return mEditorMain; }

      static public WorkPaths getWorkPaths() { return mWorkPaths; }
      static public WorkPaths getSaveLoadPaths() { return mSaveLoadPaths; }
      static public ErrorManager getErrorManager() { return mErrorManager; }
      static public PerforceManager getPerforce() { return mPerforceManager; }
      static public SoundManager getSoundManager() { return mSoundManager; }
      static public SettingsFile getSettingsFile() { return mSettingsFile; }

      static public GameResources getGameResources() { return mGameResources; }
      static public WorkTopicManager getScenarioWorkTopicManager() { return mScenarioWorkTopicManager; }

      static public string getVersion() { return "1.0.1.10"; }
      static public string getClipArtVersion() { return "1.0"; }

      //static public Dictionary<string, WorkTopic> getScenarioWorkTopics() { return mScenarioWorkTopics; }
      //static public bool sDontPromtResolvedSave = false;

      static public bool OutOfMemory = false;
      static public bool FatalSaveError = false;
      static public bool FatalSCNSaveError = false;
      static public bool FatalEdDataSaveError = false;

      static public bool NetworkWriteBlocked = false;

      static public void loadSettingsFile(string file) {mSettingsFile = SettingsFile.Load(file); }
     // public const int cTextureSetCount = 16;
      
      


      //------------------------Members
      //static private Dictionary<string, WorkTopic> mScenarioWorkTopics = new Dictionary<string, WorkTopic>();
      static private BEditorMain mEditorMain = new BEditorMain();
      static private WorkPaths mWorkPaths = new WorkPaths();
      static private WorkPaths mSaveLoadPaths = new WorkPaths();
      static private ErrorManager mErrorManager = new ErrorManager();
      static private PerforceManager mPerforceManager = new PerforceManager();
      static private SoundManager mSoundManager = new SoundManager();
      static private SettingsFile mSettingsFile = new SettingsFile();
      static private GameResources mGameResources = new GameResources();
      static private WorkTopicManager mScenarioWorkTopicManager = new WorkTopicManager();

      static public ProcessorInfo mProcessorInfo = new ProcessorInfo();

      static bool mbScanedForDevPermission = false;
      static bool mbIsDev = false;
      static public bool IsDev
      {
         get         
         {
            if(mbScanedForDevPermission == false)
            {
               mbScanedForDevPermission = true;
               if(System.Environment.UserName.ToLower().Contains("mhanson"))
               {
                  mbIsDev = true;
               }
               if (File.Exists(Path.Combine(mWorkPaths.mEditorSettings,"Dev.txt")))
               {
                  mbIsDev = true;
               }

            }

            return mbIsDev;
         }

      }


      static public void ShowMessage(string text)
      {
         if (ConsoleMode == false)
         {
            MessageBox.Show(text);
         }
         else
         {
            Console.WriteLine(text);
         }
      }

      static private bool mbConsoleMode = false;
      static public bool ConsoleMode
      {
         set
         {
            mbConsoleMode = value;
         }
         get
         {
            return mbConsoleMode;
         }

      }

      static private bool mbUsingPerforce = true;
      static public bool UsingPerforce
      {
         set
         {
            mbUsingPerforce = value;
         }
         get
         {
            return mbUsingPerforce;
         }

      }


      static private bool mbIsBatchExport = false;
      static public bool IsBatchExport
      {
         set
         {
            mbIsBatchExport = value;
         }
         get
         {
            return mbIsBatchExport;
         }

      }

      ////////////////FileNames!!!
      
      static public string ScenarioDirectory
      {
         set
         {
            mScenarioDirectory = value;
         }
         get
         {
            return mScenarioDirectory;
         }
      }
      static public string TerrainDirectory
      {
         set
         {
            mTerrainDirectory = value;
         }
         get
         {
            return mTerrainDirectory;
         }
      }

      static public string ScenarioFile
      {
         set
         {
            mScenarioFile = Path.GetFileName(value);
         }
         get
         {
            return mScenarioFile;
         }
      }

      static public string ScenarioArtFile
      {
         //set
         //{
         //   mScenarioArtFile = Path.GetFileName(value);
         //}
         get
         {
            return Path.ChangeExtension(mScenarioFile, "sc2");
         }
      }

      static public string ScenarioSoundFile
      {
         //set
         //{
         //   mScenarioSoundFile = Path.GetFileName(value);
         //}
         get
         {
            return Path.ChangeExtension(mScenarioFile, "sc3");
         }
      }

      static public string TerrainFile
      {
         set
         {
            mTerrainFile = Path.GetFileName(value);
         }
         get
         {
            return mTerrainFile;
         }
      }

     
      static public string ScenarioLightsetFilename
      {
         set
         {
            mScenarioLightsetFilename = Path.GetFileName(value);
         }
         get
         {
            return mScenarioLightsetFilename;
         }
      }

      static public string ScenarioEditorSettingsFilename
      {
         get
         {
            return "editorData.zip";
         }
      }
      static public string ScenarioSkyboxFilename
      {
         set
         {
            mScenarioSkyboxFilename = value;
         }
         get
         {
            return mScenarioSkyboxFilename;
         }
      }
      static public string ScenarioTerrainEnvMapFilename
      {
         set
         {
            mScenarioTerrainEnvMapFilename = value;
         }
         get
         {
            return mScenarioTerrainEnvMapFilename;
         }
      }
      static public string ScenarioMinimapTextureFilename
      {
         set
         {
            mScenarioMinimapTextureFilename = value;
         }
         get
         {
            return mScenarioMinimapTextureFilename;
         }
      }

      static public List<string> ScenarioSoundbankFilenames
      {
         set 
         {
            mScenarioSoundbankFilenames = value;
         }
         get 
         {
            return mScenarioSoundbankFilenames;
         }
      }

      static public string ScenarioWorld
      {
         set
         {
            mScenarioWorld = value;
         }
         get
         {
            return mScenarioWorld;
         }
      }

      static public int ScenarioBuildingTextureIndexUNSC
      {
         set
         {
            mScenarioBuildingTextureIndexUNSC = value;
         }
         get
         {
            return mScenarioBuildingTextureIndexUNSC;
         }
      }
      static public int ScenarioBuildingTextureIndexCOVN
      {
         set
         {
            mScenarioBuildingTextureIndexCOVN = value;
         }
         get
         {
            return mScenarioBuildingTextureIndexCOVN;
         }
      }
     
      static private string mScenarioDirectory = "";
      static private string mTerrainDirectory = "";
      //FilenameOnly..
      static private string mScenarioFile = "";
      //static private string mScenarioArtFile = "";
      //static private string mScenarioSoundFile = "";
      static private string mTerrainFile = "";
      static private string mScenarioLightsetFilename = "";
      static private string mScenarioSkyboxFilename = @"environment\sky\sky_dome_01.vis";
      static private string mScenarioTerrainEnvMapFilename = @"environment\sky\cloudy_sky_dome_en.ddx";
      static private string mTerrainMaterialsFilename = "";
      static private string mScenarioMinimapTextureFilename = "";
      static private int mScenarioBuildingTextureIndexUNSC = 0;
      static private int mScenarioBuildingTextureIndexCOVN = 0;
      static private List<string> mScenarioSoundbankFilenames = new List<string>();
      static private string mScenarioWorld = "";

      static public void ResetWorkingPaths()
      {
         mScenarioFile = "";
         mTerrainFile = "";
         mScenarioLightsetFilename = "";
         mScenarioMinimapTextureFilename = "";
         mScenarioSoundbankFilenames.Clear();
      }
      static public string GetBaseProjectName()
      {
         return Path.GetFileNameWithoutExtension(ScenarioFile);
      }

      //--------------------------
      //Other Stuff!
      static public int mPlayableBoundsMinX=-1;
      static public int mPlayableBoundsMinZ=-1;
      static public int mPlayableBoundsMaxX=-1;
      static public int mPlayableBoundsMaxZ=-1;

      static public bool mbAllowVeterancy=true;
      static public bool mbLoadTerrainVisRep = true;
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
      public string mFoliagePath;

      public string mGameDirectory;
      public string mGameArtDirectory;
      public string mTalkingHeadDirectory;
      public string mGameDataDirectory;
      public string mGameDataAIDataDirectory;
      public string mGameScenarioDirectory;
      public string mGameLightsetBaseDirectory;
      public string mGameSkyBoxDirectory;
      public string mGameSoundDirectory;
      public string mGameMinimapDirectory;
      public string mGameLoadmapDirectory;
      public string mGameFlashUIDirectory;
      public string mGamePhysicsDirectory;
      //NEWGAMEDIRECTORY  --search for this

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

      public string mAIDataExtension = ".ai";

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
      public string mLRPToolPath;

      public string mScenarioBackupDirectory;

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
         mLRPToolPath = mToolsDirectory + @"\LRPtreeGen\LRPtreeGen.exe";


         mTalkingHeadDirectory = mGameDirectory + @"\video\talkingheads";
         mGameArtDirectory = mGameDirectory + @"\art";
         mGameDataDirectory = mGameDirectory + @"\data";
         mGameSoundDirectory = mGameDirectory + @"\sound";
         mGameScenarioDirectory = mGameDirectory + @"\scenario";
         mScenarioBackupDirectory = mGameDirectory + @"\scenario\localbackupfiles";
         mTerrainTexturesPath = mGameArtDirectory + @"\terrain";
         mGameSkyBoxDirectory = mGameArtDirectory + @"\environment\sky";
         mGameMinimapDirectory = mGameArtDirectory + @"\ui\flash\minimaps";
         mGamePhysicsDirectory = mGameDirectory + @"\physics";
         mGameLoadmapDirectory = mGameArtDirectory + @"\ui\flash\pregame\textures";
         mGameFlashUIDirectory = mGameArtDirectory + @"\ui\flash";
         //NEWGAMEDIRECTORY  --search for this


         mClipArt = mGameArtDirectory + @"\clipart";
         mRoadsPath = mGameArtDirectory + @"\roads";
         mFoliagePath = mGameArtDirectory + @"\foliage";

         //mXTDPath = mGameScenarioDirectory;

         mScriptPowerDirectory = mGameDataDirectory + @"\powers";
         mScriptAbilityDirectory = mGameDataDirectory + @"\abilities";
         mScriptTriggerDirectory = mGameDataDirectory + @"\triggerscripts";
         mGameDataAIDataDirectory = mGameDataDirectory + @"\aidata";

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
         else if (gameDir.Contains("\\code\\"))
         {
            gameDir = gameDir.Substring(0, gameDir.IndexOf("\\code\\")) + "\\work";
         }
         else if (gameDir.Contains("\\production\\"))
         {
            gameDir = gameDir.Substring(0, gameDir.IndexOf("\\production\\")) + "\\production\\work";            
         }
         else if (gameDir.Contains("\\xbox\\"))
         {
            gameDir = gameDir.Substring(0, gameDir.IndexOf("\\xbox\\")) + "\\xbox\\work";
         }
         else if (gameDir.Contains("\\phoenix\\"))
         {
            gameDir = gameDir.Substring(0, gameDir.IndexOf("\\phoenix\\")) + "\\phoenix\\work";
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
   public class FileUtilities
   {
      public static bool BackupScenarioFile(string filename)
      {
         try
         {
            if (File.Exists(filename))
            {
               string backup = filename + ".Backup";
               backup = backup.Replace(CoreGlobals.getWorkPaths().mGameScenarioDirectory, CoreGlobals.getWorkPaths().mScenarioBackupDirectory);
               if (Directory.Exists(Path.GetDirectoryName(backup)) == false)
               {
                  Directory.CreateDirectory(Path.GetDirectoryName(backup));
               }
               if (File.Exists(backup))
               {
                  File.SetAttributes(backup, FileAttributes.Normal);
               }
               File.Copy(filename, backup, true);
            }
         }
         catch(System.Exception ex)
         {
            return false;
         }
         return true;
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

   public class DBIDHelperTool
   {
      static public void Run()
      {
         string path = Path.Combine(CoreGlobals.getWorkPaths().mGameDataDirectory, "updatedbid.bat");
         if (File.Exists(path) == false)
         {
            CoreGlobals.ShowMessage("Error with dbid tool");
         }
         
         System.Diagnostics.Process dbidtool;
         dbidtool = new System.Diagnostics.Process();
         System.Diagnostics.ProcessStartInfo p = new System.Diagnostics.ProcessStartInfo(path);
         p.WorkingDirectory = CoreGlobals.getWorkPaths().mGameDataDirectory;
         dbidtool = System.Diagnostics.Process.Start(p);
         dbidtool.WaitForExit();
         dbidtool.Close();

      }
   }

   public class XMBProcessor 
   {
      static bool mPaused = false;
      static public void Pause()
      {
         mPaused = true;
      }
      static public void Unpause()
      {
         mPaused = false;
      }

      static public void CreateXMB(string filename, bool UsePerforce)
      {
         CreateXMB(filename, null, UsePerforce);
      }
      static public void CreateXMB(string filename, string outputpath, bool UsePerforce)
      {
         if (mPaused)
            return;

         try
         {
            //xmlComp –outsamedir –checkout –errormessagebox –pauseonwarnings –file filename.xml
            //Where “filename.xml” can be any XML file (with any extension).

            if(File.Exists(CoreGlobals.getWorkPaths().mXMBToolPath) == false)
            {
               CoreGlobals.ShowMessage("Can't find: " + CoreGlobals.getWorkPaths().mXMBToolPath +  " ...Error exporting " + filename);
               return;
            }

            string arguments = "";
            if (outputpath == null)
               arguments = arguments + " -outsamedir";
            else
               arguments = arguments + " -outpath " + outputpath;

            arguments = arguments + " -errormessagebox";
            arguments = arguments + " -file \"" + filename + "\"";
            if (UsePerforce == true)
            {
               arguments = arguments + " -checkout";
            }

            System.Diagnostics.Process xmbUtility;
            xmbUtility = new System.Diagnostics.Process();
            xmbUtility = System.Diagnostics.Process.Start(CoreGlobals.getWorkPaths().mXMBToolPath, arguments);
            xmbUtility.WaitForExit();
            if (xmbUtility.ExitCode != 0)
            {
               CoreGlobals.ShowMessage("Error xmb'ing: " + filename);
            }
            xmbUtility.Close();

         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnException(ex);
         }
      }



   }



   public class ErrorManager
   {
      public string mErrorLog;
      public ErrorManager()
      {
         if (!Directory.Exists(CoreGlobals.getWorkPaths().mLogsDirectory))
            Directory.CreateDirectory(CoreGlobals.getWorkPaths().mLogsDirectory);

         mErrorLog = Path.Combine(CoreGlobals.getWorkPaths().mLogsDirectory, "ErrorLog." + System.DateTime.Now.ToFileTimeUtc() + ".txt");
      }

      public void AppendToLog(string text)
      {
         StreamWriter w = new StreamWriter(mErrorLog, true);
         w.WriteLine(text);
         w.Close();
         
      }

      public void SendToErrorWarningViewer(string text)
      {
         if (CoreGlobals.getEditorMain() != null && CoreGlobals.getEditorMain().mIGUI != null)
         {
            CoreGlobals.getEditorMain().mIGUI.AddToErrorList(text);
         }
         AppendToLog(text);
      }

      

      public void OnSimpleWarning(string text)
      {
         //MessageBox.Show(text,"Warning:");
         SendToErrorWarningViewer("Warning: " + text);
      }

      //public void OnError(string text)
      //{
      //   MessageBox.Show(text, "Error:");
      //   LogErrorToNetwork
      //}

      public void OnException(System.Exception ex)
      {
         OnException(ex, false);
      }



      public void OnException(System.Exception ex, bool silent)
      {
         if (ex is System.OutOfMemoryException)
         {
            CoreGlobals.OutOfMemory = true;
         }

         if (LogErrorToNetwork(ex.ToString()))
         {
            if (!silent)
            {
               CoreGlobals.ShowMessage("Aww Snap: Error uploaded to network: " + ex.ToString());
               //MessageBox.Show("Aww Snap: Error uploaded to network: " + ex.ToString());
            }
         }
         else
         {
            if (!silent)
            {

               Clipboard.SetText(ex.ToString());
               CoreGlobals.ShowMessage("Aww Snap: Network not detected (exception copied to clipboard please paste it in a mail it to Andrew): \n" + ex.ToString());
               //MessageBox.Show("Aww Snap: Network not detected (exception copied to clipboard please paste it in a mail it to Andrew): \n" + ex.ToString());
            }
         }

      }

      public bool LogErrorToNetwork(string text)
      {
         SendToErrorWarningViewer("Exception: " + text);
         try
         {
            if (CoreGlobals.NetworkWriteBlocked == false && Directory.Exists(CoreGlobals.getWorkPaths().mNetworkErrorDirectory))
            {
               string fileName = String.Format("{0}.{1}.{2}.txt", System.Environment.UserName, System.Environment.MachineName, System.DateTime.Now.ToFileTimeUtc());
               fileName = Path.Combine(CoreGlobals.getWorkPaths().mNetworkErrorDirectory, fileName);
               StreamWriter w = new StreamWriter(fileName);
               w.WriteLine(text);
               w.Close();
               return true;
            }
         }
         catch (System.UnauthorizedAccessException ex)
         {
            ex.ToString();
            CoreGlobals.ShowMessage("No write access to network: " + CoreGlobals.getWorkPaths().mNetworkErrorDirectory);
            CoreGlobals.NetworkWriteBlocked = true;
            return false;
         }
         return false;
      }


   }

   [XmlRoot("DBIDs")]
   public class DBIDsXML
   {
      int mMaxID = -1;
      [XmlElement("MaxID")]
      public int MaxID
      {
         get
         {
            return mMaxID;
         }
         set
         {
            mMaxID = value;
         }
      }

      static public bool GetNewDBID(string fileName, out int newValue)
      {
         Cursor.Current = Cursors.WaitCursor;
         try
         {
            newValue = int.MinValue;

            SimpleFileStatus status = CoreGlobals.getPerforce().getConnection().P4GetFileStatusSimple(fileName);
            if (status.CheckedOutThisUser == true)
            {
               MessageBox.Show("Please check in objects.xml and try again");
               return false;
            }
            if (status.State == eFileState.NotInPerforce)
            {
               MessageBox.Show("Perforce was not able to find: " + fileName + " \r\nIf the file is copied down, then you may need to make sure you phoenix profile is the default profile. ");
               return false;
            }

            PerforceChangeList changeList = CoreGlobals.getPerforce().GetNewChangeList("Trigger DBID Increment");

            if (changeList != null)
            {
               //Sync
               if (changeList.SyncFile(fileName) == false)
               {
                  MessageBox.Show("Perforce error: " + changeList.GetLastError());
                  return false;
               }
               //Checkout for Edit
               if (changeList.EditFile(fileName) == false)
               {
                  MessageBox.Show("Perforce error: " + changeList.GetLastError());
                  return false;
               }
               //load
               XmlSerializer s = new XmlSerializer(typeof(DBIDsXML), new Type[] { });
               Stream st = File.OpenRead(fileName);
               DBIDsXML data = (DBIDsXML)s.Deserialize(st);
               st.Close();

               //increment value
               data.MaxID = data.MaxID + 1;
               //Update value
               newValue = data.MaxID;

               //save
               st = File.Open(fileName, FileMode.Create);
               s.Serialize(st, data);
               st.Close();

               //checkin
               System.Threading.Thread.Sleep(100);
               changeList.Submitchanges();

               return true;
            }
            return false;
         }
         catch (System.Exception ex)
         {
            newValue = int.MinValue;
            MessageBox.Show("Problem with : " + fileName + "   ex: " + ex.ToString());
            return false;
         }
         finally
         {
            Cursor.Current = Cursors.Default;
         }
      }


   }



   
   //this may change soon.  let Andrew know if you want to add to it
   public class SettingsFile
   {
      [XmlElement]
      public bool DesignerControls = false;
      [XmlElement]
      public bool ShowFog = true;
      [XmlElement]
      public bool ArtistModePan = true;
      [XmlElement]
      public bool ThreadedStart = true;


      [XmlElement]
      public bool AutoSaveEnabled = false;
      [XmlElement]
      public int AutoSaveTimeInMinutes = 15;
      [XmlElement]
      public bool AutoSaveToCustom = true;

      [XmlElement]
      public int DepartementMode = 1;//default = design

      [XmlIgnore]
      public string mFileName = "";
      static public SettingsFile Load(string fileName)
      {
         SettingsFile settings = new SettingsFile();
         try
         {
            settings.mFileName = fileName;
            if (!File.Exists(fileName))
               return settings;
            XmlSerializer s = new XmlSerializer(typeof(SettingsFile), new Type[] { });
            Stream st = File.OpenRead(fileName);
            settings = ((SettingsFile)s.Deserialize(st));
            settings.mFileName = fileName;
            st.Close();
         }
         catch (System.Exception ex)
         {
            ex.ToString();
         }
         return settings;
      }
      public void Save(string fileName)
      {
         try
         {
            XmlSerializer s = new XmlSerializer(typeof(SettingsFile), new Type[] { });
            Stream st = File.Open(fileName, FileMode.Create);
            s.Serialize(st, this);
            st.Close();
         }
         catch (System.Exception ex)
         {
            ex.ToString();
         }
      }
      public void Save()
      {
         if (mFileName != "")
            Save(mFileName);            
      }
   }

   public class WorkTopicManager
   {
      public PerforceChangeList mChangeList = null; // new PerforceChangeList();
      public Dictionary<string, WorkTopic> getScenarioWorkTopics() { return mScenarioWorkTopics; }
      public bool sDontPromtResolvedSave = false;
      public bool sAllowOverwrite = false;

      
      private Dictionary<string, WorkTopic> mScenarioWorkTopics = new Dictionary<string, WorkTopic>();

      string mChangelistName = "";

      public string mChangeListPrefix = "*****************uninitialized";

      public PerforceChangeList GetChangeList(string name)
      {
         //if(mChangeList == null)

         if (mChangeList == null || mChangelistName != name)
         {
            mChangelistName = name;
            mChangeList = CoreGlobals.getPerforce().GetNewChangeList(mChangelistName);//"Automated action: " + Path.GetFileNameWithoutExtension(mFiles[0]));


         }

         //foreach (string file in mFiles)
         //{
         //   mChangeList.AddOrEdit(file, true);
         //}
         return mChangeList;
      }


      public event EventHandler TopicsUpdated = null;

      public void NotifyTopicsUpdated()
      {
         if (TopicsUpdated != null)
         {
            TopicsUpdated.Invoke(this, null);
         }
      }


      public void InitFileLoadRevisions()
      {
         Dictionary<string, WorkTopic>.Enumerator it = mScenarioWorkTopics.GetEnumerator();
         while (it.MoveNext())
         {
            it.Current.Value.UpdateState(true);

         }


      }

      public void SmartInitChangeList(string name)
      {
         if (mChangeList == null || mChangelistName != name)
         {
            mChangelistName = name;

            Dictionary<int, string> lists = CoreGlobals.getPerforce().GetCurrentUserChangelists();
            //get changelist???
            int id = -1;

            Dictionary<int, string>.Enumerator it = lists.GetEnumerator();

            if (lists.ContainsValue(mChangelistName))
            {
               while (it.MoveNext())
               {
                  if (it.Current.Value == mChangelistName)
                  {
                     mChangeList = CoreGlobals.getPerforce().GetExistingChangeList(it.Current.Key);
                     break;
                  }
               }
            }
            else
            {
               mChangeList = CoreGlobals.getPerforce().GetNewChangeList(mChangelistName);
            }

            //if (lists.TryGetValue(mChangelistName, out id))
            //{
            //   mChangeList = CoreGlobals.getPerforce().GetExistingChangeList(id);
            //}
            //else
            //{
            //   mChangeList = CoreGlobals.getPerforce().GetNewChangeList(mChangelistName);//"Automated action: " + Path.GetFileNameWithoutExtension(mFiles[0]));
            //}
         }
      }




      public bool AddOrEdit(string topicName)
      {
         if (mChangelistName == "" || mChangeList == null || mScenarioWorkTopics.ContainsKey(topicName) == false)
            return false;

         List<string> errors = new List<string>();
         WorkTopic topic = mScenarioWorkTopics[topicName];
         foreach(string file in topic.Files)
         {
            if (File.Exists(file) == false)   //dont try to add the file if it does not exist..?
               continue;

            if (mChangeList.AddOrEdit(file) == false)
            {
               errors.Add(file);
            }
         }
         if(errors.Count > 0)
         {
            CoreGlobals.ShowMessage("Error Checking out files.  Please check perforce.");
            return false;
         }
         return true;
      }

      //public void WorkTopicManager

   }


   public class WorkTopic
   {
      public WorkTopic(string name, string description, string fileDescription, ICollection<string> files)
      {
         mName = name;
         mDescription = description;
         mFileDescription = fileDescription;
         Files = new List<string>();
         Files.AddRange(files);
         OptionalFiles = new List<string>();
      }

      string mFileDescription;
      public string FileDescription
      {
         get
         {
            return mFileDescription;
         }
         set
         {
            mFileDescription = value;
         }
      }

      string mName;
      public string Name
      {
         get
         {
            return mName;
         }
         set
         {
            mName = value;
         }
      }
      string mDescription;
      public string Description
      {
         get
         {
            return mDescription;
         }
         set
         {
            mDescription = value;
         }
      }

      bool mbDoNotSave = false;
      public bool DoNotSave
      {
         get
         {
            return mbDoNotSave;
         }
         set
         {
            mbDoNotSave = value;
         }
      }

      bool mbPaused = false;
      public bool Paused
      {
         get
         {
            return mbPaused;
         }
         set
         {
            mbPaused = value;
         }
      }

      public eDataPatterm mDataPattern = eDataPatterm.cPrimary;

      public List<string> Files;
      public List<string> OptionalFiles;

      public bool mbDoesNotTrackChanges = true;

      bool mbChanged = false;
      public bool Changed
      {
         get
         {
            return mbDoesNotTrackChanges || mbChanged;
         }
         set
         {
            mbChanged = value;
         }
      }
      public void TopicMarkSaved()
      {
         mbChanged = false;
      }

      public void SetFilesWriteable()
      {
         foreach (string s in Files)
         {
            if (File.Exists(s))
            {

               if ((File.GetAttributes(s) & FileAttributes.ReadOnly) == FileAttributes.ReadOnly)
               {
                  File.SetAttributes(s, FileAttributes.Normal);
               }
            }

         }

      }

      public bool AreFilesWritetable(out ICollection<string> blocked)
      {
         blocked = new List<string>();
         foreach (string s in Files)
         {
            if (File.Exists(s))
            {
               if ((File.GetAttributes(s) & FileAttributes.ReadOnly) == FileAttributes.ReadOnly)
               {
                  blocked.Add(s);
               }
            }
         }
         if (blocked.Count > 0)
         {
         
            Writeable = false;
            return false;
         }

         Writeable = true;
         return true;
      }



      bool mbWriteable = false;
      public bool Writeable
      {
         get
         {
            return mbWriteable;
         }
         set
         {
            mbWriteable = value;
         }
      }
      public void UpdateState()
      {

         UpdateState(false);
      }
      private FileSystemWatcher mFileWatcher = null;
      public void EnableFileWatcher()
      {
         if (mFileWatcher == null)
         {
            mFileWatcher = new FileSystemWatcher();
            mFileWatcher.Changed += new FileSystemEventHandler(mFileWatcher_Changed);
            
         }
         if(this.Files.Count > 0)
         {
            string dir = Path.GetDirectoryName(this.Files[0]);
            if (dir.ToLower().EndsWith("quickview"))
            {
               mFileWatcher.EnableRaisingEvents = false;
               mMemoryVersion.Clear();
               return;
            }

            mFileWatcher.IncludeSubdirectories = false;
            mFileWatcher.Path = dir;          
            mFileWatcher.NotifyFilter = NotifyFilters.Attributes | NotifyFilters.LastAccess | NotifyFilters.CreationTime | NotifyFilters.LastWrite | NotifyFilters.Size;
            mFileWatcher.EnableRaisingEvents = true;
            mMemoryVersion.Clear();
         }
      }

      void mFileWatcher_Changed(object sender, FileSystemEventArgs e)
      {
         //throw new Exception("The method or operation is not implemented.");
         string file = e.FullPath;

         if (this.Files.Contains(file) == false)
            return;
         if (mbPauseFileCheck == true)
            return;

         SimpleFileStatus s = CoreGlobals.getPerforce().GetFileStatusSimple(file);
         bool oldRevision = false;
         int perforceRevision = SimpleFileStatus.cInvalidVersion;
         if (s.TryGetHeadRevision(out perforceRevision))
         {
            if (mMemoryVersion.ContainsKey(file) && perforceRevision != mMemoryVersion[file])
            {
               oldRevision = true;
            }
         }

         if (oldRevision == true)
         {
            CoreGlobals.ShowMessage("Warning Files in memory do NOT match latest perforce version: \r\n" + file);

         }

      }
      public bool mbPauseFileCheck = false;

      public string mOwner = "";

      List<SimpleFileStatus> mStatus = new List<SimpleFileStatus>();

      Dictionary<string, int> mMemoryVersion = new Dictionary<string, int>();

      public void UpdateState(bool initFileLoadVersion)
      {

         //perfoce stuff
         ICollection<string> unwriteable;
         if (CoreGlobals.UsingPerforce == true)
         {

            int numInPerforce = 0;
            int numCheckedoutByOther = 0;
            int numCheckedOutByUser = 0;
            int numCheckedOutByBoth = 0;
            int numNotInPerfoce = 0;
            int numReadOnly = 0;
            int optionalFiles = 0;
            int numFiles = this.Files.Count;

            mStatus.Clear();

            if (this.Files.Count == 0)
            {
               mState = eWorkTopicFileState.cNoFiles;
               return;
            }

            mVersionStatus = eVersionStatus.cCurrent;
            foreach (string file in this.Files)
            {
               if (File.Exists(file) == false)
               {
                  numNotInPerfoce++;
                  if (OptionalFiles.Contains(file))
                  {
                     optionalFiles++;
                  }
                  continue;
               }

               if ((File.GetAttributes(file) & FileAttributes.ReadOnly) == FileAttributes.ReadOnly)
               {
                  numReadOnly++;
               }
               SimpleFileStatus s = CoreGlobals.getPerforce().GetFileStatusSimple(file);
               string owner = s.ActionOwner;
               if (owner != "none")
               {
                  mOwner = owner;
               }

               if (initFileLoadVersion == true)
               {
                  int version = SimpleFileStatus.cInvalidVersion;
                  if (s.TryGetHaveRevision(out version))
                  {
                     mMemoryVersion[file] = version;
                  }
                  else
                  {
                     mMemoryVersion[file] = SimpleFileStatus.cInvalidVersion;
                  }
               }

               bool oldRevision = false;
               int perforceRevision = SimpleFileStatus.cInvalidVersion;
               if (s.TryGetHeadRevision(out perforceRevision))
               {
                  if (mMemoryVersion.ContainsKey(file) && perforceRevision != mMemoryVersion[file])
                  {
                     oldRevision = true;
                  }
               }

               mStatus.Add(s);

               if (s.InPerforce && (s.IsLatestRevision == false || oldRevision))
               {
                  mVersionStatus = eVersionStatus.cOutofdate;
               }

               if (s.InPerforce)
               {
                  numInPerforce++;


               }
               else
               {
                  numNotInPerfoce++;

                  if (OptionalFiles.Contains(file))
                  {
                     optionalFiles++;
                  }

               }
               if (s.State == eFileState.CheckedOutByUserAndOther)
                  numCheckedOutByBoth++;
               else if (s.State == eFileState.CheckedOutByOther)
                  numCheckedoutByOther++;
               else if (s.State == eFileState.CheckedOutByUser)
                  numCheckedOutByUser++;
            }



            //determine state
            if (numCheckedoutByOther > 0 || numCheckedOutByBoth > 0)
            {
               mState = eWorkTopicFileState.cCheckedOutByOther;
            }
            else if ((numCheckedOutByUser + optionalFiles) == numFiles)
            {
               mState = eWorkTopicFileState.cCheckedOutByUser;
            }               
            else if ((numInPerforce + optionalFiles) == numFiles && numCheckedOutByBoth == 0)
            {
               mState = eWorkTopicFileState.cCheckedIn;
            }
                     
            else if (numNotInPerfoce > 0)
            {
               if(numReadOnly > 0)
               {
                  mState = eWorkTopicFileState.cLocalReadOnly;
               }
               else
               {
                  mState = eWorkTopicFileState.cLocalWriteable;
               }
            }
            else
            {
               mState = eWorkTopicFileState.cStateBusted;
               CoreGlobals.ShowMessage("Please ask for help");
               //This is and edge case and should not happen
            }
            
            //determine writeability
            if (numReadOnly > 0)
            {
               Writeable = false;
            }
            else
            {
               Writeable = true;
            }

         }
         else
         {
            if (this.AreFilesWritetable(out unwriteable) == true)
            {
               mState = eWorkTopicFileState.cLocalWriteable;
               Writeable = true;
            }
            else
            {
               mState = eWorkTopicFileState.cLocalReadOnly;
               Writeable = false;
            }
         }

      }
      public eWorkTopicFileState mState = eWorkTopicFileState.cStateNotSet;
      public eVersionStatus mVersionStatus = eVersionStatus.cStateNotSet;

      public enum eDataPatterm
      {
         cPrimary,
         cSecondary
      }

      public enum eWorkTopicFileState
      {
         cStateNotSet,
         cStateBusted,
         cCheckedOutByUser,
         cCheckedIn,
         cCheckedOutByOther,
         cLocalWriteable,
         cLocalReadOnly,
         cNoFiles,
      }

      public enum eVersionStatus
      {
         cStateNotSet,
         cCurrent,
         cOutofdate
      }

   }


   //[XmlRoot("GenericSettings")]
   //public class GenericSettings
   //{
   //   [XmlArrayItem(ElementName = "GenericSetting", Type = typeof(GenericSetting))]
   //   [XmlArray("Settings")]
   //   List<GenericSetting> mSettings = new List<GenericSetting>();

   //   [XmlIgnore]
   //   public Dictionary<string, string> Settings = new Dictionary<string, string>();

   //   public GenericSettings Load(string fileName)//, bool bClearOldSettings)
   //   {
   //      if (!File.Exists(fileName))
   //         return null;

   //      XmlSerializer s = new XmlSerializer(typeof(GenericSettings), new Type[] { });
   //      Stream st = File.OpenRead(fileName);

   //      GenericSettings set = ((GenericSettings)s.Deserialize(st)).mSettings;
   //      //if (bClearOldSettings)
   //      //{
   //      //   mSettings.Clear();
   //      //}         
   //      //set.mSettings;

   //      mSettings = set.mSettings;
   //   }
   //   public void Save(string fileName)
   //   {
   //      //update settings


   //      XmlSerializer s = new XmlSerializer(typeof(GenericSettings), new Type[] { });
   //      Stream st = File.Open(fileName, FileMode.Create);
   //      s.Serialize(st, this);
   //      st.Close();
   //   }

   //}
   //public class GenericSetting
   //{
   //   [XmlAttribute]
   //   public string Setting;
   //   [XmlAttribute]
   //   public string Value;
   //}


}




