using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using System.Xml;
using System.Xml.Serialization;

namespace EditorCore
{
   [XmlRoot("Objects")]
   public class GameObjectsXML
   {
      [XmlAttribute("version")]
      public string version;
      [XmlElement("Object", typeof(ObjectXML))]
      public List<ObjectXML> mObjects = new List<ObjectXML>();
   }

   [XmlRoot("Object")]
   public class ObjectXML
   {
      //[XmlAttribute("id")]
      //public int mId;
      [XmlAttribute("name")]
      public string mName;

      [XmlElement("ObjectType", typeof(string))]
      public List<string> mObjectTypes = new List<string>();
      [XmlElement("Flag", typeof(string))]
      public List<string> mFlags = new List<string>();
      [XmlElement("Icon")]
      public string mIconFile;

      [XmlElement("Visual")]
      public string mAnimFile;

      [XmlElement("ObjectClass")]
      public string mObjectClass = "";

      public bool hasFlag(string flag)
      {
         if (mFlags == null) return false;

         if (mFlags.Contains(flag))
         {
            return true;
         }
         return false;
      }


   }

   [XmlRoot("Minimap")]
   public class MinimapXML
   {
      private string mMinimapTexture = "";
      public string MinimapTexture
      {
         get
         {
            return mMinimapTexture;
         }
         set
         {
            mMinimapTexture = value;
         }
      }
   }
   
   [XmlRoot( "Objectives" )]
   public class ObjectivesXML
   {
      [XmlAttribute( "version" )]
      public string version;

      [XmlElement( "Objective", typeof( ObjectiveXML ) )]
      public List<ObjectiveXML> mObjectives = new List<ObjectiveXML>();

      int mMaxObjectiveID = -1;
      [XmlElement( "MaxObjectiveID" )]
      public int MaxObjectiveID
      {
         get
         {
            return mMaxObjectiveID;
         }
         set
         {
            mMaxObjectiveID = value;
         }
      }
   }

   [XmlRoot( "Objective" )]
   public class ObjectiveXML
   {
      [XmlText]
      public string mObjectiveName;

      [XmlAttribute( "id" )]
      public int mID;

      [XmlElement( "Flag", typeof( string ) )]
      public List<string> mFlags = new List<string>();

      // Detects if a flag has been set
      public bool hasFlag( string flag )
      {
         if( mFlags == null )
         {
            return false;
         }

         if( mFlags.Contains( flag ) )
         {
            return true;
         }

         return false;
      }

      // Sets the flag
      public void setFlag( bool value, string flag )
      {
         if( value && !mFlags.Contains( flag ) )
         {
            mFlags.Add( flag );
         }
         else if( !value && mFlags.Contains( flag ) )
         {
            mFlags.Remove( flag );
         }
      }

      // Override the object ToString method
      public override string ToString()
      {
         return mObjectiveName;
      }

      [XmlElement( "Description" )]
      public string mDescription;

      [XmlElement("TrackerText")]
      public string mTrackerText = "";

      [XmlElement("TrackerDuration")]
      public int mTrackerDuration = 8000;

      [XmlElement("MinTrackerIncrement")]
      public int mMinTrackerIncrement = 1;

      [XmlElement( "Hint" )]
      public string mHint;

      [XmlElement("Score")]
      public uint mScore;

      [XmlElement("FinalCount")]
      public int mFinalCount = -1;
   }

   [XmlRoot( "ControllerConfigs" )]
   public class ControllerConfigsXML
   {
      [XmlAttribute( "version" )]
      public string version;

      [XmlElement( "Config", typeof( ConfigXML ) )]
      public List<ConfigXML> mConfigs = new List<ConfigXML>();

      [XmlElement( "FunctionStrings", typeof( FunctionSringsXML ) )]
      public FunctionSringsXML mFunctionStrings = new FunctionSringsXML();

      int mMaxConfigID = -1;
      [XmlElement( "MaxControllerConfigID" )]
      public int MaxConfigID
      {
         get
         {
            return mMaxConfigID;
         }
         set
         {
            mMaxConfigID = value;
         }
      }

      // Copy data
      public void Copy( ControllerConfigsXML assign )
      {
         version      = assign.version;
         mMaxConfigID = assign.mMaxConfigID;

         foreach( ConfigXML config in assign.mConfigs )
         {
            ConfigXML newConfig = new ConfigXML();
            newConfig.Copy( config );
            mConfigs.Add( newConfig );
         }

         mFunctionStrings.Copy( assign.mFunctionStrings );
      }
   }

   [XmlRoot( "Config" )]
   public class ConfigXML
   {
      [XmlAttribute( "name" )]
      public string mConfigName;

      [XmlAttribute( "id" )]
      public int mID;

      [XmlElement( "Function", typeof( FunctionXML ) )]
      public List<FunctionXML> mFunctions = new List<FunctionXML>();

      [XmlIgnore]
      public bool mNameModified = false;

      [XmlIgnore]
      public List<string> mModifiedControls = new List<string>();

      // Override the object ToString method
      public override string ToString()
      {
         return mConfigName;
      }

      // Copy data
      public void Copy( ConfigXML assign )
      {         
         mConfigName = assign.mConfigName;
         mID         = assign.mID;

         mFunctions.Clear();
         foreach( FunctionXML function in assign.mFunctions )
         {
            FunctionXML newFunction = new FunctionXML();
            newFunction.Copy( function );
            mFunctions.Add( newFunction );
         }

         mNameModified = assign.mNameModified;

         mModifiedControls.Clear();
         foreach( string control in assign.mModifiedControls )
         {
            string newControl = control;
            mModifiedControls.Add( newControl );
         }
      }
   }

   [XmlRoot( "FunctionStrings" )]
   public class FunctionSringsXML
   {
      [XmlElement( "FunctionString", typeof( FunctionStringXML ) )]      
      public List<FunctionStringXML> mFunctionString = new List<FunctionStringXML>();

      // Copy data
      public void Copy( FunctionSringsXML assign )
      {
         foreach( FunctionStringXML funcString in assign.mFunctionString )
         {
            FunctionStringXML newFuncString = new FunctionStringXML();
            newFuncString.Copy( funcString );
            mFunctionString.Add( newFuncString );
         }
      }
   }

   [XmlRoot( "FunctionString" )]
   public class FunctionStringXML
   {
      [XmlAttribute( "name" )]
      public string mFunctionStringName;

      [XmlText]
      public string mFunctionString;

      // Override the object ToString method
      public override string ToString()
      {
         return mFunctionString;
      }

      // Copy data
      public void Copy( FunctionStringXML assign )
      {
         mFunctionStringName = assign.mFunctionStringName;
         mFunctionString     = assign.mFunctionString;
      }
   };

   [XmlRoot( "Function" )]
   public class FunctionXML
   {
      [XmlAttribute( "name" )]
      public string mFunctionName;

      [XmlElement( "Key", typeof( string ) )]
      public List<string> mKeys = new List<string>();

      // Detects if a key has been set
      public bool hasKey( string key )
      {
         if( mKeys == null )
         {
            return false;
         }

         if( mKeys.Contains( key ) )
         {
            return true;
         }

         return false;
      }

      // Sets the key
      public void setKey( bool value, string key )
      {
         if( value && !mKeys.Contains( key ) )
         {
            mKeys.Add( key );
         }
         else if( !value && mKeys.Contains( key ) )
         {
            mKeys.Remove( key );
         }
      }

      // Override the function ToString method
      public override string ToString()
      {
         return mFunctionName;
      }

      public void Copy( FunctionXML assign )
      {
         mFunctionName = assign.mFunctionName;

         mKeys.Clear();
         foreach( string key in assign.mKeys )
         {
            string newKey = key;            
            mKeys.Add( newKey );
         }
      }
   }

   [XmlRoot("ScenarioDescriptions")]
   public class ScenarioDescriptionsXml
   {
      List<ScenarioInfoXml> mScenarioInfo = new List<ScenarioInfoXml>();

      //[XmlArrayItem(ElementName = "ScenarioInfo", Type = typeof(PlayerPositionXML))]
      //[XmlArray("ScenarioInfo")]
      [XmlElement("ScenarioInfo", typeof(ScenarioInfoXml))]
      public List<ScenarioInfoXml> ScenarioInfoList
      {
         get
         {
            return mScenarioInfo;
         }
         set
         {
            mScenarioInfo = value;
         }
      }  
   }

   [XmlRoot("ScenarioInfo")]
   public class ScenarioInfoXml
   {
      string mMapNameKeyFrame = "placeholder";
      string mMaxPlayers = "2";
      int mNameStringID = 0;
      int mInfoStringID = 0;
      string mType = "Test";
      string mScenario = "";
      string mLoadingScreen = "";

      [XmlAttribute]
      public string MapName
      {
         get
         {
            return mMapNameKeyFrame;
         }
         set
         {
            mMapNameKeyFrame = value;
         }
      }

      [XmlAttribute]
      public string MaxPlayers
      {
         get
         {
            return mMaxPlayers;
         }
         set
         {
            mMaxPlayers = value;
         }
      }


      [XmlAttribute]
      public int NameStringID
      {
         get
         {
            return mNameStringID;
         }
         set
         {
            mNameStringID = value;
         }
      }

      [XmlAttribute]
      public int InfoStringID
      {
         get
         {
            return mInfoStringID;
         }
         set
         {
            mInfoStringID = value;
         }
      }

      [XmlAttribute]
      public string Type
      {
         get
         {
            return mType;
         }
         set
         {
            mType = value;
         }
      }

      [XmlAttribute]
      public string File
      {
         get
         {
            return mScenario;
         }
         set
         {
            mScenario = value;
         }
      }

      [XmlAttribute]
      public string LoadingScreen
      {
         get { return mLoadingScreen; }
         set { mLoadingScreen = value; }
      }

   }


  // <Squads>
  //<Squad name="unsc_inf_marine_02" type="fixed" pattern="line" width="3">
  //  <UIVisual>unsc\infantry\marine_01\marine_01.vis</UIVisual>
  //  <PortraitIcon>ui\game\icon\unsc\unit\marine\marine</PortraitIcon>
  //  <DisplayNameID>3004</DisplayNameID>
  //  <RolloverTextID>3005</RolloverTextID>
  //  <DisplayNameID>3000</DisplayNameID>
  //  <RolloverTextID>3000</RolloverTextID>
  //  <BuildPoints>11.0000</BuildPoints>
  //  <Cost resourcetype="Supplies">300.0000</Cost>
  //  <Units>
  //    <Unit count="3" role="normal">unsc_inf_marine_01</Unit>
  //    <Unit count="3" role="normal">unsc_inf_heavymarine_01</Unit>
  //  </Units>
  //</Squad>

   [XmlRoot("Squads")]
   public class DataSquadsXML
   {
      [XmlElement("Squad", typeof(ProtoSquadXml))]
      public List<ProtoSquadXml> mSquads = new List<ProtoSquadXml>();


      public static DataSquadsXML ReadSquads()
      {
         string fileName = Path.Combine(CoreGlobals.getWorkPaths().mGameDataDirectory, "squads.xml");

         if (!File.Exists(fileName))
            return null;

         //fileName = AppDomain.CurrentDomain.BaseDirectory + "\\sim\\" + fileName;//"\\sim\\test_sim.xml";
         XmlSerializer s = new XmlSerializer(typeof(DataSquadsXML), new Type[] { });
         Stream st = File.OpenRead(fileName);
         return (DataSquadsXML)s.Deserialize(st);
      }
   }

   [XmlRoot("Squad")]
   public class ProtoSquadXml
   {
      string mName = "";
      [XmlAttribute("name")]
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

      [XmlArrayItem(ElementName = "Unit", Type = typeof(DataSquadUnitXml))]
      [XmlArray("Units")]
      public List<DataSquadUnitXml> mUnits = new List<DataSquadUnitXml>();
   }

   [XmlRoot("Unit")]
   public class DataSquadUnitXml
   {
      string mName = "";
      [XmlText]
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

      int mCount = 0;
      [XmlAttribute("count")]
      public int Count
      {
         get
         {
            return mCount;
         }
         set
         {
            mCount = value;
         }
      }

      string mRole = "Normal";
      [XmlAttribute("role")]
      public string Role
      {
         get
         {
            return mRole;
         }
         set
         {
            
            mRole = value;
         }
      }

   }




   public class ExternalScriptInfo
   {
      string mFileName = "";
      public string FileName
      {
         get
         {
            return mFileName;
         }
         set
         {
            mFileName = value;
         }
      }


   }
}
