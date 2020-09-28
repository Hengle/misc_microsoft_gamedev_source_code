namespace VisualEditor.PropertyPages
{
   partial class AnimTagCameraShakePage
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
         System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(AnimTagCameraShakePage));
         this.groupBox1 = new System.Windows.Forms.GroupBox();
         this.toBoneComboBox = new System.Windows.Forms.ComboBox();
         this.toBoneLabel = new System.Windows.Forms.Label();
         this.RefreshButton = new System.Windows.Forms.Button();
         this.durationSliderEdit = new VisualEditor.Controls.FloatSliderEdit();
         this.forceSliderEdit = new VisualEditor.Controls.FloatSliderEdit();
         this.label1 = new System.Windows.Forms.Label();
         this.label3 = new System.Windows.Forms.Label();
         this.positionSliderEdit = new VisualEditor.Controls.FloatSliderEdit();
         this.label2 = new System.Windows.Forms.Label();
         this.selectedCheckBox = new System.Windows.Forms.CheckBox();
         this.groupBox1.SuspendLayout();
         this.SuspendLayout();
         // 
         // groupBox1
         // 
         this.groupBox1.Controls.Add(this.selectedCheckBox);
         this.groupBox1.Controls.Add(this.toBoneComboBox);
         this.groupBox1.Controls.Add(this.toBoneLabel);
         this.groupBox1.Controls.Add(this.RefreshButton);
         this.groupBox1.Controls.Add(this.durationSliderEdit);
         this.groupBox1.Controls.Add(this.forceSliderEdit);
         this.groupBox1.Controls.Add(this.label1);
         this.groupBox1.Controls.Add(this.label3);
         this.groupBox1.Controls.Add(this.positionSliderEdit);
         this.groupBox1.Controls.Add(this.label2);
         this.groupBox1.Location = new System.Drawing.Point(0, 0);
         this.groupBox1.Name = "groupBox1";
         this.groupBox1.Padding = new System.Windows.Forms.Padding(5, 16, 5, 3);
         this.groupBox1.Size = new System.Drawing.Size(350, 250);
         this.groupBox1.TabIndex = 0;
         this.groupBox1.TabStop = false;
         this.groupBox1.Text = "Camera Shake Tag Properties";
         // 
         // toBoneComboBox
         // 
         this.toBoneComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
         this.toBoneComboBox.FormattingEnabled = true;
         this.toBoneComboBox.Location = new System.Drawing.Point(102, 161);
         this.toBoneComboBox.Name = "toBoneComboBox";
         this.toBoneComboBox.Size = new System.Drawing.Size(240, 21);
         this.toBoneComboBox.TabIndex = 31;
         this.toBoneComboBox.SelectedIndexChanged += new System.EventHandler(this.toBoneComboBox_SelectedIndexChanged);
         // 
         // toBoneLabel
         // 
         this.toBoneLabel.AutoSize = true;
         this.toBoneLabel.Location = new System.Drawing.Point(8, 163);
         this.toBoneLabel.Margin = new System.Windows.Forms.Padding(3);
         this.toBoneLabel.Name = "toBoneLabel";
         this.toBoneLabel.Size = new System.Drawing.Size(48, 13);
         this.toBoneLabel.TabIndex = 30;
         this.toBoneLabel.Text = "To Bone";
         // 
         // RefreshButton
         // 
         this.RefreshButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
         this.RefreshButton.Image = ((System.Drawing.Image)(resources.GetObject("RefreshButton.Image")));
         this.RefreshButton.Location = new System.Drawing.Point(318, 34);
         this.RefreshButton.Margin = new System.Windows.Forms.Padding(0);
         this.RefreshButton.Name = "RefreshButton";
         this.RefreshButton.Size = new System.Drawing.Size(24, 24);
         this.RefreshButton.TabIndex = 2;
         this.RefreshButton.UseVisualStyleBackColor = true;
         this.RefreshButton.Click += new System.EventHandler(this.RefreshButton_Click);
         // 
         // durationSliderEdit
         // 
         this.durationSliderEdit.Location = new System.Drawing.Point(102, 118);
         this.durationSliderEdit.Margin = new System.Windows.Forms.Padding(3, 3, 3, 0);
         this.durationSliderEdit.MaxValue = 3F;
         this.durationSliderEdit.MinValue = 0F;
         this.durationSliderEdit.Name = "durationSliderEdit";
         this.durationSliderEdit.NumDecimals = 3;
         this.durationSliderEdit.Size = new System.Drawing.Size(240, 40);
         this.durationSliderEdit.TabIndex = 6;
         this.durationSliderEdit.Value = 0F;
         this.durationSliderEdit.ValueChanged += new VisualEditor.Controls.FloatSliderEdit.ValueChangedDelegate(this.durationSliderEdit_ValueChanged);
         // 
         // forceSliderEdit
         // 
         this.forceSliderEdit.Location = new System.Drawing.Point(102, 75);
         this.forceSliderEdit.Margin = new System.Windows.Forms.Padding(3, 3, 3, 0);
         this.forceSliderEdit.MaxValue = 5F;
         this.forceSliderEdit.MinValue = 0.1F;
         this.forceSliderEdit.Name = "forceSliderEdit";
         this.forceSliderEdit.NumDecimals = 3;
         this.forceSliderEdit.Size = new System.Drawing.Size(240, 40);
         this.forceSliderEdit.TabIndex = 4;
         this.forceSliderEdit.Value = 1F;
         this.forceSliderEdit.ValueChanged += new VisualEditor.Controls.FloatSliderEdit.ValueChangedDelegate(this.forceSliderEdit_ValueChanged);
         // 
         // label1
         // 
         this.label1.AutoSize = true;
         this.label1.Location = new System.Drawing.Point(8, 118);
         this.label1.Margin = new System.Windows.Forms.Padding(3);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(47, 13);
         this.label1.TabIndex = 5;
         this.label1.Text = "Duration";
         // 
         // label3
         // 
         this.label3.AutoSize = true;
         this.label3.Location = new System.Drawing.Point(8, 75);
         this.label3.Margin = new System.Windows.Forms.Padding(3);
         this.label3.Name = "label3";
         this.label3.Size = new System.Drawing.Size(34, 13);
         this.label3.TabIndex = 3;
         this.label3.Text = "Force";
         // 
         // positionSliderEdit
         // 
         this.positionSliderEdit.Location = new System.Drawing.Point(102, 32);
         this.positionSliderEdit.Margin = new System.Windows.Forms.Padding(3, 3, 3, 0);
         this.positionSliderEdit.MaxValue = 1F;
         this.positionSliderEdit.MinValue = 0F;
         this.positionSliderEdit.Name = "positionSliderEdit";
         this.positionSliderEdit.NumDecimals = 3;
         this.positionSliderEdit.Size = new System.Drawing.Size(213, 40);
         this.positionSliderEdit.TabIndex = 1;
         this.positionSliderEdit.Value = 0F;
         this.positionSliderEdit.ValueChanged += new VisualEditor.Controls.FloatSliderEdit.ValueChangedDelegate(this.positionSliderEdit_ValueChanged);
         // 
         // label2
         // 
         this.label2.AutoSize = true;
         this.label2.Location = new System.Drawing.Point(8, 34);
         this.label2.Name = "label2";
         this.label2.Size = new System.Drawing.Size(44, 13);
         this.label2.TabIndex = 0;
         this.label2.Text = "Position";
         // 
         // selectedCheckBox
         // 
         this.selectedCheckBox.AutoSize = true;
         this.selectedCheckBox.Location = new System.Drawing.Point(11, 198);
         this.selectedCheckBox.Name = "selectedCheckBox";
         this.selectedCheckBox.Size = new System.Drawing.Size(102, 17);
         this.selectedCheckBox.TabIndex = 38;
         this.selectedCheckBox.Text = "Check Selected";
         this.selectedCheckBox.UseVisualStyleBackColor = true;
         this.selectedCheckBox.CheckedChanged += new System.EventHandler(this.selectedCheckBox_CheckedChanged);
         // 
         // AnimTagCameraShakePage
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.groupBox1);
         this.Name = "AnimTagCameraShakePage";
         this.Size = new System.Drawing.Size(350, 250);
         this.groupBox1.ResumeLayout(false);
         this.groupBox1.PerformLayout();
         this.ResumeLayout(false);

      }

      #endregion

      private System.Windows.Forms.GroupBox groupBox1;
      private VisualEditor.Controls.FloatSliderEdit positionSliderEdit;
      private System.Windows.Forms.Label label2;
      private VisualEditor.Controls.FloatSliderEdit durationSliderEdit;
      private VisualEditor.Controls.FloatSliderEdit forceSliderEdit;
      private System.Windows.Forms.Label label1;
      private System.Windows.Forms.Label label3;
      private System.Windows.Forms.Button RefreshButton;
      private System.Windows.Forms.ComboBox toBoneComboBox;
      private System.Windows.Forms.Label toBoneLabel;
      private System.Windows.Forms.CheckBox selectedCheckBox;
   }
}
