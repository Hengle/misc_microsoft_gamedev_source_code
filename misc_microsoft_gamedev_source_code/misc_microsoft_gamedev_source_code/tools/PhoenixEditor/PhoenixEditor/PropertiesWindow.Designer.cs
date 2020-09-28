namespace Terrain
{
   partial class PropertiesWindow
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
         this.MinSizetrackBar = new System.Windows.Forms.TrackBar();
         this.MaxSizetrackBar = new System.Windows.Forms.TrackBar();
         this.label1 = new System.Windows.Forms.Label();
         this.label2 = new System.Windows.Forms.Label();
         this.label3 = new System.Windows.Forms.Label();
         this.SpacingtrackBar = new System.Windows.Forms.TrackBar();
         this.label5 = new System.Windows.Forms.Label();
         this.AngleJittertrackBar = new System.Windows.Forms.TrackBar();
         this.PosJittertrackBar = new System.Windows.Forms.TrackBar();
         this.label6 = new System.Windows.Forms.Label();
         this.timer1 = new System.Windows.Forms.Timer(this.components);
         this.pictureBox1 = new System.Windows.Forms.PictureBox();
         this.ScatterCounttrackBar = new System.Windows.Forms.TrackBar();
         this.label7 = new System.Windows.Forms.Label();
         this.Defaultbutton = new System.Windows.Forms.Button();
         ((System.ComponentModel.ISupportInitialize)(this.MinSizetrackBar)).BeginInit();
         ((System.ComponentModel.ISupportInitialize)(this.MaxSizetrackBar)).BeginInit();
         ((System.ComponentModel.ISupportInitialize)(this.SpacingtrackBar)).BeginInit();
         ((System.ComponentModel.ISupportInitialize)(this.AngleJittertrackBar)).BeginInit();
         ((System.ComponentModel.ISupportInitialize)(this.PosJittertrackBar)).BeginInit();
         ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).BeginInit();
         ((System.ComponentModel.ISupportInitialize)(this.ScatterCounttrackBar)).BeginInit();
         this.SuspendLayout();
         // 
         // MinSizetrackBar
         // 
         this.MinSizetrackBar.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.MinSizetrackBar.Location = new System.Drawing.Point(73, 43);
         this.MinSizetrackBar.Maximum = 1000;
         this.MinSizetrackBar.Minimum = 10;
         this.MinSizetrackBar.Name = "MinSizetrackBar";
         this.MinSizetrackBar.Size = new System.Drawing.Size(235, 45);
         this.MinSizetrackBar.TabIndex = 0;
         this.MinSizetrackBar.TickStyle = System.Windows.Forms.TickStyle.None;
         this.MinSizetrackBar.Value = 10;
         this.MinSizetrackBar.Scroll += new System.EventHandler(this.MinSizetrackBar_Scroll);
         this.MinSizetrackBar.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this.MinSizetrackBar_KeyPress);
         // 
         // MaxSizetrackBar
         // 
         this.MaxSizetrackBar.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.MaxSizetrackBar.Location = new System.Drawing.Point(73, 74);
         this.MaxSizetrackBar.Maximum = 1000;
         this.MaxSizetrackBar.Minimum = 10;
         this.MaxSizetrackBar.Name = "MaxSizetrackBar";
         this.MaxSizetrackBar.Size = new System.Drawing.Size(235, 45);
         this.MaxSizetrackBar.TabIndex = 1;
         this.MaxSizetrackBar.TickStyle = System.Windows.Forms.TickStyle.None;
         this.MaxSizetrackBar.Value = 10;
         this.MaxSizetrackBar.Scroll += new System.EventHandler(this.MaxSizetrackBar_Scroll);
         // 
         // label1
         // 
         this.label1.AutoSize = true;
         this.label1.Location = new System.Drawing.Point(7, 44);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(47, 13);
         this.label1.TabIndex = 2;
         this.label1.Text = "Min Size";
         // 
         // label2
         // 
         this.label2.AutoSize = true;
         this.label2.Location = new System.Drawing.Point(7, 75);
         this.label2.Name = "label2";
         this.label2.Size = new System.Drawing.Size(50, 13);
         this.label2.TabIndex = 3;
         this.label2.Text = "Max Size";
         // 
         // label3
         // 
         this.label3.AutoSize = true;
         this.label3.Location = new System.Drawing.Point(7, 13);
         this.label3.Name = "label3";
         this.label3.Size = new System.Drawing.Size(46, 13);
         this.label3.TabIndex = 5;
         this.label3.Text = "Spacing";
         // 
         // SpacingtrackBar
         // 
         this.SpacingtrackBar.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.SpacingtrackBar.Location = new System.Drawing.Point(73, 12);
         this.SpacingtrackBar.Maximum = 30;
         this.SpacingtrackBar.Name = "SpacingtrackBar";
         this.SpacingtrackBar.Size = new System.Drawing.Size(235, 45);
         this.SpacingtrackBar.TabIndex = 4;
         this.SpacingtrackBar.TickStyle = System.Windows.Forms.TickStyle.None;
         this.SpacingtrackBar.Scroll += new System.EventHandler(this.SpacingtrackBar_Scroll);
         this.SpacingtrackBar.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this.SpacingtrackBar_KeyPress);
         // 
         // label5
         // 
         this.label5.AutoSize = true;
         this.label5.Location = new System.Drawing.Point(7, 106);
         this.label5.Name = "label5";
         this.label5.Size = new System.Drawing.Size(59, 13);
         this.label5.TabIndex = 7;
         this.label5.Text = "Angle Jitter";
         // 
         // AngleJittertrackBar
         // 
         this.AngleJittertrackBar.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.AngleJittertrackBar.Location = new System.Drawing.Point(72, 105);
         this.AngleJittertrackBar.Maximum = 360;
         this.AngleJittertrackBar.Name = "AngleJittertrackBar";
         this.AngleJittertrackBar.Size = new System.Drawing.Size(235, 45);
         this.AngleJittertrackBar.TabIndex = 8;
         this.AngleJittertrackBar.TickStyle = System.Windows.Forms.TickStyle.None;
         this.AngleJittertrackBar.Scroll += new System.EventHandler(this.AngleJittertrackBar_Scroll);
         // 
         // PosJittertrackBar
         // 
         this.PosJittertrackBar.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.PosJittertrackBar.Location = new System.Drawing.Point(73, 136);
         this.PosJittertrackBar.Maximum = 200;
         this.PosJittertrackBar.Name = "PosJittertrackBar";
         this.PosJittertrackBar.Size = new System.Drawing.Size(235, 45);
         this.PosJittertrackBar.TabIndex = 9;
         this.PosJittertrackBar.TickStyle = System.Windows.Forms.TickStyle.None;
         this.PosJittertrackBar.Scroll += new System.EventHandler(this.PosJittertrackBar_Scroll);
         // 
         // label6
         // 
         this.label6.AutoSize = true;
         this.label6.Location = new System.Drawing.Point(7, 137);
         this.label6.Name = "label6";
         this.label6.Size = new System.Drawing.Size(69, 13);
         this.label6.TabIndex = 10;
         this.label6.Text = "Position Jitter";
         // 
         // timer1
         // 
         this.timer1.Enabled = true;
         this.timer1.Interval = 1000;
         this.timer1.Tick += new System.EventHandler(this.timer1_Tick);
         // 
         // pictureBox1
         // 
         this.pictureBox1.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.pictureBox1.BackColor = System.Drawing.SystemColors.ControlText;
         this.pictureBox1.Location = new System.Drawing.Point(3, 219);
         this.pictureBox1.Name = "pictureBox1";
         this.pictureBox1.Size = new System.Drawing.Size(302, 120);
         this.pictureBox1.TabIndex = 11;
         this.pictureBox1.TabStop = false;
         // 
         // ScatterCounttrackBar
         // 
         this.ScatterCounttrackBar.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.ScatterCounttrackBar.Location = new System.Drawing.Point(73, 168);
         this.ScatterCounttrackBar.Minimum = 1;
         this.ScatterCounttrackBar.Name = "ScatterCounttrackBar";
         this.ScatterCounttrackBar.Size = new System.Drawing.Size(235, 45);
         this.ScatterCounttrackBar.TabIndex = 12;
         this.ScatterCounttrackBar.TickStyle = System.Windows.Forms.TickStyle.None;
         this.ScatterCounttrackBar.Value = 1;
         this.ScatterCounttrackBar.Scroll += new System.EventHandler(this.ScatterCounttrackBar_Scroll);
         // 
         // label7
         // 
         this.label7.AutoSize = true;
         this.label7.Location = new System.Drawing.Point(7, 168);
         this.label7.Name = "label7";
         this.label7.Size = new System.Drawing.Size(72, 13);
         this.label7.TabIndex = 13;
         this.label7.Text = "Scatter Count";
         // 
         // Defaultbutton
         // 
         this.Defaultbutton.Location = new System.Drawing.Point(10, 192);
         this.Defaultbutton.Name = "Defaultbutton";
         this.Defaultbutton.Size = new System.Drawing.Size(75, 21);
         this.Defaultbutton.TabIndex = 14;
         this.Defaultbutton.Text = "Default";
         this.Defaultbutton.UseVisualStyleBackColor = true;
         this.Defaultbutton.Click += new System.EventHandler(this.Defaultbutton_Click);
         // 
         // PropertiesWindow
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.AutoSize = true;
         this.Controls.Add(this.Defaultbutton);
         this.Controls.Add(this.ScatterCounttrackBar);
         this.Controls.Add(this.PosJittertrackBar);
         this.Controls.Add(this.AngleJittertrackBar);
         this.Controls.Add(this.label5);
         this.Controls.Add(this.label2);
         this.Controls.Add(this.MaxSizetrackBar);
         this.Controls.Add(this.label1);
         this.Controls.Add(this.MinSizetrackBar);
         this.Controls.Add(this.label7);
         this.Controls.Add(this.pictureBox1);
         this.Controls.Add(this.label6);
         this.Controls.Add(this.label3);
         this.Controls.Add(this.SpacingtrackBar);
         this.FloatingWindowBounds = new System.Drawing.Rectangle(0, 0, 325, 355);
         this.Key = "Properties";
         this.Name = "PropertiesWindow";
         this.Size = new System.Drawing.Size(325, 355);
         this.Text = "Brush Settings";
         this.Load += new System.EventHandler(this.PropertiesWindow_Load);
         ((System.ComponentModel.ISupportInitialize)(this.MinSizetrackBar)).EndInit();
         ((System.ComponentModel.ISupportInitialize)(this.MaxSizetrackBar)).EndInit();
         ((System.ComponentModel.ISupportInitialize)(this.SpacingtrackBar)).EndInit();
         ((System.ComponentModel.ISupportInitialize)(this.AngleJittertrackBar)).EndInit();
         ((System.ComponentModel.ISupportInitialize)(this.PosJittertrackBar)).EndInit();
         ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).EndInit();
         ((System.ComponentModel.ISupportInitialize)(this.ScatterCounttrackBar)).EndInit();
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private System.Windows.Forms.TrackBar MinSizetrackBar;
      private System.Windows.Forms.TrackBar MaxSizetrackBar;
      private System.Windows.Forms.Label label1;
      private System.Windows.Forms.Label label2;
      private System.Windows.Forms.Label label3;
      private System.Windows.Forms.TrackBar SpacingtrackBar;
      private System.Windows.Forms.Label label5;
      private System.Windows.Forms.TrackBar AngleJittertrackBar;
      private System.Windows.Forms.TrackBar PosJittertrackBar;
      private System.Windows.Forms.Label label6;
      private System.Windows.Forms.Timer timer1;
      private System.Windows.Forms.PictureBox pictureBox1;
      private System.Windows.Forms.TrackBar ScatterCounttrackBar;
      private System.Windows.Forms.Label label7;
      private System.Windows.Forms.Button Defaultbutton;

   }
}