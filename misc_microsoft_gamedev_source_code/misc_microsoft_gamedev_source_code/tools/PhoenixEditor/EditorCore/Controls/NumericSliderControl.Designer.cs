namespace EditorCore
{
   partial class NumericSliderControl
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
         this.trackBar1 = new System.Windows.Forms.TrackBar();
         ((System.ComponentModel.ISupportInitialize)(this.trackBar1)).BeginInit();
         this.SuspendLayout();
         // 
         // textBox1
         // 
         this.textBox1.Location = new System.Drawing.Point(0, 0);
         this.textBox1.Margin = new System.Windows.Forms.Padding(0);
         this.textBox1.Name = "textBox1";
         this.textBox1.Size = new System.Drawing.Size(69, 20);
         this.textBox1.TabIndex = 0;
         this.textBox1.TextChanged += new System.EventHandler(this.textBox1_TextChanged);
         // 
         // trackBar1
         // 
         this.trackBar1.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.trackBar1.BackColor = System.Drawing.SystemColors.ControlLight;
         this.trackBar1.Location = new System.Drawing.Point(69, 0);
         this.trackBar1.Margin = new System.Windows.Forms.Padding(0);
         this.trackBar1.Name = "trackBar1";
         this.trackBar1.Size = new System.Drawing.Size(125, 45);
         this.trackBar1.TabIndex = 1;
         this.trackBar1.TabStop = false;
         this.trackBar1.TickStyle = System.Windows.Forms.TickStyle.None;
         this.trackBar1.Scroll += new System.EventHandler(this.trackBar1_Scroll);
         // 
         // NumericSliderControl
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.BackColor = System.Drawing.SystemColors.ControlLight;
         this.Controls.Add(this.trackBar1);
         this.Controls.Add(this.textBox1);
         this.Name = "NumericSliderControl";
         this.Size = new System.Drawing.Size(195, 22);
         ((System.ComponentModel.ISupportInitialize)(this.trackBar1)).EndInit();
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private System.Windows.Forms.TextBox textBox1;
      private System.Windows.Forms.TrackBar trackBar1;
   }
}
