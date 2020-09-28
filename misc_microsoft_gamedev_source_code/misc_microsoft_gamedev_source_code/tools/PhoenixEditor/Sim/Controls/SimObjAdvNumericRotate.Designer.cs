namespace SimEditor.Controls
{
   partial class SimObjAdvNumericRotate
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
         this.label1 = new System.Windows.Forms.Label();
         this.label2 = new System.Windows.Forms.Label();
         this.label3 = new System.Windows.Forms.Label();
         this.button1 = new System.Windows.Forms.Button();
         this.xValue = new EditorCore.NumericSliderControl();
         this.yValue = new EditorCore.NumericSliderControl();
         this.zValue = new EditorCore.NumericSliderControl();
         this.checkBox1 = new System.Windows.Forms.CheckBox();
         this.SuspendLayout();
         // 
         // label1
         // 
         this.label1.AutoSize = true;
         this.label1.Location = new System.Drawing.Point(8, 33);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(20, 13);
         this.label1.TabIndex = 0;
         this.label1.Text = "X :";
         // 
         // label2
         // 
         this.label2.AutoSize = true;
         this.label2.Location = new System.Drawing.Point(8, 69);
         this.label2.Name = "label2";
         this.label2.Size = new System.Drawing.Size(20, 13);
         this.label2.TabIndex = 1;
         this.label2.Text = "Y :";
         // 
         // label3
         // 
         this.label3.AutoSize = true;
         this.label3.Location = new System.Drawing.Point(8, 107);
         this.label3.Name = "label3";
         this.label3.Size = new System.Drawing.Size(20, 13);
         this.label3.TabIndex = 2;
         this.label3.Text = "Z :";
         // 
         // button1
         // 
         this.button1.Location = new System.Drawing.Point(127, 135);
         this.button1.Name = "button1";
         this.button1.Size = new System.Drawing.Size(75, 23);
         this.button1.TabIndex = 5;
         this.button1.Text = "Apply";
         this.button1.UseVisualStyleBackColor = true;
         this.button1.Click += new System.EventHandler(this.button1_Click);
         // 
         // xValue
         // 
         this.xValue.BackColor = System.Drawing.SystemColors.ControlLight;
         this.xValue.Location = new System.Drawing.Point(34, 33);
         this.xValue.Name = "xValue";
         this.xValue.Size = new System.Drawing.Size(168, 22);
         this.xValue.TabIndex = 6;
         // 
         // yValue
         // 
         this.yValue.BackColor = System.Drawing.SystemColors.ControlLight;
         this.yValue.Location = new System.Drawing.Point(34, 69);
         this.yValue.Name = "yValue";
         this.yValue.Size = new System.Drawing.Size(168, 22);
         this.yValue.TabIndex = 7;
         // 
         // zValue
         // 
         this.zValue.BackColor = System.Drawing.SystemColors.ControlLight;
         this.zValue.Location = new System.Drawing.Point(34, 107);
         this.zValue.Name = "zValue";
         this.zValue.Size = new System.Drawing.Size(168, 22);
         this.zValue.TabIndex = 8;
         // 
         // checkBox1
         // 
         this.checkBox1.AutoSize = true;
         this.checkBox1.Location = new System.Drawing.Point(127, 10);
         this.checkBox1.Name = "checkBox1";
         this.checkBox1.Size = new System.Drawing.Size(67, 17);
         this.checkBox1.TabIndex = 9;
         this.checkBox1.Text = "Grouped";
         this.checkBox1.UseVisualStyleBackColor = true;
         // 
         // SimObjAdvNumericRotate
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.ClientSize = new System.Drawing.Size(214, 171);
         this.Controls.Add(this.checkBox1);
         this.Controls.Add(this.zValue);
         this.Controls.Add(this.yValue);
         this.Controls.Add(this.xValue);
         this.Controls.Add(this.button1);
         this.Controls.Add(this.label3);
         this.Controls.Add(this.label2);
         this.Controls.Add(this.label1);
         this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedToolWindow;
         this.Name = "SimObjAdvNumericRotate";
         this.Text = "Advanced Rotate";
         this.Activated += new System.EventHandler(this.SimObjAdvNumericMove_Activated);
         this.Load += new System.EventHandler(this.SimObjAdvNumericMove_Load);
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private System.Windows.Forms.Label label1;
      private System.Windows.Forms.Label label2;
      private System.Windows.Forms.Label label3;
      private System.Windows.Forms.Button button1;
      private EditorCore.NumericSliderControl xValue;
      private EditorCore.NumericSliderControl yValue;
      private EditorCore.NumericSliderControl zValue;
      private System.Windows.Forms.CheckBox checkBox1;
   }
}