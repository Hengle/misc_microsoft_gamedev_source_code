namespace PhoenixEditor.ScenarioEditor
{
   partial class SimTileQuality
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

      #region Windows Form Designer generated code

      /// <summary>
      /// Required method for Designer support - do not modify
      /// the contents of this method with the code editor.
      /// </summary>
      private void InitializeComponent()
      {
         this.components = new System.ComponentModel.Container();
         this.button1 = new System.Windows.Forms.Button();
         this.button2 = new System.Windows.Forms.Button();
         this.toolTip1 = new System.Windows.Forms.ToolTip(this.components);
         this.label1 = new System.Windows.Forms.Label();
         this.trackBar1 = new System.Windows.Forms.TrackBar();
         ((System.ComponentModel.ISupportInitialize)(this.trackBar1)).BeginInit();
         this.SuspendLayout();
         // 
         // button1
         // 
         this.button1.Location = new System.Drawing.Point(213, 101);
         this.button1.Name = "button1";
         this.button1.Size = new System.Drawing.Size(75, 23);
         this.button1.TabIndex = 0;
         this.button1.Text = "OK";
         this.button1.UseVisualStyleBackColor = true;
         this.button1.Click += new System.EventHandler(this.button1_Click);
         // 
         // button2
         // 
         this.button2.Location = new System.Drawing.Point(294, 101);
         this.button2.Name = "button2";
         this.button2.Size = new System.Drawing.Size(75, 23);
         this.button2.TabIndex = 1;
         this.button2.Text = "Cancel";
         this.button2.UseVisualStyleBackColor = true;
         this.button2.Click += new System.EventHandler(this.button2_Click);
         // 
         // label1
         // 
         this.label1.AutoSize = true;
         this.label1.Location = new System.Drawing.Point(24, 19);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(35, 13);
         this.label1.TabIndex = 2;
         this.label1.Text = "label1";
         // 
         // trackBar1
         // 
         this.trackBar1.Location = new System.Drawing.Point(69, 40);
         this.trackBar1.Name = "trackBar1";
         this.trackBar1.Size = new System.Drawing.Size(149, 45);
         this.trackBar1.TabIndex = 3;
         this.trackBar1.Scroll += new System.EventHandler(this.trackBar1_Scroll);
         // 
         // SimTileQuality
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.ClientSize = new System.Drawing.Size(381, 136);
         this.Controls.Add(this.trackBar1);
         this.Controls.Add(this.label1);
         this.Controls.Add(this.button2);
         this.Controls.Add(this.button1);
         this.Name = "SimTileQuality";
         this.Text = "SimTileQuality";
         this.Load += new System.EventHandler(this.SimTileQuality_Load);
         ((System.ComponentModel.ISupportInitialize)(this.trackBar1)).EndInit();
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private System.Windows.Forms.Button button1;
      private System.Windows.Forms.Button button2;
      private System.Windows.Forms.ToolTip toolTip1;
      private System.Windows.Forms.Label label1;
      private System.Windows.Forms.TrackBar trackBar1;
   }
}