namespace EditorCore
{
   partial class ColorPickerLauncher
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
         this.PickColorButton = new System.Windows.Forms.Button();
         this.DescriptionLabel = new System.Windows.Forms.Label();
         this.SuspendLayout();
         // 
         // PickColorButton
         // 
         this.PickColorButton.BackColor = System.Drawing.Color.White;
         this.PickColorButton.Dock = System.Windows.Forms.DockStyle.Left;
         this.PickColorButton.Location = new System.Drawing.Point(0, 0);
         this.PickColorButton.Margin = new System.Windows.Forms.Padding(0);
         this.PickColorButton.Name = "PickColorButton";
         this.PickColorButton.Size = new System.Drawing.Size(65, 19);
         this.PickColorButton.TabIndex = 0;
         this.PickColorButton.UseVisualStyleBackColor = false;
         this.PickColorButton.DragOver += new System.Windows.Forms.DragEventHandler(this.PickColorButton_DragOver);
         this.PickColorButton.Click += new System.EventHandler(this.PickColorButton_Click);
         this.PickColorButton.MouseMove += new System.Windows.Forms.MouseEventHandler(this.PickColorButton_MouseMove);
         this.PickColorButton.DragDrop += new System.Windows.Forms.DragEventHandler(this.PickColorButton_DragDrop);
         this.PickColorButton.DragEnter += new System.Windows.Forms.DragEventHandler(this.PickColorButton_DragEnter);
         // 
         // DescriptionLabel
         // 
         this.DescriptionLabel.AutoSize = true;
         this.DescriptionLabel.Location = new System.Drawing.Point(69, 0);
         this.DescriptionLabel.Name = "DescriptionLabel";
         this.DescriptionLabel.Size = new System.Drawing.Size(35, 13);
         this.DescriptionLabel.TabIndex = 1;
         this.DescriptionLabel.Text = "label1";
         // 
         // ColorPickerLauncher
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.DescriptionLabel);
         this.Controls.Add(this.PickColorButton);
         this.Name = "ColorPickerLauncher";
         this.Size = new System.Drawing.Size(146, 19);
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private System.Windows.Forms.Button PickColorButton;
      private System.Windows.Forms.Label DescriptionLabel;
   }
}
