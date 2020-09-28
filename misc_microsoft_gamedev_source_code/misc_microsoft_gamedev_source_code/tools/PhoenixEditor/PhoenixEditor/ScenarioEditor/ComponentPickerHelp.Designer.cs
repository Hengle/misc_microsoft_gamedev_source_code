namespace PhoenixEditor.ScenarioEditor
{
   partial class ComponentPickerHelp
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
         this.helpLabel = new System.Windows.Forms.Label();
         this.documentationLabel = new System.Windows.Forms.Label();
         this.SuspendLayout();
         // 
         // helpLabel
         // 
         this.helpLabel.AutoSize = true;
         this.helpLabel.Location = new System.Drawing.Point(3, 15);
         this.helpLabel.Name = "helpLabel";
         this.helpLabel.Size = new System.Drawing.Size(54, 13);
         this.helpLabel.TabIndex = 0;
         this.helpLabel.Text = "Help label";
         // 
         // documentationLabel
         // 
         this.documentationLabel.AutoSize = true;
         this.documentationLabel.Location = new System.Drawing.Point(3, 96);
         this.documentationLabel.Name = "documentationLabel";
         this.documentationLabel.Size = new System.Drawing.Size(35, 13);
         this.documentationLabel.TabIndex = 1;
         this.documentationLabel.Text = "label2";
         // 
         // ComponentPickerHelp
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.documentationLabel);
         this.Controls.Add(this.helpLabel);
         this.Name = "ComponentPickerHelp";
         this.Size = new System.Drawing.Size(409, 307);
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private System.Windows.Forms.Label helpLabel;
      private System.Windows.Forms.Label documentationLabel;
   }
}
