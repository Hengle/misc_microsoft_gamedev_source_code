namespace PhoenixEditor
{
   partial class TexturePaletteViewer
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
         this.flowLayoutPanel1 = new System.Windows.Forms.FlowLayoutPanel();
         this.paletteComboBox = new System.Windows.Forms.ComboBox();
         this.label9 = new System.Windows.Forms.Label();
         this.SuspendLayout();
         // 
         // flowLayoutPanel1
         // 
         this.flowLayoutPanel1.AutoScroll = true;
         this.flowLayoutPanel1.BackColor = System.Drawing.SystemColors.AppWorkspace;
         this.flowLayoutPanel1.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
         this.flowLayoutPanel1.Location = new System.Drawing.Point(12, 60);
         this.flowLayoutPanel1.Name = "flowLayoutPanel1";
         this.flowLayoutPanel1.Size = new System.Drawing.Size(447, 282);
         this.flowLayoutPanel1.TabIndex = 0;
         // 
         // paletteComboBox
         // 
         this.paletteComboBox.FormattingEnabled = true;
         this.paletteComboBox.Location = new System.Drawing.Point(12, 33);
         this.paletteComboBox.Name = "paletteComboBox";
         this.paletteComboBox.Size = new System.Drawing.Size(268, 21);
         this.paletteComboBox.TabIndex = 1;
         this.paletteComboBox.SelectedIndexChanged += new System.EventHandler(this.paletteComboBox_SelectedIndexChanged);
         // 
         // label9
         // 
         this.label9.AutoSize = true;
         this.label9.Location = new System.Drawing.Point(12, 17);
         this.label9.Name = "label9";
         this.label9.Size = new System.Drawing.Size(90, 13);
         this.label9.TabIndex = 2;
         this.label9.Text = "Choose a palette:";
         // 
         // TexturePaletteViewer
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.ClientSize = new System.Drawing.Size(471, 354);
         this.Controls.Add(this.label9);
         this.Controls.Add(this.paletteComboBox);
         this.Controls.Add(this.flowLayoutPanel1);
         this.Name = "TexturePaletteViewer";
         this.Text = "TexturePaletteViewer";
         this.Load += new System.EventHandler(this.TexturePaletteViewer_Load);
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private System.Windows.Forms.FlowLayoutPanel flowLayoutPanel1;
      private System.Windows.Forms.ComboBox paletteComboBox;
      private System.Windows.Forms.Label label9;
   }
}