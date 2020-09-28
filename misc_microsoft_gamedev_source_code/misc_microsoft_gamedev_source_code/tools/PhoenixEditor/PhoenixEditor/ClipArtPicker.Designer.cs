namespace PhoenixEditor
{
   partial class ClipArtPicker
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
         this.components = new System.ComponentModel.Container();
         this.flowLayoutPanel1 = new System.Windows.Forms.FlowLayoutPanel();
         this.RefreshButton = new System.Windows.Forms.Button();
         this.sizeFiltertrackBar = new System.Windows.Forms.TrackBar();
         this.SearchSizeFilterCheckBox = new System.Windows.Forms.CheckBox();
         this.UpdateMetaDatabutton = new System.Windows.Forms.Button();
         this.UpdateReadOnlycheckBox = new System.Windows.Forms.CheckBox();
         this.SearchFolderCheckBox = new System.Windows.Forms.CheckBox();
         this.subfolderPicker1 = new EditorCore.SubfolderPicker();
         this.SearchDescriptionCheckBox = new System.Windows.Forms.CheckBox();
         this.smartTreeView1 = new Xceed.SmartUI.Controls.TreeView.SmartTreeView(this.components);
         this.node1 = new Xceed.SmartUI.Controls.TreeView.Node("Metadata");
         this.radioButtonNode1 = new Xceed.SmartUI.Controls.OptionList.RadioButtonNode("Surface Details");
         this.radioButtonNode2 = new Xceed.SmartUI.Controls.OptionList.RadioButtonNode("Building Block");
         this.radioButtonNode3 = new Xceed.SmartUI.Controls.OptionList.RadioButtonNode("Stand Alone Art");
         this.node2 = new Xceed.SmartUI.Controls.TreeView.Node("Features");
         this.checkedListBoxItem1 = new Xceed.SmartUI.Controls.CheckedListBox.CheckedListBoxItem("Cliff");
         this.checkedListBoxItem2 = new Xceed.SmartUI.Controls.CheckedListBox.CheckedListBoxItem("Canyon");
         this.checkedListBoxItem3 = new Xceed.SmartUI.Controls.CheckedListBox.CheckedListBoxItem("Crater");
         this.checkedListBoxItem4 = new Xceed.SmartUI.Controls.CheckedListBox.CheckedListBoxItem("Mesa");
         this.checkedListBoxItem8 = new Xceed.SmartUI.Controls.CheckedListBox.CheckedListBoxItem("Rock Formation");
         this.checkedListBoxItem5 = new Xceed.SmartUI.Controls.CheckedListBox.CheckedListBoxItem("Mountain Range");
         this.checkedListBoxItem6 = new Xceed.SmartUI.Controls.CheckedListBox.CheckedListBoxItem("Mountain Peak");
         this.checkedListBoxItem7 = new Xceed.SmartUI.Controls.CheckedListBox.CheckedListBoxItem("Hill");
         this.checkedListBoxItem9 = new Xceed.SmartUI.Controls.CheckedListBox.CheckedListBoxItem("Water Body");
         this.checkedListBoxItem10 = new Xceed.SmartUI.Controls.CheckedListBox.CheckedListBoxItem("Glacier");
         this.checkedListBoxItem11 = new Xceed.SmartUI.Controls.CheckedListBox.CheckedListBoxItem("Misc");
         this.textBoxTool1 = new Xceed.SmartUI.Controls.ToolBar.TextBoxTool("Tag");
         this.SearchSubfoldersCheckBox = new System.Windows.Forms.CheckBox();
         this.checkBox3 = new System.Windows.Forms.CheckBox();
         this.checkBox2 = new System.Windows.Forms.CheckBox();
         this.checkBox1 = new System.Windows.Forms.CheckBox();
         this.groupBox1 = new System.Windows.Forms.GroupBox();
         this.ShowBBOnlyCheckBox = new System.Windows.Forms.CheckBox();
         this.HeightSliderControl = new EditorCore.NumericSliderControl();
         this.SmartHeightcheckBox = new System.Windows.Forms.CheckBox();
         this.DefaultButton = new System.Windows.Forms.Button();
         this.groupBox2 = new System.Windows.Forms.GroupBox();
         this.AddHeightRadioButton = new System.Windows.Forms.RadioButton();
         this.radioTallestButton = new System.Windows.Forms.RadioButton();
         this.radioReplaceButton = new System.Windows.Forms.RadioButton();
         this.radioAddButton = new System.Windows.Forms.RadioButton();
         this.ApplyButton = new System.Windows.Forms.Button();
         this.RightClickOKCheckBox = new System.Windows.Forms.CheckBox();
         this.label4 = new System.Windows.Forms.Label();
         this.LockScaleCheckBox = new System.Windows.Forms.CheckBox();
         this.label3 = new System.Windows.Forms.Label();
         this.ScaleZSliderControl = new EditorCore.NumericSliderControl();
         this.label2 = new System.Windows.Forms.Label();
         this.label1 = new System.Windows.Forms.Label();
         this.ScaleYSliderControl = new EditorCore.NumericSliderControl();
         this.ZPosUpDown = new System.Windows.Forms.NumericUpDown();
         this.XPosUpDown = new System.Windows.Forms.NumericUpDown();
         this.ChangeLabel = new System.Windows.Forms.Label();
         this.ScaleXSliderControl = new EditorCore.NumericSliderControl();
         this.RotationSliderControl = new EditorCore.NumericSliderControl();
         ((System.ComponentModel.ISupportInitialize)(this.sizeFiltertrackBar)).BeginInit();
         ((System.ComponentModel.ISupportInitialize)(this.smartTreeView1)).BeginInit();
         this.groupBox1.SuspendLayout();
         this.groupBox2.SuspendLayout();
         ((System.ComponentModel.ISupportInitialize)(this.ZPosUpDown)).BeginInit();
         ((System.ComponentModel.ISupportInitialize)(this.XPosUpDown)).BeginInit();
         this.SuspendLayout();
         // 
         // flowLayoutPanel1
         // 
         this.flowLayoutPanel1.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.flowLayoutPanel1.AutoScroll = true;
         this.flowLayoutPanel1.BackColor = System.Drawing.SystemColors.ActiveCaptionText;
         this.flowLayoutPanel1.Location = new System.Drawing.Point(3, 322);
         this.flowLayoutPanel1.Margin = new System.Windows.Forms.Padding(0);
         this.flowLayoutPanel1.Name = "flowLayoutPanel1";
         this.flowLayoutPanel1.Size = new System.Drawing.Size(170, 245);
         this.flowLayoutPanel1.TabIndex = 0;
         // 
         // RefreshButton
         // 
         this.RefreshButton.Location = new System.Drawing.Point(154, 299);
         this.RefreshButton.Margin = new System.Windows.Forms.Padding(0);
         this.RefreshButton.Name = "RefreshButton";
         this.RefreshButton.Size = new System.Drawing.Size(55, 20);
         this.RefreshButton.TabIndex = 1;
         this.RefreshButton.Text = "Refresh";
         this.RefreshButton.UseVisualStyleBackColor = true;
         this.RefreshButton.Click += new System.EventHandler(this.RefreshButton_Click);
         // 
         // sizeFiltertrackBar
         // 
         this.sizeFiltertrackBar.Location = new System.Drawing.Point(43, 297);
         this.sizeFiltertrackBar.Name = "sizeFiltertrackBar";
         this.sizeFiltertrackBar.Size = new System.Drawing.Size(108, 45);
         this.sizeFiltertrackBar.TabIndex = 5;
         this.sizeFiltertrackBar.TickStyle = System.Windows.Forms.TickStyle.None;
         this.sizeFiltertrackBar.Scroll += new System.EventHandler(this.sizeFiltertrackBar_Scroll);
         // 
         // SearchSizeFilterCheckBox
         // 
         this.SearchSizeFilterCheckBox.AutoSize = true;
         this.SearchSizeFilterCheckBox.Location = new System.Drawing.Point(3, 302);
         this.SearchSizeFilterCheckBox.Name = "SearchSizeFilterCheckBox";
         this.SearchSizeFilterCheckBox.Size = new System.Drawing.Size(46, 17);
         this.SearchSizeFilterCheckBox.TabIndex = 4;
         this.SearchSizeFilterCheckBox.Text = "Size";
         this.SearchSizeFilterCheckBox.UseVisualStyleBackColor = true;
         this.SearchSizeFilterCheckBox.CheckedChanged += new System.EventHandler(this.SizeFilterCheckBox_CheckedChanged);
         // 
         // UpdateMetaDatabutton
         // 
         this.UpdateMetaDatabutton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
         this.UpdateMetaDatabutton.Location = new System.Drawing.Point(3, 4179);
         this.UpdateMetaDatabutton.Name = "UpdateMetaDatabutton";
         this.UpdateMetaDatabutton.Size = new System.Drawing.Size(130, 23);
         this.UpdateMetaDatabutton.TabIndex = 4;
         this.UpdateMetaDatabutton.Text = "Update Old Metadata";
         this.UpdateMetaDatabutton.UseVisualStyleBackColor = true;
         this.UpdateMetaDatabutton.Visible = false;
         this.UpdateMetaDatabutton.Click += new System.EventHandler(this.UpdateMetaDatabutton_Click);
         // 
         // UpdateReadOnlycheckBox
         // 
         this.UpdateReadOnlycheckBox.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
         this.UpdateReadOnlycheckBox.AutoSize = true;
         this.UpdateReadOnlycheckBox.Location = new System.Drawing.Point(3, 4156);
         this.UpdateReadOnlycheckBox.Name = "UpdateReadOnlycheckBox";
         this.UpdateReadOnlycheckBox.Size = new System.Drawing.Size(133, 17);
         this.UpdateReadOnlycheckBox.TabIndex = 5;
         this.UpdateReadOnlycheckBox.Text = "Update Readonly Files";
         this.UpdateReadOnlycheckBox.UseVisualStyleBackColor = true;
         this.UpdateReadOnlycheckBox.Visible = false;
         // 
         // SearchFolderCheckBox
         // 
         this.SearchFolderCheckBox.AutoSize = true;
         this.SearchFolderCheckBox.Location = new System.Drawing.Point(3, 5);
         this.SearchFolderCheckBox.Name = "SearchFolderCheckBox";
         this.SearchFolderCheckBox.Size = new System.Drawing.Size(55, 17);
         this.SearchFolderCheckBox.TabIndex = 6;
         this.SearchFolderCheckBox.Text = "Folder";
         this.SearchFolderCheckBox.UseVisualStyleBackColor = true;
         this.SearchFolderCheckBox.CheckedChanged += new System.EventHandler(this.SearchFolderCheckBox_CheckedChanged);
         // 
         // subfolderPicker1
         // 
         this.subfolderPicker1.Enabled = false;
         this.subfolderPicker1.Location = new System.Drawing.Point(3, 28);
         this.subfolderPicker1.Name = "subfolderPicker1";
         this.subfolderPicker1.Size = new System.Drawing.Size(224, 109);
         this.subfolderPicker1.TabIndex = 7;
         // 
         // SearchDescriptionCheckBox
         // 
         this.SearchDescriptionCheckBox.AutoSize = true;
         this.SearchDescriptionCheckBox.Location = new System.Drawing.Point(3, 143);
         this.SearchDescriptionCheckBox.Name = "SearchDescriptionCheckBox";
         this.SearchDescriptionCheckBox.Size = new System.Drawing.Size(79, 17);
         this.SearchDescriptionCheckBox.TabIndex = 8;
         this.SearchDescriptionCheckBox.Text = "Description";
         this.SearchDescriptionCheckBox.UseVisualStyleBackColor = true;
         this.SearchDescriptionCheckBox.CheckedChanged += new System.EventHandler(this.SearchDescriptionCheckBox_CheckedChanged);
         // 
         // smartTreeView1
         // 
         this.smartTreeView1.Cursor = System.Windows.Forms.Cursors.Default;
         this.smartTreeView1.Enabled = false;
         this.smartTreeView1.Items.AddRange(new Xceed.SmartUI.SmartItem[] {
            this.node1});
         this.smartTreeView1.Location = new System.Drawing.Point(3, 166);
         this.smartTreeView1.Name = "smartTreeView1";
         this.smartTreeView1.Size = new System.Drawing.Size(224, 130);
         this.smartTreeView1.TabIndex = 11;
         this.smartTreeView1.Text = "MetaDataTreeView";
         // 
         // node1
         // 
         this.node1.AccessibleName = "Metadata";
         this.node1.Items.AddRange(new Xceed.SmartUI.SmartItem[] {
            this.radioButtonNode1,
            this.radioButtonNode2,
            this.radioButtonNode3,
            this.node2});
         this.node1.Text = "Metadata";
         // 
         // radioButtonNode1
         // 
         this.radioButtonNode1.Grouped = true;
         this.radioButtonNode1.Tag = "Surface";
         this.radioButtonNode1.Text = "Surface Details";
         this.radioButtonNode1.Visible = false;
         // 
         // radioButtonNode2
         // 
         this.radioButtonNode2.Grouped = true;
         this.radioButtonNode2.Tag = "Block";
         this.radioButtonNode2.Text = "Building Block";
         this.radioButtonNode2.Visible = false;
         // 
         // radioButtonNode3
         // 
         this.radioButtonNode3.Grouped = true;
         this.radioButtonNode3.Tag = "Art";
         this.radioButtonNode3.Text = "Stand Alone Art";
         this.radioButtonNode3.Visible = false;
         // 
         // node2
         // 
         this.node2.Items.AddRange(new Xceed.SmartUI.SmartItem[] {
            this.checkedListBoxItem1,
            this.checkedListBoxItem2,
            this.checkedListBoxItem3,
            this.checkedListBoxItem4,
            this.checkedListBoxItem8,
            this.checkedListBoxItem5,
            this.checkedListBoxItem6,
            this.checkedListBoxItem7,
            this.checkedListBoxItem9,
            this.checkedListBoxItem10,
            this.checkedListBoxItem11,
            this.textBoxTool1});
         this.node2.Text = "Features";
         // 
         // checkedListBoxItem1
         // 
         this.checkedListBoxItem1.Tag = "Cliff";
         this.checkedListBoxItem1.Text = "Cliff";
         // 
         // checkedListBoxItem2
         // 
         this.checkedListBoxItem2.Tag = "Canyon";
         this.checkedListBoxItem2.Text = "Canyon";
         // 
         // checkedListBoxItem3
         // 
         this.checkedListBoxItem3.Tag = "Crater";
         this.checkedListBoxItem3.Text = "Crater";
         // 
         // checkedListBoxItem4
         // 
         this.checkedListBoxItem4.Tag = "Mesa";
         this.checkedListBoxItem4.Text = "Mesa";
         // 
         // checkedListBoxItem8
         // 
         this.checkedListBoxItem8.Tag = "Rock Formation";
         this.checkedListBoxItem8.Text = "Rock Formation";
         // 
         // checkedListBoxItem5
         // 
         this.checkedListBoxItem5.Tag = "Mountain Range";
         this.checkedListBoxItem5.Text = "Mountain Range";
         // 
         // checkedListBoxItem6
         // 
         this.checkedListBoxItem6.Tag = "Mountain Peak";
         this.checkedListBoxItem6.Text = "Mountain Peak";
         // 
         // checkedListBoxItem7
         // 
         this.checkedListBoxItem7.Tag = "Hill";
         this.checkedListBoxItem7.Text = "Hill";
         // 
         // checkedListBoxItem9
         // 
         this.checkedListBoxItem9.Tag = "Water Body";
         this.checkedListBoxItem9.Text = "Water Body";
         // 
         // checkedListBoxItem10
         // 
         this.checkedListBoxItem10.Tag = "Glacier";
         this.checkedListBoxItem10.Text = "Glacier";
         // 
         // checkedListBoxItem11
         // 
         this.checkedListBoxItem11.Tag = "Misc";
         this.checkedListBoxItem11.Text = "Misc";
         // 
         // textBoxTool1
         // 
         this.textBoxTool1.Text = "Tag";
         // 
         // SearchSubfoldersCheckBox
         // 
         this.SearchSubfoldersCheckBox.AutoSize = true;
         this.SearchSubfoldersCheckBox.Checked = true;
         this.SearchSubfoldersCheckBox.CheckState = System.Windows.Forms.CheckState.Checked;
         this.SearchSubfoldersCheckBox.Enabled = false;
         this.SearchSubfoldersCheckBox.Location = new System.Drawing.Point(58, 5);
         this.SearchSubfoldersCheckBox.Name = "SearchSubfoldersCheckBox";
         this.SearchSubfoldersCheckBox.Size = new System.Drawing.Size(114, 17);
         this.SearchSubfoldersCheckBox.TabIndex = 12;
         this.SearchSubfoldersCheckBox.Text = "Include Subfolders";
         this.SearchSubfoldersCheckBox.UseVisualStyleBackColor = true;
         this.SearchSubfoldersCheckBox.CheckedChanged += new System.EventHandler(this.SearchSubfoldersCheckBox_CheckedChanged);
         // 
         // checkBox3
         // 
         this.checkBox3.AutoSize = true;
         this.checkBox3.Checked = true;
         this.checkBox3.CheckState = System.Windows.Forms.CheckState.Checked;
         this.checkBox3.Location = new System.Drawing.Point(6, 48);
         this.checkBox3.Name = "checkBox3";
         this.checkBox3.Size = new System.Drawing.Size(62, 17);
         this.checkBox3.TabIndex = 3;
         this.checkBox3.Text = "Objects";
         this.checkBox3.UseVisualStyleBackColor = true;
         this.checkBox3.CheckedChanged += new System.EventHandler(this.checkBox3_CheckedChanged);
         // 
         // checkBox2
         // 
         this.checkBox2.AutoSize = true;
         this.checkBox2.Checked = true;
         this.checkBox2.CheckState = System.Windows.Forms.CheckState.Checked;
         this.checkBox2.Location = new System.Drawing.Point(74, 48);
         this.checkBox2.Name = "checkBox2";
         this.checkBox2.Size = new System.Drawing.Size(67, 17);
         this.checkBox2.TabIndex = 2;
         this.checkBox2.Text = "Textures";
         this.checkBox2.UseVisualStyleBackColor = true;
         this.checkBox2.CheckedChanged += new System.EventHandler(this.checkBox2_CheckedChanged);
         // 
         // checkBox1
         // 
         this.checkBox1.AutoSize = true;
         this.checkBox1.Checked = true;
         this.checkBox1.CheckState = System.Windows.Forms.CheckState.Checked;
         this.checkBox1.Location = new System.Drawing.Point(147, 48);
         this.checkBox1.Name = "checkBox1";
         this.checkBox1.Size = new System.Drawing.Size(50, 17);
         this.checkBox1.TabIndex = 1;
         this.checkBox1.Text = "Verts";
         this.checkBox1.UseVisualStyleBackColor = true;
         this.checkBox1.CheckedChanged += new System.EventHandler(this.checkBox1_CheckedChanged);
         // 
         // groupBox1
         // 
         this.groupBox1.Controls.Add(this.ShowBBOnlyCheckBox);
         this.groupBox1.Controls.Add(this.HeightSliderControl);
         this.groupBox1.Controls.Add(this.SmartHeightcheckBox);
         this.groupBox1.Controls.Add(this.DefaultButton);
         this.groupBox1.Controls.Add(this.groupBox2);
         this.groupBox1.Controls.Add(this.ApplyButton);
         this.groupBox1.Controls.Add(this.RightClickOKCheckBox);
         this.groupBox1.Controls.Add(this.label4);
         this.groupBox1.Controls.Add(this.LockScaleCheckBox);
         this.groupBox1.Controls.Add(this.label3);
         this.groupBox1.Controls.Add(this.ScaleZSliderControl);
         this.groupBox1.Controls.Add(this.label2);
         this.groupBox1.Controls.Add(this.label1);
         this.groupBox1.Controls.Add(this.ScaleYSliderControl);
         this.groupBox1.Controls.Add(this.ZPosUpDown);
         this.groupBox1.Controls.Add(this.XPosUpDown);
         this.groupBox1.Controls.Add(this.ChangeLabel);
         this.groupBox1.Controls.Add(this.ScaleXSliderControl);
         this.groupBox1.Controls.Add(this.RotationSliderControl);
         this.groupBox1.Controls.Add(this.checkBox3);
         this.groupBox1.Controls.Add(this.checkBox2);
         this.groupBox1.Controls.Add(this.checkBox1);
         this.groupBox1.Location = new System.Drawing.Point(3, 570);
         this.groupBox1.Name = "groupBox1";
         this.groupBox1.Size = new System.Drawing.Size(224, 429);
         this.groupBox1.TabIndex = 13;
         this.groupBox1.TabStop = false;
         this.groupBox1.Text = "Clipart Controls !!!";
         // 
         // ShowBBOnlyCheckBox
         // 
         this.ShowBBOnlyCheckBox.AutoSize = true;
         this.ShowBBOnlyCheckBox.Location = new System.Drawing.Point(126, 19);
         this.ShowBBOnlyCheckBox.Name = "ShowBBOnlyCheckBox";
         this.ShowBBOnlyCheckBox.Size = new System.Drawing.Size(94, 17);
         this.ShowBBOnlyCheckBox.TabIndex = 25;
         this.ShowBBOnlyCheckBox.Text = "Show BB Only";
         this.ShowBBOnlyCheckBox.UseVisualStyleBackColor = true;
         this.ShowBBOnlyCheckBox.CheckedChanged += new System.EventHandler(this.ShowBBOnlyCheckBox_CheckedChanged);
         // 
         // HeightSliderControl
         // 
         this.HeightSliderControl.BackColor = System.Drawing.SystemColors.ControlLight;
         this.HeightSliderControl.Location = new System.Drawing.Point(6, 273);
         this.HeightSliderControl.Name = "HeightSliderControl";
         this.HeightSliderControl.Size = new System.Drawing.Size(195, 22);
         this.HeightSliderControl.TabIndex = 16;
         // 
         // SmartHeightcheckBox
         // 
         this.SmartHeightcheckBox.AutoSize = true;
         this.SmartHeightcheckBox.Location = new System.Drawing.Point(50, 257);
         this.SmartHeightcheckBox.Name = "SmartHeightcheckBox";
         this.SmartHeightcheckBox.Size = new System.Drawing.Size(87, 17);
         this.SmartHeightcheckBox.TabIndex = 24;
         this.SmartHeightcheckBox.Text = "Smart Height";
         this.SmartHeightcheckBox.UseVisualStyleBackColor = true;
         this.SmartHeightcheckBox.CheckedChanged += new System.EventHandler(this.SmartHeightcheckBox_CheckedChanged);
         // 
         // DefaultButton
         // 
         this.DefaultButton.Location = new System.Drawing.Point(149, 400);
         this.DefaultButton.Name = "DefaultButton";
         this.DefaultButton.Size = new System.Drawing.Size(57, 23);
         this.DefaultButton.TabIndex = 23;
         this.DefaultButton.Text = "Default";
         this.DefaultButton.UseVisualStyleBackColor = true;
         this.DefaultButton.Click += new System.EventHandler(this.DefaultButton_Click);
         // 
         // groupBox2
         // 
         this.groupBox2.Controls.Add(this.AddHeightRadioButton);
         this.groupBox2.Controls.Add(this.radioTallestButton);
         this.groupBox2.Controls.Add(this.radioReplaceButton);
         this.groupBox2.Controls.Add(this.radioAddButton);
         this.groupBox2.Location = new System.Drawing.Point(6, 301);
         this.groupBox2.Name = "groupBox2";
         this.groupBox2.Size = new System.Drawing.Size(200, 97);
         this.groupBox2.TabIndex = 22;
         this.groupBox2.TabStop = false;
         this.groupBox2.Text = "Merge Logic";
         // 
         // AddHeightRadioButton
         // 
         this.AddHeightRadioButton.AutoSize = true;
         this.AddHeightRadioButton.Location = new System.Drawing.Point(103, 19);
         this.AddHeightRadioButton.Name = "AddHeightRadioButton";
         this.AddHeightRadioButton.Size = new System.Drawing.Size(78, 17);
         this.AddHeightRadioButton.TabIndex = 24;
         this.AddHeightRadioButton.TabStop = true;
         this.AddHeightRadioButton.Text = "Add Height";
         this.AddHeightRadioButton.UseVisualStyleBackColor = true;
         this.AddHeightRadioButton.CheckedChanged += new System.EventHandler(this.AddHeightRadioButton_CheckedChanged);
         // 
         // radioTallestButton
         // 
         this.radioTallestButton.AutoSize = true;
         this.radioTallestButton.Location = new System.Drawing.Point(6, 65);
         this.radioTallestButton.Name = "radioTallestButton";
         this.radioTallestButton.Size = new System.Drawing.Size(56, 17);
         this.radioTallestButton.TabIndex = 23;
         this.radioTallestButton.TabStop = true;
         this.radioTallestButton.Text = "Tallest";
         this.radioTallestButton.UseVisualStyleBackColor = true;
         this.radioTallestButton.CheckedChanged += new System.EventHandler(this.radioTallestButton_CheckedChanged);
         // 
         // radioReplaceButton
         // 
         this.radioReplaceButton.AutoSize = true;
         this.radioReplaceButton.Location = new System.Drawing.Point(6, 42);
         this.radioReplaceButton.Name = "radioReplaceButton";
         this.radioReplaceButton.Size = new System.Drawing.Size(65, 17);
         this.radioReplaceButton.TabIndex = 22;
         this.radioReplaceButton.TabStop = true;
         this.radioReplaceButton.Text = "Replace";
         this.radioReplaceButton.UseVisualStyleBackColor = true;
         this.radioReplaceButton.CheckedChanged += new System.EventHandler(this.radioReplaceButton_CheckedChanged);
         // 
         // radioAddButton
         // 
         this.radioAddButton.AutoSize = true;
         this.radioAddButton.Location = new System.Drawing.Point(6, 19);
         this.radioAddButton.Name = "radioAddButton";
         this.radioAddButton.Size = new System.Drawing.Size(80, 17);
         this.radioAddButton.TabIndex = 21;
         this.radioAddButton.TabStop = true;
         this.radioAddButton.Text = "Add Normal";
         this.radioAddButton.UseVisualStyleBackColor = true;
         this.radioAddButton.CheckedChanged += new System.EventHandler(this.radioAddButton_CheckedChanged);
         // 
         // ApplyButton
         // 
         this.ApplyButton.Location = new System.Drawing.Point(6, 19);
         this.ApplyButton.Name = "ApplyButton";
         this.ApplyButton.Size = new System.Drawing.Size(105, 23);
         this.ApplyButton.TabIndex = 19;
         this.ApplyButton.Text = "Apply !";
         this.ApplyButton.UseVisualStyleBackColor = true;
         this.ApplyButton.Click += new System.EventHandler(this.ApplyButton_Click);
         // 
         // RightClickOKCheckBox
         // 
         this.RightClickOKCheckBox.AutoSize = true;
         this.RightClickOKCheckBox.Location = new System.Drawing.Point(6, 404);
         this.RightClickOKCheckBox.Name = "RightClickOKCheckBox";
         this.RightClickOKCheckBox.Size = new System.Drawing.Size(141, 17);
         this.RightClickOKCheckBox.TabIndex = 18;
         this.RightClickOKCheckBox.Text = "Right Click => Quick OK";
         this.RightClickOKCheckBox.UseVisualStyleBackColor = true;
         this.RightClickOKCheckBox.CheckedChanged += new System.EventHandler(this.RightClickOKCheckBox_CheckedChanged);
         // 
         // label4
         // 
         this.label4.AutoSize = true;
         this.label4.Location = new System.Drawing.Point(6, 257);
         this.label4.Name = "label4";
         this.label4.Size = new System.Drawing.Size(38, 13);
         this.label4.TabIndex = 17;
         this.label4.Text = "Height";
         // 
         // LockScaleCheckBox
         // 
         this.LockScaleCheckBox.AutoSize = true;
         this.LockScaleCheckBox.Checked = true;
         this.LockScaleCheckBox.CheckState = System.Windows.Forms.CheckState.Checked;
         this.LockScaleCheckBox.Location = new System.Drawing.Point(82, 121);
         this.LockScaleCheckBox.Name = "LockScaleCheckBox";
         this.LockScaleCheckBox.Size = new System.Drawing.Size(50, 17);
         this.LockScaleCheckBox.TabIndex = 15;
         this.LockScaleCheckBox.Text = "Lock";
         this.LockScaleCheckBox.UseVisualStyleBackColor = true;
         // 
         // label3
         // 
         this.label3.AutoSize = true;
         this.label3.Location = new System.Drawing.Point(6, 121);
         this.label3.Name = "label3";
         this.label3.Size = new System.Drawing.Size(70, 13);
         this.label3.TabIndex = 14;
         this.label3.Text = "Scale (X,Y,Z)";
         // 
         // ScaleZSliderControl
         // 
         this.ScaleZSliderControl.BackColor = System.Drawing.SystemColors.ControlLight;
         this.ScaleZSliderControl.Location = new System.Drawing.Point(6, 188);
         this.ScaleZSliderControl.Name = "ScaleZSliderControl";
         this.ScaleZSliderControl.Size = new System.Drawing.Size(195, 22);
         this.ScaleZSliderControl.TabIndex = 13;
         // 
         // label2
         // 
         this.label2.AutoSize = true;
         this.label2.Location = new System.Drawing.Point(6, 76);
         this.label2.Name = "label2";
         this.label2.Size = new System.Drawing.Size(47, 13);
         this.label2.TabIndex = 12;
         this.label2.Text = "Rotation";
         // 
         // label1
         // 
         this.label1.AutoSize = true;
         this.label1.Location = new System.Drawing.Point(6, 214);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(70, 13);
         this.label1.TabIndex = 11;
         this.label1.Text = "Position (X,Z)";
         // 
         // ScaleYSliderControl
         // 
         this.ScaleYSliderControl.BackColor = System.Drawing.SystemColors.ControlLight;
         this.ScaleYSliderControl.Location = new System.Drawing.Point(6, 162);
         this.ScaleYSliderControl.Name = "ScaleYSliderControl";
         this.ScaleYSliderControl.Size = new System.Drawing.Size(195, 22);
         this.ScaleYSliderControl.TabIndex = 10;
         // 
         // ZPosUpDown
         // 
         this.ZPosUpDown.Location = new System.Drawing.Point(109, 231);
         this.ZPosUpDown.Maximum = new decimal(new int[] {
            2048,
            0,
            0,
            0});
         this.ZPosUpDown.Name = "ZPosUpDown";
         this.ZPosUpDown.Size = new System.Drawing.Size(88, 20);
         this.ZPosUpDown.TabIndex = 9;
         this.ZPosUpDown.ValueChanged += new System.EventHandler(this.ZPosUpDown_ValueChanged);
         // 
         // XPosUpDown
         // 
         this.XPosUpDown.Location = new System.Drawing.Point(6, 231);
         this.XPosUpDown.Maximum = new decimal(new int[] {
            2048,
            0,
            0,
            0});
         this.XPosUpDown.Name = "XPosUpDown";
         this.XPosUpDown.Size = new System.Drawing.Size(93, 20);
         this.XPosUpDown.TabIndex = 8;
         this.XPosUpDown.ValueChanged += new System.EventHandler(this.XPosUpDown_ValueChanged);
         // 
         // ChangeLabel
         // 
         this.ChangeLabel.AutoSize = true;
         this.ChangeLabel.ForeColor = System.Drawing.Color.Green;
         this.ChangeLabel.Location = new System.Drawing.Point(106, 0);
         this.ChangeLabel.Name = "ChangeLabel";
         this.ChangeLabel.Size = new System.Drawing.Size(33, 13);
         this.ChangeLabel.TabIndex = 7;
         this.ChangeLabel.Text = "Done";
         // 
         // ScaleXSliderControl
         // 
         this.ScaleXSliderControl.BackColor = System.Drawing.SystemColors.ControlLight;
         this.ScaleXSliderControl.Location = new System.Drawing.Point(6, 137);
         this.ScaleXSliderControl.Name = "ScaleXSliderControl";
         this.ScaleXSliderControl.Size = new System.Drawing.Size(195, 22);
         this.ScaleXSliderControl.TabIndex = 6;
         // 
         // RotationSliderControl
         // 
         this.RotationSliderControl.BackColor = System.Drawing.SystemColors.ControlLight;
         this.RotationSliderControl.Location = new System.Drawing.Point(6, 92);
         this.RotationSliderControl.Name = "RotationSliderControl";
         this.RotationSliderControl.Size = new System.Drawing.Size(195, 22);
         this.RotationSliderControl.TabIndex = 5;
         // 
         // ClipArtPicker
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.AutoScroll = true;
         this.AutoScrollMinSize = new System.Drawing.Size(0, 1000);
         this.Controls.Add(this.groupBox1);
         this.Controls.Add(this.SearchSizeFilterCheckBox);
         this.Controls.Add(this.SearchFolderCheckBox);
         this.Controls.Add(this.flowLayoutPanel1);
         this.Controls.Add(this.RefreshButton);
         this.Controls.Add(this.smartTreeView1);
         this.Controls.Add(this.SearchSubfoldersCheckBox);
         this.Controls.Add(this.UpdateReadOnlycheckBox);
         this.Controls.Add(this.UpdateMetaDatabutton);
         this.Controls.Add(this.SearchDescriptionCheckBox);
         this.Controls.Add(this.subfolderPicker1);
         this.Controls.Add(this.sizeFiltertrackBar);
         this.FloatingWindowBounds = new System.Drawing.Rectangle(0, 0, 594, 620);
         this.Key = "ClipArtPicker";
         this.Name = "ClipArtPicker";
         this.Size = new System.Drawing.Size(228, 739);
         this.Text = "ClipArtPicker";
         ((System.ComponentModel.ISupportInitialize)(this.sizeFiltertrackBar)).EndInit();
         ((System.ComponentModel.ISupportInitialize)(this.smartTreeView1)).EndInit();
         this.groupBox1.ResumeLayout(false);
         this.groupBox1.PerformLayout();
         this.groupBox2.ResumeLayout(false);
         this.groupBox2.PerformLayout();
         ((System.ComponentModel.ISupportInitialize)(this.ZPosUpDown)).EndInit();
         ((System.ComponentModel.ISupportInitialize)(this.XPosUpDown)).EndInit();
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private System.Windows.Forms.FlowLayoutPanel flowLayoutPanel1;
      private System.Windows.Forms.Button RefreshButton;
      private System.Windows.Forms.TrackBar sizeFiltertrackBar;
      private System.Windows.Forms.CheckBox SearchSizeFilterCheckBox;
      private System.Windows.Forms.Button UpdateMetaDatabutton;
      private System.Windows.Forms.CheckBox UpdateReadOnlycheckBox;
      private System.Windows.Forms.CheckBox SearchFolderCheckBox;
      private EditorCore.SubfolderPicker subfolderPicker1;
      private System.Windows.Forms.CheckBox SearchDescriptionCheckBox;
      private Xceed.SmartUI.Controls.TreeView.SmartTreeView smartTreeView1;
      private Xceed.SmartUI.Controls.TreeView.Node node1;
      private Xceed.SmartUI.Controls.OptionList.RadioButtonNode radioButtonNode1;
      private Xceed.SmartUI.Controls.OptionList.RadioButtonNode radioButtonNode2;
      private Xceed.SmartUI.Controls.OptionList.RadioButtonNode radioButtonNode3;
      private Xceed.SmartUI.Controls.TreeView.Node node2;
      private Xceed.SmartUI.Controls.CheckedListBox.CheckedListBoxItem checkedListBoxItem1;
      private Xceed.SmartUI.Controls.CheckedListBox.CheckedListBoxItem checkedListBoxItem2;
      private Xceed.SmartUI.Controls.CheckedListBox.CheckedListBoxItem checkedListBoxItem3;
      private Xceed.SmartUI.Controls.CheckedListBox.CheckedListBoxItem checkedListBoxItem4;
      private Xceed.SmartUI.Controls.CheckedListBox.CheckedListBoxItem checkedListBoxItem8;
      private Xceed.SmartUI.Controls.CheckedListBox.CheckedListBoxItem checkedListBoxItem5;
      private Xceed.SmartUI.Controls.CheckedListBox.CheckedListBoxItem checkedListBoxItem6;
      private Xceed.SmartUI.Controls.CheckedListBox.CheckedListBoxItem checkedListBoxItem7;
      private Xceed.SmartUI.Controls.ToolBar.TextBoxTool textBoxTool1;
      private System.Windows.Forms.CheckBox SearchSubfoldersCheckBox;
      private System.Windows.Forms.CheckBox checkBox3;
      private System.Windows.Forms.CheckBox checkBox2;
      private System.Windows.Forms.CheckBox checkBox1;
      private Xceed.SmartUI.Controls.CheckedListBox.CheckedListBoxItem checkedListBoxItem9;
      private Xceed.SmartUI.Controls.CheckedListBox.CheckedListBoxItem checkedListBoxItem10;
      private Xceed.SmartUI.Controls.CheckedListBox.CheckedListBoxItem checkedListBoxItem11;
      private System.Windows.Forms.GroupBox groupBox1;
      private EditorCore.NumericSliderControl ScaleXSliderControl;
      private EditorCore.NumericSliderControl RotationSliderControl;
      private System.Windows.Forms.Label ChangeLabel;
      private System.Windows.Forms.NumericUpDown ZPosUpDown;
      private System.Windows.Forms.NumericUpDown XPosUpDown;
      private EditorCore.NumericSliderControl ScaleYSliderControl;
      private System.Windows.Forms.Label label3;
      private EditorCore.NumericSliderControl ScaleZSliderControl;
      private System.Windows.Forms.Label label2;
      private System.Windows.Forms.Label label1;
      private System.Windows.Forms.CheckBox LockScaleCheckBox;
      private System.Windows.Forms.Label label4;
      private EditorCore.NumericSliderControl HeightSliderControl;
      private System.Windows.Forms.GroupBox groupBox2;
      private System.Windows.Forms.RadioButton radioAddButton;
      private System.Windows.Forms.Button ApplyButton;
      private System.Windows.Forms.CheckBox RightClickOKCheckBox;
      private System.Windows.Forms.RadioButton radioTallestButton;
      private System.Windows.Forms.RadioButton radioReplaceButton;
      private System.Windows.Forms.Button DefaultButton;
      private System.Windows.Forms.CheckBox SmartHeightcheckBox;
      private System.Windows.Forms.CheckBox ShowBBOnlyCheckBox;
      private System.Windows.Forms.RadioButton AddHeightRadioButton;

   }
}