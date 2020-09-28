namespace EditorCore
{
    partial class ScalarProgressionControl
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
           this.components = new System.ComponentModel.Container();
           System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(ScalarProgressionControl));
           this.chartControl1 = new Xceed.Chart.ChartControl();
           this.listBox1 = new System.Windows.Forms.ListBox();
           this.button1 = new System.Windows.Forms.Button();
           this.button2 = new System.Windows.Forms.Button();
           this.button3 = new System.Windows.Forms.Button();
           this.button4 = new System.Windows.Forms.Button();
           this.label1 = new System.Windows.Forms.Label();
           this.label4 = new System.Windows.Forms.Label();
           this.checkBox1 = new System.Windows.Forms.CheckBox();
           this.label5 = new System.Windows.Forms.Label();
           this.numericUpDown1 = new System.Windows.Forms.NumericUpDown();
           this.numericUpDown2 = new System.Windows.Forms.NumericUpDown();
           this.numericUpDown3 = new System.Windows.Forms.NumericUpDown();
           this.numericUpDown5 = new System.Windows.Forms.NumericUpDown();
           this.label2 = new System.Windows.Forms.Label();
           this.button5 = new System.Windows.Forms.Button();
           this.groupBox1 = new System.Windows.Forms.GroupBox();
           this.checkBox2 = new System.Windows.Forms.CheckBox();
           this.button7 = new System.Windows.Forms.Button();
           this.button6 = new System.Windows.Forms.Button();
           this.toolTip1 = new System.Windows.Forms.ToolTip(this.components);
           ((System.ComponentModel.ISupportInitialize)(this.numericUpDown1)).BeginInit();
           ((System.ComponentModel.ISupportInitialize)(this.numericUpDown2)).BeginInit();
           ((System.ComponentModel.ISupportInitialize)(this.numericUpDown3)).BeginInit();
           ((System.ComponentModel.ISupportInitialize)(this.numericUpDown5)).BeginInit();
           this.groupBox1.SuspendLayout();
           this.SuspendLayout();
           // 
           // chartControl1
           // 
           this.chartControl1.AllowDrop = true;
           this.chartControl1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                       | System.Windows.Forms.AnchorStyles.Left)
                       | System.Windows.Forms.AnchorStyles.Right)));
           this.chartControl1.BackColor = System.Drawing.SystemColors.Control;
           this.chartControl1.Background = ((Xceed.Chart.Standard.Background)(resources.GetObject("chartControl1.Background")));
           this.chartControl1.Charts = ((Xceed.Chart.Core.ChartCollection)(resources.GetObject("chartControl1.Charts")));
           this.chartControl1.InteractivityOperations = ((Xceed.Chart.Core.InteractivityOperationsCollection)(resources.GetObject("chartControl1.InteractivityOperations")));
           this.chartControl1.Labels = ((Xceed.Chart.Standard.ChartLabelCollection)(resources.GetObject("chartControl1.Labels")));
           this.chartControl1.Legends = ((Xceed.Chart.Core.LegendCollection)(resources.GetObject("chartControl1.Legends")));
           this.chartControl1.Location = new System.Drawing.Point(409, 9);
           this.chartControl1.Name = "chartControl1";
           this.chartControl1.Settings = ((Xceed.Chart.Core.Settings)(resources.GetObject("chartControl1.Settings")));
           this.chartControl1.Size = new System.Drawing.Size(538, 192);
           this.chartControl1.TabIndex = 0;
           this.chartControl1.TabStop = false;
           this.chartControl1.Text = "chartControl1";
           this.chartControl1.Watermarks = ((Xceed.Chart.Standard.WatermarkCollection)(resources.GetObject("chartControl1.Watermarks")));
           this.chartControl1.MouseLeave += new System.EventHandler(this.chartControl1_MouseLeave);
           this.chartControl1.MouseDown += new System.Windows.Forms.MouseEventHandler(this.chartControl1_MouseDown);
           this.chartControl1.MouseMove += new System.Windows.Forms.MouseEventHandler(this.chartControl1_MouseMove);
           this.chartControl1.MouseDoubleClick += new System.Windows.Forms.MouseEventHandler(this.chartControl1_MouseDoubleClick);
           this.chartControl1.DragDrop += new System.Windows.Forms.DragEventHandler(this.chartControl1_DragDrop);
           this.chartControl1.DragEnter += new System.Windows.Forms.DragEventHandler(this.chartControl1_DragEnter);
           this.chartControl1.MouseEnter += new System.EventHandler(this.chartControl1_MouseEnter);
           this.chartControl1.MouseUp += new System.Windows.Forms.MouseEventHandler(this.chartControl1_MouseUp);
           this.chartControl1.DragLeave += new System.EventHandler(this.chartControl1_DragLeave);
           // 
           // listBox1
           // 
           this.listBox1.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                       | System.Windows.Forms.AnchorStyles.Left)));
           this.listBox1.FormatString = "N0";
           this.listBox1.FormattingEnabled = true;
           this.listBox1.Location = new System.Drawing.Point(6, 16);
           this.listBox1.Name = "listBox1";
           this.listBox1.Size = new System.Drawing.Size(167, 160);
           this.listBox1.TabIndex = 1;
           this.listBox1.TabStop = false;
           this.toolTip1.SetToolTip(this.listBox1, "List of all Progression Points");
           this.listBox1.EnabledChanged += new System.EventHandler(this.listBox1_EnabledChanged);
           this.listBox1.SelectedIndexChanged += new System.EventHandler(this.listBox1_SelectedIndexChanged);
           // 
           // button1
           // 
           this.button1.Font = new System.Drawing.Font("Arial Black", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
           this.button1.Image = ((System.Drawing.Image)(resources.GetObject("button1.Image")));
           this.button1.Location = new System.Drawing.Point(180, 19);
           this.button1.Name = "button1";
           this.button1.Size = new System.Drawing.Size(37, 36);
           this.button1.TabIndex = 2;
           this.button1.TabStop = false;
           this.toolTip1.SetToolTip(this.button1, "Add a new progression point");
           this.button1.UseVisualStyleBackColor = true;
           this.button1.Click += new System.EventHandler(this.button1_Click);
           // 
           // button2
           // 
           this.button2.Font = new System.Drawing.Font("Arial Black", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
           this.button2.Image = ((System.Drawing.Image)(resources.GetObject("button2.Image")));
           this.button2.Location = new System.Drawing.Point(180, 61);
           this.button2.Name = "button2";
           this.button2.Size = new System.Drawing.Size(37, 36);
           this.button2.TabIndex = 3;
           this.button2.TabStop = false;
           this.toolTip1.SetToolTip(this.button2, "Delete the selected Progression point");
           this.button2.UseVisualStyleBackColor = true;
           this.button2.Click += new System.EventHandler(this.button2_Click);
           // 
           // button3
           // 
           this.button3.Font = new System.Drawing.Font("Arial Black", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
           this.button3.Image = ((System.Drawing.Image)(resources.GetObject("button3.Image")));
           this.button3.Location = new System.Drawing.Point(223, 19);
           this.button3.Name = "button3";
           this.button3.Size = new System.Drawing.Size(37, 36);
           this.button3.TabIndex = 4;
           this.button3.TabStop = false;
           this.toolTip1.SetToolTip(this.button3, "Move the selected Progression Point up in the list");
           this.button3.UseVisualStyleBackColor = true;
           this.button3.Click += new System.EventHandler(this.button3_Click);
           // 
           // button4
           // 
           this.button4.Font = new System.Drawing.Font("Arial Black", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
           this.button4.Image = ((System.Drawing.Image)(resources.GetObject("button4.Image")));
           this.button4.Location = new System.Drawing.Point(223, 61);
           this.button4.Name = "button4";
           this.button4.Size = new System.Drawing.Size(37, 36);
           this.button4.TabIndex = 5;
           this.button4.TabStop = false;
           this.toolTip1.SetToolTip(this.button4, "Move the selected Progression Point down in the list");
           this.button4.UseVisualStyleBackColor = true;
           this.button4.Click += new System.EventHandler(this.button4_Click);
           // 
           // label1
           // 
           this.label1.AutoSize = true;
           this.label1.Location = new System.Drawing.Point(267, 29);
           this.label1.Name = "label1";
           this.label1.Size = new System.Drawing.Size(34, 13);
           this.label1.TabIndex = 7;
           this.label1.Text = "Value";
           // 
           // label4
           // 
           this.label4.AutoSize = true;
           this.label4.Location = new System.Drawing.Point(267, 79);
           this.label4.Name = "label4";
           this.label4.Size = new System.Drawing.Size(41, 13);
           this.label4.TabIndex = 13;
           this.label4.Text = "Life (%)";
           // 
           // checkBox1
           // 
           this.checkBox1.AutoSize = true;
           this.checkBox1.Location = new System.Drawing.Point(188, 172);
           this.checkBox1.Name = "checkBox1";
           this.checkBox1.Size = new System.Drawing.Size(50, 17);
           this.checkBox1.TabIndex = 14;
           this.checkBox1.TabStop = false;
           this.checkBox1.Text = "Loop";
           this.toolTip1.SetToolTip(this.checkBox1, "Selects whether the progression should loop");
           this.checkBox1.UseVisualStyleBackColor = true;
           this.checkBox1.CheckedChanged += new System.EventHandler(this.checkBox1_CheckedChanged);
           // 
           // label5
           // 
           this.label5.AutoSize = true;
           this.label5.Location = new System.Drawing.Point(267, 173);
           this.label5.Name = "label5";
           this.label5.Size = new System.Drawing.Size(38, 13);
           this.label5.TabIndex = 17;
           this.label5.Text = "Cycles";
           // 
           // numericUpDown1
           // 
           this.numericUpDown1.DecimalPlaces = 2;
           this.numericUpDown1.Location = new System.Drawing.Point(343, 27);
           this.numericUpDown1.Name = "numericUpDown1";
           this.numericUpDown1.Size = new System.Drawing.Size(60, 20);
           this.numericUpDown1.TabIndex = 19;
           this.toolTip1.SetToolTip(this.numericUpDown1, "Value of the selected Progression point");
           this.numericUpDown1.Enter += new System.EventHandler(this.numericUpDown_Enter);
           // 
           // numericUpDown2
           // 
           this.numericUpDown2.Location = new System.Drawing.Point(343, 77);
           this.numericUpDown2.Name = "numericUpDown2";
           this.numericUpDown2.Size = new System.Drawing.Size(60, 20);
           this.numericUpDown2.TabIndex = 21;
           this.toolTip1.SetToolTip(this.numericUpDown2, "Life (%) of when this progression point is active");
           this.numericUpDown2.Enter += new System.EventHandler(this.numericUpDown_Enter);
           // 
           // numericUpDown3
           // 
           this.numericUpDown3.DecimalPlaces = 1;
           this.numericUpDown3.Increment = new decimal(new int[] {
            1,
            0,
            0,
            65536});
           this.numericUpDown3.Location = new System.Drawing.Point(343, 171);
           this.numericUpDown3.Name = "numericUpDown3";
           this.numericUpDown3.Size = new System.Drawing.Size(60, 20);
           this.numericUpDown3.TabIndex = 22;
           this.toolTip1.SetToolTip(this.numericUpDown3, "The cycle time in seconds per loop cycle");
           this.numericUpDown3.Enter += new System.EventHandler(this.numericUpDown_Enter);
           this.numericUpDown3.ValueChanged += new System.EventHandler(this.numericUpDown3_ValueChanged);
           // 
           // numericUpDown5
           // 
           this.numericUpDown5.Location = new System.Drawing.Point(343, 52);
           this.numericUpDown5.Maximum = new decimal(new int[] {
            32000,
            0,
            0,
            0});
           this.numericUpDown5.Name = "numericUpDown5";
           this.numericUpDown5.Size = new System.Drawing.Size(60, 20);
           this.numericUpDown5.TabIndex = 20;
           this.toolTip1.SetToolTip(this.numericUpDown5, "Variance (%) of the selected Progression Point");
           this.numericUpDown5.Enter += new System.EventHandler(this.numericUpDown_Enter);
           // 
           // label2
           // 
           this.label2.AutoSize = true;
           this.label2.Location = new System.Drawing.Point(267, 54);
           this.label2.Name = "label2";
           this.label2.Size = new System.Drawing.Size(66, 13);
           this.label2.TabIndex = 24;
           this.label2.Text = "Variance (%)";
           // 
           // button5
           // 
           this.button5.Image = ((System.Drawing.Image)(resources.GetObject("button5.Image")));
           this.button5.Location = new System.Drawing.Point(352, 103);
           this.button5.Name = "button5";
           this.button5.Size = new System.Drawing.Size(38, 37);
           this.button5.TabIndex = 25;
           this.button5.TabStop = false;
           this.toolTip1.SetToolTip(this.button5, "Apply the changes to the selcted Progression Point.");
           this.button5.UseVisualStyleBackColor = true;
           this.button5.Click += new System.EventHandler(this.button5_Click);
           // 
           // groupBox1
           // 
           this.groupBox1.Controls.Add(this.checkBox2);
           this.groupBox1.Controls.Add(this.button7);
           this.groupBox1.Controls.Add(this.button6);
           this.groupBox1.Controls.Add(this.listBox1);
           this.groupBox1.Controls.Add(this.button4);
           this.groupBox1.Controls.Add(this.button5);
           this.groupBox1.Controls.Add(this.button3);
           this.groupBox1.Controls.Add(this.chartControl1);
           this.groupBox1.Controls.Add(this.button2);
           this.groupBox1.Controls.Add(this.label2);
           this.groupBox1.Controls.Add(this.button1);
           this.groupBox1.Controls.Add(this.checkBox1);
           this.groupBox1.Controls.Add(this.numericUpDown5);
           this.groupBox1.Controls.Add(this.label5);
           this.groupBox1.Controls.Add(this.numericUpDown2);
           this.groupBox1.Controls.Add(this.numericUpDown1);
           this.groupBox1.Controls.Add(this.label4);
           this.groupBox1.Controls.Add(this.numericUpDown3);
           this.groupBox1.Controls.Add(this.label1);
           this.groupBox1.Dock = System.Windows.Forms.DockStyle.Fill;
           this.groupBox1.Location = new System.Drawing.Point(0, 0);
           this.groupBox1.Name = "groupBox1";
           this.groupBox1.Size = new System.Drawing.Size(953, 204);
           this.groupBox1.TabIndex = 26;
           this.groupBox1.TabStop = false;
           // 
           // checkBox2
           // 
           this.checkBox2.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
           this.checkBox2.AutoSize = true;
           this.checkBox2.Location = new System.Drawing.Point(46, 181);
           this.checkBox2.Name = "checkBox2";
           this.checkBox2.Size = new System.Drawing.Size(83, 17);
           this.checkBox2.TabIndex = 28;
           this.checkBox2.Text = "View Labels";
           this.checkBox2.UseVisualStyleBackColor = true;
           this.checkBox2.CheckedChanged += new System.EventHandler(this.checkBox2_CheckedChanged);
           // 
           // button7
           // 
           this.button7.Image = ((System.Drawing.Image)(resources.GetObject("button7.Image")));
           this.button7.Location = new System.Drawing.Point(223, 103);
           this.button7.Name = "button7";
           this.button7.Size = new System.Drawing.Size(37, 36);
           this.button7.TabIndex = 27;
           this.button7.TabStop = false;
           this.toolTip1.SetToolTip(this.button7, "Save the Current Progression");
           this.button7.UseVisualStyleBackColor = true;
           this.button7.Click += new System.EventHandler(this.button7_Click);
           // 
           // button6
           // 
           this.button6.Image = ((System.Drawing.Image)(resources.GetObject("button6.Image")));
           this.button6.Location = new System.Drawing.Point(180, 103);
           this.button6.Name = "button6";
           this.button6.Size = new System.Drawing.Size(37, 36);
           this.button6.TabIndex = 26;
           this.button6.TabStop = false;
           this.toolTip1.SetToolTip(this.button6, "Load a Saved Progression");
           this.button6.UseVisualStyleBackColor = true;
           this.button6.Click += new System.EventHandler(this.button6_Click);
           // 
           // ScalarProgressionControl
           // 
           this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
           this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
           this.Controls.Add(this.groupBox1);
           this.Name = "ScalarProgressionControl";
           this.Size = new System.Drawing.Size(953, 204);
           this.Paint += new System.Windows.Forms.PaintEventHandler(this.ScalarProgressionControl_Paint);
           ((System.ComponentModel.ISupportInitialize)(this.numericUpDown1)).EndInit();
           ((System.ComponentModel.ISupportInitialize)(this.numericUpDown2)).EndInit();
           ((System.ComponentModel.ISupportInitialize)(this.numericUpDown3)).EndInit();
           ((System.ComponentModel.ISupportInitialize)(this.numericUpDown5)).EndInit();
           this.groupBox1.ResumeLayout(false);
           this.groupBox1.PerformLayout();
           this.ResumeLayout(false);

        }

        #endregion

        private Xceed.Chart.ChartControl chartControl1;
        private System.Windows.Forms.ListBox listBox1;
        private System.Windows.Forms.Button button1;
        private System.Windows.Forms.Button button2;
        private System.Windows.Forms.Button button3;
       private System.Windows.Forms.Button button4;
       private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label label4;
       private System.Windows.Forms.CheckBox checkBox1;
       private System.Windows.Forms.Label label5;
       private System.Windows.Forms.NumericUpDown numericUpDown1;
       private System.Windows.Forms.NumericUpDown numericUpDown2;
       private System.Windows.Forms.NumericUpDown numericUpDown3;
       private System.Windows.Forms.NumericUpDown numericUpDown5;
       private System.Windows.Forms.Label label2;
       private System.Windows.Forms.Button button5;
       private System.Windows.Forms.GroupBox groupBox1;
       private System.Windows.Forms.Button button7;
       private System.Windows.Forms.Button button6;
       private System.Windows.Forms.ToolTip toolTip1;
       private System.Windows.Forms.CheckBox checkBox2;

    }
}
