namespace RemoteMemory
{
   partial class LineCallstacksView
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
         System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(LineCallstacksView));
         this.label1 = new System.Windows.Forms.Label();
         this.listBox1 = new System.Windows.Forms.ListBox();
         this.SuspendLayout();
         // 
         // label1
         // 
         this.label1.AutoSize = true;
         this.label1.Font = new System.Drawing.Font("Microsoft Sans Serif", 24F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
         this.label1.ForeColor = System.Drawing.Color.Red;
         this.label1.Location = new System.Drawing.Point(26, 9);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(925, 37);
         this.label1.TabIndex = 4;
         this.label1.Text = "Due To Agressive optimization, line numbers may be slightly off.";
         // 
         // listBox1
         // 
         this.listBox1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.listBox1.FormattingEnabled = true;
         this.listBox1.Location = new System.Drawing.Point(12, 64);
         this.listBox1.Name = "listBox1";
         this.listBox1.Size = new System.Drawing.Size(943, 407);
         this.listBox1.TabIndex = 5;
         // 
         // LineCallstacksView
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.ClientSize = new System.Drawing.Size(967, 484);
         this.Controls.Add(this.listBox1);
         this.Controls.Add(this.label1);
         this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
         this.Name = "LineCallstacksView";
         this.Text = "LineCallstacksView";
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private System.Windows.Forms.Label label1;
      private System.Windows.Forms.ListBox listBox1;
   }
}