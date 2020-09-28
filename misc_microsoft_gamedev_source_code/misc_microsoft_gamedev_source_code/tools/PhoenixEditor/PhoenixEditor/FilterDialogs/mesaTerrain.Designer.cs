namespace PhoenixEditor.Filter_Dialogs
{
   partial class mesaTerrain
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
         this.button1 = new System.Windows.Forms.Button();
         this.button2 = new System.Windows.Forms.Button();
         this.checkBox1 = new System.Windows.Forms.CheckBox();
         this.label1 = new System.Windows.Forms.Label();
         this.label2 = new System.Windows.Forms.Label();
         this.trackBar1 = new System.Windows.Forms.TrackBar();
         this.trackBar2 = new System.Windows.Forms.TrackBar();
         this.label3 = new System.Windows.Forms.Label();
         this.label4 = new System.Windows.Forms.Label();
         this.label6 = new System.Windows.Forms.Label();
         this.label5 = new System.Windows.Forms.Label();
         this.numericUpDown1 = new System.Windows.Forms.NumericUpDown();
         this.numericUpDown2 = new System.Windows.Forms.NumericUpDown();
         this.checkBox2 = new System.Windows.Forms.CheckBox();
         this.label7 = new System.Windows.Forms.Label();
         ((System.ComponentModel.ISupportInitialize)(this.trackBar1)).BeginInit();
         ((System.ComponentModel.ISupportInitialize)(this.trackBar2)).BeginInit();
         ((System.ComponentModel.ISupportInitialize)(this.numericUpDown1)).BeginInit();
         ((System.ComponentModel.ISupportInitialize)(this.numericUpDown2)).BeginInit();
         this.SuspendLayout();
         // 
         // button1
         // 
         this.button1.Location = new System.Drawing.Point(271, 260);
         this.button1.Name = "button1";
         this.button1.Size = new System.Drawing.Size(75, 23);
         this.button1.TabIndex = 0;
         this.button1.Text = "Cancel";
         this.button1.UseVisualStyleBackColor = true;
         this.button1.Click += new System.EventHandler(this.button1_Click);
         // 
         // button2
         // 
         this.button2.Location = new System.Drawing.Point(190, 260);
         this.button2.Name = "button2";
         this.button2.Size = new System.Drawing.Size(75, 23);
         this.button2.TabIndex = 1;
         this.button2.Text = "Apply";
         this.button2.UseVisualStyleBackColor = true;
         this.button2.Click += new System.EventHandler(this.button2_Click);
         // 
         // checkBox1
         // 
         this.checkBox1.AutoSize = true;
         this.checkBox1.Checked = true;
         this.checkBox1.CheckState = System.Windows.Forms.CheckState.Checked;
         this.checkBox1.Location = new System.Drawing.Point(190, 228);
         this.checkBox1.Name = "checkBox1";
         this.checkBox1.Size = new System.Drawing.Size(169, 17);
         this.checkBox1.TabIndex = 2;
         this.checkBox1.Text = "Apply Only To Masked Terrain";
         this.checkBox1.UseVisualStyleBackColor = true;
         // 
         // label1
         // 
         this.label1.AutoSize = true;
         this.label1.Location = new System.Drawing.Point(8, 125);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(99, 13);
         this.label1.TabIndex = 5;
         this.label1.Text = "Mesa Top cuttoff %";
         // 
         // label2
         // 
         this.label2.AutoSize = true;
         this.label2.Location = new System.Drawing.Point(8, 166);
         this.label2.Name = "label2";
         this.label2.Size = new System.Drawing.Size(82, 13);
         this.label2.TabIndex = 6;
         this.label2.Text = "Mesa hill start %";
         // 
         // trackBar1
         // 
         this.trackBar1.LargeChange = 10;
         this.trackBar1.Location = new System.Drawing.Point(113, 115);
         this.trackBar1.Maximum = 100;
         this.trackBar1.Minimum = 1;
         this.trackBar1.Name = "trackBar1";
         this.trackBar1.Size = new System.Drawing.Size(104, 45);
         this.trackBar1.TabIndex = 7;
         this.trackBar1.Value = 65;
         this.trackBar1.Scroll += new System.EventHandler(this.trackBar1_Scroll);
         // 
         // trackBar2
         // 
         this.trackBar2.LargeChange = 10;
         this.trackBar2.Location = new System.Drawing.Point(113, 166);
         this.trackBar2.Maximum = 90;
         this.trackBar2.Minimum = 1;
         this.trackBar2.Name = "trackBar2";
         this.trackBar2.Size = new System.Drawing.Size(104, 45);
         this.trackBar2.TabIndex = 8;
         this.trackBar2.Value = 25;
         this.trackBar2.Scroll += new System.EventHandler(this.trackBar2_Scroll);
         // 
         // label3
         // 
         this.label3.AutoSize = true;
         this.label3.Location = new System.Drawing.Point(228, 125);
         this.label3.Name = "label3";
         this.label3.Size = new System.Drawing.Size(27, 13);
         this.label3.TabIndex = 9;
         this.label3.Text = "65%";
         // 
         // label4
         // 
         this.label4.AutoSize = true;
         this.label4.Location = new System.Drawing.Point(228, 179);
         this.label4.Name = "label4";
         this.label4.Size = new System.Drawing.Size(27, 13);
         this.label4.TabIndex = 10;
         this.label4.Text = "25%";
         // 
         // label6
         // 
         this.label6.AutoSize = true;
         this.label6.Location = new System.Drawing.Point(24, 78);
         this.label6.Name = "label6";
         this.label6.Size = new System.Drawing.Size(55, 13);
         this.label6.TabIndex = 14;
         this.label6.Text = "MinHeight";
         // 
         // label5
         // 
         this.label5.AutoSize = true;
         this.label5.Location = new System.Drawing.Point(24, 54);
         this.label5.Name = "label5";
         this.label5.Size = new System.Drawing.Size(59, 13);
         this.label5.TabIndex = 13;
         this.label5.Text = "Max height";
         // 
         // numericUpDown1
         // 
         this.numericUpDown1.DecimalPlaces = 3;
         this.numericUpDown1.Enabled = false;
         this.numericUpDown1.Increment = new decimal(new int[] {
            25,
            0,
            0,
            131072});
         this.numericUpDown1.Location = new System.Drawing.Point(113, 54);
         this.numericUpDown1.Maximum = new decimal(new int[] {
            40,
            0,
            0,
            0});
         this.numericUpDown1.Minimum = new decimal(new int[] {
            40,
            0,
            0,
            -2147483648});
         this.numericUpDown1.Name = "numericUpDown1";
         this.numericUpDown1.Size = new System.Drawing.Size(94, 20);
         this.numericUpDown1.TabIndex = 11;
         // 
         // numericUpDown2
         // 
         this.numericUpDown2.DecimalPlaces = 3;
         this.numericUpDown2.Enabled = false;
         this.numericUpDown2.Increment = new decimal(new int[] {
            25,
            0,
            0,
            131072});
         this.numericUpDown2.Location = new System.Drawing.Point(113, 80);
         this.numericUpDown2.Maximum = new decimal(new int[] {
            40,
            0,
            0,
            0});
         this.numericUpDown2.Minimum = new decimal(new int[] {
            40,
            0,
            0,
            -2147483648});
         this.numericUpDown2.Name = "numericUpDown2";
         this.numericUpDown2.Size = new System.Drawing.Size(94, 20);
         this.numericUpDown2.TabIndex = 12;
         // 
         // checkBox2
         // 
         this.checkBox2.AutoSize = true;
         this.checkBox2.Location = new System.Drawing.Point(8, 12);
         this.checkBox2.Name = "checkBox2";
         this.checkBox2.Size = new System.Drawing.Size(100, 17);
         this.checkBox2.TabIndex = 15;
         this.checkBox2.Text = "Specify Heights";
         this.checkBox2.UseVisualStyleBackColor = true;
         this.checkBox2.CheckedChanged += new System.EventHandler(this.checkBox2_CheckedChanged);
         // 
         // label7
         // 
         this.label7.AutoSize = true;
         this.label7.Location = new System.Drawing.Point(24, 32);
         this.label7.Name = "label7";
         this.label7.Size = new System.Drawing.Size(351, 13);
         this.label7.TabIndex = 16;
         this.label7.Text = "If Specify Heights is not selected, filter will sample terrain for max/min vals";
         // 
         // mesaTerrain
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.ClientSize = new System.Drawing.Size(378, 312);
         this.Controls.Add(this.label7);
         this.Controls.Add(this.checkBox2);
         this.Controls.Add(this.label6);
         this.Controls.Add(this.label5);
         this.Controls.Add(this.numericUpDown1);
         this.Controls.Add(this.numericUpDown2);
         this.Controls.Add(this.label1);
         this.Controls.Add(this.label4);
         this.Controls.Add(this.checkBox1);
         this.Controls.Add(this.trackBar2);
         this.Controls.Add(this.label2);
         this.Controls.Add(this.button2);
         this.Controls.Add(this.trackBar1);
         this.Controls.Add(this.button1);
         this.Controls.Add(this.label3);
         this.Name = "mesaTerrain";
         this.Text = "mesaTerrain";
         this.Load += new System.EventHandler(this.mesaTerrain_Load);
         ((System.ComponentModel.ISupportInitialize)(this.trackBar1)).EndInit();
         ((System.ComponentModel.ISupportInitialize)(this.trackBar2)).EndInit();
         ((System.ComponentModel.ISupportInitialize)(this.numericUpDown1)).EndInit();
         ((System.ComponentModel.ISupportInitialize)(this.numericUpDown2)).EndInit();
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private System.Windows.Forms.Button button1;
      private System.Windows.Forms.Button button2;
      private System.Windows.Forms.CheckBox checkBox1;
      private System.Windows.Forms.Label label1;
      private System.Windows.Forms.Label label2;
      private System.Windows.Forms.TrackBar trackBar1;
      private System.Windows.Forms.TrackBar trackBar2;
      private System.Windows.Forms.Label label3;
      private System.Windows.Forms.Label label4;
      private System.Windows.Forms.Label label6;
      private System.Windows.Forms.Label label5;
      private System.Windows.Forms.NumericUpDown numericUpDown1;
      private System.Windows.Forms.NumericUpDown numericUpDown2;
      private System.Windows.Forms.CheckBox checkBox2;
      private System.Windows.Forms.Label label7;
   }
}