namespace ParticleSystem
{
   partial class OpacityPage
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
         this.numericUpDown2 = new System.Windows.Forms.NumericUpDown();
         this.numericUpDown1 = new System.Windows.Forms.NumericUpDown();
         this.winCheckBox1 = new Xceed.Editors.WinCheckBox();
         this.label2 = new System.Windows.Forms.Label();
         this.label1 = new System.Windows.Forms.Label();
         this.groupBox2 = new System.Windows.Forms.GroupBox();
         this.scalarProgressionControl2 = new EditorCore.ScalarProgressionControl();
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
         this.groupBox1.Controls.Add(this.numericUpDown2);
         this.groupBox1.Controls.Add(this.numericUpDown1);
         this.groupBox1.Controls.Add(this.winCheckBox1);
         this.groupBox1.Controls.Add(this.label2);
         this.groupBox1.Controls.Add(this.label1);
         this.groupBox1.Location = new System.Drawing.Point(0, 0);
         this.groupBox1.Name = "groupBox1";
         this.groupBox1.Size = new System.Drawing.Size(876, 67);
         this.groupBox1.TabIndex = 0;
         this.groupBox1.TabStop = false;
         this.groupBox1.Text = "Opacity";
         // 
         // numericUpDown2
         // 
         this.numericUpDown2.Location = new System.Drawing.Point(260, 19);
         this.numericUpDown2.Maximum = new decimal(new int[] {
            1600,
            0,
            0,
            0});
         this.numericUpDown2.Name = "numericUpDown2";
         this.numericUpDown2.Size = new System.Drawing.Size(69, 20);
         this.numericUpDown2.TabIndex = 6;
         this.numericUpDown2.Enter += new System.EventHandler(this.numericUpDown_Enter);
         this.numericUpDown2.ValueChanged += new System.EventHandler(this.numericUpDown2_ValueChanged);
         // 
         // numericUpDown1
         // 
         this.numericUpDown1.Location = new System.Drawing.Point(103, 19);
         this.numericUpDown1.Name = "numericUpDown1";
         this.numericUpDown1.Size = new System.Drawing.Size(69, 20);
         this.numericUpDown1.TabIndex = 5;
         this.numericUpDown1.Enter += new System.EventHandler(this.numericUpDown_Enter);
         this.numericUpDown1.ValueChanged += new System.EventHandler(this.numericUpDown1_ValueChanged);
         // 
         // winCheckBox1
         // 
         this.winCheckBox1.Checked = false;
         this.winCheckBox1.CheckState = System.Windows.Forms.CheckState.Unchecked;
         this.winCheckBox1.Location = new System.Drawing.Point(34, 42);
         this.winCheckBox1.Name = "winCheckBox1";
         this.winCheckBox1.Size = new System.Drawing.Size(104, 24);
         this.winCheckBox1.TabIndex = 4;
         this.winCheckBox1.TabStop = false;
         this.winCheckBox1.Text = "Use Progression";
         this.winCheckBox1.CheckedChanged += new System.EventHandler(this.winCheckBox1_CheckedChanged);
         // 
         // label2
         // 
         this.label2.AutoSize = true;
         this.label2.Location = new System.Drawing.Point(188, 22);
         this.label2.Name = "label2";
         this.label2.Size = new System.Drawing.Size(66, 13);
         this.label2.TabIndex = 2;
         this.label2.Text = "Variance (%)";
         // 
         // label1
         // 
         this.label1.AutoSize = true;
         this.label1.Location = new System.Drawing.Point(16, 22);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(60, 13);
         this.label1.TabIndex = 0;
         this.label1.Text = "Opacity (%)";
         // 
         // groupBox2
         // 
         this.groupBox2.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.groupBox2.Controls.Add(this.scalarProgressionControl2);
         this.groupBox2.Location = new System.Drawing.Point(0, 72);
         this.groupBox2.Name = "groupBox2";
         this.groupBox2.Size = new System.Drawing.Size(876, 291);
         this.groupBox2.TabIndex = 2;
         this.groupBox2.TabStop = false;
         this.groupBox2.Text = "Opacity Progression";
         // 
         // scalarProgressionControl2
         // 
         this.scalarProgressionControl2.AxisMaxX = 100;
         this.scalarProgressionControl2.AxisMaxY = 100;
         this.scalarProgressionControl2.AxisMinX = 0;
         this.scalarProgressionControl2.AxisMinY = 0;
         this.scalarProgressionControl2.ChartEndColor = System.Drawing.Color.Cyan;
         this.scalarProgressionControl2.ChartStartColor = System.Drawing.Color.Blue;
         this.scalarProgressionControl2.Dock = System.Windows.Forms.DockStyle.Fill;
         this.scalarProgressionControl2.Location = new System.Drawing.Point(3, 16);
         this.scalarProgressionControl2.Name = "scalarProgressionControl2";
         this.scalarProgressionControl2.ProgressionName = null;
         this.scalarProgressionControl2.Size = new System.Drawing.Size(870, 272);
         this.scalarProgressionControl2.TabIndex = 0;
         // 
         // OpacityPage
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.groupBox2);
         this.Controls.Add(this.groupBox1);
         this.Name = "OpacityPage";
         this.Size = new System.Drawing.Size(879, 366);
         this.groupBox1.ResumeLayout(false);
         this.groupBox1.PerformLayout();
         ((System.ComponentModel.ISupportInitialize)(this.numericUpDown2)).EndInit();
         ((System.ComponentModel.ISupportInitialize)(this.numericUpDown1)).EndInit();
         this.groupBox2.ResumeLayout(false);
         this.ResumeLayout(false);

      }

      #endregion

      private System.Windows.Forms.GroupBox groupBox1;
      private System.Windows.Forms.Label label1;
      private Xceed.Editors.WinCheckBox winCheckBox1;
      private System.Windows.Forms.Label label2;
      private System.Windows.Forms.GroupBox groupBox2;      
      private System.Windows.Forms.NumericUpDown numericUpDown2;
      private System.Windows.Forms.NumericUpDown numericUpDown1;
      private EditorCore.ScalarProgressionControl scalarProgressionControl2;
   }
}
