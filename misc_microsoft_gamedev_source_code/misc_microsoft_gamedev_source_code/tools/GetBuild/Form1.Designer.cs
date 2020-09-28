namespace GetBuild
{
   partial class Form1
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
         this.buildFolderTextBox = new System.Windows.Forms.MaskedTextBox();
         this.label1 = new System.Windows.Forms.Label();
         this.localFolderTextBox = new System.Windows.Forms.MaskedTextBox();
         this.label2 = new System.Windows.Forms.Label();
         this.goButton = new System.Windows.Forms.Button();
         this.buildFolderButton = new System.Windows.Forms.Button();
         this.localFolderButton = new System.Windows.Forms.Button();
         this.logList = new System.Windows.Forms.ListBox();
         this.buildListView = new System.Windows.Forms.ListView();
         this.cancelButton = new System.Windows.Forms.Button();
         this.pickPostBatchFileButton = new System.Windows.Forms.Button();
         this.label6 = new System.Windows.Forms.Label();
         this.postBatchFileTextBox = new System.Windows.Forms.MaskedTextBox();
         this.deleteFilesCheckBox = new System.Windows.Forms.CheckBox();
         this.label7 = new System.Windows.Forms.Label();
         this.lastBuildTextBox = new System.Windows.Forms.MaskedTextBox();
         this.lastFileSetTextBox = new System.Windows.Forms.MaskedTextBox();
         this.groupBox1 = new System.Windows.Forms.GroupBox();
         this.pickPreBatchFileButton = new System.Windows.Forms.Button();
         this.label5 = new System.Windows.Forms.Label();
         this.preBatchFileTextBox = new System.Windows.Forms.MaskedTextBox();
         this.xboxFolderTextBox = new System.Windows.Forms.MaskedTextBox();
         this.groupBox2 = new System.Windows.Forms.GroupBox();
         this.historyDropdown = new System.Windows.Forms.ComboBox();
         this.lastSeparateFolderCheckBox = new System.Windows.Forms.CheckBox();
         this.lastDeleteFilesCheckBox = new System.Windows.Forms.CheckBox();
         this.label3 = new System.Windows.Forms.Label();
         this.lastFolderTextBox = new System.Windows.Forms.MaskedTextBox();
         this.label11 = new System.Windows.Forms.Label();
         this.deleteLocalButton = new System.Windows.Forms.Button();
         this.lastCopiedCheckBox = new System.Windows.Forms.CheckBox();
         this.lastXboxFolderTextBox = new System.Windows.Forms.MaskedTextBox();
         this.lastXboxTextBox = new System.Windows.Forms.MaskedTextBox();
         this.label13 = new System.Windows.Forms.Label();
         this.deleteXboxButton = new System.Windows.Forms.Button();
         this.rebootButton = new System.Windows.Forms.Button();
         this.launchButton = new System.Windows.Forms.Button();
         this.copyButton = new System.Windows.Forms.Button();
         this.groupBox3 = new System.Windows.Forms.GroupBox();
         this.fileSetComboBox = new System.Windows.Forms.ComboBox();
         this.buildTextBox = new System.Windows.Forms.MaskedTextBox();
         this.label4 = new System.Windows.Forms.Label();
         this.getCopyLaunchButton = new System.Windows.Forms.Button();
         this.label9 = new System.Windows.Forms.Label();
         this.separateFolderCheckBox = new System.Windows.Forms.CheckBox();
         this.xboxDropdown = new System.Windows.Forms.ComboBox();
         this.label12 = new System.Windows.Forms.Label();
         this.groupBox4 = new System.Windows.Forms.GroupBox();
         this.groupBox5 = new System.Windows.Forms.GroupBox();
         this.groupBox6 = new System.Windows.Forms.GroupBox();
         this.xfsCheckBox = new System.Windows.Forms.CheckBox();
         this.exeDropdown = new System.Windows.Forms.ComboBox();
         this.ipDropdown = new System.Windows.Forms.ComboBox();
         this.copyMethodDropdown = new System.Windows.Forms.ComboBox();
         this.label16 = new System.Windows.Forms.Label();
         this.label10 = new System.Windows.Forms.Label();
         this.label8 = new System.Windows.Forms.Label();
         this.label15 = new System.Windows.Forms.Label();
         this.label14 = new System.Windows.Forms.Label();
         this.configTextBox = new System.Windows.Forms.TextBox();
         this.groupBox7 = new System.Windows.Forms.GroupBox();
         this.groupBox1.SuspendLayout();
         this.groupBox2.SuspendLayout();
         this.groupBox3.SuspendLayout();
         this.groupBox4.SuspendLayout();
         this.groupBox5.SuspendLayout();
         this.groupBox6.SuspendLayout();
         this.groupBox7.SuspendLayout();
         this.SuspendLayout();
         // 
         // buildFolderTextBox
         // 
         this.buildFolderTextBox.Location = new System.Drawing.Point(94, 17);
         this.buildFolderTextBox.Name = "buildFolderTextBox";
         this.buildFolderTextBox.Size = new System.Drawing.Size(211, 20);
         this.buildFolderTextBox.TabIndex = 9;
         this.buildFolderTextBox.TextChanged += new System.EventHandler(this.buildFolderTextBox_TextChanged);
         // 
         // label1
         // 
         this.label1.AutoSize = true;
         this.label1.Location = new System.Drawing.Point(10, 22);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(65, 13);
         this.label1.TabIndex = 2;
         this.label1.Text = "Build Folder:";
         // 
         // localFolderTextBox
         // 
         this.localFolderTextBox.Location = new System.Drawing.Point(94, 45);
         this.localFolderTextBox.Name = "localFolderTextBox";
         this.localFolderTextBox.Size = new System.Drawing.Size(211, 20);
         this.localFolderTextBox.TabIndex = 11;
         this.localFolderTextBox.TextChanged += new System.EventHandler(this.localFolderTextBox_TextChanged);
         // 
         // label2
         // 
         this.label2.AutoSize = true;
         this.label2.Location = new System.Drawing.Point(10, 48);
         this.label2.Name = "label2";
         this.label2.Size = new System.Drawing.Size(68, 13);
         this.label2.TabIndex = 2;
         this.label2.Text = "Local Folder:";
         // 
         // goButton
         // 
         this.goButton.Location = new System.Drawing.Point(6, 100);
         this.goButton.Name = "goButton";
         this.goButton.Size = new System.Drawing.Size(73, 23);
         this.goButton.TabIndex = 6;
         this.goButton.Text = "Get Build";
         this.goButton.UseVisualStyleBackColor = true;
         this.goButton.Click += new System.EventHandler(this.goButton_Click);
         // 
         // buildFolderButton
         // 
         this.buildFolderButton.Location = new System.Drawing.Point(312, 16);
         this.buildFolderButton.Name = "buildFolderButton";
         this.buildFolderButton.Size = new System.Drawing.Size(24, 24);
         this.buildFolderButton.TabIndex = 10;
         this.buildFolderButton.Text = "...";
         this.buildFolderButton.UseVisualStyleBackColor = true;
         this.buildFolderButton.Click += new System.EventHandler(this.buildFolderButton_Click);
         // 
         // localFolderButton
         // 
         this.localFolderButton.Location = new System.Drawing.Point(311, 42);
         this.localFolderButton.Name = "localFolderButton";
         this.localFolderButton.Size = new System.Drawing.Size(24, 24);
         this.localFolderButton.TabIndex = 12;
         this.localFolderButton.Text = "...";
         this.localFolderButton.UseVisualStyleBackColor = true;
         this.localFolderButton.Click += new System.EventHandler(this.localFolderButton_Click);
         // 
         // logList
         // 
         this.logList.FormattingEnabled = true;
         this.logList.Location = new System.Drawing.Point(6, 14);
         this.logList.Name = "logList";
         this.logList.Size = new System.Drawing.Size(330, 134);
         this.logList.TabIndex = 33;
         // 
         // buildListView
         // 
         this.buildListView.HideSelection = false;
         this.buildListView.Location = new System.Drawing.Point(6, 17);
         this.buildListView.MultiSelect = false;
         this.buildListView.Name = "buildListView";
         this.buildListView.Size = new System.Drawing.Size(695, 181);
         this.buildListView.TabIndex = 1;
         this.buildListView.UseCompatibleStateImageBehavior = false;
         this.buildListView.View = System.Windows.Forms.View.List;
         this.buildListView.ItemSelectionChanged += new System.Windows.Forms.ListViewItemSelectionChangedEventHandler(this.buildListView_ItemSelectionChanged);
         // 
         // cancelButton
         // 
         this.cancelButton.Location = new System.Drawing.Point(268, 100);
         this.cancelButton.Name = "cancelButton";
         this.cancelButton.Size = new System.Drawing.Size(73, 23);
         this.cancelButton.TabIndex = 8;
         this.cancelButton.Text = "Cancel";
         this.cancelButton.UseVisualStyleBackColor = true;
         this.cancelButton.Click += new System.EventHandler(this.cancelButton_Click);
         // 
         // pickPostBatchFileButton
         // 
         this.pickPostBatchFileButton.Location = new System.Drawing.Point(312, 94);
         this.pickPostBatchFileButton.Name = "pickPostBatchFileButton";
         this.pickPostBatchFileButton.Size = new System.Drawing.Size(24, 24);
         this.pickPostBatchFileButton.TabIndex = 16;
         this.pickPostBatchFileButton.Text = "...";
         this.pickPostBatchFileButton.UseVisualStyleBackColor = true;
         this.pickPostBatchFileButton.Click += new System.EventHandler(this.pickPostBatchFileButton_Click);
         // 
         // label6
         // 
         this.label6.AutoSize = true;
         this.label6.Location = new System.Drawing.Point(10, 100);
         this.label6.Name = "label6";
         this.label6.Size = new System.Drawing.Size(81, 13);
         this.label6.TabIndex = 20;
         this.label6.Text = "Post Batch File:";
         // 
         // postBatchFileTextBox
         // 
         this.postBatchFileTextBox.Location = new System.Drawing.Point(94, 97);
         this.postBatchFileTextBox.Name = "postBatchFileTextBox";
         this.postBatchFileTextBox.Size = new System.Drawing.Size(212, 20);
         this.postBatchFileTextBox.TabIndex = 15;
         this.postBatchFileTextBox.TextChanged += new System.EventHandler(this.batchFileTextBox_TextChanged);
         // 
         // deleteFilesCheckBox
         // 
         this.deleteFilesCheckBox.AutoSize = true;
         this.deleteFilesCheckBox.Checked = true;
         this.deleteFilesCheckBox.CheckState = System.Windows.Forms.CheckState.Checked;
         this.deleteFilesCheckBox.Enabled = false;
         this.deleteFilesCheckBox.Location = new System.Drawing.Point(6, 77);
         this.deleteFilesCheckBox.Name = "deleteFilesCheckBox";
         this.deleteFilesCheckBox.Size = new System.Drawing.Size(142, 17);
         this.deleteFilesCheckBox.TabIndex = 4;
         this.deleteFilesCheckBox.Text = "Clear Folder Before Get?";
         this.deleteFilesCheckBox.UseVisualStyleBackColor = true;
         this.deleteFilesCheckBox.CheckedChanged += new System.EventHandler(this.deleteFilesCheckBox_CheckedChanged);
         // 
         // label7
         // 
         this.label7.AutoSize = true;
         this.label7.Location = new System.Drawing.Point(3, 50);
         this.label7.Name = "label7";
         this.label7.Size = new System.Drawing.Size(33, 13);
         this.label7.TabIndex = 22;
         this.label7.Text = "Build:";
         // 
         // lastBuildTextBox
         // 
         this.lastBuildTextBox.Location = new System.Drawing.Point(48, 47);
         this.lastBuildTextBox.Name = "lastBuildTextBox";
         this.lastBuildTextBox.ReadOnly = true;
         this.lastBuildTextBox.Size = new System.Drawing.Size(112, 20);
         this.lastBuildTextBox.TabIndex = 18;
         // 
         // lastFileSetTextBox
         // 
         this.lastFileSetTextBox.Location = new System.Drawing.Point(165, 47);
         this.lastFileSetTextBox.Name = "lastFileSetTextBox";
         this.lastFileSetTextBox.ReadOnly = true;
         this.lastFileSetTextBox.Size = new System.Drawing.Size(176, 20);
         this.lastFileSetTextBox.TabIndex = 19;
         // 
         // groupBox1
         // 
         this.groupBox1.Controls.Add(this.pickPreBatchFileButton);
         this.groupBox1.Controls.Add(this.pickPostBatchFileButton);
         this.groupBox1.Controls.Add(this.label6);
         this.groupBox1.Controls.Add(this.label5);
         this.groupBox1.Controls.Add(this.postBatchFileTextBox);
         this.groupBox1.Controls.Add(this.preBatchFileTextBox);
         this.groupBox1.Controls.Add(this.localFolderButton);
         this.groupBox1.Controls.Add(this.buildFolderButton);
         this.groupBox1.Controls.Add(this.label2);
         this.groupBox1.Controls.Add(this.label1);
         this.groupBox1.Controls.Add(this.localFolderTextBox);
         this.groupBox1.Controls.Add(this.buildFolderTextBox);
         this.groupBox1.Location = new System.Drawing.Point(4, 356);
         this.groupBox1.Name = "groupBox1";
         this.groupBox1.Size = new System.Drawing.Size(347, 154);
         this.groupBox1.TabIndex = 26;
         this.groupBox1.TabStop = false;
         this.groupBox1.Text = "Get Settings";
         // 
         // pickPreBatchFileButton
         // 
         this.pickPreBatchFileButton.Location = new System.Drawing.Point(312, 68);
         this.pickPreBatchFileButton.Name = "pickPreBatchFileButton";
         this.pickPreBatchFileButton.Size = new System.Drawing.Size(24, 24);
         this.pickPreBatchFileButton.TabIndex = 14;
         this.pickPreBatchFileButton.Text = "...";
         this.pickPreBatchFileButton.UseVisualStyleBackColor = true;
         this.pickPreBatchFileButton.Click += new System.EventHandler(this.pickPreBatchFileButton_Click);
         // 
         // label5
         // 
         this.label5.AutoSize = true;
         this.label5.Location = new System.Drawing.Point(10, 74);
         this.label5.Name = "label5";
         this.label5.Size = new System.Drawing.Size(76, 13);
         this.label5.TabIndex = 23;
         this.label5.Text = "Pre Batch File:";
         // 
         // preBatchFileTextBox
         // 
         this.preBatchFileTextBox.Location = new System.Drawing.Point(94, 71);
         this.preBatchFileTextBox.Name = "preBatchFileTextBox";
         this.preBatchFileTextBox.Size = new System.Drawing.Size(212, 20);
         this.preBatchFileTextBox.TabIndex = 13;
         // 
         // xboxFolderTextBox
         // 
         this.xboxFolderTextBox.Location = new System.Drawing.Point(78, 45);
         this.xboxFolderTextBox.Name = "xboxFolderTextBox";
         this.xboxFolderTextBox.Size = new System.Drawing.Size(261, 20);
         this.xboxFolderTextBox.TabIndex = 25;
         this.xboxFolderTextBox.TextChanged += new System.EventHandler(this.localFolderTextBox_TextChanged);
         // 
         // groupBox2
         // 
         this.groupBox2.Controls.Add(this.historyDropdown);
         this.groupBox2.Controls.Add(this.lastSeparateFolderCheckBox);
         this.groupBox2.Controls.Add(this.lastDeleteFilesCheckBox);
         this.groupBox2.Controls.Add(this.label3);
         this.groupBox2.Controls.Add(this.lastFolderTextBox);
         this.groupBox2.Controls.Add(this.label11);
         this.groupBox2.Controls.Add(this.lastFileSetTextBox);
         this.groupBox2.Controls.Add(this.lastBuildTextBox);
         this.groupBox2.Controls.Add(this.label7);
         this.groupBox2.Controls.Add(this.deleteLocalButton);
         this.groupBox2.Location = new System.Drawing.Point(363, 217);
         this.groupBox2.Name = "groupBox2";
         this.groupBox2.Size = new System.Drawing.Size(347, 133);
         this.groupBox2.TabIndex = 27;
         this.groupBox2.TabStop = false;
         this.groupBox2.Text = "Get History";
         // 
         // historyDropdown
         // 
         this.historyDropdown.FormattingEnabled = true;
         this.historyDropdown.Location = new System.Drawing.Point(48, 20);
         this.historyDropdown.Name = "historyDropdown";
         this.historyDropdown.Size = new System.Drawing.Size(290, 21);
         this.historyDropdown.TabIndex = 17;
         this.historyDropdown.SelectedIndexChanged += new System.EventHandler(this.historyDropdown_SelectedIndexChanged);
         // 
         // lastSeparateFolderCheckBox
         // 
         this.lastSeparateFolderCheckBox.AutoSize = true;
         this.lastSeparateFolderCheckBox.Enabled = false;
         this.lastSeparateFolderCheckBox.Location = new System.Drawing.Point(78, 106);
         this.lastSeparateFolderCheckBox.Name = "lastSeparateFolderCheckBox";
         this.lastSeparateFolderCheckBox.Size = new System.Drawing.Size(101, 17);
         this.lastSeparateFolderCheckBox.TabIndex = 22;
         this.lastSeparateFolderCheckBox.Text = "Separate Folder";
         this.lastSeparateFolderCheckBox.UseVisualStyleBackColor = true;
         // 
         // lastDeleteFilesCheckBox
         // 
         this.lastDeleteFilesCheckBox.AutoSize = true;
         this.lastDeleteFilesCheckBox.Enabled = false;
         this.lastDeleteFilesCheckBox.Location = new System.Drawing.Point(10, 106);
         this.lastDeleteFilesCheckBox.Name = "lastDeleteFilesCheckBox";
         this.lastDeleteFilesCheckBox.Size = new System.Drawing.Size(62, 17);
         this.lastDeleteFilesCheckBox.TabIndex = 21;
         this.lastDeleteFilesCheckBox.Text = "Cleared";
         this.lastDeleteFilesCheckBox.UseVisualStyleBackColor = true;
         // 
         // label3
         // 
         this.label3.AutoSize = true;
         this.label3.Location = new System.Drawing.Point(3, 23);
         this.label3.Name = "label3";
         this.label3.Size = new System.Drawing.Size(42, 13);
         this.label3.TabIndex = 22;
         this.label3.Text = "History:";
         // 
         // lastFolderTextBox
         // 
         this.lastFolderTextBox.Location = new System.Drawing.Point(49, 74);
         this.lastFolderTextBox.Name = "lastFolderTextBox";
         this.lastFolderTextBox.ReadOnly = true;
         this.lastFolderTextBox.Size = new System.Drawing.Size(292, 20);
         this.lastFolderTextBox.TabIndex = 20;
         // 
         // label11
         // 
         this.label11.AutoSize = true;
         this.label11.Location = new System.Drawing.Point(4, 78);
         this.label11.Name = "label11";
         this.label11.Size = new System.Drawing.Size(39, 13);
         this.label11.TabIndex = 24;
         this.label11.Text = "Folder:";
         // 
         // deleteLocalButton
         // 
         this.deleteLocalButton.Location = new System.Drawing.Point(285, 100);
         this.deleteLocalButton.Name = "deleteLocalButton";
         this.deleteLocalButton.Size = new System.Drawing.Size(56, 23);
         this.deleteLocalButton.TabIndex = 23;
         this.deleteLocalButton.Text = "Delete";
         this.deleteLocalButton.UseVisualStyleBackColor = true;
         this.deleteLocalButton.Click += new System.EventHandler(this.deleteLocalButton_Click);
         // 
         // lastCopiedCheckBox
         // 
         this.lastCopiedCheckBox.AutoSize = true;
         this.lastCopiedCheckBox.Enabled = false;
         this.lastCopiedCheckBox.Location = new System.Drawing.Point(13, 100);
         this.lastCopiedCheckBox.Name = "lastCopiedCheckBox";
         this.lastCopiedCheckBox.Size = new System.Drawing.Size(59, 17);
         this.lastCopiedCheckBox.TabIndex = 26;
         this.lastCopiedCheckBox.Text = "Copied";
         this.lastCopiedCheckBox.UseVisualStyleBackColor = true;
         // 
         // lastXboxFolderTextBox
         // 
         this.lastXboxFolderTextBox.Location = new System.Drawing.Point(165, 98);
         this.lastXboxFolderTextBox.Name = "lastXboxFolderTextBox";
         this.lastXboxFolderTextBox.ReadOnly = true;
         this.lastXboxFolderTextBox.Size = new System.Drawing.Size(114, 20);
         this.lastXboxFolderTextBox.TabIndex = 28;
         // 
         // lastXboxTextBox
         // 
         this.lastXboxTextBox.Location = new System.Drawing.Point(75, 98);
         this.lastXboxTextBox.Name = "lastXboxTextBox";
         this.lastXboxTextBox.ReadOnly = true;
         this.lastXboxTextBox.Size = new System.Drawing.Size(85, 20);
         this.lastXboxTextBox.TabIndex = 27;
         // 
         // label13
         // 
         this.label13.AutoSize = true;
         this.label13.Location = new System.Drawing.Point(9, 101);
         this.label13.Name = "label13";
         this.label13.Size = new System.Drawing.Size(34, 13);
         this.label13.TabIndex = 24;
         this.label13.Text = "Xbox:";
         this.label13.Visible = false;
         // 
         // deleteXboxButton
         // 
         this.deleteXboxButton.Location = new System.Drawing.Point(285, 96);
         this.deleteXboxButton.Name = "deleteXboxButton";
         this.deleteXboxButton.Size = new System.Drawing.Size(57, 23);
         this.deleteXboxButton.TabIndex = 32;
         this.deleteXboxButton.Text = "Delete";
         this.deleteXboxButton.UseVisualStyleBackColor = true;
         this.deleteXboxButton.Click += new System.EventHandler(this.deleteButton_Click);
         // 
         // rebootButton
         // 
         this.rebootButton.Location = new System.Drawing.Point(285, 16);
         this.rebootButton.Name = "rebootButton";
         this.rebootButton.Size = new System.Drawing.Size(56, 23);
         this.rebootButton.TabIndex = 31;
         this.rebootButton.Text = "Reboot";
         this.rebootButton.UseVisualStyleBackColor = true;
         this.rebootButton.Click += new System.EventHandler(this.rebootButton_Click);
         // 
         // launchButton
         // 
         this.launchButton.Location = new System.Drawing.Point(89, 122);
         this.launchButton.Name = "launchButton";
         this.launchButton.Size = new System.Drawing.Size(73, 23);
         this.launchButton.TabIndex = 30;
         this.launchButton.Text = "Launch";
         this.launchButton.UseVisualStyleBackColor = true;
         this.launchButton.Click += new System.EventHandler(this.launchButton_Click);
         // 
         // copyButton
         // 
         this.copyButton.Location = new System.Drawing.Point(10, 122);
         this.copyButton.Name = "copyButton";
         this.copyButton.Size = new System.Drawing.Size(73, 23);
         this.copyButton.TabIndex = 29;
         this.copyButton.Text = "Copy";
         this.copyButton.UseVisualStyleBackColor = true;
         this.copyButton.Click += new System.EventHandler(this.copyButton_Click);
         // 
         // groupBox3
         // 
         this.groupBox3.Controls.Add(this.fileSetComboBox);
         this.groupBox3.Controls.Add(this.buildTextBox);
         this.groupBox3.Controls.Add(this.label4);
         this.groupBox3.Controls.Add(this.getCopyLaunchButton);
         this.groupBox3.Controls.Add(this.label9);
         this.groupBox3.Controls.Add(this.separateFolderCheckBox);
         this.groupBox3.Controls.Add(this.deleteFilesCheckBox);
         this.groupBox3.Controls.Add(this.cancelButton);
         this.groupBox3.Controls.Add(this.goButton);
         this.groupBox3.Location = new System.Drawing.Point(4, 217);
         this.groupBox3.Name = "groupBox3";
         this.groupBox3.Size = new System.Drawing.Size(347, 133);
         this.groupBox3.TabIndex = 28;
         this.groupBox3.TabStop = false;
         this.groupBox3.Text = "Get Build";
         // 
         // fileSetComboBox
         // 
         this.fileSetComboBox.FormattingEnabled = true;
         this.fileSetComboBox.Location = new System.Drawing.Point(47, 46);
         this.fileSetComboBox.Name = "fileSetComboBox";
         this.fileSetComboBox.Size = new System.Drawing.Size(294, 21);
         this.fileSetComboBox.Sorted = true;
         this.fileSetComboBox.TabIndex = 3;
         // 
         // buildTextBox
         // 
         this.buildTextBox.Location = new System.Drawing.Point(47, 19);
         this.buildTextBox.Name = "buildTextBox";
         this.buildTextBox.ReadOnly = true;
         this.buildTextBox.Size = new System.Drawing.Size(294, 20);
         this.buildTextBox.TabIndex = 2;
         // 
         // label4
         // 
         this.label4.AutoSize = true;
         this.label4.Location = new System.Drawing.Point(1, 23);
         this.label4.Name = "label4";
         this.label4.Size = new System.Drawing.Size(33, 13);
         this.label4.TabIndex = 25;
         this.label4.Text = "Build:";
         // 
         // getCopyLaunchButton
         // 
         this.getCopyLaunchButton.Location = new System.Drawing.Point(85, 100);
         this.getCopyLaunchButton.Name = "getCopyLaunchButton";
         this.getCopyLaunchButton.Size = new System.Drawing.Size(94, 23);
         this.getCopyLaunchButton.TabIndex = 7;
         this.getCopyLaunchButton.Text = "Get/Copy/Run";
         this.getCopyLaunchButton.UseVisualStyleBackColor = true;
         this.getCopyLaunchButton.Click += new System.EventHandler(this.getCopyLaunchButton_Click);
         // 
         // label9
         // 
         this.label9.AutoSize = true;
         this.label9.Location = new System.Drawing.Point(1, 50);
         this.label9.Name = "label9";
         this.label9.Size = new System.Drawing.Size(45, 13);
         this.label9.TabIndex = 22;
         this.label9.Text = "File Set:";
         // 
         // separateFolderCheckBox
         // 
         this.separateFolderCheckBox.AutoSize = true;
         this.separateFolderCheckBox.Location = new System.Drawing.Point(154, 77);
         this.separateFolderCheckBox.Name = "separateFolderCheckBox";
         this.separateFolderCheckBox.Size = new System.Drawing.Size(152, 17);
         this.separateFolderCheckBox.TabIndex = 5;
         this.separateFolderCheckBox.Text = "Separate Folder Per Build?";
         this.separateFolderCheckBox.UseVisualStyleBackColor = true;
         this.separateFolderCheckBox.CheckedChanged += new System.EventHandler(this.deleteFilesCheckBox_CheckedChanged);
         // 
         // xboxDropdown
         // 
         this.xboxDropdown.FormattingEnabled = true;
         this.xboxDropdown.Location = new System.Drawing.Point(78, 18);
         this.xboxDropdown.Name = "xboxDropdown";
         this.xboxDropdown.Size = new System.Drawing.Size(201, 21);
         this.xboxDropdown.Sorted = true;
         this.xboxDropdown.TabIndex = 24;
         this.xboxDropdown.SelectedIndexChanged += new System.EventHandler(this.xboxDropdown_SelectedIndexChanged);
         // 
         // label12
         // 
         this.label12.AutoSize = true;
         this.label12.Location = new System.Drawing.Point(3, 21);
         this.label12.Name = "label12";
         this.label12.Size = new System.Drawing.Size(65, 13);
         this.label12.TabIndex = 22;
         this.label12.Text = "Xbox Name:";
         // 
         // groupBox4
         // 
         this.groupBox4.Controls.Add(this.buildListView);
         this.groupBox4.Location = new System.Drawing.Point(4, 7);
         this.groupBox4.Name = "groupBox4";
         this.groupBox4.Size = new System.Drawing.Size(706, 204);
         this.groupBox4.TabIndex = 29;
         this.groupBox4.TabStop = false;
         this.groupBox4.Text = "Builds";
         // 
         // groupBox5
         // 
         this.groupBox5.Controls.Add(this.logList);
         this.groupBox5.Location = new System.Drawing.Point(4, 516);
         this.groupBox5.Name = "groupBox5";
         this.groupBox5.Size = new System.Drawing.Size(347, 160);
         this.groupBox5.TabIndex = 30;
         this.groupBox5.TabStop = false;
         this.groupBox5.Text = "Log";
         // 
         // groupBox6
         // 
         this.groupBox6.Controls.Add(this.xfsCheckBox);
         this.groupBox6.Controls.Add(this.lastCopiedCheckBox);
         this.groupBox6.Controls.Add(this.exeDropdown);
         this.groupBox6.Controls.Add(this.ipDropdown);
         this.groupBox6.Controls.Add(this.copyMethodDropdown);
         this.groupBox6.Controls.Add(this.label16);
         this.groupBox6.Controls.Add(this.xboxDropdown);
         this.groupBox6.Controls.Add(this.label10);
         this.groupBox6.Controls.Add(this.label8);
         this.groupBox6.Controls.Add(this.label15);
         this.groupBox6.Controls.Add(this.label12);
         this.groupBox6.Controls.Add(this.label14);
         this.groupBox6.Controls.Add(this.xboxFolderTextBox);
         this.groupBox6.Controls.Add(this.lastXboxFolderTextBox);
         this.groupBox6.Controls.Add(this.copyButton);
         this.groupBox6.Controls.Add(this.lastXboxTextBox);
         this.groupBox6.Controls.Add(this.label13);
         this.groupBox6.Controls.Add(this.launchButton);
         this.groupBox6.Controls.Add(this.rebootButton);
         this.groupBox6.Controls.Add(this.deleteXboxButton);
         this.groupBox6.Location = new System.Drawing.Point(363, 356);
         this.groupBox6.Name = "groupBox6";
         this.groupBox6.Size = new System.Drawing.Size(347, 154);
         this.groupBox6.TabIndex = 31;
         this.groupBox6.TabStop = false;
         this.groupBox6.Text = "Copy Build:";
         // 
         // xfsCheckBox
         // 
         this.xfsCheckBox.AutoSize = true;
         this.xfsCheckBox.Location = new System.Drawing.Point(296, 126);
         this.xfsCheckBox.Name = "xfsCheckBox";
         this.xfsCheckBox.Size = new System.Drawing.Size(46, 17);
         this.xfsCheckBox.TabIndex = 26;
         this.xfsCheckBox.Text = "XFS";
         this.xfsCheckBox.UseVisualStyleBackColor = true;
         // 
         // exeDropdown
         // 
         this.exeDropdown.FormattingEnabled = true;
         this.exeDropdown.Items.AddRange(new object[] {
            "xgameP",
            "xgameF",
            "xgameFLTCG"});
         this.exeDropdown.Location = new System.Drawing.Point(196, 124);
         this.exeDropdown.Name = "exeDropdown";
         this.exeDropdown.Size = new System.Drawing.Size(90, 21);
         this.exeDropdown.TabIndex = 24;
         this.exeDropdown.Text = "xgameP";
         this.exeDropdown.SelectedIndexChanged += new System.EventHandler(this.exeDropdown_SelectedIndexChanged);
         // 
         // ipDropdown
         // 
         this.ipDropdown.FormattingEnabled = true;
         this.ipDropdown.Location = new System.Drawing.Point(225, 71);
         this.ipDropdown.Name = "ipDropdown";
         this.ipDropdown.Size = new System.Drawing.Size(113, 21);
         this.ipDropdown.Sorted = true;
         this.ipDropdown.TabIndex = 24;
         // 
         // copyMethodDropdown
         // 
         this.copyMethodDropdown.FormattingEnabled = true;
         this.copyMethodDropdown.Items.AddRange(new object[] {
            "xfsCopy",
            "xbcp",
            "Internal"});
         this.copyMethodDropdown.Location = new System.Drawing.Point(78, 71);
         this.copyMethodDropdown.Name = "copyMethodDropdown";
         this.copyMethodDropdown.Size = new System.Drawing.Size(120, 21);
         this.copyMethodDropdown.TabIndex = 24;
         this.copyMethodDropdown.Text = "xfsCopy";
         this.copyMethodDropdown.SelectedIndexChanged += new System.EventHandler(this.copyMethodDropdown_SelectedIndexChanged);
         // 
         // label16
         // 
         this.label16.AutoSize = true;
         this.label16.Location = new System.Drawing.Point(170, 127);
         this.label16.Name = "label16";
         this.label16.Size = new System.Drawing.Size(28, 13);
         this.label16.TabIndex = 22;
         this.label16.Text = "Exe:";
         // 
         // label10
         // 
         this.label10.AutoSize = true;
         this.label10.Location = new System.Drawing.Point(204, 74);
         this.label10.Name = "label10";
         this.label10.Size = new System.Drawing.Size(20, 13);
         this.label10.TabIndex = 22;
         this.label10.Text = "IP:";
         // 
         // label8
         // 
         this.label8.AutoSize = true;
         this.label8.Location = new System.Drawing.Point(4, 74);
         this.label8.Name = "label8";
         this.label8.Size = new System.Drawing.Size(73, 13);
         this.label8.TabIndex = 22;
         this.label8.Text = "Copy Method:";
         // 
         // label15
         // 
         this.label15.AutoSize = true;
         this.label15.Location = new System.Drawing.Point(4, 21);
         this.label15.Name = "label15";
         this.label15.Size = new System.Drawing.Size(65, 13);
         this.label15.TabIndex = 22;
         this.label15.Text = "Xbox Name:";
         // 
         // label14
         // 
         this.label14.AutoSize = true;
         this.label14.Location = new System.Drawing.Point(3, 48);
         this.label14.Name = "label14";
         this.label14.Size = new System.Drawing.Size(66, 13);
         this.label14.TabIndex = 2;
         this.label14.Text = "Xbox Folder:";
         // 
         // configTextBox
         // 
         this.configTextBox.AcceptsReturn = true;
         this.configTextBox.Location = new System.Drawing.Point(10, 20);
         this.configTextBox.Multiline = true;
         this.configTextBox.Name = "configTextBox";
         this.configTextBox.ScrollBars = System.Windows.Forms.ScrollBars.Vertical;
         this.configTextBox.Size = new System.Drawing.Size(332, 128);
         this.configTextBox.TabIndex = 34;
         this.configTextBox.WordWrap = false;
         // 
         // groupBox7
         // 
         this.groupBox7.Controls.Add(this.configTextBox);
         this.groupBox7.Location = new System.Drawing.Point(363, 516);
         this.groupBox7.Name = "groupBox7";
         this.groupBox7.Size = new System.Drawing.Size(347, 160);
         this.groupBox7.TabIndex = 32;
         this.groupBox7.TabStop = false;
         this.groupBox7.Text = "Configs";
         // 
         // Form1
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.ClientSize = new System.Drawing.Size(717, 682);
         this.Controls.Add(this.groupBox7);
         this.Controls.Add(this.groupBox6);
         this.Controls.Add(this.groupBox5);
         this.Controls.Add(this.groupBox4);
         this.Controls.Add(this.groupBox3);
         this.Controls.Add(this.groupBox2);
         this.Controls.Add(this.groupBox1);
         this.Name = "Form1";
         this.Text = "GetBuild - v1.20";
         this.Load += new System.EventHandler(this.Form1_Load);
         this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.Form1_FormClosing);
         this.groupBox1.ResumeLayout(false);
         this.groupBox1.PerformLayout();
         this.groupBox2.ResumeLayout(false);
         this.groupBox2.PerformLayout();
         this.groupBox3.ResumeLayout(false);
         this.groupBox3.PerformLayout();
         this.groupBox4.ResumeLayout(false);
         this.groupBox5.ResumeLayout(false);
         this.groupBox6.ResumeLayout(false);
         this.groupBox6.PerformLayout();
         this.groupBox7.ResumeLayout(false);
         this.groupBox7.PerformLayout();
         this.ResumeLayout(false);

      }

      #endregion

      private System.Windows.Forms.MaskedTextBox buildFolderTextBox;
      private System.Windows.Forms.Label label1;
      private System.Windows.Forms.MaskedTextBox localFolderTextBox;
      private System.Windows.Forms.Label label2;
      private System.Windows.Forms.Button goButton;
      private System.Windows.Forms.Button buildFolderButton;
      private System.Windows.Forms.Button localFolderButton;
      private System.Windows.Forms.ListBox logList;
      private System.Windows.Forms.ListView buildListView;
      private System.Windows.Forms.Button cancelButton;
      private System.Windows.Forms.Button pickPostBatchFileButton;
      private System.Windows.Forms.Label label6;
      private System.Windows.Forms.MaskedTextBox postBatchFileTextBox;
      private System.Windows.Forms.CheckBox deleteFilesCheckBox;
      private System.Windows.Forms.Label label7;
      private System.Windows.Forms.MaskedTextBox lastBuildTextBox;
      private System.Windows.Forms.MaskedTextBox lastFileSetTextBox;
      private System.Windows.Forms.GroupBox groupBox1;
      private System.Windows.Forms.GroupBox groupBox2;
      private System.Windows.Forms.GroupBox groupBox3;
      private System.Windows.Forms.Label label9;
      private System.Windows.Forms.CheckBox lastDeleteFilesCheckBox;
      private System.Windows.Forms.MaskedTextBox buildTextBox;
      private System.Windows.Forms.Label label4;
      private System.Windows.Forms.GroupBox groupBox4;
      private System.Windows.Forms.GroupBox groupBox5;
      private System.Windows.Forms.ComboBox fileSetComboBox;
      private System.Windows.Forms.Button pickPreBatchFileButton;
      private System.Windows.Forms.Label label5;
      private System.Windows.Forms.MaskedTextBox preBatchFileTextBox;
      private System.Windows.Forms.Button launchButton;
      private System.Windows.Forms.Button copyButton;
      private System.Windows.Forms.Button getCopyLaunchButton;
      private System.Windows.Forms.CheckBox lastCopiedCheckBox;
      private System.Windows.Forms.MaskedTextBox xboxFolderTextBox;
      private System.Windows.Forms.Button rebootButton;
      private System.Windows.Forms.CheckBox separateFolderCheckBox;
      private System.Windows.Forms.CheckBox lastSeparateFolderCheckBox;
      private System.Windows.Forms.Button deleteXboxButton;
      private System.Windows.Forms.MaskedTextBox lastFolderTextBox;
      private System.Windows.Forms.Label label11;
      private System.Windows.Forms.ComboBox xboxDropdown;
      private System.Windows.Forms.Label label12;
      private System.Windows.Forms.MaskedTextBox lastXboxFolderTextBox;
      private System.Windows.Forms.MaskedTextBox lastXboxTextBox;
      private System.Windows.Forms.Label label13;
      private System.Windows.Forms.GroupBox groupBox6;
      private System.Windows.Forms.TextBox configTextBox;
      private System.Windows.Forms.GroupBox groupBox7;
      private System.Windows.Forms.ComboBox historyDropdown;
      private System.Windows.Forms.Label label3;
      private System.Windows.Forms.Label label15;
      private System.Windows.Forms.Label label14;
      private System.Windows.Forms.Button deleteLocalButton;
      private System.Windows.Forms.ComboBox ipDropdown;
      private System.Windows.Forms.ComboBox copyMethodDropdown;
      private System.Windows.Forms.Label label10;
      private System.Windows.Forms.Label label8;
      private System.Windows.Forms.CheckBox xfsCheckBox;
      private System.Windows.Forms.ComboBox exeDropdown;
      private System.Windows.Forms.Label label16;
   }
}

