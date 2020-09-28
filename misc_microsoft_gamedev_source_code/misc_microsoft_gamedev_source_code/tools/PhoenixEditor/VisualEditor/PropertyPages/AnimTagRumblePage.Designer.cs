namespace VisualEditor.PropertyPages
{
   partial class AnimTagRumblePage
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
         System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(AnimTagRumblePage));
         this.groupBox1 = new System.Windows.Forms.GroupBox();
         this.label5 = new System.Windows.Forms.Label();
         this.loopCheckBox = new System.Windows.Forms.CheckBox();
         this.rightRumbleComboBox = new System.Windows.Forms.ComboBox();
         this.label4 = new System.Windows.Forms.Label();
         this.leftRumbleComboBox = new System.Windows.Forms.ComboBox();
         this.toBoneLabel = new System.Windows.Forms.Label();
         this.RefreshButton = new System.Windows.Forms.Button();
         this.label1 = new System.Windows.Forms.Label();
         this.label3 = new System.Windows.Forms.Label();
         this.label2 = new System.Windows.Forms.Label();
         this.selectedCheckBox = new System.Windows.Forms.CheckBox();
         this.rightStrengthSliderEdit = new VisualEditor.Controls.FloatSliderEdit();
         this.durationSliderEdit = new VisualEditor.Controls.FloatSliderEdit();
         this.leftStrengthSliderEdit = new VisualEditor.Controls.FloatSliderEdit();
         this.positionSliderEdit = new VisualEditor.Controls.FloatSliderEdit();
         this.groupBox1.SuspendLayout();
         this.SuspendLayout();
         // 
         // groupBox1
         // 
         this.groupBox1.Controls.Add(this.selectedCheckBox);
         this.groupBox1.Controls.Add(this.rightStrengthSliderEdit);
         this.groupBox1.Controls.Add(this.label5);
         this.groupBox1.Controls.Add(this.loopCheckBox);
         this.groupBox1.Controls.Add(this.rightRumbleComboBox);
         this.groupBox1.Controls.Add(this.label4);
         this.groupBox1.Controls.Add(this.leftRumbleComboBox);
         this.groupBox1.Controls.Add(this.toBoneLabel);
         this.groupBox1.Controls.Add(this.RefreshButton);
         this.groupBox1.Controls.Add(this.durationSliderEdit);
         this.groupBox1.Controls.Add(this.leftStrengthSliderEdit);
         this.groupBox1.Controls.Add(this.label1);
         this.groupBox1.Controls.Add(this.label3);
         this.groupBox1.Controls.Add(this.positionSliderEdit);
         this.groupBox1.Controls.Add(this.label2);
         this.groupBox1.Location = new System.Drawing.Point(0, 0);
         this.groupBox1.Name = "groupBox1";
         this.groupBox1.Padding = new System.Windows.Forms.Padding(5, 16, 5, 3);
         this.groupBox1.Size = new System.Drawing.Size(350, 250);
         this.groupBox1.TabIndex = 0;
         this.groupBox1.TabStop = false;
         this.groupBox1.Text = "Rumble Tag Properties";
         // 
         // label5
         // 
         this.label5.AutoSize = true;
         this.label5.Location = new System.Drawing.Point(8, 172);
         this.label5.Margin = new System.Windows.Forms.Padding(3);
         this.label5.Name = "label5";
         this.label5.Size = new System.Drawing.Size(75, 13);
         this.label5.TabIndex = 35;
         this.label5.Text = "Right Strength";
         // 
         // loopCheckBox
         // 
         this.loopCheckBox.AutoSize = true;
         this.loopCheckBox.Location = new System.Drawing.Point(11, 227);
         this.loopCheckBox.Name = "loopCheckBox";
         this.loopCheckBox.Size = new System.Drawing.Size(50, 17);
         this.loopCheckBox.TabIndex = 34;
         this.loopCheckBox.Text = "Loop";
         this.loopCheckBox.UseVisualStyleBackColor = true;
         this.loopCheckBox.CheckedChanged += new System.EventHandler(this.loopCheckBox_CheckedChanged);
         // 
         // rightRumbleComboBox
         // 
         this.rightRumbleComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
         this.rightRumbleComboBox.FormattingEnabled = true;
         this.rightRumbleComboBox.Location = new System.Drawing.Point(102, 145);
         this.rightRumbleComboBox.Name = "rightRumbleComboBox";
         this.rightRumbleComboBox.Size = new System.Drawing.Size(240, 21);
         this.rightRumbleComboBox.TabIndex = 33;
         this.rightRumbleComboBox.SelectedIndexChanged += new System.EventHandler(this.rightRumbleComboBox_SelectedIndexChanged);
         // 
         // label4
         // 
         this.label4.AutoSize = true;
         this.label4.Location = new System.Drawing.Point(8, 147);
         this.label4.Margin = new System.Windows.Forms.Padding(3);
         this.label4.Name = "label4";
         this.label4.Size = new System.Drawing.Size(71, 13);
         this.label4.TabIndex = 32;
         this.label4.Text = "Right Rumble";
         this.label4.Click += new System.EventHandler(this.label4_Click);
         // 
         // leftRumbleComboBox
         // 
         this.leftRumbleComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
         this.leftRumbleComboBox.FormattingEnabled = true;
         this.leftRumbleComboBox.Location = new System.Drawing.Point(102, 75);
         this.leftRumbleComboBox.Name = "leftRumbleComboBox";
         this.leftRumbleComboBox.Size = new System.Drawing.Size(240, 21);
         this.leftRumbleComboBox.TabIndex = 31;
         this.leftRumbleComboBox.SelectedIndexChanged += new System.EventHandler(this.leftRumbleComboBox_SelectedIndexChanged);
         // 
         // toBoneLabel
         // 
         this.toBoneLabel.AutoSize = true;
         this.toBoneLabel.Location = new System.Drawing.Point(8, 77);
         this.toBoneLabel.Margin = new System.Windows.Forms.Padding(3);
         this.toBoneLabel.Name = "toBoneLabel";
         this.toBoneLabel.Size = new System.Drawing.Size(64, 13);
         this.toBoneLabel.TabIndex = 30;
         this.toBoneLabel.Text = "Left Rumble";
         // 
         // RefreshButton
         // 
         this.RefreshButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
         this.RefreshButton.Image = ((System.Drawing.Image)(resources.GetObject("RefreshButton.Image")));
         this.RefreshButton.Location = new System.Drawing.Point(318, 34);
         this.RefreshButton.Margin = new System.Windows.Forms.Padding(0);
         this.RefreshButton.Name = "RefreshButton";
         this.RefreshButton.Size = new System.Drawing.Size(24, 24);
         this.RefreshButton.TabIndex = 2;
         this.RefreshButton.UseVisualStyleBackColor = true;
         this.RefreshButton.Click += new System.EventHandler(this.RefreshButton_Click);
         // 
         // label1
         // 
         this.label1.AutoSize = true;
         this.label1.Location = new System.Drawing.Point(8, 207);
         this.label1.Margin = new System.Windows.Forms.Padding(3);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(47, 13);
         this.label1.TabIndex = 5;
         this.label1.Text = "Duration";
         // 
         // label3
         // 
         this.label3.AutoSize = true;
         this.label3.Location = new System.Drawing.Point(8, 102);
         this.label3.Margin = new System.Windows.Forms.Padding(3);
         this.label3.Name = "label3";
         this.label3.Size = new System.Drawing.Size(68, 13);
         this.label3.TabIndex = 3;
         this.label3.Text = "Left Strength";
         // 
         // label2
         // 
         this.label2.AutoSize = true;
         this.label2.Location = new System.Drawing.Point(8, 34);
         this.label2.Name = "label2";
         this.label2.Size = new System.Drawing.Size(44, 13);
         this.label2.TabIndex = 0;
         this.label2.Text = "Position";
         // 
         // selectedCheckBox
         // 
         this.selectedCheckBox.AutoSize = true;
         this.selectedCheckBox.Location = new System.Drawing.Point(8, 54);
         this.selectedCheckBox.Name = "selectedCheckBox";
         this.selectedCheckBox.Size = new System.Drawing.Size(102, 17);
         this.selectedCheckBox.TabIndex = 37;
         this.selectedCheckBox.Text = "Check Selected";
         this.selectedCheckBox.UseVisualStyleBackColor = true;
         this.selectedCheckBox.CheckedChanged += new System.EventHandler(this.selectedCheckBox_CheckedChanged);
         // 
         // rightStrengthSliderEdit
         // 
         this.rightStrengthSliderEdit.Location = new System.Drawing.Point(102, 172);
         this.rightStrengthSliderEdit.Margin = new System.Windows.Forms.Padding(3, 3, 3, 0);
         this.rightStrengthSliderEdit.MaxValue = 1F;
         this.rightStrengthSliderEdit.MinValue = 0F;
         this.rightStrengthSliderEdit.Name = "rightStrengthSliderEdit";
         this.rightStrengthSliderEdit.NumDecimals = 3;
         this.rightStrengthSliderEdit.Size = new System.Drawing.Size(240, 40);
         this.rightStrengthSliderEdit.TabIndex = 36;
         this.rightStrengthSliderEdit.Value = 1F;
         this.rightStrengthSliderEdit.ValueChanged += new VisualEditor.Controls.FloatSliderEdit.ValueChangedDelegate(this.forceSliderEdit_ValueChanged);
         // 
         // durationSliderEdit
         // 
         this.durationSliderEdit.Location = new System.Drawing.Point(102, 207);
         this.durationSliderEdit.Margin = new System.Windows.Forms.Padding(3, 3, 3, 0);
         this.durationSliderEdit.MaxValue = 5F;
         this.durationSliderEdit.MinValue = 0F;
         this.durationSliderEdit.Name = "durationSliderEdit";
         this.durationSliderEdit.NumDecimals = 3;
         this.durationSliderEdit.Size = new System.Drawing.Size(240, 40);
         this.durationSliderEdit.TabIndex = 6;
         this.durationSliderEdit.Value = 0.25F;
         this.durationSliderEdit.ValueChanged += new VisualEditor.Controls.FloatSliderEdit.ValueChangedDelegate(this.durationSliderEdit_ValueChanged);
         // 
         // leftStrengthSliderEdit
         // 
         this.leftStrengthSliderEdit.Location = new System.Drawing.Point(102, 102);
         this.leftStrengthSliderEdit.Margin = new System.Windows.Forms.Padding(3, 3, 3, 0);
         this.leftStrengthSliderEdit.MaxValue = 1F;
         this.leftStrengthSliderEdit.MinValue = 0F;
         this.leftStrengthSliderEdit.Name = "leftStrengthSliderEdit";
         this.leftStrengthSliderEdit.NumDecimals = 3;
         this.leftStrengthSliderEdit.Size = new System.Drawing.Size(240, 40);
         this.leftStrengthSliderEdit.TabIndex = 4;
         this.leftStrengthSliderEdit.Value = 1F;
         this.leftStrengthSliderEdit.ValueChanged += new VisualEditor.Controls.FloatSliderEdit.ValueChangedDelegate(this.forceSliderEdit_ValueChanged);
         // 
         // positionSliderEdit
         // 
         this.positionSliderEdit.Location = new System.Drawing.Point(102, 32);
         this.positionSliderEdit.Margin = new System.Windows.Forms.Padding(3, 3, 3, 0);
         this.positionSliderEdit.MaxValue = 1F;
         this.positionSliderEdit.MinValue = 0F;
         this.positionSliderEdit.Name = "positionSliderEdit";
         this.positionSliderEdit.NumDecimals = 3;
         this.positionSliderEdit.Size = new System.Drawing.Size(213, 40);
         this.positionSliderEdit.TabIndex = 1;
         this.positionSliderEdit.Value = 0F;
         this.positionSliderEdit.ValueChanged += new VisualEditor.Controls.FloatSliderEdit.ValueChangedDelegate(this.positionSliderEdit_ValueChanged);
         // 
         // AnimTagRumblePage
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.groupBox1);
         this.Name = "AnimTagRumblePage";
         this.Size = new System.Drawing.Size(350, 250);
         this.groupBox1.ResumeLayout(false);
         this.groupBox1.PerformLayout();
         this.ResumeLayout(false);

      }

      #endregion

      private System.Windows.Forms.GroupBox groupBox1;
      private VisualEditor.Controls.FloatSliderEdit positionSliderEdit;
      private System.Windows.Forms.Label label2;
      private VisualEditor.Controls.FloatSliderEdit durationSliderEdit;
      private VisualEditor.Controls.FloatSliderEdit leftStrengthSliderEdit;
      private System.Windows.Forms.Label label1;
      private System.Windows.Forms.Label label3;
      private System.Windows.Forms.Button RefreshButton;
      private System.Windows.Forms.ComboBox leftRumbleComboBox;
      private System.Windows.Forms.Label toBoneLabel;
      private System.Windows.Forms.ComboBox rightRumbleComboBox;
      private System.Windows.Forms.Label label4;
      private System.Windows.Forms.CheckBox loopCheckBox;
      private VisualEditor.Controls.FloatSliderEdit rightStrengthSliderEdit;
      private System.Windows.Forms.Label label5;
      private System.Windows.Forms.CheckBox selectedCheckBox;
   }
}
