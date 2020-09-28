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
         this.zipFolderButton = new System.Windows.Forms.Button();
         this.label3 = new System.Windows.Forms.Label();
         this.zipFolderTextBox = new System.Windows.Forms.MaskedTextBox();
         this.logList = new System.Windows.Forms.ListBox();
         this.label4 = new System.Windows.Forms.Label();
         this.label5 = new System.Windows.Forms.Label();
         this.buildListView = new System.Windows.Forms.ListView();
         this.cancelButton = new System.Windows.Forms.Button();
         this.pickBatchFileButton = new System.Windows.Forms.Button();
         this.label6 = new System.Windows.Forms.Label();
         this.batchFileTextBox = new System.Windows.Forms.MaskedTextBox();
         this.deleteFilesCheckBox = new System.Windows.Forms.CheckBox();
         this.label7 = new System.Windows.Forms.Label();
         this.lastBuildTextBox = new System.Windows.Forms.MaskedTextBox();
         this.groupBox1 = new System.Windows.Forms.GroupBox();
         this.groupBox2 = new System.Windows.Forms.GroupBox();
         this.groupBox3 = new System.Windows.Forms.GroupBox();
         this.SuspendLayout();
         // 
         // buildFolderTextBox
         // 
         this.buildFolderTextBox.Location = new System.Drawing.Point(93, 378);
         this.buildFolderTextBox.Name = "buildFolderTextBox";
         this.buildFolderTextBox.Size = new System.Drawing.Size(211, 20);
         this.buildFolderTextBox.TabIndex = 4;
         this.buildFolderTextBox.TextChanged += new System.EventHandler(this.buildFolderTextBox_TextChanged);
         // 
         // label1
         // 
         this.label1.AutoSize = true;
         this.label1.Location = new System.Drawing.Point(9, 383);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(65, 13);
         this.label1.TabIndex = 2;
         this.label1.Text = "Build Folder:";
         // 
         // localFolderTextBox
         // 
         this.localFolderTextBox.Location = new System.Drawing.Point(93, 406);
         this.localFolderTextBox.Name = "localFolderTextBox";
         this.localFolderTextBox.Size = new System.Drawing.Size(211, 20);
         this.localFolderTextBox.TabIndex = 6;
         this.localFolderTextBox.TextChanged += new System.EventHandler(this.localFolderTextBox_TextChanged);
         // 
         // label2
         // 
         this.label2.AutoSize = true;
         this.label2.Location = new System.Drawing.Point(9, 409);
         this.label2.Name = "label2";
         this.label2.Size = new System.Drawing.Size(68, 13);
         this.label2.TabIndex = 2;
         this.label2.Text = "Local Folder:";
         // 
         // goButton
         // 
         this.goButton.Location = new System.Drawing.Point(546, 26);
         this.goButton.Name = "goButton";
         this.goButton.Size = new System.Drawing.Size(73, 23);
         this.goButton.TabIndex = 2;
         this.goButton.Text = "Get Build";
         this.goButton.UseVisualStyleBackColor = true;
         this.goButton.Click += new System.EventHandler(this.goButton_Click);
         // 
         // buildFolderButton
         // 
         this.buildFolderButton.Location = new System.Drawing.Point(305, 377);
         this.buildFolderButton.Name = "buildFolderButton";
         this.buildFolderButton.Size = new System.Drawing.Size(24, 24);
         this.buildFolderButton.TabIndex = 5;
         this.buildFolderButton.Text = "...";
         this.buildFolderButton.UseVisualStyleBackColor = true;
         this.buildFolderButton.Click += new System.EventHandler(this.buildFolderButton_Click);
         // 
         // localFolderButton
         // 
         this.localFolderButton.Location = new System.Drawing.Point(305, 403);
         this.localFolderButton.Name = "localFolderButton";
         this.localFolderButton.Size = new System.Drawing.Size(24, 24);
         this.localFolderButton.TabIndex = 7;
         this.localFolderButton.Text = "...";
         this.localFolderButton.UseVisualStyleBackColor = true;
         this.localFolderButton.Click += new System.EventHandler(this.localFolderButton_Click);
         // 
         // zipFolderButton
         // 
         this.zipFolderButton.Location = new System.Drawing.Point(305, 428);
         this.zipFolderButton.Name = "zipFolderButton";
         this.zipFolderButton.Size = new System.Drawing.Size(24, 24);
         this.zipFolderButton.TabIndex = 9;
         this.zipFolderButton.Text = "...";
         this.zipFolderButton.UseVisualStyleBackColor = true;
         this.zipFolderButton.Click += new System.EventHandler(this.zipFolderButton_Click);
         // 
         // label3
         // 
         this.label3.AutoSize = true;
         this.label3.Location = new System.Drawing.Point(11, 434);
         this.label3.Name = "label3";
         this.label3.Size = new System.Drawing.Size(66, 13);
         this.label3.TabIndex = 12;
         this.label3.Text = "7-Zip Folder:";
         // 
         // zipFolderTextBox
         // 
         this.zipFolderTextBox.Location = new System.Drawing.Point(93, 431);
         this.zipFolderTextBox.Name = "zipFolderTextBox";
         this.zipFolderTextBox.Size = new System.Drawing.Size(211, 20);
         this.zipFolderTextBox.TabIndex = 8;
         this.zipFolderTextBox.TextChanged += new System.EventHandler(this.zipFolderTextBox_TextChanged);
         // 
         // logList
         // 
         this.logList.FormattingEnabled = true;
         this.logList.Location = new System.Drawing.Point(12, 247);
         this.logList.Name = "logList";
         this.logList.Size = new System.Drawing.Size(528, 121);
         this.logList.TabIndex = 3;
         // 
         // label4
         // 
         this.label4.AutoSize = true;
         this.label4.Location = new System.Drawing.Point(12, 9);
         this.label4.Name = "label4";
         this.label4.Size = new System.Drawing.Size(82, 13);
         this.label4.TabIndex = 15;
         this.label4.Text = "Builds Archives:";
         // 
         // label5
         // 
         this.label5.AutoSize = true;
         this.label5.Location = new System.Drawing.Point(12, 231);
         this.label5.Name = "label5";
         this.label5.Size = new System.Drawing.Size(28, 13);
         this.label5.TabIndex = 16;
         this.label5.Text = "Log:";
         // 
         // buildListView
         // 
         this.buildListView.Location = new System.Drawing.Point(12, 25);
         this.buildListView.MultiSelect = false;
         this.buildListView.Name = "buildListView";
         this.buildListView.Size = new System.Drawing.Size(528, 203);
         this.buildListView.TabIndex = 1;
         this.buildListView.UseCompatibleStateImageBehavior = false;
         this.buildListView.View = System.Windows.Forms.View.List;
         this.buildListView.MouseDoubleClick += new System.Windows.Forms.MouseEventHandler(this.buildListView_MouseDoubleClick);
         this.buildListView.ItemSelectionChanged += new System.Windows.Forms.ListViewItemSelectionChangedEventHandler(this.buildListView_ItemSelectionChanged);
         // 
         // cancelButton
         // 
         this.cancelButton.Location = new System.Drawing.Point(546, 55);
         this.cancelButton.Name = "cancelButton";
         this.cancelButton.Size = new System.Drawing.Size(73, 23);
         this.cancelButton.TabIndex = 17;
         this.cancelButton.Text = "Cancel";
         this.cancelButton.UseVisualStyleBackColor = true;
         this.cancelButton.Click += new System.EventHandler(this.cancelButton_Click);
         // 
         // pickBatchFileButton
         // 
         this.pickBatchFileButton.Location = new System.Drawing.Point(305, 452);
         this.pickBatchFileButton.Name = "pickBatchFileButton";
         this.pickBatchFileButton.Size = new System.Drawing.Size(24, 24);
         this.pickBatchFileButton.TabIndex = 19;
         this.pickBatchFileButton.Text = "...";
         this.pickBatchFileButton.UseVisualStyleBackColor = true;
         this.pickBatchFileButton.Click += new System.EventHandler(this.pickBatchFileButton_Click);
         // 
         // label6
         // 
         this.label6.AutoSize = true;
         this.label6.Location = new System.Drawing.Point(9, 458);
         this.label6.Name = "label6";
         this.label6.Size = new System.Drawing.Size(81, 13);
         this.label6.TabIndex = 20;
         this.label6.Text = "Post Batch File:";
         // 
         // batchFileTextBox
         // 
         this.batchFileTextBox.Location = new System.Drawing.Point(93, 455);
         this.batchFileTextBox.Name = "batchFileTextBox";
         this.batchFileTextBox.Size = new System.Drawing.Size(211, 20);
         this.batchFileTextBox.TabIndex = 18;
         this.batchFileTextBox.TextChanged += new System.EventHandler(this.batchFileTextBox_TextChanged);
         // 
         // deleteFilesCheckBox
         // 
         this.deleteFilesCheckBox.AutoSize = true;
         this.deleteFilesCheckBox.Location = new System.Drawing.Point(338, 409);
         this.deleteFilesCheckBox.Name = "deleteFilesCheckBox";
         this.deleteFilesCheckBox.Size = new System.Drawing.Size(154, 17);
         this.deleteFilesCheckBox.TabIndex = 21;
         this.deleteFilesCheckBox.Text = "Clear out folder before get?";
         this.deleteFilesCheckBox.UseVisualStyleBackColor = true;
         this.deleteFilesCheckBox.CheckedChanged += new System.EventHandler(this.deleteFilesCheckBox_CheckedChanged);
         // 
         // label7
         // 
         this.label7.AutoSize = true;
         this.label7.Location = new System.Drawing.Point(338, 382);
         this.label7.Name = "label7";
         this.label7.Size = new System.Drawing.Size(90, 13);
         this.label7.TabIndex = 22;
         this.label7.Text = "Last build copied:";
         // 
         // lastBuildTextBox
         // 
         this.lastBuildTextBox.Location = new System.Drawing.Point(427, 378);
         this.lastBuildTextBox.Name = "lastBuildTextBox";
         this.lastBuildTextBox.ReadOnly = true;
         this.lastBuildTextBox.Size = new System.Drawing.Size(192, 20);
         this.lastBuildTextBox.TabIndex = 23;
         // 
         // groupBox1
         // 
         this.groupBox1.Location = new System.Drawing.Point(672, 25);
         this.groupBox1.Name = "groupBox1";
         this.groupBox1.Size = new System.Drawing.Size(200, 143);
         this.groupBox1.TabIndex = 24;
         this.groupBox1.TabStop = false;
         this.groupBox1.Text = "Xbox 360\'s";
         // 
         // groupBox2
         // 
         this.groupBox2.Location = new System.Drawing.Point(672, 202);
         this.groupBox2.Name = "groupBox2";
         this.groupBox2.Size = new System.Drawing.Size(200, 100);
         this.groupBox2.TabIndex = 25;
         this.groupBox2.TabStop = false;
         this.groupBox2.Text = "Settings";
         // 
         // groupBox3
         // 
         this.groupBox3.Location = new System.Drawing.Point(672, 334);
         this.groupBox3.Name = "groupBox3";
         this.groupBox3.Size = new System.Drawing.Size(200, 100);
         this.groupBox3.TabIndex = 26;
         this.groupBox3.TabStop = false;
         this.groupBox3.Text = "Local Builds";
         // 
         // Form1
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.ClientSize = new System.Drawing.Size(985, 631);
         this.Controls.Add(this.groupBox3);
         this.Controls.Add(this.groupBox2);
         this.Controls.Add(this.groupBox1);
         this.Controls.Add(this.lastBuildTextBox);
         this.Controls.Add(this.label7);
         this.Controls.Add(this.deleteFilesCheckBox);
         this.Controls.Add(this.pickBatchFileButton);
         this.Controls.Add(this.label6);
         this.Controls.Add(this.batchFileTextBox);
         this.Controls.Add(this.cancelButton);
         this.Controls.Add(this.buildListView);
         this.Controls.Add(this.label5);
         this.Controls.Add(this.label4);
         this.Controls.Add(this.logList);
         this.Controls.Add(this.zipFolderButton);
         this.Controls.Add(this.label3);
         this.Controls.Add(this.zipFolderTextBox);
         this.Controls.Add(this.localFolderButton);
         this.Controls.Add(this.buildFolderButton);
         this.Controls.Add(this.goButton);
         this.Controls.Add(this.label2);
         this.Controls.Add(this.label1);
         this.Controls.Add(this.localFolderTextBox);
         this.Controls.Add(this.buildFolderTextBox);
         this.Name = "Form1";
         this.Text = "xlab";
         this.Load += new System.EventHandler(this.Form1_Load);
         this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.Form1_FormClosing);
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private System.Windows.Forms.MaskedTextBox buildFolderTextBox;
      private System.Windows.Forms.Label label1;
      private System.Windows.Forms.MaskedTextBox localFolderTextBox;
      private System.Windows.Forms.Label label2;
      private System.Windows.Forms.Button goButton;
      private System.Windows.Forms.Button buildFolderButton;
      private System.Windows.Forms.Button localFolderButton;
      private System.Windows.Forms.Button zipFolderButton;
      private System.Windows.Forms.Label label3;
      private System.Windows.Forms.MaskedTextBox zipFolderTextBox;
      private System.Windows.Forms.ListBox logList;
      private System.Windows.Forms.Label label4;
      private System.Windows.Forms.Label label5;
      private System.Windows.Forms.ListView buildListView;
      private System.Windows.Forms.Button cancelButton;
      private System.Windows.Forms.Button pickBatchFileButton;
      private System.Windows.Forms.Label label6;
      private System.Windows.Forms.MaskedTextBox batchFileTextBox;
      private System.Windows.Forms.CheckBox deleteFilesCheckBox;
      private System.Windows.Forms.Label label7;
      private System.Windows.Forms.MaskedTextBox lastBuildTextBox;
      private System.Windows.Forms.GroupBox groupBox1;
      private System.Windows.Forms.GroupBox groupBox2;
      private System.Windows.Forms.GroupBox groupBox3;
   }
}

