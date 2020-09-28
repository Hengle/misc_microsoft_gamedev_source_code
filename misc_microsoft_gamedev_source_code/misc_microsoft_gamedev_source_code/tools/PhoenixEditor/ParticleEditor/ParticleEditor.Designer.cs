namespace ParticleSystem
{
   partial class ParticleEditor
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
         System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(ParticleEditor));
         this.SaveAsButton = new System.Windows.Forms.Button();
         this.button1 = new System.Windows.Forms.Button();
         this.contextMenuStrip1 = new System.Windows.Forms.ContextMenuStrip(this.components);
         this.addEmitterToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.emitterToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.magnetToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.deleteToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.emitterToolStripMenuItem1 = new System.Windows.Forms.ToolStripMenuItem();
         this.editToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.emitterNameToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.panel1 = new System.Windows.Forms.Panel();
         this.toolTip1 = new System.Windows.Forms.ToolTip(this.components);
         this.button2 = new System.Windows.Forms.Button();
         this.button3 = new System.Windows.Forms.Button();
         this.CollapseAllButton = new System.Windows.Forms.Button();
         this.imageList3 = new System.Windows.Forms.ImageList(this.components);
         this.ExpandAllButton = new System.Windows.Forms.Button();
         this.CollapseOneLevelButton = new System.Windows.Forms.Button();
         this.ExpandOneLevelButton = new System.Windows.Forms.Button();
         this.QuickViewButton = new System.Windows.Forms.Button();
         this.button4 = new System.Windows.Forms.Button();
         this.treeView1 = new System.Windows.Forms.TreeView();
         this.imageList1 = new System.Windows.Forms.ImageList(this.components);
         this.menuStrip1 = new System.Windows.Forms.MenuStrip();
         this.fileToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.toolStripMenuItem1 = new System.Windows.Forms.ToolStripMenuItem();
         this.cutToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.copyToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.pasteToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.deleteToolStripMenuItem1 = new System.Windows.Forms.ToolStripMenuItem();
         this.viewToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.toolStripSeparator1 = new System.Windows.Forms.ToolStripSeparator();
         this.qUICKVIEWToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.imageList2 = new System.Windows.Forms.ImageList(this.components);
         this.contextMenuStrip1.SuspendLayout();
         this.menuStrip1.SuspendLayout();
         this.SuspendLayout();
         // 
         // SaveAsButton
         // 
         this.SaveAsButton.Image = ((System.Drawing.Image)(resources.GetObject("SaveAsButton.Image")));
         this.SaveAsButton.Location = new System.Drawing.Point(175, 3);
         this.SaveAsButton.Margin = new System.Windows.Forms.Padding(1, 3, 1, 1);
         this.SaveAsButton.Name = "SaveAsButton";
         this.SaveAsButton.Size = new System.Drawing.Size(41, 40);
         this.SaveAsButton.TabIndex = 5;
         this.toolTip1.SetToolTip(this.SaveAsButton, "Save As");
         this.SaveAsButton.UseVisualStyleBackColor = true;
         this.SaveAsButton.Click += new System.EventHandler(this.SaveAsButton_Click);
         // 
         // button1
         // 
         this.button1.Image = ((System.Drawing.Image)(resources.GetObject("button1.Image")));
         this.button1.Location = new System.Drawing.Point(89, 3);
         this.button1.Margin = new System.Windows.Forms.Padding(1, 3, 1, 1);
         this.button1.Name = "button1";
         this.button1.Size = new System.Drawing.Size(41, 40);
         this.button1.TabIndex = 3;
         this.toolTip1.SetToolTip(this.button1, "Open File");
         this.button1.UseVisualStyleBackColor = true;
         this.button1.Click += new System.EventHandler(this.button1_Click);
         // 
         // contextMenuStrip1
         // 
         this.contextMenuStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.addEmitterToolStripMenuItem,
            this.deleteToolStripMenuItem,
            this.editToolStripMenuItem});
         this.contextMenuStrip1.Name = "contextMenuStrip1";
         this.contextMenuStrip1.Size = new System.Drawing.Size(117, 70);
         // 
         // addEmitterToolStripMenuItem
         // 
         this.addEmitterToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.emitterToolStripMenuItem,
            this.magnetToolStripMenuItem});
         this.addEmitterToolStripMenuItem.Image = ((System.Drawing.Image)(resources.GetObject("addEmitterToolStripMenuItem.Image")));
         this.addEmitterToolStripMenuItem.Name = "addEmitterToolStripMenuItem";
         this.addEmitterToolStripMenuItem.Size = new System.Drawing.Size(116, 22);
         this.addEmitterToolStripMenuItem.Text = "New";
         // 
         // emitterToolStripMenuItem
         // 
         this.emitterToolStripMenuItem.Name = "emitterToolStripMenuItem";
         this.emitterToolStripMenuItem.Size = new System.Drawing.Size(121, 22);
         this.emitterToolStripMenuItem.Text = "Emitter";
         this.emitterToolStripMenuItem.Click += new System.EventHandler(this.emitterToolStripMenuItem_Click);
         // 
         // magnetToolStripMenuItem
         // 
         this.magnetToolStripMenuItem.Name = "magnetToolStripMenuItem";
         this.magnetToolStripMenuItem.Size = new System.Drawing.Size(121, 22);
         this.magnetToolStripMenuItem.Text = "Magnet";
         this.magnetToolStripMenuItem.Click += new System.EventHandler(this.magnetToolStripMenuItem_Click);
         // 
         // deleteToolStripMenuItem
         // 
         this.deleteToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.emitterToolStripMenuItem1});
         this.deleteToolStripMenuItem.Image = ((System.Drawing.Image)(resources.GetObject("deleteToolStripMenuItem.Image")));
         this.deleteToolStripMenuItem.Name = "deleteToolStripMenuItem";
         this.deleteToolStripMenuItem.Size = new System.Drawing.Size(116, 22);
         this.deleteToolStripMenuItem.Text = "Delete";
         // 
         // emitterToolStripMenuItem1
         // 
         this.emitterToolStripMenuItem1.Name = "emitterToolStripMenuItem1";
         this.emitterToolStripMenuItem1.Size = new System.Drawing.Size(119, 22);
         this.emitterToolStripMenuItem1.Text = "Emitter";
         this.emitterToolStripMenuItem1.Click += new System.EventHandler(this.emitterToolStripMenuItem1_Click);
         // 
         // editToolStripMenuItem
         // 
         this.editToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.emitterNameToolStripMenuItem});
         this.editToolStripMenuItem.Name = "editToolStripMenuItem";
         this.editToolStripMenuItem.Size = new System.Drawing.Size(116, 22);
         this.editToolStripMenuItem.Text = "Edit";
         // 
         // emitterNameToolStripMenuItem
         // 
         this.emitterNameToolStripMenuItem.Name = "emitterNameToolStripMenuItem";
         this.emitterNameToolStripMenuItem.Size = new System.Drawing.Size(149, 22);
         this.emitterNameToolStripMenuItem.Text = "Emitter Name";
         // 
         // panel1
         // 
         this.panel1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.panel1.BackgroundImageLayout = System.Windows.Forms.ImageLayout.Stretch;
         this.panel1.Location = new System.Drawing.Point(263, 3);
         this.panel1.Name = "panel1";
         this.panel1.Size = new System.Drawing.Size(860, 643);
         this.panel1.TabIndex = 11;
         // 
         // button2
         // 
         this.button2.Image = ((System.Drawing.Image)(resources.GetObject("button2.Image")));
         this.button2.Location = new System.Drawing.Point(3, 3);
         this.button2.Margin = new System.Windows.Forms.Padding(3, 3, 1, 1);
         this.button2.Name = "button2";
         this.button2.Size = new System.Drawing.Size(41, 40);
         this.button2.TabIndex = 1;
         this.toolTip1.SetToolTip(this.button2, "Creates a new particle effect.");
         this.button2.UseVisualStyleBackColor = true;
         this.button2.Click += new System.EventHandler(this.button2_Click);
         // 
         // button3
         // 
         this.button3.BackColor = System.Drawing.SystemColors.Control;
         this.button3.Image = ((System.Drawing.Image)(resources.GetObject("button3.Image")));
         this.button3.Location = new System.Drawing.Point(46, 3);
         this.button3.Margin = new System.Windows.Forms.Padding(1, 3, 1, 1);
         this.button3.Name = "button3";
         this.button3.Size = new System.Drawing.Size(41, 40);
         this.button3.TabIndex = 2;
         this.toolTip1.SetToolTip(this.button3, "Adds a new Emitter to the current particle effect");
         this.button3.UseVisualStyleBackColor = false;
         this.button3.Click += new System.EventHandler(this.button3_Click);
         // 
         // CollapseAllButton
         // 
         this.CollapseAllButton.ImageIndex = 0;
         this.CollapseAllButton.ImageList = this.imageList3;
         this.CollapseAllButton.Location = new System.Drawing.Point(157, 45);
         this.CollapseAllButton.Margin = new System.Windows.Forms.Padding(1);
         this.CollapseAllButton.Name = "CollapseAllButton";
         this.CollapseAllButton.Size = new System.Drawing.Size(24, 24);
         this.CollapseAllButton.TabIndex = 7;
         this.CollapseAllButton.TabStop = false;
         this.toolTip1.SetToolTip(this.CollapseAllButton, "Collapse All");
         this.CollapseAllButton.UseVisualStyleBackColor = true;
         this.CollapseAllButton.Click += new System.EventHandler(this.CollapseAllButton_Click);
         // 
         // imageList3
         // 
         this.imageList3.ImageStream = ((System.Windows.Forms.ImageListStreamer)(resources.GetObject("imageList3.ImageStream")));
         this.imageList3.TransparentColor = System.Drawing.Color.Transparent;
         this.imageList3.Images.SetKeyName(0, "VisualEditor_CollapseAll.png");
         this.imageList3.Images.SetKeyName(1, "VisualEditor_ExpandAll.png");
         this.imageList3.Images.SetKeyName(2, "VisualEditor_CollapseOneLevel.png");
         this.imageList3.Images.SetKeyName(3, "VisualEditor_ExpandOneLevel.png");
         // 
         // ExpandAllButton
         // 
         this.ExpandAllButton.ImageIndex = 1;
         this.ExpandAllButton.ImageList = this.imageList3;
         this.ExpandAllButton.Location = new System.Drawing.Point(183, 45);
         this.ExpandAllButton.Margin = new System.Windows.Forms.Padding(1);
         this.ExpandAllButton.Name = "ExpandAllButton";
         this.ExpandAllButton.Size = new System.Drawing.Size(24, 24);
         this.ExpandAllButton.TabIndex = 8;
         this.ExpandAllButton.TabStop = false;
         this.toolTip1.SetToolTip(this.ExpandAllButton, "Expand All");
         this.ExpandAllButton.UseVisualStyleBackColor = true;
         this.ExpandAllButton.Click += new System.EventHandler(this.ExpandAllButton_Click);
         // 
         // CollapseOneLevelButton
         // 
         this.CollapseOneLevelButton.ImageIndex = 2;
         this.CollapseOneLevelButton.ImageList = this.imageList3;
         this.CollapseOneLevelButton.Location = new System.Drawing.Point(209, 45);
         this.CollapseOneLevelButton.Margin = new System.Windows.Forms.Padding(1);
         this.CollapseOneLevelButton.Name = "CollapseOneLevelButton";
         this.CollapseOneLevelButton.Size = new System.Drawing.Size(24, 24);
         this.CollapseOneLevelButton.TabIndex = 9;
         this.CollapseOneLevelButton.TabStop = false;
         this.toolTip1.SetToolTip(this.CollapseOneLevelButton, "Collapse One Level");
         this.CollapseOneLevelButton.UseVisualStyleBackColor = true;
         this.CollapseOneLevelButton.Click += new System.EventHandler(this.CollapseOneLevelButton_Click);
         // 
         // ExpandOneLevelButton
         // 
         this.ExpandOneLevelButton.ImageIndex = 3;
         this.ExpandOneLevelButton.ImageList = this.imageList3;
         this.ExpandOneLevelButton.Location = new System.Drawing.Point(235, 45);
         this.ExpandOneLevelButton.Margin = new System.Windows.Forms.Padding(1);
         this.ExpandOneLevelButton.Name = "ExpandOneLevelButton";
         this.ExpandOneLevelButton.Size = new System.Drawing.Size(24, 24);
         this.ExpandOneLevelButton.TabIndex = 10;
         this.ExpandOneLevelButton.TabStop = false;
         this.toolTip1.SetToolTip(this.ExpandOneLevelButton, "Expand One Level");
         this.ExpandOneLevelButton.UseVisualStyleBackColor = true;
         this.ExpandOneLevelButton.Click += new System.EventHandler(this.ExpandOneLevelButton_Click);
         // 
         // QuickViewButton
         // 
         this.QuickViewButton.Image = ((System.Drawing.Image)(resources.GetObject("QuickViewButton.Image")));
         this.QuickViewButton.Location = new System.Drawing.Point(218, 3);
         this.QuickViewButton.Margin = new System.Windows.Forms.Padding(1, 3, 1, 1);
         this.QuickViewButton.Name = "QuickViewButton";
         this.QuickViewButton.Size = new System.Drawing.Size(41, 40);
         this.QuickViewButton.TabIndex = 6;
         this.toolTip1.SetToolTip(this.QuickViewButton, "QUICKVIEW!");
         this.QuickViewButton.UseVisualStyleBackColor = true;
         this.QuickViewButton.Click += new System.EventHandler(this.QuickViewButton_Click);
         // 
         // button4
         // 
         this.button4.Image = ((System.Drawing.Image)(resources.GetObject("button4.Image")));
         this.button4.Location = new System.Drawing.Point(132, 3);
         this.button4.Margin = new System.Windows.Forms.Padding(1, 3, 1, 1);
         this.button4.Name = "button4";
         this.button4.Size = new System.Drawing.Size(41, 40);
         this.button4.TabIndex = 4;
         this.toolTip1.SetToolTip(this.button4, "Save");
         this.button4.UseVisualStyleBackColor = true;
         this.button4.Click += new System.EventHandler(this.button4_Click);
         // 
         // treeView1
         // 
         this.treeView1.AllowDrop = true;
         this.treeView1.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)));
         this.treeView1.BackColor = System.Drawing.Color.White;
         this.treeView1.Font = new System.Drawing.Font("Arial Black", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
         this.treeView1.ForeColor = System.Drawing.Color.MidnightBlue;
         this.treeView1.HideSelection = false;
         this.treeView1.ImageIndex = 0;
         this.treeView1.ImageList = this.imageList1;
         this.treeView1.Location = new System.Drawing.Point(3, 73);
         this.treeView1.Name = "treeView1";
         this.treeView1.SelectedImageIndex = 0;
         this.treeView1.Size = new System.Drawing.Size(256, 565);
         this.treeView1.TabIndex = 0;
         this.treeView1.TabStop = false;
         this.treeView1.NodeMouseDoubleClick += new System.Windows.Forms.TreeNodeMouseClickEventHandler(this.treeView1_NodeMouseDoubleClick);
         this.treeView1.DragDrop += new System.Windows.Forms.DragEventHandler(this.treeView1_DragDrop);
         this.treeView1.AfterSelect += new System.Windows.Forms.TreeViewEventHandler(this.treeView1_AfterSelect);
         this.treeView1.MouseDown += new System.Windows.Forms.MouseEventHandler(this.treeView1_MouseDown);
         this.treeView1.NodeMouseClick += new System.Windows.Forms.TreeNodeMouseClickEventHandler(this.treeView1_NodeMouseClick);
         this.treeView1.DragOver += new System.Windows.Forms.DragEventHandler(this.treeView1_DragOver);
         // 
         // imageList1
         // 
         this.imageList1.ImageStream = ((System.Windows.Forms.ImageListStreamer)(resources.GetObject("imageList1.ImageStream")));
         this.imageList1.TransparentColor = System.Drawing.Color.Black;
         this.imageList1.Images.SetKeyName(0, "particles.bmp");
         this.imageList1.Images.SetKeyName(1, "emitter.bmp");
         this.imageList1.Images.SetKeyName(2, "properties.bmp");
         this.imageList1.Images.SetKeyName(3, "shape.bmp");
         this.imageList1.Images.SetKeyName(4, "texture.bmp");
         this.imageList1.Images.SetKeyName(5, "scale.bmp");
         this.imageList1.Images.SetKeyName(6, "color.bmp");
         this.imageList1.Images.SetKeyName(7, "rotation.bmp");
         this.imageList1.Images.SetKeyName(8, "opacity.bmp");
         this.imageList1.Images.SetKeyName(9, "speed.bmp");
         this.imageList1.Images.SetKeyName(10, "intensity.bmp");
         this.imageList1.Images.SetKeyName(11, "attractor.bmp");
         // 
         // menuStrip1
         // 
         this.menuStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.fileToolStripMenuItem,
            this.toolStripMenuItem1,
            this.viewToolStripMenuItem});
         this.menuStrip1.Location = new System.Drawing.Point(0, 0);
         this.menuStrip1.Name = "menuStrip1";
         this.menuStrip1.Size = new System.Drawing.Size(1123, 24);
         this.menuStrip1.TabIndex = 13;
         this.menuStrip1.Text = "menuStrip1";
         this.menuStrip1.Visible = false;
         // 
         // fileToolStripMenuItem
         // 
         this.fileToolStripMenuItem.Name = "fileToolStripMenuItem";
         this.fileToolStripMenuItem.Size = new System.Drawing.Size(35, 20);
         this.fileToolStripMenuItem.Text = "File";
         // 
         // toolStripMenuItem1
         // 
         this.toolStripMenuItem1.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.cutToolStripMenuItem,
            this.copyToolStripMenuItem,
            this.pasteToolStripMenuItem,
            this.deleteToolStripMenuItem1});
         this.toolStripMenuItem1.Name = "toolStripMenuItem1";
         this.toolStripMenuItem1.Size = new System.Drawing.Size(37, 20);
         this.toolStripMenuItem1.Text = "Edit";
         // 
         // cutToolStripMenuItem
         // 
         this.cutToolStripMenuItem.Image = ((System.Drawing.Image)(resources.GetObject("cutToolStripMenuItem.Image")));
         this.cutToolStripMenuItem.Name = "cutToolStripMenuItem";
         this.cutToolStripMenuItem.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.X)));
         this.cutToolStripMenuItem.Size = new System.Drawing.Size(150, 22);
         this.cutToolStripMenuItem.Text = "Cut";
         this.cutToolStripMenuItem.Click += new System.EventHandler(this.MenuItemCutEmitter_Click);
         // 
         // copyToolStripMenuItem
         // 
         this.copyToolStripMenuItem.Image = ((System.Drawing.Image)(resources.GetObject("copyToolStripMenuItem.Image")));
         this.copyToolStripMenuItem.Name = "copyToolStripMenuItem";
         this.copyToolStripMenuItem.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.C)));
         this.copyToolStripMenuItem.Size = new System.Drawing.Size(150, 22);
         this.copyToolStripMenuItem.Text = "Copy";
         this.copyToolStripMenuItem.Click += new System.EventHandler(this.MenuItemCopyEmitter_Click);
         // 
         // pasteToolStripMenuItem
         // 
         this.pasteToolStripMenuItem.Image = ((System.Drawing.Image)(resources.GetObject("pasteToolStripMenuItem.Image")));
         this.pasteToolStripMenuItem.Name = "pasteToolStripMenuItem";
         this.pasteToolStripMenuItem.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.V)));
         this.pasteToolStripMenuItem.Size = new System.Drawing.Size(150, 22);
         this.pasteToolStripMenuItem.Text = "Paste";
         this.pasteToolStripMenuItem.Click += new System.EventHandler(this.MenuItemPasteEmitter_Click);
         // 
         // deleteToolStripMenuItem1
         // 
         this.deleteToolStripMenuItem1.Image = ((System.Drawing.Image)(resources.GetObject("deleteToolStripMenuItem1.Image")));
         this.deleteToolStripMenuItem1.Name = "deleteToolStripMenuItem1";
         this.deleteToolStripMenuItem1.ShortcutKeys = System.Windows.Forms.Keys.Delete;
         this.deleteToolStripMenuItem1.Size = new System.Drawing.Size(150, 22);
         this.deleteToolStripMenuItem1.Text = "Delete";
         this.deleteToolStripMenuItem1.Click += new System.EventHandler(this.MenuItemDeleteEmitter_Click);
         // 
         // viewToolStripMenuItem
         // 
         this.viewToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.toolStripSeparator1,
            this.qUICKVIEWToolStripMenuItem});
         this.viewToolStripMenuItem.Name = "viewToolStripMenuItem";
         this.viewToolStripMenuItem.Size = new System.Drawing.Size(41, 20);
         this.viewToolStripMenuItem.Text = "View";
         // 
         // toolStripSeparator1
         // 
         this.toolStripSeparator1.Name = "toolStripSeparator1";
         this.toolStripSeparator1.Size = new System.Drawing.Size(205, 6);
         // 
         // qUICKVIEWToolStripMenuItem
         // 
         this.qUICKVIEWToolStripMenuItem.Name = "qUICKVIEWToolStripMenuItem";
         this.qUICKVIEWToolStripMenuItem.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.Space)));
         this.qUICKVIEWToolStripMenuItem.Size = new System.Drawing.Size(208, 22);
         this.qUICKVIEWToolStripMenuItem.Text = "QUICKVIEW!";
         this.qUICKVIEWToolStripMenuItem.Click += new System.EventHandler(this.QuickViewButton_Click);
         // 
         // imageList2
         // 
         this.imageList2.ImageStream = ((System.Windows.Forms.ImageListStreamer)(resources.GetObject("imageList2.ImageStream")));
         this.imageList2.TransparentColor = System.Drawing.Color.Transparent;
         this.imageList2.Images.SetKeyName(0, "add.bmp");
         this.imageList2.Images.SetKeyName(1, "del.bmp");
         this.imageList2.Images.SetKeyName(2, "CutHS.png");
         this.imageList2.Images.SetKeyName(3, "CopyHS.png");
         this.imageList2.Images.SetKeyName(4, "PasteHS.png");
         // 
         // ParticleEditor
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.menuStrip1);
         this.Controls.Add(this.ExpandOneLevelButton);
         this.Controls.Add(this.QuickViewButton);
         this.Controls.Add(this.button4);
         this.Controls.Add(this.CollapseOneLevelButton);
         this.Controls.Add(this.button3);
         this.Controls.Add(this.ExpandAllButton);
         this.Controls.Add(this.CollapseAllButton);
         this.Controls.Add(this.button2);
         this.Controls.Add(this.treeView1);
         this.Controls.Add(this.panel1);
         this.Controls.Add(this.button1);
         this.Controls.Add(this.SaveAsButton);
         this.Name = "ParticleEditor";
         this.Size = new System.Drawing.Size(1123, 649);
         this.contextMenuStrip1.ResumeLayout(false);
         this.menuStrip1.ResumeLayout(false);
         this.menuStrip1.PerformLayout();
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private System.Windows.Forms.Button SaveAsButton;
      private System.Windows.Forms.Button button1;
      private System.Windows.Forms.ContextMenuStrip contextMenuStrip1;
      private System.Windows.Forms.ToolStripMenuItem addEmitterToolStripMenuItem;
      private System.Windows.Forms.ToolStripMenuItem emitterToolStripMenuItem;
      private System.Windows.Forms.ToolStripMenuItem deleteToolStripMenuItem;
      private System.Windows.Forms.ToolStripMenuItem emitterToolStripMenuItem1;
      private System.Windows.Forms.Panel panel1;
      private System.Windows.Forms.ToolTip toolTip1;
      private System.Windows.Forms.ToolStripMenuItem magnetToolStripMenuItem;
      private System.Windows.Forms.TreeView treeView1;
      private System.Windows.Forms.ImageList imageList1;
      private System.Windows.Forms.Button button2;
      private System.Windows.Forms.Button button3;
      private System.Windows.Forms.Button button4;
      private System.Windows.Forms.ToolStripMenuItem editToolStripMenuItem;
      private System.Windows.Forms.ToolStripMenuItem emitterNameToolStripMenuItem;
      private System.Windows.Forms.MenuStrip menuStrip1;
      private System.Windows.Forms.ToolStripMenuItem fileToolStripMenuItem;
      private System.Windows.Forms.ToolStripMenuItem toolStripMenuItem1;
      private System.Windows.Forms.ToolStripMenuItem cutToolStripMenuItem;
      private System.Windows.Forms.ToolStripMenuItem copyToolStripMenuItem;
      private System.Windows.Forms.ToolStripMenuItem pasteToolStripMenuItem;
      private System.Windows.Forms.ToolStripMenuItem deleteToolStripMenuItem1;
      private System.Windows.Forms.ImageList imageList2;
      private System.Windows.Forms.Button CollapseAllButton;
      private System.Windows.Forms.Button ExpandAllButton;
      private System.Windows.Forms.Button CollapseOneLevelButton;
      private System.Windows.Forms.Button ExpandOneLevelButton;
      private System.Windows.Forms.ImageList imageList3;
      private System.Windows.Forms.Button QuickViewButton;
      private System.Windows.Forms.ToolStripMenuItem viewToolStripMenuItem;
      private System.Windows.Forms.ToolStripSeparator toolStripSeparator1;
      private System.Windows.Forms.ToolStripMenuItem qUICKVIEWToolStripMenuItem;

   }
}
