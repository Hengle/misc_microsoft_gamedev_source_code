namespace PhoenixEditor
{
   partial class ExportWorkCoordinator
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
         this.NextButton = new System.Windows.Forms.Button();
         this.CancelButton = new System.Windows.Forms.Button();
         this.ExportFilesWorkTopicControl = new PhoenixEditor.WorkTopicControl();
         this.SuspendLayout();
         // 
         // NextButton
         // 
         this.NextButton.Location = new System.Drawing.Point(70, 146);
         this.NextButton.Name = "NextButton";
         this.NextButton.Size = new System.Drawing.Size(101, 23);
         this.NextButton.TabIndex = 0;
         this.NextButton.Text = "Start Export";
         this.NextButton.UseVisualStyleBackColor = true;
         this.NextButton.Click += new System.EventHandler(this.NextButton_Click);
         // 
         // CancelButton
         // 
         this.CancelButton.Location = new System.Drawing.Point(657, 146);
         this.CancelButton.Name = "CancelButton";
         this.CancelButton.Size = new System.Drawing.Size(75, 23);
         this.CancelButton.TabIndex = 1;
         this.CancelButton.Text = "Cancel";
         this.CancelButton.UseVisualStyleBackColor = true;
         this.CancelButton.Click += new System.EventHandler(this.CancelButton_Click);
         // 
         // ExportFilesWorkTopicControl
         // 
         this.ExportFilesWorkTopicControl.BackColor = System.Drawing.SystemColors.InactiveCaptionText;
         this.ExportFilesWorkTopicControl.Location = new System.Drawing.Point(12, 17);
         this.ExportFilesWorkTopicControl.Name = "ExportFilesWorkTopicControl";
         this.ExportFilesWorkTopicControl.Size = new System.Drawing.Size(813, 75);
         this.ExportFilesWorkTopicControl.TabIndex = 2;
         // 
         // ExportWorkCoordinator
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.ExportFilesWorkTopicControl);
         this.Controls.Add(this.CancelButton);
         this.Controls.Add(this.NextButton);
         this.Name = "ExportWorkCoordinator";
         this.Size = new System.Drawing.Size(842, 186);
         this.ResumeLayout(false);

      }

      #endregion

      private System.Windows.Forms.Button NextButton;
      private System.Windows.Forms.Button CancelButton;
      private WorkTopicControl ExportFilesWorkTopicControl;
   }
}
