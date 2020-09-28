namespace PhoenixEditor
{
   partial class GR2ObjectImporter
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
         this.ImportButton = new System.Windows.Forms.Button();
         this.CancelImportButton = new System.Windows.Forms.Button();
         this.PathTextBox = new System.Windows.Forms.TextBox();
         this.TemplateListBox = new System.Windows.Forms.ListBox();
         this.ObjectNameTextBox = new System.Windows.Forms.TextBox();
         this.AddToPerforceCheckBox = new System.Windows.Forms.CheckBox();
         this.InPerforceListBox = new System.Windows.Forms.ListBox();
         this.OtherAddLabel = new System.Windows.Forms.Label();
         this.label2 = new System.Windows.Forms.Label();
         this.ToAddListBox = new System.Windows.Forms.CheckedListBox();
         this.ManditoryChangeListBox = new System.Windows.Forms.ListBox();
         this.groupBox1 = new System.Windows.Forms.GroupBox();
         this.tabControl1 = new System.Windows.Forms.TabControl();
         this.BasicTabPage = new System.Windows.Forms.TabPage();
         this.AdvancedErrorsButton = new System.Windows.Forms.Button();
         this.BasicRetryButton = new System.Windows.Forms.Button();
         this.tableLayoutPanel1 = new System.Windows.Forms.TableLayoutPanel();
         this.TemplateFileNameTextBox = new System.Windows.Forms.TextBox();
         this.TemplateIgnoreButton = new System.Windows.Forms.Button();
         this.TemplateDescriptionTextBox = new System.Windows.Forms.TextBox();
         this.BasicErrorListBox = new System.Windows.Forms.ListBox();
         this.DetailsTabPage = new System.Windows.Forms.TabPage();
         this.label3 = new System.Windows.Forms.Label();
         this.DontCheckInCheckBox = new System.Windows.Forms.CheckBox();
         this.ErrorListBox = new System.Windows.Forms.ListBox();
         this.CheckOutAfterSubmitCheckBox = new System.Windows.Forms.CheckBox();
         this.label4 = new System.Windows.Forms.Label();
         this.PickAnimFileButton = new System.Windows.Forms.Button();
         this.AnimUnitFileTextBox = new System.Windows.Forms.TextBox();
         this.AnimExistingRadioButton = new System.Windows.Forms.RadioButton();
         this.AnimNewRadioButton = new System.Windows.Forms.RadioButton();
         this.label1 = new System.Windows.Forms.Label();
         this.Helpbutton1 = new System.Windows.Forms.Button();
         this.groupBox1.SuspendLayout();
         this.tabControl1.SuspendLayout();
         this.BasicTabPage.SuspendLayout();
         this.tableLayoutPanel1.SuspendLayout();
         this.DetailsTabPage.SuspendLayout();
         this.SuspendLayout();
         // 
         // ImportButton
         // 
         this.ImportButton.Location = new System.Drawing.Point(373, 11);
         this.ImportButton.Name = "ImportButton";
         this.ImportButton.Size = new System.Drawing.Size(75, 23);
         this.ImportButton.TabIndex = 1;
         this.ImportButton.Text = "Import";
         this.ImportButton.UseVisualStyleBackColor = true;
         this.ImportButton.Click += new System.EventHandler(this.ImportButton_Click);
         // 
         // CancelImportButton
         // 
         this.CancelImportButton.Location = new System.Drawing.Point(454, 11);
         this.CancelImportButton.Name = "CancelImportButton";
         this.CancelImportButton.Size = new System.Drawing.Size(75, 23);
         this.CancelImportButton.TabIndex = 2;
         this.CancelImportButton.Text = "Cancel";
         this.CancelImportButton.UseVisualStyleBackColor = true;
         this.CancelImportButton.Click += new System.EventHandler(this.CancelImportButton_Click);
         // 
         // PathTextBox
         // 
         this.PathTextBox.Location = new System.Drawing.Point(12, 40);
         this.PathTextBox.Name = "PathTextBox";
         this.PathTextBox.ReadOnly = true;
         this.PathTextBox.Size = new System.Drawing.Size(578, 20);
         this.PathTextBox.TabIndex = 3;
         // 
         // TemplateListBox
         // 
         this.TemplateListBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)));
         this.TemplateListBox.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
         this.TemplateListBox.FormattingEnabled = true;
         this.TemplateListBox.ItemHeight = 20;
         this.TemplateListBox.Location = new System.Drawing.Point(12, 66);
         this.TemplateListBox.Name = "TemplateListBox";
         this.TemplateListBox.Size = new System.Drawing.Size(204, 244);
         this.TemplateListBox.TabIndex = 5;
         // 
         // ObjectNameTextBox
         // 
         this.ObjectNameTextBox.BorderStyle = System.Windows.Forms.BorderStyle.None;
         this.ObjectNameTextBox.Font = new System.Drawing.Font("Microsoft Sans Serif", 14.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
         this.ObjectNameTextBox.Location = new System.Drawing.Point(12, 12);
         this.ObjectNameTextBox.Name = "ObjectNameTextBox";
         this.ObjectNameTextBox.Size = new System.Drawing.Size(355, 22);
         this.ObjectNameTextBox.TabIndex = 6;
         this.ObjectNameTextBox.TextChanged += new System.EventHandler(this.ObjectNameTextBox_TextChanged);
         // 
         // AddToPerforceCheckBox
         // 
         this.AddToPerforceCheckBox.AutoSize = true;
         this.AddToPerforceCheckBox.Checked = true;
         this.AddToPerforceCheckBox.CheckState = System.Windows.Forms.CheckState.Checked;
         this.AddToPerforceCheckBox.Location = new System.Drawing.Point(6, 0);
         this.AddToPerforceCheckBox.Name = "AddToPerforceCheckBox";
         this.AddToPerforceCheckBox.Size = new System.Drawing.Size(104, 17);
         this.AddToPerforceCheckBox.TabIndex = 7;
         this.AddToPerforceCheckBox.Text = "Add To Perforce";
         this.AddToPerforceCheckBox.UseVisualStyleBackColor = true;
         this.AddToPerforceCheckBox.CheckedChanged += new System.EventHandler(this.AddToPerforceCheckBox_CheckedChanged);
         // 
         // InPerforceListBox
         // 
         this.InPerforceListBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.InPerforceListBox.FormattingEnabled = true;
         this.InPerforceListBox.Location = new System.Drawing.Point(9, 284);
         this.InPerforceListBox.Name = "InPerforceListBox";
         this.InPerforceListBox.Size = new System.Drawing.Size(575, 82);
         this.InPerforceListBox.TabIndex = 10;
         // 
         // OtherAddLabel
         // 
         this.OtherAddLabel.AutoSize = true;
         this.OtherAddLabel.Location = new System.Drawing.Point(6, 108);
         this.OtherAddLabel.Name = "OtherAddLabel";
         this.OtherAddLabel.Size = new System.Drawing.Size(146, 13);
         this.OtherAddLabel.TabIndex = 11;
         this.OtherAddLabel.Text = "Other Files that will be added.";
         // 
         // label2
         // 
         this.label2.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
         this.label2.AutoSize = true;
         this.label2.Location = new System.Drawing.Point(6, 268);
         this.label2.Name = "label2";
         this.label2.Size = new System.Drawing.Size(99, 13);
         this.label2.TabIndex = 12;
         this.label2.Text = "Already in Perforce:";
         // 
         // ToAddListBox
         // 
         this.ToAddListBox.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.ToAddListBox.CheckOnClick = true;
         this.ToAddListBox.FormattingEnabled = true;
         this.ToAddListBox.Location = new System.Drawing.Point(9, 124);
         this.ToAddListBox.Name = "ToAddListBox";
         this.ToAddListBox.Size = new System.Drawing.Size(575, 139);
         this.ToAddListBox.TabIndex = 13;
         this.ToAddListBox.ItemCheck += new System.Windows.Forms.ItemCheckEventHandler(this.ToAddListBox_ItemCheck);
         // 
         // ManditoryChangeListBox
         // 
         this.ManditoryChangeListBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.ManditoryChangeListBox.FormattingEnabled = true;
         this.ManditoryChangeListBox.Location = new System.Drawing.Point(9, 88);
         this.ManditoryChangeListBox.Name = "ManditoryChangeListBox";
         this.ManditoryChangeListBox.Size = new System.Drawing.Size(575, 17);
         this.ManditoryChangeListBox.TabIndex = 14;
         // 
         // groupBox1
         // 
         this.groupBox1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.groupBox1.Controls.Add(this.tabControl1);
         this.groupBox1.Controls.Add(this.AddToPerforceCheckBox);
         this.groupBox1.Location = new System.Drawing.Point(238, 66);
         this.groupBox1.Name = "groupBox1";
         this.groupBox1.Size = new System.Drawing.Size(631, 522);
         this.groupBox1.TabIndex = 15;
         this.groupBox1.TabStop = false;
         // 
         // tabControl1
         // 
         this.tabControl1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.tabControl1.Controls.Add(this.BasicTabPage);
         this.tabControl1.Controls.Add(this.DetailsTabPage);
         this.tabControl1.Location = new System.Drawing.Point(6, 19);
         this.tabControl1.Name = "tabControl1";
         this.tabControl1.SelectedIndex = 0;
         this.tabControl1.Size = new System.Drawing.Size(611, 501);
         this.tabControl1.TabIndex = 21;
         // 
         // BasicTabPage
         // 
         this.BasicTabPage.BackColor = System.Drawing.Color.White;
         this.BasicTabPage.Controls.Add(this.AdvancedErrorsButton);
         this.BasicTabPage.Controls.Add(this.BasicRetryButton);
         this.BasicTabPage.Controls.Add(this.tableLayoutPanel1);
         this.BasicTabPage.Controls.Add(this.BasicErrorListBox);
         this.BasicTabPage.Location = new System.Drawing.Point(4, 22);
         this.BasicTabPage.Name = "BasicTabPage";
         this.BasicTabPage.Padding = new System.Windows.Forms.Padding(3);
         this.BasicTabPage.Size = new System.Drawing.Size(603, 475);
         this.BasicTabPage.TabIndex = 0;
         this.BasicTabPage.Text = "Info";
         // 
         // AdvancedErrorsButton
         // 
         this.AdvancedErrorsButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
         this.AdvancedErrorsButton.Location = new System.Drawing.Point(20, 300);
         this.AdvancedErrorsButton.Name = "AdvancedErrorsButton";
         this.AdvancedErrorsButton.Size = new System.Drawing.Size(274, 23);
         this.AdvancedErrorsButton.TabIndex = 3;
         this.AdvancedErrorsButton.Text = "You need to fix some errors in the advanced tab >>";
         this.AdvancedErrorsButton.UseVisualStyleBackColor = true;
         this.AdvancedErrorsButton.Visible = false;
         this.AdvancedErrorsButton.Click += new System.EventHandler(this.AdvancedErrorsButton_Click);
         // 
         // BasicRetryButton
         // 
         this.BasicRetryButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
         this.BasicRetryButton.Location = new System.Drawing.Point(507, 300);
         this.BasicRetryButton.Name = "BasicRetryButton";
         this.BasicRetryButton.Size = new System.Drawing.Size(75, 23);
         this.BasicRetryButton.TabIndex = 1;
         this.BasicRetryButton.Text = "Retry";
         this.BasicRetryButton.UseVisualStyleBackColor = true;
         this.BasicRetryButton.Visible = false;
         this.BasicRetryButton.Click += new System.EventHandler(this.BasicRetryButton_Click);
         // 
         // tableLayoutPanel1
         // 
         this.tableLayoutPanel1.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)));
         this.tableLayoutPanel1.ColumnCount = 3;
         this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 100F));
         this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Absolute, 138F));
         this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Absolute, 84F));
         this.tableLayoutPanel1.Controls.Add(this.TemplateFileNameTextBox, 0, 0);
         this.tableLayoutPanel1.Controls.Add(this.TemplateIgnoreButton, 2, 0);
         this.tableLayoutPanel1.Controls.Add(this.TemplateDescriptionTextBox, 1, 0);
         this.tableLayoutPanel1.Location = new System.Drawing.Point(20, 20);
         this.tableLayoutPanel1.Name = "tableLayoutPanel1";
         this.tableLayoutPanel1.RowCount = 2;
         this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle());
         this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle());
         this.tableLayoutPanel1.Size = new System.Drawing.Size(562, 274);
         this.tableLayoutPanel1.TabIndex = 2;
         // 
         // TemplateFileNameTextBox
         // 
         this.TemplateFileNameTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.TemplateFileNameTextBox.BackColor = System.Drawing.Color.White;
         this.TemplateFileNameTextBox.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Strikeout, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
         this.TemplateFileNameTextBox.Location = new System.Drawing.Point(3, 3);
         this.TemplateFileNameTextBox.Name = "TemplateFileNameTextBox";
         this.TemplateFileNameTextBox.ReadOnly = true;
         this.TemplateFileNameTextBox.Size = new System.Drawing.Size(334, 20);
         this.TemplateFileNameTextBox.TabIndex = 2;
         this.TemplateFileNameTextBox.Text = "Fatal Error. See the help button and try again.";
         // 
         // TemplateIgnoreButton
         // 
         this.TemplateIgnoreButton.ForeColor = System.Drawing.Color.Red;
         this.TemplateIgnoreButton.Location = new System.Drawing.Point(481, 3);
         this.TemplateIgnoreButton.Name = "TemplateIgnoreButton";
         this.TemplateIgnoreButton.Size = new System.Drawing.Size(75, 23);
         this.TemplateIgnoreButton.TabIndex = 0;
         this.TemplateIgnoreButton.Text = "Ignore";
         this.TemplateIgnoreButton.UseVisualStyleBackColor = true;
         // 
         // TemplateDescriptionTextBox
         // 
         this.TemplateDescriptionTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.TemplateDescriptionTextBox.ForeColor = System.Drawing.Color.Red;
         this.TemplateDescriptionTextBox.Location = new System.Drawing.Point(343, 3);
         this.TemplateDescriptionTextBox.Name = "TemplateDescriptionTextBox";
         this.TemplateDescriptionTextBox.Size = new System.Drawing.Size(132, 20);
         this.TemplateDescriptionTextBox.TabIndex = 3;
         this.TemplateDescriptionTextBox.Text = "File Missing";
         // 
         // BasicErrorListBox
         // 
         this.BasicErrorListBox.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
         this.BasicErrorListBox.BackColor = System.Drawing.SystemColors.ControlLightLight;
         this.BasicErrorListBox.BorderStyle = System.Windows.Forms.BorderStyle.None;
         this.BasicErrorListBox.Font = new System.Drawing.Font("Microsoft Sans Serif", 14F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
         this.BasicErrorListBox.ForeColor = System.Drawing.Color.Red;
         this.BasicErrorListBox.FormattingEnabled = true;
         this.BasicErrorListBox.ItemHeight = 24;
         this.BasicErrorListBox.Location = new System.Drawing.Point(6, 335);
         this.BasicErrorListBox.Name = "BasicErrorListBox";
         this.BasicErrorListBox.Size = new System.Drawing.Size(591, 120);
         this.BasicErrorListBox.TabIndex = 1;
         // 
         // DetailsTabPage
         // 
         this.DetailsTabPage.Controls.Add(this.label3);
         this.DetailsTabPage.Controls.Add(this.DontCheckInCheckBox);
         this.DetailsTabPage.Controls.Add(this.ErrorListBox);
         this.DetailsTabPage.Controls.Add(this.CheckOutAfterSubmitCheckBox);
         this.DetailsTabPage.Controls.Add(this.label4);
         this.DetailsTabPage.Controls.Add(this.PickAnimFileButton);
         this.DetailsTabPage.Controls.Add(this.label2);
         this.DetailsTabPage.Controls.Add(this.OtherAddLabel);
         this.DetailsTabPage.Controls.Add(this.AnimUnitFileTextBox);
         this.DetailsTabPage.Controls.Add(this.ToAddListBox);
         this.DetailsTabPage.Controls.Add(this.AnimExistingRadioButton);
         this.DetailsTabPage.Controls.Add(this.InPerforceListBox);
         this.DetailsTabPage.Controls.Add(this.AnimNewRadioButton);
         this.DetailsTabPage.Controls.Add(this.ManditoryChangeListBox);
         this.DetailsTabPage.Controls.Add(this.label1);
         this.DetailsTabPage.Location = new System.Drawing.Point(4, 22);
         this.DetailsTabPage.Name = "DetailsTabPage";
         this.DetailsTabPage.Padding = new System.Windows.Forms.Padding(3);
         this.DetailsTabPage.Size = new System.Drawing.Size(603, 475);
         this.DetailsTabPage.TabIndex = 1;
         this.DetailsTabPage.Text = "Advanced";
         this.DetailsTabPage.UseVisualStyleBackColor = true;
         // 
         // label3
         // 
         this.label3.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
         this.label3.AutoSize = true;
         this.label3.Location = new System.Drawing.Point(6, 377);
         this.label3.Name = "label3";
         this.label3.Size = new System.Drawing.Size(84, 13);
         this.label3.TabIndex = 17;
         this.label3.Text = "Errors/Warnings";
         // 
         // DontCheckInCheckBox
         // 
         this.DontCheckInCheckBox.AutoSize = true;
         this.DontCheckInCheckBox.ForeColor = System.Drawing.SystemColors.ActiveCaption;
         this.DontCheckInCheckBox.Location = new System.Drawing.Point(153, 6);
         this.DontCheckInCheckBox.Name = "DontCheckInCheckBox";
         this.DontCheckInCheckBox.Size = new System.Drawing.Size(96, 17);
         this.DontCheckInCheckBox.TabIndex = 21;
         this.DontCheckInCheckBox.Text = "Don\'t Check in";
         this.DontCheckInCheckBox.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
         this.DontCheckInCheckBox.UseVisualStyleBackColor = true;
         // 
         // ErrorListBox
         // 
         this.ErrorListBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.ErrorListBox.Font = new System.Drawing.Font("Microsoft Sans Serif", 14.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
         this.ErrorListBox.ForeColor = System.Drawing.Color.Red;
         this.ErrorListBox.FormattingEnabled = true;
         this.ErrorListBox.ItemHeight = 24;
         this.ErrorListBox.Location = new System.Drawing.Point(9, 393);
         this.ErrorListBox.Name = "ErrorListBox";
         this.ErrorListBox.Size = new System.Drawing.Size(575, 76);
         this.ErrorListBox.TabIndex = 16;
         // 
         // CheckOutAfterSubmitCheckBox
         // 
         this.CheckOutAfterSubmitCheckBox.AutoSize = true;
         this.CheckOutAfterSubmitCheckBox.Checked = true;
         this.CheckOutAfterSubmitCheckBox.CheckState = System.Windows.Forms.CheckState.Checked;
         this.CheckOutAfterSubmitCheckBox.Location = new System.Drawing.Point(9, 6);
         this.CheckOutAfterSubmitCheckBox.Name = "CheckOutAfterSubmitCheckBox";
         this.CheckOutAfterSubmitCheckBox.Size = new System.Drawing.Size(138, 17);
         this.CheckOutAfterSubmitCheckBox.TabIndex = 22;
         this.CheckOutAfterSubmitCheckBox.Text = "Leave Art Checked Out";
         this.CheckOutAfterSubmitCheckBox.UseVisualStyleBackColor = true;
         // 
         // label4
         // 
         this.label4.AutoSize = true;
         this.label4.Location = new System.Drawing.Point(6, 33);
         this.label4.Name = "label4";
         this.label4.Size = new System.Drawing.Size(73, 13);
         this.label4.TabIndex = 19;
         this.label4.Text = "Anim/Unit File";
         // 
         // PickAnimFileButton
         // 
         this.PickAnimFileButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
         this.PickAnimFileButton.Location = new System.Drawing.Point(354, 46);
         this.PickAnimFileButton.Name = "PickAnimFileButton";
         this.PickAnimFileButton.Size = new System.Drawing.Size(28, 23);
         this.PickAnimFileButton.TabIndex = 20;
         this.PickAnimFileButton.Text = "...";
         this.PickAnimFileButton.UseVisualStyleBackColor = true;
         this.PickAnimFileButton.Click += new System.EventHandler(this.PickAnimFileButton_Click);
         // 
         // AnimUnitFileTextBox
         // 
         this.AnimUnitFileTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.AnimUnitFileTextBox.Location = new System.Drawing.Point(9, 49);
         this.AnimUnitFileTextBox.Name = "AnimUnitFileTextBox";
         this.AnimUnitFileTextBox.Size = new System.Drawing.Size(339, 20);
         this.AnimUnitFileTextBox.TabIndex = 18;
         this.AnimUnitFileTextBox.TextChanged += new System.EventHandler(this.AnimUnitFileTextBox_TextChanged);
         // 
         // AnimExistingRadioButton
         // 
         this.AnimExistingRadioButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
         this.AnimExistingRadioButton.AutoSize = true;
         this.AnimExistingRadioButton.Location = new System.Drawing.Point(475, 46);
         this.AnimExistingRadioButton.Name = "AnimExistingRadioButton";
         this.AnimExistingRadioButton.Size = new System.Drawing.Size(83, 17);
         this.AnimExistingRadioButton.TabIndex = 17;
         this.AnimExistingRadioButton.TabStop = true;
         this.AnimExistingRadioButton.Text = "Use Existing";
         this.AnimExistingRadioButton.UseVisualStyleBackColor = true;
         this.AnimExistingRadioButton.Click += new System.EventHandler(this.OnAnimPerforceRadio);
         // 
         // AnimNewRadioButton
         // 
         this.AnimNewRadioButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
         this.AnimNewRadioButton.AutoSize = true;
         this.AnimNewRadioButton.Location = new System.Drawing.Point(388, 46);
         this.AnimNewRadioButton.Name = "AnimNewRadioButton";
         this.AnimNewRadioButton.Size = new System.Drawing.Size(81, 17);
         this.AnimNewRadioButton.TabIndex = 16;
         this.AnimNewRadioButton.TabStop = true;
         this.AnimNewRadioButton.Text = "Create New";
         this.AnimNewRadioButton.UseVisualStyleBackColor = true;
         this.AnimNewRadioButton.Click += new System.EventHandler(this.OnAnimPerforceRadio);
         // 
         // label1
         // 
         this.label1.AutoSize = true;
         this.label1.Location = new System.Drawing.Point(6, 72);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(125, 13);
         this.label1.TabIndex = 15;
         this.label1.Text = "Files that will be Updated";
         // 
         // Helpbutton1
         // 
         this.Helpbutton1.Location = new System.Drawing.Point(637, 11);
         this.Helpbutton1.Name = "Helpbutton1";
         this.Helpbutton1.Size = new System.Drawing.Size(70, 23);
         this.Helpbutton1.TabIndex = 16;
         this.Helpbutton1.Text = "Help";
         this.Helpbutton1.UseVisualStyleBackColor = true;
         this.Helpbutton1.Click += new System.EventHandler(this.Helpbutton1_Click);
         // 
         // GR2ObjectImporter
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.ClientSize = new System.Drawing.Size(881, 592);
         this.Controls.Add(this.Helpbutton1);
         this.Controls.Add(this.groupBox1);
         this.Controls.Add(this.ObjectNameTextBox);
         this.Controls.Add(this.TemplateListBox);
         this.Controls.Add(this.PathTextBox);
         this.Controls.Add(this.CancelImportButton);
         this.Controls.Add(this.ImportButton);
         this.Name = "GR2ObjectImporter";
         this.Text = "GR2ObjectImporter";
         this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.GR2ObjectImporter_FormClosing);
         this.groupBox1.ResumeLayout(false);
         this.groupBox1.PerformLayout();
         this.tabControl1.ResumeLayout(false);
         this.BasicTabPage.ResumeLayout(false);
         this.tableLayoutPanel1.ResumeLayout(false);
         this.tableLayoutPanel1.PerformLayout();
         this.DetailsTabPage.ResumeLayout(false);
         this.DetailsTabPage.PerformLayout();
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private System.Windows.Forms.Button ImportButton;
      private System.Windows.Forms.Button CancelImportButton;
      private System.Windows.Forms.TextBox PathTextBox;
      private System.Windows.Forms.ListBox TemplateListBox;
      private System.Windows.Forms.TextBox ObjectNameTextBox;
      private System.Windows.Forms.CheckBox AddToPerforceCheckBox;
      private System.Windows.Forms.ListBox InPerforceListBox;
      private System.Windows.Forms.Label OtherAddLabel;
      private System.Windows.Forms.Label label2;
      private System.Windows.Forms.CheckedListBox ToAddListBox;
      private System.Windows.Forms.ListBox ManditoryChangeListBox;
      private System.Windows.Forms.GroupBox groupBox1;
      private System.Windows.Forms.ListBox ErrorListBox;
      private System.Windows.Forms.Label label3;
      private System.Windows.Forms.Label label1;
      private System.Windows.Forms.RadioButton AnimExistingRadioButton;
      private System.Windows.Forms.RadioButton AnimNewRadioButton;
      private System.Windows.Forms.TextBox AnimUnitFileTextBox;
      private System.Windows.Forms.Label label4;
      private System.Windows.Forms.Button PickAnimFileButton;
      private System.Windows.Forms.CheckBox DontCheckInCheckBox;
      private System.Windows.Forms.CheckBox CheckOutAfterSubmitCheckBox;
      private System.Windows.Forms.TabControl tabControl1;
      private System.Windows.Forms.TabPage BasicTabPage;
      private System.Windows.Forms.TabPage DetailsTabPage;
      private System.Windows.Forms.ListBox BasicErrorListBox;
      private System.Windows.Forms.TableLayoutPanel tableLayoutPanel1;
      private System.Windows.Forms.Button TemplateIgnoreButton;
      private System.Windows.Forms.Button BasicRetryButton;
      private System.Windows.Forms.TextBox TemplateFileNameTextBox;
      private System.Windows.Forms.Button AdvancedErrorsButton;
      private System.Windows.Forms.TextBox TemplateDescriptionTextBox;
      private System.Windows.Forms.Button Helpbutton1;
   }
}
