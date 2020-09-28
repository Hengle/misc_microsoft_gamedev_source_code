namespace EditorCore
{
   partial class TimeEditor
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
         this.label1 = new System.Windows.Forms.Label();
         this.MinutesTextBox = new System.Windows.Forms.TextBox();
         this.SecondsTextBox = new System.Windows.Forms.TextBox();
         this.MillisecondsTextBox = new System.Windows.Forms.TextBox();
         this.label2 = new System.Windows.Forms.Label();
         this.label3 = new System.Windows.Forms.Label();
         this.SuspendLayout();
         // 
         // label1
         // 
         this.label1.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
         this.label1.AutoSize = true;
         this.label1.Location = new System.Drawing.Point(99, 4);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(44, 13);
         this.label1.TabIndex = 0;
         this.label1.Text = "Minutes";
         // 
         // MinutesTextBox
         // 
         this.MinutesTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.MinutesTextBox.Location = new System.Drawing.Point(4, 4);
         this.MinutesTextBox.Name = "MinutesTextBox";
         this.MinutesTextBox.Size = new System.Drawing.Size(93, 20);
         this.MinutesTextBox.TabIndex = 1;
         this.MinutesTextBox.TextChanged += new System.EventHandler(this.MinutesTextBox_TextChanged);
         // 
         // SecondsTextBox
         // 
         this.SecondsTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.SecondsTextBox.Location = new System.Drawing.Point(4, 31);
         this.SecondsTextBox.Name = "SecondsTextBox";
         this.SecondsTextBox.Size = new System.Drawing.Size(93, 20);
         this.SecondsTextBox.TabIndex = 2;
         this.SecondsTextBox.TextChanged += new System.EventHandler(this.SecondsTextBox_TextChanged);
         // 
         // MillisecondsTextBox
         // 
         this.MillisecondsTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.MillisecondsTextBox.Location = new System.Drawing.Point(4, 58);
         this.MillisecondsTextBox.Name = "MillisecondsTextBox";
         this.MillisecondsTextBox.Size = new System.Drawing.Size(93, 20);
         this.MillisecondsTextBox.TabIndex = 3;
         this.MillisecondsTextBox.TextChanged += new System.EventHandler(this.MillisecondsTextBox_TextChanged);
         // 
         // label2
         // 
         this.label2.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
         this.label2.AutoSize = true;
         this.label2.Location = new System.Drawing.Point(99, 31);
         this.label2.Name = "label2";
         this.label2.Size = new System.Drawing.Size(49, 13);
         this.label2.TabIndex = 4;
         this.label2.Text = "Seconds";
         // 
         // label3
         // 
         this.label3.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
         this.label3.AutoSize = true;
         this.label3.Location = new System.Drawing.Point(99, 58);
         this.label3.Name = "label3";
         this.label3.Size = new System.Drawing.Size(64, 13);
         this.label3.TabIndex = 5;
         this.label3.Text = "Milliseconds";
         // 
         // TimeEditor
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.label3);
         this.Controls.Add(this.label2);
         this.Controls.Add(this.MillisecondsTextBox);
         this.Controls.Add(this.SecondsTextBox);
         this.Controls.Add(this.MinutesTextBox);
         this.Controls.Add(this.label1);
         this.Name = "TimeEditor";
         this.Size = new System.Drawing.Size(166, 85);
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private System.Windows.Forms.Label label1;
      private System.Windows.Forms.TextBox MinutesTextBox;
      private System.Windows.Forms.TextBox SecondsTextBox;
      private System.Windows.Forms.TextBox MillisecondsTextBox;
      private System.Windows.Forms.Label label2;
      private System.Windows.Forms.Label label3;
   }
}
