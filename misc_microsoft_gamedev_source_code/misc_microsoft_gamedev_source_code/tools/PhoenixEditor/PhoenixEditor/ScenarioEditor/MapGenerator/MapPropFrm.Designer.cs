namespace PhoenixEditor.ScenarioEditor
{
   partial class MapPropFrm
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
         this.button1 = new System.Windows.Forms.Button();
         this.glsTextBox = new System.Windows.Forms.TextBox();
         this.SuspendLayout();
         // 
         // label1
         // 
         this.label1.AutoSize = true;
         this.label1.Location = new System.Drawing.Point(3, 42);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(75, 13);
         this.label1.TabIndex = 0;
         this.label1.Text = "GLS To Use : ";
         // 
         // button1
         // 
         this.button1.Location = new System.Drawing.Point(417, 42);
         this.button1.Name = "button1";
         this.button1.Size = new System.Drawing.Size(25, 23);
         this.button1.TabIndex = 1;
         this.button1.Text = "...";
         this.button1.UseVisualStyleBackColor = true;
         this.button1.Click += new System.EventHandler(this.button1_Click);
         // 
         // glsTextBox
         // 
         this.glsTextBox.Location = new System.Drawing.Point(84, 42);
         this.glsTextBox.Name = "glsTextBox";
         this.glsTextBox.Size = new System.Drawing.Size(327, 20);
         this.glsTextBox.TabIndex = 2;
         // 
         // MapPropFrm
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.glsTextBox);
         this.Controls.Add(this.button1);
         this.Controls.Add(this.label1);
         this.Name = "MapPropFrm";
         this.Size = new System.Drawing.Size(458, 361);
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private System.Windows.Forms.Label label1;
      private System.Windows.Forms.Button button1;
      private System.Windows.Forms.TextBox glsTextBox;
   }
}
