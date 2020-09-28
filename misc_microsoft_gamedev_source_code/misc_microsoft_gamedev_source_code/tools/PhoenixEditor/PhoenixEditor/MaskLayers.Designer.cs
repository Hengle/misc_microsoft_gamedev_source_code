namespace PhoenixEditor
{
   partial class MaskLayers
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

      #region Windows Form Designer generated code

      /// <summary>
      /// Required method for Designer support - do not modify
      /// the contents of this method with the code editor.
      /// </summary>
      private void InitializeComponent()
      {
         this.components = new System.ComponentModel.Container();
         this.MaskCheckList = new System.Windows.Forms.CheckedListBox();
         this.MaskCheckListMenu = new System.Windows.Forms.ContextMenuStrip(this.components);
         this.previewToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
         this.button1 = new System.Windows.Forms.Button();
         this.button2 = new System.Windows.Forms.Button();
         this.button3 = new System.Windows.Forms.Button();
         this.button4 = new System.Windows.Forms.Button();
         this.button5 = new System.Windows.Forms.Button();
         this.button6 = new System.Windows.Forms.Button();
         this.groupBox1 = new System.Windows.Forms.GroupBox();
         this.groupBox2 = new System.Windows.Forms.GroupBox();
         this.vertLabel = new System.Windows.Forms.Label();
         this.trackBar1 = new System.Windows.Forms.TrackBar();
         this.groupBox3 = new System.Windows.Forms.GroupBox();
         this.blurSigmaLabel = new System.Windows.Forms.Label();
         this.blurSigmaTrackbar = new System.Windows.Forms.TrackBar();
         this.groupBox4 = new System.Windows.Forms.GroupBox();
         this.groupBox11 = new System.Windows.Forms.GroupBox();
         this.checkBox2 = new System.Windows.Forms.CheckBox();
         this.PostSmoothCheckBox = new System.Windows.Forms.CheckBox();
         this.groupBox7 = new System.Windows.Forms.GroupBox();
         this.MaxHeightCheckBox = new System.Windows.Forms.CheckBox();
         this.SampleMaxHeightButton = new System.Windows.Forms.Button();
         this.MaxNoiseCheckBox = new System.Windows.Forms.CheckBox();
         this.UseMaxHeightCheckBox = new System.Windows.Forms.CheckBox();
         this.groupBox6 = new System.Windows.Forms.GroupBox();
         this.SlopeSelectSliderControl = new EditorCore.NumericSliderControl();
         this.SampleSlopeButton = new System.Windows.Forms.Button();
         this.UseSlopeCheckBox = new System.Windows.Forms.CheckBox();
         this.groupBox5 = new System.Windows.Forms.GroupBox();
         this.MinHeightCheckBox = new System.Windows.Forms.CheckBox();
         this.MinNoiseCheckBox = new System.Windows.Forms.CheckBox();
         this.SampleMinHeightButton = new System.Windows.Forms.Button();
         this.UseMinHeightCheckBox = new System.Windows.Forms.CheckBox();
         this.panel1 = new System.Windows.Forms.Panel();
         this.groupBox13 = new System.Windows.Forms.GroupBox();
         this.DisplacementFadeCheckBox = new System.Windows.Forms.CheckBox();
         this.MaskTextureButton = new System.Windows.Forms.Button();
         this.groupBox12 = new System.Windows.Forms.GroupBox();
         this.ScaleAddCheckBox = new System.Windows.Forms.CheckBox();
         this.ScaleAmountSliderControl = new EditorCore.NumericSliderControl();
         this.button8 = new System.Windows.Forms.Button();
         this.button7 = new System.Windows.Forms.Button();
         this.label1 = new System.Windows.Forms.Label();
         this.groupBox8 = new System.Windows.Forms.GroupBox();
         this.scaleToFitCheckbox = new System.Windows.Forms.CheckBox();
         this.ToFileButton = new System.Windows.Forms.Button();
         this.ToClipboardButton = new System.Windows.Forms.Button();
         this.FromClipboardButton = new System.Windows.Forms.Button();
         this.ImportMaskButton = new System.Windows.Forms.Button();
         this.groupBox9 = new System.Windows.Forms.GroupBox();
         this.DeleteMaskButton = new System.Windows.Forms.Button();
         this.SetAsBaseButton = new System.Windows.Forms.Button();
         this.CurrentMaskNameTextBox = new System.Windows.Forms.TextBox();
         this.ClearCurrentButton = new System.Windows.Forms.Button();
         this.SaveCurrentButton = new System.Windows.Forms.Button();
         this.groupBox10 = new System.Windows.Forms.GroupBox();
         this.pictureBox1 = new System.Windows.Forms.PictureBox();
         this.SetAsCurrentButton = new System.Windows.Forms.Button();
         this.ClearBaseButton = new System.Windows.Forms.Button();
         this.button9 = new System.Windows.Forms.Button();
         this.MaskCheckListMenu.SuspendLayout();
         this.groupBox1.SuspendLayout();
         this.groupBox2.SuspendLayout();
         ((System.ComponentModel.ISupportInitialize)(this.trackBar1)).BeginInit();
         this.groupBox3.SuspendLayout();
         ((System.ComponentModel.ISupportInitialize)(this.blurSigmaTrackbar)).BeginInit();
         this.groupBox4.SuspendLayout();
         this.groupBox11.SuspendLayout();
         this.groupBox7.SuspendLayout();
         this.groupBox6.SuspendLayout();
         this.groupBox5.SuspendLayout();
         this.panel1.SuspendLayout();
         this.groupBox13.SuspendLayout();
         this.groupBox12.SuspendLayout();
         this.groupBox8.SuspendLayout();
         this.groupBox9.SuspendLayout();
         this.groupBox10.SuspendLayout();
         ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).BeginInit();
         this.SuspendLayout();
         // 
         // MaskCheckList
         // 
         this.MaskCheckList.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                     | System.Windows.Forms.AnchorStyles.Right)));
         this.MaskCheckList.CheckOnClick = true;
         this.MaskCheckList.ContextMenuStrip = this.MaskCheckListMenu;
         this.MaskCheckList.FormattingEnabled = true;
         this.MaskCheckList.Location = new System.Drawing.Point(3, 91);
         this.MaskCheckList.Name = "MaskCheckList";
         this.MaskCheckList.Size = new System.Drawing.Size(229, 124);
         this.MaskCheckList.TabIndex = 1;
         this.MaskCheckList.ItemCheck += new System.Windows.Forms.ItemCheckEventHandler(this.MaskCheckList_ItemCheck);
         // 
         // MaskCheckListMenu
         // 
         this.MaskCheckListMenu.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.previewToolStripMenuItem});
         this.MaskCheckListMenu.Name = "MaskCheckListMenu";
         this.MaskCheckListMenu.Size = new System.Drawing.Size(124, 26);
         // 
         // previewToolStripMenuItem
         // 
         this.previewToolStripMenuItem.Name = "previewToolStripMenuItem";
         this.previewToolStripMenuItem.Size = new System.Drawing.Size(123, 22);
         this.previewToolStripMenuItem.Text = "Preview";
         this.previewToolStripMenuItem.Click += new System.EventHandler(this.previewToolStripMenuItem_Click);
         // 
         // button1
         // 
         this.button1.Location = new System.Drawing.Point(6, 48);
         this.button1.Name = "button1";
         this.button1.Size = new System.Drawing.Size(75, 23);
         this.button1.TabIndex = 5;
         this.button1.Text = "Edge Detect";
         this.button1.UseVisualStyleBackColor = true;
         this.button1.Click += new System.EventHandler(this.DetectEdges_Click);
         // 
         // button2
         // 
         this.button2.Location = new System.Drawing.Point(6, 19);
         this.button2.Name = "button2";
         this.button2.Size = new System.Drawing.Size(75, 23);
         this.button2.TabIndex = 6;
         this.button2.Text = "Smooth";
         this.button2.UseVisualStyleBackColor = true;
         this.button2.Click += new System.EventHandler(this.SmoothFilter_Click);
         // 
         // button3
         // 
         this.button3.Location = new System.Drawing.Point(83, 19);
         this.button3.Name = "button3";
         this.button3.Size = new System.Drawing.Size(75, 23);
         this.button3.TabIndex = 7;
         this.button3.Text = "Sharpen";
         this.button3.UseVisualStyleBackColor = true;
         this.button3.Click += new System.EventHandler(this.SharpenFilter_Click);
         // 
         // button4
         // 
         this.button4.Location = new System.Drawing.Point(87, 58);
         this.button4.Name = "button4";
         this.button4.Size = new System.Drawing.Size(75, 23);
         this.button4.TabIndex = 8;
         this.button4.Text = "Expand";
         this.button4.UseVisualStyleBackColor = true;
         this.button4.Click += new System.EventHandler(this.ExpandSelection_Click);
         // 
         // button5
         // 
         this.button5.Location = new System.Drawing.Point(6, 58);
         this.button5.Name = "button5";
         this.button5.Size = new System.Drawing.Size(75, 23);
         this.button5.TabIndex = 9;
         this.button5.Text = "Contract";
         this.button5.UseVisualStyleBackColor = true;
         this.button5.Click += new System.EventHandler(this.ContractSelection_Click);
         // 
         // button6
         // 
         this.button6.Location = new System.Drawing.Point(13, 54);
         this.button6.Name = "button6";
         this.button6.Size = new System.Drawing.Size(75, 23);
         this.button6.TabIndex = 10;
         this.button6.Text = "Blur";
         this.button6.UseVisualStyleBackColor = true;
         this.button6.Click += new System.EventHandler(this.BlurSelection_Click);
         // 
         // groupBox1
         // 
         this.groupBox1.Controls.Add(this.button1);
         this.groupBox1.Controls.Add(this.button2);
         this.groupBox1.Controls.Add(this.button3);
         this.groupBox1.Location = new System.Drawing.Point(3, 3);
         this.groupBox1.Name = "groupBox1";
         this.groupBox1.Size = new System.Drawing.Size(202, 78);
         this.groupBox1.TabIndex = 11;
         this.groupBox1.TabStop = false;
         this.groupBox1.Text = "Simple";
         // 
         // groupBox2
         // 
         this.groupBox2.Controls.Add(this.button5);
         this.groupBox2.Controls.Add(this.vertLabel);
         this.groupBox2.Controls.Add(this.trackBar1);
         this.groupBox2.Controls.Add(this.button4);
         this.groupBox2.Location = new System.Drawing.Point(3, 87);
         this.groupBox2.Name = "groupBox2";
         this.groupBox2.Size = new System.Drawing.Size(202, 92);
         this.groupBox2.TabIndex = 12;
         this.groupBox2.TabStop = false;
         this.groupBox2.Text = "Size";
         // 
         // vertLabel
         // 
         this.vertLabel.AutoSize = true;
         this.vertLabel.Location = new System.Drawing.Point(94, 29);
         this.vertLabel.Name = "vertLabel";
         this.vertLabel.Size = new System.Drawing.Size(0, 13);
         this.vertLabel.TabIndex = 11;
         // 
         // trackBar1
         // 
         this.trackBar1.Location = new System.Drawing.Point(6, 19);
         this.trackBar1.Maximum = 30;
         this.trackBar1.Minimum = 1;
         this.trackBar1.Name = "trackBar1";
         this.trackBar1.Size = new System.Drawing.Size(82, 45);
         this.trackBar1.TabIndex = 10;
         this.trackBar1.Value = 1;
         this.trackBar1.Scroll += new System.EventHandler(this.trackBar1_Scroll);
         // 
         // groupBox3
         // 
         this.groupBox3.Controls.Add(this.button6);
         this.groupBox3.Controls.Add(this.blurSigmaLabel);
         this.groupBox3.Controls.Add(this.blurSigmaTrackbar);
         this.groupBox3.Location = new System.Drawing.Point(3, 185);
         this.groupBox3.Name = "groupBox3";
         this.groupBox3.Size = new System.Drawing.Size(202, 83);
         this.groupBox3.TabIndex = 13;
         this.groupBox3.TabStop = false;
         this.groupBox3.Text = "Advanced Blur";
         // 
         // blurSigmaLabel
         // 
         this.blurSigmaLabel.AutoSize = true;
         this.blurSigmaLabel.Location = new System.Drawing.Point(94, 24);
         this.blurSigmaLabel.Name = "blurSigmaLabel";
         this.blurSigmaLabel.Size = new System.Drawing.Size(0, 13);
         this.blurSigmaLabel.TabIndex = 15;
         // 
         // blurSigmaTrackbar
         // 
         this.blurSigmaTrackbar.Location = new System.Drawing.Point(6, 14);
         this.blurSigmaTrackbar.Maximum = 50;
         this.blurSigmaTrackbar.Minimum = 10;
         this.blurSigmaTrackbar.Name = "blurSigmaTrackbar";
         this.blurSigmaTrackbar.Size = new System.Drawing.Size(82, 45);
         this.blurSigmaTrackbar.TabIndex = 14;
         this.blurSigmaTrackbar.Value = 20;
         this.blurSigmaTrackbar.Scroll += new System.EventHandler(this.blurSigmaTrackbar_Scroll);
         // 
         // groupBox4
         // 
         this.groupBox4.BackColor = System.Drawing.SystemColors.Control;
         this.groupBox4.Controls.Add(this.groupBox11);
         this.groupBox4.Controls.Add(this.PostSmoothCheckBox);
         this.groupBox4.Controls.Add(this.groupBox7);
         this.groupBox4.Controls.Add(this.groupBox6);
         this.groupBox4.Controls.Add(this.groupBox5);
         this.groupBox4.Location = new System.Drawing.Point(3, 373);
         this.groupBox4.Name = "groupBox4";
         this.groupBox4.Size = new System.Drawing.Size(202, 338);
         this.groupBox4.TabIndex = 14;
         this.groupBox4.TabStop = false;
         this.groupBox4.Text = "Component Masking";
         // 
         // groupBox11
         // 
         this.groupBox11.Controls.Add(this.checkBox2);
         this.groupBox11.Location = new System.Drawing.Point(6, 255);
         this.groupBox11.Name = "groupBox11";
         this.groupBox11.Size = new System.Drawing.Size(190, 48);
         this.groupBox11.TabIndex = 16;
         this.groupBox11.TabStop = false;
         // 
         // checkBox2
         // 
         this.checkBox2.AutoSize = true;
         this.checkBox2.Location = new System.Drawing.Point(6, 19);
         this.checkBox2.Name = "checkBox2";
         this.checkBox2.Size = new System.Drawing.Size(72, 17);
         this.checkBox2.TabIndex = 6;
         this.checkBox2.Text = "Gradation";
         this.checkBox2.UseVisualStyleBackColor = true;
         this.checkBox2.CheckedChanged += new System.EventHandler(this.checkBox2_CheckedChanged);
         // 
         // PostSmoothCheckBox
         // 
         this.PostSmoothCheckBox.AutoSize = true;
         this.PostSmoothCheckBox.Checked = true;
         this.PostSmoothCheckBox.CheckState = System.Windows.Forms.CheckState.Checked;
         this.PostSmoothCheckBox.Location = new System.Drawing.Point(6, 309);
         this.PostSmoothCheckBox.Name = "PostSmoothCheckBox";
         this.PostSmoothCheckBox.Size = new System.Drawing.Size(86, 17);
         this.PostSmoothCheckBox.TabIndex = 15;
         this.PostSmoothCheckBox.Text = "Post Smooth";
         this.PostSmoothCheckBox.UseVisualStyleBackColor = true;
         this.PostSmoothCheckBox.CheckedChanged += new System.EventHandler(this.PostSmoothCheckBox_CheckedChanged);
         // 
         // groupBox7
         // 
         this.groupBox7.Controls.Add(this.MaxHeightCheckBox);
         this.groupBox7.Controls.Add(this.SampleMaxHeightButton);
         this.groupBox7.Controls.Add(this.MaxNoiseCheckBox);
         this.groupBox7.Controls.Add(this.UseMaxHeightCheckBox);
         this.groupBox7.Location = new System.Drawing.Point(6, 176);
         this.groupBox7.Name = "groupBox7";
         this.groupBox7.Size = new System.Drawing.Size(190, 73);
         this.groupBox7.TabIndex = 5;
         this.groupBox7.TabStop = false;
         // 
         // MaxHeightCheckBox
         // 
         this.MaxHeightCheckBox.AutoSize = true;
         this.MaxHeightCheckBox.Location = new System.Drawing.Point(44, 42);
         this.MaxHeightCheckBox.Name = "MaxHeightCheckBox";
         this.MaxHeightCheckBox.Size = new System.Drawing.Size(66, 17);
         this.MaxHeightCheckBox.TabIndex = 7;
         this.MaxHeightCheckBox.Text = "Gradient";
         this.MaxHeightCheckBox.UseVisualStyleBackColor = true;
         this.MaxHeightCheckBox.CheckedChanged += new System.EventHandler(this.MaxHeightCheckBox_CheckedChanged);
         // 
         // SampleMaxHeightButton
         // 
         this.SampleMaxHeightButton.Location = new System.Drawing.Point(0, 19);
         this.SampleMaxHeightButton.Name = "SampleMaxHeightButton";
         this.SampleMaxHeightButton.Size = new System.Drawing.Size(31, 23);
         this.SampleMaxHeightButton.TabIndex = 3;
         this.SampleMaxHeightButton.Text = "Set";
         this.SampleMaxHeightButton.UseVisualStyleBackColor = true;
         this.SampleMaxHeightButton.Click += new System.EventHandler(this.SampleMaxHeightButton_Click);
         // 
         // MaxNoiseCheckBox
         // 
         this.MaxNoiseCheckBox.AutoSize = true;
         this.MaxNoiseCheckBox.Location = new System.Drawing.Point(44, 19);
         this.MaxNoiseCheckBox.Name = "MaxNoiseCheckBox";
         this.MaxNoiseCheckBox.Size = new System.Drawing.Size(53, 17);
         this.MaxNoiseCheckBox.TabIndex = 6;
         this.MaxNoiseCheckBox.Text = "Noise";
         this.MaxNoiseCheckBox.UseVisualStyleBackColor = true;
         this.MaxNoiseCheckBox.CheckedChanged += new System.EventHandler(this.MaxNoiseCheckBox_CheckedChanged);
         // 
         // UseMaxHeightCheckBox
         // 
         this.UseMaxHeightCheckBox.AutoSize = true;
         this.UseMaxHeightCheckBox.Location = new System.Drawing.Point(0, 0);
         this.UseMaxHeightCheckBox.Name = "UseMaxHeightCheckBox";
         this.UseMaxHeightCheckBox.Size = new System.Drawing.Size(80, 17);
         this.UseMaxHeightCheckBox.TabIndex = 2;
         this.UseMaxHeightCheckBox.Text = "Max Height";
         this.UseMaxHeightCheckBox.UseVisualStyleBackColor = true;
         this.UseMaxHeightCheckBox.CheckedChanged += new System.EventHandler(this.UseMaxHeightCheckBox_CheckedChanged);
         // 
         // groupBox6
         // 
         this.groupBox6.Controls.Add(this.SlopeSelectSliderControl);
         this.groupBox6.Controls.Add(this.SampleSlopeButton);
         this.groupBox6.Controls.Add(this.UseSlopeCheckBox);
         this.groupBox6.Location = new System.Drawing.Point(6, 22);
         this.groupBox6.Name = "groupBox6";
         this.groupBox6.Size = new System.Drawing.Size(190, 69);
         this.groupBox6.TabIndex = 4;
         this.groupBox6.TabStop = false;
         // 
         // SlopeSelectSliderControl
         // 
         this.SlopeSelectSliderControl.BackColor = System.Drawing.SystemColors.ControlLight;
         this.SlopeSelectSliderControl.Location = new System.Drawing.Point(38, 19);
         this.SlopeSelectSliderControl.Name = "SlopeSelectSliderControl";
         this.SlopeSelectSliderControl.Size = new System.Drawing.Size(146, 22);
         this.SlopeSelectSliderControl.TabIndex = 2;
         // 
         // SampleSlopeButton
         // 
         this.SampleSlopeButton.Location = new System.Drawing.Point(0, 19);
         this.SampleSlopeButton.Name = "SampleSlopeButton";
         this.SampleSlopeButton.Size = new System.Drawing.Size(32, 27);
         this.SampleSlopeButton.TabIndex = 0;
         this.SampleSlopeButton.Text = "Set";
         this.SampleSlopeButton.UseVisualStyleBackColor = true;
         this.SampleSlopeButton.Click += new System.EventHandler(this.SampleSlopeButton_Click);
         // 
         // UseSlopeCheckBox
         // 
         this.UseSlopeCheckBox.AutoSize = true;
         this.UseSlopeCheckBox.Location = new System.Drawing.Point(0, 0);
         this.UseSlopeCheckBox.Name = "UseSlopeCheckBox";
         this.UseSlopeCheckBox.Size = new System.Drawing.Size(53, 17);
         this.UseSlopeCheckBox.TabIndex = 1;
         this.UseSlopeCheckBox.Text = "Slope";
         this.UseSlopeCheckBox.UseVisualStyleBackColor = true;
         this.UseSlopeCheckBox.CheckedChanged += new System.EventHandler(this.UseSlopeCheckBox_CheckedChanged);
         // 
         // groupBox5
         // 
         this.groupBox5.Controls.Add(this.MinHeightCheckBox);
         this.groupBox5.Controls.Add(this.MinNoiseCheckBox);
         this.groupBox5.Controls.Add(this.SampleMinHeightButton);
         this.groupBox5.Controls.Add(this.UseMinHeightCheckBox);
         this.groupBox5.Location = new System.Drawing.Point(6, 97);
         this.groupBox5.Name = "groupBox5";
         this.groupBox5.Size = new System.Drawing.Size(190, 73);
         this.groupBox5.TabIndex = 3;
         this.groupBox5.TabStop = false;
         // 
         // MinHeightCheckBox
         // 
         this.MinHeightCheckBox.AutoSize = true;
         this.MinHeightCheckBox.Location = new System.Drawing.Point(44, 42);
         this.MinHeightCheckBox.Name = "MinHeightCheckBox";
         this.MinHeightCheckBox.Size = new System.Drawing.Size(66, 17);
         this.MinHeightCheckBox.TabIndex = 5;
         this.MinHeightCheckBox.Text = "Gradient";
         this.MinHeightCheckBox.UseVisualStyleBackColor = true;
         this.MinHeightCheckBox.CheckedChanged += new System.EventHandler(this.MinHeightCheckBox_CheckedChanged);
         // 
         // MinNoiseCheckBox
         // 
         this.MinNoiseCheckBox.AutoSize = true;
         this.MinNoiseCheckBox.Location = new System.Drawing.Point(44, 19);
         this.MinNoiseCheckBox.Name = "MinNoiseCheckBox";
         this.MinNoiseCheckBox.Size = new System.Drawing.Size(53, 17);
         this.MinNoiseCheckBox.TabIndex = 4;
         this.MinNoiseCheckBox.Text = "Noise";
         this.MinNoiseCheckBox.UseVisualStyleBackColor = true;
         this.MinNoiseCheckBox.CheckedChanged += new System.EventHandler(this.MinNoiseCheckBox_CheckedChanged);
         // 
         // SampleMinHeightButton
         // 
         this.SampleMinHeightButton.Location = new System.Drawing.Point(0, 19);
         this.SampleMinHeightButton.Name = "SampleMinHeightButton";
         this.SampleMinHeightButton.Size = new System.Drawing.Size(32, 23);
         this.SampleMinHeightButton.TabIndex = 3;
         this.SampleMinHeightButton.Text = "Set";
         this.SampleMinHeightButton.UseVisualStyleBackColor = true;
         this.SampleMinHeightButton.Click += new System.EventHandler(this.SampleMinHeightButton_Click);
         // 
         // UseMinHeightCheckBox
         // 
         this.UseMinHeightCheckBox.AutoSize = true;
         this.UseMinHeightCheckBox.Location = new System.Drawing.Point(0, 0);
         this.UseMinHeightCheckBox.Name = "UseMinHeightCheckBox";
         this.UseMinHeightCheckBox.Size = new System.Drawing.Size(77, 17);
         this.UseMinHeightCheckBox.TabIndex = 2;
         this.UseMinHeightCheckBox.Text = "Min Height";
         this.UseMinHeightCheckBox.UseVisualStyleBackColor = true;
         this.UseMinHeightCheckBox.CheckedChanged += new System.EventHandler(this.UseMinHeightCheckBox_CheckedChanged);
         // 
         // panel1
         // 
         this.panel1.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                     | System.Windows.Forms.AnchorStyles.Left)));
         this.panel1.AutoScroll = true;
         this.panel1.AutoScrollMargin = new System.Drawing.Size(0, 50);
         this.panel1.AutoScrollMinSize = new System.Drawing.Size(0, 600);
         this.panel1.Controls.Add(this.groupBox13);
         this.panel1.Controls.Add(this.groupBox12);
         this.panel1.Controls.Add(this.groupBox8);
         this.panel1.Controls.Add(this.groupBox1);
         this.panel1.Controls.Add(this.groupBox4);
         this.panel1.Controls.Add(this.groupBox2);
         this.panel1.Controls.Add(this.groupBox3);
         this.panel1.Location = new System.Drawing.Point(6, 327);
         this.panel1.Name = "panel1";
         this.panel1.Size = new System.Drawing.Size(228, 622);
         this.panel1.TabIndex = 15;
         this.panel1.Paint += new System.Windows.Forms.PaintEventHandler(this.panel1_Paint);
         // 
         // groupBox13
         // 
         this.groupBox13.Controls.Add(this.DisplacementFadeCheckBox);
         this.groupBox13.Controls.Add(this.MaskTextureButton);
         this.groupBox13.Location = new System.Drawing.Point(3, 717);
         this.groupBox13.Name = "groupBox13";
         this.groupBox13.Size = new System.Drawing.Size(200, 73);
         this.groupBox13.TabIndex = 17;
         this.groupBox13.TabStop = false;
         this.groupBox13.Text = "Texture";
         // 
         // DisplacementFadeCheckBox
         // 
         this.DisplacementFadeCheckBox.AutoSize = true;
         this.DisplacementFadeCheckBox.Location = new System.Drawing.Point(8, 48);
         this.DisplacementFadeCheckBox.Name = "DisplacementFadeCheckBox";
         this.DisplacementFadeCheckBox.Size = new System.Drawing.Size(111, 17);
         this.DisplacementFadeCheckBox.TabIndex = 1;
         this.DisplacementFadeCheckBox.Text = "Use Texture Fade";
         this.DisplacementFadeCheckBox.UseVisualStyleBackColor = true;
         // 
         // MaskTextureButton
         // 
         this.MaskTextureButton.Location = new System.Drawing.Point(7, 19);
         this.MaskTextureButton.Name = "MaskTextureButton";
         this.MaskTextureButton.Size = new System.Drawing.Size(177, 23);
         this.MaskTextureButton.TabIndex = 0;
         this.MaskTextureButton.Text = "Mask Texture Displacement";
         this.MaskTextureButton.UseVisualStyleBackColor = true;
         this.MaskTextureButton.Click += new System.EventHandler(this.MaskTextureButton_Click_1);
         // 
         // groupBox12
         // 
         this.groupBox12.Controls.Add(this.ScaleAddCheckBox);
         this.groupBox12.Controls.Add(this.ScaleAmountSliderControl);
         this.groupBox12.Controls.Add(this.button8);
         this.groupBox12.Controls.Add(this.button7);
         this.groupBox12.Controls.Add(this.label1);
         this.groupBox12.Location = new System.Drawing.Point(3, 274);
         this.groupBox12.Name = "groupBox12";
         this.groupBox12.Size = new System.Drawing.Size(202, 83);
         this.groupBox12.TabIndex = 16;
         this.groupBox12.TabStop = false;
         this.groupBox12.Text = "Scale mask";
         // 
         // ScaleAddCheckBox
         // 
         this.ScaleAddCheckBox.AutoSize = true;
         this.ScaleAddCheckBox.Location = new System.Drawing.Point(112, 23);
         this.ScaleAddCheckBox.Name = "ScaleAddCheckBox";
         this.ScaleAddCheckBox.Size = new System.Drawing.Size(90, 17);
         this.ScaleAddCheckBox.TabIndex = 18;
         this.ScaleAddCheckBox.Text = "Add/Subtract";
         this.ScaleAddCheckBox.UseVisualStyleBackColor = true;
         // 
         // ScaleAmountSliderControl
         // 
         this.ScaleAmountSliderControl.BackColor = System.Drawing.SystemColors.ControlLight;
         this.ScaleAmountSliderControl.Location = new System.Drawing.Point(6, 48);
         this.ScaleAmountSliderControl.Name = "ScaleAmountSliderControl";
         this.ScaleAmountSliderControl.Size = new System.Drawing.Size(194, 22);
         this.ScaleAmountSliderControl.TabIndex = 17;
         // 
         // button8
         // 
         this.button8.Location = new System.Drawing.Point(60, 19);
         this.button8.Name = "button8";
         this.button8.Size = new System.Drawing.Size(50, 23);
         this.button8.TabIndex = 16;
         this.button8.Text = "Lighten";
         this.button8.UseVisualStyleBackColor = true;
         this.button8.Click += new System.EventHandler(this.button8_Click);
         // 
         // button7
         // 
         this.button7.Location = new System.Drawing.Point(6, 19);
         this.button7.Name = "button7";
         this.button7.Size = new System.Drawing.Size(53, 23);
         this.button7.TabIndex = 10;
         this.button7.Text = "Darken";
         this.button7.UseVisualStyleBackColor = true;
         this.button7.Click += new System.EventHandler(this.button7_Click);
         // 
         // label1
         // 
         this.label1.AutoSize = true;
         this.label1.Location = new System.Drawing.Point(94, 24);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(0, 13);
         this.label1.TabIndex = 15;
         // 
         // groupBox8
         // 
         this.groupBox8.Controls.Add(this.scaleToFitCheckbox);
         this.groupBox8.Controls.Add(this.ToFileButton);
         this.groupBox8.Controls.Add(this.ToClipboardButton);
         this.groupBox8.Controls.Add(this.FromClipboardButton);
         this.groupBox8.Controls.Add(this.ImportMaskButton);
         this.groupBox8.Location = new System.Drawing.Point(3, 796);
         this.groupBox8.Name = "groupBox8";
         this.groupBox8.Size = new System.Drawing.Size(200, 115);
         this.groupBox8.TabIndex = 15;
         this.groupBox8.TabStop = false;
         this.groupBox8.Text = "Import Mask";
         // 
         // scaleToFitCheckbox
         // 
         this.scaleToFitCheckbox.AutoSize = true;
         this.scaleToFitCheckbox.Location = new System.Drawing.Point(8, 77);
         this.scaleToFitCheckbox.Name = "scaleToFitCheckbox";
         this.scaleToFitCheckbox.Size = new System.Drawing.Size(115, 17);
         this.scaleToFitCheckbox.TabIndex = 4;
         this.scaleToFitCheckbox.Text = "Scale To Fit Import";
         this.scaleToFitCheckbox.UseVisualStyleBackColor = true;
         this.scaleToFitCheckbox.CheckedChanged += new System.EventHandler(this.checkBox1_CheckedChanged);
         // 
         // ToFileButton
         // 
         this.ToFileButton.Location = new System.Drawing.Point(8, 48);
         this.ToFileButton.Name = "ToFileButton";
         this.ToFileButton.Size = new System.Drawing.Size(75, 23);
         this.ToFileButton.TabIndex = 3;
         this.ToFileButton.Text = "To File";
         this.ToFileButton.UseVisualStyleBackColor = true;
         this.ToFileButton.Click += new System.EventHandler(this.ToFileButton_Click);
         // 
         // ToClipboardButton
         // 
         this.ToClipboardButton.Location = new System.Drawing.Point(102, 48);
         this.ToClipboardButton.Name = "ToClipboardButton";
         this.ToClipboardButton.Size = new System.Drawing.Size(88, 23);
         this.ToClipboardButton.TabIndex = 2;
         this.ToClipboardButton.Text = "To Clipboard";
         this.ToClipboardButton.UseVisualStyleBackColor = true;
         this.ToClipboardButton.Click += new System.EventHandler(this.ToClipboardButton_Click);
         // 
         // FromClipboardButton
         // 
         this.FromClipboardButton.Location = new System.Drawing.Point(102, 19);
         this.FromClipboardButton.Name = "FromClipboardButton";
         this.FromClipboardButton.Size = new System.Drawing.Size(88, 23);
         this.FromClipboardButton.TabIndex = 1;
         this.FromClipboardButton.Text = "From Clipboard";
         this.FromClipboardButton.UseVisualStyleBackColor = true;
         this.FromClipboardButton.Click += new System.EventHandler(this.FromClipboardButton_Click);
         // 
         // ImportMaskButton
         // 
         this.ImportMaskButton.Location = new System.Drawing.Point(6, 19);
         this.ImportMaskButton.Name = "ImportMaskButton";
         this.ImportMaskButton.Size = new System.Drawing.Size(78, 23);
         this.ImportMaskButton.TabIndex = 0;
         this.ImportMaskButton.Text = "From File";
         this.ImportMaskButton.UseVisualStyleBackColor = true;
         this.ImportMaskButton.Click += new System.EventHandler(this.ImportMaskButton_Click);
         // 
         // groupBox9
         // 
         this.groupBox9.Controls.Add(this.DeleteMaskButton);
         this.groupBox9.Controls.Add(this.SetAsBaseButton);
         this.groupBox9.Controls.Add(this.CurrentMaskNameTextBox);
         this.groupBox9.Controls.Add(this.ClearCurrentButton);
         this.groupBox9.Controls.Add(this.SaveCurrentButton);
         this.groupBox9.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
         this.groupBox9.Location = new System.Drawing.Point(3, 4);
         this.groupBox9.Name = "groupBox9";
         this.groupBox9.Size = new System.Drawing.Size(229, 81);
         this.groupBox9.TabIndex = 17;
         this.groupBox9.TabStop = false;
         this.groupBox9.Text = "Current Mask";
         // 
         // DeleteMaskButton
         // 
         this.DeleteMaskButton.Location = new System.Drawing.Point(95, 44);
         this.DeleteMaskButton.Name = "DeleteMaskButton";
         this.DeleteMaskButton.Size = new System.Drawing.Size(47, 23);
         this.DeleteMaskButton.TabIndex = 5;
         this.DeleteMaskButton.Text = "Delete";
         this.DeleteMaskButton.UseVisualStyleBackColor = true;
         this.DeleteMaskButton.Click += new System.EventHandler(this.DeleteMaskButton_Click);
         // 
         // SetAsBaseButton
         // 
         this.SetAsBaseButton.Location = new System.Drawing.Point(142, 44);
         this.SetAsBaseButton.Name = "SetAsBaseButton";
         this.SetAsBaseButton.Size = new System.Drawing.Size(81, 23);
         this.SetAsBaseButton.TabIndex = 4;
         this.SetAsBaseButton.Text = "Set as Base";
         this.SetAsBaseButton.UseVisualStyleBackColor = true;
         this.SetAsBaseButton.Click += new System.EventHandler(this.SetAsBaseButton_Click);
         // 
         // CurrentMaskNameTextBox
         // 
         this.CurrentMaskNameTextBox.Location = new System.Drawing.Point(6, 19);
         this.CurrentMaskNameTextBox.Name = "CurrentMaskNameTextBox";
         this.CurrentMaskNameTextBox.Size = new System.Drawing.Size(217, 20);
         this.CurrentMaskNameTextBox.TabIndex = 3;
         // 
         // ClearCurrentButton
         // 
         this.ClearCurrentButton.Location = new System.Drawing.Point(50, 44);
         this.ClearCurrentButton.Name = "ClearCurrentButton";
         this.ClearCurrentButton.Size = new System.Drawing.Size(42, 23);
         this.ClearCurrentButton.TabIndex = 1;
         this.ClearCurrentButton.Text = "Clear";
         this.ClearCurrentButton.UseVisualStyleBackColor = true;
         this.ClearCurrentButton.Click += new System.EventHandler(this.ClearCurrentButton_Click);
         // 
         // SaveCurrentButton
         // 
         this.SaveCurrentButton.Location = new System.Drawing.Point(6, 44);
         this.SaveCurrentButton.Name = "SaveCurrentButton";
         this.SaveCurrentButton.Size = new System.Drawing.Size(40, 23);
         this.SaveCurrentButton.TabIndex = 0;
         this.SaveCurrentButton.Text = "Save";
         this.SaveCurrentButton.UseVisualStyleBackColor = true;
         this.SaveCurrentButton.Click += new System.EventHandler(this.SaveCurrentButton_Click);
         // 
         // groupBox10
         // 
         this.groupBox10.Controls.Add(this.pictureBox1);
         this.groupBox10.Controls.Add(this.SetAsCurrentButton);
         this.groupBox10.Controls.Add(this.ClearBaseButton);
         this.groupBox10.Location = new System.Drawing.Point(6, 227);
         this.groupBox10.Name = "groupBox10";
         this.groupBox10.Size = new System.Drawing.Size(205, 65);
         this.groupBox10.TabIndex = 18;
         this.groupBox10.TabStop = false;
         this.groupBox10.Text = "Base Mask (Mask for Masking)";
         // 
         // pictureBox1
         // 
         this.pictureBox1.BackColor = System.Drawing.SystemColors.Info;
         this.pictureBox1.Location = new System.Drawing.Point(7, 19);
         this.pictureBox1.Name = "pictureBox1";
         this.pictureBox1.Size = new System.Drawing.Size(42, 40);
         this.pictureBox1.TabIndex = 4;
         this.pictureBox1.TabStop = false;
         // 
         // SetAsCurrentButton
         // 
         this.SetAsCurrentButton.Location = new System.Drawing.Point(103, 19);
         this.SetAsCurrentButton.Name = "SetAsCurrentButton";
         this.SetAsCurrentButton.Size = new System.Drawing.Size(90, 23);
         this.SetAsCurrentButton.TabIndex = 3;
         this.SetAsCurrentButton.Text = "Set as Current";
         this.SetAsCurrentButton.UseVisualStyleBackColor = true;
         this.SetAsCurrentButton.Click += new System.EventHandler(this.SetAsCurrentButton_Click);
         // 
         // ClearBaseButton
         // 
         this.ClearBaseButton.Location = new System.Drawing.Point(55, 19);
         this.ClearBaseButton.Name = "ClearBaseButton";
         this.ClearBaseButton.Size = new System.Drawing.Size(42, 23);
         this.ClearBaseButton.TabIndex = 2;
         this.ClearBaseButton.Text = "Clear";
         this.ClearBaseButton.UseVisualStyleBackColor = true;
         this.ClearBaseButton.Click += new System.EventHandler(this.ClearBaseButton_Click);
         // 
         // button9
         // 
         this.button9.Location = new System.Drawing.Point(9, 298);
         this.button9.Name = "button9";
         this.button9.Size = new System.Drawing.Size(202, 23);
         this.button9.TabIndex = 19;
         this.button9.Text = "Mask Graph Generator";
         this.button9.UseVisualStyleBackColor = true;
         this.button9.Click += new System.EventHandler(this.button9_Click);
         // 
         // MaskLayers
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.button9);
         this.Controls.Add(this.groupBox10);
         this.Controls.Add(this.groupBox9);
         this.Controls.Add(this.panel1);
         this.Controls.Add(this.MaskCheckList);
         this.Key = "MaskLayers";
         this.Name = "MaskLayers";
         this.Size = new System.Drawing.Size(239, 952);
         this.Text = "MaskLayers";
         this.MaskCheckListMenu.ResumeLayout(false);
         this.groupBox1.ResumeLayout(false);
         this.groupBox2.ResumeLayout(false);
         this.groupBox2.PerformLayout();
         ((System.ComponentModel.ISupportInitialize)(this.trackBar1)).EndInit();
         this.groupBox3.ResumeLayout(false);
         this.groupBox3.PerformLayout();
         ((System.ComponentModel.ISupportInitialize)(this.blurSigmaTrackbar)).EndInit();
         this.groupBox4.ResumeLayout(false);
         this.groupBox4.PerformLayout();
         this.groupBox11.ResumeLayout(false);
         this.groupBox11.PerformLayout();
         this.groupBox7.ResumeLayout(false);
         this.groupBox7.PerformLayout();
         this.groupBox6.ResumeLayout(false);
         this.groupBox6.PerformLayout();
         this.groupBox5.ResumeLayout(false);
         this.groupBox5.PerformLayout();
         this.panel1.ResumeLayout(false);
         this.groupBox13.ResumeLayout(false);
         this.groupBox13.PerformLayout();
         this.groupBox12.ResumeLayout(false);
         this.groupBox12.PerformLayout();
         this.groupBox8.ResumeLayout(false);
         this.groupBox8.PerformLayout();
         this.groupBox9.ResumeLayout(false);
         this.groupBox9.PerformLayout();
         this.groupBox10.ResumeLayout(false);
         ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).EndInit();
         this.ResumeLayout(false);

      }

      #endregion

      private System.Windows.Forms.CheckedListBox MaskCheckList;
      private System.Windows.Forms.Button button1;
      private System.Windows.Forms.Button button2;
      private System.Windows.Forms.Button button3;
      private System.Windows.Forms.Button button4;
      private System.Windows.Forms.Button button5;
      private System.Windows.Forms.Button button6;
      private System.Windows.Forms.GroupBox groupBox1;
      private System.Windows.Forms.GroupBox groupBox2;
      private System.Windows.Forms.GroupBox groupBox3;
      private System.Windows.Forms.Label vertLabel;
      private System.Windows.Forms.TrackBar trackBar1;
      private System.Windows.Forms.Label blurSigmaLabel;
      private System.Windows.Forms.TrackBar blurSigmaTrackbar;
      private System.Windows.Forms.GroupBox groupBox4;
      private System.Windows.Forms.Button SampleSlopeButton;
      private System.Windows.Forms.CheckBox UseMinHeightCheckBox;
      private System.Windows.Forms.CheckBox UseSlopeCheckBox;
      private System.Windows.Forms.GroupBox groupBox7;
      private System.Windows.Forms.CheckBox UseMaxHeightCheckBox;
      private System.Windows.Forms.GroupBox groupBox6;
      private System.Windows.Forms.GroupBox groupBox5;
      private System.Windows.Forms.Panel panel1;
      private System.Windows.Forms.Button SampleMaxHeightButton;
      private System.Windows.Forms.Button SampleMinHeightButton;
      private System.Windows.Forms.CheckBox MinHeightCheckBox;
      private System.Windows.Forms.CheckBox MinNoiseCheckBox;
      private System.Windows.Forms.CheckBox PostSmoothCheckBox;
      private System.Windows.Forms.CheckBox MaxHeightCheckBox;
      private System.Windows.Forms.CheckBox MaxNoiseCheckBox;
      private System.Windows.Forms.GroupBox groupBox8;
      private System.Windows.Forms.Button ImportMaskButton;
      private EditorCore.NumericSliderControl SlopeSelectSliderControl;
      private System.Windows.Forms.GroupBox groupBox9;
      private System.Windows.Forms.Button SaveCurrentButton;
      private System.Windows.Forms.TextBox CurrentMaskNameTextBox;
      private System.Windows.Forms.Button ClearCurrentButton;
      private System.Windows.Forms.GroupBox groupBox10;
      private System.Windows.Forms.Button ClearBaseButton;
      private System.Windows.Forms.Button SetAsBaseButton;
      private System.Windows.Forms.Button SetAsCurrentButton;
      private System.Windows.Forms.PictureBox pictureBox1;
      private System.Windows.Forms.Button DeleteMaskButton;
      private System.Windows.Forms.GroupBox groupBox11;
      private System.Windows.Forms.CheckBox checkBox2;
      private System.Windows.Forms.GroupBox groupBox12;
      private System.Windows.Forms.Button button8;
      private System.Windows.Forms.Button button7;
      private System.Windows.Forms.Label label1;
      private System.Windows.Forms.GroupBox groupBox13;
      private System.Windows.Forms.Button MaskTextureButton;
      private System.Windows.Forms.CheckBox ScaleAddCheckBox;
      private EditorCore.NumericSliderControl ScaleAmountSliderControl;
      private System.Windows.Forms.CheckBox DisplacementFadeCheckBox;
      private System.Windows.Forms.Button FromClipboardButton;
      private System.Windows.Forms.Button ToClipboardButton;
      private System.Windows.Forms.Button ToFileButton;
      private System.Windows.Forms.CheckBox scaleToFitCheckbox;
      private System.Windows.Forms.Button button9;
      private System.Windows.Forms.ContextMenuStrip MaskCheckListMenu;
      private System.Windows.Forms.ToolStripMenuItem previewToolStripMenuItem;
   }
}