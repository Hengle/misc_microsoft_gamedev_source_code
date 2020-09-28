namespace DepGenReporter
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
           this.buttonBrowse = new System.Windows.Forms.Button();
           this.richTextBox1 = new System.Windows.Forms.RichTextBox();
           this.SuspendLayout();
           // 
           // buttonBrowse
           // 
           this.buttonBrowse.Location = new System.Drawing.Point(12, 12);
           this.buttonBrowse.Name = "buttonBrowse";
           this.buttonBrowse.Size = new System.Drawing.Size(75, 23);
           this.buttonBrowse.TabIndex = 0;
           this.buttonBrowse.Text = "Browse";
           this.buttonBrowse.UseVisualStyleBackColor = true;
           this.buttonBrowse.Click += new System.EventHandler(this.buttonBrowse_Click);
           // 
           // richTextBox1
           // 
           this.richTextBox1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                       | System.Windows.Forms.AnchorStyles.Left)
                       | System.Windows.Forms.AnchorStyles.Right)));
           this.richTextBox1.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(0)))), ((int)(((byte)(64)))), ((int)(((byte)(0)))));
           this.richTextBox1.ForeColor = System.Drawing.Color.LightGreen;
           this.richTextBox1.Location = new System.Drawing.Point(12, 41);
           this.richTextBox1.Name = "richTextBox1";
           this.richTextBox1.Size = new System.Drawing.Size(589, 260);
           this.richTextBox1.TabIndex = 1;
           this.richTextBox1.Text = "";
           // 
           // Form1
           // 
           this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
           this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
           this.ClientSize = new System.Drawing.Size(613, 313);
           this.Controls.Add(this.richTextBox1);
           this.Controls.Add(this.buttonBrowse);
           this.Name = "Form1";
           this.Text = "DepGenReporter";
           this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Button buttonBrowse;
       private System.Windows.Forms.RichTextBox richTextBox1;
    }
}

