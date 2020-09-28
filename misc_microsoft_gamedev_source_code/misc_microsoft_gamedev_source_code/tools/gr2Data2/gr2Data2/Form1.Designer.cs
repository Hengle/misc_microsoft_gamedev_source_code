namespace gr2Data2
{
    partial class Form1
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
            this.gridControl1.DataRowTemplate = this.dataRowTemplate1;
            this.gridControl1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.gridControl1.FixedHeaderRows.Add(this.groupByRow1);
            this.gridControl1.FixedHeaderRows.Add(this.columnManagerRow1);
            this.gridControl1.Location = new System.Drawing.Point(0, 0);
            this.gridControl1.Name = "gridControl1";
            this.gridControl1.Size = new System.Drawing.Size(1006, 636);
            this.gridControl1.TabIndex = 0;
            // 
            // Form1
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(1006, 636);
            this.Controls.Add(this.gridControl1);
            this.Name = "Form1";
            this.Text = "Form1";
            this.Load += new System.EventHandler(this.Form1_Load);
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

