namespace PhoenixEditor.ClientTabPages
{
   partial class ObjectEditorPage
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
         this.panel1 = new System.Windows.Forms.Panel();
         this.SaveButton = new System.Windows.Forms.Button();
         this.NewButton = new System.Windows.Forms.Button();
         this.ChangedObjectsListBox = new System.Windows.Forms.ListBox();
         this.ChangesListBox = new System.Windows.Forms.ListBox();
         this.ObjectNameLabel = new System.Windows.Forms.Label();
         this.splitContainer1 = new System.Windows.Forms.SplitContainer();
         this.propertyList1 = new EditorCore.Controls.PropertyList();
         this.label1 = new System.Windows.Forms.Label();
         this.ResetSelectedButton = new System.Windows.Forms.Button();
         this.splitContainer1.Panel1.SuspendLayout();
         this.splitContainer1.Panel2.SuspendLayout();
         this.splitContainer1.SuspendLayout();
         this.SuspendLayout();
         // 
         // panel1
         // 
         this.panel1.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.panel1.Location = new System.Drawing.Point(3, 3);
         this.panel1.Name = "panel1";
         this.panel1.Size = new System.Drawing.Size(427, 447);
         this.panel1.TabIndex = 0;
         // 
         // SaveButton
         // 
         this.SaveButton.Location = new System.Drawing.Point(7, 5);
         this.SaveButton.Name = "SaveButton";
         this.SaveButton.Size = new System.Drawing.Size(75, 23);
         this.SaveButton.TabIndex = 2;
         this.SaveButton.Text = "Save";
         this.SaveButton.UseVisualStyleBackColor = true;
         this.SaveButton.Click += new System.EventHandler(this.SaveButton_Click);
         // 
         // NewButton
         // 
         this.NewButton.Location = new System.Drawing.Point(88, 5);
         this.NewButton.Name = "NewButton";
         this.NewButton.Size = new System.Drawing.Size(75, 23);
         this.NewButton.TabIndex = 3;
         this.NewButton.Text = "New";
         this.NewButton.UseVisualStyleBackColor = true;
         this.NewButton.Click += new System.EventHandler(this.NewButton_Click);
         // 
         // ChangedObjectsListBox
         // 
         this.ChangedObjectsListBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)));
         this.ChangedObjectsListBox.FormattingEnabled = true;
         this.ChangedObjectsListBox.Location = new System.Drawing.Point(3, 484);
         this.ChangedObjectsListBox.Name = "ChangedObjectsListBox";
         this.ChangedObjectsListBox.Size = new System.Drawing.Size(120, 121);
         this.ChangedObjectsListBox.TabIndex = 4;
         this.ChangedObjectsListBox.SelectedIndexChanged += new System.EventHandler(this.ChangedObjectsListBox_SelectedIndexChanged);
         // 
         // ChangesListBox
         // 
         this.ChangesListBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)));
         this.ChangesListBox.FormattingEnabled = true;
         this.ChangesListBox.Location = new System.Drawing.Point(129, 510);
         this.ChangesListBox.Name = "ChangesListBox";
         this.ChangesListBox.Size = new System.Drawing.Size(361, 95);
         this.ChangesListBox.TabIndex = 5;
         // 
         // ObjectNameLabel
         // 
         this.ObjectNameLabel.AutoSize = true;
         this.ObjectNameLabel.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
         this.ObjectNameLabel.Location = new System.Drawing.Point(255, 5);
         this.ObjectNameLabel.Name = "ObjectNameLabel";
         this.ObjectNameLabel.Size = new System.Drawing.Size(194, 20);
         this.ObjectNameLabel.TabIndex = 8;
         this.ObjectNameLabel.Text = "please select an object";
         // 
         // splitContainer1
         // 
         this.splitContainer1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.splitContainer1.Location = new System.Drawing.Point(7, 34);
         this.splitContainer1.Name = "splitContainer1";
         // 
         // splitContainer1.Panel1
         // 
         this.splitContainer1.Panel1.Controls.Add(this.propertyList1);
         // 
         // splitContainer1.Panel2
         // 
         this.splitContainer1.Panel2.Controls.Add(this.label1);
         this.splitContainer1.Panel2.Controls.Add(this.ResetSelectedButton);
         this.splitContainer1.Panel2.Controls.Add(this.panel1);
         this.splitContainer1.Panel2.Controls.Add(this.ChangedObjectsListBox);
         this.splitContainer1.Panel2.Controls.Add(this.ChangesListBox);
         this.splitContainer1.Size = new System.Drawing.Size(887, 618);
         this.splitContainer1.SplitterDistance = 450;
         this.splitContainer1.TabIndex = 9;
         // 
         // propertyList1
         // 
         this.propertyList1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.propertyList1.AutoScroll = true;
         this.propertyList1.BackColor = System.Drawing.SystemColors.Control;
         this.propertyList1.Location = new System.Drawing.Point(3, 3);
         this.propertyList1.Name = "propertyList1";
         this.propertyList1.Size = new System.Drawing.Size(439, 602);
         this.propertyList1.TabIndex = 2;
         // 
         // label1
         // 
         this.label1.AutoSize = true;
         this.label1.Font = new System.Drawing.Font("Microsoft Sans Serif", 15.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
         this.label1.Location = new System.Drawing.Point(3, 456);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(160, 25);
         this.label1.TabIndex = 9;
         this.label1.Text = "Change History";
         // 
         // ResetSelectedButton
         // 
         this.ResetSelectedButton.Location = new System.Drawing.Point(184, 481);
         this.ResetSelectedButton.Name = "ResetSelectedButton";
         this.ResetSelectedButton.Size = new System.Drawing.Size(207, 23);
         this.ResetSelectedButton.TabIndex = 8;
         this.ResetSelectedButton.Text = "Reset Selected Property";
         this.ResetSelectedButton.UseVisualStyleBackColor = true;
         this.ResetSelectedButton.Click += new System.EventHandler(this.ResetSelectedButton_Click);
         // 
         // ObjectEditorPage
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.splitContainer1);
         this.Controls.Add(this.SaveButton);
         this.Controls.Add(this.NewButton);
         this.Controls.Add(this.ObjectNameLabel);
         this.Name = "ObjectEditorPage";
         this.Size = new System.Drawing.Size(897, 665);
         this.splitContainer1.Panel1.ResumeLayout(false);
         this.splitContainer1.Panel2.ResumeLayout(false);
         this.splitContainer1.Panel2.PerformLayout();
         this.splitContainer1.ResumeLayout(false);
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private System.Windows.Forms.Panel panel1;
      private System.Windows.Forms.Button SaveButton;
      private System.Windows.Forms.Button NewButton;
      private System.Windows.Forms.ListBox ChangedObjectsListBox;
      private System.Windows.Forms.ListBox ChangesListBox;
      private System.Windows.Forms.Label ObjectNameLabel;
      private System.Windows.Forms.SplitContainer splitContainer1;
      private EditorCore.Controls.PropertyList propertyList1;
      private System.Windows.Forms.Button ResetSelectedButton;
      private System.Windows.Forms.Label label1;
   }
}
