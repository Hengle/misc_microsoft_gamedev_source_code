namespace RemoteMemory
{
   partial class MemViewLarge
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
         this.label1 = new System.Windows.Forms.Label();
         this.label3 = new System.Windows.Forms.Label();
         this.heapFileView = new RemoteMemory.HeapFileView();
         this.topAllocs = new RemoteMemory.TopAllocators();
         this.heapKey = new RemoteMemory.HeapKey();
         this.heapLines = new RemoteMemory.HeapLines();
         this.mFileTimelines = new RemoteMemory.FileTimelines();
         this.mFileGroupings = new RemoteMemory.FileGroupings();
         this.mErrorList = new RemoteMemory.ErrorList();
         this.button1 = new System.Windows.Forms.Button();
         this.SuspendLayout();
         // 
         // label1
         // 
         this.label1.AutoSize = true;
         this.label1.Font = new System.Drawing.Font("Arial", 12F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
         this.label1.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(220)))), ((int)(((byte)(220)))), ((int)(((byte)(204)))));
         this.label1.Location = new System.Drawing.Point(12, 9);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(126, 19);
         this.label1.TabIndex = 9;
         this.label1.Text = "Heap Summary";
         // 
         // label3
         // 
         this.label3.AutoSize = true;
         this.label3.Font = new System.Drawing.Font("Arial", 12F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
         this.label3.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(220)))), ((int)(((byte)(220)))), ((int)(((byte)(204)))));
         this.label3.Location = new System.Drawing.Point(12, 531);
         this.label3.Name = "label3";
         this.label3.Size = new System.Drawing.Size(181, 19);
         this.label3.TabIndex = 11;
         this.label3.Text = "Top Allocators (top 32)";
         // 
         // heapFileView
         // 
         this.heapFileView.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(43)))), ((int)(((byte)(43)))), ((int)(((byte)(43)))));
         this.heapFileView.Location = new System.Drawing.Point(527, 9);
         this.heapFileView.Name = "heapFileView";
         this.heapFileView.Size = new System.Drawing.Size(492, 942);
         this.heapFileView.TabIndex = 0;
         // 
         // topAllocs
         // 
         this.topAllocs.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(63)))), ((int)(((byte)(63)))), ((int)(((byte)(63)))));
         this.topAllocs.Location = new System.Drawing.Point(12, 553);
         this.topAllocs.Name = "topAllocs";
         this.topAllocs.Size = new System.Drawing.Size(488, 398);
         this.topAllocs.TabIndex = 0;
         // 
         // heapKey
         // 
         this.heapKey.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(63)))), ((int)(((byte)(63)))), ((int)(((byte)(63)))));
         this.heapKey.Location = new System.Drawing.Point(12, 31);
         this.heapKey.Name = "heapKey";
         this.heapKey.Size = new System.Drawing.Size(488, 266);
         this.heapKey.TabIndex = 0;
         // 
         // heapLines
         // 
         this.heapLines.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(43)))), ((int)(((byte)(43)))), ((int)(((byte)(43)))));
         this.heapLines.Location = new System.Drawing.Point(12, 303);
         this.heapLines.Name = "heapLines";
         this.heapLines.Size = new System.Drawing.Size(488, 225);
         this.heapLines.TabIndex = 0;
         // 
         // mFileTimelines
         // 
         this.mFileTimelines.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(43)))), ((int)(((byte)(43)))), ((int)(((byte)(43)))));
         this.mFileTimelines.Location = new System.Drawing.Point(1019, 9);
         this.mFileTimelines.Name = "mFileTimelines";
         this.mFileTimelines.Size = new System.Drawing.Size(492, 519);
         this.mFileTimelines.TabIndex = 0;
         // 
         // mFileGroupings
         // 
         this.mFileGroupings.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(43)))), ((int)(((byte)(43)))), ((int)(((byte)(43)))));
         this.mFileGroupings.Location = new System.Drawing.Point(1019, 528);
         this.mFileGroupings.Name = "mFileGroupings";
         this.mFileGroupings.Size = new System.Drawing.Size(492, 423);
         this.mFileGroupings.TabIndex = 0;
         // 
         // mErrorList
         // 
         this.mErrorList.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(43)))), ((int)(((byte)(43)))), ((int)(((byte)(43)))));
         this.mErrorList.Location = new System.Drawing.Point(12, 957);
         this.mErrorList.Name = "mErrorList";
         this.mErrorList.Size = new System.Drawing.Size(1007, 138);
         this.mErrorList.TabIndex = 0;
         // 
         // button1
         // 
         this.button1.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
         this.button1.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
         this.button1.Font = new System.Drawing.Font("Microsoft Sans Serif", 6F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
         this.button1.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(220)))), ((int)(((byte)(220)))), ((int)(((byte)(204)))));
         this.button1.Location = new System.Drawing.Point(199, 531);
         this.button1.Name = "button1";
         this.button1.Size = new System.Drawing.Size(73, 19);
         this.button1.TabIndex = 20;
         this.button1.Text = "To Excel";
         this.button1.UseVisualStyleBackColor = true;
         this.button1.Click += new System.EventHandler(this.button1_Click);
         // 
         // MemViewLarge
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(43)))), ((int)(((byte)(43)))), ((int)(((byte)(43)))));
         this.ClientSize = new System.Drawing.Size(1892, 1178);
         this.Controls.Add(this.button1);
         this.Controls.Add(this.heapFileView);
         this.Controls.Add(this.topAllocs);
         this.Controls.Add(this.label3);
         this.Controls.Add(this.label1);
         this.Controls.Add(this.heapKey);
         this.Controls.Add(this.heapLines);
         this.Controls.Add(this.mFileTimelines);
         this.Controls.Add(this.mFileGroupings);
         this.Controls.Add(this.mErrorList);
         this.Name = "MemViewLarge";
         this.TabText = "Client : []";
         this.Text = "Client : []";
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private HeapLines heapLines;
      private HeapKey heapKey;
      private TopAllocators topAllocs;
      private HeapFileView heapFileView;
      private FileTimelines mFileTimelines;
      private FileGroupings mFileGroupings;
      private ErrorList mErrorList;
      private System.Windows.Forms.Label label1;
      private System.Windows.Forms.Label label3;
      private System.Windows.Forms.Button button1;
   }
}
