namespace PhoenixEditor.ScenarioEditor
{
   partial class TriggerValueList
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
         this.components = new System.ComponentModel.Container();
         this.gridControl1 = new Xceed.Grid.GridControl();
         this.colType = new Xceed.Grid.DataBoundColumn();
         this.colDefaultValue = new Xceed.Grid.DataBoundColumn();
         this.colName = new Xceed.Grid.DataBoundColumn();
         this.colID = new Xceed.Grid.DataBoundColumn();
         this.dataRowTemplate1 = new Xceed.Grid.DataRow();
         this.celldataRowTemplate1Type = new Xceed.Grid.DataCell();
         this.celldataRowTemplate1DefaultValue = new Xceed.Grid.DataCell();
         this.celldataRowTemplate1Name = new Xceed.Grid.DataCell();
         this.celldataRowTemplate1ID = new Xceed.Grid.DataCell();
         this.bindingSource2 = new System.Windows.Forms.BindingSource(this.components);
         this.groupByRow1 = new Xceed.Grid.GroupByRow();
         this.columnManagerRow1 = new Xceed.Grid.ColumnManagerRow();
         this.cellcolumnManagerRow1Type = new Xceed.Grid.ColumnManagerCell();
         this.cellcolumnManagerRow1DefaultValue = new Xceed.Grid.ColumnManagerCell();
         this.cellcolumnManagerRow1Name = new Xceed.Grid.ColumnManagerCell();
         this.cellcolumnManagerRow1ID = new Xceed.Grid.ColumnManagerCell();
         ((System.ComponentModel.ISupportInitialize)(this.gridControl1)).BeginInit();
         ((System.ComponentModel.ISupportInitialize)(this.dataRowTemplate1)).BeginInit();
         ((System.ComponentModel.ISupportInitialize)(this.bindingSource2)).BeginInit();
         ((System.ComponentModel.ISupportInitialize)(this.columnManagerRow1)).BeginInit();
         this.SuspendLayout();
         // 
         // gridControl1
         // 
         this.gridControl1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.gridControl1.Columns.Add(this.colType);
         this.gridControl1.Columns.Add(this.colDefaultValue);
         this.gridControl1.Columns.Add(this.colName);
         this.gridControl1.Columns.Add(this.colID);
         this.gridControl1.DataRowTemplate = this.dataRowTemplate1;
         this.gridControl1.DataSource = this.bindingSource2;
         this.gridControl1.FixedHeaderRows.Add(this.groupByRow1);
         this.gridControl1.FixedHeaderRows.Add(this.columnManagerRow1);
         this.gridControl1.Location = new System.Drawing.Point(3, 3);
         this.gridControl1.Name = "gridControl1";
         this.gridControl1.Size = new System.Drawing.Size(421, 243);
         this.gridControl1.TabIndex = 2;
         // 
         // colType
         // 
         this.colType.Title = "Type";
         this.colType.VisibleIndex = 1;
         this.colType.Initialize("Type");
         // 
         // colDefaultValue
         // 
         this.colDefaultValue.Title = "DefaultValue";
         this.colDefaultValue.VisibleIndex = 2;
         this.colDefaultValue.Initialize("DefaultValue");
         // 
         // colName
         // 
         this.colName.Title = "Name";
         this.colName.VisibleIndex = 0;
         this.colName.Initialize("Name");
         // 
         // colID
         // 
         this.colID.Title = "ID";
         this.colID.VisibleIndex = 3;
         this.colID.Initialize("ID");
         // 
         // dataRowTemplate1
         // 
         this.dataRowTemplate1.Cells.Add(this.celldataRowTemplate1Type);
         this.dataRowTemplate1.Cells.Add(this.celldataRowTemplate1DefaultValue);
         this.dataRowTemplate1.Cells.Add(this.celldataRowTemplate1Name);
         this.dataRowTemplate1.Cells.Add(this.celldataRowTemplate1ID);
         this.celldataRowTemplate1Type.Initialize("Type");
         this.celldataRowTemplate1DefaultValue.Initialize("DefaultValue");
         this.celldataRowTemplate1Name.Initialize("Name");
         this.celldataRowTemplate1ID.Initialize("ID");
         // 
         // bindingSource2
         // 
         this.bindingSource2.DataSource = typeof(PhoenixEditor.ScenarioEditor.TriggerValueContainer);
         // 
         // columnManagerRow1
         // 
         this.columnManagerRow1.Cells.Add(this.cellcolumnManagerRow1Type);
         this.columnManagerRow1.Cells.Add(this.cellcolumnManagerRow1DefaultValue);
         this.columnManagerRow1.Cells.Add(this.cellcolumnManagerRow1Name);
         this.columnManagerRow1.Cells.Add(this.cellcolumnManagerRow1ID);
         this.cellcolumnManagerRow1Type.Initialize("Type");
         this.cellcolumnManagerRow1DefaultValue.Initialize("DefaultValue");
         this.cellcolumnManagerRow1Name.Initialize("Name");
         this.cellcolumnManagerRow1ID.Initialize("ID");
         // 
         // TriggerValueList
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.gridControl1);
         this.Name = "TriggerValueList";
         this.Size = new System.Drawing.Size(427, 249);
         ((System.ComponentModel.ISupportInitialize)(this.gridControl1)).EndInit();
         ((System.ComponentModel.ISupportInitialize)(this.dataRowTemplate1)).EndInit();
         ((System.ComponentModel.ISupportInitialize)(this.bindingSource2)).EndInit();
         ((System.ComponentModel.ISupportInitialize)(this.columnManagerRow1)).EndInit();
         this.ResumeLayout(false);

      }

      #endregion

      private Xceed.Grid.GridControl gridControl1;
      private Xceed.Grid.DataRow dataRowTemplate1;
      private Xceed.Grid.GroupByRow groupByRow1;
      private Xceed.Grid.ColumnManagerRow columnManagerRow1;
      private System.Windows.Forms.BindingSource bindingSource2;
      private Xceed.Grid.DataBoundColumn colType;
      private Xceed.Grid.DataBoundColumn colDefaultValue;
      private Xceed.Grid.DataBoundColumn colName;
      private Xceed.Grid.DataCell celldataRowTemplate1Type;
      private Xceed.Grid.DataCell celldataRowTemplate1DefaultValue;
      private Xceed.Grid.DataCell celldataRowTemplate1Name;
      private Xceed.Grid.ColumnManagerCell cellcolumnManagerRow1Type;
      private Xceed.Grid.ColumnManagerCell cellcolumnManagerRow1DefaultValue;
      private Xceed.Grid.ColumnManagerCell cellcolumnManagerRow1Name;
      private Xceed.Grid.DataBoundColumn colID;
      private Xceed.Grid.DataCell celldataRowTemplate1ID;
      private Xceed.Grid.ColumnManagerCell cellcolumnManagerRow1ID;

   }
}
