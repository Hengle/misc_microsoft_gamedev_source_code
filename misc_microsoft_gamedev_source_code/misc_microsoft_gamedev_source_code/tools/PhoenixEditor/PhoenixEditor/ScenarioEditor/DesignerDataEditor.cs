using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

using EditorCore;
using SimEditor;

namespace PhoenixEditor.ScenarioEditor
{
   public partial class DesignerDataEditor : UserControl
   {
      public DesignerDataEditor()
      {
         InitializeComponent();
         //basicTypedSuperList1.NewObjectAdded += new BasicTypedSuperList.ObjectChanged(basicTypedSuperList1_NewObjectAdded);
         //basicTypedSuperList1.AutoScroll = true;

         betterPropertyGrid1.AddMetaDataForProp("DesignerData", "Type", "SimpleEnumeration", new string[] { "", "notSet", "Creeps", "AIWaypoint", "CameraBoundary-Hover", "CameraBoundary-Camera" });
         betterPropertyGrid1.SetTypeEditor("DesignerData", "Type", typeof(EnumeratedProperty));

      }

      //void basicTypedSuperList1_NewObjectAdded(ObjectEditorControl sender, object selectedObject)
      //{
      //   //basicTypedSuperList1.UpdateData();//??????????.Refresh();
      //   //throw new Exception("The method or operation is not implemented.");
      //}
      //public System.Collections.IList ObjectList
      //{
      //   set
      //   {
      //      //basicTypedSuperList1.
      //      basicTypedSuperList1.mListDataObjectType = typeof(DataField);
      //      basicTypedSuperList1.ObjectList = value;
      //   }
      //}
      DesignerData mData = new DesignerData();
      public DesignerData Data
      {
         set
         {
            mData = value;
            betterPropertyGrid1.SelectedObject = mData;
            //basicTypedSuperList1.mListDataObjectType = typeof(DataField);
            //basicTypedSuperList1.ObjectList = mData.mData;
         }
      }

      private void InitUI()
      {

      }

   }

   //public class 
   

   public class DesignerDataEditorProperty : HighLevelProperty
   {
      public DesignerDataEditorProperty(INamedTypedProperty prop)
         : base(prop)
      {
      }
      public override Control GetEditor(out string bindPropName)
      {
         bindPropName = "ObjectList";
         DesignerDataEditor ed = new DesignerDataEditor();

         if (PresentationValue != null)
         {
            //ed.ObjectList = (System.Collections.IList)((DesignerData)PresentationValue).mData;

            ed.Data = (DesignerData)PresentationValue;

         }
  
         return ed;
         //return base.GetEditor(out bindPropName);
      }

   }
}
