namespace PhoenixEditor.ClientTabPages
{
   partial class ScenarioDataPage
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
         this.basicTypedSuperList1 = new PhoenixEditor.ScenarioEditor.BasicTypedSuperList();
         this.CheckoutButton = new System.Windows.Forms.Button();
         this.SaveButton = new System.Windows.Forms.Button();
         this.CheckInButton = new System.Windows.Forms.Button();
         this.AutoAddButton = new System.Windows.Forms.Button();
         this.AutoRemoveButton = new System.Windows.Forms.Button();
         this.SuspendLayout();
         // 
         // basicTypedSuperList1
         // 
         this.basicTypedSuperList1.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)));
         this.basicTypedSuperList1.Enabled = false;
         this.basicTypedSuperList1.GridColor = System.Drawing.SystemColors.ActiveCaption;
         this.basicTypedSuperList1.Location = new System.Drawing.Point(13, 12);
         this.basicTypedSuperList1.Name = "basicTypedSuperList1";
         this.basicTypedSuperList1.Size = new System.Drawing.Size(1227, 395);
         this.basicTypedSuperList1.TabIndex = 0;
         this.basicTypedSuperList1.UseLabels = true;
         this.basicTypedSuperList1.WrapContents = false;
         this.basicTypedSuperList1.Load += new System.EventHandler(this.basicTypedSuperList1_Load);
         // 
         // CheckoutButton
         // 
         this.CheckoutButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
         this.CheckoutButton.Location = new System.Drawing.Point(13, 413);
         this.CheckoutButton.Name = "CheckoutButton";
         this.CheckoutButton.Size = new System.Drawing.Size(75, 23);
         this.CheckoutButton.TabIndex = 1;
         this.CheckoutButton.Text = "Check Out";
         this.CheckoutButton.UseVisualStyleBackColor = true;
         this.CheckoutButton.Click += new System.EventHandler(this.CheckoutButton_Click);
         // 
         // SaveButton
         // 
         this.SaveButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
         this.SaveButton.Location = new System.Drawing.Point(94, 414);
         this.SaveButton.Name = "SaveButton";
         this.SaveButton.Size = new System.Drawing.Size(75, 23);
         this.SaveButton.TabIndex = 2;
         this.SaveButton.Text = "Save";
         this.SaveButton.UseVisualStyleBackColor = true;
         this.SaveButton.Click += new System.EventHandler(this.SaveButton_Click);
         // 
         // CheckInButton
         // 
         this.CheckInButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
         this.CheckInButton.Location = new System.Drawing.Point(175, 413);
         this.CheckInButton.Name = "CheckInButton";
         this.CheckInButton.Size = new System.Drawing.Size(75, 23);
         this.CheckInButton.TabIndex = 3;
         this.CheckInButton.Text = "Check In";
         this.CheckInButton.UseVisualStyleBackColor = true;
         this.CheckInButton.Click += new System.EventHandler(this.CheckInButton_Click);
         // 
         // AutoAddButton
         // 
         this.AutoAddButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
         this.AutoAddButton.Location = new System.Drawing.Point(451, 414);
         this.AutoAddButton.Name = "AutoAddButton";
         this.AutoAddButton.Size = new System.Drawing.Size(117, 23);
         this.AutoAddButton.TabIndex = 4;
         this.AutoAddButton.Text = "Auto-Add New";
         this.AutoAddButton.UseVisualStyleBackColor = true;
         this.AutoAddButton.Click += new System.EventHandler(this.AutoAddButton_Click);
         // 
         // AutoRemoveButton
         // 
         this.AutoRemoveButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
         this.AutoRemoveButton.Enabled = false;
         this.AutoRemoveButton.Location = new System.Drawing.Point(591, 413);
         this.AutoRemoveButton.Name = "AutoRemoveButton";
         this.AutoRemoveButton.Size = new System.Drawing.Size(140, 23);
         this.AutoRemoveButton.TabIndex = 5;
         this.AutoRemoveButton.Text = "Auto-Remove Missing";
         this.AutoRemoveButton.UseVisualStyleBackColor = true;
         this.AutoRemoveButton.Click += new System.EventHandler(this.AutoRemoveButton_Click);
         // 
         // ScenarioDataPage
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.Controls.Add(this.AutoRemoveButton);
         this.Controls.Add(this.AutoAddButton);
         this.Controls.Add(this.CheckInButton);
         this.Controls.Add(this.SaveButton);
         this.Controls.Add(this.CheckoutButton);
         this.Controls.Add(this.basicTypedSuperList1);
         this.Name = "ScenarioDataPage";
         this.Size = new System.Drawing.Size(1256, 477);
         this.ResumeLayout(false);

      }

      #endregion

      private PhoenixEditor.ScenarioEditor.BasicTypedSuperList basicTypedSuperList1;
      private System.Windows.Forms.Button CheckoutButton;
      private System.Windows.Forms.Button SaveButton;
      private System.Windows.Forms.Button CheckInButton;
      private System.Windows.Forms.Button AutoAddButton;
      private System.Windows.Forms.Button AutoRemoveButton;
   }
}
