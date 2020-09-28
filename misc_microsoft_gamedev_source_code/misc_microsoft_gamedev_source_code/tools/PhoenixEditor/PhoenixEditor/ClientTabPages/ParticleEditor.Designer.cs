namespace PhoenixEditor
{
   partial class ParticleEditor
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
         this.SaveAsButton = new System.Windows.Forms.Button();
         this.d3dRenderPanel = new System.Windows.Forms.Panel();
         this.SuspendLayout();
         // 
         // SaveAsButton
         // 
         this.SaveAsButton.Location = new System.Drawing.Point(14, 13);
         this.SaveAsButton.Name = "SaveAsButton";
         this.SaveAsButton.Size = new System.Drawing.Size(75, 23);
         this.SaveAsButton.TabIndex = 0;
         this.SaveAsButton.Text = "Save As";
         this.SaveAsButton.UseVisualStyleBackColor = true;
         this.SaveAsButton.Click += new System.EventHandler(this.SaveAsButton_Click);
         // 
         // d3dRenderPanel
         // 
         this.d3dRenderPanel.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
         this.d3dRenderPanel.Location = new System.Drawing.Point(343, 109);
         this.d3dRenderPanel.Name = "d3dRenderPanel";
         this.d3dRenderPanel.Size = new System.Drawing.Size(369, 346);
         this.d3dRenderPanel.TabIndex = 1;
         // 
         // ParticleEditor
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.d3dRenderPanel);
         this.Controls.Add(this.SaveAsButton);
         this.Name = "ParticleEditor";
         this.Size = new System.Drawing.Size(817, 570);
         this.ResumeLayout(false);

      }

      #endregion

      private System.Windows.Forms.Button SaveAsButton;
      private System.Windows.Forms.Panel d3dRenderPanel;

   }
}
