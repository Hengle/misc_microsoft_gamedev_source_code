namespace PhoenixEditor
{
   partial class SaveProject
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
         System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(SaveProject));
         this.imageList1 = new System.Windows.Forms.ImageList(this.components);
         this.imageList2 = new System.Windows.Forms.ImageList(this.components);
         this.DirectoryTreeView = new System.Windows.Forms.TreeView();
         this.OKButton = new System.Windows.Forms.Button();
         this.CancelSaveButton = new System.Windows.Forms.Button();
         this.label2 = new System.Windows.Forms.Label();
         this.ProjectNameTextBox = new System.Windows.Forms.TextBox();
         this.label1 = new System.Windows.Forms.Label();
         this.BrowseButton = new System.Windows.Forms.Button();
         this.groupBox1 = new System.Windows.Forms.GroupBox();
         this.linkLabel1 = new System.Windows.Forms.LinkLabel();
         this.groupBox1.SuspendLayout();
         this.SuspendLayout();
         // 
         // imageList1
         // 
         this.imageList1.ImageStream = ((System.Windows.Forms.ImageListStreamer)(resources.GetObject("imageList1.ImageStream")));
         this.imageList1.TransparentColor = System.Drawing.Color.Transparent;
         this.imageList1.Images.SetKeyName(0, "folder.bmp");
         this.imageList1.Images.SetKeyName(1, "phx16.bmp");
         // 
         // imageList2
         // 
         this.imageList2.ImageStream = ((System.Windows.Forms.ImageListStreamer)(resources.GetObject("imageList2.ImageStream")));
         this.imageList2.TransparentColor = System.Drawing.Color.Transparent;
         this.imageList2.Images.SetKeyName(0, "phx48.bmp");
         // 
         // DirectoryTreeView
         // 
         this.DirectoryTreeView.ImageIndex = 0;
         this.DirectoryTreeView.ImageList = this.imageList1;
         this.DirectoryTreeView.Location = new System.Drawing.Point(12, 12);
         this.DirectoryTreeView.Name = "DirectoryTreeView";
         this.DirectoryTreeView.SelectedImageIndex = 0;
         this.DirectoryTreeView.Size = new System.Drawing.Size(684, 203);
         this.DirectoryTreeView.TabIndex = 14;
         this.DirectoryTreeView.AfterSelect += new System.Windows.Forms.TreeViewEventHandler(this.DirectoryTreeView_AfterSelect);
         // 
         // OKButton
         // 
         this.OKButton.Location = new System.Drawing.Point(18, 366);
         this.OKButton.Name = "OKButton";
         this.OKButton.Size = new System.Drawing.Size(75, 23);
         this.OKButton.TabIndex = 12;
         this.OKButton.Text = "OK";
         this.OKButton.UseVisualStyleBackColor = true;
         this.OKButton.Click += new System.EventHandler(this.OKButton_Click);
         // 
         // CancelSaveButton
         // 
         this.CancelSaveButton.DialogResult = System.Windows.Forms.DialogResult.Cancel;
         this.CancelSaveButton.Location = new System.Drawing.Point(621, 366);
         this.CancelSaveButton.Name = "CancelSaveButton";
         this.CancelSaveButton.Size = new System.Drawing.Size(75, 23);
         this.CancelSaveButton.TabIndex = 13;
         this.CancelSaveButton.Text = "Cancel";
         this.CancelSaveButton.UseVisualStyleBackColor = true;
         this.CancelSaveButton.Click += new System.EventHandler(this.CancelButton_Click);
         // 
         // label2
         // 
         this.label2.AutoSize = true;
         this.label2.Location = new System.Drawing.Point(15, 337);
         this.label2.Name = "label2";
         this.label2.Size = new System.Drawing.Size(71, 13);
         this.label2.TabIndex = 11;
         this.label2.Text = "Project Name";
         // 
         // ProjectNameTextBox
         // 
         this.ProjectNameTextBox.Location = new System.Drawing.Point(106, 337);
         this.ProjectNameTextBox.Name = "ProjectNameTextBox";
         this.ProjectNameTextBox.Size = new System.Drawing.Size(548, 20);
         this.ProjectNameTextBox.TabIndex = 10;
         this.ProjectNameTextBox.TextChanged += new System.EventHandler(this.ProjectNameTextBox_TextChanged);
         // 
         // label1
         // 
         this.label1.AutoSize = true;
         this.label1.Font = new System.Drawing.Font("Microsoft Sans Serif", 9.75F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
         this.label1.Location = new System.Drawing.Point(6, 47);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(47, 16);
         this.label1.TabIndex = 15;
         this.label1.Text = "Path: ";
         // 
         // BrowseButton
         // 
         this.BrowseButton.Location = new System.Drawing.Point(660, 337);
         this.BrowseButton.Name = "BrowseButton";
         this.BrowseButton.Size = new System.Drawing.Size(36, 23);
         this.BrowseButton.TabIndex = 16;
         this.BrowseButton.Text = "...";
         this.BrowseButton.UseVisualStyleBackColor = true;
         this.BrowseButton.Click += new System.EventHandler(this.BrowseButton_Click);
         // 
         // groupBox1
         // 
         this.groupBox1.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.groupBox1.Controls.Add(this.label1);
         this.groupBox1.Location = new System.Drawing.Point(12, 231);
         this.groupBox1.Name = "groupBox1";
         this.groupBox1.Size = new System.Drawing.Size(684, 100);
         this.groupBox1.TabIndex = 17;
         this.groupBox1.TabStop = false;
         this.groupBox1.Text = "Project Save Location";
         // 
         // linkLabel1
         // 
         this.linkLabel1.AutoSize = true;
         this.linkLabel1.Location = new System.Drawing.Point(506, 218);
         this.linkLabel1.Name = "linkLabel1";
         this.linkLabel1.Size = new System.Drawing.Size(190, 13);
         this.linkLabel1.TabIndex = 18;
         this.linkLabel1.TabStop = true;
         this.linkLabel1.Text = "help:  how to create a new major folder";
         this.linkLabel1.LinkClicked += new System.Windows.Forms.LinkLabelLinkClickedEventHandler(this.linkLabel1_LinkClicked);
         // 
         // SaveProject
         // 
         this.AcceptButton = this.OKButton;
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.ClientSize = new System.Drawing.Size(708, 398);
         this.Controls.Add(this.linkLabel1);
         this.Controls.Add(this.groupBox1);
         this.Controls.Add(this.BrowseButton);
         this.Controls.Add(this.DirectoryTreeView);
         this.Controls.Add(this.OKButton);
         this.Controls.Add(this.CancelSaveButton);
         this.Controls.Add(this.label2);
         this.Controls.Add(this.ProjectNameTextBox);
         this.Name = "SaveProject";
         this.Text = "Save Scenario";
         this.Load += new System.EventHandler(this.SaveProject_Load);
         this.groupBox1.ResumeLayout(false);
         this.groupBox1.PerformLayout();
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private System.Windows.Forms.ImageList imageList1;
      private System.Windows.Forms.ImageList imageList2;
      private System.Windows.Forms.TreeView DirectoryTreeView;
      private System.Windows.Forms.Button OKButton;
      private System.Windows.Forms.Button CancelSaveButton;
      private System.Windows.Forms.Label label2;
      private System.Windows.Forms.TextBox ProjectNameTextBox;
      private System.Windows.Forms.Label label1;
      private System.Windows.Forms.Button BrowseButton;
      private System.Windows.Forms.GroupBox groupBox1;
      private System.Windows.Forms.LinkLabel linkLabel1;
   }
}