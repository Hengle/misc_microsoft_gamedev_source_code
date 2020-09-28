namespace PhoenixEditor
{
   partial class BatchExporter
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
         this.ToExportListBox = new System.Windows.Forms.ListBox();
         this.Exportbutton = new System.Windows.Forms.Button();
         this.progressBar1 = new System.Windows.Forms.ProgressBar();
         this.CancelExportButton = new System.Windows.Forms.Button();
         this.label1 = new System.Windows.Forms.Label();
         this.label2 = new System.Windows.Forms.Label();
         this.checkBox1 = new System.Windows.Forms.CheckBox();
         this.listBox1 = new System.Windows.Forms.ListBox();
         this.groupBox1 = new System.Windows.Forms.GroupBox();
         this.radioButton4 = new System.Windows.Forms.RadioButton();
         this.radioButton3 = new System.Windows.Forms.RadioButton();
         this.radioButton2 = new System.Windows.Forms.RadioButton();
         this.radioButton1 = new System.Windows.Forms.RadioButton();
         this.groupBox2 = new System.Windows.Forms.GroupBox();
         this.usePerforce = new System.Windows.Forms.CheckBox();
         this.checkBox2 = new System.Windows.Forms.CheckBox();
         this.groupBox3 = new System.Windows.Forms.GroupBox();
         this.doLRP = new System.Windows.Forms.CheckBox();
         this.doDEP = new System.Windows.Forms.CheckBox();
         this.doXMB = new System.Windows.Forms.CheckBox();
         this.doTAG = new System.Windows.Forms.CheckBox();
         this.doXSD = new System.Windows.Forms.CheckBox();
         this.doXTH = new System.Windows.Forms.CheckBox();
         this.doXTT = new System.Windows.Forms.CheckBox();
         this.doXTD = new System.Windows.Forms.CheckBox();
         this.groupBox1.SuspendLayout();
         this.groupBox2.SuspendLayout();
         this.groupBox3.SuspendLayout();
         this.SuspendLayout();
         // 
         // ToExportListBox
         // 
         this.ToExportListBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.ToExportListBox.FormattingEnabled = true;
         this.ToExportListBox.Location = new System.Drawing.Point(12, 31);
         this.ToExportListBox.Name = "ToExportListBox";
         this.ToExportListBox.SelectionMode = System.Windows.Forms.SelectionMode.MultiExtended;
         this.ToExportListBox.Size = new System.Drawing.Size(569, 199);
         this.ToExportListBox.TabIndex = 0;
         // 
         // Exportbutton
         // 
         this.Exportbutton.Location = new System.Drawing.Point(411, 377);
         this.Exportbutton.Name = "Exportbutton";
         this.Exportbutton.Size = new System.Drawing.Size(82, 23);
         this.Exportbutton.TabIndex = 1;
         this.Exportbutton.Text = "Export";
         this.Exportbutton.UseVisualStyleBackColor = true;
         this.Exportbutton.Click += new System.EventHandler(this.Exportbutton_Click);
         // 
         // progressBar1
         // 
         this.progressBar1.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.progressBar1.Location = new System.Drawing.Point(11, 420);
         this.progressBar1.Name = "progressBar1";
         this.progressBar1.Size = new System.Drawing.Size(570, 23);
         this.progressBar1.TabIndex = 2;
         // 
         // CancelExportButton
         // 
         this.CancelExportButton.Location = new System.Drawing.Point(499, 377);
         this.CancelExportButton.Name = "CancelExportButton";
         this.CancelExportButton.Size = new System.Drawing.Size(82, 23);
         this.CancelExportButton.TabIndex = 3;
         this.CancelExportButton.Text = "Done";
         this.CancelExportButton.UseVisualStyleBackColor = true;
         this.CancelExportButton.Click += new System.EventHandler(this.CancelButton_Click);
         // 
         // label1
         // 
         this.label1.AutoSize = true;
         this.label1.Location = new System.Drawing.Point(9, 404);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(42, 13);
         this.label1.TabIndex = 5;
         this.label1.Text = "Output:";
         // 
         // label2
         // 
         this.label2.AutoSize = true;
         this.label2.Location = new System.Drawing.Point(12, 15);
         this.label2.Name = "label2";
         this.label2.Size = new System.Drawing.Size(230, 13);
         this.label2.TabIndex = 6;
         this.label2.Text = "Select the following Scenarios to be converted:";
         // 
         // checkBox1
         // 
         this.checkBox1.AutoSize = true;
         this.checkBox1.Checked = true;
         this.checkBox1.CheckState = System.Windows.Forms.CheckState.Checked;
         this.checkBox1.Location = new System.Drawing.Point(14, 43);
         this.checkBox1.Name = "checkBox1";
         this.checkBox1.Size = new System.Drawing.Size(140, 17);
         this.checkBox1.TabIndex = 7;
         this.checkBox1.Text = "Check out from perforce";
         this.checkBox1.UseVisualStyleBackColor = true;
         this.checkBox1.CheckedChanged += new System.EventHandler(this.checkBox1_CheckedChanged);
         // 
         // listBox1
         // 
         this.listBox1.FormattingEnabled = true;
         this.listBox1.Location = new System.Drawing.Point(12, 449);
         this.listBox1.Name = "listBox1";
         this.listBox1.Size = new System.Drawing.Size(569, 121);
         this.listBox1.TabIndex = 8;
         // 
         // groupBox1
         // 
         this.groupBox1.Controls.Add(this.radioButton4);
         this.groupBox1.Controls.Add(this.radioButton3);
         this.groupBox1.Controls.Add(this.radioButton2);
         this.groupBox1.Controls.Add(this.radioButton1);
         this.groupBox1.Location = new System.Drawing.Point(16, 236);
         this.groupBox1.Name = "groupBox1";
         this.groupBox1.Size = new System.Drawing.Size(166, 121);
         this.groupBox1.TabIndex = 10;
         this.groupBox1.TabStop = false;
         this.groupBox1.Text = "Export Settings";
         // 
         // radioButton4
         // 
         this.radioButton4.AutoSize = true;
         this.radioButton4.Location = new System.Drawing.Point(6, 65);
         this.radioButton4.Name = "radioButton4";
         this.radioButton4.Size = new System.Drawing.Size(94, 17);
         this.radioButton4.TabIndex = 3;
         this.radioButton4.TabStop = true;
         this.radioButton4.Text = "Quick Settings";
         this.radioButton4.UseVisualStyleBackColor = true;
         this.radioButton4.CheckedChanged += new System.EventHandler(this.radioButton4_CheckedChanged);
         // 
         // radioButton3
         // 
         this.radioButton3.AutoSize = true;
         this.radioButton3.Location = new System.Drawing.Point(6, 88);
         this.radioButton3.Name = "radioButton3";
         this.radioButton3.Size = new System.Drawing.Size(88, 17);
         this.radioButton3.TabIndex = 2;
         this.radioButton3.Text = "Final Settings";
         this.radioButton3.UseVisualStyleBackColor = true;
         this.radioButton3.CheckedChanged += new System.EventHandler(this.radioButton3_CheckedChanged);
         // 
         // radioButton2
         // 
         this.radioButton2.AutoSize = true;
         this.radioButton2.Location = new System.Drawing.Point(6, 42);
         this.radioButton2.Name = "radioButton2";
         this.radioButton2.Size = new System.Drawing.Size(101, 17);
         this.radioButton2.TabIndex = 1;
         this.radioButton2.Text = "Custom Settings";
         this.radioButton2.UseVisualStyleBackColor = true;
         this.radioButton2.CheckedChanged += new System.EventHandler(this.radioButton2_CheckedChanged);
         // 
         // radioButton1
         // 
         this.radioButton1.AutoSize = true;
         this.radioButton1.Checked = true;
         this.radioButton1.Location = new System.Drawing.Point(6, 19);
         this.radioButton1.Name = "radioButton1";
         this.radioButton1.Size = new System.Drawing.Size(148, 17);
         this.radioButton1.TabIndex = 0;
         this.radioButton1.TabStop = true;
         this.radioButton1.Text = "Scenario Defined Settings";
         this.radioButton1.UseVisualStyleBackColor = true;
         this.radioButton1.CheckedChanged += new System.EventHandler(this.radioButton1_CheckedChanged);
         // 
         // groupBox2
         // 
         this.groupBox2.Controls.Add(this.usePerforce);
         this.groupBox2.Controls.Add(this.checkBox2);
         this.groupBox2.Controls.Add(this.checkBox1);
         this.groupBox2.Location = new System.Drawing.Point(188, 236);
         this.groupBox2.Name = "groupBox2";
         this.groupBox2.Size = new System.Drawing.Size(160, 121);
         this.groupBox2.TabIndex = 11;
         this.groupBox2.TabStop = false;
         this.groupBox2.Text = "Perforce Settings";
         // 
         // usePerforce
         // 
         this.usePerforce.AutoSize = true;
         this.usePerforce.Checked = true;
         this.usePerforce.CheckState = System.Windows.Forms.CheckState.Checked;
         this.usePerforce.Location = new System.Drawing.Point(6, 19);
         this.usePerforce.Name = "usePerforce";
         this.usePerforce.Size = new System.Drawing.Size(88, 17);
         this.usePerforce.TabIndex = 9;
         this.usePerforce.Text = "Use Perforce";
         this.usePerforce.UseVisualStyleBackColor = true;
         this.usePerforce.CheckedChanged += new System.EventHandler(this.usePerforce_CheckedChanged);
         // 
         // checkBox2
         // 
         this.checkBox2.AutoSize = true;
         this.checkBox2.Location = new System.Drawing.Point(14, 66);
         this.checkBox2.Name = "checkBox2";
         this.checkBox2.Size = new System.Drawing.Size(136, 17);
         this.checkBox2.TabIndex = 8;
         this.checkBox2.Text = "Check in when finished";
         this.checkBox2.UseVisualStyleBackColor = true;
         this.checkBox2.CheckedChanged += new System.EventHandler(this.checkBox2_CheckedChanged);
         // 
         // groupBox3
         // 
         this.groupBox3.Controls.Add(this.doLRP);
         this.groupBox3.Controls.Add(this.doDEP);
         this.groupBox3.Controls.Add(this.doXMB);
         this.groupBox3.Controls.Add(this.doTAG);
         this.groupBox3.Controls.Add(this.doXSD);
         this.groupBox3.Controls.Add(this.doXTH);
         this.groupBox3.Controls.Add(this.doXTT);
         this.groupBox3.Controls.Add(this.doXTD);
         this.groupBox3.Location = new System.Drawing.Point(354, 236);
         this.groupBox3.Name = "groupBox3";
         this.groupBox3.Size = new System.Drawing.Size(139, 121);
         this.groupBox3.TabIndex = 12;
         this.groupBox3.TabStop = false;
         this.groupBox3.Text = "Filter List";
         // 
         // doLRP
         // 
         this.doLRP.AutoSize = true;
         this.doLRP.Checked = true;
         this.doLRP.CheckState = System.Windows.Forms.CheckState.Checked;
         this.doLRP.Location = new System.Drawing.Point(71, 88);
         this.doLRP.Name = "doLRP";
         this.doLRP.Size = new System.Drawing.Size(47, 17);
         this.doLRP.TabIndex = 8;
         this.doLRP.Text = "LRP";
         this.doLRP.UseVisualStyleBackColor = true;
         this.doLRP.CheckedChanged += new System.EventHandler(this.doLRP_CheckedChanged);
         // 
         // doDEP
         // 
         this.doDEP.AutoSize = true;
         this.doDEP.Checked = true;
         this.doDEP.CheckState = System.Windows.Forms.CheckState.Checked;
         this.doDEP.Location = new System.Drawing.Point(71, 66);
         this.doDEP.Name = "doDEP";
         this.doDEP.Size = new System.Drawing.Size(48, 17);
         this.doDEP.TabIndex = 7;
         this.doDEP.Text = "DEP";
         this.doDEP.UseVisualStyleBackColor = true;
         this.doDEP.CheckedChanged += new System.EventHandler(this.doDEP_CheckedChanged);
         // 
         // doXMB
         // 
         this.doXMB.AutoSize = true;
         this.doXMB.Checked = true;
         this.doXMB.CheckState = System.Windows.Forms.CheckState.Checked;
         this.doXMB.Location = new System.Drawing.Point(72, 43);
         this.doXMB.Name = "doXMB";
         this.doXMB.Size = new System.Drawing.Size(49, 17);
         this.doXMB.TabIndex = 6;
         this.doXMB.Text = "XMB";
         this.doXMB.UseVisualStyleBackColor = true;
         // 
         // doTAG
         // 
         this.doTAG.AutoSize = true;
         this.doTAG.Checked = true;
         this.doTAG.CheckState = System.Windows.Forms.CheckState.Checked;
         this.doTAG.Location = new System.Drawing.Point(72, 19);
         this.doTAG.Name = "doTAG";
         this.doTAG.Size = new System.Drawing.Size(48, 17);
         this.doTAG.TabIndex = 4;
         this.doTAG.Text = "TAG";
         this.doTAG.UseVisualStyleBackColor = true;
         this.doTAG.CheckedChanged += new System.EventHandler(this.doTAG_CheckedChanged);
         // 
         // doXSD
         // 
         this.doXSD.AutoSize = true;
         this.doXSD.Checked = true;
         this.doXSD.CheckState = System.Windows.Forms.CheckState.Checked;
         this.doXSD.Location = new System.Drawing.Point(6, 88);
         this.doXSD.Name = "doXSD";
         this.doXSD.Size = new System.Drawing.Size(48, 17);
         this.doXSD.TabIndex = 3;
         this.doXSD.Text = "XSD";
         this.doXSD.UseVisualStyleBackColor = true;
         // 
         // doXTH
         // 
         this.doXTH.AutoSize = true;
         this.doXTH.Checked = true;
         this.doXTH.CheckState = System.Windows.Forms.CheckState.Checked;
         this.doXTH.Location = new System.Drawing.Point(6, 65);
         this.doXTH.Name = "doXTH";
         this.doXTH.Size = new System.Drawing.Size(48, 17);
         this.doXTH.TabIndex = 2;
         this.doXTH.Text = "XTH";
         this.doXTH.UseVisualStyleBackColor = true;
         // 
         // doXTT
         // 
         this.doXTT.AutoSize = true;
         this.doXTT.Checked = true;
         this.doXTT.CheckState = System.Windows.Forms.CheckState.Checked;
         this.doXTT.Location = new System.Drawing.Point(6, 42);
         this.doXTT.Name = "doXTT";
         this.doXTT.Size = new System.Drawing.Size(47, 17);
         this.doXTT.TabIndex = 1;
         this.doXTT.Text = "XTT";
         this.doXTT.UseVisualStyleBackColor = true;
         // 
         // doXTD
         // 
         this.doXTD.AutoSize = true;
         this.doXTD.Checked = true;
         this.doXTD.CheckState = System.Windows.Forms.CheckState.Checked;
         this.doXTD.Location = new System.Drawing.Point(6, 19);
         this.doXTD.Name = "doXTD";
         this.doXTD.Size = new System.Drawing.Size(48, 17);
         this.doXTD.TabIndex = 0;
         this.doXTD.Text = "XTD";
         this.doXTD.UseVisualStyleBackColor = true;
         // 
         // BatchExporter
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.ClientSize = new System.Drawing.Size(593, 583);
         this.Controls.Add(this.groupBox3);
         this.Controls.Add(this.groupBox2);
         this.Controls.Add(this.groupBox1);
         this.Controls.Add(this.listBox1);
         this.Controls.Add(this.label2);
         this.Controls.Add(this.label1);
         this.Controls.Add(this.CancelExportButton);
         this.Controls.Add(this.progressBar1);
         this.Controls.Add(this.Exportbutton);
         this.Controls.Add(this.ToExportListBox);
         this.Name = "BatchExporter";
         this.Text = "Exporter";
         this.groupBox1.ResumeLayout(false);
         this.groupBox1.PerformLayout();
         this.groupBox2.ResumeLayout(false);
         this.groupBox2.PerformLayout();
         this.groupBox3.ResumeLayout(false);
         this.groupBox3.PerformLayout();
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private System.Windows.Forms.ListBox ToExportListBox;
      private System.Windows.Forms.Button Exportbutton;
      private System.Windows.Forms.ProgressBar progressBar1;
      private System.Windows.Forms.Button CancelExportButton;
      private System.Windows.Forms.Label label1;
      private System.Windows.Forms.Label label2;
      private System.Windows.Forms.CheckBox checkBox1;
      private System.Windows.Forms.ListBox listBox1;
      private System.Windows.Forms.GroupBox groupBox1;
      private System.Windows.Forms.RadioButton radioButton3;
      private System.Windows.Forms.RadioButton radioButton2;
      private System.Windows.Forms.RadioButton radioButton1;
      private System.Windows.Forms.GroupBox groupBox2;
      private System.Windows.Forms.GroupBox groupBox3;
      private System.Windows.Forms.CheckBox doXSD;
      private System.Windows.Forms.CheckBox doXTH;
      private System.Windows.Forms.CheckBox doXTT;
      private System.Windows.Forms.CheckBox doXTD;
      private System.Windows.Forms.CheckBox doTAG;
      private System.Windows.Forms.CheckBox doXMB;
      private System.Windows.Forms.CheckBox checkBox2;
      private System.Windows.Forms.CheckBox usePerforce;
      private System.Windows.Forms.CheckBox doDEP;
      private System.Windows.Forms.CheckBox doLRP;
      private System.Windows.Forms.RadioButton radioButton4;
   }
}