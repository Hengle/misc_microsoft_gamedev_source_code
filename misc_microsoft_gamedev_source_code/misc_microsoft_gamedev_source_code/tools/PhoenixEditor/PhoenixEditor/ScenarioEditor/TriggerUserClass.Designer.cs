namespace PhoenixEditor.ScenarioEditor
{
   partial class TriggerUserClass
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
         this.ClassPropsGrid = new EditorCore.BetterPropertyGrid();
         this.UserClassList = new System.Windows.Forms.TreeView();
         this.NewClassButton = new System.Windows.Forms.Button();
         this.ClassFieldsList = new PhoenixEditor.ScenarioEditor.BasicTypedSuperList();
         this.SaveButton = new System.Windows.Forms.Button();
         this.SuspendLayout();
         // 
         // ClassPropsGrid
         // 
         this.ClassPropsGrid.LastRowHack = false;
         this.ClassPropsGrid.Location = new System.Drawing.Point(277, 15);
         this.ClassPropsGrid.Name = "ClassPropsGrid";
         this.ClassPropsGrid.Size = new System.Drawing.Size(395, 221);
         this.ClassPropsGrid.TabIndex = 0;
         // 
         // UserClassList
         // 
         this.UserClassList.Location = new System.Drawing.Point(14, 15);
         this.UserClassList.Name = "UserClassList";
         this.UserClassList.Size = new System.Drawing.Size(222, 486);
         this.UserClassList.TabIndex = 1;
         this.UserClassList.NodeMouseClick += new System.Windows.Forms.TreeNodeMouseClickEventHandler(this.UserClassList_NodeMouseClick);
         // 
         // NewClassButton
         // 
         this.NewClassButton.Location = new System.Drawing.Point(14, 507);
         this.NewClassButton.Name = "NewClassButton";
         this.NewClassButton.Size = new System.Drawing.Size(75, 23);
         this.NewClassButton.TabIndex = 3;
         this.NewClassButton.Text = "New Class";
         this.NewClassButton.UseVisualStyleBackColor = true;
         this.NewClassButton.Click += new System.EventHandler(this.NewClassButton_Click);
         // 
         // ClassFieldsList
         // 
         this.ClassFieldsList.GridColor = System.Drawing.SystemColors.ActiveCaption;
         this.ClassFieldsList.Location = new System.Drawing.Point(277, 242);
         this.ClassFieldsList.Name = "ClassFieldsList";
         this.ClassFieldsList.Size = new System.Drawing.Size(395, 344);
         this.ClassFieldsList.TabIndex = 2;
         this.ClassFieldsList.UseLabels = true;
         this.ClassFieldsList.WrapContents = false;
         // 
         // SaveButton
         // 
         this.SaveButton.Location = new System.Drawing.Point(68, 563);
         this.SaveButton.Name = "SaveButton";
         this.SaveButton.Size = new System.Drawing.Size(75, 23);
         this.SaveButton.TabIndex = 4;
         this.SaveButton.Text = "Save";
         this.SaveButton.UseVisualStyleBackColor = true;
         this.SaveButton.Click += new System.EventHandler(this.SaveButton_Click);
         // 
         // TriggerUserClass
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.SaveButton);
         this.Controls.Add(this.NewClassButton);
         this.Controls.Add(this.ClassFieldsList);
         this.Controls.Add(this.UserClassList);
         this.Controls.Add(this.ClassPropsGrid);
         this.Name = "TriggerUserClass";
         this.Size = new System.Drawing.Size(692, 609);
         this.ResumeLayout(false);

      }

      #endregion

      private EditorCore.BetterPropertyGrid ClassPropsGrid;
      private System.Windows.Forms.TreeView UserClassList;
      private BasicTypedSuperList ClassFieldsList;
      private System.Windows.Forms.Button NewClassButton;
      private System.Windows.Forms.Button SaveButton;
   }
}
