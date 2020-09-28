using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using System.IO;

using EditorCore;
using Rendering;
using System.Xml;
using System.Xml.Serialization;

using Xceed.SmartUI.Controls.TreeView;
using Xceed.SmartUI.Controls.ToolBar;
using Xceed.Editors;



//-- Particle Editor Pages
namespace ParticleSystem
{
   
   public partial class ParticleEditor : EditorCore.BaseClientPage
   {
      static int gNewParticleCount = 0;
      private static ParticleEmitter s_EmitterCopy = null;  // Emitter Copy when user hits CTRL-C
      private static MagnetData      s_MagnetCopy = null;  // Emitter Copy when user hits CTRL-C
      private static bool s_bParticleQuickViewClicked = false;

      private string mArtDir = "art\\";
      private ParticleEffect mEffect;
      private EmitterPage emitterPage = new EmitterPage();
      private EmitterShape shapePage  = new EmitterShape();
      private TexturePage texturePage = new TexturePage();
      private ScalePage scalePage = new ScalePage();
      private ColorPage colorPage = new ColorPage();
      private OpacityPage opacityPage = new OpacityPage();
      private SpeedPage speedPage = new SpeedPage();
      private IntensityPage intensityPage = new IntensityPage();
      private MagnetPage magnetPage = new MagnetPage();
      private SplashPage splashPage = new SplashPage();
      
      //-- Dynamic Context Menu
      private ContextMenuStrip  mTreeViewContextMenuStrip = null;

      private ToolStripMenuItem mMenuItemAddEmitter       = null;
      private ToolStripMenuItem mMenuItemDeleteEmitter    = null;
      private ToolStripMenuItem mMenuItemEditEmitterName  = null;

      private ToolStripMenuItem mMenuItemAddMagnet = null;
      private ToolStripMenuItem mMenuItemDeleteMagnet = null;
      private ToolStripMenuItem mMenuItemEditMagnetName = null;

      //-- Cut / Copy / Paste
      private ToolStripMenuItem mMenuItemCutEmitter   = null;
      private ToolStripMenuItem mMenuItemCopyEmitter  = null;
      private ToolStripMenuItem mMenuItemPasteEmitter = null;

      private ToolStripMenuItem mMenuItemCutMagnet = null;
      private ToolStripMenuItem mMenuItemCopyMagnet = null;
      private ToolStripMenuItem mMenuItemPasteMagnet = null;
      
      enum ParticleEditorPageEnum
      {
         eUnknown = -4,
         eMagnet = -3,
         eParticleEffect = -2,
         eEmitter = -1,
         eGeneral = 0,
         eShape,
         eTextures,
         eScale,
         eColor,
         eOpacity,
         eSpeed,
         eIntensity,
         eEmitterPropertyTotal,         
         eTotal = eEmitterPropertyTotal
      };

      public ParticleEditor()
      {
         InitializeComponent();
         initDynamicContextMenu();


         mDynamicMenus.BuildDynamicMenus(null, menuStrip1);

         mEffect = new ParticleEffect();
         gNewParticleCount++;
         mEffect.Name = String.Format("Untitled_{0}.pfx", gNewParticleCount);         

         //-- add one emitter by default         
         addEmitter();

         emitterPage.Tag = ParticleEditorPageEnum.eGeneral;
         shapePage.Tag = ParticleEditorPageEnum.eShape;
         texturePage.Tag = ParticleEditorPageEnum.eTextures;
         scalePage.Tag = ParticleEditorPageEnum.eScale;
         colorPage.Tag = ParticleEditorPageEnum.eColor;
         opacityPage.Tag = ParticleEditorPageEnum.eOpacity;
         speedPage.Tag = ParticleEditorPageEnum.eSpeed;
         intensityPage.Tag = ParticleEditorPageEnum.eIntensity;
         splashPage.Tag = ParticleEditorPageEnum.eUnknown;
         magnetPage.Tag = ParticleEditorPageEnum.eMagnet;

         panel1.Controls.Clear();
         panel1.Controls.Add(emitterPage);          
         panel1.Controls.Add(shapePage);
         panel1.Controls.Add(texturePage);
         panel1.Controls.Add(scalePage);
         panel1.Controls.Add(colorPage);
         panel1.Controls.Add(opacityPage);
         panel1.Controls.Add(speedPage);
         panel1.Controls.Add(intensityPage);
         panel1.Controls.Add(splashPage);
         panel1.Controls.Add(magnetPage);
         
         for (int i = 0; i < panel1.Controls.Count; i++)
         {
            panel1.Controls[i].Dock = DockStyle.Fill;
         }

         hidePanelControls();

         bindData(0);

         
         refreshTreeView(true);

         splashPage.Show();
      }

      private void initDynamicContextMenu()
      {
         //System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(ParticleEditor));

         mTreeViewContextMenuStrip = new ContextMenuStrip();
         mTreeViewContextMenuStrip.Opening += new CancelEventHandler(TreeViewContextMenuStrip_Opening);

         mMenuItemAddEmitter = new ToolStripMenuItem();
         mMenuItemAddEmitter.Text = "Add Emitter";
         mMenuItemAddEmitter.Click += new EventHandler(MenuItemAddEmitter_Click);
         mMenuItemAddEmitter.Image = imageList2.Images[0];// ((System.Drawing.Image)(resources.GetObject("cutToolStripMenuItem.Image")));

         mMenuItemDeleteEmitter = new ToolStripMenuItem();
         mMenuItemDeleteEmitter.Text = "Delete Emitter";
         mMenuItemDeleteEmitter.Click += new EventHandler(MenuItemDeleteEmitter_Click);
         mMenuItemDeleteEmitter.Image = imageList2.Images[1];// ((System.Drawing.Image)(resources.GetObject("cutToolStripMenuItem.Image")));

         mMenuItemEditEmitterName = new ToolStripMenuItem();
         mMenuItemEditEmitterName.Text = "Edit Emitter Name";
         mMenuItemEditEmitterName.Click += new EventHandler(MenuItemEditEmitterName_Click);

         //-- Cut / Copy / Paste
         mMenuItemCutEmitter = new ToolStripMenuItem();
         mMenuItemCutEmitter.Text = "Cut";
         mMenuItemCutEmitter.Click += new EventHandler(MenuItemCutEmitter_Click);
         mMenuItemCutEmitter.Image = imageList2.Images[2];// ((System.Drawing.Image)(resources.GetObject("cutToolStripMenuItem.Image")));

         mMenuItemCopyEmitter = new ToolStripMenuItem();
         mMenuItemCopyEmitter.Text = "Copy";
         mMenuItemCopyEmitter.Click += new EventHandler(MenuItemCopyEmitter_Click);
         mMenuItemCopyEmitter.Image = imageList2.Images[3];// ((System.Drawing.Image)(resources.GetObject("copyToolStripMenuItem.Image")));

         mMenuItemPasteEmitter = new ToolStripMenuItem();
         mMenuItemPasteEmitter.Text = "Paste";
         mMenuItemPasteEmitter.Click += new EventHandler(MenuItemPasteEmitter_Click);
         mMenuItemPasteEmitter.Image = imageList2.Images[4];// ((System.Drawing.Image)(resources.GetObject("pasteToolStripMenuItem.Image")));


         // Magnets
         mMenuItemAddMagnet = new ToolStripMenuItem();
         mMenuItemAddMagnet.Text = "Add Magnet";
         mMenuItemAddMagnet.Click += new EventHandler(MenuItemAddMagnet_Click);
         mMenuItemAddMagnet.Image = imageList2.Images[0];// ((System.Drawing.Image)(resources.GetObject("cutToolStripMenuItem.Image")));

         mMenuItemDeleteMagnet = new ToolStripMenuItem();
         mMenuItemDeleteMagnet.Text = "Delete Magnet";
         mMenuItemDeleteMagnet.Click += new EventHandler(MenuItemDeleteMagnet_Click);
         mMenuItemDeleteMagnet.Image = imageList2.Images[1];// ((System.Drawing.Image)(resources.GetObject("cutToolStripMenuItem.Image")));

         mMenuItemEditMagnetName = new ToolStripMenuItem();
         mMenuItemEditMagnetName.Text = "Edit Magnet Name";
         mMenuItemEditMagnetName.Click += new EventHandler(MenuItemEditMagnetName_Click);

         //-- Cut / Copy / Paste
         mMenuItemCutMagnet = new ToolStripMenuItem();
         mMenuItemCutMagnet.Text = "Cut";
         mMenuItemCutMagnet.Click += new EventHandler(MenuItemCutMagnet_Click);
         mMenuItemCutMagnet.Image = imageList2.Images[2];// ((System.Drawing.Image)(resources.GetObject("cutToolStripMenuItem.Image")));

         mMenuItemCopyMagnet = new ToolStripMenuItem();
         mMenuItemCopyMagnet.Text = "Copy";
         mMenuItemCopyMagnet.Click += new EventHandler(MenuItemCopyMagnet_Click);
         mMenuItemCopyMagnet.Image = imageList2.Images[3];// ((System.Drawing.Image)(resources.GetObject("copyToolStripMenuItem.Image")));

         mMenuItemPasteMagnet = new ToolStripMenuItem();
         mMenuItemPasteMagnet.Text = "Paste";
         mMenuItemPasteMagnet.Click += new EventHandler(MenuItemPasteMagnet_Click);
         mMenuItemPasteMagnet.Image = imageList2.Images[4];// ((System.Drawing.Image)(resources.GetObject("pasteToolStripMenuItem.Image")));
      }

      void TreeViewContextMenuStrip_Opening(object sender, System.ComponentModel.CancelEventArgs e)
      {
         mTreeViewContextMenuStrip.Items.Clear();

         TreeNode node = treeView1.SelectedNode;
         if (node == null)
            return;

         ParticleEditorPageEnum id = (ParticleEditorPageEnum)node.Tag;
         if (id == ParticleEditorPageEnum.eParticleEffect || id == ParticleEditorPageEnum.eEmitter)
         {
            mTreeViewContextMenuStrip.Items.Add(mMenuItemAddEmitter);
         }

         if (id == ParticleEditorPageEnum.eEmitter)
         {
            mTreeViewContextMenuStrip.Items.Add(mMenuItemDeleteEmitter);
            mTreeViewContextMenuStrip.Items.Add(mMenuItemEditEmitterName);

            mTreeViewContextMenuStrip.Items.Add(mMenuItemCutEmitter);
            mTreeViewContextMenuStrip.Items.Add(mMenuItemCopyEmitter);
            mTreeViewContextMenuStrip.Items.Add(mMenuItemPasteEmitter);

            mTreeViewContextMenuStrip.Items.Add(mMenuItemAddMagnet);
         }

         if (id == ParticleEditorPageEnum.eMagnet)
         {            
            mTreeViewContextMenuStrip.Items.Add(mMenuItemDeleteMagnet);
            mTreeViewContextMenuStrip.Items.Add(mMenuItemEditMagnetName);

            mTreeViewContextMenuStrip.Items.Add(mMenuItemCutMagnet);
            mTreeViewContextMenuStrip.Items.Add(mMenuItemCopyMagnet);
            mTreeViewContextMenuStrip.Items.Add(mMenuItemPasteMagnet);
         }

         e.Cancel = false;
      }

      void MenuItemAddEmitter_Click(object sender, EventArgs e)
      {
         addEmitter();
         refreshTreeView(false);
      }

      void MenuItemDeleteEmitter_Click(object sender, EventArgs e)
      {
         TreeNode node = treeView1.SelectedNode;
         if (node == null)
            return;

         if ((ParticleEditorPageEnum)node.Tag != ParticleEditorPageEnum.eEmitter)
            return;

         mEffect.deleteEmitter(node.Index);
         refreshTreeView(false);
      }

      void MenuItemEditEmitterName_Click(object sender, EventArgs e)
      {
         TreeNode node = treeView1.SelectedNode;
         if (node == null)
            return;

         if ((ParticleEditorPageEnum)node.Tag != ParticleEditorPageEnum.eEmitter)
            return;

         int index = node.Index;
         if (index < 0 || index >= mEffect.Emitters.Count)
            return;

         NameEdit nameEditControl = new NameEdit();
         nameEditControl.NameText = mEffect.Emitters[index].Name;
         DialogResult result;
         result = nameEditControl.ShowDialog();
         if (result == DialogResult.OK)
         {
            mEffect.Emitters[index].Name = nameEditControl.NameText;
            node.Text = nameEditControl.NameText;            
         }
      }

      void MenuItemCutEmitter_Click(object sender, EventArgs e)
      {
         TreeNode node = treeView1.SelectedNode;
         if (node == null)
            return;
        
         if (mEffect.Emitters.Count == 1)
            return;

         CopySelectedEmitter();
         DeleteSelectedEmitter();
         refreshTreeView(false);
      }

      void MenuItemCopyEmitter_Click(object sender, EventArgs e)
      {
         TreeNode node = treeView1.SelectedNode;
         if (node == null)
            return;

         CopySelectedEmitter();
      }

      void MenuItemPasteEmitter_Click(object sender, EventArgs e)
      {
         TreeNode node = treeView1.SelectedNode;
         if (node == null)
            return;

         PasteOnSelectedEmitter();
         refreshTreeView(false);
      }

      void DeleteSelectedEmitter()
      {
         TreeNode node = treeView1.SelectedNode;
         if (node == null)
            return;

         if ((ParticleEditorPageEnum)node.Tag != ParticleEditorPageEnum.eEmitter)
            return;

         mEffect.deleteEmitter(node.Index);         
      }

      void CopySelectedEmitter()
      {
         TreeNode node = treeView1.SelectedNode;
         if (node == null)
            return;

         if ((ParticleEditorPageEnum)node.Tag != ParticleEditorPageEnum.eEmitter)
            return;

         int index = node.Index;
         if (index < 0 || index >= mEffect.Emitters.Count)
            return;                  

         // clear copy
         s_EmitterCopy = null;
         s_EmitterCopy = mEffect.Emitters[index].clone();
      }

      void PasteOnSelectedEmitter()
      {
         TreeNode node = treeView1.SelectedNode;
         if (node == null)
            return;

         if ((ParticleEditorPageEnum)node.Tag != ParticleEditorPageEnum.eEmitter)
            return;

         int index = node.Index;
         if (index < 0 || index >= mEffect.Emitters.Count)
            return;                  

         ParticleEmitter copiedNode = s_EmitterCopy.clone();
         //copiedNode.Name += "_Copy";
         mEffect.addEmitterAt(index, copiedNode);
      }

      //----------------
      // Magnets 
      //----------------
      void MenuItemAddMagnet_Click(object sender, EventArgs e)
      {
         TreeNode node = treeView1.SelectedNode;
         if (node == null)
            return;

         if ((ParticleEditorPageEnum)node.Tag != ParticleEditorPageEnum.eEmitter)
            return;

         int index = node.Index;
         if (index < 0 || index >= mEffect.Emitters.Count)
            return;
         addMagnet(index);

         refreshTreeView(false);
      }

      void MenuItemDeleteMagnet_Click(object sender, EventArgs e)
      {
         TreeNode node = treeView1.SelectedNode;
         if (node == null)
            return;

         if ((ParticleEditorPageEnum)node.Tag != ParticleEditorPageEnum.eMagnet)
            return;

         int emitterIndex = node.Parent.Index;
         if ((ParticleEditorPageEnum)node.Parent.Tag != ParticleEditorPageEnum.eEmitter)
            return;

         if (emitterIndex < 0 || emitterIndex >= mEffect.Emitters.Count)
            return;

         int magnetIndex = node.Index - (int) ParticleEditorPageEnum.eEmitterPropertyTotal;
         mEffect.Emitters[emitterIndex].deleteMagnet(magnetIndex);
         refreshTreeView(false);
      }

      void MenuItemEditMagnetName_Click(object sender, EventArgs e)
      {
         TreeNode node = treeView1.SelectedNode;
         if (node == null)
            return;

         if ((ParticleEditorPageEnum)node.Tag != ParticleEditorPageEnum.eMagnet)
            return;

         int emitterIndex = node.Parent.Index;
         if ((ParticleEditorPageEnum)node.Parent.Tag != ParticleEditorPageEnum.eEmitter)
            return;

         if (emitterIndex < 0 || emitterIndex >= mEffect.Emitters.Count)
            return;

         int magnetIndex = node.Index - (int)ParticleEditorPageEnum.eEmitterPropertyTotal;

         NameEdit nameEditControl = new NameEdit();
         nameEditControl.NameText = mEffect.Emitters[emitterIndex].Magnets[magnetIndex].Name;
         DialogResult result;
         result = nameEditControl.ShowDialog();
         if (result == DialogResult.OK)
         {
            mEffect.Emitters[emitterIndex].Magnets[magnetIndex].Name = nameEditControl.NameText;
            node.Text = nameEditControl.NameText;
         }
      }

      void MenuItemCutMagnet_Click(object sender, EventArgs e)
      {
         TreeNode node = treeView1.SelectedNode;
         if (node == null)
            return;

         if ((ParticleEditorPageEnum)node.Tag != ParticleEditorPageEnum.eMagnet)
            return;

         int emitterIndex = node.Parent.Index;
         if ((ParticleEditorPageEnum)node.Parent.Tag != ParticleEditorPageEnum.eEmitter)
            return;

         if (mEffect.Emitters[emitterIndex].Magnets.Count == 0)
            return;

         CopySelectedMagnet();
         DeleteSelectedMagnet();
         refreshTreeView(false);
      }

      void MenuItemCopyMagnet_Click(object sender, EventArgs e)
      {
         TreeNode node = treeView1.SelectedNode;
         if (node == null)
            return;

         if ((ParticleEditorPageEnum)node.Tag != ParticleEditorPageEnum.eMagnet)
            return;

         CopySelectedMagnet();
      }

      void MenuItemPasteMagnet_Click(object sender, EventArgs e)
      {
         TreeNode node = treeView1.SelectedNode;
         if (node == null)
            return;

         PasteOnSelectedMagnet();
         refreshTreeView(false);
      }

      void DeleteSelectedMagnet()
      {
         TreeNode node = treeView1.SelectedNode;
         if (node == null)
            return;

         if ((ParticleEditorPageEnum)node.Tag != ParticleEditorPageEnum.eMagnet)
            return;

         int emitterIndex = node.Parent.Index;
         if ((ParticleEditorPageEnum)node.Parent.Tag != ParticleEditorPageEnum.eEmitter)
            return;

         int magnetIndex = node.Index - (int)ParticleEditorPageEnum.eEmitterPropertyTotal;

         mEffect.Emitters[emitterIndex].deleteMagnet(magnetIndex);
      }

      void CopySelectedMagnet()
      {
         TreeNode node = treeView1.SelectedNode;
         if (node == null)
            return;

         if ((ParticleEditorPageEnum)node.Tag != ParticleEditorPageEnum.eMagnet)
            return;
         
         if ((ParticleEditorPageEnum)node.Parent.Tag != ParticleEditorPageEnum.eEmitter)
            return;

         int emitterIndex = node.Parent.Index;
         if (emitterIndex < 0 || emitterIndex >= mEffect.Emitters.Count)
            return;

         int magnetIndex = node.Index - (int)ParticleEditorPageEnum.eEmitterPropertyTotal;
         if (magnetIndex < 0 || magnetIndex >= mEffect.Emitters[emitterIndex].Magnets.Count)
            return;

         // clear copy
         s_MagnetCopy = null;
         s_MagnetCopy = mEffect.Emitters[emitterIndex].Magnets[magnetIndex].clone();
      }

      void PasteOnSelectedMagnet()
      {
         TreeNode node = treeView1.SelectedNode;
         if (node == null)
            return;

         if ((ParticleEditorPageEnum)node.Tag != ParticleEditorPageEnum.eMagnet)
            return;

         if ((ParticleEditorPageEnum)node.Parent.Tag != ParticleEditorPageEnum.eEmitter)
            return;

         int emitterIndex = node.Parent.Index;
         if (emitterIndex < 0 || emitterIndex >= mEffect.Emitters.Count)
            return;

         int magnetIndex = node.Index - (int)ParticleEditorPageEnum.eEmitterPropertyTotal;
         if (magnetIndex < 0 || magnetIndex >= mEffect.Emitters[emitterIndex].Magnets.Count)
            return;

         MagnetData copiedNode = s_MagnetCopy.clone();
         //copiedNode.Name += "_Copy";
         mEffect.Emitters[emitterIndex].addMagnetAt(magnetIndex, copiedNode);
      }

      public void bindData(int index)
      {
         if (index < 0 || index >= mEffect.Emitters.Count)
            return;

         //-- bind the data
         emitterPage.setData(mEffect.Emitters[index]);
         shapePage.setData(mEffect.Emitters[index]);
         texturePage.setData(mEffect.Emitters[index]);
         scalePage.setData(mEffect.Emitters[index]);
         colorPage.setData(mEffect.Emitters[index]);
         opacityPage.setData(mEffect.Emitters[index]);
         speedPage.setData(mEffect.Emitters[index]);
         intensityPage.setData(mEffect.Emitters[index]);         
      }

      public void bindMagnetData(int emitterIndex, int magnetIndex)
      {
         if (emitterIndex < 0 || emitterIndex >= mEffect.Emitters.Count)
            return;

         if (magnetIndex < 0 || magnetIndex >= mEffect.Emitters[emitterIndex].Magnets.Count)
            return;

         magnetPage.setData(mEffect.Emitters[emitterIndex].Magnets[magnetIndex]);
      }

      //CLM [04.26.06] this function called when a tab becomes 'active'
      public override void Activate()
      {
         base.Activate();
         //this call passes in the panel width and height
         //MainWindow.mMainWindow.deviceResize(d3dRenderPanel.Width, d3dRenderPanel.Height, false);

         //this call passes in the panel handle in which you want to render to.
         //this is handled during the 'present' call
         //BRenderDevice.setWindowTarget(d3dRenderPanel);

      }
      //CLM [04.26.06] this function called when a tab becomes 'deactive'
      public override void Deactivate()
      {

      }

      //CLM [04.26.06] called when this page is closed from the main tab
      public override void destroyPage()
      {
         base.destroyPage();
         deinitDeviceData();
         deinit();
      }

      //these two functions are called when the tab is created, and deinitalized respectivly
      public override void init()
      {
         base.init();
      }

      public override void deinit()
      {
         base.deinit();
         
      }

      //CLM [04.26.06] these functions called for all data that's not in the MANAGED pool for d3d.
      //on a device resize, or reset, these functions are called for you.
      public override void initDeviceData()
      {
         base.initDeviceData();
         
      }
      override public void deinitDeviceData()
      {
         base.deinitDeviceData();
         
      }

     
      //override these functions to ensure your app gets the proper processing.
      override public void input()
      {
         base.input();
        
      }
      override public void update()
      {
         base.update();
        
      }
      override public void render()
      {
         base.render();

         BRenderDevice.clear(true, true, unchecked((int)0x0000FF00), 1.0f, 0);

         BRenderDevice.beginScene();
         BRenderDevice.endScene();
         BRenderDevice.present();
      }

      public void SetTabName(string filename)
      {
         Parent.Text = Path.GetFileNameWithoutExtension(filename);
      }
      private void SaveAsButton_Click(object sender, EventArgs e)
      {
         SaveParticleSystem();
      }
      public bool SaveParticleSystem()
      {
         SaveFileDialog d = new SaveFileDialog();
         d.InitialDirectory = EditorCore.CoreGlobals.getSaveLoadPaths().mParticleEffectLastSavePath;
         d.FileName = String.Format("{0}.pfx", Parent.Text);
         d.Filter = "Particle Effect (*.pfx)|*.pfx";
         if (d.ShowDialog() == DialogResult.OK)
         {
            EditorCore.CoreGlobals.getSaveLoadPaths().mParticleEffectLastSavePath = Path.GetDirectoryName(d.FileName);
            if (SaveParticleSystem(d.FileName))
            {
               //SetTabName( d.FileName);
               return true;
            }
         }
         return false;
      }
      public bool LoadParticleSystem()
      {
         OpenFileDialog d = new OpenFileDialog();
         d.InitialDirectory = EditorCore.CoreGlobals.getSaveLoadPaths().mParticleEffectLastLoadPath;
         d.Filter = "Particle Effect (*.pfx)|*.pfx";
         if (d.ShowDialog() == DialogResult.OK)
         {
            EditorCore.CoreGlobals.getSaveLoadPaths().mParticleEffectLastLoadPath = Path.GetDirectoryName(d.FileName);
            if (LoadParticleSystem(d.FileName))
            {
               //SetTabName( d.FileName);
               return true;
            }
         }
         return false;
      }

      public bool NewParticleSystem()
      {
         SetTabName(mEffect.Name);
         return true;
      }
      public bool SaveParticleSystem(string filename)
      {
         //-- if the file exists make sure we have it checked out
         if (File.Exists(filename) && ((File.GetAttributes(filename) & FileAttributes.ReadOnly) == FileAttributes.ReadOnly))
         {
            string str = String.Format("File: {0} is read only!\nYou are not allowed to save!\nMake sure you checked the file out of Perforce!", filename);
            MessageBox.Show(str, "Read Only File", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
            return false;
         }

         mEffect.preSerialization();

         if (!Path.HasExtension(filename))
         {
            filename = String.Format("{0}.pfx", filename);
         }

         ResourcePathInfo pathInfo = new ResourcePathInfo(filename);
         mEffect.Name = pathInfo.RelativePath;

         //- strip out the art directory.   ResourcePathInfo should do this for you
         if (mArtDir.Length > 0)
            mEffect.Name = mEffect.Name.Substring(mArtDir.Length);         
                  
         XmlSerializer s = new XmlSerializer(typeof(ParticleEffect), new Type[] { });
         Stream st = File.Create(filename);
         s.Serialize(st, mEffect);
         st.Close();
         mEffect.postDeserialization();

         XMBProcessor.CreateXMB(filename, false);


         SetTabName(mEffect.Name);
         return true;
      }


      public bool WriteParticleSystem(string filename)
      {
         //-- if the file exists make sure we have it checked out
         if (File.Exists(filename) && ((File.GetAttributes(filename) & FileAttributes.ReadOnly) == FileAttributes.ReadOnly))
         {
            string str = String.Format("File: {0} is read only!\nYou are not allowed to save!\nMake sure you checked the file out of Perforce!", filename);
            MessageBox.Show(str, "Read Only File", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
            return false;
         }

         mEffect.preSerialization();

         if (!Path.HasExtension(filename))
         {
            filename = String.Format("{0}.pfx", filename);
         }

         ResourcePathInfo pathInfo = new ResourcePathInfo(filename);

         XmlSerializer s = new XmlSerializer(typeof(ParticleEffect), new Type[] { });
         Stream st = File.Create(filename);
         s.Serialize(st, mEffect);
         st.Close();
         mEffect.postDeserialization();

         XMBProcessor.CreateXMB(filename, false);

         return true;
      }

      public bool LoadParticleSystem(string filename)
      {
         /*
         // make sure we have it checked out
         if ((File.GetAttributes(filename) & FileAttributes.ReadOnly) == FileAttributes.ReadOnly)
         {
            string str = String.Format("File : {0} is read only!\nYou are not allowed to load!\nMake sure you checked the file out of Perforce!", filename);
            MessageBox.Show(str, "Read Only File", MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
            return false;
         }
         */

         SetTabName(filename);         
         XmlSerializer s = new XmlSerializer(typeof(ParticleEffect), new Type[] {});
         Stream st = File.OpenRead(filename);
         mEffect = (ParticleEffect) s.Deserialize(st);
         st.Close();
         mEffect.postDeserialization();
         ResourcePathInfo pathInfo = new ResourcePathInfo(filename);
         mEffect.Name = pathInfo.RelativePath;

         //- strip out the art directory.   ResourcePathInfo should do this for you
         if (mArtDir.Length > 0)
            mEffect.Name = mEffect.Name.Substring(mArtDir.Length);

         bindData(0);
         refreshTreeView(true);
         return true;
      }

      override public void open_file(string absoluteFilename)
      {
         LoadParticleSystem(absoluteFilename);
      }


      private void addToolStripMenuItem_Click(object sender, EventArgs e)
      {

      }

      //-- Add Emitter
      private void emitterToolStripMenuItem_Click(object sender, EventArgs e)
      {
         addEmitter();
         refreshTreeView(false);
      }

      private void button1_Click(object sender, EventArgs e)
      {
         LoadParticleSystem();
      }

      // Add Emitter
      public void addEmitter()
      {
         int num = mEffect.Emitters.Count;
         String name = String.Format("Emitter{0}", num);
         mEffect.addEmitter(name);


      }

      // Delete Emitter
      public void deleteEmitter(int index)
      {
         if (index < 0 || index >= mEffect.Emitters.Count)
            return;

         mEffect.Emitters.RemoveAt(index);
      }

      public void addMagnet(int emitterIndex)
      {
         if (emitterIndex < 0 || emitterIndex >= mEffect.Emitters.Count)
            return;

         int num = mEffect.Emitters[emitterIndex].Magnets.Count;
         String name = String.Format("Magnet{0}", num);
         mEffect.Emitters[emitterIndex].addMagnet(name);
      }

      // Delete Emitter
      public void deleteMagnet(int emitterIndex, int magnetIndex)
      {
         if (emitterIndex < 0 || emitterIndex >= mEffect.Emitters.Count)
            return;

         if (magnetIndex < 0 || magnetIndex >= mEffect.Emitters[emitterIndex].Magnets.Count)
            return;

         mEffect.Emitters[emitterIndex].Magnets.RemoveAt(magnetIndex);
      }      

      private void refreshTreeView(bool expandTree)
      {         
         treeView1.SuspendLayout();
         treeView1.Nodes.Clear();

         TreeNode rootNode = new TreeNode("Particle Effect");
         rootNode.Tag = ParticleEditorPageEnum.eParticleEffect;
         rootNode.ImageIndex = 0;
         treeView1.Nodes.Add(rootNode);
         for (int i = 0; i < mEffect.Emitters.Count; ++i)
         {
            addEmitterNodeToTree(i);

            for (int j = 0; j < mEffect.Emitters[i].Magnets.Count; ++j)
            {
               addMagnetNodeToTree(i, j);
            }
         }

         rootNode.Expand();

         if (expandTree)
            treeView1.ExpandAll();

         treeView1.ResumeLayout();
      }

      private void addMagnetNodeToTree(int emitterIndex, int magnetIndex)
      {
         if (emitterIndex < 0 || emitterIndex >= mEffect.Emitters.Count)
            return;

         if (magnetIndex < 0 || magnetIndex >= mEffect.Emitters[emitterIndex].Magnets.Count)
            return;

         ParticleEmitter emitter = mEffect.Emitters[emitterIndex];
         MagnetData magnet = mEffect.Emitters[emitterIndex].Magnets[magnetIndex];
         TreeNode rootNode = treeView1.Nodes[0];
         TreeNode emitterNode = rootNode.Nodes[emitterIndex];

         addNode(emitterNode, magnet.Name, ParticleEditorPageEnum.eMagnet, 11);

         emitterNode.Expand();
      }

      private void addEmitterNodeToTree(int index)
      {
         if (index < 0 || index >= mEffect.Emitters.Count)
            return;

         ParticleEmitter emitter = mEffect.Emitters[index];
         TreeNode rootNode = treeView1.Nodes[0];

         //rootNode.Tag = ParticleEditorPageEnum.eUnknown;
         addNode(rootNode, emitter.Name, ParticleEditorPageEnum.eEmitter, 1);
         TreeNode emitterNode = rootNode.Nodes[index];

         addNode(emitterNode, "General", ParticleEditorPageEnum.eGeneral, 2);
         addNode(emitterNode, "Shape", ParticleEditorPageEnum.eShape, 3);
         addNode(emitterNode, "Textures", ParticleEditorPageEnum.eTextures, 4);
         addNode(emitterNode, "Scale", ParticleEditorPageEnum.eScale, 5);
         addNode(emitterNode, "Color", ParticleEditorPageEnum.eColor, 6);
         addNode(emitterNode, "Opacity", ParticleEditorPageEnum.eOpacity, 8);
         addNode(emitterNode, "Speed", ParticleEditorPageEnum.eSpeed, 9);
         addNode(emitterNode, "Intensity", ParticleEditorPageEnum.eIntensity, 10);         
      }
      
      private void addNode(TreeNode parentNode, string name, ParticleEditorPageEnum tagID, int imageIndex)
      {
         TreeNode newNode = new TreeNode(name);
         newNode.Tag = tagID;
         newNode.ImageIndex = imageIndex;
         newNode.SelectedImageIndex = imageIndex;

         parentNode.Nodes.Add(newNode);
      }

      private void hidePanelControls()
      {
         for (int i = 0; i < panel1.Controls.Count; i++)
         {
            panel1.Controls[i].Hide();
         }
      }
            
      private void magnetToolStripMenuItem_Click(object sender, EventArgs e)
      {
         
      }
      
      private void treeView1_AfterSelect(object sender, TreeViewEventArgs e)
      {
         hidePanelControls();         
         ParticleEditorPageEnum pageID = (ParticleEditorPageEnum)e.Node.Tag;

         //-- if the parent of this is an emitter rebind the data
         TreeNode parentNode = e.Node.Parent;
         if (parentNode != null && ((ParticleEditorPageEnum)parentNode.Tag == ParticleEditorPageEnum.eEmitter))
         {  
            if (pageID == ParticleEditorPageEnum.eMagnet)
            {
               int magnetIndex = e.Node.Index - (int) ParticleEditorPageEnum.eEmitterPropertyTotal;
               bindMagnetData(parentNode.Index, magnetIndex);
            }
            else
               bindData(parentNode.Index);                                    
         }         

         bool bFound = false;
         for (int i = 0; i < panel1.Controls.Count && !bFound; ++i)
         {
            if (pageID == (ParticleEditorPageEnum)panel1.Controls[i].Tag)
            {
               panel1.Controls[i].Show();
               bFound = true;
            }
         }

         if (!bFound)
            splashPage.Show();

      }

      private void treeView1_NodeMouseDoubleClick(object sender, TreeNodeMouseClickEventArgs e)
      {
         if ((ParticleEditorPageEnum)e.Node.Tag != (ParticleEditorPageEnum.eEmitter))
            return;

         base.OnMouseDoubleClick(e);
      }      

      //-- Delete
      private void emitterToolStripMenuItem1_Click(object sender, EventArgs e)
      {
         TreeNode node = treeView1.SelectedNode;
         if (node == null)
            return;

         if ((ParticleEditorPageEnum)node.Tag != ParticleEditorPageEnum.eEmitter)
            return;

         mEffect.deleteEmitter(node.Index);
         refreshTreeView(false);
      }

      private void treeView1_NodeMouseClick(object sender, TreeNodeMouseClickEventArgs e)
      {
         treeView1.SelectedNode = e.Node;
         ParticleEditorPageEnum id = (ParticleEditorPageEnum)e.Node.Tag;
         if ((e.Button == MouseButtons.Right) && 
             (id == ParticleEditorPageEnum.eParticleEffect || 
              id == ParticleEditorPageEnum.eEmitter        ||
              id == ParticleEditorPageEnum.eMagnet))
         {
            mTreeViewContextMenuStrip.Show((Control)treeView1, e.X, e.Y);
         }
      }

      //-- New Particle System
      private void button2_Click(object sender, EventArgs e)
      {
         DialogResult result = MessageBox.Show(String.Format("You are about to create a new Particle Effect.  Do want to save the current one?"), "Warning!", MessageBoxButtons.YesNoCancel);
         if (result == DialogResult.Cancel)
            return;

         if (result == DialogResult.Yes)
         {
            SaveParticleSystem();
         }

         mEffect = new ParticleEffect();
         //-- add one emitter by default         
         addEmitter();
         bindData(0);
         SetTabName("NewParticleEffect");
         refreshTreeView(true);         
      }

      private void button3_Click(object sender, EventArgs e)
      {
         addEmitter();
         refreshTreeView(false);
      }

      override public void save_file()
      {
         String filepath = CoreGlobals.getWorkPaths().mGameArtDirectory + "\\" + mEffect.Name;
         SaveParticleSystem(filepath);
         SetTabName(mEffect.Name);
      }

      override public void save_file_as()
      {
         SaveParticleSystem();
      }

      private void button4_Click(object sender, EventArgs e)
      {
         save_file();
      }

      private void treeView1_MouseDown(object sender, MouseEventArgs e)
      {
         TreeView tree = (TreeView)sender;
         TreeNode node = tree.GetNodeAt(e.X, e.Y);
         tree.SelectedNode = node;
         if (node == null)
            return;

         if ((ParticleEditorPageEnum)node.Tag != ParticleEditorPageEnum.eEmitter)
            return;

         if (e.Button != MouseButtons.Middle)
            return;

         if (node!= null)
         {
            ParticleEffectTreeDragDropPacket packet = new ParticleEffectTreeDragDropPacket();
            packet.emitterIndex = node.Index;
            packet.sender = treeView1;
            treeView1.DoDragDrop(packet, DragDropEffects.Copy);
         }
      }

      private void treeView1_DragOver(object sender, DragEventArgs e)
      {         
         e.Effect = DragDropEffects.None;

         ParticleEffectTreeDragDropPacket packet = (ParticleEffectTreeDragDropPacket) e.Data.GetData(typeof(ParticleEffectTreeDragDropPacket));
         if (packet == null)
            return;
         TreeView tree = (TreeView)packet.sender;

         Point pt = new Point(e.X, e.Y);
         pt = tree.PointToClient(pt);

         TreeNode node = tree.GetNodeAt(pt);
         if (node == null)
            return;

         if ((ParticleEditorPageEnum)node.Tag != ParticleEditorPageEnum.eEmitter)
            return;

         if (packet.emitterIndex == node.Index)
            return;

         e.Effect = DragDropEffects.Copy;
      }

      private void treeView1_DragDrop(object sender, DragEventArgs e)
      {
         ParticleEffectTreeDragDropPacket packet = (ParticleEffectTreeDragDropPacket)e.Data.GetData(typeof(ParticleEffectTreeDragDropPacket));
         if (packet == null)
            return;

         TreeView tree = (TreeView) sender;
         Point pt = new Point(e.X, e.Y);
         pt = tree.PointToClient(pt);

         TreeNode node = tree.GetNodeAt(pt);
         int index = node.Index;

         if (index < 0 || index>= mEffect.Emitters.Count)
            return;

         if (packet.emitterIndex < 0 || packet.emitterIndex >= mEffect.Emitters.Count)
            return;

         //-- clone the selected emitter
         ParticleEmitter clone = mEffect.Emitters[index].clone();

         mEffect.Emitters[index] = mEffect.Emitters[packet.emitterIndex];
         mEffect.Emitters[packet.emitterIndex] = clone;
         refreshTreeView(false);
      }

      private void CollapseAllButton_Click(object sender, EventArgs e)
      {
         treeView1.CollapseAll();
         if (treeView1.Nodes.Count > 0)
         {
            treeView1.SelectedNode = treeView1.Nodes[0];
            treeView1.Select();
         }

         if (treeView1.SelectedNode != null)
         {
            expandedOneLevelRecursive(treeView1.SelectedNode);
         }
         else
         {
            foreach (TreeNode node in treeView1.Nodes)
               expandedOneLevelRecursive(node);
         }
         treeView1.Select();
      }

      private void ExpandAllButton_Click(object sender, EventArgs e)
      {
         treeView1.ExpandAll();
         treeView1.Select();
      }

      private void CollapseOneLevelButton_Click(object sender, EventArgs e)
      {
         if (treeView1.SelectedNode != null)
         {
            // Get max depth
            int maxDepth = getTreeDepthRecursive(treeView1.SelectedNode, 0);

            // Collapse everything at the given depth
            collapseAtDepthRecursive(treeView1.SelectedNode, 0, (maxDepth - 1));
         }
         else
         {
            // Get max depth
            int maxDepth = -1;
            foreach (TreeNode node in treeView1.Nodes)
            {
               int newDepth = getTreeDepthRecursive(node, 0);
               if (newDepth > maxDepth)
                  maxDepth = newDepth;
            }

            // Collapse everything at the given depth
            foreach (TreeNode node in treeView1.Nodes)
            {
               collapseAtDepthRecursive(node, 0, (maxDepth - 1));
            }
         }
         treeView1.Select();
      }

      private void ExpandOneLevelButton_Click(object sender, EventArgs e)
      {
         if (treeView1.SelectedNode != null)
         {
            expandedOneLevelRecursive(treeView1.SelectedNode);
         }
         else
         {
            foreach (TreeNode node in treeView1.Nodes)
               expandedOneLevelRecursive(node);
         }
         treeView1.Select();
      }

      private int getTreeDepthRecursive(TreeNode node, int depth)
      {
         if (node.IsExpanded)
         {
            int maxDepth = -1;
            foreach (TreeNode childNode in node.Nodes)
            {
               int newDepth = getTreeDepthRecursive(childNode, depth + 1);
               if (newDepth > maxDepth)
                  maxDepth = newDepth;
            }

            return (maxDepth);
         }
         else
         {
            return (depth);
         }
      }

      private void collapseAtDepthRecursive(TreeNode node, int depth, int collapseDepth)
      {
         if (collapseDepth == depth)
         {
            node.Collapse();
         }
         else
         {
            if (node.IsExpanded)
            {
               foreach (TreeNode childNode in node.Nodes)
               {
                  collapseAtDepthRecursive(childNode, depth + 1, collapseDepth);
               }
            }
         }
      }

      private void expandedOneLevelRecursive(TreeNode node)
      {
         if (node.IsExpanded)
         {
            foreach (TreeNode childNode in node.Nodes)
               expandedOneLevelRecursive(childNode);
         }
         else
         {
            node.Expand();
         }
      }


      public void CreateNewEffectVisFile(string newAnimFile, string gr2file)
      {
         XmlDocument animTemplateDoc = new XmlDocument();
         animTemplateDoc.Load(CoreGlobals.getWorkPaths().mGameArtDirectory + "\\template\\effect" + CoreGlobals.getWorkPaths().mUnitDataExtension);

         XmlNodeList nodes = animTemplateDoc.SelectNodes("//asset[@type='Particle']/file");

         if (nodes.Count > 0)
         {
            nodes[0].InnerText = gr2file.Replace(".pfx", ""); ;
         }
         string newFullFileName = Path.Combine(CoreGlobals.getWorkPaths().mGameArtDirectory, newAnimFile);
         animTemplateDoc.Save(newFullFileName);

         XMBProcessor.CreateXMB(newFullFileName, false);
      }


      private void QuickViewButton_Click(object sender, EventArgs e)
      {
         WriteParticleSystem(CoreGlobals.getWorkPaths().mGameArtDirectory + @"\system\quickview.pfx");
         CreateNewEffectVisFile(@"system\quickview.vis", @"system\quickview.pfx");

         if (!s_bParticleQuickViewClicked)
         {
            if (XFSInterface.launchApp())
            {
               XFSInterface.launchGame();

               XFSInterface.launchVisual("sys_quickview");

               s_bParticleQuickViewClicked = true;
            }
         }
      }
   }

   public class ParticleEffectTreeDragDropPacket
   {
      public int    emitterIndex;
      public object sender;
   };
}
