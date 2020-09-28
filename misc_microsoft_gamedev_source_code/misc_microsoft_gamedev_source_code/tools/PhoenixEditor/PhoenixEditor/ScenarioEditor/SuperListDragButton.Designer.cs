namespace PhoenixEditor.ScenarioEditor
{
   partial class SuperListDragButton
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
         // SuperListDragButton
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Name = "SuperListDragButton";
         this.Size = new System.Drawing.Size(10, 13);
         this.MouseMove += new System.Windows.Forms.MouseEventHandler(this.SuperListDragButton_MouseMove);
         this.DragDrop += new System.Windows.Forms.DragEventHandler(this.SuperListDragButton_DragDrop);
         this.DragEnter += new System.Windows.Forms.DragEventHandler(this.SuperListDragButton_DragEnter);
         this.MouseEnter += new System.EventHandler(this.SuperListDragButton_MouseEnter);
         this.MouseLeave += new System.EventHandler(this.SuperListDragButton_MouseLeave);
         this.MouseUp += new System.Windows.Forms.MouseEventHandler(this.SuperListDragButton_MouseUp);
         this.ResumeLayout(false);

      }

      #endregion
   }
}
