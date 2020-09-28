namespace VisualEditor.PropertyPages
{
   partial class AnimTagUVOffsetPage
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
         System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(AnimTagUVOffsetPage));
         this.groupBox1 = new System.Windows.Forms.GroupBox();
         this.sizeXFloatSliderEdit = new VisualEditor.Controls.FloatSliderEdit();
         this.label7 = new System.Windows.Forms.Label();
         this.sizeZFloatSliderEdit = new VisualEditor.Controls.FloatSliderEdit();
         this.label3 = new System.Windows.Forms.Label();
         this.RefreshButton = new System.Windows.Forms.Button();
         this.floatSliderEdit1 = new VisualEditor.Controls.FloatSliderEdit();
         this.label2 = new System.Windows.Forms.Label();
         this.groupBox1.SuspendLayout();
         this.SuspendLayout();
         // 
         // groupBox1
         // 
         this.groupBox1.Controls.Add(this.sizeXFloatSliderEdit);
         this.groupBox1.Controls.Add(this.label7);
         this.groupBox1.Controls.Add(this.sizeZFloatSliderEdit);
         this.groupBox1.Controls.Add(this.label3);
         this.groupBox1.Controls.Add(this.RefreshButton);
         this.groupBox1.Controls.Add(this.floatSliderEdit1);
         this.groupBox1.Controls.Add(this.label2);
         this.groupBox1.Location = new System.Drawing.Point(6, 5);
         this.groupBox1.Name = "groupBox1";
         this.groupBox1.Padding = new System.Windows.Forms.Padding(5, 16, 5, 3);
         this.groupBox1.Size = new System.Drawing.Size(351, 203);
         this.groupBox1.TabIndex = 4;
         this.groupBox1.TabStop = false;
         this.groupBox1.Text = "UV offset Tag Properties";
         // 
         // sizeXFloatSliderEdit
         // 
         this.sizeXFloatSliderEdit.Location = new System.Drawing.Point(60, 75);
         this.sizeXFloatSliderEdit.Margin = new System.Windows.Forms.Padding(3, 3, 3, 0);
         this.sizeXFloatSliderEdit.MaxValue = 1F;
         this.sizeXFloatSliderEdit.MinValue = 0F;
         this.sizeXFloatSliderEdit.Name = "sizeXFloatSliderEdit";
         this.sizeXFloatSliderEdit.NumDecimals = 2;
         this.sizeXFloatSliderEdit.Size = new System.Drawing.Size(99, 40);
         this.sizeXFloatSliderEdit.TabIndex = 39;
         this.sizeXFloatSliderEdit.Value = 0F;
         this.sizeXFloatSliderEdit.ValueChanged += new VisualEditor.Controls.FloatSliderEdit.ValueChangedDelegate(this.sizeXFloatSliderEdit_ValueChanged);
         // 
         // label7
         // 
         this.label7.AutoSize = true;
         this.label7.Location = new System.Drawing.Point(9, 75);
         this.label7.Name = "label7";
         this.label7.Size = new System.Drawing.Size(46, 13);
         this.label7.TabIndex = 38;
         this.label7.Text = "U Offset";
         // 
         // sizeZFloatSliderEdit
         // 
         this.sizeZFloatSliderEdit.Location = new System.Drawing.Point(230, 75);
         this.sizeZFloatSliderEdit.Margin = new System.Windows.Forms.Padding(3, 3, 3, 0);
         this.sizeZFloatSliderEdit.MaxValue = 1F;
         this.sizeZFloatSliderEdit.MinValue = 0F;
         this.sizeZFloatSliderEdit.Name = "sizeZFloatSliderEdit";
         this.sizeZFloatSliderEdit.NumDecimals = 2;
         this.sizeZFloatSliderEdit.Size = new System.Drawing.Size(99, 40);
         this.sizeZFloatSliderEdit.TabIndex = 31;
         this.sizeZFloatSliderEdit.Value = 0F;
         this.sizeZFloatSliderEdit.ValueChanged += new VisualEditor.Controls.FloatSliderEdit.ValueChangedDelegate(this.sizeZFloatSliderEdit_ValueChanged);
         // 
         // label3
         // 
         this.label3.AutoSize = true;
         this.label3.Location = new System.Drawing.Point(179, 75);
         this.label3.Name = "label3";
         this.label3.Size = new System.Drawing.Size(45, 13);
         this.label3.TabIndex = 30;
         this.label3.Text = "V Offset";
         // 
         // RefreshButton
         // 
         this.RefreshButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
         this.RefreshButton.Image = ((System.Drawing.Image)(resources.GetObject("RefreshButton.Image")));
         this.RefreshButton.Location = new System.Drawing.Point(319, 34);
         this.RefreshButton.Margin = new System.Windows.Forms.Padding(0);
         this.RefreshButton.Name = "RefreshButton";
         this.RefreshButton.Size = new System.Drawing.Size(24, 24);
         this.RefreshButton.TabIndex = 28;
         this.RefreshButton.UseVisualStyleBackColor = true;
         // 
         // floatSliderEdit1
         // 
         this.floatSliderEdit1.Location = new System.Drawing.Point(102, 32);
         this.floatSliderEdit1.Margin = new System.Windows.Forms.Padding(3, 3, 3, 0);
         this.floatSliderEdit1.MaxValue = 1F;
         this.floatSliderEdit1.MinValue = 0F;
         this.floatSliderEdit1.Name = "floatSliderEdit1";
         this.floatSliderEdit1.NumDecimals = 3;
         this.floatSliderEdit1.Size = new System.Drawing.Size(213, 40);
         this.floatSliderEdit1.TabIndex = 4;
         this.floatSliderEdit1.Value = 0F;
         // 
         // label2
         // 
         this.label2.AutoSize = true;
         this.label2.Location = new System.Drawing.Point(8, 34);
         this.label2.Name = "label2";
         this.label2.Size = new System.Drawing.Size(44, 13);
         this.label2.TabIndex = 3;
         this.label2.Text = "Position";
         // 
         // AnimTagUVOffsetPage
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.groupBox1);
         this.Name = "AnimTagUVOffsetPage";
         this.Size = new System.Drawing.Size(360, 211);
         this.groupBox1.ResumeLayout(false);
         this.groupBox1.PerformLayout();
         this.ResumeLayout(false);

      }

      #endregion

      private System.Windows.Forms.GroupBox groupBox1;
      private VisualEditor.Controls.FloatSliderEdit sizeXFloatSliderEdit;
      private System.Windows.Forms.Label label7;
      private VisualEditor.Controls.FloatSliderEdit sizeZFloatSliderEdit;
      private System.Windows.Forms.Label label3;
      private System.Windows.Forms.Button RefreshButton;
      private VisualEditor.Controls.FloatSliderEdit floatSliderEdit1;
      private System.Windows.Forms.Label label2;
   }
}
