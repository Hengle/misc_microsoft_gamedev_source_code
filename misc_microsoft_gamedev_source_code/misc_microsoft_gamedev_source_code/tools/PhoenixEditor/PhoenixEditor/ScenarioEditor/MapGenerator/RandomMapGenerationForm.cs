using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Drawing.Imaging;
using System.Data;
using System.Text;
using System.Windows.Forms;
using System.IO;

using Microsoft.DirectX;
using Microsoft.DirectX.Direct3D;

using EditorCore;
using SimEditor;
using RandomMapGenerator;
using Terrain;

using graphapp;

using PhoenixEditor.ScenarioEditor;

using System.Xml;
using System.Xml.Serialization;

namespace PhoenixEditor.ScenarioEditor
{

   public partial class RandomMapGenerationForm : UserControl
   {
      MapPropFrm mMapProperties = new MapPropFrm();
       ApplyMaskPropFrm mMaskProperties = new ApplyMaskPropFrm();
       ApplyObjPropFrm mObjectProperties = new ApplyObjPropFrm();
       ApplyTexturePropFrm mTextureProperties = new ApplyTexturePropFrm();
      ApplyTransPropFrm mTransformProperties = new ApplyTransPropFrm();

       MapTreeNode mRootNode = null;
        public RandomMapGenerationForm()
        {

            InitializeComponent();


            mMaskProperties.Visible = false;
            mObjectProperties.Visible = false;
            mTextureProperties.Visible = false;
            mTransformProperties.Visible = false;
            mMapProperties.Visible = false;

            propertiesPanel.Controls.Add(mMaskProperties);
            propertiesPanel.Controls.Add(mObjectProperties);
            propertiesPanel.Controls.Add(mTextureProperties);
            propertiesPanel.Controls.Add(mTransformProperties);
            propertiesPanel.Controls.Add(mMapProperties);
           
           
        }

       private void RandomMapGenerationForm_Load(object sender, EventArgs e)
       {
          List<string> maskNames = CoreGlobals.getEditorMain().mIMaskPickerUI.GetMaskNames();
          maskListComboBox.Items.AddRange(maskNames.ToArray());
          maskNames.Clear();
          maskNames = null;

          if(maskListComboBox.Items.Count!=0)
            maskListComboBox.SelectedIndex = 0;

         newScript();
       
       }

       void newScript()
       {
          mRootNode = null;
          this.treeView1.Nodes.Clear();
          mRootNode = new MapTreeNode(this.treeView1);
          this.treeView1.Nodes.Add(mRootNode);
       }

       void loadScript()
       {
          OpenFileDialog ofd = new OpenFileDialog();
          ofd.Filter = "Random Map Script(*.rms)|*.rms";
          ofd.InitialDirectory = CoreGlobals.getWorkPaths().mEditorSettings;
          if (ofd.ShowDialog() == DialogResult.OK)
          {
             loadScriptFromDiskInternal(ofd.FileName); 
          }
          
       }
       string savedFileName = "";
       void saveScript()
       {
          if (savedFileName == "")
             saveScriptAs();
          else
             saveScriptToDiskInternal(savedFileName);
       }
       void saveScriptAs()
       {
          SaveFileDialog sfd = new SaveFileDialog();
          sfd.Filter = "Random Map Script(*.rms)|*.rms";
          if (sfd.ShowDialog() == DialogResult.OK)
          {
             saveScriptToDiskInternal(sfd.FileName);
             savedFileName = sfd.FileName;
           
          }
        
       }


      void loadScriptFromDiskInternal(string filename)
      {
         newScript();

         MapTreeNodeXML xmlRoot = EditorCore.Controls.Micro.BaseLoader<MapTreeNodeXML>.Load(filename);
         mRootNode.mGLSFileToUse = xmlRoot.mGLSFileToUse;

         for (int i = 0; i < xmlRoot.mActionNodes.Count; i++)
         {
            ActionTreeNode atn = new ActionTreeNode(treeView1,mRootNode);


            atn.mMaskGenNode.Mask.GraphMemStream.Write(xmlRoot.mActionNodes[i].mMaskGenNode,0,xmlRoot.mActionNodes[i].mMaskGenNode.Length);
            atn.mMaskGenNode.Mask.GraphMemStream.Position = 0;

            atn.Text = xmlRoot.mActionNodes[i].mText;
            for (int k = 0; k < xmlRoot.mActionNodes[i].mObjectNodes.Count; k++)
            {
               ApplyObjectTreeNode objtn = new ApplyObjectTreeNode();

               objtn.mData = xmlRoot.mActionNodes[i].mObjectNodes[k];
               atn.Nodes.Add(objtn);
            }

            for (int k = 0; k < xmlRoot.mActionNodes[i].mTextureNodes.Count; k++)
            {
               ApplyTextureTreeNode objtn = new ApplyTextureTreeNode();

               objtn.mData = xmlRoot.mActionNodes[i].mTextureNodes[k];
               atn.Nodes.Add(objtn);
            }

            for (int k = 0; k < xmlRoot.mActionNodes[i].mTransformNodes.Count; k++)
            {
               ApplyTransformTreeNode objtn = new ApplyTransformTreeNode();

               objtn.mData = xmlRoot.mActionNodes[i].mTransformNodes[i];
               atn.Nodes.Add(objtn);
            }
            

            mRootNode.Nodes.Add(atn);
         }

      }

      void saveScriptToDiskInternal(string filename)
       {
          MapTreeNodeXML xmlRoot = new MapTreeNodeXML();
          xmlRoot.mGLSFileToUse = mRootNode.mGLSFileToUse;
          for (int i = 0; i < mRootNode.Nodes.Count ;i++)
          {
             if (mRootNode.Nodes[i] is ActionTreeNode)
             {
                ActionTreeNode atn = mRootNode.Nodes[i] as ActionTreeNode;
                ActionTreeNodeXML atnx = new ActionTreeNodeXML();

                atnx.mMaskGenNode = atn.mMaskGenNode.Mask.GraphMemStream.ToArray();
                atnx.mText = atn.Text;

                for (int k = 0; k < atn.Nodes.Count;k++ )
                {
                   if (atn.Nodes[k] is ApplyObjectTreeNode)
                   {
                      ApplyObjectTreeNode objtn = atn.Nodes[k] as ApplyObjectTreeNode;
                      atnx.mObjectNodes.Add(objtn.mData);
                   }
                   else if (atn.Nodes[k] is ApplyTextureTreeNode)
                   {
                      ApplyTextureTreeNode objtn = atn.Nodes[k] as ApplyTextureTreeNode;
                      atnx.mTextureNodes.Add(objtn.mData);
                   }
                   else if (atn.Nodes[k] is ApplyTransformTreeNode)
                   {
                      ApplyTransformTreeNode objtn = atn.Nodes[k] as ApplyTransformTreeNode;
                      atnx.mTransformNodes.Add(objtn.mData);
                   }
                   
                   
                   
                }

               xmlRoot.mActionNodes.Add(atnx);
             }
          }
          EditorCore.Controls.Micro.BaseLoader<MapTreeNodeXML>.Save(filename, xmlRoot);
       }

      void cleanMap()
      {
         //clean detail
         Vector3[] detialPts = TerrainGlobals.getEditor().getDetailPoints();
         int width = TerrainGlobals.getTerrain().getNumXVerts();
         for (int x = 0; x < width * width; x++)
         {
               detialPts[x].X = 0 ;
               detialPts[x].Y = 0;
               detialPts[x].Z = 0;  
         }

         //clean textures

         //clear sim 
         SimGlobals.getSimMain().selectAll();
         SimGlobals.getSimMain().DeleteSelected();

      }
      void apply()
      {
         cleanMap();
         mRootNode.apply();

         Masking.clearSelectionMask();
      }

       private void toolStripButton5_Click(object sender, EventArgs e)
       {
          string tokenMaskName = "PlayableArea";

          //Does 'PlayableArea' Exist?
          IMask inputMask = CoreGlobals.getEditorMain().mIMaskPickerUI.GetMask((string)maskListComboBox.SelectedItem);
          if(inputMask == null)
             return;


          CoreGlobals.getEditorMain().mIMaskPickerUI.AddMaskToList(inputMask, tokenMaskName);

          apply();
       }

       private void treeView1_AfterSelect(object sender, TreeViewEventArgs e)
       {
          if (treeView1.SelectedNode == null)
             return;

          TreeNode node = treeView1.SelectedNode;

          mMaskProperties.Visible = false;
          mObjectProperties.Visible = false;
          mTextureProperties.Visible = false;
          mTransformProperties.Visible = false;
          mMapProperties.Visible = false;

          if (node is MapTreeNode)
          {
             mMapProperties.setOwner((MapTreeNode)node);
             mMapProperties.Visible = true;
          }
          else if (node is ActionTreeNode)
          {
             mMaskProperties.setOwner(((ActionTreeNode)node).mMaskGenNode, ((ActionTreeNode)node));
             mMaskProperties.Visible = true;
          }
          else if (node is ApplyObjectTreeNode)
          {
             mObjectProperties.setOwner((ApplyObjectTreeNode)node);
             mObjectProperties.Visible = true;
          }
          else if (node is ApplyTextureTreeNode)
          {
             mTextureProperties.setOwner((ApplyTextureTreeNode)node);
             mTextureProperties.Visible = true;
          }
          else if (node is ApplyTransformTreeNode)
          {
             mTransformProperties.setOwner((ApplyTransformTreeNode)node);
             mTransformProperties.Visible = true;
          }
       }

       private void toolStripButton1_Click(object sender, EventArgs e)
       {
          newScript();
       }

       private void toolStripButton3_Click(object sender, EventArgs e)
       {
          saveScript();
       }

       private void toolStripButton4_Click(object sender, EventArgs e)
       {
          saveScriptAs();
       }

       private void toolStripButton2_Click(object sender, EventArgs e)
       {
          loadScript();
       }

   }

   public class MapTreeNodeXML
   {
      public string mGLSFileToUse = "default.gls";
      public List<ActionTreeNodeXML> mActionNodes = new List<ActionTreeNodeXML>();
   }
   public class ActionTreeNodeXML
   {
      public string mText = "Action";
      public byte[] mMaskGenNode = null;
      public List<ApplyObjectXML> mObjectNodes = new List<ApplyObjectXML>();
      public List<ApplyTextureXML> mTextureNodes = new List<ApplyTextureXML>();
      public List<ApplyTransformXML> mTransformNodes = new List<ApplyTransformXML>();
   }


   public class MapTreeNode : TreeNode
   {
      TreeView mOwnerTree = null;
      public String mGLSFileToUse = "scenario\\defaultGlobalLights.gls";

      public MapTreeNode()
      {
         Name = "MapTreeNode";
         Text = "Map";
      }
      public MapTreeNode(TreeView ownerTree)
      {
         mOwnerTree = ownerTree;

         Name = "MapTreeNode";
         Text = "Map";

         initContextMenu();
      }

      public void apply()
      {
         //apply our lightset
         string glsLightSet = CoreGlobals.getWorkPaths().mGameDirectory + "\\" + mGLSFileToUse;

         if(File.Exists(glsLightSet))
         {
            SimGlobals.getSimMain().LoadScenarioLights(glsLightSet, false);
            CoreGlobals.getSaveLoadPaths().mGameLightsetBaseDirectory = Path.GetDirectoryName(glsLightSet);
         }
         

         //apply children nodes
         for (int i = 0; i < Nodes.Count; i++)
         {
            if (Nodes[i] is ActionTreeNode)
               ((ActionTreeNode)Nodes[i]).apply();
         }
      }

      #region contextMenu

      ToolStripMenuItem addToolStripMenuItem;
      ToolStripMenuItem addActionToolStripMenuItem;

      void initContextMenu()
      {


         // 
         // addActionToolStripMenuItem
         // 
         this.addActionToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.addActionToolStripMenuItem.Name = "addActionToolStripMenuItem";
         this.addActionToolStripMenuItem.Size = new System.Drawing.Size(156, 22);
         this.addActionToolStripMenuItem.Text = "Add Action";
         this.addActionToolStripMenuItem.Click += new EventHandler(addActionToolStripMenuItem_Click);



         // 
         // addToolStripMenuItem
         // 
         this.addToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.addToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.addActionToolStripMenuItem});
         this.addToolStripMenuItem.Name = "addToolStripMenuItem";
         this.addToolStripMenuItem.Size = new System.Drawing.Size(152, 22);
         this.addToolStripMenuItem.Text = "Apply";


         this.ContextMenuStrip = new ContextMenuStrip();
         this.ContextMenuStrip.SuspendLayout();
         this.ContextMenuStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.addToolStripMenuItem});
         this.ContextMenuStrip.ResumeLayout(false);
      }


      private void addActionToolStripMenuItem_Click(object sender, EventArgs e)
      {
         this.Nodes.Add(new ActionTreeNode(mOwnerTree,this));
      }
      #endregion

   }

   
   public class ActionTreeNode : TreeNode
   {
      
      public MaskGenTreeNode mMaskGenNode = new MaskGenTreeNode();

      TreeView mOwnerTree = null;
      TreeNode mOwnerNode = null;
      public ActionTreeNode()
      {
         Name = "ActionTreeNode";
         Text = "Action";
      }
      public ActionTreeNode(TreeView ownerTree,TreeNode ownerNode)
      {
         Name = "ActionTreeNode";
         Text = "Action";

         mOwnerTree = ownerTree;
         mOwnerNode = ownerNode;

         initContextMenu();
      }

      public void setText(string txt)
      {
         Text = "Action : " + txt;
      }

      public void apply()
      {
         mMaskGenNode.apply();

         for(int i=0;i<Nodes.Count;i++)
         {
            if (Nodes[i] is ApplyObjectTreeNode)
               ((ApplyObjectTreeNode)Nodes[i]).apply();
            else if (Nodes[i] is ApplyTextureTreeNode)
               ((ApplyTextureTreeNode)Nodes[i]).apply();
            else if (Nodes[i] is ApplyTransformTreeNode)
               ((ApplyTransformTreeNode)Nodes[i]).apply();
         }
      }

      #region Menu Stuff

      ToolStripMenuItem addToolStripMenuItem;
      ToolStripMenuItem deleteToolStripMenuItem;
      ToolStripMenuItem moveUpToolStripMenuItem;
      ToolStripMenuItem moveDownToolStripMenuItem;
      ToolStripMenuItem addApplyTextureToolStripMenuItem;
      ToolStripMenuItem addApplyObjectToolStripMenuItem;
      ToolStripMenuItem addApplyTransformToolStripMenuItem;

      void initContextMenu()
      {

         // 
         // addApplyTextureToolStripMenuItem
         // 
         this.addApplyTextureToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.addApplyTextureToolStripMenuItem.Name = "addApplyMaskToolStripMenuItem";
         this.addApplyTextureToolStripMenuItem.Size = new System.Drawing.Size(156, 22);
         this.addApplyTextureToolStripMenuItem.Text = "Apply Texture";
         this.addApplyTextureToolStripMenuItem.Click += new EventHandler(addApplyTextureToolStripMenuItem_Click);

         // 
         // addApplyObjectToolStripMenuItem
         // 
         this.addApplyObjectToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.addApplyObjectToolStripMenuItem.Name = "addApplyObjectToolStripMenuItem";
         this.addApplyObjectToolStripMenuItem.Size = new System.Drawing.Size(156, 22);
         this.addApplyObjectToolStripMenuItem.Text = "Apply Object";
         this.addApplyObjectToolStripMenuItem.Click += new EventHandler(addApplyObjectToolStripMenuItem_Click);

         // 
         // addApplyTransformToolStripMenuItem
         // 
         this.addApplyTransformToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.addApplyTransformToolStripMenuItem.Name = "addApplyTransformToolStripMenuItem";
         this.addApplyTransformToolStripMenuItem.Size = new System.Drawing.Size(156, 22);
         this.addApplyTransformToolStripMenuItem.Text = "Apply Transform";
         this.addApplyTransformToolStripMenuItem.Click += new EventHandler(addApplyTransformToolStripMenuItem_Click);


         


         // 
         // addToolStripMenuItem
         // 
         this.addToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.addToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.addApplyObjectToolStripMenuItem,
            this.addApplyTextureToolStripMenuItem,
            this.addApplyTransformToolStripMenuItem
            });
         this.addToolStripMenuItem.Name = "addToolStripMenuItem";
         this.addToolStripMenuItem.Size = new System.Drawing.Size(152, 22);
         this.addToolStripMenuItem.Text = "Apply";

         // 
         // deleteToolStripMenuItem
         // 
         this.deleteToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.deleteToolStripMenuItem.Name = "deleteToolStripMenuItem";
         this.deleteToolStripMenuItem.Size = new System.Drawing.Size(156, 22);
         this.deleteToolStripMenuItem.Text = "Delete";
         this.deleteToolStripMenuItem.Click += new EventHandler(deleteToolStripMenuItem_Click);


         // 
         // moveUpToolStripMenuItem
         // 
         this.moveUpToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.moveUpToolStripMenuItem.Name = "moveUpToolStripMenuItem";
         this.moveUpToolStripMenuItem.Size = new System.Drawing.Size(156, 22);
         this.moveUpToolStripMenuItem.Text = "Move Up";
         this.moveUpToolStripMenuItem.Click += new EventHandler(moveUpToolStripMenuItem_Click);

         // 
         // moveDownToolStripMenuItem
         // 
         this.moveDownToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.moveDownToolStripMenuItem.Name = "moveDownToolStripMenuItem";
         this.moveDownToolStripMenuItem.Size = new System.Drawing.Size(156, 22);
         this.moveDownToolStripMenuItem.Text = "Move Down";
         this.moveDownToolStripMenuItem.Click += new EventHandler(moveDownToolStripMenuItem_Click);

         

         this.ContextMenuStrip = new ContextMenuStrip();
         this.ContextMenuStrip.SuspendLayout();
         this.ContextMenuStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.addToolStripMenuItem,
            this.moveUpToolStripMenuItem,
            this.moveDownToolStripMenuItem,
            this.deleteToolStripMenuItem});
         this.ContextMenuStrip.ResumeLayout(false);
      }


      private void addApplyTextureToolStripMenuItem_Click(object sender, EventArgs e)
      {
         this.Nodes.Add(new ApplyTextureTreeNode());
      }
      private void addApplyObjectToolStripMenuItem_Click(object sender, EventArgs e)
      {
         this.Nodes.Add(new ApplyObjectTreeNode());
      }
      private void addApplyTransformToolStripMenuItem_Click(object sender, EventArgs e)
      {
         this.Nodes.Add(new ApplyTransformTreeNode());
      }

      private void deleteToolStripMenuItem_Click(object sender, EventArgs e)
      {
         if (MessageBox.Show("Are you sure you want to delete this item? This cannot be Undone!!", "Deletion Warning", MessageBoxButtons.OKCancel, MessageBoxIcon.Exclamation) == DialogResult.OK)
            mOwnerTree.Nodes.Remove(this);
      }

      private void moveUpToolStripMenuItem_Click(object sender, EventArgs e)
      {
         int idx = mOwnerNode.Nodes.IndexOf(this);
         if (idx == 0)
            return;

         mOwnerNode.Nodes.Remove(this);
         mOwnerNode.Nodes.Insert(idx - 1, this);
      }
      private void moveDownToolStripMenuItem_Click(object sender, EventArgs e)
      {
         int idx = mOwnerNode.Nodes.IndexOf(this);
         if (idx >= mOwnerNode.Nodes.Count-1)
            return;

         mOwnerNode.Nodes.Remove(this);
         mOwnerNode.Nodes.Insert(idx + 1, this);
      }
      #endregion


   };

   public class MaskGenTreeNode
   {
      GraphBasedMask mMask = new GraphBasedMask();

     public GraphBasedMask Mask
     {
        get
        {
           return mMask;
        }
        set
        {
           mMask = value;
        }
     }
      public MaskGenTreeNode()
      {
       
      }
     ~MaskGenTreeNode()
     {
        mMask.Clear();
        mMask = null;
     }

      public void apply()
      {
         Masking.clearSelectionMask();

         mMask.loadAndExecute();

         long index;
         float value;
         mMask.ResetIterator();
         while (mMask.MoveNext(out index, out value))
         {
            Masking.addSelectedVert(index, value);
         }

         Masking.rebuildVisualsAfterSelection();
      }

   };

   public class ApplyObjectXML
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
   }
   public class ApplyObjectTreeNode : TreeNode
   {
      public ApplyObjectXML mData = new ApplyObjectXML();
      public ApplyObjectTreeNode()
      {
         Name = "ApplyObjectTreeNode";
         Text = "Object";
      }

      public void apply()
      {
         if (mData.Object == null || mData.Object == "")
            return;

         //clear our sim first
      //   SimGlobals.getSimMain().selectAll();
      //   SimGlobals.getSimMain().DeleteSelected();

         SimGlobals.getSimMain().placeItemToMaskSimple(mData.Object, true, mData.NumberToPlace);
      }

   };

   public class ApplyTextureXML
   {

      string mTexture = "";
      public string Texture
      {
         get
         {
            return mTexture;
         }
         set
         {
            mTexture = value;
         }
      }
   }
   public class ApplyTextureTreeNode : TreeNode
   {
      public ApplyTextureXML mData = new ApplyTextureXML();

      public ApplyTextureTreeNode()
      {
         Name = "ApplyTextureTreeNode";
         Text = "Texture";
      }

      public void apply()
      {
         string fName = Path.GetFileNameWithoutExtension(mData.Texture);
         string dir = Path.GetDirectoryName(mData.Texture);
         string themeName = dir.Substring(dir.LastIndexOf("\\") + 1);

         TerrainTextureDef SelectedDef = SimTerrainType.getFromTypeName(themeName + "_" + fName);
         TerrainGlobals.getTexturing().addActiveTexture(SimTerrainType.getWorkingTexName(SelectedDef));
         SimTerrainType.addActiveSetDef(SelectedDef);
         TerrainGlobals.getTerrainFrontEnd().SelectedTextureIndex = SimTerrainType.getActiveSetIndex(SelectedDef);


         TerrainGlobals.getEditor().applyTextureToMask();
      }
   };

   public class ApplyTransformXML
   {

      float yOffset =0;
      public float YOffset
      {
         get
         {
            return yOffset;
         }
         set
         {
            yOffset = value;
         }
      }
   }
   public class ApplyTransformTreeNode : TreeNode
   {

      public ApplyTransformXML mData = new ApplyTransformXML();

      public ApplyTransformTreeNode()
      {
         Name = "ApplyTransformTreeNode";
         Text = "Transform";
      }

      public void apply()
      {

         TerrainGlobals.getEditor().translateVerts2(0, mData.YOffset, 0);
      }

   };
}
