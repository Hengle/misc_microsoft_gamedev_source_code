namespace PhoenixEditor.ScenarioEditor
{
   partial class RoadPanel
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
         this.roadTextureListBox = new System.Windows.Forms.ListBox();
         this.groupBox1 = new System.Windows.Forms.GroupBox();
         this.checkBox1 = new System.Windows.Forms.CheckBox();
         this.checkBox2 = new System.Windows.Forms.CheckBox();
         this.tabControl1 = new System.Windows.Forms.TabControl();
         this.tabPage1 = new System.Windows.Forms.TabPage();
         this.label3 = new System.Windows.Forms.Label();
         this.comboBox2 = new System.Windows.Forms.ComboBox();
         this.tabPage2 = new System.Windows.Forms.TabPage();
         this.groupBox4 = new System.Windows.Forms.GroupBox();
         this.button3 = new System.Windows.Forms.Button();
         this.tesselationSlider = new EditorCore.NumericSliderControl();
         this.label2 = new System.Windows.Forms.Label();
         this.roadWidthSlider = new EditorCore.NumericSliderControl();
         this.label1 = new System.Windows.Forms.Label();
         this.comboBox1 = new System.Windows.Forms.ComboBox();
         this.label5 = new System.Windows.Forms.Label();
         this.groupBox3 = new System.Windows.Forms.GroupBox();
         this.button2 = new System.Windows.Forms.Button();
         this.button1 = new System.Windows.Forms.Button();
         this.label4 = new System.Windows.Forms.Label();
         this.roadNodeType = new System.Windows.Forms.ComboBox();
         this.radioButton3 = new System.Windows.Forms.RadioButton();
         this.radioButton5 = new System.Windows.Forms.RadioButton();
         this.radioButton6 = new System.Windows.Forms.RadioButton();
         this.groupBox1.SuspendLayout();
         this.tabControl1.SuspendLayout();
         this.tabPage1.SuspendLayout();
         this.tabPage2.SuspendLayout();
         this.groupBox4.SuspendLayout();
         this.groupBox3.SuspendLayout();
         this.SuspendLayout();
         // 
         // roadTextureListBox
         // 
         this.roadTextureListBox.FormattingEnabled = true;
         this.roadTextureListBox.Location = new System.Drawing.Point(6, 19);
         this.roadTextureListBox.Name = "roadTextureListBox";
         this.roadTextureListBox.Size = new System.Drawing.Size(211, 134);
         this.roadTextureListBox.TabIndex = 2;
         this.roadTextureListBox.SelectedIndexChanged += new System.EventHandler(this.roadTextureListBox_SelectedIndexChanged);
         // 
         // groupBox1
         // 
         this.groupBox1.Controls.Add(this.roadTextureListBox);
         this.groupBox1.Location = new System.Drawing.Point(4, 19);
         this.groupBox1.Name = "groupBox1";
         this.groupBox1.Size = new System.Drawing.Size(223, 181);
         this.groupBox1.TabIndex = 3;
         this.groupBox1.TabStop = false;
         this.groupBox1.Text = "Road Textures";
         // 
         // checkBox1
         // 
         this.checkBox1.AutoSize = true;
         this.checkBox1.Location = new System.Drawing.Point(6, 33);
         this.checkBox1.Name = "checkBox1";
         this.checkBox1.Size = new System.Drawing.Size(130, 17);
         this.checkBox1.TabIndex = 7;
         this.checkBox1.Text = "Continueous Segment";
         this.checkBox1.UseVisualStyleBackColor = true;
         this.checkBox1.CheckedChanged += new System.EventHandler(this.checkBox1_CheckedChanged);
         // 
         // checkBox2
         // 
         this.checkBox2.AutoSize = true;
         this.checkBox2.Location = new System.Drawing.Point(6, 53);
         this.checkBox2.Name = "checkBox2";
         this.checkBox2.Size = new System.Drawing.Size(146, 17);
         this.checkBox2.TabIndex = 8;
         this.checkBox2.Text = "Snap To Prime Directions";
         this.checkBox2.UseVisualStyleBackColor = true;
         this.checkBox2.CheckedChanged += new System.EventHandler(this.checkBox2_CheckedChanged);
         // 
         // tabControl1
         // 
         this.tabControl1.Controls.Add(this.tabPage1);
         this.tabControl1.Controls.Add(this.tabPage2);
         this.tabControl1.Location = new System.Drawing.Point(4, 206);
         this.tabControl1.Name = "tabControl1";
         this.tabControl1.SelectedIndex = 0;
         this.tabControl1.Size = new System.Drawing.Size(223, 409);
         this.tabControl1.TabIndex = 9;
         this.tabControl1.TabIndexChanged += new System.EventHandler(this.tabControl1_TabIndexChanged);
         this.tabControl1.SelectedIndexChanged += new System.EventHandler(this.tabControl1_SelectedIndexChanged);
         // 
         // tabPage1
         // 
         this.tabPage1.Controls.Add(this.label3);
         this.tabPage1.Controls.Add(this.comboBox2);
         this.tabPage1.Controls.Add(this.checkBox2);
         this.tabPage1.Controls.Add(this.checkBox1);
         this.tabPage1.Location = new System.Drawing.Point(4, 22);
         this.tabPage1.Name = "tabPage1";
         this.tabPage1.Padding = new System.Windows.Forms.Padding(3);
         this.tabPage1.Size = new System.Drawing.Size(215, 383);
         this.tabPage1.TabIndex = 0;
         this.tabPage1.Text = "Add Mode";
         this.tabPage1.UseVisualStyleBackColor = true;
         // 
         // label3
         // 
         this.label3.AutoSize = true;
         this.label3.Location = new System.Drawing.Point(9, 9);
         this.label3.Name = "label3";
         this.label3.Size = new System.Drawing.Size(103, 13);
         this.label3.TabIndex = 10;
         this.label3.Text = "Control Point Type : ";
         // 
         // comboBox2
         // 
         this.comboBox2.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
         this.comboBox2.FormattingEnabled = true;
         this.comboBox2.Items.AddRange(new object[] {
            "Angled",
            "Spline Curve"});
         this.comboBox2.Location = new System.Drawing.Point(118, 6);
         this.comboBox2.Name = "comboBox2";
         this.comboBox2.Size = new System.Drawing.Size(91, 21);
         this.comboBox2.TabIndex = 9;
         this.comboBox2.SelectedIndexChanged += new System.EventHandler(this.comboBox2_SelectedIndexChanged);
         // 
         // tabPage2
         // 
         this.tabPage2.Controls.Add(this.groupBox4);
         this.tabPage2.Controls.Add(this.groupBox3);
         this.tabPage2.Location = new System.Drawing.Point(4, 22);
         this.tabPage2.Name = "tabPage2";
         this.tabPage2.Padding = new System.Windows.Forms.Padding(3);
         this.tabPage2.Size = new System.Drawing.Size(215, 383);
         this.tabPage2.TabIndex = 1;
         this.tabPage2.Text = "Edit Mode";
         this.tabPage2.UseVisualStyleBackColor = true;
         // 
         // groupBox4
         // 
         this.groupBox4.Controls.Add(this.button3);
         this.groupBox4.Controls.Add(this.tesselationSlider);
         this.groupBox4.Controls.Add(this.label2);
         this.groupBox4.Controls.Add(this.roadWidthSlider);
         this.groupBox4.Controls.Add(this.label1);
         this.groupBox4.Controls.Add(this.comboBox1);
         this.groupBox4.Controls.Add(this.label5);
         this.groupBox4.Enabled = false;
         this.groupBox4.Location = new System.Drawing.Point(6, 145);
         this.groupBox4.Name = "groupBox4";
         this.groupBox4.Size = new System.Drawing.Size(200, 156);
         this.groupBox4.TabIndex = 1;
         this.groupBox4.TabStop = false;
         this.groupBox4.Text = "Selected Road Properties";
         // 
         // button3
         // 
         this.button3.Location = new System.Drawing.Point(6, 117);
         this.button3.Name = "button3";
         this.button3.Size = new System.Drawing.Size(175, 23);
         this.button3.TabIndex = 6;
         this.button3.Text = "Snap Road To Terrain";
         this.button3.UseVisualStyleBackColor = true;
         this.button3.Click += new System.EventHandler(this.button3_Click);
         // 
         // tesselationSlider
         // 
         this.tesselationSlider.BackColor = System.Drawing.SystemColors.ControlLight;
         this.tesselationSlider.Location = new System.Drawing.Point(73, 74);
         this.tesselationSlider.Name = "tesselationSlider";
         this.tesselationSlider.Size = new System.Drawing.Size(121, 22);
         this.tesselationSlider.TabIndex = 4;
         // 
         // label2
         // 
         this.label2.AutoSize = true;
         this.label2.Location = new System.Drawing.Point(6, 46);
         this.label2.Name = "label2";
         this.label2.Size = new System.Drawing.Size(44, 13);
         this.label2.TabIndex = 3;
         this.label2.Text = "Width : ";
         // 
         // roadWidthSlider
         // 
         this.roadWidthSlider.BackColor = System.Drawing.SystemColors.ControlLight;
         this.roadWidthSlider.Location = new System.Drawing.Point(73, 46);
         this.roadWidthSlider.Name = "roadWidthSlider";
         this.roadWidthSlider.Size = new System.Drawing.Size(121, 22);
         this.roadWidthSlider.TabIndex = 2;
         // 
         // label1
         // 
         this.label1.AutoSize = true;
         this.label1.Location = new System.Drawing.Point(6, 22);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(52, 13);
         this.label1.TabIndex = 1;
         this.label1.Text = "Texture : ";
         // 
         // comboBox1
         // 
         this.comboBox1.FormattingEnabled = true;
         this.comboBox1.Location = new System.Drawing.Point(73, 19);
         this.comboBox1.Name = "comboBox1";
         this.comboBox1.Size = new System.Drawing.Size(121, 21);
         this.comboBox1.TabIndex = 0;
         // 
         // label5
         // 
         this.label5.AutoSize = true;
         this.label5.Location = new System.Drawing.Point(6, 74);
         this.label5.Name = "label5";
         this.label5.Size = new System.Drawing.Size(70, 13);
         this.label5.TabIndex = 5;
         this.label5.Text = "Tesselation : ";
         // 
         // groupBox3
         // 
         this.groupBox3.Controls.Add(this.button2);
         this.groupBox3.Controls.Add(this.button1);
         this.groupBox3.Controls.Add(this.label4);
         this.groupBox3.Controls.Add(this.roadNodeType);
         this.groupBox3.Enabled = false;
         this.groupBox3.Location = new System.Drawing.Point(3, 15);
         this.groupBox3.Name = "groupBox3";
         this.groupBox3.Size = new System.Drawing.Size(206, 104);
         this.groupBox3.TabIndex = 0;
         this.groupBox3.TabStop = false;
         this.groupBox3.Text = "Selected Node Properties";
         // 
         // button2
         // 
         this.button2.Location = new System.Drawing.Point(109, 59);
         this.button2.Name = "button2";
         this.button2.Size = new System.Drawing.Size(75, 23);
         this.button2.TabIndex = 14;
         this.button2.Text = "Collapse";
         this.button2.UseVisualStyleBackColor = true;
         this.button2.Click += new System.EventHandler(this.button2_Click);
         // 
         // button1
         // 
         this.button1.Location = new System.Drawing.Point(6, 59);
         this.button1.Name = "button1";
         this.button1.Size = new System.Drawing.Size(75, 23);
         this.button1.TabIndex = 13;
         this.button1.Text = "Split";
         this.button1.UseVisualStyleBackColor = true;
         this.button1.Click += new System.EventHandler(this.button1_Click);
         // 
         // label4
         // 
         this.label4.AutoSize = true;
         this.label4.Location = new System.Drawing.Point(0, 22);
         this.label4.Name = "label4";
         this.label4.Size = new System.Drawing.Size(103, 13);
         this.label4.TabIndex = 12;
         this.label4.Text = "Control Point Type : ";
         // 
         // roadNodeType
         // 
         this.roadNodeType.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
         this.roadNodeType.FormattingEnabled = true;
         this.roadNodeType.Items.AddRange(new object[] {
            "Angled",
            "Spline Curve"});
         this.roadNodeType.Location = new System.Drawing.Point(109, 19);
         this.roadNodeType.Name = "roadNodeType";
         this.roadNodeType.Size = new System.Drawing.Size(91, 21);
         this.roadNodeType.TabIndex = 11;
         this.roadNodeType.SelectedIndexChanged += new System.EventHandler(this.comboBox3_SelectedIndexChanged);
         // 
         // radioButton3
         // 
         this.radioButton3.AutoSize = true;
         this.radioButton3.Location = new System.Drawing.Point(13, 65);
         this.radioButton3.Name = "radioButton3";
         this.radioButton3.Size = new System.Drawing.Size(81, 17);
         this.radioButton3.TabIndex = 3;
         this.radioButton3.TabStop = true;
         this.radioButton3.Text = "Spline Point";
         this.radioButton3.UseVisualStyleBackColor = true;
         // 
         // radioButton5
         // 
         this.radioButton5.AutoSize = true;
         this.radioButton5.Location = new System.Drawing.Point(13, 42);
         this.radioButton5.Name = "radioButton5";
         this.radioButton5.Size = new System.Drawing.Size(80, 17);
         this.radioButton5.TabIndex = 1;
         this.radioButton5.Text = "Tight Curve";
         this.radioButton5.UseVisualStyleBackColor = true;
         // 
         // radioButton6
         // 
         this.radioButton6.AutoSize = true;
         this.radioButton6.Checked = true;
         this.radioButton6.Location = new System.Drawing.Point(13, 19);
         this.radioButton6.Name = "radioButton6";
         this.radioButton6.Size = new System.Drawing.Size(58, 17);
         this.radioButton6.TabIndex = 0;
         this.radioButton6.TabStop = true;
         this.radioButton6.Text = "Angled";
         this.radioButton6.UseVisualStyleBackColor = true;
         // 
         // RoadPanel
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.tabControl1);
         this.Controls.Add(this.groupBox1);
         this.Key = "RoadPanel";
         this.Name = "RoadPanel";
         this.Size = new System.Drawing.Size(244, 685);
         this.Text = "RoadPanel";
         this.Load += new System.EventHandler(this.RoadPanel_Load);
         this.groupBox1.ResumeLayout(false);
         this.tabControl1.ResumeLayout(false);
         this.tabPage1.ResumeLayout(false);
         this.tabPage1.PerformLayout();
         this.tabPage2.ResumeLayout(false);
         this.groupBox4.ResumeLayout(false);
         this.groupBox4.PerformLayout();
         this.groupBox3.ResumeLayout(false);
         this.groupBox3.PerformLayout();
         this.ResumeLayout(false);

      }

      #endregion

      private System.Windows.Forms.ListBox roadTextureListBox;
      private System.Windows.Forms.GroupBox groupBox1;
      private System.Windows.Forms.CheckBox checkBox1;
      private System.Windows.Forms.CheckBox checkBox2;
      private System.Windows.Forms.TabControl tabControl1;
      private System.Windows.Forms.TabPage tabPage1;
      private System.Windows.Forms.TabPage tabPage2;
      private System.Windows.Forms.GroupBox groupBox4;
      private System.Windows.Forms.GroupBox groupBox3;
      private System.Windows.Forms.Label label2;
      private EditorCore.NumericSliderControl roadWidthSlider;
      private System.Windows.Forms.Label label1;
      private System.Windows.Forms.ComboBox comboBox1;
      private System.Windows.Forms.RadioButton radioButton3;
      private System.Windows.Forms.RadioButton radioButton5;
      private System.Windows.Forms.RadioButton radioButton6;
      private System.Windows.Forms.Label label3;
      private System.Windows.Forms.ComboBox comboBox2;
      private System.Windows.Forms.Label label4;
      private System.Windows.Forms.ComboBox roadNodeType;
      private System.Windows.Forms.Label label5;
      private EditorCore.NumericSliderControl tesselationSlider;
      private System.Windows.Forms.Button button2;
      private System.Windows.Forms.Button button1;
      private System.Windows.Forms.Button button3;
   }
}
