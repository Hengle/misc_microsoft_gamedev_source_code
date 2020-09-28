namespace EditorCore.Controls.Micro
{
   partial class CategoryTreeEditor
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
         this.textBox1 = new System.Windows.Forms.TextBox();
         this.OkButton = new System.Windows.Forms.Button();
         this.CancelButton = new System.Windows.Forms.Button();
         this.collectionChooser1 = new EditorCore.Controls.Micro.CollectionChooser();
         this.SuspendLayout();
         // 
         // textBox1
         // 
         this.textBox1.Location = new System.Drawing.Point(3, 19);
         this.textBox1.Name = "textBox1";
         this.textBox1.Size = new System.Drawing.Size(238, 20);
         this.textBox1.TabIndex = 0;
         // 
         // OkButton
         // 
         this.OkButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
         this.OkButton.Location = new System.Drawing.Point(401, 338);
         this.OkButton.Name = "OkButton";
         this.OkButton.Size = new System.Drawing.Size(75, 23);
         this.OkButton.TabIndex = 2;
         this.OkButton.Text = "OK";
         this.OkButton.UseVisualStyleBackColor = true;
         this.OkButton.Click += new System.EventHandler(this.OkButton_Click);
         // 
         // CancelButton
         // 
         this.CancelButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
         this.CancelButton.Location = new System.Drawing.Point(482, 338);
         this.CancelButton.Name = "CancelButton";
         this.CancelButton.Size = new System.Drawing.Size(75, 23);
         this.CancelButton.TabIndex = 3;
         this.CancelButton.Text = "Cancel";
         this.CancelButton.UseVisualStyleBackColor = true;
         this.CancelButton.Click += new System.EventHandler(this.CancelButton_Click);
         // 
         // collectionChooser1
         // 
         this.collectionChooser1.AllowRepeats = true;
         this.collectionChooser1.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)));
         this.collectionChooser1.Location = new System.Drawing.Point(3, 60);
         this.collectionChooser1.Name = "collectionChooser1";
         this.collectionChooser1.Size = new System.Drawing.Size(378, 266);
         this.collectionChooser1.TabIndex = 1;
         // 
         // CategoryTreeEditor
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.CancelButton);
         this.Controls.Add(this.OkButton);
         this.Controls.Add(this.collectionChooser1);
         this.Controls.Add(this.textBox1);
         this.Name = "CategoryTreeEditor";
         this.Size = new System.Drawing.Size(560, 372);
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private System.Windows.Forms.TextBox textBox1;
      private CollectionChooser collectionChooser1;
      private System.Windows.Forms.Button OkButton;
      private System.Windows.Forms.Button CancelButton;
   }
}
