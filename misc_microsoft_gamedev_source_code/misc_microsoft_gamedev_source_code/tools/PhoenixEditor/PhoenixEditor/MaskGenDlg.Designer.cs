namespace PhoenixEditor
{
   partial class MaskGenDlg
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
         this.panel1 = new System.Windows.Forms.Panel();
         this.toolStrip1 = new System.Windows.Forms.ToolStrip();
         this.toolStripButton1 = new System.Windows.Forms.ToolStripButton();
         this.toolStripButton2 = new System.Windows.Forms.ToolStripButton();
         this.toolStripSeparator1 = new System.Windows.Forms.ToolStripSeparator();
         this.toolStripButton3 = new System.Windows.Forms.ToolStripButton();
         this.toolStripButton4 = new System.Windows.Forms.ToolStripButton();
         this.toolStripSeparator2 = new System.Windows.Forms.ToolStripSeparator();
         this.toolStripButton5 = new System.Windows.Forms.ToolStripButton();
         this.toolStripButton6 = new System.Windows.Forms.ToolStripButton();
         this.pictureBox1 = new System.Windows.Forms.PictureBox();
         this.comboBox1 = new System.Windows.Forms.ComboBox();
         this.button1 = new System.Windows.Forms.Button();
         this.toolStrip1.SuspendLayout();
         ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).BeginInit();
         this.SuspendLayout();
         // 
         // panel1
         // 
         this.panel1.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
         this.panel1.Location = new System.Drawing.Point(136, 28);
         this.panel1.Name = "panel1";
         this.panel1.Size = new System.Drawing.Size(1161, 595);
         this.panel1.TabIndex = 3;
         // 
         // toolStrip1
         // 
         this.toolStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.toolStripButton1,
            this.toolStripButton2,
            this.toolStripSeparator1,
            this.toolStripButton3,
            this.toolStripButton4,
            this.toolStripSeparator2,
            this.toolStripButton5,
            this.toolStripButton6});
         this.toolStrip1.Location = new System.Drawing.Point(0, 0);
         this.toolStrip1.Name = "toolStrip1";
         this.toolStrip1.Size = new System.Drawing.Size(1297, 25);
         this.toolStrip1.TabIndex = 4;
         this.toolStrip1.Text = "toolStrip1";
         // 
         // toolStripButton1
         // 
         this.toolStripButton1.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
         this.toolStripButton1.Image = global::PhoenixEditor.Properties.Resources.MenuFileNewIcon;
         this.toolStripButton1.ImageTransparentColor = System.Drawing.Color.Magenta;
         this.toolStripButton1.Name = "toolStripButton1";
         this.toolStripButton1.Size = new System.Drawing.Size(23, 22);
         this.toolStripButton1.Text = "New Graph";
         this.toolStripButton1.Click += new System.EventHandler(this.toolStripButton1_Click);
         // 
         // toolStripButton2
         // 
         this.toolStripButton2.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
         this.toolStripButton2.Image = global::PhoenixEditor.Properties.Resources.ImageFromDiskIcon;
         this.toolStripButton2.ImageTransparentColor = System.Drawing.Color.Magenta;
         this.toolStripButton2.Name = "toolStripButton2";
         this.toolStripButton2.Size = new System.Drawing.Size(23, 22);
         this.toolStripButton2.Text = "Open Graph";
         this.toolStripButton2.Click += new System.EventHandler(this.toolStripButton2_Click);
         // 
         // toolStripSeparator1
         // 
         this.toolStripSeparator1.Name = "toolStripSeparator1";
         this.toolStripSeparator1.Size = new System.Drawing.Size(6, 25);
         // 
         // toolStripButton3
         // 
         this.toolStripButton3.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
         this.toolStripButton3.Image = global::PhoenixEditor.Properties.Resources.MenuFileSaveIcon;
         this.toolStripButton3.ImageTransparentColor = System.Drawing.Color.Magenta;
         this.toolStripButton3.Name = "toolStripButton3";
         this.toolStripButton3.Size = new System.Drawing.Size(23, 22);
         this.toolStripButton3.Text = "Save Graph";
         this.toolStripButton3.Click += new System.EventHandler(this.toolStripButton3_Click);
         // 
         // toolStripButton4
         // 
         this.toolStripButton4.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
         this.toolStripButton4.Image = global::PhoenixEditor.Properties.Resources.MenuFileSaveAsIcon;
         this.toolStripButton4.ImageTransparentColor = System.Drawing.Color.Magenta;
         this.toolStripButton4.Name = "toolStripButton4";
         this.toolStripButton4.Size = new System.Drawing.Size(23, 22);
         this.toolStripButton4.Text = "Save As";
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
         this.toolStripButton5.Image = global::PhoenixEditor.Properties.Resources.SaveToMaskList;
         this.toolStripButton5.ImageTransparentColor = System.Drawing.Color.Magenta;
         this.toolStripButton5.Name = "toolStripButton5";
         this.toolStripButton5.Size = new System.Drawing.Size(23, 22);
         this.toolStripButton5.Text = "Save To Mask List";
         this.toolStripButton5.Click += new System.EventHandler(this.toolStripButton5_Click);
         // 
         // toolStripButton6
         // 
         this.toolStripButton6.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Image;
         this.toolStripButton6.Image = global::PhoenixEditor.Properties.Resources.ApplyToCurrentTerrain;
         this.toolStripButton6.ImageTransparentColor = System.Drawing.Color.Magenta;
         this.toolStripButton6.Name = "toolStripButton6";
         this.toolStripButton6.Size = new System.Drawing.Size(23, 22);
         this.toolStripButton6.Text = "Apply As Current Mask";
         this.toolStripButton6.Click += new System.EventHandler(this.toolStripButton6_Click);
         // 
         // pictureBox1
         // 
         this.pictureBox1.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
         this.pictureBox1.Location = new System.Drawing.Point(2, 28);
         this.pictureBox1.Name = "pictureBox1";
         this.pictureBox1.Size = new System.Drawing.Size(128, 128);
         this.pictureBox1.TabIndex = 0;
         this.pictureBox1.TabStop = false;
         // 
         // comboBox1
         // 
         this.comboBox1.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
         this.comboBox1.FormattingEnabled = true;
         this.comboBox1.Location = new System.Drawing.Point(2, 162);
         this.comboBox1.Name = "comboBox1";
         this.comboBox1.Size = new System.Drawing.Size(128, 21);
         this.comboBox1.TabIndex = 5;
         this.comboBox1.SelectedIndexChanged += new System.EventHandler(this.comboBox1_SelectedIndexChanged);
         // 
         // button1
         // 
         this.button1.Location = new System.Drawing.Point(2, 189);
         this.button1.Name = "button1";
         this.button1.Size = new System.Drawing.Size(128, 23);
         this.button1.TabIndex = 6;
         this.button1.Text = "Hi-Res preview";
         this.button1.UseVisualStyleBackColor = true;
         this.button1.Click += new System.EventHandler(this.button1_Click);
         // 
         // MaskGenDlg
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.ClientSize = new System.Drawing.Size(1297, 627);
         this.Controls.Add(this.button1);
         this.Controls.Add(this.comboBox1);
         this.Controls.Add(this.toolStrip1);
         this.Controls.Add(this.panel1);
         this.Controls.Add(this.pictureBox1);
         this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
         this.Name = "MaskGenDlg";
         this.Text = "MaskGenForm";
         this.Load += new System.EventHandler(this.MaskGenDlg_Load);
         this.toolStrip1.ResumeLayout(false);
         this.toolStrip1.PerformLayout();
         ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).EndInit();
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private System.Windows.Forms.PictureBox pictureBox1;
      private System.Windows.Forms.Panel panel1;
      private System.Windows.Forms.ToolStrip toolStrip1;
      private System.Windows.Forms.ToolStripButton toolStripButton1;
      private System.Windows.Forms.ToolStripSeparator toolStripSeparator1;
      private System.Windows.Forms.ToolStripButton toolStripButton2;
      private System.Windows.Forms.ToolStripButton toolStripButton3;
      private System.Windows.Forms.ToolStripButton toolStripButton4;
      private System.Windows.Forms.ToolStripSeparator toolStripSeparator2;
      private System.Windows.Forms.ToolStripButton toolStripButton5;
      private System.Windows.Forms.ToolStripButton toolStripButton6;
      private System.Windows.Forms.ComboBox comboBox1;
      private System.Windows.Forms.Button button1;
   }
}