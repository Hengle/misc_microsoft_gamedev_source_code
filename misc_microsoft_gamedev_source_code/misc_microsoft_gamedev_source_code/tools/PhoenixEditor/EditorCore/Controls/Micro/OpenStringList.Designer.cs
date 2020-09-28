namespace EditorCore.Controls.Micro
{
   partial class OpenStringList
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
         this.listBox1 = new System.Windows.Forms.ListBox();
         this.textBox1 = new System.Windows.Forms.TextBox();
         this.PasteButton = new System.Windows.Forms.Button();
         this.SuspendLayout();
         // 
         // listBox1
         // 
         this.listBox1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.listBox1.FormattingEnabled = true;
         this.listBox1.Location = new System.Drawing.Point(4, 4);
         this.listBox1.Name = "listBox1";
         this.listBox1.Size = new System.Drawing.Size(243, 329);
         this.listBox1.TabIndex = 0;
         // 
         // textBox1
         // 
         this.textBox1.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.textBox1.Location = new System.Drawing.Point(4, 354);
         this.textBox1.Name = "textBox1";
         this.textBox1.Size = new System.Drawing.Size(243, 20);
         this.textBox1.TabIndex = 1;
         // 
         // PasteButton
         // 
         this.PasteButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
         this.PasteButton.Location = new System.Drawing.Point(84, 380);
         this.PasteButton.Name = "PasteButton";
         this.PasteButton.Size = new System.Drawing.Size(75, 23);
         this.PasteButton.TabIndex = 2;
         this.PasteButton.Text = "Paste";
         this.PasteButton.UseVisualStyleBackColor = true;
         this.PasteButton.Click += new System.EventHandler(this.PasteButton_Click);
         // 
         // OpenStringList
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.PasteButton);
         this.Controls.Add(this.textBox1);
         this.Controls.Add(this.listBox1);
         this.Name = "OpenStringList";
         this.Size = new System.Drawing.Size(250, 406);
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private System.Windows.Forms.ListBox listBox1;
      private System.Windows.Forms.TextBox textBox1;
      private System.Windows.Forms.Button PasteButton;
   }
}
