namespace PhoenixEditor.ScenarioEditor
{
   partial class ActiveLabel
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
         this.TextLabel = new System.Windows.Forms.Label();
         this.SuspendLayout();
         // 
         // TextLabel
         // 
         this.TextLabel.AutoSize = true;
         this.TextLabel.Dock = System.Windows.Forms.DockStyle.Fill;
         this.TextLabel.Location = new System.Drawing.Point(0, 0);
         this.TextLabel.Name = "TextLabel";
         this.TextLabel.Size = new System.Drawing.Size(54, 13);
         this.TextLabel.TabIndex = 1;
         this.TextLabel.Text = "TextLabel";
         this.TextLabel.MouseLeave += new System.EventHandler(this.ActiveDragLablel_MouseLeave);
         this.TextLabel.Click += new System.EventHandler(this.TextLabel_Click);
         this.TextLabel.MouseEnter += new System.EventHandler(this.ActiveDragLablel_MouseEnter);
         // 
         // ActiveLabel
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.TextLabel);
         this.Name = "ActiveLabel";
         this.Size = new System.Drawing.Size(95, 30);
         this.MouseEnter += new System.EventHandler(this.ActiveDragLablel_MouseEnter);
         this.MouseLeave += new System.EventHandler(this.ActiveDragLablel_MouseLeave);
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private System.Windows.Forms.Label TextLabel;
   }
}
