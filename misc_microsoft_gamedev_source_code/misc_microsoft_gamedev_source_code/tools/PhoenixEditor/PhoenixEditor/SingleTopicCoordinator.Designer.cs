namespace PhoenixEditor
{
   partial class SingleTopicCoordinator
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
         this.workTopicControl1 = new PhoenixEditor.WorkTopicControl();
         this.OKButton = new System.Windows.Forms.Button();
         this.CancelButton = new System.Windows.Forms.Button();
         this.SuspendLayout();
         // 
         // workTopicControl1
         // 
         this.workTopicControl1.BackColor = System.Drawing.SystemColors.InactiveCaptionText;
         this.workTopicControl1.Location = new System.Drawing.Point(12, 12);
         this.workTopicControl1.Name = "workTopicControl1";
         this.workTopicControl1.Size = new System.Drawing.Size(786, 75);
         this.workTopicControl1.TabIndex = 0;
         this.workTopicControl1.Load += new System.EventHandler(this.workTopicControl1_Load);
         // 
         // OKButton
         // 
         this.OKButton.Location = new System.Drawing.Point(12, 103);
         this.OKButton.Name = "OKButton";
         this.OKButton.Size = new System.Drawing.Size(136, 23);
         this.OKButton.TabIndex = 1;
         this.OKButton.Text = "OK";
         this.OKButton.UseVisualStyleBackColor = true;
         this.OKButton.Click += new System.EventHandler(this.OKButton_Click);
         // 
         // CancelButton
         // 
         this.CancelButton.Location = new System.Drawing.Point(668, 103);
         this.CancelButton.Name = "CancelButton";
         this.CancelButton.Size = new System.Drawing.Size(130, 23);
         this.CancelButton.TabIndex = 2;
         this.CancelButton.Text = "Cancel";
         this.CancelButton.UseVisualStyleBackColor = true;
         this.CancelButton.Click += new System.EventHandler(this.CancelButton_Click);
         // 
         // SingleTopicCoordinator
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.CancelButton);
         this.Controls.Add(this.OKButton);
         this.Controls.Add(this.workTopicControl1);
         this.Name = "SingleTopicCoordinator";
         this.Size = new System.Drawing.Size(827, 149);
         this.ResumeLayout(false);

      }

      #endregion

      private WorkTopicControl workTopicControl1;
      private System.Windows.Forms.Button OKButton;
      private System.Windows.Forms.Button CancelButton;
   }
}
