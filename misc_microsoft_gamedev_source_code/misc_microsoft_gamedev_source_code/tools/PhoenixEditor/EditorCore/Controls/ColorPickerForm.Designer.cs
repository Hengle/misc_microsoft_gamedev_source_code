namespace EditorCore
{
   partial class ColorPickerForm
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
         System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(ColorPickerForm));
         this.colorPanel1 = new Sano.PersonalProjects.ColorPicker.Controls.ColorPanel();
         this.OKColorButton = new System.Windows.Forms.Button();
         this.CancelColorButton = new System.Windows.Forms.Button();
         this.SuspendLayout();
         // 
         // colorPanel1
         // 
         this.colorPanel1.Location = new System.Drawing.Point(-2, -1);
         this.colorPanel1.Margin = new System.Windows.Forms.Padding(0);
         this.colorPanel1.Name = "colorPanel1";
         this.colorPanel1.Settings = ((Sano.PersonalProjects.ColorPicker.Controls.ColorPanelSettings)(resources.GetObject("colorPanel1.Settings")));
         this.colorPanel1.Size = new System.Drawing.Size(568, 272);
         this.colorPanel1.TabIndex = 0;
         this.colorPanel1.TopColorPickerChanged += new System.EventHandler(this.colorPanel1_TopColorPickerChanged);
         // 
         // OKColorButton
         // 
         this.OKColorButton.Location = new System.Drawing.Point(134, 282);
         this.OKColorButton.Name = "OKColorButton";
         this.OKColorButton.Size = new System.Drawing.Size(75, 23);
         this.OKColorButton.TabIndex = 1;
         this.OKColorButton.Text = "OK";
         this.OKColorButton.UseVisualStyleBackColor = true;
         this.OKColorButton.Click += new System.EventHandler(this.OKColorButton_Click);
         // 
         // CancelColorButton
         // 
         this.CancelColorButton.Location = new System.Drawing.Point(331, 282);
         this.CancelColorButton.Name = "CancelColorButton";
         this.CancelColorButton.Size = new System.Drawing.Size(75, 23);
         this.CancelColorButton.TabIndex = 2;
         this.CancelColorButton.Text = "Cancel";
         this.CancelColorButton.UseVisualStyleBackColor = true;
         this.CancelColorButton.Click += new System.EventHandler(this.CancelColorButton_Click);
         // 
         // ColorPickerForm
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.ClientSize = new System.Drawing.Size(579, 317);
         this.Controls.Add(this.CancelColorButton);
         this.Controls.Add(this.OKColorButton);
         this.Controls.Add(this.colorPanel1);
         this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedToolWindow;
         this.Name = "ColorPickerForm";
         this.Text = "ColorPickerForm";
         this.ResumeLayout(false);

      }

      #endregion

      private Sano.PersonalProjects.ColorPicker.Controls.ColorPanel colorPanel1;
      private System.Windows.Forms.Button OKColorButton;
      private System.Windows.Forms.Button CancelColorButton;
   }
}