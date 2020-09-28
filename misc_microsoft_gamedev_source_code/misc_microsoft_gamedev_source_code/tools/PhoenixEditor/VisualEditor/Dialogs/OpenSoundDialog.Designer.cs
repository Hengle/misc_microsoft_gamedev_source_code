namespace VisualEditor.Dialogs
{
   partial class OpenSoundDialog
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
         this.SoundTreeView = new System.Windows.Forms.TreeView();
         this.OpenButton = new System.Windows.Forms.Button();
         this.CancelLoadButton = new System.Windows.Forms.Button();
         this.label1 = new System.Windows.Forms.Label();
         this.textBox1 = new System.Windows.Forms.TextBox();
         this.tabControl1 = new System.Windows.Forms.TabControl();
         this.tabPage1 = new System.Windows.Forms.TabPage();
         this.tabPage2 = new System.Windows.Forms.TabPage();
         this.SoundNamesListBox = new System.Windows.Forms.ListBox();
         this.FilterTextBox = new System.Windows.Forms.TextBox();
         this.stopButton = new System.Windows.Forms.Button();
         this.playButton = new System.Windows.Forms.Button();
         this.tabControl1.SuspendLayout();
         this.tabPage1.SuspendLayout();
         this.tabPage2.SuspendLayout();
         this.SuspendLayout();
         // 
         // SoundTreeView
         // 
         this.SoundTreeView.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.SoundTreeView.HideSelection = false;
         this.SoundTreeView.Location = new System.Drawing.Point(0, 3);
         this.SoundTreeView.Name = "SoundTreeView";
         this.SoundTreeView.Size = new System.Drawing.Size(332, 407);
         this.SoundTreeView.TabIndex = 0;
         this.SoundTreeView.AfterSelect += new System.Windows.Forms.TreeViewEventHandler(this.SoundTreeView_AfterSelect);
         // 
         // OpenButton
         // 
         this.OpenButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
         this.OpenButton.DialogResult = System.Windows.Forms.DialogResult.OK;
         this.OpenButton.Location = new System.Drawing.Point(201, 507);
         this.OpenButton.Name = "OpenButton";
         this.OpenButton.Size = new System.Drawing.Size(74, 23);
         this.OpenButton.TabIndex = 4;
         this.OpenButton.Text = "Open";
         this.OpenButton.UseVisualStyleBackColor = true;
         this.OpenButton.Click += new System.EventHandler(this.OpenButton_Click);
         // 
         // CancelLoadButton
         // 
         this.CancelLoadButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
         this.CancelLoadButton.DialogResult = System.Windows.Forms.DialogResult.Cancel;
         this.CancelLoadButton.Location = new System.Drawing.Point(281, 507);
         this.CancelLoadButton.Name = "CancelLoadButton";
         this.CancelLoadButton.Size = new System.Drawing.Size(74, 23);
         this.CancelLoadButton.TabIndex = 5;
         this.CancelLoadButton.Text = "Cancel";
         this.CancelLoadButton.UseVisualStyleBackColor = true;
         this.CancelLoadButton.Click += new System.EventHandler(this.CancelLoadButton_Click);
         // 
         // label1
         // 
         this.label1.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
         this.label1.AutoSize = true;
         this.label1.Location = new System.Drawing.Point(9, 453);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(70, 13);
         this.label1.TabIndex = 0;
         this.label1.Text = "Sound name:";
         // 
         // textBox1
         // 
         this.textBox1.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.textBox1.Location = new System.Drawing.Point(12, 469);
         this.textBox1.Name = "textBox1";
         this.textBox1.Size = new System.Drawing.Size(343, 20);
         this.textBox1.TabIndex = 1;
         // 
         // tabControl1
         // 
         this.tabControl1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.tabControl1.Controls.Add(this.tabPage1);
         this.tabControl1.Controls.Add(this.tabPage2);
         this.tabControl1.Location = new System.Drawing.Point(12, 12);
         this.tabControl1.Name = "tabControl1";
         this.tabControl1.SelectedIndex = 0;
         this.tabControl1.Size = new System.Drawing.Size(343, 436);
         this.tabControl1.TabIndex = 0;
         this.tabControl1.TabStop = false;
         // 
         // tabPage1
         // 
         this.tabPage1.Controls.Add(this.SoundTreeView);
         this.tabPage1.Location = new System.Drawing.Point(4, 22);
         this.tabPage1.Name = "tabPage1";
         this.tabPage1.Padding = new System.Windows.Forms.Padding(3);
         this.tabPage1.Size = new System.Drawing.Size(335, 410);
         this.tabPage1.TabIndex = 0;
         this.tabPage1.Text = "Tree";
         this.tabPage1.UseVisualStyleBackColor = true;
         // 
         // tabPage2
         // 
         this.tabPage2.Controls.Add(this.SoundNamesListBox);
         this.tabPage2.Controls.Add(this.FilterTextBox);
         this.tabPage2.Location = new System.Drawing.Point(4, 22);
         this.tabPage2.Name = "tabPage2";
         this.tabPage2.Padding = new System.Windows.Forms.Padding(3);
         this.tabPage2.Size = new System.Drawing.Size(336, 397);
         this.tabPage2.TabIndex = 1;
         this.tabPage2.Text = "List";
         this.tabPage2.UseVisualStyleBackColor = true;
         // 
         // SoundNamesListBox
         // 
         this.SoundNamesListBox.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.SoundNamesListBox.FormattingEnabled = true;
         this.SoundNamesListBox.Location = new System.Drawing.Point(0, 29);
         this.SoundNamesListBox.Name = "SoundNamesListBox";
         this.SoundNamesListBox.Size = new System.Drawing.Size(333, 355);
         this.SoundNamesListBox.TabIndex = 1;
         this.SoundNamesListBox.MouseDoubleClick += new System.Windows.Forms.MouseEventHandler(this.SoundNamesListBox_MouseDoubleClick);
         this.SoundNamesListBox.SelectedIndexChanged += new System.EventHandler(this.SoundNamesListBox_SelectedIndexChanged);
         // 
         // FilterTextBox
         // 
         this.FilterTextBox.AccessibleRole = System.Windows.Forms.AccessibleRole.OutlineButton;
         this.FilterTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.FilterTextBox.Location = new System.Drawing.Point(0, 3);
         this.FilterTextBox.Name = "FilterTextBox";
         this.FilterTextBox.Size = new System.Drawing.Size(333, 20);
         this.FilterTextBox.TabIndex = 0;
         this.FilterTextBox.TextChanged += new System.EventHandler(this.FilterTextBox_TextChanged);
         // 
         // stopButton
         // 
         this.stopButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
         this.stopButton.Location = new System.Drawing.Point(59, 495);
         this.stopButton.Name = "stopButton";
         this.stopButton.Size = new System.Drawing.Size(41, 35);
         this.stopButton.TabIndex = 3;
         this.stopButton.Text = "Stop";
         this.stopButton.UseVisualStyleBackColor = true;
         this.stopButton.Click += new System.EventHandler(this.stopButton_Click);
         // 
         // playButton
         // 
         this.playButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
         this.playButton.Location = new System.Drawing.Point(12, 495);
         this.playButton.Name = "playButton";
         this.playButton.Size = new System.Drawing.Size(41, 35);
         this.playButton.TabIndex = 2;
         this.playButton.Text = "Play";
         this.playButton.UseVisualStyleBackColor = true;
         this.playButton.Click += new System.EventHandler(this.playButton_Click);
         // 
         // OpenSoundDialog
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.ClientSize = new System.Drawing.Size(367, 541);
         this.Controls.Add(this.stopButton);
         this.Controls.Add(this.playButton);
         this.Controls.Add(this.tabControl1);
         this.Controls.Add(this.textBox1);
         this.Controls.Add(this.label1);
         this.Controls.Add(this.CancelLoadButton);
         this.Controls.Add(this.OpenButton);
         this.MinimumSize = new System.Drawing.Size(280, 250);
         this.Name = "OpenSoundDialog";
         this.Text = "Open";
         this.tabControl1.ResumeLayout(false);
         this.tabPage1.ResumeLayout(false);
         this.tabPage2.ResumeLayout(false);
         this.tabPage2.PerformLayout();
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private System.Windows.Forms.TreeView SoundTreeView;
      private System.Windows.Forms.Button OpenButton;
      private System.Windows.Forms.Button CancelLoadButton;
      private System.Windows.Forms.Label label1;
      private System.Windows.Forms.TextBox textBox1;
      private System.Windows.Forms.TabControl tabControl1;
      private System.Windows.Forms.TabPage tabPage1;
      private System.Windows.Forms.TabPage tabPage2;
      private System.Windows.Forms.TextBox FilterTextBox;
      private System.Windows.Forms.ListBox SoundNamesListBox;
      private System.Windows.Forms.Button playButton;
      private System.Windows.Forms.Button stopButton;
   }
}