namespace EditorCore.Controls.Micro
{
   partial class ConciseList
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
         this.EditButton = new System.Windows.Forms.Button();
         this.SuspendLayout();
         // 
         // listBox1
         // 
         this.listBox1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.listBox1.FormattingEnabled = true;
         this.listBox1.Location = new System.Drawing.Point(3, 3);
         this.listBox1.Name = "listBox1";
         this.listBox1.Size = new System.Drawing.Size(456, 147);
         this.listBox1.TabIndex = 0;
         // 
         // EditButton
         // 
         this.EditButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
         this.EditButton.Location = new System.Drawing.Point(465, 3);
         this.EditButton.Name = "EditButton";
         this.EditButton.Size = new System.Drawing.Size(38, 25);
         this.EditButton.TabIndex = 1;
         this.EditButton.Text = "edit";
         this.EditButton.UseVisualStyleBackColor = true;
         this.EditButton.Click += new System.EventHandler(this.EditButton_Click);
         // 
         // ConciseList
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.EditButton);
         this.Controls.Add(this.listBox1);
         this.Name = "ConciseList";
         this.Size = new System.Drawing.Size(506, 156);
         this.ResumeLayout(false);

      }

      #endregion

      private System.Windows.Forms.ListBox listBox1;
      private System.Windows.Forms.Button EditButton;
   }
}
