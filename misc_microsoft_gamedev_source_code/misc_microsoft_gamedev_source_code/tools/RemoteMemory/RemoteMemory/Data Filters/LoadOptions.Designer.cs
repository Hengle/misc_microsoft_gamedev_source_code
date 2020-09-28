namespace RemoteMemory
{
   partial class LoadOptions
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
         this.numericUpDown1 = new System.Windows.Forms.NumericUpDown();
         this.label1 = new System.Windows.Forms.Label();
         this.label2 = new System.Windows.Forms.Label();
         ((System.ComponentModel.ISupportInitialize)(this.numericUpDown1)).BeginInit();
         this.SuspendLayout();
         // 
         // numericUpDown1
         // 
         this.numericUpDown1.Location = new System.Drawing.Point(134, 8);
         this.numericUpDown1.Maximum = new decimal(new int[] {
            16,
            0,
            0,
            0});
         this.numericUpDown1.Minimum = new decimal(new int[] {
            1,
            0,
            0,
            0});
         this.numericUpDown1.Name = "numericUpDown1";
         this.numericUpDown1.Size = new System.Drawing.Size(54, 20);
         this.numericUpDown1.TabIndex = 0;
         this.numericUpDown1.Value = new decimal(new int[] {
            1,
            0,
            0,
            0});
         this.numericUpDown1.ValueChanged += new System.EventHandler(this.numericUpDown1_ValueChanged);
         // 
         // label1
         // 
         this.label1.AutoSize = true;
         this.label1.Location = new System.Drawing.Point(4, 10);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(124, 13);
         this.label1.TabIndex = 1;
         this.label1.Text = "Max Stack Walk Depth :";
         // 
         // label2
         // 
         this.label2.AutoSize = true;
         this.label2.ForeColor = System.Drawing.Color.Red;
         this.label2.Location = new System.Drawing.Point(194, 10);
         this.label2.Name = "label2";
         this.label2.Size = new System.Drawing.Size(259, 13);
         this.label2.TabIndex = 2;
         this.label2.Text = "*Note, increasing this number will slow down analysis*";
         // 
         // LoadOptions
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.label2);
         this.Controls.Add(this.label1);
         this.Controls.Add(this.numericUpDown1);
         this.Name = "LoadOptions";
         this.Size = new System.Drawing.Size(478, 56);
         ((System.ComponentModel.ISupportInitialize)(this.numericUpDown1)).EndInit();
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private System.Windows.Forms.NumericUpDown numericUpDown1;
      private System.Windows.Forms.Label label1;
      private System.Windows.Forms.Label label2;
   }
}
