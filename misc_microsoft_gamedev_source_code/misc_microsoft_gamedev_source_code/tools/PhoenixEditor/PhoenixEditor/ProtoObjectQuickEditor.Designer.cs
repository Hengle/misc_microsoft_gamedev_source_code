namespace PhoenixEditor
{
   partial class ProtoObjectQuickEditor
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
         this.AddViaAnimButton = new System.Windows.Forms.Button();
         this.NodeComboBox = new System.Windows.Forms.ComboBox();
         this.label2 = new System.Windows.Forms.Label();
         this.listBox1 = new System.Windows.Forms.ListBox();
         this.unitcountlabel = new System.Windows.Forms.Label();
         this.SuspendLayout();
         // 
         // AddViaAnimButton
         // 
         this.AddViaAnimButton.Location = new System.Drawing.Point(16, 76);
         this.AddViaAnimButton.Name = "AddViaAnimButton";
         this.AddViaAnimButton.Size = new System.Drawing.Size(282, 23);
         this.AddViaAnimButton.TabIndex = 1;
         this.AddViaAnimButton.Text = "Add using AnimXMLFile";
         this.AddViaAnimButton.UseVisualStyleBackColor = true;
         this.AddViaAnimButton.Click += new System.EventHandler(this.AddViaAnimButton_Click);
         // 
         // NodeComboBox
         // 
         this.NodeComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
         this.NodeComboBox.FormattingEnabled = true;
         this.NodeComboBox.Location = new System.Drawing.Point(125, 21);
         this.NodeComboBox.Name = "NodeComboBox";
         this.NodeComboBox.Size = new System.Drawing.Size(190, 21);
         this.NodeComboBox.TabIndex = 2;
         // 
         // label2
         // 
         this.label2.AutoSize = true;
         this.label2.Location = new System.Drawing.Point(18, 24);
         this.label2.Name = "label2";
         this.label2.Size = new System.Drawing.Size(101, 13);
         this.label2.TabIndex = 3;
         this.label2.Text = "Copy Settings From:";
         // 
         // listBox1
         // 
         this.listBox1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.listBox1.FormattingEnabled = true;
         this.listBox1.Location = new System.Drawing.Point(21, 116);
         this.listBox1.Name = "listBox1";
         this.listBox1.Size = new System.Drawing.Size(616, 186);
         this.listBox1.TabIndex = 4;
         // 
         // unitcountlabel
         // 
         this.unitcountlabel.AutoSize = true;
         this.unitcountlabel.Location = new System.Drawing.Point(363, 21);
         this.unitcountlabel.Name = "unitcountlabel";
         this.unitcountlabel.Size = new System.Drawing.Size(35, 13);
         this.unitcountlabel.TabIndex = 5;
         this.unitcountlabel.Text = "label1";
         // 
         // ProtoObjectQuickEditor
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.ClientSize = new System.Drawing.Size(677, 328);
         this.Controls.Add(this.unitcountlabel);
         this.Controls.Add(this.listBox1);
         this.Controls.Add(this.label2);
         this.Controls.Add(this.NodeComboBox);
         this.Controls.Add(this.AddViaAnimButton);
         this.Name = "ProtoObjectQuickEditor";
         this.Text = "ProtoObjectQuickEditor";
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private System.Windows.Forms.Button AddViaAnimButton;
      private System.Windows.Forms.ComboBox NodeComboBox;
      private System.Windows.Forms.Label label2;
      private System.Windows.Forms.ListBox listBox1;
      private System.Windows.Forms.Label unitcountlabel;
   }
}