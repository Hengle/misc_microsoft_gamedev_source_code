namespace RemoteMemory
{
   partial class FileGroupingElement
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
         this.components = new System.ComponentModel.Container();
         this.mTreeView = new GDIControls.GDITreeView();
         this.timer1 = new System.Windows.Forms.Timer(this.components);
         this.SuspendLayout();
         // 
         // mTreeView
         // 
         this.mTreeView.AutoUpdate = true;
         this.mTreeView.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(63)))), ((int)(((byte)(63)))), ((int)(((byte)(63)))));
         this.mTreeView.Dock = System.Windows.Forms.DockStyle.Fill;
         this.mTreeView.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(220)))), ((int)(((byte)(220)))), ((int)(((byte)(204)))));
         this.mTreeView.Location = new System.Drawing.Point(0, 0);
         this.mTreeView.Name = "mTreeView";
         this.mTreeView.NodeHorizontalSpacing = 12;
         this.mTreeView.NodeTextColor = System.Drawing.Color.FromArgb(((int)(((byte)(220)))), ((int)(((byte)(220)))), ((int)(((byte)(204)))));
         this.mTreeView.NodeVerticalSpacing = 3;
         this.mTreeView.SelectedNodeTextColor = System.Drawing.Color.Black;
         this.mTreeView.Size = new System.Drawing.Size(271, 219);
         this.mTreeView.TabIndex = 1;
         // 
         // timer1
         // 
         this.timer1.Tick += new System.EventHandler(this.timer1_Tick);
         // 
         // FileGroupingElement
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(63)))), ((int)(((byte)(63)))), ((int)(((byte)(63)))));
         this.Controls.Add(this.mTreeView);
         this.Margin = new System.Windows.Forms.Padding(0);
         this.Name = "FileGroupingElement";
         this.Size = new System.Drawing.Size(271, 219);
         this.ResumeLayout(false);

      }

      #endregion

      private GDIControls.GDITreeView mTreeView;
      private System.Windows.Forms.Timer timer1;
   }
}
