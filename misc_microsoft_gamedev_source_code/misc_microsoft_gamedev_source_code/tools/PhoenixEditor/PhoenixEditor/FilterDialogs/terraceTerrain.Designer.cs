namespace PhoenixEditor.Filter_Dialogs
{
   partial class terraceTerrain
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
         this.button2 = new System.Windows.Forms.Button();
         this.checkBox1 = new System.Windows.Forms.CheckBox();
         this.numericUpDown1 = new System.Windows.Forms.NumericUpDown();
         this.button1 = new System.Windows.Forms.Button();
         this.label1 = new System.Windows.Forms.Label();
         ((System.ComponentModel.ISupportInitialize)(this.numericUpDown1)).BeginInit();
         this.SuspendLayout();
         // 
         // button2
         // 
         this.button2.Location = new System.Drawing.Point(201, 89);
         this.button2.Name = "button2";
         this.button2.Size = new System.Drawing.Size(79, 28);
         this.button2.TabIndex = 1;
         this.button2.Text = "Cancel";
         this.button2.UseVisualStyleBackColor = true;
         this.button2.Click += new System.EventHandler(this.button2_Click);
         // 
         // checkBox1
         // 
         this.checkBox1.AutoSize = true;
         this.checkBox1.Checked = true;
         this.checkBox1.CheckState = System.Windows.Forms.CheckState.Checked;
         this.checkBox1.Location = new System.Drawing.Point(111, 66);
         this.checkBox1.Name = "checkBox1";
         this.checkBox1.Size = new System.Drawing.Size(166, 17);
         this.checkBox1.TabIndex = 2;
         this.checkBox1.Text = "Apply Only To Masked Terain";
         this.checkBox1.UseVisualStyleBackColor = true;
         // 
         // numericUpDown1
         // 
         this.numericUpDown1.Location = new System.Drawing.Point(124, 14);
         this.numericUpDown1.Name = "numericUpDown1";
         this.numericUpDown1.Size = new System.Drawing.Size(120, 20);
         this.numericUpDown1.TabIndex = 3;
         this.numericUpDown1.Value = new decimal(new int[] {
            18,
            0,
            0,
            0});
         // 
         // button1
         // 
         this.button1.Location = new System.Drawing.Point(111, 89);
         this.button1.Name = "button1";
         this.button1.Size = new System.Drawing.Size(79, 28);
         this.button1.TabIndex = 4;
         this.button1.Text = "Apply";
         this.button1.UseVisualStyleBackColor = true;
         this.button1.Click += new System.EventHandler(this.button1_Click);
         // 
         // label1
         // 
         this.label1.AutoSize = true;
         this.label1.Location = new System.Drawing.Point(15, 14);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(103, 13);
         this.label1.TabIndex = 5;
         this.label1.Text = "Number Of Terraces";
         // 
         // terraceTerrain
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.ClientSize = new System.Drawing.Size(292, 129);
         this.Controls.Add(this.label1);
         this.Controls.Add(this.button1);
         this.Controls.Add(this.numericUpDown1);
         this.Controls.Add(this.checkBox1);
         this.Controls.Add(this.button2);
         this.Name = "terraceTerrain";
         this.Text = "terraceTerrain";
         ((System.ComponentModel.ISupportInitialize)(this.numericUpDown1)).EndInit();
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private System.Windows.Forms.Button button2;
      private System.Windows.Forms.CheckBox checkBox1;
      private System.Windows.Forms.NumericUpDown numericUpDown1;
      private System.Windows.Forms.Button button1;
      private System.Windows.Forms.Label label1;

   }
}