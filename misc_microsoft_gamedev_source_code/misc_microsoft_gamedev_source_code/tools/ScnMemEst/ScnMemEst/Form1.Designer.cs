namespace ScnMemEst
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
         this.label1 = new System.Windows.Forms.Label();
         this.button1 = new System.Windows.Forms.Button();
         this.scenarioListBox = new System.Windows.Forms.ListBox();
         this.groupBox1 = new System.Windows.Forms.GroupBox();
         this.checkBox4 = new System.Windows.Forms.CheckBox();
         this.checkBox3 = new System.Windows.Forms.CheckBox();
         this.checkBox2 = new System.Windows.Forms.CheckBox();
         this.checkBox1 = new System.Windows.Forms.CheckBox();
         this.label2 = new System.Windows.Forms.Label();
         this.textBox1 = new System.Windows.Forms.TextBox();
         this.button2 = new System.Windows.Forms.Button();
         this.groupBox1.SuspendLayout();
         this.SuspendLayout();
         // 
         // label1
         // 
         this.label1.AutoSize = true;
         this.label1.Location = new System.Drawing.Point(12, 76);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(222, 13);
         this.label1.TabIndex = 11;
         this.label1.Text = "Select Files to generate memory estimates for:";
         // 
         // button1
         // 
         this.button1.Location = new System.Drawing.Point(514, 22);
         this.button1.Name = "button1";
         this.button1.Size = new System.Drawing.Size(75, 23);
         this.button1.TabIndex = 9;
         this.button1.Text = "Calculate";
         this.button1.UseVisualStyleBackColor = true;
         this.button1.Click += new System.EventHandler(this.button1_Click);
         // 
         // scenarioListBox
         // 
         this.scenarioListBox.FormattingEnabled = true;
         this.scenarioListBox.Location = new System.Drawing.Point(12, 92);
         this.scenarioListBox.Name = "scenarioListBox";
         this.scenarioListBox.SelectionMode = System.Windows.Forms.SelectionMode.MultiExtended;
         this.scenarioListBox.Size = new System.Drawing.Size(460, 758);
         this.scenarioListBox.Sorted = true;
         this.scenarioListBox.TabIndex = 8;
         // 
         // groupBox1
         // 
         this.groupBox1.Controls.Add(this.checkBox4);
         this.groupBox1.Controls.Add(this.checkBox3);
         this.groupBox1.Controls.Add(this.checkBox2);
         this.groupBox1.Controls.Add(this.checkBox1);
         this.groupBox1.Location = new System.Drawing.Point(478, 92);
         this.groupBox1.Name = "groupBox1";
         this.groupBox1.Size = new System.Drawing.Size(111, 117);
         this.groupBox1.TabIndex = 15;
         this.groupBox1.TabStop = false;
         this.groupBox1.Text = "Filter :";
         // 
         // checkBox4
         // 
         this.checkBox4.AutoSize = true;
         this.checkBox4.Checked = true;
         this.checkBox4.CheckState = System.Windows.Forms.CheckState.Checked;
         this.checkBox4.Location = new System.Drawing.Point(6, 88);
         this.checkBox4.Name = "checkBox4";
         this.checkBox4.Size = new System.Drawing.Size(53, 17);
         this.checkBox4.TabIndex = 3;
         this.checkBox4.Text = "Alpha";
         this.checkBox4.UseVisualStyleBackColor = true;
         this.checkBox4.CheckedChanged += new System.EventHandler(this.checkBox4_CheckedChanged);
         // 
         // checkBox3
         // 
         this.checkBox3.AutoSize = true;
         this.checkBox3.Checked = true;
         this.checkBox3.CheckState = System.Windows.Forms.CheckState.Checked;
         this.checkBox3.Location = new System.Drawing.Point(6, 65);
         this.checkBox3.Name = "checkBox3";
         this.checkBox3.Size = new System.Drawing.Size(73, 17);
         this.checkBox3.TabIndex = 2;
         this.checkBox3.Text = "Campaign";
         this.checkBox3.UseVisualStyleBackColor = true;
         this.checkBox3.CheckedChanged += new System.EventHandler(this.checkBox3_CheckedChanged);
         // 
         // checkBox2
         // 
         this.checkBox2.AutoSize = true;
         this.checkBox2.Checked = true;
         this.checkBox2.CheckState = System.Windows.Forms.CheckState.Checked;
         this.checkBox2.Location = new System.Drawing.Point(6, 42);
         this.checkBox2.Name = "checkBox2";
         this.checkBox2.Size = new System.Drawing.Size(89, 17);
         this.checkBox2.TabIndex = 1;
         this.checkBox2.Text = "Development";
         this.checkBox2.UseVisualStyleBackColor = true;
         this.checkBox2.CheckedChanged += new System.EventHandler(this.checkBox2_CheckedChanged);
         // 
         // checkBox1
         // 
         this.checkBox1.AutoSize = true;
         this.checkBox1.Checked = true;
         this.checkBox1.CheckState = System.Windows.Forms.CheckState.Checked;
         this.checkBox1.Location = new System.Drawing.Point(6, 19);
         this.checkBox1.Name = "checkBox1";
         this.checkBox1.Size = new System.Drawing.Size(63, 17);
         this.checkBox1.TabIndex = 0;
         this.checkBox1.Text = "Playtest";
         this.checkBox1.UseVisualStyleBackColor = true;
         this.checkBox1.CheckedChanged += new System.EventHandler(this.checkBox1_CheckedChanged);
         // 
         // label2
         // 
         this.label2.AutoSize = true;
         this.label2.Location = new System.Drawing.Point(12, 9);
         this.label2.Name = "label2";
         this.label2.Size = new System.Drawing.Size(93, 13);
         this.label2.TabIndex = 16;
         this.label2.Text = "Output Directory : ";
         // 
         // textBox1
         // 
         this.textBox1.Location = new System.Drawing.Point(15, 25);
         this.textBox1.Name = "textBox1";
         this.textBox1.Size = new System.Drawing.Size(372, 20);
         this.textBox1.TabIndex = 17;
         // 
         // button2
         // 
         this.button2.Location = new System.Drawing.Point(397, 22);
         this.button2.Name = "button2";
         this.button2.Size = new System.Drawing.Size(25, 23);
         this.button2.TabIndex = 18;
         this.button2.Text = "...";
         this.button2.UseVisualStyleBackColor = true;
         this.button2.Click += new System.EventHandler(this.button2_Click);
         // 
         // Form1
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.ClientSize = new System.Drawing.Size(604, 870);
         this.Controls.Add(this.button2);
         this.Controls.Add(this.textBox1);
         this.Controls.Add(this.label2);
         this.Controls.Add(this.groupBox1);
         this.Controls.Add(this.label1);
         this.Controls.Add(this.button1);
         this.Controls.Add(this.scenarioListBox);
         this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
         this.Name = "Form1";
         this.Text = "Scenario Memory Estimates";
         this.Load += new System.EventHandler(this.Form1_Load);
         this.groupBox1.ResumeLayout(false);
         this.groupBox1.PerformLayout();
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private System.Windows.Forms.Label label1;
      private System.Windows.Forms.Button button1;
      private System.Windows.Forms.ListBox scenarioListBox;
      private System.Windows.Forms.GroupBox groupBox1;
      private System.Windows.Forms.CheckBox checkBox3;
      private System.Windows.Forms.CheckBox checkBox2;
      private System.Windows.Forms.CheckBox checkBox1;
      private System.Windows.Forms.CheckBox checkBox4;
      private System.Windows.Forms.Label label2;
      private System.Windows.Forms.TextBox textBox1;
      private System.Windows.Forms.Button button2;
   }
}

