namespace PhoenixEditor.ScenarioEditor
{
   partial class FunctorEditor
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
         this.flowLayoutPanel1 = new System.Windows.Forms.FlowLayoutPanel();
         this.FunctionNameLabel = new System.Windows.Forms.Label();
         this.flowLayoutPanel1.SuspendLayout();
         this.SuspendLayout();
         // 
         // flowLayoutPanel1
         // 
         this.flowLayoutPanel1.Controls.Add(this.FunctionNameLabel);
         this.flowLayoutPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
         this.flowLayoutPanel1.Location = new System.Drawing.Point(0, 0);
         this.flowLayoutPanel1.Name = "flowLayoutPanel1";
         this.flowLayoutPanel1.Size = new System.Drawing.Size(167, 54);
         this.flowLayoutPanel1.TabIndex = 0;
         // 
         // FunctionNameLabel
         // 
         this.FunctionNameLabel.AutoSize = true;
         this.FunctionNameLabel.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
         this.FunctionNameLabel.Location = new System.Drawing.Point(0, 0);
         this.FunctionNameLabel.Margin = new System.Windows.Forms.Padding(0);
         this.FunctionNameLabel.Name = "FunctionNameLabel";
         this.FunctionNameLabel.Size = new System.Drawing.Size(85, 13);
         this.FunctionNameLabel.TabIndex = 0;
         this.FunctionNameLabel.Padding = new System.Windows.Forms.Padding(0);
         this.FunctionNameLabel.Text = "functionName";
         // 
         // FunctorEditor
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.flowLayoutPanel1);
         this.Name = "FunctorEditor";
         this.Size = new System.Drawing.Size(167, 54);
         this.flowLayoutPanel1.ResumeLayout(false);
         this.flowLayoutPanel1.PerformLayout();
         this.ResumeLayout(false);

      }

      #endregion

      private System.Windows.Forms.FlowLayoutPanel flowLayoutPanel1;
      private System.Windows.Forms.Label FunctionNameLabel;
   }
}
