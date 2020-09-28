namespace PhoenixEditor
{
   partial class ObjectProperties
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
         this.ObjectPropertyGrid = new System.Windows.Forms.PropertyGrid();
         this.SuspendLayout();
         // 
         // ObjectPropertyGrid
         // 
         this.ObjectPropertyGrid.Dock = System.Windows.Forms.DockStyle.Fill;
         this.ObjectPropertyGrid.Location = new System.Drawing.Point(0, 0);
         this.ObjectPropertyGrid.Name = "ObjectPropertyGrid";
         this.ObjectPropertyGrid.Size = new System.Drawing.Size(292, 266);
         this.ObjectPropertyGrid.TabIndex = 0;
         this.ObjectPropertyGrid.Click += new System.EventHandler(this.ObjectPropertyGrid_Click);
         // 
         // ObjectProperties
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.ObjectPropertyGrid);
         this.Name = "ObjectProperties";
         this.Size = new System.Drawing.Size(292, 266);
         this.Text = "Properties";
         this.ResumeLayout(false);

      }

      #endregion

      private System.Windows.Forms.PropertyGrid ObjectPropertyGrid;
   }
}