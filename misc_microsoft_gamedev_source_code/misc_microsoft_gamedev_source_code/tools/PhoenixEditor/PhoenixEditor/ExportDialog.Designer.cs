namespace PhoenixEditor
{
   partial class ExportDialog
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
         this.ExportButton = new System.Windows.Forms.Button();
         this.CancelExportButton = new System.Windows.Forms.Button();
         this.ExportLevelListBox = new System.Windows.Forms.ListBox();
         this.lodFactorSlider = new System.Windows.Forms.TrackBar();
         this.groupBox1 = new System.Windows.Forms.GroupBox();
         this.lodlabel = new System.Windows.Forms.Label();
         this.label2 = new System.Windows.Forms.Label();
         this.biasLabel = new System.Windows.Forms.Label();
         this.label1 = new System.Windows.Forms.Label();
         this.factorLabel = new System.Windows.Forms.Label();
         this.groupBox2 = new System.Windows.Forms.GroupBox();
         this.unqLabel = new System.Windows.Forms.Label();
         this.UniRes = new System.Windows.Forms.TrackBar();
         this.label3 = new System.Windows.Forms.Label();
         this.comboBox1 = new System.Windows.Forms.ComboBox();
         this.checkBox1 = new System.Windows.Forms.CheckBox();
         this.groupBox3 = new System.Windows.Forms.GroupBox();
         ((System.ComponentModel.ISupportInitialize)(this.lodFactorSlider)).BeginInit();
         this.groupBox1.SuspendLayout();
         this.groupBox2.SuspendLayout();
         ((System.ComponentModel.ISupportInitialize)(this.UniRes)).BeginInit();
         this.groupBox3.SuspendLayout();
         this.SuspendLayout();
         // 
         // ExportButton
         // 
         this.ExportButton.DialogResult = System.Windows.Forms.DialogResult.OK;
         this.ExportButton.Location = new System.Drawing.Point(314, 83);
         this.ExportButton.Name = "ExportButton";
         this.ExportButton.Size = new System.Drawing.Size(75, 23);
         this.ExportButton.TabIndex = 0;
         this.ExportButton.Text = "Export";
         this.ExportButton.UseVisualStyleBackColor = true;
         this.ExportButton.Click += new System.EventHandler(this.ExportButton_Click);
         // 
         // CancelExportButton
         // 
         this.CancelExportButton.DialogResult = System.Windows.Forms.DialogResult.Cancel;
         this.CancelExportButton.Location = new System.Drawing.Point(314, 117);
         this.CancelExportButton.Name = "CancelExportButton";
         this.CancelExportButton.Size = new System.Drawing.Size(75, 23);
         this.CancelExportButton.TabIndex = 1;
         this.CancelExportButton.Text = "Cancel";
         this.CancelExportButton.UseVisualStyleBackColor = true;
         this.CancelExportButton.Click += new System.EventHandler(this.CancelButton_Click);
         // 
         // ExportLevelListBox
         // 
         this.ExportLevelListBox.FormattingEnabled = true;
         this.ExportLevelListBox.Items.AddRange(new object[] {
            "Quick",
            "Final",
            "Custom"});
         this.ExportLevelListBox.Location = new System.Drawing.Point(289, 31);
         this.ExportLevelListBox.Name = "ExportLevelListBox";
         this.ExportLevelListBox.Size = new System.Drawing.Size(100, 43);
         this.ExportLevelListBox.TabIndex = 3;
         this.ExportLevelListBox.SelectedIndexChanged += new System.EventHandler(this.ExportLevelListBox_SelectedIndexChanged);
         // 
         // lodFactorSlider
         // 
         this.lodFactorSlider.Location = new System.Drawing.Point(6, 49);
         this.lodFactorSlider.Maximum = 20;
         this.lodFactorSlider.Name = "lodFactorSlider";
         this.lodFactorSlider.Size = new System.Drawing.Size(227, 45);
         this.lodFactorSlider.TabIndex = 4;
         this.lodFactorSlider.Value = 10;
         this.lodFactorSlider.Scroll += new System.EventHandler(this.lodFactorSlider_Scroll);
         // 
         // groupBox1
         // 
         this.groupBox1.Controls.Add(this.lodlabel);
         this.groupBox1.Controls.Add(this.label2);
         this.groupBox1.Controls.Add(this.biasLabel);
         this.groupBox1.Controls.Add(this.label1);
         this.groupBox1.Controls.Add(this.factorLabel);
         this.groupBox1.Controls.Add(this.lodFactorSlider);
         this.groupBox1.Location = new System.Drawing.Point(12, 12);
         this.groupBox1.Name = "groupBox1";
         this.groupBox1.Size = new System.Drawing.Size(239, 110);
         this.groupBox1.TabIndex = 7;
         this.groupBox1.TabStop = false;
         this.groupBox1.Text = "LOD";
         this.groupBox1.Enter += new System.EventHandler(this.groupBox1_Enter);
         // 
         // lodlabel
         // 
         this.lodlabel.AutoSize = true;
         this.lodlabel.Location = new System.Drawing.Point(113, 36);
         this.lodlabel.Name = "lodlabel";
         this.lodlabel.Size = new System.Drawing.Size(19, 13);
         this.lodlabel.TabIndex = 11;
         this.lodlabel.Text = "10";
         // 
         // label2
         // 
         this.label2.AutoSize = true;
         this.label2.Location = new System.Drawing.Point(194, 39);
         this.label2.Name = "label2";
         this.label2.Size = new System.Drawing.Size(26, 13);
         this.label2.TabIndex = 10;
         this.label2.Text = "Perf";
         // 
         // biasLabel
         // 
         this.biasLabel.AutoSize = true;
         this.biasLabel.Location = new System.Drawing.Point(116, 111);
         this.biasLabel.Name = "biasLabel";
         this.biasLabel.Size = new System.Drawing.Size(0, 13);
         this.biasLabel.TabIndex = 8;
         // 
         // label1
         // 
         this.label1.AutoSize = true;
         this.label1.Location = new System.Drawing.Point(6, 39);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(39, 13);
         this.label1.TabIndex = 9;
         this.label1.Text = "Quality";
         // 
         // factorLabel
         // 
         this.factorLabel.AutoSize = true;
         this.factorLabel.Location = new System.Drawing.Point(116, 49);
         this.factorLabel.Name = "factorLabel";
         this.factorLabel.Size = new System.Drawing.Size(0, 13);
         this.factorLabel.TabIndex = 7;
         // 
         // groupBox2
         // 
         this.groupBox2.Controls.Add(this.unqLabel);
         this.groupBox2.Controls.Add(this.UniRes);
         this.groupBox2.Location = new System.Drawing.Point(12, 254);
         this.groupBox2.Name = "groupBox2";
         this.groupBox2.Size = new System.Drawing.Size(239, 88);
         this.groupBox2.TabIndex = 8;
         this.groupBox2.TabStop = false;
         this.groupBox2.Text = "Visuals";
         // 
         // unqLabel
         // 
         this.unqLabel.AutoSize = true;
         this.unqLabel.Location = new System.Drawing.Point(6, 16);
         this.unqLabel.Name = "unqLabel";
         this.unqLabel.Size = new System.Drawing.Size(177, 13);
         this.unqLabel.TabIndex = 2;
         this.unqLabel.Text = "Unique Tex Res (perChunk):  64x64";
         // 
         // UniRes
         // 
         this.UniRes.Enabled = false;
         this.UniRes.Location = new System.Drawing.Point(79, 32);
         this.UniRes.Maximum = 5;
         this.UniRes.Name = "UniRes";
         this.UniRes.Size = new System.Drawing.Size(104, 45);
         this.UniRes.TabIndex = 1;
         this.UniRes.Value = 2;
         this.UniRes.Scroll += new System.EventHandler(this.UniRes_Scroll);
         // 
         // label3
         // 
         this.label3.AutoSize = true;
         this.label3.Location = new System.Drawing.Point(3, 28);
         this.label3.Name = "label3";
         this.label3.Size = new System.Drawing.Size(104, 13);
         this.label3.TabIndex = 4;
         this.label3.Text = "Ambient Occlusion : ";
         // 
         // comboBox1
         // 
         this.comboBox1.FormattingEnabled = true;
         this.comboBox1.Items.AddRange(new object[] {
            "OFF",
            "Worst",
            "Medium",
            "Best",
            "Final"});
         this.comboBox1.Location = new System.Drawing.Point(113, 25);
         this.comboBox1.Name = "comboBox1";
         this.comboBox1.Size = new System.Drawing.Size(117, 21);
         this.comboBox1.TabIndex = 3;
         this.comboBox1.SelectedIndexChanged += new System.EventHandler(this.comboBox1_SelectedIndexChanged);
         // 
         // checkBox1
         // 
         this.checkBox1.AutoSize = true;
         this.checkBox1.Location = new System.Drawing.Point(101, 55);
         this.checkBox1.Name = "checkBox1";
         this.checkBox1.Size = new System.Drawing.Size(129, 17);
         this.checkBox1.TabIndex = 5;
         this.checkBox1.Text = "Include Objects in AO";
         this.checkBox1.UseVisualStyleBackColor = true;
         this.checkBox1.CheckedChanged += new System.EventHandler(this.checkBox1_CheckedChanged);
         // 
         // groupBox3
         // 
         this.groupBox3.Controls.Add(this.checkBox1);
         this.groupBox3.Controls.Add(this.label3);
         this.groupBox3.Controls.Add(this.comboBox1);
         this.groupBox3.Enabled = false;
         this.groupBox3.Location = new System.Drawing.Point(12, 139);
         this.groupBox3.Name = "groupBox3";
         this.groupBox3.Size = new System.Drawing.Size(239, 109);
         this.groupBox3.TabIndex = 9;
         this.groupBox3.TabStop = false;
         this.groupBox3.Text = "Precomputed Lighting";
         // 
         // ExportDialog
         // 
         this.AcceptButton = this.ExportButton;
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.ClientSize = new System.Drawing.Size(404, 364);
         this.Controls.Add(this.groupBox3);
         this.Controls.Add(this.groupBox2);
         this.Controls.Add(this.groupBox1);
         this.Controls.Add(this.ExportLevelListBox);
         this.Controls.Add(this.CancelExportButton);
         this.Controls.Add(this.ExportButton);
         this.Name = "ExportDialog";
         this.Text = "Export";
         this.Load += new System.EventHandler(this.ExportDialog_Load);
         ((System.ComponentModel.ISupportInitialize)(this.lodFactorSlider)).EndInit();
         this.groupBox1.ResumeLayout(false);
         this.groupBox1.PerformLayout();
         this.groupBox2.ResumeLayout(false);
         this.groupBox2.PerformLayout();
         ((System.ComponentModel.ISupportInitialize)(this.UniRes)).EndInit();
         this.groupBox3.ResumeLayout(false);
         this.groupBox3.PerformLayout();
         this.ResumeLayout(false);

      }

      #endregion

      private System.Windows.Forms.Button ExportButton;
      private System.Windows.Forms.Button CancelExportButton;
      private System.Windows.Forms.ListBox ExportLevelListBox;
      private System.Windows.Forms.TrackBar lodFactorSlider;
      private System.Windows.Forms.GroupBox groupBox1;
      private System.Windows.Forms.Label factorLabel;
      private System.Windows.Forms.Label biasLabel;
      private System.Windows.Forms.GroupBox groupBox2;
      private System.Windows.Forms.Label label1;
      private System.Windows.Forms.Label label2;
      private System.Windows.Forms.Label unqLabel;
      private System.Windows.Forms.TrackBar UniRes;
      private System.Windows.Forms.Label label3;
      private System.Windows.Forms.ComboBox comboBox1;
      private System.Windows.Forms.Label lodlabel;
      private System.Windows.Forms.CheckBox checkBox1;
      private System.Windows.Forms.GroupBox groupBox3;
   }
}