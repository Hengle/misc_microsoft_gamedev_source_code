namespace PhoenixEditor.ScenarioEditor
{
   partial class TriggerHostArea
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
         this.HostContextMenuStrip = new System.Windows.Forms.ContextMenuStrip(this.components);
         this.newTriggerToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.tabControl1 = new System.Windows.Forms.TabControl();
         this.HostContextMenuStrip.SuspendLayout();
         this.SuspendLayout();
         // 
         // HostContextMenuStrip
         // 
         this.HostContextMenuStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.newTriggerToolStripMenuItem});
         this.HostContextMenuStrip.Name = "HostContextMenuStrip";
         this.HostContextMenuStrip.Size = new System.Drawing.Size(144, 26);
         // 
         // newTriggerToolStripMenuItem
         // 
         this.newTriggerToolStripMenuItem.Name = "newTriggerToolStripMenuItem";
         this.newTriggerToolStripMenuItem.Size = new System.Drawing.Size(143, 22);
         this.newTriggerToolStripMenuItem.Text = "New Trigger";
         this.newTriggerToolStripMenuItem.Click += new System.EventHandler(this.newTriggerToolStripMenuItem_Click);
         // 
         // tabControl1
         // 
         this.tabControl1.Alignment = System.Windows.Forms.TabAlignment.Bottom;
         this.tabControl1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.tabControl1.Location = new System.Drawing.Point(0, 0);
         this.tabControl1.Multiline = true;
         this.tabControl1.Name = "tabControl1";
         this.tabControl1.SelectedIndex = 0;
         this.tabControl1.Size = new System.Drawing.Size(945, 603);
         this.tabControl1.TabIndex = 1;
         // 
         // TriggerHostArea
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.AutoScroll = true;
         this.BackColor = System.Drawing.SystemColors.Window;
         this.Controls.Add(this.tabControl1);
         this.Name = "TriggerHostArea";
         this.Size = new System.Drawing.Size(945, 603);
         this.HostContextMenuStrip.ResumeLayout(false);
         this.ResumeLayout(false);

      }

      #endregion

      private System.Windows.Forms.ContextMenuStrip HostContextMenuStrip;
      private System.Windows.Forms.ContextMenuStrip SelectedItemsContextMenuStrip;
      private System.Windows.Forms.ToolStripMenuItem newTriggerToolStripMenuItem;
      private System.Windows.Forms.TabControl tabControl1;
   }
}
