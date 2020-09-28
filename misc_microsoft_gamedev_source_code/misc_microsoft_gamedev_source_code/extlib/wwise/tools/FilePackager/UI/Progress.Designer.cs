namespace AkFilePackager
{
    partial class Progress
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
            System.Windows.Forms.Label label1;
            this.progressBar = new System.Windows.Forms.ProgressBar();
            this.buttonStopClose = new System.Windows.Forms.Button();
            this.textBoxProgMsg = new System.Windows.Forms.TextBox();
            this.listBoxMessageLog = new System.Windows.Forms.ListBox();
            label1 = new System.Windows.Forms.Label();
            this.SuspendLayout();
            // 
            // label1
            // 
            label1.AutoSize = true;
            label1.Location = new System.Drawing.Point(12, 69);
            label1.Name = "label1";
            label1.Size = new System.Drawing.Size(58, 13);
            label1.TabIndex = 4;
            label1.Text = "Messages:";
            // 
            // progressBar
            // 
            this.progressBar.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.progressBar.Location = new System.Drawing.Point(12, 39);
            this.progressBar.Name = "progressBar";
            this.progressBar.Size = new System.Drawing.Size(468, 23);
            this.progressBar.TabIndex = 0;
            // 
            // buttonStopClose
            // 
            this.buttonStopClose.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.buttonStopClose.Location = new System.Drawing.Point(405, 365);
            this.buttonStopClose.Name = "buttonStopClose";
            this.buttonStopClose.Size = new System.Drawing.Size(75, 23);
            this.buttonStopClose.TabIndex = 1;
            this.buttonStopClose.Text = "Stop";
            this.buttonStopClose.UseVisualStyleBackColor = true;
            this.buttonStopClose.Click += new System.EventHandler(this.buttonStopClose_Click);
            // 
            // textBoxProgMsg
            // 
            this.textBoxProgMsg.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.textBoxProgMsg.BackColor = System.Drawing.SystemColors.Control;
            this.textBoxProgMsg.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this.textBoxProgMsg.CausesValidation = false;
            this.textBoxProgMsg.Location = new System.Drawing.Point(13, 13);
            this.textBoxProgMsg.Name = "textBoxProgMsg";
            this.textBoxProgMsg.ReadOnly = true;
            this.textBoxProgMsg.Size = new System.Drawing.Size(467, 13);
            this.textBoxProgMsg.TabIndex = 2;
            // 
            // listBoxMessageLog
            // 
            this.listBoxMessageLog.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                        | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.listBoxMessageLog.FormattingEnabled = true;
            this.listBoxMessageLog.HorizontalScrollbar = true;
            this.listBoxMessageLog.Location = new System.Drawing.Point(12, 86);
            this.listBoxMessageLog.Name = "listBoxMessageLog";
            this.listBoxMessageLog.Size = new System.Drawing.Size(468, 264);
            this.listBoxMessageLog.TabIndex = 5;
            // 
            // Progress
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(492, 396);
            this.Controls.Add(this.listBoxMessageLog);
            this.Controls.Add(label1);
            this.Controls.Add(this.textBoxProgMsg);
            this.Controls.Add(this.buttonStopClose);
            this.Controls.Add(this.progressBar);
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "Progress";
            this.Text = "File Packaging Progress";
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.Progress_FormClosing);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.ProgressBar progressBar;
        private System.Windows.Forms.Button buttonStopClose;
        private System.Windows.Forms.TextBox textBoxProgMsg;
        private System.Windows.Forms.ListBox listBoxMessageLog;
    }
}