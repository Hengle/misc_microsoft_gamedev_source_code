namespace PhoenixEditor.ClientTabPages
{
   partial class StringTablePage
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
         this.dataRowTemplate1 = new Xceed.Grid.DataRow();
         this.dataRowStyle1 = new Xceed.Grid.VisualGridElementStyle();
         this.groupByRow1 = new Xceed.Grid.GroupByRow();
         this.columnManagerRow1 = new Xceed.Grid.ColumnManagerRow();
         this.mBtnNewString = new System.Windows.Forms.Button();
         this.mBtnDelete = new System.Windows.Forms.Button();
         this.mtxtbSearch = new System.Windows.Forms.TextBox();
         this.mBtnSearch = new System.Windows.Forms.Button();
         this.btnFilter = new System.Windows.Forms.Button();
         this.groupBox1 = new System.Windows.Forms.GroupBox();
         this.mChkCaseSensitive = new System.Windows.Forms.CheckBox();
         this.label1 = new System.Windows.Forms.Label();
         this.mBtnSave = new System.Windows.Forms.Button();
         this.mBtnCheckout = new System.Windows.Forms.Button();
         this.mBtnDiscard = new System.Windows.Forms.Button();
         this.mBtnSaveOnly = new System.Windows.Forms.Button();
         ((System.ComponentModel.ISupportInitialize)(this.gridControl1)).BeginInit();
         ((System.ComponentModel.ISupportInitialize)(this.dataRowTemplate1)).BeginInit();
         ((System.ComponentModel.ISupportInitialize)(this.columnManagerRow1)).BeginInit();
         this.groupBox1.SuspendLayout();
         this.SuspendLayout();
         // 
         // gridControl1
         // 
         this.gridControl1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.gridControl1.DataRowTemplate = this.dataRowTemplate1;
         this.gridControl1.DataRowTemplateStyles.Add(this.dataRowStyle1);
         this.gridControl1.FixedHeaderRows.Add(this.groupByRow1);
         this.gridControl1.FixedHeaderRows.Add(this.columnManagerRow1);
         this.gridControl1.Font = new System.Drawing.Font("Arial", 14.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
         this.gridControl1.Location = new System.Drawing.Point(34, 161);
         this.gridControl1.Name = "gridControl1";
         this.gridControl1.Size = new System.Drawing.Size(885, 504);
         this.gridControl1.TabIndex = 1;
         this.gridControl1.GroupAdded += new Xceed.Grid.GroupAddedEventHandler(this.gridControl1_GroupAdded);
         this.gridControl1.GroupingUpdated += new System.EventHandler(this.gridControl1_GroupingUpdated);
         this.gridControl1.QueryGroupKeys += new Xceed.Grid.QueryGroupKeysEventHandler(this.gridControl1_QueryGroupKeys);
         this.gridControl1.SizeChanged += new System.EventHandler(this.gridControl1_SizeChanged);
         this.gridControl1.KeyUp += new System.Windows.Forms.KeyEventHandler(this.gridControl1_KeyUp);
         // 
         // dataRowStyle1
         // 
         this.dataRowStyle1.ClipPartialLine = true;
         this.dataRowStyle1.Trimming = System.Drawing.StringTrimming.None;
         // 
         // groupByRow1
         // 
         this.groupByRow1.Visible = true;
         // 
         // columnManagerRow1
         // 
         this.columnManagerRow1.AllowColumnReorder = false;
         // 
         // mBtnNewString
         // 
         this.mBtnNewString.Location = new System.Drawing.Point(234, 130);
         this.mBtnNewString.Name = "mBtnNewString";
         this.mBtnNewString.Size = new System.Drawing.Size(130, 23);
         this.mBtnNewString.TabIndex = 2;
         this.mBtnNewString.Text = "Insert New String";
         this.mBtnNewString.UseVisualStyleBackColor = true;
         this.mBtnNewString.Click += new System.EventHandler(this.mBtnNewString_Click);
         // 
         // mBtnDelete
         // 
         this.mBtnDelete.Location = new System.Drawing.Point(388, 132);
         this.mBtnDelete.Name = "mBtnDelete";
         this.mBtnDelete.Size = new System.Drawing.Size(130, 23);
         this.mBtnDelete.TabIndex = 3;
         this.mBtnDelete.Text = "Delete Selected Strings";
         this.mBtnDelete.UseVisualStyleBackColor = true;
         this.mBtnDelete.Click += new System.EventHandler(this.mBtnDelete_Click);
         // 
         // mtxtbSearch
         // 
         this.mtxtbSearch.Location = new System.Drawing.Point(97, 19);
         this.mtxtbSearch.Name = "mtxtbSearch";
         this.mtxtbSearch.Size = new System.Drawing.Size(172, 20);
         this.mtxtbSearch.TabIndex = 4;
         this.mtxtbSearch.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this.mtxtbSearch_KeyPress);
         // 
         // mBtnSearch
         // 
         this.mBtnSearch.Location = new System.Drawing.Point(275, 17);
         this.mBtnSearch.Name = "mBtnSearch";
         this.mBtnSearch.Size = new System.Drawing.Size(130, 23);
         this.mBtnSearch.TabIndex = 5;
         this.mBtnSearch.Text = "Search";
         this.mBtnSearch.UseVisualStyleBackColor = true;
         this.mBtnSearch.Click += new System.EventHandler(this.mBtnSearch_Click);
         // 
         // btnFilter
         // 
         this.btnFilter.Location = new System.Drawing.Point(275, 56);
         this.btnFilter.Name = "btnFilter";
         this.btnFilter.Size = new System.Drawing.Size(130, 23);
         this.btnFilter.TabIndex = 6;
         this.btnFilter.Text = "Filter Search Results";
         this.btnFilter.UseVisualStyleBackColor = true;
         this.btnFilter.Click += new System.EventHandler(this.btnFilter_Click);
         // 
         // groupBox1
         // 
         this.groupBox1.Controls.Add(this.mChkCaseSensitive);
         this.groupBox1.Controls.Add(this.label1);
         this.groupBox1.Controls.Add(this.mtxtbSearch);
         this.groupBox1.Controls.Add(this.btnFilter);
         this.groupBox1.Controls.Add(this.mBtnSearch);
         this.groupBox1.Location = new System.Drawing.Point(201, 12);
         this.groupBox1.Name = "groupBox1";
         this.groupBox1.Size = new System.Drawing.Size(421, 89);
         this.groupBox1.TabIndex = 7;
         this.groupBox1.TabStop = false;
         this.groupBox1.Text = "Search";
         // 
         // mChkCaseSensitive
         // 
         this.mChkCaseSensitive.AutoSize = true;
         this.mChkCaseSensitive.Location = new System.Drawing.Point(97, 60);
         this.mChkCaseSensitive.Name = "mChkCaseSensitive";
         this.mChkCaseSensitive.Size = new System.Drawing.Size(133, 17);
         this.mChkCaseSensitive.TabIndex = 8;
         this.mChkCaseSensitive.Text = "Case Sensitive Search";
         this.mChkCaseSensitive.UseVisualStyleBackColor = true;
         // 
         // label1
         // 
         this.label1.AutoSize = true;
         this.label1.Location = new System.Drawing.Point(10, 23);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(81, 13);
         this.label1.TabIndex = 7;
         this.label1.Text = "Text To Search";
         // 
         // mBtnSave
         // 
         this.mBtnSave.Location = new System.Drawing.Point(26, 43);
         this.mBtnSave.Name = "mBtnSave";
         this.mBtnSave.Size = new System.Drawing.Size(151, 23);
         this.mBtnSave.TabIndex = 8;
         this.mBtnSave.Text = "Save Changes and Checkin";
         this.mBtnSave.UseVisualStyleBackColor = true;
         this.mBtnSave.Click += new System.EventHandler(this.mBtnSave_Click);
         // 
         // mBtnCheckout
         // 
         this.mBtnCheckout.Location = new System.Drawing.Point(26, 12);
         this.mBtnCheckout.Name = "mBtnCheckout";
         this.mBtnCheckout.Size = new System.Drawing.Size(151, 23);
         this.mBtnCheckout.TabIndex = 9;
         this.mBtnCheckout.Text = "Edit Stringtable";
         this.mBtnCheckout.UseVisualStyleBackColor = true;
         this.mBtnCheckout.Click += new System.EventHandler(this.mBtnCheckout_Click);
         // 
         // mBtnDiscard
         // 
         this.mBtnDiscard.Location = new System.Drawing.Point(26, 88);
         this.mBtnDiscard.Name = "mBtnDiscard";
         this.mBtnDiscard.Size = new System.Drawing.Size(151, 23);
         this.mBtnDiscard.TabIndex = 10;
         this.mBtnDiscard.Text = "Discard Changes";
         this.mBtnDiscard.UseVisualStyleBackColor = true;
         this.mBtnDiscard.Click += new System.EventHandler(this.mBtnDiscard_Click);
         // 
         // mBtnSaveOnly
         // 
         this.mBtnSaveOnly.Location = new System.Drawing.Point(26, 119);
         this.mBtnSaveOnly.Name = "mBtnSaveOnly";
         this.mBtnSaveOnly.Size = new System.Drawing.Size(151, 23);
         this.mBtnSaveOnly.TabIndex = 8;
         this.mBtnSaveOnly.Text = "Save To File Only";
         this.mBtnSaveOnly.UseVisualStyleBackColor = true;
         this.mBtnSaveOnly.Click += new System.EventHandler(this.mBtnSaveOnly_Click);
         // 
         // StringTablePage
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
         this.Controls.Add(this.mBtnDiscard);
         this.Controls.Add(this.mBtnCheckout);
         this.Controls.Add(this.mBtnSaveOnly);
         this.Controls.Add(this.mBtnSave);
         this.Controls.Add(this.groupBox1);
         this.Controls.Add(this.mBtnDelete);
         this.Controls.Add(this.mBtnNewString);
         this.Controls.Add(this.gridControl1);
         this.Name = "StringTablePage";
         this.Size = new System.Drawing.Size(971, 693);
         this.KeyUp += new System.Windows.Forms.KeyEventHandler(this.StringTablePage_KeyUp);
         this.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this.StringTablePage_KeyPress);
         ((System.ComponentModel.ISupportInitialize)(this.gridControl1)).EndInit();
         ((System.ComponentModel.ISupportInitialize)(this.dataRowTemplate1)).EndInit();
         ((System.ComponentModel.ISupportInitialize)(this.columnManagerRow1)).EndInit();
         this.groupBox1.ResumeLayout(false);
         this.groupBox1.PerformLayout();
         this.ResumeLayout(false);

      }

      #endregion

      private Xceed.Grid.GridControl gridControl1;
       private Xceed.Grid.DataRow dataRowTemplate1;
       private Xceed.Grid.GroupByRow groupByRow1;
       private Xceed.Grid.ColumnManagerRow columnManagerRow1;
      private Xceed.Grid.VisualGridElementStyle dataRowStyle1;
      private System.Windows.Forms.Button mBtnNewString;
      private System.Windows.Forms.Button mBtnDelete;
      private System.Windows.Forms.TextBox mtxtbSearch;
      private System.Windows.Forms.Button mBtnSearch;
      private System.Windows.Forms.Button btnFilter;
      private System.Windows.Forms.GroupBox groupBox1;
      private System.Windows.Forms.CheckBox mChkCaseSensitive;
      private System.Windows.Forms.Label label1;
      private System.Windows.Forms.Button mBtnSave;
      private System.Windows.Forms.Button mBtnCheckout;
      private System.Windows.Forms.Button mBtnDiscard;
      private System.Windows.Forms.Button mBtnSaveOnly;
   }
}
