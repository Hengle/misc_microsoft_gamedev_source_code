namespace RemoteMemory
{
   partial class MainForm
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
         System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(MainForm));
         this.statusStrip1 = new System.Windows.Forms.StatusStrip();
         this.timer1 = new System.Windows.Forms.Timer(this.components);
         this.toolStrip1 = new System.Windows.Forms.ToolStrip();
         this.toolStripSeparator3 = new System.Windows.Forms.ToolStripSeparator();
         this.toolStripSeparator2 = new System.Windows.Forms.ToolStripSeparator();
         this.toolStripSeparator4 = new System.Windows.Forms.ToolStripSeparator();
         this.dockPanel = new WinFormsUI.Docking.DockPanel();
         this.toolStripButton1 = new System.Windows.Forms.ToolStripButton();
         this.toolStripButton2 = new System.Windows.Forms.ToolStripButton();
         this.stopProcessButton = new System.Windows.Forms.ToolStripButton();
         this.toolStripButton4 = new System.Windows.Forms.ToolStripButton();
         this.toolStripButton3 = new System.Windows.Forms.ToolStripButton();
         this.toolStrip1.SuspendLayout();
         this.SuspendLayout();
         // 
         // statusStrip1
         // 
         this.statusStrip1.Location = new System.Drawing.Point(0, 471);
         this.statusStrip1.Name = "statusStrip1";
         this.statusStrip1.Size = new System.Drawing.Size(732, 22);
         this.statusStrip1.TabIndex = 1;
         this.statusStrip1.Text = "statusStrip1";
         // 
         // timer1
         // 
         this.timer1.Interval = 5;
         this.timer1.Tick += new System.EventHandler(this.timer1_Tick);
         // 
         // toolStrip1
         // 
         this.toolStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.toolStripButton1,
            this.toolStripButton2,
            this.stopProcessButton,
            this.toolStripSeparator3,
            this.toolStripButton4,
            this.toolStripSeparator2,
            this.toolStripButton3,
            this.toolStripSeparator4});
         this.toolStrip1.Location = new System.Drawing.Point(0, 0);
         this.toolStrip1.Name = "toolStrip1";
         this.toolStrip1.Size = new System.Drawing.Size(732, 25);
         this.toolStrip1.TabIndex = 5;
         this.toolStrip1.Text = "toolStrip1";
         // 
         // toolStripSeparator3
         // 
         this.toolStripSeparator3.Name = "toolStripSeparator3";
         this.toolStripSeparator3.Size = new System.Drawing.Size(6, 25);
         // 
         // toolStripSeparator2
         // 
         this.toolStripSeparator2.Name = "toolStripSeparator2";
         this.toolStripSeparator2.Size = new System.Drawing.Size(6, 25);
         // 
         // toolStripSeparator4
         // 
         this.toolStripSeparator4.Name = "toolStripSeparator4";
         this.toolStripSeparator4.Size = new System.Drawing.Size(6, 25);
         // 
         // dockPanel
         // 
         this.dockPanel.ActiveAutoHideContent = null;
         this.dockPanel.Dock = System.Windows.Forms.DockStyle.Fill;
         this.dockPanel.DockBottomPortion = 150;
         this.dockPanel.DockLeftPortion = 200;
         this.dockPanel.DockRightPortion = 200;
         this.dockPanel.DockTopPortion = 150;
         this.dockPanel.Font = new System.Drawing.Font("Tahoma", 11F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.World, ((byte)(0)));
         this.dockPanel.Location = new System.Drawing.Point(0, 0);
         this.dockPanel.Name = "dockPanel";
         this.dockPanel.RightToLeftLayout = true;
         this.dockPanel.Size = new System.Drawing.Size(732, 471);
         this.dockPanel.TabIndex = 0;
         // 
         // toolStripButton1
         // 
         this.toolStripButton1.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
         this.toolStripButton1.Image = global::RemoteMemory.Properties.Resources.xbox360_32x32;
         this.toolStripButton1.ImageTransparentColor = System.Drawing.Color.Magenta;
         this.toolStripButton1.Name = "toolStripButton1";
         this.toolStripButton1.Size = new System.Drawing.Size(23, 22);
         this.toolStripButton1.Text = "Connect To XBOX";
         this.toolStripButton1.Click += new System.EventHandler(this.toolStripButton1_Click);
         // 
         // toolStripButton2
         // 
         this.toolStripButton2.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
         this.toolStripButton2.Image = global::RemoteMemory.Properties.Resources.ImageFromDiskIcon;
         this.toolStripButton2.ImageTransparentColor = System.Drawing.Color.Magenta;
         this.toolStripButton2.Name = "toolStripButton2";
         this.toolStripButton2.Size = new System.Drawing.Size(23, 22);
         this.toolStripButton2.Text = "Open File";
         this.toolStripButton2.Click += new System.EventHandler(this.toolStripButton2_Click);
         // 
         // stopProcessButton
         // 
         this.stopProcessButton.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
         this.stopProcessButton.Enabled = false;
         this.stopProcessButton.Image = global::RemoteMemory.Properties.Resources.stop;
         this.stopProcessButton.ImageTransparentColor = System.Drawing.Color.Magenta;
         this.stopProcessButton.Name = "stopProcessButton";
         this.stopProcessButton.Size = new System.Drawing.Size(23, 22);
         this.stopProcessButton.Text = "Stop Processing";
         this.stopProcessButton.Click += new System.EventHandler(this.stopProcessButton_Click);
         // 
         // toolStripButton4
         // 
         this.toolStripButton4.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
         this.toolStripButton4.Enabled = false;
         this.toolStripButton4.Image = global::RemoteMemory.Properties.Resources.MenuFileSaveAsIcon;
         this.toolStripButton4.ImageTransparentColor = System.Drawing.Color.Magenta;
         this.toolStripButton4.Name = "toolStripButton4";
         this.toolStripButton4.Size = new System.Drawing.Size(23, 22);
         this.toolStripButton4.Text = "Save View As";
         this.toolStripButton4.Click += new System.EventHandler(this.toolStripButton4_Click);
         // 
         // toolStripButton3
         // 
         this.toolStripButton3.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
         this.toolStripButton3.Image = global::RemoteMemory.Properties.Resources.screenshot;
         this.toolStripButton3.ImageTransparentColor = System.Drawing.Color.Magenta;
         this.toolStripButton3.Name = "toolStripButton3";
         this.toolStripButton3.Size = new System.Drawing.Size(23, 22);
         this.toolStripButton3.Text = "Take A Screenshot";
         this.toolStripButton3.Click += new System.EventHandler(this.toolStripButton3_Click);
         // 
         // MainForm
         // 
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Inherit;
         this.ClientSize = new System.Drawing.Size(732, 493);
         this.Controls.Add(this.toolStrip1);
         this.Controls.Add(this.dockPanel);
         this.Controls.Add(this.statusStrip1);
         this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
         this.IsMdiContainer = true;
         this.Name = "MainForm";
         this.Text = "Remote Memory View";
         this.WindowState = System.Windows.Forms.FormWindowState.Maximized;
         this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.MainForm_FormClosing);
         this.toolStrip1.ResumeLayout(false);
         this.toolStrip1.PerformLayout();
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private WinFormsUI.Docking.DockPanel dockPanel;
      private System.Windows.Forms.StatusStrip statusStrip1;
      private System.Windows.Forms.Timer timer1;
      private System.Windows.Forms.ToolStrip toolStrip1;
      private System.Windows.Forms.ToolStripButton toolStripButton1;
      private System.Windows.Forms.ToolStripButton toolStripButton2;
      private System.Windows.Forms.ToolStripButton toolStripButton4;
      private System.Windows.Forms.ToolStripSeparator toolStripSeparator2;
      private System.Windows.Forms.ToolStripSeparator toolStripSeparator3;
      private System.Windows.Forms.ToolStripButton stopProcessButton;
      private System.Windows.Forms.ToolStripButton toolStripButton3;
      private System.Windows.Forms.ToolStripSeparator toolStripSeparator4;
   }
}

