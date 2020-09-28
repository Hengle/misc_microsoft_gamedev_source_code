using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using System.IO;

using EditorCore;
using EditorCore.Controls.Micro;
//using PhoenixEditor.ScenarioEditor;
using SimEditor;

namespace PhoenixEditor.ScenarioEditor
{
   public partial class TriggerUserClass : UserControl
   {
      public TriggerUserClass()
      {
         InitializeComponent();



         ClassPropsGrid.IgnoreProperties("UserClassDefinition", new string[] { "Fields" });
         ClassPropsGrid.AddMetaDataForProp("UserClassDefinition", "DBID", "ReadOnly", true);

         //ClassFieldsList
         ClassFieldsList.AddMetaDataForProp("UserClassFieldDefinition", "Type", "SimpleEnumeration", TriggerSystemMain.mTriggerDefinitions.GetTypeNames());
         ClassFieldsList.SetTypeEditor("UserClassFieldDefinition", "Type", typeof(EnumeratedProperty));
         //ClassFieldsList.AddMetaDataForProp("UserClassFieldDefinition", "DBID", "ReadOnly", true);
         //ClassFieldsList.AddMetaDataForProp("UserClassFieldDefinition", "Type", "UpdateEvent", true);
         ClassFieldsList.mListDataObjectType = typeof(UserClassFieldDefinition);


         mFileName = Path.Combine(CoreGlobals.getWorkPaths().mGameDataDirectory, "userclasses.xml");

         Load();
      }
      string mFileName = "";
      public UserClassDefinitions mUserClassDefinitions;

      //public void SetUserClasses(List<UserClassDefinition> userClasses)
      //{
      //   mUserClassDefinitions = userClasses;// new List<UserClassDefinition>();         
      //   LoadUI();
      //}
      private void Load()
      {
         if (File.Exists(mFileName))
         {
            mUserClassDefinitions = BaseLoader<UserClassDefinitions>.Load(mFileName);
            TriggerSystemMain.mTriggerDefinitions.mUserClassDefinitions = mUserClassDefinitions;
            LoadUI();
         }
         else
         {
            mUserClassDefinitions = new UserClassDefinitions();
         }
      }

      private void SaveButton_Click(object sender, EventArgs e)
      {
         BaseLoader<UserClassDefinitions>.Save(mFileName, mUserClassDefinitions);

         XMBProcessor.CreateXMB(mFileName, false);
      }

      public void LoadUI()
      {
         this.UserClassList.Nodes.Clear();
         foreach (UserClassDefinition d in mUserClassDefinitions.mUserClasses)
         {
            TreeNode n = new TreeNode();
            n.Text = d.Name;
            n.Tag = d;
            UserClassList.Nodes.Add(n);

         }
      }

      private void NewClassButton_Click(object sender, EventArgs e)
      {
         UserClassDefinition newUserClass = new UserClassDefinition();
         newUserClass.Name = "newclass";

         newUserClass.DBID = mUserClassDefinitions.mUserClasses.Count;  //replace with dbid

         SetCurrentClass(newUserClass);
         mUserClassDefinitions.mUserClasses.Add(newUserClass);

         LoadUI();
      }

      void SetCurrentClass(UserClassDefinition userclass)
      {
         this.ClassPropsGrid.SelectedObject = userclass;

         this.ClassFieldsList.ObjectList = userclass.Fields;

      }

      private void UserClassList_NodeMouseClick(object sender, TreeNodeMouseClickEventArgs e)
      {
         UserClassDefinition def = e.Node.Tag as UserClassDefinition;
         if (def != null)
         {
            SetCurrentClass(def);
         }
      }



   }
}
