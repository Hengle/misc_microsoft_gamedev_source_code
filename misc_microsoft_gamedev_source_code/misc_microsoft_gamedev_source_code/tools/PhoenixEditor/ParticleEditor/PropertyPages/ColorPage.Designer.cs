namespace ParticleSystem
{
   partial class ColorPage
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
         System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(ColorPage));
         this.groupBox1 = new System.Windows.Forms.GroupBox();
         this.groupBox6 = new System.Windows.Forms.GroupBox();
         this.label3 = new System.Windows.Forms.Label();
         this.floatSliderEdit2 = new VisualEditor.Controls.FloatSliderEdit();
         this.checkBox2 = new System.Windows.Forms.CheckBox();
         this.groupBox5 = new System.Windows.Forms.GroupBox();
         this.label2 = new System.Windows.Forms.Label();
         this.floatSliderEdit1 = new VisualEditor.Controls.FloatSliderEdit();
         this.checkBox1 = new System.Windows.Forms.CheckBox();
         this.groupBox4 = new System.Windows.Forms.GroupBox();
         this.button7 = new System.Windows.Forms.Button();
         this.button6 = new System.Windows.Forms.Button();
         this.button5 = new System.Windows.Forms.Button();
         this.button4 = new System.Windows.Forms.Button();
         this.groupBox3 = new System.Windows.Forms.GroupBox();
         this.button3 = new System.Windows.Forms.Button();
         this.button2 = new System.Windows.Forms.Button();
         this.listBox1 = new System.Windows.Forms.ListBox();
         this.groupBox2 = new System.Windows.Forms.GroupBox();
         this.label1 = new System.Windows.Forms.Label();
         this.button1 = new System.Windows.Forms.Button();
         this.smartOptionList1 = new Xceed.SmartUI.Controls.OptionList.SmartOptionList(this.components);
         this.radioButtonNode1 = new Xceed.SmartUI.Controls.OptionList.RadioButtonNode("Single Color");
         this.radioButtonNode2 = new Xceed.SmartUI.Controls.OptionList.RadioButtonNode("Progression");
         this.radioButtonNode3 = new Xceed.SmartUI.Controls.OptionList.RadioButtonNode("Pallette");
         this.colorDialog1 = new System.Windows.Forms.ColorDialog();
         this.colorProgressionControl1 = new EditorCore.ColorProgressionControl();
         this.groupBox1.SuspendLayout();
         this.groupBox6.SuspendLayout();
         this.groupBox5.SuspendLayout();
         this.groupBox4.SuspendLayout();
         this.groupBox3.SuspendLayout();
         this.groupBox2.SuspendLayout();
         ((System.ComponentModel.ISupportInitialize)(this.smartOptionList1)).BeginInit();
         this.SuspendLayout();
         // 
         // groupBox1
         // 
         this.groupBox1.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.groupBox1.Controls.Add(this.groupBox6);
         this.groupBox1.Controls.Add(this.groupBox5);
         this.groupBox1.Controls.Add(this.groupBox4);
         this.groupBox1.Controls.Add(this.groupBox3);
         this.groupBox1.Controls.Add(this.groupBox2);
         this.groupBox1.Controls.Add(this.smartOptionList1);
         this.groupBox1.Location = new System.Drawing.Point(3, 3);
         this.groupBox1.Name = "groupBox1";
         this.groupBox1.Size = new System.Drawing.Size(1345, 165);
         this.groupBox1.TabIndex = 1;
         this.groupBox1.TabStop = false;
         this.groupBox1.Text = "Color Properties";
         // 
         // groupBox6
         // 
         this.groupBox6.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
         this.groupBox6.Controls.Add(this.label3);
         this.groupBox6.Controls.Add(this.floatSliderEdit2);
         this.groupBox6.Controls.Add(this.checkBox2);
         this.groupBox6.Location = new System.Drawing.Point(467, 11);
         this.groupBox6.Name = "groupBox6";
         this.groupBox6.Size = new System.Drawing.Size(221, 147);
         this.groupBox6.TabIndex = 6;
         this.groupBox6.TabStop = false;
         this.groupBox6.Text = "Lightset Sun Color";
         // 
         // label3
         // 
         this.label3.AutoSize = true;
         this.label3.Location = new System.Drawing.Point(24, 70);
         this.label3.Name = "label3";
         this.label3.Size = new System.Drawing.Size(63, 13);
         this.label3.TabIndex = 2;
         this.label3.Text = "Intensity (%)";
         // 
         // floatSliderEdit2
         // 
         this.floatSliderEdit2.Location = new System.Drawing.Point(22, 86);
         this.floatSliderEdit2.Margin = new System.Windows.Forms.Padding(3, 3, 3, 0);
         this.floatSliderEdit2.MaxValue = 100F;
         this.floatSliderEdit2.MinValue = 0F;
         this.floatSliderEdit2.Name = "floatSliderEdit2";
         this.floatSliderEdit2.NumDecimals = 2;
         this.floatSliderEdit2.Size = new System.Drawing.Size(174, 40);
         this.floatSliderEdit2.TabIndex = 1;
         this.floatSliderEdit2.Value = 100F;
         this.floatSliderEdit2.ValueChanged += new VisualEditor.Controls.FloatSliderEdit.ValueChangedDelegate(this.floatSliderEdit2_ValueChanged);
         // 
         // checkBox2
         // 
         this.checkBox2.AutoSize = true;
         this.checkBox2.Location = new System.Drawing.Point(20, 21);
         this.checkBox2.Name = "checkBox2";
         this.checkBox2.Size = new System.Drawing.Size(141, 17);
         this.checkBox2.TabIndex = 0;
         this.checkBox2.Text = "Apply Lightset Sun Color";
         this.checkBox2.UseVisualStyleBackColor = true;
         this.checkBox2.CheckedChanged += new System.EventHandler(this.checkBox2_CheckedChanged);
         // 
         // groupBox5
         // 
         this.groupBox5.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
         this.groupBox5.Controls.Add(this.label2);
         this.groupBox5.Controls.Add(this.floatSliderEdit1);
         this.groupBox5.Controls.Add(this.checkBox1);
         this.groupBox5.Location = new System.Drawing.Point(697, 11);
         this.groupBox5.Name = "groupBox5";
         this.groupBox5.Size = new System.Drawing.Size(214, 148);
         this.groupBox5.TabIndex = 5;
         this.groupBox5.TabStop = false;
         this.groupBox5.Text = "Player Color";
         // 
         // label2
         // 
         this.label2.AutoSize = true;
         this.label2.Location = new System.Drawing.Point(14, 70);
         this.label2.Name = "label2";
         this.label2.Size = new System.Drawing.Size(66, 13);
         this.label2.TabIndex = 2;
         this.label2.Text = " Intensity (%)";
         // 
         // floatSliderEdit1
         // 
         this.floatSliderEdit1.Location = new System.Drawing.Point(17, 86);
         this.floatSliderEdit1.Margin = new System.Windows.Forms.Padding(3, 3, 3, 0);
         this.floatSliderEdit1.MaxValue = 100F;
         this.floatSliderEdit1.MinValue = 0F;
         this.floatSliderEdit1.Name = "floatSliderEdit1";
         this.floatSliderEdit1.NumDecimals = 2;
         this.floatSliderEdit1.Size = new System.Drawing.Size(174, 40);
         this.floatSliderEdit1.TabIndex = 1;
         this.floatSliderEdit1.Value = 100F;
         this.floatSliderEdit1.ValueChanged += new VisualEditor.Controls.FloatSliderEdit.ValueChangedDelegate(this.floatSliderEdit1_ValueChanged);
         // 
         // checkBox1
         // 
         this.checkBox1.AutoSize = true;
         this.checkBox1.Location = new System.Drawing.Point(17, 21);
         this.checkBox1.Name = "checkBox1";
         this.checkBox1.Size = new System.Drawing.Size(111, 17);
         this.checkBox1.TabIndex = 0;
         this.checkBox1.Text = "Apply Player Color";
         this.checkBox1.UseVisualStyleBackColor = true;
         this.checkBox1.CheckedChanged += new System.EventHandler(this.checkBox1_CheckedChanged);
         // 
         // groupBox4
         // 
         this.groupBox4.Controls.Add(this.button7);
         this.groupBox4.Controls.Add(this.button6);
         this.groupBox4.Controls.Add(this.button5);
         this.groupBox4.Controls.Add(this.button4);
         this.groupBox4.Location = new System.Drawing.Point(320, 11);
         this.groupBox4.Name = "groupBox4";
         this.groupBox4.Size = new System.Drawing.Size(119, 92);
         this.groupBox4.TabIndex = 4;
         this.groupBox4.TabStop = false;
         this.groupBox4.Text = "Fake Vertex Lighting";
         // 
         // button7
         // 
         this.button7.BackColor = System.Drawing.Color.White;
         this.button7.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
         this.button7.Location = new System.Drawing.Point(61, 53);
         this.button7.Name = "button7";
         this.button7.Size = new System.Drawing.Size(25, 25);
         this.button7.TabIndex = 3;
         this.button7.UseVisualStyleBackColor = false;
         this.button7.Click += new System.EventHandler(this.buttonVertexColor4_Click);
         // 
         // button6
         // 
         this.button6.BackColor = System.Drawing.Color.White;
         this.button6.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
         this.button6.Location = new System.Drawing.Point(30, 53);
         this.button6.Name = "button6";
         this.button6.Size = new System.Drawing.Size(25, 25);
         this.button6.TabIndex = 2;
         this.button6.UseVisualStyleBackColor = false;
         this.button6.Click += new System.EventHandler(this.buttonVertexColor3_Click);
         // 
         // button5
         // 
         this.button5.BackColor = System.Drawing.Color.White;
         this.button5.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
         this.button5.Location = new System.Drawing.Point(61, 22);
         this.button5.Name = "button5";
         this.button5.Size = new System.Drawing.Size(25, 25);
         this.button5.TabIndex = 1;
         this.button5.UseVisualStyleBackColor = false;
         this.button5.Click += new System.EventHandler(this.buttonVertexColor2_Click);
         // 
         // button4
         // 
         this.button4.BackColor = System.Drawing.Color.White;
         this.button4.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
         this.button4.Location = new System.Drawing.Point(30, 22);
         this.button4.Name = "button4";
         this.button4.Size = new System.Drawing.Size(25, 25);
         this.button4.TabIndex = 0;
         this.button4.UseVisualStyleBackColor = false;
         this.button4.Click += new System.EventHandler(this.buttonVertexColor1_Click);
         // 
         // groupBox3
         // 
         this.groupBox3.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
         this.groupBox3.Controls.Add(this.button3);
         this.groupBox3.Controls.Add(this.button2);
         this.groupBox3.Controls.Add(this.listBox1);
         this.groupBox3.Location = new System.Drawing.Point(917, 11);
         this.groupBox3.Name = "groupBox3";
         this.groupBox3.Size = new System.Drawing.Size(422, 148);
         this.groupBox3.TabIndex = 3;
         this.groupBox3.TabStop = false;
         this.groupBox3.Text = "Pallette Colors";
         // 
         // button3
         // 
         this.button3.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
         this.button3.Image = ((System.Drawing.Image)(resources.GetObject("button3.Image")));
         this.button3.Location = new System.Drawing.Point(377, 21);
         this.button3.Name = "button3";
         this.button3.Size = new System.Drawing.Size(39, 39);
         this.button3.TabIndex = 2;
         this.button3.UseVisualStyleBackColor = true;
         this.button3.Click += new System.EventHandler(this.button3_Click);
         // 
         // button2
         // 
         this.button2.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
         this.button2.Image = ((System.Drawing.Image)(resources.GetObject("button2.Image")));
         this.button2.Location = new System.Drawing.Point(332, 21);
         this.button2.Name = "button2";
         this.button2.Size = new System.Drawing.Size(39, 39);
         this.button2.TabIndex = 1;
         this.button2.UseVisualStyleBackColor = true;
         this.button2.Click += new System.EventHandler(this.button2_Click);
         // 
         // listBox1
         // 
         this.listBox1.DrawMode = System.Windows.Forms.DrawMode.OwnerDrawVariable;
         this.listBox1.FormattingEnabled = true;
         this.listBox1.Location = new System.Drawing.Point(6, 21);
         this.listBox1.Name = "listBox1";
         this.listBox1.Size = new System.Drawing.Size(316, 108);
         this.listBox1.TabIndex = 0;
         this.listBox1.TabStop = false;
         this.listBox1.MouseDoubleClick += new System.Windows.Forms.MouseEventHandler(this.listBox1_MouseDoubleClick);
         this.listBox1.EnabledChanged += new System.EventHandler(this.listBox1_EnabledChanged);
         this.listBox1.DrawItem += new System.Windows.Forms.DrawItemEventHandler(this.listBox1_DrawItem);
         this.listBox1.MeasureItem += new System.Windows.Forms.MeasureItemEventHandler(this.listBox1_MeasureItem);
         // 
         // groupBox2
         // 
         this.groupBox2.Controls.Add(this.label1);
         this.groupBox2.Controls.Add(this.button1);
         this.groupBox2.Location = new System.Drawing.Point(6, 97);
         this.groupBox2.Name = "groupBox2";
         this.groupBox2.Size = new System.Drawing.Size(260, 62);
         this.groupBox2.TabIndex = 2;
         this.groupBox2.TabStop = false;
         this.groupBox2.Text = "Single Color";
         // 
         // label1
         // 
         this.label1.AutoSize = true;
         this.label1.Location = new System.Drawing.Point(109, 30);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(35, 13);
         this.label1.TabIndex = 1;
         this.label1.Text = "White";
         // 
         // button1
         // 
         this.button1.BackColor = System.Drawing.Color.White;
         this.button1.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
         this.button1.Location = new System.Drawing.Point(20, 25);
         this.button1.Name = "button1";
         this.button1.Size = new System.Drawing.Size(74, 21);
         this.button1.TabIndex = 0;
         this.button1.TabStop = false;
         this.button1.UseVisualStyleBackColor = false;
         this.button1.Click += new System.EventHandler(this.button1_Click);
         // 
         // smartOptionList1
         // 
         this.smartOptionList1.Items.AddRange(new Xceed.SmartUI.SmartItem[] {
            this.radioButtonNode1,
            this.radioButtonNode2,
            this.radioButtonNode3});
         this.smartOptionList1.Location = new System.Drawing.Point(18, 28);
         this.smartOptionList1.Name = "smartOptionList1";
         this.smartOptionList1.Size = new System.Drawing.Size(144, 56);
         this.smartOptionList1.TabIndex = 1;
         this.smartOptionList1.TabStop = false;
         this.smartOptionList1.Text = "smartOptionList1";
         // 
         // radioButtonNode1
         // 
         this.radioButtonNode1.Checked = true;
         this.radioButtonNode1.Grouped = true;
         this.radioButtonNode1.Text = "Single Color";
         // 
         // radioButtonNode2
         // 
         this.radioButtonNode2.Grouped = true;
         this.radioButtonNode2.Text = "Progression";
         // 
         // radioButtonNode3
         // 
         this.radioButtonNode3.Grouped = true;
         this.radioButtonNode3.Text = "Pallette";
         // 
         // colorProgressionControl1
         // 
         this.colorProgressionControl1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.colorProgressionControl1.Location = new System.Drawing.Point(0, 174);
         this.colorProgressionControl1.Name = "colorProgressionControl1";
         this.colorProgressionControl1.Size = new System.Drawing.Size(1345, 577);
         this.colorProgressionControl1.TabIndex = 0;
         // 
         // ColorPage
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.groupBox1);
         this.Controls.Add(this.colorProgressionControl1);
         this.Name = "ColorPage";
         this.Size = new System.Drawing.Size(1348, 751);
         this.groupBox1.ResumeLayout(false);
         this.groupBox6.ResumeLayout(false);
         this.groupBox6.PerformLayout();
         this.groupBox5.ResumeLayout(false);
         this.groupBox5.PerformLayout();
         this.groupBox4.ResumeLayout(false);
         this.groupBox3.ResumeLayout(false);
         this.groupBox2.ResumeLayout(false);
         this.groupBox2.PerformLayout();
         ((System.ComponentModel.ISupportInitialize)(this.smartOptionList1)).EndInit();
         this.ResumeLayout(false);

      }

      #endregion

      private EditorCore.ColorProgressionControl colorProgressionControl1;
      private System.Windows.Forms.GroupBox groupBox1;
      private Xceed.SmartUI.Controls.OptionList.SmartOptionList smartOptionList1;
      private Xceed.SmartUI.Controls.OptionList.RadioButtonNode radioButtonNode1;
      private Xceed.SmartUI.Controls.OptionList.RadioButtonNode radioButtonNode2;
      private Xceed.SmartUI.Controls.OptionList.RadioButtonNode radioButtonNode3;
      private System.Windows.Forms.Button button1;
      private System.Windows.Forms.GroupBox groupBox2;
      private System.Windows.Forms.GroupBox groupBox3;
      private System.Windows.Forms.Button button3;
      private System.Windows.Forms.Button button2;
      private System.Windows.Forms.ListBox listBox1;
      private System.Windows.Forms.ColorDialog colorDialog1;
      private System.Windows.Forms.Label label1;
      private System.Windows.Forms.GroupBox groupBox4;
      private System.Windows.Forms.Button button7;
      private System.Windows.Forms.Button button6;
      private System.Windows.Forms.Button button5;
      private System.Windows.Forms.Button button4;
      private System.Windows.Forms.GroupBox groupBox5;
      private VisualEditor.Controls.FloatSliderEdit floatSliderEdit1;
      private System.Windows.Forms.CheckBox checkBox1;
      private System.Windows.Forms.Label label2;
      private System.Windows.Forms.GroupBox groupBox6;
      private System.Windows.Forms.Label label3;
      private VisualEditor.Controls.FloatSliderEdit floatSliderEdit2;
      private System.Windows.Forms.CheckBox checkBox2;

   }
}
