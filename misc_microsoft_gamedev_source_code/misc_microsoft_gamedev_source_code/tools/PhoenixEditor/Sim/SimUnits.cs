using System;
using System.Collections.Generic;
using System.Text;
using Rendering;
using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;
using System.Drawing;
using System.IO;
using System.Xml;
using System.Xml.Serialization;
using System.Collections;
using System.Runtime.InteropServices;
using System.Diagnostics;
using System.ComponentModel;

using ModelSystem;
using EditorCore;

namespace SimEditor
{
   public class SimFileData
   {
      public ProtoObjectsXML mProtoObjectsXML;
      public DataSquadsXML mProtoSquadsXML;

      public Dictionary<string, SimUnitXML> mProtoObjectsByName = new Dictionary<string, SimUnitXML>();
      public Dictionary<string, SimUnitXML> mProtoObjectsByNameLower = new Dictionary<string, SimUnitXML>();
      public Dictionary<string, ProtoSquadXml> mProtoSquadsByName = new Dictionary<string, ProtoSquadXml>();
      public bool Load()
      {
         mProtoObjectsXML = LoadObjects();
         if (mProtoObjectsXML == null)
         {
            return false;
         }

         mProtoSquadsXML = DataSquadsXML.ReadSquads();
         if (mProtoSquadsXML == null)
         {
            return false;
         }

         mProtoObjectsByName.Clear();
         mProtoObjectsByNameLower.Clear();
         foreach(SimUnitXML obj in mProtoObjectsXML.mUnits)
         {
            if (!mProtoObjectsByName.ContainsKey(obj.mName))
            {
               mProtoObjectsByName.Add(obj.mName, obj);
               mProtoObjectsByNameLower.Add(obj.mName.ToLower(), obj);
            }
            else
            {
               CoreGlobals.getErrorManager().OnSimpleWarning(String.Format("Error proto objects: {0} is already in the file", obj.mName));
            }
         }


         mProtoSquadsByName.Clear();
         foreach (ProtoSquadXml obj in mProtoSquadsXML.mSquads)
         {
            if (!mProtoSquadsByName.ContainsKey(obj.Name))
            {
               mProtoSquadsByName.Add(obj.Name, obj);
            }
            else
            {
               CoreGlobals.getErrorManager().OnSimpleWarning(String.Format("Error proto squad: {0} is already in the file", obj.Name));
            }
         }


  
         return true;
      }

      public ProtoObjectsXML LoadObjects()
      {
         //string fileName = AppDomain.CurrentDomain.BaseDirectory + "\\sim\\proto.xml";
         string fileName = Path.Combine(CoreGlobals.getWorkPaths().mGameDataDirectory, "objects.xml");
         if (!File.Exists(fileName))
         {
            CoreGlobals.getErrorManager().OnSimpleWarning("Error loading proto object data.  Can't find " + fileName);

            return null;
         }
         XmlSerializer s = new XmlSerializer(typeof(ProtoObjectsXML), new Type[] { typeof(SimUnitXML) });
         Stream st = File.OpenRead(fileName);
         ProtoObjectsXML result = (ProtoObjectsXML)s.Deserialize(st);

         if(result == null)
         {
            CoreGlobals.getErrorManager().OnSimpleWarning("No proto objects loaded check " + fileName);

         }
         return result;
      }
      public ScenarioXML LoadScenario(string fileName)
      {
         if (!File.Exists(fileName))
            return null;

         //fileName = AppDomain.CurrentDomain.BaseDirectory + "\\sim\\" + fileName;//"\\sim\\test_sim.xml";
         XmlSerializer s = new XmlSerializer(typeof(ScenarioXML), new Type[] {  typeof(ScenarioSimUnitXML) });
         Stream st = File.OpenRead(fileName);
         return (ScenarioXML ) s.Deserialize(st);
      }
      public void SaveScenario(string fileName,ScenarioXML data)
      {
         if(File.Exists(fileName))
         {
            FileAttributes fa = File.GetAttributes(fileName);

            if ((fa & FileAttributes.ReadOnly) == FileAttributes.ReadOnly)
            {
               System.Windows.Forms.MessageBox.Show(fileName + " is not checked out from perforce. \n Please check it out and try again.");
               return;
            }
         }
         //fileName = AppDomain.CurrentDomain.BaseDirectory + "\\sim\\" + fileName;//"\\sim\\test_sim2.xml";
         XmlSerializer s = new XmlSerializer(typeof(ScenarioXML), new Type[] {  typeof(ScenarioSimUnitXML) });
         Stream st = File.Open(fileName, FileMode.Create);
         s.Serialize(st,data);
         st.Close();

         XMBProcessor.CreateXMB(fileName, false);
      }

      public ScenarioExtraXML LoadScenarioExtras(string fileName)
      {
         if (!File.Exists(fileName))
            return null;

         XmlSerializer s = new XmlSerializer(typeof(ScenarioExtraXML), new Type[] { typeof(ScenarioSimUnitXML) });
         Stream st = File.OpenRead(fileName);
         return (ScenarioExtraXML)s.Deserialize(st);
      }

      public void SaveScenarioExtras(string fileName, ScenarioExtraXML data)
      {
         if (File.Exists(fileName))
         {
            FileAttributes fa = File.GetAttributes(fileName);

            if ((fa & FileAttributes.ReadOnly) == FileAttributes.ReadOnly)
            {
               System.Windows.Forms.MessageBox.Show(fileName + " is not checked out from perforce. \n Please check it out and try again.");
               return;
            }
         }

         XmlSerializer s = new XmlSerializer(typeof(ScenarioExtraXML), new Type[] { typeof(ScenarioSimUnitXML) });
         Stream st = File.Open(fileName, FileMode.Create);
         s.Serialize(st, data);
         st.Close();

         XMBProcessor.CreateXMB(fileName, false);
      }
      public void CreateScenarioExtrasIfMissing(string fileName)
      {
         if (File.Exists(fileName) == false)
         {
            ScenarioExtraXML data = new ScenarioExtraXML();
            XmlSerializer s = new XmlSerializer(typeof(ScenarioExtraXML), new Type[] { typeof(ScenarioSimUnitXML) });
            Stream st = File.Open(fileName, FileMode.Create);
            s.Serialize(st, data);
            st.Close();

            XMBProcessor.CreateXMB(fileName, false);
         }
      }


   }

 

   [XmlRoot("Scenario")]
   public class ScenarioXML
   {
      [XmlElement("Terrain")]
      public TerrainScenarioXML mTerrain = new TerrainScenarioXML();

      [XmlElement("Sky")]
      public string mSkyBox;

      [XmlElement("TerrainEnv")]
      public string mTerrainEnv;

      [XmlElement("Lightset")]
      public string mLightset = "defaultGlobalLights";
      
      [XmlElement("Pathing")]
      public string mPathingFile;

      [XmlArrayItem(ElementName = "Cinematic", Type = typeof(ScenarioCinematicXML))]
      [XmlArray("Cinematics")]
      public List<ScenarioCinematicXML> mCinematics = new List<ScenarioCinematicXML>();

      [XmlArrayItem(ElementName = "Lightset", Type = typeof(ScenarioLightsetXML))]
      [XmlArray("Lightsets")]
      public List<ScenarioLightsetXML> mLightsets = new List<ScenarioLightsetXML>();

      [XmlArrayItem(ElementName = "TalkingHead", Type = typeof(ScenarioTalkingHeadXML))]
      [XmlArray("TalkingHeads")]
      public List<ScenarioTalkingHeadXML> mTalkingHeads = new List<ScenarioTalkingHeadXML>();      

      [XmlArrayItem(ElementName = "Diplomacy", Type = typeof(DiplomacyXml))]
      [XmlArray("Diplomacies")]
      public List<DiplomacyXml> mDiplomacyData = new List<DiplomacyXml>();


      [XmlElement("PlayerPlacement")]
      public PlayerPlacement mPlayerPlacement = new PlayerPlacement();

      [XmlArrayItem(ElementName = "Position", Type = typeof(PlayerPositionXML))]
      [XmlArray("Positions")]
      public List<PlayerPositionXML> mPlayerPosition = new List<PlayerPositionXML>();

      [XmlElement("Players")]
      public Players mPlayers = new Players();

      [XmlElement( "Objectives" )]
      public ObjectivesXML mObjectives = new ObjectivesXML();

      [XmlElement("Minimap")]
      public MinimapXML mMinimap = new MinimapXML();

      [XmlElement("BuildingTextureIndexUNSC")]
      public int mBuildingTextureIndexUNSC = 0;

      [XmlElement("BuildingTextureIndexCOVN")]
      public int mBuildingTextureIndexCOVN = 0;
      
      [XmlElement("DesignObjects")]
      public DesignObjects mDesignObjects = new DesignObjects();

      [XmlArrayItem(ElementName = "ObjectGroup", Type = typeof(ObjectGroup))]
      [XmlArray("ObjectGroups")]
      public List<ObjectGroup> mObjectGroups = new List<ObjectGroup>();

      [XmlArrayItem(ElementName = "Object", Type = typeof(ScenarioSimUnitXML))]
      [XmlArray("Objects")]
      public List<ScenarioSimUnitXML> mUnits = new List<ScenarioSimUnitXML>();

      //[XmlArrayItem(ElementName = "Squad", Type = typeof(ScenarioSquadXML))]
      //[XmlArray("Squads")]
      //public List<ScenarioSquadXML> mSquads = new List<ScenarioSquadXML>();

      [XmlArrayItem(ElementName = "Light", Type = typeof(LightXML))]
      [XmlArray("Lights")]
      public List<LightXML> mLights = new List<LightXML>();

      //[XmlArrayItem(ElementName = "Player", Type = typeof(LightXML))]
      //[XmlArray("Players")]
      //public List<LightXML> mLights = new List<LightXML>();


      //[XmlElement("TriggerSystem")]
      ////[XmlIgnore]
      //public TriggerRoot mTriggers = new TriggerRoot();

      [XmlElement("TriggerSystem", typeof(TriggerRoot))]
      public List<TriggerRoot> mTriggers = new List<TriggerRoot>();

      [XmlElement("ExternalTriggerScript", typeof(ExternalScriptInfo))]
      public List<ExternalScriptInfo> mExternalScripts = new List<ExternalScriptInfo>();

      [XmlElement("NextID")]
      public int mNextID = 0;

      [XmlElement("EditorOnlyData")]
      public EditorOnlyData mEditorOnlyData = new EditorOnlyData();

      [XmlElement("SimBoundsMinX")]
      public int mSimBoundsMinX = -1;

      [XmlElement("SimBoundsMinZ")]
      public int mSimBoundsMinZ = -1;

      [XmlElement("SimBoundsMaxX")]
      public int mSimBoundsMaxX = -1;

      [XmlElement("SimBoundsMaxZ")]
      public int mSimBoundsMaxZ = -1;

      [XmlElement("SoundBank")]
      public List<string> mSoundbanks = new List<string>();

      [XmlElement("ScenarioWorld")]
      public string mScenarioWorld;

      [XmlElement("AllowVeterancy")]
      public bool mbAllowVeterancy = true;

      [XmlArrayItem(ElementName = "Object", Type = typeof(string))]
      [XmlArray("GlobalExcludeObjects")]
      public List<string> mGlobalExcludeObjects = new List<string>();
   }

   [XmlRoot("Scenario")]
   public class ScenarioExtraXML
   {
      [XmlArrayItem(ElementName = "Object", Type = typeof(ScenarioSimUnitXML))]
      [XmlArray("Objects")]
      public List<ScenarioSimUnitXML> mUnits = new List<ScenarioSimUnitXML>();

   }

   [XmlRoot("EditorOnlyData")]
   public class EditorOnlyData
   {
      [XmlArrayItem(ElementName = "HelperAreaObject", Type = typeof(HelperAreaObject))]
      [XmlArray("HelperAreas")]
      public List<HelperAreaObject> mHelperAreas = new List<HelperAreaObject>();

      [XmlArrayItem(ElementName = "HelperAreaBoxObject", Type = typeof(HelperAreaBoxObject))]
      [XmlArray("HelperAreaBoxes")]
      public List<HelperAreaBoxObject> mHelperAreaBoxes = new List<HelperAreaBoxObject>();

      [XmlArrayItem(ElementName = "HelpePositionObject", Type = typeof(HelperPositionObject))]
      [XmlArray("HelpePositions")]
      public List<HelperPositionObject> mHelperPositions = new List<HelperPositionObject>();

      [XmlArrayItem(ElementName = "Object", Type = typeof(ScenarioSimUnitXML))]
      [XmlArray("EditorOnlyObjects")]
      public List<ScenarioSimUnitXML> mEditorOnlyObjects = new List<ScenarioSimUnitXML>();
   }

   [XmlRoot("Scraps")]
   public class ScenarioScrapsXML
   {
      [XmlArrayItem(ElementName = "Object", Type = typeof(ScenarioSimUnitXML))]
      [XmlArray("Objects")]
      public List<ScenarioSimUnitXML> mUnits = new List<ScenarioSimUnitXML>();

      [XmlArrayItem(ElementName = "ObjectGroup", Type = typeof(ObjectGroup))]
      [XmlArray("ObjectGroups")]
      public List<ObjectGroup> mObjectGroups = new List<ObjectGroup>();


      [XmlElement("EditorOnlyData")]
      public EditorOnlyData mEditorOnlyData = new EditorOnlyData();
   }

   [XmlRoot("Terrain")]
   public class TerrainScenarioXML
   {
      [XmlText]
      public string mTerrainFileName = " ";

      [XmlAttribute("LoadVisRep")]
      public bool mLoadVisRep = true;
   }

   


   #region SIM
   [XmlRoot("Objects")]
   public class ProtoObjectsXML
   {
      [XmlAttribute("version")]
      public string version;
      [XmlElement("Object", typeof(SimUnitXML))]
      public List<SimUnitXML> mUnits = new List<SimUnitXML>();
   }
   
   //protoobject
   [XmlRoot("Object")]
   public class SimUnitXML : ObjectXML
   {
      public Image getImage()
      {
         Image i = null;         
         return i;
      }
   
      public string getGrannyFileName()
      {
         if (mAnimFile == null)
            return "";
         string path = Path.Combine(EditorCore.CoreGlobals.getWorkPaths().mGameArtDirectory, mAnimFile);
         path = Path.GetDirectoryName(path);
         if (Directory.Exists(path))
         {
            string[] files;

            files = Directory.GetFiles(path, Path.GetFileNameWithoutExtension(mAnimFile) + ".gr2");
            if (files.Length >= 1)
               return files[0];

            files = Directory.GetFiles(path, "*" + Path.GetFileNameWithoutExtension(mAnimFile) + "*.gr2");
            if (files.Length >= 1)
               return files[0];

     

            files = Directory.GetFiles(path, "*.gr2");
            if (files.Length >= 1)
               return files[0];
         }

         return "";
      }

      static Dictionary<string, List<string>> mGrannyFileNamesHash = new Dictionary<string, List<string>>();

      /// <summary>
      /// Gets a list of meshes used.  Note: always tries to load from .vis files.
      /// </summary>
      /// <returns></returns>
      public List<string> getGrannyFileNames()
      {
         List<string> results = new List<string>();
         string fileName = "";
         try
         {
            if (mAnimFile == null)
               return results;
            string path = Path.Combine(EditorCore.CoreGlobals.getWorkPaths().mGameArtDirectory, mAnimFile);

            fileName = Path.Combine(CoreGlobals.getWorkPaths().mGameArtDirectory, mAnimFile);

            //Always get the .vis? what does the game do
            fileName = Path.ChangeExtension(fileName, ".vis");

            results = getGrannyFileNames(fileName, true);

         }
         catch(System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnSimpleWarning(String.Format("Error parsing AnimXML: {0}.  {1}", fileName, ex.ToString()));
         }
         return results;
      }
      static public List<string> getGrannyFileNames(string fileName)
      {
         return getGrannyFileNames(fileName, false);
      }
      static public List<string> getGrannyFileNames(string fileName, bool usecache)
      {
         List<string> results = new List<string>();

         try
         {
            if (!File.Exists(fileName))
            {
               CoreGlobals.getErrorManager().OnSimpleWarning(String.Format("Can't find VisXml: {0}", fileName));
               return results;
            }

            //Try to walk the anim xml structure
            if (usecache && mGrannyFileNamesHash.ContainsKey(fileName))
            {
               return mGrannyFileNamesHash[fileName];
            }
            else
            {
               XmlDocument doc = new XmlDocument();
               doc.Load(fileName);

               XmlNodeList nodes = null;
               if (Path.GetExtension(fileName).Contains("xml"))
               {
                  nodes = doc.SelectNodes("//assetreference[@type='grannymodel']/file");
               }
               else
               {
                  nodes = doc.SelectNodes("//asset[@type='Model']/file");
               }

               foreach (XmlNode n in nodes)
               {
                  results.Add(Path.Combine(EditorCore.CoreGlobals.getWorkPaths().mGameArtDirectory, n.InnerText) + ".gr2");
               }
            }
            if (results.Count == 0)
            {
               CoreGlobals.getErrorManager().OnSimpleWarning(String.Format("No assetreferences found in AnimXML: {0}", fileName));
            }

         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnSimpleWarning(String.Format("Error parsing AnimXML: {0}.  {1}", fileName, ex.ToString()));
         }

         mGrannyFileNamesHash[fileName] = results;

         return results;

      }

      //static public List<string> getGrannyFileNames(string fileName)
      //{
      //   List<string> results = new List<string>();
         
      //   try
      //   {
      //      if (!File.Exists(fileName))
      //      {

      //         CoreGlobals.getErrorManager().OnSimpleWarning(String.Format("Can't find AnimXML: {0}", fileName));
      //         return results;
      //      }

      //      XmlSerializer s = new XmlSerializer(typeof(animfile), new Type[] { });//new Type[] { typeof(animfile) });
      //      Stream st = File.OpenRead(fileName);
      //      //lowercase everything (bitches)
      //      StreamReader sr = new StreamReader(st);
      //      string xml = sr.ReadToEnd();
      //      xml = xml.ToLower();
      //      StringReader streader = new StringReader(xml);
      //      st.Close();
      //      animfile file = (animfile)s.Deserialize(streader);

      //      if (file.component != null)
      //         foreach (component c in file.component)
      //         {
      //            if (c.assetreference != null)
      //               foreach (assetreference r in c.assetreference)
      //               {
      //                  string filename = Path.Combine(EditorCore.CoreGlobals.getWorkPaths().mGameArtDirectory, r.file) + ".gr2";
      //                  if (File.Exists(filename))
      //                     results.Add(filename);
      //               }
      //         }

      //      if (file.attachment != null)
      //         foreach (animfileAttachment a in file.attachment)
      //         {
      //            if (a.component != null)
      //               foreach (component c in a.component)
      //               {
      //                  if (c.assetreference != null)
      //                     foreach (assetreference r in c.assetreference)
      //                     {
      //                        if (r.file == null)
      //                           continue;
      //                        string filename = Path.Combine(EditorCore.CoreGlobals.getWorkPaths().mGameArtDirectory, r.file) + ".gr2";
      //                        if (File.Exists(filename))
      //                           results.Add(filename);
      //                     }
      //               }

      //         }

      //      if (results.Count == 0)
      //      {
      //         CoreGlobals.getErrorManager().OnSimpleWarning(String.Format("No assetreferences found in AnimXML: {0}", fileName));
      //      }

      //   }
      //   catch (System.Exception ex)
      //   {
      //      CoreGlobals.getErrorManager().OnSimpleWarning(String.Format("Error parsing AnimXML: {0}.  {1}", fileName, ex.ToString()));


      //   }

      //   return results;
      //}   


      public string getRawGrannyFileName()
      {
         if (mAnimFile == null)
            return "";
         string path = Path.Combine(EditorCore.CoreGlobals.getWorkPaths().mGameArtDirectory,mAnimFile);
         path = Path.GetDirectoryName(path);
         if(Directory.Exists(path))
         {
            string[] files = Directory.GetFiles(path, "*raw*.gr2");
            if (files.Length == 1)
               return files[0];
         }

         return "";
      }
   
   }

 

   [XmlRoot("ObjectGroup")]
   public class ObjectGroup
   {
      string mName = "NewGroup";
      int mId;
      bool mVisible = true;
      bool mLocked = false;
      bool mBB = false;
      bool mExploreGroup = false;
      [XmlAttribute]
      public int ID
      {
         get
         {
            return mId;
         }
         set
         {
            mId = value;
         }
      }
      [XmlAttribute]
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
      [XmlAttribute]
      public bool Visible
      {
         get
         {
            return mVisible;
         }
         set
         {
            mVisible = value;
         }
      }
      [XmlAttribute]
      public bool Locked
      {
         get
         {
            return mLocked;
         }
         set
         {
            mLocked = value;
         }
      }
      [XmlAttribute]
      public bool BB
      {
         get
         {
            return mBB;
         }
         set
         {
            mBB = value;
         }
      }
      [XmlAttribute]
      public bool ExploreGroup
      {
         get
         {
            return mExploreGroup;
         }
         set
         {
            mExploreGroup = value;
         }
      }
   }

   [XmlRoot("TalkingHead")]
   public class ScenarioTalkingHeadXML
   {
      [XmlText]
      public string mName;

      [XmlAttribute("ID")]
      public int mID = -1;
   }


   [XmlRoot("Cinematic")]
   public class ScenarioCinematicXML
   {
      [XmlText]
      public string mName;

      [XmlAttribute("ID")]
      public int mID = -1;
   }

   [XmlRoot("Lightset")]
   public class ScenarioLightsetXML
   {
      [XmlText]
      public string mName;

      [XmlAttribute("ID")]
      public int mID = -1;

      [XmlAttribute("LightProbeObject")]
      public string mLightProbeObject;
   }

   [XmlRoot("Object")]
   public class ScenarioSimUnitXML : ICloneable, IHasGroup
   {
      [XmlAttribute("IsSquad")]
      public bool mIsSquad = false;

      [XmlText]
      public string mProtoUnit;

      [XmlAttribute("Player")]
      public int mPlayer = 1;

      [XmlAttribute("ID")]
      public int mID = -1;

      [XmlIgnore]
      public Vector3 mPosition = Vector3.Empty;
      [XmlIgnore]
      public Vector3 mForward = Vector3.Empty;
      [XmlIgnore]
      public Vector3 mRight = Vector3.Empty;
      [XmlAttribute("Position")]
      public string Position
      {
         set
         {
            mPosition = TextVectorHelper.FromString(value);
         }
         get
         {
            return TextVectorHelper.ToString(mPosition);
         }
      }
      [XmlAttribute("Forward")]
      public string Forward
      {
         set
         {
            mForward = TextVectorHelper.FromString(value);
         }
         get
         {
            return TextVectorHelper.ToString(mForward);
         }
      }
      [XmlAttribute("Right")]
      public string Right
      {
         set
         {
            mRight = TextVectorHelper.FromString(value);
         }
         get
         {
            return TextVectorHelper.ToString(mRight);
         }
      }
      [XmlAttribute("TintValue")]
      public float mTintValue = 1.0f;

      [XmlElement("Flag", typeof(string))]
      public List<string> mFlags = new List<string>();

      public bool hasFlag(string flag)
      {
         if (mFlags == null) return false;

         if (mFlags.Contains(flag))
         {
            return true;
         }
         return false;
      }

      [XmlAttribute("EditorName")]
      public string Name = "";

      //public int Group = -1;
      int mGroup = -1;
      [XmlAttribute("Group")]
      public int Group
      {
         get
         {
            return mGroup;
         }
         set
         {
            mGroup = value;
         }
      }

      int mVisualVariationIndex = 0;
      [XmlAttribute("VisualVariationIndex")]
      public int VisualVariationIndex
      {
         get
         {
            return mVisualVariationIndex;
         }
         set
         {
            mVisualVariationIndex = value;
         }
      }

      #region ICloneable Members

      public object Clone()
      {
         ScenarioSimUnitXML newCopy = (ScenarioSimUnitXML)this.MemberwiseClone();
         
         newCopy.mFlags = new List<string>();
         newCopy.mFlags.AddRange(this.mFlags);


         return newCopy;
         
      }

      #endregion
   }

   //<PlayerPlacement Type="Grouped" Spacing="1" />
   //TYPE “Fixed”, “Random”, or “Grouped”
   [XmlRoot("PlayerPlacement")]
   public class PlayerPlacement
   {
      [XmlIgnore]
      public string mType = "Fixed";
      [XmlAttribute("Type")]
      public string Type
      {
         set
         {
            mType = value;
         }
         get
         {
            return mType;
         }
      }

      static public string[] GetTypeValues()
      {
         return new string[] { "Fixed", "Consecutive", "Grouped", "Random" };
      }
      

      [XmlIgnore]
      public int mSpacing = 0;
      [XmlAttribute("Spacing")]
      public int Spacing
      {
         set
         {
            mSpacing = value;
         }
         get
         {
            return mSpacing;
         }
      }
   }

   [XmlRoot("Position")]
   public class PlayerPositionXML
   {

      //Note the “Group” value is gone, and there is a new “Player” value. If the player value is set to -1, then the position is not assigned to any specific player. If it’s set to an actual player number (such as 1, 2, 3, etc), then that position is fixed for that specific player.
      //The “Number” value is used differently based on the placement type:
      //Fixed – Each player that doesn’t have a specific position set uses the positions in a consecutive order… the first player uses position 1, the second position 2, etc.
      //Random – The players are ordered by team and an initial random starting position is selected. Then the players are placed in a consecutive order based on the number value. The spacing value allows the designer to specify they want the teams to be separated by a minimum number of positions (or not if the spacing is set to 0).
      //Grouped – Players within a team are grouped together and use positions that are numbered the same… For example, players on team 1 will use all the positions with the number set to 1. Team 2 will use all the positions with the number set to 2. There is randomization between teams, so sometimes team 2 can use the number 1 positions. Also, the player order within each team is randomized.

      //    <Position Player="-1" Number="1" Position="41.8954,0.0000,84.0144" DefaultCamera="false" CameraYaw="200.0" CameraPitch="60.0" CameraZoom="40.0"/>

      [XmlIgnore]
      public int mPlayer = -1;
      [XmlAttribute("Player")]
      public string Player
      {
         set
         {
            mPlayer = Convert.ToInt32(value);
         }
         get
         {
            return Convert.ToString(mPlayer);
         }
      }

      [XmlIgnore]
      public int mNumber = 1;
      [XmlAttribute("Number")]
      public string Number
      {
         set
         {
            int oldnumber = mNumber;
            mNumber = Convert.ToInt32(value);
            if(NumberChanged != null && oldnumber != mNumber)
            {
               NumberChanged.Invoke(this, null);
            }
         }
         get
         {
            return Convert.ToString(mNumber);
         }
      }

      [XmlIgnore]
      public Vector3 mPosition = Vector3.Empty;
      [XmlAttribute("Position")]
      public string Position
      {
         set
         {
            mPosition = TextVectorHelper.FromString(value);
         }
         get
         {
            return TextVectorHelper.ToString(mPosition);
         }
      }

      [XmlIgnore]
      public Vector3 mForward = Vector3.Empty;
      [XmlAttribute("Forward")]
      public string Forward
      {
         set
         {
            mForward = TextVectorHelper.FromString(value);
         }
         get
         {
            return TextVectorHelper.ToString(mForward);
         }
      }

      [XmlIgnore]
      public bool mDefaultCamera = true;
      [XmlAttribute("DefaultCamera")]
      public bool DefaultCamera
      {
         set
         {
            mDefaultCamera = value;
         }
         get
         {
            return mDefaultCamera;
         }

      }

      [XmlIgnore]
      public float mCameraYaw = 317.2f;
      [XmlAttribute("CameraYaw")]
      public float CameraYaw
      {
         set
         {
            mCameraYaw = value;
         }
         get
         {
            return mCameraYaw;
         }
      }
      [XmlIgnore]
      public float mCameraPitch = 42;
      [XmlAttribute("CameraPitch")]
      public float CameraPitch
      {
         set
         {
            mCameraPitch = value;
         }
         get
         {
            return mCameraPitch;
         }
      }
      [XmlIgnore]
      public float mCameraZoom = 85;
      [XmlAttribute("CameraZoom")]
      public float CameraZoom
      {
         set
         {
            mCameraZoom = value;
         }
         get
         {
            return mCameraZoom;
         }
      }
      [XmlIgnore]
      public int mUnitStartObject1 = -1;
      [XmlAttribute("UnitStartObject1")]
      public int UnitStartObject1
      {
         set
         {
            mUnitStartObject1 = value;
         }
         get
         {
            return mUnitStartObject1;
         }
      }
      [XmlIgnore]
      public int mUnitStartObject2 = -1;
      [XmlAttribute("UnitStartObject2")]
      public int UnitStartObject2
      {
         set
         {
            mUnitStartObject2 = value;
         }
         get
         {
            return mUnitStartObject2;
         }
      }
      [XmlIgnore]
      public int mUnitStartObject3 = -1;
      [XmlAttribute("UnitStartObject3")]
      public int UnitStartObject3
      {
         set
         {
            mUnitStartObject3 = value;
         }
         get
         {
            return mUnitStartObject3;
         }
      }
      [XmlIgnore]
      public int mUnitStartObject4 = -1;
      [XmlAttribute("UnitStartObject4")]
      public int UnitStartObject4
      {
         set
         {
            mUnitStartObject4 = value;
         }
         get
         {
            return mUnitStartObject4;
         }
      }
      [XmlIgnore]
      public int mRallyStartObject = -1;
      [XmlAttribute("RallyStartObject")]
      public int RallyStartObject
      {
         set
         {
            mRallyStartObject = value;
         }
         get
         {
            return mRallyStartObject;
         }
      }
      public event EventHandler NumberChanged;
   }
 
      //<Pops>
      //  <Pop type="Unit" max="500">250</Pop>
      //</Pops> 
   [XmlRoot("Pop")]
   public class PlayerPop
   {
      string mPopType = "";
      int mMax = 500;
      int mPop = 250;
      [XmlAttribute("type")]
      public string PopType
      {
         get
         {
            return mPopType;
         }
         set
         {
            mPopType = value;
         }
      }
      [XmlAttribute("max")]
      public int Max
      {
         get
         {
            return mMax;
         }
         set
         {
            mMax = value;
         }
      }
      [XmlText]
      public int Pop
      {
         get
         {
            return mPop;
         }
         set
         {
            mPop = value;
            if (mPop > mMax)
               mPop = mMax;
         }
      }
   }


   //This part is not generated automatically
   public partial class PlayersPlayer
   {
      public PlayersPlayer()
      {

         DefaultSettings();
      }
      public void DefaultSettings()
      {
         this.Civ = "UNSC";
         this.Team = 1;
         this.Color = 1;

         this.UsePlayerSettings = true;
         this.UseStartingUnits = true;
         this.DefaultResources = true;
         this.Controllable = true;
      }
      public PlayersPlayer(string name, int color, int team) //: base()
      {
         DefaultSettings();
         //set defaults here..
         this.Name = name;
         this.Color = (byte)color;
         this.Team = (byte)team;
      }

      [XmlArrayItem(ElementName = "Pop", Type = typeof(PlayerPop))]
      [XmlArray("Pops")]
      public List<PlayerPop> mPlayerPop = new List<PlayerPop>();
   }
   public partial class Data
   {
      public Data()
      {
         NumberPlayers = 4;
         PlayerPlacement.Spacing = 1;
         PlayerPlacement.Type = "Fixed";
      }
   }


   #endregion

   #region Lights

   
   [XmlRoot("Light")]
   public class LightXML : ICloneable
   {
      //SHARED
      
      [XmlElement("Name")]
      public string mName = "LocalLight";
      [XmlIgnore, Description("Light Name"), Category("Data")]
      public string Name
      {
         get
         {
            return mName;
         }
         set
         {
            mName = value;
            Changed();
         }

      }

      //SHARED
      [XmlElement("Type")]
      public string mType = "omni";    //"omni", "spot"

      [XmlElement("Priority")]
      public int mPriority = 127;
      [XmlIgnore, Description("Defines how important this light is in fading calculations. [-127,127]"), Category("Data")]
      public int Priority
      {
         get
         {
            return mPriority;
         }
         set
         {
            int val = value;

            if (val > 127)
            {
               val = 127;
            }
            if (val < -127)
            {
               val = -127;
            }
            mPriority = val;
            Changed();
         }

      }

      [XmlElement("Position")]
      public string mPosition = "0,0,0";
      [XmlIgnore, Category("Data")]
      public Vector3 Position
      {
         set
         {
            mPosition = TextVectorHelper.ToString(value);
            Changed();
         }
         get
         {
            return TextVectorHelper.FromString(mPosition);

         }
      }

      [XmlElement("Radius")]
      public float mRadius = 4;
      [XmlIgnore, Category("Values")]
      public float Radius
      {
         get
         {
            return mRadius;
         }
         set
         {
            mRadius = value;
            Changed();
         }
      }

      [XmlElement("Specular")]
      public bool mSpecular = false;
      [XmlIgnore, Description("Does this light allow specular."), Category("Values")]
      public bool Specular
      {
         get
         {
            return mSpecular;
         }
         set
         {
            mSpecular = value;
            Changed();
         }
      }

      //<Intensity>10.0</Intensity>
      [XmlElement("Intensity")]
      public float mIntensity = 10;
      [XmlIgnore, Category("Values")]
      public float Intensity
      {
         get
         {
            return mIntensity;
         }
         set
         {
            mIntensity = value;
            Changed();
         }
      }

      [XmlElement("Color")]
      public string mColor = "255,255,255";
      [XmlIgnore, Description("Color of this light."), Category("Values")]
      public Color LightColor
      {
         get
         {
            string[] values = mColor.Split(',');
            //Color c = Color.FromArgb(Math.Min(255, (int)(System.Convert.ToSingle(values[0]) * 255)), Math.Min(255, (int)(System.Convert.ToSingle(values[1]) * 255)), Math.Min(255, (int)(System.Convert.ToSingle(values[2]) * 255)));
            Color c = Color.FromArgb(System.Convert.ToInt32(values[0]) , System.Convert.ToInt32(values[1]), System.Convert.ToInt32(values[2]));
            return c;
         }
         set
         {
            mColor = String.Format("{0}, {1}, {2}", value.R , value.G , value.B );
            //mColor = String.Format("{0}, {1}, {2}", value.R / 255.0, value.G / 255.0, value.B / 255.0);
            Changed();
         }
      }



      [XmlElement("Shadows")]
      public bool mShadows = true;
      [XmlIgnore, Description("Does this light cast shadows? [t,f]"), Category("Shadowing")]
      public bool Shadows
      {
         get
         {
            return mShadows;
         }
         set
         {
            mShadows = value;
            
            Changed();
         }
      }

      [XmlElement("ShadowDarkness")]
      public float mShadowDarkness = 0.5f;
      [XmlIgnore, Description("Value of how dark a shadow cast by this light is. [0,1]"), Category("Shadowing")]
      public float ShadowDarkness
      {
         get
         {
            return mShadowDarkness;
         }
         set
         {
            mShadowDarkness = value;
            Changed();
         }
      }

      [XmlElement("Fogged")]
      public bool mFogged = true;
      [XmlIgnore, Description("Is this light affected by fog? [t,f]"), Category("Values")]
      public bool Fogged
      {
         get
         {
            return mFogged;
         }
         set
         {
            mFogged = value;
            Changed();
         }
      }

      [XmlElement("FoggedShadows")]
      public bool mFoggedShadows = true;
      [XmlIgnore, Description("Does this light cast shadows in fog? [t,f]"), Category("Shadowing")]
      public bool FoggedShadows
      {
         get
         {
            return mFoggedShadows;
         }
         set
         {
            mFoggedShadows = value;
            Changed();
         }
      }

      [XmlElement("LightBuffered")]
      public bool mLightBuffered = false;
      [XmlIgnore, Description("LightBuffered? [t,f]"), Category("Data")]
      public bool LightBuffered
      {
         get
         {
            return mLightBuffered;
         }
         set
         {
            mLightBuffered = value;

            Changed();
         }
      }

      [XmlElement("TerrainOnly")]
      public bool mTerrainOnly = false;
      [XmlIgnore, Description("Is this light a terrain additive light? [t,f]"), Category("Data")]
      public bool TerrainOnly
      {
         get
         {
            return mTerrainOnly;
         }
         set
         {
            mTerrainOnly = value;
            
            Changed();
         }
      }

      [XmlElement("FarAttnStart")]
      public float mFarAttnStart = 0.5f;  // [0,.999]
      [XmlIgnore, Description("Sets the amount before falloff starts. [0,0.999]"), Category("Values")]
      public float FarAttnStart
      {
         get
         {
            return mFarAttnStart;
         }
         set
         {

            mFarAttnStart = BMathLib.Clamp(value, 0, 0.999f);
            Changed();
         }
      }

      [XmlElement("DecayDist")]
      public float mDecayDist = 70.0f;  // [.01,5000], may be > than radius
      [XmlIgnore, Description("Decay distance"), Category("Values")]
      public float DecayDist
      {
         get
         {
            return mDecayDist;
         }
         set
         {
            mDecayDist = value;
            Changed();
         }
      }

      
      //OMNI ONLY


      //SPOT ONLY
      [XmlElement("Direction")]
      public string mDirection = "0,-1,0";
      [XmlIgnore, Category("Data")]
      public Vector3 Direction
      {
         set
         {
            mDirection = TextVectorHelper.ToString(value);
            Changed();
         }
         get
         {
            return TextVectorHelper.FromString(mDirection);
         }
      }

      [XmlElement("OuterAngle")]
      public float mOuterAngle = 70.0f;
      [XmlIgnore, Description("Outer angle for the spotlight"), Category("Values"), System.ComponentModel.Editor]
      public float OuterAngle
      {
         get
         {
            //if (mInnerAngle > mOuterAngle)
            //{
            //   float temp = mOuterAngle;
            //   mOuterAngle = mInnerAngle;
            //   mInnerAngle = temp;
            //}

            return mOuterAngle;
         }
         set
         {

            mOuterAngle = value;
            //if (mInnerAngle > mOuterAngle)
            //   mOuterAngle = mInnerAngle;
            Changed();
         }
      }

      [XmlElement("InnerAngle")]
      public float mInnerAngle = 40.0f;
      [XmlIgnore, Description("Inner angle for the spotlight"), Category("Values")]
      public float InnerAngle
      {
         get
         {
            //if (mInnerAngle > mOuterAngle)
            //{
            //   float temp = mOuterAngle;
            //   mOuterAngle = mInnerAngle;
            //   mInnerAngle = temp;
            //}
            return mInnerAngle;
         }
         set
         {
            mInnerAngle = value;
            //if (mInnerAngle > mOuterAngle)
            //   mInnerAngle = mOuterAngle;

            Changed();
         }
      }




      public bool SettingsChanged() { bool temp = mSettingsChanged; mSettingsChanged = false; return temp; }
      private bool mSettingsChanged = false;
      private void Changed() 
      {
         SimGlobals.getSimMain().SetLightsDirty();
         mSettingsChanged = true; 
      }

     

      public object Clone()
      {
         object newxml = base.MemberwiseClone();
         //throw new Exception("The method or operation is not implemented.");
         return newxml;
      }

    
   }
   

   [XmlRoot("lightSet")]
   public class lightSetXML
   {


      [XmlElement("EnvironmentMap")]
      public string environmentMap;
      [XmlIgnore]
      public string EnvironmentMap
      {
         get
         {
            return environmentMap;
         }
         set
         {
            environmentMap = value;
            SimGlobals.getSimMain().SetLightsDirty();
         }
      }

      private string mObjectPropertyForFLSGen = "";
      [XmlElement("ObjectPropertyForFLSGen")]
      public string ObjectPropertyForFLSGen
      {
         get { return mObjectPropertyForFLSGen; }
         set { mObjectPropertyForFLSGen = value; }
      }
      public int getObjectIDForFLSGen()
      {
         {
            StringDecorator dec;
            if (StringDecorator.TryParse(ObjectPropertyForFLSGen, out dec))
               return Convert.ToInt32(dec.mDecorator);
            return -1;
         }
      }




      [XmlElement("backgroundColor")]
      public string backgroundColor = "90,90,90";
      [XmlIgnore]
      public Color BackgroundColor
      {
         get
         {
            string[] values = backgroundColor.Split(',');
            Color c = Color.FromArgb(System.Convert.ToInt32(values[0]), System.Convert.ToInt32(values[1]), System.Convert.ToInt32(values[2]));
            return c;
         }
         set
         {
            backgroundColor = String.Format("{0}, {1}, {2}", value.R, value.G, value.B);
            SimGlobals.getSimMain().SetLightsDirty();

         }
      }
      [XmlElement("SHFillIntensity")]
      public float shFillIntensity = 1.0f;
      [XmlIgnore]
      public float SHFillIntensity
      {
         get
         {
            return shFillIntensity;
         }
         set
         {
            shFillIntensity = value;
            SimGlobals.getSimMain().SetLightsDirty();
         }

      }

      [XmlElement("LGTIntensityScale")]
      public float mLGTIntensityScale = 1.0f;
      [XmlIgnore, Description("LGTIntensityScale. [.1,16]"), Category("LGTIntensityScale")]
      public float LGTIntensityScale
      {
         get
         {
            return mLGTIntensityScale;
         }
         set
         {
            mLGTIntensityScale = value;
            SimGlobals.getSimMain().SetLightsDirty();
         }
      }
      [XmlElement("LGTParticleIntensityScale")]
      public float mLGTParticleIntensityScale = 1.0f;
      [XmlIgnore, Description("LGTParticleIntensityScale. [.1,16]"), Category("LGTParticleIntensityScale")]
      public float LGTParticleIntensityScale
      {
         get
         {
            return mLGTParticleIntensityScale;
         }
         set
         {
            mLGTParticleIntensityScale = value;
            SimGlobals.getSimMain().SetLightsDirty();
         }
      }      

      [XmlElement("brightnessIntensity")]
      public float brightnessIntensity = 1.0f;
      [XmlIgnore]
      public float BrightnessIntensity
      {
         get
         {
            return brightnessIntensity;
         }
         set
         {
            brightnessIntensity = value;
            SimGlobals.getSimMain().SetLightsDirty();
         }
      }

      [XmlElement("sunInclination")]
      public float sunInclination;
      [XmlIgnore]
      public float SunInclination
      {
         get
         {
            return sunInclination;
         }
         set
         {
            sunInclination = value;
            SimGlobals.getSimMain().SetLightsDirty();
         }

      }


      [XmlElement("sunRotation")]
      public float sunRotation;
      [XmlIgnore]
      public float SunRotation
      {
         get
         {
            return sunRotation;
         }
         set
         {
            sunRotation = value;
            SimGlobals.getSimMain().SetLightsDirty();

         }

      }

      [XmlElement("sunUnitColor")]
      public string sunUnitColor;
      [XmlIgnore]
      public Color SunUnitColor
      {
         get
         {
            string[] values = sunUnitColor.Split(',');
            Color c = Color.FromArgb(System.Convert.ToInt32(values[0]), System.Convert.ToInt32(values[1]), System.Convert.ToInt32(values[2]));
            return c;
         }
         set
         {
            sunUnitColor = String.Format("{0}, {1}, {2}", value.R, value.G, value.B);
            SimGlobals.getSimMain().SetLightsDirty();

         }
      }


      [XmlElement("setTerrainColor")]
      public string setTerrainColor;
      [XmlIgnore]
      public Color SunTerrainColor 
      {
         get
         {
            string[] values = setTerrainColor.Split(',');
            Color c = Color.FromArgb(System.Convert.ToInt32(values[0]), System.Convert.ToInt32(values[1]), System.Convert.ToInt32(values[2]));
            return c;
         }
         set
         {
            setTerrainColor = String.Format("{0}, {1}, {2}", value.R, value.G, value.B);
            SimGlobals.getSimMain().SetLightsDirty();

         }
      }

      [XmlElement("SunParticleColor")]
      public string sunParticleColor = "255,255,255";
      [XmlIgnore]
      public Color SunParticleColor
      {
         get
         {
            string[] values = sunParticleColor.Split(',');
            Color c = Color.FromArgb(System.Convert.ToInt32(values[0]), System.Convert.ToInt32(values[1]), System.Convert.ToInt32(values[2]));
            return c;
         }
         set
         {
            sunParticleColor = String.Format("{0}, {1}, {2}", value.R, value.G, value.B);
            SimGlobals.getSimMain().SetLightsDirty();
         }
      }

      [XmlElement("terrainSpecularPower")]
      public float terrainSpecularPower = 25f;
      [XmlIgnore]
      public float TerrainSpecularPower 
      {
         get
         {
            return terrainSpecularPower;
         }
         set
         {
            terrainSpecularPower = value;
            SimGlobals.getSimMain().SetLightsDirty();
         }

      }

      [XmlElement("terrainBumpStrength")]
      public float terrainBumpStrength = 1.0f;
      [XmlIgnore]
      public float TerrainBumpStrength 
      {
         get
         {
            return terrainBumpStrength;
         }
         set
         {
            terrainBumpStrength = value;
            SimGlobals.getSimMain().SetLightsDirty();
         }

      }

      [XmlElement("TerrainAODiffuseIntensity")]
      public float terrainAODiffuseIntensity = 0.0f;
      [XmlIgnore]
      public float TerrainAODiffuseIntensity
      {
         get
         {
            return terrainAODiffuseIntensity;
         }
         set
         {
            terrainAODiffuseIntensity = value;
            SimGlobals.getSimMain().SetLightsDirty();
         }

      }

      [XmlElement("TerrainSpecOnlyColor")]
      public string terrainSpecOnlyColor = "0,0,0";
      [XmlIgnore]
      public Color TerrainSpecOnlyColor
      {
         get
         {
            string[] values = terrainSpecOnlyColor.Split(',');
            Color c = Color.FromArgb(System.Convert.ToInt32(values[0]), System.Convert.ToInt32(values[1]), System.Convert.ToInt32(values[2]));
            return c;
         }
         set
         {
            terrainSpecOnlyColor = String.Format("{0}, {1}, {2}", value.R, value.G, value.B);
            SimGlobals.getSimMain().SetLightsDirty();

         }
      }

      [XmlElement("TerrainSpecOnlyPower")]
      public float terrainSpecOnlyPower = 100.0f;
      [XmlIgnore]
      public float TerrainSpecOnlyPower
      {
         get
         {
            return terrainSpecOnlyPower;
         }
         set
         {
            terrainSpecOnlyPower = value;
            SimGlobals.getSimMain().SetLightsDirty();
         }
      }

      [XmlElement("TerrainSpecOnlyShadowDarkness")]
      public float terrainSpecOnlyShadowDarkness = 1.0f;
      [XmlIgnore]
      public float TerrainSpecOnlyShadowDarkness
      {
         get
         {
            return terrainSpecOnlyShadowDarkness;
         }
         set
         {
            terrainSpecOnlyShadowDarkness = value;
            SimGlobals.getSimMain().SetLightsDirty();
         }
      }

      [XmlElement("TerrainSpecOnlySunInclination")]
      public float terrainSpecOnlySunInclination;
      [XmlIgnore]
      public float TerrainSpecOnlySunInclination
      {
         get
         {
            return terrainSpecOnlySunInclination;
         }
         set
         {
            terrainSpecOnlySunInclination = value;
            SimGlobals.getSimMain().SetLightsDirty();
         }

      }


      [XmlElement("TerrainSpecOnlySunRotation")]
      public float terrainSpecOnlySunRotation;
      [XmlIgnore]
      public float TerrainSpecOnlySunRotation
      {
         get
         {
            return terrainSpecOnlySunRotation;
         }
         set
         {
            terrainSpecOnlySunRotation = value;
            SimGlobals.getSimMain().SetLightsDirty();

         }

      }


      [XmlElement("sunUnitShadowDarkness")]
      public float sunUnitShadowDarkness;
      [XmlIgnore]
      public float SunUnitShadowDarkness
      {
         get
         {
            return sunUnitShadowDarkness;
         }
         set
         {
            sunUnitShadowDarkness = value;
            SimGlobals.getSimMain().SetLightsDirty();

         }


      }


      [XmlElement("sunTerrainShadowDarkness")]
      public float sunTerrainShadowDarkness;
      [XmlIgnore]
      public float SunTerrainShadowDarkness
      {
         get
         {
            return sunTerrainShadowDarkness;
         }
         set
         {
            sunTerrainShadowDarkness = value;
            SimGlobals.getSimMain().SetLightsDirty();

         }

      }

      [XmlElement("sunUnitIntensity")]
      public float sunUnitIntensity;
      [XmlIgnore]
      public float SunUnitIntensity
      {
         get
         {
            return sunUnitIntensity;
         }
         set
         {
            sunUnitIntensity = value;
            SimGlobals.getSimMain().SetLightsDirty();

         }

      }
      [XmlElement("sunTerrainIntensity")]
      public float sunTerrainIntensity;
      [XmlIgnore]
      public float SunTerrainIntensity
      {
         get
         {
            return sunTerrainIntensity;
         }
         set
         {
            sunTerrainIntensity = value;
            SimGlobals.getSimMain().SetLightsDirty();

         }

      }

      [XmlElement("SunParticleIntensity")]
      public float sunParticleIntensity = 1.0f;
      [XmlIgnore]
      public float SunParticleIntensity
      {
         get
         {
            return sunParticleIntensity;
         }
         set
         {
            sunParticleIntensity = value;
            SimGlobals.getSimMain().SetLightsDirty();

         }
      }

      [XmlElement("sunShadows")]
      public bool sunShadows;
      [XmlIgnore]
      public bool SunShadows
      {
         get
         {
            return sunShadows;
         }
         set
         {
            sunShadows = value;
            SimGlobals.getSimMain().SetLightsDirty();

         }

      }
      [XmlElement("hemiInclination")]
      public float hemiInclination;
      [XmlIgnore]
      public float HemiInclination
      {
         get
         {
            return hemiInclination;
         }
         set
         {
            hemiInclination = value;
            SimGlobals.getSimMain().SetLightsDirty();

         }

      }

      [XmlElement("hemiRotation")]
      public float hemiRotation;
      [XmlIgnore]
      public float HemiRotation
      {
         get
         {
            return hemiRotation;
         }
         set
         {
            hemiRotation = value;
            SimGlobals.getSimMain().SetLightsDirty();

         }

      }

      [XmlElement("hemiUnitTopColor")]
      public string hemiUnitTopColor;
      [XmlIgnore]
      public Color HemiUnitTopColor
      {
         get
         {
            string[] values = hemiUnitTopColor.Split(',');
            Color c = Color.FromArgb(System.Convert.ToInt32(values[0]), System.Convert.ToInt32(values[1]), System.Convert.ToInt32(values[2]));
            return c;
         }
         set
         {
            hemiUnitTopColor = String.Format("{0}, {1}, {2}", value.R, value.G, value.B);
            SimGlobals.getSimMain().SetLightsDirty();

         }
      }

      [XmlElement("hemiUnitBottomColor")]
      public string hemiUnitBottomColor;
      [XmlIgnore]
      public Color HemiUnitBottomColor
      {
         get
         {
            string[] values = hemiUnitBottomColor.Split(',');
            Color c = Color.FromArgb(System.Convert.ToInt32(values[0]), System.Convert.ToInt32(values[1]), System.Convert.ToInt32(values[2]));
            return c;
         }
         set
         {
            hemiUnitBottomColor = String.Format("{0}, {1}, {2}", value.R, value.G, value.B);
            SimGlobals.getSimMain().SetLightsDirty();

         }
      }

      [XmlElement("hemiTerrainTopColor")]
      public string hemiTerrainTopColor;
      [XmlIgnore]
      public Color HemiTerrainTopColor
      {
         get
         {
            string[] values = hemiTerrainTopColor.Split(',');
            Color c = Color.FromArgb(System.Convert.ToInt32(values[0]), System.Convert.ToInt32(values[1]), System.Convert.ToInt32(values[2]));
            return c;
         }
         set
         {
            hemiTerrainTopColor = String.Format("{0}, {1}, {2}", value.R, value.G, value.B);
            SimGlobals.getSimMain().SetLightsDirty();

         }
      }

      [XmlElement("hemiTerrainBottomColor")]
      public string hemiTerrainBottomColor;
      [XmlIgnore]
      public Color HemiTerrainBottomColor
      {
         get
         {
            string[] values = hemiTerrainBottomColor.Split(',');
            Color c = Color.FromArgb(System.Convert.ToInt32(values[0]), System.Convert.ToInt32(values[1]), System.Convert.ToInt32(values[2]));
            return c;
         }
         set
         {
            hemiTerrainBottomColor = String.Format("{0}, {1}, {2}", value.R, value.G, value.B);
            SimGlobals.getSimMain().SetLightsDirty();

         }
      }

      [XmlElement("hemiUnitIntensity")]
      public float hemiUnitIntensity;
      [XmlIgnore]
      public float HemiUnitIntensity
      {
         get
         {
            return hemiUnitIntensity;
         }
         set
         {
            hemiUnitIntensity = value;
            SimGlobals.getSimMain().SetLightsDirty();

         }

      }
      [XmlElement("hemiTerrainIntensity")]
      public float hemiTerrainIntensity;
      [XmlIgnore]
      public float HemiTerrainIntensity
      {
         get
         {
            return hemiTerrainIntensity;
         }
         set
         {
            hemiTerrainIntensity = value;
            SimGlobals.getSimMain().SetLightsDirty();

         }

      }

      [XmlElement("middleGrey")]
      public float middleGrey = 0.4f;
      [XmlIgnore]
      public float MiddleGrey
      {
         get
         {
            return middleGrey;
         }
         set
         {
            middleGrey = value;
            SimGlobals.getSimMain().SetLightsDirty();

         }
      }

      [XmlElement("brightMaskThresh")]
      public float brightMaskThresh = 0.9f;
      [XmlIgnore]
      public float BrightMaskThresh
      {
         get
         {
            return brightMaskThresh;
         }
         set
         {
            brightMaskThresh = value;
            SimGlobals.getSimMain().SetLightsDirty();

         }
      }

      [XmlElement("bloomIntensity")]
      public float bloomIntensity = 0.65f;
      [XmlIgnore]
      public float BloomIntensity
      {
         get
         {
            return bloomIntensity;
         }
         set
         {
            bloomIntensity = value;
            SimGlobals.getSimMain().SetLightsDirty();

         }
      }

      [XmlElement("bloomSigma")]
      public float bloomSigma = 2.0f;
      [XmlIgnore]
      public float BloomSigma
      {
         get
         {
            return bloomSigma;
         }
         set
         {
            bloomSigma = value;
            SimGlobals.getSimMain().SetLightsDirty();

         }
      }

      [XmlElement("adaptationRate")]
      public float adaptationRate = 0.06f;
      [XmlIgnore]
      public float AdaptationRate
      {
         get
         {
            return adaptationRate;
         }
         set
         {
            adaptationRate = value;
            SimGlobals.getSimMain().SetLightsDirty();
         }
      }

      [XmlElement("logAveMin")]
      public float logAveMin = 0.5f;
      [XmlIgnore]
      public float LogAveMin
      {
         get
         {
            return logAveMin;
         }
         set
         {
            logAveMin = value;
            SimGlobals.getSimMain().SetLightsDirty();
         }
      }

      [XmlElement("logAveMax")]
      public float logAveMax = 250f;
      [XmlIgnore]
      public float LogAveMax
      {
         get
         {
            return logAveMax;
         }
         set
         {
            logAveMax = value;
            SimGlobals.getSimMain().SetLightsDirty();
         }
      }

      [XmlElement("whitePointMin")]
      public float whitePointMin = 30f;
      [XmlIgnore]
      public float WhitePointMin
      {
         get
         {
            return whitePointMin;
         }
         set
         {
            whitePointMin = value;
            SimGlobals.getSimMain().SetLightsDirty();
         }
      }

      [XmlElement("whitePointMax")]
      public float whitePointMax = 250f;
      [XmlIgnore]
      public float WhitePointMax
      {
         get
         {
            return whitePointMax;
         }
         set
         {
            whitePointMax = value;
            SimGlobals.getSimMain().SetLightsDirty();
         }
      }

//Colors range from 0-255 (default 255,255,255), intensites 0-8 (default 1), fog start 0.0 - 50000 (default to 50000), fog density 0.0-8.0 (default 0.0).
 
//Default densities should always be 0.0 (disabled). 
 
//  <zFogColor>150, 150, 255</zFogColor>
//  <zFogIntensity>1.0</zFogIntensity>
//  <zFogStart>400.0</zFogStart>
//  <zFogDensity>3.0</zFogDensity>
  
//  <planarFogColor>255, 255, 255</planarFogColor>
//  <planarFogIntensity>5.0</planarFogIntensity>
//  <planarFogStart>20.0</planarFogStart>
//  <planarFogDensity>0.0</planarFogDensity>

/////////////////////////////////
      [XmlElement("zFogColor")]
      public string _zFogColor = "255,255,255";
      [XmlIgnore]
      public Color zFogColor
      {
         get
         {
            string[] values = _zFogColor.Split(',');
            Color c = Color.FromArgb(System.Convert.ToInt32(values[0]), System.Convert.ToInt32(values[1]), System.Convert.ToInt32(values[2]));
            return c;
         }
         set
         {
            _zFogColor = String.Format("{0}, {1}, {2}", value.R, value.G, value.B);
            SimGlobals.getSimMain().SetLightsDirty();

         }
      }
      [XmlElement("zFogIntensity")]
      public float _zFogIntensity = 1f;
      [XmlIgnore]
      public float zFogIntensity
      {
         get
         {
            return _zFogIntensity;
         }
         set
         {
            _zFogIntensity = value;
            SimGlobals.getSimMain().SetLightsDirty();
         }
      }

      [XmlElement("zFogStart")]
      public float _zFogStart = 50000f;
      [XmlIgnore]
      public float zFogStart
      {
         get
         {
            return _zFogStart;
         }
         set
         {
            _zFogStart = value;
            SimGlobals.getSimMain().SetLightsDirty();
         }
      }
      [XmlElement("zFogDensity")]
      public float _zFogDensity = 0f;
      [XmlIgnore]
      public float zFogDensity
      {
         get
         {
            return _zFogDensity;
         }
         set
         {
            _zFogDensity = value;
            SimGlobals.getSimMain().SetLightsDirty();
         }
      }

      [XmlElement("planarFogColor")]
      public string _planarFogColor = "255,255,255";
      [XmlIgnore]
      public Color planarFogColor
      {
         get
         {
            string[] values = _planarFogColor.Split(',');
            Color c = Color.FromArgb(System.Convert.ToInt32(values[0]), System.Convert.ToInt32(values[1]), System.Convert.ToInt32(values[2]));
            return c;
         }
         set
         {
            _planarFogColor = String.Format("{0}, {1}, {2}", value.R, value.G, value.B);
            SimGlobals.getSimMain().SetLightsDirty();

         }
      }
      [XmlElement("planarFogIntensity")]
      public float _planarFogIntensity = 1f;
      [XmlIgnore]
      public float planarFogIntensity
      {
         get
         {
            return _planarFogIntensity;
         }
         set
         {
            _planarFogIntensity = value;
            SimGlobals.getSimMain().SetLightsDirty();
         }
      }

      [XmlElement("planarFogStart")]
      public float _planarFogStart = 50000f;
      [XmlIgnore]
      public float planarFogStart
      {
         get
         {
            return _planarFogStart;
         }
         set
         {
            _planarFogStart = value;
            SimGlobals.getSimMain().SetLightsDirty();
         }
      }
      [XmlElement("planarFogDensity")]
      public float _planarFogDensity = 0f;
      [XmlIgnore]
      public float planarFogDensity
      {
         get
         {
            return _planarFogDensity;
         }
         set
         {
            _planarFogDensity = value;
            SimGlobals.getSimMain().SetLightsDirty();
         }
      }


//New parameters:
//  <dofEnabled>0</dofEnabled>
//  <dofNearRange>350.0</dofNearRange>
//  <dofFocalPlaneDist>100.0</dofFocalPlaneDist>
//  <dofFarRange>450.0</dofFarRange>
//  <dofMaxBlurriness>1</dofMaxBlurriness>
 
//dofNearRange - from 0 to 10000
//dofFocalPlaneDist - 0 to 5000
//dofFarRange - from 0 to 5000

      [XmlElement("dofEnabled")]
      public bool _dofEnabled = false;
      [XmlIgnore]
      public bool dofEnabled
      {
         get
         {
            return _dofEnabled;
         }
         set
         {
            _dofEnabled = value;
            SimGlobals.getSimMain().SetLightsDirty();

         }
      }
      [XmlElement("dofNearRange")]
      public float _dofNearRange = 350f;
      [XmlIgnore]
      public float dofNearRange
      {
         get
         {
            return _dofNearRange;
         }
         set
         {
            _dofNearRange = value;
            SimGlobals.getSimMain().SetLightsDirty();
         }
      }
      [XmlElement("dofFocalPlaneDist")]
      public float _dofFocalPlaneDist = 100f;
      [XmlIgnore]
      public float dofFocalPlaneDist
      {
         get
         {
            return _dofFocalPlaneDist;
         }
         set
         {
            _dofFocalPlaneDist = value;
            SimGlobals.getSimMain().SetLightsDirty();
         }
      }
      [XmlElement("dofFarRange")]
      public float _dofFarRange = 450f;
      [XmlIgnore]
      public float dofFarRange
      {
         get
         {
            return _dofFarRange;
         }
         set
         {
            _dofFarRange = value;
            SimGlobals.getSimMain().SetLightsDirty();
         }
      }
      [XmlElement("dofMaxBlurriness")]
      public float _dofMaxBlurriness = 1f;
      [XmlIgnore]
      public float dofMaxBlurriness
      {
         get
         {
            return _dofMaxBlurriness;
         }
         set
         {
            _dofMaxBlurriness = value;
            SimGlobals.getSimMain().SetLightsDirty();
         }
      }
///////////////////////

      //[XmlElement("Flag", typeof(string))]
      //[XmlArrayItem(ElementName = "Light", Type = typeof(LightXML))]
      [XmlElement("Light",  typeof(LightXML))]
      //[XmlArray("Lights")]
      [Browsable(false)]
      public List<LightXML> mLights = new List<LightXML>();

      [XmlElement("TLight", typeof(LightXML))]
      [Browsable(false)]
      public List<LightXML> mTLights = new List<LightXML>();

   }

//   Middlegrey ranges from 0-2:
// <middleGrey>.9</middleGrey>
 
//White point ranges from 0-32:
//  <whitePoint>30.0</whitePoint>
 
//Bright mask ranges from 0-2:
//  <brightMaskThresh>.8</brightMaskThresh>
//Bloom intensity ranges from 0-4:
//  <bloomIntensity>1.0</bloomIntensity>
 
//Bloom sigma: 0-5
//  <bloomSigma>3.3</bloomSigma>
 
//Adaptation rate: .001 to 1 (never zero)
//  <adaptationRate>.04</adaptationRate>
   #endregion

   #region Automatic code (almost)


   /// <remarks/>
   [System.CodeDom.Compiler.GeneratedCodeAttribute("xsd", "2.0.50727.42")]
   [System.SerializableAttribute()]
   [System.Diagnostics.DebuggerStepThroughAttribute()]
   [System.ComponentModel.DesignerCategoryAttribute("code")]
   [System.Xml.Serialization.XmlTypeAttribute(AnonymousType = true)]
   [System.Xml.Serialization.XmlRootAttribute(Namespace = "", IsNullable = false)]
   public partial class Data
   {

      private DataPlayerPlacement playerPlacementField = new DataPlayerPlacement();

      private byte numberPlayersField;

      /// <remarks/>
      public DataPlayerPlacement PlayerPlacement
      {
         get
         {
            return this.playerPlacementField;
         }
         set
         {
            this.playerPlacementField = value;
         }
      }

      /// <remarks/>
      public byte NumberPlayers
      {
         get
         {
            return this.numberPlayersField;
         }
         set
         {
            this.numberPlayersField = value;
         }
      }
   }

   /// <remarks/>
   [System.CodeDom.Compiler.GeneratedCodeAttribute("xsd", "2.0.50727.42")]
   [System.SerializableAttribute()]
   [System.Diagnostics.DebuggerStepThroughAttribute()]
   [System.ComponentModel.DesignerCategoryAttribute("code")]
   [System.Xml.Serialization.XmlTypeAttribute(AnonymousType = true)]
   public partial class DataPlayerPlacement
   {

      private string typeField;

      private byte spacingField;

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public string Type
      {
         get
         {
            return this.typeField;
         }
         set
         {
            this.typeField = value;
         }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public byte Spacing
      {
         get
         {
            return this.spacingField;
         }
         set
         {
            this.spacingField = value;
         }
      }
   }





   /// <remarks/>
   [System.CodeDom.Compiler.GeneratedCodeAttribute("xsd", "2.0.50727.42")]
   [System.SerializableAttribute()]
   [System.Diagnostics.DebuggerStepThroughAttribute()]
   [System.ComponentModel.DesignerCategoryAttribute("code")]
   [System.Xml.Serialization.XmlTypeAttribute(AnonymousType = true)]
   [System.Xml.Serialization.XmlRootAttribute(Namespace = "", IsNullable = false)]
   public partial class Players 
   {

      private List<PlayersPlayer> playerField = new List<PlayersPlayer>();

      /// <remarks/>
      [System.Xml.Serialization.XmlElementAttribute("Player")]
      public List<PlayersPlayer> Player
      {
         get
         {
            return this.playerField;
         }
         set
         {
            this.playerField = value;
         }
      }

  



   }


   /// <remarks/>
   [System.CodeDom.Compiler.GeneratedCodeAttribute("xsd", "2.0.50727.42")]
   [System.SerializableAttribute()]
   [System.Diagnostics.DebuggerStepThroughAttribute()]
   [System.ComponentModel.DesignerCategoryAttribute("code")]
   [System.Xml.Serialization.XmlTypeAttribute(AnonymousType = true)]
   public partial class PlayersPlayer
   {

      private string nameField;

      private string nameIDField;

      private bool useStartingUnitsField;

      private bool controllableField;

      private bool usePlayerSettingsField;

      private string civField;

      private byte colorField;

      private byte teamField;

      private bool defaultResourcesField;

      private ushort suppliesField;

      private ushort powerField;

      private ushort favorField;

      private ushort relicsField;

      private ushort honorField;

      private string leader1;

      private string leader2;

      private string leader3;


      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public string Name
      {
         get
         {
            return this.nameField;
         }
         set
         {
            this.nameField = value;
         }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public string LocalisedDisplayName
      {
         get
         {
            return this.nameIDField;
         }
         set
         {
            this.nameIDField = value;
         }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public bool UseStartingUnits
      {
         get
         {
            return this.useStartingUnitsField;
         }
         set
         {
            this.useStartingUnitsField = value;
         }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public bool Controllable
      {
         get
         {
            return this.controllableField;
         }
         set
         {
            this.controllableField = value;
         }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public bool UsePlayerSettings
      {
         get
         {
            return this.usePlayerSettingsField;
         }
         set
         {
            this.usePlayerSettingsField = value;
         }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public string Civ
      {
         get
         {
            return this.civField;
         }
         set
         {
            this.civField = value;
         }
      }

      
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public string Leader1
      {
         get
         {
            return this.leader1;
         }
         set
         {
            this.leader1 = value;
         }
      }
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public string Leader2
      {
         get
         {
            return this.leader2;
         }
         set
         {
            this.leader2 = value;
         }
      }
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public string Leader3
      {
         get
         {
            return this.leader3;
         }
         set
         {
            this.leader3 = value;
         }
      }
      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public byte Color
      {
         get
         {
            return this.colorField;
         }
         set
         {
            this.colorField = value;
         }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public byte Team
      {
         get
         {
            return this.teamField;
         }
         set
         {
            this.teamField = value;
         }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public bool DefaultResources
      {
         get
         {
            return this.defaultResourcesField;
         }
         set
         {
            this.defaultResourcesField = value;
         }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public ushort Supplies
      {
         get
         {
            return this.suppliesField;
         }
         set
         {
            this.suppliesField = value;
         }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public ushort Power
      {
         get
         {
            return this.powerField;
         }
         set
         {
            this.powerField = value;
         }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public ushort Favor
      {
         get
         {
            return this.favorField;
         }
         set
         {
            this.favorField = value;
         }
      }
      
      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public ushort Relics
      {
         get
         {
            return this.relicsField;
         }
         set
         {
            this.relicsField = value;
         }
      }

      /// <remarks/>
      [System.Xml.Serialization.XmlAttributeAttribute()]
      public ushort Honor
      {
         get
         {
            return this.honorField;
         }
         set
         {
            this.honorField = value;
         }
      }

      //<ForbidObjects>
      //  <Object>unsc_bldg_barracks_01</Object>
      //</ForbidObjects>
      //<ForbidSquads>
      //  <Squad>unsc_veh_warthog_01</Squad>
      //</ForbidSquads>
      //<ForbidTechs>
      //  <Tech>unsc_warthog_upgrade1</Tech>
      //</ForbidTechs>

      //   [XmlElement("Flag", typeof(string))]
      //   public List<string> mFlags = new List<string>();
      //[XmlArrayItem(ElementName = "Position", Type = typeof(PlayerPositionXML))]
      //[XmlArray("Positions")]


      [XmlArrayItem(ElementName = "Object", Type = typeof(string))]
      [XmlArray("ForbidObjects")]
      public List<string> mForbidObjects = new List<string>();

      [XmlArrayItem(ElementName = "Squad", Type = typeof(string))]
      [XmlArray("ForbidSquads")]
      public List<string> mForbidSquads = new List<string>();

      [XmlArrayItem(ElementName = "Tech", Type = typeof(string))]
      [XmlArray("ForbidTechs")]
      public List<string> mForbidTechs = new List<string>();


   }
   [XmlRoot("Diplomacy")]
   public class DiplomacyXml
   {
      [XmlAttribute("Team")]
      public int mTeam;

      [XmlElement("Team", typeof(DiplomacyTeamXml))]
      public List<DiplomacyTeamXml> mTeams = new List<DiplomacyTeamXml>();

      public static void InitDiplomacyData(List<DiplomacyXml> diplomacies, int numTeams)
      {
         diplomacies.Clear();
         for (int i = 0; i < numTeams; i++)
         {
            DiplomacyXml dipl = new DiplomacyXml();
            dipl.mTeam = i+1;
            diplomacies.Add(dipl);
            for (int j = 0; j < numTeams; j++)
            {
               DiplomacyTeamXml t = new DiplomacyTeamXml();
               t.mId = j+1;
               t.mStatus = "Neutral";
               dipl.mTeams.Add(t);
               if (j == i)
                  t.mStatus = "Ally";
            }
         }
      }
   }
   [XmlRoot("Team")]
   public class DiplomacyTeamXml
   {
      [XmlAttribute("ID")]
      public int mId;
      [XmlText]
      public string mStatus;

   }

   
//  <Diplomacies>
//    <Diplomacy Team="1">
//      <Team ID="1">Ally</Team>
//      <Team ID="2">Enemy</Team>
//      <Team ID="3">Neutral</Team>
//      <Team ID="4">Neutral</Team>
//    </Diplomacy>
//    <Diplomacy Team="2">
//      <Team ID="1">Enemy</Team>
//      <Team ID="2">Ally</Team>
//      <Team ID="3">Neutral</Team>
//      <Team ID="4">Neutral</Team>
//    </Diplomacy>
//    <Diplomacy Team="3">
//      <Team ID="1">Neutral</Team>
//      <Team ID="2">Neutral</Team>
//      <Team ID="3">Ally</Team>
//      <Team ID="4">Neutral</Team>
//    </Diplomacy>
//    <Diplomacy Team="4">
//      <Team ID="1">Neutral</Team>
//      <Team ID="2">Neutral</Team>
//      <Team ID="3">Neutral</Team>
//      <Team ID="4">Ally</Team>
//    </Diplomacy>
//  </Diplomacies>
//We can modify the format if there’s something that would make it easier for you.
//Some validation/editing stuff that doesn’t have to be done first pass if it’s not easy… 
//The diplomacy of a team for itself must be Ally. The diplomacy for any other teams must be 
// Neutral or Enemy (if you want players to be allies, you have to assign them to the same team).







   #endregion


   //ongoing research...
   //public class DataField
   //{
   //   [XmlAttribute]
   //   public string Name = "";
   //   [XmlText]
   //   public string Value = "";
   //}


   //?typed?
   //public class DataField : INamedTypedProperty
   //{
   //   [XmlAttribute("Name")]
   //   public string mName = "";
   //   [XmlText]
   //   public string mValue = "";

   //   [XmlIgnore]
   //   public string mType = "";

   //   #region INamedTypedProperty Members

   //   public string GetName()
   //   {
   //      return mName;
   //   }
   //   public string GetTypeName()
   //   {
   //      return mType;
   //   }
   //   public object GetValue()
   //   {
   //      return mValue;
   //   }
   //   public void SetValue(object val)
   //   {
   //      mValue = val.ToString();
   //   }
   //   Dictionary<string, object> mMetaData = new Dictionary<string, object>();
   //   [XmlIgnore]
   //   public Dictionary<string, object> MetaData
   //   {
   //      get
   //      {
   //         return mMetaData;
   //      }
   //      set
   //      {
   //         mMetaData = value;
   //      }
   //   }
   //   #endregion
   //}

   [XmlRoot("DesignerData")]
   public class DesignerData
   {
      //[XmlArrayItem(ElementName = "Data", Type = typeof(DataField))]
      //[XmlArray("Data")]
      //public List<DataField> mData = new List<DataField>();

      private string mType = "";
      public string Type
      {
         get { return mType; }
         set { mType = value; }
      }

      private float mAttract = 0;
      public float Attract
      {
         get { return mAttract; }
         set { mAttract = value; }
      }

      private float mRepel = 0;
      public float Repel
      {
         get { return mRepel; }
         set { mRepel = value; }
      }

      private float mChokePoint = 0;
      public float ChokePoint
      {
         get { return mChokePoint; }
         set { mChokePoint = value; }
      }

      private float mValue = 0;
      public float Value
      {
         get { return mValue; }
         set { mValue = value; }
      }

      //temp extra values
      private string mValue1 = "";
      public string Value1
      {
         get { return mValue1; }
         set { mValue1 = value; }
      }
      private string mValue2 = "";
      public string Value2
      {
         get { return mValue2; }
         set { mValue2 = value; }
      }
      private string mValue3 = "";
      public string Value3
      {
         get { return mValue3; }
         set { mValue3 = value; }
      }
      private string mValue4 = "";
      public string Value4
      {
         get { return mValue4; }
         set { mValue4 = value; }
      }
      private string mValue5 = "";
      public string Value5
      {
         get { return mValue5; }
         set { mValue5 = value; }
      }
      private string mValue6 = "";
      public string Value6
      {
         get { return mValue6; }
         set { mValue6 = value; }
      }
      private string mValue7 = "";
      public string Value7
      {
         get { return mValue7; }
         set { mValue7 = value; }
      }
      private string mValue8 = "";
      public string Value8
      {
         get { return mValue8; }
         set { mValue8 = value; }
      }
      private string mValue9 = "";
      public string Value9
      {
         get { return mValue9; }
         set { mValue9 = value; }
      }
      private string mValue10 = "";
      public string Value10
      {
         get { return mValue10; }
         set { mValue10 = value; }
      }
      private string mValue11 = "";
      public string Value11
      {
         get { return mValue11; }
         set { mValue11 = value; }
      }
      private string mValue12 = "";
      public string Value12
      {
         get { return mValue12; }
         set { mValue12 = value; }
      }
   }

   [XmlRoot("DesignObjects")]
   public class DesignObjects
   {
      [XmlArrayItem(ElementName = "Sphere", Type = typeof(DesignSphereXML))]
      [XmlArray("Spheres")]
      public List<DesignSphereXML> mSpheres = new List<DesignSphereXML>();

      [XmlArrayItem(ElementName = "Lines", Type = typeof(DesignLineXML))]
      [XmlArray("Lines")]
      public List<DesignLineXML> mLines = new List<DesignLineXML>();

   }

   [XmlRoot("DesignSphere")]
   public class DesignSphereXML
   {
      [XmlAttribute("ID")]
      public int ID;
      [XmlAttribute("Name")]
      public string Name;
      [XmlAttribute("Position")]
      public string Position;
      [XmlAttribute("Radius")]
      public float Radius;    
      [XmlElement("Data")]
      public DesignerData Data = new DesignerData();
   }

   [XmlRoot("DesignLine")]
   public class DesignLineXML
   {
      [XmlAttribute("ID")]
      public int ID;
      [XmlAttribute("Name")]
      public string Name;
      [XmlAttribute("Position")]
      public string Position;

      //[XmlArrayItem(ElementName = "Point", Type = typeof(Vector3))]
      //[XmlArray("LinePoints")]
      [XmlIgnore]
      public List<Vector3> mPoints = new List<Vector3>();

      [XmlElement("Points")]
      public string LinePoints
      {
         get
         {
            string output = "";
            foreach (Vector3 v in mPoints)
            {
               if (output.Length != 0)
               {
                  output += "|";
               }
               output += TextVectorHelper.ToString(v);
            }
            return output;
         }
         set
         {
            mPoints.Clear();
            string[] vecs = value.Split('|');
            foreach (string s in vecs)
            {
               if (s == "")
                  continue;
               mPoints.Add(TextVectorHelper.FromString(s));
            }
            
         }
      }

      [XmlElement("Data")]
      public DesignerData Data = new DesignerData();
   }


   [XmlRoot("ObjectPlacementSettingsFile")]
   public class ObjectPlacementSettingsFile
   {
      [XmlArrayItem(ElementName = "ObjectPlacementSettings", Type = typeof(ObjectPlacementSettings))]
      [XmlArray("Settings")]
      public List<ObjectPlacementSettings> mSettings = new List<ObjectPlacementSettings>();

      static public List<ObjectPlacementSettings> Load(string fileName)
      {
         if (!File.Exists(fileName))
            return null;

         XmlSerializer s = new XmlSerializer(typeof(ObjectPlacementSettingsFile), new Type[] {  });
         Stream st = File.OpenRead(fileName);
         return ((ObjectPlacementSettingsFile)s.Deserialize(st)).mSettings;
      }
      static public void Save(string fileName, List<ObjectPlacementSettings> data)
      {
         ObjectPlacementSettingsFile output = new ObjectPlacementSettingsFile();
         output.mSettings = data;

         XmlSerializer s = new XmlSerializer(typeof(ObjectPlacementSettingsFile), new Type[] { });
         Stream st = File.Open(fileName, FileMode.Create);
         s.Serialize(st, output);
         st.Close();
      }

   }



   public class ObjectPlacementSettings
   {
      string mObject = "";
      public string Object
      {
         get
         {
            return mObject;
         }
         set
         {
            mObject = value;
         }
      }

      float mMinMaskRange = 0.1f;
      public float MinMaskRange
      {
         get
         {
            return mMinMaskRange;
         }
         set
         {
            mMinMaskRange = value;
         }
      }
      float mMaxMaskRange = 1f;
      public float MaxMaskRange
      {
         get
         {
            return mMaxMaskRange;
         }
         set
         {
            mMaxMaskRange = value;
         }
      }
      float mMidMaskRange = 0.5f;
      public float MidMaskRange
      {
         get
         {
            return mMidMaskRange;
         }
         set
         {
            mMidMaskRange = value;
         }
      }
      int mNumberToPlace = 10;
      public int NumberToPlace
      {
         get
         {
            return mNumberToPlace;
         }
         set
         {
            mNumberToPlace = value;
         }
      }
      int mUseTerrainSlope = 0;
      public int UseTerrainSlope
      {
         get
         {
            return mUseTerrainSlope;
         }
         set
         {
            mUseTerrainSlope = value;
         }
      }

      bool mbUseMaskDensity = true;
      public bool UseMaskDensity
      {
         get
         {
            return mbUseMaskDensity;
         }
         set
         {
            mbUseMaskDensity = value;
         }
      }


   }


}

