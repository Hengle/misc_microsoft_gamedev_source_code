using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using System.Xml;
using System.Xml.XPath;
using System.Xml.Serialization;

using EditorCore;

using SimEditor;

namespace PhoenixEditor.ScenarioEditor
{

   class SimResources
   {
      //public StringTable mStringTable = new StringTable();
      public TechData mTechData = new TechData();
      public ProtoObjectData mProtoObjectData = new ProtoObjectData();
      public ObjectTypeData mObjectTypeData = new ObjectTypeData();
      public ProtoSquadData mProtoSquadData = new ProtoSquadData();
      public GameData mGameData = new GameData();
      public Leaders mLeaders = new Leaders();
      public Powers mPowers = new Powers();
      public ExposedScripts mExposedScripts = new ExposedScripts();
      public IconData mIconData = new IconData();
      public PlacementRuleData mPlacementRuleData = new PlacementRuleData();
      public HintConcepts mHintConcepts = new HintConcepts();

      public void LoadResouces()
      {
         //mStringTable.Load();
         mTechData.Load();
         mProtoObjectData.Load();
         mObjectTypeData.Load();
         mProtoSquadData.Load();
         mGameData.Load();
         mLeaders.Load();
         mPowers.Load();
         mExposedScripts.Load();
         mIconData.Load();
         mPlacementRuleData.Load();
         SimTerrainType.loadTerrainTileTypes();

         mHintConcepts.Load();
      }
   }

   //public class LocString
   //{
   //   public int mLocID=0;
   //   public string mLocIDAsString = "";
   //   public string mString = "";
   //   public string mCategory = "";
   //   public string mScenario = "";

   //   private string mToString = null;

   //   public override string ToString()
   //   {
   //      return string.Format("[{0}] {1}", mLocID, mString);    // lazy init
   //   }
   //};

   //public class StringTable
   //{

   //   public Dictionary<string, LocString> mStringsByID = new Dictionary<string, LocString>();
   //   public Dictionary<string, Dictionary<string, LocString>> mStringsByScenario = new Dictionary<string, Dictionary<string, LocString>>();
         
   //   private XmlDocument mStringTableDoc;
   //   public void Load()
   //   {
   //      mStringsByID.Clear();
   //      mStringsByScenario.Clear();

   //      mStringTableDoc = new XmlDocument();
   //      mStringTableDoc.Load(Path.Combine(CoreGlobals.getWorkPaths().mGameDataDirectory, "stringtable.xml"));

   //      XmlNodeList strings = mStringTableDoc.SelectNodes("//String");
   //      foreach (XmlNode s in strings)
   //      {

   //         int locID = 0;
   //         string locIDAsString = "";
   //         try
   //         {
   //            locIDAsString = s.Attributes["_locID"].InnerText;
   //            locID = int.Parse( locIDAsString );
   //         }
   //         catch (Exception e)
   //         {
   //            // can we log this exception somewhere? - it should never happen :)
   //            continue;
   //         }

   //         LocString locString = new LocString();
   //         locString.mLocIDAsString = locIDAsString;
   //         locString.mLocID = locID;
   //         locString.mString = s.InnerText;
   //         locString.mCategory = (s.Attributes["category"] != null) ? s.Attributes["category"].InnerText : "";
   //         locString.mScenario = (s.Attributes["scenario"] != null) ? s.Attributes["scenario"].InnerText : "";

   //         mStringsByID.Add(locString.mLocIDAsString, locString);

   //         if (locString.mScenario.Length > 0)
   //         {
   //            Dictionary<string, LocString> scenarioStrings = mStringsByScenario[locString.mScenario];
   //            if (scenarioStrings == null)
   //            {
   //               scenarioStrings = new Dictionary<string, LocString>();
   //               mStringsByScenario.Add(locString.mScenario, scenarioStrings);
   //            }

   //            scenarioStrings.Add(locString.mLocIDAsString, locString);

   //         }

   //      }


   //   }
   //};

   public class TechData
   {

      public List<string> mTechList = new List<string>();
      public List<string> mTechDataSubType = new List<string>();
      public List<string> mTechDataSubTypeProtoSquad = new List<string>();
      public List<string> mTechDataRelativity = new List<string>();
      public List<string> mTechDataCommandType = new List<string>();
      private XmlDocument triggerDescription = null;
      public void Load()
      {
         mTechList.Clear();
         mTechDataSubType.Clear();
         mTechDataSubTypeProtoSquad.Clear();
         mTechDataRelativity.Clear();
         mTechDataCommandType.Clear();
         triggerDescription = new XmlDocument();
         triggerDescription.Load(CoreGlobals.getWorkPaths().mGameDataDirectory + "\\techs.xml");

         //Conditions
         XmlNodeList techNames = triggerDescription.SelectNodes( "//Tech" );
         foreach( XmlNode n in techNames )
         {
            // Don't add shadow techs
            bool shadow = false;
            foreach( XmlNode test in n.ChildNodes )
            {
               if( ( test.Name == "Flag" ) && ( test.InnerText == "Shadow" ) )
               {
                  shadow = true;
                  break;
               }
            }

            if( !shadow )
            {
               foreach( XmlAttribute attr in n.Attributes )
               {
                  if( attr.Name == "name" )
                  {
                     mTechList.Add( attr.Value );
                     break;
                  }
               }               
            }
         }
         mTechList.Sort();

         // Add data sub types
         XmlNodeList dataSubTypes = triggerDescription.SelectNodes("//TechDataSubType");
         foreach (XmlNode n in dataSubTypes)
         {
            mTechDataSubType.Add(n.InnerText);
         }
         mTechDataSubType.Sort();

         // Add data sub types for ProtoSquad
         XmlNodeList dataSubTypesProtoSquad = triggerDescription.SelectNodes( "//TechDataSubTypeProtoSquad" );
         foreach (XmlNode n in dataSubTypesProtoSquad)
         {
            mTechDataSubTypeProtoSquad.Add(n.InnerText);
         }
         mTechDataSubTypeProtoSquad.Sort();

         // Add data relativity
         XmlNodeList dataRelativity = triggerDescription.SelectNodes( "//TechDataRelativity" );
         foreach (XmlNode n in dataRelativity)
         {
            mTechDataRelativity.Add(n.InnerText);
         }
         mTechDataRelativity.Sort();

         // Add data command type
         XmlNodeList dataCommandType = triggerDescription.SelectNodes( "//TechDataCommandType" );
         foreach( XmlNode n in dataCommandType)
         {
            mTechDataCommandType.Add(n.InnerText);
         }
         mTechDataCommandType.Sort();
      }
   }


   public class ProtoObjectData
   {
      public List<string> mProtoObjectList = new List<string>();
      private XmlDocument mDocument = null;
      public void Load()
      {
         mProtoObjectList.Clear();
         mDocument = new XmlDocument();
         mDocument.Load(CoreGlobals.getWorkPaths().mGameDataDirectory + "\\objects.xml");

         //Conditions
         XmlNodeList objectNames = mDocument.SelectNodes("//Object/@name");
         foreach (XmlNode n in objectNames)
         {
            mProtoObjectList.Add(n.Value);
         }
         mProtoObjectList.Sort();
      }
   }

    public class ObjectTypeData
    {
       public List<string> mObjectTypeList = new List<string>();
       public List<string> mBasicObjectTypes = new List<string>();
       private XmlDocument mDocument = null;
        private XmlDocument mDocument2 = null;
        public void Load()
        {
            mObjectTypeList.Clear();
            mDocument = new XmlDocument();
            mDocument.Load(CoreGlobals.getWorkPaths().mGameDataDirectory + "\\objects.xml");
            mDocument2 = new XmlDocument();
            mDocument2.Load(CoreGlobals.getWorkPaths().mGameDataDirectory + "\\objecttypes.xml");

            //Conditions
            XmlNodeList objectNames = mDocument.SelectNodes("//Object/@name");
            List<string> objects = new List<string>();
            List<string> types = new List<string>();
            foreach (XmlNode n in objectNames)
            {
               objects.Add(n.Value);
            }

            XmlNodeList objectTypeNames = mDocument2.SelectNodes("//ObjectType");
            foreach (XmlNode t in objectTypeNames)
            {
               types.Add(t.InnerText);
            }
            objects.Sort();
            types.Sort();

            mBasicObjectTypes.AddRange(types);
            mObjectTypeList.AddRange(types);
            mObjectTypeList.AddRange(objects);
       
        }
    }

   public class IconData
   {
      public List<string> mIconObjectList = new List<string>();
      public List<string> mIconNameList = new List<string>();
      private XmlDocument mDocument = null;
      public void Load()
      {
         mIconObjectList.Clear();
         mIconNameList.Clear();
         mDocument = new XmlDocument();
         mDocument.Load( CoreGlobals.getWorkPaths().mGameDataDirectory + "\\objects.xml" );

         // Icon objects
         XmlNodeList iconTypeNames = mDocument.SelectNodes( "//Object/MinimapIconName" );
         foreach( XmlNode t in iconTypeNames )
         {
            if (t.InnerText == "")
            {
               mIconNameList.Add("NotUsed");
            }
            else
            {
               mIconNameList.Add(t.InnerText);
            }

            mIconObjectList.Add(t.ParentNode.Attributes.GetNamedItem("name").Value);
         }
      }
   }

   public class PlacementRuleData
   {
      public List<string> mPlacementRulesFileNames = new List<string>();
      public void Load()
      {
         mPlacementRulesFileNames.Clear();
         string[] fileNames = System.IO.Directory.GetFiles(CoreGlobals.getWorkPaths().mGameDataDirectory + "\\placementrules","*.xml", SearchOption.TopDirectoryOnly);
         int numFileNames = fileNames.GetLength(0);
         for( int i = 0; i < numFileNames; i++ )
         {
            string fileName = fileNames[i];
            fileName = fileName.Replace(CoreGlobals.getWorkPaths().mGameDataDirectory + "\\placementrules\\", "");
            mPlacementRulesFileNames.Add(fileName);
         }
      }
   }

    public class ProtoSquadData
    {
        public List<string> mProtoSquadList = new List<string>();
        private XmlDocument mDocument = null;
        public void Load()
        {
            mProtoSquadList.Clear();
            mDocument = new XmlDocument();
            mDocument.Load(CoreGlobals.getWorkPaths().mGameDataDirectory + "\\squads.xml");

            //Conditions
            XmlNodeList techNames = mDocument.SelectNodes("//Squad/@name");
            foreach (XmlNode n in techNames)
            {
                mProtoSquadList.Add(n.Value);
            }
            mProtoSquadList.Sort();
        }
    }

    public class GameData
    {
       public List<string> mPopTypes = new List<string>();
       public List<string> mRefCountTypes = new List<string>();
       public List<string> mHUDItems = new List<string>();
       public List<string> mFlashableItems = new List<string>();
       public List<string> mUnitFlags = new List<string>();
       public List<string> mSquadFlags = new List<string>();
       public List<string> mPlayerStates = new List<string>();
       private XmlDocument xmlDoc = null;
       public void Load()
       {
          mPopTypes.Clear();
          mRefCountTypes.Clear();
          mHUDItems.Clear();
          mFlashableItems.Clear();
          mPlayerStates.Clear();

          xmlDoc = new XmlDocument();
          xmlDoc.Load(CoreGlobals.getWorkPaths().mGameDataDirectory + "\\GameData.xml");

          //pops
          XmlNodeList nodes = xmlDoc.SelectNodes("//Pops/Pop");
          foreach (XmlNode n in nodes)
          {
             mPopTypes.Add(n.InnerText);
          }
          //Halwes - 3/28/2007 - Don't sort pop types so that IDs match sim parsing.
          //mPopTypes.Sort();

          //ref count types
          nodes = xmlDoc.SelectNodes("//RefCounts/RefCount");
          foreach (XmlNode n in nodes)
          {
             mRefCountTypes.Add(n.InnerText);
          }
          mRefCountTypes.Sort();

          //hud items
          nodes = xmlDoc.SelectNodes("//HUDItems/HUDItem");
          foreach (XmlNode n in nodes)
          {
             mHUDItems.Add(n.InnerText);
          }
          mHUDItems.Sort();

          //Flashable Items
          nodes = xmlDoc.SelectNodes("//FlashableItems/Item");
          foreach (XmlNode n in nodes)
          {
             mFlashableItems.Add(n.InnerText);
          }
          mFlashableItems.Sort();


          //unit flags
          nodes = xmlDoc.SelectNodes("//UnitFlags/UnitFlag");
          foreach (XmlNode n in nodes)
          {
             mUnitFlags.Add(n.InnerText);
          }
          mUnitFlags.Sort();

          // squad flags
          nodes = xmlDoc.SelectNodes("//SquadFlags/SquadFlag");
          foreach (XmlNode n in nodes)
          {
             mSquadFlags.Add(n.InnerText);
          }
          mSquadFlags.Sort();

          // Player States
          nodes = xmlDoc.SelectNodes("//PlayerStates/PlayerState");
          foreach(XmlNode n in nodes)
          {
             mPlayerStates.Add(n.InnerText);
          }          
          mPlayerStates.Sort();
       }
    }

    public class Leaders
    {
       public List<string> mLeaders = new List<string>();
       private XmlDocument xmlDoc = null;
       public void Load()
       {
          mLeaders.Clear();
          xmlDoc = new XmlDocument();
          xmlDoc.Load(CoreGlobals.getWorkPaths().mGameDataDirectory + "\\Leaders.xml");

          //pops
          XmlNodeList nodes = xmlDoc.SelectNodes("//Leaders/Leader/@Name");
          foreach (XmlNode n in nodes)
          {
             mLeaders.Add(n.Value);
          }
          mLeaders.Sort();
       }
    }


   public class Powers
   {
      public List<string> mPowers = new List<string>();
      private XmlDocument xmlDoc = null;
      public void Load()
      {
         mPowers.Clear();
         xmlDoc = new XmlDocument();
         xmlDoc.Load(CoreGlobals.getWorkPaths().mGameDataDirectory + "\\Powers.xml");

         //pops
         XmlNodeList nodes = xmlDoc.SelectNodes("//Powers/Power/@name");
         foreach (XmlNode n in nodes)
         {
            mPowers.Add(n.Value);
         }
         mPowers.Sort();
      }
   }

   public class ExposedScripts
   {
      public List<String> mExposedScriptNames = new List<String>();
      public List<String> mExposedScriptIDs = new List<String>();
      private XmlDocument xmlDoc = null;
      public void Load()
      {
      }
   }

   public class HintConcepts
   {
      public List<String> mConceptNames = new List<String>();
      public List<String> mConceptIDs = new List<String>();
      private XmlDocument xmlDoc = null;
      public void Load()
      {
         mConceptNames.Clear();
         mConceptIDs.Clear();
         xmlDoc = new XmlDocument();
         xmlDoc.Load(CoreGlobals.getWorkPaths().mGameDataDirectory + "\\concepts.xml");
         XmlNodeList nodes = xmlDoc.SelectNodes("//ProtoConceptDatabase/ProtoConcept");
         foreach (XmlNode n in nodes)
         {
            XmlNode name = n.Attributes.GetNamedItem("name");
            XmlNode id = n.Attributes.GetNamedItem("id");
            if ((name != null) && (id != null))
            {
               mConceptNames.Add(name.Value);
               mConceptIDs.Add(id.Value);
            }
         }
      }

   }
}
