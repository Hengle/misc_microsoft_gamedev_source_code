namespace PhoenixEditor.Filter_Dialogs
{
   partial class maskfromSlope
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
         this.button1 = new System.Windows.Forms.Button();
         this.button2 = new System.Windows.Forms.Button();
         this.checkBox1 = new System.Windows.Forms.CheckBox();
         this.trackBar1 = new System.Windows.Forms.TrackBar();
         this.label1 = new System.Windows.Forms.Label();
         this.label2 = new System.Windows.Forms.Label();
         this.trackBar2 = new System.Windows.Forms.TrackBar();
         ((System.ComponentModel.ISupportInitialize)(this.trackBar1)).BeginInit();
         ((System.ComponentModel.ISupportInitialize)(this.trackBar2)).BeginInit();
         this.SuspendLayout();
         // 
         // button1
         // 
         this.button1.Location = new System.Drawing.Point(205, 147);
         this.button1.Name = "button1";
         this.button1.Size = new System.Drawing.Size(75, 23);
         this.button1.TabIndex = 0;
         this.button1.Text = "Cancel";
         this.button1.UseVisualStyleBackColor = true;
         this.button1.Click += new System.EventHandler(this.button1_Click);
         // 
         // button2
         // 
         this.button2.Location = new System.Drawing.Point(124, 147);
         this.button2.Name = "button2";
         this.button2.Size = new System.Drawing.Size(75, 23);
         this.button2.TabIndex = 1;
         this.button2.Text = "Apply";
         this.button2.UseVisualStyleBackColor = true;
         this.button2.Click += new System.EventHandler(this.button2_Click);
         // 
         // checkBox1
         // 
         this.checkBox1.AutoSize = true;
         this.checkBox1.Location = new System.Drawing.Point(124, 124);
         this.checkBox1.Name = "checkBox1";
         this.checkBox1.Size = new System.Drawing.Size(158, 17);
         this.checkBox1.TabIndex = 2;
         this.checkBox1.Text = "Only Check Masked Terrain";
         this.checkBox1.UseVisualStyleBackColor = true;
         // 
         // trackBar1
         // 
         this.trackBar1.LargeChange = 10;
         this.trackBar1.Location = new System.Drawing.Point(141, 25);
         this.trackBar1.Maximum = 90;
         this.trackBar1.Minimum = 1;
         this.trackBar1.Name = "trackBar1";
         this.trackBar1.Size = new System.Drawing.Size(127, 45);
         this.trackBar1.TabIndex = 4;
         this.trackBar1.Value = 1;
         this.trackBar1.Scroll += new System.EventHandler(this.trackBar1_Scroll);
         // 
         // label1
         // 
         this.label1.AutoSize = true;
         this.label1.Location = new System.Drawing.Point(12, 25);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(110, 13);
         this.label1.TabIndex = 5;
         this.label1.Text = "Slope Angle to select:";
         // 
         // label2
         // 
         this.label2.AutoSize = true;
         this.label2.Location = new System.Drawing.Point(12, 78);
         this.label2.Name = "label2";
         this.label2.Size = new System.Drawing.Size(42, 13);
         this.label2.TabIndex = 6;
         this.label2.Text = "Range:";
         // 
         // trackBar2
         // 
         this.trackBar2.LargeChange = 10;
         this.trackBar2.Location = new System.Drawing.Point(141, 61);
         this.trackBar2.Maximum = 20;
         this.trackBar2.Minimum = 1;
         this.trackBar2.Name = "trackBar2";
         this.trackBar2.Size = new System.Drawing.Size(127, 45);
         this.trackBar2.TabIndex = 7;
         this.trackBar2.Value = 1;
         // 
         // maskfromSlope
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.ClientSize = new System.Drawing.Size(292, 202);
         this.Controls.Add(this.trackBar2);
         this.Controls.Add(this.label2);
         this.Controls.Add(this.label1);
         this.Controls.Add(this.trackBar1);
         this.Controls.Add(this.checkBox1);
         this.Controls.Add(this.button2);
         this.Controls.Add(this.button1);
         this.Name = "maskfromSlope";
         this.Text = "maskfromSlope";
         ((System.ComponentModel.ISupportInitialize)(this.trackBar1)).EndInit();
         ((System.ComponentModel.ISupportInitialize)(this.trackBar2)).EndInit();
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private System.Windows.Forms.Button button1;
      private System.Windows.Forms.Button button2;
      private System.Windows.Forms.CheckBox checkBox1;
      private System.Windows.Forms.TrackBar trackBar1;
      private System.Windows.Forms.Label label1;
      private System.Windows.Forms.Label label2;
      private System.Windows.Forms.TrackBar trackBar2;
   }
}