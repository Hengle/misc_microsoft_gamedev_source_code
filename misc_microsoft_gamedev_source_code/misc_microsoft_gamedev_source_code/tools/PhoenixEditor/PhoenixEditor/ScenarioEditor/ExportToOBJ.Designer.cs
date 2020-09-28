namespace PhoenixEditor.ScenarioEditor
{
   partial class ExportToOBJ
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
         this.button3 = new System.Windows.Forms.Button();
         this.button2 = new System.Windows.Forms.Button();
         this.groupBox1 = new System.Windows.Forms.GroupBox();
         this.exportSelectedVerts = new System.Windows.Forms.RadioButton();
         this.exportAllVerts = new System.Windows.Forms.RadioButton();
         this.label1 = new System.Windows.Forms.Label();
         this.button1 = new System.Windows.Forms.Button();
         this.textBox1 = new System.Windows.Forms.TextBox();
         this.groupBox2 = new System.Windows.Forms.GroupBox();
         this.refineNone = new System.Windows.Forms.RadioButton();
         this.refineTIN = new System.Windows.Forms.RadioButton();
         this.refineRQT = new System.Windows.Forms.RadioButton();
         this.groupBox3 = new System.Windows.Forms.GroupBox();
         this.highMeshLOD = new System.Windows.Forms.RadioButton();
         this.medMeshLOD = new System.Windows.Forms.RadioButton();
         this.lowMeshLOD = new System.Windows.Forms.RadioButton();
         this.groupBox1.SuspendLayout();
         this.groupBox2.SuspendLayout();
         this.groupBox3.SuspendLayout();
         this.SuspendLayout();
         // 
         // button3
         // 
         this.button3.Location = new System.Drawing.Point(340, 178);
         this.button3.Name = "button3";
         this.button3.Size = new System.Drawing.Size(75, 23);
         this.button3.TabIndex = 11;
         this.button3.Text = "Cancel";
         this.button3.UseVisualStyleBackColor = true;
         this.button3.Click += new System.EventHandler(this.button3_Click);
         // 
         // button2
         // 
         this.button2.Location = new System.Drawing.Point(259, 178);
         this.button2.Name = "button2";
         this.button2.Size = new System.Drawing.Size(75, 23);
         this.button2.TabIndex = 10;
         this.button2.Text = "OK";
         this.button2.UseVisualStyleBackColor = true;
         this.button2.Click += new System.EventHandler(this.button2_Click);
         // 
         // groupBox1
         // 
         this.groupBox1.Controls.Add(this.exportSelectedVerts);
         this.groupBox1.Controls.Add(this.exportAllVerts);
         this.groupBox1.Location = new System.Drawing.Point(10, 56);
         this.groupBox1.Name = "groupBox1";
         this.groupBox1.Size = new System.Drawing.Size(136, 93);
         this.groupBox1.TabIndex = 9;
         this.groupBox1.TabStop = false;
         this.groupBox1.Text = "Export Options";
         // 
         // exportSelectedVerts
         // 
         this.exportSelectedVerts.AutoSize = true;
         this.exportSelectedVerts.Location = new System.Drawing.Point(6, 42);
         this.exportSelectedVerts.Name = "exportSelectedVerts";
         this.exportSelectedVerts.Size = new System.Drawing.Size(118, 17);
         this.exportSelectedVerts.TabIndex = 3;
         this.exportSelectedVerts.Text = "Only Selected Verts";
         this.exportSelectedVerts.UseVisualStyleBackColor = true;
         this.exportSelectedVerts.CheckedChanged += new System.EventHandler(this.exportSelectedVerts_CheckedChanged);
         // 
         // exportAllVerts
         // 
         this.exportAllVerts.AutoSize = true;
         this.exportAllVerts.Checked = true;
         this.exportAllVerts.Location = new System.Drawing.Point(6, 19);
         this.exportAllVerts.Name = "exportAllVerts";
         this.exportAllVerts.Size = new System.Drawing.Size(62, 17);
         this.exportAllVerts.TabIndex = 2;
         this.exportAllVerts.TabStop = true;
         this.exportAllVerts.Text = "All verts";
         this.exportAllVerts.UseVisualStyleBackColor = true;
         this.exportAllVerts.CheckedChanged += new System.EventHandler(this.exportAllVerts_CheckedChanged);
         // 
         // label1
         // 
         this.label1.AutoSize = true;
         this.label1.Location = new System.Drawing.Point(7, 9);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(88, 13);
         this.label1.TabIndex = 8;
         this.label1.Text = "Path to save to : ";
         // 
         // button1
         // 
         this.button1.Location = new System.Drawing.Point(377, 28);
         this.button1.Name = "button1";
         this.button1.Size = new System.Drawing.Size(26, 23);
         this.button1.TabIndex = 7;
         this.button1.Text = "...";
         this.button1.UseVisualStyleBackColor = true;
         this.button1.Click += new System.EventHandler(this.button1_Click);
         // 
         // textBox1
         // 
         this.textBox1.Location = new System.Drawing.Point(10, 30);
         this.textBox1.Name = "textBox1";
         this.textBox1.Size = new System.Drawing.Size(361, 20);
         this.textBox1.TabIndex = 6;
         // 
         // groupBox2
         // 
         this.groupBox2.Controls.Add(this.refineNone);
         this.groupBox2.Controls.Add(this.refineTIN);
         this.groupBox2.Controls.Add(this.refineRQT);
         this.groupBox2.Location = new System.Drawing.Point(161, 57);
         this.groupBox2.Name = "groupBox2";
         this.groupBox2.Size = new System.Drawing.Size(126, 92);
         this.groupBox2.TabIndex = 12;
         this.groupBox2.TabStop = false;
         this.groupBox2.Text = "Refinement Type";
         // 
         // refineNone
         // 
         this.refineNone.AutoSize = true;
         this.refineNone.Enabled = false;
         this.refineNone.Location = new System.Drawing.Point(6, 63);
         this.refineNone.Name = "refineNone";
         this.refineNone.Size = new System.Drawing.Size(51, 17);
         this.refineNone.TabIndex = 2;
         this.refineNone.Text = "None";
         this.refineNone.UseVisualStyleBackColor = true;
         this.refineNone.CheckedChanged += new System.EventHandler(this.refineNone_CheckedChanged);
         // 
         // refineTIN
         // 
         this.refineTIN.AutoSize = true;
         this.refineTIN.Checked = true;
         this.refineTIN.Location = new System.Drawing.Point(6, 42);
         this.refineTIN.Name = "refineTIN";
         this.refineTIN.Size = new System.Drawing.Size(43, 17);
         this.refineTIN.TabIndex = 1;
         this.refineTIN.TabStop = true;
         this.refineTIN.Text = "TIN";
         this.refineTIN.UseVisualStyleBackColor = true;
         this.refineTIN.CheckedChanged += new System.EventHandler(this.refineTIN_CheckedChanged);
         // 
         // refineRQT
         // 
         this.refineRQT.AutoSize = true;
         this.refineRQT.Enabled = false;
         this.refineRQT.Location = new System.Drawing.Point(6, 19);
         this.refineRQT.Name = "refineRQT";
         this.refineRQT.Size = new System.Drawing.Size(48, 17);
         this.refineRQT.TabIndex = 0;
         this.refineRQT.Text = "RQT";
         this.refineRQT.UseVisualStyleBackColor = true;
         this.refineRQT.CheckedChanged += new System.EventHandler(this.refineRQT_CheckedChanged);
         // 
         // groupBox3
         // 
         this.groupBox3.Controls.Add(this.lowMeshLOD);
         this.groupBox3.Controls.Add(this.medMeshLOD);
         this.groupBox3.Controls.Add(this.highMeshLOD);
         this.groupBox3.Location = new System.Drawing.Point(305, 57);
         this.groupBox3.Name = "groupBox3";
         this.groupBox3.Size = new System.Drawing.Size(110, 92);
         this.groupBox3.TabIndex = 13;
         this.groupBox3.TabStop = false;
         this.groupBox3.Text = "Mesh Detail";
         // 
         // highMeshLOD
         // 
         this.highMeshLOD.AutoSize = true;
         this.highMeshLOD.Location = new System.Drawing.Point(6, 19);
         this.highMeshLOD.Name = "highMeshLOD";
         this.highMeshLOD.Size = new System.Drawing.Size(47, 17);
         this.highMeshLOD.TabIndex = 0;
         this.highMeshLOD.Text = "High";
         this.highMeshLOD.UseVisualStyleBackColor = true;
         // 
         // medMeshLOD
         // 
         this.medMeshLOD.AutoSize = true;
         this.medMeshLOD.Checked = true;
         this.medMeshLOD.Location = new System.Drawing.Point(6, 42);
         this.medMeshLOD.Name = "medMeshLOD";
         this.medMeshLOD.Size = new System.Drawing.Size(62, 17);
         this.medMeshLOD.TabIndex = 1;
         this.medMeshLOD.TabStop = true;
         this.medMeshLOD.Text = "Medium";
         this.medMeshLOD.UseVisualStyleBackColor = true;
         // 
         // lowMeshLOD
         // 
         this.lowMeshLOD.AutoSize = true;
         this.lowMeshLOD.Location = new System.Drawing.Point(6, 65);
         this.lowMeshLOD.Name = "lowMeshLOD";
         this.lowMeshLOD.Size = new System.Drawing.Size(45, 17);
         this.lowMeshLOD.TabIndex = 2;
         this.lowMeshLOD.Text = "Low";
         this.lowMeshLOD.UseVisualStyleBackColor = true;
         // 
         // ExportToOBJ
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.ClientSize = new System.Drawing.Size(454, 220);
         this.Controls.Add(this.groupBox3);
         this.Controls.Add(this.groupBox2);
         this.Controls.Add(this.button3);
         this.Controls.Add(this.button2);
         this.Controls.Add(this.groupBox1);
         this.Controls.Add(this.label1);
         this.Controls.Add(this.button1);
         this.Controls.Add(this.textBox1);
         this.Name = "ExportToOBJ";
         this.Text = "ExportToOBJ";
         this.groupBox1.ResumeLayout(false);
         this.groupBox1.PerformLayout();
         this.groupBox2.ResumeLayout(false);
         this.groupBox2.PerformLayout();
         this.groupBox3.ResumeLayout(false);
         this.groupBox3.PerformLayout();
         this.ResumeLayout(false);
         this.PerformLayout();

      }

      #endregion

      private System.Windows.Forms.Button button3;
      private System.Windows.Forms.Button button2;
      private System.Windows.Forms.GroupBox groupBox1;
      private System.Windows.Forms.Label label1;
      private System.Windows.Forms.Button button1;
      private System.Windows.Forms.TextBox textBox1;
      private System.Windows.Forms.RadioButton exportSelectedVerts;
      private System.Windows.Forms.RadioButton exportAllVerts;
      private System.Windows.Forms.GroupBox groupBox2;
      private System.Windows.Forms.RadioButton refineNone;
      private System.Windows.Forms.RadioButton refineTIN;
      private System.Windows.Forms.RadioButton refineRQT;
      private System.Windows.Forms.GroupBox groupBox3;
      private System.Windows.Forms.RadioButton lowMeshLOD;
      private System.Windows.Forms.RadioButton medMeshLOD;
      private System.Windows.Forms.RadioButton highMeshLOD;
   }
}