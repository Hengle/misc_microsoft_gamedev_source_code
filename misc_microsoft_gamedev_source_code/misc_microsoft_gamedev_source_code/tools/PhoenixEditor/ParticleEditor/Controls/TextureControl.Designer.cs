namespace ParticleSystem
{
   partial class TextureControl
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
         System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(TextureControl));
         this.groupBoxEX1 = new Dotnetrix.Controls.GroupBoxEX();
         this.groupBox1 = new System.Windows.Forms.GroupBox();
         this.numericUpDown1 = new System.Windows.Forms.NumericUpDown();
         this.button3 = new System.Windows.Forms.Button();
         this.label1 = new System.Windows.Forms.Label();
         this.button2 = new System.Windows.Forms.Button();
         this.button1 = new System.Windows.Forms.Button();
         this.listView1 = new System.Windows.Forms.ListView();
         this.toolTip1 = new System.Windows.Forms.ToolTip(this.components);
         this.groupBoxEX1.SuspendLayout();
         this.groupBox1.SuspendLayout();
         ((System.ComponentModel.ISupportInitialize)(this.numericUpDown1)).BeginInit();
         this.SuspendLayout();
         // 
         // groupBoxEX1
         // 
         this.groupBoxEX1.Controls.Add(this.groupBox1);
         this.groupBoxEX1.Controls.Add(this.button2);
         this.groupBoxEX1.Controls.Add(this.button1);
         this.groupBoxEX1.Controls.Add(this.listView1);
         this.groupBoxEX1.Dock = System.Windows.Forms.DockStyle.Fill;
         this.groupBoxEX1.Location = new System.Drawing.Point(0, 0);
         this.groupBoxEX1.Name = "groupBoxEX1";
         this.groupBoxEX1.Size = new System.Drawing.Size(945, 436);
         this.groupBoxEX1.TabIndex = 0;
         this.groupBoxEX1.TabStop = false;
         this.groupBoxEX1.Text = "Textures";
         // 
         // groupBox1
         // 
         this.groupBox1.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
         this.groupBox1.Controls.Add(this.numericUpDown1);
         this.groupBox1.Controls.Add(this.button3);
         this.groupBox1.Controls.Add(this.label1);
         this.groupBox1.Location = new System.Drawing.Point(694, 12);
         this.groupBox1.Name = "groupBox1";
         this.groupBox1.Size = new System.Drawing.Size(245, 59);
         this.groupBox1.TabIndex = 7;
         this.groupBox1.TabStop = false;
         this.groupBox1.Text = "Weight";
         // 
         // numericUpDown1
         // 
         this.numericUpDown1.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
         this.numericUpDown1.DecimalPlaces = 2;
         this.numericUpDown1.Increment = new decimal(new int[] {
            1,
            0,
            0,
            131072});
         this.numericUpDown1.Location = new System.Drawing.Point(97, 26);
         this.numericUpDown1.Maximum = new decimal(new int[] {
            -1593835520,
            466537709,
            54210,
            0});
         this.numericUpDown1.Name = "numericUpDown1";
         this.numericUpDown1.Size = new System.Drawing.Size(55, 20);
         this.numericUpDown1.TabIndex = 6;
         // 
         // button3
         // 
         this.button3.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
         this.button3.Image = ((System.Drawing.Image)(resources.GetObject("button3.Image")));
         this.button3.Location = new System.Drawing.Point(180, 15);
         this.button3.Name = "button3";
         this.button3.Size = new System.Drawing.Size(42, 38);
         this.button3.TabIndex = 5;
         this.button3.TabStop = false;
         this.toolTip1.SetToolTip(this.button3, "Apply Weight To Current Texture");
         this.button3.UseVisualStyleBackColor = true;
         this.button3.Click += new System.EventHandler(this.button3_Click);
         // 
         // label1
         // 
         this.label1.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
         this.label1.AutoSize = true;
         this.label1.Location = new System.Drawing.Point(50, 29);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(41, 13);
         this.label1.TabIndex = 3;
         this.label1.Text = "Weight";
         // 
         // button2
         // 
         this.button2.Image = ((System.Drawing.Image)(resources.GetObject("button2.Image")));
         this.button2.Location = new System.Drawing.Point(57, 27);
         this.button2.Name = "button2";
         this.button2.Size = new System.Drawing.Size(39, 38);
         this.button2.TabIndex = 2;
         this.button2.TabStop = false;
         this.toolTip1.SetToolTip(this.button2, "Remove Texture");
         this.button2.UseVisualStyleBackColor = true;
         this.button2.Click += new System.EventHandler(this.button2_Click);
         // 
         // button1
         // 
         this.button1.Image = ((System.Drawing.Image)(resources.GetObject("button1.Image")));
         this.button1.Location = new System.Drawing.Point(11, 27);
         this.button1.Name = "button1";
         this.button1.Size = new System.Drawing.Size(40, 38);
         this.button1.TabIndex = 1;
         this.button1.TabStop = false;
         this.toolTip1.SetToolTip(this.button1, "Add Texture");
         this.button1.UseVisualStyleBackColor = true;
         this.button1.Click += new System.EventHandler(this.button1_Click);
         // 
         // listView1
         // 
         this.listView1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.listView1.BackColor = System.Drawing.Color.Gray;
         this.listView1.Cursor = System.Windows.Forms.Cursors.Hand;
         this.listView1.Font = new System.Drawing.Font("Arial", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
         this.listView1.ForeColor = System.Drawing.Color.White;
         this.listView1.Location = new System.Drawing.Point(13, 77);
         this.listView1.MultiSelect = false;
         this.listView1.Name = "listView1";
         this.listView1.Size = new System.Drawing.Size(926, 352);
         this.listView1.TabIndex = 0;
         this.listView1.TabStop = false;
         this.listView1.UseCompatibleStateImageBehavior = false;
         this.listView1.View = System.Windows.Forms.View.Tile;
         this.listView1.SelectedIndexChanged += new System.EventHandler(this.listView1_SelectedIndexChanged);
         // 
         // TextureControl
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.groupBoxEX1);
         this.Name = "TextureControl";
         this.Size = new System.Drawing.Size(945, 436);
         this.groupBoxEX1.ResumeLayout(false);
         this.groupBox1.ResumeLayout(false);
         this.groupBox1.PerformLayout();
         ((System.ComponentModel.ISupportInitialize)(this.numericUpDown1)).EndInit();
         this.ResumeLayout(false);

      }

      #endregion

      private Dotnetrix.Controls.GroupBoxEX groupBoxEX1;
      private System.Windows.Forms.Label label1;
      private System.Windows.Forms.Button button2;
      private System.Windows.Forms.Button button1;
      private System.Windows.Forms.ListView listView1;
      private System.Windows.Forms.Button button3;
      private System.Windows.Forms.NumericUpDown numericUpDown1;
      private System.Windows.Forms.GroupBox groupBox1;
      private System.Windows.Forms.ToolTip toolTip1;
   }
}
