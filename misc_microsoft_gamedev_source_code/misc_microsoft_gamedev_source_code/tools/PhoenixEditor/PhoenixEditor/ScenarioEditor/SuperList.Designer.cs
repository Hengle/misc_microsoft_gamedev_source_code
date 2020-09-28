namespace PhoenixEditor.ScenarioEditor
{
   partial class SuperList
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
         this.tableLayoutPanel1 = new System.Windows.Forms.TableLayoutPanel();
         this.AddItemButton = new System.Windows.Forms.Button();
         this.SuspendLayout();
         // 
         // tableLayoutPanel1
         // 
         this.tableLayoutPanel1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.tableLayoutPanel1.CellBorderStyle = System.Windows.Forms.TableLayoutPanelCellBorderStyle.Single;
         this.tableLayoutPanel1.ColumnCount = 2;
         this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Absolute, 8F));
         this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 100F));
         this.tableLayoutPanel1.ForeColor = System.Drawing.SystemColors.ControlText;
         this.tableLayoutPanel1.Location = new System.Drawing.Point(3, 3);
         this.tableLayoutPanel1.Margin = new System.Windows.Forms.Padding(0);
         this.tableLayoutPanel1.Name = "tableLayoutPanel1";
         this.tableLayoutPanel1.RowCount = 1;
         this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 100F));
         this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 364F));
         this.tableLayoutPanel1.Size = new System.Drawing.Size(420, 365);
         this.tableLayoutPanel1.TabIndex = 0;
         // 
         // AddItemButton
         // 
         this.AddItemButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
         this.AddItemButton.BackColor = System.Drawing.SystemColors.ActiveCaption;
         this.AddItemButton.FlatStyle = System.Windows.Forms.FlatStyle.Popup;
         this.AddItemButton.Location = new System.Drawing.Point(3, 353);
         this.AddItemButton.Margin = new System.Windows.Forms.Padding(0);
         this.AddItemButton.Name = "AddItemButton";
         this.AddItemButton.Size = new System.Drawing.Size(10, 15);
         this.AddItemButton.TabIndex = 0;
         this.AddItemButton.UseVisualStyleBackColor = false;
         this.AddItemButton.Click += new System.EventHandler(this.AddItemButton_Click);
         // 
         // SuperList
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.AddItemButton);
         this.Controls.Add(this.tableLayoutPanel1);
         this.Name = "SuperList";
         this.Size = new System.Drawing.Size(423, 368);
         this.ResumeLayout(false);

      }

      #endregion

      private System.Windows.Forms.TableLayoutPanel tableLayoutPanel1;
      private System.Windows.Forms.Button AddItemButton;
   }
}
