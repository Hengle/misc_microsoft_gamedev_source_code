namespace RemoteMemory
{
   partial class xboxConnectOptions
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
         this.button4 = new System.Windows.Forms.Button();
         this.label3 = new System.Windows.Forms.Label();
         this.textBox2 = new System.Windows.Forms.TextBox();
         this.button3 = new System.Windows.Forms.Button();
         this.button2 = new System.Windows.Forms.Button();
         this.loadOptions = new RemoteMemory.LoadOptions();
         this.SuspendLayout();
         // 
         // button4
         // 
         this.button4.Location = new System.Drawing.Point(467, 25);
         this.button4.Name = "button4";
         this.button4.Size = new System.Drawing.Size(30, 23);
         this.button4.TabIndex = 16;
         this.button4.Text = "...";
         this.button4.UseVisualStyleBackColor = true;
         this.button4.Click += new System.EventHandler(this.button4_Click);
         // 
         // label3
         // 
         this.label3.AutoSize = true;
         this.label3.Location = new System.Drawing.Point(12, 30);
         this.label3.Name = "label3";
         this.label3.Size = new System.Drawing.Size(53, 13);
         this.label3.TabIndex = 15;
         this.label3.Text = "EXE File :";
         // 
         // textBox2
         // 
         this.textBox2.Location = new System.Drawing.Point(109, 27);
         this.textBox2.Name = "textBox2";
         this.textBox2.Size = new System.Drawing.Size(352, 20);
         this.textBox2.TabIndex = 14;
         // 
         // button3
         // 
         this.button3.Location = new System.Drawing.Point(354, 144);
         this.button3.Name = "button3";
         this.button3.Size = new System.Drawing.Size(75, 23);
         this.button3.TabIndex = 11;
         this.button3.Text = "GO!";
         this.button3.UseVisualStyleBackColor = true;
         this.button3.Click += new System.EventHandler(this.button2_Click);
         // 
         // button2
         // 
         this.button2.Location = new System.Drawing.Point(435, 144);
         this.button2.Name = "button2";
         this.button2.Size = new System.Drawing.Size(75, 23);
         this.button2.TabIndex = 10;
         this.button2.Text = "Cancel";
         this.button2.UseVisualStyleBackColor = true;
         this.button2.Click += new System.EventHandler(this.button3_Click);
         // 
         // loadOptions
         // 
         this.loadOptions.Location = new System.Drawing.Point(15, 54);
         this.loadOptions.Name = "loadOptions";
         this.loadOptions.Size = new System.Drawing.Size(485, 84);
         this.loadOptions.TabIndex = 17;
         // 
         // xboxConnectOptions
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.ClientSize = new System.Drawing.Size(522, 183);
         this.Controls.Add(this.loadOptions);
         this.Controls.Add(this.button4);
         this.Controls.Add(this.label3);
         this.Controls.Add(this.textBox2);
         this.Controls.Add(this.button3);
         this.Controls.Add(this.button2);
         this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedToolWindow;
         this.Name = "xboxConnectOptions";
         this.Text = "xboxConnectOptions";
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private System.Windows.Forms.Button button4;
      private System.Windows.Forms.Label label3;
      private System.Windows.Forms.TextBox textBox2;
      private System.Windows.Forms.Button button3;
      private System.Windows.Forms.Button button2;
      private LoadOptions loadOptions;
   }
}