namespace EditorCore
{
   partial class SearchPanel
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
         this.guestControlPanel = new System.Windows.Forms.Panel();
         this.SearchButton = new System.Windows.Forms.Button();
         this.SuspendLayout();
         // 
         // guestControlPanel
         // 
         this.guestControlPanel.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.guestControlPanel.Location = new System.Drawing.Point(31, 0);
         this.guestControlPanel.Margin = new System.Windows.Forms.Padding(0);
         this.guestControlPanel.Name = "guestControlPanel";
         this.guestControlPanel.Size = new System.Drawing.Size(379, 31);
         this.guestControlPanel.TabIndex = 1;
         // 
         // SearchButton
         // 
         this.SearchButton.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)));
         this.SearchButton.BackgroundImageLayout = System.Windows.Forms.ImageLayout.Zoom;
         this.SearchButton.Location = new System.Drawing.Point(1, 1);
         this.SearchButton.Margin = new System.Windows.Forms.Padding(1);
         this.SearchButton.Name = "SearchButton";
         this.SearchButton.Size = new System.Drawing.Size(30, 30);
         this.SearchButton.TabIndex = 2;
         this.SearchButton.UseVisualStyleBackColor = true;
         this.SearchButton.Click += new System.EventHandler(this.SearchButton_Click_1);
         // 
         // SearchPanel
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.SearchButton);
         this.Controls.Add(this.guestControlPanel);
         this.Name = "SearchPanel";
         this.Size = new System.Drawing.Size(410, 31);
         this.ResumeLayout(false);

      }

      #endregion

      private System.Windows.Forms.Panel guestControlPanel;
      private System.Windows.Forms.Button SearchButton;
   }
}
