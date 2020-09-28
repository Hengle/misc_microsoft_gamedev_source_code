namespace PhoenixEditor.ScenarioEditor
{
   partial class ScenarioProperties
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
         this.displaynameNameBox = new System.Windows.Forms.TextBox();
         this.scenarioFileNameBox = new System.Windows.Forms.TextBox();
         this.label2 = new System.Windows.Forms.Label();
         this.rootTerrainNameBox = new System.Windows.Forms.TextBox();
         this.label3 = new System.Windows.Forms.Label();
         this.groupBox1 = new System.Windows.Forms.GroupBox();
         this.boundryTopSlider = new EditorCore.NumericSliderControl();
         this.boundryBottomSlider = new EditorCore.NumericSliderControl();
         this.boundryRightSlider = new EditorCore.NumericSliderControl();
         this.boundryLeftSlider = new EditorCore.NumericSliderControl();
         this.label4 = new System.Windows.Forms.Label();
         this.label5 = new System.Windows.Forms.Label();
         this.label6 = new System.Windows.Forms.Label();
         this.label7 = new System.Windows.Forms.Label();
         this.groupBox2 = new System.Windows.Forms.GroupBox();
         this.groupBox1.SuspendLayout();
         this.groupBox2.SuspendLayout();
         this.SuspendLayout();
         // 
         // label1
         // 
         this.label1.AutoSize = true;
         this.label1.Location = new System.Drawing.Point(6, 42);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(126, 13);
         this.label1.TabIndex = 0;
         this.label1.Text = "Scenario Display Name : ";
         // 
         // displaynameNameBox
         // 
         this.displaynameNameBox.Location = new System.Drawing.Point(138, 42);
         this.displaynameNameBox.Name = "displaynameNameBox";
         this.displaynameNameBox.Size = new System.Drawing.Size(276, 20);
         this.displaynameNameBox.TabIndex = 1;
         // 
         // scenarioFileNameBox
         // 
         this.scenarioFileNameBox.Location = new System.Drawing.Point(138, 16);
         this.scenarioFileNameBox.Name = "scenarioFileNameBox";
         this.scenarioFileNameBox.ReadOnly = true;
         this.scenarioFileNameBox.Size = new System.Drawing.Size(276, 20);
         this.scenarioFileNameBox.TabIndex = 3;
         // 
         // label2
         // 
         this.label2.AutoSize = true;
         this.label2.Location = new System.Drawing.Point(6, 16);
         this.label2.Name = "label2";
         this.label2.Size = new System.Drawing.Size(108, 13);
         this.label2.TabIndex = 2;
         this.label2.Text = "Scenario File Name : ";
         // 
         // rootTerrainNameBox
         // 
         this.rootTerrainNameBox.Location = new System.Drawing.Point(138, 68);
         this.rootTerrainNameBox.Name = "rootTerrainNameBox";
         this.rootTerrainNameBox.ReadOnly = true;
         this.rootTerrainNameBox.Size = new System.Drawing.Size(276, 20);
         this.rootTerrainNameBox.TabIndex = 5;
         // 
         // label3
         // 
         this.label3.AutoSize = true;
         this.label3.Location = new System.Drawing.Point(6, 68);
         this.label3.Name = "label3";
         this.label3.Size = new System.Drawing.Size(94, 13);
         this.label3.TabIndex = 4;
         this.label3.Text = "Root Terrain File : ";
         // 
         // groupBox1
         // 
         this.groupBox1.Controls.Add(this.label7);
         this.groupBox1.Controls.Add(this.label6);
         this.groupBox1.Controls.Add(this.label5);
         this.groupBox1.Controls.Add(this.label4);
         this.groupBox1.Controls.Add(this.boundryRightSlider);
         this.groupBox1.Controls.Add(this.boundryLeftSlider);
         this.groupBox1.Controls.Add(this.boundryBottomSlider);
         this.groupBox1.Controls.Add(this.boundryTopSlider);
         this.groupBox1.Location = new System.Drawing.Point(17, 160);
         this.groupBox1.Name = "groupBox1";
         this.groupBox1.Size = new System.Drawing.Size(278, 151);
         this.groupBox1.TabIndex = 6;
         this.groupBox1.TabStop = false;
         this.groupBox1.Text = "Playable Boundries";
         // 
         // boundryTopSlider
         // 
         this.boundryTopSlider.BackColor = System.Drawing.SystemColors.ControlLight;
         this.boundryTopSlider.Location = new System.Drawing.Point(67, 19);
         this.boundryTopSlider.Name = "boundryTopSlider";
         this.boundryTopSlider.Size = new System.Drawing.Size(195, 22);
         this.boundryTopSlider.TabIndex = 0;
         // 
         // boundryBottomSlider
         // 
         this.boundryBottomSlider.BackColor = System.Drawing.SystemColors.ControlLight;
         this.boundryBottomSlider.Location = new System.Drawing.Point(67, 47);
         this.boundryBottomSlider.Name = "boundryBottomSlider";
         this.boundryBottomSlider.Size = new System.Drawing.Size(195, 22);
         this.boundryBottomSlider.TabIndex = 1;
         // 
         // boundryRightSlider
         // 
         this.boundryRightSlider.BackColor = System.Drawing.SystemColors.ControlLight;
         this.boundryRightSlider.Location = new System.Drawing.Point(67, 103);
         this.boundryRightSlider.Name = "boundryRightSlider";
         this.boundryRightSlider.Size = new System.Drawing.Size(195, 22);
         this.boundryRightSlider.TabIndex = 3;
         // 
         // boundryLeftSlider
         // 
         this.boundryLeftSlider.BackColor = System.Drawing.SystemColors.ControlLight;
         this.boundryLeftSlider.Location = new System.Drawing.Point(67, 75);
         this.boundryLeftSlider.Name = "boundryLeftSlider";
         this.boundryLeftSlider.Size = new System.Drawing.Size(195, 22);
         this.boundryLeftSlider.TabIndex = 2;
         // 
         // label4
         // 
         this.label4.AutoSize = true;
         this.label4.Location = new System.Drawing.Point(6, 19);
         this.label4.Name = "label4";
         this.label4.Size = new System.Drawing.Size(32, 13);
         this.label4.TabIndex = 5;
         this.label4.Text = "Top :";
         // 
         // label5
         // 
         this.label5.AutoSize = true;
         this.label5.Location = new System.Drawing.Point(6, 47);
         this.label5.Name = "label5";
         this.label5.Size = new System.Drawing.Size(46, 13);
         this.label5.TabIndex = 6;
         this.label5.Text = "Bottom :";
         // 
         // label6
         // 
         this.label6.AutoSize = true;
         this.label6.Location = new System.Drawing.Point(6, 75);
         this.label6.Name = "label6";
         this.label6.Size = new System.Drawing.Size(31, 13);
         this.label6.TabIndex = 7;
         this.label6.Text = "Left :";
         // 
         // label7
         // 
         this.label7.AutoSize = true;
         this.label7.Location = new System.Drawing.Point(6, 103);
         this.label7.Name = "label7";
         this.label7.Size = new System.Drawing.Size(38, 13);
         this.label7.TabIndex = 8;
         this.label7.Text = "Right :";
         // 
         // groupBox2
         // 
         this.groupBox2.Controls.Add(this.label2);
         this.groupBox2.Controls.Add(this.label1);
         this.groupBox2.Controls.Add(this.rootTerrainNameBox);
         this.groupBox2.Controls.Add(this.displaynameNameBox);
         this.groupBox2.Controls.Add(this.label3);
         this.groupBox2.Controls.Add(this.scenarioFileNameBox);
         this.groupBox2.Location = new System.Drawing.Point(17, 14);
         this.groupBox2.Name = "groupBox2";
         this.groupBox2.Size = new System.Drawing.Size(435, 140);
         this.groupBox2.TabIndex = 7;
         this.groupBox2.TabStop = false;
         this.groupBox2.Text = "Scenario Info";
         // 
         // ScenarioProperties
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.groupBox2);
         this.Controls.Add(this.groupBox1);
         this.Name = "ScenarioProperties";
         this.Size = new System.Drawing.Size(674, 463);
         this.Load += new System.EventHandler(this.ScenarioProperties_Load);
         this.groupBox1.ResumeLayout(false);
         this.groupBox1.PerformLayout();
         this.groupBox2.ResumeLayout(false);
         this.groupBox2.PerformLayout();
         this.ResumeLayout(false);

      }

      #endregion

      private System.Windows.Forms.Label label1;
      private System.Windows.Forms.TextBox displaynameNameBox;
      private System.Windows.Forms.TextBox scenarioFileNameBox;
      private System.Windows.Forms.Label label2;
      private System.Windows.Forms.TextBox rootTerrainNameBox;
      private System.Windows.Forms.Label label3;
      private System.Windows.Forms.GroupBox groupBox1;
      private EditorCore.NumericSliderControl boundryTopSlider;
      private System.Windows.Forms.Label label4;
      private EditorCore.NumericSliderControl boundryRightSlider;
      private EditorCore.NumericSliderControl boundryLeftSlider;
      private EditorCore.NumericSliderControl boundryBottomSlider;
      private System.Windows.Forms.Label label7;
      private System.Windows.Forms.Label label6;
      private System.Windows.Forms.Label label5;
      private System.Windows.Forms.GroupBox groupBox2;
   }
}
