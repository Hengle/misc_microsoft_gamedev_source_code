namespace PhoenixEditor
{
   partial class FloatSlider
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
         this.Minlabel = new System.Windows.Forms.Label();
         this.Maxlabel = new System.Windows.Forms.Label();
         this.Namelabel = new System.Windows.Forms.Label();
         ((System.ComponentModel.ISupportInitialize)(this.trackBar1)).BeginInit();
         this.SuspendLayout();
         // 
         // trackBar1
         // 
         this.trackBar1.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.trackBar1.LargeChange = 100;
         this.trackBar1.Location = new System.Drawing.Point(3, 35);
         this.trackBar1.Maximum = 1000;
         this.trackBar1.Name = "trackBar1";
         this.trackBar1.Size = new System.Drawing.Size(363, 45);
         this.trackBar1.TabIndex = 0;
         this.trackBar1.TickStyle = System.Windows.Forms.TickStyle.None;
         this.trackBar1.ValueChanged += new System.EventHandler(this.trackBar1_ValueChanged);
         // 
         // Minlabel
         // 
         this.Minlabel.AutoSize = true;
         this.Minlabel.Location = new System.Drawing.Point(14, 19);
         this.Minlabel.Name = "Minlabel";
         this.Minlabel.Size = new System.Drawing.Size(24, 13);
         this.Minlabel.TabIndex = 1;
         this.Minlabel.Text = "Min";
         // 
         // Maxlabel
         // 
         this.Maxlabel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
         this.Maxlabel.AutoSize = true;
         this.Maxlabel.Location = new System.Drawing.Point(336, 19);
         this.Maxlabel.Name = "Maxlabel";
         this.Maxlabel.Size = new System.Drawing.Size(27, 13);
         this.Maxlabel.TabIndex = 2;
         this.Maxlabel.Text = "Max";
         this.Maxlabel.TextAlign = System.Drawing.ContentAlignment.TopRight;
         // 
         // Namelabel
         // 
         this.Namelabel.AutoSize = true;
         this.Namelabel.Location = new System.Drawing.Point(90, 9);
         this.Namelabel.Name = "Namelabel";
         this.Namelabel.Size = new System.Drawing.Size(35, 13);
         this.Namelabel.TabIndex = 3;
         this.Namelabel.Text = "Name";
         // 
         // FloatSlider
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.Namelabel);
         this.Controls.Add(this.Maxlabel);
         this.Controls.Add(this.Minlabel);
         this.Controls.Add(this.trackBar1);
         this.Name = "FloatSlider";
         this.Size = new System.Drawing.Size(366, 83);
         ((System.ComponentModel.ISupportInitialize)(this.trackBar1)).EndInit();
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private System.Windows.Forms.TrackBar trackBar1;
      private System.Windows.Forms.Label Minlabel;
      private System.Windows.Forms.Label Maxlabel;
      private System.Windows.Forms.Label Namelabel;
   }
}
