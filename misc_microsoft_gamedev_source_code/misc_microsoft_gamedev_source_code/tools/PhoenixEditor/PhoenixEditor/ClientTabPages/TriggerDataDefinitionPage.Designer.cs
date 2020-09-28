namespace PhoenixEditor
{
   partial class TriggerDataDefinitionPage
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
         this.ConditionsListBox = new System.Windows.Forms.ListBox();
         this.VersionListBox = new System.Windows.Forms.ListBox();
         this.InVariblesList = new PhoenixEditor.ScenarioEditor.BasicTypedSuperList();
         this.OutVariablesList = new PhoenixEditor.ScenarioEditor.BasicTypedSuperList();
         this.ItemPropertyGrid = new EditorCore.BetterPropertyGrid();
         this.UpdateVersionsButton = new System.Windows.Forms.Button();
         this.SaveButton = new System.Windows.Forms.Button();
         this.label1 = new System.Windows.Forms.Label();
         this.label2 = new System.Windows.Forms.Label();
         this.SaveToNewVersionButton = new System.Windows.Forms.Button();
         this.AddConditionButton = new System.Windows.Forms.Button();
         this.AddEffectButton = new System.Windows.Forms.Button();
         this.EffectsListBox = new System.Windows.Forms.ListBox();
         this.groupBox1 = new System.Windows.Forms.GroupBox();
         this.ConversionList = new PhoenixEditor.ScenarioEditor.BasicTypedSuperList();
         this.label3 = new System.Windows.Forms.Label();
         this.SaveNewNameButton = new System.Windows.Forms.Button();
         this.NameTextBox = new System.Windows.Forms.TextBox();
         this.SaveToSelectedButton = new System.Windows.Forms.Button();
         this.groupBox2 = new System.Windows.Forms.GroupBox();
         this.gridControl1 = new Xceed.Grid.GridControl();
         this.dataRowTemplate1 = new Xceed.Grid.DataRow();
         this.groupByRow1 = new Xceed.Grid.GroupByRow();
         this.columnManagerRow1 = new Xceed.Grid.ColumnManagerRow();
         this.ScanFileButton = new System.Windows.Forms.Button();
         this.ScanFillesButton = new System.Windows.Forms.Button();
         this.HideObsoleteCheckBox = new System.Windows.Forms.CheckBox();
         this.tabControl1 = new System.Windows.Forms.TabControl();
         this.tabPage1 = new System.Windows.Forms.TabPage();
         this.label4 = new System.Windows.Forms.Label();
         this.tabPage3 = new System.Windows.Forms.TabPage();
         this.tabPage2 = new System.Windows.Forms.TabPage();
         this.groupBox1.SuspendLayout();
         this.groupBox2.SuspendLayout();
         ((System.ComponentModel.ISupportInitialize)(this.gridControl1)).BeginInit();
         ((System.ComponentModel.ISupportInitialize)(this.dataRowTemplate1)).BeginInit();
         ((System.ComponentModel.ISupportInitialize)(this.columnManagerRow1)).BeginInit();
         this.tabControl1.SuspendLayout();
         this.tabPage1.SuspendLayout();
         this.tabPage3.SuspendLayout();
         this.SuspendLayout();
         // 
         // ConditionsListBox
         // 
         this.ConditionsListBox.FormattingEnabled = true;
         this.ConditionsListBox.Location = new System.Drawing.Point(6, 6);
         this.ConditionsListBox.Name = "ConditionsListBox";
         this.ConditionsListBox.Size = new System.Drawing.Size(218, 238);
         this.ConditionsListBox.TabIndex = 0;
         // 
         // VersionListBox
         // 
         this.VersionListBox.FormattingEnabled = true;
         this.VersionListBox.Location = new System.Drawing.Point(17, 66);
         this.VersionListBox.Name = "VersionListBox";
         this.VersionListBox.Size = new System.Drawing.Size(327, 212);
         this.VersionListBox.TabIndex = 1;
         // 
         // InVariblesList
         // 
         this.InVariblesList.GridColor = System.Drawing.SystemColors.ActiveCaption;
         this.InVariblesList.Location = new System.Drawing.Point(17, 340);
         this.InVariblesList.Name = "InVariblesList";
         this.InVariblesList.Size = new System.Drawing.Size(481, 331);
         this.InVariblesList.TabIndex = 2;
         this.InVariblesList.UseLabels = true;
         this.InVariblesList.WrapContents = false;
         // 
         // OutVariablesList
         // 
         this.OutVariablesList.GridColor = System.Drawing.SystemColors.ActiveCaption;
         this.OutVariablesList.Location = new System.Drawing.Point(504, 340);
         this.OutVariablesList.Name = "OutVariablesList";
         this.OutVariablesList.Size = new System.Drawing.Size(499, 196);
         this.OutVariablesList.TabIndex = 3;
         this.OutVariablesList.UseLabels = true;
         this.OutVariablesList.WrapContents = false;
         // 
         // ItemPropertyGrid
         // 
         this.ItemPropertyGrid.Location = new System.Drawing.Point(359, 66);
         this.ItemPropertyGrid.Name = "ItemPropertyGrid";
         this.ItemPropertyGrid.Size = new System.Drawing.Size(644, 255);
         this.ItemPropertyGrid.TabIndex = 4;
         // 
         // UpdateVersionsButton
         // 
         this.UpdateVersionsButton.BackColor = System.Drawing.SystemColors.Info;
         this.UpdateVersionsButton.Location = new System.Drawing.Point(17, 172);
         this.UpdateVersionsButton.Name = "UpdateVersionsButton";
         this.UpdateVersionsButton.Size = new System.Drawing.Size(75, 23);
         this.UpdateVersionsButton.TabIndex = 5;
         this.UpdateVersionsButton.Text = "Update IDs";
         this.UpdateVersionsButton.UseVisualStyleBackColor = false;
         this.UpdateVersionsButton.Click += new System.EventHandler(this.UpdateVersionsButton_Click);
         // 
         // SaveButton
         // 
         this.SaveButton.BackColor = System.Drawing.SystemColors.Info;
         this.SaveButton.Location = new System.Drawing.Point(17, 201);
         this.SaveButton.Name = "SaveButton";
         this.SaveButton.Size = new System.Drawing.Size(75, 23);
         this.SaveButton.TabIndex = 6;
         this.SaveButton.Text = "Save All";
         this.SaveButton.UseVisualStyleBackColor = false;
         this.SaveButton.Click += new System.EventHandler(this.SaveButton_Click);
         // 
         // label1
         // 
         this.label1.AutoSize = true;
         this.label1.Location = new System.Drawing.Point(14, 324);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(87, 13);
         this.label1.TabIndex = 7;
         this.label1.Text = "Input Parameters";
         // 
         // label2
         // 
         this.label2.AutoSize = true;
         this.label2.Location = new System.Drawing.Point(501, 324);
         this.label2.Name = "label2";
         this.label2.Size = new System.Drawing.Size(95, 13);
         this.label2.TabIndex = 8;
         this.label2.Text = "Output Parameters";
         // 
         // SaveToNewVersionButton
         // 
         this.SaveToNewVersionButton.Location = new System.Drawing.Point(17, 287);
         this.SaveToNewVersionButton.Name = "SaveToNewVersionButton";
         this.SaveToNewVersionButton.Size = new System.Drawing.Size(119, 23);
         this.SaveToNewVersionButton.TabIndex = 9;
         this.SaveToNewVersionButton.Text = "Save as New Version";
         this.SaveToNewVersionButton.UseVisualStyleBackColor = true;
         this.SaveToNewVersionButton.Click += new System.EventHandler(this.SaveToNewVersionButton_Click);
         // 
         // AddConditionButton
         // 
         this.AddConditionButton.Location = new System.Drawing.Point(6, 250);
         this.AddConditionButton.Name = "AddConditionButton";
         this.AddConditionButton.Size = new System.Drawing.Size(119, 23);
         this.AddConditionButton.TabIndex = 10;
         this.AddConditionButton.Text = "Add Condition";
         this.AddConditionButton.UseVisualStyleBackColor = true;
         this.AddConditionButton.Click += new System.EventHandler(this.AddConditionButton_Click);
         // 
         // AddEffectButton
         // 
         this.AddEffectButton.Location = new System.Drawing.Point(6, 535);
         this.AddEffectButton.Name = "AddEffectButton";
         this.AddEffectButton.Size = new System.Drawing.Size(119, 23);
         this.AddEffectButton.TabIndex = 11;
         this.AddEffectButton.Text = "Add Effect";
         this.AddEffectButton.UseVisualStyleBackColor = true;
         this.AddEffectButton.Click += new System.EventHandler(this.AddEffectButton_Click);
         // 
         // EffectsListBox
         // 
         this.EffectsListBox.FormattingEnabled = true;
         this.EffectsListBox.Location = new System.Drawing.Point(6, 294);
         this.EffectsListBox.Name = "EffectsListBox";
         this.EffectsListBox.Size = new System.Drawing.Size(218, 238);
         this.EffectsListBox.TabIndex = 13;
         // 
         // groupBox1
         // 
         this.groupBox1.BackColor = System.Drawing.SystemColors.ControlLight;
         this.groupBox1.Controls.Add(this.ConversionList);
         this.groupBox1.Controls.Add(this.label3);
         this.groupBox1.Controls.Add(this.SaveNewNameButton);
         this.groupBox1.Controls.Add(this.NameTextBox);
         this.groupBox1.Controls.Add(this.SaveToSelectedButton);
         this.groupBox1.Controls.Add(this.VersionListBox);
         this.groupBox1.Controls.Add(this.InVariblesList);
         this.groupBox1.Controls.Add(this.OutVariablesList);
         this.groupBox1.Controls.Add(this.SaveToNewVersionButton);
         this.groupBox1.Controls.Add(this.label1);
         this.groupBox1.Controls.Add(this.ItemPropertyGrid);
         this.groupBox1.Controls.Add(this.label2);
         this.groupBox1.Location = new System.Drawing.Point(230, -4);
         this.groupBox1.Name = "groupBox1";
         this.groupBox1.Size = new System.Drawing.Size(1012, 677);
         this.groupBox1.TabIndex = 14;
         this.groupBox1.TabStop = false;
         // 
         // ConversionList
         // 
         this.ConversionList.GridColor = System.Drawing.SystemColors.ActiveCaption;
         this.ConversionList.Location = new System.Drawing.Point(504, 556);
         this.ConversionList.Name = "ConversionList";
         this.ConversionList.Size = new System.Drawing.Size(502, 115);
         this.ConversionList.TabIndex = 14;
         this.ConversionList.UseLabels = true;
         this.ConversionList.WrapContents = false;
         // 
         // label3
         // 
         this.label3.AutoSize = true;
         this.label3.Location = new System.Drawing.Point(504, 539);
         this.label3.Name = "label3";
         this.label3.Size = new System.Drawing.Size(292, 13);
         this.label3.TabIndex = 13;
         this.label3.Text = "Automatic Conversion Overrides (with respect to last version)";
         // 
         // SaveNewNameButton
         // 
         this.SaveNewNameButton.Location = new System.Drawing.Point(703, 20);
         this.SaveNewNameButton.Name = "SaveNewNameButton";
         this.SaveNewNameButton.Size = new System.Drawing.Size(75, 23);
         this.SaveNewNameButton.TabIndex = 12;
         this.SaveNewNameButton.Text = "Save Name";
         this.SaveNewNameButton.UseVisualStyleBackColor = true;
         // 
         // NameTextBox
         // 
         this.NameTextBox.BackColor = System.Drawing.SystemColors.Info;
         this.NameTextBox.Font = new System.Drawing.Font("Microsoft Sans Serif", 14F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
         this.NameTextBox.Location = new System.Drawing.Point(17, 20);
         this.NameTextBox.Name = "NameTextBox";
         this.NameTextBox.Size = new System.Drawing.Size(679, 29);
         this.NameTextBox.TabIndex = 11;
         // 
         // SaveToSelectedButton
         // 
         this.SaveToSelectedButton.Location = new System.Drawing.Point(142, 287);
         this.SaveToSelectedButton.Name = "SaveToSelectedButton";
         this.SaveToSelectedButton.Size = new System.Drawing.Size(117, 23);
         this.SaveToSelectedButton.TabIndex = 10;
         this.SaveToSelectedButton.Text = "Save to Selected";
         this.SaveToSelectedButton.UseVisualStyleBackColor = true;
         this.SaveToSelectedButton.Click += new System.EventHandler(this.SaveToSelectedButton_Click);
         // 
         // groupBox2
         // 
         this.groupBox2.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.groupBox2.Controls.Add(this.gridControl1);
         this.groupBox2.Controls.Add(this.ScanFileButton);
         this.groupBox2.Controls.Add(this.ScanFillesButton);
         this.groupBox2.Controls.Add(this.UpdateVersionsButton);
         this.groupBox2.Controls.Add(this.SaveButton);
         this.groupBox2.Location = new System.Drawing.Point(3, 3);
         this.groupBox2.Name = "groupBox2";
         this.groupBox2.Size = new System.Drawing.Size(1227, 740);
         this.groupBox2.TabIndex = 15;
         this.groupBox2.TabStop = false;
         this.groupBox2.Text = "Helper Tools";
         // 
         // gridControl1
         // 
         this.gridControl1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.gridControl1.DataRowTemplate = this.dataRowTemplate1;
         this.gridControl1.FixedHeaderRows.Add(this.groupByRow1);
         this.gridControl1.FixedHeaderRows.Add(this.columnManagerRow1);
         this.gridControl1.Location = new System.Drawing.Point(115, 19);
         this.gridControl1.Name = "gridControl1";
         this.gridControl1.Size = new System.Drawing.Size(1106, 715);
         this.gridControl1.TabIndex = 7;
         // 
         // ScanFileButton
         // 
         this.ScanFileButton.Location = new System.Drawing.Point(17, 59);
         this.ScanFileButton.Name = "ScanFileButton";
         this.ScanFileButton.Size = new System.Drawing.Size(92, 23);
         this.ScanFileButton.TabIndex = 2;
         this.ScanFileButton.Text = "Scan File(s)...";
         this.ScanFileButton.UseVisualStyleBackColor = true;
         this.ScanFileButton.Click += new System.EventHandler(this.ScanFileButton_Click);
         // 
         // ScanFillesButton
         // 
         this.ScanFillesButton.Location = new System.Drawing.Point(17, 30);
         this.ScanFillesButton.Name = "ScanFillesButton";
         this.ScanFillesButton.Size = new System.Drawing.Size(92, 23);
         this.ScanFillesButton.TabIndex = 0;
         this.ScanFillesButton.Text = "Scan All Files";
         this.ScanFillesButton.UseVisualStyleBackColor = true;
         this.ScanFillesButton.Click += new System.EventHandler(this.ScanFillesButton_Click);
         // 
         // HideObsoleteCheckBox
         // 
         this.HideObsoleteCheckBox.AutoSize = true;
         this.HideObsoleteCheckBox.Checked = true;
         this.HideObsoleteCheckBox.CheckState = System.Windows.Forms.CheckState.Checked;
         this.HideObsoleteCheckBox.Location = new System.Drawing.Point(6, 565);
         this.HideObsoleteCheckBox.Name = "HideObsoleteCheckBox";
         this.HideObsoleteCheckBox.Size = new System.Drawing.Size(183, 17);
         this.HideObsoleteCheckBox.TabIndex = 16;
         this.HideObsoleteCheckBox.Text = "Hide Obsolete Conditions/Effects";
         this.HideObsoleteCheckBox.UseVisualStyleBackColor = true;
         this.HideObsoleteCheckBox.CheckedChanged += new System.EventHandler(this.HideObsoleteCheckBox_CheckedChanged);
         // 
         // tabControl1
         // 
         this.tabControl1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.tabControl1.Controls.Add(this.tabPage1);
         this.tabControl1.Controls.Add(this.tabPage3);
         this.tabControl1.Controls.Add(this.tabPage2);
         this.tabControl1.Location = new System.Drawing.Point(3, 3);
         this.tabControl1.Name = "tabControl1";
         this.tabControl1.SelectedIndex = 0;
         this.tabControl1.Size = new System.Drawing.Size(1256, 772);
         this.tabControl1.TabIndex = 17;
         // 
         // tabPage1
         // 
         this.tabPage1.Controls.Add(this.label4);
         this.tabPage1.Controls.Add(this.ConditionsListBox);
         this.tabPage1.Controls.Add(this.HideObsoleteCheckBox);
         this.tabPage1.Controls.Add(this.AddConditionButton);
         this.tabPage1.Controls.Add(this.AddEffectButton);
         this.tabPage1.Controls.Add(this.groupBox1);
         this.tabPage1.Controls.Add(this.EffectsListBox);
         this.tabPage1.Location = new System.Drawing.Point(4, 22);
         this.tabPage1.Name = "tabPage1";
         this.tabPage1.Padding = new System.Windows.Forms.Padding(3);
         this.tabPage1.Size = new System.Drawing.Size(1248, 746);
         this.tabPage1.TabIndex = 0;
         this.tabPage1.Text = "Trigger Parameters";
         this.tabPage1.UseVisualStyleBackColor = true;
         // 
         // label4
         // 
         this.label4.AutoSize = true;
         this.label4.Location = new System.Drawing.Point(154, 703);
         this.label4.Name = "label4";
         this.label4.Size = new System.Drawing.Size(294, 13);
         this.label4.TabIndex = 17;
         this.label4.Text = "WTF, where did the scanner go?   ... it is on another tab now";
         // 
         // tabPage3
         // 
         this.tabPage3.Controls.Add(this.groupBox2);
         this.tabPage3.Location = new System.Drawing.Point(4, 22);
         this.tabPage3.Name = "tabPage3";
         this.tabPage3.Size = new System.Drawing.Size(1248, 746);
         this.tabPage3.TabIndex = 2;
         this.tabPage3.Text = "Scanning";
         this.tabPage3.UseVisualStyleBackColor = true;
         // 
         // tabPage2
         // 
         this.tabPage2.Location = new System.Drawing.Point(4, 22);
         this.tabPage2.Name = "tabPage2";
         this.tabPage2.Padding = new System.Windows.Forms.Padding(3);
         this.tabPage2.Size = new System.Drawing.Size(1248, 746);
         this.tabPage2.TabIndex = 1;
         this.tabPage2.Text = "Templates";
         this.tabPage2.UseVisualStyleBackColor = true;
         // 
         // TriggerDataDefinitionPage
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.Controls.Add(this.tabControl1);
         this.Name = "TriggerDataDefinitionPage";
         this.Size = new System.Drawing.Size(1262, 778);
         this.groupBox1.ResumeLayout(false);
         this.groupBox1.PerformLayout();
         this.groupBox2.ResumeLayout(false);
         ((System.ComponentModel.ISupportInitialize)(this.gridControl1)).EndInit();
         ((System.ComponentModel.ISupportInitialize)(this.dataRowTemplate1)).EndInit();
         ((System.ComponentModel.ISupportInitialize)(this.columnManagerRow1)).EndInit();
         this.tabControl1.ResumeLayout(false);
         this.tabPage1.ResumeLayout(false);
         this.tabPage1.PerformLayout();
         this.tabPage3.ResumeLayout(false);
         this.ResumeLayout(false);

      }

      #endregion

      private System.Windows.Forms.ListBox ConditionsListBox;
      private System.Windows.Forms.ListBox VersionListBox;
      private PhoenixEditor.ScenarioEditor.BasicTypedSuperList InVariblesList;
      private PhoenixEditor.ScenarioEditor.BasicTypedSuperList OutVariablesList;
      private EditorCore.BetterPropertyGrid ItemPropertyGrid;
      private System.Windows.Forms.Button UpdateVersionsButton;
      private System.Windows.Forms.Button SaveButton;
      private System.Windows.Forms.Label label1;
      private System.Windows.Forms.Label label2;
      private System.Windows.Forms.Button SaveToNewVersionButton;
      private System.Windows.Forms.Button AddConditionButton;
      private System.Windows.Forms.Button AddEffectButton;
      private System.Windows.Forms.ListBox EffectsListBox;
      private System.Windows.Forms.GroupBox groupBox1;
      private System.Windows.Forms.Button SaveToSelectedButton;
      private System.Windows.Forms.Button SaveNewNameButton;
      private System.Windows.Forms.TextBox NameTextBox;
      private System.Windows.Forms.GroupBox groupBox2;
      private System.Windows.Forms.Button ScanFillesButton;
      private System.Windows.Forms.CheckBox HideObsoleteCheckBox;
      private System.Windows.Forms.Button ScanFileButton;
      private System.Windows.Forms.TabControl tabControl1;
      private System.Windows.Forms.TabPage tabPage1;
      private System.Windows.Forms.TabPage tabPage2;
      private System.Windows.Forms.Label label3;
      private PhoenixEditor.ScenarioEditor.BasicTypedSuperList ConversionList;
      private System.Windows.Forms.TabPage tabPage3;
      private System.Windows.Forms.Label label4;
      private Xceed.Grid.GridControl gridControl1;
      private Xceed.Grid.DataRow dataRowTemplate1;
      private Xceed.Grid.GroupByRow groupByRow1;
      private Xceed.Grid.ColumnManagerRow columnManagerRow1;
   }
}
