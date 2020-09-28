namespace VisualEditor.PropertyPages
{
   partial class LogicNodeDestructionPage
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
         this.floatSliderEdit = new VisualEditor.Controls.FloatSliderEdit();
         this.label2 = new System.Windows.Forms.Label();
         this.groupBox1.SuspendLayout();
         this.SuspendLayout();
         // 
         // groupBox1
         // 
         this.groupBox1.Anchor = System.Windows.Forms.AnchorStyles.None;
         this.groupBox1.Controls.Add(this.floatSliderEdit);
         this.groupBox1.Controls.Add(this.label2);
         this.groupBox1.Location = new System.Drawing.Point(0, 0);
         this.groupBox1.Name = "groupBox1";
         this.groupBox1.Padding = new System.Windows.Forms.Padding(5, 16, 5, 3);
         this.groupBox1.Size = new System.Drawing.Size(350, 250);
         this.groupBox1.TabIndex = 3;
         this.groupBox1.TabStop = false;
         this.groupBox1.Text = "Logic (Destruction) Properties";
         // 
         // floatSliderEdit
         // 
         this.floatSliderEdit.Location = new System.Drawing.Point(102, 32);
         this.floatSliderEdit.Margin = new System.Windows.Forms.Padding(3, 3, 3, 0);
         this.floatSliderEdit.MaxValue = 100F;
         this.floatSliderEdit.MinValue = 0F;
         this.floatSliderEdit.Name = "floatSliderEdit";
         this.floatSliderEdit.NumDecimals = 0;
         this.floatSliderEdit.Size = new System.Drawing.Size(240, 40);
         this.floatSliderEdit.TabIndex = 9;
         this.floatSliderEdit.Value = 0F;
         this.floatSliderEdit.ValueChanged += new VisualEditor.Controls.FloatSliderEdit.ValueChangedDelegate(this.floatSliderEdit_ValueChanged);
         // 
         // label2
         // 
         this.label2.AutoSize = true;
         this.label2.Location = new System.Drawing.Point(8, 34);
         this.label2.Name = "label2";
         this.label2.Size = new System.Drawing.Size(62, 13);
         this.label2.TabIndex = 8;
         this.label2.Text = "Percentage";
         // 
         // LogicNodeDestructionPage
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.groupBox1);
         this.Name = "LogicNodeDestructionPage";
         this.Size = new System.Drawing.Size(350, 250);
         this.groupBox1.ResumeLayout(false);
         this.groupBox1.PerformLayout();
         this.ResumeLayout(false);

      }

      #endregion

      private System.Windows.Forms.GroupBox groupBox1;
      private VisualEditor.Controls.FloatSliderEdit floatSliderEdit;
      private System.Windows.Forms.Label label2;
   }
}
