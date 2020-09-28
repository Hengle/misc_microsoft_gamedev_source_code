namespace EditorCore
{
   partial class ProgressForm
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
         this.listBox1 = new System.Windows.Forms.ListBox();
         this.textBox1 = new System.Windows.Forms.TextBox();
         this.SuspendLayout();
         // 
         // listBox1
         // 
         this.listBox1.FormattingEnabled = true;
         this.listBox1.Location = new System.Drawing.Point(12, 12);
         this.listBox1.Name = "listBox1";
         this.listBox1.Size = new System.Drawing.Size(379, 186);
         this.listBox1.TabIndex = 0;
         // 
         // textBox1
         // 
         this.textBox1.Location = new System.Drawing.Point(13, 205);
         this.textBox1.Name = "textBox1";
         this.textBox1.Size = new System.Drawing.Size(378, 20);
         this.textBox1.TabIndex = 1;
         // 
         // ProgressForm
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.ClientSize = new System.Drawing.Size(431, 248);
         this.Controls.Add(this.textBox1);
         this.Controls.Add(this.listBox1);
         this.Name = "ProgressForm";
         this.Text = "ProgressForm";
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private System.Windows.Forms.ListBox listBox1;
      private System.Windows.Forms.TextBox textBox1;
   }
}