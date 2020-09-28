namespace VisualEditor.Dialogs
{
   partial class OpacityProgressionDialog
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
         this.scalarProgressionControl = new EditorCore.ScalarProgressionControl();
         this.OKButton = new System.Windows.Forms.Button();
         this.CancelButton = new System.Windows.Forms.Button();
         this.SuspendLayout();
         // 
         // scalarProgressionControl
         // 
         this.scalarProgressionControl.AxisMaxX = 100;
         this.scalarProgressionControl.AxisMaxY = 100;
         this.scalarProgressionControl.AxisMinX = 0;
         this.scalarProgressionControl.AxisMinY = -100;
         this.scalarProgressionControl.ChartEndColor = System.Drawing.Color.Yellow;
         this.scalarProgressionControl.ChartStartColor = System.Drawing.Color.Red;
         this.scalarProgressionControl.DefaultValue0 = -100;
         this.scalarProgressionControl.DefaultValue1 = 100;
         this.scalarProgressionControl.Location = new System.Drawing.Point(12, 12);
         this.scalarProgressionControl.LoopControl = false;
         this.scalarProgressionControl.Name = "scalarProgressionControl";
         this.scalarProgressionControl.ProgressionName = null;
         this.scalarProgressionControl.Size = new System.Drawing.Size(957, 204);
         this.scalarProgressionControl.TabIndex = 2;
         // 
         // OKButton
         // 
         this.OKButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
         this.OKButton.DialogResult = System.Windows.Forms.DialogResult.OK;
         this.OKButton.Location = new System.Drawing.Point(812, 229);
         this.OKButton.Name = "OKButton";
         this.OKButton.Size = new System.Drawing.Size(78, 23);
         this.OKButton.TabIndex = 0;
         this.OKButton.Text = "OK";
         this.OKButton.UseVisualStyleBackColor = true;
         // 
         // CancelButton
         // 
         this.CancelButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
         this.CancelButton.DialogResult = System.Windows.Forms.DialogResult.Cancel;
         this.CancelButton.Location = new System.Drawing.Point(896, 229);
         this.CancelButton.Name = "CancelButton";
         this.CancelButton.Size = new System.Drawing.Size(74, 23);
         this.CancelButton.TabIndex = 1;
         this.CancelButton.Text = "Cancel";
         this.CancelButton.UseVisualStyleBackColor = true;
         // 
         // OpacityProgressionDialog
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.ClientSize = new System.Drawing.Size(982, 264);
         this.Controls.Add(this.CancelButton);
         this.Controls.Add(this.OKButton);
         this.Controls.Add(this.scalarProgressionControl);
         this.MaximumSize = new System.Drawing.Size(990, 298);
         this.MinimumSize = new System.Drawing.Size(990, 298);
         this.Name = "OpacityProgressionDialog";
         this.Text = "Animation Opacity Progression Dialog";
         this.ResumeLayout(false);

      }

      #endregion

      private EditorCore.ScalarProgressionControl scalarProgressionControl;
      private System.Windows.Forms.Button OKButton;
      private System.Windows.Forms.Button CancelButton;
   }
}