namespace RemoteMemory
{
   partial class HeapLines
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
         this.timer1 = new System.Windows.Forms.Timer(this.components);
         this.label2 = new System.Windows.Forms.Label();
         this.fastTimeLine = new RemoteMemory.fastTimeline();
         this.hScrollBar1 = new System.Windows.Forms.HScrollBar();
         this.SuspendLayout();
         // 
         // timer1
         // 
         this.timer1.Interval = 1000;
         this.timer1.Tick += new System.EventHandler(this.timer1_Tick);
         // 
         // label2
         // 
         this.label2.AutoSize = true;
         this.label2.Font = new System.Drawing.Font("Arial", 12F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
         this.label2.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(220)))), ((int)(((byte)(220)))), ((int)(((byte)(204)))));
         this.label2.Location = new System.Drawing.Point(3, 0);
         this.label2.Name = "label2";
         this.label2.Size = new System.Drawing.Size(83, 19);
         this.label2.TabIndex = 11;
         this.label2.Text = "Time Line";
         // 
         // fastTimeLine
         // 
         this.fastTimeLine.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.fastTimeLine.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(63)))), ((int)(((byte)(63)))), ((int)(((byte)(63)))));
         this.fastTimeLine.Location = new System.Drawing.Point(0, 22);
         this.fastTimeLine.Name = "fastTimeLine";
         this.fastTimeLine.Size = new System.Drawing.Size(488, 178);
         this.fastTimeLine.TabIndex = 14;
         // 
         // hScrollBar1
         // 
         this.hScrollBar1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.hScrollBar1.Location = new System.Drawing.Point(0, 203);
         this.hScrollBar1.Name = "hScrollBar1";
         this.hScrollBar1.Size = new System.Drawing.Size(488, 22);
         this.hScrollBar1.TabIndex = 13;
         this.hScrollBar1.ValueChanged += new System.EventHandler(this.hScrollBar1_ValueChanged);
         // 
         // HeapLines
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(43)))), ((int)(((byte)(43)))), ((int)(((byte)(43)))));
         this.Controls.Add(this.hScrollBar1);
         this.Controls.Add(this.fastTimeLine);
         this.Controls.Add(this.label2);
         this.Name = "HeapLines";
         this.Size = new System.Drawing.Size(488, 225);
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private System.Windows.Forms.Timer timer1;
      private System.Windows.Forms.Label label2;
      private fastTimeline fastTimeLine;
      private System.Windows.Forms.HScrollBar hScrollBar1;
   }
}
