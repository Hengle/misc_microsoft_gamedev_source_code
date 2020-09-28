namespace PhoenixEditor
{
   partial class MainWindow
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

      #region Windows Form Designer generated code

      /// <summary>
      /// Required method for Designer support - do not modify
      /// the contents of this method with the code editor.
      /// </summary>
      private void InitializeComponent()
      {
         this.components = new System.ComponentModel.Container();
         this.toolStripContainer1 = new System.Windows.Forms.ToolStripContainer();
         this.statusStrip1 = new System.Windows.Forms.StatusStrip();
         this.toolStripProgressBar = new EditorCore.XpToolStripProgressBar();
         this.LastErrorToolStripStatusLabel = new System.Windows.Forms.ToolStripStatusLabel();
         this.panel2 = new System.Windows.Forms.Panel();
         this.closeActiveTab = new System.Windows.Forms.Button();
         this.ClientTabControl = new System.Windows.Forms.TabControl();
         this.menuStrip1 = new System.Windows.Forms.MenuStrip();
         this.fileToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.file_open = new System.Windows.Forms.ToolStripMenuItem();
         this.openScenarioToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.importGR2ToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.importSoundsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.toolStripMenuItem7 = new System.Windows.Forms.ToolStripSeparator();
         this.file_save = new System.Windows.Forms.ToolStripMenuItem();
         this.file_saveas = new System.Windows.Forms.ToolStripMenuItem();
         this.toolStripMenuItem8 = new System.Windows.Forms.ToolStripSeparator();
         this.exitToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.editorToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.viewToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.optionsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.errorListToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.protoObjectsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.toolStripMenuItem3 = new System.Windows.Forms.ToolStripMenuItem();
         this.aboutToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.SimToolStrip = new System.Windows.Forms.ToolStrip();
         this.TerrainToolStrip = new System.Windows.Forms.ToolStrip();
         this.mTexturePalette = new PhoenixEditor.TexturePalette();
         this.mMaskPalette = new PhoenixEditor.TexturePalette();
         this.timer1 = new System.Windows.Forms.Timer(this.components);
         this.soundTimer = new System.Windows.Forms.Timer(this.components);
         this.mDockLayoutManager = new Xceed.DockingWindows.DockLayoutManager();
         this.cursorAtLocLabel = new System.Windows.Forms.ToolStripStatusLabel();
         this.toolStripContainer1.BottomToolStripPanel.SuspendLayout();
         this.toolStripContainer1.ContentPanel.SuspendLayout();
         this.toolStripContainer1.TopToolStripPanel.SuspendLayout();
         this.toolStripContainer1.SuspendLayout();
         this.statusStrip1.SuspendLayout();
         this.panel2.SuspendLayout();
         this.menuStrip1.SuspendLayout();
         ((System.ComponentModel.ISupportInitialize)(this.mDockLayoutManager)).BeginInit();
         this.SuspendLayout();
         // 
         // toolStripContainer1
         // 
         // 
         // toolStripContainer1.BottomToolStripPanel
         // 
         this.toolStripContainer1.BottomToolStripPanel.Controls.Add(this.statusStrip1);
         // 
         // toolStripContainer1.ContentPanel
         // 
         this.toolStripContainer1.ContentPanel.Controls.Add(this.panel2);
         this.toolStripContainer1.ContentPanel.Size = new System.Drawing.Size(1121, 700);
         this.toolStripContainer1.Dock = System.Windows.Forms.DockStyle.Fill;
         this.toolStripContainer1.Location = new System.Drawing.Point(0, 0);
         this.toolStripContainer1.Name = "toolStripContainer1";
         this.toolStripContainer1.Size = new System.Drawing.Size(1121, 746);
         this.toolStripContainer1.TabIndex = 3;
         this.toolStripContainer1.Text = "toolStripContainer1";
         // 
         // toolStripContainer1.TopToolStripPanel
         // 
         this.toolStripContainer1.TopToolStripPanel.Controls.Add(this.menuStrip1);
         this.toolStripContainer1.TopToolStripPanel.Controls.Add(this.TerrainToolStrip);
         this.toolStripContainer1.TopToolStripPanel.Controls.Add(this.SimToolStrip);
         this.toolStripContainer1.TopToolStripPanel.Controls.Add(this.mMaskPalette);
         this.toolStripContainer1.TopToolStripPanel.Controls.Add(this.mTexturePalette);
         // 
         // statusStrip1
         // 
         this.statusStrip1.Dock = System.Windows.Forms.DockStyle.None;
         this.statusStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.toolStripProgressBar,
            this.cursorAtLocLabel,
            this.LastErrorToolStripStatusLabel});
         this.statusStrip1.Location = new System.Drawing.Point(0, 0);
         this.statusStrip1.Name = "statusStrip1";
         this.statusStrip1.Size = new System.Drawing.Size(1121, 22);
         this.statusStrip1.TabIndex = 5;
         this.statusStrip1.Text = "statusStrip1";
         // 
         // toolStripProgressBar
         // 
         this.toolStripProgressBar.Name = "toolStripProgressBar";
         this.toolStripProgressBar.Size = new System.Drawing.Size(300, 20);
         this.toolStripProgressBar.Step = 1;
         this.toolStripProgressBar.DoubleClick += new System.EventHandler(this.toolStripProgressBar_DoubleClick);
         // 
         // LastErrorToolStripStatusLabel
         // 
         this.LastErrorToolStripStatusLabel.Name = "LastErrorToolStripStatusLabel";
         this.LastErrorToolStripStatusLabel.Size = new System.Drawing.Size(16, 17);
         this.LastErrorToolStripStatusLabel.Text = "   ";
         this.LastErrorToolStripStatusLabel.Click += new System.EventHandler(this.LastErrorToolStripStatusLabel_Click);
         // 
         // panel2
         // 
         this.panel2.Controls.Add(this.closeActiveTab);
         this.panel2.Controls.Add(this.ClientTabControl);
         this.panel2.Location = new System.Drawing.Point(201, 76);
         this.panel2.Name = "panel2";
         this.panel2.Size = new System.Drawing.Size(683, 461);
         this.panel2.TabIndex = 4;
         // 
         // closeActiveTab
         // 
         this.closeActiveTab.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
         this.closeActiveTab.FlatAppearance.BorderSize = 0;
         this.closeActiveTab.FlatAppearance.MouseDownBackColor = System.Drawing.SystemColors.ControlLight;
         this.closeActiveTab.FlatAppearance.MouseOverBackColor = System.Drawing.SystemColors.Control;
         this.closeActiveTab.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
         this.closeActiveTab.Font = new System.Drawing.Font("Arial", 9.75F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
         this.closeActiveTab.Location = new System.Drawing.Point(659, 0);
         this.closeActiveTab.Name = "closeActiveTab";
         this.closeActiveTab.Size = new System.Drawing.Size(21, 21);
         this.closeActiveTab.TabIndex = 5;
         this.closeActiveTab.Text = "X";
         this.closeActiveTab.TextAlign = System.Drawing.ContentAlignment.TopCenter;
         this.closeActiveTab.UseVisualStyleBackColor = true;
         this.closeActiveTab.Click += new System.EventHandler(this.closeActiveTab_Click);
         // 
         // ClientTabControl
         // 
         this.ClientTabControl.Dock = System.Windows.Forms.DockStyle.Fill;
         this.ClientTabControl.Location = new System.Drawing.Point(0, 0);
         this.ClientTabControl.Name = "ClientTabControl";
         this.ClientTabControl.SelectedIndex = 0;
         this.ClientTabControl.Size = new System.Drawing.Size(683, 461);
         this.ClientTabControl.TabIndex = 2;
         this.ClientTabControl.Selected += new System.Windows.Forms.TabControlEventHandler(this.ClientTabControl_Selected);
         this.ClientTabControl.ControlAdded += new System.Windows.Forms.ControlEventHandler(this.ClientTabControl_ControlAdded);
         // 
         // menuStrip1
         // 
         this.menuStrip1.Dock = System.Windows.Forms.DockStyle.None;
         this.menuStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.fileToolStripMenuItem,
            this.editorToolStripMenuItem,
            this.viewToolStripMenuItem,
            this.toolStripMenuItem3});
         this.menuStrip1.Location = new System.Drawing.Point(0, 0);
         this.menuStrip1.Name = "menuStrip1";
         this.menuStrip1.Size = new System.Drawing.Size(1121, 24);
         this.menuStrip1.TabIndex = 5;
         this.menuStrip1.Text = "menuStrip1";
         // 
         // fileToolStripMenuItem
         // 
         this.fileToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.file_open,
            this.openScenarioToolStripMenuItem,
            this.importGR2ToolStripMenuItem,
            this.importSoundsToolStripMenuItem,
            this.toolStripMenuItem7,
            this.file_save,
            this.file_saveas,
            this.toolStripMenuItem8,
            this.exitToolStripMenuItem});
         this.fileToolStripMenuItem.Name = "fileToolStripMenuItem";
         this.fileToolStripMenuItem.Size = new System.Drawing.Size(35, 20);
         this.fileToolStripMenuItem.Text = "File";
         // 
         // file_open
         // 
         this.file_open.Name = "file_open";
         this.file_open.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.O)));
         this.file_open.Size = new System.Drawing.Size(167, 22);
         this.file_open.Text = "Open...";
         this.file_open.Click += new System.EventHandler(this.file_open_Click);
         // 
         // openScenarioToolStripMenuItem
         // 
         this.openScenarioToolStripMenuItem.Name = "openScenarioToolStripMenuItem";
         this.openScenarioToolStripMenuItem.Size = new System.Drawing.Size(167, 22);
         this.openScenarioToolStripMenuItem.Text = "Open Scenario...";
         this.openScenarioToolStripMenuItem.Click += new System.EventHandler(this.openScenarioToolStripMenuItem_Click);
         // 
         // importGR2ToolStripMenuItem
         // 
         this.importGR2ToolStripMenuItem.Name = "importGR2ToolStripMenuItem";
         this.importGR2ToolStripMenuItem.Size = new System.Drawing.Size(167, 22);
         this.importGR2ToolStripMenuItem.Text = "Import GR2";
         this.importGR2ToolStripMenuItem.Click += new System.EventHandler(this.importGR2ToolStripMenuItem_Click);
         // 
         // importSoundsToolStripMenuItem
         // 
         this.importSoundsToolStripMenuItem.Name = "importSoundsToolStripMenuItem";
         this.importSoundsToolStripMenuItem.Size = new System.Drawing.Size(167, 22);
         this.importSoundsToolStripMenuItem.Text = "Import Sounds";
         this.importSoundsToolStripMenuItem.Click += new System.EventHandler(this.importSoundsToolStripMenuItem_Click);
         // 
         // toolStripMenuItem7
         // 
         this.toolStripMenuItem7.Name = "toolStripMenuItem7";
         this.toolStripMenuItem7.Size = new System.Drawing.Size(164, 6);
         // 
         // file_save
         // 
         this.file_save.Name = "file_save";
         this.file_save.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.S)));
         this.file_save.Size = new System.Drawing.Size(167, 22);
         this.file_save.Text = "Save";
         this.file_save.Click += new System.EventHandler(this.file_save_Click);
         // 
         // file_saveas
         // 
         this.file_saveas.Name = "file_saveas";
         this.file_saveas.Size = new System.Drawing.Size(167, 22);
         this.file_saveas.Text = "Save As";
         this.file_saveas.Click += new System.EventHandler(this.file_saveas_Click);
         // 
         // toolStripMenuItem8
         // 
         this.toolStripMenuItem8.Name = "toolStripMenuItem8";
         this.toolStripMenuItem8.Size = new System.Drawing.Size(164, 6);
         // 
         // exitToolStripMenuItem
         // 
         this.exitToolStripMenuItem.Name = "exitToolStripMenuItem";
         this.exitToolStripMenuItem.Size = new System.Drawing.Size(167, 22);
         this.exitToolStripMenuItem.Text = "Exit";
         // 
         // editorToolStripMenuItem
         // 
         this.editorToolStripMenuItem.Name = "editorToolStripMenuItem";
         this.editorToolStripMenuItem.Size = new System.Drawing.Size(37, 20);
         this.editorToolStripMenuItem.Text = "Edit";
         // 
         // viewToolStripMenuItem
         // 
         this.viewToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.optionsToolStripMenuItem,
            this.errorListToolStripMenuItem,
            this.protoObjectsToolStripMenuItem});
         this.viewToolStripMenuItem.Name = "viewToolStripMenuItem";
         this.viewToolStripMenuItem.Size = new System.Drawing.Size(41, 20);
         this.viewToolStripMenuItem.Text = "View";
         // 
         // optionsToolStripMenuItem
         // 
         this.optionsToolStripMenuItem.Name = "optionsToolStripMenuItem";
         this.optionsToolStripMenuItem.Size = new System.Drawing.Size(182, 22);
         this.optionsToolStripMenuItem.Text = "Options";
         this.optionsToolStripMenuItem.Click += new System.EventHandler(this.optionsToolStripMenuItem_Click_1);
         // 
         // errorListToolStripMenuItem
         // 
         this.errorListToolStripMenuItem.Name = "errorListToolStripMenuItem";
         this.errorListToolStripMenuItem.Size = new System.Drawing.Size(182, 22);
         this.errorListToolStripMenuItem.Text = "Error List";
         this.errorListToolStripMenuItem.Click += new System.EventHandler(this.errorListToolStripMenuItem_Click);
         // 
         // protoObjectsToolStripMenuItem
         // 
         this.protoObjectsToolStripMenuItem.Enabled = false;
         this.protoObjectsToolStripMenuItem.Name = "protoObjectsToolStripMenuItem";
         this.protoObjectsToolStripMenuItem.Size = new System.Drawing.Size(182, 22);
         this.protoObjectsToolStripMenuItem.Text = "Proto Object Wizard";
         this.protoObjectsToolStripMenuItem.Click += new System.EventHandler(this.protoObjectsToolStripMenuItem_Click);
         // 
         // toolStripMenuItem3
         // 
         this.toolStripMenuItem3.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.aboutToolStripMenuItem});
         this.toolStripMenuItem3.Name = "toolStripMenuItem3";
         this.toolStripMenuItem3.Size = new System.Drawing.Size(40, 20);
         this.toolStripMenuItem3.Text = "Help";
         // 
         // aboutToolStripMenuItem
         // 
         this.aboutToolStripMenuItem.Name = "aboutToolStripMenuItem";
         this.aboutToolStripMenuItem.Size = new System.Drawing.Size(114, 22);
         this.aboutToolStripMenuItem.Text = "About";
         this.aboutToolStripMenuItem.Click += new System.EventHandler(this.aboutToolStripMenuItem_Click);
         // 
         // SimToolStrip
         // 
         this.SimToolStrip.Dock = System.Windows.Forms.DockStyle.None;
         this.SimToolStrip.Location = new System.Drawing.Point(3, 49);
         this.SimToolStrip.Name = "SimToolStrip";
         this.SimToolStrip.Size = new System.Drawing.Size(111, 25);
         this.SimToolStrip.TabIndex = 6;
         this.SimToolStrip.Visible = false;
         // 
         // TerrainToolStrip
         // 
         this.TerrainToolStrip.Dock = System.Windows.Forms.DockStyle.None;
         this.TerrainToolStrip.LayoutStyle = System.Windows.Forms.ToolStripLayoutStyle.HorizontalStackWithOverflow;
         this.TerrainToolStrip.Location = new System.Drawing.Point(3, 49);
         this.TerrainToolStrip.Name = "TerrainToolStrip";
         this.TerrainToolStrip.Size = new System.Drawing.Size(111, 25);
         this.TerrainToolStrip.TabIndex = 1;
         this.TerrainToolStrip.Visible = false;
         // 
         // mTexturePalette
         // 
         this.mTexturePalette.Dock = System.Windows.Forms.DockStyle.None;
         this.mTexturePalette.Location = new System.Drawing.Point(100, 49);
         this.mTexturePalette.MaximumSize = new System.Drawing.Size(50, 25);
         this.mTexturePalette.Name = "mTexturePalette";
         this.mTexturePalette.SelectedTexture = 0;
         this.mTexturePalette.Size = new System.Drawing.Size(50, 25);
         this.mTexturePalette.TabIndex = 2;
         this.mTexturePalette.Visible = false;
         // 
         // mMaskPalette
         // 
         this.mMaskPalette.Dock = System.Windows.Forms.DockStyle.None;
         this.mMaskPalette.Location = new System.Drawing.Point(100, 49);
         this.mMaskPalette.MaximumSize = new System.Drawing.Size(50, 25);
         this.mMaskPalette.Name = "mMaskPalette";
         this.mMaskPalette.SelectedTexture = 0;
         this.mMaskPalette.Size = new System.Drawing.Size(50, 25);
         this.mMaskPalette.TabIndex = 5;
         this.mMaskPalette.Text = "texturePalette1";
         this.mMaskPalette.Visible = false;
         // 
         // timer1
         // 
         this.timer1.Interval = 30;
         this.timer1.Tick += new System.EventHandler(this.timer1_Tick);
         // 
         // soundTimer
         // 
         this.soundTimer.Interval = 90;
         this.soundTimer.Tick += new System.EventHandler(this.soundTimer_Tick);
         // 
         // mDockLayoutManager
         // 
         this.mDockLayoutManager.Initialize(this.toolStripContainer1.ContentPanel, this.panel2);
         // 
         // cursorAtLocLabel
         // 
         this.cursorAtLocLabel.Name = "cursorAtLocLabel";
         this.cursorAtLocLabel.Size = new System.Drawing.Size(13, 17);
         this.cursorAtLocLabel.Text = "  ";
         // 
         // MainWindow
         // 
         this.AllowDrop = true;
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.ClientSize = new System.Drawing.Size(1121, 746);
         this.Controls.Add(this.toolStripContainer1);
         this.MainMenuStrip = this.menuStrip1;
         this.Name = "MainWindow";
         this.Text = "Phoenix Editor";
         this.Deactivate += new System.EventHandler(this.MainWindow_Deactivate);
         this.DragDrop += new System.Windows.Forms.DragEventHandler(this.MainWindow_DragDrop);
         this.Shown += new System.EventHandler(this.MainWindow_Shown);
         this.Enter += new System.EventHandler(this.MainWindow_Enter);
         this.Activated += new System.EventHandler(this.MainWindow_Activated);
         this.DragEnter += new System.Windows.Forms.DragEventHandler(this.MainWindow_DragEnter);
         this.Leave += new System.EventHandler(this.MainWindow_Leave);
         this.toolStripContainer1.BottomToolStripPanel.ResumeLayout(false);
         this.toolStripContainer1.BottomToolStripPanel.PerformLayout();
         this.toolStripContainer1.ContentPanel.ResumeLayout(false);
         this.toolStripContainer1.TopToolStripPanel.ResumeLayout(false);
         this.toolStripContainer1.TopToolStripPanel.PerformLayout();
         this.toolStripContainer1.ResumeLayout(false);
         this.toolStripContainer1.PerformLayout();
         this.statusStrip1.ResumeLayout(false);
         this.statusStrip1.PerformLayout();
         this.panel2.ResumeLayout(false);
         this.menuStrip1.ResumeLayout(false);
         this.menuStrip1.PerformLayout();
         ((System.ComponentModel.ISupportInitialize)(this.mDockLayoutManager)).EndInit();
         this.ResumeLayout(false);

      }

      

      #endregion

      private System.Windows.Forms.ToolStripContainer toolStripContainer1;
      private System.Windows.Forms.Timer timer1;
      private System.Windows.Forms.Timer soundTimer;
      private Xceed.DockingWindows.DockLayoutManager mDockLayoutManager;
      private System.Windows.Forms.ToolStrip TerrainToolStrip;
      private System.Windows.Forms.MenuStrip menuStrip1;
      private System.Windows.Forms.ToolStripMenuItem viewToolStripMenuItem;
      private System.Windows.Forms.ToolStripMenuItem fileToolStripMenuItem;
      private System.Windows.Forms.ToolStripMenuItem file_open;
      private System.Windows.Forms.ToolStripSeparator toolStripMenuItem7;
      private System.Windows.Forms.ToolStripMenuItem file_save;
      private System.Windows.Forms.ToolStripSeparator toolStripMenuItem8;
      private System.Windows.Forms.ToolStripMenuItem exitToolStripMenuItem;
      private System.Windows.Forms.ToolStripMenuItem editorToolStripMenuItem;
      private TexturePalette mTexturePalette;
      private System.Windows.Forms.Panel panel2;
      private TexturePalette mMaskPalette;
      private System.Windows.Forms.ToolStripMenuItem file_saveas;
      private System.Windows.Forms.ToolStripMenuItem toolStripMenuItem3;
      private System.Windows.Forms.ToolStripMenuItem aboutToolStripMenuItem;
      private System.Windows.Forms.ToolStrip SimToolStrip;
      private System.Windows.Forms.StatusStrip statusStrip1;
      private EditorCore.XpToolStripProgressBar toolStripProgressBar;
      private System.Windows.Forms.ToolStripStatusLabel LastErrorToolStripStatusLabel;
      private System.Windows.Forms.TabControl ClientTabControl;
      private System.Windows.Forms.Button closeActiveTab;
      private System.Windows.Forms.ToolStripMenuItem optionsToolStripMenuItem;
      private System.Windows.Forms.ToolStripMenuItem openScenarioToolStripMenuItem;
      private System.Windows.Forms.ToolStripMenuItem errorListToolStripMenuItem;
      private System.Windows.Forms.ToolStripMenuItem protoObjectsToolStripMenuItem;
      private System.Windows.Forms.ToolStripMenuItem importGR2ToolStripMenuItem;
      private System.Windows.Forms.ToolStripMenuItem importSoundsToolStripMenuItem;
      private System.Windows.Forms.ToolStripStatusLabel cursorAtLocLabel;


   }
}

