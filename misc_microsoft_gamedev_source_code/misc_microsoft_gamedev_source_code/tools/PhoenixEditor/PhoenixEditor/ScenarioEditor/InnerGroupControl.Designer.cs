namespace PhoenixEditor.ScenarioEditor
{
   partial class InnerGroupControl
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
         this.GroupInputHardPointsBar = new PhoenixEditor.ScenarioEditor.HardPointsBar();
         this.TitleLabel = new System.Windows.Forms.Label();
         this.GroupOutputHardPointsBar = new PhoenixEditor.ScenarioEditor.HardPointsBar();
         this.SuspendLayout();
         // 
         // GroupInputHardPointsBar
         // 
         this.GroupInputHardPointsBar.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
         this.GroupInputHardPointsBar.HorizontalLayout = false;
         this.GroupInputHardPointsBar.Location = new System.Drawing.Point(52, 30);
         this.GroupInputHardPointsBar.MarginSize = 25;
         this.GroupInputHardPointsBar.Name = "GroupInputHardPointsBar";
         this.GroupInputHardPointsBar.Size = new System.Drawing.Size(30, 277);
         this.GroupInputHardPointsBar.TabIndex = 0;
         // 
         // TitleLabel
         // 
         this.TitleLabel.AutoSize = true;
         this.TitleLabel.Location = new System.Drawing.Point(3, 0);
         this.TitleLabel.Name = "TitleLabel";
         this.TitleLabel.Size = new System.Drawing.Size(35, 13);
         this.TitleLabel.TabIndex = 1;
         this.TitleLabel.Text = "label1";
         // 
         // GroupOutputHardPointsBar
         // 
         this.GroupOutputHardPointsBar.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
         this.GroupOutputHardPointsBar.HorizontalLayout = false;
         this.GroupOutputHardPointsBar.Location = new System.Drawing.Point(0, 30);
         this.GroupOutputHardPointsBar.MarginSize = 25;
         this.GroupOutputHardPointsBar.Name = "GroupOutputHardPointsBar";
         this.GroupOutputHardPointsBar.Size = new System.Drawing.Size(30, 277);
         this.GroupOutputHardPointsBar.TabIndex = 2;
         // 
         // InnerGroupControl
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.BackColor = System.Drawing.Color.NavajoWhite;
         this.Controls.Add(this.GroupOutputHardPointsBar);
         this.Controls.Add(this.TitleLabel);
         this.Controls.Add(this.GroupInputHardPointsBar);
         this.Name = "InnerGroupControl";
         this.Size = new System.Drawing.Size(82, 310);
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private HardPointsBar GroupInputHardPointsBar;
      private System.Windows.Forms.Label TitleLabel;
      private HardPointsBar GroupOutputHardPointsBar;
   }
}
