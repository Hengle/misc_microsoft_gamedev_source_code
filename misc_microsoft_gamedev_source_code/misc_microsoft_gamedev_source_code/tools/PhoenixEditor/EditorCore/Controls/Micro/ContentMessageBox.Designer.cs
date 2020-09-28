namespace EditorCore.Controls.Micro
{
   partial class ContentMessageBox
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
         this.panel1 = new System.Windows.Forms.Panel();
         this.MessageLabel = new System.Windows.Forms.Label();
         this.CancelButton = new System.Windows.Forms.Button();
         this.OKButton = new System.Windows.Forms.Button();
         this.SuspendLayout();
         // 
         // panel1
         // 
         this.panel1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.panel1.Location = new System.Drawing.Point(12, 31);
         this.panel1.Name = "panel1";
         this.panel1.Size = new System.Drawing.Size(376, 83);
         this.panel1.TabIndex = 0;
         // 
         // MessageLabel
         // 
         this.MessageLabel.AutoSize = true;
         this.MessageLabel.Location = new System.Drawing.Point(13, 12);
         this.MessageLabel.Name = "MessageLabel";
         this.MessageLabel.Size = new System.Drawing.Size(35, 13);
         this.MessageLabel.TabIndex = 1;
         this.MessageLabel.Text = "label1";
         // 
         // CancelButton
         // 
         this.CancelButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
         this.CancelButton.Location = new System.Drawing.Point(313, 131);
         this.CancelButton.Name = "CancelButton";
         this.CancelButton.Size = new System.Drawing.Size(75, 23);
         this.CancelButton.TabIndex = 4;
         this.CancelButton.Text = "Cancel";
         this.CancelButton.UseVisualStyleBackColor = true;
         this.CancelButton.Click += new System.EventHandler(this.CancelButton_Click);
         // 
         // OKButton
         // 
         this.OKButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
         this.OKButton.Location = new System.Drawing.Point(232, 131);
         this.OKButton.Name = "OKButton";
         this.OKButton.Size = new System.Drawing.Size(75, 23);
         this.OKButton.TabIndex = 3;
         this.OKButton.Text = "OK";
         this.OKButton.UseVisualStyleBackColor = true;
         this.OKButton.Click += new System.EventHandler(this.OKButton_Click);
         // 
         // ContentMessageBox
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.ClientSize = new System.Drawing.Size(400, 166);
         this.Controls.Add(this.CancelButton);
         this.Controls.Add(this.OKButton);
         this.Controls.Add(this.MessageLabel);
         this.Controls.Add(this.panel1);
         this.Name = "ContentMessageBox";
         this.Text = "ContentMessageBox";
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private System.Windows.Forms.Panel panel1;
      private System.Windows.Forms.Label MessageLabel;
      private System.Windows.Forms.Button CancelButton;
      private System.Windows.Forms.Button OKButton;
   }
}