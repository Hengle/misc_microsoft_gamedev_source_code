namespace PhoenixEditor
{
   partial class ScenarioWorkCoordinator
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
         this.SaveButton = new System.Windows.Forms.Button();
         this.CancelButton = new System.Windows.Forms.Button();
         this.DontAskAgainCheckBox = new System.Windows.Forms.CheckBox();
         this.SavingLabel = new System.Windows.Forms.Label();
         this.SoundWorkTopicControl = new PhoenixEditor.WorkTopicControl();
         this.ArtObjectsTopicControl = new PhoenixEditor.WorkTopicControl();
         this.MasksWorkTopicControl = new PhoenixEditor.WorkTopicControl();
         this.LightsWorkTopicControl = new PhoenixEditor.WorkTopicControl();
         this.SimWorkTopicControl = new PhoenixEditor.WorkTopicControl();
         this.TerrainWorkTopicControl = new PhoenixEditor.WorkTopicControl();
         this.SuspendLayout();
         // 
         // SaveButton
         // 
         this.SaveButton.Enabled = false;
         this.SaveButton.Location = new System.Drawing.Point(62, 588);
         this.SaveButton.Name = "SaveButton";
         this.SaveButton.Size = new System.Drawing.Size(80, 35);
         this.SaveButton.TabIndex = 4;
         this.SaveButton.Text = "Save";
         this.SaveButton.UseVisualStyleBackColor = true;
         this.SaveButton.Click += new System.EventHandler(this.SaveButton_Click);
         // 
         // CancelButton
         // 
         this.CancelButton.Location = new System.Drawing.Point(722, 588);
         this.CancelButton.Name = "CancelButton";
         this.CancelButton.Size = new System.Drawing.Size(75, 35);
         this.CancelButton.TabIndex = 5;
         this.CancelButton.Text = "Cancel";
         this.CancelButton.UseVisualStyleBackColor = true;
         this.CancelButton.Click += new System.EventHandler(this.CancelButton_Click);
         // 
         // DontAskAgainCheckBox
         // 
         this.DontAskAgainCheckBox.AutoSize = true;
         this.DontAskAgainCheckBox.Location = new System.Drawing.Point(148, 598);
         this.DontAskAgainCheckBox.Name = "DontAskAgainCheckBox";
         this.DontAskAgainCheckBox.Size = new System.Drawing.Size(165, 17);
         this.DontAskAgainCheckBox.TabIndex = 7;
         this.DontAskAgainCheckBox.Text = "Don\'t Ask Again (this session)";
         this.DontAskAgainCheckBox.UseVisualStyleBackColor = true;
         // 
         // SavingLabel
         // 
         this.SavingLabel.AutoSize = true;
         this.SavingLabel.Font = new System.Drawing.Font("Microsoft Sans Serif", 20.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
         this.SavingLabel.Location = new System.Drawing.Point(319, 14);
         this.SavingLabel.Name = "SavingLabel";
         this.SavingLabel.Size = new System.Drawing.Size(178, 31);
         this.SavingLabel.TabIndex = 8;
         this.SavingLabel.Text = "Saving BLah";
         // 
         // SoundWorkTopicControl
         // 
         this.SoundWorkTopicControl.BackColor = System.Drawing.SystemColors.InactiveCaptionText;
         this.SoundWorkTopicControl.Location = new System.Drawing.Point(20, 397);
         this.SoundWorkTopicControl.Name = "SoundWorkTopicControl";
         this.SoundWorkTopicControl.Size = new System.Drawing.Size(814, 73);
         this.SoundWorkTopicControl.TabIndex = 10;
         // 
         // ArtObjectsTopicControl
         // 
         this.ArtObjectsTopicControl.BackColor = System.Drawing.SystemColors.InactiveCaptionText;
         this.ArtObjectsTopicControl.Location = new System.Drawing.Point(20, 313);
         this.ArtObjectsTopicControl.Name = "ArtObjectsTopicControl";
         this.ArtObjectsTopicControl.Size = new System.Drawing.Size(814, 73);
         this.ArtObjectsTopicControl.TabIndex = 9;
         // 
         // MasksWorkTopicControl
         // 
         this.MasksWorkTopicControl.BackColor = System.Drawing.SystemColors.InactiveCaptionText;
         this.MasksWorkTopicControl.Location = new System.Drawing.Point(20, 481);
         this.MasksWorkTopicControl.Name = "MasksWorkTopicControl";
         this.MasksWorkTopicControl.Size = new System.Drawing.Size(814, 73);
         this.MasksWorkTopicControl.TabIndex = 3;
         // 
         // LightsWorkTopicControl
         // 
         this.LightsWorkTopicControl.BackColor = System.Drawing.SystemColors.InactiveCaptionText;
         this.LightsWorkTopicControl.Location = new System.Drawing.Point(20, 145);
         this.LightsWorkTopicControl.Name = "LightsWorkTopicControl";
         this.LightsWorkTopicControl.Size = new System.Drawing.Size(814, 73);
         this.LightsWorkTopicControl.TabIndex = 2;
         // 
         // SimWorkTopicControl
         // 
         this.SimWorkTopicControl.BackColor = System.Drawing.SystemColors.InactiveCaptionText;
         this.SimWorkTopicControl.Location = new System.Drawing.Point(20, 229);
         this.SimWorkTopicControl.Name = "SimWorkTopicControl";
         this.SimWorkTopicControl.Size = new System.Drawing.Size(814, 73);
         this.SimWorkTopicControl.TabIndex = 1;
         // 
         // TerrainWorkTopicControl
         // 
         this.TerrainWorkTopicControl.BackColor = System.Drawing.SystemColors.InactiveCaptionText;
         this.TerrainWorkTopicControl.Location = new System.Drawing.Point(20, 61);
         this.TerrainWorkTopicControl.Name = "TerrainWorkTopicControl";
         this.TerrainWorkTopicControl.Size = new System.Drawing.Size(814, 73);
         this.TerrainWorkTopicControl.TabIndex = 0;
         // 
         // ScenarioWorkCoordinator
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.SoundWorkTopicControl);
         this.Controls.Add(this.ArtObjectsTopicControl);
         this.Controls.Add(this.SavingLabel);
         this.Controls.Add(this.DontAskAgainCheckBox);
         this.Controls.Add(this.CancelButton);
         this.Controls.Add(this.SaveButton);
         this.Controls.Add(this.MasksWorkTopicControl);
         this.Controls.Add(this.LightsWorkTopicControl);
         this.Controls.Add(this.SimWorkTopicControl);
         this.Controls.Add(this.TerrainWorkTopicControl);
         this.Name = "ScenarioWorkCoordinator";
         this.Size = new System.Drawing.Size(907, 637);
         this.Load += new System.EventHandler(this.ScenarioWorkCoordinator_Load);
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private WorkTopicControl TerrainWorkTopicControl;
      private WorkTopicControl SimWorkTopicControl;
      private WorkTopicControl LightsWorkTopicControl;
      private WorkTopicControl MasksWorkTopicControl;
      private System.Windows.Forms.Button SaveButton;
      private System.Windows.Forms.Button CancelButton;
      private System.Windows.Forms.CheckBox DontAskAgainCheckBox;
      private System.Windows.Forms.Label SavingLabel;
      private WorkTopicControl ArtObjectsTopicControl;
      private WorkTopicControl SoundWorkTopicControl;

   }
}
