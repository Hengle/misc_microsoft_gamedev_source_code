namespace EditorCore
{
   partial class FileBrowseControl
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
         this.buttonBrowse = new System.Windows.Forms.Button();
         this.buttonClear = new System.Windows.Forms.Button();
         this.SuspendLayout();
         // 
         // textBox1
         // 
         this.textBox1.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.textBox1.BackColor = System.Drawing.SystemColors.Window;
         this.textBox1.Location = new System.Drawing.Point(0, 2);
         this.textBox1.Margin = new System.Windows.Forms.Padding(3, 2, 3, 2);
         this.textBox1.Name = "textBox1";
         this.textBox1.ReadOnly = true;
         this.textBox1.Size = new System.Drawing.Size(246, 20);
         this.textBox1.TabIndex = 0;
         // 
         // buttonBrowse
         // 
         this.buttonBrowse.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
         this.buttonBrowse.Location = new System.Drawing.Point(249, -1);
         this.buttonBrowse.Margin = new System.Windows.Forms.Padding(0);
         this.buttonBrowse.Name = "buttonBrowse";
         this.buttonBrowse.Size = new System.Drawing.Size(24, 24);
         this.buttonBrowse.TabIndex = 1;
         this.buttonBrowse.Text = "...";
         this.buttonBrowse.UseVisualStyleBackColor = true;
         this.buttonBrowse.Click += new System.EventHandler(this.buttonBrowse_Click);
         // 
         // buttonClear
         // 
         this.buttonClear.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
         this.buttonClear.ForeColor = System.Drawing.SystemColors.ControlText;
         this.buttonClear.Location = new System.Drawing.Point(273, -1);
         this.buttonClear.Margin = new System.Windows.Forms.Padding(0);
         this.buttonClear.Name = "buttonClear";
         this.buttonClear.Size = new System.Drawing.Size(31, 24);
         this.buttonClear.TabIndex = 2;
         this.buttonClear.Text = "Clr";
         this.buttonClear.UseVisualStyleBackColor = true;
         this.buttonClear.Click += new System.EventHandler(this.buttonClear_Click);
         // 
         // FileBrowseControl
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.buttonClear);
         this.Controls.Add(this.buttonBrowse);
         this.Controls.Add(this.textBox1);
         this.Name = "FileBrowseControl";
         this.Size = new System.Drawing.Size(304, 24);
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private System.Windows.Forms.TextBox textBox1;
      private System.Windows.Forms.Button buttonBrowse;
      private System.Windows.Forms.Button buttonClear;
   }
}
