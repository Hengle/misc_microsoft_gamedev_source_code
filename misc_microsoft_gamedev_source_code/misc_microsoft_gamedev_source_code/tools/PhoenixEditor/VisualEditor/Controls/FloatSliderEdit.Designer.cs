namespace VisualEditor.Controls
{
   partial class FloatSliderEdit
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
         this.trackBar1 = new System.Windows.Forms.TrackBar();
         this.textBox1 = new System.Windows.Forms.TextBox();
         ((System.ComponentModel.ISupportInitialize)(this.trackBar1)).BeginInit();
         this.SuspendLayout();
         // 
         // trackBar1
         // 
         this.trackBar1.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.trackBar1.LargeChange = 25;
         this.trackBar1.Location = new System.Drawing.Point(-7, 0);
         this.trackBar1.Margin = new System.Windows.Forms.Padding(0, 3, 0, 3);
         this.trackBar1.Maximum = 1000;
         this.trackBar1.Name = "trackBar1";
         this.trackBar1.Size = new System.Drawing.Size(182, 45);
         this.trackBar1.SmallChange = 5;
         this.trackBar1.TabIndex = 0;
         this.trackBar1.TickFrequency = 100;
         this.trackBar1.Scroll += new System.EventHandler(this.trackBar1_Scroll);
         // 
         // textBox1
         // 
         this.textBox1.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
         this.textBox1.Location = new System.Drawing.Point(178, 3);
         this.textBox1.MaxLength = 5;
         this.textBox1.Name = "textBox1";
         this.textBox1.Size = new System.Drawing.Size(34, 20);
         this.textBox1.TabIndex = 1;
         this.textBox1.KeyUp += new System.Windows.Forms.KeyEventHandler(this.textBox1_KeyUp);
         this.textBox1.KeyDown += new System.Windows.Forms.KeyEventHandler(this.textBox1_KeyDown);
         // 
         // FloatSliderEdit
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.textBox1);
         this.Controls.Add(this.trackBar1);
         this.Margin = new System.Windows.Forms.Padding(3, 3, 3, 0);
         this.Name = "FloatSliderEdit";
         this.Size = new System.Drawing.Size(212, 40);
         ((System.ComponentModel.ISupportInitialize)(this.trackBar1)).EndInit();
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private System.Windows.Forms.TrackBar trackBar1;
      private System.Windows.Forms.TextBox textBox1;
   }
}
