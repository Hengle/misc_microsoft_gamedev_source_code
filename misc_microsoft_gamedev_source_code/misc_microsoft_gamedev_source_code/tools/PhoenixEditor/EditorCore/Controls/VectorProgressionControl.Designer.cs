namespace EditorCore
{
    partial class VectorProgressionControl
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
           this.scalarProgressionControl3 = new EditorCore.ScalarProgressionControl();
           this.scalarProgressionControl2 = new EditorCore.ScalarProgressionControl();
           this.scalarProgressionControl1 = new EditorCore.ScalarProgressionControl();
           this.SuspendLayout();
           // 
           // scalarProgressionControl3
           // 
           this.scalarProgressionControl3.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                       | System.Windows.Forms.AnchorStyles.Right)));
           this.scalarProgressionControl3.AxisMaxX = 100;
           this.scalarProgressionControl3.AxisMaxY = 100;
           this.scalarProgressionControl3.AxisMinX = 0;
           this.scalarProgressionControl3.AxisMinY = -100;
           this.scalarProgressionControl3.ChartEndColor = System.Drawing.Color.Yellow;
           this.scalarProgressionControl3.ChartStartColor = System.Drawing.Color.Red;
           this.scalarProgressionControl3.Location = new System.Drawing.Point(3, 407);
           this.scalarProgressionControl3.Name = "scalarProgressionControl3";
           this.scalarProgressionControl3.ProgressionName = null;
           this.scalarProgressionControl3.Size = new System.Drawing.Size(897, 207);
           this.scalarProgressionControl3.TabIndex = 3;
           // 
           // scalarProgressionControl2
           // 
           this.scalarProgressionControl2.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                       | System.Windows.Forms.AnchorStyles.Right)));
           this.scalarProgressionControl2.AxisMaxX = 100;
           this.scalarProgressionControl2.AxisMaxY = 100;
           this.scalarProgressionControl2.AxisMinX = 0;
           this.scalarProgressionControl2.AxisMinY = -100;
           this.scalarProgressionControl2.ChartEndColor = System.Drawing.Color.FromArgb(((int)(((byte)(192)))), ((int)(((byte)(255)))), ((int)(((byte)(192)))));
           this.scalarProgressionControl2.ChartStartColor = System.Drawing.Color.Green;
           this.scalarProgressionControl2.Location = new System.Drawing.Point(3, 197);
           this.scalarProgressionControl2.Name = "scalarProgressionControl2";
           this.scalarProgressionControl2.ProgressionName = "Y Speed";
           this.scalarProgressionControl2.Size = new System.Drawing.Size(900, 204);
           this.scalarProgressionControl2.TabIndex = 2;
           // 
           // scalarProgressionControl1
           // 
           this.scalarProgressionControl1.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                       | System.Windows.Forms.AnchorStyles.Right)));
           this.scalarProgressionControl1.AxisMaxX = 100;
           this.scalarProgressionControl1.AxisMaxY = 100;
           this.scalarProgressionControl1.AxisMinX = 0;
           this.scalarProgressionControl1.AxisMinY = -100;
           this.scalarProgressionControl1.ChartEndColor = System.Drawing.Color.Yellow;
           this.scalarProgressionControl1.ChartStartColor = System.Drawing.Color.Red;
           this.scalarProgressionControl1.Location = new System.Drawing.Point(3, -4);
           this.scalarProgressionControl1.Name = "scalarProgressionControl1";
           this.scalarProgressionControl1.ProgressionName = "X Speed";
           this.scalarProgressionControl1.Size = new System.Drawing.Size(897, 198);
           this.scalarProgressionControl1.TabIndex = 1;
           // 
           // VectorProgressionControl
           // 
           this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
           this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
           this.Controls.Add(this.scalarProgressionControl3);
           this.Controls.Add(this.scalarProgressionControl2);
           this.Controls.Add(this.scalarProgressionControl1);
           this.Name = "VectorProgressionControl";
           this.Size = new System.Drawing.Size(900, 617);
           this.ResumeLayout(false);

        }

        #endregion

        private ScalarProgressionControl scalarProgressionControl1;
       private ScalarProgressionControl scalarProgressionControl2;
       private ScalarProgressionControl scalarProgressionControl3;


     }
}
