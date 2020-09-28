namespace EditorCore.Controls.Micro
{
   partial class CollectionChooser
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
         this.EnumListBox = new System.Windows.Forms.ListBox();
         this.CollectionListBox = new System.Windows.Forms.ListBox();
         this.AddButton = new System.Windows.Forms.Button();
         this.RemoveButton = new System.Windows.Forms.Button();
         this.splitContainer1 = new System.Windows.Forms.SplitContainer();
         this.UpButton = new System.Windows.Forms.Button();
         this.DownButton = new System.Windows.Forms.Button();
         this.splitContainer1.Panel1.SuspendLayout();
         this.splitContainer1.Panel2.SuspendLayout();
         this.splitContainer1.SuspendLayout();
         this.SuspendLayout();
         // 
         // EnumListBox
         // 
         this.EnumListBox.Dock = System.Windows.Forms.DockStyle.Fill;
         this.EnumListBox.FormattingEnabled = true;
         this.EnumListBox.Location = new System.Drawing.Point(0, 0);
         this.EnumListBox.Name = "EnumListBox";
         this.EnumListBox.SelectionMode = System.Windows.Forms.SelectionMode.MultiExtended;
         this.EnumListBox.Size = new System.Drawing.Size(130, 225);
         this.EnumListBox.TabIndex = 0;
         // 
         // CollectionListBox
         // 
         this.CollectionListBox.Dock = System.Windows.Forms.DockStyle.Fill;
         this.CollectionListBox.FormattingEnabled = true;
         this.CollectionListBox.Location = new System.Drawing.Point(0, 0);
         this.CollectionListBox.Name = "CollectionListBox";
         this.CollectionListBox.SelectionMode = System.Windows.Forms.SelectionMode.MultiExtended;
         this.CollectionListBox.Size = new System.Drawing.Size(146, 225);
         this.CollectionListBox.TabIndex = 1;
         // 
         // AddButton
         // 
         this.AddButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
         this.AddButton.Location = new System.Drawing.Point(3, 230);
         this.AddButton.Name = "AddButton";
         this.AddButton.Size = new System.Drawing.Size(50, 23);
         this.AddButton.TabIndex = 2;
         this.AddButton.Text = "Add";
         this.AddButton.UseVisualStyleBackColor = true;
         this.AddButton.Click += new System.EventHandler(this.AddButton_Click);
         // 
         // RemoveButton
         // 
         this.RemoveButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
         this.RemoveButton.Location = new System.Drawing.Point(137, 230);
         this.RemoveButton.Name = "RemoveButton";
         this.RemoveButton.Size = new System.Drawing.Size(55, 23);
         this.RemoveButton.TabIndex = 3;
         this.RemoveButton.Text = "Remove";
         this.RemoveButton.UseVisualStyleBackColor = true;
         this.RemoveButton.Click += new System.EventHandler(this.RemoveButton_Click);
         // 
         // splitContainer1
         // 
         this.splitContainer1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.splitContainer1.Location = new System.Drawing.Point(3, 3);
         this.splitContainer1.Name = "splitContainer1";
         // 
         // splitContainer1.Panel1
         // 
         this.splitContainer1.Panel1.Controls.Add(this.EnumListBox);
         // 
         // splitContainer1.Panel2
         // 
         this.splitContainer1.Panel2.Controls.Add(this.CollectionListBox);
         this.splitContainer1.Size = new System.Drawing.Size(280, 225);
         this.splitContainer1.SplitterDistance = 130;
         this.splitContainer1.TabIndex = 4;
         // 
         // UpButton
         // 
         this.UpButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
         this.UpButton.Location = new System.Drawing.Point(198, 230);
         this.UpButton.Name = "UpButton";
         this.UpButton.Size = new System.Drawing.Size(29, 23);
         this.UpButton.TabIndex = 5;
         this.UpButton.Text = "Up";
         this.UpButton.UseVisualStyleBackColor = true;
         this.UpButton.Click += new System.EventHandler(this.UpButton_Click);
         // 
         // DownButton
         // 
         this.DownButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
         this.DownButton.Location = new System.Drawing.Point(233, 230);
         this.DownButton.Name = "DownButton";
         this.DownButton.Size = new System.Drawing.Size(50, 23);
         this.DownButton.TabIndex = 6;
         this.DownButton.Text = "Down";
         this.DownButton.UseVisualStyleBackColor = true;
         this.DownButton.Click += new System.EventHandler(this.DownButton_Click);
         // 
         // CollectionChooser
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.DownButton);
         this.Controls.Add(this.UpButton);
         this.Controls.Add(this.splitContainer1);
         this.Controls.Add(this.RemoveButton);
         this.Controls.Add(this.AddButton);
         this.Name = "CollectionChooser";
         this.Size = new System.Drawing.Size(286, 253);
         this.splitContainer1.Panel1.ResumeLayout(false);
         this.splitContainer1.Panel2.ResumeLayout(false);
         this.splitContainer1.ResumeLayout(false);
         this.ResumeLayout(false);

      }

      #endregion

      private System.Windows.Forms.ListBox EnumListBox;
      private System.Windows.Forms.ListBox CollectionListBox;
      private System.Windows.Forms.Button AddButton;
      private System.Windows.Forms.Button RemoveButton;
      private System.Windows.Forms.SplitContainer splitContainer1;
      private System.Windows.Forms.Button UpButton;
      private System.Windows.Forms.Button DownButton;
   }
}
