namespace pxdb
{
    partial class MainForm
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
           this.components = new System.ComponentModel.Container();
           this.mainGrid = new System.Windows.Forms.DataGridView();
           this.detailGrid = new System.Windows.Forms.DataGridView();
           this.detailList = new System.Windows.Forms.ListBox();
           this.groupList = new System.Windows.Forms.ListBox();
           this.dataTypeCombo = new System.Windows.Forms.ComboBox();
           this.mainBindingSource = new System.Windows.Forms.BindingSource(this.components);
           this.detailBindingSource = new System.Windows.Forms.BindingSource(this.components);
           this.panel1 = new System.Windows.Forms.Panel();
           this.panel2 = new System.Windows.Forms.Panel();
           this.panel3 = new System.Windows.Forms.Panel();
           ((System.ComponentModel.ISupportInitialize)(this.mainGrid)).BeginInit();
           ((System.ComponentModel.ISupportInitialize)(this.detailGrid)).BeginInit();
           ((System.ComponentModel.ISupportInitialize)(this.mainBindingSource)).BeginInit();
           ((System.ComponentModel.ISupportInitialize)(this.detailBindingSource)).BeginInit();
           this.panel1.SuspendLayout();
           this.panel2.SuspendLayout();
           this.panel3.SuspendLayout();
           this.SuspendLayout();
           // 
           // mainGrid
           // 
           this.mainGrid.AllowUserToOrderColumns = true;
           this.mainGrid.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                       | System.Windows.Forms.AnchorStyles.Left)
                       | System.Windows.Forms.AnchorStyles.Right)));
           this.mainGrid.ColumnHeadersHeightSizeMode = System.Windows.Forms.DataGridViewColumnHeadersHeightSizeMode.AutoSize;
           this.mainGrid.Location = new System.Drawing.Point(125, 7);
           this.mainGrid.Name = "mainGrid";
           this.mainGrid.Size = new System.Drawing.Size(938, 485);
           this.mainGrid.TabIndex = 0;
           // 
           // detailGrid
           // 
           this.detailGrid.AllowUserToOrderColumns = true;
           this.detailGrid.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                       | System.Windows.Forms.AnchorStyles.Left)
                       | System.Windows.Forms.AnchorStyles.Right)));
           this.detailGrid.ColumnHeadersHeightSizeMode = System.Windows.Forms.DataGridViewColumnHeadersHeightSizeMode.AutoSize;
           this.detailGrid.Location = new System.Drawing.Point(125, 3);
           this.detailGrid.Name = "detailGrid";
           this.detailGrid.Size = new System.Drawing.Size(938, 187);
           this.detailGrid.TabIndex = 1;
           this.detailGrid.DataError += new System.Windows.Forms.DataGridViewDataErrorEventHandler(this.detailGrid_DataError);
           // 
           // detailList
           // 
           this.detailList.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                       | System.Windows.Forms.AnchorStyles.Left)));
           this.detailList.FormattingEnabled = true;
           this.detailList.Location = new System.Drawing.Point(12, 3);
           this.detailList.Name = "detailList";
           this.detailList.Size = new System.Drawing.Size(107, 186);
           this.detailList.TabIndex = 2;
           this.detailList.SelectedIndexChanged += new System.EventHandler(this.detailList_SelectedIndexChanged);
           // 
           // groupList
           // 
           this.groupList.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                       | System.Windows.Forms.AnchorStyles.Left)));
           this.groupList.FormattingEnabled = true;
           this.groupList.Location = new System.Drawing.Point(12, 7);
           this.groupList.Name = "groupList";
           this.groupList.Size = new System.Drawing.Size(107, 485);
           this.groupList.TabIndex = 2;
           this.groupList.SelectedIndexChanged += new System.EventHandler(this.grouplList_SelectedIndexChanged);
           // 
           // dataTypeCombo
           // 
           this.dataTypeCombo.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
           this.dataTypeCombo.FormattingEnabled = true;
           this.dataTypeCombo.Location = new System.Drawing.Point(12, 12);
           this.dataTypeCombo.Name = "dataTypeCombo";
           this.dataTypeCombo.Size = new System.Drawing.Size(169, 21);
           this.dataTypeCombo.TabIndex = 3;
           this.dataTypeCombo.SelectedIndexChanged += new System.EventHandler(this.dataTypeCombo_SelectedIndexChanged);
           // 
           // panel1
           // 
           this.panel1.Controls.Add(this.dataTypeCombo);
           this.panel1.Dock = System.Windows.Forms.DockStyle.Top;
           this.panel1.Location = new System.Drawing.Point(0, 0);
           this.panel1.Name = "panel1";
           this.panel1.Size = new System.Drawing.Size(1075, 43);
           this.panel1.TabIndex = 4;
           // 
           // panel2
           // 
           this.panel2.Controls.Add(this.groupList);
           this.panel2.Controls.Add(this.mainGrid);
           this.panel2.Dock = System.Windows.Forms.DockStyle.Fill;
           this.panel2.Location = new System.Drawing.Point(0, 43);
           this.panel2.Name = "panel2";
           this.panel2.Size = new System.Drawing.Size(1075, 698);
           this.panel2.TabIndex = 5;
           // 
           // panel3
           // 
           this.panel3.Controls.Add(this.detailList);
           this.panel3.Controls.Add(this.detailGrid);
           this.panel3.Dock = System.Windows.Forms.DockStyle.Bottom;
           this.panel3.Location = new System.Drawing.Point(0, 544);
           this.panel3.Name = "panel3";
           this.panel3.Size = new System.Drawing.Size(1075, 197);
           this.panel3.TabIndex = 6;
           // 
           // MainForm
           // 
           this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
           this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
           this.ClientSize = new System.Drawing.Size(1075, 741);
           this.Controls.Add(this.panel3);
           this.Controls.Add(this.panel2);
           this.Controls.Add(this.panel1);
           this.Name = "MainForm";
           this.Text = "Phoenix Database";
           ((System.ComponentModel.ISupportInitialize)(this.mainGrid)).EndInit();
           ((System.ComponentModel.ISupportInitialize)(this.detailGrid)).EndInit();
           ((System.ComponentModel.ISupportInitialize)(this.mainBindingSource)).EndInit();
           ((System.ComponentModel.ISupportInitialize)(this.detailBindingSource)).EndInit();
           this.panel1.ResumeLayout(false);
           this.panel2.ResumeLayout(false);
           this.panel3.ResumeLayout(false);
           this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.DataGridView mainGrid;
       private System.Windows.Forms.DataGridView detailGrid;
       private System.Windows.Forms.BindingSource mainBindingSource;
       private System.Windows.Forms.BindingSource detailBindingSource;
       private System.Windows.Forms.ListBox detailList;
       private System.Windows.Forms.ListBox groupList;
       private System.Windows.Forms.ComboBox dataTypeCombo;
       private System.Windows.Forms.Panel panel1;
       private System.Windows.Forms.Panel panel2;
       private System.Windows.Forms.Panel panel3;
    }
}

