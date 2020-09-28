namespace PhoenixEditor
{
   partial class NoiseSettings
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
         this.components = new System.ComponentModel.Container();
         this.pictureBox1 = new System.Windows.Forms.PictureBox();
         this.AbsCheckBox = new System.Windows.Forms.CheckBox();
         this.CosCheckBox = new System.Windows.Forms.CheckBox();
         this.timer1 = new System.Windows.Forms.Timer(this.components);
         this.AlwaysPoscheckBox = new System.Windows.Forms.CheckBox();
         this.floatSlider6 = new PhoenixEditor.FloatSlider();
         this.floatSlider5 = new PhoenixEditor.FloatSlider();
         this.floatSlider4 = new PhoenixEditor.FloatSlider();
         this.floatSlider3 = new PhoenixEditor.FloatSlider();
         this.floatSlider2 = new PhoenixEditor.FloatSlider();
         this.floatSlider1 = new PhoenixEditor.FloatSlider();
         ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).BeginInit();
         this.SuspendLayout();
         // 
         // pictureBox1
         // 
         this.pictureBox1.Location = new System.Drawing.Point(69, 301);
         this.pictureBox1.Name = "pictureBox1";
         this.pictureBox1.Size = new System.Drawing.Size(274, 165);
         this.pictureBox1.TabIndex = 4;
         this.pictureBox1.TabStop = false;
         // 
         // AbsCheckBox
         // 
         this.AbsCheckBox.AutoSize = true;
         this.AbsCheckBox.Location = new System.Drawing.Point(289, 261);
         this.AbsCheckBox.Name = "AbsCheckBox";
         this.AbsCheckBox.Size = new System.Drawing.Size(82, 17);
         this.AbsCheckBox.TabIndex = 7;
         this.AbsCheckBox.Text = "ABS effects";
         this.AbsCheckBox.UseVisualStyleBackColor = true;
         this.AbsCheckBox.CheckedChanged += new System.EventHandler(this.AbsCheckBox_CheckedChanged);
         // 
         // CosCheckBox
         // 
         this.CosCheckBox.AutoSize = true;
         this.CosCheckBox.Location = new System.Drawing.Point(174, 261);
         this.CosCheckBox.Name = "CosCheckBox";
         this.CosCheckBox.Size = new System.Drawing.Size(79, 17);
         this.CosCheckBox.TabIndex = 8;
         this.CosCheckBox.Text = "Cos effects";
         this.CosCheckBox.UseVisualStyleBackColor = true;
         this.CosCheckBox.CheckedChanged += new System.EventHandler(this.CosCheckBox_CheckedChanged);
         // 
         // timer1
         // 
         this.timer1.Enabled = true;
         this.timer1.Interval = 1500;
         this.timer1.Tick += new System.EventHandler(this.timer1_Tick);
         // 
         // AlwaysPoscheckBox
         // 
         this.AlwaysPoscheckBox.AutoSize = true;
         this.AlwaysPoscheckBox.Location = new System.Drawing.Point(21, 261);
         this.AlwaysPoscheckBox.Name = "AlwaysPoscheckBox";
         this.AlwaysPoscheckBox.Size = new System.Drawing.Size(99, 17);
         this.AlwaysPoscheckBox.TabIndex = 9;
         this.AlwaysPoscheckBox.Text = "Always Positive";
         this.AlwaysPoscheckBox.UseVisualStyleBackColor = true;
         this.AlwaysPoscheckBox.CheckedChanged += new System.EventHandler(this.AlwaysPoscheckBox_CheckedChanged);
         // 
         // floatSlider6
         // 
         this.floatSlider6.Location = new System.Drawing.Point(227, 182);
         this.floatSlider6.Name = "floatSlider6";
         this.floatSlider6.Size = new System.Drawing.Size(210, 83);
         this.floatSlider6.TabIndex = 6;
         // 
         // floatSlider5
         // 
         this.floatSlider5.Location = new System.Drawing.Point(11, 182);
         this.floatSlider5.Name = "floatSlider5";
         this.floatSlider5.Size = new System.Drawing.Size(210, 83);
         this.floatSlider5.TabIndex = 5;
         // 
         // floatSlider4
         // 
         this.floatSlider4.Location = new System.Drawing.Point(224, 105);
         this.floatSlider4.Name = "floatSlider4";
         this.floatSlider4.Size = new System.Drawing.Size(210, 83);
         this.floatSlider4.TabIndex = 3;
         // 
         // floatSlider3
         // 
         this.floatSlider3.Location = new System.Drawing.Point(8, 105);
         this.floatSlider3.Name = "floatSlider3";
         this.floatSlider3.Size = new System.Drawing.Size(210, 83);
         this.floatSlider3.TabIndex = 2;
         // 
         // floatSlider2
         // 
         this.floatSlider2.Location = new System.Drawing.Point(224, 16);
         this.floatSlider2.Name = "floatSlider2";
         this.floatSlider2.Size = new System.Drawing.Size(210, 83);
         this.floatSlider2.TabIndex = 1;
         // 
         // floatSlider1
         // 
         this.floatSlider1.Location = new System.Drawing.Point(8, 16);
         this.floatSlider1.Name = "floatSlider1";
         this.floatSlider1.Size = new System.Drawing.Size(210, 83);
         this.floatSlider1.TabIndex = 0;
         // 
         // NoiseSettings
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.AlwaysPoscheckBox);
         this.Controls.Add(this.CosCheckBox);
         this.Controls.Add(this.AbsCheckBox);
         this.Controls.Add(this.floatSlider6);
         this.Controls.Add(this.floatSlider5);
         this.Controls.Add(this.pictureBox1);
         this.Controls.Add(this.floatSlider4);
         this.Controls.Add(this.floatSlider3);
         this.Controls.Add(this.floatSlider2);
         this.Controls.Add(this.floatSlider1);
         this.FloatingWindowBounds = new System.Drawing.Rectangle(0, 0, 462, 523);
         this.Key = "NoiseSettings";
         this.Name = "NoiseSettings";
         this.Size = new System.Drawing.Size(462, 523);
         this.Text = "NoiseSettings";
         ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).EndInit();
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private FloatSlider floatSlider1;
      private FloatSlider floatSlider2;
      private FloatSlider floatSlider3;
      private FloatSlider floatSlider4;
      private System.Windows.Forms.PictureBox pictureBox1;
      private FloatSlider floatSlider5;
      private FloatSlider floatSlider6;
      private System.Windows.Forms.CheckBox AbsCheckBox;
      private System.Windows.Forms.CheckBox CosCheckBox;
      private System.Windows.Forms.Timer timer1;
      private System.Windows.Forms.CheckBox AlwaysPoscheckBox;
   }
}