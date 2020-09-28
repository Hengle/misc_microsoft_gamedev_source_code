namespace Terrain.Controls
{
   partial class AOGenDialog
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
         this.comboBox1 = new System.Windows.Forms.ComboBox();
         this.label1 = new System.Windows.Forms.Label();
         this.button1 = new System.Windows.Forms.Button();
         this.progressBar1 = new System.Windows.Forms.ProgressBar();
         this.button2 = new System.Windows.Forms.Button();
         this.radioButton1 = new System.Windows.Forms.RadioButton();
         this.radioButton2 = new System.Windows.Forms.RadioButton();
         this.checkBox1 = new System.Windows.Forms.CheckBox();
         this.statusLabel = new System.Windows.Forms.Label();
         this.SuspendLayout();
         // 
         // comboBox1
         // 
         this.comboBox1.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
         this.comboBox1.FormattingEnabled = true;
         this.comboBox1.Items.AddRange(new object[] {
            "Low",
            "Medium",
            "High",
            "Final"});
         this.comboBox1.Location = new System.Drawing.Point(136, 9);
         this.comboBox1.Name = "comboBox1";
         this.comboBox1.Size = new System.Drawing.Size(144, 21);
         this.comboBox1.TabIndex = 0;
         // 
         // label1
         // 
         this.label1.AutoSize = true;
         this.label1.Location = new System.Drawing.Point(12, 9);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(48, 13);
         this.label1.TabIndex = 1;
         this.label1.Text = "Quality : ";
         // 
         // button1
         // 
         this.button1.Location = new System.Drawing.Point(205, 84);
         this.button1.Name = "button1";
         this.button1.Size = new System.Drawing.Size(75, 23);
         this.button1.TabIndex = 2;
         this.button1.Text = "Start";
         this.button1.UseVisualStyleBackColor = true;
         this.button1.Click += new System.EventHandler(this.button1_Click);
         // 
         // progressBar1
         // 
         this.progressBar1.Location = new System.Drawing.Point(12, 113);
         this.progressBar1.Name = "progressBar1";
         this.progressBar1.Size = new System.Drawing.Size(268, 21);
         this.progressBar1.TabIndex = 5;
         // 
         // button2
         // 
         this.button2.Enabled = false;
         this.button2.Location = new System.Drawing.Point(105, 162);
         this.button2.Name = "button2";
         this.button2.Size = new System.Drawing.Size(75, 23);
         this.button2.TabIndex = 6;
         this.button2.Text = "Cancel";
         this.button2.UseVisualStyleBackColor = true;
         this.button2.Click += new System.EventHandler(this.button2_Click);
         // 
         // radioButton1
         // 
         this.radioButton1.AutoSize = true;
         this.radioButton1.Checked = true;
         this.radioButton1.Location = new System.Drawing.Point(15, 41);
         this.radioButton1.Name = "radioButton1";
         this.radioButton1.Size = new System.Drawing.Size(88, 17);
         this.radioButton1.TabIndex = 7;
         this.radioButton1.TabStop = true;
         this.radioButton1.Text = "Network Gen";
         this.radioButton1.UseVisualStyleBackColor = true;
         // 
         // radioButton2
         // 
         this.radioButton2.AutoSize = true;
         this.radioButton2.Location = new System.Drawing.Point(15, 64);
         this.radioButton2.Name = "radioButton2";
         this.radioButton2.Size = new System.Drawing.Size(74, 17);
         this.radioButton2.TabIndex = 8;
         this.radioButton2.Text = "Local Gen";
         this.radioButton2.UseVisualStyleBackColor = true;
         // 
         // checkBox1
         // 
         this.checkBox1.AutoSize = true;
         this.checkBox1.Location = new System.Drawing.Point(180, 41);
         this.checkBox1.Name = "checkBox1";
         this.checkBox1.Size = new System.Drawing.Size(100, 17);
         this.checkBox1.TabIndex = 9;
         this.checkBox1.Text = "Include Objects";
         this.checkBox1.UseVisualStyleBackColor = true;
         // 
         // statusLabel
         // 
         this.statusLabel.AutoSize = true;
         this.statusLabel.Location = new System.Drawing.Point(133, 137);
         this.statusLabel.Name = "statusLabel";
         this.statusLabel.Size = new System.Drawing.Size(0, 13);
         this.statusLabel.TabIndex = 10;
         // 
         // AOGenDialog
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.ClientSize = new System.Drawing.Size(292, 197);
         this.Controls.Add(this.statusLabel);
         this.Controls.Add(this.checkBox1);
         this.Controls.Add(this.radioButton2);
         this.Controls.Add(this.radioButton1);
         this.Controls.Add(this.button2);
         this.Controls.Add(this.progressBar1);
         this.Controls.Add(this.button1);
         this.Controls.Add(this.label1);
         this.Controls.Add(this.comboBox1);
         this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
         this.Name = "AOGenDialog";
         this.Text = "AO Gen";
         this.TopMost = true;
         this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.AOGenDialog_FormClosing);
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private System.Windows.Forms.ComboBox comboBox1;
      private System.Windows.Forms.Label label1;
      private System.Windows.Forms.Button button1;
      private System.Windows.Forms.ProgressBar progressBar1;
      private System.Windows.Forms.Button button2;
      private System.Windows.Forms.RadioButton radioButton1;
      private System.Windows.Forms.RadioButton radioButton2;
      private System.Windows.Forms.CheckBox checkBox1;
      private System.Windows.Forms.Label statusLabel;
   }
}