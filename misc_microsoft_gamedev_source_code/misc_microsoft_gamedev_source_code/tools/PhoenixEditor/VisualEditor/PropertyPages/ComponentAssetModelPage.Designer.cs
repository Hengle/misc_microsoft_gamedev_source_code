namespace VisualEditor.PropertyPages
{
   partial class ComponentAssetModelPage
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
         this.fileBrowseControl2 = new EditorCore.FileBrowseControl();
         this.label1 = new System.Windows.Forms.Label();
         this.fileBrowseControl1 = new EditorCore.FileBrowseControl();
         this.label2 = new System.Windows.Forms.Label();
         this.label5 = new System.Windows.Forms.Label();
         this.label4 = new System.Windows.Forms.Label();
         this.label3 = new System.Windows.Forms.Label();
         this.label7 = new System.Windows.Forms.Label();
         this.label6 = new System.Windows.Forms.Label();
         this.floatSliderEdit4 = new VisualEditor.Controls.FloatSliderEdit();
         this.floatSliderEdit5 = new VisualEditor.Controls.FloatSliderEdit();
         this.sizeXFloatSliderEdit = new VisualEditor.Controls.FloatSliderEdit();
         this.floatSliderEdit2 = new VisualEditor.Controls.FloatSliderEdit();
         this.floatSliderEdit3 = new VisualEditor.Controls.FloatSliderEdit();
         this.sizeZFloatSliderEdit = new VisualEditor.Controls.FloatSliderEdit();
         this.groupBox1.SuspendLayout();
         this.SuspendLayout();
         // 
         // groupBox1
         // 
         this.groupBox1.Anchor = System.Windows.Forms.AnchorStyles.None;
         this.groupBox1.Controls.Add(this.label5);
         this.groupBox1.Controls.Add(this.fileBrowseControl2);
         this.groupBox1.Controls.Add(this.label4);
         this.groupBox1.Controls.Add(this.label1);
         this.groupBox1.Controls.Add(this.label3);
         this.groupBox1.Controls.Add(this.fileBrowseControl1);
         this.groupBox1.Controls.Add(this.floatSliderEdit4);
         this.groupBox1.Controls.Add(this.label2);
         this.groupBox1.Controls.Add(this.floatSliderEdit5);
         this.groupBox1.Controls.Add(this.floatSliderEdit2);
         this.groupBox1.Controls.Add(this.label6);
         this.groupBox1.Controls.Add(this.floatSliderEdit3);
         this.groupBox1.Controls.Add(this.sizeZFloatSliderEdit);
         this.groupBox1.Controls.Add(this.label7);
         this.groupBox1.Controls.Add(this.sizeXFloatSliderEdit);
         this.groupBox1.Location = new System.Drawing.Point(0, 0);
         this.groupBox1.Name = "groupBox1";
         this.groupBox1.Padding = new System.Windows.Forms.Padding(5, 16, 5, 3);
         this.groupBox1.Size = new System.Drawing.Size(350, 250);
         this.groupBox1.TabIndex = 1;
         this.groupBox1.TabStop = false;
         this.groupBox1.Text = "Model Asset Properties";
         // 
         // fileBrowseControl2
         // 
         this.fileBrowseControl2.FileName = "";
         this.fileBrowseControl2.FilterExtension = ".dmg";
         this.fileBrowseControl2.FilterName = "Granny Model Files";
         this.fileBrowseControl2.Location = new System.Drawing.Point(80, 62);
         this.fileBrowseControl2.Name = "fileBrowseControl2";
         this.fileBrowseControl2.ReferenceFolder = "art";
         this.fileBrowseControl2.Size = new System.Drawing.Size(262, 24);
         this.fileBrowseControl2.StartFolder = "art";
         this.fileBrowseControl2.TabIndex = 5;
         this.fileBrowseControl2.ValueChanged += new EditorCore.FileBrowseControl.ValueChangedDelegate(this.fileBrowseControl2_ValueChanged);
         // 
         // label1
         // 
         this.label1.AutoSize = true;
         this.label1.Location = new System.Drawing.Point(8, 67);
         this.label1.Margin = new System.Windows.Forms.Padding(3);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(66, 13);
         this.label1.TabIndex = 4;
         this.label1.Text = "Damage File";
         // 
         // fileBrowseControl1
         // 
         this.fileBrowseControl1.FileName = "";
         this.fileBrowseControl1.FilterExtension = ".gr2";
         this.fileBrowseControl1.FilterName = "Granny Model Files";
         this.fileBrowseControl1.Location = new System.Drawing.Point(80, 32);
         this.fileBrowseControl1.Name = "fileBrowseControl1";
         this.fileBrowseControl1.ReferenceFolder = "art";
         this.fileBrowseControl1.Size = new System.Drawing.Size(262, 24);
         this.fileBrowseControl1.StartFolder = "art";
         this.fileBrowseControl1.TabIndex = 3;
         this.fileBrowseControl1.ValueChanged += new EditorCore.FileBrowseControl.ValueChangedDelegate(this.fileBrowseControl1_ValueChanged);
         // 
         // label2
         // 
         this.label2.AutoSize = true;
         this.label2.Location = new System.Drawing.Point(8, 37);
         this.label2.Margin = new System.Windows.Forms.Padding(3);
         this.label2.Name = "label2";
         this.label2.Size = new System.Drawing.Size(55, 13);
         this.label2.TabIndex = 2;
         this.label2.Text = "Model File";
         // 
         // label5
         // 
         this.label5.AutoSize = true;
         this.label5.Location = new System.Drawing.Point(22, 194);
         this.label5.Name = "label5";
         this.label5.Size = new System.Drawing.Size(55, 13);
         this.label5.TabIndex = 61;
         this.label5.Text = "Channel 2";
         // 
         // label4
         // 
         this.label4.AutoSize = true;
         this.label4.Location = new System.Drawing.Point(22, 162);
         this.label4.Name = "label4";
         this.label4.Size = new System.Drawing.Size(55, 13);
         this.label4.TabIndex = 60;
         this.label4.Text = "Channel 1";
         // 
         // label3
         // 
         this.label3.AutoSize = true;
         this.label3.Location = new System.Drawing.Point(22, 129);
         this.label3.Name = "label3";
         this.label3.Size = new System.Drawing.Size(55, 13);
         this.label3.TabIndex = 59;
         this.label3.Text = "Channel 0";
         // 
         // label7
         // 
         this.label7.AutoSize = true;
         this.label7.Location = new System.Drawing.Point(125, 102);
         this.label7.Name = "label7";
         this.label7.Size = new System.Drawing.Size(46, 13);
         this.label7.TabIndex = 53;
         this.label7.Text = "U Offset";
         // 
         // label6
         // 
         this.label6.AutoSize = true;
         this.label6.Location = new System.Drawing.Point(249, 99);
         this.label6.Name = "label6";
         this.label6.Size = new System.Drawing.Size(45, 13);
         this.label6.TabIndex = 51;
         this.label6.Text = "V Offset";
         // 
         // floatSliderEdit4
         // 
         this.floatSliderEdit4.Location = new System.Drawing.Point(114, 185);
         this.floatSliderEdit4.Margin = new System.Windows.Forms.Padding(3, 3, 3, 0);
         this.floatSliderEdit4.MaxValue = 1F;
         this.floatSliderEdit4.MinValue = 0F;
         this.floatSliderEdit4.Name = "floatSliderEdit4";
         this.floatSliderEdit4.NumDecimals = 2;
         this.floatSliderEdit4.Size = new System.Drawing.Size(99, 40);
         this.floatSliderEdit4.TabIndex = 58;
         this.floatSliderEdit4.Value = 0F;
         this.floatSliderEdit4.ValueChanged += new VisualEditor.Controls.FloatSliderEdit.ValueChangedDelegate(this.floatSliderEdit4_ValueChanged);
         // 
         // floatSliderEdit5
         // 
         this.floatSliderEdit5.Location = new System.Drawing.Point(234, 185);
         this.floatSliderEdit5.Margin = new System.Windows.Forms.Padding(3, 3, 3, 0);
         this.floatSliderEdit5.MaxValue = 1F;
         this.floatSliderEdit5.MinValue = 0F;
         this.floatSliderEdit5.Name = "floatSliderEdit5";
         this.floatSliderEdit5.NumDecimals = 2;
         this.floatSliderEdit5.Size = new System.Drawing.Size(99, 40);
         this.floatSliderEdit5.TabIndex = 57;
         this.floatSliderEdit5.Value = 0F;
         this.floatSliderEdit5.ValueChanged += new VisualEditor.Controls.FloatSliderEdit.ValueChangedDelegate(this.floatSliderEdit5_ValueChanged);
         // 
         // sizeXFloatSliderEdit
         // 
         this.sizeXFloatSliderEdit.Location = new System.Drawing.Point(113, 118);
         this.sizeXFloatSliderEdit.Margin = new System.Windows.Forms.Padding(3, 3, 3, 0);
         this.sizeXFloatSliderEdit.MaxValue = 1F;
         this.sizeXFloatSliderEdit.MinValue = 0F;
         this.sizeXFloatSliderEdit.Name = "sizeXFloatSliderEdit";
         this.sizeXFloatSliderEdit.NumDecimals = 2;
         this.sizeXFloatSliderEdit.Size = new System.Drawing.Size(99, 40);
         this.sizeXFloatSliderEdit.TabIndex = 54;
         this.sizeXFloatSliderEdit.Value = 0F;
         this.sizeXFloatSliderEdit.ValueChanged += new VisualEditor.Controls.FloatSliderEdit.ValueChangedDelegate(this.sizeXFloatSliderEdit_ValueChanged);
         // 
         // floatSliderEdit2
         // 
         this.floatSliderEdit2.Location = new System.Drawing.Point(113, 151);
         this.floatSliderEdit2.Margin = new System.Windows.Forms.Padding(3, 3, 3, 0);
         this.floatSliderEdit2.MaxValue = 1F;
         this.floatSliderEdit2.MinValue = 0F;
         this.floatSliderEdit2.Name = "floatSliderEdit2";
         this.floatSliderEdit2.NumDecimals = 2;
         this.floatSliderEdit2.Size = new System.Drawing.Size(99, 40);
         this.floatSliderEdit2.TabIndex = 56;
         this.floatSliderEdit2.Value = 0F;
         this.floatSliderEdit2.ValueChanged += new VisualEditor.Controls.FloatSliderEdit.ValueChangedDelegate(this.floatSliderEdit2_ValueChanged);
         // 
         // floatSliderEdit3
         // 
         this.floatSliderEdit3.Location = new System.Drawing.Point(234, 151);
         this.floatSliderEdit3.Margin = new System.Windows.Forms.Padding(3, 3, 3, 0);
         this.floatSliderEdit3.MaxValue = 1F;
         this.floatSliderEdit3.MinValue = 0F;
         this.floatSliderEdit3.Name = "floatSliderEdit3";
         this.floatSliderEdit3.NumDecimals = 2;
         this.floatSliderEdit3.Size = new System.Drawing.Size(99, 40);
         this.floatSliderEdit3.TabIndex = 55;
         this.floatSliderEdit3.Value = 0F;
         this.floatSliderEdit3.ValueChanged += new VisualEditor.Controls.FloatSliderEdit.ValueChangedDelegate(this.floatSliderEdit3_ValueChanged);
         // 
         // sizeZFloatSliderEdit
         // 
         this.sizeZFloatSliderEdit.Location = new System.Drawing.Point(234, 118);
         this.sizeZFloatSliderEdit.Margin = new System.Windows.Forms.Padding(3, 3, 3, 0);
         this.sizeZFloatSliderEdit.MaxValue = 1F;
         this.sizeZFloatSliderEdit.MinValue = 0F;
         this.sizeZFloatSliderEdit.Name = "sizeZFloatSliderEdit";
         this.sizeZFloatSliderEdit.NumDecimals = 2;
         this.sizeZFloatSliderEdit.Size = new System.Drawing.Size(99, 40);
         this.sizeZFloatSliderEdit.TabIndex = 52;
         this.sizeZFloatSliderEdit.Value = 0F;
         this.sizeZFloatSliderEdit.ValueChanged += new VisualEditor.Controls.FloatSliderEdit.ValueChangedDelegate(this.sizeZFloatSliderEdit_ValueChanged);
         // 
         // ComponentAssetModelPage
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.groupBox1);
         this.Name = "ComponentAssetModelPage";
         this.Size = new System.Drawing.Size(350, 250);
         this.groupBox1.ResumeLayout(false);
         this.groupBox1.PerformLayout();
         this.ResumeLayout(false);

      }

      #endregion

      private System.Windows.Forms.GroupBox groupBox1;
      private System.Windows.Forms.Label label2;
      private EditorCore.FileBrowseControl fileBrowseControl1;
      private EditorCore.FileBrowseControl fileBrowseControl2;
      private System.Windows.Forms.Label label1;
      private System.Windows.Forms.Label label5;
      private System.Windows.Forms.Label label4;
      private System.Windows.Forms.Label label3;
      private VisualEditor.Controls.FloatSliderEdit floatSliderEdit4;
      private VisualEditor.Controls.FloatSliderEdit floatSliderEdit5;
      private VisualEditor.Controls.FloatSliderEdit sizeXFloatSliderEdit;
      private VisualEditor.Controls.FloatSliderEdit floatSliderEdit2;
      private System.Windows.Forms.Label label6;
      private VisualEditor.Controls.FloatSliderEdit floatSliderEdit3;
      private VisualEditor.Controls.FloatSliderEdit sizeZFloatSliderEdit;
      private System.Windows.Forms.Label label7;
   }
}
