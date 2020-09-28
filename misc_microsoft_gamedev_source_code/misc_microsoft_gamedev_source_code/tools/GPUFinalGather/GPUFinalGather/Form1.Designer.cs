namespace GPUFinalGather
{
   partial class Form1
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
         this.components = new System.ComponentModel.Container();
         this.d3dPanel = new System.Windows.Forms.Panel();
         this.timer1 = new System.Windows.Forms.Timer(this.components);
         this.SuspendLayout();
         // 
         // d3dPanel
         // 
         this.d3dPanel.Location = new System.Drawing.Point(12, 12);
         this.d3dPanel.Name = "d3dPanel";
         this.d3dPanel.Size = new System.Drawing.Size(256, 256);
         this.d3dPanel.TabIndex = 0;
         // 
         // timer1
         // 
         this.timer1.Tick += new System.EventHandler(this.timer1_Tick);
         // 
         // Form1
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.ClientSize = new System.Drawing.Size(276, 279);
         this.Controls.Add(this.d3dPanel);
         this.Name = "Form1";
         this.Text = "GPU Final Gathering";
         this.ResumeLayout(false);

      }

      #endregion

      private System.Windows.Forms.Panel d3dPanel;
      private System.Windows.Forms.Timer timer1;
   }
}

