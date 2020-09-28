using System;
using System.Collections.Generic;
using System.Collections;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

using EditorCore;

namespace PhoenixEditor.ScenarioEditor
{
   public partial class BasicTypedSuperList : SuperList
   {
      public BasicTypedSuperList()
      {
         //BaseGuiInitializeComponent();
         InitializeComponent();

         Init();
      }

      IList mObjectList = null;

      [Browsable(false)]
      [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
      public IList ObjectList
      {
         set
         {
            mObjectList = value;
            SetUI();
         }
         get
         {
            return mObjectList;
         }
      }


      private void SetUI()
      {
         BatchSuspend();
         //this.ClearItems();
         ClearControls();
         if (mObjectList == null)
         {            
            return;
         }
         foreach(object o in mObjectList)
         {
            AddPropertyControl(o);
         }
         BatchResume();
      }
      public void UpdateData()
      {
         //ClearControls();

         SetUI();

      }
      bool mbWrapcontents = false;
      public bool WrapContents
      {
         get
         {
            return mbWrapcontents;
         }
         set
         {
            mbWrapcontents = value;
         }
      }
      bool mbUseLabels = true;
      public bool UseLabels
      {
         get
         {
            return mbUseLabels;
         }
         set
         {
            mbUseLabels = value;
         }

      }
      private ObjectEditorControl AddPropertyControl(object o)
      {
         PropertyFlowList flow = new PropertyFlowList();
         flow.WrapContents = mbWrapcontents;
         flow.mbUseLabels = mbUseLabels;
         ApplyMetaData(flow);
         //flow.Tag = this;
         flow.SelectedObject = o;
         flow.SelectedObjectPropertyChanged += new ObjectEditorControl.PropertyChanged(flow_SelectedObjectPropertyChanged);
         flow.AnyPropertyChanged += new ObjectEditorControl.PropertyChanged(flow_AnyObjectPropertyChanged);
         this.AddRow(flow);

         flow.Dock = DockStyle.Fill;

         mListDataObjectType = o.GetType();

         return flow;
      }

      public event PropertyChanged SelectedObjectPropertyChanged;
      public event PropertyChanged AnyObjectPropertyChanged;
      public delegate void PropertyChanged(ObjectEditorControl sender, object selectedObject, HighLevelProperty prop);
      public delegate void ObjectChanged(ObjectEditorControl sender, object selectedObject);

      void flow_SelectedObjectPropertyChanged(ObjectEditorControl sender, object selectedObject, HighLevelProperty prop)
      {
         if(SelectedObjectPropertyChanged != null)
         {
            SelectedObjectPropertyChanged.Invoke(sender, selectedObject, prop);
         }
      }


      void flow_AnyObjectPropertyChanged(ObjectEditorControl sender, object selectedObject, HighLevelProperty prop)
      {
         if (AnyObjectPropertyChanged != null)
         {
            AnyObjectPropertyChanged.Invoke(sender, selectedObject, prop);
         }
      }


      public Type mListDataObjectType = null;

      public void Init()
      {
         List<string> options = new List<string>();
         options.Add("Add");
         SetSimpleMenu(options);        
      }
      public override void OnDelete(SuperListDragButton button)
      {
  
         PropertyFlowList flow = button.Tag as PropertyFlowList;
         object toDelete = flow.SelectedObject;
         mObjectList.Remove(toDelete);
         base.OnDelete(button);

         //ObjectDeleted
         if (ObjectDeleted != null)
            ObjectDeleted.Invoke(flow, toDelete);
      }

      override protected void OnReordered()
      {

         this.mObjectList.Clear();
         foreach(PropertyFlowList f in this.mControls)
         {
            mObjectList.Add(f.SelectedObject);

         }

      }

      override public DragDropEffects ValidateDragTarget(Control toMove, Control target, DragEventArgs e)
      {
         PropertyFlowList ctrlMove = toMove.Tag as PropertyFlowList;
         PropertyFlowList ctrlTarget = target.Tag as PropertyFlowList;

         SuperList targetList = target as SuperList;
         BasicTypedSuperList basidtargetList = target as BasicTypedSuperList;
         
         //to empty list //Also a basic typed super list.   Types must match         
         if (basidtargetList != null && ctrlMove != null)
         {
            if (basidtargetList.mListDataObjectType == ctrlMove.SelectedObject.GetType())
            {
               //if())
               return DragDropEffects.Move;
            }
         }

         //What to do if it is just another super list?
         //if (targetList != null && toMove != null)
         //{
         //   SuperListDragButton button = toMove as SuperListDragButton;
         //   if (targetList.GetType() == button.GetParentList().GetType())
         //   {
         //      return DragDropEffects.Move;
         //   }
         //}


         if (ctrlMove == null || ctrlTarget == null)
         {
            return DragDropEffects.None;
         }

         if ((e.KeyState & 8) == 8 && (e.AllowedEffect & DragDropEffects.Copy) == DragDropEffects.Copy)
         {
            //return DragDropEffects.Copy;
         }
         else if ((e.AllowedEffect & DragDropEffects.Move) == DragDropEffects.Move)
         {
            if (ctrlMove == ctrlTarget)
            {
               //we return move here because it makes the animation look better
               return DragDropEffects.Move;

            }
            if (ctrlMove.SelectedObject.GetType() == ctrlTarget.SelectedObject.GetType())
            {
               return DragDropEffects.Move;
            }
         }

         return DragDropEffects.None;

      }

      override public void HandleDrop(DragEventArgs e, SuperListDragButton otherButton, SuperListDragButton thisButton)
      {
         PropertyFlowList ctrlMove = otherButton.Tag as PropertyFlowList;
         PropertyFlowList ctrlTarget = thisButton.Tag as PropertyFlowList;

         if (e.Effect == DragDropEffects.Move)
         {
            //if (ctrlMove.SelectedObject == ctrlTarget.SelectedObject)
            if (ctrlMove.Parent == ctrlTarget.Parent)
            {
               this.OnReOrderRequest(otherButton, thisButton);
            }
            else if (ctrlMove.SelectedObject.GetType() == ctrlTarget.SelectedObject.GetType())
            {
               SuperList senderParent = otherButton.GetParentList();
               senderParent.OnTransferRequest(this, otherButton);
               //return DragDropEffects.Move;
            }

         }
      }


      List<ListMetaData> mMetadata = new List<ListMetaData>();
      class ListMetaData
      {
         public string a1, a2, a3; public object o1;
      }

      public void IgnoreProperties(string parentTypeName, string[] properties)
      {
         foreach (string s in properties)
         {
            AddMetaDataForProp(parentTypeName, s, "Ignore", true);
         }
      }
      public void AddMetaDataForProps(string parentTypeName, string[] properties, string metadataKey, object metadataValue)
      {
         foreach (string s in properties)
         {
            AddMetaDataForProp(parentTypeName, s, metadataKey, metadataValue);
         }
      }
      public void AddMetaDataForProp(string parentTypeName, string propertyName, string metadataKey, object metadataValue)
      {
         ListMetaData md = new ListMetaData();
         md.a1 = parentTypeName; md.a2 = propertyName; md.a3 = metadataKey; md.o1 = metadataValue;
         mMetadata.Add(md);
      }

      public ObjectEditorControl GetObjectEditor()
      {
         ObjectEditorControl e = new ObjectEditorControl();
         ApplyMetaData(e);
         return e;
      }

      private void ApplyMetaData(ObjectEditorControl propList)
      {
         foreach (ListMetaData md in mMetadata)
         {
            propList.AddMetaDataForProp(md.a1, md.a2, md.a3, md.o1);
         }
         foreach (TypeBind tb in mTypeBind)
         {
            propList.SetTypeEditor(tb.a1, tb.a2, tb.t1);
         }
      }
      List<TypeBind> mTypeBind = new List<TypeBind>();
      class TypeBind
      {
         public string a1, a2; public Type t1;
      }
      public void SetTypeEditor(string parentTypeName, string propertyName, Type toEdit)
      {
         TypeBind tb = new TypeBind();
         tb.a1 = parentTypeName; tb.a2 = propertyName; tb.t1 = toEdit;
         mTypeBind.Add(tb);
      }




      protected override void HandleMenuOption(string text, object tag)
      {
         if (mListDataObjectType != null && mObjectList != null)
         {
            object newObj = mListDataObjectType.GetConstructor(new Type[]{}).Invoke(new object[]{});

            ObjectEditorControl editorCtrl = AddPropertyControl(newObj);

            mObjectList.Add(newObj);

            if (NewObjectAdded != null)
               NewObjectAdded.Invoke(editorCtrl, newObj);
         }
      }

      public event ObjectChanged NewObjectAdded;
      public event ObjectChanged ObjectDeleted;

 

   }



}
