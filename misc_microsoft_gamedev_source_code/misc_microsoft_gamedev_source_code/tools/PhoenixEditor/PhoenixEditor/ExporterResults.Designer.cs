namespace PhoenixEditor
{
   partial class ExporterResults
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
         this.zg1 = new ZedGraph.ZedGraphControl();
         this.button2 = new System.Windows.Forms.Button();
         this.SuspendLayout();
         // 
         // button1
         // 
         this.button1.Location = new System.Drawing.Point(252, 402);
         this.button1.Name = "button1";
         this.button1.Size = new System.Drawing.Size(75, 23);
         this.button1.TabIndex = 0;
         this.button1.Text = "OK";
         this.button1.UseVisualStyleBackColor = true;
         this.button1.Click += new System.EventHandler(this.button1_Click);
         // 
         // listBox1
         // 
         this.listBox1.FormattingEnabled = true;
         this.listBox1.Location = new System.Drawing.Point(576, 12);
         this.listBox1.Name = "listBox1";
         this.listBox1.Size = new System.Drawing.Size(268, 381);
         this.listBox1.TabIndex = 1;
         // 
         // zg1
         // 
         this.zg1.EditButtons = System.Windows.Forms.MouseButtons.Left;
         this.zg1.Location = new System.Drawing.Point(12, 12);
         this.zg1.Name = "zg1";
         this.zg1.PanModifierKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Shift | System.Windows.Forms.Keys.None)));
         this.zg1.ScrollGrace = 0;
         this.zg1.ScrollMaxX = 0;
         this.zg1.ScrollMaxY = 0;
         this.zg1.ScrollMaxY2 = 0;
         this.zg1.ScrollMinX = 0;
         this.zg1.ScrollMinY = 0;
         this.zg1.ScrollMinY2 = 0;
         this.zg1.Size = new System.Drawing.Size(517, 384);
         this.zg1.TabIndex = 2;
         // 
         // button2
         // 
         this.button2.Location = new System.Drawing.Point(535, 127);
         this.button2.Name = "button2";
         this.button2.Size = new System.Drawing.Size(30, 23);
         this.button2.TabIndex = 3;
         this.button2.Text = ">>";
         this.button2.UseVisualStyleBackColor = true;
         this.button2.Click += new System.EventHandler(this.button2_Click);
         // 
         // ExporterResults
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.ClientSize = new System.Drawing.Size(568, 438);
         this.Controls.Add(this.button2);
         this.Controls.Add(this.zg1);
         this.Controls.Add(this.listBox1);
         this.Controls.Add(this.button1);
         this.Name = "ExporterResults";
         this.Text = "ExporterResults";
         this.Load += new System.EventHandler(this.ExporterResults_Load);
         this.ResumeLayout(false);

      }

      #endregion

      private System.Windows.Forms.Button button1;
      private System.Windows.Forms.ListBox listBox1;
      private ZedGraph.ZedGraphControl zg1;
      private System.Windows.Forms.Button button2;
   }
}