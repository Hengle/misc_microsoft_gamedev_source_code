namespace GenerateScenarioOpenFileLists
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
         this.components = new System.ComponentModel.Container();
         this.scenarioListBox = new System.Windows.Forms.ListBox();
         this.button1 = new System.Windows.Forms.Button();
         this.button2 = new System.Windows.Forms.Button();
         this.label1 = new System.Windows.Forms.Label();
         this.statusListBox = new System.Windows.Forms.ListBox();
         this.label2 = new System.Windows.Forms.Label();
         this.timer1 = new System.Windows.Forms.Timer(this.components);
         this.comboBox1 = new System.Windows.Forms.ComboBox();
         this.label3 = new System.Windows.Forms.Label();
         this.SuspendLayout();
         // 
         // scenarioListBox
         // 
         this.scenarioListBox.FormattingEnabled = true;
         this.scenarioListBox.Location = new System.Drawing.Point(12, 34);
         this.scenarioListBox.Name = "scenarioListBox";
         this.scenarioListBox.SelectionMode = System.Windows.Forms.SelectionMode.MultiSimple;
         this.scenarioListBox.Size = new System.Drawing.Size(585, 212);
         this.scenarioListBox.Sorted = true;
         this.scenarioListBox.TabIndex = 0;
         // 
         // button1
         // 
         this.button1.Location = new System.Drawing.Point(522, 252);
         this.button1.Name = "button1";
         this.button1.Size = new System.Drawing.Size(75, 23);
         this.button1.TabIndex = 1;
         this.button1.Text = "GO!";
         this.button1.UseVisualStyleBackColor = true;
         this.button1.Click += new System.EventHandler(this.button1_Click);
         // 
         // button2
         // 
         this.button2.Location = new System.Drawing.Point(522, 8);
         this.button2.Name = "button2";
         this.button2.Size = new System.Drawing.Size(75, 23);
         this.button2.TabIndex = 2;
         this.button2.Text = "Select All";
         this.button2.UseVisualStyleBackColor = true;
         this.button2.Click += new System.EventHandler(this.button2_Click);
         // 
         // label1
         // 
         this.label1.AutoSize = true;
         this.label1.Location = new System.Drawing.Point(12, 18);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(172, 13);
         this.label1.TabIndex = 3;
         this.label1.Text = "Select Files to generate file lists for:";
         // 
         // statusListBox
         // 
         this.statusListBox.FormattingEnabled = true;
         this.statusListBox.Location = new System.Drawing.Point(12, 282);
         this.statusListBox.Name = "statusListBox";
         this.statusListBox.Size = new System.Drawing.Size(585, 95);
         this.statusListBox.TabIndex = 4;
         // 
         // label2
         // 
         this.label2.AutoSize = true;
         this.label2.Location = new System.Drawing.Point(12, 266);
         this.label2.Name = "label2";
         this.label2.Size = new System.Drawing.Size(40, 13);
         this.label2.TabIndex = 5;
         this.label2.Text = "Status:";
         // 
         // timer1
         // 
         this.timer1.Tick += new System.EventHandler(this.timer1_Tick);
         // 
         // comboBox1
         // 
         this.comboBox1.FormattingEnabled = true;
         this.comboBox1.Items.AddRange(new object[] {
            "Show ALL",
            "Playtest Only"});
         this.comboBox1.Location = new System.Drawing.Point(337, 7);
         this.comboBox1.Name = "comboBox1";
         this.comboBox1.Size = new System.Drawing.Size(121, 21);
         this.comboBox1.TabIndex = 6;
         this.comboBox1.SelectedIndexChanged += new System.EventHandler(this.comboBox1_SelectedIndexChanged);
         // 
         // label3
         // 
         this.label3.AutoSize = true;
         this.label3.Location = new System.Drawing.Point(293, 10);
         this.label3.Name = "label3";
         this.label3.Size = new System.Drawing.Size(38, 13);
         this.label3.TabIndex = 7;
         this.label3.Text = "Filter : ";
         // 
         // Form1
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.ClientSize = new System.Drawing.Size(609, 392);
         this.Controls.Add(this.label3);
         this.Controls.Add(this.comboBox1);
         this.Controls.Add(this.label2);
         this.Controls.Add(this.statusListBox);
         this.Controls.Add(this.label1);
         this.Controls.Add(this.button2);
         this.Controls.Add(this.button1);
         this.Controls.Add(this.scenarioListBox);
         this.Name = "Form1";
         this.Text = "Generate Scenario File Lists";
         this.Load += new System.EventHandler(this.Form1_Load);
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private System.Windows.Forms.ListBox scenarioListBox;
      private System.Windows.Forms.Button button1;
      private System.Windows.Forms.Button button2;
      private System.Windows.Forms.Label label1;
      private System.Windows.Forms.ListBox statusListBox;
      private System.Windows.Forms.Label label2;
      private System.Windows.Forms.Timer timer1;
      private System.Windows.Forms.ComboBox comboBox1;
      private System.Windows.Forms.Label label3;
   }
}

