namespace PhoenixEditor
{
   partial class ScenarioSourceControl2
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
         this.CheckInAllButton = new System.Windows.Forms.Button();
         this.RefreshButton = new System.Windows.Forms.Button();
         this.AllowOverwriteCheckBox = new System.Windows.Forms.CheckBox();
         this.label1 = new System.Windows.Forms.Label();
         this.SuspendLayout();
         // 
         // CheckInAllButton
         // 
         this.CheckInAllButton.Location = new System.Drawing.Point(194, 13);
         this.CheckInAllButton.Name = "CheckInAllButton";
         this.CheckInAllButton.Size = new System.Drawing.Size(165, 23);
         this.CheckInAllButton.TabIndex = 1;
         this.CheckInAllButton.Text = "Check In All";
         this.CheckInAllButton.UseVisualStyleBackColor = true;
         this.CheckInAllButton.Click += new System.EventHandler(this.CheckInAllButton_Click);
         // 
         // RefreshButton
         // 
         this.RefreshButton.Location = new System.Drawing.Point(15, 13);
         this.RefreshButton.Name = "RefreshButton";
         this.RefreshButton.Size = new System.Drawing.Size(173, 23);
         this.RefreshButton.TabIndex = 2;
         this.RefreshButton.Text = "Refresh Perforce";
         this.RefreshButton.UseVisualStyleBackColor = true;
         this.RefreshButton.Click += new System.EventHandler(this.button1_Click);
         // 
         // AllowOverwriteCheckBox
         // 
         this.AllowOverwriteCheckBox.AutoSize = true;
         this.AllowOverwriteCheckBox.Location = new System.Drawing.Point(383, 19);
         this.AllowOverwriteCheckBox.Name = "AllowOverwriteCheckBox";
         this.AllowOverwriteCheckBox.Size = new System.Drawing.Size(164, 17);
         this.AllowOverwriteCheckBox.TabIndex = 3;
         this.AllowOverwriteCheckBox.Text = "ADVANCED: Allow Overwrite";
         this.AllowOverwriteCheckBox.UseVisualStyleBackColor = true;
         this.AllowOverwriteCheckBox.CheckedChanged += new System.EventHandler(this.AllowOverwriteCheckBox_CheckedChanged);
         // 
         // label1
         // 
         this.label1.AutoSize = true;
         this.label1.ForeColor = System.Drawing.SystemColors.Desktop;
         this.label1.Location = new System.Drawing.Point(613, 23);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(211, 13);
         this.label1.TabIndex = 4;
         this.label1.Text = "Remember to save before checking in files!";
         // 
         // ScenarioSourceControl2
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.label1);
         this.Controls.Add(this.AllowOverwriteCheckBox);
         this.Controls.Add(this.RefreshButton);
         this.Controls.Add(this.CheckInAllButton);
         this.Name = "ScenarioSourceControl2";
         this.Size = new System.Drawing.Size(1055, 636);
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private System.Windows.Forms.Button CheckInAllButton;
      private System.Windows.Forms.Button RefreshButton;
      private System.Windows.Forms.CheckBox AllowOverwriteCheckBox;
      private System.Windows.Forms.Label label1;
   }
}
