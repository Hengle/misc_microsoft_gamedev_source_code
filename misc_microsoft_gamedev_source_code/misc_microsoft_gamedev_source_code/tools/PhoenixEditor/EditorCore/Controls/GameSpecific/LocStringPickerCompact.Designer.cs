namespace EditorCore.Controls
{
   partial class LocStringPickerCompact
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
         this.StringTextLabel = new System.Windows.Forms.Label();
         this.IDTextBox = new System.Windows.Forms.TextBox();
         this.SearchButton = new System.Windows.Forms.Button();
         this.SuspendLayout();
         // 
         // StringTextLabel
         // 
         this.StringTextLabel.AutoSize = true;
         this.StringTextLabel.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
         this.StringTextLabel.Location = new System.Drawing.Point(139, 7);
         this.StringTextLabel.Name = "StringTextLabel";
         this.StringTextLabel.Size = new System.Drawing.Size(41, 13);
         this.StringTextLabel.TabIndex = 3;
         this.StringTextLabel.Text = "label1";
         // 
         // IDTextBox
         // 
         this.IDTextBox.Location = new System.Drawing.Point(5, 4);
         this.IDTextBox.Name = "IDTextBox";
         this.IDTextBox.Size = new System.Drawing.Size(100, 20);
         this.IDTextBox.TabIndex = 2;
         // 
         // SearchButton
         // 
         this.SearchButton.Location = new System.Drawing.Point(107, 2);
         this.SearchButton.Name = "SearchButton";
         this.SearchButton.Size = new System.Drawing.Size(28, 23);
         this.SearchButton.TabIndex = 4;
         this.SearchButton.UseVisualStyleBackColor = true;
         this.SearchButton.Click += new System.EventHandler(this.SearchButton_Click);
         // 
         // LocStringPickerCompact
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.SearchButton);
         this.Controls.Add(this.StringTextLabel);
         this.Controls.Add(this.IDTextBox);
         this.Name = "LocStringPickerCompact";
         this.Size = new System.Drawing.Size(401, 28);
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private System.Windows.Forms.Label StringTextLabel;
      private System.Windows.Forms.TextBox IDTextBox;
      private System.Windows.Forms.Button SearchButton;
   }
}
