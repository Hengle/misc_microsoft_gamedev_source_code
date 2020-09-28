namespace ParticleSystem
{
   partial class AngleControl
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
         this.floatSliderEdit1 = new VisualEditor.Controls.FloatSliderEdit();
         this.panel1 = new System.Windows.Forms.Panel();
         this.SuspendLayout();
         // 
         // floatSliderEdit1
         // 
         this.floatSliderEdit1.Location = new System.Drawing.Point(126, 320);
         this.floatSliderEdit1.Margin = new System.Windows.Forms.Padding(3, 3, 3, 0);
         this.floatSliderEdit1.MaxValue = 360F;
         this.floatSliderEdit1.MinValue = 0F;
         this.floatSliderEdit1.Name = "floatSliderEdit1";
         this.floatSliderEdit1.Size = new System.Drawing.Size(212, 40);
         this.floatSliderEdit1.TabIndex = 1;
         this.floatSliderEdit1.Value = 0F;         
         // 
         // panel1
         // 
         this.panel1.Location = new System.Drawing.Point(126, 165);
         this.panel1.Name = "panel1";
         this.panel1.Size = new System.Drawing.Size(212, 149);
         this.panel1.TabIndex = 2;
         this.panel1.Paint += new System.Windows.Forms.PaintEventHandler(this.panel1_Paint);
         // 
         // AngleControl
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.panel1);
         this.Controls.Add(this.floatSliderEdit1);
         this.Name = "AngleControl";
         this.Size = new System.Drawing.Size(625, 447);
         this.ResumeLayout(false);

      }

      #endregion

      private VisualEditor.Controls.FloatSliderEdit floatSliderEdit1;
      private System.Windows.Forms.Panel panel1;

   }
}
