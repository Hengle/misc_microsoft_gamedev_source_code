namespace EditorCore.Controls.Micro
{
   partial class ListBuilder
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
         this.UpButton = new System.Windows.Forms.Button();
         this.AddButton = new System.Windows.Forms.Button();
         this.RemoveButton = new System.Windows.Forms.Button();
         this.DownButton = new System.Windows.Forms.Button();
         this.SuspendLayout();
         // 
         // EnumListBox
         // 
         this.EnumListBox.FormattingEnabled = true;
         this.EnumListBox.Location = new System.Drawing.Point(3, 12);
         this.EnumListBox.Name = "EnumListBox";
         this.EnumListBox.Size = new System.Drawing.Size(181, 355);
         this.EnumListBox.TabIndex = 0;
         // 
         // CollectionListBox
         // 
         this.CollectionListBox.FormattingEnabled = true;
         this.CollectionListBox.Location = new System.Drawing.Point(216, 13);
         this.CollectionListBox.Name = "CollectionListBox";
         this.CollectionListBox.Size = new System.Drawing.Size(181, 355);
         this.CollectionListBox.TabIndex = 1;
         // 
         // UpButton
         // 
         this.UpButton.Location = new System.Drawing.Point(297, 374);
         this.UpButton.Name = "UpButton";
         this.UpButton.Size = new System.Drawing.Size(38, 23);
         this.UpButton.TabIndex = 2;
         this.UpButton.Text = "Up";
         this.UpButton.UseVisualStyleBackColor = true;
         // 
         // AddButton
         // 
         this.AddButton.Location = new System.Drawing.Point(48, 373);
         this.AddButton.Name = "AddButton";
         this.AddButton.Size = new System.Drawing.Size(75, 23);
         this.AddButton.TabIndex = 3;
         this.AddButton.Text = "Add";
         this.AddButton.UseVisualStyleBackColor = true;
         this.AddButton.Click += new System.EventHandler(this.AddButton_Click);
         // 
         // RemoveButton
         // 
         this.RemoveButton.Location = new System.Drawing.Point(216, 374);
         this.RemoveButton.Name = "RemoveButton";
         this.RemoveButton.Size = new System.Drawing.Size(75, 23);
         this.RemoveButton.TabIndex = 4;
         this.RemoveButton.Text = "Remove";
         this.RemoveButton.UseVisualStyleBackColor = true;
         this.RemoveButton.Click += new System.EventHandler(this.RemoveButton_Click);
         // 
         // DownButton
         // 
         this.DownButton.Location = new System.Drawing.Point(341, 374);
         this.DownButton.Name = "DownButton";
         this.DownButton.Size = new System.Drawing.Size(56, 23);
         this.DownButton.TabIndex = 5;
         this.DownButton.Text = "Down";
         this.DownButton.UseVisualStyleBackColor = true;
         // 
         // ListBuilder
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.DownButton);
         this.Controls.Add(this.RemoveButton);
         this.Controls.Add(this.AddButton);
         this.Controls.Add(this.UpButton);
         this.Controls.Add(this.CollectionListBox);
         this.Controls.Add(this.EnumListBox);
         this.Name = "ListBuilder";
         this.Size = new System.Drawing.Size(409, 405);
         this.ResumeLayout(false);

      }

      #endregion

      private System.Windows.Forms.ListBox EnumListBox;
      private System.Windows.Forms.ListBox CollectionListBox;
      private System.Windows.Forms.Button UpButton;
      private System.Windows.Forms.Button AddButton;
      private System.Windows.Forms.Button RemoveButton;
      private System.Windows.Forms.Button DownButton;
   }
}
