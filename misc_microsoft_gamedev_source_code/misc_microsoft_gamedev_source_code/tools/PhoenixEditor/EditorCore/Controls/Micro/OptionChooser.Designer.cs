namespace EditorCore.Controls.Micro
{
   partial class OptionChooser
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
         this.OptionListBox = new System.Windows.Forms.CheckedListBox();
         this.SelcectAllButton = new System.Windows.Forms.Button();
         this.ClearButton = new System.Windows.Forms.Button();
         this.SuspendLayout();
         // 
         // OptionListBox
         // 
         this.OptionListBox.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.OptionListBox.CheckOnClick = true;
         this.OptionListBox.FormattingEnabled = true;
         this.OptionListBox.Location = new System.Drawing.Point(3, 3);
         this.OptionListBox.MultiColumn = true;
         this.OptionListBox.Name = "OptionListBox";
         this.OptionListBox.Size = new System.Drawing.Size(174, 229);
         this.OptionListBox.TabIndex = 0;
         this.OptionListBox.SelectedIndexChanged += new System.EventHandler(this.OptionListBox_SelectedIndexChanged);
         // 
         // SelcectAllButton
         // 
         this.SelcectAllButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
         this.SelcectAllButton.Location = new System.Drawing.Point(3, 239);
         this.SelcectAllButton.Name = "SelcectAllButton";
         this.SelcectAllButton.Size = new System.Drawing.Size(73, 23);
         this.SelcectAllButton.TabIndex = 1;
         this.SelcectAllButton.Text = "Select All";
         this.SelcectAllButton.UseVisualStyleBackColor = true;
         this.SelcectAllButton.Click += new System.EventHandler(this.SelcectAllButton_Click);
         // 
         // ClearButton
         // 
         this.ClearButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
         this.ClearButton.Location = new System.Drawing.Point(82, 239);
         this.ClearButton.Name = "ClearButton";
         this.ClearButton.Size = new System.Drawing.Size(78, 23);
         this.ClearButton.TabIndex = 2;
         this.ClearButton.Text = "Clear All";
         this.ClearButton.UseVisualStyleBackColor = true;
         this.ClearButton.Click += new System.EventHandler(this.ClearButton_Click);
         // 
         // OptionChooser
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.ClearButton);
         this.Controls.Add(this.SelcectAllButton);
         this.Controls.Add(this.OptionListBox);
         this.Name = "OptionChooser";
         this.Size = new System.Drawing.Size(180, 266);
         this.ResumeLayout(false);

      }

      #endregion

      private System.Windows.Forms.CheckedListBox OptionListBox;
      private System.Windows.Forms.Button SelcectAllButton;
      private System.Windows.Forms.Button ClearButton;
   }
}
