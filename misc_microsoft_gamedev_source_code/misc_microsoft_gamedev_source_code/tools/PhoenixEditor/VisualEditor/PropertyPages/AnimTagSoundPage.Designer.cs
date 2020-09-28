namespace VisualEditor.PropertyPages
{
   partial class AnimTagSoundPage
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
         System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(AnimTagSoundPage));
         this.groupBox1 = new System.Windows.Forms.GroupBox();
         this.checkBoxFOW = new System.Windows.Forms.CheckBox();
         this.RefreshButton = new System.Windows.Forms.Button();
         this.toBoneComboBox = new System.Windows.Forms.ComboBox();
         this.toBoneLabel = new System.Windows.Forms.Label();
         this.visibleCheckBox = new System.Windows.Forms.CheckBox();
         this.label1 = new System.Windows.Forms.Label();
         this.label2 = new System.Windows.Forms.Label();
         this.soundBrowseControl1 = new VisualEditor.Controls.SoundBrowseControl();
         this.floatSliderEdit1 = new VisualEditor.Controls.FloatSliderEdit();
         this.groupBox1.SuspendLayout();
         this.SuspendLayout();
         // 
         // groupBox1
         // 
         this.groupBox1.Controls.Add(this.checkBoxFOW);
         this.groupBox1.Controls.Add(this.soundBrowseControl1);
         this.groupBox1.Controls.Add(this.RefreshButton);
         this.groupBox1.Controls.Add(this.toBoneComboBox);
         this.groupBox1.Controls.Add(this.toBoneLabel);
         this.groupBox1.Controls.Add(this.visibleCheckBox);
         this.groupBox1.Controls.Add(this.label1);
         this.groupBox1.Controls.Add(this.floatSliderEdit1);
         this.groupBox1.Controls.Add(this.label2);
         this.groupBox1.Location = new System.Drawing.Point(0, 0);
         this.groupBox1.Name = "groupBox1";
         this.groupBox1.Padding = new System.Windows.Forms.Padding(5, 16, 5, 3);
         this.groupBox1.Size = new System.Drawing.Size(350, 250);
         this.groupBox1.TabIndex = 1;
         this.groupBox1.TabStop = false;
         this.groupBox1.Text = "Sound Tag Properties";
         // 
         // checkBoxFOW
         // 
         this.checkBoxFOW.AutoSize = true;
         this.checkBoxFOW.Location = new System.Drawing.Point(11, 157);
         this.checkBoxFOW.Name = "checkBoxFOW";
         this.checkBoxFOW.Size = new System.Drawing.Size(85, 17);
         this.checkBoxFOW.TabIndex = 31;
         this.checkBoxFOW.Text = "Do not play in FOW";
         this.checkBoxFOW.UseVisualStyleBackColor = true;
         this.checkBoxFOW.CheckedChanged += new System.EventHandler(this.checkBoxFOW_CheckedChanged);
         // 
         // RefreshButton
         // 
         this.RefreshButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
         this.RefreshButton.Image = ((System.Drawing.Image)(resources.GetObject("RefreshButton.Image")));
         this.RefreshButton.Location = new System.Drawing.Point(318, 34);
         this.RefreshButton.Margin = new System.Windows.Forms.Padding(0);
         this.RefreshButton.Name = "RefreshButton";
         this.RefreshButton.Size = new System.Drawing.Size(24, 24);
         this.RefreshButton.TabIndex = 15;
         this.RefreshButton.UseVisualStyleBackColor = true;
         this.RefreshButton.Click += new System.EventHandler(this.RefreshButton_Click);
         // 
         // toBoneComboBox
         // 
         this.toBoneComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
         this.toBoneComboBox.FormattingEnabled = true;
         this.toBoneComboBox.Location = new System.Drawing.Point(102, 105);
         this.toBoneComboBox.Name = "toBoneComboBox";
         this.toBoneComboBox.Size = new System.Drawing.Size(240, 21);
         this.toBoneComboBox.TabIndex = 29;
         this.toBoneComboBox.SelectedIndexChanged += new System.EventHandler(this.toBoneComboBox_SelectedIndexChanged);
         // 
         // toBoneLabel
         // 
         this.toBoneLabel.AutoSize = true;
         this.toBoneLabel.Location = new System.Drawing.Point(8, 107);
         this.toBoneLabel.Margin = new System.Windows.Forms.Padding(3);
         this.toBoneLabel.Name = "toBoneLabel";
         this.toBoneLabel.Size = new System.Drawing.Size(48, 13);
         this.toBoneLabel.TabIndex = 28;
         this.toBoneLabel.Text = "To Bone";
         // 
         // visibleCheckBox
         // 
         this.visibleCheckBox.AutoSize = true;
         this.visibleCheckBox.Location = new System.Drawing.Point(11, 134);
         this.visibleCheckBox.Name = "visibleCheckBox";
         this.visibleCheckBox.Size = new System.Drawing.Size(116, 17);
         this.visibleCheckBox.TabIndex = 6;
         this.visibleCheckBox.Text = "Do not play past max radius";
         this.visibleCheckBox.UseVisualStyleBackColor = true;
         this.visibleCheckBox.CheckedChanged += new System.EventHandler(this.visibleCheckBox_CheckedChanged);
         // 
         // label1
         // 
         this.label1.AutoSize = true;
         this.label1.Location = new System.Drawing.Point(8, 78);
         this.label1.Margin = new System.Windows.Forms.Padding(3);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(69, 13);
         this.label1.TabIndex = 5;
         this.label1.Text = "Sound Effect";
         // 
         // label2
         // 
         this.label2.AutoSize = true;
         this.label2.Location = new System.Drawing.Point(8, 34);
         this.label2.Name = "label2";
         this.label2.Size = new System.Drawing.Size(44, 13);
         this.label2.TabIndex = 3;
         this.label2.Text = "Position";
         // 
         // soundBrowseControl1
         // 
         this.soundBrowseControl1.FileName = "";
         this.soundBrowseControl1.Location = new System.Drawing.Point(102, 75);
         this.soundBrowseControl1.Name = "soundBrowseControl1";
         this.soundBrowseControl1.Size = new System.Drawing.Size(240, 24);
         this.soundBrowseControl1.TabIndex = 30;
         this.soundBrowseControl1.ValueChanged += new VisualEditor.Controls.SoundBrowseControl.ValueChangedDelegate(this.soundBrowseControl1_ValueChanged);
         // 
         // floatSliderEdit1
         // 
         this.floatSliderEdit1.Location = new System.Drawing.Point(102, 32);
         this.floatSliderEdit1.Margin = new System.Windows.Forms.Padding(3, 3, 3, 0);
         this.floatSliderEdit1.MaxValue = 1F;
         this.floatSliderEdit1.MinValue = 0F;
         this.floatSliderEdit1.Name = "floatSliderEdit1";
         this.floatSliderEdit1.NumDecimals = 3;
         this.floatSliderEdit1.Size = new System.Drawing.Size(213, 40);
         this.floatSliderEdit1.TabIndex = 4;
         this.floatSliderEdit1.Value = 0F;
         this.floatSliderEdit1.ValueChanged += new VisualEditor.Controls.FloatSliderEdit.ValueChangedDelegate(this.floatSliderEdit1_ValueChanged);
         // 
         // AnimTagSoundPage
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.groupBox1);
         this.Name = "AnimTagSoundPage";
         this.Size = new System.Drawing.Size(350, 250);
         this.groupBox1.ResumeLayout(false);
         this.groupBox1.PerformLayout();
         this.ResumeLayout(false);

      }

      #endregion

      private System.Windows.Forms.GroupBox groupBox1;
      private VisualEditor.Controls.FloatSliderEdit floatSliderEdit1;
      private System.Windows.Forms.Label label2;
      private System.Windows.Forms.CheckBox visibleCheckBox;
      private System.Windows.Forms.Label label1;
      private System.Windows.Forms.ComboBox toBoneComboBox;
      private System.Windows.Forms.Label toBoneLabel;
      private System.Windows.Forms.Button RefreshButton;
      private VisualEditor.Controls.SoundBrowseControl soundBrowseControl1;
      private System.Windows.Forms.CheckBox checkBoxFOW;
   }
}
