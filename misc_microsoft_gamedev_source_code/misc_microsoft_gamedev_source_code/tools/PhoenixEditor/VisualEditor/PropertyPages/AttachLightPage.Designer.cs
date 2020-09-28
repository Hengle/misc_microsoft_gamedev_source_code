using EditorCore;

namespace VisualEditor.PropertyPages
{
   partial class AttachLightPage
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
         this.toBoneComboBox = new System.Windows.Forms.ComboBox();
         this.toBoneLabel = new System.Windows.Forms.Label();
         this.modelFileLabel = new System.Windows.Forms.Label();
         this.disregardBoneOrientationCheckBox = new System.Windows.Forms.CheckBox();
         this.lightFileBrowseControl = new EditorCore.FileBrowseControl();
         this.groupBox1.SuspendLayout();
         this.SuspendLayout();
         // 
         // groupBox1
         // 
         this.groupBox1.Anchor = System.Windows.Forms.AnchorStyles.None;
         this.groupBox1.Controls.Add(this.disregardBoneOrientationCheckBox);
         this.groupBox1.Controls.Add(this.toBoneComboBox);
         this.groupBox1.Controls.Add(this.toBoneLabel);
         this.groupBox1.Controls.Add(this.lightFileBrowseControl);
         this.groupBox1.Controls.Add(this.modelFileLabel);
         this.groupBox1.Location = new System.Drawing.Point(0, 0);
         this.groupBox1.Name = "groupBox1";
         this.groupBox1.Padding = new System.Windows.Forms.Padding(5, 16, 5, 3);
         this.groupBox1.Size = new System.Drawing.Size(350, 250);
         this.groupBox1.TabIndex = 3;
         this.groupBox1.TabStop = false;
         this.groupBox1.Text = "Light Attachment Properties";
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
         // modelFileLabel
         // 
         this.modelFileLabel.AutoSize = true;
         this.modelFileLabel.Location = new System.Drawing.Point(8, 37);
         this.modelFileLabel.Margin = new System.Windows.Forms.Padding(3);
         this.modelFileLabel.Name = "modelFileLabel";
         this.modelFileLabel.Size = new System.Drawing.Size(49, 13);
         this.modelFileLabel.TabIndex = 10;
         this.modelFileLabel.Text = "Light File";
         // 
         // disregardBoneOrientationCheckBox
         // 
         this.disregardBoneOrientationCheckBox.AutoSize = true;
         this.disregardBoneOrientationCheckBox.Location = new System.Drawing.Point(11, 89);
         this.disregardBoneOrientationCheckBox.Name = "disregardBoneOrientationCheckBox";
         this.disregardBoneOrientationCheckBox.Size = new System.Drawing.Size(153, 17);
         this.disregardBoneOrientationCheckBox.TabIndex = 26;
         this.disregardBoneOrientationCheckBox.Text = "Disregard Bone Orientation";
         this.disregardBoneOrientationCheckBox.UseVisualStyleBackColor = true;
         this.disregardBoneOrientationCheckBox.CheckedChanged += new System.EventHandler(this.disregardBoneOrientationCheckBox_CheckedChanged);
         // 
         // lightFileBrowseControl
         // 
         this.lightFileBrowseControl.FileName = "";
         this.lightFileBrowseControl.FilterExtension = ".lgt";
         this.lightFileBrowseControl.FilterName = "Light Effect Files";
         this.lightFileBrowseControl.Location = new System.Drawing.Point(71, 32);
         this.lightFileBrowseControl.Name = "lightFileBrowseControl";
         this.lightFileBrowseControl.ReferenceFolder = "art";
         this.lightFileBrowseControl.Size = new System.Drawing.Size(271, 24);
         this.lightFileBrowseControl.StartFolder = "art";
         this.lightFileBrowseControl.TabIndex = 11;
         this.lightFileBrowseControl.ValueChanged += new EditorCore.FileBrowseControl.ValueChangedDelegate(this.lightFileBrowseControl_ValueChanged);
         // 
         // AttachLightPage
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.groupBox1);
         this.Name = "AttachLightPage";
         this.Size = new System.Drawing.Size(350, 250);
         this.groupBox1.ResumeLayout(false);
         this.groupBox1.PerformLayout();
         this.ResumeLayout(false);

      }

      #endregion

      private System.Windows.Forms.GroupBox groupBox1;
      private System.Windows.Forms.ComboBox toBoneComboBox;
      private System.Windows.Forms.Label toBoneLabel;
      private EditorCore.FileBrowseControl lightFileBrowseControl;
      private System.Windows.Forms.Label modelFileLabel;
      private System.Windows.Forms.CheckBox disregardBoneOrientationCheckBox;
   }
}
