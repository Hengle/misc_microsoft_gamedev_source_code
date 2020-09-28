namespace PhoenixEditor.ScenarioEditor
{
   partial class ApplyTransPropFrm
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
         this.label2 = new System.Windows.Forms.Label();
         this.label1 = new System.Windows.Forms.Label();
         this.heightOffsetSlider = new EditorCore.NumericSliderControl();
         this.label4 = new System.Windows.Forms.Label();
         this.SuspendLayout();
         // 
         // label2
         // 
         this.label2.AutoSize = true;
         this.label2.Location = new System.Drawing.Point(92, 16);
         this.label2.Name = "label2";
         this.label2.Size = new System.Drawing.Size(70, 13);
         this.label2.TabIndex = 22;
         this.label2.Text = "Current Mask";
         // 
         // label1
         // 
         this.label1.AutoSize = true;
         this.label1.Location = new System.Drawing.Point(13, 16);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(73, 13);
         this.label1.TabIndex = 21;
         this.label1.Text = "Target Mask :";
         // 
         // heightOffsetSlider
         // 
         this.heightOffsetSlider.BackColor = System.Drawing.SystemColors.ControlLight;
         this.heightOffsetSlider.Location = new System.Drawing.Point(150, 60);
         this.heightOffsetSlider.Name = "heightOffsetSlider";
         this.heightOffsetSlider.Size = new System.Drawing.Size(249, 22);
         this.heightOffsetSlider.TabIndex = 26;
         this.heightOffsetSlider.Load += new System.EventHandler(this.numberToPlaceSlider_Load);
         // 
         // label4
         // 
         this.label4.AutoSize = true;
         this.label4.Location = new System.Drawing.Point(15, 60);
         this.label4.Name = "label4";
         this.label4.Size = new System.Drawing.Size(78, 13);
         this.label4.TabIndex = 25;
         this.label4.Text = "Height Offset : ";
         this.label4.Click += new System.EventHandler(this.label4_Click);
         // 
         // ApplyTransPropFrm
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.heightOffsetSlider);
         this.Controls.Add(this.label4);
         this.Controls.Add(this.label2);
         this.Controls.Add(this.label1);
         this.Name = "ApplyTransPropFrm";
         this.Size = new System.Drawing.Size(481, 281);
         this.Load += new System.EventHandler(this.ApplyTransPropFrm_Load);
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private System.Windows.Forms.Label label2;
      private System.Windows.Forms.Label label1;
      private EditorCore.NumericSliderControl heightOffsetSlider;
      private System.Windows.Forms.Label label4;
   }
}
