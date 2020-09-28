using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using System.IO;
using System.Xml;
using System.Xml.XPath;
using System.Xml.Serialization;

using SimEditor;
using EditorCore;
using VisualEditor.Controls;
using PhoenixEditor.CinematicEditor;

namespace PhoenixEditor.ScenarioEditor
{
   public class TriggerPropOperator : EnumeratedProperty
   {
      public TriggerPropOperator(INamedTypedProperty prop) : base (prop)
      {
         AddEnum("NotEqualTo","!=");
         AddEnum("LessThan","<");
         AddEnum("LessThanOrEqualTo","<=");
         AddEnum("EqualTo","=");
         AddEnum("GreaterThanOrEqualTo",">=");
         AddEnum("GreaterThan",">");

         mDefaultValue = "<";
         //PresentationValue = mDefaultValue;
      }
   }

   public class TriggerPropChatSpeaker : EnumeratedProperty
   {
      public TriggerPropChatSpeaker(INamedTypedProperty prop)
         : base(prop)
      {
         AddEnum("Serena");
         AddEnum("Forge");
         AddEnum("Cutter");
         AddEnum("Voice of God");
         AddEnum("Generic Soldiers");
         AddEnum("Arcadian Police");
         AddEnum("Civilians");
         AddEnum("Anders");
         AddEnum("RhinoCommander");
         AddEnum("Spartan1");
         AddEnum("Spartan2");
         AddEnum("SpartanSniper");
         AddEnum("SpartanRocketLauncher");
         AddEnum("Covenant");
         AddEnum("Arbiter");
         AddEnum("ODST");
         mDefaultValue = "Serena";
      }

      static public string FixUp(string oldValue)
      {
         if (oldValue == null)
            return (null);
         if (oldValue == "")
            return ("");

         int intValue;
         if (!int.TryParse(oldValue, out intValue))
            return (oldValue);

         // If we parsed an int, it's the crappy old 'ID as string' way -> convert it to the name of the enum here.
         if (oldValue == "0") return ("Serena");
         else if (oldValue == "1") return ("Forge");
         else if (oldValue == "2") return ("Cutter");
         else if (oldValue == "3") return ("Voice of God");
         else if (oldValue == "4") return ("Generic Soldiers");
         else if (oldValue == "5") return ("Arcadian Police");
         else if (oldValue == "6") return ("Civilians");
         else if (oldValue == "7") return ("Anders");
         else if (oldValue == "8") return ("RhinoCommander");
         else if (oldValue == "9") return ("Spartan1");
         else if (oldValue == "10") return ("Spartan2");
         else if (oldValue == "11") return ("SpartanSniper");
         else if (oldValue == "12") return ("SpartanRockeLauncher");
         else if (oldValue == "13") return ("Covenant");
         else if (oldValue == "14") return ("Arbiter");
         else if (oldValue == "15") return ("ODST");
         return ("Serena");
      }
   }

   public class TriggerPropTechStatus : EnumeratedProperty
   {
      public TriggerPropTechStatus(INamedTypedProperty prop) : base(prop)
      {
         AddEnum("Unobtainable");
         AddEnum("Obtainable");
         AddEnum("Available");
         AddEnum("Researching");
         AddEnum("Active");
         AddEnum("Disabled");

         mDefaultValue = "Unobtainable";
      }

      static public string FixUp(string oldValue)
      {
         if (oldValue == null)
            return (null);
         if (oldValue == "")
            return ("");

         int intValue;
         if (!int.TryParse(oldValue, out intValue))
            return (oldValue);

         // If we parsed an int, it's the crappy old 'ID as string' way -> convert it to the name of the enum here.
         if (oldValue == "0") return ("Unobtainable");
         else if (oldValue == "1") return ("Obtainable");
         else if (oldValue == "2") return ("Available");
         else if (oldValue == "3") return ("Researching");
         else if (oldValue == "4") return ("Active");
         else if (oldValue == "5") return ("Disabled");
         return ("Unobtainable");
      }
   }

   public class TriggerPropProtoObject : EnumeratedProperty
   {
      public TriggerPropProtoObject(INamedTypedProperty prop, ProtoObjectData protoObjects) : base(prop)
      {
         foreach(string unit in protoObjects.mProtoObjectList)
         {
            AddEnum(unit, unit);
         }
      }
   }

   public class TriggerPropProtoObjectList : EnumeratedCollectionProperty //EnumeratedSetProperty
   {
      public TriggerPropProtoObjectList(INamedTypedProperty prop, ProtoObjectData protoObjects)
         : base(prop)
      {
         foreach (string unit in protoObjects.mProtoObjectList)
         {
            AddEnum(unit, unit);
         }
      }
   }

   public class TriggerPropProtoObjectCollection : EnumeratedCollectionProperty
   {
      public TriggerPropProtoObjectCollection(INamedTypedProperty prop, ProtoObjectData protoObjects)
         : base(prop)
      {
         foreach (string unit in protoObjects.mProtoObjectList)
         {
            AddEnum(unit, unit);
         }
      }
   }


   public class TriggerPropObjective : EnumeratedProperty
   {
      public TriggerPropObjective( INamedTypedProperty prop, ObjectivesXML objectives ) : base( prop )
      {
         foreach( ObjectiveXML objective in objectives.mObjectives )
         {
            AddEnum( objective.mID.ToString(), objective.mObjectiveName );
         }
      }
   }

   public class TriggerPropMessageIndex : EnumeratedProperty
   {
      public TriggerPropMessageIndex( INamedTypedProperty prop )
         : base( prop )
      {
         for( int i = 0; i < 20; i++ )
         {
            AddEnum( i.ToString(), i.ToString() );
         }
      }
   }

   public class TriggerPropMessageJustify : EnumeratedProperty
   {
      public TriggerPropMessageJustify( INamedTypedProperty prop )
         : base( prop )
      {
         for( int i = 0; i < 3; i++ )
         {
            string justify = "Left";
            switch( i )
            {
               case 0:
                  justify = "Left";
                  break;

               case 1:
                  justify = "Center";
                  break;

               case 2:
                  justify = "Right";
                  break;
            }
            AddEnum( i.ToString(), justify );
         }
      }
   }

   public class TriggerPropMessagePoint : EnumeratedProperty
   {
      public TriggerPropMessagePoint( INamedTypedProperty prop )
         : base( prop )
      {
         for( int i = 12; i < 50; i = i + 2 )
         {
            AddEnum( i.ToString(), i.ToString() );
         }
      }
   }


   public class TriggerPropLeader : EnumeratedSetProperty
   {
      public TriggerPropLeader(INamedTypedProperty prop, Leaders leadersData)
         : base(prop)
      {
         foreach (string leader in leadersData.mLeaders)
         {
            AddEnum(leader, leader);
         }
      }
   }

   public class TriggerPropColor : ColorProperty
   {
      public TriggerPropColor( INamedTypedProperty prop )
         : base( prop )
      {
         mbUseTextFormat = true;
      }      
   }

   public class TriggerPropMathOperator : EnumeratedProperty
   {
      public TriggerPropMathOperator( INamedTypedProperty prop ) : base( prop )
      {
         AddEnum( "Add", "+" );
         AddEnum( "Subtract", "-" );
         AddEnum( "Multiply", "*" );
         AddEnum( "Divide", "/" );
         //AddEnum( "Modulus", "%" );

         mDefaultValue = "+";
      }
   }

   public class TriggerPropCivOperator : EnumeratedProperty
   {
      public TriggerPropCivOperator(INamedTypedProperty prop)
         : base(prop)
      {
         AddEnum("Gaia", "Gaia");
         AddEnum("UNSC", "UNSC");
         AddEnum("Covenant", "Covenant");
         mDefaultValue = "Gaia";
      }
   }

   public class TriggerPropObjectDataType : EnumeratedProperty
   {
      public TriggerPropObjectDataType(INamedTypedProperty prop, TechData techData) : base( prop )
      {
         foreach (string dataSubType in techData.mTechDataSubType)
         {
            AddEnum(dataSubType, dataSubType);
         }
      }
   }

   public class TriggerPropObjectDataRelative : EnumeratedProperty
   {
      public TriggerPropObjectDataRelative(INamedTypedProperty prop, TechData techData)
         : base(prop)
      {
         foreach (string dataRelativity in techData.mTechDataRelativity)
         {
            AddEnum(dataRelativity, dataRelativity);
         }
      }
   }

    public class TriggerPropObjectType : EnumeratedProperty
    {
        public TriggerPropObjectType(INamedTypedProperty prop, ObjectTypeData objectTypes)
            : base(prop)
        {
            foreach (string objectType in objectTypes.mObjectTypeList)
            {
                AddEnum(objectType, objectType);
            }
        }
    }

   public class TriggerPropObjectTypeList : EnumeratedSetProperty
   {
      public TriggerPropObjectTypeList(INamedTypedProperty prop, ObjectTypeData objectTypes)
         : base(prop)
      {
         foreach (string objectType in objectTypes.mObjectTypeList)
         {
            AddEnum(objectType, objectType);
         }
      }
   }

    public class TriggerPropProtoSquad : EnumeratedProperty
    {
        public TriggerPropProtoSquad(INamedTypedProperty prop, ProtoSquadData protoSquads)
            : base(prop)
        {
            foreach (string squad in protoSquads.mProtoSquadList)
            {
                AddEnum(squad, squad);
            }
        }
    }

   public class TriggerPropProtoSquadList : EnumeratedCollectionProperty// EnumeratedSetProperty
   {
      public TriggerPropProtoSquadList(INamedTypedProperty prop, ProtoSquadData protoSquads)
         : base(prop)
      {
         foreach (string squad in protoSquads.mProtoSquadList)
         {
            AddEnum(squad, squad);
         }
      }
   }

   public class TriggerPropProtoSquadCollection : EnumeratedCollectionProperty
   {
      public TriggerPropProtoSquadCollection(INamedTypedProperty prop, ProtoSquadData protoSquads)
         : base(prop)
      {
         foreach (string squad in protoSquads.mProtoSquadList)
         {
            AddEnum(squad, squad);
         }
      }
   }
   public class TriggerPropTech : EnumeratedProperty
   {
      public TriggerPropTech(INamedTypedProperty prop, TechData techData)
         : base(prop)
      {
         foreach (string tech in techData.mTechList)
         {
            AddEnum(tech, tech);
         }
      }
   }

   public class TriggerPropTechList : EnumeratedCollectionProperty //EnumeratedSetProperty
   {
      public TriggerPropTechList(INamedTypedProperty prop, TechData techData)
         : base(prop)
      {
         foreach (string tech in techData.mTechList)
         {
            AddEnum(tech, tech);
         }
      }
   }
   
   public class TriggerPropPlayer :EnumeratedProperty
   {
      public TriggerPropPlayer(INamedTypedProperty prop)
         : base(prop)
      {
         AddEnum("0");
         AddEnum("1");
         AddEnum("2");
         AddEnum("3");
         AddEnum("4");
         AddEnum("5");
         AddEnum("6");
         AddEnum("7");
         AddEnum("8");
      }
   }
   public class TriggerPropPlayerList : EnumeratedSetProperty
   {
      public TriggerPropPlayerList(INamedTypedProperty prop)
         : base(prop)
      {
         AddEnum("1");
         AddEnum("2");
         AddEnum("3");
         AddEnum("4");
         AddEnum("5");
         AddEnum("6");
         AddEnum("7");
         AddEnum("8");

         mSize = new Size(250, 300);
      }
   }

   public class TriggerPropIntegerList : HighLevelProperty
   {
      public TriggerPropIntegerList(INamedTypedProperty prop)
         : base(prop)
      {
      }
   }

   public class TriggerPropLOSType : EnumeratedProperty
   {
      public TriggerPropLOSType(INamedTypedProperty prop)
         : base(prop)
      {
         AddEnum("LOSDontCare");
         AddEnum("LOSNormal");
         AddEnum("LOSFullVisible");
         mDefaultValue = "LOSDontCare";
      }
   }

   public class TriggerPropTeam : EnumeratedProperty
   {
      public TriggerPropTeam(INamedTypedProperty prop)
         : base(prop)
      {
         AddEnum("0");
         AddEnum("1");
         AddEnum("2");
         AddEnum("3");
         AddEnum("4");
         AddEnum("5");
         AddEnum("6");
         AddEnum("7");
         AddEnum("8");
      }
   }
   public class TriggerPropTeamList : EnumeratedSetProperty
   {
      public TriggerPropTeamList(INamedTypedProperty prop)
         : base(prop)
      {
         AddEnum("1");
         AddEnum("2");
         AddEnum("3");
         AddEnum("4");
         AddEnum("5");
         AddEnum("6");
         AddEnum("7");
         AddEnum("8");

         mSize = new Size(250, 300);
      }

   }

   public class TriggerPropTalkingHead : EnumeratedProperty
   {
      public TriggerPropTalkingHead(INamedTypedProperty prop, List<EditorCinematic> videos)
         : base(prop)
      {
         foreach (EditorCinematic vid in videos)
         {
            AddEnum(vid.ID.ToString(), vid.Name);
         }
      }
   }

   public class TriggerPropCinematic : EnumeratedProperty
   {
      public TriggerPropCinematic(INamedTypedProperty prop, List<EditorCinematic> cinematics)
         : base(prop)
      {
         foreach (EditorCinematic cinematic in cinematics)
         {
            AddEnum(cinematic.ID.ToString(), cinematic.Name);
         }
      }
   }

   public class TriggerPropCinematicTag : EnumeratedProperty
   {
      public TriggerPropCinematicTag(INamedTypedProperty prop, List<EditorCinematic> cinematics)
         : base(prop)
      {
         if (cinematics.Count == 0)
            return;

         string cinematicFileName = CoreGlobals.getWorkPaths().mGameArtDirectory + @"\" + cinematics[0].Name + ".cin";
         if (!File.Exists(cinematicFileName))
         {
            //CoreGlobals.getErrorManager().OnSimpleWarning("Error loading terrain tile types.  Can't find " + fileName);
            return;
         }

         // Load it
         Stream st = File.OpenRead(cinematicFileName);
         cinematic cinematic = null;
         try
         {
            XmlSerializer s = new XmlSerializer(typeof(cinematic), new Type[] { });
            cinematic = (cinematic)s.Deserialize(st);
         }
         catch (System.Exception ex)
         {
            st.Close();
            return;
         }
         st.Close();


         uint count = 0;
         foreach (cinematicShot shot in cinematic.body)
         {
            foreach (cinematicShotTag tag in shot.tag)
            {
               switch (tag.type)
               {
                  case cinematicShotTag.TagType.Trigger:
                     AddEnum(count.ToString(), tag.name);
                     count++;
                     break;
               }
            }
         }
      }
   }

   public class ReferenceIDProperty : HighLevelProperty
   {
      public ReferenceIDProperty(INamedTypedProperty prop) : base(prop)
      {
      }
   }



   //public class TriggerPropEntity : ReferenceIDProperty
   //{
   //   public TriggerPropEntity(INamedTypedProperty prop) : base(prop)
   //   {
   //   }

   //   public override Control GetEditor(out string bindPropName)
   //   {
   //      //SplitContainer c = new SplitContainer();
   //      //c.Panel1.Controls.Add()
   //      Button b = new Button();

   //      if (PresentationValue != null)
   //      {
   //         b.Text = PresentationValue.ToString();
   //      }
   //      else
   //      {
   //         b.Text = "Pick unit.";
   //      }
   //      b.Click += new EventHandler(b_Click);

   //      bindPropName = "Text";

   //      return b;
   //   }

   //   void b_Click(object sender, EventArgs e)
   //   {

   //      foreach (EditorObject edObj in SimGlobals.getSimMain().mSelectedEditorObjects)
   //      {
   //         //hmm, this needs to be refactored.
   //         SimObject simObj = edObj as SimObject;
   //         if ((simObj != null) && (simObj.GetProperties() != null))
   //         {
   //            int id = ((SimObjectData)simObj.GetProperties()).ID;
               
   //            PresentationValue = id.ToString();
   //            ((Button)sender).Text = simObj.GetTypedProperties().EditorName;// +id.ToString();
   //            return;
   //         }
   //      }
   //   }
   //   //Reference Type:

   //   //mParmTypes.add("Entity", cParmTypeEntity);
   //   //// This is the ID of the entity placed on the map.  Was going to call this type “Object”, but what if it’s a squad of objects?

   //   //Trigger/Condition/Effect?
   //}

   public class TriggerPropEntity : ReferenceIDProperty
   {
      public TriggerPropEntity(INamedTypedProperty prop)
         : base(prop)
      {
      }
      protected string mPrompt = "Pick unit.";
       
      public override Control GetEditor(out string bindPropName)
      {
         RefSetPanel refset = new RefSetPanel();
         Label selectionLabel = new Label();
         SearchPanel searchPanel = new SearchPanel();
         searchPanel.GuestControl = selectionLabel;
         refset.GuestControl = searchPanel;// selectionLabel;
       
         if (PresentationValue != null)
         {
            StringDecorator dec;
            if(StringDecorator.TryParse(PresentationValue.ToString(), out dec))
            {
               selectionLabel.Text = dec.mValue;
            }
            //selectionLabel.Text = PresentationValue.ToString();
         }
         else
         {
            selectionLabel.Text = mPrompt;
         }
         refset.RefButtonClicked += new EventHandler(refset_RefButtonClicked);
         searchPanel.SearchButtonClicked += new EventHandler(searchPanel_SearchButtonClicked);
         bindPropName = "Text";


         refset.Width += 50;
         return refset;
      }

      void searchPanel_SearchButtonClicked(object sender, EventArgs e)
      {
         try
         {
            if (PresentationValue == null)
               return;
            StringDecorator dec;
            if (StringDecorator.TryParse(PresentationValue.ToString(), out dec))
            {
               object value;
               int id;
               string propname;
               if(int.TryParse(dec.mDecorator,out id))
               {
                  SimGlobals.getSimMain().mSelectedEditorObjects.Clear();
                  EditorObject obj = SimGlobals.getSimMain().GetEditorObjectByID(id);
                  SimGlobals.getSimMain().addSelectedObject(obj);
                  SimGlobals.getSimMain().TranslateToSelectedObject();
               }
            }
         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnSimpleWarning(ex.ToString());
         }
      }

      public virtual bool FilterObject(SimObject simobj)
      {
         return true;
      }
      public virtual bool FilterEditorObject(EditorObject edobj)
      {
         return false;
      }
      void refset_RefButtonClicked(object sender, EventArgs e)
      {
         foreach (EditorObject edObj in SimGlobals.getSimMain().mSelectedEditorObjects)
         {
            SimObject simObj = edObj as SimObject;
            if ((simObj != null) && (simObj.GetProperties() != null))
            {
               if (FilterObject(simObj) == false)
                  continue;
               int id = ((SimObjectData)simObj.GetProperties()).ID;
               String editorName = simObj.GetTypedProperties().Name;
               PresentationValue = StringDecorator.ToString(editorName, id.ToString());
               ((RefSetPanel)sender).Text = editorName;// +id.ToString();
               return;
            }
            else //for non sim objects...
            {
               if (FilterEditorObject(edObj) == false)
                  continue;

               if (edObj is IHasID)
               {
                  int id = ((IHasID)edObj).ID;
                  String editorName = edObj.Name;
                  PresentationValue = StringDecorator.ToString(editorName, id.ToString());
                  ((RefSetPanel)sender).Text = editorName;
                  return;
               }
            }
         }
      }  
   }

   public class TriggerPropSquad : TriggerPropEntity
   {
      public TriggerPropSquad(INamedTypedProperty prop)
         : base(prop)
      {
         mPrompt = "Pick Squad.";
      }
      public override bool FilterObject(SimObject simobj)
      {
         SimObjectData data = simobj.GetProperties() as SimObjectData;
         if (data != null && (data.IsSquad == true || data.IsSquad == false)) //redundant for emphasis
         //if (data != null && data.IsSquad == true)
            return true;
         else
            return false; 
      }
   }
   public class TriggerPropUnit : TriggerPropEntity
   {
      public TriggerPropUnit(INamedTypedProperty prop)
         : base(prop)
      {
         mPrompt = "Pick Unit.";
      }
      public override bool FilterObject(SimObject simobj)
      {
         SimObjectData data = simobj.GetProperties() as SimObjectData;
         if (data != null && data.IsSquad == false)
            return true;
         else
            return false;
      }
   }

   public class TriggerPropObject : TriggerPropEntity
   {
      public TriggerPropObject(INamedTypedProperty prop)
         : base(prop)
      {
         mPrompt = "Pick Object.";
      }
      public override bool FilterObject(SimObject simobj)
      {
         SimObjectData data = simobj.GetProperties() as SimObjectData;
         if (data != null && data.IsSquad == false)
            return true;
         else
            return false;
      }
   }

   public class TriggerPropInteger : NumericProperty
   {
      public TriggerPropInteger(INamedTypedProperty prop)
         : base(prop)
      {
         mbInteger = true;
         mbNonZero = false;
         mbOnlyPositive = false;
      }
   }

   public class TriggerPropEntityList : ReferenceIDProperty
   {
      public TriggerPropEntityList(INamedTypedProperty prop)
         : base(prop)
      {
      }


      protected string mDefaultName = "UnitList";

      TextBox mUnitListName;
      SplitContainer mListAndName;
      Label mSelectionLabel;

      public override Control GetEditor(out string bindPropName)
      {
         RefSetPanel refset = new RefSetPanel();
         mSelectionLabel = new Label();
         mUnitListName = new TextBox();

         mUnitListName.Text = mDefaultName;

         //mListAndName = DynamicUIHelpers.MakePair(mUnitListName, mSelectionLabel);


         SearchPanel searchPanel = new SearchPanel();
         //searchPanel.GuestControl = mListAndName;
         searchPanel.GuestControl = mSelectionLabel;
         refset.GuestControl = searchPanel;
         //refset.GuestControl = mListAndName;// mSelectionLabel;

         if (PresentationValue != null)
         {
            StringDecorator dec;
            if (StringDecorator.TryParse(PresentationValue.ToString(), out dec))
            {
               mSelectionLabel.Text = dec.mDecorator;
               mUnitListName.Text = dec.mValue;
               PresentationValue = mSelectionLabel.Text;
            }
            else
            {

               mSelectionLabel.Text = PresentationValue.ToString();
            }
         }
         else
         {
            mSelectionLabel.Text = "Pick units....";
         }
         refset.RefButtonClicked += new EventHandler(refset_RefButtonClicked);

         mUnitListName.TextChanged += new EventHandler(mUnitListName_TextChanged);
         searchPanel.SearchButtonClicked += new EventHandler(searchPanel_SearchButtonClicked);

         bindPropName = "Text";

         refset.Width += 50;
         return refset;
      }

      void searchPanel_SearchButtonClicked(object sender, EventArgs e)
      {
         try
         {
            if (PresentationValue == null)
               return;
            StringDecorator dec;
            if (StringDecorator.TryParse(PresentationValue.ToString(), out dec))
            {
               SimGlobals.getSimMain().mSelectedEditorObjects.Clear();
               string[] ids = dec.mDecorator.Split(',');
               foreach (string idstring in ids)
               {
                  object value;
                  int id;
                  string propname;
                  if (int.TryParse(idstring, out id))
                  {
                     EditorObject obj = SimGlobals.getSimMain().GetEditorObjectByID(id);
                     SimGlobals.getSimMain().addSelectedObject(obj);

                  }
               }
               SimGlobals.getSimMain().TranslateToSelectedObject();

               PresentationValue = dec.mDecorator;
            }
            else
            {
               SimGlobals.getSimMain().mSelectedEditorObjects.Clear();
               string[] ids = PresentationValue.ToString().Split(',');
               foreach (string idstring in ids)
               {
                  int id;
                  if (int.TryParse(idstring, out id))
                  {
                     EditorObject obj = SimGlobals.getSimMain().GetEditorObjectByID(id);
                     SimGlobals.getSimMain().addSelectedObject(obj);

                  }
               }
               SimGlobals.getSimMain().TranslateToSelectedObject();
            
            }
         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnSimpleWarning(ex.ToString());
         }

      }

      void mUnitListName_TextChanged(object sender, EventArgs e)
      {
         ////throw new Exception("The method or operation is not implemented.");
         //StringDecorator dec;
         //if (PresentationValue == null)
         //   return;
         //if (StringDecorator.TryParse(PresentationValue.ToString(), out dec))
         //{
         //   dec.mValue = mUnitListName.Text;

         //   PresentationValue = dec.ToString();
         //}
      }

      public virtual bool FilterObject(EditorObject edObj)
      {
         SimObject simobj = edObj as SimObject;
         if (simobj != null)
         {

            return true;
         }
         return false;
      }

      void refset_RefButtonClicked(object sender, EventArgs e)
      {
         string list = "";
         foreach (EditorObject edObj in SimGlobals.getSimMain().mSelectedEditorObjects)
         {
            if (FilterObject(edObj) == false)
               continue;

            if(list != "")
               list += ",";

            SimObject simObj = edObj as SimObject;
            if ((simObj != null) && (simObj.GetProperties() != null))
            {
               int id = ((SimObjectData)simObj.GetProperties()).ID;
               list += id.ToString();


            }
         }
         //string namedList = StringDecorator.ToString(mUnitListName.Text, list);
         string namedList = list;
         mSelectionLabel.Text = list;         
         
         PresentationValue = namedList;
         //mUnitListName.Text = mUnitListName.Text;

               //int id = ((SimObjectData)simObj.GetProperties()).ID;

               //String editorName = simObj.GetTypedProperties().Name;

               //PresentationValue = StringDecorator.ToString(editorName, id.ToString());

               //((RefSetPanel)sender).Text = editorName;// +id.ToString();
               ////return;

      }
   }

   // marchack - make this type float later.
   public class TriggerPropDistance : ReferenceIDProperty
   {
      public TriggerPropDistance(INamedTypedProperty prop)
         : base(prop)
      {
      }
      Label mRefNameLabel = null;
      NumericSliderControl mSlider = null;
      RefSetPanel mRefSetEditor;
      SearchPanel mSearchPanel;
      public override Control GetEditor(out string bindPropName)
      {
         mRefSetEditor = new RefSetPanel();
         mRefNameLabel = new Label();
         mSlider = new NumericSliderControl();
         mSearchPanel = new SearchPanel();
         mSearchPanel.GuestControl = mRefNameLabel;

         mSlider.Setup(0, 1000, true);

         mSlider.NumericValue = 1;
         mSlider.ValueChanged += new EventHandler(mSlider_ValueChanged);

         StringDecorator helperLink;
         float value;
         if (PresentationValue != null)
         {
            if (StringDecorator.TryParse(PresentationValue.ToString(), out helperLink))
            {
               string vectorType = "Radius";
               if(PresentationValue.ToString().Contains("HalfLengthX"))
               {
                  vectorType = "HalfLengthX";
               }
               else if(PresentationValue.ToString().Contains("HalfLengthY"))
               {
                  vectorType = "HalfLengthY";
               }
               else if(PresentationValue.ToString().Contains("HalfLengthZ"))
               {
                  vectorType = "HalfLengthZ";
               }

               mRefNameLabel.Text = helperLink.mValue + "." + vectorType;
               mRefSetEditor.GuestControl = mSearchPanel;// mRefNameLabel;
            }
            else if (float.TryParse(PresentationValue.ToString(), out value))
            {
               mSlider.NumericValue = value;
               mRefSetEditor.GuestControl = mSlider;
            }

         }
         else
         {
            mRefSetEditor.GuestControl = mSlider;
         }
         mRefSetEditor.RefButtonClicked += new EventHandler(refset_RefButtonClicked);
         mRefSetEditor.ClearRefButtonClicked += new EventHandler(refset_ClearRefButtonClicked);
         mSearchPanel.SearchButtonClicked += new EventHandler(mSearchPanel_SearchButtonClicked);
         bindPropName = "Text";

         mRefSetEditor.Width += 50;
         return mRefSetEditor;
      }

      void mSearchPanel_SearchButtonClicked(object sender, EventArgs e)
      {
         try
         {
            if (PresentationValue == null)
               return;
            StringDecorator dec;
            if (StringDecorator.TryParse(PresentationValue.ToString(), out dec))
            {
               object value;
               int id;
               string propname;
               if (SimGlobals.getSimMain().TryParseObjectPropertyRef(dec.mDecorator, out value, out id, out propname))
               {
                  SimGlobals.getSimMain().mSelectedEditorObjects.Clear();
                  EditorObject obj = SimGlobals.getSimMain().GetEditorObjectByID(id);
                  SimGlobals.getSimMain().addSelectedObject(obj);
                  SimGlobals.getSimMain().TranslateToSelectedObject();
               }
            }
         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnSimpleWarning(ex.ToString());
         }
      }

      void mSlider_ValueChanged(object sender, EventArgs e)
      {
         this.PresentationValue = mSlider.NumericValue.ToString();
      }

      void refset_ClearRefButtonClicked(object sender, EventArgs e)
      {
         mRefSetEditor.GuestControl = mSlider;
         this.PresentationValue = mSlider.NumericValue.ToString();

      }

      ContextMenu mDistanceMenu = new ContextMenu();
      void refset_RefButtonClicked(object sender, EventArgs e)
      {
         foreach (EditorObject edObj in SimGlobals.getSimMain().mSelectedEditorObjects)
         {
            if (edObj.GetTypeName() == "Area")
            {
               HelperAreaObject beacon = edObj as HelperAreaObject;
               if ((beacon != null) && (beacon.GetProperties() != null))
               {
                  //mRefNameLabel.Text = beacon.ToString();
                  mRefNameLabel.Text = beacon.Name + "." + "Radius";

                  mRefSetEditor.GuestControl = mSearchPanel;// mRefNameLabel;
                  PresentationValue = StringDecorator.ToString( beacon.Name, beacon.ID.ToString() + "." + "Radius" );
                  return;
               }
            }
            else if (edObj.GetTypeName() == "AreaBox")
            {
               HelperAreaBoxObject beacon = edObj as HelperAreaBoxObject;
               if ((beacon != null) && (beacon.GetProperties() != null))
               {
                  mDistanceMenu = new ContextMenu();
                  Control c = sender as Control;

                  MenuItem halfLengthXItem = new MenuItem("HalfLengthX");
                  halfLengthXItem.Click += new EventHandler(halfLengthXItem_Click);
                  halfLengthXItem.Tag = beacon;
                  mDistanceMenu.MenuItems.Add(halfLengthXItem);

                  MenuItem halfLengthYItem = new MenuItem("HalfLengthY");
                  halfLengthYItem.Click += new EventHandler(halfLengthYItem_Click);
                  halfLengthYItem.Tag = beacon;
                  mDistanceMenu.MenuItems.Add(halfLengthYItem);

                  MenuItem halfLengthZItem = new MenuItem("HalfLengthZ");
                  halfLengthZItem.Click += new EventHandler(halfLengthZItem_Click);
                  halfLengthZItem.Tag = beacon;
                  mDistanceMenu.MenuItems.Add(halfLengthZItem);

                  MenuItem lengthXItem = new MenuItem("LengthX");
                  lengthXItem.Click += new EventHandler(lengthXItem_Click);
                  lengthXItem.Tag = beacon;
                  mDistanceMenu.MenuItems.Add(lengthXItem);

                  MenuItem lengthYItem = new MenuItem("LengthY");
                  lengthYItem.Click += new EventHandler(lengthYItem_Click);
                  lengthYItem.Tag = beacon;
                  mDistanceMenu.MenuItems.Add(lengthYItem);

                  MenuItem lengthZItem = new MenuItem("LengthZ");
                  lengthZItem.Click += new EventHandler(lengthZItem_Click);
                  lengthZItem.Tag = beacon;
                  mDistanceMenu.MenuItems.Add(lengthZItem);

                  mDistanceMenu.Show(c, new Point(0, 0));
                  return;
               }
            }
         }
      }

      void halfLengthXItem_Click(object sender, EventArgs e)
      {
         MenuItem mi = sender as MenuItem;
         HelperAreaBoxObject beacon = mi.Tag as HelperAreaBoxObject;

         mRefNameLabel.Text = beacon.Name + "." + "HalfLengthX";
         mRefSetEditor.GuestControl = mSearchPanel;
         PresentationValue = StringDecorator.ToString(beacon.Name, beacon.ID.ToString() + "." + "HalfLengthX");
      }

      void halfLengthYItem_Click(object sender, EventArgs e)
      {
         MenuItem mi = sender as MenuItem;
         HelperAreaBoxObject beacon = mi.Tag as HelperAreaBoxObject;

         mRefNameLabel.Text = beacon.Name + "." + "HalfLengthY";
         mRefSetEditor.GuestControl = mSearchPanel;
         PresentationValue = StringDecorator.ToString(beacon.Name, beacon.ID.ToString() + "." + "HalfLengthY");
      }

      void halfLengthZItem_Click(object sender, EventArgs e)
      {
         MenuItem mi = sender as MenuItem;
         HelperAreaBoxObject beacon = mi.Tag as HelperAreaBoxObject;

         mRefNameLabel.Text = beacon.Name + "." + "HalfLengthZ";
         mRefSetEditor.GuestControl = mSearchPanel;
         PresentationValue = StringDecorator.ToString(beacon.Name, beacon.ID.ToString() + "." + "HalfLengthZ");
      }

      void lengthXItem_Click(object sender, EventArgs e)
      {
         MenuItem mi = sender as MenuItem;
         HelperAreaBoxObject beacon = mi.Tag as HelperAreaBoxObject;

         mRefNameLabel.Text = beacon.Name + "." + "LengthX";
         mRefSetEditor.GuestControl = mSearchPanel;
         PresentationValue = StringDecorator.ToString(beacon.Name, beacon.ID.ToString() + "." + "LengthX");
      }

      void lengthYItem_Click(object sender, EventArgs e)
      {
         MenuItem mi = sender as MenuItem;
         HelperAreaBoxObject beacon = mi.Tag as HelperAreaBoxObject;

         mRefNameLabel.Text = beacon.Name + "." + "LengthY";
         mRefSetEditor.GuestControl = mSearchPanel;
         PresentationValue = StringDecorator.ToString(beacon.Name, beacon.ID.ToString() + "." + "LengthY");
      }

      void lengthZItem_Click(object sender, EventArgs e)
      {
         MenuItem mi = sender as MenuItem;
         HelperAreaBoxObject beacon = mi.Tag as HelperAreaBoxObject;

         mRefNameLabel.Text = beacon.Name + "." + "LengthZ";
         mRefSetEditor.GuestControl = mSearchPanel;
         PresentationValue = StringDecorator.ToString(beacon.Name, beacon.ID.ToString() + "." + "LengthZ");
      }
   }

   public class TriggerPropUnitList : TriggerPropEntityList
   {
      public TriggerPropUnitList(INamedTypedProperty prop)
         : base(prop)
      {
         mDefaultName = "UnitList";

      }

      public override bool FilterObject(EditorObject edObj)
      {
         SimObject simobj = edObj as SimObject;
         if (simobj != null)
         {
            SimObjectData data = simobj.GetProperties() as SimObjectData;
            if (data != null && data.IsSquad == false)
               return true;
            else 
               return false;
         }         
         return false;
      }

   }
   public class TriggerPropSquadList : TriggerPropEntityList
   {
      public TriggerPropSquadList(INamedTypedProperty prop)
         : base(prop)
      {
         mDefaultName = "SquadList";
      }

      public override bool FilterObject(EditorObject edObj)
      {
         SimObject simobj = edObj as SimObject;
         if (simobj != null)
         {
            SimObjectData data = simobj.GetProperties() as SimObjectData;
            if (data != null && (data.IsSquad == true || data.IsSquad == false)) //redundant for emphasis
               return true;
            else
               return false;
         }         
         return false;
      }

   }


   public class TriggerPropObjectList : TriggerPropEntityList
   {
      public TriggerPropObjectList(INamedTypedProperty prop)
         : base(prop)
      {
         mDefaultName = "ObjectList";
      }

      public override bool FilterObject(EditorObject edObj)
      {
         SimObject simobj = edObj as SimObject;
         if (simobj != null)
         {
            SimObjectData data = simobj.GetProperties() as SimObjectData;
            if (data != null && data.IsSquad == false)
               return true;
            else
               return false;
         }
         return false;
      }
   }


   public class TriggerPropTrigger : ReferenceIDProperty
   {
      public TriggerPropTrigger(INamedTypedProperty prop) 
         : base(prop)
      {
      }

   }
   
   public class TriggerPropArmy : ReferenceIDProperty
   {
      public TriggerPropArmy(INamedTypedProperty prop)
         : base(prop)
      {
      }

   }


   //Resource Type:
   //mParmTypes.add("Sound", cParmTypeSound); 
   //// Uh, right now it’s the name of the sound cue “my_first_sound”, etc.  Until we change our sound system at least.
   public class TriggerPropSound : ResourceProperty
   {
      public TriggerPropSound(INamedTypedProperty prop)
         : base(prop)
      {
      }

      public override Control GetEditor(out string bindPropName)
      {         
         try
         {
            bindPropName = "FileName";
            SoundBrowseControl c = new SoundBrowseControl();
            
            if (PresentationValue != null)
            {
               c.FileName = PresentationValue.ToString();
            }

            c.ValueChanged += new SoundBrowseControl.ValueChangedDelegate(c_ValueChanged);

            return c;
         }
         catch(System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnSimpleWarning(ex.ToString());
         }
         return base.GetEditor(out bindPropName);
      }

      void c_ValueChanged(object sender, EventArgs e)
      {
         PresentationValue = ((SoundBrowseControl)sender).FileName;
      }
   }



   public class StringBinder
   {
      string val;
      public StringBinder(string v) { val = v; }
      public StringBinder() { }
      public string Value
      {
         get
         {
            return val;
         }
         set
         {
            val = value;
         }
      }
   }

   public class TriggerPropVectorList : HighLevelProperty
   {
      public TriggerPropVectorList(INamedTypedProperty prop)
         : base(prop)
      {
      }
      BasicTypedSuperList mVectorList = new BasicTypedSuperList();
      public override Control GetEditor(out string bindPropName)
      {
         try
         {
            bindPropName = "??";
            mVectorList = new BasicTypedSuperList();

            //Button clearAll = new Button();

            List<StringBinder> st = new List<StringBinder>();

            mVectorList.SetTypeEditor("StringBinder", "Value", typeof(TriggerPropVector));
            mVectorList.mListDataObjectType = typeof(StringBinder);
            mVectorList.AnyObjectPropertyChanged += new BasicTypedSuperList.PropertyChanged(c_AnyObjectPropertyChanged);
            mVectorList.NewObjectAdded += new BasicTypedSuperList.ObjectChanged(c_NewObjectAdded);
            mVectorList.ObjectDeleted += new BasicTypedSuperList.ObjectChanged(c_ObjectDeleted);

            mVectorList.AutoScroll = true;

            if (PresentationValue != null)
            {
               string[] values = PresentationValue.ToString().Split('|');
               foreach (string v in values)
               {
                  if (v != "")
                  {
                     st.Add(new StringBinder(v));
                  }
               }
            }
            mVectorList.ObjectList = st;

            return mVectorList;
         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnSimpleWarning(ex.ToString());
         }
         return base.GetEditor(out bindPropName);
      }

      void c_ObjectDeleted(ObjectEditorControl sender, object selectedObject)
      {
         UpdateData();
      }
      void c_NewObjectAdded(ObjectEditorControl sender, object selectedObject)
      {
         UpdateData();
      }
      void c_AnyObjectPropertyChanged(ObjectEditorControl sender, object selectedObject, HighLevelProperty prop)
      {
         UpdateData();
      }

      protected void UpdateData()
      {
         string result = "";
         foreach (object o in mVectorList.ObjectList)
         {
            StringBinder sb = o as StringBinder;
            if (sb != null)
            {
               if (result != "")
                  result += "|";
               result += sb.Value;
            }
         }
         if (result != "")
         {
            PresentationValue = result;
         }

      }


   }

   public class TriggerPropTime : HighLevelProperty
   {

      public TriggerPropTime(INamedTypedProperty prop)
         : base(prop)
      {
         //mbNonZero = false;
         //mbOnlyPositive = true;
      }
      //mParmTypes.add(“Time”, cParmTypeTime);
      //// Stored as a DWORD, but probably inputted as a non-negative float by designers.

      TimeEditor mTimeEditor;
      public override Control GetEditor(out string bindPropName)
      {
         mTimeEditor = new TimeEditor();

         bindPropName = "MilliSeconds";

         float timeValue;
         if(PresentationValue == null)
         {
            mTimeEditor.MilliSeconds = 0;
         }
         else if(float.TryParse(PresentationValue.ToString(), out timeValue))
         {
            mTimeEditor.MilliSeconds = timeValue;
         }
         else 
         {
            mTimeEditor.MilliSeconds = 0;
         }

         mTimeEditor.ValueChanged += new EventHandler(mTimeEditor_ValueChanged);

         return mTimeEditor;
      }

      void mTimeEditor_ValueChanged(object sender, EventArgs e)
      {
         PresentationValue = mTimeEditor.MilliSeconds.ToString();
      }


   }

   public class TriggerPropPower : EnumeratedProperty
   {
      public TriggerPropPower(INamedTypedProperty prop, Powers powers)
         : base(prop)
      {
         foreach (string power in powers.mPowers)
         {
            AddEnum(power, power);
         }
      }
   }

   public class TriggerPropVector : ReferenceIDProperty
   {
      public TriggerPropVector(INamedTypedProperty prop)
         : base(prop)
      {
      }
      Label mRefNameLabel = null;
      TextBox mVectorTextBox = null;
      RefSetPanel mRefSetEditor;
      SearchPanel mSearchPanel;

      public override Control GetEditor(out string bindPropName)
      {
         mRefSetEditor = new RefSetPanel();
         mRefNameLabel = new Label();
         mVectorTextBox = new TextBox();
         mSearchPanel = new SearchPanel();
         mSearchPanel.GuestControl = mRefNameLabel;

         mVectorTextBox.Text = "0,0,0";
         mVectorTextBox.TextChanged += new EventHandler(vectorEntry_TextChanged);

         StringDecorator helperLink;

         if (PresentationValue != null)
         {
            if (TextVectorHelper.IsValidVector3(PresentationValue.ToString()))
            {
               mVectorTextBox.Text = PresentationValue.ToString();
               mRefSetEditor.GuestControl = mVectorTextBox;
            }
            else if (StringDecorator.TryParse(PresentationValue.ToString(), out helperLink))
            {
               string vectorType = "Position";
               if (PresentationValue.ToString().Contains("Direction"))
               {
                  vectorType = "Direction";
               }

               mRefNameLabel.Text = helperLink.mValue + "." + vectorType;
               mRefSetEditor.GuestControl = mSearchPanel;
            }
         }
         else
         {
            mRefSetEditor.GuestControl = mVectorTextBox;
         }
         mRefSetEditor.RefButtonClicked += new EventHandler(refset_RefButtonClicked);
         mRefSetEditor.ClearRefButtonClicked += new EventHandler(refset_ClearRefButtonClicked);
         mSearchPanel.SearchButtonClicked += new EventHandler(mSearchPanel_SearchButtonClicked);
         bindPropName = "Text";
         return mRefSetEditor;
      }

      void mSearchPanel_SearchButtonClicked(object sender, EventArgs e)
      {
         try
         {
            if (PresentationValue == null)
               return;
            StringDecorator dec;
            if (StringDecorator.TryParse(PresentationValue.ToString(), out dec))
            {
               object value;
               int id;
               string propname;
               if (SimGlobals.getSimMain().TryParseObjectPropertyRef(dec.mDecorator, out value, out id, out propname))
               {
                  SimGlobals.getSimMain().mSelectedEditorObjects.Clear();
                  EditorObject obj = SimGlobals.getSimMain().GetEditorObjectByID(id);
                  SimGlobals.getSimMain().addSelectedObject(obj);
                  SimGlobals.getSimMain().TranslateToSelectedObject();
               }
            }
         }
         catch (System.Exception ex)
         {
            CoreGlobals.getErrorManager().OnSimpleWarning(ex.ToString());
         }
      }

      void refset_ClearRefButtonClicked(object sender, EventArgs e)
      {
         mRefSetEditor.GuestControl = mVectorTextBox;
         PresentationValue = mVectorTextBox.Text;

      }

      void vectorEntry_TextChanged(object sender, EventArgs e)
      {
         TextBox tb = sender as TextBox;
         if (TextVectorHelper.IsValidVector3(tb.Text))
         {
            tb.ForeColor = Color.Black;
            PresentationValue = tb.Text;
         }
         else
         {
            tb.ForeColor = Color.Red;
         }
      }
      ContextMenu mVectorMenu = new ContextMenu();
      void refset_RefButtonClicked(object sender, EventArgs e)
      {
         foreach (EditorObject edObj in SimGlobals.getSimMain().mSelectedEditorObjects)
         {
            HelperAreaObject beaconArea = null;
            HelperAreaBoxObject beaconAreaBox = null;
            bool box = false;
            if (edObj.GetTypeName() == "Area")
            {
               beaconArea = edObj as HelperAreaObject;
            }
            else if (edObj.GetTypeName() == "AreaBox")
            {
               box = true;
               beaconAreaBox = edObj as HelperAreaBoxObject;
            }
            
            if ((beaconArea == null) && (beaconAreaBox == null))
            {
               continue;
            }

            if ((beaconArea != null) && (beaconArea.GetProperties() == null))
            {
               continue;
            }

            if ((beaconAreaBox != null) && (beaconAreaBox.GetProperties() == null))
            {
               continue;
            }
            
            mVectorMenu = new ContextMenu();
            Control c = sender as Control;

            MenuItem positionItem = new MenuItem("Position");
            positionItem.Click += new EventHandler(positionItem_Click);
            if (box)
            {
               positionItem.Tag = beaconAreaBox;
            }            
            else
            {
               positionItem.Tag = beaconArea;
            }
            mVectorMenu.MenuItems.Add(positionItem);

            MenuItem directionItem = new MenuItem("Direction");
            directionItem.Click += new EventHandler(directionItem_Click);
            if (box)
            {
               directionItem.Tag = beaconAreaBox;
            }
            else
            {
               directionItem.Tag = beaconArea;
            }
            mVectorMenu.MenuItems.Add(directionItem);
              
            if (box)
            {
               MenuItem extent1Item = new MenuItem("Extent1");
               extent1Item.Click += new EventHandler(extent1Item_Click);
               extent1Item.Tag = beaconAreaBox;
               mVectorMenu.MenuItems.Add(extent1Item);

               MenuItem extent2Item = new MenuItem("Extent2");
               extent2Item.Click += new EventHandler(extent2Item_Click);
               extent2Item.Tag = beaconAreaBox;
               mVectorMenu.MenuItems.Add(extent2Item);
            }


            mVectorMenu.Show(c, new Point(0, 0));

            return;
         }
      }

      void positionItem_Click(object sender, EventArgs e)
      {
         MenuItem mi = sender as MenuItem;
         HelperAreaObject beaconArea = null;
         HelperAreaBoxObject beaconAreaBox = null;
         if (mi.Tag.GetType().ToString() == "SimEditor.HelperAreaObject")
         {
            beaconArea = mi.Tag as HelperAreaObject;
         }
         else
         {
            beaconAreaBox = mi.Tag as HelperAreaBoxObject;
         }

         string postion = (beaconArea != null) ? beaconArea.Position : beaconAreaBox.Position;
         mVectorTextBox.Text = postion;

         string name = (beaconArea != null) ? beaconArea.Name : beaconAreaBox.Name;
         mRefNameLabel.Text = name + "." + "Position";
         mRefSetEditor.GuestControl = mSearchPanel;
         string id = (beaconArea != null) ? beaconArea.ID.ToString() : beaconAreaBox.ID.ToString();
         PresentationValue = StringDecorator.ToString(name, id + "." + "Position" );
      }

      void directionItem_Click(object sender, EventArgs e)
      {
         MenuItem mi = sender as MenuItem;
         HelperAreaObject beaconArea = null;
         HelperAreaBoxObject beaconAreaBox = null;
         if (mi.Tag.GetType().ToString() == "SimEditor.HelperAreaObject")
         {
            beaconArea = mi.Tag as HelperAreaObject;
         }
         else
         {
            beaconAreaBox = mi.Tag as HelperAreaBoxObject;
         }

         string direction = (beaconArea != null) ? beaconArea.Direction : beaconAreaBox.Direction;
         mVectorTextBox.Text = direction;

         string name = (beaconArea != null) ? beaconArea.Name : beaconAreaBox.Name;
         mRefNameLabel.Text = name + "." + "Direction";
         mRefSetEditor.GuestControl = mSearchPanel;
         string id = (beaconArea != null) ? beaconArea.ID.ToString() : beaconAreaBox.ID.ToString();
         PresentationValue = StringDecorator.ToString( name, id + "." + "Direction" );
      }

      void extent1Item_Click(object sender, EventArgs e)
      {
         MenuItem mi = sender as MenuItem;
         HelperAreaBoxObject beaconAreaBox = null;
         beaconAreaBox = mi.Tag as HelperAreaBoxObject;
                  
         string extent1 = beaconAreaBox.Extent1;
         mVectorTextBox.Text = extent1;

         string name = beaconAreaBox.Name;
         mRefNameLabel.Text = name + "." + "Extent1";
         mRefSetEditor.GuestControl = mSearchPanel;
         string id = beaconAreaBox.ID.ToString();
         PresentationValue = StringDecorator.ToString(name, id + "." + "Extent1");
      }

      void extent2Item_Click(object sender, EventArgs e)
      {
         MenuItem mi = sender as MenuItem;
         HelperAreaBoxObject beaconAreaBox = null;
         beaconAreaBox = mi.Tag as HelperAreaBoxObject;

         string extent2 = beaconAreaBox.Extent2;
         mVectorTextBox.Text = extent2;

         string name = beaconAreaBox.Name;
         mRefNameLabel.Text = name + "." + "Extent2";
         mRefSetEditor.GuestControl = mSearchPanel;
         string id = beaconAreaBox.ID.ToString();
         PresentationValue = StringDecorator.ToString(name, id + "." + "Extent2");
      }
   }

   public class TriggerPropPopBucket : EnumeratedProperty
   {
      public TriggerPropPopBucket( INamedTypedProperty prop, GameData gameData )
         : base( prop )
      {
         foreach( string pop in gameData.mPopTypes )
         {
            AddEnum( pop, pop );
         }
      }
   }

   public class TriggerPropListPosition: EnumeratedProperty
   {
      public TriggerPropListPosition( INamedTypedProperty prop )
         : base( prop )
      {
         AddEnum("First");
         AddEnum("Last");
         AddEnum("Random");

         mDefaultValue = "First";
      }
   }

   public class TriggerPropDiplomacy : EnumeratedProperty
   {
      public TriggerPropDiplomacy( INamedTypedProperty prop )
         : base( prop )
      {
         AddEnum("Any");
         AddEnum("Self");
         AddEnum("Ally");
         AddEnum("Enemy");
         AddEnum("Neutral");
         mDefaultValue = "Neutral";
      }
   }

   public class TriggerPropExposedAction : EnumeratedProperty
   {
      public TriggerPropExposedAction( INamedTypedProperty prop )
         : base(prop)
      {
         AddEnum("ExposedAction0");
         AddEnum("ExposedAction1");
         AddEnum("ExposedAction2");

         mDefaultValue = "0";
      }

      static public string FixUp(string oldValue)
      {
         if (oldValue == null)
            return (null);
         if (oldValue == "")
            return ("");

         int intValue;
         if (!int.TryParse(oldValue, out intValue))
            return (oldValue);

         // If we parsed an int, it's the crappy old 'ID as string' way -> convert it to the name of the enum here.
         if (oldValue == "0") return ("ExposedAction0");
         else if (oldValue == "1") return ("ExposedAction1");
         else if (oldValue == "2") return ("ExposedAction2");
         return ("ExposedAction0");
      }
   }

   public class TriggerPropSquadMode : EnumeratedProperty
   {
      public TriggerPropSquadMode(INamedTypedProperty prop)
         : base(prop)
      {
         AddEnum("Normal");
         AddEnum("StandGround");
         AddEnum("LockDown");
         AddEnum("Sniper");
         AddEnum("HitAndRun");
         AddEnum("Passive");
         AddEnum("Cover");
         AddEnum("Ability");
         AddEnum("CarryingObject");
         AddEnum("Power");
         AddEnum("ScarabScan");
         AddEnum("ScarabTarget");
         AddEnum("ScarabKill");

         mDefaultValue = "Normal";
      }
   }

   public class TriggerPropExposedScript : EnumeratedProperty
   {
      public TriggerPropExposedScript(INamedTypedProperty prop, ExposedScripts exposedScripts)
         : base(prop)
      {
         for (int i = 0; i < exposedScripts.mExposedScriptIDs.Count; i++)
         {
            AddEnum(exposedScripts.mExposedScriptIDs[i], exposedScripts.mExposedScriptNames[i]);
         }
      }
   }

   public class TriggerPropDataScalar : EnumeratedProperty
   {
      public TriggerPropDataScalar(INamedTypedProperty prop)
         : base(prop)
      {
         AddEnum("Accuracy");
         AddEnum("WorkRate");
         AddEnum("Damage");         
         AddEnum("LOS");
         AddEnum("Velocity");
         AddEnum("WeaponRange");
         AddEnum("DamageTaken");

         mDefaultValue = "Accuracy";
      }

      static public string FixUp(string oldValue)
      {
         if (oldValue == null)
            return (null);
         if (oldValue == "")
            return ("");

         int intValue;
         if (!int.TryParse(oldValue, out intValue))
            return (oldValue);

         // If we parsed an int, it's the crappy old 'ID as string' way -> convert it to the name of the enum here.
         if (oldValue == "0") return ("Accuracy");
         else if (oldValue == "1") return ("WorkRate");
         else if (oldValue == "2") return ("Damage");
         else if (oldValue == "3") return ("LOS");
         else if (oldValue == "4") return ("Velocity");
         else if (oldValue == "5") return ("WeaponRange");
         else if (oldValue == "6") return ("DamageTaken");
         return ("Accuracy");
      }
   };

   //<Resource>Supplies</Resource>
   //<Resource>Power</Resource>
   //<Resource>Relics</Resource>
   //<Resource>Organics</Resource>
   //<Resource>Favor</Resource>
   //<Resource>Honor</Resource>
   public class ResourceCost
   {
      int mSupplies = 0;
      public int Supplies { get { return mSupplies; } set { mSupplies = value; } }
      int mPower = 0;
      public int Power { get { return mPower; } set { mPower = value; } }
      int mRelics = 0;
      public int Relics { get { return mRelics; } set { mRelics = value; } }
      //int mOrganics = 0;
      //public int Organics { get { return mOrganics; } set { mOrganics = value; } }
      int mFavor = 0;
      public int Favor { get { return mFavor; } set { mFavor = value; } }
      int mHonor = 0;
      public int Honor { get { return ( mHonor ); } set { mHonor = value; } }

      enum eParameterIDs
      {
         Supplies =0,
         Power = 1,
         Relics = 2,
         //Organics =3,
         Favor = 3,
         Honor = 4,    
      };

      //This is semi automated.  It would be nice to make it fully automated.
      public static bool TryParse(string s, out ResourceCost cost)
      {
         cost = new ResourceCost();
         if (s == null || s == "")
            return false;
         string[] values = s.Split(',');

         int count = 5;
         foreach (string entry in values)
         {
            string[] pair = entry.Split('=');
            int id;
            int value;
            if(int.TryParse(pair[0], out id) && int.TryParse(pair[1], out value))
            {
               if(id == (int)eParameterIDs.Supplies)
               {
                  cost.Supplies = value;
                  count--;
               }
               if (id == (int)eParameterIDs.Power)
               {
                  cost.Power = value;
                  count--;
               }
               if (id == (int)eParameterIDs.Relics)
               {
                  cost.Relics = value;
                  count--;

               }
               //if (id == (int)eParameterIDs.Organics)
               //{
               //   cost.Organics = value;
               //   count--;

               //}
               if (id == (int)eParameterIDs.Favor)
               {
                  cost.Favor = value;
                  count--;

               }
               if( id == (int)eParameterIDs.Honor )
               {
                  cost.Honor = value;
                  count--;
               }
            }
         }
         if (count != 0)
            return false;

         return true;
      }

      public override string ToString()
      {
         return string.Format("{0}={1},{2}={3},{4}={5},{6}={7},{8}={9}", (int)eParameterIDs.Supplies, Supplies,
                                                                          (int)eParameterIDs.Power, Power,
                                                                          (int)eParameterIDs.Relics, Relics,
                                                                          (int)eParameterIDs.Favor, Favor,
                                                                          (int)eParameterIDs.Honor, Honor );
      }
      public string ToDisplayString()
      {
         //return string.Format("{0}={1},{2}={3},{4}={5},{6}={7},{8}={9}", eParameterIDs.Supplies, Supplies,
         //                                                                 eParameterIDs.Power, Power,
         //                                                                 eParameterIDs.Relics, Relics,
         //                                                                 eParameterIDs.Organics, Organics,
         //                                                                 eParameterIDs.Favor, Favor);
         return string.Format("{0},{1},{2},{3},{4}", Supplies,Power,Relics,Favor,Honor);
                                                                          

      }

   }


   public class TriggerPropCost : HighLevelProperty
   {
     public TriggerPropCost(INamedTypedProperty prop)
         : base(prop)
     {

     }

      ResourceCost mResourceData = new ResourceCost();
      public override Control GetEditor(out string bindPropName)
      {
         BetterPropertyGrid grid = new BetterPropertyGrid();
         
         StringDecorator dec;
         if(PresentationValue == null || PresentationValue.ToString() == "")
         {
            mResourceData = new ResourceCost();
            PresentationValue = mResourceData.ToString();
         }
         else if (StringDecorator.TryParse(PresentationValue.ToString(), out dec))
         {
            if (ResourceCost.TryParse(dec.mDecorator, out mResourceData))
            {
               
               //PresentationValue = mResourceData.ToString();
            }
         }
         else
         {
            mResourceData = new ResourceCost();
            PresentationValue = mResourceData.ToString();
         }

         grid.SelectedObject = mResourceData;

         grid.Height = 150;
         bindPropName = "None";

         grid.AnyPropertyChanged += new ObjectEditorControl.PropertyChanged(grid_AnyPropertyChanged);


         grid.Width = 150;
         return grid;

      }

      void grid_AnyPropertyChanged(ObjectEditorControl sender, object selectedObject, HighLevelProperty prop)
      {
         //if (ResourceCost.TryParse(PresentationValue.ToString(), out mResourceData))
         {
            
            PresentationValue = StringDecorator.ToString(mResourceData.ToDisplayString(), mResourceData.ToString());
         }
      }
   }



   public class TriggerPropAnimType : EnumeratedProperty
   {
     public TriggerPropAnimType(INamedTypedProperty prop)
         : base(prop)
     {
         // ATTENTION:  These types need to match the enumeration in protovisual.h of the game code
         AddEnum("Idle");
         AddEnum("Walk");
         AddEnum("Jog");
         AddEnum("Run");
         AddEnum("RangedAttack");
         AddEnum("Attack");
         AddEnum("Limber");
         AddEnum("Unlimber");
         AddEnum("Death");
         AddEnum("Work");
         AddEnum("Research");
         AddEnum("Train");
         AddEnum("Bored");
         AddEnum("PelicanGarrison");
         AddEnum("PelicanGarrison2");
         AddEnum("PelicanGarrison3");
         AddEnum("PelicanGarrison4");
         AddEnum("PelicanGarrison5");
         AddEnum("PelicanGarrison6");
         AddEnum("PelicanGarrison7");
         AddEnum("PelicanGarrison8");
         AddEnum("PelicanGarrison9");
         AddEnum("PelicanGarrison10");
         AddEnum("PelicanUngarrison");
         AddEnum("PelicanUngarrison2");
         AddEnum("PelicanUngarrison3");
         AddEnum("PelicanUngarrison4");
         AddEnum("PelicanUngarrison5");
         AddEnum("PelicanUngarrison6");
         AddEnum("PelicanUngarrison7");
         AddEnum("PelicanUngarrison8");
         AddEnum("PelicanUngarrison9");
         AddEnum("PelicanUngarrison10");
         AddEnum("Incoming");
         AddEnum("Landing");
         AddEnum("Takeoff");
         AddEnum("Outgoing");
         AddEnum("HoverIdle");
         AddEnum("ShadePlasmaAttack");
         AddEnum("Clamshell");
         AddEnum("ClamshellRocket");
         AddEnum("Cinematic");
         AddEnum("EvadeLeft");
         AddEnum("EvadeRight");
         AddEnum("EvadeFront");
         AddEnum("EvadeBack");
         AddEnum("Block");
         AddEnum("Cheer");
         AddEnum("Retreat");   
         AddEnum("HandSignalGo");   
         AddEnum("HandSignalStop");   
         AddEnum("Reload");      
         AddEnum("CombatAction");         
         AddEnum("Flail");         
         AddEnum("Stop");         
         AddEnum("TurnAround");            
         AddEnum("TurnLeft");            
         AddEnum("TurnRight");            
         AddEnum("TurnRight45Forward");            
         AddEnum("TurnRight45Back");                    
         AddEnum("TurnLeft45Back");                    
         AddEnum("TurnLeft45Forward");                    
         AddEnum("TurnWalk");                       
         AddEnum("TurnJog");                       
         AddEnum("TurnRun");                          
         AddEnum("Repair");                          
         AddEnum("FloodDeathJog");                          
         AddEnum("FloodDeath");                          
         AddEnum("IdleWalk");                          
         AddEnum("IdleJog");                          
         AddEnum("IdleRun");                          
         AddEnum("WalkIdle");                          
         AddEnum("JogIdle");                          
         AddEnum("RunIdle");                          
         AddEnum("Hitch");                          
         AddEnum("Unhitch");                          
         AddEnum("Sprint");
         AddEnum("Recover");
         AddEnum("Heal");
         AddEnum("ArtilleryAttack");
         AddEnum("LockdownArtilleryAttack");
         AddEnum("Stasis");
         AddEnum("Roar");
         AddEnum("Cower");
         AddEnum("Kamikaze");
         AddEnum("Stunned");
         AddEnum("Detonated"); 
         AddEnum("ShatterDeath");
         AddEnum("RageLeap"); 
         AddEnum("HotDropDown");

         mDefaultValue = "Idle";         
     }

      static public string FixUp(string oldValue)
      {
         if (oldValue == null)
            return (null);
         if (oldValue == "")
            return ("");

         int intValue;
         if (!int.TryParse(oldValue, out intValue))
            return (oldValue);

         // If we parsed an int, it's the crappy old 'ID as string' way -> convert it to the name of the enum here.
         if (oldValue == "0")      return ("Idle");
         else if (oldValue == "1") return ("Walk");
         else if (oldValue == "2") return ("Jog");
         else if (oldValue == "3") return ("Run");
         else if (oldValue == "4") return ("RangedAttack");
         else if (oldValue == "5") return ("Attack");
         else if (oldValue == "6") return ("Limber");
         else if (oldValue == "7") return ("Unlimber");
         else if (oldValue == "8") return ("Death");
         else if (oldValue == "9") return ("Work");
         else if (oldValue == "10") return ("Research");
         else if (oldValue == "11") return ("Train");
         else if (oldValue == "12") return ("Bored");
         else if (oldValue == "13") return ("PelicanGarrison");
         else if (oldValue == "14") return ("PelicanGarrison2");
         else if (oldValue == "15") return ("PelicanGarrison3");
         else if (oldValue == "16") return ("PelicanGarrison4");
         else if (oldValue == "17") return ("PelicanGarrison5");
         else if (oldValue == "18") return ("PelicanGarrison6");
         else if (oldValue == "19") return ("PelicanGarrison7");
         else if (oldValue == "20") return ("PelicanGarrison8");
         else if (oldValue == "21") return ("PelicanGarrison9");
         else if (oldValue == "22") return ("PelicanGarrison10");
         else if (oldValue == "23") return ("PelicanUngarrison");
         else if (oldValue == "24") return ("PelicanUngarrison2");
         else if (oldValue == "25") return ("PelicanUngarrison3");
         else if (oldValue == "26") return ("PelicanUngarrison4");
         else if (oldValue == "27") return ("PelicanUngarrison5");
         else if (oldValue == "28") return ("PelicanUngarrison6");
         else if (oldValue == "29") return ("PelicanUngarrison7");
         else if (oldValue == "30") return ("PelicanUngarrison8");
         else if (oldValue == "31") return ("PelicanUngarrison9");
         else if (oldValue == "32") return ("PelicanUngarrison10");
         else if (oldValue == "33") return ("Incoming");
         else if (oldValue == "34") return ("Landing");
         else if (oldValue == "35") return ("Takeoff");
         else if (oldValue == "36") return ("Outgoing");
         else if (oldValue == "37") return ("HoverIdle");
         else if (oldValue == "38") return ("ShadePlasmaAttack");
         else if (oldValue == "39") return ("Clamshell");
         else if (oldValue == "40") return ("ClamshellRocket");
         else if (oldValue == "41") return ("Cinematic");
         else if (oldValue == "42") return ("EvadeLeft");
         else if (oldValue == "43") return ("EvadeRight");
         else if (oldValue == "44") return ("EvadeFront");
         else if (oldValue == "45") return ("EvadeBack");
         else if (oldValue == "46") return ("Block");
         else if (oldValue == "47") return ("Cheer");
         else if (oldValue == "48") return ("Retreat");   
         else if (oldValue == "49") return ("HandSignalGo");   
         else if (oldValue == "50") return ("HandSignalStop");   
         else if (oldValue == "51") return ("Reload");      
         else if (oldValue == "52") return ("CombatAction");         
         else if (oldValue == "53") return ("Flail");         
         else if (oldValue == "54") return ("Stop");         
         else if (oldValue == "55") return ("TurnAround");            
         else if (oldValue == "56") return ("TurnLeft");            
         else if (oldValue == "57") return ("TurnRight");            
         else if (oldValue == "58") return ("TurnRight45Forward");     
         else if (oldValue == "59") return ("TurnRight45Back");        
         else if (oldValue == "60") return ("TurnLeft45Back");         
         else if (oldValue == "61") return ("TurnLeft45Forward");      
         else if (oldValue == "62") return ("TurnWalk");               
         else if (oldValue == "63") return ("TurnJog");                
         else if (oldValue == "64") return ("TurnRun");                
         else if (oldValue == "65") return ("Repair");                 
         else if (oldValue == "66") return ("FloodDeathJog");          
         else if (oldValue == "67") return ("FloodDeath");             
         else if (oldValue == "68") return ("IdleWalk");               
         else if (oldValue == "69") return ("IdleJog");                
         else if (oldValue == "70") return ("IdleRun");                
         else if (oldValue == "71") return ("WalkIdle");               
         else if (oldValue == "72") return ("JogIdle");                
         else if (oldValue == "73") return ("RunIdle");                
         else if (oldValue == "74") return ("Hitch");                  
         else if (oldValue == "75") return ("Unhitch");                
         else if (oldValue == "76") return ("Sprint");
         else if (oldValue == "77") return ("Recover");
         else if (oldValue == "78") return ("Heal");
         else if (oldValue == "79") return ("ArtilleryAttack");
         else if (oldValue == "80") return ("LockdownArtilleryAttack");
         else if (oldValue == "81") return ("Stasis");
         else if (oldValue == "82") return ("Roar");
         else if (oldValue == "83") return ("Cower");
         else if (oldValue == "84") return ("Kamikaze");

         return ("Unobtainable");
      }
   }

   public class TriggerPropMissionType : EnumeratedProperty
   {
      public TriggerPropMissionType(INamedTypedProperty prop)
         : base(prop)
      {
         AddEnum("Invalid");
         AddEnum("Attack");
         AddEnum("Defend");
         AddEnum("Scout");
         AddEnum("Claim");
         AddEnum("Power");
         mDefaultValue = "Invalid";
      }
   }

   public class TriggerPropMissionState : EnumeratedProperty
   {
      public TriggerPropMissionState(INamedTypedProperty prop)
         : base(prop)
      {
         AddEnum("Invalid");
         AddEnum("Success");
         AddEnum("Failure");
         AddEnum("Create");
         AddEnum("Rally");
         AddEnum("Transit");
         AddEnum("Working");
         AddEnum("Opportunity");
         AddEnum("Withdraw");
         AddEnum("Retreat");
         mDefaultValue = "Invalid";
      }
   }

   public class TriggerPropAISquadAnalysisComponent : EnumeratedProperty
   {
      public TriggerPropAISquadAnalysisComponent(INamedTypedProperty prop)
         : base(prop)
      {
         AddEnum("CVLight");
         AddEnum("CVLightArmored");
         AddEnum("CVMedium");
         AddEnum("CVMediumAir");
         AddEnum("CVHeavy");
         AddEnum("CVBuilding");
         AddEnum("CVTotal");
         AddEnum("HPLight");
         AddEnum("HPLightArmored");
         AddEnum("HPMedium");
         AddEnum("HPMediumAir");
         AddEnum("HPHeavy");
         AddEnum("HPBuilding");
         AddEnum("HPTotal");
         AddEnum("SPLight");
         AddEnum("SPLightArmored");
         AddEnum("SPMedium");
         AddEnum("SPMediumAir");
         AddEnum("SPHeavy");
         AddEnum("SPBuilding");
         AddEnum("SPTotal");
         AddEnum("DPSLight");
         AddEnum("DPSLightArmored");
         AddEnum("DPSMedium");
         AddEnum("DPSMediumAir");
         AddEnum("DPSHeavy");
         AddEnum("DPSBuilding");
         AddEnum("DPSTotal");
         AddEnum("CVPercentLight");
         AddEnum("CVPercentLightArmored");
         AddEnum("CVPercentMedium");
         AddEnum("CVPercentMediumAir");
         AddEnum("CVPercentHeavy");
         AddEnum("CVPercentBuilding");
         AddEnum("HPPercentLight");
         AddEnum("HPPercentLightArmored");
         AddEnum("HPPercentMedium");
         AddEnum("HPPercentMediumAir");
         AddEnum("HPPercentHeavy");
         AddEnum("HPPercentBuilding");
         AddEnum("CVStarsLight");
         AddEnum("CVStarsLightArmored");
         AddEnum("CVStarsMedium");
         AddEnum("CVStarsMediumAir");
         AddEnum("CVStarsHeavy");
         AddEnum("CVStarsBuilding");
         AddEnum("CVStarsTotal");
      }
   }

   public class TriggerPropBidType : EnumeratedProperty
   {
      public TriggerPropBidType(INamedTypedProperty prop)
         : base(prop)
      {
         AddEnum("Invalid");
         AddEnum("None");
         AddEnum("Squad");
         AddEnum("Tech");
         AddEnum("Building");
         AddEnum("Power");
         mDefaultValue = "Invalid";
      }
   }

   public class TriggerPropBidState : EnumeratedProperty
   {
      public TriggerPropBidState(INamedTypedProperty prop)
         : base(prop)
      {
         AddEnum("Invalid");
         AddEnum("Inactive");
         AddEnum("Waiting");
         AddEnum("Approved");
         mDefaultValue = "Invalid";
      }
   }

   public class TriggerPropMissionTargetType : EnumeratedProperty
   {
      public TriggerPropMissionTargetType(INamedTypedProperty prop)
         : base(prop)
      {
         AddEnum("Invalid");
         AddEnum("Area");
         AddEnum("KBBase");
         AddEnum("CaptureNode");
         mDefaultValue = "Invalid";
      }
   }

   public class TriggerPropActionStatus : EnumeratedProperty
   {
     public TriggerPropActionStatus(INamedTypedProperty prop)
         : base(prop)
     {
         AddEnum("NotDone");
         AddEnum("DoneSuccess");
         AddEnum("DoneFailure");
         mDefaultValue = "NotDone";
     }

      static public string FixUp(string oldValue)
      {
         if (oldValue == null)
            return (null);
         if (oldValue == "")
            return ("");

         int intValue;
         if (!int.TryParse(oldValue, out intValue))
            return (oldValue);

         // If we parsed an int, it's the crappy old 'ID as string' way -> convert it to the name of the enum here.
         if (oldValue == "0") return ("NotDone");
         else if (oldValue == "1") return ("DoneSuccess");
         else if (oldValue == "2") return ("DoneFailure");
         return ("NotDone");
      }
   }


   public class TriggerPropPlayerState : EnumeratedProperty
   {
      public TriggerPropPlayerState(INamedTypedProperty prop, GameData gameData)
         : base(prop)
      {
         foreach(string playerState in gameData.mPlayerStates)
         {
            AddEnum(playerState);
         }
      }
   }

   public class TriggerPropBool : BoolProperty
   {
      public TriggerPropBool(INamedTypedProperty prop)
         : base(prop)
      {
         mbShowTextValue = true;         
         
         //we treat null as false
         if (PresentationValue == null)
            PresentationValue = false;


      }
   }

   public class TriggerPropFloat : NumericProperty
   {
      public TriggerPropFloat(INamedTypedProperty prop)
         : base(prop)
      {
         mbNonZero = false;
         mbOnlyPositive = false;
      }
   }

   public class TriggerPropGroup : EnumeratedProperty
   {
      public TriggerPropGroup(INamedTypedProperty prop, TriggerNamespace triggerNamespace)
         : base(prop)
      {

         foreach (GroupUI group in triggerNamespace.TriggerEditorData.UIData.mGroups)
         {
            AddEnum(group.InternalGroupID.ToString(), group.Title);
         }
      }
   }

   public class TriggerPropUserType : EnumeratedProperty
   {
      public TriggerPropUserType(INamedTypedProperty prop, TriggerNamespace triggerNamespace)
         : base(prop)
      {


         foreach (UserClassDefinition def in TriggerSystemMain.mTriggerDefinitions.mUserClassDefinitions.mUserClasses)
         {
            AddEnum(def.DBID.ToString(), def.Name);
         }

      
      }



   }

   public class TriggerPropRefCountType : EnumeratedProperty
   {
      public TriggerPropRefCountType(INamedTypedProperty prop, List<string> refCountTypes)
         : base(prop)
      {
         foreach (string refCountType in refCountTypes)
         {
            AddEnum(refCountType);
         }
      }
   }

   public class TriggerPropHUDItem : EnumeratedProperty
   {
      public TriggerPropHUDItem(INamedTypedProperty prop, List<string> hudItems)
         : base(prop)
      {
         foreach (string hudItem in hudItems)
         {
            AddEnum(hudItem);
         }
      }
   }

   public class TriggerPropFlashableUIItem : EnumeratedProperty
   {
      public TriggerPropFlashableUIItem(INamedTypedProperty prop, List<string> items)
         : base(prop)
      {
         foreach (string item in items)
         {
            AddEnum(item);
         }
      }
   }


   public class TriggerPropControlType : EnumeratedProperty
   {
      public TriggerPropControlType(INamedTypedProperty prop)
         : base(prop)
      {
         AddEnum("ButtonA");
         AddEnum("ButtonB");
         AddEnum("ButtonX");
         AddEnum("ButtonY");
         AddEnum("ButtonStart");
         AddEnum("ButtonBack");
         AddEnum("ButtonShoulderRight");
         AddEnum("ButtonShoulderLeft");
         AddEnum("ButtonThumbLeft");
         AddEnum("ButtonThumbRight");
         AddEnum("DpadUp");
         AddEnum("DpadDown");
         AddEnum("DpadLeft");
         AddEnum("DpadRight");
         AddEnum("TriggerLeft");
         AddEnum("TriggerRight");
         AddEnum("StickLeft");
         AddEnum("StickRight");
         AddEnum("Dpad");
      }
   }

   public class TriggerPropRumbleType : EnumeratedProperty
   {
      public TriggerPropRumbleType(INamedTypedProperty prop)
         : base(prop)
      {
         AddEnum("None");
         AddEnum("Fixed");
         AddEnum("SineWave");
         AddEnum("IntervalBurst");
         AddEnum("RandomNoise");
         AddEnum("Incline");
         AddEnum("Decline");
         AddEnum("BumpLRL");
      }
   }

   public class TriggerPropRumbleMotor : EnumeratedProperty
   {
      public TriggerPropRumbleMotor(INamedTypedProperty prop)
         : base(prop)
      {
         AddEnum("Both");
         AddEnum("Left");
         AddEnum("Right");
      }
   }

   public class TriggerPropUnitFlag : EnumeratedProperty
   {
      public TriggerPropUnitFlag(INamedTypedProperty prop, List<string> unitFlags)
         : base(prop)
      {
         foreach (string unitFlag in unitFlags)
         {
            AddEnum(unitFlag);
         }
      }
   }

   public class TriggerPropSquadFlag : EnumeratedProperty
   {
      public TriggerPropSquadFlag(INamedTypedProperty prop, List<string> squadFlags)
         : base(prop)
      {
         foreach (string squadFlag in squadFlags)
         {
            AddEnum(squadFlag);
         }
      }
   }

   public class TriggerPropIterator : HighLevelProperty
   {
      public TriggerPropIterator(INamedTypedProperty prop)
         : base(prop)
      {        
      }
      public override Control GetEditor(out string bindPropName)
      {
         bindPropName = "None";
         Label l = new Label();
         l.Text = "No value editor, use variables instead.";
         return l;
      }
   }


   //TriggerPropDesignLine
   public class TriggerPropDesignLine : TriggerPropEntity
   {
      public TriggerPropDesignLine(INamedTypedProperty prop)
         : base(prop)
      {
         mPrompt = "Pick Line.";
      }
      public override bool FilterEditorObject(EditorObject edobj)
      {
         if (edobj is GameDesignLine)
         {
            return true;
         }
         return false;
      }
   }

   public class PositionHelperObject : TriggerPropEntity
   {
      public PositionHelperObject(INamedTypedProperty prop)
         : base(prop)
      {
         mPrompt = "Pick Object.";
      }
      public override bool FilterEditorObject(EditorObject edobj)
      {
         if (edobj is HelperPositionObject)
         {
            return true;
         }
         return false;
      }
   }

   public class TriggerPropLocStringID : LocStringIDProperty
   {
      public TriggerPropLocStringID(INamedTypedProperty prop)
         : base(prop)
      {
         base.mPickerHeight = 600;
      }

   }

   public class TriggerPropFlareType : EnumeratedProperty
   {
      public TriggerPropFlareType(INamedTypedProperty prop)
         : base(prop)
      {
         // These are defined in uigame.h of the HW code
         AddEnum("Look");
         AddEnum("Help");
         AddEnum("Meet");
         AddEnum("Attack");

         mDefaultValue = "Look";
      }

      static public string FixUp(string oldValue)
      {
         if (oldValue == null)
            return (null);
         if (oldValue == "")
            return ("");

         int intValue;
         if (!int.TryParse(oldValue, out intValue))
            return (oldValue);

         // If we parsed an int, it's the crappy old 'ID as string' way -> convert it to the name of the enum here.
         if (oldValue == "0") return ("Look");
         else if (oldValue == "1") return ("Help");
         else if (oldValue == "2") return ("Meet");
         else if (oldValue == "3") return ("Attack");
         return ("Look");
      }
   }

   public class TriggerPropIconType : EnumeratedProperty
   {
      public TriggerPropIconType(INamedTypedProperty prop, IconData iconData)
         : base(prop)
      {
         int count = 0;
         foreach (string s in iconData.mIconNameList)
         {
            AddEnum(iconData.mIconObjectList[count], s);
            count++;
         }

         mDefaultValue = iconData.mIconNameList[0];
      }
   }

   public class TriggerPropPlacementRule : EnumeratedProperty
   {
      public TriggerPropPlacementRule(INamedTypedProperty prop, PlacementRuleData placementRuleData)
         : base(prop)
      {         
         foreach (string s in placementRuleData.mPlacementRulesFileNames)
         {
            AddEnum(s);
         }

         mDefaultValue = placementRuleData.mPlacementRulesFileNames[0];
      }
   }

   public class TriggerPropUISquadList : TriggerPropEntityList
   {
      public TriggerPropUISquadList(INamedTypedProperty prop)
         : base(prop)
      {
         mDefaultName = "UISquadList";
      }

      public override bool FilterObject(EditorObject edObj)
      {
         SimObject simobj = edObj as SimObject;
         if (simobj != null)
         {
            SimObjectData data = simobj.GetProperties() as SimObjectData;
            if ((data != null) && ((data.IsSquad == true) || (data.IsSquad == false))) //redundant for emphasis
               return (true);
            else
               return (false);
         }
         return (false);
      }
   }

   public class TriggerPropTechDataCommandType: EnumeratedProperty
   {
      public TriggerPropTechDataCommandType(INamedTypedProperty prop, TechData techData)
         : base(prop)
      {
         foreach(string dataCommandType in techData.mTechDataCommandType)
         {
            AddEnum(dataCommandType);
         }
      }
   }

   public class TriggerPropSquadDataType : EnumeratedProperty
   {
      public TriggerPropSquadDataType(INamedTypedProperty prop, TechData techData)
         : base(prop)
      {
         foreach (string dataSubTypeProtoSquad in techData.mTechDataSubTypeProtoSquad)
         {
            AddEnum(dataSubTypeProtoSquad);
         }
      }
   }

   //refactor this to be even bettar
   public abstract class DataEnumeratedProperty : EnumeratedProperty
   {
      public DataEnumeratedProperty(INamedTypedProperty prop)
         : base(prop)
      {
         Enum en = GetEnum();
         string[] values = Enum.GetNames(en.GetType());

         int num = values.Length;
         if (SkipLastEntry())
            num--;

         bool deletePrefix = DeletePrefix();
         for (int i = StartAtEntry(); i < num; i++)
         {
            string s = values[i];
            if (deletePrefix)
            {
               s = s.Substring(1);
            }
            AddEnum(s);
         }
         if(values.Length > 0)
            mDefaultValue = values[0];
      }
      public virtual int StartAtEntry() { return 1; }
      public virtual bool SkipLastEntry() { return true; }
      public virtual bool DeletePrefix() { return true; }
      public abstract Enum GetEnum();

   }

   public class TriggerPropEventType : DataEnumeratedProperty
   {
      public TriggerPropEventType(INamedTypedProperty prop)
         : base(prop)
      {
      }
      public override Enum GetEnum()
      {
         return mThisEnum;
      }
      
      #region pasted from .h file
      enum eSomeEnum
      {
         cEventInvalid = 0,
         cControlTilt,
         cControlZoom,
         cControlRotate,
         cControlPan,
         cControlCircleSelect,
         cControlCircleMultiSelect,
         cControlClearAllSelections,
         cControlModifierAction,
         cControlModifierSpeed,
         cControlResetCameraSettings,
         cControlGotoRally,
         cControlGotoBase,
         cControlGotoScout,
         cControlGotoNode,
         cControlGotoHero,
         cControlGotoAlert,
         cControlGotoSelected,
         cControlGroupSelect,
         cControlGroupGoto,
         cControlGroupAssign,
         cControlGroupAddTo,
         cControlScreenSelectMilitary,
         cControlGlobalSelect,
         cControlDoubleClickSelect,
         cControlFindCrowdMilitary,
         cControlFindCrowdVillager,
         cControlSetRallyPoint,
         cFlare,
         cFlareHelp,
         cFlareMeet,
         cFlareAttack,            
         cMenuShowCommand,
         cMenuCloseCommand,
         cMenuNavCommand,
         cMenuCommandHasFocus0,
         cMenuCommandHasFocus1,
         cMenuCommandHasFocus2,
         cMenuCommandHasFocus3,
         cMenuCommandHasFocus4,
         cMenuCommandHasFocus5,
         cMenuCommandHasFocus6,
         cMenuCommandHasFocus7,
         cMenuCommandHasFocusN,
         cMenuCommandClickmenuN,
         cMenuCommandIsMenuOpen,
         cMenuCommanndIsMenuNotOpen,
         cMenuShowPower,
         cMenuClosePower,
         cMenuPowerHasFocusN,
         cMenuPowerClickmenuN,
         cMenuPowerIsMenuOpen,
         cMenuPowerIsMenuNotOpen,
         cMenuShowSelectPower,
         cMenuShowAbility, 
         cMenuShowTribute, 
         cMenuShowObjectives,
         cGameEntityBuilt,
         cGameEntityKilled,
         cChatShown,
         cChatRemoved,
         cChatCompleted,
         cCommandBowl,
         cCommandAbility,
         cCommandUnpack,
         cCommandDoWork, 
         cCommandAttack,
         cCommandMove,
         cCommandTrainSquad,
         cCommandTrainSquadCancel,
         cCommandResearch,
         cCommandResearchCancel,
         cCommandBuildOther,
         cCommandRecycle,
         cCommandRecycleCancel,
         cCameraLookingAt,
         cSelectUnits,     
         cCinematicCompleted,
         cFadeCompleted,
         cUsedPower,
         cTimer1Sec,
         cControlCircleSelectFullyGrown,
         cPowerOrbitalComplete,
         cGameEntityRammed,
         cGameEntityJacked,
         cGameEntityKilledByNonProjectile,
         cSize
      }
      #endregion
      eSomeEnum mThisEnum;
   }


   public class TriggerPropGameStatePredicate : DataEnumeratedProperty
   {
      public TriggerPropGameStatePredicate(INamedTypedProperty prop)
         : base(prop)
      {
      }
      public override Enum GetEnum()
      {
         return mThisEnum;
      }

      #region pasted from .h file
      enum eSomeEnum
      {
         cEventInvalid = 0,
         cSquadsSelected,
         cHasSquads,
         cHasBuildings,
         cHasResources,
         cHasTech,
         cGameTime,
         cSize
      }
      #endregion
      eSomeEnum mThisEnum;
   }


   public class TriggerPropConcept : EnumeratedProperty
   {
      public TriggerPropConcept(INamedTypedProperty prop, HintConcepts resources)
         : base(prop)
      {

         for (int i = 0; i < resources.mConceptIDs.Count; i++)
         {
            AddEnum(resources.mConceptIDs[i], resources.mConceptNames[i]);
         }
      }
   }
   public class TriggerPropConceptList : EnumeratedCollectionProperty  //HighLevelProperty
   {
      public TriggerPropConceptList(INamedTypedProperty prop, HintConcepts resources)
         : base(prop)
      {
         for (int i = 0; i < resources.mConceptIDs.Count; i++)
         {
            AddEnum(resources.mConceptIDs[i], resources.mConceptNames[i]);
         }
      }
   }

   //experimental
   public class TriggerPropConceptList2 : EnumeratedCollectionProperty  //HighLevelProperty
   {
      public TriggerPropConceptList2(INamedTypedProperty prop)
         : base(prop)
      {
         //HintConcepts resources = TriggerSystemMain.mSimResources.mHintConcepts;

         //for (int i = 0; i < resources.mConceptIDs.Count; i++)
         //{
         //   AddEnum(resources.mConceptIDs[i], resources.mConceptNames[i]);
         //}
         

      }
      public override Control GetEditor(out string bindPropName)
      {



         return base.GetEditor(out bindPropName);
      }

   }
   public class TriggerPropTimeList : HighLevelProperty
   {
      public TriggerPropTimeList(INamedTypedProperty prop)
         : base(prop)
      {
      }
   }

   public class TriggerPropDesignLineList : TriggerPropEntityList
   {
      public TriggerPropDesignLineList(INamedTypedProperty prop)
         : base(prop)
      {
         mDefaultName = "DesignLineList";
      }

      public override bool FilterObject(EditorObject edobj)
      {
         if (edobj is GameDesignLine)
         {
            return (true);
         }
         return (false);
      }
   }

   public class TriggerPropFloatList : HighLevelProperty
   {
      public TriggerPropFloatList(INamedTypedProperty prop)
         : base(prop)
      {
      }
   }

   public class TriggerPropDifficulty : EnumeratedProperty
   {
      public TriggerPropDifficulty(INamedTypedProperty prop)
         : base(prop)
      {
         AddEnum("0", "Easy");
         AddEnum("1", "Normal");
         AddEnum("2", "Heroic");
         AddEnum("3", "Legendary");
      }
   }

   //####NEW TYPE


}


