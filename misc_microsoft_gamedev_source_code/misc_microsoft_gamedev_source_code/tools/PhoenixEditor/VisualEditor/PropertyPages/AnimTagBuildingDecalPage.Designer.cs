namespace VisualEditor.PropertyPages
{
   partial class AnimTagBuildingDecalPage
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
         System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(AnimTagBuildingDecalPage));
         this.label3 = new System.Windows.Forms.Label();
         this.RefreshButton = new System.Windows.Forms.Button();
         this.toBoneComboBox = new System.Windows.Forms.ComboBox();
         this.toBoneLabel = new System.Windows.Forms.Label();
         this.label2 = new System.Windows.Forms.Label();
         this.label7 = new System.Windows.Forms.Label();
         this.groupBox1 = new System.Windows.Forms.GroupBox();
         this.fileBrowseControl1 = new EditorCore.FileBrowseControl();
         this.label1 = new System.Windows.Forms.Label();
         this.sizeXFloatSliderEdit = new VisualEditor.Controls.FloatSliderEdit();
         this.sizeZFloatSliderEdit = new VisualEditor.Controls.FloatSliderEdit();
         this.floatSliderEdit1 = new VisualEditor.Controls.FloatSliderEdit();
         this.groupBox1.SuspendLayout();
         this.SuspendLayout();
         // 
         // label3
         // 
         this.label3.AutoSize = true;
         this.label3.Location = new System.Drawing.Point(179, 138);
         this.label3.Name = "label3";
         this.label3.Size = new System.Drawing.Size(37, 13);
         this.label3.TabIndex = 30;
         this.label3.Text = "Size Z";
         // 
         // RefreshButton
         // 
         this.RefreshButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
         this.RefreshButton.Image = ((System.Drawing.Image)(resources.GetObject("RefreshButton.Image")));
         this.RefreshButton.Location = new System.Drawing.Point(319, 34);
         this.RefreshButton.Margin = new System.Windows.Forms.Padding(0);
         this.RefreshButton.Name = "RefreshButton";
         this.RefreshButton.Size = new System.Drawing.Size(24, 24);
         this.RefreshButton.TabIndex = 28;
         this.RefreshButton.UseVisualStyleBackColor = true;
         // 
         // toBoneComboBox
         // 
         this.toBoneComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
         this.toBoneComboBox.FormattingEnabled = true;
         this.toBoneComboBox.Location = new System.Drawing.Point(103, 66);
         this.toBoneComboBox.Name = "toBoneComboBox";
         this.toBoneComboBox.Size = new System.Drawing.Size(240, 21);
         this.toBoneComboBox.TabIndex = 27;
         this.toBoneComboBox.SelectedIndexChanged += new System.EventHandler(this.toBoneComboBox_SelectedIndexChanged);
         // 
         // toBoneLabel
         // 
         this.toBoneLabel.AutoSize = true;
         this.toBoneLabel.Location = new System.Drawing.Point(9, 69);
         this.toBoneLabel.Margin = new System.Windows.Forms.Padding(3);
         this.toBoneLabel.Name = "toBoneLabel";
         this.toBoneLabel.Size = new System.Drawing.Size(48, 13);
         this.toBoneLabel.TabIndex = 26;
         this.toBoneLabel.Text = "To Bone";
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
         // label7
         // 
         this.label7.AutoSize = true;
         this.label7.Location = new System.Drawing.Point(10, 138);
         this.label7.Name = "label7";
         this.label7.Size = new System.Drawing.Size(37, 13);
         this.label7.TabIndex = 38;
         this.label7.Text = "Size X";
         // 
         // groupBox1
         // 
         this.groupBox1.Controls.Add(this.fileBrowseControl1);
         this.groupBox1.Controls.Add(this.label1);
         this.groupBox1.Controls.Add(this.sizeXFloatSliderEdit);
         this.groupBox1.Controls.Add(this.label7);
         this.groupBox1.Controls.Add(this.sizeZFloatSliderEdit);
         this.groupBox1.Controls.Add(this.label3);
         this.groupBox1.Controls.Add(this.RefreshButton);
         this.groupBox1.Controls.Add(this.toBoneComboBox);
         this.groupBox1.Controls.Add(this.toBoneLabel);
         this.groupBox1.Controls.Add(this.floatSliderEdit1);
         this.groupBox1.Controls.Add(this.label2);
         this.groupBox1.Location = new System.Drawing.Point(3, 3);
         this.groupBox1.Name = "groupBox1";
         this.groupBox1.Padding = new System.Windows.Forms.Padding(5, 16, 5, 3);
         this.groupBox1.Size = new System.Drawing.Size(351, 198);
         this.groupBox1.TabIndex = 3;
         this.groupBox1.TabStop = false;
         this.groupBox1.Text = "Building Decal Tag Properties";
         // 
         // fileBrowseControl1
         // 
         this.fileBrowseControl1.FileName = "";
         this.fileBrowseControl1.FilterExtension = ".ddx";
         this.fileBrowseControl1.FilterName = "Texture Files";
         this.fileBrowseControl1.Location = new System.Drawing.Point(103, 93);
         this.fileBrowseControl1.Name = "fileBrowseControl1";
         this.fileBrowseControl1.ReferenceFolder = "art";
         this.fileBrowseControl1.Size = new System.Drawing.Size(240, 24);
         this.fileBrowseControl1.StartFolder = "art\\decals";
         this.fileBrowseControl1.TabIndex = 41;
         this.fileBrowseControl1.ValueChanged += new EditorCore.FileBrowseControl.ValueChangedDelegate(this.fileBrowseControl1_ValueChanged);
         // 
         // label1
         // 
         this.label1.AutoSize = true;
         this.label1.Location = new System.Drawing.Point(9, 93);
         this.label1.Margin = new System.Windows.Forms.Padding(3);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(73, 13);
         this.label1.TabIndex = 40;
         this.label1.Text = "Alpha Texture";
         // 
         // sizeXFloatSliderEdit
         // 
         this.sizeXFloatSliderEdit.Location = new System.Drawing.Point(61, 138);
         this.sizeXFloatSliderEdit.Margin = new System.Windows.Forms.Padding(3, 3, 3, 0);
         this.sizeXFloatSliderEdit.MaxValue = 4F;
         this.sizeXFloatSliderEdit.MinValue = 0.1F;
         this.sizeXFloatSliderEdit.Name = "sizeXFloatSliderEdit";
         this.sizeXFloatSliderEdit.NumDecimals = 2;
         this.sizeXFloatSliderEdit.Size = new System.Drawing.Size(99, 40);
         this.sizeXFloatSliderEdit.TabIndex = 39;
         this.sizeXFloatSliderEdit.Value = 1F;
         this.sizeXFloatSliderEdit.ValueChanged += new VisualEditor.Controls.FloatSliderEdit.ValueChangedDelegate(this.sizeXFloatSliderEdit_ValueChanged);
         // 
         // sizeZFloatSliderEdit
         // 
         this.sizeZFloatSliderEdit.Location = new System.Drawing.Point(230, 138);
         this.sizeZFloatSliderEdit.Margin = new System.Windows.Forms.Padding(3, 3, 3, 0);
         this.sizeZFloatSliderEdit.MaxValue = 4F;
         this.sizeZFloatSliderEdit.MinValue = 0.1F;
         this.sizeZFloatSliderEdit.Name = "sizeZFloatSliderEdit";
         this.sizeZFloatSliderEdit.NumDecimals = 2;
         this.sizeZFloatSliderEdit.Size = new System.Drawing.Size(99, 40);
         this.sizeZFloatSliderEdit.TabIndex = 31;
         this.sizeZFloatSliderEdit.Value = 1F;
         this.sizeZFloatSliderEdit.ValueChanged += new VisualEditor.Controls.FloatSliderEdit.ValueChangedDelegate(this.sizeZFloatSliderEdit_ValueChanged);
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
         // AnimTagBuildingDecalPage
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.groupBox1);
         this.Name = "AnimTagBuildingDecalPage";
         this.Size = new System.Drawing.Size(363, 211);
         this.groupBox1.ResumeLayout(false);
         this.groupBox1.PerformLayout();
         this.ResumeLayout(false);

      }

      #endregion

      private VisualEditor.Controls.FloatSliderEdit sizeZFloatSliderEdit;
      private System.Windows.Forms.Label label3;
      private System.Windows.Forms.Button RefreshButton;
      private System.Windows.Forms.ComboBox toBoneComboBox;
      private System.Windows.Forms.Label toBoneLabel;
      private VisualEditor.Controls.FloatSliderEdit floatSliderEdit1;
      private System.Windows.Forms.Label label2;
      private VisualEditor.Controls.FloatSliderEdit sizeXFloatSliderEdit;
      private System.Windows.Forms.Label label7;
      private System.Windows.Forms.GroupBox groupBox1;
      private EditorCore.FileBrowseControl fileBrowseControl1;
      private System.Windows.Forms.Label label1;
   }
}
