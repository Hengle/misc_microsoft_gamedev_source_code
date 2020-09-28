namespace PhoenixEditor
{
   partial class NewClipArt
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
         this.pictureBox1 = new System.Windows.Forms.PictureBox();
         this.SaveAsButton = new System.Windows.Forms.Button();
         this.Cancelbutton = new System.Windows.Forms.Button();
         this.QuickSavebutton = new System.Windows.Forms.Button();
         this.checkBox1 = new System.Windows.Forms.CheckBox();
         this.VertexCheckBox = new System.Windows.Forms.CheckBox();
         this.checkBox3 = new System.Windows.Forms.CheckBox();
         this.subfolderPicker1 = new EditorCore.SubfolderPicker();
         this.smartTreeView1 = new Xceed.SmartUI.Controls.TreeView.SmartTreeView(this.components);
         this.node1 = new Xceed.SmartUI.Controls.TreeView.Node("Metadata");
         this.radioButtonNode1 = new Xceed.SmartUI.Controls.OptionList.RadioButtonNode("Surface Details");
         this.radioButtonNode2 = new Xceed.SmartUI.Controls.OptionList.RadioButtonNode("Building Block");
         this.radioButtonNode3 = new Xceed.SmartUI.Controls.OptionList.RadioButtonNode("Stand Alone Art");
         this.node2 = new Xceed.SmartUI.Controls.TreeView.Node("Features");
         this.checkedListBoxItem1 = new Xceed.SmartUI.Controls.CheckedListBox.CheckedListBoxItem("Cliff");
         this.checkedListBoxItem2 = new Xceed.SmartUI.Controls.CheckedListBox.CheckedListBoxItem("Mesa");
         this.checkedListBoxItem3 = new Xceed.SmartUI.Controls.CheckedListBox.CheckedListBoxItem("Canyon");
         this.checkedListBoxItem4 = new Xceed.SmartUI.Controls.CheckedListBox.CheckedListBoxItem("Crater");
         this.checkedListBoxItem8 = new Xceed.SmartUI.Controls.CheckedListBox.CheckedListBoxItem("Rock Formation");
         this.checkedListBoxItem5 = new Xceed.SmartUI.Controls.CheckedListBox.CheckedListBoxItem("Mountain Range");
         this.checkedListBoxItem6 = new Xceed.SmartUI.Controls.CheckedListBox.CheckedListBoxItem("Mountain Peak");
         this.checkedListBoxItem7 = new Xceed.SmartUI.Controls.CheckedListBox.CheckedListBoxItem("Hill");
         this.textBoxTool1 = new Xceed.SmartUI.Controls.ToolBar.TextBoxTool("Tag");
         this.label1 = new System.Windows.Forms.Label();
         this.MetaDescriptionTextBox = new System.Windows.Forms.TextBox();
         this.MarkForAddCheckBox = new System.Windows.Forms.CheckBox();
         this.label2 = new System.Windows.Forms.Label();
         this.toolTip1 = new System.Windows.Forms.ToolTip(this.components);
         this.FileNameLabel = new System.Windows.Forms.Label();
         this.checkedListBoxItem9 = new Xceed.SmartUI.Controls.CheckedListBox.CheckedListBoxItem("Water Body");
         this.checkedListBoxItem10 = new Xceed.SmartUI.Controls.CheckedListBox.CheckedListBoxItem("Glacier");
         this.checkedListBoxItem11 = new Xceed.SmartUI.Controls.CheckedListBox.CheckedListBoxItem("Misc");
         ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).BeginInit();
         ((System.ComponentModel.ISupportInitialize)(this.smartTreeView1)).BeginInit();
         this.SuspendLayout();
         // 
         // pictureBox1
         // 
         this.pictureBox1.BackColor = System.Drawing.SystemColors.ControlLightLight;
         this.pictureBox1.Location = new System.Drawing.Point(2, 2);
         this.pictureBox1.Name = "pictureBox1";
         this.pictureBox1.Size = new System.Drawing.Size(512, 512);
         this.pictureBox1.TabIndex = 0;
         this.pictureBox1.TabStop = false;
         // 
         // SaveAsButton
         // 
         this.SaveAsButton.Location = new System.Drawing.Point(769, 334);
         this.SaveAsButton.Name = "SaveAsButton";
         this.SaveAsButton.Size = new System.Drawing.Size(75, 23);
         this.SaveAsButton.TabIndex = 1;
         this.SaveAsButton.Text = "Save As";
         this.SaveAsButton.UseVisualStyleBackColor = true;
         this.SaveAsButton.Visible = false;
         this.SaveAsButton.Click += new System.EventHandler(this.SaveAsButton_Click);
         // 
         // Cancelbutton
         // 
         this.Cancelbutton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
         this.Cancelbutton.DialogResult = System.Windows.Forms.DialogResult.Cancel;
         this.Cancelbutton.Location = new System.Drawing.Point(791, 518);
         this.Cancelbutton.Name = "Cancelbutton";
         this.Cancelbutton.Size = new System.Drawing.Size(75, 23);
         this.Cancelbutton.TabIndex = 2;
         this.Cancelbutton.Text = "Cancel";
         this.Cancelbutton.UseVisualStyleBackColor = true;
         this.Cancelbutton.Click += new System.EventHandler(this.Cancelbutton_Click);
         // 
         // QuickSavebutton
         // 
         this.QuickSavebutton.Enabled = false;
         this.QuickSavebutton.Location = new System.Drawing.Point(142, 520);
         this.QuickSavebutton.Name = "QuickSavebutton";
         this.QuickSavebutton.Size = new System.Drawing.Size(71, 23);
         this.QuickSavebutton.TabIndex = 3;
         this.QuickSavebutton.Text = "Ok";
         this.QuickSavebutton.UseVisualStyleBackColor = true;
         this.QuickSavebutton.Click += new System.EventHandler(this.QuickSavebutton_Click);
         // 
         // checkBox1
         // 
         this.checkBox1.AutoSize = true;
         this.checkBox1.Checked = true;
         this.checkBox1.CheckState = System.Windows.Forms.CheckState.Checked;
         this.checkBox1.Location = new System.Drawing.Point(762, 419);
         this.checkBox1.Name = "checkBox1";
         this.checkBox1.Size = new System.Drawing.Size(62, 17);
         this.checkBox1.TabIndex = 4;
         this.checkBox1.Text = "Texture";
         this.checkBox1.UseVisualStyleBackColor = true;
         this.checkBox1.CheckedChanged += new System.EventHandler(this.checkBox1_CheckedChanged);
         // 
         // VertexCheckBox
         // 
         this.VertexCheckBox.AutoSize = true;
         this.VertexCheckBox.Checked = true;
         this.VertexCheckBox.CheckState = System.Windows.Forms.CheckState.Checked;
         this.VertexCheckBox.Location = new System.Drawing.Point(762, 396);
         this.VertexCheckBox.Name = "VertexCheckBox";
         this.VertexCheckBox.Size = new System.Drawing.Size(56, 17);
         this.VertexCheckBox.TabIndex = 5;
         this.VertexCheckBox.Text = "Vertex";
         this.VertexCheckBox.UseVisualStyleBackColor = true;
         this.VertexCheckBox.CheckedChanged += new System.EventHandler(this.VertexCheckBox_CheckedChanged);
         // 
         // checkBox3
         // 
         this.checkBox3.AutoSize = true;
         this.checkBox3.Checked = true;
         this.checkBox3.CheckState = System.Windows.Forms.CheckState.Checked;
         this.checkBox3.Location = new System.Drawing.Point(762, 442);
         this.checkBox3.Name = "checkBox3";
         this.checkBox3.Size = new System.Drawing.Size(98, 17);
         this.checkBox3.TabIndex = 6;
         this.checkBox3.Text = "Unit Placement";
         this.checkBox3.UseVisualStyleBackColor = true;
         this.checkBox3.CheckedChanged += new System.EventHandler(this.checkBox3_CheckedChanged);
         // 
         // subfolderPicker1
         // 
         this.subfolderPicker1.Location = new System.Drawing.Point(520, 256);
         this.subfolderPicker1.Name = "subfolderPicker1";
         this.subfolderPicker1.Size = new System.Drawing.Size(240, 258);
         this.subfolderPicker1.TabIndex = 11;
         // 
         // smartTreeView1
         // 
         this.smartTreeView1.Cursor = System.Windows.Forms.Cursors.Default;
         this.smartTreeView1.Items.AddRange(new Xceed.SmartUI.SmartItem[] {
            this.node1});
         this.smartTreeView1.Location = new System.Drawing.Point(518, 2);
         this.smartTreeView1.Name = "smartTreeView1";
         this.smartTreeView1.Size = new System.Drawing.Size(348, 248);
         this.smartTreeView1.TabIndex = 10;
         this.smartTreeView1.Text = "MetaDataTreeView";
         // 
         // node1
         // 
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
            this.checkedListBoxItem3,
            this.checkedListBoxItem4,
            this.checkedListBoxItem2,
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
         this.checkedListBoxItem2.Tag = "Mesa";
         this.checkedListBoxItem2.Text = "Mesa";
         // 
         // checkedListBoxItem3
         // 
         this.checkedListBoxItem3.Tag = "Canyon";
         this.checkedListBoxItem3.Text = "Canyon";
         // 
         // checkedListBoxItem4
         // 
         this.checkedListBoxItem4.Tag = "Crater";
         this.checkedListBoxItem4.Text = "Crater";
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
         // textBoxTool1
         // 
         this.textBoxTool1.Text = "Tag";
         // 
         // label1
         // 
         this.label1.AutoSize = true;
         this.label1.Location = new System.Drawing.Point(766, 284);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(60, 13);
         this.label1.TabIndex = 1;
         this.label1.Text = "Description";
         this.label1.Visible = false;
         // 
         // MetaDescriptionTextBox
         // 
         this.MetaDescriptionTextBox.Location = new System.Drawing.Point(769, 300);
         this.MetaDescriptionTextBox.Multiline = true;
         this.MetaDescriptionTextBox.Name = "MetaDescriptionTextBox";
         this.MetaDescriptionTextBox.Size = new System.Drawing.Size(76, 28);
         this.MetaDescriptionTextBox.TabIndex = 0;
         this.MetaDescriptionTextBox.Visible = false;
         // 
         // MarkForAddCheckBox
         // 
         this.MarkForAddCheckBox.AutoSize = true;
         this.MarkForAddCheckBox.ForeColor = System.Drawing.Color.Black;
         this.MarkForAddCheckBox.Location = new System.Drawing.Point(2, 522);
         this.MarkForAddCheckBox.Name = "MarkForAddCheckBox";
         this.MarkForAddCheckBox.Size = new System.Drawing.Size(134, 17);
         this.MarkForAddCheckBox.TabIndex = 8;
         this.MarkForAddCheckBox.Text = "Add To Source Control";
         this.toolTip1.SetToolTip(this.MarkForAddCheckBox, "This makes it easier to submit your clipart!\r\nThis will add the clip art to a per" +
                 "force\r\nchangelist.");
         this.MarkForAddCheckBox.UseVisualStyleBackColor = true;
         // 
         // label2
         // 
         this.label2.AutoSize = true;
         this.label2.Location = new System.Drawing.Point(763, 369);
         this.label2.Name = "label2";
         this.label2.Size = new System.Drawing.Size(77, 13);
         this.label2.TabIndex = 9;
         this.label2.Text = "Included Data:";
         // 
         // FileNameLabel
         // 
         this.FileNameLabel.AutoSize = true;
         this.FileNameLabel.Font = new System.Drawing.Font("Microsoft Sans Serif", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
         this.FileNameLabel.Location = new System.Drawing.Point(219, 523);
         this.FileNameLabel.Name = "FileNameLabel";
         this.FileNameLabel.Size = new System.Drawing.Size(67, 16);
         this.FileNameLabel.TabIndex = 12;
         this.FileNameLabel.Text = "FileName";
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
         // NewClipArt
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.CancelButton = this.Cancelbutton;
         this.ClientSize = new System.Drawing.Size(871, 545);
         this.Controls.Add(this.FileNameLabel);
         this.Controls.Add(this.smartTreeView1);
         this.Controls.Add(this.subfolderPicker1);
         this.Controls.Add(this.label2);
         this.Controls.Add(this.MarkForAddCheckBox);
         this.Controls.Add(this.label1);
         this.Controls.Add(this.QuickSavebutton);
         this.Controls.Add(this.MetaDescriptionTextBox);
         this.Controls.Add(this.checkBox3);
         this.Controls.Add(this.VertexCheckBox);
         this.Controls.Add(this.checkBox1);
         this.Controls.Add(this.Cancelbutton);
         this.Controls.Add(this.SaveAsButton);
         this.Controls.Add(this.pictureBox1);
         this.Name = "NewClipArt";
         this.Text = "NewClipArt";
         this.Load += new System.EventHandler(this.NewClipArt_Load);
         ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).EndInit();
         ((System.ComponentModel.ISupportInitialize)(this.smartTreeView1)).EndInit();
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private System.Windows.Forms.PictureBox pictureBox1;
      private System.Windows.Forms.Button SaveAsButton;
      private System.Windows.Forms.Button Cancelbutton;
      private System.Windows.Forms.Button QuickSavebutton;
      private System.Windows.Forms.CheckBox checkBox1;
      private System.Windows.Forms.CheckBox VertexCheckBox;
      private System.Windows.Forms.CheckBox checkBox3;
      private System.Windows.Forms.Label label1;
      private System.Windows.Forms.TextBox MetaDescriptionTextBox;
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
      private Xceed.SmartUI.Controls.CheckedListBox.CheckedListBoxItem checkedListBoxItem5;
      private Xceed.SmartUI.Controls.CheckedListBox.CheckedListBoxItem checkedListBoxItem6;
      private Xceed.SmartUI.Controls.ToolBar.TextBoxTool textBoxTool1;
      private System.Windows.Forms.CheckBox MarkForAddCheckBox;
      private System.Windows.Forms.Label label2;
      private System.Windows.Forms.ToolTip toolTip1;
      private Xceed.SmartUI.Controls.CheckedListBox.CheckedListBoxItem checkedListBoxItem7;
      private EditorCore.SubfolderPicker subfolderPicker1;
      private Xceed.SmartUI.Controls.CheckedListBox.CheckedListBoxItem checkedListBoxItem8;
      private System.Windows.Forms.Label FileNameLabel;
      private Xceed.SmartUI.Controls.CheckedListBox.CheckedListBoxItem checkedListBoxItem9;
      private Xceed.SmartUI.Controls.CheckedListBox.CheckedListBoxItem checkedListBoxItem10;
      private Xceed.SmartUI.Controls.CheckedListBox.CheckedListBoxItem checkedListBoxItem11;
   }
}