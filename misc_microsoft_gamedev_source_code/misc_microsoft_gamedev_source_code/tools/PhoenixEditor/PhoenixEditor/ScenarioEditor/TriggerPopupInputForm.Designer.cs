namespace PhoenixEditor.ScenarioEditor
{
   partial class TriggerPopupInputForm
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
         this.OKButton = new System.Windows.Forms.Button();
         this.OptionsButton = new System.Windows.Forms.Button();
         this.panel1 = new System.Windows.Forms.Panel();
         this.SuspendLayout();
         // 
         // OKButton
         // 
         this.OKButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
         this.OKButton.Location = new System.Drawing.Point(190, 0);
         this.OKButton.Margin = new System.Windows.Forms.Padding(0);
         this.OKButton.Name = "OKButton";
         this.OKButton.Size = new System.Drawing.Size(30, 23);
         this.OKButton.TabIndex = 0;
         this.OKButton.Text = "OK";
         this.OKButton.UseVisualStyleBackColor = true;
         this.OKButton.Click += new System.EventHandler(this.OKButton_Click);
         // 
         // OptionsButton
         // 
         this.OptionsButton.Location = new System.Drawing.Point(-2, 0);
         this.OptionsButton.Margin = new System.Windows.Forms.Padding(0);
         this.OptionsButton.Name = "OptionsButton";
         this.OptionsButton.Size = new System.Drawing.Size(25, 23);
         this.OptionsButton.TabIndex = 1;
         this.OptionsButton.Text = "...";
         this.OptionsButton.UseVisualStyleBackColor = true;
         this.OptionsButton.Click += new System.EventHandler(this.OptionsButton_Click);
         // 
         // panel1
         // 
         this.panel1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.panel1.Location = new System.Drawing.Point(23, 0);
         this.panel1.Margin = new System.Windows.Forms.Padding(0);
         this.panel1.Name = "panel1";
         this.panel1.Size = new System.Drawing.Size(167, 23);
         this.panel1.TabIndex = 2;
         // 
         // TriggerPopupInputForm
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.ClientSize = new System.Drawing.Size(221, 23);
         this.Controls.Add(this.panel1);
         this.Controls.Add(this.OptionsButton);
         this.Controls.Add(this.OKButton);
         this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.SizableToolWindow;
         this.MaximizeBox = false;
         this.MinimizeBox = false;
         this.Name = "TriggerPopupInputForm";
         this.Text = "TriggerPopupInputForm";
         this.Deactivate += new System.EventHandler(this.TriggerPopupInputForm_Deactivate);
         this.ResumeLayout(false);

      }

      #endregion

      private System.Windows.Forms.Button OKButton;
      private System.Windows.Forms.Button OptionsButton;
      private System.Windows.Forms.Panel panel1;
   }
}