namespace VisualEditor.PropertyPages
{
   partial class AnimTagKillAndThrowPage
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
          this.label1 = new System.Windows.Forms.Label();
          this.sampleLengthTextBox = new System.Windows.Forms.TextBox();
          this.RefreshButton = new System.Windows.Forms.Button();
          this.toBoneComboBox = new System.Windows.Forms.ComboBox();
          this.toBoneLabel = new System.Windows.Forms.Label();
          this.positionSliderEdit = new VisualEditor.Controls.FloatSliderEdit();
          this.label2 = new System.Windows.Forms.Label();
          this.groupBox1.SuspendLayout();
          this.SuspendLayout();
          // 
          // groupBox1
          // 
          this.groupBox1.Controls.Add(this.label1);
          this.groupBox1.Controls.Add(this.sampleLengthTextBox);
          this.groupBox1.Controls.Add(this.RefreshButton);
          this.groupBox1.Controls.Add(this.toBoneComboBox);
          this.groupBox1.Controls.Add(this.toBoneLabel);
          this.groupBox1.Controls.Add(this.positionSliderEdit);
          this.groupBox1.Controls.Add(this.label2);
          this.groupBox1.Location = new System.Drawing.Point(0, 0);
          this.groupBox1.Name = "groupBox1";
          this.groupBox1.Padding = new System.Windows.Forms.Padding(5, 16, 5, 3);
          this.groupBox1.Size = new System.Drawing.Size(350, 250);
          this.groupBox1.TabIndex = 0;
          this.groupBox1.TabStop = false;
          this.groupBox1.Text = "Kill && Throw Tag Properties";
          // 
          // label1
          // 
          this.label1.AutoSize = true;
          this.label1.Location = new System.Drawing.Point(8, 116);
          this.label1.Name = "label1";
          this.label1.Size = new System.Drawing.Size(78, 13);
          this.label1.TabIndex = 6;
          this.label1.Text = "Sample Length";
          // 
          // sampleLengthTextBox
          // 
          this.sampleLengthTextBox.Location = new System.Drawing.Point(102, 113);
          this.sampleLengthTextBox.Name = "sampleLengthTextBox";
          this.sampleLengthTextBox.Size = new System.Drawing.Size(45, 20);
          this.sampleLengthTextBox.TabIndex = 5;
          this.sampleLengthTextBox.Leave += new System.EventHandler(this.sampleLengthTextBox_LeaveFocus);
          // 
          // RefreshButton
          // 
          this.RefreshButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
          this.RefreshButton.Location = new System.Drawing.Point(318, 34);
          this.RefreshButton.Margin = new System.Windows.Forms.Padding(0);
          this.RefreshButton.Name = "RefreshButton";
          this.RefreshButton.Size = new System.Drawing.Size(24, 24);
          this.RefreshButton.TabIndex = 2;
          this.RefreshButton.UseVisualStyleBackColor = true;
          this.RefreshButton.Click += new System.EventHandler(this.RefreshButton_Click);
          // 
          // toBoneComboBox
          // 
          this.toBoneComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
          this.toBoneComboBox.FormattingEnabled = true;
          this.toBoneComboBox.Location = new System.Drawing.Point(102, 75);
          this.toBoneComboBox.Name = "toBoneComboBox";
          this.toBoneComboBox.Size = new System.Drawing.Size(240, 21);
          this.toBoneComboBox.TabIndex = 4;
          this.toBoneComboBox.SelectedIndexChanged += new System.EventHandler(this.toBoneComboBox_SelectedIndexChanged);
          // 
          // toBoneLabel
          // 
          this.toBoneLabel.AutoSize = true;
          this.toBoneLabel.Location = new System.Drawing.Point(8, 77);
          this.toBoneLabel.Margin = new System.Windows.Forms.Padding(3);
          this.toBoneLabel.Name = "toBoneLabel";
          this.toBoneLabel.Size = new System.Drawing.Size(48, 13);
          this.toBoneLabel.TabIndex = 3;
          this.toBoneLabel.Text = "To Bone";
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
          // label2
          // 
          this.label2.AutoSize = true;
          this.label2.Location = new System.Drawing.Point(8, 34);
          this.label2.Name = "label2";
          this.label2.Size = new System.Drawing.Size(44, 13);
          this.label2.TabIndex = 0;
          this.label2.Text = "Position";
          // 
          // AnimTagKillAndThrowPage
          // 
          this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
          this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
          this.Controls.Add(this.groupBox1);
          this.Name = "AnimTagKillAndThrowPage";
          this.Size = new System.Drawing.Size(350, 250);
          this.groupBox1.ResumeLayout(false);
          this.groupBox1.PerformLayout();
          this.ResumeLayout(false);

      }

      #endregion

      private System.Windows.Forms.GroupBox groupBox1;
      private VisualEditor.Controls.FloatSliderEdit positionSliderEdit;
       private System.Windows.Forms.Label label2;
      private System.Windows.Forms.ComboBox toBoneComboBox;
      private System.Windows.Forms.Label toBoneLabel;
      private System.Windows.Forms.Button RefreshButton;
       private System.Windows.Forms.Label label1;
       private System.Windows.Forms.TextBox sampleLengthTextBox;
   }
}
