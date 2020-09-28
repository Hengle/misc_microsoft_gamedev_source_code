namespace PhoenixEditor.ScenarioEditor
{
   partial class WaterPanel
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
         this.components = new System.ComponentModel.Container();
         System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(WaterPanel));
         this.waterBodyPaintButt = new System.Windows.Forms.Button();
         this.waterRiverButt = new System.Windows.Forms.Button();
         this.waterEditButt = new System.Windows.Forms.Button();
         this.waterOceanPaintButt = new System.Windows.Forms.Button();
         this.toolTip1 = new System.Windows.Forms.ToolTip(this.components);
         this.groupBox1 = new System.Windows.Forms.GroupBox();
         this.label6 = new System.Windows.Forms.Label();
         this.channelForceSlider = new EditorCore.NumericSliderControl();
         this.button2 = new System.Windows.Forms.Button();
         this.label3 = new System.Windows.Forms.Label();
         this.dropLifeSlider = new EditorCore.NumericSliderControl();
         this.checkBox1 = new System.Windows.Forms.CheckBox();
         this.label5 = new System.Windows.Forms.Label();
         this.randomSlider = new EditorCore.NumericSliderControl();
         this.label2 = new System.Windows.Forms.Label();
         this.hardnessSlider = new EditorCore.NumericSliderControl();
         this.label1 = new System.Windows.Forms.Label();
         this.powerSilder = new EditorCore.NumericSliderControl();
         this.button1 = new System.Windows.Forms.Button();
         this.createMaskBox = new System.Windows.Forms.ComboBox();
         this.label7 = new System.Windows.Forms.Label();
         this.groupBox1.SuspendLayout();
         this.SuspendLayout();
         // 
         // waterBodyPaintButt
         // 
         this.waterBodyPaintButt.Image = ((System.Drawing.Image)(resources.GetObject("waterBodyPaintButt.Image")));
         this.waterBodyPaintButt.Location = new System.Drawing.Point(49, 537);
         this.waterBodyPaintButt.Name = "waterBodyPaintButt";
         this.waterBodyPaintButt.Size = new System.Drawing.Size(37, 33);
         this.waterBodyPaintButt.TabIndex = 0;
         this.toolTip1.SetToolTip(this.waterBodyPaintButt, "Paint Water Body");
         this.waterBodyPaintButt.UseVisualStyleBackColor = true;
         this.waterBodyPaintButt.Visible = false;
         this.waterBodyPaintButt.Click += new System.EventHandler(this.waterBodyPaintButt_Click);
         // 
         // waterRiverButt
         // 
         this.waterRiverButt.Image = ((System.Drawing.Image)(resources.GetObject("waterRiverButt.Image")));
         this.waterRiverButt.Location = new System.Drawing.Point(135, 537);
         this.waterRiverButt.Name = "waterRiverButt";
         this.waterRiverButt.Size = new System.Drawing.Size(37, 33);
         this.waterRiverButt.TabIndex = 1;
         this.toolTip1.SetToolTip(this.waterRiverButt, "River Edit");
         this.waterRiverButt.UseVisualStyleBackColor = true;
         this.waterRiverButt.Visible = false;
         this.waterRiverButt.Click += new System.EventHandler(this.waterRiverButt_Click);
         // 
         // waterEditButt
         // 
         this.waterEditButt.FlatAppearance.BorderSize = 0;
         this.waterEditButt.Image = ((System.Drawing.Image)(resources.GetObject("waterEditButt.Image")));
         this.waterEditButt.Location = new System.Drawing.Point(9, 537);
         this.waterEditButt.Margin = new System.Windows.Forms.Padding(0);
         this.waterEditButt.Name = "waterEditButt";
         this.waterEditButt.Size = new System.Drawing.Size(37, 33);
         this.waterEditButt.TabIndex = 2;
         this.toolTip1.SetToolTip(this.waterEditButt, "Water Edit");
         this.waterEditButt.UseVisualStyleBackColor = true;
         this.waterEditButt.Visible = false;
         this.waterEditButt.Click += new System.EventHandler(this.waterEditButt_Click);
         // 
         // waterOceanPaintButt
         // 
         this.waterOceanPaintButt.Image = ((System.Drawing.Image)(resources.GetObject("waterOceanPaintButt.Image")));
         this.waterOceanPaintButt.Location = new System.Drawing.Point(92, 537);
         this.waterOceanPaintButt.Name = "waterOceanPaintButt";
         this.waterOceanPaintButt.Size = new System.Drawing.Size(37, 33);
         this.waterOceanPaintButt.TabIndex = 3;
         this.toolTip1.SetToolTip(this.waterOceanPaintButt, "Paint Water Ocean");
         this.waterOceanPaintButt.UseVisualStyleBackColor = true;
         this.waterOceanPaintButt.Visible = false;
         this.waterOceanPaintButt.Click += new System.EventHandler(this.waterOceanPaintButt_Click);
         // 
         // groupBox1
         // 
         this.groupBox1.Controls.Add(this.label7);
         this.groupBox1.Controls.Add(this.createMaskBox);
         this.groupBox1.Controls.Add(this.label6);
         this.groupBox1.Controls.Add(this.channelForceSlider);
         this.groupBox1.Controls.Add(this.button2);
         this.groupBox1.Controls.Add(this.label3);
         this.groupBox1.Controls.Add(this.dropLifeSlider);
         this.groupBox1.Controls.Add(this.checkBox1);
         this.groupBox1.Controls.Add(this.label5);
         this.groupBox1.Controls.Add(this.randomSlider);
         this.groupBox1.Controls.Add(this.label2);
         this.groupBox1.Controls.Add(this.hardnessSlider);
         this.groupBox1.Controls.Add(this.label1);
         this.groupBox1.Controls.Add(this.powerSilder);
         this.groupBox1.Controls.Add(this.button1);
         this.groupBox1.Location = new System.Drawing.Point(3, 3);
         this.groupBox1.Name = "groupBox1";
         this.groupBox1.Size = new System.Drawing.Size(237, 381);
         this.groupBox1.TabIndex = 4;
         this.groupBox1.TabStop = false;
         this.groupBox1.Text = "Hydraulic Erosion";
         // 
         // label6
         // 
         this.label6.AutoSize = true;
         this.label6.Location = new System.Drawing.Point(6, 100);
         this.label6.Name = "label6";
         this.label6.Size = new System.Drawing.Size(76, 13);
         this.label6.TabIndex = 16;
         this.label6.Text = "Channel Force";
         // 
         // channelForceSlider
         // 
         this.channelForceSlider.BackColor = System.Drawing.SystemColors.ControlLight;
         this.channelForceSlider.Location = new System.Drawing.Point(83, 100);
         this.channelForceSlider.Name = "channelForceSlider";
         this.channelForceSlider.Size = new System.Drawing.Size(147, 22);
         this.channelForceSlider.TabIndex = 15;
         // 
         // button2
         // 
         this.button2.Location = new System.Drawing.Point(6, 352);
         this.button2.Name = "button2";
         this.button2.Size = new System.Drawing.Size(75, 23);
         this.button2.TabIndex = 14;
         this.button2.Text = "Defaults";
         this.button2.UseVisualStyleBackColor = true;
         this.button2.Click += new System.EventHandler(this.button2_Click);
         // 
         // label3
         // 
         this.label3.AutoSize = true;
         this.label3.Location = new System.Drawing.Point(6, 72);
         this.label3.Name = "label3";
         this.label3.Size = new System.Drawing.Size(50, 13);
         this.label3.TabIndex = 13;
         this.label3.Text = "Drop Life";
         // 
         // dropLifeSlider
         // 
         this.dropLifeSlider.BackColor = System.Drawing.SystemColors.ControlLight;
         this.dropLifeSlider.Location = new System.Drawing.Point(83, 72);
         this.dropLifeSlider.Name = "dropLifeSlider";
         this.dropLifeSlider.Size = new System.Drawing.Size(147, 22);
         this.dropLifeSlider.TabIndex = 12;
         // 
         // checkBox1
         // 
         this.checkBox1.AutoSize = true;
         this.checkBox1.Location = new System.Drawing.Point(116, 329);
         this.checkBox1.Name = "checkBox1";
         this.checkBox1.Size = new System.Drawing.Size(114, 17);
         this.checkBox1.TabIndex = 11;
         this.checkBox1.Text = "Apply only to mask";
         this.checkBox1.UseVisualStyleBackColor = true;
         this.checkBox1.CheckedChanged += new System.EventHandler(this.checkBox1_CheckedChanged);
         // 
         // label5
         // 
         this.label5.AutoSize = true;
         this.label5.Location = new System.Drawing.Point(6, 128);
         this.label5.Name = "label5";
         this.label5.Size = new System.Drawing.Size(72, 13);
         this.label5.TabIndex = 10;
         this.label5.Text = "RandomSeed";
         // 
         // randomSlider
         // 
         this.randomSlider.BackColor = System.Drawing.SystemColors.ControlLight;
         this.randomSlider.Location = new System.Drawing.Point(83, 128);
         this.randomSlider.Name = "randomSlider";
         this.randomSlider.Size = new System.Drawing.Size(147, 22);
         this.randomSlider.TabIndex = 9;
         // 
         // label2
         // 
         this.label2.AutoSize = true;
         this.label2.Location = new System.Drawing.Point(6, 44);
         this.label2.Name = "label2";
         this.label2.Size = new System.Drawing.Size(72, 13);
         this.label2.TabIndex = 4;
         this.label2.Text = "Soil Hardness";
         // 
         // hardnessSlider
         // 
         this.hardnessSlider.BackColor = System.Drawing.SystemColors.ControlLight;
         this.hardnessSlider.Location = new System.Drawing.Point(83, 44);
         this.hardnessSlider.Name = "hardnessSlider";
         this.hardnessSlider.Size = new System.Drawing.Size(147, 22);
         this.hardnessSlider.TabIndex = 3;
         // 
         // label1
         // 
         this.label1.AutoSize = true;
         this.label1.Location = new System.Drawing.Point(6, 16);
         this.label1.Name = "label1";
         this.label1.Size = new System.Drawing.Size(37, 13);
         this.label1.TabIndex = 2;
         this.label1.Text = "Power";
         // 
         // powerSilder
         // 
         this.powerSilder.BackColor = System.Drawing.SystemColors.ControlLight;
         this.powerSilder.Location = new System.Drawing.Point(83, 16);
         this.powerSilder.Name = "powerSilder";
         this.powerSilder.Size = new System.Drawing.Size(147, 22);
         this.powerSilder.TabIndex = 1;
         // 
         // button1
         // 
         this.button1.Location = new System.Drawing.Point(149, 352);
         this.button1.Name = "button1";
         this.button1.Size = new System.Drawing.Size(75, 23);
         this.button1.TabIndex = 0;
         this.button1.Text = "Apply";
         this.button1.UseVisualStyleBackColor = true;
         this.button1.Click += new System.EventHandler(this.button1_Click);
         // 
         // createMaskBox
         // 
         this.createMaskBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
         this.createMaskBox.FormattingEnabled = true;
         this.createMaskBox.Items.AddRange(new object[] {
            "No Mask",
            "Erosion & Mask",
            "Just Mask"});
         this.createMaskBox.Location = new System.Drawing.Point(103, 279);
         this.createMaskBox.Name = "createMaskBox";
         this.createMaskBox.Size = new System.Drawing.Size(121, 21);
         this.createMaskBox.TabIndex = 18;
         this.createMaskBox.SelectedIndexChanged += new System.EventHandler(this.createMaskBox_SelectedIndexChanged);
         // 
         // label7
         // 
         this.label7.AutoSize = true;
         this.label7.Location = new System.Drawing.Point(8, 279);
         this.label7.Name = "label7";
         this.label7.Size = new System.Drawing.Size(84, 13);
         this.label7.TabIndex = 19;
         this.label7.Text = "Mask Creation : ";
         // 
         // WaterPanel
         // 
         this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
         this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
         this.Controls.Add(this.groupBox1);
         this.Controls.Add(this.waterOceanPaintButt);
         this.Controls.Add(this.waterEditButt);
         this.Controls.Add(this.waterRiverButt);
         this.Controls.Add(this.waterBodyPaintButt);
         this.Key = "WaterPanel";
         this.Name = "WaterPanel";
         this.Size = new System.Drawing.Size(249, 582);
         this.Text = "Water";
         this.Load += new System.EventHandler(this.WaterPanel_Load);
         this.groupBox1.ResumeLayout(false);
         this.groupBox1.PerformLayout();
         this.ResumeLayout(false);

      }

      #endregion

      private System.Windows.Forms.Button waterBodyPaintButt;
      private System.Windows.Forms.Button waterRiverButt;
      private System.Windows.Forms.Button waterEditButt;
      private System.Windows.Forms.Button waterOceanPaintButt;
      private System.Windows.Forms.ToolTip toolTip1;
      private System.Windows.Forms.GroupBox groupBox1;
      private System.Windows.Forms.Button button1;
      private System.Windows.Forms.Label label5;
      private EditorCore.NumericSliderControl randomSlider;
      private System.Windows.Forms.Label label2;
      private EditorCore.NumericSliderControl hardnessSlider;
      private System.Windows.Forms.Label label1;
      private EditorCore.NumericSliderControl powerSilder;
      private System.Windows.Forms.CheckBox checkBox1;
      private System.Windows.Forms.Label label3;
      private EditorCore.NumericSliderControl dropLifeSlider;
      private System.Windows.Forms.Button button2;
      private System.Windows.Forms.Label label6;
      private EditorCore.NumericSliderControl channelForceSlider;
      private System.Windows.Forms.Label label7;
      private System.Windows.Forms.ComboBox createMaskBox;
   }
}
