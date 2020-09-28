namespace XMBHelper
{
   partial class Form1
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
         this.CheckOutXMBBox = new System.Windows.Forms.CheckBox();
         this.PauseCheckBox = new System.Windows.Forms.CheckBox();
         this.SuspendLayout();
         // 
         // CheckOutXMBBox
         // 
         this.CheckOutXMBBox.AutoSize = true;
         this.CheckOutXMBBox.Checked = true;
         this.CheckOutXMBBox.CheckState = System.Windows.Forms.CheckState.Checked;
         this.CheckOutXMBBox.Location = new System.Drawing.Point(2, 3);
         this.CheckOutXMBBox.Name = "CheckOutXMBBox";
         this.CheckOutXMBBox.Size = new System.Drawing.Size(106, 17);
         this.CheckOutXMBBox.TabIndex = 0;
         this.CheckOutXMBBox.Text = "Check out XMBs";
         this.CheckOutXMBBox.UseVisualStyleBackColor = true;
         // 
         // PauseCheckBox
         // 
         this.PauseCheckBox.AutoSize = true;
         this.PauseCheckBox.Location = new System.Drawing.Point(117, 3);
         this.PauseCheckBox.Name = "PauseCheckBox";
         this.PauseCheckBox.Size = new System.Drawing.Size(56, 17);
         this.PauseCheckBox.TabIndex = 1;
         this.PauseCheckBox.Text = "Pause";
         this.PauseCheckBox.UseVisualStyleBackColor = true;
         // 
         // Form1
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.ClientSize = new System.Drawing.Size(174, 23);
         this.Controls.Add(this.PauseCheckBox);
         this.Controls.Add(this.CheckOutXMBBox);
         this.Name = "Form1";
         this.Text = "XMBHelper";
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private System.Windows.Forms.CheckBox CheckOutXMBBox;
      private System.Windows.Forms.CheckBox PauseCheckBox;
   }
}

