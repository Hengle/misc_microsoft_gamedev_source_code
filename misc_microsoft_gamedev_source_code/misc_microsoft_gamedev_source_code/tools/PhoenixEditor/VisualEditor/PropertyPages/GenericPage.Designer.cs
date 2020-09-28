namespace VisualEditor.PropertyPages
{
   partial class GenericPage
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
         this.propertyGrid = new System.Windows.Forms.PropertyGrid();
         this.SuspendLayout();
         // 
         // propertyGrid
         // 
         this.propertyGrid.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.propertyGrid.HelpVisible = false;
         this.propertyGrid.Location = new System.Drawing.Point(0, 3);
         this.propertyGrid.Name = "propertyGrid";
         this.propertyGrid.PropertySort = System.Windows.Forms.PropertySort.NoSort;
         this.propertyGrid.Size = new System.Drawing.Size(350, 244);
         this.propertyGrid.TabIndex = 5;
         this.propertyGrid.ToolbarVisible = false;
         this.propertyGrid.PropertyValueChanged += new System.Windows.Forms.PropertyValueChangedEventHandler(this.VisualPropertyGrid_PropertyValueChanged);
         // 
         // GenericPage
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.propertyGrid);
         this.Name = "GenericPage";
         this.Size = new System.Drawing.Size(350, 250);
         this.ResumeLayout(false);

      }

      #endregion

      private System.Windows.Forms.PropertyGrid propertyGrid;
   }
}
