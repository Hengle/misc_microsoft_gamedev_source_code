namespace RemoteMemory
{
   partial class FileTimelines
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
         this.label2 = new System.Windows.Forms.Label();
         this.button1 = new System.Windows.Forms.Button();
         this.flowLayoutPanel1 = new System.Windows.Forms.FlowLayoutPanel();
         this.timer1 = new System.Windows.Forms.Timer(this.components);
         this.button2 = new System.Windows.Forms.Button();
         this.button3 = new System.Windows.Forms.Button();
         this.SuspendLayout();
         // 
         // label2
         // 
         this.label2.AutoSize = true;
         this.label2.Font = new System.Drawing.Font("Arial", 12F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
         this.label2.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(220)))), ((int)(((byte)(220)))), ((int)(((byte)(204)))));
         this.label2.Location = new System.Drawing.Point(3, 0);
         this.label2.Name = "label2";
         this.label2.Size = new System.Drawing.Size(99, 19);
         this.label2.TabIndex = 13;
         this.label2.Text = "Per File Info";
         // 
         // button1
         // 
         this.button1.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
         this.button1.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(220)))), ((int)(((byte)(220)))), ((int)(((byte)(204)))));
         this.button1.Location = new System.Drawing.Point(7, 22);
         this.button1.Name = "button1";
         this.button1.Size = new System.Drawing.Size(199, 23);
         this.button1.TabIndex = 14;
         this.button1.Text = "Add a new file to watch";
         this.button1.UseVisualStyleBackColor = true;
         this.button1.Click += new System.EventHandler(this.button1_Click);
         // 
         // flowLayoutPanel1
         // 
         this.flowLayoutPanel1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.flowLayoutPanel1.AutoScroll = true;
         this.flowLayoutPanel1.FlowDirection = System.Windows.Forms.FlowDirection.TopDown;
         this.flowLayoutPanel1.Location = new System.Drawing.Point(7, 61);
         this.flowLayoutPanel1.Name = "flowLayoutPanel1";
         this.flowLayoutPanel1.Size = new System.Drawing.Size(468, 834);
         this.flowLayoutPanel1.TabIndex = 15;
         this.flowLayoutPanel1.WrapContents = false;
         // 
         // timer1
         // 
         this.timer1.Tick += new System.EventHandler(this.timer1_Tick);
         // 
         // button2
         // 
         this.button2.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
         this.button2.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
         this.button2.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(220)))), ((int)(((byte)(220)))), ((int)(((byte)(204)))));
         this.button2.Location = new System.Drawing.Point(402, 22);
         this.button2.Name = "button2";
         this.button2.Size = new System.Drawing.Size(73, 23);
         this.button2.TabIndex = 16;
         this.button2.Text = "Save List";
         this.button2.UseVisualStyleBackColor = true;
         this.button2.Click += new System.EventHandler(this.button2_Click);
         // 
         // button3
         // 
         this.button3.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
         this.button3.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
         this.button3.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(220)))), ((int)(((byte)(220)))), ((int)(((byte)(204)))));
         this.button3.Location = new System.Drawing.Point(319, 22);
         this.button3.Name = "button3";
         this.button3.Size = new System.Drawing.Size(73, 23);
         this.button3.TabIndex = 17;
         this.button3.Text = "Load List";
         this.button3.UseVisualStyleBackColor = true;
         this.button3.Click += new System.EventHandler(this.button3_Click);
         // 
         // FileTimelines
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(53)))), ((int)(((byte)(53)))), ((int)(((byte)(53)))));
         this.Controls.Add(this.button3);
         this.Controls.Add(this.button2);
         this.Controls.Add(this.flowLayoutPanel1);
         this.Controls.Add(this.button1);
         this.Controls.Add(this.label2);
         this.Name = "FileTimelines";
         this.Size = new System.Drawing.Size(478, 908);
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private System.Windows.Forms.Label label2;
      private System.Windows.Forms.Button button1;
      private System.Windows.Forms.FlowLayoutPanel flowLayoutPanel1;
      private System.Windows.Forms.Timer timer1;
      private System.Windows.Forms.Button button2;
      private System.Windows.Forms.Button button3;
   }
}
