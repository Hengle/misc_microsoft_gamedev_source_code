namespace EditorCore.Controls
{
   partial class LocStringPicker
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
         this.IDTextBox = new System.Windows.Forms.TextBox();
         this.StringTextLabel = new System.Windows.Forms.Label();
         this.ScenarioComboBox = new System.Windows.Forms.ComboBox();
         this.label1 = new System.Windows.Forms.Label();
         this.locStringList = new System.Windows.Forms.ListBox();
         this.label2 = new System.Windows.Forms.Label();
         this.CategoryComboBox = new System.Windows.Forms.ComboBox();
         this.SuspendLayout();
         // 
         // IDTextBox
         // 
         this.IDTextBox.Location = new System.Drawing.Point(4, 4);
         this.IDTextBox.Name = "IDTextBox";
         this.IDTextBox.Size = new System.Drawing.Size(100, 20);
         this.IDTextBox.TabIndex = 0;
         this.IDTextBox.TextChanged += new System.EventHandler(this.IDTextBox_TextChanged);
         // 
         // StringTextLabel
         // 
         this.StringTextLabel.AutoSize = true;
         this.StringTextLabel.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
         this.StringTextLabel.Location = new System.Drawing.Point(110, 7);
         this.StringTextLabel.Name = "StringTextLabel";
         this.StringTextLabel.Size = new System.Drawing.Size(41, 13);
         this.StringTextLabel.TabIndex = 1;
         this.StringTextLabel.Text = "label1";
         // 
         // ScenarioComboBox
         // 
         this.ScenarioComboBox.FormattingEnabled = true;
         this.ScenarioComboBox.Location = new System.Drawing.Point(58, 38);
         this.ScenarioComboBox.Name = "ScenarioComboBox";
         this.ScenarioComboBox.Size = new System.Drawing.Size(124, 21);
         this.ScenarioComboBox.TabIndex = 2;
         this.ScenarioComboBox.SelectedIndexChanged += new System.EventHandler(this.ScenarioComboBox_SelectedIndexChanged);
         // 
         // label1
         // 
         this.label1.AutoSize = true;
         this.label1.Location = new System.Drawing.Point(3, 41);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(49, 13);
         this.label1.TabIndex = 6;
         this.label1.Text = "Scenario";
         // 
         // locStringList
         // 
         this.locStringList.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.locStringList.FormattingEnabled = true;
         this.locStringList.Location = new System.Drawing.Point(6, 65);
         this.locStringList.Name = "locStringList";
         this.locStringList.Size = new System.Drawing.Size(384, 82);
         this.locStringList.TabIndex = 8;
         this.locStringList.SelectedIndexChanged += new System.EventHandler(this.listBox1_SelectedIndexChanged);
         // 
         // label2
         // 
         this.label2.AutoSize = true;
         this.label2.Location = new System.Drawing.Point(194, 41);
         this.label2.Name = "label2";
         this.label2.Size = new System.Drawing.Size(49, 13);
         this.label2.TabIndex = 10;
         this.label2.Text = "Category";
         // 
         // CategoryComboBox
         // 
         this.CategoryComboBox.FormattingEnabled = true;
         this.CategoryComboBox.Location = new System.Drawing.Point(249, 38);
         this.CategoryComboBox.Name = "CategoryComboBox";
         this.CategoryComboBox.Size = new System.Drawing.Size(105, 21);
         this.CategoryComboBox.TabIndex = 9;
         this.CategoryComboBox.SelectedIndexChanged += new System.EventHandler(this.CategoryComboBox_SelectedIndexChanged);
         // 
         // LocStringPicker
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.label2);
         this.Controls.Add(this.CategoryComboBox);
         this.Controls.Add(this.locStringList);
         this.Controls.Add(this.label1);
         this.Controls.Add(this.ScenarioComboBox);
         this.Controls.Add(this.StringTextLabel);
         this.Controls.Add(this.IDTextBox);
         this.Name = "LocStringPicker";
         this.Size = new System.Drawing.Size(393, 150);
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private System.Windows.Forms.TextBox IDTextBox;
      private System.Windows.Forms.Label StringTextLabel;
      private System.Windows.Forms.ComboBox ScenarioComboBox;
      private System.Windows.Forms.Label label1;
      private System.Windows.Forms.ListBox locStringList;
      private System.Windows.Forms.Label label2;
      private System.Windows.Forms.ComboBox CategoryComboBox;
   }
}
