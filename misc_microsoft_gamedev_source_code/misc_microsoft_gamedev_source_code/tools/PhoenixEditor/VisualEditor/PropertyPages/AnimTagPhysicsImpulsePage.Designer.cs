namespace VisualEditor.PropertyPages
{
   partial class AnimTagPhysicsImpulsePage
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
         System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(AnimTagPhysicsImpulsePage));
         this.groupBox1 = new System.Windows.Forms.GroupBox();
         this.zlabel = new System.Windows.Forms.Label();
         this.ylabel = new System.Windows.Forms.Label();
         this.xlabel1 = new System.Windows.Forms.Label();
         this.attachedCheckBox = new System.Windows.Forms.CheckBox();
         this.zTextBox = new System.Windows.Forms.TextBox();
         this.yTextBox = new System.Windows.Forms.TextBox();
         this.xTextBox = new System.Windows.Forms.TextBox();
         this.typeComboBox = new System.Windows.Forms.ComboBox();
         this.typelabel = new System.Windows.Forms.Label();
         this.toBoneComboBox = new System.Windows.Forms.ComboBox();
         this.toBoneLabel = new System.Windows.Forms.Label();
         this.RefreshButton = new System.Windows.Forms.Button();
         this.floatSliderEdit1 = new VisualEditor.Controls.FloatSliderEdit();
         this.label2 = new System.Windows.Forms.Label();
         this.groupBox1.SuspendLayout();
         this.SuspendLayout();
         // 
         // groupBox1
         // 
         this.groupBox1.Controls.Add(this.zlabel);
         this.groupBox1.Controls.Add(this.ylabel);
         this.groupBox1.Controls.Add(this.xlabel1);
         this.groupBox1.Controls.Add(this.attachedCheckBox);
         this.groupBox1.Controls.Add(this.zTextBox);
         this.groupBox1.Controls.Add(this.yTextBox);
         this.groupBox1.Controls.Add(this.xTextBox);
         this.groupBox1.Controls.Add(this.typeComboBox);
         this.groupBox1.Controls.Add(this.typelabel);
         this.groupBox1.Controls.Add(this.toBoneComboBox);
         this.groupBox1.Controls.Add(this.toBoneLabel);
         this.groupBox1.Controls.Add(this.RefreshButton);
         this.groupBox1.Controls.Add(this.floatSliderEdit1);
         this.groupBox1.Controls.Add(this.label2);
         this.groupBox1.Location = new System.Drawing.Point(0, 0);
         this.groupBox1.Name = "groupBox1";
         this.groupBox1.Padding = new System.Windows.Forms.Padding(5, 16, 5, 3);
         this.groupBox1.Size = new System.Drawing.Size(350, 250);
         this.groupBox1.TabIndex = 2;
         this.groupBox1.TabStop = false;
         this.groupBox1.Text = "Physics Impulse Tag Properties";
         this.groupBox1.Enter += new System.EventHandler(this.groupBox1_Enter);
         // 
         // zlabel
         // 
         this.zlabel.AutoSize = true;
         this.zlabel.Location = new System.Drawing.Point(8, 184);
         this.zlabel.Name = "zlabel";
         this.zlabel.Size = new System.Drawing.Size(37, 13);
         this.zlabel.TabIndex = 40;
         this.zlabel.Text = "Z/Roll";
         // 
         // ylabel
         // 
         this.ylabel.AutoSize = true;
         this.ylabel.Location = new System.Drawing.Point(8, 158);
         this.ylabel.Name = "ylabel";
         this.ylabel.Size = new System.Drawing.Size(43, 13);
         this.ylabel.TabIndex = 39;
         this.ylabel.Text = "Y/Pitch";
         // 
         // xlabel1
         // 
         this.xlabel1.AutoSize = true;
         this.xlabel1.Location = new System.Drawing.Point(8, 132);
         this.xlabel1.Name = "xlabel1";
         this.xlabel1.Size = new System.Drawing.Size(40, 13);
         this.xlabel1.TabIndex = 38;
         this.xlabel1.Text = "X/Yaw";
         // 
         // attachedCheckBox
         // 
         this.attachedCheckBox.AutoSize = true;
         this.attachedCheckBox.Location = new System.Drawing.Point(8, 207);
         this.attachedCheckBox.Name = "attachedCheckBox";
         this.attachedCheckBox.RightToLeft = System.Windows.Forms.RightToLeft.Yes;
         this.attachedCheckBox.Size = new System.Drawing.Size(136, 17);
         this.attachedCheckBox.TabIndex = 37;
         this.attachedCheckBox.Text = "Apply To Attached Unit";
         this.attachedCheckBox.TextAlign = System.Drawing.ContentAlignment.TopLeft;
         this.attachedCheckBox.UseVisualStyleBackColor = true;
         this.attachedCheckBox.CheckedChanged += new System.EventHandler(this.toBoneComboBox_SelectedIndexChanged);
         // 
         // zTextBox
         // 
         this.zTextBox.Location = new System.Drawing.Point(102, 181);
         this.zTextBox.Name = "zTextBox";
         this.zTextBox.Size = new System.Drawing.Size(100, 20);
         this.zTextBox.TabIndex = 36;
         this.zTextBox.TextChanged += new System.EventHandler(this.toBoneComboBox_SelectedIndexChanged);
         // 
         // yTextBox
         // 
         this.yTextBox.Location = new System.Drawing.Point(102, 155);
         this.yTextBox.Name = "yTextBox";
         this.yTextBox.Size = new System.Drawing.Size(100, 20);
         this.yTextBox.TabIndex = 35;
         this.yTextBox.TextChanged += new System.EventHandler(this.toBoneComboBox_SelectedIndexChanged);
         // 
         // xTextBox
         // 
         this.xTextBox.Location = new System.Drawing.Point(102, 129);
         this.xTextBox.Name = "xTextBox";
         this.xTextBox.Size = new System.Drawing.Size(100, 20);
         this.xTextBox.TabIndex = 34;
         this.xTextBox.TextChanged += new System.EventHandler(this.toBoneComboBox_SelectedIndexChanged);
         // 
         // typeComboBox
         // 
         this.typeComboBox.FormattingEnabled = true;
         this.typeComboBox.Items.AddRange(new object[] {
            "Angular",
            "Linear",
            "Point"});
         this.typeComboBox.Location = new System.Drawing.Point(102, 102);
         this.typeComboBox.Name = "typeComboBox";
         this.typeComboBox.Size = new System.Drawing.Size(121, 21);
         this.typeComboBox.TabIndex = 33;
         this.typeComboBox.SelectedIndexChanged += new System.EventHandler(this.toBoneComboBox_SelectedIndexChanged);
         // 
         // typelabel
         // 
         this.typelabel.AutoSize = true;
         this.typelabel.Location = new System.Drawing.Point(8, 105);
         this.typelabel.Name = "typelabel";
         this.typelabel.Size = new System.Drawing.Size(31, 13);
         this.typelabel.TabIndex = 32;
         this.typelabel.Text = "Type";
         // 
         // toBoneComboBox
         // 
         this.toBoneComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
         this.toBoneComboBox.FormattingEnabled = true;
         this.toBoneComboBox.Location = new System.Drawing.Point(102, 75);
         this.toBoneComboBox.Name = "toBoneComboBox";
         this.toBoneComboBox.Size = new System.Drawing.Size(240, 21);
         this.toBoneComboBox.TabIndex = 31;
         this.toBoneComboBox.SelectedIndexChanged += new System.EventHandler(this.toBoneComboBox_SelectedIndexChanged);
         // 
         // toBoneLabel
         // 
         this.toBoneLabel.AutoSize = true;
         this.toBoneLabel.Location = new System.Drawing.Point(8, 77);
         this.toBoneLabel.Margin = new System.Windows.Forms.Padding(3);
         this.toBoneLabel.Name = "toBoneLabel";
         this.toBoneLabel.Size = new System.Drawing.Size(48, 13);
         this.toBoneLabel.TabIndex = 30;
         this.toBoneLabel.Text = "To Bone";
         // 
         // RefreshButton
         // 
         this.RefreshButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
         this.RefreshButton.Image = ((System.Drawing.Image)(resources.GetObject("RefreshButton.Image")));
         this.RefreshButton.Location = new System.Drawing.Point(321, 34);
         this.RefreshButton.Margin = new System.Windows.Forms.Padding(0);
         this.RefreshButton.Name = "RefreshButton";
         this.RefreshButton.Size = new System.Drawing.Size(24, 24);
         this.RefreshButton.TabIndex = 16;
         this.RefreshButton.UseVisualStyleBackColor = true;
         this.RefreshButton.Click += new System.EventHandler(this.RefreshButton_Click);
         // 
         // floatSliderEdit1
         // 
         this.floatSliderEdit1.Location = new System.Drawing.Point(102, 32);
         this.floatSliderEdit1.Margin = new System.Windows.Forms.Padding(3, 3, 3, 0);
         this.floatSliderEdit1.MaxValue = 1F;
         this.floatSliderEdit1.MinValue = 0F;
         this.floatSliderEdit1.Name = "floatSliderEdit1";
         this.floatSliderEdit1.NumDecimals = 3;
         this.floatSliderEdit1.Size = new System.Drawing.Size(216, 40);
         this.floatSliderEdit1.TabIndex = 4;
         this.floatSliderEdit1.Value = 0F;
         this.floatSliderEdit1.ValueChanged += new VisualEditor.Controls.FloatSliderEdit.ValueChangedDelegate(this.floatSliderEdit1_ValueChanged);
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
         // AnimTagPhysicsImpulsePage
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.groupBox1);
         this.Name = "AnimTagPhysicsImpulsePage";
         this.Size = new System.Drawing.Size(350, 250);
         this.groupBox1.ResumeLayout(false);
         this.groupBox1.PerformLayout();
         this.ResumeLayout(false);

      }

      #endregion

      private System.Windows.Forms.GroupBox groupBox1;
      private VisualEditor.Controls.FloatSliderEdit floatSliderEdit1;
      private System.Windows.Forms.Label label2;
      private System.Windows.Forms.Button RefreshButton;
      private System.Windows.Forms.ComboBox toBoneComboBox;
      private System.Windows.Forms.Label toBoneLabel;
      private System.Windows.Forms.ComboBox typeComboBox;
      private System.Windows.Forms.Label typelabel;
      private System.Windows.Forms.Label ylabel;
      private System.Windows.Forms.Label xlabel1;
      private System.Windows.Forms.CheckBox attachedCheckBox;
      private System.Windows.Forms.TextBox zTextBox;
      private System.Windows.Forms.TextBox yTextBox;
      private System.Windows.Forms.TextBox xTextBox;
      private System.Windows.Forms.Label zlabel;
   }
}
