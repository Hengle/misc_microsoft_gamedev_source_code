namespace PhoenixEditor.ScenarioEditor
{
    partial class RandomMapGenerationForm
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
           this.treeView1 = new System.Windows.Forms.TreeView();
           this.toolStrip1 = new System.Windows.Forms.ToolStrip();
           this.toolStripButton1 = new System.Windows.Forms.ToolStripButton();
           this.toolStripSeparator1 = new System.Windows.Forms.ToolStripSeparator();
           this.toolStripButton2 = new System.Windows.Forms.ToolStripButton();
           this.toolStripButton3 = new System.Windows.Forms.ToolStripButton();
           this.toolStripButton4 = new System.Windows.Forms.ToolStripButton();
           this.toolStripSeparator2 = new System.Windows.Forms.ToolStripSeparator();
           this.toolStripButton5 = new System.Windows.Forms.ToolStripButton();
           this.label1 = new System.Windows.Forms.Label();
           this.maskListComboBox = new System.Windows.Forms.ComboBox();
           this.propertiesPanel = new System.Windows.Forms.Panel();
           this.toolStrip1.SuspendLayout();
           this.SuspendLayout();
           // 
           // treeView1
           // 
           this.treeView1.Location = new System.Drawing.Point(14, 95);
           this.treeView1.Name = "treeView1";
           this.treeView1.Size = new System.Drawing.Size(352, 462);
           this.treeView1.TabIndex = 13;
           this.treeView1.AfterSelect += new System.Windows.Forms.TreeViewEventHandler(this.treeView1_AfterSelect);
           // 
           // toolStrip1
           // 
           this.toolStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.toolStripButton1,
            this.toolStripSeparator1,
            this.toolStripButton2,
            this.toolStripButton3,
            this.toolStripButton4,
            this.toolStripSeparator2,
            this.toolStripButton5});
           this.toolStrip1.Location = new System.Drawing.Point(0, 0);
           this.toolStrip1.Name = "toolStrip1";
           this.toolStrip1.Size = new System.Drawing.Size(843, 25);
           this.toolStrip1.TabIndex = 14;
           this.toolStrip1.Text = "toolStrip1";
           // 
           // toolStripButton1
           // 
           this.toolStripButton1.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
           this.toolStripButton1.Image = global::PhoenixEditor.Properties.Resources.MenuFileNewIcon;
           this.toolStripButton1.ImageTransparentColor = System.Drawing.Color.Magenta;
           this.toolStripButton1.Name = "toolStripButton1";
           this.toolStripButton1.Size = new System.Drawing.Size(23, 22);
           this.toolStripButton1.Text = "toolStripButton1";
           this.toolStripButton1.Click += new System.EventHandler(this.toolStripButton1_Click);
           // 
           // toolStripSeparator1
           // 
           this.toolStripSeparator1.Name = "toolStripSeparator1";
           this.toolStripSeparator1.Size = new System.Drawing.Size(6, 25);
           // 
           // toolStripButton2
           // 
           this.toolStripButton2.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
           this.toolStripButton2.Image = global::PhoenixEditor.Properties.Resources.ImageFromDiskIcon;
           this.toolStripButton2.ImageTransparentColor = System.Drawing.Color.Magenta;
           this.toolStripButton2.Name = "toolStripButton2";
           this.toolStripButton2.Size = new System.Drawing.Size(23, 22);
           this.toolStripButton2.Text = "toolStripButton2";
           this.toolStripButton2.Click += new System.EventHandler(this.toolStripButton2_Click);
           // 
           // toolStripButton3
           // 
           this.toolStripButton3.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
           this.toolStripButton3.Image = global::PhoenixEditor.Properties.Resources.MenuFileSaveIcon;
           this.toolStripButton3.ImageTransparentColor = System.Drawing.Color.Magenta;
           this.toolStripButton3.Name = "toolStripButton3";
           this.toolStripButton3.Size = new System.Drawing.Size(23, 22);
           this.toolStripButton3.Text = "toolStripButton3";
           this.toolStripButton3.Click += new System.EventHandler(this.toolStripButton3_Click);
           // 
           // toolStripButton4
           // 
           this.toolStripButton4.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
           this.toolStripButton4.Image = global::PhoenixEditor.Properties.Resources.MenuFileSaveAsIcon;
           this.toolStripButton4.ImageTransparentColor = System.Drawing.Color.Magenta;
           this.toolStripButton4.Name = "toolStripButton4";
           this.toolStripButton4.Size = new System.Drawing.Size(23, 22);
           this.toolStripButton4.Text = "toolStripButton4";
           this.toolStripButton4.Click += new System.EventHandler(this.toolStripButton4_Click);
           // 
           // toolStripSeparator2
           // 
           this.toolStripSeparator2.Name = "toolStripSeparator2";
           this.toolStripSeparator2.Size = new System.Drawing.Size(6, 25);
           // 
           // toolStripButton5
           // 
           this.toolStripButton5.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
           this.toolStripButton5.Image = global::PhoenixEditor.Properties.Resources.ApplyToCurrentTerrain;
           this.toolStripButton5.ImageTransparentColor = System.Drawing.Color.Magenta;
           this.toolStripButton5.Name = "toolStripButton5";
           this.toolStripButton5.Size = new System.Drawing.Size(23, 22);
           this.toolStripButton5.Text = "toolStripButton5";
           this.toolStripButton5.Click += new System.EventHandler(this.toolStripButton5_Click);
           // 
           // label1
           // 
           this.label1.AutoSize = true;
           this.label1.Location = new System.Drawing.Point(22, 46);
           this.label1.Name = "label1";
           this.label1.Size = new System.Drawing.Size(82, 13);
           this.label1.TabIndex = 15;
           this.label1.Text = "Playable Mask :";
           // 
           // maskListComboBox
           // 
           this.maskListComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
           this.maskListComboBox.FormattingEnabled = true;
           this.maskListComboBox.Location = new System.Drawing.Point(124, 46);
           this.maskListComboBox.Name = "maskListComboBox";
           this.maskListComboBox.Size = new System.Drawing.Size(242, 21);
           this.maskListComboBox.TabIndex = 16;
           // 
           // propertiesPanel
           // 
           this.propertiesPanel.Location = new System.Drawing.Point(372, 95);
           this.propertiesPanel.Name = "propertiesPanel";
           this.propertiesPanel.Size = new System.Drawing.Size(458, 462);
           this.propertiesPanel.TabIndex = 17;
           // 
           // RandomMapGenerationForm
           // 
           this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
           this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
           this.Controls.Add(this.propertiesPanel);
           this.Controls.Add(this.maskListComboBox);
           this.Controls.Add(this.label1);
           this.Controls.Add(this.toolStrip1);
           this.Controls.Add(this.treeView1);
           this.Name = "RandomMapGenerationForm";
           this.Size = new System.Drawing.Size(843, 578);
           this.Load += new System.EventHandler(this.RandomMapGenerationForm_Load);
           this.toolStrip1.ResumeLayout(false);
           this.toolStrip1.PerformLayout();
           this.ResumeLayout(false);
           this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.TreeView treeView1;
       private System.Windows.Forms.ToolStrip toolStrip1;
       private System.Windows.Forms.ToolStripButton toolStripButton1;
       private System.Windows.Forms.ToolStripSeparator toolStripSeparator1;
       private System.Windows.Forms.ToolStripButton toolStripButton2;
       private System.Windows.Forms.ToolStripButton toolStripButton3;
       private System.Windows.Forms.ToolStripButton toolStripButton4;
       private System.Windows.Forms.Label label1;
       private System.Windows.Forms.ComboBox maskListComboBox;
       private System.Windows.Forms.Panel propertiesPanel;
       private System.Windows.Forms.ToolStripSeparator toolStripSeparator2;
       private System.Windows.Forms.ToolStripButton toolStripButton5;
    }
}
