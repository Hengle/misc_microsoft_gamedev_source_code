namespace VisualEditor.PropertyPages
{
   partial class ComponentAssetLightPage
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
         this.groupBox1 = new System.Windows.Forms.GroupBox();
         this.fileBrowseControl1 = new EditorCore.FileBrowseControl();
         this.label2 = new System.Windows.Forms.Label();
         this.groupBox1.SuspendLayout();
         this.SuspendLayout();
         // 
         // groupBox1
         // 
         this.groupBox1.Anchor = System.Windows.Forms.AnchorStyles.None;
         this.groupBox1.Controls.Add(this.fileBrowseControl1);
         this.groupBox1.Controls.Add(this.label2);
         this.groupBox1.Location = new System.Drawing.Point(0, 0);
         this.groupBox1.Name = "groupBox1";
         this.groupBox1.Padding = new System.Windows.Forms.Padding(5, 16, 5, 3);
         this.groupBox1.Size = new System.Drawing.Size(350, 250);
         this.groupBox1.TabIndex = 2;
         this.groupBox1.TabStop = false;
         this.groupBox1.Text = "Light Asset Properties";
         // 
         // fileBrowseControl1
         // 
         this.fileBrowseControl1.FileName = "";
         this.fileBrowseControl1.FilterExtension = ".lgt";
         this.fileBrowseControl1.FilterName = "Light Effect Files";
         this.fileBrowseControl1.Location = new System.Drawing.Point(69, 32);
         this.fileBrowseControl1.Name = "fileBrowseControl1";
         this.fileBrowseControl1.ReferenceFolder = "art";
         this.fileBrowseControl1.Size = new System.Drawing.Size(273, 24);
         this.fileBrowseControl1.StartFolder = "art";
         this.fileBrowseControl1.TabIndex = 5;
         this.fileBrowseControl1.ValueChanged += new EditorCore.FileBrowseControl.ValueChangedDelegate(this.fileBrowseControl1_ValueChanged);
         // 
         // label2
         // 
         this.label2.AutoSize = true;
         this.label2.Location = new System.Drawing.Point(8, 37);
         this.label2.Margin = new System.Windows.Forms.Padding(3);
         this.label2.Name = "label2";
         this.label2.Size = new System.Drawing.Size(49, 13);
         this.label2.TabIndex = 4;
         this.label2.Text = "Light File";
         // 
         // ComponentAssetLightPage
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.groupBox1);
         this.Name = "ComponentAssetLightPage";
         this.Size = new System.Drawing.Size(350, 250);
         this.groupBox1.ResumeLayout(false);
         this.groupBox1.PerformLayout();
         this.ResumeLayout(false);

      }

      #endregion

      private System.Windows.Forms.GroupBox groupBox1;
      private EditorCore.FileBrowseControl fileBrowseControl1;
      private System.Windows.Forms.Label label2;
   }
}
