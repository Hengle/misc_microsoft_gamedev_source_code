namespace EditorCore.Controls.Micro
{
   partial class HybridGrid
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
         this.gridControl1 = new Xceed.Grid.GridControl();
         this.groupByRow1 = new Xceed.Grid.GroupByRow();
         this.columnManagerRow1 = new Xceed.Grid.ColumnManagerRow();
         this.dataRowTemplate1 = new Xceed.Grid.DataRow();
         ((System.ComponentModel.ISupportInitialize)(this.gridControl1)).BeginInit();
         ((System.ComponentModel.ISupportInitialize)(this.columnManagerRow1)).BeginInit();
         ((System.ComponentModel.ISupportInitialize)(this.dataRowTemplate1)).BeginInit();
         this.SuspendLayout();
         // 
         // gridControl1
         // 
         this.gridControl1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.gridControl1.DataRowTemplate = this.dataRowTemplate1;
         this.gridControl1.FixedHeaderRows.Add(this.groupByRow1);
         this.gridControl1.FixedHeaderRows.Add(this.columnManagerRow1);
         this.gridControl1.Location = new System.Drawing.Point(13, 18);
         this.gridControl1.Name = "gridControl1";
         this.gridControl1.Size = new System.Drawing.Size(675, 567);
         this.gridControl1.TabIndex = 0;
         // 
         // HybridGrid
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.gridControl1);
         this.Name = "HybridGrid";
         this.Size = new System.Drawing.Size(712, 608);
         ((System.ComponentModel.ISupportInitialize)(this.gridControl1)).EndInit();
         ((System.ComponentModel.ISupportInitialize)(this.columnManagerRow1)).EndInit();
         ((System.ComponentModel.ISupportInitialize)(this.dataRowTemplate1)).EndInit();
         this.ResumeLayout(false);

      }

      #endregion

      private Xceed.Grid.GridControl gridControl1;
      private Xceed.Grid.DataRow dataRowTemplate1;
      private Xceed.Grid.GroupByRow groupByRow1;
      private Xceed.Grid.ColumnManagerRow columnManagerRow1;
   }
}
