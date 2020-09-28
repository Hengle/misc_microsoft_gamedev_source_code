namespace PhoenixEditor.ScenarioEditor
{
   partial class TemplateVersionControl
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
         this.TemplateNamesListBox = new System.Windows.Forms.ListBox();
         this.HideObsoleteCheckBox = new System.Windows.Forms.CheckBox();
         this.AddTemplateButton = new System.Windows.Forms.Button();
         this.groupBox1 = new System.Windows.Forms.GroupBox();
         this.AddNewVersionButton = new System.Windows.Forms.Button();
         this.SaveNewNameButton = new System.Windows.Forms.Button();
         this.NameTextBox = new System.Windows.Forms.TextBox();
         this.SaveChangesButton = new System.Windows.Forms.Button();
         this.ItemPropertyGrid = new EditorCore.BetterPropertyGrid();
         this.VersionsListBox = new System.Windows.Forms.ListBox();
         this.groupBox2 = new System.Windows.Forms.GroupBox();
         this.ScanFileButton = new System.Windows.Forms.Button();
         this.ScanResultsTreeView = new System.Windows.Forms.TreeView();
         this.ScanFillesButton = new System.Windows.Forms.Button();
         this.groupBox1.SuspendLayout();
         this.groupBox2.SuspendLayout();
         this.SuspendLayout();
         // 
         // TemplateNamesListBox
         // 
         this.TemplateNamesListBox.FormattingEnabled = true;
         this.TemplateNamesListBox.Location = new System.Drawing.Point(14, 24);
         this.TemplateNamesListBox.Name = "TemplateNamesListBox";
         this.TemplateNamesListBox.Size = new System.Drawing.Size(216, 329);
         this.TemplateNamesListBox.TabIndex = 0;
         this.TemplateNamesListBox.SelectedIndexChanged += new System.EventHandler(this.TemplateNamesListBox_SelectedIndexChanged);
         // 
         // HideObsoleteCheckBox
         // 
         this.HideObsoleteCheckBox.AutoSize = true;
         this.HideObsoleteCheckBox.Location = new System.Drawing.Point(14, 395);
         this.HideObsoleteCheckBox.Name = "HideObsoleteCheckBox";
         this.HideObsoleteCheckBox.Size = new System.Drawing.Size(93, 17);
         this.HideObsoleteCheckBox.TabIndex = 1;
         this.HideObsoleteCheckBox.Text = "Hide Obsolete";
         this.HideObsoleteCheckBox.UseVisualStyleBackColor = true;
         this.HideObsoleteCheckBox.CheckedChanged += new System.EventHandler(this.HideObsoleteCheckBox_CheckedChanged);
         // 
         // AddTemplateButton
         // 
         this.AddTemplateButton.Location = new System.Drawing.Point(14, 360);
         this.AddTemplateButton.Name = "AddTemplateButton";
         this.AddTemplateButton.Size = new System.Drawing.Size(118, 23);
         this.AddTemplateButton.TabIndex = 2;
         this.AddTemplateButton.Text = "Add Template";
         this.AddTemplateButton.UseVisualStyleBackColor = true;
         this.AddTemplateButton.Click += new System.EventHandler(this.AddTemplateButton_Click);
         // 
         // groupBox1
         // 
         this.groupBox1.Controls.Add(this.AddNewVersionButton);
         this.groupBox1.Controls.Add(this.SaveNewNameButton);
         this.groupBox1.Controls.Add(this.NameTextBox);
         this.groupBox1.Controls.Add(this.ItemPropertyGrid);
         this.groupBox1.Controls.Add(this.VersionsListBox);
         this.groupBox1.Location = new System.Drawing.Point(247, 24);
         this.groupBox1.Name = "groupBox1";
         this.groupBox1.Size = new System.Drawing.Size(786, 329);
         this.groupBox1.TabIndex = 3;
         this.groupBox1.TabStop = false;
         // 
         // AddNewVersionButton
         // 
         this.AddNewVersionButton.Location = new System.Drawing.Point(22, 289);
         this.AddNewVersionButton.Name = "AddNewVersionButton";
         this.AddNewVersionButton.Size = new System.Drawing.Size(118, 23);
         this.AddNewVersionButton.TabIndex = 15;
         this.AddNewVersionButton.Text = "Add NewVersion";
         this.AddNewVersionButton.UseVisualStyleBackColor = true;
         this.AddNewVersionButton.Click += new System.EventHandler(this.AddNewVersionButton_Click);
         // 
         // SaveNewNameButton
         // 
         this.SaveNewNameButton.Location = new System.Drawing.Point(691, 19);
         this.SaveNewNameButton.Name = "SaveNewNameButton";
         this.SaveNewNameButton.Size = new System.Drawing.Size(75, 23);
         this.SaveNewNameButton.TabIndex = 14;
         this.SaveNewNameButton.Text = "Save Name";
         this.SaveNewNameButton.UseVisualStyleBackColor = true;
         // 
         // NameTextBox
         // 
         this.NameTextBox.BackColor = System.Drawing.SystemColors.Info;
         this.NameTextBox.Font = new System.Drawing.Font("Microsoft Sans Serif", 14F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
         this.NameTextBox.Location = new System.Drawing.Point(22, 19);
         this.NameTextBox.Name = "NameTextBox";
         this.NameTextBox.Size = new System.Drawing.Size(663, 29);
         this.NameTextBox.TabIndex = 13;
         // 
         // SaveChangesButton
         // 
         this.SaveChangesButton.Location = new System.Drawing.Point(529, 378);
         this.SaveChangesButton.Name = "SaveChangesButton";
         this.SaveChangesButton.Size = new System.Drawing.Size(123, 23);
         this.SaveChangesButton.TabIndex = 6;
         this.SaveChangesButton.Text = "Save Changes";
         this.SaveChangesButton.UseVisualStyleBackColor = true;
         this.SaveChangesButton.Click += new System.EventHandler(this.SaveChangesButton_Click);
         // 
         // ItemPropertyGrid
         // 
         this.ItemPropertyGrid.Location = new System.Drawing.Point(297, 71);
         this.ItemPropertyGrid.Name = "ItemPropertyGrid";
         this.ItemPropertyGrid.Size = new System.Drawing.Size(469, 212);
         this.ItemPropertyGrid.TabIndex = 5;
         // 
         // VersionsListBox
         // 
         this.VersionsListBox.FormattingEnabled = true;
         this.VersionsListBox.Location = new System.Drawing.Point(22, 71);
         this.VersionsListBox.Name = "VersionsListBox";
         this.VersionsListBox.Size = new System.Drawing.Size(250, 212);
         this.VersionsListBox.TabIndex = 0;
         this.VersionsListBox.SelectedValueChanged += new System.EventHandler(this.VersionsListBox_SelectedValueChanged);
         // 
         // groupBox2
         // 
         this.groupBox2.Controls.Add(this.ScanFileButton);
         this.groupBox2.Controls.Add(this.ScanResultsTreeView);
         this.groupBox2.Controls.Add(this.ScanFillesButton);
         this.groupBox2.Location = new System.Drawing.Point(14, 439);
         this.groupBox2.Name = "groupBox2";
         this.groupBox2.Size = new System.Drawing.Size(1019, 320);
         this.groupBox2.TabIndex = 4;
         this.groupBox2.TabStop = false;
         this.groupBox2.Text = "Scan";
         // 
         // ScanFileButton
         // 
         this.ScanFileButton.Location = new System.Drawing.Point(15, 57);
         this.ScanFileButton.Name = "ScanFileButton";
         this.ScanFileButton.Size = new System.Drawing.Size(92, 23);
         this.ScanFileButton.TabIndex = 5;
         this.ScanFileButton.Text = "Scan File(s)...";
         this.ScanFileButton.UseVisualStyleBackColor = true;
         // 
         // ScanResultsTreeView
         // 
         this.ScanResultsTreeView.Location = new System.Drawing.Point(144, 28);
         this.ScanResultsTreeView.Name = "ScanResultsTreeView";
         this.ScanResultsTreeView.Size = new System.Drawing.Size(841, 211);
         this.ScanResultsTreeView.TabIndex = 4;
         // 
         // ScanFillesButton
         // 
         this.ScanFillesButton.Location = new System.Drawing.Point(15, 28);
         this.ScanFillesButton.Name = "ScanFillesButton";
         this.ScanFillesButton.Size = new System.Drawing.Size(92, 23);
         this.ScanFillesButton.TabIndex = 3;
         this.ScanFillesButton.Text = "Scan All Files";
         this.ScanFillesButton.UseVisualStyleBackColor = true;
         // 
         // TemplateVersionControl
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.groupBox2);
         this.Controls.Add(this.groupBox1);
         this.Controls.Add(this.AddTemplateButton);
         this.Controls.Add(this.SaveChangesButton);
         this.Controls.Add(this.HideObsoleteCheckBox);
         this.Controls.Add(this.TemplateNamesListBox);
         this.Name = "TemplateVersionControl";
         this.Size = new System.Drawing.Size(1084, 831);
         this.groupBox1.ResumeLayout(false);
         this.groupBox1.PerformLayout();
         this.groupBox2.ResumeLayout(false);
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private System.Windows.Forms.ListBox TemplateNamesListBox;
      private System.Windows.Forms.CheckBox HideObsoleteCheckBox;
      private System.Windows.Forms.Button AddTemplateButton;
      private System.Windows.Forms.GroupBox groupBox1;
      private System.Windows.Forms.ListBox VersionsListBox;
      private System.Windows.Forms.GroupBox groupBox2;
      private System.Windows.Forms.Button SaveChangesButton;
      private EditorCore.BetterPropertyGrid ItemPropertyGrid;
      private System.Windows.Forms.Button SaveNewNameButton;
      private System.Windows.Forms.TextBox NameTextBox;
      private System.Windows.Forms.Button AddNewVersionButton;
      private System.Windows.Forms.Button ScanFileButton;
      private System.Windows.Forms.TreeView ScanResultsTreeView;
      private System.Windows.Forms.Button ScanFillesButton;
   }
}
