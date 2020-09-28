namespace PhoenixEditor
{
   partial class UnitPicker
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
         this.FilterComboBox = new System.Windows.Forms.ComboBox();
         this.PlayerIDComboBox = new System.Windows.Forms.ComboBox();
         this.label1 = new System.Windows.Forms.Label();
         this.tabControl1 = new System.Windows.Forms.TabControl();
         this.tabPage1 = new System.Windows.Forms.TabPage();
         this.UnitTreeView = new System.Windows.Forms.TreeView();
         this.tabPage2 = new System.Windows.Forms.TabPage();
         this.UnitListBox = new System.Windows.Forms.ListBox();
         this.tabPage3 = new System.Windows.Forms.TabPage();
         this.SquadTreeView = new System.Windows.Forms.TreeView();
         this.advancedBrushBox = new System.Windows.Forms.GroupBox();
         this.label4 = new System.Windows.Forms.Label();
         this.numericUpDown3 = new System.Windows.Forms.NumericUpDown();
         this.label3 = new System.Windows.Forms.Label();
         this.numericUpDown2 = new System.Windows.Forms.NumericUpDown();
         this.label2 = new System.Windows.Forms.Label();
         this.numericUpDown1 = new System.Windows.Forms.NumericUpDown();
         this.checkBox1 = new System.Windows.Forms.CheckBox();
         this.button1 = new System.Windows.Forms.Button();
         this.checkBox3 = new System.Windows.Forms.CheckBox();
         this.FillToMaskSettingsButton = new System.Windows.Forms.Button();
         this.groupBox1 = new System.Windows.Forms.GroupBox();
         this.selectEnvironmentComboBox = new System.Windows.Forms.ComboBox();
         this.LoadFillSetttingsButton = new System.Windows.Forms.Button();
         this.SaveFillSettingsButton = new System.Windows.Forms.Button();
         this.panel1 = new System.Windows.Forms.Panel();
         this.checkBox2 = new System.Windows.Forms.CheckBox();
         this.tabControl1.SuspendLayout();
         this.tabPage1.SuspendLayout();
         this.tabPage2.SuspendLayout();
         this.tabPage3.SuspendLayout();
         this.advancedBrushBox.SuspendLayout();
         ((System.ComponentModel.ISupportInitialize)(this.numericUpDown3)).BeginInit();
         ((System.ComponentModel.ISupportInitialize)(this.numericUpDown2)).BeginInit();
         ((System.ComponentModel.ISupportInitialize)(this.numericUpDown1)).BeginInit();
         this.groupBox1.SuspendLayout();
         this.panel1.SuspendLayout();
         this.SuspendLayout();
         // 
         // FilterComboBox
         // 
         this.FilterComboBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.FilterComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
         this.FilterComboBox.FormattingEnabled = true;
         this.FilterComboBox.Items.AddRange(new object[] {
            "All",
            "nt",
            "unsc",
            "cov",
            "inf",
            "veh",
            "air",
            "bldg"});
         this.FilterComboBox.Location = new System.Drawing.Point(3, 18);
         this.FilterComboBox.Name = "FilterComboBox";
         this.FilterComboBox.Size = new System.Drawing.Size(411, 21);
         this.FilterComboBox.TabIndex = 2;
         this.FilterComboBox.SelectedIndexChanged += new System.EventHandler(this.FilterComboBox_SelectedIndexChanged);
         this.FilterComboBox.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this.FilterComboBox_KeyPress);
         // 
         // PlayerIDComboBox
         // 
         this.PlayerIDComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
         this.PlayerIDComboBox.FormattingEnabled = true;
         this.PlayerIDComboBox.Items.AddRange(new object[] {
            "0",
            "1",
            "2",
            "3",
            "4",
            "5",
            "6",
            "7",
            "8"});
         this.PlayerIDComboBox.Location = new System.Drawing.Point(65, 14);
         this.PlayerIDComboBox.Name = "PlayerIDComboBox";
         this.PlayerIDComboBox.Size = new System.Drawing.Size(67, 21);
         this.PlayerIDComboBox.TabIndex = 3;
         this.PlayerIDComboBox.SelectedIndexChanged += new System.EventHandler(this.PlayerIDComboBox_SelectedIndexChanged);
         this.PlayerIDComboBox.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this.comboBox1_KeyPress);
         // 
         // label1
         // 
         this.label1.AutoSize = true;
         this.label1.Location = new System.Drawing.Point(9, 17);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(50, 13);
         this.label1.TabIndex = 4;
         this.label1.Text = "Player ID";
         this.label1.Click += new System.EventHandler(this.label1_Click);
         // 
         // tabControl1
         // 
         this.tabControl1.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.tabControl1.Controls.Add(this.tabPage1);
         this.tabControl1.Controls.Add(this.tabPage2);
         this.tabControl1.Controls.Add(this.tabPage3);
         this.tabControl1.Location = new System.Drawing.Point(3, 41);
         this.tabControl1.Name = "tabControl1";
         this.tabControl1.SelectedIndex = 0;
         this.tabControl1.Size = new System.Drawing.Size(428, 566);
         this.tabControl1.TabIndex = 5;
         this.tabControl1.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this.tabControl1_KeyPress);
         // 
         // tabPage1
         // 
         this.tabPage1.Controls.Add(this.UnitTreeView);
         this.tabPage1.Location = new System.Drawing.Point(4, 22);
         this.tabPage1.Name = "tabPage1";
         this.tabPage1.Padding = new System.Windows.Forms.Padding(3);
         this.tabPage1.Size = new System.Drawing.Size(420, 540);
         this.tabPage1.TabIndex = 0;
         this.tabPage1.Text = "Unit Tree";
         this.tabPage1.UseVisualStyleBackColor = true;
         // 
         // UnitTreeView
         // 
         this.UnitTreeView.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.UnitTreeView.Location = new System.Drawing.Point(3, 6);
         this.UnitTreeView.Name = "UnitTreeView";
         this.UnitTreeView.Size = new System.Drawing.Size(411, 528);
         this.UnitTreeView.TabIndex = 0;
         this.UnitTreeView.AfterSelect += new System.Windows.Forms.TreeViewEventHandler(this.UnitTreeView_AfterSelect);
         this.UnitTreeView.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this.UnitTreeView_KeyPress);
         this.UnitTreeView.NodeMouseClick += new System.Windows.Forms.TreeNodeMouseClickEventHandler(this.UnitTreeView_NodeMouseClick);
         this.UnitTreeView.KeyDown += new System.Windows.Forms.KeyEventHandler(this.UnitTreeView_KeyDown);
         // 
         // tabPage2
         // 
         this.tabPage2.Controls.Add(this.UnitListBox);
         this.tabPage2.Controls.Add(this.FilterComboBox);
         this.tabPage2.Location = new System.Drawing.Point(4, 22);
         this.tabPage2.Name = "tabPage2";
         this.tabPage2.Padding = new System.Windows.Forms.Padding(3);
         this.tabPage2.Size = new System.Drawing.Size(420, 540);
         this.tabPage2.TabIndex = 1;
         this.tabPage2.Text = "Unit List";
         this.tabPage2.UseVisualStyleBackColor = true;
         // 
         // UnitListBox
         // 
         this.UnitListBox.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.UnitListBox.FormattingEnabled = true;
         this.UnitListBox.Location = new System.Drawing.Point(3, 46);
         this.UnitListBox.Name = "UnitListBox";
         this.UnitListBox.Size = new System.Drawing.Size(411, 355);
         this.UnitListBox.TabIndex = 3;
         this.UnitListBox.SelectedIndexChanged += new System.EventHandler(this.UnitListBox_SelectedIndexChanged);
         this.UnitListBox.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this.UnitListBox_KeyPress);
         this.UnitListBox.KeyDown += new System.Windows.Forms.KeyEventHandler(this.UnitListBox_KeyDown);
         this.UnitListBox.Click += new System.EventHandler(this.UnitListBox_Click);
         // 
         // tabPage3
         // 
         this.tabPage3.Controls.Add(this.SquadTreeView);
         this.tabPage3.Location = new System.Drawing.Point(4, 22);
         this.tabPage3.Name = "tabPage3";
         this.tabPage3.Size = new System.Drawing.Size(420, 540);
         this.tabPage3.TabIndex = 2;
         this.tabPage3.Text = "Squad Tree";
         this.tabPage3.UseVisualStyleBackColor = true;
         // 
         // SquadTreeView
         // 
         this.SquadTreeView.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.SquadTreeView.Enabled = false;
         this.SquadTreeView.Location = new System.Drawing.Point(4, 3);
         this.SquadTreeView.Name = "SquadTreeView";
         this.SquadTreeView.Size = new System.Drawing.Size(410, 534);
         this.SquadTreeView.TabIndex = 0;
         this.SquadTreeView.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this.SquadTreeView_KeyPress);
         this.SquadTreeView.NodeMouseClick += new System.Windows.Forms.TreeNodeMouseClickEventHandler(this.SquadTreeView_NodeMouseClick);
         this.SquadTreeView.KeyDown += new System.Windows.Forms.KeyEventHandler(this.SquadTreeView_KeyDown);
         // 
         // advancedBrushBox
         // 
         this.advancedBrushBox.Controls.Add(this.label4);
         this.advancedBrushBox.Controls.Add(this.numericUpDown3);
         this.advancedBrushBox.Controls.Add(this.label3);
         this.advancedBrushBox.Controls.Add(this.numericUpDown2);
         this.advancedBrushBox.Controls.Add(this.label2);
         this.advancedBrushBox.Controls.Add(this.numericUpDown1);
         this.advancedBrushBox.Enabled = false;
         this.advancedBrushBox.Location = new System.Drawing.Point(3, 85);
         this.advancedBrushBox.Name = "advancedBrushBox";
         this.advancedBrushBox.Size = new System.Drawing.Size(236, 97);
         this.advancedBrushBox.TabIndex = 6;
         this.advancedBrushBox.TabStop = false;
         this.advancedBrushBox.Text = "Advanced Brush Settings";
         // 
         // label4
         // 
         this.label4.AutoSize = true;
         this.label4.Location = new System.Drawing.Point(6, 16);
         this.label4.Name = "label4";
         this.label4.Size = new System.Drawing.Size(40, 13);
         this.label4.TabIndex = 6;
         this.label4.Text = "Radius";
         // 
         // numericUpDown3
         // 
         this.numericUpDown3.DecimalPlaces = 2;
         this.numericUpDown3.Location = new System.Drawing.Point(161, 14);
         this.numericUpDown3.Minimum = new decimal(new int[] {
            6,
            0,
            0,
            0});
         this.numericUpDown3.Name = "numericUpDown3";
         this.numericUpDown3.Size = new System.Drawing.Size(61, 20);
         this.numericUpDown3.TabIndex = 5;
         this.numericUpDown3.Value = new decimal(new int[] {
            25,
            0,
            0,
            0});
         this.numericUpDown3.ValueChanged += new System.EventHandler(this.numericUpDown3_ValueChanged);
         // 
         // label3
         // 
         this.label3.AutoSize = true;
         this.label3.Location = new System.Drawing.Point(6, 68);
         this.label3.Name = "label3";
         this.label3.Size = new System.Drawing.Size(106, 13);
         this.label3.TabIndex = 4;
         this.label3.Text = "Max Rand Rot Angle";
         // 
         // numericUpDown2
         // 
         this.numericUpDown2.Location = new System.Drawing.Point(161, 68);
         this.numericUpDown2.Maximum = new decimal(new int[] {
            360,
            0,
            0,
            0});
         this.numericUpDown2.Name = "numericUpDown2";
         this.numericUpDown2.Size = new System.Drawing.Size(61, 20);
         this.numericUpDown2.TabIndex = 3;
         this.numericUpDown2.ValueChanged += new System.EventHandler(this.numericUpDown2_ValueChanged);
         // 
         // label2
         // 
         this.label2.AutoSize = true;
         this.label2.Location = new System.Drawing.Point(6, 41);
         this.label2.Name = "label2";
         this.label2.Size = new System.Drawing.Size(30, 13);
         this.label2.TabIndex = 2;
         this.label2.Text = "Fill %";
         // 
         // numericUpDown1
         // 
         this.numericUpDown1.DecimalPlaces = 2;
         this.numericUpDown1.Increment = new decimal(new int[] {
            1,
            0,
            0,
            131072});
         this.numericUpDown1.Location = new System.Drawing.Point(161, 39);
         this.numericUpDown1.Maximum = new decimal(new int[] {
            1,
            0,
            0,
            0});
         this.numericUpDown1.Name = "numericUpDown1";
         this.numericUpDown1.Size = new System.Drawing.Size(61, 20);
         this.numericUpDown1.TabIndex = 1;
         this.numericUpDown1.Value = new decimal(new int[] {
            1,
            0,
            0,
            0});
         this.numericUpDown1.ValueChanged += new System.EventHandler(this.numericUpDown1_ValueChanged);
         // 
         // checkBox1
         // 
         this.checkBox1.AutoSize = true;
         this.checkBox1.Location = new System.Drawing.Point(5, 62);
         this.checkBox1.Name = "checkBox1";
         this.checkBox1.Size = new System.Drawing.Size(141, 17);
         this.checkBox1.TabIndex = 0;
         this.checkBox1.Text = "Enable Advanced Brush";
         this.checkBox1.UseVisualStyleBackColor = true;
         this.checkBox1.CheckedChanged += new System.EventHandler(this.checkBox1_CheckedChanged);
         // 
         // button1
         // 
         this.button1.Location = new System.Drawing.Point(61, 19);
         this.button1.Name = "button1";
         this.button1.Size = new System.Drawing.Size(89, 33);
         this.button1.TabIndex = 7;
         this.button1.Text = "Fill To Mask";
         this.button1.UseVisualStyleBackColor = true;
         this.button1.Click += new System.EventHandler(this.button1_Click);
         // 
         // checkBox3
         // 
         this.checkBox3.AutoSize = true;
         this.checkBox3.Location = new System.Drawing.Point(5, 14);
         this.checkBox3.Name = "checkBox3";
         this.checkBox3.Size = new System.Drawing.Size(130, 17);
         this.checkBox3.TabIndex = 7;
         this.checkBox3.Text = "Allow Bounds Overlap";
         this.checkBox3.UseVisualStyleBackColor = true;
         this.checkBox3.CheckedChanged += new System.EventHandler(this.checkBox3_CheckedChanged);
         // 
         // FillToMaskSettingsButton
         // 
         this.FillToMaskSettingsButton.Location = new System.Drawing.Point(9, 57);
         this.FillToMaskSettingsButton.Name = "FillToMaskSettingsButton";
         this.FillToMaskSettingsButton.Size = new System.Drawing.Size(65, 23);
         this.FillToMaskSettingsButton.TabIndex = 8;
         this.FillToMaskSettingsButton.Text = "Settings...";
         this.FillToMaskSettingsButton.UseVisualStyleBackColor = true;
         this.FillToMaskSettingsButton.Click += new System.EventHandler(this.FillToMaskSettingsButton_Click);
         // 
         // groupBox1
         // 
         this.groupBox1.Controls.Add(this.selectEnvironmentComboBox);
         this.groupBox1.Controls.Add(this.LoadFillSetttingsButton);
         this.groupBox1.Controls.Add(this.SaveFillSettingsButton);
         this.groupBox1.Controls.Add(this.FillToMaskSettingsButton);
         this.groupBox1.Controls.Add(this.button1);
         this.groupBox1.Location = new System.Drawing.Point(3, 188);
         this.groupBox1.Name = "groupBox1";
         this.groupBox1.Size = new System.Drawing.Size(236, 97);
         this.groupBox1.TabIndex = 9;
         this.groupBox1.TabStop = false;
         this.groupBox1.Text = "Auto Environment (mask based)";
         // 
         // selectEnvironmentComboBox
         // 
         this.selectEnvironmentComboBox.FormattingEnabled = true;
         this.selectEnvironmentComboBox.Location = new System.Drawing.Point(9, 25);
         this.selectEnvironmentComboBox.Name = "selectEnvironmentComboBox";
         this.selectEnvironmentComboBox.Size = new System.Drawing.Size(37, 21);
         this.selectEnvironmentComboBox.TabIndex = 11;
         this.selectEnvironmentComboBox.Visible = false;
         this.selectEnvironmentComboBox.SelectedIndexChanged += new System.EventHandler(this.selectEnvironmentComboBox_SelectedIndexChanged);
         // 
         // LoadFillSetttingsButton
         // 
         this.LoadFillSetttingsButton.Location = new System.Drawing.Point(138, 57);
         this.LoadFillSetttingsButton.Name = "LoadFillSetttingsButton";
         this.LoadFillSetttingsButton.Size = new System.Drawing.Size(46, 23);
         this.LoadFillSetttingsButton.TabIndex = 10;
         this.LoadFillSetttingsButton.Text = "Load";
         this.LoadFillSetttingsButton.UseVisualStyleBackColor = true;
         this.LoadFillSetttingsButton.Click += new System.EventHandler(this.LoadFillSetttingsButton_Click);
         // 
         // SaveFillSettingsButton
         // 
         this.SaveFillSettingsButton.Location = new System.Drawing.Point(82, 57);
         this.SaveFillSettingsButton.Name = "SaveFillSettingsButton";
         this.SaveFillSettingsButton.Size = new System.Drawing.Size(50, 23);
         this.SaveFillSettingsButton.TabIndex = 9;
         this.SaveFillSettingsButton.Text = "Save";
         this.SaveFillSettingsButton.UseVisualStyleBackColor = true;
         this.SaveFillSettingsButton.Click += new System.EventHandler(this.SaveFillSettingsButton_Click);
         // 
         // panel1
         // 
         this.panel1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.panel1.AutoScroll = true;
         this.panel1.Controls.Add(this.checkBox2);
         this.panel1.Controls.Add(this.checkBox1);
         this.panel1.Controls.Add(this.groupBox1);
         this.panel1.Controls.Add(this.advancedBrushBox);
         this.panel1.Controls.Add(this.checkBox3);
         this.panel1.Location = new System.Drawing.Point(7, 613);
         this.panel1.Name = "panel1";
         this.panel1.Size = new System.Drawing.Size(420, 303);
         this.panel1.TabIndex = 10;
         // 
         // checkBox2
         // 
         this.checkBox2.AutoSize = true;
         this.checkBox2.Location = new System.Drawing.Point(5, 37);
         this.checkBox2.Name = "checkBox2";
         this.checkBox2.Size = new System.Drawing.Size(99, 17);
         this.checkBox2.TabIndex = 10;
         this.checkBox2.Text = "Paint Variations";
         this.checkBox2.UseVisualStyleBackColor = true;
         this.checkBox2.CheckedChanged += new System.EventHandler(this.checkBox2_CheckedChanged);
         // 
         // UnitPicker
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.panel1);
         this.Controls.Add(this.tabControl1);
         this.Controls.Add(this.label1);
         this.Controls.Add(this.PlayerIDComboBox);
         this.Key = "UnitPicker";
         this.Name = "UnitPicker";
         this.Size = new System.Drawing.Size(434, 919);
         this.Text = "UnitPicker";
         this.Load += new System.EventHandler(this.UnitPicker_Load);
         this.tabControl1.ResumeLayout(false);
         this.tabPage1.ResumeLayout(false);
         this.tabPage2.ResumeLayout(false);
         this.tabPage3.ResumeLayout(false);
         this.advancedBrushBox.ResumeLayout(false);
         this.advancedBrushBox.PerformLayout();
         ((System.ComponentModel.ISupportInitialize)(this.numericUpDown3)).EndInit();
         ((System.ComponentModel.ISupportInitialize)(this.numericUpDown2)).EndInit();
         ((System.ComponentModel.ISupportInitialize)(this.numericUpDown1)).EndInit();
         this.groupBox1.ResumeLayout(false);
         this.panel1.ResumeLayout(false);
         this.panel1.PerformLayout();
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private System.Windows.Forms.ComboBox FilterComboBox;
      private System.Windows.Forms.ComboBox PlayerIDComboBox;
      private System.Windows.Forms.Label label1;
      private System.Windows.Forms.TabControl tabControl1;
      private System.Windows.Forms.TabPage tabPage1;
      private System.Windows.Forms.TabPage tabPage2;
      private System.Windows.Forms.TreeView UnitTreeView;
      private System.Windows.Forms.ListBox UnitListBox;
      private System.Windows.Forms.GroupBox advancedBrushBox;
      private System.Windows.Forms.Label label3;
      private System.Windows.Forms.NumericUpDown numericUpDown2;
      private System.Windows.Forms.Label label2;
      private System.Windows.Forms.NumericUpDown numericUpDown1;
      private System.Windows.Forms.CheckBox checkBox1;
      private System.Windows.Forms.CheckBox checkBox3;
      private System.Windows.Forms.Label label4;
      private System.Windows.Forms.NumericUpDown numericUpDown3;
      private System.Windows.Forms.TabPage tabPage3;
      private System.Windows.Forms.TreeView SquadTreeView;
      private System.Windows.Forms.Button button1;
      private System.Windows.Forms.Button FillToMaskSettingsButton;
      private System.Windows.Forms.GroupBox groupBox1;
      private System.Windows.Forms.Button LoadFillSetttingsButton;
      private System.Windows.Forms.Button SaveFillSettingsButton;
      private System.Windows.Forms.Panel panel1;
      private System.Windows.Forms.ComboBox selectEnvironmentComboBox;
      private System.Windows.Forms.CheckBox checkBox2;
   }
}