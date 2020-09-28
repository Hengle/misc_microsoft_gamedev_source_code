namespace PhoenixEditor.FilterDialogs
{
   partial class MemoryEstimateDlg
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
         this.button1 = new System.Windows.Forms.Button();
         this.listBox1 = new System.Windows.Forms.ListBox();
         this.zedGraphControl1 = new ZedGraph.ZedGraphControl();
         this.button2 = new System.Windows.Forms.Button();
         this.timer1 = new System.Windows.Forms.Timer(this.components);
         this.button3 = new System.Windows.Forms.Button();
         this.SuspendLayout();
         // 
         // button1
         // 
         this.button1.Location = new System.Drawing.Point(787, 418);
         this.button1.Name = "button1";
         this.button1.Size = new System.Drawing.Size(75, 23);
         this.button1.TabIndex = 4;
         this.button1.Text = "OK";
         this.button1.UseVisualStyleBackColor = true;
         this.button1.Click += new System.EventHandler(this.button1_Click);
         // 
         // listBox1
         // 
         this.listBox1.FormattingEnabled = true;
         this.listBox1.Location = new System.Drawing.Point(570, 12);
         this.listBox1.Name = "listBox1";
         this.listBox1.Size = new System.Drawing.Size(292, 394);
         this.listBox1.TabIndex = 6;
         // 
         // zedGraphControl1
         // 
         this.zedGraphControl1.EditButtons = System.Windows.Forms.MouseButtons.Left;
         this.zedGraphControl1.Location = new System.Drawing.Point(12, 12);
         this.zedGraphControl1.Name = "zedGraphControl1";
         this.zedGraphControl1.PanModifierKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Shift | System.Windows.Forms.Keys.None)));
         this.zedGraphControl1.ScrollGrace = 0;
         this.zedGraphControl1.ScrollMaxX = 0;
         this.zedGraphControl1.ScrollMaxY = 0;
         this.zedGraphControl1.ScrollMaxY2 = 0;
         this.zedGraphControl1.ScrollMinX = 0;
         this.zedGraphControl1.ScrollMinY = 0;
         this.zedGraphControl1.ScrollMinY2 = 0;
         this.zedGraphControl1.Size = new System.Drawing.Size(543, 400);
         this.zedGraphControl1.TabIndex = 7;
         // 
         // button2
         // 
         this.button2.Location = new System.Drawing.Point(706, 418);
         this.button2.Name = "button2";
         this.button2.Size = new System.Drawing.Size(75, 23);
         this.button2.TabIndex = 8;
         this.button2.Text = "Recalculate";
         this.button2.UseVisualStyleBackColor = true;
         this.button2.Click += new System.EventHandler(this.button2_Click);
         // 
         // timer1
         // 
         this.timer1.Interval = 1000;
         this.timer1.Tick += new System.EventHandler(this.timer1_Tick);
         // 
         // button3
         // 
         this.button3.Location = new System.Drawing.Point(246, 418);
         this.button3.Name = "button3";
         this.button3.Size = new System.Drawing.Size(104, 23);
         this.button3.TabIndex = 9;
         this.button3.Text = "Advanced View";
         this.button3.UseVisualStyleBackColor = true;
         this.button3.Click += new System.EventHandler(this.button3_Click);
         // 
         // MemoryEstimateDlg
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.ClientSize = new System.Drawing.Size(874, 459);
         this.Controls.Add(this.button3);
         this.Controls.Add(this.button2);
         this.Controls.Add(this.zedGraphControl1);
         this.Controls.Add(this.listBox1);
         this.Controls.Add(this.button1);
         this.Name = "MemoryEstimateDlg";
         this.Text = "MemoryEstimate";
         this.Load += new System.EventHandler(this.MemoryEstimate_Load);
         this.ResumeLayout(false);

      }

      #endregion

      private System.Windows.Forms.Button button1;
      private System.Windows.Forms.ListBox listBox1;
      private ZedGraph.ZedGraphControl zedGraphControl1;
      private System.Windows.Forms.Button button2;
      private System.Windows.Forms.Timer timer1;
      private System.Windows.Forms.Button button3;
   }
}