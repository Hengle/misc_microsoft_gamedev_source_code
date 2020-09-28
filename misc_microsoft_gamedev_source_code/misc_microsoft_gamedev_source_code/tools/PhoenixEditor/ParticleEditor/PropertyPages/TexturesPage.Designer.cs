namespace ParticleSystem
{
   partial class TexturePage
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
         System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(TexturePage));
         this.tabControl1 = new System.Windows.Forms.TabControl();
         this.tabPage7 = new System.Windows.Forms.TabPage();
         this.groupBox1 = new System.Windows.Forms.GroupBox();
         this.smartOptionList2 = new Xceed.SmartUI.Controls.OptionList.SmartOptionList(this.components);
         this.radioButtonNode4 = new Xceed.SmartUI.Controls.OptionList.RadioButtonNode("Multiply", 0);
         this.radioButtonNode6 = new Xceed.SmartUI.Controls.OptionList.RadioButtonNode("AlphaBlend", 1);
         this.imageList1 = new System.Windows.Forms.ImageList(this.components);
         this.smartOptionList1 = new Xceed.SmartUI.Controls.OptionList.SmartOptionList(this.components);
         this.radioButtonNode1 = new Xceed.SmartUI.Controls.OptionList.RadioButtonNode("Multiply", 0);
         this.radioButtonNode3 = new Xceed.SmartUI.Controls.OptionList.RadioButtonNode("AlphaBlend", 2);
         this.label2 = new System.Windows.Forms.Label();
         this.label1 = new System.Windows.Forms.Label();
         this.tabPage1 = new System.Windows.Forms.TabPage();
         this.uvControl1 = new ParticleSystem.UVControl();
         this.textureControl1 = new ParticleSystem.TextureControl();
         this.tabPage4 = new System.Windows.Forms.TabPage();
         this.uvControl4 = new ParticleSystem.UVControl();
         this.textureControl4 = new ParticleSystem.TextureControl();
         this.tabPage5 = new System.Windows.Forms.TabPage();
         this.uvControl6 = new ParticleSystem.UVControl();
         this.textureControl6 = new ParticleSystem.TextureControl();
         this.tabPage3 = new System.Windows.Forms.TabPage();
         this.uvControl3 = new ParticleSystem.UVControl();
         this.textureControl3 = new ParticleSystem.TextureControl();
         this.uvControl5 = new ParticleSystem.UVControl();
         this.textureControl5 = new ParticleSystem.TextureControl();
         this.tabControl1.SuspendLayout();
         this.tabPage7.SuspendLayout();
         this.groupBox1.SuspendLayout();
         ((System.ComponentModel.ISupportInitialize)(this.smartOptionList2)).BeginInit();
         ((System.ComponentModel.ISupportInitialize)(this.smartOptionList1)).BeginInit();
         this.tabPage1.SuspendLayout();
         this.tabPage4.SuspendLayout();
         this.tabPage5.SuspendLayout();
         this.tabPage3.SuspendLayout();
         this.SuspendLayout();
         // 
         // tabControl1
         // 
         this.tabControl1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.tabControl1.Controls.Add(this.tabPage7);
         this.tabControl1.Controls.Add(this.tabPage1);
         this.tabControl1.Controls.Add(this.tabPage4);
         this.tabControl1.Controls.Add(this.tabPage5);
         this.tabControl1.Controls.Add(this.tabPage3);
         this.tabControl1.Location = new System.Drawing.Point(0, 0);
         this.tabControl1.Name = "tabControl1";
         this.tabControl1.SelectedIndex = 0;
         this.tabControl1.Size = new System.Drawing.Size(928, 499);
         this.tabControl1.TabIndex = 0;
         // 
         // tabPage7
         // 
         this.tabPage7.Controls.Add(this.groupBox1);
         this.tabPage7.Location = new System.Drawing.Point(4, 22);
         this.tabPage7.Name = "tabPage7";
         this.tabPage7.Padding = new System.Windows.Forms.Padding(3);
         this.tabPage7.Size = new System.Drawing.Size(920, 473);
         this.tabPage7.TabIndex = 6;
         this.tabPage7.Text = "Blend Setup";
         this.tabPage7.UseVisualStyleBackColor = true;
         // 
         // groupBox1
         // 
         this.groupBox1.Controls.Add(this.smartOptionList2);
         this.groupBox1.Controls.Add(this.smartOptionList1);
         this.groupBox1.Controls.Add(this.label2);
         this.groupBox1.Controls.Add(this.label1);
         this.groupBox1.Dock = System.Windows.Forms.DockStyle.Fill;
         this.groupBox1.Location = new System.Drawing.Point(3, 3);
         this.groupBox1.Name = "groupBox1";
         this.groupBox1.Size = new System.Drawing.Size(914, 467);
         this.groupBox1.TabIndex = 0;
         this.groupBox1.TabStop = false;
         this.groupBox1.Text = "Blends";
         // 
         // smartOptionList2
         // 
         this.smartOptionList2.Items.AddRange(new Xceed.SmartUI.SmartItem[] {
            this.radioButtonNode4,
            this.radioButtonNode6});
         this.smartOptionList2.ItemsImageList = this.imageList1;
         this.smartOptionList2.Location = new System.Drawing.Point(204, 52);
         this.smartOptionList2.Name = "smartOptionList2";
         this.smartOptionList2.Size = new System.Drawing.Size(164, 74);
         this.smartOptionList2.TabIndex = 7;
         this.smartOptionList2.Text = "smartOptionList2";
         // 
         // radioButtonNode4
         // 
         this.radioButtonNode4.Grouped = true;
         this.radioButtonNode4.ImageIndex = 0;
         this.radioButtonNode4.Text = "Multiply";
         // 
         // radioButtonNode6
         // 
         this.radioButtonNode6.Grouped = true;
         this.radioButtonNode6.ImageIndex = 1;
         this.radioButtonNode6.Text = "AlphaBlend";
         // 
         // imageList1
         // 
         this.imageList1.ImageStream = ((System.Windows.Forms.ImageListStreamer)(resources.GetObject("imageList1.ImageStream")));
         this.imageList1.TransparentColor = System.Drawing.Color.Transparent;
         this.imageList1.Images.SetKeyName(0, "blend_additive.bmp");
         this.imageList1.Images.SetKeyName(1, "blend_alpha.bmp");
         this.imageList1.Images.SetKeyName(2, "blend_alpha.bmp");
         // 
         // smartOptionList1
         // 
         this.smartOptionList1.Cursor = System.Windows.Forms.Cursors.Default;
         this.smartOptionList1.Items.AddRange(new Xceed.SmartUI.SmartItem[] {
            this.radioButtonNode1,
            this.radioButtonNode3});
         this.smartOptionList1.ItemsImageList = this.imageList1;
         this.smartOptionList1.Location = new System.Drawing.Point(17, 52);
         this.smartOptionList1.Name = "smartOptionList1";
         this.smartOptionList1.Size = new System.Drawing.Size(164, 74);
         this.smartOptionList1.TabIndex = 6;
         this.smartOptionList1.Text = "smartOptionList1";
         // 
         // radioButtonNode1
         // 
         this.radioButtonNode1.Grouped = true;
         this.radioButtonNode1.ImageIndex = 0;
         this.radioButtonNode1.Text = "Multiply";
         // 
         // radioButtonNode3
         // 
         this.radioButtonNode3.Grouped = true;
         this.radioButtonNode3.ImageIndex = 2;
         this.radioButtonNode3.Text = "AlphaBlend";
         // 
         // label2
         // 
         this.label2.AutoSize = true;
         this.label2.Location = new System.Drawing.Point(223, 36);
         this.label2.Name = "label2";
         this.label2.Size = new System.Drawing.Size(107, 13);
         this.label2.TabIndex = 2;
         this.label2.Text = "Layer 2 <==> Layer 3";
         // 
         // label1
         // 
         this.label1.AutoSize = true;
         this.label1.Location = new System.Drawing.Point(42, 36);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(107, 13);
         this.label1.TabIndex = 0;
         this.label1.Text = "Layer 1 <==> Layer 2";
         // 
         // tabPage1
         // 
         this.tabPage1.Controls.Add(this.uvControl1);
         this.tabPage1.Controls.Add(this.textureControl1);
         this.tabPage1.Location = new System.Drawing.Point(4, 22);
         this.tabPage1.Name = "tabPage1";
         this.tabPage1.Padding = new System.Windows.Forms.Padding(3);
         this.tabPage1.Size = new System.Drawing.Size(920, 473);
         this.tabPage1.TabIndex = 0;
         this.tabPage1.Text = "Diffuse Layer 1";
         this.tabPage1.UseVisualStyleBackColor = true;
         // 
         // uvControl1
         // 
         this.uvControl1.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
         this.uvControl1.Location = new System.Drawing.Point(687, 6);
         this.uvControl1.Name = "uvControl1";
         this.uvControl1.Size = new System.Drawing.Size(227, 461);
         this.uvControl1.TabIndex = 1;
         // 
         // textureControl1
         // 
         this.textureControl1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.textureControl1.Location = new System.Drawing.Point(6, 6);
         this.textureControl1.Name = "textureControl1";
         this.textureControl1.Size = new System.Drawing.Size(675, 461);
         this.textureControl1.TabIndex = 0;
         // 
         // tabPage4
         // 
         this.tabPage4.Controls.Add(this.uvControl4);
         this.tabPage4.Controls.Add(this.textureControl4);
         this.tabPage4.Location = new System.Drawing.Point(4, 22);
         this.tabPage4.Name = "tabPage4";
         this.tabPage4.Padding = new System.Windows.Forms.Padding(3);
         this.tabPage4.Size = new System.Drawing.Size(920, 473);
         this.tabPage4.TabIndex = 3;
         this.tabPage4.Text = "Diffuse Layer 2";
         this.tabPage4.UseVisualStyleBackColor = true;
         // 
         // uvControl4
         // 
         this.uvControl4.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
         this.uvControl4.Location = new System.Drawing.Point(687, 6);
         this.uvControl4.Name = "uvControl4";
         this.uvControl4.Size = new System.Drawing.Size(227, 461);
         this.uvControl4.TabIndex = 3;
         // 
         // textureControl4
         // 
         this.textureControl4.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.textureControl4.Location = new System.Drawing.Point(6, 6);
         this.textureControl4.Name = "textureControl4";
         this.textureControl4.Size = new System.Drawing.Size(675, 461);
         this.textureControl4.TabIndex = 2;
         // 
         // tabPage5
         // 
         this.tabPage5.Controls.Add(this.uvControl6);
         this.tabPage5.Controls.Add(this.textureControl6);
         this.tabPage5.Location = new System.Drawing.Point(4, 22);
         this.tabPage5.Name = "tabPage5";
         this.tabPage5.Padding = new System.Windows.Forms.Padding(3);
         this.tabPage5.Size = new System.Drawing.Size(920, 473);
         this.tabPage5.TabIndex = 4;
         this.tabPage5.Text = "Diffuse Layer 3";
         this.tabPage5.UseVisualStyleBackColor = true;
         // 
         // uvControl6
         // 
         this.uvControl6.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
         this.uvControl6.Location = new System.Drawing.Point(687, 6);
         this.uvControl6.Name = "uvControl6";
         this.uvControl6.Size = new System.Drawing.Size(227, 461);
         this.uvControl6.TabIndex = 3;
         // 
         // textureControl6
         // 
         this.textureControl6.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.textureControl6.Location = new System.Drawing.Point(6, 6);
         this.textureControl6.Name = "textureControl6";
         this.textureControl6.Size = new System.Drawing.Size(675, 461);
         this.textureControl6.TabIndex = 2;
         // 
         // tabPage3
         // 
         this.tabPage3.Controls.Add(this.uvControl3);
         this.tabPage3.Controls.Add(this.textureControl3);
         this.tabPage3.Location = new System.Drawing.Point(4, 22);
         this.tabPage3.Name = "tabPage3";
         this.tabPage3.Padding = new System.Windows.Forms.Padding(3);
         this.tabPage3.Size = new System.Drawing.Size(920, 473);
         this.tabPage3.TabIndex = 2;
         this.tabPage3.Text = "Intensity";
         this.tabPage3.UseVisualStyleBackColor = true;
         // 
         // uvControl3
         // 
         this.uvControl3.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
         this.uvControl3.Location = new System.Drawing.Point(687, 6);
         this.uvControl3.Name = "uvControl3";
         this.uvControl3.Size = new System.Drawing.Size(227, 461);
         this.uvControl3.TabIndex = 1;
         // 
         // textureControl3
         // 
         this.textureControl3.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.textureControl3.Location = new System.Drawing.Point(6, 6);
         this.textureControl3.Name = "textureControl3";
         this.textureControl3.Size = new System.Drawing.Size(675, 461);
         this.textureControl3.TabIndex = 0;
         // 
         // uvControl5
         // 
         this.uvControl5.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
         this.uvControl5.Location = new System.Drawing.Point(687, 6);
         this.uvControl5.Name = "uvControl5";
         this.uvControl5.Size = new System.Drawing.Size(227, 159);
         this.uvControl5.TabIndex = 1;
         // 
         // textureControl5
         // 
         this.textureControl5.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.textureControl5.Location = new System.Drawing.Point(6, 6);
         this.textureControl5.Name = "textureControl5";
         this.textureControl5.Size = new System.Drawing.Size(675, 461);
         this.textureControl5.TabIndex = 0;
         // 
         // TexturePage
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.BackColor = System.Drawing.SystemColors.ActiveBorder;
         this.Controls.Add(this.tabControl1);
         this.Name = "TexturePage";
         this.Size = new System.Drawing.Size(928, 499);
         this.tabControl1.ResumeLayout(false);
         this.tabPage7.ResumeLayout(false);
         this.groupBox1.ResumeLayout(false);
         this.groupBox1.PerformLayout();
         ((System.ComponentModel.ISupportInitialize)(this.smartOptionList2)).EndInit();
         ((System.ComponentModel.ISupportInitialize)(this.smartOptionList1)).EndInit();
         this.tabPage1.ResumeLayout(false);
         this.tabPage4.ResumeLayout(false);
         this.tabPage5.ResumeLayout(false);
         this.tabPage3.ResumeLayout(false);
         this.ResumeLayout(false);

      }

      #endregion
      private System.Windows.Forms.TabControl tabControl1;
      private System.Windows.Forms.TabPage tabPage1;
      private UVControl uvControl1;
      private TextureControl textureControl1;
      private System.Windows.Forms.TabPage tabPage3;
      private UVControl uvControl3;
      private TextureControl textureControl3;
      private System.Windows.Forms.TabPage tabPage4;
      private System.Windows.Forms.TabPage tabPage7;
      private System.Windows.Forms.TabPage tabPage5;
      private System.Windows.Forms.GroupBox groupBox1;
      private UVControl uvControl5;
      private TextureControl textureControl5;
      private System.Windows.Forms.Label label2;
      private System.Windows.Forms.Label label1;
      private System.Windows.Forms.ImageList imageList1;
      private Xceed.SmartUI.Controls.OptionList.SmartOptionList smartOptionList2;
      private Xceed.SmartUI.Controls.OptionList.RadioButtonNode radioButtonNode4;
      private UVControl uvControl4;
      private TextureControl textureControl4;
      private UVControl uvControl6;
      private TextureControl textureControl6;
      private Xceed.SmartUI.Controls.OptionList.RadioButtonNode radioButtonNode6;
      private Xceed.SmartUI.Controls.OptionList.SmartOptionList smartOptionList1;
      private Xceed.SmartUI.Controls.OptionList.RadioButtonNode radioButtonNode1;
      private Xceed.SmartUI.Controls.OptionList.RadioButtonNode radioButtonNode3;
   }
}
