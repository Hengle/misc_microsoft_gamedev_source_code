namespace RemoteMemory
{
   partial class virtualMemLayout
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
         this.headerLabel = new System.Windows.Forms.Label();
         this.mVirtual4KRange = new RemoteMemory.MemBitmap();
         this.mVirtual64KRange = new RemoteMemory.MemBitmap();
         this.mImage64KRange = new RemoteMemory.MemBitmap();
         this.mEncrypted64KRange = new RemoteMemory.MemBitmap();
         this.mImage4KRange = new RemoteMemory.MemBitmap();
         this.Physical64KRange = new RemoteMemory.MemBitmap();
         this.mPhysical16MBRange = new RemoteMemory.MemBitmap();
         this.Physical4KRange = new RemoteMemory.MemBitmap();
         this.SuspendLayout();
         // 
         // headerLabel
         // 
         this.headerLabel.AutoSize = true;
         this.headerLabel.Font = new System.Drawing.Font("Arial", 12F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
         this.headerLabel.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(220)))), ((int)(((byte)(220)))), ((int)(((byte)(204)))));
         this.headerLabel.Location = new System.Drawing.Point(3, 0);
         this.headerLabel.Name = "headerLabel";
         this.headerLabel.Size = new System.Drawing.Size(164, 19);
         this.headerLabel.TabIndex = 8;
         this.headerLabel.Text = "Virtual Memory View";
         // 
         // mVirtual4KRange
         // 
         this.mVirtual4KRange.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(63)))), ((int)(((byte)(63)))), ((int)(((byte)(63)))));
         this.mVirtual4KRange.Location = new System.Drawing.Point(0, 22);
         this.mVirtual4KRange.Name = "mVirtual4KRange";
         this.mVirtual4KRange.Size = new System.Drawing.Size(522, 123);
         this.mVirtual4KRange.TabIndex = 0;
         // 
         // mVirtual64KRange
         // 
         this.mVirtual64KRange.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(63)))), ((int)(((byte)(63)))), ((int)(((byte)(63)))));
         this.mVirtual64KRange.Location = new System.Drawing.Point(0, 168);
         this.mVirtual64KRange.Name = "mVirtual64KRange";
         this.mVirtual64KRange.Size = new System.Drawing.Size(522, 123);
         this.mVirtual64KRange.TabIndex = 1;
         // 
         // mImage64KRange
         // 
         this.mImage64KRange.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(63)))), ((int)(((byte)(63)))), ((int)(((byte)(63)))));
         this.mImage64KRange.Location = new System.Drawing.Point(0, 297);
         this.mImage64KRange.Name = "mImage64KRange";
         this.mImage64KRange.Size = new System.Drawing.Size(522, 61);
         this.mImage64KRange.TabIndex = 2;
         // 
         // mEncrypted64KRange
         // 
         this.mEncrypted64KRange.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(63)))), ((int)(((byte)(63)))), ((int)(((byte)(63)))));
         this.mEncrypted64KRange.Location = new System.Drawing.Point(0, 364);
         this.mEncrypted64KRange.Name = "mEncrypted64KRange";
         this.mEncrypted64KRange.Size = new System.Drawing.Size(522, 29);
         this.mEncrypted64KRange.TabIndex = 3;
         // 
         // mImage4KRange
         // 
         this.mImage4KRange.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(63)))), ((int)(((byte)(63)))), ((int)(((byte)(63)))));
         this.mImage4KRange.Location = new System.Drawing.Point(0, 399);
         this.mImage4KRange.Name = "mImage4KRange";
         this.mImage4KRange.Size = new System.Drawing.Size(522, 46);
         this.mImage4KRange.TabIndex = 4;
         // 
         // Physical64KRange
         // 
         this.Physical64KRange.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(63)))), ((int)(((byte)(63)))), ((int)(((byte)(63)))));
         this.Physical64KRange.Location = new System.Drawing.Point(0, 451);
         this.Physical64KRange.Name = "Physical64KRange";
         this.Physical64KRange.Size = new System.Drawing.Size(522, 97);
         this.Physical64KRange.TabIndex = 5;
         // 
         // mPhysical16MBRange
         // 
         this.mPhysical16MBRange.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(63)))), ((int)(((byte)(63)))), ((int)(((byte)(63)))));
         this.mPhysical16MBRange.Location = new System.Drawing.Point(0, 554);
         this.mPhysical16MBRange.Name = "mPhysical16MBRange";
         this.mPhysical16MBRange.Size = new System.Drawing.Size(522, 123);
         this.mPhysical16MBRange.TabIndex = 6;
         // 
         // Physical4KRange
         // 
         this.Physical4KRange.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(63)))), ((int)(((byte)(63)))), ((int)(((byte)(63)))));
         this.Physical4KRange.Location = new System.Drawing.Point(0, 683);
         this.Physical4KRange.Name = "Physical4KRange";
         this.Physical4KRange.Size = new System.Drawing.Size(522, 123);
         this.Physical4KRange.TabIndex = 7;
         // 
         // virtualMemLayout
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(43)))), ((int)(((byte)(43)))), ((int)(((byte)(43)))));
         this.Controls.Add(this.headerLabel);
         this.Controls.Add(this.mVirtual4KRange);
         this.Controls.Add(this.mVirtual64KRange);
         this.Controls.Add(this.mImage64KRange);
         this.Controls.Add(this.mEncrypted64KRange);
         this.Controls.Add(this.mImage4KRange);
         this.Controls.Add(this.Physical64KRange);
         this.Controls.Add(this.mPhysical16MBRange);
         this.Controls.Add(this.Physical4KRange);
         this.Name = "virtualMemLayout";
         this.Size = new System.Drawing.Size(899, 1037);
         this.ResumeLayout(false);
         this.PerformLayout();

      }


      MemBitmap mVirtual4KRange;
      MemBitmap mVirtual64KRange;
      MemBitmap mImage64KRange;
      MemBitmap mEncrypted64KRange;
      MemBitmap mImage4KRange;
      MemBitmap Physical64KRange;
      MemBitmap mPhysical16MBRange;
      MemBitmap Physical4KRange;
      #endregion
      private System.Windows.Forms.Label headerLabel;


   }
}
