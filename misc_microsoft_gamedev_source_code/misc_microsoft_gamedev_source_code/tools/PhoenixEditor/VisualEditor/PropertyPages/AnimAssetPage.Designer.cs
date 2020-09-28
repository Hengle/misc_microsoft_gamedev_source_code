using EditorCore;
namespace VisualEditor.PropertyPages
{
   partial class AnimAssetPage
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
         this.opacityProgressionButton = new System.Windows.Forms.Button();
         this.opacityProgressionCheckBox = new System.Windows.Forms.CheckBox();
         this.weightNumericUpDown1 = new System.Windows.Forms.NumericUpDown();
         this.label1 = new System.Windows.Forms.Label();
         this.fileBrowseControl1 = new EditorCore.FileBrowseControl();
         this.label2 = new System.Windows.Forms.Label();
         this.groupBox1.SuspendLayout();
         ((System.ComponentModel.ISupportInitialize)(this.weightNumericUpDown1)).BeginInit();
         this.SuspendLayout();
         // 
         // groupBox1
         // 
         this.groupBox1.Anchor = System.Windows.Forms.AnchorStyles.None;
         this.groupBox1.Controls.Add(this.opacityProgressionButton);
         this.groupBox1.Controls.Add(this.opacityProgressionCheckBox);
         this.groupBox1.Controls.Add(this.weightNumericUpDown1);
         this.groupBox1.Controls.Add(this.label1);
         this.groupBox1.Controls.Add(this.fileBrowseControl1);
         this.groupBox1.Controls.Add(this.label2);
         this.groupBox1.Location = new System.Drawing.Point(0, 0);
         this.groupBox1.Name = "groupBox1";
         this.groupBox1.Padding = new System.Windows.Forms.Padding(5, 16, 5, 3);
         this.groupBox1.Size = new System.Drawing.Size(350, 250);
         this.groupBox1.TabIndex = 0;
         this.groupBox1.TabStop = false;
         this.groupBox1.Text = "Anim Asset Properties";
         // 
         // opacityProgressionButton
         // 
         this.opacityProgressionButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
         this.opacityProgressionButton.Location = new System.Drawing.Point(318, 100);
         this.opacityProgressionButton.Margin = new System.Windows.Forms.Padding(0);
         this.opacityProgressionButton.Name = "opacityProgressionButton";
         this.opacityProgressionButton.Size = new System.Drawing.Size(24, 24);
         this.opacityProgressionButton.TabIndex = 5;
         this.opacityProgressionButton.Text = "...";
         this.opacityProgressionButton.UseVisualStyleBackColor = true;
         this.opacityProgressionButton.Click += new System.EventHandler(this.opacityProgressionButton_Click);
         // 
         // opacityProgressionCheckBox
         // 
         this.opacityProgressionCheckBox.AutoSize = true;
         this.opacityProgressionCheckBox.Location = new System.Drawing.Point(11, 105);
         this.opacityProgressionCheckBox.Name = "opacityProgressionCheckBox";
         this.opacityProgressionCheckBox.Size = new System.Drawing.Size(142, 17);
         this.opacityProgressionCheckBox.TabIndex = 4;
         this.opacityProgressionCheckBox.Text = "Use Opacity Progression";
         this.opacityProgressionCheckBox.UseVisualStyleBackColor = true;
         this.opacityProgressionCheckBox.CheckedChanged += new System.EventHandler(this.opacityProgressionCheckBox_CheckedChanged);
         // 
         // weightNumericUpDown1
         // 
         this.weightNumericUpDown1.Location = new System.Drawing.Point(86, 63);
         this.weightNumericUpDown1.Name = "weightNumericUpDown1";
         this.weightNumericUpDown1.Size = new System.Drawing.Size(97, 20);
         this.weightNumericUpDown1.TabIndex = 3;
         this.weightNumericUpDown1.Value = new decimal(new int[] {
            1,
            0,
            0,
            0});
         this.weightNumericUpDown1.ValueChanged += new System.EventHandler(this.weightNumericUpDown1_ValueChanged);
         // 
         // label1
         // 
         this.label1.AutoSize = true;
         this.label1.Location = new System.Drawing.Point(8, 65);
         this.label1.Margin = new System.Windows.Forms.Padding(3);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(41, 13);
         this.label1.TabIndex = 2;
         this.label1.Text = "Weight";
         // 
         // fileBrowseControl1
         // 
         this.fileBrowseControl1.FileName = "";
         this.fileBrowseControl1.FilterExtension = ".gr2";
         this.fileBrowseControl1.FilterName = "Granny Animation Files";
         this.fileBrowseControl1.Location = new System.Drawing.Point(86, 32);
         this.fileBrowseControl1.Name = "fileBrowseControl1";
         this.fileBrowseControl1.ReferenceFolder = "art";
         this.fileBrowseControl1.Size = new System.Drawing.Size(256, 24);
         this.fileBrowseControl1.StartFolder = "art";
         this.fileBrowseControl1.TabIndex = 1;
         this.fileBrowseControl1.ValueChanged += new EditorCore.FileBrowseControl.ValueChangedDelegate(this.fileBrowseControl1_ValueChanged);
         // 
         // label2
         // 
         this.label2.AutoSize = true;
         this.label2.Location = new System.Drawing.Point(8, 32);
         this.label2.Margin = new System.Windows.Forms.Padding(3);
         this.label2.Name = "label2";
         this.label2.Size = new System.Drawing.Size(72, 13);
         this.label2.TabIndex = 0;
         this.label2.Text = "Animation File";
         // 
         // AnimAssetPage
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.groupBox1);
         this.Name = "AnimAssetPage";
         this.Size = new System.Drawing.Size(350, 250);
         this.groupBox1.ResumeLayout(false);
         this.groupBox1.PerformLayout();
         ((System.ComponentModel.ISupportInitialize)(this.weightNumericUpDown1)).EndInit();
         this.ResumeLayout(false);

      }

      #endregion

      private System.Windows.Forms.GroupBox groupBox1;
      private EditorCore.FileBrowseControl fileBrowseControl1;
      private System.Windows.Forms.Label label2;
      private System.Windows.Forms.NumericUpDown weightNumericUpDown1;
      private System.Windows.Forms.Label label1;
      private System.Windows.Forms.CheckBox opacityProgressionCheckBox;
      private System.Windows.Forms.Button opacityProgressionButton;
   }
}
