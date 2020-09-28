namespace RemoteMemory
{
   partial class FileGroupings
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
         this.tabControl1 = new System.Windows.Forms.TabControl();
         this.label2 = new System.Windows.Forms.Label();
         this.button3 = new System.Windows.Forms.Button();
         this.button1 = new System.Windows.Forms.Button();
         this.SuspendLayout();
         // 
         // tabControl1
         // 
         this.tabControl1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.tabControl1.Appearance = System.Windows.Forms.TabAppearance.FlatButtons;
         this.tabControl1.Location = new System.Drawing.Point(3, 35);
         this.tabControl1.Name = "tabControl1";
         this.tabControl1.Padding = new System.Drawing.Point(0, 0);
         this.tabControl1.SelectedIndex = 0;
         this.tabControl1.Size = new System.Drawing.Size(438, 304);
         this.tabControl1.TabIndex = 0;
         this.tabControl1.Visible = false;
         
         // 
         // label2
         // 
         this.label2.AutoSize = true;
         this.label2.Font = new System.Drawing.Font("Arial", 12F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
         this.label2.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(220)))), ((int)(((byte)(220)))), ((int)(((byte)(204)))));
         this.label2.Location = new System.Drawing.Point(3, 7);
         this.label2.Name = "label2";
         this.label2.Size = new System.Drawing.Size(121, 19);
         this.label2.TabIndex = 14;
         this.label2.Text = "File Groupings";
         // 
         // button3
         // 
         this.button3.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
         this.button3.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
         this.button3.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(220)))), ((int)(((byte)(220)))), ((int)(((byte)(204)))));
         this.button3.Location = new System.Drawing.Point(368, 3);
         this.button3.Name = "button3";
         this.button3.Size = new System.Drawing.Size(73, 23);
         this.button3.TabIndex = 18;
         this.button3.Text = "Load List";
         this.button3.UseVisualStyleBackColor = true;
         this.button3.Click += new System.EventHandler(this.button3_Click);
         // 
         // button1
         // 
         this.button1.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
         this.button1.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
         this.button1.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(220)))), ((int)(((byte)(220)))), ((int)(((byte)(204)))));
         this.button1.Location = new System.Drawing.Point(289, 3);
         this.button1.Name = "button1";
         this.button1.Size = new System.Drawing.Size(73, 23);
         this.button1.TabIndex = 19;
         this.button1.Text = "To Excel";
         this.button1.UseVisualStyleBackColor = true;
         this.button1.Click += new System.EventHandler(this.button1_Click);
         // 
         // FileGroupings
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(53)))), ((int)(((byte)(53)))), ((int)(((byte)(53)))));
         this.Controls.Add(this.button1);
         this.Controls.Add(this.button3);
         this.Controls.Add(this.label2);
         this.Controls.Add(this.tabControl1);
         this.Name = "FileGroupings";
         this.Size = new System.Drawing.Size(444, 342);
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private System.Windows.Forms.TabControl tabControl1;
      private System.Windows.Forms.Label label2;
      private System.Windows.Forms.Button button3;
      private System.Windows.Forms.Button button1;
   }
}
