namespace xfs
{
   partial class LogMonitor
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
         this.logTabs = new System.Windows.Forms.TabControl();
         this.SuspendLayout();
         // 
         // logTabs
         // 
         this.logTabs.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.logTabs.Location = new System.Drawing.Point(3, 3);
         this.logTabs.Multiline = true;
         this.logTabs.Name = "logTabs";
         this.logTabs.SelectedIndex = 0;
         this.logTabs.Size = new System.Drawing.Size(525, 238);
         this.logTabs.TabIndex = 0;
         this.logTabs.SelectedIndexChanged += new System.EventHandler(this.logTabs_SelectedIndexChanged);
         // 
         // LogMonitor
         // 
         this.Controls.Add(this.logTabs);
         this.Name = "LogMonitor";
         this.Size = new System.Drawing.Size(531, 244);
         this.ResumeLayout(false);

      }

      #endregion

      private System.Windows.Forms.TabControl logTabs;

   }
}
