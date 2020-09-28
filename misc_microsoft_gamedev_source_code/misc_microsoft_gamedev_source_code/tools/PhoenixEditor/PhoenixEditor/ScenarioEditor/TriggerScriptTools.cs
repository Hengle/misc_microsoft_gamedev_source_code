using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;

using SimEditor;
using EditorCore;

namespace PhoenixEditor.ScenarioEditor
{
   public partial class TriggerScriptTools : UserControl
   {
      public TriggerScriptTools()
      {
         InitializeComponent();

         ScriptGroupsTreeView.ImageList = SharedResources.GetImageList(new string[] { "Group.bmp" });

         ScriptGroupsTreeView.NodeMouseClick += new TreeNodeMouseClickEventHandler(ScriptGroupsTreeView_NodeMouseClick);
         //ScriptGroupsTreeView.DragEnter
      }

      void ScriptGroupsTreeView_NodeMouseClick(object sender, TreeNodeMouseClickEventArgs e)
      {
         GroupUI groupui = e.Node.Tag as GroupUI;
         if (groupui != null)
         {
            mHostArea.ShowGroupTab(groupui.InternalGroupID);

         }
         else
         {
            mHostArea.ShowGroupTab(-1);
         }
      }


      TriggerHostArea mHostArea = null;
      TriggerNamespace mCurrentTriggerNamespace = null;
      [Browsable(false)]
      [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
      public TriggerHostArea HostArea
      {
         set
         {
            mHostArea = value;
            mCurrentTriggerNamespace = value.CurrentTriggerNamespace;
            LoadGroupTree();

         }
         get
         {
            return mHostArea;
         }
      }



      private void LoadGroupTree()
      {
         Dictionary<int, GroupUI> groupsById = new Dictionary<int, GroupUI>();
         Dictionary<int, TreeNode> treeNodesById = new Dictionary<int, TreeNode>();
         foreach (GroupUI groupui in mCurrentTriggerNamespace.TriggerEditorData.UIData.mGroups) 
         {
            groupsById[groupui.InternalGroupID] = groupui;
         }

         this.ScriptGroupsTreeView.Nodes.Clear();

         TreeNode root = new TreeNode("Main");
         treeNodesById[-1] = root;

         foreach (GroupUI groupui in mCurrentTriggerNamespace.TriggerEditorData.UIData.mGroups)
         {
            TreeNode node = new TreeNode();
            node.Text = groupui.Title;
            node.Tag = groupui;
            treeNodesById[groupui.InternalGroupID] = node;
         }
         foreach(TreeNode node in treeNodesById.Values)
         {
            GroupUI groupui = node.Tag as GroupUI;
            if(groupui != null)
               treeNodesById[groupui.GroupID].Nodes.Add(node);
         }

         ScriptGroupsTreeView.Nodes.Add(root);
         ScriptGroupsTreeView.ExpandAll();
      }
   }
}
