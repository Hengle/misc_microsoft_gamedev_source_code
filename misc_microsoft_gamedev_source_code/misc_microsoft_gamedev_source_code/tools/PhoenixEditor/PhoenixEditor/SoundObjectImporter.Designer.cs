namespace PhoenixEditor
{
   partial class SoundObjectImporter
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
         this.ExistingSoundsObjectsListBox = new System.Windows.Forms.ListBox();
         this.label1 = new System.Windows.Forms.Label();
         this.SoundsToChoose = new System.Windows.Forms.ListBox();
         this.label2 = new System.Windows.Forms.Label();
         this.checkBox1 = new System.Windows.Forms.CheckBox();
         this.AddSelectedButton = new System.Windows.Forms.Button();
         this.SoundsToImport = new System.Windows.Forms.ListBox();
         this.label3 = new System.Windows.Forms.Label();
         this.RemoveSelectedButton = new System.Windows.Forms.Button();
         this.ImportButton = new System.Windows.Forms.Button();
         this.SoundBehindFOWCheckBox = new System.Windows.Forms.CheckBox();
         this.SuspendLayout();
         // 
         // ExistingSoundsObjectsListBox
         // 
         this.ExistingSoundsObjectsListBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)));
         this.ExistingSoundsObjectsListBox.FormattingEnabled = true;
         this.ExistingSoundsObjectsListBox.Location = new System.Drawing.Point(8, 39);
         this.ExistingSoundsObjectsListBox.Name = "ExistingSoundsObjectsListBox";
         this.ExistingSoundsObjectsListBox.Size = new System.Drawing.Size(243, 342);
         this.ExistingSoundsObjectsListBox.Sorted = true;
         this.ExistingSoundsObjectsListBox.TabIndex = 0;
         // 
         // label1
         // 
         this.label1.AutoSize = true;
         this.label1.Location = new System.Drawing.Point(5, 13);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(161, 13);
         this.label1.TabIndex = 1;
         this.label1.Text = "Existing Sound Objects (just info)";
         // 
         // SoundsToChoose
         // 
         this.SoundsToChoose.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)));
         this.SoundsToChoose.FormattingEnabled = true;
         this.SoundsToChoose.Location = new System.Drawing.Point(272, 41);
         this.SoundsToChoose.Name = "SoundsToChoose";
         this.SoundsToChoose.Size = new System.Drawing.Size(283, 342);
         this.SoundsToChoose.Sorted = true;
         this.SoundsToChoose.TabIndex = 2;
         // 
         // label2
         // 
         this.label2.AutoSize = true;
         this.label2.Location = new System.Drawing.Point(269, 13);
         this.label2.Name = "label2";
         this.label2.Size = new System.Drawing.Size(217, 13);
         this.label2.TabIndex = 3;
         this.label2.Text = "Sounds (Must have matching play and stop )";
         // 
         // checkBox1
         // 
         this.checkBox1.AutoSize = true;
         this.checkBox1.Checked = true;
         this.checkBox1.CheckState = System.Windows.Forms.CheckState.Checked;
         this.checkBox1.Location = new System.Drawing.Point(234, 396);
         this.checkBox1.Name = "checkBox1";
         this.checkBox1.Size = new System.Drawing.Size(165, 17);
         this.checkBox1.TabIndex = 4;
         this.checkBox1.Text = "Hide sounds already imported";
         this.checkBox1.UseVisualStyleBackColor = true;
         this.checkBox1.Visible = false;
         // 
         // AddSelectedButton
         // 
         this.AddSelectedButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
         this.AddSelectedButton.Location = new System.Drawing.Point(422, 396);
         this.AddSelectedButton.Name = "AddSelectedButton";
         this.AddSelectedButton.Size = new System.Drawing.Size(133, 23);
         this.AddSelectedButton.TabIndex = 5;
         this.AddSelectedButton.Text = "Add Selected ->";
         this.AddSelectedButton.UseVisualStyleBackColor = true;
         this.AddSelectedButton.Click += new System.EventHandler(this.AddSelectedButton_Click);
         // 
         // SoundsToImport
         // 
         this.SoundsToImport.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)));
         this.SoundsToImport.FormattingEnabled = true;
         this.SoundsToImport.Location = new System.Drawing.Point(582, 41);
         this.SoundsToImport.Name = "SoundsToImport";
         this.SoundsToImport.Size = new System.Drawing.Size(278, 342);
         this.SoundsToImport.Sorted = true;
         this.SoundsToImport.TabIndex = 6;
         // 
         // label3
         // 
         this.label3.AutoSize = true;
         this.label3.Location = new System.Drawing.Point(579, 14);
         this.label3.Name = "label3";
         this.label3.Size = new System.Drawing.Size(91, 13);
         this.label3.TabIndex = 7;
         this.label3.Text = "Sounds To Import";
         // 
         // RemoveSelectedButton
         // 
         this.RemoveSelectedButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
         this.RemoveSelectedButton.Location = new System.Drawing.Point(658, 396);
         this.RemoveSelectedButton.Name = "RemoveSelectedButton";
         this.RemoveSelectedButton.Size = new System.Drawing.Size(115, 23);
         this.RemoveSelectedButton.TabIndex = 8;
         this.RemoveSelectedButton.Text = "Remove Selected";
         this.RemoveSelectedButton.UseVisualStyleBackColor = true;
         this.RemoveSelectedButton.Click += new System.EventHandler(this.RemoveSelectedButton_Click);
         // 
         // ImportButton
         // 
         this.ImportButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
         this.ImportButton.Location = new System.Drawing.Point(866, 203);
         this.ImportButton.Name = "ImportButton";
         this.ImportButton.Size = new System.Drawing.Size(92, 53);
         this.ImportButton.TabIndex = 9;
         this.ImportButton.Text = "Import!";
         this.ImportButton.UseVisualStyleBackColor = true;
         this.ImportButton.Click += new System.EventHandler(this.ImportButton_Click);
         // 
         // SoundBehindFOWCheckBox
         // 
         this.SoundBehindFOWCheckBox.AutoSize = true;
         this.SoundBehindFOWCheckBox.Location = new System.Drawing.Point(867, 180);
         this.SoundBehindFOWCheckBox.Name = "SoundBehindFOWCheckBox";
         this.SoundBehindFOWCheckBox.Size = new System.Drawing.Size(115, 17);
         this.SoundBehindFOWCheckBox.TabIndex = 10;
         this.SoundBehindFOWCheckBox.Text = "SoundBehindFOW";
         this.SoundBehindFOWCheckBox.UseVisualStyleBackColor = true;
         // 
         // SoundObjectImporter
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.ClientSize = new System.Drawing.Size(989, 431);
         this.Controls.Add(this.SoundBehindFOWCheckBox);
         this.Controls.Add(this.ImportButton);
         this.Controls.Add(this.RemoveSelectedButton);
         this.Controls.Add(this.label3);
         this.Controls.Add(this.SoundsToImport);
         this.Controls.Add(this.AddSelectedButton);
         this.Controls.Add(this.checkBox1);
         this.Controls.Add(this.label2);
         this.Controls.Add(this.SoundsToChoose);
         this.Controls.Add(this.label1);
         this.Controls.Add(this.ExistingSoundsObjectsListBox);
         this.Name = "SoundObjectImporter";
         this.Text = "SoundObjectImporter";
         this.Load += new System.EventHandler(this.SoundObjectImporter_Load);
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private System.Windows.Forms.ListBox ExistingSoundsObjectsListBox;
      private System.Windows.Forms.Label label1;
      private System.Windows.Forms.ListBox SoundsToChoose;
      private System.Windows.Forms.Label label2;
      private System.Windows.Forms.CheckBox checkBox1;
      private System.Windows.Forms.Button AddSelectedButton;
      private System.Windows.Forms.ListBox SoundsToImport;
      private System.Windows.Forms.Label label3;
      private System.Windows.Forms.Button RemoveSelectedButton;
      private System.Windows.Forms.Button ImportButton;
      private System.Windows.Forms.CheckBox SoundBehindFOWCheckBox;
   }
}