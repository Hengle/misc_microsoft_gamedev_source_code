namespace GDIControls
{
   partial class GDITreeView
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
         this.vScrollBar1 = new System.Windows.Forms.VScrollBar();
         this.mainContextMenu = new System.Windows.Forms.ContextMenuStrip(this.components);
         this.expandAllToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.collapseAllToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.mainSpacerToolStripMenuItem = new System.Windows.Forms.ToolStripSeparator();
         this.mainContextMenu.SuspendLayout();
         this.SuspendLayout();
         // 
         // vScrollBar1
         // 
         this.vScrollBar1.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.vScrollBar1.LargeChange = 1;
         this.vScrollBar1.Location = new System.Drawing.Point(380, 0);
         this.vScrollBar1.Name = "vScrollBar1";
         this.vScrollBar1.Size = new System.Drawing.Size(17, 300);
         this.vScrollBar1.TabIndex = 0;
         this.vScrollBar1.Visible = false;
         this.vScrollBar1.ValueChanged += new System.EventHandler(this.vScrollBar1_ValueChanged);
         // 
         // mainContextMenu
         // 
         this.mainContextMenu.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.expandAllToolStripMenuItem,
            this.collapseAllToolStripMenuItem,
            this.mainSpacerToolStripMenuItem});
         this.mainContextMenu.Name = "mainContextMenu";
         this.mainContextMenu.Size = new System.Drawing.Size(140, 54);
         // 
         // expandAllToolStripMenuItem
         // 
         this.expandAllToolStripMenuItem.Name = "expandAllToolStripMenuItem";
         this.expandAllToolStripMenuItem.Size = new System.Drawing.Size(152, 22);
         this.expandAllToolStripMenuItem.Text = "Expand All";
         this.expandAllToolStripMenuItem.Click += new System.EventHandler(this.expandAllToolStripMenuItem_Click);
         // 
         // collapseAllToolStripMenuItem
         // 
         this.collapseAllToolStripMenuItem.Name = "collapseAllToolStripMenuItem";
         this.collapseAllToolStripMenuItem.Size = new System.Drawing.Size(152, 22);
         this.collapseAllToolStripMenuItem.Text = "Collapse All";
         this.collapseAllToolStripMenuItem.Click += new System.EventHandler(this.collapseAllToolStripMenuItem_Click);
         // 
         // mainSpacerToolStripMenuItem
         // 
         this.mainSpacerToolStripMenuItem.Name = "mainSpacerToolStripMenuItem";
         this.mainSpacerToolStripMenuItem.Size = new System.Drawing.Size(149, 6);
         // 
         // GDITreeView
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.vScrollBar1);
         this.Name = "GDITreeView";
         this.Size = new System.Drawing.Size(397, 300);
         this.mainContextMenu.ResumeLayout(false);
         this.ResumeLayout(false);

      }

     

      #endregion

      private System.Windows.Forms.VScrollBar vScrollBar1;
      private System.Windows.Forms.ContextMenuStrip mainContextMenu;
      private System.Windows.Forms.ToolStripMenuItem expandAllToolStripMenuItem;
      private System.Windows.Forms.ToolStripMenuItem collapseAllToolStripMenuItem;
      private System.Windows.Forms.ToolStripSeparator mainSpacerToolStripMenuItem;
   }
}
