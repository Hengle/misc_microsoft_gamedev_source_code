namespace PhoenixEditor.ScenarioEditor
{
   partial class ScenarioSettings
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
         this.label1 = new System.Windows.Forms.Label();
         this.label2 = new System.Windows.Forms.Label();
         this.label3 = new System.Windows.Forms.Label();
         this.label4 = new System.Windows.Forms.Label();
         this.groupBox1 = new System.Windows.Forms.GroupBox();
         this.label5 = new System.Windows.Forms.Label();
         this.minZBounds = new EditorCore.NumericSliderControl();
         this.maxXBounds = new EditorCore.NumericSliderControl();
         this.minXBounds = new EditorCore.NumericSliderControl();
         this.maxZBounds = new EditorCore.NumericSliderControl();
         this.groupBox2 = new System.Windows.Forms.GroupBox();
         this.button1 = new System.Windows.Forms.Button();
         this.textBox1 = new System.Windows.Forms.TextBox();
         this.label6 = new System.Windows.Forms.Label();
         this.groupBox3 = new System.Windows.Forms.GroupBox();
         this.cinematicDeleteButton = new System.Windows.Forms.Button();
         this.cinematicAddButton = new System.Windows.Forms.Button();
         this.cinematicListBox = new System.Windows.Forms.ListBox();
         this.groupBox4 = new System.Windows.Forms.GroupBox();
         this.DeleteLightsetButton = new System.Windows.Forms.Button();
         this.AddLightsetButton = new System.Windows.Forms.Button();
         this.LightsetListBox = new System.Windows.Forms.ListBox();
         this.lightSetListBoxMenu = new System.Windows.Forms.ContextMenuStrip(this.components);
         this.setFLSGenLocationToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.generateFLSToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.groupBox5 = new System.Windows.Forms.GroupBox();
         this.soundBankBox = new System.Windows.Forms.TextBox();
         this.label7 = new System.Windows.Forms.Label();
         this.groupBox6 = new System.Windows.Forms.GroupBox();
         this.comboBox2 = new System.Windows.Forms.ComboBox();
         this.label9 = new System.Windows.Forms.Label();
         this.comboBox1 = new System.Windows.Forms.ComboBox();
         this.label8 = new System.Windows.Forms.Label();
         this.groupBox7 = new System.Windows.Forms.GroupBox();
         this.worldComboBox = new System.Windows.Forms.ComboBox();
         this.talkingHeadDeleteBtn = new System.Windows.Forms.Button();
         this.groupBox8 = new System.Windows.Forms.GroupBox();
         this.talkingHeadVideos = new System.Windows.Forms.ListBox();
         this.talkingHeadAddBtn = new System.Windows.Forms.Button();
         this.groupBox9 = new System.Windows.Forms.GroupBox();
         this.LoadVisRepBox = new System.Windows.Forms.CheckBox();
         this.VeterancyCheck = new System.Windows.Forms.CheckBox();
         this.groupBox10 = new System.Windows.Forms.GroupBox();
         this.GlobalExcludeUnitsOptionChooser = new EditorCore.Controls.Micro.CollectionChooser();
         this.groupBox1.SuspendLayout();
         this.groupBox2.SuspendLayout();
         this.groupBox3.SuspendLayout();
         this.groupBox4.SuspendLayout();
         this.lightSetListBoxMenu.SuspendLayout();
         this.groupBox5.SuspendLayout();
         this.groupBox6.SuspendLayout();
         this.groupBox7.SuspendLayout();
         this.groupBox8.SuspendLayout();
         this.groupBox9.SuspendLayout();
         this.groupBox10.SuspendLayout();
         this.SuspendLayout();
         // 
         // label1
         // 
         this.label1.AutoSize = true;
         this.label1.Location = new System.Drawing.Point(292, 24);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(40, 13);
         this.label1.TabIndex = 4;
         this.label1.Text = "MAX X";
         // 
         // label2
         // 
         this.label2.AutoSize = true;
         this.label2.Location = new System.Drawing.Point(22, 24);
         this.label2.Name = "label2";
         this.label2.Size = new System.Drawing.Size(37, 13);
         this.label2.TabIndex = 5;
         this.label2.Text = "MIN X";
         // 
         // label3
         // 
         this.label3.AutoSize = true;
         this.label3.Location = new System.Drawing.Point(292, 52);
         this.label3.Name = "label3";
         this.label3.Size = new System.Drawing.Size(40, 13);
         this.label3.TabIndex = 6;
         this.label3.Text = "MAX Z";
         // 
         // label4
         // 
         this.label4.AutoSize = true;
         this.label4.Location = new System.Drawing.Point(22, 52);
         this.label4.Name = "label4";
         this.label4.Size = new System.Drawing.Size(37, 13);
         this.label4.TabIndex = 7;
         this.label4.Text = "MIN Z";
         // 
         // groupBox1
         // 
         this.groupBox1.Controls.Add(this.label5);
         this.groupBox1.Controls.Add(this.label2);
         this.groupBox1.Controls.Add(this.minZBounds);
         this.groupBox1.Controls.Add(this.maxXBounds);
         this.groupBox1.Controls.Add(this.minXBounds);
         this.groupBox1.Controls.Add(this.label4);
         this.groupBox1.Controls.Add(this.maxZBounds);
         this.groupBox1.Controls.Add(this.label3);
         this.groupBox1.Controls.Add(this.label1);
         this.groupBox1.Location = new System.Drawing.Point(13, 13);
         this.groupBox1.Name = "groupBox1";
         this.groupBox1.Size = new System.Drawing.Size(543, 124);
         this.groupBox1.TabIndex = 8;
         this.groupBox1.TabStop = false;
         this.groupBox1.Text = "Playable Bounds";
         // 
         // label5
         // 
         this.label5.AutoSize = true;
         this.label5.Location = new System.Drawing.Point(21, 74);
         this.label5.Name = "label5";
         this.label5.Size = new System.Drawing.Size(443, 39);
         this.label5.TabIndex = 8;
         this.label5.Text = "\r\nTo see your changes, update your sim rep (NOTE ensure your simrep density is hi" +
             "gh enough)\r\nNOTE*The above values are in numbers of VIS VERTS. ";
         // 
         // minZBounds
         // 
         this.minZBounds.BackColor = System.Drawing.SystemColors.ControlLight;
         this.minZBounds.Location = new System.Drawing.Point(63, 49);
         this.minZBounds.Name = "minZBounds";
         this.minZBounds.Size = new System.Drawing.Size(195, 22);
         this.minZBounds.TabIndex = 2;
         // 
         // maxXBounds
         // 
         this.maxXBounds.BackColor = System.Drawing.SystemColors.ControlLight;
         this.maxXBounds.Location = new System.Drawing.Point(333, 22);
         this.maxXBounds.Name = "maxXBounds";
         this.maxXBounds.Size = new System.Drawing.Size(195, 22);
         this.maxXBounds.TabIndex = 3;
         // 
         // minXBounds
         // 
         this.minXBounds.BackColor = System.Drawing.SystemColors.ControlLight;
         this.minXBounds.Location = new System.Drawing.Point(63, 21);
         this.minXBounds.Name = "minXBounds";
         this.minXBounds.Size = new System.Drawing.Size(195, 22);
         this.minXBounds.TabIndex = 1;
         // 
         // maxZBounds
         // 
         this.maxZBounds.BackColor = System.Drawing.SystemColors.ControlLight;
         this.maxZBounds.Location = new System.Drawing.Point(333, 49);
         this.maxZBounds.Name = "maxZBounds";
         this.maxZBounds.Size = new System.Drawing.Size(195, 22);
         this.maxZBounds.TabIndex = 0;
         // 
         // groupBox2
         // 
         this.groupBox2.Controls.Add(this.button1);
         this.groupBox2.Controls.Add(this.textBox1);
         this.groupBox2.Controls.Add(this.label6);
         this.groupBox2.Location = new System.Drawing.Point(14, 143);
         this.groupBox2.Name = "groupBox2";
         this.groupBox2.Size = new System.Drawing.Size(542, 54);
         this.groupBox2.TabIndex = 9;
         this.groupBox2.TabStop = false;
         this.groupBox2.Text = "Minimap";
         // 
         // button1
         // 
         this.button1.Location = new System.Drawing.Point(501, 23);
         this.button1.Name = "button1";
         this.button1.Size = new System.Drawing.Size(26, 23);
         this.button1.TabIndex = 2;
         this.button1.Text = "...";
         this.button1.UseVisualStyleBackColor = true;
         this.button1.Click += new System.EventHandler(this.button1_Click);
         // 
         // textBox1
         // 
         this.textBox1.Location = new System.Drawing.Point(127, 25);
         this.textBox1.Name = "textBox1";
         this.textBox1.Size = new System.Drawing.Size(368, 20);
         this.textBox1.TabIndex = 1;
         this.textBox1.TextChanged += new System.EventHandler(this.textBox1_TextChanged);
         // 
         // label6
         // 
         this.label6.AutoSize = true;
         this.label6.Location = new System.Drawing.Point(21, 25);
         this.label6.Name = "label6";
         this.label6.Size = new System.Drawing.Size(67, 13);
         this.label6.TabIndex = 0;
         this.label6.Text = "Map Texture";
         // 
         // groupBox3
         // 
         this.groupBox3.Controls.Add(this.cinematicDeleteButton);
         this.groupBox3.Controls.Add(this.cinematicAddButton);
         this.groupBox3.Controls.Add(this.cinematicListBox);
         this.groupBox3.Location = new System.Drawing.Point(14, 295);
         this.groupBox3.Name = "groupBox3";
         this.groupBox3.Size = new System.Drawing.Size(706, 138);
         this.groupBox3.TabIndex = 10;
         this.groupBox3.TabStop = false;
         this.groupBox3.Text = "Cinematics";
         // 
         // cinematicDeleteButton
         // 
         this.cinematicDeleteButton.Location = new System.Drawing.Point(625, 48);
         this.cinematicDeleteButton.Name = "cinematicDeleteButton";
         this.cinematicDeleteButton.Size = new System.Drawing.Size(75, 23);
         this.cinematicDeleteButton.TabIndex = 5;
         this.cinematicDeleteButton.Text = "Delete";
         this.cinematicDeleteButton.UseVisualStyleBackColor = true;
         this.cinematicDeleteButton.Click += new System.EventHandler(this.cinematicDeleteButton_Click);
         // 
         // cinematicAddButton
         // 
         this.cinematicAddButton.Location = new System.Drawing.Point(625, 19);
         this.cinematicAddButton.Name = "cinematicAddButton";
         this.cinematicAddButton.Size = new System.Drawing.Size(75, 23);
         this.cinematicAddButton.TabIndex = 4;
         this.cinematicAddButton.Text = "Add";
         this.cinematicAddButton.UseVisualStyleBackColor = true;
         this.cinematicAddButton.Click += new System.EventHandler(this.cinematicAddButton_Click);
         // 
         // cinematicListBox
         // 
         this.cinematicListBox.FormattingEnabled = true;
         this.cinematicListBox.Location = new System.Drawing.Point(24, 19);
         this.cinematicListBox.Name = "cinematicListBox";
         this.cinematicListBox.Size = new System.Drawing.Size(595, 108);
         this.cinematicListBox.TabIndex = 3;
         // 
         // groupBox4
         // 
         this.groupBox4.Controls.Add(this.DeleteLightsetButton);
         this.groupBox4.Controls.Add(this.AddLightsetButton);
         this.groupBox4.Controls.Add(this.LightsetListBox);
         this.groupBox4.Location = new System.Drawing.Point(14, 434);
         this.groupBox4.Name = "groupBox4";
         this.groupBox4.Size = new System.Drawing.Size(706, 144);
         this.groupBox4.TabIndex = 11;
         this.groupBox4.TabStop = false;
         this.groupBox4.Text = "Lightsets";
         // 
         // DeleteLightsetButton
         // 
         this.DeleteLightsetButton.Location = new System.Drawing.Point(625, 48);
         this.DeleteLightsetButton.Name = "DeleteLightsetButton";
         this.DeleteLightsetButton.Size = new System.Drawing.Size(75, 23);
         this.DeleteLightsetButton.TabIndex = 5;
         this.DeleteLightsetButton.Text = "Delete";
         this.DeleteLightsetButton.UseVisualStyleBackColor = true;
         this.DeleteLightsetButton.Click += new System.EventHandler(this.DeleteLightsetButton_Click);
         // 
         // AddLightsetButton
         // 
         this.AddLightsetButton.Location = new System.Drawing.Point(624, 19);
         this.AddLightsetButton.Name = "AddLightsetButton";
         this.AddLightsetButton.Size = new System.Drawing.Size(75, 23);
         this.AddLightsetButton.TabIndex = 4;
         this.AddLightsetButton.Text = "Add";
         this.AddLightsetButton.UseVisualStyleBackColor = true;
         this.AddLightsetButton.Click += new System.EventHandler(this.AddLightsetButton_Click);
         // 
         // LightsetListBox
         // 
         this.LightsetListBox.ContextMenuStrip = this.lightSetListBoxMenu;
         this.LightsetListBox.FormattingEnabled = true;
         this.LightsetListBox.Location = new System.Drawing.Point(24, 19);
         this.LightsetListBox.Name = "LightsetListBox";
         this.LightsetListBox.Size = new System.Drawing.Size(594, 108);
         this.LightsetListBox.TabIndex = 3;
         this.LightsetListBox.SelectedIndexChanged += new System.EventHandler(this.LightsetListBox_SelectedIndexChanged);
         // 
         // lightSetListBoxMenu
         // 
         this.lightSetListBoxMenu.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.setFLSGenLocationToolStripMenuItem,
            this.generateFLSToolStripMenuItem});
         this.lightSetListBoxMenu.Name = "lightSetListBoxMenu";
         this.lightSetListBoxMenu.Size = new System.Drawing.Size(159, 48);
         // 
         // setFLSGenLocationToolStripMenuItem
         // 
         this.setFLSGenLocationToolStripMenuItem.Name = "setFLSGenLocationToolStripMenuItem";
         this.setFLSGenLocationToolStripMenuItem.Size = new System.Drawing.Size(158, 22);
         this.setFLSGenLocationToolStripMenuItem.Text = "Set Light Probe";
         this.setFLSGenLocationToolStripMenuItem.Click += new System.EventHandler(this.setFLSGenLocationToolStripMenuItem_Click);
         // 
         // generateFLSToolStripMenuItem
         // 
         this.generateFLSToolStripMenuItem.Name = "generateFLSToolStripMenuItem";
         this.generateFLSToolStripMenuItem.Size = new System.Drawing.Size(158, 22);
         this.generateFLSToolStripMenuItem.Text = "Generate FLS";
         this.generateFLSToolStripMenuItem.Click += new System.EventHandler(this.generateFLSToolStripMenuItem_Click);
         // 
         // groupBox5
         // 
         this.groupBox5.Controls.Add(this.soundBankBox);
         this.groupBox5.Controls.Add(this.label7);
         this.groupBox5.Location = new System.Drawing.Point(13, 584);
         this.groupBox5.Name = "groupBox5";
         this.groupBox5.Size = new System.Drawing.Size(706, 99);
         this.groupBox5.TabIndex = 12;
         this.groupBox5.TabStop = false;
         this.groupBox5.Text = "Sound";
         // 
         // soundBankBox
         // 
         this.soundBankBox.Location = new System.Drawing.Point(24, 30);
         this.soundBankBox.Multiline = true;
         this.soundBankBox.Name = "soundBankBox";
         this.soundBankBox.Size = new System.Drawing.Size(595, 64);
         this.soundBankBox.TabIndex = 1;
         this.soundBankBox.TextChanged += new System.EventHandler(this.textBox2_TextChanged);
         // 
         // label7
         // 
         this.label7.AutoSize = true;
         this.label7.Location = new System.Drawing.Point(22, 14);
         this.label7.Name = "label7";
         this.label7.Size = new System.Drawing.Size(71, 13);
         this.label7.TabIndex = 0;
         this.label7.Text = "Sound Banks";
         // 
         // groupBox6
         // 
         this.groupBox6.Controls.Add(this.comboBox2);
         this.groupBox6.Controls.Add(this.label9);
         this.groupBox6.Controls.Add(this.comboBox1);
         this.groupBox6.Controls.Add(this.label8);
         this.groupBox6.Location = new System.Drawing.Point(14, 203);
         this.groupBox6.Name = "groupBox6";
         this.groupBox6.Size = new System.Drawing.Size(542, 86);
         this.groupBox6.TabIndex = 13;
         this.groupBox6.TabStop = false;
         this.groupBox6.Text = "Building Decal Textures";
         // 
         // comboBox2
         // 
         this.comboBox2.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
         this.comboBox2.FormattingEnabled = true;
         this.comboBox2.Location = new System.Drawing.Point(127, 52);
         this.comboBox2.Name = "comboBox2";
         this.comboBox2.Size = new System.Drawing.Size(368, 21);
         this.comboBox2.TabIndex = 3;
         this.comboBox2.SelectedIndexChanged += new System.EventHandler(this.comboBox2_SelectedIndexChanged);
         // 
         // label9
         // 
         this.label9.AutoSize = true;
         this.label9.Location = new System.Drawing.Point(20, 55);
         this.label9.Name = "label9";
         this.label9.Size = new System.Drawing.Size(92, 13);
         this.label9.TabIndex = 2;
         this.label9.Text = "Covenant Texture";
         // 
         // comboBox1
         // 
         this.comboBox1.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
         this.comboBox1.FormattingEnabled = true;
         this.comboBox1.Location = new System.Drawing.Point(127, 25);
         this.comboBox1.Name = "comboBox1";
         this.comboBox1.Size = new System.Drawing.Size(368, 21);
         this.comboBox1.TabIndex = 1;
         this.comboBox1.SelectedIndexChanged += new System.EventHandler(this.comboBox1_SelectedIndexChanged);
         // 
         // label8
         // 
         this.label8.AutoSize = true;
         this.label8.Location = new System.Drawing.Point(20, 28);
         this.label8.Name = "label8";
         this.label8.Size = new System.Drawing.Size(76, 13);
         this.label8.TabIndex = 0;
         this.label8.Text = "UNSC Texture";
         // 
         // groupBox7
         // 
         this.groupBox7.Controls.Add(this.worldComboBox);
         this.groupBox7.Location = new System.Drawing.Point(14, 790);
         this.groupBox7.Name = "groupBox7";
         this.groupBox7.Size = new System.Drawing.Size(704, 53);
         this.groupBox7.TabIndex = 14;
         this.groupBox7.TabStop = false;
         this.groupBox7.Text = "World";
         // 
         // worldComboBox
         // 
         this.worldComboBox.FormattingEnabled = true;
         this.worldComboBox.Location = new System.Drawing.Point(24, 16);
         this.worldComboBox.Name = "worldComboBox";
         this.worldComboBox.Size = new System.Drawing.Size(595, 21);
         this.worldComboBox.TabIndex = 0;
         this.worldComboBox.SelectedIndexChanged += new System.EventHandler(this.worldComboBox_SelectedIndexChanged);
         // 
         // talkingHeadDeleteBtn
         // 
         this.talkingHeadDeleteBtn.Location = new System.Drawing.Point(625, 48);
         this.talkingHeadDeleteBtn.Name = "talkingHeadDeleteBtn";
         this.talkingHeadDeleteBtn.Size = new System.Drawing.Size(75, 23);
         this.talkingHeadDeleteBtn.TabIndex = 5;
         this.talkingHeadDeleteBtn.Text = "Delete";
         this.talkingHeadDeleteBtn.UseVisualStyleBackColor = true;
         this.talkingHeadDeleteBtn.Click += new System.EventHandler(this.talkingHeadDeleteBtn_Click);
         // 
         // groupBox8
         // 
         this.groupBox8.Controls.Add(this.talkingHeadVideos);
         this.groupBox8.Controls.Add(this.talkingHeadDeleteBtn);
         this.groupBox8.Controls.Add(this.talkingHeadAddBtn);
         this.groupBox8.Location = new System.Drawing.Point(14, 689);
         this.groupBox8.Name = "groupBox8";
         this.groupBox8.Size = new System.Drawing.Size(706, 95);
         this.groupBox8.TabIndex = 11;
         this.groupBox8.TabStop = false;
         this.groupBox8.Text = "Talking Head Videos";
         // 
         // talkingHeadVideos
         // 
         this.talkingHeadVideos.FormattingEnabled = true;
         this.talkingHeadVideos.Location = new System.Drawing.Point(24, 19);
         this.talkingHeadVideos.Name = "talkingHeadVideos";
         this.talkingHeadVideos.Size = new System.Drawing.Size(595, 56);
         this.talkingHeadVideos.TabIndex = 3;
         // 
         // talkingHeadAddBtn
         // 
         this.talkingHeadAddBtn.Location = new System.Drawing.Point(625, 19);
         this.talkingHeadAddBtn.Name = "talkingHeadAddBtn";
         this.talkingHeadAddBtn.Size = new System.Drawing.Size(75, 23);
         this.talkingHeadAddBtn.TabIndex = 4;
         this.talkingHeadAddBtn.Text = "Add";
         this.talkingHeadAddBtn.UseVisualStyleBackColor = true;
         this.talkingHeadAddBtn.Click += new System.EventHandler(this.talkingHeadAddBtn_Click);
         // 
         // groupBox9
         // 
         this.groupBox9.Controls.Add(this.LoadVisRepBox);
         this.groupBox9.Controls.Add(this.VeterancyCheck);
         this.groupBox9.Location = new System.Drawing.Point(573, 19);
         this.groupBox9.Name = "groupBox9";
         this.groupBox9.Size = new System.Drawing.Size(145, 269);
         this.groupBox9.TabIndex = 15;
         this.groupBox9.TabStop = false;
         this.groupBox9.Text = "Flags";
         // 
         // LoadVisRepBox
         // 
         this.LoadVisRepBox.AutoSize = true;
         this.LoadVisRepBox.Checked = true;
         this.LoadVisRepBox.CheckState = System.Windows.Forms.CheckState.Checked;
         this.LoadVisRepBox.Location = new System.Drawing.Point(7, 42);
         this.LoadVisRepBox.Name = "LoadVisRepBox";
         this.LoadVisRepBox.Size = new System.Drawing.Size(126, 17);
         this.LoadVisRepBox.TabIndex = 1;
         this.LoadVisRepBox.Text = "Load Terrain Vis Rep";
         this.LoadVisRepBox.UseVisualStyleBackColor = true;
         this.LoadVisRepBox.CheckedChanged += new System.EventHandler(this.LoadVisRepBox_CheckedChanged);
         // 
         // VeterancyCheck
         // 
         this.VeterancyCheck.AutoSize = true;
         this.VeterancyCheck.Checked = true;
         this.VeterancyCheck.CheckState = System.Windows.Forms.CheckState.Checked;
         this.VeterancyCheck.Location = new System.Drawing.Point(7, 20);
         this.VeterancyCheck.Name = "VeterancyCheck";
         this.VeterancyCheck.Size = new System.Drawing.Size(102, 17);
         this.VeterancyCheck.TabIndex = 0;
         this.VeterancyCheck.Text = "Allow Veterancy";
         this.VeterancyCheck.UseVisualStyleBackColor = true;
         this.VeterancyCheck.CheckedChanged += new System.EventHandler(this.VeterancyCheck_CheckedChanged);
         // 
         // groupBox10
         // 
         this.groupBox10.Controls.Add(this.GlobalExcludeUnitsOptionChooser);
         this.groupBox10.Location = new System.Drawing.Point(726, 19);
         this.groupBox10.Name = "groupBox10";
         this.groupBox10.Size = new System.Drawing.Size(520, 824);
         this.groupBox10.TabIndex = 17;
         this.groupBox10.TabStop = false;
         this.groupBox10.Text = "Global (Archive) Exclude Unit List";
         // 
         // GlobalExcludeUnitsOptionChooser
         // 
         this.GlobalExcludeUnitsOptionChooser.AllowRepeats = true;
         this.GlobalExcludeUnitsOptionChooser.Dock = System.Windows.Forms.DockStyle.Fill;
         this.GlobalExcludeUnitsOptionChooser.Enabled = false;
         this.GlobalExcludeUnitsOptionChooser.Location = new System.Drawing.Point(3, 16);
         this.GlobalExcludeUnitsOptionChooser.Name = "GlobalExcludeUnitsOptionChooser";
         this.GlobalExcludeUnitsOptionChooser.Size = new System.Drawing.Size(514, 805);
         this.GlobalExcludeUnitsOptionChooser.TabIndex = 3;
         // 
         // ScenarioSettings
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.groupBox10);
         this.Controls.Add(this.groupBox9);
         this.Controls.Add(this.groupBox8);
         this.Controls.Add(this.groupBox2);
         this.Controls.Add(this.groupBox7);
         this.Controls.Add(this.groupBox6);
         this.Controls.Add(this.groupBox1);
         this.Controls.Add(this.groupBox3);
         this.Controls.Add(this.groupBox4);
         this.Controls.Add(this.groupBox5);
         this.Name = "ScenarioSettings";
         this.Size = new System.Drawing.Size(1249, 846);
         this.groupBox1.ResumeLayout(false);
         this.groupBox1.PerformLayout();
         this.groupBox2.ResumeLayout(false);
         this.groupBox2.PerformLayout();
         this.groupBox3.ResumeLayout(false);
         this.groupBox4.ResumeLayout(false);
         this.lightSetListBoxMenu.ResumeLayout(false);
         this.groupBox5.ResumeLayout(false);
         this.groupBox5.PerformLayout();
         this.groupBox6.ResumeLayout(false);
         this.groupBox6.PerformLayout();
         this.groupBox7.ResumeLayout(false);
         this.groupBox8.ResumeLayout(false);
         this.groupBox9.ResumeLayout(false);
         this.groupBox9.PerformLayout();
         this.groupBox10.ResumeLayout(false);
         this.ResumeLayout(false);

      }

      #endregion

      private EditorCore.NumericSliderControl maxZBounds;
      private EditorCore.NumericSliderControl minXBounds;
      private EditorCore.NumericSliderControl minZBounds;
      private EditorCore.NumericSliderControl maxXBounds;
      private System.Windows.Forms.Label label1;
      private System.Windows.Forms.Label label2;
      private System.Windows.Forms.Label label3;
      private System.Windows.Forms.Label label4;
      private System.Windows.Forms.GroupBox groupBox1;
      private System.Windows.Forms.Label label5;
      private System.Windows.Forms.GroupBox groupBox2;
      private System.Windows.Forms.Button button1;
      private System.Windows.Forms.TextBox textBox1;
      private System.Windows.Forms.Label label6;
      private System.Windows.Forms.GroupBox groupBox3;
      private System.Windows.Forms.Button cinematicDeleteButton;
      private System.Windows.Forms.Button cinematicAddButton;
      private System.Windows.Forms.ListBox cinematicListBox;
      private System.Windows.Forms.GroupBox groupBox4;
      private System.Windows.Forms.Button DeleteLightsetButton;
      private System.Windows.Forms.Button AddLightsetButton;
      private System.Windows.Forms.ListBox LightsetListBox;
      private System.Windows.Forms.GroupBox groupBox5;
      private System.Windows.Forms.TextBox soundBankBox;
      private System.Windows.Forms.Label label7;
      private System.Windows.Forms.ContextMenuStrip lightSetListBoxMenu;
      private System.Windows.Forms.ToolStripMenuItem setFLSGenLocationToolStripMenuItem;
      private System.Windows.Forms.ToolStripMenuItem generateFLSToolStripMenuItem;
      private System.Windows.Forms.GroupBox groupBox6;
      private System.Windows.Forms.Label label8;
      private System.Windows.Forms.ComboBox comboBox1;
      private System.Windows.Forms.ComboBox comboBox2;
      private System.Windows.Forms.Label label9;
      private System.Windows.Forms.GroupBox groupBox7;
      private System.Windows.Forms.ComboBox worldComboBox;
      private System.Windows.Forms.Button talkingHeadDeleteBtn;
      private System.Windows.Forms.GroupBox groupBox8;
      private System.Windows.Forms.ListBox talkingHeadVideos;
      private System.Windows.Forms.Button talkingHeadAddBtn;
      private System.Windows.Forms.GroupBox groupBox9;
      private System.Windows.Forms.CheckBox VeterancyCheck;
      private System.Windows.Forms.CheckBox LoadVisRepBox;
      private System.Windows.Forms.GroupBox groupBox10;
      private EditorCore.Controls.Micro.CollectionChooser GlobalExcludeUnitsOptionChooser;

   }
}
