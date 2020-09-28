namespace PhoenixEditor.ScenarioEditor
{
   partial class TriggerScriptTools
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
         this.ScriptGroupsTreeView = new System.Windows.Forms.TreeView();
         this.SuspendLayout();
         // 
         // ScriptGroupsTreeView
         // 
         this.ScriptGroupsTreeView.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.ScriptGroupsTreeView.Location = new System.Drawing.Point(4, 3);
         this.ScriptGroupsTreeView.Name = "ScriptGroupsTreeView";
         this.ScriptGroupsTreeView.Size = new System.Drawing.Size(191, 208);
         this.ScriptGroupsTreeView.TabIndex = 0;
         // 
         // TriggerScriptTools
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.ScriptGroupsTreeView);
         this.Name = "TriggerScriptTools";
         this.Size = new System.Drawing.Size(198, 214);
         this.ResumeLayout(false);

      }

      #endregion

      private System.Windows.Forms.TreeView ScriptGroupsTreeView;
   }
}
