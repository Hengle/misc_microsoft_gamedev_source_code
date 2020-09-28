namespace AkFilePackager
{
    partial class EditModeForm
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
            System.Windows.Forms.Label label1;
            System.Windows.Forms.Label label2;
            System.Windows.Forms.Label label3;
            this.textBoxOutputFilePath = new System.Windows.Forms.TextBox();
            this.textBoxDefaultBlockSize = new System.Windows.Forms.TextBox();
            this.menuStrip1 = new System.Windows.Forms.MenuStrip();
            this.fileToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.loadInfoFileToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator2 = new System.Windows.Forms.ToolStripSeparator();
            this.openToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.closeLayoutToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.saveToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.saveAsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator3 = new System.Windows.Forms.ToolStripSeparator();
            this.removeAllMissingFilesToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator1 = new System.Windows.Forms.ToolStripSeparator();
            this.generateFilePackageToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator4 = new System.Windows.Forms.ToolStripSeparator();
            this.quitToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.label4 = new System.Windows.Forms.Label();
            this.textBoxInfoFile = new System.Windows.Forms.TextBox();
            this.listViewFiles = new AkFilePackager.ReorderingListView();
            this.file = new System.Windows.Forms.ColumnHeader();
            this.type = new System.Windows.Forms.ColumnHeader();
            this.language = new System.Windows.Forms.ColumnHeader();
            this.status = new System.Windows.Forms.ColumnHeader();
            label1 = new System.Windows.Forms.Label();
            label2 = new System.Windows.Forms.Label();
            label3 = new System.Windows.Forms.Label();
            this.menuStrip1.SuspendLayout();
            this.SuspendLayout();
            // 
            // label1
            // 
            label1.AutoSize = true;
            label1.Location = new System.Drawing.Point(12, 54);
            label1.Name = "label1";
            label1.Size = new System.Drawing.Size(87, 13);
            label1.TabIndex = 1;
            label1.Text = "Output file name:";
            // 
            // label2
            // 
            label2.AutoSize = true;
            label2.Location = new System.Drawing.Point(12, 83);
            label2.Name = "label2";
            label2.Size = new System.Drawing.Size(94, 13);
            label2.TabIndex = 2;
            label2.Text = "Default block size:";
            // 
            // label3
            // 
            label3.AutoSize = true;
            label3.Location = new System.Drawing.Point(12, 110);
            label3.Name = "label3";
            label3.Size = new System.Drawing.Size(53, 13);
            label3.TabIndex = 5;
            label3.Text = "File order:";
            // 
            // textBoxOutputFilePath
            // 
            this.textBoxOutputFilePath.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.textBoxOutputFilePath.Location = new System.Drawing.Point(112, 54);
            this.textBoxOutputFilePath.MaxLength = 256;
            this.textBoxOutputFilePath.Name = "textBoxOutputFilePath";
            this.textBoxOutputFilePath.Size = new System.Drawing.Size(259, 20);
            this.textBoxOutputFilePath.TabIndex = 0;
            this.textBoxOutputFilePath.Validating += new System.ComponentModel.CancelEventHandler(this.textBoxOutputFilePath_Validating);
            // 
            // textBoxDefaultBlockSize
            // 
            this.textBoxDefaultBlockSize.Location = new System.Drawing.Point(112, 80);
            this.textBoxDefaultBlockSize.Name = "textBoxDefaultBlockSize";
            this.textBoxDefaultBlockSize.Size = new System.Drawing.Size(100, 20);
            this.textBoxDefaultBlockSize.TabIndex = 3;
            this.textBoxDefaultBlockSize.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this.textBoxDefaultBlockSize_KeyPress);
            this.textBoxDefaultBlockSize.Validating += new System.ComponentModel.CancelEventHandler(this.textBoxDefaultBlockSize_Validating);
            // 
            // menuStrip1
            // 
            this.menuStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.fileToolStripMenuItem});
            this.menuStrip1.Location = new System.Drawing.Point(0, 0);
            this.menuStrip1.Name = "menuStrip1";
            this.menuStrip1.RenderMode = System.Windows.Forms.ToolStripRenderMode.System;
            this.menuStrip1.Size = new System.Drawing.Size(383, 24);
            this.menuStrip1.TabIndex = 7;
            this.menuStrip1.Text = "menuStrip1";
            this.menuStrip1.MenuActivate += new System.EventHandler(this.menuStrip1_MenuActivate);
            // 
            // fileToolStripMenuItem
            // 
            this.fileToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.loadInfoFileToolStripMenuItem,
            this.toolStripSeparator2,
            this.openToolStripMenuItem,
            this.closeLayoutToolStripMenuItem,
            this.saveToolStripMenuItem,
            this.saveAsToolStripMenuItem,
            this.toolStripSeparator3,
            this.removeAllMissingFilesToolStripMenuItem,
            this.toolStripSeparator1,
            this.generateFilePackageToolStripMenuItem,
            this.toolStripSeparator4,
            this.quitToolStripMenuItem});
            this.fileToolStripMenuItem.Name = "fileToolStripMenuItem";
            this.fileToolStripMenuItem.Size = new System.Drawing.Size(35, 20);
            this.fileToolStripMenuItem.Text = "File";
            // 
            // loadInfoFileToolStripMenuItem
            // 
            this.loadInfoFileToolStripMenuItem.Name = "loadInfoFileToolStripMenuItem";
            this.loadInfoFileToolStripMenuItem.Size = new System.Drawing.Size(199, 22);
            this.loadInfoFileToolStripMenuItem.Text = "Load Info file...";
            this.loadInfoFileToolStripMenuItem.Click += new System.EventHandler(this.menuFileLoadInfoFile_Click);
            // 
            // toolStripSeparator2
            // 
            this.toolStripSeparator2.Name = "toolStripSeparator2";
            this.toolStripSeparator2.Size = new System.Drawing.Size(196, 6);
            // 
            // openToolStripMenuItem
            // 
            this.openToolStripMenuItem.Name = "openToolStripMenuItem";
            this.openToolStripMenuItem.Size = new System.Drawing.Size(199, 22);
            this.openToolStripMenuItem.Text = "Open Layout...";
            this.openToolStripMenuItem.Click += new System.EventHandler(this.menuFileOpen_Click);
            // 
            // closeLayoutToolStripMenuItem
            // 
            this.closeLayoutToolStripMenuItem.BackColor = System.Drawing.SystemColors.Control;
            this.closeLayoutToolStripMenuItem.Name = "closeLayoutToolStripMenuItem";
            this.closeLayoutToolStripMenuItem.Size = new System.Drawing.Size(199, 22);
            this.closeLayoutToolStripMenuItem.Text = "Close Layout";
            this.closeLayoutToolStripMenuItem.Click += new System.EventHandler(this.closeLayoutToolStripMenuItem_Click);
            // 
            // saveToolStripMenuItem
            // 
            this.saveToolStripMenuItem.Name = "saveToolStripMenuItem";
            this.saveToolStripMenuItem.Size = new System.Drawing.Size(199, 22);
            this.saveToolStripMenuItem.Text = "Save Layout";
            this.saveToolStripMenuItem.Click += new System.EventHandler(this.menuFileSave_Click);
            // 
            // saveAsToolStripMenuItem
            // 
            this.saveAsToolStripMenuItem.Name = "saveAsToolStripMenuItem";
            this.saveAsToolStripMenuItem.Size = new System.Drawing.Size(199, 22);
            this.saveAsToolStripMenuItem.Text = "Save Layout as...";
            this.saveAsToolStripMenuItem.Click += new System.EventHandler(this.menuFileSaveAs_Click);
            // 
            // toolStripSeparator3
            // 
            this.toolStripSeparator3.Name = "toolStripSeparator3";
            this.toolStripSeparator3.Size = new System.Drawing.Size(196, 6);
            // 
            // removeAllMissingFilesToolStripMenuItem
            // 
            this.removeAllMissingFilesToolStripMenuItem.Name = "removeAllMissingFilesToolStripMenuItem";
            this.removeAllMissingFilesToolStripMenuItem.Size = new System.Drawing.Size(199, 22);
            this.removeAllMissingFilesToolStripMenuItem.Text = "Remove All Missing Files";
            this.removeAllMissingFilesToolStripMenuItem.Click += new System.EventHandler(this.removeAllMissingFilesToolStripMenuItem_Click);
            // 
            // toolStripSeparator1
            // 
            this.toolStripSeparator1.Name = "toolStripSeparator1";
            this.toolStripSeparator1.Size = new System.Drawing.Size(196, 6);
            // 
            // generateFilePackageToolStripMenuItem
            // 
            this.generateFilePackageToolStripMenuItem.Name = "generateFilePackageToolStripMenuItem";
            this.generateFilePackageToolStripMenuItem.Size = new System.Drawing.Size(199, 22);
            this.generateFilePackageToolStripMenuItem.Text = "Generate File Package";
            this.generateFilePackageToolStripMenuItem.Click += new System.EventHandler(this.generateFilePackageToolStripMenuItem_Click);
            // 
            // toolStripSeparator4
            // 
            this.toolStripSeparator4.Name = "toolStripSeparator4";
            this.toolStripSeparator4.Size = new System.Drawing.Size(196, 6);
            // 
            // quitToolStripMenuItem
            // 
            this.quitToolStripMenuItem.Name = "quitToolStripMenuItem";
            this.quitToolStripMenuItem.Size = new System.Drawing.Size(199, 22);
            this.quitToolStripMenuItem.Text = "Quit";
            this.quitToolStripMenuItem.Click += new System.EventHandler(this.quitToolStripMenuItem_Click);
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Location = new System.Drawing.Point(12, 28);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(80, 13);
            this.label4.TabIndex = 10;
            this.label4.Text = "Current info file:";
            // 
            // textBoxInfoFile
            // 
            this.textBoxInfoFile.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.textBoxInfoFile.Location = new System.Drawing.Point(112, 28);
            this.textBoxInfoFile.Name = "textBoxInfoFile";
            this.textBoxInfoFile.ReadOnly = true;
            this.textBoxInfoFile.Size = new System.Drawing.Size(259, 20);
            this.textBoxInfoFile.TabIndex = 11;
            // 
            // listViewFiles
            // 
            this.listViewFiles.AllowDrop = true;
            this.listViewFiles.AllowRowReorder = true;
            this.listViewFiles.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.listViewFiles.AutoArrange = false;
            this.listViewFiles.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.file,
            this.type,
            this.language,
            this.status});
            this.listViewFiles.FullRowSelect = true;
            this.listViewFiles.HeaderStyle = System.Windows.Forms.ColumnHeaderStyle.Nonclickable;
            this.listViewFiles.LabelWrap = false;
            this.listViewFiles.Location = new System.Drawing.Point(15, 130);
            this.listViewFiles.Name = "listViewFiles";
            this.listViewFiles.ShowGroups = false;
            this.listViewFiles.Size = new System.Drawing.Size(356, 324);
            this.listViewFiles.TabIndex = 9;
            this.listViewFiles.UseCompatibleStateImageBehavior = false;
            this.listViewFiles.View = System.Windows.Forms.View.Details;
            // 
            // file
            // 
            this.file.Text = "File";
            this.file.Width = 120;
            // 
            // type
            // 
            this.type.Text = "File Type";
            this.type.Width = 90;
            // 
            // language
            // 
            this.language.Text = "Language";
            this.language.Width = 80;
            // 
            // status
            // 
            this.status.Text = "Status";
            // 
            // EditModeForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.SystemColors.Control;
            this.ClientSize = new System.Drawing.Size(383, 466);
            this.Controls.Add(this.textBoxInfoFile);
            this.Controls.Add(this.label4);
            this.Controls.Add(this.listViewFiles);
            this.Controls.Add(label3);
            this.Controls.Add(this.textBoxDefaultBlockSize);
            this.Controls.Add(label2);
            this.Controls.Add(label1);
            this.Controls.Add(this.textBoxOutputFilePath);
            this.Controls.Add(this.menuStrip1);
            this.ForeColor = System.Drawing.SystemColors.ControlText;
            this.MainMenuStrip = this.menuStrip1;
            this.Name = "EditModeForm";
            this.Text = "File package editor";
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.EditMode_FormClosing);
            this.menuStrip1.ResumeLayout(false);
            this.menuStrip1.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.TextBox textBoxOutputFilePath;
        private System.Windows.Forms.TextBox textBoxDefaultBlockSize;
        private System.Windows.Forms.MenuStrip menuStrip1;
        private System.Windows.Forms.ToolStripMenuItem fileToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem openToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem saveToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem saveAsToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem quitToolStripMenuItem;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator1;
        private System.Windows.Forms.ToolStripMenuItem loadInfoFileToolStripMenuItem;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator2;
        private ReorderingListView listViewFiles;
        private System.Windows.Forms.ColumnHeader file;
        private System.Windows.Forms.ToolStripMenuItem removeAllMissingFilesToolStripMenuItem;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator3;
        private System.Windows.Forms.ColumnHeader status;
        private System.Windows.Forms.ToolStripMenuItem generateFilePackageToolStripMenuItem;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator4;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.TextBox textBoxInfoFile;
        private System.Windows.Forms.ColumnHeader type;
        private System.Windows.Forms.ToolStripMenuItem closeLayoutToolStripMenuItem;
        private System.Windows.Forms.ColumnHeader language;
    }
}

