namespace PhoenixEditor.ClientTabPages
{
   partial class HintDatabasePage
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
         this.ConceptsListbox = new System.Windows.Forms.ListBox();
         this.NewConceptButton = new System.Windows.Forms.Button();
         this.DeleteConceptButton = new System.Windows.Forms.Button();
         this.propertyList1 = new EditorCore.Controls.PropertyList();
         this.SaveButton = new System.Windows.Forms.Button();
         this.SuspendLayout();
         // 
         // ConceptsListbox
         // 
         this.ConceptsListbox.FormattingEnabled = true;
         this.ConceptsListbox.Location = new System.Drawing.Point(13, 39);
         this.ConceptsListbox.Name = "ConceptsListbox";
         this.ConceptsListbox.Size = new System.Drawing.Size(191, 394);
         this.ConceptsListbox.TabIndex = 0;
         this.ConceptsListbox.MouseClick += new System.Windows.Forms.MouseEventHandler(this.ConceptsListbox_MouseClick);
         // 
         // NewConceptButton
         // 
         this.NewConceptButton.Location = new System.Drawing.Point(13, 439);
         this.NewConceptButton.Name = "NewConceptButton";
         this.NewConceptButton.Size = new System.Drawing.Size(101, 23);
         this.NewConceptButton.TabIndex = 1;
         this.NewConceptButton.Text = "New Concept";
         this.NewConceptButton.UseVisualStyleBackColor = true;
         this.NewConceptButton.Click += new System.EventHandler(this.NewConceptButton_Click);
         // 
         // DeleteConceptButton
         // 
         this.DeleteConceptButton.Location = new System.Drawing.Point(13, 468);
         this.DeleteConceptButton.Name = "DeleteConceptButton";
         this.DeleteConceptButton.Size = new System.Drawing.Size(101, 23);
         this.DeleteConceptButton.TabIndex = 2;
         this.DeleteConceptButton.Text = "Delete Concept";
         this.DeleteConceptButton.UseVisualStyleBackColor = true;
         this.DeleteConceptButton.Click += new System.EventHandler(this.DeleteConceptButton_Click);
         // 
         // propertyList1
         // 
         this.propertyList1.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)));
         this.propertyList1.AutoScroll = true;
         this.propertyList1.Location = new System.Drawing.Point(222, 39);
         this.propertyList1.Name = "propertyList1";
         this.propertyList1.Size = new System.Drawing.Size(548, 545);
         this.propertyList1.TabIndex = 3;
         // 
         // SaveButton
         // 
         this.SaveButton.Location = new System.Drawing.Point(13, 561);
         this.SaveButton.Name = "SaveButton";
         this.SaveButton.Size = new System.Drawing.Size(75, 23);
         this.SaveButton.TabIndex = 4;
         this.SaveButton.Text = "Save";
         this.SaveButton.UseVisualStyleBackColor = true;
         this.SaveButton.Click += new System.EventHandler(this.SaveButton_Click);
         // 
         // HintDatabasePage
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.SaveButton);
         this.Controls.Add(this.propertyList1);
         this.Controls.Add(this.DeleteConceptButton);
         this.Controls.Add(this.NewConceptButton);
         this.Controls.Add(this.ConceptsListbox);
         this.Name = "HintDatabasePage";
         this.Size = new System.Drawing.Size(1164, 617);
         this.ResumeLayout(false);

      }

      #endregion

      private System.Windows.Forms.ListBox ConceptsListbox;
      private System.Windows.Forms.Button NewConceptButton;
      private System.Windows.Forms.Button DeleteConceptButton;
      private EditorCore.Controls.PropertyList propertyList1;
      private System.Windows.Forms.Button SaveButton;
   }
}
