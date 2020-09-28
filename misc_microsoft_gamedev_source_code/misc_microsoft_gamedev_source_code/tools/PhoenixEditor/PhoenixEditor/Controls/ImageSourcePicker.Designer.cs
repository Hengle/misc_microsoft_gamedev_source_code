namespace PhoenixEditor
{
   partial class ImageSourcePicker
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
         this.tabControl1 = new System.Windows.Forms.TabControl();
         this.tabPage1 = new System.Windows.Forms.TabPage();
         this.tabPage2 = new System.Windows.Forms.TabPage();
         this.tabPage3 = new System.Windows.Forms.TabPage();
         this.PickImageButton = new System.Windows.Forms.Button();
         this.tabPage4 = new System.Windows.Forms.TabPage();
         this.SolidButton = new System.Windows.Forms.Button();
         this.GradientButton = new System.Windows.Forms.Button();
         this.FractalButton = new System.Windows.Forms.Button();
         this.tabControl1.SuspendLayout();
         this.tabPage1.SuspendLayout();
         this.tabPage3.SuspendLayout();
         this.tabPage4.SuspendLayout();
         this.SuspendLayout();
         // 
         // tabControl1
         // 
         this.tabControl1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.tabControl1.Controls.Add(this.tabPage4);
         this.tabControl1.Controls.Add(this.tabPage1);
         this.tabControl1.Controls.Add(this.tabPage2);
         this.tabControl1.Controls.Add(this.tabPage3);
         this.tabControl1.Location = new System.Drawing.Point(3, 3);
         this.tabControl1.Name = "tabControl1";
         this.tabControl1.SelectedIndex = 0;
         this.tabControl1.Size = new System.Drawing.Size(389, 300);
         this.tabControl1.TabIndex = 0;
         // 
         // tabPage1
         // 
         this.tabPage1.Controls.Add(this.PickImageButton);
         this.tabPage1.Location = new System.Drawing.Point(4, 22);
         this.tabPage1.Name = "tabPage1";
         this.tabPage1.Padding = new System.Windows.Forms.Padding(3);
         this.tabPage1.Size = new System.Drawing.Size(381, 274);
         this.tabPage1.TabIndex = 0;
         this.tabPage1.Text = "Raster (bitmap) Image";
         this.tabPage1.UseVisualStyleBackColor = true;
         this.tabPage1.Click += new System.EventHandler(this.tabPage1_Click);
         // 
         // tabPage2
         // 
         this.tabPage2.Location = new System.Drawing.Point(4, 22);
         this.tabPage2.Name = "tabPage2";
         this.tabPage2.Padding = new System.Windows.Forms.Padding(3);
         this.tabPage2.Size = new System.Drawing.Size(381, 274);
         this.tabPage2.TabIndex = 1;
         this.tabPage2.Text = "Mask Data";
         this.tabPage2.UseVisualStyleBackColor = true;
         // 
         // tabPage3
         // 
         this.tabPage3.Controls.Add(this.FractalButton);
         this.tabPage3.Location = new System.Drawing.Point(4, 22);
         this.tabPage3.Name = "tabPage3";
         this.tabPage3.Padding = new System.Windows.Forms.Padding(3);
         this.tabPage3.Size = new System.Drawing.Size(381, 274);
         this.tabPage3.TabIndex = 2;
         this.tabPage3.Text = "Mathmatical (fractal)";
         this.tabPage3.UseVisualStyleBackColor = true;
         // 
         // PickImageButton
         // 
         this.PickImageButton.Location = new System.Drawing.Point(16, 50);
         this.PickImageButton.Name = "PickImageButton";
         this.PickImageButton.Size = new System.Drawing.Size(142, 23);
         this.PickImageButton.TabIndex = 0;
         this.PickImageButton.Text = "Pick Image";
         this.PickImageButton.UseVisualStyleBackColor = true;
         this.PickImageButton.Click += new System.EventHandler(this.PickImageButton_Click);
         // 
         // tabPage4
         // 
         this.tabPage4.Controls.Add(this.GradientButton);
         this.tabPage4.Controls.Add(this.SolidButton);
         this.tabPage4.Location = new System.Drawing.Point(4, 22);
         this.tabPage4.Name = "tabPage4";
         this.tabPage4.Padding = new System.Windows.Forms.Padding(3);
         this.tabPage4.Size = new System.Drawing.Size(381, 274);
         this.tabPage4.TabIndex = 3;
         this.tabPage4.Text = "Basic";
         this.tabPage4.UseVisualStyleBackColor = true;
         // 
         // SolidButton
         // 
         this.SolidButton.Location = new System.Drawing.Point(6, 26);
         this.SolidButton.Name = "SolidButton";
         this.SolidButton.Size = new System.Drawing.Size(75, 23);
         this.SolidButton.TabIndex = 0;
         this.SolidButton.Text = "Solid";
         this.SolidButton.UseVisualStyleBackColor = true;
         this.SolidButton.Click += new System.EventHandler(this.SolidButton_Click);
         // 
         // GradientButton
         // 
         this.GradientButton.Location = new System.Drawing.Point(6, 69);
         this.GradientButton.Name = "GradientButton";
         this.GradientButton.Size = new System.Drawing.Size(75, 23);
         this.GradientButton.TabIndex = 1;
         this.GradientButton.Text = "Gradient";
         this.GradientButton.UseVisualStyleBackColor = true;
         this.GradientButton.Click += new System.EventHandler(this.GradientButton_Click);
         // 
         // FractalButton
         // 
         this.FractalButton.Location = new System.Drawing.Point(6, 44);
         this.FractalButton.Name = "FractalButton";
         this.FractalButton.Size = new System.Drawing.Size(75, 23);
         this.FractalButton.TabIndex = 0;
         this.FractalButton.Text = "Fractal";
         this.FractalButton.UseVisualStyleBackColor = true;
         this.FractalButton.Click += new System.EventHandler(this.FractalButton_Click);
         // 
         // ImageSourcePicker
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.tabControl1);
         this.Name = "ImageSourcePicker";
         this.Size = new System.Drawing.Size(395, 306);
         this.tabControl1.ResumeLayout(false);
         this.tabPage1.ResumeLayout(false);
         this.tabPage3.ResumeLayout(false);
         this.tabPage4.ResumeLayout(false);
         this.ResumeLayout(false);

      }

      #endregion

      private System.Windows.Forms.TabControl tabControl1;
      private System.Windows.Forms.TabPage tabPage1;
      private System.Windows.Forms.TabPage tabPage2;
      private System.Windows.Forms.TabPage tabPage3;
      private System.Windows.Forms.TabPage tabPage4;
      private System.Windows.Forms.Button GradientButton;
      private System.Windows.Forms.Button SolidButton;
      private System.Windows.Forms.Button PickImageButton;
      private System.Windows.Forms.Button FractalButton;
   }
}
