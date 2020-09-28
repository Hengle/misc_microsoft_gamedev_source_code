using EditorCore;

namespace VisualEditor.PropertyPages
{
   partial class AnimTagLightPage
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
         System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(AnimTagLightPage));
         this.groupBox1 = new System.Windows.Forms.GroupBox();
         this.lifespanFloatSliderEdit = new VisualEditor.Controls.FloatSliderEdit();
         this.label3 = new System.Windows.Forms.Label();
         this.disregardBoneOrientationCheckBox = new System.Windows.Forms.CheckBox();
         this.RefreshButton = new System.Windows.Forms.Button();
         this.toBoneComboBox = new System.Windows.Forms.ComboBox();
         this.toBoneLabel = new System.Windows.Forms.Label();
         this.fileBrowseControl1 = new EditorCore.FileBrowseControl();
         this.label1 = new System.Windows.Forms.Label();
         this.floatSliderEdit1 = new VisualEditor.Controls.FloatSliderEdit();
         this.label2 = new System.Windows.Forms.Label();
         this.groupBox1.SuspendLayout();
         this.SuspendLayout();
         // 
         // groupBox1
         // 
         this.groupBox1.Controls.Add(this.lifespanFloatSliderEdit);
         this.groupBox1.Controls.Add(this.label3);
         this.groupBox1.Controls.Add(this.disregardBoneOrientationCheckBox);
         this.groupBox1.Controls.Add(this.RefreshButton);
         this.groupBox1.Controls.Add(this.toBoneComboBox);
         this.groupBox1.Controls.Add(this.toBoneLabel);
         this.groupBox1.Controls.Add(this.fileBrowseControl1);
         this.groupBox1.Controls.Add(this.label1);
         this.groupBox1.Controls.Add(this.floatSliderEdit1);
         this.groupBox1.Controls.Add(this.label2);
         this.groupBox1.Location = new System.Drawing.Point(0, 0);
         this.groupBox1.Name = "groupBox1";
         this.groupBox1.Padding = new System.Windows.Forms.Padding(5, 16, 5, 3);
         this.groupBox1.Size = new System.Drawing.Size(350, 250);
         this.groupBox1.TabIndex = 2;
         this.groupBox1.TabStop = false;
         this.groupBox1.Text = "Light Tag Properties";
         // 
         // lifespanFloatSliderEdit
         // 
         this.lifespanFloatSliderEdit.Location = new System.Drawing.Point(102, 164);
         this.lifespanFloatSliderEdit.Margin = new System.Windows.Forms.Padding(3, 3, 3, 0);
         this.lifespanFloatSliderEdit.MaxValue = 60F;
         this.lifespanFloatSliderEdit.MinValue = 0F;
         this.lifespanFloatSliderEdit.Name = "lifespanFloatSliderEdit";
         this.lifespanFloatSliderEdit.NumDecimals = 2;
         this.lifespanFloatSliderEdit.Size = new System.Drawing.Size(240, 40);
         this.lifespanFloatSliderEdit.TabIndex = 33;
         this.lifespanFloatSliderEdit.Value = 0.25F;
         this.lifespanFloatSliderEdit.ValueChanged += new VisualEditor.Controls.FloatSliderEdit.ValueChangedDelegate(this.lifespanFloatSliderEdit_ValueChanged);
         // 
         // label3
         // 
         this.label3.AutoSize = true;
         this.label3.Location = new System.Drawing.Point(8, 166);
         this.label3.Name = "label3";
         this.label3.Size = new System.Drawing.Size(52, 13);
         this.label3.TabIndex = 32;
         this.label3.Text = "Life Span";
         // 
         // disregardBoneOrientationCheckBox
         // 
         this.disregardBoneOrientationCheckBox.AutoSize = true;
         this.disregardBoneOrientationCheckBox.Location = new System.Drawing.Point(11, 132);
         this.disregardBoneOrientationCheckBox.Name = "disregardBoneOrientationCheckBox";
         this.disregardBoneOrientationCheckBox.Size = new System.Drawing.Size(153, 17);
         this.disregardBoneOrientationCheckBox.TabIndex = 29;
         this.disregardBoneOrientationCheckBox.Text = "Disregard Bone Orientation";
         this.disregardBoneOrientationCheckBox.UseVisualStyleBackColor = true;
         this.disregardBoneOrientationCheckBox.CheckedChanged += new System.EventHandler(this.disregardBoneOrientationCheckBox_CheckedChanged);
         // 
         // RefreshButton
         // 
         this.RefreshButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
         this.RefreshButton.Image = ((System.Drawing.Image)(resources.GetObject("RefreshButton.Image")));
         this.RefreshButton.Location = new System.Drawing.Point(318, 34);
         this.RefreshButton.Margin = new System.Windows.Forms.Padding(0);
         this.RefreshButton.Name = "RefreshButton";
         this.RefreshButton.Size = new System.Drawing.Size(24, 24);
         this.RefreshButton.TabIndex = 28;
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
         this.toBoneComboBox.TabIndex = 27;
         this.toBoneComboBox.SelectedIndexChanged += new System.EventHandler(this.toBoneComboBox_SelectedIndexChanged);
         // 
         // toBoneLabel
         // 
         this.toBoneLabel.AutoSize = true;
         this.toBoneLabel.Location = new System.Drawing.Point(8, 108);
         this.toBoneLabel.Margin = new System.Windows.Forms.Padding(3);
         this.toBoneLabel.Name = "toBoneLabel";
         this.toBoneLabel.Size = new System.Drawing.Size(48, 13);
         this.toBoneLabel.TabIndex = 26;
         this.toBoneLabel.Text = "To Bone";
         // 
         // fileBrowseControl1
         // 
         this.fileBrowseControl1.FileName = "";
         this.fileBrowseControl1.FilterExtension = ".lgt";
         this.fileBrowseControl1.FilterName = "Light Effect Files";
         this.fileBrowseControl1.Location = new System.Drawing.Point(102, 75);
         this.fileBrowseControl1.Name = "fileBrowseControl1";
         this.fileBrowseControl1.ReferenceFolder = "art";
         this.fileBrowseControl1.Size = new System.Drawing.Size(240, 24);
         this.fileBrowseControl1.StartFolder = "art";
         this.fileBrowseControl1.TabIndex = 6;
         this.fileBrowseControl1.ValueChanged += new EditorCore.FileBrowseControl.ValueChangedDelegate(this.fileBrowseControl1_ValueChanged);
         // 
         // label1
         // 
         this.label1.AutoSize = true;
         this.label1.Location = new System.Drawing.Point(8, 75);
         this.label1.Margin = new System.Windows.Forms.Padding(3);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(61, 13);
         this.label1.TabIndex = 5;
         this.label1.Text = "Light Effect";
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
         // label2
         // 
         this.label2.AutoSize = true;
         this.label2.Location = new System.Drawing.Point(8, 34);
         this.label2.Name = "label2";
         this.label2.Size = new System.Drawing.Size(44, 13);
         this.label2.TabIndex = 3;
         this.label2.Text = "Position";
         // 
         // AnimTagLightPage
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.groupBox1);
         this.Name = "AnimTagLightPage";
         this.Size = new System.Drawing.Size(350, 250);
         this.groupBox1.ResumeLayout(false);
         this.groupBox1.PerformLayout();
         this.ResumeLayout(false);

      }

      #endregion

      private System.Windows.Forms.GroupBox groupBox1;
      private System.Windows.Forms.ComboBox toBoneComboBox;
      private System.Windows.Forms.Label toBoneLabel;
      private EditorCore.FileBrowseControl fileBrowseControl1;
      private System.Windows.Forms.Label label1;
      private VisualEditor.Controls.FloatSliderEdit floatSliderEdit1;
      private System.Windows.Forms.Label label2;
      private System.Windows.Forms.Button RefreshButton;
      private System.Windows.Forms.CheckBox disregardBoneOrientationCheckBox;
      private VisualEditor.Controls.FloatSliderEdit lifespanFloatSliderEdit;
      private System.Windows.Forms.Label label3;
   }
}
