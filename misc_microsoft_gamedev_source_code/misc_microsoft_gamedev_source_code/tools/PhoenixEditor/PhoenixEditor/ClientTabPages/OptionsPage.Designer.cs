namespace PhoenixEditor.ClientTabPages
{
   partial class OptionsPage
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
         this.tabControl1 = new System.Windows.Forms.TabControl();
         this.editorOptionsTab = new System.Windows.Forms.TabPage();
         this.groupBox2 = new System.Windows.Forms.GroupBox();
         this.zoomCamLabel = new System.Windows.Forms.Label();
         this.transCamLabel = new System.Windows.Forms.Label();
         this.label9 = new System.Windows.Forms.Label();
         this.label8 = new System.Windows.Forms.Label();
         this.StaticKeysLabel = new System.Windows.Forms.Label();
         this.label4 = new System.Windows.Forms.Label();
         this.rotCamLabel = new System.Windows.Forms.Label();
         this.label2 = new System.Windows.Forms.Label();
         this.keyOptionsBox = new System.Windows.Forms.ComboBox();
         this.groupBox1 = new System.Windows.Forms.GroupBox();
         this.label1 = new System.Windows.Forms.Label();
         this.d3dResBox = new System.Windows.Forms.ComboBox();
         this.optionsOK = new System.Windows.Forms.Button();
         this.tabControl1.SuspendLayout();
         this.editorOptionsTab.SuspendLayout();
         this.groupBox2.SuspendLayout();
         this.groupBox1.SuspendLayout();
         this.SuspendLayout();
         // 
         // tabControl1
         // 
         this.tabControl1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.tabControl1.Controls.Add(this.editorOptionsTab);
         this.tabControl1.Location = new System.Drawing.Point(3, 3);
         this.tabControl1.Multiline = true;
         this.tabControl1.Name = "tabControl1";
         this.tabControl1.SelectedIndex = 0;
         this.tabControl1.Size = new System.Drawing.Size(855, 586);
         this.tabControl1.TabIndex = 0;
         // 
         // editorOptionsTab
         // 
         this.editorOptionsTab.Controls.Add(this.groupBox2);
         this.editorOptionsTab.Controls.Add(this.groupBox1);
         this.editorOptionsTab.Location = new System.Drawing.Point(4, 22);
         this.editorOptionsTab.Name = "editorOptionsTab";
         this.editorOptionsTab.Padding = new System.Windows.Forms.Padding(3);
         this.editorOptionsTab.Size = new System.Drawing.Size(847, 560);
         this.editorOptionsTab.TabIndex = 0;
         this.editorOptionsTab.Text = "Editor Options";
         this.editorOptionsTab.UseVisualStyleBackColor = true;
         // 
         // groupBox2
         // 
         this.groupBox2.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.groupBox2.Controls.Add(this.zoomCamLabel);
         this.groupBox2.Controls.Add(this.transCamLabel);
         this.groupBox2.Controls.Add(this.label9);
         this.groupBox2.Controls.Add(this.label8);
         this.groupBox2.Controls.Add(this.StaticKeysLabel);
         this.groupBox2.Controls.Add(this.label4);
         this.groupBox2.Controls.Add(this.rotCamLabel);
         this.groupBox2.Controls.Add(this.label2);
         this.groupBox2.Controls.Add(this.keyOptionsBox);
         this.groupBox2.Location = new System.Drawing.Point(6, 94);
         this.groupBox2.Name = "groupBox2";
         this.groupBox2.Size = new System.Drawing.Size(835, 460);
         this.groupBox2.TabIndex = 3;
         this.groupBox2.TabStop = false;
         this.groupBox2.Text = "Keyboard Settings";
         // 
         // zoomCamLabel
         // 
         this.zoomCamLabel.AutoSize = true;
         this.zoomCamLabel.Location = new System.Drawing.Point(133, 139);
         this.zoomCamLabel.Name = "zoomCamLabel";
         this.zoomCamLabel.Size = new System.Drawing.Size(41, 13);
         this.zoomCamLabel.TabIndex = 10;
         this.zoomCamLabel.Text = "label11";
         // 
         // transCamLabel
         // 
         this.transCamLabel.AutoSize = true;
         this.transCamLabel.Location = new System.Drawing.Point(133, 85);
         this.transCamLabel.Name = "transCamLabel";
         this.transCamLabel.Size = new System.Drawing.Size(41, 13);
         this.transCamLabel.TabIndex = 9;
         this.transCamLabel.Text = "label10";
         // 
         // label9
         // 
         this.label9.AutoSize = true;
         this.label9.Location = new System.Drawing.Point(16, 112);
         this.label9.Name = "label9";
         this.label9.Size = new System.Drawing.Size(84, 13);
         this.label9.TabIndex = 8;
         this.label9.Text = "Rotate Camera :";
         // 
         // label8
         // 
         this.label8.AutoSize = true;
         this.label8.Location = new System.Drawing.Point(16, 139);
         this.label8.Name = "label8";
         this.label8.Size = new System.Drawing.Size(79, 13);
         this.label8.TabIndex = 7;
         this.label8.Text = "Zoom Camera :";
         // 
         // StaticKeysLabel
         // 
         this.StaticKeysLabel.AutoSize = true;
         this.StaticKeysLabel.Location = new System.Drawing.Point(413, 21);
         this.StaticKeysLabel.Name = "StaticKeysLabel";
         this.StaticKeysLabel.Size = new System.Drawing.Size(81, 13);
         this.StaticKeysLabel.TabIndex = 6;
         this.StaticKeysLabel.Text = "staticKeysLabel";
         // 
         // label4
         // 
         this.label4.AutoSize = true;
         this.label4.Location = new System.Drawing.Point(16, 21);
         this.label4.Name = "label4";
         this.label4.Size = new System.Drawing.Size(73, 13);
         this.label4.TabIndex = 3;
         this.label4.Text = "Control Mode:";
         // 
         // rotCamLabel
         // 
         this.rotCamLabel.AutoSize = true;
         this.rotCamLabel.Location = new System.Drawing.Point(133, 112);
         this.rotCamLabel.Name = "rotCamLabel";
         this.rotCamLabel.Size = new System.Drawing.Size(35, 13);
         this.rotCamLabel.TabIndex = 2;
         this.rotCamLabel.Text = "label3";
         // 
         // label2
         // 
         this.label2.AutoSize = true;
         this.label2.Location = new System.Drawing.Point(16, 85);
         this.label2.Name = "label2";
         this.label2.Size = new System.Drawing.Size(99, 13);
         this.label2.TabIndex = 1;
         this.label2.Text = "Translate Camera : ";
         // 
         // keyOptionsBox
         // 
         this.keyOptionsBox.FormattingEnabled = true;
         this.keyOptionsBox.Items.AddRange(new object[] {
            "Artist Mode (default)",
            "Designer Mode"});
         this.keyOptionsBox.Location = new System.Drawing.Point(19, 37);
         this.keyOptionsBox.Name = "keyOptionsBox";
         this.keyOptionsBox.Size = new System.Drawing.Size(121, 21);
         this.keyOptionsBox.TabIndex = 0;
         this.keyOptionsBox.SelectedIndexChanged += new System.EventHandler(this.keyOptionsBox_SelectedIndexChanged);
         // 
         // groupBox1
         // 
         this.groupBox1.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.groupBox1.Controls.Add(this.label1);
         this.groupBox1.Controls.Add(this.d3dResBox);
         this.groupBox1.Location = new System.Drawing.Point(6, 6);
         this.groupBox1.Name = "groupBox1";
         this.groupBox1.Size = new System.Drawing.Size(835, 82);
         this.groupBox1.TabIndex = 2;
         this.groupBox1.TabStop = false;
         this.groupBox1.Text = "D3D Options";
         // 
         // label1
         // 
         this.label1.AutoSize = true;
         this.label1.Location = new System.Drawing.Point(16, 33);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(136, 13);
         this.label1.TabIndex = 0;
         this.label1.Text = "D3D BackBuffer resolution:";
         // 
         // d3dResBox
         // 
         this.d3dResBox.FormattingEnabled = true;
         this.d3dResBox.Items.AddRange(new object[] {
            "640x480",
            "800x600",
            "1024x768",
            "Match To Client Window"});
         this.d3dResBox.Location = new System.Drawing.Point(19, 49);
         this.d3dResBox.Name = "d3dResBox";
         this.d3dResBox.Size = new System.Drawing.Size(155, 21);
         this.d3dResBox.TabIndex = 1;
         this.d3dResBox.SelectedIndexChanged += new System.EventHandler(this.d3dResBox_SelectedIndexChanged);
         // 
         // optionsOK
         // 
         this.optionsOK.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
         this.optionsOK.Location = new System.Drawing.Point(779, 591);
         this.optionsOK.Name = "optionsOK";
         this.optionsOK.Size = new System.Drawing.Size(75, 23);
         this.optionsOK.TabIndex = 2;
         this.optionsOK.Text = "Apply";
         this.optionsOK.UseVisualStyleBackColor = true;
         this.optionsOK.Click += new System.EventHandler(this.optionsOK_Click);
         // 
         // OptionsPage
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.optionsOK);
         this.Controls.Add(this.tabControl1);
         this.Name = "OptionsPage";
         this.Size = new System.Drawing.Size(861, 627);
         this.Load += new System.EventHandler(this.OptionsPage_Load);
         this.tabControl1.ResumeLayout(false);
         this.editorOptionsTab.ResumeLayout(false);
         this.groupBox2.ResumeLayout(false);
         this.groupBox2.PerformLayout();
         this.groupBox1.ResumeLayout(false);
         this.groupBox1.PerformLayout();
         this.ResumeLayout(false);

      }

      #endregion

      private System.Windows.Forms.TabControl tabControl1;
      private System.Windows.Forms.TabPage editorOptionsTab;
      private System.Windows.Forms.Button optionsOK;
      private System.Windows.Forms.ComboBox d3dResBox;
      private System.Windows.Forms.Label label1;
      private System.Windows.Forms.GroupBox groupBox2;
      private System.Windows.Forms.ComboBox keyOptionsBox;
      private System.Windows.Forms.GroupBox groupBox1;
      private System.Windows.Forms.Label zoomCamLabel;
      private System.Windows.Forms.Label transCamLabel;
      private System.Windows.Forms.Label label9;
      private System.Windows.Forms.Label label8;
      private System.Windows.Forms.Label StaticKeysLabel;
      private System.Windows.Forms.Label label4;
      private System.Windows.Forms.Label rotCamLabel;
      private System.Windows.Forms.Label label2;
   }
}
