using EditorCore;

namespace VisualEditor.PropertyPages
{
   partial class AttachModelPage
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
         this.fromBoneComboBox = new System.Windows.Forms.ComboBox();
         this.toBoneComboBox = new System.Windows.Forms.ComboBox();
         this.fromBoneLabel = new System.Windows.Forms.Label();
         this.toBoneLabel = new System.Windows.Forms.Label();
         this.modelFileLabel = new System.Windows.Forms.Label();
         this.modelFileBrowseControl = new EditorCore.FileBrowseControl();
         this.disregardBoneOrientationCheckBox = new System.Windows.Forms.CheckBox();
         this.groupBox1.SuspendLayout();
         this.SuspendLayout();
         // 
         // groupBox1
         // 
         this.groupBox1.Anchor = System.Windows.Forms.AnchorStyles.None;
         this.groupBox1.Controls.Add(this.disregardBoneOrientationCheckBox);
         this.groupBox1.Controls.Add(this.fromBoneComboBox);
         this.groupBox1.Controls.Add(this.toBoneComboBox);
         this.groupBox1.Controls.Add(this.fromBoneLabel);
         this.groupBox1.Controls.Add(this.toBoneLabel);
         this.groupBox1.Controls.Add(this.modelFileBrowseControl);
         this.groupBox1.Controls.Add(this.modelFileLabel);
         this.groupBox1.Location = new System.Drawing.Point(0, 0);
         this.groupBox1.Name = "groupBox1";
         this.groupBox1.Padding = new System.Windows.Forms.Padding(5, 16, 5, 3);
         this.groupBox1.Size = new System.Drawing.Size(350, 250);
         this.groupBox1.TabIndex = 3;
         this.groupBox1.TabStop = false;
         this.groupBox1.Text = "Model Attachment Properties";
         // 
         // fromBoneComboBox
         // 
         this.fromBoneComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
         this.fromBoneComboBox.FormattingEnabled = true;
         this.fromBoneComboBox.Location = new System.Drawing.Point(133, 89);
         this.fromBoneComboBox.Name = "fromBoneComboBox";
         this.fromBoneComboBox.Size = new System.Drawing.Size(209, 21);
         this.fromBoneComboBox.TabIndex = 26;
         this.fromBoneComboBox.SelectedIndexChanged += new System.EventHandler(this.fromBoneComboBox_SelectedIndexChanged);
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
         // fromBoneLabel
         // 
         this.fromBoneLabel.AutoSize = true;
         this.fromBoneLabel.Location = new System.Drawing.Point(8, 92);
         this.fromBoneLabel.Margin = new System.Windows.Forms.Padding(3);
         this.fromBoneLabel.Name = "fromBoneLabel";
         this.fromBoneLabel.Size = new System.Drawing.Size(58, 13);
         this.fromBoneLabel.TabIndex = 18;
         this.fromBoneLabel.Text = "From Bone";
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
         this.modelFileLabel.Size = new System.Drawing.Size(55, 13);
         this.modelFileLabel.TabIndex = 10;
         this.modelFileLabel.Text = "Model File";
         // 
         // modelFileBrowseControl
         // 
         this.modelFileBrowseControl.FileName = "";
         this.modelFileBrowseControl.FilterExtension = ".gr2";
         this.modelFileBrowseControl.FilterName = "Granny Model Files";
         this.modelFileBrowseControl.Location = new System.Drawing.Point(71, 32);
         this.modelFileBrowseControl.Name = "modelFileBrowseControl";
         this.modelFileBrowseControl.ReferenceFolder = "art";
         this.modelFileBrowseControl.Size = new System.Drawing.Size(271, 24);
         this.modelFileBrowseControl.StartFolder = "art";
         this.modelFileBrowseControl.TabIndex = 11;
         this.modelFileBrowseControl.ValueChanged += new EditorCore.FileBrowseControl.ValueChangedDelegate(this.modelFileBrowseControl_ValueChanged);
         // 
         // disregardBoneOrientationCheckBox
         // 
         this.disregardBoneOrientationCheckBox.AutoSize = true;
         this.disregardBoneOrientationCheckBox.Location = new System.Drawing.Point(8, 125);
         this.disregardBoneOrientationCheckBox.Name = "disregardBoneOrientationCheckBox";
         this.disregardBoneOrientationCheckBox.Size = new System.Drawing.Size(153, 17);
         this.disregardBoneOrientationCheckBox.TabIndex = 27;
         this.disregardBoneOrientationCheckBox.Text = "Disregard Bone Orientation";
         this.disregardBoneOrientationCheckBox.UseVisualStyleBackColor = true;
         this.disregardBoneOrientationCheckBox.CheckedChanged += new System.EventHandler(this.disregardBoneOrientationCheckBox_CheckedChanged);
         // 
         // AttachModelPage
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.groupBox1);
         this.Name = "AttachModelPage";
         this.Size = new System.Drawing.Size(350, 250);
         this.groupBox1.ResumeLayout(false);
         this.groupBox1.PerformLayout();
         this.ResumeLayout(false);

      }

      #endregion

      private System.Windows.Forms.GroupBox groupBox1;
      private System.Windows.Forms.ComboBox fromBoneComboBox;
      private System.Windows.Forms.ComboBox toBoneComboBox;
      private System.Windows.Forms.Label fromBoneLabel;
      private System.Windows.Forms.Label toBoneLabel;
      private EditorCore.FileBrowseControl modelFileBrowseControl;
      private System.Windows.Forms.Label modelFileLabel;
      private System.Windows.Forms.CheckBox disregardBoneOrientationCheckBox;
   }
}
