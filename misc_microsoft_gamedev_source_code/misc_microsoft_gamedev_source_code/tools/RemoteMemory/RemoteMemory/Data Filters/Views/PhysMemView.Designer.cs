namespace RemoteMemory
{
   partial class PhysMemView
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
         this.mPhysicalMemoryRange = new RemoteMemory.MemBitmap();
         this.SuspendLayout();
         // 
         // headerLabel
         // 
         this.headerLabel.AutoSize = true;
         this.headerLabel.Font = new System.Drawing.Font("Arial", 12F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
         this.headerLabel.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(220)))), ((int)(((byte)(220)))), ((int)(((byte)(204)))));
         this.headerLabel.Location = new System.Drawing.Point(3, 0);
         this.headerLabel.Name = "headerLabel";
         this.headerLabel.Size = new System.Drawing.Size(180, 19);
         this.headerLabel.TabIndex = 9;
         this.headerLabel.Text = "Physical Memory View";
         // 
         // mPhysicalMemoryRange
         // 
         this.mPhysicalMemoryRange.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(63)))), ((int)(((byte)(63)))), ((int)(((byte)(63)))));
         this.mPhysicalMemoryRange.Location = new System.Drawing.Point(0, 45);
         this.mPhysicalMemoryRange.Name = "mPhysicalMemoryRange";
         this.mPhysicalMemoryRange.Size = new System.Drawing.Size(460, 780);
         this.mPhysicalMemoryRange.TabIndex = 10;
         // 
         // PhysMemView
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(43)))), ((int)(((byte)(43)))), ((int)(((byte)(43)))));
         this.Controls.Add(this.mPhysicalMemoryRange);
         this.Controls.Add(this.headerLabel);
         this.Name = "PhysMemView";
         this.Size = new System.Drawing.Size(468, 856);
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private System.Windows.Forms.Label headerLabel;
      private MemBitmap mPhysicalMemoryRange;
   }
}
