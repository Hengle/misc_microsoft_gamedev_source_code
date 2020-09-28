namespace PhoenixEditor.ScenarioEditor
{
   partial class FoliagePanel
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
         this.foliageListBox = new System.Windows.Forms.ListBox();
         this.flowLayoutPanel1 = new System.Windows.Forms.FlowLayoutPanel();
         this.groupBox1 = new System.Windows.Forms.GroupBox();
         this.paintErase = new System.Windows.Forms.RadioButton();
         this.paintSet = new System.Windows.Forms.RadioButton();
         this.setMenuStrip = new System.Windows.Forms.ContextMenuStrip(this.components);
         this.removeFromMapToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.bladeMenuStrip = new System.Windows.Forms.ContextMenuStrip(this.components);
         this.removeFromMaToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.eraseAllButton = new System.Windows.Forms.Button();
         this.groupBox1.SuspendLayout();
         this.setMenuStrip.SuspendLayout();
         this.bladeMenuStrip.SuspendLayout();
         this.SuspendLayout();
         // 
         // foliageListBox
         // 
         this.foliageListBox.FormattingEnabled = true;
         this.foliageListBox.Location = new System.Drawing.Point(3, 17);
         this.foliageListBox.Name = "foliageListBox";
         this.foliageListBox.Size = new System.Drawing.Size(226, 69);
         this.foliageListBox.TabIndex = 0;
         this.foliageListBox.SelectedIndexChanged += new System.EventHandler(this.foliageListBox_SelectedIndexChanged);
         // 
         // flowLayoutPanel1
         // 
         this.flowLayoutPanel1.AutoScroll = true;
         this.flowLayoutPanel1.BackColor = System.Drawing.Color.White;
         this.flowLayoutPanel1.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
         this.flowLayoutPanel1.FlowDirection = System.Windows.Forms.FlowDirection.TopDown;
         this.flowLayoutPanel1.Location = new System.Drawing.Point(3, 104);
         this.flowLayoutPanel1.Name = "flowLayoutPanel1";
         this.flowLayoutPanel1.Size = new System.Drawing.Size(226, 340);
         this.flowLayoutPanel1.TabIndex = 1;
         this.flowLayoutPanel1.WrapContents = false;
         // 
         // groupBox1
         // 
         this.groupBox1.Controls.Add(this.paintErase);
         this.groupBox1.Controls.Add(this.paintSet);
         this.groupBox1.Location = new System.Drawing.Point(3, 450);
         this.groupBox1.Name = "groupBox1";
         this.groupBox1.Size = new System.Drawing.Size(200, 81);
         this.groupBox1.TabIndex = 2;
         this.groupBox1.TabStop = false;
         this.groupBox1.Text = "Paint Mode";
         // 
         // paintErase
         // 
         this.paintErase.AutoSize = true;
         this.paintErase.Location = new System.Drawing.Point(6, 42);
         this.paintErase.Name = "paintErase";
         this.paintErase.Size = new System.Drawing.Size(66, 17);
         this.paintErase.TabIndex = 1;
         this.paintErase.Text = "Erase All";
         this.paintErase.UseVisualStyleBackColor = true;
         this.paintErase.CheckedChanged += new System.EventHandler(this.paintErase_CheckedChanged);
         // 
         // paintSet
         // 
         this.paintSet.AutoSize = true;
         this.paintSet.Checked = true;
         this.paintSet.Location = new System.Drawing.Point(6, 19);
         this.paintSet.Name = "paintSet";
         this.paintSet.Size = new System.Drawing.Size(102, 17);
         this.paintSet.TabIndex = 0;
         this.paintSet.TabStop = true;
         this.paintSet.Text = "Set To Selected";
         this.paintSet.UseVisualStyleBackColor = true;
         this.paintSet.CheckedChanged += new System.EventHandler(this.paintSet_CheckedChanged);
         // 
         // setMenuStrip
         // 
         this.setMenuStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.removeFromMapToolStripMenuItem});
         this.setMenuStrip.Name = "setMenuStrip";
         this.setMenuStrip.Size = new System.Drawing.Size(175, 26);
         // 
         // removeFromMapToolStripMenuItem
         // 
         this.removeFromMapToolStripMenuItem.Name = "removeFromMapToolStripMenuItem";
         this.removeFromMapToolStripMenuItem.Size = new System.Drawing.Size(174, 22);
         this.removeFromMapToolStripMenuItem.Text = "Remove From Map";
         this.removeFromMapToolStripMenuItem.Click += new System.EventHandler(this.removeFromMapToolStripMenuItem_Click);
         // 
         // bladeMenuStrip
         // 
         this.bladeMenuStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.removeFromMaToolStripMenuItem});
         this.bladeMenuStrip.Name = "bladeMenuStrip";
         this.bladeMenuStrip.Size = new System.Drawing.Size(175, 26);
         // 
         // removeFromMaToolStripMenuItem
         // 
         this.removeFromMaToolStripMenuItem.Name = "removeFromMaToolStripMenuItem";
         this.removeFromMaToolStripMenuItem.Size = new System.Drawing.Size(174, 22);
         this.removeFromMaToolStripMenuItem.Text = "Remove From Map";
         this.removeFromMaToolStripMenuItem.Click += new System.EventHandler(this.removeFromMaToolStripMenuItem_Click);
         // 
         // eraseAllButton
         // 
         this.eraseAllButton.Location = new System.Drawing.Point(34, 546);
         this.eraseAllButton.Name = "eraseAllButton";
         this.eraseAllButton.Size = new System.Drawing.Size(135, 23);
         this.eraseAllButton.TabIndex = 3;
         this.eraseAllButton.Text = "Erase All Foliage";
         this.eraseAllButton.UseVisualStyleBackColor = true;
         this.eraseAllButton.Click += new System.EventHandler(this.eraseAllButton_Click);
         // 
         // FoliagePanel
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.eraseAllButton);
         this.Controls.Add(this.groupBox1);
         this.Controls.Add(this.flowLayoutPanel1);
         this.Controls.Add(this.foliageListBox);
         this.Key = "FoliagePanel";
         this.Name = "FoliagePanel";
         this.Size = new System.Drawing.Size(232, 659);
         this.Text = "Foliage";
         this.Load += new System.EventHandler(this.FoliagePanel_Load);
         this.groupBox1.ResumeLayout(false);
         this.groupBox1.PerformLayout();
         this.setMenuStrip.ResumeLayout(false);
         this.bladeMenuStrip.ResumeLayout(false);
         this.ResumeLayout(false);

      }

      #endregion

      private System.Windows.Forms.ListBox foliageListBox;
      private System.Windows.Forms.FlowLayoutPanel flowLayoutPanel1;
      private System.Windows.Forms.GroupBox groupBox1;
      public System.Windows.Forms.ContextMenuStrip setMenuStrip;
      private System.Windows.Forms.ToolStripMenuItem removeFromMapToolStripMenuItem;
      public System.Windows.Forms.ContextMenuStrip bladeMenuStrip;
      private System.Windows.Forms.ToolStripMenuItem removeFromMaToolStripMenuItem;
      private System.Windows.Forms.RadioButton paintErase;
      private System.Windows.Forms.RadioButton paintSet;
      private System.Windows.Forms.Button eraseAllButton;
   }
}
