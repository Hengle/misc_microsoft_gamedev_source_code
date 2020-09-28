namespace graphapp
{
   partial class MaskGenForm
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
         this.SuspendLayout();
         // 
         // MaskGenForm
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Name = "MaskGenForm";
         this.Size = new System.Drawing.Size(354, 291);
         this.MouseDown += new System.Windows.Forms.MouseEventHandler(this.MaskGenForm_MouseDown);
         this.MouseMove += new System.Windows.Forms.MouseEventHandler(this.MaskGenForm_MouseMove);
         this.MouseDoubleClick += new System.Windows.Forms.MouseEventHandler(this.MaskGenForm_MouseDoubleClick);
         this.KeyUp += new System.Windows.Forms.KeyEventHandler(this.MaskGenForm_KeyUp);
         this.MouseUp += new System.Windows.Forms.MouseEventHandler(this.MaskGenForm_MouseUp);
         this.KeyDown += new System.Windows.Forms.KeyEventHandler(this.MaskGenForm_KeyDown);
         this.ResumeLayout(false);

      }

      #endregion

   }
}
