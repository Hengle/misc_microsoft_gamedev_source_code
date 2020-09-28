namespace VisualEditor
{
   partial class VisualEditorPage
   {
      /// <summary> 
      /// Required designer variable.
      /// </summary>
      private System.ComponentModel.IContainer components = null;

      /// <summary> 
      /// Clean up any resources being used.
      /// </summary>
      /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
      protected override void Dispose(bool disposing)
      {
         if (disposing && (components != null))
         {
            components.Dispose();
         }
         base.Dispose(disposing);
      }

      #region Component Designer generated code

      /// <summary> 
      /// Required method for Designer support - do not modify 
      /// the contents of this method with the code editor.
      /// </summary>
      private void InitializeComponent()
      {
         this.components = new System.ComponentModel.Container();
         System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(VisualEditorPage));
         this.panel1 = new System.Windows.Forms.Panel();
         this.menuStrip1 = new System.Windows.Forms.MenuStrip();
         this.fileToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.editToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.undoToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.redoToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.toolStripSeparator1 = new System.Windows.Forms.ToolStripSeparator();
         this.cutToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.copyToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.pasteToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.deleteToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.viewToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.resortItemsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.splitContainer1 = new System.Windows.Forms.SplitContainer();
         this.QuickViewButton = new System.Windows.Forms.Button();
         this.UndoButton = new System.Windows.Forms.Button();
         this.RedoButton = new System.Windows.Forms.Button();
         this.NewButton = new System.Windows.Forms.Button();
         this.ExpandOneLeveButton = new System.Windows.Forms.Button();
         this.CollapseOneLevelButton = new System.Windows.Forms.Button();
         this.ExpandAllButton = new System.Windows.Forms.Button();
         this.CollapseAllButton = new System.Windows.Forms.Button();
         this.SaveAsButton = new System.Windows.Forms.Button();
         this.SaveButton = new System.Windows.Forms.Button();
         this.OpenButton = new System.Windows.Forms.Button();
         this.panelProperty = new System.Windows.Forms.Panel();
         this.VisualTreeView = new System.Windows.Forms.TreeView();
         this.TreeViewImageList = new System.Windows.Forms.ImageList(this.components);
         this.AnimationControlPanel = new System.Windows.Forms.Panel();
         this.MotionButton = new System.Windows.Forms.Button();
         this.AnimationControlImageList = new System.Windows.Forms.ImageList(this.components);
         this.SoundButton = new System.Windows.Forms.Button();
         this.AnimationDurationTextBox = new System.Windows.Forms.TextBox();
         this.AnimationTimeTextBox = new System.Windows.Forms.TextBox();
         this.SpeedButton = new System.Windows.Forms.Button();
         this.RepeatButton = new System.Windows.Forms.Button();
         this.AnimationTimeTrackBar = new System.Windows.Forms.TrackBar();
         this.ForwardButton = new System.Windows.Forms.Button();
         this.PlayButton = new System.Windows.Forms.Button();
         this.RewindButton = new System.Windows.Forms.Button();
         this.panel2 = new System.Windows.Forms.Panel();
         this.toolStripContainer1 = new System.Windows.Forms.ToolStripContainer();
         this.toolTip1 = new System.Windows.Forms.ToolTip(this.components);
         this.qUICKVIEWToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.toolStripSeparator2 = new System.Windows.Forms.ToolStripSeparator();
         this.panel1.SuspendLayout();
         this.menuStrip1.SuspendLayout();
         this.splitContainer1.Panel1.SuspendLayout();
         this.splitContainer1.Panel2.SuspendLayout();
         this.splitContainer1.SuspendLayout();
         this.AnimationControlPanel.SuspendLayout();
         ((System.ComponentModel.ISupportInitialize)(this.AnimationTimeTrackBar)).BeginInit();
         this.panel2.SuspendLayout();
         this.toolStripContainer1.SuspendLayout();
         this.SuspendLayout();
         // 
         // panel1
         // 
         this.panel1.Controls.Add(this.menuStrip1);
         this.panel1.Controls.Add(this.splitContainer1);
         this.panel1.Dock = System.Windows.Forms.DockStyle.Fill;
         this.panel1.Location = new System.Drawing.Point(0, 0);
         this.panel1.Name = "panel1";
         this.panel1.Size = new System.Drawing.Size(850, 589);
         this.panel1.TabIndex = 0;
         // 
         // menuStrip1
         // 
         this.menuStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.fileToolStripMenuItem,
            this.editToolStripMenuItem,
            this.viewToolStripMenuItem});
         this.menuStrip1.Location = new System.Drawing.Point(0, 0);
         this.menuStrip1.Name = "menuStrip1";
         this.menuStrip1.Size = new System.Drawing.Size(850, 24);
         this.menuStrip1.TabIndex = 1;
         this.menuStrip1.Text = "menuStrip1";
         this.menuStrip1.Visible = false;
         // 
         // fileToolStripMenuItem
         // 
         this.fileToolStripMenuItem.Name = "fileToolStripMenuItem";
         this.fileToolStripMenuItem.Size = new System.Drawing.Size(35, 20);
         this.fileToolStripMenuItem.Text = "File";
         // 
         // editToolStripMenuItem
         // 
         this.editToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.undoToolStripMenuItem,
            this.redoToolStripMenuItem,
            this.toolStripSeparator1,
            this.cutToolStripMenuItem,
            this.copyToolStripMenuItem,
            this.pasteToolStripMenuItem,
            this.deleteToolStripMenuItem});
         this.editToolStripMenuItem.Name = "editToolStripMenuItem";
         this.editToolStripMenuItem.Size = new System.Drawing.Size(37, 20);
         this.editToolStripMenuItem.Text = "Edit";
         // 
         // undoToolStripMenuItem
         // 
         this.undoToolStripMenuItem.Image = ((System.Drawing.Image)(resources.GetObject("undoToolStripMenuItem.Image")));
         this.undoToolStripMenuItem.Name = "undoToolStripMenuItem";
         this.undoToolStripMenuItem.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.Z)));
         this.undoToolStripMenuItem.Size = new System.Drawing.Size(152, 22);
         this.undoToolStripMenuItem.Text = "Undo";
         this.undoToolStripMenuItem.Click += new System.EventHandler(this.undoToolStripMenuItem_Click);
         // 
         // redoToolStripMenuItem
         // 
         this.redoToolStripMenuItem.Image = ((System.Drawing.Image)(resources.GetObject("redoToolStripMenuItem.Image")));
         this.redoToolStripMenuItem.Name = "redoToolStripMenuItem";
         this.redoToolStripMenuItem.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.Y)));
         this.redoToolStripMenuItem.Size = new System.Drawing.Size(139, 22);
         this.redoToolStripMenuItem.Text = "Redo";
         this.redoToolStripMenuItem.Click += new System.EventHandler(this.redoToolStripMenuItem_Click);
         // 
         // toolStripSeparator1
         // 
         this.toolStripSeparator1.Name = "toolStripSeparator1";
         this.toolStripSeparator1.Size = new System.Drawing.Size(136, 6);
         // 
         // cutToolStripMenuItem
         // 
         this.cutToolStripMenuItem.Image = ((System.Drawing.Image)(resources.GetObject("cutToolStripMenuItem.Image")));
         this.cutToolStripMenuItem.Name = "cutToolStripMenuItem";
         this.cutToolStripMenuItem.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.X)));
         this.cutToolStripMenuItem.Size = new System.Drawing.Size(139, 22);
         this.cutToolStripMenuItem.Text = "Cut";
         this.cutToolStripMenuItem.Click += new System.EventHandler(this.cutToolStripMenuItem_Click);
         // 
         // copyToolStripMenuItem
         // 
         this.copyToolStripMenuItem.Image = ((System.Drawing.Image)(resources.GetObject("copyToolStripMenuItem.Image")));
         this.copyToolStripMenuItem.Name = "copyToolStripMenuItem";
         this.copyToolStripMenuItem.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.C)));
         this.copyToolStripMenuItem.Size = new System.Drawing.Size(139, 22);
         this.copyToolStripMenuItem.Text = "Copy";
         this.copyToolStripMenuItem.Click += new System.EventHandler(this.copyToolStripMenuItem_Click);
         // 
         // pasteToolStripMenuItem
         // 
         this.pasteToolStripMenuItem.Image = ((System.Drawing.Image)(resources.GetObject("pasteToolStripMenuItem.Image")));
         this.pasteToolStripMenuItem.Name = "pasteToolStripMenuItem";
         this.pasteToolStripMenuItem.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.V)));
         this.pasteToolStripMenuItem.Size = new System.Drawing.Size(139, 22);
         this.pasteToolStripMenuItem.Text = "Paste";
         this.pasteToolStripMenuItem.Click += new System.EventHandler(this.pasteToolStripMenuItem_Click);
         // 
         // deleteToolStripMenuItem
         // 
         this.deleteToolStripMenuItem.Image = ((System.Drawing.Image)(resources.GetObject("deleteToolStripMenuItem.Image")));
         this.deleteToolStripMenuItem.Name = "deleteToolStripMenuItem";
         this.deleteToolStripMenuItem.ShortcutKeys = System.Windows.Forms.Keys.Delete;
         this.deleteToolStripMenuItem.Size = new System.Drawing.Size(139, 22);
         this.deleteToolStripMenuItem.Text = "Delete";
         this.deleteToolStripMenuItem.Click += new System.EventHandler(this.deleteToolStripMenuItem_Click);
         // 
         // viewToolStripMenuItem
         // 
         this.viewToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.toolStripSeparator2,
            this.resortItemsToolStripMenuItem,
            this.qUICKVIEWToolStripMenuItem});
         this.viewToolStripMenuItem.Name = "viewToolStripMenuItem";
         this.viewToolStripMenuItem.Size = new System.Drawing.Size(41, 20);
         this.viewToolStripMenuItem.Text = "View";
         // 
         // resortItemsToolStripMenuItem
         // 
         this.resortItemsToolStripMenuItem.Name = "resortItemsToolStripMenuItem";
         this.resortItemsToolStripMenuItem.ShortcutKeys = System.Windows.Forms.Keys.F5;
         this.resortItemsToolStripMenuItem.Size = new System.Drawing.Size(156, 22);
         this.resortItemsToolStripMenuItem.Text = "Refresh Tree";
         this.resortItemsToolStripMenuItem.Click += new System.EventHandler(this.resortItemsToolStripMenuItem_Click);
         // 
         // splitContainer1
         // 
         this.splitContainer1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.splitContainer1.Location = new System.Drawing.Point(3, 3);
         this.splitContainer1.Name = "splitContainer1";
         // 
         // splitContainer1.Panel1
         // 
         this.splitContainer1.Panel1.Controls.Add(this.QuickViewButton);
         this.splitContainer1.Panel1.Controls.Add(this.UndoButton);
         this.splitContainer1.Panel1.Controls.Add(this.RedoButton);
         this.splitContainer1.Panel1.Controls.Add(this.NewButton);
         this.splitContainer1.Panel1.Controls.Add(this.ExpandOneLeveButton);
         this.splitContainer1.Panel1.Controls.Add(this.CollapseOneLevelButton);
         this.splitContainer1.Panel1.Controls.Add(this.ExpandAllButton);
         this.splitContainer1.Panel1.Controls.Add(this.CollapseAllButton);
         this.splitContainer1.Panel1.Controls.Add(this.SaveAsButton);
         this.splitContainer1.Panel1.Controls.Add(this.SaveButton);
         this.splitContainer1.Panel1.Controls.Add(this.OpenButton);
         this.splitContainer1.Panel1.Controls.Add(this.panelProperty);
         this.splitContainer1.Panel1.Controls.Add(this.VisualTreeView);
         this.splitContainer1.Panel1MinSize = 371;
         // 
         // splitContainer1.Panel2
         // 
         this.splitContainer1.Panel2.Controls.Add(this.AnimationControlPanel);
         this.splitContainer1.Panel2.Controls.Add(this.panel2);
         this.splitContainer1.Size = new System.Drawing.Size(844, 583);
         this.splitContainer1.SplitterDistance = 371;
         this.splitContainer1.TabIndex = 5;
         // 
         // QuickViewButton
         // 
         this.QuickViewButton.Image = ((System.Drawing.Image)(resources.GetObject("QuickViewButton.Image")));
         this.QuickViewButton.Location = new System.Drawing.Point(168, 0);
         this.QuickViewButton.Margin = new System.Windows.Forms.Padding(1, 3, 1, 1);
         this.QuickViewButton.Name = "QuickViewButton";
         this.QuickViewButton.Size = new System.Drawing.Size(40, 40);
         this.QuickViewButton.TabIndex = 6;
         this.toolTip1.SetToolTip(this.QuickViewButton, "QUICKVIEW!");
         this.QuickViewButton.UseVisualStyleBackColor = true;
         this.QuickViewButton.Click += new System.EventHandler(this.QuickViewButton_Click);
         // 
         // UndoButton
         // 
         this.UndoButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
         this.UndoButton.Image = ((System.Drawing.Image)(resources.GetObject("UndoButton.Image")));
         this.UndoButton.Location = new System.Drawing.Point(217, 16);
         this.UndoButton.Margin = new System.Windows.Forms.Padding(1, 3, 1, 1);
         this.UndoButton.Name = "UndoButton";
         this.UndoButton.Size = new System.Drawing.Size(24, 24);
         this.UndoButton.TabIndex = 7;
         this.UndoButton.TabStop = false;
         this.toolTip1.SetToolTip(this.UndoButton, "Undo");
         this.UndoButton.UseVisualStyleBackColor = true;
         this.UndoButton.Click += new System.EventHandler(this.UndoButton_Click);
         // 
         // RedoButton
         // 
         this.RedoButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
         this.RedoButton.Image = ((System.Drawing.Image)(resources.GetObject("RedoButton.Image")));
         this.RedoButton.Location = new System.Drawing.Point(243, 16);
         this.RedoButton.Margin = new System.Windows.Forms.Padding(1, 3, 1, 1);
         this.RedoButton.Name = "RedoButton";
         this.RedoButton.Size = new System.Drawing.Size(24, 24);
         this.RedoButton.TabIndex = 8;
         this.RedoButton.TabStop = false;
         this.toolTip1.SetToolTip(this.RedoButton, "Redo");
         this.RedoButton.UseVisualStyleBackColor = true;
         this.RedoButton.Click += new System.EventHandler(this.RedoButton_Click);
         // 
         // NewButton
         // 
         this.NewButton.Image = ((System.Drawing.Image)(resources.GetObject("NewButton.Image")));
         this.NewButton.Location = new System.Drawing.Point(0, 0);
         this.NewButton.Margin = new System.Windows.Forms.Padding(1, 3, 1, 1);
         this.NewButton.Name = "NewButton";
         this.NewButton.Size = new System.Drawing.Size(40, 40);
         this.NewButton.TabIndex = 2;
         this.toolTip1.SetToolTip(this.NewButton, "Create New File");
         this.NewButton.UseVisualStyleBackColor = true;
         this.NewButton.Click += new System.EventHandler(this.NewButton_Click);
         // 
         // ExpandOneLeveButton
         // 
         this.ExpandOneLeveButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
         this.ExpandOneLeveButton.Image = ((System.Drawing.Image)(resources.GetObject("ExpandOneLeveButton.Image")));
         this.ExpandOneLeveButton.Location = new System.Drawing.Point(346, 16);
         this.ExpandOneLeveButton.Margin = new System.Windows.Forms.Padding(1, 3, 1, 1);
         this.ExpandOneLeveButton.Name = "ExpandOneLeveButton";
         this.ExpandOneLeveButton.Size = new System.Drawing.Size(24, 24);
         this.ExpandOneLeveButton.TabIndex = 12;
         this.ExpandOneLeveButton.TabStop = false;
         this.toolTip1.SetToolTip(this.ExpandOneLeveButton, "Expand One Level");
         this.ExpandOneLeveButton.UseVisualStyleBackColor = true;
         this.ExpandOneLeveButton.Click += new System.EventHandler(this.ExpandOneLeveButton_Click);
         // 
         // CollapseOneLevelButton
         // 
         this.CollapseOneLevelButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
         this.CollapseOneLevelButton.Image = ((System.Drawing.Image)(resources.GetObject("CollapseOneLevelButton.Image")));
         this.CollapseOneLevelButton.Location = new System.Drawing.Point(321, 16);
         this.CollapseOneLevelButton.Margin = new System.Windows.Forms.Padding(1, 3, 1, 1);
         this.CollapseOneLevelButton.Name = "CollapseOneLevelButton";
         this.CollapseOneLevelButton.Size = new System.Drawing.Size(24, 24);
         this.CollapseOneLevelButton.TabIndex = 11;
         this.CollapseOneLevelButton.TabStop = false;
         this.toolTip1.SetToolTip(this.CollapseOneLevelButton, "Collapse One Level");
         this.CollapseOneLevelButton.UseVisualStyleBackColor = true;
         this.CollapseOneLevelButton.Click += new System.EventHandler(this.CollapseOneLevelButton_Click);
         // 
         // ExpandAllButton
         // 
         this.ExpandAllButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
         this.ExpandAllButton.Image = ((System.Drawing.Image)(resources.GetObject("ExpandAllButton.Image")));
         this.ExpandAllButton.Location = new System.Drawing.Point(295, 16);
         this.ExpandAllButton.Margin = new System.Windows.Forms.Padding(1, 3, 1, 1);
         this.ExpandAllButton.Name = "ExpandAllButton";
         this.ExpandAllButton.Size = new System.Drawing.Size(24, 24);
         this.ExpandAllButton.TabIndex = 10;
         this.ExpandAllButton.TabStop = false;
         this.toolTip1.SetToolTip(this.ExpandAllButton, "Expand All");
         this.ExpandAllButton.UseVisualStyleBackColor = true;
         this.ExpandAllButton.Click += new System.EventHandler(this.ExpandAllButton_Click);
         // 
         // CollapseAllButton
         // 
         this.CollapseAllButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
         this.CollapseAllButton.Image = ((System.Drawing.Image)(resources.GetObject("CollapseAllButton.Image")));
         this.CollapseAllButton.Location = new System.Drawing.Point(269, 16);
         this.CollapseAllButton.Margin = new System.Windows.Forms.Padding(1, 3, 1, 1);
         this.CollapseAllButton.Name = "CollapseAllButton";
         this.CollapseAllButton.Size = new System.Drawing.Size(24, 24);
         this.CollapseAllButton.TabIndex = 9;
         this.CollapseAllButton.TabStop = false;
         this.toolTip1.SetToolTip(this.CollapseAllButton, "Collapse All");
         this.CollapseAllButton.UseVisualStyleBackColor = true;
         this.CollapseAllButton.Click += new System.EventHandler(this.CollapseAllButton_Click);
         // 
         // SaveAsButton
         // 
         this.SaveAsButton.Image = ((System.Drawing.Image)(resources.GetObject("SaveAsButton.Image")));
         this.SaveAsButton.Location = new System.Drawing.Point(126, 0);
         this.SaveAsButton.Margin = new System.Windows.Forms.Padding(1, 3, 1, 1);
         this.SaveAsButton.Name = "SaveAsButton";
         this.SaveAsButton.Size = new System.Drawing.Size(40, 40);
         this.SaveAsButton.TabIndex = 5;
         this.toolTip1.SetToolTip(this.SaveAsButton, "Save As");
         this.SaveAsButton.UseVisualStyleBackColor = true;
         this.SaveAsButton.Click += new System.EventHandler(this.SaveAsButton_Click);
         // 
         // SaveButton
         // 
         this.SaveButton.Image = ((System.Drawing.Image)(resources.GetObject("SaveButton.Image")));
         this.SaveButton.Location = new System.Drawing.Point(84, 0);
         this.SaveButton.Margin = new System.Windows.Forms.Padding(1, 3, 1, 1);
         this.SaveButton.Name = "SaveButton";
         this.SaveButton.Size = new System.Drawing.Size(40, 40);
         this.SaveButton.TabIndex = 4;
         this.toolTip1.SetToolTip(this.SaveButton, "Save");
         this.SaveButton.UseVisualStyleBackColor = true;
         this.SaveButton.Click += new System.EventHandler(this.SaveButton_Click);
         // 
         // OpenButton
         // 
         this.OpenButton.Image = ((System.Drawing.Image)(resources.GetObject("OpenButton.Image")));
         this.OpenButton.Location = new System.Drawing.Point(42, 0);
         this.OpenButton.Margin = new System.Windows.Forms.Padding(1, 3, 1, 1);
         this.OpenButton.Name = "OpenButton";
         this.OpenButton.Size = new System.Drawing.Size(40, 40);
         this.OpenButton.TabIndex = 3;
         this.toolTip1.SetToolTip(this.OpenButton, "Open File");
         this.OpenButton.UseVisualStyleBackColor = true;
         this.OpenButton.Click += new System.EventHandler(this.OpenButton_Click);
         // 
         // panelProperty
         // 
         this.panelProperty.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.panelProperty.Location = new System.Drawing.Point(0, 333);
         this.panelProperty.MaximumSize = new System.Drawing.Size(350, 250);
         this.panelProperty.MinimumSize = new System.Drawing.Size(350, 250);
         this.panelProperty.Name = "panelProperty";
         this.panelProperty.Size = new System.Drawing.Size(350, 250);
         this.panelProperty.TabIndex = 1;
         // 
         // VisualTreeView
         // 
         this.VisualTreeView.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.VisualTreeView.HideSelection = false;
         this.VisualTreeView.ImageIndex = 0;
         this.VisualTreeView.ImageList = this.TreeViewImageList;
         this.VisualTreeView.Location = new System.Drawing.Point(0, 44);
         this.VisualTreeView.Name = "VisualTreeView";
         this.VisualTreeView.SelectedImageIndex = 0;
         this.VisualTreeView.Size = new System.Drawing.Size(370, 283);
         this.VisualTreeView.TabIndex = 0;
         this.VisualTreeView.AfterSelect += new System.Windows.Forms.TreeViewEventHandler(this.VisualTreeView_AfterSelect);
         this.VisualTreeView.NodeMouseClick += new System.Windows.Forms.TreeNodeMouseClickEventHandler(this.VisualTreeView_NodeMouseClick);
         // 
         // TreeViewImageList
         // 
         this.TreeViewImageList.ImageStream = ((System.Windows.Forms.ImageListStreamer)(resources.GetObject("TreeViewImageList.ImageStream")));
         this.TreeViewImageList.TransparentColor = System.Drawing.Color.Transparent;
         this.TreeViewImageList.Images.SetKeyName(0, "VisualNode_visual.ico");
         this.TreeViewImageList.Images.SetKeyName(1, "VisualNode_model.ico");
         this.TreeViewImageList.Images.SetKeyName(2, "VisualNode_component.ico");
         this.TreeViewImageList.Images.SetKeyName(3, "VisualNode_animation.ico");
         this.TreeViewImageList.Images.SetKeyName(4, "VisualNode_attachment_model.png");
         this.TreeViewImageList.Images.SetKeyName(5, "VisualNode_attachment_modelref.png");
         this.TreeViewImageList.Images.SetKeyName(6, "VisualNode_attachment_particle.png");
         this.TreeViewImageList.Images.SetKeyName(7, "VisualNode_attachment_light.png");
         this.TreeViewImageList.Images.SetKeyName(8, "VisualNode_attachment_point.png");
         this.TreeViewImageList.Images.SetKeyName(9, "VisualNode_tag_attack.png");
         this.TreeViewImageList.Images.SetKeyName(10, "VisualNode_tag_sound.png");
         this.TreeViewImageList.Images.SetKeyName(11, "VisualNode_tag_particle.png");
         this.TreeViewImageList.Images.SetKeyName(12, "VisualNode_tag_light.png");
         this.TreeViewImageList.Images.SetKeyName(13, "VisualNode_tag_camshake.png");
         // 
         // AnimationControlPanel
         // 
         this.AnimationControlPanel.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.AnimationControlPanel.Controls.Add(this.MotionButton);
         this.AnimationControlPanel.Controls.Add(this.SoundButton);
         this.AnimationControlPanel.Controls.Add(this.AnimationDurationTextBox);
         this.AnimationControlPanel.Controls.Add(this.AnimationTimeTextBox);
         this.AnimationControlPanel.Controls.Add(this.SpeedButton);
         this.AnimationControlPanel.Controls.Add(this.RepeatButton);
         this.AnimationControlPanel.Controls.Add(this.AnimationTimeTrackBar);
         this.AnimationControlPanel.Controls.Add(this.ForwardButton);
         this.AnimationControlPanel.Controls.Add(this.PlayButton);
         this.AnimationControlPanel.Controls.Add(this.RewindButton);
         this.AnimationControlPanel.Enabled = false;
         this.AnimationControlPanel.Location = new System.Drawing.Point(1, 555);
         this.AnimationControlPanel.Name = "AnimationControlPanel";
         this.AnimationControlPanel.Size = new System.Drawing.Size(468, 28);
         this.AnimationControlPanel.TabIndex = 20;
         // 
         // MotionButton
         // 
         this.MotionButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
         this.MotionButton.ImageIndex = 2;
         this.MotionButton.ImageList = this.AnimationControlImageList;
         this.MotionButton.Location = new System.Drawing.Point(441, 4);
         this.MotionButton.Name = "MotionButton";
         this.MotionButton.Size = new System.Drawing.Size(24, 24);
         this.MotionButton.TabIndex = 9;
         this.MotionButton.TabStop = false;
         this.toolTip1.SetToolTip(this.MotionButton, "Motion on/off");
         this.MotionButton.UseVisualStyleBackColor = true;
         this.MotionButton.Click += new System.EventHandler(this.MotionButton_Click);
         // 
         // AnimationControlImageList
         // 
         this.AnimationControlImageList.ImageStream = ((System.Windows.Forms.ImageListStreamer)(resources.GetObject("AnimationControlImageList.ImageStream")));
         this.AnimationControlImageList.TransparentColor = System.Drawing.Color.Transparent;
         this.AnimationControlImageList.Images.SetKeyName(0, "VisualEditor_Play.png");
         this.AnimationControlImageList.Images.SetKeyName(1, "VisualEditor_Pause.png");
         this.AnimationControlImageList.Images.SetKeyName(2, "VisualEditor_RepeatOn.png");
         this.AnimationControlImageList.Images.SetKeyName(3, "VisualEditor_RepeatOff.png");
         this.AnimationControlImageList.Images.SetKeyName(4, "VisualEditor_Forward.png");
         this.AnimationControlImageList.Images.SetKeyName(5, "VisualEditor_Rewind.png");
         this.AnimationControlImageList.Images.SetKeyName(6, "VisualEditor_SoundOn.png");
         this.AnimationControlImageList.Images.SetKeyName(7, "VisualEditor_SoundOff.png");
         // 
         // SoundButton
         // 
         this.SoundButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
         this.SoundButton.ImageIndex = 6;
         this.SoundButton.ImageList = this.AnimationControlImageList;
         this.SoundButton.Location = new System.Drawing.Point(417, 4);
         this.SoundButton.Margin = new System.Windows.Forms.Padding(0);
         this.SoundButton.Name = "SoundButton";
         this.SoundButton.Size = new System.Drawing.Size(24, 24);
         this.SoundButton.TabIndex = 8;
         this.SoundButton.TabStop = false;
         this.toolTip1.SetToolTip(this.SoundButton, "Loop on/off");
         this.SoundButton.UseVisualStyleBackColor = true;
         this.SoundButton.Click += new System.EventHandler(this.SoundButton_Click);
         // 
         // AnimationDurationTextBox
         // 
         this.AnimationDurationTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
         this.AnimationDurationTextBox.BackColor = System.Drawing.SystemColors.Control;
         this.AnimationDurationTextBox.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(29)))), ((int)(((byte)(102)))), ((int)(((byte)(245)))));
         this.AnimationDurationTextBox.Location = new System.Drawing.Point(317, 6);
         this.AnimationDurationTextBox.Margin = new System.Windows.Forms.Padding(2);
         this.AnimationDurationTextBox.MaxLength = 5;
         this.AnimationDurationTextBox.Name = "AnimationDurationTextBox";
         this.AnimationDurationTextBox.Size = new System.Drawing.Size(35, 20);
         this.AnimationDurationTextBox.TabIndex = 6;
         this.AnimationDurationTextBox.TabStop = false;
         this.toolTip1.SetToolTip(this.AnimationDurationTextBox, "Duration");
         // 
         // AnimationTimeTextBox
         // 
         this.AnimationTimeTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
         this.AnimationTimeTextBox.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(29)))), ((int)(((byte)(102)))), ((int)(((byte)(245)))));
         this.AnimationTimeTextBox.Location = new System.Drawing.Point(280, 6);
         this.AnimationTimeTextBox.Margin = new System.Windows.Forms.Padding(2);
         this.AnimationTimeTextBox.MaxLength = 5;
         this.AnimationTimeTextBox.Name = "AnimationTimeTextBox";
         this.AnimationTimeTextBox.Size = new System.Drawing.Size(35, 20);
         this.AnimationTimeTextBox.TabIndex = 4;
         this.AnimationTimeTextBox.TabStop = false;
         // 
         // SpeedButton
         // 
         this.SpeedButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
         this.SpeedButton.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(29)))), ((int)(((byte)(102)))), ((int)(((byte)(245)))));
         this.SpeedButton.ImageList = this.AnimationControlImageList;
         this.SpeedButton.Location = new System.Drawing.Point(354, 4);
         this.SpeedButton.Margin = new System.Windows.Forms.Padding(0);
         this.SpeedButton.Name = "SpeedButton";
         this.SpeedButton.Size = new System.Drawing.Size(39, 24);
         this.SpeedButton.TabIndex = 7;
         this.SpeedButton.TabStop = false;
         this.SpeedButton.Text = "1x";
         this.toolTip1.SetToolTip(this.SpeedButton, "Speed");
         this.SpeedButton.UseVisualStyleBackColor = true;
         this.SpeedButton.Click += new System.EventHandler(this.SpeedButton_Click);
         // 
         // RepeatButton
         // 
         this.RepeatButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
         this.RepeatButton.ImageIndex = 2;
         this.RepeatButton.ImageList = this.AnimationControlImageList;
         this.RepeatButton.Location = new System.Drawing.Point(393, 4);
         this.RepeatButton.Margin = new System.Windows.Forms.Padding(0);
         this.RepeatButton.Name = "RepeatButton";
         this.RepeatButton.Size = new System.Drawing.Size(24, 24);
         this.RepeatButton.TabIndex = 20;
         this.RepeatButton.TabStop = false;
         this.toolTip1.SetToolTip(this.RepeatButton, "Loop on/off");
         this.RepeatButton.UseVisualStyleBackColor = true;
         this.RepeatButton.Click += new System.EventHandler(this.RepeatButton_Click);
         // 
         // AnimationTimeTrackBar
         // 
         this.AnimationTimeTrackBar.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.AnimationTimeTrackBar.LargeChange = 25;
         this.AnimationTimeTrackBar.Location = new System.Drawing.Point(75, 3);
         this.AnimationTimeTrackBar.Maximum = 1000;
         this.AnimationTimeTrackBar.Name = "AnimationTimeTrackBar";
         this.AnimationTimeTrackBar.Size = new System.Drawing.Size(200, 45);
         this.AnimationTimeTrackBar.SmallChange = 5;
         this.AnimationTimeTrackBar.TabIndex = 3;
         this.AnimationTimeTrackBar.TabStop = false;
         this.AnimationTimeTrackBar.TickFrequency = 1000;
         this.AnimationTimeTrackBar.TickStyle = System.Windows.Forms.TickStyle.None;
         this.AnimationTimeTrackBar.Scroll += new System.EventHandler(this.AnimationTimeTrackBar_Scroll);
         // 
         // ForwardButton
         // 
         this.ForwardButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
         this.ForwardButton.ImageIndex = 4;
         this.ForwardButton.ImageList = this.AnimationControlImageList;
         this.ForwardButton.Location = new System.Drawing.Point(48, 3);
         this.ForwardButton.Margin = new System.Windows.Forms.Padding(0);
         this.ForwardButton.Name = "ForwardButton";
         this.ForwardButton.Size = new System.Drawing.Size(24, 24);
         this.ForwardButton.TabIndex = 2;
         this.ForwardButton.TabStop = false;
         this.toolTip1.SetToolTip(this.ForwardButton, "Forward Animation");
         this.ForwardButton.UseVisualStyleBackColor = true;
         this.ForwardButton.Click += new System.EventHandler(this.ForwardButton_Click);
         // 
         // PlayButton
         // 
         this.PlayButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
         this.PlayButton.ImageIndex = 0;
         this.PlayButton.ImageList = this.AnimationControlImageList;
         this.PlayButton.Location = new System.Drawing.Point(24, 3);
         this.PlayButton.Margin = new System.Windows.Forms.Padding(0);
         this.PlayButton.Name = "PlayButton";
         this.PlayButton.Size = new System.Drawing.Size(24, 24);
         this.PlayButton.TabIndex = 1;
         this.PlayButton.TabStop = false;
         this.toolTip1.SetToolTip(this.PlayButton, "Play/Pause Animation");
         this.PlayButton.UseVisualStyleBackColor = true;
         this.PlayButton.Click += new System.EventHandler(this.PlayButton_Click);
         // 
         // RewindButton
         // 
         this.RewindButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
         this.RewindButton.ImageIndex = 5;
         this.RewindButton.ImageList = this.AnimationControlImageList;
         this.RewindButton.Location = new System.Drawing.Point(0, 3);
         this.RewindButton.Margin = new System.Windows.Forms.Padding(0);
         this.RewindButton.Name = "RewindButton";
         this.RewindButton.Size = new System.Drawing.Size(24, 24);
         this.RewindButton.TabIndex = 0;
         this.RewindButton.TabStop = false;
         this.toolTip1.SetToolTip(this.RewindButton, "Rewind Animation");
         this.RewindButton.UseVisualStyleBackColor = true;
         this.RewindButton.Click += new System.EventHandler(this.RewindButton_Click);
         // 
         // panel2
         // 
         this.panel2.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.panel2.BackColor = System.Drawing.SystemColors.Control;
         this.panel2.BackgroundImage = ((System.Drawing.Image)(resources.GetObject("panel2.BackgroundImage")));
         this.panel2.BackgroundImageLayout = System.Windows.Forms.ImageLayout.Center;
         this.panel2.Controls.Add(this.toolStripContainer1);
         this.panel2.Location = new System.Drawing.Point(1, 0);
         this.panel2.Name = "panel2";
         this.panel2.Size = new System.Drawing.Size(468, 552);
         this.panel2.TabIndex = 0;
         this.panel2.MouseClick += new System.Windows.Forms.MouseEventHandler(this.panel2_MouseClick);
         this.panel2.Resize += new System.EventHandler(this.panel2_Resize);
         // 
         // toolStripContainer1
         // 
         this.toolStripContainer1.BottomToolStripPanelVisible = false;
         // 
         // toolStripContainer1.ContentPanel
         // 
         this.toolStripContainer1.ContentPanel.BackColor = System.Drawing.Color.Transparent;
         this.toolStripContainer1.ContentPanel.Enabled = false;
         this.toolStripContainer1.ContentPanel.Size = new System.Drawing.Size(287, 19);
         this.toolStripContainer1.ContentPanel.Visible = false;
         this.toolStripContainer1.LeftToolStripPanelVisible = false;
         this.toolStripContainer1.Location = new System.Drawing.Point(258, 22);
         this.toolStripContainer1.Name = "toolStripContainer1";
         this.toolStripContainer1.RightToolStripPanelVisible = false;
         this.toolStripContainer1.Size = new System.Drawing.Size(287, 44);
         this.toolStripContainer1.TabIndex = 0;
         this.toolStripContainer1.Text = "toolStripContainer1";
         this.toolStripContainer1.Visible = false;
         // 
         // qUICKVIEWToolStripMenuItem
         // 
         this.qUICKVIEWToolStripMenuItem.Name = "qUICKVIEWToolStripMenuItem";
         this.qUICKVIEWToolStripMenuItem.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.Space)));
         this.qUICKVIEWToolStripMenuItem.Size = new System.Drawing.Size(197, 22);
         this.qUICKVIEWToolStripMenuItem.Text = "QUICKVIEW!";
         this.qUICKVIEWToolStripMenuItem.Click += new System.EventHandler(this.QuickViewButton_Click);
         // 
         // toolStripSeparator2
         // 
         this.toolStripSeparator2.Name = "toolStripSeparator2";
         this.toolStripSeparator2.Size = new System.Drawing.Size(194, 6);
         // 
         // VisualEditorPage
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.panel1);
         this.Name = "VisualEditorPage";
         this.Size = new System.Drawing.Size(850, 589);
         this.panel1.ResumeLayout(false);
         this.panel1.PerformLayout();
         this.menuStrip1.ResumeLayout(false);
         this.menuStrip1.PerformLayout();
         this.splitContainer1.Panel1.ResumeLayout(false);
         this.splitContainer1.Panel2.ResumeLayout(false);
         this.splitContainer1.ResumeLayout(false);
         this.AnimationControlPanel.ResumeLayout(false);
         this.AnimationControlPanel.PerformLayout();
         ((System.ComponentModel.ISupportInitialize)(this.AnimationTimeTrackBar)).EndInit();
         this.panel2.ResumeLayout(false);
         this.toolStripContainer1.ResumeLayout(false);
         this.toolStripContainer1.PerformLayout();
         this.ResumeLayout(false);

      }

      #endregion

      private System.Windows.Forms.Panel panel1;
      private System.Windows.Forms.ToolStripContainer toolStripContainer1;
      private System.Windows.Forms.MenuStrip menuStrip1;
      private System.Windows.Forms.ToolStripMenuItem fileToolStripMenuItem;
      private System.Windows.Forms.TreeView VisualTreeView;
      private System.Windows.Forms.Panel panel2;
      private System.Windows.Forms.ImageList TreeViewImageList;
      private System.Windows.Forms.SplitContainer splitContainer1;
      private System.Windows.Forms.Panel panelProperty;
      private System.Windows.Forms.Button OpenButton;
      private System.Windows.Forms.Button SaveAsButton;
      private System.Windows.Forms.Button SaveButton;
      private System.Windows.Forms.Button CollapseAllButton;
      private System.Windows.Forms.Button ExpandOneLeveButton;
      private System.Windows.Forms.Button CollapseOneLevelButton;
      private System.Windows.Forms.Button ExpandAllButton;
      private System.Windows.Forms.Button NewButton;
      private System.Windows.Forms.Button UndoButton;
      private System.Windows.Forms.Button RedoButton;
      private System.Windows.Forms.ToolStripMenuItem editToolStripMenuItem;
      private System.Windows.Forms.ToolStripMenuItem undoToolStripMenuItem;
      private System.Windows.Forms.ToolStripMenuItem redoToolStripMenuItem;
      private System.Windows.Forms.ToolStripSeparator toolStripSeparator1;
      private System.Windows.Forms.ToolStripMenuItem copyToolStripMenuItem;
      private System.Windows.Forms.ToolStripMenuItem pasteToolStripMenuItem;
      private System.Windows.Forms.ToolStripMenuItem deleteToolStripMenuItem;
      private System.Windows.Forms.ToolTip toolTip1;
      private System.Windows.Forms.ToolStripMenuItem cutToolStripMenuItem;
      private System.Windows.Forms.Button PlayButton;
      private System.Windows.Forms.TrackBar AnimationTimeTrackBar;
      private System.Windows.Forms.Button ForwardButton;
      private System.Windows.Forms.Button RewindButton;
      private System.Windows.Forms.Panel AnimationControlPanel;
      private System.Windows.Forms.ImageList AnimationControlImageList;
      private System.Windows.Forms.Button RepeatButton;
      private System.Windows.Forms.Button SpeedButton;
      private System.Windows.Forms.TextBox AnimationTimeTextBox;
      private System.Windows.Forms.TextBox AnimationDurationTextBox;
      private System.Windows.Forms.ToolStripMenuItem viewToolStripMenuItem;
      private System.Windows.Forms.ToolStripMenuItem resortItemsToolStripMenuItem;
      private System.Windows.Forms.Button SoundButton;
      private System.Windows.Forms.Button MotionButton;
      private System.Windows.Forms.Button QuickViewButton;
      private System.Windows.Forms.ToolStripMenuItem qUICKVIEWToolStripMenuItem;
      private System.Windows.Forms.ToolStripSeparator toolStripSeparator2;
   }
}
