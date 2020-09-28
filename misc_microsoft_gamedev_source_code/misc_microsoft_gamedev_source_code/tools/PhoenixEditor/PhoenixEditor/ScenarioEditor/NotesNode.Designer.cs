namespace PhoenixEditor.ScenarioEditor
{
   partial class NotesNode
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
         this.NoteTextBox = new System.Windows.Forms.TextBox();
         this.TitleTextBox = new System.Windows.Forms.TextBox();
         this.ResizePanel = new System.Windows.Forms.Panel();
         this.MinMaxPanel = new System.Windows.Forms.Panel();
         this.SuspendLayout();
         // 
         // NoteTextBox
         // 
         this.NoteTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.NoteTextBox.BackColor = System.Drawing.SystemColors.Info;
         this.NoteTextBox.Location = new System.Drawing.Point(3, 31);
         this.NoteTextBox.Multiline = true;
         this.NoteTextBox.Name = "NoteTextBox";
         this.NoteTextBox.Size = new System.Drawing.Size(335, 239);
         this.NoteTextBox.TabIndex = 0;
         // 
         // TitleTextBox
         // 
         this.TitleTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.TitleTextBox.BackColor = System.Drawing.SystemColors.Info;
         this.TitleTextBox.Font = new System.Drawing.Font("Microsoft Sans Serif", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
         this.TitleTextBox.Location = new System.Drawing.Point(3, 3);
         this.TitleTextBox.Name = "TitleTextBox";
         this.TitleTextBox.Size = new System.Drawing.Size(334, 22);
         this.TitleTextBox.TabIndex = 1;
         // 
         // ResizePanel
         // 
         this.ResizePanel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
         this.ResizePanel.BackColor = System.Drawing.SystemColors.InactiveCaptionText;
         this.ResizePanel.Location = new System.Drawing.Point(318, 260);
         this.ResizePanel.Name = "ResizePanel";
         this.ResizePanel.Size = new System.Drawing.Size(20, 13);
         this.ResizePanel.TabIndex = 2;
         // 
         // MinMaxPanel
         // 
         this.MinMaxPanel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
         this.MinMaxPanel.BackColor = System.Drawing.SystemColors.InactiveCaptionText;
         this.MinMaxPanel.Location = new System.Drawing.Point(317, 0);
         this.MinMaxPanel.Name = "MinMaxPanel";
         this.MinMaxPanel.Size = new System.Drawing.Size(21, 11);
         this.MinMaxPanel.TabIndex = 3;
         // 
         // NotesNode
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.BackColor = System.Drawing.SystemColors.Window;
         this.Controls.Add(this.MinMaxPanel);
         this.Controls.Add(this.ResizePanel);
         this.Controls.Add(this.TitleTextBox);
         this.Controls.Add(this.NoteTextBox);
         this.Name = "NotesNode";
         this.Size = new System.Drawing.Size(341, 273);
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private System.Windows.Forms.TextBox NoteTextBox;
      private System.Windows.Forms.TextBox TitleTextBox;
      private System.Windows.Forms.Panel ResizePanel;
      private System.Windows.Forms.Panel MinMaxPanel;
   }
}
