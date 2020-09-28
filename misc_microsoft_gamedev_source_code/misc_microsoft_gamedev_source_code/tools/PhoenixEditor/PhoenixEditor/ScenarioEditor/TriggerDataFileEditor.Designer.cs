namespace PhoenixEditor.ScenarioEditor
{
   partial class TriggerDataFileEditor
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
         this.AddRowButton = new System.Windows.Forms.Button();
         this.LoadButton = new System.Windows.Forms.Button();
         this.SaveButton = new System.Windows.Forms.Button();
         this.SaveAsButton = new System.Windows.Forms.Button();
         this.AddNewTableButton = new System.Windows.Forms.Button();
         this.TableListBox = new System.Windows.Forms.ListBox();
         this.TableNameLabel = new System.Windows.Forms.Label();
         this.TableTypeLabel = new System.Windows.Forms.Label();
         this.hybridGrid1 = new EditorCore.Controls.Micro.HybridGrid();
         this.DeleteRowButton = new System.Windows.Forms.Button();
         this.RowUpButton = new System.Windows.Forms.Button();
         this.RowDownButton = new System.Windows.Forms.Button();
         this.ImportColumnButton = new System.Windows.Forms.Button();
         this.SuspendLayout();
         // 
         // AddRowButton
         // 
         this.AddRowButton.Location = new System.Drawing.Point(250, 528);
         this.AddRowButton.Name = "AddRowButton";
         this.AddRowButton.Size = new System.Drawing.Size(75, 23);
         this.AddRowButton.TabIndex = 2;
         this.AddRowButton.Text = "Add Row";
         this.AddRowButton.UseVisualStyleBackColor = true;
         this.AddRowButton.Click += new System.EventHandler(this.addrow_click);
         // 
         // LoadButton
         // 
         this.LoadButton.Location = new System.Drawing.Point(19, 3);
         this.LoadButton.Name = "LoadButton";
         this.LoadButton.Size = new System.Drawing.Size(53, 23);
         this.LoadButton.TabIndex = 3;
         this.LoadButton.Text = "Load";
         this.LoadButton.UseVisualStyleBackColor = true;
         this.LoadButton.Click += new System.EventHandler(this.LoadButton_Click);
         // 
         // SaveButton
         // 
         this.SaveButton.Location = new System.Drawing.Point(78, 3);
         this.SaveButton.Name = "SaveButton";
         this.SaveButton.Size = new System.Drawing.Size(75, 23);
         this.SaveButton.TabIndex = 4;
         this.SaveButton.Text = "Save";
         this.SaveButton.UseVisualStyleBackColor = true;
         this.SaveButton.Click += new System.EventHandler(this.SaveButton_Click);
         // 
         // SaveAsButton
         // 
         this.SaveAsButton.Location = new System.Drawing.Point(157, 3);
         this.SaveAsButton.Name = "SaveAsButton";
         this.SaveAsButton.Size = new System.Drawing.Size(75, 23);
         this.SaveAsButton.TabIndex = 5;
         this.SaveAsButton.Text = "Save As";
         this.SaveAsButton.UseVisualStyleBackColor = true;
         this.SaveAsButton.Click += new System.EventHandler(this.SaveAsButton_Click);
         // 
         // AddNewTableButton
         // 
         this.AddNewTableButton.Location = new System.Drawing.Point(19, 528);
         this.AddNewTableButton.Name = "AddNewTableButton";
         this.AddNewTableButton.Size = new System.Drawing.Size(104, 23);
         this.AddNewTableButton.TabIndex = 6;
         this.AddNewTableButton.Text = "Add Table";
         this.AddNewTableButton.UseVisualStyleBackColor = true;
         this.AddNewTableButton.Click += new System.EventHandler(this.AddNewTableButton_Click);
         // 
         // TableListBox
         // 
         this.TableListBox.FormattingEnabled = true;
         this.TableListBox.Location = new System.Drawing.Point(19, 32);
         this.TableListBox.Name = "TableListBox";
         this.TableListBox.Size = new System.Drawing.Size(213, 485);
         this.TableListBox.TabIndex = 7;
         this.TableListBox.SelectedIndexChanged += new System.EventHandler(this.TableListBox_SelectedIndexChanged);
         // 
         // TableNameLabel
         // 
         this.TableNameLabel.AutoSize = true;
         this.TableNameLabel.Font = new System.Drawing.Font("Microsoft Sans Serif", 15.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
         this.TableNameLabel.Location = new System.Drawing.Point(290, 32);
         this.TableNameLabel.Name = "TableNameLabel";
         this.TableNameLabel.Size = new System.Drawing.Size(122, 25);
         this.TableNameLabel.TabIndex = 8;
         this.TableNameLabel.Text = "TableName";
         // 
         // TableTypeLabel
         // 
         this.TableTypeLabel.AutoSize = true;
         this.TableTypeLabel.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
         this.TableTypeLabel.Location = new System.Drawing.Point(625, 41);
         this.TableTypeLabel.Name = "TableTypeLabel";
         this.TableTypeLabel.Size = new System.Drawing.Size(82, 20);
         this.TableTypeLabel.TabIndex = 9;
         this.TableTypeLabel.Text = "TableType";
         // 
         // hybridGrid1
         // 
         this.hybridGrid1.Location = new System.Drawing.Point(238, 57);
         this.hybridGrid1.Name = "hybridGrid1";
         this.hybridGrid1.Size = new System.Drawing.Size(860, 481);
         this.hybridGrid1.TabIndex = 0;
         // 
         // DeleteRowButton
         // 
         this.DeleteRowButton.Location = new System.Drawing.Point(331, 528);
         this.DeleteRowButton.Name = "DeleteRowButton";
         this.DeleteRowButton.Size = new System.Drawing.Size(75, 23);
         this.DeleteRowButton.TabIndex = 10;
         this.DeleteRowButton.Text = "DeleteRow";
         this.DeleteRowButton.UseVisualStyleBackColor = true;
         this.DeleteRowButton.Click += new System.EventHandler(this.DeleteRowButton_Click);
         // 
         // RowUpButton
         // 
         this.RowUpButton.Location = new System.Drawing.Point(490, 528);
         this.RowUpButton.Name = "RowUpButton";
         this.RowUpButton.Size = new System.Drawing.Size(75, 23);
         this.RowUpButton.TabIndex = 11;
         this.RowUpButton.Text = "Row Up";
         this.RowUpButton.UseVisualStyleBackColor = true;
         this.RowUpButton.Click += new System.EventHandler(this.RowUpButton_Click);
         // 
         // RowDownButton
         // 
         this.RowDownButton.Location = new System.Drawing.Point(571, 528);
         this.RowDownButton.Name = "RowDownButton";
         this.RowDownButton.Size = new System.Drawing.Size(75, 23);
         this.RowDownButton.TabIndex = 12;
         this.RowDownButton.Text = "Row Down";
         this.RowDownButton.UseVisualStyleBackColor = true;
         this.RowDownButton.Click += new System.EventHandler(this.RowDownButton_Click);
         // 
         // ImportColumnButton
         // 
         this.ImportColumnButton.Location = new System.Drawing.Point(705, 528);
         this.ImportColumnButton.Name = "ImportColumnButton";
         this.ImportColumnButton.Size = new System.Drawing.Size(98, 23);
         this.ImportColumnButton.TabIndex = 13;
         this.ImportColumnButton.Text = "Import Column";
         this.ImportColumnButton.UseVisualStyleBackColor = true;
         this.ImportColumnButton.Click += new System.EventHandler(this.ImportColumnButton_Click);
         // 
         // TriggerDataFileEditor
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.ImportColumnButton);
         this.Controls.Add(this.RowDownButton);
         this.Controls.Add(this.RowUpButton);
         this.Controls.Add(this.DeleteRowButton);
         this.Controls.Add(this.TableTypeLabel);
         this.Controls.Add(this.TableNameLabel);
         this.Controls.Add(this.TableListBox);
         this.Controls.Add(this.AddNewTableButton);
         this.Controls.Add(this.SaveAsButton);
         this.Controls.Add(this.SaveButton);
         this.Controls.Add(this.LoadButton);
         this.Controls.Add(this.AddRowButton);
         this.Controls.Add(this.hybridGrid1);
         this.Name = "TriggerDataFileEditor";
         this.Size = new System.Drawing.Size(1101, 591);
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private EditorCore.Controls.Micro.HybridGrid hybridGrid1;
      private System.Windows.Forms.Button AddRowButton;
      private System.Windows.Forms.Button LoadButton;
      private System.Windows.Forms.Button SaveButton;
      private System.Windows.Forms.Button SaveAsButton;
      private System.Windows.Forms.Button AddNewTableButton;
      private System.Windows.Forms.ListBox TableListBox;
      private System.Windows.Forms.Label TableNameLabel;
      private System.Windows.Forms.Label TableTypeLabel;
      private System.Windows.Forms.Button DeleteRowButton;
      private System.Windows.Forms.Button RowUpButton;
      private System.Windows.Forms.Button RowDownButton;
      private System.Windows.Forms.Button ImportColumnButton;
   }
}
