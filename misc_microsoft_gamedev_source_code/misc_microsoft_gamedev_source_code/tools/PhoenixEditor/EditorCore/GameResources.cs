using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using System.Xml;
using System.Xml.XPath;
using System.Xml.Serialization;


namespace EditorCore
{
   public class GameResources
   {
      public StringTable mStringTable = new StringTable();

      private List<EditorCinematic> mEditorCinematics = new List<EditorCinematic>();
      private List<EditorLightset> mEditorLightsets = new List<EditorLightset>();
      private List<EditorCinematic> mTalkingHeadVideos = new List<EditorCinematic>();

      public GameResources()
      {
         LoadResouces();
      }

      public void LoadResouces()
      {
         mStringTable.Load();

      }


      public List<EditorCinematic> Cinematics
      {
         set { mEditorCinematics = value; }
         get { return mEditorCinematics; }
      }

      //Cinematic list operations
      public int getNumCinematics() { return (mEditorCinematics.Count); }

      public EditorCinematic getCinematic(int index)
      {
         if((index < 0) || (index >= mEditorCinematics.Count))
            return null;

         return (mEditorCinematics[index]);
      }

      public void AddCinematic(EditorCinematic cinematic)
      {
         mEditorCinematics.Add(cinematic);
      }

      public void AddCinematic(string cinematicName)
      {
         EditorCinematic cinematic = new EditorCinematic(cinematicName, -1, false);
         mEditorCinematics.Add(cinematic);
      }

      public void ClearCinematics()
      {
         mEditorCinematics.Clear();
      }

      public void RemoveCinematic(string cinematicName)
      {
         for (int i = 0; i < mEditorCinematics.Count; i++)
         {
            if (String.Compare(mEditorCinematics[i].Name, cinematicName, true) == 0)
            {
               mEditorCinematics.RemoveAt(i);
               break;
            }
         }
      }

      public bool ContainsCinematic(string cinematicName)
      {
         for (int i = 0; i < mEditorCinematics.Count; i++)
         {
            if (String.Compare(mEditorCinematics[i].Name, cinematicName, true) == 0)
               return true;
         }

         return false;
      }


      public List<EditorCinematic> TalkingHeads
      {
         get { return mTalkingHeadVideos; }
      }

      public int getNumTalkingHeadVideos() { return (mTalkingHeadVideos.Count); }

      public EditorCinematic getTalkingHeadVideo(int index)
      {
         if ((index < 0) || (index >= mTalkingHeadVideos.Count))
            return null;

         return (mTalkingHeadVideos[index]);
      }

      public void AddTalkingHeadVideo(EditorCinematic video)
      {
         mTalkingHeadVideos.Add(video);
      }

      public void AddTalkingHeadVideo(string video)
      {
         EditorCinematic vid = new EditorCinematic(video, -1, false);
         mTalkingHeadVideos.Add(vid);
      }

      public void ClearTalkingHeadVideos()
      {
         mTalkingHeadVideos.Clear();
      }

      public void RemoveTalkingHeadVideo(string videoName)
      {
         for (int i = 0; i < mTalkingHeadVideos.Count; i++)
         {
            if (String.Compare(mTalkingHeadVideos[i].Name, videoName, true) == 0)
            {
               mTalkingHeadVideos.RemoveAt(i);
               break;
            }
         }
      }

      public bool ContainsTalkingHeadVideo(string videoName)
      {
         for (int i = 0; i < mTalkingHeadVideos.Count; i++)
         {
            if (String.Compare(mTalkingHeadVideos[i].Name, videoName, true) == 0)
               return true;
         }

         return false;
      }

      public void FixTalkingHeadDuplicates()
      {
         List<EditorCinematic> tempList = new List<EditorCinematic>();
         tempList.AddRange(mTalkingHeadVideos);
         mTalkingHeadVideos.Clear();
         for (int i = 0; i < tempList.Count; i++)
         {
            EditorCinematic item = tempList[i];
            if (ContainsTalkingHeadVideo(item.Name) == false)
            {
               mTalkingHeadVideos.Add(item);
            }
         }

      }



      public List<EditorLightset> Lightsets
      {
         set { mEditorLightsets = value; }
         get { return mEditorLightsets; }
      }

      //Lightset list operations
      public int getNumLightsets() { return (mEditorLightsets.Count); }

      public EditorLightset getLightset(int index)
      {
         if ((index < 0) || (index >= mEditorLightsets.Count))
            return null;

         return (mEditorLightsets[index]);
      }

      public void AddLightset(EditorLightset Lightset)
      {
         mEditorLightsets.Add(Lightset);
      }

      public EditorLightset AddLightset(string LightsetName)
      {
         EditorLightset Lightset = new EditorLightset(LightsetName, -1, false);
         mEditorLightsets.Add(Lightset);
         return Lightset;
      }
      public void ClearLightsets()
      {
         mEditorLightsets.Clear();
      }


      public void RemoveLightset(string LightsetName)
      {
         for (int i = 0; i < mEditorLightsets.Count; i++)
         {
            if (String.Compare(mEditorLightsets[i].Name, LightsetName, true) == 0)
            {
               mEditorLightsets.RemoveAt(i);
               break;
            }
         }
      }

      public bool ContainsLightset(string LightsetName)
      {
         for (int i = 0; i < mEditorLightsets.Count; i++)
         {
            if (String.Compare(mEditorLightsets[i].Name, LightsetName, true) == 0)
               return true;
         }

         return false;
      }



   }


   public class LocString
   {
      public int mLocID = 0;
      public string mLocIDAsString = "";
      public string mString = "";
      public string mCategory = "";
      public string mScenario = "";

      private string mToString = null;

      public override string ToString()
      {
         return string.Format("[{0}] {1}", mLocID, mString);    // lazy init
      }
   };

   public class StringTable
   {

      public Dictionary<string, LocString> mStringsByID = new Dictionary<string, LocString>();
      public Dictionary<string, Dictionary<string, LocString>> mStringsByScenario = new Dictionary<string, Dictionary<string, LocString>>();
      public Dictionary<string, Dictionary<string, LocString>> mStringsByCategory = new Dictionary<string, Dictionary<string, LocString>>();

      private XmlDocument mStringTableDoc;
      public void Load()
      {
         mStringsByID.Clear();
         mStringsByScenario.Clear();
         mStringsByCategory.Clear();

         mStringTableDoc = new XmlDocument();
         string path = Path.Combine(CoreGlobals.getWorkPaths().mGameDataDirectory, "stringtable.xml");
         if (!File.Exists(path))
            return;

         mStringTableDoc.Load(path);

         XmlNodeList strings = mStringTableDoc.SelectNodes("//String");
         foreach (XmlNode s in strings)
         {

            int locID = 0;
            string locIDAsString = "";
            try
            {
               locIDAsString = s.Attributes["_locID"].InnerText;
               locID = int.Parse(locIDAsString);
            }
            catch (Exception e)
            {
               // can we log this exception somewhere? - it should never happen :)
               continue;
            }

            LocString locString = new LocString();
            locString.mLocIDAsString = locIDAsString;
            locString.mLocID = locID;
            locString.mString = s.InnerText;
            locString.mCategory = (s.Attributes["category"] != null) ? s.Attributes["category"].InnerText : "";
            locString.mScenario = (s.Attributes["scenario"] != null) ? s.Attributes["scenario"].InnerText : "<unassigned>";

            mStringsByID.Add(locString.mLocIDAsString, locString);

            if (locString.mScenario.Length > 0)
            {
               Dictionary<string, LocString> scenarioStrings; 
               if(mStringsByScenario.TryGetValue(locString.mScenario, out scenarioStrings) == false)
               {
                  scenarioStrings = new Dictionary<string, LocString>();
                  mStringsByScenario.Add(locString.mScenario, scenarioStrings);
               }

               scenarioStrings.Add(locString.mLocIDAsString, locString);

            }
            if (locString.mCategory.Length > 0)
            {
               Dictionary<string, LocString> CategoryStrings;
               if (mStringsByCategory.TryGetValue(locString.mCategory, out CategoryStrings) == false)
               {
                  CategoryStrings = new Dictionary<string, LocString>();
                  mStringsByCategory.Add(locString.mCategory, CategoryStrings);
               }
               CategoryStrings.Add(locString.mLocIDAsString, locString);
            }
         }
      }
   };


   public class EditorCinematic
   {
      private string mName = "";
      private int mID = -1;

      static public UniqueID sCinematicIDs = new UniqueID();


      public string Name
      {
         set { mName = value; }
         get { return mName; }
      }

      public int ID
      {
         get { return mID; }
         set { mID = value; }
      }

      public EditorCinematic(string name, int id, bool bLoading)
      {
         mName = name; 
         
         if (id == -1)
         {
            mID = sCinematicIDs.GetUniqueID();
         }
         else
         {
            mID = id;
            if (bLoading)
            {
               sCinematicIDs.RegisterID(id);
            }
         }
      }
   };

   public class EditorLightset
   {
      private int mID = -1;
      private string mName = "";
      private string mObjectPropertyForFLSGen = "";
      

      static public UniqueID sLightsetIDs = new UniqueID();

      public int ID
      {
         get { return mID; }
         set { mID = value; }
      }

      public string Name
      {
         set { mName = value; }
         get { return mName; }
      }

      public string ObjectPropertyForFLSGen
      {
         get { return mObjectPropertyForFLSGen; }
         set { mObjectPropertyForFLSGen = value; }
      }
      public int ObjectIDForFLSGen
      {
         get 
         {
            StringDecorator dec;
            if (StringDecorator.TryParse(ObjectPropertyForFLSGen, out dec))
               return Convert.ToInt32(dec.mDecorator);
            return -1; 
         }
      }

     

      public string getIDString()
      {
         return ID.ToString() + "#" + Name;
      }


      public EditorLightset(string name, int id, bool bLoading)
      {
         mName = name;

         if (id == -1)
         {
            mID = sLightsetIDs.GetUniqueID();
         }
         else
         {
            mID = id;
            if (bLoading)
            {
               sLightsetIDs.RegisterID(id);
            }
         }
      }
   };

}
