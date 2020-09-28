namespace GameDatabase
{
   partial class DatabaseMainPage
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
         System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(DatabaseMainPage));
         this.menuStrip1 = new System.Windows.Forms.MenuStrip();
         this.fileToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.todoPutYourMenusHereToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.toolStripContainer1 = new System.Windows.Forms.ToolStripContainer();
         this.DummyToolStrip = new System.Windows.Forms.ToolStrip();
         this.toolStrip1 = new System.Windows.Forms.ToolStrip();
         this.toolStripButton1 = new System.Windows.Forms.ToolStripButton();
         this.menuStrip1.SuspendLayout();
         this.toolStripContainer1.TopToolStripPanel.SuspendLayout();
         this.toolStripContainer1.SuspendLayout();
         this.toolStrip1.SuspendLayout();
         this.SuspendLayout();
         // 
         // menuStrip1
         // 
         this.menuStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.fileToolStripMenuItem,
            this.todoPutYourMenusHereToolStripMenuItem});
         this.menuStrip1.Location = new System.Drawing.Point(0, 0);
         this.menuStrip1.Name = "menuStrip1";
         this.menuStrip1.Size = new System.Drawing.Size(860, 24);
         this.menuStrip1.TabIndex = 0;
         this.menuStrip1.Text = "menuStrip1";
         // 
         // fileToolStripMenuItem
         // 
         this.fileToolStripMenuItem.Name = "fileToolStripMenuItem";
         this.fileToolStripMenuItem.Size = new System.Drawing.Size(35, 20);
         this.fileToolStripMenuItem.Text = "File";
         // 
         // todoPutYourMenusHereToolStripMenuItem
         // 
         this.todoPutYourMenusHereToolStripMenuItem.Name = "todoPutYourMenusHereToolStripMenuItem";
         this.todoPutYourMenusHereToolStripMenuItem.Size = new System.Drawing.Size(150, 20);
         this.todoPutYourMenusHereToolStripMenuItem.Text = "Todo, put your menus here";
         // 
         // toolStripContainer1
         // 
         // 
         // toolStripContainer1.ContentPanel
         // 
         this.toolStripContainer1.ContentPanel.Size = new System.Drawing.Size(390, 34);
         this.toolStripContainer1.Location = new System.Drawing.Point(256, 368);
         this.toolStripContainer1.Name = "toolStripContainer1";
         this.toolStripContainer1.Size = new System.Drawing.Size(390, 84);
         this.toolStripContainer1.TabIndex = 1;
         this.toolStripContainer1.Text = "toolStripContainer1";
         // 
         // toolStripContainer1.TopToolStripPanel
         // 
         this.toolStripContainer1.TopToolStripPanel.Controls.Add(this.DummyToolStrip);
         this.toolStripContainer1.TopToolStripPanel.Controls.Add(this.toolStrip1);
         this.toolStripContainer1.Visible = false;
         // 
         // DummyToolStrip
         // 
         this.DummyToolStrip.BackColor = System.Drawing.SystemColors.ActiveCaption;
         this.DummyToolStrip.Dock = System.Windows.Forms.DockStyle.None;
         this.DummyToolStrip.Location = new System.Drawing.Point(3, 0);
         this.DummyToolStrip.Name = "DummyToolStrip";
         this.DummyToolStrip.Size = new System.Drawing.Size(111, 25);
         this.DummyToolStrip.TabIndex = 12;
         // 
         // toolStrip1
         // 
         this.toolStrip1.Dock = System.Windows.Forms.DockStyle.None;
         this.toolStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.toolStripButton1});
         this.toolStrip1.Location = new System.Drawing.Point(3, 25);
         this.toolStrip1.Name = "toolStrip1";
         this.toolStrip1.Size = new System.Drawing.Size(182, 25);
         this.toolStrip1.TabIndex = 0;
         // 
         // toolStripButton1
         // 
         this.toolStripButton1.Image = ((System.Drawing.Image)(resources.GetObject("toolStripButton1.Image")));
         this.toolStripButton1.ImageTransparentColor = System.Drawing.Color.Magenta;
         this.toolStripButton1.Name = "toolStripButton1";
         this.toolStripButton1.Size = new System.Drawing.Size(170, 22);
         this.toolStripButton1.Text = "Todo, Put your toolbars here.";
         // 
         // DatabaseMainPage
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.toolStripContainer1);
         this.Controls.Add(this.menuStrip1);
         this.Name = "DatabaseMainPage";
         this.Size = new System.Drawing.Size(860, 622);
         this.menuStrip1.ResumeLayout(false);
         this.menuStrip1.PerformLayout();
         this.toolStripContainer1.TopToolStripPanel.ResumeLayout(false);
         this.toolStripContainer1.TopToolStripPanel.PerformLayout();
         this.toolStripContainer1.ResumeLayout(false);
         this.toolStripContainer1.PerformLayout();
         this.toolStrip1.ResumeLayout(false);
         this.toolStrip1.PerformLayout();
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private System.Windows.Forms.MenuStrip menuStrip1;
      private System.Windows.Forms.ToolStripMenuItem fileToolStripMenuItem;
      private System.Windows.Forms.ToolStripContainer toolStripContainer1;
      private System.Windows.Forms.ToolStripMenuItem todoPutYourMenusHereToolStripMenuItem;
      private System.Windows.Forms.ToolStrip toolStrip1;
      private System.Windows.Forms.ToolStripButton toolStripButton1;
      private System.Windows.Forms.ToolStrip DummyToolStrip;
   }
}
