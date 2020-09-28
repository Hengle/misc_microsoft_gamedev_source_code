namespace RemoteMemory
{
   partial class FileTimelineElement
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
         this.components = new System.ComponentModel.Container();
         this.pictureBox1 = new System.Windows.Forms.PictureBox();
         this.FilenameLabel = new System.Windows.Forms.Label();
         this.mTimeline = new RemoteMemory.fastTimeline();
         this.hScrollBar1 = new System.Windows.Forms.HScrollBar();
         this.timer1 = new System.Windows.Forms.Timer(this.components);
         ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).BeginInit();
         this.SuspendLayout();
         // 
         // pictureBox1
         // 
         this.pictureBox1.Image = global::RemoteMemory.Properties.Resources.stop;
         this.pictureBox1.InitialImage = global::RemoteMemory.Properties.Resources.stop;
         this.pictureBox1.Location = new System.Drawing.Point(374, 3);
         this.pictureBox1.Name = "pictureBox1";
         this.pictureBox1.Size = new System.Drawing.Size(16, 16);
         this.pictureBox1.TabIndex = 0;
         this.pictureBox1.TabStop = false;
         this.pictureBox1.Click += new System.EventHandler(this.pictureBox1_Click);
         // 
         // FilenameLabel
         // 
         this.FilenameLabel.AutoSize = true;
         this.FilenameLabel.Font = new System.Drawing.Font("Arial", 12F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
         this.FilenameLabel.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(220)))), ((int)(((byte)(220)))), ((int)(((byte)(204)))));
         this.FilenameLabel.Location = new System.Drawing.Point(3, 3);
         this.FilenameLabel.Name = "FilenameLabel";
         this.FilenameLabel.Size = new System.Drawing.Size(73, 19);
         this.FilenameLabel.TabIndex = 14;
         this.FilenameLabel.Text = "filename";
         // 
         // mTimeline
         // 
         this.mTimeline.AutoSize = true;
         this.mTimeline.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(63)))), ((int)(((byte)(63)))), ((int)(((byte)(63)))));
         this.mTimeline.Location = new System.Drawing.Point(0, 31);
         this.mTimeline.Name = "mTimeline";
         this.mTimeline.Size = new System.Drawing.Size(390, 146);
         this.mTimeline.TabIndex = 14;
         // 
         // hScrollBar1
         // 
         this.hScrollBar1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.hScrollBar1.Location = new System.Drawing.Point(0, 180);
         this.hScrollBar1.Name = "hScrollBar1";
         this.hScrollBar1.Size = new System.Drawing.Size(393, 22);
         this.hScrollBar1.TabIndex = 15;
         this.hScrollBar1.ValueChanged += new System.EventHandler(this.hScrollBar1_ValueChanged);
         // 
         // timer1
         // 
         this.timer1.Tick += new System.EventHandler(this.timer1_Tick);
         // 
         // FileTimelineElement
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(53)))), ((int)(((byte)(53)))), ((int)(((byte)(53)))));
         this.Controls.Add(this.hScrollBar1);
         this.Controls.Add(this.FilenameLabel);
         this.Controls.Add(this.pictureBox1);
         this.Controls.Add(this.mTimeline);
         this.Name = "FileTimelineElement";
         this.Size = new System.Drawing.Size(393, 202);
         ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).EndInit();
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private System.Windows.Forms.PictureBox pictureBox1;
      private System.Windows.Forms.Label FilenameLabel;
      private fastTimeline mTimeline;
      private System.Windows.Forms.HScrollBar hScrollBar1;
      private System.Windows.Forms.Timer timer1;
   }
}
