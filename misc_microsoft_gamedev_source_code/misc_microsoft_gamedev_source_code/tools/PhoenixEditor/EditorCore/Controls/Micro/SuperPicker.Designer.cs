namespace EditorCore.Controls.Micro
{
   partial class SuperPicker
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
         this.OKButton = new System.Windows.Forms.Button();
         this.CancelButton = new System.Windows.Forms.Button();
         this.SelectionLabel = new System.Windows.Forms.Label();
         this.panel1 = new System.Windows.Forms.Panel();
         this.autoTree1 = new EditorCore.Controls.Micro.AutoTree();
         this.categoryTree1 = new EditorCore.Controls.Micro.CategoryTree();
         this.SuspendLayout();
         // 
         // OKButton
         // 
         this.OKButton.Location = new System.Drawing.Point(787, 388);
         this.OKButton.Name = "OKButton";
         this.OKButton.Size = new System.Drawing.Size(75, 23);
         this.OKButton.TabIndex = 2;
         this.OKButton.Text = "OK";
         this.OKButton.UseVisualStyleBackColor = true;
         this.OKButton.Click += new System.EventHandler(this.OKButton_Click);
         // 
         // CancelButton
         // 
         this.CancelButton.Location = new System.Drawing.Point(868, 388);
         this.CancelButton.Name = "CancelButton";
         this.CancelButton.Size = new System.Drawing.Size(75, 23);
         this.CancelButton.TabIndex = 3;
         this.CancelButton.Text = "Cancel";
         this.CancelButton.UseVisualStyleBackColor = true;
         this.CancelButton.Click += new System.EventHandler(this.CancelButton_Click);
         // 
         // SelectionLabel
         // 
         this.SelectionLabel.AutoSize = true;
         this.SelectionLabel.Font = new System.Drawing.Font("Microsoft Sans Serif", 14.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
         this.SelectionLabel.Location = new System.Drawing.Point(491, 14);
         this.SelectionLabel.Name = "SelectionLabel";
         this.SelectionLabel.Size = new System.Drawing.Size(66, 24);
         this.SelectionLabel.TabIndex = 4;
         this.SelectionLabel.Text = "label1";
         // 
         // panel1
         // 
         this.panel1.Location = new System.Drawing.Point(491, 41);
         this.panel1.Name = "panel1";
         this.panel1.Size = new System.Drawing.Size(452, 341);
         this.panel1.TabIndex = 5;
         // 
         // autoTree1
         // 
         this.autoTree1.Location = new System.Drawing.Point(258, 3);
         this.autoTree1.Name = "autoTree1";
         this.autoTree1.Size = new System.Drawing.Size(227, 418);
         this.autoTree1.TabIndex = 1;
         // 
         // categoryTree1
         // 
         this.categoryTree1.Font = new System.Drawing.Font("Microsoft Sans Serif", 14F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
         this.categoryTree1.Location = new System.Drawing.Point(3, 6);
         this.categoryTree1.Name = "categoryTree1";
         this.categoryTree1.Size = new System.Drawing.Size(249, 418);
         this.categoryTree1.TabIndex = 0;
         // 
         // SuperPicker
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.panel1);
         this.Controls.Add(this.SelectionLabel);
         this.Controls.Add(this.CancelButton);
         this.Controls.Add(this.OKButton);
         this.Controls.Add(this.autoTree1);
         this.Controls.Add(this.categoryTree1);
         this.Name = "SuperPicker";
         this.Size = new System.Drawing.Size(961, 424);
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private CategoryTree categoryTree1;
      private AutoTree autoTree1;
      private System.Windows.Forms.Button OKButton;
      private System.Windows.Forms.Button CancelButton;
      private System.Windows.Forms.Label SelectionLabel;
      private System.Windows.Forms.Panel panel1;
   }
}
