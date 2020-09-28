namespace VisualEditor.PropertyPages
{
   partial class LogicNodeVariationPage
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
         this.groupBox1 = new System.Windows.Forms.GroupBox();
         this.comboBox1 = new System.Windows.Forms.ComboBox();
         this.label1 = new System.Windows.Forms.Label();
         this.weightNumericUpDown1 = new System.Windows.Forms.NumericUpDown();
         this.label3 = new System.Windows.Forms.Label();
         this.textBoxName = new System.Windows.Forms.TextBox();
         this.label2 = new System.Windows.Forms.Label();
         this.groupBox1.SuspendLayout();
         ((System.ComponentModel.ISupportInitialize)(this.weightNumericUpDown1)).BeginInit();
         this.SuspendLayout();
         // 
         // groupBox1
         // 
         this.groupBox1.Anchor = System.Windows.Forms.AnchorStyles.None;
         this.groupBox1.Controls.Add(this.comboBox1);
         this.groupBox1.Controls.Add(this.label1);
         this.groupBox1.Controls.Add(this.weightNumericUpDown1);
         this.groupBox1.Controls.Add(this.label3);
         this.groupBox1.Controls.Add(this.textBoxName);
         this.groupBox1.Controls.Add(this.label2);
         this.groupBox1.Location = new System.Drawing.Point(0, 0);
         this.groupBox1.Name = "groupBox1";
         this.groupBox1.Padding = new System.Windows.Forms.Padding(5, 16, 5, 3);
         this.groupBox1.Size = new System.Drawing.Size(350, 250);
         this.groupBox1.TabIndex = 2;
         this.groupBox1.TabStop = false;
         this.groupBox1.Text = "Logic (Variation) Properties";
         // 
         // comboBox1
         // 
         this.comboBox1.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
         this.comboBox1.FormattingEnabled = true;
         this.comboBox1.Location = new System.Drawing.Point(102, 84);
         this.comboBox1.Name = "comboBox1";
         this.comboBox1.Size = new System.Drawing.Size(240, 21);
         this.comboBox1.TabIndex = 4;
         this.comboBox1.SelectedIndexChanged += new System.EventHandler(this.comboBox1_SelectedIndexChanged);
         // 
         // label1
         // 
         this.label1.AutoSize = true;
         this.label1.Location = new System.Drawing.Point(8, 87);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(56, 13);
         this.label1.TabIndex = 3;
         this.label1.Text = "Ref Model";
         // 
         // weightNumericUpDown1
         // 
         this.weightNumericUpDown1.Location = new System.Drawing.Point(102, 58);
         this.weightNumericUpDown1.Name = "weightNumericUpDown1";
         this.weightNumericUpDown1.Size = new System.Drawing.Size(97, 20);
         this.weightNumericUpDown1.TabIndex = 7;
         this.weightNumericUpDown1.Value = new decimal(new int[] {
            1,
            0,
            0,
            0});
         this.weightNumericUpDown1.ValueChanged += new System.EventHandler(this.weightNumericUpDown1_ValueChanged);
         // 
         // label3
         // 
         this.label3.AutoSize = true;
         this.label3.Location = new System.Drawing.Point(8, 60);
         this.label3.Margin = new System.Windows.Forms.Padding(3);
         this.label3.Name = "label3";
         this.label3.Size = new System.Drawing.Size(41, 13);
         this.label3.TabIndex = 6;
         this.label3.Text = "Weight";
         // 
         // textBoxName
         // 
         this.textBoxName.Location = new System.Drawing.Point(102, 32);
         this.textBoxName.Name = "textBoxName";
         this.textBoxName.Size = new System.Drawing.Size(240, 20);
         this.textBoxName.TabIndex = 5;
         this.textBoxName.TextChanged += new System.EventHandler(this.textBoxName_TextChanged);
         // 
         // label2
         // 
         this.label2.AutoSize = true;
         this.label2.Location = new System.Drawing.Point(8, 35);
         this.label2.Name = "label2";
         this.label2.Size = new System.Drawing.Size(35, 13);
         this.label2.TabIndex = 4;
         this.label2.Text = "Name";
         // 
         // LogicNodeVariationPage
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.groupBox1);
         this.Name = "LogicNodeVariationPage";
         this.Size = new System.Drawing.Size(350, 250);
         this.groupBox1.ResumeLayout(false);
         this.groupBox1.PerformLayout();
         ((System.ComponentModel.ISupportInitialize)(this.weightNumericUpDown1)).EndInit();
         this.ResumeLayout(false);

      }

      #endregion

      private System.Windows.Forms.GroupBox groupBox1;
      private System.Windows.Forms.TextBox textBoxName;
      private System.Windows.Forms.Label label2;
      private System.Windows.Forms.NumericUpDown weightNumericUpDown1;
      private System.Windows.Forms.Label label3;
      private System.Windows.Forms.ComboBox comboBox1;
      private System.Windows.Forms.Label label1;
   }
}
