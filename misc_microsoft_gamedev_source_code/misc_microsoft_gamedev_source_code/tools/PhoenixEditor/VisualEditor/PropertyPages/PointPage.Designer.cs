namespace VisualEditor.PropertyPages
{
   partial class PointPage
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
         this.dataComboBox = new System.Windows.Forms.ComboBox();
         this.label2 = new System.Windows.Forms.Label();
         this.toBoneComboBox = new System.Windows.Forms.ComboBox();
         this.toBoneLabel = new System.Windows.Forms.Label();
         this.typeComboBox = new System.Windows.Forms.ComboBox();
         this.label1 = new System.Windows.Forms.Label();
         this.groupBox1.SuspendLayout();
         this.SuspendLayout();
         // 
         // groupBox1
         // 
         this.groupBox1.Anchor = System.Windows.Forms.AnchorStyles.None;
         this.groupBox1.Controls.Add(this.dataComboBox);
         this.groupBox1.Controls.Add(this.label2);
         this.groupBox1.Controls.Add(this.toBoneComboBox);
         this.groupBox1.Controls.Add(this.toBoneLabel);
         this.groupBox1.Controls.Add(this.typeComboBox);
         this.groupBox1.Controls.Add(this.label1);
         this.groupBox1.Location = new System.Drawing.Point(0, 0);
         this.groupBox1.Name = "groupBox1";
         this.groupBox1.Padding = new System.Windows.Forms.Padding(5, 16, 5, 3);
         this.groupBox1.Size = new System.Drawing.Size(350, 250);
         this.groupBox1.TabIndex = 3;
         this.groupBox1.TabStop = false;
         this.groupBox1.Text = "Point Properties";
         // 
         // dataComboBox
         // 
         this.dataComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
         this.dataComboBox.FormattingEnabled = true;
         this.dataComboBox.Location = new System.Drawing.Point(133, 86);
         this.dataComboBox.Name = "dataComboBox";
         this.dataComboBox.Size = new System.Drawing.Size(209, 21);
         this.dataComboBox.TabIndex = 27;
         this.dataComboBox.SelectedIndexChanged += new System.EventHandler(this.dataComboBox_SelectedIndexChanged);
         // 
         // label2
         // 
         this.label2.AutoSize = true;
         this.label2.Location = new System.Drawing.Point(8, 89);
         this.label2.Margin = new System.Windows.Forms.Padding(3);
         this.label2.Name = "label2";
         this.label2.Size = new System.Drawing.Size(30, 13);
         this.label2.TabIndex = 26;
         this.label2.Text = "Data";
         // 
         // toBoneComboBox
         // 
         this.toBoneComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
         this.toBoneComboBox.FormattingEnabled = true;
         this.toBoneComboBox.Location = new System.Drawing.Point(133, 59);
         this.toBoneComboBox.Name = "toBoneComboBox";
         this.toBoneComboBox.Size = new System.Drawing.Size(209, 21);
         this.toBoneComboBox.TabIndex = 25;
         this.toBoneComboBox.SelectedIndexChanged += new System.EventHandler(this.toBoneComboBox_SelectedIndexChanged);
         // 
         // toBoneLabel
         // 
         this.toBoneLabel.AutoSize = true;
         this.toBoneLabel.Location = new System.Drawing.Point(8, 62);
         this.toBoneLabel.Margin = new System.Windows.Forms.Padding(3);
         this.toBoneLabel.Name = "toBoneLabel";
         this.toBoneLabel.Size = new System.Drawing.Size(48, 13);
         this.toBoneLabel.TabIndex = 16;
         this.toBoneLabel.Text = "To Bone";
         // 
         // typeComboBox
         // 
         this.typeComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
         this.typeComboBox.FormattingEnabled = true;
         this.typeComboBox.Location = new System.Drawing.Point(133, 32);
         this.typeComboBox.Name = "typeComboBox";
         this.typeComboBox.Size = new System.Drawing.Size(209, 21);
         this.typeComboBox.TabIndex = 3;
         this.typeComboBox.SelectedIndexChanged += new System.EventHandler(this.typeComboBox_SelectedIndexChanged);
         // 
         // label1
         // 
         this.label1.AutoSize = true;
         this.label1.Location = new System.Drawing.Point(8, 35);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(31, 13);
         this.label1.TabIndex = 2;
         this.label1.Text = "Type";
         // 
         // PointPage
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.groupBox1);
         this.Name = "PointPage";
         this.Size = new System.Drawing.Size(350, 250);
         this.groupBox1.ResumeLayout(false);
         this.groupBox1.PerformLayout();
         this.ResumeLayout(false);

      }

      #endregion

      private System.Windows.Forms.GroupBox groupBox1;
      private System.Windows.Forms.ComboBox toBoneComboBox;
      private System.Windows.Forms.Label toBoneLabel;
      private System.Windows.Forms.ComboBox typeComboBox;
      private System.Windows.Forms.Label label1;
      private System.Windows.Forms.ComboBox dataComboBox;
      private System.Windows.Forms.Label label2;
   }
}
