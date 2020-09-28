namespace xfs
{
    partial class FilterComboBox
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
            this.buttonFilterPreset = new System.Windows.Forms.Button();
            this.buttonFilterHistory = new System.Windows.Forms.Button();
            this.buttonFilterXFS = new System.Windows.Forms.Button();
            this.buttonCmdSubmit = new System.Windows.Forms.Button();
            this.autoCompleteComboBox1 = new tabComplete.AutoCompleteComboBox();
            this.SuspendLayout();
            // 
            // buttonFilterPreset
            // 
            this.buttonFilterPreset.Location = new System.Drawing.Point(3, 3);
            this.buttonFilterPreset.Name = "buttonFilterPreset";
            this.buttonFilterPreset.Size = new System.Drawing.Size(75, 23);
            this.buttonFilterPreset.TabIndex = 1;
            this.buttonFilterPreset.Text = "Presets";
            this.buttonFilterPreset.UseVisualStyleBackColor = true;
            // 
            // buttonFilterHistory
            // 
            this.buttonFilterHistory.Location = new System.Drawing.Point(84, 3);
            this.buttonFilterHistory.Name = "buttonFilterHistory";
            this.buttonFilterHistory.Size = new System.Drawing.Size(75, 23);
            this.buttonFilterHistory.TabIndex = 2;
            this.buttonFilterHistory.Text = "History";
            this.buttonFilterHistory.UseVisualStyleBackColor = true;
            // 
            // buttonFilterXFS
            // 
            this.buttonFilterXFS.Location = new System.Drawing.Point(166, 2);
            this.buttonFilterXFS.Name = "buttonFilterXFS";
            this.buttonFilterXFS.Size = new System.Drawing.Size(75, 23);
            this.buttonFilterXFS.TabIndex = 3;
            this.buttonFilterXFS.Text = "XFS";
            this.buttonFilterXFS.UseVisualStyleBackColor = true;
            // 
            // buttonCmdSubmit
            // 
            this.buttonCmdSubmit.Location = new System.Drawing.Point(564, 31);
            this.buttonCmdSubmit.Name = "buttonCmdSubmit";
            this.buttonCmdSubmit.Size = new System.Drawing.Size(42, 21);
            this.buttonCmdSubmit.TabIndex = 4;
            this.buttonCmdSubmit.Text = "<";
            this.buttonCmdSubmit.UseVisualStyleBackColor = true;
            // 
            // autoCompleteComboBox1
            // 
            this.autoCompleteComboBox1.FormattingEnabled = true;
            this.autoCompleteComboBox1.LimitToList = true;
            this.autoCompleteComboBox1.Location = new System.Drawing.Point(3, 32);
            this.autoCompleteComboBox1.Name = "autoCompleteComboBox1";
            this.autoCompleteComboBox1.Size = new System.Drawing.Size(555, 21);
            this.autoCompleteComboBox1.TabIndex = 0;
            // 
            // FilterComboBox
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.buttonCmdSubmit);
            this.Controls.Add(this.buttonFilterXFS);
            this.Controls.Add(this.buttonFilterHistory);
            this.Controls.Add(this.buttonFilterPreset);
            this.Controls.Add(this.autoCompleteComboBox1);
            this.Name = "FilterComboBox";
            this.Size = new System.Drawing.Size(609, 57);
            this.ResumeLayout(false);

        }

        #endregion

        private tabComplete.AutoCompleteComboBox autoCompleteComboBox1;
        private System.Windows.Forms.Button buttonFilterPreset;
        private System.Windows.Forms.Button buttonFilterHistory;
        private System.Windows.Forms.Button buttonFilterXFS;
        private System.Windows.Forms.Button buttonCmdSubmit;
    }
}
