namespace EditorCore
{
   partial class RefSetPanel
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
         this.guestControlPanel = new System.Windows.Forms.Panel();
         this.SetRefButton = new System.Windows.Forms.Button();
         this.tableLayoutPanel1 = new System.Windows.Forms.TableLayoutPanel();
         this.ClearRefButton = new System.Windows.Forms.Button();
         this.tableLayoutPanel1.SuspendLayout();
         this.SuspendLayout();
         // 
         // guestControlPanel
         // 
         this.guestControlPanel.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.guestControlPanel.Location = new System.Drawing.Point(0, 0);
         this.guestControlPanel.Margin = new System.Windows.Forms.Padding(0);
         this.guestControlPanel.Name = "guestControlPanel";
         this.guestControlPanel.Size = new System.Drawing.Size(166, 29);
         this.guestControlPanel.TabIndex = 0;
         // 
         // SetRefButton
         // 
         this.SetRefButton.BackgroundImageLayout = System.Windows.Forms.ImageLayout.Zoom;
         this.SetRefButton.Dock = System.Windows.Forms.DockStyle.Top;
         this.SetRefButton.Location = new System.Drawing.Point(1, 1);
         this.SetRefButton.Margin = new System.Windows.Forms.Padding(1);
         this.SetRefButton.Name = "SetRefButton";
         this.SetRefButton.Size = new System.Drawing.Size(36, 27);
         this.SetRefButton.TabIndex = 1;
         this.SetRefButton.UseVisualStyleBackColor = true;
         this.SetRefButton.Click += new System.EventHandler(this.SetRefButton_Click);
         // 
         // tableLayoutPanel1
         // 
         this.tableLayoutPanel1.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.tableLayoutPanel1.ColumnCount = 2;
         this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 50F));
         this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 50F));
         this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Absolute, 20F));
         this.tableLayoutPanel1.Controls.Add(this.ClearRefButton, 1, 0);
         this.tableLayoutPanel1.Controls.Add(this.SetRefButton, 0, 0);
         this.tableLayoutPanel1.Location = new System.Drawing.Point(166, 0);
         this.tableLayoutPanel1.Margin = new System.Windows.Forms.Padding(0);
         this.tableLayoutPanel1.Name = "tableLayoutPanel1";
         this.tableLayoutPanel1.RowCount = 1;
         this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 100F));
         this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 20F));
         this.tableLayoutPanel1.Size = new System.Drawing.Size(77, 29);
         this.tableLayoutPanel1.TabIndex = 2;
         // 
         // ClearRefButton
         // 
         this.ClearRefButton.BackgroundImageLayout = System.Windows.Forms.ImageLayout.Zoom;
         this.ClearRefButton.Dock = System.Windows.Forms.DockStyle.Top;
         this.ClearRefButton.Location = new System.Drawing.Point(39, 1);
         this.ClearRefButton.Margin = new System.Windows.Forms.Padding(1);
         this.ClearRefButton.Name = "ClearRefButton";
         this.ClearRefButton.Size = new System.Drawing.Size(37, 27);
         this.ClearRefButton.TabIndex = 2;
         this.ClearRefButton.UseVisualStyleBackColor = true;
         this.ClearRefButton.Click += new System.EventHandler(this.ClearRefButton_Click);
         // 
         // RefSetPanel
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.BackgroundImageLayout = System.Windows.Forms.ImageLayout.Stretch;
         this.Controls.Add(this.tableLayoutPanel1);
         this.Controls.Add(this.guestControlPanel);
         this.Name = "RefSetPanel";
         this.Size = new System.Drawing.Size(243, 29);
         this.tableLayoutPanel1.ResumeLayout(false);
         this.ResumeLayout(false);

      }

      #endregion

      private System.Windows.Forms.Panel guestControlPanel;
      private System.Windows.Forms.Button SetRefButton;
      private System.Windows.Forms.TableLayoutPanel tableLayoutPanel1;
      private System.Windows.Forms.Button ClearRefButton;
   }
}
