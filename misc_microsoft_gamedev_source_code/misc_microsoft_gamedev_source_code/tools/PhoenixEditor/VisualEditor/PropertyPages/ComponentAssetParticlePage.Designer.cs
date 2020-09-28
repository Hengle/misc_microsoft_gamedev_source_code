using EditorCore;
namespace VisualEditor.PropertyPages
{
   partial class ComponentAssetParticlePage
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
         this.label3 = new System.Windows.Forms.Label();
         this.fileBrowseControl1 = new EditorCore.FileBrowseControl();
         this.groupBox1.SuspendLayout();
         this.SuspendLayout();
         // 
         // groupBox1
         // 
         this.groupBox1.Anchor = System.Windows.Forms.AnchorStyles.None;
         this.groupBox1.Controls.Add(this.fileBrowseControl1);
         this.groupBox1.Controls.Add(this.label3);
         this.groupBox1.Location = new System.Drawing.Point(0, 0);
         this.groupBox1.Name = "groupBox1";
         this.groupBox1.Padding = new System.Windows.Forms.Padding(5, 16, 5, 3);
         this.groupBox1.Size = new System.Drawing.Size(350, 250);
         this.groupBox1.TabIndex = 2;
         this.groupBox1.TabStop = false;
         this.groupBox1.Text = "Particle Asset Properties";
         // 
         // label3
         // 
         this.label3.AutoSize = true;
         this.label3.Location = new System.Drawing.Point(8, 37);
         this.label3.Margin = new System.Windows.Forms.Padding(3);
         this.label3.Name = "label3";
         this.label3.Size = new System.Drawing.Size(61, 13);
         this.label3.TabIndex = 6;
         this.label3.Text = "Particle File";
         // 
         // fileBrowseControl1
         // 
         this.fileBrowseControl1.FileName = "";
         this.fileBrowseControl1.FilterExtension = ".pfx";
         this.fileBrowseControl1.FilterName = "Particle Effect Files";
         this.fileBrowseControl1.Location = new System.Drawing.Point(69, 32);
         this.fileBrowseControl1.Name = "fileBrowseControl1";
         this.fileBrowseControl1.ReferenceFolder = "art";
         this.fileBrowseControl1.Size = new System.Drawing.Size(273, 24);
         this.fileBrowseControl1.StartFolder = "art\\effects";
         this.fileBrowseControl1.TabIndex = 7;
         this.fileBrowseControl1.ValueChanged += new EditorCore.FileBrowseControl.ValueChangedDelegate(this.fileBrowseControl1_ValueChanged);
         // 
         // ComponentAssetParticlePage
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.groupBox1);
         this.Name = "ComponentAssetParticlePage";
         this.Size = new System.Drawing.Size(350, 250);
         this.groupBox1.ResumeLayout(false);
         this.groupBox1.PerformLayout();
         this.ResumeLayout(false);

      }

      #endregion

      private System.Windows.Forms.GroupBox groupBox1;
      private EditorCore.FileBrowseControl fileBrowseControl1;
      private System.Windows.Forms.Label label3;
   }
}
