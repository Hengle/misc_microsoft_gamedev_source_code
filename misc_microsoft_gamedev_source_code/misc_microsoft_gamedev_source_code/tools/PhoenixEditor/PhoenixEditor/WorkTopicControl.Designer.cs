namespace PhoenixEditor
{
   partial class WorkTopicControl
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
         System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(WorkTopicControl));
         this.TopicNameLabel = new System.Windows.Forms.Label();
         this.TopicDescriptionLabel = new System.Windows.Forms.Label();
         this.StatusLabel = new System.Windows.Forms.Label();
         this.SourceActionButton = new System.Windows.Forms.Button();
         this.imageList1 = new System.Windows.Forms.ImageList(this.components);
         this.DontSaveCheckBox = new System.Windows.Forms.CheckBox();
         this.OverwriteButton = new System.Windows.Forms.Button();
         this.CheckInButton = new System.Windows.Forms.Button();
         this.RevertButton = new System.Windows.Forms.Button();
         this.FileDescriptionLabel = new System.Windows.Forms.Label();
         this.WriteStatusLabel = new System.Windows.Forms.Label();
         this.OutOfDateLabel = new System.Windows.Forms.Label();
         this.SuspendLayout();
         // 
         // TopicNameLabel
         // 
         this.TopicNameLabel.AutoSize = true;
         this.TopicNameLabel.Font = new System.Drawing.Font("Microsoft Sans Serif", 15.75F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
         this.TopicNameLabel.Location = new System.Drawing.Point(136, 10);
         this.TopicNameLabel.Name = "TopicNameLabel";
         this.TopicNameLabel.Size = new System.Drawing.Size(130, 25);
         this.TopicNameLabel.TabIndex = 0;
         this.TopicNameLabel.Text = "TopicName";
         // 
         // TopicDescriptionLabel
         // 
         this.TopicDescriptionLabel.AutoSize = true;
         this.TopicDescriptionLabel.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
         this.TopicDescriptionLabel.Location = new System.Drawing.Point(153, 35);
         this.TopicDescriptionLabel.Name = "TopicDescriptionLabel";
         this.TopicDescriptionLabel.Size = new System.Drawing.Size(89, 20);
         this.TopicDescriptionLabel.TabIndex = 1;
         this.TopicDescriptionLabel.Text = "Description";
         // 
         // StatusLabel
         // 
         this.StatusLabel.AutoSize = true;
         this.StatusLabel.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
         this.StatusLabel.Location = new System.Drawing.Point(294, 14);
         this.StatusLabel.Name = "StatusLabel";
         this.StatusLabel.Size = new System.Drawing.Size(79, 20);
         this.StatusLabel.TabIndex = 2;
         this.StatusLabel.Text = "status....";
         // 
         // SourceActionButton
         // 
         this.SourceActionButton.Location = new System.Drawing.Point(568, 10);
         this.SourceActionButton.Name = "SourceActionButton";
         this.SourceActionButton.Size = new System.Drawing.Size(72, 23);
         this.SourceActionButton.TabIndex = 3;
         this.SourceActionButton.Text = "Check Out";
         this.SourceActionButton.UseVisualStyleBackColor = true;
         this.SourceActionButton.Click += new System.EventHandler(this.CheckOutButton_Click);
         // 
         // imageList1
         // 
         this.imageList1.ImageStream = ((System.Windows.Forms.ImageListStreamer)(resources.GetObject("imageList1.ImageStream")));
         this.imageList1.TransparentColor = System.Drawing.Color.Transparent;
         this.imageList1.Images.SetKeyName(0, "stateNotSet.bmp");
         this.imageList1.Images.SetKeyName(1, "busted.bmp");
         this.imageList1.Images.SetKeyName(2, "");
         this.imageList1.Images.SetKeyName(3, "");
         this.imageList1.Images.SetKeyName(4, "");
         this.imageList1.Images.SetKeyName(5, "Warning.bmp");
         this.imageList1.Images.SetKeyName(6, "nosource.bmp");
         this.imageList1.Images.SetKeyName(7, "nosource.bmp");
         // 
         // DontSaveCheckBox
         // 
         this.DontSaveCheckBox.AutoSize = true;
         this.DontSaveCheckBox.Font = new System.Drawing.Font("Microsoft Sans Serif", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
         this.DontSaveCheckBox.Location = new System.Drawing.Point(12, 28);
         this.DontSaveCheckBox.Name = "DontSaveCheckBox";
         this.DontSaveCheckBox.Size = new System.Drawing.Size(99, 20);
         this.DontSaveCheckBox.TabIndex = 6;
         this.DontSaveCheckBox.Text = "Do not save";
         this.DontSaveCheckBox.UseVisualStyleBackColor = true;
         this.DontSaveCheckBox.CheckedChanged += new System.EventHandler(this.DontSaveCheckBox_CheckedChanged);
         // 
         // OverwriteButton
         // 
         this.OverwriteButton.Location = new System.Drawing.Point(536, 39);
         this.OverwriteButton.Name = "OverwriteButton";
         this.OverwriteButton.Size = new System.Drawing.Size(172, 23);
         this.OverwriteButton.TabIndex = 7;
         this.OverwriteButton.Text = "ADVANCED: Overwrite files";
         this.OverwriteButton.UseVisualStyleBackColor = true;
         this.OverwriteButton.Visible = false;
         this.OverwriteButton.Click += new System.EventHandler(this.OverwriteButton_Click);
         // 
         // CheckInButton
         // 
         this.CheckInButton.Location = new System.Drawing.Point(646, 10);
         this.CheckInButton.Name = "CheckInButton";
         this.CheckInButton.Size = new System.Drawing.Size(62, 23);
         this.CheckInButton.TabIndex = 8;
         this.CheckInButton.Text = "Check In";
         this.CheckInButton.UseVisualStyleBackColor = true;
         this.CheckInButton.Visible = false;
         this.CheckInButton.Click += new System.EventHandler(this.CheckInButton_Click);
         // 
         // RevertButton
         // 
         this.RevertButton.Location = new System.Drawing.Point(714, 10);
         this.RevertButton.Name = "RevertButton";
         this.RevertButton.Size = new System.Drawing.Size(68, 23);
         this.RevertButton.TabIndex = 9;
         this.RevertButton.Text = "Revert";
         this.RevertButton.UseVisualStyleBackColor = true;
         this.RevertButton.Click += new System.EventHandler(this.RevertButton_Click);
         // 
         // FileDescriptionLabel
         // 
         this.FileDescriptionLabel.AutoSize = true;
         this.FileDescriptionLabel.Location = new System.Drawing.Point(156, 57);
         this.FileDescriptionLabel.Name = "FileDescriptionLabel";
         this.FileDescriptionLabel.Size = new System.Drawing.Size(76, 13);
         this.FileDescriptionLabel.TabIndex = 10;
         this.FileDescriptionLabel.Text = "FileDescription";
         // 
         // WriteStatusLabel
         // 
         this.WriteStatusLabel.AutoSize = true;
         this.WriteStatusLabel.ForeColor = System.Drawing.Color.Green;
         this.WriteStatusLabel.Location = new System.Drawing.Point(9, 51);
         this.WriteStatusLabel.Name = "WriteStatusLabel";
         this.WriteStatusLabel.Size = new System.Drawing.Size(87, 13);
         this.WriteStatusLabel.TabIndex = 11;
         this.WriteStatusLabel.Text = "+Local Overwrite";
         // 
         // OutOfDateLabel
         // 
         this.OutOfDateLabel.AutoSize = true;
         this.OutOfDateLabel.ForeColor = System.Drawing.Color.Red;
         this.OutOfDateLabel.Location = new System.Drawing.Point(9, 10);
         this.OutOfDateLabel.Name = "OutOfDateLabel";
         this.OutOfDateLabel.Size = new System.Drawing.Size(89, 13);
         this.OutOfDateLabel.TabIndex = 12;
         this.OutOfDateLabel.Text = "Files Out of Date!";
         // 
         // WorkTopicControl
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.BackColor = System.Drawing.SystemColors.InactiveCaptionText;
         this.Controls.Add(this.OutOfDateLabel);
         this.Controls.Add(this.WriteStatusLabel);
         this.Controls.Add(this.FileDescriptionLabel);
         this.Controls.Add(this.RevertButton);
         this.Controls.Add(this.CheckInButton);
         this.Controls.Add(this.OverwriteButton);
         this.Controls.Add(this.DontSaveCheckBox);
         this.Controls.Add(this.SourceActionButton);
         this.Controls.Add(this.StatusLabel);
         this.Controls.Add(this.TopicDescriptionLabel);
         this.Controls.Add(this.TopicNameLabel);
         this.Name = "WorkTopicControl";
         this.Size = new System.Drawing.Size(786, 75);
         this.Load += new System.EventHandler(this.WorkTopicControl_Load);
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private System.Windows.Forms.Label TopicNameLabel;
      private System.Windows.Forms.Label TopicDescriptionLabel;
      private System.Windows.Forms.Label StatusLabel;
      private System.Windows.Forms.Button SourceActionButton;
      private System.Windows.Forms.ImageList imageList1;
      private System.Windows.Forms.CheckBox DontSaveCheckBox;
      private System.Windows.Forms.Button OverwriteButton;
      private System.Windows.Forms.Button CheckInButton;
      private System.Windows.Forms.Button RevertButton;
      private System.Windows.Forms.Label FileDescriptionLabel;
      private System.Windows.Forms.Label WriteStatusLabel;
      private System.Windows.Forms.Label OutOfDateLabel;
   }
}
