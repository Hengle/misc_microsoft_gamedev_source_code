namespace PhoenixEditor
{
   partial class ModifierControl
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
         this.Namelabel = new System.Windows.Forms.Label();
         this.floatSliderMin = new PhoenixEditor.FloatSlider();
         this.floatSliderMax = new PhoenixEditor.FloatSlider();
         this.checkBox1 = new System.Windows.Forms.CheckBox();
         this.SuspendLayout();
         // 
         // Namelabel
         // 
         this.Namelabel.AutoSize = true;
         this.Namelabel.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
         this.Namelabel.Location = new System.Drawing.Point(3, 7);
         this.Namelabel.Name = "Namelabel";
         this.Namelabel.Size = new System.Drawing.Size(51, 20);
         this.Namelabel.TabIndex = 0;
         this.Namelabel.Text = "Name";
         // 
         // floatSliderMin
         // 
         this.floatSliderMin.Location = new System.Drawing.Point(3, 36);
         this.floatSliderMin.Name = "floatSliderMin";
         this.floatSliderMin.Size = new System.Drawing.Size(249, 83);
         this.floatSliderMin.TabIndex = 2;
         // 
         // floatSliderMax
         // 
         this.floatSliderMax.Location = new System.Drawing.Point(271, 36);
         this.floatSliderMax.Name = "floatSliderMax";
         this.floatSliderMax.Size = new System.Drawing.Size(272, 83);
         this.floatSliderMax.TabIndex = 3;
         // 
         // checkBox1
         // 
         this.checkBox1.AutoSize = true;
         this.checkBox1.Location = new System.Drawing.Point(101, 9);
         this.checkBox1.Name = "checkBox1";
         this.checkBox1.Size = new System.Drawing.Size(67, 17);
         this.checkBox1.TabIndex = 4;
         this.checkBox1.Text = "Pressure";
         this.checkBox1.UseVisualStyleBackColor = true;
         this.checkBox1.CheckedChanged += new System.EventHandler(this.checkBox1_CheckedChanged);
         // 
         // ModifierControl
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
         this.Controls.Add(this.checkBox1);
         this.Controls.Add(this.floatSliderMax);
         this.Controls.Add(this.floatSliderMin);
         this.Controls.Add(this.Namelabel);
         this.Name = "ModifierControl";
         this.Size = new System.Drawing.Size(551, 111);
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private System.Windows.Forms.Label Namelabel;
      private FloatSlider floatSliderMin;
      private FloatSlider floatSliderMax;
      private System.Windows.Forms.CheckBox checkBox1;
   }
}
