namespace PhoenixEditor
{
   partial class OpenProject
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
         System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(OpenProject));
         this.imageList1 = new System.Windows.Forms.ImageList(this.components);
         this.imageList2 = new System.Windows.Forms.ImageList(this.components);
         this.DirectoryTreeView = new System.Windows.Forms.TreeView();
         this.OKButton = new System.Windows.Forms.Button();
         this.CancelOpenButton = new System.Windows.Forms.Button();
         this.textBox1 = new System.Windows.Forms.TextBox();
         this.comboBox1 = new System.Windows.Forms.ComboBox();
         this.groupBox1 = new System.Windows.Forms.GroupBox();
         this.label1 = new System.Windows.Forms.Label();
         this.radioButton2 = new System.Windows.Forms.RadioButton();
         this.radioButton1 = new System.Windows.Forms.RadioButton();
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
         this.DirectoryTreeView.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.DirectoryTreeView.ImageIndex = 0;
         this.DirectoryTreeView.ImageList = this.imageList1;
         this.DirectoryTreeView.Location = new System.Drawing.Point(12, 12);
         this.DirectoryTreeView.Name = "DirectoryTreeView";
         this.DirectoryTreeView.SelectedImageIndex = 0;
         this.DirectoryTreeView.Size = new System.Drawing.Size(271, 293);
         this.DirectoryTreeView.TabIndex = 17;
         this.DirectoryTreeView.DoubleClick += new System.EventHandler(this.DirectoryTreeView_DoubleClick);
         this.DirectoryTreeView.AfterSelect += new System.Windows.Forms.TreeViewEventHandler(this.DirectoryTreeView_AfterSelect);
         // 
         // OKButton
         // 
         this.OKButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
         this.OKButton.DialogResult = System.Windows.Forms.DialogResult.OK;
         this.OKButton.Location = new System.Drawing.Point(356, 364);
         this.OKButton.Name = "OKButton";
         this.OKButton.Size = new System.Drawing.Size(75, 23);
         this.OKButton.TabIndex = 18;
         this.OKButton.Text = "OK";
         this.OKButton.UseVisualStyleBackColor = true;
         this.OKButton.Click += new System.EventHandler(this.OKButton_Click);
         // 
         // CancelOpenButton
         // 
         this.CancelOpenButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
         this.CancelOpenButton.DialogResult = System.Windows.Forms.DialogResult.Cancel;
         this.CancelOpenButton.Location = new System.Drawing.Point(437, 364);
         this.CancelOpenButton.Name = "CancelOpenButton";
         this.CancelOpenButton.Size = new System.Drawing.Size(75, 23);
         this.CancelOpenButton.TabIndex = 19;
         this.CancelOpenButton.Text = "Cancel";
         this.CancelOpenButton.UseVisualStyleBackColor = true;
         this.CancelOpenButton.Click += new System.EventHandler(this.CancelButton_Click);
         // 
         // textBox1
         // 
         this.textBox1.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.textBox1.Location = new System.Drawing.Point(12, 311);
         this.textBox1.Multiline = true;
         this.textBox1.Name = "textBox1";
         this.textBox1.ReadOnly = true;
         this.textBox1.Size = new System.Drawing.Size(271, 47);
         this.textBox1.TabIndex = 20;
         // 
         // comboBox1
         // 
         this.comboBox1.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
         this.comboBox1.FormattingEnabled = true;
         this.comboBox1.Items.AddRange(new object[] {
            "Origional Size",
            "512x512",
            "640x640",
            "768x768",
            "896x896",
            "1024x1024",
            "1280x1280"});
         this.comboBox1.Location = new System.Drawing.Point(19, 19);
         this.comboBox1.Name = "comboBox1";
         this.comboBox1.Size = new System.Drawing.Size(175, 21);
         this.comboBox1.TabIndex = 22;
         this.comboBox1.SelectedIndexChanged += new System.EventHandler(this.comboBox1_SelectedIndexChanged);
         // 
         // groupBox1
         // 
         this.groupBox1.Controls.Add(this.label1);
         this.groupBox1.Controls.Add(this.radioButton2);
         this.groupBox1.Controls.Add(this.radioButton1);
         this.groupBox1.Controls.Add(this.comboBox1);
         this.groupBox1.Location = new System.Drawing.Point(303, 12);
         this.groupBox1.Name = "groupBox1";
         this.groupBox1.Size = new System.Drawing.Size(214, 149);
         this.groupBox1.TabIndex = 23;
         this.groupBox1.TabStop = false;
         this.groupBox1.Text = "Open As..";
         // 
         // label1
         // 
         this.label1.AutoSize = true;
         this.label1.Location = new System.Drawing.Point(6, 76);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(74, 13);
         this.label1.TabIndex = 25;
         this.label1.Text = "For Resizing : ";
         // 
         // radioButton2
         // 
         this.radioButton2.AutoSize = true;
         this.radioButton2.Location = new System.Drawing.Point(39, 115);
         this.radioButton2.Name = "radioButton2";
         this.radioButton2.Size = new System.Drawing.Size(57, 17);
         this.radioButton2.TabIndex = 24;
         this.radioButton2.Text = "Resize";
         this.radioButton2.UseVisualStyleBackColor = true;
         this.radioButton2.CheckedChanged += new System.EventHandler(this.radioButton2_CheckedChanged);
         // 
         // radioButton1
         // 
         this.radioButton1.AutoSize = true;
         this.radioButton1.Checked = true;
         this.radioButton1.Location = new System.Drawing.Point(39, 92);
         this.radioButton1.Name = "radioButton1";
         this.radioButton1.Size = new System.Drawing.Size(170, 17);
         this.radioButton1.TabIndex = 23;
         this.radioButton1.TabStop = true;
         this.radioButton1.Text = "Center (creates border / crops)";
         this.radioButton1.UseVisualStyleBackColor = true;
         this.radioButton1.CheckedChanged += new System.EventHandler(this.radioButton1_CheckedChanged);
         // 
         // OpenProject
         // 
         this.AcceptButton = this.OKButton;
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.ClientSize = new System.Drawing.Size(529, 400);
         this.Controls.Add(this.groupBox1);
         this.Controls.Add(this.textBox1);
         this.Controls.Add(this.CancelOpenButton);
         this.Controls.Add(this.OKButton);
         this.Controls.Add(this.DirectoryTreeView);
         this.Name = "OpenProject";
         this.Text = "Open Scenario";
         this.Load += new System.EventHandler(this.OpenProject_Load);
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
      private System.Windows.Forms.Button CancelOpenButton;
      private System.Windows.Forms.TextBox textBox1;
      private System.Windows.Forms.ComboBox comboBox1;
      private System.Windows.Forms.GroupBox groupBox1;
      private System.Windows.Forms.Label label1;
      private System.Windows.Forms.RadioButton radioButton2;
      private System.Windows.Forms.RadioButton radioButton1;
   }
}