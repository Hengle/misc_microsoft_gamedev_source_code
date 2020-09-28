namespace ParticleSystem
{
   partial class IntensityPage
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
         this.checkBox1 = new System.Windows.Forms.CheckBox();
         this.numericUpDown2 = new System.Windows.Forms.NumericUpDown();
         this.label2 = new System.Windows.Forms.Label();
         this.numericUpDown1 = new System.Windows.Forms.NumericUpDown();
         this.label1 = new System.Windows.Forms.Label();
         this.groupBox2 = new System.Windows.Forms.GroupBox();
         this.scalarProgressionControl1 = new EditorCore.ScalarProgressionControl();
         this.groupBox1.SuspendLayout();
         ((System.ComponentModel.ISupportInitialize)(this.numericUpDown2)).BeginInit();
         ((System.ComponentModel.ISupportInitialize)(this.numericUpDown1)).BeginInit();
         this.groupBox2.SuspendLayout();
         this.SuspendLayout();
         // 
         // groupBox1
         // 
         this.groupBox1.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.groupBox1.Controls.Add(this.checkBox1);
         this.groupBox1.Controls.Add(this.numericUpDown2);
         this.groupBox1.Controls.Add(this.label2);
         this.groupBox1.Controls.Add(this.numericUpDown1);
         this.groupBox1.Controls.Add(this.label1);
         this.groupBox1.Location = new System.Drawing.Point(0, 0);
         this.groupBox1.Name = "groupBox1";
         this.groupBox1.Size = new System.Drawing.Size(1049, 69);
         this.groupBox1.TabIndex = 0;
         this.groupBox1.TabStop = false;
         this.groupBox1.Text = "Intensity";
         // 
         // checkBox1
         // 
         this.checkBox1.AutoSize = true;
         this.checkBox1.Location = new System.Drawing.Point(30, 45);
         this.checkBox1.Name = "checkBox1";
         this.checkBox1.Size = new System.Drawing.Size(103, 17);
         this.checkBox1.TabIndex = 4;
         this.checkBox1.TabStop = false;
         this.checkBox1.Text = "Use Progression";
         this.checkBox1.UseVisualStyleBackColor = true;
         this.checkBox1.CheckedChanged += new System.EventHandler(this.winCheckBox1_CheckedChanged);
         // 
         // numericUpDown2
         // 
         this.numericUpDown2.Location = new System.Drawing.Point(233, 19);
         this.numericUpDown2.Name = "numericUpDown2";
         this.numericUpDown2.Size = new System.Drawing.Size(76, 20);
         this.numericUpDown2.TabIndex = 3;
         this.numericUpDown2.Enter += new System.EventHandler(this.numericUpDown_Enter);
         this.numericUpDown2.ValueChanged += new System.EventHandler(this.numericUpDown2_ValueChanged);
         // 
         // label2
         // 
         this.label2.AutoSize = true;
         this.label2.Location = new System.Drawing.Point(164, 21);
         this.label2.Name = "label2";
         this.label2.Size = new System.Drawing.Size(63, 13);
         this.label2.TabIndex = 2;
         this.label2.Text = "Variance(%)";
         // 
         // numericUpDown1
         // 
         this.numericUpDown1.Location = new System.Drawing.Point(87, 19);
         this.numericUpDown1.Maximum = new decimal(new int[] {
            32000,
            0,
            0,
            0});
         this.numericUpDown1.Name = "numericUpDown1";
         this.numericUpDown1.Size = new System.Drawing.Size(77, 20);
         this.numericUpDown1.TabIndex = 1;
         this.numericUpDown1.Enter += new System.EventHandler(this.numericUpDown_Enter);
         this.numericUpDown1.ValueChanged += new System.EventHandler(this.numericUpDown1_ValueChanged);
         // 
         // label1
         // 
         this.label1.AutoSize = true;
         this.label1.Location = new System.Drawing.Point(18, 21);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(63, 13);
         this.label1.TabIndex = 0;
         this.label1.Text = "Intensity (%)";
         // 
         // groupBox2
         // 
         this.groupBox2.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.groupBox2.Controls.Add(this.scalarProgressionControl1);
         this.groupBox2.Location = new System.Drawing.Point(0, 75);
         this.groupBox2.Name = "groupBox2";
         this.groupBox2.Size = new System.Drawing.Size(1049, 268);
         this.groupBox2.TabIndex = 1;
         this.groupBox2.TabStop = false;
         this.groupBox2.Text = "Intensity Progression";
         // 
         // scalarProgressionControl1
         // 
         this.scalarProgressionControl1.AxisMaxX = 100;
         this.scalarProgressionControl1.AxisMaxY = 100;
         this.scalarProgressionControl1.AxisMinX = 0;
         this.scalarProgressionControl1.AxisMinY = -100;
         this.scalarProgressionControl1.ChartEndColor = System.Drawing.Color.Yellow;
         this.scalarProgressionControl1.ChartStartColor = System.Drawing.Color.Red;
         this.scalarProgressionControl1.Dock = System.Windows.Forms.DockStyle.Fill;
         this.scalarProgressionControl1.Location = new System.Drawing.Point(3, 16);
         this.scalarProgressionControl1.Name = "scalarProgressionControl1";
         this.scalarProgressionControl1.ProgressionName = null;
         this.scalarProgressionControl1.Size = new System.Drawing.Size(1043, 249);
         this.scalarProgressionControl1.TabIndex = 0;
         // 
         // IntensityPage
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.groupBox2);
         this.Controls.Add(this.groupBox1);
         this.Name = "IntensityPage";
         this.Size = new System.Drawing.Size(1052, 691);
         this.groupBox1.ResumeLayout(false);
         this.groupBox1.PerformLayout();
         ((System.ComponentModel.ISupportInitialize)(this.numericUpDown2)).EndInit();
         ((System.ComponentModel.ISupportInitialize)(this.numericUpDown1)).EndInit();
         this.groupBox2.ResumeLayout(false);
         this.ResumeLayout(false);

      }

      #endregion

      private System.Windows.Forms.GroupBox groupBox1;
      private System.Windows.Forms.NumericUpDown numericUpDown1;
      private System.Windows.Forms.Label label1;
      private System.Windows.Forms.CheckBox checkBox1;
      private System.Windows.Forms.NumericUpDown numericUpDown2;
      private System.Windows.Forms.Label label2;
      private System.Windows.Forms.GroupBox groupBox2;
      private EditorCore.ScalarProgressionControl scalarProgressionControl1;
   }
}
