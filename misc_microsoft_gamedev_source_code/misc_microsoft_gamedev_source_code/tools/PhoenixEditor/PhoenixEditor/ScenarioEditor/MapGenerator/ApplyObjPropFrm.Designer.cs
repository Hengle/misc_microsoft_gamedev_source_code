namespace PhoenixEditor.ScenarioEditor
{
   partial class ApplyObjPropFrm
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
         this.label3 = new System.Windows.Forms.Label();
         this.objectComboList = new System.Windows.Forms.ComboBox();
         this.label4 = new System.Windows.Forms.Label();
         this.numberToPlaceSlider = new EditorCore.NumericSliderControl();
         this.SuspendLayout();
         // 
         // label1
         // 
         this.label1.AutoSize = true;
         this.label1.Location = new System.Drawing.Point(17, 14);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(73, 13);
         this.label1.TabIndex = 19;
         this.label1.Text = "Target Mask :";
         // 
         // label2
         // 
         this.label2.AutoSize = true;
         this.label2.Location = new System.Drawing.Point(96, 14);
         this.label2.Name = "label2";
         this.label2.Size = new System.Drawing.Size(70, 13);
         this.label2.TabIndex = 20;
         this.label2.Text = "Current Mask";
         // 
         // label3
         // 
         this.label3.AutoSize = true;
         this.label3.Location = new System.Drawing.Point(17, 49);
         this.label3.Name = "label3";
         this.label3.Size = new System.Drawing.Size(90, 13);
         this.label3.TabIndex = 21;
         this.label3.Text = "Object To Place :";
         // 
         // objectComboList
         // 
         this.objectComboList.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
         this.objectComboList.FormattingEnabled = true;
         this.objectComboList.Location = new System.Drawing.Point(152, 46);
         this.objectComboList.MaxDropDownItems = 25;
         this.objectComboList.Name = "objectComboList";
         this.objectComboList.Size = new System.Drawing.Size(249, 21);
         this.objectComboList.TabIndex = 22;
         this.objectComboList.SelectedIndexChanged += new System.EventHandler(this.objectComboList_SelectedIndexChanged);
         // 
         // label4
         // 
         this.label4.AutoSize = true;
         this.label4.Location = new System.Drawing.Point(17, 80);
         this.label4.Name = "label4";
         this.label4.Size = new System.Drawing.Size(99, 13);
         this.label4.TabIndex = 23;
         this.label4.Text = "Number To Place : ";
         // 
         // numberToPlaceSlider
         // 
         this.numberToPlaceSlider.BackColor = System.Drawing.SystemColors.ControlLight;
         this.numberToPlaceSlider.Location = new System.Drawing.Point(152, 80);
         this.numberToPlaceSlider.Name = "numberToPlaceSlider";
         this.numberToPlaceSlider.Size = new System.Drawing.Size(249, 22);
         this.numberToPlaceSlider.TabIndex = 24;
         // 
         // ApplyObjPropFrm
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.numberToPlaceSlider);
         this.Controls.Add(this.label4);
         this.Controls.Add(this.objectComboList);
         this.Controls.Add(this.label3);
         this.Controls.Add(this.label2);
         this.Controls.Add(this.label1);
         this.Name = "ApplyObjPropFrm";
         this.Size = new System.Drawing.Size(474, 355);
         this.Load += new System.EventHandler(this.ApplyObjPropFrm_Load);
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private System.Windows.Forms.Label label1;
      private System.Windows.Forms.Label label2;
      private System.Windows.Forms.Label label3;
      private System.Windows.Forms.ComboBox objectComboList;
      private System.Windows.Forms.Label label4;
      private EditorCore.NumericSliderControl numberToPlaceSlider;
   }
}
