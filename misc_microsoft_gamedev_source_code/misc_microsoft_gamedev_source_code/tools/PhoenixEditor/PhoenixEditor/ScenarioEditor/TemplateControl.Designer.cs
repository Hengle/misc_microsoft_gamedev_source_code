namespace PhoenixEditor.ScenarioEditor
{
   partial class TemplateControl
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
         this.panel1 = new System.Windows.Forms.Panel();
         this.TriggerNameText = new System.Windows.Forms.Label();
         this.OutHardPointsBar = new PhoenixEditor.ScenarioEditor.HardPointsBar();
         this.INHardPointsBar = new PhoenixEditor.ScenarioEditor.HardPointsBar();
         this.VarOutHardPointsBar = new PhoenixEditor.ScenarioEditor.HardPointsBar();
         this.VarInHardPointsBar = new PhoenixEditor.ScenarioEditor.HardPointsBar();
         this.panel1.SuspendLayout();
         this.SuspendLayout();
         // 
         // panel1
         // 
         this.panel1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.panel1.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
         this.panel1.Controls.Add(this.TriggerNameText);
         this.panel1.Location = new System.Drawing.Point(17, 16);
         this.panel1.Margin = new System.Windows.Forms.Padding(0);
         this.panel1.Name = "panel1";
         this.panel1.Size = new System.Drawing.Size(96, 79);
         this.panel1.TabIndex = 0;
         this.panel1.Paint += new System.Windows.Forms.PaintEventHandler(this.panel1_Paint);
         // 
         // TriggerNameText
         // 
         this.TriggerNameText.AutoSize = true;
         this.TriggerNameText.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
         this.TriggerNameText.Location = new System.Drawing.Point(0, 3);
         this.TriggerNameText.Margin = new System.Windows.Forms.Padding(0, 3, 0, 0);
         this.TriggerNameText.Name = "TriggerNameText";
         this.TriggerNameText.Size = new System.Drawing.Size(37, 13);
         this.TriggerNameText.TabIndex = 0;
         this.TriggerNameText.Text = "name";
         // 
         // OutHardPointsBar
         // 
         this.OutHardPointsBar.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.OutHardPointsBar.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
         this.OutHardPointsBar.HorizontalLayout = false;
         this.OutHardPointsBar.Location = new System.Drawing.Point(113, 16);
         this.OutHardPointsBar.Margin = new System.Windows.Forms.Padding(0);
         this.OutHardPointsBar.MarginSize = 25;
         this.OutHardPointsBar.Name = "OutHardPointsBar";
         this.OutHardPointsBar.Size = new System.Drawing.Size(16, 79);
         this.OutHardPointsBar.TabIndex = 2;
         this.OutHardPointsBar.Load += new System.EventHandler(this.OutHardPointsBar_Load);
         // 
         // INHardPointsBar
         // 
         this.INHardPointsBar.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)));
         this.INHardPointsBar.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
         this.INHardPointsBar.HorizontalLayout = false;
         this.INHardPointsBar.Location = new System.Drawing.Point(0, 16);
         this.INHardPointsBar.Margin = new System.Windows.Forms.Padding(0);
         this.INHardPointsBar.MarginSize = 25;
         this.INHardPointsBar.Name = "INHardPointsBar";
         this.INHardPointsBar.Size = new System.Drawing.Size(17, 78);
         this.INHardPointsBar.TabIndex = 3;
         // 
         // VarOutHardPointsBar
         // 
         this.VarOutHardPointsBar.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.VarOutHardPointsBar.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
         this.VarOutHardPointsBar.HorizontalLayout = true;
         this.VarOutHardPointsBar.Location = new System.Drawing.Point(0, 95);
         this.VarOutHardPointsBar.Margin = new System.Windows.Forms.Padding(0);
         this.VarOutHardPointsBar.MarginSize = 25;
         this.VarOutHardPointsBar.Name = "VarOutHardPointsBar";
         this.VarOutHardPointsBar.Size = new System.Drawing.Size(129, 16);
         this.VarOutHardPointsBar.TabIndex = 4;
         // 
         // VarInHardPointsBar
         // 
         this.VarInHardPointsBar.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.VarInHardPointsBar.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
         this.VarInHardPointsBar.HorizontalLayout = true;
         this.VarInHardPointsBar.Location = new System.Drawing.Point(0, 0);
         this.VarInHardPointsBar.Margin = new System.Windows.Forms.Padding(0);
         this.VarInHardPointsBar.MarginSize = 25;
         this.VarInHardPointsBar.Name = "VarInHardPointsBar";
         this.VarInHardPointsBar.Size = new System.Drawing.Size(129, 16);
         this.VarInHardPointsBar.TabIndex = 5;
         // 
         // TemplateControl
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.Controls.Add(this.VarInHardPointsBar);
         this.Controls.Add(this.VarOutHardPointsBar);
         this.Controls.Add(this.INHardPointsBar);
         this.Controls.Add(this.OutHardPointsBar);
         this.Controls.Add(this.panel1);
         this.Name = "TemplateControl";
         this.Size = new System.Drawing.Size(129, 111);
         this.panel1.ResumeLayout(false);
         this.panel1.PerformLayout();
         this.ResumeLayout(false);

      }

      #endregion

      private System.Windows.Forms.Panel panel1;
      private System.Windows.Forms.Label TriggerNameText;
      private HardPointsBar OutHardPointsBar;
      private HardPointsBar INHardPointsBar;
      private HardPointsBar VarOutHardPointsBar;
      private HardPointsBar VarInHardPointsBar;
   }
}
