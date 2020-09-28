namespace EditorCore.Controls.Micro
{
   partial class DynamicHelp
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
         this.HelpLabel = new System.Windows.Forms.Label();
         this.HelpTextBox = new System.Windows.Forms.TextBox();
         this.SaveButton = new System.Windows.Forms.Button();
         this.label1 = new System.Windows.Forms.Label();
         this.SuspendLayout();
         // 
         // HelpLabel
         // 
         this.HelpLabel.AutoSize = true;
         this.HelpLabel.Font = new System.Drawing.Font("Microsoft Sans Serif", 11.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
         this.HelpLabel.Location = new System.Drawing.Point(3, 4);
         this.HelpLabel.Name = "HelpLabel";
         this.HelpLabel.Size = new System.Drawing.Size(52, 18);
         this.HelpLabel.TabIndex = 0;
         this.HelpLabel.Text = "label1";
         // 
         // HelpTextBox
         // 
         this.HelpTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.HelpTextBox.BackColor = System.Drawing.SystemColors.Info;
         this.HelpTextBox.Location = new System.Drawing.Point(6, 25);
         this.HelpTextBox.Multiline = true;
         this.HelpTextBox.Name = "HelpTextBox";
         this.HelpTextBox.Size = new System.Drawing.Size(252, 129);
         this.HelpTextBox.TabIndex = 1;
         // 
         // SaveButton
         // 
         this.SaveButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
         this.SaveButton.Enabled = false;
         this.SaveButton.Location = new System.Drawing.Point(6, 160);
         this.SaveButton.Name = "SaveButton";
         this.SaveButton.Size = new System.Drawing.Size(75, 23);
         this.SaveButton.TabIndex = 3;
         this.SaveButton.Text = "Save";
         this.SaveButton.UseVisualStyleBackColor = true;
         // 
         // label1
         // 
         this.label1.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
         this.label1.AutoSize = true;
         this.label1.Location = new System.Drawing.Point(87, 160);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(111, 13);
         this.label1.TabIndex = 4;
         this.label1.Text = "(Just type to add help)";
         // 
         // DynamicHelp
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.BackColor = System.Drawing.SystemColors.Info;
         this.Controls.Add(this.label1);
         this.Controls.Add(this.SaveButton);
         this.Controls.Add(this.HelpTextBox);
         this.Controls.Add(this.HelpLabel);
         this.Name = "DynamicHelp";
         this.Size = new System.Drawing.Size(271, 190);
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private System.Windows.Forms.Label HelpLabel;
      private System.Windows.Forms.TextBox HelpTextBox;
      private System.Windows.Forms.Button SaveButton;
      private System.Windows.Forms.Label label1;
   }
}
