namespace VisualEditor.PropertyPages
{
   partial class AttachTerrainEffectPage
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
         this.groupBox1 = new System.Windows.Forms.GroupBox();
         this.disregardBoneOrientationCheckBox = new System.Windows.Forms.CheckBox();
         this.toBoneComboBox = new System.Windows.Forms.ComboBox();
         this.toBoneLabel = new System.Windows.Forms.Label();
         this.terrainEffectFileBrowseControl = new EditorCore.FileBrowseControl();
         this.particleFileLabel = new System.Windows.Forms.Label();
         this.groupBox1.SuspendLayout();
         this.SuspendLayout();
         // 
         // groupBox1
         // 
         this.groupBox1.Anchor = System.Windows.Forms.AnchorStyles.None;
         this.groupBox1.Controls.Add(this.disregardBoneOrientationCheckBox);
         this.groupBox1.Controls.Add(this.toBoneComboBox);
         this.groupBox1.Controls.Add(this.toBoneLabel);
         this.groupBox1.Controls.Add(this.terrainEffectFileBrowseControl);
         this.groupBox1.Controls.Add(this.particleFileLabel);
         this.groupBox1.Location = new System.Drawing.Point(0, 0);
         this.groupBox1.Name = "groupBox1";
         this.groupBox1.Padding = new System.Windows.Forms.Padding(5, 16, 5, 3);
         this.groupBox1.Size = new System.Drawing.Size(350, 250);
         this.groupBox1.TabIndex = 4;
         this.groupBox1.TabStop = false;
         this.groupBox1.Text = "TerrainEffect Attachment Properties";
         // 
         // disregardBoneOrientationCheckBox
         // 
         this.disregardBoneOrientationCheckBox.AutoSize = true;
         this.disregardBoneOrientationCheckBox.Location = new System.Drawing.Point(11, 89);
         this.disregardBoneOrientationCheckBox.Name = "disregardBoneOrientationCheckBox";
         this.disregardBoneOrientationCheckBox.Size = new System.Drawing.Size(153, 17);
         this.disregardBoneOrientationCheckBox.TabIndex = 28;
         this.disregardBoneOrientationCheckBox.Text = "Disregard Bone Orientation";
         this.disregardBoneOrientationCheckBox.UseVisualStyleBackColor = true;
         this.disregardBoneOrientationCheckBox.CheckedChanged += new System.EventHandler(this.disregardBoneOrientationCheckBox_CheckedChanged);
         // 
         // toBoneComboBox
         // 
         this.toBoneComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
         this.toBoneComboBox.FormattingEnabled = true;
         this.toBoneComboBox.Location = new System.Drawing.Point(133, 62);
         this.toBoneComboBox.Name = "toBoneComboBox";
         this.toBoneComboBox.Size = new System.Drawing.Size(209, 21);
         this.toBoneComboBox.TabIndex = 25;
         this.toBoneComboBox.SelectedIndexChanged += new System.EventHandler(this.toBoneComboBox_SelectedIndexChanged);
         // 
         // toBoneLabel
         // 
         this.toBoneLabel.AutoSize = true;
         this.toBoneLabel.Location = new System.Drawing.Point(8, 65);
         this.toBoneLabel.Margin = new System.Windows.Forms.Padding(3);
         this.toBoneLabel.Name = "toBoneLabel";
         this.toBoneLabel.Size = new System.Drawing.Size(48, 13);
         this.toBoneLabel.TabIndex = 16;
         this.toBoneLabel.Text = "To Bone";
         // 
         // terrainEffectFileBrowseControl
         // 
         this.terrainEffectFileBrowseControl.FileName = "";
         this.terrainEffectFileBrowseControl.FilterExtension = ".tfx";
         this.terrainEffectFileBrowseControl.FilterName = "TerrainEffect Files";
         this.terrainEffectFileBrowseControl.Location = new System.Drawing.Point(101, 32);
         this.terrainEffectFileBrowseControl.Name = "terrainEffectFileBrowseControl";
         this.terrainEffectFileBrowseControl.ReferenceFolder = "art";
         this.terrainEffectFileBrowseControl.Size = new System.Drawing.Size(241, 24);
         this.terrainEffectFileBrowseControl.StartFolder = "art\\effects\\terraineffects";
         this.terrainEffectFileBrowseControl.TabIndex = 13;
         this.terrainEffectFileBrowseControl.ValueChanged += new EditorCore.FileBrowseControl.ValueChangedDelegate(this.terrainEffectBrowseControl_ValueChanged);
         // 
         // particleFileLabel
         // 
         this.particleFileLabel.AutoSize = true;
         this.particleFileLabel.Location = new System.Drawing.Point(8, 38);
         this.particleFileLabel.Margin = new System.Windows.Forms.Padding(3);
         this.particleFileLabel.Name = "particleFileLabel";
         this.particleFileLabel.Size = new System.Drawing.Size(87, 13);
         this.particleFileLabel.TabIndex = 12;
         this.particleFileLabel.Text = "TerrainEffect File";
         // 
         // AttachTerrainEffectPage
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.groupBox1);
         this.Name = "AttachTerrainEffectPage";
         this.Size = new System.Drawing.Size(350, 250);
         this.groupBox1.ResumeLayout(false);
         this.groupBox1.PerformLayout();
         this.ResumeLayout(false);

      }

      #endregion

      private System.Windows.Forms.GroupBox groupBox1;
      private System.Windows.Forms.CheckBox disregardBoneOrientationCheckBox;
      private System.Windows.Forms.ComboBox toBoneComboBox;
      private System.Windows.Forms.Label toBoneLabel;
      private EditorCore.FileBrowseControl terrainEffectFileBrowseControl;
      private System.Windows.Forms.Label particleFileLabel;
   }
}
