namespace PhoenixEditor.ScenarioEditor
{
   partial class TriggerSearch
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
         this.tabControl1 = new System.Windows.Forms.TabControl();
         this.tabPage1 = new System.Windows.Forms.TabPage();
         this.FoundVarLabel = new System.Windows.Forms.Label();
         this.VarClearSearchButton = new System.Windows.Forms.Button();
         this.VarListView = new System.Windows.Forms.ListView();
         this.VarSearchGridControl = new Xceed.Grid.GridControl();
         this.dataRowTemplate1 = new Xceed.Grid.DataRow();
         this.groupByRow1 = new Xceed.Grid.GroupByRow();
         this.columnManagerRow1 = new Xceed.Grid.ColumnManagerRow();
         this.VarSearchButton = new System.Windows.Forms.Button();
         this.tabPage2 = new System.Windows.Forms.TabPage();
         this.TriggerGridControl = new Xceed.Grid.GridControl();
         this.dataRowTemplate2 = new Xceed.Grid.DataRow();
         this.groupByRow2 = new Xceed.Grid.GroupByRow();
         this.columnManagerRow2 = new Xceed.Grid.ColumnManagerRow();
         this.TriggerClearButton = new System.Windows.Forms.Button();
         this.TriggerSearchButton = new System.Windows.Forms.Button();
         this.tabPage3 = new System.Windows.Forms.TabPage();
         this.ComponentGridControl = new Xceed.Grid.GridControl();
         this.dataRowTemplate3 = new Xceed.Grid.DataRow();
         this.groupByRow3 = new Xceed.Grid.GroupByRow();
         this.columnManagerRow3 = new Xceed.Grid.ColumnManagerRow();
         this.ComponentClearButton = new System.Windows.Forms.Button();
         this.ComponentSearchButton = new System.Windows.Forms.Button();
         this.VarSearchBetterPropertyGrid = new EditorCore.BetterPropertyGrid();
         this.TriggerBetterPropertyGrid = new EditorCore.BetterPropertyGrid();
         this.ComponentBetterPropertyGrid = new EditorCore.BetterPropertyGrid();
         this.tabControl1.SuspendLayout();
         this.tabPage1.SuspendLayout();
         ((System.ComponentModel.ISupportInitialize)(this.VarSearchGridControl)).BeginInit();
         ((System.ComponentModel.ISupportInitialize)(this.dataRowTemplate1)).BeginInit();
         ((System.ComponentModel.ISupportInitialize)(this.columnManagerRow1)).BeginInit();
         this.tabPage2.SuspendLayout();
         ((System.ComponentModel.ISupportInitialize)(this.TriggerGridControl)).BeginInit();
         ((System.ComponentModel.ISupportInitialize)(this.dataRowTemplate2)).BeginInit();
         ((System.ComponentModel.ISupportInitialize)(this.columnManagerRow2)).BeginInit();
         this.tabPage3.SuspendLayout();
         ((System.ComponentModel.ISupportInitialize)(this.ComponentGridControl)).BeginInit();
         ((System.ComponentModel.ISupportInitialize)(this.dataRowTemplate3)).BeginInit();
         ((System.ComponentModel.ISupportInitialize)(this.columnManagerRow3)).BeginInit();
         this.SuspendLayout();
         // 
         // tabControl1
         // 
         this.tabControl1.Controls.Add(this.tabPage1);
         this.tabControl1.Controls.Add(this.tabPage2);
         this.tabControl1.Controls.Add(this.tabPage3);
         this.tabControl1.Dock = System.Windows.Forms.DockStyle.Fill;
         this.tabControl1.Location = new System.Drawing.Point(0, 0);
         this.tabControl1.Name = "tabControl1";
         this.tabControl1.SelectedIndex = 0;
         this.tabControl1.Size = new System.Drawing.Size(658, 826);
         this.tabControl1.TabIndex = 0;
         // 
         // tabPage1
         // 
         this.tabPage1.Controls.Add(this.FoundVarLabel);
         this.tabPage1.Controls.Add(this.VarClearSearchButton);
         this.tabPage1.Controls.Add(this.VarListView);
         this.tabPage1.Controls.Add(this.VarSearchGridControl);
         this.tabPage1.Controls.Add(this.VarSearchBetterPropertyGrid);
         this.tabPage1.Controls.Add(this.VarSearchButton);
         this.tabPage1.Location = new System.Drawing.Point(4, 22);
         this.tabPage1.Name = "tabPage1";
         this.tabPage1.Padding = new System.Windows.Forms.Padding(3);
         this.tabPage1.Size = new System.Drawing.Size(650, 800);
         this.tabPage1.TabIndex = 0;
         this.tabPage1.Text = "Variables";
         this.tabPage1.UseVisualStyleBackColor = true;
         // 
         // FoundVarLabel
         // 
         this.FoundVarLabel.AutoSize = true;
         this.FoundVarLabel.Location = new System.Drawing.Point(6, 366);
         this.FoundVarLabel.Name = "FoundVarLabel";
         this.FoundVarLabel.Size = new System.Drawing.Size(0, 13);
         this.FoundVarLabel.TabIndex = 7;
         // 
         // VarClearSearchButton
         // 
         this.VarClearSearchButton.Location = new System.Drawing.Point(50, 122);
         this.VarClearSearchButton.Name = "VarClearSearchButton";
         this.VarClearSearchButton.Size = new System.Drawing.Size(90, 23);
         this.VarClearSearchButton.TabIndex = 6;
         this.VarClearSearchButton.Text = "Clear";
         this.VarClearSearchButton.UseVisualStyleBackColor = true;
         this.VarClearSearchButton.Click += new System.EventHandler(this.VarClearSearchButton_Click);
         // 
         // VarListView
         // 
         this.VarListView.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.VarListView.Location = new System.Drawing.Point(9, 151);
         this.VarListView.Name = "VarListView";
         this.VarListView.Size = new System.Drawing.Size(635, 207);
         this.VarListView.Sorting = System.Windows.Forms.SortOrder.Ascending;
         this.VarListView.TabIndex = 5;
         this.VarListView.UseCompatibleStateImageBehavior = false;
         this.VarListView.View = System.Windows.Forms.View.List;
         this.VarListView.SelectedIndexChanged += new System.EventHandler(this.VarListView_SelectedIndexChanged);
         // 
         // VarSearchGridControl
         // 
         this.VarSearchGridControl.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.VarSearchGridControl.DataRowTemplate = this.dataRowTemplate1;
         this.VarSearchGridControl.FixedHeaderRows.Add(this.groupByRow1);
         this.VarSearchGridControl.FixedHeaderRows.Add(this.columnManagerRow1);
         this.VarSearchGridControl.Location = new System.Drawing.Point(9, 382);
         this.VarSearchGridControl.Name = "VarSearchGridControl";
         this.VarSearchGridControl.Size = new System.Drawing.Size(635, 403);
         this.VarSearchGridControl.TabIndex = 4;
         // 
         // VarSearchButton
         // 
         this.VarSearchButton.Location = new System.Drawing.Point(205, 122);
         this.VarSearchButton.Name = "VarSearchButton";
         this.VarSearchButton.Size = new System.Drawing.Size(75, 23);
         this.VarSearchButton.TabIndex = 1;
         this.VarSearchButton.Text = "Search";
         this.VarSearchButton.UseVisualStyleBackColor = true;
         this.VarSearchButton.Click += new System.EventHandler(this.VarSearchButton_Click);
         // 
         // tabPage2
         // 
         this.tabPage2.Controls.Add(this.TriggerGridControl);
         this.tabPage2.Controls.Add(this.TriggerClearButton);
         this.tabPage2.Controls.Add(this.TriggerSearchButton);
         this.tabPage2.Controls.Add(this.TriggerBetterPropertyGrid);
         this.tabPage2.Location = new System.Drawing.Point(4, 22);
         this.tabPage2.Name = "tabPage2";
         this.tabPage2.Padding = new System.Windows.Forms.Padding(3);
         this.tabPage2.Size = new System.Drawing.Size(650, 800);
         this.tabPage2.TabIndex = 1;
         this.tabPage2.Text = "Triggers";
         this.tabPage2.UseVisualStyleBackColor = true;
         // 
         // TriggerGridControl
         // 
         this.TriggerGridControl.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.TriggerGridControl.DataRowTemplate = this.dataRowTemplate2;
         this.TriggerGridControl.FixedHeaderRows.Add(this.groupByRow2);
         this.TriggerGridControl.FixedHeaderRows.Add(this.columnManagerRow2);
         this.TriggerGridControl.Location = new System.Drawing.Point(6, 174);
         this.TriggerGridControl.Name = "TriggerGridControl";
         this.TriggerGridControl.Size = new System.Drawing.Size(638, 620);
         this.TriggerGridControl.TabIndex = 3;
         // 
         // TriggerClearButton
         // 
         this.TriggerClearButton.Location = new System.Drawing.Point(48, 132);
         this.TriggerClearButton.Name = "TriggerClearButton";
         this.TriggerClearButton.Size = new System.Drawing.Size(75, 23);
         this.TriggerClearButton.TabIndex = 2;
         this.TriggerClearButton.Text = "Clear";
         this.TriggerClearButton.UseVisualStyleBackColor = true;
         this.TriggerClearButton.Click += new System.EventHandler(this.TriggerClearButton_Click);
         // 
         // TriggerSearchButton
         // 
         this.TriggerSearchButton.Location = new System.Drawing.Point(186, 132);
         this.TriggerSearchButton.Name = "TriggerSearchButton";
         this.TriggerSearchButton.Size = new System.Drawing.Size(75, 23);
         this.TriggerSearchButton.TabIndex = 0;
         this.TriggerSearchButton.Text = "Search";
         this.TriggerSearchButton.UseVisualStyleBackColor = true;
         this.TriggerSearchButton.Click += new System.EventHandler(this.TriggerSearchButton_Click);
         // 
         // tabPage3
         // 
         this.tabPage3.Controls.Add(this.ComponentGridControl);
         this.tabPage3.Controls.Add(this.ComponentClearButton);
         this.tabPage3.Controls.Add(this.ComponentSearchButton);
         this.tabPage3.Controls.Add(this.ComponentBetterPropertyGrid);
         this.tabPage3.Location = new System.Drawing.Point(4, 22);
         this.tabPage3.Name = "tabPage3";
         this.tabPage3.Padding = new System.Windows.Forms.Padding(3);
         this.tabPage3.Size = new System.Drawing.Size(650, 800);
         this.tabPage3.TabIndex = 2;
         this.tabPage3.Text = "Conditions / Effects";
         this.tabPage3.UseVisualStyleBackColor = true;
         // 
         // ComponentGridControl
         // 
         this.ComponentGridControl.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.ComponentGridControl.DataRowTemplate = this.dataRowTemplate3;
         this.ComponentGridControl.FixedHeaderRows.Add(this.groupByRow3);
         this.ComponentGridControl.FixedHeaderRows.Add(this.columnManagerRow3);
         this.ComponentGridControl.Location = new System.Drawing.Point(6, 172);
         this.ComponentGridControl.Name = "ComponentGridControl";
         this.ComponentGridControl.Size = new System.Drawing.Size(638, 622);
         this.ComponentGridControl.TabIndex = 5;
         // 
         // ComponentClearButton
         // 
         this.ComponentClearButton.Location = new System.Drawing.Point(51, 133);
         this.ComponentClearButton.Name = "ComponentClearButton";
         this.ComponentClearButton.Size = new System.Drawing.Size(75, 23);
         this.ComponentClearButton.TabIndex = 4;
         this.ComponentClearButton.Text = "Clear";
         this.ComponentClearButton.UseVisualStyleBackColor = true;
         this.ComponentClearButton.Click += new System.EventHandler(this.ComponentClearButton_Click);
         // 
         // ComponentSearchButton
         // 
         this.ComponentSearchButton.Location = new System.Drawing.Point(201, 132);
         this.ComponentSearchButton.Name = "ComponentSearchButton";
         this.ComponentSearchButton.Size = new System.Drawing.Size(75, 23);
         this.ComponentSearchButton.TabIndex = 3;
         this.ComponentSearchButton.Text = "Search";
         this.ComponentSearchButton.UseVisualStyleBackColor = true;
         this.ComponentSearchButton.Click += new System.EventHandler(this.ComponentSearchButton_Click);
         // 
         // VarSearchBetterPropertyGrid
         // 
         this.VarSearchBetterPropertyGrid.LastRowHack = false;
         this.VarSearchBetterPropertyGrid.Location = new System.Drawing.Point(9, 16);
         this.VarSearchBetterPropertyGrid.Name = "VarSearchBetterPropertyGrid";
         this.VarSearchBetterPropertyGrid.Size = new System.Drawing.Size(342, 100);
         this.VarSearchBetterPropertyGrid.TabIndex = 3;
         // 
         // TriggerBetterPropertyGrid
         // 
         this.TriggerBetterPropertyGrid.LastRowHack = false;
         this.TriggerBetterPropertyGrid.Location = new System.Drawing.Point(6, 6);
         this.TriggerBetterPropertyGrid.Name = "TriggerBetterPropertyGrid";
         this.TriggerBetterPropertyGrid.Size = new System.Drawing.Size(333, 120);
         this.TriggerBetterPropertyGrid.TabIndex = 1;
         // 
         // ComponentBetterPropertyGrid
         // 
         this.ComponentBetterPropertyGrid.LastRowHack = false;
         this.ComponentBetterPropertyGrid.Location = new System.Drawing.Point(6, 6);
         this.ComponentBetterPropertyGrid.Name = "ComponentBetterPropertyGrid";
         this.ComponentBetterPropertyGrid.Size = new System.Drawing.Size(333, 120);
         this.ComponentBetterPropertyGrid.TabIndex = 2;
         // 
         // TriggerSearch
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.tabControl1);
         this.Name = "TriggerSearch";
         this.Size = new System.Drawing.Size(658, 826);
         this.tabControl1.ResumeLayout(false);
         this.tabPage1.ResumeLayout(false);
         this.tabPage1.PerformLayout();
         ((System.ComponentModel.ISupportInitialize)(this.VarSearchGridControl)).EndInit();
         ((System.ComponentModel.ISupportInitialize)(this.dataRowTemplate1)).EndInit();
         ((System.ComponentModel.ISupportInitialize)(this.columnManagerRow1)).EndInit();
         this.tabPage2.ResumeLayout(false);
         ((System.ComponentModel.ISupportInitialize)(this.TriggerGridControl)).EndInit();
         ((System.ComponentModel.ISupportInitialize)(this.dataRowTemplate2)).EndInit();
         ((System.ComponentModel.ISupportInitialize)(this.columnManagerRow2)).EndInit();
         this.tabPage3.ResumeLayout(false);
         ((System.ComponentModel.ISupportInitialize)(this.ComponentGridControl)).EndInit();
         ((System.ComponentModel.ISupportInitialize)(this.dataRowTemplate3)).EndInit();
         ((System.ComponentModel.ISupportInitialize)(this.columnManagerRow3)).EndInit();
         this.ResumeLayout(false);

      }

      #endregion

      private System.Windows.Forms.TabControl tabControl1;
      private System.Windows.Forms.TabPage tabPage1;
      private System.Windows.Forms.Button VarSearchButton;
      private System.Windows.Forms.TabPage tabPage2;
      private System.Windows.Forms.TabPage tabPage3;
      private EditorCore.BetterPropertyGrid VarSearchBetterPropertyGrid;
      private Xceed.Grid.GridControl VarSearchGridControl;
      private Xceed.Grid.DataRow dataRowTemplate1;
      private Xceed.Grid.GroupByRow groupByRow1;
      private Xceed.Grid.ColumnManagerRow columnManagerRow1;
      private System.Windows.Forms.ListView VarListView;
      private System.Windows.Forms.Button VarClearSearchButton;
      private System.Windows.Forms.Label FoundVarLabel;
      private EditorCore.BetterPropertyGrid TriggerBetterPropertyGrid;
      private System.Windows.Forms.Button TriggerSearchButton;
      private System.Windows.Forms.Button TriggerClearButton;
      private System.Windows.Forms.Button ComponentClearButton;
      private System.Windows.Forms.Button ComponentSearchButton;
      private EditorCore.BetterPropertyGrid ComponentBetterPropertyGrid;
      private Xceed.Grid.GridControl TriggerGridControl;
      private Xceed.Grid.DataRow dataRowTemplate2;
      private Xceed.Grid.GroupByRow groupByRow2;
      private Xceed.Grid.ColumnManagerRow columnManagerRow2;
      private Xceed.Grid.GridControl ComponentGridControl;
      private Xceed.Grid.DataRow dataRowTemplate3;
      private Xceed.Grid.GroupByRow groupByRow3;
      private Xceed.Grid.ColumnManagerRow columnManagerRow3;
   }
}
