namespace PhoenixEditor
{
   partial class PlacementHelper
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
         this.SnapToVisButton = new System.Windows.Forms.Button();
         this.xBox = new System.Windows.Forms.NumericUpDown();
         this.yBox = new System.Windows.Forms.NumericUpDown();
         this.zBox = new System.Windows.Forms.NumericUpDown();
         this.JoinButton = new System.Windows.Forms.Button();
         this.AlignmentSlider = new EditorCore.NumericSliderControl();
         this.label1 = new System.Windows.Forms.Label();
         this.MatchRotationButton = new System.Windows.Forms.Button();
         this.SetToOriginButton = new System.Windows.Forms.Button();
         this.groupBox1 = new System.Windows.Forms.GroupBox();
         this.SetRotationCheckBox = new System.Windows.Forms.CheckBox();
         ((System.ComponentModel.ISupportInitialize)(this.xBox)).BeginInit();
         ((System.ComponentModel.ISupportInitialize)(this.yBox)).BeginInit();
         ((System.ComponentModel.ISupportInitialize)(this.zBox)).BeginInit();
         this.groupBox1.SuspendLayout();
         this.SuspendLayout();
         // 
         // SnapToVisButton
         // 
         this.SnapToVisButton.Location = new System.Drawing.Point(6, 19);
         this.SnapToVisButton.Name = "SnapToVisButton";
         this.SnapToVisButton.Size = new System.Drawing.Size(78, 23);
         this.SnapToVisButton.TabIndex = 0;
         this.SnapToVisButton.Text = "SnapToVis";
         this.SnapToVisButton.UseVisualStyleBackColor = true;
         this.SnapToVisButton.Click += new System.EventHandler(this.SnapToVisButton_Click);
         // 
         // xBox
         // 
         this.xBox.Enabled = false;
         this.xBox.Location = new System.Drawing.Point(4, 4);
         this.xBox.Name = "xBox";
         this.xBox.Size = new System.Drawing.Size(67, 20);
         this.xBox.TabIndex = 1;
         // 
         // yBox
         // 
         this.yBox.Enabled = false;
         this.yBox.Location = new System.Drawing.Point(2, 30);
         this.yBox.Name = "yBox";
         this.yBox.Size = new System.Drawing.Size(69, 20);
         this.yBox.TabIndex = 2;
         // 
         // zBox
         // 
         this.zBox.Enabled = false;
         this.zBox.Location = new System.Drawing.Point(0, 56);
         this.zBox.Name = "zBox";
         this.zBox.Size = new System.Drawing.Size(69, 20);
         this.zBox.TabIndex = 3;
         // 
         // JoinButton
         // 
         this.JoinButton.Location = new System.Drawing.Point(168, 3);
         this.JoinButton.Name = "JoinButton";
         this.JoinButton.Size = new System.Drawing.Size(101, 23);
         this.JoinButton.TabIndex = 4;
         this.JoinButton.Text = "Merge Positions";
         this.JoinButton.UseVisualStyleBackColor = true;
         this.JoinButton.Click += new System.EventHandler(this.JoinButton_Click);
         // 
         // AlignmentSlider
         // 
         this.AlignmentSlider.BackColor = System.Drawing.SystemColors.ControlLight;
         this.AlignmentSlider.Enabled = false;
         this.AlignmentSlider.Location = new System.Drawing.Point(6, 68);
         this.AlignmentSlider.Name = "AlignmentSlider";
         this.AlignmentSlider.Size = new System.Drawing.Size(178, 22);
         this.AlignmentSlider.TabIndex = 6;
         // 
         // label1
         // 
         this.label1.AutoSize = true;
         this.label1.Location = new System.Drawing.Point(6, 51);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(123, 13);
         this.label1.TabIndex = 7;
         this.label1.Text = "Slope Alignment Percent";
         // 
         // MatchRotationButton
         // 
         this.MatchRotationButton.Location = new System.Drawing.Point(168, 29);
         this.MatchRotationButton.Name = "MatchRotationButton";
         this.MatchRotationButton.Size = new System.Drawing.Size(101, 23);
         this.MatchRotationButton.TabIndex = 8;
         this.MatchRotationButton.Text = "Match Rotation";
         this.MatchRotationButton.UseVisualStyleBackColor = true;
         this.MatchRotationButton.Click += new System.EventHandler(this.MatchRotationButton_Click);
         // 
         // SetToOriginButton
         // 
         this.SetToOriginButton.Location = new System.Drawing.Point(168, 55);
         this.SetToOriginButton.Name = "SetToOriginButton";
         this.SetToOriginButton.Size = new System.Drawing.Size(101, 23);
         this.SetToOriginButton.TabIndex = 9;
         this.SetToOriginButton.Text = "Set To Origin";
         this.SetToOriginButton.UseVisualStyleBackColor = true;
         this.SetToOriginButton.Click += new System.EventHandler(this.SetToOriginButton_Click);
         // 
         // groupBox1
         // 
         this.groupBox1.Controls.Add(this.SetRotationCheckBox);
         this.groupBox1.Controls.Add(this.SnapToVisButton);
         this.groupBox1.Controls.Add(this.AlignmentSlider);
         this.groupBox1.Controls.Add(this.label1);
         this.groupBox1.Location = new System.Drawing.Point(284, 4);
         this.groupBox1.Name = "groupBox1";
         this.groupBox1.Size = new System.Drawing.Size(200, 101);
         this.groupBox1.TabIndex = 10;
         this.groupBox1.TabStop = false;
         this.groupBox1.Text = "Snap";
         // 
         // SetRotationCheckBox
         // 
         this.SetRotationCheckBox.AutoSize = true;
         this.SetRotationCheckBox.Location = new System.Drawing.Point(91, 23);
         this.SetRotationCheckBox.Name = "SetRotationCheckBox";
         this.SetRotationCheckBox.Size = new System.Drawing.Size(85, 17);
         this.SetRotationCheckBox.TabIndex = 8;
         this.SetRotationCheckBox.Text = "Set Rotation";
         this.SetRotationCheckBox.UseVisualStyleBackColor = true;
         this.SetRotationCheckBox.CheckedChanged += new System.EventHandler(this.SetRotationCheckBox_CheckedChanged);
         // 
         // PlacementHelper
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.groupBox1);
         this.Controls.Add(this.SetToOriginButton);
         this.Controls.Add(this.MatchRotationButton);
         this.Controls.Add(this.JoinButton);
         this.Controls.Add(this.zBox);
         this.Controls.Add(this.yBox);
         this.Controls.Add(this.xBox);
         this.Name = "PlacementHelper";
         this.Size = new System.Drawing.Size(609, 108);
         ((System.ComponentModel.ISupportInitialize)(this.xBox)).EndInit();
         ((System.ComponentModel.ISupportInitialize)(this.yBox)).EndInit();
         ((System.ComponentModel.ISupportInitialize)(this.zBox)).EndInit();
         this.groupBox1.ResumeLayout(false);
         this.groupBox1.PerformLayout();
         this.ResumeLayout(false);

      }

      #endregion

      private System.Windows.Forms.Button SnapToVisButton;
      private System.Windows.Forms.NumericUpDown xBox;
      private System.Windows.Forms.NumericUpDown yBox;
      private System.Windows.Forms.NumericUpDown zBox;
      private System.Windows.Forms.Button JoinButton;
      private EditorCore.NumericSliderControl AlignmentSlider;
      private System.Windows.Forms.Label label1;
      private System.Windows.Forms.Button MatchRotationButton;
      private System.Windows.Forms.Button SetToOriginButton;
      private System.Windows.Forms.GroupBox groupBox1;
      private System.Windows.Forms.CheckBox SetRotationCheckBox;
   }
}
