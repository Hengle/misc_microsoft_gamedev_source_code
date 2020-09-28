namespace ParticleSystem
{
   partial class SpeedPage
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
         this.groupBox1 = new System.Windows.Forms.GroupBox();
         this.vectorProgressionControl1 = new EditorCore.VectorProgressionControl();
         this.groupBox2 = new System.Windows.Forms.GroupBox();
         this.checkBox3 = new System.Windows.Forms.CheckBox();
         this.checkBox2 = new System.Windows.Forms.CheckBox();
         this.checkBox1 = new System.Windows.Forms.CheckBox();
         this.numericUpDown6 = new System.Windows.Forms.NumericUpDown();
         this.numericUpDown5 = new System.Windows.Forms.NumericUpDown();
         this.numericUpDown4 = new System.Windows.Forms.NumericUpDown();
         this.label6 = new System.Windows.Forms.Label();
         this.label5 = new System.Windows.Forms.Label();
         this.label4 = new System.Windows.Forms.Label();
         this.label3 = new System.Windows.Forms.Label();
         this.label2 = new System.Windows.Forms.Label();
         this.label1 = new System.Windows.Forms.Label();
         this.numericUpDown3 = new System.Windows.Forms.NumericUpDown();
         this.numericUpDown2 = new System.Windows.Forms.NumericUpDown();
         this.numericUpDown1 = new System.Windows.Forms.NumericUpDown();
         this.groupBox1.SuspendLayout();
         this.groupBox2.SuspendLayout();
         ((System.ComponentModel.ISupportInitialize)(this.numericUpDown6)).BeginInit();
         ((System.ComponentModel.ISupportInitialize)(this.numericUpDown5)).BeginInit();
         ((System.ComponentModel.ISupportInitialize)(this.numericUpDown4)).BeginInit();
         ((System.ComponentModel.ISupportInitialize)(this.numericUpDown3)).BeginInit();
         ((System.ComponentModel.ISupportInitialize)(this.numericUpDown2)).BeginInit();
         ((System.ComponentModel.ISupportInitialize)(this.numericUpDown1)).BeginInit();
         this.SuspendLayout();
         // 
         // groupBox1
         // 
         this.groupBox1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.groupBox1.Controls.Add(this.vectorProgressionControl1);
         this.groupBox1.Location = new System.Drawing.Point(0, 89);
         this.groupBox1.Name = "groupBox1";
         this.groupBox1.Size = new System.Drawing.Size(951, 636);
         this.groupBox1.TabIndex = 1;
         this.groupBox1.TabStop = false;
         this.groupBox1.Text = "Speed Progression";
         // 
         // vectorProgressionControl1
         // 
         this.vectorProgressionControl1.Dock = System.Windows.Forms.DockStyle.Fill;
         this.vectorProgressionControl1.Location = new System.Drawing.Point(3, 16);
         this.vectorProgressionControl1.Name = "vectorProgressionControl1";
         this.vectorProgressionControl1.Size = new System.Drawing.Size(945, 617);
         this.vectorProgressionControl1.TabIndex = 0;
         // 
         // groupBox2
         // 
         this.groupBox2.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.groupBox2.Controls.Add(this.checkBox3);
         this.groupBox2.Controls.Add(this.checkBox2);
         this.groupBox2.Controls.Add(this.checkBox1);
         this.groupBox2.Controls.Add(this.numericUpDown6);
         this.groupBox2.Controls.Add(this.numericUpDown5);
         this.groupBox2.Controls.Add(this.numericUpDown4);
         this.groupBox2.Controls.Add(this.label6);
         this.groupBox2.Controls.Add(this.label5);
         this.groupBox2.Controls.Add(this.label4);
         this.groupBox2.Controls.Add(this.label3);
         this.groupBox2.Controls.Add(this.label2);
         this.groupBox2.Controls.Add(this.label1);
         this.groupBox2.Controls.Add(this.numericUpDown3);
         this.groupBox2.Controls.Add(this.numericUpDown2);
         this.groupBox2.Controls.Add(this.numericUpDown1);
         this.groupBox2.Location = new System.Drawing.Point(0, 0);
         this.groupBox2.Name = "groupBox2";
         this.groupBox2.Size = new System.Drawing.Size(948, 87);
         this.groupBox2.TabIndex = 2;
         this.groupBox2.TabStop = false;
         this.groupBox2.Text = "Speed";
         // 
         // checkBox3
         // 
         this.checkBox3.AutoSize = true;
         this.checkBox3.Location = new System.Drawing.Point(390, 65);
         this.checkBox3.Name = "checkBox3";
         this.checkBox3.Size = new System.Drawing.Size(113, 17);
         this.checkBox3.TabIndex = 14;
         this.checkBox3.TabStop = false;
         this.checkBox3.Text = "Use Z Progression";
         this.checkBox3.UseVisualStyleBackColor = true;
         this.checkBox3.CheckedChanged += new System.EventHandler(this.checkBox_CheckedChanged);
         // 
         // checkBox2
         // 
         this.checkBox2.AutoSize = true;
         this.checkBox2.Location = new System.Drawing.Point(390, 38);
         this.checkBox2.Name = "checkBox2";
         this.checkBox2.Size = new System.Drawing.Size(113, 17);
         this.checkBox2.TabIndex = 13;
         this.checkBox2.TabStop = false;
         this.checkBox2.Text = "Use Y Progression";
         this.checkBox2.UseVisualStyleBackColor = true;
         this.checkBox2.CheckedChanged += new System.EventHandler(this.checkBox_CheckedChanged);
         // 
         // checkBox1
         // 
         this.checkBox1.AutoSize = true;
         this.checkBox1.Location = new System.Drawing.Point(390, 12);
         this.checkBox1.Name = "checkBox1";
         this.checkBox1.Size = new System.Drawing.Size(113, 17);
         this.checkBox1.TabIndex = 12;
         this.checkBox1.TabStop = false;
         this.checkBox1.Text = "Use X Progression";
         this.checkBox1.UseVisualStyleBackColor = true;
         this.checkBox1.CheckedChanged += new System.EventHandler(this.checkBox_CheckedChanged);
         // 
         // numericUpDown6
         // 
         this.numericUpDown6.Location = new System.Drawing.Point(281, 63);
         this.numericUpDown6.Maximum = new decimal(new int[] {
            16000,
            0,
            0,
            0});
         this.numericUpDown6.Name = "numericUpDown6";
         this.numericUpDown6.Size = new System.Drawing.Size(93, 20);
         this.numericUpDown6.TabIndex = 11;
         this.numericUpDown6.Enter += new System.EventHandler(this.numericUpDown_Enter);
         this.numericUpDown6.ValueChanged += new System.EventHandler(this.numericUpDown1_ValueChanged);
         // 
         // numericUpDown5
         // 
         this.numericUpDown5.Location = new System.Drawing.Point(281, 37);
         this.numericUpDown5.Maximum = new decimal(new int[] {
            16000,
            0,
            0,
            0});
         this.numericUpDown5.Name = "numericUpDown5";
         this.numericUpDown5.Size = new System.Drawing.Size(93, 20);
         this.numericUpDown5.TabIndex = 10;
         this.numericUpDown5.Enter += new System.EventHandler(this.numericUpDown_Enter);
         this.numericUpDown5.ValueChanged += new System.EventHandler(this.numericUpDown1_ValueChanged);
         // 
         // numericUpDown4
         // 
         this.numericUpDown4.Location = new System.Drawing.Point(281, 11);
         this.numericUpDown4.Maximum = new decimal(new int[] {
            16000,
            0,
            0,
            0});
         this.numericUpDown4.Name = "numericUpDown4";
         this.numericUpDown4.Size = new System.Drawing.Size(93, 20);
         this.numericUpDown4.TabIndex = 9;
         this.numericUpDown4.Enter += new System.EventHandler(this.numericUpDown_Enter);
         this.numericUpDown4.ValueChanged += new System.EventHandler(this.numericUpDown1_ValueChanged);
         // 
         // label6
         // 
         this.label6.AutoSize = true;
         this.label6.Location = new System.Drawing.Point(200, 65);
         this.label6.Name = "label6";
         this.label6.Size = new System.Drawing.Size(66, 13);
         this.label6.TabIndex = 8;
         this.label6.Text = "Variance (%)";
         // 
         // label5
         // 
         this.label5.AutoSize = true;
         this.label5.Location = new System.Drawing.Point(200, 39);
         this.label5.Name = "label5";
         this.label5.Size = new System.Drawing.Size(66, 13);
         this.label5.TabIndex = 7;
         this.label5.Text = "Variance (%)";
         // 
         // label4
         // 
         this.label4.AutoSize = true;
         this.label4.Location = new System.Drawing.Point(200, 13);
         this.label4.Name = "label4";
         this.label4.Size = new System.Drawing.Size(66, 13);
         this.label4.TabIndex = 6;
         this.label4.Text = "Variance (%)";
         // 
         // label3
         // 
         this.label3.AutoSize = true;
         this.label3.Location = new System.Drawing.Point(11, 65);
         this.label3.Name = "label3";
         this.label3.Size = new System.Drawing.Size(65, 13);
         this.label3.TabIndex = 5;
         this.label3.Text = "Z Speed (%)";
         // 
         // label2
         // 
         this.label2.AutoSize = true;
         this.label2.Location = new System.Drawing.Point(11, 39);
         this.label2.Name = "label2";
         this.label2.Size = new System.Drawing.Size(65, 13);
         this.label2.TabIndex = 4;
         this.label2.Text = "Y Speed (%)";
         // 
         // label1
         // 
         this.label1.AutoSize = true;
         this.label1.Location = new System.Drawing.Point(11, 13);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(65, 13);
         this.label1.TabIndex = 3;
         this.label1.Text = "X Speed (%)";
         // 
         // numericUpDown3
         // 
         this.numericUpDown3.Location = new System.Drawing.Point(83, 63);
         this.numericUpDown3.Maximum = new decimal(new int[] {
            32000,
            0,
            0,
            0});
         this.numericUpDown3.Minimum = new decimal(new int[] {
            32000,
            0,
            0,
            -2147483648});
         this.numericUpDown3.Name = "numericUpDown3";
         this.numericUpDown3.Size = new System.Drawing.Size(93, 20);
         this.numericUpDown3.TabIndex = 2;
         this.numericUpDown3.Enter += new System.EventHandler(this.numericUpDown_Enter);
         this.numericUpDown3.ValueChanged += new System.EventHandler(this.numericUpDown1_ValueChanged);
         // 
         // numericUpDown2
         // 
         this.numericUpDown2.Location = new System.Drawing.Point(83, 37);
         this.numericUpDown2.Maximum = new decimal(new int[] {
            32000,
            0,
            0,
            0});
         this.numericUpDown2.Minimum = new decimal(new int[] {
            32000,
            0,
            0,
            -2147483648});
         this.numericUpDown2.Name = "numericUpDown2";
         this.numericUpDown2.Size = new System.Drawing.Size(93, 20);
         this.numericUpDown2.TabIndex = 1;
         this.numericUpDown2.Enter += new System.EventHandler(this.numericUpDown_Enter);
         this.numericUpDown2.ValueChanged += new System.EventHandler(this.numericUpDown1_ValueChanged);
         // 
         // numericUpDown1
         // 
         this.numericUpDown1.Location = new System.Drawing.Point(83, 11);
         this.numericUpDown1.Maximum = new decimal(new int[] {
            32000,
            0,
            0,
            0});
         this.numericUpDown1.Minimum = new decimal(new int[] {
            32000,
            0,
            0,
            -2147483648});
         this.numericUpDown1.Name = "numericUpDown1";
         this.numericUpDown1.Size = new System.Drawing.Size(93, 20);
         this.numericUpDown1.TabIndex = 0;
         this.numericUpDown1.Enter += new System.EventHandler(this.numericUpDown_Enter);
         this.numericUpDown1.ValueChanged += new System.EventHandler(this.numericUpDown1_ValueChanged);
         // 
         // SpeedPage
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.groupBox2);
         this.Controls.Add(this.groupBox1);
         this.Name = "SpeedPage";
         this.Size = new System.Drawing.Size(954, 728);
         this.groupBox1.ResumeLayout(false);
         this.groupBox2.ResumeLayout(false);
         this.groupBox2.PerformLayout();
         ((System.ComponentModel.ISupportInitialize)(this.numericUpDown6)).EndInit();
         ((System.ComponentModel.ISupportInitialize)(this.numericUpDown5)).EndInit();
         ((System.ComponentModel.ISupportInitialize)(this.numericUpDown4)).EndInit();
         ((System.ComponentModel.ISupportInitialize)(this.numericUpDown3)).EndInit();
         ((System.ComponentModel.ISupportInitialize)(this.numericUpDown2)).EndInit();
         ((System.ComponentModel.ISupportInitialize)(this.numericUpDown1)).EndInit();
         this.ResumeLayout(false);

      }

      #endregion

      private EditorCore.VectorProgressionControl vectorProgressionControl1;
      private System.Windows.Forms.GroupBox groupBox1;
      private System.Windows.Forms.GroupBox groupBox2;
      private System.Windows.Forms.Label label3;
      private System.Windows.Forms.Label label2;
      private System.Windows.Forms.Label label1;
      private System.Windows.Forms.NumericUpDown numericUpDown3;
      private System.Windows.Forms.NumericUpDown numericUpDown2;
      private System.Windows.Forms.NumericUpDown numericUpDown1;
      private System.Windows.Forms.NumericUpDown numericUpDown6;
      private System.Windows.Forms.NumericUpDown numericUpDown5;
      private System.Windows.Forms.NumericUpDown numericUpDown4;
      private System.Windows.Forms.Label label6;
      private System.Windows.Forms.Label label5;
      private System.Windows.Forms.Label label4;
      private System.Windows.Forms.CheckBox checkBox1;
      private System.Windows.Forms.CheckBox checkBox3;
      private System.Windows.Forms.CheckBox checkBox2;
   }
}
