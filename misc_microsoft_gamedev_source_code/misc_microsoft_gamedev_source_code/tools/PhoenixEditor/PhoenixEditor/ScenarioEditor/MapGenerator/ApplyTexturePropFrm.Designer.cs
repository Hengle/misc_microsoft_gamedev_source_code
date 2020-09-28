namespace PhoenixEditor.ScenarioEditor
{
   partial class ApplyTexturePropFrm
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
         this.label1 = new System.Windows.Forms.Label();
         this.label2 = new System.Windows.Forms.Label();
         this.intensitySlider = new EditorCore.NumericSliderControl();
         this.label4 = new System.Windows.Forms.Label();
         this.textureComboList = new System.Windows.Forms.ComboBox();
         this.label3 = new System.Windows.Forms.Label();
         this.SuspendLayout();
         // 
         // label1
         // 
         this.label1.AutoSize = true;
         this.label1.Location = new System.Drawing.Point(21, 27);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(73, 13);
         this.label1.TabIndex = 17;
         this.label1.Text = "Target Mask :";
         // 
         // label2
         // 
         this.label2.AutoSize = true;
         this.label2.Location = new System.Drawing.Point(100, 27);
         this.label2.Name = "label2";
         this.label2.Size = new System.Drawing.Size(70, 13);
         this.label2.TabIndex = 21;
         this.label2.Text = "Current Mask";
         // 
         // intensitySlider
         // 
         this.intensitySlider.BackColor = System.Drawing.SystemColors.ControlLight;
         this.intensitySlider.Location = new System.Drawing.Point(160, 92);
         this.intensitySlider.Name = "intensitySlider";
         this.intensitySlider.Size = new System.Drawing.Size(249, 22);
         this.intensitySlider.TabIndex = 28;
         // 
         // label4
         // 
         this.label4.AutoSize = true;
         this.label4.Location = new System.Drawing.Point(25, 92);
         this.label4.Name = "label4";
         this.label4.Size = new System.Drawing.Size(88, 13);
         this.label4.TabIndex = 27;
         this.label4.Text = "Intensity Scalar : ";
         // 
         // textureComboList
         // 
         this.textureComboList.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
         this.textureComboList.FormattingEnabled = true;
         this.textureComboList.Location = new System.Drawing.Point(160, 58);
         this.textureComboList.MaxDropDownItems = 25;
         this.textureComboList.Name = "textureComboList";
         this.textureComboList.Size = new System.Drawing.Size(249, 21);
         this.textureComboList.TabIndex = 26;
         this.textureComboList.SelectedIndexChanged += new System.EventHandler(this.textureComboList_SelectedIndexChanged);
         // 
         // label3
         // 
         this.label3.AutoSize = true;
         this.label3.Location = new System.Drawing.Point(25, 61);
         this.label3.Name = "label3";
         this.label3.Size = new System.Drawing.Size(95, 13);
         this.label3.TabIndex = 25;
         this.label3.Text = "Texture To Place :";
         // 
         // ApplyTexturePropFrm
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.intensitySlider);
         this.Controls.Add(this.label4);
         this.Controls.Add(this.textureComboList);
         this.Controls.Add(this.label3);
         this.Controls.Add(this.label2);
         this.Controls.Add(this.label1);
         this.Name = "ApplyTexturePropFrm";
         this.Size = new System.Drawing.Size(479, 378);
         this.Load += new System.EventHandler(this.ApplyTexturePropFrm_Load);
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private System.Windows.Forms.Label label1;
      private System.Windows.Forms.Label label2;
      private EditorCore.NumericSliderControl intensitySlider;
      private System.Windows.Forms.Label label4;
      private System.Windows.Forms.ComboBox textureComboList;
      private System.Windows.Forms.Label label3;
   }
}
